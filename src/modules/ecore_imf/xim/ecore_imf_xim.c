/**
 * @file
 * @brief Ecore IMF XIM module for X Input Method support.
 *
 * This file implements the Ecore IMF module for XIM, allowing
 * applications to use X Input Methods for text input. It handles
 * communication with the XIM server, manages input contexts (ICs),
 * and processes preedit and commit strings.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Input.h>
#include <Ecore_IMF.h>
#include <Ecore_X.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <langinfo.h>
#include <assert.h>

static int _ecore_imf_xim_log_dom = -1;

#ifdef CRI
#undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_ecore_imf_xim_log_dom, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_ecore_imf_xim_log_dom, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_ecore_imf_xim_log_dom, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_ecore_imf_xim_log_dom, __VA_ARGS__)


static Eina_List *open_ims = NULL;

#define FEEDBACK_MASK (XIMReverse | XIMUnderline | XIMHighlight)

typedef struct _XIM_Im_Info XIM_Im_Info;

typedef struct _Ecore_IMF_Context_Data Ecore_IMF_Context_Data;

/**
 * @brief Holds information related to an X Input Method (XIM) instance.
 *
 * This structure stores data for a specific XIM server connection,
 * including the associated window, locale, XIM object, and a list
 * of input contexts (ICs) using this IM. It also tracks XIM capabilities
 * like string conversion and cursor support.
 */
struct _XIM_Im_Info
{
   Ecore_X_Window win; ///< The Ecore_X_Window associated with this IM.
   Ecore_IMF_Context_Data *user; ///< User data, typically the active IMF context.
   char          *locale; ///< The locale string for this IM (e.g., "en_US.UTF-8").
   XIM            im; ///< The XIM object representing the connection to the XIM server.
   Eina_List     *ics; ///< A list of Ecore_IMF_Context_Data instances using this IM.
   Eina_Bool      reconnecting; ///< Flag indicating if a reconnection attempt is in progress.
   XIMStyles     *xim_styles; ///< Supported XIM input styles.
   Eina_Bool      supports_string_conversion : 1; ///< True if XNStringConversionCallback is supported.
   Eina_Bool      supports_cursor : 1; ///< True if XNCursor (for preedit cursor feedback) is supported.
};

/**
 * @brief Holds data specific to an Ecore IMF context using XIM.
 *
 * This structure contains all necessary information for managing an
 * input context (IC) with an XIM server. It includes the client window,
 * the XIC, preedit string data, focus state, and XIM callbacks.
 */
struct _Ecore_IMF_Context_Data
{
   Ecore_X_Window win; ///< The client Ecore_X_Window for this input context.
   long           mask; ///< Event mask used by the XIC for filtering events.
   XIC            ic; ///< The X Input Context (XIC) for composed characters.
   char          *locale; ///< Locale string for this context.
   XIM_Im_Info   *im_info; ///< Pointer to the XIM_Im_Info this context belongs to.
   int            preedit_length; ///< Current length of the preedit string (in characters).
   int            preedit_cursor; ///< Current cursor position within the preedit string.
   Eina_Unicode  *preedit_chars; ///< The preedit string as Eina_Unicode characters.
   Eina_Bool      use_preedit; ///< Flag indicating if preedit (on-the-spot) is used.
   Eina_Bool      finalizing; ///< Flag indicating if the context is being destroyed.
   Eina_Bool      has_focus; ///< Flag indicating if the context currently has input focus.
   Eina_Bool      in_toplevel; ///< Flag indicating if the client window is a toplevel window. (Currently unused)
   XIMFeedback   *feedbacks; ///< Array of XIMFeedback attributes for the preedit string.
                               ///< Example: {XIMUnderline, XIMHighlight, ...}

   XIMCallback    destroy_cb; ///< XIM callback for IC destruction.

   XIMCallback    preedit_start_cb; ///< XIM callback for preedit start.
   XIMCallback    preedit_done_cb; ///< XIM callback for preedit done.
   XIMCallback    preedit_draw_cb; ///< XIM callback for preedit draw (updates).
   XIMCallback    preedit_caret_cb; ///< XIM callback for preedit caret movement.
};

/* prototype */
/** @internal */
static Ecore_IMF_Context_Data *_ecore_imf_xim_context_data_new(void);
/** @internal */
static void                    _ecore_imf_xim_context_data_destroy(Ecore_IMF_Context_Data *imf_context_data);

/** @internal
 * @brief Adds a preedit attribute to a list based on XIMFeedback.
 * @param attrs Pointer to the Eina_List of Ecore_IMF_Preedit_Attr.
 * @param str The UTF-8 preedit string.
 * @param feedback The XIMFeedback style (e.g., XIMUnderline).
 * @param start_pos Start character offset in the preedit string.
 * @param end_pos End character offset in the preedit string.
 */
static void          _ecore_imf_xim_feedback_attr_add(Eina_List **attrs,
                                                     const char *str,
                                                     XIMFeedback feedback,
                                                     int start_pos,
                                                     int end_pos);

/** @internal
 * @brief Reinitializes the X Input Context (XIC) for the given IMF context.
 * This typically involves destroying the old XIC and creating a new one,
 * for example, when preedit usage changes.
 * @param ctx The Ecore IMF context.
 */
static void          _ecore_imf_xim_ic_reinitialize(Ecore_IMF_Context *ctx);
/** @internal
 * @brief Sets the client window for the X Input Context (XIC).
 * This function is distinct from the Ecore_IMF_Context_Class callback.
 * It manages the association of the XIC with a specific X window and
 * handles IM info updates.
 * @param ctx The Ecore IMF context.
 * @param window The Ecore_X_Window to associate with the IC.
 */
static void          _ecore_imf_xim_ic_client_window_set(Ecore_IMF_Context *ctx,
                                                        Ecore_X_Window window);
/** @internal
 * @brief XIM callback invoked when preedit starts.
 * @param xic The X Input Context.
 * @param client_data The Ecore_IMF_Context.
 * @param call_data XIM specific call data (unused).
 */
static void          _ecore_imf_xim_preedit_start_call(XIC xic,
                                                      XPointer client_data,
                                                      XPointer call_data);
/** @internal
 * @brief XIM callback invoked when preedit finishes.
 * @param xic The X Input Context.
 * @param client_data The Ecore_IMF_Context.
 * @param call_data XIM specific call data (unused).
 */
static void          _ecore_imf_xim_preedit_done_call(XIC xic,
                                                     XPointer client_data,
                                                     XPointer call_data);
/** @internal
 * @brief XIM callback invoked to draw/update the preedit string.
 * @param xic The X Input Context.
 * @param client_data The Ecore_IMF_Context.
 * @param call_data Structure containing preedit update information.
 */
static void          _ecore_imf_xim_preedit_draw_call(XIC xic,
                                                     XPointer client_data,
                                                     XIMPreeditDrawCallbackStruct *call_data);
/** @internal
 * @brief XIM callback invoked for preedit caret movement.
 * @param xic The X Input Context.
 * @param client_data The Ecore_IMF_Context.
 * @param call_data Structure containing caret movement information.
 */
static void          _ecore_imf_xim_preedit_caret_call(XIC xic,
                                                      XPointer client_data,
                                                      XIMPreeditCaretCallbackStruct *call_data);

/** @internal
 * @brief Converts XIMText (potentially multi-byte) to a UTF-8 string.
 * @param ctx The Ecore IMF context (unused).
 * @param xim_text The XIMText structure from Xlib.
 * @param[out] text Pointer to store the resulting UTF-8 string (must be freed by caller).
 * @return The length of the converted UTF-8 string in characters, or 0 on error/no text.
 */
static int           _ecore_imf_xim_text_to_utf8(Ecore_IMF_Context *ctx,
                                                XIMText *xim_text,
                                                char **text);

/** @internal
 * @brief Creates an XVaNestedList for setting up XIM preedit callbacks.
 * @param ctx The Ecore IMF context.
 * @return An XVaNestedList suitable for XSetICValues with XNPreeditCallbacks.
 *         The caller is responsible for freeing this list with XFree.
 */
static XVaNestedList _ecore_imf_xim_preedit_callback_set(Ecore_IMF_Context *ctx);
/** @internal
 * @brief Gets or creates the X Input Context (XIC) for the given IMF context.
 * @param ctx The Ecore IMF context.
 * @return The XIC, or NULL on failure.
 */
static XIC           _ecore_imf_xim_ic_get(Ecore_IMF_Context *ctx);
/** @internal
 * @brief Gets or creates an XIM_Im_Info structure for a given window and locale.
 * Manages a list of open IMs to reuse existing connections.
 * @param window The Ecore_X_Window associated with the IM.
 * @param locale The locale string.
 * @return A pointer to the XIM_Im_Info, or NULL on failure.
 */
static XIM_Im_Info  *_ecore_imf_xim_im_get(Ecore_X_Window window,
                                          char *locale);
