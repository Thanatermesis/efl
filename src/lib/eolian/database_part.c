#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Frees the memory allocated for an Eolian_Part object.
 *
 * This function releases all resources associated with the given Eolian_Part,
 * including its name, file path, and documentation.
 * If the part has not been validated (part->base.validated is false),
 * its klass_name is also freed.
 *
 * @param part The Eolian_Part object to delete. If NULL, the function does nothing.
 */
void
database_part_del(Eolian_Part *part)
{
   if (!part) return;
   eina_stringshare_del(part->base.file);
   eina_stringshare_del(part->base.name);
   if (!part->base.validated)
     eina_stringshare_del(part->klass_name);
   database_doc_del(part->doc);
   free(part);
}
