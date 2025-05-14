/* NOTE : Before testing elm_store,
          email data files should exist in your local storage.
          And you can just get example files in enlightenment website.
          Use wget to obtain it. It almost 50 Megabytes.
          http://www.enlightenment.org/~raster/store.tar.gz
 */

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

typedef struct _My_Item My_Item;

struct _My_Item
{
  char *from, *subject, *date, *head_content;
};

/**
 * @brief Callback for the "selected" event on the genlist.
 * @param data User data pointer (unused).
 * @param obj The Evas_Object that emitted the signal (unused).
 * @param event_info The event-specific information. For "selected", this is a pointer to the selected Elm_Object_Item.
 */
// callbacks just to see user interacting with genlist
static void
_st_selected(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   printf("selected: %p\n", event_info);
}

/**
 * @brief Callback for the "clicked,double" event on the genlist.
 * @param data User data pointer (unused).
 * @param obj The Evas_Object that emitted the signal (unused).
 * @param event_info The event-specific information. For "clicked,double", this is a pointer to the double-clicked Elm_Object_Item.
 */
static void
_st_double_clicked(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   printf("double clicked: %p\n", event_info);
}

/**
 * @brief Callback for the "longpressed" event on the genlist.
 * @param data User data pointer (unused).
 * @param obj The Evas_Object that emitted the signal (unused).
 * @param event_info The event-specific information. For "longpressed", this is a pointer to the long-pressed Elm_Object_Item.
 */
static void
_st_longpress(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   printf("longpress %p\n", event_info);
}

// store callbacks to handle loading/parsing/freeing of store items from src
static Elm_Genlist_Item_Class *itc1;

static const Elm_Store_Item_Mapping it1_mapping[] =
{
  {
    ELM_STORE_ITEM_MAPPING_LABEL,
      "elm.title.1", ELM_STORE_ITEM_MAPPING_OFFSET(My_Item, from),
      { .empty = {
        EINA_TRUE
      } } },
  {
    ELM_STORE_ITEM_MAPPING_LABEL,
      "elm.title.2", ELM_STORE_ITEM_MAPPING_OFFSET(My_Item, subject),
      { .empty = {
        EINA_TRUE
      } } },
  {
    ELM_STORE_ITEM_MAPPING_LABEL,
      "elm.text", ELM_STORE_ITEM_MAPPING_OFFSET(My_Item, head_content),
      { .empty = {
        EINA_TRUE
      } } },
  {
    ELM_STORE_ITEM_MAPPING_ICON,
      "elm.swallow.icon", 0,
      { .icon = {
        48, 48,
        ELM_ICON_LOOKUP_THEME_FDO,
        EINA_TRUE, EINA_FALSE,
        EINA_TRUE,
        EINA_FALSE, EINA_FALSE,
      } } },
  {
    ELM_STORE_ITEM_MAPPING_CUSTOM,
      "elm.swallow.end", 0,
      { .custom = {
        NULL
      } } },
  ELM_STORE_ITEM_MAPPING_END
};

/**
 * @brief Lists items from the filesystem for the store.
 *
 * This function is called by the store for each file found in the target
 * directory. It runs in a worker thread.
 * Its purpose is to quickly decide if a file should be included in the store,
 * and to provide basic information like a sorting key and the item class
 * without parsing the whole file.
 * The sorting key is a string generated from the file's numeric ID, allowing
 * for lexicographical sorting that is equivalent to numerical sorting.
 *
 * @param data User data passed to elm_store_list_func_set() (unused).
 * @param item_info Information about the file item. This function populates
 *        the base fields of this struct.
 * @return @c EINA_TRUE to include the item in the store, @c EINA_FALSE to
 *         ignore it.
 */
////// **** WARNING ***********************************************************
////   * This function runs inside a thread outside efl mainloop. Be careful! *
//     ************************************************************************
static Eina_Bool
_st_store_list(void *data EINA_UNUSED, Elm_Store_Item_Info *item_info)
{
   Elm_Store_Item_Info_Filesystem *info = (Elm_Store_Item_Info_Filesystem *)item_info;
   int id;
   char sort_id[7];

   // create a sort id based on the filename itself assuming it is a numeric
   // value like the id number in mh mail folders which is what this test
   // uses as a data source
   char *file = strrchr(info->path, '/');
   if (file) file++;
   else file = info->path;
   id = atoi(file);
   sort_id[0] = ((id >> 30) & 0x3f) + 32;
   sort_id[1] = ((id >> 24) & 0x3f) + 32;
   sort_id[2] = ((id >> 18) & 0x3f) + 32;
   sort_id[3] = ((id >> 12) & 0x3f) + 32;
   sort_id[4] = ((id >>  6) & 0x3f) + 32;
   sort_id[5] = ((id >>  0) & 0x3f) + 32;
   sort_id[6] = 0;
   info->base.sort_id = strdup(sort_id);
   // choose the item genlist item class to use (only item style should be
   // provided by the app, store will fill everything else in, so it also
   // has to be writable
   info->base.item_class = itc1; // based on item info - return the item class wanted (only style field used - rest reset to internal funcs store sets up to get label/icon etc)
   info->base.mapping = it1_mapping;
   info->base.data = NULL; // if we can already parse and load all of item here and want to - set this
   return EINA_TRUE; // return true to include this, false not to
}
//     ************************************************************************
////   * End of separate thread function.                                     *
////// ************************************************************************

