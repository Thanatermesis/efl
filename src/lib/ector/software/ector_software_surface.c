#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Ector.h>
#include <software/Ector_Software.h>

#include "ector_private.h"
#include "ector_software_private.h"

#define MY_CLASS ECTOR_SOFTWARE_SURFACE_CLASS

typedef struct _Ector_Software_Task Ector_Software_Task;

struct _Ector_Software_Task
{
   Eina_Thread_Queue_Msg member; /**< Message queue member for Eina_Thread_Queue. */

   Ector_Thread_Worker_Cb cb;    /**< Worker callback function to be executed. */
   Eina_Free_Cb done;            /**< Callback function to be called when the task is done. */
   void *data;                   /**< Data to be passed to the callbacks. */
};

static int _count_init = 0; /**< Initialization counter for software rendering resources. */
static unsigned int current = 0; /**< Index for the next preparation thread to use. */
static unsigned int cpu_core = 0; /**< Number of CPU cores available for preparation threads. */
static Ector_Software_Thread *ths = NULL; /**< Array of preparation threads. */
static Eina_Thread_Queue *render_queue = NULL; /**< Queue for tasks ready for rendering. */
static Ector_Software_Thread render_thread; /**< Main rendering thread, used when no other cores are available. */

/**
 * @brief Processes tasks from a preparation thread's queue and sends them to the render queue.
 *
 * This function runs in a separate thread. It waits for tasks from its assigned
 * queue, executes the task's worker callback, and then forwards the task
 * to the main render_queue for final processing or notification.
 *
 * @param data Pointer to the Ector_Software_Thread structure for this thread.
 * @param t The Eina_Thread handle for this thread.
 * @return The Ector_Software_Thread pointer passed as data.
 */
static void *
_prepare_process(void *data, Eina_Thread t)
{
   Ector_Software_Thread *th = data;

   eina_thread_name_set(t, "Ector Preparing Thread");
   do
     {
        Ector_Software_Task *task, todo;
        void *ref;

        task = eina_thread_queue_wait(th->queue, &ref);

        if (!task) break ;
        todo.cb = task->cb;
        todo.data = task->data;
        todo.done = task->done;

        eina_thread_queue_wait_done(th->queue, ref);

        if (!todo.cb) break ;

        todo.cb(todo.data, th);

        task = eina_thread_queue_send(render_queue, sizeof (Ector_Software_Task), &ref);
        task->cb = todo.cb;
        task->data = todo.data;
        task->done = todo.done;
        eina_thread_queue_send_done(render_queue, ref);
     }
   while (1);

   return th;
}

/**
 * @brief Initializes the software rendering threads and queues.
 *
 * This function sets up the preparation threads based on the number of CPU cores.
 * If only one core (or less) is detected, it falls back to a single-threaded mode
 * where preparation happens in the main rendering thread.
 * It initializes a render_queue for tasks that have been prepared and are ready
 * for rendering.
 * This function uses a counter (_count_init) to ensure initialization happens only once.
 */
static void
_ector_software_init(void)
{
   int cpu, i;

   ++_count_init;
   if (_count_init != 1) return;

   cpu = eina_cpu_count() - 1;
   if (cpu < 1)
     {
        render_thread.queue = NULL;
        ector_software_thread_init(&render_thread);
        return ;
     }
   cpu = cpu > 8 ? 8 : cpu;
   cpu_core = cpu;

   render_queue = eina_thread_queue_new();

   ths = malloc(sizeof(Ector_Software_Thread) * cpu);
   for (i = 0; i < cpu; i++)
     {
        Ector_Software_Thread *t;

        t = &ths[i];
        t->queue = eina_thread_queue_new();
        ector_software_thread_init(t);
        if (!eina_thread_create(&t->thread, EINA_THREAD_NORMAL, -1,
                                _prepare_process, t))
          {
             eina_thread_queue_free(t->queue);
             t->queue = NULL;
          }
     }
}

/**
 * @brief Shuts down the software rendering threads and queues.
 *
 * This function signals all preparation threads to terminate, waits for them
 * to finish, and then cleans up all associated resources including queues and
 * thread structures.
 * It uses a counter (_count_init) to ensure shutdown happens only when all
 * users have finished.
 */
static void
_ector_software_shutdown(void)
{
   Ector_Software_Thread *t;
   unsigned int i;

   --_count_init;
   if (_count_init != 0) return;

   if (!ths)
     {
        ector_software_thread_shutdown(&render_thread);
        return ;
     }

   for (i = 0; i < cpu_core; i++)
     {
        Ector_Software_Task *task;
        void *ref;

        t = &ths[i];

        task = eina_thread_queue_send(t->queue, sizeof (Ector_Software_Task), &ref);
        task->cb = NULL;
        task->data = NULL;
        eina_thread_queue_send_done(t->queue, ref);

        eina_thread_join(t->thread);
        eina_thread_queue_free(t->queue);
        ector_software_thread_shutdown(t);
     }

   eina_thread_queue_free(render_queue);
   render_queue = NULL;

   free(ths);
   ths = NULL;
}

