#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "elua_private.h"

/* bytecode caching */

#if LUA_VERSION_NUM > 501
#  define elua_load(L, reader, data, chunkname) lua_load(L, reader, data, chunkname, NULL)
#else
#  define elua_load(L, reader, data, chunkname) lua_load(L, reader, data, chunkname)
#endif

/**
 * @brief Checks if a precompiled bytecode file is valid and up-to-date.
 *
 * If the original source file doesn't exist, the bytecode is considered valid.
 * If the bytecode file's modification time is more recent than the source file's,
 * it's considered valid. Otherwise, it's considered outdated.
 *
 * @param of Pointer to an Eina_File representing the opened bytecode file.
 *           If NULL, this function will attempt to open the source file.
 * @param fname The filename of the original Lua source file.
 * @param bc Pointer to an Eina_Bool that will be set to EINA_TRUE if
 *           bytecode regeneration is triggered, EINA_FALSE otherwise.
 * @return Eina_File* Pointer to the opened Eina_File (either the bytecode
 *         or the source file if regeneration is needed). Returns NULL on failure.
 */
static Eina_File *
check_bc(Eina_File *of, const char *fname, Eina_Bool *bc)
{
   if (of)
     {
        struct stat bc_stat, sc_stat;
        /* original file doesn't exist, only bytecode does, use bytecode */
        if (stat(fname, &sc_stat) < 0)
          return of;
        if (stat(eina_file_filename_get(of), &bc_stat) < 0)
          {
             /* what? */
             eina_file_close(of);
             goto generate;
          }
        /* bytecode is newer than original file, use bytecode */
        if (bc_stat.st_mtime > sc_stat.st_mtime)
          return of;
        /* bytecode is not new enough; trigger regeneration */
        eina_file_close(of);
     }
generate:
   *bc = EINA_TRUE;
   return eina_file_open(fname, EINA_FALSE);
}

/**
 * @brief Opens a Lua source file, potentially checking for a precompiled bytecode version.
 *
 * If `allow_bc` is true and the filename ends with ".lua", this function
 * will first look for a corresponding ".luac" file. If found, `check_bc`
 * is called to validate it. If the bytecode is not found or is invalid,
 * the original source file is opened.
 *
 * @param fname The filename of the Lua source file to open.
 * @param bc Pointer to an Eina_Bool that will be set by `check_bc` if
 *           bytecode checking occurs.
 * @param allow_bc If EINA_TRUE, allows checking for and using bytecode files.
 * @return Eina_File* Pointer to the opened Eina_File. Returns NULL on failure.
 */
static Eina_File *
open_src(const char *fname, Eina_Bool *bc, Eina_Bool allow_bc)
{
   Eina_File  *f   = NULL;
   const char *ext = strstr(fname, ".lua");
   if (ext && !ext[4] && allow_bc)
     {
        char buf[PATH_MAX];
        snprintf(buf, sizeof(buf), "%sc", fname);
        f = check_bc(eina_file_open(buf, EINA_FALSE), fname, bc);
     }
   if (!f) f = eina_file_open(fname, EINA_FALSE);
   return  f;
}

/**
 * @brief Lua writer function used by lua_dump.
 *
 * This function is called by `lua_dump` to write chunks of bytecode
 * to a file stream.
 *
 * @param L The Lua state (unused).
 * @param p Pointer to the data to write.
 * @param size The size of the data to write.
 * @param ud Userdata, expected to be a FILE* stream.
 * @return int 0 on success, non-zero on error (consistent with fwrite).
 */
static int
writef(lua_State *L EINA_UNUSED, const void *p, size_t size, void *ud)
{
   FILE *f = ud;
   return ferror(f) || (fwrite(p, 1, size, f) != size);
}

/**
 * @brief Opens a temporary file for writing bytecode.
 *
 * Creates a unique temporary filename based on `fname` with a ".XXXXXX.cache"
 * pattern, opens it in write-binary mode ("wb"), and returns the FILE stream.
 * The actual temporary filename used is written back to `buf`.
 *
 * @param fname The base filename for generating the temporary file name.
 * @param buf Buffer to store the actual temporary filename created.
 * @param buflen Size of the `buf` buffer.
 * @return FILE* Pointer to the opened temporary file stream, or NULL on failure.
 */
