#include "evas_common_private.h"
#include "evas_private.h"
#include <errno.h>

#define MY_CLASS EVAS_GRID_CLASS

typedef struct _Evas_Grid_Data              Evas_Grid_Data;
typedef struct _Evas_Object_Grid_Option     Evas_Object_Grid_Option;
typedef struct _Evas_Object_Grid_Iterator   Evas_Object_Grid_Iterator;
typedef struct _Evas_Object_Grid_Accessor   Evas_Object_Grid_Accessor;

/**
 * @brief Structure to hold options for an object within an Evas_Object_Grid.
 *
 * This structure stores the Evas_Object itself, its position (x, y) and
 * dimensions (w, h) within the virtual grid, and a pointer to its
 * corresponding Eina_List node in the grid's children list.
 */
struct _Evas_Object_Grid_Option
{
   Evas_Object *obj; /**< The child Evas_Object. */
   Eina_List *l; /**< Pointer to the Eina_List node containing this option in the grid's children list. */
   int x, y, w, h; /**< Virtual x, y coordinates and width, height of the child in grid cells. */
};

/**
 * @brief Private data for the Evas_Object_Grid smart object.
 *
 * This structure holds all internal data for a grid object, including its
 * children, virtual grid size, and mirroring state.
 */
struct _Evas_Grid_Data
{
   Evas_Object_Smart_Clipped_Data base; /**< Base smart clipped data. */
   Eina_List *children; /**< List of Evas_Object_Grid_Option for packed children. */
   struct {
      int w, h; /**< Width and height of the virtual grid in cells. */
   } size; /**< Virtual size of the grid. */
   Eina_Bool is_mirrored : 1; /**< EINA_TRUE if the grid layout is mirrored (RTL), EINA_FALSE otherwise. */
};

/**
 * @brief Iterator structure for Evas_Object_Grid children.
 *
 * This structure wraps a real Eina_Iterator to iterate over the
 * child objects packed into the grid.
 */
struct _Evas_Object_Grid_Iterator
{
   Eina_Iterator iterator; /**< The public Eina_Iterator interface. */

   Eina_Iterator *real_iterator; /**< The underlying Eina_List iterator. */
   const Evas_Object *grid; /**< The grid object being iterated. */
};

/**
 * @brief Accessor structure for Evas_Object_Grid children.
 *
 * This structure wraps a real Eina_Accessor to provide indexed
 * access to child objects packed into the grid.
 */
struct _Evas_Object_Grid_Accessor
{
   Eina_Accessor accessor; /**< The public Eina_Accessor interface. */

   Eina_Accessor *real_accessor; /**< The underlying Eina_List accessor. */
   const Evas_Object *grid; /**< The grid object being accessed. */
};

#define EVAS_OBJECT_GRID_DATA_GET(o, ptr)			\
  Evas_Grid_Data *ptr = efl_data_scope_get(o, MY_CLASS)

#define EVAS_OBJECT_GRID_DATA_GET_OR_RETURN(o, ptr)			\
  EVAS_OBJECT_GRID_DATA_GET(o, ptr);					\
  if (!ptr)								\
    {									\
      ERR("No widget data for object %p (%s)",				\
	   o, evas_object_type_get(o));					\
       return;								\
    }

#define EVAS_OBJECT_GRID_DATA_GET_OR_RETURN_VAL(o, ptr, val)		\
  EVAS_OBJECT_GRID_DATA_GET(o, ptr);					\
  if (!ptr)								\
    {									\
       ERR("No widget data for object %p (%s)",	                \
	       o, evas_object_type_get(o));				\
       return val;							\
    }

static const char EVAS_OBJECT_GRID_OPTION_KEY[] = "|EvGd"; /**< Key for storing Evas_Object_Grid_Option on a child object. */

/**
 * @brief Advances the grid iterator to the next child object.
 * @param it The grid iterator.
 * @param data Pointer to store the next Evas_Object.
 * @return EINA_TRUE if successful, EINA_FALSE otherwise (end of iteration).
 */
