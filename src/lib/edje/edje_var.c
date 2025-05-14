#include "edje_private.h"

static Eina_Bool _edje_var_timer_cb(void *data);
static Eina_Bool _edje_var_anim_cb(void *data);

static Ecore_Animator *_edje_animator = NULL; /**< Global animator for Edje variable animations. */
static Eina_List *_edje_anim_list = NULL; /**< List of Edje objects that have active variable animations. */

/**
 * @brief Callback function for Edje variable timers.
 *
 * This function is executed when an Ecore_Timer associated with an Edje variable
 * expires. It pushes the Edje script's VM, initializes globals, pushes the
 * timer's value as a parameter, and runs the specified Embryo function.
 *
 * @param data Pointer to the Edje_Var_Timer structure.
 * @return ECORE_CALLBACK_CANCEL to automatically remove the timer after execution.
 */
static Eina_Bool
_edje_var_timer_cb(void *data)
{
   Edje_Var_Timer *et;
   Edje *ed;
   Embryo_Function fn;

   et = data;
   if (!et) return ECORE_CALLBACK_CANCEL;
   ed = et->edje;
//      _edje_embryo_script_reset(ed);
   embryo_program_vm_push(ed->collection->script);
   _edje_embryo_globals_init(ed);
   embryo_parameter_cell_push(ed->collection->script, (Embryo_Cell)et->val);
   ed->var_pool->timers = eina_inlist_remove(ed->var_pool->timers,
                                             EINA_INLIST_GET(et));
   fn = et->func;
   free(et);
   {
      void *pdata;
      int ret;

      pdata = embryo_program_data_get(ed->collection->script);
      embryo_program_data_set(ed->collection->script, ed);
      embryo_program_max_cycle_run_set(ed->collection->script, 5000000);
      ret = embryo_program_run(ed->collection->script, fn);
      if (ret == EMBRYO_PROGRAM_FAIL)
        {
           ERR("ERROR with embryo script (timer callback). "
               "OBJECT NAME: '%s', "
               "OBJECT FILE: '%s', "
               "ERROR: '%s'",
               ed->collection->part,
               ed->file->path,
               embryo_error_string_get(embryo_program_error_get(ed->collection->script)));
        }
      else if (ret == EMBRYO_PROGRAM_TOOLONG)
        {
           ERR("ERROR with embryo script (timer callback). "
               "OBJECT NAME: '%s', "
               "OBJECT FILE: '%s', "
               "ERROR: 'Script exceeded maximum allowed cycle count of %i'",
               ed->collection->part,
               ed->file->path,
               embryo_program_max_cycle_run_get(ed->collection->script));
        }
      embryo_program_data_set(ed->collection->script, pdata);
      embryo_program_vm_pop(ed->collection->script);
      _edje_recalc(ed);
   }
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Callback function for Edje variable animations.
 *
 * This function is executed by the global Ecore_Animator (_edje_animator).
 * It iterates through all Edje objects in _edje_anim_list that have active
 * variable animations. For each Edje object, it iterates through its
 * Edje_Var_Animator list, calculates the animation progress (0.0 to 1.0),
 * and calls the associated Embryo script function with the timer value and
 * progress as parameters.
 *
 * Animations that reach 1.0 progress are marked for deletion.
 * If an Edje object no longer has active animators, it's removed from
 * _edje_anim_list. If _edje_anim_list becomes empty, the global
 * _edje_animator is deleted.
 *
 * @param data Not used.
 * @return ECORE_CALLBACK_RENEW if there are still active animations,
 *         ECORE_CALLBACK_CANCEL otherwise (implicitly, as _edje_animator is deleted).
 */
static Eina_Bool
_edje_var_anim_cb(void *data EINA_UNUSED)
{
   Eina_List *l, *tl = NULL;
   double t;
   const void *tmp;

   t = ecore_loop_time_get();
   EINA_LIST_FOREACH(_edje_anim_list, l, tmp)
     tl = eina_list_append(tl, tmp);
   while (tl)
     {
        Edje *ed;
        Eina_List *tl2;
        Edje_Var_Animator *ea;

        ed = eina_list_data_get(tl);
        _edje_ref(ed);
        _edje_block(ed);
        _edje_util_freeze(ed);
        tl = eina_list_remove(tl, ed);
        if (!ed->var_pool) continue;
        tl2 = NULL;
        EINA_LIST_FOREACH(ed->var_pool->animators, l, tmp)
          tl2 = eina_list_append(tl2, tmp);
        ed->var_pool->walking_list++;
        while (tl2)
          {
             ea = eina_list_data_get(tl2);
             if ((ed->var_pool) && (!ea->delete_me))
               {
                  if ((!ed->paused) && (!ed->delete_me))
                    {
                       Embryo_Function fn;
                       float v;
                       int ret;

                       v = (t - ea->start) / ea->len;
                       if (v > 1.0) v = 1.0;
//		       _edje_embryo_script_reset(ed);
                       embryo_program_vm_push(ed->collection->script);
                       _edje_embryo_globals_init(ed);
                       embryo_parameter_cell_push(ed->collection->script, (Embryo_Cell)ea->val);
                       embryo_parameter_cell_push(ed->collection->script, EMBRYO_FLOAT_TO_CELL(v));
                       fn = ea->func;
                       {
                          void *pdata;

                          pdata = embryo_program_data_get(ed->collection->script);
                          embryo_program_data_set(ed->collection->script, ed);
                          embryo_program_max_cycle_run_set(ed->collection->script, 5000000);
                          ret = embryo_program_run(ed->collection->script, fn);
                          if (ret == EMBRYO_PROGRAM_FAIL)
                            {
                               ERR("ERROR with embryo script (anim callback). "
                                   "OBJECT NAME: '%s', "
                                   "OBJECT FILE: '%s', "
                                   "ERROR: '%s'",
                                   ed->collection->part,
                                   ed->file->path,
                                   embryo_error_string_get(embryo_program_error_get(ed->collection->script)));
                            }
                          else if (ret == EMBRYO_PROGRAM_TOOLONG)
                            {
                               ERR("ERROR with embryo script (anim callback). "
                                   "OBJECT NAME: '%s', "
                                   "OBJECT FILE: '%s', "
                                   "ERROR: 'Script exceeded maximum allowed cycle count of %i'",
                                   ed->collection->part,
                                   ed->file->path,
                                   embryo_program_max_cycle_run_get(ed->collection->script));
                            }
                          embryo_program_data_set(ed->collection->script, pdata);
                          embryo_program_vm_pop(ed->collection->script);
                          _edje_recalc(ed);
                       }
                       if (EQ(v, FROM_INT(1))) ea->delete_me = 1;
                    }
               }
             tl2 = eina_list_remove(tl2, ea);
             if (ed->block_break)
               {
                  eina_list_free(tl2);
                  break;
               }
          }
        ed->var_pool->walking_list--;
        EINA_LIST_FOREACH(ed->var_pool->animators, l, ea)
          {
             if (ea->delete_me)
               {
                  l = eina_list_next(l);
                  ed->var_pool->animators = eina_list_remove(ed->var_pool->animators, ea);
                  free(ea);
               }
             else
               l = eina_list_next(l);
          }
        if (!ed->var_pool->animators)
          _edje_anim_list = eina_list_remove(_edje_anim_list, ed);
        _edje_unblock(ed);
        _edje_util_thaw(ed);
        _edje_unref(ed);
     }
   if (!_edje_anim_list)
     {
        if (_edje_animator)
          {
             ecore_animator_del(_edje_animator);
             _edje_animator = NULL;
          }
     }
   return !!_edje_animator;
}

/**
 * @brief Allocates a new Edje_Var structure.
 *
 * The allocated structure is zero-initialized.
 *
 * @return A pointer to the newly allocated Edje_Var, or NULL on failure.
 */
Edje_Var *
_edje_var_new(void)
{
   return calloc(1, sizeof(Edje_Var));
}

/**
 * @brief Frees an Edje_Var structure.
 *
 * If the variable type is EDJE_VAR_STRING, the associated string data
 * is also freed.
 *
 * @param var Pointer to the Edje_Var to free.
 */
void
_edje_var_free(Edje_Var *var)
{
   if (var->type == EDJE_VAR_STRING)
     {
        if (var->data.s.v)
          {
             free(var->data.s.v);
          }
     }
   free(var);
}

/**
 * @brief Initializes the variable pool for an Edje object.
 *
 * If the Edje object has an Embryo script and the variable pool
 * (ed->var_pool) hasn't been initialized yet, this function allocates
 * the pool and an array for variables based on the count of variables
 * declared in the script.
 *
 * @param ed Pointer to the Edje object.
 */
void
_edje_var_init(Edje *ed)
{
   if (!ed) return;
   if (!ed->collection) return;
   if (!ed->collection->script) return;
   if (ed->var_pool) return;
   ed->var_pool = calloc(1, sizeof(Edje_Var_Pool));
   if (!ed->var_pool) return;
   embryo_program_vm_push(ed->collection->script);
   ed->var_pool->size = embryo_program_variable_count_get(ed->collection->script);
   embryo_program_vm_pop(ed->collection->script);
   if (ed->var_pool->size > 0)
     ed->var_pool->vars = calloc(1, sizeof(Edje_Var) * ed->var_pool->size);
}

/**
 * @brief Shuts down and frees the variable pool for an Edje object.
 *
 * This function frees all variables within the pool, including their
 * data (strings, lists of Edje_Var). It also cancels and frees all
 * active timers and animators associated with this Edje object's
 * variable pool.
 *
 * @param ed Pointer to the Edje object.
 */
void
_edje_var_shutdown(Edje *ed)
{
   Edje_Var_Timer *et;

   if (!ed->var_pool) return;
   if (ed->var_pool->vars)
     {
        int i;

        for (i = 0; i < ed->var_pool->size; i++)
          {
             if (ed->var_pool->vars[i].type == EDJE_VAR_STRING)
               {
                  if (ed->var_pool->vars[i].data.s.v)
                    {
                       free(ed->var_pool->vars[i].data.s.v);
                       ed->var_pool->vars[i].data.s.v = NULL;
                    }
               }
             else if (ed->var_pool->vars[i].type == EDJE_VAR_LIST)
               {
                  while (ed->var_pool->vars[i].data.l.v)
                    {
                       _edje_var_free(eina_list_data_get(ed->var_pool->vars[i].data.l.v));
                       ed->var_pool->vars[i].data.l.v = eina_list_remove_list(ed->var_pool->vars[i].data.l.v, ed->var_pool->vars[i].data.l.v);
                    }
               }
          }
        free(ed->var_pool->vars);
     }
   EINA_INLIST_FREE(ed->var_pool->timers, et)
     {
        ed->var_pool->timers = eina_inlist_remove(ed->var_pool->timers,
                                                  EINA_INLIST_GET(et));
        ecore_timer_del(et->timer);
        free(et);
     }
   if (ed->var_pool->animators)
     {
        _edje_anim_list = eina_list_remove(_edje_anim_list, ed);
        if (!_edje_anim_list)
          {
             if (_edje_animator)
               {
                  ecore_animator_del(_edje_animator);
                  _edje_animator = NULL;
               }
          }
     }
   while (ed->var_pool->animators)
     {
        Edje_Var_Animator *ea;

        ea = eina_list_data_get(ed->var_pool->animators);
        ed->var_pool->animators = eina_list_remove(ed->var_pool->animators, ea);
        free(ea);
     }
   free(ed->var_pool);
   ed->var_pool = NULL;
}

/**
 * @brief Gets the ID for a named variable from the Edje object's script.
 *
 * The ID is derived from the Embryo_Cell address of the variable.
 * This ID is typically used with EDJE_VAR_MAGIC_BASE to access
 * variables in the ed->var_pool->vars array.
 *
 * @param ed Pointer to the Edje object.
 * @param string The name of the variable in the script.
 * @return The integer ID of the variable, or 0 if not found or on error.
 */
int
_edje_var_string_id_get(Edje *ed, const char *string)
{
   Embryo_Cell cell, *cptr;

   if (!ed) return 0;
   if (!ed->collection) return 0;
   if (!ed->collection->script) return 0;
   if (!string) return 0;
   cell = embryo_program_variable_find(ed->collection->script, (char *)string);
   if (cell == EMBRYO_CELL_NONE) return 0;
   cptr = embryo_data_address_get(ed->collection->script, cell);
   if (!cptr) return 0;
   return (int)(*cptr);
}

/**
 * @brief Gets an integer value from an Edje_Var, performing auto-casting.
 *
 * - If type is EDJE_VAR_STRING, it's converted to int (string data freed).
 * - If type is EDJE_VAR_FLOAT, it's cast to int.
 * - If type is EDJE_VAR_NONE, it's set to EDJE_VAR_INT and returns 0.
 * - If type is EDJE_VAR_LIST or EDJE_VAR_HASH, returns 0.
 *
 * @param ed The Edje object (currently unused).
 * @param var Pointer to the Edje_Var.
 * @return The integer value of the variable.
 */
int
_edje_var_var_int_get(Edje *ed EINA_UNUSED, Edje_Var *var)
{
   /* auto-cast */
   if (var->type == EDJE_VAR_STRING)
     {
        if (var->data.s.v)
          {
             double f;

             f = eina_convert_strtod_c(var->data.s.v, NULL);
             free(var->data.s.v);
             var->data.s.v = NULL;
             var->data.i.v = (int)f;
          }
        var->type = EDJE_VAR_INT;
     }
   else if (var->type == EDJE_VAR_FLOAT)
     {
        int tmp = (int)(var->data.f.v);
        var->data.i.v = tmp;
        var->type = EDJE_VAR_INT;
     }
   else if (var->type == EDJE_VAR_NONE)
     {
        var->type = EDJE_VAR_INT;
     }
   else if (var->type == EDJE_VAR_LIST)
     {
        return 0;
     }
   else if (var->type == EDJE_VAR_HASH)
     {
        return 0;
     }
   return var->data.i.v;
}

/**
 * @brief Sets an integer value to an Edje_Var, performing auto-casting.
 *
 * - If type is EDJE_VAR_STRING, string data is freed and type becomes EDJE_VAR_INT.
 * - If type is EDJE_VAR_FLOAT, type becomes EDJE_VAR_INT.
 * - If type is EDJE_VAR_NONE, type becomes EDJE_VAR_INT.
 * - If type is EDJE_VAR_LIST or EDJE_VAR_HASH, the function returns without modification.
 *
 * @param ed The Edje object (currently unused).
 * @param var Pointer to the Edje_Var.
 * @param v The integer value to set.
 */
void
_edje_var_var_int_set(Edje *ed EINA_UNUSED, Edje_Var *var, int v)
{
   /* auto-cast */
   if (var->type == EDJE_VAR_STRING)
     {
        if (var->data.s.v)
          {
             free(var->data.s.v);
             var->data.s.v = NULL;
          }
        var->type = EDJE_VAR_INT;
     }
   else if (var->type == EDJE_VAR_FLOAT)
     {
        var->type = EDJE_VAR_INT;
     }
   else if (var->type == EDJE_VAR_NONE)
     {
        var->type = EDJE_VAR_INT;
     }
   else if (var->type == EDJE_VAR_LIST)
     {
        return;
     }
   else if (var->type == EDJE_VAR_HASH)
     {
        return;
     }
   var->data.i.v = v;
}

/**
 * @brief Gets a double (float) value from an Edje_Var, performing auto-casting.
 *
 * - If type is EDJE_VAR_STRING, it's converted to double (string data freed).
 * - If type is EDJE_VAR_INT, it's cast to double.
 * - If type is EDJE_VAR_NONE, it's set to EDJE_VAR_FLOAT and returns 0.0.
 * - If type is EDJE_VAR_LIST or EDJE_VAR_HASH, returns 0.0.
 *
 * @param ed The Edje object (currently unused).
 * @param var Pointer to the Edje_Var.
 * @return The double value of the variable.
 */
double
_edje_var_var_float_get(Edje *ed EINA_UNUSED, Edje_Var *var)
{
   /* auto-cast */
   if (var->type == EDJE_VAR_STRING)
     {
        if (var->data.s.v)
          {
             double f;

             f = eina_convert_strtod_c(var->data.s.v, NULL);
             free(var->data.s.v);
             var->data.s.v = NULL;
             var->data.f.v = f;
          }
        var->type = EDJE_VAR_FLOAT;
     }
   else if (var->type == EDJE_VAR_INT)
     {
        double tmp = (double)(var->data.i.v);
        var->data.f.v = tmp;
        var->type = EDJE_VAR_FLOAT;
     }
   else if (var->type == EDJE_VAR_NONE)
     {
        var->type = EDJE_VAR_FLOAT;
     }
   else if (var->type == EDJE_VAR_LIST)
     {
        return 0.0;
     }
   else if (var->type == EDJE_VAR_HASH)
     {
        return 0.0;
     }
   return var->data.f.v;
}

/**
 * @brief Sets a double (float) value to an Edje_Var, performing auto-casting.
 *
 * - If type is EDJE_VAR_STRING, string data is freed and type becomes EDJE_VAR_FLOAT.
 * - If type is EDJE_VAR_INT, type becomes EDJE_VAR_FLOAT.
 * - If type is EDJE_VAR_NONE, type becomes EDJE_VAR_FLOAT.
 * - If type is EDJE_VAR_LIST or EDJE_VAR_HASH, the function returns without modification.
 *
 * @param ed The Edje object (currently unused).
 * @param var Pointer to the Edje_Var.
 * @param v The double value to set.
 */
void
_edje_var_var_float_set(Edje *ed EINA_UNUSED, Edje_Var *var, double v)
{
   /* auto-cast */
   if (var->type == EDJE_VAR_STRING)
     {
        if (var->data.s.v)
          {
             free(var->data.s.v);
             var->data.s.v = NULL;
          }
        var->type = EDJE_VAR_FLOAT;
     }
   else if (var->type == EDJE_VAR_INT)
     {
        var->data.f.v = 0;
        var->type = EDJE_VAR_FLOAT;
     }
   else if (var->type == EDJE_VAR_NONE)
     {
        var->type = EDJE_VAR_FLOAT;
     }
   else if (var->type == EDJE_VAR_LIST)
     {
        return;
     }
   else if (var->type == EDJE_VAR_HASH)
     {
        return;
     }
   var->data.f.v = v;
}

/**
 * @brief Gets a string value from an Edje_Var, performing auto-casting.
 *
 * - If type is EDJE_VAR_INT, it's converted to string (new string allocated).
 * - If type is EDJE_VAR_FLOAT, it's converted to string (new string allocated).
 * - If type is EDJE_VAR_NONE, it's set to EDJE_VAR_STRING and returns an empty string (newly allocated).
 * - If type is EDJE_VAR_LIST or EDJE_VAR_HASH, returns NULL.
 *
 * @note The returned string is owned by the Edje_Var and should not be freed by the caller.
 *       It will be freed when the Edje_Var type changes or when _edje_var_free is called.
 *
 * @param ed The Edje object (currently unused).
 * @param var Pointer to the Edje_Var.
 * @return The string value of the variable, or NULL for list/hash types.
 */
const char *
_edje_var_var_str_get(Edje *ed EINA_UNUSED, Edje_Var *var)
{
   /* auto-cast */
   if (var->type == EDJE_VAR_INT)
     {
        char buf[64];

        snprintf(buf, sizeof(buf), "%i", var->data.i.v);
        var->data.s.v = strdup(buf);
        var->type = EDJE_VAR_STRING;
     }
   else if (var->type == EDJE_VAR_FLOAT)
     {
        char buf[64];

        snprintf(buf, sizeof(buf), "%f", var->data.f.v);
        var->data.s.v = strdup(buf);
        var->type = EDJE_VAR_STRING;
     }
   else if (var->type == EDJE_VAR_NONE)
     {
        var->data.s.v = strdup("");
        var->type = EDJE_VAR_STRING;
     }
   else if (var->type == EDJE_VAR_LIST)
     {
        return NULL;
     }
   else if (var->type == EDJE_VAR_HASH)
     {
        return NULL;
     }
   return var->data.s.v;
}

/**
 * @brief Sets a string value to an Edje_Var, performing auto-casting.
 *
 * - If type is EDJE_VAR_STRING, existing string data is freed.
 * - If type is EDJE_VAR_INT or EDJE_VAR_FLOAT, type becomes EDJE_VAR_STRING.
 * - If type is EDJE_VAR_NONE, type becomes EDJE_VAR_STRING.
 * - If type is EDJE_VAR_LIST or EDJE_VAR_HASH, the function returns without modification.
 * A copy of the input string `str` is made.
 *
 * @param ed The Edje object (currently unused).
 * @param var Pointer to the Edje_Var.
 * @param str The string value to set.
 */
void
_edje_var_var_str_set(Edje *ed EINA_UNUSED, Edje_Var *var, const char *str)
{
   /* auto-cast */
   if (var->type == EDJE_VAR_STRING)
     {
        if (var->data.s.v)
          {
             free(var->data.s.v);
             var->data.s.v = NULL;
          }
     }
   else if (var->type == EDJE_VAR_INT)
     {
        var->type = EDJE_VAR_STRING;
     }
   else if (var->type == EDJE_VAR_FLOAT)
     {
        var->type = EDJE_VAR_STRING;
     }
   else if (var->type == EDJE_VAR_NONE)
     {
        var->type = EDJE_VAR_STRING;
     }
   else if (var->type == EDJE_VAR_LIST)
     {
        return;
     }
   else if (var->type == EDJE_VAR_HASH)
     {
        return;
     }
   var->data.s.v = strdup(str);
}

/**
 * @brief Gets the integer value of a variable by its ID.
 *
 * The ID is adjusted by EDJE_VAR_MAGIC_BASE to index the vars array.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the variable (obtained from _edje_var_string_id_get).
 * @return The integer value, or 0 if ID is invalid or ed/var_pool is NULL.
 */
int
_edje_var_int_get(Edje *ed, int id)
{
   if (!ed) return 0;
   if (!ed->var_pool) return 0;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return 0;
   return _edje_var_var_int_get(ed, &(ed->var_pool->vars[id]));
}

/**
 * @brief Sets the integer value of a variable by its ID.
 *
 * The ID is adjusted by EDJE_VAR_MAGIC_BASE to index the vars array.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the variable.
 * @param v The integer value to set.
 */
void
_edje_var_int_set(Edje *ed, int id, int v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   _edje_var_var_int_set(ed, &(ed->var_pool->vars[id]), v);
}

/**
 * @brief Gets the double (float) value of a variable by its ID.
 *
 * The ID is adjusted by EDJE_VAR_MAGIC_BASE to index the vars array.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the variable.
 * @return The double value, or 0.0 if ID is invalid or ed/var_pool is NULL.
 */
double
_edje_var_float_get(Edje *ed, int id)
{
   if (!ed) return 0;
   if (!ed->var_pool) return 0;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return 0;
   return _edje_var_var_float_get(ed, &(ed->var_pool->vars[id]));
}

/**
 * @brief Sets the double (float) value of a variable by its ID.
 *
 * The ID is adjusted by EDJE_VAR_MAGIC_BASE to index the vars array.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the variable.
 * @param v The double value to set.
 */
void
_edje_var_float_set(Edje *ed, int id, double v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   _edje_var_var_float_set(ed, &(ed->var_pool->vars[id]), v);
}

/**
 * @brief Gets the string value of a variable by its ID.
 *
 * The ID is adjusted by EDJE_VAR_MAGIC_BASE to index the vars array.
 * @note The returned string is owned by the Edje_Var. See _edje_var_var_str_get().
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the variable.
 * @return The string value, or NULL if ID is invalid or ed/var_pool is NULL.
 */
const char *
_edje_var_str_get(Edje *ed, int id)
{
   if (!ed) return NULL;
   if (!ed->var_pool) return NULL;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return NULL;
   return _edje_var_var_str_get(ed, &(ed->var_pool->vars[id]));
}

/**
 * @brief Sets the string value of a variable by its ID.
 *
 * The ID is adjusted by EDJE_VAR_MAGIC_BASE to index the vars array.
 * A copy of the input string `str` is made.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the variable.
 * @param str The string value to set. Must not be NULL.
 */
void
_edje_var_str_set(Edje *ed, int id, const char *str)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   if (!str) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   _edje_var_var_str_set(ed, &(ed->var_pool->vars[id]), str);
}

