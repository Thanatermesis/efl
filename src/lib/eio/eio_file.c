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

#include "eio_private.h"
#include "Eio.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @brief Worker thread function for listing files in a directory (string filenames).
 *
 * This function is executed in a separate thread to perform the potentially
 * blocking operation of listing directory contents. It iterates through
 * files, applies an optional filter, and sends batches of filenames
 * back to the main thread via ecore_thread_feedback().
 * Any associated data set via eio_file_associate_add() before starting
 * the operation is attached to the Eio_File_Char sent for each file.
 *
 * @param data Pointer to an Eio_File_Char_Ls structure containing operation parameters
 *             and callbacks. This structure holds the directory to list, filter callback, etc.
 * @param thread The Ecore_Thread in which this function is executing. Used for
 *               sending feedback and checking for cancellation.
 */
static void
_eio_file_heavy(void *data, Ecore_Thread *thread)
{
   Eio_File_Char_Ls *async = data;
   Eina_Iterator *ls;
   const char *file;
   Eina_List *pack = NULL;
   double start;

   ls = eina_file_ls(async->ls.directory);
   if (!ls)
     {
        eio_file_thread_error(&async->ls.common, thread);
        return;
     }

   eio_file_container_set(&async->ls.common, eina_iterator_container_get(ls));

   start = ecore_time_get();

   EINA_ITERATOR_FOREACH(ls, file)
     {
        Eina_Bool filter = EINA_TRUE;

        if (async->filter_cb)
	         {
             filter = async->filter_cb((void*) async->ls.common.data, &async->ls.common, file);
	         }

        if (filter)
          {
             Eio_File_Char *send_fc;

             send_fc = eio_char_malloc();
             if (!send_fc) goto on_error;

             send_fc->filename = file;
	            send_fc->associated = async->ls.common.worker.associated;
             async->ls.common.worker.associated = NULL;

	            pack = eina_list_append(pack, send_fc);
          }
        else
          {
on_error:
             eina_stringshare_del(file);

             if (async->ls.common.worker.associated)
               {
                  eina_hash_free(async->ls.common.worker.associated);
                  async->ls.common.worker.associated = NULL;
               }
          }

        pack = eio_pack_send(thread, pack, &start);

        if (ecore_thread_check(thread))
          break;
     }

   if (pack) ecore_thread_feedback(thread, pack);

   async->ls.ls = ls;
}

/**
 * @brief Notification callback for file listing (string filenames).
 *
 * This function is called in the main thread when the worker thread
 * (_eio_file_heavy) sends a batch of filenames. It processes these
 * filenames, either by calling the main_cb for each file or by
 * gathering them into an Eina_Array if main_internal_cb is set.
 * Associated data with each file is made available via async->ls.common.main.associated
 * before calling the main_cb.
 *
 * @param data Pointer to an Eio_File_Char_Ls structure.
 * @param thread The Ecore_Thread that sent the notification (unused in this function).
 * @param msg_data An Eina_List of Eio_File_Char pointers. Each Eio_File_Char
 *                 contains a filename (const char*) and potentially associated data (Eina_Hash*).
 *                 Example of Eina_List structure:
 *                 list -> Eio_File_Char{ filename="file1.txt", associated=hash1 }
 *                      -> Eio_File_Char{ filename="image.jpg", associated=hash2 }
 *                      -> NULL
 *                 This function takes ownership of the list and its contents, freeing
 *                 the Eio_File_Char structures and their stringshared filenames.
 *                 The associated hash is also freed if present.
 */
void
_eio_string_notify(void *data, Ecore_Thread *thread EINA_UNUSED, void *msg_data)
{
   Eio_File_Char_Ls *async = data;
   Eina_List *pack = msg_data;
   Eio_File_Char *info;

   async->ls.common.length += eina_list_count(pack);

   // Check if it is an internal use
   if (async->ls.gather)
     {
        Eina_Array *gather;

        gather = eina_array_new(eina_list_count(pack));
        EINA_LIST_FREE(pack, info)
          {
             if (!gather)
               eina_stringshare_del(info->filename);
             else
               eina_array_push(gather, info->filename);
             eio_char_free(info);
          }

        // transfer ownership to caller
        async->main_internal_cb((void*) async->ls.common.data,
                                &async->ls.common,
                                gather);

        return ;
     }

   EINA_LIST_FREE(pack, info)
     {
        async->ls.common.main.associated = info->associated;

        async->main_cb((void*) async->ls.common.data,
                       &async->ls.common,
                       info->filename);

        if (async->ls.common.main.associated)
          {
             eina_hash_free(async->ls.common.main.associated);
             async->ls.common.main.associated = NULL;
          }

        eina_stringshare_del(info->filename);
        eio_char_free(info);
     }
}

/**
 * @brief Core worker logic for listing files with Eina_File_Direct_Info.
 *
 * This function iterates over an Eina_Iterator yielding Eina_File_Direct_Info structures.
 * It applies an optional filter, packages the info into Eio_File_Direct_Info structures,
 * and sends them in batches to the main thread. This is a helper function used by
 * _eio_file_direct_heavy and _eio_file_stat_heavy.
 *
 * @param thread The Ecore_Thread in which this function is executing.
 * @param async Pointer to an Eio_File_Direct_Ls structure containing operation parameters.
 * @param ls An Eina_Iterator providing Eina_File_Direct_Info structures for directory entries.
 *           This iterator is consumed by this function.
 */
