/**
 * @file
 * @brief Evas Vector Graphics (VG) loader implementation for SVG files.
 *
 * This file contains the logic for parsing SVG files and converting them
 * into an internal representation suitable for rendering with Evas VG engine.
 * It handles various SVG elements, attributes, styles, transformations,
 * gradients, and structure.
 */
#include "vg_common.h"

static int _evas_vg_loader_svg_log_dom = -1; /**< Log domain for SVG loader messages. */

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_vg_loader_svg_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_vg_loader_svg_log_dom, __VA_ARGS__)

/**
 * @brief Holds global parsing state and context for SVG elements.
 *
 * This structure stores information relevant across the parsing of the entire
 * SVG document, such as the global viewport dimensions and temporary state
 * for parsing specific elements like gradients.
 */
typedef struct _Evas_SVG_Parser Evas_SVG_Parser;
/**
 * @struct _Evas_SVG_Parser
 * @brief Holds global parsing state and context for SVG elements.
 *
 * This structure stores information relevant across the parsing of the entire
 * SVG document, such as the global viewport dimensions and temporary state
 * for parsing specific elements like gradients.
 */
struct _Evas_SVG_Parser {
   /** @brief Global viewport dimensions extracted from the <svg> tag. */
   struct {
      int x;      /**< Viewport x coordinate. */
      int y;      /**< Viewport y coordinate. */
      int width;  /**< Viewport width. */
      int height; /**< Viewport height. */
   } global;
   /** @brief State specific to parsing gradient elements. */
   struct {
      Eina_Bool fx_parsed; /**< Flag indicating if 'fx' attribute was parsed for radial gradient. */
      Eina_Bool fy_parsed; /**< Flag indicating if 'fy' attribute was parsed for radial gradient. */
   } gradient;

   Svg_Node *node;                   /**< Pointer to the currently parsed SVG node being processed. */
   Svg_Style_Gradient *style_grad; /**< Pointer to the gradient style currently being parsed. */
   Efl_Gfx_Gradient_Stop *grad_stop; /**< Pointer to the gradient stop currently being parsed. */
};

/**
 * @brief Manages the state during the SVG loading process.
 *
 * This structure holds the necessary context for parsing an SVG file,
 * including the node stack, document root, definitions, gradients,
 * and the global parser state.
 */
typedef struct _Evas_SVG_Loader Evas_SVG_Loader;
/**
 * @struct _Evas_SVG_Loader
 * @brief Manages the state during the SVG loading process.
 */
struct _Evas_SVG_Loader
{
   Eina_Array *stack;                   /**< Stack to keep track of nested SVG elements during parsing. */
   Svg_Node *doc;                       /**< Root node of the parsed SVG document structure. */
   Svg_Node *def;                       /**< Pointer to the <defs> node, holding reusable definitions. */
   Eina_List *gradients;                /**< List of gradients defined outside the <defs> section. */
   Svg_Style_Gradient *latest_gradient; /**< Pointer to the most recently parsed gradient, used for associating stops. */
   Evas_SVG_Parser *svg_parse;          /**< Global parser state and context. */
   int level;                           /**< Current nesting level in the XML structure. */
   Eina_Bool result:1;                  /**< Flag indicating the overall success of the parsing process. */
};

/**
 * @typedef Factory_Method
 * @brief Function pointer type for creating specific SVG node types.
 * @param loader The SVG loader context.
 * @param parent The parent node in the SVG tree.
 * @param buf The buffer containing the element's attributes.
 * @param buflen The length of the attribute buffer.
 * @return A pointer to the newly created Svg_Node, or NULL on failure.
 */
typedef Svg_Node *(*Factory_Method)(Evas_SVG_Loader *loader, Svg_Node *parent, const char *buf, unsigned buflen);

/**
 * @typedef Gradient_Factory_Method
 * @brief Function pointer type for creating specific SVG gradient types.
 * @param loader The SVG loader context.
 * @param buf The buffer containing the gradient element's attributes.
 * @param buflen The length of the attribute buffer.
 * @return A pointer to the newly created Svg_Style_Gradient, or NULL on failure.
 */
typedef Svg_Style_Gradient *(*Gradient_Factory_Method)(Evas_SVG_Loader *loader, const char *buf, unsigned buflen);

/**
 * @enum SVG_Parser_Length_Type
 * @brief Specifies the context for interpreting percentage length values.
 *
 * Used by _to_double and _gradient_to_double to correctly calculate
 * percentage values based on the viewport width, height, or diagonal.
 */
typedef enum {
   SVG_PARSER_LENGTH_VERTICAL,   /**< Length relates to the height of the viewport or bounding box. */
   SVG_PARSER_LENGTH_HORIZONTAL, /**< Length relates to the width of the viewport or bounding box. */
   /**
    * @brief Length relates to neither width nor height specifically.
    * Used for values like gradient radii, often calculated based on diagonal length.
    */
   SVG_PARSER_LENGTH_OTHER
} SVG_Parser_Length_Type;

/**
 * @brief Skips leading whitespace characters in a string segment.
 * @param str The input string.
 * @param end Pointer to the character after the last valid character in str, or NULL if str is null-terminated.
 * @return Pointer to the first non-whitespace character, or the end of the string/segment.
 */
char *
_skip_space(const char *str, const char *end)
{
   while (((end != NULL && str < end) || (end == NULL && *str != '\0')) && isspace(*str))
     ++str;
   return (char *)str;
}

/**
 * @brief Creates a shared string instance for an ID attribute.
 * @param str The ID string to share.
 * @return A new Eina_Stringshare instance, or NULL if str is NULL.
 */
static inline Eina_Stringshare *
_copy_id(const char* str)
{
   if (str == NULL) return NULL;

   return eina_stringshare_add(str);
}

/**
 * @brief Skips optional whitespace and a comma in a string.
 * @param content The input string.
 * @return Pointer to the character after the skipped comma and whitespace,
 *         or the original pointer if no comma was found.
 */
static const char *
_skipcomma(const char *content)
{
   content = _skip_space(content, NULL);
   if (*content == ',') return content + 1;
   return content;
}

/**
 * @brief Parses a floating-point number from the beginning of a string.
 *
 * Advances the content pointer past the parsed number and any trailing comma/whitespace.
 *
 * @param content Pointer to the string pointer to parse from. This will be updated.
 * @param number Pointer to a double where the parsed number will be stored.
 * @return EINA_TRUE if a number was successfully parsed, EINA_FALSE otherwise.
 */
static inline Eina_Bool
_parse_number(const char **content, double *number)
{
   char *end = NULL;

   *number = eina_convert_strtod_c(*content, &end);
   // if the start of string is not number
   if ((*content) == end) return EINA_FALSE;
   //skip comma if any
   *content = _skipcomma(end);
   return EINA_TRUE;
}

/**
 * According to https://www.w3.org/TR/SVG/coords.html#Units
 *
 * TODO
 * Since this documentation is not obvious, more clean recalculation with dpi
 * is required, but for now default w3 constants would be used.
 * Assumes 90 DPI based on CSS Values and Units Module Level 3 recommendation.
 * (1in = 90px, 1cm = 35.43307px, 1mm = 3.543307px, 1pt = 1.25px, 1pc = 15px)
 */
static inline double
_to_double(Evas_SVG_Parser *svg_parse, const char *str, SVG_Parser_Length_Type type)
{
   double parsed_value = eina_convert_strtod_c(str, NULL);

   if (strstr(str, "cm"))
     parsed_value = parsed_value * 35.43307;
   else if (strstr(str, "mm"))
     parsed_value = parsed_value * 3.543307;
   else if (strstr(str, "pt"))
     parsed_value = parsed_value * 1.25;
   else if (strstr(str, "pc"))
     parsed_value = parsed_value * 15;
   else if (strstr(str, "in"))
     parsed_value = parsed_value * 90;
   else if (strstr(str, "%"))
     {
        if (type == SVG_PARSER_LENGTH_VERTICAL)
          parsed_value = (parsed_value / 100.0) * svg_parse->global.height;
        else if (type == SVG_PARSER_LENGTH_HORIZONTAL)
          parsed_value = (parsed_value / 100.0) * svg_parse->global.width;
        else // if other then it's radius
          {
             double max = svg_parse->global.width;
             if (max < svg_parse->global.height) max = svg_parse->global.height;
             parsed_value = (parsed_value / 100.0) * max;
          }
     }

   //TODO: implement 'em', 'ex' attributes

   return parsed_value;
}

/**
 * @brief Converts a string representation of a length (potentially with units)
 *        into a double value, interpreted as a percentage relative to the
 *        SVG viewport dimensions.
 *
 * This is specifically used for gradient attributes like cx, cy, fx, fy, r,
 * x1, y1, x2, y2 when gradientUnits="objectBoundingBox" (the default).
 * It handles units like %, cm, mm, pt, pc, in and converts them to a
 * percentage (0.0 to 1.0) based on the specified length type (horizontal,
 * vertical, or other).
 *
 * @param svg_parse The global SVG parser state containing viewport dimensions.
 * @param str The string containing the length value (e.g., "50%", "10pt").
 * @param type The type of length, determining the reference dimension for percentages.
 * @return The calculated length as a percentage (double).
 */
static inline double
_gradient_to_double(Evas_SVG_Parser *svg_parse, const char *str, SVG_Parser_Length_Type type)
{
   char *end = NULL;

   double parsed_value = eina_convert_strtod_c(str, &end);
   double max = 1;

   /**
    * That is according to Units in here
    *
    * https://www.w3.org/TR/2015/WD-SVG2-20150915/coords.html
    */
   if (type == SVG_PARSER_LENGTH_VERTICAL)
     max = svg_parse->global.height;
   else if (type == SVG_PARSER_LENGTH_HORIZONTAL)
     max = svg_parse->global.width;
   else if (type == SVG_PARSER_LENGTH_OTHER)
     max = sqrt(pow(svg_parse->global.height, 2) +
                pow(svg_parse->global.width, 2)) / sqrt(2.0);

   if (strstr(str, "%"))
     parsed_value = parsed_value / 100.0;
   else if (strstr(str, "cm"))
     parsed_value = parsed_value * 35.43307;
   else if (strstr(str, "mm"))
     parsed_value = parsed_value * 3.543307;
   else if (strstr(str, "pt"))
     parsed_value = parsed_value * 1.25;
   else if (strstr(str, "pc"))
     parsed_value = parsed_value * 15;
   else if (strstr(str, "in"))
     parsed_value = parsed_value * 90;
   //TODO: implement 'em', 'ex' attributes

   /* Transform into global percentage */
   parsed_value = parsed_value / max;

   return parsed_value;
}

/**
 * @brief Converts a string representation of a gradient stop offset into a double value.
 *
 * Handles percentage values (e.g., "50%") and unitless values (interpreted as
 * fractions, e.g., "0.5"). Ensures the format is valid.
 *
 * @param str The string containing the offset value.
 * @return The offset as a double between 0.0 and 1.0, or 0.0 if invalid.
 */
static inline double
_to_offset(const char *str)
{
   char *end = NULL;
   const char* str_end = str + strlen(str);
   double parsed_value = eina_convert_strtod_c(str, &end);
   char *ptr = strstr(str, "%");

   end = _skip_space(end, NULL);

   if (ptr)
     {
        parsed_value = parsed_value / 100.0;
        if (end != ptr || (end + 1) != str_end)
          return 0;
     }
   else if (end != str_end)
     {
        return 0;
     }

   return parsed_value;
}

/**
 * @brief Converts a string representation of opacity (0.0 to 1.0) into an integer alpha value (0 to 255).
 * @param str The string containing the opacity value.
 * @return The alpha value as an integer (0-255). Returns 0 if the string is not a valid number.
 */
static inline int
_to_opacity(const char *str)
{
   char *end = NULL;
   int a = 0;
   double opacity = eina_convert_strtod_c(str, &end);

   if (end && (*end == '\0'))
     a = lrint(opacity * 255);
   return a;
}

/**
 * @def _PARSE_TAG
 * @brief Macro to generate functions for parsing string tags into enum values.
 *
 * Creates a static inline function `_to_##Short_Name` that takes a string
 * and returns the corresponding enum value from `Tags_Array` based on string comparison.
 *
 * @param Type The enum type to return.
 * @param Short_Name The suffix for the generated function name (e.g., `line_cap`).
 * @param Tags_Array An array of structs, where each struct has a `tag` (const char*)
 *                   and a `Short_Name` (Type) member.
 * @param Default The default enum value to return if no tag matches.
 */
#define _PARSE_TAG(Type, Short_Name, Tags_Array, Default)               \
  static Type _to_##Short_Name(const char *str)                         \
  {                                                                     \
     unsigned int i;                                                    \
                                                                        \
     for (i = 0; i < sizeof (Tags_Array) / sizeof (Tags_Array[0]); i++) \
       if (!strcmp(str, Tags_Array[i].tag))                             \
         return Tags_Array[i].Short_Name;                               \
     return Default;                                                    \
  }
/* parse the line cap used during stroking a path.
 * Value:    butt | round | square | inherit
 * Initial:    butt
 * https://www.w3.org/TR/SVG/painting.html
 */
static struct {
   Efl_Gfx_Cap line_cap;
   const char *tag;
} line_cap_tags[] = {
  { EFL_GFX_CAP_BUTT, "butt" },
  { EFL_GFX_CAP_ROUND, "round" },
  { EFL_GFX_CAP_SQUARE, "square" }
};

_PARSE_TAG(Efl_Gfx_Cap, line_cap, line_cap_tags, EFL_GFX_CAP_LAST);

/* parse the line join used during stroking a path.
 * Value:   miter | round | bevel | inherit
 * Initial:    miter
 * https://www.w3.org/TR/SVG/painting.html
 */
static struct {
   Efl_Gfx_Join line_join;
   const char *tag;
} line_join_tags[] = {
  { EFL_GFX_JOIN_MITER, "miter" },
  { EFL_GFX_JOIN_ROUND, "round" },
  { EFL_GFX_JOIN_BEVEL, "bevel" }
};

_PARSE_TAG(Efl_Gfx_Join, line_join, line_join_tags, EFL_GFX_JOIN_LAST);

/* parse the fill rule used during filling a path.
 * Value:   nonzero | evenodd | inherit
 * Initial:    nonzero
 * https://www.w3.org/TR/SVG/painting.html
 */

static struct {
   Efl_Gfx_Fill_Rule fill_rule;
   const char *tag;
} fill_rule_tags[] = {
  { EFL_GFX_FILL_RULE_ODD_EVEN, "evenodd" }
};

_PARSE_TAG(Efl_Gfx_Fill_Rule, fill_rule, fill_rule_tags, EFL_GFX_FILL_RULE_WINDING);

/* parse the dash pattern used during stroking a path.
 * Value:   none | <dasharray> | inherit
 * Initial:    none
 * https://www.w3.org/TR/SVG/painting.html
 */
/**
 * @brief Parses an SVG 'stroke-dasharray' string into an array of Efl_Gfx_Dash structures.
 *
 * Handles comma/space separated numbers. If an odd number of values is provided,
 * the list of values is repeated to yield an even number of values.
 * Allocates memory for the dash array, which must be freed by the caller.
 *
 * @param str The 'stroke-dasharray' attribute string (e.g., "5, 5", "10 5 2 5").
 * @param dash Pointer to store the allocated array of Efl_Gfx_Dash structures.
 *             Example structure: `[{length=5, gap=5}, {length=2, gap=3}]`
 * @param length Pointer to store the number of elements in the allocated dash array.
 */
static inline void
_parse_dash_array(const char *str, Efl_Gfx_Dash** dash, int *length)
{
   if (strlen(str) >= 4 && !strncmp(str, "none", 4)) return;

   // It is assumed that the length of the dasharray string is 255 or less.
   double tmp[255];
   char *end = NULL;
   int leni, gapi, count = 0, index = 0;

   while (*str)
     {
        // skip white space, comma
        str = _skipcomma(str);
        tmp[count++] = eina_convert_strtod_c(str, &end);
        str = _skipcomma(end);
     }

   if (count & 0x1)
     { // odd case.
        *length = count;
        *dash = calloc(*length, sizeof(Efl_Gfx_Dash));
        while (index < count)
          {
             leni = (2 * index) % count;
             gapi = (2 * index + 1) % count;
             (*dash)[index].length = tmp[leni];
             (*dash)[index].gap = tmp[gapi];
             index++;
          }
     }
   else
     { // even case
        *length = count/2;
        *dash = calloc(*length, sizeof(Efl_Gfx_Dash));
        while (index < *length)
          {
             (*dash)[index].length = tmp[2 * index];
             (*dash)[index].gap = tmp[2 * index + 1];
             index++;
          }
     }
}

