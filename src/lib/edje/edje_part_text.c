/**
 * @file
 * @brief Edje part text handling
 *
 * This file implements the Efl.Canvas.Layout_Part_Text interface for Edje parts.
 * It allows Edje text parts to be manipulated using the standard Efl.Text APIs.
 * This involves proxying calls to the underlying Edje object and managing
 * user-defined text styles.
 */

#include "edje_private.h"
#include "edje_part_helper.h"
#include "efl_canvas_layout_part_text.eo.h"
#define MY_CLASS EFL_CANVAS_LAYOUT_PART_TEXT_CLASS

/**
 * @brief Implements the proxy for Efl.Canvas.Layout_Part_Text.
 * This macro generates the necessary functions to proxy calls from the Efl.Canvas.Layout_Part_Text
 * interface to the underlying Edje object.
 * @param text The name of the proxy (used for function naming).
 * @param MY_CLASS The Eolian class being implemented.
 * @param EINA_FALSE Indicates that this is not a beta API.
 */
PROXY_IMPLEMENTATION(text, MY_CLASS, EINA_FALSE)
#undef PROXY_IMPLEMENTATION

/**
 * @brief Sets the simple text for the Edje part.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param text The text string to set.
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_text_set(Eo *obj,
      void *_pd EINA_UNUSED, const char *text)
{
   PROXY_DATA_GET(obj, pd);
   _edje_efl_text_text_set(obj, pd->ed, pd->part, text, EINA_FALSE, EINA_FALSE);
}

/**
 * @brief Gets the simple text from the Edje part.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @return The text string.
 */
EOLIAN static const char *
_efl_canvas_layout_part_text_efl_text_text_get(const Eo *obj,
      void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_efl_text_text_get(obj, pd->ed, pd->part, EINA_FALSE, EINA_FALSE);
}

/**
 * @brief Gets the markup text from the Edje part.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @return The markup string.
 */
EOLIAN static const char *
_efl_canvas_layout_part_text_efl_text_markup_markup_get(const Eo *obj,
      void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   return _edje_efl_text_text_get(obj, pd->ed, pd->part, EINA_FALSE, EINA_TRUE);
}

/**
 * @brief Sets the markup text for the Edje part.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param text The markup string to set.
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_markup_markup_set(Eo *obj,
      void *_pd EINA_UNUSED, const char *text)
{
   PROXY_DATA_GET(obj, pd);
   _edje_efl_text_text_set(obj, pd->ed, pd->part, text, EINA_FALSE, EINA_TRUE);
}

/* More Efl.Text.* API (@since 1.22) */

/**
 * @brief Sets the background type for the text style.
 * This function updates the user-defined text style for the part.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param type The background type to set (e.g., #EFL_TEXT_STYLE_BACKGROUND_TYPE_NONE).
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_style_text_background_type_set(Eo *obj,
      void *_pd EINA_UNUSED,
      Efl_Text_Style_Background_Type type)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;

   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part);

   eud->u.text_style.types |= EDJE_PART_TEXT_PROP_NONE;
   efl_text_background_type_set(pd->rp->object, type);
}

/**
 * @brief Gets the background type for the text style.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @return The background type.
 */
EOLIAN static Efl_Text_Style_Background_Type
_efl_canvas_layout_part_text_efl_text_style_text_background_type_get(const Eo *obj,
      void *_pd EINA_UNUSED)
{

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT)
      return EFL_TEXT_STYLE_BACKGROUND_TYPE_NONE;

   return efl_text_background_type_get(pd->rp->object);
}

/**
 * @def TEXT_COLOR_IMPL
 * @brief Macro to generate Eolian functions for setting and getting text style colors.
 * This macro simplifies the creation of setter and getter functions for various
 * text color properties (e.g., background, glow, normal, outline).
 *
 * @param x The prefix for the Efl C API function (e.g., `text_background`).
 * @param X The corresponding Edje property type suffix (e.g., `BACKING`).
 *
 * Example usage: `TEXT_COLOR_IMPL(text_background, BACKING)` will generate:
 * - `_efl_canvas_layout_part_text_efl_text_style_text_background_color_set()`
 * - `_efl_canvas_layout_part_text_efl_text_style_text_background_color_get()`
 */
