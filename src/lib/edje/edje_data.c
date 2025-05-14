#include "edje_private.h"

/** @file edje_data.c
 * @brief This file defines the Eet_Data_Descriptor structures used for
 * serializing and deserializing Edje theme files (.edj).
 *
 * These descriptors map C structures to a format that Eet can store in a file
 * and retrieve later. This allows Edje to save and load its complex theme
 * data, including parts, programs, styles, images, fonts, and other resources.
 */

// EAPI (Exported API) Eet_Data_Descriptors. These are top-level descriptors
// for major components of an Edje file.
/** @brief Eet_Data_Descriptor for the main Edje_File structure. */
EAPI Eet_Data_Descriptor * _edje_edd_edje_file = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Collection structures. */
EAPI Eet_Data_Descriptor * _edje_edd_edje_part_collection = NULL;
/** @brief Eet_Data_Descriptor for Edje_Color_Class_Info structures. */
EAPI Eet_Data_Descriptor * _edje_edd_edje_color_class_info = NULL;

// Static Eet_Data_Descriptors. These are used internally to define
// sub-components or specific data types within the Edje file structure.

/** @brief Eet_Data_Descriptor for Edje_String (localizable string with ID). */
Eet_Data_Descriptor *_edje_edd_edje_string = NULL;
/** @brief Eet_Data_Descriptor for Edje_Style. */
Eet_Data_Descriptor *_edje_edd_edje_style = NULL;
/** @brief Eet_Data_Descriptor for Edje_Style_Tag (key-value pair for styles). */
Eet_Data_Descriptor *_edje_edd_edje_style_tag = NULL;
/** @brief Eet_Data_Descriptor for Edje_Color_Tree_Node (nodes in the color class hierarchy). */
Eet_Data_Descriptor *_edje_edd_edje_color_tree_node = NULL;
/** @brief Eet_Data_Descriptor for Edje_Color_Class. */
Eet_Data_Descriptor *_edje_edd_edje_color_class = NULL;
/** @brief Eet_Data_Descriptor for Edje_Text_Class. */
Eet_Data_Descriptor *_edje_edd_edje_text_class = NULL;
/** @brief Eet_Data_Descriptor for Edje_Size_Class. */
Eet_Data_Descriptor *_edje_edd_edje_size_class = NULL;
/** @brief Eet_Data_Descriptor for Edje_External_Directory. */
Eet_Data_Descriptor *_edje_edd_edje_external_directory = NULL;
/** @brief Eet_Data_Descriptor for Edje_External_Directory_Entry. */
Eet_Data_Descriptor *_edje_edd_edje_external_directory_entry = NULL;
/** @brief Eet_Data_Descriptor for Edje_Font_Directory_Entry. */
Eet_Data_Descriptor *_edje_edd_edje_font_directory_entry = NULL;
/** @brief Eet_Data_Descriptor for Edje_Image_Hash (mapping image IDs). */
Eet_Data_Descriptor *_edje_edd_edje_image_id_hash = NULL;
/** @brief Eet_Data_Descriptor for Edje_Image_Directory. */
Eet_Data_Descriptor *_edje_edd_edje_image_directory = NULL;
/** @brief Eet_Data_Descriptor for Edje_Image_Directory_Entry. */
Eet_Data_Descriptor *_edje_edd_edje_image_directory_entry = NULL;
/** @brief Eet_Data_Descriptor for Edje_Image_Directory_Set. */
Eet_Data_Descriptor *_edje_edd_edje_image_directory_set = NULL;
/** @brief Eet_Data_Descriptor for Edje_Image_Directory_Set_Entry. */
Eet_Data_Descriptor *_edje_edd_edje_image_directory_set_entry = NULL;
/** @brief Eet_Data_Descriptor for Edje_Vector_Directory_Entry (for vector graphics like SVG). */
Eet_Data_Descriptor *_edje_edd_edje_vector_directory_entry = NULL;
/** @brief Eet_Data_Descriptor for Edje_Model_Directory (for 3D models). */
Eet_Data_Descriptor *_edje_edd_edje_model_directory = NULL;
/** @brief Eet_Data_Descriptor for Edje_Model_Directory_Entry. */
Eet_Data_Descriptor *_edje_edd_edje_model_directory_entry = NULL;
/** @brief Eet_Data_Descriptor for Edje_Limit (named numerical limits). */
Eet_Data_Descriptor *_edje_edd_edje_limit = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Limit. */
Eet_Data_Descriptor *_edje_edd_edje_limit_pointer = NULL;
/** @brief Eet_Data_Descriptor for Edje_Sound_Sample. */
Eet_Data_Descriptor *_edje_edd_edje_sound_sample = NULL;
/** @brief Eet_Data_Descriptor for Edje_Mo (translation file information). */
Eet_Data_Descriptor *_edje_edd_edje_translation_file = NULL;
/** @brief Eet_Data_Descriptor for Edje_Sound_Tone. */
Eet_Data_Descriptor *_edje_edd_edje_sound_tone = NULL;
/** @brief Eet_Data_Descriptor for Edje_Sound_Directory. */
Eet_Data_Descriptor *_edje_edd_edje_sound_directory = NULL;
/** @brief Eet_Data_Descriptor for Edje_Mo_Directory (directory of translation files). */
Eet_Data_Descriptor *_edje_edd_edje_mo_directory = NULL;
/** @brief Eet_Data_Descriptor for Edje_Vibration_Sample. */
Eet_Data_Descriptor *_edje_edd_edje_vibration_sample = NULL;
/** @brief Eet_Data_Descriptor for Edje_Vibration_Directory. */
Eet_Data_Descriptor *_edje_edd_edje_vibration_directory = NULL;
/** @brief Eet_Data_Descriptor for Edje_Program. */
Eet_Data_Descriptor *_edje_edd_edje_program = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Program. */
Eet_Data_Descriptor *_edje_edd_edje_program_pointer = NULL;
/** @brief Eet_Data_Descriptor for Edje_Program_Target. */
Eet_Data_Descriptor *_edje_edd_edje_program_target = NULL;
/** @brief Eet_Data_Descriptor for Edje_Program_After (dependencies between programs). */
Eet_Data_Descriptor *_edje_edd_edje_program_after = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Collection_Directory_Entry. */
Eet_Data_Descriptor *_edje_edd_edje_part_collection_directory_entry = NULL;
/** @brief Eet_Data_Descriptor for Edje_Pack_Element (elements in box/table layouts). */
Eet_Data_Descriptor *_edje_edd_edje_pack_element = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Pack_Element. */
Eet_Data_Descriptor *_edje_edd_edje_pack_element_pointer = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part. */
Eet_Data_Descriptor *_edje_edd_edje_part = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part. */
Eet_Data_Descriptor *_edje_edd_edje_part_pointer = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Allowed_Seat (seat restrictions for a part). */
Eet_Data_Descriptor *_edje_edd_edje_part_allowed_seat = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Allowed_Seat. */
Eet_Data_Descriptor *_edje_edd_edje_part_allowed_seat_pointer = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Variant (variant part descriptions). */
Eet_Data_Descriptor *_edje_edd_edje_part_description_variant = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Common (rectangle type). */
Eet_Data_Descriptor *_edje_edd_edje_part_description_rectangle = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Snapshot. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_snapshot = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Common (spacer type). */
Eet_Data_Descriptor *_edje_edd_edje_part_description_spacer = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Common (swallow type). */
Eet_Data_Descriptor *_edje_edd_edje_part_description_swallow = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Common (group type). */
Eet_Data_Descriptor *_edje_edd_edje_part_description_group = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Image. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_image = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Proxy. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_proxy = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Text. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_text = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Text (textblock type). */
Eet_Data_Descriptor *_edje_edd_edje_part_description_textblock = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Box. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_box = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Table. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_table = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_External. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_external = NULL;
/** @brief Eet_Data_Descriptor for a list of Edje_Part_Description_Variant. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_variant_list = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_Common (rectangle). */
Eet_Data_Descriptor *_edje_edd_edje_part_description_rectangle_pointer = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_Snapshot. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_snapshot_pointer = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_Common (spacer). */
Eet_Data_Descriptor *_edje_edd_edje_part_description_spacer_pointer = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_Common (swallow). */
Eet_Data_Descriptor *_edje_edd_edje_part_description_swallow_pointer = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_Common (group). */
Eet_Data_Descriptor *_edje_edd_edje_part_description_group_pointer = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_Image. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_image_pointer = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_Proxy. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_proxy_pointer = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_Text. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_text_pointer = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_Text (textblock). */
Eet_Data_Descriptor *_edje_edd_edje_part_description_textblock_pointer = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_Box. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_box_pointer = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_Table. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_table_pointer = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_External. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_external_pointer = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Spec_Filter_Data (filter data key-value). */
Eet_Data_Descriptor *_edje_edd_edje_part_description_filter_data = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Image_Id (image ID with set flag). */
Eet_Data_Descriptor *_edje_edd_edje_part_image_id = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Image_Id. */
Eet_Data_Descriptor *_edje_edd_edje_part_image_id_pointer = NULL;
/** @brief Eet_Data_Descriptor for Edje_External_Param (parameters for external parts). */
Eet_Data_Descriptor *_edje_edd_edje_external_param = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Limit (limit applied to a specific part). */
Eet_Data_Descriptor *_edje_edd_edje_part_limit = NULL;
/** @brief Eet_Data_Descriptor for Edje_Physics_Face (physics properties of a face). */
Eet_Data_Descriptor *_edje_edd_edje_physics_face = NULL;
/** @brief Eet_Data_Descriptor for Edje_Map_Color (color definition for map points). */
Eet_Data_Descriptor *_edje_edd_edje_map_colors = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Map_Color. */
Eet_Data_Descriptor *_edje_edd_edje_map_colors_pointer = NULL;
/** @brief Eet_Data_Descriptor for Edje_Gfx_Filter (gfx filter definition). */
Eet_Data_Descriptor *_edje_edd_edje_filter = NULL;
/** @brief Eet_Data_Descriptor for Edje_Gfx_Filter_Directory. */
Eet_Data_Descriptor *_edje_edd_edje_filter_directory = NULL;
/** @brief Eet_Data_Descriptor for Edje_Part_Description_Vector. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_vector = NULL;
/** @brief Eet_Data_Descriptor for a pointer to Edje_Part_Description_Vector. */
Eet_Data_Descriptor *_edje_edd_edje_part_description_vector_pointer = NULL;


