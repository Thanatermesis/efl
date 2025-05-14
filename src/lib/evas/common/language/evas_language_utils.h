#ifndef _EVAS_LANGUAGE_UTILS
#define _EVAS_LANGUAGE_UTILS

#include <Eina.h>
#include "evas_bidi_utils.h"

/**
 * @brief Enumeration of Unicode script properties.
 *
 * These values represent the script of a character as defined by the
 * Unicode standard.
 */
typedef enum
{
  EVAS_SCRIPT_COMMON       = 0,   /**< Characters used across many scripts (e.g., punctuation) */
  EVAS_SCRIPT_INHERITED,          /**< Characters that inherit script from preceding characters */
  EVAS_SCRIPT_ARABIC,             /**< Arabic script */
  EVAS_SCRIPT_ARMENIAN,           /**< Armenian script */
  EVAS_SCRIPT_BENGALI,            /**< Bengali script */
  EVAS_SCRIPT_BOPOMOFO,           /**< Bopomofo script */
  EVAS_SCRIPT_CHEROKEE,           /**< Cherokee script */
  EVAS_SCRIPT_COPTIC,             /**< Coptic script */
  EVAS_SCRIPT_CYRILLIC,           /**< Cyrillic script */
  EVAS_SCRIPT_DESERET,            /**< Deseret script */
  EVAS_SCRIPT_DEVANAGARI,         /**< Devanagari script */
  EVAS_SCRIPT_ETHIOPIC,           /**< Ethiopic script */
  EVAS_SCRIPT_GEORGIAN,           /**< Georgian script */
  EVAS_SCRIPT_GOTHIC,             /**< Gothic script */
  EVAS_SCRIPT_GREEK,              /**< Greek script */
  EVAS_SCRIPT_GUJARATI,           /**< Gujarati script */
  EVAS_SCRIPT_GURMUKHI,           /**< Gurmukhi script */
  EVAS_SCRIPT_HAN,                /**< Han script (Chinese, Japanese, Korean ideographs) */
  EVAS_SCRIPT_HANGUL,             /**< Hangul script (Korean) */
  EVAS_SCRIPT_HEBREW,             /**< Hebrew script */
  EVAS_SCRIPT_HIRAGANA,           /**< Hiragana script (Japanese) */
  EVAS_SCRIPT_KANNADA,            /**< Kannada script */
  EVAS_SCRIPT_KATAKANA,           /**< Katakana script (Japanese) */
  EVAS_SCRIPT_KHMER,              /**< Khmer script */
  EVAS_SCRIPT_LAO,                /**< Lao script */
  EVAS_SCRIPT_LATIN,              /**< Latin script */
  EVAS_SCRIPT_MALAYALAM,          /**< Malayalam script */
  EVAS_SCRIPT_MONGOLIAN,          /**< Mongolian script */
  EVAS_SCRIPT_MYANMAR,            /**< Myanmar script */
  EVAS_SCRIPT_OGHAM,              /**< Ogham script */
  EVAS_SCRIPT_OLD_ITALIC,         /**< Old Italic script */
  EVAS_SCRIPT_ORIYA,              /**< Oriya script */
  EVAS_SCRIPT_RUNIC,              /**< Runic script */
  EVAS_SCRIPT_SINHALA,            /**< Sinhala script */
  EVAS_SCRIPT_SYRIAC,             /**< Syriac script */
  EVAS_SCRIPT_TAMIL,              /**< Tamil script */
  EVAS_SCRIPT_TELUGU,             /**< Telugu script */
  EVAS_SCRIPT_THAANA,             /**< Thaana script */
  EVAS_SCRIPT_THAI,               /**< Thai script */
  EVAS_SCRIPT_TIBETAN,            /**< Tibetan script */
  EVAS_SCRIPT_CANADIAN_ABORIGINAL, /**< Canadian Aboriginal Syllabics script */
  EVAS_SCRIPT_YI,                 /**< Yi script */
  EVAS_SCRIPT_TAGALOG,            /**< Tagalog script */
  EVAS_SCRIPT_HANUNOO,            /**< Hanunoo script */
  EVAS_SCRIPT_BUHID,              /**< Buhid script */
  EVAS_SCRIPT_TAGBANWA,           /**< Tagbanwa script */

  /* Unicode-4.0 additions */
  EVAS_SCRIPT_BRAILLE,            /**< Braille script */
  EVAS_SCRIPT_CYPRIOT,            /**< Cypriot script */
  EVAS_SCRIPT_LIMBU,              /**< Limbu script */
  EVAS_SCRIPT_OSMANYA,            /**< Osmanya script */
  EVAS_SCRIPT_SHAVIAN,            /**< Shavian script */
  EVAS_SCRIPT_LINEAR_B,           /**< Linear B script */
  EVAS_SCRIPT_TAI_LE,             /**< Tai Le script */
  EVAS_SCRIPT_UGARITIC,           /**< Ugaritic script */

  /* Unicode-4.1 additions */
  EVAS_SCRIPT_NEW_TAI_LUE,        /**< New Tai Lue script */
  EVAS_SCRIPT_BUGINESE,           /**< Buginese script */
  EVAS_SCRIPT_GLAGOLITIC,         /**< Glagolitic script */
  EVAS_SCRIPT_TIFINAGH,           /**< Tifinagh script */
  EVAS_SCRIPT_SYLOTI_NAGRI,       /**< Syloti Nagri script */
  EVAS_SCRIPT_OLD_PERSIAN,        /**< Old Persian script */
  EVAS_SCRIPT_KHAROSHTHI,         /**< Kharoshthi script */

  /* Unicode-5.0 additions */
  EVAS_SCRIPT_UNKNOWN,            /**< Unknown script */
  EVAS_SCRIPT_BALINESE,           /**< Balinese script */
  EVAS_SCRIPT_CUNEIFORM,          /**< Cuneiform script */
  EVAS_SCRIPT_PHOENICIAN,         /**< Phoenician script */
  EVAS_SCRIPT_PHAGS_PA,           /**< Phags-pa script */
  EVAS_SCRIPT_NKO,                /**< N'Ko script */

  /* Unicode-5.1 additions */
  EVAS_SCRIPT_KAYAH_LI,           /**< Kayah Li script */
  EVAS_SCRIPT_LEPCHA,             /**< Lepcha script */
  EVAS_SCRIPT_REJANG,             /**< Rejang script */
  EVAS_SCRIPT_SUNDANESE,          /**< Sundanese script */
  EVAS_SCRIPT_SAURASHTRA,         /**< Saurashtra script */
  EVAS_SCRIPT_CHAM,               /**< Cham script */
  EVAS_SCRIPT_OL_CHIKI,           /**< Ol Chiki script */
  EVAS_SCRIPT_VAI,                /**< Vai script */
  EVAS_SCRIPT_CARIAN,             /**< Carian script */
  EVAS_SCRIPT_LYCIAN,             /**< Lycian script */
  EVAS_SCRIPT_LYDIAN,             /**< Lydian script */

  /* Unicode-5.2 additions */
  EVAS_SCRIPT_AVESTAN,                /**< Avestan script */
  EVAS_SCRIPT_BAMUM,                  /**< Bamum script */
  EVAS_SCRIPT_EGYPTIAN_HIEROGLYPHS,   /**< Egyptian Hieroglyphs script */
  EVAS_SCRIPT_IMPERIAL_ARAMAIC,       /**< Imperial Aramaic script */
  EVAS_SCRIPT_INSCRIPTIONAL_PAHLAVI,  /**< Inscriptional Pahlavi script */
  EVAS_SCRIPT_INSCRIPTIONAL_PARTHIAN, /**< Inscriptional Parthian script */
  EVAS_SCRIPT_JAVANESE,               /**< Javanese script */
  EVAS_SCRIPT_KAITHI,                 /**< Kaithi script */
  EVAS_SCRIPT_LISU,                   /**< Lisu script */
  EVAS_SCRIPT_MEETEI_MAYEK,           /**< Meetei Mayek script */
  EVAS_SCRIPT_OLD_SOUTH_ARABIAN,      /**< Old South Arabian script */
  EVAS_SCRIPT_OLD_TURKIC,             /**< Old Turkic script */
  EVAS_SCRIPT_SAMARITAN,              /**< Samaritan script */
  EVAS_SCRIPT_TAI_THAM,               /**< Tai Tham script */
  EVAS_SCRIPT_TAI_VIET,                /**< Tai Viet script */

  /* Unicode-6.0 additions */
  EVAS_SCRIPT_BATAK,                  /**< Batak script */
  EVAS_SCRIPT_BRAHMI,                 /**< Brahmi script */
  EVAS_SCRIPT_MANDAIC,                /**< Mandaic script */
} Evas_Script_Type;

