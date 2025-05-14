#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_WIDGET_FOCUS_MANAGER_PROTECTED
#define EFL_UI_FOCUS_LAYER_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"

#define MY_CLASS EFL_UI_FOCUS_LAYER_MIXIN

/**
 * @brief Private data structure for the Efl_Ui_Focus_Layer_Mixin.
 */
typedef struct {
   Efl_Ui_Focus_Object *old_focus; /**< Stores the focus object that was focused before this layer became active. Used to restore focus when the layer is disabled. */
   Efl_Ui_Focus_Manager *registered_manager; /**< The manager this layer is registered with. Typically the top-level widget (e.g., window). */
   Efl_Ui_Focus_Manager *manager; /**< The internal focus manager for this layer. Manages focus within the layer's own hierarchy. */
   Eina_Bool cycle; /**< If EINA_TRUE, focus movement will cycle within this layer. */
   Eina_Bool enable_on_visible; /**< If EINA_TRUE, the layer will automatically enable/disable itself based on its visibility. */
} Efl_Ui_Focus_Layer_Data;

/**
 * @brief Creates a new focus manager for this layer.
 *
 * This function is called when a focus manager is requested for this layer.
 * It creates an instance of EFL_UI_FOCUS_MANAGER_ROOT_FOCUS_CLASS,
 * which is a specialized focus manager that roots its focus within the provided `root` object.
 *
 * @param obj The Efl_Ui_Focus_Layer object.
 * @param pd Private data for the Efl_Ui_Focus_Layer.
 * @param root The root object for the new focus manager.
 * @return The newly created focus manager.
 */
EOLIAN static Efl_Ui_Focus_Manager*
_efl_ui_focus_layer_efl_ui_widget_focus_manager_focus_manager_create(Eo *obj, Efl_Ui_Focus_Layer_Data *pd EINA_UNUSED, Efl_Ui_Focus_Object *root)
{
   pd->manager = efl_add(EFL_UI_FOCUS_MANAGER_ROOT_FOCUS_CLASS, obj, efl_ui_focus_manager_root_set(efl_added, root));
   return pd->manager;
}

/**
 * @brief Sets the visibility of the focus layer.
 *
 * If `enable_on_visible` is true, this function will also enable or disable
 * the focus layer accordingly.
 *
 * @param obj The Efl_Ui_Focus_Layer object.
 * @param pd Private data for the Efl_Ui_Focus_Layer.
 * @param v EINA_TRUE if visible, EINA_FALSE otherwise.
 */
EOLIAN static void
_efl_ui_focus_layer_efl_gfx_entity_visible_set(Eo *obj, Efl_Ui_Focus_Layer_Data *pd, Eina_Bool v)
{
   efl_gfx_entity_visible_set(efl_super(obj, MY_CLASS), v);

   if (pd->enable_on_visible)
     {
        efl_ui_focus_layer_enable_set(obj, v);
     }
}

/**
 * @brief Moves the focus within the layer based on the given direction.
 *
 * This function handles focus movement within the layer. If the movement
 * results in no newly focused object (e.g., reaching the end of the focus chain)
 * and cycling is disabled, it returns NULL. If cycling is enabled, it may
 * reset focus to the layer itself and then return the current focus.
 *
 * @param obj The Efl_Ui_Focus_Layer object.
 * @param pd Private data for the Efl_Ui_Focus_Layer.
 * @param direction The direction to move the focus.
 * @return The newly focused object, or NULL if no object could be focused in that direction.
 */
EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_focus_layer_efl_ui_focus_manager_move(Eo *obj, Efl_Ui_Focus_Layer_Data *pd, Efl_Ui_Focus_Direction direction)
{
   Eo *ret = efl_ui_focus_manager_move(pd->manager, direction);

   if (ret)
     return ret;

   //ret is NULL here, if we do not want to cycle return NULL, which will result in obj being unset
   if (!pd->cycle)
     return NULL;

   if ((direction == EFL_UI_FOCUS_DIRECTION_PREVIOUS ) || (direction == EFL_UI_FOCUS_DIRECTION_NEXT))
     efl_ui_focus_manager_focus_set(pd->manager, obj);

   return efl_ui_focus_manager_focus_get(obj);
}

/**
 * @brief Invalidates the focus layer object.
 *
 * This function ensures that the layer is disabled before the object
 * is fully invalidated.
 *
 * @param obj The Efl_Ui_Focus_Layer object.
 * @param pd Private data for the Efl_Ui_Focus_Layer.
 */
