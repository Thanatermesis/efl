#ifndef _EVAS_CONVERT_MAIN_H
#define _EVAS_CONVERT_MAIN_H

/**
 * @brief Initializes the Evas common conversion module.
 *
 * This function should be called before any other conversion functions are used.
 * It sets up any necessary internal structures or lookup tables.
 */
EVAS_API void             evas_common_convert_init          (void);

/**
 * @brief Retrieves a conversion function based on destination buffer properties.
 *
 * This function selects an appropriate conversion function to convert RGBA data
 * to the specified destination format.
 *
 * @param dest Pointer to the destination buffer.
 * @param w Width of the image in pixels.
 * @param h Height of the image in pixels.
 * @param depth Bit depth of the destination image (e.g., 8, 16, 24, 32).
 * @param rmask Red channel mask for the destination format.
 * @param gmask Green channel mask for the destination format.
 * @param bmask Blue channel mask for the destination format.
 * @param pal_mode Palette mode for palettized destination formats.
 *                 Example: PAL_MODE_RGB332, PAL_MODE_GRAY64.
 * @param rotation Rotation angle (0, 90, 180, 270 degrees).
 * @return A function pointer to the appropriate RGBA conversion routine,
 *         or NULL if no suitable converter is found.
 */
EVAS_API Gfx_Func_Convert evas_common_convert_func_get      (DATA8 *dest, int w, int h, int depth, DATA32 rmask, DATA32 gmask, DATA32 bmask, Convert_Pal_Mode pal_mode, int rotation);


#endif /* _EVAS_CONVERT_MAIN_H */