/* list stuff */

/**
 * @brief Appends an Edje_Var to a list variable.
 *
 * The target variable (identified by `id`) must be of type EDJE_VAR_LIST.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param var Pointer to the Edje_Var to append. This var is now owned by the list.
 */
void
_edje_var_list_var_append(Edje *ed, int id, Edje_Var *var)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type != EDJE_VAR_LIST) return;
   ed->var_pool->vars[id].data.l.v = eina_list_append(ed->var_pool->vars[id].data.l.v, var);
}

/**
 * @brief Prepends an Edje_Var to a list variable.
 *
 * The target variable (identified by `id`) must be of type EDJE_VAR_LIST.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param var Pointer to the Edje_Var to prepend. This var is now owned by the list.
 */
void
_edje_var_list_var_prepend(Edje *ed, int id, Edje_Var *var)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type != EDJE_VAR_LIST) return;
   ed->var_pool->vars[id].data.l.v = eina_list_prepend(ed->var_pool->vars[id].data.l.v, var);
}

/**
 * @brief Appends an Edje_Var to a list variable, relative to another item.
 *
 * The target variable (identified by `id`) must be of type EDJE_VAR_LIST.
 * The `var` is inserted after `relative`.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param var Pointer to the Edje_Var to append. This var is now owned by the list.
 * @param relative Pointer to an existing Edje_Var in the list, after which `var` will be inserted.
 */
