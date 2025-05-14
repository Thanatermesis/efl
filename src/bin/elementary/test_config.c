#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

#ifdef MAX_PROFILES
# undef MAX_PROFILES
#endif

#define MAX_PROFILES 20

#ifdef LOG
# undef LOG
#endif

#define LOG(m)                                                               \
   do {                                                                      \
        lb = elm_label_add(win);                                             \
        elm_object_text_set(lb, m);                                          \
        evas_object_size_hint_weight_set(lb, 0.0, 0.0);                      \
        evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, EVAS_HINT_FILL); \
        evas_object_show(lb);                                                \
   } while(0)

typedef struct _Prof_Data Prof_Data;
typedef struct _App_Data  App_Data;

/**
 * @brief Holds data for a profile configuration UI section.
 *
 * This struct contains widgets and data related to selecting a profile and
 * a set of available profiles, either for the current window or for a new
 * window to be created.
 */
struct _Prof_Data
{
   Evas_Object *rdg;  /**< Radio group for selecting a single profile. */
   Eina_List   *cks;  /**< List of checkboxes for selecting available profiles. */
   const char  *profile; /**< The name of the currently selected profile. */
   const char  *available_profiles[MAX_PROFILES]; /**< Array of available profile names. */
   int          count; /**< Number of profiles in available_profiles. */
};

/**
 * @brief Application data for the configuration test.
 *
 * This struct holds all the relevant data for a single test window instance,
 * including its window object, the list of all system profiles, and profile
 * data for both the current window and for creating a new one.
 */
struct _App_Data
{
   Evas_Object *win; /**< The main window of the test application. */
   Eina_List   *profiles; /**< List of all profiles available in the system. */
   Prof_Data    curr; /**< Profile data for the current window. */
   Prof_Data    new; /**< Profile data for creating a new window. */
};

void test_config(void *data, Evas_Object *obj, void *event_info);

/**
 * @brief Clears the selected profile string in a Prof_Data structure.
 * @param pd The profile data structure to modify.
 */
static void
_profile_clear(Prof_Data *pd)
{
   if (pd->profile)
     eina_stringshare_del(pd->profile);
   pd->profile = NULL;
}

/**
 * @brief Clears the list of available profiles in a Prof_Data structure.
 *
 * This function iterates through the available_profiles array and frees
 * each stringshare instance.
 * @param pd The profile data structure to modify.
 */
static void
_profiles_clear(Prof_Data *pd)
{
   int i;
   for (i = 0; i < MAX_PROFILES; i++)
     {
        if (pd->available_profiles[i])
          eina_stringshare_del(pd->available_profiles[i]);
        pd->available_profiles[i] = NULL;
     }
}

/**
 * @brief Updates the UI label to show the window's current and available profiles.
 *
 * It retrieves the current profile and the list of available profiles from the
 * window and formats them into a string to be displayed in a label.
 * @param win The window object whose profile information is to be displayed.
 */
static void
_profile_update(Evas_Object *win)
{
   Evas_Object *lb = evas_object_data_get(win, "lb");
   char **profiles = NULL;
   const char *profile;
   unsigned int i, n = 0;
   char buf[PATH_MAX];

   profile = elm_win_profile_get(win);
   snprintf(buf, sizeof(buf),
            "Profile: <b>%s</b><br/>"
            "Available profiles: <b>",
            profile);

   elm_win_available_profiles_get(win, &profiles, &n);
   if ((profiles) && (n > 0))
     {
        for (i = 0; i < n; i++)
          {
             if (strlen(buf) >= (sizeof(buf) - 3)) break;
             if (i >= 1) strcat(buf, ", ");
             if (strlen(buf) >= (sizeof(buf) - 1 - strlen(profiles[i]))) break;
             strcat(buf, profiles[i]);
          }
        if (strlen(buf) < (sizeof(buf) - 5)) strcat(buf, "</b>");
     }
   elm_object_text_set(lb, buf);
}

/**
 * @brief Callback to set the profile of the current window.
 *
 * This function is called when the "Set" button for the current window's
 * profile is clicked. It gets the selected profile from the radio group
 * and applies it to the window.
 * @param data The window object.
 * @param obj Unused.
 * @param event_info Unused.
 */
