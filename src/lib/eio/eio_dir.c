/* EIO - EFL data type library
 * Copyright (C) 2010 Enlightenment Developers:
 *           Cedric Bail <cedric.bail@free.fr>
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

#ifdef _WIN32
# include <evil_private.h> /* mkdir */
#endif

#include "eio_private.h"
#include "Eio.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @brief Simple strcmp wrapper for qsort/eina_list_sort.
 * @param a First string.
 * @param b Second string.
 * @return strcmp result.
 */
static int
eio_strcmp(const void *a, const void *b)
{
   return strcmp(a, b);
}

/**
 * @brief Callback function used during recursive directory listing for copy/move operations.
 *
 * This function is called for each item found in the directory. It applies an optional
 * filter and categorizes the item (file, directory, link) into appropriate lists
 * within the Eio_Dir_Copy structure.
 *
 * @param copy The Eio_Dir_Copy context, holding lists for files, dirs, and links.
 * @param handler The Eio_File handler associated with this operation.
 * @param info Information about the currently processed file/directory.
 * @return EINA_TRUE to continue processing, EINA_FALSE if an error occurred or filtering stopped it.
 */
static Eina_Bool
_eio_dir_recursive_progress(Eio_Dir_Copy *copy, Eio_File *handler, const Eina_File_Direct_Info *info)
{
   if (copy->filter_cb && !copy->filter_cb(&copy->progress.common.data, handler, info))
     return EINA_FALSE;

   switch (info->type)
     {
      case EINA_FILE_UNKNOWN:
         eio_file_thread_error(&copy->progress.common, handler->thread);
         return EINA_FALSE;
      case EINA_FILE_LNK:
         copy->links = eina_list_append(copy->links, eina_stringshare_add(info->path));
         break;
      case EINA_FILE_DIR:
         copy->dirs = eina_list_append(copy->dirs, eina_stringshare_add(info->path));
         break;
      default:
         copy->files = eina_list_append(copy->files, eina_stringshare_add(info->path));
         break;
     }

   return EINA_TRUE;
}

/**
 * @brief Core recursive directory listing function.
 *
 * This function traverses a directory tree starting from 'target'. For each entry,
 * it calls 'filter_cb'. If the entry is a directory and passes the filter,
 * the function recurses into it.
 *
 * @param thread The worker thread performing the listing.
 * @param common The Eio_File common structure for error reporting and context.
 * @param filter_cb A callback function to filter entries. It receives 'data',
 *        'common', and 'info' for the current entry.
 * @param Eina_File_Ls A function pointer (e.g., eina_file_stat_ls or eina_file_direct_ls)
 *        used to list directory contents. This determines if stat calls are made.
 * @param data User data to be passed to 'filter_cb'.
 * @param target The path to the directory to list.
 * @return EINA_TRUE on success, EINA_FALSE on error or if the thread was cancelled.
 */
static Eina_Bool
_eio_file_recursiv_ls(Ecore_Thread *thread,
                      Eio_File *common,
                      Eio_Filter_Direct_Cb filter_cb,
		      Eina_Iterator *(*Eina_File_Ls)(const char *target),
                      void *data,
                      const char *target)
{
   Eina_File_Direct_Info *info;
   Eina_Iterator *it = NULL;
   Eina_List *dirs = NULL;
   const char *dir;

   it = Eina_File_Ls(target);
   if (!it)
     {
        eio_file_thread_error(common, thread);
        return EINA_FALSE;
     }

   eio_file_container_set(common, eina_iterator_container_get(it));

   EINA_ITERATOR_FOREACH(it, info)
     {
        Eina_Bool filter = EINA_TRUE;
        _eio_stat_t buffer;

        switch (info->type)
          {
           case EINA_FILE_DIR:
              if (_eio_lstat(info->path, &buffer) != 0)
		continue;

#ifndef _WIN32
              if (S_ISLNK(buffer.st_mode))
                info->type = EINA_FILE_LNK;
#endif
           default:
              break;
          }

        filter = filter_cb(data, common, info);
        if (filter && info->type == EINA_FILE_DIR)
          dirs = eina_list_append(dirs, eina_stringshare_add(info->path));

        if (ecore_thread_check(thread))
          goto on_error;
     }

   eio_file_container_set(common, NULL);

   eina_iterator_free(it);
   it = NULL;

   EINA_LIST_FREE(dirs, dir)
     {
       Eina_Bool err;

       err = !_eio_file_recursiv_ls(thread, common, filter_cb, Eina_File_Ls, data, dir);

       eina_stringshare_del(dir);
       if (err) goto on_error;
     }

   return EINA_TRUE;

 on_error:
   if (it) eina_iterator_free(it);

   EINA_LIST_FREE(dirs, dir)
     eina_stringshare_del(dir);

   return EINA_FALSE;
}

/**
 * @brief Wrapper for _eio_file_recursiv_ls specific to directory copy/move preparations.
 *
 * This function uses _eio_file_recursiv_ls with _eio_dir_recursive_progress as the
 * filter callback to populate the Eio_Dir_Copy structure with lists of files,
 * directories, and links to be processed. It uses eina_file_stat_ls to get file info.
 *
 * @param thread The worker thread.
 * @param copy The Eio_Dir_Copy context to populate.
 * @param target The source directory path to list.
 * @return EINA_TRUE on success, EINA_FALSE on error.
 */