/**
 * @brief Extracts an ID reference from a URL string (e.g., "url(#myGradient)", "#elementId").
 *
 * Skips "url(", leading/trailing whitespace, and the leading '#'.
 * Assumes the ID length is less than 50 characters.
 *
 * @param url The URL string.
 * @return A shared string containing the extracted ID, or NULL if parsing fails.
 */
static Eina_Stringshare *
 _id_from_url(const char *url)
{
   char tmp[50]; // Assumes ID length < 50
   int i = 0;

   url = _skip_space(url, NULL);
   if ((*url) == '(')
     {
        ++url;
        url = _skip_space(url, NULL);
     }

   if ((*url) == '#')
     ++url;

   while ((*url) != ')')
     {
        tmp[i++] = *url;
        ++url;
     }
   tmp[i] = '\0';

   return eina_stringshare_add(tmp);
}

/**
 * @brief Parses a single color component (R, G, or B) from a string.
 *
 * Handles numeric values (0-255) and percentages (0%-100%).
 * Advances the end pointer past the parsed component and any trailing whitespace/comma.
 *
 * @param value The string containing the color component value (e.g., "255", "50%").
 * @param end Pointer to a char pointer that will be updated to point after the parsed value.
 * @return The parsed color component as an unsigned char (0-255), or 0 on error.
 */
static unsigned char
_color_parser(const char *value, char **end)
{
   double r;

   r = eina_convert_strtod_c(value, end);
   *end = _skip_space(*end, NULL);
   if (**end == '%')
     r = 255 * r / 100;
   *end = _skip_space(*end, NULL);

   if (r < 0 || r > 255)
     {
        *end = NULL;
        return 0;
     }

   return lrint(r);
}

/**
 * @brief Lookup table for named SVG colors.
 * Maps color names (lowercase) to their 32-bit ARGB hex values (alpha is FF).
 */
static const struct {
   const char *name;     /**< Lowercase color name (e.g., "black"). */
   unsigned int value;  /**< ARGB hex value (e.g., 0xff000000). */
} colors[] = {
  { "aliceblue", 0xfff0f8ff },
  { "antiquewhite", 0xfffaebd7 },
  { "aqua", 0xff00ffff },
  { "aquamarine", 0xff7fffd4 },
  { "azure", 0xfff0ffff },
  { "beige", 0xfff5f5dc },
  { "bisque", 0xffffe4c4 },
  { "black", 0xff000000 },
  { "blanchedalmond", 0xffffebcd },
  { "blue", 0xff0000ff },
  { "blueviolet", 0xff8a2be2 },
  { "brown", 0xffa52a2a },
  { "burlywood", 0xffdeb887 },
  { "cadetblue", 0xff5f9ea0 },
  { "chartreuse", 0xff7fff00 },
  { "chocolate", 0xffd2691e },
  { "coral", 0xffff7f50 },
  { "cornflowerblue", 0xff6495ed },
  { "cornsilk", 0xfffff8dc },
  { "crimson", 0xffdc143c },
  { "cyan", 0xff00ffff },
  { "darkblue", 0xff00008b },
  { "darkcyan", 0xff008b8b },
  { "darkgoldenrod", 0xffb8860b },
  { "darkgray", 0xffa9a9a9 },
  { "darkgrey", 0xffa9a9a9 },
  { "darkgreen", 0xff006400 },
  { "darkkhaki", 0xffbdb76b },
  { "darkmagenta", 0xff8b008b },
  { "darkolivegreen", 0xff556b2f },
  { "darkorange", 0xffff8c00 },
  { "darkorchid", 0xff9932cc },
  { "darkred", 0xff8b0000 },
  { "darksalmon", 0xffe9967a },
  { "darkseagreen", 0xff8fbc8f },
  { "darkslateblue", 0xff483d8b },
  { "darkslategray", 0xff2f4f4f },
  { "darkslategrey", 0xff2f4f4f },
  { "darkturquoise", 0xff00ced1 },
  { "darkviolet", 0xff9400d3 },
  { "deeppink", 0xffff1493 },
  { "deepskyblue", 0xff00bfff },
  { "dimgray", 0xff696969 },
  { "dimgrey", 0xff696969 },
  { "dodgerblue", 0xff1e90ff },
  { "firebrick", 0xffb22222 },
  { "floralwhite", 0xfffffaf0 },
  { "forestgreen", 0xff228b22 },
  { "fuchsia", 0xffff00ff },
  { "gainsboro", 0xffdcdcdc },
  { "ghostwhite", 0xfff8f8ff },
  { "gold", 0xffffd700 },
  { "goldenrod", 0xffdaa520 },
  { "gray", 0xff808080 },
  { "grey", 0xff808080 },
  { "green", 0xff008000 },
  { "greenyellow", 0xffadff2f },
  { "honeydew", 0xfff0fff0 },
  { "hotpink", 0xffff69b4 },
  { "indianred", 0xffcd5c5c },
  { "indigo", 0xff4b0082 },
  { "ivory", 0xfffffff0 },
  { "khaki", 0xfff0e68c },
  { "lavender", 0xffe6e6fa },
  { "lavenderblush", 0xfffff0f5 },
  { "lawngreen", 0xff7cfc00 },
  { "lemonchiffon", 0xfffffacd },
  { "lightblue", 0xffadd8e6 },
  { "lightcoral", 0xfff08080 },
  { "lightcyan", 0xffe0ffff },
  { "lightgoldenrodyellow", 0xfffafad2 },
  { "lightgray", 0xffd3d3d3 },
  { "lightgrey", 0xffd3d3d3 },
  { "lightgreen", 0xff90ee90 },
  { "lightpink", 0xffffb6c1 },
  { "lightsalmon", 0xffffa07a },
  { "lightseagreen", 0xff20b2aa },
  { "lightskyblue", 0xff87cefa },
  { "lightslategray", 0xff778899 },
  { "lightslategrey", 0xff778899 },
  { "lightsteelblue", 0xffb0c4de },
  { "lightyellow", 0xffffffe0 },
  { "lime", 0xff00ff00 },
  { "limegreen", 0xff32cd32 },
  { "linen", 0xfffaf0e6 },
  { "magenta", 0xffff00ff },
  { "maroon", 0xff800000 },
  { "mediumaquamarine", 0xff66cdaa },
  { "mediumblue", 0xff0000cd },
  { "mediumorchid", 0xffba55d3 },
  { "mediumpurple", 0xff9370d8 },
  { "mediumseagreen", 0xff3cb371 },
  { "mediumslateblue", 0xff7b68ee },
  { "mediumspringgreen", 0xff00fa9a },
  { "mediumturquoise", 0xff48d1cc },
  { "mediumvioletred", 0xffc71585 },
  { "midnightblue", 0xff191970 },
  { "mintcream", 0xfff5fffa },
  { "mistyrose", 0xffffe4e1 },
  { "moccasin", 0xffffe4b5 },
  { "navajowhite", 0xffffdead },
  { "navy", 0xff000080 },
  { "oldlace", 0xfffdf5e6 },
  { "olive", 0xff808000 },
  { "olivedrab", 0xff6b8e23 },
  { "orange", 0xffffa500 },
  { "orangered", 0xffff4500 },
  { "orchid", 0xffda70d6 },
  { "palegoldenrod", 0xffeee8aa },
  { "palegreen", 0xff98fb98 },
  { "paleturquoise", 0xffafeeee },
  { "palevioletred", 0xffd87093 },
  { "papayawhip", 0xffffefd5 },
  { "peachpuff", 0xffffdab9 },
  { "peru", 0xffcd853f },
  { "pink", 0xffffc0cb },
  { "plum", 0xffdda0dd },
  { "powderblue", 0xffb0e0e6 },
  { "purple", 0xff800080 },
  { "red", 0xffff0000 },
  { "rosybrown", 0xffbc8f8f },
  { "royalblue", 0xff4169e1 },
  { "saddlebrown", 0xff8b4513 },
  { "salmon", 0xfffa8072 },
  { "sandybrown", 0xfff4a460 },
  { "seagreen", 0xff2e8b57 },
  { "seashell", 0xfffff5ee },
  { "sienna", 0xffa0522d },
  { "silver", 0xffc0c0c0 },
  { "skyblue", 0xff87ceeb },
  { "slateblue", 0xff6a5acd },
  { "slategray", 0xff708090 },
  { "slategrey", 0xff708090 },
  { "snow", 0xfffffafa },
  { "springgreen", 0xff00ff7f },
  { "steelblue", 0xff4682b4 },
  { "tan", 0xffd2b48c },
  { "teal", 0xff008080 },
  { "thistle", 0xffd8bfd8 },
  { "tomato", 0xffff6347 },
  { "turquoise", 0xff40e0d0 },
  { "violet", 0xffee82ee },
  { "wheat", 0xfff5deb3 },
  { "white", 0xffffffff },
  { "whitesmoke", 0xfff5f5f5 },
  { "yellow", 0xffffff00 },
  { "yellowgreen", 0xff9acd32 }
};

/**
 * @brief Parses an SVG color string into RGB components or a URL reference.
 *
 * Handles various SVG color formats:
 * - Hexadecimal: #RGB, #RRGGBB
 * - Functional: rgb(r, g, b), rgb(r%, g%, b%)
 * - Named colors (case-insensitive lookup in the `colors` table).
 * - URL references: url(#someGradient)
 *
 * @param str The color string to parse.
 * @param r Pointer to store the red component (0-255).
 * @param g Pointer to store the green component (0-255).
 * @param b Pointer to store the blue component (0-255).
 * @param ref Pointer to store a shared string for the ID if the color is a URL reference.
 *            Set to NULL if the color is not a URL.
 */
static inline void
_to_color(const char *str, int *r, int *g, int *b, Eina_Stringshare** ref)
{
   unsigned int i, len = strlen(str);
   char *red, *green, *blue;
   unsigned char tr, tg, tb;

   if (len == 4 && str[0] == '#')
     {
        // case for "#456" should be interprete as "#445566"
        if (isxdigit(str[1]) &&
            isxdigit(str[2]) &&
            isxdigit(str[3]))
          {
             char tmp[3] = { '\0', '\0', '\0' };
             tmp[0] = str[1]; tmp[1] = str[1]; *r = strtol(tmp, NULL, 16);
             tmp[0] = str[2]; tmp[1] = str[2]; *g = strtol(tmp, NULL, 16);
             tmp[0] = str[3]; tmp[1] = str[3]; *b = strtol(tmp, NULL, 16);
          }
     }
   else if (len == 7 && str[0] == '#')
     {
        if (isxdigit(str[1]) &&
            isxdigit(str[2]) &&
            isxdigit(str[3]) &&
            isxdigit(str[4]) &&
            isxdigit(str[5]) &&
            isxdigit(str[6]))
          {
             char tmp[3] = { '\0', '\0', '\0' };
             tmp[0] = str[1]; tmp[1] = str[2]; *r = strtol(tmp, NULL, 16);
             tmp[0] = str[3]; tmp[1] = str[4]; *g = strtol(tmp, NULL, 16);
             tmp[0] = str[5]; tmp[1] = str[6]; *b = strtol(tmp, NULL, 16);
          }
     }
   else if (len >= 10 &&
            (str[0] == 'r' || str[0] == 'R') &&
            (str[1] == 'g' || str[1] == 'G') &&
            (str[2] == 'b' || str[2] == 'B') &&
            str[3] == '(' &&
            str[len - 1] == ')')
     {
        tr = _color_parser(str + 4, &red);
        if (red && *red == ',')
          {
             tg = _color_parser(red + 1, &green);
             if (green && *green == ',')
               {
                  tb = _color_parser(green + 1, &blue);
                  if (blue && blue[0] == ')' && blue[1] == '\0')
                    {
                       *r = tr; *g = tg; *b = tb;
                    }
               }
          }
     }
   else if (len >= 3 && !strncmp(str, "url",3))
     {
        *ref = _id_from_url(str+3);
     }
   else
     {
        //handle named color
        for (i = 0; i < (sizeof (colors) / sizeof (colors[0])); i++)
          if (!strcasecmp(colors[i].name, str))
            {
               *r = R_VAL(&(colors[i].value));
               *g = G_VAL(&(colors[i].value));
               *b = B_VAL(&(colors[i].value));
            }
     }
}

/**
 * @brief Parses a string containing a sequence of numbers (space/comma separated)
 *        into an array of doubles.
 *
 * Used for parsing attributes like 'points' in <polygon>/<polyline> or
 * parameters within transform functions.
 *
 * @param str The input string containing numbers.
 * @param points Array to store the parsed double values. Assumed to be large enough.
 * @param pt_count Pointer to store the number of points parsed.
 * @return Pointer to the character in the string after the last parsed number
 *         and subsequent delimiter/whitespace.
 */
static inline char *
parse_numbers_array(char *str, double *points, int *pt_count)
{
   int count = 0;
   char *end = NULL;

   str = _skip_space(str, NULL);
   while (isdigit(*str) ||
          *str == '-' ||
          *str == '+' ||
          *str == '.')
     {
        points[count++] = eina_convert_strtod_c(str, &end);
        str = end;
        str = _skip_space(str, NULL);
        if (*str == ',')
          ++str;
        //eat the rest of space
        str = _skip_space(str, NULL);
     }
   *pt_count = count;
   return str;
}

/**
 * @enum _Matrix_State
 * @brief Internal state identifier for parsing different transformation functions.
 */
typedef enum _Matrix_State
{
  SVG_MATRIX_UNKNOWN,   /**< Initial or error state. */
  SVG_MATRIX_MATRIX,    /**< Parsing a 'matrix(a b c d e f)' function. */
  SVG_MATRIX_TRANSLATE, /**< Parsing a 'translate(tx [ty])' function. */
  SVG_MATRIX_ROTATE,    /**< Parsing a 'rotate(angle [cx cy])' function. */
  SVG_MATRIX_SCALE,     /**< Parsing a 'scale(sx [sy])' function. */
  SVG_MATRIX_SKEWX,     /**< Parsing a 'skewX(angle)' function. */
  SVG_MATRIX_SKEWY      /**< Parsing a 'skewY(angle)' function. */
} Matrix_State;

