/**
 * @file
 * @brief Implements the Eolian parser.
 *
 * This file contains the logic for parsing Eolian (.eo and .eot) files.
 * It uses a lexer (eo_lexer.c) to tokenize the input and then recursively
 * descends through the grammar to build up Eolian objects like classes,
 * types, methods, properties, constants, and events. These objects are
 * then added to the Eolian database.
 *
 * The parser handles various language constructs, including:
 * - Versioning directives (#version)
 * - Imports (import, parse)
 * - Type definitions (type, struct, enum, function pointers)
 * - Constants (const)
 * - Errors (error)
 * - Class definitions (class, abstract, mixin, interface)
 *   - Inheritance (extends, implements)
 *   - Composition (composites)
 *   - Requirements (requires for mixins)
 *   - C prefixes (c_prefix, event_c_prefix)
 *   - Data types (data)
 *   - Methods (methods, @property)
 *   - Parts (parts)
 *   - Implements (implements)
 *   - Constructors (constructors)
 *   - Events (events)
 * - Expressions (for constant values, default parameter values, etc.)
 *
 * Error handling is done via setjmp/longjmp to unwind the parsing stack
 * upon encountering a syntax error.
 */
#include <assert.h>
#include <limits.h>

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "eo_parser.h"
#include "eolian_priv.h"

