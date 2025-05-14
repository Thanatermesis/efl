enum node_type;

/**
 * @union hashval
 * @brief Represents the different types of values a hash node can hold.
 *
 * This union allows a hash node to store an integer, a character pointer, or a
 * pointer to a macro definition, depending on the node's type.
 */
union hashval {
   int                 ival;   /**< Integer value, for T_CONST nodes. */
   char               *cpval; /**< Character pointer value. */
   DEFINITION         *defn;  /**< Pointer to a macro definition, for T_MACRO nodes. */
};

/**
 * @struct hashnode
 * @brief Represents a node in the hash table.
 *
 * Each node stores a name (e.g., a macro name), its value, and metadata.
 * Nodes in the same hash bucket are stored in a doubly-linked list for
 * efficient insertion and deletion.
 */
struct hashnode {
   struct hashnode    *next;	/**< Pointer to the next node in the same bucket. */
   struct hashnode    *prev;	/**< Pointer to the previous node in the same bucket. */
   /**
    * @brief Pointer to the head of the hash bucket's chain.
    * This allows for efficient updates to the chain head when a node is deleted,
    * especially if it's the first node in the bucket.
    */
   struct hashnode   **bucket_hdr;
   enum node_type      type;	/**< The type of the token (e.g., T_MACRO, T_CONST). */
   int                 length;	/**< The length of the name, for quick comparisons. */
   char               *name;	/**< The name of the token (e.g., macro name). */
   union hashval       value;	/**< The value associated with the token. */
};

typedef struct hashnode HASHNODE;

/* Some definitions for the hash table.  The hash function MUST be
   computed as shown in hashf () below.  That is because the rescan
   loop computes the hash value `on the fly' for most tokens,
   in order to avoid the overhead of a lot of procedure calls to
   the hashf () function.  Hashf () only exists for the sake of
   politeness, for use when speed isn't so important. */

#define HASHSIZE 1403
#define HASHSTEP(old, c) ((old << 2) + c)
#define MAKE_POS(v) (v & 0x7fffffff)	/* make number positive */

/**
 * @brief Computes a hash value for a given string.
 *
 * This hash function must be compatible with the HASHSTEP macro, which is used
 * for on-the-fly hash computation during token scanning to improve performance.
 *
 * @param name The string to hash.
 * @param len The length of the string.
 * @param hashsize The size of the hash table, used for the modulo operation.
 * @return The computed hash value.
 */
extern int          hashf(const char *name, int len, int hashsize);
/**
 * @brief Looks up a name in the hash table.
 *
 * Finds the most recent entry for a given name. The name is treated as an
 * identifier and is terminated by the first non-identifier character if its
 * length is not provided.
 *
 * @param name The name to look up.
 * @param len The length of the name. If negative, the length is computed by
 *            scanning for the first non-identifier character.
 * @param hash The precomputed hash value. If negative, the hash is computed
 *             by this function.
 * @return A pointer to the HASHNODE if found, otherwise NULL.
 */
extern HASHNODE    *cpp_lookup(const char *name, int len, int hash);
/**
 * @brief Deletes a node from the hash table.
 *
 * This function removes a hash node from its bucket's linked list and frees
 * associated memory. For macro nodes (T_MACRO), it also frees the memory
 * used by the macro definition, its pattern, and arguments.
 *
 * @note For stability, the `DEFINITION` struct of a macro is not freed. This
 * prevents a crash if a macro is undefined while it is being expanded.
 *
 * @param hp Pointer to the HASHNODE to be deleted.
 */
extern void         delete_macro(HASHNODE * hp);
/**
 * @brief Installs a new name in the main hash table.
 *
 * A new node is created and added to the hash table. If a node with the same
 * name already exists, the new node is inserted before it, effectively
 * shadowing the old one. This is important for handling macro redefinitions
 * and `defined` operator semantics.
 *
 * @param name The name to install.
 * @param len The length of the name. If negative, it's computed by scanning.
 * @param type The type of the node (e.g., T_MACRO).
 * @param ivalue The integer value, used if type is T_CONST.
 * @param value The character pointer value for other types.
 * @param hash The precomputed hash value. If negative, it's computed.
 * @return A pointer to the newly created HASHNODE.
 */
extern HASHNODE    *install(const char *name, int len, enum node_type type,
			    int ivalue, char *value, int hash);
/**
 * @brief Frees resources used by the hash table.
 *
 * This function should be called when the hash table is no longer needed to
 * release memory. It is intended to free all nodes in the table.
 *
 * @note The current implementation only frees the head of each hash bucket's
 * chain, which can lead to memory leaks if a bucket contains more than one node.
 *
 * @param pfile The cpp_reader context (currently unused).
 */
extern void         cpp_hash_cleanup(cpp_reader * pfile);
