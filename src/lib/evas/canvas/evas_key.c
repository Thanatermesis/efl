#include "evas_common_private.h"
#include "evas_private.h"

/* private calls */

/**
 * @internal
 * @brief Finds the numerical index of a modifier key by its name.
 *
 * This function searches through the list of registered modifier keys
 * for a given Evas_Modifier structure and returns the index of the
 * modifier key if found.
 *
 * @param m Pointer to the Evas_Modifier structure containing the list of modifiers.
 *          The structure is expected to have `mod.count` (number of modifiers)
 *          and `mod.list` (array of C-strings, each being a modifier name).
 *          Example `m->mod.list` structure: `{"Shift", "Control", "Alt"}`
 * @param keyname The name of the modifier key to search for (e.g., "Shift", "Control").
 * @return The index of the modifier key if found (0 to m->mod.count - 1),
 *         or -1 if the keyname is not found in the list.
 */
static int
evas_key_modifier_number(const Evas_Modifier *m, const char *keyname)
{
   int i;

   for (i = 0; i < m->mod.count; i++)
     {
	if (!strcmp(m->mod.list[i], keyname)) return i;
     }
   return -1;
}

/**
 * @internal
 * @brief Finds the numerical index of a lock key by its name.
 *
 * This function searches through the list of registered lock keys
 * for a given Evas_Lock structure and returns the index of the
 * lock key if found.
 *
 * @param l Pointer to the Evas_Lock structure containing the list of locks.
 *          The structure is expected to have `lock.count` (number of locks)
 *          and `lock.list` (array of C-strings, each being a lock name).
 *          Example `l->lock.list` structure: `{"Caps_Lock", "Num_Lock"}`
 * @param keyname The name of the lock key to search for (e.g., "Caps_Lock", "Num_Lock").
 * @return The index of the lock key if found (0 to l->lock.count - 1),
 *         or -1 if the keyname is not found in the list.
 */
static int
evas_key_lock_number(const Evas_Lock *l, const char *keyname)
{
   int i;

   for (i = 0; i < l->lock.count; i++)
     {
	if (!strcmp(l->lock.list[i], keyname)) return i;
     }
   return -1;
}

/* local calls */

/* public calls */

/**
 * @brief Retrieves the Evas_Modifier structure associated with an Evas canvas.
 *
 * This function provides access to the modifier key state (e.g., Shift, Ctrl)
 * for the given Evas canvas. The returned structure contains the list of
 * known modifiers and their current states.
 *
 * @param eo_e Pointer to the Evas canvas object.
 * @return A pointer to the constant Evas_Modifier structure for the canvas,
 *         or NULL if an error occurs or on invalid input.
 * @see evas_key_modifier_is_set()
 * @see evas_seat_key_modifier_is_set()
 */
EVAS_API const Evas_Modifier*
evas_key_modifier_get(const Evas *eo_e)
{
   EVAS_LEGACY_API(eo_e, e, NULL);
   return &(e->modifiers);
}

/**
 * @brief Retrieves the Evas_Lock structure associated with an Evas canvas.
 *
 * This function provides access to the lock key state (e.g., Caps_Lock, Num_Lock)
 * for the given Evas canvas. The returned structure contains the list of
 * known lock keys and their current states.
 *
 * @param eo_e Pointer to the Evas canvas object.
 * @return A pointer to the constant Evas_Lock structure for the canvas,
 *         or NULL if an error occurs or on invalid input.
 * @see evas_key_lock_is_set()
 * @see evas_seat_key_lock_is_set()
 */
EVAS_API const Evas_Lock*
evas_key_lock_get(const Evas *eo_e)
{
   EVAS_LEGACY_API(eo_e, e, NULL);
   return &(e->locks);
}

