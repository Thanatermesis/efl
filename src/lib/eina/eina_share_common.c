/* EINA - EFL data type library
 * Copyright (C) 2002,2003,2004,2005,2006,2007,2008,2010
 *                         Carsten Haitzler,
 *                         Jorge Luis Zapata Muga,
 *                         Cedric Bail,
 *                         Gustavo Sverzut Barbieri
 *                         Tom Hacohen
 *                         Brett Nash
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 2008 Peter Wehrfritz
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies of the Software and its Copyright notices. In addition publicly
 *  documented acknowledgment must be given that this software has been used if no
 *  source code of this software is made available publicly. This includes
 *  acknowledgments in either Copyright notices, Manuals, Publicity and Marketing
 *  documents or any documentation provided with any product containing this
 *  software. This License does not apply to any software that links to the
 *  libraries provided by this software (statically or dynamically), but only to
 *  the software provided.
 *
 *  Please see the OLD-COPYING.PLAIN for a plain-english explanation of this notice
 *  and it's intent.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 *  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "eina_config.h"
#include "eina_private.h"
#include "eina_hash.h"
#include "eina_rbtree.h"
#include "eina_lock.h"

/* undefs EINA_ARG_NONULL() so NULL checks are not compiled out! */
#include "eina_safety_checks.h"
#include "eina_share_common.h"

/*============================================================================*
*                                  Local                                     *
*============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @internal
 * @brief The global Eina stringshare log domain.
 * Used for logging messages specific to the string sharing mechanism.
 */
extern int _eina_share_stringshare_log_dom;

#ifdef DBG_STRINGSHARE
#undef DBG_STRINGSHARE
#endif
#define DBG_STRINGSHARE(...) EINA_LOG_DOM_DBG(_eina_share_stringshare_log_dom, __VA_ARGS__)

#define EINA_SHARE_COMMON_BUCKETS 256 /**< Number of primary hash buckets. */
#define EINA_SHARE_COMMON_MASK 0xFF /**< Mask to extract parts of the hash. */
/**< Macro to determine the primary bucket index from a full hash. Uses upper bits. */
#define EINA_SHARE_COMMON_BUCKET_IDX(h) ((h >> 8) & EINA_SHARE_COMMON_MASK)
/**< Macro to determine the secondary hash for node collision resolution within a bucket. Uses lower bits. */
#define EINA_SHARE_COMMON_NODE_HASH(h) (h & EINA_SHARE_COMMON_MASK)

static const char EINA_MAGIC_SHARE_STR[] = "Eina Share"; /**< Magic string for Eina_Share_Common structure. */
static const char EINA_MAGIC_SHARE_HEAD_STR[] = "Eina Share Head"; /**< Magic string for Eina_Share_Common_Head structure. */

/**< Global counter for active eina_share_common instances. Used for global init/shutdown of shared resources like mutexes. */
static int _eina_share_common_count = 0;

/**
 * @internal
 * @brief Macro to check the magic number of an Eina_Share_Common_Head structure.
 * @param d Pointer to the Eina_Share_Common_Head structure.
 * @param unlock Unlock statement to execute before returning on failure.
 * @param ... Return value(s) on failure.
 */
#define EINA_MAGIC_CHECK_SHARE_COMMON_HEAD(d, unlock, ...)      \
   do {                                                          \
        if (!EINA_MAGIC_CHECK((d), EINA_MAGIC_SHARE_HEAD))  \
          {                                                           \
             EINA_MAGIC_FAIL((d), EINA_MAGIC_SHARE_HEAD);    \
             unlock;                                                 \
             return __VA_ARGS__;                                     \
          }                                                           \
     } while (0)

/**
 * @internal
 * @brief Macro to check the magic number of an Eina_Share_Common_Node structure.
 * @param d Pointer to the Eina_Share_Common_Node structure.
 * @param _node_magic Expected magic number for the node.
 * @param unlock Unlock statement to execute before returning on failure.
 */
#define EINA_MAGIC_CHECK_SHARE_COMMON_NODE(d, _node_magic, unlock)              \
   do {                                                          \
        if (!EINA_MAGIC_CHECK((d), _node_magic))    \
          {                                                           \
             EINA_MAGIC_FAIL((d), _node_magic);        \
             unlock;                                                   \
          }                                                           \
     } while (0)

#ifdef EINA_STRINGSHARE_USAGE
/**
 * @internal
 * @struct _Eina_Share_Common_Population
 * @brief Structure to track population statistics for string sharing.
 * Used when EINA_STRINGSHARE_USAGE is defined.
 */
typedef struct _Eina_Share_Common_Population Eina_Share_Common_Population;
struct _Eina_Share_Common_Population
{
   int count; /**< Current number of items. */
   int max;   /**< Maximum number of items observed. */
};
#endif

/**
 * @internal
 * @struct _Eina_Share_Common
 * @brief Core data structure for managing shared strings.
 * Contains an array of buckets, each potentially pointing to a red-black tree
 * of Eina_Share_Common_Head structures.
 */
typedef struct _Eina_Share_Common Eina_Share_Common;
/**
 * @internal
 * @struct _Eina_Share_Common_Node
 * @brief Represents a single shared string instance.
 * Nodes with the same hash (but different string content) are chained in a linked list
 * within an Eina_Share_Common_Head.
 */
