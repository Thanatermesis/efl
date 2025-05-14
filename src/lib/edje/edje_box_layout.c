#include "edje_private.h"

#include "../evas/canvas/evas_box_eo.h"

#include <Eo.h>

/**
 * @brief Structure to hold animation data for a single Evas_Object within a transition.
 *
 * This structure stores the start and end geometries (x, y, w, h) for an object
 * that is part of a box layout transition.
 */
typedef struct _Edje_Transition_Animation_Data Edje_Transition_Animation_Data;
struct _Edje_Transition_Animation_Data
{
   Evas_Object *obj; /**< The Evas_Object being animated. */
   struct
   {
      Evas_Coord x, y, w, h;
   } start, end; /**< Start and end geometry states for the animation. */
};

/**
 * @brief Structure to manage the animation of an Edje part that is a box.
 *
 * This structure holds all necessary information for animating a box layout,
 * including its start and end layout properties (layout function, alignment, padding),
 * a list of child objects being animated, and the current progress of the animation.
 */
struct _Edje_Part_Box_Animation
{
   struct
   {
      Evas_Object_Box_Layout layout; /**< The box layout function (e.g., evas_object_box_layout_horizontal). */
      void                  *data; /**< Custom data for the layout function. */
      void                   (*free_data)(void *data); /**< Function to free the custom layout data. */
      Edje_Alignment         align; /**< Alignment of the box content (x, y). */
      Evas_Point             padding; /**< Padding within the box (x, y). */
   } start, end; /**< Start and end states of the box layout properties for the animation. */
   Eina_List   *objs; /**< List of Edje_Transition_Animation_Data for child objects. */
   Eina_Bool    recalculate : 1; /**< Flag indicating if coordinates need recalculation. */
   Evas_Object *box; /**< The Evas_Object representing the box itself. */
   double       progress; /**< Current animation progress (0.0 to 1.0). */
   double       start_progress; /**< Progress value when the current animation segment started. */
   int          box_start_w, box_start_h; /**< Initial width and height of the box at the start of a recalculation. */
};

/**
 * @brief Finds a box layout function by name, with a fallback.
 *
 * Attempts to find the layout function specified by @p name. If not found,
 * it tries the alternative name @p name_alt. If neither is found, it defaults
 * to `evas_object_box_layout_horizontal`.
 *
 * @param name The primary name of the layout function to find.
 * @param name_alt The alternative (fallback) name of the layout function.
 * @param[out] cb Pointer to store the found layout function.
 * @param[out] data Pointer to store data associated with the layout function.
 * @param[out] free_data Pointer to store the function for freeing the layout data.
 */
static void
_edje_box_layout_find_all(const char *name, const char *name_alt, Evas_Object_Box_Layout *cb, void **data, void(**free_data) (void *data))
{
   if (!_edje_box_layout_find(name, cb, data, free_data))
     {
        if ((!name_alt) ||
            (!_edje_box_layout_find(name_alt, cb, data, free_data)))
          {
             ERR("box layout '%s' (fallback '%s') not available, using horizontal.",
                 name, name_alt);
             *cb = evas_object_box_layout_horizontal;
             *free_data = NULL;
             *data = NULL;
          }
     }
}

/**
 * @brief Calculates the start and end coordinates for all child objects in a box layout animation.
 *
 * This function is called when a box layout animation needs to determine the
 * target positions and sizes of its children. It first records the current (start)
 * geometry of each child relative to the box. Then, it applies the end layout
 * properties (padding, alignment, layout function) to the box to determine
 * the final (end) geometry of each child.
 *
 * @param obj The box Evas_Object.
 * @param priv The private data of the box object.
 * @param anim The animation data for the box layout.
 */