#define CASE_LOCK(ls, var, msg) \
   if (has_##var) \
     eo_lexer_syntax_error(ls, "double " msg); \
   has_##var = EINA_TRUE;

#define FILL_BASE(exp, ls, l, c, tp) \
   (exp).unit = ls->unit; \
   (exp).file = eina_stringshare_ref(ls->filename); \
   (exp).line = l; \
   (exp).column = c; \
   (exp).type = EOLIAN_OBJECT_##tp

#define FILL_DOC(ls, def, docf) \
   if (ls->t.token == TOK_DOC) \
     { \
        def->docf = ls->t.value.doc; \
        ls->t.value.doc = NULL; \
        eo_lexer_get(ls); \
     }

/**
 * @brief Reports a syntax error indicating an expected token was not found.
 * @param ls The lexer state.
 * @param token The token that was expected.
 */
static void
error_expected(Eo_Lexer *ls, int token)
{
   char  buf[256 + 128];
   char tbuf[256];
   eo_lexer_token_to_str(token, tbuf);
   snprintf(buf, sizeof(buf), "'%s' expected", tbuf);
   eo_lexer_syntax_error(ls, buf);
}

/**
 * @brief Tests if the current token matches the given token and advances the lexer if it does.
 * @param ls The lexer state.
 * @param token The token to test against.
 * @return EINA_TRUE if the current token matches and the lexer was advanced, EINA_FALSE otherwise.
 */
static Eina_Bool
test_next(Eo_Lexer *ls, int token)
{
   if (ls->t.token == token)
     {
        eo_lexer_get(ls);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

/**
 * @brief Checks if the current token matches the given token, raising a syntax error if not.
 * @param ls The lexer state.
 * @param token The token to check for.
 */
static void
check(Eo_Lexer *ls, int token)
{
   if (ls->t.token != token)
     error_expected(ls, token);
}

/**
 * @brief Checks if the current token's keyword matches the given keyword, raising a syntax error if not.
 * @param ls The lexer state.
 * @param kw The keyword to check for.
 */
static void
check_kw(Eo_Lexer *ls, int kw)
{
   if (ls->t.kw != kw)
     error_expected(ls, TOK_VALUE + kw);
}

/**
 * @brief Checks if the current token matches the given token, advances the lexer, raising a syntax error if not.
 * @param ls The lexer state.
 * @param token The token to check for.
 */
static void
check_next(Eo_Lexer *ls, int token)
{
   check(ls, token);
   eo_lexer_get(ls);
}

/**
 * @brief Checks if the current token's keyword matches, advances the lexer, raising a syntax error if not.
 * @param ls The lexer state.
 * @param kw The keyword to check for.
 */
static void
check_kw_next(Eo_Lexer *ls, int kw)
{
   check_kw(ls, kw);
   eo_lexer_get(ls);
}

/**
 * @brief Checks if the current token matches 'what', intended to close 'who' opened at 'where':'col'.
 *
 * Provides a more informative error message if the token doesn't match,
 * indicating what was expected and where the opening token was.
 * @param ls The lexer state.
 * @param what The expected closing token (e.g., ')', '}').
 * @param who The opening token that 'what' is supposed to close (e.g., '(', '{').
 * @param where The line number where 'who' was encountered.
 * @param col The column number where 'who' was encountered.
 */
static void
check_match(Eo_Lexer *ls, int what, int who, int where, int col)
{
   if (!test_next(ls, what))
     {
        if (where == ls->line_number)
          error_expected(ls, what);
        else
          {
             char  buf[256 + 256 + 128];
             char tbuf[256];
             char vbuf[256];
             eo_lexer_token_to_str(what, tbuf);
             eo_lexer_token_to_str(who , vbuf);
             snprintf(buf, sizeof(buf),
                      "'%s' expected (to close '%s' at line %d, column %d)",
                      tbuf, vbuf, where, col);
             eo_lexer_syntax_error(ls, buf);
          }
     }
}

/**
 * @brief Converts a fully qualified Eolian name (e.g., "My.Object.Name") to a C-style name.
 *
 * Dots ('.') are replaced with underscores ('_').
 * @param fulln The fully qualified Eolian name. Example: "Efl.Ui.Widget.text_set"
 * @return A new Eina_Stringshare containing the C-style name. Example: "Efl_Ui_Widget_text_set"
 *         The caller owns a reference to the returned stringshare.
 */
static Eina_Stringshare *
make_c_name(const char *fulln)
{
   char *mbuf = strdup(fulln);
   for (char *p = mbuf; (p = strchr(p, '.')); ++p)
     *p = '_';
   Eina_Stringshare *ret = eina_stringshare_add(mbuf);
   free(mbuf);
   return ret;
}

/**
 * @brief Compares two filenames for equality.
 * @param fn1 The first filename.
 * @param fn2 The second filename.
 * @return EINA_TRUE if the filenames are identical, EINA_FALSE otherwise.
 */
static Eina_Bool
compare_class_file(const char *fn1, const char *fn2)
{
   return !strcmp(fn1, fn2);
}

/**
 * @brief Retrieves an Eolian declaration (class, typedecl, constant) by name.
 *
 * Searches first in the main unit's objects, then in the staging unit's objects.
 * @param ls The lexer state, used to access the Eolian state.
 * @param name The name of the declaration to find.
 * @return A pointer to the Eolian_Object if found and is of a valid type, NULL otherwise.
 */
static Eolian_Object *
_eolian_decl_get(Eo_Lexer *ls, const char *name)
{
   Eolian_Object *obj = eina_hash_find(ls->state->main.unit.objects, name);
   if (!obj)
     obj = eina_hash_find(ls->state->staging.unit.objects, name);
   if (obj && ((obj->type == EOLIAN_OBJECT_CLASS) ||
               (obj->type == EOLIAN_OBJECT_TYPEDECL) ||
               (obj->type == EOLIAN_OBJECT_CONSTANT)))
     return obj;

   return NULL;
}

/**
 * @brief Gets a human-readable string representation of an Eolian object's declaration type.
 * @param obj The Eolian object.
 * @return A string describing the object's declaration type (e.g., "class", "type alias", "constant").
 *         Returns "unknown" if the type is not recognized.
 */
static const char *
_eolian_decl_name_get(Eolian_Object *obj)
{
   switch (obj->type)
     {
      case EOLIAN_OBJECT_CLASS:
        return "class";
      case EOLIAN_OBJECT_TYPEDECL:
        switch (((Eolian_Typedecl *)obj)->type)
          {
           case EOLIAN_TYPEDECL_ALIAS:
             return "type alias";
           case EOLIAN_TYPEDECL_STRUCT:
           case EOLIAN_TYPEDECL_STRUCT_OPAQUE:
             return "struct";
           case EOLIAN_TYPEDECL_ENUM:
             return "enum";
           default:
             break;
          }
        goto end;
      case EOLIAN_OBJECT_CONSTANT:
        return "constant";
      default:
        break;
     }
end:
   return "unknown";
}

/**
 * @brief Reports a syntax error for redefinition of an Eolian object.
 * @param ls The lexer state.
 * @param obj The original Eolian object that was defined.
 * @param nobj The new Eolian object that is causing the redefinition.
 */
static void
redef_error(Eo_Lexer *ls, Eolian_Object *obj, Eolian_Object *nobj)
{
   char buf[256 + 128], fbuf[256] = { '\0' };
   if (ls->filename != obj->file)
     snprintf(fbuf, sizeof(fbuf), "%s:%d:%d", obj->file, obj->line, obj->column);
   else
     snprintf(fbuf, sizeof(fbuf), "%d:%d", obj->line, obj->column);

   if (nobj->type != obj->type)
     snprintf(buf, sizeof(buf), "%s '%s' redefined as %s (originally at %s)",
              _eolian_decl_name_get(obj), obj->name,
              _eolian_decl_name_get(nobj), fbuf);
   else
     snprintf(buf, sizeof(buf), "%s '%s' redefined (originally at %s)",
              _eolian_decl_name_get(obj), obj->name, fbuf);

   eo_lexer_syntax_error(ls, buf);
}

/**
 * @brief Parses a potentially qualified name (e.g., "Foo.Bar.Baz" or "MyName").
 *
 * Appends the parsed name to the provided string buffer.
 * Expects the lexer to be positioned at the start of the name.
 * Consumes tokens forming the name.
 * @param ls The lexer state.
 * @param buf The string buffer to append the parsed name to. It will be reset before appending.
 * @return The same string buffer `buf` containing the parsed name.
 *         Example: If input is "My.Identifier", buf will contain "My.Identifier".
 */
static Eina_Strbuf *
parse_name(Eo_Lexer *ls, Eina_Strbuf *buf)
{
   check(ls, TOK_VALUE);
   if (eo_lexer_is_type_keyword(ls->t.kw))
     eo_lexer_syntax_error(ls, "invalid name");
   eina_strbuf_reset(buf);
   for (;;)
     {
        eina_strbuf_append(buf, ls->t.value.s);
        eo_lexer_get(ls);
        if (ls->t.token != '.') break;
        eo_lexer_get(ls);
        eina_strbuf_append(buf, ".");
        check(ls, TOK_VALUE);
        if (eo_lexer_is_type_keyword(ls->t.kw))
          eo_lexer_syntax_error(ls, "invalid name");
     }
   return buf;
}

/**
 * @brief Parses a C name specified with the `@c_name("actual_c_name")` syntax.
 *
 * Expects the lexer to be positioned at the `@c_name` keyword.
 * Consumes the `@c_name`, '(', the C name string, and ')'.
 * @param ls The lexer state.
 * @return A new Eina_Stringshare containing the parsed C name.
 *         Example: If input is `@c_name("my_custom_c_identifier")`, returns "my_custom_c_identifier".
 *         The caller owns a reference to the returned stringshare. Returns NULL on parsing error (unreachable due to longjmp).
 */
static Eina_Stringshare *
parse_c_name(Eo_Lexer *ls)
{
   eo_lexer_get(ls);
   int pline = ls->line_number, pcol = ls->column;
   check_next(ls, '(');
   check(ls, TOK_VALUE);
   if (eo_lexer_is_type_keyword(ls->t.kw))
     eo_lexer_syntax_error(ls, "invalid name");
   Eina_Stringshare *cname = eina_stringshare_add(ls->t.value.s);
   eo_lexer_get(ls);
   if (ls->t.token != ')')
     {
        eina_stringshare_del(cname);
        check_match(ls, ')', '(', pline, pcol);
        return NULL; /* unreachable */
     }
   eo_lexer_get(ls);
   return cname;
}

/**
 * @brief Converts a token to its corresponding Eolian_Binary_Operator enum value.
 * @param tok The token representing a binary operator (e.g., '+', TOK_EQ, TOK_AND).
 * @return The Eolian_Binary_Operator enum value, or EOLIAN_BINOP_INVALID if the token is not a recognized binary operator.
 */
static Eolian_Binary_Operator
get_binop_id(int tok)
{
   switch (tok)
     {
      case '+': return EOLIAN_BINOP_ADD;
      case '-': return EOLIAN_BINOP_SUB;
      case '*': return EOLIAN_BINOP_MUL;
      case '/': return EOLIAN_BINOP_DIV;
      case '%': return EOLIAN_BINOP_MOD;

      case TOK_EQ: return EOLIAN_BINOP_EQ;
      case TOK_NQ: return EOLIAN_BINOP_NQ;
      case '>'   : return EOLIAN_BINOP_GT;
      case '<'   : return EOLIAN_BINOP_LT;
      case TOK_GE: return EOLIAN_BINOP_GE;
      case TOK_LE: return EOLIAN_BINOP_LE;

      case TOK_AND: return EOLIAN_BINOP_AND;
      case TOK_OR : return EOLIAN_BINOP_OR;

      case '&': return EOLIAN_BINOP_BAND;
      case '|': return EOLIAN_BINOP_BOR;
      case '^': return EOLIAN_BINOP_BXOR;

      case TOK_LSH: return EOLIAN_BINOP_LSH;
      case TOK_RSH: return EOLIAN_BINOP_RSH;

      default: return EOLIAN_BINOP_INVALID;
     }
}

static Eolian_Unary_Operator
get_unop_id(int tok)
{
   switch (tok)
     {
      case '-': return EOLIAN_UNOP_UNM;
      case '+': return EOLIAN_UNOP_UNP;
      case '!': return EOLIAN_UNOP_NOT;
      case '~': return EOLIAN_UNOP_BNOT;

      default: return EOLIAN_UNOP_INVALID;
     }
}

static const int binprec[] = {
   -1, /* invalid */

   8, /* + */
   8, /* - */
   9, /* * */
   9, /* / */
   9, /* % */

   3, /* == */
   3, /* != */
   3, /* >  */
   3, /* <  */
   3, /* >= */
   3, /* <= */

   2, /* && */
   1, /* || */

   6, /* &  */
   4, /* |  */
   5, /* ^  */
   7, /* << */
   7  /* >> */
};

/** @brief Precedence level for unary operators. */
#define UNARY_PRECEDENCE 10

static Eolian_Expression *parse_expr_bin(Eo_Lexer *ls, int min_prec);
static Eolian_Expression *parse_expr(Eo_Lexer *ls);

/**
 * @brief Parses a simple expression component.
 *
 * This handles literals (numbers, strings, chars, booleans, null),
 * named identifiers (constants, enum fields), unary operations, and
 * parenthesized expressions.
 * @param ls The lexer state.
 * @return A new Eolian_Expression representing the parsed simple expression.
 *         The caller receives a new reference that it must manage (typically by passing to eo_lexer_expr_release_ref).
 */
static Eolian_Expression *
parse_expr_simple(Eo_Lexer *ls)
{
   Eolian_Expression *expr;
   Eolian_Unary_Operator unop = get_unop_id(ls->t.token);
   if (unop != EOLIAN_UNOP_INVALID)
     {
        int line = ls->line_number, col = ls->column;
        eo_lexer_get(ls);
        Eolian_Expression *exp = parse_expr_bin(ls, UNARY_PRECEDENCE);
        expr = eo_lexer_expr_new(ls);
        FILL_BASE(expr->base, ls, line, col, EXPRESSION);
        expr->unop = unop;
        expr->type = EOLIAN_EXPR_UNARY;
        expr->expr = exp;
        eo_lexer_expr_release_ref(ls, exp);
        return expr;
     }
   switch (ls->t.token)
     {
      case TOK_NUMBER:
        {
           int line = ls->line_number, col = ls->column;
           expr = eo_lexer_expr_new(ls);
           FILL_BASE(expr->base, ls, line, col, EXPRESSION);
           expr->type = ls->t.kw + 1; /* map Numbers from lexer to expr type */
           memcpy(&expr->value, &ls->t.value, sizeof(expr->value));
           eo_lexer_get(ls);
           break;
        }
      case TOK_STRING:
        {
           int line = ls->line_number, col = ls->column;
           expr = eo_lexer_expr_new(ls);
           FILL_BASE(expr->base, ls, line, col, EXPRESSION);
           expr->type = EOLIAN_EXPR_STRING;
           expr->value.s = eina_stringshare_ref(ls->t.value.s);
           eo_lexer_get(ls);
           break;
        }
      case TOK_CHAR:
        {
           int line = ls->line_number, col = ls->column;
           expr = eo_lexer_expr_new(ls);
           FILL_BASE(expr->base, ls, line, col, EXPRESSION);
           expr->type = EOLIAN_EXPR_CHAR;
           expr->value.c = ls->t.value.c;
           eo_lexer_get(ls);
           break;
        }
      case TOK_VALUE:
        {
           int line = ls->line_number, col = ls->column;
           switch (ls->t.kw)
             {
              case KW_true:
              case KW_false:
                {
                   expr = eo_lexer_expr_new(ls);
                   expr->type = EOLIAN_EXPR_BOOL;
                   expr->value.b = (ls->t.kw == KW_true);
                   eo_lexer_get(ls);
                   break;
                }
              case KW_null:
                {
                   expr = eo_lexer_expr_new(ls);
                   expr->type = EOLIAN_EXPR_NULL;
                   eo_lexer_get(ls);
                   break;
                }
              default:
                {
                   Eina_Strbuf *buf = eina_strbuf_new();
                   eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
                   expr = eo_lexer_expr_new(ls);
                   expr->type = EOLIAN_EXPR_NAME;
                   parse_name(ls, buf);
                   expr->value.s = eina_stringshare_add(eina_strbuf_string_get
                       (buf));
                   eo_lexer_dtor_pop(ls);
                   break;
                }
             }
           FILL_BASE(expr->base, ls, line, col, EXPRESSION);
           break;
        }
      case '(':
        {
           int line = ls->line_number, col = ls->column;
           eo_lexer_get(ls);
           expr = parse_expr(ls);
           check_match(ls, ')', '(', line, col);
           break;
        }
      default:
        expr = NULL; /* shut up compiler */
        eo_lexer_syntax_error(ls, "unexpected symbol");
        break;
     }

   return expr;
}

/**
 * @brief Parses a binary expression using precedence climbing.
 *
 * This function recursively parses expressions, handling binary operators
 * according to their precedence and associativity.
 * @param ls The lexer state.
 * @param min_prec The minimum precedence level for operators to be considered at this level of recursion.
 * @return A new Eolian_Expression representing the parsed binary (or simple, if no operators apply) expression.
 *         The caller receives a new reference that it must manage.
 */
static Eolian_Expression *
parse_expr_bin(Eo_Lexer *ls, int min_prec)
{
   int line = ls->line_number, col = ls->column;
   Eolian_Expression *lhs = parse_expr_simple(ls);
   for (;;)
     {
        Eolian_Expression *rhs, *bin;
        Eolian_Binary_Operator op = get_binop_id(ls->t.token);
        int prec = binprec[op];
        if ((op == EOLIAN_BINOP_INVALID) || (prec < 0) || (prec < min_prec))
          break;
        eo_lexer_get(ls);
        rhs = parse_expr_bin(ls, prec + 1);
        bin = eo_lexer_expr_new(ls);
        FILL_BASE(bin->base, ls, line, col, EXPRESSION);
        bin->binop = op;
        bin->type = EOLIAN_EXPR_BINARY;
        bin->lhs = lhs;
        eo_lexer_expr_release_ref(ls, lhs);
        bin->rhs = rhs;
        eo_lexer_expr_release_ref(ls, rhs);
        lhs = bin;
     }
   return lhs;
}

/**
 * @brief Parses a full expression.
 *
 * This is the main entry point for expression parsing, starting with the lowest precedence.
 * @param ls The lexer state.
 * @return A new Eolian_Expression representing the parsed expression.
 *         The caller receives a new reference that it must manage.
 */
static Eolian_Expression *
parse_expr(Eo_Lexer *ls)
{
   return parse_expr_bin(ls, 1);
}

static Eolian_Type *parse_type_void(Eo_Lexer *ls, Eina_Bool allow_ptr, Eina_Bool allow_const);

/**
 * @brief Parses a type name, ensuring it is not 'void'.
 *
 * Wraps parse_type_void and raises an error if 'void' is parsed.
 * @param ls The lexer state.
 * @param allow_ptr Whether pointer types (ptr()) are allowed.
 * @param allow_const Whether const qualifiers (const()) are allowed.
 * @return A new Eolian_Type representing the parsed non-void type.
 *         The caller receives a new reference that it must manage.
 */
static Eolian_Type *
parse_type(Eo_Lexer *ls, Eina_Bool allow_ptr, Eina_Bool allow_const)
{
   Eolian_Type *ret;
   eo_lexer_context_push(ls);
   ret = parse_type_void(ls, allow_ptr, allow_const);
   if (ret->type == EOLIAN_TYPE_VOID)
     {
        eo_lexer_context_restore(ls);
        eo_lexer_syntax_error(ls, "non-void type expected");
     }
   eo_lexer_context_pop(ls);
   return ret;
}

/**
 * @brief Frees an Eolian_Struct_Type_Field object.
 * @param def The struct field to free.
 */
static void
_struct_field_free(Eolian_Struct_Type_Field *def)
{
   eina_stringshare_del(def->base.file);
   eina_stringshare_del(def->base.name);
   database_type_del(def->type);
   database_doc_del(def->doc);
   free(def);
}

/**
 * @brief Parses a struct definition.
 *
 * Handles struct fields, their types, and optional @by_ref/@move qualifiers.
 * Also handles documentation for the struct and its fields.
 *
 * Example Eolian syntax:
 * @code
 * struct My_Struct @beta {
 *   doc: "A cool struct.";
 *   field1: int; // A field
 *   field2: string @move;
 * }
 * @endcode
 *
 * @param ls The lexer state.
 * @param name The name of the struct.
 * @param is_extern Whether the struct is marked @extern.
 * @param is_beta Whether the struct is marked @beta.
 * @param line The line number where the struct keyword was found.
 * @param column The column number where the struct keyword was found.
 * @param freefunc Optional C function name for freeing the struct (from @free).
 * @param cname Optional C name for the struct (from @c_name).
 * @return A new Eolian_Typedecl representing the parsed struct.
 *         The caller receives a new reference that it must manage.
 */
static Eolian_Typedecl *
parse_struct(Eo_Lexer *ls, const char *name, Eina_Bool is_extern,
             Eina_Bool is_beta, int line, int column, const char *freefunc,
             const char *cname)
{
   int bline = ls->line_number, bcolumn = ls->column;
   Eolian_Typedecl *def = eo_lexer_typedecl_new(ls);
   def->is_extern = is_extern;
   def->base.is_beta = is_beta;
   def->base.name = name;
   def->type = EOLIAN_TYPEDECL_STRUCT;
   def->fields = eina_hash_string_small_new(EINA_FREE_CB(_struct_field_free));
   if (freefunc)
     {
        def->freefunc = eina_stringshare_ref(freefunc);
        def->ownable = EINA_TRUE;
     }
   if (cname)
     def->base.c_name = eina_stringshare_ref(cname);
   else
     def->base.c_name = make_c_name(name);
   /* we can't know the order, pop when both are filled */
   if (freefunc && cname)
     {
        eo_lexer_dtor_pop(ls);
        eo_lexer_dtor_pop(ls);
     }
   else if (freefunc || cname)
     eo_lexer_dtor_pop(ls);
   check_next(ls, '{');
   FILL_DOC(ls, def, doc);
   while (ls->t.token != '}')
     {
        const char *fname;
        Eolian_Struct_Type_Field *fdef;
        Eolian_Type *tp;
        int fline = ls->line_number, fcol = ls->column;
        check(ls, TOK_VALUE);
        if (eina_hash_find(def->fields, ls->t.value.s))
          eo_lexer_syntax_error(ls, "double field definition");
        fdef = calloc(1, sizeof(Eolian_Struct_Type_Field));
        fname = eina_stringshare_ref(ls->t.value.s);
        eina_hash_add(def->fields, fname, fdef);
        def->field_list = eina_list_append(def->field_list, fdef);
        eolian_object_ref(&fdef->base);
        eo_lexer_get(ls);
        check_next(ls, ':');
        tp = parse_type(ls, EINA_TRUE, EINA_TRUE);
        FILL_BASE(fdef->base, ls, fline, fcol, STRUCT_FIELD);
        fdef->type = eo_lexer_type_release(ls, tp);
        fdef->base.name = eina_stringshare_ref(fname);
        Eina_Bool has_move = EINA_FALSE, has_by_ref = EINA_FALSE;
        for (;;) switch (ls->t.kw)
          {
           case KW_at_by_ref:
             CASE_LOCK(ls, by_ref, "by_ref qualifier");
             fdef->by_ref = EINA_TRUE;
             eo_lexer_get(ls);
             break;
           case KW_at_move:
             CASE_LOCK(ls, move, "move qualifier");
             fdef->move = EINA_TRUE;
             eo_lexer_get(ls);
             break;
           default:
             goto qual_end;
          }
qual_end:
        check_next(ls, ';');
        FILL_DOC(ls, fdef, doc);
        if (def->doc && fdef->doc && def->doc->since && !fdef->doc->since)
          fdef->doc->since = eina_stringshare_ref (def->doc->since);
     }
   check_match(ls, '}', '{', bline, bcolumn);
   FILL_BASE(def->base, ls, line, column, TYPEDECL);
   database_struct_add(ls->unit, eo_lexer_typedecl_release(ls, def));
   return def;
}

/**
 * @brief Frees an Eolian_Enum_Type_Field object.
 * @param def The enum field to free.
 */
static void
_enum_field_free(Eolian_Enum_Type_Field *def)
{
   eina_stringshare_del(def->base.file);
   eina_stringshare_del(def->base.name);
   database_expr_del(def->value);
   database_doc_del(def->doc);
   free(def);
}

/**
 * @brief Parses an enum definition.
 *
 * Handles enum fields, their optional explicit values, and documentation.
 * If values are not explicit, they are auto-incremented from the previous field
 * or from 0 for the first field.
 * Also handles legacy enum mapping.
 *
 * Example Eolian syntax:
 * @code
 * enum My_Enum @beta {
 *   doc: "An enumeration.";
 *   legacy: "MY_PFX"; // Optional legacy prefix
 *   FIELD_A,          // Value will be 0
 *   FIELD_B = 5,      // Value will be 5
 *   FIELD_C           // Value will be 6 (5+1)
 * }
 * @endcode
 *
 * @param ls The lexer state.
 * @param name The name of the enum.
 * @param is_extern Whether the enum is marked @extern.
 * @param is_beta Whether the enum is marked @beta.
 * @param line The line number where the enum keyword was found.
 * @param column The column number where the enum keyword was found.
 * @param cname Optional C name for the enum (from @c_name).
 * @return A new Eolian_Typedecl representing the parsed enum.
 *         The caller receives a new reference that it must manage.
 */
static Eolian_Typedecl *
parse_enum(Eo_Lexer *ls, const char *name, Eina_Bool is_extern,
           Eina_Bool is_beta, int line, int column, const char *cname)
{
   int bline = ls->line_number, bcolumn = ls->column;
   Eolian_Typedecl *def = eo_lexer_typedecl_new(ls);
   def->is_extern = is_extern;
   def->base.is_beta = is_beta;
   def->base.name = name;
   if (cname)
     {
        def->base.c_name = eina_stringshare_ref(cname);
        eo_lexer_dtor_pop(ls);
     }
   else
     def->base.c_name = make_c_name(name);
   def->type = EOLIAN_TYPEDECL_ENUM;
   def->fields = eina_hash_string_small_new(EINA_FREE_CB(_enum_field_free));
   check_next(ls, '{');
   FILL_DOC(ls, def, doc);
   if (ls->t.token == TOK_VALUE && ls->t.kw == KW_legacy)
     {
         if (eo_lexer_lookahead(ls) == ':')
           {
              /* consume keyword */
              eo_lexer_get(ls);
              /* consume colon */
              eo_lexer_get(ls);
              check(ls, TOK_VALUE);
              def->legacy = eina_stringshare_ref(ls->t.value.s);
              eo_lexer_get(ls);
              check_next(ls, ';');
           }
     }
   Eolian_Enum_Type_Field *prev_fl = NULL;
   int fl_nadd = 0;
   for (;;)
     {
        const char *fname;
        Eolian_Enum_Type_Field *fdef;
        int fline = ls->line_number, fcol = ls->column;
        check(ls, TOK_VALUE);
        if (eina_hash_find(def->fields, ls->t.value.s))
          eo_lexer_syntax_error(ls, "double field definition");
        fdef = calloc(1, sizeof(Eolian_Enum_Type_Field));
        fname = eina_stringshare_ref(ls->t.value.s);
        eina_hash_add(def->fields, fname, fdef);
        def->field_list = eina_list_append(def->field_list, fdef);
        eolian_object_ref(&fdef->base);
        eo_lexer_get(ls);
        FILL_BASE(fdef->base, ls, fline, fcol, ENUM_FIELD);
        fdef->base_enum = def;
        fdef->base.name = eina_stringshare_ref(fname);
        if (ls->t.token != '=')
          {
             if (!prev_fl)
               {
                  Eolian_Expression *eexp = eo_lexer_expr_new(ls);
                  FILL_BASE(eexp->base, ls, -1, -1, EXPRESSION);
                  eexp->type = EOLIAN_EXPR_INT;
                  eexp->value.i = 0;
                  fdef->value = eexp;
                  fdef->is_public_value = EINA_TRUE;
                  eo_lexer_expr_release_ref(ls, eexp);
                  prev_fl = fdef;
                  fl_nadd = 0;
               }
             else
               {
                  Eolian_Expression *rhs = eo_lexer_expr_new(ls),
                                    *bin = eo_lexer_expr_new(ls);
                  FILL_BASE(rhs->base, ls, -1, -1, EXPRESSION);
                  FILL_BASE(bin->base, ls, -1, -1, EXPRESSION);

                  rhs->type = EOLIAN_EXPR_INT;
                  rhs->value.i = ++fl_nadd;

                  bin->type = EOLIAN_EXPR_BINARY;
                  bin->binop = EOLIAN_BINOP_ADD;
                  bin->lhs = prev_fl->value;
                  bin->rhs = rhs;
                  bin->weak_lhs = EINA_TRUE;
                  eo_lexer_expr_release_ref(ls, rhs);

                  fdef->value = bin;
                  eo_lexer_expr_release_ref(ls, bin);
               }
          }
        else
          {
             ls->expr_mode = EINA_TRUE;
             eo_lexer_get(ls);
             fdef->value = parse_expr(ls);
             fdef->is_public_value = EINA_TRUE;
             ls->expr_mode = EINA_FALSE;
             prev_fl = fdef;
             fl_nadd = 0;
             eo_lexer_expr_release_ref(ls, fdef->value);
          }
        Eina_Bool want_next = (ls->t.token == ',');
        if (want_next)
          eo_lexer_get(ls);
        FILL_DOC(ls, fdef, doc);
        if (def->doc && fdef->doc && def->doc->since && !fdef->doc->since)
          fdef->doc->since = eina_stringshare_ref (def->doc->since);
        if (!want_next || ls->t.token == '}')
          break;
     }
   check_match(ls, '}', '{', bline, bcolumn);
   FILL_BASE(def->base, ls, line, column, TYPEDECL);
   database_enum_add(ls->unit, eo_lexer_typedecl_release(ls, def));
   return def;
}

/**
 * @brief Parses an error type specification, e.g., `error(Eina.Error, My.Custom.Error)`.
 *
 * An error type can list one or more error names, separated by commas.
 * Expects the lexer to be positioned after the `error(` part.
 *
 * @param ls The lexer state.
 * @return A new Eolian_Type representing the parsed error type. This may be a
 *         linked list of types if multiple errors are specified.
 *         The caller receives a new reference that it must manage.
 */
static Eolian_Type *
parse_type_error(Eo_Lexer *ls)
{
   Eolian_Type *def = eo_lexer_type_new(ls);
   Eina_Strbuf *buf = eina_strbuf_new();
   eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
   for (Eolian_Type *cdef = def;; cdef = cdef->next_type)
     {
        FILL_BASE(cdef->base, ls, ls->line_number, ls->column, TYPE);
        parse_name(ls, buf);
        cdef->type = EOLIAN_TYPE_ERROR;
        cdef->base.name = eina_stringshare_add(eina_strbuf_string_get(buf));
        eina_strbuf_reset(buf);
        if (ls->t.token != ',')
          break;
        eo_lexer_get(ls);
        cdef->next_type = eo_lexer_type_release(ls, eo_lexer_type_new(ls));
     }
   eo_lexer_dtor_pop(ls);
   return def;
}

/**
 * @brief Parses a type name, which can be 'void', a basic type, a user-defined type,
 *        or a complex type like ptr(), const(), error(), or generic types (e.g. list<int>).
 *
 * This is the core type parsing function.
 *
 * @param ls The lexer state.
 * @param allow_ptr Whether pointer types (ptr()) are allowed in the current context.
 * @param allow_const Whether const qualifiers (const()) are allowed in the current context.
 * @return A new Eolian_Type representing the parsed type.
 *         The caller receives a new reference that it must manage.
 *         Example return for `ptr(const(int))`:
 *         Eolian_Type (is_ptr=TRUE)
 *         `->` base_type: Eolian_Type (is_const=TRUE)
 *             `->` base_type: Eolian_Type (name="int", btype=EOLIAN_TYPE_BUILTIN_INT)
 */
static Eolian_Type *
parse_type_void(Eo_Lexer *ls, Eina_Bool allow_ptr, Eina_Bool allow_const)
{
   Eolian_Type *def;
   Eina_Strbuf *buf;
   int line = ls->line_number, col = ls->column;
   switch (ls->t.kw)
     {
      case KW_const:
        {
           if (!allow_const)
             break;
           int pline, pcol;
           eo_lexer_get(ls);
           pline = ls->line_number;
           pcol = ls->column;
           check_next(ls, '(');
           def = parse_type_void(ls, allow_ptr, EINA_FALSE);
           FILL_BASE(def->base, ls, line, col, TYPE);
           def->is_const = EINA_TRUE;
           check_match(ls, ')', '(', pline, pcol);
           return def;
        }
      case KW_ptr:
        {
           if (!allow_ptr)
             break;
           int pline, pcol;
           eo_lexer_get(ls);
           pline = ls->line_number;
           pcol = ls->column;
           check_next(ls, '(');
           def = parse_type_void(ls, EINA_FALSE, allow_const);
           FILL_BASE(def->base, ls, line, col, TYPE);
           def->is_ptr = EINA_TRUE;
           check_match(ls, ')', '(', pline, pcol);
           return def;
        }
      case KW_error:
        {
           int pline, pcolumn;
           eo_lexer_get(ls);
           pline = ls->line_number;
           pcolumn = ls->column;
           check_next(ls, '(');
           def = parse_type_error(ls);
           check_match(ls, ')', '(', pline, pcolumn);
           return def;
        }
      default:
        break;
     }
   def = eo_lexer_type_new(ls);
   FILL_BASE(def->base, ls, line, col, TYPE);
   if (ls->t.kw == KW_void)
     {
        def->type = EOLIAN_TYPE_VOID;
        def->btype = EOLIAN_TYPE_BUILTIN_VOID;
        eo_lexer_get(ls);
     }
   else if (ls->t.kw == KW___undefined_type)
     {
        def->type = EOLIAN_TYPE_UNDEFINED;
        eo_lexer_get(ls);
     }
   else
     {
        int tpid = ls->t.kw;
        def->type = EOLIAN_TYPE_REGULAR;
        check(ls, TOK_VALUE);
        if (eo_lexer_is_type_keyword(ls->t.kw))
          {
             def->btype = ls->t.kw - KW_byte + 1;
             def->base.name = eina_stringshare_ref(ls->t.value.s);
             def->base.c_name = eina_stringshare_add(eo_lexer_get_c_type(ls->t.kw));
             eo_lexer_get(ls);
             if ((tpid >= KW_accessor && tpid <= KW_list) ||
                 (tpid >= KW_slice && tpid <= KW_rw_slice) || (tpid == KW_hash))
               {
                  int bline = ls->line_number, bcol = ls->column;
                  check_next(ls, '<');
                  if (tpid == KW_future)
                    def->base_type = eo_lexer_type_release(ls, parse_type_void(ls, EINA_TRUE, EINA_TRUE));
                  else
                    def->base_type = eo_lexer_type_release(ls, parse_type(ls, EINA_TRUE, EINA_TRUE));
                  /* view-only types are not allowed to own the contents */
                  if (tpid == KW_array || tpid == KW_hash || tpid == KW_list || tpid == KW_future)
                    if ((def->base_type->move = ls->t.kw == KW_at_move))
                      eo_lexer_get(ls);
                  if (tpid == KW_hash)
                    {
                       check_next(ls, ',');
                       def->base_type->next_type =
                         eo_lexer_type_release(ls, parse_type(ls, EINA_TRUE, EINA_TRUE));
                       if ((def->base_type->next_type->move = ls->t.kw == KW_at_move))
                         eo_lexer_get(ls);
                    }
                  check_match(ls, '>', '<', bline, bcol);
               }
          }
        else
          {
             const char *bnm, *nm;
             char *fnm;
             buf = eina_strbuf_new();
             eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
             eo_lexer_context_push(ls);
             parse_name(ls, buf);
             nm = eina_strbuf_string_get(buf);
             bnm = eina_stringshare_ref(ls->filename);
             fnm = database_class_to_filename(nm);
             if (!compare_class_file(bnm, fnm))
               {
                  eina_stringshare_del(bnm);
                  if (eina_hash_find(ls->state->filenames_eo, fnm))
                    {
                       database_defer(ls->state, fnm, EINA_TRUE);
                       def->type = EOLIAN_TYPE_CLASS;
                    }
                  free(fnm);
               }
             else
               {
                  eina_stringshare_del(bnm);
                  free(fnm);
                  def->type = EOLIAN_TYPE_CLASS;
               }
             def->base.name = eina_stringshare_add(nm);
             eo_lexer_context_pop(ls);
             eo_lexer_dtor_pop(ls);
          }
     }
   return def;
}

/**
 * @brief Parses a type alias (typedef) definition.
 *
 * Example Eolian syntax:
 * @code
 * type My_Int_Alias @beta : int;
 * type My_Object_Alias @c_name("my_c_obj_alias") : Some.Other.Object;
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at 'type' keyword.
 * @return A new Eolian_Typedecl representing the parsed type alias.
 *         The caller receives a new reference that it must manage.
 */
static Eolian_Typedecl *
parse_typedef(Eo_Lexer *ls)
{
   Eolian_Typedecl *def = eo_lexer_typedecl_new(ls);
   Eina_Strbuf *buf;
   eo_lexer_get(ls);
   Eina_Stringshare *cname = NULL;
   Eina_Bool has_extern = EINA_FALSE, has_beta = EINA_FALSE, has_c_name = EINA_FALSE;
   for (;;) switch (ls->t.kw)
     {
      case KW_at_extern:
        CASE_LOCK(ls, extern, "extern qualifier");
        def->is_extern = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_beta:
        CASE_LOCK(ls, beta, "beta qualifier");
        def->base.is_beta = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_c_name:
        CASE_LOCK(ls, c_name, "@c_name specifier");
        cname = parse_c_name(ls);
        eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_stringshare_del), (void *)cname);
        break;
      default:
        goto tags_done;
     }
tags_done:
   def->type = EOLIAN_TYPEDECL_ALIAS;
   buf = eina_strbuf_new();
   eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
   eo_lexer_context_push(ls);
   FILL_BASE(def->base, ls, ls->line_number, ls->column, TYPEDECL);
   parse_name(ls, buf);
   def->base.name = eina_stringshare_add(eina_strbuf_string_get(buf));
   if (cname)
     {
        def->base.c_name = eina_stringshare_ref(cname);
        eo_lexer_dtor_pop(ls);
     }
   else
     def->base.c_name = make_c_name(def->base.name);
   Eolian_Object *decl = _eolian_decl_get(ls, def->base.name);
   if (decl)
     {
        eo_lexer_context_restore(ls);
        redef_error(ls, decl, &def->base);
     }
   eo_lexer_context_pop(ls);
   check_next(ls, ':');
   def->base_type = eo_lexer_type_release(ls, parse_type(ls, EINA_FALSE, EINA_FALSE));
   check_next(ls, ';');
   FILL_DOC(ls, def, doc);
   eo_lexer_dtor_pop(ls);
   return def;
}

/**
 * @brief Parses a constant definition.
 *
 * Example Eolian syntax:
 * @code
 * const MY_CONSTANT @beta : int = 42;
 * const MY_STRING_CONST @c_name("MY_C_STR") : string = "hello";
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at 'const' keyword.
 * @return A new Eolian_Constant representing the parsed constant.
 *         The caller receives a new reference that it must manage.
 */
static Eolian_Constant *
parse_constant(Eo_Lexer *ls)
{
   Eolian_Constant *def = eo_lexer_constant_new(ls);
   Eina_Strbuf *buf;
   eo_lexer_get(ls);
   Eina_Stringshare *cname = NULL;
   Eina_Bool has_extern = EINA_FALSE, has_beta = EINA_FALSE, has_c_name = EINA_FALSE;
   for (;;) switch (ls->t.kw)
     {
      case KW_at_extern:
        CASE_LOCK(ls, extern, "extern qualifier");
        def->is_extern = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_beta:
        CASE_LOCK(ls, beta, "beta qualifier");
        def->base.is_beta = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_c_name:
        CASE_LOCK(ls, c_name, "@c_name specifier");
        cname = parse_c_name(ls);
        eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_stringshare_del), (void *)cname);
        break;
      default:
        goto tags_done;
     }