/** @brief Helper macro for defining entries in the matrix_tags lookup table. */
#define MATRIX_DEF(Name, Value)                 \
  { #Name, sizeof (#Name), Value}

/**
 * @brief Lookup table for SVG transform function names.
 * Maps function names (e.g., "matrix", "translate") to their corresponding
 * internal parser state (_Matrix_State).
 */
static const struct {
   const char *tag;     /**< The transform function name string. */
   int sz;              /**< The size of the tag string (including null terminator). */
   Matrix_State state;  /**< The corresponding parser state enum. */
} matrix_tags[] = {
  MATRIX_DEF(matrix, SVG_MATRIX_MATRIX),
  MATRIX_DEF(translate, SVG_MATRIX_TRANSLATE),
  MATRIX_DEF(rotate, SVG_MATRIX_ROTATE),
  MATRIX_DEF(scale, SVG_MATRIX_SCALE),
  MATRIX_DEF(skewX, SVG_MATRIX_SKEWX),
  MATRIX_DEF(skewY, SVG_MATRIX_SKEWY)
};

/* parse transform attribute
 * https://www.w3.org/TR/SVG/coords.html#TransformAttribute
 * Handles matrix(), translate(), scale(), rotate(), skewX(), skewY().
 * Composes multiple transforms in the order they appear.
 *
 * @param value The string value of the 'transform' attribute.
 * @return A newly allocated Eina_Matrix3 representing the combined transformation.
 *         The caller is responsible for freeing this matrix. Returns an identity
 *         matrix if parsing fails or the input is empty/invalid.
 */
static Eina_Matrix3 *
_parse_transformation_matrix(const char *value)
{
   unsigned int i;
   double points[8]; // Buffer for transform parameters
   int pt_count = 0;
   double sx, sy;
   Matrix_State state = SVG_MATRIX_UNKNOWN;
   Eina_Matrix3 *matrix = calloc(1, sizeof(Eina_Matrix3));
   char *str = (char *)value;
   char *end = str + strlen(str);

   eina_matrix3_identity(matrix);
   while (str < end)
     {
        if (isspace(*str) || (*str == ','))
          {
             ++str;
             continue;
          }
        for (i = 0; i < sizeof (matrix_tags) / sizeof(matrix_tags[0]); i++)
          if (!strncmp(matrix_tags[i].tag, str, matrix_tags[i].sz -1))
            {
               state = matrix_tags[i].state;
               str += (matrix_tags[i].sz -1);
            }
        if ( state == SVG_MATRIX_UNKNOWN)
          goto error;

        str = _skip_space(str, end);
        if (*str != '(')
          goto error;
        ++str;
        str = parse_numbers_array(str, points, &pt_count);
        if (*str != ')')
          goto error;
        ++str;

        if (state == SVG_MATRIX_MATRIX)
          {
             Eina_Matrix3 tmp;

             if (pt_count != 6) goto error;

             eina_matrix3_identity(&tmp);
             eina_matrix3_values_set(&tmp,
                                     points[0], points[2], points[4],
                                     points[1], points[3], points[5],
                                     0, 0, 1);
             eina_matrix3_compose(matrix, &tmp, matrix);
          }
        else if (state == SVG_MATRIX_TRANSLATE)
          {
             if (pt_count == 1)
               eina_matrix3_translate(matrix, points[0], 0);
             else if (pt_count == 2)
               eina_matrix3_translate(matrix, points[0], points[1]);
             else
               goto error;
          }
        else if (state == SVG_MATRIX_ROTATE)
          {
             //Transform to signed.
             points[0] = fmod(points[0], 360);
             if (points[0] < 0) points[0] += 360;

             if (pt_count == 1)
               {
                  eina_matrix3_rotate(matrix, points[0] * (M_PI/180.0));
               }
             else if (pt_count == 3)
               {
                  eina_matrix3_translate(matrix, points[1], points[2]);
                  eina_matrix3_rotate(matrix, points[0] * (M_PI/180.0));
                  eina_matrix3_translate(matrix, -points[1], -points[2]);
               }
             else
               {
                  goto error;
               }
          }
        else if (state == SVG_MATRIX_SCALE)
          {
             if (pt_count < 1 || pt_count > 2) goto error;

             sx = points[0];
             sy = sx;
             if (pt_count == 2)
               sy = points[1];
             eina_matrix3_scale(matrix, sx, sy);
          }
     }
 error:
   return matrix;
}

/** @brief Helper macro for defining entries in the length_tags lookup table. */
#define LENGTH_DEF(Name, Value)                 \
  { #Name, sizeof (#Name), Value}

/**
 * @brief Lookup table for SVG length unit identifiers.
 * Maps unit strings (e.g., "%", "px") to their corresponding Svg_Length_Type enum.
 */
static const struct {
   const char *tag;     /**< The unit identifier string. */
   int sz;              /**< The size of the tag string (including null terminator). */
   Svg_Length_Type type;/**< The corresponding length type enum. */
} length_tags[] = {
  LENGTH_DEF(%, SVG_LT_PERCENT),
  LENGTH_DEF(px, SVG_LT_PX),
  LENGTH_DEF(pc, SVG_LT_PC),
  LENGTH_DEF(pt, SVG_LT_PT),
  LENGTH_DEF(mm, SVG_LT_MM),
  LENGTH_DEF(cm, SVG_LT_CM),
  LENGTH_DEF(in, SVG_LT_IN)
};

/**
 * @brief Parses a length string, extracting the numeric value and identifying the unit type.
 *
 * This function primarily identifies the unit type based on the suffix.
 * It does *not* perform unit conversion; that happens in functions like _to_double.
 *
 * @param str The length string (e.g., "100px", "50%", "10").
 * @param type Pointer to store the identified Svg_Length_Type (defaults to SVG_LT_PX if no unit found).
 * @return The numeric value extracted from the string using eina_convert_strtod_c.
 */
static double
parse_length(const char *str, Svg_Length_Type *type)
{
   unsigned int i;
   double value;
   int sz = strlen(str);

   *type = SVG_LT_PX;
   for (i = 0; i < sizeof (length_tags) / sizeof(length_tags[0]); i++)
     if (length_tags[i].sz - 1 == sz && !strncmp(length_tags[i].tag, str, sz))
       {
          *type = length_tags[i].type;
       }
   value = eina_convert_strtod_c(str, NULL);
   return value;
}

// Forward declarations for attribute parsing functions
static Eina_Bool _parse_style_attr(void *data, const char *key, const char *value);
static Eina_Bool _attr_style_node(void *data, const char *str);

/**
 * @brief Parses attributes specific to the <svg> element.
 *
 * Handles 'width', 'height', 'viewBox', 'preserveAspectRatio', and 'style'.
 * Other attributes are delegated to the general style parser `_parse_style_attr`.
 * Updates the global viewport information in `loader->svg_parse->global`.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name (e.g., "width").
 * @param value The attribute value (e.g., "100px").
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_parse_svg_node(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Node *node = loader->svg_parse->node;
   Svg_Doc_Node *doc = &(node->node.doc);
   Svg_Length_Type type;

   // @TODO handle length unit.
   if (!strcmp(key, "width"))
     {
        doc->width = parse_length(value, &type);
     }
   else if (!strcmp(key, "height"))
     {
        doc->height = parse_length(value, &type);
     }
   else if (!strcmp(key, "viewBox"))
     {
        if (_parse_number(&value, &doc->vx))
          {
             if (_parse_number(&value, &doc->vy))
               {
                  if (_parse_number(&value, &doc->vw))
                    {
                       _parse_number(&value, &doc->vh);
                       loader->svg_parse->global.height = doc->vh;
                    }
                  loader->svg_parse->global.width = doc->vw;
               }
             loader->svg_parse->global.y = doc->vy;
          }
        loader->svg_parse->global.x = doc->vx;
     }
   else if (!strcmp(key, "preserveAspectRatio"))
     {
        if (!strcmp(value, "none"))
          doc->preserve_aspect = EINA_FALSE;
     }
   else if (!strcmp(key, "style"))
     {
        _attr_style_node(loader, value);
     }
   else
     {
        _parse_style_attr(loader, key, value);
     }
   return EINA_TRUE;
}

/**
 * @brief Parses a paint attribute value (e.g., for 'fill' or 'stroke').
 *
 * Handles "none", "currentColor", color values (parsed by _to_color),
 * and URL references (parsed by _to_color). Updates the Svg_Paint structure.
 * Reference: https://www.w3.org/TR/SVGTiny12/painting.html#SpecifyingPaint
 *
 * @param loader The SVG loader context (unused).
 * @param paint Pointer to the Svg_Paint structure to update.
 * @param value The attribute value string (e.g., "red", "#ff0000", "url(#myGrad)", "none").
 */
static void
_handle_paint_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Paint* paint, const char *value)
{
   if (!strcmp(value, "none"))
     {
        // no paint property
        paint->none = EINA_TRUE;
        return;
     }
   paint->none = EINA_FALSE;
   if (!strcmp(value, "currentColor"))
     {
        paint->cur_color = EINA_TRUE;
        return;
     }
   _to_color(value, &paint->r, &paint->g, &paint->b, &paint->url);
}

/**
 * @brief Handles the 'color' style property.
 * Parses the color value and sets the r, g, b fields in the node's style.
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose style is being updated.
 * @param value The 'color' attribute value.
 */
static void
_handle_color_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node* node, const char *value)
{
   Svg_Style_Property *style = node->style;
   _to_color(value, &style->r, &style->g, &style->b, NULL);
}

/**
 * @brief Handles the 'fill' style property or attribute.
 * Sets the SVG_FILL_FLAGS_PAINT flag and delegates parsing to _handle_paint_attr.
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose style is being updated.
 * @param value The 'fill' attribute value.
 */
static void
_handle_fill_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node* node, const char *value)
{
   Svg_Style_Property *style = node->style;
   style->fill.flags |= SVG_FILL_FLAGS_PAINT;
   _handle_paint_attr(loader, &style->fill.paint, value);
}

/**
 * @brief Handles the 'stroke' style property or attribute.
 * Sets the SVG_STROKE_FLAGS_PAINT flag and delegates parsing to _handle_paint_attr.
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose style is being updated.
 * @param value The 'stroke' attribute value.
 */
static void
_handle_stroke_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node* node, const char *value)
{
   Svg_Style_Property *style = node->style;
   style->stroke.flags |= SVG_STROKE_FLAGS_PAINT;
   _handle_paint_attr(loader, &style->stroke.paint, value);
}

/**
 * @brief Handles the 'stroke-opacity' style property or attribute.
 * Sets the SVG_STROKE_FLAGS_OPACITY flag and the opacity value (0-255).
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose style is being updated.
 * @param value The 'stroke-opacity' attribute value (0.0-1.0).
 */
static void
_handle_stroke_opacity_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node* node, const char *value)
{
   node->style->stroke.flags |= SVG_STROKE_FLAGS_OPACITY;
   node->style->stroke.opacity = _to_opacity(value);
}

/**
 * @brief Handles the 'stroke-dasharray' style property or attribute.
 * Sets the SVG_STROKE_FLAGS_DASH flag and parses the dash array.
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose style is being updated.
 * @param value The 'stroke-dasharray' attribute value (e.g., "5 5", "none").
 */
static void
_handle_stroke_dasharray_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node* node, const char *value)
{
   node->style->stroke.flags |= SVG_STROKE_FLAGS_DASH;
   _parse_dash_array(value, &node->style->stroke.dash, &node->style->stroke.dash_count);
}

/**
 * @brief Handles the 'stroke-width' style property or attribute.
 * Sets the SVG_STROKE_FLAGS_WIDTH flag and the width value (converted to pixels).
 * @param loader The SVG loader context.
 * @param node The SVG node whose style is being updated.
 * @param value The 'stroke-width' attribute value (e.g., "2px", "1").
 */
static void
_handle_stroke_width_attr(Evas_SVG_Loader *loader, Svg_Node* node, const char *value)
{
   node->style->stroke.flags |= SVG_STROKE_FLAGS_WIDTH;
   node->style->stroke.width = _to_double(loader->svg_parse, value, SVG_PARSER_LENGTH_HORIZONTAL);
}

/**
 * @brief Handles the 'stroke-linecap' style property or attribute.
 * Sets the SVG_STROKE_FLAGS_CAP flag and the line cap enum value.
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose style is being updated.
 * @param value The 'stroke-linecap' attribute value ("butt", "round", "square").
 */
static void
_handle_stroke_linecap_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node* node, const char *value)
{
   node->style->stroke.flags |= SVG_STROKE_FLAGS_CAP;
   node->style->stroke.cap = _to_line_cap(value);
}

/**
 * @brief Handles the 'stroke-linejoin' style property or attribute.
 * Sets the SVG_STROKE_FLAGS_JOIN flag and the line join enum value.
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose style is being updated.
 * @param value The 'stroke-linejoin' attribute value ("miter", "round", "bevel").
 */
static void
_handle_stroke_linejoin_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node* node, const char *value)
{
   node->style->stroke.flags |= SVG_STROKE_FLAGS_JOIN;
   node->style->stroke.join = _to_line_join(value);
}

/**
 * @brief Handles the 'fill-rule' style property or attribute.
 * Sets the SVG_FILL_FLAGS_FILL_RULE flag and the fill rule enum value.
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose style is being updated.
 * @param value The 'fill-rule' attribute value ("nonzero", "evenodd").
 */
static void
_handle_fill_rule_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node* node, const char *value)
{
   node->style->fill.flags |= SVG_FILL_FLAGS_FILL_RULE;
   node->style->fill.fill_rule = _to_fill_rule(value);
}

/**
 * @brief Handles the 'opacity' style property or attribute (overall element opacity).
 * Sets the node's overall opacity value (0-255).
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose style is being updated.
 * @param value The 'opacity' attribute value (0.0-1.0).
 */
static void
_handle_opacity_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node* node, const char *value)
{
   node->style->opacity = _to_opacity(value);
}

/**
 * @brief Handles the 'fill-opacity' style property or attribute.
 * Sets the SVG_FILL_FLAGS_OPACITY flag and the fill opacity value (0-255).
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose style is being updated.
 * @param value The 'fill-opacity' attribute value (0.0-1.0).
 */
static void
_handle_fill_opacity_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node* node, const char *value)
{
   node->style->fill.flags |= SVG_FILL_FLAGS_OPACITY;
   node->style->fill.opacity = _to_opacity(value);
}

/**
 * @brief Handles the 'transform' attribute.
 * Parses the transformation string and sets the node's transform matrix.
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose transform is being set.
 * @param value The 'transform' attribute value (e.g., "translate(10, 10) rotate(45)").
 */
static void
_handle_transform_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node* node, const char *value)
{
   node->transform = _parse_transformation_matrix(value);
}

/**
 * @brief Handles the 'clip-path' attribute.
 * Sets the SVG_COMPOSITE_FLAGS_CLIP_PATH flag and stores the URL reference.
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose clip path is being set.
 * @param value The 'clip-path' attribute value (e.g., "url(#myClip)").
 */
static void _handle_clip_path_attr(Evas_SVG_Loader* loader EINA_UNUSED, Svg_Node* node, const char* value)
{
    Svg_Style_Property* style = node->style;
    style->comp.flags |= SVG_COMPOSITE_FLAGS_CLIP_PATH;

    int len = strlen(value);
    if (len >= 3 && !strncmp(value, "url", 3)) style->comp.url = _id_from_url((const char*)(value + 3));
}

/**
 * @brief Handles the 'display' attribute.
 * Sets the node's visibility flag based on the value ("none" or other).
 * @param loader The SVG loader context (unused).
 * @param node The SVG node whose display property is being set.
 * @param value The 'display' attribute value.
 */
static void
_handle_display_attr(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node* node, const char *value)
{
   //TODO : The display attribute can have various values as well as "none".
   //       The default is "inline" which means visible and "none" means invisible.
   //       Depending on the type of node, additional functionality may be required.
   //       refer to https://developer.mozilla.org/en-US/docs/Web/SVG/Attribute/display
   if (!strcmp(value, "none")) node->display = EINA_FALSE;
   else node->display = EINA_TRUE;
}

/**
 * @typedef Style_Method
 * @brief Function pointer type for handling specific style properties/attributes.
 * @param loader The SVG loader context.
 * @param node The SVG node being styled.
 * @param value The attribute/property value string.
 */
typedef void (*Style_Method)(Evas_SVG_Loader *loader, Svg_Node *node, const char *value);