/**
 * @brief Determines the end of a script run within a string.
 *
 * This function identifies the length of a contiguous run of characters
 * belonging to the same script, starting from a given position. It also
 * considers BiDirectional properties to correctly segment runs.
 *
 * @param str The Unicode string to analyze.
 * @param bidi_props Bidirectional properties of the paragraph containing the string.
 * @param start The starting index within the string to begin analysis.
 * @param len The maximum length of the string segment to consider from the start index.
 * @return The length of the script run from the start index. Returns 0 if the
 *         end of the provided length is reached or if no explicit script is found.
 */
int
evas_common_language_script_end_of_run_get(const Eina_Unicode *str, const Evas_BiDi_Paragraph_Props *bidi_props, size_t start, int len);

/**
 * @brief Gets the dominant script type for a given Unicode string.
 *
 * It iterates through the string until an explicit script is found.
 * If no explicit script is found, EVAS_SCRIPT_COMMON is returned.
 *
 * @param str The Unicode string.
 * @param len The length of the string.
 * @return The dominant Evas_Script_Type in the string.
 */
Evas_Script_Type
evas_common_language_script_type_get(const Eina_Unicode *str, size_t len);

/**
 * @brief Gets the script type of a single Unicode character.
 *
 * @param unicode The Unicode character.
 * @return The Evas_Script_Type of the character.
 */
