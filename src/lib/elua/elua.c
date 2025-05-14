#include <limits.h>
#include <Ecore_File.h>
#include "elua_private.h"

static Eina_Prefix *_elua_pfx = NULL; ///< Prefix for Elua library related paths.

static int _elua_init_counter = 0; ///< Initialization counter for elua_init/elua_shutdown.
int _elua_log_dom = -1; ///< Log domain for Elua.

/**
 * @brief Initializes the Elua library.
 *
 * This function initializes Eina and Ecore_File, registers a log domain for Elua,
 * and sets up the Elua prefix for path resolution.
 * It uses a counter to handle multiple init calls.
 *
 * @return The new init counter value on success, EINA_FALSE on failure.
 */
EAPI int
elua_init(void)
{
   const char *dom = "elua";
   if (_elua_init_counter > 0) return ++_elua_init_counter;

   eina_init();
   ecore_file_init();

   _elua_log_dom = eina_log_domain_register(dom, EINA_COLOR_LIGHTBLUE);
   if (_elua_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: %s", dom);
        return EINA_FALSE;
     }

   eina_log_timing(_elua_log_dom, EINA_LOG_STATE_STOP, EINA_LOG_STATE_INIT);
   INF("elua init");

   _elua_pfx = eina_prefix_new(NULL, elua_init, "ELUA", "elua", "checkme",
                               PACKAGE_BIN_DIR, "", PACKAGE_DATA_DIR,
                               LOCALE_DIR);

   if (!_elua_pfx)
     {
        ERR("could not find elua prefix");
        return EINA_FALSE;
     }

   return ++_elua_init_counter;
}

/**
 * @brief Shuts down the Elua library.
 *
 * This function decrements the init counter. If the counter reaches zero,
 * it frees the Elua prefix, unregisters the log domain, and shuts down
 * Ecore_File and Eina.
 *
 * @return The new init counter value on success, EINA_FALSE if init count was already zero or less.
 */
EAPI int
elua_shutdown(void)
{
   if (_elua_init_counter <= 0)
     {
        EINA_LOG_ERR("Init count not greater than 0 in shutdown.");
        return EINA_FALSE;
     }
   --_elua_init_counter;

   if (_elua_init_counter > 0)
     return _elua_init_counter;

   INF("shutdown");
   eina_log_timing(_elua_log_dom, EINA_LOG_STATE_START, EINA_LOG_STATE_SHUTDOWN);

   eina_prefix_free(_elua_pfx);
   _elua_pfx = NULL;

   eina_log_domain_unregister(_elua_log_dom);
   _elua_log_dom = -1;

   ecore_file_shutdown();
   eina_shutdown();
   return _elua_init_counter;
}

#ifdef ENABLE_LUA_OLD
/**
 * @brief Lua C function to load the CFFI module as 'ffi'.
 *
 * This function is used as a custom loader for the 'ffi' module.
 * It expects two upvalues:
 * 1. The actual CFFI module (or its loader).
 * 2. The name "cffi".
 * It calls the CFFI module loader with "cffi" and the second upvalue.
 *
 * @param L The Lua state.
 * @return 1, leaving the loaded module on the stack.
 */
static int
_ffi_loader(lua_State *L)
{
   lua_pushvalue(L, lua_upvalueindex(1));
   lua_pushliteral(L, "cffi");
   lua_pushvalue(L, lua_upvalueindex(2));
   lua_call(L, 2, 1);
   return 1;
}

#if LUA_VERSION_NUM < 502
/* adapted from lua 5.2 source */
/**
 * @brief Pushes the next path template from a path string onto the Lua stack.
 *
 * This function is an adaptation from Lua 5.2 source for Lua 5.1.
 * It parses a path string (like package.path) and pushes the next
 * template (substring between separators) onto the Lua stack.
 *
 * @param L The Lua state.
 * @param path The current position in the path string.
 * @return Pointer to the character in the path string after the pushed template,
 *         or NULL if no more templates are found.
 */
static const char *
_push_next_template(lua_State *L, const char *path)
{
   while (*path == *LUA_PATHSEP) ++path;
   if (!*path)
     return NULL;
   const char *l = strchr(path, *LUA_PATHSEP);
   if (!l)
     l = path + strlen(path);
   lua_pushlstring(L, path, l - path);
   return l;
}

/**
 * @brief Implements package.searchpath for Lua 5.1.
 *
 * Lua 5.1 does not have `package.searchpath`. This function provides
 * a compatible implementation. It searches for a Lua module given its name
 * and a path string.
 *
 * @param L The Lua state.
 *   - Stack index 1: module name (string).
 *   - Stack index 2: path string (string).
 *   - Stack index 3 (optional): path separator (string, default ".").
 *   - Stack index 4 (optional): directory separator (string, default LUA_DIRSEP).
 * @return 2 if not found (nil, error message), or 1 if found (filename).
 */
static int
_elua_searchpath(lua_State *L)
{
   const char *name = luaL_checkstring(L, 1);
   const char *path = luaL_checkstring(L, 2);
   const char *sep  = luaL_optstring(L, 3, ".");
   const char *dsep = luaL_optstring(L, 4, LUA_DIRSEP);
   luaL_Buffer msg;
   luaL_buffinit(L, &msg);
   if (*sep)
     name = luaL_gsub(L, name, sep, dsep);
   while ((path = _push_next_template(L, path)))
     {
        const char *fname = luaL_gsub(L, lua_tostring(L, -1), LUA_PATH_MARK, name);
        lua_remove(L, -2);
        FILE *rf = fopen(fname, "r");
        if (rf)
          {
             fclose(rf);
             return 1; /* found */
          }
        lua_pushfstring(L, "\n\tno file " LUA_QS, fname);
        lua_remove(L, -2);
        luaL_addvalue(&msg);
     }
   luaL_pushresult(&msg);
   lua_pushnil(L);
   lua_insert(L, -2);
   return 2; /* nil plus error message */
}
#endif
#endif

