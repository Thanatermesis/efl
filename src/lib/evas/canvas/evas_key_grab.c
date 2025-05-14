#include "evas_common_private.h"
#include "evas_private.h"

/* private calls */

/* FIXME: this is not optimal, but works. i should have a hash of keys per */
/* Evas and then a linked lists of grabs for that key and what */
/* modifiers/not_modifers they use */

static Evas_Key_Grab *evas_key_grab_new  (Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, const char *keyname, Evas_Modifier_Mask modifiers, Evas_Modifier_Mask not_modifiers, Eina_Bool exclusive);
static Evas_Key_Grab *evas_key_grab_find (Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, const char *keyname, Evas_Modifier_Mask modifiers, Evas_Modifier_Mask not_modifiers);

/**
 * @internal
 * @brief Creates a new key grab for an object.
 *
 * This function attempts to create a new key grab. It checks for existing
 * exclusive grabs that might conflict. If an exclusive grab is requested and
 * a non-exclusive one exists, the existing one is deactivated.
 *
 * @param eo_obj The Evas object to associate the grab with.
 * @param obj The protected data of the Evas object.
 * @param keyname The name of the key to grab (e.g., "Control_L", "a", "F1").
 * @param modifiers A bitmask of required modifiers (e.g., EVAS_MODIFIER_CTRL).
 * @param not_modifiers A bitmask of modifiers that must NOT be active.
 * @param exclusive If EINA_TRUE, this grab is exclusive, meaning no other
 *                  grab for the same key and modifiers can be active.
 * @return A pointer to the newly created Evas_Key_Grab, or NULL on failure
 *         (e.g., memory allocation error, conflict with an existing exclusive grab).
 */
static Evas_Key_Grab *
evas_key_grab_new(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, const char *keyname, Evas_Modifier_Mask modifiers, Evas_Modifier_Mask not_modifiers, Eina_Bool exclusive)
{
   /* MEM OK */
   Eina_List *l;
   Evas_Key_Grab *g;
   Eina_Bool have_exclusion = EINA_FALSE;

   EINA_LIST_FOREACH(obj->layer->evas->grabs, l, g)
     {
        if ((g->modifiers == modifiers) &&
            (g->not_modifiers == not_modifiers) &&
            (!strcmp(g->keyname, keyname)) &&
            (g->exclusive))
          {
             have_exclusion = EINA_TRUE;
             break;
          }
     }

   if (have_exclusion && exclusive) return NULL;

   g = calloc(1, sizeof(Evas_Key_Grab));
   if (!g) return NULL;
   g->object = eo_obj;
   g->modifiers = modifiers;
   g->not_modifiers = not_modifiers;
   g->exclusive = exclusive;
   g->keyname = strdup(keyname);
   if (obj->layer->evas->walking_grabs)
     g->just_added = EINA_TRUE;
   g->is_active = EINA_TRUE;
   if (!g->keyname)
     {
        free(g);
        return NULL;
     }

   if (exclusive)
     {
        Evas_Key_Grab *ge;
        EINA_LIST_FOREACH(obj->layer->evas->grabs, l, ge)
          {
             if ((ge->modifiers == modifiers) &&
                 (ge->not_modifiers == not_modifiers) &&
                 (!strcmp(ge->keyname, keyname)))
               {
                  ge->is_active = EINA_FALSE;
               }
          }
     }
   if (have_exclusion) g->is_active = EINA_FALSE;

   obj->grabs = eina_list_append(obj->grabs, g);
   obj->layer->evas->grabs = eina_list_append(obj->layer->evas->grabs, g);
   return g;
}

/**
 * @internal
 * @brief Finds an existing key grab for an object.
 *
 * This function searches the list of grabs associated with the canvas
 * (not just the object) for a specific key grab matching the given parameters
 * and associated with the specified object.
 *
 * @param eo_obj The Evas object the grab is associated with.
 * @param obj The protected data of the Evas object (used to access canvas grabs).
 * @param keyname The name of the key.
 * @param modifiers The bitmask of required modifiers.
 * @param not_modifiers The bitmask of modifiers that must NOT be active.
 * @return A pointer to the Evas_Key_Grab if found, otherwise NULL.
 */
