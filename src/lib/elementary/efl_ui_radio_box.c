#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "elm_priv.h"

#define MY_CLASS EFL_UI_RADIO_BOX_CLASS

/**
 * @brief Private data structure for Efl_Ui_Radio_Box.
 *
 * This structure holds the internal state of an Efl_Ui_Radio_Box instance.
 * @c in_pack is a flag to prevent re-entrancy issues during packing operations.
 * @c group is the associated radio group that manages the radio buttons within this box.
 */
typedef struct {
   Eina_Bool in_pack; /**< Flag to indicate if a packing operation is in progress. */
   Efl_Ui_Radio_Group *group; /**< The radio group associated with this radio box. */
} Efl_Ui_Radio_Box_Data;

/**
 * @brief Safely begins the process of registering a sub-object with the radio group.
 *
 * This function checks if a packing operation is already in progress (pd->in_pack)
 * to prevent re-entrancy. If the sub-object is a radio button (@c is_radio is true),
 * it registers the sub-object with the radio group. It then sets the @c in_pack flag.
 *
 * @param subobj The sub-object to potentially register.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @param is_radio EINA_TRUE if @p subobj is an Efl_Ui_Radio, EINA_FALSE otherwise.
 * @return EINA_TRUE if the operation can proceed, EINA_FALSE on error (though currently always returns EINA_TRUE).
 */
static inline Eina_Bool
register_safe_in_group_begin(Eo *subobj, Efl_Ui_Radio_Box_Data *pd, Eina_Bool is_radio)
{
   if (pd->in_pack) return EINA_TRUE;
   if (is_radio)
     efl_ui_radio_group_register(pd->group, subobj);
   pd->in_pack = EINA_TRUE;

   return EINA_TRUE;
}

/**
 * @brief Safely ends the process of registering/unregistering a sub-object with the radio group.
 *
 * This function unregisters the @p subobj from the radio group if it's a radio button
 * (@c is_radio is true) and the preceding operation (@c result) failed.
 * It then clears the @c in_pack flag.
 *
 * @param subobj The sub-object that was part of the operation.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @param is_radio EINA_TRUE if @p subobj is an Efl_Ui_Radio, EINA_FALSE otherwise.
 * @param result The result of the packing operation that was performed.
 * @return The value of @p result.
 */
static inline Eina_Bool
register_safe_group_end(Eo *subobj, Efl_Ui_Radio_Box_Data *pd, Eina_Bool is_radio, Eina_Bool result)
{
   if (is_radio && (!result))
     efl_ui_radio_group_unregister(pd->group, subobj);
   pd->in_pack = EINA_FALSE;

   return result;
}

/**
 * @brief Macro to safely wrap packing operations for radio buttons.
 *
 * This macro ensures that radio buttons are correctly registered with the
 * Efl_Ui_Radio_Group before a packing operation and unregistered if the
 * operation fails. It uses register_safe_in_group_begin() and
 * register_safe_group_end() to manage the registration state.
 *
 * @param f The packing function call to be executed (e.g., efl_pack(efl_super(obj, MY_CLASS), subobj)).
 *          This function is expected to return an Eina_Bool indicating success or failure.
 */
#define REGISTER_SAFE(f) \
  Eina_Bool result, is_radio = efl_isa(subobj, EFL_UI_RADIO_CLASS); \
  if (!register_safe_in_group_begin(subobj, pd, is_radio)) \
    return EINA_FALSE; \
  result = f ; \
  return register_safe_group_end(subobj, pd, is_radio, result);

/**
 * @brief Unregisters all radio items from the group when they are removed from the box.
 *
 * This function iterates over all content items in the radio box and unregisters
 * each one from the associated radio group. This is typically called before
 * clearing or unpacking all items from the box.
 *
 * @param obj The Efl_Ui_Radio_Box object.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 */
static void
unpack_from_logical(Eo *obj, Efl_Ui_Radio_Box_Data *pd)
{
   int length = efl_content_count(obj);
   for (int i = 0; i < length; ++i)
     {
        efl_ui_radio_group_unregister(pd->group, efl_pack_content_get(obj, i));
     }
}

