#include <ctype.h>

#include "docs.h"

/**
 * @internal
 * @brief Appends a given number of spaces to a string buffer for indentation.
 * @param[in] buf The string buffer to append to.
 * @param[in] ind The number of spaces to indent.
 * @return The number of spaces appended.
 */
static int
_indent_line(Eina_Strbuf *buf, int ind)
{
   int i;
   for (i = 0; i < ind; ++i)
     eina_strbuf_append_char(buf, ' ');
   return ind;
}

#define DOC_LINE_LIMIT 79
#define DOC_LINE_TEST 59
#define DOC_LINE_OVER 39

#define DOC_LIMIT(ind) ((ind > DOC_LINE_TEST) ? (ind + DOC_LINE_OVER) \
                                              : DOC_LINE_LIMIT)

#define SUMMARY_OR_DEFAULT(sum) (sum ? sum : "No description supplied.")

/**
 * @internal
 * @brief Generates a C-style reference for a given Eolian reference name.
 *
 * This function resolves an Eolian reference name (e.g., "My.Class.method")
 * into its corresponding C representation. It handles various Eolian object
 * types like constants, struct fields, enum fields, methods, and properties.
 * The resolved name is appended to the provided string buffer.
 *
 * If the reference cannot be resolved, the original reference name is appended
 * as a fallback.
 *
 * @param[in] state The Eolian state.
 * @param[in] refn The Eolian reference name to resolve.
 * @param[in,out] wbuf The string buffer to append the resolved C name to.
 */
static void
_generate_ref(const Eolian_State *state, const char *refn, Eina_Strbuf *wbuf)
{
   const Eolian_Object *decl = eolian_state_object_by_name_get(state, refn);
   if (decl)
     {
        char *n = strdup(eolian_object_name_get(decl));
        char *p = n;
        while ((p = strchr(p, '.'))) *p = '_';
        if (eolian_object_type_get(decl) == EOLIAN_OBJECT_CONSTANT)
          eina_str_toupper(&n);
        eina_strbuf_append(wbuf, n);
        free(n);
        return;
     }

   /* not a plain declaration, so it must be struct/enum field or func */
   const char *sfx = strrchr(refn, '.');
   if (!sfx) goto noref;

   Eina_Stringshare *bname = eina_stringshare_add_length(refn, sfx - refn);

   const Eolian_Typedecl *tp = eolian_state_struct_by_name_get(state, bname);
   if (tp)
     {
        if (!eolian_typedecl_struct_field_get(tp, sfx + 1))
          {
             eina_stringshare_del(bname);
             goto noref;
          }
        _generate_ref(state, bname, wbuf);
        eina_strbuf_append(wbuf, sfx);
        eina_stringshare_del(bname);
        return;
     }

   tp = eolian_state_enum_by_name_get(state, bname);
   if (tp)
     {
        const Eolian_Enum_Type_Field *efl = eolian_typedecl_enum_field_get(tp, sfx + 1);
        if (!efl)
          {
             eina_stringshare_del(bname);
             goto noref;
          }
        Eina_Stringshare *str = eolian_typedecl_enum_field_c_constant_get(efl);
        eina_strbuf_append(wbuf, str);
        eina_stringshare_del(bname);
        return;
     }

   const Eolian_Class *cl = eolian_state_class_by_name_get(state, bname);
   const Eolian_Function *fn = NULL;
   /* match methods and properties; we're only figuring out existence */
   Eolian_Function_Type ftype = EOLIAN_UNRESOLVED;
   if (!cl)
     {
        const char *mname = NULL;
        if (!strcmp(sfx, ".get")) ftype = EOLIAN_PROP_GET;
        else if (!strcmp(sfx, ".set")) ftype = EOLIAN_PROP_SET;
        if (ftype != EOLIAN_UNRESOLVED)
          {
             eina_stringshare_del(bname);
             mname = sfx - 1;
             while ((mname != refn) && (*mname != '.')) --mname;
             if (mname == refn) goto noref;
             bname = eina_stringshare_add_length(refn, mname - refn);
             cl = eolian_state_class_by_name_get(state, bname);
             eina_stringshare_del(bname);
          }
        if (cl)
          {
             char *meth = eina_strndup(mname + 1, sfx - mname - 1);
             fn = eolian_class_function_by_name_get(cl, meth, ftype);
             if (ftype == EOLIAN_UNRESOLVED)
               ftype = eolian_function_type_get(fn);
             free(meth);
          }
     }
   else
     {
        fn = eolian_class_function_by_name_get(cl, sfx + 1, ftype);
        ftype = eolian_function_type_get(fn);
     }

   if (!fn) goto noref;

   Eina_Stringshare *fcn = eolian_function_full_c_name_get(fn, ftype);
   if (!fcn) goto noref;
   eina_strbuf_append(wbuf, fcn);
   eina_stringshare_del(fcn);
   return;
noref:
   eina_strbuf_append(wbuf, refn);
}