static Eina_Bool
_eio_dir_recursiv_ls(Ecore_Thread *thread, Eio_Dir_Copy *copy, const char *target)
{
   if (!_eio_file_recursiv_ls(thread, &copy->progress.common,
                              (Eio_Filter_Direct_Cb) _eio_dir_recursive_progress,
			      eina_file_stat_ls,
                              copy, target))
     return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * @brief Initializes parameters and structures for a directory copy or move operation.
 *
 * This function calculates the total number of items to process, sorts the lists
 * of files, directories, and links, prepares path lengths, and creates the
 * destination directory if it doesn't exist.
 *
 * @param thread The worker thread.
 * @param[out] step Pointer to store the initial progress step (usually 0).
 * @param[out] count Pointer to store the total number of items to process.
 * @param[out] length_source Pointer to store the length of the source path.
 * @param[out] length_dest Pointer to store the length of the destination path.
 * @param order The Eio_Dir_Copy structure containing lists of items and operation details.
 * @param progress An Eio_File_Progress structure to be initialized for sub-operations.
 * @return EINA_TRUE on success, EINA_FALSE on error.
 */
static Eina_Bool
_eio_dir_init(Ecore_Thread *thread,
              long long *step, long long *count,
              int *length_source, int *length_dest,
              Eio_Dir_Copy *order,
              Eio_File_Progress *progress)
{
   struct stat buffer;

   /* notify main thread of the amount of work todo */
   *step = 0;
   *count = eina_list_count(order->files)
     + eina_list_count(order->dirs) * 2
     + eina_list_count(order->links);
   eio_progress_send(thread, &order->progress, *step, *count);

   /* sort the content, so we create the directory in the right order */
   order->dirs = eina_list_sort(order->dirs, -1, eio_strcmp);
   order->files = eina_list_sort(order->files, -1, eio_strcmp);
   order->links = eina_list_sort(order->links, -1, eio_strcmp);

   /* prepare stuff */
   *length_source = eina_stringshare_strlen(order->progress.source);
   *length_dest = eina_stringshare_strlen(order->progress.dest);

   memcpy(progress, &order->progress, sizeof (Eio_File_Progress));
   progress->source = NULL;
   progress->dest = NULL;

   /* create destination dir if not available */
   if (stat(order->progress.dest, &buffer) != 0)
     {
        if (stat(order->progress.source, &buffer) != 0)
          {
             eio_file_thread_error(&order->progress.common, thread);
             return EINA_FALSE;
          }

        if (mkdir(order->progress.dest, buffer.st_mode) != 0)
          {
             eio_file_thread_error(&order->progress.common, thread);
             return EINA_FALSE;
          }
     }

   return EINA_TRUE;
}

/**
 * @brief Constructs the full target path for a file or directory within the destination.
 *
 * Given a source item's path ('dir'), this function creates the corresponding path
 * in the destination directory. It uses the base destination path from 'order'
 * and the relative path of 'dir' (calculated using 'length_source').
 *
 * @param order The Eio_Dir_Copy context, containing source and destination base paths.
 * @param target The Eina_Strbuf to which the resulting target path will be appended.
 * @param dir The full path of the source item.
 * @param length_source The length of the base source path (order->progress.source).
 * @param length_dest The length of the base destination path (order->progress.dest).
 */
static void
_eio_dir_target(Eio_Dir_Copy *order, Eina_Strbuf *target, const char *dir, int length_source, int length_dest)
{
   int length;

   length = eina_stringshare_strlen(dir);

   eina_strbuf_append_length(target, order->progress.dest, length_dest);
   eina_strbuf_append(target, "/");
   eina_strbuf_append_length(target, dir + length_source, length - length_source + 1);
}

/**
 * @brief Creates all directories listed in order->dirs within the destination.
 *
 * Iterates through the sorted list of source directories, constructs their
 * corresponding target paths, and creates them. Progress is reported after
 * each successful mkdir.
 *
 * @param thread The worker thread.
 * @param order The Eio_Dir_Copy context, containing the list of directories to create.
 * @param step Pointer to the current progress step, incremented here.
 * @param count Total number of items for progress reporting.
 * @param length_source Length of the base source path.
 * @param length_dest Length of the base destination path.
 * @return EINA_TRUE on success, EINA_FALSE on error or if cancelled.
 */
static Eina_Bool
_eio_dir_mkdir(Ecore_Thread *thread, Eio_Dir_Copy *order,
               long long *step, long long count,
               int length_source, int length_dest)
{
   const char *dir;
   Eina_List *l;
   Eina_Strbuf *target = eina_strbuf_new();

   /* create all directory */
   EINA_LIST_FOREACH(order->dirs, l, dir)
     {
        eina_strbuf_reset(target);
        /* build target dir path */
        _eio_dir_target(order, target, dir, length_source, length_dest);

        /* create the directory (we will apply the mode later) */
        if (mkdir(eina_strbuf_string_get(target), 0777) != 0)
          {
             eio_file_thread_error(&order->progress.common, thread);
             eina_strbuf_free(target);
             return EINA_FALSE;
          }

        /* inform main thread */
        (*step)++;
        eio_progress_send(thread, &order->progress, *step, count);

        /* check for cancel request */
        if (ecore_thread_check(thread))
          {
             eina_strbuf_free(target);
             return EINA_FALSE;
          }
     }

   eina_strbuf_free(target);
   return EINA_TRUE;
}

/* no symbolic link on Windows */
#ifndef _WIN32
/**
 * @brief Recreates symbolic links listed in order->links within the destination.
 *
 * Iterates through the list of source symbolic links. For each link, it reads
 * its target. If the link target is within the source directory being copied/moved,
 * the target path is adjusted to be relative to the new destination. Otherwise,
 * the original link target is used. A new symbolic link is then created at the
 * destination.
 * This function is not compiled on Windows.
 *
 * @param thread The worker thread.
 * @param order The Eio_Dir_Copy context, containing the list of links.
 * @param step Pointer to the current progress step, incremented here.
 * @param count Total number of items for progress reporting.
 * @param length_source Length of the base source path.
 * @param length_dest Length of the base destination path.
 * @return EINA_TRUE on success, EINA_FALSE on error or if cancelled.
 */
static Eina_Bool
_eio_dir_link(Ecore_Thread *thread, Eio_Dir_Copy *order,
              long long *step, long long count,
              int length_source, int length_dest)
{
   const char *ln;
   Eina_List *l;
   Eina_Strbuf *oldpath, *buffer;
   char *target = NULL, *newpath = NULL;
   ssize_t bsz = -1;
   struct stat st;

   oldpath = eina_strbuf_new();
   buffer = eina_strbuf_new();

   /* Build once the base of the link target */
   eina_strbuf_append_length(buffer, order->progress.dest, length_dest);
   eina_strbuf_append(buffer, "/");

   /* recreate all links */
   EINA_LIST_FOREACH(order->links, l, ln)
     {
        ssize_t length = -1;

        eina_strbuf_reset(oldpath);

        /* build oldpath link */
        _eio_dir_target(order, oldpath, ln, length_source, length_dest);

        if (lstat(ln, &st) == -1)
          {
             goto on_error;
          }
        if (st.st_size == 0)
          {
             bsz = PATH_MAX;
             free(target);
             target = malloc(bsz);
          }
        else if(bsz < st.st_size + 1)
          {
             bsz = st.st_size +1;
             free(target);
             target = malloc(bsz);
          }

        /* read link target */
        if (target)
          length = readlink(ln, target, bsz);
        if (length < 0)
          goto on_error;

        if (strncmp(target, order->progress.source, length_source) == 0)
          {
             /* The link is inside the zone to copy, so rename it */
             eina_strbuf_insert_length(buffer, target + length_source,length - length_source + 1, length_dest + 1);
             newpath = target;
          }
        else
          {
             /* The link is outside the zone to copy */
             newpath = target;
          }

        /* create the link */
        if (symlink(newpath, eina_strbuf_string_get(oldpath)) != 0)
          goto on_error;

        /* inform main thread */
        (*step)++;
        eio_progress_send(thread, &order->progress, *step, count);

        /* check for cancel request */
        if (ecore_thread_check(thread))
          goto on_thread_error;
     }

   eina_strbuf_free(oldpath);
   eina_strbuf_free(buffer);
   if(target) free(target);

   return EINA_TRUE;

 on_error:
   eio_file_thread_error(&order->progress.common, thread);
 on_thread_error:
   eina_strbuf_free(oldpath);
   eina_strbuf_free(buffer);
   if(target) free(target);
   return EINA_FALSE;
}
#endif

/**
 * @brief Sets permissions on newly created directories and optionally removes source directories.
 *
 * Iterates through the `order->dirs` list in reverse (from deepest to shallowest).
 * For each directory, it retrieves the original mode from the source directory and
 * applies it to the corresponding newly created directory in the destination.
 * If `rmdir_source` is true (typically for a move operation), it attempts to remove
 * the original source directory.
 *
 * @param thread The worker thread.
 * @param order The Eio_Dir_Copy context, containing the list of directories.
 *              The `order->dirs` list is consumed by this function.
 * @param step Pointer to the current progress step, incremented here.
 * @param count Total number of items for progress reporting.
 * @param length_source Length of the base source path.
 * @param length_dest Length of the base destination path.
 * @param rmdir_source If EINA_TRUE, attempts to remove the source directory after
 *                     processing its corresponding destination directory.
 * @return EINA_TRUE on success, EINA_FALSE on error or if cancelled.
 */
static Eina_Bool
_eio_dir_chmod(Ecore_Thread *thread, Eio_Dir_Copy *order,
               long long *step, long long count,
               int length_source, int length_dest,
               Eina_Bool rmdir_source)
{
   const char *dir = NULL;
   Eina_Strbuf *target;
   struct stat buffer;

   target = eina_strbuf_new();

   while (order->dirs)
     {
        eina_strbuf_reset(target);

        /* destroy in reverse order so that we don't prevent change of lower dir */
        dir = eina_list_data_get(eina_list_last(order->dirs));
        order->dirs = eina_list_remove_list(order->dirs, eina_list_last(order->dirs));

        /* build target dir path */
        _eio_dir_target(order, target, dir, length_source, length_dest);

        /* FIXME: in some case we already did a stat call, so would be nice to reuse previous result here */
        /* stat the original dir for mode info */
        if (stat(dir, &buffer) != 0)
          goto on_error;

        /* set the orginal mode to the newly created dir */
        if (chmod(eina_strbuf_string_get(target), buffer.st_mode) != 0)
          goto on_error;

        /* if required destroy original directory */
        if (rmdir_source)
          {
             if (rmdir(dir) != 0)
               goto on_error;
          }

        /* inform main thread */
        (*step)++;
        eio_progress_send(thread, &order->progress, *step, count);

        /* check for cancel request */
        if (ecore_thread_check(thread))
          goto on_cancel;

        eina_stringshare_del(dir);
        dir = NULL;
     }

   eina_strbuf_free(target);
   return EINA_TRUE;

 on_error:
   eio_file_thread_error(&order->progress.common, thread);
 on_cancel:
   if (dir) eina_stringshare_del(dir);
   eina_strbuf_free(target);
   return EINA_FALSE;
}

/**
 * @brief Worker thread function for recursively copying a directory.
 *
 * This function orchestrates the directory copy process:
 * 1. Lists all contents of the source directory (_eio_dir_recursiv_ls).
 * 2. Initializes copy parameters and creates the top-level destination directory (_eio_dir_init).
 * 3. Creates all subdirectories in the destination (_eio_dir_mkdir).
 * 4. Copies all files from source to destination (eio_file_copy_do).
 * 5. Recreates symbolic links in the destination (_eio_dir_link, non-Windows).
 * 6. Sets correct permissions on the newly created directories (_eio_dir_chmod).
 * Reports progress throughout the operation. Cleans up resources on completion or error.
 *
 * @param data Pointer to an Eio_Dir_Copy structure containing operation details.
 * @param thread The Ecore_Thread executing this function.
 */
static void
_eio_dir_copy_heavy(void *data, Ecore_Thread *thread)
{
   Eio_Dir_Copy *copy = data;
   const char *file = NULL;
   const char *dir;
   const char *ln;

   Eio_File_Progress file_copy;
   Eina_Strbuf *target;

   int length_source = 0;
   int length_dest = 0;
   long long count;
   long long step;

   /* list all the content that should be copied */
   if (!_eio_dir_recursiv_ls(thread, copy, copy->progress.source))
     return;

   target = eina_strbuf_new();

   /* init all structure needed to copy the file */
   if (!_eio_dir_init(thread, &step, &count, &length_source, &length_dest, copy, &file_copy))
     goto on_error;

   /* suboperation is a file copy */
   file_copy.op = EIO_FILE_COPY;

   /* create all directory */
   if (!_eio_dir_mkdir(thread, copy, &step, count, length_source, length_dest))
     goto on_error;

   /* copy all files */
   EINA_LIST_FREE(copy->files, file)
     {
        eina_strbuf_reset(target);

        /* build target file path */
        _eio_dir_target(copy, target, file, length_source, length_dest);

        file_copy.source = file;
        file_copy.dest = eina_stringshare_add(eina_strbuf_string_get(target));

        /* copy the file */
        if (!eio_file_copy_do(thread, &file_copy))
          {
             copy->progress.common.error = file_copy.common.error;
             goto on_error;
          }

        /* notify main thread */
        step++;
        eio_progress_send(thread, &copy->progress, step, count);

        if (ecore_thread_check(thread))
          goto on_error;

        eina_stringshare_del(file_copy.dest);
        eina_stringshare_del(file);
     }
   file_copy.dest = NULL;
   file = NULL;

   /* recreate link */
   /* no symbolic link on Windows */
#ifndef _WIN32
   if (!_eio_dir_link(thread, copy, &step, count, length_source, length_dest))
     goto on_error;
#endif

   /* set directory right back */
   if (!_eio_dir_chmod(thread, copy, &step, count, length_source, length_dest, EINA_FALSE))
     goto on_error;

 on_error:
   /* cleanup the mess */
   if (file_copy.dest) eina_stringshare_del(file_copy.dest);
   if (file) eina_stringshare_del(file);

   EINA_LIST_FREE(copy->files, file)
     eina_stringshare_del(file);
   EINA_LIST_FREE(copy->dirs, dir)
     eina_stringshare_del(dir);
   EINA_LIST_FREE(copy->links, ln)
     eina_stringshare_del(ln);

   if (!ecore_thread_check(thread))
     eio_progress_send(thread, &copy->progress, count, count);

   eina_strbuf_free(target);

   return;
}

/**
 * @brief Notification callback for directory copy/move operations.
 *
 * This function is called from the worker thread (via ecore_thread_feedback)
 * to pass progress information to the main thread. It then invokes the
 * user-provided progress callback.
 *
 * @param data Pointer to the Eio_Dir_Copy structure.
 * @param thread The Ecore_Thread (unused in this function).
 * @param msg_data Pointer to an Eio_Progress structure containing current progress.
 */
static void
_eio_dir_copy_notify(void *data, Ecore_Thread *thread EINA_UNUSED, void *msg_data)
{
   Eio_Dir_Copy *copy = data;
   Eio_Progress *progress = msg_data;

   eio_progress_cb(progress, &copy->progress);
}

/**
 * @brief Frees resources associated with an Eio_Dir_Copy structure.
 *
 * Releases stringshared source and destination paths and common Eio_File resources.
 * Does not free the Eio_Dir_Copy structure itself, as it's typically part of a larger allocation.
 *
 * @param copy The Eio_Dir_Copy structure to clean up.
 */
static void
_eio_dir_copy_free(Eio_Dir_Copy *copy)
{
   eina_stringshare_del(copy->progress.source);
   eina_stringshare_del(copy->progress.dest);
   eio_file_free(&copy->progress.common);
}

/**
 * @brief Callback executed in the main thread when a directory copy/move operation completes successfully.
 *
 * Invokes the user-provided done_cb and then frees the Eio_Dir_Copy resources.
 *
 * @param data Pointer to the Eio_Dir_Copy structure.
 * @param thread The Ecore_Thread (unused).
 */
static void
_eio_dir_copy_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Dir_Copy *copy = data;

   copy->progress.common.done_cb((void*) copy->progress.common.data, &copy->progress.common);

   _eio_dir_copy_free(copy);
}

