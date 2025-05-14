#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <string.h>
#include <errno.h>

#include "elm_prefs_cc.h"

/**
 * @brief Allocates memory and exits on failure.
 *
 * This function is a wrapper around calloc(1, size) that provides
 * error handling. If memory allocation fails, an error message
 * is printed to standard error, and the program exits with a status of -1.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL if allocation fails (though
 *         the program will exit before NULL can be returned in case of error).
 *         The memory is initialized to zero.
 */
void *
mem_alloc(size_t size)
{
   void *mem;

   mem = calloc(1, size);
   if (mem) return mem;
   ERR("%s:%i memory allocation of %zu bytes failed. %s",
       file_in, line, size, strerror(errno));
   exit(-1);
   return NULL;
}

/**
 * @brief Duplicates a string and exits on failure.
 *
 * This function is a wrapper around strdup(s) that provides
 * error handling. If string duplication fails (due to memory allocation
 * failure), an error message is printed to standard error, and the
 * program exits with a status of -1.
 *
 * @param s The null-terminated string to duplicate.
 * @return A pointer to the newly allocated string, which is a duplicate of s.
 *         Returns NULL if duplication fails (though the program will exit
 *         before NULL can be returned in case of error).
 */
char *
mem_strdup(const char *s)
{
   void *str;

   str = strdup(s);
   if (str) return str;
   ERR("%s:%i memory allocation of %zu bytes failed. %s. string "
       "being duplicated: \"%s\"",
       file_in, line, strlen(s) + 1, strerror(errno), s);
   exit(-1);
   return NULL;
}
