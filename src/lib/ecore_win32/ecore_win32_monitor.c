#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <dbt.h>

#include <Eina.h>

#include "Ecore_Win32.h"
#include "ecore_win32_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @brief Private data structure for Ecore_Win32_Monitor.
 *
 * This structure holds the public monitor information along with
 * internal details like the monitor's device name and a flag
 * for pending deletion.
 */
typedef struct
{
   Ecore_Win32_Monitor monitor; /**< Public monitor information. */
   char *name;                  /**< The device name of the monitor (e.g., "\\\\.\\DISPLAY1"). */
   Eina_Bool delete_me : 1;     /**< Flag indicating if the monitor is marked for deletion. */
} Ecore_Win32_Monitor_Priv;

/**
 * @brief Function pointer type for GetDpiForMonitor.
 *
 * This function retrieves the dots per inch (DPI) for a display.
 * @param hmonitor Handle to the monitor.
 * @param dpiType The type of DPI being requested. 0 for effective DPI.
 * @param dpiX Pointer to a UINT to receive the DPI value along the x-axis.
 * @param dpiY Pointer to a UINT to receive the DPI value along the y-axis.
 * @return S_OK if successful, or an error code otherwise.
 */
typedef HRESULT (WINAPI *GetDpiForMonitor_t)(HMONITOR, int, UINT *, UINT *);

static HMODULE _ecore_win32_mod = NULL; /**< Handle to the shcore.dll module, used for GetDpiForMonitor. */
static GetDpiForMonitor_t GetDpiForMonitor_ = NULL; /**< Pointer to the GetDpiForMonitor function. */
static Eina_List *ecore_win32_monitors = NULL; /**< List of Ecore_Win32_Monitor_Priv structures, representing all detected monitors. */

#ifndef GUID_DEVINTERFACE_MONITOR
static GUID GUID_DEVINTERFACE_MONITOR = {0xe6f07b5f, 0xee97, 0x4a90, { 0xb0, 0x76, 0x33, 0xf5, 0x7b, 0xf4, 0xea, 0xa7} };
#endif

/**
 * @brief Frees the memory allocated for an Ecore_Win32_Monitor_Priv structure.
 * @param p Pointer to the Ecore_Win32_Monitor_Priv structure to free.
 */
static void
_ecore_win32_monitor_free(void *p)
{
   Ecore_Win32_Monitor_Priv *ewm = p;

   if (ewm)
     {
        free(ewm->name);
        free(ewm);
     }
}

/**
 * @brief Callback function for EnumDisplayMonitors.
 *
 * This function is called by EnumDisplayMonitors for each monitor found.
 * It updates the internal list of monitors (ecore_win32_monitors).
 * The behavior depends on the value of the `data` parameter:
 * - If `data` is 0 (or any value other than 1 or 2): Assumes a new monitor is being added or an initial scan.
 * - If `data` is 1: Checks if a monitor with the given name already exists. If not, it's considered new. (This case seems to have a logic error, as `is_added` is set to `EINA_TRUE` if a *different* monitor is found, not if the current one is new).
 * - If `data` is 2: Marks an existing monitor as not to be deleted. If the monitor is not found, it does nothing.
 *
 * @param m Handle to the display monitor.
 * @param monitor EINA_UNUSED Handle to a device context.
 * @param r EINA_UNUSED Pointer to a RECT structure.
 * @param data Application-defined value passed from EnumDisplayMonitors.
 *             Interpreted as:
 *             - 0: Initial population or generic update.
 *             - 1: Check for new monitor (potentially flawed logic).
 *             - 2: Mark existing monitor to not be deleted.
 * @return TRUE to continue enumeration, FALSE to stop.
 */