static Evas_Key_Grab *
evas_key_grab_find(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, const char *keyname, Evas_Modifier_Mask modifiers, Evas_Modifier_Mask not_modifiers)
{
   /* MEM OK */
   Eina_List *l;
   Evas_Key_Grab *g;

   EINA_LIST_FOREACH(obj->layer->evas->grabs, l, g)
     {
        if ((g->modifiers == modifiers) &&
            (g->not_modifiers == not_modifiers) &&
            (!strcmp(g->keyname, keyname)))
          {
             if (eo_obj == g->object) return g;
          }
     }
   return NULL;
}

/* local calls */

/**
 * @internal
 * @brief Cleans up all key grabs associated with an Evas object.
 *
 * This function is called when an object is being deleted or its layer
 * is changing. It removes all key grabs associated with the object.
 * If the canvas is currently iterating through its grabs (`walking_grabs` is true),
 * grabs are marked for deletion (`delete_me = EINA_TRUE`) to be cleaned up later.
 * Otherwise, they are freed immediately.
 *
 * @param eo_obj The Evas object whose grabs are to be cleaned up (unused).
 * @param obj The protected data of the Evas object.
 */
void
evas_object_grabs_cleanup(Evas_Object *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj)
{
   if ((!obj->layer) || (!obj->layer->evas)) return;
   if (obj->layer->evas->walking_grabs)
     {
        Eina_List *l;
        Evas_Key_Grab *g;

        EINA_LIST_FOREACH(obj->grabs, l, g)
          g->delete_me = EINA_TRUE;
     }
   else
     {
        while (obj->grabs)
          {
             Evas_Key_Grab *g = obj->grabs->data;
             obj->layer->evas->grabs =
               eina_list_remove(obj->layer->evas->grabs, g);
             obj->grabs = eina_list_remove(obj->grabs, g);
             if (g->keyname) free(g->keyname);
             free(g);
          }
     }
}

/**
 * @internal
 * @brief Frees a specific key grab.
 *
 * This function finds a specific key grab associated with an object and
 * removes it from both the object's list of grabs and the canvas's global
 * list of grabs. It then frees the memory allocated for the grab.
 * This function does not handle the `walking_grabs` scenario; it assumes
 * immediate removal is safe.
 *
 * @param eo_obj The Evas object the grab is associated with.
 * @param obj The protected data of the Evas object.
 * @param keyname The name of the key for the grab to be freed.
 * @param modifiers The bitmask of required modifiers for the grab.
 * @param not_modifiers The bitmask of modifiers that must NOT be active for the grab.
 */
void
evas_key_grab_free(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, const char *keyname, Evas_Modifier_Mask modifiers, Evas_Modifier_Mask not_modifiers)
{
   /* MEM OK */
   Evas_Key_Grab *g;

   g = evas_key_grab_find(eo_obj, obj, keyname, modifiers, not_modifiers);
   if (!g) return;
   Evas_Object_Protected_Data *g_object = efl_data_scope_get(g->object, EFL_CANVAS_OBJECT_CLASS);
   g_object->grabs = eina_list_remove(g_object->grabs, g);
   obj->layer->evas->grabs = eina_list_remove(obj->layer->evas->grabs, g);
   if (g->keyname) free(g->keyname);
   free(g);
}

// Legacy implementation. TODO: remove use of Evas_Modifier_Mask

