#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED

#include <Elementary.h>

#include "../../static_libs/buildsystem/buildsystem.h"
#include "elm_priv.h"
#include "elm_widget_web.h"

#define MY_CLASS elm_web_class_get()

#define MY_CLASS_NAME "Elm_Web"
#define MY_CLASS_NAME_LEGACY "elm_web"

/**
 * @internal
 * @brief Structure to hold function pointers and data for a loaded web engine module.
 *
 * This allows Elementary to interact with different web rendering engines
 * (like WebKit-EFL) through a common interface. The actual functions are
 * loaded dynamically from a shared module.
 */
typedef struct _Elm_Web_Module Elm_Web_Module;
struct _Elm_Web_Module
{
   void (*unneed_web)(void); /**< Function pointer for uninitializing the web engine module. */
   Eina_Bool (*need_web)(void); /**< Function pointer for initializing the web engine module. */

   void (*window_features_ref)(Elm_Web_Window_Features *wf); /**< Function pointer to reference window features. */
   void (*window_features_unref)(Elm_Web_Window_Features *wf); /**< Function pointer to unreference window features. */
   /**
    * @brief Function pointer to get a boolean property of window features.
    * @param wf The window features object.
    * @param flag The specific feature flag to query.
    * @return EINA_TRUE if the feature is enabled, EINA_FALSE otherwise.
    */
   Eina_Bool (*window_features_property_get)(const Elm_Web_Window_Features *wf,
                                             Elm_Web_Window_Feature_Flag flag);
   /**
    * @brief Function pointer to get the region (geometry) of window features.
    * @param wf The window features object.
    * @param x Pointer to store the x-coordinate.
    * @param y Pointer to store the y-coordinate.
    * @param w Pointer to store the width.
    * @param h Pointer to store the height.
    */
   void (*window_features_region_get)(const Elm_Web_Window_Features *wf,
                                      Evas_Coord *x,
                                      Evas_Coord *y,
                                      Evas_Coord *w,
                                      Evas_Coord *h);

   const Efl_Class *(*class_get)(void); /**< Function pointer to get the Efl_Class of the web widget implementation from the module. */

   Eina_Module *m; /**< Handle to the loaded Eina_Module. */
};

/**
 * @internal
 * @brief Global static instance holding the currently loaded web module's functions and data.
 */
static Elm_Web_Module ewm = {
  NULL,
  NULL,

  NULL,
  NULL,
  NULL,
  NULL,

  NULL,

  NULL
};

static const char SIG_URI_CHANGED[] = "uri,changed"; // deprecated, use "url,changed" instead.
static const char SIG_URL_CHANGED[] = "url,changed";

static const Evas_Smart_Cb_Description _elm_web_smart_callbacks[] = {
   { SIG_URI_CHANGED, "s" },
   { SIG_URL_CHANGED, "s" },
   { SIG_WIDGET_FOCUSED, ""}, /**< handled by elm_widget */
   { SIG_WIDGET_UNFOCUSED, ""}, /**< handled by elm_widget */
   { NULL, NULL }
};

// FIXME: init/shutdown module below

/**
 * @internal
 * @brief Calls the unneed function of the currently loaded web module, if available.
 *
 * This is typically used during shutdown or when the web functionality is no longer required.
 */
void
_elm_unneed_web(void)
{
   if (!ewm.unneed_web) return ;
   ewm.unneed_web();
}

/**
 * @brief Ensures that the web engine module is loaded and initialized.
 *
 * This function attempts to load the web engine module if it hasn't been loaded yet,
 * or calls the `need_web` function of an already loaded module.
 *
 * @return @c EINA_TRUE if the web functionality is available and initialized,
 *         @c EINA_FALSE otherwise.
 * @see _elm_web_init()
 */
EAPI Eina_Bool
elm_need_web(void)
{
   if (!ewm.need_web) return EINA_FALSE;
   return ewm.need_web();
}

