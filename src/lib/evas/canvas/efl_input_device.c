#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Evas.h"
#define EFL_INTERNAL_UNSTABLE
#include "Evas_Internal.h"
#include "evas_common_private.h"
#include "evas_private.h"

#define MY_CLASS EFL_INPUT_DEVICE_CLASS

/**
 * @brief Private data structure for Efl_Input_Device.
 *
 * This structure holds all the private data for an Efl_Input_Device instance.
 * It includes references to the Eo object itself, the associated Evas canvas,
 * source device, child devices, and grab information.
 */
typedef struct _Efl_Input_Device_Data   Efl_Input_Device_Data;
struct _Efl_Input_Device_Data
{
   Eo               *eo; /**< The Efl_Input_Device object instance. */
   Eo               *evas; /**< The Evas canvas associated with this device. @since 1.22 */
   Efl_Input_Device *source;  /**< The source device this device originates from (e.g., a physical device for a virtual one). This is a reference. */
   Eina_List        *children; /**< A list of child devices. Children are ref'ed by efl_parent, not by this list. This is typically used for seat devices to list their attached physical devices. */
   Eina_Hash        *grabs; /**< A hash table of Evas_Object instances that have grabbed this device for pointer events. Keys are Efl_Canvas_Object*, values are Evas_Object_Pointer_Data*. */
   unsigned int      id; /**< The seat ID for this device. Only relevant if klass is EFL_INPUT_DEVICE_TYPE_SEAT. */
   Efl_Input_Device_Type klass; /**< The type of this input device (e.g., mouse, keyboard, seat). */
   unsigned int      subclass; /**< The Evas_Device_Subclass, indicating a more specific type of device (e.g., finger, stylus for touch). Currently unused by EOLIAN API. */
   unsigned int      pointer_count; /**< Number of pointer devices (mouse, touch, pen, wand) associated with this device. Only relevant if klass is EFL_INPUT_DEVICE_TYPE_SEAT. */
};

/**
 * @brief Private data structure for iterating over child devices.
 *
 * This structure is used to implement an Eina_Iterator for the children
 * of an Efl_Input_Device.
 */
typedef struct _Child_Device_Iterator Child_Device_Iterator;

struct _Child_Device_Iterator
{
   Eina_Iterator  iterator;
   Eina_List     *list;
   Eina_Iterator *real_iterator; /**< The underlying Eina_List iterator. */
   Eo            *object; /**< The parent Efl_Input_Device object whose children are being iterated. */
};

/**
 * @brief Checks if the device is a pointer type.
 *
 * Pointer types include mouse, touch, pen, and wand.
 *
 * @param pd The private data of the Efl_Input_Device.
 * @return EINA_TRUE if the device is a pointer type, EINA_FALSE otherwise.
 */
static Eina_Bool
_is_pointer(Efl_Input_Device_Data *pd)
{
   return (pd->klass == EFL_INPUT_DEVICE_TYPE_MOUSE ||
           pd->klass == EFL_INPUT_DEVICE_TYPE_TOUCH ||
           pd->klass == EFL_INPUT_DEVICE_TYPE_PEN ||
           pd->klass == EFL_INPUT_DEVICE_TYPE_WAND);
}

/**
 * @brief Updates the pointer count of a seat device.
 *
 * If the given device `dev` is a pointer type, the pointer count
 * of the `seat` device is incremented. This is typically called when
 * a new pointer device is added as a child to a seat.
 *
 * @param seat The private data of the seat device.
 * @param dev The private data of the device being checked/added.
 */
static void
_seat_pointers_update(Efl_Input_Device_Data *seat, Efl_Input_Device_Data *dev)
{
   if (seat && _is_pointer(dev))
     seat->pointer_count++;
}

EOLIAN static Efl_Object *
_efl_input_device_efl_object_constructor(Eo *obj, Efl_Input_Device_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd->eo = obj;
   return obj;
}

EOLIAN static void
_efl_input_device_efl_object_destructor(Eo *obj, Efl_Input_Device_Data *pd)
{
   // Free the list of children. Note: children themselves are unref'd by efl_parent_set.
   pd->children = eina_list_free(pd->children);
   if (pd->klass != EFL_INPUT_DEVICE_TYPE_SEAT)
     {
        // If this device is not a seat, remove it from its parent seat's children list.
        Efl_Input_Device_Data *p;
        Eo *seat;

        seat = efl_input_device_seat_get(obj);
        p = efl_data_scope_get(seat, MY_CLASS);
        if (p) p->children = eina_list_remove(p->children, obj);
     }
   // Unreference the source device.
   efl_unref(pd->source);

   // Free the hash table of grabs.
   if (pd->grabs)
     {
        eina_hash_free(pd->grabs);
        pd->grabs = NULL;
     }

   return efl_destructor(efl_super(obj, MY_CLASS));
}