/** @internal
 * @brief Initializes the XIM connection (info->im) within an XIM_Im_Info struct.
 * If XOpenIM fails, it registers an IM instantiate callback for later connection.
 * @param info The XIM_Im_Info structure to initialize.
 */
static void          _ecore_imf_xim_info_im_init(XIM_Im_Info *info);
/** @internal
 * @brief Shuts down an XIM connection and cleans up associated resources.
 * @param display The Ecore_X_Display (unused).
 * @param is_error Error flag (unused).
 * @param info The XIM_Im_Info structure to shut down.
 */
static void          _ecore_imf_xim_info_im_shutdown(Ecore_X_Display *display,
                                             int is_error,
                                             XIM_Im_Info *info);
/** @internal
 * @brief XIM callback invoked when an IM server becomes available.
 * @param display The X Display.
 * @param client_data The XIM_Im_Info that registered the callback.
 * @param call_data XIM specific call data (unused).
 */
static void          _ecore_imf_xim_instantiate_cb(Display *display,
                                                  XPointer client_data,
                                                  XPointer call_data);
/** @internal
 * @brief XIM callback invoked when an IM server connection is destroyed.
 * @param xim The XIM object (unused).
 * @param client_data The XIM_Im_Info associated with the IM.
 * @param call_data XIM specific call data (unused).
 */
static void          _ecore_imf_xim_destroy_cb(XIM xim,
                                              XPointer client_data,
                                              XPointer call_data);
/** @internal
 * @brief Sets up an XIM connection after it's established (e.g., queries capabilities).
 * @param info The XIM_Im_Info structure for the established IM.
 */
static void          _ecore_imf_xim_im_setup(XIM_Im_Info *info);

/** @internal Counter for ecore_x_init calls. */
static unsigned int init_count;

/**
 * @internal
 * @brief Converts a UTF-8 character offset to a byte index.
 *
 * This function iterates through a UTF-8 string to find the byte
 * position corresponding to a given character offset.
 *
 * @param str The UTF-8 encoded string.
 * @param offset The character offset (number of UTF-8 characters from the start).
 * @return The byte index corresponding to the character offset.
 */
static unsigned int
_ecore_imf_xim_utf8_offset_to_index(const char *str, int offset)
{
   int idx = 0;
   int i;
   for (i = 0; i < offset; i++)
     {
        eina_unicode_utf8_next_get(str, &idx);
     }

   return idx;
}

/**
 * @internal
 * @brief Adds XIM-specific data to an Ecore IMF context.
 *
 * This function is called when a new Ecore IMF context is created with
 * the XIM module. It allocates and initializes an Ecore_IMF_Context_Data
 * structure and sets it as the private data for the context.
 *
 * @param ctx The Ecore IMF context to which XIM data will be added.
 */
static void
_ecore_imf_context_xim_add(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Context_Data *imf_context_data = _ecore_imf_xim_context_data_new();
   DBG("ctx=%p, imf_context_data=%p", ctx, imf_context_data);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   imf_context_data->use_preedit = EINA_TRUE;
   imf_context_data->finalizing = EINA_FALSE;
   imf_context_data->has_focus = EINA_FALSE;
   imf_context_data->in_toplevel = EINA_FALSE;

   ecore_imf_context_data_set(ctx, imf_context_data);
}

/**
 * @internal
 * @brief Deletes XIM-specific data from an Ecore IMF context.
 *
 * This function is called when an Ecore IMF context using the XIM module
 * is being destroyed. It cleans up resources associated with the
 * Ecore_IMF_Context_Data, such as unregistering IM callbacks and
 * destroying the XIC.
 *
 * @param ctx The Ecore IMF context from which XIM data will be deleted.
 */
static void
_ecore_imf_context_xim_del(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   DBG("ctx=%p, imf_context_data=%p", ctx, imf_context_data);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   imf_context_data->finalizing = EINA_TRUE;
   if (imf_context_data->im_info && !imf_context_data->im_info->ics->next)
     {
        if (imf_context_data->im_info->reconnecting == EINA_TRUE)
          {
             Ecore_X_Display *dsp;
             dsp = ecore_x_display_get();
             if (dsp)
               XUnregisterIMInstantiateCallback(dsp,
                                                NULL, NULL, NULL,
                                                _ecore_imf_xim_instantiate_cb,
                                                (XPointer)imf_context_data->im_info);
          }
        else if (imf_context_data->im_info->im)
          {
             if (ecore_x_display_get())
               {
                  XIMCallback im_destroy_callback;
                  im_destroy_callback.client_data = NULL;
                  im_destroy_callback.callback = NULL;
                  XSetIMValues(imf_context_data->im_info->im,
                               XNDestroyCallback, &im_destroy_callback,
                               NULL);
               }
          }
     }

   _ecore_imf_xim_ic_client_window_set(ctx, 0);

   _ecore_imf_xim_context_data_destroy(imf_context_data);
}

/**
 * @internal
 * @brief Sets the client window for the Ecore IMF context.
 *
 * This is the Ecore_IMF_Context_Class callback function. It delegates
 * to the internal _ecore_imf_xim_ic_client_window_set to handle
 * XIC and IM association.
 *
 * @param ctx The Ecore IMF context.
 * @param window A pointer to an Ecore_X_Window (cast from void*).
 */
static void
_ecore_imf_context_xim_client_window_set(Ecore_IMF_Context *ctx,
                                         void *window)
{
   DBG("ctx=%p, window=%p", ctx, window);
   _ecore_imf_xim_ic_client_window_set(ctx, (Ecore_X_Window)((unsigned long)window));
}

/**
 * @internal
 * @brief Retrieves the current preedit string and cursor position.
 *
 * This function is part of the Ecore_IMF_Context_Class interface.
 * It converts the internally stored Eina_Unicode preedit string to UTF-8.
 *
 * @param ctx The Ecore IMF context.
 * @param[out] str Pointer to store the UTF-8 preedit string. The caller
 *                 receives ownership of this string and must free it.
 *                 Set to NULL if there is no preedit string.
 * @param[out] cursor_pos Pointer to store the character offset of the cursor
 *                        within the preedit string.
 */
static void
_ecore_imf_context_xim_preedit_string_get(Ecore_IMF_Context *ctx,
                                          char **str,
                                          int *cursor_pos)
{
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   char *utf8;
   int len;

   DBG("ctx=%p, imf_context_data=%p, str=%p, cursor_pos=%p",
       ctx, imf_context_data, str, cursor_pos);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   if (imf_context_data->preedit_chars)
     {
        utf8 = eina_unicode_unicode_to_utf8(imf_context_data->preedit_chars,
                                            &len);
        if (str)
          *str = utf8;
        else
          free(utf8);
     }
   else
     {
        if (str)
          *str = NULL;
     }

   if (cursor_pos)
     *cursor_pos = imf_context_data->preedit_cursor;
}

/**
 * @internal
 * @brief Retrieves the current preedit string, attributes, and cursor position.
 *
 * This function is part of the Ecore_IMF_Context_Class interface.
 * It gets the preedit string (as UTF-8) and cursor position, and then
 * converts XIMFeedback information into a list of Ecore_IMF_Preedit_Attr.
 *
 * @param ctx The Ecore IMF context.
 * @param[out] str Pointer to store the UTF-8 preedit string. The caller
 *                 receives ownership of this string and must free it.
 *                 Set to NULL if there is no preedit string.
 * @param[out] attrs Pointer to an Eina_List to store Ecore_IMF_Preedit_Attr
 *                   elements. The caller receives ownership of this list and
 *                   its contents, and must free them.
 *                   Example list structure:
 *                   `attrs -> [attr1, attr2, ...]`
 *                   where `attr1` is `Ecore_IMF_Preedit_Attr*` like:
 *                   `{ start_index=0, end_index=3, preedit_type=ECORE_IMF_PREEDIT_TYPE_SUB1 }`
 * @param[out] cursor_pos Pointer to store the character offset of the cursor
 *                        within the preedit string.
 */
static void
_ecore_imf_context_xim_preedit_string_with_attributes_get(Ecore_IMF_Context *ctx,
                                                          char **str,
                                                          Eina_List **attrs,
                                                          int *cursor_pos)
{
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);

   DBG("ctx=%p, imf_context_data=%p, str=%p, attrs=%p, cursor_pos=%p",
       ctx, imf_context_data, str, attrs, cursor_pos);

   _ecore_imf_context_xim_preedit_string_get(ctx, str, cursor_pos);

   if (!attrs) return;
   if (!imf_context_data || !imf_context_data->feedbacks) return;

   int i = 0;
   XIMFeedback last_feedback = 0;
   int start = -1;

   for (i = 0; i < imf_context_data->preedit_length; i++)
     {
        XIMFeedback new_feedback = imf_context_data->feedbacks[i] & FEEDBACK_MASK;

        if (new_feedback != last_feedback)
          {
             if (start >= 0)
               _ecore_imf_xim_feedback_attr_add(attrs, *str, last_feedback, start, i);

             last_feedback = new_feedback;
             start = i;
          }
     }

   if (start >= 0)
     _ecore_imf_xim_feedback_attr_add(attrs, *str, last_feedback, start, i);
}

