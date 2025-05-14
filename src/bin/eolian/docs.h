#ifndef EOLIAN_GEN_DOCS_H
#define EOLIAN_GEN_DOCS_H

#include "main.h"

/**
 * @brief Generates a complete Doxygen-style documentation comment block for
 *        an Eolian element.
 *
 * This function creates either a compact, single-line comment if only a
 * summary is available, or a full multi-line block with `@brief` if a
 * detailed description is also present. It includes "since" information,
 * group affiliation, and handles proper formatting and indentation.
 *
 * @param[in] state The Eolian state, for resolving references within docs.
 * @param[in] doc The Eolian documentation object to generate from.
 * @param[in] group The documentation group name (e.g., for `@ingroup`). Can be NULL.
 * @param[in] indent The base indentation level (number of spaces) for the comment.
 *
 * @return A new Eina_Strbuf containing the generated documentation comment.
 *         The caller is responsible for freeing this buffer.
 */
Eina_Strbuf *eo_gen_docs_full_gen(const Eolian_State *state, const Eolian_Documentation *doc, const char *group, int indent);

/**
 * @brief Generates documentation for an Eolian function (method or property).
 *
 * This function builds a comprehensive Doxygen comment for a function.
 * It intelligently combines documentation from multiple sources: the main
 * summary and description from the function's implement, specific details
 * from property getters/setters, and documentation for all parameters and
 * the return value. It also includes "since" and "ingroup" tags.
 *
 * @param[in] state The Eolian state, for resolving references.
 * @param[in] fid The Eolian function object.
 * @param[in] ftype The specific function type to document (e.g.,
 *            EOLIAN_METHOD, EOLIAN_PROP_GET, EOLIAN_PROP_SET).
 * @param[in] indent The base indentation level for the comment.
 *
 * @return A new Eina_Strbuf containing the generated documentation comment.
 *         The caller is responsible for freeing this buffer.
 */
Eina_Strbuf *eo_gen_docs_func_gen(const Eolian_State *state, const Eolian_Function *fid, Eolian_Function_Type ftype, int indent);

/**
 * @brief Generates documentation for an Eolian event.
 *
 * This function creates a Doxygen comment block for a given Eolian event.
 * If the event has an associated data type, a `@return` tag is automatically
 * generated to document it. If no documentation is found for the event, a
 * default "No description" comment is produced.
 *
 * @param[in] state The Eolian state, used for resolving references.
 * @param[in] ev The Eolian event to document.
 * @param[in] group The documentation group name (e.g., for `@ingroup`). Can be NULL.
 *
 * @return A new Eina_Strbuf containing the generated documentation comment.
 *         The caller is responsible for freeing this buffer.
 */
Eina_Strbuf *eo_gen_docs_event_gen(const Eolian_State *state, const Eolian_Event *ev, const char *group);

#endif

