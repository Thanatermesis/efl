#include "elput_private.h"

/**
 * @brief Frees the resources associated with a seat event.
 *
 * This function is typically used as a callback for ecore_event_add when
 * an event related to a seat (like ELPUT_EVENT_SEAT_CAPS or
 * ELPUT_EVENT_SEAT_FRAME) is added. It ensures that the seat data
 * (referenced by @p d) is properly destroyed and the event data (@p ev)
 * is freed.
 *
 * @param d User data, expected to be an Elput_Seat or related structure
 *          that needs to be destroyed via _udev_seat_destroy.
 * @param ev The event data to be freed.
 */
static void
_seat_event_free(void *d, void *ev)
{
   _udev_seat_destroy(d);
   free(ev);
}

/**
 * @brief Updates and sends an event about the capabilities of a seat.
 *
 * This function is called when the capabilities of a seat (e.g., number of
 * pointers, keyboards, touch devices) might have changed. It allocates an
 * Elput_Event_Seat_Caps event, populates it with the current counts from
 * the seat, and adds it to the ecore event queue.
 *
 * @param seat The seat whose capabilities need to be updated and broadcast.
 */
static void
_seat_caps_update(Elput_Seat *seat)
{
   Elput_Event_Seat_Caps *ev;

   ev = calloc(1, sizeof(Elput_Event_Seat_Caps));
   if (!ev) return;

   ev->pointer_count = seat->count.ptr;
   ev->keyboard_count = seat->count.kbd;
   ev->touch_count = seat->count.touch;
   ev->seat = seat;
   seat->refs++;

   ecore_event_add(ELPUT_EVENT_SEAT_CAPS, ev, _seat_event_free, seat);
}

/**
 * @brief Sends a seat frame event.
 *
 * A frame event indicates that a logical set of input events has been
 * processed and that the application can now update its state or redraw.
 * This function allocates an Elput_Event_Seat_Frame event and adds it to
 * the ecore event queue.
 *
 * @param seat The seat for which the frame event is being sent.
 */
static void
_seat_frame_send(Elput_Seat *seat)
{
   Elput_Event_Seat_Frame *ev;

   ev = calloc(1, sizeof(Elput_Event_Seat_Frame));
   if (!ev) return;

   ev->seat = seat;
   seat->refs++;
   ecore_event_add(ELPUT_EVENT_SEAT_FRAME, ev, _seat_event_free, seat);
}

/**
 * @brief Updates the physical LEDs on a keyboard device.
 *
 * Translates Elput_Leds bitmask (NUM, CAPS, SCROLL) into libinput's
 * LED equivalents and instructs libinput to update the LEDs on the
 * specified device.
 *
 * @param edev The Elput_Device representing the keyboard.
 * @param leds A bitmask of Elput_Leds to be set on the keyboard.
 *             Example: ELPUT_LED_NUM | ELPUT_LED_CAPS
 */
static void
_evdev_leds_update(Elput_Device *edev, Elput_Leds leds)
{
   enum libinput_led input_leds = 0;

   if (leds & ELPUT_LED_NUM)
     input_leds |= LIBINPUT_LED_NUM_LOCK;
   if (leds & ELPUT_LED_CAPS)
     input_leds |= LIBINPUT_LED_CAPS_LOCK;
   if (leds & ELPUT_LED_SCROLL)
     input_leds |= LIBINPUT_LED_SCROLL_LOCK;

   libinput_device_led_update(edev->device, input_leds);
}

/**
 * @brief Updates the modifier state for a keyboard and its associated seat.
 *
 * This function serializes the depressed, latched, locked, and effective
 * group states from the XKB state. It then translates these XKB modifier
 * states into Ecore_Event_Modifier flags on the seat. It also checks if
 * the LED state has changed based on active XKB LEDs and, if so, updates
 * the physical LEDs on all keyboard devices associated with the seat.
 *
 * @param kbd The keyboard whose XKB state is used to derive modifiers.
 * @param seat The seat whose modifier flags will be updated.
 */
static void
_keyboard_modifiers_update(Elput_Keyboard *kbd, Elput_Seat *seat)
{
   xkb_mod_mask_t mask;
   Elput_Leds leds = 0;

   kbd->mods.depressed =
     xkb_state_serialize_mods(kbd->state, XKB_STATE_DEPRESSED);
   kbd->mods.latched =
     xkb_state_serialize_mods(kbd->state, XKB_STATE_LATCHED);
   kbd->mods.locked =
     xkb_state_serialize_mods(kbd->state, XKB_STATE_LOCKED);
   kbd->mods.group =
     xkb_state_serialize_mods(kbd->state, XKB_STATE_EFFECTIVE);

   mask = (kbd->mods.depressed | kbd->mods.latched);

   seat->modifiers = 0;
   if (mask & kbd->info->mods.ctrl)
     seat->modifiers |= ECORE_EVENT_MODIFIER_CTRL;
   if (mask & kbd->info->mods.alt)
     seat->modifiers |= ECORE_EVENT_MODIFIER_ALT;
   if (mask & kbd->info->mods.shift)
     seat->modifiers |= ECORE_EVENT_MODIFIER_SHIFT;
   if (mask & kbd->info->mods.super)
     seat->modifiers |= ECORE_EVENT_MODIFIER_WIN;
   if (mask & kbd->info->mods.altgr)
     seat->modifiers |= ECORE_EVENT_MODIFIER_ALTGR;
   if (mask & kbd->info->mods.caps)
     seat->modifiers |= ECORE_EVENT_MODIFIER_CAPS;
   if (mask & kbd->info->mods.num)
     seat->modifiers |= ECORE_EVENT_MODIFIER_NUM;

   if (kbd->mods.locked & kbd->info->mods.caps)
     seat->modifiers |= ECORE_EVENT_LOCK_CAPS;
   if (kbd->mods.locked & kbd->info->mods.num)
     seat->modifiers |= ECORE_EVENT_LOCK_NUM;

   if (xkb_state_led_index_is_active(kbd->state, kbd->info->leds.num))
     leds |= ELPUT_LED_NUM;

   if (xkb_state_led_index_is_active(kbd->state, kbd->info->leds.caps))
     leds |= ELPUT_LED_CAPS;

   if (xkb_state_led_index_is_active(kbd->state, kbd->info->leds.scroll))
     leds |= ELPUT_LED_SCROLL;

   if (kbd->leds != leds)
     {
        Eina_List *l;
        Elput_Device *edev;

        EINA_LIST_FOREACH(seat->devices, l, edev)
          _evdev_leds_update(edev, leds);

        kbd->leds = leds;
     }
}

/**
 * @brief Creates and initializes an Elput_Keyboard_Info structure from an XKB keymap.
 *
 * This function allocates an Elput_Keyboard_Info structure, takes a reference
 * to the provided XKB keymap, and populates the structure with modifier and
 * LED indices obtained from the keymap. These indices are used for quick
 * lookup of common modifiers (Ctrl, Alt, Shift, Super, etc.) and LEDs
 * (Num Lock, Caps Lock, Scroll Lock).
 *
 * @param keymap The XKB keymap from which to derive keyboard information.
 *               The function will take its own reference to this keymap.
 * @return A pointer to the newly created Elput_Keyboard_Info structure,
 *         or NULL on allocation failure. The caller is responsible for
 *         eventually calling _keyboard_info_destroy on the returned structure.
 */
static Elput_Keyboard_Info *
_keyboard_info_create(struct xkb_keymap *keymap)
{
   Elput_Keyboard_Info *info;

   info = calloc(1, sizeof(Elput_Keyboard_Info));
   if (!info) return NULL;

   info->keymap.map = xkb_keymap_ref(keymap);
   info->refs = 1;

   info->mods.super =
     1 << xkb_keymap_mod_get_index(info->keymap.map, XKB_MOD_NAME_LOGO);
   info->mods.shift =
     1 << xkb_keymap_mod_get_index(info->keymap.map, XKB_MOD_NAME_SHIFT);
   info->mods.caps =
     1 << xkb_keymap_mod_get_index(info->keymap.map, XKB_MOD_NAME_CAPS);
   info->mods.num =
     1 << xkb_keymap_mod_get_index(info->keymap.map, "Mod2");
   info->mods.ctrl =
     1 << xkb_keymap_mod_get_index(info->keymap.map, XKB_MOD_NAME_CTRL);
   info->mods.alt =
     1 << xkb_keymap_mod_get_index(info->keymap.map, XKB_MOD_NAME_ALT);
   info->mods.altgr =
     1 << xkb_keymap_mod_get_index(info->keymap.map, "ISO_Level3_Shift");

   info->leds.num =
     xkb_keymap_led_get_index(info->keymap.map, XKB_LED_NAME_NUM);
   info->leds.caps =
     xkb_keymap_led_get_index(info->keymap.map, XKB_LED_NAME_CAPS);
   info->leds.scroll =
     xkb_keymap_led_get_index(info->keymap.map, XKB_LED_NAME_SCROLL);

   return info;
}

/**
 * @brief Decrements the reference count of an Elput_Keyboard_Info structure and frees it if the count reaches zero.
 *
 * This function unrefs the XKB keymap associated with the info structure
 * and then frees the structure itself when its reference count drops to zero.
 *
 * @param info The Elput_Keyboard_Info structure to destroy.
 */
static void
_keyboard_info_destroy(Elput_Keyboard_Info *info)
{
   if (--info->refs > 0) return;

   xkb_keymap_unref(info->keymap.map);

   free(info);
}

/**
 * @brief Creates the XKB context and keymap.
 *
 * @param kbd The keyboard config to be set up for XKB.
 * @return EINA_TRUE if build successful, EINA_FALSE otherwise.
 *
 * Sets default settings for the keyboard if not set by the system.
 * Assumes evdev rules, with a pc105 model keyboard and US key layout.
 */
static Eina_Bool
_keyboard_global_build(Elput_Keyboard *kbd)
{
   struct xkb_keymap *keymap;

   kbd->context = xkb_context_new(0);
   if (!kbd->context) return EINA_FALSE;

   if (!kbd->names.rules) kbd->names.rules = strdup("evdev");
   if (!kbd->names.model) kbd->names.model = strdup("pc105");
   if (!kbd->names.layout) kbd->names.layout = strdup("us");

   keymap = xkb_keymap_new_from_names(kbd->context, &kbd->names, 0);
   if (!keymap) return EINA_FALSE;

   kbd->info = _keyboard_info_create(keymap);
   xkb_keymap_unref(keymap);

   if (!kbd->info) return EINA_FALSE;
   return EINA_TRUE;
}

/**
 * @brief Allocates and initializes a basic Elput_Keyboard structure.
 *
 * This function allocates memory for an Elput_Keyboard structure and
 * associates it with the given seat. Further initialization, such as
 * setting up XKB context and keymap, is done elsewhere (e.g., in
 * _keyboard_init or _keyboard_global_build).
 *
 * @param seat The Elput_Seat to which this keyboard will belong.
 * @return A pointer to the newly allocated Elput_Keyboard structure,
 *         or NULL on allocation failure.
 */
static Elput_Keyboard *
_keyboard_create(Elput_Seat *seat)
{
   Elput_Keyboard *kbd;

   kbd = calloc(1, sizeof(Elput_Keyboard));
   if (!kbd) return NULL;

   kbd->seat = seat;

   return kbd;
}

/**
 * @brief Initializes or reinitializes the XKB compose state for a keyboard.
 *
 * This function determines the current locale (from LC_ALL, LC_CTYPE, or LANG
 * environment variables, defaulting to "C") and creates an XKB compose table
 * and state based on that locale. If a compose table or state already exists
 * for the keyboard, they are unreferenced first. This allows for dynamic
 * updates to compose behavior if the locale changes.
 *
 * @param kbd The Elput_Keyboard for which to initialize the compose state.
 */
