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
 * @brief Worker thread function to create a directory.
 * @param data Pointer to Eio_File_Mkdir structure.
 * @param thread The Ecore_Thread executing this function.
 */
static void
_eio_file_mkdir(void *data, Ecore_Thread *thread)
{
   Eio_File_Mkdir *m = data;

   if (mkdir(m->path, m->mode) != 0)
     eio_file_thread_error(&m->common, thread);
}

/**
 * @brief Frees resources associated with an Eio_File_Mkdir operation.
 * @param m Pointer to the Eio_File_Mkdir structure to free.
 */
static void
_eio_mkdir_free(Eio_File_Mkdir *m)
{
   eina_stringshare_del(m->path);
   eio_file_free(&m->common);
}

/**
 * @brief Callback executed in the main loop when directory creation succeeds.
 * @param data Pointer to Eio_File_Mkdir structure.
 * @param thread The Ecore_Thread that executed the operation (unused).
 */
static void
_eio_file_mkdir_done(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Mkdir *m = data;

   if (m->common.done_cb)
     m->common.done_cb((void*) m->common.data, &m->common);

   _eio_mkdir_free(m);
}

/**
 * @brief Callback executed in the main loop when directory creation fails.
 * @param data Pointer to Eio_File_Mkdir structure.
 * @param thread The Ecore_Thread that executed the operation (unused).
 */
static void
_eio_file_mkdir_error(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Mkdir *m = data;

   eio_file_error(&m->common);
   _eio_mkdir_free(m);
}

/**
 * @brief Worker thread function to delete a file.
 * @param data Pointer to Eio_File_Unlink structure.
 * @param thread The Ecore_Thread executing this function.
 */
static void
_eio_file_unlink(void *data, Ecore_Thread *thread)
{
   Eio_File_Unlink *l = data;

   if (unlink(l->path) != 0)
     eio_file_thread_error(&l->common, thread);
}

/**
 * @brief Frees resources associated with an Eio_File_Unlink operation.
 * @param l Pointer to the Eio_File_Unlink structure to free.
 */
static void
_eio_unlink_free(Eio_File_Unlink *l)
{
   eina_stringshare_del(l->path);
   eio_file_free(&l->common);
}

/**
 * @brief Callback executed in the main loop when file deletion succeeds.
 * @param data Pointer to Eio_File_Unlink structure.
 * @param thread The Ecore_Thread that executed the operation (unused).
 */
static void
_eio_file_unlink_done(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Unlink *l = data;

   if (l->common.done_cb)
     l->common.done_cb((void*) l->common.data, &l->common);

   _eio_unlink_free(l);
}

/**
 * @brief Callback executed in the main loop when file deletion fails.
 * @param data Pointer to Eio_File_Unlink structure.
 * @param thread The Ecore_Thread that executed the operation (unused).
 */
static void
_eio_file_unlink_error(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Unlink *l = data;

   eio_file_error(&l->common);
   _eio_unlink_free(l);
}

/**
 * @brief Converts a system stat structure (_eio_stat_t) to an Eina_Stat structure.
 * @param es Pointer to the destination Eina_Stat structure.
 * @param st Pointer to the source _eio_stat_t structure.
 */
static void
_eio_file_struct_2_eina(Eina_Stat *es, _eio_stat_t *st)
{
   es->dev = st->st_dev;
   es->ino = st->st_ino;
   es->mode = st->st_mode;
   es->nlink = st->st_nlink;
   es->uid = st->st_uid;
   es->gid = st->st_gid;
   es->rdev = st->st_rdev;
   es->size = st->st_size;
#ifdef _WIN32
   es->blksize = 0;
   es->blocks = 0;
#else
   es->blksize = st->st_blksize;
   es->blocks = st->st_blocks;
#endif
   es->atime = st->st_atime;
   es->mtime = st->st_mtime;
   es->ctime = st->st_ctime;
#ifdef _STAT_VER_LINUX
   es->atimensec = st->st_atim.tv_nsec;
   es->mtimensec = st->st_mtim.tv_nsec;
   es->ctimensec = st->st_ctim.tv_nsec;
#else
   es->atimensec = 0;
   es->mtimensec = 0;
   es->ctimensec = 0;
#endif
}

