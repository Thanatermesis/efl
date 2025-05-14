#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#ifdef HAVE_SYS_TIMES_H
# include <sys/times.h>
#endif

#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <windows.h>
# undef WIN32_LEAN_AND_MEAN
#endif

#include <Eio.h>

#include <Elementary.h>

static Elm_Genlist_Item_Class it_eio;

#ifdef _WIN32
ULONGLONG st_time_kernel;
ULONGLONG st_time_user;
ULONGLONG en_time_kernel;
ULONGLONG en_time_user;
#else
static clock_t st_time;
static clock_t en_time;
static struct tms st_cpu;
static struct tms en_cpu;
#endif

static void _sel_file(void *data, Evas_Object *obj, void *event_info);
static Eina_Bool _ls_filter_cb(void *data, Eio_File *handler, const char *file);
static void _ls_main_cb(void *data, Eio_File *handler, const char *file);
static void _ls_done_cb(void *data, Eio_File *handler);
static void _ls_error_cb(void *data, Eio_File *handler, int error);
static void _file_chosen(void *data, Evas_Object *obj, void *event_info);
static char *_gl_text_get(void *data, Evas_Object *obj, const char *part);
static Evas_Object *_gl_content_get(void *data, Evas_Object *obj, const char *part);
static Eina_Bool _gl_state_get(void *data, Evas_Object *obj, const char *part);
static void _gl_del(void *data, Evas_Object *obj);
static void _test_eio_clear(void *data, Evas_Object *obj, void *event);

/**
 * @brief Callback for when a file is selected in the genlist.
 *
 * This function is currently a no-op.
 * @param data User data, unused.
 * @param obj The Evas object, unused.
 * @param event_info Event-specific info, unused.
 */
static void
_sel_file(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
}

/**
 * @brief Filter callback for eio_file_ls.
 *
 * This function is called for each file found by eio_file_ls and decides
 * whether to include it in the results. This implementation includes all files.
 * @param data User data, unused.
 * @param handler The Eio_File handler.
 * @param file The file name.
 * @return EINA_TRUE to include the file, EINA_FALSE to exclude.
 */
static Eina_Bool
_ls_filter_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, const char *file EINA_UNUSED)
{
  return EINA_TRUE;
}

/**
 * @brief Comparison function for sorting genlist items.
 *
 * This function is used by elm_genlist_item_sorted_insert to sort items
 * alphabetically based on their string data.
 * @param data1 First item to compare (an Elm_Object_Item*).
 * @param data2 Second item to compare (an Elm_Object_Item*).
 * @return An integer less than, equal to, or greater than zero if data1 is
 * found, respectively, to be less than, to match, or be greater than data2.
 */
static int
_compare_cb(const void *data1, const void *data2)
{
   Elm_Object_Item *it = (Elm_Object_Item *)data1;
   Elm_Object_Item *it2 = (Elm_Object_Item *)data2;
   return strcoll(elm_object_item_data_get(it),
                  elm_object_item_data_get(it2));
}

/**
 * @brief Main callback for eio_file_ls, called for each file.
 *
 * This function is called for each file that passes the filter. It inserts
 * the file as a new item into the genlist, sorted alphabetically.
 * @param data The genlist widget.
 * @param handler The Eio_File handler.
 * @param file The file name.
 */
static void
_ls_main_cb(void *data, Eio_File *handler EINA_UNUSED, const char *file)
{
   elm_genlist_item_sorted_insert(data,
                                  &it_eio,
                                  eina_stringshare_add(file),
                                  NULL,
                                  ELM_GENLIST_ITEM_NONE,
                                  _compare_cb,
                                  _sel_file,
                                  NULL);
}

/**
 * @brief Callback for when eio_file_ls has finished listing files.
 *
 * This function is called when the directory listing operation is complete.
 * It calculates and prints the time taken for the operation.
 * @param data User data, unused.
 * @param handler The Eio_File handler.
 */
static void
_ls_done_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED)
{
#ifdef _WIN32
   FILETIME tc;
   FILETIME te;
   FILETIME tk;
   FILETIME tu;
   ULARGE_INTEGER time_kernel;
   ULARGE_INTEGER time_user;

   if (!GetProcessTimes(GetCurrentProcess(),
                        &tc, &te, &tk, &tu))
     return;

   time_kernel.u.LowPart = tk.dwLowDateTime;
   time_kernel.u.HighPart = tk.dwHighDateTime;
   en_time_kernel = time_kernel.QuadPart;

   time_user.u.LowPart = tu.dwLowDateTime;
   time_user.u.HighPart = tu.dwHighDateTime;
   en_time_user = time_user.QuadPart;

   fprintf(stderr, "ls done\n");
   fprintf(stderr, "Kernel Time: %lld, User Time: %lld",
           (en_time_kernel - st_time_kernel),
           (en_time_user - st_time_user));
#else
   en_time = times(&en_cpu);
   fprintf(stderr, "ls done\n");
   fprintf(stderr, "Real Time: %.jd, User Time: %.jd, System Time: %.jd\n",
           (intmax_t)(en_time - st_time),
           (intmax_t)(en_cpu.tms_utime - st_cpu.tms_utime),
           (intmax_t)(en_cpu.tms_stime - st_cpu.tms_stime));
#endif
}

/**
 * @brief Callback for when an error occurs in eio_file_ls.
 *
 * This function is called if an error happens during the file listing.
 * It prints the error message to stderr.
 * @param data User data, unused.
 * @param handler The Eio_File handler.
 * @param error The error code (errno).
 */
static void
_ls_error_cb(void *data EINA_UNUSED, Eio_File *handler EINA_UNUSED, int error)
{
   fprintf(stderr, "error: [%s]\n", strerror(error));
}

