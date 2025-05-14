/** @file
 *  Small compiler  - maintenance of various lists
 *
 *  Name list (aliases)
 *  Include path list
 *
 *  Copyright (c) ITB CompuPhase, 2001-2003
 *
 *  This software is provided "as-is", without any express or implied warranty.
 *  In no event will the authors be held liable for any damages arising from
 *  the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1.  The origin of this software must not be misrepresented; you must not
 *      claim that you wrote the original software. If you use this software in
 *      a product, an acknowledgment in the product documentation would be
 *      appreciated but is not required.
 *  2.  Altered source versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software.
 *  3.  This notice may not be removed or altered from any source distribution.
 *
 *  Version: $Id$
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "embryo_cc_sc.h"

/**
 * @brief Inserts a new string pair into a sorted linked list.
 *
 * The list is sorted by the 'first' string.
 *
 * @param root The head of the stringpair list (a dummy node).
 * @param first The first string of the pair. This string is duplicated.
 * @param second The second string of the pair. This string is duplicated.
 * @param matchlength The length to be used for matching this pair, typically strlen(first).
 * @return A pointer to the newly inserted stringpair node, or NULL on memory allocation failure.
 */
static stringpair  *
insert_stringpair(stringpair * root, char *first, char *second, int matchlength)
{
   stringpair         *cur, *pred;

   assert(root != NULL);
   assert(first != NULL);
   assert(second != NULL);
   /* create a new node, and check whether all is okay */
   if (!(cur = (stringpair *)malloc(sizeof(stringpair))))
      return NULL;
   cur->first = strdup(first);
   cur->second = strdup(second);
   cur->matchlength = matchlength;
   if (!cur->first || !cur->second)
     {
	if (cur->first)
	   free(cur->first);
	if (cur->second)
	   free(cur->second);
	free(cur);
	return NULL;
     }				/* if */
   /* link the node to the tree, find the position */
   for (pred = root; pred->next && strcmp(pred->next->first, first) < 0;
	pred = pred->next)
      /* nothing */ ;
   cur->next = pred->next;
   pred->next = cur;
   return cur;
}

/**
 * @brief Deletes all nodes in a stringpair list, freeing associated memory.
 *
 * This function iterates through the list starting from root->next and frees
 * each node and the strings it contains. The root node itself is then zeroed out.
 *
 * @param root The head of the stringpair list (a dummy node).
 */
static void
delete_stringpairtable(stringpair * root)
{
   stringpair         *cur, *next;

   assert(root != NULL);
   cur = root->next;
   while (cur)
     {
	next = cur->next;
	assert(cur->first != NULL);
	assert(cur->second != NULL);
	free(cur->first);
	free(cur->second);
	free(cur);
	cur = next;
     }				/* while */
   memset(root, 0, sizeof(stringpair));
}

/**
 * @brief Finds a stringpair in a list based on the 'first' string and a specific match length.
 *
 * It searches for a node where the 'first' string matches the provided 'first'
 * parameter up to 'matchlength' characters, and where the node's 'matchlength'
 * field also matches the provided 'matchlength'.
 *
 * @param cur The starting node to search from (typically root->next).
 * @param first The string to search for.
 * @param matchlength The number of characters to compare and the expected matchlength of the node.
 * @return A pointer to the found stringpair node, or NULL if not found.
 */
static stringpair  *
find_stringpair(stringpair * cur, char *first, int matchlength)
{
   int                 result = 0;

   assert(matchlength > 0);	/* the function cannot handle zero-length comparison */
   assert(first != NULL);
   while (cur && result <= 0)
     {
	result = (int)*cur->first - (int)*first;
	if (result == 0 && matchlength == cur->matchlength)
	  {
	     result = strncmp(cur->first, first, matchlength);
	     if (result == 0)
		return cur;
	  }			/* if */
	cur = cur->next;
     }				/* while */
   return NULL;
}

/**
 * @brief Deletes a specific item from a stringpair list.
 *
 * @param root The head of the stringpair list (a dummy node).
 * @param item A pointer to the stringpair node to be deleted.
 * @return TRUE if the item was found and deleted, FALSE otherwise.
 */