/**
 * @brief Worker thread function to get file status (stat).
 * @param data Pointer to Eio_File_Stat structure.
 * @param thread The Ecore_Thread executing this function.
 */
static void
_eio_file_stat(void *data, Ecore_Thread *thread)
{
   Eio_File_Stat *s = data;
   _eio_stat_t buf;

   if (_eio_stat(s->path, &buf) != 0)
     eio_file_thread_error(&s->common, thread);

   _eio_file_struct_2_eina(&s->buffer, &buf);
}

/**
 * @brief Worker thread function to get file status (lstat, does not follow symlinks).
 * @param data Pointer to Eio_File_Stat structure.
 * @param thread The Ecore_Thread executing this function.
 */
static void
_eio_file_lstat(void *data, Ecore_Thread *thread)
{
   Eio_File_Stat *s = data;
   _eio_stat_t buf;

   if (_eio_lstat(s->path, &buf) != 0)
     eio_file_thread_error(&s->common, thread);

   _eio_file_struct_2_eina(&s->buffer, &buf);
}

/**
 * @brief Frees resources associated with an Eio_File_Stat operation.
 * @param s Pointer to the Eio_File_Stat structure to free.
 */
static void
_eio_stat_free(Eio_File_Stat *s)
{
   eina_stringshare_del(s->path);
   eio_file_free(&s->common);
}

/**
 * @brief Callback executed in the main loop when stat/lstat operation succeeds.
 * @param data Pointer to Eio_File_Stat structure.
 * @param thread The Ecore_Thread that executed the operation (unused).
 */
static void
_eio_file_stat_done(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Stat *s = data;

   if (s->done_cb)
     s->done_cb((void*) s->common.data, &s->common, &s->buffer);

   _eio_stat_free(s);
}

/**
 * @brief Callback executed in the main loop when stat/lstat operation fails.
 * @param data Pointer to Eio_File_Stat structure.
 * @param thread The Ecore_Thread that executed the operation (unused).
 */
static void
_eio_file_stat_error(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Stat *s = data;

   eio_file_error(&s->common);
   _eio_stat_free(s);
}

/**
 * @brief Worker thread function to change file permissions.
 * @param data Pointer to Eio_File_Chmod structure.
 * @param thread The Ecore_Thread executing this function.
 */
static void
_eio_file_chmod(void *data, Ecore_Thread *thread)
{
   Eio_File_Chmod *ch = data;

   if (chmod(ch->path, ch->mode) != 0)
     eio_file_thread_error(&ch->common, thread);
}

#if defined(HAVE_CHOWN) && defined(HAVE_GETPWENT)
/**
 * @brief Worker thread function to change file ownership.
 * @param data Pointer to Eio_File_Chown structure.
 * @param thread The Ecore_Thread executing this function.
 * @note This function attempts to resolve user and group names to UID/GID.
 *       If conversion fails, it sets an error and cancels the thread.
 */
static void
_eio_file_chown(void *data, Ecore_Thread *thread)
{
   Eio_File_Chown *own = data;
   char *tmp;
   uid_t owner = -1;
   gid_t group = -1;

   own->common.error = 0;

   if (own->user)
     {
        owner = strtol(own->user, &tmp, 10);

        if (*tmp != '\0')
          {
             struct passwd *pw = NULL;

             own->common.error = EIO_FILE_GETPWNAM;

             pw = getpwnam(own->user);
             if (!pw) goto on_error;

             owner = pw->pw_uid;
          }
     }

   if (own->group)
     {
        group = strtol(own->group, &tmp, 10);

        if (*tmp != '\0')
          {
             struct group *grp = NULL;

             own->common.error = EIO_FILE_GETGRNAM;

             grp = getgrnam(own->group);
             if (!grp) goto on_error;

             group = grp->gr_gid;
          }
     }

   if (owner == (uid_t) -1 && group == (gid_t) -1)
     goto on_error;

   if (chown(own->path, owner, group) != 0)
     eio_file_thread_error(&own->common, thread);

   return;

 on_error:
   ecore_thread_cancel(thread);
   return;
}

