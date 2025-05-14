#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>
#include "elm_priv.h"

/**
 * @brief Private data structure for the Efl_Ui_Spotlight_Animation_Manager.
 *
 * This structure holds all the necessary data for managing animations
 * within a spotlight container, including references to the container itself,
 * animation objects, content elements, and state variables.
 */
typedef struct {
   Efl_Ui_Spotlight_Container *container; /**< The spotlight container this manager is bound to. */
   Efl_Canvas_Animation *jump_anim[2];    /**< Animations for 'jump' transition (0: out, 1: in). */
   Efl_Canvas_Animation *push_anim[2];    /**< Animations for 'push' transition (0: out, 1: in). */
   Efl_Canvas_Animation *pop_anim[2];     /**< Animations for 'pop' transition (0: out, 1: in). */
   Efl_Gfx_Entity *content[2];            /**< Current (0) and next (1) content elements being animated. */
   Efl_Gfx_Entity *clipper;               /**< Clipper object to ensure content stays within bounds. */
   int ids[2];                            /**< Indices of the current (0) and next (1) content elements during animation. */
   Eina_Size2D page_size;                 /**< The size of each page/view in the spotlight. */
   Eina_Bool animation;                   /**< Flag indicating whether animations are currently enabled. */
} Efl_Ui_Spotlight_Animation_Manager_Data;

#define MY_CLASS EFL_UI_SPOTLIGHT_ANIMATION_MANAGER_CLASS

/**
 * @brief Synchronizes the geometry of the content elements and clipper.
 *
 * This function ensures that the clipper matches the container's geometry
 * and that the content elements are centered within the container based
 * on the current page_size.
 *
 * @param obj The Efl_Ui_Spotlight_Animation_Manager object (unused).
 * @param pd The private data of the Efl_Ui_Spotlight_Animation_Manager.
 */
static void
_geom_sync(Eo *obj EINA_UNUSED, Efl_Ui_Spotlight_Animation_Manager_Data *pd)
{
   Eina_Rect group_pos = efl_gfx_entity_geometry_get(pd->container);
   Eina_Rect goal = EINA_RECT_EMPTY();
   goal.size = pd->page_size;
   goal.y = (group_pos.y + group_pos.h/2)-pd->page_size.h/2;
   goal.x = (group_pos.x + group_pos.w/2)-pd->page_size.w/2;
   efl_gfx_entity_geometry_set(pd->clipper, group_pos);
   for (int i = 0; i < 2; ++i)
     {
        if (pd->content[i])
          efl_gfx_entity_geometry_set(pd->content[i], goal);
     }
}

/**
 * @brief Callback executed during an animation's progress.
 *
 * This function is called repeatedly as an animation runs. It calculates
 * the absolute position between the transitioning views based on the
 * animation's progress and emits the EFL_UI_SPOTLIGHT_MANAGER_EVENT_POS_UPDATE event.
 *
 * @param data The Efl_Ui_Spotlight_Animation_Manager object.
 * @param ev The Efl_Event details (unused, but contains the animation object as ev->object).
 */
static void
_running_cb(void *data, const Efl_Event *ev EINA_UNUSED)
{
   Efl_Ui_Spotlight_Animation_Manager_Data *pd = efl_data_scope_safe_get(data, MY_CLASS);
   double absolut_position;

   EINA_SAFETY_ON_NULL_RETURN(pd);
   //calculate absolut position, multiply pos with 2.0 because duration is only 0.5)
   absolut_position = pd->ids[0] + (pd->ids[1] - pd->ids[0])*(efl_canvas_object_animation_progress_get(ev->object));
   efl_event_callback_call(data, EFL_UI_SPOTLIGHT_MANAGER_EVENT_POS_UPDATE, &absolut_position);
}

/**
 * @brief Callback executed when an animation related to an object finishes or is cancelled.
 *
 * This function is typically used to hide an object after its "out" animation
 * has completed. It checks if the animation truly finished (ev->info is EINA_FALSE
 * if finished, EINA_TRUE if cancelled before end).
 * It also cleans up its own event listener and the _running_cb listener.
 *
 * @param data The Efl_Ui_Spotlight_Animation_Manager object.
 * @param ev The Efl_Event details, where ev->object is the animated Efl_Gfx_Entity.
 */
static void
_hide_object_cb(void *data, const Efl_Event *ev)
{
   // ev->info is EINA_TRUE if animation was cancelled, EINA_FALSE if it finished.
   // We only hide if it finished.
   if (!ev->info)
     {
        efl_gfx_entity_visible_set(ev->object, EINA_FALSE);
        efl_event_callback_del(ev->object, ev->desc, _hide_object_cb, data);
        efl_event_callback_del(ev->object, EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_PROGRESS_UPDATED, _running_cb, data);
     }
}