/**
 * @internal
 * @brief Handles the focus-in event for the Ecore IMF context.
 *
 * This function is part of the Ecore_IMF_Context_Class interface.
 * It sets the XIC focus using XSetICFocus() and shows the input panel
 * if it's enabled.
 *
 * @param ctx The Ecore IMF context that gained focus.
 */
static void
_ecore_imf_context_xim_focus_in(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Context_Data *imf_context_data  = ecore_imf_context_data_get(ctx);
   XIC ic;

   DBG("ctx=%p, imf_context_data=%p", ctx, imf_context_data);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   ic = imf_context_data->ic;
   imf_context_data->has_focus = EINA_TRUE;

   if (ecore_imf_context_input_panel_enabled_get(ctx))
     ecore_imf_context_input_panel_show(ctx);

   if (ic)
     {
        char *str;

#ifdef X_HAVE_UTF8_STRING
        if ((str = Xutf8ResetIC(ic)))
#else
        if ((str = XmbResetIC(ic)))
#endif
          XFree(str);

        XSetICFocus(ic);
     }
}

/**
 * @internal
 * @brief Handles the focus-out event for the Ecore IMF context.
 *
 * This function is part of the Ecore_IMF_Context_Class interface.
 * It unsets the XIC focus using XUnsetICFocus() and hides the input panel
 * if it's enabled.
 *
 * @param ctx The Ecore IMF context that lost focus.
 */
static void
_ecore_imf_context_xim_focus_out(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   XIC ic;

   DBG("ctx=%p, imf_context_data=%p", ctx, imf_context_data);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   if (imf_context_data->has_focus == EINA_TRUE)
     {
        imf_context_data->has_focus = EINA_FALSE;
        ic = imf_context_data->ic;
        if (ic)
          XUnsetICFocus(ic);

        if (ecore_imf_context_input_panel_enabled_get(ctx))
          ecore_imf_context_input_panel_hide(ctx);
     }
}

/**
 * @internal
 * @brief Resets the Ecore IMF context.
 *
 * This function is part of the Ecore_IMF_Context_Class interface.
 * It resets the X Input Context (XIC) using XmbResetIC (or Xutf8ResetIC).
 * This typically clears any existing preedit string and resets the
 * input method's internal state. Any pending composition is committed.
 *
 * @param ctx The Ecore IMF context to reset.
 */
static void
_ecore_imf_context_xim_reset(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   XIC ic;
   char *result;
   /* restore conversion state after resetting ic later */
   XIMPreeditState preedit_state = XIMPreeditUnKnown;
   XVaNestedList preedit_attr;
   Eina_Bool have_preedit_state = EINA_FALSE;

   DBG("ctx=%p, imf_context_data=%p", ctx, imf_context_data);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   ic = imf_context_data->ic;
   if (!ic)
     return;

   if (imf_context_data->preedit_length == 0)
     return;

   preedit_attr = XVaCreateNestedList(0,
                                      XNPreeditState, &preedit_state,
                                      NULL);
   if (!XGetICValues(ic,
                     XNPreeditAttributes, preedit_attr,
                     NULL))
     have_preedit_state = EINA_TRUE;

   XFree(preedit_attr);

   result = XmbResetIC(ic);

   preedit_attr = XVaCreateNestedList(0,
                                      XNPreeditState, preedit_state,
                                      NULL);
   if (have_preedit_state)
     XSetICValues(ic,
                  XNPreeditAttributes, preedit_attr,
                  NULL);

   XFree(preedit_attr);

   if (imf_context_data->feedbacks)
     {
        free(imf_context_data->feedbacks);
        imf_context_data->feedbacks = NULL;
     }

   if (imf_context_data->preedit_length)
     {
        imf_context_data->preedit_length = 0;
        free(imf_context_data->preedit_chars);
        imf_context_data->preedit_chars = NULL;

        ecore_imf_context_event_callback_call(ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
     }

   if (result)
     {
        char *result_utf8 = strdup(result);
        if (result_utf8)
          {
             ecore_imf_context_event_callback_call(ctx, ECORE_IMF_CALLBACK_COMMIT, result_utf8);
             free(result_utf8);
          }
     }

   XFree(result);
}

/**
 * @internal
 * @brief Sets whether to use on-the-spot preedit for the context.
 *
 * This function is part of the Ecore_IMF_Context_Class interface.
 * If the `use_preedit` state changes, the X Input Context (XIC) is
 * reinitialized to reflect the new preedit style (e.g., XIMPreeditCallbacks
 * vs. XIMPreeditNothing).
 *
 * @param ctx The Ecore IMF context.
 * @param use_preedit EINA_TRUE to enable on-the-spot preedit, EINA_FALSE otherwise.
 */
static void
_ecore_imf_context_xim_use_preedit_set(Ecore_IMF_Context *ctx,
                                       Eina_Bool use_preedit)
{
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);

   DBG("ctx=%p, imf_context_data=%p, use_preedit=%hhu", ctx, imf_context_data, use_preedit);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   use_preedit = use_preedit != EINA_FALSE;

   if (imf_context_data->use_preedit != use_preedit)
     {
        imf_context_data->use_preedit = use_preedit;
        _ecore_imf_xim_ic_reinitialize(ctx);
     }
}

static void
_ecore_imf_xim_feedback_attr_add(Eina_List **attrs,
                                const char *str,
                                XIMFeedback feedback,
                                int start_pos,
                                int end_pos)
{
   Ecore_IMF_Preedit_Attr *attr;
   // Convert character offsets to byte indices for UTF-8 string
   unsigned int start_index = _ecore_imf_xim_utf8_offset_to_index(str, start_pos);
   unsigned int end_index = _ecore_imf_xim_utf8_offset_to_index(str, end_pos);

   if (feedback & FEEDBACK_MASK)
     {
        attr = calloc(1, sizeof(Ecore_IMF_Preedit_Attr));
        attr->start_index = start_index;
        attr->end_index = end_index;
        *attrs = eina_list_append(*attrs, attr);
     }
   else
     return;

   if (feedback & XIMUnderline)
     attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB1;

   if (feedback & XIMReverse)
     attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB2;

   if (feedback & XIMHighlight)
     attr->preedit_type = ECORE_IMF_PREEDIT_TYPE_SUB3;
}

/**
 * @internal
 * @brief Sets the cursor location for the Ecore IMF context.
 *
 * This function is part of the Ecore_IMF_Context_Class interface.
 * It informs the XIM server about the current cursor (caret) position
 * and dimensions. This is used for "over-the-spot" preedit style,
 * where the IM needs to draw the preedit string near the cursor.
 * The spot location is typically set to the bottom-left of the cursor area.
 *
 * @param ctx The Ecore IMF context.
 * @param x The x-coordinate of the cursor rectangle.
 * @param y The y-coordinate of the cursor rectangle.
 * @param w The width of the cursor rectangle.
 * @param h The height of the cursor rectangle.
 */
static void
_ecore_imf_context_xim_cursor_location_set(Ecore_IMF_Context *ctx,
                                           int x, int y, int w, int h)
{
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   XIC ic;
   XVaNestedList preedit_attr;
   XPoint spot;

   DBG("ctx=%p, imf_context_data=%p, location=(%d, %d, %d, %d)",
       ctx, imf_context_data, x, y, w, h);

   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);
   ic = imf_context_data->ic;
   if (!ic)
     return;

   spot.x = x;
   spot.y = y + h;

   preedit_attr = XVaCreateNestedList(0,
                                      XNSpotLocation, &spot,
                                      NULL);
   XSetICValues(ic,
                XNPreeditAttributes, preedit_attr,
                NULL);

   XFree(preedit_attr);
}

/**
 * @internal
 * @brief Shows the input panel (e.g., virtual keyboard).
 *
 * This function is part of the Ecore_IMF_Context_Class interface.
 * It requests the virtual keyboard to be shown for the associated window.
 *
 * @param ctx The Ecore IMF context.
 */
static void
_ecore_imf_context_xim_input_panel_show(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   DBG("ctx=%p, imf_context_data=%p", ctx, imf_context_data);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   if (!ecore_x_display_get()) return;
   ecore_x_e_virtual_keyboard_state_set
        (imf_context_data->win, ECORE_X_VIRTUAL_KEYBOARD_STATE_ON);
}

/**
 * @internal
 * @brief Hides the input panel (e.g., virtual keyboard).
 *
 * This function is part of the Ecore_IMF_Context_Class interface.
 * It requests the virtual keyboard to be hidden for the associated window.
 *
 * @param ctx The Ecore IMF context.
 */
