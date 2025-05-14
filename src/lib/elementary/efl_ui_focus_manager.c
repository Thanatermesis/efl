#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"

/**
 * @brief Frees the memory allocated for focus relations.
 *
 * This function deallocates the provided Efl_Ui_Focus_Relations structure
 * and the eina_iterators it contains for right, left, top, and down relations.
 *
 * @param rel A pointer to the Efl_Ui_Focus_Relations structure to be freed.
 *            This structure typically holds iterators to UI elements that are
 *            logically positioned relative to a focused element.
 *            For example:
 *            rel->right might be an iterator over elements to the right.
 *            rel->left might be an iterator over elements to the left.
 *            rel->top might be an iterator over elements above.
 *            rel->down might be an iterator over elements below.
 */
EAPI void
efl_ui_focus_relation_free(Efl_Ui_Focus_Relations *rel)
{
   eina_iterator_free(rel->right);
   eina_iterator_free(rel->left);
   eina_iterator_free(rel->top);
   eina_iterator_free(rel->down);
   free(rel);
}

#include "efl_ui_focus_manager.eo.c"
#include "efl_ui_focus_manager_window_root.eo.c"
