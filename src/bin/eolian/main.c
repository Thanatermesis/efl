/**
 * @file
 * @brief Main implementation file for the Eolian C generator.
 * This file contains the main logic for parsing command-line arguments,
 * processing Eolian files, and generating C header and source files.
 */
#include <stdlib.h>
#include <unistd.h>

#include "main.h"
#include "types.h"
#include "headers.h"
#include "sources.h"

int _eolian_gen_log_dom = -1; /**< Global log domain for Eolian generator. */
char* _eolian_api_symbol; /**< String used for API export/import symbols (e.g., "EAPI"). */

/**
 * @brief Flags representing different types of files to generate.
 */
enum
{
   GEN_H        = 1 << 0,
   GEN_H_STUB   = 1 << 1,
   GEN_C        = 1 << 2,
   GEN_C_IMPL   = 1 << 3,
   GEN_D        = 1 << 4,
   GEN_D_FULL   = 1 << 5 /**< Generate full dependencies including C files. */
};

/**
 * @brief Array of default file extensions corresponding to generation flags.
 * The order must match the bit positions in the generation flags enum.
 * For example, _dexts[0] corresponds to GEN_H, _dexts[1] to GEN_H_STUB, etc.
 * - _dexts[0]: ".h" (for GEN_H)
 * - _dexts[1]: ".stub.h" (for GEN_H_STUB)
 * - _dexts[2]: ".c" (for GEN_C)
 * - _dexts[3]: ".c" (for GEN_C_IMPL)
 * - _dexts[4]: ".d" (for GEN_D)
 * - _dexts[5]: ".d" (for GEN_D_FULL)
 */
static const char *_dexts[6] =
{
  ".h", ".stub.h", ".c", ".c", ".d", ".d"
};

/**
 * @brief Calculates the bit position of the least significant bit set in a flag.
 * For example, if flag is GEN_C (1 << 2), it returns 2.
 * @param flag The integer flag.
 * @return The bit position (0-indexed).
 */
static int
_get_bit_pos(int flag)
{
   int pos = 0;
   for (; !(flag & 1); flag >>= 1)
     ++pos;
   return pos;
}

/**
 * @brief Prints the usage message for the Eolian generator.
 * @param progn The program name (argv[0]).
 * @param outf The output file stream (e.g., stdout or stderr).
 */
static void
_print_usage(const char *progn, FILE *outf)
{
   fprintf(outf, "Usage: %s [options] [input]\n", progn);
   fprintf(outf, "Options:\n"
                 "  -I inc        include path \"inc\"\n"
                 "  -S            do not scan system dir for eo files\n"
                 "  -g type       generate file of type \"type\"\n"
                 "  -o name       specify the base name for output\n"
                 "  -o type:name  specify a particular output filename\n"
                 "  -h            print this message and exit\n"
                 "  -v            print version and exit\n"
                 "  -e            api symbol string to be used for import/export symbol"
                 "\n"
                 "Available types:\n"
                 "  h: C header file (.eo.h/.eot.h)\n"
                 "  s: Stub C header file (.eo.stub.h/.eot.stub.h)\n"
                 "  c: C source file (.eo.c)\n"
                 "  i: Implementation file (.c, merged with existing)\n"
                 "  d: Make-style dependencies, only for headers (.d)\n"
                 "  D: Like 'd' but for all generated files (.d)\n"
                 "\n"
                 "By default, the 'hc' set is used ('h' for .eot files).\n\n"
                 "The system-wide Eolian directory is scanned for eo files\n"
                 "by default, together with all specified '-I' flags.\n\n"
                 "Output filenames are determined from input .eo filename.\n"
                 "Default output path is where the input file is.\n\n"
                 "Also, specifying a type-dependent input file automatically\n"
                 "adds it to generated files, so if you specify those, you\n"
                 "don't need to explicitly specify -g for those types anymore.\n\n"
                 "Explicit output base name is without extension. The extension\n"
                 "is determined from the input file name. If that is not possible\n"
                 "for some reason, it defaults to \".eo\". Obviously, this does not\n"
                 "affect specific filenames (-o x:y) as these are full names.\n"
                 "Implementation files are a special case (no \".eo\" added).\n");
}

