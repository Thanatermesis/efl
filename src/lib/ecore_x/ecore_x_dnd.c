#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>

#include "Ecore.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"
#include "Ecore_X_Atoms.h"

EAPI int ECORE_X_EVENT_XDND_ENTER = 0; /**< XDND Enter event type */
EAPI int ECORE_X_EVENT_XDND_POSITION = 0; /**< XDND Position event type */
EAPI int ECORE_X_EVENT_XDND_STATUS = 0; /**< XDND Status event type */
EAPI int ECORE_X_EVENT_XDND_LEAVE = 0; /**< XDND Leave event type */
EAPI int ECORE_X_EVENT_XDND_DROP = 0; /**< XDND Drop event type */
EAPI int ECORE_X_EVENT_XDND_FINISHED = 0; /**< XDND Finished event type */

static Ecore_X_DND_Source *_source = NULL; /**< Global DND source state */
static Ecore_X_DND_Target *_target = NULL; /**< Global DND target state */
static int _ecore_x_dnd_init_count = 0; /**< Initialization counter for DND module */

/**
 * @brief Structure to cache DND versions of windows during a drag operation.
 * This is used to optimize repeated queries for a window's DND version.
 */
typedef struct _Version_Cache_Item
{
   Ecore_X_Window win; /**< The window ID */
   int            ver; /**< The DND protocol version supported by the window (0 if not DND-aware) */
} Version_Cache_Item;
static Version_Cache_Item *_version_cache = NULL; /**< Cache for DND versions of windows */
static int _version_cache_num = 0, _version_cache_alloc = 0; /**< Number of items and allocated size for _version_cache */
static void (*_posupdatecb)(void *,
                            Ecore_X_Xdnd_Position *); /**< Callback for DND position updates */
static void *_posupdatedata; /**< User data for the position update callback */

/**
 * @internal
 * @brief Initializes the DND module.
 *
 * Sets up global DND source and target structures, and registers DND event types.
 * This function is called internally and uses a counter to manage multiple
 * initializations.
 */
void
_ecore_x_dnd_init(void)
{
   if (!_ecore_x_dnd_init_count)
     {
        _source = calloc(1, sizeof(Ecore_X_DND_Source));
        if (!_source) return;
        _source->version = ECORE_X_DND_VERSION;
        _source->win = None;
        _source->dest = None;
        _source->state = ECORE_X_DND_SOURCE_IDLE;
        _source->prev.window = 0;

        _target = calloc(1, sizeof(Ecore_X_DND_Target));
        if (!_target)
          {
             free(_source);
             _source = NULL;
             return;
          }
        _target->win = None;
        _target->source = None;
        _target->state = ECORE_X_DND_TARGET_IDLE;

        ECORE_X_EVENT_XDND_ENTER = ecore_event_type_new();
        ECORE_X_EVENT_XDND_POSITION = ecore_event_type_new();
        ECORE_X_EVENT_XDND_STATUS = ecore_event_type_new();
        ECORE_X_EVENT_XDND_LEAVE = ecore_event_type_new();
        ECORE_X_EVENT_XDND_DROP = ecore_event_type_new();
        ECORE_X_EVENT_XDND_FINISHED = ecore_event_type_new();
     }

   _ecore_x_dnd_init_count++;
}

/**
 * @internal
 * @brief Shuts down the DND module.
 *
 * Frees resources allocated by the DND module, including global DND structures
 * and event types. This function uses a counter to ensure resources are freed
 * only when the last user de-initializes the module.
 */
void
_ecore_x_dnd_shutdown(void)
{
   _ecore_x_dnd_init_count--;
   if (_ecore_x_dnd_init_count > 0)
     return;

   ecore_event_type_flush(ECORE_X_EVENT_XDND_ENTER,
                          ECORE_X_EVENT_XDND_POSITION,
                          ECORE_X_EVENT_XDND_STATUS,
                          ECORE_X_EVENT_XDND_LEAVE,
                          ECORE_X_EVENT_XDND_DROP,
                          ECORE_X_EVENT_XDND_FINISHED);

   if (_source)
     free(_source);

   _source = NULL;

   if (_target)
     free(_target);

   _target = NULL;

   _ecore_x_dnd_init_count = 0;
}

