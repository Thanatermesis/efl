#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Efl.h"
#include "Efl_MVVM_Common.h"

/** @brief Generic unknown error for Efl_Model. */
EAPI Eina_Error EFL_MODEL_ERROR_UNKNOWN = 0;
/** @brief Operation not supported error for Efl_Model. */
EAPI Eina_Error EFL_MODEL_ERROR_NOT_SUPPORTED = 0;
/** @brief Value not found error for Efl_Model. */
EAPI Eina_Error EFL_MODEL_ERROR_NOT_FOUND = 0;
/** @brief Value is read-only error for Efl_Model. */
EAPI Eina_Error EFL_MODEL_ERROR_READ_ONLY = 0;
/** @brief Initialization failed error for Efl_Model. */
EAPI Eina_Error EFL_MODEL_ERROR_INIT_FAILED = 0;
/** @brief Permission denied error for Efl_Model. */
EAPI Eina_Error EFL_MODEL_ERROR_PERMISSION_DENIED = 0;
/** @brief Incorrect value error for Efl_Model. */
EAPI Eina_Error EFL_MODEL_ERROR_INCORRECT_VALUE = 0;
/** @brief Invalid object error for Efl_Model. */
EAPI Eina_Error EFL_MODEL_ERROR_INVALID_OBJECT = 0;

/** @brief Operation not supported error for Efl_Factory. */
EAPI Eina_Error EFL_FACTORY_ERROR_NOT_SUPPORTED = 0;
/** @brief Invalid key error for Efl_Property. */
EAPI Eina_Error EFL_PROPERTY_ERROR_INVALID_KEY = 0;

static const char EFL_MODEL_ERROR_UNKNOWN_STR[]           = "Unknown Error";
static const char EFL_MODEL_ERROR_NOT_SUPPORTED_STR[]     = "Operation not supported";
static const char EFL_MODEL_ERROR_NOT_FOUND_STR[]         = "Value not found";
static const char EFL_MODEL_ERROR_READ_ONLY_STR[]         = "Value read only";
static const char EFL_MODEL_ERROR_INIT_FAILED_STR[]       = "Init failed";
static const char EFL_MODEL_ERROR_PERMISSION_DENIED_STR[] = "Permission denied";
static const char EFL_MODEL_ERROR_INCORRECT_VALUE_STR[]   = "Incorrect value";
static const char EFL_MODEL_ERROR_INVALID_OBJECT_STR[]    = "Object is invalid";

static const char EFL_FACTORY_ERROR_NOT_SUPPORTED_STR[]   = "Operation not supported";

static const char EFL_PROPERTY_ERROR_INVALID_KEY_STR[]    = "Incorrect key provided";

/**
 * @brief Initializes the Efl_Model error messages.
 *
 * This function registers static error messages for Efl_Model, Efl_Factory,
 * and Efl_Property related errors. It should be called once during
 * application startup.
 *
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
EAPI int
efl_model_init(void)
{
#define _ERROR(Name) EFL_MODEL_ERROR_##Name = eina_error_msg_static_register(EFL_MODEL_ERROR_##Name##_STR);
   _ERROR(INCORRECT_VALUE);
   _ERROR(UNKNOWN);
   _ERROR(NOT_SUPPORTED);
   _ERROR(NOT_FOUND);
   _ERROR(READ_ONLY);
   _ERROR(INIT_FAILED);
   _ERROR(PERMISSION_DENIED);
   _ERROR(INVALID_OBJECT);

#undef _ERROR
#define _ERROR(Name) EFL_FACTORY_ERROR_##Name = eina_error_msg_static_register(EFL_FACTORY_ERROR_##Name##_STR);
   _ERROR(NOT_SUPPORTED);

#undef _ERROR
#define _ERROR(Name) EFL_PROPERTY_ERROR_##Name = eina_error_msg_static_register(EFL_PROPERTY_ERROR_##Name##_STR);
   _ERROR(INVALID_KEY);

   return EINA_TRUE;
}

#undef _ERROR

/**
 * @internal
 * @brief Notifies listeners that properties on a model have changed.
 *
 * This is an internal helper function to construct and dispatch the
 * EFL_MODEL_EVENT_PROPERTIES_CHANGED event.
 *
 * @param model The model on which properties have changed.
 * @param ... A NULL-terminated list of stringshared property names that have changed.
 *            Example: "property1", "property2", NULL
 */
EAPI void
_efl_model_properties_changed_internal(const Efl_Model *model, ...)
{
   Efl_Model_Property_Event ev = { 0 };
   Eina_Array *properties = eina_array_new(1);
   Eina_Stringshare *sp;
   const char *property;
   va_list args;

   va_start(args, model);

   while ((property = (const char*) va_arg(args, const char*)))
     {
        eina_array_push(properties, eina_stringshare_add(property));
     }

   va_end(args);

   ev.changed_properties = properties;

   efl_event_callback_call((Efl_Model *) model, EFL_MODEL_EVENT_PROPERTIES_CHANGED, &ev);

   while ((sp = eina_array_pop(properties)))
     eina_stringshare_del(sp);
   eina_array_free(properties);
}

