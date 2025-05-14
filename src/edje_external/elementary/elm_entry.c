#include <assert.h>
#include "private.h"

/**
 * @brief Structure to hold parameters for an Elm_Entry widget.
 * This structure is used to pass parameters when creating or updating
 * an Elm_Entry widget externally, for example, from an Edje theme.
 */
typedef struct _Elm_Params_Entry
{
   Elm_Params base; /**< Base parameters, common to all Elm_Params. */
   const char *label; /**< The label text to set on the entry. If NULL, it's ignored. */
   const char *entry; /**< The initial text content of the entry. If NULL, it's ignored. */
   Evas_Object *icon; /**< An Evas_Object to be used as an icon. If NULL, it's ignored. */
   Eina_Bool scrollable:1; /**< Whether the entry should be scrollable. */
   Eina_Bool scrollable_exists:1; /**< Flag indicating if the scrollable parameter was provided. */
   Eina_Bool single_line:1; /**< Whether the entry should be single line. */
   Eina_Bool single_line_exists:1; /**< Flag indicating if the single_line parameter was provided. */
   Eina_Bool password:1; /**< Whether the entry should be in password mode. */
   Eina_Bool password_exists:1; /**< Flag indicating if the password parameter was provided. */
   Eina_Bool horizontal_bounce:1; /**< Whether horizontal bounce is enabled for scrolling. */
   Eina_Bool horizontal_bounce_exists:1; /**< Flag indicating if the horizontal_bounce parameter was provided. */
   Eina_Bool vertical_bounce:1; /**< Whether vertical bounce is enabled for scrolling. */
   Eina_Bool vertical_bounce_exists:1; /**< Flag indicating if the vertical_bounce parameter was provided. */
   Eina_Bool editable:1; /**< Whether the entry should be editable. */
   Eina_Bool editable_exists:1; /**< Flag indicating if the editable parameter was provided. */
   const char *line_wrap; /**< String representation of the line wrap type (e.g., "none", "char"). If NULL, it's ignored. */
} Elm_Params_Entry;

/**
 * @brief Macro to find the index of a string within an array of strings.
 * @param CHOICES The array of C-string choices.
 * @param STR The C-string to search for.
 * @return The index of STR in CHOICES if found, otherwise continues execution
 *         (intended to be used in functions that then return a default or error).
 *
 * Example:
 * @code
 * static const char *my_choices[] = {"apple", "banana", "cherry"};
 * int get_choice_idx(const char *s) {
 *   CHOICE_GET(my_choices, s); // i will be 0 for "apple", 1 for "banana", etc.
 *   return -1; // Or some default if not found by the macro's return
 * }
 * @endcode
 */
#define CHOICE_GET(CHOICES, STR)                \
  unsigned int i;                               \
  for (i = 0; i < sizeof(CHOICES)/sizeof (CHOICES)[0]; i++)         \
    if (strcmp((STR), (CHOICES)[i]) == 0)           \
      return i

/**
 * @brief Array of strings representing the possible line wrap types for an entry.
 * The order of these strings corresponds to the Elm_Wrap_Type enum values.
 * The NULL terminator is important for the CHOICE_GET macro's sizeof logic if it were
 * to rely on it, but here it primarily marks the end for iteration if needed elsewhere.
 * The actual mapping to Elm_Wrap_Type relies on the order.
 *
 * Structure of elements:
 * - "none": Corresponds to ELM_WRAP_NONE
 * - "char": Corresponds to ELM_WRAP_CHAR
 * - "word": Corresponds to ELM_WRAP_WORD
 * - "mixed": Corresponds to ELM_WRAP_MIXED
 * - NULL: Sentinel value, also aligns with ELM_WRAP_LAST if it's the next enum.
 */
static const char *entry_line_wrap_choices[] =
{
   "none", "char", "word", "mixed", NULL
};

/**
 * @brief Converts a string representation of a line wrap type to an Elm_Wrap_Type.
 * @param line_wrap_str The string representation of the line wrap type (e.g., "char", "word").
 * @return The corresponding Elm_Wrap_Type enum value. Returns ELM_WRAP_LAST if the string is not recognized.
 */