static int
delete_stringpair(stringpair * root, stringpair * item)
{
   stringpair         *cur;

   assert(root != NULL);
   cur = root;
   while (cur->next)
     {
	if (cur->next == item)
	  {
	     cur->next = item->next;	/* unlink from list */
	     assert(item->first != NULL);
	     assert(item->second != NULL);
	     free(item->first);
	     free(item->second);
	     free(item);
	     return TRUE;
	  }			/* if */
	cur = cur->next;
     }				/* while */
   return FALSE;
}

/* ----- alias table --------------------------------------------- */
/**
 * @brief The head of the alias table.
 *
 * This is a linked list of stringpair structures, where 'first' is the name
 * and 'second' is the alias. The list is kept sorted by name.
 */
static stringpair   alias_tab = { NULL, NULL, NULL, 0 };    /* alias table */

/**
 * @brief Inserts a new alias into the alias table.
 *
 * @param name The name to be aliased.
 * @param alias The alias string.
 * @return A pointer to the newly inserted alias node. Exits with error 103 on memory failure.
 */
stringpair *
insert_alias(char *name, char *alias)
{
   stringpair         *cur;

   assert(name != NULL);
   assert(strlen(name) <= sNAMEMAX);
   assert(alias != NULL);
   assert(strlen(alias) <= sEXPMAX);
   if (!(cur = insert_stringpair(&alias_tab, name, alias, strlen(name))))
      error(103);		/* insufficient memory (fatal error) */
   return cur;
}

/**
 * @brief Looks up an alias in the alias table.
 *
 * If the name is found, the corresponding alias string is copied into target.
 *
 * @param target Buffer to store the found alias. Must be at least sEXPMAX+1 characters.
 * @param name The name to look up.
 * @return TRUE if the alias was found, FALSE otherwise.
 */
int
lookup_alias(char *target, char *name)
{
   stringpair         *cur =
      find_stringpair(alias_tab.next, name, strlen(name));
   if (cur)
     {
	assert(strlen(cur->second) <= sEXPMAX);
	strcpy(target, cur->second);
     }				/* if */
   return !!cur;
}

/**
 * @brief Deletes all entries from the alias table and frees associated memory.
 */
void
delete_aliastable(void)
{
   delete_stringpairtable(&alias_tab);
}

/* ----- include paths list -------------------------------------- */
/**
 * @brief The head of the include paths list.
 *
 * This is a singly linked list of stringlist structures, where each 'line'
 * member stores an include path. New paths are added to the front of the list.
 */
static stringlist   includepaths = { NULL, NULL };	/* directory list for include files */

/**
 * @brief Inserts a new path into the include paths list.
 *
 * The new path is added to the beginning of the list.
 *
 * @param path The directory path to add. This string is duplicated.
 * @return A pointer to the newly inserted stringlist node. Exits with error 103 on memory failure.
 */
stringlist *
insert_path(char *path)
{
   stringlist         *cur;

   assert(path != NULL);
   if (!(cur = (stringlist *)malloc(sizeof(stringlist))))
      error(103);		/* insufficient memory (fatal error) */
   if (!(cur->line = strdup(path)))
      error(103);		/* insufficient memory (fatal error) */
   cur->next = includepaths.next;
   includepaths.next = cur;
   return cur;
}

/**
 * @brief Retrieves an include path by its index.
 *
 * Paths are indexed starting from 0 for the most recently added path.
 *
 * @param idx The index of the path to retrieve.
 * @return A pointer to the path string if found, NULL otherwise.
 */
char *
get_path(int idx)
{
   stringlist         *cur = includepaths.next;

   while (cur && idx-- > 0)
      cur = cur->next;
   if (cur)
     {
	assert(cur->line != NULL);
	return cur->line;
     }				/* if */
   return NULL;
}

/**
 * @brief Deletes all entries from the include paths list and frees associated memory.
 */
void
delete_pathtable(void)
{
   stringlist         *cur = includepaths.next, *next;

   while (cur)
     {
	next = cur->next;
	assert(cur->line != NULL);
	free(cur->line);
	free(cur);
	cur = next;
     }				/* while */
   memset(&includepaths, 0, sizeof(stringlist));
}

/* ----- text substitution patterns ------------------------------ */

/**
 * @brief The head of the text substitution patterns list.
 *
 * This is a linked list of stringpair structures, where 'first' is the pattern
 * and 'second' is the substitution. 'matchlength' stores the length of the pattern
 * to be matched (prefix length). The list is kept sorted by pattern.
 */
