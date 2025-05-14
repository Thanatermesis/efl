#ifndef __EO_LEXER_H__
#define __EO_LEXER_H__

#include <setjmp.h>

#include <Eina.h>
#include <Eolian.h>

#include "eolian_database.h"

/* a token is an int, custom tokens start at this - single-char tokens are
 * simply represented by their ascii */
#define START_CUSTOM 257 /**< Base value for custom token identifiers. */

/**
 * @enum Tokens
 * @brief Defines the set of custom tokens used by the lexer, beyond single ASCII characters.
 */

enum Tokens
{
   TOK_EQ = START_CUSTOM, TOK_NQ, TOK_GE, TOK_LE,
   TOK_AND, TOK_OR, TOK_LSH, TOK_RSH,

   TOK_DOC, TOK_STRING, TOK_CHAR, TOK_NUMBER, TOK_VALUE
};

/* all keywords in eolian, they can still be used as names (they're TOK_VALUE)
 * they just fill in the "kw" field of the token
 *
 * reserved for the future: @nullable
 */
/**
 * @def KEYWORDS
 * @brief Macro that defines all Eolian language keywords.
 * This macro uses helper macros KW(), KWAT(), and KWH() to categorize keywords.
 * - KW(x) for regular keywords (e.g., class, const).
 * - KWAT(x) for keywords prefixed with '@' (e.g., @beta, @nullable).
 * - KWH(x) for keywords prefixed with '#' (e.g., #version).
 * These are used to populate the ::Keywords enum.
 */
#define KEYWORDS KW(class), KW(const), KW(enum), KW(return), KW(struct), \
    \
    KW(abstract), KW(c_prefix), KW(composites), KW(constructor), KW(constructors), \
    KW(data), KW(destructor), KW(error), KW(event_c_prefix), KW(events), KW(extends), \
    KW(free), KW(get), KW(implements), KW(import), KW(interface), \
    KW(keys), KW(legacy), KW(methods), KW(mixin), KW(params), \
    KW(parse), KW(parts), KW(ptr), KW(set), KW(type), KW(values), KW(requires), \
    \
    KWAT(auto), KWAT(beta), KWAT(by_ref), KWAT(c_name), KWAT(const), \
    KWAT(empty), KWAT(extern), KWAT(free), KWAT(hot), KWAT(in), KWAT(inout), \
    KWAT(move), KWAT(no_unused), KWAT(nullable), KWAT(optional), KWAT(out), \
    KWAT(private), KWAT(property), KWAT(protected), KWAT(restart), \
    KWAT(pure_virtual), KWAT(static), \
    \
    KWH(version), \
    \
    KW(byte), KW(ubyte), KW(char), KW(short), KW(ushort), KW(int), KW(uint), \
    KW(long), KW(ulong), KW(llong), KW(ullong), \
    \
    KW(int8), KW(uint8), KW(int16), KW(uint16), KW(int32), KW(uint32), \
    KW(int64), KW(uint64), KW(int128), KW(uint128), \
    \
    KW(size), KW(ssize), KW(intptr), KW(uintptr), KW(ptrdiff), \
    \
    KW(time), \
    \
    KW(float), KW(double), \
    \
    KW(bool), \
    \
    KW(slice), KW(rw_slice), \
    \
    KW(void), \
    \
    KW(accessor), KW(array), KW(future), KW(iterator), KW(list), \
    KW(any_value), KW(any_value_ref), KW(binbuf), KW(event), \
    KW(mstring), KW(string), KW(stringshare), KW(strbuf), \
    \
    KW(hash), \
    KW(void_ptr), \
    KW(function), \
    KW(__undefined_type), \
    \
    KW(true), KW(false), KW(null)

/* "regular" keyword and @ prefixed keyword */
#define KW(x) KW_##x
#define KWAT(x) KW_at_##x /**< Helper macro to define '@' prefixed keywords for the ::Keywords enum. */
#define KWH(x) KW_hash_##x /**< Helper macro to define '#' prefixed keywords for the ::Keywords enum. */

/**
 * @enum Keywords
 * @brief Enumerates all keywords recognized by the Eolian lexer.
 * The values are generated using the #KEYWORDS macro. KW_UNKNOWN is a default/error value.
 */