static void
_ecore_imf_context_xim_input_panel_hide(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   DBG("ctx=%p, imf_context_data=%p", ctx, imf_context_data);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   if (!ecore_x_display_get()) return;
   ecore_x_e_virtual_keyboard_state_set
        (imf_context_data->win, ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF);
}

/**
 * @internal
 * @brief Converts Ecore_IMF keyboard modifiers to Ecore_X modifiers.
 *
 * This function maps modifier flags from the Ecore_IMF_KEYBOARD_MODIFIER_*
 * enum to the corresponding ECORE_X_MODIFIER_* flags. This is used when
 * constructing XKeyEvent structures from Ecore_IMF_Event data.
 *
 * @param state A bitmask of Ecore_IMF_KEYBOARD_MODIFIER_* flags.
 *              Example: ECORE_IMF_KEYBOARD_MODIFIER_CTRL | ECORE_IMF_KEYBOARD_MODIFIER_SHIFT
 * @return A bitmask of ECORE_X_MODIFIER_* flags.
 *         Example: ECORE_X_MODIFIER_CTRL | ECORE_X_MODIFIER_SHIFT
 */
static unsigned int
_ecore_x_event_reverse_modifiers(unsigned int state)
{
   unsigned int modifiers = 0;

   /**< "Control" is pressed */
   if (state & ECORE_IMF_KEYBOARD_MODIFIER_CTRL)
     modifiers |= ECORE_X_MODIFIER_CTRL;

   /**< "Alt" is pressed */
   if (state & ECORE_IMF_KEYBOARD_MODIFIER_ALT)
     modifiers |= ECORE_X_MODIFIER_ALT;

   /**< "Shift" is pressed */
   if (state & ECORE_IMF_KEYBOARD_MODIFIER_SHIFT)
     modifiers |= ECORE_X_MODIFIER_SHIFT;

   /**< "Win" (between "Ctrl" and "Alt") is pressed */
   if (state & ECORE_IMF_KEYBOARD_MODIFIER_WIN)
     modifiers |= ECORE_X_MODIFIER_WIN;

   /**< "AltGr" is pressed */
   if (state & ECORE_IMF_KEYBOARD_MODIFIER_ALTGR)
     modifiers |= ECORE_X_MODIFIER_ALTGR;

   return modifiers;
}

/**
 * @internal
 * @brief Converts Ecore_IMF keyboard locks to Ecore_X locks.
 *
 * This function maps lock key flags from the ECORE_IMF_KEYBOARD_LOCK_*
 * enum to the corresponding ECORE_X_LOCK_* flags. This is used when
 * constructing XKeyEvent structures from Ecore_IMF_Event data.
 *
 * @param state A bitmask of ECORE_IMF_KEYBOARD_LOCK_* flags.
 *              Example: ECORE_IMF_KEYBOARD_LOCK_CAPS
 * @return A bitmask of ECORE_X_LOCK_* flags.
 *         Example: ECORE_X_LOCK_CAPS
 */
static unsigned int
_ecore_x_event_reverse_locks(unsigned int state)
{
   unsigned int locks = 0;

   /**< "Num" lock is active */
   if (state & ECORE_IMF_KEYBOARD_LOCK_NUM)
     locks |= ECORE_X_LOCK_NUM;

   if (state & ECORE_IMF_KEYBOARD_LOCK_CAPS)
     locks |= ECORE_X_LOCK_CAPS;

   if (state & ECORE_IMF_KEYBOARD_LOCK_SCROLL)
     locks |= ECORE_X_LOCK_SCROLL;

   return locks;
}

/**
 * @internal
 * @brief Gets an X KeyCode from a key name string.
 *
 * This function converts a human-readable key name (e.g., "space", "a", "Shift_L")
 * into an X KeyCode. It handles a special case for "Keycode-0".
 *
 * @param dsp The Ecore_X_Display.
 * @param keyname The string representation of the key (e.g., "Return", "Control_L").
 * @return The X KeyCode corresponding to the keyname, or 0 if not found (except for "Keycode-0").
 */
static KeyCode
_ecore_imf_xim_keycode_get(Ecore_X_Display *dsp,
                           const char *keyname)
{
   KeyCode keycode;

   //DBG("keyname=%s keysym=%lu", keyname, XStringToKeysym(keyname));
   if (strcmp(keyname, "Keycode-0") == 0)
     keycode = 0;
   else
     keycode = XKeysymToKeycode(dsp, XStringToKeysym(keyname));

   return keycode;
}

/**
 * @internal
 * @brief Filters an Ecore IMF event and forwards it to the XIM server if necessary.
 *
 * This function is part of the Ecore_IMF_Context_Class interface.
 * It currently only handles ECORE_IMF_EVENT_KEY_DOWN events.
 * For key down events, it constructs an XKeyPressedEvent and passes it to
 * Xutf8LookupString (or XmbLookupString) to be processed by the XIM.
 * If the XIM produces a composed string (commit string), the
 * ECORE_IMF_CALLBACK_COMMIT event is triggered.
 *
 * @param ctx The Ecore IMF context.
 * @param type The type of the Ecore IMF event (e.g., ECORE_IMF_EVENT_KEY_DOWN).
 * @param event A pointer to the Ecore_IMF_Event structure.
 *              Example for ECORE_IMF_EVENT_KEY_DOWN:
 *              `Ecore_IMF_Event_Key_Down *ev = (Ecore_IMF_Event_Key_Down *)event;`
 *              `ev->keyname` (e.g., "a"), `ev->compose` (e.g., "a"),
 *              `ev->modifiers` (e.g., ECORE_IMF_KEYBOARD_MODIFIER_SHIFT).
 * @return EINA_TRUE if the event was consumed by the XIM (resulting in a commit or preedit change),
 *         EINA_FALSE otherwise (event should be processed further by the application).
 */
static Eina_Bool
_ecore_imf_context_xim_filter_event(Ecore_IMF_Context *ctx,
                                    Ecore_IMF_Event_Type type,
                                    Ecore_IMF_Event *event)
{
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   XIC ic;
   Ecore_X_Display *dsp;
   Ecore_X_Window win;

   int val;
   char compose_buffer[256];
   KeySym sym;
   char *compose = NULL;
   char *tmp = NULL;
   Eina_Bool result = EINA_FALSE;

   DBG("ctx=%p, imf_context_data=%p, type=%d, event=%p",
       ctx, imf_context_data, type, event);
   EINA_SAFETY_ON_NULL_RETURN_VAL(imf_context_data, EINA_FALSE);
   ic = imf_context_data->ic;
   if (!ic)
     ic = _ecore_imf_xim_ic_get(ctx);

   if (type == ECORE_IMF_EVENT_KEY_DOWN)
     {
        XKeyPressedEvent xev;
        Ecore_IMF_Event_Key_Down *ev = (Ecore_IMF_Event_Key_Down *)event;
        DBG("ECORE_IMF_EVENT_KEY_DOWN");

        dsp = ecore_x_display_get();
        win = imf_context_data->win;

        xev.type = KeyPress;
        xev.serial = 0; /* hope it doesn't matter */
        xev.send_event = 0;
        xev.display = dsp;
        xev.window = win;
        xev.root = dsp ? ecore_x_window_root_get(win) : 0;
        xev.subwindow = win;
        xev.time = ev->timestamp;
        xev.x = xev.x_root = 0;
        xev.y = xev.y_root = 0;
        xev.state = 0;
        if (dsp)
          {
             xev.state |= _ecore_x_event_reverse_modifiers(ev->modifiers);
             xev.state |= _ecore_x_event_reverse_locks(ev->locks);
             xev.keycode = _ecore_imf_xim_keycode_get(dsp, ev->keyname);
          }
        xev.same_screen = True;

        if (ic)
          {
             Status mbstatus;
#ifdef X_HAVE_UTF8_STRING
             val = Xutf8LookupString(ic,
                                     &xev,
                                     compose_buffer,
                                     sizeof(compose_buffer) - 1,
                                     &sym,
                                     &mbstatus);
#else /* ifdef X_HAVE_UTF8_STRING */
             val = XmbLookupString(ic,
                                   &xev,
                                   compose_buffer,
                                   sizeof(compose_buffer) - 1,
                                   &sym,
                                   &mbstatus);
#endif /* ifdef X_HAVE_UTF8_STRING */
             if (mbstatus == XBufferOverflow)
               {
                  tmp = malloc(sizeof (char) * (val + 1));
                  if (!tmp)
                    return EINA_FALSE;

                  compose = tmp;

#ifdef X_HAVE_UTF8_STRING
                  val = Xutf8LookupString(ic,
                                          &xev,
                                          tmp,
                                          val,
                                          &sym,
                                          &mbstatus);
#else /* ifdef X_HAVE_UTF8_STRING */
                  val = XmbLookupString(ic,
                                        &xev,
                                        tmp,
                                        val,
                                        &sym,
                                        &mbstatus);
#endif /* ifdef X_HAVE_UTF8_STRING */
                  if (val > 0)
                    {
                       tmp[val] = '\0';
#ifndef X_HAVE_UTF8_STRING
                       compose = eina_str_convert(nl_langinfo(CODESET),
                                                  "UTF-8", tmp);
                       free(tmp);
                       tmp = compose;
#endif /* ifndef X_HAVE_UTF8_STRING */
                    }
                  else
                    compose = NULL;
               }
             else if (val > 0)
               {
                  compose_buffer[val] = '\0';
#ifdef X_HAVE_UTF8_STRING
                  compose = strdup(compose_buffer);
#else /* ifdef X_HAVE_UTF8_STRING */
                  compose = eina_str_convert(nl_langinfo(CODESET), "UTF-8",
                                             compose_buffer);
#endif /* ifdef X_HAVE_UTF8_STRING */
               }
          }
        else
          {
             compose = strdup(ev->compose);
          }

        if (compose)
          {
             Eina_Unicode *unicode;
             int len;
             unicode = eina_unicode_utf8_to_unicode(compose, &len);
             if (!unicode) abort();
             if (unicode[0] >= 0x20 && unicode[0] != 0x7f)
               {
                  ecore_imf_context_event_callback_call(ctx, ECORE_IMF_CALLBACK_COMMIT, compose);
                  result = EINA_TRUE;
               }
             free(compose);
             free(unicode);
          }
     }

   return result;
}

