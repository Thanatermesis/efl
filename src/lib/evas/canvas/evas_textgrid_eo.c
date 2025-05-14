/**
 * @brief Internal implementation for evas_obj_textgrid_supported_font_styles_set.
 *
 * Sets the font styles supported by the textgrid object.
 * @note The public API indicates this feature is not yet implemented.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[in] styles A bitmask of supported #Evas_Textgrid_Font_Style.
 */
void _evas_textgrid_supported_font_styles_set(Eo *obj, Evas_Textgrid_Data *pd, Evas_Textgrid_Font_Style styles);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_textgrid_supported_font_styles_set, EFL_FUNC_CALL(styles), Evas_Textgrid_Font_Style styles);
/**
 * @brief Internal implementation for evas_obj_textgrid_supported_font_styles_get.
 *
 * Gets the font styles supported by the textgrid object.
 * @note The public API indicates this feature is not yet implemented.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @return A bitmask of supported #Evas_Textgrid_Font_Style. Currently defaults to 0 as per public API.
 */
Evas_Textgrid_Font_Style _evas_textgrid_supported_font_styles_get(const Eo *obj, Evas_Textgrid_Data *pd);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(evas_obj_textgrid_supported_font_styles_get, Evas_Textgrid_Font_Style, 0);
/**
 * @brief Internal implementation for evas_obj_textgrid_grid_size_set.
 *
 * Sets the dimensions (width and height) of the textgrid.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[in] w The number of columns (width in cells).
 * @param[in] h The number of rows (height in cells).
 */
void _evas_textgrid_grid_size_set(Eo *obj, Evas_Textgrid_Data *pd, int w, int h);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_textgrid_grid_size_set, EFL_FUNC_CALL(w, h), int w, int h);
/**
 * @brief Internal implementation for evas_obj_textgrid_grid_size_get.
 *
 * Retrieves the dimensions (width and height) of the textgrid.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[out] w Pointer to store the number of columns (width in cells).
 * @param[out] h Pointer to store the number of rows (height in cells).
 */
void _evas_textgrid_grid_size_get(const Eo *obj, Evas_Textgrid_Data *pd, int *w, int *h);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_textgrid_grid_size_get, EFL_FUNC_CALL(w, h), int *w, int *h);
/**
 * @brief Internal implementation for evas_obj_textgrid_cell_size_get.
 *
 * Retrieves the size of a single cell in pixels.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[out] width Pointer to store the cell width in pixels.
 * @param[out] height Pointer to store the cell height in pixels.
 */
void _evas_textgrid_cell_size_get(const Eo *obj, Evas_Textgrid_Data *pd, int *width, int *height);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_textgrid_cell_size_get, EFL_FUNC_CALL(width, height), int *width, int *height);
/**
 * @brief Internal implementation for evas_obj_textgrid_update_add.
 *
 * Marks a rectangular region of cells as needing a redraw.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[in] x The starting column (X-coordinate) of the region.
 * @param[in] y The starting row (Y-coordinate) of the region.
 * @param[in] w The width of the region in cells.
 * @param[in] h The height of the region in cells.
 */
void _evas_textgrid_update_add(Eo *obj, Evas_Textgrid_Data *pd, int x, int y, int w, int h);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_textgrid_update_add, EFL_FUNC_CALL(x, y, w, h), int x, int y, int w, int h);
/**
 * @brief Internal implementation for evas_obj_textgrid_cellrow_set.
 *
 * Sets the content of an entire row of cells in the textgrid.
 * The `row` parameter is an array of #Evas_Textgrid_Cell structures,
 * with the length equal to the grid width.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[in] y The row index to set.
 * @param[in] row Pointer to an array of #Evas_Textgrid_Cell data for the row.
 */
void _evas_textgrid_cellrow_set(Eo *obj, Evas_Textgrid_Data *pd, int y, const Evas_Textgrid_Cell *row);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_textgrid_cellrow_set, EFL_FUNC_CALL(y, row), int y, const Evas_Textgrid_Cell *row);
/**
 * @brief Internal implementation for evas_obj_textgrid_cellrow_get.
 *
 * Retrieves a pointer to the cell data for a specified row.
 * The returned pointer is to an array of #Evas_Textgrid_Cell structures.
 * This data can be modified directly, followed by a call to
 * _evas_textgrid_update_add() for the affected region.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[in] y The row index to get.
 * @return Pointer to the first #Evas_Textgrid_Cell of the specified row,
 *         or @c NULL on error or if y is out of bounds.
 */
Evas_Textgrid_Cell *_evas_textgrid_cellrow_get(const Eo *obj, Evas_Textgrid_Data *pd, int y);

