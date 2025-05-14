#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "codegen.h"

/**
 * @brief Frees a DBus_Property structure and removes it from its interface's list.
 *
 * @param property The property to free.
 */
void
property_free(DBus_Property *property)
{
   property->iface->properties = eina_inlist_remove(property->iface->properties,
                                                    EINA_INLIST_GET(property));
   free(property->c_name);
   free(property->cb_name);
   free(property->name);
   free(property->type);
   free(property);
}

/**
 * @brief Creates a new DBus_Property and adds it to the given interface.
 *
 * @param iface The interface to which the new property will belong.
 * @return A pointer to the newly created DBus_Property, or NULL on failure.
 */
DBus_Property *
property_new(DBus_Interface *iface)
{
   DBus_Property *prop = calloc(1, sizeof(DBus_Property));
   EINA_SAFETY_ON_NULL_RETURN_VAL(prop, NULL);
   iface->properties = eina_inlist_append(iface->properties,
                                          EINA_INLIST_GET(prop));
   prop->iface = iface;
   return prop;
}

/**
 * @brief Creates a new DBus_Method and adds it to the given interface.
 *
 * @param iface The interface to which the new method will belong.
 * @return A pointer to the newly created DBus_Method, or NULL on failure.
 */
DBus_Method *
method_new(DBus_Interface *iface)
{
   DBus_Method *method = calloc(1, sizeof(DBus_Method));
   EINA_SAFETY_ON_NULL_RETURN_VAL(method, NULL);
   iface->methods = eina_inlist_append(iface->methods, EINA_INLIST_GET(method));
   method->iface = iface;
   return method;
}

/**
 * @brief Frees a DBus_Method structure, its arguments, and removes it from its interface's list.
 *
 * @param method The method to free.
 */
void
method_free(DBus_Method *method)
{
   DBus_Arg *arg;
   Eina_Inlist *inlist;

   EINA_INLIST_FOREACH_SAFE(method->args, inlist, arg)
     arg_free(arg);
   method->iface->methods = eina_inlist_remove(method->iface->methods,
                                               EINA_INLIST_GET(method));
   free(method->c_name);
   free(method->cb_name);
   free(method->function_cb);
   free(method->name);
   free(method);
}

/**
 * @brief Frees a DBus_Arg structure.
 *
 * @param arg The argument to free.
 */
void
arg_free(DBus_Arg *arg)
{
   free(arg->c_name);
   free(arg->name);
   free(arg->type);
   free(arg);
}

/**
 * @brief Creates a new DBus_Signal and adds it to the given interface.
 *
 * @param iface The interface to which the new signal will belong.
 * @return A pointer to the newly created DBus_Signal, or NULL on failure.
 */
DBus_Signal *
signal_new(DBus_Interface *iface)
{
   DBus_Signal *sig = calloc(1, sizeof(DBus_Signal));
   EINA_SAFETY_ON_NULL_RETURN_VAL(sig, NULL);
   iface->signals = eina_inlist_append(iface->signals, EINA_INLIST_GET(sig));
   sig->iface = iface;
   return sig;
}

/**
 * @brief Frees a DBus_Signal structure, its arguments, and removes it from its interface's list.
 *
 * @param sig The signal to free.
 */
void
signal_free(DBus_Signal *sig)
{
   DBus_Arg *arg;
   Eina_Inlist *inlist;

   EINA_INLIST_FOREACH_SAFE(sig->args, inlist, arg)
     arg_free(arg);
   sig->iface->signals = eina_inlist_remove(sig->iface->signals,
                                               EINA_INLIST_GET(sig));
   free(sig->c_name);
   free(sig->struct_name);
   free(sig->free_function);
   free(sig->cb_name);
   free(sig->name);
   free(sig->signal_event);
   free(sig);
}

/**
 * @brief Creates a new DBus_Interface and adds it to the given D-Bus object.
 *
 * @param obj The D-Bus object to which the new interface will belong.
 * @return A pointer to the newly created DBus_Interface, or NULL on failure.
 */
DBus_Interface *
interface_new(DBus_Object *obj)
{
   DBus_Interface *iface = calloc(1, sizeof(DBus_Interface));
   EINA_SAFETY_ON_NULL_RETURN_VAL(iface, NULL);
   obj->ifaces = eina_inlist_append(obj->ifaces, EINA_INLIST_GET(iface));
   iface->obj = obj;
   return iface;
}

/**
 * @brief Frees a DBus_Interface structure, all its signals, methods, and properties,
 * and removes it from its object's list.
 *
 * @param iface The interface to free.
 */
void
interface_free(DBus_Interface *iface)
{
   while (iface->signals)
     {
        DBus_Signal *sig = EINA_INLIST_CONTAINER_GET(iface->signals,
                                                        DBus_Signal);
        signal_free(sig);
     }
   while (iface->methods)
     {
        DBus_Method *method = EINA_INLIST_CONTAINER_GET(iface->methods,
                                                        DBus_Method);
        method_free(method);
     }
   while (iface->properties)
     {
        DBus_Property *property = EINA_INLIST_CONTAINER_GET(iface->properties,
                                                            DBus_Property);
        property_free(property);
     }
   iface->obj->ifaces = eina_inlist_remove(iface->obj->ifaces,
                                           EINA_INLIST_GET(iface));
   free(iface->c_name);
   free(iface->name);
   free(iface);
}

/**
 * @brief Frees a DBus_Object structure and all its interfaces.
 *
 * @param obj The D-Bus object to free.
 */
void
object_free(DBus_Object *obj)
{
   while (obj->ifaces)
     {
        DBus_Interface *iface = EINA_INLIST_CONTAINER_GET(obj->ifaces,
                                                          DBus_Interface);
        interface_free(iface);
     }
   free(obj->c_name);
   free(obj->name);
   free(obj);
}
