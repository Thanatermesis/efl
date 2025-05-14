#include "evas_common_private.h"
#include "evas_private.h"
//#include "evas_cs.h"

/**
 * @internal
 * @brief Blocks asynchronous operations on the canvas associated with the output.
 *
 * This function retrieves the Evas_Public_Data associated with the canvas
 * of the given output and then blocks asynchronous operations on that canvas.
 * It's a helper function to ensure that operations on the output are
 * synchronized with the canvas.
 *
 * @param output The Efl_Canvas_Output whose associated canvas needs to be blocked.
 * @return A pointer to Evas_Public_Data if successful, NULL otherwise.
 */
static Evas_Public_Data *
_efl_canvas_output_async_block(Efl_Canvas_Output *output)
{
   Evas_Public_Data *e;

   if (!output->canvas) return NULL;
   e = efl_data_scope_get(output->canvas, EVAS_CANVAS_CLASS);
   if (!e) return NULL;

   evas_canvas_async_block(e);

   return e;
}

/**
 * @internal
 * @brief Retrieves or initializes the engine-specific information for an output.
 *
 * If the output already has an info structure, this function does nothing.
 * Otherwise, it allocates and initializes a new info structure based on the
 * engine's requirements. The `info_size` from the engine functions is used
 * to determine the size of this structure. A magic number is assigned to
 * track the validity of the info structure.
 *
 * @param e The Evas_Public_Data associated with the canvas.
 * @param output The Efl_Canvas_Output for which to get/initialize the info.
 */
void
efl_canvas_output_info_get(Evas_Public_Data *e, Efl_Canvas_Output *output)
{
   if (output->info) return;
   if (!e->engine.func->info_size)
     {
        CRI("Engine not up to date no info size provided.");
        return ;
     }

   output->info = calloc(1, e->engine.func->info_size);
   if (!output->info) return;
   output->info->magic = rand();
   output->info_magic = output->info->magic;

   if (e->engine.func->output_info_setup)
     e->engine.func->output_info_setup(output->info);
}

/**
 * @brief Adds a new output to an Evas canvas.
 *
 * This function creates a new output associated with the given Evas canvas.
 * It allocates memory for the Efl_Canvas_Output structure, initializes it,
 * and links it to the canvas. It also sets up the engine-specific
 * information for this new output.
 *
 * @param canvas The Evas canvas to which the new output will be added.
 *               Must be a valid Evas canvas object.
 * @return A pointer to the newly created Efl_Canvas_Output on success,
 *         or NULL on failure (e.g., if canvas is invalid or memory allocation fails).
 */
EVAS_API Efl_Canvas_Output *
efl_canvas_output_add(Evas *canvas)
{
   Efl_Canvas_Output *r;
   Evas_Public_Data *e;

   if (!efl_isa(canvas, EVAS_CANVAS_CLASS)) return NULL;

   r = calloc(1, sizeof (Efl_Canvas_Output));
   if (!r) return NULL;

   efl_wref_add(canvas, &r->canvas);
   r->changed = EINA_TRUE;

   e = _efl_canvas_output_async_block(r);
   if (!e)
     {
        efl_wref_del(canvas, &r->canvas);
        free(r);
        return NULL;
     }

   // Track this output in Evas
   e->outputs = eina_list_append(e->outputs, r);

   // The engine is already initialized, use it
   // right away to setup the info structure
   efl_canvas_output_info_get(e, r);

   return r;
}

/**
 * @brief Deletes an Evas canvas output.
 *
 * This function removes the specified output from its associated Evas canvas.
 * It frees resources used by the output, including engine-specific data and
 * the Efl_Canvas_Output structure itself. It also ensures that asynchronous
 * operations are blocked during the deletion process.
 *
 * @param output The Efl_Canvas_Output to be deleted.
 */
EVAS_API void
efl_canvas_output_del(Efl_Canvas_Output *output)
{
   if (output->canvas)
     {
        Evas_Public_Data *e;

        e = _efl_canvas_output_async_block(output);
        if (!e) goto on_error;

        if (e->engine.func)
          {
             e->engine.func->output_free(_evas_engine_context(e),
                                         output->output);
             free(output->info);
             output->info = NULL;
          }
        e->outputs = eina_list_remove(e->outputs, output);

        efl_wref_del(output->canvas, &output->canvas);
     }

 on_error:
   free(output);
}

/**
 * @brief Sets the viewport geometry for an Evas canvas output.
 *
 * This function defines the rectangular area (viewport) of the canvas
 * that this output will display. If the new geometry is different from
 * the current one, the output is marked as changed.
 *
 * @param output The Efl_Canvas_Output whose viewport is to be set.
 * @param x The x-coordinate of the top-left corner of the viewport.
 * @param y The y-coordinate of the top-left corner of the viewport.
 * @param w The width of the viewport.
 * @param h The height of the viewport.
 */