/**
 * @brief Macro to define a mempool, allocator, and deallocator for a specific Edje part description type.
 *
 * This macro simplifies the creation of memory pools for different types of
 * Edje_Part_Description_Common derived structures. It initializes common fields
 * like `clip_to_id`, `map.zoom.x`, `map.zoom.y`, and `map.zoom.id_center`
 * to default values upon allocation.
 *
 * @param Type The suffix for the mempool variable (e.g., RECTANGLE for _emp_RECTANGLE).
 * @param Minus The suffix for the allocator/deallocator functions (e.g., rectangle for mem_alloc_rectangle).
 */
/* allocate a description struct.
 * this initializes clip_to_id as this field will not be present in most
 * edje files.
 */
#define EMP(Type, Minus)                            \
  EAPI Eina_Mempool *_emp_##Type = NULL;            \
                                                    \
  static void *                                     \
  mem_alloc_##Minus(size_t size)                    \
  {                                                 \
     Edje_Part_Description_Common *data;            \
                                                    \
     data = eina_mempool_malloc(_emp_##Type, size); \
     memset(data, 0, size);                         \
     data->clip_to_id = -1;                         \
     data->map.zoom.x = data->map.zoom.y = 1.0;     \
     data->map.zoom.id_center = -1;                 \
     return data;                                   \
  }                                                 \
                                                    \
  static void                                       \
    mem_free_##Minus(void *data)                    \
  {                                                 \
     eina_mempool_free(_emp_##Type, data);          \
  }

/** @brief Mempool for Edje_Part_Description_Common of type RECTANGLE. */
EMP(RECTANGLE, rectangle)
/** @brief Mempool for Edje_Part_Description_Text. */
EMP(TEXT, text)
/** @brief Mempool for Edje_Part_Description_Image. */
EMP(IMAGE, image)
/** @brief Mempool for Edje_Part_Description_Proxy. */
EMP(PROXY, proxy)
/** @brief Mempool for Edje_Part_Description_Common of type SWALLOW. */
EMP(SWALLOW, swallow)
/** @brief Mempool for Edje_Part_Description_Text of type TEXTBLOCK. */
EMP(TEXTBLOCK, textblock)
/** @brief Mempool for Edje_Part_Description_Common of type GROUP. */
EMP(GROUP, group)
/** @brief Mempool for Edje_Part_Description_Box. */
EMP(BOX, box)
/** @brief Mempool for Edje_Part_Description_Table. */
EMP(TABLE, table)
/** @brief Mempool for Edje_Part_Description_External. */
EMP(EXTERNAL, external)
/** @brief Mempool for Edje_Part_Description_Common of type SPACER. */
EMP(SPACER, spacer)
/** @brief Mempool for Edje_Part_Description_Snapshot. */
EMP(SNAPSHOT, snapshot)
/** @brief Mempool for Edje_Part_Description_Vector. */
EMP(VECTOR, vector)
#undef EMP

/** @brief Mempool for Edje_Part structures. */
EAPI Eina_Mempool *_emp_part = NULL;

/**
 * @brief Allocates memory for an Edje_Part from its mempool.
 * Initializes `dragable.threshold_id` to -1.
 * @param size The size of the memory to allocate (should be sizeof(Edje_Part)).
 * @return A pointer to the allocated Edje_Part, or NULL on failure.
 */
static void *
mem_alloc_part(size_t size)
{
   Edje_Part *ep;

   ep = eina_mempool_malloc(_emp_part, size);
   memset(ep, 0, size);
   // This value need to be defined for older file that didn't provide it
   // as it should -1 by default instead of 0.
   ep->dragable.threshold_id = -1;
   return ep;
}

/**
 * @brief Frees memory previously allocated for an Edje_Part back to its mempool.
 * @param data Pointer to the Edje_Part to free.
 */
static void
mem_free_part(void *data)
{
   eina_mempool_free(_emp_part, data);
}

/**
 * @brief Macro to safely free an Eet_Data_Descriptor and set its pointer to NULL.
 * @param eed The Eet_Data_Descriptor pointer to free.
 */
#define FREED(eed)                      \
  if (eed)                              \
    {                                   \
       eet_data_descriptor_free((eed)); \
       (eed) = NULL;                    \
    }

/**
 * @brief Structure to map Edje_Part_Type enum values to their string representations.
 * Used for converting part types during serialization/deserialization of variant descriptions.
 */
struct
{
   Edje_Part_Type type; /**< The enum value of the part type. */
   const char    *name; /**< The string name of the part type. */
} variant_convertion[] = {
   { EDJE_PART_TYPE_RECTANGLE, "rectangle" },
   { EDJE_PART_TYPE_SWALLOW, "swallow" },
   { EDJE_PART_TYPE_GROUP, "group" },
   { EDJE_PART_TYPE_IMAGE, "image" },
   { EDJE_PART_TYPE_TEXT, "text" },
   { EDJE_PART_TYPE_TEXTBLOCK, "textblock" },
   { EDJE_PART_TYPE_BOX, "box" },
   { EDJE_PART_TYPE_TABLE, "table" },
   { EDJE_PART_TYPE_EXTERNAL, "external" },
   { EDJE_PART_TYPE_PROXY, "proxy" },
   { EDJE_PART_TYPE_SPACER, "spacer" },
   { EDJE_PART_TYPE_SNAPSHOT, "snapshot" },
   { EDJE_PART_TYPE_VECTOR, "vector" }
};

static const char *
_edje_description_variant_type_get(const void *data, Eina_Bool *unknow EINA_UNUSED)
{
   const unsigned char *type;
   unsigned int i;

   type = data;

   for (i = 0; i < (sizeof (variant_convertion) / sizeof (variant_convertion[0])); ++i)
     if (*type == variant_convertion[i].type)
       return variant_convertion[i].name;

   return NULL;
}

/**
 * @brief Sets the part type in a data structure based on its string name.
 * This function is used by Eet to set the type of a variant part description
 * when deserializing from a string representation.
 * @param type The string name of the part type (e.g., "rectangle", "text").
 * @param data Pointer to an unsigned char where the Edje_Part_Type enum value will be stored.
 * @param unknow Eina_UNUSED parameter, not used.
 * @return EINA_TRUE if the type string was recognized and set, EINA_FALSE otherwise.
 */
static Eina_Bool
_edje_description_variant_type_set(const char *type, void *data, Eina_Bool unknow EINA_UNUSED)
{
   unsigned char *dt;
   unsigned int i;

   dt = data;

   for (i = 0; i < (sizeof (variant_convertion) / sizeof (variant_convertion[0])); ++i)
     if (!strcmp(variant_convertion[i].name, type))
       {
          *dt = variant_convertion[i].type;
          return EINA_TRUE;
       }

   return EINA_FALSE;
}

/** @brief Size of the memory-mapped string data block from an Edje file.
 * Used for a workaround with ancient Edje files where strings might be direct pointers
 * into a mapped region rather than separately allocated.
 */
size_t  _edje_data_string_mapping_size = 0;
/** @brief Pointer to the start of the memory-mapped string data block.
 * @see _edje_data_string_mapping_size
 */
void   *_edje_data_string_mapping = NULL;

/**
 * @brief Custom hash add function for Eet_Data_Descriptor.
 * This function includes a workaround for ancient Edje files. If a string key
 * falls within a known memory-mapped region (_edje_data_string_mapping),
 * it uses eina_hash_direct_add to avoid duplicating the key string. Otherwise,
 * it uses the standard eina_hash_add.
 *
 * @param hash The Eina_Hash to add to. If NULL, a new small string hash is created.
 * @param key The string key to add.
 * @param data The data associated with the key.
 * @return The hash table (possibly newly created), or NULL on failure to create a new hash.
 */
static Eina_Hash *
_edje_eina_hash_add_alloc(Eina_Hash *hash,
                          const char *key,
                          void *data)
{
   if (!hash)
     hash = eina_hash_string_small_new(free);

   if (!hash)
     return NULL;

   // XXX: ancient edje file workaround
   if ((_edje_data_string_mapping) &&
       ((key >=  (char *)_edje_data_string_mapping) &&
        (key <  ((char *)_edje_data_string_mapping + _edje_data_string_mapping_size))))
     eina_hash_direct_add(hash, key, data);
   else
     eina_hash_add(hash, key, data);
   return hash;
}

/**
 * @brief Shuts down the Edje data descriptor system.
 * This function frees all globally allocated Eet_Data_Descriptor structures
 * that were initialized by _edje_edd_init().
 * FIXME: remove EAPI when edje_convert goes, should be static.
 */
// FIXME: remove EAPI when edje_convert goes
EAPI void
_edje_edd_shutdown(void)
{
   FREED(_edje_edd_edje_string);
   FREED(_edje_edd_edje_style);
   FREED(_edje_edd_edje_style_tag);
   FREED(_edje_edd_edje_color_class);
   FREED(_edje_edd_edje_text_class);
   FREED(_edje_edd_edje_size_class);
   FREED(_edje_edd_edje_external_directory);
   FREED(_edje_edd_edje_external_directory_entry);
   FREED(_edje_edd_edje_font_directory_entry);
   FREED(_edje_edd_edje_image_id_hash);
   FREED(_edje_edd_edje_image_directory);
   FREED(_edje_edd_edje_image_directory_entry);
   FREED(_edje_edd_edje_image_directory_set);
   FREED(_edje_edd_edje_image_directory_set_entry);
   FREED(_edje_edd_edje_vector_directory_entry);
   FREED(_edje_edd_edje_model_directory);
   FREED(_edje_edd_edje_model_directory_entry);
   FREED(_edje_edd_edje_limit);
   FREED(_edje_edd_edje_limit_pointer);
   FREED(_edje_edd_edje_sound_sample);
   FREED(_edje_edd_edje_translation_file);
   FREED(_edje_edd_edje_sound_tone);
   FREED(_edje_edd_edje_sound_directory);
   FREED(_edje_edd_edje_mo_directory);
   FREED(_edje_edd_edje_vibration_sample);
   FREED(_edje_edd_edje_vibration_directory);
   FREED(_edje_edd_edje_filter);
   FREED(_edje_edd_edje_filter_directory);
   FREED(_edje_edd_edje_program);
   FREED(_edje_edd_edje_program_pointer);
   FREED(_edje_edd_edje_program_target);
   FREED(_edje_edd_edje_program_after);
   FREED(_edje_edd_edje_part_collection_directory_entry);
   FREED(_edje_edd_edje_pack_element);
   FREED(_edje_edd_edje_pack_element_pointer);
   FREED(_edje_edd_edje_part);
   FREED(_edje_edd_edje_part_pointer);
   FREED(_edje_edd_edje_part_allowed_seat);
   FREED(_edje_edd_edje_part_allowed_seat_pointer);
   FREED(_edje_edd_edje_part_description_variant);
   FREED(_edje_edd_edje_part_description_rectangle);
   FREED(_edje_edd_edje_part_description_snapshot);
   FREED(_edje_edd_edje_part_description_spacer);
   FREED(_edje_edd_edje_part_description_swallow);
   FREED(_edje_edd_edje_part_description_group);
   FREED(_edje_edd_edje_part_description_image);
   FREED(_edje_edd_edje_part_description_proxy);
   FREED(_edje_edd_edje_part_description_text);
   FREED(_edje_edd_edje_part_description_textblock);
   FREED(_edje_edd_edje_part_description_box);
   FREED(_edje_edd_edje_part_description_table);
   FREED(_edje_edd_edje_part_description_external);
   FREED(_edje_edd_edje_part_description_vector);
   FREED(_edje_edd_edje_part_description_vector_pointer);
   FREED(_edje_edd_edje_part_description_variant_list);
   FREED(_edje_edd_edje_part_description_rectangle_pointer);
   FREED(_edje_edd_edje_part_description_snapshot_pointer);
   FREED(_edje_edd_edje_part_description_spacer_pointer);
   FREED(_edje_edd_edje_part_description_swallow_pointer);
   FREED(_edje_edd_edje_part_description_group_pointer);
   FREED(_edje_edd_edje_part_description_image_pointer);
   FREED(_edje_edd_edje_part_description_proxy_pointer);
   FREED(_edje_edd_edje_part_description_text_pointer);
   FREED(_edje_edd_edje_part_description_textblock_pointer);
   FREED(_edje_edd_edje_part_description_box_pointer);
   FREED(_edje_edd_edje_part_description_table_pointer);
   FREED(_edje_edd_edje_part_description_external_pointer);
   FREED(_edje_edd_edje_part_description_filter_data);
   FREED(_edje_edd_edje_part_image_id);
   FREED(_edje_edd_edje_part_image_id_pointer);
   FREED(_edje_edd_edje_external_param);
   FREED(_edje_edd_edje_part_limit);
   FREED(_edje_edd_edje_physics_face);
   FREED(_edje_edd_edje_map_colors);
   FREED(_edje_edd_edje_map_colors_pointer);
   FREED(_edje_edd_edje_color_tree_node);

   FREED(_edje_edd_edje_file);
   FREED(_edje_edd_edje_part_collection);
   FREED(_edje_edd_edje_color_class_info);
}

/**
 * @brief Macro to define an Eet_Data_Descriptor for a pointer to a given Edje type.
 * This creates a helper struct `_Edje_Type_Pointer` containing a single member `pointer`
 * of type `Edje_Type *`. It then creates an Eet_Data_Descriptor for this helper struct,
 * effectively describing a pointer to `Edje_Type`.
 *
 * @param Type The base Edje type (e.g., `Program`, `Part`).
 * @param Name The name suffix for the Eet_Data_Descriptor variable (e.g., `program` for `_edje_edd_edje_program_pointer`).
 *             The corresponding descriptor for `Edje_Type` itself is assumed to be `_edje_edd_edje_Name`.
 *
 * Example: `EDJE_DEFINE_POINTER_TYPE(Program, program)` will define `_edje_edd_edje_program_pointer`
 *          which describes a structure holding an `Edje_Program *`. This pointer descriptor will
 *          refer to `_edje_edd_edje_program` for the actual `Edje_Program` structure definition.
 */
#define EDJE_DEFINE_POINTER_TYPE(Type, Name)                                                                                         \
  {                                                                                                                                  \
     typedef struct _Edje_##Type##_Pointer Edje_##Type## _Pointer;                                                                   \
     struct _Edje_##Type##_Pointer                                                                                                   \
     {                                                                                                                               \
        Edje_##Type * pointer;                                                                                                       \
     };                                                                                                                              \
                                                                                                                                     \
     EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_##Type##_Pointer);                                                          \
     _edje_edd_edje_##Name##_pointer =                                                                                               \
       eet_data_descriptor_file_new(&eddc);                                                                                          \
     EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_##Name##_pointer, Edje_##Type##_Pointer, "pointer", pointer, _edje_edd_edje_##Name); \
  }

/**
 * @brief Initializes all Eet_Data_Descriptors for Edje structures.
 * This function is responsible for creating and configuring all the necessary
 * Eet_Data_Descriptor instances that define how Edje data is serialized and
 * deserialized. It sets up descriptors for basic types, complex structures,
 * lists, hashes, and variants.
 * FIXME: remove EAPI when edje_convert goes, should be static.
 */
// FIXME: remove EAPI when edje_convert goes
EAPI void
_edje_edd_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   /* Edje_String: Represents a string that can be localized, associated with an ID. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_String);
   _edje_edd_edje_string = eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_string, Edje_String, "str", str, EET_T_STRING); // The actual string content.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_string, Edje_String, "id", id, EET_T_UINT);     // Numeric ID for the string.

   /* Edje_External_Directory_Entry: An entry in the external file directory. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_External_Directory_Entry);
   _edje_edd_edje_external_directory_entry =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_external_directory_entry, Edje_External_Directory_Entry, "entry", entry, EET_T_STRING); // Name of the external entry.

   /* Edje_External_Directory: A list of external file entries.
    * Example `entries` array structure:
    * [
    *   { "entry": "external_program_name" },
    *   { "entry": "another_external_resource" }
    * ]
    */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_External_Directory);
   _edje_edd_edje_external_directory =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_external_directory, Edje_External_Directory, "entries", entries, _edje_edd_edje_external_directory_entry); // Variable array of Edje_External_Directory_Entry.

   /* Edje_Font_Directory_Entry: An entry in the font directory, mapping a font name to a file. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Font_Directory_Entry);
   _edje_edd_edje_font_directory_entry =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_font_directory_entry, Edje_Font_Directory_Entry, "entry", entry, EET_T_STRING); // Font name used in EDC.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_font_directory_entry, Edje_Font_Directory_Entry, "file", file, EET_T_STRING);   // Actual font file path.

   /* Edje_Image_Hash: A simple structure to hold an image ID, used in a hash table. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Image_Hash);
   _edje_edd_edje_image_id_hash = eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_id_hash, Edje_Image_Hash, "id", id, EET_T_INT); // Image ID.

   /* Edje_Image_Directory_Entry: An entry in the image directory. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Image_Directory_Entry);
   _edje_edd_edje_image_directory_entry =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "entry", entry, EET_T_STRING);             // Image file path or identifier.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "source_type", source_type, EET_T_INT);   // Type of image source (e.g., bitmap, svg).
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "source_param", source_param, EET_T_INT); // Parameter for the source type (e.g., compression level).
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "id", id, EET_T_INT);                       // Unique ID for this image entry.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "external_id", external_id, EET_T_STRING); // ID for externally stored images.

   /* Edje_Image_Directory_Set_Entry: An entry within an image set, defining properties for a specific image in the set. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Image_Directory_Set_Entry);
   _edje_edd_edje_image_directory_set_entry =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "name", name, EET_T_STRING);             // Name of the image within the set.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "id", id, EET_T_INT);                   // ID of this specific image in the set.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "w", size.w, EET_T_INT);                 // Nominal width.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "h", size.h, EET_T_INT);                 // Nominal height.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "min.w", size.min.w, EET_T_INT);         // Minimum width for scaling.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "min.h", size.min.h, EET_T_INT);         // Minimum height for scaling.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "max.w", size.max.w, EET_T_INT);         // Maximum width for scaling.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "max.h", size.max.h, EET_T_INT);         // Maximum height for scaling.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "border.l", border.l, EET_T_INT);         // Left border size.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "border.r", border.r, EET_T_INT);         // Right border size.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "border.t", border.t, EET_T_INT);         // Top border size.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "border.b", border.b, EET_T_INT);         // Bottom border size.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "border.scale_by", border.scale_by, EDJE_T_FLOAT); // Border scale factor.

   /* Edje_Image_Directory_Set: A set of related images, often different sizes or states of the same conceptual image.
    * Example `entries` list structure:
    * [
    *   { "name": "button_normal.png", "id": 0, "w": 100, "h": 30, ... },
    *   { "name": "button_pressed.png", "id": 1, "w": 100, "h": 30, ... }
    * ]
    */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Image_Directory_Set);
   _edje_edd_edje_image_directory_set =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set, Edje_Image_Directory_Set, "name", name, EET_T_STRING); // Name of the image set.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set, Edje_Image_Directory_Set, "id", id, EET_T_INT);         // Unique ID for this image set.
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_image_directory_set, Edje_Image_Directory_Set, "entries", entries, _edje_edd_edje_image_directory_set_entry); // List of Edje_Image_Directory_Set_Entry.

   /* Edje_Vector_Directory_Entry: An entry for vector graphics (e.g., SVG) in the image directory. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Vector_Directory_Entry);
   _edje_edd_edje_vector_directory_entry =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_vector_directory_entry, Edje_Vector_Directory_Entry, "entry", entry, EET_T_STRING); // Vector file path or identifier.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_vector_directory_entry, Edje_Vector_Directory_Entry, "id", id, EET_T_INT);             // Unique ID for this vector entry.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_vector_directory_entry, Edje_Vector_Directory_Entry, "type", type, EET_T_INT);         // Type of vector (e.g., SVG, Lottie).

   /* Edje_Image_Directory: Contains lists of image entries, image sets, and vector entries.
    * Example `entries` array structure:
    * [ { "entry": "image1.png", "id": 10, ... }, { "entry": "image2.jpg", "id": 11, ... } ]
    * Example `sets` array structure:
    * [ { "name": "button_states", "id": 20, "entries": [...] }, ... ]
    * Example `vectors` array structure:
    * [ { "entry": "icon.svg", "id": 30, "type": 0 }, ... ]
    */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Image_Directory);
   _edje_edd_edje_image_directory =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_image_directory, Edje_Image_Directory, "entries", entries, _edje_edd_edje_image_directory_entry); // Variable array of Edje_Image_Directory_Entry.
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_image_directory, Edje_Image_Directory, "sets", sets, _edje_edd_edje_image_directory_set);       // Variable array of Edje_Image_Directory_Set.
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_image_directory, Edje_Image_Directory, "vectors", vectors, _edje_edd_edje_vector_directory_entry); // Variable array of Edje_Vector_Directory_Entry.

   /* Edje_Color_Class_Info: Information about color classes, primarily a list of color names.
    * Example `colors` list structure:
    * [ "background_color", "text_color", "highlight_color" ]
    */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Color_Class_Info);
   _edje_edd_edje_color_class_info = eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(_edje_edd_edje_color_class_info, Edje_Color_Class_Info, "colors", colors); // List of color class names (strings).

   /* Edje_Mo: Information about a Gettext MO translation file. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Mo);
   _edje_edd_edje_translation_file = eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_translation_file, Edje_Mo, "locale", locale, EET_T_STRING); // Locale string (e.g., "en_US", "fr_FR").
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_translation_file, Edje_Mo, "mo_src", mo_src, EET_T_STRING); // Path or identifier for the MO file.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_translation_file, Edje_Mo, "id", id, EET_T_INT);           // Unique ID for this translation.

   /* Edje_Mo_Directory: A list of MO file entries.
    * Example `mo_entries` array structure:
    * [
    *   { "locale": "en_US", "mo_src": "translations/en_US.mo", "id": 0 },
    *   { "locale": "fr_FR", "mo_src": "translations/fr_FR.mo", "id": 1 }
    * ]
    */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Mo_Directory);
   _edje_edd_edje_mo_directory = eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_mo_directory, Edje_Mo_Directory, "mo_entries", mo_entries, _edje_edd_edje_translation_file); // Variable array of Edje_Mo.

   /* Edje_Model_Directory_Entry: An entry in the 3D model directory. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Model_Directory_Entry);
   _edje_edd_edje_model_directory_entry =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_model_directory_entry, Edje_Model_Directory_Entry, "entry", entry, EET_T_STRING); // Model file path or identifier.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_model_directory_entry, Edje_Model_Directory_Entry, "id", id, EET_T_INT);             // Unique ID for this model entry.

   /* Edje_Model_Directory: A list of 3D model entries.
    * Example `entries` array structure:
    * [
    *   { "entry": "models/object1.obj", "id": 0 },
    *   { "entry": "models/character.glb", "id": 1 }
    * ]
    */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Model_Directory);
   _edje_edd_edje_model_directory =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_model_directory, Edje_Model_Directory, "entries", entries, _edje_edd_edje_model_directory_entry); // Variable array of Edje_Model_Directory_Entry.

   /* Edje_Sound_Sample: Describes a sound sample. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Sound_Sample);
   _edje_edd_edje_sound_sample =
     eet_data_descriptor_file_new(&eddc);
   _edje_edd_edje_sound_sample =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_sound_sample, Edje_Sound_Sample, "name", name, EET_T_STRING);             // Name of the sound sample.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_sound_sample, Edje_Sound_Sample, "snd_src", snd_src, EET_T_STRING);       // Source path/identifier of the sound file.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_sound_sample, Edje_Sound_Sample, "compression", compression, EET_T_INT); // Compression type/level.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_sound_sample, Edje_Sound_Sample, "mode", mode, EET_T_INT);                 // Playback mode.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_sound_sample, Edje_Sound_Sample, "quality", quality, EET_T_DOUBLE);       // Quality factor.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_sound_sample, Edje_Sound_Sample, "id", id, EET_T_INT);                     // Unique ID for the sound sample.

   /* Edje_Sound_Tone: Describes a sound tone (e.g., a specific frequency or predefined tone). */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Sound_Tone);
   _edje_edd_edje_sound_tone =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_sound_tone, Edje_Sound_Tone, "name", name, EET_T_STRING); // Name of the tone.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_sound_tone, Edje_Sound_Tone, "value", value, EET_T_INT);   // Value associated with the tone (e.g., frequency).
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_sound_tone, Edje_Sound_Tone, "id", id, EET_T_INT);         // Unique ID for the tone.

   /* Edje_Sound_Directory: Contains lists of sound samples and tones.
    * Example `samples` array structure:
    * [ { "name": "click.wav", "id": 0, ... }, { "name": "alert.ogg", "id": 1, ... } ]
    * Example `tones` array structure:
    * [ { "name": "DTMF_1", "value": 697, "id": 100 }, ... ]
    */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Sound_Directory);
   _edje_edd_edje_sound_directory =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_sound_directory, Edje_Sound_Directory, "samples", samples, _edje_edd_edje_sound_sample); // Variable array of Edje_Sound_Sample.
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_sound_directory, Edje_Sound_Directory, "tones", tones, _edje_edd_edje_sound_tone);       // Variable array of Edje_Sound_Tone.

   /* Edje_Vibration_Sample: Describes a vibration pattern. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Vibration_Sample);
   _edje_edd_edje_vibration_sample =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_vibration_sample, Edje_Vibration_Sample, "name", name, EET_T_STRING); // Name of the vibration pattern.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_vibration_sample, Edje_Vibration_Sample, "src", src, EET_T_STRING);     // Source or definition of the vibration.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_vibration_sample, Edje_Vibration_Sample, "id", id, EET_T_INT);         // Unique ID for the vibration sample.

   /* Edje_Vibration_Directory: Contains a list of vibration samples.
    * Example `samples` array structure:
    * [ { "name": "short_buzz", "id": 0, ... }, { "name": "long_pulse", "id": 1, ... } ]
    */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Vibration_Directory);
   _edje_edd_edje_vibration_directory =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_vibration_directory, Edje_Vibration_Directory, "samples", samples, _edje_edd_edje_vibration_sample); // Variable array of Edje_Vibration_Sample.

   /* Edje_Gfx_Filter: Describes a graphics filter program. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Gfx_Filter);
   _edje_edd_edje_filter = eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_filter, Edje_Gfx_Filter, "name", name, EET_T_STRING);     // Name of the filter.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_filter, Edje_Gfx_Filter, "script", script, EET_T_STRING); // Lua script code for the filter.

   /* Edje_Gfx_Filter_Directory: Contains a list of graphics filter programs.
    * Example `filters` array structure:
    * [ { "name": "blur_filter", "script": "..." }, { "name": "sharpen_filter", "script": "..." } ]
    */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Gfx_Filter_Directory);
   _edje_edd_edje_filter_directory = eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_filter_directory, Edje_Gfx_Filter_Directory, "filters", filters, _edje_edd_edje_filter); // Variable array of Edje_Gfx_Filter.

   /* Edje_Part_Collection_Directory_Entry: An entry in the directory of part collections (groups). */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Collection_Directory_Entry);
   _edje_edd_edje_part_collection_directory_entry =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "entry", entry, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.RECTANGLE", count.RECTANGLE, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.TEXT", count.TEXT, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.IMAGE", count.IMAGE, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.PROXY", count.PROXY, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.SWALLOW", count.SWALLOW, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.TEXTBLOCK", count.TEXTBLOCK, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.GROUP", count.GROUP, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.BOX", count.BOX, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.TABLE", count.TABLE, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.EXTERNAL", count.EXTERNAL, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.SPACER", count.SPACER, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.SNAPSHOT", count.SNAPSHOT, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.VECTOR", count.VECTOR, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.part", count.part, EET_T_INT); // Total count of parts in the collection.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "group_alias", group_alias, EET_T_UCHAR); // Flag indicating if this collection is an alias.

   /* Edje_Style_Tag: A key-value pair defining a style property. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Style_Tag);
   _edje_edd_edje_style_tag =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_style_tag, Edje_Style_Tag, "key", key, EET_T_STRING);     // Style property key (e.g., "font-size").
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_style_tag, Edje_Style_Tag, "value", value, EET_T_STRING); // Style property value (e.g., "12").

   /* Edje_Part_Allowed_Seat: Defines a seat name that a part is allowed to interact with. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Allowed_Seat);
   _edje_edd_edje_part_allowed_seat =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_allowed_seat, Edje_Part_Allowed_Seat, "name", name, EET_T_STRING); // Name of the allowed seat.

   /* Edje_Style: A named collection of style tags.
    * Example `tags` list structure:
    * [
    *   { "key": "font", "value": "Sans" },
    *   { "key": "font_size", "value": "10" }
    * ]
    */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Style);
   _edje_edd_edje_style =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_style, Edje_Style, "name", name, EET_T_STRING); // Name of the style.
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_style, Edje_Style, "tags", tags, _edje_edd_edje_style_tag); // List of Edje_Style_Tag.

   /* Edje_Color_Tree_Node: A node in the color class hierarchy, can contain child color classes.
    * Example `color_classes` list structure:
    * [ "base_color", "accent_color" ]
    */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Color_Tree_Node);
   _edje_edd_edje_color_tree_node =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_tree_node, Edje_Color_Tree_Node, "name", name, EET_T_STRING); // Name of this color tree node.
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(_edje_edd_edje_color_tree_node, Edje_Color_Tree_Node, "color_classes", color_classes); // List of child color class names (strings).

   /* Edje_Color_Class: Defines a named color with primary and optional secondary/tertiary colors. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Color_Class);
   _edje_edd_edje_color_class =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "r", r, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "g", g, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "b", b, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "a", a, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "r2", r2, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "g2", g2, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "b2", b2, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "a2", a2, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "r3", r3, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "g3", g3, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "b3", b3, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "a3", a3, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "desc", desc, EET_T_STRING);     // Description of the color class.

   /* Edje_Text_Class: Defines a named text style with font and size. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Text_Class);
   _edje_edd_edje_text_class =
      eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_text_class, Edje_Text_Class, "name", name, EET_T_STRING); // Name of the text class.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_text_class, Edje_Text_Class, "font", font, EET_T_STRING); // Font name (e.g., "Sans", "Serif").
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_text_class, Edje_Text_Class, "size", size, EET_T_INT);    // Font size in points.

   /* Edje_Size_Class: Defines a named size constraint with min/max width and height. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Size_Class);
   _edje_edd_edje_size_class =
      eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_size_class, Edje_Size_Class, "name", name, EET_T_STRING); // Name of the size class.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_size_class, Edje_Size_Class, "minw", minw, EET_T_INT);    // Minimum width.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_size_class, Edje_Size_Class, "minh", minh, EET_T_INT);    // Minimum height.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_size_class, Edje_Size_Class, "maxw", maxw, EET_T_INT);    // Maximum width.
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_size_class, Edje_Size_Class, "maxh", maxh, EET_T_INT);    // Maximum height.

   /* Edje_Part_Description_Spec_Filter_Data: Key-value pair for filter data. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Spec_Filter_Data);
   _edje_edd_edje_part_description_filter_data = eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_filter_data, Edje_Part_Description_Spec_Filter_Data, "name", name, EET_T_STRING);   // Data item name (key).
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_filter_data, Edje_Part_Description_Spec_Filter_Data, "value", value, EET_T_STRING); // Data item value.

   /* Edje_File: The root structure of an Edje file, containing all other directories and global properties. */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_File);
   eddc.func.hash_add = (void * (*)(void *, const char *, void *))_edje_eina_hash_add_alloc;
   _edje_edd_edje_file = eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_file, Edje_File, "compiler", compiler, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_file, Edje_File, "version", version, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_file, Edje_File, "minor", minor, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_file, Edje_File, "feature_ver", feature_ver, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_file, Edje_File, "efl_version.major", efl_version.major, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_file, Edje_File, "efl_version.minor", efl_version.minor, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_file, Edje_File, "base_scale", base_scale, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_file, Edje_File, "id", id, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY_STRING(_edje_edd_edje_file, Edje_File, "requires", requires);
   EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_file, Edje_File, "external_dir", external_dir, _edje_edd_edje_external_directory);
   EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_file, Edje_File, "image_dir", image_dir, _edje_edd_edje_image_directory);
   EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_file, Edje_File, "model_dir", model_dir, _edje_edd_edje_model_directory);
   EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_file, Edje_File, "sound_dir", sound_dir, _edje_edd_edje_sound_directory);
   EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_file, Edje_File, "mo_dir", mo_dir, _edje_edd_edje_mo_directory);
   EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_file, Edje_File, "filter_dir", filter_dir, _edje_edd_edje_filter_directory);

   EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_file, Edje_File, "vibration_dir", vibration_dir, _edje_edd_edje_vibration_directory);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_file, Edje_File, "styles", styles, _edje_edd_edje_style);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_file, Edje_File, "color_tree", color_tree, _edje_edd_edje_color_tree_node);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_file, Edje_File, "color_classes", color_classes, _edje_edd_edje_color_class);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_file, Edje_File, "text_classes", text_classes, _edje_edd_edje_text_class);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_file, Edje_File, "size_classes", size_classes, _edje_edd_edje_size_class);
   EET_DATA_DESCRIPTOR_ADD_HASH(_edje_edd_edje_file, Edje_File, "data", data, _edje_edd_edje_string);
   EET_DATA_DESCRIPTOR_ADD_HASH(_edje_edd_edje_file, Edje_File, "fonts", fonts, _edje_edd_edje_font_directory_entry);
   EET_DATA_DESCRIPTOR_ADD_HASH(_edje_edd_edje_file, Edje_File, "collection", collection, _edje_edd_edje_part_collection_directory_entry);
   EET_DATA_DESCRIPTOR_ADD_HASH(_edje_edd_edje_file, Edje_File, "image_id_hash", image_id_hash, _edje_edd_edje_image_id_hash);

   /* parts & limit & programs - loaded induvidually */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Limit);
   _edje_edd_edje_limit = eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_limit, Edje_Limit, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_limit, Edje_Limit, "value", value, EET_T_INT);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Program_Target);
   _edje_edd_edje_program_target =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program_target, Edje_Program_Target, "id", id, EET_T_INT);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Program_After);
   _edje_edd_edje_program_after =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program_after,
                                 Edje_Program_After, "id", id, EET_T_INT);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Program);
   _edje_edd_edje_program =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "signal", signal, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "source", source, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "filter_part", filter.part, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "filter_state", filter.state, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "in.from", in.from, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "in.range", in.range, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "action", action, EET_T_INT);

   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "sample_name", sample_name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "tone_name", tone_name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "duration", duration, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "speed", speed, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "vibration_name", vibration_name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "vibration_repeat", vibration_repeat, EET_T_INT);

   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "seat", seat, EET_T_STRING);

   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "state", state, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "state2", state2, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "value", value, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "value2", value2, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "tween.mode", tween.mode, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "tween.time", tween.time, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "v1", tween.v1, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "v2", tween.v2, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "v3", tween.v3, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "v4", tween.v4, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "use_duration_factor", tween.use_duration_factor, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_program, Edje_Program, "targets", targets, _edje_edd_edje_program_target);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_program, Edje_Program, "after", after, _edje_edd_edje_program_after);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "api.name", api.name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "api.description", api.description, EET_T_STRING);

   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "param.src", param.src, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "param.dst", param.dst, EET_T_INT);

