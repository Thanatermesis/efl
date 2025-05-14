EWAPI const Efl_Event_Description _ELM_GLVIEW_EVENT_CREATED =
   EFL_EVENT_DESCRIPTION("created");
EWAPI const Efl_Event_Description _ELM_GLVIEW_EVENT_DESTROYED =
   EFL_EVENT_DESCRIPTION("destroyed");
EWAPI const Efl_Event_Description _ELM_GLVIEW_EVENT_RESIZED =
   EFL_EVENT_DESCRIPTION("resized");
EWAPI const Efl_Event_Description _ELM_GLVIEW_EVENT_RENDER =
   EFL_EVENT_DESCRIPTION("render");

/**
 * @internal
 * @brief Internal implementation for elm_obj_glview_version_constructor.
 *
 * Initializes the GLView object with a specific Evas_GL context version.
 *
 * @param obj The Eo object.
 * @param pd The private data for Elm_Glview.
 * @param version The Evas_GL context version to use.
 */
void _elm_glview_version_constructor(Eo *obj, Elm_Glview_Data *pd, Evas_GL_Context_Version version);

EOAPI EFL_VOID_FUNC_BODYV(elm_obj_glview_version_constructor, EFL_FUNC_CALL(version), Evas_GL_Context_Version version);

/**
 * @internal
 * @brief Internal implementation for elm_obj_glview_resize_policy_set.
 *
 * Sets the resize policy for the GLView object.
 *
 * @param obj The Eo object.
 * @param pd The private data for Elm_Glview.
 * @param policy The resize policy to set.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool _elm_glview_resize_policy_set(Eo *obj, Elm_Glview_Data *pd, Elm_GLView_Resize_Policy policy);

EOAPI EFL_FUNC_BODYV(elm_obj_glview_resize_policy_set, Eina_Bool, 0, EFL_FUNC_CALL(policy), Elm_GLView_Resize_Policy policy);

/**
 * @internal
 * @brief Internal implementation for elm_obj_glview_render_policy_set.
 *
 * Sets the render policy for the GLView object.
 *
 * @param obj The Eo object.
 * @param pd The private data for Elm_Glview.
 * @param policy The render policy to set.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool _elm_glview_render_policy_set(Eo *obj, Elm_Glview_Data *pd, Elm_GLView_Render_Policy policy);

EOAPI EFL_FUNC_BODYV(elm_obj_glview_render_policy_set, Eina_Bool, 0, EFL_FUNC_CALL(policy), Elm_GLView_Render_Policy policy);

/**
 * @internal
 * @brief Internal implementation for elm_obj_glview_mode_set.
 *
 * Sets the rendering mode (alpha, depth, stencil, etc.) for the GLView.
 *
 * @param obj The Eo object.
 * @param pd The private data for Elm_Glview.
 * @param mode The rendering mode to set.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool _elm_glview_mode_set(Eo *obj, Elm_Glview_Data *pd, Elm_GLView_Mode mode);

EOAPI EFL_FUNC_BODYV(elm_obj_glview_mode_set, Eina_Bool, 0, EFL_FUNC_CALL(mode), Elm_GLView_Mode mode);

/**
 * @internal
 * @brief Internal implementation for elm_obj_glview_gl_api_get.
 *
 * Retrieves the Evas_GL_API structure associated with the GLView.
 *
 * @param obj The Eo object.
 * @param pd The private data for Elm_Glview.
 * @return A pointer to the Evas_GL_API structure, or NULL on failure.
 */
Evas_GL_API *_elm_glview_gl_api_get(const Eo *obj, Elm_Glview_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_glview_gl_api_get, Evas_GL_API *, NULL);

/**
 * @internal
 * @brief Internal implementation for elm_obj_glview_evas_gl_get.
 *
 * Retrieves the Evas_GL context associated with the GLView.
 *
 * @param obj The Eo object.
 * @param pd The private data for Elm_Glview.
 * @return A pointer to the Evas_GL context, or NULL on failure.
 */
Evas_GL *_elm_glview_evas_gl_get(const Eo *obj, Elm_Glview_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_glview_evas_gl_get, Evas_GL *, NULL);

/**
 * @internal
 * @brief Internal implementation for elm_obj_glview_rotation_get.
 *
 * Gets the current rotation of the GLView, particularly for direct rendering.
 *
 * @param obj The Eo object.
 * @param pd The private data for Elm_Glview.
 * @return The rotation angle in degrees (0, 90, 180, or 270).
 */
int _elm_glview_rotation_get(const Eo *obj, Elm_Glview_Data *pd);

EOAPI EFL_FUNC_BODY_CONST(elm_obj_glview_rotation_get, int, 0);

/**
 * @internal
 * @brief Internal implementation for elm_obj_glview_draw_request.
 *
 * Notifies the GLView that it needs to be redrawn.
 *
 * @param obj The Eo object.
 * @param pd The private data for Elm_Glview.
 */
void _elm_glview_draw_request(Eo *obj, Elm_Glview_Data *pd);

EOAPI EFL_VOID_FUNC_BODY(elm_obj_glview_draw_request);

