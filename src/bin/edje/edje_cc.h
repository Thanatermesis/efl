/**
 * @file
 * @brief Main header file for the Edje Compiler (edje_cc).
 *
 * This file defines the primary structures, enums, global variables,
 * and function prototypes used throughout the Edje compilation process.
 * It includes necessary headers and sets up logging macros.
 */
#ifndef EDJE_CC_H
#define EDJE_CC_H

#include <edje_private.h>

/**
 * @brief Global Eina_Prefix object.
 * Used for locating data files (themes, fonts, etc.) relative to the
 * application's installation directory or development environment.
 */
extern Eina_Prefix *pfx;

/*
 * On Windows, if the file is not opened in binary mode,
 * read does not return the correct size, because of
 * CR / LF translation.
 */
#ifndef O_BINARY
# define O_BINARY 0
#endif

/* logging variables */
extern int _edje_cc_log_dom; /**< Log domain for edje_cc. */
#define EDJE_CC_DEFAULT_LOG_COLOR EINA_COLOR_CYAN /**< Default log color for edje_cc messages. */

#ifdef ERR
# undef ERR
#endif
/** @brief Macro for logging error messages. */
#define ERR(...) EINA_LOG_DOM_ERR(_edje_cc_log_dom, __VA_ARGS__)
#ifdef INF
# undef INF
#endif
/** @brief Macro for logging informational messages. */
#define INF(...) EINA_LOG_DOM_INFO(_edje_cc_log_dom, __VA_ARGS__)
#ifdef WRN
# undef WRN
#endif
/** @brief Macro for logging warning messages. */
#define WRN(...) EINA_LOG_DOM_WARN(_edje_cc_log_dom, __VA_ARGS__)
#ifdef CRI
# undef CRI
#endif
/** @brief Macro for logging critical messages. */
#define CRI(...) EINA_LOG_DOM_CRIT(_edje_cc_log_dom, __VA_ARGS__)
#ifdef DBG
# undef DBG
#endif
/** @brief Macro for logging debug messages. */
#define DBG(...) EINA_LOG_DOM_DBG(_edje_cc_log_dom, __VA_ARGS__)

/* types */
typedef struct _New_Object_Handler    New_Object_Handler;       /**< Handler for new top-level objects in EDC. */
typedef struct _New_Statement_Handler New_Statement_Handler;    /**< Handler for statements within objects. */
typedef struct _New_Nested_Handler    New_Nested_Handler;       /**< Handler for nested blocks within statements. */
typedef struct _External_List         External_List;            /**< A list of external files. */
typedef struct _External              External;                 /**< Represents an external file definition. */
typedef struct _Code                  Code;                     /**< Represents a script block (Embryo or Lua). */
typedef struct _Code_Program          Code_Program;             /**< Represents a program within a script block. */
typedef struct _SrcFile               SrcFile;                  /**< Represents a source file included in the EDJ. */
typedef struct _SrcFile_List          SrcFile_List;             /**< A list of source files. */

typedef struct _Edje_Program_Parser                  Edje_Program_Parser;           /**< Parser structure for Edje programs. */
typedef struct _Edje_Pack_Element_Parser             Edje_Pack_Element_Parser;      /**< Parser structure for Edje pack elements. */
typedef struct _Edje_Part_Parser                     Edje_Part_Parser;              /**< Parser structure for Edje parts. */
typedef struct _Edje_Part_Collection_Parser          Edje_Part_Collection_Parser;   /**< Parser structure for Edje part collections (groups). */

/**
 * @brief Defines a handler for processing new top-level objects in an EDC file.
 *
 * These handlers are invoked when the parser encounters a new block
 * like "images", "fonts", "collections", etc.
 */
struct _New_Object_Handler
{
   const char *type;    /**< The keyword that identifies this object type (e.g., "images"). */
   void (*func)(void);  /**< Pointer to the function that handles this object type. */
};

/**
 * @brief Defines a handler for processing statements within an EDC object.
 *
 * These handlers are invoked for properties or statements within a block,
 * such as "image: 'my_image.png' COMP;" within an "images" block.
 */
struct _New_Statement_Handler
{
   const char *type;    /**< The keyword that identifies this statement (e.g., "image"). */
   void (*func)(void);  /**< Pointer to the function that handles this statement. */
};

