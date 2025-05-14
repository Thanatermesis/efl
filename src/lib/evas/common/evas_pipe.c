#include "evas_common_private.h"
#include <unistd.h>

#ifdef BUILD_PIPE_RENDER

/**
 * @brief Structure to hold thread-specific information for pipe rendering.
 */
typedef struct _Thinfo
{
   RGBA_Image            *im; /**< The image being processed by this thread. */
   int                    thread_num; /**< Identifier for this thread. */
   Eina_Thread            thread_id; /**< The Eina thread identifier. */
   Eina_Barrier          *barrier; /**< Barrier for synchronization with other threads. */
   const Eina_Inlist     *tasks; /**< List of rendering tasks assigned to this thread.
                                   *   Each element is a RGBA_Pipe_Thread_Info. */
   Eina_Array             cutout_trash; /**< Array for recycling Cutout_Rects structures.
                                         *   Elements are (Cutout_Rects *). */
   Eina_Array             rects_task; /**< Array of pipe operations (RGBA_Pipe_Op *) that need preparation. */
} Thinfo;

static RGBA_Pipe *evas_common_pipe_add(RGBA_Pipe *pipe, RGBA_Pipe_Op **op);
static void evas_common_pipe_draw_context_copy(RGBA_Draw_Context *dc, RGBA_Pipe_Op *op);
static void evas_common_pipe_op_free(RGBA_Pipe_Op *op);

/* utils */

/**
 * @brief Adds a new operation to the rendering pipe.
 *
 * If rpipe is NULL, a new pipe is created. If the current pipe segment
 * is full, a new segment is allocated and appended.
 *
 * @param rpipe The existing rendering pipe, or NULL to create a new one.
 * @param op Pointer to store the newly added pipe operation.
 * @return The (potentially new) head of the rendering pipe list.
 */
static RGBA_Pipe *
evas_common_pipe_add(RGBA_Pipe *rpipe, RGBA_Pipe_Op **op)
{
   RGBA_Pipe *p;
   int first_pipe = 0;

   if (!rpipe)
     {
        first_pipe = 1;
        p = calloc(1, sizeof(RGBA_Pipe));
        if (!p) return NULL;
        rpipe = (RGBA_Pipe *)eina_inlist_append(EINA_INLIST_GET(rpipe), EINA_INLIST_GET(p));
     }
   p = (RGBA_Pipe *)(EINA_INLIST_GET(rpipe))->last;
   if (p->op_num == PIPE_LEN)
     {
        p = calloc(1, sizeof(RGBA_Pipe));
        if (!p) return NULL;
        rpipe = (RGBA_Pipe *)eina_inlist_append(EINA_INLIST_GET(rpipe), EINA_INLIST_GET(p));
     }
   p->op_num++;
   *op = &(p->op[p->op_num - 1]);
   if (first_pipe)
     {
        /* FIXME: PTHREAD init any thread locks etc */
     }
   return rpipe;
}

/**
 * @brief Copies the draw context into a pipe operation.
 *
 * This function performs a deep copy of the draw context, including
 * allocating and copying cutout rectangles if they are active.
 *
 * @param dc The source RGBA_Draw_Context to copy.
 * @param op The destination RGBA_Pipe_Op where the context will be stored.
 */
static void
evas_common_pipe_draw_context_copy(RGBA_Draw_Context *dc, RGBA_Pipe_Op *op)
{
   memcpy(&(op->context), dc, sizeof(RGBA_Draw_Context));
   if (op->context.cutout.active > 0)
     {
        op->context.cutout.rects = malloc(sizeof(Cutout_Rect) * op->context.cutout.active);
        memcpy(op->context.cutout.rects, dc->cutout.rects, sizeof(Cutout_Rect) * op->context.cutout.active);
     }
   else
     {
        op->context.cutout.rects = NULL;
     }
}

/**
 * @brief Frees resources associated with a pipe operation's context.
 *
 * Specifically, this cleans up cutout rectangles within the operation's context.
 *
 * @param op The RGBA_Pipe_Op whose context resources are to be freed.
 */
static void
evas_common_pipe_op_free(RGBA_Pipe_Op *op)
{
   evas_common_draw_context_apply_clean_cutouts(&op->context.cutout);
}

/* main api calls */

/**
 * @brief Main function for a rendering worker thread.
 *
 * This thread waits for tasks, processes them by iterating through pipe operations,
 * and then waits for the next set of tasks.
 *
 * @param data Pointer to Thinfo structure for this thread.
 * @param t The Eina_Thread handle (unused).
 * @return NULL always.
 */