/**
 * @brief Prints the version of the Eolian C generator.
 * @param outf The output file stream (e.g., stdout).
 */
static void
_print_version(FILE *outf)
{
   fprintf(outf, "Eolian C generator version: " PACKAGE_VERSION "\n");
}

/**
 * @brief Attempts to set an output filename for a specific generation type.
 *
 * This function is used when parsing the -o type:name command-line option.
 * It updates the `outs` array with the specified filename `val` for the
 * generation type `t`, and sets the corresponding bit in `what`.
 *
 * @param t The character representing the generation type (e.g., 'h', 'c').
 * @param outs Array of output filename strings. The index corresponds to the
 *             bit position of the generation type.
 * @param val The filename to set.
 * @param[in,out] what Pointer to an integer holding the bitmask of generation types.
 *                     The bit corresponding to type `t` will be set.
 * @return EINA_TRUE if the type `t` is valid and the output filename was set,
 *         EINA_FALSE otherwise.
 */
static Eina_Bool
_try_set_out(char t, char **outs, const char *val, int *what)
{
   int pos = -1;
   switch (t)
     {
      case 'h':
        pos = _get_bit_pos(GEN_H);
        *what |= GEN_H;
        break;
      case 's':
        pos = _get_bit_pos(GEN_H_STUB);
        *what |= GEN_H_STUB;
        break;
      case 'c':
        pos = _get_bit_pos(GEN_C);
        *what |= GEN_C;
        break;
      case 'i':
        pos = _get_bit_pos(GEN_C_IMPL);
        *what |= GEN_C_IMPL;
        break;
      case 'd':
        pos = _get_bit_pos(GEN_D);
        *what |= GEN_D;
        break;
      case 'D':
        pos = _get_bit_pos(GEN_D_FULL);
        *what |= GEN_D_FULL;
        break;
     }
   if (pos < 0)
     return EINA_FALSE;
   if (outs[pos])
     free(outs[pos]);
   outs[pos] = strdup(val);
   return EINA_TRUE;
}

/**
 * @brief Fills any unspecified output filenames based on the input filename and base name.
 *
 * If an output filename for a particular generation type hasn't been explicitly
 * set (e.g., via -o type:name), this function generates a default name.
 * The default name is constructed from:
 * - `base`: If provided, this is used as the base of the filename.
 * - `val`: If `base` is NULL, the base is derived from `val` (the input filename)
 *          by stripping its extension.
 * - The original extension of `val` (or ".eo" if `val` has no extension).
 * - The specific extension for the generation type (from `_dexts`).
 *
 * For example, if input is "foo.eo" and GEN_H is requested:
 * - If `base` is "bar", output for GEN_H becomes "bar.eo.h".
 * - If `base` is NULL, output for GEN_H becomes "foo.eo.h".
 *
 * The GEN_C_IMPL type is special: it doesn't append the original extension.
 * So, for "foo.eo" and GEN_C_IMPL:
 * - If `base` is "bar", output becomes "bar.c".
 * - If `base` is NULL, output becomes "foo.c".
 *
 * @param outs Array of output filename strings. This array is modified in place.
 * @param val The input Eolian filename (e.g., "path/to/file.eo").
 * @param base The base name for output files, specified by -o name (can be NULL).
 */
static void _fill_all_outs(char **outs, const char *val, char *base)
{
   const char *ext = strrchr(val, '.');
   if (!ext)
     ext = ".eo";

   char *basen = base;
   if (!basen)
     {
        basen = strdup(val);
        char *p = strrchr(basen, '.');
        if (p) *p = '\0';
     }

   size_t blen = strlen(basen),
          elen = strlen(ext);

   for (size_t i = 0; i < (sizeof(_dexts) / sizeof(char *)); ++i)
     {
        if (outs[i])
          continue;
        size_t dlen = strlen(_dexts[i]);
        char *str = malloc(blen + elen + dlen + 1);
        char *p = str;
        memcpy(p, basen, blen);
        p += blen;
        if ((1 << i) != GEN_C_IMPL)
          {
             memcpy(p, ext, elen);
             p += elen;
          }
        memcpy(p, _dexts[i], dlen);
        p[dlen] = '\0';
        outs[i] = str;
     }

   if (!base)
     free(basen);
}