/**
 * @internal
 * @brief Checks if a specific key (modifier or lock) is set for a given seat.
 *
 * This function determines if the key corresponding to the numerical index `n`
 * is active for the specified input device (seat). It uses a bitmask
 * representation for active keys.
 *
 * @param n The numerical index of the key (0-63). This index is typically
 *          obtained from evas_key_modifier_number() or evas_key_lock_number().
 * @param masks A hash table mapping Evas_Device pointers (seats) to their
 *              respective Evas_Modifier_Mask bitmasks.
 * @param seat Pointer to the Evas_Device representing the input seat.
 * @return EINA_TRUE if the key is set (active) for the seat, EINA_FALSE otherwise.
 *         Returns EINA_FALSE if n is out of range [0, 63] or if the seat
 *         is not found in the masks.
 */
static Eina_Bool
_key_is_set(int n, Eina_Hash *masks, const Evas_Device *seat)
{
   Evas_Modifier_Mask num, *seat_mask;

   if (n < 0) return 0;
   else if (n >= 64) return 0;
   num = (Evas_Modifier_Mask)n;
   num = 1ULL << num;
   seat_mask = eina_hash_find(masks, &seat);
   if (!seat_mask) return 0;
   if (*seat_mask & num) return 1;
   return 0;
}

/**
 * @brief Checks if a named modifier key is active for a specific seat.
 *
 * This function determines if the modifier key specified by `keyname`
 * (e.g., "Shift", "Control") is currently active for the given input `seat`.
 * If `seat` is NULL, the default seat for the canvas associated with `m` is used.
 *
 * @param m Pointer to the Evas_Modifier structure (obtained via evas_key_modifier_get()).
 * @param keyname The name of the modifier key to check (e.g., "Shift").
 * @param seat Pointer to the Evas_Device representing the input seat.
 *             If NULL, the default seat is used.
 * @return EINA_TRUE if the modifier is set for the seat, EINA_FALSE otherwise.
 *         Returns EINA_FALSE on NULL `m`, NULL `keyname`, or if the seat is invalid.
 */
EVAS_API Eina_Bool
evas_seat_key_modifier_is_set(const Evas_Modifier *m, const char *keyname,
                              const Evas_Device *seat)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(m, EINA_FALSE);
   if (!seat)
     seat = m->e->default_seat;
   EINA_SAFETY_ON_NULL_RETURN_VAL(seat, 0);
   if (!keyname) return 0;
   return _key_is_set(evas_key_modifier_number(m, keyname), m->masks, seat);
}

/**
 * @brief Checks if a named modifier key is active for the default seat.
 *
 * This is a convenience function that calls evas_seat_key_modifier_is_set()
 * with the default seat associated with the Evas_Modifier structure.
 *
 * @param m Pointer to the Evas_Modifier structure.
 * @param keyname The name of the modifier key to check (e.g., "Alt").
 * @return EINA_TRUE if the modifier is set for the default seat, EINA_FALSE otherwise.
 * @see evas_seat_key_modifier_is_set()
 */
EVAS_API Eina_Bool
evas_key_modifier_is_set(const Evas_Modifier *m, const char *keyname)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(m, EINA_FALSE);
   return evas_seat_key_modifier_is_set(m, keyname, NULL);
}

/**
 * @brief Checks if a named lock key is active for the default seat.
 *
 * This is a convenience function that calls evas_seat_key_lock_is_set()
 * with the default seat associated with the Evas_Lock structure.
 *
 * @param l Pointer to the Evas_Lock structure.
 * @param keyname The name of the lock key to check (e.g., "Caps_Lock").
 * @return EINA_TRUE if the lock key is set for the default seat, EINA_FALSE otherwise.
 * @see evas_seat_key_lock_is_set()
 */
EVAS_API Eina_Bool
evas_key_lock_is_set(const Evas_Lock *l, const char *keyname)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(l, EINA_FALSE);
   return evas_seat_key_lock_is_set(l, keyname, NULL);
}

