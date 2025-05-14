#include "ecore_evas_extn_engine.h"

#ifndef O_BINARY
# define O_BINARY 0
#endif

/**
 * @brief Structure to manage an external shared memory buffer.
 *
 * This structure holds information about a shared memory buffer,
 * including its file path, lock file path, memory address,
 * file descriptors, dimensions, and ownership status.
 */
struct _Extnbuf
{
   const char *file, *lock; /**< File path and lock file path for the shared memory. */
   void *addr; /**< Memory-mapped address of the shared buffer. */
   int fd, lockfd; /**< File descriptors for the shared memory and lock file. */
   int w, h, stride, size; /**< Width, height, stride (bytes per row), and total size of the buffer. */
   Eina_Bool have_lock : 1; /**< Flag indicating if the buffer is currently locked. */
   Eina_Bool am_owner : 1; /**< Flag indicating if this instance is the owner (creator) of the buffer. */
};

/**
 * @brief Creates or opens an external shared memory buffer.
 * @param base The base name for the shared memory file.
 * @param id An identifier, typically the display number.
 * @param sys EINA_TRUE if the buffer should be system-wide accessible, EINA_FALSE otherwise.
 * @param num A unique number for this buffer.
 * @param w The width of the buffer in pixels.
 * @param h The height of the buffer in pixels.
 * @param owner EINA_TRUE if this instance should create and own the buffer,
 *              EINA_FALSE to open an existing one.
 * @return A pointer to the Extnbuf structure on success, or NULL on failure.
 *
 * If 'owner' is EINA_TRUE, this function creates a new shared memory segment
 * and a lock file. The shared memory file will be named "/<base>-<id>.<num>".
 * If 'owner' is EINA_FALSE, it attempts to open an existing shared memory segment
 * with the same naming convention.
 * The buffer size is calculated based on w, h, and a 4-byte-per-pixel format,
 * rounded up to the nearest page size.
 */
Extnbuf *
_extnbuf_new(const char *base, int id, Eina_Bool sys, int num,
             int w, int h, Eina_Bool owner)
{
   Extnbuf *b;
   char file[PATH_MAX];
   mode_t mode = S_IRUSR;
   int prot = PROT_READ;
   int page_size;
   Eina_Tmpstr *tmp = NULL;

   page_size = eina_cpu_page_size();

   b = calloc(1, sizeof(Extnbuf));
   b->fd = -1;
   b->lockfd = -1;
   b->addr = MAP_FAILED;
   b->w = w;
   b->h = h;
   b->stride = w * 4;
   b->size = page_size * (((b->stride * b->h) + (page_size - 1)) / page_size);
   b->am_owner = owner;

   snprintf(file, sizeof(file), "/%s-%i.%i", base, id, num);
   b->file = eina_stringshare_add(file);
   if (!b->file) goto err;


   if (sys) mode |= S_IRGRP | S_IROTH;

   if (owner)
     {
        mode |= S_IWUSR;
        prot |= PROT_WRITE;
     }

   if (b->am_owner)
     {
        b->lockfd = eina_file_mkstemp("ee-lock-XXXXXX", &tmp);
        if (b->lockfd < 0) goto err;
        b->lock = eina_stringshare_add(tmp);
        if (!b->lock) goto err;
        b->fd = shm_open(b->file, O_RDWR | O_CREAT | O_EXCL, mode);
        if (b->fd < 0) goto err;
        if (ftruncate(b->fd, b->size) < 0) goto err;
     }
   else
     {
        b->fd = shm_open(b->file, O_RDONLY, mode);
        if (b->fd < 0) goto err;
     }
   b->addr = mmap(NULL, b->size, prot, MAP_SHARED, b->fd, 0);
   if (b->addr == MAP_FAILED) goto err;
   eina_tmpstr_del(tmp);
   return b;
err:
   eina_tmpstr_del(tmp);
   _extnbuf_free(b);
   return NULL;
}

/**
 * @brief Frees an external shared memory buffer.
 * @param b The Extnbuf structure to free.
 *
 * This function unmaps the shared memory, closes file descriptors,
 * and if this instance is the owner, unlinks the shared memory file
 * and the lock file. It also releases any held locks.
 */
void
_extnbuf_free(Extnbuf *b)
{
   if (b->have_lock) _extnbuf_unlock(b);

   if (b->am_owner)
     {
        if (b->file) shm_unlink(b->file);
        if (b->lock) unlink(b->lock);
     }

   if (b->addr != MAP_FAILED) munmap(b->addr, b->size);
   if (b->fd >= 0) close(b->fd);
   if (b->lockfd >= 0) close(b->lockfd);
   eina_stringshare_del(b->file);
   eina_stringshare_del(b->lock);
   b->file = NULL;
   b->lock = NULL;
   b->addr = MAP_FAILED;
   b->fd = 1;
   b->lockfd = 1;
   b->am_owner = EINA_FALSE;
   b->have_lock = EINA_FALSE;
   b->w = 0;
   b->h = 0;
   b->stride = 0;
   b->size = 0;
   free(b);
}