static void
_eio_file_eina_ls_heavy(Ecore_Thread *thread, Eio_File_Direct_Ls *async, Eina_Iterator *ls)
{
   const Eina_File_Direct_Info *info;
   Eina_List *pack = NULL;
   double start;

   if (!ls)
     {
        eio_file_thread_error(&async->ls.common, thread);
        return;
     }

   eio_file_container_set(&async->ls.common, eina_iterator_container_get(ls));

   start = ecore_time_get();

   EINA_ITERATOR_FOREACH(ls, info)
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
	            if (!send_di) continue;

             memcpy(&send_di->info, info, sizeof (Eina_File_Direct_Info));
             send_di->associated = async->ls.common.worker.associated;
	            async->ls.common.worker.associated = NULL;

             pack = eina_list_append(pack, send_di);
	         }
        else if (async->ls.common.worker.associated)
          {
             eina_hash_free(async->ls.common.worker.associated);
             async->ls.common.worker.associated = NULL;
          }

        pack = eio_pack_send(thread, pack, &start);

        if (ecore_thread_check(thread))
          break;
     }

   if (pack) ecore_thread_feedback(thread, pack);

   async->ls.ls = ls;
}

/**
 * @brief Worker thread function for eio_file_direct_ls.
 *
 * This function obtains an iterator for directory listing with direct file info
 * (Eina_File_Direct_Info) using eina_file_direct_ls() and then processes it
 * using _eio_file_eina_ls_heavy().
 *
 * @param data Pointer to an Eio_File_Direct_Ls structure.
 * @param thread The Ecore_Thread in which this function is executing.
 */
static void
_eio_file_direct_heavy(void *data, Ecore_Thread *thread)
{
   Eio_File_Direct_Ls *async = data;
   Eina_Iterator *ls;

   ls = eina_file_direct_ls(async->ls.directory);

   _eio_file_eina_ls_heavy(thread, async, ls);
}

/**
 * @brief Worker thread function for eio_file_stat_ls.
 *
 * This function obtains an iterator for directory listing with stat information
 * (Eina_File_Direct_Info, but populated via stat) using eina_file_stat_ls()
 * and then processes it using _eio_file_eina_ls_heavy().
 *
 * @param data Pointer to an Eio_File_Direct_Ls structure.
 * @param thread The Ecore_Thread in which this function is executing.
 */
static void
_eio_file_stat_heavy(void *data, Ecore_Thread *thread)
{
   Eio_File_Direct_Ls *async = data;
   Eina_Iterator *ls;

   ls = eina_file_stat_ls(async->ls.directory);

   _eio_file_eina_ls_heavy(thread, async, ls);
}

/**
 * @brief Notification callback for direct/stat file listing.
 *
 * This function is called in the main thread when the worker thread
 * (_eio_file_direct_heavy or _eio_file_stat_heavy) sends a batch of
 * Eio_File_Direct_Info structures. It processes these, either by calling
 * the main_cb for each item or by gathering them into an Eina_Array
 * if main_internal_cb is set.
 * Associated data with each file is made available via async->ls.common.main.associated
 * before calling the main_cb.
 *
 * @param data Pointer to an Eio_File_Direct_Ls structure.
 * @param thread The Ecore_Thread that sent the notification (unused).
 * @param msg_data An Eina_List of Eio_File_Direct_Info pointers. Each contains
 *                 an Eina_File_Direct_Info structure and potentially associated data.
 *                 Example of Eina_List structure:
 *                 list -> Eio_File_Direct_Info{ info={...}, associated=hash1 }
 *                      -> Eio_File_Direct_Info{ info={...}, associated=hash2 }
 *                      -> NULL
 *                 This function takes ownership of the list and its Eio_File_Direct_Info
 *                 elements, freeing them. The associated hash is also freed if present.
 *                 If not gathering, the Eina_File_Direct_Info content itself is passed to main_cb.
 *                 If gathering, pointers to Eina_File_Direct_Info within Eio_File_Direct_Info
 *                 are added to an Eina_Array. The Eio_File_Direct_Info structs are still freed.
 */
void
_eio_direct_notify(void *data, Ecore_Thread *thread EINA_UNUSED, void *msg_data)
{
   Eio_File_Direct_Ls *async = data;
   Eina_List *pack = msg_data;
   Eio_File_Direct_Info *info;

   async->ls.common.length += eina_list_count(pack);

   // Check if it is an internal use
   if (async->ls.gather)
     {
        Eina_Array *gather;

        gather = eina_array_new(eina_list_count(pack));
        EINA_LIST_FREE(pack, info)
          eina_array_push(gather, &info->info);

        // transfer ownership to caller
        async->main_internal_cb((void*) async->ls.common.data,
                                &async->ls.common,
                                gather);

        return ;
     }

   EINA_LIST_FREE(pack, info)
     {
        async->ls.common.main.associated = info->associated;

        async->main_cb((void*) async->ls.common.data,
                       &async->ls.common,
                       &info->info);

        if (async->ls.common.main.associated)
          {
             eina_hash_free(async->ls.common.main.associated);
             async->ls.common.main.associated = NULL;
          }

        eio_direct_info_free(info);
     }
}

#ifndef MAP_HUGETLB
# define MAP_HUGETLB 0
#endif

/**
 * @brief Worker thread function for file copying.
 *
 * This function is executed in a separate thread to perform the file copy operation.
 * It calls eio_file_copy_do() to do the actual copying.
 *
 * @param data Pointer to an Eio_File_Progress structure containing copy parameters
 *             (source, destination, etc.).
 * @param thread The Ecore_Thread in which this function is executing.
 */
static void
_eio_file_copy_heavy(void *data, Ecore_Thread *thread)
{
   Eio_File_Progress *copy = data;

   eio_file_copy_do(thread, copy);
}