#ifdef HAVE_EPHYSICS
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "physics.w", physics.w, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "physics.x", physics.x, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "physics.y", physics.y, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "physics.z", physics.z, EET_T_DOUBLE);
#endif
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "channel", channel, EET_T_UCHAR);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Image_Id);
   _edje_edd_edje_part_image_id =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_image_id, Edje_Part_Image_Id, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_image_id, Edje_Part_Image_Id, "set", set, EET_T_UCHAR);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_External_Param);
   _edje_edd_edje_external_param =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_external_param, Edje_External_Param, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_external_param, Edje_External_Param, "type", type, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_external_param, Edje_External_Param, "i", i, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_external_param, Edje_External_Param, "d", d, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_external_param, Edje_External_Param, "s", s, EET_T_STRING);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Physics_Face);
   _edje_edd_edje_physics_face = eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_physics_face, Edje_Physics_Face, "type", type, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_physics_face, Edje_Physics_Face, "source", source, EET_T_STRING);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Map_Color);
   _edje_edd_edje_map_colors = eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_map_colors, Edje_Map_Color,
                                 "idx", idx, EET_T_UINT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_map_colors, Edje_Map_Color,
                                 "r", r, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_map_colors, Edje_Map_Color,
                                 "g", g, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_map_colors, Edje_Map_Color,
                                 "b", b, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_map_colors, Edje_Map_Color,
                                 "a", a, EET_T_UCHAR);

   EDJE_DEFINE_POINTER_TYPE(Map_Color, map_colors);

