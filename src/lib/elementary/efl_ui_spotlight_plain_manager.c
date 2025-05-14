#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>
#include "elm_priv.h"
#include "efl_ui_spotlight_plain_manager.eo.h"

/**
 * @brief Private data structure for the Efl_Ui_Spotlight_Plain_Manager class.
 *
 * This structure holds all the necessary data for managing a plain spotlight,
 * including the container, page size, current content, clipper, animation state,
 * and last known position.
 */
typedef struct {
   Efl_Ui_Spotlight_Container * container; /**< The spotlight container being managed. */
   Eina_Size2D page_size; /**< The size of each page/content element. */
   Efl_Ui_Widget *current_content; /**< The currently visible content widget. */
   Efl_Gfx_Entity *clipper; /**< A clipper object to ensure content stays within bounds. */
   Eina_Bool animation; /**< Flag indicating if transitions should be animated (currently unused by plain manager). */
   double last_pos; /**< The last emitted position, used to avoid redundant events. */
} Efl_Ui_Spotlight_Plain_Manager_Data;

#define MY_CLASS EFL_UI_SPOTLIGHT_PLAIN_MANAGER_CLASS

/**
 * @brief Emits the POS_UPDATE event if the current content's position has changed.
 *
 * This function calculates the absolute position (index) of the current content
 * within the container and triggers the EFL_UI_SPOTLIGHT_MANAGER_EVENT_POS_UPDATE
 * event if this position differs from the last recorded position.
 *
 * @param obj The Efl_Ui_Spotlight_Plain_Manager object.
 * @param pd The private data of the Efl_Ui_Spotlight_Plain_Manager object.
 */
static void
_emit_position(Eo *obj EINA_UNUSED, Efl_Ui_Spotlight_Plain_Manager_Data *pd)
{
   double absolut_position = efl_pack_index_get(pd->container, pd->current_content);
   if (!EINA_DBL_EQ(pd->last_pos, absolut_position))
     efl_event_callback_call(obj, EFL_UI_SPOTLIGHT_MANAGER_EVENT_POS_UPDATE, &absolut_position);

   pd->last_pos = absolut_position;
}

/**
 * @brief Synchronizes the geometry of the current content and its clipper.
 *
 * This function calculates the target geometry for the current content based on
 * the container's geometry and the manager's page size. It then applies this
 * geometry to both the clipper and the current content, ensuring the content
 * is centered within the container.
 *
 * @param obj The Efl_Ui_Spotlight_Plain_Manager object.
 * @param pd The private data of the Efl_Ui_Spotlight_Plain_Manager object.
 */
static void
_geom_sync(Eo *obj EINA_UNUSED, Efl_Ui_Spotlight_Plain_Manager_Data *pd)
{
   Efl_Gfx_Entity *entity = pd->current_content;
   Eina_Rect group_pos = efl_gfx_entity_geometry_get(pd->container);
   Eina_Rect goal = EINA_RECT_EMPTY();

   goal.size = pd->page_size;
   goal.y = (group_pos.y + group_pos.h/2)-pd->page_size.h/2;
   goal.x = (group_pos.x + group_pos.w/2)-pd->page_size.w/2;
   efl_gfx_entity_geometry_set(pd->clipper, goal);
   efl_gfx_entity_geometry_set(entity, goal);
}

/**
 * @internal
 * @brief Binds the manager to a spotlight container.
 *
 * This function is called when the manager is associated with a spotlight container.
 * It initializes the clipper, sets up existing content elements (sets their clipper,
 * adds them to the container group, and initially hides them), and then makes the
 * active element visible.
 *
 * @param obj The Efl_Ui_Spotlight_Plain_Manager object.
 * @param pd The private data of the Efl_Ui_Spotlight_Plain_Manager object.
 * @param spotlight The Efl_Ui_Spotlight_Container to bind to.
 */
EOLIAN static void
_efl_ui_spotlight_plain_manager_efl_ui_spotlight_manager_bind(Eo *obj, Efl_Ui_Spotlight_Plain_Manager_Data *pd, Efl_Ui_Spotlight_Container *spotlight)
{
   if (spotlight)
     {
        Efl_Ui_Widget *index;

        pd->container = spotlight;

        pd->clipper = efl_add(EFL_CANVAS_RECTANGLE_CLASS,
                              evas_object_evas_get(spotlight));
        evas_object_static_clip_set(pd->clipper, EINA_TRUE);
        efl_canvas_group_member_add(spotlight, pd->clipper);

        for (int i = 0; i < efl_content_count(spotlight) ; ++i) {
           Efl_Gfx_Entity *elem = efl_pack_content_get(spotlight, i);
           efl_key_data_set(elem, "_elm_leaveme", spotlight);
           efl_canvas_object_clipper_set(elem, pd->clipper);
           efl_canvas_group_member_add(pd->container, elem);
           efl_gfx_entity_visible_set(elem, EINA_FALSE);
        }
        index = efl_ui_spotlight_active_element_get(spotlight);
        if (index)
          {
             pd->current_content = index;
             efl_gfx_entity_visible_set(pd->current_content, EINA_TRUE);
             _geom_sync(obj, pd);
             _emit_position(obj, pd);
          }
     }
}

