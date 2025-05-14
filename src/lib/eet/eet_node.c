/**
 * @file
 * @brief Eet_Node functions for creating and manipulating Eet data structures in memory.
 *
 * This file implements the Eet_Node API, which allows for the construction
 * of tree-like data structures that can be serialized and deserialized using Eet.
 * Eet_Node provides a way to represent various data types (integers, strings,
 * lists, arrays, hashes, and structures) in a hierarchical manner.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <string.h>
#include <stdio.h>

#include <Eina.h>

#include "Eet.h"
#include "Eet_private.h"

static Eina_Mempool *_eet_node_mp = NULL; /**< Mempool for Eet_Node allocations. */

/**
 * @brief Allocates a new Eet_Node from the mempool.
 *
 * This function is a low-level allocator for Eet_Node structures.
 * It is typically used internally by other Eet_Node creation functions.
 * The allocated node is zero-initialized.
 *
 * @return A pointer to the newly allocated Eet_Node, or @c NULL on failure.
 * @see eet_node_free()
 */
Eet_Node *
eet_node_new(void)
{
   Eet_Node *result;

   result = eina_mempool_malloc(_eet_node_mp, sizeof (Eet_Node));
   if (!result)
     return NULL;

   memset(result, 0, sizeof (Eet_Node));
   return result;
}

/**
 * @brief Frees an Eet_Node allocated from the mempool.
 *
 * This function returns an Eet_Node to the mempool. It does not free
 * any data pointed to by the node's members (e.g., strings, child nodes).
 * For a recursive free, use eet_node_del().
 *
 * @param node The Eet_Node to free.
 * @see eet_node_new()
 * @see eet_node_del()
 */
void
eet_node_free(Eet_Node *node)
{
   eina_mempool_free(_eet_node_mp, node);
}

/**
 * @internal
 * @brief Internal helper to create a new Eet_Node with a name and type.
 *
 * Allocates and initializes a basic Eet_Node. The name is stringshared.
 *
 * @param name The name for the node.
 * @param type The Eet type for the node (e.g., EET_T_INT, EET_G_LIST).
 * @return A pointer to the newly created Eet_Node, or @c NULL on failure.
 */
static Eet_Node *
_eet_node_new(const char *name,
              int         type)
{
   Eet_Node *n;

   n = eet_node_new();
   if (!n)
     return NULL;

   n->type = type;
   n->name = eina_stringshare_add(name);

   return n;
}

/**
 * @internal
 * @brief Internal helper to append a list of nodes as children to a parent node.
 *
 * The provided list of @p nodes is reversed and prepended to the parent node's
 * existing children (n->values). This maintains the order of @p nodes when
 * they become children.
 *
 * @param n The parent Eet_Node to append children to.
 * @param nodes An Eina_List of Eet_Node children to append.
 */
static void
_eet_node_append(Eet_Node  *n,
                 Eina_List *nodes)
{
   Eet_Node *value;
   Eina_List *l;

   EINA_LIST_REVERSE_FOREACH(nodes, l, value)
     {
        value->next = n->values;
        n->values = value;
     }
}

/**
 * @internal
 * @def EET_NODE_NEW
 * @brief Macro to generate Eet_Node creation functions for basic scalar types.
 *
 * This macro simplifies the creation of functions like `eet_node_int_new()`,
 * `eet_node_char_new()`, etc.
 *
 * @param Eet_type The Eet type constant (e.g., EET_T_INT).
 * @param Name The suffix for the function name (e.g., `int` for `eet_node_int_new`).
 * @param Value The field name in the `Eet_Node_Data_Value` union (e.g., `i` for int).
 * @param Type The C data type of the value (e.g., `int`).
 */
#define EET_NODE_NEW(Eet_type, Name, Value, Type)         \
  EAPI Eet_Node *                                         \
  eet_node_ ## Name ## _new(const char *name, Type Value) \
  {                                                       \
     Eet_Node *n;                                         \
                                                          \
     n = _eet_node_new(name, Eet_type);                   \
     if (!n) { return NULL; }                             \
                                                          \
     n->data.value.Value = Value;                         \
                                                          \
     return n;                                            \
  }

/**
 * @internal
 * @def EET_NODE_STR_NEW
 * @brief Macro to generate Eet_Node creation functions for string types.
 *
 * This macro is similar to EET_NODE_NEW but specifically for string types,
 * as it uses `eina_stringshare_add()` for the value.
 *
 * @param Eet_type The Eet type constant (e.g., EET_T_STRING).
 * @param Name The suffix for the function name (e.g., `string` for `eet_node_string_new`).
 * @param Value The field name in the `Eet_Node_Data_Value` union (e.g., `str`).
 * @param Type The C data type of the value (e.g., `const char *`).
 */
#define EET_NODE_STR_NEW(Eet_type, Name, Value, Type)     \
  EAPI Eet_Node *                                         \
  eet_node_ ## Name ## _new(const char *name, Type Value) \
  {                                                       \
     Eet_Node *n;                                         \
                                                          \
     n = _eet_node_new(name, Eet_type);                   \
     if (!n) { return NULL; }                             \
                                                          \
     n->data.value.Value = eina_stringshare_add(Value);   \
                                                          \
     return n;                                            \
  }

