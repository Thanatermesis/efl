#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>

#include "eolian_database.h"

/**
 * @brief Evaluates an Eolian expression.
 *
 * This function evaluates the given Eolian expression based on the provided mask.
 * It's a wrapper around database_expr_eval, passing NULL for context and unit.
 *
 * @param expr The Eolian expression to evaluate.
 * @param m The mask to apply during evaluation, determining which parts of
 *          the expression are considered.
 * @return The result of the expression evaluation as an Eolian_Value.
 *         If an error occurs or expr is NULL, an Eolian_Value with type
 *         EOLIAN_EXPR_UNKNOWN is returned.
 */
EOLIAN_API Eolian_Value
eolian_expression_eval(const Eolian_Expression *expr, Eolian_Expression_Mask m)
{
   Eolian_Value err;
   err.type = EOLIAN_EXPR_UNKNOWN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(expr, err);
   return database_expr_eval(NULL, (Eolian_Expression *)expr, m, NULL, NULL);
}

/**
 * @brief Evaluates an Eolian expression and fills a provided Eolian_Value structure.
 *
 * This function evaluates the given Eolian expression based on the provided mask
 * and stores the result in the Eolian_Value pointed to by @p val.
 *
 * @param expr The Eolian expression to evaluate.
 * @param m The mask to apply during evaluation.
 * @param val A pointer to an Eolian_Value structure to be filled with the result.
 * @return @c EINA_TRUE on successful evaluation, @c EINA_FALSE otherwise (e.g., if
 *         expr is NULL or evaluation fails).
 */
EOLIAN_API Eina_Bool
eolian_expression_eval_fill(const Eolian_Expression *expr,
                            Eolian_Expression_Mask m, Eolian_Value *val)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(expr, EINA_FALSE);
   Eolian_Value ret = database_expr_eval(NULL, (Eolian_Expression *)expr, m,
                                         NULL, NULL);
   if (ret.type == EOLIAN_EXPR_UNKNOWN)
     return EINA_FALSE;
   *val = ret;
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Appends a character to a string buffer, escaping it if necessary.
 *
 * This function checks if the character @p c is a special character
 * (e.g., newline, tab, quote) and appends its escaped representation
 * (e.g., "\\n", "\\t", "\\'") to the Eina_Strbuf @p buf.
 * Non-printable characters are appended as hexadecimal escape sequences (e.g., "\\x1F").
 * Other characters are appended as is.
 *
 * @param buf The string buffer to append to.
 * @param c The character to append (and potentially escape).
 */
static void
_append_char_escaped(Eina_Strbuf *buf, char c)
{
   switch (c)
     {
      case '\'': eina_strbuf_append(buf, "\\\'"); break;
      case '\"': eina_strbuf_append(buf, "\\\""); break;
      case '\?': eina_strbuf_append(buf, "\\\?"); break;
      case '\\': eina_strbuf_append(buf, "\\\\"); break;
      case '\a': eina_strbuf_append(buf, "\\a"); break;
      case '\b': eina_strbuf_append(buf, "\\b"); break;
      case '\f': eina_strbuf_append(buf, "\\f"); break;
      case '\n': eina_strbuf_append(buf, "\\n"); break;
      case '\r': eina_strbuf_append(buf, "\\r"); break;
      case '\t': eina_strbuf_append(buf, "\\t"); break;
      case '\v': eina_strbuf_append(buf, "\\v"); break;
      default:
         if ((c < 32) || (c > 126))
           eina_strbuf_append_printf(buf, "\\x%X", (unsigned char)c);
         else
           eina_strbuf_append_char(buf, c);
         break;
     }
}

/**
 * @internal
 * @brief Converts a numerical Eolian_Value to its string representation.
 *
 * This function appends the string representation of a numerical Eolian_Value
 * (integer, float, etc.) to the given Eina_Strbuf. It includes type suffixes
 * like 'U', 'L', 'UL', 'LL', 'ULL', 'f' as appropriate for C literals.
 *
 * @param v A pointer to the Eolian_Value containing the number.
 *          The type of this value must be one of the numerical EOLIAN_EXPR types.
 * @param buf The string buffer to append the number string to.
 */
