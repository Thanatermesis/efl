#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Gets the type of the event.
 *
 * @param[in] event The event object.
 * @return The type of the event, or @c NULL if the event type is void or on error.
 */
EOLIAN_API const Eolian_Type *
eolian_event_type_get(const Eolian_Event *event)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(event, NULL);
   if (event->type && (event->type->type == EOLIAN_TYPE_VOID))
     return NULL;
   return event->type;
}

/**
 * @brief Gets the class that owns the event.
 *
 * @param[in] event The event object.
 * @return The class that owns the event, or @c NULL on error.
 */
EOLIAN_API const Eolian_Class *
eolian_event_class_get(const Eolian_Event *event)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(event, NULL);
   return event->klass;
}

/**
 * @brief Gets the documentation for the event.
 *
 * @param[in] event The event object.
 * @return The documentation for the event, or @c NULL on error.
 */
EOLIAN_API const Eolian_Documentation *
eolian_event_documentation_get(const Eolian_Event *event)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(event, NULL);
   return event->doc;
}

/**
 * @brief Gets the scope of the event.
 *
 * @param[in] event The event object.
 * @return The scope of the event (e.g., EOLIAN_SCOPE_PUBLIC, EOLIAN_SCOPE_PRIVATE),
 *         or EOLIAN_SCOPE_UNKNOWN on error.
 */
EOLIAN_API Eolian_Object_Scope
eolian_event_scope_get(const Eolian_Event *event)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(event, EOLIAN_SCOPE_UNKNOWN);
   return event->scope;
}

/**
 * @brief Checks if the event is marked as "hot".
 *
 * A "hot" event is one that is expected to be emitted frequently.
 *
 * @param[in] event The event object.
 * @return @c EINA_TRUE if the event is hot, @c EINA_FALSE otherwise or on error.
 */
EOLIAN_API Eina_Bool
eolian_event_is_hot(const Eolian_Event *event)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(event, EINA_FALSE);
   return event->is_hot;
}

/**
 * @brief Checks if the event is marked as "restart".
 *
 * A "restart" event indicates that the object's state might have significantly
 * changed, potentially requiring UI elements or other dependent components to refresh.
 *
 * @param[in] event The event object.
 * @return @c EINA_TRUE if the event is a restart event, @c EINA_FALSE otherwise or on error.
 */
EOLIAN_API Eina_Bool
eolian_event_is_restart(const Eolian_Event *event)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(event, EINA_FALSE);
   return event->is_restart;
}

/**
 * @brief Gets the C macro name for the event.
 *
 * This function constructs a standardized C macro name for an event.
 * The format is typically `CLASS_PREFIX_EVENT_EVENT_NAME`.
 * For example, if the class prefix is `MY_WIDGET` and the event name is `clicked`,
 * the macro would be `MY_WIDGET_EVENT_CLICKED`.
 * If the event's class `ev_prefix` is not set, it falls back to `c_prefix`,
 * and then to the class's base name.
 * Characters like '.' and ',' in the generated string are replaced with '_'.
 *
 * @param[in] event The event object.
 * @return A stringshared C macro name for the event. The caller does not own the string.
 *         Returns @c NULL if the input event is @c NULL (though EINA_SAFETY handles this).
 */
EOLIAN_API Eina_Stringshare *
eolian_event_c_macro_get(const Eolian_Event *event)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(event, NULL);
    char  buf[512];
    char *tmp = buf;
    const char *pfx = event->klass->ev_prefix;
    if (!pfx) pfx = event->klass->c_prefix;
    if (!pfx) pfx = event->klass->base.name;
    snprintf(buf, sizeof(buf), "%s_EVENT_%s", pfx, event->base.name);
    eina_str_toupper(&tmp);
    while ((tmp = strpbrk(tmp, ".,"))) *tmp = '_';
    return eina_stringshare_add(buf);
}

/**
 * @brief Retrieves an event from a class by its name.
 *
 * @param[in] klass The class object to search within.
 * @param[in] event_name The name of the event to find.
 * @return The event object if found, otherwise @c NULL.
 */
EOLIAN_API const Eolian_Event *
eolian_class_event_by_name_get(const Eolian_Class *klass, const char *event_name)
{
   Eina_List *itr;
   Eolian_Event *event = NULL;
   if (!klass) return NULL;
   Eina_Stringshare *shr_ev = eina_stringshare_add(event_name);

   EINA_LIST_FOREACH(klass->events, itr, event)
        {
           if (event->base.name == shr_ev)
              goto end;
        }

   event = NULL;
end:
   eina_stringshare_del(shr_ev);
   return event;
}