/* Generate Eet_Node creation functions for scalar types */
EET_NODE_NEW(EET_T_CHAR, char, c, char) /**< @brief Creates a new char Eet_Node. @param name Node name. @param c Char value. @return New node or NULL. */
EET_NODE_NEW(EET_T_SHORT, short, s, short) /**< @brief Creates a new short Eet_Node. @param name Node name. @param s Short value. @return New node or NULL. */
EET_NODE_NEW(EET_T_INT, int, i, int) /**< @brief Creates a new int Eet_Node. @param name Node name. @param i Int value. @return New node or NULL. */
EET_NODE_NEW(EET_T_LONG_LONG, long_long, l, long long) /**< @brief Creates a new long long Eet_Node. @param name Node name. @param l Long long value. @return New node or NULL. */
EET_NODE_NEW(EET_T_FLOAT, float, f, float) /**< @brief Creates a new float Eet_Node. @param name Node name. @param f Float value. @return New node or NULL. */
EET_NODE_NEW(EET_T_DOUBLE, double, d, double) /**< @brief Creates a new double Eet_Node. @param name Node name. @param d Double value. @return New node or NULL. */
EET_NODE_NEW(EET_T_UCHAR, unsigned_char, uc, unsigned char) /**< @brief Creates a new unsigned char Eet_Node. @param name Node name. @param uc Unsigned char value. @return New node or NULL. */
EET_NODE_NEW(EET_T_USHORT, unsigned_short, us, unsigned short) /**< @brief Creates a new unsigned short Eet_Node. @param name Node name. @param us Unsigned short value. @return New node or NULL. */
EET_NODE_NEW(EET_T_UINT, unsigned_int, ui, unsigned int) /**< @brief Creates a new unsigned int Eet_Node. @param name Node name. @param ui Unsigned int value. @return New node or NULL. */
EET_NODE_NEW(EET_T_ULONG_LONG, unsigned_long_long, ul, unsigned long long) /**< @brief Creates a new unsigned long long Eet_Node. @param name Node name. @param ul Unsigned long long value. @return New node or NULL. */

/* Generate Eet_Node creation functions for string types */
EET_NODE_STR_NEW(EET_T_STRING, string, str, const char *) /**< @brief Creates a new string Eet_Node. @param name Node name. @param str String value (will be stringshared). @return New node or NULL. */
EET_NODE_STR_NEW(EET_T_INLINED_STRING, inlined_string, str, const char *) /**< @brief Creates a new inlined string Eet_Node. @param name Node name. @param str String value (will be stringshared). @return New node or NULL. */

/**
 * @brief Creates a new Eet_Node representing a NULL value.
 *
 * @param name The name for the node.
 * @return A pointer to the newly created Eet_Node, or @c NULL on failure.
 */
Eet_Node *
eet_node_null_new(const char *name)
{
   Eet_Node *n;

   n = _eet_node_new(name, EET_T_NULL);
   if (!n)
     return NULL;

   n->data.value.str = NULL;

   return n;
}

/**
 * @brief Creates a new Eet_Node representing a list.
 *
 * The provided @p nodes are appended as children to this new list node.
 * The order of nodes in the @p nodes list is preserved.
 *
 * @param name The name for the list node.
 * @param nodes An Eina_List of Eet_Node elements for the list.
 *              The list itself is not consumed, but its elements are linked.
 * @return A pointer to the newly created list Eet_Node, or @c NULL on failure.
 * @par Example
 * @code
 * Eina_List *items = NULL;
 * items = eina_list_append(items, eet_node_int_new("item1", 10));
 * items = eina_list_append(items, eet_node_string_new("item2", "hello"));
 * Eet_Node *list_node = eet_node_list_new("my_list", items);
 * // items list can be freed if no longer needed, its Eet_Node elements are now owned by list_node
 * eina_list_free(items);
 * @endcode
 */
Eet_Node *
eet_node_list_new(const char *name,
                  Eina_List  *nodes)
{
   Eet_Node *n;

   n = _eet_node_new(name, EET_G_LIST);
   if (!n)
     return NULL;

   _eet_node_append(n, nodes);

   return n;
}

/**
 * @brief Creates a new Eet_Node representing a fixed-size array.
 *
 * The provided @p nodes are appended as children to this new array node.
 * The @p count specifies the number of elements in the array.
 * The order of nodes in the @p nodes list is preserved.
 *
 * @param name The name for the array node.
 * @param count The number of elements in the array. This should match the
 *              number of elements in the @p nodes list.
 * @param nodes An Eina_List of Eet_Node elements for the array.
 *              The list itself is not consumed, but its elements are linked.
 * @return A pointer to the newly created array Eet_Node, or @c NULL on failure.
 * @par Example
 * @code
 * Eina_List *elements = NULL;
 * elements = eina_list_append(elements, eet_node_int_new(NULL, 1)); // Array elements often don't need names
 * elements = eina_list_append(elements, eet_node_int_new(NULL, 2));
 * elements = eina_list_append(elements, eet_node_int_new(NULL, 3));
 * Eet_Node *array_node = eet_node_array_new("my_array", 3, elements);
 * // elements list can be freed
 * eina_list_free(elements);
 * @endcode
 */