EOLIAN static void
_efl_input_device_efl_object_parent_set(Eo *obj, Efl_Input_Device_Data *pd EINA_UNUSED, Eo *parent)
{
   Efl_Input_Device_Data *p;

   if (parent)
     {
        // If the new parent is an Efl_Input_Device, it must be a SEAT.
        // Add this object to the parent's children list.
        if (efl_isa(parent, MY_CLASS))
          {
             p = efl_data_scope_get(parent, MY_CLASS);
             EINA_SAFETY_ON_FALSE_RETURN(p->klass == EFL_INPUT_DEVICE_TYPE_SEAT); // Parent device must be a seat
             if (!eina_list_data_find(p->children, obj))
               {
                  p->children = eina_list_append(p->children, obj);
                  _seat_pointers_update(p, pd); // Update pointer count on the seat
               }
          }
        // The parent can also be an Efl_Canvas_Scene.
        else if(!efl_isa(parent, EFL_CANVAS_SCENE_INTERFACE))
          {
             EINA_SAFETY_ERROR("The parent of a device must be a seat or the canvas");
             return;
          }
     }
   else
     {
        // If unparenting, remove this device from its old parent's children list.
        Eo *old_parent = efl_parent_get(obj);
        if (old_parent && efl_isa(old_parent, MY_CLASS))
          {
             p = efl_data_scope_get(old_parent, MY_CLASS);
             p->children = eina_list_remove(p->children, obj);
             if (_is_pointer(pd)) // If this was a pointer device, decrement seat's pointer count
               p->pointer_count--;
          }
     }

   efl_parent_set(efl_super(obj, MY_CLASS), parent);
}

EOLIAN static void
_efl_input_device_device_type_set(Eo *obj, Efl_Input_Device_Data *pd, Efl_Input_Device_Type klass)
{
   EINA_SAFETY_ON_TRUE_RETURN(pd->klass); // Device type should only be set once.
   pd->klass = klass;
   // If this device is not a seat itself, and it's a pointer type,
   // update the pointer count of its parent seat.
   if (klass != EFL_INPUT_DEVICE_TYPE_SEAT)
     {
        Efl_Input_Device_Data *seat_pd = efl_data_scope_get(efl_input_device_seat_get(obj), MY_CLASS);
        _seat_pointers_update(seat_pd, pd);
     }
}

EOLIAN static Efl_Input_Device_Type
_efl_input_device_device_type_get(const Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd)
{
   return pd->klass;
}

EOLIAN static void
_efl_input_device_source_set(Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd, Efl_Input_Device *src)
{
   if (pd->source == src) return;
   efl_unref(pd->source); // Unref the old source
   pd->source = efl_ref(src); // Ref the new source
}

EOLIAN static Efl_Input_Device *
_efl_input_device_source_get(const Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd)
{
   return pd->source;
}

EOLIAN static void
_efl_input_device_seat_id_set(Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd, unsigned int id)
{
   // Seat ID can only be set for seat devices.
   EINA_SAFETY_ON_TRUE_RETURN(pd->klass != EFL_INPUT_DEVICE_TYPE_SEAT);
   pd->id = id;
}

EOLIAN static unsigned int
_efl_input_device_seat_id_get(const Eo *obj, Efl_Input_Device_Data *pd)
{
   // If this is a seat, return its own ID.
   if (pd->klass == EFL_INPUT_DEVICE_TYPE_SEAT)
     return pd->id;
   // Otherwise, get the ID from the parent seat.
   return efl_input_device_seat_id_get(efl_input_device_seat_get(obj));
}