EOLIAN static void
_efl_ui_focus_layer_efl_object_invalidate(Eo *obj, Efl_Ui_Focus_Layer_Data *pd EINA_UNUSED)
{
   efl_ui_focus_layer_enable_set(obj, EINA_FALSE);
   efl_invalidate(efl_super(obj, MY_CLASS));
}

/**
 * @brief Gets the focus manager this layer is registered with.
 *
 * @param obj The Efl_Ui_Focus_Layer object.
 * @param pd Private data for the Efl_Ui_Focus_Layer.
 * @return The registered focus manager, or NULL if not registered.
 */
EOLIAN static Efl_Ui_Focus_Manager*
_efl_ui_focus_layer_efl_ui_focus_object_focus_manager_get(const Eo *obj EINA_UNUSED, Efl_Ui_Focus_Layer_Data *pd EINA_UNUSED)
{
   if (pd->registered_manager)
     return pd->registered_manager;
   else
     return NULL;
}

/**
 * @brief Gets the focus parent of this layer.
 *
 * The focus parent is typically the root of the manager this layer is registered with.
 *
 * @param obj The Efl_Ui_Focus_Layer object.
 * @param pd Private data for the Efl_Ui_Focus_Layer.
 * @return The focus parent object, or NULL if not registered.
 */
EOLIAN static Efl_Ui_Focus_Object*
_efl_ui_focus_layer_efl_ui_focus_object_focus_parent_get(const Eo *obj EINA_UNUSED, Efl_Ui_Focus_Layer_Data *pd)
{
   if (pd->registered_manager)
     return efl_ui_focus_manager_root_get(pd->registered_manager);
   else
     return NULL;
}

/**
 * @brief Applies focus state for the widget.
 *
 * This function is part of the Efl.Ui.Widget.Focus_State interface.
 * For a focus layer, it currently does not apply any specific state and returns EINA_FALSE.
 *
 * @param obj The Efl_Ui_Focus_Layer object.
 * @param pd Private data for the Efl_Ui_Focus_Layer.
 * @param current_state The current focus state of the widget.
 * @param configured_state Pointer to store the configured focus state.
 * @param redirect Pointer to store a widget to redirect focus to.
 * @return EINA_FALSE, indicating no state was applied or changed.
 */