static void
_keyboard_compose_init(Elput_Keyboard *kbd)
{
   const char *locale;

   if (!(locale = getenv("LC_ALL")))
     if (!(locale = getenv("LC_CTYPE")))
       if (!(locale = getenv("LANG")))
         locale = "C";

   if (kbd->compose_table) xkb_compose_table_unref(kbd->compose_table);
   kbd->compose_table = xkb_compose_table_new_from_locale(kbd->context, locale,
     XKB_COMPOSE_COMPILE_NO_FLAGS);
   if (kbd->compose_state) xkb_compose_state_unref(kbd->compose_state);
   if (kbd->compose_table)
     {
        kbd->compose_state =
          xkb_compose_state_new(kbd->compose_table, XKB_COMPOSE_STATE_NO_FLAGS);
     }
   else
     kbd->compose_state = NULL;
}

/**
 * Create a new keyboard object for the seat, initialized
 * to the given keymap.
 */
static Eina_Bool
_keyboard_init(Elput_Seat *seat, struct xkb_keymap *keymap)
{
   Elput_Keyboard *kbd;

   if (seat->kbd)
     {
        seat->count.kbd += 1;
        if (seat->count.kbd == 1)
          _seat_caps_update(seat);
        return EINA_TRUE;
     }

   kbd = _keyboard_create(seat);
   if (!kbd) return EINA_FALSE;

   if (keymap)
     {
        if (seat->manager->cached.keymap == keymap)
          kbd->context = xkb_context_ref(seat->manager->cached.context);
        kbd->info = _keyboard_info_create(keymap);
        if (!kbd->info) goto err;
     }
   else
     {
        if (!_keyboard_global_build(kbd)) goto err;
        kbd->info->refs++;
     }

   kbd->state = xkb_state_new(kbd->info->keymap.map);
   if (!kbd->state) goto err;
   kbd->maskless_state = xkb_state_new(kbd->info->keymap.map);

   _keyboard_compose_init(kbd);

   seat->kbd = kbd;
   seat->count.kbd = 1;

   _seat_caps_update(seat);

   return EINA_TRUE;

err:
   if (kbd->info) _keyboard_info_destroy(kbd->info);
   free(kbd);
   return EINA_FALSE;
}

/**
 * @brief Resets the XKB state of a keyboard.
 *
 * This function creates a new, clean XKB state object based on the
 * keyboard's current keymap and replaces the existing state. This is
 * typically done when a keyboard device is released or its state needs
 * to be cleared (e.g., to remove any stuck modifiers).
 *
 * @param kbd The Elput_Keyboard whose XKB state is to be reset.
 */
static void
_keyboard_state_reset(Elput_Keyboard *kbd)
{
   struct xkb_state *state;

   state = xkb_state_new(kbd->info->keymap.map);
   if (!state) return;

   xkb_state_unref(kbd->state);
   kbd->state = state;
}

/**
 * @brief Decrements the keyboard count for a seat and updates capabilities if it reaches zero.
 *
 * This function is called when a keyboard device is removed or disassociated
 * from a seat. It decrements the seat's keyboard counter. If the count
 * drops to zero, it means there are no more active keyboards on the seat,
 * so the XKB state of the seat's keyboard object is reset, and a seat
 * capabilities update event is triggered.
 *
 * @param seat The Elput_Seat from which a keyboard is being released.
 */
static void
_keyboard_release(Elput_Seat *seat)
{
   seat->count.kbd--;
   if (seat->count.kbd == 0)
     {
        _keyboard_state_reset(seat->kbd);
        _seat_caps_update(seat);
     }
}

/**
 * @brief Frees an Ecore_Event_Key and unreferences its associated Evas_Device.
 *
 * This function is used as a callback for ecore_event_add when keyboard
 * or mouse events are generated. It unreferences the Evas_Device (if any)
 * associated with the event and then frees the event structure itself.
 *
 * @param dev User data, expected to be an Evas_Device (or NULL) associated
 *            with the event. This device will be unreferenced.
 * @param ev The event data (e.g., Ecore_Event_Key, Ecore_Event_Mouse_Button)
 *           to be freed.
 */
static void
_event_free(void *dev, void *ev)
{
   if (dev) efl_unref(dev);
   free(ev);
}

/**
 * Creates an event key object and generates a corresponding event.
 *
 * An ECORE_EVENT_KEY_DOWN event is generated on state
 * LIBINPUT_KEY_STATE_PRESSED, or ECORE_EVENT_KEY_DOWN otherwise.
 */
static void
_keyboard_key_send(Elput_Device *dev, enum libinput_key_state state, const char *keyname, const char *key, const char *compose, unsigned int code, unsigned int timestamp)
{
   Ecore_Event_Key *ev;

   ev = calloc(1, sizeof(Ecore_Event_Key) + strlen(key) + strlen(keyname) +
               ((compose[0] != '\0') ? strlen(compose) : 0) + 3);
   if (!ev) return;

   ev->keyname = (char *)(ev + 1);
   ev->key = ev->keyname + strlen(keyname) + 1;
   ev->compose = strlen(compose) ? ev->key + strlen(key) + 1 : NULL;
   ev->string = ev->compose;

   strcpy((char *)ev->keyname, keyname);
   strcpy((char *)ev->key, key);
   if (strlen(compose)) strcpy((char *)ev->compose, compose);

   ev->keycode = code;
   ev->modifiers = dev->seat->modifiers;
   ev->timestamp = timestamp;
   ev->same_screen = 1;
   ev->dev = dev->evas_device;
   if (ev->dev) efl_ref(ev->dev);

   ev->window = dev->seat->manager->window;
   ev->event_window = dev->seat->manager->window;
   ev->root_window = dev->seat->manager->window;

   if (state == LIBINPUT_KEY_STATE_PRESSED)
     ecore_event_add(ECORE_EVENT_KEY_DOWN, ev, _event_free, ev->dev);
   else
     ecore_event_add(ECORE_EVENT_KEY_UP, ev, _event_free, ev->dev);
}

/**
 * @brief Sends an event containing the current XKB modifier states.
 *
 * This function allocates an Elput_Event_Modifiers_Send event, populates it
 * with the depressed, latched, locked, and group modifier states from the
 * provided keyboard's XKB state, and adds it to the ecore event queue.
 * This is used to notify interested parties (e.g., a window manager or
 * toolkit) about changes in the raw XKB modifier states.
 *
 * @param kbd The Elput_Keyboard whose modifier states are to be sent.
 */
static void
_keyboard_modifiers_send(Elput_Keyboard *kbd)
{
   Elput_Event_Modifiers_Send *ev;

   ev = calloc(1, sizeof(Elput_Event_Modifiers_Send));
   if (!ev) return;

   ev->depressed = kbd->mods.depressed;
   ev->latched = kbd->mods.latched;
   ev->locked = kbd->mods.locked;
   ev->group = kbd->mods.group;

   ecore_event_add(ELPUT_EVENT_MODIFIERS_SEND, ev, NULL, NULL);
}

/**
 * @brief Updates the XKB state for a keyboard, typically after a keymap change.
 *
 * This function creates new XKB state objects (both regular and maskless)
 * based on the provided new keymap. It preserves the currently latched and
 * locked modifiers from the old state and applies them to the new state.
 * The keyboard's group is also applied from the seat manager's cached group.
 *
 * @param kbd The Elput_Keyboard whose XKB state is to be updated.
 * @param map The new XKB keymap to use for creating the state.
 * @param[out] latched Pointer to store the serialized latched modifiers from the old state.
 * @param[out] locked Pointer to store the serialized locked modifiers from the old state.
 * @return EINA_TRUE if the state was successfully updated, EINA_FALSE otherwise (e.g., on allocation failure).
 */
static Eina_Bool
_keyboard_state_update(Elput_Keyboard *kbd, struct xkb_keymap *map, xkb_mod_mask_t *latched, xkb_mod_mask_t *locked)
{
   struct xkb_state *state, *maskless_state;

   state = xkb_state_new(map);
   if (!state) return EINA_FALSE;
   maskless_state = xkb_state_new(map);
   if (!maskless_state)
     {
        xkb_state_unref(state);
        return EINA_FALSE;
     }

   *latched = xkb_state_serialize_mods(kbd->state, XKB_STATE_MODS_LATCHED);
   *locked = xkb_state_serialize_mods(kbd->state, XKB_STATE_MODS_LOCKED);
   xkb_state_update_mask(state, 0, *latched, *locked,
                         kbd->seat->manager->cached.group, 0, 0);

   xkb_state_unref(kbd->state);
   kbd->state = state;
   xkb_state_unref(kbd->maskless_state);
   kbd->maskless_state = maskless_state;
   return EINA_TRUE;
}

/**
 * @brief Updates the keymap for the keyboard associated with a seat.
 *
 * This function is called when the system keymap changes. It retrieves the
 * keyboard for the seat and marks it as pending a keymap update. If there are
 * no keys currently pressed (key_count is 0), it proceeds to update the
 * keymap immediately.
 *
 * The update involves:
 * 1. If a cached keymap exists in the seat manager, it uses that.
 *    A new Elput_Keyboard_Info is created from this cached keymap.
 * 2. Otherwise, it performs a global build of the keyboard (defaulting to
 *    "evdev" rules, "pc105" model, "us" layout).
 * 3. The XKB state is updated using _keyboard_state_update, preserving
 *    latched and locked modifiers.
 * 4. The old Elput_Keyboard_Info is destroyed, and the new one is assigned.
 * 5. The XKB compose state is re-initialized.
 * 6. Keyboard modifiers are updated and, if necessary, an event is sent.
 *
 * @param seat The Elput_Seat whose keyboard keymap needs to be updated.
 */
void
_keyboard_keymap_update(Elput_Seat *seat)
{
   Elput_Keyboard *kbd;
   Elput_Keyboard_Info *info = NULL;
   xkb_mod_mask_t latched, locked;
   Eina_Bool state = EINA_TRUE;

   kbd = _evdev_keyboard_get(seat);
   if (!kbd) return;
   kbd->pending_keymap = 1;
   if (kbd->key_count) return;

   if (kbd->seat->manager->cached.keymap)
     {
        if (kbd->context) xkb_context_unref(kbd->context);
        kbd->context = xkb_context_ref(kbd->seat->manager->cached.context);
        info = _keyboard_info_create(kbd->seat->manager->cached.keymap);
        if (!info) return;
        state = _keyboard_state_update(kbd, info->keymap.map,
                                       &latched, &locked);
     }
   else if (!_keyboard_global_build(kbd))
     return;
   else
     state = _keyboard_state_update(kbd, kbd->info->keymap.map,
                                    &latched, &locked);

   kbd->pending_keymap = 0;
   if (!state)
     {
        if (info) _keyboard_info_destroy(info);
        return;
     }

   if (info)
     {
        _keyboard_info_destroy(kbd->info);
        kbd->info = info;
     }

   _keyboard_compose_init(kbd);

   _keyboard_modifiers_update(kbd, seat);

   if ((!latched) && (!locked)) return;

   _keyboard_modifiers_send(kbd);
}

/**
 * @brief Updates the keyboard layout group for the keyboard associated with a seat.
 *
 * This function is called when the active keyboard layout group changes.
 * It retrieves the keyboard for the seat, updates its XKB state using the
 * existing keymap but applying the new group (implicitly handled by
 * _keyboard_state_update which uses seat->manager->cached.group).
 * It then re-initializes the XKB compose state (as compose sequences can
 * be layout-dependent) and updates the keyboard modifiers. If latched or
 * locked modifiers are active, a modifier update event is sent.
 *
 * @param seat The Elput_Seat whose keyboard group needs to be updated.
 */
