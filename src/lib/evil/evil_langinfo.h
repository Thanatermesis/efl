/**
 * @file
 * @brief Language information constants and functions.
 *
 * This file defines constants and functions for retrieving
 * locale-specific information, similar to the POSIX langinfo.h.
 */
#ifndef __EVIL_LANGINFO_H__
#define __EVIL_LANGINFO_H__


#include <locale.h>

/**
 * @typedef nl_item
 * @brief Type for specifying a locale information item.
 *
 * This integer type is used to identify specific pieces of
 * locale-dependent information to be retrieved by nl_langinfo().
 * The values for nl_item are constructed using macros like __NL_ITEM().
 */
typedef int            nl_item;

/**
 * @def __NL_ITEM(CATEGORY, INDEX)
 * @brief Macro to create an nl_item value.
 *
 * Combines a category and an index to form a unique nl_item.
 * The category is shifted left by 16 bits and ORed with the index.
 *
 * @param CATEGORY The locale category (e.g., LC_CTYPE, LC_NUMERIC).
 * @param INDEX The specific item index within the category.
 * @return An nl_item value.
 */
#define __NL_ITEM( CATEGORY, INDEX )  ((CATEGORY << 16) | INDEX)
/**
 * @def __NL_ITEM_CATEGORY(ITEM)
 * @brief Macro to extract the category from an nl_item value.
 *
 * @param ITEM The nl_item value.
 * @return The category part of the nl_item.
 */
#define __NL_ITEM_CATEGORY( ITEM )    (ITEM >> 16)
/**
 * @def __NL_ITEM_INDEX(ITEM)
 * @brief Macro to extract the index from an nl_item value.
 *
 * @param ITEM The nl_item value.
 * @return The index part of the nl_item.
 */
#define __NL_ITEM_INDEX( ITEM )       (ITEM & 0xffff)

/**
 * @enum
 * @brief Enumeration of locale information items.
 *
 * Defines symbolic constants for various locale-specific data
 * that can be queried using nl_langinfo().
 */
enum {
  /*
   * LC_CTYPE category...
   * Character set classification items.
   */
  _NL_CTYPE_CODESET     = __NL_ITEM( LC_CTYPE, 0 ), /**< Character set name. Example: "UTF-8". */
  _NL_NUMERIC_RADIXCHAR = __NL_ITEM( LC_NUMERIC, 0 ), /**< Radix character (decimal point). Example: ".". */

  D_T_FMT, /**< String for formatting date and time for strftime(). Example: "%a %d %b %Y %T %Z". */
#define D_T_FMT D_T_FMT
  D_FMT,   /**< String for formatting date for strftime(). Example: "%m/%d/%Y". */
#define D_FMT D_FMT
  T_FMT,   /**< String for formatting time for strftime(). Example: "%T". */
#define T_FMT T_FMT
  T_FMT_AMPM,  /**< String for formatting 12-hour time with AM/PM for strftime(). Example: "%r". */
#define T_FMT_AMPM T_FMT_AMPM
  /*
   * Dummy entry, to terminate the list.
   */
  _NL_ITEM_CLASSIFICATION_END /**< Marks the end of the nl_item list. */
};

/*
 * Define the public aliases for the enumerated classification indices...
 */
/**
 * @def CODESET
 * @brief Alias for _NL_CTYPE_CODESET.
 * Represents the character set name.
 */
# define CODESET       _NL_CTYPE_CODESET
/**
 * @def RADIXCHAR
 * @brief Alias for _NL_NUMERIC_RADIXCHAR.
 * Represents the radix character (e.g., decimal point).
 */
# define RADIXCHAR     _NL_NUMERIC_RADIXCHAR

/**
 * @brief Retrieve locale-specific information.
 *
 * This function returns a string containing information about the
 * program's current locale, as specified by the `index` argument.
 *
 * @param index An nl_item constant specifying the information to retrieve.
 *              For example, CODESET for the codeset name, or RADIXCHAR
 *              for the decimal separator.
 * @return A pointer to a string containing the requested information.
 *         The string is statically allocated and may be overwritten by
 *         subsequent calls. If the item is not available or the index is
 *         invalid, it may return an empty string or a default value.
 *         The caller should not attempt to free this string.
 */
EVIL_API char *nl_langinfo(nl_item index);


#endif /*__EVIL_LANGINFO_H__ */
