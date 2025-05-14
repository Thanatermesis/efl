#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Elementary.h>
#include "elm_priv.h"

// This class store the size information in a compressed array and unpack it
// when necessary. It does maintain a cache of up to 3 uncompressed slot.
// That cache could get dropped when the application is entering the 'pause'
// state.

#define EFL_UI_EXACT_MODEL_CONTENT 1024
#define EFL_UI_EXACT_MODEL_CONTENT_LENGTH (EFL_UI_EXACT_MODEL_CONTENT * sizeof (unsigned int))

// For now only vertical logic is implemented. Horizontal list and grid are not supported.

typedef struct _Efl_Ui_Exact_Model_Data Efl_Ui_Exact_Model_Data;
/**
 * @brief Private data structure for the Efl_Ui_Exact_Model.
 *
 * This structure holds all the data necessary for the exact model to function,
 * including compressed size information, total size, and cached slots for
 * uncompressed data.
 */
struct _Efl_Ui_Exact_Model_Data
{
   Efl_Ui_Exact_Model_Data *parent; /**< Pointer to the parent model's data if this is a child model. */

   struct {
      Eina_List *width;  /**< List of Eina_Binbuf containing compressed width data for segments. */
      Eina_List *height; /**< List of Eina_Binbuf containing compressed height data for segments. */
   } compressed; /**< Holds lists of compressed data segments. */

   struct {
      unsigned int width;  /**< Total width accumulated from all items (max width in vertical list). */
      unsigned int height; /**< Total height accumulated from all items. */
   } total_size; /**< Stores the overall dimensions calculated from item sizes. */

   struct {
      unsigned int *width;        /**< Buffer for uncompressed width data of the current slot. */
      unsigned int *height;       /**< Buffer for uncompressed height data of the current slot. */
      unsigned int start_offset;  /**< The starting index of the items covered by this slot, divided by EFL_UI_EXACT_MODEL_CONTENT. */
      unsigned short usage;       /**< Usage counter for LRU cache replacement strategy. */
      Eina_Bool defined : 1;      /**< Flag indicating if this slot contains valid data. */
      struct {
         Eina_Bool width : 1;     /**< Flag indicating if the width data in this slot is currently decompressed. */
         Eina_Bool height : 1;    /**< Flag indicating if the height data in this slot is currently decompressed. */
      } decompressed; /**< Flags to track decompression status of width and height data. */
   } slot[3]; /**< Cache slots for uncompressed data. Each slot can hold EFL_UI_EXACT_MODEL_CONTENT items. */
};

static Efl_Object *
_efl_ui_exact_model_efl_object_constructor(Eo *obj, Efl_Ui_Exact_Model_Data *pd)
{
   Eo *parent = efl_parent_get(obj);

   if (parent && efl_isa(parent, EFL_UI_EXACT_MODEL_CLASS))
     pd->parent = efl_data_scope_get(efl_parent_get(obj), EFL_UI_EXACT_MODEL_CLASS);

   return efl_constructor(efl_super(obj, EFL_UI_EXACT_MODEL_CLASS));
}

/**
 * @brief Compresses a buffer of size data and stores it in the compressed list.
 *
 * This function takes a buffer of uncompressed size data (either width or height),
 * compresses it using Emile (LZ4), and then stores or updates the corresponding
 * compressed Eina_Binbuf in the provided Eina_List. The list is indexed by
 * `index / EFL_UI_EXACT_MODEL_CONTENT`.
 *
 * @param index The starting item index for which the buffer data is relevant.
 *              This determines the position in the `compressed` list.
 * @param compressed The Eina_List holding Eina_Binbuf(s) of compressed data.
 *                   This list will be modified.
 * @param buffer Pointer to the uncompressed data (array of unsigned int) to be compressed.
 *               The buffer is assumed to be of size EFL_UI_EXACT_MODEL_CONTENT_LENGTH.
 * @return The (potentially modified) `compressed` Eina_List.
 */