/**
 * @internal
 * @brief Default converter function for DND data.
 *
 * This function is used to convert data to a format suitable for X text properties.
 * It's typically used when setting DND types.
 *
 * @param target The target type (unused).
 * @param data The source data to convert.
 * @param size The size of the source data.
 * @param data_ret Pointer to store the converted data.
 * @param size_ret Pointer to store the size of the converted data.
 * @param tprop Target property (unused).
 * @param count Count (unused).
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_ecore_x_dnd_converter_copy(char *target EINA_UNUSED,
                            void *data,
                            int size,
                            void **data_ret,
                            int *size_ret,
                            Ecore_X_Atom *tprop EINA_UNUSED,
                            int *count EINA_UNUSED)
{
   XTextProperty text_prop;
   char *mystr;
   XICCEncodingStyle style = XTextStyle;

   if (!data || !size)
     return EINA_FALSE;

   mystr = calloc(1, size + 1);
   if (!mystr)
     return EINA_FALSE;

   memcpy(mystr, data, size);

   if (XmbTextListToTextProperty(_ecore_x_disp, &mystr, 1, style,
                                 &text_prop) == Success)
     {
        int bufsize = strlen((char *)text_prop.value) + 1;
        if (_ecore_xlib_sync) ecore_x_sync();
        *data_ret = malloc(bufsize);
        if (!*data_ret)
          {
             free(mystr);
             return EINA_FALSE;
          }
        memcpy(*data_ret, text_prop.value, bufsize);
        *size_ret = bufsize;
        XFree(text_prop.value);
        free(mystr);
        return EINA_TRUE;
     }
   else
     {
        if (_ecore_xlib_sync) ecore_x_sync();
        free(mystr);
        return EINA_FALSE;
     }
}

/**
 * @brief Sets a window as DND (Drag and Drop) aware or not.
 *
 * This function sets the XDND_AWARE property on the given window.
 * If @p on is @c EINA_TRUE, the property is set to indicate the window
 * supports the DND protocol version ECORE_X_DND_VERSION.
 * If @p on is @c EINA_FALSE, the property is deleted.
 *
 * @param win The window to set DND awareness for.
 * @param on @c EINA_TRUE to make the window DND aware, @c EINA_FALSE otherwise.
 */
EAPI void
ecore_x_dnd_aware_set(Ecore_X_Window win,
                      Eina_Bool on)
{
   Ecore_X_Atom prop_data = ECORE_X_DND_VERSION;

   LOGFN;
   if (on)
     ecore_x_window_prop_property_set(win, ECORE_X_ATOM_XDND_AWARE,
                                      XA_ATOM, 32, &prop_data, 1);
   else
     ecore_x_window_prop_property_del(win, ECORE_X_ATOM_XDND_AWARE);
}

/**
 * @brief Gets the DND protocol version supported by a window.
 *
 * This function queries the XDND_AWARE property of the given window
 * to determine the DND protocol version it supports.
 * During an active drag operation (when _source->state is ECORE_X_DND_SOURCE_DRAGGING),
 * this function uses an internal cache (_version_cache) to avoid repeated X server queries,
 * which can be expensive during mouse move events.
 *
 * @param win The window to query.
 * @return The DND protocol version supported by the window, or 0 if the window
 *         is not DND aware or an error occurs.
 */
EAPI int
ecore_x_dnd_version_get(Ecore_X_Window win)
{
   unsigned char *prop_data;
   int num;
   Version_Cache_Item *t;

   LOGFN;
   // this looks hacky - and it is, but we need a way of caching info about
   // a window while dragging, because we literally query this every mouse
   // move and going to and from x multiple times per move is EXPENSIVE
   // and slows things down, puts lots of load on x etc.
   if (_source->state == ECORE_X_DND_SOURCE_DRAGGING)
     if (_version_cache)
       {
          int i;

          for (i = 0; i < _version_cache_num; i++)
            {
               if (_version_cache[i].win == win)
                 return _version_cache[i].ver;
            }
       }

   if (ecore_x_window_prop_property_get(win, ECORE_X_ATOM_XDND_AWARE,
                                        XA_ATOM, 32, &prop_data, &num))
     {
        int version = (int)*prop_data;
        free(prop_data);
        if (_source->state == ECORE_X_DND_SOURCE_DRAGGING)
          {
             _version_cache_num++;
             if (_version_cache_num > _version_cache_alloc)
               _version_cache_alloc += 16;

             t = realloc(_version_cache,
                         _version_cache_alloc *
                         sizeof(Version_Cache_Item));
             if (!t) return 0;
             _version_cache = t;
             _version_cache[_version_cache_num - 1].win = win;
             _version_cache[_version_cache_num - 1].ver = version;
          }

        return version;
     }

   if (_source->state == ECORE_X_DND_SOURCE_DRAGGING)
     {
        _version_cache_num++;
        if (_version_cache_num > _version_cache_alloc)
          _version_cache_alloc += 16;

        t = realloc(_version_cache, _version_cache_alloc *
                    sizeof(Version_Cache_Item));
        if (!t)
          {
             if (prop_data) free(prop_data);
             return 0;
          }

        _version_cache = t;
        _version_cache[_version_cache_num - 1].win = win;
        _version_cache[_version_cache_num - 1].ver = 0;
     }

   if (prop_data) free(prop_data);

   return 0;
}