/**
 * @brief Implements Efl.Pack.pack_clear for Efl_Ui_Radio_Box.
 *
 * Before clearing all packed elements from the superclass, this function
 * ensures that all radio items are unregistered from the internal radio group.
 *
 * @param obj The Efl_Ui_Radio_Box object.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_radio_box_efl_pack_pack_clear(Eo *obj, Efl_Ui_Radio_Box_Data *pd)
{
   unpack_from_logical(obj, pd);
   return efl_pack_clear(efl_super(obj, MY_CLASS));
}

/**
 * @brief Implements Efl.Pack.unpack_all for Efl_Ui_Radio_Box.
 *
 * Before unpacking all elements using the superclass implementation, this function
 * ensures that all radio items are unregistered from the internal radio group.
 *
 * @param obj The Efl_Ui_Radio_Box object.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_radio_box_efl_pack_unpack_all(Eo *obj, Efl_Ui_Radio_Box_Data *pd)
{
   unpack_from_logical(obj, pd);
   return efl_pack_unpack_all(efl_super(obj, MY_CLASS));
}

/**
 * @brief Implements Efl.Pack.unpack for Efl_Ui_Radio_Box.
 *
 * Unregisters the given @p subobj from the internal radio group before
 * calling the superclass's unpack implementation.
 *
 * @param obj The Efl_Ui_Radio_Box object.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @param subobj The sub-object to unpack.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_radio_box_efl_pack_unpack(Eo *obj, Efl_Ui_Radio_Box_Data *pd, Efl_Gfx_Entity *subobj)
{
   efl_ui_radio_group_unregister(pd->group, subobj);
   return efl_pack_unpack(efl_super(obj, MY_CLASS), subobj);
}

/**
 * @brief Implements Efl.Pack_Linear.pack_unpack_at for Efl_Ui_Radio_Box.
 *
 * Unregisters the sub-object at the given @p index from the internal radio group
 * before calling the superclass's unpack_at implementation.
 *
 * @param obj The Efl_Ui_Radio_Box object.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @param index The index of the sub-object to unpack.
 * @return The unpacked sub-object, or @c NULL on failure.
 */
EOLIAN static Efl_Gfx_Entity*
_efl_ui_radio_box_efl_pack_linear_pack_unpack_at(Eo *obj, Efl_Ui_Radio_Box_Data *pd, int index)
{
   efl_ui_radio_group_unregister(pd->group, efl_pack_content_get(obj, index));
   return efl_pack_unpack_at(efl_super(obj, MY_CLASS), index);
}