static const Ecore_IMF_Context_Info xim_info = {
   .id = "xim",
   .description = "X input method",
   .default_locales = "ko:ja:th:zh",
   .canvas_type = "evas",
   .canvas_required = 1,
};

static Ecore_IMF_Context_Class xim_class = {
   .add = _ecore_imf_context_xim_add,
   .del = _ecore_imf_context_xim_del,
   .client_window_set = _ecore_imf_context_xim_client_window_set,
   .client_canvas_set = NULL,
   .show = _ecore_imf_context_xim_input_panel_show,
   .hide = _ecore_imf_context_xim_input_panel_hide,
   .preedit_string_get = _ecore_imf_context_xim_preedit_string_get,
   .focus_in = _ecore_imf_context_xim_focus_in,
   .focus_out = _ecore_imf_context_xim_focus_out,
   .reset = _ecore_imf_context_xim_reset,
   .cursor_position_set = NULL,
   .use_preedit_set = _ecore_imf_context_xim_use_preedit_set,
   .input_mode_set = NULL,
   .filter_event = _ecore_imf_context_xim_filter_event,
   .preedit_string_with_attributes_get = _ecore_imf_context_xim_preedit_string_with_attributes_get,
   .prediction_allow_set = NULL,
   .autocapital_type_set = NULL,
   .control_panel_show = NULL,
   .control_panel_hide = NULL,
   .input_panel_layout_set = NULL,
   .input_panel_layout_get = NULL,
   .input_panel_language_set = NULL,
   .input_panel_language_get = NULL,
   .cursor_location_set = _ecore_imf_context_xim_cursor_location_set,
   .input_panel_imdata_set = NULL,
   .input_panel_imdata_get = NULL,
   .input_panel_return_key_type_set = NULL,
   .input_panel_return_key_disabled_set = NULL,
   .input_panel_caps_lock_mode_set = NULL
};

/**
 * @internal
 * @brief Creates a new XIM Ecore IMF context.
 *
 * This function is registered with Ecore IMF as the module's context
 * creation function. It initializes Ecore_X if not already initialized
 * and creates a new Ecore_IMF_Context using the xim_class.
 *
 * @return A new Ecore_IMF_Context instance, or NULL on failure.
 */
static Ecore_IMF_Context *
xim_imf_module_create(void)
{
   Ecore_IMF_Context *ctx;

   if (!ecore_x_init(NULL))
     return NULL;
   init_count++;
   ctx = ecore_imf_context_new(&xim_class);
   DBG("ctx=%p", ctx);
   return ctx;
}

/**
 * @internal
 * @brief Cleans up resources when the XIM Ecore IMF module exits.
 *
 * This function is registered with Ecore IMF as the module's exit
 * function. It decrements the ecore_x_init counter and shuts down
 * Ecore_X if the counter reaches zero.
 *
 * @return Always NULL.
 */
static Ecore_IMF_Context *
xim_imf_module_exit(void)
{
   if (init_count)
     {
        ecore_x_shutdown();
        init_count--;
     }
   DBG(" ");
   return NULL;
}

/**
 * @internal
 * @brief Initializes the Ecore IMF XIM module.
 *
 * This function is called by EINA_MODULE_INIT. It checks for necessary
 * environment conditions (DISPLAY variable, ELM_DISPLAY setting),
 * initializes Eina, registers a log domain, and registers the XIM
 * module with Ecore IMF.
 *
 * @return EINA_TRUE on successful initialization, EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_imf_xim_init(void)
{
   const char *s;

   if (!getenv("DISPLAY")) return EINA_FALSE;
   if ((s = getenv("ELM_DISPLAY")))
     {
        if (strcmp(s, "x11")) return EINA_FALSE;
     }
   eina_init();

   _ecore_imf_xim_log_dom = eina_log_domain_register("ecore_imf_xim", NULL);
   if (_ecore_imf_xim_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: ecore_imf_xim");
     }

   DBG(" ");

   ecore_imf_module_register(&xim_info,
                             xim_imf_module_create,
                             xim_imf_module_exit);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Shuts down the Ecore IMF XIM module.
 *
 * This function is called by EINA_MODULE_SHUTDOWN. It cleans up any
 * open XIM connections, unregisters the log domain, and shuts down Eina.
 */
static void
_ecore_imf_xim_shutdown(void)
{
   if (open_ims)
     {
        XIM_Im_Info *info = open_ims->data;
        Ecore_X_Display *display = ecore_x_display_get();

        if (display)
          _ecore_imf_xim_info_im_shutdown(display, EINA_FALSE, info);
     }

   if (_ecore_imf_xim_log_dom >= 0)
     {
        eina_log_domain_unregister(_ecore_imf_xim_log_dom);
        _ecore_imf_xim_log_dom = -1;
     }

   eina_shutdown();
}

EINA_MODULE_INIT(_ecore_imf_xim_init);
EINA_MODULE_SHUTDOWN(_ecore_imf_xim_shutdown);

/*
 * internal functions
 */
/**
 * @internal
 * @brief Allocates and initializes a new Ecore_IMF_Context_Data structure.
 *
 * This function checks if X locales are supported and retrieves the current
 * LC_CTYPE locale. It then allocates memory for Ecore_IMF_Context_Data
 * and duplicates the locale string.
 *
 * @return A pointer to the newly allocated Ecore_IMF_Context_Data,
 *         or NULL on failure (e.g., locale not supported, memory allocation failed).
 */
static Ecore_IMF_Context_Data *
_ecore_imf_xim_context_data_new(void)
{
   Ecore_IMF_Context_Data *imf_context_data = NULL;
   char *locale;

   locale = setlocale(LC_CTYPE, "");
   if (!locale) return NULL;

   if (!XSupportsLocale()) return NULL;

   imf_context_data = calloc(1, sizeof(Ecore_IMF_Context_Data));
   EINA_SAFETY_ON_NULL_RETURN_VAL(imf_context_data, NULL);

   imf_context_data->locale = strdup(locale);
   if (!imf_context_data->locale) goto error;

   return imf_context_data;
error:
   _ecore_imf_xim_context_data_destroy(imf_context_data);
   return NULL;
}

/**
 * @internal
 * @brief Frees an Ecore_IMF_Context_Data structure and its associated resources.
 *
 * This function destroys the X Input Context (XIC) if it exists, frees
 * the preedit string, feedback attributes, locale string, and finally
 * the Ecore_IMF_Context_Data structure itself.
 *
 * @param imf_context_data Pointer to the Ecore_IMF_Context_Data to destroy.
 */
static void
_ecore_imf_xim_context_data_destroy(Ecore_IMF_Context_Data *imf_context_data)
{
   if (!imf_context_data)
     return;

   if (imf_context_data->ic)
     XDestroyIC(imf_context_data->ic);

   free(imf_context_data->preedit_chars);

   if (imf_context_data->feedbacks)
     {
        free(imf_context_data->feedbacks);
        imf_context_data->feedbacks = NULL;
     }

   free(imf_context_data->locale);
   free(imf_context_data);
}

/**
 * @internal
 * @brief XIM callback: Preedit Start.
 *
 * This function is invoked by the XIM server when a preedit sequence begins.
 * It triggers the ECORE_IMF_CALLBACK_PREEDIT_START event on the
 * Ecore IMF context, unless the context is currently being finalized.
 *
 * @param xic The X Input Context (unused).
 * @param client_data A pointer to the Ecore_IMF_Context.
 * @param call_data XIM-specific callback data (unused).
 */