void
_keyboard_group_update(Elput_Seat *seat)
{
   Elput_Keyboard *kbd;
   xkb_mod_mask_t latched, locked;
   Eina_Bool state;

   kbd = _evdev_keyboard_get(seat);
   if (!kbd) return;

   state = _keyboard_state_update(kbd, kbd->info->keymap.map, &latched, &locked);
   if (!state) return;
   _keyboard_compose_init(kbd);

   _keyboard_modifiers_update(kbd, seat);

   if ((!latched) && (!locked)) return;

   _keyboard_modifiers_send(kbd);
}

/**
 * @brief Retrieves a remapped key code for a given original key code on a specific device.
 *
 * If key remapping is enabled for the device (edev->key_remap is true and
 * edev->key_remap_hash exists), this function looks up the original key code
 * in the hash table. If a mapping is found, the remapped key code is returned.
 * Otherwise, the original key code is returned.
 *
 * @param edev The Elput_Device for which to check for remapped keys.
 * @param code The original key code.
 * @return The remapped key code if a mapping exists, otherwise the original @p code.
 */
static int
_keyboard_remapped_key_get(Elput_Device *edev, int code)
{
   void *ret = NULL;

   if (!edev) return code;
   if (!edev->key_remap) return code;
   if (!edev->key_remap_hash) return code;

   ret = eina_hash_find(edev->key_remap_hash, &code);
   if (ret) code = (int)(intptr_t)ret;
   return code;
}

static int
_keyboard_keysym_translate(xkb_keysym_t keysym, unsigned int modifiers, char *buffer, int bytes)
{
/* this function is copied, with slight changes in variable names, from KeyBind.c in libX11
 * the license from that file can be found below:
 */
/*

Copyright 1985, 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
   if (!keysym) return 0;

   /*
    * This section handles the translation of keysyms when the Control
    * modifier is active. It attempts to produce ASCII control characters
    * (e.g., Ctrl+C -> ETX (0x03)).
    * The logic is based on common terminal behavior and X11's KeyBind.c.
    *
    * It checks if the keysym is a basic Latin character or a specific
    * control-related key (like BackSpace, Return, Escape, KP_Space, etc.).
    * If so, it maps it to the corresponding C0 control code.
    * For example:
    *   - '@' through '_' (and 'a' through 'z') are mapped by `c &= 0x1F`.
    *   - ' ' (space) is also mapped by `c &= 0x1F` to NUL (0x00) if it's KP_Space,
    *     or to itself (0x20) then masked to NUL (0x00).
    *   - Specific numeric keys ('2' through '7', '8', '/') are mapped to
    *     alternative control codes or DEL.
    */
   if (modifiers & ECORE_EVENT_MODIFIER_CTRL)
     {
        Eina_Bool valid_control_code = EINA_TRUE;
        unsigned long hbytes = 0;
        unsigned char c;

        hbytes = (keysym >> 8);
        if (!(bytes &&
        ((hbytes == 0) ||
        ((hbytes == 0xFF) &&
        (((keysym >= XKB_KEY_BackSpace) && (keysym <= XKB_KEY_Clear)) ||
        (keysym == XKB_KEY_Return) ||
        (keysym == XKB_KEY_Escape) ||
        (keysym == XKB_KEY_KP_Space) ||
        (keysym == XKB_KEY_KP_Tab) ||
        (keysym == XKB_KEY_KP_Enter) ||
        ((keysym >= XKB_KEY_KP_Multiply) && (keysym <= XKB_KEY_KP_9)) ||
        (keysym == XKB_KEY_KP_Equal) ||
        (keysym == XKB_KEY_Delete))))))
          return 0;

        if (keysym == XKB_KEY_KP_Space)
          c = (XKB_KEY_space & 0x7F);
        else if (hbytes == 0xFF)
          c = (keysym & 0x7F);
        else
          c = (keysym & 0xFF);

        /* We are building here a control code
           for more details, read:
           https://en.wikipedia.org/wiki/C0_and_C1_control_codes#C0_.28ASCII_and_derivatives.29
         */

        if (((c >= '@') && (c <= '_')) || /* those are the one defined in C0 with capital letters */
             ((c >= 'a') && (c <= 'z')) ||  /* the lowercase symbols (not part of the standard, but useful) */
              c == ' ')
          c &= 0x1F;
        else if (c == '\x7f')
          c = '\177';
        /* following codes are alternatives, they are longer here, i dont want to change them */
        else if (c == '2')
          c = '\000'; /* 0 code */
        else if ((c >= '3') && (c <= '7'))
          c -= ('3' - '\033'); /* from escape to unitseperator code*/
        else if (c == '8')
          c = '\177'; /* delete code */
        else if (c == '/')
          c = '_' & 0x1F; /* unit seperator code */
        else
          valid_control_code = EINA_FALSE;

        if (valid_control_code)
          buffer[0] = c;
        else
          return 0;
     }
   else
     {
        /* if its not a control code, try to produce useful output */
        if (!xkb_keysym_to_utf8(keysym, buffer, bytes))
          return 0;
     }

   return 1;
}

/* from weston/clients/window.c */
/**
 * @brief Processes a key press through the XKB compose sequence state.
 *
 * If a compose sequence is active (i.e., kbd->compose_state exists), this
 * function feeds the given keysym @p sym into the compose state.
 *
 * - If the keysym is accepted and the sequence is still composing,
 *   it returns XKB_KEY_NoSymbol (indicating the key press is consumed by
 *   the compose sequence).
 * - If the keysym completes a compose sequence, it returns the resulting
 *   composed keysym.
 * - If the compose sequence is cancelled or nothing happens, it returns
 *   XKB_KEY_NoSymbol or the original symbol, respectively.
 * - If no compose state exists or the keysym is XKB_KEY_NoSymbol, the
 *   original symbol is returned.
 *
 * @param sym The keysym from the current key press.
 * @param kbd The Elput_Keyboard containing the XKB compose state.
 * @return The resulting keysym after processing by the compose state,
 *         or XKB_KEY_NoSymbol if the key press was consumed by composition,
 *         or the original @p sym if no composition occurred.
 */
static xkb_keysym_t
process_key_press(xkb_keysym_t sym, Elput_Keyboard *kbd)
{
   if (!kbd->compose_state)
     return sym;
   if (sym == XKB_KEY_NoSymbol)
     return sym;
   if (xkb_compose_state_feed(kbd->compose_state, sym) != XKB_COMPOSE_FEED_ACCEPTED)
     return sym;

   switch (xkb_compose_state_get_status(kbd->compose_state))
     {
      case XKB_COMPOSE_COMPOSING:
        return XKB_KEY_NoSymbol;
      case XKB_COMPOSE_COMPOSED:
        return xkb_compose_state_get_one_sym(kbd->compose_state);
      case XKB_COMPOSE_CANCELLED:
        return XKB_KEY_NoSymbol;
      case XKB_COMPOSE_NOTHING:
      default: break;
     }
   return sym;
}

/**
 * Retrieve the string name ('q', 'bracketleft', etc.) for a given key symbol.
 */
static void
_elput_symbol_rep_find(xkb_keysym_t keysym, char *buffer, int size, unsigned int code)
{
    if (xkb_keysym_get_name(keysym, buffer, size) != 0)
      return;

    snprintf(buffer, size, "Keycode-%u", code);
}

/**
 * Handle keyboard events emitted by libinput.
 *
 * Processes a single key pressed / released event from libinput.  The
 * key code will be remapped to another code if one has been registered
 * (see elput_input_key_remap_enable()), and then XKB notified of the
 * keyboard status change.  XKB translates the key code into the
 * appropriate symbol for the current keyboard layout.  Compose
 * translation is performed, if appropriate.  An up or down event is
 * then generated for the processed key via _keyboard_key_send().
 */
static void
_keyboard_key(struct libinput_device *idevice, struct libinput_event_keyboard *event)
{
   Elput_Device *dev;
   Elput_Keyboard *kbd;
   enum libinput_key_state state;
   xkb_keysym_t sym_name, sym = XKB_KEY_NoSymbol;
   const xkb_keysym_t *syms;
   unsigned int code = 0;
   unsigned int nsyms;
   unsigned int timestamp;
   char key[256] = {0}, keyname[256] = {0}, compose[256] = {0};
   int count;

   dev = libinput_device_get_user_data(idevice);
   if (!dev) return;

   kbd = _evdev_keyboard_get(dev->seat);
   if (!kbd) return;

   /* Retrieve details about the event from libinput */
   state = libinput_event_keyboard_get_key_state(event);
   kbd->key_count = count = libinput_event_keyboard_get_seat_key_count(event);

   /* Ignore key events that are not seat wide state changes. */
   if (((state == LIBINPUT_KEY_STATE_PRESSED) && (count != 1)) ||
       ((state == LIBINPUT_KEY_STATE_RELEASED) && (count != 0)))
     return;

   /* Retrieve the code and remap it if a remap for it has been registered */
   code = libinput_event_keyboard_get_key(event);
   code = _keyboard_remapped_key_get(dev, code) + 8;

   timestamp = libinput_event_keyboard_get_time(event);

   /* Update the XKB keyboard state for the key that was pressed or released */
   if (state == LIBINPUT_KEY_STATE_PRESSED)
     xkb_state_update_key(kbd->state, code, XKB_KEY_DOWN);
   else
     xkb_state_update_key(kbd->state, code, XKB_KEY_UP);

   /* Apply the current keyboard state to translate the code for the key
    * that was struck into its effective symbol (after applying
    * modifiers like CAPSLOCK and so on).  We also use the maskless
    * keyboard state to lookup the underlying symbol name (i.e. without
    * applying modifiers).
    */
   nsyms = xkb_key_get_syms(kbd->state, code, &syms);
   if (nsyms == 1) sym = syms[0];
   sym_name = xkb_state_key_get_one_sym(kbd->maskless_state, code);

   if (state == LIBINPUT_KEY_STATE_PRESSED)
     sym = process_key_press(sym, kbd);

   /* Lookup the textual name ('q', 'space', 'bracketleft', etc.) of the
    * symbol and of the symbol name.
    */
   _elput_symbol_rep_find(sym, key, sizeof(key), code);
   _elput_symbol_rep_find(sym_name, keyname, sizeof(keyname), code);

   /* If no keyname was found, name it "Keycode-NNN" */
   if (keyname[0] == '\0')
     snprintf(keyname, sizeof(keyname), "Keycode-%u", code);

   /* If Shift key is active, downcase the keyname's first letter */
   if (xkb_state_mod_index_is_active(kbd->state, kbd->info->mods.shift,
                                     XKB_STATE_MODS_EFFECTIVE))
     {
        if (keyname[0] != '\0')
          keyname[0] = tolower(keyname[0]);
     }

   /* Update the seat's modifiers to match what's active in the kbd */
   _keyboard_modifiers_update(kbd, dev->seat);

   /* Translate the key symbol into a printable character in Unicode (UTF-8) format */
   _keyboard_keysym_translate(sym, dev->seat->modifiers, compose, sizeof(compose));

   /* Issue the appropriate key up or down event with all related key data */
   _keyboard_key_send(dev, state, keyname, key, compose, code, timestamp);

   if ((kbd->pending_keymap) && (count == 0))
     _keyboard_keymap_update(dev->seat);

   if (state == LIBINPUT_KEY_STATE_PRESSED)
     {
        kbd->grab.key = code;
        kbd->grab.timestamp = timestamp;
     }
}

/**
 * @brief Allocates and initializes a basic Elput_Pointer structure.
 *
 * This function allocates memory for an Elput_Pointer structure, associates
 * it with the given seat, and sets a default mouse click threshold.
 * Further initialization related to device capabilities is done elsewhere.
 *
 * @param seat The Elput_Seat to which this pointer will belong.
 * @return A pointer to the newly allocated Elput_Pointer structure,
 *         or NULL on allocation failure.
 */
