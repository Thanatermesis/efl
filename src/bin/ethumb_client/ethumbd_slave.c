/**
 * @file
 *
 * Copyright (C) 2009 by ProFUSION embedded systems
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 *
 * @author Rafael Antognolli <antognolli@profusion.mobi>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include <Eina.h>
#include <Ecore.h>
#include <Ethumb.h>

#include "ethumbd_private.h"

#define DBG(...) EINA_LOG_DOM_DBG(_log_domain, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_domain, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_log_domain, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_domain, __VA_ARGS__)

#define NETHUMBS 100

static int _log_domain = -1;

/**
 * @brief Represents the state of a child process.
 *
 * This struct holds all the necessary data for a slave process instance,
 * including the event handler for communication with the parent and an
 * array of Ethumb objects for thumbnailing operations.
 */
struct _Ethumbd_Child
{
#ifndef _WIN32
   Ecore_Fd_Handler *fd_handler;
#else
   Ecore_Win32_Handler *fd_handler;
#endif

   Ethumb *ethumbt[NETHUMBS];
};


/**
 * @brief Safely reads a specified number of bytes from a stream.
 *
 * This function attempts to read exactly @p size bytes from @p stream into
 * @p buf. It reads one byte at a time to ensure all bytes are consumed.
 * This is a blocking read.
 *
 * @param stream The input stream to read from (e.g., stdin).
 * @param buf The buffer to store the read data.
 * @param size The number of bytes to read.
 * @return 1 on success, 0 on failure (e.g., EOF).
 */
static int
_ec_read_safe(FILE* stream, void *buf, ssize_t size)
{
   ssize_t todo;
   unsigned char *p;
   int c;

   todo = size;
   p = buf;

   while (todo > 0)
     {
        c = getc(stream);
        if (c == EOF)
          {
             ERR("could not read from stream %p", stream);
             return 0;
          }
        *p = c;
        ++p;
        --todo;
     }
   return 1;
}

/**
 * @brief Safely writes a specified number of bytes to a stream.
 *
 * This function writes exactly @p size bytes from @p buf to @p stream.
 * It writes one byte at a time. This is a blocking write.
 *
 * @param stream The output stream to write to (e.g., stdout).
 * @param buf The buffer containing data to write.
 * @param size The number of bytes to write.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_write_safe(FILE *stream, const void *buf, ssize_t size)
{
   ssize_t todo;
   const unsigned char *p;

   todo = size;
   p = buf;

   while (todo > 0)
     {
        if (putc(*p, stream) == EOF)
          {
             ERR("could not write to stream %p", stream);
             return 0;
          }
        ++p;
        --todo;
     }

   return 1;
}

/**
 * @brief Reads a length-prefixed string from the input pipe (stdin).
 *
 * The communication protocol for strings is an integer representing the
 * string length, followed by the string data itself (without a null
 * terminator). This function reads the length, allocates memory for the
 * string, reads the string data, and null-terminates it.
 *
 * @param ec The child process context (unused).
 * @param str A pointer to a char* which will be allocated and filled with
 *            the read string. The caller is responsible for freeing this
 *            memory. If no string is sent (size 0), it is set to NULL.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_pipe_str_read(struct _Ethumbd_Child *ec EINA_UNUSED, char **str)
{
   int size;
   int r;
   char buf[PATH_MAX] = { '\0' };

   r = _ec_read_safe(stdin, &size, sizeof(size));
   if (!r)
     {
	*str = NULL;
	return 0;
     }
   if ((size < 0) || (size >= PATH_MAX))
     {
	*str = NULL;
	return 0;
     }

   if (!size)
     {
	*str = NULL;
	return 1;
     }

   r = _ec_read_safe(stdin, buf, size);
   if (!r)
     {
	*str = NULL;
	return 0;
     }
   buf[size] = 0;

   *str = strdup(buf);
   return 1;
}

/**
 * @brief Creates and initializes a new Ethumbd_Child structure.
 *
 * Allocates memory for a new _Ethumbd_Child structure and initializes its
 * fields to zero.
 *
 * @return A pointer to the newly allocated _Ethumbd_Child, or NULL on failure.
 */
static struct _Ethumbd_Child *
_ec_new(void)
{
   struct _Ethumbd_Child *ec = calloc(1, sizeof(*ec));

   return ec;
}