/**
 * @brief Callback executed in the main thread when a directory copy/move operation fails or is cancelled.
 *
 * Invokes the user-provided error_cb (via eio_file_error) and then frees the Eio_Dir_Copy resources.
 *
 * @param data Pointer to the Eio_Dir_Copy structure.
 * @param thread The Ecore_Thread (unused).
 */
static void
_eio_dir_copy_error(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Dir_Copy *copy = data;

   eio_file_error(&copy->progress.common);

   _eio_dir_copy_free(copy);
}

/**
 * @brief Worker thread function for recursively moving a directory.
 *
 * This function orchestrates the directory move process:
 * 1. Attempts a simple `rename()`. If successful, the operation is done.
 * 2. If `rename()` fails (e.g., across different filesystems), it proceeds with a copy-then-delete strategy:
 *    a. Lists all contents of the source directory (_eio_dir_recursiv_ls).
 *    b. Initializes parameters and creates the top-level destination directory (_eio_dir_init).
 *    c. Creates all subdirectories in the destination (_eio_dir_mkdir).
 *    d. Moves files: tries `rename()` first, falls back to copy-then-unlink if `rename()` fails with EXDEV.
 *    e. Recreates symbolic links in the destination (_eio_dir_link, non-Windows).
 *    f. Sets correct permissions on newly created directories and removes original source directories (_eio_dir_chmod with rmdir_source=EINA_TRUE).
 *    g. Removes the original top-level source directory.
 * Reports progress throughout the operation. Cleans up resources on completion or error.
 *
 * @param data Pointer to an Eio_Dir_Copy structure containing operation details (used as Eio_Dir_Move).
 * @param thread The Ecore_Thread executing this function.
 */