static void
_edje_box_layout_calculate_coords(Evas_Object *obj, Evas_Object_Box_Data *priv, Edje_Part_Box_Animation *anim)
{
   Eina_List *l;
   Edje_Transition_Animation_Data *tad;
   Evas_Coord x, y;

   evas_object_geometry_get(obj, &x, &y, &anim->box_start_w, &anim->box_start_h);
   EINA_LIST_FOREACH(anim->objs, l, tad)
     {
        evas_object_geometry_get(tad->obj, &tad->start.x, &tad->start.y,
                                 &tad->start.w, &tad->start.h);
        tad->start.x = tad->start.x - x;
        tad->start.y = tad->start.y - y;
     }
   evas_object_box_padding_set(obj, anim->end.padding.x, anim->end.padding.y);
   evas_object_box_align_set(obj, TO_DOUBLE(anim->end.align.x), TO_DOUBLE(anim->end.align.y));
   if (anim->end.layout)
     anim->end.layout(obj, priv, anim->end.data);
   else if (anim->start.layout)
     anim->start.layout(obj, priv, anim->start.data);

   EINA_LIST_FOREACH(anim->objs, l, tad)
     {
        evas_object_geometry_get(tad->obj, &tad->end.x, &tad->end.y,
                                 &tad->end.w, &tad->end.h);
        tad->end.x = tad->end.x - x;
        tad->end.y = tad->end.y - y;
     }
}

/**
 * @brief Executes a step in the box layout animation.
 *
 * This function is called to update the position and size of child objects
 * within the box based on the current animation progress. It interpolates
 * the geometry of each child object between its start and end states.
 *
 * @param obj The box Evas_Object.
 * @param anim The animation data for the box layout.
 */
static void
_edje_box_layout_exec(Evas_Object *obj, Edje_Part_Box_Animation *anim)
{
   Eina_List *l;
   Edje_Transition_Animation_Data *tad;
   Evas_Coord x, y, w, h;
   Evas_Coord cur_x, cur_y, cur_w, cur_h;
   double progress;

   evas_object_geometry_get(obj, &x, &y, &w, &h);
   progress = (anim->progress - anim->start_progress) / (1 - anim->start_progress);

   EINA_LIST_FOREACH(anim->objs, l, tad)
     {
        cur_x = x + (tad->start.x + ((tad->end.x - tad->start.x) * progress)) * (w / (double)anim->box_start_w);
        cur_y = y + (tad->start.y + ((tad->end.y - tad->start.y) * progress)) * (h / (double)anim->box_start_h);
        cur_w = (w / (double)anim->box_start_w) * (tad->start.w + ((tad->end.w - tad->start.w) * progress));
        cur_h = (h / (double)anim->box_start_h) * (tad->start.h + ((tad->end.h - tad->start.h) * progress));
        evas_object_move(tad->obj, cur_x, cur_y);
        evas_object_resize(tad->obj, cur_w, cur_h);
     }
}

/**
 * @brief Custom layout function for animated Edje box parts.
 *
 * This function is set as the layout callback for an Evas_Object_Box when
 * it's part of an Edje animation. It handles applying the correct layout
 * (start, end, or interpolated) based on the animation progress.
 * If progress is 0, it applies the start layout. If progress is > 0 and < 1,
 * it recalculates coordinates if needed and executes the animation step.
 *
 * @param obj The box Evas_Object.
 * @param priv The private data of the box object.
 * @param data Custom data, expected to be an Edje_Part_Box_Animation pointer.
 */
static void
_edje_box_layout(Evas_Object *obj, Evas_Object_Box_Data *priv, void *data)
{
   Edje_Part_Box_Animation *anim = data;

   if (EINA_DBL_EQ(anim->progress, 0.0))
     {
        if (anim->start.layout)
          {
             evas_object_box_padding_set(obj, anim->start.padding.x, anim->start.padding.y);
             evas_object_box_align_set(obj, TO_DOUBLE(anim->start.align.x), TO_DOUBLE(anim->start.align.y));
             anim->start.layout(obj, priv, anim->start.data);
          }
        return;
     }

   if (anim->recalculate)
     {
        _edje_box_layout_calculate_coords(obj, priv, anim);
        anim->start_progress = anim->progress;
        anim->recalculate = EINA_FALSE;
     }

   if ((anim->progress > 0) && (anim->start_progress < 1))
     _edje_box_layout_exec(obj, anim);
}

