#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_LAYOUT_PROTECTED
#define EFL_UI_SCROLL_MANAGER_PROTECTED
#define EFL_UI_SCROLLBAR_PROTECTED

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_layout.h"
#include "efl_ui_widget_scroller.h"

#include "efl_ui_internal_text_scroller.h"

#define EFL_UI_SCROLLER_DATA_GET(o, sd) \
  Efl_Ui_Scroller_Data * sd = efl_data_scope_safe_get(o, EFL_UI_SCROLLER_CLASS)

#define EFL_UI_SCROLLER_DATA_GET_OR_RETURN(o, ptr, ...) \
  EFL_UI_SCROLLER_DATA_GET(o, ptr);                         \
  if (EINA_UNLIKELY(!ptr))                            \
    {                                                 \
       ERR("No widget data for object %p (%s)",       \
           o, evas_object_type_get(o));               \
       return __VA_ARGS__;                                    \
    }
#define MY_CLASS EFL_UI_INTERNAL_TEXT_SCROLLER_CLASS
#define MY_CLASS_PFX efl_ui_internal_text_scroller

#define MY_CLASS_NAME "Efl.Ui.Internal_Text_Scroller"

/**
 * @brief Internal data structure for the Efl.Ui.Internal_Text_Scroller widget.
 *
 * This structure holds all the private data members needed for the
 * internal text scroller's operation.
 */
typedef struct _Efl_Ui_Internal_Text_Scroller_Data
{
   Efl_Canvas_Textblock *text_obj; /**< The textblock object displaying the text. */
   Efl_Ui_Table *text_table;       /**< The table used to layout the textblock. */
   Eo *smanager;                   /**< The scroll manager associated with this scroller. */

   Efl_Ui_Text_Scroller_Mode mode; /**< The current scrolling mode (single line or multiline). */

   Eina_Bool  match_content_w: 1;  /**< Flag to indicate if width should match content. (Currently unused) */
   Eina_Bool  match_content_h: 1;  /**< Flag to indicate if height should match content. (Currently unused) */
} Efl_Ui_Internal_Text_Scroller_Data;

#define EFL_UI_INTERNAL_TEXT_SCROLLER_DATA_GET(o, sd) \
  Efl_Ui_Internal_Text_Scroller_Data * sd = efl_data_scope_safe_get(o, EFL_UI_INTERNAL_TEXT_SCROLLER_CLASS)

#define EFL_UI_INTERNAL_TEXT_SCROLLER_DATA_GET_OR_RETURN(o, ptr, ...) \
  EFL_UI_INTERNAL_TEXT_SCROLLER_DATA_GET(o, ptr);                         \
  if (EINA_UNLIKELY(!ptr))                            \
    {                                                 \
       ERR("No widget data for object %p (%s)",       \
           o, evas_object_type_get(o));               \
       return __VA_ARGS__;                                    \
    }

/**
 * @brief Constructor for the Efl.Ui.Internal_Text_Scroller object.
 *
 * Initializes the internal text scroller, sets default scrollbar modes.
 *
 * @param obj The Efl.Ui.Internal_Text_Scroller object.
 * @param sd Private data for the Efl.Ui.Internal_Text_Scroller object.
 * @return The constructed Efl.Ui.Internal_Text_Scroller object.
 */
EOLIAN static Eo *
_efl_ui_internal_text_scroller_efl_object_constructor(Eo *obj,
                                        Efl_Ui_Internal_Text_Scroller_Data *sd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   efl_ui_scrollbar_bar_mode_set(obj,
         EFL_UI_SCROLLBAR_MODE_OFF, EFL_UI_SCROLLBAR_MODE_OFF);

   return obj;
}

/**
 * @brief Calculates the size of the internal text scroller content.
 *
 * This function is called when the group needs to recalculate its layout.
 * It determines the formatted size of the textblock and adjusts the
 * scroller's content size accordingly, based on the current scroller mode
 * (single-line or multi-line) and viewport dimensions.
 *
 * @param obj The Efl.Ui.Internal_Text_Scroller object.
 * @param sd Private data for the Efl.Ui.Internal_Text_Scroller object.
 */
EOLIAN static void
_efl_ui_internal_text_scroller_efl_canvas_group_group_calculate(Eo *obj,
      Efl_Ui_Internal_Text_Scroller_Data *sd)
{
   Eina_Size2D size = {-1, -1};
   Eina_Rect view = EINA_RECT(0, 0, 0, 0);
   Evas_Coord vmw = 0, vmh = 0;

   efl_canvas_group_need_recalculate_set(obj, EINA_FALSE);
   EFL_UI_SCROLLER_DATA_GET_OR_RETURN(obj, psd);

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);


   if (psd->smanager)
     {
        view = efl_ui_scrollable_viewport_geometry_get(psd->smanager);
     }

   edje_object_size_min_calc(wd->resize_obj, &vmw, &vmh);

   if (sd->text_obj)
     {
        Eina_Size2D fsz = EINA_SIZE2D(0, 0);
        Eina_Size2D sz = EINA_SIZE2D(0, 0);

        sz = efl_gfx_entity_size_get(sd->text_table);
        efl_event_freeze(sd->text_table);
        efl_event_freeze(sd->text_obj);
        efl_gfx_entity_size_set(sd->text_table, view.size);
        efl_gfx_entity_size_set(sd->text_obj, view.size);
        fsz = efl_canvas_textblock_size_formatted_get(sd->text_obj);
        efl_gfx_entity_size_set(sd->text_table, sz);
        efl_gfx_entity_size_set(sd->text_obj, sz);
        efl_event_thaw(sd->text_obj);
        efl_event_thaw(sd->text_table);


        if (sd->mode == EFL_UI_TEXT_SCROLLER_MODE_SINGLELINE)
          {
             size.h = vmh + fsz.h;
             if (fsz.w < view.w)
               {
                  fsz.w = view.w;
               }
          }
        else
          {
             if (fsz.h < view.h)
                fsz.h = view.h;

             if (fsz.w < view.w)
                fsz.w = view.w;
          }

        efl_gfx_entity_size_set(sd->text_table, fsz);
        efl_gfx_hint_size_restricted_min_set(obj, size);
     }
}

