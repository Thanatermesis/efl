#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"
#include "eo_lexer.h"

/**
 * @file database_type_api.c
 * @brief Implementation of Eolian type information retrieval functions.
 *
 * This file contains functions to access various properties of Eolian
 * types and type declarations, such as their kind, underlying C types,
 * documentation, and associated data structures (like struct fields or
 * enum values).
 */

/**
 * @brief Gets the general kind of an Eolian type.
 *
 * @param[in] tp The Eolian type.
 * @return The general kind of the type (e.g., regular, class, error).
 *         Returns #EOLIAN_TYPE_UNKNOWN_TYPE if tp is NULL.
 */
EOLIAN_API Eolian_Type_Type
eolian_type_type_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EOLIAN_TYPE_UNKNOWN_TYPE);
   return tp->type;
}

/**
 * @brief Gets the specific built-in type if the Eolian type is a built-in.
 *
 * @param[in] tp The Eolian type.
 * @return The specific built-in type (e.g., int, float, string).
 *         Returns #EOLIAN_TYPE_BUILTIN_INVALID if tp is NULL or not a built-in type.
 */
EOLIAN_API Eolian_Type_Builtin_Type
eolian_type_builtin_type_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EOLIAN_TYPE_BUILTIN_INVALID);
   return tp->btype;
}

/**
 * @brief Gets the kind of an Eolian type declaration.
 *
 * @param[in] tp The Eolian type declaration.
 * @return The kind of the type declaration (e.g., struct, enum, alias).
 *         Returns #EOLIAN_TYPEDECL_UNKNOWN if tp is NULL.
 */
EOLIAN_API Eolian_Typedecl_Type
eolian_typedecl_type_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EOLIAN_TYPEDECL_UNKNOWN);
   return tp->type;
}

/**
 * @brief Gets an iterator over the fields of a struct type declaration.
 *
 * @param[in] tp The Eolian type declaration, must be a struct.
 * @return An Eina_Iterator for #Eolian_Struct_Type_Field elements.
 *         Returns NULL if tp is NULL or not a struct type.
 *         The caller is responsible for freeing the iterator.
 */
EOLIAN_API Eina_Iterator *
eolian_typedecl_struct_fields_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (tp->type != EOLIAN_TYPEDECL_STRUCT)
     return NULL;
   return eina_list_iterator_new(tp->field_list);
}

/**
 * @brief Gets a specific field of a struct type declaration by its name.
 *
 * @param[in] tp The Eolian type declaration, must be a struct.
 * @param[in] field The name of the field to retrieve.
 * @return A pointer to the #Eolian_Struct_Type_Field if found.
 *         Returns NULL if tp is NULL, field is NULL, tp is not a struct,
 *         or the field is not found.
 */
EOLIAN_API const Eolian_Struct_Type_Field *
eolian_typedecl_struct_field_get(const Eolian_Typedecl *tp, const char *field)
{
   Eolian_Struct_Type_Field *sf = NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(field, NULL);
   if (tp->type != EOLIAN_TYPEDECL_STRUCT)
     return NULL;
   sf = eina_hash_find(tp->fields, field);
   if (!sf) return NULL;
   return sf;
}

/**
 * @brief Gets the documentation for a struct field.
 *
 * @param[in] fl The Eolian struct type field.
 * @return A pointer to the #Eolian_Documentation for the field.
 *         Returns NULL if fl is NULL or has no documentation.
 */
EOLIAN_API const Eolian_Documentation *
eolian_typedecl_struct_field_documentation_get(const Eolian_Struct_Type_Field *fl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   return fl->doc;
}

/**
 * @brief Gets the Eolian type of a struct field.
 *
 * @param[in] fl The Eolian struct type field.
 * @return A pointer to the #Eolian_Type of the field.
 *         Returns NULL if fl is NULL.
 */
EOLIAN_API const Eolian_Type *
eolian_typedecl_struct_field_type_get(const Eolian_Struct_Type_Field *fl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   return fl->type;
}