/**
 * @brief Frees the data associated with an Edje box layout animation.
 *
 * This function is responsible for cleaning up all resources allocated for
 * an Edje_Part_Box_Animation structure, including freeing custom layout data
 * for start and end states, and freeing the list of child animation data.
 *
 * @param data A pointer to the Edje_Part_Box_Animation structure to be freed.
 */
void
_edje_box_layout_free_data(void *data)
{
   Edje_Transition_Animation_Data *tad;
   Edje_Part_Box_Animation *anim = data;
   if (anim->start.free_data && anim->start.data)
     anim->start.free_data(anim->start.data);
   if (anim->end.free_data && anim->end.data)
     anim->end.free_data(anim->end.data);
   EINA_LIST_FREE(anim->objs, tad)
     free(tad);
   free(data);
}

/**
 * @brief Creates and initializes a new Edje_Part_Box_Animation structure.
 *
 * Allocates memory for a new Edje_Part_Box_Animation, initializes its fields,
 * and sets up the custom box layout function (_edje_box_layout) on the provided
 * Evas_Object (box).
 *
 * @param box The Evas_Object that will be animated as a box.
 * @return A pointer to the newly created Edje_Part_Box_Animation structure,
 *         or NULL on allocation failure.
 */
Edje_Part_Box_Animation *
_edje_box_layout_anim_new(Evas_Object *box)
{
   Edje_Part_Box_Animation *anim = calloc(1, sizeof(Edje_Part_Box_Animation));
   if (!anim)
     return NULL;

   anim->box = box;
   evas_object_box_layout_set(box, _edje_box_layout, anim, NULL);

   return anim;
}

/**
 * @brief Applies recalculation for an Edje box part during state transitions.
 *
 * This function is called when an Edje part of type CONTAINER (box) needs to
 * update its layout due to a state change or animation. It determines the
 * start and end layout properties based on the current and target Edje descriptions
 * and the animation progress (ep->description_pos).
 *
 * If transitioning (ep->param2 is set and description_pos is not 0), it sets up
 * the 'end' state of the animation using param2_desc and recalculates coordinates.
 * If at the start of a state (description_pos is 0) or if the start layout isn't set,
 * it sets up the 'start' state using chosen_desc.
 *
 * @param ed The Edje object.
 * @param ep The real part being processed.
 * @param p3 Calculation parameters (unused in this function).
 * @param chosen_desc The Edje part description for the current state.
 */
void
_edje_box_recalc_apply(Edje *ed EINA_UNUSED, Edje_Real_Part *ep, Edje_Calc_Params *p3 EINA_UNUSED, Edje_Part_Description_Box *chosen_desc)
{
   Evas_Object_Box_Data *priv;
#if 0
   int min_w, min_h;
#endif
   if ((ep->type != EDJE_RP_TYPE_CONTAINER) ||
       (!ep->typedata.container)) return;

   if ((ep->param2) && (NEQ(ep->description_pos, ZERO)))
     {
        Edje_Part_Description_Box *param2_desc = (Edje_Part_Description_Box *)ep->param2->description;
        if (ep->typedata.container->anim->end.layout == NULL)
          {
             _edje_box_layout_find_all(param2_desc->box.layout, param2_desc->box.alt_layout, &ep->typedata.container->anim->end.layout, &ep->typedata.container->anim->end.data, &ep->typedata.container->anim->end.free_data);
             ep->typedata.container->anim->end.padding.x = param2_desc->box.padding.x;
             ep->typedata.container->anim->end.padding.y = param2_desc->box.padding.y;
             ep->typedata.container->anim->end.align.x = param2_desc->box.align.x;
             ep->typedata.container->anim->end.align.y = param2_desc->box.align.y;

             priv = efl_data_scope_get(ep->object, EVAS_BOX_CLASS);
             if (priv == NULL)
               return;

             evas_object_box_padding_set(ep->object, ep->typedata.container->anim->start.padding.x, ep->typedata.container->anim->start.padding.y);
             evas_object_box_align_set(ep->object, TO_DOUBLE(ep->typedata.container->anim->start.align.x), TO_DOUBLE(ep->typedata.container->anim->start.align.y));
             ep->typedata.container->anim->start.layout(ep->object, priv, ep->typedata.container->anim->start.data);
             _edje_box_layout_calculate_coords(ep->object, priv, ep->typedata.container->anim);
             ep->typedata.container->anim->start_progress = 0.0;
          }
        evas_object_smart_changed(ep->object);
     }
   else
     {
        ep->typedata.container->anim->end.layout = NULL;
     }

   if (EINA_DBL_EQ(ep->description_pos, 0.0) || !ep->typedata.container->anim->start.layout)
     {
        _edje_box_layout_find_all(chosen_desc->box.layout, chosen_desc->box.alt_layout, &ep->typedata.container->anim->start.layout, &ep->typedata.container->anim->start.data, &ep->typedata.container->anim->start.free_data);
        ep->typedata.container->anim->start.padding.x = chosen_desc->box.padding.x;
        ep->typedata.container->anim->start.padding.y = chosen_desc->box.padding.y;
        ep->typedata.container->anim->start.align.x = chosen_desc->box.align.x;
        ep->typedata.container->anim->start.align.y = chosen_desc->box.align.y;
        evas_object_smart_changed(ep->object);
     }

   ep->typedata.container->anim->progress = ep->description_pos;

   if (evas_object_smart_need_recalculate_get(ep->object))
     {
        evas_object_smart_need_recalculate_set(ep->object, 0);
        evas_object_smart_calculate(ep->object);
     }
#if 0 /* Why the hell do we affect part size after resize ??? */
   evas_object_size_hint_combined_min_get(ep->object, &min_w, &min_h);
   if (chosen_desc->box.min.h && (p3->w < min_w))
     p3->w = min_w;
   if (chosen_desc->box.min.v && (p3->h < min_h))
     p3->h = min_h;
#endif
}