/** @brief Helper macro for defining entries in the style_tags lookup table. */
#define STYLE_DEF(Name, Name1)       \
  { #Name, sizeof (#Name), _handle_##Name1##_attr}

/**
 * @brief Lookup table for common SVG style properties and attributes.
 * Maps property/attribute names (e.g., "fill", "stroke-width") to their
 * corresponding handler functions (_handle_*_attr).
 */
static const struct {
   const char *tag;     /**< The style property/attribute name. */
   int sz;              /**< The size of the tag string (including null terminator). */
   Style_Method tag_handler; /**< The function to handle this property/attribute. */
} style_tags[] = {
  STYLE_DEF(color, color),
  STYLE_DEF(fill, fill),
  STYLE_DEF(fill-rule, fill_rule),
  STYLE_DEF(fill-opacity, fill_opacity),
  STYLE_DEF(opacity, opacity),
  STYLE_DEF(stroke, stroke),
  STYLE_DEF(stroke-width, stroke_width),
  STYLE_DEF(stroke-linejoin, stroke_linejoin),
  STYLE_DEF(stroke-linecap, stroke_linecap),
  STYLE_DEF(stroke-opacity, stroke_opacity),
  STYLE_DEF(stroke-dasharray, stroke_dasharray),
  STYLE_DEF(transform, transform),
  STYLE_DEF(display, display)
};

/**
 * @brief Parses a single style property (key/value pair) or a direct attribute.
 *
 * This function is called either directly for attributes like 'fill', 'stroke', etc.,
 * or indirectly via `_attr_style_node` when parsing a 'style' attribute string.
 * It looks up the key in `style_tags` and calls the appropriate handler function.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The style property or attribute name (e.g., "fill", "stroke-width").
 * @param value The property or attribute value (e.g., "red", "2px").
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_parse_style_attr(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Node* node = loader->svg_parse->node;
   unsigned int i;
   int sz;

   // trim the white space
   key = _skip_space(key, NULL);

   value = _skip_space(value, NULL);

   sz = strlen(key);
   for (i = 0; i < sizeof (style_tags) / sizeof(style_tags[0]); i++)
     if (style_tags[i].sz - 1 == sz && !strncmp(style_tags[i].tag, key, sz))
       {
          style_tags[i].tag_handler(loader, node, value);
          return EINA_TRUE;
       }

   return EINA_TRUE;
}

/**
 * @brief Parses the content of a 'style' attribute string.
 *
 * Uses `eina_simple_xml_attribute_w3c_parse` to break down the style string
 * (e.g., "fill: red; stroke: blue;") into individual key/value pairs and
 * calls `_parse_style_attr` for each one.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param str The value of the 'style' attribute.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_style_node(void *data, const char *str)
{
   eina_simple_xml_attribute_w3c_parse(str,
                                       _parse_style_attr, data);
   return EINA_TRUE;
}

/**
 * @brief Parses attributes specific to the <g> (group) element.
 *
 * Handles 'style', 'transform', 'clip-path', and 'id'.
 * Other attributes are delegated to the general style parser `_parse_style_attr`.
 * Reference: https://www.w3.org/TR/SVG/struct.html#Groups
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name.
 * @param value The attribute value.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_parse_g_node(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Node* node = loader->svg_parse->node;

   if (!strcmp(key, "style"))
     {
        return _attr_style_node(loader, value);
     }
   else if (!strcmp(key, "transform"))
     {
        node->transform = _parse_transformation_matrix(value);
     }
   else if (!strcmp(key, "clip-path"))
     {
        _handle_clip_path_attr(loader, node, value);
     }
   else if (!strcmp(key, "id"))
     {
        node->id = _copy_id(value);
     }
   else
     {
        _parse_style_attr(loader, key, value);
     }
   return EINA_TRUE;
}

/**
 * @brief Parses attributes specific to the <clipPath> element.
 *
 * Handles 'style', 'transform', 'clip-path', and 'id'.
 * Other attributes are delegated to the general style parser `_parse_style_attr`.
 * Reference: https://www.w3.org/TR/SVG/masking.html#ClipPathElement
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name.
 * @param value The attribute value.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool _attr_parse_clip_path_node(void* data, const char* key, const char* value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Node* node = loader->svg_parse->node;

   if (!strcmp(key, "style"))
     {
        return _attr_style_node(loader, value);
     }
   else if (!strcmp(key, "transform"))
     {
        node->transform = _parse_transformation_matrix(value);
     }
   else if (!strcmp(key, "clip-path"))
     {
        _handle_clip_path_attr(loader, node, value);
     }
   else if (!strcmp(key, "id"))
     {
        node->id = _copy_id(value);
     }
   else
     {
        _parse_style_attr(loader, key, value);
     }
   return EINA_TRUE;
}

/**
 * @brief Creates a new Svg_Node with default style properties.
 *
 * Allocates memory for the node and its style property structure.
 * Initializes default style values according to SVG specifications
 * (e.g., fill=black, stroke=none, opacity=1).
 * Appends the new node to the parent's child list if a parent is provided.
 *
 * @param parent The parent node in the SVG tree, or NULL for the root/defs node.
 * @param type The type of the SVG node to create (e.g., SVG_NODE_RECT).
 * @return A pointer to the newly allocated and initialized Svg_Node.
 */
static Svg_Node *
_create_node(Svg_Node *parent, Svg_Node_Type type)
{
   Svg_Node *node = calloc(1, sizeof(Svg_Node));

   // default fill property
   node->style = calloc(1, sizeof(Svg_Style_Property));

   // update the default value of stroke and fill
   //https://www.w3.org/TR/SVGTiny12/painting.html#SpecifyingPaint
   // default fill color is black
   //node->style->fill.paint.r = 0;
   //node->style->fill.paint.g = 0;
   //node->style->fill.paint.b = 0;
   node->style->fill.paint.none = EINA_FALSE;
   // default fill opacity is 1
   node->style->fill.opacity = 255;
   node->style->opacity = 255;

   // default fill rule is nonzero
   node->style->fill.fill_rule = EFL_GFX_FILL_RULE_WINDING;

   // default stroke is none
   node->style->stroke.paint.none = EINA_TRUE;
   // default stroke opacity is 1
   node->style->stroke.opacity = 255;
   // default stroke width is 1
   node->style->stroke.width = 1;
   // default line cap is butt
   node->style->stroke.cap = EFL_GFX_CAP_BUTT;
   // default line join is miter
   node->style->stroke.join = EFL_GFX_JOIN_MITER;
   node->style->stroke.scale = 1.0;

   // default display is true("inline").
   node->display = EINA_TRUE;

   node->parent = parent;
   node->type = type;
   node->child = NULL;

   if (parent)
     parent->child = eina_list_append(parent->child, node);
   return node;
}

/**
 * @brief Factory function for creating an SVG <defs> node.
 * @param loader The SVG loader context (unused).
 * @param parent Parent node (unused, defs is usually top-level or under root).
 * @param buf Buffer with attributes (unused).
 * @param buflen Length of attribute buffer (unused).
 * @return A new Svg_Node of type SVG_NODE_DEFS.
 */
static Svg_Node *
_create_defs_node(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node *parent EINA_UNUSED, const char *buf EINA_UNUSED, unsigned buflen EINA_UNUSED)
{
   Svg_Node *node = _create_node(NULL, SVG_NODE_DEFS);
   // Attributes are parsed later if needed, or handled by children
   eina_simple_xml_attributes_parse(buf, buflen,
                                    NULL, node); // No specific attribute parser needed here
   return node;
}

/**
 * @brief Factory function for creating an SVG <g> node.
 * @param loader The SVG loader context.
 * @param parent The parent node.
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Node of type SVG_NODE_G with parsed attributes.
 */
static Svg_Node *
_create_g_node(Evas_SVG_Loader *loader, Svg_Node *parent, const char *buf, unsigned buflen)
{
   loader->svg_parse->node = _create_node(parent, SVG_NODE_G);

   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_g_node, loader);
   return loader->svg_parse->node;
}

/**
 * @brief Factory function for creating the root SVG <svg> node.
 * @param loader The SVG loader context.
 * @param parent The parent node (should be NULL for the root).
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Node of type SVG_NODE_DOC with parsed attributes.
 */
static Svg_Node *
_create_svg_node(Evas_SVG_Loader *loader, Svg_Node *parent, const char *buf, unsigned buflen)
{
   loader->svg_parse->node = _create_node(parent, SVG_NODE_DOC);
   Svg_Doc_Node *doc = &(loader->svg_parse->node->node.doc);

   doc->preserve_aspect = EINA_TRUE; // Default preserveAspectRatio
   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_svg_node, loader);

   return loader->svg_parse->node;
}

/**
 * @brief Factory function for creating an SVG <switch> node (currently ignored).
 * @param loader The SVG loader context (unused).
 * @param parent The parent node (unused).
 * @param buf Buffer with attributes (unused).
 * @param buflen Length of attribute buffer (unused).
 * @return NULL, as <switch> elements are not currently processed.
 */
static Svg_Node *
_create_switch_node(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node *parent EINA_UNUSED, const char *buf EINA_UNUSED, unsigned buflen EINA_UNUSED)
{
   // TODO: Implement <switch> element handling if required.
   return NULL;
}

/**
 * @brief Factory function for creating an SVG <mask> node (currently creates a hidden node).
 * @param loader The SVG loader context (unused).
 * @param parent The parent node (unused, masks are usually in <defs>).
 * @param buf Buffer with attributes (unused).
 * @param buflen Length of attribute buffer (unused).
 * @return A new Svg_Node of type SVG_NODE_UNKNOWN marked as not displayed.
 */
static Svg_Node *
_create_mask_node(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node *parent EINA_UNUSED, const char *buf EINA_UNUSED, unsigned buflen EINA_UNUSED)
{
   // TODO: Implement proper <mask> element handling.
   Svg_Node *node = _create_node(NULL, SVG_NODE_UNKNOWN);

   node->display = EINA_FALSE; // Masks themselves aren't displayed directly
   return node;
}

/**
 * @brief Factory function for creating an SVG <clipPath> node.
 * @param loader The SVG loader context.
 * @param parent The parent node (usually NULL or <defs>).
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Node of type SVG_NODE_CLIP_PATH with parsed attributes.
 */
static Svg_Node *
_create_clipPath_node(Evas_SVG_Loader *loader EINA_UNUSED, Svg_Node *parent EINA_UNUSED, const char *buf EINA_UNUSED, unsigned buflen EINA_UNUSED)
{
   loader->svg_parse->node = _create_node(parent, SVG_NODE_CLIP_PATH);

   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_clip_path_node, loader);
   return loader->svg_parse->node;
}

/**
 * @brief Parses attributes specific to the <path> element.
 *
 * Handles 'd' (path data), 'style', 'clip-path', and 'id'.
 * Other attributes are delegated to the general style parser `_parse_style_attr`.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name.
 * @param value The attribute value.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_parse_path_node(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Node* node = loader->svg_parse->node;
   Svg_Path_Node *path = &(node->node.path);

   if (!strcmp(key, "d"))
     {
        path->path = eina_stringshare_add(value);
     }
   else if (!strcmp(key, "style"))
     {
        _attr_style_node(loader, value);
     }
   else if (!strcmp(key, "clip-path"))
     {
        _handle_clip_path_attr(loader, node, value);
     }
   else if (!strcmp(key, "id"))
     {
        node->id = _copy_id(value);
     }
   else
     {
        _parse_style_attr(loader, key, value);
     }
   return EINA_TRUE;
}

/**
 * @brief Factory function for creating an SVG <path> node.
 * @param loader The SVG loader context.
 * @param parent The parent node.
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Node of type SVG_NODE_PATH with parsed attributes.
 */
static Svg_Node *
_create_path_node(Evas_SVG_Loader *loader, Svg_Node *parent, const char *buf, unsigned buflen)
{
   loader->svg_parse->node = _create_node(parent, SVG_NODE_PATH);

   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_path_node, loader);

   return loader->svg_parse->node;
}

/** @brief Helper macro for defining entries in the circle_tags lookup table. */
#define CIRCLE_DEF(Name, Field, Type)       \
  { #Name, Type, sizeof (#Name), offsetof(Svg_Circle_Node, Field)}

/**
 * @brief Lookup table for SVG <circle> element attributes.
 * Maps attribute names ("cx", "cy", "r") to their corresponding field offset
 * within the Svg_Circle_Node struct and the length type for percentage calculation.
 */
static const struct {
   const char *tag;             /**< The attribute name string. */
   SVG_Parser_Length_Type type; /**< The length type for percentage calculation. */
   int sz;                      /**< The size of the tag string (including null terminator). */
   size_t offset;               /**< The offset of the corresponding field in Svg_Circle_Node. */
} circle_tags[] = {
  CIRCLE_DEF(cx, cx, SVG_PARSER_LENGTH_HORIZONTAL),
  CIRCLE_DEF(cy, cy, SVG_PARSER_LENGTH_VERTICAL),
  CIRCLE_DEF(r, r, SVG_PARSER_LENGTH_OTHER)
};

/* parse the attributes for a circle element.
 * https://www.w3.org/TR/SVG/shapes.html#CircleElement
 * Handles 'cx', 'cy', 'r', 'style', 'clip-path', and 'id'.
 * Other attributes are delegated to the general style parser `_parse_style_attr`.
 * Uses the `circle_tags` lookup table to set the appropriate fields in the node.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name.
 * @param value The attribute value.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_parse_circle_node(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Node* node = loader->svg_parse->node;
   Svg_Circle_Node *circle = &(node->node.circle);
   unsigned int i;
   unsigned char *array;
   int sz = strlen(key);

   array = (unsigned char*) circle;
   for (i = 0; i < sizeof (circle_tags) / sizeof(circle_tags[0]); i++)
     if (circle_tags[i].sz - 1 == sz && !strncmp(circle_tags[i].tag, key, sz))
       {
          *((double*) (array + circle_tags[i].offset)) =
             _to_double(loader->svg_parse, value, circle_tags[i].type);
          return EINA_TRUE;
       }

   if (!strcmp(key, "style"))
     {
        _attr_style_node(loader, value);
     }
   else if (!strcmp(key, "clip-path"))
     {
        _handle_clip_path_attr(loader, node, value);
     }
   else if (!strcmp(key, "id"))
     {
        node->id = _copy_id(value);
     }
   else
     {
        _parse_style_attr(loader, key, value);
     }
   return EINA_TRUE;
}

/**
 * @brief Factory function for creating an SVG <circle> node.
 * @param loader The SVG loader context.
 * @param parent The parent node.
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Node of type SVG_NODE_CIRCLE with parsed attributes.
 */
static Svg_Node *
_create_circle_node(Evas_SVG_Loader *loader, Svg_Node *parent, const char *buf, unsigned buflen)
{
   loader->svg_parse->node = _create_node(parent, SVG_NODE_CIRCLE);

   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_circle_node, loader);
   return loader->svg_parse->node;
}

/** @brief Helper macro for defining entries in the ellipse_tags lookup table. */
#define ELLIPSE_DEF(Name, Field, Type)       \
  { #Name, Type, sizeof (#Name), offsetof(Svg_Ellipse_Node, Field)}

/**
 * @brief Lookup table for SVG <ellipse> element attributes.
 * Maps attribute names ("cx", "cy", "rx", "ry") to their corresponding field offset
 * within the Svg_Ellipse_Node struct and the length type for percentage calculation.
 */
static const struct {
   const char *tag;             /**< The attribute name string. */
   SVG_Parser_Length_Type type; /**< The length type for percentage calculation. */
   int sz;                      /**< The size of the tag string (including null terminator). */
   size_t offset;               /**< The offset of the corresponding field in Svg_Ellipse_Node. */
} ellipse_tags[] = {
  ELLIPSE_DEF(cx,cx, SVG_PARSER_LENGTH_HORIZONTAL),
  ELLIPSE_DEF(cy,cy, SVG_PARSER_LENGTH_VERTICAL),
  ELLIPSE_DEF(rx,rx, SVG_PARSER_LENGTH_HORIZONTAL),
  ELLIPSE_DEF(ry,ry, SVG_PARSER_LENGTH_VERTICAL)
};

/* parse the attributes for an ellipse element.
 * https://www.w3.org/TR/SVG/shapes.html#EllipseElement
 * Handles 'cx', 'cy', 'rx', 'ry', 'style', 'clip-path', and 'id'.
 * Other attributes are delegated to the general style parser `_parse_style_attr`.
 * Uses the `ellipse_tags` lookup table to set the appropriate fields in the node.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name.
 * @param value The attribute value.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_parse_ellipse_node(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Node* node = loader->svg_parse->node;
   Svg_Ellipse_Node *ellipse = &(node->node.ellipse);
   unsigned int i;
   unsigned char *array;
   int sz = strlen(key);

   array = (unsigned char*) ellipse;
   for (i = 0; i < sizeof (ellipse_tags) / sizeof(ellipse_tags[0]); i++)
     if (ellipse_tags[i].sz - 1 == sz && !strncmp(ellipse_tags[i].tag, key, sz))
       {
          *((double*) (array + ellipse_tags[i].offset)) =
             _to_double(loader->svg_parse, value, ellipse_tags[i].type);
          return EINA_TRUE;
       }

   if (!strcmp(key, "id"))
     {
        node->id = _copy_id(value);
     }
   else if (!strcmp(key, "clip-path"))
     {
        _handle_clip_path_attr(loader, node, value);
     }
   else if (!strcmp(key, "style"))
     {
        _attr_style_node(loader, value);
     }
   else
     {
        _parse_style_attr(loader, key, value);
     }
   return EINA_TRUE;
}

/**
 * @brief Factory function for creating an SVG <ellipse> node.
 * @param loader The SVG loader context.
 * @param parent The parent node.
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Node of type SVG_NODE_ELLIPSE with parsed attributes.
 */
static Svg_Node *
_create_ellipse_node(Evas_SVG_Loader *loader, Svg_Node *parent, const char *buf, unsigned buflen)
{
   loader->svg_parse->node = _create_node(parent, SVG_NODE_ELLIPSE);

   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_ellipse_node, loader);
   return loader->svg_parse->node;
}