/**
 * @brief Checks if a struct field is passed by reference.
 *
 * @param[in] fl The Eolian struct type field.
 * @return #EINA_TRUE if the field is by reference, #EINA_FALSE otherwise.
 *         Returns #EINA_FALSE if fl is NULL.
 */
EOLIAN_API Eina_Bool
eolian_typedecl_struct_field_is_by_ref(const Eolian_Struct_Type_Field *fl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, EINA_FALSE);
   return fl->by_ref;
}

/**
 * @brief Checks if a struct field has "move" semantics.
 *
 * This typically means ownership of the underlying data is transferred.
 *
 * @param[in] fl The Eolian struct type field.
 * @return #EINA_TRUE if the field has move semantics, #EINA_FALSE otherwise.
 *         Returns #EINA_FALSE if fl is NULL.
 */
EOLIAN_API Eina_Bool
eolian_typedecl_struct_field_is_move(const Eolian_Struct_Type_Field *fl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, EINA_FALSE);
   return fl->move;
}

/**
 * @brief Gets the C type string for a struct field.
 *
 * This function generates the C type representation for the given struct field,
 * considering whether it's passed by reference.
 * For example, for a field `int_val: int;` it would return `"int"`.
 * For a field `str_val: string @by_ref;` it might return `"const char **"` or similar.
 *
 * @param[in] fl The Eolian struct type field.
 * @return An Eina_Stringshare containing the C type string.
 *         The caller is responsible for unreferencing the stringshare
 *         when it's no longer needed (e.g., using eina_stringshare_del()).
 *         Returns NULL if fl is NULL.
 */
EOLIAN_API Eina_Stringshare *
eolian_typedecl_struct_field_c_type_get(const Eolian_Struct_Type_Field *fl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   Eina_Strbuf *buf = eina_strbuf_new();
   database_type_to_str(fl->type, buf, NULL, EOLIAN_C_TYPE_DEFAULT, fl->by_ref);
   Eina_Stringshare *ret = eina_stringshare_add(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Gets an iterator over the fields of an enum type declaration.
 *
 * @param[in] tp The Eolian type declaration, must be an enum.
 * @return An Eina_Iterator for #Eolian_Enum_Type_Field elements.
 *         Returns NULL if tp is NULL or not an enum type.
 *         The caller is responsible for freeing the iterator.
 */
EOLIAN_API Eina_Iterator *
eolian_typedecl_enum_fields_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (tp->type != EOLIAN_TYPEDECL_ENUM)
     return NULL;
   return eina_list_iterator_new(tp->field_list);
}

/**
 * @brief Gets a specific field of an enum type declaration by its name.
 *
 * @param[in] tp The Eolian type declaration, must be an enum.
 * @param[in] field The name of the enum field to retrieve.
 * @return A pointer to the #Eolian_Enum_Type_Field if found.
 *         Returns NULL if tp is NULL, field is NULL, tp is not an enum,
 *         or the field is not found.
 */
EOLIAN_API const Eolian_Enum_Type_Field *
eolian_typedecl_enum_field_get(const Eolian_Typedecl *tp, const char *field)
{
   Eolian_Enum_Type_Field *ef = NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(field, NULL);
   if (tp->type != EOLIAN_TYPEDECL_ENUM)
     return NULL;
   ef = eina_hash_find(tp->fields, field);
   if (!ef) return NULL;
   return ef;
}

/**
 * @brief Gets the C constant name for an enum field.
 *
 * This constructs the typical C preprocessor macro name for an enum value.
 * For example, if an enum `My_Enum` has a field `first_value`, this might
 * return `"MY_ENUM_FIRST_VALUE"`. It also considers legacy prefixes.
 *
 * @param[in] fl The Eolian enum type field.
 * @return An Eina_Stringshare containing the C constant name.
 *         The caller is responsible for unreferencing the stringshare.
 *         Returns NULL if fl is NULL.
 */
EOLIAN_API Eina_Stringshare *
eolian_typedecl_enum_field_c_constant_get(const Eolian_Enum_Type_Field *fl)
{
   Eina_Stringshare *ret;
   Eina_Strbuf *buf;
   char *bufp, *p;
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   buf = eina_strbuf_new();
   if (fl->base_enum->legacy)
     eina_strbuf_append(buf, fl->base_enum->legacy);
   else
     eina_strbuf_append(buf, fl->base_enum->base.c_name);
   eina_strbuf_append_char(buf, '_');
   eina_strbuf_append(buf, fl->base.name);
   bufp = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   eina_str_toupper(&bufp);
   while ((p = strchr(bufp, '.'))) *p = '_';
   ret = eina_stringshare_add(bufp);
   free(bufp);
   return ret;
}

/**
 * @brief Gets the documentation for an enum field.
 *
 * @param[in] fl The Eolian enum type field.
 * @return A pointer to the #Eolian_Documentation for the field.
 *         Returns NULL if fl is NULL or has no documentation.
 */
EOLIAN_API const Eolian_Documentation *
eolian_typedecl_enum_field_documentation_get(const Eolian_Enum_Type_Field *fl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   return fl->doc;
}

/**
 * @brief Gets the value (as an expression) of an enum field.
 *
 * Enum fields can have explicit values assigned to them. This function
 * retrieves that value.
 *
 * @param[in] fl The Eolian enum type field.
 * @param[in] force If #EINA_TRUE, returns the value even if it's not public.
 *                  If #EINA_FALSE, only returns public values.
 * @return A pointer to the #Eolian_Expression representing the field's value.
 *         Returns NULL if fl is NULL, or if force is #EINA_FALSE and the
 *         value is not public.
 */
EOLIAN_API const Eolian_Expression *
eolian_typedecl_enum_field_value_get(const Eolian_Enum_Type_Field *fl, Eina_Bool force)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   if (!force && !fl->is_public_value) return NULL;
   return fl->value;
}

