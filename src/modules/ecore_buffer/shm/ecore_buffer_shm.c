#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#include <fcntl.h>
#include <string.h>

#include <Eina.h>
#include "Ecore.h"
#include "ecore_private.h"
#include <Ecore_Buffer.h>

#ifndef O_BINARY
# define O_BINARY 0
#endif

typedef struct _Ecore_Buffer_Shm_Data Ecore_Buffer_Shm_Data;

/**
 * @brief Structure to hold shared memory buffer data.
 */
struct _Ecore_Buffer_Shm_Data {
     const char *file; /**< Path to the shared memory file. */
     void *addr;       /**< Mapped memory address. */
     int w;            /**< Width of the buffer in pixels. */
     int h;            /**< Height of the buffer in pixels. */
     int stride;       /**< Stride of the buffer in bytes. */
     int size;         /**< Total size of the mapped memory in bytes. */
     Eina_Bool am_owner : 1; /**< EINA_TRUE if this instance created the shared memory, EINA_FALSE otherwise. */
};

/**
 * @brief Frees a shared memory buffer.
 *
 * This function unmaps the shared memory and, if this instance is the owner,
 * unlinks the associated file.
 *
 * @param bmdata Unused module data.
 * @param bdata Pointer to the Ecore_Buffer_Shm_Data structure to free.
 */
static void
_ecore_buffer_shm_buffer_free(Ecore_Buffer_Module_Data bmdata EINA_UNUSED, Ecore_Buffer_Data bdata)
{
   Ecore_Buffer_Shm_Data* b = bdata;

   if (b->am_owner)
     if (b->file) unlink(b->file);

   if (b->addr != MAP_FAILED) munmap(b->addr, b->size);
   eina_stringshare_del(b->file);
   b->file = NULL;
   b->addr = MAP_FAILED;
   b->am_owner = EINA_FALSE;
   b->w = 0;
   b->h = 0;
   b->stride = 0;
   b->size = 0;
   free(b);
}

/**
 * @brief Allocates a new shared memory buffer.
 *
 * Creates a new shared memory segment, maps it, and returns a handle to it.
 * The shared memory file is created in a temporary location.
 *
 * @param bmdata Module data (unused in this implementation).
 * @param width The desired width of the buffer in pixels.
 * @param height The desired height of the buffer in pixels.
 * @param format The pixel format of the buffer (unused in this implementation, assumes 32bpp).
 * @param flags Allocation flags (unused in this implementation).
 * @return A pointer to Ecore_Buffer_Data (specifically Ecore_Buffer_Shm_Data) on success,
 *         NULL on failure.
 */