enum Keywords
{
   KW_UNKNOWN = 0,
   KEYWORDS
};

#undef KW
#undef KWAT
#undef KWH

/**
 * @enum Numbers
 * @brief Enumerates the different C-like numerical types that can be parsed.
 * Used in Eo_Token to specify the kind of number parsed (e.g. int, float, long long).
 */
enum Numbers
{
   NUM_INT,
   NUM_UINT,
   NUM_LONG,
   NUM_ULONG,
   NUM_LLONG,
   NUM_ULLONG,
   NUM_FLOAT,
   NUM_DOUBLE
};

/**
 * @union Eo_Token_Union
 * @brief Holds the actual value associated with a token, if any.
 * The active member depends on the token type. For example, for a TOK_NUMBER,
 * one of the numerical types (i, u, l, ul, ll, ull, f, d) will be used.
 * For TOK_STRING or TOK_VALUE, 's' will point to a string.
 * For TOK_CHAR, 'c' will hold the character.
 * For TOK_DOC, 'doc' will point to the documentation structure.
 */
typedef union
{
   char               c;  /**< Value for a character token (TOK_CHAR). */
   const    char     *s; /**< String value for TOK_STRING, TOK_VALUE. */
   signed   int       i;  /**< Value for an integer number token. */
   unsigned int       u;  /**< Value for an unsigned integer number token. */
   signed   long      l;  /**< Value for a long integer number token. */
   unsigned long      ul; /**< Value for an unsigned long integer number token. */
   signed   long long ll; /**< Value for a long long integer number token. */
   unsigned long long ull;/**< Value for an unsigned long long integer number token. */
   float              f;  /**< Value for a float number token. */
   double             d;  /**< Value for a double number token. */
   Eolian_Documentation *doc; /**< Pointer to parsed documentation (TOK_DOC). */
} Eo_Token_Union;

/* a token - "token" is the actual token id, "value" is the value of a token
 * if needed - NULL otherwise - for example the value of a TOK_VALUE, "kw"
 * is the keyword id if this is a keyword, it's 0 when not a keyword */
/**
 * @struct _Eo_Token
 * @brief Represents a lexical token.
 * This structure holds the token type, an optional keyword identifier if the
 * token is a keyword, and the token's value (if any) in the `value` union.
 */
typedef struct _Eo_Token
{
   int token; /**< The token identifier (from ::Tokens or ASCII value). */
   int kw;    /**< Keyword identifier (from ::Keywords enum) if token is a keyword (TOK_VALUE), 0 otherwise. */
   Eo_Token_Union value; /**< The actual value of the token, if applicable. */
} Eo_Token;

/**
 * @struct _Lexer_Ctx
 * @brief Stores a snapshot of the lexer's state.
 * Used for saving and restoring the lexer's position and current token,
 * primarily for lookahead operations.
 */
typedef struct _Lexer_Ctx
{
   int line;            /**< Saved line number. */
   int column;          /**< Saved column number. */
   const char *linestr; /**< Pointer to the start of the saved line in the input stream. */
   Eo_Token token;       /**< Saved token. */
} Lexer_Ctx;

/**
 * @struct _Eo_Lexer_Dtor
 * @brief Represents a destructor entry for resource management.
 * Allows registering a callback function to free data, which can be
 * automatically called when the lexer is freed or when a destructor is popped.
 * This helps in managing resources in a scoped manner during parsing.
 */
typedef struct _Eo_Lexer_Dtor
{
   Eina_Free_Cb free_cb; /**< The callback function to free the associated data. */
   void *data;            /**< Pointer to the data that needs to be freed. */
} Eo_Lexer_Dtor;

/**
 * @struct _Eo_Lexer
 * @brief Holds the entire state of the Eolian lexer.
 * This structure contains all necessary information for the lexing process,
 * including input stream management, current token, lookahead token,
 * error handling, and resource management.
 */
