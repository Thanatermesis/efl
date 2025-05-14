#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_glview.h"

#define MY_CLASS ELM_GLVIEW_CLASS

#define MY_CLASS_NAME "Elm_Glview"
#define MY_CLASS_NAME_LEGACY "elm_glview"

static const char SIG_FOCUSED[] = "focused";
static const char SIG_UNFOCUSED[] = "unfocused";

/* smart callbacks coming from elm glview objects: */
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_FOCUSED, ""},
   {SIG_UNFOCUSED, ""},
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {NULL, NULL}
};

/**
 * @internal
 * @brief Updates the focus state of the GLView widget.
 *
 * This function is called when the focus state of the GLView object needs to be updated.
 * It propagates the focus to the superclass and then sets the focus on the internal
 * resize object accordingly.
 *
 * @param obj The Evas object.
 * @param _pd Private data, unused in this function.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_glview_efl_ui_focus_object_on_focus_update(Eo *obj, Elm_Glview_Data *_pd EINA_UNUSED)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);
   Eina_Bool int_ret = EINA_FALSE;

   int_ret = efl_ui_focus_object_on_focus_update(efl_super(obj, MY_CLASS));
   if (!int_ret) return EINA_FALSE;

   if (efl_ui_focus_object_focus_get(obj))
     evas_object_focus_set(wd->resize_obj, EINA_TRUE);
   else
     evas_object_focus_set(wd->resize_obj, EINA_FALSE);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Updates or recreates the Evas_GL surface for the GLView.
 *
 * This function is responsible for managing the underlying Evas_GL surface.
 * It handles destroying an old surface, creating a new one based on current
 * dimensions and configuration, and setting it to the image object used for display.
 * It also warns if direct rendering is requested with an incompatible render policy
 * and fakes a resize event to allow clients to reconfigure their viewports.
 *
 * @param obj The Evas object (GLView).
 */
static void
_glview_update_surface(Evas_Object *obj)
{
   Evas_Native_Surface ns = {};
   Evas_GL_Options_Bits opt;

   ELM_GLVIEW_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   if (!sd) return;

   if (!evas_gl_make_current(sd->evasgl, NULL, NULL))
     return;

   if (sd->surface)
     {
        evas_object_image_native_surface_set(wd->resize_obj, NULL);
        evas_gl_surface_destroy(sd->evasgl, sd->surface);
     }

   evas_object_image_size_set(wd->resize_obj, sd->w, sd->h);

   opt = sd->config->options_bits;
   if ((opt & EVAS_GL_OPTIONS_DIRECT) &&
       (sd->render_policy != ELM_GLVIEW_RENDER_POLICY_ON_DEMAND))
     {
        if (!sd->warned_about_dr)
          {
             WRN("App requested direct rendering but render policy is not ON_DEMAND. "
                 "Disabling direct rendering...");
             sd->warned_about_dr = EINA_TRUE;
          }
        sd->config->options_bits &= ~(EVAS_GL_OPTIONS_DIRECT);
     }
   sd->surface = evas_gl_surface_create(sd->evasgl, sd->config, sd->w, sd->h);
   sd->config->options_bits = opt;
   evas_gl_native_surface_get(sd->evasgl, sd->surface, &ns);
   evas_object_image_native_surface_set(wd->resize_obj, &ns);
   elm_obj_glview_draw_request(obj);

   // fake a resize event so that clients can reconfigure their viewport
   sd->resized = EINA_TRUE;
}

/**
 * @internal
 * @brief Sets the size of the GLView widget.
 *
 * This function is called when the size of the GLView entity changes.
 * It propagates the size change to the superclass. If the scale policy is
 * ELM_GLVIEW_RESIZE_POLICY_RECREATE, it updates the internal dimensions (sd->w, sd->h)
 * and recreates the GL surface by calling _glview_update_surface().
 *
 * @param obj The Evas object.
 * @param sd The private data of the GLView.
 * @param sz The new size (width and height).
 */