static Elm_Wrap_Type
_entry_line_wrap_choices_setting_get(const char *line_wrap_str)
{
   assert(sizeof(entry_line_wrap_choices)/
          sizeof(entry_line_wrap_choices[0]) == ELM_WRAP_LAST + 1);
   CHOICE_GET(entry_line_wrap_choices, line_wrap_str);
   return ELM_WRAP_LAST;
}

/**
 * @brief Sets the state of an Elm_Entry object based on external parameters.
 * This function is typically called during animations or state transitions defined
 * in an Edje theme, applying parameters smoothly or directly.
 *
 * @param data User data, unused in this function.
 * @param obj The Elm_Entry Evas_Object to modify.
 * @param from_params The Elm_Params_Entry representing the starting state (can be NULL).
 * @param to_params The Elm_Params_Entry representing the target state (can be NULL).
 * @param pos The position in the transition (0.0 to 1.0), unused in this function
 *            as properties are set directly without interpolation.
 */
static void
external_entry_state_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const void *from_params, const void *to_params,
                         float pos EINA_UNUSED)
{
   const Elm_Params_Entry *p;
   Eina_Bool hbounce, vbounce;
   Elm_Wrap_Type line_wrap;

   if (to_params) p = to_params;
   else if (from_params) p = from_params;
   else return;

   if (p->label)
     elm_object_text_set(obj, p->label);
   if (p->entry)
     elm_object_text_set(obj, p->entry);
   if (p->scrollable_exists)
     elm_entry_scrollable_set(obj, p->scrollable);
   if (p->single_line_exists)
     elm_entry_single_line_set(obj, p->single_line);
   if (p->password_exists)
     elm_entry_password_set(obj, p->password);
   if (p->horizontal_bounce_exists && p->vertical_bounce_exists)
     elm_scroller_bounce_set(obj, p->horizontal_bounce, p->vertical_bounce);
   else if (p->horizontal_bounce_exists || p->vertical_bounce_exists)
     {
        elm_scroller_bounce_get(obj, &hbounce, &vbounce);
        if (p->horizontal_bounce_exists)
          elm_scroller_bounce_set(obj, p->horizontal_bounce, vbounce);
        else
          elm_scroller_bounce_set(obj, hbounce, p->vertical_bounce);
     }
   if (p->editable_exists)
     elm_entry_editable_set(obj, p->editable);
   if (p->line_wrap)
     {
        line_wrap = _entry_line_wrap_choices_setting_get(p->line_wrap);
        elm_entry_line_wrap_set(obj, line_wrap);
     }
   if (p->icon)
     elm_object_part_content_set(obj, "icon", p->icon);
}

/**
 * @brief Sets a single external parameter on an Elm_Entry object.
 * This function is called by Edje to apply individual properties defined in
 * an EDC (Edje Data Collection) file to the Elm_Entry widget.
 *
 * @param data User data, unused in this function.
 * @param obj The Elm_Entry Evas_Object to modify.
 * @param param A pointer to the Edje_External_Param structure containing the
 *              parameter name, type, and value to be set.
 *              Example for `param`:
 *              - param->name: "label"
 *              - param->type: EDJE_EXTERNAL_PARAM_TYPE_STRING
 *              - param->s: "Enter text here"
 *              Or:
 *              - param->name: "password"
 *              - param->type: EDJE_EXTERNAL_PARAM_TYPE_BOOL
 *              - param->i: 1 (for EINA_TRUE)
 * @return EINA_TRUE if the parameter was successfully set, EINA_FALSE otherwise.
 */