/**
 * @brief Creates a new Elua state.
 *
 * This function initializes a new Lua state, sets up basic Elua structures
 * within it (like storing the Elua_State pointer in the Lua registry for
 * later retrieval), and opens standard Lua libraries.
 * If ENABLE_LUA_OLD is defined, it attempts to set up CFFI.
 *
 * @param progname The program name, used for error reporting. Can be NULL.
 * @return A pointer to the newly created Elua_State, or NULL on failure.
 */
EAPI Elua_State *
elua_state_new(const char *progname)
{
   Elua_State *ret = NULL;
   lua_State *L = luaL_newstate();
   if (!L)
     return NULL;
   ret = calloc(1, sizeof(Elua_State));
   ret->luastate = L;
   if (progname) ret->progname = eina_stringshare_add(progname);
   luaL_openlibs(L);
#ifdef ENABLE_LUA_OLD
   /* search for cffi-lua early, and pass it through as ffi */
   lua_getglobal(L, "package");
#if LUA_VERSION_NUM < 502
   /* lua 5.1 does not have package.searchpath, we rely on having that */
   lua_getfield(L, -1, "searchpath");
   if (lua_isnil(L, -1))
     {
        lua_pushcfunction(L, _elua_searchpath);
        lua_setfield(L, -3, "searchpath");
     }
   lua_pop(L, 1);
#endif
   lua_getfield(L, -1, "preload");
   lua_getfield(L, -2, "searchers");
   if (lua_isnil(L, -1))
     {
        lua_pop(L, 1);
        lua_getfield(L, -2, "loaders");
     }
   if (lua_isnil(L, -1))
     {
        ERR("could not find a module searcher");
        goto err;
     }
   lua_rawgeti(L, -1, 3);
   lua_pushliteral(L, "cffi");
   if (lua_pcall(L, 1, 2, 0))
     {
        ERR("could not find the cffi module");
        goto err;
     }
   if (!lua_isfunction(L, -2))
     {
        ERR("could not find the cffi module: %s", lua_tostring(L, -2));
        goto err;
     }
   lua_pushcclosure(L, _ffi_loader, 2);
   lua_setfield(L, -3, "ffi");
   lua_pop(L, 3);
#endif
   /* on 64-bit, split the state pointer into two and reconstruct later */
   size_t retn = (size_t)ret;
   if (sizeof(void *) < sizeof(lua_Number))
     {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, (lua_Number)retn);
     }
   else
     {
        size_t hbits = (sizeof(void *) / 2) * CHAR_BIT;
        lua_pushnumber(L, (lua_Number)(retn >> hbits));
        lua_pushnumber(L, (lua_Number)(retn & (((size_t)1 << hbits) - 1)));
     }
   lua_setfield(L, LUA_REGISTRYINDEX, "elua_ptr1"); // Store the higher bits of Elua_State*
   lua_setfield(L, LUA_REGISTRYINDEX, "elua_ptr2"); // Store the lower bits of Elua_State* or full pointer if lua_Number is small
   return ret;
#ifdef ENABLE_LUA_OLD
err: // Error handling path for CFFI setup failure
   lua_close(L);
   eina_stringshare_del(ret->progname);
   free(ret);
   return NULL;
#endif
}

/**
 * @brief Frees an Elua_State and its associated resources.
 *
 * This function closes the Lua state, frees any C module cleanup functions,
 * and releases all string shares and allocated memory associated with the
 * Elua_State.
 *
 * @param es The Elua_State to free. If NULL, the function does nothing.
 */
EAPI void
elua_state_free(Elua_State *es)
{
   void *data;
   if (!es) return;
   if (es->luastate)
     {
        EINA_LIST_FREE(es->cmods, data)
          {
             lua_rawgeti(es->luastate, LUA_REGISTRYINDEX, (size_t)data);
             lua_call(es->luastate, 0, 0);
          }
        lua_close(es->luastate);
     }
   else if (es->cmods)
     eina_list_free(es->cmods);
   EINA_LIST_FREE(es->lmods, data)
     eina_stringshare_del(data);
   EINA_LIST_FREE(es->lincs, data)
     eina_stringshare_del(data);
   eina_stringshare_del(es->progname);
   eina_stringshare_del(es->coredir);
   eina_stringshare_del(es->moddir);
   eina_stringshare_del(es->appsdir);
   free(es);
}

/**
 * @brief Sets the core, modules, and application directories for an Elua_State.
 *
 * Paths are sanitized and stored as Eina_Stringshare.
 *
 * @param es The Elua_State to modify.
 * @param core The path to the core scripts directory. If NULL, the existing path is kept.
 * @param mods The path to the modules directory. If NULL, the existing path is kept.
 * @param apps The path to the applications directory. If NULL, the existing path is kept.
 */
EAPI void
elua_state_dirs_set(Elua_State *es, const char *core, const char *mods,
                    const char *apps)
{
   char *spath = NULL;
   EINA_SAFETY_ON_NULL_RETURN(es);
   if (core)
     {
        eina_stringshare_del(es->coredir);
        spath = eina_file_path_sanitize(core);
        es->coredir = eina_stringshare_add(spath);
        free(spath);
     }
   if (mods)
     {
        eina_stringshare_del(es->moddir);
        spath = eina_file_path_sanitize(mods);
        es->moddir = eina_stringshare_add(spath);
        free(spath);
     }
   if (apps)
     {
        eina_stringshare_del(es->appsdir);
        spath = eina_file_path_sanitize(apps);
        es->appsdir = eina_stringshare_add(spath);
        free(spath);
     }
}