static Eina_List *
_efl_ui_exact_model_slot_compress(unsigned int index, Eina_List *compressed, unsigned int *buffer)
{
   unsigned int list_index = index / EFL_UI_EXACT_MODEL_CONTENT;
   static Eina_Binbuf *z = NULL;
   Eina_Binbuf *cbuf;
   Eina_Binbuf *tbuf;
   Eina_List *l = NULL;
   unsigned int i;

   l = eina_list_nth_list(compressed, list_index);

   tbuf = eina_binbuf_manage_new((unsigned char *) buffer, EFL_UI_EXACT_MODEL_CONTENT_LENGTH, EINA_TRUE);
   if (!tbuf) return compressed;

   cbuf = emile_compress(tbuf, EMILE_LZ4, EMILE_COMPRESSOR_FAST);
   eina_binbuf_free(tbuf);
   if (!cbuf) return compressed;

   // Make sure the list has all the buffer up to the needed one filled with valid data
   if (list_index)
     {
        // Create the compressed zero buffer once.
        if (!z)
          {
             unsigned char *zmem;

             zmem = calloc(EFL_UI_EXACT_MODEL_CONTENT, sizeof (unsigned int));
             if (!zmem)
               {
                  if (cbuf) eina_binbuf_free(cbuf);
                  return compressed;
               }

             tbuf = eina_binbuf_manage_new(zmem, EFL_UI_EXACT_MODEL_CONTENT_LENGTH, EINA_TRUE);
             if (!tbuf)
               {
                  if (cbuf) eina_binbuf_free(cbuf);
                  if (zmem) free(zmem);
                  return compressed;
               }

             z = emile_compress(tbuf, EMILE_LZ4, EMILE_COMPRESSOR_FAST);

             eina_binbuf_free(tbuf);
             free(zmem);
          }

        // Fill the list all the way to the needed index with buffer full of zero
        for (i = 0; i < list_index; i++)
          {
             compressed = eina_list_append(compressed, z);
          }
        l = eina_list_last(compressed);
     }

   // Replace older buffer by newer buffer
   tbuf = eina_list_data_get(l);
   compressed = eina_list_prepend_relative(compressed, l, cbuf);
   compressed = eina_list_remove_list(compressed, l);
   if (tbuf != z) eina_binbuf_free(tbuf);

   return compressed;
}

/**
 * @brief Expands a compressed data segment from the list into a buffer.
 *
 * Retrieves a compressed Eina_Binbuf from the given `list` at `list_index`,
 * decompresses it using Emile (LZ4), and stores the result in `buffer`.
 * If `buffer` is NULL, it will be allocated. If the `list_index` is out of
 * bounds for `list`, the `buffer` is zeroed out.
 *
 * @param list_index The index in the `list` from which to retrieve the compressed data.
 * @param buffer The buffer to store the decompressed data. If NULL, it's allocated.
 *               Must be large enough for EFL_UI_EXACT_MODEL_CONTENT_LENGTH bytes.
 * @param list The Eina_List containing Eina_Binbuf(s) of compressed data.
 * @return Pointer to the `buffer` containing the decompressed data. The caller
 *         is responsible for freeing this buffer if it was allocated by this function
 *         (i.e., if the input `buffer` was NULL and a new one was created).
 *         However, in the context of its usage within this file, the buffer management
 *         is typically handled by the slot structure.
 */
static unsigned int *
_efl_ui_exact_model_buffer_expand(unsigned int list_index, unsigned int *buffer, Eina_List *list)
{
   Eina_Binbuf *tmp;
   Eina_List *l = NULL;

   if (!buffer) buffer = malloc(EFL_UI_EXACT_MODEL_CONTENT_LENGTH);

   l = eina_list_nth_list(list, list_index);

   // Check if the data is in the list
   if (!l)
     {
        // Not found -> everything is assumed to be zero
        memset(buffer, 0, EFL_UI_EXACT_MODEL_CONTENT_LENGTH);
        return buffer;
     }

   // Found -> expand in buffer
   tmp = eina_binbuf_manage_new((unsigned char*) buffer, EFL_UI_EXACT_MODEL_CONTENT_LENGTH, EINA_TRUE);
   emile_expand(eina_list_data_get(l), tmp, EMILE_LZ4);
   eina_binbuf_free(tmp);

   return buffer;
}

/**
 * @brief Finds or allocates a cache slot for a given item index.
 *
 * This function implements an LRU (Least Recently Used) cache policy for the
 * uncompressed data slots. It tries to find an existing slot that covers the
 * `index`. If not found, it selects a slot for replacement (either an unused
 * one or the LRU one). If a slot is replaced, its modified data (if any) is
 * compressed back into storage. The selected slot is then prepared for the
 * new `index`. If `width_get` or `height_get` are true, the corresponding
 * data for the new slot is decompressed.
 *
 * @param pd Pointer to the Efl_Ui_Exact_Model_Data instance.
 * @param index The item index for which a slot is needed.
 * @param width_get If EINA_TRUE, ensure the width data for the slot is decompressed.
 * @param height_get If EINA_TRUE, ensure the height data for the slot is decompressed.
 * @return The index of the cache slot (0 to 2) that now covers the requested `index`.
 */
