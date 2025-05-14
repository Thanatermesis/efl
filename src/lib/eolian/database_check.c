#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "eo_lexer.h"
#include "eolian_priv.h"

/**
 * @brief Checks for cyclic dependencies among Eolian objects.
 *
 * This function is used to detect cycles in the dependency graph of Eolian
 * objects, such as classes, typedecls, and variables. It adds the object
 * to the hash table if not already present, marking it as visited.
 *
 * @param chash A hash table storing visited objects to detect cycles.
 *              The key is a pointer to the Eolian_Object, and the value is the object itself.
 * @param obj The Eolian_Object to check.
 * @return @c EINA_TRUE if a cycle is detected (object already in chash),
 *         @c EINA_FALSE otherwise.
 */
static Eina_Bool
_check_cycle(Eina_Hash *chash, const Eolian_Object *obj)
{
   /* need to check this for classes, typedecls, vars (toplevel objects) */
   if (eina_hash_find(chash, &obj))
     return EINA_TRUE;
   eina_hash_add(chash, &obj, obj);
   return EINA_FALSE;
}

/**
 * @brief Adds a dependency unit to a set of dependencies.
 *
 * This function adds the given Eolian_Unit to the depset hash table if it's
 * not already present. This is used to track the direct dependencies of an
 * Eolian unit or object.
 *
 * @param depset A hash table representing the set of Eolian_Unit dependencies.
 *               The key is a pointer to the Eolian_Unit, and the value is the unit itself.
 * @param dep The Eolian_Unit to add as a dependency.
 */
static void
_add_dep(Eina_Hash *depset, const Eolian_Unit *dep)
{
   if (!eina_hash_find(depset, &dep))
     eina_hash_add(depset, &dep, dep);
}

/**
 * @brief Recursively checks an Eolian_Type and its components for dependencies.
 *
 * This function traverses an Eolian_Type, including its base type and any
 * next types (e.g., in function pointers or complex types), and adds the
 * units of any referenced type declarations (classes, structs, enums) to
 * the depset. It uses chash to detect cycles during type checking, although
 * the primary cycle detection is handled by _check_cycle for top-level objects.
 *
 * @param tp The Eolian_Type to check.
 * @param depset A hash table to collect Eolian_Unit dependencies.
 * @param chash A hash table used for cycle detection (primarily by callers).
 */
static void
_check_type(const Eolian_Type *tp, Eina_Hash *depset, Eina_Hash *chash)
{
   if (tp->base_type)
      _check_type(tp->base_type, depset, chash);

   if (tp->next_type)
     {
        const Eolian_Type *ntp = tp->next_type;
        for (; ntp; ntp = ntp->next_type)
          _check_type(ntp, depset, chash);
     }

   /* also covers EOLIAN_TYPE_CLASS */
   if (tp->tdecl)
     _add_dep(depset, ((const Eolian_Object *)tp->tdecl)->unit);
}

/**
 * @brief Callback function used by database_expr_eval to collect dependencies from expressions.
 *
 * This function is called for each Eolian_Object encountered during the
 * evaluation of an expression. If the object is a typedecl or a constant,
 * its defining unit is added to the depset.
 *
 * @param obj The Eolian_Object found within the expression.
 * @param data User data, expected to be an Eina_Hash (depset) to store dependencies.
 */
static void
_check_expr_cb(const Eolian_Object *obj, void *data)
{
   Eina_Hash *depset = data;
   switch (obj->type)
     {
      case EOLIAN_OBJECT_TYPEDECL:
      case EOLIAN_OBJECT_CONSTANT:
        _add_dep(depset, obj->unit);
      default:
        break;
     }
}

/**
 * @brief Checks an Eolian_Expression for dependencies.
 *
 * This function evaluates the given expression and uses _check_expr_cb
 * to collect all Eolian_Unit dependencies referenced within the expression.
 *
 * @param expr The Eolian_Expression to check.
 * @param depset A hash table to collect Eolian_Unit dependencies.
 */
static void
_check_expr(const Eolian_Expression *expr, Eina_Hash *depset)
{
   database_expr_eval(expr->base.unit, (Eolian_Expression *)expr,
                      EOLIAN_MASK_ALL, _check_expr_cb, depset);
}