/**
 * @brief Gets the data pointer and dimensions of the buffer.
 * @param b The Extnbuf structure.
 * @param[out] w Pointer to store the width of the buffer (optional).
 * @param[out] h Pointer to store the height of the buffer (optional).
 * @param[out] stride Pointer to store the stride of the buffer (optional).
 * @return A pointer to the raw buffer data, or NULL if b is NULL.
 *
 * This function does not perform any locking; it simply returns the current
 * memory address and dimensions.
 */
void *
_extnbuf_data_get(Extnbuf *b, int *w, int *h, int *stride)
{
   if (!b) return NULL;
   if (w) *w = b->w;
   if (h) *h = b->h;
   if (stride) *stride = b->stride;
   return b->addr;
}

/**
 * @brief Locks the buffer and returns its data pointer and dimensions.
 * @param b The Extnbuf structure.
 * @param[out] w Pointer to store the width of the buffer (optional).
 * @param[out] h Pointer to store the height of the buffer (optional).
 * @param[out] stride Pointer to store the stride of the buffer (optional).
 * @return A pointer to the raw buffer data on successful lock, or NULL on failure or if b is NULL.
 *
 * If the buffer is not already locked, this function attempts to acquire a
 * file lock (F_WRLCK for owner, F_RDLCK for client) on the associated lock file.
 * If successful, it marks the buffer as locked and returns its data.
 */
void *
_extnbuf_lock(Extnbuf *b, int *w, int *h, int *stride)
{
   if (!b) return NULL;
   if (!b->have_lock)
     {
        if (b->lockfd >= 0)
          {
             struct flock filelock;

             filelock.l_type = b->am_owner ? F_WRLCK : F_RDLCK;
             filelock.l_whence = SEEK_SET;
             filelock.l_start = 0;
             filelock.l_len = 0;
             if (fcntl(b->lockfd, F_SETLK, &filelock) == -1)
               {
                  ERR("lock take fail");
                  return NULL;
               }
          }
        b->have_lock = EINA_TRUE;
     }
   return _extnbuf_data_get(b, w, h, stride);
}

/**
 * @brief Unlocks the buffer.
 * @param b The Extnbuf structure.
 *
 * If the buffer is currently locked and has a valid lock file descriptor,
 * this function releases the file lock (F_UNLCK) and marks the buffer as unlocked.
 */
void
_extnbuf_unlock(Extnbuf *b)
{
   if (!b || !b->have_lock) return;
   if (b->lockfd >= 0)
     {
        struct flock filelock;

        filelock.l_type = F_UNLCK;
        filelock.l_whence = SEEK_SET;
        filelock.l_start = 0;
        filelock.l_len = 0;
        if (fcntl(b->lockfd, F_SETLKW, &filelock) == -1)
          {
             ERR("lock release fail");
             return;
          }
     }
   b->have_lock = EINA_FALSE;
}

/**
 * @brief Gets the path of the lock file.
 * @param b The Extnbuf structure.
 * @return The path to the lock file, or NULL if not set.
 */
const char *
_extnbuf_lock_file_get(const Extnbuf *b)
{
   if (!b) return NULL;
   return b->lock;
}

/**
 * @brief Sets the lock file path for a client buffer.
 * @param b The Extnbuf structure.
 * @param file The path to the lock file.
 * @return EINA_TRUE on success, EINA_FALSE on failure or if 'b' is an owner.
 *
 * This function is intended for client-side buffers (not owners).
 * It updates the lock file path and attempts to open it.
 * If 'file' is NULL, it clears the current lock file information.
 */
Eina_Bool
_extnbuf_lock_file_set(Extnbuf *b, const char *file)
{
   if (!b) return EINA_FALSE;
   if (b->am_owner) return EINA_FALSE;
   if (b->lock) eina_stringshare_del(b->lock);
   if (b->lockfd >= 0) close(b->lockfd);
   b->lockfd = -1;
   if (!file)
     {
        b->lock = NULL;
        b->lockfd = -1;
        return EINA_TRUE;
     }
   b->lock = eina_stringshare_add(file);
   if (!b->lock) goto err;
   b->lockfd = open(b->lock, O_RDWR | O_BINARY);
   if (b->lockfd >= 0) return EINA_TRUE;
err:
   if (b->lock) eina_stringshare_del(b->lock);
   if (b->lockfd >= 0) close(b->lockfd);
   b->lockfd = -1;
   b->lock = NULL;
   return EINA_FALSE;
}

/**
 * @brief Gets the current lock status of the buffer.
 * @param b The Extnbuf structure.
 * @return EINA_TRUE if the buffer is locked, EINA_FALSE otherwise or if b is NULL.
 */
Eina_Bool
_extnbuf_lock_get(const Extnbuf *b)
{
   if (!b) return EINA_FALSE;
   return b->have_lock;
}
