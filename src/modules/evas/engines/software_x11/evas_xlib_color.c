#include "evas_common_private.h"

#include "evas_engine.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

/** @brief Private data associated with a Convert_Pal structure. */
typedef struct _Convert_Pal_Priv Convert_Pal_Priv;

/**
 * @brief Holds the X11 display, colormap, and visual associated with a
 * specific allocated palette. Used to identify shared palettes.
 */
struct _Convert_Pal_Priv
{
   Display *disp;
   Colormap cmap;
   Visual  *vis; /**< The X visual. */
};

/**
 * @brief Function pointer type for X color allocation functions.
 * @param d The X display.
 * @param cmap The X colormap.
 * @param v The X visual.
 * @return A pointer to an array of allocated pixel values (DATA8*),
 *         or NULL on failure. The size and layout depend on the specific
 *         allocation function.
 */
typedef DATA8 * (*X_Func_Alloc_Colors)(Display *d, Colormap cmap, Visual *v);

/** @brief Array of function pointers for allocating different palette types. Indexed by Convert_Pal_Mode. */
static X_Func_Alloc_Colors x_color_alloc[PAL_MODE_LAST + 1];
/** @brief Array storing the number of colors for each palette type. Indexed by Convert_Pal_Mode. */
static int x_color_count[PAL_MODE_LAST + 1];
/** @brief List of currently active palettes (Convert_Pal*) to allow sharing. */
static Eina_List *palettes = NULL;

/** @brief Allocates an RGB color cube. */
static DATA8 *x_color_alloc_rgb(int nr, int ng, int nb, Display *d, Colormap cmap, Visual *v);
/** @brief Allocates a grayscale ramp. */
static DATA8 *x_color_alloc_gray(int ng, Display *d, Colormap cmap, Visual *v);

/** @brief Allocates an RGB 3:3:2 palette (8 levels R, 8 levels G, 4 levels B). */

static DATA8 *x_color_alloc_rgb_332(Display *d, Colormap cmap, Visual *v);
/** @brief Allocates an RGB 6:6:6 palette (6 levels R, 6 levels G, 6 levels B). */
static DATA8 *x_color_alloc_rgb_666(Display *d, Colormap cmap, Visual *v);
/** @brief Allocates an RGB 2:3:2 palette (4 levels R, 8 levels G, 4 levels B). */
static DATA8 *x_color_alloc_rgb_232(Display *d, Colormap cmap, Visual *v);
/** @brief Allocates an RGB 2:2:2 palette (4 levels R, 4 levels G, 4 levels B). */
static DATA8 *x_color_alloc_rgb_222(Display *d, Colormap cmap, Visual *v);
/** @brief Allocates an RGB 2:2:1 palette (4 levels R, 4 levels G, 2 levels B). */
static DATA8 *x_color_alloc_rgb_221(Display *d, Colormap cmap, Visual *v);
/** @brief Allocates an RGB 1:2:1 palette (2 levels R, 4 levels G, 2 levels B). */
static DATA8 *x_color_alloc_rgb_121(Display *d, Colormap cmap, Visual *v);
/** @brief Allocates an RGB 1:1:1 palette (2 levels R, 2 levels G, 2 levels B). */
static DATA8 *x_color_alloc_rgb_111(Display *d, Colormap cmap, Visual *v);
/** @brief Allocates a 256-level grayscale palette. */
static DATA8 *x_color_alloc_gray_256(Display *d, Colormap cmap, Visual *v);
/** @brief Allocates a 64-level grayscale palette. */
static DATA8 *x_color_alloc_gray_64(Display *d, Colormap cmap, Visual *v);
/** @brief Allocates a 16-level grayscale palette (using 32 levels internally?). */
static DATA8 *x_color_alloc_gray_16(Display *d, Colormap cmap, Visual *v);
/** @brief Allocates a 4-level grayscale palette (using 16 levels internally?). */
static DATA8 *x_color_alloc_gray_4(Display *d, Colormap cmap, Visual *v);
/** @brief Allocates a 2-level monochrome palette. */
static DATA8 *x_color_alloc_mono(Display *d, Colormap cmap, Visual *v);

