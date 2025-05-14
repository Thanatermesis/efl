#ifndef _EFL_UI_CONTAINER_HELPER_H_
#define _EFL_UI_CONTAINER_HELPER_H_

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"

typedef struct _Efl_Ui_Container_Item_Hints Efl_Ui_Container_Item_Hints;
typedef struct _Efl_Ui_Container_Layout_Calc Efl_Ui_Container_Layout_Calc;

/**
 * @brief Structure to hold sizing and alignment hints for a container item.
 * This structure is typically used as an array of two, one for the X-axis (index 0)
 * and one for the Y-axis (index 1).
 * For example:
 * Efl_Ui_Container_Item_Hints hints[2];
 * hints[0] refers to X-axis properties (width, horizontal alignment, etc.)
 * hints[1] refers to Y-axis properties (height, vertical alignment, etc.)
 */
struct _Efl_Ui_Container_Item_Hints
{
   int max; /**< Maximum size (width or height) the item can take. */
   int min; /**< Minimum size (width or height) the item must have. */
   int aspect; /**< Aspect ratio value (width component if index 0, height component if index 1). */
   int margin[2]; /**< Margin around the item. margin[0] is start, margin[1] is end. */
   Efl_Gfx_Hint_Aspect aspect_type; /**< Type of aspect ratio control (e.g., EFL_GFX_HINT_ASPECT_NONE, EFL_GFX_HINT_ASPECT_BOTH). */
   double weight; /**< Weight of the item, used for distributing extra space. (0.0 to 1.0) */
   double align; /**< Alignment of the item within its allocated space. (0.0 for start, 0.5 for center, 1.0 for end) */
   double space; /**< Calculated space needed by the item, including min size and margins. */
   Eina_Bool fill; /**< Whether the item should fill the available space along its axis. */
};

/**
 * @brief Structure to hold calculated layout properties for a container or its items.
 * This structure is typically used as an array of two, one for the X-axis (index 0)
 * and one for the Y-axis (index 1).
 * For example:
 * Efl_Ui_Container_Layout_Calc layout_calcs[2];
 * layout_calcs[0] refers to X-axis layout (position, size, etc.)
 * layout_calcs[1] refers to Y-axis layout (position, size, etc.)
 */
struct _Efl_Ui_Container_Layout_Calc
{
   int pos; /**< Calculated starting position (x or y coordinate) of the content area or item. */
   int size; /**< Calculated available size (width or height) for content or item. */
   int margin[2]; /**< Margin for the container itself. margin[0] is start, margin[1] is end. */
   double align; /**< Alignment of the content within the container if no item has weight. (0.0 for start, 0.5 for center, 1.0 for end) */
   unsigned int pad; /**< Padding between items within the container. */
   Eina_Bool fill : 1; /**< Whether the content should fill the container if no item has weight. */
};

/**
 * @brief Calculates and adjusts minimum and maximum dimensions based on aspect ratio.
 *
 * This function refines the provided width (`cw`) and height (`ch`)
 * based on the item's aspect ratio hints, ensuring that the dimensions
 * respect both the aspect ratio and the min/max constraints.
 *
 * @param item An array of two Efl_Ui_Container_Item_Hints structures.
 *             item[0] for X-axis hints, item[1] for Y-axis hints.
 *             The aspect ratio is derived from item[0].aspect and item[1].aspect.
 * @param[in,out] cw Pointer to the current width to be adjusted.
 * @param[in,out] ch Pointer to the current height to be adjusted.
 * @param aspect_check If EINA_TRUE, aspect ratio calculations are enforced.
 */
void _efl_ui_container_layout_min_max_calc(Efl_Ui_Container_Item_Hints *item, int *cw, int *ch, Eina_Bool aspect_check);

/**
 * @brief Initializes item hints from an Efl_Gfx_Hint object.
 *
 * Populates an Efl_Ui_Container_Item_Hints array by fetching various
 * graphics hints (weight, alignment, margin, fill, size, aspect)
 * from the given Evas object `o`. It also performs normalization
 * and validation of these hints.
 *
 * @param o The Evas object from which to retrieve hints.
 * @param item An array of two Efl_Ui_Container_Item_Hints structures to be populated.
 *             item[0] will store X-axis hints, item[1] will store Y-axis hints.
 *             Example:
 *             Efl_Ui_Container_Item_Hints item_hints[2];
 *             _efl_ui_container_layout_item_init(my_object, item_hints);
 *             // item_hints[0] now contains X-axis hints (width, h_align, etc.)
 *             // item_hints[1] now contains Y-axis hints (height, v_align, etc.)
 */
void _efl_ui_container_layout_item_init(Eo* o, Efl_Ui_Container_Item_Hints *item);

/**
 * @brief Initializes layout calculation data for a container object.
 *
 * Populates an Efl_Ui_Container_Layout_Calc array by fetching geometry,
 * margin, padding, and alignment information from the given container
 * object `obj`. This sets up the initial state for layout calculations.
 *
 * @param obj The container Evas object.
 * @param calc An array of two Efl_Ui_Container_Layout_Calc structures to be populated.
 *             calc[0] will store X-axis layout data, calc[1] will store Y-axis layout data.
 *             Example:
 *             Efl_Ui_Container_Layout_Calc layout_data[2];
 *             _efl_ui_container_layout_init(my_container, layout_data);
 *             // layout_data[0] now contains X-axis info (x_pos, width, h_align, etc.)
 *             // layout_data[1] now contains Y-axis info (y_pos, height, v_align, etc.)
 */
void _efl_ui_container_layout_init(Eo* obj, Efl_Ui_Container_Layout_Calc *calc);

#endif
