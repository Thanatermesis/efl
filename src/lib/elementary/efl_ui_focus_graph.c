#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#define EFL_UI_FOCUS_OBJECT_PROTECTED


#include <Elementary.h>
#include "elm_priv.h"
#include "efl_ui_focus_graph.h"

/**
 * @brief Represents the four primary quadrants relative to an origin rectangle.
 * Used as bitmasks to identify the spatial relationship of one rectangle to another.
 */
typedef enum {
  Q_TOP = 1,    /**< Element is primarily above the origin. */
  Q_RIGHT = 2,  /**< Element is primarily to the right of the origin. */
  Q_LEFT = 4,   /**< Element is primarily to the left of the origin. */
  Q_BOTTOM = 8, /**< Element is primarily below the origin. */
  Q_LAST = 16   /**< Sentinel value, not a quadrant. */
} Quadrant;

/** @brief Helper array to iterate over Quadrant enum values. */
Quadrant q_helper[] = {Q_TOP, Q_RIGHT, Q_LEFT, Q_BOTTOM};

/**
 * @internal
 * @brief Converts an opaque graph member to an Efl_Ui_Focus_Object.
 *
 * This function uses the offset provided in the context to find the
 * Efl_Ui_Focus_Object pointer within the opaque structure.
 *
 * @param ctx The focus graph context.
 * @param member The opaque graph member.
 * @return The Efl_Ui_Focus_Object pointer.
 */
static inline Efl_Ui_Focus_Object*
_convert(Efl_Ui_Focus_Graph_Context *ctx, Opaque_Graph_Member *member)
{
   return *((Efl_Ui_Focus_Object**)(((char*) member) + ctx->offset_focusable));
}

/**
 * @internal
 * @brief Calculates the distance between the edges of two rectangles in a specific quadrant.
 *
 * For Q_TOP, it's the vertical distance from the bottom edge of r2 to the top edge of o.
 * For Q_LEFT, it's the horizontal distance from the right edge of r2 to the left edge of o.
 * For Q_BOTTOM, it's the vertical distance from the top edge of o to the top edge of r2.
 * For Q_RIGHT, it's the horizontal distance from the left edge of o to the left edge of r2.
 *
 * @param o The origin rectangle.
 * @param r2 The other rectangle.
 * @param q The quadrant indicating the direction of distance calculation.
 * @return The calculated distance. Returns INT_MAX if the quadrant is invalid.
 */
static inline unsigned int
_distance(Eina_Rect o, Eina_Rect r2, Quadrant q)
{
   int res = INT_MAX;
   if (q == Q_TOP)
     res = o.y - eina_rectangle_max_y(&r2.rect);
   else if (q == Q_LEFT)
     res = o.x - eina_rectangle_max_x(&r2.rect);
   else if (q == Q_BOTTOM)
     res = r2.y - eina_rectangle_max_y(&o.rect);
   else if (q == Q_RIGHT)
     res = r2.x - eina_rectangle_max_x(&o.rect);

   return res;
}

/**
 * @internal
 * @brief Retrieves the specific directional result structure from the overall result.
 *
 * @param q The quadrant for which to get the result.
 * @param result The overall calculation result structure.
 * @return A pointer to the Efl_Ui_Focus_Graph_Calc_Direction_Result for the specified quadrant,
 *         or NULL if the quadrant is invalid.
 */
static inline Efl_Ui_Focus_Graph_Calc_Direction_Result*
_result_get(Quadrant q, Efl_Ui_Focus_Graph_Calc_Result *result)
{
  if (q == Q_TOP) return &result->top;
  else if (q == Q_LEFT) return &result->left;
  else if (q == Q_BOTTOM) return &result->bottom;
  else if (q == Q_RIGHT) return &result->right;
  else return NULL;
}