/**
 * @brief Checks if a specific DND data type is set for a window.
 *
 * This function queries the XDND_TYPE_LIST property of the given window
 * to see if the specified @p type (e.g., "text/uri-list", "text/plain")
 * is among the supported data types.
 *
 * @param win The window to check.
 * @param type The DND data type string to check for.
 * @return @c EINA_TRUE if the type is set, @c EINA_FALSE otherwise or on error.
 */
EAPI Eina_Bool
ecore_x_dnd_type_isset(Ecore_X_Window win,
                       const char *type)
{
   int num, i, ret = EINA_FALSE;
   unsigned char *data;
   Ecore_X_Atom *atoms, atom;

   LOGFN;
   if (!ecore_x_window_prop_property_get(win, ECORE_X_ATOM_XDND_TYPE_LIST,
                                         XA_ATOM, 32, &data, &num))
     return ret;

   atom = ecore_x_atom_get(type);
   atoms = (Ecore_X_Atom *)data;

   for (i = 0; i < num; ++i)
     {
        if (atom == atoms[i])
          {
             ret = EINA_TRUE;
             break;
          }
     }

   if (data) free(data);
   return ret;
}

/**
 * @brief Adds or removes a DND data type for a window.
 *
 * This function modifies the XDND_TYPE_LIST property of the given window.
 * If @p on is @c EINA_TRUE, the specified @p type is added to the list of
 * supported data types (prepended to the existing list).
 * If @p on is @c EINA_FALSE, the specified @p type is removed from the list.
 *
 * @param win The window to modify.
 * @param type The DND data type string (e.g., "text/uri-list").
 * @param on @c EINA_TRUE to add the type, @c EINA_FALSE to remove it.
 */
EAPI void
ecore_x_dnd_type_set(Ecore_X_Window win,
                     const char *type,
                     Eina_Bool on)
{
   Ecore_X_Atom atom;
   Ecore_X_Atom *oldset = NULL, *newset = NULL;
   int i, j = 0, num = 0;
   unsigned char *data = NULL;
   unsigned char *old_data = NULL;

   LOGFN;
   atom = ecore_x_atom_get(type);

   LOGFN;
   if (on)
     {
        if (ecore_x_window_prop_property_get(win, ECORE_X_ATOM_XDND_TYPE_LIST,
                                             XA_ATOM, 32, &old_data, &num) > 0)
          {
             if (ecore_x_dnd_type_isset(win, type))
               {
                  if (old_data) free(old_data);
                  return;
               }
          }

        newset = calloc(num + 1, sizeof(Ecore_X_Atom));
        if (!newset)
          {
             if (old_data) free(old_data);
             return;
          }

        oldset = (Ecore_X_Atom *)old_data;
        data = (unsigned char *)newset;

        for (i = 0; i < num; i++)
          newset[i + 1] = oldset[i];
        /* prepend the new type */
        newset[0] = atom;

        ecore_x_window_prop_property_set(win, ECORE_X_ATOM_XDND_TYPE_LIST,
                                         XA_ATOM, 32, data, num + 1);
     }
   else
     {
        if (ecore_x_window_prop_property_get(win, ECORE_X_ATOM_XDND_TYPE_LIST,
                                             XA_ATOM, 32, &old_data, &num) == 0)
           return;
        if (!ecore_x_dnd_type_isset(win, type))
          {
             if (old_data) free(old_data);
             return;
          }

        newset = calloc(num - 1, sizeof(Ecore_X_Atom));
        if (!newset)
          {
             if (old_data) free(old_data);
             return;
          }

        oldset = (Ecore_X_Atom *)old_data;
        data = (unsigned char *)newset;
        for (i = 0; i < num; i++)
          if (oldset[i] != atom)
            newset[j++] = oldset[i];

        ecore_x_window_prop_property_set(win, ECORE_X_ATOM_XDND_TYPE_LIST,
                                         XA_ATOM, 32, data, num - 1);
     }

   if (oldset) XFree(oldset);
   free(newset);
}