/**
 * @internal
 * @brief Appends a formatted documentation section to a buffer.
 *
 * This function processes a block of documentation text (`desc`), handling
 * word wrapping, indentation, and special formatting tags. The formatted text
 * is appended to `buf`.
 *
 * It supports the following special syntax in the input text:
 * - `Note:`, `Warning:`, `Remark:`, `TODO:`: These are converted to
 *   `@note`, `@warning`, `@remark`, `@todo` Doxygen commands.
 * - `\\@`, `\\$`: Escapes for '@' and '$' characters.
 * - `@ref.name`: Converted to `@ref C_style_name`. The name is resolved
 *   using _generate_ref().
 * - `$[...text...]`: Formats `text` with a fixed-width font, using `<tt>`.
 * - `$name`: Converted to `@c name`.
 * - Newlines are preserved and formatted correctly within the C-style comment.
 *
 * @param[in] state The Eolian state, used for resolving references.
 * @param[in] desc The description text to append.
 * @param[in] ind The base indentation level (number of spaces).
 * @param[in] curl The current line length.
 * @param[in,out] buf The main string buffer for the final documentation comment.
 * @param[in,out] wbuf A temporary working buffer for word processing.
 * @return The updated current line length.
 */
static int
_append_section(const Eolian_State *state, const char *desc, int ind, int curl,
                Eina_Strbuf *buf, Eina_Strbuf *wbuf)
{
   Eina_Bool try_note = EINA_TRUE;
   while (*desc)
     {
        while (*desc && isspace(*desc) && (*desc != '\n'))
          eina_strbuf_append_char(wbuf, *desc++);
        if (try_note)
          {
#define CHECK_NOTE(str) !strncmp(desc, str ": ", sizeof(str ":"))
             if (CHECK_NOTE("Note"))
               {
                  eina_strbuf_append(wbuf, "@note ");
                  desc += sizeof("Note:");
               }
             else if (CHECK_NOTE("Warning"))
               {
                  eina_strbuf_append(wbuf, "@warning ");
                  desc += sizeof("Warning:");
               }
             else if (CHECK_NOTE("Remark"))
               {
                  eina_strbuf_append(wbuf, "@remark ");
                  desc += sizeof("Remark:");
               }
             else if (CHECK_NOTE("TODO"))
               {
                  eina_strbuf_append(wbuf, "@todo ");
                  desc += sizeof("TODO:");
               }
#undef CHECK_NOTE
             try_note = EINA_FALSE;
          }
        int limit = DOC_LIMIT(ind);
        int wlen;
        if (*desc == '\\')
          {
             desc++;
             if ((*desc != '@') && (*desc != '$'))
               eina_strbuf_append_char(wbuf, '\\');
             eina_strbuf_append_char(wbuf, *desc++);
          }
        else if (*desc == '@')
          {
             const char *ref = ++desc;
             if (isalpha(*desc) || (*desc == '_'))
               {
                  eina_strbuf_append(wbuf, "@ref ");
                  while (isalnum(*desc) || (*desc == '.') || (*desc == '_'))
                    ++desc;
                  if (*(desc - 1) == '.') --desc;
                  Eina_Stringshare *refn = eina_stringshare_add_length(ref, desc - ref);
                  _generate_ref(state, refn, wbuf);
                  eina_stringshare_del(refn);
               }
             else
               eina_strbuf_append_char(wbuf, '@');
          }
        else if (*desc == '$')
          {
             if (*++desc == '[')
               {
                  ++desc;
                  eina_strbuf_append(wbuf, "<tt>");
                  wlen = eina_strbuf_length_get(wbuf);
                  while ((*desc != '\0') && (*desc != ']') && (*desc != '\n'))
                    {
                       if (*desc == ' ')
                         {
                            eina_strbuf_append_char(wbuf, ' ');
                            wlen = eina_strbuf_length_get(wbuf);
                            if ((int)(curl + wlen) > limit)
                              {
                                 curl = 3;
                                 eina_strbuf_append_char(buf, '\n');
                                 curl += _indent_line(buf, ind);
                                 eina_strbuf_append(buf, " * ");
                                 if (*eina_strbuf_string_get(wbuf) == ' ')
                                   eina_strbuf_remove(wbuf, 0, 1);
                              }
                            curl += eina_strbuf_length_get(wbuf);
                            eina_strbuf_append(buf, eina_strbuf_string_get(wbuf));
                            eina_strbuf_reset(wbuf);
                            ++desc;
                            continue;
                         }
                       /* skip escape */
                       if (*desc == '\\')
                         {
                            ++desc;
                            if ((*desc == '\0') || (*desc == '\n'))
                              break;
                         }
                       eina_strbuf_append_char(wbuf, *desc++);
                    }
                  if (*desc == ']')
                    ++desc;
                  eina_strbuf_append(wbuf, "</tt>");
                  curl += 5;
                  goto split;
               }
             if (isalpha(*desc))
               eina_strbuf_append(wbuf, "@c ");
             else
               eina_strbuf_append_char(wbuf, '$');
          }
        while (*desc && !isspace(*desc))
          eina_strbuf_append_char(wbuf, *desc++);
split:
        wlen = eina_strbuf_length_get(wbuf);
        if ((int)(curl + wlen) > limit)
          {
             curl = 3;
             eina_strbuf_append_char(buf, '\n');
             curl += _indent_line(buf, ind);
             eina_strbuf_append(buf, " * ");
             if (*eina_strbuf_string_get(wbuf) == ' ')
               eina_strbuf_remove(wbuf, 0, 1);
          }
        curl += eina_strbuf_length_get(wbuf);
        eina_strbuf_append(buf, eina_strbuf_string_get(wbuf));
        eina_strbuf_reset(wbuf);
        if (*desc == '\n')
          {
             desc++;
             eina_strbuf_append_char(buf, '\n');
             while (*desc == '\n')
               {
                  _indent_line(buf, ind);
                  eina_strbuf_append(buf, " *\n");
                  desc++;
                  try_note = EINA_TRUE;
               }
             curl = _indent_line(buf, ind) + 3;
             eina_strbuf_append(buf, " * ");
          }
     }
   return curl;
}