static Ecore_Buffer_Data
_ecore_buffer_shm_buffer_alloc(Ecore_Buffer_Module_Data bmdata, int width, int height, Ecore_Buffer_Format format EINA_UNUSED, unsigned int flags EINA_UNUSED)
{
   Ecore_Buffer_Shm_Data* b;
   char *name;
   int fd, page_size;

   page_size = eina_cpu_page_size();

   b = calloc(1, sizeof(Ecore_Buffer_Shm_Data));
   fd = -1;
   b->addr = MAP_FAILED;
   b->w = width;
   b->h = height;
   b->stride = width * sizeof(int);
   b->size = page_size * (((b->stride * b->h) + (page_size - 1)) / page_size);
   b->am_owner = EINA_TRUE;

   name = eina_vpath_resolve("(:usr.run:)/ecore-buffer-shared-XXXXXX");
   if (!name) goto err;

   fd = mkostemp(name, O_CLOEXEC);
   if (fd < 0) goto err_fd;
   b->file = eina_stringshare_add(name);
   free(name);

   if (ftruncate(fd, b->size) < 0) goto err;

   b->addr = mmap(NULL, b->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (b->addr == MAP_FAILED) goto err;
   close(fd);

   return b;
err:
   close(fd);
err_fd:
   _ecore_buffer_shm_buffer_free(bmdata, b);
   return NULL;
}

/**
 * @brief Exports a shared memory buffer.
 *
 * Opens the shared memory file associated with the buffer and returns its
 * file descriptor.
 *
 * @param bmdata Unused module data.
 * @param bdata Pointer to the Ecore_Buffer_Shm_Data structure.
 * @param id Pointer to an integer where the file descriptor will be stored.
 * @return EXPORT_TYPE_FD on success.
 */
static Ecore_Export_Type
_ecore_buffer_shm_buffer_export(Ecore_Buffer_Module_Data bmdata EINA_UNUSED, Ecore_Buffer_Data bdata, int *id)
{
   Ecore_Buffer_Shm_Data* b = bdata;
   int fd;

   fd = open(b->file, O_RDWR | O_BINARY | O_CLOEXEC);
   if (id) *id = fd;

   return EXPORT_TYPE_FD;
}

/**
 * @brief Imports a shared memory buffer from a file descriptor.
 *
 * Maps an existing shared memory segment, identified by a file descriptor,
 * into the process's address space.
 *
 * @param bmdata Unused module data.
 * @param w The width of the buffer in pixels.
 * @param h The height of the buffer in pixels.
 * @param format The pixel format of the buffer (unused, assumes 32bpp).
 * @param type The type of export identifier; must be EXPORT_TYPE_FD.
 * @param export_id The file descriptor of the shared memory segment.
 * @param flags Import flags (unused in this implementation).
 * @return A pointer to Ecore_Buffer_Data (specifically Ecore_Buffer_Shm_Data) on success,
 *         NULL on failure.
 */
static void *
_ecore_buffer_shm_buffer_import(Ecore_Buffer_Module_Data bmdata EINA_UNUSED, int w, int h, Ecore_Buffer_Format format EINA_UNUSED, Ecore_Export_Type type, int export_id, unsigned int flags EINA_UNUSED)
{
   Ecore_Buffer_Shm_Data* b;
   int fd, page_size;

   EINA_SAFETY_ON_FALSE_RETURN_VAL(type == EXPORT_TYPE_FD, NULL);

   b = calloc(1, sizeof(Ecore_Buffer_Shm_Data));
   if (!b) return NULL;

   page_size = eina_cpu_page_size();

   fd = export_id;
   b->w = w;
   b->h = h;
   b->stride = w * sizeof(int);
   b->size = page_size * (((b->stride * b->h) + (page_size - 1)) / page_size);
   b->am_owner = EINA_FALSE;

   b->addr = mmap(NULL, b->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   if (b->addr == MAP_FAILED) goto err;

   return b;
err:
   _ecore_buffer_shm_buffer_free(bmdata, b);
   return NULL;
}

/**
 * @brief Gets the direct pointer to the shared memory data.
 *
 * @param bmdata Unused module data.
 * @param bdata Pointer to the Ecore_Buffer_Shm_Data structure.
 * @return A void pointer to the mapped shared memory address.
 */
static void *
_ecore_buffer_shm_data_get(Ecore_Buffer_Module_Data bmdata EINA_UNUSED, Ecore_Buffer_Data bdata)
{
   Ecore_Buffer_Shm_Data *b = bdata;

   return b->addr;
}

static Ecore_Buffer_Backend _ecore_buffer_shm_backend = {
     "shm",
     NULL,
     NULL,
     &_ecore_buffer_shm_buffer_alloc,
     &_ecore_buffer_shm_buffer_free,
     &_ecore_buffer_shm_buffer_export,
     &_ecore_buffer_shm_buffer_import,
     &_ecore_buffer_shm_data_get,
     NULL,
     NULL,
};

/**
 * @brief Initializes the shared memory buffer module.
 *
 * Registers the SHM backend with Ecore_Buffer.
 *
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool shm_init(void)
{
   return ecore_buffer_register(&_ecore_buffer_shm_backend);
}

/**
 * @brief Shuts down the shared memory buffer module.
 *
 * Unregisters the SHM backend from Ecore_Buffer.
 */
void shm_shutdown(void)
{
   ecore_buffer_unregister(&_ecore_buffer_shm_backend);
}

EINA_MODULE_INIT(shm_init);
EINA_MODULE_SHUTDOWN(shm_shutdown);