/**
 * @brief Frees resources associated with an Ethumbd_Child.
 *
 * This function cleans up all resources held by the _Ethumbd_Child struct,
 * including deleting the Ecore fd handler and freeing all active Ethumb
 * instances.
 *
 * @param ec The Ethumbd_Child structure to free.
 */
static void
_ec_free(struct _Ethumbd_Child *ec)
{
   int i;

   if (ec->fd_handler)
     {
#ifndef _WIN32
     ecore_main_fd_handler_del(ec->fd_handler);
#else
     ecore_main_win32_handler_del(ec->fd_handler);
#endif
     }

   for (i = 0; i < NETHUMBS; i++)
     {
	if (ec->ethumbt[i])
	  ethumb_free(ec->ethumbt[i]);
     }

   free(ec);
}

/**
 * @brief Handles the 'new' operation from the parent process.
 *
 * Reads an index from stdin and creates a new Ethumb object at that
 * index in the ethumbt array. This corresponds to a client creating a new
 * Ethumb handle.
 *
 * @param ec The child process context.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_op_new(struct _Ethumbd_Child *ec)
{
   int r;
   int idx;

   r = _ec_read_safe(stdin, &idx, sizeof(idx));
   if (!r)
     return 0;
   if ((idx < 0) || (idx >= NETHUMBS))
     return 0;

   DBG("ethumbd new(). idx = %d", idx);

   ec->ethumbt[idx] = ethumb_new();
   return 1;
}

/**
 * @brief Handles the 'delete' operation from the parent process.
 *
 * Reads an index from stdin, frees the Ethumb object at that index, and
 * sets the pointer to NULL. This corresponds to a client freeing an
 * Ethumb handle.
 *
 * @param ec The child process context.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_op_del(struct _Ethumbd_Child *ec)
{
   int r;
   int idx;

   r = _ec_read_safe(stdin, &idx, sizeof(idx));
   if (!r)
     return 0;
   if ((idx < 0) || (idx >= NETHUMBS))
     return 0;

   DBG("ethumbd del(). idx = %d", idx);

   ethumb_free(ec->ethumbt[idx]);
   ec->ethumbt[idx] = NULL;
   return 1;
}

/**
 * @brief Callback executed when thumbnail generation is complete.
 *
 * This function is called by the Ethumb library when a thumbnail generation
 * process finishes. It sends the result (success or failure), along with
 * the thumbnail path and key, back to the parent process via stdout.
 *
 * The data sent to the parent has the following structure:
 * - int total_size: total size of the following data in bytes.
 * - Eina_Bool success: 1 if thumbnail was generated, 0 otherwise.
 * - int size_path: length of thumb_path string + 1 for null terminator.
 * - char thumb_path[]: the path to the generated thumbnail.
 * - int size_key: length of thumb_key string + 1 for null terminator.
 * - char thumb_key[]: the key for the generated thumbnail.
 *
 * @param data The user data passed to ethumb_generate() (unused).
 * @param e The Ethumb object.
 * @param success EINA_TRUE if generation was successful, EINA_FALSE otherwise.
 */
static void
_ec_op_generated_cb(void *data EINA_UNUSED, Ethumb *e, Eina_Bool success)
{
   const char *thumb_path, *thumb_key;
   int size_path, size_key, size_cmd;

   DBG("thumb generated (%i)!", success);
   ethumb_thumb_path_get(e, &thumb_path, &thumb_key);

   if (!thumb_path)
     size_path = 0;
   else
     size_path = strlen(thumb_path) + 1;

   if (!thumb_key)
     size_key = 0;
   else
     size_key = strlen(thumb_key) + 1;

   size_cmd = sizeof(success) + sizeof(size_path) + size_path +
      sizeof(size_key) + size_key;

   _ec_write_safe(stdout, &size_cmd, sizeof(size_cmd));
   _ec_write_safe(stdout, &success, sizeof(success));

   _ec_write_safe(stdout, &size_path, sizeof(size_path));
   _ec_write_safe(stdout, thumb_path, size_path);

   _ec_write_safe(stdout, &size_key, sizeof(size_key));
   _ec_write_safe(stdout, thumb_key, size_key);
   fflush(stdout);
}

