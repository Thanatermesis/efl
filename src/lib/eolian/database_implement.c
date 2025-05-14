#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Frees an Eolian_Implement object and its associated data.
 *
 * This function deallocates the memory used by the given Eolian_Implement
 * structure. It also releases any stringshared members (file, name) and
 * any associated documentation (common_doc, get_doc, set_doc).
 *
 * @param impl Pointer to the Eolian_Implement object to be freed.
 *             If NULL, the function does nothing.
 */
void
database_implement_del(Eolian_Implement *impl)
{
   if (!impl) return;
   eina_stringshare_del(impl->base.file);
   eina_stringshare_del(impl->base.name);
   database_doc_del(impl->common_doc);
   database_doc_del(impl->get_doc);
   database_doc_del(impl->set_doc);
   free(impl);
}