/**
 * @brief Defines a handler for processing nested blocks within EDC statements.
 *
 * These handlers manage the parsing stack for nested structures like
 * "description { ... }" within a "part { ... }" block.
 */
struct _New_Nested_Handler
{
   const char *type;          /**< The keyword that identifies the start of this nested block (e.g., "description"). */
   const char *token;         /**< The token associated with this handler (often the same as type). */
   void (*func_push)(void);   /**< Function to call when entering (pushing) this nested block. */
   void (*func_pop)(void);    /**< Function to call when exiting (popping) this nested block. */
};

/**
 * @brief Represents a list of external file declarations.
 *
 * Externals are typically other EDJ files whose resources can be referenced.
 */
struct _External_List
{
   Eina_List *list; /**< An Eina_List of #_External structures. */
};

/**
 * @brief Represents a single external file declaration.
 */
struct _External
{
    char *name; /**< The name or path of the external file. */
};

/**
 * @brief Represents a block of script code (Embryo or Lua) in an EDC file.
 */
struct _Code
{
   int       l1, l2;      /**< Start and end line numbers of the script block in the source EDC. */
   char      *shared;     /**< Name if this script is shared. NULL otherwise. */
   char      *original;   /**< The original, unprocessed script code. */
   Eina_List *programs;   /**< An Eina_List of #_Code_Program structures defined within this script block. */
   Eina_List *vars;       /**< List of variables (not fully utilized in current edje_cc). */
   Eina_List *func;       /**< List of functions (not fully utilized in current edje_cc). */
   Eina_Bool  parsed : 1; /**< Flag indicating if the script has been parsed. */
   Eina_Bool  is_lua : 1; /**< Flag indicating if the script is Lua (true) or Embryo (false). */
};

/**
 * @brief Represents a single program (function) defined within a script block.
 */
struct _Code_Program
{
   int        l1, l2;    /**< Start and end line numbers of the program in the source EDC. */
   int        id;        /**< Numeric ID assigned to this program. */
   char      *script;   /**< The processed script code for this program. */
   char      *original; /**< The original, unprocessed script code for this program. */
};

/**
 * @brief Represents a source file (e.g., EDC, LUA, INC) that is part of the compiled EDJ.
 * This is used if the `-no-save` option is not used, to embed sources.
 */
struct _SrcFile
{
   char *name; /**< The name/path of the source file. */
   char *file; /**< The content of the source file. */
};

/**
 * @brief Represents a list of source files.
 */
struct _SrcFile_List
{
   Eina_List *list; /**< An Eina_List of #_SrcFile structures. */
};

/**
 * @brief Parser-specific extension of the Edje_Program structure.
 *
 * Holds additional information needed during the parsing phase of an Edje program.
 */
struct _Edje_Program_Parser
{
   Edje_Program common;     /**< The common Edje_Program data. */
   Eina_Bool can_override;  /**< Flag indicating if this program can override an existing one (e.g., from an inherited group). */
};

/**
 * @brief Parser-specific extension of the Edje_Pack_Element structure.
 *
 * Holds additional information needed during the parsing phase for elements
 * within a BOX or TABLE part's "items" block.
 */
struct _Edje_Pack_Element_Parser
{
   Edje_Pack_Element common; /**< The common Edje_Pack_Element data. */
   Eina_Bool can_override;   /**< Flag indicating if this pack element can override an existing one. */
};

/**
 * @brief Parser-specific extension of the Edje_Part structure.
 *
 * Holds additional information needed during the parsing and reordering phase of an Edje part.
 */
struct _Edje_Part_Parser
{
   Edje_Part common; /**< The common Edje_Part data. */
   /**
    * @brief Structure holding information for reordering parts.
    * Parts can be inserted before or after other existing parts.
    */
   struct {
      Eina_Bool           done;           /**< Flag indicating if reordering for this part is complete. */
      const char         *insert_before;  /**< The name of the part before which this part should be inserted. */
      const char         *insert_after;   /**< The name of the part after which this part should be inserted. */
      Edje_Part_Parser   *before;         /**< Pointer to the parser structure of the part identified by insert_before. */
      Edje_Part_Parser   *after;          /**< Pointer to the parser structure of the part identified by insert_after. */
      int                 linked_prev;    /**< Internal linkage count for previous part during reordering. */
      int                 linked_next;    /**< Internal linkage count for next part during reordering. */
   } reorder;
   Eina_Bool can_override; /**< Flag indicating if this part can override an existing one (e.g., from an inherited group). */
};