Eet_Node *
eet_node_array_new(const char *name,
                   int         count,
                   Eina_List  *nodes)
{
   Eet_Node *n;

   n = _eet_node_new(name, EET_G_ARRAY);
   if (!n)
     return NULL;

   n->count = count;

   _eet_node_append(n, nodes);

   return n;
}

/**
 * @brief Creates a new Eet_Node representing a variable-size array.
 *
 * The provided @p nodes are appended as children to this new array node.
 * The count of elements is determined by the number of items in the @p nodes list.
 * The order of nodes in the @p nodes list is preserved.
 *
 * @param name The name for the variable-size array node.
 * @param nodes An Eina_List of Eet_Node elements for the array.
 *              The list itself is not consumed, but its elements are linked.
 *              The count of elements in this list will be used as the array count.
 * @return A pointer to the newly created variable-size array Eet_Node, or @c NULL on failure.
 * @par Example
 * @code
 * Eina_List *elements = NULL;
 * elements = eina_list_append(elements, eet_node_string_new(NULL, "apple"));
 * elements = eina_list_append(elements, eet_node_string_new(NULL, "banana"));
 * Eet_Node *var_array_node = eet_node_var_array_new("my_fruits", elements);
 * // elements list can be freed
 * eina_list_free(elements);
 * @endcode
 */
Eet_Node *
eet_node_var_array_new(const char *name,
                       Eina_List  *nodes)
{
   Eet_Node *n;

   n = _eet_node_new(name, EET_G_VAR_ARRAY);
   if (!n)
     return NULL;

   n->count = eina_list_count(nodes);

   _eet_node_append(n, nodes);

   return n;
}

/**
 * @brief Creates a new Eet_Node representing a hash (or a single hash entry).
 *
 * This function creates a hash node. In Eet's node representation, a hash node
 * typically holds a single key-value pair. To represent a full hash table,
 * multiple such nodes (each created with `eet_node_hash_new`) would be added
 * as children to a parent struct or list node.
 *
 * The @p key is stringshared. The @p node becomes the value associated with the key.
 *
 * @param name The name for the hash node itself (can be distinct from the key).
 * @param key The key for this hash entry.
 * @param node The Eet_Node representing the value for this key.
 * @return A pointer to the newly created hash Eet_Node, or @c NULL on failure.
 *         Returns @c NULL if @p node is @c NULL.
 * @par Example
 * @code
 * Eet_Node *value_node = eet_node_int_new("value_for_key1", 123);
 * Eet_Node *hash_entry_node = eet_node_hash_new("entry1", "key1", value_node);
 * // hash_entry_node can then be added to a struct or list representing the hash table
 * @endcode
 */
Eet_Node *
eet_node_hash_new(const char *name,
                  const char *key,
                  Eet_Node   *node)
{
   Eina_List *nodes;
   Eet_Node *n;

   if (!node)
     return NULL;

   n = _eet_node_new(name, EET_G_HASH);
   if (!n)
     return NULL;

   n->key = eina_stringshare_add(key);
   nodes = eina_list_append(NULL, node);

   _eet_node_append(n, nodes);

   return n;
}

/**
 * @brief Creates a new Eet_Node representing a structure.
 *
 * The provided @p nodes are appended as children (fields) to this new struct node.
 * The order of nodes in the @p nodes list is preserved.
 *
 * @param name The name for the struct node.
 * @param nodes An Eina_List of Eet_Node elements representing the fields of the structure.
 *              The list itself is not consumed, but its elements are linked.
 * @return A pointer to the newly created struct Eet_Node, or @c NULL on failure.
 * @par Example
 * @code
 * Eina_List *fields = NULL;
 * fields = eina_list_append(fields, eet_node_int_new("id", 1));
 * fields = eina_list_append(fields, eet_node_string_new("label", "example"));
 * Eet_Node *struct_node = eet_node_struct_new("my_struct", fields);
 * // fields list can be freed
 * eina_list_free(fields);
 * @endcode
 */
Eet_Node *
eet_node_struct_new(const char *name,
                    Eina_List  *nodes)
{
   Eet_Node *n;

   n = _eet_node_new(name, EET_G_UNKNOWN);
   if (!n)
     return NULL;

   _eet_node_append(n, nodes);

   return n;
}

/**
 * @brief Creates a new struct Eet_Node or wraps an existing non-struct child.
 *
 * This function is intended to ensure that a child node can be treated as
 * part of a structure.
 * - If @p child is already a struct (type EET_G_UNKNOWN), it is returned directly.
 * - Otherwise, a new struct node with the name @p parent is created, and @p child
 *   is added as its first child.
 *
 * @param parent The name to use if a new parent struct node needs to be created.
 * @param child The child Eet_Node.
 * @return A pointer to an Eet_Node suitable for use as a struct. This might be
 *         @p child itself or a new parent node. Returns @c NULL if @p child is @c NULL
 *         or if memory allocation fails.
 */