/**
 * @brief Callback for when a file/directory is chosen in the fileselector.
 *
 * This function is triggered by the "file,chosen" smart callback of the
 * fileselector button. It starts the asynchronous directory listing with Eio.
 * It also records the start time for performance measurement.
 * @param data The genlist widget, passed as user data.
 * @param obj The fileselector button, unused.
 * @param event_info The chosen file path (a const char*).
 */
static void
_file_chosen(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   const char *file = event_info;
   if (file)
     {
#ifdef _WIN32
        FILETIME tc;
        FILETIME te;
        FILETIME tk;
        FILETIME tu;
        ULARGE_INTEGER time_kernel;
        ULARGE_INTEGER time_user;

        if (!GetProcessTimes(GetCurrentProcess(),
                             &tc, &te, &tk, &tu))
          return;

        time_kernel.u.LowPart = tk.dwLowDateTime;
        time_kernel.u.HighPart = tk.dwHighDateTime;
        st_time_kernel = time_kernel.QuadPart;

        time_user.u.LowPart = tu.dwLowDateTime;
        time_user.u.HighPart = tu.dwHighDateTime;
        st_time_user = time_user.QuadPart;
#else
        st_time = times(&st_cpu);
#endif
        eio_file_ls(file,
                    _ls_filter_cb,
                    _ls_main_cb,
                    _ls_done_cb,
                    _ls_error_cb,
                    data);
     }
}

/**
 * @brief Get the text for a genlist item.
 *
 * This is a callback function for the genlist item class. It provides the
 * text to be displayed for each item.
 * @param data The item's data (the file path as a string).
 * @param obj The genlist widget, unused.
 * @param part The theme part name, unused.
 * @return A newly allocated string for the item's label. The caller is
 * responsible for freeing it. E.g., "Item # /path/to/file".
 */
static char *
_gl_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   char buf[PATH_MAX];
   snprintf(buf, sizeof(buf), "Item # %s", (char*)data);
   return strdup(buf);
}

/**
 * @brief Get the content for a genlist item.
 *
 * This is a callback function for the genlist item class. It provides a
 * content object (like an icon) for the item. This implementation returns NULL.
 * @param data The item's data, unused.
 * @param obj The genlist widget, unused.
 * @param part The theme part name, unused.
 * @return An Evas_Object to be used as content, or NULL.
 */
static Evas_Object *
_gl_content_get(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   return NULL;
}

/**
 * @brief Get the state for a genlist item.
 *
 * This is a callback function for the genlist item class. It's used for
 * parts like check boxes or radio buttons to get their state. This
 * implementation always returns false (unchecked).
 * @param data The item's data, unused.
 * @param obj The genlist widget, unused.
 * @param part The theme part name, unused.
 * @return EINA_TRUE for "on" state, EINA_FALSE for "off".
 */
static Eina_Bool
_gl_state_get(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   return EINA_FALSE;
}

/**
 * @brief Callback for genlist item deletion.
 *
 * This is a callback function for the genlist item class, called when an
 * item is deleted. It can be used to free item-specific data.
 * @param data The item's data, unused.
 * @param obj The genlist widget, unused.
 */
static void
_gl_del(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED)
{
}

/**
 * @brief Callback to clear the genlist.
 *
 * This function is connected to the "clicked" signal of the "clear" button.
 * It removes all items from the genlist.
 * @param data The genlist widget to be cleared.
 * @param obj The button that was clicked, unused.
 * @param event Event-specific information, unused.
 */
static void
_test_eio_clear(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   elm_genlist_clear(data);
}

/**
 * @brief Main function for the Eio test.
 *
 * This function sets up the window and all the UI components for the Eio
 * test application. This includes a genlist to display files, a fileselector
 * button to choose a directory, and a clear button.
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_eio(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *vbox, *hbox, *ic, *bt, *fs_bt, *gl;

   it_eio.item_style     = "default";
   it_eio.func.text_get = _gl_text_get;
   it_eio.func.content_get  = _gl_content_get;
   it_eio.func.state_get = _gl_state_get;
   it_eio.func.del       = _gl_del;

   win = elm_win_util_standard_add("fileselector-button", "File Selector Button");
   elm_win_autodel_set(win, EINA_TRUE);

   vbox = elm_box_add(win);
   evas_object_size_hint_weight_set(vbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, vbox);
   evas_object_show(vbox);

   gl = elm_genlist_add(win);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(vbox, gl);
   evas_object_show(gl);

   /* file selector button */
   hbox = elm_box_add(win);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   ic = elm_icon_add(win);
   elm_icon_standard_set(ic, "file");
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   fs_bt = elm_fileselector_button_add(win);
   elm_object_text_set(fs_bt, "Select a dir");
   elm_object_part_content_set(fs_bt, "icon", ic);
   elm_fileselector_button_inwin_mode_set(fs_bt, EINA_TRUE);
   elm_fileselector_folder_only_set(fs_bt, EINA_TRUE);

   elm_box_pack_end(hbox, fs_bt);
   elm_box_pack_end(vbox, hbox);
   evas_object_show(fs_bt);
   evas_object_show(ic);

   /* attribute setting buttons */
   bt = elm_button_add(win);
   elm_object_text_set(bt, "clear");
   evas_object_smart_callback_add(bt, "clicked", _test_eio_clear, gl);
   elm_box_pack_end(hbox, bt);
   evas_object_show(bt);
   evas_object_show(hbox);

   evas_object_smart_callback_add(fs_bt, "file,chosen", _file_chosen, gl);

   evas_object_resize(win, 300 * elm_config_scale_get(),
                           500 * elm_config_scale_get());
   evas_object_show(win);
}