EOLIAN static void
_efl_ui_spotlight_animation_manager_efl_ui_spotlight_manager_bind(Eo *obj, Efl_Ui_Spotlight_Animation_Manager_Data *pd, Efl_Ui_Spotlight_Container *spotlight)
{
   if (spotlight)
     {
        // Store the container and set up the clipper.
        pd->container = spotlight;

        pd->clipper = efl_add(EFL_CANVAS_RECTANGLE_CLASS,
                              evas_object_evas_get(pd->container));
        evas_object_static_clip_set(pd->clipper, EINA_TRUE);
        efl_canvas_group_member_add(spotlight, pd->clipper);

        // Initialize existing content: set clipper, add to container, hide.
        for (int i = 0; i < efl_content_count(spotlight) ; ++i) {
           Efl_Gfx_Entity *elem = efl_pack_content_get(spotlight, i);
           efl_key_data_set(elem, "_elm_leaveme", spotlight); // Mark to prevent accidental deletion by parent.
           efl_canvas_object_clipper_set(elem, pd->clipper);
           efl_canvas_group_member_add(pd->container, elem);
           efl_gfx_entity_visible_set(elem, EINA_FALSE);
        }
        // If there's an active element, make it visible and sync geometry.
        if (efl_ui_spotlight_active_element_get(spotlight))
          {
             pd->content[0] = efl_ui_spotlight_active_element_get(spotlight);
             efl_gfx_entity_visible_set(pd->content[0], EINA_TRUE);
             _geom_sync(obj, pd);
          }
     }
}

/**
 * @brief Updates the cached indices (pd->ids) of the currently managed content elements.
 *
 * This is necessary when content is added or removed from the container,
 * as the indices of existing elements might change. It avoids updating the
 * index of an element that is currently being removed (identified by `avoid_index`).
 * After updating indices, it emits a POS_UPDATE event with the new target position.
 *
 * @param obj The Efl_Ui_Spotlight_Animation_Manager object.
 * @param pd The private data of the Efl_Ui_Spotlight_Animation_Manager.
 * @param avoid_index The index of content that is being deleted, or -1 if not applicable.
 *                    This index should not be updated if it matches one of pd->ids.
 */
static void
_update_ids(Eo *obj, Efl_Ui_Spotlight_Animation_Manager_Data *pd, int avoid_index)
{
   for (int i = 0; i < 2; ++i)
     {
        // Only update if the id is valid and not the one being avoided (e.g. currently deleted).
        if (pd->ids[i] != -1 && pd->ids[i] != avoid_index)
          pd->ids[i] = efl_pack_index_get(pd->container, pd->content[i]);
     }
   // Report the target position. If pd->content[1] is NULL (e.g. after a deletion),
   // pd->ids[1] might be -1. The spotlight_container should handle this.
   double pos = pd->ids[1];
   efl_event_callback_call(obj, EFL_UI_SPOTLIGHT_MANAGER_EVENT_POS_UPDATE, &pos);
}

EOLIAN static void
_efl_ui_spotlight_animation_manager_efl_ui_spotlight_manager_content_add(Eo *obj, Efl_Ui_Spotlight_Animation_Manager_Data *pd, Efl_Gfx_Entity *subobj, int index EINA_UNUSED)
{
   efl_key_data_set(subobj, "_elm_leaveme", pd->container); // Mark to prevent accidental deletion by parent.
   efl_canvas_object_clipper_set(subobj, pd->clipper);
   efl_canvas_group_member_add(pd->container, subobj);
   efl_gfx_entity_visible_set(subobj, EINA_FALSE); // New content is initially hidden.
   _update_ids(obj, pd, -1); // Update indices as content structure changed.
}

EOLIAN static void
_efl_ui_spotlight_animation_manager_efl_ui_spotlight_manager_content_del(Eo *obj, Efl_Ui_Spotlight_Animation_Manager_Data *pd, Efl_Gfx_Entity *subobj, int index)
{
   efl_key_data_set(subobj, "_elm_leaveme", NULL); // Unmark.
   efl_canvas_object_clipper_set(subobj, NULL);
   efl_canvas_group_member_remove(pd->container, subobj); // Remove from our managed group.
   // If the deleted subobj was one of the active/next content, nullify its reference.
   for (int i = 0; i < 2; ++i)
     {
        if (pd->content[i] == subobj)
          pd->content[i] = NULL;
     }
   _update_ids(obj, pd, index); // Update indices, avoiding the deleted index.
}