EOLIAN static void
_elm_glview_efl_gfx_entity_size_set(Eo *obj, Elm_Glview_Data *sd, Eina_Size2D sz)
{
   if (_evas_object_intercept_call(obj, EVAS_OBJECT_INTERCEPT_CB_RESIZE, 0, sz.w, sz.h))
     return;

   efl_gfx_entity_size_set(efl_super(obj, MY_CLASS), sz);

   sd->resized = EINA_TRUE;

   if (sd->scale_policy == ELM_GLVIEW_RESIZE_POLICY_RECREATE)
     {
        if ((sz.w == 0) || (sz.h == 0))
          sz = EINA_SIZE2D(64, 64);

        sd->w = sz.w;
        sd->h = sz.h;

        _glview_update_surface(obj);
     }
}

/**
 * @internal
 * @brief Callback function for rendering the GLView content.
 *
 * This function is invoked to perform GL rendering. It ensures the GL context
 * is current, calls the user-defined initialization function (once),
 * resize function (if the view was resized), and the main rendering function.
 * It handles different rendering policies (on-demand vs. always) and manages
 * an idle enterer for continuous rendering if required.
 *
 * @param obj The Evas object (GLView).
 * @param event The Efl_Event data, unused in this function.
 */
static void
_render_cb(void *obj, const Efl_Event *event EINA_UNUSED)
{
   ELM_GLVIEW_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, );

   evas_object_render_op_set(wd->resize_obj, evas_object_render_op_get(obj));

   // Do a make current
   if (!evas_gl_make_current(sd->evasgl, sd->surface, sd->context))
     {
        ERR("Failed doing make current.");
        goto on_error;
     }

   // Call the init function if it hasn't been called already
   if (!sd->initialized)
     {
        //TODO:will be optimized
        efl_event_callback_legacy_call(obj, ELM_GLVIEW_EVENT_CREATED, NULL);
        if (sd->init_func) sd->init_func(obj);
        sd->initialized = EINA_TRUE;
     }

   if (sd->resized)
     {
        //TODO:will be optimized
        efl_event_callback_legacy_call(obj, ELM_GLVIEW_EVENT_RESIZED, NULL);
        if (sd->resize_func) sd->resize_func(obj);
        sd->resized = EINA_FALSE;
     }

   if (sd->render_policy == ELM_GLVIEW_RENDER_POLICY_ALWAYS)
     evas_sync(evas_object_evas_get(obj));
   // Call the render function
   if (sd->render_func) sd->render_func(obj);
   //TODO:will be optimized
   efl_event_callback_legacy_call(obj, ELM_GLVIEW_EVENT_RENDER, NULL);

   // Depending on the policy return true or false
   if (sd->render_policy == ELM_GLVIEW_RENDER_POLICY_ON_DEMAND)
     {
        return;
     }
   else if (sd->render_policy == ELM_GLVIEW_RENDER_POLICY_ALWAYS)
     {
        // Return false so it only runs once
        goto on_error;
     }
   else
     {
        ERR("Invalid Render Policy.");
        goto on_error;
     }

   return;

 on_error:
   efl_event_callback_del(efl_main_loop_get(),
                         EFL_LOOP_EVENT_IDLE_ENTER,
                         _render_cb,
                         obj);
   sd->render_idle_enterer = 0;
}

/**
 * @internal
 * @brief Configures callbacks and mechanisms based on the current render policy.
 *
 * This function adjusts the GLView's rendering behavior according to the
 * specified render policy.
 * For ELM_GLVIEW_RENDER_POLICY_ON_DEMAND, it sets up a pixel get callback
 * to trigger rendering when Evas needs the pixels.
 * For ELM_GLVIEW_RENDER_POLICY_ALWAYS, it sets up an idle enterer to
 * continuously call the _render_cb function.
 *
 * @param obj The Evas object (GLView).
 */