static void
_number_to_str(const Eolian_Value *v, Eina_Strbuf *buf)
{
   switch (v->type)
     {
      case EOLIAN_EXPR_INT:
        eina_strbuf_append_printf(buf, "%d", v->value.i); break;
      case EOLIAN_EXPR_UINT:
        eina_strbuf_append_printf(buf, "%uU", v->value.u); break;
      case EOLIAN_EXPR_LONG:
        eina_strbuf_append_printf(buf, "%ldL", v->value.l); break;
      case EOLIAN_EXPR_ULONG:
        eina_strbuf_append_printf(buf, "%luUL", v->value.ul); break;
      case EOLIAN_EXPR_LLONG:
        eina_strbuf_append_printf(buf, "%ldLL", (long)v->value.ll); break;
      case EOLIAN_EXPR_ULLONG:
        eina_strbuf_append_printf(buf, "%luULL", (unsigned long)v->value.ull);
        break;
      case EOLIAN_EXPR_FLOAT:
        eina_strbuf_append_printf(buf, "%ff", v->value.f); break;
      case EOLIAN_EXPR_DOUBLE:
        eina_strbuf_append_printf(buf, "%f", v->value.d); break;
      default:
        break;
     }
}

/**
 * @brief Converts an Eolian_Value to its C literal string representation.
 *
 * This function takes an Eolian_Value and returns a stringshare string
 * that represents this value as a C literal.
 * For example:
 * - A boolean true becomes "EINA_TRUE".
 * - A character 'a' becomes "'a'".
 * - A string "foo" becomes "\"foo\"".
 * - An integer 5 becomes "5".
 * - NULL becomes "NULL".
 *
 * @param val The Eolian_Value to convert.
 * @return A new Eina_Stringshare string representing the C literal,
 *         or @c NULL if the value type is not supported or @p val is @c NULL.
 *         The caller is responsible for freeing the returned stringshare
 *         using eina_stringshare_del().
 */
EOLIAN_API Eina_Stringshare *
eolian_expression_value_to_literal(const Eolian_Value *val)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(val, NULL);
   switch (val->type)
     {
      case EOLIAN_EXPR_BOOL:
        return eina_stringshare_add(val->value.b ? "EINA_TRUE"
                                                 : "EINA_FALSE");
      case EOLIAN_EXPR_NULL:
        return eina_stringshare_add("NULL");
      case EOLIAN_EXPR_CHAR:
        {
           char c = val->value.c;
           Eina_Strbuf *buf = eina_strbuf_new();
           const char *ret;
           eina_strbuf_append_char(buf, '\'');
           _append_char_escaped(buf, c);
           eina_strbuf_append_char(buf, '\'');
           ret = eina_stringshare_add(eina_strbuf_string_get(buf));
           eina_strbuf_free(buf);
           return ret;
        }
      case EOLIAN_EXPR_STRING:
        {
           const char *ret;
           char *c = (char*)val->value.s;
           Eina_Strbuf *buf = eina_strbuf_new();
           eina_strbuf_append_char(buf, '\"');
           while (*c) _append_char_escaped(buf, *(c++));
           eina_strbuf_append_char(buf, '\"');
           ret = eina_stringshare_add(eina_strbuf_string_get(buf));
           eina_strbuf_free(buf);
           return ret;
        }
      case EOLIAN_EXPR_INT:
      case EOLIAN_EXPR_UINT:
      case EOLIAN_EXPR_LONG:
      case EOLIAN_EXPR_ULONG:
      case EOLIAN_EXPR_LLONG:
      case EOLIAN_EXPR_ULLONG:
      case EOLIAN_EXPR_FLOAT:
      case EOLIAN_EXPR_DOUBLE:
        {
           const char *ret;
           Eina_Strbuf *buf = eina_strbuf_new();
           _number_to_str(val, buf);
           ret = eina_stringshare_add(eina_strbuf_string_get(buf));
           eina_strbuf_free(buf);
           return ret;
        }
      default:
        return NULL;
     }
}

/**
 * @internal
 * @brief Array of C binary operator strings.
 * Indexed by Eolian_Binary_Operator enum values.
 * Provides the string representation for each binary operator (e.g., "+", "==").
 */
static const char *_binops[] = {
    "+", "-", "*", "/", "%",
    "==", "!=", ">", "<", ">=", "<=",
    "&&", "||",
    "&", "|", "^", "<<", ">>"
};

/**
 * @internal
 * @brief Array of C unary operator strings.
 * Indexed by Eolian_Unary_Operator enum values.
 * Provides the string representation for each unary operator (e.g., "-", "!").
 */
static const char *_unops[] = {
    "-", "+", "!", "~"
};

/**
 * @internal
 * @brief Recursively serializes an Eolian_Expression to a string buffer.
 *
 * This function converts an Eolian expression tree into its string representation.
 * For example, a binary expression `a + b` would be serialized to "(a + b)"
 * if it's a sub-expression, or "a + b" if it's the outermost expression.
 * Literals are converted using eolian_expression_value_to_literal().
 *
 * @param expr The Eolian_Expression to serialize.
 * @param buf The Eina_Strbuf to append the serialized expression to.
 * @param outer A boolean flag indicating if this is the outermost expression
 *              (EINA_TRUE) or a sub-expression (EINA_FALSE). This affects
 *              whether parentheses are added around binary expressions.
 * @return @c EINA_TRUE on successful serialization, @c EINA_FALSE if an error
 *         occurred (e.g., unknown expression type).
 */
