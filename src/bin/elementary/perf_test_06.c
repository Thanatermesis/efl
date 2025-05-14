#ifdef T2
TPROT(06);
#endif
#ifdef T1
{ TFUN(06), "Rectangles (Many) - Solid", 2.0 },
#endif
#if !defined(T1) && !defined(T2)
# include "perf.h"

/** @file
 * @brief Performance test for rendering many solid rectangle objects.
 *
 * This test creates NUM_MANY rectangle objects and animates their
 * size and position.
 */

static Evas_Object *objs[NUM_MANY]; /**< Array to store pointers to the rectangle Evas_Object instances. */

/**
 * @brief Initializes the rectangle objects for the performance test.
 *
 * This function creates NUM_MANY rectangle objects, sets their initial color
 * to a random value, makes them pass events, and shows them.
 * Each object is added to a cleanup list.
 *
 * @param e The Evas canvas on which to create the objects.
 */
TST(06, init) (Evas *e) {
   Evas_Object *o;
   int i;

   srnd();
   for (i = 0; i < NUM_MANY; i++)
     {
        objs[i] = o = evas_object_rectangle_add(e);
        cleanup_add(o);
        evas_object_color_set
          (o, rnd() & 0xff, rnd() & 0xff, rnd() & 0xff, 0xff);
        evas_object_pass_events_set(o, EINA_TRUE);
        evas_object_show(o);
     }
}

/**
 * @brief Updates the geometry of the rectangle objects for each frame.
 *
 * This function is called on every tick (frame). It calculates new
 * positions (x, y) and dimensions (w, h) for each rectangle object
 * based on sinusoidal functions of time and an index. This creates
 * a dynamic animation.
 *
 * @param e The Evas canvas (unused in this function).
 * @param f A time factor, typically representing the animation progress or time elapsed.
 *          Used to drive the sinusoidal animations. For example, f might range from 0.0 to 1.0.
 * @param win_w The current width of the window.
 * @param win_h The current height of the window.
 */
TST(06, tick) (Evas *e EINA_UNUSED, double f, Evas_Coord win_w, Evas_Coord win_h) {
   int i;
   Evas_Coord x, y, w, h, w0, h0;

   for (i = 0; i < NUM_MANY; i++)
     {
        Evas_Object *o = objs[i];
        w0 = 120;
        h0 = 120;
        w = 5 + ((1.0 + cos((double)((f * 30.0) + (i * 10)))) * w0 * 2);
        h = 5 + ((1.0 + sin((double)((f * 40.0) + (i * 19)))) * h0 * 2);
        x = (win_w / 2) - (w / 2);
        x += sin((double)((f * 50.0) + (i * 13))) * (w0 / 2.0);
        y = (win_h / 2) - (h / 2);
        y += cos((double)((f * 45.0) + (i * 28))) * (h0 / 2.0);
        evas_object_geometry_set(o, x, y, w, h);
     }
}
#endif