/**
 * @brief Fills the directory paths in Elua_State if they are not already set.
 *
 * It tries to get paths from environment variables (ELUA_CORE_DIR,
 * ELUA_MODULES_DIR, ELUA_APPS_DIR) first, unless `ignore_env` is true.
 * If environment variables are not set or ignored, it falls back to default
 * paths relative to the Elua prefix.
 *
 * @param es The Elua_State to modify.
 * @param ignore_env If EINA_TRUE, environment variables are ignored.
 */
EAPI void
elua_state_dirs_fill(Elua_State *es, Eina_Bool ignore_env)
{
   const char *coredir = NULL, *moddir = NULL, *appsdir = NULL;
   char coredirbuf[PATH_MAX], moddirbuf[PATH_MAX], appsdirbuf[PATH_MAX];
   EINA_SAFETY_ON_NULL_RETURN(es);
   if (!(coredir = es->coredir))
     {
        if (ignore_env || !(coredir = getenv("ELUA_CORE_DIR")) || !coredir[0])
          {
             coredir = coredirbuf;
             snprintf(coredirbuf, sizeof(coredirbuf), "%s/core",
                      eina_prefix_data_get(_elua_pfx));
          }
        if (coredir) {
            char *sdir = eina_file_path_sanitize(coredir);
            es->coredir = eina_stringshare_add(sdir);
            free(sdir);
        }
     }
   if (!(moddir = es->moddir))
     {
        if (ignore_env || !(moddir = getenv("ELUA_MODULES_DIR")) || !moddir[0])
          {
             moddir = moddirbuf;
             snprintf(moddirbuf, sizeof(moddirbuf), "%s/modules",
                      eina_prefix_data_get(_elua_pfx));
          }
        if (moddir) {
            char *sdir = eina_file_path_sanitize(moddir);
            es->moddir = eina_stringshare_add(sdir);
            free(sdir);
        }
     }
   if (!(appsdir = es->appsdir))
     {
        if (ignore_env || !(appsdir = getenv("ELUA_APPS_DIR")) || !appsdir[0])
          {
             appsdir = appsdirbuf;
             snprintf(appsdirbuf, sizeof(appsdirbuf), "%s/apps",
                      eina_prefix_data_get(_elua_pfx));
          }
        if (appsdir) {
            char *sdir = eina_file_path_sanitize(appsdir);
            es->appsdir = eina_stringshare_add(sdir);
            free(sdir);
        }
     }
}

/**
 * @brief Gets the core directory path from an Elua_State.
 *
 * @param es The Elua_State.
 * @return The Eina_Stringshare for the core directory path, or NULL if not set or `es` is NULL.
 *         The caller should not free the returned stringshare.
 */
EAPI Eina_Stringshare *
elua_state_core_dir_get(const Elua_State *es)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, NULL);
   return es->coredir;
}

/**
 * @brief Gets the modules directory path from an Elua_State.
 *
 * @param es The Elua_State.
 * @return The Eina_Stringshare for the modules directory path, or NULL if not set or `es` is NULL.
 *         The caller should not free the returned stringshare.
 */
EAPI Eina_Stringshare *
elua_state_mod_dir_get(const Elua_State *es)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, NULL);
   return es->moddir;
}

/**
 * @brief Gets the applications directory path from an Elua_State.
 *
 * @param es The Elua_State.
 * @return The Eina_Stringshare for the applications directory path, or NULL if not set or `es` is NULL.
 *         The caller should not free the returned stringshare.
 */
EAPI Eina_Stringshare *
elua_state_apps_dir_get(const Elua_State *es)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, NULL);
   return es->appsdir;
}

/**
 * @brief Gets the program name associated with an Elua_State.
 *
 * @param es The Elua_State.
 * @return The Eina_Stringshare for the program name, or NULL if not set or `es` is NULL.
 *         The caller should not free the returned stringshare.
 */
EAPI Eina_Stringshare *
elua_state_prog_name_get(const Elua_State *es)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, NULL);
   return es->progname;
}

/**
 * @brief Adds a path to the Lua include paths list for an Elua_State.
 *
 * These paths are used by the custom module loader to search for Lua files.
 * The path is sanitized and stored as an Eina_Stringshare.
 *
 * @param es The Elua_State to modify.
 * @param path The include path to add. Must not be NULL or empty.
 */
EAPI void
elua_state_include_path_add(Elua_State *es, const char *path)
{
   char *spath = NULL;
   EINA_SAFETY_ON_NULL_RETURN(es);
   EINA_SAFETY_ON_NULL_RETURN(path);
   EINA_SAFETY_ON_FALSE_RETURN(path[0]);
   spath = eina_file_path_sanitize(path);
   es->lincs = eina_list_append(es->lincs, eina_stringshare_add(spath));
   free(spath);
}

/**
 * @brief Pushes the Elua 'require' function onto the Lua stack.
 *
 * The 'require' function is stored in the Lua registry during Elua setup.
 * This function retrieves it.
 *
 * @param es The Elua_State.
 * @return EINA_TRUE if the function was successfully pushed, EINA_FALSE otherwise
 *         (e.g., if `es` is NULL or the reference is LUA_REFNIL).
 */