static void *
evas_common_pipe_thread(void *data, Eina_Thread t EINA_UNUSED)
{
   Thinfo *thinfo;

   eina_thread_name_set(eina_thread_self(), "Ecore-pipe");
   // INF("TH [...........");
   thinfo = data;
   for (;;)
     {
        const RGBA_Pipe_Thread_Info *info;
        RGBA_Pipe *p;

        /* wait for start signal */
// INF(" TH %i START...", thinfo->thread_num);
        eina_barrier_wait(&(thinfo->barrier[0]));

        EINA_INLIST_FOREACH(thinfo->tasks, info)
          {
             EINA_INLIST_FOREACH(EINA_INLIST_GET(thinfo->im->cache_entry.pipe), p)
               {
                  int i;

                  for (i = 0; i < p->op_num; i++)
                    {
                       if (p->op[i].op_func && p->op[i].render)
                         p->op[i].op_func(thinfo->im, &(p->op[i]), info);
                    }
               }
          }

        thinfo->tasks = NULL;

        eina_barrier_wait(&(thinfo->barrier[1]));
     }
   return NULL;
}

static Eina_List *im_task = NULL;
static Eina_List *text_task = NULL;
static Thinfo task_thinfo[TH_MAX];
static Eina_Barrier task_thbarrier[2];
static LK(im_task_mutex);
static LK(text_task_mutex);

static int               thread_num = 0;
static Thinfo            thinfo[TH_MAX];
static Eina_Barrier      thbarrier[2];

static RGBA_Pipe_Thread_Info *buf = NULL; /**< Buffer for RGBA_Pipe_Thread_Info structures. */
static unsigned int           buf_size = 0; /**< Current allocated size of buf, in number of elements. */

/**
 * @brief Pops a Cutout_Rects structure from the thread's trash array.
 *
 * If the trash is empty, a new Cutout_Rects structure is allocated.
 * This is used for recycling Cutout_Rects to reduce allocations.
 *
 * @param info The thread information structure containing the cutout_trash.
 * @return A pointer to a Cutout_Rects structure.
 */
static Cutout_Rects *
evas_pipe_cutout_rects_pop(Thinfo *info)
{
   Cutout_Rects *r;

   r = eina_array_pop(&info->cutout_trash);
   if (!r) r = evas_common_draw_context_cutouts_new();
   return r;
}

/**
 * @brief Pushes a Cutout_Rects structure back into the thread's trash array.
 *
 * The Cutout_Rects structure is freed before being pushed for recycling.
 *
 * @param info The thread information structure containing the cutout_trash.
 * @param r The Cutout_Rects structure to recycle.
 */
static void
evas_pipe_cutout_rects_push(Thinfo *info, Cutout_Rects *r)
{
   /* evas_common_draw_context_apply_clean_cutouts(r); */
   evas_common_draw_context_cutouts_free(r);
   eina_array_push(&info->cutout_trash, r);
}

/**
 * @brief Rotates a Cutout_Rects structure to a worker thread's trash for recycling.
 *
 * This distributes the responsibility of recycling cutouts among worker threads.
 *
 * @param r The Cutout_Rects structure to recycle.
 */
static void
evas_pipe_cutout_rects_rotate(Cutout_Rects *r)
{
   static int current = 0;

   if (current >= thread_num) current = 0;
   evas_pipe_cutout_rects_push(&task_thinfo[current], r);
   current++;
}

/**
 * @brief Pushes a pipe operation to a worker thread's task list for preparation.
 *
 * This distributes pipe operation preparation tasks among worker threads.
 *
 * @param op The RGBA_Pipe_Op to be prepared.
 */
static void
evas_pipe_prepare_push(RGBA_Pipe_Op *op)
{
   static int current = 0;

   if (current >= thread_num) current = 0;
   eina_array_push(&task_thinfo[current].rects_task, op);
   current++;
}

/**
 * @brief Prepares an image for threaded pipe rendering.
 *
 * Divides the image into smaller rectangular tasks and assigns them to worker threads.
 * If only one thread is configured, this function does nothing.
 *
 * @param im The RGBA_Image to prepare for rendering.
 */
static void
evas_common_pipe_begin(RGBA_Image *im)
{
#define SZ 128
   unsigned int x, y, cpu;
   RGBA_Pipe_Thread_Info *info;
   unsigned int estimatex, estimatey;
   unsigned int needed_size;

   if (!im->cache_entry.pipe) return;
   if (thread_num == 1) return;

   if (im->cache_entry.w * im->cache_entry.h / thread_num < SZ * SZ)
     {
        estimatex = im->cache_entry.w;
        estimatey = im->cache_entry.h / thread_num;
        if (estimatey == 0) estimatey = 1;
     }
   else
     {
        estimatex = SZ;
        estimatey = SZ;
     }

   needed_size = ((im->cache_entry.w / estimatex) + 1 ) * ((im->cache_entry.h /  estimatey) + 1);
   if (buf_size < needed_size)
     {
        RGBA_Pipe_Thread_Info *temp;

        temp = realloc(buf, sizeof (RGBA_Pipe_Thread_Info) * needed_size);
        if (temp)
          {
             buf = temp;
             buf_size = needed_size;
          }
     }

   info = buf;
   cpu = 0;
   for (y = 0; y < im->cache_entry.h; y += estimatey)
     for (x = 0; x < im->cache_entry.w; x += estimatex)
       {
          EINA_RECTANGLE_SET(&info->area, x, y,
                             (x + estimatex > im->cache_entry.w) ? im->cache_entry.w - x : estimatex,
                             (y + estimatey > im->cache_entry.h) ? im->cache_entry.h - y : estimatey);

	  thinfo[cpu].im = im;
          thinfo[cpu].tasks = eina_inlist_prepend((void*) thinfo[cpu].tasks, EINA_INLIST_GET(info));
          cpu++;
          if (cpu >= (unsigned int) thread_num) cpu = 0;

          info++;
       }

   /* tell worker threads to start */
   eina_barrier_wait(&(thbarrier[0]));
}