/**
 * @brief Represents a group of target parts for program actions.
 *
 * Allows a program's action to affect multiple parts simultaneously.
 * Example: `action: STATE_SET "new_state" TARGET "part1" TARGET "part2";`
 * Here, "part1" and "part2" would be in the `targets` list.
 */
typedef struct Edje_Target_Group
{
   char *name;      /**< The name of this target group (not typically used directly by edje_cc). */
   char **targets;  /**< Null-terminated array of strings, where each string is a target part name. */
} Edje_Target_Group;

/**
 * @brief Represents a link between a program and a part description.
 *
 * This structure is used internally during parsing to associate programs
 * (like `after: "event_name" "source_name";`) with the specific part
 * description state they belong to.
 */
typedef struct Edje_Part_Description_Link
{
   Edje_Program *pr;                   /**< Pointer to the Edje_Program being linked. */
   Edje_Part_Description_Common *ed;   /**< Pointer to the common part description data. */
   Edje_Part_Parser *epp;              /**< Pointer to the part parser structure this link belongs to. */
} Edje_Part_Description_Link;

/**
 * @brief Parser-specific extension of the Edje_Part_Collection (group) structure.
 *
 * Holds additional information needed during the parsing phase of an Edje group.
 */
struct _Edje_Part_Collection_Parser
{
   Edje_Part_Collection common;             /**< The common Edje_Part_Collection data. */
   char *default_source;                    /**< Default 'source' for programs in this group if not specified. */
   Eina_List *target_groups;                /**< An Eina_List of #Edje_Target_Group structures. */
   Eina_List *links;                        /**< An Eina_List of #Edje_Part_Description_Link structures, used for resolving program targets. */
   Eina_Hash *link_hash;                    /**< Hash table for quick lookup of links. */
   Eina_List *base_codes;                   /**< List of #Code script blocks defined directly within this group. */
   Eina_Bool default_mouse_events;          /**< Default value for 'mouse_events' property of parts in this group. */
   Eina_Bool inherit_only;                  /**< If true, this group is only for inheritance and won't be directly usable. */
   Eina_Bool inherit_script : 1;            /**< If true, scripts from inherited groups are also inherited. */
   Eina_Bool skip_namespace_validation : 1; /**< If true, namespace validation for parts/programs is skipped for this group. */
};

/**
 * @brief Enum defining types of line-based anchors for part positioning.
 * Used in `rel1.to_x`, `rel1.to_y`, `rel2.to_x`, `rel2.to_y` properties.
 */
typedef enum
{
   EDJE_PART_ANCHOR_LINE_RELATIVE = -1,      /**< Relative to the opposite edge of the same part (internal use). */
   EDJE_PART_ANCHOR_LINE_NONE,               /**< No specific line anchor. */
   EDJE_PART_ANCHOR_LINE_TOP,                /**< Anchor to the top line of the target. */
   EDJE_PART_ANCHOR_LINE_BOTTOM,             /**< Anchor to the bottom line of the target. */
   EDJE_PART_ANCHOR_LINE_LEFT,               /**< Anchor to the left line of the target. */
   EDJE_PART_ANCHOR_LINE_RIGHT,              /**< Anchor to the right line of the target. */
   EDJE_PART_ANCHOR_LINE_VERTICAL_CENTER,    /**< Anchor to the vertical center line of the target. */
   EDJE_PART_ANCHOR_LINE_HORIZONTAL_CENTER   /**< Anchor to the horizontal center line of the target. */
} Edje_Part_Anchor_Line;

/**
 * @brief Enum defining types of fill-based anchors for part sizing.
 * Used in `fill.smooth`, `fill.style` (though style is more complex).
 * This seems to be a simplified internal representation.
 */
