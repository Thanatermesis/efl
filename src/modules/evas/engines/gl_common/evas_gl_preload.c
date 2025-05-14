#include "evas_gl_private.h"

static Eina_Thread async_loader_thread;
static Eina_Condition async_loader_cond;
static Eina_Lock async_loader_lock;

static Evas_GL_Texture_Async_Preload *async_current = NULL;
static Eina_List *async_loader_tex = NULL;
static Eina_List *async_loader_todie = NULL;
static Eina_Bool async_loader_exit = EINA_FALSE;
static Eina_Bool async_loader_running = EINA_FALSE;
static Eina_Bool async_loader_standby = EINA_FALSE;
static Eina_Bool async_current_cancel = EINA_FALSE;
static int async_loader_init = 0;

static void *async_engine_data = NULL;
static evas_gl_make_current_cb async_gl_make_current = NULL;

/**
 * @brief Push an asynchronous texture preload request to the loader queue.
 *
 * This function adds a new texture preload request to the end of the
 * asynchronous loader queue. The actual loading is performed by a
 * background thread. This function is thread-safe.
 *
 * @param async The asynchronous preload request to add.
 * @return EINA_TRUE on success, EINA_FALSE otherwise.
 */
Eina_Bool
evas_gl_preload_push(Evas_GL_Texture_Async_Preload *async)
{
   if (!async_loader_init) return EINA_FALSE;

   eina_lock_take(&async_loader_lock);
   async_loader_tex = eina_list_append(async_loader_tex, async);
   eina_lock_release(&async_loader_lock);

   return EINA_TRUE;
}

/**
 * @brief Remove a texture from the asynchronous preloading system.
 *
 * This function removes a texture from the preload queue. If the texture
 * is currently being uploaded, the upload is cancelled. If the texture
 * was already in the queue but not yet being processed, it is simply
 * removed. This function is thread-safe.
 *
 * @param tex The texture to remove from the preloading system.
 */
void
evas_gl_preload_pop(Evas_GL_Texture *tex)
{
   Evas_GL_Texture_Async_Preload *async;
   Eina_List *l;

   if (!async_loader_init) return ;

   eina_lock_take(&async_loader_lock);

   if (async_gl_make_current && async_current && async_current->tex == tex)
     {
        Eina_Bool running = async_loader_running;
        evas_gl_make_current_cb tmp_cb = async_gl_make_current;
        Evas_GL_Texture_Async_Preload *current = async_current;
        void *tmp_data = async_engine_data;

        async_current_cancel = EINA_TRUE;
        async_current = NULL;
        eina_lock_release(&async_loader_lock);

        if (running) evas_gl_preload_render_lock(tmp_cb, tmp_data);

        evas_gl_common_texture_free(current->tex, EINA_FALSE);
        evas_cache_image_drop(&current->im->cache_entry);
        free(current);

        if (running) evas_gl_preload_render_unlock(tmp_cb, tmp_data);

        return ;
     }

   EINA_LIST_FOREACH(async_loader_tex, l, async)
     if (async->tex == tex)
       {
          async_loader_tex = eina_list_remove_list(async_loader_tex, l);

          evas_gl_common_texture_free(async->tex, EINA_FALSE);
          evas_cache_image_drop(&async->im->cache_entry);
          free(async);

          break;
       }

   eina_lock_release(&async_loader_lock);
}

/**
 * @brief Process completed texture preloads in the main thread.
 *
 * This function is called in the main loop to handle cleanup for textures
 * that have finished preloading. It frees resources associated with the
 * preload request and marks the texture as ready. It ensures that GL
 * operations are performed while the render lock is held if the async
 * loader was running.
 *
 * The `async_loader_todie` list contains preload requests that are
 * completed and ready for cleanup.
 */