static void
_set_render_policy_callback(Evas_Object *obj)
{
   ELM_GLVIEW_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   switch (sd->render_policy)
     {
      case ELM_GLVIEW_RENDER_POLICY_ON_DEMAND:
         if (sd->render_idle_enterer)
           {
              evas_object_image_pixels_dirty_set(wd->resize_obj, EINA_TRUE);
              evas_object_image_data_update_add(wd->resize_obj, 0, 0, sd->w, sd->h);
           }
         // Delete idle_enterer if it for some reason is around
         efl_event_callback_del(efl_main_loop_get(),
                               EFL_LOOP_EVENT_IDLE_ENTER,
                               _render_cb,
                               obj);
         sd->render_idle_enterer = 0;

        // Set pixel getter callback
        evas_object_image_pixels_get_callback_set
          (wd->resize_obj,
          (Evas_Object_Image_Pixels_Get_Cb)_render_cb,
          obj);
        break;

      case ELM_GLVIEW_RENDER_POLICY_ALWAYS:
        if (evas_object_image_pixels_dirty_get(wd->resize_obj) && !sd->render_idle_enterer)
          sd->render_idle_enterer = efl_event_callback_priority_add(efl_main_loop_get(),
                                                                   EFL_LOOP_EVENT_IDLE_ENTER,
                                                                   EFL_CALLBACK_PRIORITY_BEFORE,
                                                                   _render_cb,
                                                                   obj);
        // Unset the pixel getter callback if set already
        evas_object_image_pixels_get_callback_set
          (wd->resize_obj, NULL, NULL);

        break;

      default:
        ERR("Invalid Render Policy.");
        return;
     }
}

/**
 * @internal
 * @brief Called when the GLView object is added to a canvas group.
 *
 * This function initializes the visual representation of the GLView.
 * It creates an Evas image object that will be used as the target
 * for rendering the Evas_GL surface. This image object is set as the
 * widget's resize object.
 *
 * @param obj The Evas object.
 * @param priv Private data, unused in this function.
 */
