#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eet.h>
#include <Evas.h>
#include <Elementary.h>

/**
 * @internal
 * @brief Holds information for a main loop callback.
 * Used by the WRAPPER_TO_XFER_MAIN_LOOP macro to pass data to
 * a function that will be executed in the main loop thread.
 */
typedef struct
{
   Eina_Debug_Session *session; /**< The debug session. */
   int srcid;                   /**< The source ID. */
   void *buffer;                /**< A data buffer. */
   unsigned int size;           /**< The size of the buffer. */
} _Main_Loop_Info;

/**
 * @brief A macro to create a wrapper function that executes in the main loop thread.
 *
 * This macro generates two functions:
 * 1. A static function `_intern_main_loop<foo>` that is called asynchronously
 *    in the main loop. It unpacks the _Main_Loop_Info data and calls the
 *    actual worker function `_main_loop<foo>`.
 * 2. A function `<foo>` that can be called from any thread. It packages up its
 *    arguments into a _Main_Loop_Info struct and schedules
 *    `_intern_main_loop<foo>` for execution in the main loop.
 *
 * This is used to safely delegate work from a debugger thread to the main
 * application thread in an EFL application.
 *
 * @param foo The base name of the function to be wrapped. A function named
 *            `_main_loop<foo>` must be defined.
 */
