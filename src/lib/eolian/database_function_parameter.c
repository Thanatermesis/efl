#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Frees the memory allocated for an Eolian_Function_Parameter.
 *
 * This function releases all resources associated with the given function
 * parameter descriptor, including its name, type, default value expression,
 * and documentation.
 *
 * @param pdesc The function parameter descriptor to delete.
 */
void
database_parameter_del(Eolian_Function_Parameter *pdesc)
{
   eina_stringshare_del(pdesc->base.file);
   eina_stringshare_del(pdesc->base.name);

   database_type_del(pdesc->type);
   database_expr_del(pdesc->value);
   database_doc_del(pdesc->doc);
   free(pdesc);
}