tags_done:
   buf = eina_strbuf_new();
   eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
   eo_lexer_context_push(ls);
   FILL_BASE(def->base, ls, ls->line_number, ls->column, CONSTANT);
   parse_name(ls, buf);
   def->base.name = eina_stringshare_add(eina_strbuf_string_get(buf));
   if (cname)
     {
        def->base.c_name = eina_stringshare_ref(cname);
        eo_lexer_dtor_pop(ls);
     }
   else
     def->base.c_name = make_c_name(def->base.name);
   Eolian_Object *decl = _eolian_decl_get(ls, def->base.name);
   if (decl)
     {
        eo_lexer_context_restore(ls);
        redef_error(ls, decl, &def->base);
     }
   eo_lexer_context_pop(ls);
   check_next(ls, ':');
   def->base_type = eo_lexer_type_release(ls, parse_type(ls, EINA_FALSE, EINA_FALSE));
   /* constants are required to have a value */
   check(ls, '=');
   ls->expr_mode = EINA_TRUE;
   eo_lexer_get(ls);
   def->value = parse_expr(ls);
   ls->expr_mode = EINA_FALSE;
   eo_lexer_expr_release_ref(ls, def->value);
   check_next(ls, ';');
   FILL_DOC(ls, def, doc);
   eo_lexer_dtor_pop(ls);
   return def;
}