static void
_bt_profile_set(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   App_Data *ad = evas_object_data_get((Evas_Object *)data, "ad");
   Evas_Object *rd = elm_radio_selected_object_get(ad->curr.rdg);
   const char *profile = elm_object_text_get(rd);
   if (strcmp(profile, "Nothing") != 0)
     elm_win_profile_set(ad->win, elm_object_text_get(rd));
   else
     elm_win_profile_set(ad->win, NULL);
   _profile_update(ad->win);
}

/**
 * @brief Callback to set the list of available profiles for the current window.
 *
 * This is triggered by the "Set" button for available profiles. It collects
 * the names of the profiles from the checked boxes and sets them as the
 * available profiles for the window.
 * @param data The window object.
 * @param obj Unused.
 * @param event_info Unused.
 */
static void
_bt_available_profiles_set(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   App_Data *ad = evas_object_data_get((Evas_Object *)data, "ad");
   Eina_List *l = NULL;
   const char *str;
   Evas_Object *o;
   int i = 0;

   _profiles_clear(&ad->curr);

   EINA_LIST_FOREACH(ad->curr.cks, l, o)
     {
        if (elm_check_state_get(o))
          {
             str = evas_object_data_get(o, "profile");
             if (str)
               {
                  ad->curr.available_profiles[i] = eina_stringshare_add(str);
                  i++;
               }
          }
     }
   ad->curr.count = i;

   elm_win_available_profiles_set(ad->win,
                                  ad->curr.available_profiles,
                                  ad->curr.count);
   _profile_update(ad->win);
}

/**
 * @brief Callback to create a new window with a specified profile configuration.
 *
 * This function is triggered by the "Create" button. It gathers the profile
 * and available profiles settings from the "new window" UI section and
 * calls test_config() to create a new window with these settings.
 * @param data The main window object containing the App_Data.
 * @param obj Unused.
 * @param event_info Unused.
 */
static void
_bt_win_add(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   App_Data *ad = evas_object_data_get((Evas_Object *)data, "ad");
   Evas_Object *rd = elm_radio_selected_object_get(ad->new.rdg);
   const char *profile = elm_object_text_get(rd);
   const char *str;
   Eina_List *l = NULL;
   Evas_Object *o;
   int i = 0;

   _profile_clear(&ad->new);
   _profiles_clear(&ad->new);

   if (strcmp(profile, "Nothing") != 0)
     ad->new.profile = (char *)eina_stringshare_add(profile);

   EINA_LIST_FOREACH(ad->new.cks, l, o)
     {
        if (elm_check_state_get(o))
          {
             str = evas_object_data_get(o, "profile");
             if (str)
               {
                  ad->new.available_profiles[i] = eina_stringshare_add(str);
                  i++;
               }
          }
     }
   ad->new.count = i;

   test_config(&(ad->new), NULL, NULL);
}

/**
 * @brief Callback for the "profile,changed" smart event on a window.
 *
 * This function is called when a window's profile has been changed.
 * It calls _profile_update() to refresh the displayed information.
 * @param data Unused.
 * @param obj The window object whose profile changed.
 * @param event Unused.
 */
static void
_win_profile_changed_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   _profile_update(obj);
}

/**
 * @brief Callback for the "delete,request" smart event to clean up resources.
 *
 * This function is called when the window is being closed. It frees all
 * the memory allocated for the App_Data struct and its members.
 * @param data Unused.
 * @param obj The window object that is being deleted.
 * @param event_info Unused.
 */
static void
_win_del_cb(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   App_Data *ad = evas_object_data_get(obj, "ad");
   Evas_Object *o;
   char *str;

   elm_config_profile_list_free(ad->profiles);
   ad->profiles = NULL;

   EINA_LIST_FREE(ad->curr.cks, o)
     {
        str = evas_object_data_del(o, "profile");
        if (str) eina_stringshare_del(str);
     }

   EINA_LIST_FREE(ad->new.cks, o)
     {
        str = evas_object_data_del(o, "profile");
        if (str) eina_stringshare_del(str);
     }

   _profile_clear(&ad->curr);
   _profiles_clear(&ad->curr);
   _profile_clear(&ad->new);
   _profiles_clear(&ad->new);

   free(ad);
}

/**
 * @brief Creates and returns a radio group for profile selection.
 *
 * This function creates a horizontal box with a radio button for each
 * available system profile, plus one for "Nothing".
 * @param win The parent window.
 * @param bx The box to pack the radio group into.
 * @return The created radio group object.
 */