/**
 * @brief Implements Efl.Pack.pack for Efl_Ui_Radio_Box.
 *
 * Packs the @p subobj into the radio box. If @p subobj is an Efl_Ui_Radio,
 * it is registered with the internal radio group. This operation is
 * wrapped by the REGISTER_SAFE macro.
 *
 * @param obj The Efl_Ui_Radio_Box object.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @param subobj The sub-object to pack.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_radio_box_efl_pack_pack(Eo *obj, Efl_Ui_Radio_Box_Data *pd, Efl_Gfx_Entity *subobj)
{
   REGISTER_SAFE(efl_pack(efl_super(obj, MY_CLASS), subobj))
}

/**
 * @brief Implements Efl.Pack_Linear.pack_begin for Efl_Ui_Radio_Box.
 *
 * Packs the @p subobj at the beginning of the radio box. If @p subobj is an Efl_Ui_Radio,
 * it is registered with the internal radio group. This operation is
 * wrapped by the REGISTER_SAFE macro.
 *
 * @param obj The Efl_Ui_Radio_Box object.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @param subobj The sub-object to pack.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_radio_box_efl_pack_linear_pack_begin(Eo *obj, Efl_Ui_Radio_Box_Data *pd, Efl_Gfx_Entity *subobj)
{
   REGISTER_SAFE(efl_pack_begin(efl_super(obj, MY_CLASS), subobj))
}

/**
 * @brief Implements Efl.Pack_Linear.pack_end for Efl_Ui_Radio_Box.
 *
 * Packs the @p subobj at the end of the radio box. If @p subobj is an Efl_Ui_Radio,
 * it is registered with the internal radio group. This operation is
 * wrapped by the REGISTER_SAFE macro.
 *
 * @param obj The Efl_Ui_Radio_Box object.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @param subobj The sub-object to pack.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_radio_box_efl_pack_linear_pack_end(Eo *obj, Efl_Ui_Radio_Box_Data *pd, Efl_Gfx_Entity *subobj)
{
   REGISTER_SAFE(efl_pack_end(efl_super(obj, MY_CLASS), subobj))
}

/**
 * @brief Implements Efl.Pack_Linear.pack_before for Efl_Ui_Radio_Box.
 *
 * Packs the @p subobj before the @p existing sub-object in the radio box.
 * If @p subobj is an Efl_Ui_Radio, it is registered with the internal radio group.
 * This operation is wrapped by the REGISTER_SAFE macro.
 *
 * @param obj The Efl_Ui_Radio_Box object.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @param subobj The sub-object to pack.
 * @param existing The existing sub-object before which @p subobj will be packed.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_radio_box_efl_pack_linear_pack_before(Eo *obj, Efl_Ui_Radio_Box_Data *pd, Efl_Gfx_Entity *subobj, const Efl_Gfx_Entity *existing)
{
   REGISTER_SAFE(efl_pack_before(efl_super(obj, MY_CLASS), subobj, existing));
}

/**
 * @brief Implements Efl.Pack_Linear.pack_after for Efl_Ui_Radio_Box.
 *
 * Packs the @p subobj after the @p existing sub-object in the radio box.
 * If @p subobj is an Efl_Ui_Radio, it is registered with the internal radio group.
 * This operation is wrapped by the REGISTER_SAFE macro.
 *
 * @param obj The Efl_Ui_Radio_Box object.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @param subobj The sub-object to pack.
 * @param existing The existing sub-object after which @p subobj will be packed.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_radio_box_efl_pack_linear_pack_after(Eo *obj, Efl_Ui_Radio_Box_Data *pd, Efl_Gfx_Entity *subobj, const Efl_Gfx_Entity *existing)
{
   REGISTER_SAFE(efl_pack_after(efl_super(obj, MY_CLASS), subobj, existing));
}

/**
 * @brief Implements Efl.Pack_Linear.pack_at for Efl_Ui_Radio_Box.
 *
 * Packs the @p subobj at the specified @p index in the radio box.
 * If @p subobj is an Efl_Ui_Radio, it is registered with the internal radio group.
 * This operation is wrapped by the REGISTER_SAFE macro.
 *
 * @param obj The Efl_Ui_Radio_Box object.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @param subobj The sub-object to pack.
 * @param index The index at which to pack @p subobj.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EOLIAN static Eina_Bool
_efl_ui_radio_box_efl_pack_linear_pack_at(Eo *obj, Efl_Ui_Radio_Box_Data *pd, Efl_Gfx_Entity *subobj, int index)
{
   REGISTER_SAFE(efl_pack_at(efl_super(obj, MY_CLASS), subobj, index));
}

/**
 * @brief Constructor for Efl_Ui_Radio_Box.
 *
 * Initializes the radio box by creating an internal Efl_Ui_Radio_Group,
 * attaching it as a composite object, and forwarding relevant events
 * (selection changed, value changed) from the group to the radio box itself.
 *
 * @param obj The Efl_Ui_Radio_Box object being constructed.
 * @param pd The private data of the Efl_Ui_Radio_Box.
 * @return The constructed Efl_Object.
 */
EOLIAN static Efl_Object*
_efl_ui_radio_box_efl_object_constructor(Eo *obj, Efl_Ui_Radio_Box_Data *pd)
{
   pd->group = efl_new(EFL_UI_RADIO_GROUP_IMPL_CLASS, NULL);
   efl_composite_attach(obj, pd->group);
   efl_event_callback_forwarder_add(pd->group, EFL_UI_SELECTABLE_EVENT_SELECTION_CHANGED, obj);
   efl_event_callback_forwarder_add(pd->group, EFL_UI_RADIO_GROUP_EVENT_VALUE_CHANGED, obj);
   return efl_constructor(efl_super(obj, MY_CLASS));
}


#include "efl_ui_radio_box.eo.c"