/**
 * @brief Wraps the content of a string buffer with include guards.
 *
 * The include guard is generated based on `fname` and an optional `gname`
 * (guard name suffix). Dots in `fname` are replaced with underscores, and
 * the whole name is uppercased.
 * Example: fname="my.header.h", gname="TYPES" -> _MY_HEADER_H_TYPES_
 *
 * @param fname The base filename for the guard (e.g., "my_header.eo.h").
 * @param gname An optional suffix for the guard name (e.g., "TYPES", "STUBS"). Can be NULL or empty.
 * @param buf The string buffer containing the content to be wrapped. This buffer
 *            is freed by the function, and a new buffer with the guards is returned.
 * @return A new Eina_Strbuf containing the original content wrapped in include guards,
 *         or NULL if the input `buf` was NULL. The caller is responsible for freeing
 *         the returned buffer.
 */
static Eina_Strbuf *
_include_guard(const char *fname, const char *gname, Eina_Strbuf *buf)
{
   if (!buf)
     return NULL;

   if (!gname)
     gname = "";

   char iname[256] = {0};
   strncpy(iname, fname, sizeof(iname) - 1);
   char *inamep = iname;
   eina_str_toupper(&inamep);

   Eina_Strbuf *g = eina_strbuf_new();
   eina_strbuf_append_printf(g, "#ifndef _%s_%s\n", iname, gname);
   eina_strbuf_append_printf(g, "#define _%s_%s\n\n", iname, gname);

   eina_strbuf_replace_all(g, ".", "_");
   eina_strbuf_append(g, eina_strbuf_string_get(buf));
   eina_strbuf_append(g, "\n#endif\n");
   eina_strbuf_free(buf);
   return g;
}

/**
 * @brief Extracts the filename from a full path.
 * Handles both '/' and '\' as directory separators.
 * @param path The full path string (e.g., "/usr/local/file.ext" or "C:\Users\file.ext").
 * @return A pointer to the filename part of the path, or the original path if
 *         no directory separators are found. Returns NULL if `path` is NULL.
 *         The returned pointer is part of the input `path` string, not a new allocation.
 */
static const char *
_get_filename(const char *path)
{
   if (!path)
     return NULL;
   const char *ret1 = strrchr(path, '/');
   const char *ret2 = strrchr(path, '\\');
   if (!ret1 && !ret2)
     return path;
   if (ret1 && ret2)
     {
        if (ret1 > ret2)
          return ret1 + 1;
        else
          return ret2 + 1;
     }
   if (ret1)
     return ret1 + 1;
   return ret2 + 1;
}

/**
 * @brief Writes the content of a string buffer to a file.
 * @param fname The name of the file to write.
 * @param buf The string buffer containing the data to write.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., cannot open file,
 *         write error).
 */
static Eina_Bool
_write_file(const char *fname, const Eina_Strbuf *buf)
{
   FILE *f = fopen(fname, "wb");
   if (!f)
     {
        fprintf(stderr, "eolian: could not open '%s' (%s)\n",
                fname, strerror(errno));
        return EINA_FALSE;
     }

   Eina_Bool fret = EINA_TRUE;

   size_t bl = eina_strbuf_length_get(buf);
   if (!bl)
     goto end;

   if (fwrite(eina_strbuf_string_get(buf), 1, bl, f) != bl)
     {
        fprintf(stderr, "eolian: could not write '%s' (%s)\n",
                fname, strerror(errno));
        fret = EINA_FALSE;
     }

end:
   fclose(f);
   return fret;
}