/**
 * @brief Checks if a named lock key is active for a specific seat.
 *
 * This function determines if the lock key specified by `keyname`
 * (e.g., "Num_Lock", "Scroll_Lock") is currently active for the given input `seat`.
 * If `seat` is NULL, the default seat for the canvas associated with `l` is used.
 *
 * @param l Pointer to the Evas_Lock structure (obtained via evas_key_lock_get()).
 * @param keyname The name of the lock key to check (e.g., "Num_Lock").
 * @param seat Pointer to the Evas_Device representing the input seat.
 *             If NULL, the default seat is used.
 * @return EINA_TRUE if the lock key is set for the seat, EINA_FALSE otherwise.
 *         Returns EINA_FALSE on NULL `l`, NULL `keyname`, or if the seat is invalid.
 */
EVAS_API Eina_Bool
evas_seat_key_lock_is_set(const Evas_Lock *l, const char *keyname,
                          const Evas_Device *seat)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(l, EINA_FALSE);
   if (!seat)
     seat = l->e->default_seat;
   EINA_SAFETY_ON_NULL_RETURN_VAL(seat, 0);
   if (!keyname) return 0;
   return _key_is_set(evas_key_lock_number(l, keyname), l->masks, seat);
}

/**
 * @internal
 * @brief Adds a new modifier key name to the canvas's list of known modifiers.
 * @ingroup Evas_Canvas_Group
 *
 * This function registers a new modifier key (e.g., "Super", "Hyper") with the Evas canvas.
 * If the modifier already exists, it is effectively re-added (no duplication, but internal state might be reset).
 * The maximum number of modifiers is limited to 64.
 * Adding a modifier clears any previously cached modifier masks for all seats,
 * requiring them to be re-evaluated.
 *
 * @param eo_e The Evas canvas object.
 * @param e Pointer to the Evas public data structure.
 * @param keyname The name of the modifier key to add. Must not be NULL.
 *                The string is duplicated internally.
 */
EOLIAN void
_evas_canvas_key_modifier_add(Eo *eo_e, Evas_Public_Data *e, const char *keyname)
{
   if (!keyname) return;
   if (e->modifiers.mod.count >= 64) return;
   evas_key_modifier_del(eo_e, keyname);
   e->modifiers.mod.count++;
   e->modifiers.mod.list = realloc(e->modifiers.mod.list, e->modifiers.mod.count * sizeof(char *));
   e->modifiers.mod.list[e->modifiers.mod.count - 1] = strdup(keyname);
   eina_hash_free_buckets(e->modifiers.masks);
}

/**
 * @internal
 * @brief Deletes a modifier key name from the canvas's list of known modifiers.
 * @ingroup Evas_Canvas_Group
 *
 * This function unregisters a modifier key from the Evas canvas.
 * If the modifier is found and removed, its associated memory is freed.
 * Deleting a modifier clears any previously cached modifier masks for all seats.
 *
 * @param eo_e The Evas canvas object (unused in current implementation but part of EOLIAN signature).
 * @param e Pointer to the Evas public data structure.
 * @param keyname The name of the modifier key to delete. If NULL or not found, the function does nothing.
 */
EOLIAN void
_evas_canvas_key_modifier_del(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e, const char *keyname)
{
   int i;

   if (!keyname) return;
   for (i = 0; i < e->modifiers.mod.count; i++)
     {
	if (!strcmp(e->modifiers.mod.list[i], keyname))
	  {
	     int j;

	     free(e->modifiers.mod.list[i]);
	     e->modifiers.mod.count--;
	     for (j = i; j < e->modifiers.mod.count; j++)
	       e->modifiers.mod.list[j] = e->modifiers.mod.list[j + 1];
             eina_hash_free_buckets(e->modifiers.masks);
	     return;
	  }
     }
}

/**
 * @internal
 * @brief Adds a new lock key name to the canvas's list of known locks.
 * @ingroup Evas_Canvas_Group
 *
 * This function registers a new lock key (e.g., "Kana_Lock") with the Evas canvas.
 * If the lock key already exists, it is effectively re-added.
 * The maximum number of lock keys is limited to 64.
 * Adding a lock key clears any previously cached lock masks for all seats.
 *
 * @param eo_e The Evas canvas object.
 * @param e Pointer to the Evas public data structure.
 * @param keyname The name of the lock key to add. Must not be NULL.
 *                The string is duplicated internally.
 */