static BOOL CALLBACK
_ecore_win32_monitor_update_cb(HMONITOR m, HDC monitor EINA_UNUSED, LPRECT r EINA_UNUSED, LPARAM data)
{
   MONITORINFOEX mi;
   Ecore_Win32_Monitor_Priv *ewm;
   Eina_Bool is_added;

   mi.cbSize = sizeof(MONITORINFOEX);
   GetMonitorInfo(m, (MONITORINFO *)&mi);

   if (data == 1)
     {
        Eina_List *l;

        is_added = EINA_FALSE;
        EINA_LIST_FOREACH(ecore_win32_monitors, l, ewm)
          {
             if (strcmp(mi.szDevice, ewm->name) != 0)
               {
                  is_added = EINA_TRUE;
                  break;
               }
          }
     }
   else if (data == 2)
     {
        Eina_List *l;

        EINA_LIST_FOREACH(ecore_win32_monitors, l, ewm)
          {
             if (strcmp(mi.szDevice, ewm->name) == 0)
               {
                  ewm->delete_me = EINA_FALSE;
                  return FALSE;
               }
          }
     }
   else
     is_added = EINA_TRUE;

   if (!is_added)
     return TRUE;

   ewm = (Ecore_Win32_Monitor_Priv *)malloc(sizeof(Ecore_Win32_Monitor_Priv));
   if (ewm)
     {
        ewm->monitor.desktop.x = mi.rcMonitor.left;
        ewm->monitor.desktop.y = mi.rcMonitor.top;
        ewm->monitor.desktop.w = mi.rcMonitor.right - mi.rcMonitor.left;
        ewm->monitor.desktop.h = mi.rcMonitor.bottom - mi.rcMonitor.top;
        if (!GetDpiForMonitor_ ||
            (GetDpiForMonitor_(m, 0,
                               &ewm->monitor.dpi.x,
                               &ewm->monitor.dpi.y) != S_OK))
          {
             HDC dc;

             dc = GetDC(NULL);
             ewm->monitor.dpi.x = GetDeviceCaps(dc, LOGPIXELSX);
             ewm->monitor.dpi.y = GetDeviceCaps(dc, LOGPIXELSY);
             ReleaseDC(NULL, dc);
          }
        ewm->name = strdup(mi.szDevice);
        if (ewm->name)
          ecore_win32_monitors = eina_list_append(ecore_win32_monitors, ewm);
        else
          free(ewm);
     }

   return TRUE;
}

/**
 * @endcond
 */


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

/** @brief Handle to a hidden window used for receiving monitor device change notifications. */
HWND ecore_win32_monitor_window = NULL;

/**
 * @brief Initializes the monitor detection system.
 *
 * This function creates a hidden window to receive device change notifications
 * for monitors. It registers for these notifications and performs an initial
 * scan for connected monitors. It also attempts to load `shcore.dll` to get
 * the `GetDpiForMonitor` function for more accurate DPI information.
 */
void
ecore_win32_monitor_init(void)
{
   DEV_BROADCAST_DEVICEINTERFACE notification;
   DWORD style;

   style = WS_POPUP & ~(WS_CAPTION | WS_THICKFRAME);
   ecore_win32_monitor_window = CreateWindow(ECORE_WIN32_WINDOW_CLASS, "",
                                             style,
                                             10, 10,
                                             100, 100,
                                             NULL, NULL,
                                             _ecore_win32_instance, NULL);

   if (ecore_win32_monitor_window)
     {
        ZeroMemory(&notification, sizeof(notification));
        notification.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
        notification.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
        notification.dbcc_classguid = GUID_DEVINTERFACE_MONITOR;
        RegisterDeviceNotification(ecore_win32_monitor_window,
                                   &notification,
                                   DEVICE_NOTIFY_WINDOW_HANDLE);
     }

   /*
    * Even if RegisterDeviceNotification() fails, the next call will
    * fill one item of the monitor lists, except if there is no more
    * memory
    */
   ecore_win32_monitor_update(0);

   _ecore_win32_mod = LoadLibrary("shcore.dll");
   if (_ecore_win32_mod)
     GetDpiForMonitor_ = (GetDpiForMonitor_t)GetProcAddress(_ecore_win32_mod,
                                                            "GetDpiForMonitor");
}

