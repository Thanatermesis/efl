
void _evas_text_shadow_color_set(Eo *obj, Evas_Text_Data *pd, int r, int g, int b, int a);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_text_shadow_color_set, EFL_FUNC_CALL(r, g, b, a), int r, int g, int b, int a);

void _evas_text_shadow_color_get(const Eo *obj, Evas_Text_Data *pd, int *r, int *g, int *b, int *a);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_text_shadow_color_get, EFL_FUNC_CALL(r, g, b, a), int *r, int *g, int *b, int *a);

void _evas_text_ellipsis_set(Eo *obj, Evas_Text_Data *pd, double ellipsis);

/**
 * @internal
 * @brief Eolian reflection function for the 'ellipsis' property setter.
 *
 * This function serves as a bridge for the Eolian property system. When a property
 * named "ellipsis" is set on an Evas_Text object using generic Eolian property
 * access (e.g., efl_property_set()), this function is invoked.
 * It is responsible for converting the provided Eina_Value (which holds the
 * property value in a generic container) into the specific C type expected by
 * the underlying implementation (a double for ellipsis). After successful
 * conversion, it calls the actual C function (evas_obj_text_ellipsis_set)
 * that implements the property logic.
 *
 * @param obj The Evas_Text object instance.
 * @param val An Eina_Value containing the new value for the 'ellipsis' property.
 *            This value must be convertible to a double.
 * @return EINA_ERROR_NO_ERROR on success, or an Eina_Error code if the
 *         value conversion fails (e.g., EINA_ERROR_VALUE_FAILED).
 */
static Eina_Error
__eolian_evas_text_ellipsis_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   double cval;
   if (!eina_value_double_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   evas_obj_text_ellipsis_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_text_ellipsis_set, EFL_FUNC_CALL(ellipsis), double ellipsis);

double _evas_text_ellipsis_get(const Eo *obj, Evas_Text_Data *pd);

/**
 * @internal
 * @brief Eolian reflection function for the 'ellipsis' property getter.
 *
 * This function serves as a bridge for the Eolian property system. When a property
 * named "ellipsis" is read from an Evas_Text object using generic Eolian property
 * access (e.g., efl_property_get()), this function is invoked.
 * It calls the actual C function (evas_obj_text_ellipsis_get) that implements
 * the property logic to retrieve the current ellipsis value. The retrieved C type
 * (a double) is then wrapped into an Eina_Value container to be returned to the
 * Eolian system.
 *
 * @param obj The Evas_Text object instance.
 * @return An Eina_Value initialized with the current 'ellipsis' value (double).
 *         If the underlying getter returns a value, it's wrapped; otherwise,
 *         an Eina_Value representing an error or void might be returned depending
 *         on Eolian conventions for ungettable properties (though 'ellipsis' is gettable).
 */
static Eina_Value
__eolian_evas_text_ellipsis_get_reflect(const Eo *obj)
{
   double val = evas_obj_text_ellipsis_get(obj);
   return eina_value_double_init(val);
}

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_text_ellipsis_get, double, -1.000000 /* +1.000000 */);

void _evas_text_bidi_delimiters_set(Eo *obj, Evas_Text_Data *pd, const char *delim);


static Eina_Error
__eolian_evas_text_bidi_delimiters_set_reflect(Eo *obj, Eina_Value val)
{
   Eina_Error r = 0;   const char *cval;
   if (!eina_value_string_convert(&val, &cval))
      {
         r = EINA_ERROR_VALUE_FAILED;
         goto end;
      }
   evas_obj_text_bidi_delimiters_set(obj, cval);
 end:
   eina_value_flush(&val);
   return r;
}

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_text_bidi_delimiters_set, EFL_FUNC_CALL(delim), const char *delim);

const char *_evas_text_bidi_delimiters_get(const Eo *obj, Evas_Text_Data *pd);


static Eina_Value
__eolian_evas_text_bidi_delimiters_get_reflect(const Eo *obj)
{
   const char *val = evas_obj_text_bidi_delimiters_get(obj);
   return eina_value_string_init(val);
}

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_text_bidi_delimiters_get, const char *, NULL);