static Eina_Bool
_expr_serialize(const Eolian_Expression *expr, Eina_Strbuf *buf, Eina_Bool outer)
{
   switch (expr->type)
     {
      case EOLIAN_EXPR_UNKNOWN:
        return EINA_FALSE;
      case EOLIAN_EXPR_INT:
      case EOLIAN_EXPR_UINT:
      case EOLIAN_EXPR_LONG:
      case EOLIAN_EXPR_ULONG:
      case EOLIAN_EXPR_LLONG:
      case EOLIAN_EXPR_ULLONG:
      case EOLIAN_EXPR_FLOAT:
      case EOLIAN_EXPR_DOUBLE:
      case EOLIAN_EXPR_STRING:
      case EOLIAN_EXPR_CHAR:
        {
           Eolian_Value v;
           v.type = expr->type;
           v.value = expr->value;
           const char *x = eolian_expression_value_to_literal(&v);
           if (!x)
             return EINA_FALSE;
           eina_strbuf_append(buf, x);
           eina_stringshare_del(x);
           break;
        }
      case EOLIAN_EXPR_NULL:
        eina_strbuf_append(buf, "null");
        break;
      case EOLIAN_EXPR_BOOL:
        eina_strbuf_append(buf, expr->value.b ? "true" : "false");
        break;
      case EOLIAN_EXPR_NAME:
        {
           eina_strbuf_append(buf, expr->value.s);
           break;
        }
      case EOLIAN_EXPR_UNARY:
        eina_strbuf_append(buf, _unops[expr->unop]);
        _expr_serialize(expr->expr, buf, EINA_FALSE);
        break;
      case EOLIAN_EXPR_BINARY:
        if (!outer)
          eina_strbuf_append_char(buf, '(');
        _expr_serialize(expr->lhs, buf, EINA_FALSE);
        eina_strbuf_append_printf(buf, " %s ", _binops[expr->binop]);
        _expr_serialize(expr->rhs, buf, EINA_FALSE);
        if (!outer)
          eina_strbuf_append_char(buf, ')');
        break;
      default:
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @brief Serializes an Eolian_Expression into a human-readable string.
 *
 * This function converts an entire Eolian expression tree into its string
 * representation. For example, an expression representing `(2 + foo) * 3`
 * would be serialized to the string "((2 + foo) * 3)".
 *
 * @param expr The Eolian_Expression to serialize.
 * @return A new Eina_Stringshare string containing the serialized expression,
 *         or @c NULL if @p expr is @c NULL or serialization fails.
 *         The caller is responsible for freeing the returned stringshare
 *         using eina_stringshare_del().
 */
EOLIAN_API Eina_Stringshare *
eolian_expression_serialize(const Eolian_Expression *expr)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(expr, NULL);
   Eina_Strbuf *buf = eina_strbuf_new();
   if (!_expr_serialize(expr, buf, EINA_TRUE))
     {
        eina_strbuf_free(buf);
        return NULL;
     }
   const char *ret = eina_stringshare_add(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Gets the type of an Eolian expression.
 *
 * @param expr The Eolian expression.
 * @return The Eolian_Expression_Type of the expression, or
 *         EOLIAN_EXPR_UNKNOWN if @p expr is @c NULL.
 */
EOLIAN_API Eolian_Expression_Type
eolian_expression_type_get(const Eolian_Expression *expr)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(expr, EOLIAN_EXPR_UNKNOWN);
   return expr->type;
}

/**
 * @brief Gets the binary operator from a binary Eolian expression.
 *
 * @param expr The Eolian expression. Must be of type EOLIAN_EXPR_BINARY.
 * @return The Eolian_Binary_Operator if the expression is a binary operation,
 *         EOLIAN_BINOP_INVALID otherwise (e.g., if @p expr is @c NULL or not
 *         a binary expression).
 */
EOLIAN_API Eolian_Binary_Operator
eolian_expression_binary_operator_get(const Eolian_Expression *expr)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(expr, EOLIAN_BINOP_INVALID);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(expr->type == EOLIAN_EXPR_BINARY,
                                   EOLIAN_BINOP_INVALID);
   return expr->binop;
}

/**
 * @brief Gets the left-hand side (LHS) sub-expression of a binary Eolian expression.
 *
 * @param expr The Eolian expression. Must be of type EOLIAN_EXPR_BINARY.
 * @return A pointer to the LHS Eolian_Expression, or @c NULL if @p expr is @c NULL
 *         or not a binary expression. The returned expression is part of the
 *         original expression tree and should not be freed separately.
 */
EOLIAN_API const Eolian_Expression *
eolian_expression_binary_lhs_get(const Eolian_Expression *expr)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(expr, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(expr->type == EOLIAN_EXPR_BINARY, NULL);
   return expr->lhs;
}

/**
 * @brief Gets the right-hand side (RHS) sub-expression of a binary Eolian expression.
 *
 * @param expr The Eolian expression. Must be of type EOLIAN_EXPR_BINARY.
 * @return A pointer to the RHS Eolian_Expression, or @c NULL if @p expr is @c NULL
 *         or not a binary expression. The returned expression is part of the
 *         original expression tree and should not be freed separately.
 */
EOLIAN_API const Eolian_Expression *
eolian_expression_binary_rhs_get(const Eolian_Expression *expr)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(expr, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(expr->type == EOLIAN_EXPR_BINARY, NULL);
   return expr->rhs;
}

/**
 * @brief Gets the unary operator from a unary Eolian expression.
 *
 * @param expr The Eolian expression. Must be of type EOLIAN_EXPR_UNARY.
 * @return The Eolian_Unary_Operator if the expression is a unary operation,
 *         EOLIAN_UNOP_INVALID otherwise (e.g., if @p expr is @c NULL or not
 *         a unary expression).
 */
EOLIAN_API Eolian_Unary_Operator
eolian_expression_unary_operator_get(const Eolian_Expression *expr)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(expr, EOLIAN_UNOP_INVALID);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(expr->type == EOLIAN_EXPR_UNARY,
                                   EOLIAN_UNOP_INVALID);
   return expr->unop;
}