static Eina_Bool
_evas_object_grid_iterator_next(Evas_Object_Grid_Iterator *it, void **data)
{
   Evas_Object_Grid_Option *opt;

   if (!eina_iterator_next(it->real_iterator, (void **)&opt))
     return EINA_FALSE;
   if (data) *data = opt->obj;
   return EINA_TRUE;
}

/**
 * @brief Gets the container (the grid object) of the iterator.
 * @param it The grid iterator.
 * @return The Evas_Object that is the grid.
 */
static Evas_Object *
_evas_object_grid_iterator_get_container(Evas_Object_Grid_Iterator *it)
{
   return (Evas_Object *)it->grid;
}

/**
 * @brief Frees the grid iterator.
 * @param it The grid iterator to free.
 */
static void
_evas_object_grid_iterator_free(Evas_Object_Grid_Iterator *it)
{
   eina_iterator_free(it->real_iterator);
   free(it);
}

/**
 * @brief Gets the child object at a specific index using the accessor.
 * @param it The grid accessor.
 * @param idx The index of the child object to retrieve.
 * @param data Pointer to store the Evas_Object at the given index.
 * @return EINA_TRUE if successful, EINA_FALSE otherwise (index out of bounds).
 */
static Eina_Bool
_evas_object_grid_accessor_get_at(Evas_Object_Grid_Accessor *it, unsigned int idx, void **data)
{
   Evas_Object_Grid_Option *opt = NULL;

   if (!eina_accessor_data_get(it->real_accessor, idx, (void **)&opt))
     return EINA_FALSE;
   if (data) *data = opt->obj;
   return EINA_TRUE;
}

/**
 * @brief Gets the container (the grid object) of the accessor.
 * @param it The grid accessor.
 * @return The Evas_Object that is the grid.
 */
static Evas_Object *
_evas_object_grid_accessor_get_container(Evas_Object_Grid_Accessor *it)
{
   return (Evas_Object *)it->grid;
}

/**
 * @brief Frees the grid accessor.
 * @param it The grid accessor to free.
 */
static void
_evas_object_grid_accessor_free(Evas_Object_Grid_Accessor *it)
{
   eina_accessor_free(it->real_accessor);
   free(it);
}

/**
 * @brief Retrieves the Evas_Object_Grid_Option associated with a child object.
 * @param o The child Evas_Object.
 * @return The Evas_Object_Grid_Option for the child, or NULL if not found.
 */
static Evas_Object_Grid_Option *
_evas_object_grid_option_get(Evas_Object *o)
{
   return evas_object_data_get(o, EVAS_OBJECT_GRID_OPTION_KEY);
}

/**
 * @brief Associates an Evas_Object_Grid_Option with a child object.
 * @param o The child Evas_Object.
 * @param opt The Evas_Object_Grid_Option to set.
 */
static void
_evas_object_grid_option_set(Evas_Object *o, const Evas_Object_Grid_Option *opt)
{
   evas_object_data_set(o, EVAS_OBJECT_GRID_OPTION_KEY, opt);
}

/**
 * @brief Removes and returns the Evas_Object_Grid_Option associated with a child object.
 * @param o The child Evas_Object.
 * @return The removed Evas_Object_Grid_Option, or NULL if not found.
 */
static Evas_Object_Grid_Option *
_evas_object_grid_option_del(Evas_Object *o)
{
   return evas_object_data_del(o, EVAS_OBJECT_GRID_OPTION_KEY);
}

/**
 * @brief Callback invoked when a child object of the grid is deleted.
 *
 * This function ensures the child is properly unpacked from the grid
 * before it is fully deleted.
 * @param data The grid Evas_Object (passed as user data).
 * @param evas The Evas canvas.
 * @param child The child Evas_Object being deleted.
 * @param einfo Event-specific information (unused).
 */