static Evas_Object *
_radio_add(Evas_Object *win, Evas_Object *bx)
{
   App_Data *ad = evas_object_data_get(win, "ad");
   Evas_Object *bx2, *rd, *rdg = NULL;
   Eina_List *l = NULL;
   const char *str;
   int i = 0;

   bx2 = elm_box_add(win);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, 0.0);
   elm_box_align_set(bx2, 0.0, 0.5);
   elm_box_horizontal_set(bx2, EINA_TRUE);
   elm_box_pack_end(bx, bx2);
   evas_object_show(bx2);

   rdg = rd = elm_radio_add(win);
   elm_radio_state_value_set(rd, i);
   elm_radio_group_add(rd, rdg);
   elm_object_text_set(rd, "Nothing");
   evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_box_pack_end(bx2, rd);
   evas_object_show(rd);
   i++;

   EINA_LIST_FOREACH(ad->profiles, l, str)
     {
        rd = elm_radio_add(win);
        elm_radio_state_value_set(rd, i);
        elm_radio_group_add(rd, rdg);
        elm_object_text_set(rd, str);
        evas_object_size_hint_weight_set(rd, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_box_pack_end(bx2, rd);
        evas_object_show(rd);
        i++;
     }

   return rdg;
}

/**
 * @brief Creates and returns a list of checkboxes for available profile selection.
 *
 * This function creates a checkbox for each available system profile.
 * @param win The parent window.
 * @param bx The box to pack the checkboxes into.
 * @return A list of the created checkbox objects.
 */
static Eina_List *
_check_add(Evas_Object *win, Evas_Object *bx)
{
   App_Data *ad = evas_object_data_get(win, "ad");
   Evas_Object *bx2, *ck;
   Eina_List *l = NULL, *ll = NULL;
   const char *str;

   bx2 = elm_box_add(win);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, 0.0);
   elm_box_align_set(bx2, 0.0, 0.5);
   elm_box_horizontal_set(bx2, EINA_TRUE);
   elm_box_pack_end(bx, bx2);
   evas_object_show(bx2);

   EINA_LIST_FOREACH(ad->profiles, l, str)
     {
        ck = elm_check_add(win);
        elm_object_text_set(ck, str);
        evas_object_data_set(ck, "profile", eina_stringshare_add(str));
        evas_object_size_hint_weight_set(ck, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_box_pack_end(bx2, ck);
        evas_object_show(ck);

        ll = eina_list_append(ll, ck);
     }

   return ll;
}

/**
 * @brief Creates an inlined window.
 *
 * An inlined window is a window that is rendered into an image object in a
 * parent window, rather than being a separate top-level window. This function
 * demonstrates this feature.
 * @param parent The parent window.
 * @return The newly created inlined window object, or NULL on failure.
 */
static Evas_Object *
_inlined_add(Evas_Object *parent)
{
   Evas_Object *win, *bg, *bx, *lb;

   win = elm_win_add(parent, "inlined", ELM_WIN_INLINED_IMAGE);
   if (!win) return NULL;

   bg = elm_bg_add(win);
   elm_bg_color_set(bg, 110, 210, 120);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bg);
   evas_object_show(bg);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, 0.0);
   evas_object_show(bx);

   LOG("ELM_WIN_INLINED_IMAGE");
   elm_box_pack_end(bx, lb);

   LOG("Profile: <b>N/A</b>");
   elm_box_pack_end(bx, lb);
   evas_object_data_set(win, "lb", lb);

   evas_object_move(win, 10, 100);
   evas_object_resize(win, 150 * elm_config_scale_get(),
                           70  * elm_config_scale_get());
   evas_object_move(elm_win_inlined_image_object_get(win), 10, 100);
   evas_object_resize(elm_win_inlined_image_object_get(win),
                      150 * elm_config_scale_get(),
                      70   * elm_config_scale_get());

   evas_object_smart_callback_add(win, "profile,changed", _win_profile_changed_cb, NULL);
   evas_object_show(win);

   return win;
}

/**
 * @brief Creates a socket window for another process to plug into.
 *
 * This demonstrates inter-process window embedding by creating a window that
 * listens on a socket for a client (a plug) to connect. It also creates an
 * inlined window inside itself.
 * @param name The service name for the socket.
 * @return The socket window object, or NULL on failure.
 */
