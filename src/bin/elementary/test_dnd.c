#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#include <Elementary.h>

#define FILESEP "file://"
#define FILESEP_LEN sizeof(FILESEP) - 1

static const char *img[9] =
{
   "panel_01.jpg",
   "plant_01.jpg",
   "rock_01.jpg",
   "rock_02.jpg",
   "sky_01.jpg",
   "sky_02.jpg",
   "sky_03.jpg",
   "sky_04.jpg",
   "wood_01.jpg",
};

struct _anim_icon_st
{
   int start_x;
   int start_y;
   Evas_Object *o;
};
typedef struct _anim_icon_st anim_icon_st;

struct _drag_anim_st
{
   Evas_Object *icwin;
   Evas *e;
   Evas_Coord mdx;     /* Mouse-down x */
   Evas_Coord mdy;     /* Mouse-down y */
   Eina_List *icons;   /* List of icons to animate (anim_icon_st) */
   Ecore_Timer *tm;
   Ecore_Animator *ea;
   Evas_Object *gl;
};
typedef struct _drag_anim_st drag_anim_st;

#define DRAG_TIMEOUT 0.3
#define ANIM_TIME 0.5

static Eina_Bool _5s_cancel = EINA_FALSE;
static Ecore_Timer *_5s_timeout = NULL;

/**
 * @brief Compare two item pointers.
 *
 * This is used for searching in an Eina_List.
 *
 * @param d1 First pointer.
 * @param d2 Second pointer.
 * @return The difference between the two pointers.
 */
static int
_item_ptr_cmp(const void *d1, const void *d2)
{
   return ((const char *) d1 - (const char *) d2);
}

static Elm_Genlist_Item_Class *itc1;
static Elm_Gengrid_Item_Class *gic;

/**
 * @brief Build a drag data string from a list of items.
 *
 * The format of the returned string is a newline-separated list of file URIs.
 * e.g., "file:///path/to/image1.jpg\nfile:///path/to/image2.jpg"
 *
 * @param items Pointer to a list of Elm_Object_Item pointers.
 * @return A newly allocated string with the drag data, or NULL on failure.
 *         The caller is responsible for freeing the returned string.
 */
static const char *
_drag_data_build(Eina_List **items)
{
   const char *drag_data = NULL;
   if (*items)
     {
        Eina_Strbuf *str;
        Eina_List *l;
        Elm_Object_Item *it;
        const char *t;
        int i = 0;

        str = eina_strbuf_new();
        if (!str) return NULL;

        /* drag data in form: file://URI1\nfile://URI2 */
        EINA_LIST_FOREACH(*items, l, it)
          {
             t = (char *)elm_object_item_data_get(it);
             if (t)
               {
                  if (i > 0)
                    eina_strbuf_append(str, "\n");
                  eina_strbuf_append(str, FILESEP);
                  eina_strbuf_append(str, t);
                  i++;
               }
          }
        drag_data = eina_strbuf_string_steal(str);
        eina_strbuf_free(str);
     }
   return drag_data;
}

/**
 * @brief Extract a single file URI from a drag data string.
 *
 * This function parses a string of newline-separated URIs (as created by
 * _drag_data_build) and returns one URI at a time. It modifies the input
 * string by replacing newlines with null terminators.
 *
 * @param drag_data A pointer to a character pointer. On each call, this
 *                  pointer is advanced to the next URI in the string.
 * @return A pointer to the current URI within the original string, or NULL
 *         if no more URIs are found. The returned pointer is not a new
 *         allocation and should not be freed.
 */
static char *
_drag_data_extract(char **drag_data)
{
   char *uri = NULL;
   if (!drag_data)
     return uri;

   char *p = *drag_data;
   if (!p)
     return uri;
   char *s = strstr(p, FILESEP);
   if (s)
     p += FILESEP_LEN;
   s = strchr(p, '\n');
   uri = p;
   if (s)
     {
        if (s - p > 0)
          {
             char *s1 = s - 1;
             if (s1[0] == '\r')
               s1[0] = '\0';
             else
               {
                  char *s2 = s + 1;
                  if (s2[0] == '\r')
                    {
                       s[0] = '\0';
                       s++;
                    }
                  else
                    s[0] = '\0';
               }
          }
        else
          s[0] = '\0';
        s++;
     }
   else
     p = NULL;
   *drag_data = s;

   return uri;
}

/**
 * @brief Get the text for a genlist or gengrid item.
 *
 * @param data The item data (a string with the file path).
 * @param obj The genlist/gengrid object.
 * @param part The part name.
 * @return A duplicated string of the item data. The caller (Elementary) will
 *         free it.
 */
static char *
gl_text_get(void *data, Evas_Object *obj EINA_UNUSED, const char *part EINA_UNUSED)
{
   return strdup(data);
}

/**
 * @brief Get the content (icon) for a genlist or gengrid item.
 *
 * @param data The item data (a string with the file path for the icon).
 * @param obj The genlist/gengrid object.
 * @param part The part name to get content for. Expected to be
 *             "elm.swallow.icon".
 * @return A new icon object, or NULL if the part is not "elm.swallow.icon".
 */
static Evas_Object *
gl_content_get(void *data, Evas_Object *obj, const char *part)
{
   if (!strcmp(part, "elm.swallow.icon"))
     {
        Evas_Object *icon = elm_icon_add(obj);
        elm_image_file_set(icon, data, NULL);
        evas_object_size_hint_aspect_set(icon, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        evas_object_show(icon);
        return icon;
     }
   return NULL;
}

/**
 * @brief Callback for item deletion in gengrid.
 *
 * This is called when an item is deleted from the gengrid, and it frees the
 * associated stringshared data.
 *
 * @param data The item data (a stringshared file path).
 * @param obj The gengrid object.
 */
static void
gl_del_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   eina_stringshare_del(data);
}

/**
 * @brief Callback for window deletion request.
 *
 * Cleans up drag-and-drop containers and item classes associated with the
 * widget passed in @p data.
 *
 * @param data The widget (genlist or gengrid) to clean up.
 * @param obj The window object.
 * @param event_info Event-specific information.
 */
static void
_win_del(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   printf("<%s> <%d> will del <%p>\n", __func__, __LINE__, data);
   elm_drop_item_container_del(data);
   elm_drag_item_container_del(data);

   if (gic) elm_gengrid_item_class_free(gic);
   gic = NULL;
   if (itc1) elm_genlist_item_class_free(itc1);
   itc1 = NULL;
}

/**
 * @brief Get the genlist item at specific coordinates.
 *
 * @param obj The genlist object.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param xposret Unused for vertical genlist.
 * @param yposret A pointer to store the relative position within the item
 *                (-1 for top, 0 for middle, 1 for bottom).
 * @return The Elm_Object_Item at the given coordinates, or NULL.
 */
static Elm_Object_Item *
_gl_item_getcb(Evas_Object *obj, Evas_Coord x, Evas_Coord y, int *xposret EINA_UNUSED, int *yposret)
{  /* This function returns pointer to item under (x,y) coords */
   printf("<%s> <%d> obj=<%p>\n", __func__, __LINE__, obj);
   Elm_Object_Item *gli;
   gli = elm_genlist_at_xy_item_get(obj, x, y, yposret);
   if (gli)
     printf("over <%s>, gli=<%p> yposret %i\n",
           (char *)elm_object_item_data_get(gli), gli, *yposret);
   else
     printf("over none, yposret %i\n", *yposret);
   return gli;
}

/**
 * @brief Get the gengrid item at specific coordinates.
 *
 * @param obj The gengrid object.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param xposret A pointer to store the relative horizontal position within
 *                the item (-1 for left, 0 for center, 1 for right).
 * @param yposret A pointer to store the relative vertical position within
 *                the item (-1 for top, 0 for center, 1 for bottom).
 * @return The Elm_Object_Item at the given coordinates, or NULL.
 */