#define WRAPPER_TO_XFER_MAIN_LOOP(foo) \
static void \
_intern_main_loop ## foo(void *data) \
{ \
   _Main_Loop_Info *info = data; \
   _main_loop ## foo(info->session, info->srcid, info->buffer, info->size); \
   free(info->buffer); \
   free(info); \
} \
static Eina_Bool \
foo(Eina_Debug_Session *session, int srcid, void *buffer, int size) \
{ \
   _Main_Loop_Info *info = calloc(1, sizeof(*info)); \
   info->session = session; \
   info->srcid = srcid; \
   info->size = size; \
   if (info->size) \
     { \
        info->buffer = malloc(info->size); \
        memcpy(info->buffer, buffer, info->size); \
     } \
   ecore_main_loop_thread_safe_call_async(_intern_main_loop ## foo, info); \
   return EINA_TRUE; \
}

#ifndef WORDS_BIGENDIAN
#define SWAP_64(x) x
#define SWAP_32(x) x
#define SWAP_16(x) x
#define SWAP_DBL(x) x
#else
#define SWAP_64(x) eina_swap64(x)
#define SWAP_32(x) eina_swap32(x)
#define SWAP_16(x) eina_swap16(x)
#define SWAP_DBL(x) SWAP_64(x)
#endif

/**
 * @brief Extracts an integer from a buffer and advances the buffer pointer.
 * @param _buf The buffer to extract from. This pointer is advanced by `sizeof(int)`.
 * @return The extracted integer, with byte order corrected for endianness.
 */
#define EXTRACT_INT(_buf) \
({ \
   int __i; \
   memcpy(&__i, _buf, sizeof(int)); \
   _buf += sizeof(int); \
   SWAP_32(__i); \
})

/**
 * @brief Extracts a double from a buffer and advances the buffer pointer.
 * @param _buf The buffer to extract from. This pointer is advanced by `sizeof(double)`.
 * @return The extracted double, with byte order corrected for endianness.
 */
#define EXTRACT_DOUBLE(_buf) \
({ \
   double __d; \
   memcpy(&__d, _buf, sizeof(double)); \
   _buf += sizeof(double); \
   SWAP_DBL(__d); \
})

/**
 * @brief Extracts a string from a buffer and advances the buffer pointer.
 * @param _buf The buffer to extract from. This pointer is advanced by string length + 1.
 * @return The extracted string (newly allocated). The caller must free it.
 */
#define EXTRACT_STRING(_buf) \
({ \
   char *__s = _buf ? strdup(_buf) : NULL; \
   int __len = (__s ? strlen(__s) : 0) + 1; \
   _buf += __len; \
   __s; \
})

/**
 * @brief Stores an integer into a buffer and advances the buffer pointer.
 * @param _buf The buffer to store into. This pointer is advanced by `sizeof(int)`.
 * @param __i The integer to store. Byte order is corrected for endianness.
 */
#define STORE_INT(_buf, __i) \
({ \
   int __si = SWAP_32(__i); \
   memcpy(_buf, &__si, sizeof(int)); \
   _buf += sizeof(int); \
})

/**
 * @brief Stores a double into a buffer and advances the buffer pointer.
 * @param _buf The buffer to store into. This pointer is advanced by `sizeof(double)`.
 * @param __d The double to store. Byte order is corrected for endianness.
 */
#define STORE_DOUBLE(_buf, __d) \
{ \
   double __d2 = SWAP_DBL(__d); \
   memcpy(_buf, &__d2, sizeof(double)); \
   _buf += sizeof(double); \
}

/**
 * @brief Stores a string into a buffer and advances the buffer pointer.
 * @param _buf The buffer to store into. This pointer is advanced by string length + 1.
 * @param __s The string to store.
 */
#define STORE_STRING(_buf, __s) \
{ \
   int __len = (__s ? strlen(__s) : 0) + 1; \
   if (__s) memcpy(_buf, __s, __len); \
   else *_buf = '\0'; \
   _buf += __len; \
}

/**
 * @brief Delimiter used in test files to separate sections, e.g., for screenshots.
 */
#define SHOT_DELIMITER '+'

/**
 * @brief Defines the types of actions that can be recorded or played back.
 */
typedef enum
{
   EXACTNESS_ACTION_UNKNOWN = 0,   /**< Unknown or uninitialized action. */
   EXACTNESS_ACTION_MOUSE_IN,      /**< Mouse cursor entered the window. */
   EXACTNESS_ACTION_MOUSE_OUT,     /**< Mouse cursor left the window. */
   EXACTNESS_ACTION_MOUSE_WHEEL,   /**< Mouse wheel was scrolled. */
   EXACTNESS_ACTION_MULTI_DOWN,    /**< Mouse button or touch-down event. */
   EXACTNESS_ACTION_MULTI_UP,      /**< Mouse button or touch-up event. */
   EXACTNESS_ACTION_MULTI_MOVE,    /**< Mouse or touch move event. */
   EXACTNESS_ACTION_KEY_DOWN,      /**< A keyboard key was pressed. */
   EXACTNESS_ACTION_KEY_UP,        /**< A keyboard key was released. */
   EXACTNESS_ACTION_TAKE_SHOT,     /**< A screenshot should be taken. */
   EXACTNESS_ACTION_EFL_EVENT,     /**< An EFL smart event should be triggered on a widget. */
   EXACTNESS_ACTION_CLICK_ON,      /**< A widget should be clicked. */
   EXACTNESS_ACTION_STABILIZE,     /**< Wait for the UI to become idle/stable. */
   EXACTNESS_ACTION_LAST = EXACTNESS_ACTION_STABILIZE /**< Marker for the last action type. */
   /* Add any supported actions here and update _LAST */
} Exactness_Action_Type;

/**
 * @brief Data for a mouse wheel action.
 */
typedef struct
{
   int direction; /**< The direction of the wheel scroll (0 for vertical). */
   int z;         /**< The amount of wheel movement. */
} Exactness_Action_Mouse_Wheel;

/**
 * @brief Data for a key down or key up action.
 * These fields correspond to the parameters of Evas key events.
 */
typedef struct
{
   const char *keyname;   /**< The name of the key (e.g., "Return"). */
   const char *key;       /**< The logical key symbol (e.g., "Return"). */
   const char *string;    /**< The generated string if the key is printable. */
   const char *compose;   /**< The composed string. */
   unsigned int keycode;  /**< The hardware keycode. */
} Exactness_Action_Key_Down_Up;

/**
 * @brief Data for a multi-touch down or up event.
 * Corresponds to Evas' multi-touch event data.
 */
typedef struct
{
   int d;                     /**< The touch-point's device ID. */
   int b;                     /**< For mouse events, the button number. */
   int x;                     /**< The x-coordinate of the event. */
   int y;                     /**< The y-coordinate of the event. */
   double rad;                /**< The radius of the touch area. */
   double radx;               /**< The x-axis radius of the touch area ellipse. */
   double rady;               /**< The y-axis radius of the touch area ellipse. */
   double pres;               /**< The pressure of the touch. */
   double ang;                /**< The angle of the touch ellipse. */
   double fx;                 /**< The x-coordinate with sub-pixel precision. */
   double fy;                 /**< The y-coordinate with sub-pixel precision. */
   Evas_Button_Flags flags;   /**< Evas button flags (e.g., double-click). */
} Exactness_Action_Multi_Event;

/**
 * @brief Data for a multi-touch move event.
 * Corresponds to Evas' multi-touch event data.
 */
typedef struct
{
   int d;      /**< The touch-point's device ID. */
   int x;      /**< The x-coordinate of the event. */
   int y;      /**< The y-coordinate of the event. */
   double rad; /**< The radius of the touch area. */
   double radx;/**< The x-axis radius of the touch area ellipse. */
   double rady;/**< The y-axis radius of the touch area ellipse. */
   double pres;/**< The pressure of the touch. */
   double ang; /**< The angle of the touch ellipse. */
   double fx;  /**< The x-coordinate with sub-pixel precision. */
   double fy;  /**< The y-coordinate with sub-pixel precision. */
} Exactness_Action_Multi_Move;

/**
 * @brief Data for an action that triggers a specific EFL smart event on a widget.
 */
typedef struct
{
   char *wdg_name;   /**< The name of the target widget, as set by `elm_object_name_set()`. */
   char *event_name; /**< The name of the smart event to trigger (e.g., "clicked"). */
} Exactness_Action_Efl_Event;

/**
 * @brief Data for an action that simulates a click on a named widget.
 */
typedef struct
{
   char *wdg_name;	/**< The name of the widget to click, as set by `elm_object_name_set()`. */
} Exactness_Action_Click_On;

/**
 * @brief Represents a single action in a test sequence.
 *
 * This is a generic container for any of the supported action types.
 */
typedef struct
{
   Exactness_Action_Type type;   /**< The type of the action. */
   unsigned int n_evas;          /**< The index of the Evas canvas on which to perform the action. */
   unsigned int delay_ms;        /**< A delay in milliseconds to wait before performing this action. */
   void *data;                   /**< A pointer to a struct holding data specific to the action type. */
} Exactness_Action;

/**
 * @brief Represents a serializable snapshot of an Eo object's state.
 */
typedef struct
{
   long long id;                 /**< The unique ID of the object (the Eo pointer value). */
   long long parent_id;          /**< The unique ID of the object's parent. */
   const char *kl_name;          /**< The class name of the object (e.g., "Elm_Button"). */

   Eina_List *children;          /**< A list of child Exactness_Object pointers. This is not serialized to EET, but rebuilt on load. */

   /* Evas stuff */
   int x;                        /**< The X coordinate of the object's geometry. */
   int y;                        /**< The Y coordinate of the object's geometry. */
   int w;                        /**< The width of the object. */
   int h;                        /**< The height of the object. */
} Exactness_Object;

/**
 * @brief A container for a snapshot of the object tree from a window.
 */
typedef struct
{
   Eina_List *objs;        /**< A flat list of all serializable objects in the tree. */
   /* main_objs not in EET */
   Eina_List *main_objs;   /**< A list of root objects (those with no parent). Not serialized, but rebuilt on load. */
} Exactness_Objects;

/**
 * @brief Represents a raw image, typically a screenshot.
 */
typedef struct
{
   unsigned int w;   /**< Width of the image in pixels. */
   unsigned int h;   /**< Height of the image in pixels. */
   void *pixels;     /**< Pointer to the raw pixel data (usually 32-bit ARGB). */
} Exactness_Image;

/**
 * @brief Represents a complete test case.
 *
 * A test unit contains the sequence of actions to perform, the expected outcomes (screenshots),
 * and snapshots of the UI object tree.
 */
typedef struct
{
   Eina_List *actions;     /**< A list of Exactness_Action to be performed. */
   /* imgs not in EET */
   Eina_List *imgs;        /**< A list of Exactness_Image, typically screenshots taken during the test. Not directly serialized, but stored in the EET file. */
   Eina_List *objs;        /**< A list of Exactness_Objects snapshots. */
   const char *fonts_path; /**< A custom font path to be used for the test, ensuring consistent text rendering. */
   int nb_shots;           /**< The number of screenshots (`imgs`) associated with this test unit. */
} Exactness_Unit;

/**
 * @internal
 * @brief Get the string representation for an action type.
 * @param type The action type.
 * @return A string constant for the type.
 */
const char *_exactness_action_type_to_string_get(Exactness_Action_Type type);

/**
 * @brief Check if the current process is the original application being tested.
 * Exactness relaunches the application, so this is used to distinguish
 * between the parent (test runner) and child (tested app) processes.
 * @return EINA_TRUE if it is the original process, EINA_FALSE otherwise.
 */
Eina_Bool ex_is_original_app(void);
/**
 * @brief Set an environment variable to mark the current process as the original one.
 * This is called by the test runner to be checked by `ex_is_original_app()`.
 */
void ex_set_original_envvar(void);
/**
 * @brief Compare two images and produce a visual diff.
 * @param img1 The first image.
 * @param img2 The second image.
 * @param[out] diff_img If the images differ, this will be set to a newly allocated
 *                      image highlighting the differences. The caller must free it using exactness_image_free().
 * @return EINA_TRUE if the images are different, EINA_FALSE otherwise.
 */
Eina_Bool exactness_image_compare(Exactness_Image *img1, Exactness_Image *img2, Exactness_Image **diff_img);
/**
 * @brief Reads a complete test unit from a .ext file.
 * @param filename The path to the test file.
 * @return A newly allocated Exactness_Unit, or NULL on failure. The caller is
 *         responsible for freeing the unit and its contents.
 */
Exactness_Unit *exactness_unit_file_read(const char *filename);
/**
 * @brief Writes a complete test unit to a .ext file.
 * @param unit The test unit to write.
 * @param filename The path to the output file.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool exactness_unit_file_write(Exactness_Unit *unit, const char *filename);
/**
 * @brief Frees the memory used by an Exactness_Image.
 * @param img The image to free.
 */
void exactness_image_free(Exactness_Image *img);

/**
 * @brief Adds the Exactness theme overlay.
 * This is used by the player to display UI elements over the application
 * being tested.
 */
void ex_prepare_elm_overlay(void);