typedef struct _Eina_Share_Common_Node Eina_Share_Common_Node;
/**
 * @internal
 * @struct _Eina_Share_Common_Head
 * @brief Represents a collection of shared strings that have the same
 * EINA_SHARE_COMMON_NODE_HASH value.
 * These heads are organized in a red-black tree within each primary bucket.
 * Each head contains a linked list of actual string nodes.
 * The first node is often embedded (builtin_node) for optimization.
 */
typedef struct _Eina_Share_Common_Head Eina_Share_Common_Head;

/**
 * @internal
 * @struct _Eina_Share
 * @brief Public-facing handle for a shared string manager instance.
 * This wraps the internal _Eina_Share_Common structure and holds
 * per-instance configuration like the node magic number and usage statistics.
 */
struct _Eina_Share
{
   Eina_Share_Common *share; /**< Pointer to the internal shared data. */
   Eina_Magic node_magic;    /**< Magic number for individual string nodes (_Eina_Share_Common_Node). */
#ifdef EINA_STRINGSHARE_USAGE
   Eina_Share_Common_Population population; /**< Overall population statistics. */
   Eina_Share_Common_Population population_group[4]; /**< Population statistics grouped by string length (0, 1, 2, 3). */
   int max_node_population; /**< Maximum number of strings sharing a single _Eina_Share_Common_Head. */
#endif
};

/**
 * @internal
 * @struct _Eina_Share_Common
 * @brief Definition of the core shared string data structure.
 */
struct _Eina_Share_Common
{
   Eina_Share_Common_Head *buckets[EINA_SHARE_COMMON_BUCKETS]; /**< Array of hash buckets. Each bucket is the root of an Rbtree of _Eina_Share_Common_Head. */

   EINA_MAGIC /**< Magic number for _Eina_Share_Common itself. */
};

/**
 * @internal
 * @struct _Eina_Share_Common_Node
 * @brief Definition of a shared string node.
 */
struct _Eina_Share_Common_Node
{
   Eina_Share_Common_Node *next; /**< Pointer to the next node in the collision chain (if any). */

   EINA_MAGIC /**< Magic number for this node. */

   unsigned int length;     /**< Length of the string (excluding null terminator). */
   unsigned int references; /**< Reference count for this string. */
   char str[];              /**< Flexible array member for the string data. Null termination is handled separately. */
};

/**
 * @internal
 * @struct _Eina_Share_Common_Head
 * @brief Definition of a hash collision group head.
 */
struct _Eina_Share_Common_Head
{
   EINA_RBTREE; /**< Makes this struct usable as an Eina_Rbtree node. */
   EINA_MAGIC   /**< Magic number for this head structure. */

   int hash;    /**< The full hash value for strings managed by this head. */

#ifdef EINA_STRINGSHARE_USAGE
   int population; /**< Number of unique strings currently managed by this head. */
#endif

   Eina_Share_Common_Node *head;         /**< Pointer to the first string node in the collision list. */
   Eina_Share_Common_Node builtin_node; /**< Embedded node for the first string added to this head, to save an allocation. */
};

/**
 * @internal
 * @brief Flag indicating whether threading support (mutexes) has been activated.
 */
Eina_Bool _share_common_threads_activated = EINA_FALSE;

/**
 * @internal
 * @brief A global spinlock protecting access to the shared string tables.
 * This lock is used for all modifications to the shared data structures.
 */
static Eina_Spinlock _mutex_big;

#ifdef EINA_STRINGSHARE_USAGE

/**
 * @internal
 * @brief Initializes population statistics for a shared string manager.
 * Only active if EINA_STRINGSHARE_USAGE is defined.
 * @param share The Eina_Share instance to initialize.
 */
static void
_eina_share_common_population_init(Eina_Share *share)
{
   unsigned int i;

   for (i = 0;
        i < sizeof (share->population_group) /
        sizeof (share->population_group[0]);
        ++i)
     {
        share->population_group[i].count = 0;
        share->population_group[i].max = 0;
     }
}

/**
 * @internal
 * @brief Resets population statistics for a shared string manager during shutdown.
 * Only active if EINA_STRINGSHARE_USAGE is defined.
 * @param share The Eina_Share instance.
 */
static void
_eina_share_common_population_shutdown(Eina_Share *share)
{
   unsigned int i;

   share->max_node_population = 0;
   share->population.count = 0;
   share->population.max = 0;

   for (i = 0;
        i < sizeof (share->population_group) /
        sizeof (share->population_group[0]);
        ++i)
     {
        share->population_group[i].count = 0;
        share->population_group[i].max = 0;
     }
}

/**
 * @internal
 * @brief Prints population statistics for a shared string manager.
 * Only active if EINA_STRINGSHARE_USAGE is defined.
 * @param share The Eina_Share instance whose statistics are to be printed.
 */
static void
_eina_share_common_population_stats(Eina_Share *share)
{
   unsigned int i;

      DBG_STRINGSHARE("eina share_common statistic:");
      DBG_STRINGSHARE(" * maximum shared strings : %i",
                      share->population.max);
      DBG_STRINGSHARE(" * maximum shared strings per node : %i",
                      share->max_node_population);

   for (i = 0;
        i < sizeof (share->population_group) /
        sizeof (share->population_group[0]);
        ++i)
      DBG_STRINGSHARE("DDD: %i strings of length %u, max strings: %i",
                      share->population_group[i].count, i,
                      share->population_group[i].max);
}