EOLIAN static void
_elm_glview_efl_canvas_group_group_add(Eo *obj, Elm_Glview_Data *priv EINA_UNUSED)
{
   Evas_Object *img;

   // Create image to render Evas_GL Surface
   img = evas_object_image_filled_add(evas_object_evas_get(obj));
   elm_widget_resize_object_set(obj, img);
   evas_object_image_size_set(img, 1, 1);

   efl_canvas_group_add(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Constructor for the Elm_Glview object.
 *
 * Initializes the core components of the GLView, including Evas_GL,
 * a default GL configuration, initial policies (scale and render),
 * default dimensions, GLES version, and the GL context.
 * This function is called as part of the object instantiation process.
 *
 * @param obj The Evas object being constructed.
 * @param priv The private data structure for the GLView.
 */
static void
_elm_glview_constructor(Eo *obj, Elm_Glview_Data *priv)
{
   // Evas_GL
   priv->evasgl = evas_gl_new(evas_object_evas_get(obj));
   if (!priv->evasgl)
     {
        ERR("Failed Creating an Evas GL Object.");
        return;
     }

   // Create a default config
   priv->config = evas_gl_config_new();
   if (!priv->config)
     {
        ERR("Failed Creating a Config Object.");
        evas_gl_free(priv->evasgl);
        priv->evasgl = NULL;
        return;
     }
   priv->config->color_format = EVAS_GL_RGB_888;

   // Initialize variables
   priv->scale_policy = ELM_GLVIEW_RESIZE_POLICY_RECREATE;
   priv->render_policy = ELM_GLVIEW_RENDER_POLICY_ON_DEMAND;

   // Initialize it to (64,64)  (It's an arbitrary value)
   priv->w = 64;
   priv->h = 64;

   // Set context version
   if (!priv->gles_version)
     priv->gles_version = EVAS_GL_GLES_2_X;
   priv->config->gles_version = priv->gles_version;

   // Create Context
   if (priv->gles_version == EVAS_GL_GLES_2_X)
     priv->context = evas_gl_context_create(priv->evasgl, NULL);
   else
     priv->context = evas_gl_context_version_create(priv->evasgl, NULL, priv->gles_version);
   if ((!priv->context) || (!evas_gl_context_api_get(priv->evasgl, priv->context)))
     {
        ERR("Error Creating an Evas_GL Context.");
        ELM_SAFE_FREE(priv->config, evas_gl_config_free);
        ELM_SAFE_FREE(priv->evasgl, evas_gl_free);
        return;
     }
}

/**
 * @internal
 * @brief Called when the GLView object is being deleted from a canvas group.
 *
 * This function handles the cleanup of all resources associated with the GLView.
 * It calls the user-defined deletion callback, removes any pending render callbacks,
 * and frees all Evas_GL related resources (surface, context, config, and Evas_GL itself).
 *
 * @param obj The Evas object.
 * @param sd The private data of the GLView.
 */
EOLIAN static void
_elm_glview_efl_canvas_group_group_del(Eo *obj, Elm_Glview_Data *sd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   // Call delete func if it's registered
   if (sd->del_func)
     {
        evas_gl_make_current(sd->evasgl, sd->surface, sd->context);
        sd->del_func(obj);
     }
   //TODO:will be optimised
   efl_event_callback_legacy_call(obj, ELM_GLVIEW_EVENT_DESTROYED, NULL);

   efl_event_callback_del(efl_main_loop_get(),
                         EFL_LOOP_EVENT_IDLE_ENTER,
                         _render_cb,
                         obj);
   evas_gl_make_current(sd->evasgl, NULL, NULL);

   if (sd->surface)
     {
        evas_object_image_native_surface_set(wd->resize_obj, NULL);
        evas_gl_surface_destroy(sd->evasgl, sd->surface);
     }
   if (sd->context) evas_gl_context_destroy(sd->evasgl, sd->context);
   if (sd->config) evas_gl_config_free(sd->config);
   if (sd->evasgl) evas_gl_free(sd->evasgl);

   efl_canvas_group_del(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Callback invoked when an Efl_Event_Callback is added to the GLView.
 *
 * This function listens for specific event callback additions.
 * If a callback for ELM_GLVIEW_EVENT_CREATED is added, it resets the
 * `initialized` flag to ensure the init function is called again.
 * If a callback for ELM_GLVIEW_EVENT_RENDER is added, it ensures the
 * render policy callbacks are correctly set up.
 *
 * @param data User data associated with the callback, unused here.
 * @param ev The Efl_Event structure containing event information.
 */
static void
_cb_added(void *data EINA_UNUSED, const Efl_Event *ev)
{
   const Efl_Callback_Array_Item *event = ev->info;

   ELM_GLVIEW_DATA_GET(ev->object, sd);

   if (event->desc == ELM_GLVIEW_EVENT_CREATED)
     {
        sd->initialized = EINA_FALSE;
     }
   else if (event->desc == ELM_GLVIEW_EVENT_RENDER)
     {
        _set_render_policy_callback(ev->object);
     }
}

/**
 * @brief Adds a new GLView widget to the given parent evas object.
 *
 * This function creates a new GLView widget with a default GLES 2.X context.
 *
 * @param parent The parent object.
 * @return The new object or NULL if it cannot be created.
 */
EAPI Evas_Object *
elm_glview_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent,
                         elm_obj_glview_version_constructor(efl_added, EVAS_GL_GLES_2_X));
}

/**
 * @brief Adds a new GLView widget to the given parent evas object with a specific GLES version.
 *
 * @param parent The parent object.
 * @param version The GLES context version to use (e.g., EVAS_GL_GLES_2_X, EVAS_GL_GLES_3_X).
 *                If an invalid version is provided, it defaults to EVAS_GL_GLES_2_X.
 * @return The new object or NULL if it cannot be created.
 */
EAPI Evas_Object *
elm_glview_version_add(Evas_Object *parent, Evas_GL_Context_Version version)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(MY_CLASS, parent,
                         elm_obj_glview_version_constructor(efl_added, version));
}

/**
 * @internal
 * @brief Internal constructor helper that sets the GLES version.
 *
 * This function is called by elm_glview_add and elm_glview_version_add.
 * It sets the desired GLES version in the private data and then calls
 * the main _elm_glview_constructor. It also sets up legacy smart callbacks
 * and the access role.
 *
 * @param obj The Evas object.
 * @param sd The private data of the GLView.
 * @param version The GLES context version.
 */
EOLIAN static void
_elm_glview_version_constructor(Eo *obj, Elm_Glview_Data *sd,
                                Evas_GL_Context_Version version)
{
   sd->gles_version =
     ((version > 0) && (version <= 3)) ? version : EVAS_GL_GLES_2_X;
   _elm_glview_constructor(obj, sd);

   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_ANIMATION);
   efl_event_callback_add(obj, EFL_EVENT_CALLBACK_ADD, _cb_added, NULL);
}