EVAS_API void
efl_canvas_output_view_set(Efl_Canvas_Output *output,
                           Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   Evas_Public_Data *e;

   e = _efl_canvas_output_async_block(output);
   if (!e) return;

   if (output->geometry.x != x) goto changed;
   if (output->geometry.y != y) goto changed;
   if (output->geometry.w != w) goto changed;
   if (output->geometry.h != h) goto changed;
   return;

 changed:
   output->geometry.x = x;
   output->geometry.y = y;
   output->geometry.w = w;
   output->geometry.h = h;
   output->changed = EINA_TRUE;
   // XXX: tell evas to add damage if viewport loc/size changed
}

/**
 * @brief Retrieves the viewport geometry for an Evas canvas output.
 *
 * This function gets the current rectangular area (viewport) of the canvas
 * that this output is displaying.
 *
 * @param output The Efl_Canvas_Output whose viewport is to be retrieved.
 * @param x Pointer to store the x-coordinate of the viewport. Can be NULL.
 * @param y Pointer to store the y-coordinate of the viewport. Can be NULL.
 * @param w Pointer to store the width of the viewport. Can be NULL.
 * @param h Pointer to store the height of the viewport. Can be NULL.
 */
EVAS_API void
efl_canvas_output_view_get(Efl_Canvas_Output *output,
                           Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   if (x) *x = output->geometry.x;
   if (y) *y = output->geometry.y;
   if (w) *w = output->geometry.w;
   if (h) *h = output->geometry.h;
}

/**
 * @brief Sets the engine-specific information for an Evas canvas output.
 *
 * This function updates the Evas engine with new information for the specified
 * output. It verifies the validity of the provided info structure using a magic
 * number. If the output already has an engine-specific context (`output->output`),
 * it attempts to update it. Otherwise, it sets up a new engine context.
 *
 * @param output The Efl_Canvas_Output for which to set the engine info.
 * @param info A pointer to the Evas_Engine_Info structure containing the
 *             new engine-specific data. This structure is typically obtained
 *             via efl_canvas_output_engine_info_get(), modified, and then passed back.
 * @return EINA_TRUE if the engine information was successfully set or updated,
 *         EINA_FALSE otherwise (e.g., if the info structure is invalid,
 *         or if the engine fails to set up/update).
 */
EVAS_API Eina_Bool
efl_canvas_output_engine_info_set(Efl_Canvas_Output *output,
                                  Evas_Engine_Info *info)
{
   Evas_Public_Data *e;

   e = _efl_canvas_output_async_block(output);
   if (!e) return EINA_FALSE;
   if (output->info != info) return EINA_FALSE;
   if (info->magic != output->info_magic) return EINA_FALSE;

   if (output->output)
     {
        if (e->engine.func->output_update)
          {
             e->engine.func->output_update(_evas_engine_context(e), output->output, info,
                                           output->geometry.w, output->geometry.h);
          }
        else
          {
             // For engine who do not provide an update function
             e->engine.func->output_free(_evas_engine_context(e),
                                         output->output);

             goto setup;
          }
     }
   else
     {
     setup:
        output->output =
          e->engine.func->output_setup(_evas_engine_context(e), info,
                                       output->geometry.w, output->geometry.h);
     }

   return !!output->output;
}

/**
 * @brief Retrieves the engine-specific information for an Evas canvas output.
 *
 * This function returns a pointer to the Evas_Engine_Info structure associated
 * with the given output. This structure contains engine-specific data that
 * can be modified and then passed to efl_canvas_output_engine_info_set().
 * A magic number is updated to ensure that the info structure is not used
 * stale after retrieval.
 *
 * @param output The Efl_Canvas_Output from which to get the engine info.
 * @return A pointer to the Evas_Engine_Info structure if available,
 *         NULL otherwise. The returned pointer is owned by the output and
 *         should not be freed by the caller.
 */
EVAS_API Evas_Engine_Info*
efl_canvas_output_engine_info_get(Efl_Canvas_Output *output)
{
   Evas_Engine_Info *info = output->info;

   if (!info) return NULL;

   output->info_magic = info->magic;
   return output->info;
}

/**
 * @brief Locks an Evas canvas output.
 *
 * Increments the lock count for the given output. When an output is locked
 * (lock count > 0), certain operations might be deferred or behave differently,
 * depending on the engine implementation. This is often used to prevent
 * updates or rendering during a critical section.
 *
 * @param output The Efl_Canvas_Output to lock.
 * @return EINA_TRUE always (the operation itself is considered successful).
 */
EVAS_API Eina_Bool
efl_canvas_output_lock(Efl_Canvas_Output *output)
{
   output->lock++;
   return EINA_TRUE;
}

/**
 * @brief Unlocks an Evas canvas output.
 *
 * Decrements the lock count for the given output.
 *
 * @param output The Efl_Canvas_Output to unlock.
 * @return EINA_TRUE if the output remains locked (lock count > 0 after decrementing),
 *         EINA_FALSE if the output is now unlocked (lock count is 0).
 */
EVAS_API Eina_Bool
efl_canvas_output_unlock(Efl_Canvas_Output *output)
{
   return !!(--output->lock);
}
