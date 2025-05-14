#ifndef EOLIAN_GEN_HEADERS_H
#define EOLIAN_GEN_HEADERS_H

#include "main.h"

/**
 * @brief Generates C code for a list of function parameters.
 *
 * Iterates over Eolian function parameters and calls _gen_param for each one,
 * appending the results to the provided string buffer. It also handles
 * comma separation between parameters.
 * The flagbuf logic for EINA_ARG_NONNULL is currently disabled.
 *
 * @param itr An iterator over Eolian_Function_Parameter objects.
 * @param buf The string buffer to append the generated C parameter list to.
 * @param flagbuf A pointer to a string buffer for EINA_ARG_NONNULL (currently unused).
 * @param nidx A pointer to an integer representing the current parameter index,
 *             which is incremented by this function.
 * @param ftype The Eolian function type, passed to _gen_param.
 */
void eo_gen_params(Eina_Iterator *itr, Eina_Strbuf *buf, Eina_Strbuf **flagbuf, int *nidx, Eolian_Function_Type ftype);

/**
 * @brief Generates the complete C header content for an Eolian class.
 *
 * This function orchestrates the generation of:
 * - Beta API guards for the entire class.
 * - Class documentation.
 * - The class macro definition (e.g., #define MY_CLASS_CLASS my_class_class_get()).
 * - The declaration for the class_get function (e.g., const Efl_Class *my_class_class_get(void)).
 * - Declarations for all methods and properties of the class (via _gen_func).
 * - Declarations and macro definitions for all events of the class.
 *
 * @param state The Eolian state.
 * @param cl The Eolian class for which to generate the header.
 * @param buf The string buffer to append the generated C header content to.
 */
void eo_gen_header_gen(const Eolian_State *state, const Eolian_Class *cl, Eina_Strbuf *buf);

#endif