EOLIAN static Efl_Input_Device *
_efl_input_device_seat_get(const Eo *obj, Efl_Input_Device_Data *pd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(obj, NULL);

   // If this device is a seat, return itself.
   if (pd->klass == EFL_INPUT_DEVICE_TYPE_SEAT)
     return pd->eo;

   // Traverse up the parent chain to find the seat.
   // A device's parent can be another device (if it's part of a seat) or the canvas.
   while ((obj = efl_parent_get(obj)))
     {
        if (!efl_isa(obj, MY_CLASS)) break; // Stop if parent is not an Efl_Input_Device (e.g., it's the canvas)
        pd = efl_data_scope_get(obj, MY_CLASS);
        if (pd->klass == EFL_INPUT_DEVICE_TYPE_SEAT)
          return pd->eo; // Found the seat
     }

   return NULL; // No seat found in the hierarchy.
}

/**
 * @brief Advances the child device iterator.
 *
 * Implements Eina_Iterator->next.
 *
 * @param it The iterator.
 * @param data Pointer to store the next child device.
 * @return EINA_TRUE if successful, EINA_FALSE if no more items.
 */
static Eina_Bool
_child_device_iterator_next(Child_Device_Iterator *it, void **data)
{
   Eo *sub;

   if (!eina_iterator_next(it->real_iterator, (void **) &sub))
     return EINA_FALSE;

   if (data) *data = sub;
   return EINA_TRUE;
}

/**
 * @brief Gets the container (parent device) of the iterator.
 *
 * Implements Eina_Iterator->get_container.
 *
 * @param it The iterator.
 * @return The Efl_Input_Device that owns these children.
 */
static Eo *
_child_device_iterator_get_container(Child_Device_Iterator *it)
{
   return it->object;
}

/**
 * @brief Frees the child device iterator.
 *
 * Implements Eina_Iterator->free.
 *
 * @param it The iterator to free.
 */
static void
_child_device_iterator_free(Child_Device_Iterator *it)
{
   eina_iterator_free(it->real_iterator);
   free(it);
}