Eet_Node *
eet_node_struct_child_new(const char *parent,
                          Eet_Node   *child)
{
   Eet_Node *n;

   if (!child) return NULL;

   if (child->type != EET_G_UNKNOWN)
     return child;

   n = _eet_node_new(parent, EET_G_UNKNOWN);
   if (!n)
     return NULL;

   _eet_node_append(n, eina_list_prepend(NULL, child));

   return n;
}

/**
 * @brief Gets the first child of a given Eet_Node.
 *
 * For group nodes (list, array, struct, hash), this returns the first element
 * or field. For scalar nodes, this will be @c NULL.
 *
 * @param node The Eet_Node to get children from.
 * @return A pointer to the first child Eet_Node, or @c NULL if no children or @p node is @c NULL.
 */
Eet_Node *
eet_node_children_get(Eet_Node *node)
{
   if (!node) return NULL;
   return node->values;
}

/**
 * @brief Gets the next sibling of a given Eet_Node.
 *
 * @param node The Eet_Node to get the next sibling of.
 * @return A pointer to the next sibling Eet_Node, or @c NULL if no next sibling or @p node is @c NULL.
 */
Eet_Node *
eet_node_next_get(Eet_Node *node)
{
   if (!node) return NULL;
   return node->next;
}

/**
 * @brief Gets the parent of a given Eet_Node.
 *
 * @param node The Eet_Node to get the parent of.
 * @return A pointer to the parent Eet_Node, or @c NULL if no parent or @p node is @c NULL.
 */
Eet_Node *
eet_node_parent_get(Eet_Node *node)
{
   if (!node) return NULL;
   return node->parent;
}

/**
 * @brief Appends a child node to a list within a parent node.
 *
 * If a list with the given @p name already exists as a child of @p parent,
 * @p child is appended to that existing list.
 * Otherwise, a new list node with the given @p name is created, @p child is
 * added to it, and this new list node is added as a child to @p parent.
 *
 * The @p child node's `parent` pointer is set to @p parent.
 *
 * @param parent The parent Eet_Node. Must not be @c NULL.
 * @param name The name of the list to find or create.
 * @param child The Eet_Node to append. Must not be @c NULL.
 */
void
eet_node_list_append(Eet_Node   *parent,
                     const char *name,
                     Eet_Node   *child)
{
   const char *tmp;
   Eet_Node *nn;

   if ((!parent) || (!child)) return;
   tmp = eina_stringshare_add(name);

   for (nn = parent->values; nn; nn = nn->next)
     if (nn->name == tmp && nn->type == EET_G_LIST)
       {
          Eet_Node *n;

          if (!nn->values)
            nn->values = child;
          else
            {
               for (n = nn->values; n->next; n = n->next)
                 ;
               n->next = child;
            }

          child->next = NULL;
          child->parent = parent;

          eina_stringshare_del(tmp);

          return;
       }

   /* No list found, so create it. */
   nn = eet_node_list_new(tmp, eina_list_append(NULL, child));

   /* And add it to the parent. */
   nn->next = parent->values;
   parent->values = nn;
   child->parent = parent;

   eina_stringshare_del(tmp);
}

/**
 * @brief Appends or replaces a child node (field) in a struct parent node.
 *
 * If @p parent is not a struct type (EET_G_UNKNOWN), an error is logged,
 * and @p child is deleted.
 *
 * If a child with the same @p name and @p child->type already exists in @p parent,
 * the existing child is deleted and replaced by the new @p child.
 * Otherwise, @p child is appended to the parent's children.
 *
 * The @p child node's `parent` pointer is set to @p parent.
 *
 * @param parent The parent Eet_Node (must be of type EET_G_UNKNOWN). Must not be @c NULL.
 * @param name The name of the field to append/replace. This name is associated with the @p child
 *             when it's part of the struct, but @p child retains its original name.
 *             Effectively, this @p name is used for lookup within the struct.
 * @param child The Eet_Node to append as a field. Must not be @c NULL.
 */
void
eet_node_struct_append(Eet_Node   *parent,
                       const char *name,
                       Eet_Node   *child)
{
   const char *tmp;
   Eet_Node *prev;
   Eet_Node *nn;

   if ((!parent) || (!child)) return;
   if (parent->type != EET_G_UNKNOWN)
     {
        ERR("[%s] is not a structure. Will not insert [%s] in it",
            parent->name,
            name);
        eet_node_del(child);
        return;
     }

   tmp = eina_stringshare_add(name);

   for (prev = NULL, nn = parent->values; nn; prev = nn, nn = nn->next)
     if (nn->name == tmp && nn->type == child->type)
       {
          if (prev)
            prev->next = nn->next;
          else
            parent->values = nn->next;

          nn->next = NULL;
          eet_node_del(nn);

          break;
       }

   if (prev)
     {
        prev->next = child;
        child->next = NULL;
     }
   else
     {
        child->next = NULL;
        parent->values = child;
     }
   child->parent = parent;

   eina_stringshare_del(tmp);
}