EAPI Eina_Bool
elua_state_require_ref_push(Elua_State *es)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(es->requireref != LUA_REFNIL, EINA_FALSE);
   lua_rawgeti(es->luastate, LUA_REGISTRYINDEX, es->requireref);
   return EINA_TRUE;
}

/**
 * @brief Pushes the Elua 'appload' function onto the Lua stack.
 *
 * The 'appload' function is stored in the Lua registry during Elua setup.
 * This function retrieves it.
 *
 * @param es The Elua_State.
 * @return EINA_TRUE if the function was successfully pushed, EINA_FALSE otherwise
 *         (e.g., if `es` is NULL or the reference is LUA_REFNIL).
 */
EAPI Eina_Bool
elua_state_appload_ref_push(Elua_State *es)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(es->apploadref != LUA_REFNIL, EINA_FALSE);
   lua_rawgeti(es->luastate, LUA_REGISTRYINDEX, es->apploadref);
   return EINA_TRUE;
}

/**
 * @brief Gets the underlying lua_State from an Elua_State.
 *
 * @param es The Elua_State.
 * @return The lua_State pointer, or NULL if `es` is NULL.
 */
EAPI lua_State *
elua_state_lua_state_get(const Elua_State *es)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, NULL);
   return es->luastate;
}

/**
 * @brief Retrieves the Elua_State associated with a lua_State.
 *
 * The Elua_State pointer is stored in the Lua registry using two keys
 * ("elua_ptr1", "elua_ptr2") to handle potential size differences between
 * pointers and lua_Number, especially on 64-bit systems. This function
 * reconstructs the pointer.
 *
 * @param L The lua_State.
 * @return The Elua_State pointer, or NULL if not found or `L` is NULL.
 */
EAPI Elua_State *
elua_state_from_lua_state_get(lua_State *L)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(L, NULL);
   lua_getfield(L, LUA_REGISTRYINDEX, "elua_ptr1");
   lua_getfield(L, LUA_REGISTRYINDEX, "elua_ptr2");
   if (!lua_isnil(L, -1) && !lua_isnil(L, -2))
     {
        size_t p1 = (size_t)lua_tonumber(L, -2),
               p2 = (size_t)lua_tonumber(L, -1);
        if (p2 && (sizeof(void *) >= sizeof(lua_Number)))
          p1 |= p2 << ((sizeof(void *) / 2) * CHAR_BIT);
        lua_pop(L, 2);
        return (Elua_State *)p1;
     }
   lua_pop(L, 2);
   return NULL;
}

/**
 * @brief Lua C function to bind a text domain for internationalization.
 *
 * Wraps `bindtextdomain` and `bind_textdomain_codeset`.
 * It prevents binding the default package domain if it's "elua".
 *
 * @param L The Lua state.
 *   - Stack index 1: textdomain name (string).
 *   - Stack index 2: directory name containing message catalogs (string).
 * @return 1 with the bound directory path on success, or 2 with nil and an error message on failure.
 *         If NLS is disabled, returns 1 with an empty string.
 */
static int
_elua_gettext_bind_textdomain(lua_State *L)
{
#ifdef ENABLE_NLS
   const char *textdomain = luaL_checkstring(L, 1);
   const char *dirname    = luaL_checkstring(L, 2);
   const char *ret;
   if (!textdomain[0] || !strcmp(textdomain, PACKAGE))
     {
        lua_pushnil(L);
        lua_pushliteral(L, "invalid textdomain");
        return 2;
     }
   if (!(ret = bindtextdomain(textdomain, dirname)))
     {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
     }
   bind_textdomain_codeset(textdomain, "UTF-8");
   lua_pushstring(L, ret);
   return 1;
#else
   lua_pushliteral(L, "");
   return 1;
#endif
}

/**
 * @brief Lua C function to get the current message language.
 *
 * It checks standard environment variables (LANGUAGE, LC_ALL, LC_MESSAGES, LANG)
 * in order to determine the language.
 *
 * @param L The Lua state.
 * @return 1, pushing the language string onto the stack, or nil if no language
 *         environment variable is found.
 */
static int
_elua_get_message_language(lua_State *L)
{
   const char *e;
   e = getenv("LANGUAGE");
   if (e && e[0]) goto success;
   e = getenv("LC_ALL");
   if (e && e[0]) goto success;
   e = getenv("LC_MESSAGES");
   if (e && e[0]) goto success;
   e = getenv("LANG");
   if (e && e[0]) goto success;
   lua_pushnil(L);
   return 1;
success:
   lua_pushstring(L, e);
   return 1;
};

/**
 * @brief Lua C function to get locale-specific numeric and monetary formatting information.
 *
 * Wraps `localeconv()` and returns its contents as a Lua table.
 * CHAR_MAX values for char fields in `struct lconv` are converted to -1.
 *
 * @param L The Lua state.
 * @return 1, pushing a Lua table with lconv fields onto the stack.
 * The table keys are strings matching the `struct lconv` field names.
 * Example table structure:
 * {
 *   decimal_point = ".",
 *   thousands_sep = ",",
 *   grouping = "\003",
 *   int_curr_symbol = "USD ",
 *   currency_symbol = "$",
 *   mon_decimal_point = ".",
 *   mon_thousands_sep = ",",
 *   mon_grouping = "\003",
 *   positive_sign = "",
 *   negative_sign = "-",
 *   frac_digits = 2,
 *   p_cs_precedes = 1,
 *   n_cs_precedes = 1,
 *   p_sep_by_space = 0,
 *   n_sep_by_space = 0,
 *   p_sign_posn = 1,
 *   n_sign_posn = 1,
 *   int_frac_digits = 2
 * }
 */