static Elput_Pointer *
_pointer_create(Elput_Seat *seat)
{
   Elput_Pointer *ptr;

   ptr = calloc(1, sizeof(Elput_Pointer));
   if (!ptr) return NULL;

   ptr->seat = seat;
   ptr->mouse.threshold = 250;

   return ptr;
}

/**
 * @brief Initializes pointer capabilities for a seat.
 *
 * If the seat does not already have an active pointer (seat->ptr is NULL),
 * this function creates a new Elput_Pointer structure for it using
 * _pointer_create, sets its initial pressure to 1.0, and increments the
 * seat's pointer device count.
 * If a pointer already exists, it simply increments the count.
 * In either case where the count becomes 1 (i.e., a pointer becomes active
 * for the first time or reactivated), it calls _seat_caps_update to notify
 * about the change in seat capabilities.
 *
 * @param seat The Elput_Seat for which to initialize pointer capabilities.
 * @return EINA_TRUE if initialization was successful (or if a pointer already existed),
 *         EINA_FALSE if creating a new pointer failed.
 */
static Eina_Bool
_pointer_init(Elput_Seat *seat)
{
   Elput_Pointer *ptr;

   if (seat->ptr)
     {
        seat->count.ptr += 1;
        if (seat->count.ptr == 1)
          _seat_caps_update(seat);
        return EINA_TRUE;
     }

   ptr = _pointer_create(seat);
   if (!ptr) return EINA_FALSE;

   seat->ptr = ptr;
   seat->count.ptr = 1;
   ptr->pressure = 1.0;

   _seat_caps_update(seat);

   return EINA_TRUE;
}

/**
 * @brief Decrements the pointer device count for a seat.
 *
 * This function is called when a pointer device is removed or disassociated
 * from a seat. It decrements the seat's pointer counter. If the count
 * drops to zero, it means there are no more active pointer devices on the seat.
 * In this case, it resets the button state of the seat's pointer object and
 * calls _seat_caps_update to notify about the change in seat capabilities.
 *
 * @param seat The Elput_Seat from which a pointer device is being released.
 */
static void
_pointer_release(Elput_Seat *seat)
{
   seat->count.ptr--;
   if (seat->count.ptr == 0)
     {
        seat->ptr->buttons = 0;
        _seat_caps_update(seat);
     }
}

/**
 * @brief Allocates and initializes a basic Elput_Touch structure.
 *
 * This function allocates memory for an Elput_Touch structure, associates
 * it with the given seat, and sets a default pressure value.
 * Further initialization related to device capabilities is done elsewhere.
 *
 * @param seat The Elput_Seat to which this touch device will belong.
 * @return A pointer to the newly allocated Elput_Touch structure,
 *         or NULL on allocation failure.
 */
static Elput_Touch *
_touch_create(Elput_Seat *seat)
{
   Elput_Touch *touch;

   touch = calloc(1, sizeof(Elput_Touch));
   if (!touch) return NULL;

   touch->seat = seat;
   touch->pressure = 1.0;

   return touch;
}

/**
 * @brief Initializes touch capabilities for a seat.
 *
 * If the seat does not already have an active touch device (seat->touch is NULL),
 * this function creates a new Elput_Touch structure for it using _touch_create
 * and increments the seat's touch device count.
 * If a touch device already exists, it simply increments the count.
 * In either case where the count becomes 1 (i.e., a touch device becomes active
 * for the first time or reactivated), it calls _seat_caps_update to notify
 * about the change in seat capabilities.
 *
 * @param seat The Elput_Seat for which to initialize touch capabilities.
 * @return EINA_TRUE if initialization was successful (or if a touch device already existed),
 *         EINA_FALSE if creating a new touch device failed.
 */
static Eina_Bool
_touch_init(Elput_Seat *seat)
{
   Elput_Touch *touch;

   if (seat->touch)
     {
        seat->count.touch += 1;
        if (seat->count.touch == 1)
          _seat_caps_update(seat);
        return EINA_TRUE;
     }

   touch = _touch_create(seat);
   if (!touch) return EINA_FALSE;

   seat->touch = touch;
   seat->count.touch = 1;

   _seat_caps_update(seat);

   return EINA_TRUE;
}

/**
 * @brief Decrements the touch device count for a seat.
 *
 * This function is called when a touch device is removed or disassociated
 * from a seat. It decrements the seat's touch counter. If the count
 * drops to zero, it means there are no more active touch devices on the seat.
 * In this case, it resets the touch points count of the seat's touch object
 * and calls _seat_caps_update to notify about the change in seat capabilities.
 *
 * @param seat The Elput_Seat from which a touch device is being released.
 */
static void
_touch_release(Elput_Seat *seat)
{
   seat->count.touch--;
   if (seat->count.touch == 0)
     {
        seat->touch->points = 0;
        _seat_caps_update(seat);
     }
}

/**
 * @brief Sends an ECORE_EVENT_MOUSE_MOVE event based on the current pointer state.
 *
 * This function retrieves the pointer and keyboard state for the device's seat.
 * It clamps the pointer coordinates to the configured output dimensions.
 * It then allocates and populates an Ecore_Event_Mouse_Move event with the
 * current pointer position, timestamp, modifiers, and multi-touch information
 * (if applicable from a touch device on the same seat). The event is then
 * added to the ecore event queue.
 *
 * @param edev The Elput_Device that generated the motion.
 */
static void
_pointer_motion_send(Elput_Device *edev)
{
   Elput_Pointer *ptr;
   Elput_Keyboard *kbd;
   Elput_Touch *touch;
   Ecore_Event_Mouse_Move *ev;
   double x, y;

   ptr = _evdev_pointer_get(edev->seat);
   if (!ptr) return;

   ev = calloc(1, sizeof(Ecore_Event_Mouse_Move));
   if (!ev) return;
   edev->seat->pending_motion = 0;

   x = ptr->seat->pointer.x;
   y = ptr->seat->pointer.y;

   if (x < ptr->minx)
     x = ptr->minx;
   else if (x >= ptr->minx + ptr->seat->manager->input.pointer_w)
     x = ptr->minx + ptr->seat->manager->input.pointer_w - 1;

   if (y < ptr->miny)
     y = ptr->miny;
   else if (y >= ptr->miny + ptr->seat->manager->input.pointer_h)
     y = ptr->miny + ptr->seat->manager->input.pointer_h - 1;

   ptr->seat->pointer.x = x;
   ptr->seat->pointer.y = y;

   ev->window = edev->seat->manager->window;
   ev->event_window = edev->seat->manager->window;
   ev->root_window = edev->seat->manager->window;
   ev->timestamp = ptr->timestamp;
   ev->same_screen = 1;
   ev->dev = edev->evas_device;
   if (ev->dev) efl_ref(ev->dev);

   ev->x = ptr->seat->pointer.x;
   ev->y = ptr->seat->pointer.y;
   ev->root.x = ptr->seat->pointer.x;
   ev->root.y = ptr->seat->pointer.y;

   kbd = _evdev_keyboard_get(edev->seat);
   if (kbd) _keyboard_modifiers_update(kbd, edev->seat);

   ev->modifiers = edev->seat->modifiers;

   touch = _evdev_touch_get(edev->seat);
   if (touch) ev->multi.device = touch->slot;

   ev->multi.radius = 1;
   ev->multi.radius_x = 1;
   ev->multi.radius_y = 1;
   ev->multi.pressure = ptr->pressure;
   ev->multi.angle = 0.0;
   ev->multi.x = ptr->seat->pointer.x;
   ev->multi.y = ptr->seat->pointer.y;
   ev->multi.root.x = ptr->seat->pointer.x;
   ev->multi.root.y = ptr->seat->pointer.y;

   ecore_event_add(ECORE_EVENT_MOUSE_MOVE, ev, _event_free, ev->dev);
}

/**
 * @brief Sends an ELPUT_EVENT_POINTER_MOTION event with explicitly provided deltas.
 *
 * This function is used to report relative motion when the deltas are calculated
 * externally (e.g., from absolute motion events). It creates and populates an
 * Elput_Event_Pointer_Motion event with the given deltas and the timestamp
 * from the original libinput event. Both accelerated and unaccelerated deltas
 * are set to the same provided values.
 *
 * @param event The original libinput pointer event, used for timestamp.
 * @param dx The relative change in the X-coordinate.
 * @param dy The relative change in the Y-coordinate.
 */
static void
_pointer_motion_relative_fake(struct libinput_event_pointer *event, double dx, double dy)
{
   Elput_Event_Pointer_Motion *ev;

   ev = calloc(1, sizeof(Elput_Event_Pointer_Motion));
   EINA_SAFETY_ON_NULL_RETURN(ev);

   ev->time_usec = libinput_event_pointer_get_time_usec(event);
   ev->dx = dx;
   ev->dy = dy;
   ev->dx_unaccel = dx;
   ev->dy_unaccel = dy;

   ecore_event_add(ELPUT_EVENT_POINTER_MOTION, ev, NULL, NULL);
}

/**
 * @brief Sends an ELPUT_EVENT_POINTER_MOTION event based on a libinput relative motion event.
 *
 * This function extracts the timestamp, accelerated deltas (dx, dy), and
 * unaccelerated deltas (dx_unaccel, dy_unaccel) from the libinput pointer
 * event. It then creates and populates an Elput_Event_Pointer_Motion event
 * with this information and adds it to the ecore event queue. This event
 * provides raw motion data, separate from the ECORE_EVENT_MOUSE_MOVE.
 *
 * @param event The libinput pointer event containing relative motion data.
 */
static void
_pointer_motion_relative(struct libinput_event_pointer *event)
{
   Elput_Event_Pointer_Motion *ev;

   ev = calloc(1, sizeof(Elput_Event_Pointer_Motion));
   EINA_SAFETY_ON_NULL_RETURN(ev);

   ev->time_usec = libinput_event_pointer_get_time_usec(event);
   ev->dx = libinput_event_pointer_get_dx(event);
   ev->dy = libinput_event_pointer_get_dy(event);
   ev->dx_unaccel = libinput_event_pointer_get_dx_unaccelerated(event);
   ev->dy_unaccel = libinput_event_pointer_get_dy_unaccelerated(event);

   ecore_event_add(ELPUT_EVENT_POINTER_MOTION, ev, NULL, NULL);
}

/**
 * @brief Handles relative pointer motion events from libinput.
 *
 * This function processes a LIBINPUT_EVENT_POINTER_MOTION event.
 * It retrieves the Elput_Device and Elput_Pointer associated with the event.
 * It applies device-specific transformations (swap axes, invert X/Y) to the
 * deltas obtained from libinput. The seat's pointer coordinates are updated
 * with these transformed deltas.
 * Finally, it calls _pointer_motion_send to generate an ECORE_EVENT_MOUSE_MOVE
 * and _pointer_motion_relative to generate an ELPUT_EVENT_POINTER_MOTION.
 *
 * @param idev The libinput device that generated the event.
 * @param event The libinput pointer event data for relative motion.
 * @return EINA_TRUE if the event was processed successfully, EINA_FALSE otherwise
 *         (e.g., if the device or pointer could not be retrieved).
 */
static Eina_Bool
_pointer_motion(struct libinput_device *idev, struct libinput_event_pointer *event)
{
   Elput_Device *edev;
   Elput_Pointer *ptr;
   double dx, dy, tmp;

   edev = libinput_device_get_user_data(idev);
   if (!edev) return EINA_FALSE;

   ptr = _evdev_pointer_get(edev->seat);
   if (!ptr) return EINA_FALSE;

   dx = libinput_event_pointer_get_dx(event);
   dy = libinput_event_pointer_get_dy(event);

   if (edev->swap)
     {
        tmp = dx;
        dx = dy;
        dy = tmp;
     }
   if (edev->invert_x) dx *= -1;
   if (edev->invert_y) dy *= -1;

   ptr->seat->pointer.x += dx;
   ptr->seat->pointer.y += dy;
   ptr->timestamp = libinput_event_pointer_get_time(event);

   _pointer_motion_send(edev);
   _pointer_motion_relative(event);

   return EINA_TRUE;
}