/**
 * @brief Allocates an RGB color cube with specified levels per component.
 *
 * Iterates through all combinations of red, green, and blue levels,
 * attempting to allocate each color using XAllocColor. If any allocation
 * fails or the allocated color differs significantly from the requested color,
 * it frees any already allocated colors and returns NULL.
 *
 * @param nr Number of red levels.
 * @param ng Number of green levels.
 * @param nb Number of blue levels.
 * @param d The X display.
 * @param cmap The X colormap.
 * @param v The X visual (currently unused, marked EINA_UNUSED).
 * @return A pointer to a flat array (DATA8*) containing the allocated pixel
 *         values for the color cube, ordered B-major, then G, then R.
 *         The size is nr * ng * nb. Returns NULL on failure.
 *         Example for nr=2, ng=2, nb=2: [R0G0B0, R0G0B1, R0G1B0, R0G1B1, R1G0B0, R1G0B1, R1G1B0, R1G1B1]
 */
static DATA8 *
x_color_alloc_rgb(int nr, int ng, int nb, Display *d, Colormap cmap, Visual *v EINA_UNUSED)
{
   int r, g, b, i;
   DATA8 *color_lut;
   int delt = 0;
/*
   int sig_mask = 0;

   for (i = 0; i < v->bits_per_rgb; i++)
     sig_mask |= (0x1 << i);
   sig_mask <<= (16 - v->bits_per_rgb);
*/
   i = 0;
   color_lut = malloc((nr) * (ng) * (nb));
   if (!color_lut) return NULL;
   delt = 0x0101 * 3;
   for (r = 0; r < (nr); r++)
     {
        for (g = 0; g < (ng); g++)
          {
             for (b = 0; b < (nb); b++)
               {
                  XColor xcl;
                  XColor xcl_in;
                  int val;
                  Status ret;
                  int dr, dg, db;

                  val = (int)(((r * 255) / ((nr) - 1)));
                  val = (val << 8) | val;
                  xcl.red = (unsigned short)(val);
                  val = (int)(((g * 255) / ((ng) - 1)));
                  val = (val << 8) | val;
                  xcl.green = (unsigned short)(val);
                  val = (int)(((b * 255) / ((nb) - 1)));
                  val = (val << 8) | val;
                  xcl.blue = (unsigned short)(val);
                  xcl.pixel = 0;
                  xcl.flags = 0;
                  xcl.pad = 0;
                  xcl_in = xcl;
                  ret = XAllocColor(d, cmap, &xcl);
                  dr = (int)xcl_in.red - (int)xcl.red;
                  if (dr < 0) dr = -dr;
                  dg = (int)xcl_in.green - (int)xcl.green;
                  if (dg < 0) dg = -dg;
                  db = (int)xcl_in.blue - (int)xcl.blue;
                  if (db < 0) db = -db;
/*
                  printf("ASK [%i]: %04x %04x %04x = %04x %04x %04x | dif = %04x / %04x\n",
                         ret,
                         xcl_in.red, xcl_in.green, xcl_in.blue,
                         xcl.red, xcl.green, xcl.blue,
                         (dr + dg +db), delt);
 */
                  if ((ret == 0) ||
                      ((dr + dg + db) > delt)
/*
||
                      ((xcl_in.red & sig_mask) != (xcl.red & sig_mask)) ||
                      ((xcl_in.green & sig_mask) != (xcl.green & sig_mask)) ||
                      ((xcl_in.blue & sig_mask) != (xcl.blue & sig_mask))
*/
                      )
                    {
                       unsigned long pixels[256];
                       int j;

                       if (i > 0)
                         {
                            for (j = 0; j < i; j++)
                              pixels[j] = (unsigned long)color_lut[j];
                            XFreeColors(d, cmap, pixels, i, 0);
                         }
                       free(color_lut);
                       return NULL;
                    }
                  color_lut[i] = xcl.pixel;
                  i++;
               }
          }
     }
   return color_lut;
}

