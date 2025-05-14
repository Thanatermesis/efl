/**
 * @file
 * @brief This file implements the Efl_Ui_Text_Factory_Emoticons class,
 *        which is responsible for creating emoticons within text.
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include <Elementary_Cursor.h>
#include "elm_priv.h"

#define MY_CLASS EFL_UI_TEXT_FACTORY_EMOTICONS_CLASS

/**
 * @brief Private data for the Efl_Ui_Text_Factory_Emoticons class.
 * @since 1.24
 */
typedef struct _Efl_Ui_Text_Factory_Emoticons_Data Efl_Ui_Text_Factory_Emoticons_Data;

/**
 * @brief Structure holding the private data for the Efl_Ui_Text_Factory_Emoticons class.
 * @since 1.24
 */
struct _Efl_Ui_Text_Factory_Emoticons_Data
{
   /* Add any private data members here if needed in the future. */
};

/**
 * @brief Creates a canvas object (emoticon) for a given key.
 *
 * This function is called by the textblock infrastructure to create
 * a visual representation for an emoticon identified by a key (e.g., ":smile:").
 * It attempts to create a layout based on the provided key. If the key
 * is not found in the theme, it falls back to a default "wtf" emoticon.
 *
 * @param[in] obj The Efl_Ui_Text_Factory_Emoticons object.
 * @param[in] pd Private data for the Efl_Ui_Text_Factory_Emoticons object.
 * @param[in] object The parent Efl_Canvas_Object (usually the textblock).
 * @param[in] key The string key identifying the emoticon to create (e.g., "smile", "wink").
 *
 * @return A new Efl_Canvas_Layout object representing the emoticon, or @c NULL on failure.
 *         The returned object is owned by the caller and should be unreferenced when no longer needed.
 *
 * @since 1.24
 */
EOLIAN static Efl_Canvas_Object
*_efl_ui_text_factory_emoticons_efl_canvas_textblock_factory_create(
      Eo *obj EINA_UNUSED,
      Efl_Ui_Text_Factory_Emoticons_Data *pd EINA_UNUSED,
      Efl_Canvas_Object *object,
      const char *key)
{
   Eo *o;

   o = efl_add(EFL_CANVAS_LAYOUT_CLASS, object);
   if (elm_widget_element_update(object, o, key) == EFL_UI_THEME_APPLY_ERROR_GENERIC)
     {
        elm_widget_element_update(object, o, "wtf");
     }
   return o;
}

#include "efl_ui_text_factory_emoticons.eo.c"