static Eina_Bool
external_entry_param_set(void *data EINA_UNUSED, Evas_Object *obj,
                         const Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_object_text_set(obj, param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "icon"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Evas_Object *icon = external_common_param_icon_get(obj, param);
             elm_object_part_content_set(obj, "icon", icon);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "entry"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             elm_object_text_set(obj, param->s);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "scrollable"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_entry_scrollable_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "single line"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_entry_single_line_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "password"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_entry_password_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal bounce"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             Eina_Bool hbounce, vbounce;
             elm_scroller_bounce_get(obj, NULL, &vbounce);
             hbounce = !!param->i;
             elm_scroller_bounce_set(obj, hbounce, vbounce);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "vertical bounce"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             Eina_Bool hbounce, vbounce;
             elm_scroller_bounce_get(obj, &hbounce, NULL);
             vbounce = !!param->i;
             elm_scroller_bounce_set(obj, hbounce, vbounce);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "editable"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             elm_entry_editable_set(obj, param->i);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "line wrap"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Elm_Wrap_Type line_wrap;
             line_wrap = _entry_line_wrap_choices_setting_get(param->s);
             if (line_wrap == ELM_WRAP_LAST) return EINA_FALSE;
             elm_entry_line_wrap_set(obj, line_wrap);
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Retrieves a single external parameter from an Elm_Entry object.
 * This function is called by Edje to get the current value of a property
 * of the Elm_Entry widget.
 *
 * @param data User data, unused in this function.
 * @param obj The Elm_Entry Evas_Object to query.
 * @param param A pointer to an Edje_External_Param structure. The `name` field
 *              indicates which parameter to retrieve. The function will fill
 *              the appropriate value field (e.g., `s` for string, `i` for int/bool)
 *              and `type` if necessary.
 *              Example for `param` (input):
 *              - param->name: "editable"
 *              - param->type: EDJE_EXTERNAL_PARAM_TYPE_BOOL (type is often pre-filled by Edje)
 *              Example for `param` (output, on success):
 *              - param->name: "editable"
 *              - param->type: EDJE_EXTERNAL_PARAM_TYPE_BOOL
 *              - param->i: 1 (if entry is editable)
 * @return EINA_TRUE if the parameter was successfully retrieved, EINA_FALSE otherwise.
 */
static Eina_Bool
external_entry_param_get(void *data EINA_UNUSED, const Evas_Object *obj, Edje_External_Param *param)
{
   if (!strcmp(param->name, "label"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_object_text_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "icon"))
     {
        /* not easy to get icon name back from live object */
        return EINA_FALSE;
     }
   else if (!strcmp(param->name, "entry"))
     {
        if (param->type ==  EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             param->s = elm_object_text_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "scrollable"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_entry_scrollable_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "single line"))
     {
        if (param->type ==  EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_entry_single_line_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "password"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_entry_password_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "horizontal bounce"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             Eina_Bool hbounce;
             elm_scroller_bounce_get(obj, &hbounce, NULL);
             param->i = hbounce;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "vertical bounce"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             Eina_Bool vbounce;
             elm_scroller_bounce_get(obj, NULL, &vbounce);
             param->i = vbounce;
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "editable"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_BOOL)
          {
             param->i = elm_entry_editable_get(obj);
             return EINA_TRUE;
          }
     }
   else if (!strcmp(param->name, "line wrap"))
     {
        if (param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING)
          {
             Elm_Wrap_Type line_wrap;
             line_wrap = elm_entry_line_wrap_get(obj);
             param->s = entry_line_wrap_choices[line_wrap];
             return EINA_TRUE;
          }
     }

   ERR("unknown parameter '%s' of type '%s'",
       param->name, edje_external_param_type_str(param->type));

   return EINA_FALSE;
}

/**
 * @brief Parses a list of Edje_External_Param structures and creates an Elm_Params_Entry structure.
 * This function is used to convert a list of parameters, typically from an Edje
 * theme definition, into a structured Elm_Params_Entry object that can be used
 * by `external_entry_state_set`.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object associated with these parameters (used for context, e.g., icon parsing).
 * @param params An Eina_List of Edje_External_Param pointers.
 *               Example of `params` list structure:
 *               Node 1: Edje_External_Param {name="label", type=STRING, s="Name"}
 *               Node 2: Edje_External_Param {name="password", type=BOOL, i=1}
 *               ...
 * @return A pointer to a newly allocated Elm_Params_Entry structure filled with
 *         the parsed parameters. Returns NULL on allocation failure. The caller
 *         is responsible for freeing this memory using `external_entry_params_free`.
 */
static void *
external_entry_params_parse(void *data EINA_UNUSED, Evas_Object *obj,
                            const Eina_List *params)
{
   Elm_Params_Entry *mem;
   Edje_External_Param *param;
   const Eina_List *l;

   mem = ELM_NEW(Elm_Params_Entry);
   if (!mem)
     return NULL;

   external_common_icon_param_parse(&mem->icon, obj, params);

   EINA_LIST_FOREACH(params, l, param)
     {
        if (!strcmp(param->name, "label"))
          {
             mem->label = eina_stringshare_add(param->s);
          }
        else if (!strcmp(param->name, "entry"))
          {
             mem->entry = eina_stringshare_add(param->s);
          }
        else if (!strcmp(param->name, "scrollable"))
          {
             mem->scrollable = !!param->i;
             mem->scrollable_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "single line"))
          {
             mem->single_line = !!param->i;
             mem->single_line_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "password"))
          {
             mem->password = !!param->i;
             mem->password_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "horizontal bounce"))
          {
             mem->horizontal_bounce = !!param->i;
             mem->horizontal_bounce_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "vertical bounce"))
          {
             mem->vertical_bounce = !!param->i;
             mem->vertical_bounce_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "editable"))
          {
             mem->editable = !!param->i;
             mem->editable_exists = EINA_TRUE;
          }
        else if (!strcmp(param->name, "line wrap"))
          mem->line_wrap = eina_stringshare_add(param->s);
     }

   return mem;
}

