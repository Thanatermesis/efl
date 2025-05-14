#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eina_abstract_content.h"

/**
 * @internal
 * @struct _Eina_Content
 * @brief Represents a piece of content with associated metadata.
 *
 * This structure holds the actual data as a slice, its MIME type,
 * an optional file path if the content is written to a temporary file,
 * and a reference count.
 */
struct _Eina_Content
{
   Eina_Rw_Slice data; /**< The actual content data. */
   const char *type; /**< The IANA MIME type of the content (e.g., "text/plain"). */
   const char *file; /**< Optional path to a temporary file holding the content. NULL if not materialized to a file. */
   EINA_REFCOUNT; /**< Reference count for managing the lifecycle of the content object. */
};
EINA_API const Eina_Value_Type *EINA_VALUE_TYPE_CONTENT;

/** @internal @brief Log domain for abstract content operations. */
static int _eina_abstract_content_log_domain = -1;

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_eina_abstract_content_log_domain, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_eina_abstract_content_log_domain, __VA_ARGS__)

/** @internal @brief Hash table storing conversion callbacks. Key: from_type (Eina_Stringshare *), Value: Eina_List* of Eina_Content_Conversion_Node. */
static Eina_Hash *conversion_callbacks;

/**
 * @internal
 * @struct Eina_Content_Conversion_Node
 * @brief Represents a single conversion capability.
 *
 * This structure stores the target MIME type for a conversion and the
 * callback function that performs this conversion.
 */
typedef struct {
   const char *to; /**< The target MIME type (e.g., "text/plain;charset=utf-8"). This is a stringshared pointer. */
   Eina_Content_Conversion_Callback callback; /**< The function pointer to perform the conversion. */
} Eina_Content_Conversion_Node;

/**
 * @internal
 * @brief Increments the reference count of an Eina_Content object.
 * @param content The content object to reference.
 */
static void
_eina_content_ref(Eina_Content *content)
{
   EINA_REFCOUNT_REF(content);
}