/**
 * @brief Notification callback for file copy progress.
 *
 * This function is called in the main thread when the worker thread
 * (_eio_file_copy_heavy via _eio_file_copy_progress) sends progress information.
 * It calls eio_progress_cb() to forward the progress to the user's callback.
 *
 * @param data Pointer to an Eio_File_Progress structure.
 * @param thread The Ecore_Thread that sent the notification (unused).
 * @param msg_data Pointer to an Eio_Progress structure containing current and total bytes.
 *                 This structure is freed by eio_progress_cb().
 */
static void
_eio_file_copy_notify(void *data, Ecore_Thread *thread EINA_UNUSED, void *msg_data)
{
   Eio_File_Progress *copy = data;

   eio_progress_cb(msg_data, copy);
}

/**
 * @brief Frees resources associated with an Eio_File_Progress structure for a copy operation.
 *
 * This includes stringshared source and destination paths and the common Eio_File structure.
 *
 * @param copy Pointer to the Eio_File_Progress structure to free.
 */
static void
_eio_file_copy_free(Eio_File_Progress *copy)
{
   eina_stringshare_del(copy->source);
   eina_stringshare_del(copy->dest);
   eio_file_free(&copy->common);
}

/**
 * @brief End callback for a successful file copy operation.
 *
 * This function is called in the main thread when the copy operation completes successfully.
 * It invokes the user-provided done_cb and then frees the Eio_File_Progress structure.
 *
 * @param data Pointer to an Eio_File_Progress structure.
 * @param thread The Ecore_Thread (unused).
 */
static void
_eio_file_copy_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Progress *copy = data;

   copy->common.done_cb((void*) copy->common.data, &copy->common);

   _eio_file_copy_free(copy);
}

/**
 * @brief Error callback for a failed file copy operation.
 *
 * This function is called in the main thread if an error occurs during the copy.
 * It invokes the user-provided error_cb (via eio_file_error) and then frees
 * the Eio_File_Progress structure.
 *
 * @param data Pointer to an Eio_File_Progress structure.
 * @param thread The Ecore_Thread (unused).
 */
static void
_eio_file_copy_error(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Progress *copy = data;

   eio_file_error(&copy->common);

   _eio_file_copy_free(copy);
}

/**
 * @brief Frees resources associated with an Eio_File_Move structure.
 *
 * This includes stringshared source and destination paths from its embedded
 * Eio_File_Progress structure, and the common Eio_File structure.
 *
 * @param move Pointer to the Eio_File_Move structure to free.
 */
static void
_eio_file_move_free(Eio_File_Move *move)
{
   eina_stringshare_del(move->progress.source);
   eina_stringshare_del(move->progress.dest);
   eio_file_free(&move->progress.common);
}

/**
 * @brief Progress callback for the copy phase of a file move operation.
 *
 * This function is used when a move operation falls back to copy-then-delete
 * (e.g., across different filesystems). It relays progress information from the
 * underlying copy operation to the progress_cb of the original move request.
 *
 * @param data Pointer to an Eio_File_Move structure.
 * @param handler The Eio_File handler for the copy operation (unused).
 * @param info Pointer to an Eio_Progress structure with current/total bytes for the copy.
 */
static void
_eio_file_move_copy_progress(void *data, Eio_File *handler EINA_UNUSED, const Eio_Progress *info)
{
   Eio_File_Move *move = data;

   move->progress.progress_cb((void*) move->progress.common.data, &move->progress.common, info);
}

/**
 * @brief Done callback for the unlink phase of a file move operation.
 *
 * This is called after successfully unlinking the source file during a
 * copy-then-delete move. It signifies the completion of the entire move
 * operation. It calls the user's done_cb for the move and frees resources.
 *
 * @param data Pointer to an Eio_File_Move structure.
 * @param handler The Eio_File handler for the unlink operation (unused).
 */
static void
_eio_file_move_unlink_done(void *data, Eio_File *handler EINA_UNUSED)
{
   Eio_File_Move *move = data;

   move->progress.common.done_cb((void*) move->progress.common.data, &move->progress.common);

   _eio_file_move_free(move);
}

/**
 * @brief Error callback for the unlink phase of a file move operation.
 *
 * This is called if unlinking the source file fails during a copy-then-delete move.
 * The move operation is considered failed. It sets the error, calls the user's
 * error_cb for the move, and frees resources.
 *
 * @param data Pointer to an Eio_File_Move structure.
 * @param handler The Eio_File handler for the unlink operation (unused).
 * @param error The error code from the unlink operation.
 */
static void
_eio_file_move_unlink_error(void *data, Eio_File *handler EINA_UNUSED, int error)
{
   Eio_File_Move *move = data;

   move->copy = NULL;

   move->progress.common.error = error;
   eio_file_error(&move->progress.common);

   _eio_file_move_free(move);
}

/**
 * @brief Done callback for the copy phase of a file move operation.
 *
 * This is called after successfully copying the file during a copy-then-delete move.
 * It then initiates the unlinking of the source file.
 *
 * @param data Pointer to an Eio_File_Move structure.
 * @param handler The Eio_File handler for the copy operation (unused).
 */
static void
_eio_file_move_copy_done(void *data, Eio_File *handler EINA_UNUSED)
{
   Eio_File_Move *move = data;
   Eio_File *rm;

   rm = eio_file_unlink(move->progress.source,
			_eio_file_move_unlink_done,
			_eio_file_move_unlink_error,
			move);
   if (rm) move->copy = rm;
}