#define TEXT_COLOR_IMPL(x, X) \
EOLIAN static void \
_efl_canvas_layout_part_text_efl_text_style_ ##x ##_color_set(Eo *obj, \
      void *_pd EINA_UNUSED, \
      unsigned char r, unsigned char g, unsigned char b, unsigned char a) \
{ \
   Edje_User_Defined *eud; \
 \
   PROXY_DATA_GET(obj, pd); \
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return; \
 \
   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part); \
 \
   eud->u.text_style.types |= EDJE_PART_TEXT_PROP_COLOR_ ##X; \
   efl_ ##x ##_color_set(pd->rp->object, r, g, b, a); \
} \
\
EOLIAN static void \
_efl_canvas_layout_part_text_efl_text_style_ ##x ##_color_get(const Eo *obj, \
      void *_pd EINA_UNUSED, \
      unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a) \
{ \
   PROXY_DATA_GET(obj, pd); \
   *r = *g = *b = *a = 0; \
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return; \
   efl_ ##x ##_color_get(pd->rp->object, r, g, b, a); \
}

TEXT_COLOR_IMPL(text_background, BACKING)
TEXT_COLOR_IMPL(text_glow, GLOW)
TEXT_COLOR_IMPL(text_secondary_glow, GLOW2)
TEXT_COLOR_IMPL(text, NORMAL)
TEXT_COLOR_IMPL(text_outline, OUTLINE)
TEXT_COLOR_IMPL(text_shadow, SHADOW)
TEXT_COLOR_IMPL(text_strikethrough, STRIKETHROUGH)
TEXT_COLOR_IMPL(text_underline, UNDERLINE)
TEXT_COLOR_IMPL(text_secondary_underline, UNDERLINE2)
TEXT_COLOR_IMPL(text_underline_dashed, UNDERLINE_DASHED)

/**
 * @brief Sets the text effect type for the text style.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param type The text effect type (e.g., #EFL_TEXT_STYLE_EFFECT_TYPE_NONE, #EFL_TEXT_STYLE_EFFECT_TYPE_PLAIN).
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_style_text_effect_type_set(Eo *obj,
      void *_pd EINA_UNUSED,
      Efl_Text_Style_Effect_Type type)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;

   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part);

   eud->u.text_style.types |= EDJE_PART_TEXT_PROP_EFFECT_TYPE;
   efl_text_effect_type_set(pd->rp->object, type);
}

/**
 * @brief Sets the ellipsis value for the text format.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param value The ellipsis value (0.0 to 1.0, or -1.0 for no ellipsis).
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_format_ellipsis_set(Eo *obj,
      void *_pd EINA_UNUSED, double value)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;

   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part);

   eud->u.text_style.types |= EDJE_PART_TEXT_PROP_ELLIPSIS;
   efl_text_ellipsis_set(pd->rp->object, value);
}

/**
 * @brief Gets the ellipsis value for the text format.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @return The ellipsis value.
 */
EOLIAN static double
_efl_canvas_layout_part_text_efl_text_format_ellipsis_get(const Eo *obj,
      void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return 0.0;

   return efl_text_ellipsis_get(pd->rp->object);
}

/**
 * @brief Sets the font family for the text.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param font The font family name (e.g., "Sans", "Serif").
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_font_properties_font_family_set(Eo *obj,
      void *_pd EINA_UNUSED, const char *font)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;


   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part);

   eud->u.text_style.types |= EDJE_PART_TEXT_PROP_FONT;
   efl_text_font_family_set(pd->rp->object, font);
}

/**
 * @brief Gets the font family for the text.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @return The font family name.
 */
EOLIAN static const char *
_efl_canvas_layout_part_text_efl_text_font_properties_font_family_get(const Eo *obj,
      void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return NULL;

   return efl_text_font_family_get(pd->rp->object);
}

/**
 * @brief Sets the font size for the text.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param size The font size in points.
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_font_properties_font_size_set(Eo *obj,
      void *_pd EINA_UNUSED, Efl_Font_Size size)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;


   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part);

   eud->u.text_style.types |= EDJE_PART_TEXT_PROP_FONT;
   efl_text_font_size_set(pd->rp->object, size);
}

/**
 * @brief Gets the font size for the text.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @return The font size in points.
 */
EOLIAN static Efl_Font_Size
_efl_canvas_layout_part_text_efl_text_font_properties_font_size_get(const Eo *obj,
      void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return 0;

   return efl_text_font_size_get(pd->rp->object);
}