/**
 * @brief Handles absolute pointer motion events from libinput.
 *
 * This function processes a LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE event.
 * It retrieves the Elput_Device and Elput_Pointer. It gets the transformed
 * absolute X and Y coordinates from the libinput event (scaled to the
 * device's output width/height: edev->ow, edev->oh).
 * These absolute coordinates update the device's internal absolute position
 * (edev->absx, edev->absy) and the seat's pointer position.
 * It then calls _pointer_motion_send to generate an ECORE_EVENT_MOUSE_MOVE
 * and _pointer_motion_relative_fake to generate an ELPUT_EVENT_POINTER_MOTION
 * using the difference between the new and old absolute positions as deltas.
 *
 * @param idev The libinput device that generated the event.
 * @param event The libinput pointer event data for absolute motion.
 * @return EINA_TRUE if the event was processed successfully, EINA_FALSE otherwise.
 */
static Eina_Bool
_pointer_motion_abs(struct libinput_device *idev, struct libinput_event_pointer *event)
{
   Elput_Device *edev;
   Elput_Pointer *ptr;
   double x, y;

   edev = libinput_device_get_user_data(idev);
   if (!edev) return EINA_FALSE;

   ptr = _evdev_pointer_get(edev->seat);
   if (!ptr) return EINA_FALSE;

   x = edev->absx;
   edev->absx =
     libinput_event_pointer_get_absolute_x_transformed(event, edev->ow);
   ptr->seat->pointer.x = edev->absx;

   y = edev->absy;
   edev->absy =
     libinput_event_pointer_get_absolute_y_transformed(event, edev->oh);
   ptr->seat->pointer.y = edev->absy;

   ptr->timestamp = libinput_event_pointer_get_time(event);

   /* TODO: these needs to run a matrix transform based on output */

   _pointer_motion_send(edev);
   _pointer_motion_relative_fake(event, edev->absx - x, edev->absy - y);

   return EINA_TRUE;
}

/**
 * @brief Sends an ECORE_EVENT_MOUSE_BUTTON_DOWN or ECORE_EVENT_MOUSE_BUTTON_UP event.
 *
 * This function constructs and sends a mouse button event based on the current
 * state of the pointer and the provided button state (pressed or released).
 * It populates the event with coordinates, timestamp, button number,
 * click status (double/triple), modifiers, and multi-touch information.
 *
 * @param edev The Elput_Device that generated the button event.
 * @param state The state of the button (LIBINPUT_BUTTON_STATE_PRESSED or
 *              LIBINPUT_BUTTON_STATE_RELEASED), though this function uses
 *              its boolean interpretation (1 for pressed, 0 for released)
 *              to determine event type.
 */
static void
_pointer_button_send(Elput_Device *edev, enum libinput_button_state state)
{
   Elput_Pointer *ptr;
   Elput_Keyboard *kbd;
   Elput_Touch *touch;
   Ecore_Event_Mouse_Button *ev;

   ptr = _evdev_pointer_get(edev->seat);
   if (!ptr) return;

   ev = calloc(1, sizeof(Ecore_Event_Mouse_Button));
   if (!ev) return;

   ev->window = edev->seat->manager->window;
   ev->event_window = edev->seat->manager->window;
   ev->root_window = edev->seat->manager->window;
   ev->timestamp = ptr->timestamp;
   ev->same_screen = 1;
   ev->dev = edev->evas_device;
   if (ev->dev) efl_ref(ev->dev);

   ev->x = ptr->seat->pointer.x;
   ev->y = ptr->seat->pointer.y;
   ev->root.x = ptr->seat->pointer.x;
   ev->root.y = ptr->seat->pointer.y;

   touch = _evdev_touch_get(edev->seat);
   if (touch) ev->multi.device = touch->slot;
   ev->multi.radius = 1;
   ev->multi.radius_x = 1;
   ev->multi.radius_y = 1;
   ev->multi.pressure = ptr->pressure;
   ev->multi.angle = 0.0;
   ev->multi.x = ptr->seat->pointer.x;
   ev->multi.y = ptr->seat->pointer.y;
   ev->multi.root.x = ptr->seat->pointer.x;
   ev->multi.root.y = ptr->seat->pointer.y;

   ev->buttons = ptr->buttons;

   ev->double_click = ptr->mouse.double_click;
   ev->triple_click = ptr->mouse.triple_click;

   kbd = _evdev_keyboard_get(edev->seat);
   if (kbd)
     _keyboard_modifiers_update(kbd, edev->seat);
   ev->modifiers = edev->seat->modifiers;

   if (state)
     ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, ev, _event_free, ev->dev);
   else
     ecore_event_add(ECORE_EVENT_MOUSE_BUTTON_UP, ev, _event_free, ev->dev);
}

/**
 * @brief Updates double and triple click detection state for a pointer.
 *
 * This function checks if the current button press (@p btn) constitutes a
 * double or triple click based on the time elapsed since the previous
 * press(es) and whether the same button was involved. It updates the
 * `ptr->mouse.double_click` and `ptr->mouse.triple_click` flags accordingly.
 * It also updates the history of previous button presses and timestamps.
 *
 * @param ptr The Elput_Pointer whose click state is to be updated.
 * @param btn The button number that was just pressed.
 */
static void
_pointer_click_update(Elput_Pointer *ptr, unsigned int btn)
{
   unsigned int current;

   current = ptr->timestamp;
   ptr->mouse.double_click = EINA_FALSE;
   ptr->mouse.triple_click = EINA_FALSE;

   if (((current - ptr->mouse.prev_time) <= ptr->mouse.threshold) &&
       (btn == ptr->mouse.prev_button))
     {
        ptr->mouse.double_click = EINA_TRUE;
        if (((current - ptr->mouse.last_time) <=
             (2 * ptr->mouse.threshold)) &&
            (btn == ptr->mouse.last_button))
          {
             ptr->mouse.triple_click = EINA_TRUE;
             ptr->mouse.prev_time = 0;
             ptr->mouse.last_time = 0;
             current = 0;
          }
     }

   ptr->mouse.last_time = ptr->mouse.prev_time;
   ptr->mouse.prev_time = current;
   ptr->mouse.last_button = ptr->mouse.prev_button;
   ptr->mouse.prev_button = ptr->buttons;
}

/**
 * @brief Handles pointer button events from libinput.
 *
 * This function processes a LIBINPUT_EVENT_POINTER_BUTTON event.
 * It retrieves the Elput_Device and Elput_Pointer. It checks if the event
 * represents a seat-wide state change (i.e., the first press or last release
 * of a button on the seat).
 * It translates the libinput button code (e.g., BTN_MIDDLE to 2, BTN_RIGHT to 3,
 * effectively swapping them from typical kernel/libinput to X11-like numbering).
 * If the button is being pressed, it calls _pointer_click_update to detect
 * double/triple clicks. If there's a pending motion event for the seat,
 * it's sent first. Finally, _pointer_button_send is called to generate
 * the Ecore mouse button event.
 *
 * @param idev The libinput device that generated the event.
 * @param event The libinput pointer event data for button press/release.
 * @return EINA_TRUE if the event was processed successfully, EINA_FALSE otherwise.
 */
static Eina_Bool
_pointer_button(struct libinput_device *idev, struct libinput_event_pointer *event)
{
   Elput_Device *edev;
   Elput_Pointer *ptr;
   int count;
   enum libinput_button_state state;
   unsigned int btn;

   edev = libinput_device_get_user_data(idev);
   if (!edev) return EINA_FALSE;

   ptr = _evdev_pointer_get(edev->seat);
   if (!ptr) return EINA_FALSE;

   state = libinput_event_pointer_get_button_state(event);
   count = libinput_event_pointer_get_seat_button_count(event);

   /* Ignore button events that are not seat wide state changes. */
   if (((state == LIBINPUT_BUTTON_STATE_PRESSED) && (count != 1)) ||
       ((state == LIBINPUT_BUTTON_STATE_RELEASED) && (count != 0)))
     return EINA_FALSE;

   btn = libinput_event_pointer_get_button(event);

   btn = ((btn & 0x00F) + 1);
   if (btn == 3) btn = 2;
   else if (btn == 2) btn = 3;

   ptr->buttons = btn;
   ptr->timestamp = libinput_event_pointer_get_time(event);

   if (state) _pointer_click_update(ptr, btn);
   if (edev->seat->pending_motion) _pointer_motion_send(edev);

   _pointer_button_send(edev, state);

   return EINA_TRUE;
}

/**
 * @brief Sends an ECORE_EVENT_MOUSE_WHEEL event.
 *
 * This function constructs and sends a mouse wheel (scroll) event.
 * It populates the event with the current pointer coordinates, timestamp,
 * scroll direction (0 for vertical, 1 for horizontal), scroll value (amount),
 * and current keyboard modifiers.
 *
 * @param dev The Elput_Device that generated the axis event.
 * @param direction The direction of the scroll: 0 for vertical, 1 for horizontal.
 * @param value The discrete value of the scroll (e.g., number of wheel ticks).
 */
static void
_pointer_axis_send(Elput_Device *dev, int direction, int value)
{
   Elput_Pointer *ptr;
   Elput_Keyboard *kbd;
   Ecore_Event_Mouse_Wheel *ev;

   ptr = _evdev_pointer_get(dev->seat);
   if (!ptr) return;

   ev = calloc(1, sizeof(Ecore_Event_Mouse_Wheel));
   if (!ev) return;

   ev->window = dev->seat->manager->window;
   ev->event_window = dev->seat->manager->window;
   ev->root_window = dev->seat->manager->window;
   ev->timestamp = ptr->timestamp;
   ev->same_screen = 1;
   ev->dev = dev->evas_device;
   if (ev->dev) efl_ref(ev->dev);

   ev->x = ptr->seat->pointer.x;
   ev->y = ptr->seat->pointer.y;
   ev->root.x = ptr->seat->pointer.x;
   ev->root.y = ptr->seat->pointer.y;

   ev->z = value;
   ev->direction = direction;

   kbd = _evdev_keyboard_get(dev->seat);
   if (kbd) _keyboard_modifiers_update(kbd, dev->seat);

   ev->modifiers = dev->seat->modifiers;

   ecore_event_add(ECORE_EVENT_MOUSE_WHEEL, ev, _event_free, ev->dev);
}

/**
 * @brief Retrieves the axis value from a libinput pointer event, considering the source.
 *
 * Libinput provides axis values differently depending on the source:
 * - LIBINPUT_POINTER_AXIS_SOURCE_WHEEL: Discrete values (e.g., wheel ticks).
 * - LIBINPUT_POINTER_AXIS_SOURCE_FINGER, LIBINPUT_POINTER_AXIS_SOURCE_CONTINUOUS:
 *   Continuous values (e.g., from touchpad scrolling).
 * This function calls the appropriate libinput getter based on the source.
 *
 * @param event The libinput pointer event containing axis data.
 * @param axis The specific axis (e.g., LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL)
 *             for which to get the value.
 * @return The axis value, interpreted as discrete or continuous based on source.
 *         Returns 0.0 if the source is unknown or not handled.
 */
static double
_pointer_axis_value(struct libinput_event_pointer *event, enum libinput_pointer_axis axis)
{
   enum libinput_pointer_axis_source source;
   double val = 0.0;

   source = libinput_event_pointer_get_axis_source(event);
   switch (source)
     {
      case LIBINPUT_POINTER_AXIS_SOURCE_WHEEL:
        val = libinput_event_pointer_get_axis_value_discrete(event, axis);
        break;
      case LIBINPUT_POINTER_AXIS_SOURCE_FINGER:
      case LIBINPUT_POINTER_AXIS_SOURCE_CONTINUOUS:
        val = libinput_event_pointer_get_axis_value(event, axis);
        break;
      default:
        break;
     }

   return val;
}

