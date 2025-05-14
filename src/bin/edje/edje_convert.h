#ifndef EDJE_CONVERT_H__
# define EDJE_CONVERT_H__

/**
 * @file
 * @brief Type definitions and function prototypes for converting older Edje file
 * formats to the current format.
 *
 * This header defines structures representing older versions of Edje file
 * components. These "Old_" prefixed structures are used as an intermediate
 * representation when loading and converting Edje files from previous format
 * versions.
 */

/** @brief Represents an older version of an Edje file. */
typedef struct _Old_Edje_File			Old_Edje_File;
/** @brief Represents an older version of an Edje image directory. */
typedef struct _Old_Edje_Image_Directory	Old_Edje_Image_Directory;
/** @brief Represents an older version of an Edje font directory. */
typedef struct _Old_Edje_Font_Directory         Old_Edje_Font_Directory;
/** @brief Represents an older version of an Edje external resource directory. */
typedef struct _Old_Edje_External_Directory	Old_Edje_External_Directory;
/** @brief Represents an older version of an Edje part. */
typedef struct _Old_Edje_Part			Old_Edje_Part;
/** @brief Represents an older version of an Edje part collection. */
typedef struct _Old_Edje_Part_Collection	Old_Edje_Part_Collection;
/** @brief Represents an older version of an Edje part collection directory. */
typedef struct _Old_Edje_Part_Collection_Directory Old_Edje_Part_Collection_Directory;
/** @brief Represents an older version of an Edje part description. */
typedef struct _Old_Edje_Part_Description       Old_Edje_Part_Description;
/** @brief Represents an older version of the image-specific part description. */
typedef struct _Old_Edje_Part_Description_Spec_Image Old_Edje_Part_Description_Spec_Image;
/** @brief Represents an older version of a key-value data entry. */
typedef struct _Old_Edje_Data                   Old_Edje_Data;

/**
 * @brief Structure for old Edje key-value data pairs.
 * Used for generic data storage within old Edje files.
 */
struct _Old_Edje_Data
{
   const char *key;
   char *value; /**< The string value associated with the key. */
};

/*----------*/

/**
 * @brief Structure for the old Edje font directory.
 * Contains a list of font entries.
 */
struct _Old_Edje_Font_Directory
{
   Eina_List *entries; /**< A list of Edje_Font_Directory_Entry structures. */
};

/**
 * @brief Structure for the old Edje image directory.
 * Contains lists of image entries and image sets.
 */
struct _Old_Edje_Image_Directory
{
   Eina_List *entries; /**< A list of Edje_Image_Directory_Entry structures. */
   Eina_List *sets;    /**< A list of Edje_Image_Directory_Set structures. */
};

/**
 * @brief Structure for the old Edje external resource directory.
 * Contains a list of external resource entries.
 */
struct _Old_Edje_External_Directory
{
   Eina_List *entries; /**< A list of Edje_External_Directory_Entry structures. */
};

/**
 * @brief Main structure representing an old Edje file.
 * This structure holds all the top-level components of an Edje file
 * from an older format version.
 */
struct _Old_Edje_File
{
   const char                     *path; /**< Filesystem path to the Edje file. */
   time_t                          mtime;

   Old_Edje_External_Directory    *external_dir;
   Old_Edje_Font_Directory        *font_dir;
   Old_Edje_Image_Directory       *image_dir;
   Old_Edje_Part_Collection_Directory *collection_dir;
   Eina_List                      *data;
   Eina_List                      *styles;
   Eina_List                      *color_classes;
   Eina_List                      *text_classes;
   Eina_List                      *size_classes;

   const char                     *compiler;
   int                             version;     /**< Edje file format version. */
   int                             feature_ver; /**< Edje feature version. */
};

/**
 * @brief Structure representing an old Edje part collection (group).
 * A collection groups parts and programs.
 */
struct _Old_Edje_Part_Collection
{
   Eina_List *programs; /**< A list of Edje_Program structures. */
   Eina_List *parts;    /**< A list of Old_Edje_Part structures. */
   Eina_List *data;

   int        id; /* the collection id */

   Eina_Hash *alias; /* aliasing part*/

   struct {
      Edje_Size min, max;
   } prop;

   int        references;
#ifdef EDJE_PROGRAM_CACHE
   struct {
      Eina_Hash                   *no_matches;
      Eina_Hash                   *matches;
   } prog_cache;
#endif

   Embryo_Program   *script; /* all the embryo script code for this group */
   const char       *part;

   unsigned char    script_only;

   unsigned char    lua_script_only;

   unsigned char    checked : 1; /**< Flag indicating if the collection has been checked (e.g., for errors). */
};

/**
 * @brief Structure representing an old Edje part.
 * Parts are the basic building blocks of an Edje UI.
 */
