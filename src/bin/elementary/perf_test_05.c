#ifdef T2
TPROT(05);
#endif
#ifdef T1
{ TFUN(05), "Rectangles (Many)", 1.0 },
#endif
#if !defined(T1) && !defined(T2)
# include "perf.h"

/**
 * @file
 * @brief Performance test for rendering many rectangle objects.
 *
 * This test creates NUM_MANY rectangle objects and animates their
 * size and position.
 */

/** @brief Array to store pointers to the Evas_Object rectangles. */
static Evas_Object *objs[NUM_MANY];

/**
 * @brief Initializes the rectangle objects for the performance test.
 *
 * Creates NUM_MANY rectangle objects, sets their initial color (semi-transparent
 * random colors), makes them pass events, and shows them.
 * Each object is added to a cleanup list.
 *
 * @param e The Evas canvas.
 */
TST(05, init) (Evas *e) {
   Evas_Object *o;
   int i;

   srnd();
   for (i = 0; i < NUM_MANY; i++)
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
 * @brief Updates the geometry of the rectangle objects for each frame.
 *
 * This function is called on every tick (frame) to update the position
 * and size of all rectangle objects, creating an animation effect.
 * The new width (w) and height (h) are calculated using cosine and sine
 * functions of the frame time (f) and object index (i), resulting in
 * a pulsating effect.
 * The new x and y coordinates are calculated to keep the rectangles
 * generally centered while also moving them based on sine/cosine functions
 * of frame time and object index.
 *
 * @param e The Evas canvas (unused).
 * @param f The current frame time, typically a value that increments with time.
 *          Used to drive the animation. Example: 0.0, 0.016, 0.032, ...
 * @param win_w The width of the window.
 * @param win_h The height of the window.
 */
TST(05, tick) (Evas *e EINA_UNUSED, double f, Evas_Coord win_w, Evas_Coord win_h) {
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