/**
 * @brief Handles the 'generate' thumbnail operation.
 *
 * Reads all necessary parameters for thumbnail generation from stdin,
 * including the Ethumb object index, file path, key, and destination
 * thumbnail path/key. It then initiates the thumbnail generation. If the
 * thumbnail already exists, the completion callback is invoked immediately.
 * Otherwise, generation is started asynchronously.
 *
 * @param ec The child process context.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_op_generate(struct _Ethumbd_Child *ec)
{
   int idx;
   char *path, *key, *thumb_path, *thumb_key;
   int r;

   r = _ec_read_safe(stdin, &idx, sizeof(idx));
   if (!r)
     return 0;
   if ((idx < 0) || (idx >= NETHUMBS))
     return 0;

   r = _ec_pipe_str_read(ec, &path);
   if (!r)
     return 0;
   r = _ec_pipe_str_read(ec, &key);
   if (!r)
     {
        free(path);
        return 0;
     }
   r = _ec_pipe_str_read(ec, &thumb_path);
   if (!r)
     {
        free(path);
        free(key);
        return 0;
     }
   r = _ec_pipe_str_read(ec, &thumb_key);
   if (!r)
     {
        free(path);
        free(key);
        free(thumb_path);
        return 0;
     }

   ethumb_file_set(ec->ethumbt[idx], path, key);
   ethumb_thumb_path_set(ec->ethumbt[idx], thumb_path, thumb_key);

   if (ethumb_exists(ec->ethumbt[idx]))
     {
        _ec_op_generated_cb(ec, ec->ethumbt[idx], EINA_TRUE);
     }
   else
     {
        ethumb_generate(ec->ethumbt[idx], _ec_op_generated_cb, ec, NULL);
     }

   free(path);
   free(key);
   free(thumb_path);
   free(thumb_key);

   return 1;
}

/**
 * @brief Sets the FDO (freedesktop.org) compliant thumbnailing option.
 *
 * Reads a boolean value from stdin and configures the Ethumb object
 * to follow or ignore the FDO thumbnailing specification.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_fdo_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   int value;

   r = _ec_read_safe(stdin, &value, sizeof(value));
   if (!r)
     return 0;
   ethumb_thumb_fdo_set(e, value);
   DBG("fdo = %d", value);

   return 1;
}

/**
 * @brief Sets the thumbnail size.
 *
 * Reads width and height from stdin and applies them to the Ethumb object.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_size_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   int w, h;
   int type;

   r = _ec_read_safe(stdin, &w, sizeof(w));
   if (!r)
     return 0;
   r = _ec_read_safe(stdin, &type, sizeof(type));
   if (!r)
     return 0;
   r = _ec_read_safe(stdin, &h, sizeof(h));
   if (!r)
     return 0;
   ethumb_thumb_size_set(e, w, h);
   DBG("size = %dx%d", w, h);

   return 1;
}

/**
 * @brief Sets the thumbnail image format.
 *
 * Reads a format enum from stdin and applies it to the Ethumb object.
 * Example formats could be ETHUMB_THUMB_FMT_PNG or ETHUMB_THUMB_FMT_JPEG.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_format_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   int value;

   r = _ec_read_safe(stdin, &value, sizeof(value));
   if (!r)
     return 0;
   ethumb_thumb_format_set(e, value);
   DBG("format = %d", value);

   return 1;
}

/**
 * @brief Sets the aspect ratio handling for the thumbnail.
 *
 * Reads an aspect mode from stdin and applies it to the Ethumb object.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_aspect_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   int value;

   r = _ec_read_safe(stdin, &value, sizeof(value));
   if (!r)
     return 0;
   ethumb_thumb_aspect_set(e, value);
   DBG("aspect = %d", value);

   return 1;
}

/**
 * @brief Sets the thumbnail orientation.
 *
 * Reads an orientation mode from stdin and applies it. This is used to
 * rotate the thumbnail based on e.g. EXIF data.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_orientation_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   int value;

   r = _ec_read_safe(stdin, &value, sizeof(value));
   if (!r)
     return 0;
   ethumb_thumb_orientation_set(e, value);
   DBG("orientation = %d", value);

   return 1;
}

/**
 * @brief Sets the thumbnail crop alignment.
 *
 * Reads x and y float values for alignment from stdin and applies them.
 * These are values between 0.0 and 1.0.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_crop_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   float x, y;
   int type;

   r = _ec_read_safe(stdin, &x, sizeof(x));
   if (!r)
     return 0;
   r = _ec_read_safe(stdin, &type, sizeof(type));
   if (!r)
     return 0;
   r = _ec_read_safe(stdin, &y, sizeof(y));
   if (!r)
     return 0;
   ethumb_thumb_crop_align_set(e, x, y);
   DBG("crop = %fx%f", x, y);

   return 1;
}

/**
 * @brief Sets the quality for lossy formats like JPEG.
 *
 * Reads an integer quality value (0-100) from stdin and applies it.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_quality_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   int value;

   r = _ec_read_safe(stdin, &value, sizeof(value));
   if (!r)
     return 0;
   ethumb_thumb_quality_set(e, value);
   DBG("quality = %d", value);

   return 1;
}

/**
 * @brief Sets the compression level for formats like PNG.
 *
 * Reads an integer compression value (0-9) from stdin and applies it.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_compress_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   int value;

   r = _ec_read_safe(stdin, &value, sizeof(value));
   if (!r)
     return 0;
   ethumb_thumb_compress_set(e, value);
   DBG("compress = %d", value);

   return 1;
}

/**
 * @brief Sets a decorative frame for the thumbnail.
 *
 * Reads the Edje theme file, group name, and swallow name from stdin and
 * configures the Ethumb object to render a frame around the thumbnail.
 *
 * @param ec The child process context.
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_frame_set(struct _Ethumbd_Child *ec, Ethumb *e)
{
   int r;
   int type;
   char *theme_file, *group, *swallow;

   r = _ec_pipe_str_read(ec, &theme_file);
   if (!r)
     return 0;
   r = _ec_read_safe(stdin, &type, sizeof(type));
   if (!r)
     {
        free(theme_file);
        return 0;
     }
   r = _ec_pipe_str_read(ec, &group);
   if (!r)
     {
        free(theme_file);
        return 0;
     }
   r = _ec_read_safe(stdin, &type, sizeof(type));
   if (!r)
     {
        free(theme_file);
        free(group);
        return 0;
     }
   r = _ec_pipe_str_read(ec, &swallow);
   if (!r)
     {
        free(theme_file);
        free(group);
        return 0;
     }
   DBG("frame = %s:%s:%s", theme_file, group, swallow);
   ethumb_frame_set(e, theme_file, group, swallow);
   free(theme_file);
   free(group);
   free(swallow);

   return 1;
}

/**
 * @brief Sets the directory where the thumbnail will be saved.
 *
 * Reads a directory path from stdin and applies it.
 *
 * @param ec The child process context.
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_directory_set(struct _Ethumbd_Child *ec, Ethumb *e)
{
   int r;
   char *directory;

   r = _ec_pipe_str_read(ec, &directory);
   if (!r)
     return 0;
   ethumb_thumb_dir_path_set(e, directory);
   DBG("directory = %s", directory);
   free(directory);

   return 1;
}

/**
 * @brief Sets the category of the thumbnail.
 *
 * The category is used as a subdirectory within the thumbnail directory
 * to organize thumbnails. Reads the category string from stdin.
 *
 * @param ec The child process context.
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_category_set(struct _Ethumbd_Child *ec, Ethumb *e)
{
   int r;
   char *category;

   r = _ec_pipe_str_read(ec, &category);
   if (!r)
     return 0;
   ethumb_thumb_category_set(e, category);
   DBG("category = %s", category);
   free(category);

   return 1;
}

/**
 * @brief Sets the time position for video thumbnails.
 *
 * Reads a float value (in seconds) from stdin to specify the frame to
 * use for the thumbnail.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_video_time_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   float value;

   r = _ec_read_safe(stdin, &value, sizeof(value));
   if (!r)
     return 0;
   ethumb_video_time_set(e, value);
   DBG("video_time = %f", value);

   return 1;
}

/**
 * @brief Sets the start time for a sequence of video thumbnails.
 *
 * Reads a float value (in seconds) from stdin.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_video_start_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   float value;

   r = _ec_read_safe(stdin, &value, sizeof(value));
   if (!r)
     return 0;
   ethumb_video_start_set(e, value);
   DBG("video_start = %f", value);

   return 1;
}

/**
 * @brief Sets the interval between thumbnails in a sequence.
 *
 * Reads a float value (in seconds) from stdin.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_video_interval_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   float value;

   r = _ec_read_safe(stdin, &value, sizeof(value));
   if (!r)
     return 0;
   ethumb_video_interval_set(e, value);
   DBG("video_interval = %f", value);

   return 1;
}

/**
 * @brief Sets the number of thumbnails to generate from a video.
 *
 * Reads an integer value from stdin.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_video_ntimes_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   int value;

   r = _ec_read_safe(stdin, &value, sizeof(value));
   if (!r)
     return 0;
   ethumb_video_ntimes_set(e, value);
   DBG("video_ntimes = %d", value);

   return 1;
}

/**
 * @brief Sets the frames-per-second for video processing.
 *
 * Reads an integer value from stdin.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_video_fps_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   int value;

   r = _ec_read_safe(stdin, &value, sizeof(value));
   if (!r)
     return 0;
   ethumb_video_fps_set(e, value);
   DBG("video_fps = %d", value);

   return 1;
}

/**
 * @brief Sets the page number for document thumbnails.
 *
 * Reads an integer page number from stdin.
 *
 * @param ec The child process context (unused).
 * @param e The Ethumb object to configure.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_document_page_set(struct _Ethumbd_Child *ec EINA_UNUSED, Ethumb *e)
{
   int r;
   int value;

   r = _ec_read_safe(stdin, &value, sizeof(value));
   if (!r)
     return 0;
   ethumb_document_page_set(e, value);
   DBG("document_page = %d", value);

   return 1;
}

/**
 * @brief Dispatches and processes a single setup command.
 *
 * Based on the @p type parameter, this function calls the appropriate
 * `_ec_*_set` function to configure the Ethumb object at @p idx.
 *
 * @param ec The child process context.
 * @param idx The index of the Ethumb object to configure.
 * @param type The type of setup operation to perform.
 */