static void
_eio_dir_move_heavy(void *data, Ecore_Thread *thread)
{
   Eio_Dir_Copy *move = data;
   const char *file = NULL;
   const char *dir = NULL;

   Eio_File_Progress file_move;
   Eina_Strbuf *target;

   int length_source;
   int length_dest;
   long long count;
   long long step;

   /* just try a rename, maybe we are lucky... */
   if (rename(move->progress.source, move->progress.dest) == 0)
     {
        /* we are really lucky */
        eio_progress_send(thread, &move->progress, 1, 1);
        return;
     }

   /* list all the content that should be moved */
   if (!_eio_dir_recursiv_ls(thread, move, move->progress.source))
     return;

   target = eina_strbuf_new();

   /* init all structure needed to move the file */
   if (!_eio_dir_init(thread, &step, &count, &length_source, &length_dest, move, &file_move))
     goto on_error;

   /* sub operation is a file move */
   file_move.op = EIO_FILE_MOVE;

   /* create all directory */
   if (!_eio_dir_mkdir(thread, move, &step, count, length_source, length_dest))
     goto on_error;

   /* move file around */
   EINA_LIST_FREE(move->files, file)
     {
        eina_strbuf_reset(target);

        /* build target file path */
        _eio_dir_target(move, target, file, length_source, length_dest);

        file_move.source = file;
        file_move.dest = eina_stringshare_add(eina_strbuf_string_get(target));

        /* first try to rename */
        if (rename(file_move.source, file_move.dest) < 0)
          {
             if (errno != EXDEV)
               {
                  eio_file_thread_error(&move->progress.common, thread);
                  goto on_error;
               }

             /* then try real copy */
             if (!eio_file_copy_do(thread, &file_move))
               {
                  move->progress.common.error = file_move.common.error;
                  goto on_error;
               }

             /* and unlink the original */
             if (unlink(file) != 0)
               {
                  eio_file_thread_error(&move->progress.common, thread);
                  goto on_error;
               }
          }

        step++;
        eio_progress_send(thread, &move->progress, step, count);

        if (ecore_thread_check(thread))
          goto on_error;

        eina_stringshare_del(file_move.dest);
        eina_stringshare_del(file);
     }
   file_move.dest = NULL;
   file = NULL;

   /* recreate link */
/* no symbolic link on Windows */
#ifndef _WIN32
   if (!_eio_dir_link(thread, move, &step, count, length_source, length_dest))
     goto on_error;
#endif

   /* set directory right back */
   if (!_eio_dir_chmod(thread, move, &step, count, length_source, length_dest, EINA_TRUE))
     goto on_error;

   if (rmdir(move->progress.source) != 0)
     goto on_error;

 on_error:
   /* cleanup the mess */
   if (file_move.dest) eina_stringshare_del(file_move.dest);
   if (file) eina_stringshare_del(file);

   EINA_LIST_FREE(move->files, file)
     eina_stringshare_del(file);
   EINA_LIST_FREE(move->dirs, dir)
     eina_stringshare_del(dir);

   if (!ecore_thread_check(thread))
     eio_progress_send(thread, &move->progress, count, count);

   eina_strbuf_free(target);

   return;
}