/**
 * @brief Parses an error definition.
 *
 * Error definitions associate a name with a descriptive message.
 *
 * Example Eolian syntax:
 * @code
 * error My.Custom.Error @beta = "A custom error occurred.";
 * error Another.Error @c_name("ANOTHER_C_ERROR") = "Something else went wrong.";
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at 'error' keyword.
 * @return A new Eolian_Error representing the parsed error.
 *         The caller receives a new reference that it must manage.
 */
static Eolian_Error *
parse_error(Eo_Lexer *ls)
{
   Eolian_Error *def = eo_lexer_error_new(ls);
   Eina_Strbuf *buf;
   eo_lexer_get(ls);
   Eina_Stringshare *cname = NULL;
   Eina_Bool has_extern = EINA_FALSE, has_beta = EINA_FALSE, has_c_name = EINA_FALSE;
   for (;;) switch (ls->t.kw)
     {
      case KW_at_extern:
        CASE_LOCK(ls, extern, "extern qualifier");
        def->is_extern = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_beta:
        CASE_LOCK(ls, beta, "beta qualifier");
        def->base.is_beta = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_c_name:
        CASE_LOCK(ls, c_name, "@c_name specifier");
        cname = parse_c_name(ls);
        eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_stringshare_del), (void *)cname);
        break;
      default:
        goto tags_done;
     }
tags_done:
   buf = eina_strbuf_new();
   eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
   eo_lexer_context_push(ls);
   FILL_BASE(def->base, ls, ls->line_number, ls->column, ERROR);
   parse_name(ls, buf);
   def->base.name = eina_stringshare_add(eina_strbuf_string_get(buf));
   if (cname)
     {
        def->base.c_name = eina_stringshare_ref(cname);
        eo_lexer_dtor_pop(ls);
     }
   else
     def->base.c_name = make_c_name(def->base.name);
   Eolian_Object *decl = _eolian_decl_get(ls, def->base.name);
   if (decl)
     {
        eo_lexer_context_restore(ls);
        redef_error(ls, decl, &def->base);
     }
   eo_lexer_context_pop(ls);
   check(ls, '=');
   /* we need to parse a string so switch to exprmode */
   ls->expr_mode = EINA_TRUE;
   /* consume = to get string */
   eo_lexer_get(ls);
   /* verify and switch back to plain syntax mode */
   check(ls, TOK_STRING);
   ls->expr_mode = EINA_FALSE;
   def->msg = eina_stringshare_ref(ls->t.value.s);
   eo_lexer_get(ls);
   check_next(ls, ';');
   FILL_DOC(ls, def, doc);
   eo_lexer_dtor_pop(ls);
   return def;
}

typedef struct _Eo_Ret_Def
{
   Eolian_Type *type;
   Eolian_Documentation *doc;
   Eolian_Expression *default_ret_val;
   Eina_Bool no_unused: 1;
   Eina_Bool move: 1; /**< @move qualifier for the return value. */
   Eina_Bool by_ref: 1; /**< @by_ref qualifier for the return value. */
} Eo_Ret_Def;

/**
 * @brief Parses a return type specification for a function, method, or property getter.
 *
 * Handles the return type, optional default value, documentation, and qualifiers
 * like @no_unused, @move, @by_ref.
 *
 * Example Eolian syntax (within a method/property):
 * @code
 * return: int (0) @no_unused; // Returns int, default 0, must be used
 *   doc: "The return value.";
 * return: string @move;
 *   doc: "A moved string.";
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at 'return' keyword.
 * @param[out] ret Pointer to an Eo_Ret_Def struct to be filled with parsed information.
 *                 The `type`, `doc`, and `default_ret_val` fields will be allocated
 *                 by the parser and their ownership is transferred via this struct.
 *                 The caller is responsible for managing these resources if they are set.
 * @param allow_void Whether 'void' is a permissible return type.
 * @param allow_def Whether a default return value `(expr)` is allowed.
 * @param is_funcptr Whether this return is for a function pointer (disables some qualifiers).
 */
static void
parse_return(Eo_Lexer *ls, Eo_Ret_Def *ret, Eina_Bool allow_void,
             Eina_Bool allow_def, Eina_Bool is_funcptr)
{
   eo_lexer_get(ls);
   check_next(ls, ':');
   if (allow_void)
     ret->type = parse_type_void(ls, EINA_TRUE, EINA_TRUE);
   else
     ret->type = parse_type(ls, EINA_TRUE, EINA_TRUE);
   ret->doc = NULL;
   ret->default_ret_val = NULL;
   ret->no_unused = EINA_FALSE;
   ret->move = EINA_FALSE;
   ret->by_ref = EINA_FALSE;
   if (allow_def && (ls->t.token == '('))
     {
        int line = ls->line_number, col = ls->column;
        ls->expr_mode = EINA_TRUE;
        eo_lexer_get(ls);
        ret->default_ret_val = parse_expr(ls);
        ls->expr_mode = EINA_FALSE;
        check_match(ls, ')', '(', line, col);
     }
   Eina_Bool has_no_unused = EINA_FALSE, has_move = EINA_FALSE,
             has_by_ref = EINA_FALSE;
   if (!is_funcptr) for (;;) switch (ls->t.kw)
     {
      case KW_at_no_unused:
        CASE_LOCK(ls, no_unused, "no_unused qualifier");
        ret->no_unused = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_move:
        CASE_LOCK(ls, move, "move qualifier");
        ret->move = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_by_ref:
        CASE_LOCK(ls, by_ref, "by_ref qualifier");
        ret->by_ref = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      default:
        goto end;
     }
end:
   check_next(ls, ';');
   FILL_DOC(ls, ret, doc);
}

/**
 * @brief Parses a single function/method parameter.
 *
 * Handles parameter direction (@in, @out, @inout), name, type, optional
 * default value (for 'values' blocks or out parameters), documentation,
 * and qualifiers like @optional, @move, @by_ref.
 *
 * Example Eolian syntax (within a params/values block):
 * @code
 * @in my_param: int @optional;
 *   doc: "An optional input parameter.";
 * @out result: string @move (null);
 *   doc: "An output string, moved.";
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at the start of the parameter definition (e.g., @in or name).
 * @param[in,out] params A pointer to an Eina_List of Eolian_Function_Parameter. The new parameter will be appended.
 * @param allow_inout Whether @in, @out, @inout direction specifiers are allowed.
 * @param is_vals Whether this parameter is part of a 'values' block (affects default value parsing).
 * @param func The parent function/method, used to check beta status for type parsing.
 */
static void
parse_param(Eo_Lexer *ls, Eina_List **params, Eina_Bool allow_inout,
            Eina_Bool is_vals, const Eolian_Function *func)
{
   Eina_Bool has_optional = EINA_FALSE,
             has_move     = EINA_FALSE,
             has_by_ref   = EINA_FALSE;
   Eolian_Function_Parameter *par = calloc(1, sizeof(Eolian_Function_Parameter));
   par->param_dir = EOLIAN_PARAMETER_IN;
   FILL_BASE(par->base, ls, ls->line_number, ls->column, FUNCTION_PARAMETER);
   *params = eina_list_append(*params, par);
   eolian_object_ref(&par->base);
   if (allow_inout && (ls->t.kw == KW_at_in))
     {
        par->param_dir = EOLIAN_PARAMETER_IN;
        eo_lexer_get(ls);
     }
   else if (allow_inout && ls->t.kw == KW_at_out)
     {
        par->param_dir = EOLIAN_PARAMETER_OUT;
        eo_lexer_get(ls);
     }
   else if (allow_inout && ls->t.kw == KW_at_inout)
     {
        par->param_dir = EOLIAN_PARAMETER_INOUT;
        eo_lexer_get(ls);
     }
   else par->param_dir = EOLIAN_PARAMETER_IN;
   check(ls, TOK_VALUE);
   par->base.name = eina_stringshare_ref(ls->t.value.s);
   eo_lexer_get(ls);
   check_next(ls, ':');
   if ((ls->klass && ls->klass->base.is_beta) || func->base.is_beta)
     {
       if (par->param_dir == EOLIAN_PARAMETER_OUT || par->param_dir == EOLIAN_PARAMETER_INOUT)
         {
            /* void is allowed for out/inout for beta-api for now to make a voidptr */
            par->type = eo_lexer_type_release(ls, parse_type_void(ls, EINA_TRUE, EINA_TRUE));
            goto type_done;
         }
     }
   par->type = eo_lexer_type_release(ls, parse_type(ls, EINA_TRUE, EINA_TRUE));
type_done:
   if ((is_vals || (par->param_dir == EOLIAN_PARAMETER_OUT)) && (ls->t.token == '('))
     {
        int line = ls->line_number, col = ls->column;
        ls->expr_mode = EINA_TRUE;
        eo_lexer_get(ls);
        par->value = parse_expr(ls);
        ls->expr_mode = EINA_FALSE;
        eo_lexer_expr_release_ref(ls, par->value);
        check_match(ls, ')', '(', line, col);
     }
   for (;;) switch (ls->t.kw)
     {
      case KW_at_optional:
        CASE_LOCK(ls, optional, "optional qualifier");
        par->optional = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_move:
        CASE_LOCK(ls, move, "move qualifier");
        par->move = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_by_ref:
        CASE_LOCK(ls, by_ref, "by_ref qualifier");
        par->by_ref = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      default:
        goto end;
     }
end:
   check_next(ls, ';');
   FILL_DOC(ls, par, doc);
}

/**
 * @brief Parses a block of parameters (e.g., `params { ... }` or `keys { ... }` or `values { ... }`).
 *
 * Example Eolian syntax:
 * @code
 * params {
 *   @in p1: int;
 *   @out p2: string;
 * }
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at the keyword like 'params', 'keys', 'values'.
 * @param[in,out] params A pointer to an Eina_List of Eolian_Function_Parameter. Parsed parameters will be appended.
 * @param allow_inout Whether @in, @out, @inout direction specifiers are allowed for parameters in this block.
 * @param is_vals Whether this block is a 'values' block (affects default value parsing for parameters).
 * @param func The parent function/method, passed down to parse_param.
 */
static void
parse_params(Eo_Lexer *ls, Eina_List **params, Eina_Bool allow_inout,
             Eina_Bool is_vals, const Eolian_Function *func)
{
   int line, col;
   eo_lexer_get(ls);
   line = ls->line_number, col = ls->column;
   check_next(ls, '{');
   while (ls->t.token != '}')
     parse_param(ls, params, allow_inout, is_vals, func);
   check_match(ls, '}', '{', line, col);
}

/**
 * @brief Checks if the current class context allows for @pure_virtual methods/properties.
 *
 * @pure_virtual is only allowed in abstract classes or mixins.
 * Raises a syntax error if the context is invalid.
 * @param ls The lexer state.
 */
static void
check_abstract_pure_virtual(Eo_Lexer *ls)
{
   if ((ls->klass->type != EOLIAN_CLASS_ABSTRACT) && (ls->klass->type != EOLIAN_CLASS_MIXIN))
     eo_lexer_syntax_error(ls, "@pure_virtual only allowed in abstract classes or mixins");
}

/**
 * @brief Parses a property accessor (get or set block).
 *
 * Handles @pure_virtual, @protected qualifiers, return type (for get),
 * key parameters, and value parameters.
 *
 * Example Eolian syntax (within a property):
 * @code
 * get @pure_virtual {
 *   doc: "Getter documentation.";
 *   return: int;
 *   keys { k: string; }
 * }
 * set @protected {
 *   doc: "Setter documentation.";
 *   values { v: int; }
 * }
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at 'get' or 'set' keyword.
 * @param prop The Eolian_Function (property) object to populate with accessor details.
 */