/**
 * @brief Sets the list of DND data types supported by a window.
 *
 * This function replaces the XDND_TYPE_LIST property of the given window
 * with a new list of types. For each type, it also registers a default
 * converter function (_ecore_x_dnd_converter_copy).
 *
 * @param win The window to set the types for.
 * @param types An array of DND data type strings.
 *              Example: `const char *my_types[] = {"text/plain", "text/uri-list"};`
 * @param num_types The number of types in the @p types array.
 */
EAPI void
ecore_x_dnd_types_set(Ecore_X_Window win,
                      const char **types,
                      unsigned int num_types)
{
   Ecore_X_Atom *newset = NULL;
   unsigned int i;
   unsigned char *data = NULL;

   LOGFN;
   if (!num_types)
     ecore_x_window_prop_property_del(win, ECORE_X_ATOM_XDND_TYPE_LIST);
   else
     {
        newset = calloc(num_types, sizeof(Ecore_X_Atom));
        if (!newset)
          return;

        data = (unsigned char *)newset;
        for (i = 0; i < num_types; i++)
          {
             newset[i] = ecore_x_atom_get(types[i]);
             ecore_x_selection_converter_atom_add(newset[i],
                                                  _ecore_x_dnd_converter_copy);
          }
        ecore_x_window_prop_property_set(win, ECORE_X_ATOM_XDND_TYPE_LIST,
                                         XA_ATOM, 32, data, num_types);
        free(newset);
     }
}

/**
 * @brief Sets the list of DND actions supported by a window.
 *
 * This function sets the XDND_ACTION_LIST property of the given window.
 * For each action, it also registers a default converter function
 * (_ecore_x_dnd_converter_copy).
 *
 * @param win The window to set the actions for.
 * @param actions An array of DND action atoms.
 *                Example: `Ecore_X_Atom my_actions[] = {ECORE_X_ATOM_XDND_ACTION_COPY, ECORE_X_ATOM_XDND_ACTION_MOVE};`
 * @param num_actions The number of actions in the @p actions array.
 */
EAPI void
ecore_x_dnd_actions_set(Ecore_X_Window win,
                        Ecore_X_Atom *actions,
                        unsigned int num_actions)
{
   unsigned int i;
   unsigned char *data = NULL;

   LOGFN;
   if (!num_actions)
     ecore_x_window_prop_property_del(win, ECORE_X_ATOM_XDND_ACTION_LIST);
   else
     {
        data = (unsigned char *)actions;
        for (i = 0; i < num_actions; i++)
          {
             ecore_x_selection_converter_atom_add(actions[i],
                                                  _ecore_x_dnd_converter_copy);
          }
        ecore_x_window_prop_property_set(win, ECORE_X_ATOM_XDND_ACTION_LIST,
                                         XA_ATOM, 32, data, num_actions);
     }
}

/**
 * The DND position update cb is called Ecore_X sends a DND position to a
 * client.
 *
 * It essentially mirrors some of the data sent in the position message.
 * Generally this cb should be set just before position update is called.
 * Please note well you need to look after your own data pointer if someone
 * trashes you position update cb set.
 *
 * It is considered good form to clear this when the dnd event finishes.
 *
 * @param cb Callback to updated each time ecore_x sends a position update.
 * @param data User data.
 */
EAPI void
ecore_x_dnd_callback_pos_update_set(
  void (*cb)(void *,
             Ecore_X_Xdnd_Position *data),
  const void *data)
{
   _posupdatecb = cb;
   _posupdatedata = (void *)data; /* Discard the const early */
}

/**
 * @internal
 * @brief Gets the global DND source state.
 * @return A pointer to the global Ecore_X_DND_Source structure.
 */
Ecore_X_DND_Source *
_ecore_x_dnd_source_get(void)
{
   return _source;
}