static int
_elua_get_localeconv(lua_State *L)
{
   struct lconv *lc = localeconv();
   lua_createtable(L, 0, 24);

#define ELUA_LCF_S(name) \
   lua_pushstring(L, lc->name); \
   lua_setfield(L, -2, #name);

#define ELUA_LCF_C(name) \
   lua_pushinteger(L, (lc->name == CHAR_MAX) ? -1 : (int)lc->name); \
   lua_setfield(L, -2, #name);

   ELUA_LCF_S(decimal_point);
   ELUA_LCF_S(thousands_sep);
   ELUA_LCF_S(grouping);
   ELUA_LCF_S(int_curr_symbol);
   ELUA_LCF_S(currency_symbol);
   ELUA_LCF_S(mon_decimal_point);
   ELUA_LCF_S(mon_thousands_sep);
   ELUA_LCF_S(mon_grouping);
   ELUA_LCF_S(positive_sign);
   ELUA_LCF_S(negative_sign);

   ELUA_LCF_C(frac_digits);
   ELUA_LCF_C(p_cs_precedes);
   ELUA_LCF_C(n_cs_precedes);
   ELUA_LCF_C(p_sep_by_space);
   ELUA_LCF_C(n_sep_by_space);
   ELUA_LCF_C(p_sign_posn);
   ELUA_LCF_C(n_sign_posn);
   ELUA_LCF_C(int_frac_digits);

#undef ELUA_LCF_S
#undef ELUA_LCF_C

   return 1;
};

#ifdef ENABLE_NLS
/**
 * @brief Lua C function to translate a message using a specific domain (dgettext).
 *
 * @param L The Lua state.
 *   - Stack index 1: domain name (string).
 *   - Stack index 2: message ID (string).
 * @return 1, pushing the translated string or nil if translation fails.
 */
static int
_elua_dgettext(lua_State *L)
{
   const char *domain = luaL_checkstring(L, 1);
   const char *msgid  = luaL_checkstring(L, 2);
   char *ret = dgettext(domain, msgid);
   if (!ret)
     lua_pushnil(L);
   else
     lua_pushstring(L, ret);
   return 1;
}

/**
 * @brief Lua C function to translate a plural message using a specific domain (dngettext).
 *
 * @param L The Lua state.
 *   - Stack index 1: domain name (string).
 *   - Stack index 2: singular message ID (string).
 *   - Stack index 3: plural message ID (string).
 *   - Stack index 4: count (long).
 * @return 1, pushing the translated string or nil if translation fails.
 */
static int
_elua_dngettext(lua_State *L)
{
   const char *domain  = luaL_checkstring(L, 1);
   const char *msgid   = luaL_checkstring(L, 2);
   const char *plmsgid = luaL_checkstring(L, 3);
   char *ret = dngettext(domain, msgid, plmsgid, luaL_checklong(L, 4));
   if (!ret)
     lua_pushnil(L);
   else
     lua_pushstring(L, ret);
   return 1;
}
#endif

/**
 * @brief Lua library definition for gettext related functions.
 * This table is registered into Lua to provide i18n functionalities.
 */
const luaL_Reg gettextlib[] =
{
   { "bind_textdomain", _elua_gettext_bind_textdomain },
   { "get_message_language", _elua_get_message_language },
   { "get_localeconv", _elua_get_localeconv },
#ifdef ENABLE_NLS
   { "dgettext", _elua_dgettext },
   { "dngettext", _elua_dngettext },
#endif
   { NULL, NULL }
};

/**
 * @brief Sets up internationalization (i18n) for the Elua state.
 *
 * This involves loading a Lua script (`gettext.lua` from the core directory)
 * and registering the C functions in `gettextlib` for use by that script.
 *
 * @param es The Elua_State.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_elua_state_i18n_setup(Elua_State *es)
{
   char buf[PATH_MAX];
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(es->coredir, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(es->progname, EINA_FALSE);
   snprintf(buf, sizeof(buf), "%s/gettext.lua", es->coredir);
   if (elua_util_error_report(es, elua_io_loadfile(es, buf)))
     return EINA_FALSE;
   lua_createtable(es->luastate, 0, 0);
   elua_register(es->luastate, gettextlib);
   lua_call(es->luastate, 1, 0);
   return EINA_TRUE;
}

int _elua_module_init(lua_State *L);
int _elua_module_system_init(lua_State *L);

/**
 * @brief Lua C function to check if a path is a directory.
 * Wraps `ecore_file_is_dir`.
 * @param L The Lua state. Stack index 1: path (string).
 * @return 1, pushing a boolean result onto the stack.
 */
static int
_elua_file_is_dir(lua_State *L)
{
   lua_pushboolean(L, ecore_file_is_dir(luaL_checkstring(L, 1)));
   return 1;
}

/**
 * @brief Lua C function to check if a file or directory exists.
 * Wraps `ecore_file_exists`.
 * @param L The Lua state. Stack index 1: path (string).
 * @return 1, pushing a boolean result onto the stack.
 */
static int
_elua_file_exists(lua_State *L)
{
   lua_pushboolean(L, ecore_file_exists(luaL_checkstring(L, 1)));
   return 1;
}

/**
 * @brief Lua C function to create a directory.
 * Wraps `ecore_file_mkdir`.
 * @param L The Lua state. Stack index 1: path (string).
 * @return 1, pushing a boolean result (true on success) onto the stack.
 */
static int
_elua_file_mkdir(lua_State *L)
{
   lua_pushboolean(L, ecore_file_mkdir(luaL_checkstring(L, 1)));
   return 1;
}

/**
 * @brief Lua C function to create a directory path recursively.
 * Wraps `ecore_file_mkpath`.
 * @param L The Lua state. Stack index 1: path (string).
 * @return 1, pushing a boolean result (true on success) onto the stack.
 */
static int
_elua_file_mkpath(lua_State *L)
{
   lua_pushboolean(L, ecore_file_mkpath(luaL_checkstring(L, 1)));
   return 1;
}

/**
 * @brief Lua C function to remove a directory.
 * Wraps `ecore_file_rmdir`.
 * @param L The Lua state. Stack index 1: path (string).
 * @return 1, pushing a boolean result (true on success) onto the stack.
 */
static int
_elua_file_rmdir(lua_State *L)
{
   lua_pushboolean(L, ecore_file_rmdir(luaL_checkstring(L, 1)));
   return 1;
}

/**
 * @brief Lua C function to unlink (delete) a file.
 * Wraps `ecore_file_unlink`.
 * @param L The Lua state. Stack index 1: path (string).
 * @return 1, pushing a boolean result (true on success) onto the stack.
 */
static int
_elua_file_unlink(lua_State *L)
{
   lua_pushboolean(L, ecore_file_unlink(luaL_checkstring(L, 1)));
   return 1;
}

/**
 * @brief Lua C function to recursively remove a file or directory.
 * Wraps `ecore_file_recursive_rm`.
 * @param L The Lua state. Stack index 1: path (string).
 * @return 1, pushing a boolean result (true on success) onto the stack.
 */
static int
_elua_file_rmrf(lua_State *L)
{
   lua_pushboolean(L, ecore_file_recursive_rm(luaL_checkstring(L, 1)));
   return 1;
}

/**
 * @brief Lua library definition for C utility functions.
 * This table is registered into Lua to provide various file system operations
 * and module initialization helpers.
 */
const luaL_Reg _elua_cutillib[] =
{
   { "init_module", _elua_module_init },
   { "popenv"     , _elua_io_popen    },
   { "file_is_dir", _elua_file_is_dir },
   { "file_exists", _elua_file_exists },
   { "file_mkdir" , _elua_file_mkdir  },
   { "file_mkpath", _elua_file_mkpath },
   { "file_rmdir" , _elua_file_rmdir  },
   { "file_unlink", _elua_file_unlink },
   { "file_rmrf"  , _elua_file_rmrf   },
   { NULL         , NULL              }
};

/**
 * @brief Sets up core Elua modules.
 *
 * This function loads a Lua script (`module.lua` from the core directory)
 * and provides it with C helper functions (from `_elua_cutillib`) and
 * a system initialization function (`_elua_module_system_init`).
 *
 * @param es The Elua_State.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_elua_state_modules_setup(const Elua_State *es)
{
   char buf[PATH_MAX];
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(es->coredir, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(es->progname, EINA_FALSE);
   snprintf(buf, sizeof(buf), "%s/module.lua", es->coredir);
   if (elua_util_error_report(es, elua_io_loadfile(es, buf)))
     return EINA_FALSE;
   lua_pushcfunction(es->luastate, _elua_module_system_init);
   lua_createtable(es->luastate, 0, 0);
   elua_register(es->luastate, _elua_cutillib);
   lua_call(es->luastate, 2, 0);
   return EINA_TRUE;
}

/**
 * @brief Lua C function called by `module.lua` to initialize a C module.
 *
 * This function can execute an optional Lua initialization function (first argument)
 * and register an optional Lua cleanup function (second argument) to be called
 * when the Elua_State is freed.
 *
 * @param L The Lua state.
 *   - Stack index 1 (optional): Lua initialization function.
 *   - Stack index 2 (optional): Lua cleanup function.
 * @return 0.
 */
int
_elua_module_init(lua_State *L)
{
   Elua_State *es = elua_state_from_lua_state_get(L);
   if (!lua_isnoneornil(L, 1))
     {
        lua_pushvalue(L, 1);
        lua_call(L, 0, 0);
     }
   if (!lua_isnoneornil(L, 2))
     {
        lua_pushvalue(L, 2);
        es->cmods = eina_list_append(es->cmods,
           (void*)(size_t)luaL_ref(L, LUA_REGISTRYINDEX));
     }
   return 0;
}

/**
 * @brief Lua C function to initialize system-level module settings.
 *
 * This function is called from `module.lua`. It receives Lua functions for
 * `require`, `appload`, and initial Lua search paths. It stores references
 * to `require` and `appload` in the Elua_State and constructs the full
 * Lua module search path (LUA_PATH) and application search path by
 * prepending Elua-specific directories (core, custom includes, modules, apps).
 *
 * @param L The Lua state.
 *   - Stack index 1: The `require` function from `module.lua`.
 *   - Stack index 2: The `appload` function from `module.lua`.
 *   - Stack index 3: The initial Lua module search path string.
 *   - Stack index 4: The initial Lua application search path string.
 * @return 2, pushing the constructed module path and app path onto the stack.
 */
int
_elua_module_system_init(lua_State *L)
{
   Elua_State       *es       = elua_state_from_lua_state_get(L);
   const char       *corepath = es->coredir;
   const char       *modpath  = es->moddir;
   const char       *appspath = es->appsdir;
   Eina_Stringshare *data     = NULL;
   if (!corepath || !modpath || !appspath)
     return 0;
   lua_pushvalue(L, 1);
   es->requireref = luaL_ref(L, LUA_REGISTRYINDEX);
   lua_pushvalue(L, 2);
   es->apploadref = luaL_ref(L, LUA_REGISTRYINDEX);

   /* module path, local directories take priority */
   int n = 0;
   lua_pushvalue(L, 3); ++n;
   lua_pushfstring(L, ";%s/?.lua", corepath); ++n;
   EINA_LIST_FREE(es->lincs, data)
     {
        lua_pushfstring(L, ";%s/?.lua", data);
        eina_stringshare_del(data);
        ++n;
     }
   lua_pushfstring(L, ";%s/?.eo.lua", modpath); ++n;
   lua_pushfstring(L, ";%s/?.lua", modpath); ++n;
   lua_pushfstring(L, ";%s/?.lua", appspath); ++n;
   lua_concat(L, n);

   /* apps path, local directory takes priority as well */
   lua_pushvalue(L, 4);
   lua_pushfstring(L, ";%s/?.lua", appspath);
   lua_concat(L, 2);

   return 2;
}

/**
 * @brief Performs the final setup for an Elua_State.
 *
 * This function sets up modules, internationalization, and I/O.
 * It then iterates through a list of pre-registered Lua modules (`es->lmods`)
 * and requires them. This is used for modules that need to be loaded
 * before the main script or application runs.
 *
 * @param es The Elua_State to set up.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool
elua_state_setup(Elua_State *es)
{
   Eina_Stringshare *data;
   Eina_Bool failed = EINA_FALSE;

   if (!_elua_state_modules_setup(es))
     return EINA_FALSE;
   if (!_elua_state_i18n_setup(es))
     return EINA_FALSE;
   if (!_elua_state_io_setup(es))
     return EINA_FALSE;

   /* finally require the necessary modules */
   EINA_LIST_FREE(es->lmods, data)
     {
        if (!failed)
          {
             if (!elua_state_require_ref_push(es))
               {
                  failed = EINA_TRUE;
                  break;
               }
             lua_pushstring(es->luastate, data);
             if (elua_util_error_report(es, lua_pcall(es->luastate, 1, 0, 0)))
               {
                  failed = EINA_TRUE;
                  break;
               }
          }
        eina_stringshare_del(data);
     }

   return EINA_TRUE;
}

/* Utility functions - these could be written using the other APIs */

/**
 * @brief Custom Lua traceback function.
 *
 * This function is pushed onto the Lua stack before calling a protected
 * function via `lua_pcall`. If an error occurs, Lua calls this function
 * to generate a traceback string. It uses `debug.traceback`.
 *
 * @param L The Lua state. The error object or message is at stack index 1.
 * @return 1, leaving the traceback string on the stack.
 */
static int
_elua_traceback(lua_State *L)
{
   lua_getglobal(L, "debug");
   if (!lua_istable(L, -1))
     {
        lua_pop(L, 1);
        return 1;
     }
   lua_getfield(L, -1, "traceback");
   if (!lua_isfunction(L, -1))
     {
        lua_pop(L, 2);
        return 1;
     }
   lua_pushvalue(L, 1);
   lua_pushinteger(L, 2);
   lua_call(L, 2, 1);
   return 1;
}

/**
 * @brief Calls a Lua function with error handling and traceback.
 *
 * This function wraps `lua_pcall`, using `_elua_traceback` as the
 * message handler to provide detailed error messages. It also performs
 * a garbage collection cycle if an error occurs.
 *
 * @param es The Elua_State.
 * @param narg Number of arguments.
 * @param nret Number of results.
 * @return The status code from `lua_pcall` (0 for success, non-zero for errors).
 *         Returns -1 if `es` is NULL.
 */
static int
_elua_docall(Elua_State *es, int narg, int nret)
{
   int status;
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, -1);
   int bs = lua_gettop(es->luastate) - narg;
   lua_pushcfunction(es->luastate, _elua_traceback);
   lua_insert(es->luastate, bs);
   status = lua_pcall(es->luastate, narg, nret, bs);
   lua_remove(es->luastate, bs);
   if (status)
      lua_gc(es->luastate, LUA_GCCOLLECT, 0);
   return status;
}

