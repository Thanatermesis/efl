/* Legacy only */

#include "evas_common_private.h"
#include "evas_private.h"

#define CSO_DATA_GET(eo_obj, ptr)                                           \
  Evas_Object_Smart_Clipped_Data *ptr = evas_object_smart_data_get(eo_obj);

#define CSO_DATA_GET_OR_RETURN(eo_obj, ptr, ...) \
  CSO_DATA_GET(eo_obj, ptr) \
  if (!ptr) return __VA_ARGS__;

/**
 * @brief Gets the clipper object for a smart clipped object.
 *
 * This function returns the Evas_Object that is acting as the clipper
 * for the given smart clipped object @p eo_obj.
 *
 * @param[in] eo_obj The smart clipped object.
 * @return The clipper object, or @c NULL if none is set or on error.
 */
EVAS_API Evas_Object *
evas_object_smart_clipped_clipper_get(const Evas_Object *eo_obj)
{
   CSO_DATA_GET_OR_RETURN(eo_obj, cso, NULL);

   return cso->clipper;
}

/**
 * @internal
 * @brief Smart callback for when a smart clipped object is added.
 *
 * Initializes the smart clipped object. This is typically called by Evas
 * when the object is created.
 *
 * @param eo_obj The Evas_Object being added.
 */
static void
evas_object_smart_clipped_smart_add(Evas_Object *eo_obj)
{
   _evas_object_smart_clipped_init(eo_obj);
}

/**
 * @internal
 * @brief Smart callback for when a smart clipped object is deleted.
 *
 * Cleans up resources used by the smart clipped object, such as unsetting
 * its clipper and deleting all its members. This is typically called by Evas
 * when the object is about to be destroyed.
 *
 * @param eo_obj The Evas_Object being deleted.
 */
static void
evas_object_smart_clipped_smart_del(Evas_Object *eo_obj)
{
   CSO_DATA_GET_OR_RETURN(eo_obj, cso);

   _efl_canvas_group_group_members_all_del(eo_obj);
   cso->clipper = NULL;
}

/**
 * @internal
 * @brief Smart callback for when a smart clipped object is moved.
 *
 * Handles the move operation for the smart clipped object. This function
 * ensures that the object is an instance of EFL_CANVAS_GROUP_CLASS before
 * proceeding with the internal move logic.
 *
 * @param eo_obj The Evas_Object being moved.
 * @param x The new X coordinate.
 * @param y The new Y coordinate.
 */
void
evas_object_smart_clipped_smart_move(Evas_Object *eo_obj, Evas_Coord x, Evas_Coord y)
{
   if (!efl_isa(eo_obj, EFL_CANVAS_GROUP_CLASS)) return;
   _evas_object_smart_clipped_smart_move_internal(eo_obj, x, y);
}

/**
 * @internal
 * @brief Smart callback for when a smart clipped object is shown.
 *
 * If the clipper object has clippees, this function shows the clipper.
 * This ensures that the clipping effect becomes visible when the smart
 * object itself is shown.
 *
 * @param eo_obj The Evas_Object being shown.
 */
static void
evas_object_smart_clipped_smart_show(Evas_Object *eo_obj)
{
   CSO_DATA_GET_OR_RETURN(eo_obj, cso);
   if (evas_object_clipees_has(cso->clipper))
     evas_object_show(cso->clipper); /* just show if clipper being used */
}

/**
 * @internal
 * @brief Smart callback for when a smart clipped object is hidden.
 *
 * Hides the clipper object associated with the smart clipped object.
 * This ensures that the clipping effect is removed when the smart
 * object itself is hidden.
 *
 * @param eo_obj The Evas_Object being hidden.
 */
static void
evas_object_smart_clipped_smart_hide(Evas_Object *eo_obj)
{
   CSO_DATA_GET_OR_RETURN(eo_obj, cso);
   evas_object_hide(cso->clipper);
}

/**
 * @internal
 * @brief Smart callback for setting the color of a smart clipped object.
 *
 * Sets the color of the clipper object. The color is specified by
 * red (r), green (g), blue (b), and alpha (a) components.
 *
 * @param eo_obj The Evas_Object whose color is being set.
 * @param r The red component (0-255).
 * @param g The green component (0-255).
 * @param b The blue component (0-255).
 * @param a The alpha component (0-255).
 */
static void
evas_object_smart_clipped_smart_color_set(Evas_Object *eo_obj, int r, int g, int b, int a)
{
   CSO_DATA_GET_OR_RETURN(eo_obj, cso);
   evas_object_color_set(cso->clipper, r, g, b, a);
}