static void
_on_child_del(void *data, Evas *evas EINA_UNUSED, Evas_Object *child, void *einfo EINA_UNUSED)
{
   Evas_Object *grid = data;
   evas_object_grid_unpack(grid, child);
}

/**
 * @brief Connects a child object to the grid.
 *
 * This involves adding a EVAS_CALLBACK_DEL listener to the child
 * so that it can be automatically unpacked if deleted externally.
 * @param o The grid Evas_Object.
 * @param child The child Evas_Object to connect.
 */
static void
_evas_object_grid_child_connect(Evas_Object *o, Evas_Object *child)
{
   evas_object_event_callback_add
     (child, EVAS_CALLBACK_DEL, _on_child_del, o);
}

/**
 * @brief Disconnects a child object from the grid.
 *
 * This removes the EVAS_CALLBACK_DEL listener from the child.
 * @param o The grid Evas_Object.
 * @param child The child Evas_Object to disconnect.
 */
static void
_evas_object_grid_child_disconnect(Evas_Object *o, Evas_Object *child)
{
   evas_object_event_callback_del_full
     (child, EVAS_CALLBACK_DEL, _on_child_del, o);
}

EVAS_SMART_SUBCLASS_NEW("Evas_Object_Grid", _evas_object_grid,
			Evas_Smart_Class, Evas_Smart_Class,
			evas_object_smart_clipped_class_get, NULL)

/**
 * @brief Smart callback for adding an Evas_Object_Grid.
 *
 * Initializes the private data for the grid, sets default virtual size.
 * This function is called when a new grid object is created.
 * @param o The Evas_Object_Grid being added.
 */
static void
_evas_object_grid_smart_add(Evas_Object *o)
{
   Evas_Object_Smart_Clipped_Data *base;
   Evas_Grid_Data *priv;

   // Grid is an ugly mix of legacy & eo...
   base = evas_object_smart_data_get(o);
   priv = efl_data_scope_get(o, MY_CLASS);
   priv->base = *base;
   evas_object_smart_data_set(o, priv);

   priv->size.w = 100;
   priv->size.h = 100;
}

/**
 * @brief Smart callback for deleting an Evas_Object_Grid.
 *
 * Cleans up all resources used by the grid, including its children
 * and clipper. This function is called when a grid object is deleted.
 * @param o The Evas_Object_Grid being deleted.
 */
static void
_evas_object_grid_smart_del(Evas_Object *o)
{
   EVAS_OBJECT_GRID_DATA_GET(o, priv);

   while (priv->children)
     {
        Evas_Object_Grid_Option *opt = priv->children->data;
        _evas_object_grid_child_disconnect(o, opt->obj);
        _evas_object_grid_option_del(opt->obj);
        free(opt);
        priv->children = eina_list_remove_list(priv->children, priv->children);
     }
   //Free the clipper resource properly,
   Eo *clipper = evas_object_smart_clipped_clipper_get(o);
   if (clipper) evas_object_del(clipper);
   /*  below deletion occurs the recursive member remove hell. */
   //   _evas_object_grid_parent_sc->del(o);
}

/**
 * @brief Smart callback for resizing an Evas_Object_Grid.
 *
 * This function is called when the grid object's own geometry changes.
 * It marks the grid as changed, which will trigger a recalculation
 * of its children's positions and sizes.
 * @param o The Evas_Object_Grid being resized.
 * @param w The new width of the grid.
 * @param h The new height of the grid.
 */
static void
_evas_object_grid_smart_resize(Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   Evas_Coord ow, oh;
   evas_object_geometry_get(o, NULL, NULL, &ow, &oh);
   if ((ow == w) && (oh == h)) return;
   evas_object_smart_changed(o);
}

/**
 * @brief Smart callback for calculating the layout of an Evas_Object_Grid.
 *
 * This function is responsible for positioning and sizing all child objects
 * within the grid based on their pack options and the grid's current
 * dimensions and virtual size. It handles mirroring (RTL layouts) as well.
 * @param o The Evas_Object_Grid to calculate.
 */
