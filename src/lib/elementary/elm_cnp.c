#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include <Ecore_Evas.h>

#include "elm_priv.h"
#include "elm_entry_eo.h"

/**
 * @internal
 * @brief Converts an Elm_Sel_Type to its corresponding Ecore_Evas_Selection_Buffer.
 * This function maps the Elementary selection types (like PRIMARY, XDND, CLIPBOARD)
 * to the Ecore_Evas buffer types used by the underlying windowing system integration.
 * @param type The Elm_Sel_Type to convert.
 * @return The corresponding Ecore_Evas_Selection_Buffer, or ECORE_EVAS_SELECTION_BUFFER_LAST if not found.
 */
static inline Ecore_Evas_Selection_Buffer
_elm_sel_type_to_ee_type(Elm_Sel_Type type)
{
   if (type == ELM_SEL_TYPE_PRIMARY)
     return ECORE_EVAS_SELECTION_BUFFER_SELECTION_BUFFER;
   if (type == ELM_SEL_TYPE_XDND)
     return ECORE_EVAS_SELECTION_BUFFER_DRAG_AND_DROP_BUFFER;
   if (type == ELM_SEL_TYPE_CLIPBOARD)
     return ECORE_EVAS_SELECTION_BUFFER_COPY_AND_PASTE_BUFFER;
   return ECORE_EVAS_SELECTION_BUFFER_LAST;
}

/**
 * @internal
 * @brief Converts an Elm_Sel_Format to an array of corresponding MIME types.
 * This function translates Elementary's internal content format identifiers
 * (like TEXT, MARKUP, IMAGE) into a list of standard MIME type strings.
 * For example, ELM_SEL_FORMAT_IMAGE might map to "image/png", "image/jpeg", etc.
 * The returned Eina_Array contains C-strings (const char *) of MIME types.
 * Example of returned array structure for ELM_SEL_FORMAT_IMAGE:
 *   ret[0] = "image/png"
 *   ret[1] = "image/jpeg"
 *   ...
 * @param format The Elm_Sel_Format bitmask.
 * @return A new Eina_Array containing MIME type strings. The caller is responsible for freeing this array.
 *         Returns an empty array if no matching MIME types are found for the given format,
 *         and logs an error.
 */
static inline Eina_Array*
_elm_sel_format_to_mime_type(Elm_Sel_Format format)
{
   Eina_Array *ret = eina_array_new(10);
   if (format & ELM_SEL_FORMAT_URILIST)
     eina_array_push(ret, "text/uri-list");
   if (format & ELM_SEL_FORMAT_TEXT)
     eina_array_push(ret, "text/plain;charset=utf-8");
   if (format & ELM_SEL_FORMAT_MARKUP)
      eina_array_push(ret, "application/x-elementary-markup");
   if (format & ELM_SEL_FORMAT_IMAGE)
     {
        eina_array_push(ret, "image/png");
        eina_array_push(ret, "image/jpeg");
        eina_array_push(ret, "image/x-ms-bmp");
        eina_array_push(ret, "image/gif");
        eina_array_push(ret, "image/tiff");
        eina_array_push(ret, "image/svg+xml");
        eina_array_push(ret, "image/x-xpixmap");
        eina_array_push(ret, "image/x-tga");
        eina_array_push(ret, "image/x-portable-pixmap");
     }
   if (format & ELM_SEL_FORMAT_VCARD)
     eina_array_push(ret, "text/vcard");
   if (format & ELM_SEL_FORMAT_HTML)
     eina_array_push(ret, "application/xhtml+xml");

   if (eina_array_count(ret) == 0)
     ERR("Specified mime type is not available");

   return ret;
}

/**
 * @internal
 * @brief Structure to map a sequence of bytes (magic numbers) to a MIME type.
 * This is used to identify image formats by their initial bytes.
 */
typedef struct {
  const unsigned char image_sequence[16]; /**< The byte sequence (magic numbers) to match. */
  const size_t image_sequence_len; /**< The length of the byte sequence. */
  const char *mimetype; /**< The corresponding MIME type string. */
} Mimetype_Content_Matcher;

/**
 * @internal
 * @brief Array of Mimetype_Content_Matcher structures for known image formats.
 * This table is used by _elm_sel_from_content_to_mime_type to detect
 * image MIME types based on the initial bytes of the content.
 */
