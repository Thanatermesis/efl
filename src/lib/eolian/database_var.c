#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eo_lexer.h"

/**
 * @brief Deletes an Eolian_Constant object and frees its resources.
 *
 * This function decrements the reference count of the constant. If the
 * reference count reaches zero, it frees all associated data, including
 * its name, C name, file path, base type, value expression, and documentation.
 *
 * @param var The Eolian_Constant object to delete.
 */
void
database_constant_del(Eolian_Constant *var)
{
   if (!var || eolian_object_unref(&var->base)) return;
   eina_stringshare_del(var->base.file);
   eina_stringshare_del(var->base.name);
   eina_stringshare_del(var->base.c_name);
   if (var->base_type)
     database_type_del(var->base_type);
   if (var->value) database_expr_del(var->value);
   database_doc_del(var->doc);
   free(var);
}

/**
 * @brief Adds an Eolian_Constant object to an Eolian_Unit.
 *
 * This function registers the constant within the given unit. It adds the
 * constant to the unit's list of constants and also maps it by its file
 * of origin in the unit's staging area. Finally, it registers the constant
 * as a database object within the unit.
 *
 * @param unit The Eolian_Unit to which the constant will be added.
 * @param var The Eolian_Constant object to add.
 */
void
database_constant_add(Eolian_Unit *unit, Eolian_Constant *var)
{
   EOLIAN_OBJECT_ADD(unit, var->base.name, var, constants);
   eina_hash_set(unit->state->staging.constants_f, var->base.file, eina_list_append
                 ((Eina_List*)eina_hash_find(unit->state->staging.constants_f, var->base.file), var));
   database_object_add(unit, &var->base);
}