#define EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_FIELDS(Edd, Type)                                              \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "state.name", state.name, EET_T_STRING);                            \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "state.value", state.value, EET_T_DOUBLE);                          \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "visible", visible, EET_T_CHAR);                                    \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "no_render", no_render, EET_T_UCHAR);                               \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "limit", limit, EET_T_CHAR);                                        \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "align.x", align.x, EDJE_T_FLOAT);                                  \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "align.y", align.y, EDJE_T_FLOAT);                                  \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "fixed.w", fixed.w, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "fixed.h", fixed.h, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "user_set.fixed", user_set.fixed, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "minmul.have", minmul.have, EET_T_UCHAR);                           \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "minmul.w", minmul.w, EDJE_T_FLOAT);                                \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "minmul.h", minmul.h, EDJE_T_FLOAT);                                \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "min.w", min.w, EET_T_INT);                                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "min.h", min.h, EET_T_INT);                                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "min.limit", min.limit, EET_T_UCHAR);                               \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "max.w", max.w, EET_T_INT);                                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "max.h", max.h, EET_T_INT);                                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "max.limit", max.limit, EET_T_UCHAR);                               \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "step.x", step.x, EET_T_INT);                                       \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "step.y", step.y, EET_T_INT);                                       \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "aspect.min", aspect.min, EDJE_T_FLOAT);                            \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "aspect.max", aspect.max, EDJE_T_FLOAT);                            \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "aspect.prefer", aspect.prefer, EET_T_CHAR);                        \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.relative_x", rel1.relative_x, EDJE_T_FLOAT);                  \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.relative_y", rel1.relative_y, EDJE_T_FLOAT);                  \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.offset_x", rel1.offset_x, EET_T_INT);                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.offset_y", rel1.offset_y, EET_T_INT);                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.id_x", rel1.id_x, EET_T_INT);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.id_y", rel1.id_y, EET_T_INT);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.relative_x", rel2.relative_x, EDJE_T_FLOAT);                  \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.relative_y", rel2.relative_y, EDJE_T_FLOAT);                  \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.offset_x", rel2.offset_x, EET_T_INT);                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.offset_y", rel2.offset_y, EET_T_INT);                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.id_x", rel2.id_x, EET_T_INT);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.id_y", rel2.id_y, EET_T_INT);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "clip_to_id", clip_to_id, EET_T_INT);                               \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "size_class", size_class, EET_T_STRING);                            \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color_class", color_class, EET_T_STRING);                          \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color.r", color.r, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color.g", color.g, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color.b", color.b, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color.a", color.a, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color2.r", color2.r, EET_T_UCHAR);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color2.g", color2.g, EET_T_UCHAR);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color2.b", color2.b, EET_T_UCHAR);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color2.a", color2.a, EET_T_UCHAR);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.id_persp", map.id_persp, EET_T_INT);                           \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.id_light", map.id_light, EET_T_INT);                           \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.rot.id_center", map.rot.id_center, EET_T_INT);                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.zoom.id_center", map.zoom.id_center, EET_T_INT);               \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.rot.x", map.rot.x, EDJE_T_FLOAT);                              \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.rot.y", map.rot.y, EDJE_T_FLOAT);                              \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.rot.z", map.rot.z, EDJE_T_FLOAT);                              \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.on", map.on, EET_T_UCHAR);                                     \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.smooth", map.smooth, EET_T_UCHAR);                             \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.alpha", map.alpha, EET_T_UCHAR);                               \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.persp_on", map.persp_on, EET_T_UCHAR);                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.backcull", map.backcull, EET_T_UCHAR);                         \
  EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(Edd, Type, "map.color", map.colors, _edje_edd_edje_map_colors_pointer);    \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.zoom.x", map.zoom.x, EDJE_T_FLOAT);                            \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.zoom.y", map.zoom.y, EDJE_T_FLOAT);                            \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "persp.zplane", persp.zplane, EET_T_INT);                           \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "persp.focal", persp.focal, EET_T_INT);                             \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "offset_is_scaled", offset_is_scaled, EET_T_UCHAR);