static void
_evas_gl_preload_main_loop_wakeup(void)
{
   Evas_GL_Texture_Async_Preload *async;
   evas_gl_make_current_cb cb = async_gl_make_current;
   void *data = async_engine_data;
   Eina_Bool running = async_loader_running;

   if (running) evas_gl_preload_render_lock(cb, data);
   EINA_LIST_FREE(async_loader_todie, async)
     {
        Eo *target;

        if (async->tex)
          {
             EINA_LIST_FREE(async->tex->targets, target)
               evas_object_image_pixels_dirty_set(target, EINA_TRUE);
          }
        async->im->cache_entry.flags.preload_done = 0;
        if (async->tex)
          {
             async->tex->was_preloaded = EINA_TRUE;

             async->tex->ptt->allocations =
               eina_list_remove(async->tex->ptt->allocations,
                                async->tex->aptt);
             eina_rectangle_pool_release(async->tex->aptt);
             async->tex->aptt = NULL;
             pt_unref(async->tex->ptt);
             async->tex->ptt = NULL;

             evas_gl_common_texture_free(async->tex, EINA_FALSE);
          }
        evas_cache_image_drop(&async->im->cache_entry);
        free(async);
     }
   if (running) evas_gl_preload_render_unlock(cb, data);
}

/**
 * @brief Callback wrapper to wake up the main loop for preload processing.
 *
 * This is a simple callback wrapper for evas_async_events_put() to schedule
 * _evas_gl_preload_main_loop_wakeup() to be executed in the main loop.
 */
static void
_evas_gl_preload_main_loop_wakeup_cb(void *target EINA_UNUSED,
                                     Evas_Callback_Type type EINA_UNUSED,
                                     void *event_info EINA_UNUSED)
{
   _evas_gl_preload_main_loop_wakeup();
}

/**
 * @brief Manages the lock and state for the async preloader thread.
 *
 * This function is called from within the async preloader thread. It checks
 * if the main thread has requested a "standby". If so, it releases the GL
 * context, signals the main thread, and waits. When woken up, it re-acquires
 * the GL context before returning. This mechanism allows the main thread to
 * safely perform its own rendering operations.
 *
 * @return EINA_TRUE if the thread can continue, EINA_FALSE if the thread
 *         should exit.
 */
static Eina_Bool
_evas_gl_preload_lock(void)
{
   eina_lock_take(&async_loader_lock);
   if (async_loader_standby)
     {
        async_gl_make_current(async_engine_data, NULL);

        async_loader_running = EINA_FALSE;

        eina_condition_signal(&async_loader_cond);

        eina_condition_wait(&async_loader_cond);
        if (async_loader_exit)
          {
             eina_lock_release(&async_loader_lock);
             return EINA_FALSE;
          }

        async_gl_make_current(async_engine_data, async_engine_data);
     }
   async_loader_running = EINA_TRUE;
   eina_lock_release(&async_loader_lock);

   return EINA_TRUE;
}

/**
 * @brief Main function for the asynchronous texture preloading thread.
 *
 * This thread function runs in the background and processes texture preload
 * requests from a queue (`async_loader_tex`). It takes one request at a time,
 * acquires the GL context, uploads the texture data, and then releases the
 * context. It coordinates with the main rendering thread using locks and
 * conditions to avoid GL context conflicts.
 *
 * @param data Unused thread data.
 * @param t The thread object.
 * @return NULL when the thread finishes.
 */
static void *
_evas_gl_preload_tile_async(void *data EINA_UNUSED, Eina_Thread t EINA_UNUSED)
{
   eina_lock_take(&async_loader_lock);
   while (!async_loader_exit)
     {
        Evas_GL_Texture_Async_Preload *async;
        unsigned int bytes_count;

        if (!async_loader_standby && async_loader_tex)
          goto get_next;

     retry:
        eina_condition_wait(&async_loader_cond);
        if (async_loader_exit) break;

     get_next:
        // Get a texture to upload
        async = eina_list_data_get(async_loader_tex);
        async_loader_tex = eina_list_remove_list(async_loader_tex, async_loader_tex);
        if (!async) continue;

        switch (async->im->cache_entry.space)
          {
           case EVAS_COLORSPACE_ARGB8888: bytes_count = 4; break;
           case EVAS_COLORSPACE_GRY8: bytes_count = 1; break;
           case EVAS_COLORSPACE_AGRY88: bytes_count = 2; break;
           default: continue;
          }

        async_loader_running = EINA_TRUE;
        async_current = async;

        eina_lock_release(&async_loader_lock);

        // Switch context to this thread
        if (!async_gl_make_current(async_engine_data, async_engine_data))
          {
             eina_lock_take(&async_loader_lock);
             async_loader_tex = eina_list_append(async_loader_tex, async_current);
             async_loader_running = EINA_FALSE;
             async_current = NULL;

             if (async_loader_standby)
               eina_condition_signal(&async_loader_cond);

             goto retry;
          }

        // FIXME: loop until all subtile are uploaded or the image is about to be deleted
        evas_gl_common_texture_upload(async->tex, async->im, bytes_count);

        // Shall we block now ?
        if (!_evas_gl_preload_lock())
          break;

        // Release context
        async_gl_make_current(async_engine_data, NULL);

        evas_async_events_put(NULL, 0, NULL, _evas_gl_preload_main_loop_wakeup_cb);

        eina_lock_take(&async_loader_lock);
        async_current = NULL;
        async_loader_todie = eina_list_append(async_loader_todie, async);
        async_loader_running = EINA_FALSE;

        if (async_loader_standby)
          eina_condition_signal(&async_loader_cond);
     }
   eina_lock_release(&async_loader_lock);

   return NULL;
}