/**
 * @brief Frees resources associated with an Eio_File_Chown operation.
 * @param ch Pointer to the Eio_File_Chown structure to free.
 */
static void
_eio_chown_free(Eio_File_Chown *ch)
{
   if (ch->user) eina_stringshare_del(ch->user);
   if (ch->group) eina_stringshare_del(ch->group);
   eina_stringshare_del(ch->path);
   eio_file_free(&ch->common);
}

/**
 * @brief Callback executed in the main loop when file ownership change succeeds.
 * @param data Pointer to Eio_File_Chown structure.
 * @param thread The Ecore_Thread that executed the operation (unused).
 */
static void
_eio_file_chown_done(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Chown *ch = data;

   if (ch->common.done_cb)
     ch->common.done_cb((void*) ch->common.data, &ch->common);

   _eio_chown_free(ch);
}

/**
 * @brief Callback executed in the main loop when file ownership change fails.
 * @param data Pointer to Eio_File_Chown structure.
 * @param thread The Ecore_Thread that executed the operation (unused).
 */
static void
_eio_file_chown_error(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_File_Chown *ch = data;

   eio_file_error(&ch->common);
   _eio_chown_free(ch);
}
#endif

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
 * @brief Invokes the error callback for an Eio_File operation.
 * @param common Pointer to the Eio_File structure.
 * @details This function is called from the main loop when an error occurs
 *          in a worker thread and has been signaled. It calls the user-provided
 *          error callback and sets the thread pointer to NULL.
 */
void
eio_file_error(Eio_File *common)
{
   if (common->error_cb)
     common->error_cb((void*) common->data, common, common->error);
   common->thread = NULL;
}

/**
 * @brief Sets the error code and cancels the Ecore_Thread.
 * @param common Pointer to the Eio_File structure.
 * @param thread The Ecore_Thread in which the error occurred.
 * @details This function is called from within a worker thread when an error
 *          is encountered. It stores `errno` in `common->error` and requests
 *          the cancellation of the thread.
 */
void
eio_file_thread_error(Eio_File *common, Ecore_Thread *thread)
{
   common->error = errno;
   ecore_thread_cancel(thread);
}

/**
 * @brief Frees an Eio_File structure and associated data.
 * @param common Pointer to the Eio_File structure to free.
 * @details This function frees any associated data hashes, unregisters the
 *          file operation, and then frees the common Eio_File structure itself.
 */
void
eio_file_free(Eio_File *common)
{
   if (common->worker.associated)
     eina_hash_free(common->worker.associated);
   if (common->main.associated)
     eina_hash_free(common->main.associated);
   eio_file_unregister(common);
   eio_common_free(common);
}

/**
 * @brief Sets up and starts a long-running Eio_File operation with feedback.
 * @param common Pointer to the Eio_File structure.
 * @param done_cb Callback for successful completion (not used directly here, typically for end_cb).
 * @param error_cb Callback for error conditions.
 * @param data User data to pass to callbacks.
 * @param heavy_cb The actual worker function to execute in the thread.
 * @param notify_cb Callback for progress notifications from the worker thread.
 * @param end_cb Callback for when the thread finishes successfully (main loop).
 * @param cancel_cb Callback for when the thread is cancelled or finishes due to error (main loop).
 * @return EINA_TRUE if the thread was successfully started, EINA_FALSE otherwise.
 * @details This function initializes the Eio_File structure and launches a new
 *          thread using `ecore_thread_feedback_run`. It registers the Eio_File
 *          if the thread starts successfully.
 */