/**
 * @brief Checks an Eolian_Function_Parameter for dependencies.
 *
 * This function examines the type and default value (if any) of a function
 * parameter and adds their respective dependencies to depset.
 *
 * @param arg The Eolian_Function_Parameter to check.
 * @param depset A hash table to collect Eolian_Unit dependencies.
 * @param chash A hash table used for cycle detection by _check_type.
 */
static void
_check_param(const Eolian_Function_Parameter *arg, Eina_Hash *depset,
             Eina_Hash *chash)
{
   if (arg->type)
     _check_type(arg->type, depset, chash);
   if (arg->value)
     _check_expr(arg->value, depset);
}

/**
 * @brief Checks an Eolian_Function (method, property, or function pointer) for dependencies.
 *
 * This function inspects the return types, return values (for properties),
 * and parameters of a function to find all Eolian_Unit dependencies.
 *
 * @param f The Eolian_Function to check.
 * @param depset A hash table to collect Eolian_Unit dependencies.
 * @param chash A hash table used for cycle detection by _check_type.
 */
static void
_check_function(const Eolian_Function *f, Eina_Hash *depset, Eina_Hash *chash)
{
   if (f->get_ret_type)
     _check_type(f->get_ret_type, depset, chash);
   if (f->set_ret_type)
     _check_type(f->set_ret_type, depset, chash);

   if (f->get_ret_val)
     _check_expr(f->get_ret_val, depset);
   if (f->set_ret_val)
     _check_expr(f->set_ret_val, depset);

   Eina_List *l;
   const Eolian_Function_Parameter *arg;
   if ((f->type == EOLIAN_METHOD) || (f->type == EOLIAN_FUNCTION_POINTER))
     {
        EINA_LIST_FOREACH(f->params, l, arg)
          _check_param(arg, depset, chash);
     }
   else
     {
        EINA_LIST_FOREACH(f->prop_values, l, arg)
          _check_param(arg, depset, chash);
        EINA_LIST_FOREACH(f->prop_values_get, l, arg)
          _check_param(arg, depset, chash);
        EINA_LIST_FOREACH(f->prop_values_set, l, arg)
          _check_param(arg, depset, chash);
        EINA_LIST_FOREACH(f->prop_keys, l, arg)
          _check_param(arg, depset, chash);
        EINA_LIST_FOREACH(f->prop_keys_get, l, arg)
          _check_param(arg, depset, chash);
        EINA_LIST_FOREACH(f->prop_keys_set, l, arg)
          _check_param(arg, depset, chash);
     }
}

/**
 * @brief Checks an Eolian_Class for dependencies and cycles.
 *
 * This function performs a thorough check of a class, including its parent,
 * extended classes, required classes, composite classes, properties, methods,
 * events, and parts. It collects all Eolian_Unit dependencies and uses
 * _check_cycle to detect inheritance or composition cycles.
 *
 * @param cl The Eolian_Class to check.
 * @param depset A hash table to collect Eolian_Unit dependencies.
 * @param chash A hash table used for cycle detection among top-level objects.
 */
static void
_check_class(const Eolian_Class *cl, Eina_Hash *depset, Eina_Hash *chash)
{
   if (_check_cycle(chash, &cl->base))
     return;

   _add_dep(depset, cl->base.unit);

   const Eolian_Class *icl = cl->parent;
   if (icl)
     _add_dep(depset, icl->base.unit);

   Eina_Iterator *itr = eina_list_iterator_new(cl->extends);
   EINA_ITERATOR_FOREACH(itr, icl)
     _add_dep(depset, icl->base.unit);
   eina_iterator_free(itr);

   itr = eina_list_iterator_new(cl->requires);
   EINA_ITERATOR_FOREACH(itr, icl)
     _add_dep(depset, icl->base.unit);
   eina_iterator_free(itr);

   itr = eina_list_iterator_new(cl->composite);
   EINA_ITERATOR_FOREACH(itr, icl)
     _add_dep(depset, icl->base.unit);
   eina_iterator_free(itr);

   const Eolian_Function *fid;
   itr = eina_list_iterator_new(cl->properties);
   EINA_ITERATOR_FOREACH(itr, fid)
     _check_function(fid, depset, chash);
   eina_iterator_free(itr);

   itr = eina_list_iterator_new(cl->methods);
   EINA_ITERATOR_FOREACH(itr, fid)
     _check_function(fid, depset, chash);
   eina_iterator_free(itr);

   const Eolian_Event *ev;
   itr = eina_list_iterator_new(cl->events);
   EINA_ITERATOR_FOREACH(itr, ev)
     {
        if (ev->type)
          _check_type(ev->type, depset, chash);
     }
   eina_iterator_free(itr);

   const Eolian_Part *part;
   itr = eina_list_iterator_new(cl->parts);
   EINA_ITERATOR_FOREACH(itr, part)
     _add_dep(depset, part->klass->base.unit);
   eina_iterator_free(itr);
}