/**
 * @brief Shuts down the monitor detection system.
 *
 * This function frees all resources associated with monitor detection,
 * including the list of monitors, the loaded `shcore.dll` module,
 * and the hidden notification window.
 */
void
ecore_win32_monitor_shutdown(void)
{
   Ecore_Win32_Monitor_Priv *ewm;

   if (_ecore_win32_mod)
     FreeLibrary(_ecore_win32_mod);
   EINA_LIST_FREE(ecore_win32_monitors, ewm)
     _ecore_win32_monitor_free(ewm);
   if (ecore_win32_monitor_window)
     DestroyWindow(ecore_win32_monitor_window);
}

/**
 * @brief Updates the list of available monitors.
 *
 * This function calls EnumDisplayMonitors with the
 * _ecore_win32_monitor_update_cb callback to refresh the monitor list.
 *
 * @param d An integer passed to the callback _ecore_win32_monitor_update_cb.
 *          - If `d` is 2, all monitors are initially marked for deletion.
 *            The callback will then unmark existing monitors. After enumeration,
 *            any monitors still marked for deletion are removed from the list.
 *            Note: In this case (d == 2), the current implementation only removes
 *            the *first* monitor found to be marked for deletion due to the `break`
 *            statement in the loop. To remove all disconnected monitors, the loop
 *            would need to be adjusted.
 *          - For other values of `d` (e.g., 0 for initial scan), the behavior
 *            is dictated by the callback logic for adding new monitors.
 */
void
ecore_win32_monitor_update(int d)
{
   Ecore_Win32_Monitor_Priv *ewm;
   Eina_List *l;

   // If d is 2, it signifies a removal check. Mark all current monitors
   // as potentially being deleted. The callback will unmark those that still exist.
   if (d == 2)
     {
        EINA_LIST_FOREACH(ecore_win32_monitors, l, ewm)
          ewm->delete_me = EINA_TRUE;
     }

   EnumDisplayMonitors(NULL, NULL, _ecore_win32_monitor_update_cb, d);

   // If d was 2, iterate through the list and remove any monitors
   // that are still marked for deletion (i.e., were not found by EnumDisplayMonitors).
   // Note: This loop will only remove the first such monitor found due to 'break'.
   if (d == 2)
     {
        EINA_LIST_FOREACH(ecore_win32_monitors, l, ewm)
          {
             if (ewm->delete_me == EINA_TRUE)
               {
                  ecore_win32_monitors = eina_list_remove(ecore_win32_monitors, ewm);
                  _ecore_win32_monitor_free(ewm);
                  break; // Only removes the first deleted monitor found
               }
          }
     }
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @brief Gets an iterator for the list of currently detected monitors.
 *
 * The iterator will provide Ecore_Win32_Monitor_Priv structures.
 * The caller should not free the structures obtained from the iterator,
 * but must free the iterator itself using eina_iterator_free().
 *
 * @return An Eina_Iterator for the list of monitors.
 *         The data pointed to by the iterator is of type Ecore_Win32_Monitor_Priv*.
 *         Example:
 *         ```c
 *         Eina_Iterator *it = ecore_win32_monitors_get();
 *         Ecore_Win32_Monitor_Priv *monitor_priv;
 *         EINA_ITERATOR_FOREACH(it, monitor_priv)
 *         {
 *             // Access monitor_priv->monitor for public Ecore_Win32_Monitor data
 *             printf("Monitor: %s, X: %d, Y: %d, W: %d, H: %d, DPI_X: %u, DPI_Y: %u\n",
 *                    monitor_priv->name,
 *                    monitor_priv->monitor.desktop.x,
 *                    monitor_priv->monitor.desktop.y,
 *                    monitor_priv->monitor.desktop.w,
 *                    monitor_priv->monitor.desktop.h,
 *                    monitor_priv->monitor.dpi.x,
 *                    monitor_priv->monitor.dpi.y);
 *         }
 *         eina_iterator_free(it);
 *         ```
 */
EAPI Eina_Iterator *
ecore_win32_monitors_get(void)
{
   return eina_list_iterator_new(ecore_win32_monitors);
}