static FILE *
bc_tmp_open(const char *fname, char *buf, size_t buflen)
{
   Eina_Tmpstr *tmp_file;
   int fd;
   snprintf(buf, buflen, "%s.XXXXXX.cache", fname);
   fd = eina_file_mkstemp(buf, &tmp_file);
   if (fd < 0)
     return NULL;
   eina_strlcpy(buf, tmp_file, buflen);
   eina_tmpstr_del(tmp_file);
   return fdopen(fd, "wb");
}

/**
 * @brief Dumps the Lua bytecode from the top of the stack to a cached file.
 *
 * The bytecode is first written to a temporary file. If successful,
 * this temporary file is then renamed to `fname` with a "c" appended
 * (e.g., "script.lua" becomes "script.luac").
 *
 * @param L The Lua state, with the compiled chunk at the top of the stack.
 * @param fname The original filename of the Lua script.
 */
static void
write_bc(lua_State *L, const char *fname)
{
   FILE *f;
   char buf[PATH_MAX];
   if ((f = bc_tmp_open(fname, buf, sizeof(buf))))
     {
        char buf2[PATH_MAX];
        if (lua_dump(L, writef, f))
          {
             fclose(f);
             /* there really is nothing to handle here */
             (void)!!remove(buf);
             return;
          }
        else fclose(f);
        snprintf(buf2, sizeof(buf2), "%sc", fname);
        if (rename(buf, buf2))
          {
             /* a futile attempt at cleanup */
             (void)!!remove(buf);
             (void)!!remove(buf2);
          }
     }
}

/**
 * @brief Lua reader function for reading from stdin.
 *
 * This function is used by `lua_load` to read chunks of data from stdin.
 * It reads up to LUAL_BUFFERSIZE bytes into the provided buffer.
 *
 * @param L The Lua state (unused).
 * @param ud Userdata, expected to be a char** pointing to a buffer.
 * @param size Pointer to a size_t where the number of bytes read will be stored.
 * @return const char* Pointer to the buffer containing the read data,
 *         or NULL if EOF is reached or an error occurs.
 */
static const char *
getf(lua_State *L EINA_UNUSED, void *ud, size_t *size)
{
   char *buff = *((char**)ud);
   if (feof(stdin)) return NULL;
   *size = fread(buff, 1, LUAL_BUFFERSIZE, stdin);
   return (*size > 0) ? buff : NULL;
}

/**
 * @brief Loads Lua code from stdin.
 *
 * Uses the `getf` reader function to load code from standard input.
 *
 * @param L The Lua state.
 * @return int Lua status code (e.g., LUA_OK, LUA_ERRSYNTAX, LUA_ERRFILE).
 */
static int
elua_loadstdin(lua_State *L)
{
   char buff[LUAL_BUFFERSIZE];
   int status = elua_load(L, getf, &buff, "=stdin");
   if (ferror(stdin))
     {
        lua_pop(L, 1);
        lua_pushfstring(L, "cannot read stdin: %s", strerror(errno));
        return LUA_ERRFILE;
     }
   return status;
}

/**
 * @brief Structure to hold information for reading from a memory-mapped file.
 */
typedef struct Map_Stream
{
   char   *fmap; /**< Pointer to the memory-mapped file content. */
   size_t  flen; /**< Length of the memory-mapped file content. */
} Map_Stream;

/**
 * @brief Lua reader function for reading from a memory-mapped file.
 *
 * This function is used by `lua_load`. It provides the entire content
 * of the memory-mapped file in a single call.
 *
 * @param L The Lua state (unused).
 * @param ud Userdata, expected to be a Map_Stream* pointing to the mapped file info.
 * @param size Pointer to a size_t where the length of the data will be stored.
 * @return const char* Pointer to the memory-mapped data. `s->fmap` is set to NULL
 *         after the first call to signal Lua that all data has been read.
 */
static const char *
getf_map(lua_State *L EINA_UNUSED, void *ud, size_t *size)
{
   Map_Stream *s    = ud;
   const char *fmap = s->fmap;
   *size = s->flen;
   /* gotta null it - tell lua to terminate reading */
   s->fmap = NULL;
   return fmap;
}

/**
 * @brief Loads a Lua file, handling bytecode caching and memory mapping.
 *
 * This function attempts to load a Lua file. It supports:
 * - Loading from stdin if `fname` is NULL.
 * - Checking for and using precompiled bytecode (`.luac` files).
 * - Regenerating bytecode if the source is newer or bytecode is missing/corrupt.
 * - Memory-mapping the file for efficient reading.
 *
 * @param es Pointer to the Elua_State.
 * @param fname The filename of the Lua script to load. If NULL, reads from stdin.
 * @return int Lua status code (e.g., LUA_OK, LUA_ERRSYNTAX, LUA_ERRFILE).
 *         Returns -1 if `es` or `es->luastate` is NULL.
 */