static void
_ecore_imf_xim_preedit_start_call(XIC xic EINA_UNUSED,
                                 XPointer client_data,
                                 XPointer call_data EINA_UNUSED)
{
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)client_data;
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);

   DBG("ctx=%p, imf_context_data=%p", ctx, imf_context_data);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   if (imf_context_data->finalizing == EINA_FALSE)
     ecore_imf_context_event_callback_call(ctx, ECORE_IMF_CALLBACK_PREEDIT_START, NULL);
}

/**
 * @internal
 * @brief XIM callback: Preedit Done.
 *
 * This function is invoked by the XIM server when a preedit sequence ends.
 * It clears the internal preedit string and triggers the
 * ECORE_IMF_CALLBACK_PREEDIT_CHANGED and ECORE_IMF_CALLBACK_PREEDIT_END
 * events on the Ecore IMF context, unless the context is finalizing.
 *
 * @param xic The X Input Context (unused).
 * @param client_data A pointer to the Ecore_IMF_Context.
 * @param call_data XIM-specific callback data (unused).
 */
static void
_ecore_imf_xim_preedit_done_call(XIC xic EINA_UNUSED,
                                XPointer client_data,
                                XPointer call_data EINA_UNUSED)
{
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)client_data;
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);

   DBG("ctx=%p, imf_context_data=%p", ctx, imf_context_data);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   if (imf_context_data->preedit_length)
     {
        imf_context_data->preedit_length = 0;
        free(imf_context_data->preedit_chars);
        imf_context_data->preedit_chars = NULL;
        ecore_imf_context_event_callback_call(ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
     }

   if (imf_context_data->finalizing == EINA_FALSE)
     ecore_imf_context_event_callback_call(ctx, ECORE_IMF_CALLBACK_PREEDIT_END, NULL);
}

/* FIXME: This function has a note about wide character support and potential size mismatches. */
/**
 * @internal
 * @brief Converts XIMText to a UTF-8 string.
 *
 * This function takes an XIMText structure, which might contain multi-byte
 * or wide character strings, and attempts to convert it into a UTF-8 encoded
 * C string. It currently warns if wide characters are encountered as they
 * are not fully supported.
 *
 * @param ctx The Ecore IMF context (currently unused).
 * @param xim_text A pointer to the XIMText structure received from the XIM server.
 *                 Example: `xim_text->string.multi_byte` or `xim_text->string.wide_char`.
 *                          `xim_text->encoding_is_wchar` indicates the type.
 *                          `xim_text->length` is the length in bytes or wide chars.
 * @param[out] text A pointer to a char* where the newly allocated UTF-8 string
 *                  will be stored. The caller is responsible for freeing this string.
 *                  Set to NULL on failure or if xim_text is NULL/empty.
 * @return The length of the converted UTF-8 string in characters (not bytes),
 *         or 0 if no text was converted or an error occurred.
 */
static int
_ecore_imf_xim_text_to_utf8(Ecore_IMF_Context *ctx EINA_UNUSED,
                           XIMText *xim_text,
                           char **text)
{
   int text_length = 0;
   char *result = NULL;

   if (xim_text && xim_text->string.multi_byte)
     {
        if (xim_text->encoding_is_wchar)
          {
             WRN("Wide character return from Xlib not currently supported");
             *text = NULL;
             return 0;
          }

        /* XXX Convert to UTF-8 */
        result = strdup(xim_text->string.multi_byte);
        if (result)
          {
             text_length = eina_unicode_utf8_get_len(result);
             if (text_length != xim_text->length)
               {
                  WRN("Size mismatch when converting text from input method: supplied length = %d, result length = %d",
                      xim_text->length, text_length);
               }
          }
        else
          {
             WRN("Error converting text from IM to UCS-4");
             *text = NULL;
             return 0;
          }

        *text = result;
        return text_length;
     }
   else
     {
        *text = NULL;
        return 0;
     }
}

/**
 * @internal
 * @brief XIM callback: Preedit Draw.
 *
 * This function is invoked by the XIM server when the preedit string needs
 * to be updated (e.g., characters inserted, deleted, or replaced).
 * It modifies the internal Eina_Unicode preedit string based on the
 * information in `call_data` (caret position, changed range, new text).
 * It also updates the XIMFeedback attributes for the preedit string.
 * Finally, it triggers the ECORE_IMF_CALLBACK_PREEDIT_CHANGED event.
 *
 * @param xic The X Input Context (unused).
 * @param client_data A pointer to the Ecore_IMF_Context.
 * @param call_data A pointer to XIMPreeditDrawCallbackStruct containing details
 *                  about the change:
 *                  `call_data->caret`: new caret position (character offset).
 *                  `call_data->chg_first`: starting character offset of the change.
 *                  `call_data->chg_length`: number of characters to be replaced/deleted.
 *                  `call_data->text`: an XIMText structure containing the new text
 *                                     to insert/replace, or NULL if deleting.
 *                                     `call_data->text->feedback` contains an array
 *                                     of XIMFeedback attributes for the new text.
 */
static void
_ecore_imf_xim_preedit_draw_call(XIC xic EINA_UNUSED,
                                XPointer client_data,
                                XIMPreeditDrawCallbackStruct *call_data)
{
   Eina_Bool ret = EINA_FALSE;
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)client_data;
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   XIMText *t = call_data->text;
   char *tmp;
   Eina_Unicode *new_text = NULL;
   Eina_UStrbuf *preedit_bufs = NULL;
   int new_text_length;
   int i = 0;

   DBG("ctx=%p, imf_context_data=%p", ctx, imf_context_data);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   imf_context_data->preedit_cursor = call_data->caret;

   preedit_bufs = eina_ustrbuf_new();
   if (imf_context_data->preedit_chars)
     {
        ret = eina_ustrbuf_append(preedit_bufs, imf_context_data->preedit_chars);
        if (ret == EINA_FALSE) goto done;
     }

   new_text_length = _ecore_imf_xim_text_to_utf8(ctx, t, &tmp);
   if (tmp)
     {
        int tmp_len;
        new_text = eina_unicode_utf8_to_unicode(tmp, &tmp_len);
        free(tmp);
     }

   if (t == NULL)
     {
        /* delete string */
        ret = eina_ustrbuf_remove(preedit_bufs,
                                  call_data->chg_first, call_data->chg_length);
     }
   else if (call_data->chg_length == 0)
     {
        /* insert string */
        ret = eina_ustrbuf_insert(preedit_bufs, new_text, call_data->chg_first);
     }
   else if (call_data->chg_length > 0)
     {
        /* replace string */
        ret = eina_ustrbuf_remove(preedit_bufs,
                                  call_data->chg_first, call_data->chg_length);
        if (ret == EINA_FALSE) goto done;

        ret = eina_ustrbuf_insert_n(preedit_bufs, new_text,
                                    new_text_length, call_data->chg_first);
        if (ret == EINA_FALSE) goto done;
     }
   else
     {
        ret = EINA_FALSE;
     }

done:
   if (ret == EINA_TRUE)
     {
        free(imf_context_data->preedit_chars);
        imf_context_data->preedit_chars =
          eina_ustrbuf_string_steal(preedit_bufs);
        imf_context_data->preedit_length =
          eina_unicode_strlen(imf_context_data->preedit_chars);

        if (imf_context_data->feedbacks)
          {
             free(imf_context_data->feedbacks);
             imf_context_data->feedbacks = NULL;
          }

        if (imf_context_data->preedit_length > 0)
          {
             imf_context_data->feedbacks = calloc(imf_context_data->preedit_length, sizeof(XIMFeedback));

             for (i = 0; i < imf_context_data->preedit_length; i++)
               {
                  if (t)
                    imf_context_data->feedbacks[i] = t->feedback[i];
               }
          }

        ecore_imf_context_event_callback_call(ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
     }

   free(new_text);
   eina_ustrbuf_free(preedit_bufs);
}

/**
 * @internal
 * @brief XIM callback: Preedit Caret.
 *
 * This function is invoked by the XIM server when the preedit caret (cursor)
 * position needs to be updated. It updates the internal preedit cursor
 * position and triggers the ECORE_IMF_CALLBACK_PREEDIT_CHANGED event if the
 * position changed absolutely and the context is not finalizing.
 *
 * @param xic The X Input Context (unused).
 * @param client_data A pointer to the Ecore_IMF_Context.
 * @param call_data A pointer to XIMPreeditCaretCallbackStruct containing details
 *                  about the caret movement:
 *                  `call_data->position`: new caret position (character offset).
 *                  `call_data->direction`: type of movement (e.g., XIMAbsolutePosition).
 *                  `call_data->style`: style of caret (unused here).
 */