/**
 * @brief Sets the shadow direction for the text style.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param type The shadow direction type (e.g., #EFL_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM_RIGHT).
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_style_text_shadow_direction_set(Eo *obj,
      void *_pd EINA_UNUSED,
      Efl_Text_Style_Shadow_Direction type)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;

   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part);

   eud->u.text_style.types |= EDJE_PART_TEXT_PROP_SHADOW_DIRECTION;
   efl_text_shadow_direction_set(pd->rp->object, type);
}

/**
 * @brief Sets the strikethrough type for the text style.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param type The strikethrough type (e.g., #EFL_TEXT_STYLE_STRIKETHROUGH_TYPE_NONE, #EFL_TEXT_STYLE_STRIKETHROUGH_TYPE_SINGLE).
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_style_text_strikethrough_type_set(Eo *obj,
      void *_pd EINA_UNUSED,
      Efl_Text_Style_Strikethrough_Type type)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;

   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part);

   eud->u.text_style.types |= EDJE_PART_TEXT_PROP_STRIKETHROUGH_TYPE;
   efl_text_strikethrough_type_set(pd->rp->object, type);
}

/**
 * @brief Sets the underline type for the text style.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param type The underline type (e.g., #EFL_TEXT_STYLE_UNDERLINE_TYPE_NONE, #EFL_TEXT_STYLE_UNDERLINE_TYPE_SINGLE).
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_style_text_underline_type_set(Eo *obj,
      void *_pd EINA_UNUSED,
      Efl_Text_Style_Underline_Type type)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;


   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part);

   eud->u.text_style.types = EDJE_PART_TEXT_PROP_UNDERLINE_TYPE;
   efl_text_underline_type_set(pd->rp->object, type);
}

/**
 * @brief Sets the underline height for the text style.
 * This typically represents a relative height or offset.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param value The underline height value.
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_style_text_underline_height_set(Eo *obj,
      void *_pd EINA_UNUSED,
      double value)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;

   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part);

   eud->u.text_style.types |= EDJE_PART_TEXT_PROP_UNDERLINE_HEIGHT;
   efl_text_underline_height_set(pd->rp->object, value);
}

/**
 * @brief Sets the width of the dashes for a dashed underline.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param value The width of each dash in pixels.
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_style_text_underline_dashed_width_set(Eo *obj,
      void *_pd EINA_UNUSED,
      int value)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;

   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part);

   eud->u.text_style.types |= EDJE_PART_TEXT_PROP_UNDERLINE_DASHED_WIDTH;
   efl_text_underline_dashed_width_set(pd->rp->object, value);
}

/**
 * @brief Sets the gap between dashes for a dashed underline.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param value The gap between dashes in pixels.
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_style_text_underline_dashed_gap_set(Eo *obj,
      void *_pd EINA_UNUSED,
      int value)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;

   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part);

   eud->u.text_style.types |= EDJE_PART_TEXT_PROP_UNDERLINE_DASHED_GAP;
   efl_text_underline_dashed_gap_set(pd->rp->object, value);
}

/**
 * @brief Sets the text wrapping mode for the text format.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param wrap The text wrapping mode (e.g., #EFL_TEXT_FORMAT_WRAP_NONE, #EFL_TEXT_FORMAT_WRAP_WORD).
 */
EOLIAN static void
_efl_canvas_layout_part_text_efl_text_format_wrap_set(Eo *obj,
      void *_pd EINA_UNUSED, Efl_Text_Format_Wrap wrap)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;

   eud = _edje_user_text_style_definition_fetch(pd->ed, pd->part);

   eud->u.text_style.types |= EDJE_PART_TEXT_PROP_WRAP;
   efl_text_wrap_set(pd->rp->object, wrap);
}

/**
 * @brief Gets the text wrapping mode for the text format.
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @return The text wrapping mode.
 */
EOLIAN static Efl_Text_Format_Wrap
_efl_canvas_layout_part_text_efl_text_format_wrap_get(const Eo *obj,
      void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);

   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT)
      return EFL_TEXT_FORMAT_WRAP_NONE;

   return efl_text_wrap_get(pd->rp->object);
}

/**
 * @brief Allocates and initializes a new Edje_Part_Text_Prop structure.
 * This function creates a new text property holder, sets its type,
 * and appends it to the provided list of properties.
 *
 * @param props Pointer to the Eina_List of properties to append to.
 * @param type The type of the text property (Edje_Part_Text_Prop_Type).
 * @return A pointer to the newly allocated Edje_Part_Text_Prop.
 */
