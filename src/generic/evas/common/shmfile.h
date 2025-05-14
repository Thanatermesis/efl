#ifndef SHMFILE_H
#define SHMFILE_H 1

/**
 * @file shmfile.h
 * @brief API for managing shared memory segments.
 *
 * This header defines the interface for allocating and freeing shared memory.
 * It supports different mechanisms based on the operating system (Windows, POSIX shm_open, or fallback to malloc).
 */

#ifdef __cplusplus
extern "C" {
#endif

extern int   shm_fd;     /**< File descriptor or handle for the shared memory segment.
                              * On Windows, this is a HANDLE. On POSIX systems using shm_open, this is a file descriptor.
                              * It is -1 (or NULL on Windows) if no shared memory is currently mapped or if using malloc fallback. */
extern int   shm_size;   /**< Size of the allocated shared memory segment in bytes. */
extern void *shm_addr;   /**< Pointer to the beginning of the mapped shared memory segment.
                              * NULL if no shared memory is currently mapped. */
extern char *shmfile;  /**< Name of the shared memory object.
                              * This is used for shm_open on POSIX systems and CreateFileMapping on Windows.
                              * It is dynamically generated to be unique. NULL if not used or not allocated. */

/**
 * @brief Allocates a shared memory segment.
 *
 * This function attempts to create and map a shared memory segment.
 * The method used depends on the compilation flags:
 * 1. Windows: Uses CreateFileMapping and MapViewOfFile.
 * 2. POSIX with HAVE_SHM_OPEN: Uses shm_open and mmap.
 * 3. Fallback: Uses malloc (not actually shared memory, but a regular memory allocation).
 *
 * The shared memory name (`shmfile`) is generated dynamically to be unique,
 * incorporating the process ID and a random number.
 *
 * @param dsize The desired size of the shared memory segment in bytes.
 */
void shm_alloc  (unsigned long dsize);

/**
 * @brief Frees/unmaps the previously allocated shared memory segment.
 *
 * This function unmaps the shared memory segment and, if applicable,
 * closes the handle/file descriptor and removes the shared memory object.
 * If the memory was allocated via the malloc fallback, it simply frees the memory.
 */
void shm_free   (void);

#ifdef __cplusplus
}
#endif

#endif
