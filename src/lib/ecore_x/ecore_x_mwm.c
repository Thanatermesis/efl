/*
 * Various MWM related functions.
 *
 * This is ALL the code involving anything MWM related. for both WM and
 * client.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdlib.h>

#include "Ecore.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"
#include "Ecore_X_Atoms.h"

#define ECORE_X_MWM_HINTS_FUNCTIONS   (1 << 0)
#define ECORE_X_MWM_HINTS_DECORATIONS (1 << 1)
#define ECORE_X_MWM_HINTS_INPUT_MODE  (1 << 2)
#define ECORE_X_MWM_HINTS_STATUS      (1 << 3)

/**
 * @brief Structure to hold MWM hints.
 * This structure is used to interpret the _MOTIF_WM_HINTS property.
 */
typedef struct _mwmhints
{
   CARD32 flags;       /**< Indicates which fields are valid */
   CARD32 functions;   /**< Defines which MWM functions are available */
   CARD32 decorations; /**< Defines which MWM decorations are applied */
   INT32  inputmode;   /**< Defines the MWM input mode */
   CARD32 status;      /**< Reserved for future MWM use */
}
MWMHints;

/**
 * @brief Retrieves the MWM (Motif Window Manager) hints for a given window.
 *
 * This function fetches and interprets the _MOTIF_WM_HINTS property
 * from the specified window. It populates the provided pointers with the
 * corresponding hint values if they are set. If a hint is not set, a
 * default value is provided.
 *
 * @param win The Ecore_X_Window to get the MWM hints from.
 * @param fhint Pointer to store the function hint (e.g., ECORE_X_MWM_HINT_FUNC_ALL, ECORE_X_MWM_HINT_FUNC_RESIZE).
 *              If NULL, this hint is not retrieved.
 * @param dhint Pointer to store the decoration hint (e.g., ECORE_X_MWM_HINT_DECOR_ALL, ECORE_X_MWM_HINT_DECOR_BORDER).
 *              If NULL, this hint is not retrieved.
 * @param ihint Pointer to store the input mode hint (e.g., ECORE_X_MWM_HINT_INPUT_MODELESS, ECORE_X_MWM_HINT_INPUT_PRIMARY_APPLICATION_MODAL).
 *              If NULL, this hint is not retrieved.
 * @return EINA_TRUE if the hints were successfully retrieved or defaults applied,
 *         EINA_FALSE otherwise (e.g., if the property does not exist or is malformed).
 */
EAPI Eina_Bool
ecore_x_mwm_hints_get(Ecore_X_Window win,
                      Ecore_X_MWM_Hint_Func *fhint,
                      Ecore_X_MWM_Hint_Decor *dhint,
                      Ecore_X_MWM_Hint_Input *ihint)
{
   unsigned char *p = NULL;
   MWMHints *mwmhints = NULL;
   int num;
   Eina_Bool ret;

   LOGFN;
   ret = EINA_FALSE;
   if (!ecore_x_window_prop_property_get(win,
                                         ECORE_X_ATOM_MOTIF_WM_HINTS,
                                         ECORE_X_ATOM_MOTIF_WM_HINTS,
                                         32, &p, &num))
     {
        if (p) free(p);
        return EINA_FALSE;
     }

   mwmhints = (MWMHints *)p;
   if (mwmhints)
     {
        if (num >= 4)
          {
             if (dhint)
               {
                  if (mwmhints->flags & ECORE_X_MWM_HINTS_DECORATIONS)
                    *dhint = mwmhints->decorations;
                  else
                    *dhint = ECORE_X_MWM_HINT_DECOR_ALL;
               }

             if (fhint)
               {
                  if (mwmhints->flags & ECORE_X_MWM_HINTS_FUNCTIONS)
                    *fhint = mwmhints->functions;
                  else
                    *fhint = ECORE_X_MWM_HINT_FUNC_ALL;
               }

             if (ihint)
               {
                  if (mwmhints->flags & ECORE_X_MWM_HINTS_INPUT_MODE)
                    *ihint = mwmhints->inputmode;
                  else
                    *ihint = ECORE_X_MWM_HINT_INPUT_MODELESS;
               }

             ret = EINA_TRUE;
          }

        free(mwmhints);
     }

   return ret;
}

/**
 * @brief Sets the MWM (Motif Window Manager) hints to make a window borderless or bordered.
 *
 * This function modifies the _MOTIF_WM_HINTS property on the specified window
 * to request that the window manager remove or add window decorations (borders, title bar, etc.).
 *
 * The MWM hints structure is defined as:
 * typedef struct {
 *     CARD32  flags;
 *     CARD32  functions;
 *     CARD32  decorations;
 *     INT32   inputMode;
 *     CARD32  status;
 * } PropMotifWmHints;
 *
 * This function sets:
 * - flags to ECORE_X_MWM_HINTS_DECORATIONS (which is (1L << 1), or 2)
 * - decorations to 0 if borderless is EINA_TRUE (requesting no decorations),
 *   or to ECORE_X_MWM_HINT_DECOR_ALL (which is (1L << 0), or 1) if borderless is EINA_FALSE.
 * The other fields (functions, inputMode, status) are set to 0, meaning the WM should use defaults.
 *
 * The data array passed to ecore_x_window_prop_property_set will be:
 * data[0] = flags (ECORE_X_MWM_HINTS_DECORATIONS, i.e., 2)
 * data[1] = functions (0)
 * data[2] = decorations (0 for borderless, 1 for bordered)
 * data[3] = inputMode (0)
 * data[4] = status (0)
 *
 * @param win The Ecore_X_Window to modify.
 * @param borderless If EINA_TRUE, request a borderless window.
 *                   If EINA_FALSE, request a bordered window (default decorations).
 */
EAPI void
ecore_x_mwm_borderless_set(Ecore_X_Window win,
                           Eina_Bool borderless)
{
   unsigned int data[5] = {0, 0, 0, 0, 0};

   data[0] = 2; /* just set the decorations hint! */
   data[2] = !borderless;

   LOGFN;
   ecore_x_window_prop_property_set(win,
                                    ECORE_X_ATOM_MOTIF_WM_HINTS,
                                    ECORE_X_ATOM_MOTIF_WM_HINTS,
                                    32, (void *)data, 5);
}