/**
 * @brief Prepares command-line arguments for a Lua script.
 *
 * This function takes C-style `argc` and `argv` and converts them into
 * a Lua table and individual arguments suitable for a Lua script.
 * The Lua script will receive these arguments as varargs, and a global
 * table named `arg` will also be populated.
 *
 * @param es The Elua_State.
 * @param argc The argument count from C main.
 * @param argv The argument vector from C main.
 * @param n The index in `argv` of the first script argument (or script name).
 * @return The number of arguments pushed onto the Lua stack for the script.
 *         Returns -1 if `es` is NULL.
 */
static int
_elua_getargs(Elua_State *es, int argc, char **argv, int n)
{
   int i;
   int narg = argc - (n + 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, -1);
   luaL_checkstack(es->luastate, narg + 3, "too many arguments to script");
   for (i = n + 1; i < argc; ++i)
     {
        lua_pushstring(es->luastate, argv[i]);
     }
   lua_createtable(es->luastate, narg, n + 1);
   for (i = 0; i < argc; ++i)
     {
        lua_pushstring(es->luastate, argv[i]);
        lua_rawseti(es->luastate, -2, i - n);
     }
   return narg;
}

/**
 * @brief Requires a Lua library within an Elua_State.
 *
 * This function uses the Elua 'require' mechanism. If the 'require'
 * function is not yet available (e.g., during early setup), the library
 * name is queued to be loaded later by `elua_state_setup`.
 *
 * @param es The Elua_State.
 * @param libname The name of the library to require (e.g., "foo.bar").
 * @return EINA_TRUE on success or if queued, EINA_FALSE on error.
 *         If queued, the return value is 0 (interpreted as EINA_FALSE by some checks,
 *         but it's not an immediate error).
 */