void _evas_text_outline_color_set(Eo *obj, Evas_Text_Data *pd, int r, int g, int b, int a);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_text_outline_color_set, EFL_FUNC_CALL(r, g, b, a), int r, int g, int b, int a);

void _evas_text_outline_color_get(const Eo *obj, Evas_Text_Data *pd, int *r, int *g, int *b, int *a);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_text_outline_color_get, EFL_FUNC_CALL(r, g, b, a), int *r, int *g, int *b, int *a);

void _evas_text_glow2_color_set(Eo *obj, Evas_Text_Data *pd, int r, int g, int b, int a);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_text_glow2_color_set, EFL_FUNC_CALL(r, g, b, a), int r, int g, int b, int a);

void _evas_text_glow2_color_get(const Eo *obj, Evas_Text_Data *pd, int *r, int *g, int *b, int *a);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_text_glow2_color_get, EFL_FUNC_CALL(r, g, b, a), int *r, int *g, int *b, int *a);

void _evas_text_style_set(Eo *obj, Evas_Text_Data *pd, Evas_Text_Style_Type style);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_text_style_set, EFL_FUNC_CALL(style), Evas_Text_Style_Type style);

Evas_Text_Style_Type _evas_text_style_get(const Eo *obj, Evas_Text_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_text_style_get, Evas_Text_Style_Type, 0);

void _evas_text_glow_color_set(Eo *obj, Evas_Text_Data *pd, int r, int g, int b, int a);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_text_glow_color_set, EFL_FUNC_CALL(r, g, b, a), int r, int g, int b, int a);

void _evas_text_glow_color_get(const Eo *obj, Evas_Text_Data *pd, int *r, int *g, int *b, int *a);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_text_glow_color_get, EFL_FUNC_CALL(r, g, b, a), int *r, int *g, int *b, int *a);

int _evas_text_max_descent_get(const Eo *obj, Evas_Text_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_text_max_descent_get, int, 0);

void _evas_text_style_pad_get(const Eo *obj, Evas_Text_Data *pd, int *l, int *r, int *t, int *b);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_text_style_pad_get, EFL_FUNC_CALL(l, r, t, b), int *l, int *r, int *t, int *b);

Efl_Text_Bidirectional_Type _evas_text_direction_get(const Eo *obj, Evas_Text_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_text_direction_get, Efl_Text_Bidirectional_Type, 0);

int _evas_text_ascent_get(const Eo *obj, Evas_Text_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_text_ascent_get, int, 0);

int _evas_text_horiz_advance_get(const Eo *obj, Evas_Text_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_text_horiz_advance_get, int, 0);

int _evas_text_inset_get(const Eo *obj, Evas_Text_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_text_inset_get, int, 0);

int _evas_text_max_ascent_get(const Eo *obj, Evas_Text_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_text_max_ascent_get, int, 0);

int _evas_text_vert_advance_get(const Eo *obj, Evas_Text_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_text_vert_advance_get, int, 0);

int _evas_text_descent_get(const Eo *obj, Evas_Text_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_text_descent_get, int, 0);

int _evas_text_last_up_to_pos(const Eo *obj, Evas_Text_Data *pd, int x, int y);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV_CONST(evas_obj_text_last_up_to_pos, int, -1 /* +1 */, EFL_FUNC_CALL(x, y), int x, int y);

int _evas_text_char_coords_get(const Eo *obj, Evas_Text_Data *pd, int x, int y, int *cx, int *cy, int *cw, int *ch);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV_CONST(evas_obj_text_char_coords_get, int, 0, EFL_FUNC_CALL(x, y, cx, cy, cw, ch), int x, int y, int *cx, int *cy, int *cw, int *ch);

Eina_Bool _evas_text_char_pos_get(const Eo *obj, Evas_Text_Data *pd, int pos, int *cx, int *cy, int *cw, int *ch);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV_CONST(evas_obj_text_char_pos_get, Eina_Bool, 0, EFL_FUNC_CALL(pos, cx, cy, cw, ch), int pos, int *cx, int *cy, int *cw, int *ch);