/**
 * @brief Error callback for the copy phase of a file move operation.
 *
 * This is called if copying the file fails during a copy-then-delete move.
 * The move operation is considered failed. It sets the error, calls the user's
 * error_cb for the move, and frees resources.
 *
 * @param data Pointer to an Eio_File_Move structure.
 * @param handler The Eio_File handler for the copy operation (unused).
 * @param error The error code from the copy operation.
 */
static void
_eio_file_move_copy_error(void *data, Eio_File *handler EINA_UNUSED, int error)
{
   Eio_File_Move *move = data;

   move->progress.common.error = error;
   eio_file_error(&move->progress.common);

   _eio_file_move_free(move);
}

/**
 * @brief Worker thread function for file moving.
 *
 * This function is executed in a separate thread. It first attempts to rename
 * the file. If rename() succeeds, it sends a progress update indicating completion.
 * If rename() fails, it signals an error. The main thread error handler
 * (_eio_file_move_error) may then decide to attempt a copy-then-delete strategy
 * if the error is EXDEV.
 *
 * @param data Pointer to an Eio_File_Move structure.
 * @param thread The Ecore_Thread in which this function is executing.
 */
static void
_eio_file_move_heavy(void *data, Ecore_Thread *thread)
{
   Eio_File_Move *move = data;

   if (rename(move->progress.source, move->progress.dest) < 0)
     eio_file_thread_error(&move->progress.common, thread);
   else
     eio_progress_send(thread, &move->progress, 1, 1);
}

/**
 * @brief Notification callback for file move progress.
 *
 * This function is called in the main thread when the worker thread (_eio_file_move_heavy)
 * sends progress, typically after a successful rename indicating 100% completion.
 * It calls eio_progress_cb() to forward the progress.
 *
 * @param data Pointer to an Eio_File_Move structure.
 * @param thread The Ecore_Thread (unused).
 * @param msg_data Pointer to an Eio_Progress structure.
 */
static void
_eio_file_move_notify(void *data, Ecore_Thread *thread EINA_UNUSED, void *msg_data)
{
   Eio_File_Move *move = data;

   eio_progress_cb(msg_data, &move->progress);
}

/**
 * @brief End callback for a successful file move operation (via direct rename).
 *
 * This function is called in the main thread when the move (rename) completes successfully.
 * It invokes the user-provided done_cb and then frees the Eio_File_Move structure.
 * This is typically for moves that succeeded with a simple rename().
 *
 * @param data Pointer to an Eio_File_Move structure.
 * @param thread The Ecore_Thread (unused).
 */
static void
_eio_file_move_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Move *move = data;

   move->progress.common.done_cb((void*) move->progress.common.data, &move->progress.common);

   _eio_file_move_free(move);
}

/**
 * @brief Error callback for a file move operation.
 *
 * This function is called in the main thread if an error occurs during the move.
 * If the error is EXDEV (cross-device link), it attempts to fall back to a
 * copy-then-delete strategy by initiating an eio_file_copy().
 * Otherwise, or if the fallback copy fails to start, it calls the user's error_cb
 * and frees resources.
 *
 * @param data Pointer to an Eio_File_Move structure.
 * @param thread The Ecore_Thread (unused).
 */