/**
 * @brief Adds a new web widget to a parent Evas object.
 *
 * This function creates a new web widget instance using the Efl class
 * provided by the currently loaded web engine module.
 *
 * @param parent The parent Evas object.
 * @return A new Evas_Object for the web widget, or @c NULL on failure (e.g., if
 *         the parent is invalid or the web module/class isn't loaded).
 *
 * @see elm_legacy_add()
 * @see elm_web_real_class_get()
 */
EAPI Evas_Object *
elm_web_add(Evas_Object *parent)
{
   if (!parent || !ewm.class_get) return NULL;

   return elm_legacy_add(ewm.class_get(), parent);
}

/**
 * @brief Gets the Efl_Class for the web widget implementation.
 *
 * This function returns the Efl_Class object provided by the currently
 * loaded web engine module. This class is used to instantiate web widgets.
 *
 * @return The Efl_Class for the web widget, or @c NULL if the web module
 *         or its class getter function is not loaded.
 */
EAPI const Efl_Class *
elm_web_real_class_get(void)
{
   if (!ewm.class_get) return NULL;

   return ewm.class_get();
}

/**
 * @internal
 * @brief Efl object constructor for Elm_Web.
 *
 * Initializes the Elm_Web object, sets its legacy type name,
 * registers smart callbacks, sets the accessibility role, and
 * enables legacy focus handling.
 *
 * @param obj The Eo object to construct.
 * @param sd The Elm_Web_Data (private data) for this object.
 * @return The constructed Eo object.
 */
EOLIAN static Eo *
_elm_web_efl_object_constructor(Eo *obj, Elm_Web_Data *sd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   sd->obj = obj;
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   evas_object_smart_callbacks_descriptions_set(obj, _elm_web_smart_callbacks);
   efl_access_object_role_set(obj, EFL_ACCESS_ROLE_HTML_CONTAINER);
   legacy_object_focus_handle(obj);
   return obj;
}

EAPI Eina_Bool
elm_web_uri_set(Evas_Object *obj, const char *url)
{
   ELM_WEB_CHECK(obj) EINA_FALSE;

   return elm_obj_web_url_set(obj, url);
}

EAPI const char *
elm_web_uri_get(const Evas_Object *obj)
{
   return elm_obj_web_url_get((Eo *) obj);
}

// FIXME: override with module function
/**
 * @brief References (increments the reference count of) window features.
 *
 * This function delegates to the `window_features_ref` function pointer
 * from the loaded web module, if available.
 *
 * @param wf Pointer to the Elm_Web_Window_Features structure to reference.
 */
EAPI void
elm_web_window_features_ref(Elm_Web_Window_Features *wf)
{
   if (!ewm.window_features_ref) return ;
   ewm.window_features_ref(wf);
}

/**
 * @brief Unreferences (decrements the reference count of) window features.
 *
 * This function delegates to the `window_features_unref` function pointer
 * from the loaded web module, if available. If the reference count reaches zero,
 * the features might be freed.
 *
 * @param wf Pointer to the Elm_Web_Window_Features structure to unreference.
 */
EAPI void
elm_web_window_features_unref(Elm_Web_Window_Features *wf)
{
   if (!ewm.window_features_unref) return ;
   ewm.window_features_unref(wf);
}

/**
 * @brief Gets a boolean property of the given window features.
 *
 * This function delegates to the `window_features_property_get` function pointer
 * from the loaded web module, if available.
 *
 * @param wf Pointer to the Elm_Web_Window_Features structure.
 * @param flag The Elm_Web_Window_Feature_Flag to query (e.g., toolbar visibility,
 *        scrollbars visibility).
 * @return @c EINA_TRUE if the feature is enabled/present, @c EINA_FALSE otherwise or on error.
 */
EAPI Eina_Bool
elm_web_window_features_property_get(const Elm_Web_Window_Features *wf,
                                     Elm_Web_Window_Feature_Flag flag)
{
   if (!ewm.window_features_property_get) return EINA_FALSE;
   return ewm.window_features_property_get(wf, flag);
}