/**
 * @brief Allocates a grayscale ramp with a specified number of levels.
 *
 * Iterates through the gray levels, attempting to allocate each using
 * XAllocColor. It checks if the allocated color matches the requested color
 * within the precision defined by the visual's bits_per_rgb. If any
 * allocation fails or the color match is poor, it frees already allocated
 * colors and returns NULL.
 *
 * @param ng Number of gray levels.
 * @param d The X display.
 * @param cmap The X colormap.
 * @param v The X visual.
 * @return A pointer to a flat array (DATA8*) containing the allocated pixel
 *         values for the gray ramp, from darkest to lightest.
 *         The size is ng. Returns NULL on failure.
 *         Example for ng=4: [Gray0, Gray1, Gray2, Gray3]
 */
static DATA8 *
x_color_alloc_gray(int ng, Display *d, Colormap cmap, Visual *v)
{
   int g, i;
   DATA8 *color_lut;
   int sig_mask = 0;

   for (i = 0; i < v->bits_per_rgb; i++)
     sig_mask |= (0x1 << i);
   sig_mask <<= (16 - v->bits_per_rgb);
   i = 0;
   color_lut = malloc(ng);
   if (!color_lut) return NULL;
   for (g = 0; g < (ng); g++)
     {
        XColor xcl;
        XColor xcl_in;
        int val;
        Status ret;

        val = (int)(((g * 255) / ((ng) - 1)));
        val = (val << 8) | val;
        xcl.red = (unsigned short)(val);
        xcl.green = (unsigned short)(val);
        xcl.blue = (unsigned short)(val);
        xcl.pixel = 0;
        xcl.flags = 0;
        xcl.pad = 0;
        xcl_in = xcl;
        ret = XAllocColor(d, cmap, &xcl);
        if ((ret == 0) ||
            ((xcl_in.red & sig_mask) != (xcl.red & sig_mask)) ||
            ((xcl_in.green & sig_mask) != (xcl.green & sig_mask)) ||
            ((xcl_in.blue & sig_mask) != (xcl.blue & sig_mask)))
          {
             unsigned long pixels[256];
             int j;

             if (i > 0)
               {
                  for (j = 0; j < i; j++)
                    pixels[j] = (unsigned long)color_lut[j];
                  XFreeColors(d, cmap, pixels, i, 0);
               }
             free(color_lut);
             return NULL;
          }
        color_lut[i] = xcl.pixel;
        i++;
     }
   return color_lut;
}

/** @brief Wrapper for x_color_alloc_rgb for 3:3:2 mode. */
static DATA8 *
x_color_alloc_rgb_332(Display *d, Colormap cmap, Visual *v)
{
   return x_color_alloc_rgb(8, 8, 4, d, cmap, v);
}

/** @brief Wrapper for x_color_alloc_rgb for 6:6:6 mode. */
static DATA8 *
x_color_alloc_rgb_666(Display *d, Colormap cmap, Visual *v)
{
   return x_color_alloc_rgb(6, 6, 6, d, cmap, v);
}

/** @brief Wrapper for x_color_alloc_rgb for 2:3:2 mode. */
static DATA8 *
x_color_alloc_rgb_232(Display *d, Colormap cmap, Visual *v)
{
   return x_color_alloc_rgb(4, 8, 4, d, cmap, v);
}

/** @brief Wrapper for x_color_alloc_rgb for 2:2:2 mode. */
static DATA8 *
x_color_alloc_rgb_222(Display *d, Colormap cmap, Visual *v)
{
   return x_color_alloc_rgb(4, 4, 4, d, cmap, v);
}

/** @brief Wrapper for x_color_alloc_rgb for 2:2:1 mode. */
static DATA8 *
x_color_alloc_rgb_221(Display *d, Colormap cmap, Visual *v)
{
   return x_color_alloc_rgb(4, 4, 2, d, cmap, v);
}

/** @brief Wrapper for x_color_alloc_rgb for 1:2:1 mode. */
static DATA8 *
x_color_alloc_rgb_121(Display *d, Colormap cmap, Visual *v)
{
   return x_color_alloc_rgb(2, 4, 2, d, cmap, v);
}