static void
_eio_file_move_error(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Move *move = data;

   if (move->copy)
     {
        eio_file_cancel(move->copy);
        return;
     }

   if (move->progress.common.error == EXDEV)
     {
        Eio_File *eio_cp;

        eio_cp = eio_file_copy(move->progress.source, move->progress.dest,
                               move->progress.progress_cb ? _eio_file_move_copy_progress : NULL,
                               _eio_file_move_copy_done,
                               _eio_file_move_copy_error,
                               move);

        if (eio_cp)
          {
             move->copy = eio_cp;
             move->progress.common.thread = ((Eio_File_Progress*)move->copy)->common.thread;
             return;
          }
     }

   eio_file_error(&move->progress.common);

   _eio_file_move_free(move);
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
 * @brief Calls the user-provided progress callback for a file operation.
 *
 * This function is a helper to invoke the Eio_Progress_Cb stored within an
 * Eio_File_Progress structure. It also frees the Eio_Progress data that was
 * passed from the worker thread.
 *
 * @param progress The progress data (e.g., current and total bytes) received from
 *                 the worker thread. This structure is freed by this function.
 * @param op The Eio_File_Progress structure associated with the ongoing operation,
 *           containing the user's callback and data.
 */
void
eio_progress_cb(Eio_Progress *progress, Eio_File_Progress *op)
{
   op->progress_cb((void *) op->common.data, &op->common, progress);

   eio_progress_free(progress);
}

/**
 * @brief Internal progress callback for eina_file_copy.
 *
 * This function is passed to eina_file_copy() to receive progress updates.
 * It then sends these updates to the main thread using eio_progress_send().
 * It also checks if the Ecore_Thread has been cancelled.
 *
 * @param data A context array: `void *ctx[2] = {thread, copy_op_struct};`.
 *             `ctx[0]` is the Ecore_Thread*.
 *             `ctx[1]` is the Eio_File_Progress*.
 * @param done The number of bytes copied so far.
 * @param total The total number of bytes to copy.
 * @return EINA_TRUE to continue copying, EINA_FALSE to cancel.
 */
static Eina_Bool
_eio_file_copy_progress(void *data, unsigned long long done, unsigned long long total)
{
   void **ctx = data;
   Ecore_Thread *thread = ctx[0];
   Eio_File_Progress *copy = ctx[1];

   eio_progress_send(thread, copy, done, total);

   return !ecore_thread_check(thread);
}

/**
 * @brief Performs the actual file copy using eina_file_copy.
 *
 * This function is called by the worker thread (_eio_file_copy_heavy) to execute
 * the blocking file copy operation. It sets up the progress callback context
 * for eina_file_copy.
 *
 * @param thread The Ecore_Thread in which the copy is being performed. Used by
 *               _eio_file_copy_progress to send updates and check for cancellation.
 * @param copy The Eio_File_Progress structure containing source, destination,
 *             and other operation details.
 * @return EINA_TRUE if eina_file_copy was successfully initiated, EINA_FALSE otherwise.
 *         If EINA_FALSE, an error is signaled to the main thread.
 */
Eina_Bool
eio_file_copy_do(Ecore_Thread *thread, Eio_File_Progress *copy)
{
   void *ctx[2] = {thread, copy};
   Eina_Bool ret = eina_file_copy(copy->source, copy->dest,
                                  (EINA_FILE_COPY_PERMISSION |
                                   EINA_FILE_COPY_XATTR),
                                  _eio_file_copy_progress,
                                  ctx);

   if (!ret)
     {
        eio_file_thread_error(&copy->common, thread);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @brief Frees resources associated with an Eio_File_Ls structure.
 *
 * This is a common cleanup function for asynchronous listing operations.
 * It frees the stringshared directory path and the common Eio_File structure.
 * The Eina_Iterator (async->ls) should be freed separately before calling this.
 *
 * @param async Pointer to the Eio_File_Ls structure to free.
 */
void
eio_async_free(Eio_File_Ls *async)
{
   eina_stringshare_del(async->directory);
   eio_file_free(&async->common);
}

/**
 * @brief Generic end callback for successful asynchronous listing operations.
 *
 * This function is called in the main thread when an ls-like operation
 * (e.g., eio_file_ls, eio_file_direct_ls) completes successfully.
 * It invokes the user-provided done_cb, frees the Eina_Iterator,
 * and then calls eio_async_free() to release other resources.
 *
 * @param data Pointer to an Eio_File_Ls (or compatible, like Eio_File_Direct_Ls) structure.
 * @param thread The Ecore_Thread (unused).
 */
void
eio_async_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Ls *async = data;

   async->common.done_cb((void*) async->common.data, &async->common);
   eio_file_container_set(&async->common, NULL);
   eina_iterator_free(async->ls);
   eio_async_free(async);
}

/**
 * @brief Generic error callback for failed asynchronous listing operations.
 *
 * This function is called in the main thread if an error occurs during an
 * ls-like operation. It invokes the user-provided error_cb (via eio_file_error)
 * and then calls eio_async_free() to release resources. The Eina_Iterator
 * might not be valid or fully populated in error cases, so it's not explicitly freed here
 * (it's assumed to be handled or irrelevant if the operation failed early).
 *
 * @param data Pointer to an Eio_File_Ls (or compatible) structure.
 * @param thread The Ecore_Thread (unused).
 */
void
eio_async_error(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Ls *async = data;

   eio_file_error(&async->common);

   eio_async_free(async);
}

/**
 * @endcond
 */


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @brief Internal implementation for asynchronous directory listing (string filenames).
 *
 * This function sets up and starts an asynchronous operation to list files in a directory.
 * It's the core logic used by eio_file_ls() and _eio_file_ls().
 * Results are delivered as string filenames.
 *
 * @param dir The directory path to list.
 * @param filter_cb Optional callback to filter files in the worker thread.
 *                  `Eina_Bool filter_cb(void *data, Eio_File *handler, const char *file)`
 * @param main_cb Callback invoked for each file found (if not gathering).
 *                `void main_cb(void *data, Eio_File *handler, const char *file)`
 * @param main_internal_cb Callback invoked with an array of all files (if gathering).
 *                         `void main_internal_cb(void *data, Eio_File *handler, Eina_Array *files)`
 *                         The Eina_Array contains `const char *` filenames.
 * @param done_cb Callback invoked when the listing is complete.
 *                `void done_cb(void *data, Eio_File *handler)`
 * @param error_cb Callback invoked if an error occurs.
 *                 `void error_cb(void *data, Eio_File *handler, int error_code)`
 * @param data User data passed to all callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure to start.
 *         This handle can be used with eio_file_cancel() or eio_file_check().
 */
static Eio_File *
_eio_file_internal_ls(const char *dir,
                      Eio_Filter_Cb filter_cb,
                      Eio_Main_Cb main_cb,
                      Eio_Array_Cb main_internal_cb,
                      Eio_Done_Cb done_cb,
                      Eio_Error_Cb error_cb,
                      const void *data)
{
   Eio_File_Char_Ls *async;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dir, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = eio_common_alloc(sizeof(Eio_File_Char_Ls));
   EINA_SAFETY_ON_NULL_RETURN_VAL(async, NULL);

   async->ls.directory = eina_stringshare_add(dir);
   async->filter_cb = filter_cb;
   if (main_internal_cb)
     {
        async->main_internal_cb = main_internal_cb;
        async->ls.gather = EINA_TRUE;
     }
   else
     {
        async->main_cb = main_cb;
     }

   if (!eio_long_file_set(&async->ls.common,
			  done_cb,
			  error_cb,
			  data,
			  _eio_file_heavy,
			  _eio_string_notify,
			  eio_async_end,
			  eio_async_error))
     return NULL;

   return &async->ls.common;
}

/**
 * @brief Asynchronously lists files in a directory, gathering results into an Eina_Array.
 *
 * This is an internal variant of eio_file_ls that uses an Eio_Array_Cb
 * to deliver all filenames in a single Eina_Array upon completion.
 * No filter callback is supported in this variant.
 *
 * @param dir The directory path to list.
 * @param main_internal_cb Callback invoked once with an Eina_Array of all filenames.
 *                         The Eina_Array contains `const char *` (stringshared) filenames.
 *                         The caller is responsible for freeing the Eina_Array and its contents
 *                         (e.g., by iterating and calling eina_stringshare_del on each filename,
 *                         then eina_array_free).
 *                         `void main_internal_cb(void *data, Eio_File *handler, Eina_Array *filenames)`
 * @param done_cb Callback invoked when the listing is complete (after main_internal_cb).
 * @param error_cb Callback invoked if an error occurs.
 * @param data User data passed to all callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 * @see eio_file_ls()
 * @see _eio_file_internal_ls()
 */
EIO_API Eio_File *
eio_file_ls(const char *dir,
	    Eio_Filter_Cb filter_cb,
	    Eio_Main_Cb main_cb,
	    Eio_Done_Cb done_cb,
	    Eio_Error_Cb error_cb,
	    const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_cb, NULL);

   return _eio_file_internal_ls(dir, filter_cb, main_cb, NULL, done_cb, error_cb, data);
}

/**
 * @brief Asynchronously lists files in a directory, gathering results into an Eina_Array.
 * @deprecated Use eio_file_ls_array_get() instead if available, or manage through standard eio_file_ls.
 * This function is a wrapper around _eio_file_internal_ls for a specific internal use case.
 *
 * This variant of file listing collects all filenames into an Eina_Array
 * and delivers them via the `main_internal_cb`.
 *
 * @param dir The directory path to list.
 * @param main_internal_cb Callback invoked once with an Eina_Array of `const char *` filenames.
 *                         The array and its stringshared contents become the responsibility of the callback.
 *                         Example: `void callback(void *data, Eio_File *handler, Eina_Array *filenames)`
 *                         `filenames` would contain `["file1.txt", "file2.png", ...]`.
 * @param done_cb Callback invoked when the listing is complete.
 * @param error_cb Callback invoked if an error occurs.
 * @param data User data passed to all callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 */
Eio_File *
_eio_file_ls(const char *dir,
             Eio_Array_Cb main_internal_cb,
             Eio_Done_Cb done_cb,
             Eio_Error_Cb error_cb,
             const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_internal_cb, NULL);

   return _eio_file_internal_ls(dir, NULL, NULL, main_internal_cb, done_cb, error_cb, data);
}

