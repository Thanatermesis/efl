#include "evas_common_private.h"
#include "evas_convert_gry_1.h"

/**
 * @brief Converts RGBA image data to 1-bit grayscale with dithering.
 * @see evas_convert_gry_1.h for detailed parameter descriptions.
 *
 * @param src Pointer to the source image data (currently unused).
 * @param dst Pointer to the destination image data (currently unused).
 * @param src_jump Jump in source data to next line (currently unused).
 * @param dst_jump Jump in destination data to next line (currently unused).
 * @param w Width of the image (currently unused).
 * @param h Height of the image (currently unused).
 * @param dith_x X offset for dithering (currently unused).
 * @param dith_y Y offset for dithering (currently unused).
 * @param pal Palette for conversion (currently unused).
 *
 * @note The current implementation is a stub and does not perform any conversion.
 *       The EINA_UNUSED macro indicates that parameters are intentionally not used
 *       in this stub implementation.
 */
void evas_common_convert_rgba_to_1bpp_gry_1_dith(DATA32 *src EINA_UNUSED, DATA8 *dst EINA_UNUSED, int src_jump EINA_UNUSED, int dst_jump EINA_UNUSED, int w EINA_UNUSED, int h EINA_UNUSED, int dith_x EINA_UNUSED, int dith_y EINA_UNUSED, DATA8 *pal EINA_UNUSED)
{
}