EOLIAN static Eina_Bool
_efl_ui_focus_layer_efl_ui_widget_focus_state_apply(Eo *obj EINA_UNUSED, Efl_Ui_Focus_Layer_Data *pd EINA_UNUSED, Efl_Ui_Widget_Focus_State current_state EINA_UNUSED, Efl_Ui_Widget_Focus_State *configured_state EINA_UNUSED, Efl_Ui_Widget *redirect EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Constructor for the Efl_Ui_Focus_Layer.
 *
 * Initializes default values for `enable_on_visible` and `cycle`.
 *
 * @param obj The Efl_Ui_Focus_Layer object being constructed.
 * @param pd Private data for the Efl_Ui_Focus_Layer.
 * @return The constructed object.
 */
EOLIAN static Efl_Object*
_efl_ui_focus_layer_efl_object_constructor(Eo *obj, Efl_Ui_Focus_Layer_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd->enable_on_visible = EINA_TRUE;
   pd->cycle = EINA_TRUE;
   return obj;
}

/**
 * @brief Publishes focus manager and parent changed events.
 *
 * This helper function is used to emit events indicating that the
 * focus manager or focus parent of this layer has changed.
 *
 * @param obj The Efl_Ui_Focus_Layer object.
 * @param omanager The old focus manager.
 * @param oobj The old focus parent object.
 */
static void
_publish_state_change(Eo *obj, Efl_Ui_Focus_Manager *omanager, Efl_Ui_Focus_Object *oobj)
{
   efl_event_callback_call(obj, EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_MANAGER_CHANGED, omanager);
   efl_event_callback_call(obj, EFL_UI_FOCUS_OBJECT_EVENT_FOCUS_PARENT_CHANGED, oobj);
}

/**
 * @brief Enables or disables the focus layer.
 *
 * When enabled, the layer registers itself with its parent focus manager (typically a window)
 * and redirects focus to its internal manager. It also attempts to store the previously
 * focused element to restore it upon disabling.
 * When disabled, it unregisters itself and attempts to restore focus to the previously
 * focused element or clears the redirection.
 *
 * @param obj The Efl_Ui_Focus_Layer object.
 * @param pd Private data for the Efl_Ui_Focus_Layer.
 * @param v EINA_TRUE to enable, EINA_FALSE to disable.
 */
EOLIAN static void
_efl_ui_focus_layer_enable_set(Eo *obj, Efl_Ui_Focus_Layer_Data *pd, Eina_Bool v)
{
   if (!elm_object_tree_focus_allow_get(obj))
     v = EINA_FALSE;
   if (v)
     {
        Efl_Ui_Focus_Manager *manager;

        pd->registered_manager = elm_widget_top_get(obj);
        EINA_SAFETY_ON_FALSE_RETURN(efl_isa(pd->registered_manager, EFL_UI_WIN_CLASS));
        manager = efl_ui_focus_util_active_manager(pd->registered_manager);

        efl_ui_focus_manager_calc_register_logical(pd->registered_manager, obj, efl_ui_focus_manager_root_get(pd->registered_manager), obj);
        _publish_state_change(obj, NULL, NULL);

        pd->old_focus = efl_ui_focus_manager_focus_get(manager);
        efl_ui_focus_manager_focus_set(pd->manager, obj);

     }
   else
     {
        Eina_Bool fallback = EINA_TRUE;

        Eo *oobj;

        if (!pd->registered_manager) return;

        oobj = efl_ui_focus_manager_root_get(pd->registered_manager);

        //restore old focus
        if (pd->old_focus)
          {
             Efl_Ui_Focus_Manager *manager;

             manager = efl_ui_focus_object_focus_manager_get(pd->old_focus);
             if (manager)
               {
                  efl_ui_focus_manager_focus_set(manager, pd->old_focus);
                  fallback = EINA_FALSE;
               }
          }

        pd->old_focus = NULL;

        if (fallback && efl_ui_focus_manager_redirect_get(pd->registered_manager) == obj)
          {
             Efl_Ui_Focus_Manager *m = pd->registered_manager;

             while (efl_ui_focus_manager_redirect_get(m))
               {
                  Efl_Ui_Focus_Manager *old = m;

                  m = efl_ui_focus_manager_redirect_get(m);
                  efl_ui_focus_manager_redirect_set(old, NULL);
               }
          }

        efl_ui_focus_manager_calc_unregister(pd->registered_manager, obj);
        pd->registered_manager = NULL;
        _publish_state_change(obj, pd->registered_manager, oobj);
     }
}

/**
 * @brief Gets the enabled state of the focus layer.
 *
 * The layer is considered enabled if it has a registered manager and
 * that manager is currently redirecting focus to this layer.
 *
 * @param obj The Efl_Ui_Focus_Layer object.
 * @param pd Private data for the Efl_Ui_Focus_Layer.
 * @return EINA_TRUE if enabled, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_focus_layer_enable_get(const Eo *obj EINA_UNUSED, Efl_Ui_Focus_Layer_Data *pd)
{
   if (!pd->registered_manager) return EINA_FALSE;
   return (efl_ui_focus_manager_redirect_get(pd->registered_manager) == obj);
}

/**
 * @brief Sets the behaviour of the focus layer.
 *
 * This configures whether the layer should automatically enable/disable
 * based on its visibility, and whether focus movement should cycle within the layer.
 *
 * @param obj The Efl_Ui_Focus_Layer object.
 * @param pd Private data for the Efl_Ui_Focus_Layer.
 * @param enable_on_visible If EINA_TRUE, layer enables/disables with visibility.
 * @param cycle If EINA_TRUE, focus cycles within the layer.
 */
EOLIAN static void
_efl_ui_focus_layer_behaviour_set(Eo *obj EINA_UNUSED, Efl_Ui_Focus_Layer_Data *pd, Eina_Bool enable_on_visible, Eina_Bool cycle)
{
   pd->enable_on_visible = enable_on_visible;
   pd->cycle = cycle;
}

/**
 * @brief Gets the behaviour of the focus layer.
 *
 * Retrieves the current settings for `enable_on_visible` and `cycle`.
 *
 * @param obj The Efl_Ui_Focus_Layer object.
 * @param pd Private data for the Efl_Ui_Focus_Layer.
 * @param enable_on_visible Pointer to store the enable_on_visible flag.
 * @param cycle Pointer to store the cycle flag.
 */
EOLIAN static void
_efl_ui_focus_layer_behaviour_get(const Eo *obj EINA_UNUSED, Efl_Ui_Focus_Layer_Data *pd, Eina_Bool *enable_on_visible, Eina_Bool *cycle)
{
   *cycle = pd->cycle;
   *enable_on_visible = pd->enable_on_visible;
}

#include "efl_ui_focus_layer.eo.c"