/**
 * @internal
 * @brief Adds to population statistics without taking the global lock.
 * Assumes the caller holds the lock. Only active if EINA_STRINGSHARE_USAGE is defined.
 * @param share The Eina_Share instance.
 * @param slen The length of the string being added.
 */
static void
eina_share_common_population_nolock_add(Eina_Share *share, int slen)
{
   share->population.count++;
   if (share->population.count > share->population.max)
      share->population.max = share->population.count;

   if (slen < 4)
     {
        share->population_group[slen].count++;
        if (share->population_group[slen].count >
            share->population_group[slen].max)
           share->population_group[slen].max =
              share->population_group[slen].count;
     }
}

void
eina_share_common_population_add(Eina_Share *share, int slen)
{
   eina_spinlock_take(&_mutex_big);
   eina_share_common_population_nolock_add(share, slen);
   eina_spinlock_release(&_mutex_big);
}

/**
 * @internal
 * @brief Subtracts from population statistics without taking the global lock.
 * Assumes the caller holds the lock. Only active if EINA_STRINGSHARE_USAGE is defined.
 * @param share The Eina_Share instance.
 * @param slen The length of the string being deleted.
 */
static void
eina_share_common_population_nolock_del(Eina_Share *share, int slen)
{
   share->population.count--;
   if (slen < 4)
      share->population_group[slen].count--;
}

/**
 * @brief Public function to decrement population statistics (lock-protected).
 * Only active if EINA_STRINGSHARE_USAGE is defined.
 * @param share The Eina_Share instance.
 * @param slen The length of the string being deleted.
 */
void
eina_share_common_population_del(Eina_Share *share, int slen)
{
   eina_spinlock_take(&_mutex_big);
   eina_share_common_population_nolock_del(share, slen);
   eina_spinlock_release(&_mutex_big);
}

/**
 * @internal
 * @brief Initializes population count for a new Eina_Share_Common_Head.
 * Only active if EINA_STRINGSHARE_USAGE is defined.
 * @param share The Eina_Share instance (unused if EINA_STRINGSHARE_USAGE is off).
 * @param head The Eina_Share_Common_Head being initialized.
 */
static void
_eina_share_common_population_head_init(EINA_UNUSED Eina_Share *share,
                                        Eina_Share_Common_Head *head)
{
   head->population = 1;
}

/**
 * @internal
 * @brief Increments population count for an Eina_Share_Common_Head.
 * Only active if EINA_STRINGSHARE_USAGE is defined.
 * @param share The Eina_Share instance.
 * @param head The Eina_Share_Common_Head to which a node is added.
 */
static void
_eina_share_common_population_head_add(Eina_Share *share,
                                       Eina_Share_Common_Head *head)
{
   head->population++;
   if (head->population > share->max_node_population)
      share->max_node_population = head->population;
}

/**
 * @internal
 * @brief Decrements population count for an Eina_Share_Common_Head.
 * Only active if EINA_STRINGSHARE_USAGE is defined.
 * @param share The Eina_Share instance (unused if EINA_STRINGSHARE_USAGE is off).
 * @param head The Eina_Share_Common_Head from which a node is removed.
 */
static void
_eina_share_common_population_head_del(EINA_UNUSED Eina_Share *share,
                                       Eina_Share_Common_Head *head)
{
   head->population--;
}

#else /* EINA_STRINGSHARE_USAGE undefined */

/* Stubs for population functions when EINA_STRINGSHARE_USAGE is not defined. */
static void _eina_share_common_population_init(EINA_UNUSED Eina_Share *share) {
}
static void _eina_share_common_population_shutdown(EINA_UNUSED Eina_Share *share)
{
}
static void _eina_share_common_population_stats(EINA_UNUSED Eina_Share *share) {
}
static void eina_share_common_population_nolock_add(EINA_UNUSED Eina_Share *share,
                                                    EINA_UNUSED int slen) {
}
void eina_share_common_population_add(EINA_UNUSED Eina_Share *share,
                                      EINA_UNUSED int slen) {
}
static void eina_share_common_population_nolock_del(EINA_UNUSED Eina_Share *share,
                                                    EINA_UNUSED int slen) {
}
void eina_share_common_population_del(EINA_UNUSED Eina_Share *share,
                                      EINA_UNUSED int slen) {
}
static void _eina_share_common_population_head_init(
   EINA_UNUSED Eina_Share *share,
   EINA_UNUSED Eina_Share_Common_Head *head) {
}
static void _eina_share_common_population_head_add(
   EINA_UNUSED Eina_Share *share,
   EINA_UNUSED
   Eina_Share_Common_Head *head) {
}
static void _eina_share_common_population_head_del(
   EINA_UNUSED Eina_Share *share,
   EINA_UNUSED
   Eina_Share_Common_Head *head) {
}
#endif

/**
 * @internal
 * @brief Compares an Eina_Share_Common_Head with a target hash value.
 * Used for looking up heads in the Rbtree. Compares based on the
 * EINA_SHARE_COMMON_NODE_HASH part of the head's stored hash.
 * @param ed The Eina_Share_Common_Head node from the Rbtree.
 * @param hash Pointer to the target hash value (specifically, the EINA_SHARE_COMMON_NODE_HASH part).
 * @param length Unused.
 * @param data Unused.
 * @return Negative if ed's hash is less than target, positive if greater, zero if equal.
 */