/**
 * @internal
 * @brief Smart callback for setting a clip on a smart clipped object.
 *
 * Sets the clip for the clipper object associated with @p eo_obj.
 * This means the clipper itself will be clipped by @p clip.
 *
 * @param eo_obj The smart clipped object.
 * @param clip The Evas_Object to use as a clip.
 */
static void
evas_object_smart_clipped_smart_clip_set(Evas_Object *eo_obj, Evas_Object *clip)
{
   CSO_DATA_GET_OR_RETURN(eo_obj, cso);
   evas_object_clip_set(cso->clipper, clip);
}

/**
 * @internal
 * @brief Smart callback for unsetting a clip on a smart clipped object.
 *
 * Unsets the clip from the clipper object associated with @p eo_obj.
 *
 * @param eo_obj The smart clipped object.
 */
static void
evas_object_smart_clipped_smart_clip_unset(Evas_Object *eo_obj)
{
   CSO_DATA_GET_OR_RETURN(eo_obj, cso);
   evas_object_clip_unset(cso->clipper);
}

/**
 * @internal
 * @brief Smart callback for adding a member to a smart clipped object.
 *
 * When a member is added to the smart clipped object, this function sets
 * the smart object's internal clipper on the new @p member.
 * If the smart object is visible, it also ensures the clipper is shown.
 *
 * @param eo_obj The smart clipped object (parent).
 * @param member The Evas_Object being added as a member.
 */
static void
evas_object_smart_clipped_smart_member_add(Evas_Object *eo_obj, Evas_Object *member)
{
   CSO_DATA_GET_OR_RETURN(eo_obj, cso);
   if (!cso->clipper || member == cso->clipper)
     return;
   evas_object_clip_set(member, cso->clipper);
   if (evas_object_visible_get(eo_obj))
     evas_object_show(cso->clipper);
}

/**
 * @internal
 * @brief Smart callback for deleting a member from a smart clipped object.
 *
 * When a member is removed from the smart clipped object, this function
 * unsets the smart object's internal clipper from the @p member.
 * If the clipper no longer has any clippees, it is hidden.
 *
 * @param eo_obj The smart clipped object (parent).
 * @param member The Evas_Object being removed as a member.
 */
static void
evas_object_smart_clipped_smart_member_del(Evas_Object *eo_obj, Evas_Object *member)
{
   CSO_DATA_GET_OR_RETURN(eo_obj, cso);
   if (!cso->clipper)
     return;
   evas_object_clip_unset(member);
   if (!evas_object_clipees_has(cso->clipper))
     evas_object_hide(cso->clipper);
}

/**
 * @brief Sets the smart functions for an Evas_Smart_Class to implement clipped smart object behavior.
 *
 * This function populates the provided Evas_Smart_Class structure @p sc
 * with the standard callback functions for a clipped smart object.
 * These callbacks handle operations like add, delete, move, show, hide,
 * color set, clip set/unset, and member add/delete.
 *
 * @param[out] sc The Evas_Smart_Class structure to populate. Must not be @c NULL.
 */
EVAS_API void
evas_object_smart_clipped_smart_set(Evas_Smart_Class *sc)
{
   if (!sc)
     return;

   sc->add = evas_object_smart_clipped_smart_add;
   sc->del = evas_object_smart_clipped_smart_del;
   sc->move = evas_object_smart_clipped_smart_move;
   sc->show = evas_object_smart_clipped_smart_show;
   sc->hide = evas_object_smart_clipped_smart_hide;
   sc->color_set = evas_object_smart_clipped_smart_color_set;
   sc->clip_set = evas_object_smart_clipped_smart_clip_set;
   sc->clip_unset = evas_object_smart_clipped_smart_clip_unset;
   sc->calculate = NULL;
   sc->member_add = evas_object_smart_clipped_smart_member_add;
   sc->member_del = evas_object_smart_clipped_smart_member_del;
}

/**
 * @brief Retrieves the Evas_Smart_Class for clipped smart objects.
 *
 * This function returns a pointer to a static Evas_Smart_Class structure
 * that defines the behavior of clipped smart objects. This class is
 * initialized with the name "EvasObjectSmartClipped" and the appropriate
 * smart callback functions.
 *
 * The first time this function is called, it initializes the smart class.
 * Subsequent calls return the already initialized class.
 *
 * @return A pointer to the constant Evas_Smart_Class for clipped smart objects.
 */
EVAS_API const Evas_Smart_Class *
evas_object_smart_clipped_class_get(void)
{
   static Evas_Smart_Class _sc = EVAS_SMART_CLASS_INIT_NAME_VERSION("EvasObjectSmartClipped");
   static const Evas_Smart_Class *class = NULL;

   if (class)
     return class;

   evas_object_smart_clipped_smart_set(&_sc);
   class = &_sc;
   return class;
}