typedef struct _Eo_Lexer
{
   int          current; /**< Current character being processed by the lexer. */

   int          column;      /**< Token-aware column number (start of current token). */
   int          icolumn;     /**< Character-aware column number (current character position). */

   int          line_number;  /**< Token-aware line number (start of current token). */
   int          iline_number; /**< Character-aware line number (current character position). */

   Eo_Token     t;           /**< The current token that has been lexed. */
   Eo_Token     lookahead;   /**< The next token (lookahead). Allows LL(2) parsing. -1 if not yet peeked. */

   Eina_Strbuf *buff;       /**< String buffer used for accumulating token contents (e.g., strings, numbers, identifiers). */

   Eina_File   *handle;      /**< Handle to the memory-mapped input file. */
   const char  *source;      /**< Full path to the source file being lexed. */
   const char  *filename;    /**< Basename of the source file. */

   const char  *stream;      /**< Pointer to the current character in the memory-mapped file. */
   const char  *stream_end;  /**< Pointer to the end of the memory-mapped file. */
   const char  *stream_line; /**< Pointer to the beginning of the current line in the input stream, for error reporting. */

   Eolian_State *state;      /**< Pointer to the global Eolian state. */
   Eolian_Unit  *unit;       /**< The Eolian unit (file context) being populated during parsing. */

   jmp_buf      err_jmp;     /**< Jump buffer for error handling (longjmp on error). */

   Eina_List   *saved_ctxs;  /**< List of saved lexer contexts (Lexer_Ctx), used for lookahead. */

   Eolian_Class *klass;      /**< Current class context, used for resolving in-class documentation references. */

   Eina_List   *dtors;       /**< List of destructors (Eo_Lexer_Dtor) for scoped resource management. */
   Eina_Hash   *nodes;       /**< Hash table for managing allocated Eolian objects (types, expressions, etc.) during parsing.
                               *   Objects are temporarily stored here until released into the Eolian database. */

   Eina_Bool    expr_mode;   /**< Flag indicating if expression-related tokens (e.g., ==, &&) should be lexed. */

   char         decpoint;    /**< Decimal point character, typically '.', but can be affected by locale for parsing numbers. */
} Eo_Lexer;

/**
 * @enum _Eo_Lexer_Error
 * @brief Defines types of errors that can occur during lexing.
 * Used as the return value for longjmp when an error is encountered.
 */
typedef enum _Eo_Lexer_Error
{
   EO_LEXER_ERROR_UNKNOWN = 0, /**< Unknown or unspecified error. */
   EO_LEXER_ERROR_NORMAL,
   EO_LEXER_ERROR_OOM          /**< Out of memory error. */
} Eo_Lexer_Error;

/**
 * @brief Initializes the lexer subsystem.
 * Sets up global resources like the keyword map. Must be called before any
 * lexing operations.
 */
void        eo_lexer_init           (void);

/**
 * @brief Shuts down the lexer subsystem.
 * Frees global resources allocated by eo_lexer_init().
 */
void        eo_lexer_shutdown       (void);

/**
 * @brief Creates a new lexer instance for a given source file.
 * @param state The Eolian state.
 * @param source The path to the source file to be lexed.
 * @return A new Eo_Lexer instance, or NULL on failure (e.g., file not found, OOM).
 *         On failure, an error is logged to the Eolian state.
 */
Eo_Lexer   *eo_lexer_new            (Eolian_State *state, const char *source);

/**
 * @brief Frees a lexer instance and its associated resources.
 * @param ls The lexer instance to free.
 */
void        eo_lexer_free           (Eo_Lexer *ls);

/**
 * @brief Gets the next token from the input stream.
 * This function advances the lexer's position. The current token's details
 * are stored in `ls->t`.
 * @param ls The lexer instance.
 * @return The token type (an ::Tokens enum value or an ASCII character code).
 *         Returns -1 on EOF or error.
 */
int         eo_lexer_get            (Eo_Lexer *ls);

/**
 * @brief Looks ahead one token without consuming it from the stream.
 * The lookahead token is stored in `ls->lookahead`. Subsequent calls to
 * eo_lexer_get() will return this lookahead token first.
 * @param ls The lexer instance.
 * @return The type of the lookahead token.
 */
int         eo_lexer_lookahead      (Eo_Lexer *ls);