#ifdef HAVE_EPHYSICS
#define EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON(Edd, Type)                                                          \
  {                                                                                                                 \
     EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_FIELDS(Edd, Type)                                                      \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mass", physics.mass, EET_T_DOUBLE);                          \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.restitution", physics.restitution, EET_T_DOUBLE);            \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.friction", physics.friction, EET_T_DOUBLE);                  \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.damping.linear", physics.damping.linear, EET_T_DOUBLE);      \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.damping.angular", physics.damping.angular, EET_T_DOUBLE);    \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.sleep.linear", physics.sleep.linear, EET_T_DOUBLE);          \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.sleep.angular", physics.sleep.angular, EET_T_DOUBLE);        \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.material", physics.material, EET_T_UCHAR);                   \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.density", physics.density, EET_T_DOUBLE);                    \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.hardness", physics.hardness, EET_T_DOUBLE);                  \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.z", physics.z, EET_T_INT);                                   \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.depth", physics.depth, EET_T_INT);                           \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.ignore_part_pos", physics.ignore_part_pos, EET_T_UCHAR);     \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.light_on", physics.light_on, EET_T_UCHAR);                   \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.backcull", physics.backcull, EET_T_UCHAR);                   \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mov_freedom.lin.x", physics.mov_freedom.lin.x, EET_T_UCHAR); \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mov_freedom.lin.y", physics.mov_freedom.lin.y, EET_T_UCHAR); \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mov_freedom.lin.z", physics.mov_freedom.lin.z, EET_T_UCHAR); \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mov_freedom.ang.x", physics.mov_freedom.ang.x, EET_T_UCHAR); \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mov_freedom.ang.y", physics.mov_freedom.ang.y, EET_T_UCHAR); \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mov_freedom.ang.z", physics.mov_freedom.ang.z, EET_T_UCHAR); \
     EET_DATA_DESCRIPTOR_ADD_LIST(Edd, Type, "physics.faces", physics.faces, _edje_edd_edje_physics_face);          \
  }
