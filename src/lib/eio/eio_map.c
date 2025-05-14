/* EIO - EFL data type library
 * Copyright (C) 2010 Enlightenment Developers:
 *           Cedric Bail <cedric.bail@free.fr>
 *           Vincent "caro" Torri  <vtorri at univ-evry dot fr>
 *           Stephen "okra" Houston <UnixTitan@gmail.com>
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
 */

/**
 * @file
 * @brief These routines are used for asynchronous file and memory map operations.
 *
 * It provides function to open, close, and map files in a non-blocking way
 * using Ecore_Thread for background processing.
 */

#include "eio_private.h"
#include "Eio.h"


/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @brief Job function to open a file.
 *
 * This function is executed in a separate thread. It attempts to open the
 * file specified in the Eio_File_Map structure.
 *
 * @param data Pointer to the Eio_File_Map structure.
 * @param thread Pointer to the Ecore_Thread executing this job.
 */
static void
_eio_file_open_job(void *data, Ecore_Thread *thread)
{
   Eio_File_Map *map = data;

   map->result = eina_file_open(map->name, map->shared);
   if (!map->result) eio_file_thread_error(&map->common, thread);
}

/**
 * @brief Frees resources associated with an Eio_File_Map structure used for file opening.
 *
 * @param map Pointer to the Eio_File_Map structure to free.
 */
static void
_eio_file_open_free(Eio_File_Map *map)
{
   if (map->name) eina_stringshare_del(map->name);
   eio_file_free((Eio_File*)map);
}

/**
 * @brief Callback function executed in the main loop after a file open job completes successfully.
 *
 * @param data Pointer to the Eio_File_Map structure.
 * @param thread Pointer to the Ecore_Thread that executed the job (unused).
 */
static void
_eio_file_open_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Map *map = data;

   map->open_cb((void*) map->common.data, &map->common, map->result);
   _eio_file_open_free(map);
}

/**
 * @brief Callback function executed in the main loop if a file open job is cancelled or fails.
 *
 * @param data Pointer to the Eio_File_Map structure.
 * @param thread Pointer to the Ecore_Thread that was supposed to execute the job (unused).
 */
static void
_eio_file_open_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Map *map = data;

   eio_file_error(&map->common);
   _eio_file_open_free(map);
}

/**
 * @brief Job function to close a file.
 *
 * This function is executed in a separate thread. It retrieves the file size
 * and then closes the file.
 *
 * @param data Pointer to the Eio_File_Map structure.
 * @param thread Pointer to the Ecore_Thread executing this job (unused).
 */
static void
_eio_file_close_job(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Map *map = data;

   map->common.length = eina_file_size_get(map->result);
   eina_file_close(map->result);
}

/**
 * @brief Callback function executed in the main loop after a file close job completes successfully.
 *
 * @param data Pointer to the Eio_File_Map structure.
 * @param thread Pointer to the Ecore_Thread that executed the job (unused).
 */
static void
_eio_file_close_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Map *map = data;

   map->common.done_cb((void*) map->common.data, &map->common);
   _eio_file_open_free(map);
}

/**
 * @brief Callback function executed in the main loop if a file close job is cancelled or fails.
 *
 * @param data Pointer to the Eio_File_Map structure.
 * @param thread Pointer to the Ecore_Thread that was supposed to execute the job (unused).
 */
static void
_eio_file_close_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Map *map = data;

   eio_file_error(&map->common);
   _eio_file_open_free(map);
}

/**
 * @brief Job function to map an entire file into memory.
 *
 * This function is executed in a separate thread. It maps the entire file
 * specified in the Eio_File_Map_Rule structure. If a filter callback is provided,
 * it is called to potentially reject the map.
 *
 * @param data Pointer to the Eio_File_Map_Rule structure.
 * @param thread Pointer to the Ecore_Thread executing this job.
 */
static void
_eio_file_map_all_job(void *data, Ecore_Thread *thread)
{
   Eio_File_Map_Rule *map = data;

   eio_file_container_set(&map->common, map->file);
   map->result = eina_file_map_all(map->common.container, map->rule);
   if (map->result && map->filter_cb)
     {
        if (!map->filter_cb((void*) map->common.data,
                            &map->common,
                            map->result,
			    map->length))
          {
             eina_file_map_free(map->common.container, map->result);
             map->result = NULL;
          }
     }

   if (!map->result)
     eio_file_thread_error(&map->common, thread);
}