void
_edje_var_list_var_append_relative(Edje *ed, int id, Edje_Var *var, Edje_Var *relative)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type != EDJE_VAR_LIST) return;
   ed->var_pool->vars[id].data.l.v = eina_list_append_relative(ed->var_pool->vars[id].data.l.v, var, relative);
}

/**
 * @brief Prepends an Edje_Var to a list variable, relative to another item.
 *
 * The target variable (identified by `id`) must be of type EDJE_VAR_LIST.
 * The `var` is inserted before `relative`.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param var Pointer to the Edje_Var to prepend. This var is now owned by the list.
 * @param relative Pointer to an existing Edje_Var in the list, before which `var` will be inserted.
 */
void
_edje_var_list_var_prepend_relative(Edje *ed, int id, Edje_Var *var, Edje_Var *relative)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type != EDJE_VAR_LIST) return;
   ed->var_pool->vars[id].data.l.v = eina_list_prepend_relative(ed->var_pool->vars[id].data.l.v, var, relative);
}

/**
 * @brief Gets the Nth Edje_Var from a list variable.
 *
 * The target variable (identified by `id`) must be of type EDJE_VAR_LIST.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param n The zero-based index of the item to retrieve.
 * @return Pointer to the Nth Edje_Var, or NULL if not found or on error.
 *         The returned Edje_Var is still owned by the list.
 */
