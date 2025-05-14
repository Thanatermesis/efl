#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

// 16 ^ 4 = 65k
#define BLOK 16 /**< Defines the number of items to create in each level of the factory. */
// homogeneous layout
//#define HOMOG 1 /**< If defined, sets the box layout to be homogeneous. */
// aligned to top of box
#define ZEROALIGN 1 /**< If defined, aligns the box content to the top (0.0). */
#define DEFSZ 64 /**< Default initial size (height) for factory created items. */

/**
 * @brief Callback function invoked when a factory item is unrealized.
 *
 * This function is responsible for cleaning up the content of the factory item
 * when it is no longer visible or needed. It sets the factory's content to NULL,
 * which effectively deletes the previously set content.
 *
 * @param data User data passed to the callback (unused).
 * @param obj The Evas_Object (factory item) that triggered the callback.
 * @param event_info Additional event information (unused).
 */
static void
fac_unrealize(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   // setting factory content to null deletes it
   printf("--------DELETE for factory %p [f: %p]\n", elm_object_content_get(obj), obj);
   elm_object_content_set(obj, NULL);
}

/**
 * @brief Callback function invoked to realize the final level of factory items.
 *
 * This function creates a box container and populates it with a set of buttons.
 * Each button is labeled with a number derived from its position and the factory's
 * "num" data. This represents the deepest level of content in the nested factory structure.
 *
 * @param data User data, expected to be the parent Evas_Object (window).
 * @param obj The Evas_Object (factory item) that triggered the callback.
 * @param event_info Additional event information (unused).
 */
static void
fac_realize_end(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   Evas_Object *bx, *bt;
   int i;

   bx = elm_box_add(win);
   printf("   ADD lv 3 = %p [%i]\n", bx, (BLOK * (int)(long)evas_object_data_get(obj, "num")));
#ifdef HOMOG
   elm_box_homogeneous_set(bx, EINA_TRUE);
#endif
#ifdef ZEROALIGN
   elm_box_align_set(bx, 0.0, 0.0);
#endif

   for (i = 0; i < BLOK; i++)
     {
        char buf[32];

        snprintf(buf, sizeof(buf), "%i",
                 (i + (BLOK * (int)(long)evas_object_data_get(obj, "num"))));

        bt = elm_button_add(win);
        evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_object_text_set(bt, buf);
        elm_box_pack_end(bx, bt);
        evas_object_show(bt);
     }

   elm_object_content_set(obj, bx);
   evas_object_show(bx);
}

/**
 * @brief Callback function invoked to realize the second level of factory items.
 *
 * This function creates a box container and populates it with another set of
 * factory items (`elm_factory_add`). Each of these nested factory items will,
 * in turn, realize its content using the `fac_realize_end` callback.
 * It also sets up "unrealize" callbacks for these nested factories.
 *
 * @param data User data, expected to be the parent Evas_Object (window).
 * @param obj The Evas_Object (factory item) that triggered the callback.
 * @param event_info Additional event information (unused).
 */