Eina_Bool
eio_long_file_set(Eio_File *common,
		  Eio_Done_Cb done_cb,
		  Eio_Error_Cb error_cb,
		  const void *data,
		  Ecore_Thread_Cb heavy_cb,
		  Ecore_Thread_Notify_Cb notify_cb,
		  Ecore_Thread_Cb end_cb,
		  Ecore_Thread_Cb cancel_cb)
{
   Ecore_Thread *thread;

   common->done_cb = done_cb;
   common->error_cb = error_cb;
   common->data = data;
   common->error = 0;
   common->length = 0;
   common->thread = NULL;
   common->container = NULL;
   common->worker.associated = NULL;
   common->main.associated = NULL;

   /* Be aware that ecore_thread_feedback_run could call cancel_cb if something goes wrong.
      This means that common would be destroyed if thread == NULL.
    */
   thread = ecore_thread_feedback_run(heavy_cb,
                                      notify_cb,
                                      end_cb,
                                      cancel_cb,
                                      common,
                                      EINA_FALSE);
   if (thread)
     {
        common->thread = thread;
        eio_file_register(common);
     }
   return !!thread;
}

/**
 * @brief Sets up and starts a standard Eio_File operation.
 * @param common Pointer to the Eio_File structure.
 * @param done_cb Callback for successful completion (not used directly here, typically for end_cb).
 * @param error_cb Callback for error conditions.
 * @param data User data to pass to callbacks.
 * @param job_cb The actual worker function to execute in the thread.
 * @param end_cb Callback for when the thread finishes successfully (main loop).
 * @param cancel_cb Callback for when the thread is cancelled or finishes due to error (main loop).
 * @return EINA_TRUE if the thread was successfully started, EINA_FALSE otherwise.
 * @details This function initializes the Eio_File structure and launches a new
 *          thread using `ecore_thread_run`. It registers the Eio_File
 *          if the thread starts successfully.
 */
Eina_Bool
eio_file_set(Eio_File *common,
	     Eio_Done_Cb done_cb,
	     Eio_Error_Cb error_cb,
	     const void *data,
	     Ecore_Thread_Cb job_cb,
	     Ecore_Thread_Cb end_cb,
	     Ecore_Thread_Cb cancel_cb)
{
   Ecore_Thread *thread;

   common->done_cb = done_cb;
   common->error_cb = error_cb;
   common->data = data;
   common->error = 0;
   common->length = 0;
   common->thread = NULL;
   common->container = NULL;
   common->worker.associated = NULL;
   common->main.associated = NULL;

   /* Be aware that ecore_thread_run could call cancel_cb if something goes wrong.
      This means that common would be destroyed if thread == NULL.
   */
   thread = ecore_thread_run(job_cb, end_cb, cancel_cb, common);

   if (thread)
     {
        common->thread = thread;
        eio_file_register(common);
     }
   return !!thread;
}

/**
 * @brief Associates a container object with an Eio_File operation.
 * @param common Pointer to the Eio_File structure.
 * @param container Pointer to the container object.
 * @details This is typically used when an Eio_File operation is part of a larger
 *          construct, like an Eio_Monitor or Eio_Ls. The container points back
 *          to that parent object.
 */
void
eio_file_container_set(Eio_File *common, void *container)
{
   common->container = container;
}

/**
 * @endcond
 */


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @brief Asynchronously get file status information (like stat(2)).
 * @param path The path to the file or directory.
 * @param done_cb Function to call on success. It receives the user data, the Eio_File handle, and an Eina_Stat buffer.
 * @param error_cb Function to call on error. It receives the user data, the Eio_File handle, and the error code (errno).
 * @param data Custom data pointer to pass to callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on failure to queue.
 *
 * This function queues an operation to get metadata about a file, similar to
 * the stat() system call. The operation is performed in a separate thread.
 * Callbacks are invoked in the main loop.
 */