static void
parse_accessor(Eo_Lexer *ls, Eolian_Function *prop)
{
   int line, col;
   Eina_Bool has_return = EINA_FALSE, has_keys      = EINA_FALSE,
             has_values = EINA_FALSE, has_protected = EINA_FALSE,
             has_virtp  = EINA_FALSE;
   Eina_Bool is_get = (ls->t.kw == KW_get);
   if (is_get)
     {
        if (prop->base.file)
          eina_stringshare_del(prop->base.file);
        FILL_BASE(prop->base, ls, ls->line_number, ls->column, FUNCTION);
        if (prop->type == EOLIAN_PROP_SET)
          prop->type = EOLIAN_PROPERTY;
        else
          prop->type = EOLIAN_PROP_GET;
     }
   else
     {
        FILL_BASE(prop->set_base, ls, ls->line_number, ls->column, FUNCTION);
        if (prop->type == EOLIAN_PROP_GET)
          prop->type = EOLIAN_PROPERTY;
        else
          prop->type = EOLIAN_PROP_SET;
     }
   eo_lexer_get(ls);
   for (;;) switch (ls->t.kw)
     {
      case KW_at_pure_virtual:
        check_abstract_pure_virtual(ls);
        CASE_LOCK(ls, virtp, "pure_virtual qualifier");
        if (is_get) prop->impl->get_pure_virtual = EINA_TRUE;
        else prop->impl->set_pure_virtual = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_protected:
        CASE_LOCK(ls, protected, "protected qualifier");
        if (is_get) prop->get_scope = EOLIAN_SCOPE_PROTECTED;
        else prop->set_scope = EOLIAN_SCOPE_PROTECTED;
        eo_lexer_get(ls);
        break;
      default:
        goto parse_accessor;
     }
parse_accessor:
   line = ls->line_number;
   col = ls->column;
   check_next(ls, '{');
   if ((ls->t.token == TOK_DOC) && !prop->impl->common_doc)
     {
        Eolian_Object tmp;
        memset(&tmp, 0, sizeof(Eolian_Object));
        tmp.file = prop->base.file;
        tmp.line = line;
        tmp.column = col;
        tmp.unit = ls->unit;
        eolian_state_log_obj(ls->state, &tmp,
                             "%s doc without property doc for '%s.%s'",
                             is_get ? "getter" : "setter",
                             ls->klass->base.name, prop->base.name);
     }
   if (is_get)
     {
        FILL_DOC(ls, prop->impl, get_doc);
     }
   else
     {
        FILL_DOC(ls, prop->impl, set_doc);
     }
   for (;;) switch (ls->t.kw)
     {
      case KW_return:
        CASE_LOCK(ls, return, "return")
        Eo_Ret_Def ret;
        parse_return(ls, &ret, is_get, EINA_TRUE, EINA_FALSE);
        if (ret.default_ret_val)
          eo_lexer_expr_release_ref(ls, ret.default_ret_val);
        if (is_get)
          {
             prop->get_ret_type = eo_lexer_type_release(ls, ret.type);
             prop->get_return_doc = ret.doc;
             prop->get_ret_val = ret.default_ret_val;
             prop->get_return_no_unused = ret.no_unused;
             prop->get_return_by_ref = ret.by_ref;
             prop->get_return_move = ret.move;
          }
        else
          {
             prop->set_ret_type = eo_lexer_type_release(ls, ret.type);
             prop->set_return_doc = ret.doc;
             prop->set_ret_val = ret.default_ret_val;
             prop->set_return_no_unused = ret.no_unused;
             prop->set_return_by_ref = ret.by_ref;
             prop->set_return_move = ret.move;
          }
        break;
      case KW_keys:
        {
           Eina_List **stor;
           CASE_LOCK(ls, keys, "keys definition")
           stor = is_get ? &prop->prop_keys_get : &prop->prop_keys_set;
           parse_params(ls, stor, EINA_FALSE, EINA_FALSE, prop);
           break;
        }
      case KW_values:
        {
           Eina_List **stor;
           CASE_LOCK(ls, values, "values definition")
           stor = is_get ? &prop->prop_values_get : &prop->prop_values_set;
           parse_params(ls, stor, EINA_FALSE, EINA_TRUE, prop);
           break;
        }
      default:
        goto end;
     }
end:
   check_match(ls, '}', '{', line, col);
}

/**
 * @brief Sets the pure_virtual flag on a function's implementation details.
 *
 * This is used for methods and properties. If the class is an interface,
 * or if the `virt` flag is explicitly true (from an @pure_virtual tag),
 * the corresponding pure_virtual flags in the function's Eolian_Implement
 * structure are set.
 *
 * @param ls The lexer state (to check class type).
 * @param foo_id The function (method or property) whose implementation flags are to be set.
 * @param virt EINA_TRUE if @pure_virtual was explicitly specified for this function/property.
 */
static void
_func_pure_virtual_set(Eo_Lexer *ls, Eolian_Function *foo_id, Eina_Bool virt)
{
   if (ls->klass->type != EOLIAN_CLASS_INTERFACE && !virt)
     return;

   if (foo_id->type == EOLIAN_PROP_GET || foo_id->type == EOLIAN_METHOD)
     foo_id->impl->get_pure_virtual = EINA_TRUE;
   else if (foo_id->type == EOLIAN_PROP_SET)
     foo_id->impl->set_pure_virtual = EINA_TRUE;
   else if (foo_id->type == EOLIAN_PROPERTY)
     foo_id->impl->get_pure_virtual = foo_id->impl->set_pure_virtual = EINA_TRUE;
}

/**
 * @brief Parses a property definition (introduced by `@property` inside `methods { ... }`).
 *
 * A property can have get and/or set accessors, keys, and values.
 * It can also have qualifiers like @protected, @static, @beta, @pure_virtual.
 *
 * Example Eolian syntax:
 * @code
 * @property my_prop @beta {
 *   doc: "A property.";
 *   keys { k: string; }
 *   get { return: int; }
 *   set { values { v: int; } }
 * }
 * @endcode
 *
 * @param ls The lexer state. Expects lexer after `@property` keyword.
 */
static void
parse_property(Eo_Lexer *ls)
{
   int line, col;
   Eolian_Function *prop = NULL;
   Eolian_Implement *impl = NULL;
   Eina_Bool has_get       = EINA_FALSE, has_set    = EINA_FALSE,
             has_keys      = EINA_FALSE, has_values = EINA_FALSE,
             has_protected = EINA_FALSE, has_class  = EINA_FALSE,
             has_beta      = EINA_FALSE, has_virtp  = EINA_FALSE;
   prop = calloc(1, sizeof(Eolian_Function));
   prop->klass = ls->klass;
   prop->type = EOLIAN_UNRESOLVED;
   prop->get_scope = prop->set_scope = EOLIAN_SCOPE_PUBLIC;
   FILL_BASE(prop->base, ls, ls->line_number, ls->column, FUNCTION);
   impl = calloc(1, sizeof(Eolian_Implement));
   impl->klass = impl->implklass = ls->klass;
   impl->foo_id = prop;
   FILL_BASE(impl->base, ls, ls->line_number, ls->column, IMPLEMENT);
   prop->impl = impl;
   ls->klass->properties = eina_list_append(ls->klass->properties, prop);
   ls->klass->implements = eina_list_append(ls->klass->implements, impl);
   eolian_object_ref(&prop->base);
   eolian_object_ref(&impl->base);
   check(ls, TOK_VALUE);
   if (ls->t.kw == KW_get || ls->t.kw == KW_set)
     {
        eo_lexer_syntax_error(ls, "reserved keyword as property name");
        return;
     }
   prop->base.name = eina_stringshare_ref(ls->t.value.s);
   impl->base.name = eina_stringshare_printf("%s.%s", ls->klass->base.name, prop->base.name);
   eo_lexer_get(ls);
   for (;;) switch (ls->t.kw)
     {
      case KW_at_protected:
        CASE_LOCK(ls, protected, "protected qualifier")
        prop->get_scope = prop->set_scope = EOLIAN_SCOPE_PROTECTED;
        eo_lexer_get(ls);
        break;
      case KW_at_static:
        CASE_LOCK(ls, class, "class qualifier");
        prop->is_static = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_beta:
        CASE_LOCK(ls, beta, "beta qualifier");
        prop->base.is_beta = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_pure_virtual:
        check_abstract_pure_virtual(ls);
        CASE_LOCK(ls, virtp, "pure_virtual qualifier");
        eo_lexer_get(ls);
        break;
      default:
        goto body;
     }
body:
   line = ls->line_number;
   col = ls->column;
   check_next(ls, '{');
   FILL_DOC(ls, prop->impl, common_doc);
   for (;;) switch (ls->t.kw)
     {
      case KW_get:
        CASE_LOCK(ls, get, "get definition")
        impl->is_prop_get = EINA_TRUE;
        parse_accessor(ls, prop);
        break;
      case KW_set:
        CASE_LOCK(ls, set, "set definition")
        impl->is_prop_set = EINA_TRUE;
        parse_accessor(ls, prop);
        break;
      case KW_keys:
        CASE_LOCK(ls, keys, "keys definition")
        parse_params(ls, &prop->prop_keys, EINA_FALSE, EINA_FALSE, prop);
        break;
      case KW_values:
        CASE_LOCK(ls, values, "values definition")
        parse_params(ls, &prop->prop_values, EINA_FALSE, EINA_TRUE, prop);
        break;
      default:
        goto end;
     }
end:
   check_match(ls, '}', '{', line, col);
   if (!has_get && !has_set)
     {
        prop->type = EOLIAN_PROPERTY;
        impl->is_prop_get = impl->is_prop_set = EINA_TRUE;
     }
   _func_pure_virtual_set(ls, prop, has_virtp);
}

/**
 * @brief Parses a function pointer type definition.
 *
 * Example Eolian syntax:
 * @code
 * function My_Callback_Type @beta (
 *   @c_name("my_c_callback_t")
 * ) {
 *   doc: "A callback function type.";
 *   return: void;
 *   params {
 *     @in data: ptr(void);
 *     @in event_info: ptr(const(Some_Event));
 *   }
 * }; // Note the semicolon at the end
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at 'function' keyword.
 * @return A new Eolian_Typedecl representing the parsed function pointer type.
 *         The caller receives a new reference that it must manage.
 */
static Eolian_Typedecl*
parse_function_pointer(Eo_Lexer *ls)
{
   int bline, bcol;
   int line = ls->line_number, col = ls->column;

   Eolian_Typedecl *def = eo_lexer_typedecl_new(ls);
   Eina_Strbuf *buf = eina_strbuf_new();
   eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
   Eolian_Function *meth = NULL;

   Eina_Stringshare *cname = NULL;
   Eina_Bool has_params = EINA_FALSE,
             has_return = EINA_FALSE,
             has_c_name = EINA_FALSE;

   eo_lexer_get(ls);

   def->type = EOLIAN_TYPEDECL_FUNCTION_POINTER;
   Eina_Bool has_extern = EINA_FALSE, has_beta = EINA_FALSE;
   for (;;) switch (ls->t.kw)
     {
      case KW_at_extern:
        CASE_LOCK(ls, extern, "extern qualifier");
        def->is_extern = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_beta:
        CASE_LOCK(ls, beta, "beta qualifier");
        def->base.is_beta = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_c_name:
        CASE_LOCK(ls, c_name, "@c_name specifier");
        cname = parse_c_name(ls);
        eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_stringshare_del), (void *)cname);
        break;
      default:
        goto tags_done;
     }
tags_done:
   parse_name(ls, buf);
   def->base.name = eina_stringshare_add(eina_strbuf_string_get(buf));
   if (cname)
     {
        def->base.c_name = eina_stringshare_ref(cname);
        eo_lexer_dtor_pop(ls);
     }
   else
     def->base.c_name = make_c_name(def->base.name);
   eo_lexer_dtor_pop(ls);

   meth = calloc(1, sizeof(Eolian_Function));
   meth->klass = NULL;
   meth->type = EOLIAN_FUNCTION_POINTER;
   meth->get_scope = meth->set_scope = EOLIAN_SCOPE_PUBLIC;
   meth->base.name = eina_stringshare_add(eolian_object_short_name_get(&def->base));

   def->function_pointer = meth;
   eolian_object_ref(&meth->base);

   meth->base.is_beta = (ls->t.kw == KW_at_beta);
   if (meth->base.is_beta)
     eo_lexer_get(ls);

   bline = ls->line_number;
   bcol = ls->column;
   check_next(ls, '{');
   FILL_DOC(ls, def, doc);
   for (;;) switch (ls->t.kw)
     {
      case KW_return:
        CASE_LOCK(ls, return, "return");
        Eo_Ret_Def ret;
        parse_return(ls, &ret, EINA_FALSE, EINA_FALSE, EINA_TRUE);
        meth->get_ret_type = eo_lexer_type_release(ls, ret.type);
        meth->get_return_doc = ret.doc;
        meth->get_ret_val = NULL;
        meth->get_return_no_unused = EINA_FALSE;
        break;
      case KW_params:
        CASE_LOCK(ls, params, "params definition");
        parse_params(ls, &meth->params, EINA_TRUE, EINA_FALSE, meth);
        break;
      default:
        goto end;
     }
end:
   check_match(ls, '}', '{', bline, bcol);
   check_next(ls, ';');
   FILL_BASE(def->base, ls, line, col, TYPEDECL);
   FILL_BASE(meth->base, ls, line, col, FUNCTION);
   return def;
}

/**
 * @brief Parses a method definition.
 *
 * Methods can have parameters, a return type, and qualifiers like
 * @protected, @const (for the object instance), @static, @beta, @pure_virtual.
 *
 * Example Eolian syntax:
 * @code
 * my_method @const @beta (
 *   param1: int,
 *   param2: string
 * ) {
 *   doc: "A method.";
 *   return: bool;
 *   params {
 *     p1: int;
 *     p2: string @optional;
 *   }
 * }
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at the method name.
 */
static void
parse_method(Eo_Lexer *ls)
{
   int line, col;
   Eolian_Function *meth = NULL;
   Eolian_Implement *impl = NULL;
   Eina_Bool has_const       = EINA_FALSE, has_params = EINA_FALSE,
             has_return      = EINA_FALSE, has_protected = EINA_FALSE,
             has_class       = EINA_FALSE, has_beta   = EINA_FALSE,
             has_virtp       = EINA_FALSE;
   meth = calloc(1, sizeof(Eolian_Function));
   meth->klass = ls->klass;
   meth->type = EOLIAN_METHOD;
   meth->get_scope = meth->set_scope = EOLIAN_SCOPE_PUBLIC;
   FILL_BASE(meth->base, ls, ls->line_number, ls->column, FUNCTION);
   impl = calloc(1, sizeof(Eolian_Implement));
   impl->klass = impl->implklass = ls->klass;
   impl->foo_id = meth;
   FILL_BASE(impl->base, ls, ls->line_number, ls->column, IMPLEMENT);
   meth->impl = impl;
   ls->klass->methods = eina_list_append(ls->klass->methods, meth);
   ls->klass->implements = eina_list_append(ls->klass->implements, impl);
   eolian_object_ref(&meth->base);
   eolian_object_ref(&impl->base);
   check(ls, TOK_VALUE);
   if (ls->t.kw == KW_get || ls->t.kw == KW_set)
     {
        eo_lexer_syntax_error(ls, "reserved keyword as method name");
        return;
     }
   meth->base.name = eina_stringshare_ref(ls->t.value.s);
   impl->base.name = eina_stringshare_printf("%s.%s", ls->klass->base.name, meth->base.name);
   eo_lexer_get(ls);
   for (;;) switch (ls->t.kw)
     {
      case KW_at_protected:
        CASE_LOCK(ls, protected, "protected qualifier")
        meth->get_scope = meth->set_scope = EOLIAN_SCOPE_PROTECTED;
        eo_lexer_get(ls);
        break;
      case KW_at_const:
        CASE_LOCK(ls, const, "const qualifier")
        meth->obj_is_const = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_static:
        CASE_LOCK(ls, class, "class qualifier");
        meth->is_static = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_beta:
        CASE_LOCK(ls, beta, "beta qualifier");
        meth->base.is_beta = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_pure_virtual:
        check_abstract_pure_virtual(ls);
        CASE_LOCK(ls, virtp, "pure_virtual qualifier");
        eo_lexer_get(ls);
        break;
      default:
        goto body;
     }
body:
   line = ls->line_number;
   col = ls->column;
   check_next(ls, '{');
   FILL_DOC(ls, meth->impl, common_doc);
   for (;;) switch (ls->t.kw)
     {
      case KW_return:
        CASE_LOCK(ls, return, "return")
        Eo_Ret_Def ret;
        parse_return(ls, &ret, EINA_FALSE, EINA_TRUE, EINA_FALSE);
        if (ret.default_ret_val)
          eo_lexer_expr_release_ref(ls, ret.default_ret_val);
        meth->get_ret_type = eo_lexer_type_release(ls, ret.type);
        meth->get_return_doc = ret.doc;
        meth->get_ret_val = ret.default_ret_val;
        meth->get_return_no_unused = ret.no_unused;
        meth->get_return_by_ref = ret.by_ref;
        meth->get_return_move = ret.move;
        break;
      case KW_params:
        CASE_LOCK(ls, params, "params definition")
        parse_params(ls, &meth->params, EINA_TRUE, EINA_FALSE, meth);
        break;
      default:
        goto end;
     }
end:
   check_match(ls, '}', '{', line, col);
   _func_pure_virtual_set(ls, meth, has_virtp);
}