/**
 * @brief Worker thread function for recursively deleting a directory (rm -rf).
 *
 * This function orchestrates the recursive deletion:
 * 1. Lists all contents of the source directory (_eio_dir_recursiv_ls).
 * 2. Unlinks all files found.
 * 3. Reverses the list of directories (to delete deepest first).
 * 4. Removes all directories.
 * 5. Removes the top-level source directory.
 * Reports progress for each unlinked file and removed directory.
 *
 * @param data Pointer to an Eio_Dir_Copy structure (used as Eio_Dir_RmRf).
 *             `rmrf->progress.source` is the directory to delete.
 *             `rmrf->progress.dest` is used to report the path of the item being deleted.
 * @param thread The Ecore_Thread executing this function.
 */
static void
_eio_dir_rmrf_heavy(void *data, Ecore_Thread *thread)
{
   Eio_Dir_Copy *rmrf = data;
   const char *file = NULL;
   const char *dir = NULL;

   long long count;
   long long step;

   /* list all the content that should be moved */
   if (!_eio_dir_recursiv_ls(thread, rmrf, rmrf->progress.source))
     return;

   /* init counter */
   step = 0;
   count = ((long long) eina_list_count(rmrf->files)) + ((long long) eina_list_count(rmrf->dirs)) + 1;

   EINA_LIST_FREE(rmrf->files, file)
     {
        if (unlink(file) != 0)
          {
             eio_file_thread_error(&rmrf->progress.common, thread);
             goto on_error;
          }

        eina_stringshare_replace(&rmrf->progress.dest, file);

        step++;
        eio_progress_send(thread, &rmrf->progress, step, count);

        if (ecore_thread_check(thread))
          goto on_error;

        eina_stringshare_del(file);
     }
   file = NULL;

   /* reverse directory listing, so the leaf would be destroyed before
      the root */
   rmrf->dirs = eina_list_reverse(rmrf->dirs);

   EINA_LIST_FREE(rmrf->dirs, dir)
     {
        if (rmdir(dir) != 0)
          {
             eio_file_thread_error(&rmrf->progress.common, thread);
             goto on_error;
          }

        eina_stringshare_replace(&rmrf->progress.dest, dir);

        step++;
        eio_progress_send(thread, &rmrf->progress, step, count);

        if (ecore_thread_check(thread))
          goto on_error;

        eina_stringshare_del(dir);
     }
   dir = NULL;

   if (rmdir(rmrf->progress.source) != 0)
     goto on_error;
   step++;

 on_error:
   if (dir) eina_stringshare_del(dir);
   if (file) eina_stringshare_del(file);

   EINA_LIST_FREE(rmrf->dirs, dir)
     eina_stringshare_del(dir);
   EINA_LIST_FREE(rmrf->files, file)
     eina_stringshare_del(file);

   if (!ecore_thread_check(thread))
     eio_progress_send(thread, &rmrf->progress, count, count);

   return;
}