EVAS_API EVAS_API_WEAK EFL_FUNC_BODYV_CONST(evas_obj_textgrid_cellrow_get, Evas_Textgrid_Cell *, NULL, EFL_FUNC_CALL(y), int y);
/**
 * @brief Internal implementation for evas_obj_textgrid_palette_set.
 *
 * Sets a color in the specified palette at the given index.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[in] pal The #Evas_Textgrid_Palette type (e.g., #EVAS_TEXTGRID_PALETTE_STANDARD).
 * @param[in] idx The palette index (0-255).
 * @param[in] r The red component (0-255).
 * @param[in] g The green component (0-255).
 * @param[in] b The blue component (0-255).
 * @param[in] a The alpha component (0-255).
 */
void _evas_textgrid_palette_set(Eo *obj, Evas_Textgrid_Data *pd, Evas_Textgrid_Palette pal, int idx, int r, int g, int b, int a);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(evas_obj_textgrid_palette_set, EFL_FUNC_CALL(pal, idx, r, g, b, a), Evas_Textgrid_Palette pal, int idx, int r, int g, int b, int a);
/**
 * @brief Internal implementation for evas_obj_textgrid_palette_get.
 *
 * Retrieves a color from the specified palette at the given index.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[in] pal The #Evas_Textgrid_Palette type.
 * @param[in] idx The palette index.
 * @param[out] r Pointer to store the red component.
 * @param[out] g Pointer to store the green component.
 * @param[out] b Pointer to store the blue component.
 * @param[out] a Pointer to store the alpha component.
 */
void _evas_textgrid_palette_get(const Eo *obj, Evas_Textgrid_Data *pd, Evas_Textgrid_Palette pal, int idx, int *r, int *g, int *b, int *a);

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV_CONST(evas_obj_textgrid_palette_get, EFL_FUNC_CALL(pal, idx, r, g, b, a), Evas_Textgrid_Palette pal, int idx, int *r, int *g, int *b, int *a);
/**
 * @brief Constructor for Evas_Textgrid objects.
 *
 * Initializes the Evas_Textgrid object instance.
 * This function is called when a new Evas_Textgrid object is created.
 *
 * @param[in] obj The Eo object to construct.
 * @param[in] pd The private data for the object.
 * @return The constructed Eo object, or @c NULL on failure.
 */
Efl_Object *_evas_textgrid_efl_object_constructor(Eo *obj, Evas_Textgrid_Data *pd);

/**
 * @brief Destructor for Evas_Textgrid objects.
 *
 * Cleans up resources used by the Evas_Textgrid object instance.
 * This function is called when an Evas_Textgrid object is destroyed.
 *
 * @param[in] obj The Eo object to destruct.
 * @param[in] pd The private data of the object.
 */
void _evas_textgrid_efl_object_destructor(Eo *obj, Evas_Textgrid_Data *pd);

/**
 * @brief Internal implementation for efl_text_font_family_set.
 *
 * Sets the font family (name) for the textgrid.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[in] font The font family name (e.g., "Monospace", "DejaVu Sans Mono").
 */
void _evas_textgrid_efl_text_font_font_family_set(Eo *obj, Evas_Textgrid_Data *pd, const char *font);
/**
 * @brief Internal implementation for efl_text_font_family_get.
 *
 * Retrieves the font family (name) used by the textgrid.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @return The current font family name. The returned string is an Eina_Stringshare,
 *         so it should not be freed by the caller.
 */
const char * _evas_textgrid_efl_text_font_font_family_get(const Eo *obj, Evas_Textgrid_Data *pd);

/**
 * @brief Internal implementation for efl_text_font_size_set.
 *
 * Sets the font size for the textgrid.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[in] size The font size (e.g., 10, 12). This is an #Efl_Font_Size (typedef for double).
 */
void _evas_textgrid_efl_text_font_font_size_set(Eo *obj, Evas_Textgrid_Data *pd, Efl_Font_Size size);
/**
 * @brief Internal implementation for efl_text_font_size_get.
 *
 * Retrieves the font size used by the textgrid.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @return The current font size (#Evas_Font_Size is an `int` type for legacy reasons,
 *         but #Efl_Font_Size is `double`).
 */
Evas_Font_Size _evas_textgrid_efl_text_font_font_size_get(const Eo *obj, Evas_Textgrid_Data *pd);

/**
 * @brief Internal implementation for efl_text_font_source_set.
 *
 * Sets the font source (e.g., a file path) for the textgrid.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[in] font_source The path to the font file.
 */
void _evas_textgrid_efl_text_font_font_source_set(Eo *obj, Evas_Textgrid_Data *pd, const char *font_source);
/**
 * @brief Internal implementation for efl_text_font_source_get.
 *
 * Retrieves the font source used by the textgrid.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @return The current font source path. The returned string is an Eina_Stringshare.
 */
const char *_evas_textgrid_efl_text_font_font_source_get(const Eo *obj, Evas_Textgrid_Data *pd);

/**
 * @brief Internal implementation for efl_text_font_bitmap_scalable_set.
 *
 * Sets whether bitmap fonts should be scaled.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[in] scalable The #Efl_Text_Font_Bitmap_Scalable mode (e.g., #EFL_TEXT_FONT_BITMAP_SCALABLE_FIXED).
 */