/**
 * @internal
 * @brief Efl_Object constructor for GLView.
 *
 * Standard Efl_Object constructor. It handles legacy focus behavior
 * and calls the superclass constructor.
 *
 * @param obj The Evas object.
 * @param pd Private data, unused in this function.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object*
_elm_glview_efl_object_constructor(Eo *obj, Elm_Glview_Data *pd EINA_UNUSED)
{
   legacy_object_focus_handle(obj);
   return efl_constructor(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Efl_Object finalize step for GLView.
 *
 * This function is called during the finalization phase of object construction.
 * It checks if Evas_GL was successfully initialized. If not, it indicates an error.
 *
 * @param obj The Evas object.
 * @param sd The private data of the GLView.
 * @return The finalized Eo object, or NULL on critical failure (EvasGL not initialized).
 */
EOLIAN static Eo *
_elm_glview_efl_object_finalize(Eo *obj, Elm_Glview_Data *sd)
{
   if (!sd->evasgl)
     {
        ERR("Failed");
        return NULL;
     }

   return efl_finalize(efl_super(obj, MY_CLASS));
}

/**
 * @internal
 * @brief Retrieves the Evas_GL_API structure for the GLView.
 *
 * This allows access to the GL functions for the current context.
 *
 * @param obj The Evas object, unused in this function.
 * @param sd The private data of the GLView.
 * @return A pointer to the Evas_GL_API structure.
 */
EOLIAN static Evas_GL_API*
_elm_glview_gl_api_get(const Eo *obj EINA_UNUSED, Elm_Glview_Data *sd)
{
   return evas_gl_context_api_get(sd->evasgl, sd->context);
}