/**
 * @brief Callback used by _eio_file_recursiv_ls during stat/direct ls operations.
 *
 * This function is called for each file/directory found by the recursive listing.
 * It applies the user-provided filter (if any). If the item passes the filter,
 * it's packaged into an Eio_File_Direct_Info structure and added to a list (`async->pack`).
 * This list is periodically sent to the main thread via `ecore_thread_feedback`.
 *
 * @param async The Eio_File_Dir_Ls context for the ls operation.
 * @param handler The Eio_File handler (unused, part of Eio_Filter_Direct_Cb signature).
 * @param info Information about the currently processed file/directory.
 * @return EINA_TRUE if the item passed the filter (or no filter), EINA_FALSE otherwise or on error.
 *         The return value also influences whether _eio_file_recursiv_ls recurses into directories.
 */
static Eina_Bool
_eio_dir_stat_find_forward(Eio_File_Dir_Ls *async,
                           Eio_File *handler,
                           Eina_File_Direct_Info *info)
{
   Eina_Bool filter = EINA_TRUE;

   if (async->filter_cb)
     {
        filter = async->filter_cb((void*) async->ls.common.data, &async->ls.common, info);
     }

   if (filter)
     {
        Eio_File_Direct_Info *send_di;

        send_di = eio_direct_info_malloc();
        if (!send_di) return EINA_FALSE;

        memcpy(&send_di->info, info, sizeof (Eina_File_Direct_Info));
	send_di->associated = async->ls.common.worker.associated;
	async->ls.common.worker.associated = NULL;

        async->pack = eina_list_append(async->pack, send_di);
     }
   else if (async->ls.common.worker.associated)
     {
        eina_hash_free(async->ls.common.worker.associated);
        async->ls.common.worker.associated = NULL;
     }

   async->pack = eio_pack_send(handler->thread, async->pack, &async->start);

   return filter;
}

/**
 * @brief Worker thread function for `eio_dir_stat_ls`.
 *
 * Performs a recursive directory listing using `_eio_file_recursiv_ls` with
 * `eina_file_stat_ls` (which performs `lstat` on each item) and
 * `_eio_dir_stat_find_forward` as the processing callback.
 * Results are batched and sent to the main thread.
 *
 * @param data Pointer to an Eio_File_Dir_Ls structure.
 * @param thread The Ecore_Thread executing this function.
 */
static void
_eio_dir_stat_find_heavy(void *data, Ecore_Thread *thread)
{
   Eio_File_Dir_Ls *async = data;

   async->ls.common.thread = thread;
   async->pack = NULL;
   async->start = ecore_time_get();

   _eio_file_recursiv_ls(thread, &async->ls.common,
                         (Eio_Filter_Direct_Cb) _eio_dir_stat_find_forward,
			 eina_file_stat_ls,
                         async, async->ls.directory);

   if (async->pack) ecore_thread_feedback(thread, async->pack);
   async->pack = NULL;
}

/**
 * @brief Worker thread function for `eio_dir_direct_ls`.
 *
 * Performs a recursive directory listing using `_eio_file_recursiv_ls` with
 * `eina_file_direct_ls` (which uses `readdir` and `d_type` without `lstat`) and
 * `_eio_dir_stat_find_forward` as the processing callback.
 * Results are batched and sent to the main thread. Note that file types might be
 * less accurate (e.g., EINA_FILE_LNK might appear as EINA_FILE_UNKNOWN if d_type is not DT_LNK).
 *
 * @param data Pointer to an Eio_File_Dir_Ls structure.
 * @param thread The Ecore_Thread executing this function.
 */
static void
_eio_dir_direct_find_heavy(void *data, Ecore_Thread *thread)
{
   Eio_File_Dir_Ls *async = data;

   async->ls.common.thread = thread;
   async->pack = NULL;
   async->start = ecore_time_get();

   _eio_file_recursiv_ls(thread, &async->ls.common,
                         (Eio_Filter_Direct_Cb) _eio_dir_stat_find_forward,
			 eina_file_direct_ls,
                         async, async->ls.directory);

   if (async->pack) ecore_thread_feedback(thread, async->pack);
   async->pack = NULL;
}

/**
 * @brief Common 'done' callback for directory listing operations (stat_ls, direct_ls).
 *
 * Executed in the main thread when the listing operation completes successfully.
 * Invokes the user-provided `done_cb` and frees the Eio_File_Ls (async) structure.
 *
 * @param data Pointer to an Eio_File_Ls structure.
 * @param thread The Ecore_Thread (unused).
 */
static void
_eio_dir_stat_done(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Ls *async = data;

   async->common.done_cb((void*) async->common.data, &async->common);

   eio_async_free(async);
}

/**
 * @brief Common 'error' callback for directory listing operations (stat_ls, direct_ls).
 *
 * Executed in the main thread when the listing operation fails or is cancelled.
 * Invokes the user-provided `error_cb` (via eio_file_error) and frees the
 * Eio_File_Ls (async) structure.
 *
 * @param data Pointer to an Eio_File_Ls structure.
 * @param thread The Ecore_Thread (unused).
 */
static void
_eio_dir_stat_error(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Ls *async = data;

   eio_file_error(&async->common);

   eio_async_free(async);
}

/**
 * @endcond
 */

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/