EOLIAN void
_evas_canvas_key_lock_add(Eo *eo_e, Evas_Public_Data *e, const char *keyname)
{
   if (!keyname) return;
   if (e->locks.lock.count >= 64) return;
   evas_key_lock_del(eo_e, keyname);
   e->locks.lock.count++;
   e->locks.lock.list = realloc(e->locks.lock.list, e->locks.lock.count * sizeof(char *));
   e->locks.lock.list[e->locks.lock.count - 1] = strdup(keyname);
   eina_hash_free_buckets(e->locks.masks);
}

/**
 * @internal
 * @brief Deletes a lock key name from the canvas's list of known locks.
 * @ingroup Evas_Canvas_Group
 *
 * This function unregisters a lock key from the Evas canvas.
 * If the lock key is found and removed, its associated memory is freed.
 * Deleting a lock key clears any previously cached lock masks for all seats.
 *
 * @param eo_e The Evas canvas object (unused in current implementation but part of EOLIAN signature).
 * @param e Pointer to the Evas public data structure.
 * @param keyname The name of the lock key to delete. If NULL or not found, the function does nothing.
 */
EOLIAN void
_evas_canvas_key_lock_del(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e, const char *keyname)
{
   int i;
   if (!keyname) return;
   for (i = 0; i < e->locks.lock.count; i++)
     {
	if (!strcmp(e->locks.lock.list[i], keyname))
	  {
	     int j;

	     free(e->locks.lock.list[i]);
	     e->locks.lock.count--;
	     for (j = i; j < e->locks.lock.count; j++)
	       e->locks.lock.list[j] = e->locks.lock.list[j + 1];
             eina_hash_free_buckets(e->locks.masks);
	     return;
	  }
     }
}

/**
 * @internal
 * @brief Sets or clears a bit in the key mask for a specific seat.
 *
 * This function updates the bitmask associated with a given `seat` in the `masks`
 * hash table. The bit corresponding to the key index `n` is either set (if `add` is EINA_TRUE)
 * or cleared (if `add` is EINA_FALSE).
 * If `add` is EINA_TRUE and no mask exists for the seat, a new mask is allocated and added.
 * If `add` is EINA_FALSE and clearing the bit results in an all-zero mask, the entry for
 * the seat might be removed from the hash (depending on eina_hash_del_by_key behavior with calloc'd data).
 *
 * @param n The numerical index of the key (modifier or lock), typically 0-63.
 *          Values outside this range are ignored.
 * @param masks Pointer to the Eina_Hash table storing seat-to-mask mappings.
 *              The keys are `Efl_Input_Device*` (seats) and values are `Evas_Modifier_Mask*`.
 * @param seat Pointer to the Efl_Input_Device (seat) whose mask is to be modified.
 * @param add If EINA_TRUE, sets the bit for key `n`. If EINA_FALSE, clears the bit.
 */
static void
_mask_set(int n, Eina_Hash *masks, Efl_Input_Device *seat, Eina_Bool add)
{
   Evas_Modifier_Mask *current_mask;
   Evas_Modifier_Mask num;

   if (n < 0 || n > 63) return;
   num = 1ULL << n;

   current_mask = eina_hash_find(masks, &seat);
   if (add)
     {
        if (!current_mask)
          {
             current_mask = calloc(1, sizeof(Evas_Modifier_Mask));
             EINA_SAFETY_ON_NULL_RETURN(current_mask);
             eina_hash_add(masks, &seat, current_mask);
          }
        *current_mask |= num;
     }
   else
     {
        if (!current_mask) return;
        *current_mask &= ~num;
        if (!(*current_mask))
          eina_hash_del_by_key(masks, &seat);
     }
}