static int
_eina_share_common_cmp(const Eina_Share_Common_Head *ed,
                       const int *hash,
                       EINA_UNUSED int length,
                       EINA_UNUSED void *data)
{
   EINA_MAGIC_CHECK_SHARE_COMMON_HEAD(ed, , 0);

   return EINA_SHARE_COMMON_NODE_HASH(ed->hash) - *hash;
}

/**
 * @internal
 * @brief Rbtree callback to determine the relative order of two Eina_Share_Common_Head nodes.
 * Compares based on the EINA_SHARE_COMMON_NODE_HASH part of their stored hashes.
 * @param left The left Eina_Share_Common_Head node.
 * @param right The right Eina_Share_Common_Head node.
 * @param data Unused.
 * @return EINA_RBTREE_LEFT if left < right, EINA_RBTREE_RIGHT if left >= right.
 */
static Eina_Rbtree_Direction
_eina_share_common_node(const Eina_Share_Common_Head *left,
                        const Eina_Share_Common_Head *right,
                        EINA_UNUSED void *data)
{
   EINA_MAGIC_CHECK_SHARE_COMMON_HEAD(left,  , 0);
   EINA_MAGIC_CHECK_SHARE_COMMON_HEAD(right, , 0);

   if (EINA_SHARE_COMMON_NODE_HASH(left->hash) - EINA_SHARE_COMMON_NODE_HASH(right->hash) < 0)
      return EINA_RBTREE_LEFT;

   return EINA_RBTREE_RIGHT;
}

/**
 * @internal
 * @brief Frees an Eina_Share_Common_Head and all its associated Eina_Share_Common_Node(s).
 * This is typically used as a callback when deleting nodes from the Rbtree.
 * @param ed The Eina_Share_Common_Head to free.
 * @param data Unused.
 */
static void
_eina_share_common_head_free(Eina_Share_Common_Head *ed, EINA_UNUSED void *data)
{
   EINA_MAGIC_CHECK_SHARE_COMMON_HEAD(ed, );

   while (ed->head)
     {
        Eina_Share_Common_Node *el = ed->head;

        ed->head = ed->head->next;
        if (el != &ed->builtin_node)
           MAGIC_FREE(el);
     }
           MAGIC_FREE(ed);
}

/**
 * @internal
 * @brief Initializes an Eina_Share_Common_Node.
 * Sets its magic number, reference count, length, and copies the string data.
 * @param node The node to initialize.
 * @param str The string content.
 * @param slen The length of the string content.
 * @param null_size The size of the null terminator to append.
 * @param node_magic The magic number to set for this node.
 */
static void
_eina_share_common_node_init(Eina_Share_Common_Node *node,
                             const char *str,
                             int slen,
                             unsigned int null_size,
                             Eina_Magic node_magic)
{
   EINA_MAGIC_SET(node, node_magic);
   node->references = 1;
   node->length = slen;
   memcpy(node->str, str, slen);
   memset(node->str + slen, 0, null_size); /* Nullify the null */

   (void) node_magic; /* When magic are disable, node_magic is unused, this remove a warning. */
}

/**
 * @internal
 * @brief Allocates memory for an Eina_Share_Common_Head, including space for its builtin_node's string.
 * The allocation size is calculated to hold the Eina_Share_Common_Head structure
 * up to the `str` field of its `builtin_node`, plus the actual string length `slen`.
 * @param slen The length of the string that the builtin_node will hold.
 * @return A pointer to the allocated Eina_Share_Common_Head, or NULL on failure.
 */
static Eina_Share_Common_Head *
_eina_share_common_head_alloc(int slen)
{
   Eina_Share_Common_Head *head;
   const size_t head_size = offsetof(Eina_Share_Common_Head, builtin_node.str);

   head = malloc(head_size + slen);
   return head;
}

/**
 * @internal
 * @brief Adds a new Eina_Share_Common_Head to a given bucket (Rbtree) for a new string.
 * This function is called when a string is added and no existing head matches its
 * EINA_SHARE_COMMON_NODE_HASH. It allocates a new head, initializes its builtin_node
 * with the provided string, and inserts the head into the Rbtree.
 * @param share The main Eina_Share context.
 * @param p_bucket Pointer to the Rbtree root for the target bucket.
 * @param hash The full hash of the string.
 * @param str The string content.
 * @param slen The length of the string.
 * @param null_size The size of the null terminator.
 * @return Pointer to the string data within the newly added builtin_node, or NULL on failure.
 */
static const char *
_eina_share_common_add_head(Eina_Share *share,
                            Eina_Share_Common_Head **p_bucket,
                            int hash,
                            const char *str,
                            unsigned int slen,
                            unsigned int null_size)
{
   Eina_Rbtree **p_tree = (Eina_Rbtree **)p_bucket;
   Eina_Share_Common_Head *head;

   head = _eina_share_common_head_alloc(slen + null_size);
   if (!head)
      return NULL;

   EINA_MAGIC_SET(head, EINA_MAGIC_SHARE_HEAD);
   head->hash = hash;
   head->head = &head->builtin_node;
   _eina_share_common_node_init(head->head,
                                str,
                                slen,
                                null_size,
                                share->node_magic);
   head->head->next = NULL;

   _eina_share_common_population_head_init(share, head);

   *p_tree = eina_rbtree_inline_insert
         (*p_tree, EINA_RBTREE_GET(head),
         EINA_RBTREE_CMP_NODE_CB(_eina_share_common_node), NULL);

   return head->head->str;
}