/**
 * @brief Adds a new hash entry (key-value pair) as a child to a parent node.
 *
 * This function creates a new hash node (representing a single key-value pair)
 * using @p name for the hash node itself, @p key for the hash key, and @p child
 * as the value. This new hash node is then prepended to the @p parent's children.
 *
 * The @p child node's `parent` pointer is set to @p parent.
 *
 * @note This effectively adds a new key-value pair to a conceptual hash table
 *       represented by children of @p parent. It does not search for an existing
 *       hash node with @p name.
 *
 * @param parent The parent Eet_Node. Must not be @c NULL.
 * @param name The name for the new hash Eet_Node that will be created.
 * @param key The key for the hash entry.
 * @param child The Eet_Node representing the value for the key. Must not be @c NULL.
 */
void
eet_node_hash_add(Eet_Node   *parent,
                  const char *name,
                  const char *key,
                  Eet_Node   *child)
{
   Eet_Node *nn;

   if ((!parent) || (!child)) return;

   /* No list found, so create it. */
   nn = eet_node_hash_new(name, key, child);

   /* And add it to the parent. */
   nn->next = parent->values;
   parent->values = nn;
   child->parent = parent;
}

/**
 * @brief Gets the type of an Eet_Node.
 *
 * @param node The Eet_Node to query.
 * @return The type of the node (e.g., EET_T_INT, EET_G_LIST).
 *         Returns EET_T_UNKNOW if @p node is @c NULL.
 */
int
eet_node_type_get(Eet_Node *node)
{
   if (!node) return EET_T_UNKNOW; /* Note: EET_T_UNKNOW is likely a typo for EET_G_UNKNOWN or another default */
   return node->type;
}

/**
 * @brief Gets the data payload of an Eet_Node.
 *
 * The interpretation of this data depends on the node's type.
 *
 * @param node The Eet_Node to query.
 * @return A pointer to the Eet_Node_Data union containing the node's value,
 *         or @c NULL if @p node is @c NULL.
 */
Eet_Node_Data *
eet_node_value_get(Eet_Node *node)
{
   if (!node) return NULL;
   return &node->data;
}

/**
 * @brief Gets the name of an Eet_Node.
 *
 * @param node The Eet_Node to query.
 * @return The name of the node (a stringshared string), or @c NULL if @p node is @c NULL.
 */
const char *
eet_node_name_get(Eet_Node *node)
{
   if (!node) return NULL;
   return node->name;
}

/**
 * @brief Recursively deletes an Eet_Node and all its children.
 *
 * This function frees the specified node @p n and all nodes in its subtree.
 * It handles different node types appropriately:
 * - For hash nodes, the key string is freed.
 * - For group nodes (struct, array, list, hash), all child nodes are recursively deleted.
 * - For string nodes, the stringshared value is released.
 * - Scalar types require no special value freeing beyond the node itself.
 * Finally, the node's name is released and the node itself is freed using `eet_node_free()`.
 *
 * @param n The Eet_Node to delete. If @c NULL, the function does nothing.
 */
void
eet_node_del(Eet_Node *n)
{
   Eet_Node *nn;
   Eet_Node *tmp;

   if (!n)
     return;

   switch (n->type)
     {
      case EET_G_HASH:
        eina_stringshare_del(n->key);
        /* No break here as we want it to fall through and free the resources */
        EINA_FALLTHROUGH;

      case EET_G_UNKNOWN:
      case EET_G_VAR_ARRAY:
      case EET_G_ARRAY:
      case EET_G_LIST:
        for (nn = n->values; nn; )
          {
             tmp = nn;
             nn = nn->next;
             eet_node_del(tmp);
          }
        break;

      case EET_T_STRING:
      case EET_T_INLINED_STRING:
        eina_stringshare_del(n->data.value.str);
        break;

      case EET_T_CHAR:
      case EET_T_SHORT:
      case EET_T_INT:
      case EET_T_LONG_LONG:
      case EET_T_FLOAT:
      case EET_T_DOUBLE:
      case EET_T_UCHAR:
      case EET_T_USHORT:
      case EET_T_UINT:
        break;
     }

   eina_stringshare_del(n->name);
   eet_node_free(n);
}

static const char *eet_node_dump_g_name[6] = {
   "struct",
   "array",
   "var_array",
   "list",
   "hash",
   "???" /* Placeholder for unknown group types */
};

/** @internal Mapping of Eet basic types to their names and format specifiers for dumping. */
static const char *eet_node_dump_t_name[14][2] = {
   { "???: ", "???" }, /* EET_T_UNKNOW or unhandled */
   { "char: ", "%hhi" }, /* EET_T_CHAR */
   { "short: ", "%hi" }, /* EET_T_SHORT */
   { "int: ", "%i" }, /* EET_T_INT */
   { "long_long: ", "%lli" }, /* EET_T_LONG_LONG */
   { "float: ", "%1.25f" }, /* EET_T_FLOAT */
   { "double: ", "%1.25f" }, /* EET_T_DOUBLE */
   { "uchar: ", "%hhu" }, /* EET_T_UCHAR */
   { "ushort: ", "%i" }, /* EET_T_USHORT - Note: %i might be problematic for full unsigned short range, %hu preferred */
   { "uint: ", "%u" }, /* EET_T_UINT */
   { "ulong_long: ", "%llu" }, /* EET_T_ULONG_LONG */
   { "null", "" } /* EET_T_NULL */
   /* EET_T_STRING and EET_T_INLINED_STRING are handled specially */
};