EIO_API Eio_File *
eio_file_direct_stat(const char *path,
		     Eio_Stat_Cb done_cb,
		     Eio_Error_Cb error_cb,
		     const void *data)
{
   Eio_File_Stat *s = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   s = eio_common_alloc(sizeof (Eio_File_Stat));
   if (!s) return NULL;

   s->path = eina_stringshare_add(path);
   s->done_cb = done_cb;

   if (!eio_file_set(&s->common,
		      NULL,
		      error_cb,
		      data,
		      _eio_file_stat,
		      _eio_file_stat_done,
		      _eio_file_stat_error))
     /* THERE IS NO MEMLEAK HERE, ECORE_THREAD CANCEL CALLBACK HAS BEEN ALREADY CALLED
	AND s HAS BEEN FREED, SAME FOR ALL CALL TO EIO_FILE_SET ! */
     return NULL;

   return &s->common;
}

/**
 * @brief Asynchronously get file status information (like lstat(2)).
 * @param path The path to the file, directory, or symbolic link.
 * @param done_cb Function to call on success. It receives the user data, the Eio_File handle, and an Eina_Stat buffer.
 * @param error_cb Function to call on error. It receives the user data, the Eio_File handle, and the error code (errno).
 * @param data Custom data pointer to pass to callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on failure to queue.
 *
 * This function queues an operation to get metadata about a file, similar to
 * the lstat() system call. If path is a symbolic link, it stats the link
 * itself, not the file it points to. The operation is performed in a
 * separate thread. Callbacks are invoked in the main loop.
 */
EIO_API Eio_File *
eio_file_direct_lstat(const char *path,
		      Eio_Stat_Cb done_cb,
		      Eio_Error_Cb error_cb,
		      const void *data)
{
   Eio_File_Stat *s = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   s = eio_common_alloc(sizeof (Eio_File_Stat));
   if (!s) return NULL;

   s->path = eina_stringshare_add(path);
   s->done_cb = done_cb;

   if (!eio_file_set(&s->common,
		      NULL,
		      error_cb,
		      data,
		      _eio_file_lstat,
		      _eio_file_stat_done,
		      _eio_file_stat_error))
     return NULL;

   return &s->common;
}

/**
 * @brief Asynchronously delete a name and possibly the file it refers to (like unlink(2)).
 * @param path The path to the file to delete.
 * @param done_cb Function to call on success. It receives the user data and the Eio_File handle.
 * @param error_cb Function to call on error. It receives the user data, the Eio_File handle, and the error code (errno).
 * @param data Custom data pointer to pass to callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on failure to queue.
 *
 * This function queues an operation to delete a file. The operation is
 * performed in a separate thread. Callbacks are invoked in the main loop.
 */
EIO_API Eio_File *
eio_file_unlink(const char *path,
		Eio_Done_Cb done_cb,
		Eio_Error_Cb error_cb,
		const void *data)
{
   Eio_File_Unlink *l = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   l = eio_common_alloc(sizeof (Eio_File_Unlink));
   if (!l) return NULL;

   l->path = eina_stringshare_add(path);

   if (!eio_file_set(&l->common,
		      done_cb,
		      error_cb,
		      data,
		      _eio_file_unlink,
		      _eio_file_unlink_done,
		      _eio_file_unlink_error))
     return NULL;

   return &l->common;
}

/**
 * @brief Asynchronously create a directory (like mkdir(2)).
 * @param path The path of the directory to create.
 * @param mode The permissions for the new directory (e.g., 0755).
 * @param done_cb Function to call on success. It receives the user data and the Eio_File handle.
 * @param error_cb Function to call on error. It receives the user data, the Eio_File handle, and the error code (errno).
 * @param data Custom data pointer to pass to callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on failure to queue.
 *
 * This function queues an operation to create a new directory. The operation
 * is performed in a separate thread. Callbacks are invoked in the main loop.
 */