Efl_Object *_evas_text_efl_object_constructor(Eo *obj, Evas_Text_Data *pd);


void _evas_text_efl_object_destructor(Eo *obj, Evas_Text_Data *pd);


void _evas_text_efl_gfx_entity_size_set(Eo *obj, Evas_Text_Data *pd, Eina_Size2D size);


void _evas_text_efl_gfx_entity_scale_set(Eo *obj, Evas_Text_Data *pd, double scale);


void _evas_text_efl_text_text_set(Eo *obj, Evas_Text_Data *pd, const char *text);


const char *_evas_text_efl_text_text_get(const Eo *obj, Evas_Text_Data *pd);


void _evas_text_efl_text_font_font_set(Eo *obj, Evas_Text_Data *pd, const char *font, Efl_Font_Size size);


void _evas_text_efl_text_font_font_get(const Eo *obj, Evas_Text_Data *pd, const char **font, Efl_Font_Size *size);


void _evas_text_efl_text_font_font_source_set(Eo *obj, Evas_Text_Data *pd, const char *font_source);


const char *_evas_text_efl_text_font_font_source_get(const Eo *obj, Evas_Text_Data *pd);


void _evas_text_efl_text_font_font_bitmap_scalable_set(Eo *obj, Evas_Text_Data *pd, Efl_Text_Font_Bitmap_Scalable scalable);


Efl_Text_Font_Bitmap_Scalable _evas_text_efl_text_font_font_bitmap_scalable_get(const Eo *obj, Evas_Text_Data *pd);


void _evas_text_efl_gfx_filter_filter_program_set(Eo *obj, Evas_Text_Data *pd, const char *code, const char *name);


void _evas_text_efl_canvas_filter_internal_filter_dirty(Eo *obj, Evas_Text_Data *pd);


Eina_Bool _evas_text_efl_canvas_filter_internal_filter_input_alpha(Eo *obj, Evas_Text_Data *pd);


Eina_Bool _evas_text_efl_canvas_filter_internal_filter_input_render(Eo *obj, Evas_Text_Data *pd, void *filter, void *engine, void *output, void *drawctx, void *data, int l, int r, int t, int b, int x, int y, Eina_Bool do_async);


void _evas_text_efl_canvas_filter_internal_filter_state_prepare(Eo *obj, Evas_Text_Data *pd, Efl_Canvas_Filter_State *state, void *data);


void _evas_text_efl_canvas_object_paragraph_direction_set(Eo *obj, Evas_Text_Data *pd, Efl_Text_Bidirectional_Type dir);


Efl_Text_Bidirectional_Type _evas_text_efl_canvas_object_paragraph_direction_get(const Eo *obj, Evas_Text_Data *pd);

/**
 * @internal
 * @brief Initializes the Evas_Text Efl_Class.
 *
 * This function is automatically called once by the EO C system during the
 * Evas_Text class construction (when the class is first used or explicitly
 * referenced). Its primary role is to set up the class's operational capabilities.
 * This includes:
 * 1. Defining the set of Efl_Object_Ops (operations/methods) specific to Evas_Text.
 *    Each operation maps an abstract operation ID (defined in the .eo file, e.g.,
 *    `evas_obj_text_shadow_color_set`) to its concrete C implementation function
 *    (e.g., `_evas_text_shadow_color_set`).
 * 2. Registering Eolian properties and their associated reflection functions.
 *    Reflection functions (`_reflect` suffixed) allow generic access to properties
 *    via names (strings) using `efl_property_set` and `efl_property_get`.
 *
 * @param klass The Efl_Class pointer representing the Evas_Text class being initialized.
 * @return @c EINA_TRUE if the class initialization was successful, @c EINA_FALSE otherwise.
 *         A failure here typically prevents the class from being used.
 */