/**
 * @brief Reads the entire content of a file into a new string buffer.
 * If the file does not exist, an empty string buffer is created and EINA_TRUE is returned.
 * @param fname The name of the file to read.
 * @param[out] buf Pointer to an Eina_Strbuf pointer. On success, this will point
 *                 to a newly allocated string buffer containing the file content.
 *                 The caller is responsible for freeing this buffer.
 * @return EINA_TRUE on success (including file not found), EINA_FALSE on
 *         read errors or memory allocation failure.
 */
static Eina_Bool
_read_file(const char *fname, Eina_Strbuf **buf)
{
   FILE *f = fopen(fname, "rb");
   if (!f)
     {
        *buf = eina_strbuf_new();
        return EINA_TRUE;
     }

   fseek(f, 0, SEEK_END);
   long fs = ftell(f);
   if (fs < 0)
     {
        fprintf(stderr, "eolian: could not get length of '%s'\n", fname);
        fclose(f);
        return EINA_FALSE;
     }
   fseek(f, 0, SEEK_SET);

   char *cont = malloc(fs + 1);
   if (!cont)
     {
        fprintf(stderr, "eolian: could not allocate memory for '%s'\n", fname);
        fclose(f);
        return EINA_FALSE;
     }

   long as = fread(cont, 1, fs, f);
   if (as != fs)
     {
        fprintf(stderr, "eolian: could not read %ld bytes from '%s' (got %ld)\n",
                fs, fname, as);
        free(cont);
        fclose(f);
        return EINA_FALSE;
     }

   cont[fs] = '\0';
   fclose(f);
   *buf = eina_strbuf_manage_new_length(cont, fs);
   return EINA_TRUE;
}

/**
 * @brief Converts an Eolian name (e.g., "My.Object.Name") to a C-style full name
 *        (e.g., "my_object_name") by replacing dots with underscores.
 * @param nm The Eolian name string.
 * @return A newly allocated string with the C-style name, or NULL if `nm` is NULL.
 *         Aborts on memory allocation failure. The caller is responsible for
 *         freeing the returned string.
 */
char *eo_gen_c_full_name_get(const char *nm)
{
   if (!nm)
     return NULL;
   char *buf = strdup(nm);
   if (!buf)
     abort();
   for (char *p = strchr(buf, '.'); p; p = strchr(p, '.'))
     *p = '_';
   return buf;
}

/**
 * @brief Generates C-style names (regular, uppercase, lowercase) for a given Eolian class.
 *
 * This function populates the output parameters with newly allocated strings
 * for the class name in different C-style formats.
 * - `cname`: Standard C name (e.g., "my_class_name").
 * - `cnameu`: Uppercase C name (e.g., "MY_CLASS_NAME").
 * - `cnamel`: Lowercase C name (e.g., "my_class_name").
 *
 * @param cl The Eolian_Class object.
 * @param[out] cname Pointer to a char* to store the standard C name. If the input
 *                   pointer is NULL, this name is not generated. The caller is
 *                   responsible for freeing the allocated string.
 * @param[out] cnameu Pointer to a char* to store the uppercase C name. If the input
 *                    pointer is NULL, this name is not generated. The caller is
 *                    responsible for freeing the allocated string.
 * @param[out] cnamel Pointer to a char* to store the lowercase C name. If the input
 *                    pointer is NULL, this name is not generated. The caller is
 *                    responsible for freeing the allocated string.
 * @note This function will abort if memory allocation fails. If `cname` (the output parameter)
 *       is NULL, the internally generated `cn` (standard C name) will be freed if it's not
 *       assigned to `*cname`.
 */