/**
 * @brief Internal implementation for asynchronous directory listing (Eina_File_Direct_Info).
 *
 * This function sets up and starts an asynchronous operation to list files in a directory.
 * It's the core logic used by eio_file_direct_ls() and _eio_file_direct_ls().
 * Results are delivered as Eina_File_Direct_Info structures, providing more detailed
 * file information (like type, path) without needing a separate stat call for basic info.
 *
 * @param dir The directory path to list.
 * @param filter_cb Optional callback to filter files in the worker thread.
 *                  `Eina_Bool filter_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)`
 * @param main_cb Callback invoked for each file found (if not gathering).
 *                `void main_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)`
 * @param main_internal_cb Callback invoked with an array of all file info (if gathering).
 *                         `void main_internal_cb(void *data, Eio_File *handler, Eina_Array *infos)`
 *                         The Eina_Array contains pointers to Eina_File_Direct_Info structures.
 *                         These structures are typically part of larger Eio_File_Direct_Info items
 *                         managed by the Eio operation, and their lifetime should be respected.
 * @param done_cb Callback invoked when the listing is complete.
 * @param error_cb Callback invoked if an error occurs.
 * @param data User data passed to all callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 */
static Eio_File *
_eio_file_direct_internal_ls(const char *dir,
                             Eio_Filter_Direct_Cb filter_cb,
                             Eio_Main_Direct_Cb main_cb,
                             Eio_Array_Cb main_internal_cb,
                             Eio_Done_Cb done_cb,
                             Eio_Error_Cb error_cb,
                             const void *data)
{
   Eio_File_Direct_Ls *async;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dir, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = eio_common_alloc(sizeof(Eio_File_Direct_Ls));
   EINA_SAFETY_ON_NULL_RETURN_VAL(async, NULL);

   async->ls.directory = eina_stringshare_add(dir);
   async->filter_cb = filter_cb;
   if (main_internal_cb)
     {
        async->main_internal_cb = main_internal_cb;
        async->ls.gather = EINA_TRUE;
     }
   else
     {
        async->main_cb = main_cb;
     }

   if (!eio_long_file_set(&async->ls.common,
			  done_cb,
			  error_cb,
			  data,
			  _eio_file_direct_heavy,
			  _eio_direct_notify,
			  eio_async_end,
			  eio_async_error))
     return NULL;

   return &async->ls.common;
}

/**
 * @brief Asynchronously lists files in a directory with detailed Eina_File_Direct_Info,
 * gathering results into an Eina_Array.
 * @deprecated Use eio_file_direct_ls_array_get() or similar if available.
 * This function is a wrapper around _eio_file_direct_internal_ls for a specific internal use case.
 *
 * This variant collects all Eina_File_Direct_Info structures into an Eina_Array
 * and delivers them via the `main_internal_cb`.
 *
 * @param dir The directory path to list.
 * @param main_internal_cb Callback invoked once with an Eina_Array of `Eina_File_Direct_Info*`.
 *                         The array contains pointers to `Eina_File_Direct_Info` structs.
 *                         The actual `Eina_File_Direct_Info` data is valid during the callback.
 *                         If long-term storage is needed, data must be copied.
 *                         Example: `void callback(void *data, Eio_File *handler, Eina_Array *infos)`
 *                         `infos` would contain `[info_ptr1, info_ptr2, ...]`.
 * @param done_cb Callback invoked when the listing is complete.
 * @param error_cb Callback invoked if an error occurs.
 * @param data User data passed to all callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 */