static void
_evas_object_grid_smart_calculate(Evas_Object *o)
{
   Eina_List *l;
   Evas_Object_Grid_Option *opt;
   Evas *e;
   Evas_Coord x, y, w, h;
   long long xl, yl, wl, hl, vwl, vhl;
   Eina_Bool mirror;

   EVAS_OBJECT_GRID_DATA_GET_OR_RETURN(o, priv);
   if (!priv->children) return;

   e = evas_object_evas_get(o);
   evas_event_freeze(e);

   evas_object_geometry_get(o, &x, &y, &w, &h);
   xl = x;
   yl = y;
   wl = w;
   hl = h;
   mirror = priv->is_mirrored;
   vwl = priv->size.w;
   vhl = priv->size.h;
   EINA_LIST_FOREACH(priv->children, l, opt)
     {
        long long x1, y1, x2, y2;

        if (vwl > 0)
          {
             if (!mirror)
               {
                  x1 = xl + ((wl * (long long)opt->x) / vwl);
                  x2 = xl + ((wl * (long long)(opt->x + opt->w)) / vwl);
               }
             else
               {
                  x1 = xl + ((wl * (vwl - (long long)(opt->x + opt->w))) / vwl);
                  x2 = xl + ((wl * (vwl - (long long)opt->x)) / vwl);
               }
          }
        else
          {
             x1 = xl;
             x2 = xl;
          }
        if (vhl > 0)
          {
             y1 = yl + ((hl * (long long)opt->y) / vhl);
             y2 = yl + ((hl * (long long)(opt->y + opt->h)) / vhl);
          }
        else
          {
             y1 = yl;
             y2 = yl;
          }
        evas_object_move(opt->obj, x1, y1);
        evas_object_resize(opt->obj, x2 - x1, y2 - y1);
     }

   evas_event_thaw(e);
}

/**
 * @brief Sets up the user-defined smart class functions for Evas_Object_Grid.
 *
 * This function assigns the custom add, del, resize, and calculate
 * callbacks to the Evas_Smart_Class structure.
 * @param sc The Evas_Smart_Class to configure.
 */
static void
_evas_object_grid_smart_set_user(Evas_Smart_Class *sc)
{
   sc->add = _evas_object_grid_smart_add;
   sc->del = _evas_object_grid_smart_del;
   sc->resize = _evas_object_grid_smart_resize;
   sc->calculate = _evas_object_grid_smart_calculate;
}

/**
 * @brief Adds a new Evas_Object_Grid to the given Evas canvas.
 * @param evas The Evas canvas to add the grid to.
 * @return A new Evas_Object_Grid instance, or NULL on failure.
 * @ingroup Evas_Object_Grid_Group
 */
EVAS_API Evas_Object *
evas_object_grid_add(Evas *evas)
{
   evas = evas_find(evas);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(evas, EVAS_CANVAS_CLASS), NULL);
   return efl_add(MY_CLASS, evas, efl_canvas_object_legacy_ctor(efl_added));
}

/**
 * @internal
 * @brief EFL object constructor for Evas_Grid.
 *
 * Initializes the Evas_Grid object. Sets it as a clipped group
 * and attaches the legacy smart class.
 * @param obj The Evas_Grid object.
 * @param class_data Private data for the class (unused).
 * @return The constructed Evas_Grid object.
 */
EOLIAN static Eo *
_evas_grid_efl_object_constructor(Eo *obj, Evas_Grid_Data *class_data EINA_UNUSED)
{
   efl_canvas_group_clipped_set(obj, EINA_TRUE);
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   evas_object_smart_attach(obj, _evas_object_grid_smart_class_new());

   return obj;
}

/**
 * @internal
 * @brief Adds a new Evas_Grid as a child of a parent Evas object.
 *
 * This is typically used in scenarios where a grid is part of a larger UI
 * hierarchy defined in EO.
 * @param parent The parent Evas object.
 * @param _pd Private data of the grid (unused).
 * @return The new Evas_Grid object, or NULL on failure.
 */