static void
_ecore_imf_xim_preedit_caret_call(XIC xic EINA_UNUSED,
                                 XPointer client_data,
                                 XIMPreeditCaretCallbackStruct *call_data)
{
   Ecore_IMF_Context *ctx = (Ecore_IMF_Context *)client_data;
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);

   DBG("ctx=%p, imf_context_data=%p", ctx, imf_context_data);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   if (call_data->direction == XIMAbsolutePosition)
     {
        imf_context_data->preedit_cursor = call_data->position;
        if (imf_context_data->finalizing == EINA_FALSE)
          ecore_imf_context_event_callback_call(ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
     }
}

static XVaNestedList
_ecore_imf_xim_preedit_callback_set(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Context_Data *imf_context_data;
   imf_context_data = ecore_imf_context_data_get(ctx);
   if (!imf_context_data)
     return XVaCreateNestedList(0, NULL);

   imf_context_data->preedit_start_cb.client_data = (XPointer)ctx;
   imf_context_data->preedit_start_cb.callback = (XIMProc)_ecore_imf_xim_preedit_start_call;

   imf_context_data->preedit_done_cb.client_data = (XPointer)ctx;
   imf_context_data->preedit_done_cb.callback = (XIMProc)_ecore_imf_xim_preedit_done_call;

   imf_context_data->preedit_draw_cb.client_data = (XPointer)ctx;
   imf_context_data->preedit_draw_cb.callback = (XIMProc)_ecore_imf_xim_preedit_draw_call;

   imf_context_data->preedit_caret_cb.client_data = (XPointer)ctx;
   imf_context_data->preedit_caret_cb.callback = (XIMProc)_ecore_imf_xim_preedit_caret_call;

   return XVaCreateNestedList(0,
                              XNPreeditStartCallback,
                              &imf_context_data->preedit_start_cb,
                              XNPreeditDoneCallback,
                              &imf_context_data->preedit_done_cb,
                              XNPreeditDrawCallback,
                              &imf_context_data->preedit_draw_cb,
                              XNPreeditCaretCallback,
                              &imf_context_data->preedit_caret_cb,
                              NULL);
}

/**
 * @internal
 * @brief Retrieves or creates the X Input Context (XIC) for an Ecore IMF context.
 *
 * If an XIC already exists for the context, it is returned. Otherwise, a new
 * XIC is created using XCreateIC. The creation process involves:
 * - Determining the appropriate XIMStyle (e.g., "OverTheSpot", "OffTheSpot" via callbacks).
 * - Setting up preedit attributes (XNSpotLocation, XNFontSet, or XNPreeditCallbacks).
 * - Associating the XIC with the client window.
 * - Retrieving and storing the event mask (XNFilterEvents) for the XIC.
 * If an XIC is successfully created and the context has focus, XSetICFocus is called.
 *
 * @param ctx The Ecore IMF context for which to get the XIC.
 * @return The existing or newly created XIC, or NULL on failure.
 */
static XIC
_ecore_imf_xim_ic_get(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Context_Data *imf_context_data;
   XIC ic;
   imf_context_data = ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN_VAL(imf_context_data, NULL);

   if (!ecore_x_display_get()) return NULL;
   ic = imf_context_data->ic;
   if (!ic)
     {
        XIM_Im_Info *im_info = imf_context_data->im_info;
        XVaNestedList preedit_attr = NULL;
        XIMStyle im_style = 0;
        XPoint spot = { 0, 0 };
        char *name = NULL;

        if (!im_info)
          {
             WRN("Could not open XIM.");
             return NULL;
          }

        // supported styles
        // "OverTheSpot" = XIMPreeditPosition | XIMStatusNothing
        // "OffTheSpot" = XIMPreeditArea | XIMStatusArea
        // "Root" = XIMPreeditNothing | XIMStatusNothing

        if (imf_context_data->use_preedit == EINA_TRUE)
          {
             if (im_info->supports_cursor)
               {
                  // kinput2 DOES do this...
                  XFontSet fs;
                  char **missing_charset_list;
                  int missing_charset_count;
                  char *def_string;

                  im_style |= XIMPreeditPosition;
                  im_style |= XIMStatusNothing;
                  fs = XCreateFontSet(ecore_x_display_get(),
                                      "fixed",
                                      &missing_charset_list,
                                      &missing_charset_count,
                                      &def_string);
                  preedit_attr = XVaCreateNestedList(0,
                                                     XNSpotLocation, &spot,
                                                     XNFontSet, fs,
                                                     NULL);
               }
             else
               {
                  im_style |= XIMPreeditCallbacks;
                  im_style |= XIMStatusNothing;
                  preedit_attr = _ecore_imf_xim_preedit_callback_set(ctx);
               }
             name = XNPreeditAttributes;
          }
        else
          {
             im_style |= XIMPreeditNothing;
             im_style |= XIMStatusNothing;
          }

        if (!im_info->xim_styles)
          {
             WRN("No XIM styles supported! Wanted %#llx",
                 (unsigned long long)im_style);
             im_style = 0;
          }
        else
          {
             XIMStyle fallback = 0;
             int i;

             for (i = 0; i < im_info->xim_styles->count_styles; i++)
               {
                  XIMStyle cur = im_info->xim_styles->supported_styles[i];
                  if (cur == im_style)
                    break;
                  else if (cur == (XIMPreeditNothing | XIMStatusNothing))
                    /* TODO: fallback is just that or the anyone? */
                    fallback = cur;
               }

             if (i == im_info->xim_styles->count_styles)
               {
                  if (fallback)
                    {
                       WRN("Wanted XIM style %#llx not found, using fallback %#llx instead.",
                           (unsigned long long)im_style,
                           (unsigned long long)fallback);
                       im_style = fallback;
                    }
                  else
                    {
                       WRN("Wanted XIM style %#llx not found, no fallback supported.",
                           (unsigned long long)im_style);
                       im_style = 0;
                    }
               }
          }

        if ((im_info->im) && (im_style))
          {
             ic = XCreateIC(im_info->im,
                            XNInputStyle, im_style,
                            XNClientWindow, imf_context_data->win,
                            name, preedit_attr, NULL);
          }
        XFree(preedit_attr);
        if (ic)
          {
             unsigned long mask = 0xaaaaaaaa;
             XGetICValues(ic,
                          XNFilterEvents, &mask,
                          NULL);
             imf_context_data->mask = mask;
             ecore_x_event_mask_set(imf_context_data->win, mask);
          }

        imf_context_data->ic = ic;
        if (ic && imf_context_data->has_focus == EINA_TRUE)
          XSetICFocus(ic);
     }

   return ic;
}

/**
 * @internal
 * @brief Reinitializes the X Input Context (XIC) for an Ecore IMF context.
 *
 * This function destroys the current XIC (if one exists) and clears its
 * reference in the context data. This forces _ecore_imf_xim_ic_get() to
 * create a new XIC the next time it's called. This is typically used when
 * XIC properties like the preedit style need to change. If there was an
 * active preedit string, it is cleared and a PREEDIT_CHANGED event is emitted.
 *
 * @param ctx The Ecore IMF context whose XIC needs reinitialization.
 */
static void
_ecore_imf_xim_ic_reinitialize(Ecore_IMF_Context *ctx)
{
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   XIC ic = imf_context_data->ic;
   if (ic)
     {
        XDestroyIC(ic);
        imf_context_data->ic = NULL;
        if (imf_context_data->preedit_length)
          {
             imf_context_data->preedit_length = 0;
             free(imf_context_data->preedit_chars);
             imf_context_data->preedit_chars = NULL;
             ecore_imf_context_event_callback_call(ctx, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, NULL);
          }
     }
}

static void
_ecore_imf_xim_ic_client_window_set(Ecore_IMF_Context *ctx,
                                    Ecore_X_Window window)
{
   Ecore_IMF_Context_Data *imf_context_data = ecore_imf_context_data_get(ctx);
   Ecore_X_Window old_win;

   DBG("ctx=%p, imf_context_data=%p", ctx, imf_context_data);
   EINA_SAFETY_ON_NULL_RETURN(imf_context_data);

   /* reinitialize IC */
   _ecore_imf_xim_ic_reinitialize(ctx);

   old_win = imf_context_data->win;
   DBG("old_win=%#x, window=%#x", old_win, window);
   if (old_win != 0 && old_win != window)   /* XXX how do check window... */
     {
        XIM_Im_Info *info = imf_context_data->im_info;
        if (info)
          {
             info->ics = eina_list_remove(info->ics, imf_context_data);
             info->user = NULL;
             info = NULL;
          }
     }

   imf_context_data->win = window;

   if (window) /* XXX */
     {
        XIM_Im_Info *info = NULL;
        info = _ecore_imf_xim_im_get(window, imf_context_data->locale);
        imf_context_data->im_info = info;
        imf_context_data->im_info->ics =
          eina_list_prepend(imf_context_data->im_info->ics,
                            imf_context_data);
        if (imf_context_data->im_info)
          imf_context_data->im_info->user = imf_context_data;
     }
}