EIO_API Eio_File *
eio_file_direct_ls(const char *dir,
		   Eio_Filter_Direct_Cb filter_cb,
		   Eio_Main_Direct_Cb main_cb,
		   Eio_Done_Cb done_cb,
		   Eio_Error_Cb error_cb,
		   const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_cb, NULL);

   return _eio_file_direct_internal_ls(dir, filter_cb, main_cb, NULL, done_cb, error_cb, data);
}

/**
 * @brief Asynchronously lists files with Eina_File_Direct_Info, gathering results into an Eina_Array.
 * @deprecated Use eio_file_direct_ls_array_get() or similar if available.
 * This function is a wrapper around _eio_file_direct_internal_ls for a specific internal use case.
 *
 * This variant collects all Eina_File_Direct_Info structures into an Eina_Array
 * and delivers them via the `main_internal_cb`.
 *
 * @param dir The directory path to list.
 * @param main_internal_cb Callback invoked once with an Eina_Array of `Eina_File_Direct_Info*`.
 *                         The array contains pointers to `Eina_File_Direct_Info` structs.
 *                         The actual `Eina_File_Direct_Info` data is valid during the callback.
 *                         If long-term storage is needed, data must be copied.
 *                         Example: `void callback(void *data, Eio_File *handler, Eina_Array *infos)`
 *                         `infos` would contain `[info_ptr1, info_ptr2, ...]`.
 * @param done_cb Callback invoked when the listing is complete.
 * @param error_cb Callback invoked if an error occurs.
 * @param data User data passed to all callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 */
Eio_File *
_eio_file_direct_ls(const char *dir,
                    Eio_Array_Cb main_internal_cb,
                    Eio_Done_Cb done_cb,
                    Eio_Error_Cb error_cb,
                    const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_internal_cb, NULL);

   return _eio_file_direct_internal_ls(dir, NULL, NULL, main_internal_cb, done_cb, error_cb, data);
}

/**
 * @brief Internal implementation for asynchronous directory listing with stat info.
 *
 * This function sets up and starts an asynchronous operation to list files in a directory,
 * retrieving full stat information for each file (similar to `ls -l`).
 * It's the core logic used by eio_file_stat_ls() and _eio_file_stat_ls().
 * Results are delivered as Eina_File_Direct_Info structures, populated from stat data.
 *
 * @param dir The directory path to list.
 * @param filter_cb Optional callback to filter files in the worker thread.
 *                  `Eina_Bool filter_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)`
 * @param main_cb Callback invoked for each file found (if not gathering).
 *                `void main_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)`
 * @param main_internal_cb Callback invoked with an array of all file info (if gathering).
 *                         `void main_internal_cb(void *data, Eio_File *handler, Eina_Array *infos)`
 *                         The Eina_Array contains pointers to Eina_File_Direct_Info structures.
 * @param done_cb Callback invoked when the listing is complete.
 * @param error_cb Callback invoked if an error occurs.
 * @param data User data passed to all callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 */
static Eio_File *
_eio_file_stat_internal_ls(const char *dir,
                           Eio_Filter_Direct_Cb filter_cb,
                           Eio_Main_Direct_Cb main_cb,
                           Eio_Array_Cb main_internal_cb,
                           Eio_Done_Cb done_cb,
                           Eio_Error_Cb error_cb,
                           const void *data)
{
   Eio_File_Direct_Ls *async;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dir, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   async = eio_common_alloc(sizeof(Eio_File_Direct_Ls));
   EINA_SAFETY_ON_NULL_RETURN_VAL(async, NULL);

   async->ls.directory = eina_stringshare_add(dir);
   async->filter_cb = filter_cb;
   if (main_internal_cb)
     {
        async->main_internal_cb = main_internal_cb;
        async->ls.gather = EINA_TRUE;
     }
   else
     {
        async->main_cb = main_cb;
     }

   if (!eio_long_file_set(&async->ls.common,
			  done_cb,
			  error_cb,
			  data,
			  _eio_file_stat_heavy,
			  _eio_direct_notify,
			  eio_async_end,
			  eio_async_error))
     return NULL;

   return &async->ls.common;
}

/**
 * @brief Asynchronously lists files in a directory with full stat information,
 * gathering results into an Eina_Array.
 * @deprecated Use eio_file_stat_ls_array_get() or similar if available.
 * This function is a wrapper around _eio_file_stat_internal_ls for a specific internal use case.
 *
 * This variant collects all Eina_File_Direct_Info (populated by stat) structures
 * into an Eina_Array and delivers them via the `main_internal_cb`.
 *
 * @param dir The directory path to list.
 * @param main_internal_cb Callback invoked once with an Eina_Array of `Eina_File_Direct_Info*`.
 *                         The array contains pointers to `Eina_File_Direct_Info` structs.
 *                         The actual `Eina_File_Direct_Info` data is valid during the callback.
 *                         If long-term storage is needed, data must be copied.
 * @param done_cb Callback invoked when the listing is complete.
 * @param error_cb Callback invoked if an error occurs.
 * @param data User data passed to all callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 */
EIO_API Eio_File *
eio_file_stat_ls(const char *dir,
                 Eio_Filter_Direct_Cb filter_cb,
                 Eio_Main_Direct_Cb main_cb,
                 Eio_Done_Cb done_cb,
                 Eio_Error_Cb error_cb,
                 const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_cb, NULL);

   return _eio_file_stat_internal_ls(dir, filter_cb, main_cb, NULL, done_cb, error_cb, data);
}

