#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "Elementary.h"

#ifndef EFL_BUILD
# define EFL_BUILD
#endif
#undef ELM_MODULE_HELPER_H
#include "elm_module_helper.h"

// module api funcs needed
/**
 * @brief Initialize the module.
 * @param m The module data (unused).
 * @return 1 on success, 0 on failure.
 *
 * This function is called by the module loader to initialize the module.
 * It currently does nothing and always returns success.
 */
EMODAPI int
elm_modapi_init(void *m EINA_UNUSED)
{
   return 1; // succeed always
}

/**
 * @brief Shutdown the module.
 * @param m The module data (unused).
 * @return 1 on success, 0 on failure.
 *
 * This function is called by the module loader to shut down the module.
 * It currently does nothing and always returns success.
 */
EMODAPI int
elm_modapi_shutdown(void *m EINA_UNUSED)
{
   return 1; // succeed always
}

// module fucns for the specific module type
/**
 * @brief Hook an Evas object.
 * @param obj The Evas object to hook.
 *
 * This function is called when an object is "hooked" by the module.
 * This is a placeholder implementation that prints the object's memory address.
 */
EMODAPI void
obj_hook(Evas_Object *obj)
{
   printf("hook: %p\n", obj);
}

/**
 * @brief Unhook an Evas object.
 * @param obj The Evas object to unhook.
 *
 * This function is called when an object is "unhooked" from the module.
 * This is a placeholder implementation that prints the object's memory address.
 */
EMODAPI void
obj_unhook(Evas_Object *obj)
{
   printf("unhook: %p\n", obj);
}

/**
 * @brief Handle a long-press event on an Evas object.
 * @param obj The Evas object that received the long-press event.
 *
 * This function is called when a long-press event occurs on a hooked object.
 * This is a placeholder implementation that prints the object's memory address.
 */
EMODAPI void
obj_longpress(Evas_Object *obj)
{
   printf("longpress: %p\n", obj);
}

/**
 * @internal
 * @brief Internal module initialization.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * This function is called by the EINA_MODULE_INIT macro when the module
 * is loaded. It performs any necessary setup for the module.
 * It currently does nothing and always returns success.
 */
static Eina_Bool
_module_init(void)
{
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Internal module shutdown.
 *
 * This function is called by the EINA_MODULE_SHUTDOWN macro when the module
 * is unloaded. It performs any necessary cleanup for the module.
 * It currently does nothing.
 */
static void
_module_shutdown(void)
{
}

EINA_MODULE_VERSION("0.1");
EINA_MODULE_AUTHOR("Enlightenment Community");
EINA_MODULE_DESCRIPTION("Entry test");
EINA_MODULE_LICENSE("GPLv2");

EINA_MODULE_INIT(_module_init);
EINA_MODULE_SHUTDOWN(_module_shutdown);