/** @brief Wrapper for x_color_alloc_rgb for 1:1:1 mode. */
static DATA8 *
x_color_alloc_rgb_111(Display *d, Colormap cmap, Visual *v)
{
   return x_color_alloc_rgb(2, 2, 2, d, cmap, v);
}

/** @brief Wrapper for x_color_alloc_gray for 256 gray levels. */
static DATA8 *
x_color_alloc_gray_256(Display *d, Colormap cmap, Visual *v)
{
   return x_color_alloc_gray(256, d, cmap, v);
}

/** @brief Wrapper for x_color_alloc_gray for 64 gray levels. */
static DATA8 *
x_color_alloc_gray_64(Display *d, Colormap cmap, Visual *v)
{
   return x_color_alloc_gray(64, d, cmap, v);
}

/** @brief Wrapper for x_color_alloc_gray for 16 gray levels. */
static DATA8 *
x_color_alloc_gray_16(Display *d, Colormap cmap, Visual *v)
{
   // Note: Allocates 32 levels internally, likely for better distribution?
   return x_color_alloc_gray(32, d, cmap, v);
}

/** @brief Wrapper for x_color_alloc_gray for 4 gray levels. */
static DATA8 *
x_color_alloc_gray_4(Display *d, Colormap cmap, Visual *v)
{
   // Note: Allocates 16 levels internally, likely for better distribution?
   return x_color_alloc_gray(16, d, cmap, v);
}

/** @brief Wrapper for x_color_alloc_gray for 2 gray levels (monochrome). */
static DATA8 *
x_color_alloc_mono(Display *d, Colormap cmap, Visual *v)
{
   return x_color_alloc_gray(2, d, cmap, v);
}

/**
 * @brief Initializes the Xlib color allocation function pointers and counts.
 *
 * Populates the static `x_color_alloc` and `x_color_count` arrays based on
 * the defined palette modes (Convert_Pal_Mode). This ensures that
 * evas_software_xlib_x_color_allocate() knows which function to call and how
 * many colors to expect for each mode. Uses a static flag `initialised` to
 * ensure it only runs once.
 */
void
evas_software_xlib_x_color_init(void)
{
   static int initialised = 0;

   if (initialised) return;
   x_color_alloc[PAL_MODE_NONE] = NULL;
   x_color_count[PAL_MODE_NONE] = 0;

   x_color_alloc[PAL_MODE_MONO] = x_color_alloc_mono;
   x_color_count[PAL_MODE_MONO] = 2;

   x_color_alloc[PAL_MODE_GRAY4] = x_color_alloc_gray_4;
   x_color_count[PAL_MODE_GRAY4] = 4;

   x_color_alloc[PAL_MODE_GRAY16] = x_color_alloc_gray_16;
   x_color_count[PAL_MODE_GRAY16] = 16;

   x_color_alloc[PAL_MODE_GRAY64] = x_color_alloc_gray_64;
   x_color_count[PAL_MODE_GRAY64] = 64;

   x_color_alloc[PAL_MODE_GRAY256] = x_color_alloc_gray_256;
   x_color_count[PAL_MODE_GRAY256] = 256;

   x_color_alloc[PAL_MODE_RGB111] = x_color_alloc_rgb_111;
   x_color_count[PAL_MODE_RGB111] = 2 * 2 * 2;

   x_color_alloc[PAL_MODE_RGB121] = x_color_alloc_rgb_121;
   x_color_count[PAL_MODE_RGB121] = 2 * 4 * 2;

   x_color_alloc[PAL_MODE_RGB221] = x_color_alloc_rgb_221;
   x_color_count[PAL_MODE_RGB221] = 4 * 4 * 2;

   x_color_alloc[PAL_MODE_RGB222] = x_color_alloc_rgb_222;
   x_color_count[PAL_MODE_RGB222] = 4 * 4 * 4;

   x_color_alloc[PAL_MODE_RGB232] = x_color_alloc_rgb_232;
   x_color_count[PAL_MODE_RGB232] = 4 * 8 * 4;

   x_color_alloc[PAL_MODE_RGB666] = x_color_alloc_rgb_666;
   x_color_count[PAL_MODE_RGB666] = 6 * 6 * 6;

   x_color_alloc[PAL_MODE_RGB332] = x_color_alloc_rgb_332;
   x_color_count[PAL_MODE_RGB332] = 8 * 8 * 4;

   x_color_alloc[PAL_MODE_LAST] = NULL;
   x_color_count[PAL_MODE_LAST] = 0;
   initialised = 1;
}