EOLIAN static Evas_Object*
_evas_grid_add_to(Eo *parent, Evas_Grid_Data *_pd EINA_UNUSED)
{
   Evas *evas;
   Evas_Object *ret;
   evas = evas_object_evas_get(parent);
   ret = evas_object_grid_add(evas);
   evas_object_smart_member_add(ret, parent);

   return ret;
}

/**
 * @internal
 * @brief Sets the virtual size of the grid.
 *
 * The virtual size defines the number of rows and columns in the grid.
 * Child objects are positioned and sized relative to this virtual grid.
 * @param o The Evas_Grid object.
 * @param priv The private data of the grid.
 * @param w The virtual width (number of columns).
 * @param h The virtual height (number of rows).
 */
EOLIAN static void
_evas_grid_grid_size_set(Eo *o, Evas_Grid_Data *priv, int w, int h)
{
   if ((priv->size.w == w) && (priv->size.h == h)) return;
   priv->size.w = w;
   priv->size.h = h;
   evas_object_smart_changed(o);
}

/**
 * @internal
 * @brief Gets the virtual size of the grid.
 * @param o The Evas_Grid object (unused).
 * @param priv The private data of the grid.
 * @param w Pointer to store the virtual width.
 * @param h Pointer to store the virtual height.
 */
EOLIAN static void
_evas_grid_grid_size_get(const Eo *o EINA_UNUSED, Evas_Grid_Data *priv, int *w, int *h)
{
   if (w) *w = priv->size.w;
   if (h) *h = priv->size.h;
}

/**
 * @internal
 * @brief Packs a child object into the grid.
 *
 * The child is placed at the specified virtual coordinates (x, y) and
 * given the specified virtual dimensions (w, h) in grid cells.
 * @param o The Evas_Grid object.
 * @param priv The private data of the grid.
 * @param child The Evas_Object to pack.
 * @param x The virtual x-coordinate (column).
 * @param y The virtual y-coordinate (row).
 * @param w The virtual width (number of columns spanned).
 * @param h The virtual height (number of rows spanned).
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., memory allocation).
 */
EOLIAN static Eina_Bool
_evas_grid_pack(Eo *o, Evas_Grid_Data *priv, Evas_Object *child, int x, int y, int w, int h)
{
   Evas_Object_Grid_Option *opt;
   Eina_Bool newobj = EINA_FALSE;

   opt = _evas_object_grid_option_get(child);
   if (!opt)
     {
        opt = malloc(sizeof(*opt));
        if (!opt)
          {
             ERR("could not allocate grid option data.");
             return EINA_FALSE;
          }
        newobj = EINA_TRUE;
     }

   opt->x = x;
   opt->y = y;
   opt->w = w;
   opt->h = h;

   if (newobj)
     {
        opt->obj = child;
        priv->children = eina_list_append(priv->children, opt);
        opt->l = eina_list_last(priv->children);
        _evas_object_grid_option_set(child, opt);
        evas_object_smart_member_add(child, o);
        _evas_object_grid_child_connect(o, child);
     }
   // FIXME: we could keep a changed list
   evas_object_smart_changed(o);

   return EINA_TRUE;
}

/**
 * @brief Helper function to remove a grid option from the children list.
 * @param priv The private data of the grid.
 * @param opt The Evas_Object_Grid_Option to remove.
 */
static void
_evas_object_grid_remove_opt(Evas_Grid_Data *priv, Evas_Object_Grid_Option *opt)
{
   priv->children = eina_list_remove_list(priv->children, opt->l);
   opt->l = NULL;
}

/**
 * @internal
 * @brief Unpacks (removes) a child object from the grid.
 * @param o The Evas_Grid object.
 * @param priv The private data of the grid.
 * @param child The Evas_Object to unpack.
 * @return EINA_TRUE on success, EINA_FALSE if the child is not part of this grid or has no packing options.
 */
