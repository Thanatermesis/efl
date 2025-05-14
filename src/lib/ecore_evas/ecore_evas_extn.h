#ifndef ECORE_EVAS_EXTN_H_
# define ECORE_EVAS_EXTN_H_

/**
 * @file
 * @brief Defines the Ecore_Evas extension interface.
 */

/**
 * @brief Opaque type for the Ecore_Evas extension interface.
 */
typedef struct _Ecore_Evas_Interface_Extn Ecore_Evas_Interface_Extn;

/**
 * @brief Structure defining the interface for an Ecore_Evas extension.
 *
 * This structure extends the base Ecore_Evas_Interface with functions
 * specific to external Ecore_Evas instances, such as those used for
 * client-server communication (e.g., Wayland, X11 sockets).
 */
struct _Ecore_Evas_Interface_Extn
{
   Ecore_Evas_Interface base; /**< Base Ecore_Evas interface. */

   /**
    * @brief Locks data access for the Ecore_Evas.
    * @param ee The Ecore_Evas instance.
    *
    * This function is used to synchronize access to shared data structures
    * within the Ecore_Evas extension, particularly in multi-threaded
    * scenarios.
    */
   void            (*data_lock)(Ecore_Evas *ee);
   /**
    * @brief Unlocks data access for the Ecore_Evas.
    * @param ee The Ecore_Evas instance.
    *
    * This function releases the lock acquired by data_lock().
    */
   void            (*data_unlock)(Ecore_Evas *ee);
   /**
    * @brief Connects the Ecore_Evas to a service.
    * @param ee The Ecore_Evas instance.
    * @param svcname The name of the service to connect to (e.g., "wayland-0", "display:0").
    * @param svcnum A service-specific number (e.g., display number for X11).
    * @param svcsys A boolean indicating if the service is a system service.
    * @return EINA_TRUE on success, EINA_FALSE on failure.
    *
    * This function attempts to establish a connection to an external service
    * that provides a display or rendering context.
    */
   Eina_Bool       (*connect)(Ecore_Evas *ee, const char *svcname, int svcnum, Eina_Bool svcsys);
   /**
    * @brief Makes the Ecore_Evas listen for incoming connections on a service.
    * @param ee The Ecore_Evas instance.
    * @param svcname The name of the service to listen on (e.g., "wayland-0", "display:0").
    * @param svcnum A service-specific number (e.g., display number for X11).
    * @param svcsys A boolean indicating if the service is a system service.
    * @return EINA_TRUE on success, EINA_FALSE on failure.
    *
    * This function sets up the Ecore_Evas to act as a server, listening for
    * client connections on the specified service.
    */
   Eina_Bool       (*listen)(Ecore_Evas *ee, const char *svcname, int svcnum, Eina_Bool svcsys);
};

#endif