/**
 * @internal
 * @brief Sets the rendering mode for the GLView.
 *
 * Configures various aspects of the GL rendering pipeline, such as alpha channel,
 * depth buffer, stencil buffer, multisampling, and direct rendering options.
 * After configuring, it updates the GL surface.
 *
 * @param obj The Evas object.
 * @param sd The private data of the GLView.
 * @param mode A bitmask of Elm_GLView_Mode flags specifying the desired modes.
 *             Example: ELM_GLVIEW_ALPHA | ELM_GLVIEW_DEPTH_24 | ELM_GLVIEW_STENCIL_8
 * @return EINA_TRUE if the mode was set successfully and surface created, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_elm_glview_mode_set(Eo *obj, Elm_Glview_Data *sd, Elm_GLView_Mode mode)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);

   // Set the configs
   if (mode & ELM_GLVIEW_ALPHA) sd->config->color_format = EVAS_GL_RGBA_8888;
   else sd->config->color_format = EVAS_GL_RGB_888;

   if (mode & ELM_GLVIEW_DEPTH)
     {
        const int mask = 7 << 6;
        if ((mode & mask) == (ELM_GLVIEW_DEPTH_8 & mask))
          sd->config->depth_bits = EVAS_GL_DEPTH_BIT_8;
        else if ((mode & mask) == (ELM_GLVIEW_DEPTH_16 & mask))
          sd->config->depth_bits = EVAS_GL_DEPTH_BIT_16;
        else if ((mode & mask) == (ELM_GLVIEW_DEPTH_24 & mask))
          sd->config->depth_bits = EVAS_GL_DEPTH_BIT_24;
        else if ((mode & mask) == (ELM_GLVIEW_DEPTH_32 & mask))
          sd->config->depth_bits = EVAS_GL_DEPTH_BIT_32;
        else
          sd->config->depth_bits = EVAS_GL_DEPTH_BIT_24;
     }
   else
     sd->config->depth_bits = EVAS_GL_DEPTH_NONE;

   if (mode & ELM_GLVIEW_STENCIL)
     {
        const int mask = 7 << 9;
        if ((mode & mask) == (ELM_GLVIEW_STENCIL_1 & mask))
          sd->config->stencil_bits = EVAS_GL_STENCIL_BIT_1;
        else if ((mode & mask) == (ELM_GLVIEW_STENCIL_2 & mask))
          sd->config->stencil_bits = EVAS_GL_STENCIL_BIT_2;
        else if ((mode & mask) == (ELM_GLVIEW_STENCIL_4 & mask))
          sd->config->stencil_bits = EVAS_GL_STENCIL_BIT_4;
        else if ((mode & mask) == (ELM_GLVIEW_STENCIL_8 & mask))
          sd->config->stencil_bits = EVAS_GL_STENCIL_BIT_8;
        else if ((mode & mask) == (ELM_GLVIEW_STENCIL_16 & mask))
          sd->config->stencil_bits = EVAS_GL_STENCIL_BIT_16;
        else
          sd->config->stencil_bits = EVAS_GL_STENCIL_BIT_8;
     }
   else
     sd->config->stencil_bits = EVAS_GL_STENCIL_NONE;

   if (mode & ELM_GLVIEW_MULTISAMPLE_HIGH)
     {
        if ((mode & ELM_GLVIEW_MULTISAMPLE_HIGH) == ELM_GLVIEW_MULTISAMPLE_LOW)
          sd->config->multisample_bits = EVAS_GL_MULTISAMPLE_LOW;
        else if ((mode & ELM_GLVIEW_MULTISAMPLE_HIGH) == ELM_GLVIEW_MULTISAMPLE_MED)
          sd->config->multisample_bits = EVAS_GL_MULTISAMPLE_MED;
        else
          sd->config->multisample_bits = EVAS_GL_MULTISAMPLE_HIGH;
     }
   else
     sd->config->multisample_bits = EVAS_GL_MULTISAMPLE_NONE;

   sd->config->options_bits = EVAS_GL_OPTIONS_NONE;
   if (mode & ELM_GLVIEW_DIRECT)
     sd->config->options_bits = EVAS_GL_OPTIONS_DIRECT;
   if (mode & ELM_GLVIEW_CLIENT_SIDE_ROTATION)
     sd->config->options_bits |= EVAS_GL_OPTIONS_CLIENT_SIDE_ROTATION;

   // Check for Alpha Channel and enable it
   if (mode & ELM_GLVIEW_ALPHA)
     evas_object_image_alpha_set(wd->resize_obj, EINA_TRUE);
   else
     evas_object_image_alpha_set(wd->resize_obj, EINA_FALSE);

   sd->mode = mode;
   sd->warned_about_dr = EINA_FALSE;

   _glview_update_surface(obj);
   if (!sd->surface)
     {
        ERR("Failed to create a surface with the requested configuration.");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Sets the resize policy for the GLView.
 *
 * Determines how the GLView responds to size changes:
 * - ELM_GLVIEW_RESIZE_POLICY_RECREATE: The GL surface is destroyed and recreated with the new size.
 * - ELM_GLVIEW_RESIZE_POLICY_SCALE: The GL surface is scaled (not typically used directly by GLView,
 *   but the image object it renders to might scale).
 *
 * @param obj The Evas object.
 * @param sd The private data of the GLView.
 * @param policy The desired resize policy.
 *               Example: ELM_GLVIEW_RESIZE_POLICY_RECREATE
 * @return EINA_TRUE on success, EINA_FALSE if the policy is invalid.
 */
