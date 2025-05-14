#ifdef _WIN32
# include <evil_private.h> /* realpath */
#endif

#include "elua_private.h"

/**
 * @brief Expands a filename to its full path and constructs a command line string.
 *
 * This function takes a filename and an array of arguments, resolves the
 * filename to an absolute path, and then constructs a command line string
 * suitable for functions like popen. Arguments are quoted and escaped.
 * It also verifies if the file exists and is readable.
 *
 * @param fname The filename to resolve and use as the command.
 * @param argv Null-terminated array of strings representing the arguments to the command.
 *             The last element of argv must be NULL if it's not empty.
 * @return A newly allocated string containing the command line, or NULL on failure
 *         (e.g., file not found, realpath fails, memory allocation fails).
 *         The caller is responsible for freeing the returned string.
 */
/* expand fname to full path name (so that PATH is ignored) plus turn
 * stuff into a command, and also verify whether the path exists */
static char *
get_cmdline_from_argv(const char *fname, const char **argv)
{
   Eina_Strbuf *buf;
   char        *ret;
   char         pbuf[PATH_MAX];
   const char  *arg = NULL;

   FILE *testf = fopen(fname, "rb");
   if  (!testf)
      return NULL;

   fclose(testf);

   /* for windows, we have realpath in evil, no need for GetFullPathName */
   if (!realpath(fname, pbuf))
      return NULL;

   buf = eina_strbuf_new();
   eina_strbuf_append_char(buf, '"');
   eina_strbuf_append(buf, pbuf);
   eina_strbuf_append_char(buf, '"');

   while ((arg = *(argv++)))
     {
        char c;
        eina_strbuf_append_char(buf, ' ');
        eina_strbuf_append_char(buf, '"');

        while ((c = *(arg++)))
          {
#ifndef _WIN32
             if (c == '"' || c == '$') eina_strbuf_append_char(buf, '\\');
             eina_strbuf_append_char(buf, c);
#else
             if      (c == '"') eina_strbuf_append_char(buf, '\\');
             else if (c == '%') eina_strbuf_append_char(buf,  '"');
             eina_strbuf_append_char(buf, c);
             if (c == '%') eina_strbuf_append_char(buf,  '"');
#endif
          }

        eina_strbuf_append_char(buf, '"');
     }

   ret = strdup(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief A wrapper around popen/_popen that uses a pre-constructed command line.
 *
 * This function takes a path, mode, and arguments, constructs the command line
 * using get_cmdline_from_argv, and then calls the system's popen implementation.
 *
 * @param path The path to the executable.
 * @param md The mode string for popen (e.g., "r", "w").
 * @param argv A NULL-terminated array of argument strings for the command.
 * @return A FILE pointer from popen, or NULL on failure.
 */
static FILE *
elua_popen_c(const char *path, const char *md, const char *argv[])
{
   FILE *ret;

   char *cmdline = get_cmdline_from_argv(path, argv);
   if  (!cmdline) return NULL;

#ifndef _WIN32
   ret = popen(cmdline, md);
#else
   ret = _popen(cmdline, md);
#endif

   free(cmdline);

   if (!ret) return NULL;

   return ret;
}

/**
 * @brief Pushes a boolean result and optional error information onto the Lua stack.
 *
 * If 'i' is true (non-zero), pushes true.
 * If 'i' is false (zero), pushes nil, an error message (constructed from fname
 * and strerror(errno) or just strerror(errno) if fname is NULL), and the errno value.
 *
 * @param L The Lua state.
 * @param i The integer condition (0 for failure, non-zero for success).
 * @param fname Optional filename to include in the error message. Can be NULL.
 * @return The number of values pushed onto the Lua stack (1 for success, 3 for failure).
 */
static int
push_ret(lua_State *L, int i, const char *fname)
{
   int en = errno;
   if (i)
     {
        lua_pushboolean(L, 1);
        return 1;
     }
   else
     {
        lua_pushnil(L);
        if (fname)
           lua_pushfstring(L, "%s: %s", fname, strerror(en));
        else
           lua_pushfstring(L, "%s", strerror(en));
        lua_pushinteger(L, en);
        return 3;
     }
}

/**
 * @brief Retrieves a FILE* from a Lua userdata object.
 *
 * Checks if the userdata at index 1 is a "ELUA_FILE*" and if the file is open.
 * Raises a Lua error if the file is closed or the userdata is invalid.
 *
 * @param L The Lua state.
 * @return The FILE* pointer.
 */
static FILE *
tofile(lua_State *L)
{
   FILE **f = (FILE**)luaL_checkudata(L, 1, "ELUA_FILE*");
   if (!*f)
     {
        luaL_error(L, "attempt to use a closed file");
     }
   return *f;
}

/**
 * @brief Lua binding for closing a file.
 *
 * Expects a "ELUA_FILE*" userdata as the first argument.
 * Closes the file and sets the internal FILE* to NULL on success.
 * Pushes boolean success and error information using push_ret.
 *
 * @param L The Lua state.
 * @return Number of results pushed by push_ret.
 */
static int
elua_close(lua_State *L)
{
   FILE **f = (FILE**)luaL_checkudata(L, 1, "ELUA_FILE*");
   int ok = (fclose(*f) == 0);
   if (ok) *f = NULL;
   return push_ret(L, ok, NULL);
}

/**
 * @brief Lua binding for flushing a file's output buffer.
 *
 * Expects a "ELUA_FILE*" userdata as the first argument.
 * Calls fflush on the file.
 * Pushes boolean success and error information using push_ret.
 *
 * @param L The Lua state.
 * @return Number of results pushed by push_ret.
 */
static int
elua_flush(lua_State *L)
{
   return push_ret(L, fflush(tofile(L)) == 0, NULL);
}

static int elua_readline(lua_State *L);

/**
 * @brief Lua binding for creating a line iterator for a file.
 *
 * Expects a "ELUA_FILE*" userdata as the first argument.
 * Returns an iterator function (elua_readline) that can be used in a for loop.
 * The file object itself is passed as an upvalue to the iterator.
 *
 * @param L The Lua state.
 * @return 1 (the iterator function is pushed onto the stack).
 */
static int
elua_lines(lua_State *L)
{
   lua_pushvalue(L, 1);
   lua_pushcclosure(L, elua_readline, 1);
   return 1;
}

/**
 * @brief Reads a number from a file.
 *
 * Attempts to scan a Lua number from the given file stream.
 * If successful, pushes the number onto the Lua stack.
 *
 * @param L The Lua state.
 * @param f The FILE stream to read from.
 * @return 1 if a number was successfully read and pushed, 0 otherwise.
 */
static int
read_number(lua_State *L, FILE *f)
{
   lua_Number d;
   if (fscanf(f, LUA_NUMBER_SCAN, &d) == 1)
     {
        lua_pushnumber(L, d);
        return 1;
     }
   return 0;
}

/**
 * @brief Tests for End-Of-File (EOF) on a file stream.
 *
 * Reads a character and then ungets it to check for EOF without consuming input.
 * Pushes an empty string onto the Lua stack (as per standard Lua io.read behavior
 * when reading 0 characters).
 *
 * @param L The Lua state.
 * @param f The FILE stream to test.
 * @return 1 if not EOF, 0 if EOF.
 */
static int
test_eof(lua_State *L, FILE *f)
{
   int c = getc(f);
   ungetc(c, f);
   lua_pushlstring(L, NULL, 0);
   return (c != EOF);
}

/**
 * @brief Reads a line from a file stream.
 *
 * Reads characters into a Lua buffer until a newline or EOF is encountered.
 * The newline character itself is not included in the resulting string.
 * Pushes the read line (or partial line at EOF) onto the Lua stack.
 *
 * @param L The Lua state.
 * @param f The FILE stream to read from.
 * @return 1 if a line (or part of it) was read and pushed, 0 if EOF was
 *         reached immediately and nothing was read.
 */
static int
read_line(lua_State *L, FILE *f)
{
   luaL_Buffer b;
   luaL_buffinit(L, &b);
   for (;;)
     {
        size_t l;
        char *p = luaL_prepbuffer(&b);
        if (fgets(p, LUAL_BUFFERSIZE, f) == NULL)
          {
             luaL_pushresult(&b);
             return (elua_strlen(L, -1) > 0);
          }
        l = strlen(p);
        if (!l || p[l - 1] != '\n')
           luaL_addsize(&b, l);
        else
          {
             luaL_addsize(&b, l - 1);
             luaL_pushresult(&b);
             return 1;
          }
     }
}

/**
 * @brief Reads a specified number of characters from a file stream.
 *
 * Reads up to 'n' characters from the file stream into a Lua buffer.
 * Pushes the string of read characters onto the Lua stack.
 *
 * @param L The Lua state.
 * @param f The FILE stream to read from.
 * @param n The maximum number of characters to read.
 * @return 1 if 'n' characters were read or if some characters were read before EOF.
 *         Returns 1 even if fewer than 'n' characters were read due to EOF,
 *         as long as elua_strlen(L, -1) > 0.
 *         Returns 0 if n > 0 and EOF is reached immediately.
 */
static int
read_chars(lua_State *L, FILE *f, size_t n)
{
   size_t rlen;
   size_t nr;
   luaL_Buffer b;
   luaL_buffinit(L, &b);
   rlen = LUAL_BUFFERSIZE;
   do
     {
        char *p = luaL_prepbuffer(&b);
        if (rlen > n) rlen = n;
        nr = fread(p, sizeof(char), rlen, f);
        luaL_addsize(&b, nr);
        n -= nr;
     } while (n > 0 && nr == rlen);
   luaL_pushresult(&b);
   return (n == 0 || elua_strlen(L, -1) > 0);
}

/**
 * @brief The iterator function for reading lines from a file.
 *
 * This function is used by `elua_lines`. It retrieves the FILE* from its upvalue.
 * Calls read_line to read the next line from the file.
 * If successful, the line is on top of the stack.
 * If an error occurs during reading (ferror), it raises a Lua error.
 *
 * @param L The Lua state. The FILE* userdata is an upvalue.
 * @return 1 if a line is successfully read and pushed, otherwise raises a Lua error
 *         or handles EOF by returning 0 from read_line (which means Lua stops iterating).
 */
static int
elua_readline(lua_State *L)
{
   FILE *f = *(FILE**)lua_touserdata(L, lua_upvalueindex(1));
   int success;
   if (!f)
     {
        luaL_error(L, "file is already closed");
        return 0; /* shut up coverity; luaL_error does a longjmp */
     }
   success = read_line(L, f);
   if (ferror(f))
      return luaL_error(L, "%s", strerror(errno));
   return success;
}

/**
 * @brief Lua binding for reading from a file with various formats.
 *
 * Implements behavior similar to Lua's `file:read(...)`.
 * Expects a "ELUA_FILE*" userdata as the first argument.
 * Subsequent arguments specify what to read:
 * - No arguments: reads the next line (equivalent to "*l").
 * - A number `n`: reads up to `n` characters. If `n` is 0, tests for EOF.
 * - A string:
 *   - "*n": reads a number.
 *   - "*l": reads the next line.
 *   - "*a": reads the whole file from the current position.
 *
 * @param L The Lua state.
 * @return The number of items successfully read and pushed onto the stack.
 *         On read failure or EOF for a specific format, pushes nil for that item.
 *         If ferror is set on the file, returns error information via push_ret.
 */
static int
elua_read(lua_State *L)
{
   FILE *f   = tofile(L);
   int nargs = lua_gettop(L) - 1;
   int first = 2;
   int success, n;
   clearerr(f);
   if (!nargs)
     {
        success = read_line(L, f);
        n = first + 1;
     }
   else
     {
        luaL_checkstack(L, nargs + LUA_MINSTACK, "too many arguments");
        success = 1;
        for (n = first; nargs-- && success; ++n)
          {
             if (lua_type(L, n) == LUA_TNUMBER)
               {
                  size_t l = (size_t)lua_tointeger(L, n);
                  success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
               }
             else
               {
                  const char *p = lua_tostring(L, n);
                  luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
                  switch (p[1])
                    {
                       case 'n':
                          success = read_number(L, f);
                          break;
                       case 'l':
                          success = read_line(L, f);
                          break;
                       case 'a':
                          read_chars(L, f, ~((size_t)0));
                          success = 1;
                          break;
                       default:
                          return luaL_argerror(L, n, "invalid format");
                    }
               }
          }
     }
   if (ferror(f))
      return push_ret(L, 0, NULL);
   if (!success)
     {
        lua_pop(L, 1);
        lua_pushnil(L);
     }
   return n - first;
}

/**
 * @brief Lua binding for writing to a file.
 *
 * Implements behavior similar to Lua's `file:write(...)`.
 * Expects a "ELUA_FILE*" userdata as the first argument.
 * Subsequent arguments are values to be written to the file.
 * Numbers are formatted using LUA_NUMBER_FMT. Strings are written as is.
 *
 * @param L The Lua state.
 * @return Returns results via push_ret: on success, the file object itself (though
 *         standard Lua returns true); on failure, nil, error message, and errno.
 *         (Note: The current push_ret for success pushes boolean true, not the file object).
 */
static int
elua_write(lua_State *L)
{
   FILE *f    = tofile(L);
   int nargs  = lua_gettop(L) - 1;
   int status = 1, arg = 2;
   for (; nargs--; ++arg)
     {
        if (lua_type(L, arg) == LUA_TNUMBER)
           status = status && (fprintf(f, LUA_NUMBER_FMT,
                                       lua_tonumber(L, arg)) > 0);
        else
          {
             size_t l;
             const char *s = luaL_checklstring(L, arg, &l);
             status = status && (fwrite(s, sizeof(char), l, f) == l);
          }
     }
   return push_ret(L, status, NULL);
}

/**
 * @brief Lua garbage collection metamethod (__gc) for file objects.
 *
 * Ensures that the file is closed when the Lua userdata object is garbage collected.
 * Expects a "ELUA_FILE*" userdata as the first argument.
 *
 * @param L The Lua state.
 * @return 0.
 */
static int
elua_fgc(lua_State *L)
{
   FILE **f = (FILE**)luaL_checkudata(L, 1, "ELUA_FILE*");
   if (*f)
     {
        fclose(*f);
        *f = NULL;
     }
   return 0;
}

/**
 * @brief Lua __tostring metamethod for file objects.
 *
 * Provides a string representation for "ELUA_FILE*" userdata.
 * Shows "file (closed)" if the file is closed, otherwise "file (pointer_value)".
 *
 * @param L The Lua state.
 * @return 1 (the string representation is pushed onto the stack).
 */
static int
elua_ftostring(lua_State *L)
{
   FILE *f = *((FILE**)luaL_checkudata(L, 1, "ELUA_FILE*"));
   if  (!f)
      lua_pushliteral(L, "file (closed)");
   else
      lua_pushfstring(L, "file (%p)", f);
   return 1;
}

/**
 * @brief Lua library definition for file operations on popen'd streams.
 *
 * This table lists the functions that will be available on file objects
 * created by `_elua_io_popen`.
 */
static const luaL_Reg elua_popenlib[] =
{
   { "close"     , elua_close     },
   { "flush"     , elua_flush     },
   { "lines"     , elua_lines     },
   { "read"      , elua_read      },
   { "write"     , elua_write     },
   { "__gc"      , elua_fgc       },
   { "__tostring", elua_ftostring },
   { NULL        , NULL           }
};

/**
 * @brief Creates a new Lua userdata to hold a FILE*.
 *
 * Allocates userdata, initializes the FILE* to NULL, and sets its metatable
 * to "ELUA_FILE*". If the metatable doesn't exist, it's created and
 * populated with the functions from `elua_popenlib`.
 *
 * @param L The Lua state.
 * @return A pointer to the FILE* within the userdata.
 */
static FILE **
elua_newfile(lua_State *L)
{
   FILE **f = (FILE**)lua_newuserdata(L, sizeof(FILE*));
   *f = NULL;
   if (luaL_newmetatable(L, "ELUA_FILE*"))
     {
        lua_pushvalue(L, -1);
        lua_setfield (L, -2, "__index");
        elua_register(L, elua_popenlib);
     }
   lua_setmetatable(L, -2);
   return f;
}

/**
 * @brief Lua binding for popen.
 *
 * Opens a process by creating a pipe, forking, and invoking the shell.
 *
 * Lua usage: `local f, err, errno = elua.io.popen(prog, mode, arg1, arg2, ...)`
 *
 * @param L The Lua state.
 *   - Stack index 1: program name (string).
 *   - Stack index 2: mode (string, optional, defaults to "r").
 *   - Stack index 3...: arguments to the program (strings, optional).
 * @return 1 if successful (pushes the file userdata).
 *         3 if an error occurs (pushes nil, error message, errno).
 */
int
_elua_io_popen(lua_State *L)
{
   const char *fname = luaL_checkstring(L, 1);
   const char *mode  = luaL_optstring(L, 2, "r");
   int nargs = lua_gettop(L) - 2;
   FILE **pf = elua_newfile(L);
   if (nargs > 0)
     {
        const char **argv = (const char**)alloca((nargs + 1) * sizeof(char*));
        memset(argv, 0, (nargs + 1) * sizeof(char*));
        for (; nargs; --nargs)
          {
             argv[nargs - 1] = lua_tostring(L, nargs + 2);
          }
        *pf = elua_popen_c(fname, mode, argv);
     }
   else
     {
        const char *argv = NULL;
        *pf = elua_popen_c(fname, mode, &argv);
     }
   return (!*pf) ? push_ret(L, 0, fname) : 1;
}