/**
 * @brief Checks if a given index is valid for the spotlight container.
 *
 * A valid index is non-negative and less than the total number of
 * content elements in the container.
 *
 * @param obj The Efl_Ui_Spotlight_Container object.
 * @param index The index to check.
 * @return EINA_TRUE if the index is valid, EINA_FALSE otherwise.
 */
static Eina_Bool
is_valid(Eo *obj, int index)
{
   if (index < 0) return EINA_FALSE;
   if (index >= efl_content_count(obj)) return EINA_FALSE;

   return EINA_TRUE;
}

EOLIAN static void
_efl_ui_spotlight_animation_manager_efl_ui_spotlight_manager_switch_to(Eo *obj, Efl_Ui_Spotlight_Animation_Manager_Data *pd,
                                                                       int from, int to,
                                                                       Efl_Ui_Spotlight_Manager_Switch_Reason reason EINA_UNUSED)
{
   // Avoid redundant switches to the same target if it's already the upcoming content.
   if (efl_pack_content_get(pd->container, to) == pd->content[1])
     return;

   // Handle transition if both 'from' and 'to' indices are valid.
   if (is_valid(pd->container, to) && is_valid(pd->container, from))
     {
        int tmp[2] = {from, to}; // Store 'from' and 'to' indices.

        for (int i = 0; i < 2; ++i) // 0 for 'from' content, 1 for 'to' content
          {
             Efl_Canvas_Animation *animation = NULL;
             pd->ids[i] = tmp[i];
             pd->content[i] = efl_pack_content_get(pd->container, pd->ids[i]);

             // Select animation based on the switch reason.
             // For 'push', the element at index 'i' (0=old, 1=new) gets push_anim[i].
             // push_anim[0] is 'out' animation, push_anim[1] is 'in' animation.
             if (reason == EFL_UI_SPOTLIGHT_MANAGER_SWITCH_REASON_PUSH)
               animation = pd->push_anim[i];
             // For 'pop', similar logic with pop_anim.
             else if (reason == EFL_UI_SPOTLIGHT_MANAGER_SWITCH_REASON_POP)
               animation = pd->pop_anim[i];
             // Default to jump animation if no specific reason matches or animations not set.
             if (!animation)
               animation = pd->jump_anim[i];

             if (pd->animation)
               // Start animation:
               // For content[0] (from): progress from -1.0 (current) to 0.0 (disappeared left/right)
               // For content[1] (to): progress from 1.0 (new from right/left) to 0.0 (current)
               // The actual animation definitions (jump_anim, push_anim, pop_anim)
               // define what -1, 0, 1 mean in terms of visual properties (e.g., translation).
               efl_canvas_object_animation_start(pd->content[i], animation, -1.0 + 2.0 * i, 0.0);
             efl_gfx_entity_visible_set(pd->content[i], EINA_TRUE);
          }
        if (pd->animation)
          {
             // When the 'from' content animation finishes, hide it.
             efl_event_callback_add(pd->content[0], EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_CHANGED, _hide_object_cb, obj);
             // During the 'from' content animation, update the position.
             efl_event_callback_add(pd->content[0], EFL_CANVAS_OBJECT_ANIMATION_EVENT_ANIMATION_PROGRESS_UPDATED, _running_cb, obj);
             // Ensure the 'to' content is stacked above the 'from' content.
             efl_gfx_stack_above(pd->content[1], pd->content[0]);
          }
     }
   else // Handle cases where 'from' might be invalid (e.g., initial setup or empty stack)
     {
        double pos = to;

        pd->ids[0] = -1;       // No 'from' content.
        pd->content[0] = NULL;
        pd->ids[1] = to;       // Set 'to' content as the target.
        pd->content[1] = efl_pack_content_get(pd->container, to);
        if (pd->content[1]) // Make sure content exists at 'to' index
          efl_gfx_entity_visible_set(pd->content[1], EINA_TRUE);
        // Directly update position as there's no animation from a previous state.
        efl_event_callback_call(obj, EFL_UI_SPOTLIGHT_MANAGER_EVENT_POS_UPDATE, &pos);
     }

   _geom_sync(obj, pd); // Ensure geometry is up-to-date.
}

EOLIAN static void
_efl_ui_spotlight_animation_manager_efl_ui_spotlight_manager_size_set(Eo *obj, Efl_Ui_Spotlight_Animation_Manager_Data *pd, Eina_Size2D size)
{
   pd->page_size = size; // Store the new page size.
   _geom_sync(obj, pd);  // Update geometries based on the new size.
}