/**
 * @brief Reports a lexing error with a custom message and associated token.
 * This function formats an error message, logs it, and then performs a longjmp
 * to the error handler set up in eo_lexer_new().
 * @param ls The lexer instance.
 * @param msg The error message.
 * @param token The token associated with the error (or 0 if none specific).
 */
void        eo_lexer_lex_error      (Eo_Lexer *ls, const char *msg, int token);

/**
 * @brief Reports a syntax error using the current token.
 * A convenience wrapper around eo_lexer_lex_error() that uses the
 * currently lexed token (`ls->t.token`) for context.
 * @param ls The lexer instance.
 * @param msg The error message.
 */
void        eo_lexer_syntax_error   (Eo_Lexer *ls, const char *msg);

/**
 * @brief Converts a token identifier to its string representation.
 * @param token The token identifier (from ::Tokens or ASCII value).
 * @param[out] buf Buffer to store the string representation. Should be large enough.
 *                 For example, "<string>" or the character itself for single char tokens.
 */
void        eo_lexer_token_to_str   (int token, char *buf);

/**
 * @brief Gets the string representation of a keyword.
 * @param kw The keyword identifier (from ::Keywords enum).
 * @return The string representation of the keyword (e.g., "class", "@beta").
 */
const char *eo_lexer_keyword_str_get(int kw);

/**
 * @brief Checks if a given keyword identifier represents a built-in type.
 * @param kw The keyword identifier.
 * @return EINA_TRUE if it's a type keyword (e.g., KW_int, KW_float), EINA_FALSE otherwise.
 */
Eina_Bool   eo_lexer_is_type_keyword(int kw);

/**
 * @brief Gets the keyword identifier from a keyword string.
 * @param kw The keyword string (e.g., "class", "@beta").
 * @return The keyword identifier (from ::Keywords enum), or KW_UNKNOWN if not found.
 */
int         eo_lexer_keyword_str_to_id(const char *kw);

/**
 * @brief Gets the C type name corresponding to a built-in Eolian type keyword.
 * @param kw The keyword identifier for a built-in type (e.g., KW_uint, KW_char).
 * @return The C type string (e.g., "unsigned int", "char"), or NULL if not a valid type keyword.
 */
const char *eo_lexer_get_c_type     (int kw);

/**
 * @brief Pushes the current lexer context (position, current token) onto a stack.
 * Used for lookahead operations or other situations requiring backtracking.
 * @param ls The lexer instance.
 */
void eo_lexer_context_push   (Eo_Lexer *ls);

/**
 * @brief Pops the last saved lexer context from the stack and discards it.
 * @param ls The lexer instance.
 */
void eo_lexer_context_pop    (Eo_Lexer *ls);

/**
 * @brief Restores the lexer state from the top of the context stack without popping it.
 * The lexer's current position and token are set to the values from the saved context.
 * @param ls The lexer instance.
 */
void eo_lexer_context_restore(Eo_Lexer *ls);

/**
 * @brief Clears all saved lexer contexts from the stack.
 * @param ls The lexer instance.
 */
void eo_lexer_context_clear  (Eo_Lexer *ls);

/**
 * @brief Allocates a new Eolian object node managed by the lexer.
 * These objects are tracked by the lexer and automatically freed if not
 * explicitly released (e.g., on parsing error).
 * @param ls The lexer instance.
 * @param objsize The size of the Eolian object to allocate.
 * @return A pointer to the allocated Eolian_Object, or triggers an OOM error via longjmp.
 */
Eolian_Object *eo_lexer_node_new(Eo_Lexer *ls, size_t objsize);

/**
 * @brief Releases an Eolian object from the lexer's management.
 * This indicates the object is now managed elsewhere (e.g., added to the database)
 * and should not be freed by the lexer's cleanup.
 * @param ls The lexer instance.
 * @param obj The Eolian object to release.
 * @return The released Eolian_Object.
 */
Eolian_Object *eo_lexer_node_release(Eo_Lexer *ls, Eolian_Object *obj);

/** @brief Convenience wrapper to allocate a new Eolian_Type node. */
static inline Eolian_Type *
eo_lexer_type_new(Eo_Lexer *ls)
{
   return (Eolian_Type *)eo_lexer_node_new(ls, sizeof(Eolian_Type));
}

