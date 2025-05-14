/**
 * @file
 * @brief Implementation of the Shared_Buffer API.
 *
 * This file contains the internal structure definition and function
 * implementations for managing shared buffers.
 */
#include "shared_buffer.h"

/**
 * @brief Internal structure for a Shared_Buffer.
 *
 * Holds all the necessary information for a shared buffer, including its
 * associated Ecore_Buffer, underlying resource, dimensions, format, flags,
 * and current state.
 */
struct _Shared_Buffer
{
   Ecore_Buffer *buffer;        /**< Optional Ecore_Buffer wrapper. */
   struct bq_buffer *resource;  /**< The underlying buffer queue resource. */
   const char *engine;          /**< Name of the engine (e.g., "wayland", "drm"). */
   int w, h;                    /**< Width and height of the buffer. */
   int format;                  /**< Pixel format of the buffer. */
   unsigned int flags;          /**< Buffer flags. */
   Shared_Buffer_State state;   /**< Current state of the buffer. */
};

/**
 * @brief Creates a new Shared_Buffer instance.
 * @param engine The engine name.
 * @param resource The bq_buffer resource.
 * @param w Width.
 * @param h Height.
 * @param format Pixel format.
 * @param flags Buffer flags.
 * @return A new Shared_Buffer or NULL on failure.
 */
Shared_Buffer *
_shared_buffer_new(const char *engine, struct bq_buffer *resource, int w, int h, int format, unsigned int flags)
{
   Shared_Buffer *sb;

   sb = calloc(1, sizeof(Shared_Buffer));
   if (!sb)
     return NULL;

   sb->engine = eina_stringshare_add(engine);
   sb->resource = resource;
   sb->w = w;
   sb->h = h;
   sb->format = format;
   sb->flags = flags;
   sb->state = SHARED_BUFFER_STATE_NEW;

   return sb;
}

/**
 * @brief Frees the memory allocated for a Shared_Buffer.
 * @param sb The Shared_Buffer to free.
 */
void
_shared_buffer_free(Shared_Buffer *sb)
{
   EINA_SAFETY_ON_NULL_RETURN(sb);

   if (sb->engine) eina_stringshare_del(sb->engine);
   free(sb);
}

/**
 * @brief Retrieves information about a Shared_Buffer.
 * @param sb The Shared_Buffer.
 * @param[out] engine Pointer to store the engine name.
 * @param[out] w Pointer to store the width.
 * @param[out] h Pointer to store the height.
 * @param[out] format Pointer to store the format.
 * @param[out] flags Pointer to store the flags.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
_shared_buffer_info_get(Shared_Buffer *sb, const char **engine, int *w, int *h, int *format, unsigned int *flags)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, EINA_FALSE);

   if (engine) *engine = sb->engine;
   if (w) *w = sb->w;
   if (h) *h = sb->h;
   if (format) *format = sb->format;
   if (flags) *flags = sb->flags;

   return EINA_TRUE;
}

/**
 * @brief Sets the Ecore_Buffer for a Shared_Buffer.
 * @param sb The Shared_Buffer.
 * @param buffer The Ecore_Buffer to set.
 * @return EINA_TRUE on success, EINA_FALSE if buffer already exists or on failure.
 */
Eina_Bool
_shared_buffer_buffer_set(Shared_Buffer *sb, Ecore_Buffer *buffer)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, EINA_FALSE);

   if (sb->buffer)
     {
        ERR("Already exist buffer");
        return EINA_FALSE;
     }

   sb->buffer = buffer;

   return EINA_TRUE;
}

/**
 * @brief Gets the Ecore_Buffer from a Shared_Buffer.
 * @param sb The Shared_Buffer.
 * @return The Ecore_Buffer, or NULL if not set or on failure.
 */
Ecore_Buffer *
_shared_buffer_buffer_get(Shared_Buffer *sb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, NULL);

   return sb->buffer;
}

/**
 * @brief Sets the bq_buffer resource for a Shared_Buffer.
 * @param sb The Shared_Buffer.
 * @param resource The bq_buffer resource to set.
 * @return EINA_TRUE on success, EINA_FALSE if resource already exists or on failure.
 */
Eina_Bool
_shared_buffer_resource_set(Shared_Buffer *sb, struct bq_buffer *resource)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, EINA_FALSE);

   if (sb->resource)
     {
        ERR("Already exist resource");
        return EINA_FALSE;
     }

   sb->resource = resource;

   return EINA_TRUE;
}

/**
 * @brief Gets the bq_buffer resource from a Shared_Buffer.
 * @param sb The Shared_Buffer.
 * @return The bq_buffer resource, or NULL if not set or on failure.
 */
struct bq_buffer *
_shared_buffer_resource_get(Shared_Buffer *sb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, NULL);

   return sb->resource;
}

/**
 * @brief Sets the state of a Shared_Buffer.
 * @param sb The Shared_Buffer.
 * @param state The new state.
 */
void
_shared_buffer_state_set(Shared_Buffer *sb, Shared_Buffer_State state)
{
   EINA_SAFETY_ON_NULL_RETURN(sb);

   sb->state = state;
}

/**
 * @brief Gets the state of a Shared_Buffer.
 * @param sb The Shared_Buffer.
 * @return The current state, or SHARED_BUFFER_STATE_UNKNOWN on failure.
 */
Shared_Buffer_State
_shared_buffer_state_get(Shared_Buffer *sb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, SHARED_BUFFER_STATE_UNKNOWN);

   return sb->state;
}

/**
 * @brief Gets a string representation of the Shared_Buffer's state.
 * @param sb The Shared_Buffer.
 * @return A string describing the state. Returns "INVAILD OBJECT" if sb is NULL.
 */
const char *
_shared_buffer_state_string_get(Shared_Buffer *sb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sb, "INVAILD OBJECT");

   switch (sb->state)
     {
      case SHARED_BUFFER_STATE_ENQUEUE:
         return "SHARED_BUFFER_STATE_ENQUEUE";
      case SHARED_BUFFER_STATE_SUBMIT:
         return "SHARED_BUFFER_STATE_SUBMIT";
      case SHARED_BUFFER_STATE_DEQUEUE:
         return "SHARED_BUFFER_STATE_DEQUEUE";
      case SHARED_BUFFER_STATE_ATTACH:
         return "SHARED_BUFFER_STATE_ATTACH";
      case SHARED_BUFFER_STATE_IMPORT:
         return "SHARED_BUFFER_STATE_IMPORT";
      case SHARED_BUFFER_STATE_DETACH:
         return "SHARED_BUFFER_STATE_DETACH";
      case SHARED_BUFFER_STATE_ACQUIRE:
         return "SHARED_BUFFER_STATE_ACQUIRE";
      case SHARED_BUFFER_STATE_RELEASE:
         return "SHARED_BUFFER_STATE_RELEASE";
      default:
      case SHARED_BUFFER_STATE_UNKNOWN:
         return "SHARED_BUFFER_STATE_UNKNOWN";
     }
}
