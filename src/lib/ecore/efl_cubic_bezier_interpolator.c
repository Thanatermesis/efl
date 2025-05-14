#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Ecore.h"
#include "ecore_private.h"

#define MY_CLASS EFL_CUBIC_BEZIER_INTERPOLATOR_CLASS

typedef struct _Efl_Cubic_Bezier_Interpolator_Data Efl_Cubic_Bezier_Interpolator_Data;

/**
 * @brief Private data for the Efl.Cubic_Bezier_Interpolator class.
 *
 * This structure holds the four control points that define the cubic Bezier curve.
 * The points are stored as (P1x, P1y, P2x, P2y).
 * P0 is implicitly (0,0) and P3 is implicitly (1,1).
 */
struct _Efl_Cubic_Bezier_Interpolator_Data
{
   double control_points[4]; /**< Array storing the control points: [P1.x, P1.y, P2.x, P2.y] */
};

/**
 * @brief Interpolates a value along the cubic Bezier curve.
 *
 * This function calculates the interpolated value at a given progress point (0.0 to 1.0)
 * along the cubic Bezier curve defined by the control points.
 * If progress is outside the [0.0, 1.0] range, it's returned as is.
 *
 * @param eo_obj The Efl.Object instance.
 * @param pd The private data for the interpolator.
 * @param progress The input progress value, typically between 0.0 and 1.0.
 * @return The interpolated value.
 */
EOLIAN static double
_efl_cubic_bezier_interpolator_efl_interpolator_interpolate(Eo *eo_obj EINA_UNUSED,
                                                            Efl_Cubic_Bezier_Interpolator_Data *pd EINA_UNUSED,
                                                            double progress)
{
   if ((progress < 0.0) || (progress > 1.0))
     return progress;

   return ecore_animator_pos_map_n(progress, ECORE_POS_MAP_CUBIC_BEZIER, 4,
                                   pd->control_points);
}

/**
 * @brief Sets the control points for the cubic Bezier curve.
 *
 * The cubic Bezier curve is defined by four points: P0, P1, P2, and P3.
 * P0 is implicitly (0,0) and P3 is implicitly (1,1). This function sets
 * the coordinates of P1 and P2.
 *
 * @param eo_obj The Efl.Object instance.
 * @param pd The private data for the interpolator.
 * @param p1 The first control point (P1). For example: (Eina_Vector2){.x = 0.25, .y = 0.1}.
 * @param p2 The second control point (P2). For example: (Eina_Vector2){.x = 0.25, .y = 1.0}.
 */
EOLIAN static void
_efl_cubic_bezier_interpolator_control_points_set(Eo *eo_obj EINA_UNUSED,
                                           Efl_Cubic_Bezier_Interpolator_Data *pd,
                                           Eina_Vector2 p1, Eina_Vector2 p2)
{
   pd->control_points[0] = p1.x;
   pd->control_points[1] = p1.y;
   pd->control_points[2] = p2.x;
   pd->control_points[3] = p2.y;
}

/**
 * @brief Gets the control points of the cubic Bezier curve.
 *
 * Retrieves the coordinates of the control points P1 and P2.
 * P0 is implicitly (0,0) and P3 is implicitly (1,1).
 *
 * @param eo_obj The Efl.Object instance.
 * @param pd The private data for the interpolator.
 * @param p1 Pointer to an Eina_Vector2 structure to store the first control point (P1). Can be NULL.
 * @param p2 Pointer to an Eina_Vector2 structure to store the second control point (P2). Can be NULL.
 */
EOLIAN static void
_efl_cubic_bezier_interpolator_control_points_get(const Eo *eo_obj EINA_UNUSED,
                                           Efl_Cubic_Bezier_Interpolator_Data *pd,
                                           Eina_Vector2 *p1, Eina_Vector2 *p2)
{
   if (p1)
     {
        p1->x = pd->control_points[0];
        p1->y = pd->control_points[1];
     }

   if (p2)
     {
        p2->x = pd->control_points[2];
        p2->y = pd->control_points[3];
     }
}

/**
 * @brief Constructor for the Efl.Cubic_Bezier_Interpolator.
 *
 * Initializes the Efl.Cubic_Bezier_Interpolator object and sets default
 * control points. The default control points (1.0, 1.0, 1.0, 1.0)
 * effectively create a linear interpolation, as P1 and P2 are set to P3.
 *
 * @param eo_obj The Efl.Object instance being constructed.
 * @param pd The private data for the interpolator.
 * @return The constructed Efl.Object instance.
 */
EOLIAN static Efl_Object *
_efl_cubic_bezier_interpolator_efl_object_constructor(Eo *eo_obj,
                                                      Efl_Cubic_Bezier_Interpolator_Data *pd)
{
   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));

   pd->control_points[0] = 1.0;
   pd->control_points[1] = 1.0;
   pd->control_points[2] = 1.0;
   pd->control_points[3] = 1.0;

   return eo_obj;
}

#include "efl_cubic_bezier_interpolator.eo.c"