/**
 * @brief Handles changes in the active content of the spotlight container.
 *
 * This function is called when the active element in the container might have changed.
 * It updates the visibility of the old and new current content, synchronizes geometry,
 * and emits a position update.
 *
 * @param obj The Efl_Ui_Spotlight_Plain_Manager object.
 * @param pd The private data of the Efl_Ui_Spotlight_Plain_Manager object.
 */
static void
_content_changed(Eo *obj, Efl_Ui_Spotlight_Plain_Manager_Data *pd)
{
   if (efl_ui_spotlight_active_element_get(pd->container) != pd->current_content)
     {
        Efl_Ui_Widget *old_current_content = pd->current_content;
        pd->current_content = efl_ui_spotlight_active_element_get(pd->container);
        efl_gfx_entity_visible_set(old_current_content, EINA_FALSE);
        efl_gfx_entity_visible_set(pd->current_content, EINA_TRUE);
        _geom_sync(obj, pd);
     }
   _emit_position(obj, pd);
}

/**
 * @internal
 * @brief Handles the addition of new content to the spotlight container.
 *
 * This function is called when a new sub-object (content) is added to the
 * spotlight container. It sets the clipper for the new content, adds it to the
 * container's canvas group, initially hides it, and then triggers a content
 * changed check.
 *
 * @param obj The Efl_Ui_Spotlight_Plain_Manager object.
 * @param pd The private data of the Efl_Ui_Spotlight_Plain_Manager object.
 * @param subobj The Efl_Gfx_Entity (content) that was added.
 * @param index The index at which the content was added (unused by plain manager).
 */
EOLIAN static void
_efl_ui_spotlight_plain_manager_efl_ui_spotlight_manager_content_add(Eo *obj, Efl_Ui_Spotlight_Plain_Manager_Data *pd, Efl_Gfx_Entity *subobj, int index EINA_UNUSED)
{
   efl_key_data_set(subobj, "_elm_leaveme", pd->container);
   efl_canvas_object_clipper_set(subobj, pd->clipper);
   efl_canvas_group_member_add(pd->container, subobj);
   efl_gfx_entity_visible_set(subobj, EINA_FALSE);
   _content_changed(obj, pd);
}

/**
 * @internal
 * @brief Handles the deletion of content from the spotlight container.
 *
 * This function is called when a sub-object (content) is removed from the
 * spotlight container. It unsets the clipper, removes it from the container's
 * canvas group, updates the current content if necessary, and then triggers
 * a content changed check.
 *
 * @param obj The Efl_Ui_Spotlight_Plain_Manager object.
 * @param pd The private data of the Efl_Ui_Spotlight_Plain_Manager object.
 * @param subobj The Efl_Gfx_Entity (content) that was removed.
 * @param index The index from which the content was removed (unused by plain manager).
 */
EOLIAN static void
_efl_ui_spotlight_plain_manager_efl_ui_spotlight_manager_content_del(Eo *obj, Efl_Ui_Spotlight_Plain_Manager_Data *pd, Efl_Gfx_Entity *subobj, int index EINA_UNUSED)
{
   efl_key_data_set(subobj, "_elm_leaveme", NULL);
   efl_canvas_object_clipper_set(subobj, NULL);
   efl_canvas_group_member_remove(pd->container, subobj);
   if (pd->current_content == subobj)
     pd->current_content = NULL;
   _content_changed(obj, pd);
}

/**
 * @internal
 * @brief Switches the visible content in the spotlight.
 *
 * This function handles the logic for switching from one content element to another.
 * It hides the 'from' object and shows the 'to' object, updates the current content,
 * emits a position update, and synchronizes geometry. The 'from' index and 'reason'
 * are unused by this plain manager as it performs an immediate switch without animation.
 *
 * @param obj The Efl_Ui_Spotlight_Plain_Manager object.
 * @param pd The private data of the Efl_Ui_Spotlight_Plain_Manager object.
 * @param from The index of the content element to switch from.
 * @param to The index of the content element to switch to.
 * @param reason The reason for the switch (e.g., user interaction, programmatic change). Unused.
 */