/**
 * @internal
 * @brief Helper function for eet_node_dump to print indentation.
 *
 * Prints `level` number of "  " (two spaces) strings using the dumpfunc.
 *
 * @param level The current indentation level.
 * @param dumpfunc The callback function to use for printing.
 * @param dumpdata User data for the dumpfunc.
 */
static void
eet_node_dump_level(int               level,
                    Eet_Dump_Callback dumpfunc,
                    void             *dumpdata)
{
   int i;

   for (i = 0; i < level; i++) dumpfunc(dumpdata, "  ");
}

/**
 * @internal
 * @brief Escapes special characters in a string for safe dumping.
 *
 * Replaces '\"', '\\', and '\n' with their escaped versions ("\\\"", "\\\\", "\\n").
 * The returned string must be freed by the caller using `free()`.
 *
 * @param str The input string to escape.
 * @return A newly allocated string with escaped characters, or @c NULL on allocation failure.
 */
static char *
eet_node_string_escape(const char *str)
{
   char *s, *sp;
   const char *strp;
   int sz = 0;

   for (strp = str; *strp; strp++)
     {
        if (*strp == '\"')
          sz += 2;
        else if (*strp == '\\')
          sz += 2;
        else if (*strp == '\n')
          sz += 2;
        else
          sz += 1;
     }
   s = malloc(sz + 1);
   if (!s)
     return NULL;

   for (strp = str, sp = s; *strp; strp++, sp++)
     {
        if (*strp == '\"'
            || *strp == '\\'
            || *strp == '\n')
          {
             *sp = '\\';
             sp++;
          }

        if (*strp == '\n')
          *sp = 'n';
        else
          *sp = *strp;
     }
   *sp = 0;
   return s;
}

/**
 * @internal
 * @brief Dumps an escaped string using the provided callback.
 *
 * Escapes the string using `eet_node_string_escape` and then prints it
 * via `dumpfunc`. Frees the temporary escaped string.
 *
 * @param dumpdata User data for the dumpfunc.
 * @param dumpfunc The callback function to use for printing.
 * @param str The string to escape and dump.
 */
static void
eet_node_dump_string_escape(void             *dumpdata,
                            Eet_Dump_Callback dumpfunc,
                            const char       *str)
{
   char *s;

   s = eet_node_string_escape(str);
   if (!s)
     return;

   dumpfunc(dumpdata, s);
   free(s);
}

/**
 * @internal
 * @brief Dumps a simple (scalar or string) Eet_Node.
 *
 * Formats and prints the name and value of a simple type node (char, int, string, etc.)
 * using the `dumpfunc`. Handles indentation and string escaping.
 *
 * @param n The Eet_Node to dump (must be a simple type).
 * @param level The current indentation level.
 * @param dumpfunc The callback function to use for printing.
 * @param dumpdata User data for the dumpfunc.
 */
static void
eet_node_dump_simple_type(Eet_Node         *n,
                          int               level,
                          Eet_Dump_Callback dumpfunc,
                          void             *dumpdata)
{
   const char *type_name = NULL;
   char tbuf[256];

   eet_node_dump_level(level, dumpfunc, dumpdata);
   dumpfunc(dumpdata, "value \"");
   eet_node_dump_string_escape(dumpdata, dumpfunc, n->name);
   dumpfunc(dumpdata, "\" ");

#ifdef EET_T_TYPE
# undef EET_T_TYPE
#endif /* ifdef EET_T_TYPE */

#define EET_T_TYPE(Eet_Type, Type)                        \
case Eet_Type:                                            \
{                                                         \
   dumpfunc(dumpdata, eet_node_dump_t_name[Eet_Type][0]); \
   snprintf(tbuf,                                         \
            sizeof (tbuf),                                \
            eet_node_dump_t_name[Eet_Type][1],            \
            n->data.value.Type);                          \
   dumpfunc(dumpdata, tbuf);                              \
   break;                                                 \
}

   switch (n->type)
     {
        EET_T_TYPE(EET_T_CHAR, c);
        EET_T_TYPE(EET_T_SHORT, s);
        EET_T_TYPE(EET_T_INT, i);
        EET_T_TYPE(EET_T_LONG_LONG, l);
        EET_T_TYPE(EET_T_FLOAT, f);
        EET_T_TYPE(EET_T_DOUBLE, d);
        EET_T_TYPE(EET_T_UCHAR, uc);
        EET_T_TYPE(EET_T_USHORT, us);
        EET_T_TYPE(EET_T_UINT, ui);
        EET_T_TYPE(EET_T_ULONG_LONG, ul);

      case EET_T_INLINED_STRING:
        type_name = "inlined: \"";
        /* inlined string are just like a string, but not inside the general
         * dictionnary. No need to duplicate code. */
        EINA_FALLTHROUGH;

      case EET_T_STRING:
        if (!type_name)
          type_name = "string: \"";

        dumpfunc(dumpdata, type_name);
        eet_node_dump_string_escape(dumpdata, dumpfunc, n->data.value.str);
        dumpfunc(dumpdata, "\"");
        break;

      case EET_T_NULL:
        dumpfunc(dumpdata, "null");
        break;

      default:
        dumpfunc(dumpdata, "???: ???");
        break;
     }

   dumpfunc(dumpdata, ";\n");
}