static Elm_Object_Item *
_grid_item_getcb(Evas_Object *obj, Evas_Coord x, Evas_Coord y, int *xposret, int *yposret)
{  /* This function returns pointer to item under (x,y) coords */
   printf("<%s> <%d> obj=<%p>\n", __func__, __LINE__, obj);
   Elm_Object_Item *item;
   item = elm_gengrid_at_xy_item_get(obj, x, y, xposret, yposret);
   if (item)
     printf("over <%s>, item=<%p> xposret %i yposret %i\n",
           (char *)elm_object_item_data_get(item), item, *xposret, *yposret);
   else
     printf("over none, xposret %i yposret %i\n", *xposret, *yposret);
   return item;
}

/**
 * @brief Callback for drag position updates over a genlist.
 *
 * This function is called when the drag cursor moves over a droppable item.
 * It's mainly used for debugging purposes here.
 *
 * @param data User data.
 * @param obj The genlist object.
 * @param it The item under the cursor.
 * @param x The current x-coordinate of the cursor.
 * @param y The current y-coordinate of the cursor.
 * @param xposret The relative horizontal position within the item.
 * @param yposret The relative vertical position within the item.
 * @param action The current drag and drop action.
 */
static void
_gl_poscb(void *data EINA_UNUSED, Evas_Object *obj, Elm_Object_Item *it, Evas_Coord x, Evas_Coord y, int xposret, int yposret, Elm_Xdnd_Action action EINA_UNUSED)
{
   printf("<%s> <%d> obj: %p, item: %p <%s>, x y: %d %d, posret: %d %d\n",
          __func__, __LINE__, obj, it, elm_object_item_text_get(it),
          x, y, xposret, yposret);
}

/**
 * @brief Callback for drop events on a genlist.
 *
 * This function is called when data is dropped onto the genlist. It extracts
 * the file URIs from the drop data and inserts new items into the genlist.
 * The insertion position depends on where the drop occurred on an existing
 * item (before, or after).
 *
 * @param data User data.
 * @param obj The genlist object.
 * @param it The item that was dropped on.
 * @param ev The selection data containing the dropped content.
 * @param xposret The relative horizontal position of the drop.
 * @param yposret The relative vertical position of the drop.
 * @return EINA_TRUE on successful handling of the drop, EINA_FALSE otherwise.
 */
static Eina_Bool
_gl_dropcb(void *data EINA_UNUSED, Evas_Object *obj, Elm_Object_Item *it, Elm_Selection_Data *ev, int xposret EINA_UNUSED, int yposret)
{  /* This function is called when data is dropped on the genlist */
   printf("<%s> <%d> str=<%s>\n", __func__, __LINE__, (char *) ev->data);
   if (!ev->data)
     return EINA_FALSE;
   if (ev->len <= 0)
     return EINA_FALSE;

   char *dd = eina_strndup(ev->data, ev->len);
   if (!dd) return EINA_FALSE;
   char *p = dd;

   char *s =  _drag_data_extract(&p);
   while (s)
     {
        switch(yposret)
          {
           case -1:  /* Dropped on top-part of the it item */
                {
                   elm_genlist_item_insert_before(obj,
                         itc1, strdup(s), NULL, it,
                         ELM_GENLIST_ITEM_NONE,
                         NULL, NULL);
                   break;
                }
           case  0:  /* Dropped on center of the it item      */
           case  1:  /* Dropped on botton-part of the it item */
                {
                   if (!it) it = elm_genlist_last_item_get(obj);
                   if (it) it = elm_genlist_item_insert_after(obj,
                                      itc1, strdup(s), NULL, it,
                                      ELM_GENLIST_ITEM_NONE,
                                                              NULL, NULL);
                   else
                     it = elm_genlist_item_append(obj,
                                itc1, strdup(s), NULL,
                                ELM_GENLIST_ITEM_NONE,
                                NULL, NULL);
                   break;
                }
           default:
                {
                   free(dd);
                   return EINA_FALSE;
                }
          }

        s = _drag_data_extract(&p);
     }
   free(dd);

   return EINA_TRUE;
}

/**
 * @brief Callback for drop events on a gengrid.
 *
 * Handles data dropped onto the gengrid. It parses the drop data and inserts
 * new items into the grid.
 *
 * @param data User data.
 * @param obj The gengrid object.
 * @param it The item that was dropped on.
 * @param ev The selection data.
 * @param xposret Relative horizontal drop position.
 * @param yposret Relative vertical drop position.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool
_grid_dropcb(void *data EINA_UNUSED, Evas_Object *obj, Elm_Object_Item *it, Elm_Selection_Data *ev, int xposret EINA_UNUSED, int yposret EINA_UNUSED)
{  /* This function is called when data is dropped on the genlist */
   printf("<%s> <%d> str=<%s>\n", __func__, __LINE__, (char *) ev->data);
   if (!ev->data)
     return EINA_FALSE;
   if (ev->len <= 0)
     return EINA_FALSE;

   char *dd = eina_strndup(ev->data, ev->len);
   if (!dd) return EINA_FALSE;
   char *p = dd;
   char *s = _drag_data_extract(&p);
   while(s)
     {
        if (it)
          it = elm_gengrid_item_insert_after(obj, gic, eina_stringshare_add(s), it, NULL, NULL);
        else
          it = elm_gengrid_item_append(obj, gic, eina_stringshare_add(s), NULL, NULL);
        s = _drag_data_extract(&p);
     }
   free(dd);

   return EINA_TRUE;
}