/**
 * @brief Fetches the detailed data for a specific store item.
 *
 * This function is called by the store when an item's full data is needed
 * (e.g., when it is about to become visible in a list). It runs in a worker
 * thread to avoid blocking the main loop.
 * It reads an email file, parses out the From, Subject, and Date headers,
 * and a short preview of the body content. This data is then attached to the
 * store item.
 *
 * @param data User data passed to elm_store_fetch_func_set() (unused).
 * @param sti The store item to fetch data for. The parsed data should be
 *        attached to it using elm_store_item_data_set().
 */
////// **** WARNING ***********************************************************
////   * This function runs inside a thread outside efl mainloop. Be careful! *
//     ************************************************************************
static void
_st_store_fetch(void *data EINA_UNUSED, Elm_Store_Item *sti)
{
   const char *path = elm_store_item_filesystem_path_get(sti);
   My_Item *myit;
   FILE *f;
   char buf[4096], *p;
   Eina_Bool have_content = EINA_FALSE;
   char *content = NULL, *content_pos = NULL, *content_end = NULL;

   // if we already have my item data - skip
   if (elm_store_item_data_get(sti)) return;
   // open the mail file and parse it
   f = fopen(path, "rb");
   if (!f) return;

   // alloc my item in memory that holds data to show in the list
   myit = calloc(1, sizeof(My_Item));
   if (!myit)
     {
        fclose(f);
        return;
     }
   while (fgets(buf, sizeof(buf), f))
     {
        if (!have_content)
          {
             if (!isblank(buf[0]))
               {
                  // get key: From:, Subject: etc.
                  if (!strncmp(buf, "From:", 5))
                    {
                       p = buf + 5;
                       while ((*p) && (isblank(*p))) p++;
                       p = strdup(p);
                       if (p)
                         {
                            myit->from = p;
                            p = strchr(p, '\n');
                            if (p) *p = 0;
                         }
                    }
                  else if (!strncmp(buf, "Subject:", 8))
                    {
                       p = buf + 8;
                       while ((*p) && (isblank(*p))) p++;
                       p = strdup(p);
                       if (p)
                         {
                            myit->subject = p;
                            p = strchr(p, '\n');
                            if (p) *p = 0;
                         }
                    }
                  else if (!strncmp(buf, "Date:", 5))
                    {
                       p = buf + 5;
                       while ((*p) && (isblank(*p))) p++;
                       p = strdup(p);
                       if (p)
                         {
                            myit->date = p;
                            p = strchr(p, '\n');
                            if (p) *p = 0;
                         }
                    }
                  else if (buf[0] == '\n') // begin of content
                    have_content = EINA_TRUE;
               }
          }
        else
          {
             // get first 320 bytes of content/body
             if (!content)
               {
                  content = calloc(1, 320);
                  content_pos = content;
                  content_end = content + 319;
               }
             strncat(content_pos, buf, content_end - content_pos - 1);
             content_pos = content + strlen(content);
          }
     }
   fclose(f);
   myit->head_content = elm_entry_utf8_to_markup(content);
   free(content);
   elm_store_item_data_set(sti, myit);
}
//     ************************************************************************
////   * End of separate thread function.                                     *
////// ************************************************************************

/**
 * @brief Frees the data associated with a store item.
 *
 * This function is called by the store when an item's data is no longer
 * needed (e.g., when it has been scrolled far off-screen). Its purpose is to
 * free up memory.
 *
 * @param data User data passed to elm_store_unfetch_func_set() (unused).
 * @param sti The store item whose data should be freed.
 */
static void
_st_store_unfetch(void *data EINA_UNUSED, Elm_Store_Item *sti)
{
   My_Item *myit = elm_store_item_data_get(sti);
   if (!myit) return;
   if (myit->from) free(myit->from);
   if (myit->subject) free(myit->subject);
   if (myit->date) free(myit->date);
   if (myit->head_content) free(myit->head_content);
   free(myit);
}

/**
 * @brief Sets up and runs the elm_store test.
 *
 * This function creates a window with a genlist widget that is populated
 * from a filesystem store. It configures the store to read files from the
 * "./store" directory, sets up the callbacks for listing, fetching, and
 * un-fetching item data, and displays the window.
 *
 * @param data Test data (unused).
 * @param obj The parent object (unused).
 * @param event_info Event info (unused).
 */
void
test_store(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *gl, *bx;
   Elm_Store *st;

   win = elm_win_util_standard_add("store", "Store");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   gl = elm_genlist_add(win);
   elm_genlist_mode_set(gl, ELM_LIST_COMPRESS);
   evas_object_smart_callback_add(gl, "selected", _st_selected, NULL);
   evas_object_smart_callback_add(gl, "clicked,double", _st_double_clicked, NULL);
   evas_object_smart_callback_add(gl, "longpressed", _st_longpress, NULL);
   evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, gl);
   evas_object_show(gl);

   itc1 = elm_genlist_item_class_new();
   itc1->item_style = "message";

   st = elm_store_filesystem_new();
   elm_store_list_func_set(st, _st_store_list, NULL);
   elm_store_fetch_func_set(st, _st_store_fetch, NULL);
   //elm_store_fetch_thread_set(st, EINA_FALSE);
   elm_store_unfetch_func_set(st, _st_store_unfetch, NULL);
   elm_store_sorted_set(st, EINA_TRUE);
   elm_store_target_genlist_set(st, gl);
   elm_store_filesystem_directory_set(st, "./store");

   /* item_class_ref is needed for itc1. some items can be added in callbacks */
   elm_genlist_item_class_ref(itc1);
   elm_genlist_item_class_free(itc1);

   evas_object_resize(win, 480 * elm_config_scale_get(),
                           800 * elm_config_scale_get());
   evas_object_show(win);
}