/**
 * @internal
 * @brief Dumps the starting part of a group Eet_Node (struct, list, array, hash).
 *
 * Prints the indentation, group type, name, and opening brace '{'.
 *
 * @param level The current indentation level.
 * @param dumpfunc The callback function to use for printing.
 * @param dumpdata User data for the dumpfunc.
 * @param group_type The type of the group (EET_G_UNKNOWN, EET_G_LIST, etc.).
 * @param name The name of the group node.
 */
static void
eet_node_dump_group_start(int               level,
                          Eet_Dump_Callback dumpfunc,
                          void             *dumpdata,
                          int               group_type,
                          const char       *name)
{
   int chnk_type;

   chnk_type = (group_type >= EET_G_UNKNOWN && group_type <= EET_G_HASH) ?
     group_type : EET_G_UNKNOWN;

   eet_node_dump_level(level, dumpfunc, dumpdata);
   dumpfunc(dumpdata, "group \"");
   eet_node_dump_string_escape(dumpdata, dumpfunc, name);
   dumpfunc(dumpdata, "\" ");

   dumpfunc(dumpdata, eet_node_dump_g_name[chnk_type - EET_G_UNKNOWN]);
   dumpfunc(dumpdata, " {\n");
}

/**
 * @internal
 * @brief Dumps the ending part of a group Eet_Node.
 *
 * Prints the indentation and closing brace '}'.
 *
 * @param level The current indentation level.
 * @param dumpfunc The callback function to use for printing.
 * @param dumpdata User data for the dumpfunc.
 */
static void
eet_node_dump_group_end(int               level,
                        Eet_Dump_Callback dumpfunc,
                        void             *dumpdata)
{
   eet_node_dump_level(level, dumpfunc, dumpdata);
   dumpfunc(dumpdata, "}\n");
}

/**
 * @brief Dumps the structure of an Eet_Node tree for debugging.
 *
 * Recursively traverses the Eet_Node tree starting from @p n and prints
 * its structure using the provided @p dumpfunc callback.
 *
 * @param n The root Eet_Node of the tree to dump.
 * @param dumplevel The initial indentation level for dumping.
 * @param dumpfunc The callback function to be called for each piece of text to dump.
 *                 Example: `static void my_dump_func(void *data, const char *text) { fprintf(stdout, "%s", text); }`
 * @param dumpdata User-specific data to be passed to @p dumpfunc.
 */
void
eet_node_dump(Eet_Node         *n,
              int               dumplevel,
              Eet_Dump_Callback dumpfunc,
              void             *dumpdata)
{
   Eet_Node *it;

   if (!n)
     return;

   switch (n->type)
     {
      case EET_G_VAR_ARRAY:
      case EET_G_ARRAY:
      case EET_G_UNKNOWN:
      case EET_G_HASH:
      case EET_G_LIST:
        eet_node_dump_group_start(dumplevel,
                                  dumpfunc,
                                  dumpdata,
                                  n->type,
                                  n->name);

        if (n->type == EET_G_VAR_ARRAY
            || n->type == EET_G_ARRAY)
          {
             char tbuf[256];

             eet_node_dump_level(dumplevel, dumpfunc, dumpdata);
             dumpfunc(dumpdata, "    count ");
             eina_convert_itoa(n->count, tbuf);
             dumpfunc(dumpdata, tbuf);
             dumpfunc(dumpdata, ";\n");
          }
        else if (n->type == EET_G_HASH)
          {
             eet_node_dump_level(dumplevel, dumpfunc, dumpdata);
             dumpfunc(dumpdata, "    key \"");
             eet_node_dump_string_escape(dumpdata, dumpfunc, n->key);
             dumpfunc(dumpdata, "\";\n");
          }

        for (it = n->values; it; it = it->next)
          eet_node_dump(it, dumplevel + 2, dumpfunc, dumpdata);

        eet_node_dump_group_end(dumplevel, dumpfunc, dumpdata);
        break;

      case EET_T_STRING:
      case EET_T_INLINED_STRING:
      case EET_T_CHAR:
      case EET_T_SHORT:
      case EET_T_INT:
      case EET_T_LONG_LONG:
      case EET_T_FLOAT:
      case EET_T_DOUBLE:
      case EET_T_UCHAR:
      case EET_T_USHORT:
      case EET_T_UINT:
      case EET_T_ULONG_LONG:
        eet_node_dump_simple_type(n, dumplevel, dumpfunc, dumpdata);
        break;
     }
}