/**
 * @brief Notifies that a specific property of a model has been invalidated.
 *
 * This function is used to signal that a single property's value is no longer
 * valid and should be re-fetched or considered stale. It triggers the
 * EFL_MODEL_EVENT_PROPERTIES_CHANGED event with the `invalidated_properties`
 * field populated.
 *
 * @param model The model whose property has been invalidated.
 * @param property The name of the property that has been invalidated.
 *                 Example: "propertyName"
 */
EAPI void
efl_model_property_invalidated_notify(Efl_Model *model, const char *property)
{
   Eina_Array *invalidated_properties = eina_array_new(1);
   EINA_SAFETY_ON_NULL_RETURN(invalidated_properties);

   Eina_Stringshare *sp = eina_stringshare_add(property);

   Eina_Bool ret = eina_array_push(invalidated_properties, sp);
   EINA_SAFETY_ON_FALSE_GOTO(ret, on_error);

   Efl_Model_Property_Event evt = {.invalidated_properties = invalidated_properties};
   efl_event_callback_call(model, EFL_MODEL_EVENT_PROPERTIES_CHANGED, &evt);

on_error:
   eina_stringshare_del(sp);
   eina_array_free(invalidated_properties);
}

typedef struct _Efl_Model_Value_Struct_Desc Efl_Model_Value_Struct_Desc;

struct _Efl_Model_Value_Struct_Desc
{
   Eina_Value_Struct_Desc base; /**< Base Eina_Value_Struct_Desc structure. */
   void *data; /**< User data to be passed to the setup_cb. */
   Eina_Value_Struct_Member members[]; /**< Array of structure members. */
};

/**
 * @brief Creates a new Eina_Value_Struct_Desc for use with Efl_Model.
 *
 * This function dynamically allocates and initializes a structure description
 * that can be used to represent complex data types within the Efl_Model framework.
 * The `setup_cb` is called for each member to configure its name and type.
 *
 * The `data` pointer provided to this function will be passed as the first
 * argument to the `setup_cb` for each member.
 *
 * Example of `setup_cb`:
 * @code
 * static void
 * _my_struct_member_setup(void *userdata, unsigned int member_index, Eina_Value_Struct_Member *member_info)
 * {
 *    My_Struct_Definition *def = userdata; // User data passed to efl_model_value_struct_description_new
 *    switch (member_index)
 *    {
 *      case 0:
 *        member_info->name = eina_stringshare_add("name");
 *        member_info->type = EINA_VALUE_TYPE_STRINGSHARE;
 *        break;
 *      case 1:
 *        member_info->name = eina_stringshare_add("age");
 *        member_info->type = EINA_VALUE_TYPE_INT;
 *        break;
 *    }
 * }
 * @endcode
 *
 * @param member_count The number of members in the structure. Must be greater than 0.
 * @param setup_cb A callback function to set up each member of the structure.
 * @param data User-provided data that will be passed to the `setup_cb`.
 * @return A newly allocated Eina_Value_Struct_Desc on success, or NULL on failure.
 *         The caller is responsible for freeing this with efl_model_value_struct_description_free().
 */
EAPI Eina_Value_Struct_Desc *
efl_model_value_struct_description_new(unsigned int member_count, Efl_Model_Value_Struct_Member_Setup_Cb setup_cb, void *data)
{
   Efl_Model_Value_Struct_Desc *desc;
   unsigned int offset = 0;
   size_t i;

   EINA_SAFETY_ON_FALSE_RETURN_VAL(member_count > 0, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(setup_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(data, NULL);

   desc = malloc(sizeof(Efl_Model_Value_Struct_Desc) + member_count * sizeof(Eina_Value_Struct_Member));
   EINA_SAFETY_ON_NULL_RETURN_VAL(desc, NULL);

   desc->base.version = EINA_VALUE_STRUCT_DESC_VERSION;
   desc->base.ops = EINA_VALUE_STRUCT_OPERATIONS_STRINGSHARE;
   desc->base.members = desc->members;
   desc->base.member_count = member_count;
   desc->base.size = 0;
   desc->data = data;

   for (i = 0; i < member_count; ++i)
     {
        Eina_Value_Struct_Member *m = (Eina_Value_Struct_Member *)desc->members + i;
        unsigned int size;

        m->offset = offset;
        setup_cb(data, i, m);

        size = m->type->value_size;
        if (size % sizeof(void *) != 0)
          size += size - (size % sizeof(void *));

        offset += size;
     }

   desc->base.size = offset;
   return &desc->base;
}

/**
 * @brief Frees an Eina_Value_Struct_Desc created by efl_model_value_struct_description_new().
 *
 * This function releases the memory allocated for the structure description,
 * including the stringshared names of its members.
 *
 * @param desc The structure description to free. If NULL, the function does nothing.
 */
EAPI void
efl_model_value_struct_description_free(Eina_Value_Struct_Desc *desc)
{
   size_t i;

   if (!desc) return;

   for (i = 0; i < desc->member_count; i++)
     eina_stringshare_del(desc->members[i].name);
   free(desc);
}