EVAS_API void
evas_common_pipe_flush(RGBA_Image *im)
{
   if (!im->cache_entry.pipe) return;
   if (thread_num > 1)
     {
       /* sync worker threads */
       eina_barrier_wait(&(thbarrier[1]));
     }
   else
     {
        RGBA_Pipe_Thread_Info info;
        RGBA_Pipe *p;
        int i;

        EINA_RECTANGLE_SET(&info.area, 0, 0, im->cache_entry.w, im->cache_entry.h);

        /* process pipe - 1 thead */
        for (p = im->cache_entry.pipe; p; p = (RGBA_Pipe *)(EINA_INLIST_GET(p))->next)
          {
             for (i = 0; i < p->op_num; i++)
               {
                  if (p->op[i].render && p->op[i].op_func)
                    {
                       p->op[i].op_func(im, &(p->op[i]), &info);
                    }
               }
          }
     }

   evas_common_cpu_end_opt();
   evas_common_pipe_free(im);
}

EVAS_API void
evas_common_pipe_free(RGBA_Image *im)
{

   RGBA_Pipe *p;
   int i;

   if (!im->cache_entry.pipe) return;
   /* FIXME: PTHREAD join all threads here (if not finished) */

   /* free pipe */
   while (im->cache_entry.pipe)
     {
        p = im->cache_entry.pipe;
        for (i = 0; i < p->op_num; i++)
          {
             if (p->op[i].free_func)
               {
                  p->op[i].free_func(&(p->op[i]));
               }
             if (p->op[i].rects) evas_pipe_cutout_rects_rotate(p->op[i].rects);
          }
        im->cache_entry.pipe = (RGBA_Pipe *)eina_inlist_remove(EINA_INLIST_GET(im->cache_entry.pipe), EINA_INLIST_GET(p));
        free(p);
     }
}



/* draw ops */
/**************** RECT ******************/

/**
 * @brief Executes the rectangle drawing operation for a specific thread and area.
 *
 * This function is called by a worker thread to draw a part of a rectangle.
 *
 * @param dst The destination image.
 * @param op The pipe operation containing rectangle parameters and context.
 * @param info Thread-specific information, including the area to render.
 */
static void
evas_common_pipe_rectangle_draw_do(RGBA_Image *dst, const RGBA_Pipe_Op *op, const RGBA_Pipe_Thread_Info *info)
{
   RGBA_Draw_Context context;

   memcpy(&(context), &(op->context), sizeof(RGBA_Draw_Context));
   evas_common_rectangle_draw_do(op->rects, &info->area, dst, &(context),
				 op->op.rect.x, op->op.rect.y,
				 op->op.rect.w, op->op.rect.h);
}

/**
 * @brief Prepares a rectangle drawing operation.
 *
 * This involves calculating cutout rectangles for the operation.
 *
 * @param data Pointer to Thinfo for the current thread (used for recycling cutouts).
 * @param dst The destination image.
 * @param op The pipe operation for the rectangle.
 * @return EINA_TRUE if rendering is needed, EINA_FALSE otherwise.
 */
static Eina_Bool
evas_common_pipe_rectangle_prepare(void *data, RGBA_Image *dst, RGBA_Pipe_Op *op)
{
   Cutout_Rects *recycle;
   Thinfo *info = data;
   Eina_Bool r;

   recycle = evas_pipe_cutout_rects_pop(info);
   r = evas_common_rectangle_draw_prepare(&recycle, dst, &(op->context),
                                          op->op.rect.x, op->op.rect.y,
                                          op->op.rect.w, op->op.rect.h);
   if (recycle->active) op->rects = recycle;
   else evas_pipe_cutout_rects_push(info, recycle);

   return r;
}

EVAS_API void
evas_common_pipe_rectangle_draw(RGBA_Image *dst, RGBA_Draw_Context *dc, int x, int y, int w, int h)
{
   RGBA_Pipe_Op *op;

   if ((w < 1) || (h < 1)) return;
   dst->cache_entry.pipe = evas_common_pipe_add(dst->cache_entry.pipe, &op);
   if (!dst->cache_entry.pipe) return;
   op->op.rect.x = x;
   op->op.rect.y = y;
   op->op.rect.w = w;
   op->op.rect.h = h;
   op->op_func = evas_common_pipe_rectangle_draw_do;
   op->free_func = evas_common_pipe_op_free;
   op->prepare_func = evas_common_pipe_rectangle_prepare;
   evas_pipe_prepare_push(op);
   evas_common_pipe_draw_context_copy(dc, op);
}