/**
 * @internal
 * @brief Legacy internal function to grab a key for an object.
 *
 * This function is a wrapper around evas_key_grab_new, providing the
 * core logic for the legacy evas_object_key_grab API.
 * It validates that modifiers and not_modifiers are not identical (unless both are 0)
 * and that a keyname is provided.
 *
 * @param eo_obj The Evas object to associate the grab with.
 * @param obj The protected data of the Evas object.
 * @param keyname The name of the key to grab.
 * @param modifiers A bitmask of required legacy Evas_Modifier_Mask.
 * @param not_modifiers A bitmask of legacy Evas_Modifier_Mask that must NOT be active.
 * @param exclusive If EINA_TRUE, this grab is exclusive.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_object_key_grab(Eo *eo_obj, Evas_Object_Protected_Data *obj, const char *keyname,
                 Evas_Modifier_Mask modifiers, Evas_Modifier_Mask not_modifiers,
                 Eina_Bool exclusive)
{
   /* MEM OK */
   Evas_Key_Grab *g;

   if (((modifiers == not_modifiers) && (modifiers != 0)) || (!keyname)) return EINA_FALSE;
   g = evas_key_grab_new(eo_obj, obj, keyname, modifiers, not_modifiers, exclusive);
   return ((!g) ? EINA_FALSE : EINA_TRUE);
}

/**
 * @internal
 * @brief Legacy internal function to ungrab a key for an object.
 *
 * This function is the core logic for the legacy evas_object_key_ungrab API.
 * It finds the specified key grab. If the canvas is `walking_grabs`, the grab
 * is marked for deletion. Otherwise, if the grab was exclusive, it reactivates
 * any other grabs for the same key combination that might have been deactivated.
 * Finally, it calls evas_key_grab_free to remove and free the grab.
 *
 * @param eo_obj The Evas object from which to ungrab the key.
 * @param obj The protected data of the Evas object.
 * @param keyname The name of the key to ungrab.
 * @param modifiers The bitmask of legacy Evas_Modifier_Mask used when grabbing.
 * @param not_modifiers The bitmask of legacy Evas_Modifier_Mask used when grabbing.
 */
static void
_object_key_ungrab(Eo *eo_obj, Evas_Object_Protected_Data *obj, const char *keyname,
                   Evas_Modifier_Mask modifiers, Evas_Modifier_Mask not_modifiers)
{
   /* MEM OK */
   Evas_Key_Grab *g;
   Eina_List *l;

   if (!keyname) return;
   g = evas_key_grab_find(eo_obj, obj, keyname, modifiers, not_modifiers);
   if (!g) return;
   Evas_Object_Protected_Data *g_object = efl_data_scope_get(g->object, EFL_CANVAS_OBJECT_CLASS);
   if (g_object->layer->evas->walking_grabs)
     {
        if (!g->delete_me)
          {
             g_object->layer->evas->delete_grabs++;
             g->delete_me = EINA_TRUE;
          }
     }
   else
     {
        if (g->exclusive)
          {
             Evas_Key_Grab *ge;
             EINA_LIST_FOREACH(obj->layer->evas->grabs, l, ge)
               {
                  if ((ge->modifiers == modifiers) &&
                     (ge->not_modifiers == not_modifiers) &&
                     (!strcmp(ge->keyname, keyname)))
                    {
                       if (!ge->is_active) ge->is_active = EINA_TRUE;
                    }
               }
          }

        evas_key_grab_free(g->object, g_object, keyname, modifiers, not_modifiers);
     }
}

/**
 * @internal
 * @brief Converts Efl_Input_Modifier flags to legacy Evas_Modifier_Mask.
 *
 * This utility function maps the newer Efl_Input_Modifier bitmask (used by EO APIs)
 * to the older Evas_Modifier_Mask bitmask (used by legacy APIs and internal grab logic).
 * It iterates through known Efl_Input_Modifier flags, converts them to their
 * string representations (e.g., "Control"), and then gets the corresponding
 * Evas_Modifier_Mask bit from the Evas canvas.
 *
 * @param e Pointer to the Evas_Public_Data for the canvas.
 * @param in The Efl_Input_Modifier bitmask to convert.
 * @return The corresponding Evas_Modifier_Mask bitmask.
 */