/**
 * @brief Job function to map a specific region of a file into memory.
 *
 * This function is executed in a separate thread. It maps a region of the file
 * specified by offset and length in the Eio_File_Map_Rule structure.
 * If a filter callback is provided, it is called to potentially reject the map.
 *
 * @param data Pointer to the Eio_File_Map_Rule structure.
 * @param thread Pointer to the Ecore_Thread executing this job.
 */
static void
_eio_file_map_new_job(void *data, Ecore_Thread *thread)
{
   Eio_File_Map_Rule *map = data;

   eio_file_container_set(&map->common, map->file);
   map->result = eina_file_map_new(map->common.container, map->rule,
                                   map->offset, map->length);
   if (map->result && map->filter_cb)
     {
        if (!map->filter_cb((void*) map->common.data,
                            &map->common,
                            map->result,
			    map->length))
          {
             eina_file_map_free(map->common.container, map->result);
             map->result = NULL;
          }
     }

   if (!map->result)
     eio_file_thread_error(&map->common, thread);
}

/**
 * @brief Callback function executed in the main loop after a file map job completes successfully.
 *
 * @param data Pointer to the Eio_File_Map_Rule structure.
 * @param thread Pointer to the Ecore_Thread that executed the job (unused).
 */
static void
_eio_file_map_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Map_Rule *map = data;

   map->map_cb((void*) map->common.data, &map->common, map->result, map->length);
   eio_file_free((Eio_File*)map);
}

/**
 * @brief Callback function executed in the main loop if a file map job is cancelled or fails.
 *
 * @param data Pointer to the Eio_File_Map_Rule structure.
 * @param thread Pointer to the Ecore_Thread that was supposed to execute the job (unused).
 */
static void
_eio_file_map_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Map_Rule *map = data;

   eio_file_error(&map->common);
   eio_file_free((Eio_File*)map);
}

/**
 * @endcond
 */


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */


/**
 * @endcond
 */

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @brief Asynchronously opens a file.
 *
 * This function queues a request to open a file. The actual file opening
 * is performed in a separate thread.
 *
 * @param name The path to the file to open.
 * @param shared EINA_TRUE if the file should be opened with shared access, EINA_FALSE otherwise.
 * @param open_cb Callback function to be called when the file is successfully opened.
 *                The opened Eina_File handle is passed to this callback.
 * @param error_cb Callback function to be called if an error occurs during file opening.
 * @param data Custom data to be passed to the callback functions.
 * @return An Eio_File handle representing the asynchronous operation, or NULL on failure to queue.
 *         The Eina_File* itself is delivered via the open_cb.
 */
