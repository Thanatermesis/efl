#include "efl_canvas_gesture_private.h"

#define MY_CLASS EFL_CANVAS_GESTURE_RECOGNIZER_CUSTOM_CLASS

/**
 * @brief Gets the Efl_Canvas_Gesture_Custom class.
 *
 * This function returns the Eolian class associated with the custom gesture.
 *
 * @param obj The Eolian object.
 * @param pd The private data for the custom gesture recognizer.
 * @return The Efl_Canvas_Gesture_Custom class.
 */
EOLIAN static const Efl_Class *
_efl_canvas_gesture_recognizer_custom_efl_canvas_gesture_recognizer_type_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Recognizer_Custom_Data *pd EINA_UNUSED)
{
   return EFL_CANVAS_GESTURE_CUSTOM_CLASS;
}

/**
 * @brief Finalizes the Eolian object for the custom gesture recognizer.
 *
 * This function is called when the object is being finalized. It ensures
 * that the gesture name (pd->name) is set before finalizing the superclass.
 *
 * @param obj The Eolian object to finalize.
 * @param pd The private data for the custom gesture recognizer.
 * @return The finalized Eolian object, or NULL on failure.
 */
EOLIAN static Eo *
_efl_canvas_gesture_recognizer_custom_efl_object_finalize(Eo *obj, Efl_Canvas_Gesture_Recognizer_Custom_Data *pd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->name, NULL);
   return efl_finalize(efl_super(obj, MY_CLASS));
}

/**
 * @brief Destroys the Eolian object for the custom gesture recognizer.
 *
 * This function is called when the object is being destroyed. It frees
 * the allocated stringshare for the gesture name and then calls the
 * destructor of the superclass.
 *
 * @param obj The Eolian object to destroy.
 * @param pd The private data for the custom gesture recognizer.
 */
EOLIAN static void
_efl_canvas_gesture_recognizer_custom_efl_object_destructor(Eo *obj, Efl_Canvas_Gesture_Recognizer_Custom_Data *pd)
{
   eina_stringshare_del(pd->name);
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Sets the name of the custom gesture.
 *
 * This function updates the internal name identifier for this custom gesture.
 * The name is stored as an Eina_Stringshare.
 *
 * @param obj The Eolian object.
 * @param pd The private data for the custom gesture recognizer.
 * @param name The new name for the gesture. For example, "MyCustomTap".
 */
EOLIAN static void
_efl_canvas_gesture_recognizer_custom_gesture_name_set(Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Recognizer_Custom_Data *pd, const char *name)
{
   eina_stringshare_replace(&pd->name, name);
}

/**
 * @brief Gets the name of the custom gesture.
 *
 * This function retrieves the internal name identifier for this custom gesture.
 *
 * @param obj The Eolian object.
 * @param pd The private data for the custom gesture recognizer.
 * @return The Eina_Stringshare representing the name of the gesture.
 *         For example, "MyCustomTap".
 */
EOLIAN static Eina_Stringshare *
_efl_canvas_gesture_recognizer_custom_gesture_name_get(const Eo *obj EINA_UNUSED, Efl_Canvas_Gesture_Recognizer_Custom_Data *pd)
{
   return pd->name;
}

#include "efl_canvas_gesture_recognizer_custom.eo.c"