/**
 * @internal
 * @brief Gets the global DND target state.
 * @return A pointer to the global Ecore_X_DND_Target structure.
 */
Ecore_X_DND_Target *
_ecore_x_dnd_target_get(void)
{
   return _target;
}


/**
 * @internal
 * @brief Internal function to begin a DND operation.
 *
 * This function initiates a DND drag. It takes ownership of the XdndSelection,
 * sets the source window state, and prepares for dragging.
 *
 * @param source The source window initiating the drag.
 * @param self @c EINA_TRUE if this is a self-drag (drag within the same application/toolkit instance),
 *             @c EINA_FALSE otherwise. If not a self-drag, the source window will be ignored for events.
 * @param data The data associated with the drag (typically type information).
 * @param size The size of the data.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
static Eina_Bool
_ecore_x_dnd_begin(Ecore_X_Window source,
                   Eina_Bool self,
                   unsigned char *data,
                   int size)
{
   LOGFN;
   if (!ecore_x_dnd_version_get(source))
     return EINA_FALSE;

   /* Take ownership of XdndSelection */
   if (!ecore_x_selection_xdnd_set(source, data, size))
     return EINA_FALSE;

   if (_version_cache)
     {
        free(_version_cache);
        _version_cache = NULL;
        _version_cache_num = 0;
        _version_cache_alloc = 0;
     }

   ecore_x_window_shadow_tree_flush();

   _source->win = source;
   if (!self) ecore_x_window_ignore_set(_source->win, 1);
   _source->state = ECORE_X_DND_SOURCE_DRAGGING;
   _source->time = _ecore_x_event_last_time;
   _source->prev.window = 0;

   /* Default Accepted Action: move */
   _source->action = ECORE_X_ATOM_XDND_ACTION_MOVE;
   _source->accepted_action = None;
   _source->dest = None;

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Internal function to handle a DND drop.
 *
 * This function finalizes a DND operation by sending appropriate messages
 * (XdndDrop or XdndLeave) to the destination window.
 *
 * @param self @c EINA_TRUE if this was a self-drag operation, @c EINA_FALSE otherwise.
 *             This affects whether the source window's ignore status is reset.
 * @return @c EINA_TRUE if an XdndDrop message was sent (meaning the drop was on a willing target),
 *         @c EINA_FALSE otherwise (e.g., dropped on nothing or an unwilling target).
 */
static Eina_Bool
_ecore_x_dnd_drop(Eina_Bool self)
{
   XEvent xev = { 0 };
   int status = EINA_FALSE;

   LOGFN;
   if (_source->dest)
     {
        xev.xany.type = ClientMessage;
        xev.xany.display = _ecore_x_disp;
        xev.xclient.format = 32;
        xev.xclient.window = _source->dest;

        if (_source->will_accept)
          {
             xev.xclient.message_type = ECORE_X_ATOM_XDND_DROP;
             xev.xclient.data.l[0] = _source->win;
             xev.xclient.data.l[1] = 0;
             xev.xclient.data.l[2] = _source->time;
             XSendEvent(_ecore_x_disp, _source->dest, False, 0, &xev);
             if (_ecore_xlib_sync) ecore_x_sync();
             _source->state = ECORE_X_DND_SOURCE_DROPPED;
             status = EINA_TRUE;
          }
        else
          {
             xev.xclient.message_type = ECORE_X_ATOM_XDND_LEAVE;
             xev.xclient.data.l[0] = _source->win;
             xev.xclient.data.l[1] = 0;
             XSendEvent(_ecore_x_disp, _source->dest, False, 0, &xev);
             if (_ecore_xlib_sync) ecore_x_sync();
             _source->state = ECORE_X_DND_SOURCE_IDLE;
          }
     }
   else
     {
        /* Dropping on nothing */
        ecore_x_selection_xdnd_clear();
        _source->state = ECORE_X_DND_SOURCE_IDLE;
     }

   if (!self) ecore_x_window_ignore_set(_source->win, 0);

   _source->prev.window = 0;

   return status;
}

