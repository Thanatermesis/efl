#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"

/**
 * @brief Frees the memory allocated for an Eolian_Event object.
 *
 * This function deallocates the given Eolian_Event and all its
 * associated data, including its name, file, type, and documentation.
 * If the provided event is NULL, the function does nothing.
 *
 * @param event The Eolian_Event to delete.
 */
void
database_event_del(Eolian_Event *event)
{
   if (!event) return;
   eina_stringshare_del(event->base.file);
   eina_stringshare_del(event->base.name);
   database_type_del(event->type);
   database_doc_del(event->doc);
   free(event);
}