static void
_ec_setup_process(struct _Ethumbd_Child *ec, int idx, int type)
{
   Ethumb *e;

   e = ec->ethumbt[idx];

   switch (type)
     {
      case ETHUMBD_FDO:
	 _ec_fdo_set(ec, e);
	 break;
      case ETHUMBD_SIZE_W:
	 _ec_size_set(ec, e);
	 break;
      case ETHUMBD_FORMAT:
	 _ec_format_set(ec, e);
	 break;
      case ETHUMBD_ASPECT:
	 _ec_aspect_set(ec, e);
	 break;
      case ETHUMBD_ORIENTATION:
	 _ec_orientation_set(ec, e);
	 break;
      case ETHUMBD_CROP_X:
	 _ec_crop_set(ec, e);
	 break;
      case ETHUMBD_QUALITY:
	 _ec_quality_set(ec, e);
	 break;
      case ETHUMBD_COMPRESS:
	 _ec_compress_set(ec, e);
	 break;
      case ETHUMBD_FRAME_FILE:
	 _ec_frame_set(ec, e);
	 break;
      case ETHUMBD_DIRECTORY:
	 _ec_directory_set(ec, e);
	 break;
      case ETHUMBD_CATEGORY:
	 _ec_category_set(ec, e);
	 break;
      case ETHUMBD_VIDEO_TIME:
	 _ec_video_time_set(ec, e);
	 break;
      case ETHUMBD_VIDEO_START:
	 _ec_video_start_set(ec, e);
	 break;
      case ETHUMBD_VIDEO_INTERVAL:
	 _ec_video_interval_set(ec, e);
	 break;
      case ETHUMBD_VIDEO_NTIMES:
	 _ec_video_ntimes_set(ec, e);
	 break;
      case ETHUMBD_VIDEO_FPS:
	 _ec_video_fps_set(ec, e);
	 break;
      case ETHUMBD_DOCUMENT_PAGE:
	 _ec_document_page_set(ec, e);
	 break;
      default:
	 ERR("wrong type!");
     }
}