/**
 * @brief Begins a DND (Drag and Drop) operation from a source window.
 *
 * This function initiates a DND drag. It's a wrapper around _ecore_x_dnd_begin,
 * specifically for non-self-drags (i.e., drags that might go to other applications).
 *
 * @param source The source window initiating the drag.
 * @param data The data associated with the drag (typically type information).
 *             This data is usually a list of atoms representing the offered types.
 * @param size The size of the @p data in bytes.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_x_dnd_begin(Ecore_X_Window source,
                  unsigned char *data,
                  int size)
{
   return _ecore_x_dnd_begin(source, EINA_FALSE, data, size);
}

/**
 * @brief Finalizes a DND (Drag and Drop) operation.
 *
 * This function is called when the drag operation is completed (e.g., mouse button released).
 * It's a wrapper around _ecore_x_dnd_drop for non-self-drags.
 *
 * @return @c EINA_TRUE if the drop was on a willing target, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_dnd_drop(void)
{
   return _ecore_x_dnd_drop(EINA_FALSE);
}

/**
 * @brief Begins a self-DND (Drag and Drop) operation from a source window.
 *
 * This function initiates a DND drag intended for the same application or toolkit instance.
 * It's a wrapper around _ecore_x_dnd_begin, marking the drag as a "self" drag.
 *
 * @param source The source window initiating the drag.
 * @param data The data associated with the drag (typically type information).
 * @param size The size of the @p data in bytes.
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_x_dnd_self_begin(Ecore_X_Window source,
                       unsigned char *data,
                       int size)
{
   return _ecore_x_dnd_begin(source, EINA_TRUE, data, size);
}

/**
 * @brief Finalizes a self-DND (Drag and Drop) operation.
 *
 * This function is called when a self-drag operation is completed.
 * It's a wrapper around _ecore_x_dnd_drop for self-drags.
 *
 * @return @c EINA_TRUE if the drop was on a willing target, @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_x_dnd_self_drop(void)
{
   return _ecore_x_dnd_drop(EINA_TRUE);
}

/**
 * @brief Sends an XDND_STATUS message from a target window to a source window.
 *
 * This function is called by a DND target window in response to an XDND_POSITION
 * message from a DND source. It informs the source whether the target is willing
 * to accept the drop, the action it would perform, and a rectangle within which
 * further XDND_POSITION messages can be suppressed.
 *
 * @param will_accept @c EINA_TRUE if the target will accept the drop, @c EINA_FALSE otherwise.
 * @param suppress @c EINA_TRUE if the source should suppress sending XDND_POSITION
 *                 messages while the cursor is within @p rectangle.
 * @param rectangle The rectangle (x, y, width, height) within which position updates can be suppressed.
 *                  Example: `Ecore_X_Rectangle rect = {10, 10, 100, 50};`
 * @param action The DND action atom (e.g., ECORE_X_ATOM_XDND_ACTION_COPY) the target
 *               will perform if the drop occurs. Set to `None` if @p will_accept is @c EINA_FALSE.
 */
EAPI void
ecore_x_dnd_send_status(Eina_Bool will_accept,
                        Eina_Bool suppress,
                        Ecore_X_Rectangle rectangle,
                        Ecore_X_Atom action)
{
   XEvent xev = { 0 };

   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);

   if (_target->state == ECORE_X_DND_TARGET_IDLE)
     return;

   LOGFN;
   memset(&xev, 0, sizeof(XEvent));

   _target->will_accept = will_accept;

   xev.xclient.type = ClientMessage;
   xev.xclient.display = _ecore_x_disp;
   xev.xclient.message_type = ECORE_X_ATOM_XDND_STATUS;
   xev.xclient.format = 32;
   xev.xclient.window = _target->source;

   xev.xclient.data.l[0] = _target->win;
   xev.xclient.data.l[1] = 0;
   if (will_accept)
     xev.xclient.data.l[1] |= 0x1UL;

   if (!suppress)
     xev.xclient.data.l[1] |= 0x2UL;

   /* Set rectangle information */
   xev.xclient.data.l[2] = rectangle.x;
   xev.xclient.data.l[2] <<= 16;
   xev.xclient.data.l[2] |= rectangle.y;
   xev.xclient.data.l[3] = rectangle.width;
   xev.xclient.data.l[3] <<= 16;
   xev.xclient.data.l[3] |= rectangle.height;

   if (will_accept)
     {
        xev.xclient.data.l[4] = action;
        _target->accepted_action = action;
     }
   else
     {
        xev.xclient.data.l[4] = None;
        _target->accepted_action = action;
     }

   XSendEvent(_ecore_x_disp, _target->source, False, 0, &xev);
   if (_ecore_xlib_sync) ecore_x_sync();
}