/*
 * @internal
 * @brief Determines the quadrant(s) an element `elem` occupies relative to an `origin` rectangle.
 *
 * This function classifies the `elem`'s position into three levels of precision (lvl1, lvl2, lvl3)
 * for each of the four cardinal directions (Top, Bottom, Left, Right).
 *
 * - lvl1: Strict overlap. For Q_TOP, `elem`'s horizontal span must overlap with `origin`'s
 *         horizontal span, and `elem` must be above `origin`.
 * - lvl2: Projected overlap. For Q_TOP, `elem`'s horizontal span must overlap with a
 *         widened horizontal span of `origin` (widened by the vertical distance between them),
 *         and `elem` must be above `origin`. This creates a cone-like area.
 * - lvl3: General direction. For Q_TOP, `elem` simply needs to be geometrically above `origin`
 *         (max_y of `elem` <= y of `origin`).
 *
 * The parameters `lvl1`, `lvl2`, and `lvl3` are output parameters that will be bitmasks
 * of `Quadrant` enums. For example, if `elem` is directly above and slightly to the left
 * of `origin` according to lvl2 criteria, `*lvl2` might be `Q_TOP | Q_LEFT`.
 *
_quadrant_get return in which quadrant the elem is placed, oriented at origin

All this is based on three levels

lvl1:
       |     |
       | Top |
  _____|_____|_____
       |     |
  left |clean|right
  _____|_____|_____
       |     |
       | Bot |
       |     |
lvl3:
  \               /
    \    Top    /
      \ _____ /
       |     |
  left |clean|right
       |_____|
      /       \
    /   Bot     \
  /               \

lvl3:
       |     |
  L & T| Top | R & T
  _____|_____|_____
       |     |
  left |clean|right
  _____|_____|_____
       |     |
  L & B| Bot | R & B
       |     |


 */

static inline void
_quadrant_get(Eina_Rect origin, Eina_Rect elem, Quadrant *lvl2, Quadrant *lvl1, Quadrant *lvl3)
{
  int dis = 0;

  *lvl1 = 0;
  *lvl2 = 0;
  *lvl3 = 0;

  if (eina_rectangle_max_y(&elem.rect) <= origin.y)
    {
       dis = origin.y - elem.y;

       *lvl3 |= Q_TOP;

       if (eina_spans_intersect(origin.x - dis, 2*dis + origin.w, elem.x, elem.w))
         *lvl2 |= Q_TOP;
       if (eina_spans_intersect(origin.x, origin.w, elem.x, elem.w))
         *lvl1 |= Q_TOP;
    }
  if (elem.y >= eina_rectangle_max_y(&origin.rect))
    {
       dis = eina_rectangle_max_y(&elem.rect) - origin.y;

       *lvl3 |= Q_BOTTOM;

       if (eina_spans_intersect(origin.x - dis, 2*dis + origin.w, elem.x, elem.w))
         *lvl2 |= Q_BOTTOM;
       if (eina_spans_intersect(origin.x, origin.w, elem.x, elem.w))
         *lvl1 |= Q_BOTTOM;
    }
  if (elem.x >= eina_rectangle_max_x(&origin.rect))
    {
       dis = eina_rectangle_max_x(&elem.rect) - origin.x;

       *lvl3 |= Q_RIGHT;

       if (eina_spans_intersect(origin.y - dis, 2*dis + origin.h, elem.y, elem.h))
         *lvl2 |= Q_RIGHT;
       if (eina_spans_intersect(origin.y, origin.h, elem.y, elem.h))
         *lvl1 |= Q_RIGHT;
    }
  if (eina_rectangle_max_x(&elem.rect) <= origin.x)
    {
       dis = origin.x - elem.x;

       *lvl3 |= Q_LEFT;

       if (eina_spans_intersect(origin.y - dis, 2*dis + origin.h, elem.y, elem.h))
         *lvl2 |= Q_LEFT;
       if (eina_spans_intersect(origin.y, origin.h, elem.y, elem.h))
         *lvl1 |= Q_LEFT;
    }

}