/**
 * @internal
 * @brief Turns a specific modifier key ON for a given seat.
 * @ingroup Evas_Canvas_Group
 *
 * This function marks the modifier key specified by `keyname` as active (ON)
 * for the input `seat`. If `seat` is NULL, the default seat of the canvas is used.
 * The seat must be of type EFL_INPUT_DEVICE_TYPE_SEAT.
 *
 * @param eo_e The Evas canvas object (unused).
 * @param e Pointer to the Evas public data structure.
 * @param keyname The name of the modifier key to turn ON (e.g., "Shift").
 * @param seat The specific input seat. If NULL, uses the default seat.
 */
EOLIAN void
_evas_canvas_seat_key_modifier_on(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e,
                                  const char *keyname, Efl_Input_Device *seat)
{
   if (!seat)
     seat = e->default_seat;
   EINA_SAFETY_ON_NULL_RETURN(seat);
   if (efl_input_device_type_get(seat) != EFL_INPUT_DEVICE_TYPE_SEAT) return;
   _mask_set(evas_key_modifier_number(&(e->modifiers), keyname),
             e->modifiers.masks, seat, EINA_TRUE);
}

/**
 * @internal
 * @brief Turns a specific modifier key OFF for a given seat.
 * @ingroup Evas_Canvas_Group
 *
 * This function marks the modifier key specified by `keyname` as inactive (OFF)
 * for the input `seat`. If `seat` is NULL, the default seat of the canvas is used.
 *
 * @param eo_e The Evas canvas object (unused).
 * @param e Pointer to the Evas public data structure.
 * @param keyname The name of the modifier key to turn OFF (e.g., "Control").
 * @param seat The specific input seat. If NULL, uses the default seat.
 */
EOLIAN void
_evas_canvas_seat_key_modifier_off(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e,
                                   const char *keyname, Efl_Input_Device *seat)
{
   if (!seat)
     seat = e->default_seat;
   EINA_SAFETY_ON_NULL_RETURN(seat);
   _mask_set(evas_key_modifier_number(&(e->modifiers), keyname),
             e->modifiers.masks, seat, EINA_FALSE);
}

/**
 * @internal
 * @brief Turns a specific modifier key ON for the default seat.
 * @ingroup Evas_Canvas_Group
 *
 * This is a convenience function that calls _evas_canvas_seat_key_modifier_on()
 * with a NULL seat, effectively targeting the default seat of the canvas.
 *
 * @param eo_e The Evas canvas object.
 * @param e Pointer to the Evas public data structure.
 * @param keyname The name of the modifier key to turn ON.
 */
EOLIAN void
_evas_canvas_key_modifier_on(Eo *eo_e, Evas_Public_Data *e, const char *keyname)
{
   _evas_canvas_seat_key_modifier_on(eo_e, e, keyname, NULL);
}

/**
 * @internal
 * @brief Turns a specific modifier key OFF for the default seat.
 * @ingroup Evas_Canvas_Group
 *
 * This is a convenience function that calls _evas_canvas_seat_key_modifier_off()
 * with a NULL seat, effectively targeting the default seat of the canvas.
 *
 * @param eo_e The Evas canvas object.
 * @param e Pointer to the Evas public data structure.
 * @param keyname The name of the modifier key to turn OFF.
 */
EOLIAN void
_evas_canvas_key_modifier_off(Eo *eo_e, Evas_Public_Data *e,
                              const char *keyname)
{
   _evas_canvas_seat_key_modifier_off(eo_e, e, keyname, NULL);
}

/**
 * @internal
 * @brief Turns a specific lock key ON for a given seat.
 * @ingroup Evas_Canvas_Group
 *
 * This function marks the lock key specified by `keyname` as active (ON)
 * for the input `seat`. If `seat` is NULL, the default seat of the canvas is used.
 * The seat must be of type EFL_INPUT_DEVICE_TYPE_SEAT.
 *
 * @param eo_e The Evas canvas object (unused).
 * @param e Pointer to the Evas public data structure.
 * @param keyname The name of the lock key to turn ON (e.g., "Caps_Lock").
 * @param seat The specific input seat. If NULL, uses the default seat.
 */