/**
 * @brief Handles a sequence of setup operations for an Ethumb object.
 *
 * This function reads setup commands from stdin in a loop until a
 * ETHUMBD_SETUP_FINISHED command is received. It reads the Ethumb object
 * index, then iteratively reads command types and calls
 * _ec_setup_process() for each one.
 *
 * @param ec The child process context.
 * @return 1 on success, 0 on failure.
 */
static int
_ec_op_setup(struct _Ethumbd_Child *ec)
{
   int r;
   int idx;
   int type;

   r = _ec_read_safe(stdin, &idx, sizeof(idx));
   if (!r)
     return 0;
   if ((idx < 0) || (idx >= NETHUMBS))
     return 0;

   r = _ec_read_safe(stdin, &type, sizeof(type));
   if (!r)
     return 0;
   while (type != ETHUMBD_SETUP_FINISHED)
     {
	_ec_setup_process(ec, idx, type);
	r = _ec_read_safe(stdin, &type, sizeof(type));
	if (!r)
	  return 0;
     }

   return 1;
}

/**
 * @brief Ecore file descriptor handler for parent communication.
 *
 * This function is the heart of the slave's event loop. It's called by
 * Ecore whenever there is data to be read from stdin. It reads an
 * operation ID and dispatches to the corresponding `_ec_op_*` function.
 * If the pipe closes or an error occurs, it quits the main loop.
 *
 * @param data The user data, a pointer to the _Ethumbd_Child struct.
 * @param fd_handler The Ecore_Fd_Handler that triggered the callback. On Windows, this is an Ecore_Win32_Handler.
 * @return 1 (ECORE_CALLBACK_RENEW) to continue processing, or 0 (ECORE_CALLBACK_CANCEL) on error.
 */