struct _Old_Edje_Part
{
   const char            *name; /**< The name, if any, of the part. */
   Old_Edje_Part_Description *default_desc; /**< The part descriptor for the default state ("default" 0.0). */
   Eina_List             *other_desc; /**< List of Old_Edje_Part_Description for other states. */
   const char            *source, *source2, *source3, *source4, *source5, *source6;
   int                    id; /* its id number */
   int                    clip_to_id; /* the part id to clip this one to */
   Edje_Part_Dragable     dragable;
   Eina_List             *items; /* packed items for box and table */
   unsigned char          type; /* what type (image, rect, text) */
   unsigned char          effect; /* 0 = plain... */
   unsigned char          mouse_events; /* it will affect/respond to mouse events */
   unsigned char          repeat_events; /* it will repeat events to objects below */
   Evas_Event_Flags       ignore_flags;
   Evas_Event_Flags       mask_flags;
   unsigned char          scale; /* should certain properties scale with edje scale factor? */
   unsigned char          precise_is_inside;
   unsigned char          use_alternate_font_metrics;
   unsigned char          pointer_mode;
   unsigned char          entry_mode;
   unsigned char          select_mode;
   unsigned char          multiline;
   Edje_Part_Api	  api;
   unsigned char          required; /**< Flag indicating if this part is required by the theme. */
};

/**
 * @brief Structure for image-specific properties in an old Edje part description.
 */
struct _Old_Edje_Part_Description_Spec_Image
{
   Eina_List     *tween_list; /**< List of Edje_Part_Image_Id structures for tweening. */
   int            id;         /**< The image ID (index into the image directory) to use. -1 for no image. */
   int            scale_hint; /**< Evas scale hint (Evas_Scale_Hint). */
   Eina_Bool      set; /* if image condition it's content */

   Edje_Part_Description_Spec_Border border;
   Edje_Part_Description_Spec_Fill   fill;   /**< Fill properties for the image. */
};

/**
 * @brief Structure representing an old Edje part description (state).
 * Describes the appearance and behavior of a part in a specific state.
 */
struct _Old_Edje_Part_Description
{
   Edje_Part_Description_Common common; /**< Common properties shared by all description types. */
   Old_Edje_Part_Description_Spec_Image image; /**< Image-specific properties. */
   Edje_Part_Description_Spec_Text text;
   Edje_Part_Description_Spec_Box box;
   Edje_Part_Description_Spec_Table table;

   Eina_List *external_params; /**< List of parameters for external type parts. */
};

/**
 * @brief Structure for the old Edje part collection directory.
 * Contains a list of collection directory entries.
 */
struct _Old_Edje_Part_Collection_Directory
{
   Eina_List *entries; /**< A list of Edje_Part_Collection_Directory_Entry structures. */

   int        references; /**< Reference count (likely unused or for internal purposes). */
};

/**
 * @brief Converts an old Edje file structure to the current format.
 * @param ef Eet_File handle for error reporting.
 * @param oedf Pointer to the Old_Edje_File structure to convert.
 * @return Pointer to a new Edje_File structure, or NULL on failure.
 */
Edje_File *_edje_file_convert(Eet_File *ef, Old_Edje_File *oedf);

/**
 * @brief Converts an old Edje part collection to the current format.
 * @param ef Eet_File handle for error reporting.
 * @param ce Pointer to the directory entry for the collection, updated during conversion.
 * @param oedc Pointer to the Old_Edje_Part_Collection to convert.
 * @return Pointer to a new Edje_Part_Collection, or NULL on failure (though typically aborts).
 */
Edje_Part_Collection *_edje_collection_convert(Eet_File *ef,
					       Edje_Part_Collection_Directory_Entry *ce,
					       Old_Edje_Part_Collection *oedc);
/**
 * @brief Converts an old Edje part description to the current format.
 * @param type The type of the part (e.g., EDJE_PART_TYPE_RECTANGLE).
 * @param ce Pointer to the collection directory entry, used for mempools.
 * @param oed Pointer to the Old_Edje_Part_Description to convert (will be freed).
 * @return Pointer to a new Edje_Part_Description_Common (or typed variant), or NULL on failure.
 */
Edje_Part_Description_Common *_edje_description_convert(int type,
							Edje_Part_Collection_Directory_Entry *ce,
							Old_Edje_Part_Description *oed);
/**
 * @brief Gets the currently set global Edje_File.
 * @return Const pointer to the current Edje_File.
 * @see _edje_file_set
 */
const Edje_File *_edje_file_get(void);

/**
 * @brief Sets the global Edje_File pointer.
 * @param edf Const pointer to the Edje_File to set.
 * @see _edje_file_get
 */
void _edje_file_set(const Edje_File *edf);

#endif