/**
 * @brief Adds a child object to an Edje real part that is a box container for animation.
 *
 * When a child object is added to an Edje part that is a box and is being animated,
 * this function creates an Edje_Transition_Animation_Data structure for the child,
 * adds it to the animation's list of objects, and flags that recalculation is needed.
 *
 * @param rp The Edje_Real_Part representing the box container.
 * @param child_obj The Evas_Object being added as a child.
 * @return EINA_TRUE if the child was successfully added for animation,
 *         EINA_FALSE otherwise (e.g., if rp is not a container or memory allocation fails).
 */
Eina_Bool
_edje_box_layout_add_child(Edje_Real_Part *rp, Evas_Object *child_obj)
{
   Edje_Transition_Animation_Data *tad;

   if ((rp->type != EDJE_RP_TYPE_CONTAINER) ||
       (!rp->typedata.container)) return EINA_FALSE;
   tad = calloc(1, sizeof(Edje_Transition_Animation_Data));
   if (!tad) return EINA_FALSE;
   tad->obj = child_obj;
   rp->typedata.container->anim->objs = eina_list_append(rp->typedata.container->anim->objs, tad);
   rp->typedata.container->anim->recalculate = EINA_TRUE;
   return EINA_TRUE;
}

/**
 * @brief Removes a child object from an Edje real part's box layout animation.
 *
 * When a child object is removed from an Edje part that is a box and is being animated,
 * this function finds and removes the corresponding Edje_Transition_Animation_Data
 * from the animation's list of objects and flags that recalculation is needed.
 *
 * @param rp The Edje_Real_Part representing the box container.
 * @param child_obj The Evas_Object being removed.
 */
void
_edje_box_layout_remove_child(Edje_Real_Part *rp, Evas_Object *child_obj)
{
   Eina_List *l;
   Edje_Transition_Animation_Data *tad;

   if ((rp->type != EDJE_RP_TYPE_CONTAINER) ||
       (!rp->typedata.container)) return;
   EINA_LIST_FOREACH(rp->typedata.container->anim->objs, l, tad)
     {
        if (tad->obj == child_obj)
          {
             free(eina_list_data_get(l));
             rp->typedata.container->anim->objs = eina_list_remove_list(rp->typedata.container->anim->objs, l);
             rp->typedata.container->anim->recalculate = EINA_TRUE;
             break;
          }
     }
   rp->typedata.container->anim->recalculate = EINA_TRUE;
}