/**************** LINE ******************/

/**
 * @brief Executes the line drawing operation for a specific thread and area.
 *
 * This function is called by a worker thread to draw a part of a line.
 * The context is clipped to the thread's assigned area.
 *
 * @param dst The destination image.
 * @param op The pipe operation containing line parameters and context.
 * @param info Thread-specific information, including the area to render and clip.
 */
static void
evas_common_pipe_line_draw_do(RGBA_Image *dst, const RGBA_Pipe_Op *op, const RGBA_Pipe_Thread_Info *info)
{
   RGBA_Draw_Context context;

   memcpy(&(context), &(op->context), sizeof(RGBA_Draw_Context));
   evas_common_draw_context_clip_clip(&(context), info->area.x, info->area.y, info->area.w, info->area.h);
   evas_common_line_draw(dst, &(context),
                         op->op.line.x0, op->op.line.y0,
                         op->op.line.x1, op->op.line.y1);
}

EVAS_API void
evas_common_pipe_line_draw(RGBA_Image *dst, RGBA_Draw_Context *dc,
                           int x0, int y0, int x1, int y1)
{
   RGBA_Pipe_Op *op;

   dst->cache_entry.pipe = evas_common_pipe_add(dst->cache_entry.pipe, &op);
   if (!dst->cache_entry.pipe) return;
   op->op.line.x0 = x0;
   op->op.line.y0 = y0;
   op->op.line.x1 = x1;
   op->op.line.y1 = y1;
   op->op_func = evas_common_pipe_line_draw_do;
   op->free_func = evas_common_pipe_op_free;
   op->prepare_func = NULL;
   op->render = EINA_TRUE;
   evas_common_pipe_draw_context_copy(dc, op);
}

/**************** POLY ******************/

/**
 * @brief Frees resources specific to a polygon pipe operation.
 *
 * Currently, this function is a no-op for polygon points as they are not
 * deep-copied into the pipe operation but might be in the future.
 * It calls the generic pipe operation free function.
 *
 * @param op The polygon pipe operation.
 */
static void
evas_common_pipe_op_poly_free(RGBA_Pipe_Op *op)
{
#if 0 // FIXME: Points are not copied currently, so no need to free them here.
   RGBA_Polygon_Point *p;

   while (op->op.poly.points)
     {
        p = op->op.poly.points;
        op->op.poly.points = (RGBA_Polygon_Point *)eina_inlist_remove(EINA_INLIST_GET(op->op.poly.points),
                                                      EINA_INLIST_GET(p));
        free(p);
     }
#endif
   evas_common_pipe_op_free(op);
}

/**
 * @brief Executes the polygon drawing operation for a specific thread and area.
 *
 * This function is called by a worker thread to draw a part of a polygon.
 * The context is clipped to the thread's assigned area.
 *
 * @param dst The destination image.
 * @param op The pipe operation containing polygon parameters and context.
 * @param info Thread-specific information, including the area to render and clip.
 */
static void
evas_common_pipe_poly_draw_do(RGBA_Image *dst, const RGBA_Pipe_Op *op, const RGBA_Pipe_Thread_Info *info)
{
   RGBA_Draw_Context context;

   memcpy(&(context), &(op->context), sizeof(RGBA_Draw_Context));
   evas_common_draw_context_clip_clip(&(context), info->area.x, info->area.y, info->area.w, info->area.h);
   evas_common_polygon_draw(dst, &(context),
                            op->op.poly.points, op->op.poly.x, op->op.poly.y);
}

EVAS_API void
evas_common_pipe_poly_draw(RGBA_Image *dst, RGBA_Draw_Context *dc,
                           RGBA_Polygon_Point *points, int x, int y)
{
   RGBA_Pipe_Op *op;
   /* RGBA_Polygon_Point *pts = NULL, *p, *pp; */

   if (!points) return;
   dst->cache_entry.pipe = evas_common_pipe_add(dst->cache_entry.pipe, &op);
   if (!dst->cache_entry.pipe) return;
   /* FIXME: copy points - maybe we should refcount? */
#if 0
   for (p = points; p; p = (RGBA_Polygon_Point *)(EINA_INLIST_GET(p))->next)
     {
        pp = calloc(1, sizeof(RGBA_Polygon_Point));
        if (pp)
          {
             pp->x = p->x + x;
             pp->y = p->y + y;
             pts = (RGBA_Polygon_Point *)eina_inlist_append(EINA_INLIST_GET(pts), EINA_INLIST_GET(pp));
          }
     }
#endif
   op->op.poly.x = x;
   op->op.poly.y = y;
   op->op.poly.points = points/* pts */;
   op->op_func = evas_common_pipe_poly_draw_do;
   op->free_func = evas_common_pipe_op_poly_free;
   op->render = EINA_TRUE;
   op->prepare_func = NULL; /* FIXME: If we really want to improve it, we should prepare span for it here */
   evas_common_pipe_draw_context_copy(dc, op);
}

/**************** TEXT ******************/