/**
 * @brief Handles pointer axis (scroll) events from libinput.
 *
 * This function processes a LIBINPUT_EVENT_POINTER_AXIS event.
 * It checks if the event has vertical or horizontal scroll data.
 * For each present axis, it retrieves the scroll value using
 * _pointer_axis_value and determines the direction (0 for vertical,
 * 1 for horizontal). The pointer's timestamp is updated, and then
 * _pointer_axis_send is called to generate an ECORE_EVENT_MOUSE_WHEEL.
 * If both vertical and horizontal data are present, horizontal typically
 * takes precedence for the `val` and `dir` sent to _pointer_axis_send
 * due to the order of checks.
 *
 * @param idevice The libinput device that generated the event.
 * @param event The libinput pointer event data for axis scroll.
 * @return EINA_TRUE if a scroll event was processed and sent, EINA_FALSE otherwise
 *         (e.g., if no axis data was found or device/pointer retrieval failed).
 */
static Eina_Bool
_pointer_axis(struct libinput_device *idevice, struct libinput_event_pointer *event)
{
   Elput_Device *dev;
   Elput_Pointer *ptr;
   enum libinput_pointer_axis axis;
   Eina_Bool vert = EINA_FALSE, horiz = EINA_FALSE;
   int dir = 0, val = 0;

   dev = libinput_device_get_user_data(idevice);
   if (!dev) return EINA_FALSE;

   ptr = _evdev_pointer_get(dev->seat);
   if (!ptr) return EINA_FALSE;

   vert =
     libinput_event_pointer_has_axis(event,
                                     LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
   horiz =
     libinput_event_pointer_has_axis(event,
                                     LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL);
   if ((!vert) && (!horiz)) return EINA_FALSE;

   if (vert)
     {
        axis = LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL;
        val = _pointer_axis_value(event, axis);
     }

   if (horiz)
     {
        axis = LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL;
        val = _pointer_axis_value(event, axis);
        dir = 1;
     }

   ptr->timestamp = libinput_event_pointer_get_time(event);

   _pointer_axis_send(dev, dir, val);

   return EINA_TRUE;
}

/**
 * @brief Sends a touch-related event, masquerading as a mouse button event.
 *
 * This function is used to send ECORE_EVENT_MOUSE_BUTTON_DOWN or
 * ECORE_EVENT_MOUSE_BUTTON_UP events for touch interactions.
 * It populates the event with the current touch coordinates (from the seat's
 * pointer, which is updated by touch events), timestamp, modifiers, and
 * multi-touch specific information like slot ID and pressure.
 * The button number is hardcoded to 1 (left mouse button equivalent).
 *
 * @param dev The Elput_Device that generated the touch event.
 * @param type The Ecore event type to send (ECORE_EVENT_MOUSE_BUTTON_DOWN or
 *             ECORE_EVENT_MOUSE_BUTTON_UP).
 */
static void
_touch_event_send(Elput_Device *dev, int type)
{
   Elput_Touch *touch;
   Ecore_Event_Mouse_Button *ev;
   unsigned int btn = 0;

   touch = _evdev_touch_get(dev->seat);
   if (!touch) return;

   ev = calloc(1, sizeof(Ecore_Event_Mouse_Button));
   if (!ev) return;

   ev->window = dev->seat->manager->window;
   ev->event_window = dev->seat->manager->window;
   ev->root_window = dev->seat->manager->window;
   ev->timestamp = touch->timestamp;
   ev->same_screen = 1;

   ev->x = touch->seat->pointer.x;
   ev->y = touch->seat->pointer.y;
   ev->root.x = touch->seat->pointer.x;
   ev->root.y = touch->seat->pointer.y;

   ev->modifiers = dev->seat->modifiers;

   ev->multi.device = touch->slot;
   ev->multi.radius = 1;
   ev->multi.radius_x = 1;
   ev->multi.radius_y = 1;
   ev->multi.pressure = touch->pressure;
   ev->multi.angle = 0.0;
   ev->multi.x = ev->x;
   ev->multi.y = ev->y;
   ev->multi.root.x = ev->x;
   ev->multi.root.y = ev->y;

   btn = ((btn & 0x00F) + 1);
// XXX: this code is useless. above btn is set to 0 at declaration time, then
// no code changes it until the above like effectively makes it 1. it can
// only ever be 1 so the below lines are pointless. this is probably a bug
// lurking...
//   if (btn == 3) btn = 2;
//   else if (btn == 2) btn = 3;
   ev->buttons = btn;

   ecore_event_add(type, ev, NULL, NULL);
}

/**
 * @brief Sends a touch motion event, masquerading as a mouse move event.
 *
 * This function constructs and sends an ECORE_EVENT_MOUSE_MOVE event for
 * touch motion. It populates the event with the current touch coordinates
 * (rounded, from the seat's pointer), timestamp, modifiers, and multi-touch
 * specific information like slot ID and pressure.
 *
 * @param dev The Elput_Device that generated the touch motion.
 */
static void
_touch_motion_send(Elput_Device *dev)
{
   Elput_Touch *touch;
   Ecore_Event_Mouse_Move *ev;

   touch = _evdev_touch_get(dev->seat);
   if (!touch) return;

   ev = calloc(1, sizeof(Ecore_Event_Mouse_Move));
   if (!ev) return;

   ev->window = dev->seat->manager->window;
   ev->event_window = dev->seat->manager->window;
   ev->root_window = dev->seat->manager->window;
   ev->timestamp = touch->timestamp;
   ev->same_screen = 1;
   ev->dev = dev->evas_device;
   if (ev->dev) efl_ref(ev->dev);

   ev->x = lround(touch->seat->pointer.x);
   ev->y = lround(touch->seat->pointer.y);
   ev->root.x = ev->x;
   ev->root.y = ev->y;

   ev->modifiers = dev->seat->modifiers;

   ev->multi.device = touch->slot;
   ev->multi.radius = 1;
   ev->multi.radius_x = 1;
   ev->multi.radius_y = 1;
   ev->multi.pressure = touch->pressure;
   ev->multi.angle = 0.0;
   ev->multi.x = touch->seat->pointer.x;
   ev->multi.y = touch->seat->pointer.y;
   ev->multi.root.x = touch->seat->pointer.x;
   ev->multi.root.y = touch->seat->pointer.y;

   ecore_event_add(ECORE_EVENT_MOUSE_MOVE, ev, _event_free, ev->dev);
}

/**
 * @brief Handles touch down events from libinput.
 *
 * This function processes a LIBINPUT_EVENT_TOUCH_DOWN event.
 * It retrieves the Elput_Device and Elput_Touch structures.
 * It updates the touch slot, timestamp, and the seat's pointer coordinates
 * based on the transformed touch coordinates from the event (scaled to
 * device's output dimensions: dev->ow, dev->oh).
 * If this touch down corresponds to the "grabbed" slot (primary touch point),
 * its grab coordinates are updated. The number of active touch points is
 * incremented.
 * A touch motion event is sent via _touch_motion_send, followed by a
 * touch down event (as mouse button down) via _touch_event_send.
 * If this is the first touch point (points == 1), it establishes this slot
 * as the grabbed slot for gesture tracking or primary interaction.
 *
 * @param idevice The libinput device that generated the event.
 * @param event The libinput touch event data for touch down.
 */
static void
_touch_down(struct libinput_device *idevice, struct libinput_event_touch *event)
{
   Elput_Device *dev;
   Elput_Touch *touch;

   dev = libinput_device_get_user_data(idevice);
   if (!dev) return;

   touch = _evdev_touch_get(dev->seat);
   if (!touch) return;

   touch->slot = libinput_event_touch_get_slot(event);
   touch->timestamp = libinput_event_touch_get_time(event);

   touch->seat->pointer.x =
     libinput_event_touch_get_x_transformed(event, dev->ow);
   touch->seat->pointer.y =
     libinput_event_touch_get_y_transformed(event, dev->oh);

   /* TODO: these needs to run a matrix transform based on output */
   /* _ecore_drm2_output_coordinate_transform(dev->output, */
   /*                                         touch->seat->pointer.x, touch->seat->pointer.y, */
   /*                                         &touch->seat->pointer.x, &touch->seat->pointer.y); */

   if (touch->slot == touch->grab.id)
     {
        touch->grab.x = touch->seat->pointer.x;
        touch->grab.y = touch->seat->pointer.y;
     }

   touch->points++;

   _touch_motion_send(dev);
   _touch_event_send(dev, ECORE_EVENT_MOUSE_BUTTON_DOWN);

   if (touch->points == 1)
     {
        touch->grab.id = touch->slot;
        touch->grab.x = touch->seat->pointer.x;
        touch->grab.y = touch->seat->pointer.y;
        touch->grab.timestamp = touch->timestamp;
     }
}

/**
 * @brief Handles touch up events from libinput.
 *
 * This function processes a LIBINPUT_EVENT_TOUCH_UP event.
 * It retrieves the Elput_Device and Elput_Touch structures.
 * The number of active touch points is decremented. The touch slot and
 * timestamp are updated from the event.
 * A touch motion event is sent via _touch_motion_send (to update to the
 * final position before lift-off), followed by a touch up event (as mouse
 * button up) via _touch_event_send.
 *
 * @param idevice The libinput device that generated the event.
 * @param event The libinput touch event data for touch up.
 */
static void
_touch_up(struct libinput_device *idevice, struct libinput_event_touch *event)
{
   Elput_Device *dev;
   Elput_Touch *touch;

   dev = libinput_device_get_user_data(idevice);
   if (!dev) return;

   touch = _evdev_touch_get(dev->seat);
   if (!touch) return;

   touch->points--;
   touch->slot = libinput_event_touch_get_slot(event);
   touch->timestamp = libinput_event_touch_get_time(event);

   _touch_motion_send(dev);
   _touch_event_send(dev, ECORE_EVENT_MOUSE_BUTTON_UP);
}

/**
 * @brief Handles touch motion events from libinput.
 *
 * This function processes a LIBINPUT_EVENT_TOUCH_MOTION event.
 * It retrieves the Elput_Device and Elput_Touch structures.
 * It updates the seat's pointer coordinates based on the transformed touch
 * coordinates from the event (scaled to device's output dimensions:
 * dev->ow, dev->oh). The touch slot and timestamp are also updated.
 * Finally, a touch motion event is sent via _touch_motion_send.
 *
 * @param idevice The libinput device that generated the event.
 * @param event The libinput touch event data for touch motion.
 */
static void
_touch_motion(struct libinput_device *idevice, struct libinput_event_touch *event)
{
   Elput_Device *dev;
   Elput_Touch *touch;

   dev = libinput_device_get_user_data(idevice);
   if (!dev) return;

   touch = _evdev_touch_get(dev->seat);
   if (!touch) return;

   touch->seat->pointer.x =
     libinput_event_touch_get_x_transformed(event, dev->ow);
   touch->seat->pointer.y =
     libinput_event_touch_get_y_transformed(event, dev->oh);

   /* TODO: these needs to run a matrix transform based on output */
   /* _ecore_drm2_output_coordinate_transform(dev->output, */
   /*                                         touch->seat->pointer.x, touch->seat->pointer.y, */
   /*                                         &touch->seat->pointer.x, &touch->seat->pointer.y); */

   touch->slot = libinput_event_touch_get_slot(event);
   touch->timestamp = libinput_event_touch_get_time(event);

   _touch_motion_send(dev);
}

/**
 * @brief Applies a calibration matrix to a libinput device.
 *
 * This function attempts to apply a calibration matrix to the given Elput_Device.
 * It first checks if the device supports calibration and can retrieve a default matrix.
 * Then, it queries udev for a property "WL_CALIBRATION" associated with the
 * device's sysname. If found, this property is expected to contain 6 float values
 * for the calibration matrix (a, b, c, d, e, f for x' = ax + by + c, y' = dx + ey + f).
 * The translational components (c and f, which are cal[2] and cal[5]) are
 * normalized by the device's output width (dev->ow) and height (dev->oh)
 * respectively, before being applied to the libinput device.
 *
 * The calibration matrix format is typically:
 *   cal[0] = sx (scale x)
 *   cal[1] = rxy (rotation/shear xy)
 *   cal[2] = tx (translate x, in pixels)
 *   cal[3] = ryx (rotation/shear yx)
 *   cal[4] = sy (scale y)
 *   cal[5] = ty (translate y, in pixels)
 *
 * Libinput expects normalized translation, so tx/width and ty/height.
 *
 * @param dev The Elput_Device to calibrate.
 */
void
_evdev_device_calibrate(Elput_Device *dev)
{
   float cal[6];
   const char *vals;
   const char *sysname;
   const char *device;
   Eina_List *devices;
   int w = 0, h = 0;
   enum libinput_config_status status;

   w = dev->ow;
   h = dev->oh;
   if ((w == 0) || (h == 0)) return;

   if ((!libinput_device_config_calibration_has_matrix(dev->device)) ||
       (libinput_device_config_calibration_get_default_matrix(dev->device, cal) != 0))
     return;

   sysname = libinput_device_get_sysname(dev->device);

   devices = eeze_udev_find_by_subsystem_sysname("input", sysname);
   EINA_LIST_FREE(devices, device)
     {
        vals = eeze_udev_syspath_get_property(device, "WL_CALIBRATION");
        if ((!vals) ||
            (sscanf(vals, "%f %f %f %f %f %f",
                    &cal[0], &cal[1], &cal[2], &cal[3], &cal[4], &cal[5]) != 6))
          goto cont;

        cal[2] /= w;
        cal[5] /= h;

        status =
          libinput_device_config_calibration_set_matrix(dev->device, cal);
        if (status != LIBINPUT_CONFIG_STATUS_SUCCESS)
          WRN("Failed to apply device calibration");

cont:
        eina_stringshare_del(device);
     }
}

/**
 * @brief Frees an Ecore_Event_Axis_Update event and its associated data.
 *
 * This function is used as a callback for ecore_event_add when axis update
 * events (typically from tablet tools) are generated. It unreferences the
 * Evas_Device (if any) associated with the event, frees the array of
 * Ecore_Axis data, and then frees the event structure itself.
 *
 * @param d User data, unused in this function.
 * @param event The Ecore_Event_Axis_Update event data to be freed.
 */
static void
_axis_event_free(void *d EINA_UNUSED, void *event)
{
   Ecore_Event_Axis_Update *ev = event;

   if (ev->dev) efl_unref(ev->dev);
   free(ev->axis);
   free(ev);
}

/**
 * @brief Handles tablet tool axis events from libinput.
 *
 * This function processes a LIBINPUT_EVENT_TABLET_TOOL_AXIS event.
 * It updates the seat's pointer coordinates based on the transformed absolute
 * X and Y coordinates from the tablet tool event.
 * It then checks for changes in various axes supported by the tablet tool:
 * X, Y, pressure, distance, tilt (calculating magnitude and azimuth), and rotation.
 * For each changed axis, it populates an Ecore_Axis structure with the
 * appropriate label and value.
 *
 * If X or Y coordinates changed, a standard pointer motion event is sent
 * via _pointer_motion_send.
 * If any axis values changed, an ECORE_EVENT_AXIS_UPDATE event is created,
 * populated with an array of Ecore_Axis structures for all changed axes,
 * and added to the ecore event queue.
 *
 * @param idev The libinput device that generated the event.
 * @param event The libinput tablet tool event data.
 */
static void
_tablet_tool_axis(struct libinput_device *idev, struct libinput_event_tablet_tool *event)
{
   Elput_Pointer *ptr;
   struct libinput_tablet_tool *tool;
   Elput_Device *dev = libinput_device_get_user_data(idev);
   Ecore_Event_Axis_Update *ev;
   Ecore_Axis ax[8] = {}, *axis = NULL;
   int i, num = 0;

   ptr = _evdev_pointer_get(dev->seat);
   EINA_SAFETY_ON_NULL_RETURN(ptr);
   tool = libinput_event_tablet_tool_get_tool(event);

   ptr->seat->pointer.x =
     libinput_event_tablet_tool_get_x_transformed(event, dev->ow);
   ptr->seat->pointer.y =
     libinput_event_tablet_tool_get_y_transformed(event, dev->oh);

   if (libinput_event_tablet_tool_x_has_changed(event))
     {
        ax[num].label = ECORE_AXIS_LABEL_X;
        ax[num].value = ptr->seat->pointer.x;
        num++;
     }
   if (libinput_event_tablet_tool_y_has_changed(event))
     {
        ax[num].label = ECORE_AXIS_LABEL_Y;
        ax[num].value = ptr->seat->pointer.y;
        num++;
     }
   if (libinput_tablet_tool_has_pressure(tool))
     {
        if (libinput_event_tablet_tool_pressure_has_changed(event))
          {
             ax[num].label = ECORE_AXIS_LABEL_PRESSURE;
             ax[num].value = ptr->pressure =
               libinput_event_tablet_tool_get_pressure(event);
             num++;
          }
     }
   if (libinput_tablet_tool_has_distance(tool))
     {
        if (libinput_event_tablet_tool_distance_has_changed(event))
          {
             ax[num].label = ECORE_AXIS_LABEL_DISTANCE;
             ax[num].value = libinput_event_tablet_tool_get_distance(event);
             num++;
          }
     }
   if (libinput_tablet_tool_has_tilt(tool))
     {
        if (libinput_event_tablet_tool_tilt_x_has_changed(event) ||
            libinput_event_tablet_tool_tilt_y_has_changed(event))
          {
             double x = sin(libinput_event_tablet_tool_get_tilt_x(event));
             double y = sin(-libinput_event_tablet_tool_get_tilt_y(event));

             ax[num].label = ECORE_AXIS_LABEL_TILT;
             ax[num].value = asin(sqrt((x * x) + (y * y)));
             num++;

             /* note: the value of atan2(0,0) is implementation-defined */
             ax[num].label = ECORE_AXIS_LABEL_AZIMUTH;
             ax[num].value = atan2(y, x);
             num++;
          }
     }
   if (libinput_tablet_tool_has_rotation(tool))
     {
        if (libinput_event_tablet_tool_rotation_has_changed(event))
          {
             ax[num].label = ECORE_AXIS_LABEL_TWIST;
             ax[num].value = libinput_event_tablet_tool_get_rotation(event);
             ax[num].value *= M_PI / 180;
             num++;
          }
     }

   ptr->timestamp = libinput_event_tablet_tool_get_time(event);

   /* FIXME: other properties which efl event structs don't support:
    * slider_position
    * wheel_delta
    */

   if (libinput_event_tablet_tool_x_has_changed(event) ||
       libinput_event_tablet_tool_y_has_changed(event))
     _pointer_motion_send(dev);

   if (!num) return;
   ev = calloc(1, sizeof(Ecore_Event_Axis_Update));

   ev->window = dev->seat->manager->window;
   ev->event_window = dev->seat->manager->window;
   ev->root_window = dev->seat->manager->window;
   ev->timestamp = ptr->timestamp;
   ev->naxis = num;
   ev->dev = dev->evas_device;
   if (ev->dev) efl_ref(ev->dev);
   ev->axis = axis = calloc(num, sizeof(Ecore_Axis));
   for (i = 0; i < num; i++)
     {
        axis[i].label = ax[i].label;
        axis[i].value = ax[i].value;
     }
   ecore_event_add(ECORE_EVENT_AXIS_UPDATE, ev, _axis_event_free, NULL);
}

/**
 * @brief Handles tablet tool tip (pen down/up) events from libinput.
 *
 * This function processes a LIBINPUT_EVENT_TABLET_TOOL_TIP event, which
 * indicates the pen tip touching or leaving the surface.
 * It maps the libinput tip state (LIBINPUT_TABLET_TOOL_TIP_DOWN or
 * LIBINPUT_TABLET_TOOL_TIP_UP) to a libinput button state (PRESSED or RELEASED).
 * The pointer's button state is set to 1 (simulating a left mouse button).
 * If the tip is pressed down, _pointer_click_update is called to handle
 * potential double/triple click logic (though less common for pens).
 * Finally, _pointer_button_send is called to generate an Ecore mouse button
 * down or up event, effectively treating the pen tip as a mouse button.
 *
 * @param idev The libinput device that generated the event.
 * @param event The libinput tablet tool event data for tip state change.
 */
static void
_tablet_tool_tip(struct libinput_device *idev, struct libinput_event_tablet_tool *event)
{
   Elput_Pointer *ptr;
   Elput_Device *dev = libinput_device_get_user_data(idev);
   int state;
   int press[] =
     {
        [LIBINPUT_TABLET_TOOL_TIP_DOWN] = LIBINPUT_BUTTON_STATE_PRESSED,
        [LIBINPUT_TABLET_TOOL_TIP_UP] = LIBINPUT_BUTTON_STATE_RELEASED,
     };

   ptr = _evdev_pointer_get(dev->seat);
   EINA_SAFETY_ON_NULL_RETURN(ptr);

   state = libinput_event_tablet_tool_get_tip_state(event);
   ptr->buttons = 1;
   ptr->timestamp = libinput_event_tablet_tool_get_time(event);

   if (press[state]) _pointer_click_update(ptr, 1);

   _pointer_button_send(dev, press[state]);
}

/**
 * @brief Frees an Elput_Event_Switch event and its associated device reference.
 *
 * This function is used as a callback for ecore_event_add when switch toggle
 * events are generated. It decrements the reference count of the Elput_Device
 * associated with the switch event (destroying it if refs hit zero) and then
 * frees the event structure itself.
 *
 * @param data User data, unused in this function.
 * @param event The Elput_Event_Switch event data to be freed.
 */
static void
_switch_event_free(void *data EINA_UNUSED, void *event)
{
   Elput_Event_Switch *ev = event;

   _evdev_device_destroy(ev->device);
   free(ev);
}

/**
 * @brief Handles switch toggle events from libinput.
 *
 * This function processes a LIBINPUT_EVENT_SWITCH_TOGGLE event (e.g., lid switch,
 * tablet mode switch).
 * It allocates an Elput_Event_Switch, populates it with the device (taking a
 * reference), timestamp, switch type, and switch state from the libinput event.
 * The Elput_Event_Switch is then added to the ecore event queue.
 *
 * @param idev The libinput device that generated the event.
 * @param event The libinput switch event data.
 */
static void
_switch_toggle(struct libinput_device *idev, struct libinput_event_switch *event)
{
   Elput_Event_Switch *ev;

   ev = calloc(1, sizeof(Elput_Event_Switch));
   if (!ev) return;
   ev->device = libinput_device_get_user_data(idev);
   ev->device->refs++;
   ev->time_usec = libinput_event_switch_get_time_usec(event);
   ev->type = (Elput_Switch_Type)libinput_event_switch_get_switch(event);
   ev->state = (Elput_Switch_State)libinput_event_switch_get_switch_state(event);
   ecore_event_add(ELPUT_EVENT_SWITCH, ev, _switch_event_free, NULL);
}

/**
 * @brief Processes a generic libinput event and dispatches it to the appropriate handler.
 *
 * This function is the main entry point for handling events received from
 * libinput. It determines the type of the libinput event and calls the
 * corresponding internal processing function (e.g., _keyboard_key for
 * keyboard events, _pointer_motion for pointer motion, etc.).
 *
 * If the event results in a state change that should trigger a "frame"
 * (e.g., pointer motion, button press/release, axis event), it sets the
 * `frame` flag. After processing, if `frame` is true, it calls
 * _seat_frame_send to notify that a logical frame of input is complete.
 *
 * @param event The libinput_event to process.
 * @return 1 if the event was recognized and handled (or ignored by design,
 *           like LIBINPUT_EVENT_TOUCH_FRAME), 0 if the event type was
 *           not recognized or handled by this function.
 */
int
_evdev_event_process(struct libinput_event *event)
{
   struct libinput_device *idev;
   int ret = 1;
   Eina_Bool frame = EINA_FALSE;

   idev = libinput_event_get_device(event);
   switch (libinput_event_get_type(event))
     {
      case LIBINPUT_EVENT_KEYBOARD_KEY:
        _keyboard_key(idev, libinput_event_get_keyboard_event(event));
        break;
      case LIBINPUT_EVENT_POINTER_MOTION:
        frame =
          _pointer_motion(idev, libinput_event_get_pointer_event(event));
        break;
      case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
        frame =
          _pointer_motion_abs(idev, libinput_event_get_pointer_event(event));
        break;
      case LIBINPUT_EVENT_POINTER_BUTTON:
        frame =
          _pointer_button(idev, libinput_event_get_pointer_event(event));
        break;
      case LIBINPUT_EVENT_POINTER_AXIS:
        frame =
          _pointer_axis(idev, libinput_event_get_pointer_event(event));
        break;
      case LIBINPUT_EVENT_TOUCH_DOWN:
        _touch_down(idev, libinput_event_get_touch_event(event));
        break;
      case LIBINPUT_EVENT_TOUCH_MOTION:
        _touch_motion(idev, libinput_event_get_touch_event(event));
        break;
      case LIBINPUT_EVENT_TOUCH_UP:
        _touch_up(idev, libinput_event_get_touch_event(event));
        break;
      case LIBINPUT_EVENT_TOUCH_FRAME: break;
      case LIBINPUT_EVENT_TABLET_TOOL_AXIS:
        _tablet_tool_axis(idev, libinput_event_get_tablet_tool_event(event));
        break;
      case LIBINPUT_EVENT_TABLET_TOOL_PROXIMITY: /* is this useful? */
        break;
      case LIBINPUT_EVENT_TABLET_TOOL_TIP: /* is this useful? */
        _tablet_tool_tip(idev, libinput_event_get_tablet_tool_event(event));
        break;
      case LIBINPUT_EVENT_SWITCH_TOGGLE:
        _switch_toggle(idev, libinput_event_get_switch_event(event));
        break;
      default:
        ret = 0;
        break;
     }

   if (frame)
     {
        Elput_Device *edev;

        edev = libinput_device_get_user_data(idev);
        if (edev) _seat_frame_send(edev->seat);
     }

   return ret;
}

/**
 * @brief Creates and initializes an Elput_Device from a libinput_device.
 *
 * This function allocates an Elput_Device structure and associates it with
 * the given Elput_Seat and libinput_device. It sets the output dimensions
 * from the seat manager and stores the device's output name (or device name
 * as a fallback).
 *
 * It then checks the capabilities of the libinput_device (keyboard, pointer,
 * touch, tablet, switch, gesture) and sets corresponding flags in
 * `edev->caps`. Based on these capabilities, it initializes the necessary
 * sub-systems on the seat (e.g., _keyboard_init, _pointer_init, _touch_init).
 *
 * The Elput_Device is set as user data for the libinput_device, and a
 * reference to the libinput_device is taken.
 * If the device supports tap-to-click, it's configured with its default state.
 *
 * @param seat The Elput_Seat to which this new device will belong.
 * @param device The libinput_device to wrap.
 * @return A pointer to the newly created and initialized Elput_Device,
 *         or NULL on allocation failure.
 */
Elput_Device *
_evdev_device_create(Elput_Seat *seat, struct libinput_device *device)
{
   Elput_Device *edev;
   const char *oname;

   edev = calloc(1, sizeof(Elput_Device));
   if (!edev) return NULL;

   edev->refs = 1;
   edev->seat = seat;
   edev->device = device;
   edev->ow = seat->manager->output_w;
   edev->oh = seat->manager->output_h;

   oname = libinput_device_get_output_name(device);
   if (!oname)
     oname = libinput_device_get_name(device);
   eina_stringshare_replace(&edev->output_name, oname);

   if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_SWITCH))
     edev->caps |= ELPUT_DEVICE_CAPS_SWITCH;
   if ((libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_KEYBOARD)) &&
       (libinput_device_keyboard_has_key(device, KEY_ENTER)))
     edev->caps |= ELPUT_DEVICE_CAPS_KEYBOARD;
   if (edev->caps & (ELPUT_DEVICE_CAPS_SWITCH | ELPUT_DEVICE_CAPS_KEYBOARD))
     _keyboard_init(seat, seat->manager->cached.keymap);

   if ((libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_POINTER) &&
       (libinput_device_pointer_has_button(device, BTN_LEFT))))
     edev->caps |= ELPUT_DEVICE_CAPS_POINTER;
   if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TABLET_TOOL))
     edev->caps |= ELPUT_DEVICE_CAPS_POINTER | ELPUT_DEVICE_CAPS_TABLET_TOOL;
   if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TABLET_PAD))
     edev->caps |= ELPUT_DEVICE_CAPS_TABLET_PAD;
   if (edev->caps & ELPUT_DEVICE_CAPS_POINTER)
     _pointer_init(seat);

   if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TOUCH))
     edev->caps |= ELPUT_DEVICE_CAPS_TOUCH;
   if (libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_GESTURE))
     edev->caps |= ELPUT_DEVICE_CAPS_TOUCH | ELPUT_DEVICE_CAPS_GESTURE;
   if (edev->caps & ELPUT_DEVICE_CAPS_TOUCH)
     _touch_init(seat);

   libinput_device_set_user_data(device, edev);
   libinput_device_ref(edev->device);

   if (libinput_device_config_tap_get_finger_count(edev->device) > 0)
     {
        Eina_Bool enable = EINA_FALSE;

        enable = libinput_device_config_tap_get_default_enabled(edev->device);
        libinput_device_config_tap_set_enabled(edev->device, enable);
     }

   return edev;
}