EIO_API Eio_File *
eio_dir_copy(const char *source,
             const char *dest,
             Eio_Filter_Direct_Cb filter_cb,
             Eio_Progress_Cb progress_cb,
             Eio_Done_Cb done_cb,
             Eio_Error_Cb error_cb,
             const void *data)
{
   Eio_Dir_Copy *copy;

   EINA_SAFETY_ON_NULL_RETURN_VAL(source, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(dest, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   copy = eio_common_alloc(sizeof(Eio_Dir_Copy));
   EINA_SAFETY_ON_NULL_RETURN_VAL(copy, NULL);

   /** @ Eio_Dir_Copy::progress::op Operation type. */
   copy->progress.op = EIO_DIR_COPY;
   /** @ Eio_Dir_Copy::progress::progress_cb User callback for progress updates. */
   copy->progress.progress_cb = progress_cb;
   /** @ Eio_Dir_Copy::progress::source Source directory path. */
   copy->progress.source = eina_stringshare_add(source);
   /** @ Eio_Dir_Copy::progress::dest Destination directory path. */
   copy->progress.dest = eina_stringshare_add(dest);
   /** @ Eio_Dir_Copy::filter_cb Optional filter for items during listing. */
   copy->filter_cb = filter_cb;
   /** @ Eio_Dir_Copy::files List of files to be copied. Populated by _eio_dir_recursiv_ls. */
   copy->files = NULL;
   /** @ Eio_Dir_Copy::dirs List of directories to be copied/created. Populated by _eio_dir_recursiv_ls. */
   copy->dirs = NULL;
   /** @ Eio_Dir_Copy::links List of symbolic links to be recreated. Populated by _eio_dir_recursiv_ls. */
   copy->links = NULL;

   if (!eio_long_file_set(&copy->progress.common,
                          done_cb,
                          error_cb,
                          data,
                          _eio_dir_copy_heavy,
                          _eio_dir_copy_notify,
                          _eio_dir_copy_end,
                          _eio_dir_copy_error))
     return NULL;

   return &copy->progress.common;
}

EIO_API Eio_File *
eio_dir_move(const char *source,
             const char *dest,
             Eio_Filter_Direct_Cb filter_cb,
             Eio_Progress_Cb progress_cb,
             Eio_Done_Cb done_cb,
             Eio_Error_Cb error_cb,
             const void *data)
{
   Eio_Dir_Copy *move;

   EINA_SAFETY_ON_NULL_RETURN_VAL(source, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(dest, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   move = eio_common_alloc(sizeof(Eio_Dir_Copy));
   EINA_SAFETY_ON_NULL_RETURN_VAL(move, NULL);

   /** @ Eio_Dir_Copy::progress::op Operation type. */
   move->progress.op = EIO_DIR_MOVE;
   /** @ Eio_Dir_Copy::progress::progress_cb User callback for progress updates. */
   move->progress.progress_cb = progress_cb;
   /** @ Eio_Dir_Copy::progress::source Source directory path. */
   move->progress.source = eina_stringshare_add(source);
   /** @ Eio_Dir_Copy::progress::dest Destination directory path. */
   move->progress.dest = eina_stringshare_add(dest);
   /** @ Eio_Dir_Copy::filter_cb Optional filter for items during listing. */
   move->filter_cb = filter_cb;
   /** @ Eio_Dir_Copy::files List of files to be moved. Populated by _eio_dir_recursiv_ls. */
   move->files = NULL;
   /** @ Eio_Dir_Copy::dirs List of directories to be moved/created. Populated by _eio_dir_recursiv_ls. */
   move->dirs = NULL;
   /** @ Eio_Dir_Copy::links List of symbolic links to be recreated. Populated by _eio_dir_recursiv_ls. */
   move->links = NULL;

   if (!eio_long_file_set(&move->progress.common,
                          done_cb,
                          error_cb,
                          data,
                          _eio_dir_move_heavy,
                          _eio_dir_copy_notify,
                          _eio_dir_copy_end,
                          _eio_dir_copy_error))
     return NULL;

   return &move->progress.common;
}

EIO_API Eio_File *
eio_dir_unlink(const char *path,
               Eio_Filter_Direct_Cb filter_cb,
               Eio_Progress_Cb progress_cb,
               Eio_Done_Cb done_cb,
               Eio_Error_Cb error_cb,
               const void *data)
{
   Eio_Dir_Copy *rmrf;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   rmrf = eio_common_alloc(sizeof(Eio_Dir_Copy));
   EINA_SAFETY_ON_NULL_RETURN_VAL(rmrf, NULL);

   /** @ Eio_Dir_Copy::progress::op Operation type. */
   rmrf->progress.op = EIO_UNLINK;
   /** @ Eio_Dir_Copy::progress::progress_cb User callback for progress updates. */
   rmrf->progress.progress_cb = progress_cb;
   /** @ Eio_Dir_Copy::progress::source Path of the directory to remove. */
   rmrf->progress.source = eina_stringshare_add(path);
   /** @ Eio_Dir_Copy::progress::dest Used to report the path of the item being deleted during progress. */
   rmrf->progress.dest = NULL;
   /** @ Eio_Dir_Copy::filter_cb Optional filter for items during listing (before deletion). */
   rmrf->filter_cb = filter_cb;
   /** @ Eio_Dir_Copy::files List of files to be unlinked. Populated by _eio_dir_recursiv_ls. */
   rmrf->files = NULL;
   /** @ Eio_Dir_Copy::dirs List of directories to be removed. Populated by _eio_dir_recursiv_ls. */
   rmrf->dirs = NULL;
   /** @ Eio_Dir_Copy::links List of symbolic links (typically not specially handled by rmrf, but populated). */
   rmrf->links = NULL;

   if (!eio_long_file_set(&rmrf->progress.common,
                          done_cb,
                          error_cb,
                          data,
                          _eio_dir_rmrf_heavy,
                          _eio_dir_copy_notify,
                          _eio_dir_copy_end,
                          _eio_dir_copy_error))
     return NULL;

   return &rmrf->progress.common;
}

/**
 * @internal
 * @brief Internal implementation for eio_dir_stat_ls and _eio_dir_stat_ls.
 *
 * Sets up an Eio_File_Dir_Ls structure and launches a worker thread
 * (_eio_dir_stat_find_heavy) to perform a recursive directory listing
 * using `lstat` for file information.
 *
 * @param dir The directory to list.
 * @param filter_cb Optional callback to filter results.
 * @param main_cb Callback for individual results (if not gathering).
 * @param main_internal_cb Callback for an array of results (if gathering).
 * @param done_cb Callback when operation is successfully done.
 * @param error_cb Callback when an error occurs.
 * @param data User data for callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 */
static Eio_File *
_eio_dir_stat_internal_ls(const char *dir,
                          Eio_Filter_Direct_Cb filter_cb,
                          Eio_Main_Direct_Cb main_cb,
                          Eio_Array_Cb main_internal_cb,
                          Eio_Done_Cb done_cb,
                          Eio_Error_Cb error_cb,
                          const void *data)
{
   Eio_File_Dir_Ls *async;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dir, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = eio_common_alloc(sizeof(Eio_File_Dir_Ls));
   EINA_SAFETY_ON_NULL_RETURN_VAL(async, NULL);

   /** @ Eio_File_Dir_Ls::ls::directory The directory being listed. */
   /* Eio_Filter_Direct_Cb must be casted to Eio_Filter_Dir_Cb here
    * because we keep the Eio_File_Dir_Ls pointing to that variant
    * where info can be modified, but in our case it's already doing
    * stat() then it shouldn't be needed!
    */
   async->ls.directory = eina_stringshare_add(dir);
   /** @ Eio_File_Dir_Ls::filter_cb User-provided filter callback. */
   async->filter_cb = (Eio_Filter_Dir_Cb)filter_cb;
   if (main_internal_cb)
     {
        /** @ Eio_File_Dir_Ls::main_internal_cb Callback for batched results. */
        async->main_internal_cb = main_internal_cb;
        /** @ Eio_File_Ls::gather If true, results are batched. */
        async->ls.gather = EINA_TRUE;
     }
   else
     {
        /** @ Eio_File_Dir_Ls::main_cb Callback for individual results. */
        async->main_cb = main_cb;
     }

   if (!eio_long_file_set(&async->ls.common,
                          done_cb,
                          error_cb,
                          data,
                          _eio_dir_stat_find_heavy,
                          _eio_direct_notify,
                          _eio_dir_stat_done,
                          _eio_dir_stat_error))
     return NULL;

   return &async->ls.common;
}

EIO_API Eio_File *
eio_dir_stat_ls(const char *dir,
                Eio_Filter_Direct_Cb filter_cb,
                Eio_Main_Direct_Cb main_cb,
                Eio_Done_Cb done_cb,
                Eio_Error_Cb error_cb,
                const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_cb, NULL);

   return _eio_dir_stat_internal_ls(dir, filter_cb, main_cb, NULL, done_cb, error_cb, data);
}

Eio_File *
_eio_dir_stat_ls(const char *dir,
                 Eio_Array_Cb main_internal_cb,
                 Eio_Done_Cb done_cb,
                 Eio_Error_Cb error_cb,
                 const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_internal_cb, NULL);

   return _eio_dir_stat_internal_ls(dir, NULL, NULL, main_internal_cb, done_cb, error_cb, data);
}

/**
 * @internal
 * @brief Internal implementation for eio_dir_direct_ls and _eio_dir_direct_ls.
 *
 * Sets up an Eio_File_Dir_Ls structure and launches a worker thread
 * (_eio_dir_direct_find_heavy) to perform a recursive directory listing
 * using `readdir` (via `eina_file_direct_ls`) for file information, which
 * avoids `lstat` calls for performance but may be less accurate for file types.
 *
 * @param dir The directory to list.
 * @param filter_cb Optional callback to filter results. Allows modification of info.
 * @param main_cb Callback for individual results (if not gathering).
 * @param main_internal_cb Callback for an array of results (if gathering).
 * @param done_cb Callback when operation is successfully done.
 * @param error_cb Callback when an error occurs.
 * @param data User data for callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 */
static Eio_File *
_eio_dir_direct_internal_ls(const char *dir,
                            Eio_Filter_Dir_Cb filter_cb,
                            Eio_Main_Direct_Cb main_cb,
                            Eio_Array_Cb main_internal_cb,
                            Eio_Done_Cb done_cb,
                            Eio_Error_Cb error_cb,
                            const void *data)
{
   Eio_File_Dir_Ls *async;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dir, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = eio_common_alloc(sizeof(Eio_File_Dir_Ls));
   EINA_SAFETY_ON_NULL_RETURN_VAL(async, NULL);

   /** @ Eio_File_Dir_Ls::ls::directory The directory being listed. */
   async->ls.directory = eina_stringshare_add(dir);
   /** @ Eio_File_Dir_Ls::filter_cb User-provided filter callback. */
   async->filter_cb = filter_cb;
   if (main_internal_cb)
     {
        /** @ Eio_File_Dir_Ls::main_internal_cb Callback for batched results. */
        async->main_internal_cb = main_internal_cb;
        /** @ Eio_File_Ls::gather If true, results are batched. */
        async->ls.gather = EINA_TRUE;
     }
   else
     {
        /** @ Eio_File_Dir_Ls::main_cb Callback for individual results. */
        async->main_cb = main_cb;
     }

   if (!eio_long_file_set(&async->ls.common,
                          done_cb,
                          error_cb,
                          data,
                          _eio_dir_direct_find_heavy,
                          _eio_direct_notify,
                          _eio_dir_stat_done,
                          _eio_dir_stat_error))
     return NULL;

   return &async->ls.common;
}

EIO_API Eio_File *
eio_dir_direct_ls(const char *dir,
		  Eio_Filter_Dir_Cb filter_cb,
		  Eio_Main_Direct_Cb main_cb,
		  Eio_Done_Cb done_cb,
		  Eio_Error_Cb error_cb,
		  const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_cb, NULL);

   return _eio_dir_direct_internal_ls(dir, filter_cb, main_cb, NULL, done_cb, error_cb, data);
}

Eio_File *
_eio_dir_direct_ls(const char *dir,
                   Eio_Array_Cb main_internal_cb,
                   Eio_Done_Cb done_cb,
                   Eio_Error_Cb error_cb,
                   const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_internal_cb, NULL);

   return _eio_dir_direct_internal_ls(dir, NULL, NULL, main_internal_cb, done_cb, error_cb, data);
}