/**
 * @brief Gets the legacy prefix for an enum type declaration.
 *
 * Some enums might have a legacy prefix used in C code generation.
 *
 * @param[in] tp The Eolian type declaration, must be an enum.
 * @return The legacy prefix string if it exists.
 *         Returns NULL if tp is NULL, not an enum, or has no legacy prefix.
 */
EOLIAN_API const char *
eolian_typedecl_enum_legacy_prefix_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (tp->type != EOLIAN_TYPEDECL_ENUM)
     return NULL;
   return tp->legacy;
}

/**
 * @brief Gets the documentation for a type declaration.
 *
 * @param[in] tp The Eolian type declaration.
 * @return A pointer to the #Eolian_Documentation for the type declaration.
 *         Returns NULL if tp is NULL or has no documentation.
 */
EOLIAN_API const Eolian_Documentation *
eolian_typedecl_documentation_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->doc;
}

/**
 * @brief Gets the base type of an Eolian type.
 *
 * For aliased types, this returns the type it aliases. For other types,
 * it might return the type itself or a more fundamental type it's based on.
 *
 * @param[in] tp The Eolian type.
 * @return A pointer to the base #Eolian_Type.
 *         Returns NULL if tp is NULL.
 */
EOLIAN_API const Eolian_Type *
eolian_type_base_type_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->base_type;
}

/**
 * @brief Gets the next type in a sequence of types (e.g., for function pointers or complex types).
 *
 * This is used for types that are composed of other types, like a pointer to a function
 * that returns a pointer. `next_type` would point to the subsequent type in such a chain.
 *
 * @param[in] tp The Eolian type.
 * @return A pointer to the next #Eolian_Type in the sequence.
 *         Returns NULL if tp is NULL or there is no next type.
 */
EOLIAN_API const Eolian_Type *
eolian_type_next_type_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->next_type;
}

/**
 * @brief Gets the type declaration associated with a regular Eolian type.
 *
 * If the Eolian type is of kind #EOLIAN_TYPE_REGULAR, this function
 * returns its corresponding #Eolian_Typedecl (e.g., a struct or enum definition).
 *
 * @param[in] tp The Eolian type.
 * @return A pointer to the #Eolian_Typedecl.
 *         Returns NULL if tp is NULL or not a regular type.
 */