/**
 * @internal
 * @brief Appends a "@since" version tag to the documentation if available.
 *
 * @param[in] since The version string (e.g., "1.2.3"). Can be NULL.
 * @param[in] indent The indentation level.
 * @param[in] curl The current line length.
 * @param[in,out] buf The string buffer to append to.
 * @return The updated current line length.
 */
static int
_append_since(const char *since, int indent, int curl, Eina_Strbuf *buf)
{
   if (since)
     {
        eina_strbuf_append_char(buf, '\n');
        _indent_line(buf, indent);
        eina_strbuf_append(buf, " *\n");
        curl = _indent_line(buf, indent);
        eina_strbuf_append(buf, " * @since ");
        eina_strbuf_append(buf, since);
        curl += strlen(since) + sizeof(" * @since ") - 1;
     }
   return curl;
}

/**
 * @internal
 * @brief Appends an extra line of text to the documentation.
 *
 * Used for adding things like `@return` documentation for events.
 *
 * @param[in] el The extra text to append. Can be NULL.
 * @param[in] indent The indentation level.
 * @param[in] curl The current line length.
 * @param[in] nl If EINA_TRUE, adds a blank " *" line before the text.
 * @param[in,out] buf The string buffer to append to.
 * @return The updated current line length.
 */
static int
_append_extra(const char *el, int indent, int curl, Eina_Bool nl, Eina_Strbuf *buf)
{
   if (el)
     {
        eina_strbuf_append_char(buf, '\n');
        if (nl)
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
          }
        curl = _indent_line(buf, indent);
        eina_strbuf_append(buf, " * ");
        eina_strbuf_append(buf, el);
        curl += strlen(el) + sizeof(" * ") - 1;
     }
   return curl;
}

/**
 * @internal
 * @brief Sanitizes a group name by replacing dots with underscores.
 *
 * This is done to make the group name a valid C identifier for use
 * with `@ingroup`.
 *
 * @param[in] group The group name string.
 * @return A newly allocated sanitized string, or NULL if input is NULL.
 *         The caller is responsible for freeing the returned string.
 */