/** @brief Convenience wrapper to release an Eolian_Type node. */
static inline Eolian_Type *
eo_lexer_type_release(Eo_Lexer *ls, Eolian_Type *tp)
{
   return (Eolian_Type *)eo_lexer_node_release(ls, (Eolian_Object *)tp);
}

/** @brief Convenience wrapper to allocate a new Eolian_Typedecl node. */
static inline Eolian_Typedecl *
eo_lexer_typedecl_new(Eo_Lexer *ls)
{
   return (Eolian_Typedecl *)eo_lexer_node_new(ls, sizeof(Eolian_Typedecl));
}

/** @brief Convenience wrapper to release an Eolian_Typedecl node. */
static inline Eolian_Typedecl *
eo_lexer_typedecl_release(Eo_Lexer *ls, Eolian_Typedecl *tp)
{
   return (Eolian_Typedecl *)eo_lexer_node_release(ls, (Eolian_Object *)tp);
}

/** @brief Convenience wrapper to allocate a new Eolian_Constant node. */
static inline Eolian_Constant *
eo_lexer_constant_new(Eo_Lexer *ls)
{
   return (Eolian_Constant *)eo_lexer_node_new(ls, sizeof(Eolian_Constant));
}

/** @brief Convenience wrapper to release an Eolian_Constant node. */
static inline Eolian_Constant *
eo_lexer_constant_release(Eo_Lexer *ls, Eolian_Constant *var)
{
   return (Eolian_Constant *)eo_lexer_node_release(ls, (Eolian_Object *)var);
}

/** @brief Convenience wrapper to allocate a new Eolian_Expression node. */
static inline Eolian_Expression *
eo_lexer_expr_new(Eo_Lexer *ls)
{
   return (Eolian_Expression *)eo_lexer_node_new(ls, sizeof(Eolian_Expression));
}

/** @brief Convenience wrapper to release an Eolian_Expression node. */
static inline Eolian_Expression *
eo_lexer_expr_release(Eo_Lexer *ls, Eolian_Expression *expr)
{
   return (Eolian_Expression *)eo_lexer_node_release(ls, (Eolian_Object *)expr);
}

/**
 * @brief Convenience wrapper to ref and then release an Eolian_Expression node.
 * This is useful when an expression is created and immediately used by another
 * structure that takes ownership, while the lexer also needs to release its own
 * initial reference from `eo_lexer_node_new`.
 */
static inline Eolian_Expression *
eo_lexer_expr_release_ref(Eo_Lexer *ls, Eolian_Expression *expr)
{
   eolian_object_ref(&expr->base);
   return eo_lexer_expr_release(ls, expr);
}

/** @brief Convenience wrapper to allocate a new Eolian_Error node. */
static inline Eolian_Error *
eo_lexer_error_new(Eo_Lexer *ls)
{
   return (Eolian_Error *)eo_lexer_node_new(ls, sizeof(Eolian_Error));
}

/** @brief Convenience wrapper to release an Eolian_Error node. */
static inline Eolian_Error *
eo_lexer_error_release(Eo_Lexer *ls, Eolian_Error *err)
{
   return (Eolian_Error *)eo_lexer_node_release(ls, (Eolian_Object *)err);
}

/**
 * @brief Pushes a destructor function onto the lexer's destructor stack.
 * This is used for RAII-like resource management. If an error occurs (via longjmp),
 * any destructors remaining on the stack when `eo_lexer_free` is called will be executed.
 * @param ls The lexer instance.
 * @param free_cb The callback function to execute for cleanup.
 * @param data The data to pass to the callback function.
 */
void eo_lexer_dtor_push(Eo_Lexer *ls, Eina_Free_Cb free_cb, void *data);

/**
 * @brief Pops a destructor from the lexer's destructor stack and executes it.
 * This is called when a resource is successfully managed and cleaned up normally.
 * @param ls The lexer instance.
 */
void eo_lexer_dtor_pop(Eo_Lexer *ls);


#endif /* __EO_LEXER_H__ */