typedef enum
{
   EDJE_PART_ANCHOR_FILL_BOTH,        /**< Fill in both horizontal and vertical directions. */
   EDJE_PART_ANCHOR_FILL_HORIZONTAL,  /**< Fill horizontally. */
   EDJE_PART_ANCHOR_FILL_VERTICAL     /**< Fill vertically. */
} Edje_Part_Anchor_Fill;

/**
 * @brief Represents a single anchor point setting (e.g., top, left, fill).
 * This structure is likely used internally by the parser to temporarily store
 * anchor properties before they are fully processed into an Edje_Part_Description.
 */
typedef struct
{
   union {
      Edje_Part_Anchor_Line line; /**< Line-based anchor type, if applicable. */
      Edje_Part_Anchor_Fill fill; /**< Fill-based anchor type, if applicable. */
   } base;
   Eina_Bool set : 1; /**< True if this anchor property has been set in the EDC. */
} Edje_Part_Anchor;

/**
 * @brief Collection of all possible anchor settings for a part description.
 * This structure is likely used internally by the parser to temporarily store
 * all anchor-related properties of a part description state.
 */
typedef struct
{
   Edje_Part_Anchor top;                 /**< Top anchor setting. */
   Edje_Part_Anchor bottom;              /**< Bottom anchor setting. */
   Edje_Part_Anchor left;                /**< Left anchor setting. */
   Edje_Part_Anchor right;               /**< Right anchor setting. */
   Edje_Part_Anchor vertical_center;     /**< Vertical center anchor setting. */
   Edje_Part_Anchor horizontal_center;   /**< Horizontal center anchor setting. */
   Edje_Part_Anchor fill;                /**< Fill anchor setting. */
} Edje_Part_Description_Anchors;

/* global fn calls */

/** @brief Sets up initial data structures for compilation. */
void    data_setup(void);
/** @brief Writes the compiled Edje data to the output file. */
void    data_write(void);
/** @brief Queues a font group (face) name for later lookup and ID assignment. */
void    data_queue_face_group_lookup(const char *name);
/** @brief Queues a collection/group name for later lookup and ID assignment, associated with a part. */
void    data_queue_group_lookup(const char *name, Edje_Part *part);
/** @brief Queues a part name within a collection for later lookup and ID assignment. */
void    data_queue_part_lookup(Edje_Part_Collection *pc, const char *name, int *dest);
/** @brief Queues a nested part name (e.g., "clip_to") for later lookup and ID assignment. */
void    data_queue_part_nest_lookup(Edje_Part_Collection *pc, const char *name, int *dest, char **dest2);
/** @brief Queues a lookup for a nested part that was copied (e.g. during inheritance). */
void    data_queue_copied_part_nest_lookup(Edje_Part_Collection *pc, int *src, int *dest, char **dest2);
/** @brief Queues a lookup for a reallocated part. */
void    data_queue_part_reallocated_lookup(Edje_Part_Collection *pc, const char *name,
					   unsigned char **base, int offset);
/** @brief Deletes a queued part lookup. */
void    part_lookup_del(Edje_Part_Collection *pc, int *dest);
/** @brief Deletes a queued part lookup by name. */
void    part_lookup_delete(Edje_Part_Collection *pc, const char *name, int *dest, char **dest2);
/** @brief Queues a lookup for a part ID that was copied from another part. */
void    data_queue_copied_part_lookup(Edje_Part_Collection *pc, int *src, int *dest);
/** @brief Queues a program name for later lookup and ID assignment. Returns a pointer for later renaming. */
void   *data_queue_program_lookup(Edje_Part_Collection *pc, const char *name, int *dest);
/** @brief Renames a program that was previously queued for lookup. */
void    program_lookup_rename(void *p, const char *name);
/** @brief Deletes a lookup for a copied program by name. */
void    copied_program_lookup_delete(Edje_Part_Collection *pc, const char *name);
/** @brief Queues a lookup for a program ID that was copied from another program. */
Eina_Bool     data_queue_copied_program_lookup(Edje_Part_Collection *pc, int *src, int *dest);
/** @brief Deletes a lookup for a copied anonymous program. */
void    copied_program_anonymous_lookup_delete(Edje_Part_Collection *pc, int *dest);
/** @brief Queues an anonymous program for later lookup and ID assignment. */
void    data_queue_anonymous_lookup(Edje_Part_Collection *pc, Edje_Program *ep, int *dest);
/** @brief Queues a lookup for an anonymous program ID that was copied. */
void    data_queue_copied_anonymous_lookup(Edje_Part_Collection *pc, int *src, int *dest);
/** @brief Queues an image name for later lookup and ID assignment. */
void    data_queue_image_lookup(char *name, int *dest, Eina_Bool *set);
/** @brief Queues a lookup for an image ID that was copied. */
void    data_queue_copied_image_lookup(int *src, int *dest, Eina_Bool *set);
/** @brief Removes a queued image lookup. */
void    data_queue_image_remove(int *dest, Eina_Bool *set);
/** @brief Processes all queued lookups (parts, programs, images, etc.) to resolve names to IDs. */
void    data_process_lookups(void);
/** @brief Processes all script blocks (compiles Embryo, prepares Lua). */
void    data_process_scripts(void);
/** @brief Processes lookups within scripts (e.g., resolving API function names). */
void    data_process_script_lookups(void);
/** @brief Processes the color class definitions and resolves color codes. */
void    process_color_tree(char *s, const char *file_in, int line);