/**
 * @brief Schedules a task for software processing.
 *
 * If multiple CPU cores are available and threads are initialized (ths != NULL),
 * the task is added to the queue of one of the preparation threads in a round-robin fashion.
 * Otherwise (e.g., single-core system or threads not initialized), the function
 * currently does nothing, implying the task might be handled inline by the caller
 * or a subsequent `ector_software_wait` call.
 *
 * @param cb The worker callback function to execute for the task.
 * @param done The callback function to call when the task is completed.
 * @param data User data to be passed to the callbacks.
 */
void
ector_software_schedule(Ector_Thread_Worker_Cb cb, Eina_Free_Cb done, void *data)
{
   Ector_Software_Thread *t;
   Ector_Software_Task *task;
   void *ref;

   // Not enough CPU, doing it inline in the rendering thread
   if (!ths) return ;

   t = &ths[current];
   current = (current + 1) % cpu_core;

   task = eina_thread_queue_send(t->queue, sizeof (Ector_Software_Task), &ref);
   task->cb = cb;
   task->done = done;
   task->data = data;
   eina_thread_queue_send_done(t->queue, ref);
}

// Do not call this function if the done function has already called
/**
 * @brief Waits for a specific scheduled task to complete.
 *
 * If software threads are not initialized (ths == NULL), it executes the task
 * inline (cb) and then calls the done callback.
 *
 * If threads are initialized, it waits for tasks from the render_queue.
 * For each task retrieved, it calls its 'done' callback. It continues this
 * process until the specific task (identified by the combination of cb, done, and data)
 * is found and its 'done' callback has been invoked.
 *
 * @warning Do not call this function if the 'done' callback for the target task
 *          might have already been called (e.g., by another wait or due to fast processing).
 *
 * @param cb The worker callback of the task to wait for.
 * @param done The 'done' callback of the task to wait for.
 * @param data The user data associated with the task to wait for.
 */
void
ector_software_wait(Ector_Thread_Worker_Cb cb, Eina_Free_Cb done, void *data)
{
   Ector_Software_Task *task, covering;

   // First handle case with just inlined prepare code call inside the rendering thread
   if (!ths)
     {
        render_thread.thread = eina_thread_self();
        cb(data, &render_thread);
        done(data);

        return ;
     }

   // We don't know which task is going to be done first, so
   // we iterate until we find ourself back and trigger all
   // the done call along the way.
   do
     {
        void *ref;

        task = eina_thread_queue_wait(render_queue, &ref);
        if (!task) break;
        covering.cb = task->cb;
        covering.done = task->done;
        covering.data = task->data;
        eina_thread_queue_wait_done(render_queue, ref);

        covering.done(covering.data);
     }
   while (covering.cb != cb ||
          covering.done != done ||
          covering.data != data);
}

/**
 * @brief Factory function to create software renderers.
 *
 * Based on the requested renderer type (mixin), this function creates and
 * returns an instance of the corresponding software renderer (e.g., shape, image, gradient).
 * The newly created renderer is associated with the provided Ector_Surface object.
 *
 * @param obj The Ector_Surface object for which to create the renderer.
 * @param pd Private data of the Ector_Software_Surface. Not used in this function.
 * @param type The Efl_Class of the renderer mixin type to create (e.g., ECTOR_RENDERER_SHAPE_MIXIN).
 * @return A new Ector_Renderer instance on success, or NULL on failure (e.g., unknown type).
 *         The returned renderer has its reference count incremented.
 */
static Ector_Renderer *
_ector_software_surface_ector_surface_renderer_factory_new(Eo *obj,
                                                           Ector_Software_Surface_Data *pd EINA_UNUSED,
                                                           const Efl_Class *type)
{
   if (type == ECTOR_RENDERER_SHAPE_MIXIN)
     return efl_add_ref(ECTOR_RENDERER_SOFTWARE_SHAPE_CLASS, NULL, ector_renderer_surface_set(efl_added, obj));
   else if (type == ECTOR_RENDERER_IMAGE_MIXIN)
     return efl_add_ref(ECTOR_RENDERER_SOFTWARE_IMAGE_CLASS, NULL, ector_renderer_surface_set(efl_added, obj));
   else if (type == ECTOR_RENDERER_GRADIENT_LINEAR_MIXIN)
     return efl_add_ref(ECTOR_RENDERER_SOFTWARE_GRADIENT_LINEAR_CLASS, NULL, ector_renderer_surface_set(efl_added, obj));
   else if (type == ECTOR_RENDERER_GRADIENT_RADIAL_MIXIN)
     return efl_add_ref(ECTOR_RENDERER_SOFTWARE_GRADIENT_RADIAL_CLASS, NULL, ector_renderer_surface_set(efl_added, obj));

   ERR("Couldn't find class for type: %s", efl_class_name_get(type));
   return NULL;
}

/**
 * @brief Constructor for Ector_Software_Surface objects.
 *
 * Initializes software rendering resources via _ector_software_init().
 * Calls the parent class constructor.
 * Allocates and initializes a Software_Rasterizer for this surface.
 * References the surface's underlying software buffer for the rasterizer.
 *
 * @param obj The Eo object being constructed.
 * @param pd Pointer to the private data structure for this Ector_Software_Surface.
 * @return The constructed Eo object.
 */