void _evas_textgrid_efl_text_font_font_bitmap_scalable_set(Eo *obj, Evas_Textgrid_Data *pd, Efl_Text_Font_Bitmap_Scalable scalable);
/**
 * @brief Internal implementation for efl_text_font_bitmap_scalable_get.
 *
 * Retrieves whether bitmap fonts are scaled.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @return The current #Efl_Text_Font_Bitmap_Scalable mode.
 */
Efl_Text_Font_Bitmap_Scalable _evas_textgrid_efl_text_font_font_bitmap_scalable_get(const Eo *obj, Evas_Textgrid_Data *pd);

/**
 * @brief Internal implementation for efl_gfx_entity_scale_set.
 *
 * Sets the scaling factor for the textgrid object.
 * Note: Textgrid is not typically scalable in the same way as other gfx entities.
 * This might affect font rendering or cell calculations if not handled carefully.
 *
 * @param[in] obj The Eo object.
 * @param[in] pd The private data of the object.
 * @param[in] scale The scaling factor.
 */
void _evas_textgrid_efl_gfx_entity_scale_set(Eo *obj, Evas_Textgrid_Data *pd, double scale);

/**
 * @brief Initializes the Evas_Textgrid EFL class.
 *
 * This function is called once when the Evas_Textgrid class is first used.
 * It sets up the operations (methods) for the class.
 *
 * @param[in] klass The Efl_Class to initialize.
 * @return #EINA_TRUE on success, #EINA_FALSE otherwise.
 */
static Eina_Bool
_evas_textgrid_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EVAS_TEXTGRID_EXTRA_OPS
#define EVAS_TEXTGRID_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(evas_obj_textgrid_supported_font_styles_set, _evas_textgrid_supported_font_styles_set),
      EFL_OBJECT_OP_FUNC(evas_obj_textgrid_supported_font_styles_get, _evas_textgrid_supported_font_styles_get),
      EFL_OBJECT_OP_FUNC(evas_obj_textgrid_grid_size_set, _evas_textgrid_grid_size_set),
      EFL_OBJECT_OP_FUNC(evas_obj_textgrid_grid_size_get, _evas_textgrid_grid_size_get),
      EFL_OBJECT_OP_FUNC(evas_obj_textgrid_cell_size_get, _evas_textgrid_cell_size_get),
      EFL_OBJECT_OP_FUNC(evas_obj_textgrid_update_add, _evas_textgrid_update_add),
      EFL_OBJECT_OP_FUNC(evas_obj_textgrid_cellrow_set, _evas_textgrid_cellrow_set),
      EFL_OBJECT_OP_FUNC(evas_obj_textgrid_cellrow_get, _evas_textgrid_cellrow_get),
      EFL_OBJECT_OP_FUNC(evas_obj_textgrid_palette_set, _evas_textgrid_palette_set),
      EFL_OBJECT_OP_FUNC(evas_obj_textgrid_palette_get, _evas_textgrid_palette_get),
      EFL_OBJECT_OP_FUNC(efl_constructor, _evas_textgrid_efl_object_constructor),
      EFL_OBJECT_OP_FUNC(efl_destructor, _evas_textgrid_efl_object_destructor),
      EFL_OBJECT_OP_FUNC(efl_text_font_family_set, _evas_textgrid_efl_text_font_font_family_set),
      EFL_OBJECT_OP_FUNC(efl_text_font_family_get, _evas_textgrid_efl_text_font_font_family_get),
      EFL_OBJECT_OP_FUNC(efl_text_font_size_set, _evas_textgrid_efl_text_font_font_size_set),
      EFL_OBJECT_OP_FUNC(efl_text_font_size_get, _evas_textgrid_efl_text_font_font_size_get),
      EFL_OBJECT_OP_FUNC(efl_text_font_source_set, _evas_textgrid_efl_text_font_font_source_set),
      EFL_OBJECT_OP_FUNC(efl_text_font_source_get, _evas_textgrid_efl_text_font_font_source_get),
      EFL_OBJECT_OP_FUNC(efl_text_font_bitmap_scalable_set, _evas_textgrid_efl_text_font_font_bitmap_scalable_set),
      EFL_OBJECT_OP_FUNC(efl_text_font_bitmap_scalable_get, _evas_textgrid_efl_text_font_font_bitmap_scalable_get),
      EFL_OBJECT_OP_FUNC(efl_gfx_entity_scale_set, _evas_textgrid_efl_gfx_entity_scale_set),
      EVAS_TEXTGRID_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

static const Efl_Class_Description _evas_textgrid_class_desc = {
   EO_VERSION,
   "Evas.Textgrid",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Evas_Textgrid_Data),
   _evas_textgrid_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(evas_textgrid_class_get, &_evas_textgrid_class_desc, EFL_CANVAS_OBJECT_CLASS, EFL_TEXT_FONT_PROPERTIES_INTERFACE, NULL);

#include "evas_textgrid_eo.legacy.c"