Edje_Var *
_edje_var_list_nth(Edje *ed, int id, int n)
{
   if (!ed) return NULL;
   if (!ed->var_pool) return NULL;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return NULL;
   if (ed->var_pool->vars[id].type != EDJE_VAR_LIST) return NULL;
   return eina_list_nth(ed->var_pool->vars[id].data.l.v, n);
}

/**
 * @brief Gets the number of items in a list variable.
 *
 * If the variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 * If it's not EDJE_VAR_LIST or EDJE_VAR_NONE, returns 0.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @return The number of items in the list, or 0 on error or if not a list.
 */
int
_edje_var_list_count_get(Edje *ed, int id)
{
   if (!ed) return 0;
   if (!ed->var_pool) return 0;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return 0;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return 0;
   return eina_list_count(ed->var_pool->vars[id].data.l.v);
}

/**
 * @brief Removes the Nth item from a list variable.
 *
 * If the variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 * The removed Edje_Var item is freed using _edje_var_free().
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param n The zero-based index of the item to remove.
 */
void
_edje_var_list_remove_nth(Edje *ed, int id, int n)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Eina_List *nth;

      nth = eina_list_nth_list(ed->var_pool->vars[id].data.l.v, n);
      if (nth)
        {
           _edje_var_free(eina_list_data_get(nth));
           ed->var_pool->vars[id].data.l.v = eina_list_remove_list(ed->var_pool->vars[id].data.l.v, nth);
        }
   }
}