/**
 * @brief Finalizer for the Efl.Ui.Internal_Text_Scroller object.
 *
 * Completes the initialization of the internal text scroller.
 * Sets the text table as the content of the scroller and ensures
 * scrollbars are initially off (mode_set will adjust them later).
 *
 * @param obj The Efl.Ui.Internal_Text_Scroller object.
 * @param sd Private data for the Efl.Ui.Internal_Text_Scroller object.
 * @return The finalized Efl.Ui.Internal_Text_Scroller object.
 */
EOLIAN static Eo *
_efl_ui_internal_text_scroller_efl_object_finalize(Eo *obj,
                                     Efl_Ui_Internal_Text_Scroller_Data *sd)
{
   obj = efl_finalize(efl_super(obj, MY_CLASS));
   efl_ui_scrollbar_bar_mode_set(obj,
         EFL_UI_SCROLLBAR_MODE_OFF, EFL_UI_SCROLLBAR_MODE_OFF);
   efl_content_set(obj, sd->text_table);
   return obj;
}

/**
 * @brief Destructor for the Efl.Ui.Internal_Text_Scroller object.
 *
 * Cleans up resources used by the internal text scroller.
 *
 * @param obj The Efl.Ui.Internal_Text_Scroller object.
 * @param sd Private data for the Efl.Ui.Internal_Text_Scroller object.
 */
EOLIAN static void
_efl_ui_internal_text_scroller_efl_object_destructor(Eo *obj,
                                       Efl_Ui_Internal_Text_Scroller_Data *sd EINA_UNUSED)
{
   efl_destructor(efl_super(obj, MY_CLASS));
}

/**
 * @brief Initializes the internal text scroller with textblock and table.
 *
 * This function sets up the core components (textblock and table) for the scroller.
 * It should only be called during the construction phase of the object.
 *
 * @param obj The Efl.Ui.Internal_Text_Scroller object.
 * @param sd Private data for the Efl.Ui.Internal_Text_Scroller object.
 * @param text_obj The Efl_Canvas_Textblock object to be scrolled.
 * @param text_table The Efl_Ui_Table object used for laying out the textblock.
 */
EOLIAN static void
_efl_ui_internal_text_scroller_initialize(Eo *obj,
                                       Efl_Ui_Internal_Text_Scroller_Data *sd,
                                       Efl_Canvas_Textblock *text_obj,
                                       Efl_Ui_Table *text_table)
{
   if (efl_finalized_get(obj))
     {
        ERR("Can only be called on construction");
        return;
     }

   sd->text_obj = text_obj;
   sd->text_table = text_table;
}

/**
 * @brief Sets the scrolling mode for the internal text scroller.
 *
 * This function configures the scroller to operate in either single-line
 * or multi-line mode, adjusting scrollbar visibility accordingly.
 *
 * @param obj The Efl.Ui.Internal_Text_Scroller object.
 * @param sd Private data for the Efl.Ui.Internal_Text_Scroller object.
 * @param mode The desired Efl_Ui_Text_Scroller_Mode (EFL_UI_TEXT_SCROLLER_MODE_SINGLELINE or EFL_UI_TEXT_SCROLLER_MODE_MULTILINE).
 */
EOLIAN static void
_efl_ui_internal_text_scroller_scroller_mode_set(Eo *obj,
                                       Efl_Ui_Internal_Text_Scroller_Data *sd,
                                       Efl_Ui_Text_Scroller_Mode mode)
{
   sd->mode = mode;
   if (mode == EFL_UI_TEXT_SCROLLER_MODE_MULTILINE)
     {
        efl_ui_scrollbar_bar_mode_set(obj,
              EFL_UI_SCROLLBAR_MODE_AUTO, EFL_UI_SCROLLBAR_MODE_AUTO);
     }
   else // default (single-line)
     {
        efl_ui_scrollbar_bar_mode_set(obj,
              EFL_UI_SCROLLBAR_MODE_OFF, EFL_UI_SCROLLBAR_MODE_OFF);
     }
}

/**
 * @brief Gets the clip object for the scroller's viewport.
 *
 * Retrieves the Evas_Object that is used as the clipper for the scrollable
 * content area (pan_obj) of the scroller.
 *
 * @param obj The Efl.Ui.Internal_Text_Scroller object.
 * @param sd Private data for the Efl.Ui.Internal_Text_Scroller object. (Unused in this function)
 * @return The Evas_Object used as the viewport clip, or NULL on failure.
 */
EOLIAN static Eo *
_efl_ui_internal_text_scroller_viewport_clip_get(const Eo *obj,
      Efl_Ui_Internal_Text_Scroller_Data *sd EINA_UNUSED)
{
   EFL_UI_SCROLLER_DATA_GET_OR_RETURN(obj, psd, NULL);
   return evas_object_clip_get(psd->pan_obj);
}

/* Internal EO APIs and hidden overrides */

#include "efl_ui_internal_text_scroller.eo.c"
