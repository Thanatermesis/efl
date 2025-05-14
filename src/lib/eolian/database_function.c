#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Frees all resources associated with an Eolian_Function.
 *
 * This function deallocates memory for the function's name, file,
 * parameters, return types, return values, and documentation.
 *
 * @param fid The Eolian_Function to delete.
 */
void
database_function_del(Eolian_Function *fid)
{
   Eolian_Function_Parameter *param;
   Eina_Stringshare *cls_name;
   if (!fid) return;

   eina_stringshare_del(fid->base.file);
   eina_stringshare_del(fid->base.name);
   EINA_LIST_FREE(fid->prop_values, param) database_parameter_del(param);
   EINA_LIST_FREE(fid->prop_values_get, param) database_parameter_del(param);
   EINA_LIST_FREE(fid->prop_values_set, param) database_parameter_del(param);
   EINA_LIST_FREE(fid->prop_keys, param) database_parameter_del(param);
   EINA_LIST_FREE(fid->prop_keys_get, param) database_parameter_del(param);
   EINA_LIST_FREE(fid->prop_keys_set, param) database_parameter_del(param);
   EINA_LIST_FREE(fid->ctor_of, cls_name) eina_stringshare_del(cls_name);
   database_type_del(fid->get_ret_type);
   database_type_del(fid->set_ret_type);
   database_expr_del(fid->get_ret_val);
   database_expr_del(fid->set_ret_val);
   database_doc_del(fid->get_return_doc);
   database_doc_del(fid->set_return_doc);
   free(fid);
}

/**
 * @brief Inserts data into a sorted Eina_List if it's not already present.
 *
 * This function searches for the correct position to insert the data to maintain
 * sorted order. If an element that compares as equal to data already exists,
 * the list is not modified.
 *
 * @param l The Eina_List to insert into.
 * @param func The comparison function to use for sorting and duplicate checking.
 * @param data The data to insert.
 * @return The (potentially modified) Eina_List.
 */
static Eina_List*
_list_sorted_insert_no_dup(Eina_List *l, Eina_Compare_Cb func, const void *data)
{
   Eina_List *lnear;
   int cmp;

   if (!l)
     return eina_list_append(NULL, data);
   else
     lnear = eina_list_search_sorted_near_list(l, func, data, &cmp);

   if (cmp < 0)
     return eina_list_append_relative_list(l, data, lnear);
   else if (cmp > 0)
     return eina_list_prepend_relative_list(l, data, lnear);
   return l;
}

/**
 * @brief Adds a class to the list of classes for which this function is a constructor.
 *
 * The class name is added to a sorted list, ensuring no duplicates.
 *
 * @param func The Eolian_Function to mark as a constructor.
 * @param cls The Eolian_Class for which this function is a constructor.
 */
void
database_function_constructor_add(Eolian_Function *func, const Eolian_Class *cls)
{
   func->ctor_of = _list_sorted_insert_no_dup
     (func->ctor_of, EINA_COMPARE_CB(strcmp),
      eina_stringshare_ref(cls->base.name));
}

/**
 * @brief Checks if an Eolian_Function matches a given Eolian_Function_Type.
 *
 * This function handles special cases for property getters and setters:
 * - If ftype is EOLIAN_PROP_GET, it returns true if fid is EOLIAN_PROP_GET or EOLIAN_PROPERTY.
 * - If ftype is EOLIAN_PROP_SET, it returns true if fid is EOLIAN_PROP_SET or EOLIAN_PROPERTY.
 * - If ftype is EOLIAN_UNRESOLVED, it always returns true.
 * Otherwise, it checks for a direct match between fid->type and ftype.
 *
 * @param fid The Eolian_Function to check.
 * @param ftype The Eolian_Function_Type to compare against.
 * @return EINA_TRUE if the function matches the type, EINA_FALSE otherwise.
 */
Eina_Bool
database_function_is_type(Eolian_Function *fid, Eolian_Function_Type ftype)
{
   if (ftype == EOLIAN_UNRESOLVED)
     return EINA_TRUE;
   else if (ftype == EOLIAN_PROP_GET)
     return (fid->type == EOLIAN_PROP_GET) || (fid->type == EOLIAN_PROPERTY);
   else if (ftype == EOLIAN_PROP_SET)
     return (fid->type == EOLIAN_PROP_SET) || (fid->type == EOLIAN_PROPERTY);
   return (fid->type == ftype);
}