/**
 * @brief Gets the integer value of the Nth item in a list variable.
 *
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 * Uses _edje_var_var_int_get() for value retrieval and type casting.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param n The zero-based index of the item.
 * @return The integer value, or 0 if not found, not a list, or on error.
 */
int
_edje_var_list_nth_int_get(Edje *ed, int id, int n)
{
   if (!ed) return 0;
   if (!ed->var_pool) return 0;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return 0;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return 0;
   {
      Edje_Var *var;

      id += EDJE_VAR_MAGIC_BASE;
      var = _edje_var_list_nth(ed, id, n);
      if (!var) return 0;
      return _edje_var_var_int_get(ed, var);
   }
}

/**
 * @brief Sets the integer value of the Nth item in a list variable.
 *
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 * Uses _edje_var_var_int_set() for value setting and type casting.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param n The zero-based index of the item.
 * @param v The integer value to set.
 */
void
_edje_var_list_nth_int_set(Edje *ed, int id, int n, int v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Edje_Var *var;

      id += EDJE_VAR_MAGIC_BASE;
      var = _edje_var_list_nth(ed, id, n);
      if (!var) return;
      _edje_var_var_int_set(ed, var, v);
   }
}

/**
 * @brief Appends an integer value to a list variable.
 *
 * A new Edje_Var of type integer is created and appended to the list.
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param v The integer value to append.
 */