EIO_API Eio_File *
eio_file_mkdir(const char *path,
	       mode_t mode,
	       Eio_Done_Cb done_cb,
	       Eio_Error_Cb error_cb,
	       const void *data)
{
   Eio_File_Mkdir *r = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   r = eio_common_alloc(sizeof (Eio_File_Mkdir));
   if (!r) return NULL;

   r->path = eina_stringshare_add(path);
   r->mode = mode;

   if (!eio_file_set(&r->common,
		     done_cb,
		     error_cb,
		      data,
		     _eio_file_mkdir,
		     _eio_file_mkdir_done,
		     _eio_file_mkdir_error))
     return NULL;

   return &r->common;
}

/**
 * @brief Asynchronously change file permissions (like chmod(2)).
 * @param path The path to the file or directory.
 * @param mode The new permissions (e.g., 0644).
 * @param done_cb Function to call on success. It receives the user data and the Eio_File handle.
 * @param error_cb Function to call on error. It receives the user data, the Eio_File handle, and the error code (errno).
 * @param data Custom data pointer to pass to callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on failure to queue.
 *
 * This function queues an operation to change the permissions of a file or
 * directory. The operation is performed in a separate thread. Callbacks are
 * invoked in the main loop.
 * @note Internally, this function reuses Eio_File_Mkdir structure for Eio_File_Chmod,
 * as they share similar basic fields (path, mode, common). The done and error
 * callbacks are correctly routed to _eio_file_mkdir_done and _eio_file_mkdir_error
 * but the worker function is _eio_file_chmod. This is a bit of a misnomer in struct usage.
 */
EIO_API Eio_File *
eio_file_chmod(const char *path,
	       mode_t mode,
	       Eio_Done_Cb done_cb,
	       Eio_Error_Cb error_cb,
	       const void *data)
{
   Eio_File_Mkdir *r = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   r = eio_common_alloc(sizeof (Eio_File_Mkdir));
   if (!r) return NULL;

   r->path = eina_stringshare_add(path);
   r->mode = mode;

   if (!eio_file_set(&r->common,
		     done_cb,
		     error_cb,
		      data,
		     _eio_file_chmod,
		     _eio_file_mkdir_done,
		     _eio_file_mkdir_error))
     return NULL;

   return &r->common;
}

/**
 * @brief Asynchronously change file ownership (like chown(2)).
 * @param path The path to the file or directory.
 * @param user The new user name or UID as a string. If NULL, user is not changed.
 * @param group The new group name or GID as a string. If NULL, group is not changed.
 * @param done_cb Function to call on success. It receives the user data and the Eio_File handle.
 * @param error_cb Function to call on error. It receives the user data, the Eio_File handle, and the error code (errno).
 * @param data Custom data pointer to pass to callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on failure to queue or if chown is not available.
 *
 * This function queues an operation to change the owner and/or group of a file
 * or directory. The operation is performed in a separate thread. Callbacks are
 * invoked in the main loop.
 * If the system does not support chown or getpwent (for name resolution),
 * this function will immediately call the error_cb with EINVAL and return NULL.
 */
EIO_API Eio_File *
eio_file_chown(const char *path,
	       const char *user,
	       const char *group,
	       Eio_Done_Cb done_cb,
	       Eio_Error_Cb error_cb,
	       const void *data)
{
#if defined(HAVE_CHOWN) && defined(HAVE_GETPWENT)
   Eio_File_Chown *c = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(path, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   c = eio_common_alloc(sizeof (Eio_File_Chown));
   if (!c) return NULL;

   c->path = eina_stringshare_add(path);
   c->user = eina_stringshare_add(user);
   c->group = eina_stringshare_add(group);

   if (!eio_file_set(&c->common,
		     done_cb,
		     error_cb,
		     data,
		     _eio_file_chown,
		     _eio_file_chown_done,
		     _eio_file_chown_error))
     return NULL;

   return &c->common;
#else
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);
   error_cb((char *)data, NULL, EINVAL);
   return NULL;
   (void)path;
   (void)user;
   (void)group;
   (void)done_cb;
#endif
}