#else
#define EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON(Edd, Type)     \
  {                                                            \
     EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_FIELDS(Edd, Type) \
  }
#endif

#define EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB_FIELDS(Edd, Type, Dec)                                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "state.name", Dec.state.name, EET_T_STRING);                            \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "state.value", Dec.state.value, EET_T_DOUBLE);                          \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "visible", Dec.visible, EET_T_CHAR);                                    \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "align.x", Dec.align.x, EDJE_T_FLOAT);                                  \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "align.y", Dec.align.y, EDJE_T_FLOAT);                                  \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "fixed.w", Dec.fixed.w, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "fixed.h", Dec.fixed.h, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "user_set.fixed", Dec.user_set.fixed, EET_T_UCHAR);                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "min.w", Dec.min.w, EET_T_INT);                                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "min.h", Dec.min.h, EET_T_INT);                                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "min.limit", Dec.min.limit, EET_T_CHAR);                                \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "max.w", Dec.max.w, EET_T_INT);                                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "max.h", Dec.max.h, EET_T_INT);                                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "max.limit", Dec.max.limit, EET_T_CHAR);                                \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "step.x", Dec.step.x, EET_T_INT);                                       \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "step.y", Dec.step.y, EET_T_INT);                                       \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "aspect.min", Dec.aspect.min, EDJE_T_FLOAT);                            \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "aspect.max", Dec.aspect.max, EDJE_T_FLOAT);                            \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "aspect.prefer", Dec.aspect.prefer, EET_T_CHAR);                        \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.relative_x", Dec.rel1.relative_x, EDJE_T_FLOAT);                  \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.relative_y", Dec.rel1.relative_y, EDJE_T_FLOAT);                  \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.offset_x", Dec.rel1.offset_x, EET_T_INT);                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.offset_y", Dec.rel1.offset_y, EET_T_INT);                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.id_x", Dec.rel1.id_x, EET_T_INT);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.id_y", Dec.rel1.id_y, EET_T_INT);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.relative_x", Dec.rel2.relative_x, EDJE_T_FLOAT);                  \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.relative_y", Dec.rel2.relative_y, EDJE_T_FLOAT);                  \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.offset_x", Dec.rel2.offset_x, EET_T_INT);                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.offset_y", Dec.rel2.offset_y, EET_T_INT);                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.id_x", Dec.rel2.id_x, EET_T_INT);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.id_y", Dec.rel2.id_y, EET_T_INT);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "clip_to_id", Dec.clip_to_id, EET_T_INT);                               \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "no_render", Dec.no_render, EET_T_UCHAR);                               \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "size_class", Dec.size_class, EET_T_STRING);                            \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color_class", Dec.color_class, EET_T_STRING);                          \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color.r", Dec.color.r, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color.g", Dec.color.g, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color.b", Dec.color.b, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color.a", Dec.color.a, EET_T_UCHAR);                                   \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color2.r", Dec.color2.r, EET_T_UCHAR);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color2.g", Dec.color2.g, EET_T_UCHAR);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color2.b", Dec.color2.b, EET_T_UCHAR);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color2.a", Dec.color2.a, EET_T_UCHAR);                                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.id_persp", Dec.map.id_persp, EET_T_INT);                           \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.id_light", Dec.map.id_light, EET_T_INT);                           \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.rot.id_center", Dec.map.rot.id_center, EET_T_INT);                 \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.zoom.id_center", Dec.map.zoom.id_center, EET_T_INT);               \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.rot.x", Dec.map.rot.x, EDJE_T_FLOAT);                              \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.rot.y", Dec.map.rot.y, EDJE_T_FLOAT);                              \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.rot.z", Dec.map.rot.z, EDJE_T_FLOAT);                              \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.on", Dec.map.on, EET_T_UCHAR);                                     \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.smooth", Dec.map.smooth, EET_T_UCHAR);                             \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.alpha", Dec.map.alpha, EET_T_UCHAR);                               \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.persp_on", Dec.map.persp_on, EET_T_UCHAR);                         \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.backcull", Dec.map.backcull, EET_T_UCHAR);                         \
  EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(Edd, Type, "map.color", Dec.map.colors, _edje_edd_edje_map_colors_pointer);    \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.zoom.x", Dec.map.zoom.x, EDJE_T_FLOAT);                            \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.zoom.y", Dec.map.zoom.y, EDJE_T_FLOAT);                            \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "persp.zplane", Dec.persp.zplane, EET_T_INT);                           \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "persp.focal", Dec.persp.focal, EET_T_INT);                             \
  EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "offset_is_scaled", Dec.offset_is_scaled, EET_T_UCHAR);