/**
 * @brief Checks an Eolian_Typedecl (struct, enum, alias) for dependencies and cycles.
 *
 * This function examines a type declaration, including its base type (for aliases),
 * fields (for structs and enums), and function pointer signature (if applicable).
 * It collects all Eolian_Unit dependencies and uses _check_cycle to detect
 * cycles in type definitions.
 *
 * @param tp The Eolian_Typedecl to check.
 * @param depset A hash table to collect Eolian_Unit dependencies.
 * @param chash A hash table used for cycle detection among top-level objects.
 */
static void
_check_typedecl(const Eolian_Typedecl *tp, Eina_Hash *depset, Eina_Hash *chash)
{
   if (_check_cycle(chash, &tp->base))
     return;

   _add_dep(depset, tp->base.unit);

   if (tp->base_type)
     _check_type(tp->base_type, depset, chash);

   if (tp->field_list)
     {
        Eina_List *l;
        void *data;
        EINA_LIST_FOREACH(tp->field_list, l, data)
          {
             switch (tp->type)
               {
                case EOLIAN_TYPEDECL_STRUCT:
                  _check_type(((const Eolian_Struct_Type_Field *)data)->type,
                              depset, chash);
                  break;
                case EOLIAN_TYPEDECL_ENUM:
                  _check_expr(((const Eolian_Enum_Type_Field *)data)->value,
                              depset);
                  break;
                default:
                  break;
               }
          }
     }

   if (tp->function_pointer)
     _check_function(tp->function_pointer, depset, chash);
}

/**
 * @brief Checks an Eolian_Constant for dependencies and cycles.
 *
 * This function inspects a constant, including its base type and value expression.
 * It collects all Eolian_Unit dependencies and uses _check_cycle to detect
 * cycles (though less common for constants, it maintains consistency).
 *
 * @param v The Eolian_Constant to check.
 * @param depset A hash table to collect Eolian_Unit dependencies.
 * @param chash A hash table used for cycle detection among top-level objects.
 */
static void
_check_constant(const Eolian_Constant *v, Eina_Hash *depset, Eina_Hash *chash)
{
   if (_check_cycle(chash, &v->base))
     return;

   _add_dep(depset, v->base.unit);

   _check_type(v->base_type, depset, chash);
   if (v->value)
     _check_expr(v->value, depset);
}

/**
 * @brief Checks a single Eolian_Unit for unused dependencies.
 *
 * This function iterates over all objects defined directly within the given unit
 * (classes, typedecls, constants) and collects their actual dependencies
 * by calling the respective _check_* functions. It then compares these
 * collected dependencies against the unit's declared children (imports/includes).
 * If a child unit is declared but not found in the collected dependencies,
 * a warning about an unused dependency is logged.
 *
 * @param unit The Eolian_Unit to check.
 */