/**
 * @brief Parses a class part definition.
 *
 * Parts declare a named member of a class that is an instance of another class.
 *
 * Example Eolian syntax:
 * @code
 * my_part @beta : Other.Class;
 *   doc: "This is a part of the class.";
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at the part name.
 */
static void
parse_part(Eo_Lexer *ls)
{
   Eolian_Part *part = calloc(1, sizeof(Eolian_Part));
   FILL_BASE(part->base, ls, ls->line_number, ls->column, PART);
   ls->klass->parts = eina_list_append(ls->klass->parts, part);
   eolian_object_ref(&part->base);
   check(ls, TOK_VALUE);
   part->base.name = eina_stringshare_ref(ls->t.value.s);
   eo_lexer_get(ls);
   if (ls->t.kw == KW_at_beta)
     {
        part->base.is_beta = EINA_TRUE;
        eo_lexer_get(ls);
     }
   check_next(ls, ':');
   Eina_Strbuf *buf = eina_strbuf_new();
   eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
   eo_lexer_context_push(ls);
   parse_name(ls, buf);
   const char *nm = eina_strbuf_string_get(buf);
   char *fnm = database_class_to_filename(nm);
   if (!eina_hash_find(ls->state->filenames_eo, fnm))
     {
        free(fnm);
        char ebuf[PATH_MAX];
        eo_lexer_context_restore(ls);
        snprintf(ebuf, sizeof(ebuf), "unknown class '%s'", nm);
        eo_lexer_syntax_error(ls, ebuf);
        return;
     }
   database_defer(ls->state, fnm, EINA_TRUE);
   free(fnm);
   part->klass_name = eina_stringshare_add(nm);
   eo_lexer_dtor_pop(ls);
   check_next(ls, ';');
   FILL_DOC(ls, part, doc);
}

/**
 * @brief Parses an implement directive for a class.
 *
 * This specifies how a class implements methods/properties from its interfaces
 * or its own declared functions. It can involve aliasing, or specifying
 * @auto/@empty implementations.
 *
 * Example Eolian syntax:
 * @code
 * // Implementing an interface method
 * Interface.Name.method_name;
 *   doc: "Implementation details for this interface method.";
 *
 * // Implementing a property from an interface with specific get/set handling
 * Interface.Name.property_name {
 *   get @auto; // Auto-generate getter
 *   set @empty; // Provide an empty setter
 * }
 *
 * // Implementing a class's own constructor/destructor
 * class.constructor;
 * .my_own_method @auto; // Implement local method automatically
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at the start of the implement statement
 *           (e.g., class name, '.', or @auto/@empty).
 * @param iface EINA_TRUE if parsing implements for an interface class type,
 *              EINA_FALSE otherwise. This affects validation for `class.constructor/destructor`.
 */
static void
parse_implement(Eo_Lexer *ls, Eina_Bool iface)
{
   Eina_Strbuf *buf = NULL;
   Eolian_Implement *impl = NULL;
   int iline = ls->line_number, icol = ls->column;
   if (iface)
     check_kw(ls, KW_class);
   if (ls->t.kw == KW_class)
     {
        eo_lexer_get(ls);
        check_next(ls, '.');
        if (ls->t.kw == KW_destructor)
          {
             ls->klass->class_dtor_enable = EINA_TRUE;
             eo_lexer_get(ls);
          }
        else
          {
             check_kw_next(ls, KW_constructor);
             ls->klass->class_ctor_enable = EINA_TRUE;
          }
        check_next(ls, ';');
        return;
     }
   Eina_Bool glob_auto = EINA_FALSE, glob_empty = EINA_FALSE;
   switch (ls->t.kw)
     {
        case KW_at_auto:
          glob_auto = EINA_TRUE;
          eo_lexer_get(ls);
          break;
        case KW_at_empty:
          glob_empty = EINA_TRUE;
          eo_lexer_get(ls);
          break;
        default:
          break;
     }
   if (ls->t.token == '.')
     {
        eo_lexer_get(ls);
        if (ls->t.token != TOK_VALUE)
          eo_lexer_syntax_error(ls, "name expected");
        Eina_Stringshare *iname = eina_stringshare_printf("%s.%s",
                                                          ls->klass->base.name,
                                                          ls->t.value.s);
        Eina_List *l;
        Eolian_Implement *fimp;
        EINA_LIST_FOREACH(ls->klass->implements, l, fimp)
          if (iname == fimp->base.name)
            {
               impl = fimp;
               break;
            }
        eina_stringshare_del(iname);
        if (!impl)
          {
             eo_lexer_syntax_error(ls, "implement of non-existent function");
             return;
          }
        eo_lexer_get(ls);
        goto propbeg;
     }
   else
     {
        impl = calloc(1, sizeof(Eolian_Implement));
        FILL_BASE(impl->base, ls, iline, icol, IMPLEMENT);
        ls->klass->implements = eina_list_append(ls->klass->implements, impl);
        eolian_object_ref(&impl->base);
     }
   if (ls->t.token != TOK_VALUE)
     eo_lexer_syntax_error(ls, "class name expected");
   buf = eina_strbuf_new();
   eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
   eina_strbuf_append(buf, ls->t.value.s);
   eo_lexer_get(ls);
   check_next(ls, '.');
   if (ls->t.token != TOK_VALUE)
     eo_lexer_syntax_error(ls, "name or constructor/destructor expected");
   for (;;)
     {
        if ((ls->t.kw == KW_constructor) || (ls->t.kw == KW_destructor))
          {
             eina_strbuf_append_char(buf, '.');
             eina_strbuf_append(buf, eo_lexer_keyword_str_get(ls->t.kw));
             eo_lexer_get(ls);
             check(ls, ';');
             goto propbeg;
          }
        eina_strbuf_append_char(buf, '.');
        check(ls, TOK_VALUE);
        eina_strbuf_append(buf, ls->t.value.s);
        eo_lexer_get(ls);
        if (ls->t.token != '.') break;
        eo_lexer_get(ls);
     }
propbeg:
   if (ls->t.token == '{')
     {
        Eina_Bool has_get = EINA_FALSE, has_set = EINA_FALSE;
        eo_lexer_get(ls);
        FILL_DOC(ls, impl, common_doc);
        for (;;) switch (ls->t.kw)
          {
           case KW_get:
             CASE_LOCK(ls, get, "get specifier");
             eo_lexer_get(ls);
             impl->is_prop_get = EINA_TRUE;
             impl->get_auto = glob_auto;
             impl->get_empty = glob_empty;
             if (ls->t.kw == KW_at_auto)
               {
                  impl->get_auto = EINA_TRUE;
                  eo_lexer_get(ls);
               }
             else if (ls->t.kw == KW_at_empty)
               {
                  impl->get_empty = EINA_TRUE;
                  eo_lexer_get(ls);
               }
             check_next(ls, ';');
             FILL_DOC(ls, impl, get_doc);
             break;
           case KW_set:
             CASE_LOCK(ls, set, "set specifier");
             eo_lexer_get(ls);
             impl->is_prop_set = EINA_TRUE;
             impl->set_auto = glob_auto;
             impl->set_empty = glob_empty;
             if (ls->t.kw == KW_at_auto)
               {
                  impl->set_auto = EINA_TRUE;
                  eo_lexer_get(ls);
               }
             else if (ls->t.kw == KW_at_empty)
               {
                  impl->set_empty = EINA_TRUE;
                  eo_lexer_get(ls);
               }
             check_next(ls, ';');
             FILL_DOC(ls, impl, set_doc);
             break;
           default:
             goto propend;
          }
propend:
        if (!has_get && !has_set)
          eo_lexer_syntax_error(ls, "property implements need at least get or set specified");
        check_next(ls, '}');
     }
   else
     {
        if (glob_auto)
          impl->get_auto = impl->set_auto = EINA_TRUE;
        if (glob_empty)
          impl->get_empty = impl->set_empty = EINA_TRUE;
        check_next(ls, ';');
        FILL_DOC(ls, impl, common_doc);
     }
   if (buf)
     {
        impl->base.name = eina_stringshare_add(eina_strbuf_string_get(buf));
        eo_lexer_dtor_pop(ls);
     }
}

/**
 * @brief Parses a constructor declaration for a class.
 *
 * Constructors are functions from other classes (often parents or interfaces)
 * that are designated as constructors for the current class. They can be optional.
 *
 * Example Eolian syntax:
 * @code
 * // Using a constructor from a parent/interface
 * Parent.Class.constructor_name @optional;
 *
 * // Designating a local method as a constructor (less common, typically via implements)
 * .my_local_constructor_method;
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at the start of the constructor name
 *           (e.g., class name or '.').
 */
static void
parse_constructor(Eo_Lexer *ls)
{
   Eina_Strbuf *buf = NULL;
   Eolian_Constructor *ctor = NULL;
   ctor = calloc(1, sizeof(Eolian_Constructor));
   FILL_BASE(ctor->base, ls, ls->line_number, ls->column, CONSTRUCTOR);
   ls->klass->constructors = eina_list_append(ls->klass->constructors, ctor);
   eolian_object_ref(&ctor->base);
   if (ls->t.token == '.')
     {
        check_next(ls, '.');
        if (ls->t.token != TOK_VALUE)
          eo_lexer_syntax_error(ls, "name expected");
        ctor->base.name = eina_stringshare_printf("%s.%s",
                                                  ls->klass->base.name,
                                                  ls->t.value.s);
        eo_lexer_get(ls);
        while (ls->t.kw == KW_at_optional)
          {
             if (ls->t.kw == KW_at_optional)
               {
                  ctor->is_optional = EINA_TRUE;
               }
             eo_lexer_get(ls);
          }
        check_next(ls, ';');
        return;
     }
   check(ls, TOK_VALUE);
   buf = eina_strbuf_new();
   eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
   eina_strbuf_append(buf, ls->t.value.s);
   eo_lexer_get(ls);
   check_next(ls, '.');
   check(ls, TOK_VALUE);
   for (;;)
     {
        eina_strbuf_append_char(buf, '.');
        check(ls, TOK_VALUE);
        eina_strbuf_append(buf, ls->t.value.s);
        eo_lexer_get(ls);
        if (ls->t.token != '.') break;
        eo_lexer_get(ls);
     }
   while (ls->t.kw == KW_at_optional)
     {
        if (ls->t.kw == KW_at_optional)
          {
             ctor->is_optional = EINA_TRUE;
          }
        eo_lexer_get(ls);
     }
   check_next(ls, ';');
   ctor->base.name = eina_stringshare_add(eina_strbuf_string_get(buf));
   eo_lexer_dtor_pop(ls);
}

/**
 * @brief Parses an event declaration for a class.
 *
 * Events declare named signals that a class can emit, along with the type of
 * data associated with the event. Events can have scope (@private, @protected),
 * and qualifiers (@beta, @hot, @restart).
 *
 * Example Eolian syntax:
 * @code
 * my_event_name @hot @beta : ptr(const(My_Event_Info_Struct));
 *   doc: "Description of the event.";
 * another_event, yet_another_event : void; // Multiple events with same type/doc
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at the event name.
 */
static void
parse_event(Eo_Lexer *ls)
{
   Eolian_Event *ev = calloc(1, sizeof(Eolian_Event));
   FILL_BASE(ev->base, ls, ls->line_number, ls->column, EVENT);
   ev->scope = EOLIAN_SCOPE_PUBLIC;
   Eina_Strbuf *buf = eina_strbuf_new();
   eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
   ls->klass->events = eina_list_append(ls->klass->events, ev);
   eolian_object_ref(&ev->base);
   check(ls, TOK_VALUE);
   eina_strbuf_append(buf, ls->t.value.s);
   eo_lexer_get(ls);
   while (ls->t.token == ',')
     {
        eo_lexer_get(ls);
        check(ls, TOK_VALUE);
        eina_strbuf_append_char(buf, ',');
        eina_strbuf_append(buf, ls->t.value.s);
        eo_lexer_get(ls);
     }
   ev->base.name = eina_stringshare_add(eina_strbuf_string_get(buf));
   eo_lexer_dtor_pop(ls);
   Eina_Bool has_scope = EINA_FALSE, has_beta = EINA_FALSE,
             has_hot   = EINA_FALSE, has_restart = EINA_FALSE;
   for (;;) switch (ls->t.kw)
     {
      case KW_at_private:
      case KW_at_protected:
        CASE_LOCK(ls, scope, "scope qualifier")
        ev->scope = (ls->t.kw == KW_at_private)
                     ? EOLIAN_SCOPE_PRIVATE
                     : EOLIAN_SCOPE_PROTECTED;
        eo_lexer_get(ls);
        break;
      case KW_at_beta:
        CASE_LOCK(ls, beta, "beta qualifier")
        ev->base.is_beta = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_hot:
        CASE_LOCK(ls, hot, "hot qualifier");
        ev->is_hot = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_restart:
        CASE_LOCK(ls, restart, "restart qualifier");
        ev->is_restart = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      default:
        goto end;
     }
end:
   check_next(ls, ':');
   ev->type = eo_lexer_type_release(ls, parse_type_void(ls, EINA_TRUE, EINA_TRUE));
   check(ls, ';');
   eo_lexer_get(ls);
   FILL_DOC(ls, ev, doc);
   ev->klass = ls->klass;
}

