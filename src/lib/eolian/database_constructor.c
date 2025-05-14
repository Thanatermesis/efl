#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Frees the memory allocated for an Eolian_Constructor object.
 *
 * This function deallocates the memory for the Eolian_Constructor structure
 * itself, as well as any associated stringshared members like file and name.
 * If the provided constructor pointer is NULL, the function does nothing.
 *
 * @param ctor A pointer to the Eolian_Constructor object to be deleted.
 */
void
database_constructor_del(Eolian_Constructor *ctor)
{
   if (!ctor) return;
   eina_stringshare_del(ctor->base.file);
   eina_stringshare_del(ctor->base.name);
   free(ctor);
}
