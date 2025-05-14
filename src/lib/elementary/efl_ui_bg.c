/**
 * @file
 * @brief These routines are for the Efl Ui Bg widget.
 *
 * The Background widget is a very simple one. It's intended to be a
 * static image, plain rectangle or a scalable image that is placed on
 * the background of a window or container object. It can be set to
 * scale, center or tile the image.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define ELM_LAYOUT_PROTECTED

#include <Elementary.h>
#include "elm_priv.h"
#include "efl_ui_bg_private.h"

#define MY_CLASS EFL_UI_BG_CLASS
#define MY_CLASS_PFX efl_ui_bg

#define MY_CLASS_NAME "Efl.Ui.Bg"

/**
 * @brief Describes the part aliases for the bg widget.
 * @details This array maps user-facing part names to internal Edje part names.
 *          - "overlay": Maps to "elm.swallow.content", typically used for overlaying content.
 */
static const Elm_Layout_Part_Alias_Description _content_aliases[] =
{
   {"overlay", "elm.swallow.content"},
   {NULL, NULL}
};

/**
 * @internal
 * @brief Constructor for the Efl.Ui.Bg object.
 * @param[in] obj The Efl.Ui.Bg object to construct.
 * @param[in] pd The private data for the Efl.Ui.Bg object.
 * @return The constructed Efl.Ui.Bg object, or @c NULL on failure.
 */
EOLIAN static Eo *
_efl_ui_bg_efl_object_constructor(Eo *obj, Efl_Ui_Bg_Data *pd)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   if (!elm_widget_theme_klass_get(obj))
     elm_widget_theme_klass_set(obj, "bg");

   obj = efl_constructor(efl_super(obj, MY_CLASS));
   elm_widget_can_focus_set(obj, EINA_FALSE);

   if (elm_widget_theme_object_set(obj, wd->resize_obj,
                                       elm_widget_theme_klass_get(obj),
                                       elm_widget_theme_element_get(obj),
                                       elm_widget_theme_style_get(obj)) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     CRI("Failed to set layout!");

   if (elm_widget_is_legacy(obj))
     {
        pd->rect = efl_add(EFL_CANVAS_RECTANGLE_CLASS, obj,
                           efl_gfx_color_set(efl_added, 0, 0, 0, 0),
                           efl_content_set(efl_part(obj, "elm.swallow.rectangle"), efl_added));

        pd->img = elm_image_add(obj);
        efl_gfx_image_scale_method_set(pd->img, EFL_GFX_IMAGE_SCALE_METHOD_EXPAND);
        elm_layout_content_set(obj, "elm.swallow.background", pd->img);
     }
   else
     {
        pd->rect = efl_add(EFL_CANVAS_RECTANGLE_CLASS, obj,
                           efl_gfx_color_set(efl_added, 0, 0, 0, 0),
                           efl_content_set(efl_part(obj, "efl.rectangle"), efl_added));

        pd->img = efl_add(EFL_UI_IMAGE_CLASS, obj,
                          efl_gfx_image_scale_method_set(efl_added, EFL_GFX_IMAGE_SCALE_METHOD_EXPAND),
                          efl_content_set(efl_part(obj, "efl.background"), efl_added));
     }
   pd->file = NULL;
   pd->key = NULL;

   efl_access_object_access_type_set(obj, EFL_ACCESS_TYPE_DISABLED);

   efl_ui_widget_focus_allow_set(obj, EINA_FALSE);

   efl_composite_attach(obj, pd->img);

   return obj;
}

/**
 * @internal
 * @brief Destructor for the Efl.Ui.Bg object.
 * @param[in] obj The Efl.Ui.Bg object to destruct.
 * @param[in] sd The private data for the Efl.Ui.Bg object.
 */