static void
_check_unit(const Eolian_Unit *unit)
{
   Eina_Hash *depset = eina_hash_pointer_new(NULL);

   /* collect all real dependencies of the unit */
   Eina_Hash *chash = eina_hash_pointer_new(NULL);
   Eina_Iterator *itr = eolian_unit_objects_get(unit);
   const Eolian_Object *obj;
   EINA_ITERATOR_FOREACH(itr, obj)
     {
        /* skip stuff merged in from children */
        if (obj->unit != unit)
          continue;
        switch (obj->type)
          {
           case EOLIAN_OBJECT_CLASS:
             _check_class((const Eolian_Class *)obj, depset, chash);
             break;
           case EOLIAN_OBJECT_TYPEDECL:
             _check_typedecl((const Eolian_Typedecl *)obj, depset, chash);
             break;
           case EOLIAN_OBJECT_CONSTANT:
             _check_constant((const Eolian_Constant *)obj, depset, chash);
             break;
           default:
             continue;
          }
     }
   eina_hash_free(chash);

   /* check collected deps against children units */
   Eina_Iterator *citr = eolian_unit_children_get(unit);
   const Eolian_Unit *cunit;
   EINA_ITERATOR_FOREACH(citr, cunit)
     {
        if (!eina_hash_find(depset, &cunit))
          {
             eolian_state_log(unit->state, "%s: unused dependency %s",
                              unit->file, cunit->file);
          }
     }
   eina_iterator_free(citr);

   eina_hash_free(depset);
}

/**
 * @brief Checks for namespace conflicts within an Eolian_Unit.
 *
 * This function iterates through all objects in the unit. For each object
 * with a qualified name (e.g., "Namespace.Object"), it checks if an object
 * with the name "Namespace" also exists in the same unit. If such a conflict
 * is found (and the conflicting namespace object is not beta, unless
 * EOLIAN_CHECK_NAMESPACES_BETA is set), an error is logged.
 *
 * Example of a conflict:
 *   - Object A: `MyLib.Core.Helper`
 *   - Object B: `MyLib.Core` (if `MyLib.Core` is a class, struct, etc.)
 * This would be a conflict because `MyLib.Core` is used both as a namespace
 * prefix and as an object name.
 *
 * @param src The Eolian_Unit to check for namespace conflicts.
 * @return @c EINA_TRUE if no namespace conflicts are found, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_check_namespaces(const Eolian_Unit *src)
{
   Eina_Bool ret = EINA_TRUE;
   Eina_Iterator *itr = eina_hash_iterator_data_new(src->objects);
   const Eolian_Object *obj;

   Eina_Bool check_beta = !!getenv("EOLIAN_CHECK_NAMESPACES_BETA");

   EINA_ITERATOR_FOREACH(itr, obj)
     {
        char const *dot = strrchr(obj->name, '.');
        if (!dot)
          continue;
        Eina_Stringshare *ssr = eina_stringshare_add_length(obj->name,
          dot - obj->name);
        const Eolian_Object *cobj = eina_hash_find(src->objects, ssr);
        eina_stringshare_del(ssr);
        if (cobj && (check_beta || !eolian_object_is_beta(cobj)))
          {
             eolian_state_log_obj(src->state, obj,
               "the namespace of object '%s' conflicts with %s:%d:%d",
               obj->name, cobj->file, cobj->line, cobj->column);
             ret = EINA_FALSE;
          }
     }
   eina_iterator_free(itr);
   return ret;
}

/**
 * @brief Performs various checks on the Eolian database associated with a state.
 *
 * This function serves as the main entry point for database validation.
 * It currently performs two main types of checks:
 * 1. Unused dependency check: Iterates through all units in the state and
 *    calls _check_unit for each to find and report unused dependencies.
 * 2. Namespace conflict check: Calls _check_namespaces on the main unit
 *    of the state to detect naming conflicts between objects and their namespaces.
 *
 * @param state The Eolian_State whose database is to be checked.
 * @return @c EINA_TRUE if all checks pass, @c EINA_FALSE if any check fails.
 */
Eina_Bool
database_check(const Eolian_State *state)
{
   Eina_Bool ret = EINA_TRUE;

   /* check for extra dependencies */
   Eina_Iterator *itr = eolian_state_units_get(state);
   const Eolian_Unit *unit;
   EINA_ITERATOR_FOREACH(itr, unit)
     _check_unit(unit);
   eina_iterator_free(itr);

   /* namespace checks */
   if (!_check_namespaces(&state->main.unit))
     ret = EINA_FALSE;

   return ret;
}