EOLIAN static Eina_Iterator *
_efl_input_device_children_iterate(Eo *obj, Efl_Input_Device_Data *pd)
{
   Child_Device_Iterator *it;

   it = calloc(1, sizeof(*it));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->list = pd->children; // The list of children to iterate
   it->real_iterator = eina_list_iterator_new(it->list); // Underlying list iterator
   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.next = FUNC_ITERATOR_NEXT(_child_device_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(_child_device_iterator_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_child_device_iterator_free);
   it->object = obj; // The parent device object

   return &it->iterator;
}

EOLIAN static int
_efl_input_device_pointer_device_count_get(const Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd)
{
   // Only seats have a meaningful pointer device count.
   if (pd->klass == EFL_INPUT_DEVICE_TYPE_SEAT)
     return pd->pointer_count;
   return -1; // Return -1 for non-seat devices.
}

EOLIAN static Eina_Bool
_efl_input_device_is_pointer_type_get(const Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd)
{
   return _is_pointer(pd);
}

/**
 * @brief Gets the list of child devices. (Internal, for Evas compatibility)
 *
 * @param obj The Efl_Input_Device object.
 * @param pd The private data of the Efl_Input_Device.
 * @return A const Eina_List* of child devices, or NULL.
 *         The list elements are Eo* pointers to child Efl_Input_Device objects.
 *         Example:
 *         If a seat has a mouse and a keyboard:
 *         list -> [Eo* mouse_device, Eo* keyboard_device]
 */
static const Eina_List *
_efl_input_device_children_get(const Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd)
{
   return pd->children;
}

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(efl_input_device_children_get, const Eina_List *, NULL);

/**
 * @brief Gets the Evas canvas associated with this device. (Internal, for Evas compatibility)
 *
 * @param obj The Efl_Input_Device object.
 * @param pd The private data of the Efl_Input_Device.
 * @return The Evas* canvas, or NULL.
 */
static Evas *
_efl_input_device_evas_get(const Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd)
{
   return pd->evas;
}

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(efl_input_device_evas_get, Evas *, NULL);

/**
 * @brief Sets the Evas canvas associated with this device. (Internal, for Evas compatibility)
 *
 * @param obj The Efl_Input_Device object.
 * @param pd The private data of the Efl_Input_Device.
 * @param e The Evas* canvas to associate.
 */
static void
_efl_input_device_evas_set(Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd, Evas *e)
{
   pd->evas = e;
}

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(efl_input_device_evas_set, EFL_FUNC_CALL(e), Evas *e);

/**
 * @brief Gets the Evas device subclass. (Internal, for Evas compatibility)
 *
 * @param obj The Efl_Input_Device object.
 * @param pd The private data of the Efl_Input_Device.
 * @return The Evas_Device_Subclass.
 */
static Evas_Device_Subclass
_efl_input_device_subclass_get(const Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd)
{
   return pd->subclass;
}

EVAS_API EVAS_API_WEAK EFL_FUNC_BODY_CONST(efl_input_device_subclass_get, Evas_Device_Subclass, 0);

/**
 * @brief Sets the Evas device subclass. (Internal, for Evas compatibility)
 *
 * @param obj The Efl_Input_Device object.
 * @param pd The private data of the Efl_Input_Device.
 * @param sub_clas The Evas_Device_Subclass to set.
 */
static void
_efl_input_device_subclass_set(Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd,
                               Evas_Device_Subclass sub_clas)
{
   pd->subclass = sub_clas;
}

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(efl_input_device_subclass_set, EFL_FUNC_CALL(sub_clas), Evas_Device_Subclass sub_clas);

/**
 * @brief Callback function to delete grab data from the hash.
 *
 * This function is called by eina_hash_free_buckets or eina_hash_del
 * when an entry is removed from the grabs hash. It ensures that
 * evas_object_pointer_grab_del is called to clean up the grab state
 * on the Evas_Object.
 *
 * @param data The Evas_Object_Pointer_Data associated with the grab.
 */
static void
_grab_del(void *data)
{
   Evas_Object_Pointer_Data *pdata = data;

   evas_object_pointer_grab_del(pdata->obj, pdata);
}

/**
 * @brief Registers an object to grab pointer events from this device. (Internal, for Evas compatibility)
 *
 * This function adds an Efl_Canvas_Object to the device's internal hash of grabbed objects.
 * This is typically used by Evas to manage which objects receive pointer events exclusively.
 *
 * @param obj The Efl_Input_Device object.
 * @param pd The private data of the Efl_Input_Device.
 * @param grab The Efl_Canvas_Object that is grabbing the device.
 * @param pdata Pointer data associated with the grab (Evas specific).
 */
static void
_efl_input_device_grab_register(Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd,
                                Efl_Canvas_Object *grab, Evas_Object_Pointer_Data *pdata)
{
   if (!pd->grabs) pd->grabs = eina_hash_pointer_new(_grab_del); // Create hash on demand
   eina_hash_add(pd->grabs, &grab, pdata);
}

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(efl_input_device_grab_register, EFL_FUNC_CALL(grab, pdata),
                          Efl_Canvas_Object *grab, Evas_Object_Pointer_Data *pdata);

/**
 * @brief Unregisters an object from grabbing pointer events from this device. (Internal, for Evas compatibility)
 *
 * This function removes an Efl_Canvas_Object from the device's internal hash of grabbed objects.
 *
 * @param obj The Efl_Input_Device object.
 * @param pd The private data of the Efl_Input_Device.
 * @param grab The Efl_Canvas_Object that is ungrabbing the device.
 * @param pdata Pointer data associated with the grab (Evas specific, used for matching).
 */
static void
_efl_input_device_grab_unregister(Eo *obj EINA_UNUSED, Efl_Input_Device_Data *pd,
                                  Efl_Canvas_Object *grab, Evas_Object_Pointer_Data *pdata)
{
   eina_hash_del(pd->grabs, &grab, pdata); // _grab_del will be called by the hash if item is found
}

EVAS_API EVAS_API_WEAK EFL_VOID_FUNC_BODYV(efl_input_device_grab_unregister, EFL_FUNC_CALL(grab, pdata),
                          Efl_Canvas_Object *grab, Evas_Object_Pointer_Data *pdata);

#define EFL_INPUT_DEVICE_EXTRA_OPS                                      \
  EFL_OBJECT_OP_FUNC(efl_input_device_evas_get, _efl_input_device_evas_get), \
  EFL_OBJECT_OP_FUNC(efl_input_device_evas_set, _efl_input_device_evas_set), \
  EFL_OBJECT_OP_FUNC(efl_input_device_subclass_get, _efl_input_device_subclass_get), \
  EFL_OBJECT_OP_FUNC(efl_input_device_subclass_set, _efl_input_device_subclass_set), \
  EFL_OBJECT_OP_FUNC(efl_input_device_children_get, _efl_input_device_children_get), \
  EFL_OBJECT_OP_FUNC(efl_input_device_grab_register, _efl_input_device_grab_register), \
  EFL_OBJECT_OP_FUNC(efl_input_device_grab_unregister, _efl_input_device_grab_unregister),

#include "efl_input_device.eo.c"
