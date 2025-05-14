#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Sets up and applies a complex Evas_Map to an object.
 *
 * This function creates an 8-point map, effectively splitting the object's
 * visual representation into two quadrilaterals and mapping different parts of
 * the source object to them. This creates a distorted, folded-paper-like effect.
 *
 * The map consists of two parts:
 * 1. A 100x100 pixel rectangle on screen at position (100, 0), which displays
 *    the left half of the source image.
 *    - Vertices: (100,0), (200,0), (200,100), (100,100)
 *    - UV mapping: (0,0), (w/2,0), (w/2,h), (0,h)
 *
 * 2. A distorted quadrilateral on screen, which displays the right half of the
 *    source image.
 *    - Vertices: (200,0), (100,200), (100,300), (200,100)
 *    - UV mapping: (w/2,0), (w,0), (w,h), (w/2,h)
 *
 * @param obj The Evas_Object to apply the map to.
 * @param w The width of the source object for UV mapping.
 * @param h The height of the source object for UV mapping.
 */
static void
_map_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Evas_Map *map;

   map = evas_map_new(8);
   //1st rect
   evas_map_point_coord_set(map, 0, 100, 0, 0);
   evas_map_point_coord_set(map, 1, 200, 0, 0);
   evas_map_point_coord_set(map, 2, 200, 100, 0);
   evas_map_point_coord_set(map, 3, 100, 100, 0);
   //2nd rect
   evas_map_point_coord_set(map, 4, 200, 0, 0);
   evas_map_point_coord_set(map, 5, 100, 200, 0);
   evas_map_point_coord_set(map, 6, 100, 300, 0);
   evas_map_point_coord_set(map, 7, 200, 100, 0);

   //uv: 1st rect
   evas_map_point_image_uv_set(map, 0, 0, 0);
   evas_map_point_image_uv_set(map, 1, w / 2, 0);
   evas_map_point_image_uv_set(map, 2, w / 2, h);
   evas_map_point_image_uv_set(map, 3, 0, h);
   //uv: 2nd rect
   evas_map_point_image_uv_set(map, 4, w / 2, 0);
   evas_map_point_image_uv_set(map, 5, w, 0);
   evas_map_point_image_uv_set(map, 6, w, h);
   evas_map_point_image_uv_set(map, 7, w / 2, h);

   evas_object_map_enable_set(obj, EINA_TRUE);
   evas_object_map_set(obj, map);
   evas_map_free(map);
}

/**
 * @brief Callback function for the EVAS_CALLBACK_RESIZE event on the image object.
 *
 * Whenever the image object is resized, this function is called. It retrieves
 * the new size of the object and calls _map_set() to recalculate and re-apply
 * the Evas_Map based on the new dimensions. This ensures the map distortion
 * scales correctly with the object's size.
 *
 * @param data Custom data pointer (unused).
 * @param e The Evas canvas (unused).
 * @param obj The object that triggered the event.
 * @param event_info Event-specific information (unused).
 */
static void
_image_resize_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Eina_Size2D sz;

   sz = efl_gfx_entity_size_get(obj);
   _map_set(obj, sz.w, sz.h);
}

/**
 * @brief Test function for Evas_Map functionality.
 *
 * This test creates a window and an image object. It then applies a complex
 * 8-point Evas_Map to the image, demonstrating how maps can be used to create
 * non-trivial geometric transformations and distortions of Evas objects.
 *
 * The function also sets up a resize callback to ensure the map is correctly
 * updated when the object's size changes.
 *
 * @param data Custom data, passed from elementary_test (unused).
 * @param obj The parent object, passed from elementary_test (unused).
 * @param event_info Event information, passed from elementary_test (unused).
 */
void
test_evas_map(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
              void *event_info EINA_UNUSED)
{
   const Evas_Coord W = 300, H = 300;
   Evas_Object *win, *img;
   char buf[PATH_MAX];

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                 efl_text_set(efl_added, "Evas Map"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   /* image with a min size */
   snprintf(buf, sizeof(buf), "%s/images/rock_02.jpg", elm_app_data_dir_get());
   img = efl_add(EFL_UI_IMAGE_CLASS, win,
                 efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(64, 64)),
                 efl_file_set(efl_added, buf),
                 efl_file_load(efl_added));
   efl_gfx_image_scale_method_set(img, EFL_GFX_IMAGE_SCALE_METHOD_FILL);
   evas_object_event_callback_add(img, EVAS_CALLBACK_RESIZE,
                                  _image_resize_cb, NULL);

   _map_set(img, W, H);

   efl_content_set(win, img);
   efl_gfx_entity_size_set(win, EINA_SIZE2D(W, H));
}