EAPI int
elua_io_loadfile(const Elua_State *es, const char *fname)
{
   Map_Stream s;
   int status;
   Eina_File *f;
   const char *chname;
   Eina_Bool bcache = EINA_FALSE;
   lua_State *L;
   if (!es || !es->luastate) return -1;
   L = es->luastate;
   if (!fname)
     {
        return elua_loadstdin(L);
     }
   if (!(f = open_src(fname, &bcache, EINA_TRUE)))
     {
        lua_pushfstring(L, "cannot open %s: %s", fname, strerror(errno));
        return LUA_ERRFILE;
     }
   chname = lua_pushfstring(L, "@%s", fname);
   s.flen = eina_file_size_get(f);
   if (!(s.fmap = eina_file_map_all(f, EINA_FILE_RANDOM)))
     {
        lua_pushfstring(L, "cannot read %s: %s", chname + 1, strerror(errno));
        lua_remove(L, -2);
        return LUA_ERRFILE;
     }
   status = elua_load(L, getf_map, &s, chname);
   eina_file_map_free(f, s.fmap);
   eina_file_close(f);
   if (status)
     {
        /* we loaded bytecode and that failed; try loading source instead */
        if (!bcache)
          {
             /* can't open real file, so return original error */
             if (!(f = open_src(fname, &bcache, EINA_FALSE)))
               {
                  lua_remove(L, -2);
                  return status;
               }
             s.flen = eina_file_size_get(f);
             /* can't read real file, so return original error */
             if (!(s.fmap = eina_file_map_all(f, EINA_FILE_RANDOM)))
               {
                  lua_remove(L, -2);
                  return status;
               }
             /* loaded original file, pop old error and load again */
             lua_pop(L, 1);
             status = elua_load(L, getf_map, &s, chname);
             eina_file_map_free(f, s.fmap);
             eina_file_close(f);
             /* force write new bytecode */
             if (!status)
               write_bc(L, fname);
          }
        /* whatever happened here, proceed to the end... */
     }
   else if (bcache)
     write_bc(L, fname); /* success and bytecode write */
   lua_remove(L, -2);
   return status;
}

/* lua function */

/**
 * @brief Lua-accessible `loadfile` function.
 *
 * Implements the `loadfile` function available within Lua scripts.
 * It wraps `elua_io_loadfile` and handles setting the environment
 * of the loaded chunk if provided.
 *
 * Lua signature: `loadfile ([filename [, mode [, env]]])`
 * This implementation primarily uses `filename` and `env`.
 *
 * @param L The Lua state.
 *   - Stack index 1: Optional filename (string). If nil or absent, loads from stdin.
 *   - Stack index 2: Optional mode (string, largely ignored by this C implementation).
 *   - Stack index 3: Optional environment table for the loaded chunk.
 * @return int Number of return values on the Lua stack.
 *   - On success: Returns 1 (the loaded chunk).
 *   - On error: Returns 2 (nil, error message).
 */
static int
loadfile(lua_State *L)
{
   Elua_State *es = elua_state_from_lua_state_get(L);
   const char *fname = luaL_optstring(L, 1, NULL);
   int status = elua_io_loadfile(es, fname),
       hasenv = (lua_gettop(L) >= 3);
   if (!status)
     {
        if (hasenv)
          {
             lua_pushvalue(L, 3);
#if LUA_VERSION_NUM < 502
             lua_setfenv(L, -2);
#else
             if (!lua_setupvalue(L, -2, 1))
               lua_pop(L, 1);
#endif
          }
        return 1;
     }
   lua_pushnil(L);
   lua_insert(L, -2);
   return 2;
}

/**
 * @brief Sets up the `loadfile` function in the global Lua environment.
 *
 * @param es Pointer to the Elua_State.
 * @return Eina_Bool EINA_TRUE on success, EINA_FALSE on failure (e.g., if `es` is invalid).
 */
Eina_Bool
_elua_state_io_setup(const Elua_State *es)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL(es && es->luastate, EINA_FALSE);
   lua_pushcfunction(es->luastate, loadfile);
   lua_setglobal(es->luastate, "loadfile");
   return EINA_TRUE;
}