EAPI Eina_Bool
elua_util_require(Elua_State *es, const char *libname)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, EINA_FALSE);
   if (!elua_state_require_ref_push(es))
     {
        /* store stuff until things are correctly set up */
        es->lmods = eina_list_append(es->lmods, eina_stringshare_add(libname));
        return 0;
     }
   lua_pushstring(es->luastate, libname);
   return !elua_util_error_report(es, lua_pcall(es->luastate, 1, 0, 0));
}

/**
 * @brief Loads and runs a Lua script from a file.
 *
 * @param es The Elua_State.
 * @param fname The path to the Lua script file.
 * @return EINA_TRUE on success, EINA_FALSE on error. Errors are reported via `elua_util_error_report`.
 */
EAPI Eina_Bool
elua_util_file_run(Elua_State *es, const char *fname)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, EINA_FALSE);
   return !elua_util_error_report(es, elua_io_loadfile(es, fname)
                                  || _elua_docall(es, 0, 1));
}

/**
 * @brief Loads and runs a Lua script from a string.
 *
 * @param es The Elua_State.
 * @param chunk A string containing the Lua code.
 * @param chname A name for the chunk (used in error messages).
 * @return EINA_TRUE on success, EINA_FALSE on error. Errors are reported via `elua_util_error_report`.
 */