static Edje_Part_Text_Prop *
_prop_new(Eina_List **props, Edje_Part_Text_Prop_Type type)
{
   Edje_Part_Text_Prop *prop;

   prop = malloc(sizeof(*prop));
   prop->type = type;

   *props = eina_list_append(*props, prop);

   return prop;
}

/**
 * @brief Collects current text style properties from a real part and stores them.
 * This function is called to save the current state of text properties
 * that have been modified via the Efl.Text API. These properties are
 * stored in the Edje_User_Defined structure associated with the part.
 * This is typically used when the Edje object is being reloaded or
 * when the part's state needs to be preserved.
 *
 * @param ed The Edje object.
 * @param eud The Edje_User_Defined structure for the part, which will store the collected properties.
 */
void
_canvas_layout_user_text_collect(Edje *ed, Edje_User_Defined *eud)
{
   Edje_Real_Part *rp;
   Eina_List **props = &eud->u.text_style.props;

   rp = _edje_real_part_recursive_get(&ed, eud->part);
   if (!rp) return;

   if (eud->u.text_style.types == EDJE_PART_TEXT_PROP_NONE) return;

   if (eud->u.text_style.types & EDJE_PART_TEXT_PROP_BACKING_TYPE)
     {
        Edje_Part_Text_Prop *prop;

        prop = _prop_new(props, EDJE_PART_TEXT_PROP_BACKING_TYPE);
        prop->val.backing = efl_text_background_type_get(rp->object);
     }
#define STYLE_COLOR_COLLECT(x, X) \
   if (eud->u.text_style.types & EDJE_PART_TEXT_PROP_COLOR_ ##X) \
     { \
        Edje_Part_Text_Prop *prop; \
        prop = _prop_new(props, EDJE_PART_TEXT_PROP_COLOR_ ##X); \
        efl_ ##x ##_color_get(rp->object, \
              &prop->val.color.r, &prop->val.color.g, \
              &prop->val.color.b, &prop->val.color.a); \
     } \

      STYLE_COLOR_COLLECT(text_background, BACKING)
      STYLE_COLOR_COLLECT(text_glow, GLOW)
      STYLE_COLOR_COLLECT(text_secondary_glow, GLOW2)
      STYLE_COLOR_COLLECT(text, NORMAL)
      STYLE_COLOR_COLLECT(text_outline, OUTLINE)
      STYLE_COLOR_COLLECT(text_shadow, SHADOW)
      STYLE_COLOR_COLLECT(text_strikethrough, STRIKETHROUGH)
      STYLE_COLOR_COLLECT(text_underline, UNDERLINE)
      STYLE_COLOR_COLLECT(text_secondary_underline, UNDERLINE2)
      STYLE_COLOR_COLLECT(text_underline_dashed, UNDERLINE_DASHED)
#undef STYLE_COLOR_COLLECT

      if (eud->u.text_style.types & EDJE_PART_TEXT_PROP_EFFECT_TYPE)
        {
           Edje_Part_Text_Prop *prop;

           prop = _prop_new(props, EDJE_PART_TEXT_PROP_EFFECT_TYPE);
           // Example: prop->val.effect might be EFL_TEXT_STYLE_EFFECT_TYPE_PLAIN
           prop->val.effect = efl_text_effect_type_get(rp->object);
        }

   if (eud->u.text_style.types & EDJE_PART_TEXT_PROP_ELLIPSIS)
     {
        Edje_Part_Text_Prop *prop;

        prop = _prop_new(props, EDJE_PART_TEXT_PROP_ELLIPSIS);
        prop->val.nd = efl_text_ellipsis_get(rp->object);
     }

   if (eud->u.text_style.types & EDJE_PART_TEXT_PROP_FONT)
     {
        Edje_Part_Text_Prop *prop;

        prop = _prop_new(props, EDJE_PART_TEXT_PROP_FONT);
        // Example: prop->val.font.font might be "Sans", prop->val.font.size might be 12
        prop->val.font.font = efl_text_font_family_get(rp->object);
        prop->val.font.size = efl_text_font_size_get(rp->object);
     }

   if (eud->u.text_style.types & EDJE_PART_TEXT_PROP_SHADOW_DIRECTION)
     {
        Edje_Part_Text_Prop *prop;

        prop = _prop_new(props, EDJE_PART_TEXT_PROP_SHADOW_DIRECTION);
        // Example: prop->val.shadow might be EFL_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM
        prop->val.shadow = efl_text_shadow_direction_get(rp->object);
     }

   if (eud->u.text_style.types & EDJE_PART_TEXT_PROP_STRIKETHROUGH_TYPE)
     {
        Edje_Part_Text_Prop *prop;

        prop = _prop_new(props, EDJE_PART_TEXT_PROP_STRIKETHROUGH_TYPE);
        // Example: prop->val.strikethrough_type might be EFL_TEXT_STYLE_STRIKETHROUGH_TYPE_SINGLE
        prop->val.strikethrough_type = efl_text_strikethrough_type_get(rp->object);
     }

   if (eud->u.text_style.types & EDJE_PART_TEXT_PROP_UNDERLINE_DASHED_GAP)
     {
        Edje_Part_Text_Prop *prop;

        prop = _prop_new(props, EDJE_PART_TEXT_PROP_UNDERLINE_DASHED_GAP);
        prop->val.ni = efl_text_underline_dashed_gap_get(rp->object);
     }

   if (eud->u.text_style.types & EDJE_PART_TEXT_PROP_UNDERLINE_DASHED_WIDTH)
     {
        Edje_Part_Text_Prop *prop;

        prop = _prop_new(props, EDJE_PART_TEXT_PROP_UNDERLINE_DASHED_WIDTH);
        prop->val.ni = efl_text_underline_dashed_width_get(rp->object);
     }

   if (eud->u.text_style.types & EDJE_PART_TEXT_PROP_UNDERLINE_TYPE)
     {
        Edje_Part_Text_Prop *prop;

        prop = _prop_new(props, EDJE_PART_TEXT_PROP_UNDERLINE_TYPE);
        // Example: prop->val.ni (as underline_type) might be EFL_TEXT_STYLE_UNDERLINE_TYPE_SINGLE
        prop->val.ni = efl_text_underline_type_get(rp->object);
     }

   if (eud->u.text_style.types & EDJE_PART_TEXT_PROP_UNDERLINE_HEIGHT)
     {
        Edje_Part_Text_Prop *prop;

        prop = _prop_new(props, EDJE_PART_TEXT_PROP_UNDERLINE_HEIGHT);
        // Note: Storing double as int (prop->val.ni), potential precision loss if not handled carefully.
        // This seems to be a mismatch with efl_text_underline_height_get returning double.
        // However, the setter efl_text_underline_height_set takes a double.
        // The 'apply' function uses prop->val.nd for this property.
        prop->val.ni = efl_text_underline_height_get(rp->object);
     }

   if (eud->u.text_style.types & EDJE_PART_TEXT_PROP_WRAP)
     {
        Edje_Part_Text_Prop *prop;

        prop = _prop_new(props, EDJE_PART_TEXT_PROP_WRAP);
        // Example: prop->val.wrap might be EFL_TEXT_FORMAT_WRAP_WORD
        prop->val.wrap = efl_text_wrap_get(rp->object);

     }
}

/**
 * @brief Applies a stored text style property to an Edje part.
 * This function is used to restore a previously collected text style property
 * to the actual Edje part object. It iterates through the stored properties
 * and applies them one by one.
 *
 * @param eud The Edje_User_Defined structure containing information about the part.
 * @param obj The Eolian object representing the Edje layout.
 * @param prop The specific Edje_Part_Text_Prop to apply.
 */
void
_canvas_layout_user_text_apply(Edje_User_Defined *eud, Eo *obj,
      Edje_Part_Text_Prop *prop)
{
   switch (prop->type)
     {

      case EDJE_PART_TEXT_PROP_BACKING_TYPE:
        efl_text_background_type_set(
              efl_part(obj,
                 eud->part),
              prop->val.backing);
        break;

#define STYLE_COLOR_CASE(x, X) \
      case EDJE_PART_TEXT_PROP_COLOR_##X : \
        efl_##x ##_color_set(efl_part(obj, \
                 eud->part), \
                 prop->val.color.r, \
                 prop->val.color.g, \
                 prop->val.color.b, \
                 prop->val.color.a); \
        break;

      STYLE_COLOR_CASE(text_background, BACKING)
      STYLE_COLOR_CASE(text_glow, GLOW)
      STYLE_COLOR_CASE(text_secondary_glow, GLOW2)
      STYLE_COLOR_CASE(text, NORMAL)
      STYLE_COLOR_CASE(text_outline, OUTLINE)
      STYLE_COLOR_CASE(text_shadow, SHADOW)
      STYLE_COLOR_CASE(text_strikethrough, STRIKETHROUGH)
      STYLE_COLOR_CASE(text_underline, UNDERLINE)
      STYLE_COLOR_CASE(text_secondary_underline, UNDERLINE2)
      STYLE_COLOR_CASE(text_underline_dashed, UNDERLINE_DASHED)
#undef STYLE_COLOR_CASE

      case EDJE_PART_TEXT_PROP_EFFECT_TYPE:
        efl_text_effect_type_set(
              efl_part(obj,
                 eud->part),
              prop->val.effect);
        break;

      case EDJE_PART_TEXT_PROP_ELLIPSIS:
        efl_text_ellipsis_set(efl_part(obj,
                 eud->part),
              prop->val.nd);
        break;

      case EDJE_PART_TEXT_PROP_FONT:
        efl_text_font_family_set(efl_part(obj,
                 eud->part),
              prop->val.font.font);
        efl_text_font_size_set(efl_part(obj,
                 eud->part),
              prop->val.font.size);
        break;

      case EDJE_PART_TEXT_PROP_SHADOW_DIRECTION:
        efl_text_shadow_direction_set(
              efl_part(obj,
                 eud->part),
              prop->val.shadow);
        break;

      case EDJE_PART_TEXT_PROP_STRIKETHROUGH_TYPE:
        efl_text_strikethrough_type_set(
              efl_part(obj,
                 eud->part),
              prop->val.strikethrough_type);
        break;

      case EDJE_PART_TEXT_PROP_UNDERLINE_DASHED_WIDTH:
        efl_text_underline_dashed_width_set(
              efl_part(obj,
                 eud->part),
              prop->val.ni);
        break;

      case EDJE_PART_TEXT_PROP_UNDERLINE_DASHED_GAP:
        efl_text_underline_dashed_gap_set(
              efl_part(obj,
                 eud->part),
              prop->val.ni);
        break;

      case EDJE_PART_TEXT_PROP_UNDERLINE_TYPE:
        efl_text_underline_type_set(
              efl_part(obj,
                 eud->part),
              prop->val.underline_type);
        break;

      case EDJE_PART_TEXT_PROP_UNDERLINE_HEIGHT:
        efl_text_underline_height_set(
              efl_part(obj,
                 eud->part),
              prop->val.nd);
        break;

      case EDJE_PART_TEXT_PROP_WRAP:
        efl_text_wrap_set(efl_part(obj,
                 eud->part),
              prop->val.wrap);
        break;

      default:
        break;
     }
}

