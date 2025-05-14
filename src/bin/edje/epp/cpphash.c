/* Part of CPP library.  (Macro hash table support.)
 * Copyright (C) 1986, 87, 89, 92, 93, 94, 1995 Free Software Foundation, Inc.
 * Written by Per Bothner, 1994.
 * Based on CCCP program by by Paul Rubin, June 1986
 * Adapted to ANSI C, Richard Stallman, Jan 1987
 * Copyright (C) 2003-2011 Kim Woelders
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * 
 * In other words, you are welcome to use, share and improve this program.
 * You are forbidden to forbid anyone else to use, share and improve
 * what you give them.   Help stamp out software-hoarding!  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eina.h>
#include <string.h>
#include <stdlib.h>

#include "cpplib.h"
#include "cpphash.h"

static HASHNODE    *hashtab[HASHSIZE];

#define IS_IDCHAR(ch) is_idchar[(unsigned char)(ch)]

/**
 * @brief Computes a hash value for a name.
 *
 * @details The hash function must be compatible with the one computed
 * incrementally elsewhere in the scanner (using the HASHSTEP macro).
 */
int
hashf(const char *name, int len, int hashsize)
{
   int                 r = 0;

   while (len--)
      r = HASHSTEP(r, *name++);

   return MAKE_POS(r) % hashsize;
}

/**
 * @brief Finds the most recent hash node for a name.
 *
 * @details The name is considered to end at the first non-identifier
 * character. This finds nodes installed by install().
 *
 * If @p len is non-negative, it is used as the length of the name.
 * Otherwise, the length is computed by scanning the name.
 *
 * If @p hash is non-negative, it is used as the precomputed hash code.
 * Otherwise, the hash code is computed.
 */
HASHNODE           *
cpp_lookup(const char *name, int len, int hash)
{
   const char         *bp;
   HASHNODE           *bucket;

   if (len < 0)
     {
	for (bp = name; IS_IDCHAR(*bp); bp++)
	   ;
	len = bp - name;
     }
   if (hash < 0)
      hash = hashf(name, len, HASHSIZE);

   bucket = hashtab[hash];
   while (bucket)
     {
	if (bucket->length == len
	    && strncmp((const char *)bucket->name, name, len) == 0)
	   return bucket;
	bucket = bucket->next;
     }
   return (HASHNODE *) 0;
}

/**
 * @brief Deletes a hash node, with special handling for macros.
 *
 * @details When deleting a macro, its DEFINITION struct is not freed. This is
 * a deliberate choice to prevent a crash if a macro is undefined (`#undef`)
 * while it is being expanded. While this may result in a memory leak, it is
 * necessary for stability.
 */
void
delete_macro(HASHNODE * hp)
{

   if (hp->prev)
      hp->prev->next = hp->next;
   if (hp->next)
      hp->next->prev = hp->prev;

   /* make sure that the bucket chain header that
    * the deleted guy was on points to the right thing afterwards. */
   if (hp == *hp->bucket_hdr)
      *hp->bucket_hdr = hp->next;

   if (hp->type == T_MACRO)
     {
	DEFINITION         *d = hp->value.defn;
	struct reflist     *ap, *nextap;

	for (ap = d->pattern; ap; ap = nextap)
	  {
	     nextap = ap->next;
	     free(ap);
	  }
	if (d->nargs >= 0)
	   free(d->args.argnames);
	free(d);
     }
   free(hp);
}
/**
 * @brief Installs a name in the main hash table.
 *
 * @details A new entry is created even if one with the same name already
 * exists. The name is considered to end at the first non-alphanumeric
 * character. It is the caller's responsibility to check for redefinitions if
 * necessary.
 *
 * Nodes are inserted at the head of the hash bucket's linked list, so they are
 * found first by lookup. This shadowing is important for handling macro
 * redefinitions and the `defined` operator.
 *
 * If @p len is non-negative, it is the length of the name; otherwise, the
 * length is computed.
 *
 * If @p hash is non-negative, it is the precomputed hash code; otherwise, the
 * hash code is computed.
 */
HASHNODE           *
install(const char *name, int len, enum node_type type, int ivalue, char *value,
	int hash)
{
   HASHNODE           *hp;
   int                 i, bucket;
   const char         *p;

   if (len < 0)
     {
	p = name;
	while (IS_IDCHAR(*p))
	   p++;
	len = p - name;
     }
   if (hash < 0)
      hash = hashf(name, len, HASHSIZE);

   i = sizeof(HASHNODE) + len + 1;
   hp = (HASHNODE *) xmalloc(i);
   bucket = hash;
   hp->bucket_hdr = &hashtab[bucket];
   hp->next = hashtab[bucket];
   hashtab[bucket] = hp;
   hp->prev = NULL;
   if (hp->next)
      hp->next->prev = hp;
   hp->type = type;
   hp->length = len;
   if (hp->type == T_CONST)
      hp->value.ival = ivalue;
   else
      hp->value.cpval = value;
   hp->name = ((char *)hp) + sizeof(HASHNODE);
   memcpy(hp->name, name, len);
   hp->name[len] = 0;
   return hp;
}

/**
 * @brief Frees memory used by the hash table.
 *
 * @details This function iterates through all hash table buckets and frees the
 * head node of each chain via delete_macro().
 *
 * @note Since only the head of each chain is deleted, this may result in
 * memory leaks if chains contain more than one node.
 */
void
cpp_hash_cleanup(cpp_reader * pfile EINA_UNUSED)
{
   int                 i;

   for (i = HASHSIZE; --i >= 0;)
     {
        if (hashtab[i])
          delete_macro(hashtab[i]);
     }
}