static Eo *
_ector_software_surface_efl_object_constructor(Eo *obj, Ector_Software_Surface_Data *pd)
{
   _ector_software_init();

   obj = efl_constructor(efl_super(obj, MY_CLASS));
   pd->rasterizer = (Software_Rasterizer *) calloc(1, sizeof(Software_Rasterizer));
   ector_software_rasterizer_init(pd->rasterizer);
   pd->rasterizer->fill_data.raster_buffer = efl_data_ref(obj, ECTOR_SOFTWARE_BUFFER_BASE_MIXIN);
   return obj;
}

/**
 * @brief Destructor for Ector_Software_Surface objects.
 *
 * Unreferences the software buffer used by the rasterizer.
 * Frees the Software_Rasterizer.
 * Calls the parent class destructor.
 * Shuts down software rendering resources via _ector_software_shutdown().
 *
 * @param obj The Eo object being destructed.
 * @param pd Pointer to the private data structure for this Ector_Software_Surface.
 */
static void
_ector_software_surface_efl_object_destructor(Eo *obj, Ector_Software_Surface_Data *pd)
{
   efl_data_unref(obj, pd->rasterizer->fill_data.raster_buffer);
   free(pd->rasterizer);
   pd->rasterizer = NULL;
   efl_destructor(efl_super(obj, ECTOR_SOFTWARE_SURFACE_CLASS));

   _ector_software_shutdown();
}

/**
 * @brief Gets the reference point of the Ector software surface.
 *
 * The reference point is an offset used for drawing operations.
 *
 * @param obj The Ector_Software_Surface object. Not used in this function.
 * @param pd Private data of the Ector_Software_Surface.
 * @param x Pointer to store the x-coordinate of the reference point. Can be NULL.
 * @param y Pointer to store the y-coordinate of the reference point. Can be NULL.
 */
static void
_ector_software_surface_ector_surface_reference_point_get(const Eo *obj EINA_UNUSED,
                                                          Ector_Software_Surface_Data *pd,
                                                          int* x, int* y)
{
   if (x) *x = pd->x;
   if (y) *y = pd->y;
}

/**
 * @brief Sets the reference point of the Ector software surface.
 *
 * The reference point is an offset used for drawing operations.
 *
 * @param obj The Ector_Software_Surface object. Not used in this function.
 * @param pd Private data of the Ector_Software_Surface.
 * @param x The x-coordinate of the reference point.
 * @param y The y-coordinate of the reference point.
 */
static void
_ector_software_surface_ector_surface_reference_point_set(Eo *obj EINA_UNUSED,
                                                          Ector_Software_Surface_Data *pd,
                                                          int x, int y)
{
   pd->x = x;
   pd->y = y;
}

/**
 * @brief Draws an image (Ector_Buffer) onto the Ector software surface.
 *
 * This function performs a software blit of the source buffer onto the
 * destination surface's buffer, applying an overall alpha value.
 * The blending formula used is: dst = (src * alpha/256) + (dst * (255 - src_alpha)/256),
 * where src_alpha is the alpha component of the pre-multiplied source pixel.
 *
 * @param obj The Ector_Software_Surface object. Not used in this function.
 * @param pd Private data of the Ector_Software_Surface, containing the rasterizer and destination buffer.
 * @param buffer The source Ector_Buffer to draw.
 * @param x The x-coordinate on the destination surface to draw the top-left of the image.
 * @param y The y-coordinate on the destination surface to draw the top-left of the image.
 * @param alpha An overall alpha value (0-255) to apply to the source image during drawing.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., invalid buffers).
 */
static Eina_Bool
_ector_software_surface_ector_surface_draw_image(Eo *obj EINA_UNUSED,
                                                 Ector_Software_Surface_Data *pd,
                                                 Ector_Buffer *buffer, int x, int y, int alpha)
{
   if (!buffer || !pd->rasterizer || !pd->rasterizer->fill_data.raster_buffer->pixels.u32)
     return EINA_FALSE;

   Ector_Software_Buffer_Base_Data *bd = efl_data_scope_get(buffer, ECTOR_SOFTWARE_BUFFER_BASE_MIXIN);
   const int pix_stride = pd->rasterizer->fill_data.raster_buffer->stride / 4;

   uint32_t *src = bd->pixels.u32;
   if (!src) return EINA_FALSE;

   for (unsigned int local_y = 0; local_y <  bd->generic->h; local_y++)
     {
        uint32_t *dst = pd->rasterizer->fill_data.raster_buffer->pixels.u32 + (x + ((local_y + y) * pix_stride));
        for (unsigned int local_x = 0; local_x <  bd->generic->w; local_x++)
          {
             *src = draw_mul_256(alpha, *src);
             int inv_alpha = 255 - ((*src) >> 24);
             *dst = *src + draw_mul_256(inv_alpha, *dst);
             dst++;
             src++;
          }
     }
   return EINA_TRUE;
}
#include "ector_software_surface.eo.c"
#include "ector_renderer_software.eo.c"