static XIM_Im_Info *
_ecore_imf_xim_im_get(Ecore_X_Window window,
                     char *locale)
{
   Eina_List *l;
   XIM_Im_Info *im_info = NULL;
   XIM_Im_Info *info = NULL;

   DBG(" ");
   EINA_LIST_FOREACH (open_ims, l, im_info)
     {
        if (strcmp(im_info->locale, locale) == 0)
          {
             if (im_info->im)
               {
                  return im_info;
               }
             else
               {
                  info = im_info;
                  break;
               }
          }
     }

   if (!info)
     {
        info = calloc(1, sizeof(XIM_Im_Info));
        if (!info) return NULL;
        open_ims = eina_list_prepend(open_ims, info);
        info->win = window;
        info->locale = strdup(locale);
        info->reconnecting = EINA_FALSE;
     }

   _ecore_imf_xim_info_im_init(info);
   return info;
}

/**
 * @internal
 * @brief Initializes the XIM (X Input Method) connection for a given XIM_Im_Info structure.
 *
 * This function attempts to open a connection to the XIM server using XOpenIM().
 * If successful, it calls _ecore_imf_xim_im_setup() to configure the IM.
 * If XOpenIM() fails (e.g., no IM server running), it registers an
 * IMInstantiateCallback with X to be notified when an IM server becomes available.
 * It ensures that X locales are supported and locale modifiers are set.
 *
 * @param info A pointer to the XIM_Im_Info structure for which to initialize the IM.
 *             `info->im` will be set to the new XIM object or NULL.
 *             `info->reconnecting` will be set if waiting for an IM server.
 */
static void
_ecore_imf_xim_info_im_init(XIM_Im_Info *info)
{
   Ecore_X_Display *dsp;

   assert(info->im == NULL);
   if (info->reconnecting == EINA_TRUE)
     return;

   if (XSupportsLocale())
     {
        if (!XSetLocaleModifiers(""))
          WRN("Unable to set locale modifiers with XSetLocaleModifiers()");
        dsp = ecore_x_display_get();
        if (dsp)
          {
             info->im = XOpenIM(dsp, NULL, NULL, NULL);
             if (!info->im)
               {
                  XRegisterIMInstantiateCallback(dsp,
                                                 NULL, NULL, NULL,
                                                 _ecore_imf_xim_instantiate_cb,
                                                 (XPointer)info);
                  info->reconnecting = EINA_TRUE;
                  return;
               }
             _ecore_imf_xim_im_setup(info);
          }
     }
}

static void
_ecore_imf_xim_info_im_shutdown(Ecore_X_Display *display EINA_UNUSED,
                               int is_error EINA_UNUSED,
                               XIM_Im_Info *info)
{
   Eina_List *ics, *tmp_list;
   Ecore_IMF_Context *ctx;

   // Remove the IM info from the global list of open IMs.
   open_ims = eina_list_remove(open_ims, info);

   ics = info->ics;
   info->ics = NULL;

   // For each IC associated with this IM, detach it from its client window.
   // This effectively resets the IC's window association.
   EINA_LIST_FOREACH (ics, tmp_list, ctx)
     _ecore_imf_xim_ic_client_window_set(ctx, 0);

   // Free all ICs that were using this IM.
   // This also destroys their associated Ecore_IMF_Context_Data.
   EINA_LIST_FREE (ics, ctx)
     {
        Ecore_IMF_Context_Data *imf_context_data;
        imf_context_data = ecore_imf_context_data_get(ctx);
        _ecore_imf_xim_context_data_destroy(imf_context_data);
     }

   free(info->locale);

   if (info->xim_styles)
     XFree(info->xim_styles); // Free cached XIM styles.

   if (info->im)
     XCloseIM(info->im); // Close the XIM connection.

   free(info); // Free the XIM_Im_Info structure itself.
}

/**
 * @internal
 * @brief XIM callback: IM Instantiate.
 *
 * This function is invoked by Xlib when an XIM server becomes available after
 * a previous XOpenIM call failed and an instantiate callback was registered.
 * It attempts to open the IM again and, if successful, sets up the IM and
 * unregisters this callback.
 *
 * @param display The X Display on which the IM became available.
 * @param client_data A pointer to the XIM_Im_Info structure that was waiting
 *                    for the IM.
 * @param call_data XIM-specific callback data (unused).
 */
static void
_ecore_imf_xim_instantiate_cb(Display *display,
                             XPointer client_data,
                             XPointer call_data EINA_UNUSED)
{
   XIM_Im_Info *info = (XIM_Im_Info *)client_data;
   XIM im = XOpenIM(display, NULL, NULL, NULL);
   EINA_SAFETY_ON_NULL_RETURN(im);

   info->im = im;
   _ecore_imf_xim_im_setup(info); // Proceed with IM setup.

   // Unregister this callback as the IM is now instantiated.
   XUnregisterIMInstantiateCallback(display, NULL, NULL, NULL,
                                    _ecore_imf_xim_instantiate_cb,
                                    (XPointer)info);
   info->reconnecting = EINA_FALSE; // No longer in reconnecting state.
}

/**
 * @internal
 * @brief Sets up an XIM_Im_Info structure after a successful XOpenIM or instantiation.
 *
 * This function is called once an XIM connection (info->im) is established.
 * It performs the following setup steps:
 * 1. Registers a destroy callback (XNDestroyCallback) with the XIM server.
 *    This callback (_ecore_imf_xim_destroy_cb) will be invoked if the IM server
 *    connection is unexpectedly terminated.
 * 2. Queries the IM server for supported input styles (XNQueryInputStyle) and
 *    stores them in `info->xim_styles`.
 * 3. Queries the IM server for a list of supported IC (Input Context) values
 *    (XNQueryICValuesList).
 * 4. From the list of supported IC values, it checks for specific capabilities:
 *    - XNStringConversionCallback: Sets `info->supports_string_conversion`.
 *    - XNCursor: Sets `info->supports_cursor`.
 *
 * @param info A pointer to the XIM_Im_Info structure to set up.
 *             `info->im` must be a valid XIM object.
 */
static void
_ecore_imf_xim_im_setup(XIM_Im_Info *info)
{
   XIMValuesList *ic_values = NULL;
   XIMCallback im_destroy_callback;

   if (!info->im)
     return;

   im_destroy_callback.client_data = (XPointer)info;
   im_destroy_callback.callback = (XIMProc)_ecore_imf_xim_destroy_cb;
   XSetIMValues(info->im,
                XNDestroyCallback, &im_destroy_callback,
                NULL);

   XGetIMValues(info->im,
                XNQueryInputStyle, &info->xim_styles,
                XNQueryICValuesList, &ic_values,
                NULL);

   if (ic_values)
     {
        int i;

        for (i = 0; i < ic_values->count_values; i++)
          {
             if (!strcmp(ic_values->supported_values[i],
                         XNStringConversionCallback))
               info->supports_string_conversion = EINA_TRUE;
             if (!strcmp(ic_values->supported_values[i],
                         XNCursor))
               info->supports_cursor = EINA_TRUE;
          }

        XFree(ic_values);
     }
}

/**
 * @internal
 * @brief XIM callback: IM Destroy.
 *
 * This function is invoked by Xlib if the XIM server connection associated
 * with `info->im` is destroyed (e.g., IM server crashes or exits).
 * It marks the IM as unavailable (`info->im = NULL`), clears the IC
 * reference in the associated user data (if any), and then attempts to
 * re-initialize the IM connection by calling _ecore_imf_xim_info_im_init().
 * This re-initialization might lead to registering an instantiate callback
 * if the IM server is not immediately available again.
 *
 * @param xim The XIM object that was destroyed (unused, as info->im holds it).
 * @param client_data A pointer to the XIM_Im_Info structure whose IM was destroyed.
 * @param call_data XIM-specific callback data (unused).
 */
static void
_ecore_imf_xim_destroy_cb(XIM xim EINA_UNUSED,
                         XPointer client_data,
                         XPointer call_data EINA_UNUSED)
{
   XIM_Im_Info *info = (XIM_Im_Info *)client_data;

   // If there's an associated IMF context, clear its IC, as it's no longer valid.
   if (info->user) info->user->ic = NULL;
   info->im = NULL; // Mark the IM as destroyed.

   // Attempt to re-establish the IM connection.
   // This might register an instantiate callback if the IM server is not immediately available.
   // Note: The commented out _ecore_imf_xim_ic_reinitialize(ctx) would be problematic
   // here as 'ctx' is not directly available. The current approach of re-initializing
   // the IM info is more robust for handling IM server disappearance.
   _ecore_imf_xim_info_im_init(info);

   return;
}