/**
 * @brief Parses a `methods { ... }` block within a class definition.
 *
 * This block contains method and property (@property) definitions.
 * @param ls The lexer state. Expects lexer at the 'methods' keyword.
 */
static void
parse_methods(Eo_Lexer *ls)
{
   int line, col;
   eo_lexer_get(ls);
   line = ls->line_number, col = ls->column;
   check_next(ls, '{');
   while (ls->t.token != '}')
     {
        if (ls->t.kw == KW_at_property)
          {
             eo_lexer_get(ls);
             parse_property(ls);
             continue;
          }
        parse_method(ls);
     }
   check_match(ls, '}', '{', line, col);
}

/**
 * @brief Parses a `parts { ... }` block within a class definition.
 *
 * This block contains part definitions.
 * @param ls The lexer state. Expects lexer at the 'parts' keyword.
 */
static void
parse_parts(Eo_Lexer *ls)
{
   int line, col;
   eo_lexer_get(ls);
   line = ls->line_number, col = ls->column;
   check_next(ls, '{');
   while (ls->t.token != '}')
     parse_part(ls);
   check_match(ls, '}', '{', line, col);
}

/**
 * @brief Parses an `implements { ... }` block within a class definition.
 *
 * This block contains implement directives.
 * @param ls The lexer state. Expects lexer at the 'implements' keyword.
 * @param iface EINA_TRUE if parsing implements for an interface class type,
 *              EINA_FALSE otherwise. Passed to parse_implement.
 */
static void
parse_implements(Eo_Lexer *ls, Eina_Bool iface)
{
   int line, col;
   eo_lexer_get(ls);
   line = ls->line_number, col = ls->column;
   check_next(ls, '{');
   while (ls->t.token != '}')
     parse_implement(ls, iface);
   check_match(ls, '}', '{', line, col);
}

/**
 * @brief Parses a `constructors { ... }` block within a class definition.
 *
 * This block contains constructor declarations.
 * @param ls The lexer state. Expects lexer at the 'constructors' keyword.
 */
static void
parse_constructors(Eo_Lexer *ls)
{
   int line, col;
   eo_lexer_get(ls);
   line = ls->line_number, col = ls->column;
   check_next(ls, '{');
   while (ls->t.token != '}')
     parse_constructor(ls);
   check_match(ls, '}', '{', line, col);
}

/**
 * @brief Parses an `events { ... }` block within a class definition.
 *
 * This block contains event declarations.
 * @param ls The lexer state. Expects lexer at the 'events' keyword.
 */
static void
parse_events(Eo_Lexer *ls)
{
   int line, col;
   eo_lexer_get(ls);
   line = ls->line_number;
   col = ls->column;
   check(ls, '{');
   eo_lexer_get(ls);
   while (ls->t.token != '}')
     parse_event(ls);
   check_match(ls, '}', '{', line, col);
}

/**
 * @brief Validates a C prefix string (for `c_prefix` or `event_c_prefix`).
 *
 * A valid prefix must start with an underscore or a lowercase letter,
 * and subsequent characters can be underscores, lowercase letters, or digits.
 * Raises a syntax error if the prefix is invalid.
 * @param ls The lexer state. Expects current token to be the prefix string.
 */
static void
_validate_pfx(Eo_Lexer *ls)
{
   char ebuf[PATH_MAX];
   check(ls, TOK_VALUE);
   const char *str = ls->t.value.s;
   if ((*str != '_') && ((*str < 'a') || (*str > 'z')))
     goto error;
   for (++str; *str; ++str)
     {
        if (*str == '_')
          continue;
        if ((*str >= 'a') && (*str <= 'z'))
          continue;
        if ((*str >= '0') && (*str <= '9'))
          continue;
        goto error;
     }
   return;
error:
   snprintf(ebuf, sizeof(ebuf), "invalid prefix '%s'", ls->t.value.s);
   eo_lexer_syntax_error(ls, ebuf);
}

/**
 * @brief Parses the body of a class definition (contents within `{ ... }`).
 *
 * This handles various class elements like documentation, c_prefix, event_c_prefix,
 * data type, methods, parts, implements, constructors, and events sections.
 *
 * @param ls The lexer state. Expects lexer to be positioned after the opening '{' of the class body.
 * @param type The type of the class being parsed (e.g., EOLIAN_CLASS_REGULAR, EOLIAN_CLASS_INTERFACE).
 */
static void
parse_class_body(Eo_Lexer *ls, Eolian_Class_Type type)
{
   Eina_Bool has_c_prefix     = EINA_FALSE,
             has_event_c_prefix = EINA_FALSE,
             has_data          = EINA_FALSE,
             has_methods       = EINA_FALSE,
             has_parts         = EINA_FALSE,
             has_implements    = EINA_FALSE,
             has_constructors  = EINA_FALSE,
             has_events        = EINA_FALSE;
   FILL_DOC(ls, ls->klass, doc);
   if (type == EOLIAN_CLASS_INTERFACE)
     {
        ls->klass->data_type = eina_stringshare_add("null");
     }
   for (;;) switch (ls->t.kw)
     {
      case KW_c_prefix:
        CASE_LOCK(ls, c_prefix, "c prefix definition")
        eo_lexer_get(ls);
        check_next(ls, ':');
        _validate_pfx(ls);
        ls->klass->c_prefix = eina_stringshare_ref(ls->t.value.s);
        eo_lexer_get(ls);
        check_next(ls, ';');
        break;
      case KW_event_c_prefix:
        CASE_LOCK(ls, event_c_prefix, "event prefix definition")
        eo_lexer_get(ls);
        check_next(ls, ':');
        _validate_pfx(ls);
        ls->klass->ev_prefix = eina_stringshare_ref(ls->t.value.s);
        eo_lexer_get(ls);
        check_next(ls, ';');
        break;
      case KW_data:
        if (type == EOLIAN_CLASS_INTERFACE) return;
        CASE_LOCK(ls, data, "data definition")
        eo_lexer_get(ls);
        check_next(ls, ':');
        check(ls, TOK_VALUE);
        ls->klass->data_type = eina_stringshare_ref(ls->t.value.s);
        eo_lexer_get(ls);
        check_next(ls, ';');
        break;
      case KW_methods:
        CASE_LOCK(ls, methods, "methods definition")
        parse_methods(ls);
        break;
      case KW_parts:
        CASE_LOCK(ls, parts, "parts definition")
        parse_parts(ls);
        break;
      case KW_implements:
        CASE_LOCK(ls, implements, "implements definition")
        parse_implements(ls, type == EOLIAN_CLASS_INTERFACE);
        break;
      case KW_constructors:
        if (type == EOLIAN_CLASS_INTERFACE || type == EOLIAN_CLASS_MIXIN)
          return;
        CASE_LOCK(ls, constructors, "constructors definition")
        parse_constructors(ls);
        break;
      case KW_events:
        CASE_LOCK(ls, events, "events definition")
        parse_events(ls);
        break;
      default:
        return;
     }
}

/**
 * @brief Parses an inherited class name and registers it as a dependency.
 *
 * This is used for `extends ParentClass` and `implements Interface1, Interface2`.
 * It checks for self-inheritance and duplicate inherits.
 *
 * @param ls The lexer state. Expects lexer at the start of the inherited class name.
 * @param buf A temporary string buffer for parsing the name.
 * @param parent EINA_TRUE if this is a parent class (from `extends`),
 *               EINA_FALSE if it's an extended interface (from `implements`).
 */
static void
_inherit_dep(Eo_Lexer *ls, Eina_Strbuf *buf, Eina_Bool parent)
{
   char ebuf[PATH_MAX];
   const char *iname;
   char *fnm;
   eina_strbuf_reset(buf);
   eo_lexer_context_push(ls);
   parse_name(ls, buf);
   iname = eina_strbuf_string_get(buf);
   fnm = database_class_to_filename(iname);
   if (compare_class_file(fnm, ls->filename))
     {
        free(fnm);
        eo_lexer_context_restore(ls);
        snprintf(ebuf, sizeof(ebuf), "class '%s' cannot inherit from itself",
                 iname);
        eo_lexer_syntax_error(ls, ebuf);
        return; /* unreachable (longjmp above), make static analysis shut up */
     }
   if (!eina_hash_find(ls->state->filenames_eo, fnm))
     {
        free(fnm);
        eo_lexer_context_restore(ls);
        snprintf(ebuf, sizeof(ebuf), "unknown inherit '%s'", iname);
        eo_lexer_syntax_error(ls, ebuf);
        return;
     }

   Eina_Stringshare *inames = eina_stringshare_add(iname), *oiname = NULL;
   /* never allow duplicate inherits */
   if (!parent)
     {
        Eina_List *l;
        if (inames == ls->klass->parent_name)
          goto inherit_dup;
        EINA_LIST_FOREACH(ls->klass->extends, l, oiname)
          {
             if (inames == oiname)
               goto inherit_dup;
          }
     }
   database_defer(ls->state, fnm, EINA_TRUE);
   if (parent)
     ls->klass->parent_name = inames;
   else
     ls->klass->extends = eina_list_append(ls->klass->extends, inames);
   free(fnm);
   eo_lexer_context_pop(ls);
   return;

inherit_dup:
   free(fnm);
   eina_stringshare_del(inames);
   eo_lexer_context_restore(ls);
   snprintf(ebuf, sizeof(ebuf), "duplicate inherit '%s'", iname);
   eo_lexer_syntax_error(ls, ebuf);
}

/**
 * @brief Parses a required interface name for a mixin class and adds it to the class's requirements.
 *
 * This is used for `requires Interface1, Interface2` in mixin definitions.
 * It checks for duplicate entries and registers the required interface as a dependency.
 *
 * @param ls The lexer state. Expects lexer at the start of the required interface name.
 * @param buf A temporary string buffer for parsing the name.
 */
static void
_requires_add(Eo_Lexer *ls, Eina_Strbuf *buf)
{
   const char *required;
   char *fnm;
   Eina_List *l;
   const char *oname;
   char ebuf[PATH_MAX];

   eina_strbuf_reset(buf);
   eo_lexer_context_push(ls);
   parse_name(ls, buf);
   required = eina_stringshare_add(eina_strbuf_string_get(buf));

   EINA_LIST_FOREACH(ls->klass->requires, l, oname)
     if (required == oname)
       {
          eo_lexer_context_restore(ls);
          eina_stringshare_del(required);
          snprintf(ebuf, sizeof(ebuf), "duplicate entry '%s'", oname);
          eo_lexer_syntax_error(ls, ebuf);
          return;
       }

   fnm = database_class_to_filename(required);

   ls->klass->requires = eina_list_append(ls->klass->requires, required);
   database_defer(ls->state, fnm, EINA_TRUE);
   eo_lexer_context_pop(ls);

   free(fnm);
}

/**
 * @brief Parses a composite interface name for a class and adds it to the class's composite list.
 *
 * This is used for `composites Interface1, Interface2`.
 * It checks for duplicate entries, unknown interfaces, and registers the composite
 * interface as a dependency.
 *
 * @param ls The lexer state. Expects lexer at the start of the composite interface name.
 * @param buf A temporary string buffer for parsing the name.
 */
static void
_composite_add(Eo_Lexer *ls, Eina_Strbuf *buf)
{
   const char *oname;
   char ebuf[PATH_MAX];
   Eina_List *l;

   eina_strbuf_reset(buf);
   eo_lexer_context_push(ls);
   parse_name(ls, buf);
   const char *nm = eina_stringshare_add(eina_strbuf_string_get(buf));

   EINA_LIST_FOREACH(ls->klass->composite, l, oname)
     if (nm == oname)
       {
          eo_lexer_context_restore(ls);
          snprintf(ebuf, sizeof(ebuf), "duplicate entry '%s'", nm);
          eina_stringshare_del(nm);
          eo_lexer_syntax_error(ls, ebuf);
          return;
       }

   char *fnm = database_class_to_filename(nm);
   if (!eina_hash_find(ls->state->filenames_eo, fnm))
     {
        free(fnm);
        eo_lexer_context_restore(ls);
        snprintf(ebuf, sizeof(ebuf), "unknown interface '%s'", nm);
        eina_stringshare_del(nm);
        eo_lexer_syntax_error(ls, ebuf);
        return;
     }
   /* composite == definitely a dependency */
   database_defer(ls->state, fnm, EINA_TRUE);
   free(fnm);
   ls->klass->composite = eina_list_append(ls->klass->composite, nm);
   eo_lexer_context_pop(ls);
}

/**
 * @brief Parses a class definition (regular, abstract, mixin, or interface).
 *
 * This handles the class name, optional @beta/@c_name qualifiers,
 * inheritance (`extends`, `implements`), mixin requirements (`requires`),
 * composite interfaces (`composites`), and the class body.
 * It also validates that the class name matches the filename.
 *
 * Example Eolian syntax:
 * @code
 * class My.Class @beta extends Parent.Class implements Iface1, Iface2 {
 *   // class body
 * }
 *
 * mixin My.Mixin requires Iface1 {
 *   // mixin body
 * }
 * @endcode
 *
 * @param ls The lexer state. Expects lexer at the class type keyword (e.g., 'class', 'mixin').
 * @param type The Eolian_Class_Type of the class to parse.
 */
static void
parse_class(Eo_Lexer *ls, Eolian_Class_Type type)
{
   const char *bnm;
   char *fnm;
   Eina_Bool same;
   int line, col;
   Eina_Strbuf *buf = eina_strbuf_new();
   eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
   ls->klass = (Eolian_Class *)eo_lexer_node_new(ls, sizeof(Eolian_Class));
   FILL_BASE(ls->klass->base, ls, ls->line_number, ls->column, CLASS);
   eo_lexer_get(ls);
   ls->klass->type = type;
   eo_lexer_context_push(ls);
   Eina_Stringshare *cname = NULL;
   Eina_Bool has_beta = EINA_FALSE, has_c_name = EINA_FALSE;
   for (;;) switch (ls->t.kw)
     {
      case KW_at_beta:
        CASE_LOCK(ls, beta, "beta qualifier");
        ls->klass->base.is_beta = EINA_TRUE;
        eo_lexer_get(ls);
        break;
      case KW_at_c_name:
        CASE_LOCK(ls, c_name, "@c_name specifier");
        cname = parse_c_name(ls);
        eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_stringshare_del), (void *)cname);
        break;
      default:
        goto tags_done;
     }
