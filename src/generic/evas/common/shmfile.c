#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/**
 * @file shmfile.c
 * @brief Implementation of shared memory allocation and management.
 *
 * This file provides functions to allocate and free shared memory segments
 * using platform-specific mechanisms (Windows API, POSIX shm_open) or
 * a fallback to standard malloc if shared memory is not available/supported.
 * The primary use case appears to be for an Evas loader, potentially to
 * share image data or other resources.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <zlib.h>

#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <windows.h>
# undef WIN32_LEAN_AND_MEAN
#else
# include <sys/mman.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
/** @brief Handle to the shared memory mapping object (Windows). NULL if not used. */
HANDLE shm_fd = NULL;
#else
/** @brief File descriptor for the shared memory object (POSIX). -1 if not used. */
int shm_fd = -1;
#endif
/** @brief Size of the allocated shared memory segment in bytes. */
static int shm_size = 0;
/** @brief Pointer to the mapped shared memory address. NULL if not mapped. */
void *shm_addr = NULL;
/** @brief Name of the shared memory object. Dynamically generated. NULL if not used. */
char *shmfile = NULL;

/**
 * @brief Allocates or maps a shared memory segment.
 *
 * Attempts to create a shared memory segment using platform-specific methods.
 * On Windows, it uses `CreateFileMapping` and `MapViewOfFile`.
 * On POSIX systems with `HAVE_SHM_OPEN`, it uses `shm_open` and `mmap`.
 * If these methods are unavailable or fail, it falls back to `malloc`.
 * The shared memory object name is generated to be unique using the process ID
 * and a random number to avoid collisions.
 *
 * @param dsize The size in bytes of the memory segment to allocate.
 */
void
shm_alloc(unsigned long dsize)
{
#ifdef _WIN32
   if (!shmfile) shmfile = malloc(1024);
   if (!shmfile) goto failed;
   shmfile[0] = 0;
   srand(time(NULL));
   do
     {
        snprintf(shmfile, 1024, "/evas-loader.%i.%i",
                 (int)getpid(), (int)rand());
        shm_fd = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                   0, dsize, shmfile);
     }
   while ((shm_fd == NULL) || (GetLastError() == ERROR_ALREADY_EXISTS));

   shm_addr = MapViewOfFile(shm_fd, FILE_MAP_WRITE, 0, 0, dsize);
   if (!shm_addr)
     {
        free(shmfile);
        CloseHandle(shm_fd);
        goto failed;
     }
   shm_size = dsize;
   return;
failed:
#elif HAVE_SHM_OPEN
   if (!shmfile) shmfile = malloc(1024);
   if (!shmfile) goto failed;
   shmfile[0] = 0;
   srand(time(NULL));
   do
     {
        snprintf(shmfile, 1024, "/evas-loader.%i.%i",
                 (int)getpid(), (int)rand());
        shm_fd = shm_open(shmfile, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
     }
   while (shm_fd < 0);

   if (ftruncate(shm_fd, dsize) < 0)
     {
        close(shm_fd);
        shm_unlink(shmfile);
        shm_fd = -1;
	goto failed;
     }
   shm_addr = mmap(NULL, dsize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
   if (shm_addr == MAP_FAILED)
     {
        close(shm_fd);
        shm_unlink(shmfile);
        shm_fd = -1;
        goto failed;
     }
   shm_size = dsize;
   return;
failed:
#endif
   // Fallback: if shared memory mechanisms are not available or fail, use malloc.
   // This is not true shared memory but provides a memory buffer.
   shm_addr = malloc(dsize);
}

/**
 * @brief Frees and unmaps the shared memory segment.
 *
 * Releases the resources associated with the shared memory segment.
 * On Windows, it unmaps the view, closes the handle.
 * On POSIX systems with `HAVE_SHM_OPEN`, it unmaps the memory, closes the
 * file descriptor, and unlinks the shared memory object.
 * If memory was allocated via the `malloc` fallback, it calls `free`.
 * Resets global shared memory state variables.
 */
void
shm_free(void)
{
#ifdef _WIN32
   if (shm_fd) // Check if a valid shared memory handle exists
     {
        UnmapViewOfFile(shm_addr);
        CloseHandle(shm_fd);
        free(shmfile);
        shm_addr = NULL;
        shm_fd = NULL;
        shmfile = NULL;
        return;
     }
#elif HAVE_SHM_OPEN
   if (shm_fd >= 0)
     {
        munmap(shm_addr, shm_size);
        close(shm_fd);
        shm_fd = -1;
        shm_addr = NULL;
        if (shmfile) free(shmfile);
        shmfile = NULL;
        return;
     }
#endif
   // Fallback: if shm_addr was allocated by malloc (e.g., shm_alloc failed or no SHM support)
   free(shm_addr);
   shm_addr = NULL;
#ifdef _WIN32
   shm_fd = NULL;
#else
   shm_fd = -1;
#endif
}

#ifdef __cplusplus
}
#endif