static Eina_Bool
_evas_text_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EVAS_TEXT_EXTRA_OPS
#define EVAS_TEXT_EXTRA_OPS
#endif

   // This macro defines the Evas_Text specific Efl_Object_Ops structure.
   // Efl_Object_Ops are essentially vtables for EO classes.
   // Each EFL_OBJECT_OP_FUNC entry maps an operation ID (which is a function pointer
   // itself, often derived from the function name in the .eo file like
   // `evas_obj_text_shadow_color_set`) to the actual C function implementation
   // (e.g., `_evas_text_shadow_color_set`). This allows calling methods on an object
   // polymorphically via efl_call().
   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(evas_obj_text_shadow_color_set, _evas_text_shadow_color_set),
      EFL_OBJECT_OP_FUNC(evas_obj_text_shadow_color_get, _evas_text_shadow_color_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_ellipsis_set, _evas_text_ellipsis_set),
      EFL_OBJECT_OP_FUNC(evas_obj_text_ellipsis_get, _evas_text_ellipsis_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_bidi_delimiters_set, _evas_text_bidi_delimiters_set),
      EFL_OBJECT_OP_FUNC(evas_obj_text_bidi_delimiters_get, _evas_text_bidi_delimiters_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_outline_color_set, _evas_text_outline_color_set),
      EFL_OBJECT_OP_FUNC(evas_obj_text_outline_color_get, _evas_text_outline_color_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_glow2_color_set, _evas_text_glow2_color_set),
      EFL_OBJECT_OP_FUNC(evas_obj_text_glow2_color_get, _evas_text_glow2_color_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_style_set, _evas_text_style_set),
      EFL_OBJECT_OP_FUNC(evas_obj_text_style_get, _evas_text_style_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_glow_color_set, _evas_text_glow_color_set),
      EFL_OBJECT_OP_FUNC(evas_obj_text_glow_color_get, _evas_text_glow_color_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_max_descent_get, _evas_text_max_descent_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_style_pad_get, _evas_text_style_pad_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_direction_get, _evas_text_direction_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_ascent_get, _evas_text_ascent_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_horiz_advance_get, _evas_text_horiz_advance_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_inset_get, _evas_text_inset_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_max_ascent_get, _evas_text_max_ascent_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_vert_advance_get, _evas_text_vert_advance_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_descent_get, _evas_text_descent_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_last_up_to_pos, _evas_text_last_up_to_pos),
      EFL_OBJECT_OP_FUNC(evas_obj_text_char_coords_get, _evas_text_char_coords_get),
      EFL_OBJECT_OP_FUNC(evas_obj_text_char_pos_get, _evas_text_char_pos_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _evas_text_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _evas_text_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_size_set, _evas_text_efl_gfx_entity_size_set),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_scale_set, _evas_text_efl_gfx_entity_scale_set),
      EFL_OBJECT_OP_FUNC(efl_text_set, _evas_text_efl_text_text_set),
      EFL_OBJECT_OP_FUNC(efl_text_get, _evas_text_efl_text_text_get),
      EFL_OBJECT_OP_FUNC(efl_text_font_family_set, _evas_text_efl_text_font_font_family_set),
      EFL_OBJECT_OP_FUNC(efl_text_font_family_get, _evas_text_efl_text_font_font_family_get),
      EFL_OBJECT_OP_FUNC(efl_text_font_size_set, _evas_text_efl_text_font_font_size_set),
      EFL_OBJECT_OP_FUNC(efl_text_font_size_get, _evas_text_efl_text_font_font_size_get),
      EFL_OBJECT_OP_FUNC(efl_text_font_source_set, _evas_text_efl_text_font_font_source_set),
      EFL_OBJECT_OP_FUNC(efl_text_font_source_get, _evas_text_efl_text_font_font_source_get),
      EFL_OBJECT_OP_FUNC(efl_text_font_bitmap_scalable_set, _evas_text_efl_text_font_font_bitmap_scalable_set),
      EFL_OBJECT_OP_FUNC(efl_text_font_bitmap_scalable_get, _evas_text_efl_text_font_font_bitmap_scalable_get),
      EFL_OBJECT_OP_FUNC(efl_gfx_filter_program_set, _evas_text_efl_gfx_filter_filter_program_set),
      EFL_OBJECT_OP_FUNC(evas_filter_dirty, _evas_text_efl_canvas_filter_internal_filter_dirty),
      EFL_OBJECT_OP_FUNC(evas_filter_input_alpha, _evas_text_efl_canvas_filter_internal_filter_input_alpha),
      EFL_OBJECT_OP_FUNC(evas_filter_input_render, _evas_text_efl_canvas_filter_internal_filter_input_render),
      EFL_OBJECT_OP_FUNC(evas_filter_state_prepare, _evas_text_efl_canvas_filter_internal_filter_state_prepare),
      EFL_OBJECT_OP_FUNC(efl_canvas_object_paragraph_direction_set, _evas_text_efl_canvas_object_paragraph_direction_set),
      EFL_OBJECT_OP_FUNC(efl_canvas_object_paragraph_direction_get, _evas_text_efl_canvas_object_paragraph_direction_get),
      EVAS_TEXT_EXTRA_OPS
   );
   opsp = &ops;

   // This static array defines the Eolian property reflection capabilities for Evas_Text.
   // Each element in `refl_table` describes one property:
   // - The first field is the property name as a string (e.g., "ellipsis").
   // - The second field is a function pointer to the setter reflection function
   //   (e.g., `__eolian_evas_text_ellipsis_set_reflect`). This function is called
   //   by `efl_property_set` when the property is set by its string name.
   // - The third field is a function pointer to the getter reflection function
   //   (e.g., `__eolian_evas_text_ellipsis_get_reflect`). This function is called
   //   by `efl_property_get`.
   // This table enables dynamic, name-based access to object properties.
   static const Efl_Object_Property_Reflection refl_table[] = {
      {"ellipsis", __eolian_evas_text_ellipsis_set_reflect, __eolian_evas_text_ellipsis_get_reflect},
      {"bidi_delimiters", __eolian_evas_text_bidi_delimiters_set_reflect, __eolian_evas_text_bidi_delimiters_get_reflect},
   };
   static const Efl_Object_Property_Reflection_Ops rops = {
      refl_table, EINA_C_ARRAY_LENGTH(refl_table)
   };
   ropsp = &rops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @internal
 * @brief Evas_Text class description structure.
 *
 * This structure provides essential metadata for the Evas_Text class to the
 * EO (Efl Object) system. It's used by `EFL_DEFINE_CLASS` to formally
 * declare and register the class.
 */