/**
 * @brief Gets the sub-expression of a unary Eolian expression.
 *
 * This is the expression upon which the unary operator acts.
 * For example, in `-x`, `x` is the sub-expression.
 *
 * @param expr The Eolian expression. Must be of type EOLIAN_EXPR_UNARY.
 * @return A pointer to the sub-Eolian_Expression, or @c NULL if @p expr is @c NULL
 *         or not a unary expression. The returned expression is part of the
 *         original expression tree and should not be freed separately.
 */
EOLIAN_API const Eolian_Expression *
eolian_expression_unary_expression_get(const Eolian_Expression *expr)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(expr, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(expr->type == EOLIAN_EXPR_UNARY, NULL);
   return expr->expr;
}

/**
 * @brief Gets the Eolian_Value from an expression node that directly holds a value.
 *
 * This function is applicable to expression nodes that are literals (like integers,
 * strings, booleans, null) or named constants that have resolved to a value.
 * It is not applicable for binary or unary operations, or unresolved names.
 *
 * @param expr The Eolian expression. Must be a value-holding type (not UNKNOWN,
 *             BINARY, or UNARY).
 * @return An Eolian_Value structure containing the type and value. If @p expr is
 *         @c NULL or not a value-holding type, an Eolian_Value with type
 *         EOLIAN_EXPR_UNKNOWN is returned.
 */
EOLIAN_API Eolian_Value
eolian_expression_value_get(const Eolian_Expression *expr)
{
   Eolian_Value v;
   v.type = EOLIAN_EXPR_UNKNOWN;
   EINA_SAFETY_ON_NULL_RETURN_VAL(expr, v);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(expr->type != EOLIAN_EXPR_UNKNOWN
                                && expr->type != EOLIAN_EXPR_BINARY
                                && expr->type != EOLIAN_EXPR_UNARY, v);
   v.type  = expr->type;
   v.value = expr->value;
   return v;
}

/**
 * @brief Fills an Eolian_Value structure from an expression node that directly holds a value.
 *
 * Similar to eolian_expression_value_get(), but fills a pre-allocated
 * Eolian_Value structure instead of returning by value.
 * This function is applicable to expression nodes that are literals or resolved constants.
 *
 * @param expr The Eolian expression. Must be a value-holding type.
 * @param val A pointer to an Eolian_Value structure to be filled.
 * @return @c EINA_TRUE if @p val was successfully filled, @c EINA_FALSE otherwise
 *         (e.g., if @p expr is @c NULL, or not a value-holding type, or @p val is @c NULL,
 *         though the function doesn't explicitly check for @p val being @c NULL).
 */
EOLIAN_API Eina_Bool
eolian_expression_value_get_fill(const Eolian_Expression *expr, Eolian_Value *val)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(expr, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(expr->type != EOLIAN_EXPR_UNKNOWN
                                && expr->type != EOLIAN_EXPR_BINARY
                                && expr->type != EOLIAN_EXPR_UNARY, EINA_FALSE);
   val->type  = expr->type;
   val->value = expr->value;
   return EINA_TRUE;
}
