#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define EFL_ACCESS_OBJECT_PROTECTED
#define EFL_ACCESS_COMPONENT_PROTECTED
#define ELM_WIDGET_PROTECTED
#define ELM_WIDGET_ITEM_PROTECTED
#define EFL_INPUT_EVENT_PROTECTED
#define EFL_UI_L10N_PROTECTED
#define EFL_UI_FOCUS_OBJECT_PROTECTED
#define EFL_UI_WIDGET_PART_BG_PROTECTED
#define EFL_PART_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_container.h"
#include "elm_interface_scrollable.h"
#include "elm_pan_eo.h"
#include "elm_part_helper.h"
#include "elm_widget_combobox.h"

/**
 * @internal
 * @brief Structure to hold the state of a widget tree iterator.
 */
typedef struct {
   Eina_Iterator iterator; /**< The Eina_Iterator structure itself. */
   Efl_Ui_Widget *origin; /**< The widget where the iteration started. */
   Efl_Ui_Widget *current; /**< The current widget the iterator is pointing to. */
} Widget_Iterator;

/**
 * @internal
 * @brief Initializes a new Widget_Iterator.
 *
 * @param origin The widget from which the iteration will start.
 * @return A pointer to the newly allocated Widget_Iterator, or NULL on failure.
 */
static Widget_Iterator*
iter_init(Efl_Ui_Widget *origin)
{
   Widget_Iterator *it;

   it = calloc(1, sizeof(Widget_Iterator));

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);

   it->origin = origin;
   it->iterator.version = EINA_ITERATOR_VERSION;
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(NULL);
   it->iterator.free = FUNC_ITERATOR_FREE(free);

   return it;
}

/**
 * @internal
 * @brief Fetches the parent widget of a given Efl_Gfx_Entity.
 *
 * This function checks if the object is an Efl_Ui_Widget and calls
 * efl_ui_widget_parent_get, otherwise it retrieves the "elm-parent"
 * data. This is a compatibility layer.
 *
 * @param o The Efl_Gfx_Entity to get the parent from.
 * @return The parent Efl_Ui_Widget, or NULL if not found.
 */
static Efl_Ui_Widget*
_fetch_parent_widget(Efl_Gfx_Entity* o)
{
   Efl_Ui_Widget *parent;

   if (efl_isa(o, EFL_UI_WIDGET_CLASS))
     parent = efl_ui_widget_parent_get(o);
   else
     parent = evas_object_data_get(o, "elm-parent");

   return parent;
}

/**
 * @internal
 * @brief Finds the next sibling widget of a given Efl_Gfx_Entity.
 *
 * It first fetches the parent of @p o, then looks for @p o in the
 * parent's children list to find the next child.
 *
 * @param o The Efl_Gfx_Entity whose next sibling is to be found.
 * @return The next sibling Efl_Ui_Widget, or NULL if it's the last child or not found.
 */
static Efl_Ui_Widget*
_next_widget(Efl_Gfx_Entity* o)
{
   Efl_Ui_Widget *parent;

   parent = _fetch_parent_widget(o);
   ELM_WIDGET_DATA_GET_OR_RETURN(parent, pd, NULL);
   unsigned int id;
   if (eina_array_find(pd->children, o, &id) && id + 1 < eina_array_count(pd->children))
     return eina_array_data_get(pd->children, id + 1);
   else
     return NULL;
}

/**
 * @internal
 * @brief Advances the widget tree iterator to the next element.
 *
 * This function implements a depth-first traversal of the widget tree.
 * - If the current widget has children, it moves to the first child.
 * - If the current widget has no children, it moves to its next sibling.
 * - If there are no more siblings, it moves up to the parent and tries to find the parent's next sibling.
 * This process continues until the next widget is found or the entire subtree from the origin has been traversed.
 *
 * @param it The widget iterator.
 * @param data Pointer to store the next widget (Efl_Ui_Widget or Efl_Gfx_Entity).
 * @return EINA_TRUE if a next widget was found, EINA_FALSE otherwise.
 */
static Eina_Bool
_widget_next(Widget_Iterator *it, void **data)
{
   Efl_Ui_Widget *runner;
   Efl_Ui_Widget_Data *pd = NULL;

   //Init case
   if (!it->current)
     {
        it->current = it->origin;
        goto deliver;
     }

   if (efl_isa(it->current, EFL_UI_WIDGET_CLASS))
     {
        pd = efl_data_scope_safe_get(it->current, EFL_UI_WIDGET_CLASS);
        EINA_SAFETY_ON_NULL_RETURN_VAL(pd, EINA_FALSE);
     }

   //If there is a child, go there
   if (pd && eina_array_count(pd->children))
     {
        it->current = eina_array_data_get(pd->children, 0);
        goto deliver;
     }

   //If there is no child, then iterate up the parents until we find a widget with a next widget
   runner = it->current;
   do
     {
        Efl_Ui_Widget *tmp = _next_widget(runner);

        if (tmp)
          {
             it->current = tmp;
             goto deliver;
          }

        runner = _fetch_parent_widget(runner);
     }
   while(runner && runner != it->origin);

   //Reaching this point here means that there is no widget left, as there is no more parent we can explore
   it->current = NULL;

deliver:
   *data = (void*)it->current;
   return !!it->current;
}

EAPI Eina_Iterator*
efl_ui_widget_tree_iterator(Efl_Ui_Widget *obj)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, pd, NULL);
   Widget_Iterator *it = iter_init(obj);
   it->iterator.next = FUNC_ITERATOR_NEXT(_widget_next);

   return &it->iterator;
}

/**
 * @internal
 * @brief Filter function for eina_iterator_filter_new.
 *
 * Checks if the given data item is an Efl_Ui_Widget.
 *
 * @param container The iterator container (unused).
 * @param data The data item to check.
 * @param fdata User data for the filter (unused).
 * @return EINA_TRUE if data is an Efl_Ui_Widget, EINA_FALSE otherwise.
 */
static Eina_Bool
_only_widget(const void *container EINA_UNUSED, void *data, void *fdata EINA_UNUSED)
{
   return efl_isa(data, EFL_UI_WIDGET_CLASS);
}

EAPI Eina_Iterator*
efl_ui_widget_tree_widget_iterator(Efl_Ui_Widget *obj)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, pd, NULL);
   Eina_Iterator *tree_iterator = efl_ui_widget_tree_iterator(obj);

   return eina_iterator_filter_new(tree_iterator, _only_widget, NULL, NULL);
}

/**
 * @internal
 * @brief Advances the parent iterator to the next parent.
 *
 * If it's the first call, it returns the origin widget.
 * Otherwise, it returns the parent of the current widget.
 *
 * @param it The widget iterator.
 * @param data Pointer to store the next parent widget.
 * @return EINA_TRUE if a next parent was found, EINA_FALSE otherwise.
 */
static Eina_Bool
_parent_next(Widget_Iterator *it, void **data)
{
   if (!it->current)
     {
        *data = it->origin;
        it->current = *data;
     }
   else
     {
        Efl_Ui_Widget *parent = efl_ui_widget_parent_get(it->current);

        *data = parent;
        it->current = parent;
     }

   return !!*data;
}

EAPI Eina_Iterator*
efl_ui_widget_parent_iterator(Efl_Ui_Widget *obj)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, pd, NULL);
   Widget_Iterator *it = iter_init(obj);
   it->iterator.next = FUNC_ITERATOR_NEXT(_parent_next);
   return &it->iterator;
}