EOLIAN static Eina_Bool
_evas_grid_unpack(Eo *o, Evas_Grid_Data *priv, Evas_Object *child)
{
   Evas_Object_Grid_Option *opt;

   if (o != evas_object_smart_parent_get(child))
     {
	ERR("cannot unpack child from incorrect grid!");
        return EINA_FALSE;
     }

   opt = _evas_object_grid_option_del(child);
   if (!opt)
     {
	ERR("cannot unpack child with no packing option!");
        return EINA_FALSE;
     }

   _evas_object_grid_child_disconnect(o, child);
   _evas_object_grid_remove_opt(priv, opt);
   evas_object_smart_member_del(child);
   free(opt);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Clears all children from the grid.
 *
 * Unpacks all child objects. If 'clear' is EINA_TRUE, the child objects
 * are also deleted.
 * @param o The Evas_Grid object.
 * @param priv The private data of the grid.
 * @param clear If EINA_TRUE, delete the child objects; otherwise, just unpack them.
 */
EOLIAN static void
_evas_grid_clear(Eo *o, Evas_Grid_Data *priv, Eina_Bool clear)
{
   Evas_Object_Grid_Option *opt;

   EINA_LIST_FREE(priv->children, opt)
     {
	_evas_object_grid_child_disconnect(o, opt->obj);
	_evas_object_grid_option_del(opt->obj);
	evas_object_smart_member_del(opt->obj);
	if (clear)
	  evas_object_del(opt->obj);
	free(opt);
     }
}

/**
 * @internal
 * @brief Retrieves the packing options for a child object.
 * @param o The Evas_Grid object (unused).
 * @param _pd Private data of the grid (unused).
 * @param child The child Evas_Object.
 * @param x Pointer to store the virtual x-coordinate.
 * @param y Pointer to store the virtual y-coordinate.
 * @param w Pointer to store the virtual width.
 * @param h Pointer to store the virtual height.
 * @return EINA_TRUE if packing options were found and retrieved, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_evas_grid_pack_get(const Eo *o EINA_UNUSED, Evas_Grid_Data *_pd EINA_UNUSED, Evas_Object *child, int *x, int *y, int *w, int *h)
{
   Evas_Object_Grid_Option *opt;

   if (x) *x = 0;
   if (y) *y = 0;
   if (w) *w = 0;
   if (h) *h = 0;
   opt = _evas_object_grid_option_get(child);
   if (!opt) return 0;
   if (x) *x = opt->x;
   if (y) *y = opt->y;
   if (w) *w = opt->w;
   if (h) *h = opt->h;

   return 1;
}

/**
 * @internal
 * @brief Creates a new iterator for the grid's children.
 *
 * The iterator allows sequential access to the Evas_Object children
 * packed into the grid.
 * @param o The Evas_Grid object.
 * @param priv The private data of the grid.
 * @return A new Eina_Iterator, or NULL if the grid has no children or on allocation failure.
 * The caller is responsible for freeing the iterator using eina_iterator_free().
 */
EOLIAN static Eina_Iterator*
_evas_grid_iterator_new(const Eo *o, Evas_Grid_Data *priv)
{
   Evas_Object_Grid_Iterator *it;

   if (!priv->children) return NULL;

   it = calloc(1, sizeof(Evas_Object_Grid_Iterator));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->real_iterator = eina_list_iterator_new(priv->children);
   it->grid = o;

   it->iterator.next = FUNC_ITERATOR_NEXT(_evas_object_grid_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(_evas_object_grid_iterator_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_evas_object_grid_iterator_free);

   return &it->iterator;
}

/**
 * @internal
 * @brief Creates a new accessor for the grid's children.
 *
 * The accessor allows random access (by index) to the Evas_Object children
 * packed into the grid.
 * @param o The Evas_Grid object.
 * @param priv The private data of the grid.
 * @return A new Eina_Accessor, or NULL if the grid has no children or on allocation failure.
 * The caller is responsible for freeing the accessor using eina_accessor_free().
 */
EOLIAN static Eina_Accessor*
_evas_grid_accessor_new(const Eo *o, Evas_Grid_Data *priv)
{
   Evas_Object_Grid_Accessor *it;

   if (!priv->children) return NULL;

   it = calloc(1, sizeof(Evas_Object_Grid_Accessor));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->accessor, EINA_MAGIC_ACCESSOR);

   it->real_accessor = eina_list_accessor_new(priv->children);
   it->grid = o;

   it->accessor.get_at = FUNC_ACCESSOR_GET_AT(_evas_object_grid_accessor_get_at);
   it->accessor.get_container = FUNC_ACCESSOR_GET_CONTAINER(_evas_object_grid_accessor_get_container);
   it->accessor.free = FUNC_ACCESSOR_FREE(_evas_object_grid_accessor_free);

   return &it->accessor;
}

/**
 * @internal
 * @brief Gets a list of all child objects packed in the grid.
 * @param o The Evas_Grid object (unused).
 * @param priv The private data of the grid.
 * @return A new Eina_List containing Evas_Object pointers of the children.
 * The caller is responsible for freeing this list (e.g., using eina_list_free()),
 * but not the objects contained within it. Returns NULL if there are no children.
 * The order of objects in the list corresponds to their packing order.
 */
EOLIAN static Eina_List*
_evas_grid_children_get(const Eo *o EINA_UNUSED, Evas_Grid_Data *priv)
{
   Eina_List *new_list = NULL, *l;
   Evas_Object_Grid_Option *opt;

   EINA_LIST_FOREACH(priv->children, l, opt)
      new_list = eina_list_append(new_list, opt->obj);

   return new_list;
}

/**
 * @internal
 * @brief Gets the mirrored mode of the grid.
 * @param o The Evas_Grid object (unused).
 * @param priv The private data of the grid.
 * @return EINA_TRUE if mirrored (RTL), EINA_FALSE otherwise (LTR).
 */
EOLIAN static Eina_Bool
_evas_grid_efl_ui_i18n_mirrored_get(const Eo *o EINA_UNUSED, Evas_Grid_Data *priv)
{
   return priv->is_mirrored;
}

/**
 * @internal
 * @brief Sets the mirrored mode of the grid.
 *
 * If mirrored is EINA_TRUE, the grid layout is right-to-left (RTL).
 * Otherwise, it is left-to-right (LTR).
 * @param o The Evas_Grid object.
 * @param priv The private data of the grid.
 * @param mirrored The mirrored mode to set.
 */
EOLIAN static void
_evas_grid_efl_ui_i18n_mirrored_set(Eo *o EINA_UNUSED, Evas_Grid_Data *priv, Eina_Bool mirrored)
{
   mirrored = !!mirrored;
   if (priv->is_mirrored != mirrored)
     {
        priv->is_mirrored = mirrored;
        _evas_object_grid_smart_calculate(o);
     }
}

/**
 * @brief Sets the mirrored mode of the grid.
 * @param obj The Evas_Grid object.
 * @param mirrored If EINA_TRUE, the layout becomes right-to-left.
 *                 Otherwise, it is left-to-right.
 * @ingroup Evas_Object_Grid_Group
 */
EVAS_API void
evas_object_grid_mirrored_set(Evas_Grid *obj, Eina_Bool mirrored)
{
   efl_ui_mirrored_set(obj, mirrored);
}

/**
 * @brief Gets the mirrored mode of the grid.
 * @param obj The Evas_Grid object.
 * @return EINA_TRUE if mirrored, EINA_FALSE otherwise.
 * @ingroup Evas_Object_Grid_Group
 */
EVAS_API Eina_Bool
evas_object_grid_mirrored_get(const Evas_Grid *obj)
{
   return efl_ui_mirrored_get(obj);
}

#include "canvas/evas_grid_eo.c"