/** @brief Cleans up image-related data in a part description after processing. */
void    part_description_image_cleanup(Edje_Part *ep);

/** @brief Checks if the parser is currently inside a verbatim script block. */
int     is_verbatim(void);
/** @brief Enables or disables verbatim mode for script parsing. */
void    track_verbatim(int on);
/** @brief Sets the content of the current verbatim block. */
void    set_verbatim(char *s, int l1, int l2);
/** @brief Gets the content of the current verbatim block. */
char   *get_verbatim(void);
/** @brief Gets the starting line number of the current verbatim block. */
int     get_verbatim_line1(void);
/** @brief Gets the ending line number of the current verbatim block. */
int     get_verbatim_line2(void);
/** @brief Main compilation function that orchestrates the parsing and processing. */
void    compile(void);
/** @brief Checks if the Nth token in the current statement is a parameter (string). */
int     is_param(int n);
/** @brief Checks if the Nth token in the current statement is a number. */
int     is_num(int n);
/** @brief Parses the Nth token as a string. */
char   *parse_str(int n);
/** @brief Parses the Nth token as an enum value, checking against a list of valid string-to-int mappings. */
int     parse_enum(int n, ...);
/** @brief Parses the Nth token as a set of flags, checking against a list of valid string-to-int mappings. */
int     parse_flags(int n, ...);
/** @brief Parses the Nth token as an integer. */
int     parse_int(int n);
/** @brief Parses the Nth token as an integer within a specified range. */
int     parse_int_range(int n, int f, int t);
/** @brief Parses the Nth token as a boolean (0 or 1). */
int     parse_bool(int n);
/** @brief Parses the Nth token as a floating-point number. */
double  parse_float(int n);
/** @brief Parses the Nth token as a floating-point number within a specified range. */
double  parse_float_range(int n, double f, double t);
/** @brief Gets the total number of arguments (tokens) in the current statement. */
int     get_arg_count(void);
/** @brief Checks if the current statement has exactly N arguments. Aborts on failure. */
void    check_arg_count(int n);
/** @brief Checks if the current statement has at least N arguments. Aborts on failure. */
void    check_min_arg_count(int n);
/** @brief Checks if the current statement has between N and M arguments (inclusive). Aborts on failure. */
int     check_range_arg_count(int n, int m);
/** @brief Checks if the Nth parameter was originally quoted in the EDC source. */
int     param_had_quote(int n);

/** @brief Returns the number of registered top-level object handlers. */
int     object_handler_num(void);
/** @brief Returns the number of registered short top-level object handlers. */
int     object_handler_short_num(void);
/** @brief Returns the number of registered statement handlers. */
int     statement_handler_num(void);
/** @brief Returns the number of registered short statement handlers. */
int     statement_handler_short_num(void);
/** @brief Returns the number of registered short single-token statement handlers. */
int     statement_handler_short_single_num(void);
/** @brief Returns the number of registered nested block handlers. */
int     nested_handler_num(void);
/** @brief Returns the number of registered short nested block handlers. */
int     nested_handler_short_num(void);

/** @brief Registers a new color class name. */
void    color_class_register(const char *name);