static const Mimetype_Content_Matcher matchers[] = {
  {{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}, 8, "image/png"}, /**< PNG magic numbers */
  {{0xFF, 0xD8}, 2, "image/jpeg"}, /**< JPEG magic numbers */
  {{0x42, 0x4D}, 2, "image/x-ms-bmp"},
  {{0x47, 0x49, 0x46, 0x38, 0x37, 0x61}, 6, "image/gif"},
  {{0x47, 0x49, 0x46, 0x38, 0x39, 0x61}, 6, "image/gif"},
  {{0x49, 0x49, 0x2A, 00}, 4, "image/tiff"},
  {{0x4D, 0x4D, 0x00, 0x2A}, 4, "image/tiff"}, /**< TIFF magic numbers (big endian) */
  {{0},0, NULL} /**< Sentinel to mark the end of the array. */
};

/**
 * @internal
 * @brief Determines the MIME type of image data by inspecting its initial bytes.
 * This function iterates through the `matchers` table to find a matching
 * byte sequence (magic numbers) at the beginning of the provided buffer.
 * It's used when the format is ELM_SEL_FORMAT_IMAGE to get a specific image MIME type.
 * The returned Eina_Array will contain a single C-string (const char *) of the detected MIME type.
 * Example of returned array structure if PNG is detected:
 *   ret[0] = "image/png"
 * @param buf Pointer to the data buffer.
 * @param buflen Length of the data buffer.
 * @return A new Eina_Array containing the detected MIME type string.
 *         Returns an array with one element if a match is found.
 *         Returns an empty array and logs an error if no match is found or if `buflen` is too small.
 *         The caller is responsible for freeing this array.
 */
static inline Eina_Array*
_elm_sel_from_content_to_mime_type(const void *buf, size_t buflen)
{
   Eina_Array *ret = eina_array_new(10);

   for (int i = 0; matchers[i].mimetype && eina_array_count(ret) == 0; ++i)
     {
        if (matchers[i].image_sequence_len >= buflen) continue;
        for (size_t j = 0; j < matchers[i].image_sequence_len; ++j)
          {
             if (((const unsigned  char*)buf)[j] == matchers[i].image_sequence[j])
               {
                  eina_array_push(ret, matchers[i].mimetype);
                  break;
               }
          }
     }

   if (eina_array_count(ret) != 1)
     ERR("Specified mime type is not available");

   return ret;
}

/**
 * @internal
 * @brief Converts a MIME type string to an Elm_Sel_Format.
 * This function performs the reverse of _elm_sel_format_to_mime_type, mapping
 * a standard MIME type string back to an Elementary content format identifier.
 * @param mime_type The MIME type string (e.g., "text/plain;charset=utf-8", "image/png").
 * @return The corresponding Elm_Sel_Format, or ELM_SEL_FORMAT_NONE if no match is found.
 */
static inline Elm_Sel_Format
_mime_type_to_elm_sel_format(const char *mime_type)
{
   if (eina_streq(mime_type, "text/vcard"))
     return ELM_SEL_FORMAT_VCARD;
   else if (eina_streq(mime_type, "application/x-elementary-markup"))
     return ELM_SEL_FORMAT_MARKUP;
   else if (eina_streq(mime_type, "application/xhtml+xml"))
     return ELM_SEL_FORMAT_HTML;
   else if (eina_streq(mime_type, "text/uri-list"))
     return ELM_SEL_FORMAT_URILIST;
   else if (!strncmp(mime_type, "text/", strlen("text/")))
     return ELM_SEL_FORMAT_TEXT;
   else if (!strncmp(mime_type, "image/", strlen("image/")))
     return ELM_SEL_FORMAT_IMAGE;

   return ELM_SEL_FORMAT_NONE;
}

/**
 * @internal
 * @brief Gets the ID of the default seat associated with an Evas object.
 * A seat typically represents a user's set of input devices (keyboard, mouse)
 * and their focus. This function retrieves the ID for the default seat.
 * @param obj The Evas_Object to get the default seat for.
 * @return The integer ID of the default seat.
 */
static int
_default_seat(const Eo *obj)
{
   return evas_device_seat_id_get(evas_default_device_get(evas_object_evas_get(obj), EVAS_DEVICE_CLASS_SEAT));
}