Evas_Script_Type
evas_common_language_char_script_get(Eina_Unicode unicode);

/**
 * @brief Gets the language code from the current locale.
 *
 * Retrieves the language part (e.g., "en" from "en_US.UTF-8") of the
 * LC_MESSAGES locale. The result is cached for subsequent calls.
 *
 * @return A pointer to a statically allocated string containing the
 *         language code (e.g., "en", "he"). Returns an empty string if
 *         the locale cannot be determined. The returned string should not
 *         be modified or freed.
 */
const char *
evas_common_language_from_locale_get(void);

/**
 * @brief Gets the full language and territory code from the current locale.
 *
 * Retrieves the language and territory part (e.g., "en_US" from "en_US.UTF-8")
 * of the LC_MESSAGES locale. The result is cached for subsequent calls.
 *
 * @return A pointer to a statically allocated string containing the
 *         full language code (e.g., "en_US", "he_IL"). Returns an empty
 *         string if the locale cannot be determined. The returned string
 *         should not be modified or freed.
 */
const char *
evas_common_language_from_locale_full_get(void);

/**
 * @brief Gets the default writing direction based on the current locale's language.
 *
 * This function determines if the default language (obtained via dgettext)
 * is right-to-left (RTL) or left-to-right (LTR). The result is cached.
 *
 * @return EVAS_BIDI_DIRECTION_RTL if the language is RTL,
 *         EVAS_BIDI_DIRECTION_LTR if LTR. Defaults to LTR if undetermined.
 */
Evas_BiDi_Direction
evas_common_language_direction_get(void);

/**
 * @brief Reinitializes cached language and direction information.
 *
 * This function clears any cached locale-dependent information, such as
 * language code and writing direction. Subsequent calls to functions like
 * evas_common_language_from_locale_get() or evas_common_language_direction_get()
 * will re-query the system locale. This is useful if the locale changes
 * during runtime.
 */
void
evas_common_language_reinit(void);
#endif