void
_edje_var_list_int_append(Edje *ed, int id, int v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Edje_Var *var;

      var = _edje_var_new();
      if (!var) return;
      id += EDJE_VAR_MAGIC_BASE;
      _edje_var_var_int_set(ed, var, v);
      _edje_var_list_var_append(ed, id, var);
   }
}

/**
 * @brief Prepends an integer value to a list variable.
 *
 * A new Edje_Var of type integer is created and prepended to the list.
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param v The integer value to prepend.
 */
void
_edje_var_list_int_prepend(Edje *ed, int id, int v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Edje_Var *var;

      var = _edje_var_new();
      if (!var) return;
      id += EDJE_VAR_MAGIC_BASE;
      _edje_var_var_int_set(ed, var, v);
      _edje_var_list_var_prepend(ed, id, var);
   }
}

/**
 * @brief Inserts an integer value into a list variable at the Nth position.
 *
 * A new Edje_Var of type integer is created. If the Nth item exists, the new
 * item is inserted before it. Otherwise, it's appended to the end of the list.
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param n The zero-based index at which to insert.
 * @param v The integer value to insert.
 */
void
_edje_var_list_int_insert(Edje *ed, int id, int n, int v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Edje_Var *var, *var_rel;

      var = _edje_var_new();
      if (!var) return;
      id += EDJE_VAR_MAGIC_BASE;
      _edje_var_var_int_set(ed, var, v);
      var_rel = _edje_var_list_nth(ed, id, n);
      if (!var_rel)
        _edje_var_list_var_append(ed, id, var);
      else
        _edje_var_list_var_prepend_relative(ed, id, var, var_rel);
   }
}

/**
 * @brief Gets the float (double) value of the Nth item in a list variable.
 *
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 * Uses _edje_var_var_float_get() for value retrieval and type casting.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param n The zero-based index of the item.
 * @return The float value, or 0.0 if not found, not a list, or on error.
 */
double
_edje_var_list_nth_float_get(Edje *ed, int id, int n)
{
   if (!ed) return 0;
   if (!ed->var_pool) return 0;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return 0;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return 0;
   {
      Edje_Var *var;

      id += EDJE_VAR_MAGIC_BASE;
      var = _edje_var_list_nth(ed, id, n);
      if (!var) return 0;
      return _edje_var_var_float_get(ed, var);
   }
}

/**
 * @brief Sets the float (double) value of the Nth item in a list variable.
 *
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 * Uses _edje_var_var_float_set() for value setting and type casting.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param n The zero-based index of the item.
 * @param v The float value to set.
 */
void
_edje_var_list_nth_float_set(Edje *ed, int id, int n, double v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Edje_Var *var;

      id += EDJE_VAR_MAGIC_BASE;
      var = _edje_var_list_nth(ed, id, n);
      if (!var) return;
      _edje_var_var_float_set(ed, var, v);
   }
}

/**
 * @brief Appends a float (double) value to a list variable.
 *
 * A new Edje_Var of type float is created and appended to the list.
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param v The float value to append.
 */
void
_edje_var_list_float_append(Edje *ed, int id, double v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Edje_Var *var;

      var = _edje_var_new();
      if (!var) return;
      id += EDJE_VAR_MAGIC_BASE;
      _edje_var_var_float_set(ed, var, v);
      _edje_var_list_var_append(ed, id, var);
   }
}

/**
 * @brief Prepends a float (double) value to a list variable.
 *
 * A new Edje_Var of type float is created and prepended to the list.
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param v The float value to prepend.
 */
void
_edje_var_list_float_prepend(Edje *ed, int id, double v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Edje_Var *var;

      var = _edje_var_new();
      if (!var) return;
      id += EDJE_VAR_MAGIC_BASE;
      _edje_var_var_float_set(ed, var, v);
      _edje_var_list_var_prepend(ed, id, var);
   }
}