/** @brief Reorders parts based on `insert_before` and `insert_after` properties. */
void    reorder_parts(void);
/** @brief Processes the main EDC source file. */
void    source_edd(void);
/** @brief Fetches and processes included source files. */
void    source_fetch(void);
/** @brief Appends source file contents to an Eet_File. */
int     source_append(Eet_File *ef);
/** @brief Loads source file list from an Eet_File. */
SrcFile_List *source_load(Eet_File *ef);
/** @brief Saves the font map to an Eet_File. */
int     source_fontmap_save(Eet_File *ef, Eina_List *fonts);
/** @brief Loads the font map from an Eet_File. */
Edje_Font_List *source_fontmap_load(Eet_File *ef);

/** @brief Allocates memory, aborts on failure. Wrapper around malloc. */
void   *mem_alloc(size_t size);
/** @brief Duplicates a string, aborts on failure. Wrapper around strdup. */
char   *mem_strdup(const char *s);
/** @brief Macro for `sizeof`, used for brevity. */
#define SZ sizeof

/**
 * @brief Records a file being used during compilation (for watch/dependency tracking).
 * @param filename The name of the file.
 * @param type 'E' for EDC source, 'I' for image, 'F' for font, 'S' for sound, 'O' for output EDJ, etc.
 */
void    using_file(const char *filename, const char type);

/**
 * @brief Prints an error message and aborts compilation, cleaning up Eet_File if provided.
 * @param ef Optional Eet_File to close before aborting.
 * @param fmt The format string for the error message.
 * @param ... Variable arguments for the format string.
 */
void    error_and_abort(Eet_File *ef, const char *fmt, ...) EINA_PRINTF(2, 3);

/** @brief Pushes a string onto a quick parsing stack (likely for nested state). */
void stack_push_quick(const char *str);
/**
 * @brief Pops a string from the quick parsing stack.
 * @param check_last If true, ensures the popped string matches an expected value (not implemented here).
 * @param do_free If true, frees the popped string.
 * @return The popped string.
 */
char *stack_pop_quick(Eina_Bool check_last, Eina_Bool do_free);
/** @brief Replaces the top of the quick parsing stack with a new token. */
void stack_replace_quick(const char *token);
/** @brief Checks if wildcard handling is enabled for EDC handlers. */
Eina_Bool edje_cc_handlers_wildcard(void);
/** @brief Allocates data structures for handler hierarchy management. */
void edje_cc_handlers_hierarchy_alloc(void);
/** @brief Frees data structures for handler hierarchy management. */
void edje_cc_handlers_hierarchy_free(void);
/** @brief Notifies handlers when a parsing stack level is popped. */
void edje_cc_handlers_pop_notify(const char *token);
/** @brief Gets the index of a parameter string (token) from the current statement. */
int get_param_index(char *str);

/** @brief Frees the root of the parsed color class tree. */
void color_tree_root_free(void);
/**
 * @brief Converts a color string (e.g., "#RRGGBBAA", "color_class_name") into RGBA components.
 * @param str The input color string.
 * @param r Pointer to store the red component (0-255).
 * @param g Pointer to store the green component (0-255).
 * @param b Pointer to store the blue component (0-255).
 * @param a Pointer to store the alpha component (0-255).
 */
void convert_color_code(char *str, int *r, int *g, int *b, int *a);

/** @brief Rewrites script code, possibly for macro expansion or other preprocessing. */
void script_rewrite(Code *code);