static char *
_sanitize_group(const char *group)
{
   if (!group) return NULL;
   char *ret = strdup(group);
   char *p;
   while ((p = strchr(ret, '.'))) *p = '_';
   return ret;
}

/**
 * @internal
 * @brief Appends an "@ingroup" tag to the documentation.
 *
 * @param[in,out] buf The string buffer to append to.
 * @param[in] sgrp The sanitized group name. The string is freed by this function.
 * @param[in] indent The indentation level.
 */
static void
_append_group(Eina_Strbuf *buf, char *sgrp, int indent)
{
   if (!sgrp) return;
   eina_strbuf_append(buf, " * @ingroup ");
   eina_strbuf_append(buf, sgrp);
   eina_strbuf_append_char(buf, '\n');
   _indent_line(buf, indent);
   free(sgrp);
}

/**
 * @internal
 * @brief Generates a brief, single-line style documentation comment.
 *
 * This is used when an Eolian element has only a summary and no detailed
 * description. The output is formatted like `/**< summary text * /`.
 *
 * @param[in] state The Eolian state.
 * @param[in] summary The summary text.
 * @param[in] since The "since" version string.
 * @param[in] group The documentation group.
 * @param[in] el Extra text to append (e.g., return value info).
 * @param[in] indent The base indentation level.
 * @param[in,out] buf The string buffer to write the documentation to.
 */
static void
_gen_doc_brief(const Eolian_State *state, const char *summary, const char *since,
               const char *group, const char *el, int indent, Eina_Strbuf *buf)
{
   int curl = 4 + indent;
   Eina_Strbuf *wbuf = eina_strbuf_new();
   if (indent)
     eina_strbuf_append(buf, "/**< ");
   else
     eina_strbuf_append(buf, "/** ");
   curl = _append_section(state, summary, indent, curl, buf, wbuf);
   eina_strbuf_free(wbuf);
   curl = _append_extra(el, indent, curl, EINA_FALSE, buf);
   curl = _append_since(since, indent, curl, buf);
   char *sgrp = _sanitize_group(group);
   if (((curl + 3) > DOC_LIMIT(indent)) || sgrp)
     {
        eina_strbuf_append_char(buf, '\n');
        _indent_line(buf, indent);
        if (sgrp) eina_strbuf_append(buf, " *");
     }
   if (sgrp)
     {
        eina_strbuf_append_char(buf, '\n');
        _indent_line(buf, indent);
     }
   _append_group(buf, sgrp, indent);
   eina_strbuf_append(buf, " */");
}

/**
 * @internal
 * @brief Generates a full, multi-line documentation comment.
 *
 * This is used when an Eolian element has both a summary and a detailed
 * description. The output is a standard Doxygen block with `@brief`.
 *
 * @param[in] state The Eolian state.
 * @param[in] summary The summary text.
 * @param[in] description The detailed description text.
 * @param[in] since The "since" version string.
 * @param[in] group The documentation group.
 * @param[in] el Extra text to append (e.g., return value info).
 * @param[in] indent The base indentation level.
 * @param[in,out] buf The string buffer to write the documentation to.
 */
static void
_gen_doc_full(const Eolian_State *state, const char *summary,
              const char *description, const char *since,
              const char *group, const char *el, int indent, Eina_Strbuf *buf)
{
   int curl = 0;
   Eina_Strbuf *wbuf = eina_strbuf_new();
   if (indent)
     eina_strbuf_append(buf, "/**<\n");
   else
     eina_strbuf_append(buf, "/**\n");
   curl += _indent_line(buf, indent);
   eina_strbuf_append(buf, " * @brief ");
   curl += sizeof(" * @brief ") - 1;
   _append_section(state, summary, indent, curl, buf, wbuf);
   eina_strbuf_append_char(buf, '\n');
   _indent_line(buf, indent);
   eina_strbuf_append(buf, " *\n");
   curl = _indent_line(buf, indent);
   eina_strbuf_append(buf, " * ");
   _append_section(state, description, indent, curl + 3, buf, wbuf);
   curl = _append_extra(el, indent, curl, EINA_TRUE, buf);
   curl = _append_since(since, indent, curl, buf);
   eina_strbuf_append_char(buf, '\n');
   _indent_line(buf, indent);
   char *sgrp = _sanitize_group(group);
   if (sgrp)
     {
        eina_strbuf_append(buf, " *\n");
        _indent_line(buf, indent);
     }
   _append_group(buf, sgrp, indent);
   eina_strbuf_append(buf, " */");
   eina_strbuf_free(wbuf);
}