static unsigned char
_efl_ui_exact_model_slot_find(Efl_Ui_Exact_Model_Data *pd, unsigned int index,
                              Eina_Bool width_get, Eina_Bool height_get)
{
   unsigned char lookup;
   unsigned char found = EINA_C_ARRAY_LENGTH(pd->parent->slot);

   for (lookup = 0; lookup < EINA_C_ARRAY_LENGTH(pd->parent->slot); lookup++)
     {
        // Check if the slot has valid content
        if (!pd->parent->slot[lookup].defined)
          continue;
        if (pd->parent->slot[lookup].start_offset <= index &&
            index < pd->parent->slot[lookup].start_offset + EFL_UI_EXACT_MODEL_CONTENT)
          found = lookup;
        // Reduce usage to find unused slot.
        if (pd->parent->slot[lookup].usage > 0)
          pd->parent->slot[lookup].usage--;
     }

   // Do we need to find a new slot?
   if (found == EINA_C_ARRAY_LENGTH(pd->parent->slot))
     {
        found = 0;
        for (lookup = 0; lookup < EINA_C_ARRAY_LENGTH(pd->parent->slot); lookup++)
          {
             if (!pd->parent->slot[lookup].defined)
               {
                  // Found an empty slot, let's use that.
                  found = lookup;
                  break;
               }
             if (pd->parent->slot[lookup].usage < pd->parent->slot[found].usage)
               found = lookup;
          }

        // Commit change back to the stored buffer list
        if (pd->parent->slot[found].defined &&
            (pd->parent->slot[found].width ||
             pd->parent->slot[found].height))
          {
             if (pd->parent->slot[found].width &&
                 pd->parent->slot[found].decompressed.width)
               pd->parent->compressed.width = _efl_ui_exact_model_slot_compress(index,
                                                                                pd->parent->compressed.width,
                                                                                pd->parent->slot[found].width);
             if (pd->parent->slot[found].height &&
                 pd->parent->slot[found].decompressed.height)
               pd->parent->compressed.height = _efl_ui_exact_model_slot_compress(index,
                                                                                 pd->parent->compressed.height,
                                                                                 pd->parent->slot[found].height);
          }

        pd->parent->slot[found].defined = EINA_TRUE;
        pd->parent->slot[found].decompressed.width = EINA_FALSE;
        pd->parent->slot[found].decompressed.height = EINA_FALSE;
        pd->parent->slot[found].start_offset = index / EFL_UI_EXACT_MODEL_CONTENT;
     }

   // Increase usage of the returned slot for now
   pd->parent->slot[found].usage++;

   // Unpack the data if requested
   if (width_get && !pd->parent->slot[found].decompressed.width)
     {
        pd->parent->slot[found].width = _efl_ui_exact_model_buffer_expand(pd->parent->slot[found].start_offset,
                                                                          pd->parent->slot[found].width,
                                                                          pd->parent->compressed.width);
        pd->parent->slot[found].decompressed.width = EINA_TRUE;
     }
   if (height_get && !pd->parent->slot[found].decompressed.height)
     {
        pd->parent->slot[found].height = _efl_ui_exact_model_buffer_expand(pd->parent->slot[found].start_offset,
                                                                          pd->parent->slot[found].height,
                                                                          pd->parent->compressed.height);
        pd->parent->slot[found].decompressed.height = EINA_TRUE;
     }

   return found;
}

/**
 * @brief Sets a property value for the model or a specific item.
 *
 * Handles setting properties like "self.w" (item width) and "self.h" (item height).
 * For item-specific properties, it finds the appropriate cache slot, updates the
 * size in the uncompressed buffer, and updates total size calculations.
 * Read-only properties like "total.w", "total.h", "item.w", "item.h" will
 * result in a rejected future.
 *
 * @param obj The Efl_Ui_Exact_Model object.
 * @param pd Pointer to the Efl_Ui_Exact_Model_Data instance.
 * @param property The name of the property to set (e.g., "self.w", "self.h").
 * @param value The Eina_Value containing the new value for the property.
 * @return An Eina_Future that resolves with the set value on success, or is
 *         rejected with an error code (e.g., EFL_MODEL_ERROR_INCORRECT_VALUE,
 *         EFL_MODEL_ERROR_READ_ONLY) on failure.
 */