EIO_API Eio_File *
eio_file_open(const char *name, Eina_Bool shared,
	      Eio_Open_Cb open_cb,
	      Eio_Error_Cb error_cb,
	      const void *data)
{
   Eio_File_Map *map;

   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(open_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   map = malloc(sizeof (Eio_File_Map));
   EINA_SAFETY_ON_NULL_RETURN_VAL(map, NULL);

   map->open_cb = open_cb;
   map->name = eina_stringshare_add(name);
   map->shared = shared;
   map->result = NULL;

   if (!eio_file_set(&map->common,
                     NULL,
                     error_cb,
                     data,
                     _eio_file_open_job,
                     _eio_file_open_end,
                     _eio_file_open_cancel))
     return NULL;

   return &map->common;
}

/**
 * @brief Asynchronously closes an opened file.
 *
 * This function queues a request to close an already opened file.
 * The actual file closing is performed in a separate thread.
 *
 * @param f The Eina_File handle to close. This handle must have been obtained
 *          from a successful eio_file_open() operation or similar.
 * @param done_cb Callback function to be called when the file is successfully closed.
 * @param error_cb Callback function to be called if an error occurs during file closing.
 * @param data Custom data to be passed to the callback functions.
 * @return An Eio_File handle representing the asynchronous operation, or NULL on failure to queue.
 */
EIO_API Eio_File *
eio_file_close(Eina_File *f,
               Eio_Done_Cb done_cb,
               Eio_Error_Cb error_cb,
               const void *data)
{
   Eio_File_Map *map;

   EINA_SAFETY_ON_NULL_RETURN_VAL(f, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   map = malloc(sizeof (Eio_File_Map));
   EINA_SAFETY_ON_NULL_RETURN_VAL(map, NULL);

   map->name = NULL;
   map->result = f;

   if (!eio_file_set(&map->common,
                     done_cb,
                     error_cb,
                     data,
                     _eio_file_close_job,
                     _eio_file_close_end,
                     _eio_file_close_cancel))
     return NULL;

   return &map->common;
}

/**
 * @brief Asynchronously maps an entire file into memory.
 *
 * This function queues a request to map an entire opened file into memory.
 * The actual mapping is performed in a separate thread.
 *
 * @param f The Eina_File handle of the file to map.
 * @param rule The population rule for the memory map (e.g., EINA_FILE_POPULATE, EINA_FILE_WILLNEED).
 * @param filter_cb Optional callback function to filter/validate the mapped memory region
 *                  before the main map_cb is called. If this callback returns EINA_FALSE,
 *                  the map is considered failed/rejected.
 * @param map_cb Callback function to be called when the file is successfully mapped.
 *               The pointer to the mapped memory and its length are passed to this callback.
 * @param error_cb Callback function to be called if an error occurs during mapping.
 * @param data Custom data to be passed to the callback functions.
 * @return An Eio_File handle representing the asynchronous operation, or NULL on failure to queue.
 *         The mapped memory region is delivered via the map_cb.
 */
EIO_API Eio_File *
eio_file_map_all(Eina_File *f,
                 Eina_File_Populate rule,
                 Eio_Filter_Map_Cb filter_cb,
                 Eio_Map_Cb map_cb,
                 Eio_Error_Cb error_cb,
                 const void *data)
{
   Eio_File_Map_Rule *map;

   EINA_SAFETY_ON_NULL_RETURN_VAL(f, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(map_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   map = malloc(sizeof (Eio_File_Map_Rule));
   EINA_SAFETY_ON_NULL_RETURN_VAL(map, NULL);

   map->file = f;
   map->filter_cb = filter_cb;
   map->map_cb = map_cb;
   map->rule = rule;
   map->result = NULL;
   map->length = eina_file_size_get(f);

   if (!eio_file_set(&map->common,
                     NULL,
                     error_cb,
                     data,
                     _eio_file_map_all_job,
                     _eio_file_map_end,
                     _eio_file_map_cancel))
     return NULL;

   return &map->common;
}

/**
 * @brief Asynchronously maps a specific region of a file into memory.
 *
 * This function queues a request to map a specified region of an opened file into memory.
 * The actual mapping is performed in a separate thread.
 *
 * @param f The Eina_File handle of the file to map.
 * @param rule The population rule for the memory map (e.g., EINA_FILE_POPULATE, EINA_FILE_WILLNEED).
 * @param offset The starting offset within the file for the memory map.
 * @param length The length of the region to map.
 * @param filter_cb Optional callback function to filter/validate the mapped memory region
 *                  before the main map_cb is called. If this callback returns EINA_FALSE,
 *                  the map is considered failed/rejected.
 * @param map_cb Callback function to be called when the region is successfully mapped.
 *               The pointer to the mapped memory and its length are passed to this callback.
 * @param error_cb Callback function to be called if an error occurs during mapping.
 * @param data Custom data to be passed to the callback functions.
 * @return An Eio_File handle representing the asynchronous operation, or NULL on failure to queue.
 *         The mapped memory region is delivered via the map_cb.
 */
EIO_API Eio_File *
eio_file_map_new(Eina_File *f,
                 Eina_File_Populate rule,
                 unsigned long int offset,
                 unsigned long int length,
                 Eio_Filter_Map_Cb filter_cb,
                 Eio_Map_Cb map_cb,
                 Eio_Error_Cb error_cb,
                 const void *data)
{
   Eio_File_Map_Rule *map;

   EINA_SAFETY_ON_NULL_RETURN_VAL(f, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(map_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   map = malloc(sizeof (Eio_File_Map_Rule));
   EINA_SAFETY_ON_NULL_RETURN_VAL(map, NULL);

   map->file = f;
   map->filter_cb = filter_cb;
   map->map_cb = map_cb;
   map->rule = rule;
   map->result = NULL;
   map->offset = offset;
   map->length = length;

   if (!eio_file_set(&map->common,
                     NULL,
                     error_cb,
                     data,
                     _eio_file_map_new_job,
                     _eio_file_map_end,
                     _eio_file_map_cancel))
     return NULL;

   return &map->common;
}
