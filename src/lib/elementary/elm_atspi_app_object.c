#ifdef HAVE_CONFIG_H
  #include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED

#include "elm_priv.h"

extern Eina_List *_elm_win_list;

typedef struct _Elm_Atspi_App_Object_Data Elm_Atspi_App_Object_Data;

/**
 * @brief Private data for the Elm_Atspi_App_Object.
 *
 * This structure holds the private data associated with an Elm_Atspi_App_Object instance.
 */
struct _Elm_Atspi_App_Object_Data
{
   const char *descr; /**< The accessible description of the application object. */
};

/**
 * @brief Destructor for the Elm_Atspi_App_Object.
 *
 * This function is called when the Elm_Atspi_App_Object is being destroyed.
 * It cleans up any allocated resources, such as the description string.
 *
 * @param obj The Efl_Object instance.
 * @param _pd The private data of the Elm_Atspi_App_Object.
 */
EOLIAN static void
_elm_atspi_app_object_efl_object_destructor(Eo *obj EINA_UNUSED, Elm_Atspi_App_Object_Data *_pd)
{
   if (_pd->descr) eina_stringshare_del(_pd->descr);

   efl_destructor(efl_super(obj, ELM_ATSPI_APP_OBJECT_CLASS));
}

/**
 * @brief Gets the accessible children of the application object.
 *
 * This function retrieves a list of all top-level window objects
 * that are part of the application and are accessible.
 *
 * @param obj The Efl_Access_Object instance (unused).
 * @param _pd The private data of the Elm_Atspi_App_Object (unused).
 * @return A list (Eina_List *) of accessible child objects (Evas_Object *).
 *         The caller is responsible for freeing the list if it's not empty,
 *         but not the items it contains.
 *         Returns @c NULL if there are no accessible children.
 *
 * @note This function iterates through the global list of Elementary windows
 *       (_elm_win_list) and filters for those that implement the
 *       EFL_ACCESS_OBJECT_MIXIN and have an EFL_ACCESS_TYPE_REGULAR type.
 */
EOLIAN static Eina_List*
_elm_atspi_app_object_efl_access_object_access_children_get(const Eo *obj EINA_UNUSED, Elm_Atspi_App_Object_Data *_pd EINA_UNUSED)
{
   Eina_List *l, *accs = NULL;
   Evas_Object *win;

   EINA_LIST_FOREACH(_elm_win_list, l, win)
     {
        Efl_Access_Type type;
        if (!efl_isa(win, EFL_ACCESS_OBJECT_MIXIN))
          continue;
        type = efl_access_object_access_type_get(win);
        if (type == EFL_ACCESS_TYPE_REGULAR)
          accs = eina_list_append(accs, win);
     }

   return accs;
}

/**
 * @brief Gets the internationalized accessible name of the application object.
 *
 * This function retrieves the application's name, which is typically
 * set via elm_app_name_set().
 *
 * @param obj The Efl_Access_Object instance (unused).
 * @param _pd The private data of the Elm_Atspi_App_Object (unused).
 * @return The internationalized name of the application as a stringshared const char *.
 *         Returns @c NULL if the name has not been set.
 */
EOLIAN static const char*
_elm_atspi_app_object_efl_access_object_i18n_name_get(const Eo *obj EINA_UNUSED, Elm_Atspi_App_Object_Data *_pd EINA_UNUSED)
{
   const char *ret;
   ret = elm_app_name_get();
   return ret;
}

/**
 * @brief Gets the accessible description of the application object.
 *
 * @param obj The Efl_Access_Object instance (unused).
 * @param _pd The private data of the Elm_Atspi_App_Object.
 * @return The description of the application object as a stringshared const char *.
 *         Returns @c NULL if no description is set.
 */
EOLIAN static const char*
_elm_atspi_app_object_efl_access_object_description_get(const Eo *obj EINA_UNUSED, Elm_Atspi_App_Object_Data *_pd)
{
   return _pd->descr;
}

/**
 * @brief Sets the accessible description of the application object.
 *
 * @param obj The Efl_Access_Object instance (unused).
 * @param _pd The private data of the Elm_Atspi_App_Object.
 * @param descr The description string to set. The string is stringshared.
 */
EOLIAN static void
_elm_atspi_app_object_efl_access_object_description_set(Eo *obj EINA_UNUSED, Elm_Atspi_App_Object_Data *_pd, const char *descr)
{
   eina_stringshare_replace(&_pd->descr, descr);
}

/**
 * @brief Gets the accessible role of the application object.
 *
 * @param obj The Efl_Access_Object instance (unused).
 * @param _pd The private data of the Elm_Atspi_App_Object (unused).
 * @return Always returns #EFL_ACCESS_ROLE_APPLICATION.
 */
EOLIAN static Efl_Access_Role
_elm_atspi_app_object_efl_access_object_role_get(const Eo *obj EINA_UNUSED, Elm_Atspi_App_Object_Data *_pd EINA_UNUSED)
{
   return EFL_ACCESS_ROLE_APPLICATION;
}

#include "elm_atspi_app_object_eo.c"