EOLIAN static void
_efl_ui_bg_efl_object_destructor(Eo *obj, Efl_Ui_Bg_Data *sd)
{
   ELM_SAFE_FREE(sd->file, eina_stringshare_del);
   ELM_SAFE_FREE(sd->key, eina_stringshare_del);

   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Sets the mode of display for a given background object.
 * @param[in] obj The Evas_Object (bg widget) to set the option for.
 * @param[in] option The desired background option.
 * @see Elm_Bg_Option
 * @ingroup Elm_Bg_Group
 */
EAPI void
elm_bg_option_set(Evas_Object *obj, Elm_Bg_Option option)
{
   Efl_Gfx_Image_Scale_Method type;

   switch (option)
     {
       case ELM_BG_OPTION_CENTER:
         type = EFL_GFX_IMAGE_SCALE_METHOD_NONE;
         break;
       case ELM_BG_OPTION_SCALE:
         type = EFL_GFX_IMAGE_SCALE_METHOD_EXPAND;
         break;
       case ELM_BG_OPTION_TILE:
         type = EFL_GFX_IMAGE_SCALE_METHOD_TILE;
         break;
       case ELM_BG_OPTION_STRETCH:
         type = EFL_GFX_IMAGE_SCALE_METHOD_FILL;
         break;
       case ELM_BG_OPTION_LAST:
       default:
         type = EFL_GFX_IMAGE_SCALE_METHOD_EXPAND;
     }
   efl_gfx_image_scale_method_set(obj, type);
}

/**
 * @brief Gets the mode of display for a given background object.
 * @param[in] obj The Evas_Object (bg widget) to get the option from.
 * @return The background option.
 * @see Elm_Bg_Option
 * @ingroup Elm_Bg_Group
 */
EAPI Elm_Bg_Option
elm_bg_option_get(const Evas_Object *obj)
{
   Efl_Gfx_Image_Scale_Method type;
   Elm_Bg_Option option = ELM_BG_OPTION_LAST;

   type = efl_gfx_image_scale_method_get(obj);
   switch (type)
     {
       case EFL_GFX_IMAGE_SCALE_METHOD_NONE:
         option = ELM_BG_OPTION_CENTER;
        break;
       case EFL_GFX_IMAGE_SCALE_METHOD_EXPAND:
         option = ELM_BG_OPTION_SCALE;
        break;
       case EFL_GFX_IMAGE_SCALE_METHOD_TILE:
         option = ELM_BG_OPTION_TILE;
         break;
       case EFL_GFX_IMAGE_SCALE_METHOD_FILL:
         option = ELM_BG_OPTION_STRETCH;
         break;
       case EFL_GFX_IMAGE_SCALE_METHOD_FIT:
       default:
         ERR("Scale type %d cannot be converted to Elm_Bg_Option", type);
         break;
     }

   return option;
}

/**
 * @brief Sets the color on a given background object.
 * @details This function sets the color of the rectangle used as background.
 *          The color is visible only if no image is set or if the image
 *          has transparent parts.
 *          If r, g, and b are all -1, the color is reset (transparent).
 *
 * @param[in] obj The Evas_Object (bg widget) to set the color for.
 * @param[in] r The red component of the color (0-255).
 * @param[in] g The green component of the color (0-255).
 * @param[in] b The blue component of the color (0-255).
 * @ingroup Elm_Bg_Group
 */
EAPI void
elm_bg_color_set(Evas_Object *obj,
                 int r,
                 int g,
                 int b)
{
   int a = 255;
   EFL_UI_BG_DATA_GET_OR_RETURN(obj, sd);

   // reset color
   if ((r == -1) && (g == -1) && (b == -1))
   {
      r = g = b = a = 0;
   }
   efl_gfx_color_set(sd->rect, r, g, b, a);
}

/**
 * @internal
 * @brief Sets the color of the background object.
 * @param[in] obj The Efl.Ui.Bg object.
 * @param[in] sd The private data for the Efl.Ui.Bg object.
 * @param[in] r Red component (0-255).
 * @param[in] g Green component (0-255).
 * @param[in] b Blue component (0-255).
 * @param[in] a Alpha component (0-255).
 */
EOLIAN static void
_efl_ui_bg_efl_gfx_color_color_set(Eo *obj, Efl_Ui_Bg_Data *sd, int r, int g, int b, int a)
{
   efl_gfx_color_set(efl_super(obj, MY_CLASS), r, g, b, a);
   efl_gfx_color_set(sd->rect, r, g, b, a);
}

/**
 * @brief Gets the color on a given background object.
 * @param[in] obj The Evas_Object (bg widget) to get the color from.
 * @param[out] r Pointer to store the red component.
 * @param[out] g Pointer to store the green component.
 * @param[out] b Pointer to store the blue component.
 * @ingroup Elm_Bg_Group
 */
EAPI void
elm_bg_color_get(const Evas_Object *obj,
                 int *r,
                 int *g,
                 int *b)
{
   EFL_UI_BG_CHECK(obj);
   efl_gfx_color_get((Eo *) obj, r, g, b, NULL);
}

/**
 * @internal
 * @brief Gets the color of the background object.
 * @param[in] obj The Efl.Ui.Bg object.
 * @param[in] sd The private data for the Efl.Ui.Bg object.
 * @param[out] r Pointer to store the red component.
 * @param[out] g Pointer to store the green component.
 * @param[out] b Pointer to store the blue component.
 * @param[out] a Pointer to store the alpha component.
 */
EOLIAN static void
_efl_ui_bg_efl_gfx_color_color_get(const Eo *obj, Efl_Ui_Bg_Data *sd, int *r, int *g, int *b, int *a)
{
   if (!sd->rect)
     efl_gfx_color_get(efl_super(obj, MY_CLASS), r, g, b, a);
   else
     efl_gfx_color_get(sd->rect, r, g, b, a);
}

/**
 * @brief Sets the size of the pixmap used for background.
 * @details This function is used to set the size of the image to be loaded from disk.
 *          It is useful when the image file is an SVG file or any other scalable
 *          format. For non-scalable formats like PNG, JPG, this function has no effect.
 *
 * @param[in] obj The Evas_Object (bg widget) to set the load size for.
 * @param[in] w The width of the image.
 * @param[in] h The height of the image.
 * @ingroup Elm_Bg_Group
 */
EAPI void
elm_bg_load_size_set(Evas_Object *obj, int w, int h)
{
   EFL_UI_BG_DATA_GET_OR_RETURN(obj, sd);
   efl_gfx_image_load_controller_load_size_set(sd->img, EINA_SIZE2D(w, h));
}

/**
 * @brief Sets the file (and group) to be used as background.
 * @details This function sets a new image file for the background object.
 *          If the file is an Edje file, the group to be used can also be specified.
 *
 * @param[in] obj The Evas_Object (bg widget) to set the file for.
 * @param[in] file The path to the image or Edje file.
 * @param[in] group The Edje group name (if file is an Edje file), or @c NULL.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 * @ingroup Elm_Bg_Group
 */
EAPI Eina_Bool
elm_bg_file_set(Eo *obj, const char *file, const char *group)
{
   EFL_UI_BG_DATA_GET_OR_RETURN_VAL(obj, sd, EINA_FALSE);
   Eina_Bool ret = efl_file_simple_load((Eo *) obj, file, group);
   if (ret) elm_image_preload_disabled_set(sd->img, EINA_TRUE);

   return ret;
}

/**
 * @internal
 * @brief Loads the image file for the background.
 * @param[in] obj The Efl.Ui.Bg object (unused).
 * @param[in] sd The private data for the Efl.Ui.Bg object.
 * @return An Eina_Error code indicating success or failure.
 */
EOLIAN static Eina_Error
_efl_ui_bg_efl_file_load(Eo *obj EINA_UNUSED, Efl_Ui_Bg_Data *sd)
{
   return efl_file_load(sd->img);
}

/**
 * @internal
 * @brief Unloads the image file for the background.
 * @param[in] obj The Efl.Ui.Bg object (unused).
 * @param[in] sd The private data for the Efl.Ui.Bg object.
 */
EOLIAN static void
_efl_ui_bg_efl_file_unload(Eo *obj EINA_UNUSED, Efl_Ui_Bg_Data *sd)
{
   efl_file_unload(sd->img);
}

/**
 * @internal
 * @brief Sets the file for the background image.
 * @param[in] obj The Efl.Ui.Bg object (unused).
 * @param[in] sd The private data for the Efl.Ui.Bg object.
 * @param[in] file The path to the image file.
 * @return An Eina_Error code indicating success or failure.
 */
EOLIAN static Eina_Error
_efl_ui_bg_efl_file_file_set(Eo *obj EINA_UNUSED, Efl_Ui_Bg_Data *sd, const char *file)
{
   eina_stringshare_replace(&sd->file, file);

   return efl_file_set(sd->img, file);
}

/**
 * @internal
 * @brief Sets the key (group) for the background image (if Edje).
 * @param[in] obj The Efl.Ui.Bg object (unused).
 * @param[in] sd The private data for the Efl.Ui.Bg object.
 * @param[in] key The Edje group name.
 */
EOLIAN static void
_efl_ui_bg_efl_file_key_set(Eo *obj EINA_UNUSED, Efl_Ui_Bg_Data *sd, const char *key)
{
   eina_stringshare_replace(&sd->key, key);

   efl_file_key_set(sd->img, key);
}

/**
 * @brief Gets the file (and group) being used as background.
 * @param[in] obj The Evas_Object (bg widget) to get the file from.
 * @param[out] file Pointer to store the path to the image or Edje file.
 * @param[out] group Pointer to store the Edje group name.
 * @ingroup Elm_Bg_Group
 */
EAPI void
elm_bg_file_get(const Eo *obj, const char **file, const char **group)
{
   efl_file_simple_get((Eo *) obj, file, group);
}

/**
 * @internal
 * @brief Gets the file path of the background image.
 * @param[in] obj The Efl.Ui.Bg object.
 * @param[in] sd The private data for the Efl.Ui.Bg object.
 * @return The file path stringshared, or @c NULL if not set.
 */
EOLIAN static const char *
_efl_ui_bg_efl_file_file_get(const Eo *obj, Efl_Ui_Bg_Data *sd)
{
   if (elm_widget_is_legacy(obj))
     return sd->file;

   return efl_file_get(sd->img);
}

/**
 * @internal
 * @brief Gets the key (group) of the background image (if Edje).
 * @param[in] obj The Efl.Ui.Bg object.
 * @param[in] sd The private data for the Efl.Ui.Bg object.
 * @return The Edje group name stringshared, or @c NULL if not set or not an Edje file.
 */
EOLIAN static const char *
_efl_ui_bg_efl_file_key_get(const Eo *obj, Efl_Ui_Bg_Data *sd)
{
   if (elm_widget_is_legacy(obj))
     return sd->key;

   return efl_file_key_get(sd->img);
}

/**
 * @internal
 * @brief Sets the mmaped file for the background image.
 * @param[in] obj The Efl.Ui.Bg object (unused).
 * @param[in] sd The private data for the Efl.Ui.Bg object.
 * @param[in] file The Eina_File pointer representing the mmaped file.
 * @return An Eina_Error code indicating success or failure.
 */
EOLIAN static Eina_Error
_efl_ui_bg_efl_file_mmap_set(Eo *obj EINA_UNUSED, Efl_Ui_Bg_Data *sd,
                             const Eina_File *file)
{
   return efl_file_mmap_set(sd->img, file);
}

/**
 * @internal
 * @brief Gets the mmaped file of the background image.
 * @param[in] obj The Efl.Ui.Bg object (unused).
 * @param[in] sd The private data for the Efl.Ui.Bg object.
 * @return The Eina_File pointer, or @c NULL if not set.
 */
EOLIAN static const Eina_File *
_efl_ui_bg_efl_file_mmap_get(const Eo *obj EINA_UNUSED, Efl_Ui_Bg_Data *sd)
{
   return efl_file_mmap_get(sd->img);
}

/**
 * @internal
 * @brief Finalizes the Efl.Ui.Bg object.
 * @details This function is called when the object is being finalized.
 *          It ensures that the image is loaded if a file or mmap is set.
 * @param[in] obj The Efl.Ui.Bg object to finalize.
 * @param[in] sd The private data for the Efl.Ui.Bg object.
 * @return The finalized Efl.Ui.Bg object, or @c NULL on failure.
 */
EOLIAN static Eo *
_efl_ui_bg_efl_object_finalize(Eo *obj, Efl_Ui_Bg_Data *sd)
{
   obj = efl_finalize(efl_super(obj, MY_CLASS));
   if (!obj) return NULL;
   if (efl_file_get(sd->img) || efl_file_mmap_get(sd->img)) efl_file_load(sd->img);
   return obj;
}

/* Internal EO APIs and hidden overrides */

EFL_UI_LAYOUT_CONTENT_ALIASES_IMPLEMENT(MY_CLASS_PFX)

#define EFL_UI_BG_EXTRA_OPS \
   EFL_UI_LAYOUT_CONTENT_ALIASES_OPS(MY_CLASS_PFX)

#include "efl_ui_bg.eo.c"


#include "efl_ui_bg_legacy_eo.h"

#define MY_CLASS_NAME_LEGACY "elm_bg"

/**
 * @internal
 * @brief Class constructor for the legacy Elm_Bg widget.
 * @details Registers the legacy smart type "elm_bg".
 * @param[in] klass The Efl_Class for the legacy Elm_Bg widget.
 */
static void
_efl_ui_bg_legacy_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @internal
 * @brief Constructor for the legacy Elm_Bg object.
 * @param[in] obj The legacy Elm_Bg object to construct.
 * @param[in] _pd Private data (unused in this context).
 * @return The constructed legacy Elm_Bg object.
 */
EOLIAN static Eo *
_efl_ui_bg_legacy_efl_object_constructor(Eo *obj, void *_pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, EFL_UI_BG_LEGACY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   EFL_UI_BG_DATA_GET_OR_RETURN_VAL(obj, pd, obj);
   efl_gfx_entity_scale_set(pd->img, 1.0);
   efl_ui_layout_finger_size_multiplier_set(obj, 0, 0);
   return obj;
}

/**
 * @brief Adds a new background to the parent
 *
 * @param parent The parent object
 * @return The new object or NULL if it cannot be created
 *
 * @ingroup Elm_Bg_Group
 */
EAPI Evas_Object *
elm_bg_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   return elm_legacy_add(EFL_UI_BG_LEGACY_CLASS, parent);
}

#include "efl_ui_bg_legacy_eo.c"