static inline Evas_Modifier_Mask
_efl_input_modifier_to_evas_modifier_mask(Evas_Public_Data *e, Efl_Input_Modifier in)
{
   Evas_Modifier_Mask out = 0;
   size_t i;

   static const Efl_Input_Modifier mods[] = {
      EFL_INPUT_MODIFIER_ALT,
      EFL_INPUT_MODIFIER_CONTROL,
      EFL_INPUT_MODIFIER_SHIFT,
      EFL_INPUT_MODIFIER_META,
      EFL_INPUT_MODIFIER_ALTGR,
      EFL_INPUT_MODIFIER_HYPER,
      EFL_INPUT_MODIFIER_SUPER
   };

   for (i = 0; i < EINA_C_ARRAY_LENGTH(mods); i++)
     if (in & mods[i])
       {
          out |= evas_key_modifier_mask_get
                (e->evas, _efl_input_modifier_to_string(mods[i]));
       }

   return out;
}

// EO API

/**
 * @internal
 * @brief Efl_Canvas_Object API implementation for grabbing a key.
 * @see efl_canvas_object_key_grab
 *
 * This function implements the Eolian interface for key grabbing.
 * It converts the Efl_Input_Modifier parameters to the internal Evas_Modifier_Mask
 * format and then calls the internal _object_key_grab function.
 *
 * @param eo_obj The Evas object (Eo pointer).
 * @param obj The protected data of the Evas object.
 * @param keyname The name of the key to grab.
 * @param mod An Efl_Input_Modifier bitmask of required modifiers.
 * @param not_mod An Efl_Input_Modifier bitmask of modifiers that must NOT be active.
 * @param exclusive If EINA_TRUE, this grab is exclusive.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EOLIAN Eina_Bool
_efl_canvas_object_key_grab(Eo *eo_obj, Evas_Object_Protected_Data *obj,
                            const char *keyname, Efl_Input_Modifier mod,
                            Efl_Input_Modifier not_mod, Eina_Bool exclusive)
{
   Evas_Modifier_Mask modifiers, not_modifiers;

   EVAS_OBJECT_DATA_VALID_CHECK(obj, EINA_FALSE);
   modifiers = _efl_input_modifier_to_evas_modifier_mask(obj->layer->evas, mod);
   not_modifiers = _efl_input_modifier_to_evas_modifier_mask(obj->layer->evas, not_mod);

   return _object_key_grab(eo_obj, obj, keyname, modifiers, not_modifiers, exclusive);
}

/**
 * @internal
 * @brief Efl_Canvas_Object API implementation for ungrabbing a key.
 * @see efl_canvas_object_key_ungrab
 *
 * This function implements the Eolian interface for key ungrabbing.
 * It converts the Efl_Input_Modifier parameters to the internal Evas_Modifier_Mask
 * format and then calls the internal _object_key_ungrab function.
 *
 * @param eo_obj The Evas object (Eo pointer).
 * @param obj The protected data of the Evas object.
 * @param keyname The name of the key to ungrab.
 * @param mod An Efl_Input_Modifier bitmask of modifiers used when grabbing.
 * @param not_mod An Efl_Input_Modifier bitmask of modifiers used when grabbing.
 */
EOLIAN void
_efl_canvas_object_key_ungrab(Eo *eo_obj, Evas_Object_Protected_Data *obj,
                              const char *keyname, Efl_Input_Modifier mod,
                              Efl_Input_Modifier not_mod)
{
   Evas_Modifier_Mask modifiers, not_modifiers;

   EVAS_OBJECT_DATA_VALID_CHECK(obj);
   modifiers = _efl_input_modifier_to_evas_modifier_mask(obj->layer->evas, mod);
   not_modifiers = _efl_input_modifier_to_evas_modifier_mask(obj->layer->evas, not_mod);

   _object_key_ungrab(eo_obj, obj, keyname, modifiers, not_modifiers);
}

// Legacy API

