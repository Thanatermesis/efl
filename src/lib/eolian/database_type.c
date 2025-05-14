#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eo_lexer.h"

/**
 * @brief Deletes an Eolian_Type object and its associated data.
 *
 * This function recursively deletes the base type and next type if they exist.
 * It also unreferences the object and frees associated stringshares and memory.
 *
 * @param tp The Eolian_Type object to delete.
 */
void
database_type_del(Eolian_Type *tp)
{
   if (!tp || eolian_object_unref(&tp->base)) return;
   eina_stringshare_del(tp->base.file);
   eina_stringshare_del(tp->base.name);
   eina_stringshare_del(tp->base.c_name);
   database_type_del(tp->base_type);
   database_type_del(tp->next_type);
   free(tp);
}

/**
 * @brief Deletes an Eolian_Typedecl object and its associated data.
 *
 * This function unreferences the object and frees associated stringshares,
 * hash tables, lists, and memory. It also deletes the base type and
 * any associated function pointer or documentation.
 *
 * @param tp The Eolian_Typedecl object to delete.
 */
void
database_typedecl_del(Eolian_Typedecl *tp)
{
   if (!tp || eolian_object_unref(&tp->base)) return;
   eina_stringshare_del(tp->base.file);
   eina_stringshare_del(tp->base.name);
   eina_stringshare_del(tp->base.c_name);
   database_type_del(tp->base_type);
   if (tp->fields) eina_hash_free(tp->fields);
   if (tp->field_list) eina_list_free(tp->field_list);
   database_function_del(tp->function_pointer);
   eina_stringshare_del(tp->freefunc);
   database_doc_del(tp->doc);
   free(tp);
}

/**
 * @brief Adds an Eolian_Typedecl (alias) to the Eolian_Unit.
 *
 * This function registers the type declaration as an alias within the unit's
 * object database and staging area.
 *
 * @param unit The Eolian_Unit to add the type declaration to.
 * @param tp The Eolian_Typedecl (alias) to add.
 */
void
database_type_add(Eolian_Unit *unit, Eolian_Typedecl *tp)
{
   EOLIAN_OBJECT_ADD(unit, tp->base.name, tp, aliases);
   eina_hash_set(unit->state->staging.aliases_f, tp->base.file, eina_list_append
                ((Eina_List*)eina_hash_find(unit->state->staging.aliases_f, tp->base.file),
                tp));
   database_object_add(unit, &tp->base);
}

/**
 * @brief Adds an Eolian_Typedecl (struct) to the Eolian_Unit.
 *
 * This function registers the type declaration as a struct within the unit's
 * object database and staging area.
 *
 * @param unit The Eolian_Unit to add the type declaration to.
 * @param tp The Eolian_Typedecl (struct) to add.
 */
void
database_struct_add(Eolian_Unit *unit, Eolian_Typedecl *tp)
{
   EOLIAN_OBJECT_ADD(unit, tp->base.name, tp, structs);
   Eina_Hash *sh = unit->state->staging.structs_f;
   eina_hash_set(sh, tp->base.file, eina_list_append
                ((Eina_List*)eina_hash_find(sh, tp->base.file), tp));
   database_object_add(unit, &tp->base);
}

/**
 * @brief Adds an Eolian_Typedecl (enum) to the Eolian_Unit.
 *
 * This function registers the type declaration as an enum within the unit's
 * object database and staging area.
 *
 * @param unit The Eolian_Unit to add the type declaration to.
 * @param tp The Eolian_Typedecl (enum) to add.
 */
void
database_enum_add(Eolian_Unit *unit, Eolian_Typedecl *tp)
{
   EOLIAN_OBJECT_ADD(unit, tp->base.name, tp, enums);
   eina_hash_set(unit->state->staging.enums_f, tp->base.file, eina_list_append
                ((Eina_List*)eina_hash_find(unit->state->staging.enums_f, tp->base.file), tp));
   database_object_add(unit, &tp->base);
}

/**
 * @brief Checks if an Eolian_Type is "ownable" in C.
 *
 * An ownable type is typically a pointer type or a type that can be
 * treated as such (e.g., function pointers, classes). This function
 * determines if a given Eolian_Type qualifies.
 *
 * @param unit The Eolian_Unit context (can be NULL if not resolving aliases).
 * @param tp The Eolian_Type to check.
 * @param allow_void If EINA_TRUE, 'void' types are considered ownable.
 * @param otp Optional output parameter. If not NULL, it will be set to the
 *            effective type after resolving aliases (if any).
 * @return EINA_TRUE if the type is ownable, EINA_FALSE otherwise.
 */
