#ifndef _ECORE_EVAS_BUFFER_PRIVATE_H_
#define _ECORE_EVAS_BUFFER_PRIVATE_H_

typedef struct _Ecore_Evas_Engine_Buffer_Data Ecore_Evas_Engine_Buffer_Data;

/**
 * @internal
 * @brief Structure holding private data for the Ecore_Evas buffer engine.
 *
 * This structure contains all the necessary information for managing a
 * buffer-based Ecore_Evas instance, including pixel data, associated Evas
 * image object (if any), and custom memory allocation functions.
 */
struct _Ecore_Evas_Engine_Buffer_Data {
   void *pixels; /**< Pointer to the raw pixel data of the buffer. */
   Evas_Object *image; /**< Evas image object associated with this buffer, if used as an image source. NULL otherwise. */
   void  (*free_func) (void *data, void *pix); /**< Custom function to free the pixel data. @param data User data passed to free_func. @param pix Pointer to pixel data to free. */
   void *(*alloc_func) (void *data, int size); /**< Custom function to allocate pixel data. @param data User data passed to alloc_func. @param size Size in bytes to allocate. @return Pointer to allocated memory or NULL on failure. */
   void *data; /**< User data pointer passed to alloc_func and free_func. */
   Eina_Bool lock_data : 1; /**< Flag indicating if the pixel data is currently locked (e.g., during an update). */
   Eina_Bool resized : 1; /**< Flag indicating if the buffer has been resized and needs processing. */
};

#endif /* _ECORE_EVAS_BUFFER_PRIVATE_H_ */