EAPI Eina_Bool
elm_cnp_selection_set(Evas_Object *obj, Elm_Sel_Type selection,
                                     Elm_Sel_Format format,
                                     const void *buf, size_t buflen)
{
   Eina_Content *content;
   Ecore_Evas *ee;
   const char *mime_type;
   Eina_Slice data;
   Eina_Array *tmp;
   unsigned char *mem_buf = NULL;

   if (!obj)
     {
        ERR("elm_cnp_selection_set() passed NULL object");
        return EINA_FALSE;
     }
   ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));
   if (!ee)
     {
        ERR("elm_cnp_selection_set() can't fine ecore_evas for obj %p", obj);
        return EINA_FALSE;
     }

   if (((format == ELM_SEL_FORMAT_TEXT) && ((char *)buf)[buflen - 1] != '\0') ||
       ((format == ELM_SEL_FORMAT_URILIST) && ((char *)buf)[buflen - 1] != '\0'))
     {
        mem_buf = eina_memdup((unsigned char *)buf, buflen, EINA_TRUE);
        data.mem = mem_buf;
        data.len = buflen + 1;
     }
   else
     {
        data.mem = buf;
        data.len = buflen;
     }

   if (format == ELM_SEL_FORMAT_IMAGE)
     {
        tmp = _elm_sel_from_content_to_mime_type(buf, buflen);
     }
   else
     {
        tmp = _elm_sel_format_to_mime_type(format);
     }


   if (eina_array_count(tmp) != 1)
     {
        ERR("You cannot specify more than one format when setting selection");
     }
   mime_type = eina_array_data_get(tmp, 0);
   eina_array_free(tmp);
   content = eina_content_new(data, mime_type);
   _register_selection_changed(obj);

   if (mem_buf != NULL)
     free(mem_buf);

   return ecore_evas_selection_set(ee, _default_seat(obj), _elm_sel_type_to_ee_type(selection), content);
}

EAPI Eina_Bool
elm_object_cnp_selection_clear(Evas_Object *obj,
                                              Elm_Sel_Type selection)
{
   Ecore_Evas *ee;

   ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));
   return ecore_evas_selection_set(ee, _default_seat(obj), _elm_sel_type_to_ee_type(selection), NULL);
}

EAPI Eina_Bool
elm_cnp_clipboard_selection_has_owner(Evas_Object *obj)
{
   Ecore_Evas *ee;

   ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));
   return ecore_evas_selection_exists(ee, _default_seat(obj), ECORE_EVAS_SELECTION_BUFFER_COPY_AND_PASTE_BUFFER);

}

typedef struct _Sel_Lost_Data Sel_Lost_Data;
struct _Sel_Lost_Data
{
   const Evas_Object *obj;
   Elm_Sel_Type type;
   void *udata;
   Elm_Selection_Loss_Cb loss_cb; /**< User-provided callback to invoke on selection loss. */
};

/**
 * @internal
 * @brief Callback function invoked when a window manager selection changes.
 * This function is registered as an event listener for EFL_UI_SELECTION_EVENT_WM_SELECTION_CHANGED.
 * It checks if the selection change corresponds to the type (PRIMARY or CLIPBOARD)
 * being monitored and if the change was not caused by the object itself.
 * If these conditions are met, it calls the user-provided `loss_cb`.
 * @param data A pointer to Sel_Lost_Data containing context for the callback.
 * @param ev The Efl_Event information.
 */
static void
_selection_changed_cb(void *data, const Efl_Event *ev)
{
   Sel_Lost_Data *ldata = data;
   Efl_Ui_Wm_Selection_Changed *changed = ev->info;

   if (changed->buffer == EFL_UI_CNP_BUFFER_SELECTION && ldata->type != ELM_SEL_TYPE_PRIMARY)
     return;

   if (changed->buffer == EFL_UI_CNP_BUFFER_COPY_AND_PASTE && ldata->type != ELM_SEL_TYPE_CLIPBOARD)
     return;

   if (ldata->obj == changed->caused_by)
     return;

   ldata->loss_cb(ldata->udata, ldata->type);
   efl_event_callback_del(ev->object, ev->desc, _selection_changed_cb, data);
   free(data);
}

EAPI void
elm_cnp_selection_loss_callback_set(Evas_Object *obj, Elm_Sel_Type type, Elm_Selection_Loss_Cb func, const void *data)
{
   Sel_Lost_Data *ldata = calloc(1, sizeof(Sel_Lost_Data));

   if (!ldata) return;
   ldata->obj = obj;
   ldata->type = type;
   ldata->udata = (void *)data;
   ldata->loss_cb = func;
   efl_event_callback_add(obj, EFL_UI_SELECTION_EVENT_WM_SELECTION_CHANGED, _selection_changed_cb, ldata);
}