/**
 * @brief Destroys an Elput_Device and releases its resources.
 *
 * This function decrements the reference count of the Elput_Device. If the
 * reference count reaches zero, it proceeds to release resources associated
 * with the device's capabilities (e.g., calling _pointer_release,
 * _keyboard_release, _touch_release for the seat).
 * It then unreferences the underlying libinput_device, frees the output name
 * stringshare, frees any key remapping hash table, and finally frees the
 * Elput_Device structure itself.
 *
 * @param edev The Elput_Device to destroy.
 */
void
_evdev_device_destroy(Elput_Device *edev)
{
   if (!edev) return;
   edev->refs--;
   if (edev->refs) return;

   if (edev->caps & ELPUT_DEVICE_CAPS_POINTER)
     _pointer_release(edev->seat);
   if (edev->caps & ELPUT_DEVICE_CAPS_KEYBOARD)
     _keyboard_release(edev->seat);
   if (edev->caps & ELPUT_DEVICE_CAPS_TOUCH)
     _touch_release(edev->seat);

   libinput_device_unref(edev->device);
   eina_stringshare_del(edev->output_name);

   if (edev->key_remap_hash) eina_hash_free(edev->key_remap_hash);

   free(edev);
}

/**
 * @brief Destroys an Elput_Keyboard structure and its associated XKB resources.
 *
 * This function frees all allocated strings within the `kbd->names` structure
 * (rules, model, layout, variant, options). It then unreferences XKB
 * resources: compose table, compose state, XKB state, maskless XKB state,
 * and the Elput_Keyboard_Info (which in turn unrefs the XKB keymap).
 * Finally, it unreferences the XKB context and frees the Elput_Keyboard
 * structure itself.
 *
 * @param kbd The Elput_Keyboard structure to destroy.
 */