// In the main loop
// Push stuff on the todo queue
// Upload the mini texture
// Use the mini texture
// Once download of the big texture, destroy mini texture and image data


/**
 * @brief Pause the async preloader thread to allow rendering in the main thread.
 *
 * This function is called from the main thread before it starts rendering.
 * It signals the async preloader thread to pause if it is currently running
 * (i.e., holding the GL context). It waits until the preloader thread has
 * released the context and entered a standby state. The main thread can then
 * safely acquire the GL context for its own use.
 *
 * @param make_current A function pointer to make the GL context current.
 * @param engine_data The GL engine data.
 */
EMODAPI void
evas_gl_preload_render_lock(evas_gl_make_current_cb make_current, void *engine_data)
{
   if (!async_loader_init) return ;
   eina_lock_take(&async_loader_lock);
   if (async_loader_running)
     {
        async_loader_standby = EINA_TRUE;
        eina_condition_wait(&async_loader_cond);

        make_current(engine_data, engine_data);

        async_engine_data = NULL;
        async_gl_make_current = NULL;
     }

   eina_lock_release(&async_loader_lock);
}

/**
 * @brief Resume the async preloader thread after main thread rendering is complete.
 *
 * This function is called from the main thread after it has finished its
 * rendering and released the GL context. It signals the async preloader
 * thread to resume its operations if there are pending preload requests.
 *
 * @param make_current A function pointer to make the GL context current.
 * @param engine_data The GL engine data.
 */
EMODAPI void
evas_gl_preload_render_unlock(evas_gl_make_current_cb make_current, void *engine_data)
{
   if (!async_loader_init) return ;
   if (!make_current) return ;

   eina_lock_take(&async_loader_lock);
   if (!async_loader_running && (async_loader_tex || async_current))
     {
        make_current(engine_data, NULL);

        async_gl_make_current = make_current;
        async_engine_data = engine_data;

        async_loader_standby = EINA_FALSE;
        eina_condition_signal(&async_loader_cond);
     }
   eina_lock_release(&async_loader_lock);
}

/**
 * @brief Temporarily stop the async preloader if it's using the same engine data.
 *
 * This is a convenience function that simply calls evas_gl_preload_render_lock().
 * It is intended to be used when the main thread needs to interrupt the preloader
 * for a short period. It only acts if the engine_data matches the one used by
 * the preloader.
 *
 * @param make_current A function pointer to make the GL context current.
 * @param engine_data The GL engine data to check against.
 */
EMODAPI void
evas_gl_preload_render_relax(evas_gl_make_current_cb make_current, void *engine_data)
{
   if (engine_data != async_engine_data) return ;

   evas_gl_preload_render_lock(make_current, engine_data);
}

/**
 * @brief Callback function invoked when a target object is deleted.
 *
 * This function is registered as a callback for the EFL_EVENT_DEL event on
 * target objects. When a target object is deleted, this function calls
 * evas_gl_preload_target_unregister() to clean up references to the texture.
 *
 * @param data The Evas_GL_Texture associated with the target.
 * @param event The EFL event information.
 */