/**
 * @brief Frees resources specific to a text pipe operation.
 *
 * This unreferences the international text properties and calls the generic
 * pipe operation free function.
 *
 * @param op The text pipe operation.
 */
static void
evas_common_pipe_op_text_free(RGBA_Pipe_Op *op)
{
   evas_common_text_props_content_unref(op->op.text.intl_props);
   evas_common_pipe_op_free(op);
}

/**
 * @brief Executes the text drawing operation for a specific thread and area.
 *
 * This function is called by a worker thread to draw a part of a text string.
 *
 * @param dst The destination image.
 * @param op The pipe operation containing text parameters, properties, and context.
 * @param info Thread-specific information, including the area to render.
 */
static void
evas_common_pipe_text_draw_do(RGBA_Image *dst, const RGBA_Pipe_Op *op, const RGBA_Pipe_Thread_Info *info)
{
   RGBA_Draw_Context context;

   memcpy(&(context), &(op->context), sizeof(RGBA_Draw_Context));
   evas_common_font_draw_do(op->rects, &info->area, op->op.text.func, dst, &(context), op->op.text.x, op->op.text.y, op->op.text.intl_props);
}

/**
 * @brief Prepares a text drawing operation.
 *
 * This involves calculating cutout rectangles for the text rendering.
 *
 * @param data Pointer to Thinfo for the current thread (used for recycling cutouts).
 * @param dst The destination image.
 * @param op The pipe operation for the text.
 * @return EINA_TRUE if rendering is needed, EINA_FALSE otherwise.
 */
static Eina_Bool
evas_common_pipe_text_draw_prepare(void *data, RGBA_Image *dst, RGBA_Pipe_Op *op)
{
   Cutout_Rects *recycle;
   Thinfo *info = data;
   Eina_Bool r;

   recycle = evas_pipe_cutout_rects_pop(info);
   r = evas_common_font_draw_prepare_cutout(&recycle, dst, &(op->context),
					    &(op->op.text.func));
   if (recycle->active) op->rects = recycle;
   else evas_pipe_cutout_rects_push(info, recycle);

   return r;
}

EVAS_API void
evas_common_pipe_text_draw(RGBA_Image *dst, RGBA_Draw_Context *dc,
			   int x, int y, Evas_Text_Props *intl_props)
{
   RGBA_Pipe_Op *op;

   dst->cache_entry.pipe = evas_common_pipe_add(dst->cache_entry.pipe, &op);
   if (!dst->cache_entry.pipe) return;
   op->op.text.x = x;
   op->op.text.y = y;
   op->op.text.intl_props = intl_props;
   evas_common_text_props_content_ref(intl_props);
   op->op_func = evas_common_pipe_text_draw_do;
   op->free_func = evas_common_pipe_op_text_free;
   op->prepare_func = evas_common_pipe_text_draw_prepare;
   evas_pipe_prepare_push(op);
   evas_common_pipe_draw_context_copy(dc, op);
   evas_common_pipe_text_prepare(intl_props);
}

/**************** IMAGE *****************/

/**
 * @brief Frees resources specific to an image pipe operation.
 *
 * This decrements the reference count of the source image and drops it
 * from the cache if the reference count reaches zero. It then calls the
 * generic pipe operation free function.
 *
 * @param op The image pipe operation.
 */
static void
evas_common_pipe_op_image_free(RGBA_Pipe_Op *op)
{
   op->op.image.src->ref--;
   if (op->op.image.src->ref == 0)
     {
        evas_cache_image_drop(&op->op.image.src->cache_entry);
     }
   evas_common_pipe_op_free(op);
}

/**
 * @brief Prepares an image drawing (scaling) operation.
 *
 * This involves calculating cutout rectangles for the scaled image rendering.
 *
 * @param data Pointer to Thinfo for the current thread (used for recycling cutouts).
 * @param dst The destination image.
 * @param op The pipe operation for the image.
 * @return EINA_TRUE if rendering is needed, EINA_FALSE otherwise.
 */
static Eina_Bool
evas_common_pipe_op_image_prepare(void *data, RGBA_Image *dst, RGBA_Pipe_Op *op)
{
   Cutout_Rects *recycle;
   Thinfo *info = data;
   Eina_Bool r;

   recycle = evas_pipe_cutout_rects_pop(info);
   r = evas_common_scale_rgba_in_to_out_clip_prepare(&recycle,
						     op->op.image.src, dst,
						     &(op->context),
                                                     op->op.image.dx, op->op.image.dy,
                                                     op->op.image.dw, op->op.image.dh);
   if (recycle->active) op->rects = recycle;
   else evas_pipe_cutout_rects_push(info, recycle);

   return r;
}

/**
 * @brief Executes the image drawing (scaling) operation for a specific thread and area.
 *
 * This function is called by a worker thread to draw a part of a scaled image.
 * It handles both smooth and sampled scaling.
 *
 * @param dst The destination image.
 * @param op The pipe operation containing image scaling parameters and context.
 * @param info Thread-specific information, including the area to render.
 */