static Evas_Object *
_socket_add(const char *name)
{
   Evas_Object *win, *bg, *bx, *lb;

   win = elm_win_add(NULL, "socket image", ELM_WIN_SOCKET_IMAGE);
   if (!win) return NULL;

   if (elm_win_socket_listen(win, name, 0, EINA_FALSE))
     {
        elm_win_autodel_set(win, EINA_TRUE);

        bg = elm_bg_add(win);
        elm_bg_color_set(bg, 80, 110, 205);
        evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_win_resize_object_add(win, bg);
        evas_object_show(bg);

        bx = elm_box_add(win);
        evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, 0.0);
        evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, 0.0);
        evas_object_show(bx);

        LOG("ELM_WIN_SOCKET_IMAGE");
        elm_box_pack_end(bx, lb);

        LOG("Profile: <b>N/A</b>");
        elm_box_pack_end(bx, lb);
        evas_object_data_set(win, "lb", lb);

        _inlined_add(win);

        evas_object_move(win, 0, 0);
        evas_object_resize(win, 150 * elm_config_scale_get(),
                                200 * elm_config_scale_get());

        evas_object_smart_callback_add(win, "profile,changed", _win_profile_changed_cb, NULL);
        evas_object_show(win);
     }
   else
     {
        evas_object_del(win);
        win = NULL;
     }

   return win;
}

/**
 * @brief Creates a plug widget to connect to a socket window.
 *
 * This widget will render the contents of the socket window it connects to.
 * This demonstrates the client side of inter-process window embedding.
 * @param win The parent window.
 * @param bx The box to pack the plug into.
 * @param name The service name of the socket to connect to.
 * @return The plug object if connection is successful, otherwise NULL.
 */
static Evas_Object *
_plug_add(Evas_Object *win, Evas_Object *bx, const char *name)
{
   Evas_Object *plug, *ly;
   Eina_Bool res = EINA_FALSE;
   char buf[PATH_MAX];

   plug = elm_plug_add(win);
   if (plug) res = elm_plug_connect(plug, name, 0, EINA_FALSE);

   if (res)
     {
        ly = elm_layout_add(win);
        snprintf(buf, sizeof(buf), "%s/objects/test.edj", elm_app_data_dir_get());
        elm_layout_file_set(ly, buf, "win_config");
        evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_fill_set(ly, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_show(ly);

        evas_object_size_hint_weight_set(plug, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_box_pack_end(bx, ly);
        elm_object_part_content_set(ly, "swallow", plug);
        evas_object_show(plug);
     }
   else
     {
        if (plug) evas_object_del(plug);
        plug = NULL;
     }

   return plug;
}

#ifdef FRAME
# undef FRAME
#endif

#define FRAME(t)                                                                  \
   do {                                                                           \
        fr = elm_frame_add(bx);                                                   \
        elm_object_text_set(fr, t);                                               \
        evas_object_size_hint_weight_set(fr, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND); \
        evas_object_size_hint_fill_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);       \
        elm_box_pack_end(bx, fr);                                                 \
        evas_object_show(fr);                                                     \
        bx2 = elm_box_add(win);                                                   \
        evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0.0);             \
        evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, 0.0);                \
        elm_box_align_set(bx2, 0.0, 0.5);                                         \
        elm_object_content_set(fr, bx2);                                          \
        evas_object_show(bx2);                                                    \
   } while(0)

/**
 * @brief Main function for the Elementary configuration test.
 *
 * This function creates the main window and UI for testing profile and other
 * configuration options. It can be called recursively to create new windows
 * with specific profile settings.
 *
 * @param data If not NULL, it is a pointer to a Prof_Data struct which
 *        contains the profile settings for the new window. This is used
 *        when creating a new window from an existing test window.
 *        The Prof_Data passed would have its members set, for example:
 *        - `.profile` = "my_profile"
 *        - `.available_profiles` = {"p1", "p2", NULL, ...}
 *        - `.count` = 2
 * @param obj Unused when creating a new window via button click. It is the
 *        object that triggered the test (e.g., from a test launcher).
 * @param event_info Unused.
 */