/**
 * @brief Parses the 'points' attribute string for <polygon> and <polyline>.
 *
 * Extracts coordinate pairs (x, y) from a space/comma separated string of numbers.
 * Allocates memory for the points array, which must be freed by the caller.
 * Uses a temporary buffer and reallocates as needed.
 *
 * @param str The 'points' attribute value string (e.g., "100,10 250,150 150,210").
 * @param points Pointer to store the allocated array of doubles (x1, y1, x2, y2, ...).
 * @param point_count Pointer to store the total number of double values parsed (twice the number of points).
 */
static void
_attr_parse_polygon_points(const char *str, double **points, int *point_count)
{
   double tmp[50]; // Temporary buffer for points
   int tmp_count=0;
   int count = 0;
   double num;
   double *point_array = NULL, *tmp_array;

   while (_parse_number(&str, &num))
     {
        tmp[tmp_count++] = num;
        if (tmp_count == 50)
          {
             tmp_array = realloc(point_array, (count + tmp_count) * sizeof(double));
             if (!tmp_array) goto error_alloc;
             point_array = tmp_array;
             memcpy(&point_array[count], tmp, tmp_count * sizeof(double));
             count += tmp_count;
             tmp_count = 0;
          }
     }

   if (tmp_count > 0)
     {
        tmp_array = realloc(point_array, (count + tmp_count) * sizeof(double));
        if (!tmp_array) goto error_alloc;
        point_array = tmp_array;
        memcpy(&point_array[count], tmp, tmp_count * sizeof(double));
        count += tmp_count;
     }
   *point_count = count;
   *points = point_array;
   return;

error_alloc:
   ERR("allocation for point array failed. out of memory");
   abort();
}

/* parse the attributes for a polygon element.
 * https://www.w3.org/TR/SVG/shapes.html#PolygonElement
 * https://www.w3.org/TR/SVG/shapes.html#PolylineElement
 * Handles 'points', 'style', 'clip-path', and 'id'.
 * Other attributes are delegated to the general style parser `_parse_style_attr`.
 * Calls `_attr_parse_polygon_points` to handle the 'points' attribute.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name.
 * @param value The attribute value.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_parse_polygon_node(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Node *node = loader->svg_parse->node;
   Svg_Polygon_Node *polygon = NULL;

   if (node->type == SVG_NODE_POLYGON)
     polygon = &(node->node.polygon);
   else
     polygon = &(node->node.polyline);


   if (!strcmp(key, "points"))
     {
        _attr_parse_polygon_points(value, &polygon->points, &polygon->points_count);
     }
   else if (!strcmp(key, "style"))
     {
        _attr_style_node(loader, value);
     }
   else if (!strcmp(key, "clip-path"))
     {
        _handle_clip_path_attr(loader, node, value);
     }
   else if (!strcmp(key, "id"))
     {
        node->id = _copy_id(value);
     }
   else
     {
        _parse_style_attr(loader, key, value);
     }
   return EINA_TRUE;
}

/**
 * @brief Factory function for creating an SVG <polygon> node.
 * @param loader The SVG loader context.
 * @param parent The parent node.
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Node of type SVG_NODE_POLYGON with parsed attributes.
 */
static Svg_Node *
_create_polygon_node(Evas_SVG_Loader *loader, Svg_Node *parent, const char *buf, unsigned buflen)
{
   loader->svg_parse->node = _create_node(parent, SVG_NODE_POLYGON);

   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_polygon_node, loader);
   return loader->svg_parse->node;
}

/**
 * @brief Factory function for creating an SVG <polyline> node.
 * @param loader The SVG loader context.
 * @param parent The parent node.
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Node of type SVG_NODE_POLYLINE with parsed attributes.
 */
static Svg_Node *
_create_polyline_node(Evas_SVG_Loader *loader, Svg_Node *parent, const char *buf, unsigned buflen)
{
   loader->svg_parse->node = _create_node(parent, SVG_NODE_POLYLINE);

   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_polygon_node, loader); // Uses the same attribute parser as polygon
   return loader->svg_parse->node;
}

/** @brief Helper macro for defining entries in the rect_tags lookup table. */
#define RECT_DEF(Name, Field, Type)       \
  { #Name, Type, sizeof (#Name), offsetof(Svg_Rect_Node, Field)}

/**
 * @brief Lookup table for SVG <rect> element attributes.
 * Maps attribute names ("x", "y", "width", "height", "rx", "ry") to their
 * corresponding field offset within the Svg_Rect_Node struct and the length
 * type for percentage calculation.
 */
static const struct {
   const char *tag;             /**< The attribute name string. */
   SVG_Parser_Length_Type type; /**< The length type for percentage calculation. */
   int sz;                      /**< The size of the tag string (including null terminator). */
   size_t offset;               /**< The offset of the corresponding field in Svg_Rect_Node. */
} rect_tags[] = {
  RECT_DEF(x,x, SVG_PARSER_LENGTH_HORIZONTAL),
  RECT_DEF(y, y, SVG_PARSER_LENGTH_VERTICAL),
  RECT_DEF(width, w, SVG_PARSER_LENGTH_HORIZONTAL),
  RECT_DEF(height, h, SVG_PARSER_LENGTH_VERTICAL),
  RECT_DEF(rx, rx, SVG_PARSER_LENGTH_HORIZONTAL),
  RECT_DEF(ry, ry, SVG_PARSER_LENGTH_VERTICAL)
};

/* parse the attributes for a rect element.
 * https://www.w3.org/TR/SVG/shapes.html#RectElement
 * Handles 'x', 'y', 'width', 'height', 'rx', 'ry', 'style', 'clip-path', and 'id'.
 * Other attributes are delegated to the general style parser `_parse_style_attr`.
 * Uses the `rect_tags` lookup table to set the appropriate fields in the node.
 * Handles the special case where only 'rx' or 'ry' is specified.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name.
 * @param value The attribute value.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_parse_rect_node(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Node *node = loader->svg_parse->node;
   Svg_Rect_Node *rect = & (node->node.rect);
   unsigned int i;
   unsigned char *array;
   int sz = strlen(key);

   array = (unsigned char*) rect;
   for (i = 0; i < sizeof (rect_tags) / sizeof(rect_tags[0]); i++)
     if (rect_tags[i].sz - 1 == sz && !strncmp(rect_tags[i].tag, key, sz))
       {
          *((double*) (array + rect_tags[i].offset)) = _to_double(loader->svg_parse, value, rect_tags[i].type);

          //Case if only rx or ry is declared
          if (!strncmp(rect_tags[i].tag, "rx", sz)) rect->has_rx = EINA_TRUE;
          if (!strncmp(rect_tags[i].tag, "ry", sz)) rect->has_ry = EINA_TRUE;

          if (!EINA_DBL_EQ(rect->rx, 0) && EINA_DBL_EQ(rect->ry, 0) && rect->has_rx && !rect->has_ry) rect->ry = rect->rx;
          if (!EINA_DBL_EQ(rect->ry, 0) && EINA_DBL_EQ(rect->rx, 0) && !rect->has_rx && rect->has_ry) rect->rx = rect->ry;
          return EINA_TRUE;
       }

   if (!strcmp(key, "id"))
     {
        node->id = _copy_id(value);
     }
   else if (!strcmp(key, "style"))
     {
        _attr_style_node(loader, value);
     }
   else if (!strcmp(key, "clip-path"))
     {
        _handle_clip_path_attr(loader, node, value);
     }
   else
     {
        _parse_style_attr(loader, key, value);
     }


   return EINA_TRUE;
}

/**
 * @brief Factory function for creating an SVG <rect> node.
 * Initializes has_rx and has_ry flags before parsing attributes.
 * @param loader The SVG loader context.
 * @param parent The parent node.
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Node of type SVG_NODE_RECT with parsed attributes.
 */
static Svg_Node *
_create_rect_node(Evas_SVG_Loader *loader, Svg_Node *parent, const char *buf, unsigned buflen)
{
   loader->svg_parse->node = _create_node(parent, SVG_NODE_RECT);

   // Initialize flags for rx/ry handling
   if (loader->svg_parse->node) {
        loader->svg_parse->node->node.rect.has_rx = EINA_FALSE;
        loader->svg_parse->node->node.rect.has_ry = EINA_FALSE;
   }

   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_rect_node, loader);
   return loader->svg_parse->node;
}

/** @brief Helper macro for defining entries in the line_tags lookup table. */
#define LINE_DEF(Name, Field, Type)       \
  { #Name, Type, sizeof (#Name), offsetof(Svg_Line_Node, Field)}

/**
 * @brief Lookup table for SVG <line> element attributes.
 * Maps attribute names ("x1", "y1", "x2", "y2") to their corresponding field offset
 * within the Svg_Line_Node struct and the length type for percentage calculation.
 */
static const struct {
   const char *tag;             /**< The attribute name string. */
   SVG_Parser_Length_Type type; /**< The length type for percentage calculation. */
   int sz;                      /**< The size of the tag string (including null terminator). */
   size_t offset;               /**< The offset of the corresponding field in Svg_Line_Node. */
} line_tags[] = {
  LINE_DEF(x1, x1, SVG_PARSER_LENGTH_HORIZONTAL),
  LINE_DEF(y1, y1, SVG_PARSER_LENGTH_VERTICAL),
  LINE_DEF(x2, x2, SVG_PARSER_LENGTH_HORIZONTAL),
  LINE_DEF(y2, y2, SVG_PARSER_LENGTH_VERTICAL)
};

/* parse the attributes for a rect element.
 * https://www.w3.org/TR/SVG/shapes.html#LineElement
 * Handles 'x1', 'y1', 'x2', 'y2', 'style', 'clip-path', and 'id'.
 * Other attributes are delegated to the general style parser `_parse_style_attr`.
 * Uses the `line_tags` lookup table to set the appropriate fields in the node.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name.
 * @param value The attribute value.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_parse_line_node(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Node *node = loader->svg_parse->node;
   Svg_Line_Node *line = & (node->node.line);
   unsigned int i;
   unsigned char *array;
   int sz = strlen(key);

   array = (unsigned char*) line;
   for (i = 0; i < sizeof (line_tags) / sizeof(line_tags[0]); i++)
     if (line_tags[i].sz - 1 == sz && !strncmp(line_tags[i].tag, key, sz))
       {
          *((double*) (array + line_tags[i].offset)) = _to_double(loader->svg_parse, value, line_tags[i].type);
          return EINA_TRUE;
       }

   if (!strcmp(key, "id"))
     {
        node->id = _copy_id(value);
     }
   else if (!strcmp(key, "style"))
     {
        _attr_style_node(loader, value);
     }
   else if (!strcmp(key, "clip-path"))
     {
        _handle_clip_path_attr(loader, node, value);
     }
   else
     {
        _parse_style_attr(loader, key, value);
     }
   return EINA_TRUE;
}

/**
 * @brief Factory function for creating an SVG <line> node.
 * @param loader The SVG loader context.
 * @param parent The parent node.
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Node of type SVG_NODE_LINE with parsed attributes.
 */
static Svg_Node *
_create_line_node(Evas_SVG_Loader *loader, Svg_Node *parent, const char *buf, unsigned buflen)
{
   loader->svg_parse->node = _create_node(parent, SVG_NODE_LINE);

   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_line_node, loader);
   return loader->svg_parse->node;
}

/**
 * @brief Extracts an ID reference from an 'xlink:href' or 'href' attribute value.
 * Skips leading whitespace and the leading '#'.
 * @param href The attribute value string (e.g., "#myElement").
 * @return A shared string containing the extracted ID.
 */
static Eina_Stringshare *
_id_from_href(const char *href)
{
   href = _skip_space(href, NULL);
   if ((*href) == '#')
     href++;
   return eina_stringshare_add(href);
}

/**
 * @brief Finds the <defs> node associated with a given node.
 * Traverses up the parent chain to the root <svg> node and returns its defs pointer.
 * @param node The node from which to start the search.
 * @return Pointer to the Svg_Node representing the <defs> element, or NULL if not found.
 */
static Svg_Node*
_get_defs_node(Svg_Node *node)
{
   if (!node) return NULL;

   while (node->parent != NULL)
     {
        node = node->parent;
     }

   if (node->type == SVG_NODE_DOC)
     return node->node.doc.defs;

   return NULL;
}

/**
 * @brief Finds a direct child node with a specific ID.
 * Iterates through the direct children of the given node.
 * @param node The parent node whose children to search.
 * @param id The ID string to search for.
 * @return Pointer to the child Svg_Node with the matching ID, or NULL if not found.
 */
static Svg_Node*
_find_child_by_id(Svg_Node *node, const char *id)
{
   Eina_List *l;
   Svg_Node *child;

   if (!node) return NULL;

   EINA_LIST_FOREACH(node->child, l, child)
     {
        if ((child->id != NULL) && !strcmp(child->id, id))
          return child;
     }
   return NULL;
}

/**
 * @brief Recursively finds any node within the subtree (including the node itself)
 *        with a specific ID.
 * Performs a depth-first search starting from the given node.
 * @param node The starting node for the search.
 * @param id The ID string to search for.
 * @return Pointer to the Svg_Node with the matching ID, or NULL if not found.
 */
static Svg_Node* _find_node_by_id(Svg_Node *node, const char* id)
{
   Svg_Node *child, *result = NULL;
   Eina_List *l;
   if ((node->id) && !strcmp(node->id, id)) return node;

   EINA_LIST_FOREACH(node->child, l, child)
     {
        result = _find_node_by_id(child, id);
        if (result) break;
     }
   return result;
}

/**
 * @brief Creates a deep copy of a list of gradient stops.
 * Allocates new memory for each stop and copies the data.
 * @param from The Eina_List of Efl_Gfx_Gradient_Stop pointers to clone.
 * @return A new Eina_List containing copies of the original stops. The caller
 *         is responsible for freeing the list and its contents.
 */
static Eina_List *
_clone_grad_stops(Eina_List *from)
{
   Efl_Gfx_Gradient_Stop *stop;
   Eina_List *l;
   Eina_List *res = NULL;

   EINA_LIST_FOREACH(from, l, stop)
     {
        Efl_Gfx_Gradient_Stop *new_stop;

        new_stop = calloc(1, sizeof(Efl_Gfx_Gradient_Stop));
        memcpy(new_stop, stop, sizeof(Efl_Gfx_Gradient_Stop));
        res = eina_list_append(res, new_stop);
     }

   return res;
}

/**
 * @brief Creates a deep copy of an Svg_Style_Gradient structure.
 *
 * Allocates new memory for the gradient structure, its transform matrix (if any),
 * its linear/radial specific data, and clones its gradient stops using `_clone_grad_stops`.
 * Copies ID, reference, spread, and other flags.
 *
 * @param from Pointer to the Svg_Style_Gradient to clone.
 * @return A pointer to the newly allocated and copied Svg_Style_Gradient,
 *         or NULL if `from` is NULL. The caller is responsible for freeing the returned structure.
 */
