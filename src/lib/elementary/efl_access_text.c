#ifdef HAVE_CONFIG_H
  #include "elementary_config.h"
#endif

#define EFL_ACCESS_TEXT_PROTECTED

#include "elm_priv.h"

/**
 * @internal
 * @brief Frees an Efl_Access_Text_Attribute structure and its associated data.
 *
 * This function releases the memory allocated for an Efl_Access_Text_Attribute
 * structure. It specifically handles the `name` and `value` members, which are
 * expected to be Eina_Stringshare instances, by calling `eina_stringshare_del`
 * on them if they are not NULL. Finally, it frees the structure itself.
 *
 * @param[in] attr A pointer to the Efl_Access_Text_Attribute structure to be freed.
 *                 If @p attr is @c NULL, the function does nothing.
 */
void
EAPI elm_atspi_text_text_attribute_free(Efl_Access_Text_Attribute *attr)
{
   if (!attr) return;
   if (attr->name) eina_stringshare_del(attr->name);
   if (attr->value) eina_stringshare_del(attr->value);
   free(attr);
}

/**
 * @internal
 * @brief Frees an Efl_Access_Text_Range structure and its associated content.
 *
 * This function releases the memory allocated for an Efl_Access_Text_Range
 * structure. It first frees the `content` string within the range structure,
 * and then frees the structure itself.
 *
 * @param[in] range A pointer to the Efl_Access_Text_Range structure to be freed.
 *                  It is assumed that @p range is a valid pointer if not NULL.
 *                  `free(NULL)` is a no-op, so if @p range is NULL,
 *                  `range->content` would not be accessed and `free(range)`
 *                  would do nothing. However, typical usage implies @p range
 *                  is non-NULL if its members are to be accessed.
 */
EAPI void
elm_atspi_text_text_range_free(Efl_Access_Text_Range *range)
{
   // The cast to (char*) is present because range->content might be
   // const char* in the struct definition for API reasons, but known
   // to be dynamically allocated mutable memory here.
   free((char*)range->content);
   free(range);
}

#include "efl_access_text.eo.c"