EOLIAN void
_evas_canvas_seat_key_lock_on(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e,
                              const char *keyname, Efl_Input_Device *seat)
{
   if (!seat)
     seat = e->default_seat;
   EINA_SAFETY_ON_NULL_RETURN(seat);
   if (efl_input_device_type_get(seat) != EFL_INPUT_DEVICE_TYPE_SEAT) return;
   _mask_set(evas_key_lock_number(&(e->locks), keyname), e->locks.masks,
             seat, EINA_TRUE);
}

/**
 * @internal
 * @brief Turns a specific lock key OFF for a given seat.
 * @ingroup Evas_Canvas_Group
 *
 * This function marks the lock key specified by `keyname` as inactive (OFF)
 * for the input `seat`. If `seat` is NULL, the default seat of the canvas is used.
 *
 * @param eo_e The Evas canvas object (unused).
 * @param e Pointer to the Evas public data structure.
 * @param keyname The name of the lock key to turn OFF (e.g., "Num_Lock").
 * @param seat The specific input seat. If NULL, uses the default seat.
 */
EOLIAN void
_evas_canvas_seat_key_lock_off(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e,
                               const char *keyname, Efl_Input_Device *seat)
{
   if (!seat)
     seat = e->default_seat;
   EINA_SAFETY_ON_NULL_RETURN(seat);
   _mask_set(evas_key_lock_number(&(e->locks), keyname), e->locks.masks,
             seat, EINA_FALSE);
}

/**
 * @internal
 * @brief Turns a specific lock key ON for the default seat.
 * @ingroup Evas_Canvas_Group
 *
 * This is a convenience function that calls _evas_canvas_seat_key_lock_on()
 * with a NULL seat, effectively targeting the default seat of the canvas.
 *
 * @param eo_e The Evas canvas object.
 * @param e Pointer to the Evas public data structure.
 * @param keyname The name of the lock key to turn ON.
 */
EOLIAN void
_evas_canvas_key_lock_on(Eo *eo_e, Evas_Public_Data *e, const char *keyname)
{
   _evas_canvas_seat_key_lock_on(eo_e, e, keyname, NULL);
}

/**
 * @internal
 * @brief Turns a specific lock key OFF for the default seat.
 * @ingroup Evas_Canvas_Group
 *
 * This is a convenience function that calls _evas_canvas_seat_key_lock_off()
 * with a NULL seat, effectively targeting the default seat of the canvas.
 *
 * @param eo_e The Evas canvas object.
 * @param e Pointer to the Evas public data structure.
 * @param keyname The name of the lock key to turn OFF.
 */
EOLIAN void
_evas_canvas_key_lock_off(Eo *eo_e, Evas_Public_Data *e, const char *keyname)
{
   _evas_canvas_seat_key_lock_off(eo_e, e, keyname, NULL);
}

/* errr need to add key grabbing/ungrabbing calls - missing modifier stuff. */

/**
 * @brief Retrieves the bitmask for a named modifier key.
 *
 * This function returns a bitmask representing the modifier key specified by `keyname`.
 * Each known modifier key is assigned a unique bit in an Evas_Modifier_Mask (a 64-bit unsigned integer).
 * For example, if "Shift" is the 0th modifier, its mask is `1ULL << 0`.
 * If "Control" is the 1st modifier, its mask is `1ULL << 1`.
 *
 * @param eo_e Pointer to the Evas canvas object.
 * @param keyname The name of the modifier key (e.g., "Shift", "Control", "Alt").
 * @return The Evas_Modifier_Mask for the given keyname.
 *         Returns 0 if `keyname` is NULL, not a registered modifier,
 *         or if its index is out of the valid range [0, 63].
 *         Example return: `0x01` (for the first modifier), `0x02` (for the second), etc.
 */
EVAS_API Evas_Modifier_Mask
evas_key_modifier_mask_get(const Evas *eo_e, const char *keyname)
{
   int n;

   if (!keyname) return 0;
   EVAS_LEGACY_API(eo_e, e, 0);
   n = evas_key_modifier_number(&(e->modifiers), keyname);
   if (n < 0 || n > 63) return 0;
   return 1ULL << n;
}