Eina_Bool
database_type_is_ownable(const Eolian_Unit *unit, const Eolian_Type *tp, Eina_Bool allow_void, const Eolian_Type **otp)
{
   if (otp) *otp = tp;
   if (tp->is_ptr)
     return EINA_TRUE;
   if (tp->type == EOLIAN_TYPE_REGULAR)
     {
        int kw = eo_lexer_keyword_str_to_id(tp->base.name);
        const char *ct = eo_lexer_get_c_type(kw);
        if (!ct)
          {
             const Eolian_Typedecl *tpp = database_type_decl_find(unit, tp);
             if (!tpp)
               return EINA_FALSE;
             if (tpp->type == EOLIAN_TYPEDECL_FUNCTION_POINTER)
               return EINA_TRUE;
             if (tpp->type == EOLIAN_TYPEDECL_ALIAS)
               return database_type_is_ownable(unit, tpp->base_type, allow_void, otp);
             return EINA_FALSE;
          }
        return (ct[strlen(ct) - 1] == '*');
     }
   if (allow_void && (tp->type == EOLIAN_TYPE_VOID))
     return EINA_TRUE;
   return (tp->type == EOLIAN_TYPE_CLASS);
}

/**
 * @internal
 * @brief Appends a suffix to a string buffer, adding a space if needed.
 *
 * If the last character in the buffer is not a '*', a space is appended
 * before the suffix. This is typically used for C type string construction.
 *
 * @param buf The Eina_Strbuf to append to.
 * @param suffix The suffix string to append.
 */
static void
_buf_add_suffix(Eina_Strbuf *buf, const char *suffix)
{
   if (!suffix) return;
   if (eina_strbuf_string_get(buf)[eina_strbuf_length_get(buf) - 1] != '*')
     eina_strbuf_append_char(buf, ' ');
   eina_strbuf_append(buf, suffix);
}

/**
 * @brief Converts an Eolian_Type to its C string representation.
 *
 * This function generates the C type string (e.g., "const char *", "int")
 * for a given Eolian_Type and appends it to the provided string buffer.
 * It handles constness, pointers, and specific C type contexts (like return types).
 *
 * @param tp The Eolian_Type to convert.
 * @param buf The Eina_Strbuf to append the C type string to.
 * @param name Optional name to append after the type (e.g., for a variable name).
 * @param ctype The C type context (e.g., return, parameter).
 * @param by_ref EINA_TRUE if the type is passed by reference (adds an extra '*').
 */
void
database_type_to_str(const Eolian_Type *tp,
                     Eina_Strbuf *buf, const char *name,
                     Eolian_C_Type_Type ctype, Eina_Bool by_ref)
{
   if ((tp->type == EOLIAN_TYPE_REGULAR
     || tp->type == EOLIAN_TYPE_CLASS
     || tp->type == EOLIAN_TYPE_VOID)
     && tp->is_const
     && ((ctype != EOLIAN_C_TYPE_RETURN) || by_ref || database_type_is_ownable(NULL, tp, EINA_FALSE, NULL)))
     {
        eina_strbuf_append(buf, "const ");
     }
   if (tp->type == EOLIAN_TYPE_REGULAR
    || tp->type == EOLIAN_TYPE_CLASS)
     {
        int kw = eo_lexer_keyword_str_to_id(tp->base.name);
        if (kw && eo_lexer_is_type_keyword(kw))
          eina_strbuf_append(buf, eo_lexer_get_c_type(kw));
        else
          eina_strbuf_append(buf, tp->base.c_name);
     }
   else if (tp->type == EOLIAN_TYPE_ERROR)
     eina_strbuf_append(buf, "Eina_Error");
   else if (tp->type == EOLIAN_TYPE_VOID)
     eina_strbuf_append(buf, "void");
   else if (tp->type == EOLIAN_TYPE_UNDEFINED)
     eina_strbuf_append(buf, "__undefined_type");
   else
     {
        /* handles arrays and pointers as they all serialize to pointers */
        database_type_to_str(tp->base_type, buf, NULL,
                             EOLIAN_C_TYPE_DEFAULT, EINA_FALSE);
        _buf_add_suffix(buf, "*");
        if (tp->is_const && (ctype != EOLIAN_C_TYPE_RETURN))
          eina_strbuf_append(buf, " const");
     }
   if (tp->type == EOLIAN_TYPE_CLASS)
     _buf_add_suffix(buf, "*");
   if (tp->is_ptr)
     _buf_add_suffix(buf, "*");
   if (by_ref)
     _buf_add_suffix(buf, "*");
   _buf_add_suffix(buf, name);
}

/**
 * @internal
 * @brief Converts an Eolian_Typedecl (struct) to its C string representation.
 *
 * Generates the C `struct` definition string, including its fields.
 * For opaque structs, only "struct foo" is generated.
 * For regular structs, "struct foo { field1_type field1_name; ... }" is generated.
 *
 * @param tp The Eolian_Typedecl (struct) to convert.
 * @param buf The Eina_Strbuf to append the C struct string to.
 */