EOLIAN_API const Eolian_Typedecl *
eolian_type_typedecl_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (eolian_type_type_get(tp) != EOLIAN_TYPE_REGULAR)
     return NULL;
   return tp->tdecl;
}

/**
 * @brief Gets the base type of an Eolian type declaration.
 *
 * For alias type declarations, this returns the #Eolian_Type that the alias refers to.
 *
 * @param[in] tp The Eolian type declaration.
 * @return A pointer to the base #Eolian_Type.
 *         Returns NULL if tp is NULL.
 */
EOLIAN_API const Eolian_Type *
eolian_typedecl_base_type_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->base_type;
}

/**
 * @brief Recursively resolves an Eolian type to its ultimate base type if it's an alias.
 *
 * If the given type is an alias, this function will follow the chain of aliases
 * until it reaches a non-alias type. If the type is not an alias, or is a pointer,
 * it returns the type itself.
 *
 * @param[in] tp The Eolian type.
 * @return The ultimate base #Eolian_Type after resolving aliases.
 *         Returns tp itself if it's not an alias or is a pointer.
 *         Returns NULL if the input tp is NULL and it's not a pointer type.
 */
EOLIAN_API const Eolian_Type *
eolian_type_aliased_base_get(const Eolian_Type *tp)
{
   if (!tp || tp->type != EOLIAN_TYPE_REGULAR || tp->is_ptr)
     return tp;
   const Eolian_Typedecl *btp = eolian_type_typedecl_get(tp);
   if (btp && (btp->type == EOLIAN_TYPEDECL_ALIAS))
     return eolian_typedecl_aliased_base_get(btp);
   return tp;
}

/**
 * @brief Recursively resolves an Eolian type declaration to its ultimate base type if it's an alias.
 *
 * If the given type declaration is an alias, this function will follow the chain of aliases
 * via its base_type until it reaches a non-alias type.
 *
 * @param[in] tp The Eolian type declaration.
 * @return The ultimate base #Eolian_Type after resolving aliases.
 *         Returns NULL if tp is NULL or not an alias type declaration.
 */
EOLIAN_API const Eolian_Type *
eolian_typedecl_aliased_base_get(const Eolian_Typedecl *tp)
{
   if (!tp || tp->type != EOLIAN_TYPEDECL_ALIAS)
     return NULL;
   return eolian_type_aliased_base_get(tp->base_type);
}

/**
 * @brief Gets the Eolian class associated with a class type.
 *
 * If the Eolian type is of kind #EOLIAN_TYPE_CLASS, this function
 * returns its corresponding #Eolian_Class definition.
 *
 * @param[in] tp The Eolian type.
 * @return A pointer to the #Eolian_Class.
 *         Returns NULL if tp is NULL or not a class type.
 */
EOLIAN_API const Eolian_Class *
eolian_type_class_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (eolian_type_type_get(tp) != EOLIAN_TYPE_CLASS)
     return NULL;
   return tp->klass;
}

/**
 * @brief Gets the Eolian error associated with an error type.
 *
 * If the Eolian type is of kind #EOLIAN_TYPE_ERROR, this function
 * returns its corresponding #Eolian_Error definition.
 *
 * @param[in] tp The Eolian type.
 * @return A pointer to the #Eolian_Error.
 *         Returns NULL if tp is NULL or not an error type.
 */
EOLIAN_API const Eolian_Error *
eolian_type_error_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (eolian_type_type_get(tp) != EOLIAN_TYPE_ERROR)
     return NULL;
   return tp->error;
}

/**
 * @brief Checks if an Eolian type has "move" semantics.
 *
 * This typically means ownership of the underlying data is transferred when
 * the type is used (e.g., as a return value or parameter).
 *
 * @param[in] tp The Eolian type.
 * @return #EINA_TRUE if the type has move semantics, #EINA_FALSE otherwise.
 *         Returns #EINA_FALSE if tp is NULL.
 */
EOLIAN_API Eina_Bool
eolian_type_is_move(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EINA_FALSE);
   return tp->move;
}