#ifdef HAVE_EPHYSICS
#define EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB(Edd, Type, Dec)                                                     \
  {                                                                                                                     \
     EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB_FIELDS(Edd, Type, Dec)                                                 \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mass", Dec.physics.mass, EET_T_DOUBLE);                          \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.restitution", Dec.physics.restitution, EET_T_DOUBLE);            \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.friction", Dec.physics.friction, EET_T_DOUBLE);                  \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.damping.linear", Dec.physics.damping.linear, EET_T_DOUBLE);      \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.damping.angular", Dec.physics.damping.angular, EET_T_DOUBLE);    \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.sleep.linear", Dec.physics.sleep.linear, EET_T_DOUBLE);          \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.sleep.angular", Dec.physics.sleep.angular, EET_T_DOUBLE);        \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.material", Dec.physics.material, EET_T_UCHAR);                   \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.density", Dec.physics.density, EET_T_DOUBLE);                    \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.hardness", Dec.physics.hardness, EET_T_DOUBLE);                  \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.z", Dec.physics.z, EET_T_INT);                                   \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.depth", Dec.physics.depth, EET_T_INT);                           \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.ignore_part_pos", Dec.physics.ignore_part_pos, EET_T_UCHAR);     \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.light_on", Dec.physics.light_on, EET_T_UCHAR);                   \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.backcull", Dec.physics.backcull, EET_T_UCHAR);                   \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mov_freedom.lin.x", Dec.physics.mov_freedom.lin.x, EET_T_UCHAR); \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mov_freedom.lin.y", Dec.physics.mov_freedom.lin.y, EET_T_UCHAR); \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mov_freedom.lin.z", Dec.physics.mov_freedom.lin.z, EET_T_UCHAR); \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mov_freedom.ang.x", Dec.physics.mov_freedom.ang.x, EET_T_UCHAR); \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mov_freedom.ang.y", Dec.physics.mov_freedom.ang.y, EET_T_UCHAR); \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "physics.mov_freedom.ang.z", Dec.physics.mov_freedom.ang.z, EET_T_UCHAR); \
     EET_DATA_DESCRIPTOR_ADD_LIST(Edd, Type, "physics.faces", Dec.physics.faces, _edje_edd_edje_physics_face);          \
  }
#else
#define EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB(Edd, Type, Dec)     \
  {                                                                     \
     EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB_FIELDS(Edd, Type, Dec) \
  }
#endif

#define EET_DATA_DESCRIPTOR_ADD_SUB_NESTED_LOOK(Edd, Type, Dec)                                                     \
  {                                                                                                                 \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "Dec##.orientation.look1.x", Dec.orientation.data[0], EDJE_T_FLOAT);  \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "Dec##.orientation.look1.y", Dec.orientation.data[1], EDJE_T_FLOAT);  \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "Dec##.orientation.look1.z", Dec.orientation.data[2], EDJE_T_FLOAT);  \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "Dec##.orientation.look2.x", Dec.orientation.data[3], EDJE_T_FLOAT);  \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "Dec##.orientation.look2.y", Dec.orientation.data[4], EDJE_T_FLOAT);  \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "Dec##.orientation.look2.z", Dec.orientation.data[5], EDJE_T_FLOAT);  \
     EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, #Dec ".orientation.look_to", Dec.orientation.look_to, EET_T_INT);     \
  }

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Common);
   eddc.func.mem_free = mem_free_rectangle;
   eddc.func.mem_alloc = mem_alloc_rectangle;
   _edje_edd_edje_part_description_rectangle =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON(_edje_edd_edje_part_description_rectangle, Edje_Part_Description_Common);

   // SVG, since 1.18
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Vector);
   eddc.func.mem_free = mem_free_vector;
   eddc.func.mem_alloc = mem_alloc_vector;
   _edje_edd_edje_part_description_vector =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB(_edje_edd_edje_part_description_vector, Edje_Part_Description_Vector, common);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_vector, Edje_Part_Description_Vector, "vg.id", vg.id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_vector, Edje_Part_Description_Vector, "vg.set", vg.set, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_vector, Edje_Part_Description_Vector, "vg.type", vg.type, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_vector, Edje_Part_Description_Vector, "vg.frame", vg.frame, EET_T_DOUBLE);


   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Common);
   eddc.func.mem_free = mem_free_spacer;
   eddc.func.mem_alloc = mem_alloc_spacer;
   _edje_edd_edje_part_description_spacer =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON(_edje_edd_edje_part_description_spacer, Edje_Part_Description_Common);

   // SNAPSHOT, since 1.16
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Snapshot);
   eddc.func.mem_free = mem_free_snapshot;
   eddc.func.mem_alloc = mem_alloc_snapshot;
   _edje_edd_edje_part_description_snapshot =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB(_edje_edd_edje_part_description_snapshot, Edje_Part_Description_Snapshot, common);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_snapshot, Edje_Part_Description_Snapshot, "filter.code", filter.code, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(_edje_edd_edje_part_description_snapshot, Edje_Part_Description_Snapshot, "filter.sources", filter.sources);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_description_snapshot, Edje_Part_Description_Snapshot, "filter.data", filter.data, _edje_edd_edje_part_description_filter_data);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Common);
   eddc.func.mem_free = mem_free_swallow;
   eddc.func.mem_alloc = mem_alloc_swallow;
   _edje_edd_edje_part_description_swallow =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON(_edje_edd_edje_part_description_swallow, Edje_Part_Description_Common);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Common);
   eddc.func.mem_free = mem_free_group;
   eddc.func.mem_alloc = mem_alloc_group;
   _edje_edd_edje_part_description_group =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON(_edje_edd_edje_part_description_group, Edje_Part_Description_Common);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Image);
   eddc.func.mem_free = mem_free_image;
   eddc.func.mem_alloc = mem_alloc_image;
   _edje_edd_edje_part_description_image =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, common);

   EDJE_DEFINE_POINTER_TYPE(Part_Image_Id, part_image_id);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.tweens", image.tweens, _edje_edd_edje_part_image_id_pointer);

   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.id", image.id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.set", image.set, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.l", image.border.l, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.r", image.border.r, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.t", image.border.t, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.b", image.border.b, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.no_fill", image.border.no_fill, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.scale", image.border.scale, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.scale_by", image.border.scale_by, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.smooth", image.fill.smooth, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.pos_rel_x", image.fill.pos_rel_x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.pos_abs_x", image.fill.pos_abs_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.rel_x", image.fill.rel_x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.abs_x", image.fill.abs_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.pos_rel_y", image.fill.pos_rel_y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.pos_abs_y", image.fill.pos_abs_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.rel_y", image.fill.rel_y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.abs_y", image.fill.abs_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.type", image.fill.type, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.filter.code", filter.code, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.filter.sources", filter.sources); // @since 1.15
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.filter.data", filter.data, _edje_edd_edje_part_description_filter_data); // @since 1.15

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Proxy);
   eddc.func.mem_free = mem_free_proxy;
   eddc.func.mem_alloc = mem_alloc_proxy;
   _edje_edd_edje_part_description_proxy =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, common);

   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.id", proxy.id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.fill.smooth", proxy.fill.smooth, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.fill.pos_rel_x", proxy.fill.pos_rel_x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.fill.pos_abs_x", proxy.fill.pos_abs_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.fill.rel_x", proxy.fill.rel_x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.fill.abs_x", proxy.fill.abs_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.fill.pos_rel_y", proxy.fill.pos_rel_y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.fill.pos_abs_y", proxy.fill.pos_abs_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.fill.rel_y", proxy.fill.rel_y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.fill.abs_y", proxy.fill.abs_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.fill.type", proxy.fill.type, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.source_visible", proxy.source_visible, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.source_clip", proxy.source_clip, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.filter.code", filter.code, EET_T_STRING); // @since 1.16
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.filter.sources", filter.sources); // @since 1.16
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_description_proxy, Edje_Part_Description_Proxy, "proxy.filter.data", filter.data, _edje_edd_edje_part_description_filter_data); // @since 1.16

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Text);
   eddc.func.mem_free = mem_free_text;
   eddc.func.mem_alloc = mem_alloc_text;
   _edje_edd_edje_part_description_text =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, common);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.color3.r", text.color3.r, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.color3.g", text.color3.g, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.color3.b", text.color3.b, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.color3.a", text.color3.a, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.text", text.text, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.domain", text.domain, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.text_class", text.text_class, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.style", text.style, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.font", text.font, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.size_range_min", text.size_range_min, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.size_range_max", text.size_range_max, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.repch", text.repch, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.size", text.size, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.fit_x", text.fit_x, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.fit_y", text.fit_y, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.min_x", text.min_x, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.min_y", text.min_y, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.max_x", text.max_x, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.max_y", text.max_y, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.align.x", text.align.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.align.y", text.align.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.id_source", text.id_source, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.id_source_part", text.id_source_part, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.id_text_source", text.id_text_source, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.id_text_source_part", text.id_text_source_part, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.ellipsis", text.ellipsis, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.filter", filter.code, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_LIST_STRING(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.filter_sources", filter.sources);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.filter.data", filter.data, _edje_edd_edje_part_description_filter_data); // @since 1.15

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Text);
   eddc.func.mem_free = mem_free_textblock;
   eddc.func.mem_alloc = mem_alloc_textblock;
   _edje_edd_edje_part_description_textblock =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, common);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.color3.r", text.color3.r, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.color3.g", text.color3.g, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.color3.b", text.color3.b, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.color3.a", text.color3.a, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.text", text.text, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.domain", text.domain, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.text_class", text.text_class, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.style", text.style, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.font", text.font, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.repch", text.repch, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.size", text.size, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.size_range_min", text.size_range_min, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.size_range_max", text.size_range_max, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.fit_x", text.fit_x, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.fit_y", text.fit_y, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.fit_step", text.fit_step, EET_T_UINT);
   EET_DATA_DESCRIPTOR_ADD_LIST_UINT(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.fit_size_array", text.fit_size_array);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.min_x", text.min_x, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.min_y", text.min_y, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.max_x", text.max_x, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.max_y", text.max_y, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.align.x", text.align.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.align.y", text.align.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.id_source", text.id_source, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.id_source_part", text.id_source_part, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.id_text_source", text.id_text_source, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.id_text_source_part", text.id_text_source_part, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_textblock, Edje_Part_Description_Text, "text.ellipsis", text.ellipsis, EET_T_DOUBLE);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Box);
   eddc.func.mem_free = mem_free_box;
   eddc.func.mem_alloc = mem_alloc_box;
   _edje_edd_edje_part_description_box =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, common);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.layout", box.layout, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.alt_layout", box.alt_layout, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.align.x", box.align.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.align.y", box.align.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.padding.x", box.padding.x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.padding.y", box.padding.y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.min.h", box.min.h, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.min.v", box.min.v, EET_T_UCHAR);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Table);
   eddc.func.mem_free = mem_free_table;
   eddc.func.mem_alloc = mem_alloc_table;
   _edje_edd_edje_part_description_table =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, common);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, "table.homogeneous", table.homogeneous, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, "table.align.x", table.align.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, "table.align.y", table.align.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, "table.padding.x", table.padding.x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, "table.padding.y", table.padding.y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, "table.min.h", table.min.h, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, "table.min.v", table.min.v, EET_T_UCHAR);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_External);
   eddc.func.mem_free = mem_free_external;
   eddc.func.mem_alloc = mem_alloc_external;
   _edje_edd_edje_part_description_external =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON_SUB(_edje_edd_edje_part_description_external, Edje_Part_Description_External, common);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_part_description_external, Edje_Part_Description_External, "external_params", external_params, _edje_edd_edje_external_param);

   EDJE_DEFINE_POINTER_TYPE(Part_Description_Common, part_description_spacer);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Common, part_description_rectangle);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Common, part_description_swallow);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Common, part_description_group);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Snapshot, part_description_snapshot);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Image, part_description_image);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Proxy, part_description_proxy);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Text, part_description_text);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Text, part_description_textblock);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Box, part_description_box);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Table, part_description_table);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_External, part_description_external);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Vector, part_description_vector);

   eddc.version = EET_DATA_DESCRIPTOR_CLASS_VERSION;
   eddc.func.type_get = _edje_description_variant_type_get;
   eddc.func.type_set = _edje_description_variant_type_set;
   _edje_edd_edje_part_description_variant = eet_data_descriptor_file_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "spacer", _edje_edd_edje_part_description_spacer);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "rectangle", _edje_edd_edje_part_description_rectangle);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "snapshot", _edje_edd_edje_part_description_snapshot);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "swallow", _edje_edd_edje_part_description_swallow);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "group", _edje_edd_edje_part_description_group);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "image", _edje_edd_edje_part_description_image);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "proxy", _edje_edd_edje_part_description_proxy);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "text", _edje_edd_edje_part_description_text);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "textblock", _edje_edd_edje_part_description_textblock);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "box", _edje_edd_edje_part_description_box);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "table", _edje_edd_edje_part_description_table);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "external", _edje_edd_edje_part_description_external);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "vector", _edje_edd_edje_part_description_vector);