EOLIAN static Eina_Bool
_elm_glview_resize_policy_set(Eo *obj, Elm_Glview_Data *sd, Elm_GLView_Resize_Policy policy)
{
   if (policy == sd->scale_policy) return EINA_TRUE;
   switch (policy)
     {
      case ELM_GLVIEW_RESIZE_POLICY_RECREATE:
      case ELM_GLVIEW_RESIZE_POLICY_SCALE:
        sd->scale_policy = policy;
        _glview_update_surface(obj);
        elm_obj_glview_draw_request(obj);
        return EINA_TRUE;

      default:
        ERR("Invalid Scale Policy.");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Sets the rendering policy for the GLView.
 *
 * Determines when the GLView's content is rendered:
 * - ELM_GLVIEW_RENDER_POLICY_ON_DEMAND: Rendering occurs only when explicitly requested
 *   (e.g., via elm_obj_glview_draw_request()) or when Evas needs the pixels.
 * - ELM_GLVIEW_RENDER_POLICY_ALWAYS: Rendering occurs continuously, typically on every idle loop.
 *
 * @param obj The Evas object.
 * @param sd The private data of the GLView.
 * @param policy The desired render policy.
 *               Example: ELM_GLVIEW_RENDER_POLICY_ALWAYS
 * @return EINA_TRUE on success, EINA_FALSE if the policy is invalid.
 */
EOLIAN static Eina_Bool
_elm_glview_render_policy_set(Eo *obj, Elm_Glview_Data *sd, Elm_GLView_Render_Policy policy)
{
   if ((policy != ELM_GLVIEW_RENDER_POLICY_ON_DEMAND) &&
       (policy != ELM_GLVIEW_RENDER_POLICY_ALWAYS))
     {
        ERR("Invalid Render Policy.");
        return EINA_FALSE;
     }

   if (sd->render_policy == policy) return EINA_TRUE;

   sd->warned_about_dr = EINA_FALSE;
   sd->render_policy = policy;
   _set_render_policy_callback(obj);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Sets the internal view size of the GLView's surface.
 *
 * This directly sets the dimensions (sd->w, sd->h) for the GL surface,
 * updates the surface, and requests a redraw. This is different from
 * _elm_glview_efl_gfx_entity_size_set which handles widget sizing and policies.
 *
 * @param obj The Evas object.
 * @param sd The private data of the GLView.
 * @param sz The new view size (width and height).
 */
EOLIAN static void
_elm_glview_efl_gfx_view_view_size_set(Eo *obj, Elm_Glview_Data *sd, Eina_Size2D sz)
{
   if ((sz.w == sd->w) && (sz.h == sd->h)) return;

   sd->w = sz.w;
   sd->h = sz.h;

   _glview_update_surface(obj);
   elm_obj_glview_draw_request(obj);
}

/**
 * @internal
 * @brief Gets the internal view size of the GLView's surface.
 *
 * @param obj The Evas object, unused in this function.
 * @param sd The private data of the GLView.
 * @return The current view size (Eina_Size2D) of the GL surface.
 */
EOLIAN static Eina_Size2D
_elm_glview_efl_gfx_view_view_size_get(const Eo *obj EINA_UNUSED, Elm_Glview_Data *sd)
{
   return EINA_SIZE2D(sd->w, sd->h);
}

/**
 * @internal
 * @brief Requests a redraw of the GLView content.
 *
 * Marks the underlying image object's pixels as dirty, which will trigger
 * a call to the rendering callback (_render_cb) when Evas processes updates
 * or when the pixel get callback is invoked (for on-demand rendering).
 * If the render policy is ELM_GLVIEW_RENDER_POLICY_ALWAYS and an idle enterer
 * is not already active, it adds one to ensure continuous rendering.
 *
 * @param obj The Evas object.
 * @param sd The private data of the GLView.
 */
EOLIAN static void
_elm_glview_draw_request(Eo *obj, Elm_Glview_Data *sd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_image_pixels_dirty_set(wd->resize_obj, EINA_TRUE);
   evas_object_image_data_update_add(wd->resize_obj, 0, 0, sd->w, sd->h);
   if (sd->render_policy == ELM_GLVIEW_RENDER_POLICY_ALWAYS &&
       !sd->render_idle_enterer)
     sd->render_idle_enterer = efl_event_callback_priority_add(efl_main_loop_get(),
                                                              EFL_LOOP_EVENT_IDLE_ENTER,
                                                              EFL_CALLBACK_PRIORITY_BEFORE,
                                                              _render_cb, obj);
}

/**
 * @internal
 * @brief Retrieves the Evas_GL context associated with the GLView.
 *
 * @param obj The Evas object, unused in this function.
 * @param sd The private data of the GLView.
 * @return The Evas_GL context.
 */
EOLIAN static Evas_GL *
_elm_glview_evas_gl_get(const Eo *obj EINA_UNUSED, Elm_Glview_Data *sd)
{
   return sd->evasgl;
}

/**
 * @internal
 * @brief Retrieves the current rotation of the Evas_GL surface.
 *
 * @param obj The Evas object, unused in this function.
 * @param sd The private data of the GLView.
 * @return The rotation angle (0, 90, 180, or 270).
 */
EOLIAN static int
_elm_glview_rotation_get(const Eo *obj EINA_UNUSED, Elm_Glview_Data *sd)
{
   return evas_gl_rotation_get(sd->evasgl);
}

/**
 * @internal
 * @brief Class constructor for Elm_Glview.
 *
 * Registers the legacy smart type name for the GLView class.
 *
 * @param klass The Efl_Class being constructed.
 */
static void
_elm_glview_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/* Legacy deprecated functions */

/**
 * @brief Notify that the GLView content has changed and needs a redraw.
 * @deprecated Use elm_obj_glview_draw_request() instead.
 * @param obj The GLView object.
 */
EAPI void
elm_glview_changed_set(Evas_Object *obj)
{
   ELM_GLVIEW_CHECK(obj);

   elm_obj_glview_draw_request(obj);
}

/**
 * @brief Get the size of the GLView's rendering surface.
 * @deprecated Use efl_gfx_view_size_get() instead.
 * @param obj The GLView object.
 * @param[out] w Pointer to store the width.
 * @param[out] h Pointer to store the height.
 */
EAPI void
elm_glview_size_get(const Elm_Glview *obj, int *w, int *h)
{
   Eina_Size2D sz;
   sz = efl_gfx_view_size_get(obj);
   if (w) *w = sz.w;
   if (h) *h = sz.h;
}

/**
 * @brief Set the size of the GLView's rendering surface.
 * @deprecated Use efl_gfx_view_size_set() instead.
 * @param obj The GLView object.
 * @param w The width to set.
 * @param h The height to set.
 */
EAPI void
elm_glview_size_set(Elm_Glview *obj, int w, int h)
{
   efl_gfx_view_size_set(obj, EINA_SIZE2D(w, h));
}

/**
 * @brief Set the initialization callback for the GLView.
 * @deprecated Use efl_event_callback_add with ELM_GLVIEW_EVENT_CREATED instead.
 * @param obj The GLView object.
 * @param func The callback function for GL initialization.
 */
EAPI void
elm_glview_init_func_set(Elm_Glview *obj, Elm_GLView_Func_Cb func)
{
   ELM_GLVIEW_CHECK(obj);
   ELM_GLVIEW_DATA_GET(obj, sd);

   sd->initialized = EINA_FALSE;
   sd->init_func = func;
}

/**
 * @brief Set the deletion callback for the GLView.
 * @deprecated Use efl_event_callback_add with ELM_GLVIEW_EVENT_DESTROYED instead.
 * @param obj The GLView object.
 * @param func The callback function for GL resource cleanup.
 */
EAPI void
elm_glview_del_func_set(Elm_Glview *obj, Elm_GLView_Func_Cb func)
{
   ELM_GLVIEW_CHECK(obj);
   ELM_GLVIEW_DATA_GET(obj, sd);

   sd->del_func = func;
}

/**
 * @brief Set the resize callback for the GLView.
 * @deprecated Use efl_event_callback_add with ELM_GLVIEW_EVENT_RESIZED instead.
 * @param obj The GLView object.
 * @param func The callback function for handling GL viewport resizing.
 */
EAPI void
elm_glview_resize_func_set(Elm_Glview *obj, Elm_GLView_Func_Cb func)
{
   ELM_GLVIEW_CHECK(obj);
   ELM_GLVIEW_DATA_GET(obj, sd);

   sd->resize_func = func;
}

/**
 * @brief Set the rendering callback for the GLView.
 * @deprecated Use efl_event_callback_add with ELM_GLVIEW_EVENT_RENDER instead.
 * @param obj The GLView object.
 * @param func The callback function for GL rendering.
 */
EAPI void
elm_glview_render_func_set(Elm_Glview *obj, Elm_GLView_Func_Cb func)
{
   ELM_GLVIEW_CHECK(obj);
   ELM_GLVIEW_DATA_GET(obj, sd);

   sd->render_func = func;
   _set_render_policy_callback(obj);
}

/* Internal EO APIs and hidden overrides */

#define ELM_GLVIEW_EXTRA_OPS \
   EFL_CANVAS_GROUP_ADD_DEL_OPS(elm_glview)

#include "elm_glview_eo.c"