static void
fac_realize2(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   Evas_Object *bx, *fc;
   int i;

   bx = elm_box_add(win);
   printf("  ADD lv 2 = %p [%i]\n", bx, (BLOK * (int)(long)evas_object_data_get(obj, "num")));
#ifdef HOMOG
   elm_box_homogeneous_set(bx, EINA_TRUE);
#endif
#ifdef ZEROALIGN
   elm_box_align_set(bx, 0.0, 0.0);
#endif

   for (i = 0; i < BLOK; i++)
     {
        fc = elm_factory_add(win);
        elm_factory_maxmin_mode_set(fc, EINA_TRUE);
        // initial height per factory of DEFSZ
        // scrollbar will be wrong until enough
        // children have been realized and the
        // real size is known
        evas_object_data_set(fc, "num", (void *)(long)(i + (BLOK * (int)(long)evas_object_data_get(obj, "num"))));
        evas_object_size_hint_min_set(fc, 0, DEFSZ);
        evas_object_size_hint_weight_set(fc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(fc, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_smart_callback_add(fc, "realize", fac_realize_end, win);
        evas_object_smart_callback_add(fc, "unrealize", fac_unrealize, win);
        elm_box_pack_end(bx, fc);
        evas_object_show(fc);
     }

   elm_object_content_set(obj, bx);
   evas_object_show(bx);
}

/**
 * @brief Callback function invoked to realize the first level of factory items.
 *
 * This function creates a box container and populates it with a set of
 * factory items (`elm_factory_add`). Each of these nested factory items will,
 * in turn, realize its content using the `fac_realize2` callback.
 * Note: The "unrealize" callback is commented out for this level in the original code.
 *
 * @param data User data, expected to be the parent Evas_Object (window).
 * @param obj The Evas_Object (factory item) that triggered the callback.
 * @param event_info Additional event information (unused).
 */
static void
fac_realize1(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Evas_Object *win = data;
   Evas_Object *bx, *fc;
   int i;

   bx = elm_box_add(win);
   printf(" ADD lv 1 = %p [%i]\n", bx, (BLOK * (int)(long)evas_object_data_get(obj, "num")));
#ifdef HOMOG
   elm_box_homogeneous_set(bx, EINA_TRUE);
#endif
#ifdef ZEROALIGN
   elm_box_align_set(bx, 0.0, 0.0);
#endif

   for (i = 0; i < BLOK; i++)
     {
        fc = elm_factory_add(win);
        elm_factory_maxmin_mode_set(fc, EINA_TRUE);
        // initial height per factory of DEFSZ
        // scrollbar will be wrong until enough
        // children have been realized and the
        // real size is known
        evas_object_data_set(fc, "num", (void *)(long)(i + (BLOK * (int)(long)evas_object_data_get(obj, "num"))));
        evas_object_size_hint_min_set(fc, 0, DEFSZ);
        evas_object_size_hint_weight_set(fc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(fc, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_smart_callback_add(fc, "realize", fac_realize2, win);
//        evas_object_smart_callback_add(fc, "unrealize", fac_unrealize, win);
        elm_box_pack_end(bx, fc);
        evas_object_show(fc);
     }

   elm_object_content_set(obj, bx);
   evas_object_show(bx);
}

/**
 * @brief Sets up and displays a test window for `elm_factory`.
 *
 * This function creates a window with a scroller containing a box.
 * The box is populated with a series of `elm_factory` objects.
 * Each factory object is configured to realize its content (another level of factories or buttons)
 * when it becomes visible within the scroller. This demonstrates a nested factory setup.
 * The `BLOK` define controls how many items are created at each level, leading to
 * `BLOK` * `BLOK` * `BLOK` * `BLOK` (BLOK^4) total items at the deepest level if all are realized.
 *
 * @param data User data passed to the test function (unused).
 * @param obj The Evas_Object that might have triggered this test (unused).
 * @param event_info Additional event information (unused).
 */
void
test_factory(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *sc, *fc;
   int i;

   win = elm_win_util_standard_add("factory", "Factory");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
#ifdef HOMOG
   elm_box_homogeneous_set(bx, EINA_TRUE);
#endif
#ifdef ZEROALIGN
   elm_box_align_set(bx, 0.0, 0.0);
#endif
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, 0.0);

   for (i = 0; i < BLOK; i++)
     {
        fc = elm_factory_add(win);
        elm_factory_maxmin_mode_set(fc, EINA_TRUE);
        // initial height per factory of DEFSZ
        // scrollbar will be wrong until enough
        // children have been realized and the
        // real size is known
        evas_object_data_set(fc, "num", (void *)(long)i);
        evas_object_size_hint_min_set(fc, 0, DEFSZ);
        evas_object_size_hint_weight_set(fc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(fc, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_smart_callback_add(fc, "realize", fac_realize1, win);
//        evas_object_smart_callback_add(fc, "unrealize", fac_unrealize, win);
        elm_box_pack_end(bx, fc);
        evas_object_show(fc);
     }

   sc = elm_scroller_add(win);
   elm_scroller_bounce_set(sc, EINA_FALSE, EINA_TRUE);
   evas_object_size_hint_weight_set(sc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, sc);

   elm_object_content_set(sc, bx);
   evas_object_show(bx);

   evas_object_show(sc);

   evas_object_resize(win, 320 * elm_config_scale_get(),
                           480 * elm_config_scale_get());
   evas_object_show(win);
}