/**
 * @brief Sends an XDND_FINISHED message from a target window to a source window.
 *
 * This function is called by a DND target window after it has processed a drop
 * (e.g., after receiving an XDND_DROP message and attempting to retrieve the data).
 * It informs the source that the DND transaction is complete from the target's perspective.
 */
EAPI void
ecore_x_dnd_send_finished(void)
{
   XEvent xev = { 0 };

   EINA_SAFETY_ON_NULL_RETURN(_ecore_x_disp);

   if (_target->state == ECORE_X_DND_TARGET_IDLE)
     return;

   LOGFN;
   xev.xany.type = ClientMessage;
   xev.xany.display = _ecore_x_disp;
   xev.xclient.message_type = ECORE_X_ATOM_XDND_FINISHED;
   xev.xclient.format = 32;
   xev.xclient.window = _target->source;

   xev.xclient.data.l[0] = _target->win;
   xev.xclient.data.l[1] = 0;
   xev.xclient.data.l[2] = 0;
   if (_target->will_accept)
     {
        xev.xclient.data.l[1] |= 0x1UL;
        xev.xclient.data.l[2] = _target->accepted_action;
     }

   XSendEvent(_ecore_x_disp, _target->source, False, 0, &xev);
   if (_ecore_xlib_sync) ecore_x_sync();

   _target->state = ECORE_X_DND_TARGET_IDLE;
}

/**
 * @brief Sets the current action for an ongoing DND source operation.
 *
 * This function allows the DND source to change its preferred action
 * (e.g., from move to copy) during a drag. If a drag is in progress,
 * it may re-send an XDND_POSITION message to the current target
 * with the updated action.
 *
 * @param action The new DND action atom (e.g., ECORE_X_ATOM_XDND_ACTION_COPY,
 *               ECORE_X_ATOM_XDND_ACTION_MOVE).
 */
EAPI void
ecore_x_dnd_source_action_set(Ecore_X_Atom action)
{
   _source->action = action;
   if (_source->prev.window)
     _ecore_x_dnd_drag(_source->prev.window, _source->prev.x, _source->prev.y);
}

/**
 * @brief Gets the current action for an ongoing DND source operation.
 *
 * @return The current DND action atom set by the source.
 */
EAPI Ecore_X_Atom
ecore_x_dnd_source_action_get(void)
{
   return _source->action;
}

/**
 * @internal
 * @brief Handles the logic for dragging in a DND operation.
 *
 * This function is called typically on mouse motion during a DND drag.
 * It identifies the window under the cursor, sends XDND_ENTER, XDND_LEAVE,
 * and XDND_POSITION messages as appropriate to potential target windows.
 * It uses a shadow window tree for efficient window lookup.
 *
 * @param root The root window relevant to the current drag coordinates.
 * @param x The current X coordinate of the cursor.
 * @param y The current Y coordinate of thecursor.
 */
