#ifndef EFL_UI_FOCUS_GRAPH_H
#define EFL_UI_FOCUS_GRAPH_H

/**
 * @file
 * @brief These routines are used for focus graph calculations in EFL UI.
 */

#include <stdio.h>
#include <stdlib.h>
#include <Eina.h>

/**
 * @brief Opaque type representing a member in the focus graph.
 * This is used to abstract the actual data structure of graph nodes.
 */
typedef struct _Opaque_Graph_Memeber Opaque_Graph_Member;

/**
 * @brief Stores the result of a focus calculation for a single direction.
 */
typedef struct {
  Eina_List *relation;    /**< List of Opaque_Graph_Member objects that are candidates in this direction.
                           *   Example: If moving right, this list might contain [objA, objB] if both are
                           *   equally good candidates. objA and objB would be Opaque_Graph_Member*.
                           */
  int lvl;                /**< The classification level (1, 2, or 3) of the relation, indicating geometric proximity.
                           *   Lower levels are generally preferred.
                           */
  unsigned int distance;  /**< The calculated distance to the related object(s).
                           *   Example: 10 (pixels or abstract units).
                           */
} Efl_Ui_Focus_Graph_Calc_Direction_Result;

/**
 * @brief Stores the overall result of focus calculations for all four directions.
 */
typedef struct {
  Efl_Ui_Focus_Graph_Calc_Direction_Result right;   /**< Result for the right direction. */
  Efl_Ui_Focus_Graph_Calc_Direction_Result left;    /**< Result for the left direction. */
  Efl_Ui_Focus_Graph_Calc_Direction_Result top;     /**< Result for the top direction. */
  Efl_Ui_Focus_Graph_Calc_Direction_Result bottom;  /**< Result for the bottom direction. */
} Efl_Ui_Focus_Graph_Calc_Result;

/**
 * @brief Context for focus graph calculations.
 */
typedef struct {
   size_t offset_focusable; /**< Offset within the Opaque_Graph_Member structure to find the actual
                             *   Efl_Ui_Focus_Object pointer. This allows the graph logic to be
                             *   generic and work with different embedding structures.
                             *   Example: If Opaque_Graph_Member is a struct MyNode { int data; Efl_Ui_Focus_Object *focusable; },
                             *   then offset_focusable would be offsetof(MyNode, focusable).
                             */
} Efl_Ui_Focus_Graph_Context;

/**
 * @brief Calculates the nearest focusable objects in all four directions from a given origin object.
 *
 * @param context The focus graph context, containing information like the offset to the focusable object.
 * @param nodes An iterator over all potential Opaque_Graph_Member candidates in the current focus scope.
 * @param member The Opaque_Graph_Member representing the origin object from which to calculate relations.
 * @param result A pointer to an Efl_Ui_Focus_Graph_Calc_Result structure that will be filled with the results.
 *               The members of this struct (top, bottom, left, right) will contain lists of
 *               Opaque_Graph_Member candidates for each direction.
 *               Example (after call, simplified):
 *               result->right.relation might be a list containing one Opaque_Graph_Member*.
 *               result->right.lvl might be 1.
 *               result->right.distance might be 50.
 */
void efl_ui_focus_graph_calc(Efl_Ui_Focus_Graph_Context *context, Eina_Iterator *nodes, Opaque_Graph_Member *member, Efl_Ui_Focus_Graph_Calc_Result *result);

#endif