/**
 * @brief Inserts a float (double) value into a list variable at the Nth position.
 *
 * A new Edje_Var of type float is created. If the Nth item exists, the new
 * item is inserted before it. Otherwise, it's appended to the end of the list.
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param n The zero-based index at which to insert.
 * @param v The float value to insert.
 */
void
_edje_var_list_float_insert(Edje *ed, int id, int n, double v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Edje_Var *var, *var_rel;

      var = _edje_var_new();
      if (!var) return;
      id += EDJE_VAR_MAGIC_BASE;
      _edje_var_var_float_set(ed, var, v);
      var_rel = _edje_var_list_nth(ed, id, n);
      if (!var_rel)
        _edje_var_list_var_append(ed, id, var);
      else
        _edje_var_list_var_prepend_relative(ed, id, var, var_rel);
   }
}

/**
 * @brief Gets the string value of the Nth item in a list variable.
 *
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 * Uses _edje_var_var_str_get() for value retrieval and type casting.
 * @note The returned string is owned by the Edje_Var item in the list.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param n The zero-based index of the item.
 * @return The string value, or NULL if not found, not a list, or on error.
 */
const char *
_edje_var_list_nth_str_get(Edje *ed, int id, int n)
{
   if (!ed) return NULL;
   if (!ed->var_pool) return NULL;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return NULL;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return NULL;
   {
      Edje_Var *var;

      id += EDJE_VAR_MAGIC_BASE;
      var = _edje_var_list_nth(ed, id, n);
      if (!var) return NULL;
      return _edje_var_var_str_get(ed, var);
   }
}

/**
 * @brief Sets the string value of the Nth item in a list variable.
 *
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 * Uses _edje_var_var_str_set() for value setting and type casting. A copy of `v` is made.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param n The zero-based index of the item.
 * @param v The string value to set.
 */
void
_edje_var_list_nth_str_set(Edje *ed, int id, int n, const char *v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Edje_Var *var;

      id += EDJE_VAR_MAGIC_BASE;
      var = _edje_var_list_nth(ed, id, n);
      if (!var) return;
      _edje_var_var_str_set(ed, var, v);
   }
}

/**
 * @brief Appends a string value to a list variable.
 *
 * A new Edje_Var of type string is created (copying `v`) and appended to the list.
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param v The string value to append.
 */
void
_edje_var_list_str_append(Edje *ed, int id, const char *v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Edje_Var *var;

      var = _edje_var_new();
      if (!var) return;
      id += EDJE_VAR_MAGIC_BASE;
      _edje_var_var_str_set(ed, var, v);
      _edje_var_list_var_append(ed, id, var);
   }
}

/**
 * @brief Prepends a string value to a list variable.
 *
 * A new Edje_Var of type string is created (copying `v`) and prepended to the list.
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param v The string value to prepend.
 */
void
_edje_var_list_str_prepend(Edje *ed, int id, const char *v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Edje_Var *var;

      var = _edje_var_new();
      if (!var) return;
      id += EDJE_VAR_MAGIC_BASE;
      _edje_var_var_str_set(ed, var, v);
      _edje_var_list_var_prepend(ed, id, var);
   }
}

/**
 * @brief Inserts a string value into a list variable at the Nth position.
 *
 * A new Edje_Var of type string is created (copying `v`). If the Nth item exists,
 * the new item is inserted before it. Otherwise, it's appended to the end of the list.
 * If the list variable (identified by `id`) is EDJE_VAR_NONE, its type is changed to EDJE_VAR_LIST.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the list variable.
 * @param n The zero-based index at which to insert.
 * @param v The string value to insert.
 */
void
_edje_var_list_str_insert(Edje *ed, int id, int n, const char *v)
{
   if (!ed) return;
   if (!ed->var_pool) return;
   id -= EDJE_VAR_MAGIC_BASE;
   if ((id < 0) || (id >= ed->var_pool->size)) return;
   if (ed->var_pool->vars[id].type == EDJE_VAR_NONE)
     ed->var_pool->vars[id].type = EDJE_VAR_LIST;
   else if (ed->var_pool->vars[id].type != EDJE_VAR_LIST)
     return;
   {
      Edje_Var *var, *var_rel;

      var = _edje_var_new();
      if (!var) return;
      id += EDJE_VAR_MAGIC_BASE;
      _edje_var_var_str_set(ed, var, v);
      var_rel = _edje_var_list_nth(ed, id, n);
      if (!var_rel)
        _edje_var_list_var_append(ed, id, var);
      else
        _edje_var_list_var_prepend_relative(ed, id, var, var_rel);
   }
}

/**
 * @brief Adds a timer that calls an Embryo function after a delay.
 *
 * Creates an Ecore_Timer that, upon expiration, will call the Embryo function
 * `fname` within the Edje object's script. The integer `val` is passed as
 * a parameter to the Embryo function.
 *
 * @param ed Pointer to the Edje object.
 * @param in The delay in seconds before the timer fires.
 * @param fname The name of the Embryo function to call.
 * @param val An integer value to pass to the Embryo function.
 * @return A unique ID for the timer, or 0 on failure. This ID can be used
 *         with _edje_var_timer_del() or _edje_var_timer_reset().
 */
int
_edje_var_timer_add(Edje *ed, double in, const char *fname, int val)
{
   Edje_Var_Timer *et;
   Embryo_Function fn;

   if (!ed->var_pool) return 0;
   fn = embryo_program_function_find(ed->collection->script, (char *)fname);
   if (fn == EMBRYO_FUNCTION_NONE) return 0;
   et = calloc(1, sizeof(Edje_Var_Timer));
   if (!et) return 0;
   et->id = ++ed->var_pool->id_count;
   et->edje = ed;
   et->func = fn;
   et->val = val;
   et->timer = ecore_timer_add(in, _edje_var_timer_cb, et);
   if (!et->timer)
     {
        free(et);
        return 0;
     }
   ed->var_pool->timers = eina_inlist_prepend(ed->var_pool->timers,
                                              EINA_INLIST_GET(et));
   return et->id;
}

