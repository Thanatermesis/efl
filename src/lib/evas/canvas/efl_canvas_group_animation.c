#include "efl_canvas_group_animation_private.h"

/**
 * @brief Adds an animation to the group.
 *
 * If the group animation's duration has been explicitly set, the added
 * animation's duration will be updated to match the group's duration.
 * The final state keep property of the animation is also synchronized
 * with the group's property.
 *
 * @param eo_obj The Efl_Canvas_Group_Animation object.
 * @param pd The private data of the Efl_Canvas_Group_Animation object.
 * @param animation The Efl_Canvas_Animation to add.
 */
EOLIAN static void
_efl_canvas_group_animation_animation_add(Eo *eo_obj,
                                   Efl_Canvas_Group_Animation_Data *pd,
                                   Efl_Canvas_Animation *animation)
{
   if (!animation) return;

   /* To preserve each animation's duration, group animation's duration is
    * copied to each animation's duration only if group animation's duration is
    * set. */
   if (pd->is_duration_set)
     {
        double duration = efl_animation_duration_get(efl_super(eo_obj, MY_CLASS));
        efl_animation_duration_set(animation, duration);
     }

   Eina_Bool keep_final_state = efl_animation_final_state_keep_get(eo_obj);
   efl_animation_final_state_keep_set(animation, keep_final_state);

   pd->animations = eina_list_append(pd->animations, animation);
   efl_ref(animation);
}

/**
 * @brief Deletes an animation from the group.
 *
 * If the animation is found in the group, it is removed and its
 * reference count is decremented.
 *
 * @param eo_obj The Efl_Canvas_Group_Animation object (unused).
 * @param pd The private data of the Efl_Canvas_Group_Animation object.
 * @param animation The Efl_Canvas_Animation to delete.
 */
EOLIAN static void
_efl_canvas_group_animation_animation_del(Eo *eo_obj EINA_UNUSED,
                                   Efl_Canvas_Group_Animation_Data *pd,
                                   Efl_Canvas_Animation *animation)
{
   Eina_List *list;
   if (!animation) return;

   list = eina_list_data_find_list(pd->animations, animation);
   if (list)
     {
        pd->animations = eina_list_remove_list(pd->animations, list);
        efl_unref(animation);
     }
   else
     {
        ERR("Animation(%s@%p) is not in the group animation.",
            efl_class_name_get(animation), animation);
     }
}

/**
 * @brief Gets an iterator over the animations in the group.
 *
 * @param eo_obj The Efl_Canvas_Group_Animation object (unused).
 * @param pd The private data of the Efl_Canvas_Group_Animation object.
 * @return An Eina_Iterator for the list of animations. The caller
 *         is responsible for freeing the iterator.
 *         Example of iterating:
 *         Eina_Iterator *it = _efl_canvas_group_animation_animations_get(obj, pd);
 *         Efl_Canvas_Animation *anim;
 *         EINA_ITERATOR_FOREACH(it, anim) {
 *           // process anim
 *         }
 *         eina_iterator_free(it);
 */
EOLIAN static Eina_Iterator*
_efl_canvas_group_animation_animations_get(const Eo *eo_obj EINA_UNUSED,
                                    Efl_Canvas_Group_Animation_Data *pd)
{
   return eina_list_iterator_new(pd->animations);
}

/**
 * @brief Sets the duration for the group animation and all its child animations.
 *
 * This function sets the duration for the group animation itself and then
 * propagates this duration to all animations currently in the group.
 * It also marks that the group's duration has been explicitly set,
 * which affects how subsequently added animations are handled.
 *
 * @param eo_obj The Efl_Canvas_Group_Animation object.
 * @param pd The private data of the Efl_Canvas_Group_Animation object.
 * @param duration The duration in seconds. Must be non-negative.
 *                 Example: 2.5 for 2.5 seconds.
 */
