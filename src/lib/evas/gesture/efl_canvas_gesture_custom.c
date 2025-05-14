#define EFL_CANVAS_GESTURE_CUSTOM_PROTECTED
#include "efl_canvas_gesture_private.h"

#define MY_CLASS EFL_CANVAS_GESTURE_CUSTOM_CLASS


/**
 * @brief Destructor for the Efl_Canvas_Gesture_Custom object.
 *
 * This function is called when the Efl_Canvas_Gesture_Custom object is being destroyed.
 * It frees any resources allocated by the object, specifically the gesture_name string.
 *
 * @param obj The Efl_Canvas_Gesture_Custom object.
 * @param pd The private data associated with the object.
 */
EOLIAN static void
_efl_canvas_gesture_custom_efl_object_destructor(Eo *obj, Efl_Canvas_Gesture_Custom_Data *pd)
{
   eina_stringshare_del(pd->gesture_name);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Sets the name of the custom gesture.
 *
 * This function updates the internal gesture_name string with the provided name.
 * The name is stored as an Eina_Stringshare, which means the string is interned
 * for efficiency.
 *
 * @param obj The Efl_Canvas_Gesture_Custom object (unused).
 * @param pd The private data associated with the object, containing the gesture_name.
 * @param name The new name for the gesture. For example, "my_custom_swipe".
 */
EOLIAN static void
_efl_canvas_gesture_custom_gesture_name_set(Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Custom_Data *pd, const char *name)
{
   eina_stringshare_replace(&pd->gesture_name, name);
}

/**
 * @brief Gets the name of the custom gesture.
 *
 * This function returns the currently set gesture_name.
 * The returned string is an Eina_Stringshare.
 *
 * @param obj The Efl_Canvas_Gesture_Custom object (unused).
 * @param pd The private data associated with the object, containing the gesture_name.
 * @return The Eina_Stringshare representing the gesture name. For example, "my_custom_swipe".
 *         Returns NULL if no name has been set.
 */
EOLIAN static Eina_Stringshare *
_efl_canvas_gesture_custom_gesture_name_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Custom_Data *pd)
{
   return pd->gesture_name;
}
#include "efl_canvas_gesture_custom.eo.c"