/**
 * @brief Finds an active Edje_Var_Timer by its ID.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the timer to find.
 * @return Pointer to the Edje_Var_Timer if found, NULL otherwise.
 */
static Edje_Var_Timer *
_edje_var_timer_find(Edje *ed, int id)
{
   Edje_Var_Timer *et;

   if (!ed->var_pool) return NULL;

   EINA_INLIST_FOREACH(ed->var_pool->timers, et)
     if (et->id == id) return et;

   return NULL;
}

/**
 * @brief Deletes an active Edje variable timer.
 *
 * Finds the timer by its ID, removes it from the Edje object's list of
 * timers, deletes the Ecore_Timer, and frees the Edje_Var_Timer structure.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the timer to delete.
 */
void
_edje_var_timer_del(Edje *ed, int id)
{
   Edje_Var_Timer *et;

   et = _edje_var_timer_find(ed, id);
   if (!et) return;

   ed->var_pool->timers = eina_inlist_remove(ed->var_pool->timers,
                                              EINA_INLIST_GET(et));
   ecore_timer_del(et->timer);
   free(et);
}

/**
 * @brief Resets an active Edje variable timer.
 *
 * Finds the timer by its ID and resets its Ecore_Timer, causing it to
 * restart its countdown from its original `in` value.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the timer to reset.
 */
void
_edje_var_timer_reset(Edje *ed, int id)
{
   Edje_Var_Timer *et;

   et = _edje_var_timer_find(ed, id);
   if (et)
     ecore_timer_reset(et->timer);
}

/**
 * @brief Adds an animation that calls an Embryo function over a duration.
 *
 * Creates an Edje_Var_Animator that will cause the Embryo function `fname`
 * to be called repeatedly by the Ecore_Animator system. The Embryo function
 * will receive two parameters: the integer `val`, and a float value
 * representing the animation progress (from 0.0 at the start to 1.0 at `len` seconds).
 *
 * If this is the first animator for this Edje object, it's added to the
 * global `_edje_anim_list`. If this is the very first animator globally,
 * `_edje_animator` (an Ecore_Animator) is created.
 *
 * @param ed Pointer to the Edje object.
 * @param len The duration of the animation in seconds. Must be > 0.0.
 * @param fname The name of the Embryo function to call.
 * @param val An integer value to pass as the first parameter to the Embryo function.
 * @return A unique ID for the animator, or 0 on failure. This ID can be used
 *         with _edje_var_anim_del().
 */
int
_edje_var_anim_add(Edje *ed, double len, const char *fname, int val)
{
   Edje_Var_Animator *ea;
   Embryo_Function fn;

   if (!ed->var_pool) return 0;
   if (len <= 0.0) return 0;
   fn = embryo_program_function_find(ed->collection->script, (char *)fname);
   if (fn == EMBRYO_FUNCTION_NONE) return 0;
   ea = calloc(1, sizeof(Edje_Var_Animator));
   if (!ea) return 0;
   ea->start = ecore_loop_time_get();
   ea->len = len;
   ea->id = ++ed->var_pool->id_count;
   ea->edje = ed;
   ea->func = fn;
   ea->val = val;
   if (!ed->var_pool->animators)
     _edje_anim_list = eina_list_append(_edje_anim_list, ed);
   ed->var_pool->animators = eina_list_prepend(ed->var_pool->animators, ea);
   if (!_edje_animator)
     _edje_animator = ecore_animator_add(_edje_var_anim_cb, NULL);
   return ea->id;
}

/**
 * @brief Finds an active Edje_Var_Animator by its ID.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the animator to find.
 * @return Pointer to the Edje_Var_Animator if found, NULL otherwise.
 */
static Edje_Var_Animator *
_edje_var_anim_find(Edje *ed, int id)
{
   Eina_List *l;
   Edje_Var_Animator *ea;

   if (!ed->var_pool) return NULL;

   EINA_LIST_FOREACH(ed->var_pool->animators, l, ea)
     if (ea->id == id) return ea;

   return NULL;
}

/**
 * @brief Deletes an active Edje variable animator.
 *
 * Finds the animator by its ID. If the animator list is currently being
 * walked (iterated over, e.g., in `_edje_var_anim_cb`), the animator is
 * marked for deletion (`delete_me = 1`) and will be cleaned up later.
 * Otherwise, it's removed from the list and freed immediately.
 *
 * If removing this animator results in the Edje object having no more
 * animators, the Edje object is removed from `_edje_anim_list`.
 * If `_edje_anim_list` becomes empty, the global `_edje_animator` is deleted.
 *
 * @param ed Pointer to the Edje object.
 * @param id The ID of the animator to delete.
 */
void
_edje_var_anim_del(Edje *ed, int id)
{
   Edje_Var_Animator *ea;

   ea = _edje_var_anim_find(ed, id);
   if (!ea) return;

   if (ed->var_pool->walking_list)
     {
        ea->delete_me = 1;
        return;
     }

   ed->var_pool->animators = eina_list_remove(ed->var_pool->animators, ea);
   free(ea);

   if (ed->var_pool->animators) return;

   _edje_anim_list = eina_list_remove(_edje_anim_list, ed);
   if (!_edje_anim_list)
     {
        if (_edje_animator)
          {
             ecore_animator_del(_edje_animator);
             _edje_animator = NULL;
          }
     }
}