/**
 * @brief Walks an Eet_Node tree and reconstructs it using user-provided callbacks.
 *
 * This function traverses the Eet_Node tree starting from @p root. For each node
 * encountered, it calls appropriate functions from the @p cb structure to allow
 * the user to build a custom representation of the Eet data.
 *
 * This is useful for converting an Eet_Node tree into a different data structure,
 * for example, application-specific structs.
 *
 * @param parent A pointer to the parent object in the user's custom data structure.
 *               This is passed to `cb->struct_add` when adding the current `me` object.
 *               For the initial call (top-level node), this can be @c NULL.
 * @param name The name of the current node being processed, relative to its @p parent
 *             in the user's structure. Passed to `cb->struct_add`.
 *             For the initial call, this might be the overall name of the structure.
 * @param root The Eet_Node to start walking from.
 * @param cb A pointer to an Eet_Node_Walk structure containing callback functions.
 *           These functions are responsible for allocating and populating the
 *           user's custom data structures.
 * @param user_data User-specific data to be passed to all callback functions.
 * @return A pointer to the user-defined object created for the @p root node,
 *         as returned by one of the `cb` functions (e.g., `cb->struct_alloc`, `cb->array`, etc.).
 *         Returns @c NULL if @p root is @c NULL and no parent object is provided to attach a NULL value to.
 *         If @p root is a hash node and @p parent is @c NULL, it also returns @c NULL as hashes
 *         are typically properties of a parent structure.
 */
void *
eet_node_walk(void          *parent,
              const char    *name,
              Eet_Node      *root,
              Eet_Node_Walk *cb,
              void          *user_data)
{
   Eet_Node *it;
   void *me = NULL;
   int i;

   if (!root)
     {
        if (parent)
          cb->struct_add(parent, name, NULL, user_data);

        return NULL;
     }

   switch (root->type)
     {
      case EET_G_UNKNOWN:
        me = cb->struct_alloc(root->name, user_data);

        for (it = root->values; it; it = it->next)
          eet_node_walk(me, it->name, it, cb, user_data);

        break;

      case EET_G_VAR_ARRAY:
      case EET_G_ARRAY:
        me = cb->array(root->type == EET_G_VAR_ARRAY ? EINA_TRUE : EINA_FALSE,
                       root->name, root->count, user_data);

        for (i = 0, it = root->values; it; it = it->next)
          cb->insert(me, i++, eet_node_walk(NULL,
                                            NULL,
                                            it,
                                            cb,
                                            user_data), user_data);

        break;

      case EET_G_LIST:
        me = cb->list(root->name, user_data);

        for (it = root->values; it; it = it->next)
          cb->append(me, eet_node_walk(NULL,
                                       NULL,
                                       it,
                                       cb,
                                       user_data), user_data);

        break;

      case EET_G_HASH:
        if (!parent)
          return NULL;

        return cb->hash(parent, root->name, root->key,
                        eet_node_walk(NULL,
                                      NULL,
                                      root->values,
                                      cb,
                                      user_data), user_data);

      case EET_T_STRING:
      case EET_T_INLINED_STRING:
      case EET_T_CHAR:
      case EET_T_SHORT:
      case EET_T_INT:
      case EET_T_LONG_LONG:
      case EET_T_FLOAT:
      case EET_T_DOUBLE:
      case EET_T_UCHAR:
      case EET_T_USHORT:
      case EET_T_UINT:
      case EET_T_ULONG_LONG:
        me = cb->simple(root->type, &root->data, user_data);
        break;
     }

   if (parent)
     cb->struct_add(parent, name, me, user_data);

   return me;
}

/**
 * @brief Initializes the Eet_Node subsystem.
 *
 * Sets up the mempool used for Eet_Node allocations. This function must be
 * called before any other `eet_node_*` functions are used.
 * The mempool type can be influenced by the `EINA_MEMPOOL` environment variable.
 *
 * @return 1 on success, 0 on failure (e.g., if mempool creation fails).
 * @see eet_node_shutdown()
 */
int
eet_node_init(void)
{
   const char *choice;
   const char *tmp;

#ifdef EINA_DEFAULT_MEMPOOL
   choice = "pass_through";
#else
   choice = "chained_mempool";
#endif
   tmp = getenv("EINA_MEMPOOL");
   if (tmp && tmp[0])
     choice = tmp;

   _eet_node_mp =
     eina_mempool_add(choice, "eet-node-alloc", NULL, sizeof(Eet_Node), 32);

   return _eet_node_mp ? 1 : 0;
}

/**
 * @brief Shuts down the Eet_Node subsystem.
 *
 * Deletes the mempool used for Eet_Node allocations. This function should be
 * called when Eet_Node functionality is no longer needed, typically during
 * application shutdown.
 *
 * @see eet_node_init()
 */
void
eet_node_shutdown(void)
{
   eina_mempool_del(_eet_node_mp);
   _eet_node_mp = NULL;
}