EOLIAN static void
_efl_ui_spotlight_plain_manager_efl_ui_spotlight_manager_switch_to(Eo *obj, Efl_Ui_Spotlight_Plain_Manager_Data *pd, int from EINA_UNUSED, int to, Efl_Ui_Spotlight_Manager_Switch_Reason reason EINA_UNUSED)
{
   Efl_Gfx_Entity *to_obj, *from_obj;
   to_obj = efl_pack_content_get(pd->container, to);
   from_obj = efl_pack_content_get(pd->container, from);
   if (from_obj)
     {
        efl_gfx_entity_visible_set(from_obj, EINA_FALSE);
        pd->current_content = NULL;
     }

   if (to_obj)
     {
        efl_gfx_entity_visible_set(to_obj, EINA_TRUE);
        pd->current_content = efl_pack_content_get(pd->container, to);
     }

   _emit_position(obj, pd);
   _geom_sync(obj, pd);
}

/**
 * @internal
 * @brief Sets the size for each page/content element in the spotlight.
 *
 * This function updates the internal page size and then triggers a geometry
 * synchronization to apply the new size.
 *
 * @param obj The Efl_Ui_Spotlight_Plain_Manager object.
 * @param pd The private data of the Efl_Ui_Spotlight_Plain_Manager object.
 * @param size The new Eina_Size2D for the content elements.
 */
EOLIAN static void
_efl_ui_spotlight_plain_manager_efl_ui_spotlight_manager_size_set(Eo *obj, Efl_Ui_Spotlight_Plain_Manager_Data *pd, Eina_Size2D size)
{
   pd->page_size = size;
   _geom_sync(obj, pd);
}

/**
 * @internal
 * @brief Destructor for the Efl_Ui_Spotlight_Plain_Manager object.
 *
 * This function is called when the object is being destroyed. It ensures that
 * all content elements within the container are made visible again, as the manager
 * might have hidden some of them.
 *
 * @param obj The Efl_Ui_Spotlight_Plain_Manager object being destructed.
 * @param pd The private data of the Efl_Ui_Spotlight_Plain_Manager object.
 */
EOLIAN static void
_efl_ui_spotlight_plain_manager_efl_object_destructor(Eo *obj, Efl_Ui_Spotlight_Plain_Manager_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));

   // Ensure all elements are visible when the manager is destroyed,
   // as the manager controls their visibility.
   if (pd->container) // Check if container is still valid
     {
        for (int i = 0; i < efl_content_count(pd->container); ++i)
          {
             Efl_Gfx_Stack *elem = efl_pack_content_get(pd->container, i);
             if (elem) efl_gfx_entity_visible_set(elem, EINA_TRUE);
          }
     }
}

/**
 * @internal
 * @brief Sets whether transitions should be animated.
 *
 * For the plain manager, this setting is stored but does not affect behavior
 * as transitions are immediate.
 *
 * @param obj The Efl_Ui_Spotlight_Plain_Manager object.
 * @param pd The private data of the Efl_Ui_Spotlight_Plain_Manager object.
 * @param animation EINA_TRUE to enable animations, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_ui_spotlight_plain_manager_efl_ui_spotlight_manager_animated_transition_set(Eo *obj EINA_UNUSED, Efl_Ui_Spotlight_Plain_Manager_Data *pd, Eina_Bool animation)
{
   pd->animation = animation;
}

/**
 * @internal
 * @brief Gets whether transitions are set to be animated.
 *
 * For the plain manager, this returns the stored value, though animations
 * are not actually performed.
 *
 * @param obj The Efl_Ui_Spotlight_Plain_Manager object.
 * @param pd The private data of the Efl_Ui_Spotlight_Plain_Manager object.
 * @return EINA_TRUE if animations are set to be enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_spotlight_plain_manager_efl_ui_spotlight_manager_animated_transition_get(const Eo *obj EINA_UNUSED, Efl_Ui_Spotlight_Plain_Manager_Data *pd)
{
   return pd->animation;
}

/**
 * @internal
 * @brief Invalidates the Efl_Ui_Spotlight_Plain_Manager object.
 *
 * This function is called when the object is being invalidated (e.g., before
 * destruction or when its canvas is removed). It cleans up resources like the
 * clipper and unsets the clipper from all content elements.
 *
 * @param obj The Efl_Ui_Spotlight_Plain_Manager object being invalidated.
 * @param pd The private data of the Efl_Ui_Spotlight_Plain_Manager object.
 */
EOLIAN static void
_efl_ui_spotlight_plain_manager_efl_object_invalidate(Eo *obj, Efl_Ui_Spotlight_Plain_Manager_Data *pd)
{
   efl_del(pd->clipper);
   pd->clipper = NULL; // Avoid dangling pointer

   if (pd->container) // Check if container is still valid
     {
        for (int i = 0; i < efl_content_count(pd->container); ++i)
          {
             Efl_Gfx_Entity *elem = efl_pack_content_get(pd->container, i);
             if (elem) efl_canvas_object_clipper_set(elem, NULL);
          }
     }

   efl_invalidate(efl_super(obj, MY_CLASS));
}

#include "efl_ui_spotlight_plain_manager.eo.c"