void
_ecore_x_dnd_drag(Ecore_X_Window root,
                  int x,
                  int y)
{
   XEvent xev = { 0 };
   Ecore_X_Window win;
   Ecore_X_Window *skip;
   Ecore_X_Xdnd_Position pos;
   int num;

   if (_source->state != ECORE_X_DND_SOURCE_DRAGGING)
     return;

   /* Preinitialize XEvent struct */
   memset(&xev, 0, sizeof(XEvent));
   xev.xany.type = ClientMessage;
   xev.xany.display = _ecore_x_disp;
   xev.xclient.format = 32;

   /* Attempt to find a DND-capable window under the cursor */
   skip = ecore_x_window_ignore_list(&num);
// WARNING - this function is HEAVY. it goes to and from x a LOT walking the
// window tree - use the SHADOW version - makes a 1-off tree copy, then uses
// that instead.
//   win = ecore_x_window_at_xy_with_skip_get(x, y, skip, num);
   win = ecore_x_window_shadow_tree_at_xy_with_skip_get(root, x, y, skip, num);
// NOTE: This now uses the shadow version to find parent windows
//   while ((win) && !(ecore_x_dnd_version_get(win)))
//     win = ecore_x_window_parent_get(win);
   while ((win) && !(ecore_x_dnd_version_get(win)))
     win = ecore_x_window_shadow_parent_get(root, win);

   /* Send XdndLeave to current destination window if we have left it */
   if ((_source->dest) && (win != _source->dest))
     {
        xev.xclient.window = _source->dest;
        xev.xclient.message_type = ECORE_X_ATOM_XDND_LEAVE;
        xev.xclient.data.l[0] = _source->win;
        xev.xclient.data.l[1] = 0;

        XSendEvent(_ecore_x_disp, _source->dest, False, 0, &xev);
        if (_ecore_xlib_sync) ecore_x_sync();
        _source->suppress = 0;
     }

   if (win)
     {
        int x1, x2, y1, y2;

        _source->version = MIN(ECORE_X_DND_VERSION,
                               ecore_x_dnd_version_get(win));
        if (win != _source->dest)
          {
             int i;
             unsigned char *data;
             Ecore_X_Atom *types;

             if (ecore_x_window_prop_property_get(_source->win,
                                                  ECORE_X_ATOM_XDND_TYPE_LIST,
                                                  XA_ATOM,
                                                  32,
                                                  &data,
                                                  &num))
               {
                  types = (Ecore_X_Atom *)data;

                  /* Entered new window, send XdndEnter */
                  xev.xclient.window = win;
                  xev.xclient.message_type = ECORE_X_ATOM_XDND_ENTER;
                  xev.xclient.data.l[0] = _source->win;
                  xev.xclient.data.l[1] = 0;
                  if (num > 3)
                     xev.xclient.data.l[1] |= 0x1UL;
                  else
                     xev.xclient.data.l[1] &= 0xfffffffeUL;

                  xev.xclient.data.l[1] |= ((unsigned long)_source->version) << 24;

                  for (i = 2; i < 5; i++)
                     xev.xclient.data.l[i] = 0;
                  for (i = 0; i < MIN(num, 3); ++i)
                     xev.xclient.data.l[i + 2] = types[i];
                  XFree(data);
                  XSendEvent(_ecore_x_disp, win, False, 0, &xev);
                  if (_ecore_xlib_sync) ecore_x_sync();
               }
             _source->await_status = 0;
             _source->will_accept = 0;
          }

        /* Determine if we're still in the rectangle from the last status */
        x1 = _source->rectangle.x;
        x2 = _source->rectangle.x + _source->rectangle.width;
        y1 = _source->rectangle.y;
        y2 = _source->rectangle.y + _source->rectangle.height;

        if ((!_source->await_status) ||
            (!_source->suppress) ||
            ((x < x1) || (x > x2) || (y < y1) || (y > y2)))
          {
             xev.xclient.window = win;
             xev.xclient.message_type = ECORE_X_ATOM_XDND_POSITION;
             xev.xclient.data.l[0] = _source->win;
             xev.xclient.data.l[1] = 0; /* Reserved */
             xev.xclient.data.l[2] = ((x << 16) & 0xffff0000) | (y & 0xffff);
             xev.xclient.data.l[3] = _source->time; /* Version 1 */
             xev.xclient.data.l[4] = _source->action; /* Version 2, Needs to be pre-set */
             XSendEvent(_ecore_x_disp, win, False, 0, &xev);
             if (_ecore_xlib_sync) ecore_x_sync();

             _source->await_status = 1;
          }
     }

   if (_posupdatecb)
     {
        pos.position.x = x;
        pos.position.y = y;
        pos.win = win;
        pos.prev = _source->dest;
        _posupdatecb(_posupdatedata, &pos);
     }

   _source->prev.x = x;
   _source->prev.y = y;
   _source->prev.window = root;
   _source->dest = win;
}

/**
 * @brief Aborts an ongoing DND operation initiated by the given source window.
 *
 * This function effectively cancels the drag by simulating a drop onto nothing,
 * thereby cleaning up the DND state. It should be called if the source
 * decides to cancel the drag (e.g., if the Esc key is pressed).
 *
 * @param xwin_source The source window that initiated the DND operation to be aborted.
 * @return @c EINA_TRUE if the DND operation was successfully aborted (i.e., if
 *         @p xwin_source matched the current DND source window).
 *         @c EINA_FALSE otherwise (e.g., if no DND operation was active or
 *         @p xwin_source was not the initiator).
 */
EAPI Eina_Bool
ecore_x_dnd_abort(Ecore_X_Window xwin_source)
{
   if (xwin_source == _source->win)
     {
        _source->will_accept = 0;
        return ecore_x_dnd_self_drop();
     }
   else return EINA_FALSE;
}

/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/
