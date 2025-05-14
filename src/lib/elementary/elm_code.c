#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Eo.h>
#include <Evas.h>

#include "Elementary.h"

#include "elm_code_private.h"

/**
 * @brief Event triggered when a single line has finished loading.
 *
 * This event is fired by Elm_Code when a line's content and associated metadata
 * (like syntax highlighting information) have been fully processed and are ready.
 * Widgets displaying the code can listen to this event to update their view
 * incrementally as lines are loaded.
 *
 * The event_info for this event is typically an Elm_Code_Line pointer.
 */
EAPI const Efl_Event_Description ELM_CODE_EVENT_LINE_LOAD_DONE =
    EFL_EVENT_DESCRIPTION("line,load,done");

/**
 * @brief Event triggered when the entire file has finished loading.
 *
 * This event is fired by Elm_Code when all lines in the associated file
 * have been loaded and processed. Widgets can use this event to perform
 * actions that require the full content to be available, such as final
 * layout adjustments or enabling certain features.
 *
 * The event_info for this event is typically NULL or a pointer to the Elm_Code_File.
 */
EAPI const Efl_Event_Description ELM_CODE_EVENT_FILE_LOAD_DONE =
    EFL_EVENT_DESCRIPTION("file,load,done");


EAPI Elm_Code *
elm_code_create(void)
{
   Elm_Code *ret;

   ret = calloc(1, sizeof(Elm_Code));
   if (!ret) return NULL;
   ret->config.indent_style_efl = EINA_TRUE;

   // create an in-memory backing for this elm_code by default
   elm_code_file_new(ret);
   return ret;
}

EAPI void
elm_code_free(Elm_Code *code)
{
   Evas_Object *widget;
   Elm_Code_Parser *parser;

   if (code->file)
     elm_code_file_free(code->file);

   EINA_LIST_FREE(code->widgets, widget)
     {
        evas_object_hide(widget);
        evas_object_del(widget);
     }

   EINA_LIST_FREE(code->parsers, parser)
     {
        _elm_code_parser_free(parser);
     }

   free(code);
}

EAPI void
elm_code_callback_fire(Elm_Code *code, const Efl_Event_Description *signal, void *data)
{
   Eina_List *item;
   Eo *widget;

   EINA_LIST_FOREACH(code->widgets, item, widget)
     {
        efl_event_callback_legacy_call(widget, signal, data);
     }
}