/**
 * @brief Grab a key press/release event for a specific Evas object.
 * @param eo_obj The object to grab the key for.
 * @param keyname The name of the key to grab (e.g. "Control_L", "Alt_L",
 * "Shift_L", "a", "b", "Up", "Down", "Escape", "F1", etc.).
 * @param modifiers A mask of modifiers that must be active for this grab
 * to trigger (E.g. EVAS_MODIFIER_CTRL | EVAS_MODIFIER_ALT).
 * @param not_modifiers A mask of modifiers that must NOT be active for this
 * grab to trigger.
 * @param exclusive Set to EINA_TRUE to make this a "greedy" grab. If a greedy
 * grab exists for a particular key and modifier combination, no other grabs
 * for the same combination will be active (though they still exist).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * This function allows an Evas object to "grab" key events. When a key is
 * pressed or released, Evas checks if any object has a grab for that specific
 * key and modifier combination. If a grab exists, the key event is sent
 * *only* to the grabbing object(s).
 *
 * If multiple objects grab the same key combination without `exclusive` set,
 * all of them will receive the event. If one or more grabs are `exclusive`,
 * only the most recently added exclusive grab will receive the event.
 *
 * The `modifiers` and `not_modifiers` masks allow fine-grained control. For
 * example, to grab "Ctrl+a" but not "Ctrl+Shift+a":
 * @code
 * evas_object_key_grab(obj, "a", EVAS_MODIFIER_CTRL, EVAS_MODIFIER_SHIFT, EINA_FALSE);
 * @endcode
 *
 * To grab "a" only when no modifiers are active:
 * @code
 * evas_object_key_grab(obj, "a", 0, EVAS_MODIFIER_CTRL | EVAS_MODIFIER_SHIFT | EVAS_MODIFIER_ALT, EINA_FALSE);
 * @endcode
 *
 * @note It is an error to set the same bits in both `modifiers` and `not_modifiers`
 * unless both are 0.
 * @note The key names are case-sensitive and should match the names used by the
 * underlying windowing system or input library (e.g., XKB).
 *
 * @see evas_object_key_ungrab()
 * @see evas_key_modifier_mask_get()
 * @ingroup Evas_Object_Group_Input
 */
EVAS_API Eina_Bool
evas_object_key_grab(Evas_Object *eo_obj, const char *keyname,
                     Evas_Modifier_Mask modifiers, Evas_Modifier_Mask not_modifiers,
                     Eina_Bool exclusive)
{
   Evas_Object_Protected_Data *obj;

   obj = EVAS_OBJECT_DATA_SAFE_GET(eo_obj);
   EVAS_OBJECT_DATA_VALID_CHECK(obj, EINA_FALSE);

   return _object_key_grab(eo_obj, obj, keyname, modifiers, not_modifiers, exclusive);
}

/**
 * @brief Ungrab a key press/release event for a specific Evas object.
 * @param eo_obj The object to ungrab the key from.
 * @param keyname The name of the key that was grabbed.
 * @param modifiers The modifier mask used when the key was grabbed.
 * @param not_modifiers The "not_modifiers" mask used when the key was grabbed.
 *
 * This function removes a key grab previously set up with
 * evas_object_key_grab(). All parameters (`keyname`, `modifiers`,
 * `not_modifiers`) must match the original call to evas_object_key_grab()
 * for the ungrab to be successful.
 *
 * If the removed grab was an exclusive grab, and other non-exclusive grabs
 * for the same key combination exist, or if other exclusive grabs exist
 * (which were previously suppressed by this one), one of them may become active.
 *
 * @see evas_object_key_grab()
 * @ingroup Evas_Object_Group_Input
 */
EVAS_API void
evas_object_key_ungrab(Efl_Canvas_Object *eo_obj, const char *keyname,
                       Evas_Modifier_Mask modifiers, Evas_Modifier_Mask not_modifiers)
{
   Evas_Object_Protected_Data *obj;

   obj = EVAS_OBJECT_DATA_SAFE_GET(eo_obj);
   EVAS_OBJECT_DATA_VALID_CHECK(obj);

   _object_key_ungrab(eo_obj, obj, keyname, modifiers, not_modifiers);
}
