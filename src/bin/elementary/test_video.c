#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

/**
 * @brief Callback function for when a video file is selected.
 *
 * This function is called when the "file,chosen" smart callback is emitted
 * from the file selector button. It sets the chosen file on the video object
 * and starts playback.
 *
 * @param data The video object (Evas_Object *) to control.
 * @param obj The object that emitted the signal (unused).
 * @param event_info The selected file path (const char *).
 */
static void
my_bt_open(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Object *video = data;
   const char *file = event_info;

   if (file)
     {
        elm_video_file_set(video, file);
        elm_video_play(video);
     }
}

/**
 * @brief Creates the video test window.
 *
 * This function sets up a window with an elm_player widget that contains an
 * elm_video object. It also adds a file selector button to allow the user
 * to choose a video file to play. This serves as a test case for the
 * Elementary video player functionality.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_video(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bg, *video, *bt, *tb, *player;

   win = elm_win_add(NULL, "video", ELM_WIN_BASIC);
   elm_win_title_set(win, "Video");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   video = elm_video_add(win);
   evas_object_size_hint_weight_set(video, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(video);

   player = elm_player_add(win);
   evas_object_size_hint_weight_set(player, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_content_set(player, video);
   elm_win_resize_object_add(win, player);
   evas_object_show(player);

   tb = elm_table_add(win);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, tb);
   evas_object_show(tb);

   bt = elm_fileselector_button_add(win);
   elm_object_text_set(bt, "Select Video");
   evas_object_smart_callback_add(bt, "file,chosen", my_bt_open, video);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bt, 0.5, 0.1);
   elm_table_pack(tb, bt, 0, 0, 1, 1);
   evas_object_show(bt);

   evas_object_resize(win, 800 * elm_config_scale_get(),
                           500 * elm_config_scale_get());
   evas_object_show(win);
}