/**
 * @internal
 * @brief Deletes an Eina_Share_Common_Head from its bucket (Rbtree) and frees its memory.
 * This is called when the last string node within a head is removed.
 * @param p_bucket Pointer to the Rbtree root for the target bucket.
 * @param head The Eina_Share_Common_Head to delete.
 */
static void
_eina_share_common_del_head(Eina_Share_Common_Head **p_bucket,
                            Eina_Share_Common_Head *head)
{
   Eina_Rbtree **p_tree = (Eina_Rbtree **)p_bucket;

   *p_tree = eina_rbtree_inline_remove
         (*p_tree, EINA_RBTREE_GET(head),
         EINA_RBTREE_CMP_NODE_CB(_eina_share_common_node), NULL);

         MAGIC_FREE(head);
}

/**
 * @internal
 * @brief Checks if an Eina_Share_Common_Node's string content matches a given string and length.
 * @param node The node to check.
 * @param str The string to compare against.
 * @param slen The length of the string to compare.
 * @return #EINA_TRUE if the node's string matches, #EINA_FALSE otherwise.
 */
static inline Eina_Bool
_eina_share_common_node_eq(const Eina_Share_Common_Node *node,
                           const char *str,
                           unsigned int slen)
{
   return ((node->length == slen) &&
           (memcmp(node->str, str, slen) == 0));
}

/**
 * @internal
 * @brief Finds an Eina_Share_Common_Node within a given Eina_Share_Common_Head
 * that matches the provided string content and length.
 * Implements a move-to-front heuristic: if a matching node is found and it's not
 * the first one, it's moved to the head of the list for faster future access.
 * @param head The Eina_Share_Common_Head to search within.
 * @param str The string content to find.
 * @param slen The length of the string.
 * @return Pointer to the matching Eina_Share_Common_Node, or NULL if not found.
 */
static Eina_Share_Common_Node *
_eina_share_common_head_find(Eina_Share_Common_Head *head,
                             const char *str,
                             unsigned int slen)
{
   Eina_Share_Common_Node *node, *prev;

   node = head->head;
   if (_eina_share_common_node_eq(node, str, slen))
      return node;

   prev = node;
   node = node->next;
   for (; node; prev = node, node = node->next)
      if (_eina_share_common_node_eq(node, str, slen))
        {
           /* promote node except builtin_node, make hot items be at the beginning */
           if (node->next)
             {
                prev->next = node->next;
                node->next = head->head;
                head->head = node;
             }
           return node;
        }

   return NULL;
}

/**
 * @internal
 * @brief Removes a specific Eina_Share_Common_Node from the linked list within an Eina_Share_Common_Head.
 * @param head The Eina_Share_Common_Head containing the node.
 * @param node The Eina_Share_Common_Node to remove.
 * @return #EINA_TRUE if the node was found and removed, #EINA_FALSE if the node was not found in the list.
 */
static Eina_Bool
_eina_share_common_head_remove_node(Eina_Share_Common_Head *head,
                                    const Eina_Share_Common_Node *node)
{
   Eina_Share_Common_Node *cur, *prev;

   if (head->head == node)
     {
        head->head = node->next;
        return 1;
     }

   prev = head->head;
   cur = head->head->next;
   for (; cur; prev = cur, cur = cur->next)
      if (cur == node)
        {
           prev->next = cur->next;
           return 1;
        }

   return 0;
}

/**
 * @internal
 * @brief Finds an Eina_Share_Common_Head within a bucket (Rbtree) that matches a given hash.
 * The comparison is based on the EINA_SHARE_COMMON_NODE_HASH part of the full hash.
 * @param bucket The root of the Rbtree (bucket) to search.
 * @param hash The EINA_SHARE_COMMON_NODE_HASH part of the full hash to search for.
 * @return Pointer to the matching Eina_Share_Common_Head, or NULL if not found.
 */
static Eina_Share_Common_Head *
_eina_share_common_find_hash(Eina_Share_Common_Head *bucket, int hash)
{
   return (Eina_Share_Common_Head *)eina_rbtree_inline_lookup
             (EINA_RBTREE_GET(bucket), &hash, 0,
             EINA_RBTREE_CMP_KEY_CB(_eina_share_common_cmp), NULL);
}

/**
 * @internal
 * @brief Retrieves the Eina_Share_Common_Head to which a given Eina_Share_Common_Node belongs,
 * specifically if the node is the `builtin_node` of the head.
 * This works by traversing to the end of the node's collision list (if any, though
 * for a builtin_node it should be the only one or the one it points to is NULL)
 * and then calculating the start of the Eina_Share_Common_Head structure based on the
 * known offset of `builtin_node`.
 * @param node Pointer to an Eina_Share_Common_Node, expected to be a `builtin_node`.
 * @return Pointer to the containing Eina_Share_Common_Head, or NULL/invalid if assumptions are violated or magic check fails.
 * @warning This function assumes `node` is part of a `builtin_node` structure.
 *          It traverses `node->next` until it finds the last node in the chain,
 *          then assumes *that last node* is the `builtin_node`. This logic might be
 *          fragile if a `builtin_node` could have `next` pointing to other non-builtin nodes
 *          that are *not* the end of the chain for *this specific head's builtin_node*.
 *          However, given typical usage, a `builtin_node` is either standalone or its `next`
 *          is NULL if it's the only node, or it's the actual `builtin_node` structure itself.
 *          The critical part is that the final node found by traversing `next` pointers
 *          must be the `builtin_node` for the offset calculation to be correct.
 */