/**
 * @internal
 * @brief Top-level helper for generating a documentation comment string buffer.
 *
 * This function inspects the provided Eolian documentation. If a detailed
 * description exists, it calls _gen_doc_full() to generate a full
 * documentation block. Otherwise, it calls _gen_doc_brief() for a compact
 * comment.
 *
 * @param[in] state The Eolian state.
 * @param[in] doc The Eolian documentation object.
 * @param[in] group The documentation group.
 * @param[in] el Extra text to append.
 * @param[in] indent The base indentation level.
 * @return A new Eina_Strbuf containing the generated documentation comment,
 *         or NULL if `doc` is NULL. The caller owns the returned buffer.
 */
static Eina_Strbuf *
_gen_doc_buf(const Eolian_State *state, const Eolian_Documentation *doc,
             const char *group, const char *el, int indent)
{
   if (!doc) return NULL;

   const char *sum = eolian_documentation_summary_get(doc);
   sum = SUMMARY_OR_DEFAULT(sum);
   const char *desc = eolian_documentation_description_get(doc);
   const char *since = eolian_documentation_since_get(doc);

   Eina_Strbuf *buf = eina_strbuf_new();
   if (!desc)
     _gen_doc_brief(state, sum, since, group, el, indent, buf);
   else
     _gen_doc_full(state, sum, desc, since, group, el, indent, buf);
   return buf;
}

/**
 * @internal
 * @brief Implements the public API function eo_gen_docs_full_gen.
 *
 * This function is a simple wrapper around _gen_doc_buf, providing the
 * public interface for generating documentation from an Eolian_Documentation
 * object. It passes NULL for the extra line parameter.
 *
 * @see eo_gen_docs_full_gen in docs.h
 */
Eina_Strbuf *
eo_gen_docs_full_gen(const Eolian_State *state, const Eolian_Documentation *doc,
                     const char *group, int indent)
{
   return _gen_doc_buf(state, doc, group, NULL, indent);
}

/**
 * @internal
 * @brief Implements the public API function eo_gen_docs_event_gen.
 *
 * This function generates documentation for an Eolian event. It extracts the
 * summary, description, and "since" information. It also constructs a
 * `@return` annotation from the event's type if it has one.
 * If no formal documentation is available for the event, a default
 * comment block is generated. Otherwise, it uses _gen_doc_buf to create
 * the full documentation.
 *
 * @see eo_gen_docs_event_gen in docs.h
 */
Eina_Strbuf *
eo_gen_docs_event_gen(const Eolian_State *state, const Eolian_Event *ev,
                      const char *group)
{
   if (!ev) return NULL;

   const Eolian_Documentation *doc = eolian_event_documentation_get(ev);

   char buf[1024];
   const Eolian_Type *rt = eolian_event_type_get(ev);
   const char *p = NULL;
   if (rt)
     {
        p = buf;
        Eina_Stringshare *rts = eolian_type_c_type_get(rt);
        snprintf(buf, sizeof(buf), "@return %s", rts);
        eina_stringshare_del(rts);
     }

   if (!doc)
     {
        Eina_Strbuf *bufs = eina_strbuf_new();
        eina_strbuf_append(bufs, "/**\n * No description\n");
        if (p)
          {
             eina_strbuf_append(bufs, " * ");
             eina_strbuf_append(bufs, p);
             eina_strbuf_append_char(bufs, '\n');
          }
        eina_strbuf_append(bufs, " */");
        return bufs;
     }

   return _gen_doc_buf(state, doc, group, p, 0);
}

/**
 * @internal
 * @brief Implements the public API function eo_gen_docs_func_gen.
 *
 * This is a complex documentation generator for functions (methods, properties).
 * It orchestrates the collection of documentation from various Eolian objects
 * associated with a function, such as:
 * - The main implement documentation (for methods or properties).
 * - Specific documentation for property getters/setters.
 * - Return value documentation.
 * - Parameter documentation.
 *
 * A key part of its logic is to correctly identify and iterate over parameters,
 * which can be keys or values for properties. It also handles a special case for
 * property getters where a single value parameter is treated as the return value
 * if no explicit return type is defined.
 *
 * The function builds a complete Doxygen comment block, including brief summary,
 * detailed description, parameter list, return value, "since" version, and group.
 * If only a summary is present, it generates a compact, single-line comment;
 * otherwise, it creates a full multi-line block.
 *
 * @see eo_gen_docs_func_gen in docs.h
 */
