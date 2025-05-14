#ifdef T2
TPROT(03);
#endif
#ifdef T1
{ TFUN(03), "Rectangles", 1.0 },
#endif
#if !defined(T1) && !defined(T2)
# include "perf.h"

/** @file
 * @brief Performance test for Evas rectangle objects.
 *
 * This test creates a number of rectangle objects and animates their
 * size and position.
 */

/** @brief Array to store Evas_Object pointers for the rectangles.
 *
 * This array holds references to all the rectangle objects created by
 * TST(03, init) so they can be manipulated in TST(03, tick).
 * The size of the array is determined by the NUM macro.
 * Example:
 * @code
 * // Assuming NUM is 3
 * // objs[0] might point to the first rectangle
 * // objs[1] might point to the second rectangle
 * // objs[2] might point to the third rectangle
 * @endcode
 */
static Evas_Object *objs[NUM];

/**
 * @brief Initializes the rectangle performance test.
 *
 * This function is called once at the beginning of the test. It creates
 * NUM rectangle objects, sets their initial color (semi-transparent random colors),
 * makes them pass events, and shows them.
 * The created objects are stored in the global `objs` array.
 *
 * @param e The Evas canvas to add objects to.
 */
TST(03, init) (Evas *e) {
   Evas_Object *o;
   int i;

   srnd();
   for (i = 0; i < NUM; i++)
     {
        objs[i] = o = evas_object_rectangle_add(e);
        cleanup_add(o);
        evas_object_color_set
          (o, rnd() & 0x7f, rnd() & 0x7f, rnd() & 0x7f, 0x80);
        evas_object_pass_events_set(o, EINA_TRUE);
        evas_object_show(o);
     }
}

/**
 * @brief Updates the rectangles' geometry for each frame.
 *
 * This function is called repeatedly for each frame of the animation.
 * It calculates new positions (x, y) and dimensions (w, h) for each
 * rectangle based on sinusoidal functions of time (`f`) and the object's
 * index (`i`). This creates a dynamic, wave-like animation.
 *
 * @param e The Evas canvas (unused in this function).
 * @param f A time factor, typically incrementing with each frame, used for animation.
 *          Example: 0.0, 0.016, 0.032, ...
 * @param win_w The current width of the window.
 * @param win_h The current height of the window.
 */
TST(03, tick) (Evas *e EINA_UNUSED, double f, Evas_Coord win_w, Evas_Coord win_h) {
   int i;
   Evas_Coord x, y, w, h, w0, h0;

   for (i = 0; i < NUM; i++)
     {
        Evas_Object *o = objs[i];
        w0 = 120;
        h0 = 120;
        w = 5 + ((1.0 + cos((double)((f * 30.0) + (i * 10)))) * w0 * 2);
        h = 5 + ((1.0 + sin((double)((f * 40.0) + (i * 19)))) * h0 * 2);
        x = (win_w / 2) - (w / 2);
        x += (Evas_Coord)(sin((double)((f * 50.0) + (i * 13))) * (w0 / 2.0));
        y = (win_h / 2) - (h / 2);
        y += (Evas_Coord)(cos((double)((f * 45.0) + (i * 28))) * (h0 / 2.0));
        evas_object_geometry_set(o, x, y, w, h);
     }
}
#endif