static Eina_Share_Common_Head *
_eina_share_common_head_from_node(Eina_Share_Common_Node *node)
{
   Eina_Share_Common_Head *head;
   const size_t offset = offsetof(Eina_Share_Common_Head, builtin_node);

   /* Traverse to the end of the list. The last node is assumed to be the builtin_node
    * if this node originated from a builtin_node context.
    * This logic is specific to how builtin_nodes are linked or identified.
    * If 'node' is already the builtin_node and has no 'next', this loop is skipped.
    * If 'node' is a dynamically allocated node that is part of a list headed by a builtin_node,
    * this will find the tail, which is NOT the builtin_node.
    * This function seems intended to be called ONLY with a node that IS the builtin_node,
    * or where the list structure guarantees the tail is the builtin_node.
    * Given the context of its use in eina_share_common_del, 'node' can be any node.
    * If 'node' is the builtin_node, node->next is NULL (if it's the only one) or points to others.
    * If 'node' is NOT the builtin_node, this logic is problematic.
    * Re-evaluating: The primary user `eina_share_common_del` calls this.
    * If `node` is `&ed->builtin_node`, then `node->next` might be set.
    * The loop `while (node->next) node = node->next;` finds the *last* node in the chain
    * starting from the given `node`.
    * Then it assumes *this last node* is the `builtin_node` of some `Eina_Share_Common_Head`.
    * This is only correct if the original `node` passed in was indeed the `builtin_node`
    * or part of a chain that *terminates* with the `builtin_node` (which is unusual,
    * typically builtin_node is at the head).
    *
    * A more robust way if `node` could be any node in the list belonging to a head
    * (where one of them is a builtin_node) would be to have a direct back-pointer
    * or a different way to identify the head.
    *
    * Given the existing code, the assumption is that this function is called in a context
    * where `node` is the `builtin_node` itself, or the list structure is such that
    * the tail of the list starting from `node` is the `builtin_node`.
    * The most common case is when `node` *is* `ed->builtin_node`. In this case,
    * if `ed->builtin_node.next` is NULL, the loop doesn't run.
    * If `ed->builtin_node.next` is not NULL, it means other nodes were added *after* the builtin one,
    * which contradicts the typical list structure where new nodes are prepended.
    *
    * Let's assume the intent is: if `node` is the `builtin_node`, this works.
    * If `node` is a dynamically allocated node, this function is likely to return an incorrect head
    * unless that dynamic node is the *last* in a chain that *is* the `builtin_node` itself.
    * The `offsetof` calculation relies on `node` pointing to the `builtin_node` field.
    */
   while (node->next) /* This implies that the builtin_node is always the TAIL of its own list if it has one. */
     node = node->next; /* This will point 'node' to the actual Eina_Share_Common_Node that is embedded. */
   head = (Eina_Share_Common_Head *)((char*)node - offset);
   EINA_MAGIC_CHECK_SHARE_COMMON_HEAD(head, , 0);

   return head;
}

/**
 * @internal
 * @brief Allocates memory for an Eina_Share_Common_Node and its associated string data.
 * @param slen The length of the string data.
 * @param null_size The size of the null terminator to append.
 * @return Pointer to the allocated Eina_Share_Common_Node, or NULL on failure.
 */
static Eina_Share_Common_Node *
_eina_share_common_node_alloc(unsigned int slen, unsigned int null_size)
{
   Eina_Share_Common_Node *node;
   const size_t node_size = offsetof(Eina_Share_Common_Node, str);

   node = malloc(node_size + slen + null_size);
   return node;
}

/**
 * @internal
 * @brief Retrieves an Eina_Share_Common_Node pointer from a pointer to its string data.
 * This works by subtracting the known offset of the `str` field within the
 * Eina_Share_Common_Node structure. It then performs a magic check on the presumed node.
 * @param str Pointer to the character data (the `str` field of an Eina_Share_Common_Node).
 * @param node_magic The expected magic number for the node.
 * @return Pointer to the Eina_Share_Common_Node if valid, NULL otherwise (e.g., if magic check fails).
 */
static Eina_Share_Common_Node *
_eina_share_common_node_from_str(const char *str, Eina_Magic node_magic)
{
   Eina_Share_Common_Node *node;
   const size_t offset = offsetof(Eina_Share_Common_Node, str);

   node = (Eina_Share_Common_Node *)(str - offset);
   EINA_MAGIC_CHECK_SHARE_COMMON_NODE(node, node_magic, node = NULL);
   return node;

   (void) node_magic; /* When magic are disable, node_magic is unused, this remove a warning. */
}

/**
 * @internal
 * @brief Callback function for Eina_Iterator, used during eina_share_common_dump.
 * Iterates through all nodes in a given Eina_Share_Common_Head and updates dump statistics.
 * @param rbtree Unused.
 * @param head The Eina_Share_Common_Head currently being processed.
 * @param fdata Pointer to the struct dumpinfo to accumulate statistics.
 * @return #EINA_TRUE to continue iteration.
 */