EINA_API Eina_Bool
eina_content_converter_conversion_register(const char *from, const char *to, Eina_Content_Conversion_Callback conversion)
{
   Eina_Content_Conversion_Node *node = calloc(1, sizeof(Eina_Content_Conversion_Node));

   Eina_Stringshare *shared_from = eina_stringshare_add(from);

   if (eina_content_converter_convert_can(from, to))
     {
        ERR("Conversion from %s to %s is already possible", from, to);
        eina_stringshare_del(shared_from);
        free(node);
        return EINA_FALSE;
     }

   node->to = eina_stringshare_add(to);
   node->callback = conversion;

   eina_hash_list_append(conversion_callbacks, shared_from, node);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Fetches a list of possible conversion nodes for a given source MIME type.
 * @param from The source MIME type string (e.g., "text/plain").
 * @return A list of Eina_Content_Conversion_Node structures that can convert from the given type.
 *         The list itself is owned by the hash and should not be freed by the caller.
 *         Returns NULL if no conversions are registered for the 'from' type.
 */
static inline Eina_List*
_conversion_callback_fetch_possible(const char *from)
{
   Eina_Stringshare *shared_from = eina_stringshare_add(from);
   Eina_List *res = eina_hash_find(conversion_callbacks, shared_from);
   eina_stringshare_del(shared_from);
   return res;
}

/**
 * @internal
 * @brief Fetches a specific conversion callback for a given source and target MIME type.
 * @param from The source MIME type string (e.g., "text/plain").
 * @param to The target MIME type string (e.g., "text/plain;charset=utf-8").
 * @return The Eina_Content_Conversion_Callback function if a direct conversion is registered,
 *         otherwise NULL.
 */
static inline Eina_Content_Conversion_Callback
_conversion_callback_fetch(const char *from, const char *to)
{
   Eina_List *possibilities = _conversion_callback_fetch_possible(from);
   Eina_Content_Conversion_Node *n;
   Eina_Content_Conversion_Callback result = NULL;
   Eina_List *l;
   Eina_Stringshare *shared_to = eina_stringshare_add(to);

   EINA_LIST_FOREACH(possibilities, l, n)
     {
        if (n->to == shared_to)
          {
             result = n->callback;
             goto end;
          }
     }
end:
   eina_stringshare_del(shared_to);
   return result;
}

EINA_API Eina_Bool
eina_content_converter_convert_can(const char *from, const char *to)
{
   return !!_conversion_callback_fetch(from, to);
}

/**
 * @internal
 * @brief Callback for eina_iterator_processed_new to extract the 'to' type from a conversion node.
 * @param container Unused.
 * @param data A pointer to an Eina_Content_Conversion_Node.
 * @param fdata Unused.
 * @return The 'to' field (target MIME type string) of the Eina_Content_Conversion_Node.
 */
static const void*
_process_cb(const void *container EINA_UNUSED, void *data, void *fdata EINA_UNUSED)
{
   Eina_Content_Conversion_Node *n = data;

   return n->to;
 }

EINA_API Eina_Iterator*
eina_content_converter_possible_conversions(const char *from)
{
   Eina_List *possibilities = _conversion_callback_fetch_possible(from);

   return eina_iterator_processed_new(eina_list_iterator_new(possibilities) , EINA_PROCESS_CB(_process_cb), NULL, possibilities);
}

EINA_API Eina_Content*
eina_content_new(Eina_Slice data, const char *type)
{
   Eina_Content *content;

   if (eina_str_has_prefix(type,"text"))
     {
        //last char in the mem must be \0
        if (((char*)data.mem)[data.len - 1] != '\0')
          {
             ERR("Last character is not a null character! but type is text!");
             return NULL;
          }
     }

   content = calloc(1, sizeof(Eina_Content));
   EINA_SAFETY_ON_NULL_RETURN_VAL(content, NULL);
   content->data = eina_slice_dup(data);
   content->type = eina_stringshare_add(type);
   EINA_SAFETY_ON_NULL_GOTO(content->data.mem, err);

   _eina_content_ref(content);
   return content;

err:
   if (content->data.mem)
     {
        free(content->data.mem);
        content->data.mem = NULL;
     }
   free(content);
   return NULL;
}

EINA_API void
eina_content_free(Eina_Content *content)
{
   EINA_REFCOUNT_UNREF(content)
     {
        if (content->file)
          eina_tmpstr_del(content->file);
        free(content->data.mem);
        free(content);
     }
}

EINA_API const char*
eina_content_as_file(Eina_Content *content)
{
   if (!content->file)
     {
        Eina_Tmpstr *path;
        int fd = eina_file_mkstemp("prefixXXXXXX.ext", &path);

        if (fd < 0)
          {
             ERR("Failed to create tmp file");
             return NULL;
          }

        if (write(fd, content->data.mem, content->data.len) < 0)
          {
             ERR("Failed to write to a file");
             eina_tmpstr_del(path);
             close(fd);
             return NULL;
          }

        content->file = path;
        close(fd);
     }
   return content->file;
}

EINA_API const char*
eina_content_type_get(Eina_Content *content)
{
   return content->type;
}

EINA_API Eina_Slice
eina_content_data_get(Eina_Content *content)
{
   return eina_rw_slice_slice_get(content->data);
}

EINA_API Eina_Content*
eina_content_convert(Eina_Content *content, const char *new_type)
{
   Eina_Content_Conversion_Callback callback = _conversion_callback_fetch(content->type, new_type);

   if (!callback)
     {
        ERR("No suitable conversion found");
        return NULL;
     }

   return callback(content, new_type);
}

/**
 * @internal
 * @brief A generic content converter that simply copies the data and assigns a new type.
 *
 * This is used for conversions where the underlying data representation does not change,
 * but the type information does (e.g., "text/plain" to "text/plain;charset=utf-8"
 * when the original was already UTF-8 or US-ASCII).
 * @param from The source Eina_Content object.
 * @param to_type The target MIME type string.
 * @return A new Eina_Content object with the same data as 'from' but with 'to_type'.
 */
static Eina_Content*
_copy_converter(Eina_Content *from, const char *to_type)
{
   Eina_Slice slice = eina_content_data_get(from);
   return eina_content_new(slice, to_type);
}

/**
 * @internal
 * @brief Converts content from ISO-8859-1 (Latin-1) encoding to UTF-8.
 * @param from The source Eina_Content object, assumed to be "text/plain;charset=iso-8859-1".
 * @param to_type The target MIME type string, typically "text/plain;charset=utf-8".
 * @return A new Eina_Content object containing the UTF-8 encoded data.
 */
static Eina_Content*
_latin1_to_utf8_converter(Eina_Content *from, const char *to_type)
{
   Eina_Slice slice = eina_content_data_get(from);
   Eina_Strbuf *out = eina_strbuf_new();

   for (unsigned int i = 0; i < slice.len; ++i)
     {
        const unsigned char c = ((char*)slice.mem)[i];
        if (c < 128)
          eina_strbuf_append_char(out, c);
        else
          {
             eina_strbuf_append_char(out, 0xc0 | c >> 6);
             eina_strbuf_append_char(out, 0x80 | (c & 0x3f));
          }
     }
   Eina_Slice new;
   new.len = eina_strbuf_length_get(out);
   new.mem = eina_strbuf_string_get(out);
   Eina_Content *c = eina_content_new(new, to_type);
   eina_strbuf_free(out);
   return c;
}

/**
 * @internal
 * @brief Eina_Value_Type callback: Sets up memory for an Eina_Content value.
 * @param type Unused.
 * @param mem Pointer to the memory to be initialized (will hold an Eina_Content*).
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_eina_value_type_content_setup(const Eina_Value_Type *type EINA_UNUSED, void *mem)
{
   memset(mem, 0, sizeof(Eina_Content*));
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Eina_Value_Type callback: Flushes (frees) an Eina_Content value.
 * @param type Unused.
 * @param mem Pointer to the memory holding the Eina_Content* to be freed.
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_eina_value_type_content_flush(const Eina_Value_Type *type EINA_UNUSED,
                                 void *mem)
{
   Eina_Content **content = mem;

   eina_content_free(*content);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Eina_Value_Type callback: Copies an Eina_Content value.
 * This involves incrementing the reference count of the Eina_Content object.
 * @param type Unused.
 * @param src Pointer to the source Eina_Content* container.
 * @param dst Pointer to the destination Eina_Content* container.
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_eina_value_type_content_copy(const Eina_Value_Type *type EINA_UNUSED, const void *src, void *dst)
{
   Eina_Content * const *srcc = src;
   Eina_Content **dstc = dst;

   *dstc = *srcc;
   _eina_content_ref(*dstc);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Eina_Value_Type callback: Compares two Eina_Content values.
 * Compares first by type string pointer, then by data content.
 * @param type Unused.
 * @param a Pointer to the first Eina_Content* container.
 * @param b Pointer to the second Eina_Content* container.
 * @return An integer less than, equal to, or greater than zero if the first content
 *         is found, respectively, to be less than, to match, or be greater than the second.
 *         Returns -1 if types are different, otherwise result of eina_rw_slice_compare.
 */
static int
_eina_value_type_content_compare(const Eina_Value_Type *type EINA_UNUSED, const void *a, const void *b)
{
   Eina_Content * const *ra = a;
   Eina_Content * const *rb = b;

   if ((*ra)->type != (*rb)->type)
     return -1;

   return eina_rw_slice_compare((*ra)->data, (*rb)->data);
}

/**
 * @internal
 * @brief Eina_Value_Type callback: Converts an Eina_Content value to another Eina_Value_Type.
 * Supports conversion to EINA_VALUE_TYPE_STRINGSHARE or EINA_VALUE_TYPE_STRING
 * if the content is "text/plain;charset=utf-8" or can be converted to it.
 * @param type Unused.
 * @param convert The target Eina_Value_Type.
 * @param type_mem Pointer to the Eina_Content* container to convert from.
 * @param convert_mem Pointer to the memory for the converted value.
 * @return EINA_TRUE on successful conversion, EINA_FALSE otherwise.
 */
static Eina_Bool
_eina_value_type_content_convert_to(const Eina_Value_Type *type EINA_UNUSED, const Eina_Value_Type *convert, const void *type_mem, void *convert_mem)
{
   Eina_Content * const *ra = type_mem;

   if (convert == EINA_VALUE_TYPE_STRINGSHARE ||
       convert == EINA_VALUE_TYPE_STRING)
     {
        const char *type = eina_content_type_get(*ra);
        if (eina_streq(type, "text/plain;charset=utf-8"))
          {
             Eina_Slice data = eina_content_data_get(*ra);
             return eina_value_type_pset(convert, convert_mem, &data.mem);
          }
        else
          {
             Eina_Iterator *iter = eina_content_possible_conversions(*ra);
             const char *type;

             EINA_ITERATOR_FOREACH(iter, type)
               {
                  if (eina_streq(type, "text/plain;charset=utf-8"))
                    {
                       Eina_Content *conv_result = eina_content_convert(*ra, type);

                       Eina_Slice data = eina_content_data_get(conv_result);
                       Eina_Bool success = eina_value_type_pset(convert, convert_mem, &data.mem);
                       eina_content_free(conv_result);
                       return success;
                    }
               }
             //create some fallback
             {
                char buf[128];
                char *tmp = (char*) &buf;
                snprintf(buf, sizeof(buf), "Content %p cannot be converted to \"text/plain;charset=utf-8\"", *ra);
                return eina_value_type_pset(convert, convert_mem, &tmp);
             }
          }
     }
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Eina_Value_Type callback: Converts from another Eina_Value_Type to Eina_Content.
 * This conversion is not currently supported.
 * @param type Unused.
 * @param convert Unused.
 * @param type_mem Unused.
 * @param convert_mem Unused.
 * @return Always EINA_FALSE.
 */
static Eina_Bool
_eina_value_type_content_convert_from(const Eina_Value_Type *type EINA_UNUSED, const Eina_Value_Type *convert EINA_UNUSED, void *type_mem EINA_UNUSED, const void *convert_mem EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Eina_Value_Type callback: Sets an Eina_Content value from a pointer.
 * Increments the reference count of the assigned Eina_Content.
 * @param type Unused.
 * @param mem Pointer to the Eina_Content* container to set.
 * @param ptr Pointer to an Eina_Content* to assign from.
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_eina_value_type_content_pset(const Eina_Value_Type *type EINA_UNUSED, void *mem, const void *ptr)
{
   Eina_Content * const *srcc = ptr;
   Eina_Content **dstc = mem;

   *dstc = *srcc;
   _eina_content_ref(*dstc);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Eina_Value_Type callback: Sets an Eina_Content value from a va_list.
 * Increments the reference count of the assigned Eina_Content.
 * @param type Unused.
 * @param mem Pointer to the Eina_Content* container to set.
 * @param args va_list containing the Eina_Content* to assign.
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_eina_value_type_content_vset(const Eina_Value_Type *type EINA_UNUSED, void *mem, va_list args)
{
   Eina_Content **dst = mem;
   Eina_Content *content = va_arg(args, Eina_Content*);

   *dst = content;
   _eina_content_ref(*dst);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Eina_Value_Type callback: Gets an Eina_Content value into a pointer.
 * Increments the reference count of the retrieved Eina_Content.
 * @param type Unused.
 * @param mem Pointer to the Eina_Content* container to get from.
 * @param ptr Pointer to an Eina_Content* to store the retrieved content.
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_eina_value_type_content_pget(const Eina_Value_Type *type EINA_UNUSED, const void *mem, void *ptr)
{
   Eina_Content * const *src = mem;
   Eina_Content **dst = ptr;

   *dst = *src;
   _eina_content_ref(*dst);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Definition of the Eina_Value_Type for Eina_Content.
 *
 * This structure provides all the necessary function pointers for Eina_Value
 * to manage Eina_Content objects, including setup, flush, copy, compare,
 * conversion, and get/set operations.
 */
EINA_API const Eina_Value_Type _EINA_VALUE_TYPE_CONTENT ={
  EINA_VALUE_TYPE_VERSION,
  sizeof(Eina_Content*),
  "Eina_Abstract_Content",
  _eina_value_type_content_setup,
  _eina_value_type_content_flush,
  _eina_value_type_content_copy,
  _eina_value_type_content_compare,
  _eina_value_type_content_convert_to,
  _eina_value_type_content_convert_from,
  _eina_value_type_content_vset,
  _eina_value_type_content_pset,
  _eina_value_type_content_pget
};

/**
 * @internal
 * @brief Frees an Eina_List of Eina_Content_Conversion_Node structures.
 * This function is used as a callback for eina_hash_free when destroying
 * the `conversion_callbacks` hash table. It iterates through the list,
 * frees the stringshared 'to' type, and then frees the node itself.
 * @param v A pointer to an Eina_List of Eina_Content_Conversion_Node.
 */
static void
_free_node(void *v)
{
   Eina_Content_Conversion_Node *n;
   EINA_LIST_FREE(v, n)
     {
        eina_stringshare_del(n->to);
        free(n);
     }
}

/**
 * @internal
 * @brief Initializes the Eina abstract content subsystem.
 * Registers the log domain, creates the hash table for conversion callbacks,
 * sets up the EINA_VALUE_TYPE_CONTENT, and registers default text converters.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
eina_abstract_content_init(void)
{
   _eina_abstract_content_log_domain = eina_log_domain_register("eina_abstract_content", "white");
   conversion_callbacks = eina_hash_stringshared_new(_free_node);

   EINA_VALUE_TYPE_CONTENT = &_EINA_VALUE_TYPE_CONTENT;

   // text/plain is assumed to be charset "US-ASCII"

   eina_content_converter_conversion_register("text/plain", "text/plain;charset=utf-8", _copy_converter);
   eina_content_converter_conversion_register("text/plain", "text/plain;charset=iso-8859-1", _copy_converter);
   eina_content_converter_conversion_register("text/plain;charset=iso-8859-1", "text/plain;charset=utf-8", _latin1_to_utf8_converter);
   eina_content_converter_conversion_register("text/plain;charset=iso-8859-1", "text/plain", _copy_converter);

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Shuts down the Eina abstract content subsystem.
 * Frees the hash table for conversion callbacks. The log domain will be
 * unregistered by Eina itself.
 * @return EINA_TRUE on success.
 */
Eina_Bool
eina_abstract_content_shutdown(void)
{
   eina_hash_free(conversion_callbacks);

   return EINA_TRUE;
}

Eina_Value*
eina_value_content_new(Eina_Content *content)
{
   Eina_Value *v = eina_value_new(EINA_VALUE_TYPE_CONTENT);

   if (!eina_value_pset(v, &content))
     return NULL;
   return v;
}


Eina_Value
eina_value_content_init(Eina_Content *content)
{
   Eina_Value v;

   EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_value_setup(&v, EINA_VALUE_TYPE_CONTENT), EINA_VALUE_EMPTY);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_value_pset(&v, &content), EINA_VALUE_EMPTY);

   return v;
}

Eina_Content*
eina_value_to_content(const Eina_Value *value)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL(eina_value_type_get(value) == EINA_VALUE_TYPE_CONTENT, NULL);
   Eina_Content *result = calloc(1, sizeof(Eina_Content));
   if (!eina_value_pget(value, &result))
     {
        free(result);
        return NULL;
     }
   return result;
}
