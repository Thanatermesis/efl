#ifdef T2
TPROT(04);
#endif
#ifdef T1
{ TFUN(04), "Rectangles - Solid", 2.0 },
#endif
#if !defined(T1) && !defined(T2)
# include "perf.h"
static Evas_Object *objs[NUM];

/**
 * @brief Initializes the test by creating and configuring solid rectangle objects.
 *
 * This function creates NUM (defined in perf.h) Evas rectangle objects.
 * Each object is assigned a random color and is set to pass events.
 * The objects are stored in the global `objs` array for later manipulation.
 *
 * @param e The Evas canvas on which to create the objects.
 */
TST(04, init) (Evas *e) {
   Evas_Object *o;
   int i;

   srnd();
   for (i = 0; i < NUM; i++)
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
 * This function is called for each animation tick. It recalculates the
 * position (x, y) and size (w, h) of each rectangle object stored in `objs`.
 * The new geometry is based on sinusoidal functions of the time factor `f`
 * and the object's index `i`, creating an animated effect.
 *
 * @param e The Evas canvas (unused in this function).
 * @param f A time factor, typically incrementing, used for animation.
 *          Example: 0.0, 0.016, 0.032, ...
 * @param win_w The current width of the window.
 * @param win_h The current height of the window.
 */
TST(04, tick) (Evas *e EINA_UNUSED, double f, Evas_Coord win_w, Evas_Coord win_h) {
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
        x += sin((double)((f * 50.0) + (i * 13))) * (w0 / 2.0);
        y = (win_h / 2) - (h / 2);
        y += cos((double)((f * 45.0) + (i * 28))) * (h0 / 2.0);
        evas_object_geometry_set(o, x, y, w, h);
     }
}
#endif
