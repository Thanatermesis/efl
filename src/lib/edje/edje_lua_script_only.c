#include "edje_private.h"

/**
 * @brief Checks if the Edje object is configured to run only Lua scripts.
 *
 * This function determines if the associated Edje collection is set to
 * lua_script_only mode.
 *
 * @param ed The Edje object.
 * @return EINA_TRUE if the Edje object is in Lua script only mode, EINA_FALSE otherwise.
 */
Eina_Bool
_edje_lua_script_only(Edje *ed)
{
   if ((ed->collection) && (ed->collection->lua_script_only))
     return EINA_TRUE;
   return EINA_FALSE;
}

/**
 * @brief Initializes Lua scripting for an Edje object if its collection supports it.
 *
 * This function calls _edje_lua2_script_init if the Edje object has an
 * associated collection.
 *
 * @param ed The Edje object to initialize Lua scripting for.
 */
void
_edje_lua_script_only_init(Edje *ed)
{
   if (ed->collection)
     _edje_lua2_script_init(ed);
}

/**
 * @brief Shuts down the Lua scripting environment for an Edje object.
 *
 * This function calls _edje_lua2_script_func_shutdown if the Edje object
 * has an associated collection and a Lua state (ed->L) exists.
 *
 * @param ed The Edje object whose Lua scripting environment is to be shut down.
 */
void
_edje_lua_script_only_shutdown(Edje *ed)
{
   if (ed->collection && ed->L)
     _edje_lua2_script_func_shutdown(ed);
}

/**
 * @brief Calls the Lua 'show' function for an Edje object.
 *
 * This function invokes the Lua script's 'show' function if the Edje object
 * has an associated collection and a Lua state (ed->L) exists. This is typically
 * called when the Edje object becomes visible.
 *
 * @param ed The Edje object.
 */
void
_edje_lua_script_only_show(Edje *ed)
{
   if (ed->collection && ed->L)
     _edje_lua2_script_func_show(ed);
}

/**
 * @brief Calls the Lua 'hide' function for an Edje object.
 *
 * This function invokes the Lua script's 'hide' function if the Edje object
 * has an associated collection and a Lua state (ed->L) exists. This is typically
 * called when the Edje object becomes hidden.
 *
 * @param ed The Edje object.
 */
void
_edje_lua_script_only_hide(Edje *ed)
{
   if (ed->collection && ed->L)
     _edje_lua2_script_func_hide(ed);
}

/**
 * @brief Calls the Lua 'move' function for an Edje object.
 *
 * This function invokes the Lua script's 'move' function if the Edje object
 * has an associated collection and a Lua state (ed->L) exists. This is typically
 * called when the Edje object's position changes.
 *
 * @param ed The Edje object.
 */
void
_edje_lua_script_only_move(Edje *ed)
{
   if (ed->collection && ed->L)
     _edje_lua2_script_func_move(ed);
}

/**
 * @brief Calls the Lua 'resize' function for an Edje object.
 *
 * This function invokes the Lua script's 'resize' function if the Edje object
 * has an associated collection and a Lua state (ed->L) exists. This is typically
 * called when the Edje object's size changes.
 *
 * @param ed The Edje object.
 */
void
_edje_lua_script_only_resize(Edje *ed)
{
   if (ed->collection && ed->L)
     _edje_lua2_script_func_resize(ed);
}

/**
 * @brief Calls the Lua 'message' function for an Edje object to handle an incoming message.
 *
 * This function invokes the Lua script's 'message' function if the Edje object
 * has an associated collection and a Lua state (ed->L) exists. This allows the
 * Lua script to process messages sent to the Edje object.
 *
 * @param ed The Edje object.
 * @param em The message to be processed.
 */
void
_edje_lua_script_only_message(Edje *ed, Edje_Message *em)
{
   if (ed->collection && ed->L)
     _edje_lua2_script_func_message(ed, em);
}