#ifndef _WIN32
static Eina_Bool
_ec_fd_handler(void *data, Ecore_Fd_Handler *fd_handler)
#else
static Eina_Bool
_ec_fd_handler(void *data, Ecore_Win32_Handler *fd_handler EINA_UNUSED)
#endif
{
   struct _Ethumbd_Child *ec = data;
   int op_id;
   int r;

#ifndef _WIN32
   if (ecore_main_fd_handler_active_get(fd_handler, ECORE_FD_ERROR))
     {
	ERR("error on pipein! child exiting...");
	ec->fd_handler = NULL;
	ecore_main_loop_quit();
	return 0;
     }
#endif

   r = _ec_read_safe(stdin, &op_id, sizeof(op_id));
   if (!r)
     {
	DBG("ethumbd exited! child exiting...");
	ec->fd_handler = NULL;
	ecore_main_loop_quit();
	return 0;
     }

   DBG("received op: %d", op_id);

   switch (op_id)
     {
      case ETHUMBD_OP_NEW:
	 r = _ec_op_new(ec);
	 break;
      case ETHUMBD_OP_GENERATE:
	 r = _ec_op_generate(ec);
	 break;
      case ETHUMBD_OP_SETUP:
	 r = _ec_op_setup(ec);
	 break;
      case ETHUMBD_OP_DEL:
	 r = _ec_op_del(ec);
	 break;
      default:
	 ERR("invalid operation: %d", op_id);
	 r = 0;
     }

   if (!r)
     {
	ERR("ethumbd exited! child exiting...");
	ec->fd_handler = NULL;
	ecore_main_loop_quit();
     }

   return r;
}

/**
 * @brief Sets up the main communication channel with the parent.
 *
 * This function registers an Ecore file descriptor handler that will listen
 * for incoming data on stdin.
 *
 * @param ec The child process context.
 */
static void
_ec_setup(struct _Ethumbd_Child *ec)
{
#ifndef _WIN32
   ec->fd_handler = ecore_main_fd_handler_add(
      STDIN_FILENO, ECORE_FD_READ | ECORE_FD_ERROR,
      _ec_fd_handler, ec, NULL, NULL);
#else
   ec->fd_handler = ecore_main_win32_handler_add(
      GetStdHandle(STD_INPUT_HANDLE),
      _ec_fd_handler, ec);
#endif
}

/**
 * @brief Main function of the ethumbd slave process.
 *
 * Initializes Eina, Ecore, and Ethumb. Creates the child context,
 * sets up the communication handler, and starts the Ecore main loop.
 * Cleans up resources on exit.
 *
 * @param argc Argument count (unused).
 * @param argv Argument vector (unused).
 * @return 0 on successful shutdown, 1 on initialization failure.
 */
int
main(int argc EINA_UNUSED, const char *argv[] EINA_UNUSED)
{
   struct _Ethumbd_Child *ec;

   ethumb_init();

   if (_log_domain < 0)
     {
	_log_domain = eina_log_domain_register("ethumbd_child", NULL);

	if (_log_domain < 0)
	  {
	     EINA_LOG_CRIT("could not register log domain 'ethumbd_child'");
	     ethumb_shutdown();
	     return 1;
	  }
     }

   ec = _ec_new();

   _ec_setup(ec);

   DBG("child started!");
   ecore_main_loop_begin();
   DBG("child finishing.");

   _ec_free(ec);

   if (_log_domain >= 0)
     {
	eina_log_domain_unregister(_log_domain);
	_log_domain = -1;
     }
   ethumb_shutdown();

   return 0;
}