/**
 * @brief Gets the region (position and size) of the given window features.
 *
 * This function delegates to the `window_features_region_get` function pointer
 * from the loaded web module, if available. If not available, or if parameters
 * are NULL, output coordinates are set to 0.
 *
 * @param wf Pointer to the Elm_Web_Window_Features structure.
 * @param[out] x Pointer to store the x-coordinate of the region. Can be @c NULL.
 * @param[out] y Pointer to store the y-coordinate of the region. Can be @c NULL.
 * @param[out] w Pointer to store the width of the region. Can be @c NULL.
 * @param[out] h Pointer to store the height of the region. Can be @c NULL.
 */
EAPI void
elm_web_window_features_region_get(const Elm_Web_Window_Features *wf,
                                   Evas_Coord *x,
                                   Evas_Coord *y,
                                   Evas_Coord *w,
                                   Evas_Coord *h)
{
   if (x) *x = 0;
   if (y) *y = 0;
   if (w) *w = 0;
   if (h) *h = 0;

   if (!ewm.window_features_region_get) return;
   ewm.window_features_region_get(wf, x, y, w, h);
}

/**
 * @internal
 * @brief Converts between legacy Elm_Web_Zoom_Mode and Efl_Ui_Zoom_Mode.
 *
 * This helper function is used to maintain compatibility between the older
 * Elm_Web API and the newer Efl_Ui_Zoom interface.
 *
 * @param[in,out] legacy_mode Pointer to the Elm_Web_Zoom_Mode variable.
 *                            If @p to_legacy is @c EINA_TRUE, this is an output.
 *                            Otherwise, it's an input.
 * @param[in,out] mode Pointer to the Efl_Ui_Zoom_Mode variable.
 *                     If @p to_legacy is @c EINA_FALSE, this is an output.
 *                     Otherwise, it's an input.
 * @param to_legacy If @c EINA_TRUE, converts from @p mode to @p legacy_mode.
 *                  If @c EINA_FALSE, converts from @p legacy_mode to @p mode.
 */
static inline void
_convert_web_zoom_mode(Elm_Web_Zoom_Mode *legacy_mode, Efl_Ui_Zoom_Mode *mode, Eina_Bool to_legacy)
{
   #define CONVERT(LEGACY_MODE, NEW_MODE) \
      if (to_legacy  && *mode == NEW_MODE) \
        { \
           *legacy_mode =LEGACY_MODE; \
           return; \
        } \
      if (!to_legacy && *legacy_mode == LEGACY_MODE) \
        { \
           *mode = NEW_MODE; \
           return; \
        } \

   CONVERT(ELM_WEB_ZOOM_MODE_MANUAL,    EFL_UI_ZOOM_MODE_MANUAL)
   CONVERT(ELM_WEB_ZOOM_MODE_AUTO_FIT,  EFL_UI_ZOOM_MODE_AUTO_FIT)
   CONVERT(ELM_WEB_ZOOM_MODE_AUTO_FILL, EFL_UI_ZOOM_MODE_AUTO_FILL)
   CONVERT(ELM_WEB_ZOOM_MODE_LAST,      EFL_UI_ZOOM_MODE_LAST)
   CONVERT(ELM_WEB_ZOOM_MODE_LAST,      EFL_UI_ZOOM_MODE_AUTO_FIT_IN)

   #undef CONVERT
}

EAPI void
elm_web_zoom_mode_set(Evas_Object *obj, Elm_Web_Zoom_Mode mode)
{
   Efl_Ui_Zoom_Mode new_mode = EFL_UI_ZOOM_MODE_MANUAL;;

   _convert_web_zoom_mode(&mode, &new_mode, EINA_FALSE);

   efl_ui_zoom_mode_set(obj, new_mode);
}

EAPI Elm_Web_Zoom_Mode
elm_web_zoom_mode_get(const Evas_Object *obj)
{
   Efl_Ui_Zoom_Mode new_mode = efl_ui_zoom_mode_get(obj);;
   Elm_Web_Zoom_Mode mode = ELM_WEB_ZOOM_MODE_MANUAL;

   _convert_web_zoom_mode(&mode, &new_mode, EINA_TRUE);

   return mode;
}