/**
 * @internal
 * @brief Implements the Efl.Object.constructor interface for Elm_Glview.
 *
 * Called when the object is constructed.
 *
 * @param obj The Eo object being constructed.
 * @param pd The private data for Elm_Glview.
 * @return The constructed object, or NULL on failure.
 */
Efl_Object *_elm_glview_efl_object_constructor(Eo *obj, Elm_Glview_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Object.finalize interface for Elm_Glview.
 *
 * Called when the object is finalized. This is where final setup that
 * depends on other parts of the object (like parent) can be done.
 *
 * @param obj The Eo object being finalized.
 * @param pd The private data for Elm_Glview.
 * @return The finalized object.
 */
Efl_Object *_elm_glview_efl_object_finalize(Eo *obj, Elm_Glview_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Gfx.Entity.size_set interface for Elm_Glview.
 *
 * Called when the size of the GLView entity is set.
 *
 * @param obj The Eo object.
 * @param pd The private data for Elm_Glview.
 * @param size The new size of the entity.
 */
void _elm_glview_efl_gfx_entity_size_set(Eo *obj, Elm_Glview_Data *pd, Eina_Size2D size);

/**
 * @internal
 * @brief Implements the Efl.Ui.Focus.Object.on_focus_update interface for Elm_Glview.
 *
 * Handles focus updates for the GLView object.
 *
 * @param obj The Eo object.
 * @param pd The private data for Elm_Glview.
 * @return EINA_TRUE if focus update was handled, EINA_FALSE otherwise.
 */
Eina_Bool _elm_glview_efl_ui_focus_object_on_focus_update(Eo *obj, Elm_Glview_Data *pd);

/**
 * @internal
 * @brief Implements the Efl.Gfx.View.view_size_set interface for Elm_Glview.
 *
 * Sets the view size for the GLView. This might be different from the entity size,
 * for example, if there's scaling involved.
 *
 * @param obj The Eo object.
 * @param pd The private data for Elm_Glview.
 * @param size The new view size.
 */
void _elm_glview_efl_gfx_view_view_size_set(Eo *obj, Elm_Glview_Data *pd, Eina_Size2D size);

/**
 * @internal
 * @brief Implements the Efl.Gfx.View.view_size_get interface for Elm_Glview.
 *
 * Gets the view size for the GLView.
 *
 * @param obj The Eo object.
 * @param pd The private data for Elm_Glview.
 * @return The current view size.
 */
Eina_Size2D _elm_glview_efl_gfx_view_view_size_get(const Eo *obj, Elm_Glview_Data *pd);

/**
 * @internal
 * @brief Initializes the Elm_Glview class.
 *
 * This function is called once when the class is being set up.
 * It defines the operations (methods) for the class.
 *
 * @param klass The Efl_Class to initialize.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_elm_glview_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef ELM_GLVIEW_EXTRA_OPS
#define ELM_GLVIEW_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(elm_obj_glview_version_constructor, _elm_glview_version_constructor),
      EFL_OBJECT_OP_FUNC(elm_obj_glview_resize_policy_set, _elm_glview_resize_policy_set),
      EFL_OBJECT_OP_FUNC(elm_obj_glview_render_policy_set, _elm_glview_render_policy_set),
      EFL_OBJECT_OP_FUNC(elm_obj_glview_mode_set, _elm_glview_mode_set),
      EFL_OBJECT_OP_FUNC(elm_obj_glview_gl_api_get, _elm_glview_gl_api_get),
      EFL_OBJECT_OP_FUNC(elm_obj_glview_evas_gl_get, _elm_glview_evas_gl_get),
      EFL_OBJECT_OP_FUNC(elm_obj_glview_rotation_get, _elm_glview_rotation_get),
      EFL_OBJECT_OP_FUNC(elm_obj_glview_draw_request, _elm_glview_draw_request),
      EFL_OBJECT_OP_FUNC(efl_constructor, _elm_glview_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_finalize, _elm_glview_efl_object_finalize),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _elm_glview_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_ui_focus_object_on_focus_update, _elm_glview_efl_ui_focus_object_on_focus_update),
      EFL_OBJECT_OP_FUNC(efl_gfx_view_size_set, _elm_glview_efl_gfx_view_view_size_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_view_size_get, _elm_glview_efl_gfx_view_view_size_get),
      ELM_GLVIEW_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Describes the Elm_Glview class structure.
 *
 * This structure provides metadata about the Elm_Glview class,
 * including its version, name, type, size of private data,
 * and initializer/constructor functions.
 */
static const Efl_Class_Description _elm_glview_class_desc = {
   EO_VERSION,
   "Elm.Glview",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Elm_Glview_Data),
   _elm_glview_class_initializer,
   _elm_glview_class_constructor,
   NULL
};

/**
 * @internal
 * @brief Macro that defines the Elm_Glview class.
 *
 * This macro uses the class description and parent classes to formally define
 * the Elm_Glview class within the Efl object system.
 * It also generates the `elm_glview_class_get()` function.
 */
EFL_DEFINE_CLASS(elm_glview_class_get, &_elm_glview_class_desc, EFL_UI_WIDGET_CLASS, EFL_GFX_VIEW_INTERFACE, EFL_UI_LEGACY_INTERFACE, NULL);

#include "elm_glview_eo.legacy.c"