/**
 * @brief Calculates the nearest focusable objects in all four directions from a given origin object.
 *
 * This function iterates through a list of candidate nodes and determines which ones are
 * the best candidates for focus navigation in the top, bottom, left, and right directions
 * relative to the `origin_obj`.
 *
 * The selection criteria prioritize:
 * 1. Lower level (lvl1 > lvl2 > lvl3) as determined by `_quadrant_get`.
 * 2. Shorter distance as determined by `_distance`.
 * If multiple objects have the same level and distance, they are all added to the result list
 * for that direction.
 *
 * @param ctx The focus graph context.
 * @param nodes An iterator over all potential Opaque_Graph_Member candidates.
 * @param origin_obj The Opaque_Graph_Member representing the origin object.
 * @param result A pointer to an Efl_Ui_Focus_Graph_Calc_Result structure to be filled.
 */
void
efl_ui_focus_graph_calc(Efl_Ui_Focus_Graph_Context *ctx, Eina_Iterator *nodes, Opaque_Graph_Member *origin_obj, Efl_Ui_Focus_Graph_Calc_Result *result)
{
   Opaque_Graph_Member *elem_obj;
   Eina_Rect origin, elem;

   for (int i = 0; i < 4; ++i)
     {
        Efl_Ui_Focus_Graph_Calc_Direction_Result *res;

        res = _result_get(q_helper[i], result);

        res->distance = INT_MAX;
        res->lvl = INT_MAX;
        res->relation = NULL;
     }

   origin = efl_ui_focus_object_focus_geometry_get(_convert(ctx, origin_obj));

   //printf("=========> CALCING %p %s\n", _convert(ctx, origin_obj), efl_class_name_get(_convert(ctx, origin_obj)));

   EINA_ITERATOR_FOREACH(nodes, elem_obj)
     {
        Efl_Ui_Focus_Graph_Calc_Direction_Result *res;
        unsigned int distance;
        Quadrant lvl3, lvl2, lvl1;

        if (elem_obj == origin_obj) continue;

        elem = efl_ui_focus_object_focus_geometry_get(_convert(ctx, elem_obj));

        if (eina_rectangle_intersection(&origin.rect, &elem.rect)) continue;

        _quadrant_get(origin, elem, &lvl2, &lvl1, &lvl3);

        for (int i = 0; i < 4; ++i)
          {
            if (q_helper[i] & lvl3)
              {
                 int lvl;
                 res = _result_get(q_helper[i], result);
                 EINA_SAFETY_ON_NULL_GOTO(res, cont);

                 distance = _distance(origin, elem, q_helper[i]);

                 if (q_helper[i] & lvl1)
                   lvl = 1;
                 else if (q_helper[i] & lvl2)
                   lvl = 2;
                 else //if (q_helper[i] & lvl3)
                   lvl = 3;

                 if (lvl < res->lvl)
                   {
                      res->relation = eina_list_free(res->relation);
                      res->relation = eina_list_append(res->relation, elem_obj);
                      res->distance = distance;
                      res->lvl = lvl;
                      //printf("=========> %p:%d LVL_DROP     %d \t %d \t %p %s\n", res, i, distance, lvl, origin_obj, efl_class_name_get(_convert(ctx, elem_obj)));
                   }
                 else if (lvl == res->lvl && res->distance > distance)
                   {
                      res->relation = eina_list_free(res->relation);
                      res->relation = eina_list_append(res->relation, elem_obj);
                      res->distance = distance;
                      //printf("=========> %p:%d DIST_DROP    %d \t %d \t %p %s\n", res, i, distance, lvl, origin_obj, efl_class_name_get(_convert(ctx, elem_obj)));

                   }
                 else if (lvl == res->lvl && res->distance >= distance)
                   {
                      res->relation = eina_list_append(res->relation, elem_obj);
                      //printf("=========> %p:%d DIST_ADD     %d \t %d \t %p %s\n", res, i, distance, lvl, origin_obj, efl_class_name_get(_convert(ctx, elem_obj)));
                   }
              }
          }

        continue;
cont:
        continue;
     }
   eina_iterator_free(nodes);
}