EAPI void
elm_web_zoom_set(Evas_Object *obj, double zoom)
{
   efl_ui_zoom_level_set(obj, zoom);
}

EAPI double
elm_web_zoom_get(const Evas_Object *obj)
{
   return efl_ui_zoom_level_get(obj);
}

static void
_elm_web_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @internal
 * @brief Platform-dependent shared library extension.
 */
#if defined(_WIN32) || defined(__CYGWIN__)
# define EFL_SHARED_EXTENSION ".dll"
#else
# define EFL_SHARED_EXTENSION ".so"
#endif

/**
 * @internal
 * @brief Initializes the Elm_Web subsystem by loading a specific web engine module.
 *
 * This function attempts to locate and load a shared library module corresponding
 * to the specified @p engine name (e.g., "ewk", "webkit1"). It resolves function
 * pointers from the loaded module and stores them in the global `ewm` struct.
 *
 * If a module is already loaded, and a different engine is requested, the old
 * module's function pointers (except for `class_get`) are cleared, and the new
 * module is loaded. This is done with a purposeful leak of the old module handle
 * to prevent issues with potential lingering state in the unloaded engine.
 *
 * @param engine A string identifying the web engine to load (e.g., "ewk").
 *               If NULL or an empty string, it might try a default engine.
 * @return @c EINA_TRUE if the engine module was successfully loaded and the
 *         mandatory `ewm_class_get` symbol was found. @c EINA_FALSE otherwise.
 *
 * @note The function searches for modules in standard Elementary module paths
 *       and also tries a path constructed using `_elm_lib_dir`.
 * @see eina_module_new()
 * @see eina_module_load()
 * @see eina_module_symbol_get()
 */
Eina_Bool
_elm_web_init(const char *engine)
{
   char buf[PATH_MAX];

     if (!bs_mod_get(buf, sizeof(buf), "elementary/web", engine))
       snprintf(buf, sizeof(buf),
                "%s/elementary/modules/web/%s/%s/module"EFL_SHARED_EXTENSION,
                _elm_lib_dir, engine, MODULE_ARCH);

   if (ewm.m)
     {
        // Check if the module is already open
        if (!strcmp(buf, eina_module_file_get(ewm.m)))
          return EINA_TRUE;

        // We are leaking reference on purpose here, as we can't be sure that
        // the web engine is not leaking state around preventing a clean exit.
        // Only future elm_web object created from now will use the new engine.
        ewm.unneed_web = NULL;
        ewm.need_web = NULL;
        ewm.window_features_ref = NULL;
        ewm.window_features_unref = NULL;
        ewm.window_features_property_get = NULL;
        ewm.window_features_region_get = NULL;
        ewm.class_get = NULL;
     }

   ewm.m = eina_module_new(buf);
   if (!ewm.m) return EINA_FALSE;

   if (!eina_module_load(ewm.m))
     {
        eina_module_free(ewm.m);
        ewm.m = NULL;
        return EINA_FALSE;
     }

   ewm.unneed_web = eina_module_symbol_get(ewm.m, "ewm_unneed_web");
   ewm.need_web = eina_module_symbol_get(ewm.m, "ewm_need_web");
   ewm.window_features_ref = eina_module_symbol_get(ewm.m, "ewm_window_features_ref");
   ewm.window_features_unref = eina_module_symbol_get(ewm.m, "ewm_window_features_unref");
   ewm.window_features_property_get = eina_module_symbol_get(ewm.m, "ewm_window_features_property_get");
   ewm.window_features_region_get = eina_module_symbol_get(ewm.m, "ewm_window_features_region_get");
   ewm.class_get = eina_module_symbol_get(ewm.m, "ewm_class_get");

   // Only the class_get is mandatory
   if (!ewm.class_get) return EINA_FALSE;
   return EINA_TRUE;
}

#undef ELM_WEB_CLASS
#define ELM_WEB_CLASS elm_web_class_get()

#include "elm_web_eo.c"