/**
 * @brief Checks if an Eolian type is marked as const.
 *
 * @param[in] tp The Eolian type.
 * @return #EINA_TRUE if the type is const, #EINA_FALSE otherwise.
 *         Returns #EINA_FALSE if tp is NULL.
 */
EOLIAN_API Eina_Bool
eolian_type_is_const(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EINA_FALSE);
   return tp->is_const;
}

/**
 * @brief Checks if an Eolian type is a pointer.
 *
 * @param[in] tp The Eolian type.
 * @return #EINA_TRUE if the type is a pointer, #EINA_FALSE otherwise.
 *         Returns #EINA_FALSE if tp is NULL.
 */
EOLIAN_API Eina_Bool
eolian_type_is_ptr(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EINA_FALSE);
   return tp->is_ptr;
}

/**
 * @brief Checks if an Eolian type declaration is marked as extern.
 *
 * Extern types are typically defined outside the current Eolian scope,
 * often in external C libraries.
 *
 * @param[in] tp The Eolian type declaration.
 * @return #EINA_TRUE if the type declaration is extern, #EINA_FALSE otherwise.
 *         Returns #EINA_FALSE if tp is NULL.
 */
EOLIAN_API Eina_Bool
eolian_typedecl_is_extern(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EINA_FALSE);
   return tp->is_extern;
}

/**
 * @brief Gets the C type string for an Eolian type.
 *
 * This function generates the C type representation for the given Eolian type.
 * For example, for an Eolian type `integer` it would return `"int"`.
 * For an Eolian type `string` it would return `"const char *"`.
 *
 * @param[in] tp The Eolian type.
 * @return An Eina_Stringshare containing the C type string.
 *         The caller is responsible for unreferencing the stringshare.
 *         Returns NULL if tp is NULL.
 */
EOLIAN_API Eina_Stringshare *
eolian_type_c_type_get(const Eolian_Type *tp)
{
   Eina_Stringshare *ret;
   Eina_Strbuf *buf;
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   buf = eina_strbuf_new();
   database_type_to_str(tp, buf, NULL, EOLIAN_C_TYPE_DEFAULT, EINA_FALSE);
   ret = eina_stringshare_add(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Gets the C type string for an Eolian type declaration.
 *
 * This function generates the C type representation for the given Eolian type declaration.
 * For example, for a struct `My_Struct`, it might return `"My_Struct"`.
 * For an enum `My_Enum`, it might return `"My_Enum"`.
 *
 * @param[in] tp The Eolian type declaration.
 * @return An Eina_Stringshare containing the C type string.
 *         The caller is responsible for unreferencing the stringshare.
 *         Returns NULL if tp is NULL.
 */
EOLIAN_API Eina_Stringshare *
eolian_typedecl_c_type_get(const Eolian_Typedecl *tp)
{
   Eina_Stringshare *ret;
   Eina_Strbuf *buf;
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   buf = eina_strbuf_new();
   database_typedecl_to_str(tp, buf);
   ret = eina_stringshare_add(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Gets the name of the C function used to free an instance of this type declaration.
 *
 * This is relevant for types that require custom deallocation logic.
 *
 * @param[in] tp The Eolian type declaration.
 * @return The name of the free function if specified.
 *         Returns NULL if tp is NULL or no free function is specified.
 */
EOLIAN_API const char *
eolian_typedecl_free_func_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->freefunc;
}

/**
 * @brief Gets the Eolian function definition for a function pointer type declaration.
 *
 * If the type declaration is a function pointer (callback), this returns
 * the #Eolian_Function that describes its signature (parameters and return type).
 *
 * @param[in] tp The Eolian type declaration.
 * @return A pointer to the #Eolian_Function representing the function pointer's signature.
 *         Returns NULL if tp is NULL or not a function pointer type.
 */
EOLIAN_API const Eolian_Function *
eolian_typedecl_function_pointer_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (eolian_typedecl_type_get(tp) != EOLIAN_TYPEDECL_FUNCTION_POINTER)
     return NULL;
   return tp->function_pointer;
}