static Eina_Future *
_efl_ui_exact_model_efl_model_property_set(Eo *obj, Efl_Ui_Exact_Model_Data *pd,
                                           const char *property, Eina_Value *value)
{
   if (pd->parent)
    {
       if (eina_streq(property, _efl_model_property_selfw))
         {
            unsigned int index;
            unsigned char found;

            index = efl_composite_model_index_get(obj);
            found = _efl_ui_exact_model_slot_find(pd, index, EINA_TRUE, EINA_FALSE);
            if (!eina_value_uint_convert(value, &pd->parent->slot[found].width[index % EFL_UI_EXACT_MODEL_CONTENT]))
              return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);
            // We succeeded so let's update the max total size width (As we only handle vertical list case at the moment)
            if (pd->parent->total_size.width < pd->parent->slot[found].width[index % EFL_UI_EXACT_MODEL_CONTENT])
              pd->parent->total_size.width = pd->parent->slot[found].width[index % EFL_UI_EXACT_MODEL_CONTENT];
            return efl_loop_future_resolved(obj, eina_value_uint_init(pd->parent->slot[found].width[index % EFL_UI_EXACT_MODEL_CONTENT]));
         }
       if (eina_streq(property, _efl_model_property_selfh))
         {
            unsigned int old_value;
            unsigned int index;
            unsigned char found;

            index = efl_composite_model_index_get(obj);
            found = _efl_ui_exact_model_slot_find(pd, index, EINA_FALSE, EINA_TRUE);
            old_value = pd->parent->slot[found].height[index % EFL_UI_EXACT_MODEL_CONTENT];
            if (!eina_value_uint_convert(value, &pd->parent->slot[found].height[index % EFL_UI_EXACT_MODEL_CONTENT]))
              return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_INCORRECT_VALUE);
            // We succeeded so let's update the total size
            pd->parent->total_size.height += pd->parent->slot[found].height[index % EFL_UI_EXACT_MODEL_CONTENT] - old_value;
            return efl_loop_future_resolved(obj, eina_value_uint_init(pd->parent->slot[found].height[index % EFL_UI_EXACT_MODEL_CONTENT]));
         }
       // The following property are calculated by the model and so READ_ONLY
       if (eina_streq(property, _efl_model_property_totalh))
         {
            return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_READ_ONLY);
         }
       if (eina_streq(property, _efl_model_property_totalw))
         {
            return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_READ_ONLY);
         }
    }

   if (eina_streq(property, _efl_model_property_itemw))
     {
        // The exact model can not guess a general item size if asked
        // and should refuse to remember anything like that.
        return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_READ_ONLY);
     }
   if (eina_streq(property, _efl_model_property_itemh))
     {
        // The exact model can not guess a general item size if asked
        // and should refuse to remember anything like that.
        return efl_loop_future_rejected(obj, EFL_MODEL_ERROR_READ_ONLY);
     }

   return efl_model_property_set(efl_super(obj, EFL_UI_EXACT_MODEL_CLASS), property, value);
}

/**
 * @brief Gets a property value from the model or a specific item.
 *
 * Handles getting properties like "self.w" (item width), "self.h" (item height),
 * "total.w" (total width of all items), and "total.h" (total height of all items).
 * For item-specific properties, it finds the appropriate cache slot and retrieves
 * the size from the uncompressed buffer.
 * For "item.w" and "item.h" (generic item size), it returns an error as the
 * exact model does not assume a uniform item size.
 *
 * @param obj The Efl_Ui_Exact_Model object.
 * @param pd Pointer to the Efl_Ui_Exact_Model_Data instance.
 * @param property The name of the property to get (e.g., "self.w", "total.h").
 * @return An Eina_Value containing the property value. For "item.w" and "item.h",
 *         it returns an Eina_Value of type error with EAGAIN. For other properties,
 *         it returns the corresponding unsigned integer value.
 */
static Eina_Value *
_efl_ui_exact_model_efl_model_property_get(const Eo *obj, Efl_Ui_Exact_Model_Data *pd,
                                           const char *property)
{
   if (pd->parent)
     {
        if (eina_streq(property, _efl_model_property_selfw))
          {
             unsigned int index;
             unsigned char found;

             index = efl_composite_model_index_get(obj);
             found = _efl_ui_exact_model_slot_find(pd, index, EINA_TRUE, EINA_FALSE);
             return eina_value_uint_new(pd->parent->slot[found].width[index % EFL_UI_EXACT_MODEL_CONTENT]);
          }
        if (eina_streq(property, _efl_model_property_selfh))
          {
             unsigned int index;
             unsigned char found;

             index = efl_composite_model_index_get(obj);
             found = _efl_ui_exact_model_slot_find(pd, index, EINA_FALSE, EINA_TRUE);
             return eina_value_uint_new(pd->parent->slot[found].height[index % EFL_UI_EXACT_MODEL_CONTENT]);
          }
     }
   if (eina_streq(property, _efl_model_property_totalh))
     {
        return eina_value_uint_new(pd->total_size.height);
     }
   if (eina_streq(property, _efl_model_property_totalw))
     {
        return eina_value_uint_new(pd->total_size.width);
     }
   if (eina_streq(property, _efl_model_property_itemw))
     {
        // The exact model can not guess a general item size if asked.
        return eina_value_error_new(EAGAIN);
     }
   if (eina_streq(property, _efl_model_property_itemh))
     {
        // The exact model can not guess a general item size if asked.
        return eina_value_error_new(EAGAIN);
     }
   return efl_model_property_get(efl_super(obj, EFL_UI_EXACT_MODEL_CLASS), property);
}

#include "efl_ui_exact_model.eo.c"
