#ifndef _EFL_CANVAS_EVENT_GRABBER_EO_LEGACY_H_
#define _EFL_CANVAS_EVENT_GRABBER_EO_LEGACY_H_

#ifndef _EFL_CANVAS_EVENT_GRABBER_EO_CLASS_TYPE
#define _EFL_CANVAS_EVENT_GRABBER_EO_CLASS_TYPE

/**
 * @brief Type definition for legacy Evas access to an Efl_Canvas_Event_Grabber.
 *
 * This typedef ensures that Efl_Canvas_Event_Grabber can be used as an Eo
 * object in legacy C Evas API calls.
 *
 * @ingroup Evas_Object_Event_Grabber_Group
 */
typedef Eo Efl_Canvas_Event_Grabber;

#endif

#ifndef _EFL_CANVAS_EVENT_GRABBER_EO_TYPES
#define _EFL_CANVAS_EVENT_GRABBER_EO_TYPES

/* Placeholder for legacy Evas-specific type definitions related to Event_Grabber. */
/* In the EFL/Eo system, types are typically defined via .eo files or are standard C types. */

#endif

/**
 * @brief Stops the grabber from updating its internal stacking order while
 * visible
 *
 * @param[in] obj The object.
 * @param[in] set If @c true, stop updating
 *
 * @since 1.20
 *
 * @ingroup Evas_Object_Event_Grabber_Group
 */
EVAS_API void evas_object_event_grabber_freeze_when_visible_set(Efl_Canvas_Event_Grabber *obj, Eina_Bool set);

/**
 * @brief Stops the grabber from updating its internal stacking order while
 * visible
 *
 * @param[in] obj The object.
 *
 * @return If @c true, stop updating
 *
 * @since 1.20
 *
 * @ingroup Evas_Object_Event_Grabber_Group
 */
EVAS_API Eina_Bool evas_object_event_grabber_freeze_when_visible_get(const Efl_Canvas_Event_Grabber *obj);

#endif