static Svg_Style_Gradient *
_clone_gradient(Svg_Style_Gradient *from)
{
   Svg_Style_Gradient *grad;

   if (!from) return NULL;

   grad= calloc(1, sizeof(Svg_Style_Gradient));
   grad->type = from->type;
   grad->id = _copy_id(from->id);
   grad->ref = _copy_id(from->ref);
   grad->spread = from->spread;
   grad->use_percentage = from->use_percentage;
   grad->user_space = from->user_space;
   if (from->transform)
     {
        grad->transform = calloc(1, sizeof(Eina_Matrix3));
        eina_matrix3_copy(grad->transform, from->transform);
     }
   grad->stops = _clone_grad_stops(from->stops);
   if (grad->type == SVG_LINEAR_GRADIENT)
     {
        grad->linear = calloc(1, sizeof(Svg_Linear_Gradient));
        memcpy(grad->linear, from->linear, sizeof(Svg_Linear_Gradient));
     }
   else if (grad->type == SVG_RADIAL_GRADIENT)
     {
        grad->radial = calloc(1, sizeof(Svg_Radial_Gradient));
        memcpy(grad->radial, from->radial, sizeof(Svg_Radial_Gradient));
     }

   return grad;
}

/**
 * @brief Copies attributes and style properties from one Svg_Node to another.
 *
 * Performs a deep copy of the transform matrix (if present) and the
 * Svg_Style_Property structure. Also copies node-specific data (like circle radius,
 * rect dimensions, path data, polygon points) based on the node type.
 * Assumes `to->style` is already allocated.
 *
 * @param to The destination Svg_Node.
 * @param from The source Svg_Node.
 */
static void
_copy_attribute(Svg_Node *to, Svg_Node *from)
{
   // copy matrix attribute (deep copy)
   if (from->transform)
     {
        to->transform = calloc(1, sizeof(Eina_Matrix3));
        eina_matrix3_copy(to->transform, from->transform);
     }
   // copy style attribute;
   memcpy(to->style, from->style, sizeof(Svg_Style_Property));

   // copy node attribute
   switch (from->type)
     {
        case SVG_NODE_CIRCLE:
           to->node.circle.cx = from->node.circle.cx;
           to->node.circle.cy = from->node.circle.cy;
           to->node.circle.r = from->node.circle.r;
           break;
        case SVG_NODE_ELLIPSE:
           to->node.ellipse.cx = from->node.ellipse.cx;
           to->node.ellipse.cy = from->node.ellipse.cy;
           to->node.ellipse.rx = from->node.ellipse.rx;
           to->node.ellipse.ry = from->node.ellipse.ry;
           break;
        case SVG_NODE_RECT:
           to->node.rect.x = from->node.rect.x;
           to->node.rect.y = from->node.rect.y;
           to->node.rect.w = from->node.rect.w;
           to->node.rect.h = from->node.rect.h;
           to->node.rect.rx = from->node.rect.rx;
           to->node.rect.ry = from->node.rect.ry;
           to->node.rect.has_rx = from->node.rect.has_rx;
           to->node.rect.has_ry = from->node.rect.has_ry;
           break;
        case SVG_NODE_LINE:
           to->node.line.x1 = from->node.line.x1;
           to->node.line.y1 = from->node.line.y1;
           to->node.line.x2 = from->node.line.x2;
           to->node.line.y2 = from->node.line.y2;
           break;
        case SVG_NODE_PATH:
           to->node.path.path = eina_stringshare_add(from->node.path.path);
           break;
        case SVG_NODE_POLYGON:
           to->node.polygon.points_count = from->node.polygon.points_count;
           to->node.polygon.points = malloc(to->node.polygon.points_count * sizeof(double));
           memcpy(to->node.polygon.points, from->node.polygon.points, to->node.polygon.points_count * sizeof(double));
           break;
        case SVG_NODE_POLYLINE:
           to->node.polyline.points_count = from->node.polyline.points_count;
           to->node.polyline.points = malloc(to->node.polyline.points_count * sizeof(double));
           memcpy(to->node.polyline.points, from->node.polyline.points, to->node.polyline.points_count * sizeof(double));
           break;
        default:
           break;
     }

}

/**
 * @brief Recursively clones an Svg_Node and its entire subtree.
 *
 * Creates a new node of the same type, copies its attributes using `_copy_attribute`,
 * and then recursively calls itself to clone all children, attaching them to the new node.
 *
 * @param from The Svg_Node to clone.
 * @param parent The parent for the newly created top-level clone.
 */
static void
_clone_node(Svg_Node *from, Svg_Node *parent)
{
   Svg_Node *new_node;
   Eina_List *l;
   Svg_Node *child;

   if (!from) return;

   new_node = _create_node(parent, from->type);
   _copy_attribute(new_node, from);

   EINA_LIST_FOREACH(from->child, l, child)
     {
         _clone_node(child, new_node);
     }

}

/**
 * @brief Parses attributes specific to the <use> element.
 *
 * Handles 'xlink:href' (or 'href') to find the referenced element (usually in <defs>)
 * and clones it using `_clone_node`, adding the clone as a child of the <use> node's
 * representation (which is created as a <g> node).
 * Also handles 'style', 'transform', 'clip-path', and 'id' attributes for the <use> element itself.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name.
 * @param value The attribute value.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_parse_use_node(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Node *defs, *node_from, *node = loader->svg_parse->node;
   Eina_Stringshare *id;

   if (!strcmp(key, "xlink:href"))
     {
        id = _id_from_href(value);
        defs = _get_defs_node(node);
        node_from = _find_child_by_id(defs, id);
        _clone_node(node_from, node);
        eina_stringshare_del(id);
     }
   else if (!strcmp(key, "clip-path"))
     {
        _handle_clip_path_attr(loader, node, value);
     }
   else
     {
        _attr_parse_g_node(data, key, value);
     }
   return EINA_TRUE;
}

/**
 * @brief Factory function for creating an SVG <use> node.
 *
 * Creates an Svg_Node of type SVG_NODE_G to represent the <use> element itself
 * (as <use> can have transforms and styles). The actual referenced content
 * is cloned and added as children during attribute parsing (`_attr_parse_use_node`).
 *
 * @param loader The SVG loader context.
 * @param parent The parent node.
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Node of type SVG_NODE_G representing the <use> element.
 */
static Svg_Node *
_create_use_node(Evas_SVG_Loader *loader, Svg_Node *parent, const char *buf, unsigned buflen)
{
   // Represent <use> as a group node to hold its own transforms/styles
   loader->svg_parse->node = _create_node(parent, SVG_NODE_G);

   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_use_node, loader);
   return loader->svg_parse->node;
}