static void _gl_obj_mouse_move( void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _gl_obj_mouse_up( void *data, Evas *e, Evas_Object *obj, void *event_info);

/**
 * @brief Free resources associated with a drag animation state.
 *
 * Stops any ongoing timers or animators and frees the memory used by the
 * drag_anim_st structure and its associated icons.
 *
 * @param anim_st The drag animation state to free.
 */
static void
anim_st_free(drag_anim_st *anim_st)
{  /* Stops and free mem of ongoing animation */
   printf("<%s> <%d>\n", __func__, __LINE__);
   if (anim_st)
     {
        evas_object_event_callback_del_full
           (anim_st->gl, EVAS_CALLBACK_MOUSE_MOVE, _gl_obj_mouse_move, anim_st);
        evas_object_event_callback_del_full
           (anim_st->gl, EVAS_CALLBACK_MOUSE_UP, _gl_obj_mouse_up, anim_st);
        if (anim_st->tm)
          {
             ecore_timer_del(anim_st->tm);
             anim_st->tm = NULL;
          }

        if (anim_st->ea)
          {
             ecore_animator_del(anim_st->ea);
             anim_st->ea = NULL;
          }

        anim_icon_st *st;

        EINA_LIST_FREE(anim_st->icons, st)
          {
             evas_object_hide(st->o);
             evas_object_del(st->o);
             free(st);
          }

        free(anim_st);
     }
}

/**
 * @brief Ecore animator callback to play the drag animation.
 *
 * This function is called for each frame of the drag start animation.
 * It moves the animated icons towards the mouse cursor. The animation is
 * cancelled and resources are freed when it completes or is interrupted.
 *
 * @param data The drag_anim_st data structure.
 * @param pos The position in the animation timeline (0.0 to 1.0).
 * @return ECORE_CALLBACK_RENEW to continue animation, ECORE_CALLBACK_CANCEL to stop.
 */
static Eina_Bool
_drag_anim_play(void *data, double pos)
{  /* Impl of the animation of icons, called on frame time */
   drag_anim_st *anim_st = data;
   printf("<%s> <%d>\n", __func__, __LINE__);
   Eina_List *l;
   anim_icon_st *st;

   if (anim_st)
     {
        if (pos > 0.99)
          {
             anim_st->ea = NULL;  /* Avoid deleting on mouse up */

             EINA_LIST_FOREACH(anim_st->icons, l, st)
                evas_object_hide(st->o);   /* Hide animated icons */
             anim_st_free(anim_st);
             return ECORE_CALLBACK_CANCEL;
          }

        EINA_LIST_FOREACH(anim_st->icons, l, st)
          {
             int x, y, w, h;
             Evas_Coord xm, ym;
             evas_object_geometry_get(st->o, NULL, NULL, &w, &h);
             evas_pointer_canvas_xy_get(anim_st->e, &xm, &ym);
             x = st->start_x + (pos * (xm - (st->start_x + (w/2))));
             y = st->start_y + (pos * (ym - (st->start_y + (h/2))));
             evas_object_move(st->o, x, y);
          }

        return ECORE_CALLBACK_RENEW;
     }

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Timer callback to start the drag animation.
 *
 * This is called after a short delay (DRAG_TIMEOUT) when the user holds
 * the mouse button down on an item. It creates icons for the selected items
 * and starts the ecore animator to move them.
 *
 * @param data The drag_anim_st data structure.
 * @return ECORE_CALLBACK_CANCEL to stop the timer.
 */
static Eina_Bool
_gl_anim_start(void *data)
{  /* Start icons animation before actually drag-starts */
   drag_anim_st *anim_st = data;
   printf("<%s> <%d>\n", __func__, __LINE__);
   int yposret = 0;

   Eina_List *l;
   Eina_List *items = eina_list_clone(elm_genlist_selected_items_get(anim_st->gl));
   Elm_Object_Item *gli = elm_genlist_at_xy_item_get(anim_st->gl,
         anim_st->mdx, anim_st->mdy, &yposret);
   if (gli)
     {  /* Add the item mouse is over to the list if NOT seleced */
        void *p = eina_list_search_unsorted(items, _item_ptr_cmp, gli);
        if (!p)
          items = eina_list_append(items, gli);
     }

   EINA_LIST_FOREACH(items, l, gli)
     {  /* Now add icons to animation window */
        Evas_Object *o = elm_object_item_part_content_get(gli,
              "elm.swallow.icon");

        if (o)
          {
             int w, h;
             const char *f;
             const char *g;
             anim_icon_st *st = calloc(1, sizeof(*st));
             elm_image_file_get(o, &f, &g);
             Evas_Object *ic = elm_icon_add(anim_st->gl);
             elm_image_file_set(ic, f, g);
             evas_object_geometry_get(o, &st->start_x, &st->start_y, &w, &h);
             evas_object_size_hint_align_set(ic,
                   EVAS_HINT_FILL, EVAS_HINT_FILL);
             evas_object_size_hint_weight_set(ic,
                   EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

             evas_object_move(ic, st->start_x, st->start_y);
             evas_object_resize(ic, w, h);
             evas_object_show(ic);

             st->o = ic;
             anim_st->icons = eina_list_append(anim_st->icons, st);
          }
     }

   eina_list_free(items);

   anim_st->tm = NULL;
   anim_st->ea = ecore_animator_timeline_add(DRAG_TIMEOUT,
         _drag_anim_play, anim_st);

   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Mouse up event callback on the genlist.
 *
 * If the user releases the mouse button before the drag operation officially
 * starts, this callback cancels the pending drag animation.
 *
 * @param data The drag_anim_st data structure.
 * @param e The Evas canvas.
 * @param obj The genlist object.
 * @param event_info The mouse up event info.
 */
static void
_gl_obj_mouse_up(
   void *data,
   Evas *e EINA_UNUSED,
   Evas_Object *obj EINA_UNUSED,
   void *event_info EINA_UNUSED)
{  /* Cancel any drag waiting to start on timeout */
   drag_anim_st *anim_st = data;
   anim_st_free(anim_st);
}

/**
 * @brief Mouse move event callback on the genlist.
 *
 * If the mouse moves while the event is on hold (i.e., before the drag
 * timeout), this cancels the drag animation. This prevents the animation
 * from starting if the user clicks and drags immediately.
 *
 * @param data The drag_anim_st data structure.
 * @param e The Evas canvas.
 * @param obj The genlist object.
 * @param event_info The mouse move event info.
 */
static void
_gl_obj_mouse_move(
   void *data,
   Evas *e EINA_UNUSED,
   Evas_Object *obj EINA_UNUSED,
   void *event_info)
{  /* Cancel any drag waiting to start on timeout */

   if (((Evas_Event_Mouse_Move *)event_info)->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        drag_anim_st *anim_st = data;
        anim_st_free(anim_st);
     }
}

/**
 * @brief Mouse down event callback on the genlist for custom animation.
 *
 * This function initiates the custom drag animation. It sets a timer that,
 * after a short delay, will start the animation of icons moving from their
 * original positions.
 *
 * @param data The genlist object.
 * @param e The Evas canvas.
 * @param obj The object that received the event.
 * @param event_info The mouse down event info.
 */
static void
_gl_obj_mouse_down(
   void *data,
   Evas *e EINA_UNUSED,
   Evas_Object *obj EINA_UNUSED,
   void *event_info)
{  /* Launch a timer to start drag animation */
   Evas_Event_Mouse_Down *ev = event_info;
   drag_anim_st *anim_st = calloc(1, sizeof(*anim_st));
   anim_st->e = e;
   anim_st->mdx = ev->canvas.x;
   anim_st->mdy = ev->canvas.y;
   anim_st->gl = data;
   anim_st->tm = ecore_timer_add(DRAG_TIMEOUT, _gl_anim_start, anim_st);
   evas_object_event_callback_add(data, EVAS_CALLBACK_MOUSE_UP,
         _gl_obj_mouse_up, anim_st);
   evas_object_event_callback_add(data, EVAS_CALLBACK_MOUSE_MOVE,
         _gl_obj_mouse_move, anim_st);
}
/* END   - Handling drag start animation */

/**
 * @brief Callback for when a drag operation has finished.
 *
 * This function is called after a drop has occurred (or was cancelled).
 * If the drop was accepted by the target (@p doaccept is EINA_TRUE),
 * it deletes the dragged items from the source genlist/gengrid.
 * It also handles cleanup of the 5-second cancel timer and frees the list
 * of dragged items.
 *
 * @param data The list of dragged items (Eina_List of Elm_Object_Item*).
 * @param obj The source object.
 * @param doaccept EINA_TRUE if the drop was accepted, EINA_FALSE otherwise.
 */
static void
_gl_dragdone(void *data, Evas_Object *obj EINA_UNUSED, Eina_Bool doaccept)
{
   printf("<%s> <%d> data=<%p> doaccept=<%d>\n",
         __func__, __LINE__, data, doaccept);

   Elm_Object_Item *it;
   Eina_List *l;

   if (_5s_cancel)
     {
        ecore_timer_del(_5s_timeout);
        _5s_timeout = NULL;
     }

   if (doaccept)
     {  /* Remove items dragged out (accepted by target) */
        EINA_LIST_FOREACH(data, l, it)
           elm_object_item_del(it);
     }

   eina_list_free(data);
   return;
}

/**
 * @brief Timer callback to cancel a drag operation after a timeout.
 *
 * This function is part of a feature to test drag cancellation. If a drag
 * operation is ongoing for 5 seconds, this timer fires and cancels it.
 *
 * @param data The object on which drag is active.
 * @return ECORE_CALLBACK_CANCEL to stop the timer.
 */
static Eina_Bool
_5s_timeout_gone(void *data)
{
   printf("Cancel DnD\n");
   elm_drag_cancel(data);
   _5s_timeout = NULL;
   return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Create an icon for the drag operation.
 *
 * This callback creates the visual representation (an icon) that is dragged
 * around by the user. It also provides offsets to position the icon relative
 * to the cursor.
 *
 * @param data The item being dragged (Elm_Object_Item*).
 * @param win The window where the icon should be created.
 * @param xoff Pointer to store the horizontal offset of the icon from the cursor.
 * @param yoff Pointer to store the vertical offset of the icon from the cursor.
 * @return The newly created icon object.
 */
static Evas_Object *
_gl_createicon(void *data, Evas_Object *win, Evas_Coord *xoff, Evas_Coord *yoff)
{
   printf("<%s> <%d>\n", __func__, __LINE__);
   Evas_Object *icon = NULL;
   Evas_Object *o = elm_object_item_part_content_get(data, "elm.swallow.icon");

   if (o)
     {
        int xm, ym, w = 30, h = 30;
        const char *f;
        const char *g;
        elm_image_file_get(o, &f, &g);
        evas_pointer_canvas_xy_get(evas_object_evas_get(o), &xm, &ym);
        if (xoff) *xoff = xm - (w/2);
        if (yoff) *yoff = ym - (h/2);
        icon = elm_icon_add(win);
        elm_image_file_set(icon, f, g);
        evas_object_size_hint_align_set(icon,
              EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_weight_set(icon,
              EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        if (xoff && yoff) evas_object_move(icon, *xoff, *yoff);
        evas_object_resize(icon, w, h);
     }

   return icon;
}

/**
 * @brief Get a list of icons for the default drag animation.
 *
 * This function collects icons from all selected items in the genlist, plus the
 * item currently under the mouse pointer. These icons are then used by the
 * default Elementary drag animation.
 *
 * @param data The genlist object.
 * @return A list (Eina_List) of newly created Evas_Object icons.
 */
static Eina_List *
_gl_icons_get(void *data)
{  /* Start icons animation before actually drag-starts */
   printf("<%s> <%d>\n", __func__, __LINE__);
   int yposret = 0;

   Eina_List *l;

   Eina_List *icons = NULL;

   Evas_Coord xm, ym;
   evas_pointer_canvas_xy_get(evas_object_evas_get(data), &xm, &ym);
   Eina_List *items = eina_list_clone(elm_genlist_selected_items_get(data));
   Elm_Object_Item *gli = elm_genlist_at_xy_item_get(data,
         xm, ym, &yposret);
   if (gli)
     {  /* Add the item mouse is over to the list if NOT seleced */
        void *p = eina_list_search_unsorted(items, _item_ptr_cmp, gli);
        if (!p)
          items = eina_list_append(items, gli);
     }

   EINA_LIST_FOREACH(items, l, gli)
     {  /* Now add icons to animation window */
        Evas_Object *o = elm_object_item_part_content_get(gli,
              "elm.swallow.icon");

        if (o)
          {
             int x, y, w, h;
             const char *f, *g;
             elm_image_file_get(o, &f, &g);
             Evas_Object *ic = elm_icon_add(data);
             elm_image_file_set(ic, f, g);
             evas_object_geometry_get(o, &x, &y, &w, &h);
             evas_object_size_hint_align_set(ic,
                   EVAS_HINT_FILL, EVAS_HINT_FILL);
             evas_object_size_hint_weight_set(ic,
                   EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

             evas_object_move(ic, x, y);
             evas_object_resize(ic, w, h);
             evas_object_show(ic);

             icons =  eina_list_append(icons, ic);
          }
     }

   eina_list_free(items);
   return icons;
}

/**
 * @brief Get the drag data from a genlist.
 *
 * This function collects all selected items (plus the one under the cursor)
 * and builds a drag data string from them using _drag_data_build().
 *
 * @param obj The genlist object.
 * @param it The item under the cursor when the drag started.
 * @param[out] items A pointer to an Eina_List* which will be populated with
 *                   the list of dragged items. This list is later used in
 *                   _gl_dragdone to delete items if the drop is accepted.
 * @return A string containing the drag data, which should be freed by the caller.
 */
static const char *
_gl_get_drag_data(Evas_Object *obj, Elm_Object_Item *it, Eina_List **items)
{  /* Construct a string of dragged info, user frees returned string */
   const char *drag_data = NULL;
   printf("<%s> <%d>\n", __func__, __LINE__);

   *items = eina_list_clone(elm_genlist_selected_items_get(obj));
   if (it)
     {  /* Add the item mouse is over to the list if NOT seleced */
        void *p = eina_list_search_unsorted(*items, _item_ptr_cmp, it);
        if (!p)
          *items = eina_list_append(*items, it);
     }
   drag_data = _drag_data_build(items);
   printf("<%s> <%d> Sending <%s>\n", __func__, __LINE__, drag_data);

   return drag_data;
}

/**
 * @brief Get the drag data from a gengrid.
 *
 * Similar to _gl_get_drag_data(), but for a gengrid widget.
 *
 * @param obj The gengrid object.
 * @param it The item under the cursor.
 * @param[out] items A pointer to an Eina_List* to be populated with dragged items.
 * @return A string with drag data, to be freed by the caller.
 */
static const char *
_grid_get_drag_data(Evas_Object *obj, Elm_Object_Item *it, Eina_List **items)
{  /* Construct a string of dragged info, user frees returned string */
   const char *drag_data = NULL;
   printf("<%s> <%d>\n", __func__, __LINE__);

   *items = eina_list_clone(elm_gengrid_selected_items_get(obj));
   if (it)
     {  /* Add the item mouse is over to the list if NOT seleced */
        void *p = eina_list_search_unsorted(*items, _item_ptr_cmp, it);
        if (!p)
          *items = eina_list_append(*items, it);
     }
   drag_data = _drag_data_build(items);
   printf("<%s> <%d> Sending <%s>\n", __func__, __LINE__, drag_data);

   return drag_data;
}

/**
 * @brief Callback to get drag data for a genlist with default animation.
 *
 * This function is called when a drag is initiated. It populates the
 * Elm_Drag_User_Info structure with all the necessary information for the
 * drag-and-drop operation to proceed, including data, callbacks, and a list of
 * icons for the default animation.
 *
 * @param obj The genlist object.
 * @param it The item under the cursor.
 * @param[out] info The structure to fill with drag information.
 * @return EINA_TRUE if drag should start, EINA_FALSE to abort.
 */
static Eina_Bool
_gl_dnd_default_anim_data_getcb(Evas_Object *obj,  /* The genlist object */
      Elm_Object_Item *it,
      Elm_Drag_User_Info *info)
{  /* This called before starting to drag, mouse-down was on it */
   info->format = ELM_SEL_FORMAT_TARGETS;
   info->createicon = _gl_createicon;
   info->createdata = it;
   info->icons = _gl_icons_get(obj);
   info->dragdone = _gl_dragdone;

   /* Now, collect data to send for drop from ALL selected items */
   /* Save list pointer to remove items after drop and free list on done */
   info->data = _gl_get_drag_data(obj, it, (Eina_List **) &info->donecbdata);
   printf("%s - data = %s\n", __func__, info->data);
   info->acceptdata = info->donecbdata;

   if (info->data)
     return EINA_TRUE;
   else
     return EINA_FALSE;
}

/**
 * @brief Callback to get drag data for a genlist with custom user animation.
 *
 * This is similar to _gl_dnd_default_anim_data_getcb(), but it does not
 * provide a list of icons. The animation is handled separately by custom
 * mouse event callbacks.
 *
 * @param obj The genlist object.
 * @param it The item under the cursor.
 * @param[out] info The structure to fill with drag information.
 * @return EINA_TRUE if drag should start, EINA_FALSE to abort.
 */
static Eina_Bool
_gl_data_getcb(Evas_Object *obj,  /* The genlist object */
      Elm_Object_Item *it,
      Elm_Drag_User_Info *info)
{  /* This called before starting to drag, mouse-down was on it */
   info->format = ELM_SEL_FORMAT_TARGETS;
   info->createicon = _gl_createicon;
   info->createdata = it;
   info->dragdone = _gl_dragdone;

   /* Now, collect data to send for drop from ALL selected items */
   /* Save list pointer to remove items after drop and free list on done */
   info->data = _gl_get_drag_data(obj, it, (Eina_List **) &info->donecbdata);
   info->acceptdata = info->donecbdata;

   if (info->data)
     return EINA_TRUE;
   else
     return EINA_FALSE;
}

/**
 * @brief Get a list of icons for the default drag animation from a gengrid.
 *
 * This function is analogous to _gl_icons_get(), but operates on a gengrid.
 * It collects icons from selected items and the item under the cursor.
 *
 * @param data The gengrid object.
 * @return A list (Eina_List) of newly created Evas_Object icons.
 */
static Eina_List *
_grid_icons_get(void *data)
{  /* Start icons animation before actually drag-starts */
   printf("<%s> <%d>\n", __func__, __LINE__);

   Eina_List *l;

   Eina_List *icons = NULL;

   Evas_Coord xm, ym;
   evas_pointer_canvas_xy_get(evas_object_evas_get(data), &xm, &ym);
   Eina_List *items = eina_list_clone(elm_gengrid_selected_items_get(data));
   Elm_Object_Item *gli = elm_gengrid_at_xy_item_get(data,
         xm, ym, NULL, NULL);
   if (gli)
     {  /* Add the item mouse is over to the list if NOT seleced */
        void *p = eina_list_search_unsorted(items, _item_ptr_cmp, gli);
        if (!p)
          items = eina_list_append(items, gli);
     }

   EINA_LIST_FOREACH(items, l, gli)
     {  /* Now add icons to animation window */
        Evas_Object *o = elm_object_item_part_content_get(gli,
              "elm.swallow.icon");

        if (o)
          {
             int x, y, w, h;
             const char *f, *g;
             elm_image_file_get(o, &f, &g);
             Evas_Object *ic = elm_icon_add(data);
             elm_image_file_set(ic, f, g);
             evas_object_geometry_get(o, &x, &y, &w, &h);
             evas_object_size_hint_align_set(ic,
                   EVAS_HINT_FILL, EVAS_HINT_FILL);
             evas_object_size_hint_weight_set(ic,
                   EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

             evas_object_move(ic, x, y);
             evas_object_resize(ic, w, h);
             evas_object_show(ic);

             icons =  eina_list_append(icons, ic);
          }
     }

   eina_list_free(items);
   return icons;
}

/**
 * @brief Callback for when the drag operation officially starts.
 *
 * This is called after the initial timeout and when the user has moved the
 * cursor enough to start a drag. It is used here to start a 5-second timer
 * to test drag cancellation.
 *
 * @param data User data provided in Elm_Drag_User_Info.
 * @param obj The object being dragged from.
 */
static void
_gl_dragstart(void *data EINA_UNUSED, Evas_Object *obj)
{
   printf("<%s> <%d>\n", __func__, __LINE__);
   if (_5s_cancel)
     _5s_timeout = ecore_timer_add(5.0, _5s_timeout_gone, obj);
}

/**
 * @brief Callback to get drag data for a gengrid.
 *
 * Populates the Elm_Drag_User_Info structure for a drag operation
 * originating from a gengrid. It sets up callbacks for creating the drag icon,
 * handling drag start and completion, and provides the data to be dragged.
 *
 * @param obj The gengrid object.
 * @param it The item under the cursor.
 * @param[out] info The structure to fill with drag information.
 * @return EINA_TRUE if drag should start, EINA_FALSE to abort.
 */
static Eina_Bool
_grid_data_getcb(Evas_Object *obj,  /* The genlist object */
                 Elm_Object_Item *it,
                 Elm_Drag_User_Info *info)
{
   /* This called before starting to drag, mouse-down was on it */
   info->format = ELM_SEL_FORMAT_TARGETS;
   info->createicon = _gl_createicon;
   info->createdata = it;
   info->dragstart = _gl_dragstart;
   info->icons = _grid_icons_get(obj);
   info->dragdone = _gl_dragdone;

   /* Now, collect data to send for drop from ALL selected items */
   /* Save list pointer to remove items after drop and free list on done */
   info->data = _grid_get_drag_data(obj, it, (Eina_List **) &info->donecbdata);
   printf("%s %d- data = %s\n", __func__, __LINE__, info->data);
   info->acceptdata = info->donecbdata;

   if (info->data)
     return EINA_TRUE;
   else
     return EINA_FALSE;
}

/**
 * @brief Test for drag and drop between two genlists with default animation.
 *
 * This test creates a window with two genlists. Users can drag items from one
 * list and drop them onto the other. The drag animation is handled by
 * Elementary's default mechanism.
 */
void
test_dnd_genlist_default_anim(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   Evas_Object *win, *gl, *bxx, *bx2, *lb;
   int i, j;

   win = elm_win_util_standard_add("dnd-genlist-default-anim", "DnD-Genlist-Default-Anim");
   elm_win_autodel_set(win, EINA_TRUE);

   bxx = elm_box_add(win);
   elm_box_horizontal_set(bxx, EINA_FALSE);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

   lb = elm_label_add(win);
   elm_object_text_set(lb, "Drag and drop between genlists with default anim.");
   evas_object_size_hint_min_set(lb, 0, 50);
   evas_object_size_hint_align_set(lb, EVAS_HINT_FILL, 0.5);
   evas_object_show(lb);
   elm_box_pack_end(bxx, lb);

   bx2 = elm_box_add(win);
   elm_box_horizontal_set(bx2, EINA_TRUE);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_show(bx2);
   elm_box_pack_end(bxx, bx2);

   itc1 = elm_genlist_item_class_new();
   itc1->item_style     = "default";
   itc1->func.text_get = gl_text_get;
   itc1->func.content_get  = gl_content_get;
   itc1->func.del       = NULL;

   for (j = 0; j < 2; j++)
     {
        gl = elm_genlist_add(win);

        /* START Drag and Drop handling */
        evas_object_smart_callback_add(win, "delete,request", _win_del, gl);
        elm_genlist_multi_select_set(gl, EINA_TRUE); /* We allow multi drag */
        elm_drop_item_container_add(gl,
              ELM_SEL_FORMAT_TARGETS,
              _gl_item_getcb,
              NULL, NULL,
              NULL, NULL,
              _gl_poscb, NULL,
              _gl_dropcb, NULL);

        elm_drag_item_container_add(gl, ANIM_TIME, DRAG_TIMEOUT,
              _gl_item_getcb, _gl_dnd_default_anim_data_getcb);

        // FIXME: This causes genlist to resize the horiz axis very slowly :(
        // Reenable this and resize the window horizontally, then try to resize it back
        //elm_genlist_mode_set(gl, ELM_LIST_LIMIT);
        evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(bx2, gl);
        evas_object_show(gl);

        for (i = 0; i < 20; i++)
          {
             snprintf(buf, sizeof(buf), "%s/images/%s", elm_app_data_dir_get(), img[(i % 9)]);
             const char *path = eina_stringshare_add(buf);
             elm_genlist_item_append(gl, itc1, path, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
          }
     }

   evas_object_resize(win, 680 * elm_config_scale_get(),
                           800 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test for drag and drop between two genlists with custom animation.
 *
 * This test is similar to test_dnd_genlist_default_anim(), but it demonstrates
 * how to implement a custom animation for the start of a drag operation instead
 * of using the default one.
 */
void
test_dnd_genlist_user_anim(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   Evas_Object *win, *gl, *bxx;
   int i, j;

   win = elm_win_util_standard_add("dnd-genlist-user-anim", "DnD-Genlist-User-Anim");
   elm_win_autodel_set(win, EINA_TRUE);

   bxx = elm_box_add(win);
   elm_box_horizontal_set(bxx, EINA_TRUE);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

   itc1 = elm_genlist_item_class_new();
   itc1->item_style     = "default";
   itc1->func.text_get = gl_text_get;
   itc1->func.content_get  = gl_content_get;
   itc1->func.del       = NULL;

   for (j = 0; j < 2; j++)
     {
        gl = elm_genlist_add(win);

        /* START Drag and Drop handling */
        evas_object_smart_callback_add(win, "delete,request", _win_del, gl);
        elm_genlist_multi_select_set(gl, EINA_TRUE); /* We allow multi drag */
        elm_drop_item_container_add(gl,
              ELM_SEL_FORMAT_TARGETS,
              _gl_item_getcb,
              NULL, NULL,
              NULL, NULL,
              NULL, NULL,
              _gl_dropcb, NULL);

        elm_drag_item_container_add(gl, ANIM_TIME, DRAG_TIMEOUT,
              _gl_item_getcb, _gl_data_getcb);

        /* We add mouse-down, up callbacks to start/stop drag animation */
        evas_object_event_callback_add(gl, EVAS_CALLBACK_MOUSE_DOWN,
              _gl_obj_mouse_down, gl);
        /* END Drag and Drop handling */

        // FIXME: This causes genlist to resize the horiz axis very slowly :(
        // Reenable this and resize the window horizontally, then try to resize it back
        //elm_genlist_mode_set(gl, ELM_LIST_LIMIT);
        evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(bxx, gl);
        evas_object_show(gl);

        for (i = 0; i < 20; i++)
          {
             snprintf(buf, sizeof(buf), "%s/images/%s", elm_app_data_dir_get(), img[(i % 9)]);
             const char *path = eina_stringshare_add(buf);
             elm_genlist_item_append(gl, itc1, path, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
          }
     }

   evas_object_resize(win, 680 * elm_config_scale_get(),
                           800 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Test for drag and drop between a genlist and a gengrid.
 *
 * This test sets up a window containing a genlist and a gengrid to demonstrate
 * drag and drop interoperability between different container widgets.
 */
void
test_dnd_genlist_gengrid(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   Evas_Object *win, *bxx;
   int i;

   win = elm_win_util_standard_add("dnd-genlist-gengrid", "DnD-Genlist-Gengrid");
   elm_win_autodel_set(win, EINA_TRUE);

   bxx = elm_box_add(win);
   elm_box_horizontal_set(bxx, EINA_TRUE);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

     {
        itc1 = elm_genlist_item_class_new();
        itc1->item_style     = "default";
        itc1->func.text_get = gl_text_get;
        itc1->func.content_get  = gl_content_get;
        itc1->func.del       = NULL;

        Evas_Object *gl = elm_genlist_add(win);
        evas_object_smart_callback_add(win, "delete,request", _win_del, gl);

        /* START Drag and Drop handling */
        elm_genlist_multi_select_set(gl, EINA_TRUE); /* We allow multi drag */
        elm_drop_item_container_add(gl, ELM_SEL_FORMAT_TARGETS, _gl_item_getcb, NULL, NULL,
              NULL, NULL, NULL, NULL, _gl_dropcb, NULL);

        elm_drag_item_container_add(gl, ANIM_TIME, DRAG_TIMEOUT,
              _gl_item_getcb, _gl_dnd_default_anim_data_getcb);
        /* END Drag and Drop handling */

        // FIXME: This causes genlist to resize the horiz axis very slowly :(
        // Reenable this and resize the window horizontally, then try to resize it back
        //elm_genlist_mode_set(gl, ELM_LIST_LIMIT);
        evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(bxx, gl);
        evas_object_show(gl);

        for (i = 0; i < 20; i++)
          {
             snprintf(buf, sizeof(buf), "%s/images/%s", elm_app_data_dir_get(), img[(i % 9)]);
             const char *path = eina_stringshare_add(buf);
             elm_genlist_item_append(gl, itc1, path, NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
          }
     }

     {
        Evas_Object *grid = elm_gengrid_add(win);
        evas_object_smart_callback_add(win, "delete,request", _win_del, grid);
        elm_gengrid_item_size_set(grid,
                                  ELM_SCALE_SIZE(150), ELM_SCALE_SIZE(150));
        elm_gengrid_horizontal_set(grid, EINA_FALSE);
        elm_gengrid_reorder_mode_set(grid, EINA_FALSE);
        elm_gengrid_multi_select_set(grid, EINA_TRUE); /* We allow multi drag */
        evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(grid, EVAS_HINT_FILL, EVAS_HINT_FILL);

        gic = elm_gengrid_item_class_new();
        gic->item_style = "default";
        gic->func.text_get = gl_text_get;
        gic->func.content_get = gl_content_get;
        gic->func.del = gl_del_cb;

        elm_drop_item_container_add(grid, ELM_SEL_FORMAT_TARGETS, _grid_item_getcb, NULL, NULL,
              NULL, NULL, NULL, NULL, _grid_dropcb, NULL);

        elm_drag_item_container_add(grid, ANIM_TIME, DRAG_TIMEOUT,
                                    _grid_item_getcb, _grid_data_getcb);
        for (i = 0; i < 20; i++)
          {
             snprintf(buf, sizeof(buf), "%s/images/%s", elm_app_data_dir_get(), img[(i % 9)]);
             const char *path = eina_stringshare_add(buf);
             elm_gengrid_item_append(grid, gic, path, NULL, NULL);
          }
        elm_box_pack_end(bxx, grid);
        evas_object_show(grid);
     }

   evas_object_resize(win, 680 * elm_config_scale_get(),
                           800 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Drop callback that creates a new button in a box.
 *
 * When an item is dropped onto the target box, this callback is invoked.
 * It extracts the image path from the drop data and creates a new button
 * with that image as its icon, then adds the button to the box.
 *
 * @param data The parent window object.
 * @param obj The box object that is the drop target.
 * @param ev The selection data from the drop.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool _drop_box_button_new_cb(void *data, Evas_Object *obj, Elm_Selection_Data *ev)
{
   Evas_Object *win = data;
   if (ev->len <= 0)
     return EINA_FALSE;

   char *dd = eina_strndup(ev->data, ev->len);
   if (!dd) return EINA_FALSE;
   char *p = dd;
   char *s = _drag_data_extract(&p);
   while (s)
     {
        Evas_Object *ic = elm_icon_add(win);
        elm_image_file_set(ic, s, NULL);
        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        Evas_Object *bt = elm_button_add(win);
        elm_object_text_set(bt, "Dropped button");
        elm_object_part_content_set(bt, "icon", ic);
        elm_box_pack_end(obj, bt);
        evas_object_show(bt);
        evas_object_show(ic);
        s = _drag_data_extract(&p);
     }
   free(dd);

   return EINA_TRUE;
}

/**
 * @brief Callback for when the drag cursor enters a button's area.
 *
 * This is used for demonstrating multiple drop callbacks on a single widget.
 *
 * @param data User data.
 * @param obj The button object.
 */
void _enter_but_cb(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED)
{
   printf("Entered %s - drop it here and I will never print this line anymore.\n", __func__);
}

/**
 * @brief Drop callback to change the icon of a button.
 *
 * When an item is dropped on the button, this function takes the image path
 * from the drop data and sets it as the new icon for the button.
 *
 * @param data The parent window object.
 * @param obj The button object.
 * @param ev The selection data.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool _drop_but_icon_change_cb(void *data, Evas_Object *obj, Elm_Selection_Data *ev)
{
   Evas_Object *win = data;
   Evas_Object *ic;
   if (ev->len <= 0)
     return EINA_FALSE;

   char *dd = eina_strndup(ev->data, ev->len);
   if (!dd) return EINA_FALSE;
   char *p = dd;
   char *s = _drag_data_extract(&p);
   ic = elm_icon_add(win);
   elm_image_file_set(ic, s, NULL);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   evas_object_del(elm_object_part_content_get(obj, "icon"));
   elm_object_part_content_set(obj, "icon", ic);
   evas_object_show(ic);
   free(dd);

   return EINA_TRUE;
}

/* Callback used to test multi-callbacks feature */
/**
 * @brief Drop callback that removes itself.
 *
 * This function demonstrates how drop callbacks can be managed dynamically.
 * When called, it removes itself and another associated callback (_enter_but_cb)
 * from the button's drop targets, so they won't be triggered on subsequent drops.
 *
 * @param data User data.
 * @param obj The button object.
 * @param ev The selection data.
 * @return EINA_TRUE.
 */
static Eina_Bool _drop_but_cb_remove_cb(void *data EINA_UNUSED, Evas_Object *obj, Elm_Selection_Data *ev EINA_UNUSED)
{
   printf("Second callback called - removing it\n");
   elm_drop_target_del(obj, ELM_SEL_FORMAT_TARGETS, _enter_but_cb, NULL, NULL, NULL, NULL, NULL, _drop_but_cb_remove_cb, NULL);
   return EINA_TRUE;
}

/**
 * @brief Drop callback to change the background image.
 *
 * Handles a drop on the window background, changing the background image
 * to the one specified in the drop data.
 *
 * @param data User data.
 * @param obj The background object.
 * @param ev The selection data.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
static Eina_Bool _drop_bg_change_cb(void *data EINA_UNUSED, Evas_Object *obj, Elm_Selection_Data *ev)
{
   if (ev->len <= 0)
     return EINA_FALSE;

   char *dd = eina_strndup(ev->data, ev->len);
   if (!dd) return EINA_FALSE;
   char *p = dd;
   char *s = _drag_data_extract(&p);
   elm_bg_file_set(obj, s, NULL);
   free(dd);

   return EINA_TRUE;
}

/**
 * @brief Callback for the 'cancel after 5s' check button.
 *
 * Updates the global flag `_5s_cancel` based on the state of the check button.
 *
 * @param data User data.
 * @param obj The check button object.
 * @param event_info Event information.
 */
static void
_5s_cancel_ck_changed(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   _5s_cancel = elm_check_state_get(obj);
}

/**
 * @brief Test demonstrating multiple drag-and-drop features.
 *
 * This test sets up a window with a gengrid as a source of draggable items
 * and various drop targets (a box, buttons, the window background). It showcases:
 * - Dropping to create new widgets.
 * - Dropping to modify existing widgets (changing an icon).
 * - Having multiple drop callbacks on a single widget.
 * - Dynamically adding/removing drop callbacks.
 * - Timed cancellation of a drag operation.
 */
void
test_dnd_multi_features(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   char buf[PATH_MAX];
   Evas_Object *win, *bxx, *bg;
   int i;

   win = elm_win_util_standard_add("dnd-multi-features", "DnD-Multi Features");
   elm_win_autodel_set(win, EINA_TRUE);

   bg = elm_bg_add(win);
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_drop_target_add(bg, ELM_SEL_FORMAT_TARGETS, NULL, NULL, NULL, NULL, NULL, NULL, _drop_bg_change_cb, NULL);
   elm_win_resize_object_add(win, bg);

   /* And show the background. */
   evas_object_show(bg);
   bxx = elm_box_add(win);
   elm_box_horizontal_set(bxx, EINA_TRUE);
   evas_object_size_hint_weight_set(bxx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bxx);
   evas_object_show(bxx);

     {
        Evas_Object *grid = elm_gengrid_add(bxx);
        evas_object_smart_callback_add(win, "delete,request", _win_del, grid);
        elm_gengrid_item_size_set(grid,
                                  ELM_SCALE_SIZE(100), ELM_SCALE_SIZE(100));
        elm_gengrid_horizontal_set(grid, EINA_FALSE);
        elm_gengrid_reorder_mode_set(grid, EINA_FALSE);
        elm_gengrid_multi_select_set(grid, EINA_TRUE); /* We allow multi drag */
        evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(grid, EVAS_HINT_FILL, EVAS_HINT_FILL);

        gic = elm_gengrid_item_class_new();
        gic->item_style = "default";
        gic->func.text_get = gl_text_get;
        gic->func.content_get = gl_content_get;

        elm_drag_item_container_add(grid, ANIM_TIME, DRAG_TIMEOUT,
              _grid_item_getcb, _grid_data_getcb);
        for (i = 0; i < 10; i++)
          {
             snprintf(buf, sizeof(buf), "%s/images/%s", elm_app_data_dir_get(), img[(i % 9)]);
             const char *path = eina_stringshare_add(buf);
             elm_gengrid_item_append(grid, gic, path, NULL, NULL);
          }
        elm_box_pack_end(bxx, grid);
        evas_object_show(grid);
     }

     {
        Evas_Object *ic, *bt;
        Evas_Object *vert_box = elm_box_add(bxx);
        evas_object_size_hint_weight_set(vert_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_box_pack_end(bxx, vert_box);
        evas_object_show(vert_box);
        elm_drop_target_add(vert_box, ELM_SEL_FORMAT_TARGETS, NULL, NULL, NULL, NULL, NULL, NULL, _drop_box_button_new_cb, win);

        _5s_cancel = EINA_FALSE;
        Evas_Object *ck = elm_check_add(vert_box);
        elm_object_style_set(ck, "toggle");
        elm_object_text_set(ck, "Cancel after 5s:");
        elm_check_state_set(ck, _5s_cancel);
        evas_object_smart_callback_add(ck, "changed", _5s_cancel_ck_changed, NULL);
        elm_box_pack_end(vert_box, ck);
        evas_object_show(ck);

        ic = elm_icon_add(win);
        snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
        elm_image_file_set(ic, buf, NULL);
        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        bt = elm_button_add(win);
        elm_object_text_set(bt, "Multi-callbacks check");
        elm_drop_target_add(bt, ELM_SEL_FORMAT_TARGETS, NULL, NULL, NULL, NULL, NULL, NULL, _drop_but_icon_change_cb, win);
        elm_drop_target_add(bt, ELM_SEL_FORMAT_TARGETS, _enter_but_cb, NULL, NULL, NULL, NULL, NULL, _drop_but_cb_remove_cb, NULL);
        elm_object_part_content_set(bt, "icon", ic);
        elm_box_pack_end(vert_box, bt);
        evas_object_show(bt);
        evas_object_show(ic);

        ic = elm_icon_add(win);
        snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
        elm_image_file_set(ic, buf, NULL);
        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        bt = elm_button_add(win);
        elm_object_text_set(bt, "Drop into me to change my icon");
        elm_drop_target_add(bt, ELM_SEL_FORMAT_TARGETS, NULL, NULL, NULL, NULL, NULL, NULL, _drop_but_icon_change_cb, win);
        elm_object_part_content_set(bt, "icon", ic);
        elm_box_pack_end(vert_box, bt);
        evas_object_show(bt);
        evas_object_show(ic);

        ic = elm_icon_add(win);
        snprintf(buf, sizeof(buf), "%s/images/logo_small.png", elm_app_data_dir_get());
        elm_image_file_set(ic, buf, NULL);
        evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
        bt = elm_button_add(win);
        elm_object_text_set(bt, "No action on drop");
        elm_object_part_content_set(bt, "icon", ic);
        elm_box_pack_end(vert_box, bt);
        evas_object_show(bt);
        evas_object_show(ic);
     }
   evas_object_resize(win, 680 * elm_config_scale_get(),
                           800 * elm_config_scale_get());
   evas_object_show(win);
}

/**
 * @brief Generic callback for when a drag enters a drop target area.
 *
 * Used for logging/debugging purposes.
 *
 * @param data User data.
 * @param obj The object that was entered.
 */
static void
_enter_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   printf("%s: obj: %s %p enter\n", __func__,
          evas_object_type_get(obj), obj);
}

/**
 * @brief Generic callback for when a drag leaves a drop target area.
 *
 * Used for logging/debugging purposes.
 *
 * @param data User data.
 * @param obj The object that was left.
 */
static void
_leave_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   printf("%s: obj: %s %p leave\n", __func__,
          evas_object_type_get(obj), obj);
}

/**
 * @brief Generic callback for drag position updates over a drop target.
 *
 * Used for logging/debugging purposes.
 *
 * @param data User data.
 * @param obj The object under the cursor.
 * @param x The x-coordinate of the drag.
 * @param y The y-coordinate of the drag.
 * @param action The current drag action.
 */
static void
_pos_cb(void *data EINA_UNUSED, Evas_Object *obj, Evas_Coord x, Evas_Coord y, Elm_Xdnd_Action action)
{
   printf("%s: obj: %s %p pos: %d %d, action: %d\n", __func__,
          evas_object_type_get(obj), obj, x, y, action);
}

/**
 * @brief Drop callback for a label widget.
 *
 * Sets the text of the label to the text content from the drop data.
 *
 * @param data User data.
 * @param obj The label object.
 * @param ev The selection data containing text.
 * @return EINA_TRUE.
 */
static Eina_Bool
_label_drop_cb(void *data EINA_UNUSED, Evas_Object *obj, Elm_Selection_Data *ev)
{
   const char *text = ev->data;
   printf("%s: obj: %s %p drop data: %s\n", __func__,
          evas_object_type_get(obj), obj, text);
   elm_object_text_set(obj, text);
   return EINA_TRUE;
}

/**
 * @brief Drop callback for an image widget.
 *
 * Sets the file of the image widget to the path provided in the drop data.
 *
 * @param data User data.
 * @param obj The image object.
 * @param ev The selection data containing a file path.
 * @return EINA_TRUE.
 */
static Eina_Bool
_image_drop_cb(void *data EINA_UNUSED, Evas_Object *obj, Elm_Selection_Data *ev)
{
   char *f = (char*)eina_memdup(ev->data, ev->len, 1);

   printf("%s: obj: %s %p drop data: %s\n", __func__,
          evas_object_type_get(obj), obj, f);
   elm_image_file_set(obj, f, NULL);
   free(f);
   return EINA_TRUE;
}

/**
 * @brief Create an icon for dragging a label.
 *
 * Creates a new label to be used as the drag icon, copying the text from
 * the original label.
 *
 * @param data The original label object.
 * @param parent The parent window for the new icon.
 * @param[out] xoff Horizontal offset for the icon.
 * @param[out] yoff Vertical offset for the icon.
 * @return The new label icon object.
 */
static Evas_Object *
_label_create_icon(void *data, Evas_Object *parent, Evas_Coord *xoff, Evas_Coord *yoff)
{
   Evas_Object *lb = data;
   Evas_Object *icon;
   const char *text;
   Evas_Coord x, y, w, h;
   int xm, ym;

   icon = elm_label_add(parent);
   text = elm_object_text_get(lb);
   elm_object_text_set(icon, text);

   evas_object_geometry_get(lb, &x, &y, &w, &h);
   evas_object_move(icon, x, y);
   evas_object_resize(icon, w, h);
   evas_object_show(icon);

   evas_pointer_canvas_xy_get(evas_object_evas_get(lb), &xm, &ym);
   if (xoff) *xoff = xm - (w / 2);
   if (yoff) *yoff = ym - (h / 2);

   return icon;
}

/**
 * @brief Drag done callback for a label drag operation.
 *
 * Frees the text data that was allocated for the drag operation.
 *
 * @param data The text data to free.
 * @param obj The source object (unused).
 */
static void
_label_drag_done_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   free(data);
}

/**
 * @brief Mouse down callback to start dragging from a label.
 *
 * When the mouse is pressed on a label, this function initiates a drag
 * operation with the label's text as the data.
 *
 * @param data The label object.
 * @param e The Evas canvas.
 * @param obj The object that received the event.
 * @param event_info The mouse down event info.
 */
static void
_label_mouse_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *lb = data;
   const char *mkup = elm_object_text_get(lb);
   char *text = evas_textblock_text_markup_to_utf8(NULL, mkup);

   if (!text)
     {
        printf("Cannot convert text\n");
     }
   elm_drag_start(lb, ELM_SEL_FORMAT_TEXT, text, ELM_XDND_ACTION_COPY,
                  _label_create_icon, lb,
                  NULL, NULL, NULL, NULL,
                  _label_drag_done_cb, text);
}

/**
 * @brief Create an icon for dragging an image.
 *
 * Creates a new image object to be used as the drag icon, copying the file
 * from the original image.
 *
 * @param data The original image object.
 * @param parent The parent window for the new icon.
 * @param[out] xoff Horizontal offset for the icon.
 * @param[out] yoff Vertical offset for the icon.
 * @return The new image icon object.
 */
static Evas_Object *
_image_create_icon(void *data, Evas_Object *parent, Evas_Coord *xoff, Evas_Coord *yoff)
{
   Evas_Object *ic;
   Evas_Object *io = data;
   const char *f, *g;
   Evas_Coord x, y, w, h, xm, ym;

   elm_image_file_get(io, &f, &g);
   ic = elm_image_add(parent);
   elm_image_file_set(ic, f, g);
   evas_object_geometry_get(io, &x, &y, &w, &h);
   evas_object_move(ic, x, y);
   evas_object_resize(ic, 60, 60);
   evas_object_show(ic);

   evas_pointer_canvas_xy_get(evas_object_evas_get(io), &xm, &ym);
   if (xoff) *xoff = xm - 30;
   if (yoff) *yoff = ym - 30;

   return ic;
}

/**
 * @brief Mouse down callback to start dragging from an image.
 *
 * Initiates a drag operation with the image's file path as the data.
 *
 * @param data The image object.
 * @param e The Evas canvas.
 * @param obj The object that received the event.
 * @param event_info The mouse down event info.
 */
static void
_image_mouse_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *io = data;
   const char *f;
   char dd[PATH_MAX];

   elm_image_file_get(io, &f, NULL);
   snprintf(dd, sizeof(dd), "file://%s", f);

   elm_drag_start(io, ELM_SEL_FORMAT_IMAGE, dd, ELM_XDND_ACTION_COPY,
                  _image_create_icon, io,
                  NULL, NULL, NULL, NULL, NULL, NULL);
}

/**
 * @brief Test drag and drop with different data types.
 *
 * This test demonstrates dragging and dropping different types of content
 * (text and images) between various widgets like labels, entries, and image
 * objects. It shows how to handle different data formats in both drag source
 * and drop target.
 */
void
test_dnd_types(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *bx1, *lb, *io, *en;
   int i;

   win = elm_win_util_standard_add("dnd-types", "DnD types");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   elm_box_horizontal_set(bx, EINA_TRUE);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   for(i = 0; i < 2; i++)
     {
        char buf[PATH_MAX];
        bx1 = elm_box_add(win);
        evas_object_size_hint_weight_set(bx1, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(bx1, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(bx, bx1);
        evas_object_show(bx1);

        lb = elm_label_add(win);
        if (i == 0)
          elm_object_text_set(lb, "This is an label.<br/>You can drag or drop text here.<br/>"
                              "This is a larger label with newlines<br/>"
                              "to make it bigger, bit it won't expand or wrap<br/>"
                              "just be a block of text that can't change its<br/>"
                              "formatting as it's fixed based on text<br/>");
        else
          elm_object_text_set(lb, "Another label for drag and drop test.<br/>"
                              "A small label. You can drag or drop text.<br/><br/><br/>");
        elm_drop_target_add(lb, ELM_SEL_FORMAT_TEXT,
                            _enter_cb, NULL,
                            _leave_cb, NULL,
                            _pos_cb, NULL,
                            _label_drop_cb, NULL);
        evas_object_event_callback_add(lb, EVAS_CALLBACK_MOUSE_DOWN, _label_mouse_down_cb, lb);
        elm_box_pack_end(bx1, lb);
        evas_object_show(lb);

        en = elm_entry_add(win);
        if (i == 0)
          {
             elm_object_text_set(en,
                                 "Entry:<br/>You can only drop <b>text</> here.<br/>"
                                 "This is an entry widget in this window that<br/>"
                                 "uses markup <b>like this</> for styling.");
             elm_entry_cnp_mode_set(en, ELM_CNP_MODE_PLAINTEXT);
          }
        else
          {
             elm_object_text_set(en,
                                 "Entry:<br/>You can drop <b>text or image</> here.<br/>"
                                 "This is an entry widget in this window that<br/>"
                                 "uses markup <b>like this</> for styling.");

          }
        evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(en, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_pack_end(bx1, en);
        evas_object_show(en);

        if (i == 0)
          snprintf(buf, sizeof(buf), "%s/images/logo.png", elm_app_data_dir_get());
        else
          snprintf(buf, sizeof(buf), "%s/images/rock_02.jpg", elm_app_data_dir_get());
        io = elm_image_add(win);
        elm_image_file_set(io, buf, NULL);
        evas_object_size_hint_weight_set(io, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(io, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_min_set(io, 100, 100);
        elm_drop_target_add(io, ELM_SEL_FORMAT_IMAGE,
                            _enter_cb, NULL,
                            _leave_cb, NULL,
                            _pos_cb, NULL,
                            _image_drop_cb, NULL);
        evas_object_event_callback_add(io, EVAS_CALLBACK_MOUSE_DOWN, _image_mouse_down_cb, io);
        elm_box_pack_end(bx1, io);
        evas_object_show(io);
     }

   evas_object_show(win);
   evas_object_resize(win, 400 * elm_config_scale_get(),
                           400 * elm_config_scale_get());
}