static const Efl_Class_Description _evas_text_class_desc = {
   EO_VERSION, /**< Specifies the EO API version this class complies with. Ensures compatibility. */
   "Evas.Text", /**< The fully qualified, unique name of the class. Used for identification and lookup within the EO system. */
   EFL_CLASS_TYPE_REGULAR, /**< Defines the nature of the class. `EFL_CLASS_TYPE_REGULAR` means it's a standard instantiable class. Other types include interfaces or mixins. */
   sizeof(Evas_Text_Data), /**< The size in bytes of the private data structure (`Evas_Text_Data`) that will be allocated for each instance of this class. */
   _evas_text_class_initializer, /**< Function pointer to the class initializer. This function is called once when the class is first loaded/used, to set up static class data and operations. */
   NULL, /**< Function pointer to the class constructor (efl_constructor). If NULL, it often implies that the parent class's constructor is sufficient or that object construction is primarily handled by `efl_add` and the `efl_object_constructor` op. */
   NULL  /**< Function pointer to the class destructor (efl_destructor). If NULL, similar to the constructor, it may rely on the parent's destructor or the `efl_object_destructor` op for cleanup. */
};

EFL_DEFINE_CLASS(evas_text_class_get, &_evas_text_class_desc, EFL_CANVAS_OBJECT_CLASS, EFL_TEXT_INTERFACE, EFL_TEXT_FONT_PROPERTIES_INTERFACE, EFL_CANVAS_FILTER_INTERNAL_MIXIN, NULL);

#include "evas_text_eo.legacy.c"