void
test_config(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   App_Data *ad;
   Prof_Data *pd = (Prof_Data *)data;
   Evas_Object *win, *sc, *bx, *fr, *bx2, *lb, *bt;
   Ecore_Evas *ee;
   const char *siname = "_TestConfigSocketImage_";
   char buf[PATH_MAX];

   if (!(ad = calloc(1, sizeof(App_Data)))) return;

   win = elm_win_util_standard_add("config", "Configuration");
   elm_win_autodel_set(win, EINA_TRUE);
   evas_object_data_set(win, "ad", ad);
   ad->win = win;
   ad->profiles = elm_config_profile_list_get();

   sc = elm_scroller_add(win);
   elm_scroller_bounce_set(sc, EINA_FALSE, EINA_TRUE);
   evas_object_size_hint_weight_set(sc, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, sc);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(sc, bx);

   FRAME("Current window profile");
   ee = ecore_evas_ecore_evas_get(evas_object_evas_get(win));
   snprintf(buf, sizeof(buf),
            "Virtual desktop window profile support: <b>%s</b>",
            ecore_evas_window_profile_supported_get(ee) ? "Yes" : "No");
   LOG(buf);
   elm_box_pack_end(bx2, lb);

   LOG("Profile: <b>N/A</b><br/>Available profiles:");
   elm_box_pack_end(bx2, lb);
   evas_object_data_set(win, "lb", lb);

   LOG("<br/>Window profile");
   elm_box_pack_end(bx2, lb);
   ad->curr.rdg = _radio_add(win, bx2);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Set");
   evas_object_smart_callback_add(bt, "clicked", _bt_profile_set, win);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   LOG("Window available profiles");
   elm_box_pack_end(bx2, lb);
   ad->curr.cks = _check_add(win, bx2);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Set");
   evas_object_smart_callback_add(bt, "clicked", _bt_available_profiles_set, win);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   FRAME("Socket");
   if (_socket_add(siname))
     {
        LOG("Starting socket image.");
        elm_box_pack_end(bx2, lb);
     }
   else
     {
        LOG("Failed to create socket.<br/>"
            "Please check whether another test configuration window is<br/>"
            "already running and providing socket image.");
        elm_box_pack_end(bx2, lb);
     }

   FRAME("Plug");
   if (!_plug_add(win, bx2, siname))
     {
        LOG("Failed to connect to server.");
        elm_box_pack_end(bx2, lb);
     }

   FRAME("Create new window with profile");
   LOG("Window profile");
   elm_box_pack_end(bx2, lb);
   ad->new.rdg = _radio_add(win, bx2);

   LOG("Window available profiles");
   elm_box_pack_end(bx2, lb);
   ad->new.cks = _check_add(win, bx2);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Create");
   evas_object_smart_callback_add(bt, "clicked", _bt_win_add, win);
   elm_box_pack_end(bx2, bt);
   evas_object_show(bt);

   evas_object_smart_callback_add(win, "profile,changed", _win_profile_changed_cb, NULL);
   evas_object_smart_callback_add(win, "delete,request", _win_del_cb, NULL);

   if (pd && !obj) //obj is NULL when called by _bt_win_add but not when user clicks this test
     {
        if (pd->available_profiles[0])
          elm_win_available_profiles_set(win,
                                         pd->available_profiles,
                                         pd->count);
        if (pd->profile)
          elm_win_profile_set(win, pd->profile);

        _profile_update(win);
     }

   evas_object_show(bx);
   evas_object_show(sc);

   evas_object_resize(win, 400 * elm_config_scale_get(),
                           500 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Callback to push the next page in the font overlay test.
 *
 * This function is called when the "Next" button is clicked. It pushes a new
 * page onto the naviframe which contains a textblock to demonstrate the
 * font overlay effect.
 *
 * @param data The naviframe object.
 * @param obj Unused.
 * @param event_info Unused.
 */
static void
_font_overlay_page_next(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *layout;
   Evas_Object *nf = (Evas_Object *)data;
   char buf[255];

   layout = elm_layout_add(nf);
   snprintf(buf, sizeof(buf), "%s/objects/test.edj", elm_app_data_dir_get());
   elm_layout_file_set(layout, buf, "font_overlay_layout");
   elm_layout_text_set(layout, "elm.text", "TEXTBLOCK part of test_class");
   evas_object_show(layout);

   elm_naviframe_item_push(nf, "Font Overlay", NULL, NULL, layout, NULL);
}

/**
 * @brief Callback to apply the font overlay settings.
 *
 * Triggered by the "Apply Font Overlay" button. It retrieves the font name
 * and size from the entry fields, sets them as a font overlay configuration,
 * and applies the changes globally.
 *
 * @param data The naviframe object, used to access the entry fields.
 * @param obj Unused.
 * @param event_info Unused.
 */
static void
_apply_font_overlay_btn_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *nf = (Evas_Object *)data;
   Evas_Object *entry;
   const char *font;
   const char *font_size_temp;
   int font_size;

   entry = (Evas_Object *)evas_object_data_get(nf, "font_entry");
   font = elm_entry_entry_get(entry);
   entry = (Evas_Object *)evas_object_data_get(nf, "font_size_entry");
   font_size_temp = elm_entry_entry_get(entry);
   font_size = atoi(font_size_temp);

   printf("Font overlay set: Font [%s], FontSize [%d]\n", font, font_size);
   elm_config_font_overlay_set("font_overlay_test", font, font_size);
   elm_config_font_overlay_apply();
}

/**
 * @brief Main function for the font overlay configuration test.
 *
 * This function sets up a window with UI elements to test the font overlay
 * feature of Elementary. Users can input a font name and size, apply it,
 * and navigate to another page to see its effect.
 *
 * @param data Unused.
 * @param obj Unused.
 * @param event_info Unused.
 */
void
test_config_font_overlay(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *nf, *box, *horizon_box, *label, *btn, *entry;
   Elm_Object_Item *it;

   win = elm_win_util_standard_add("naviframe", "Naviframe");
   elm_win_focus_highlight_enabled_set(win, EINA_TRUE);
   elm_win_autodel_set(win, EINA_TRUE);

   nf = elm_naviframe_add(win);
   evas_object_size_hint_weight_set(nf, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, nf);
   evas_object_show(nf);

   box = elm_box_add(nf);
   evas_object_show(box);

   horizon_box = elm_box_add(box);
   elm_box_horizontal_set(horizon_box, EINA_TRUE);
   evas_object_size_hint_align_set(horizon_box, EVAS_HINT_FILL, -1);
   evas_object_size_hint_weight_set(horizon_box, EVAS_HINT_EXPAND, -1);
   elm_box_pack_end(box, horizon_box);
   evas_object_show(horizon_box);

   label = elm_label_add(horizon_box);
   elm_object_text_set(label, "Font:");
   elm_box_pack_end(horizon_box, label);
   evas_object_show(label);

   entry = elm_entry_add(horizon_box);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, -1);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, -1);
   elm_object_part_text_set(entry, "elm.guide", "Input Font");
   elm_box_pack_end(horizon_box, entry);
   evas_object_show(entry);
   evas_object_data_set(nf, "font_entry", entry);

   horizon_box = elm_box_add(box);
   elm_box_horizontal_set(horizon_box, EINA_TRUE);
   evas_object_size_hint_align_set(horizon_box, EVAS_HINT_FILL, -1);
   evas_object_size_hint_weight_set(horizon_box, EVAS_HINT_EXPAND, -1);
   elm_box_pack_end(box, horizon_box);
   evas_object_show(horizon_box);

   label = elm_label_add(horizon_box);
   elm_object_text_set(label, "Size:");
   elm_box_pack_end(horizon_box, label);
   evas_object_show(label);

   entry = elm_entry_add(horizon_box);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, -1);
   evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, -1);
   elm_object_part_text_set(entry, "elm.guide", "Input Font Size");
   elm_box_pack_end(horizon_box, entry);
   evas_object_show(entry);
   evas_object_data_set(nf, "font_size_entry", entry);

   btn = elm_button_add(box);
   elm_object_text_set(btn, "Apply Font Overlay");
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, -1);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, -1);
   evas_object_smart_callback_add(btn, "clicked", _apply_font_overlay_btn_clicked_cb, nf);
   elm_box_pack_end(box, btn);
   evas_object_show(btn);

   btn = elm_button_add(nf);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(btn, "clicked", _font_overlay_page_next, nf);
   elm_object_text_set(btn, "Next");
   evas_object_show(btn);

   it = elm_naviframe_item_push(nf, "Font Overlay", NULL, btn, box, NULL);
   evas_object_data_set(nf, "page1", it);

   evas_object_resize(win, 400 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
   evas_object_show(win);
}