static void
_evas_gl_preload_target_die(void *data, const Efl_Event *event)
{
   Evas_GL_Texture *tex = data;

   evas_gl_preload_target_unregister(tex, event->object);
}

/**
 * @brief Register a target object that uses a preloaded texture.
 *
 * This function associates a target object (e.g., an Evas image object)
 * with a preloaded texture. It adds a "DEL" event callback to the target
 * to ensure that resources are cleaned up when the target is deleted. It
 * also increments the texture's reference count.
 *
 * @param tex The preloaded texture.
 * @param target The target object using the texture.
 */
void
evas_gl_preload_target_register(Evas_GL_Texture *tex, Eo *target)
{
   EINA_SAFETY_ON_NULL_RETURN(tex);

   efl_event_callback_add(target, EFL_EVENT_DEL, _evas_gl_preload_target_die, tex);
   tex->targets = eina_list_append(tex->targets, target);
   tex->references++;
}

/**
 * @brief Unregister a target object from a preloaded texture.
 *
 * This function removes the association between a target object and a
 * preloaded texture. It removes the "DEL" event callback and decrements
 * the texture's reference count. When the reference count drops to a
 * certain level, the texture resources can be freed.
 *
 * @param tex The preloaded texture.
 * @param target The target object to unregister.
 */
void
evas_gl_preload_target_unregister(Evas_GL_Texture *tex, Eo *target)
{
   Eina_List *l;
   const Eo *o;

   EINA_SAFETY_ON_NULL_RETURN(tex);

   efl_event_callback_del(target, EFL_EVENT_DEL, _evas_gl_preload_target_die, tex);

   EINA_LIST_FOREACH(tex->targets, l, o)
     if (o == target)
       {
          void *data = async_engine_data;
          evas_gl_make_current_cb cb = async_gl_make_current;
          Eina_Bool running = async_loader_running;

          if (running) evas_gl_preload_render_lock(cb, data);
          tex->targets = eina_list_remove_list(tex->targets, l);
          evas_gl_common_texture_free(tex, EINA_FALSE);
          if (running) evas_gl_preload_render_unlock(cb, data);

          break;
       }
}

/**
 * @brief Initialize the asynchronous texture preloading system.
 *
 * This function sets up the preloading system. It is controlled by the
 * `EVAS_GL_PRELOAD` environment variable. If the variable is set to "1",
 * this function creates the necessary locks, conditions, and the background
 * preloader thread. It uses a reference count to handle multiple init/shutdown
 * calls.
 *
 * @return The current initialization reference count.
 */
EMODAPI int
evas_gl_preload_init(void)
{
   const char *s = getenv("EVAS_GL_PRELOAD");
   if (!s || (atoi(s) != 1)) return 0;
   if (async_loader_init++) return async_loader_init;

   eina_lock_new(&async_loader_lock);
   eina_condition_new(&async_loader_cond, &async_loader_lock);

   if (!eina_thread_create(&async_loader_thread, EINA_THREAD_BACKGROUND, -1, _evas_gl_preload_tile_async, NULL))
     {
        // FIXME: handle error case
     }

   return async_loader_init;
}

/**
 * @brief Shut down the asynchronous texture preloading system.
 *
 * This function cleans up and shuts down the preloading system. It signals
 * the background thread to exit, waits for it to join, and then frees all
 * associated resources (locks, conditions). It decrements the reference
 * count and only performs the shutdown when the count reaches zero.
 *
 * @return The current initialization reference count.
 */
EMODAPI int
evas_gl_preload_shutdown(void)
{
   const char *s = getenv("EVAS_GL_PRELOAD");
   if (!s || (atoi(s) != 1)) return 0;
   if (--async_loader_init) return async_loader_init;

   async_loader_exit = EINA_TRUE;
   eina_condition_signal(&async_loader_cond);

   eina_thread_join(async_loader_thread);

   eina_condition_free(&async_loader_cond);
   eina_lock_free(&async_loader_lock);

   return async_loader_init;
}

/**
 * @brief Check if the asynchronous texture preloading system is enabled.
 *
 * @return EINA_TRUE if the preloading system is initialized and enabled,
 *         EINA_FALSE otherwise.
 */
EMODAPI Eina_Bool
evas_gl_preload_enabled(void)
{
   return (async_loader_init >= 1);
}
