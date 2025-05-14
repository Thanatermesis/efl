#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <string.h>
#include <errno.h>

#include "edje_cc.h"

/**
 * @brief Allocates a block of memory and initializes it to zero.
 *
 * This function is a wrapper around calloc. If the allocation fails,
 * it prints an error message to stderr and exits the program.
 *
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL on failure (though
 *         the program will exit before returning NULL in practice).
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
 * @brief Duplicates a string.
 *
 * This function is a wrapper around strdup. If the allocation fails,
 * it prints an error message to stderr and exits the program.
 *
 * @param s The null-terminated string to duplicate.
 * @return A pointer to the newly allocated string, or NULL on failure (though
 *         the program will exit before returning NULL in practice).
 */
char *
mem_strdup(const char *s)
{
   void *str;

   str = strdup(s);
   if (str) return str;
   ERR("%s:%i memory allocation of %zu bytes failed. %s. string being duplicated: \"%s\"",
       file_in, line, strlen(s) + 1, strerror(errno), s);
   exit(-1);
   return NULL;
}