/**
 * @brief Allocates or retrieves a shared color palette.
 * @copydetails evas_software_xlib_x_color_allocate
 *
 * Implementation details:
 * - Iterates through the `palettes` list to find an existing match based on
 *   display, colormap, and visual. If found, increments reference count and returns.
 * - If no match, allocates a new `Convert_Pal` and `Convert_Pal_Priv`.
 * - Tries to allocate colors using the appropriate function from `x_color_alloc`,
 *   starting from the requested `colors` mode and decrementing if allocation fails.
 * - If successful, stores palette details, adds it to the `palettes` list, and returns it.
 * - Cleans up and returns NULL if allocation fails for all attempted modes.
 */
Convert_Pal *
evas_software_xlib_x_color_allocate(Display *disp,
                                    Colormap cmap,
                                    Visual *vis,
                                    Convert_Pal_Mode colors)
{
   Convert_Pal_Priv *palpriv;
   Convert_Pal *pal;
   Convert_Pal_Mode c;
   Eina_List *l;

/*   printf("ALLOC cmap=%i vis=%p\n", cmap, vis);*/
   EINA_LIST_FOREACH(palettes, l, pal)
     {
        palpriv = pal->data;
        if ((disp == palpriv->disp) &&
            (vis == palpriv->vis) &&
            (cmap == palpriv->cmap))
          {
             pal->references++;
             return pal;
          }
     }
   pal = calloc(1, sizeof(struct _Convert_Pal));
   if (!pal) return NULL;
   for (c = colors; c > PAL_MODE_NONE; c--)
     {
        if (x_color_alloc[c])
          {
/*	     printf("TRY PAL %i\n", c);*/
             pal->lookup = (x_color_alloc[c])(disp, cmap, vis);
             if (pal->lookup) break;
          }
     }
   pal->references = 1;
   pal->colors = c;
   pal->count = x_color_count[c];
   palpriv = calloc(1, sizeof(Convert_Pal_Priv));
   pal->data = palpriv;
   if (!palpriv)
     {
        if (pal->lookup) free(pal->lookup);
        free(pal);
        return NULL;
     }
   palpriv->disp = disp;
   palpriv->vis = vis;
   palpriv->cmap = cmap;
   if (pal->colors == PAL_MODE_NONE)
     {
        if (pal->lookup) free(pal->lookup);
        free(palpriv);
        free(pal);
        return NULL;
     }
   palettes = eina_list_append(palettes, pal);
   return pal;
}

/**
 * @brief Deallocates a reference to a color palette.
 * @copydetails evas_software_xlib_x_color_deallocate
 *
 * Implementation details:
 * - Decrements the `references` count in the `Convert_Pal` structure.
 * - If `references` is still greater than 0, returns immediately.
 * - If `references` reaches 0:
 *   - If a color lookup table (`pal->lookup`) exists, it collects the pixel
 *     values into an array.
 *   - Calls `XFreeColors` to release the allocated colors in the X server.
 *   - Frees the `pal->lookup` array.
 *   - Frees the private data structure `pal->data`.
 *   - Removes the palette from the global `palettes` list.
 *   - Frees the `Convert_Pal` structure itself.
 */
void
evas_software_xlib_x_color_deallocate(Display *disp,
                                      Colormap cmap,
                                      Visual *vis EINA_UNUSED, // Currently unused
                                      Convert_Pal *pal)
{
   unsigned long pixels[256];
   int j;

   pal->references--;
   if (pal->references > 0) return;
   if (pal->lookup)
     {
        for (j = 0; j < pal->count; j++)
          pixels[j] = (unsigned long)pal->lookup[j];
        XFreeColors(disp, cmap, pixels, pal->count, 0);
        free(pal->lookup);
     }
   free(pal->data);
   palettes = eina_list_remove(palettes, pal);
   free(pal);
}