EAPI Eina_Bool
elua_util_string_run(Elua_State *es, const char *chunk, const char *chname)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, EINA_FALSE);
   return !elua_util_error_report(es, luaL_loadbuffer(es->luastate, chunk,
                                                      strlen(chunk), chname)
                                      || _elua_docall(es, 0, 0));
}

/**
 * @brief Loads an Elua application.
 *
 * This function uses the Elua 'appload' mechanism.
 * The 'appload' Lua function is expected to return two values:
 * 1. The loaded application (e.g., a function or table), or nil on error.
 * 2. An error message if the first return value is nil.
 *
 * @param es The Elua_State.
 * @param appname The name of the application to load.
 * @return 0 if the application was loaded successfully (first return from appload was not nil).
 *         1 if the application loading failed (first return from appload was nil, error message is on stack).
 *         -1 if `es` is NULL or appload reference is invalid.
 */
EAPI int
elua_util_app_load(Elua_State *es, const char *appname)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, -1);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(elua_state_appload_ref_push(es), -1);
   lua_pushstring(es->luastate, appname);
   lua_call(es->luastate, 1, 2);
   if (lua_isnil(es->luastate, -2))
     {
        lua_remove(es->luastate, -2);
        return 1;
     }
   lua_pop(es->luastate, 1);
   return 0;
}

/**
 * @brief Runs a Lua script or an Elua application, processing command-line arguments.
 *
 * This function determines if `argv[n]` is a file or an application name.
 * It loads the script/app, passes arguments to it, and executes it.
 * If the script/app returns a boolean value, it's stored in `*quit`.
 *
 * @param es The Elua_State.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param n Index in `argv` of the script/application name.
 * @param[out] quit Pointer to an integer where the script's boolean return value (if any) is stored.
 *                  Typically indicates if the program should quit.
 * @return EINA_TRUE on success, EINA_FALSE on error.
 *         Returns -1 (cast to Eina_Bool) if `n >= argc` or `es` is NULL.
 */
EAPI Eina_Bool
elua_util_script_run(Elua_State *es, int argc, char **argv, int n, int *quit)
{
   int status, narg;
   const char *fname;
   EINA_SAFETY_ON_FALSE_RETURN_VAL(n < argc, -1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(es, -1);
   fname = argv[n];
   narg = _elua_getargs(es, argc, argv, n);
   lua_setglobal(es->luastate, "arg");
   if (fname[0] == '-' && !fname[1]) fname = NULL;
   if (fname)
     {
        /* check if there is a file of that name */
        FILE *f = fopen(fname, "rb");
        if (f)
          {
             fclose(f);
             status = elua_io_loadfile(es, fname);
          }
        else
          status = elua_util_app_load(es, fname);
     }
   else
     status = elua_io_loadfile(es, fname);
   lua_insert(es->luastate, -(narg + 1));
   if (!status)
     status = _elua_docall(es, narg, 1);
   else
     lua_pop(es->luastate, narg);
   if (!status)
     {
        *quit = lua_toboolean(es->luastate, -1);
        lua_pop(es->luastate, 1);
     }
   return !elua_util_error_report(es, status);
}

/**
 * @brief Prints an error message to the Elua log.
 *
 * Prepends the program name if provided.
 *
 * @param pname The program name (can be NULL).
 * @param msg The error message.
 */
static void
_elua_errmsg(const char *pname, const char *msg)
{
   ERR("%s%s%s", pname ? pname : "", pname ? ": " : "", msg);
}

/**
 * @brief Reports a Lua error if one occurred.
 *
 * If `status` is non-zero (indicating an error) and there's an error
 * message on top of the Lua stack, this function logs the error message
 * using `_elua_errmsg` and pops the message from the stack.
 *
 * @param es The Elua_State.
 * @param status The status code from a Lua operation (e.g., `lua_pcall`, `luaL_loadfile`).
 *               0 means success, non-zero means error.
 * @return The original `status` value.
 */
EAPI int
elua_util_error_report(const Elua_State *es, int status)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL(es, status);
   if (status && !lua_isnil(es->luastate, -1))
     {
        const char *msg = lua_tostring(es->luastate, -1);
        _elua_errmsg(es->progname, msg ? msg : "(non-string error)");
        lua_pop(es->luastate, 1);
     }
   return status;
}
