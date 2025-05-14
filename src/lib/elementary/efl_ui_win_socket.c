#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_UI_WIN_PROTECTED
#define EFL_UI_WIN_SOCKET_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "efl_ui_win_socket_legacy_eo.h"

#define MY_CLASS EFL_UI_WIN_SOCKET_CLASS
#define MY_CLASS_NAME "Efl.Ui.Win_Socket"
#define MY_CLASS_NAME_LEGACY "elm_win"

/**
 * @brief Private data for the Efl.Ui.Win_Socket class.
 * @since 1.25
 */
typedef struct
{
} Efl_Ui_Win_Socket_Data;

/**
 * @brief Finalizes the Efl.Ui.Win_Socket object.
 *
 * This function is called during the finalization phase of the object's lifecycle.
 * It sets the window type to EFL_UI_WIN_TYPE_SOCKET_IMAGE.
 *
 * @param obj The Efl.Ui.Win_Socket object.
 * @param pd The private data for the Efl.Ui.Win_Socket object.
 * @return The finalized Efl_Object.
 */
EOLIAN static Efl_Object *
_efl_ui_win_socket_efl_object_finalize(Eo *obj, Efl_Ui_Win_Socket_Data *pd EINA_UNUSED)
{
   efl_ui_win_type_set(obj, EFL_UI_WIN_TYPE_SOCKET_IMAGE);
   obj = efl_finalize(efl_super(obj, MY_CLASS));

   return obj;
}

/**
 * @brief Makes the window listen on a socket for an incoming client.
 *
 * This function sets up the window to act as a server, listening for
 * a connection from a client application. The client can then draw
 * into this window.
 *
 * @param obj The Efl.Ui.Win_Socket object.
 * @param pd The private data for the Efl.Ui.Win_Socket object.
 * @param svcname The service name to announce. For example, "myservice".
 *                If @c EINA_TRUE is passed for @p svcsys, this name is
 *                ignored and a system-wide service name is used.
 * @param svcnum A number to add to the service name to ensure uniqueness.
 *               For example, if @p svcname is "myservice" and @p svcnum is 2,
 *               the effective service name might be "myservice_2".
 *               Ignored if @p svcsys is @c EINA_TRUE.
 * @param svcsys If @c EINA_TRUE, listen on a system-wide service (e.g., for X11).
 *               If @c EINA_FALSE, use the provided @p svcname and @p svcnum.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EOLIAN static Eina_Bool
_efl_ui_win_socket_socket_listen(Eo *obj, Efl_Ui_Win_Socket_Data *pd EINA_UNUSED, const char *svcname, int svcnum, Eina_Bool svcsys)
{
   Ecore_Evas *ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));

   if (!ee) return EINA_FALSE;
   return ecore_evas_extn_socket_listen(ee, svcname, svcnum, svcsys);
}

#include "efl_ui_win_socket.eo.c"

/**
 * @brief Legacy class constructor for Efl.Ui.Win_Socket.
 *
 * This function is called when the Efl.Ui.Win_Socket_Legacy class is constructed.
 * It registers the legacy type name "elm_win" for this class, allowing
 * it to be used with older Elm_Win APIs.
 *
 * @param klass The Efl_Class being constructed.
 */
static void
_efl_ui_win_socket_legacy_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

/**
 * @brief Finalizes the legacy Efl.Ui.Win_Socket object.
 *
 * This function is called during the finalization phase of the legacy object's lifecycle.
 * It sets the canvas object type to the legacy name "elm_win".
 *
 * @param obj The legacy Efl.Ui.Win_Socket object.
 * @param pd The private data for the legacy Efl.Ui.Win_Socket object (unused).
 * @return The finalized Efl_Object.
 */
EOLIAN static Eo *
_efl_ui_win_socket_legacy_efl_object_finalize(Eo *obj, void *pd EINA_UNUSED)
{
   obj = efl_finalize(efl_super(obj, EFL_UI_WIN_SOCKET_LEGACY_CLASS));
   efl_canvas_object_type_set(obj, MY_CLASS_NAME_LEGACY);
   return obj;
}

#include "efl_ui_win_socket_legacy_eo.c"