/**
 * @brief Retrieves content from an Elm_Entry object.
 * Elm_Entry widgets typically do not support named content parts in the same
 * way other widgets might (e.g., a button having a "label" part). This function
 * reflects that by always returning NULL and logging an error.
 *
 * @param data User data, unused in this function.
 * @param obj The Evas_Object, unused.
 * @param content The name of the content part to retrieve, unused.
 * @return Always returns NULL for Elm_Entry.
 */
static Evas_Object *external_entry_content_get(void *data EINA_UNUSED,
                                               const Evas_Object *obj EINA_UNUSED, const char *content EINA_UNUSED)
{
   ERR("No content.");
   return NULL;
}

/**
 * @brief Frees the memory allocated for an Elm_Params_Entry structure.
 * This function should be called to release the resources associated with
 * an Elm_Params_Entry object that was created by `external_entry_params_parse`.
 *
 * @param params A pointer to the Elm_Params_Entry structure to be freed.
 */
static void
external_entry_params_free(void *params)
{
   Elm_Params_Entry *mem = params;
   if (mem->label)
     eina_stringshare_del(mem->label);
   if (mem->entry)
     eina_stringshare_del(mem->entry);
   if (mem->line_wrap)
     eina_stringshare_del(mem->line_wrap);
   free(params);
}

/**
 * @brief Defines the external parameters supported by the Elm_Entry widget.
 * This array provides metadata about each parameter that can be set or retrieved
 * externally, such as its name and type. It's used by Edje to understand how
 * to interact with the widget's properties.
 *
 * Structure of elements (Edje_External_Param_Info):
 * - DEFINE_EXTERNAL_COMMON_PARAMS: Macro that expands to common parameters like "visible", "disabled", etc.
 *   Each of these would be an Edje_External_Param_Info struct, e.g.,
 *   { "visible", EDJE_EXTERNAL_PARAM_TYPE_BOOL, ... }
 * - EDJE_EXTERNAL_PARAM_INFO_STRING("label"): Defines a string parameter named "label".
 *   This translates to: { "label", EDJE_EXTERNAL_PARAM_TYPE_STRING, ... }
 * - EDJE_EXTERNAL_PARAM_INFO_BOOL("scrollable"): Defines a boolean parameter named "scrollable".
 *   This translates to: { "scrollable", EDJE_EXTERNAL_PARAM_TYPE_BOOL, ... }
 * - ... and so on for other parameters.
 * - EDJE_EXTERNAL_PARAM_INFO_SENTINEL: Marks the end of the parameter list.
 *   This translates to: { NULL, EDJE_EXTERNAL_PARAM_TYPE_INVALID, ... }
 */
static Edje_External_Param_Info external_entry_params[] = {
   DEFINE_EXTERNAL_COMMON_PARAMS,
   EDJE_EXTERNAL_PARAM_INFO_STRING("label"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("icon"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("entry"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("scrollable"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("single line"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("password"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("horizontal bounce"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("vertical bounce"),
   EDJE_EXTERNAL_PARAM_INFO_BOOL("editable"),
   EDJE_EXTERNAL_PARAM_INFO_STRING("line_wrap"),
   EDJE_EXTERNAL_PARAM_INFO_SENTINEL
};

DEFINE_EXTERNAL_ICON_ADD(entry, "entry");
DEFINE_EXTERNAL_TYPE_SIMPLE(entry, "Entry");