static Eina_Bool
eina_iterator_array_check(const Eina_Rbtree *rbtree EINA_UNUSED,
                          Eina_Share_Common_Head *head,
                          struct dumpinfo *fdata)
{
   Eina_Share_Common_Node *node;

   fdata->used += sizeof(Eina_Share_Common_Head);
   for (node = head->head; node; node = node->next)
     {
        EINA_LOG_DBG("DDD: %5i %5i ", node->length, node->references);
        EINA_LOG_DBG("'%.*s'", node->length, ((char *)node) + sizeof(Eina_Share_Common_Node));
        fdata->used += sizeof(Eina_Share_Common_Node);
        fdata->used += node->length;
        fdata->saved += (node->references - 1) * node->length;
        fdata->dups += node->references - 1;
        fdata->unique++;
     }

   return EINA_TRUE;
}

/**
 * @endcond
 */


/*============================================================================*
*                                 Global                                     *
*============================================================================*/

/**
 * @internal
 * @brief Initialize the share_common module.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function sets up the share_common module of Eina. It is called by
 * eina_init().
 *
 * @see eina_init()
 */
Eina_Bool
eina_share_common_init(Eina_Share **_share,
                       Eina_Magic node_magic,
                       const char *node_magic_STR)
{
   Eina_Share *share;

   share = *_share = calloc(1, sizeof(Eina_Share));
   if (!share) goto on_error;

   share->share = calloc(1, sizeof(Eina_Share_Common));
   if (!share->share) goto on_error;

   share->node_magic = node_magic;
#define EMS(n) eina_magic_string_static_set(n, n ## _STR)
   EMS(EINA_MAGIC_SHARE);
   EMS(EINA_MAGIC_SHARE_HEAD);
   EMS(node_magic);
#undef EMS
   EINA_MAGIC_SET(share->share, EINA_MAGIC_SHARE);

   _eina_share_common_population_init(share);

   /* below is the common part among other all eina_share_common user */
   if (_eina_share_common_count++ != 0)
     return EINA_TRUE;

   eina_spinlock_new(&_mutex_big);
   return EINA_TRUE;

 on_error:
   _eina_share_common_count--;
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Shut down the share_common module.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function shuts down the share_common module set up by
 * eina_share_common_init(). It is called by eina_shutdown().
 *
 * @see eina_shutdown()
 */
Eina_Bool
eina_share_common_shutdown(Eina_Share **_share)
{
   unsigned int i;
   Eina_Share *share = *_share;

   eina_spinlock_take(&_mutex_big);

   _eina_share_common_population_stats(share);

   /* remove any string still in the table */
   for (i = 0; i < EINA_SHARE_COMMON_BUCKETS; i++)
     {
        eina_rbtree_delete(EINA_RBTREE_GET(
                              share->share->buckets[i]),
                           EINA_RBTREE_FREE_CB(
                              _eina_share_common_head_free), NULL);
        share->share->buckets[i] = NULL;
     }
   MAGIC_FREE(share->share);

   _eina_share_common_population_shutdown(share);

   eina_spinlock_release(&_mutex_big);

   free(*_share);
   *_share = NULL;

   /* below is the common part among other all eina_share_common user */
   if (--_eina_share_common_count != 0)
     return EINA_TRUE;

   eina_spinlock_free(&_mutex_big);

   return EINA_TRUE;
}

#ifdef EFL_HAVE_THREADS

/**
 * @internal
 * @brief Activate the share_common mutexes.
 *
 * This function activate the mutexes in the eina share_common module. It is called by
 * eina_threads_init().
 *
 * @see eina_threads_init()
 */
void
eina_share_common_threads_init(void)
{
   _share_common_threads_activated = EINA_TRUE;
}

/**
 * @internal
 * @brief Shut down the share_common mutexes.
 *
 * This function shuts down the mutexes in the share_common module.
 * It is called by eina_threads_shutdown().
 *
 * @see eina_threads_shutdown()
 */
void
eina_share_common_threads_shutdown(void)
{
   _share_common_threads_activated = EINA_FALSE;
}

#endif

/*============================================================================*
*                                   API                                      *
*============================================================================*/

/**
 * @cond LOCAL
 */

const char *
eina_share_common_add_length(Eina_Share *share,
                             const char *str,
                             unsigned int slen,
                             unsigned int null_size)
{
   Eina_Share_Common_Head **p_bucket, *ed;
   Eina_Share_Common_Node *el;
   int hash;

   if (!str)
      return NULL;

   eina_share_common_population_add(share, slen);

   if (slen == 0)
      return NULL;

   hash = eina_hash_superfast(str, slen);

   eina_spinlock_take(&_mutex_big);
   p_bucket = share->share->buckets + EINA_SHARE_COMMON_BUCKET_IDX(hash);

   ed = _eina_share_common_find_hash(*p_bucket, EINA_SHARE_COMMON_NODE_HASH(hash));
   if (!ed)
     {
        const char *s = _eina_share_common_add_head(share,
                                                    p_bucket,
                                                    hash,
                                                    str,
                                                    slen,
                                                    null_size);
        eina_spinlock_release(&_mutex_big);
        return s;
     }

   EINA_MAGIC_CHECK_SHARE_COMMON_HEAD(ed, eina_spinlock_release(&_mutex_big), NULL);

   el = _eina_share_common_head_find(ed, str, slen);
   if (el)
     {
        EINA_MAGIC_CHECK_SHARE_COMMON_NODE
          (el, share->node_magic,
           eina_spinlock_release(&_mutex_big); return NULL);
        el->references++;
        eina_spinlock_release(&_mutex_big);
        return el->str;
     }

   el = _eina_share_common_node_alloc(slen, null_size);
   if (!el)
     {
                                           eina_spinlock_release(&_mutex_big);
        return NULL;
     }

   _eina_share_common_node_init(el, str, slen, null_size, share->node_magic);
   el->next = ed->head;
   ed->head = el;
   _eina_share_common_population_head_add(share, ed);

   eina_spinlock_release(&_mutex_big);

   return el->str;
}

const char *
eina_share_common_ref(Eina_Share *share, const char *str)
{
   Eina_Share_Common_Node *node;

   if (!str)
      return NULL;

   eina_spinlock_take(&_mutex_big);
   node = _eina_share_common_node_from_str(str, share->node_magic);
   if (!node)
     {
        eina_spinlock_release(&_mutex_big);
        return str;
     }
   node->references++;

   eina_share_common_population_nolock_add(share, node->length);

   eina_spinlock_release(&_mutex_big);

   return str;
}


Eina_Bool
eina_share_common_del(Eina_Share *share, const char *str)
{
   unsigned int slen;
   Eina_Share_Common_Head *ed;
   Eina_Share_Common_Head **p_bucket;
   Eina_Share_Common_Node *node;

   if (!str)
      return EINA_TRUE;

   eina_spinlock_take(&_mutex_big);

   node = _eina_share_common_node_from_str(str, share->node_magic);
   if (!node)
      goto on_error;

   slen = node->length;
   eina_share_common_population_nolock_del(share, slen);
   if (node->references > 1)
     {
        node->references--;
        eina_spinlock_release(&_mutex_big);
        return EINA_TRUE;
     }

   node->references = 0;

   ed = _eina_share_common_head_from_node(node);
   if (!ed)
      goto on_error;

   EINA_MAGIC_CHECK_SHARE_COMMON_HEAD(ed, eina_spinlock_release(&_mutex_big), EINA_FALSE);

   if (node != &ed->builtin_node)
     {
        if (!_eina_share_common_head_remove_node(ed, node))
          goto on_error;
        MAGIC_FREE(node);
     }

   if (!ed->head || ed->head->references == 0)
     {
        p_bucket = share->share->buckets + EINA_SHARE_COMMON_BUCKET_IDX(ed->hash);
        _eina_share_common_del_head(p_bucket, ed);
     }
   else
      _eina_share_common_population_head_del(share, ed);

   eina_spinlock_release(&_mutex_big);

   return EINA_TRUE;

on_error:
   eina_spinlock_release(&_mutex_big);
   /* possible segfault happened before here, but... */
   return EINA_FALSE;
}

int
eina_share_common_length(EINA_UNUSED Eina_Share *share, const char *str)
{
   const Eina_Share_Common_Node *node;

   if (!str)
      return -1;

   node = _eina_share_common_node_from_str(str, share->node_magic);
   if (!node) return 0;
   return node->length;
}

void
eina_share_common_dump(Eina_Share *share, void (*additional_dump)(
                          struct dumpinfo *), int used)
{
   Eina_Iterator *it;
   unsigned int i;
   struct dumpinfo di;

   if (!share)
      return;

   di.used = used;
   di.saved = 0;
   di.dups = 0;
   di.unique = 0;

   eina_spinlock_take(&_mutex_big);
   for (i = 0; i < EINA_SHARE_COMMON_BUCKETS; i++)
     {
        if (!share->share->buckets[i])
          {
             continue;
          }

        it = eina_rbtree_iterator_prefix(
              (Eina_Rbtree *)share->share->buckets[i]);
        eina_iterator_foreach(it, EINA_EACH_CB(eina_iterator_array_check), &di);
        eina_iterator_free(it);
     }
   if (additional_dump)
      additional_dump(&di);

#ifdef EINA_STRINGSHARE_USAGE
   /* One character strings are not counted in the hash. */
   di.saved += share->population_group[0].count * sizeof(char);
   di.saved += share->population_group[1].count * sizeof(char) * 2;
#endif
   EINA_LOG_DBG("DDD:-------------------");
   EINA_LOG_DBG("DDD: usage (bytes) = %i, saved = %i (%3.0f%%)",
                di.used, di.saved, di.used ? (di.saved * 100.0 / di.used) : 0.0);
   EINA_LOG_DBG("DDD: unique: %d, duplicates: %d (%3.0f%%)",
                di.unique, di.dups, di.unique ? (di.dups * 100.0 / di.unique) : 0.0);

#ifdef EINA_STRINGSHARE_USAGE
   DBG_STRINGSHARE("DDD: Allocated strings: %i", share->population.count);
   DBG_STRINGSHARE("DDD: Max allocated strings: %i", share->population.max);
   DBG_STRINGSHARE("DDD: Max shared strings per node : %i", share->max_node_population);

   for (i = 0;
        i < sizeof (share->population_group) /
        sizeof (share->population_group[0]);
        ++i)
      DBG_STRINGSHARE("DDD: %i strings of length %u, max strings: %i",
                      share->population_group[i].count, i,
                      share->population_group[i].max);
#endif

   eina_spinlock_release(&_mutex_big);
}

/**
 * @endcond
 */
