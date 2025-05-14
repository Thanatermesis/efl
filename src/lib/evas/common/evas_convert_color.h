#ifndef _EVAS_CONVERT_COLOR_H
#define _EVAS_CONVERT_COLOR_H

/**
 * @brief Premultiplies an array of 16-bit Alpha-Grayscale (AG) pixel data.
 * @param data Pointer to the array of AG pixel data. (AAGG format)
 * @param len Number of pixels in the data array.
 * @return The number of pixels that were either fully transparent or fully opaque.
 */
EVAS_API DATA32 evas_common_convert_ag_premul                          (DATA16 *data, unsigned int len);

/**
 * @brief Unpremultiplies an array of 16-bit Alpha-Grayscale (AG) pixel data.
 * @param data Pointer to the array of AG pixel data. (AAGG format, premultiplied)
 * @param len Number of pixels in the data array.
 */
EVAS_API void evas_common_convert_ag_unpremul                          (DATA16 *data, unsigned int len);

/**
 * @brief Premultiplies an array of 32-bit ARGB pixel data.
 * @param src Pointer to the array of ARGB pixel data.
 * @param len Number of pixels in the data array.
 * @return Result of efl_draw_argb_premul (likely count of non-alpha-solid pixels or similar).
 */
EVAS_API DATA32 evas_common_convert_argb_premul                        (DATA32 *src, unsigned int len);

/**
 * @brief Unpremultiplies an array of 32-bit ARGB pixel data.
 * @param src Pointer to the array of ARGB pixel data (premultiplied).
 * @param len Number of pixels in the data array.
 */
EVAS_API void evas_common_convert_argb_unpremul                        (DATA32 *src, unsigned int len);

/**
 * @brief Premultiplies RGB color components by an alpha value.
 * @param a Alpha value (0-255).
 * @param r Pointer to the Red component (0-255). Modified in place.
 * @param g Pointer to the Green component (0-255). Modified in place.
 * @param b Pointer to the Blue component (0-255). Modified in place.
 */
EVAS_API void evas_common_convert_color_argb_premul                    (int a, int *r, int *g, int *b);

/**
 * @brief Unpremultiplies RGB color components by an alpha value.
 * @param a Alpha value (0-255).
 * @param r Pointer to the Red component (0-255). Modified in place.
 * @param g Pointer to the Green component (0-255). Modified in place.
 * @param b Pointer to the Blue component (0-255). Modified in place.
 */
EVAS_API void evas_common_convert_color_argb_unpremul                  (int a, int *r, int *g, int *b);

/**
 * @brief Converts HSV (Hue, Saturation, Value) color to RGB.
 * @param h Hue component (0.0-360.0).
 * @param s Saturation component (0.0-1.0).
 * @param v Value component (0.0-1.0).
 * @param r Pointer to store the Red component (0-255).
 * @param g Pointer to store the Green component (0-255).
 * @param b Pointer to store the Blue component (0-255).
 */
EVAS_API void evas_common_convert_color_hsv_to_rgb                     (float h, float s, float v, int *r, int *g, int *b);

/**
 * @brief Converts RGB color to HSV (Hue, Saturation, Value).
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param h Pointer to store the Hue component (0.0-360.0).
 * @param s Pointer to store the Saturation component (0.0-1.0).
 * @param v Pointer to store the Value component (0.0-1.0).
 */
EVAS_API void evas_common_convert_color_rgb_to_hsv                     (int r, int g, int b, float *h, float *s, float *v);

/**
 * @brief Converts HSV color to RGB using integer arithmetic.
 * @param h Hue component (0-255, maps to 0-360 degrees).
 * @param s Saturation component (0-255).
 * @param v Value component (0-255).
 * @param r Pointer to store the Red component (0-255).
 * @param g Pointer to store the Green component (0-255).
 * @param b Pointer to store the Blue component (0-255).
 */
EVAS_API void evas_common_convert_color_hsv_to_rgb_int                 (int h, int s, int v, int *r, int *g, int *b);

/**
 * @brief Converts RGB color to HSV using integer arithmetic.
 * @param r Red component (0-255).
 * @param g Green component (0-255).
 * @param b Blue component (0-255).
 * @param h Pointer to store the Hue component (0-1530, maps to 0-360 degrees).
 * @param s Pointer to store the Saturation component (0-255).
 * @param v Pointer to store the Value component (0-255).
 */
EVAS_API void evas_common_convert_color_rgb_to_hsv_int                 (int r, int g, int b, int *h, int *s, int *v);


#endif /* _EVAS_CONVERT_COLOR_H */