#define EDJE_ADD_ARRAY_MAPPING(Variant, Type, Minus)                                     \
  {                                                                                      \
     Edje_Part_Description_List tmp;                                                     \
                                                                                         \
     eet_data_descriptor_element_add(Variant, Type,                                      \
                                     EET_T_UNKNOW, EET_G_VAR_ARRAY,                      \
                                     0, (char *)(&tmp.desc_count) - (char *)(&tmp),      \
                                     NULL,                                               \
                                     _edje_edd_edje_part_description_##Minus##_pointer); \
  }

   _edje_edd_edje_part_description_variant_list = eet_data_descriptor_file_new(&eddc);

   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "rectangle", rectangle);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "snapshot", snapshot);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "spacer", spacer);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "swallow", swallow);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "group", group);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "image", image);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "proxy", proxy);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "text", text);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "textblock", textblock);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "box", box);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "table", table);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "external", external);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "vector", vector);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Pack_Element);
   _edje_edd_edje_pack_element =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "type", type, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "source", source, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "min.w", min.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "min.h", min.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "spread.w", spread.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "spread.h", spread.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "prefer.w", prefer.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "prefer.h", prefer.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "max.w", max.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "max.h", max.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "padding.l", padding.l, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "padding.r", padding.r, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "padding.t", padding.t, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "padding.b", padding.b, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "align.x", align.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "align.y", align.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "weight.x", weight.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "weight.y", weight.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "aspect.w", aspect.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "aspect.h", aspect.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "aspect.mode", aspect.mode, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "options", options, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "col", col, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "row", row, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "colspan", colspan, EET_T_USHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "rowspan", rowspan, EET_T_USHORT);

   EDJE_DEFINE_POINTER_TYPE(Pack_Element, pack_element);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part);
   eddc.func.mem_free = mem_free_part;
   eddc.func.mem_alloc = mem_alloc_part;
   _edje_edd_edje_part =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_VARIANT(_edje_edd_edje_part, Edje_Part, "default_desc", default_desc, type, _edje_edd_edje_part_description_variant);
   EET_DATA_DESCRIPTOR_ADD_VARIANT(_edje_edd_edje_part, Edje_Part, "other", other, type, _edje_edd_edje_part_description_variant_list);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "source", source, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "source2", source2, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "source3", source3, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "source4", source4, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "source5", source5, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "source6", source6, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "clip_to_id", clip_to_id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "no_render", no_render, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.x", dragable.x, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.step_x", dragable.step_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.count_x", dragable.count_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.y", dragable.y, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.step_y", dragable.step_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.count_y", dragable.count_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.counfine_id", dragable.confine_id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.threshold_id", dragable.threshold_id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.events_id", dragable.event_id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part, Edje_Part, "items", items, _edje_edd_edje_pack_element_pointer);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "type", type, EET_T_UCHAR);
#ifdef HAVE_EPHYSICS
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "physics_body", physics_body, EET_T_UCHAR);
#endif
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "effect", effect, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "mouse_events", mouse_events, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "anti_alias", anti_alias, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "repeat_events", repeat_events, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "ignore_flags", ignore_flags, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "mask_flags", mask_flags, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "scale", scale, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "precise_is_inside", precise_is_inside, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "use_alternate_font_metrics", use_alternate_font_metrics, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "pointer_mode", pointer_mode, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "entry_mode", entry_mode, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "select_mode", select_mode, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "cursor_mode", cursor_mode, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "multiline", multiline, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "access", access, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "api.name", api.name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "api.description", api.description, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "nested_children_count", nested_children_count, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "required", required, EET_T_UCHAR);

   EDJE_DEFINE_POINTER_TYPE(Part_Allowed_Seat, part_allowed_seat);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part, Edje_Part, "allowed_seats", allowed_seats, _edje_edd_edje_part_allowed_seat_pointer);


   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Limit);
   _edje_edd_edje_part_limit = eet_data_descriptor_file_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_limit, Edje_Part_Limit, "part", part, EET_T_INT);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Collection);
   _edje_edd_edje_part_collection =
     eet_data_descriptor_file_new(&eddc);

   EDJE_DEFINE_POINTER_TYPE(Program, program);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "programs.fnmatch", programs.fnmatch, _edje_edd_edje_program_pointer);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "programs.strcmp", programs.strcmp, _edje_edd_edje_program_pointer);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "programs.strncmp", programs.strncmp, _edje_edd_edje_program_pointer);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "programs.strrncmp", programs.strrncmp, _edje_edd_edje_program_pointer);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "programs.nocmp", programs.nocmp, _edje_edd_edje_program_pointer);

   EDJE_DEFINE_POINTER_TYPE(Part, part);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "parts", parts, _edje_edd_edje_part_pointer);

   EDJE_DEFINE_POINTER_TYPE(Limit, limit);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "limits.vertical", limits.vertical, _edje_edd_edje_limit_pointer);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "limits.horizontal", limits.horizontal, _edje_edd_edje_limit_pointer);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "limits.parts", limits.parts, _edje_edd_edje_part_limit);

   EET_DATA_DESCRIPTOR_ADD_HASH(_edje_edd_edje_part_collection, Edje_Part_Collection, "data", data, _edje_edd_edje_string);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_HASH_STRING(_edje_edd_edje_part_collection, Edje_Part_Collection, "alias", alias);
   EET_DATA_DESCRIPTOR_ADD_HASH_STRING(_edje_edd_edje_part_collection, Edje_Part_Collection, "aliased", aliased);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "prop.min.w", prop.min.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "prop.min.h", prop.min.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "prop.max.w", prop.max.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "prop.max.h", prop.max.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "part", part, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "lua_script_only", lua_script_only, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "prop.orientation", prop.orientation, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "broadcast_signal", broadcast_signal, EET_T_UCHAR);
#ifdef HAVE_EPHYSICS
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "physics.world.rate", physics.world.rate, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "physics.world.gravity.x", physics.world.gravity.x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "physics.world.gravity.y", physics.world.gravity.y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "physics.world.gravity.z", physics.world.gravity.z, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "physics.world.z", physics.world.z, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "physics.world.depth", physics.world.depth, EET_T_INT);
#endif
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "physics_enabled", physics_enabled, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "script_recursion", script_recursion, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "use_custom_seat_names", use_custom_seat_names, EET_T_UCHAR);
}

EAPI void
_edje_data_font_list_desc_make(Eet_Data_Descriptor **_font_list_edd,
                               Eet_Data_Descriptor **_font_edd)
{  /* User have to free: _font_list_edd, _font_edd */
  Eet_Data_Descriptor_Class eddc;

  eet_eina_stream_data_descriptor_class_set(&eddc, sizeof (eddc),
                                            "font", sizeof (Edje_Font));
  *_font_edd = eet_data_descriptor_stream_new(&eddc);
  EET_DATA_DESCRIPTOR_ADD_BASIC(*_font_edd, Edje_Font,
                                "file", file, EET_T_INLINED_STRING);
  EET_DATA_DESCRIPTOR_ADD_BASIC(*_font_edd, Edje_Font,
                                "name", name, EET_T_INLINED_STRING);

  eet_eina_stream_data_descriptor_class_set(&eddc, sizeof (eddc),
                                            "font_list", sizeof (Edje_Font_List));
  *_font_list_edd = eet_data_descriptor_stream_new(&eddc);
  EET_DATA_DESCRIPTOR_ADD_LIST(*_font_list_edd, Edje_Font_List,
                               "list", list, *_font_edd);
}