/** @brief Helper macro for defining entries in graphics_tags and group_tags lookup tables. */
#define TAG_DEF(Name)                                   \
  { #Name, sizeof (#Name), _create_##Name##_node }

/**
 * @brief Lookup table for SVG graphics elements.
 * Maps element tag names (e.g., "circle", "path") to their corresponding
 * node factory functions (_create_*_node).
 * TODO: implement 'text' primitive
 */
static const struct {
   const char *tag;     /**< The element tag name. */
   int sz;              /**< The size of the tag string (including null terminator). */
   Factory_Method tag_handler; /**< The factory function to create this node type. */
} graphics_tags[] = {
  TAG_DEF(use),
  TAG_DEF(circle),
  TAG_DEF(ellipse),
  TAG_DEF(path),
  TAG_DEF(polygon),
  TAG_DEF(rect),
  TAG_DEF(polyline),
  TAG_DEF(line),
};

/**
 * @brief Lookup table for SVG container/structural elements.
 * Maps element tag names (e.g., "g", "svg", "defs") to their corresponding
 * node factory functions (_create_*_node).
 */
static const struct {
   const char *tag;     /**< The element tag name. */
   int sz;              /**< The size of the tag string (including null terminator). */
   Factory_Method tag_handler; /**< The factory function to create this node type. */
} group_tags[] = {
  TAG_DEF(defs),
  TAG_DEF(g),
  TAG_DEF(svg),
  TAG_DEF(switch),
  TAG_DEF(mask),
  TAG_DEF(clipPath)
};

/**
 * @def FIND_FACTORY
 * @brief Macro to generate functions for finding factory methods based on tag names.
 *
 * Creates a static inline function `_find_##Short_Name##_factory` that takes a tag name string
 * and returns the corresponding Factory_Method function pointer from `Tags_Array`.
 *
 * @param Short_Name The suffix for the generated function name (e.g., `group`, `graphics`).
 * @param Tags_Array The lookup table (e.g., `group_tags`, `graphics_tags`) containing
 *                   tag names and their associated factory functions.
 */
#define FIND_FACTORY(Short_Name, Tags_Array)                            \
  static Factory_Method                                                 \
  _find_##Short_Name##_factory(const char  *name)                       \
  {                                                                     \
     unsigned int i;                                                    \
     int sz = strlen(name);                                             \
                                                                        \
     for (i = 0; i < sizeof (Tags_Array) / sizeof(Tags_Array[0]); i++)  \
       if (Tags_Array[i].sz - 1 == sz && !strncmp(Tags_Array[i].tag, name, sz)) \
         {                                                              \
            return Tags_Array[i].tag_handler;                           \
         }                                                              \
     return NULL;                                                       \
  }

FIND_FACTORY(group, group_tags);
FIND_FACTORY(graphics, graphics_tags);

Efl_Gfx_Gradient_Spread
_parse_spread_value(const char *value)
{
   Efl_Gfx_Gradient_Spread spread = EFL_GFX_GRADIENT_SPREAD_PAD;

   if (!strcmp(value, "reflect"))
     {
        spread = EFL_GFX_GRADIENT_SPREAD_REFLECT;
     }
   else if (!strcmp(value, "repeat"))
     {
        spread = EFL_GFX_GRADIENT_SPREAD_REPEAT;
     }

   return spread;
}

// --- Radial Gradient Attribute Handlers ---

/** @brief Handles the 'cx' attribute for radial gradients. Sets fx if not explicitly parsed. */
static void
_handle_radial_cx_attr(Evas_SVG_Loader *loader, Svg_Radial_Gradient* radial, const char *value)
{
   radial->cx = _gradient_to_double(loader->svg_parse, value, SVG_PARSER_LENGTH_HORIZONTAL);
   // Default fx to cx if fx is not specified
   if (!loader->svg_parse->gradient.fx_parsed)
     radial->fx = radial->cx;
}

/** @brief Handles the 'cy' attribute for radial gradients. Sets fy if not explicitly parsed. */
static void
_handle_radial_cy_attr(Evas_SVG_Loader *loader, Svg_Radial_Gradient* radial, const char *value)
{
   radial->cy = _gradient_to_double(loader->svg_parse, value, SVG_PARSER_LENGTH_VERTICAL);
   // Default fy to cy if fy is not specified
   if (!loader->svg_parse->gradient.fy_parsed)
     radial->fy = radial->cy;
}

/** @brief Handles the 'fx' attribute for radial gradients. */
static void
_handle_radial_fx_attr(Evas_SVG_Loader *loader, Svg_Radial_Gradient* radial, const char *value)
{
   radial->fx = _gradient_to_double(loader->svg_parse, value, SVG_PARSER_LENGTH_HORIZONTAL);
   loader->svg_parse->gradient.fx_parsed = EINA_TRUE; // Mark fx as explicitly parsed
}

/** @brief Handles the 'fy' attribute for radial gradients. */
static void
_handle_radial_fy_attr(Evas_SVG_Loader *loader, Svg_Radial_Gradient* radial, const char *value)
{
   radial->fy = _gradient_to_double(loader->svg_parse, value, SVG_PARSER_LENGTH_VERTICAL);
   loader->svg_parse->gradient.fy_parsed = EINA_TRUE; // Mark fy as explicitly parsed
}

/** @brief Handles the 'r' attribute for radial gradients. */
static void
_handle_radial_r_attr(Evas_SVG_Loader *loader, Svg_Radial_Gradient* radial, const char *value)
{
   radial->r = _gradient_to_double(loader->svg_parse, value, SVG_PARSER_LENGTH_OTHER);
}

// --- Radial Gradient Recalculation Handlers (for gradientUnits="objectBoundingBox") ---

/** @brief Recalculates radial 'cx' from percentage to absolute coordinates if needed. */
static void
_recalc_radial_cx_attr(Evas_SVG_Loader *loader, Svg_Radial_Gradient* radial, Eina_Bool user_space)
{
   // If using objectBoundingBox units (default), convert percentage to absolute coordinate
   if (!user_space)
     {
        radial->cx = radial->cx * loader->svg_parse->global.width;
     }
}

/** @brief Recalculates radial 'cy' from percentage to absolute coordinates if needed. */
static void
_recalc_radial_cy_attr(Evas_SVG_Loader *loader, Svg_Radial_Gradient* radial, Eina_Bool user_space)
{
   if (!user_space)
     {
        radial->cy = radial->cy * loader->svg_parse->global.height;
     }
}

/** @brief Recalculates radial 'fx' from percentage to absolute coordinates if needed. */
static void
_recalc_radial_fx_attr(Evas_SVG_Loader *loader, Svg_Radial_Gradient* radial, Eina_Bool user_space)
{
   if (!user_space)
     {
        radial->fx = radial->fx * loader->svg_parse->global.width;
     }
}

/** @brief Recalculates radial 'fy' from percentage to absolute coordinates if needed. */
static void
_recalc_radial_fy_attr(Evas_SVG_Loader *loader, Svg_Radial_Gradient* radial, Eina_Bool user_space)
{
   if (!user_space)
     {
        radial->fy = radial->fy * loader->svg_parse->global.height;
     }
}

/** @brief Recalculates radial 'r' from percentage to absolute coordinates if needed. */
static void
_recalc_radial_r_attr(Evas_SVG_Loader *loader, Svg_Radial_Gradient* radial, Eina_Bool user_space)
{
   // If using objectBoundingBox units (default), convert percentage based on viewport diagonal
   if (!user_space)
     {
        // Normalize radius based on viewport diagonal (sqrt(w^2 + h^2) / sqrt(2))
        radial->r = radial->r * (sqrt(pow(loader->svg_parse->global.height, 2) + pow(loader->svg_parse->global.width, 2)) / sqrt(2.0));
     }
}

/**
 * @typedef Radial_Method
 * @brief Function pointer type for handling specific radial gradient attributes during initial parsing.
 * @param loader The SVG loader context.
 * @param radial The radial gradient structure being populated.
 * @param value The attribute value string.
 */
typedef void (*Radial_Method)(Evas_SVG_Loader *loader, Svg_Radial_Gradient *radial, const char *value);
/**
 * @typedef Radial_Method_Recalc
 * @brief Function pointer type for recalculating radial gradient attributes after parsing,
 *        based on the `gradientUnits` attribute.
 * @param loader The SVG loader context.
 * @param radial The radial gradient structure to potentially update.
 * @param user_space EINA_TRUE if `gradientUnits="userSpaceOnUse"`, EINA_FALSE otherwise.
 */
typedef void (*Radial_Method_Recalc)(Evas_SVG_Loader *loader, Svg_Radial_Gradient *radial, Eina_Bool user_space);

/** @brief Helper macro for defining entries in the radial_tags lookup table. */
#define RADIAL_DEF(Name)       \
  { #Name, sizeof (#Name), _handle_radial_##Name##_attr, _recalc_radial_##Name##_attr}

/**
 * @brief Lookup table for SVG <radialGradient> element attributes.
 * Maps attribute names ("cx", "cy", "fx", "fy", "r") to their initial parsing
 * handler and their final recalculation handler (based on gradientUnits).
 */
static const struct {
   const char *tag;             /**< The attribute name string. */
   int sz;                      /**< The size of the tag string (including null terminator). */
   Radial_Method tag_handler;   /**< The initial parsing handler function. */
   Radial_Method_Recalc tag_recalc; /**< The recalculation handler function. */
} radial_tags[] = {
  RADIAL_DEF(cx),
  RADIAL_DEF(cy),
  RADIAL_DEF(fx),
  RADIAL_DEF(fy),
  RADIAL_DEF(r)
};

/**
 * @brief Parses attributes specific to the <radialGradient> element.
 *
 * Handles 'id', 'spreadMethod', 'xlink:href', 'gradientUnits', and specific
 * radial attributes ('cx', 'cy', 'fx', 'fy', 'r') using the `radial_tags` table.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name.
 * @param value The attribute value.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_parse_radial_gradient_node(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Style_Gradient *grad = loader->svg_parse->style_grad;
   Svg_Radial_Gradient *radial = grad->radial;
   unsigned int i;
   int sz = strlen(key);

   for (i = 0; i < sizeof (radial_tags) / sizeof(radial_tags[0]); i++)
     if (radial_tags[i].sz - 1 == sz && !strncmp(radial_tags[i].tag, key, sz))
       {
          radial_tags[i].tag_handler(loader, radial, value);
          return EINA_TRUE;
       }

   if (!strcmp(key, "id"))
     {
        grad->id = _copy_id(value);
     }
   else if (!strcmp(key, "spreadMethod"))
     {
        grad->spread = _parse_spread_value(value);
     }
   else if (!strcmp(key, "xlink:href"))
     {
        grad->ref = _id_from_href(value);
     }
   else if (!strcmp(key, "gradientUnits") && !strcmp(value, "userSpaceOnUse"))
     {
        grad->user_space = EINA_TRUE;
     }

   return EINA_TRUE;
}

/**
 * @brief Factory function for creating an SVG <radialGradient> style structure.
 *
 * Allocates the gradient structure, sets defaults (cx=cy=fx=fy=r=0.5, units=objectBoundingBox),
 * parses attributes using `_attr_parse_radial_gradient_node`, and then performs
 * recalculation based on `gradientUnits` using the handlers in `radial_tags`.
 *
 * @param loader The SVG loader context.
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Style_Gradient structure populated with radial gradient data.
 */
static Svg_Style_Gradient *
_create_radialGradient(Evas_SVG_Loader *loader, const char *buf, unsigned buflen)
{
   unsigned int i = 0;
   Svg_Style_Gradient *grad = calloc(1, sizeof(Svg_Style_Gradient));
   loader->svg_parse->style_grad = grad;

   grad->type = SVG_RADIAL_GRADIENT;
   grad->user_space = EINA_FALSE;
   grad->radial = calloc(1, sizeof(Svg_Radial_Gradient));
   /**
    * Default values of gradient
    */
   grad->radial->cx = 0.5;
   grad->radial->cy = 0.5;
   grad->radial->fx = 0.5;
   grad->radial->fy = 0.5;
   grad->radial->r = 0.5;

   loader->svg_parse->gradient.fx_parsed = EINA_FALSE;
   loader->svg_parse->gradient.fy_parsed = EINA_FALSE;
   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_radial_gradient_node, loader);

   for (i = 0; i < sizeof (radial_tags) / sizeof(radial_tags[0]); i++)
     radial_tags[i].tag_recalc(loader, grad->radial, grad->user_space);

   grad->use_percentage = EINA_TRUE;

   return loader->svg_parse->style_grad;
}

/**
 * @brief Parses attributes specific to the <stop> element within a gradient.
 *
 * Handles 'offset', 'stop-opacity', 'stop-color', and 'style'.
 * Updates the Efl_Gfx_Gradient_Stop structure pointed to by `loader->svg_parse->grad_stop`.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name.
 * @param value The attribute value.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_parse_stops(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Efl_Gfx_Gradient_Stop *stop = loader->svg_parse->grad_stop;

   if (!strcmp(key, "offset"))
     {
        stop->offset = _to_offset(value);
     }
   else if (!strcmp(key, "stop-opacity"))
     {
        stop->a = _to_opacity(value);
     }
   else if (!strcmp(key, "stop-color"))
     {
        _to_color(value, &stop->r, &stop->g, &stop->b, NULL);
     }
   else if (!strcmp(key, "style"))
     {
        eina_simple_xml_attribute_w3c_parse(value,
                                            _attr_parse_stops, data);
     }

   return EINA_TRUE;
}

// --- Linear Gradient Attribute Handlers ---

/** @brief Handles the 'x1' attribute for linear gradients. */
static void
_handle_linear_x1_attr(Evas_SVG_Loader *loader, Svg_Linear_Gradient* linear, const char *value)
{
   linear->x1 = _gradient_to_double(loader->svg_parse, value, SVG_PARSER_LENGTH_HORIZONTAL);
}

/** @brief Handles the 'y1' attribute for linear gradients. */
static void
_handle_linear_y1_attr(Evas_SVG_Loader *loader, Svg_Linear_Gradient* linear, const char *value)
{
   linear->y1 = _gradient_to_double(loader->svg_parse, value, SVG_PARSER_LENGTH_VERTICAL);
}

/** @brief Handles the 'x2' attribute for linear gradients. */
static void
_handle_linear_x2_attr(Evas_SVG_Loader *loader, Svg_Linear_Gradient* linear, const char *value)
{
   linear->x2 = _gradient_to_double(loader->svg_parse, value, SVG_PARSER_LENGTH_HORIZONTAL);
}

/** @brief Handles the 'y2' attribute for linear gradients. */
static void
_handle_linear_y2_attr(Evas_SVG_Loader *loader, Svg_Linear_Gradient* linear, const char *value)
{
   linear->y2 = _gradient_to_double(loader->svg_parse, value, SVG_PARSER_LENGTH_VERTICAL);
}

// --- Linear Gradient Recalculation Handlers (for gradientUnits="objectBoundingBox") ---

/** @brief Recalculates linear 'x1' from percentage to absolute coordinates if needed. */
static void
_recalc_linear_x1_attr(Evas_SVG_Loader *loader, Svg_Linear_Gradient* linear, Eina_Bool user_space)
{
   // If using objectBoundingBox units (default), convert percentage to absolute coordinate
   if (!user_space)
     {
        linear->x1 = linear->x1 * loader->svg_parse->global.width;
     }
}

/** @brief Recalculates linear 'y1' from percentage to absolute coordinates if needed. */
static void
_recalc_linear_y1_attr(Evas_SVG_Loader *loader, Svg_Linear_Gradient* linear, Eina_Bool user_space)
{
   if (!user_space)
     {
        linear->y1 = linear->y1 * loader->svg_parse->global.height;
     }
}

/** @brief Recalculates linear 'x2' from percentage to absolute coordinates if needed. */
static void
_recalc_linear_x2_attr(Evas_SVG_Loader *loader, Svg_Linear_Gradient* linear, Eina_Bool user_space)
{
   if (!user_space)
     {
        linear->x2 = linear->x2 * loader->svg_parse->global.width;
     }
}

/** @brief Recalculates linear 'y2' from percentage to absolute coordinates if needed. */
static void
_recalc_linear_y2_attr(Evas_SVG_Loader *loader, Svg_Linear_Gradient* linear, Eina_Bool user_space)
{
   if (!user_space)
     {
        linear->y2 = linear->y2 * loader->svg_parse->global.height;
     }
}

/**
 * @typedef Linear_Method
 * @brief Function pointer type for handling specific linear gradient attributes during initial parsing.
 * @param loader The SVG loader context.
 * @param linear The linear gradient structure being populated.
 * @param value The attribute value string.
 */
typedef void (*Linear_Method)(Evas_SVG_Loader *loader, Svg_Linear_Gradient *linear, const char *value);
/**
 * @typedef Linear_Method_Recalc
 * @brief Function pointer type for recalculating linear gradient attributes after parsing,
 *        based on the `gradientUnits` attribute.
 * @param loader The SVG loader context.
 * @param linear The linear gradient structure to potentially update.
 * @param user_space EINA_TRUE if `gradientUnits="userSpaceOnUse"`, EINA_FALSE otherwise.
 */
typedef void (*Linear_Method_Recalc)(Evas_SVG_Loader *loader, Svg_Linear_Gradient *linear, Eina_Bool user_space);

/** @brief Helper macro for defining entries in the linear_tags lookup table. */
#define LINEAR_DEF(Name)       \
  { #Name, sizeof (#Name), _handle_linear_##Name##_attr, _recalc_linear_##Name##_attr}

/**
 * @brief Lookup table for SVG <linearGradient> element attributes.
 * Maps attribute names ("x1", "y1", "x2", "y2") to their initial parsing
 * handler and their final recalculation handler (based on gradientUnits).
 */
static const struct {
   const char *tag;             /**< The attribute name string. */
   int sz;                      /**< The size of the tag string (including null terminator). */
   Linear_Method tag_handler;   /**< The initial parsing handler function. */
   Linear_Method_Recalc tag_recalc; /**< The recalculation handler function. */
} linear_tags[] = {
  LINEAR_DEF(x1),
  LINEAR_DEF(y1),
  LINEAR_DEF(x2),
  LINEAR_DEF(y2)
};

/**
 * @brief Parses attributes specific to the <linearGradient> element.
 *
 * Handles 'id', 'spreadMethod', 'xlink:href', 'gradientUnits', 'gradientTransform',
 * and specific linear attributes ('x1', 'y1', 'x2', 'y2') using the `linear_tags` table.
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param key The attribute name.
 * @param value The attribute value.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
_attr_parse_linear_gradient_node(void *data, const char *key, const char *value)
{
   Evas_SVG_Loader *loader = data;
   Svg_Style_Gradient *grad = loader->svg_parse->style_grad;
   Svg_Linear_Gradient *linear = grad->linear;
   unsigned int i;
   int sz = strlen(key);

   for (i = 0; i < sizeof (linear_tags) / sizeof(linear_tags[0]); i++)
     if (linear_tags[i].sz - 1 == sz && !strncmp(linear_tags[i].tag, key, sz))
       {
          linear_tags[i].tag_handler(loader, linear, value);
          return EINA_TRUE;
       }

   if (!strcmp(key, "id"))
     {
        grad->id = _copy_id(value);
     }
   else if (!strcmp(key, "spreadMethod"))
     {
        grad->spread = _parse_spread_value(value);
     }
   else if (!strcmp(key, "xlink:href"))
     {
        grad->ref = _id_from_href(value);
     }
   else if (!strcmp(key, "gradientUnits") && !strcmp(value, "userSpaceOnUse"))
     {
        grad->user_space = EINA_TRUE;
     }
   else if (!strcmp(key, "gradientTransform"))
     {
        grad->transform = _parse_transformation_matrix(value);
     }

   return EINA_TRUE;
}

/**
 * @brief Factory function for creating an SVG <linearGradient> style structure.
 *
 * Allocates the gradient structure, sets defaults (x1=0, y1=0, x2=100%, y2=0, units=objectBoundingBox),
 * parses attributes using `_attr_parse_linear_gradient_node`, and then performs
 * recalculation based on `gradientUnits` using the handlers in `linear_tags`.
 *
 * @param loader The SVG loader context.
 * @param buf Buffer containing the element's attributes.
 * @param buflen Length of the attribute buffer.
 * @return A new Svg_Style_Gradient structure populated with linear gradient data.
 */
static Svg_Style_Gradient *
_create_linearGradient(Evas_SVG_Loader *loader, const char *buf, unsigned buflen)
{
   Svg_Style_Gradient *grad = calloc(1, sizeof(Svg_Style_Gradient));
   loader->svg_parse->style_grad = grad;
   unsigned int i;

   grad->type = SVG_LINEAR_GRADIENT;
   grad->user_space = EINA_FALSE;
   grad->linear = calloc(1, sizeof(Svg_Linear_Gradient));
   /**
    * Default value of x2 is 100%
    */
   grad->linear->x2 = 1;
   eina_simple_xml_attributes_parse(buf, buflen,
                                    _attr_parse_linear_gradient_node, loader);

   for (i = 0; i < sizeof (linear_tags) / sizeof(linear_tags[0]); i++)
     linear_tags[i].tag_recalc(loader, grad->linear, grad->user_space);

   grad->use_percentage = EINA_TRUE;

   return loader->svg_parse->style_grad;
}

/** @brief Helper macro for defining entries in the gradient_tags lookup table. */
#define GRADIENT_DEF(Name)                                   \
  { #Name, sizeof (#Name), _create_##Name }

/**
 * @brief Lookup table for SVG gradient elements.
 * Maps element tag names ("linearGradient", "radialGradient") to their
 * corresponding gradient factory functions (_create_*Gradient).
 * Note on units: Initial parsing stores lengths as percentages (0.0-1.0)
 * relative to the viewport/bbox. Recalculation converts these to absolute
 * pixel values if gradientUnits="userSpaceOnUse" is *not* set.
 */
static const struct {
   const char *tag;     /**< The gradient element tag name. */
   int sz;              /**< The size of the tag string (including null terminator). */
   Gradient_Factory_Method tag_handler; /**< The factory function for this gradient type. */
} gradient_tags[] = {
  GRADIENT_DEF(linearGradient),
  GRADIENT_DEF(radialGradient)
};

/**
 * @brief Finds the appropriate gradient factory function based on the element tag name.
 * @param name The gradient element tag name (e.g., "linearGradient").
 * @return The corresponding Gradient_Factory_Method function pointer, or NULL if not found.
 */
static Gradient_Factory_Method
_find_gradient_factory(const char  *name)
{
   unsigned int i;
   int sz = strlen(name);

   for (i = 0; i < sizeof (gradient_tags) / sizeof(gradient_tags[0]); i++)
     if (gradient_tags[i].sz - 1 == sz && !strncmp(gradient_tags[i].tag, name, sz))
       {
          return gradient_tags[i].tag_handler;
       }
   return NULL;
}

/**
 * @brief Retrieves the current parent node from the loader's context.
 * Gets the node from the top of the stack, or returns the document root if the stack is empty.
 * @param loader The SVG loader context.
 * @return Pointer to the current parent Svg_Node.
 */
static Svg_Node*
_get_parent_node_from_loader(Evas_SVG_Loader *loader)
{
   if (eina_array_count(loader->stack) > 0)
     // Get the node currently at the top of the stack
     return eina_array_data_get(loader->stack, eina_array_count(loader->stack) - 1);
   else
     // If stack is empty, the parent is the document root
     return loader->doc;
}

/**
 * @brief Callback function for handling opening XML tags during parsing.
 *
 * Identifies the tag name (e.g., "svg", "g", "path", "linearGradient", "stop").
 * Finds the appropriate factory function using `_find_group_factory`,
 * `_find_graphics_factory`, or `_find_gradient_factory`.
 * Calls the factory function to create the corresponding Svg_Node or Svg_Style_Gradient.
 * Pushes container nodes onto the loader's stack.
 * Handles <stop> elements specifically to associate them with the latest gradient.
 *
 * @param loader The SVG loader context.
 * @param content Pointer to the start of the tag content (e.g., "svg ").
 * @param length Total length of the tag content including attributes.
 * @param empty EINA_TRUE if the tag is self-closing (e.g., "<rect .../>").
 */
static void
_evas_svg_loader_xml_open_parser(Evas_SVG_Loader *loader,
                                 const char *content, unsigned int length, Eina_Bool empty)
{
   const char *attrs = NULL; // Pointer to the start of attributes within content
   int attrs_length = 0;     // Length of the attribute string
   int sz = length;
   char tag_name[20] = "";
   Factory_Method method;
   Gradient_Factory_Method gradient_method;
   Svg_Node *node = NULL, *parent;
   loader->level++;
   attrs = eina_simple_xml_tag_attributes_find(content, length);

   if (!attrs)
     {
        // parse the empty tag
        attrs = content;
        while ((attrs != NULL) && *attrs != '>')
          attrs++;
     }

   if (attrs)
     {
        // find out the tag name starting from content till sz length
        sz = attrs - content;
        while ((sz > 0) && (isspace(content[sz - 1])))
          sz--;
        if ((unsigned int)sz >= sizeof(tag_name)) return;
        strncpy(tag_name, content, sz);
        tag_name[sz] = '\0';
        attrs_length = length - sz;
     }

   if ((method = _find_group_factory(tag_name)))
     {
        //group
        if (!loader->doc)
          {
             if (strcmp(tag_name, "svg"))
               return; // Not a valid svg document
             node = method(loader, NULL, attrs, attrs_length);
             loader->doc = node;
          }
        else
          {
             if (!strcmp(tag_name, "svg")) return; //Already loadded <svg>(SvgNodeType::Doc) tag
             parent = _get_parent_node_from_loader(loader);
             node = method(loader, parent, attrs, attrs_length);
          }

        if (node->type == SVG_NODE_DEFS)
          {
             loader->doc->node.doc.defs = node;
             loader->def = node;
             if (!empty) eina_array_push(loader->stack, node);
          }
        else eina_array_push(loader->stack, node);
     }
   else if ((method = _find_graphics_factory(tag_name)))
     {
        parent = _get_parent_node_from_loader(loader);
        node = method(loader, parent, attrs, attrs_length);
     }
   else if ((gradient_method = _find_gradient_factory(tag_name)))
     {
        Svg_Style_Gradient *gradient;
        gradient = gradient_method(loader, attrs, attrs_length);
        /*FIXME: The current parsing structure does not distinguish end tags.
                 There is no way to know if the currently parsed gradient is in defs.
                 If a gradient is declared outside of defs after defs is set, it is included in the gradients of defs.
                 But finally, the loader has a gradient style list regardless of defs.
                 This is only to support this when multiple gradients are declared, even if no defs are declared.
                 refer to: https://developer.mozilla.org/en-US/docs/Web/SVG/Element/defs */
        if (loader->doc->node.doc.defs)
          {
             loader->def->node.defs.gradients = eina_list_append(loader->def->node.defs.gradients, gradient);
          }
        else
          {
             loader->gradients = eina_list_append(loader->gradients, gradient);
          }
        loader->latest_gradient = gradient;
     }
   else if (!strcmp(tag_name, "stop"))
     {
        Efl_Gfx_Gradient_Stop *stop = calloc(1, sizeof(Efl_Gfx_Gradient_Stop));
        loader->svg_parse->grad_stop = stop;
        /* default value for opacity */
        stop->a = 255;
        eina_simple_xml_attributes_parse(attrs, attrs_length,
                                    _attr_parse_stops, loader);
        if (loader->latest_gradient)
          {
             loader->latest_gradient->stops = eina_list_append(loader->latest_gradient->stops, stop);
          }
     }

}

#define POP_TAG(Tag)                            \
  { #Tag, sizeof (#Tag) }

static const struct {
   const char *tag;
   size_t sz;
} pop_array[] = {
  POP_TAG(g),
  POP_TAG(svg),
  POP_TAG(defs),
  POP_TAG(mask),
  POP_TAG(clipPath)
};

/**
 * @brief Callback function for handling closing XML tags during parsing.
 *
 * Identifies container tags ("g", "svg", "defs", "mask", "clipPath") using `pop_array`.
 * If a matching container tag is found, pops the corresponding node from the loader's stack.
 * Decrements the nesting level counter.
 *
 * @param loader The SVG loader context.
 * @param content Pointer to the start of the closing tag name (e.g., "g").
 * @param length Length of the closing tag name (unused).
 */
static void
_evas_svg_loader_xml_close_parser(Evas_SVG_Loader *loader,
                                  const char *content,
                                  unsigned int length EINA_UNUSED)
{
   unsigned int i;

   content = _skip_space(content, NULL);

   for (i = 0; i < sizeof (pop_array) / sizeof (pop_array[0]); i++)
     if (!strncmp(content, pop_array[i].tag, pop_array[i].sz - 1))
       {
          eina_array_pop(loader->stack);
          break ;
       }

   loader->level--;
}

/**
 * @brief Main callback function for the eina_simple_xml_parse engine.
 *
 * Dispatches parsing events (open tag, close tag, data, etc.) to the
 * appropriate handler functions (`_evas_svg_loader_xml_open_parser`,
 * `_evas_svg_loader_xml_close_parser`).
 *
 * @param data Pointer to the Evas_SVG_Loader context.
 * @param type The type of XML event (e.g., EINA_SIMPLE_XML_OPEN).
 * @param content Pointer to the relevant content for the event.
 * @param offset Offset of the content within the original buffer (unused).
 * @param length Length of the content.
 * @return Always returns EINA_TRUE to continue parsing.
 */
static Eina_Bool
_evas_svg_loader_parser(void *data, Eina_Simple_XML_Type type,
                        const char *content,
                        unsigned int offset EINA_UNUSED, unsigned int length)
{
   Evas_SVG_Loader *loader = data;

   switch (type)
     {
      case EINA_SIMPLE_XML_OPEN:
         _evas_svg_loader_xml_open_parser(loader, content, length, EINA_FALSE);
         break;
      case EINA_SIMPLE_XML_OPEN_EMPTY:
         _evas_svg_loader_xml_open_parser(loader, content, length, EINA_TRUE);
         break;
      case EINA_SIMPLE_XML_CLOSE:
         _evas_svg_loader_xml_close_parser(loader, content, length);
         break;
      case EINA_SIMPLE_XML_DATA:
      case EINA_SIMPLE_XML_CDATA:
      case EINA_SIMPLE_XML_DOCTYPE_CHILD:
         break;
      case EINA_SIMPLE_XML_IGNORED:
      case EINA_SIMPLE_XML_COMMENT:
      case EINA_SIMPLE_XML_DOCTYPE:
         break;

      default:
         break;
     }

   return EINA_TRUE;
}

/**
 * @brief Inherits style properties from a parent style to a child style.
 *
 * For each style property (fill paint, fill opacity, fill rule, stroke paint, etc.),
 * if the child does not have the corresponding flag set (meaning it wasn't
 * explicitly defined on the child), the property's value is copied from the parent.
 * Handles deep copying for URL references and dash arrays.
 *
 * @param child The child's Svg_Style_Property structure to update.
 * @param parent The parent's Svg_Style_Property structure to inherit from. Can be NULL.
 */
static void
_inherit_style(Svg_Style_Property *child, Svg_Style_Property *parent)
{
   if (parent == NULL)
     return; // Nothing to inherit from
   // inherit the property of parent if not present in child.
   // fill
   if (!(child->fill.flags & SVG_FILL_FLAGS_PAINT))
     {
        child->fill.paint.r = parent->fill.paint.r;
        child->fill.paint.g = parent->fill.paint.g;
        child->fill.paint.b = parent->fill.paint.b;
        child->fill.paint.none = parent->fill.paint.none;
        child->fill.paint.cur_color = parent->fill.paint.cur_color;
        child->fill.paint.url = _copy_id(parent->fill.paint.url);
     }
   if (!(child->fill.flags & SVG_FILL_FLAGS_OPACITY))
     {
        child->fill.opacity = parent->fill.opacity;
     }
   if (!(child->fill.flags & SVG_FILL_FLAGS_FILL_RULE))
     {
        child->fill.fill_rule = parent->fill.fill_rule;
     }
   // stroke
   if (!(child->stroke.flags & SVG_STROKE_FLAGS_PAINT))
     {
        child->stroke.paint.r = parent->stroke.paint.r;
        child->stroke.paint.g = parent->stroke.paint.g;
        child->stroke.paint.b = parent->stroke.paint.b;
        child->stroke.paint.none = parent->stroke.paint.none;
        child->stroke.paint.cur_color = parent->stroke.paint.cur_color;
        child->stroke.paint.url = _copy_id(parent->stroke.paint.url);
     }
   if (!(child->stroke.flags & SVG_STROKE_FLAGS_OPACITY))
     {
        child->stroke.opacity = parent->stroke.opacity;
     }
   if (!(child->stroke.flags & SVG_STROKE_FLAGS_WIDTH))
     {
        child->stroke.width = parent->stroke.width;
     }
   if (!(child->stroke.flags & SVG_STROKE_FLAGS_CAP))
     {
        child->stroke.cap = parent->stroke.cap;
     }
   if (!(child->stroke.flags & SVG_STROKE_FLAGS_JOIN))
     {
        child->stroke.join = parent->stroke.join;
     }
   if (!(child->stroke.flags & SVG_STROKE_FLAGS_DASH))
     {
        int i = 0;
        int count = parent->stroke.dash_count;
        if (count > 0)
          {
             if (child->stroke.dash) free(child->stroke.dash);
             child->stroke.dash = calloc(count, sizeof(Efl_Gfx_Dash));
             child->stroke.dash_count = count;
             for (i = 0; i < count; i++)
               {
                  child->stroke.dash[i].length = parent->stroke.dash[i].length;
                  child->stroke.dash[i].gap = parent->stroke.dash[i].gap;
               }
          }
     }
}

/**
 * @brief Recursively updates the style of a node and its descendants by applying inheritance.
 *
 * Calls `_inherit_style` to apply the parent's style to the current node,
 * then recursively calls itself for all children, passing the current node's
 * (now updated) style as the new parent style.
 *
 * @param node The starting node of the subtree to update.
 * @param parent_style The style properties inherited from the node's parent. Can be NULL for the root.
 */
void
_update_style(Svg_Node *node, Svg_Style_Property *parent_style)
{
   Eina_List *l;
   Svg_Node *child;

   _inherit_style(node->style, parent_style);

   EINA_LIST_FOREACH(node->child, l, child)
     {
        _update_style(child, node->style);
     }
}

/**
 * @brief Finds a gradient by ID in a list and returns a deep copy.
 *
 * Searches the provided list for a gradient with a matching ID. If found,
 * creates a clone using `_clone_gradient`. If the cloned gradient has a
 * reference (`xlink:href`), it attempts to find the referenced gradient in the
 * same list and copies its stops if the clone doesn't already have stops.
 *
 * @param grad_list The Eina_List of Svg_Style_Gradient pointers to search within.
 * @param id The ID of the gradient to find and duplicate.
 * @return A pointer to a newly allocated copy of the found gradient, or NULL if not found.
 *         The caller is responsible for freeing the returned structure.
 */
static Svg_Style_Gradient*
_dup_gradient(Eina_List *grad_list, const char *id)
{
   Svg_Style_Gradient *grad;
   Svg_Style_Gradient *result = NULL;
   Eina_List *l;

   EINA_LIST_FOREACH(grad_list, l, grad)
     {
        if (!strcmp(grad->id, id))
          {
             result = _clone_gradient(grad);
             break;
          }
     }

   if (result && result->ref)
     {
        EINA_LIST_FOREACH(grad_list, l, grad)
          {
             if (!strcmp(grad->id, result->ref))
               {
                  if (!result->stops)
                    {
                       result->stops = _clone_grad_stops(grad->stops);
                    }
                  //TODO properly inherit other property
                  break;
               }
         }
     }

   return result;
}

/**
 * @brief Recursively resolves gradient URL references within a node subtree.
 *
 * Traverses the node tree. For leaf nodes (nodes without children), checks if
 * the fill or stroke paint properties have a URL reference. If so, calls
 * `_dup_gradient` to find and clone the referenced gradient from the provided
 * list and assigns it to the `gradient` field in the paint structure.
 *
 * @param node The starting node of the subtree to update.
 * @param grad_list The Eina_List containing all defined Svg_Style_Gradient structures (e.g., from <defs>).
 */
void
_update_gradient(Svg_Node *node, Eina_List *grad_list)
{
   Eina_List *l;
   Svg_Node *child;

   if (node->child)
     {
        EINA_LIST_FOREACH(node->child, l, child)
          {
             _update_gradient(child, grad_list);
          }
     }
   else
     {
        if (node->style->fill.paint.url)
          {
             node->style->fill.paint.gradient = _dup_gradient(grad_list, node->style->fill.paint.url);
          }
        else if (node->style->stroke.paint.url)
          {
             node->style->stroke.paint.gradient = _dup_gradient(grad_list, node->style->stroke.paint.url);
          }
     }
}

/**
 * @brief Recursively resolves composite URL references (e.g., clip-path) within a node subtree.
 *
 * Traverses the node tree. For each node, checks if its composite style property
 * has a URL reference (`comp.url`) but the resolved node pointer (`comp.node`) is NULL.
 * If so, calls `_find_node_by_id` starting from the `root` to find the referenced
 * node (e.g., the <clipPath> element) and stores a pointer to it in `comp.node`.
 *
 * @param node The current node being processed in the traversal.
 * @param root The root node of the SVG document (or the <defs> node), used as the starting point for ID searches.
 */
static void _update_composite(Svg_Node* node, Svg_Node* root)
{
   Svg_Node *child;
   Eina_List *l;
   if (node->style->comp.url && !node->style->comp.node) {
        Svg_Node *findResult = _find_node_by_id(root, node->style->comp.url);
        if (findResult) node->style->comp.node = findResult;
   }
   EINA_LIST_FOREACH(node->child, l, child)
     {
        _update_composite(child, root);
     }
}

static Eina_Bool
evas_vg_load_file_data_svg(Vg_File_Data *vfd EINA_UNUSED)
{
   return EINA_TRUE;
}

static Eina_Bool
evas_vg_load_file_close_svg(Vg_File_Data *vfd)
{
   if (vfd->root) efl_unref(vfd->root);
   free(vfd);
   return EINA_TRUE;
}

/**
 * @brief Evas VG loader function for opening and parsing an SVG file.
 *
 * This is the main entry point for loading an SVG. It performs the following steps:
 * 1. Initializes the Evas_SVG_Loader context.
 * 2. Maps the entire SVG file content into memory.
 * 3. Parses the XML content using `eina_simple_xml_parse` with `_evas_svg_loader_parser` as the callback.
 * 4. Performs post-processing:
 *    - Inherits styles using `_update_style`.
 *    - Resolves gradient references using `_update_gradient`.
 *    - Resolves composite references (clip-path) using `_update_composite`.
 * 5. Converts the internal Svg_Node tree into an Efl VG node tree using `vg_common_svg_create_vg_node`.
 * 6. Frees the intermediate Svg_Node tree.
 * 7. Returns the final Vg_File_Data containing the Efl VG root node.
 *
 * @param file Eina_File handle for the opened SVG file.
 * @param key Optional key associated with the file (unused).
 * @param error Pointer to an integer to store the Evas load error code.
 * @return Pointer to the Vg_File_Data structure containing the loaded VG data,
 *         or NULL on failure (error code will be set).
 */
static Vg_File_Data*
evas_vg_load_file_open_svg(Eina_File *file,
                           const char *key EINA_UNUSED,
                           int *error)
{
   Evas_SVG_Loader loader = {
     .stack = NULL, .doc = NULL, .def = NULL, .gradients = NULL,
     .latest_gradient = NULL, .svg_parse = NULL, .level = 0, .result = EINA_FALSE
   };
   const char   *content;
   unsigned int  length;
   Svg_Node     *defs;

   loader.svg_parse = calloc(1, sizeof(Evas_SVG_Parser));
   length = eina_file_size_get(file);
   content = eina_file_map_all(file, EINA_FILE_SEQUENTIAL);
   if (content)
     {
       loader.stack = eina_array_new(8);
       eina_simple_xml_parse(content, length, EINA_TRUE,
                                 _evas_svg_loader_parser, &loader);

       eina_array_free(loader.stack);
       eina_file_map_free(file, (void*) content);
     }

   if (loader.doc)
     {
        _update_style(loader.doc, NULL);
        defs = loader.doc->node.doc.defs;
        if (defs)
          _update_gradient(loader.doc, defs->node.defs.gradients);
        if (loader.gradients)
          {
             Eina_List* gradient_list = loader.gradients;
             _update_gradient(loader.doc, gradient_list);
             eina_list_free(gradient_list);
          }

        _update_composite(loader.doc, loader.doc);
        if (defs) _update_composite(loader.doc, defs);

        *error = EVAS_LOAD_ERROR_NONE;
     }
   else
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
     }
   free(loader.svg_parse);

   Vg_File_Data* result = vg_common_svg_create_vg_node(loader.doc);
   vg_common_svg_node_free(loader.doc);
   return result;
}

/**
 * @brief Structure containing the function pointers for the SVG VG loader module.
 */
static Evas_Vg_Load_Func evas_vg_load_svg_func =
{
   /* .file_open */ evas_vg_load_file_open_svg,
   /* .file_close */ evas_vg_load_file_close_svg,
   /* .file_data */ evas_vg_load_file_data_svg,
   /* .frame_load - not used */ NULL,
};

/**
 * @brief Evas module initialization function.
 * Registers the SVG loader functions and the log domain.
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_vg_load_svg_func);
   _evas_vg_loader_svg_log_dom = eina_log_domain_register
     ("vg-load-svg", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_vg_loader_svg_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }
   return 1;
}

/**
 * @brief Evas module shutdown function.
 * Unregisters the log domain.
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_vg_loader_svg_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_vg_loader_svg_log_dom);
        _evas_vg_loader_svg_log_dom = -1;
     }
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "svg",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_VG_LOADER, vg_loader, svg);

#ifndef EVAS_STATIC_BUILD_VG_SVG
EVAS_EINA_MODULE_DEFINE(vg_loader, svg);
#endif