Eina_Strbuf *
eo_gen_docs_func_gen(const Eolian_State *state, const Eolian_Function *fid,
                     Eolian_Function_Type ftype, int indent)
{
   const Eolian_Function_Parameter *par = NULL;
   const Eolian_Function_Parameter *vpar = NULL;

   const Eolian_Documentation *doc, *pdoc, *rdoc;

   Eina_Iterator *itr = NULL;
   Eina_Iterator *vitr = NULL;
   Eina_Bool force_out = EINA_FALSE;

   Eina_Strbuf *buf = eina_strbuf_new();
   Eina_Strbuf *wbuf = NULL;

   const char *sum = NULL, *desc = NULL, *since = NULL;

   int curl = 0;

   const char *group = eolian_class_c_name_get(eolian_function_class_get(fid));
   const Eolian_Implement *fimp = eolian_function_implement_get(fid);

   if (ftype == EOLIAN_METHOD)
     {
        doc = eolian_implement_documentation_get(fimp, EOLIAN_METHOD);
        pdoc = NULL;
     }
   else
     {
        doc = eolian_implement_documentation_get(fimp, EOLIAN_PROPERTY);
        pdoc = eolian_implement_documentation_get(fimp, ftype);
        if (!doc && pdoc) doc = pdoc;
        if (pdoc == doc) pdoc = NULL;
     }

   rdoc = eolian_function_return_documentation_get(fid, ftype);

   if (doc)
     {
         sum = eolian_documentation_summary_get(doc);
         desc = eolian_documentation_description_get(doc);
         since = eolian_documentation_since_get(doc);
         if (pdoc && eolian_documentation_since_get(pdoc))
           since = eolian_documentation_since_get(pdoc);
     }

   if (ftype == EOLIAN_METHOD)
     {
        itr = eolian_function_parameters_get(fid);
     }
   else
     {
        itr = eolian_property_keys_get(fid, ftype);
        vitr = eolian_property_values_get(fid, ftype);
        if (!vitr || !eina_iterator_next(vitr, (void**)&vpar))
          {
             eina_iterator_free(vitr);
             vitr = NULL;
         }
     }

   if (!itr || !eina_iterator_next(itr, (void**)&par))
     {
        eina_iterator_free(itr);
        itr = NULL;
     }

   /* when return is not set on getter, value becomes return instead of param */
   if (ftype == EOLIAN_PROP_GET && !eolian_function_return_type_get(fid, ftype))
     {
        const Eolian_Function_Parameter *rvpar = vpar;
        if (!eina_iterator_next(vitr, (void**)&vpar))
          {
             /* one value - not out param */
             eina_iterator_free(vitr);
             rdoc = rvpar ? eolian_parameter_documentation_get(rvpar) : NULL;
             vitr = NULL;
             vpar = NULL;
          }
        else
          {
             /* multiple values - always out params */
             eina_iterator_free(vitr);
             vitr = eolian_property_values_get(fid, ftype);
             if (!vitr)
               vpar = NULL;
             else if (!eina_iterator_next(vitr, (void**)&vpar))
               {
                  eina_iterator_free(vitr);
                  vitr = NULL;
                  vpar = NULL;
               }
          }
     }

   if (!par)
     {
        /* no keys, try values */
        itr = vitr;
        par = vpar;
        vitr = NULL;
        vpar = NULL;
        if (ftype == EOLIAN_PROP_GET)
          force_out = EINA_TRUE;
     }

   /* only summary, nothing else; generate standard brief doc */
   if (!desc && !par && !vpar && !rdoc && (ftype == EOLIAN_METHOD || !pdoc))
     {
        _gen_doc_brief(state, SUMMARY_OR_DEFAULT(sum), since, group,
                       NULL, indent, buf);
        return buf;
     }

   wbuf = eina_strbuf_new();

   eina_strbuf_append(buf, "/**\n");
   curl += _indent_line(buf, indent);
   eina_strbuf_append(buf, " * @brief ");
   curl += sizeof(" * @brief ") - 1;
   _append_section(state, SUMMARY_OR_DEFAULT(sum),
                   indent, curl, buf, wbuf);

   eina_strbuf_append_char(buf, '\n');
   if (desc || since || par || rdoc || pdoc)
     {
        _indent_line(buf, indent);
        eina_strbuf_append(buf, " *\n");
     }

   if (desc)
     {
        curl = _indent_line(buf, indent);
        eina_strbuf_append(buf, " * ");
        _append_section(state, desc, indent, curl + 3, buf, wbuf);
        eina_strbuf_append_char(buf, '\n');
        if (par || rdoc || pdoc || since)
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
          }
     }

   if (pdoc)
     {
        const char *pdesc = eolian_documentation_description_get(pdoc);
        curl = _indent_line(buf, indent);
        eina_strbuf_append(buf, " * ");
        const char *psum = eolian_documentation_summary_get(pdoc);
        _append_section(state, SUMMARY_OR_DEFAULT(psum), indent,
            curl + 3, buf, wbuf);
        eina_strbuf_append_char(buf, '\n');
        if (pdesc)
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
             curl = _indent_line(buf, indent);
             eina_strbuf_append(buf, " * ");
             _append_section(state, pdesc, indent, curl + 3, buf, wbuf);
             eina_strbuf_append_char(buf, '\n');
          }
        if (par || rdoc || since)
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
          }
     }

   if (!eolian_function_is_static(fid))
     {
        _indent_line(buf, indent);
        eina_strbuf_append(buf, " * @param[in] obj The object.\n");
        if (!par && (rdoc || since))
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
          }
     }

   while (par)
     {
        const Eolian_Documentation *adoc = eolian_parameter_documentation_get(par);
        curl = _indent_line(buf, indent);

        Eolian_Parameter_Direction dir = EOLIAN_PARAMETER_OUT;
        if (!force_out)
          dir = eolian_parameter_direction_get(par);

        switch (dir)
          {
           case EOLIAN_PARAMETER_OUT:
             eina_strbuf_append(buf, " * @param[out] ");
             curl += sizeof(" * @param[out] ") - 1;
             break;
           case EOLIAN_PARAMETER_INOUT:
             eina_strbuf_append(buf, " * @param[in,out] ");
             curl += sizeof(" * @param[in,out] ") - 1;
             break;
           default:
             eina_strbuf_append(buf, " * @param[in] ");
             curl += sizeof(" * @param[in] ") - 1;
             break;
          }

        const char *nm = eolian_parameter_name_get(par);
        eina_strbuf_append(buf, nm);
        curl += strlen(nm);

        if (adoc)
          {
             eina_strbuf_append_char(buf, ' ');
             curl += 1;
             const char *asum = eolian_documentation_summary_get(adoc);
             _append_section(state, SUMMARY_OR_DEFAULT(asum),
                             indent, curl, buf, wbuf);
          }

        eina_strbuf_append_char(buf, '\n');
        if (!eina_iterator_next(itr, (void**)&par))
          {
             par = NULL;
             if (vpar)
               {
                  eina_iterator_free(itr);
                  itr = vitr;
                  par = vpar;
                  vitr = NULL;
                  vpar = NULL;
                  if (ftype == EOLIAN_PROP_GET)
                    force_out = EINA_TRUE;
               }
          }

        if (!par && (rdoc || since))
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
          }
     }
   eina_iterator_free(itr);

   if (rdoc)
     {
        curl = _indent_line(buf, indent);
        eina_strbuf_append(buf, " * @return ");
        curl += sizeof(" * @return ") - 1;
        const char *rsum = eolian_documentation_summary_get(rdoc);
        _append_section(state, SUMMARY_OR_DEFAULT(rsum), indent,
            curl, buf, wbuf);
        eina_strbuf_append_char(buf, '\n');
        if (since)
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
          }
     }

   if (since)
     {
        curl = _indent_line(buf, indent);
        eina_strbuf_append(buf, " * @since ");
        eina_strbuf_append(buf, since);
        eina_strbuf_append_char(buf, '\n');
     }

   _indent_line(buf, indent);
   eina_strbuf_append(buf, " *\n");

   _indent_line(buf, indent);
   _append_group(buf, _sanitize_group(group), indent);
   eina_strbuf_append(buf, " */");
   eina_strbuf_free(wbuf);
   return buf;
}
