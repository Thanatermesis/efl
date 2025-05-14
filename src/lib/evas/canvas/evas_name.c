#include "evas_common_private.h"
#include "evas_private.h"

/**
 * @brief Sets the name of an Evas object.
 *
 * This function assigns a name to the given Evas object. If the object
 * already has a name, the old name is removed from the canvas's name hash
 * before the new name is set. If the new name is NULL, the object's name
 * is effectively unset.
 *
 * The name is used to identify the object, for example, when searching for
 * objects by name using evas_object_name_find() or
 * evas_object_name_child_find().
 *
 * @param eo_obj The Evas object to name.
 * @param name The name to set. If NULL, the current name is removed.
 */
EVAS_API void
evas_object_name_set(Evas_Object *eo_obj, const char *name)
{
   Evas_Object_Protected_Data *obj = efl_isa(eo_obj, EFL_CANVAS_OBJECT_CLASS) ?
            efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS) : NULL;
   if (!obj) return;
   if (obj->name)
     {
        if (obj->layer && obj->layer->evas && obj->layer->evas->name_hash)
          eina_hash_list_remove(obj->layer->evas->name_hash, obj->name, eo_obj);
        free(obj->name);
     }
   if (!name) obj->name = NULL;
   else
     {
        obj->name = strdup(name);
        if (obj->layer && obj->layer->evas && obj->layer->evas->name_hash)
          eina_hash_list_prepend(obj->layer->evas->name_hash, obj->name, eo_obj);
     }
}

/**
 * @brief Retrieves the name of an Evas object.
 *
 * This function returns the name previously set on the Evas object
 * using evas_object_name_set().
 *
 * @param eo_obj The Evas object whose name is to be retrieved.
 * @return The name of the object, or NULL if the object has no name or
 *         if eo_obj is not a valid Evas object.
 */
EVAS_API const char *
evas_object_name_get(const Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = efl_isa(eo_obj, EFL_CANVAS_OBJECT_CLASS) ?
            efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS) : NULL;
   if (!obj) return NULL;
   return obj->name;
}

/**
 * @internal
 * @brief Finds an Evas object by its name within a given Evas canvas.
 *
 * This function searches for an Evas object within the specified Evas canvas
 * that has the given name. It uses a hash table for efficient lookups.
 *
 * @param eo_e The Evas canvas (Eo object, unused in this specific implementation but part of EOLIAN signature).
 * @param e Pointer to the Evas public data structure, which contains the name hash.
 * @param name The name of the object to find.
 * @return The Evas_Object* if found, otherwise NULL. Returns NULL if name is NULL.
 */
EOLIAN Evas_Object*
_evas_canvas_object_name_find(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e, const char *name)
{
   if (!name) return NULL;
   else return eina_list_data_get(eina_hash_find(e->name_hash, name));
}

/**
 * @internal
 * @brief Finds a child object by name within the smart members of a given Evas object.
 *
 * This function searches for a direct child or, if @p recurse is non-zero,
 * a descendant of @p eo_obj that has the specified @p name.
 * It iterates through the smart members of the object.
 *
 * @param eo_obj The parent Evas object to search within. Must be a smart object (EFL_CANVAS_GROUP_CLASS).
 * @param name The name of the child object to find.
 * @param recurse The number of levels to recurse. If 0, only direct children are searched.
 *                If > 0, recursion depth is limited. If < 0 (e.g., -1), recurses indefinitely.
 * @return The Evas_Object* of the found child, or NULL if not found or if @p eo_obj is not a group.
 */
static Evas_Object *
_priv_evas_object_name_child_find(const Evas_Object *eo_obj, const char *name, int recurse)
{
   const Eina_Inlist *lst;
   Evas_Object_Protected_Data *child;

   if (!efl_isa(eo_obj, EFL_CANVAS_GROUP_CLASS)) return NULL;
   lst = evas_object_smart_members_get_direct(eo_obj);
   EINA_INLIST_FOREACH(lst, child)
     {
        if (child->delete_me) continue;
        if (!child->name) continue;
        if (!strcmp(name, child->name)) return child->object;
        if (recurse != 0)
          {
             if ((eo_obj = _priv_evas_object_name_child_find(child->object, name, recurse - 1)))
               return (Evas_Object *)eo_obj;
          }
     }
   return NULL;
}

/**
 * @brief Finds a child object by name within a given Evas object's hierarchy.
 *
 * This function searches for a child object with the specified @p name, starting
 * from @p eo_obj. The search can be recursive.
 *
 * @param eo_obj The parent Evas object.
 * @param name The name of the child object to find. Cannot be NULL.
 * @param recurse If 0, only direct children are searched.
 *                If > 0, search up to @p recurse levels deep.
 *                If < 0 (e.g. -1), search recursively through all descendants.
 * @return The Evas_Object* of the found child, or NULL if not found or if @p name is NULL.
 */
EVAS_API Evas_Object *
evas_object_name_child_find(const Evas_Object *eo_obj, const char *name, int recurse)
{
   return (!name ?  NULL : _priv_evas_object_name_child_find(eo_obj, name, recurse));
}

/* new in EO */
/**
 * @internal
 * @brief Overrides the debug name string for an Evas object.
 *
 * This function appends detailed state information to the debug name string buffer
 * for an Evas object. This includes visibility, geometry, color, and flags like
 * no_render or clipper. This is typically used for debugging purposes.
 *
 * @param eo_obj The Evas object (Eo handle).
 * @param obj The protected data of the Evas object.
 * @param sb The string buffer to append the debug information to.
 */
EOLIAN void
_efl_canvas_object_efl_object_debug_name_override(Eo *eo_obj, Evas_Object_Protected_Data *obj, Eina_Strbuf *sb)
{
   const char *norend = obj->no_render ? ":no_render" : "";
   const char *clip = obj->clip.clipees ? ":clipper" : "";

   efl_debug_name_override(efl_super(eo_obj, EFL_CANVAS_OBJECT_CLASS), sb);
   if (!obj->cur)
     {
        eina_strbuf_append_printf(sb, ":nostate");
     }
   else if (obj->cur->visible)
     {
        eina_strbuf_append_printf(sb, "%s%s:(%d,%d %dx%d)", norend, clip,
                                  obj->cur->geometry.x, obj->cur->geometry.y,
                                  obj->cur->geometry.w, obj->cur->geometry.h);
        if ((obj->cur->color.r != 255) || (obj->cur->color.g != 255) ||
            (obj->cur->color.b != 255) || (obj->cur->color.a != 255))
          {
             eina_strbuf_append_printf(sb, ":rgba(%d,%d,%d,%d)",
                                       obj->cur->color.r, obj->cur->color.g,
                                       obj->cur->color.b, obj->cur->color.a);
          }
     }
   else
     {
        eina_strbuf_append_printf(sb, ":hidden%s%s", norend, clip);
     }
}
