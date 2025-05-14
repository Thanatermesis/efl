#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Efl_Ui.h>
#include <Elementary.h>

/**
 * @brief Sets up a complex graphical mapping on an Evas_Object.
 *
 * This function defines a non-trivial mapping by specifying 8 points,
 * which create two separate rectangular regions on the canvas. These regions
 * are then textured using corresponding parts of the source image, as
 * defined by the UV coordinates.
 *
 * The mapping is structured as follows:
 * - Point 0-3: Define the first rectangle.
 *   - (100,0) -> (200,0) -> (200,100) -> (100,100)
 *   - Mapped from UV [0,0] to [0.5,1] of the source image.
 * - Point 4-7: Define the second rectangle.
 *   - (200,0) -> (100,200) -> (100,300) -> (200,100)
 *   - Mapped from UV [0.5,0] to [1,1] of the source image.
 *
 * @param obj The Evas_Object to apply the mapping to.
 */
static void
_map_set(Evas_Object *obj)
{
   efl_gfx_mapping_reset(obj);
   efl_gfx_mapping_point_count_set(obj, 8);
   //1st rect
   efl_gfx_mapping_coord_absolute_set(obj, 0, 100, 0, 0);
   efl_gfx_mapping_coord_absolute_set(obj, 1, 200, 0, 0);
   efl_gfx_mapping_coord_absolute_set(obj, 2, 200, 100, 0);
   efl_gfx_mapping_coord_absolute_set(obj, 3, 100, 100, 0);
   //2nd rect
   efl_gfx_mapping_coord_absolute_set(obj, 4, 200, 0, 0);
   efl_gfx_mapping_coord_absolute_set(obj, 5, 100, 200, 0);
   efl_gfx_mapping_coord_absolute_set(obj, 6, 100, 300, 0);
   efl_gfx_mapping_coord_absolute_set(obj, 7, 200, 100, 0);

   //uv: 1st rect: uv: [0-1]
   efl_gfx_mapping_uv_set(obj, 0, 0, 0);
   efl_gfx_mapping_uv_set(obj, 1, 0.5, 0);
   efl_gfx_mapping_uv_set(obj, 2, 0.5, 1);
   efl_gfx_mapping_uv_set(obj, 3, 0, 1);
   //uv: 2nd rect
   efl_gfx_mapping_uv_set(obj, 4, 0.5, 0);
   efl_gfx_mapping_uv_set(obj, 5, 1, 0);
   efl_gfx_mapping_uv_set(obj, 6, 1, 1);
   efl_gfx_mapping_uv_set(obj, 7, 0.5, 1);
}

/**
 * @brief Callback function invoked when the image object is resized.
 *
 * This function ensures that the custom graphical mapping is reapplied
 * whenever the image's size changes, maintaining the intended visual effect.
 *
 * @param data Custom data pointer (unused).
 * @param e The Evas canvas (unused).
 * @param obj The Evas_Object that was resized.
 * @param event_info Event-specific information (unused).
 */
static void
_image_resize_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   _map_set(obj);
}

/**
 * @brief Test case for Efl_Gfx_Mapping.
 *
 * This test creates a window and displays an image with a complex graphical
 * mapping applied. The mapping splits the image into two rectangular regions
 * and positions them non-contiguously on the canvas. It demonstrates how to
 * use efl_gfx_mapping API to achieve custom transformations.
 *
 * @param data Custom data pointer (unused).
 * @param obj The parent object (unused).
 * @param event_info Event-specific information (unused).
 */
void
test_efl_gfx_mapping(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
              void *event_info EINA_UNUSED)
{
   const Evas_Coord W = 300, H = 300;
   Evas_Object *win, *img;
   char buf[PATH_MAX];

   win = efl_add(EFL_UI_WIN_CLASS, efl_main_loop_get(),
                 efl_text_set(efl_added, "Efl Gfx Map"),
                 efl_ui_win_autodel_set(efl_added, EINA_TRUE));

   /* image with a min size */
   snprintf(buf, sizeof(buf), "%s/images/rock_02.jpg", elm_app_data_dir_get());
   img = efl_add(EFL_UI_IMAGE_CLASS, win,
                 efl_gfx_hint_size_min_set(efl_added, EINA_SIZE2D(64, 64)),
                 efl_file_set(efl_added, buf));
   efl_gfx_image_scale_method_set(img, EFL_GFX_IMAGE_SCALE_METHOD_FILL);
   evas_object_event_callback_add(img, EVAS_CALLBACK_RESIZE,
                                  _image_resize_cb, NULL);

   _map_set(img);

   efl_content_set(win, img);
   efl_gfx_entity_size_set(win, EINA_SIZE2D(W, H));
}