tags_done:
   parse_name(ls, buf);
   bnm = eina_stringshare_ref(ls->filename);
   fnm = database_class_to_filename(eina_strbuf_string_get(buf));
   same = compare_class_file(bnm, fnm);
   eina_stringshare_del(bnm);
   free(fnm);
   if (!same)
     {
        eo_lexer_context_restore(ls);
        eo_lexer_syntax_error(ls, "class and file names differ");
     }
   ls->klass->base.name = eina_stringshare_add(eina_strbuf_string_get(buf));
   if (cname)
     {
        ls->klass->base.c_name = eina_stringshare_ref(cname);
        eo_lexer_dtor_pop(ls);
     }
   else
     ls->klass->base.c_name = make_c_name(ls->klass->base.name);
   Eolian_Object *decl = _eolian_decl_get(ls, ls->klass->base.name);
   if (decl)
     {
        eo_lexer_context_restore(ls);
        redef_error(ls, decl, &ls->klass->base);
     }
   eo_lexer_context_pop(ls);
   eo_lexer_dtor_pop(ls);

   Eina_Bool is_reg = (type == EOLIAN_CLASS_REGULAR) || (type == EOLIAN_CLASS_ABSTRACT);
   if (ls->t.token != '{')
     {
        line = ls->line_number;
        col = ls->column;
        Eina_Strbuf *ibuf = eina_strbuf_new();
        eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), ibuf);
        /* new inherits syntax, keep alongside old for now */
        if (ls->t.kw == KW_requires)
          {
             if (type != EOLIAN_CLASS_MIXIN)
               {
                  eo_lexer_syntax_error(ls, "\"requires\" keyword is only needed for mixin classes");
               }
             eo_lexer_get(ls);
             do
               _requires_add(ls, ibuf);
             while (test_next(ls, ','));
          }

        if (ls->t.kw == KW_extends || (is_reg && (ls->t.kw == KW_implements)))
          {
             Eina_Bool ext = (ls->t.kw == KW_extends);
             eo_lexer_get(ls);
             if (is_reg && ext)
               {
                  /* regular class can have a parent, but just one */
                  _inherit_dep(ls, ibuf, EINA_TRUE);
                  /* followed by composites */
                  if (ls->t.kw == KW_composites)
                    goto noimp_comp;
                  /* if not followed by implements, we're done */
                  if (ls->t.kw != KW_implements)
                    {
                       eo_lexer_dtor_pop(ls);
                       goto inherit_done;
                    }
                  eo_lexer_get(ls);
               }
             do
               _inherit_dep(ls, ibuf, EINA_FALSE);
             while (test_next(ls, ','));
          }

noimp_comp:
        if (ls->t.kw == KW_composites)
          {
             if (type == EOLIAN_CLASS_INTERFACE)
                eo_lexer_syntax_error(ls, "interfaces cannot composite");
             eo_lexer_get(ls);
             do
               _composite_add(ls, ibuf);
             while (test_next(ls, ','));
          }

        eo_lexer_dtor_pop(ls);
     }
inherit_done:
   line = ls->line_number;
   col = ls->column;
   check_next(ls, '{');
   parse_class_body(ls, type);
   check_match(ls, '}', '{', line, col);
}

/**
 * @brief Parses a single top-level declaration unit within an Eolian file.
 *
 * This can be a class definition, an import statement, a type definition,
 * a constant, an error, a struct, or an enum.
 * For .eo files, it expects primarily a class definition.
 * For .eot files, it can parse various declarations but typically not a full class.
 *
 * @param ls The lexer state.
 * @param eot EINA_TRUE if parsing an .eot file (restricts to one class definition if any).
 * @return EINA_TRUE if a class was parsed (and thus, for .eot, no more units should be parsed),
 *         EINA_FALSE otherwise.
 */
static Eina_Bool
parse_unit(Eo_Lexer *ls, Eina_Bool eot)
{
   switch (ls->t.kw)
     {
      case KW_abstract:
        if (eot) goto def;
        parse_class(ls, EOLIAN_CLASS_ABSTRACT);
        goto found_class;
      case KW_class:
        if (eot) goto def;
        parse_class(ls, EOLIAN_CLASS_REGULAR);
        goto found_class;
      case KW_mixin:
        if (eot) goto def;
        parse_class(ls, EOLIAN_CLASS_MIXIN);
        goto found_class;
      case KW_interface:
        if (eot) goto def;
        parse_class(ls, EOLIAN_CLASS_INTERFACE);
        goto found_class;
      case KW_import:
      case KW_parse:
        {
           Eina_Bool isdep = (ls->t.kw == KW_import);
           Eina_Strbuf *buf = eina_strbuf_new();
           eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
           char errbuf[PATH_MAX];
           eo_lexer_get(ls);
           check(ls, TOK_VALUE);
           eina_strbuf_append(buf, ls->t.value.s);
           eina_strbuf_append(buf, ".eot");
           if (!eina_hash_find(ls->state->filenames_eot, eina_strbuf_string_get(buf)))
             {
                size_t buflen = eina_strbuf_length_get(buf);
                eina_strbuf_remove(buf, buflen - 1, buflen);
                if (!eina_hash_find(ls->state->filenames_eo, eina_strbuf_string_get(buf)))
                  {
                     eo_lexer_dtor_pop(ls);
                     snprintf(errbuf, sizeof(errbuf),
                              "unknown import '%s'", ls->t.value.s);
                     eo_lexer_syntax_error(ls, errbuf);
                  }
             }
           database_defer(ls->state, eina_strbuf_string_get(buf), isdep);
           eo_lexer_dtor_pop(ls);
           eo_lexer_get(ls);
           check_next(ls, ';');
           break;
        }
      case KW_type:
        {
           database_type_add(ls->unit,
             eo_lexer_typedecl_release(ls, parse_typedef(ls)));
           break;
        }
      case KW_function:
        {
           database_type_add(ls->unit,
             eo_lexer_typedecl_release(ls, parse_function_pointer(ls)));
           break;
        }
      case KW_const:
        {
           database_constant_add(ls->unit, eo_lexer_constant_release(ls, parse_constant(ls)));
           break;
        }
      case KW_error:
        database_error_add(ls->unit, eo_lexer_error_release(ls, parse_error(ls)));
        break;
      case KW_struct:
      case KW_enum:
        {
           Eina_Bool is_enum = (ls->t.kw == KW_enum);
           const char *name;
           int line, col;
           const char *freefunc = NULL, *cname = NULL;
           Eina_Strbuf *buf;
           eo_lexer_get(ls);
           Eina_Bool has_extern = EINA_FALSE, has_free   = EINA_FALSE,
                     has_beta   = EINA_FALSE, has_c_name = EINA_FALSE;
           for (;;) switch (ls->t.kw)
             {
              case KW_at_extern:
                CASE_LOCK(ls, extern, "@extern qualifier")
                eo_lexer_get(ls);
                break;
              case KW_at_beta:
                CASE_LOCK(ls, beta, "@beta qualifier")
                eo_lexer_get(ls);
                break;
              case KW_at_c_name:
                CASE_LOCK(ls, c_name, "@c_name specifier");
                cname = parse_c_name(ls);
                eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_stringshare_del), (void *)cname);
                break;
              case KW_at_free:
                {
                   CASE_LOCK(ls, free, "@free qualifier")
                   if (is_enum)
                     eo_lexer_syntax_error(ls, "enums cannot have @free");
                   eo_lexer_get(ls);
                   int pline = ls->line_number, pcol = ls->column;
                   check_next(ls, '(');
                   check(ls, TOK_VALUE);
                   freefunc = eina_stringshare_add(ls->t.value.s);
                   eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_stringshare_del), (void *)freefunc);
                   eo_lexer_get(ls);
                   check_match(ls, ')', '(', pline, pcol);
                   break;
                }
              default:
                goto postparams;
             }
postparams:
           buf = eina_strbuf_new();
           eo_lexer_dtor_push(ls, EINA_FREE_CB(eina_strbuf_free), buf);
           eo_lexer_context_push(ls);
           line = ls->line_number;
           col = ls->column;
           parse_name(ls, buf);
           name = eina_stringshare_add(eina_strbuf_string_get(buf));
           Eolian_Object *decl = _eolian_decl_get(ls, name);
           if (decl)
             {
                eina_stringshare_del(name);
                eo_lexer_context_restore(ls);
                Eolian_Typedecl tdecl;
                tdecl.base.type = EOLIAN_OBJECT_TYPEDECL;
                tdecl.type =
                  (is_enum ? EOLIAN_TYPEDECL_ENUM : EOLIAN_TYPEDECL_STRUCT);
                redef_error(ls, decl, &tdecl.base);
             }
           eo_lexer_context_pop(ls);
           eo_lexer_dtor_pop(ls);
           if (!is_enum && ls->t.token == ';')
             {
                Eolian_Typedecl *def = eo_lexer_typedecl_new(ls);
                def->is_extern = has_extern;
                def->base.is_beta = has_beta;
                def->type = EOLIAN_TYPEDECL_STRUCT_OPAQUE;
                if (freefunc)
                  {
                     def->freefunc = eina_stringshare_ref(freefunc);
                     def->ownable = EINA_TRUE;
                  }
                def->base.name = name;
                if (cname)
                  def->base.c_name = eina_stringshare_ref(cname);
                /* we can't know the order, pop when both are filled */
                if (freefunc && cname)
                  {
                     eo_lexer_dtor_pop(ls);
                     eo_lexer_dtor_pop(ls);
                  }
                else if (freefunc || cname)
                  eo_lexer_dtor_pop(ls);
                if (!def->base.c_name)
                  def->base.c_name = make_c_name(name);
                eo_lexer_get(ls);
                FILL_DOC(ls, def, doc);
                FILL_BASE(def->base, ls, line, col, TYPEDECL);
                database_struct_add(ls->unit, eo_lexer_typedecl_release(ls, def));
                break;
             }
           if (is_enum)
             parse_enum(ls, name, has_extern, has_beta, line, col, cname);
           else
             parse_struct(ls, name, has_extern, has_beta, line, col, freefunc, cname);
           break;
        }
      def:
      default:
        eo_lexer_syntax_error(ls, "invalid token");
        break;
     }
   return EINA_FALSE;
found_class:
   database_object_add(ls->unit, &ls->klass->base);
   return EINA_TRUE;
}

/**
 * @brief Parses a chunk of an Eolian file, which consists of a header and then declarations.
 *
 * The header can contain a `#version` directive. After the header, it parses
 * declaration units one by one.
 *
 * @param ls The lexer state.
 * @param eot EINA_TRUE if parsing an .eot file. This flag is passed to `parse_unit`
 *            and influences whether multiple class definitions are allowed.
 */
static void
parse_chunk(Eo_Lexer *ls, Eina_Bool eot)
{
   Eina_Bool parsing_header = EINA_TRUE;
   Eina_Bool has_version = EINA_FALSE;
   while (ls->t.token >= 0)
     switch (ls->t.kw)
       {
        case KW_hash_version:
          {
             CASE_LOCK(ls, version, "#version specifier");
             if (!parsing_header)
               eo_lexer_syntax_error(ls, "header keyword outside of unit header");
             eo_lexer_get(ls);

             check(ls, TOK_NUMBER);
             if (ls->t.kw != NUM_INT)
               eo_lexer_syntax_error(ls, "invalid #version value");
             if (ls->t.value.u > USHRT_MAX)
               eo_lexer_syntax_error(ls, "#version too high");
             else if (ls->t.value.u < 1)
               eo_lexer_syntax_error(ls, "#version too low");

             ls->unit->version = (unsigned short)(ls->t.value.u);
             if (ls->unit->version > EOLIAN_FILE_FORMAT_VERSION)
               eo_lexer_syntax_error(ls, "file version too new for this version of Eolian");
             eo_lexer_get(ls);
             break;
           }
        default:
          parsing_header = EINA_FALSE;
          /* set eot to EINA_TRUE so that we only allow parsing of one class */
          if (parse_unit(ls, eot))
            eot = EINA_TRUE;
          break;
       }
}

Eolian_Unit *
eo_parser_database_fill(Eolian_Unit *parent, const char *filename, Eina_Bool eot)
{
   int status = 0;
   const char *fsl = strrchr(filename, '/');
   const char *bsl = strrchr(filename, '\\');
   const char *fname = NULL;
   if (fsl || bsl)
     fname = eina_stringshare_add((fsl > bsl) ? (fsl + 1) : (bsl + 1));
   else
     fname = eina_stringshare_add(filename);

   Eolian_Unit *ret = eina_hash_find(parent->state->main.units, fname);
   if (!ret)
     ret = eina_hash_find(parent->state->staging.units, fname);

   if (ret)
     {
        if ((parent != ret) && !eina_hash_find(parent->children, fname))
          eina_hash_add(parent->children, fname, ret);
        eina_stringshare_del(fname);
        return ret;
     }

   Eo_Lexer *ls = eo_lexer_new(parent->state, filename);
   if (!ls)
     {
        eolian_state_log(parent->state, "unable to create lexer for file '%s'",
                         filename);
        goto error;
     }

   /* read first token */
   eo_lexer_get(ls);

   if ((status = setjmp(ls->err_jmp)))
     goto error;

   parse_chunk(ls, eot);
   if (eot) goto done;

   Eolian_Class *cl;
   if (!(cl = ls->klass))
     {
        eolian_state_log(ls->state, "no class for file '%s'", filename);
        goto error;
     }
   ls->klass = NULL;
   EOLIAN_OBJECT_ADD(ls->unit, cl->base.name, cl, classes);
   eina_hash_set(ls->state->staging.classes_f, cl->base.file, cl);
   eo_lexer_node_release(ls, &cl->base);

done:
   ret = ls->unit;
   eina_hash_add(parent->children, fname, ret);
   eina_stringshare_del(fname);

   eo_lexer_free(ls);
   return ret;

error:
   eina_stringshare_del(fname);
   eo_lexer_free(ls);
   switch (status)
     {
      case EO_LEXER_ERROR_OOM:
        eolian_state_panic(parent->state, "out of memory");
        break;
      default:
        break;
     }
   return NULL;
}