static void
evas_common_pipe_image_draw_do(RGBA_Image *dst, const RGBA_Pipe_Op *op, const RGBA_Pipe_Thread_Info *info)
{
   RGBA_Draw_Context context;

   memcpy(&(context), &(op->context), sizeof(RGBA_Draw_Context));

#ifdef SCALECACHE
   /* FIXME: Make the scalecache path use the prepared Cutout ? */
   evas_common_draw_context_clip_clip(&(context), info->area.x, info->area.y, info->area.w, info->area.h);
   evas_common_rgba_image_scalecache_do((Image_Entry *)(op->op.image.src),
                                        dst, &(context),
                                        op->op.image.smooth,
                                        op->op.image.sx,
                                        op->op.image.sy,
                                        op->op.image.sw,
                                        op->op.image.sh,
                                        op->op.image.dx,
                                        op->op.image.dy,
                                        op->op.image.dw,
                                        op->op.image.dh);
#else
   if (op->op.image.smooth)
     {
        evas_common_scale_rgba_in_to_out_clip_smooth_do(op->rects, &info->area,
                                                        op->op.image.src,
                                                        dst, &(context),
                                                        op->op.image.sx,
                                                        op->op.image.sy,
                                                        op->op.image.sw,
                                                        op->op.image.sh,
                                                        op->op.image.dx,
                                                        op->op.image.dy,
                                                        op->op.image.dw,
                                                        op->op.image.dh);
     }
   else
     {
        evas_common_scale_rgba_in_to_out_clip_sample_do(op->rects, &info->area,
                                                        op->op.image.src,
                                                        dst, &(context),
                                                        op->op.image.sx,
                                                        op->op.image.sy,
                                                        op->op.image.sw,
                                                        op->op.image.sh,
                                                        op->op.image.dx,
                                                        op->op.image.dy,
                                                        op->op.image.dw,
                                                        op->op.image.dh);
     }
#endif
}

EVAS_API void
evas_common_pipe_image_draw(RGBA_Image *src, RGBA_Image *dst,
			   RGBA_Draw_Context *dc, int smooth,
			   int src_region_x, int src_region_y,
			   int src_region_w, int src_region_h,
			   int dst_region_x, int dst_region_y,
			   int dst_region_w, int dst_region_h)
{
   RGBA_Pipe_Op *op;

   if (!src) return;
//   evas_common_pipe_flush(src);
   dst->cache_entry.pipe = evas_common_pipe_add(dst->cache_entry.pipe, &op);
   if (!dst->cache_entry.pipe) return;
   op->op.image.smooth = smooth;
   op->op.image.sx = src_region_x;
   op->op.image.sy = src_region_y;
   op->op.image.sw = src_region_w;
   op->op.image.sh = src_region_h;
   op->op.image.dx = dst_region_x;
   op->op.image.dy = dst_region_y;
   op->op.image.dw = dst_region_w;
   op->op.image.dh = dst_region_h;
   src->ref++;
   op->op.image.src = src;
   op->op_func = evas_common_pipe_image_draw_do;
   op->free_func = evas_common_pipe_op_image_free;
   op->prepare_func = evas_common_pipe_op_image_prepare;
   evas_pipe_prepare_push(op);
   evas_common_pipe_draw_context_copy(dc, op);

   evas_common_pipe_image_load(src);
}

/**
 * @brief Frees resources specific to a map pipe operation.
 *
 * This decrements the reference count of the source image and drops it
 * from the cache if the reference count reaches zero. It then calls the
 * generic pipe operation free function.
 *
 * @param op The map pipe operation.
 */
static void
evas_common_pipe_op_map_free(RGBA_Pipe_Op *op)
{
   op->op.map.src->ref--;
   if (op->op.map.src->ref == 0)
     evas_cache_image_drop(&op->op.map.src->cache_entry);
   /* free(op->op.map.p); */
   evas_common_pipe_op_free(op);
}

/**
 * @brief Executes the map rendering operation for a specific thread and area.
 *
 * This function is called by a worker thread to render a part of a mapped image.
 *
 * @param dst The destination image.
 * @param op The pipe operation containing map parameters and context.
 * @param info Thread-specific information, including the area to render.
 */
static void
evas_common_pipe_map_draw_do(RGBA_Image *dst, const RGBA_Pipe_Op *op, const RGBA_Pipe_Thread_Info *info)
{
   RGBA_Draw_Context context;

   memcpy(&(context), &(op->context), sizeof(RGBA_Draw_Context));
   evas_common_map_rgba_do(&info->area, op->op.map.src, dst,
                           &context, op->op.map.m,
                           op->op.map.smooth, op->op.map.anti_alias,
                           op->op.map.level);
}

/**
 * @brief Prepares a map rendering operation.
 *
 * This function checks if the map operation is valid and can be rendered.
 *
 * @param data Unused.
 * @param dst The destination image.
 * @param op The pipe operation for the map.
 * @return EINA_TRUE if rendering is needed, EINA_FALSE otherwise.
 */