void
_evdev_keyboard_destroy(Elput_Keyboard *kbd)
{
   free((char *)kbd->names.rules);
   free((char *)kbd->names.model);
   free((char *)kbd->names.layout);
   free((char *)kbd->names.variant);
   free((char *)kbd->names.options);

   if (kbd->compose_table) xkb_compose_table_unref(kbd->compose_table);
   if (kbd->compose_state) xkb_compose_state_unref(kbd->compose_state);

   if (kbd->state) xkb_state_unref(kbd->state);
   if (kbd->maskless_state) xkb_state_unref(kbd->maskless_state);
   if (kbd->info) _keyboard_info_destroy(kbd->info);

   xkb_context_unref(kbd->context);

   free(kbd);
}

/**
 * @brief Destroys an Elput_Pointer structure.
 *
 * Currently, this function primarily frees the Elput_Pointer structure itself.
 * The "FIXME" comment suggests that if any dynamically allocated resources
 * were added to Elput_Pointer, they should be freed here.
 *
 * @param ptr The Elput_Pointer structure to destroy.
 */
void
_evdev_pointer_destroy(Elput_Pointer *ptr)
{
   /* FIXME: destroy any resources inside pointer structure */
   free(ptr);
}

/**
 * @brief Destroys an Elput_Touch structure.
 *
 * Currently, this function primarily frees the Elput_Touch structure itself.
 * The "FIXME" comment suggests that if any dynamically allocated resources
 * were added to Elput_Touch, they should be freed here.
 *
 * @param touch The Elput_Touch structure to destroy.
 */
void
_evdev_touch_destroy(Elput_Touch *touch)
{
   /* FIXME: destroy any resources inside touch structure */
   free(touch);
}

/**
 * @brief Publicly accessible wrapper to send a pointer motion event.
 *
 * This function simply calls the internal _pointer_motion_send function.
 * It provides a way for other parts of the elput system to trigger a
 * pointer motion event dispatch if needed, for example, after a programmatic
 * cursor move or focus change that should also emit a move event.
 *
 * @param edev The Elput_Device for which to send a pointer motion event.
 *             The event will use the current state of this device's seat pointer.
 */
void
_evdev_pointer_motion_send(Elput_Device *edev)
{
   _pointer_motion_send(edev);
}

/**
 * @brief Retrieves the Elput_Pointer associated with a seat, if one is active.
 *
 * This function checks if the given seat is valid and if it has an active
 * pointer device (i.e., seat->count.ptr > 0).
 *
 * @param seat The Elput_Seat from which to get the pointer.
 * @return A pointer to the Elput_Pointer structure if active, otherwise NULL.
 */
Elput_Pointer *
_evdev_pointer_get(Elput_Seat *seat)
{
   if (!seat) return NULL;
   if (seat->count.ptr) return seat->ptr;
   return NULL;
}

/**
 * @brief Retrieves the Elput_Keyboard associated with a seat, if one is active.
 *
 * This function checks if the given seat is valid and if it has an active
 * keyboard device (i.e., seat->count.kbd > 0).
 *
 * @param seat The Elput_Seat from which to get the keyboard.
 * @return A pointer to the Elput_Keyboard structure if active, otherwise NULL.
 */
Elput_Keyboard *
_evdev_keyboard_get(Elput_Seat *seat)
{
   if (!seat) return NULL;
   if (seat->count.kbd) return seat->kbd;
   return NULL;
}

/**
 * @brief Retrieves the Elput_Touch associated with a seat, if one is active.
 *
 * This function checks if the given seat is valid and if it has an active
 * touch device (i.e., seat->count.touch > 0).
 *
 * @param seat The Elput_Seat from which to get the touch device.
 * @return A pointer to the Elput_Touch structure if active, otherwise NULL.
 */
Elput_Touch *
_evdev_touch_get(Elput_Seat *seat)
{
   if (!seat) return NULL;
   if (seat->count.touch) return seat->touch;
   return NULL;
}