EOLIAN static void
_efl_canvas_group_animation_efl_canvas_animation_duration_set(Eo *eo_obj,
                                                Efl_Canvas_Group_Animation_Data *pd,
                                                double duration)
{
   EINA_SAFETY_ON_FALSE_RETURN(duration >= 0.0);

   efl_animation_duration_set(efl_super(eo_obj, MY_CLASS), duration);
   duration = efl_animation_duration_get(eo_obj);

   Eina_List *l;
   Efl_Canvas_Animation *anim;
   EINA_LIST_FOREACH(pd->animations, l, anim)
     {
        efl_animation_duration_set(anim, duration);
     }

   pd->is_duration_set = EINA_TRUE;
}

/**
 * @brief Sets whether to keep the final state for the group animation and all its child animations.
 *
 * This function sets the final_state_keep property for the group animation
 * itself and then propagates this setting to all animations currently
 * in the group.
 *
 * @param eo_obj The Efl_Canvas_Group_Animation object.
 * @param pd The private data of the Efl_Canvas_Group_Animation object.
 * @param keep_final_state EINA_TRUE to keep the final state, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_canvas_group_animation_efl_canvas_animation_final_state_keep_set(Eo *eo_obj,
                                                        Efl_Canvas_Group_Animation_Data *pd,
                                                        Eina_Bool keep_final_state)
{
   Eina_List *l;
   Efl_Canvas_Animation *anim;
   EINA_LIST_FOREACH(pd->animations, l, anim)
     {
        efl_animation_final_state_keep_set(anim, keep_final_state);
     }

   efl_animation_final_state_keep_set(efl_super(eo_obj, MY_CLASS), keep_final_state);
}

/**
 * @brief Sets the interpolator for the group animation and all its child animations.
 *
 * This function sets the interpolator for the group animation itself and
 * then propagates this interpolator to all animations currently in the group.
 *
 * @param eo_obj The Efl_Canvas_Group_Animation object.
 * @param pd The private data of the Efl_Canvas_Group_Animation object.
 * @param interpolator The Efl_Interpolator to set.
 *                     Example: efl_new(EFL_ANIMATION_INTERPOLATOR_LINEAR_CLASS)
 */
EOLIAN static void
_efl_canvas_group_animation_efl_canvas_animation_interpolator_set(Eo *eo_obj,
                                                    Efl_Canvas_Group_Animation_Data *pd,
                                                    Efl_Interpolator *interpolator)
{
   Eina_List *l;
   Efl_Canvas_Animation *anim;
   EINA_LIST_FOREACH(pd->animations, l, anim)
     {
        efl_animation_interpolator_set(anim, interpolator);
     }

   efl_animation_interpolator_set(efl_super(eo_obj, MY_CLASS), interpolator);
}

/**
 * @brief Constructor for Efl_Canvas_Group_Animation.
 *
 * Initializes the group animation object, primarily by setting up the
 * internal list of animations.
 *
 * @param eo_obj The Efl_Canvas_Group_Animation object being constructed.
 * @param pd The private data of the Efl_Canvas_Group_Animation object.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_canvas_group_animation_efl_object_constructor(Eo *eo_obj,
                                            Efl_Canvas_Group_Animation_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));
   pd->animations = NULL;
   // pd->is_duration_set is implicitly EINA_FALSE (0) by calloc

   return eo_obj;
}

/**
 * @brief Destructor for Efl_Canvas_Group_Animation.
 *
 * Cleans up resources used by the group animation, specifically by
 * unreferencing all animations in its internal list and freeing the list.
 *
 * @param eo_obj The Efl_Canvas_Group_Animation object being destructed.
 * @param pd The private data of the Efl_Canvas_Group_Animation object.
 */
EOLIAN static void
_efl_canvas_group_animation_efl_object_destructor(Eo *eo_obj,
                                           Efl_Canvas_Group_Animation_Data *pd)
{
   Efl_Canvas_Animation *anim;

   EINA_LIST_FREE(pd->animations, anim)
     efl_unref(anim);

   efl_destructor(efl_super(eo_obj, MY_CLASS));
}

#include "efl_canvas_group_animation.eo.c"