typedef struct {
   Elm_Drop_Cb data_cb;
   void *data;
   Elm_Sel_Format format; /**< The expected format of the selection data. */
} Callback_Storage;

/**
 * @internal
 * @brief Callback invoked when selection data has been successfully retrieved.
 * This function is part of the asynchronous process of getting selection data.
 * It receives the data as an Eina_Value (containing Eina_Content),
 * converts it to Elm_Selection_Data, and then passes it to either the
 * user-provided callback (cb_storage->data_cb) or, for entries, directly
 * pastes the text.
 * @param obj The Evas_Object that requested the selection data.
 * @param data A pointer to Callback_Storage containing context for this delivery.
 * @param value The Eina_Value containing the retrieved selection data (as Eina_Content).
 * @return EINA_VALUE_EMPTY.
 */
static Eina_Value
_callback_storage_deliver(Eo *obj, void *data, const Eina_Value value)
{
   Callback_Storage *cb_storage = data;
   Eina_Content *content = eina_value_to_content(&value);
   Elm_Sel_Format format = _mime_type_to_elm_sel_format(eina_content_type_get(content));
   Eina_Slice cdata;

   cdata = eina_content_data_get(content);
   Elm_Selection_Data d = { 0 };
   d.data = eina_memdup((unsigned char*)cdata.bytes, cdata.len, EINA_FALSE);
   d.len = cdata.len;
   d.format = _mime_type_to_elm_sel_format(eina_content_type_get(content));

   if (cb_storage->data_cb)
     {
        cb_storage->data_cb(cb_storage->data, obj, &d);
     }
   else
     {
        EINA_SAFETY_ON_FALSE_GOTO(format == ELM_SEL_FORMAT_TEXT || format == ELM_SEL_FORMAT_MARKUP || format == ELM_SEL_FORMAT_HTML, end);

        _elm_entry_entry_paste(obj, (const char *) d.data);
     }

end:
   free(d.data);

   return EINA_VALUE_EMPTY;
}

/**
 * @internal
 * @brief Callback invoked if an error occurs while retrieving selection data.
 * This function logs an error message indicating that the content could not be received.
 * @param obj The Evas_Object involved (unused).
 * @param data The callback storage data (unused).
 * @param error The Eina_Error code indicating the failure reason.
 * @return EINA_VALUE_EMPTY.
 */
static Eina_Value
_callback_storage_error(Eo *obj EINA_UNUSED, void *data EINA_UNUSED, Eina_Error error)
{
   ERR("Content cound not be received because of %s.", eina_error_msg_get(error));
   return EINA_VALUE_EMPTY;
}

/**
 * @internal
 * @brief Callback invoked to free the Callback_Storage structure.
 * This function is called when the future associated with the selection get operation
 * is completed (either successfully or with an error), allowing for cleanup
 * of the allocated Callback_Storage.
 * @param obj The Evas_Object involved (unused).
 * @param data A pointer to the Callback_Storage to be freed.
 * @param dead_future The completed Eina_Future (unused).
 */
static void
_callback_storage_free(Eo *obj EINA_UNUSED, void *data, const Eina_Future *dead_future EINA_UNUSED)
{
   free(data);
}

EAPI Eina_Bool
elm_cnp_selection_get(const Evas_Object *obj, Elm_Sel_Type selection,
                                     Elm_Sel_Format format,
                                     Elm_Drop_Cb data_cb, void *udata)
{
   Ecore_Evas *ee;
   Eina_Array *mime_types;
   Eina_Future *future;
   Callback_Storage *storage;

   ee = ecore_evas_ecore_evas_get(evas_object_evas_get(obj));
   mime_types = _elm_sel_format_to_mime_type(format);
   future = ecore_evas_selection_get(ee, _default_seat(obj), _elm_sel_type_to_ee_type(selection), eina_array_iterator_new(mime_types));
   storage = calloc(1,sizeof(Callback_Storage));
   storage->data_cb = data_cb;
   storage->data = udata;
   storage->format = format;

   efl_future_then(obj, future, _callback_storage_deliver, _callback_storage_error, _callback_storage_free, EINA_VALUE_TYPE_CONTENT, storage);

   return EINA_TRUE;
}