static void
_stype_to_str(const Eolian_Typedecl *tp, Eina_Strbuf *buf)
{
   eina_strbuf_append(buf, "struct ");
   eina_strbuf_append(buf, tp->base.c_name);
   if (tp->type == EOLIAN_TYPEDECL_STRUCT_OPAQUE)
     return;
   eina_strbuf_append(buf, " { ");
   Eina_List *l;
   Eolian_Struct_Type_Field *sf;
   EINA_LIST_FOREACH(tp->field_list, l, sf)
     {
        database_type_to_str(sf->type, buf, sf->base.name,
                             EOLIAN_C_TYPE_DEFAULT, sf->by_ref);
        eina_strbuf_append(buf, "; ");
     }
   eina_strbuf_append(buf, "}");
}

/**
 * @internal
 * @brief Converts an Eolian_Typedecl (enum) to its C string representation.
 *
 * Generates the C `enum` definition string, including its fields and their
 * optional values. Example: "enum foo { BAR, BAZ = 2, QUX }"
 *
 * @param tp The Eolian_Typedecl (enum) to convert.
 * @param buf The Eina_Strbuf to append the C enum string to.
 */
static void
_etype_to_str(const Eolian_Typedecl *tp, Eina_Strbuf *buf)
{
   eina_strbuf_append(buf, "enum ");
   eina_strbuf_append(buf, tp->base.c_name);
   eina_strbuf_append(buf, " { ");
   Eina_List *l;
   Eolian_Enum_Type_Field *ef;
   EINA_LIST_FOREACH(tp->field_list, l, ef)
     {
        eina_strbuf_append(buf, ef->base.name);
        if (ef->value)
          {
             Eolian_Value val = eolian_expression_eval(ef->value,
                                                       EOLIAN_MASK_INT);
             const char *ret;
             eina_strbuf_append(buf, " = ");
             ret = eolian_expression_value_to_literal(&val);
             eina_strbuf_append(buf, ret);
             eina_stringshare_del(ret);
          }
        if (l != eina_list_last(tp->field_list))
          eina_strbuf_append(buf, ", ");
     }
   eina_strbuf_append(buf, " }");
}

/**
 * @internal
 * @brief Converts an Eolian_Typedecl (alias/typedef) to its C string representation.
 *
 * Generates the C `typedef` string. Example: "typedef int my_int_alias;"
 *
 * @param tp The Eolian_Typedecl (alias) to convert.
 * @param buf The Eina_Strbuf to append the C typedef string to.
 */
static void
_atype_to_str(const Eolian_Typedecl *tp, Eina_Strbuf *buf)
{
   eina_strbuf_append(buf, "typedef ");
   database_type_to_str(tp->base_type, buf, tp->base.c_name,
                        EOLIAN_C_TYPE_DEFAULT, EINA_FALSE);
}

/**
 * @brief Converts an Eolian_Typedecl to its C string representation.
 *
 * This function dispatches to the appropriate internal function based on the
 * type of the Eolian_Typedecl (alias, enum, struct).
 *
 * @param tp The Eolian_Typedecl to convert.
 * @param buf The Eina_Strbuf to append the C type declaration string to.
 */
void
database_typedecl_to_str(const Eolian_Typedecl *tp, Eina_Strbuf *buf)
{
   switch (tp->type)
     {
      case EOLIAN_TYPEDECL_ALIAS:
        _atype_to_str(tp, buf);
        break;
      case EOLIAN_TYPEDECL_ENUM:
        _etype_to_str(tp, buf);
        break;
      case EOLIAN_TYPEDECL_STRUCT:
      case EOLIAN_TYPEDECL_STRUCT_OPAQUE:
        _stype_to_str(tp, buf);
        break;
      default:
        break;
     }
}

/**
 * @brief Finds the Eolian_Typedecl corresponding to an Eolian_Type.
 *
 * This function attempts to resolve an Eolian_Type (which might be a simple
 * type name) to its full Eolian_Typedecl definition within the given unit.
 * It checks if the type already has a cached declaration, then looks up
 * in the unit's objects. It handles built-in types by returning NULL.
 *
 * @param unit The Eolian_Unit to search within.
 * @param tp The Eolian_Type to find the declaration for.
 * @return The Eolian_Typedecl if found, otherwise NULL.
 *         Returns NULL for built-in types or if the type is not EOLIAN_TYPE_REGULAR.
 */
Eolian_Typedecl *database_type_decl_find(const Eolian_Unit *unit, const Eolian_Type *tp)
{
   if (tp->type != EOLIAN_TYPE_REGULAR)
     return NULL;
   if (tp->tdecl)
     return tp->tdecl;
   /* try looking up if it belongs to a struct, enum or an alias... otherwise
    * return NULL, but first check for builtins
    */
   int  kw = eo_lexer_keyword_str_to_id(tp->base.name);
   if (!kw || kw < KW_byte || kw >= KW_true)
     {
        Eolian_Object *decl = eina_hash_find(unit->objects, tp->base.name);
        if (decl && decl->type == EOLIAN_OBJECT_TYPEDECL)
          return (Eolian_Typedecl *)decl;
     }
   return NULL;
}