EOLIAN static void
_efl_ui_spotlight_animation_manager_efl_object_invalidate(Eo *obj, Efl_Ui_Spotlight_Animation_Manager_Data *pd)
{
   efl_invalidate(efl_super(obj, MY_CLASS)); // Call parent's invalidate.

   efl_del(pd->clipper); // Delete the clipper object.
   pd->clipper = NULL;

   // Clean up content elements: reset mapping colors and remove clipper.
   // This is important if the elements are reparented or reused elsewhere.
   if (pd->container) // Ensure container exists before iterating its content
     {
        for (int i = 0; i < efl_content_count(pd->container); ++i)
          {
             Efl_Gfx_Entity *elem = efl_pack_content_get(pd->container, i);
             if (!elem) continue;
             // Reset any color modifications applied by animations.
             for (int d = 0; d < 4; d++) // Assuming 4 points for mapping
               {
                  efl_gfx_mapping_color_set(elem, d, 255, 255, 255, 255);
               }

             efl_canvas_object_clipper_set(elem, NULL); // Remove our clipper.
          }
     }
}

EOLIAN static void
_efl_ui_spotlight_animation_manager_efl_ui_spotlight_manager_animated_transition_set(Eo *obj EINA_UNUSED, Efl_Ui_Spotlight_Animation_Manager_Data *pd, Eina_Bool animation)
{
   // Stop any currently running animations on the active/next content elements
   // if the animation state is changing.
   for (int i = 0; i < 2; ++i)
     {
        if (pd->content[i])
          efl_canvas_object_animation_stop(pd->content[i]);
     }
   pd->animation = animation; // Update the animation enabled flag.
}

EOLIAN static Eina_Bool
_efl_ui_spotlight_animation_manager_efl_ui_spotlight_manager_animated_transition_get(const Eo *obj EINA_UNUSED, Efl_Ui_Spotlight_Animation_Manager_Data *pd)
{
   return pd->animation; // Return the current state of the animation flag.
}

EOLIAN static void
_efl_ui_spotlight_animation_manager_push_setup_set(Eo *obj EINA_UNUSED, Efl_Ui_Spotlight_Animation_Manager_Data *pd, Efl_Canvas_Animation *in, Efl_Canvas_Animation *out)
{
   EINA_SAFETY_ON_NULL_RETURN(out);
   EINA_SAFETY_ON_NULL_RETURN(in);

   // Set the 'out' (current content moving out) and 'in' (new content moving in) animations for PUSH.
   // pd->push_anim[0] is for the item moving out.
   // pd->push_anim[1] is for the item moving in.
   efl_replace(&pd->push_anim[0], out);
   efl_replace(&pd->push_anim[1], in);
}

EOLIAN static void
_efl_ui_spotlight_animation_manager_pop_setup_set(Eo *obj EINA_UNUSED, Efl_Ui_Spotlight_Animation_Manager_Data *pd, Efl_Canvas_Animation *in, Efl_Canvas_Animation *out)
{
   EINA_SAFETY_ON_NULL_RETURN(out);
   EINA_SAFETY_ON_NULL_RETURN(in);

   // Set the 'out' (current content moving out) and 'in' (new content moving in) animations for POP.
   // pd->pop_anim[0] is for the item moving out.
   // pd->pop_anim[1] is for the item moving in.
   efl_replace(&pd->pop_anim[0], out);
   efl_replace(&pd->pop_anim[1], in);
}

EOLIAN static void
_efl_ui_spotlight_animation_manager_jump_setup_set(Eo *obj EINA_UNUSED, Efl_Ui_Spotlight_Animation_Manager_Data *pd, Efl_Canvas_Animation *in, Efl_Canvas_Animation *out)
{
   EINA_SAFETY_ON_NULL_RETURN(out);
   EINA_SAFETY_ON_NULL_RETURN(in);

   // Set the 'out' (current content moving out) and 'in' (new content moving in) animations for JUMP.
   // pd->jump_anim[0] is for the item moving out.
   // pd->jump_anim[1] is for the item moving in.
   efl_replace(&pd->jump_anim[0], out);
   efl_replace(&pd->jump_anim[1], in);
}

EOLIAN static Efl_Object*
_efl_ui_spotlight_animation_manager_efl_object_finalize(Eo *obj, Efl_Ui_Spotlight_Animation_Manager_Data *pd)
{
   // Ensure default jump animations are set if none were provided.
   // This is crucial for basic functionality if custom animations aren't configured.
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->jump_anim[0], NULL); // Outgoing animation for jump
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd->jump_anim[1], NULL); // Incoming animation for jump

   return efl_finalize(efl_super(obj, MY_CLASS)); // Call parent's finalize.
}

#include "efl_ui_spotlight_animation_manager.eo.c"