static Eina_Bool
evas_common_pipe_map_draw_prepare(void *data EINA_UNUSED, RGBA_Image *dst, RGBA_Pipe_Op *op)
{
   RGBA_Draw_Context context;
   Eina_Bool r;

   memcpy(&(context), &(op->context), sizeof(RGBA_Draw_Context));
   r = evas_common_map_rgba_prepare(op->op.map.src, dst,
				    &context, op->op.map.m);
   return r;
}

EVAS_API void
evas_common_pipe_map_draw(RGBA_Image *src, RGBA_Image *dst,
                          RGBA_Draw_Context *dc, RGBA_Map *m,
                          int smooth, int level)
{
   RGBA_Pipe_Op *op;
   /* RGBA_Map_Point *pts_copy; */

   if (!src) return;
   /* pts_copy = malloc(sizeof (RGBA_Map_Point) * 4); */
   /* if (!pts_copy) return; */
   dst->cache_entry.pipe = evas_common_pipe_add(dst->cache_entry.pipe, &op);
   if (!dst->cache_entry.pipe)
     {
       /* free(pts_copy); */
       return;
     }

   /* for (i = 0; i < 4; ++i) */
   /*   pts_copy[i] = p[i]; */

   op->op.map.smooth = smooth;
   op->op.map.level = level;
   src->ref++;
   op->op.map.src = src;
   op->op.map.m = m;
	op->op.map.anti_alias = dc->anti_alias;
   op->op_func = evas_common_pipe_map_draw_do;
   op->free_func = evas_common_pipe_op_map_free;
   op->prepare_func = evas_common_pipe_map_draw_prepare;
   evas_pipe_prepare_push(op);
   evas_common_pipe_draw_context_copy(dc, op);

   evas_common_pipe_image_load(src);
}

/**
 * @brief Recursively renders map operations.
 *
 * Map operations can be nested (an image used in a map can itself be
 * the result of other pipe operations, including other maps). This function
 * ensures that source images for maps are rendered before the map itself.
 * After recursively processing dependencies, it begins and flushes the pipe
 * for the current root image.
 *
 * @param root The root image whose map operations (and their dependencies) are to be rendered.
 */
static void
evas_common_pipe_map_render(RGBA_Image *root)
{
  RGBA_Pipe *p;
  int i;

  /* Map imply that we need to process them recursively first. */
  for (p = root->cache_entry.pipe; p; p = (RGBA_Pipe *)(EINA_INLIST_GET(p))->next)
    {
      for (i = 0; i < p->op_num; i++)
	{
	  if (p->op[i].op_func == evas_common_pipe_map_draw_do)
	    {
	      if (p->op[i].op.map.src->cache_entry.pipe)
		evas_common_pipe_map_render(p->op[i].op.map.src);
	    }
	  else if (p->op[i].op_func == evas_common_pipe_image_draw_do)
	    {
	      if (p->op[i].op.image.src->cache_entry.pipe)
		evas_common_pipe_map_render(p->op[i].op.image.src);
	    }
	}
    }

  evas_common_pipe_begin(root);
  evas_common_pipe_flush(root);
}

/**
 * @brief Main function for a loading/preparation worker thread.
 *
 * This thread waits for tasks related to loading image data, normalizing
 * colorspaces, preparing text glyphs, and preparing other drawing operations
 * (like calculating cutouts).
 *
 * @param data Pointer to Thinfo structure for this thread.
 * @param t The Eina_Thread handle (unused).
 * @return NULL always.
 */
static void*
evas_common_pipe_load(void *data, Eina_Thread t EINA_UNUSED)
{
  Thinfo *tinfo;

  tinfo = data;
  for (;;)
    {
      RGBA_Pipe_Op *op;
      Eina_Array_Iterator it;
      unsigned int i;
      /* wait for start signal */
      eina_barrier_wait(&(tinfo->barrier[0]));

      while (im_task)
	{
	  RGBA_Image *im = NULL;

	  LKL(im_task_mutex);
	  im = eina_list_data_get(im_task);
	  im_task = eina_list_remove_list(im_task, im_task);
	  LKU(im_task_mutex);

	  if (im)
	    {
	      if (im->cache_entry.space == EVAS_COLORSPACE_ARGB8888)
		evas_cache_image_load_data(&im->cache_entry);
	      evas_common_image_colorspace_normalize(im);

	      im->flags &= ~RGBA_IMAGE_TODO_LOAD;
	    }
	}

      while (text_task)
	{
           Evas_Text_Props *text_props;
           RGBA_Font_Int *fi;

           LKL(text_task_mutex);
           fi = eina_list_data_get(text_task);
           text_task = eina_list_remove_list(text_task, text_task);
           LKU(text_task_mutex);

           if (fi)
             {
                LKL(fi->ft_mutex);
                EINA_LIST_FREE(fi->task, text_props)
		  {
                     evas_common_font_draw_prepare(text_props);
                     text_props->changed = EINA_FALSE;
		     text_props->prepare = EINA_FALSE;
		  }
                LKU(fi->ft_mutex);
             }
	}

      EINA_ARRAY_ITER_NEXT(&tinfo->rects_task, i, op, it)
        op->render = op->prepare_func(tinfo, tinfo->im, op);
      eina_array_clean(&tinfo->rects_task);

      /* send finished signal */
      eina_barrier_wait(&(tinfo->barrier[1]));
    }

  return NULL;
}