void eo_gen_class_names_get(const Eolian_Class *cl, char **cname,
                            char **cnameu, char **cnamel)
{
   char *cn = NULL, *cnu = NULL, *cnl = NULL;
   cn = eo_gen_c_full_name_get(eolian_class_c_name_get(cl));
   if (!cn)
     abort();
   if (cname)
     *cname = cn;

   if (cnameu)
     {
        cnu = strdup(cn);
        if (!cnu)
          {
             free(cn);
             abort();
          }
        eina_str_toupper(&cnu);
        *cnameu = cnu;
     }

   if (cnamel)
     {
        cnl = strdup(cn);
        if (!cnl)
          {
             free(cn);
             free(cnu);
             abort();
          }
        eina_str_tolower(&cnl);
        *cnamel = cnl;
     }

   if (!cname)
     free(cn);
}

/**
 * @brief Generates and writes a C header file (.eo.h or .eot.h).
 *
 * This function generates:
 * 1. Type definitions related to the Eolian objects in the input file.
 * 2. Class typedefs.
 * 3. General header content for classes.
 * All content is wrapped in appropriate include guards.
 *
 * @param eos The global Eolian state.
 * @param state The Eolian state specific to the current generation context (often same as eos).
 * @param ofname The output filename for the header.
 * @param ifname The input Eolian filename (basename, e.g., "my_object.eo").
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_write_header(const Eolian_State *eos, const Eolian_State *state, const char *ofname,
              const char *ifname)
{
   INF("generating header: %s", ofname);
   Eina_Strbuf *buf = eina_strbuf_new();

   eo_gen_types_header_gen(state, eolian_state_objects_by_file_get(eos, ifname),
                           buf, EINA_TRUE);
   buf = _include_guard(ifname, "TYPES", buf);

   Eina_Strbuf *cltd = eo_gen_class_typedef_gen(eos, ifname);
   if (cltd)
     {
        cltd = _include_guard(ifname, "CLASS_TYPE", cltd);
        eina_strbuf_prepend_char(buf, '\n');
        eina_strbuf_prepend(buf, eina_strbuf_string_get(cltd));
        eina_strbuf_free(cltd);
     }

   eo_gen_header_gen(state, eolian_state_class_by_file_get(eos, ifname), buf);

   buf = _include_guard(_get_filename(ofname), NULL, buf);
   if (_write_file(ofname, buf))
     {
        eina_strbuf_free(buf);
        return EINA_TRUE;
     }

   eina_strbuf_free(buf);
   return EINA_FALSE;
}

/**
 * @brief Generates and writes a C stub header file (.eo.stub.h or .eot.stub.h).
 *
 * This function generates:
 * 1. Type definitions (typically for enums, structs if not full types).
 * 2. Class typedefs.
 * The content is wrapped in include guards. Stub headers are minimal
 * declarations often used for forward declarations or when full type
 * information isn't needed.
 *
 * @param eos The global Eolian state.
 * @param state The Eolian state specific to the current generation context.
 * @param ofname The output filename for the stub header.
 * @param ifname The input Eolian filename (basename).
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_write_stub_header(const Eolian_State *eos, const Eolian_State *state, const char *ofname,
                   const char *ifname)
{
   INF("generating stub header: %s", ofname);
   Eina_Strbuf *buf = eina_strbuf_new();

   eo_gen_types_header_gen(state, eolian_state_objects_by_file_get(eos, ifname),
                           buf, EINA_FALSE);

   Eina_Strbuf *cltd = eo_gen_class_typedef_gen(eos, ifname);
   if (cltd)
     {
        eina_strbuf_prepend_char(buf, '\n');
        eina_strbuf_prepend(buf, eina_strbuf_string_get(cltd));
        eina_strbuf_free(cltd);
     }

   buf = _include_guard(_get_filename(ofname), "STUBS", buf);

   Eina_Bool ret = _write_file(ofname, buf);
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Generates and writes a C source file (.eo.c).
 *
 * This function generates:
 * 1. Source code for Eolian types (e.g., enum to string functions).
 * 2. Source code for Eolian class methods and infrastructure.
 * If no actual code is generated (e.g., for an Eolian file with only types
 * or an empty .eot file), a comment "Nothing to implement." is written to
 * ensure a file is created, which can be useful for build systems.
 *
 * @param eos The global Eolian state.
 * @param ofname The output filename for the source file.
 * @param ifname The input Eolian filename (basename).
 * @param eot EINA_TRUE if the input file is an .eot file, EINA_FALSE otherwise.
 *            .eot files might require a stub even if no class is present.
 * @return EINA_TRUE if the file was written or if no class/eot indicated no action,
 *         EINA_FALSE on write failure.
 */