/**
 * @brief Sets the text expansion type for the Edje part.
 * Text expansion determines how text content scales or fits within the part boundaries.
 * This updates both the user-defined definition and the real part's typedata.
 *
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @param type The text expansion type (e.g., #EFL_CANVAS_LAYOUT_PART_TEXT_EXPAND_NONE,
 *             #EFL_CANVAS_LAYOUT_PART_TEXT_EXPAND_SCALE_FIT_CONTENTS).
 */
EOLIAN static void
_efl_canvas_layout_part_text_text_expand_set(Eo *obj,
      void *_pd EINA_UNUSED,
      Efl_Canvas_Layout_Part_Text_Expand type)
{
   Edje_User_Defined *eud;

   PROXY_DATA_GET(obj, pd);
   if (pd->rp->part->type == EDJE_PART_TYPE_TEXT) return;

   eud = _edje_user_text_expand_definition_fetch(pd->ed, pd->part);
   eud->u.text_expand.expand = type;
   pd->rp->typedata.text->expand = type;

}

/**
 * @brief Gets the text expansion type for the Edje part.
 *
 * @param obj The Eolian object.
 * @param _pd Private data for the Eolian object (unused).
 * @return The current text expansion type.
 */
EOLIAN static Efl_Canvas_Layout_Part_Text_Expand
_efl_canvas_layout_part_text_text_expand_get(const Eo *obj,
      void *_pd EINA_UNUSED)
{
   PROXY_DATA_GET(obj, pd);
   return pd->rp->typedata.text->expand;
}

#include "efl_canvas_layout_part_text.eo.c"