static stringpair   substpair = { NULL, NULL, NULL, 0 };    /* list of substitution pairs */
/**
 * @brief A quick lookup index for substitution patterns.
 *
 * `substindex[c - 'A']` points to the first substitution pattern in `substpair`
 * whose `first` string starts with character `c`. This speeds up searches.
 * The index covers 'A'-'Z', 'a'-'z', and '_'.
 * Example: `substindex[0]` for 'A', `substindex['_' - 'A']` for '_'.
 */
static stringpair  *substindex['z' - 'A' + 1];	/* quick index to first character */

/**
 * @brief Adjusts the quick lookup index for a given starting character.
 *
 * After a substitution pattern is added or removed, this function updates
 * the `substindex` for the starting character of that pattern. It finds the
 * first pattern in the `substpair` list that starts with `c` and updates
 * the corresponding `substindex` entry.
 *
 * @param c The first character of the pattern for which the index needs adjustment.
 *          Must be 'A'-'Z', 'a'-'z', or '_'.
 */
static void
adjustindex(char c)
{
   stringpair         *cur;

   assert((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_');
   assert('A' < '_' && '_' < 'z');

   for (cur = substpair.next; cur && cur->first[0] != c;
	cur = cur->next)
      /* nothing */ ;
   substindex[(int)c - 'A'] = cur;
}

/**
 * @brief Inserts a new text substitution pattern.
 *
 * The pattern and substitution are stored, and the `substindex` is updated.
 *
 * @param pattern The pattern string to search for. This string is duplicated.
 * @param substitution The string to replace the pattern with. This string is duplicated.
 * @param prefixlen The length of the pattern to match.
 * @return A pointer to the newly inserted stringpair node. Exits with error 103 on memory failure.
 */
stringpair *
insert_subst(char *pattern, char *substitution, int prefixlen)
{
   stringpair         *cur;

   assert(pattern != NULL);
   assert(substitution != NULL);
   if (!(cur = insert_stringpair(&substpair, pattern, substitution, prefixlen)))
      error(103);		/* insufficient memory (fatal error) */
   adjustindex(*pattern);
   return cur;
}

/**
 * @brief Finds a substitution pattern.
 *
 * Uses `substindex` for a quick initial lookup, then searches the list.
 *
 * @param name The beginning of a string to check for a pattern match.
 * @param length The length of the prefix of 'name' to match against patterns.
 * @return A pointer to the found stringpair node if a pattern matches the
 *         prefix of 'name' with the given 'length', NULL otherwise.
 */
stringpair *
find_subst(char *name, int length)
{
   stringpair         *item;

   assert(name != NULL);
   assert(length > 0);
   assert((*name >= 'A' && *name <= 'Z') || (*name >= 'a' && *name <= 'z')
	  || *name == '_');
   item = substindex[(int)*name - 'A'];
   if (item)
      item = find_stringpair(item, name, length);
   return item;
}

/**
 * @brief Deletes a substitution pattern.
 *
 * Finds the pattern matching 'name' and 'length', removes it from the list,
 * and updates `substindex`.
 *
 * @param name The pattern string to delete.
 * @param length The prefix length of the pattern to delete.
 * @return TRUE if the pattern was found and deleted, FALSE otherwise.
 */
int
delete_subst(char *name, int length)
{
   stringpair         *item;

   assert(name != NULL);
   assert(length > 0);
   assert((*name >= 'A' && *name <= 'Z') || (*name >= 'a' && *name <= 'z')
	  || *name == '_');
   item = substindex[(int)*name - 'A'];
   if (item)
      item = find_stringpair(item, name, length);
   if (!item)
      return FALSE;
   delete_stringpair(&substpair, item);
   adjustindex(*name);
   return TRUE;
}

/**
 * @brief Deletes all text substitution patterns and resets the index.
 *
 * Frees all memory associated with the substitution patterns and clears
 * the `substpair` list and the `substindex` array.
 */
void
delete_substtable(void)
{
   int                 i;

   delete_stringpairtable(&substpair);
   for (i = 0; i < (int)(sizeof(substindex) / sizeof(substindex[0])); i++)
      substindex[i] = NULL;
}