/* global vars */
extern Eina_List             *ext_dirs;         /**< List of directories to search for external files. */
extern Eina_List             *img_dirs;         /**< List of directories to search for image files. Paths are `char *`. */
extern Eina_List             *fnt_dirs;         /**< List of directories to search for font files. Paths are `char *`. */
extern Eina_List             *snd_dirs;         /**< List of directories to search for sound files. Paths are `char *`. */
extern Eina_List             *mo_dirs;          /**< List of directories to search for localization (.mo) files. Paths are `char *`. */
extern Eina_List             *vibration_dirs;   /**< List of directories to search for vibration pattern files. Paths are `char *`. */
extern Eina_List             *data_dirs;        /**< List of directories to search for generic data files. Paths are `char *`. */
extern char                  *file_in;          /**< Path to the input EDC file. */
extern char                  *file_out;         /**< Path to the output EDJ file. */
extern char                  *watchfile;        /**< Path to the file for dumping source file paths (for watching changes). */
extern char                  *depfile;          /**< Path to the file for dumping GNU make-style dependencies. */
extern char                  *license;          /**< Path to the main license file. */
extern char                  *authors;          /**< Path to the main authors file. */
extern Eina_List             *licenses;         /**< List of additional license files. Paths are `char *`. */
extern int                    no_lossy;         /**< Flag: Disallow lossy image compression. */
extern int                    no_comp;          /**< Flag: Disallow lossless image compression (store uncompressed). */
extern int                    no_raw;           /**< Flag: Disallow raw (uncompressed, unoptimized) image storage. */
extern int                    no_etc1;          /**< Flag: Disallow ETC1 compression for images. */
extern int                    no_etc2;          /**< Flag: Disallow ETC2 compression for images. */
extern int                    no_save;          /**< Flag: Do not save EDC source files into the EDJ. */
extern int                    min_quality;      /**< Minimum quality for lossy image compression (0-100). */
extern int                    max_quality;      /**< Maximum quality for lossy image compression (0-100). */
extern int                    line;             /**< Current line number being parsed in the input file. */
extern Eina_List             *stack;            /**< Main parsing stack, stores parser state/context. Elements are internal parser structures. */
extern Edje_File             *edje_file;        /**< Top-level structure representing the EDJ file being built. */
extern Eina_List             *edje_collections; /**< List of #Edje_Part_Collection_Parser structures (parsed groups). */
extern Eina_Hash             *edje_collections_lookup; /**< Hash table for quick lookup of collections by name. Name (char*) -> #Edje_Part_Collection_Parser. */
extern Eina_List             *externals;        /**< List of #External structures. */
extern Eina_List             *fonts;            /**< List of #Edje_Font structures. */
extern Eina_List             *codes;            /**< List of #Code structures (script blocks). */
extern Eina_List             *defines;          /**< List of preprocessor defines (`char *` like "-DNAME=VALUE"). */
extern Eina_List             *aliases;          /**< List of color aliases (not fully clear how used, likely internal). */
extern New_Object_Handler     object_handlers[]; /**< Array of handlers for top-level EDC objects. */
extern New_Object_Handler     object_handlers_short[]; /**< Array of handlers for short-form top-level EDC objects. */
extern New_Statement_Handler  statement_handlers[]; /**< Array of handlers for statements within EDC objects. */
extern New_Statement_Handler  statement_handlers_short[]; /**< Array of handlers for short-form statements. */
extern New_Statement_Handler  statement_handlers_short_single[]; /**< Array of handlers for single-token short-form statements. */
extern New_Nested_Handler     nested_handlers[]; /**< Array of handlers for nested blocks. */
extern New_Nested_Handler     nested_handlers_short[]; /**< Array of handlers for short-form nested blocks. */
extern int                    compress_mode;    /**< EET compression mode for the output EDJ file. See EET_Compression_Type. */
extern int                    threads;          /**< Number of threads to use for compilation (0 for main loop only, >0 for multi-threaded). */
extern int                    annotate;         /**< Flag: Annotate dumped source files (used with -w). */
extern Eina_Bool              current_group_inherit; /**< Flag indicating if the current group being parsed has `inherit: 1;`. */
extern Eina_List             *color_tree_root;  /**< Root of the parsed color class tree. Structure is internal to color parsing. */
extern int                    beta;             /**< Flag: Enable beta features or behavior. */
extern int                    no_warn_unused_images; /**< Flag: Suppress warnings for unused images. */

extern Eina_Hash             *color_class_reg;  /**< Hash table registering defined color classes. Name (char*) -> Color definition (internal struct). */

extern int                    had_quote;        /**< Flag used by parsing functions to indicate if a parsed string token was originally quoted. */

extern unsigned int           max_open_files;   /**< System limit for maximum number of open files. */
extern Eina_Array            *requires;         /**< Array of strings, listing required modules/features for the EDJ. Example: `eina_array_push(requires, eina_stringshare_add("efl_version=1.20"));` */
extern Eina_Bool              namespace_verify; /**< Flag: Enable namespace verification for parts and signals. */
#endif