static volatile int bval = 0; /**< Unused variable. */

/**
 * @brief Triggers the loading and preparation phase for an image.
 *
 * Assigns the image to all loading worker threads and signals them
 * to start processing their respective tasks (image loading, text prep, op prep).
 * It then waits for all loading threads to complete.
 *
 * @param im The image for which loading/preparation tasks should be performed.
 */
static void
evas_common_pipe_load_do(RGBA_Image *im)
{
   int i;

   for (i = 0; i < thread_num; i++)
     task_thinfo[i].im = im;

   /* Notify worker thread. */
   eina_barrier_wait(&(task_thbarrier[0]));

   /* sync worker threads */
   eina_barrier_wait(&(task_thbarrier[1]));
}

EVAS_API void
evas_common_pipe_image_load(RGBA_Image *im)
{
  if (im->flags & RGBA_IMAGE_TODO_LOAD)
    return;

  if (im->cache_entry.space == EVAS_COLORSPACE_ARGB8888
      && !evas_cache_image_is_loaded(&(im->cache_entry)))
    goto add_task;

  if (!((!im->cs.data) || ((!im->cs.dirty) && (!(im->flags & RGBA_IMAGE_IS_DIRTY)))))
    goto add_task;

  return;

 add_task:
  LKL(im_task_mutex);
  im_task = eina_list_append(im_task, im);
  LKU(im_task_mutex);
  im->flags |= RGBA_IMAGE_TODO_LOAD;
}

EVAS_API void
evas_common_pipe_text_prepare(Evas_Text_Props *text_props)
{
   RGBA_Font_Int *fi;

   fi = text_props->font_instance;
   if (!fi) return;

   if (!text_props->changed && text_props->generation == fi->generation && text_props->glyphs)
     return;

   LKL(fi->ft_mutex);

   if (!fi->task)
     {
       LKL(text_task_mutex);
       text_task = eina_list_append(text_task, fi);
       LKU(text_task_mutex);
     }

   if (text_props->prepare) goto end;
   text_props->prepare = EINA_TRUE;
   fi->task = eina_list_append(fi->task, text_props);

 end:
   LKU(fi->ft_mutex);
}

EVAS_API void
evas_common_pipe_map_begin(RGBA_Image *root)
{
  if (!evas_common_pipe_init())
    {
      RGBA_Image *im;

      EINA_LIST_FREE(im_task, im)
	{
	  if (im->cache_entry.space == EVAS_COLORSPACE_ARGB8888)
	    evas_cache_image_load_data(&im->cache_entry);
	  evas_common_image_colorspace_normalize(im);

	  im->flags &= ~RGBA_IMAGE_TODO_LOAD;
	}
    }

  evas_common_pipe_load_do(root);

  evas_common_pipe_map_render(root);
}
#endif

EVAS_API Eina_Bool
evas_common_pipe_init(void)
{
#ifdef BUILD_PIPE_RENDER
   if (thread_num == 0)
     {
	int cpunum;
	int i;

	cpunum = eina_cpu_count();
	thread_num = cpunum;
// on  single cpu we still want this initted.. otherwise we block forever
// waiting onm pthread barriers for async rendering on a single core!
//	if (thread_num == 1) return EINA_FALSE;

	eina_threads_init();

        LKI(im_task_mutex);
	LKI(text_task_mutex);

	eina_barrier_new(&(thbarrier[0]), thread_num + 1);
	eina_barrier_new(&(thbarrier[1]), thread_num + 1);
	for (i = 0; i < thread_num; i++)
	  {
	     thinfo[i].thread_num = i;
	     thinfo[i].tasks = NULL;
	     thinfo[i].barrier = thbarrier;

             eina_thread_create(&(thinfo[i].thread_id), EINA_THREAD_NORMAL, i,
                                evas_common_pipe_thread, &(thinfo[i]));
	  }

	eina_barrier_new(&(task_thbarrier[0]), thread_num + 1);
	eina_barrier_new(&(task_thbarrier[1]), thread_num + 1);
	for (i = 0; i < thread_num; i++)
	  {
	     task_thinfo[i].thread_num = i;
	     task_thinfo[i].tasks = NULL;
	     task_thinfo[i].barrier = task_thbarrier;
             eina_array_step_set(&task_thinfo[i].cutout_trash, sizeof (Eina_Array), 8);
             eina_array_step_set(&task_thinfo[i].rects_task, sizeof (Eina_Array), 8);

             eina_thread_create(&(task_thinfo[i].thread_id), EINA_THREAD_NORMAL, i,
                                evas_common_pipe_load, &(task_thinfo[i]));
	  }
     }

   if (thread_num == 1) return EINA_FALSE;
   return EINA_TRUE;
#endif
   return EINA_FALSE;
}