static Eina_Bool
_write_source(const Eolian_State *eos, const char *ofname,
              const char *ifname, Eina_Bool eot)
{
   INF("generating source: %s", ofname);
   Eina_Strbuf *buf = eina_strbuf_new();
   Eina_Bool ret = EINA_FALSE;

   const Eolian_Class *cl = eolian_state_class_by_file_get(eos, ifname);
   eo_gen_types_source_gen(eolian_state_objects_by_file_get(eos, ifname), buf);
   eo_gen_source_gen(cl, buf);
   if (cl || eot)
     {
        /* always have at least a stub in order to allow unconditional generation */
        if (!eina_strbuf_length_get(buf))
          eina_strbuf_append(buf, "/* Nothing to implement. */\n");
        if (!_write_file(ofname, buf))
          goto done;
        ret = EINA_TRUE;
     }

done:
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Generates and writes (or merges into) a C implementation file (.c).
 *
 * This function reads an existing implementation file (if any), generates
 * stubs or boilerplate for Eolian class implementations (e.g., method skeletons),
 * and merges this with the existing content. It's designed to help developers
 * by providing the structure for implementing Eolian interfaces.
 *
 * @param eos The global Eolian state.
 * @param ofname The output filename for the implementation file. This file might
 *               be read from and written to.
 * @param ifname The input Eolian filename (basename).
 * @return EINA_TRUE on success, EINA_FALSE if the class is not found, or on
 *         file read/write errors.
 */
static Eina_Bool
_write_impl(const Eolian_State *eos, const char *ofname, const char *ifname)
{
   INF("generating impl: %s", ofname);

   const Eolian_Class *cl = eolian_state_class_by_file_get(eos, ifname);
   if (!cl)
     return EINA_FALSE;

   Eina_Strbuf *buf;
   if (!_read_file(ofname, &buf))
     return EINA_FALSE;

   eo_gen_impl_gen(cl, buf);
   Eina_Bool ret = _write_file(ofname, buf);
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Appends a dependency line to a buffer if the specified generation type is active.
 * A dependency line typically looks like: "output_file.h: input.eo dep1.eo dep2.eo\n"
 *
 * @param buf The main string buffer to append the full dependency line to.
 * @param dbuf A string buffer containing the common part of the dependency line,
 *             starting from ": " followed by all dependency files (e.g., ": main.eo common.eo").
 * @param outs Array of output filenames. `outs[_get_bit_pos(what)]` gives the target filename.
 * @param gen_what Bitmask of currently active generation types.
 * @param what The specific generation type flag (e.g., GEN_H, GEN_C) to check for.
 */
static void
_append_dep_line(Eina_Strbuf *buf, Eina_Strbuf *dbuf, char **outs, int gen_what, int what)
{
   if (!(gen_what & what))
     return;
   eina_strbuf_append(buf, outs[_get_bit_pos(what)]);
   eina_strbuf_append_buffer(buf, dbuf);
}

/**
 * @brief Generates and writes a Makefile-style dependency file (.d).
 *
 * This file lists dependencies for generated files. For example, it might state
 * that `my_object.eo.h` depends on `my_object.eo` and any Eolian files it imports.
 *
 * The `gen_what` parameter controls which generated files' dependencies are included:
 * - If `GEN_D_FULL` is set in `gen_what`, dependencies for .c and .c (impl) files
 *   are also included.
 * - Otherwise (for `GEN_D`), only dependencies for .h and .stub.h files are included.
 *
 * @param eos The global Eolian state.
 * @param ofname The output filename for the dependency file (e.g., "my_object.eo.d").
 * @param ifname The input Eolian filename (basename, e.g., "my_object.eo").
 * @param outs Array of output filenames for various generated types.
 * @param gen_what Bitmask indicating which generation types are active. This determines
 *                 which output files will have their dependencies listed.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unit not found, write error).
 */
static Eina_Bool
_write_deps(const Eolian_State *eos, const char *ofname, const char *ifname,
            char **outs, int gen_what)
{
   INF("generating deps: %s", ofname);

   Eina_Bool ret = EINA_TRUE;
   Eina_Strbuf *buf = eina_strbuf_new();
   Eina_Strbuf *dbuf = eina_strbuf_new();

   const Eolian_Unit *un = eolian_state_unit_by_file_get(eos, ifname);
   if (!un)
     {
        ret = EINA_FALSE;
        goto result;
     }

   eina_strbuf_append(dbuf, ": ");
   /* every generated file depends on its .eo/.eot file */
   eina_strbuf_append(dbuf, eolian_unit_file_path_get(un));

   const Eolian_Unit *dun;
   Eina_Iterator *deps = eolian_unit_children_get(un);
   EINA_ITERATOR_FOREACH(deps, dun)
     {
        const char *dpath = eolian_unit_file_path_get(dun);
        if (!dpath)
          continue;
        eina_strbuf_append_char(dbuf, ' ');
        eina_strbuf_append(dbuf, dpath);
     }
   eina_iterator_free(deps);
   eina_strbuf_append_char(dbuf, '\n');

   _append_dep_line(buf, dbuf, outs, gen_what, GEN_H);
   _append_dep_line(buf, dbuf, outs, gen_what, GEN_H_STUB);

   if (gen_what & GEN_D_FULL)
     {
        _append_dep_line(buf, dbuf, outs, gen_what, GEN_C);
        _append_dep_line(buf, dbuf, outs, gen_what, GEN_C_IMPL);
     }

   ret = _write_file(ofname, buf);
result:
   eina_strbuf_free(dbuf);
   eina_strbuf_free(buf);
   return ret;
}

/**
 * @brief Main entry point for the Eolian C generator.
 *
 * Parses command-line arguments, initializes Eolian and Eina, processes the
 * input Eolian file, and generates the requested output files (headers, sources,
 * dependency files).
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return 0 on success, 1 on failure.
 */
int
main(int argc, char **argv)
{
   int pret = 1;

   char *outs[sizeof(_dexts) / sizeof(void *)] = {
     NULL, NULL, NULL, NULL, NULL, NULL
   };
   char *basen = NULL;
   _eolian_api_symbol = strdup("EAPI");
   Eina_List *includes = NULL;

   eina_init();
   eolian_init();

   Eolian_State *eos = eolian_state_new();

   const char *dom = "eolian_gen";
   _eolian_gen_log_dom = eina_log_domain_register(dom, EINA_COLOR_GREEN);
   if (_eolian_gen_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: %s", dom);
        goto end;
     }

   eina_log_timing(_eolian_gen_log_dom, EINA_LOG_STATE_STOP, EINA_LOG_STATE_INIT);

   int gen_what = 0;
   Eina_Bool scan_system = EINA_TRUE;

   for (int opt; (opt = getopt(argc, argv, "SI:g:o:hve:")) != -1;)
     switch (opt)
       {
        case 0:
          break;
        case 'S':
          scan_system = EINA_FALSE;
          break;
        case 'I':
          /* just a pointer to argv contents, so it persists */
          includes = eina_list_append(includes, optarg);
          break;
        case 'e':
          free(_eolian_api_symbol);
          _eolian_api_symbol = strdup(optarg);
          break;
        case 'g':
          for (const char *wstr = optarg; *wstr; ++wstr)
            switch (*wstr)
              {
               case 'h':
                 gen_what |= GEN_H;
                 break;
               case 's':
                 gen_what |= GEN_H_STUB;
                 break;
               case 'c':
                 gen_what |= GEN_C;
                 break;
               case 'i':
                 gen_what |= GEN_C_IMPL;
                 break;
               case 'd':
                 gen_what |= GEN_D;
                 break;
               case 'D':
                 gen_what |= GEN_D_FULL;
                 break;
               default:
                 fprintf(stderr, "unknown type: '%c'\n", *wstr);
                 goto end;
              }
          break;
        case 'o':
          if (strchr(optarg, ':'))
            {
               const char *abeg = optarg;
               const char *cpos = strchr(abeg, ':');
               if (((cpos - abeg) != 1) ||
                   !_try_set_out(*abeg, outs, cpos + 1, &gen_what))
                 {
                    char *oa = strdup(abeg);
                    oa[cpos - abeg] = '\0';
                    fprintf(stderr, "unknown type: '%s'\n", oa);
                    free(oa);
                    goto end;
                 }
            }
          else
            {
               if (basen)
                 free(basen);
               basen = strdup(optarg);
            }
          break;
        case 'h':
          _print_usage(argv[0], stdout);
          pret = 0;
          goto end;
        case 'v':
          _print_version(stdout);
          pret = 0;
          goto end;
        default:
          _print_usage(argv[0], stderr);
          goto end;
       }

   const char *input = argv[optind];
   if (!input)
     {
        fprintf(stderr, "eolian: no input file\n");
        goto end;
     }

   const char *ext = strrchr(input, '.');
   if (!ext || (strcmp(ext, ".eo") && strcmp(ext, ".eot")))
     {
        fprintf(stderr, "eolian: invalid input file '%s'\n", input);
        goto end;
     }

   if (scan_system)
     {
        if (!eolian_state_system_directory_add(eos))
          {
             fprintf(stderr, "eolian: could not scan system directory\n");
             goto end;
          }
     }

   const char *inc;
   EINA_LIST_FREE(includes, inc)
     {
        if (!eolian_state_directory_add(eos, inc))
          {
             fprintf(stderr, "eolian: could not scan '%s'\n", inc);
             goto end;
          }
     }

   if (!eolian_state_file_path_parse(eos, input))
     {
        fprintf(stderr, "eolian: could not parse file '%s'\n", input);
        goto end;
     }

   _fill_all_outs(outs, input, basen);

   const char *eobn = _get_filename(input);

   if (!gen_what)
     gen_what = GEN_H | GEN_C;

   Eina_Bool succ = EINA_TRUE;
   if (gen_what & GEN_H)
     succ = _write_header(eos, eos, outs[_get_bit_pos(GEN_H)], eobn);
   if (succ && (gen_what & GEN_H_STUB))
     succ = _write_stub_header(eos, eos, outs[_get_bit_pos(GEN_H_STUB)], eobn);
   if (succ && (gen_what & GEN_C))
     succ = _write_source(eos, outs[_get_bit_pos(GEN_C)], eobn, !strcmp(ext, ".eot"));
   if (succ && (gen_what & GEN_C_IMPL))
     succ = _write_impl(eos, outs[_get_bit_pos(GEN_C_IMPL)], eobn);

   if (succ && (gen_what & GEN_D_FULL))
     succ = _write_deps(eos, outs[_get_bit_pos(GEN_D_FULL)], eobn, outs, gen_what);
   else if (succ && (gen_what & GEN_D))
     succ = _write_deps(eos, outs[_get_bit_pos(GEN_D)], eobn, outs, gen_what);

   if (!succ)
     goto end;

   pret = 0;
end:
   if (_eolian_gen_log_dom >= 0)
     {
        eina_log_timing(_eolian_gen_log_dom, EINA_LOG_STATE_START, EINA_LOG_STATE_SHUTDOWN);
        eina_log_domain_unregister(_eolian_gen_log_dom);
     }

   eina_list_free(includes);
   for (size_t i = 0; i < (sizeof(_dexts) / sizeof(char *)); ++i)
     free(outs[i]);
   free(basen);

   free(_eolian_api_symbol);
   
   eolian_state_free(eos);
   eolian_shutdown();
   eina_shutdown();

   return pret;
}
