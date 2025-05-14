/**
 * @file
 * @brief Functions for parsing Eolian files and populating the Eolian database.
 */
#ifndef __EO_PARSER_H__
#define __EO_PARSER_H__

#include "eolian_api.h"
#include "eo_lexer.h"

/**
 * @brief Parses an Eolian file and populates the Eolian database.
 *
 * This function takes a parent Eolian_Unit, the filename of the Eolian file
 * to parse, and a boolean indicating if it's an EOT (Eolian Object Text) file.
 * It then uses the lexer to tokenize the file and parses the tokens to
 * construct Eolian objects (classes, types, constants, etc.), adding them
 * to the database.
 *
 * If the file has already been parsed and is present in the main or staging
 * units of the parent's state, it will be reused.
 *
 * @param parent The parent Eolian unit under which the parsed unit will be registered.
 *               Cannot be NULL.
 * @param filename The full path to the .eo or .eot file to parse.
 *                 Example: "/path/to/my_object.eo"
 * @param eot EINA_TRUE if the file is an .eot file (typically used for type
 *            definitions or constants without a full class), EINA_FALSE otherwise (for .eo files).
 * @return A pointer to the Eolian_Unit representing the parsed file, or NULL on error.
 *         The returned unit is owned by the Eolian_State and should not be freed
 *         directly by the caller.
 */
Eolian_Unit *eo_parser_database_fill(Eolian_Unit *parent, const char *filename, Eina_Bool eot);

#endif /* __EO_PARSER_H__ */
