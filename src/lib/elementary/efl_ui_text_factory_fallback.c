#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include <Elementary_Cursor.h>
#include "elm_priv.h"

#define MY_CLASS EFL_UI_TEXT_FACTORY_FALLBACK_CLASS

/**
 * @brief Private data for the Efl_Ui_Text_Factory_Fallback class.
 */
typedef struct _Efl_Ui_Text_Factory_Fallback_Data Efl_Ui_Text_Factory_Fallback_Data;

/**
 * @brief Private data structure for Efl_Ui_Text_Factory_Fallback.
 *
 * This structure holds references to other textblock factories
 * that this fallback factory will delegate to.
 */
struct _Efl_Ui_Text_Factory_Fallback_Data
{
   Efl_Canvas_Textblock_Factory *emoticon_factory; /**< Factory for handling emoticons. */
   Efl_Canvas_Textblock_Factory *image_factory;    /**< Factory for handling image files. */
};

/**
 * @brief Constructor for the Efl_Ui_Text_Factory_Fallback object.
 *
 * Initializes the fallback factory by creating instances of emoticon and image factories.
 *
 * @param obj The Efl_Ui_Text_Factory_Fallback object.
 * @param pd Private data for the object.
 * @return The constructed Efl_Ui_Text_Factory_Fallback object.
 */
EOLIAN static Eo *
_efl_ui_text_factory_fallback_efl_object_constructor(Eo *obj,
     Efl_Ui_Text_Factory_Fallback_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd->image_factory = efl_add(EFL_UI_TEXT_FACTORY_IMAGES_CLASS, obj);
   pd->emoticon_factory = efl_add(EFL_UI_TEXT_FACTORY_EMOTICONS_CLASS, obj);
   return obj;
}

/**
 * @brief Destructor for the Efl_Ui_Text_Factory_Fallback object.
 *
 * Cleans up resources used by the fallback factory.
 *
 * @param obj The Efl_Ui_Text_Factory_Fallback object.
 * @param pd Private data for the object.
 */
EOLIAN static void
_efl_ui_text_factory_fallback_efl_object_destructor(Eo *obj,
     Efl_Ui_Text_Factory_Fallback_Data *pd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Creates a canvas object based on a key string.
 *
 * This function acts as a fallback factory, attempting to create an object
 * first as an image file if the key starts with "file://", otherwise
 * as an emoticon. It delegates the actual object creation to specialized
 * factories (image_factory or emoticon_factory).
 *
 * @param obj The Efl_Ui_Text_Factory_Fallback object (unused).
 * @param pd Private data containing the delegate factories.
 * @param object The parent Efl_Canvas_Object for the new object.
 * @param key A string identifier for the object to create.
 *            Examples:
 *            - "file:///path/to/image.png" for an image.
 *            - "emoticon-name" for an emoticon (e.g., "smile").
 * @return A new Efl_Canvas_Object if successful, otherwise NULL.
 */
EOLIAN static Efl_Canvas_Object
*_efl_ui_text_factory_fallback_efl_canvas_textblock_factory_create(
      Eo *obj EINA_UNUSED,
      Efl_Ui_Text_Factory_Fallback_Data *pd,
      Efl_Canvas_Object *object,
      const char *key)
{
   Efl_Canvas_Object *o = NULL;

   // Parse the string. Can be either:
   //   1. some/name - an emoticon (load from theme)
   //   2. file:// - image file
   if (key && !strncmp(key, "file://", 7)) // Check if the key indicates a file path.
     {
        const char *fname = key + 7; // Extract the filename from the key.
        o = efl_canvas_textblock_factory_create(pd->image_factory, object, fname);
     }
   else
     {
        o = efl_canvas_textblock_factory_create(pd->emoticon_factory, object, key);
     }
   return o;
}

#include "efl_ui_text_factory_fallback.eo.c"