/**
 * @brief Asynchronously lists files with stat info, gathering results into an Eina_Array.
 * @deprecated Use eio_file_stat_ls_array_get() or similar if available.
 * This function is a wrapper around _eio_file_stat_internal_ls for a specific internal use case.
 *
 * This variant collects all Eina_File_Direct_Info (populated by stat) structures
 * into an Eina_Array and delivers them via the `main_internal_cb`.
 *
 * @param dir The directory path to list.
 * @param main_internal_cb Callback invoked once with an Eina_Array of `Eina_File_Direct_Info*`.
 *                         The array contains pointers to `Eina_File_Direct_Info` structs.
 *                         The actual `Eina_File_Direct_Info` data is valid during the callback.
 *                         If long-term storage is needed, data must be copied.
 * @param done_cb Callback invoked when the listing is complete.
 * @param error_cb Callback invoked if an error occurs.
 * @param data User data passed to all callbacks.
 * @return An Eio_File handle for the operation, or NULL on failure.
 */
Eio_File *
_eio_file_stat_ls(const char *dir,
                 Eio_Array_Cb main_internal_cb,
                 Eio_Done_Cb done_cb,
                 Eio_Error_Cb error_cb,
                 const void *data)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(main_internal_cb, NULL);

   return _eio_file_stat_internal_ls(dir, NULL, NULL, main_internal_cb, done_cb, error_cb, data);
}

EIO_API Eina_Bool
eio_file_cancel(Eio_File *ls)
{
   if (!ls) return EINA_FALSE;
   EINA_SAFETY_ON_NULL_RETURN_VAL(ls, EINA_FALSE);
   // ensure callbacks are not called aftera  cancel otherwise bad things
   // happen higher up the stack - you cant stop these being caleld even if
   // the dataptr they are passed has been freed or invalidated. being unable
   // to stop future cb's and cancel them is BAD.
   ls->error_cb = NULL;;
   ls->done_cb = NULL;
   return ecore_thread_cancel(ls->thread);
}

EIO_API Eina_Bool
eio_file_check(Eio_File *ls)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ls, EINA_TRUE);
   return ecore_thread_check(ls->thread);
}

EIO_API void *
eio_file_container_get(Eio_File *ls)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ls, NULL);
   return ls->container;
}

EIO_API Eina_Bool
eio_file_associate_add(Eio_File *ls,
                       const char *key,
                       const void *data, Eina_Free_Cb free_cb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ls, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(key, EINA_FALSE);
   /* FIXME: Check if we are in the right worker thread */
   if (!ls->worker.associated)
     ls->worker.associated = eina_hash_string_small_new(eio_associate_free);

   return eina_hash_add(ls->worker.associated,
                        key,
                        eio_associate_malloc(data, free_cb));
}

EIO_API Eina_Bool
eio_file_associate_direct_add(Eio_File *ls,
                              const char *key,
                              const void *data, Eina_Free_Cb free_cb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(ls, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(key, EINA_FALSE);
   /* FIXME: Check if we are in the right worker thread */
   if (!ls->worker.associated)
     ls->worker.associated = eina_hash_string_small_new(eio_associate_free);

   return eina_hash_direct_add(ls->worker.associated,
                               key,
                               eio_associate_malloc(data, free_cb));
}

EIO_API void *
eio_file_associate_find(Eio_File *ls, const char *key)
{
   Eio_File_Associate *search;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ls, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(key, NULL);
   if (!ls->main.associated)
     return NULL;

   search = eina_hash_find(ls->main.associated, key);
   if (!search) return NULL;
   return search->data;
}

EIO_API Eio_File *
eio_file_copy(const char *source,
	      const char *dest,
	      Eio_Progress_Cb progress_cb,
	      Eio_Done_Cb done_cb,
	      Eio_Error_Cb error_cb,
	      const void *data)
{
   Eio_File_Progress *copy;

   EINA_SAFETY_ON_NULL_RETURN_VAL(source, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(dest, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   copy = eio_common_alloc(sizeof(Eio_File_Progress));
   EINA_SAFETY_ON_NULL_RETURN_VAL(copy, NULL);

   copy->op = EIO_FILE_COPY;
   copy->progress_cb = progress_cb;
   copy->source = eina_stringshare_add(source);
   copy->dest = eina_stringshare_add(dest);

   if (!eio_long_file_set(&copy->common,
			  done_cb,
			  error_cb,
			  data,
			  _eio_file_copy_heavy,
			  _eio_file_copy_notify,
			  _eio_file_copy_end,
			  _eio_file_copy_error))
     return NULL;

   return &copy->common;
}

EIO_API Eio_File *
eio_file_move(const char *source,
	      const char *dest,
	      Eio_Progress_Cb progress_cb,
	      Eio_Done_Cb done_cb,
	      Eio_Error_Cb error_cb,
	      const void *data)
{
   Eio_File_Move *move;

   EINA_SAFETY_ON_NULL_RETURN_VAL(source, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(dest, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   move = eio_common_alloc(sizeof(Eio_File_Move));
   EINA_SAFETY_ON_NULL_RETURN_VAL(move, NULL);

   move->progress.op = EIO_FILE_MOVE;
   move->progress.progress_cb = progress_cb;
   move->progress.source = eina_stringshare_add(source);
   move->progress.dest = eina_stringshare_add(dest);
   move->copy = NULL;

   if (!eio_long_file_set(&move->progress.common,
			  done_cb,
			  error_cb,
			  data,
			  _eio_file_move_heavy,
			  _eio_file_move_notify,
			  _eio_file_move_end,
			  _eio_file_move_error))
     return NULL;

   return &move->progress.common;
}
