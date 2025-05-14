#ifdef T2
TPROT(02);
#endif
#ifdef T1
{ TFUN(02), "Rectangles (Few) - Solid", 0.4 },
#endif
#if !defined(T1) && !defined(T2)
# include "perf.h"
/** @brief Array to store Evas_Object pointers for the rectangles. */
static Evas_Object *objs[NUM_FEW];

/**
 * @brief Initializes the test by creating a few rectangle objects.
 *
 * This function creates NUM_FEW rectangle objects, sets their initial color
 * randomly, makes them pass events, and shows them on the canvas.
 * Each object is added to the cleanup list.
 *
 * @param e The Evas canvas.
 */
TST(02, init) (Evas *e) {
   Evas_Object *o;
   int i;

   srnd();
   for (i = 0; i < NUM_FEW; i++)
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
 * @brief Updates the geometry of the rectangles on each tick.
 *
 * This function is called repeatedly to update the position and size
 * of each rectangle object. The new geometry is calculated based on
 * trigonometric functions of the time factor 'f' and the object's index 'i',
 * creating an animated effect.
 *
 * @param e The Evas canvas (unused in this function).
 * @param f A time factor, typically incrementing, used for animation.
 *          Example: 0.0, 0.01, 0.02, ...
 * @param win_w The width of the window.
 * @param win_h The height of the window.
 */
TST(02, tick) (Evas *e EINA_UNUSED, double f, Evas_Coord win_w, Evas_Coord win_h) {
   int i;
   Evas_Coord x, y, w, h, w0, h0;

   for (i = 0; i < NUM_FEW; i++)
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
