/*
    Concerning the EPC reference:

    The formatting for blocks and properties has been implemented as a table
    which is filled using ALIASES.
    For maximum flexibility I implemented them in the \@code/\@encode style,
    this means that missing one or changing the order most certainly cause
    formatting errors.

    \@block
        block name
    \@context
        code sample of the block
    \@description
        the block's description
    \@endblock

    \@property
        property name
    \@parameters
        property's parameter list
    \@effect
        the property description (lol)
    \@endproperty
*/

#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "elm_prefs_cc.h"

void check_min_arg_count(int min_required_args);
int params_min_check(int n);

/**
 * @page epcref Elementary Prefs Data Collection Reference
 * An Elementary Prefs Collection is a plain text file (normally
 * identified with the .epc extension), consisting of instructions for the
 * Elm Prefs Compiler.
 *
 * The syntax for the elementary prefs data files follows a simple structure of
 * "blocks { .. }" that can contain "properties: ..", more blocks, or both.
 *
 * @anchor sec_quickaccess Quick access to block descriptions:
 * <ul>
 *    <li>@ref sec_page "Page"</li>
 *    <li>@ref sec_items "Items"</li>
 *    <ul>
 *      <li>@ref sec_items_bool "Bool"</li>
 *      <li>@ref sec_items_int "Int"</li>
 *      <li>@ref sec_items_float "Float"</li>
 *      <li>@ref sec_items_date "Date"</li>
 *      <li>@ref sec_items_text "Text"</li>
 *      <li>@ref sec_items_textarea "Text Area"</li>
 *    </ul>
 * </ul>
 *
 * @author Ricardo de Almeida Gonzaga (ricardotk) ricardo@profusion.mobi
 *
 * <table class="edcref" border="0">
 */

/**
 * @brief Pointer to the currently processed page node in the EPC file.
 * This global variable holds the context of the page being parsed, allowing
 * subsequent item and property handlers to associate data with the correct page.
 */
static Elm_Prefs_Page_Node *current_page = NULL;
/**
 * @brief Pointer to the currently processed item node within a page in the EPC file.
 * This global variable holds the context of the item being parsed, allowing
 * property handlers to associate data with the correct item.
 */
static Elm_Prefs_Item_Node *current_item = NULL;

/* objects and statments */

/* Collection */
/**
 * @brief Handler for the 'collection' object.
 * This function is called when a 'collection' block is encountered in the
 * EPC file. It currently performs no specific actions but serves as a
 * placeholder and entry point for parsing the top-level structure.
 */
static void ob_collection(void);

/**
   @epcsection{page,Page}
 */

/**
    @page epcref
    @block
        page
    @context
        collection {
            ..
            page {
                name: "main";
                version: 1;
                title: "Preference Widgets";
                subtitle: "Subtitle";
                widget: "elm/vertical_frame";

                items { }
            }
            ..
        }
    @description
        A "page" block is a grouping of prefs items together. A prefs widget
        must always be bound to a page, that can address other pages in the
        collection too as @b sub-pages.
    @endblock

    @property
        name
    @parameters
        [page name]
    @effect
        The name of the page (to be referred by the application). It must be
        unique within the collection.
    @endproperty

    @property
        version
    @parameters
        [page version]
    @effect
        The page's version.
    @endproperty

    @property
        title
    @parameters
        [page title]
    @effect
        Set page's title.
    @endproperty

    @property
        subtitle
    @parameters
        [page subtitle]
    @effect
        Set page's subtitle.
    @endproperty

    @property
        widget
    @parameters
        [page widget]
    @effect
        Set the widget from among the available page widgets. Valid, built-in
        page widgets are:
            @li elm/horizontal_box
            @li elm/vertical_box
            @li elm/horizontal_frame
            @li elm/vertical_frame
    @endproperty

    @property
        style
    @parameters
        [page style]
    @effect
        Set page's style.
    @endproperty

    @property
        icon
    @parameters
        [page icon]
    @effect
        Set page's icon.
    @endproperty

    @property
        autosave
    @parameters
        [1 or 0]
    @effect
        Takes a boolean value specifying whether page is autosaved (1)
        or not (0). The default value is 1.
    @endproperty
*/

/* Page */
/**
 * @brief Handler for the 'page' object within a 'collection'.
 * This function is called when a 'page' block is encountered. It allocates
 * a new Elm_Prefs_Page_Node and appends it to the list of pages in the
 * global elm_prefs_file structure. It also sets current_page to this new node.
 */
static void ob_collection_page(void);
/**
 * @brief Handler for the 'name' property of a 'page' object.
 * Parses and assigns the page name. The name is expected as a single string argument.
 * Example in .epc: name: "main_settings";
 */
static void st_collection_page_name(void);
/**
 * @brief Handler for the 'version' property of a 'page' object.
 * Parses and assigns the page version. The version is expected as a single integer argument.
 * Example in .epc: version: 1;
 */
static void st_collection_page_version(void);
/**
 * @brief Handler for the 'title' property of a 'page' object.
 * Parses and assigns the page title. The title is expected as a single string argument.
 * Example in .epc: title: "General Settings";
 */
static void st_collection_page_title(void);
/**
 * @brief Handler for the 'subtitle' property of a 'page' object.
 * Parses and assigns the page subtitle. The subtitle is expected as a single string argument.
 * Example in .epc: subtitle: "Configure basic application settings";
 */
static void st_collection_page_subtitle(void);
/**
 * @brief Handler for the 'widget' property of a 'page' object.
 * Parses and assigns the page widget type. The widget type is expected as a single string argument.
 * Example in .epc: widget: "elm/vertical_frame";
 */
static void st_collection_page_widget(void);
/**
 * @brief Handler for the 'style' property of a 'page' object.
 * Parses and assigns the page style. The style is expected as a single string argument.
 * Example in .epc: style: "custom_page_style";
 */
static void st_collection_page_style(void);
/**
 * @brief Handler for the 'icon' property of a 'page' object.
 * Parses and assigns the page icon. The icon is expected as a single string argument (e.g., a file path or theme icon name).
 * Example in .epc: icon: "preferences-system";
 */
static void st_collection_page_icon(void);
/**
 * @brief Handler for the 'autosave' property of a 'page' object.
 * Parses and assigns the page autosave behavior. Expected as a single boolean argument (1 for true, 0 for false).
 * Example in .epc: autosave: 0;
 */
static void st_collection_page_autosave(void);

/**
   @epcsection{items,Items}
 */

/**
    @page epcref
    @block
        items
    @context
        page {
            ..
            items {
                name: "item";
                type: INT;
                label: "Just a item label...";

                type { }
            }
            ..
        }

    @description
        An "item" block declares a new Prefs item, along with its properties.
    @endblock

    @property
        name
    @parameters
        [item name]
    @effect
        The item's name, to be referred by the application. It must be unique
        within the page.
    @endproperty

    @property
        type
    @parameters
        [TYPE]
    @effect
        Set the type (all types must be entered in capitals) from among the
        available types, that are:
            @li BOOL - Checkbox.
            @li INT - Slider.
            @li FLOAT - Slider.
            @li DATE - Date/time display and input widget.
            @li TEXT - Single-line text entry.
            @li TEXTAREA - Text entry.
            @li LABEL - Read-only label.
            @li PAGE - Prefs subpage object.
            @li SEPARATOR - Line that serves only to divide and organize prefs
            item.
            @li SWALLOW - Swallows an Evas_Object.
            @li SAVE - Button that will get all the values bounded to items and
            save it on CFG file.
            @li ACTION - Button that will emit a signal to .C file and call a
            smart callback.
            @li RESET - Button that will return all the values bounded to items
            as default declared on .EPC file.
    @endproperty

    @property
        label
    @parameters
        [a string to label]
    @effect
        Set a string to item's label.
    @endproperty

    @property
        icon
    @parameters
        [item icon]
    @effect
        This is the item icon.
    @endproperty

    @property
        persistent
    @parameters
        [1 or 0]
    @effect
        Takes a boolean value specifying whether item is persistent (1) or
        not (0). The default value is 1. A non persistent item doesn't save
        its values when saved.
    @endproperty

    @property
        editable
    @parameters
        [1 or 0]
    @effect
        Takes a boolean value specifying whether item is editable (1)
        or not (0). The default value is 1.
    @endproperty

    @property
        visible
    @parameters
        [1 or 0]
    @effect
        Takes a boolean value specifying whether item is visible (1) or not (0).
        The default value is 1.
    @endproperty

    @property
        style
    @parameters
        [item style]
    @effect
        This is the item's style.
    @endproperty

    @property
        widget
    @parameters
        [item widget]
    @effect
        This is the item's widget, for cases where a widget differs than the
        default assigned to the type is desired.
    @endproperty
*/
/* Items */

/**
 * @brief Handler for the 'items' object within a 'page'.
 * This function is called when an 'items' block is encountered inside a 'page'
 * block. It currently performs no specific actions but serves as a structural
 * placeholder for grouping item definitions.
 */
static void ob_collection_page_items(void);

/**
 * @brief Handler for an 'item' object within 'items'.
 * This function is called when an 'item' block is encountered. It allocates
 * a new Elm_Prefs_Item_Node, initializes its default visibility, persistence,
 * and editability, and appends it to the list of items in the current_page.
 * It also sets current_item to this new node.
 */
static void ob_collection_page_items_item(void);
/**
 * @brief Handler for the 'name' property of an 'item' object.
 * Parses and assigns the item name. The name is expected as a single string argument.
 * Example in .epc: name: "enable_feature_x";
 */
static void st_collection_page_items_item_name(void);
/**
 * @brief Handler for the 'type' property of an 'item' object.
 * Parses and assigns the item type (e.g., BOOL, INT, TEXT). It also sets
 * default values for editability, persistence, and type-specific fields
 * (like min/max for INT/FLOAT, default date for DATE) based on the parsed type.
 * Example in .epc: type: BOOL;
 */
static void st_collection_page_items_item_type(void);
/**
 * @brief Handler for the 'label' property of an 'item' object.
 * Parses and assigns the item label. The label is expected as a single string argument.
 * Example in .epc: label: "Enable Feature X";
 */
static void st_collection_page_items_item_label(void);
/**
 * @brief Handler for the 'icon' property of an 'item' object.
 * Parses and assigns the item icon. The icon is expected as a single string argument.
 * Example in .epc: icon: "object-select";
 */
static void st_collection_page_items_item_icon(void);
/**
 * @brief Handler for the 'persistent' property of an 'item' object.
 * Parses and assigns the item persistence flag. Expected as a single boolean argument.
 * Example in .epc: persistent: 0;
 */
static void st_collection_page_items_item_persistent(void);
/**
 * @brief Handler for the 'editable' property of an 'item' object.
 * Parses and assigns the item editable flag. Expected as a single boolean argument.
 * Example in .epc: editable: 0;
 */
static void st_collection_page_items_item_editable(void);
/**
 * @brief Handler for the 'visible' property of an 'item' object.
 * Parses and assigns the item visibility flag. Expected as a single boolean argument.
 * Example in .epc: visible: 0;
 */
static void st_collection_page_items_item_visible(void);
/**
 * @brief Handler for the 'source' property of a 'PAGE' type 'item' object.
 * Parses and assigns the source page name for a subpage item. Expected as a single string argument.
 * This is used when an item is of type ELM_PREFS_TYPE_PAGE.
 * Example in .epc: source: "advanced_settings_page";
 */
static void st_collection_page_items_item_source(void);
/**
 * @brief Handler for the 'style' property of an 'item' object.
 * Parses and assigns the item style. The style is expected as a single string argument.
 * Example in .epc: style: "custom_item_style";
 */
static void st_collection_page_items_item_style(void);
/**
 * @brief Handler for the 'widget' property of an 'item' object.
 * Parses and assigns a custom widget for the item. Expected as a single string argument.
 * This allows overriding the default widget associated with the item's type.
 * Example in .epc: widget: "custom/my_bool_widget";
 */
static void st_collection_page_items_item_widget(void);

/**
   @epcsection{items_bool,Bool item sub blocks}
 */

/**
    @page epcref

    @block
        bool
    @context
        item {
          ..
            bool {
                default: true;
            }
          ..
        }
    @description
    @endblock

    @property
        default
    @parameters
        [true or false]
    @effect
        Takes a boolean value specifying whether item is checked (true)
        or not (false).
    @endproperty
*/

/* Item: Bool */
/**
 * @brief Handler for the 'bool' object within an 'item'.
 * This function is called when a 'bool' block is encountered inside an 'item'
 * block. It's intended for type-specific properties of a boolean item.
 * Currently, it includes a TODO to check if the current item's type matches ELM_PREFS_TYPE_BOOL.
 */
static void ob_collection_page_items_item_bool(void);
/**
 * @brief Handler for the 'default' property of a 'bool' item.
 * Parses and assigns the default boolean value for the item. Expected as a single boolean argument.
 * Example in .epc: default: true;
 */
static void st_collection_page_items_item_bool_default(void);

/**
   @epcsection{items_int,Int item sub blocks}
 */

/**
    @page epcref

    @block
        int
    @context
        item {
          ..
            int {
                default: 150;
                min: 0;
                max: 300;
            }
          ..
        }
    @description
    @endblock

    @property
        default
    @parameters
        [value]
    @effect
        Set a default (initial) value to the item.
    @endproperty

    @property
        min
    @parameters
        [value]
    @effect
        Set a minimum value to the item. Note that, without a minimum
        value, the widget implementing the item visually will get an
        available range of values bound to the minimum of the least
        integer number representable, and it might not be what you
        want.

    @endproperty

    @property
        max
    @parameters
        [value]
    @effect
        Set a maximum value to the item. Note that, without a maximum
        value, the widget implementing the item visually will get an
        available range of values bound to the maximum of the least
        integer number representable, and it might not be what you
        want.
    @endproperty
*/

/* Item: Integer */
/**
 * @brief Handler for the 'int' object within an 'item'.
 * This function is called when an 'int' block is encountered inside an 'item'
 * block. It's intended for type-specific properties of an integer item.
 * Currently, it includes a TODO to check if the current item's type matches ELM_PREFS_TYPE_INT.
 */
static void ob_collection_page_items_item_int(void);
/**
 * @brief Handler for the 'default' property of an 'int' item.
 * Parses and assigns the default integer value for the item. Expected as a single integer argument.
 * Example in .epc: default: 100;
 */
static void st_collection_page_items_item_int_default(void);
/**
 * @brief Handler for the 'max' property of an 'int' item.
 * Parses and assigns the maximum integer value for the item. Expected as a single integer argument.
 * Example in .epc: max: 255;
 */
static void st_collection_page_items_item_int_max(void);
/**
 * @brief Handler for the 'min' property of an 'int' item.
 * Parses and assigns the minimum integer value for the item. Expected as a single integer argument.
 * Example in .epc: min: 0;
 */
static void st_collection_page_items_item_int_min(void);

/**
   @epcsection{items_float,Float item sub blocks}
 */

/**
    @page epcref

    @block
        float
    @context
        item {
          ..
            float {
                default: 0.5;
                min: 0;
                max: 1;
            }
          ..
        }
    @description
    @endblock

    @property
        default
    @parameters
        [value]
    @effect
        Set a default (initial) value to the item.
    @endproperty

    @property
        min
    @parameters
        [value]
    @effect
        Set a minimum value to the item. Note that, without a minimum
        value, the widget implementing the item visually will get an
        available range of values bound to the minimum of the least
        floating point number representable, and it might not be what you
        want.
    @endproperty

    @property
        max
    @parameters
        [value]
    @effect
        Set a maximum value to the item. Note that, without a maximum
        value, the widget implementing the item visually will get an
        available range of values bound to the maximum of the least
        floating point number representable, and it might not be what you
        want.
    @endproperty
*/

/* Item: Float */
/**
 * @brief Handler for the 'float' object within an 'item'.
 * This function is called when a 'float' block is encountered inside an 'item'
 * block. It's intended for type-specific properties of a float item.
 * Currently, it includes a TODO to check if the current item's type matches ELM_PREFS_TYPE_FLOAT.
 */
static void ob_collection_page_items_item_float(void);
/**
 * @brief Handler for the 'default' property of a 'float' item.
 * Parses and assigns the default float value for the item. Expected as a single float argument.
 * Example in .epc: default: 0.75;
 */
static void st_collection_page_items_item_float_default(void);
/**
 * @brief Handler for the 'max' property of a 'float' item.
 * Parses and assigns the maximum float value for the item. Expected as a single float argument.
 * Example in .epc: max: 1.0;
 */
static void st_collection_page_items_item_float_max(void);
/**
 * @brief Handler for the 'min' property of a 'float' item.
 * Parses and assigns the minimum float value for the item. Expected as a single float argument.
 * Example in .epc: min: 0.0;
 */
static void st_collection_page_items_item_float_min(void);

/**
   @epcsection{items_date,Date item sub blocks}
 */

/**
    @page epcref

    @block
        date
    @context
        item {
          ..
            date {
                default: 2012 11 05;
                min: 1900 1 1;
                max: 2200 12 31;
            }
          ..
        }
    @description
    @endblock

    @property
        default
    @parameters
        [year month date] or "today"
    @effect
        Set a default (initial) date to the item. "today" will translate
        to current date.
    @endproperty

    @property
        min
    @parameters
        [year month date] or "today"
    @effect
        Set a minimum date to the item. "today" will translate
        to current date.
    @endproperty

    @property
        max
    @parameters
        [year month date] or "today"
    @effect
        Set a maximum date to the item. "today" will translate
        to current date.
    @endproperty
*/

/* Item: Date */
/**
 * @brief Handler for the 'date' object within an 'item'.
 * This function is called when a 'date' block is encountered inside an 'item'
 * block. It's intended for type-specific properties of a date item.
 * Currently, it includes a TODO to check if the current item's type matches ELM_PREFS_TYPE_DATE.
 */
static void ob_collection_page_items_item_date(void);
/**
 * @brief Handler for the 'default' property of a 'date' item.
 * Parses and assigns the default date for the item.
 * Accepts either "today" as a string or three integer arguments: year, month, day.
 * Example in .epc: default: 2023 12 25; or default: "today";
 */
static void st_collection_page_items_item_date_default(void);
/**
 * @brief Handler for the 'max' property of a 'date' item.
 * Parses and assigns the maximum allowable date for the item.
 * Accepts either "today" as a string or three integer arguments: year, month, day.
 * Example in .epc: max: 2030 12 31; or max: "today";
 */
static void st_collection_page_items_item_date_max(void);
/**
 * @brief Handler for the 'min' property of a 'date' item.
 * Parses and assigns the minimum allowable date for the item.
 * Accepts either "today" as a string or three integer arguments: year, month, day.
 * Example in .epc: min: 2000 01 01; or min: "today";
 */
static void st_collection_page_items_item_date_min(void);

/**
   @epcsection{items_text,Text item sub blocks}
 */

/**
    @page epcref

    @block
        text
    @context
        item {
          ..
            text {
                default: "Default text";
                placeholder: "Text:";
                accept: "^[a-zA-Z ]$";
                deny: "";
            }
          ..
        }
    @description
    @endblock

    @property
        default
    @parameters
        [a string of text]
    @effect
        Set a default text.
    @endproperty

    @property
        placeholder
    @parameters
        [a string of text]
    @effect
        Set a placeholder.
    @endproperty

    @property
        accept
    @parameters
        [regular expression]
    @effect
        Set an acceptance regular expression. It must be a valid one.
    @endproperty

    @property
        deny
    @parameters
        [regular expression]
    @effect
        Set a denial regular expression. It must be a valid one.
    @endproperty
*/

/**
   @epcsection{items_textarea,Text Area item sub blocks}
 */

/**
    @page epcref

    @block
        textarea
    @context
        item {
          ..
            textarea {
                default: "Default text";
                placeholder: "No Numbers!";
                accept: "";
                deny: "^[0-9]*$";
            }
          ..
        }
    @description
    @endblock

    @property
        default
    @parameters
        [a string of text]
    @effect
        Set a default text.
    @endproperty

    @property
        placeholder
    @parameters
        [a string of text]
    @effect
        Set a placeholder.
    @endproperty

    @property
        accept
    @parameters
        [regular expression]
    @effect
        Set an acceptance regular expression. It must be a valid one.
    @endproperty

    @property
        deny
    @parameters
        [regular expression]
    @effect
        Set a denial regular expression. It must be a valid one.
    @endproperty
*/

/* Item: Text and Text Area */
/**
 * @brief Handler for the 'text' object within an 'item'.
 * This function is called when a 'text' block is encountered inside an 'item'
 * block. It's intended for type-specific properties of a single-line text item.
 * Currently, it includes a TODO to check if the current item's type matches ELM_PREFS_TYPE_TEXT.
 */
static void ob_collection_page_items_item_text(void);
/**
 * @brief Handler for the 'textarea' object within an 'item'.
 * This function is called when a 'textarea' block is encountered inside an 'item'
 * block. It's intended for type-specific properties of a multi-line text item.
 * Currently, it includes a TODO to check if the current item's type matches ELM_PREFS_TYPE_TEXTAREA.
 */
static void ob_collection_page_items_item_textarea(void);

/* String shared statements */
/**
 * @brief Handler for the 'default' property of a 'text' or 'textarea' item.
 * Parses and assigns the default string value. Expected as a single string argument.
 * Example in .epc: default: "Initial text";
 */
static void st_collection_page_items_item_string_default(void);
/**
 * @brief Handler for the 'placeholder' property of a 'text' or 'textarea' item.
 * Parses and assigns the placeholder string value. Expected as a single string argument.
 * Example in .epc: placeholder: "Enter your name here";
 */
static void st_collection_page_items_item_string_placeholder(void);
/**
 * @brief Handler for the 'accept' property of a 'text' or 'textarea' item.
 * Parses and assigns a regular expression for accepted input. Expected as a single string argument.
 * Also checks if the provided regex is valid.
 * Example in .epc: accept: "^[a-zA-Z]+$";
 */
static void st_collection_page_items_item_string_accept(void);
/**
 * @brief Handler for the 'deny' property of a 'text' or 'textarea' item.
 * Parses and assigns a regular expression for denied input. Expected as a single string argument.
 * Also checks if the provided regex is valid.
 * Example in .epc: deny: "[0-9]";
 */
static void st_collection_page_items_item_string_deny(void);
/**
 * @brief Handler for the 'max' property (length) of a 'text' or 'textarea' item.
 * Parses and assigns the maximum allowed length for the string. Expected as a single integer argument.
 * Example in .epc: max: 255; (for max length)
 */
static void st_collection_page_items_item_string_max(void);
/**
 * @brief Handler for the 'min' property (length) of a 'text' or 'textarea' item.
 * Parses and assigns the minimum allowed length for the string. Expected as a single integer argument.
 * Example in .epc: min: 5; (for min length)
 */
static void st_collection_page_items_item_string_min(void);

/**
 * @brief Array of statement handlers.
 * This array maps EPC statement paths (e.g., "collection.page.name") to their
 * corresponding handler functions. The parser uses this table to dispatch
 * handling of properties found in the EPC file.
 *
 * Each element is a New_Statement_Handler struct:
 * @code
 * typedef struct _New_Statement_Handler New_Statement_Handler;
 * struct _New_Statement_Handler
 * {
 *    const char *name; // The full path of the statement, e.g., "collection.page.items.item.bool.default"
 *    void (*func)(void); // Pointer to the function that handles this statement
 * };
 * @endcode
 * Example entry:
 * `{"collection.page.name", st_collection_page_name}`
 * This means if the parser encounters "name: ..." inside a "page" block,
 * which is inside a "collection" block, it will call `st_collection_page_name()`.
 */
New_Statement_Handler statement_handlers[] =
{
   {"collection.page.name", st_collection_page_name},
   {"collection.page.name", st_collection_page_name},
   {"collection.page.version", st_collection_page_version},
   {"collection.page.title", st_collection_page_title},
   {"collection.page.subtitle", st_collection_page_subtitle},
   {"collection.page.widget", st_collection_page_widget},
   {"collection.page.style", st_collection_page_style},
   {"collection.page.icon", st_collection_page_icon},
   {"collection.page.autosave", st_collection_page_autosave},

   {"collection.page.items.item.name", st_collection_page_items_item_name},
   {"collection.page.items.item.type", st_collection_page_items_item_type},
   {"collection.page.items.item.label", st_collection_page_items_item_label},
   {"collection.page.items.item.icon", st_collection_page_items_item_icon},
   {"collection.page.items.item.persistent", st_collection_page_items_item_persistent},
   {"collection.page.items.item.editable", st_collection_page_items_item_editable},
   {"collection.page.items.item.visible", st_collection_page_items_item_visible},
   {"collection.page.items.item.source", st_collection_page_items_item_source},
   {"collection.page.items.item.style", st_collection_page_items_item_style},
   {"collection.page.items.item.widget", st_collection_page_items_item_widget},

   {"collection.page.items.item.bool.default", st_collection_page_items_item_bool_default},

   {"collection.page.items.item.int.default", st_collection_page_items_item_int_default},
   {"collection.page.items.item.int.max", st_collection_page_items_item_int_max},
   {"collection.page.items.item.int.min", st_collection_page_items_item_int_min},

   {"collection.page.items.item.float.default", st_collection_page_items_item_float_default},
   {"collection.page.items.item.float.max", st_collection_page_items_item_float_max},
   {"collection.page.items.item.float.min", st_collection_page_items_item_float_min},

   {"collection.page.items.item.date.default", st_collection_page_items_item_date_default},
   {"collection.page.items.item.date.max", st_collection_page_items_item_date_max},
   {"collection.page.items.item.date.min", st_collection_page_items_item_date_min},

   {"collection.page.items.item.text.placeholder", st_collection_page_items_item_string_placeholder},
   {"collection.page.items.item.text.default", st_collection_page_items_item_string_default},
   {"collection.page.items.item.text.accept", st_collection_page_items_item_string_accept},
   {"collection.page.items.item.text.deny", st_collection_page_items_item_string_deny},
   {"collection.page.items.item.text.max", st_collection_page_items_item_string_max},
   {"collection.page.items.item.text.min", st_collection_page_items_item_string_min},

   {"collection.page.items.item.textarea.placeholder", st_collection_page_items_item_string_placeholder},
   {"collection.page.items.item.textarea.default", st_collection_page_items_item_string_default},
   {"collection.page.items.item.textarea.accept", st_collection_page_items_item_string_accept},
   {"collection.page.items.item.textarea.deny", st_collection_page_items_item_string_deny},
   {"collection.page.items.item.textarea.max", st_collection_page_items_item_string_max},
   {"collection.page.items.item.textarea.min", st_collection_page_items_item_string_min}
};

/**
 * @brief Array of object handlers.
 * This array maps EPC object block paths (e.g., "collection.page.items") to their
 * corresponding handler functions. The parser uses this table to dispatch
 * handling of blocks found in the EPC file.
 *
 * Each element is a New_Object_Handler struct:
 * @code
 * typedef struct _New_Object_Handler New_Object_Handler;
 * struct _New_Object_Handler
 * {
 *    const char *name; // The full path of the object block, e.g., "collection.page.items.item"
 *    void (*func)(void); // Pointer to the function that handles this object block
 * };
 * @endcode
 * Example entry:
 * `{"collection.page", ob_collection_page}`
 * This means if the parser encounters a "page { ... }" block inside a
 * "collection { ... }" block, it will call `ob_collection_page()`.
 */
New_Object_Handler object_handlers[] =
{
   {"collection", ob_collection},
   {"collection.page", ob_collection_page},

   {"collection.page.items", ob_collection_page_items},
   {"collection.page.items.item", ob_collection_page_items_item},
   {"collection.page.items.item.bool", ob_collection_page_items_item_bool},
   {"collection.page.items.item.int", ob_collection_page_items_item_int},
   {"collection.page.items.item.float", ob_collection_page_items_item_float},
   {"collection.page.items.item.date", ob_collection_page_items_item_date},
   {"collection.page.items.item.text", ob_collection_page_items_item_text},
   {"collection.page.items.item.textarea", ob_collection_page_items_item_textarea}
};

/**
 * @brief Gets the number of registered object handlers.
 * @return The total count of object handlers in the object_handlers array.
 * This is used by the parser to know the bounds of the handler table.
 */
int
object_handler_num(void)
{
   return (sizeof(object_handlers) / sizeof (New_Object_Handler));
}

/**
 * @brief Gets the number of registered statement handlers.
 * @return The total count of statement handlers in the statement_handlers array.
 * This is used by the parser to know the bounds of the handler table.
 */
int
statement_handler_num(void)
{
   return (sizeof(statement_handlers) / sizeof (New_Statement_Handler));
}

static void
ob_collection(void)
{
   // NULL
}

static void
ob_collection_page(void)
{
   current_page = mem_alloc(SZ(Elm_Prefs_Page_Node));
   if (current_page)
     elm_prefs_file->pages =
        eina_list_append(elm_prefs_file->pages, current_page);
}

static void
st_collection_page_name(void)
{
   check_arg_count(1);

   current_page->name = parse_str(0);

   //TODO: check for duplicated name entries.
}

static void
st_collection_page_version(void)
{
   check_arg_count(1);

   current_page->version = parse_int(0);
}

static void
st_collection_page_title(void)
{
   check_arg_count(1);

   current_page->title = parse_str(0);
}

static void
st_collection_page_subtitle(void)
{
   check_arg_count(1);

   current_page->sub_title = parse_str(0);
}

static void
st_collection_page_widget(void)
{
   check_arg_count(1);

   current_page->widget = parse_str(0);
}

static void
st_collection_page_style(void)
{
   check_arg_count(1);

   current_page->style = parse_str(0);
}

static void
st_collection_page_icon(void)
{
   check_arg_count(1);

   current_page->icon = parse_str(0);
}

static void
st_collection_page_autosave(void)
{
   check_arg_count(1);

   current_page->autosave = parse_bool(0);
}

static void
ob_collection_page_items(void)
{
   // NULL
}

static void
ob_collection_page_items_item(void)
{
   current_item = mem_alloc(SZ(Elm_Prefs_Item_Node));
   if (current_item)
     {
        current_item->visible = EINA_TRUE;
        current_item->persistent = EINA_TRUE;
        current_item->editable = EINA_TRUE;
        current_page->items =
          eina_list_append(current_page->items, current_item);
     }
}

static void
st_collection_page_items_item_name(void)
{
   check_arg_count(1);

   current_item->name = parse_str(0);
}

static void
st_collection_page_items_item_type(void)
{
   check_arg_count(1);

   current_item->type = parse_enum(0,
                                   "ACTION", ELM_PREFS_TYPE_ACTION,
                                   "BOOL", ELM_PREFS_TYPE_BOOL,
                                   "INT", ELM_PREFS_TYPE_INT,
                                   "FLOAT", ELM_PREFS_TYPE_FLOAT,
                                   "DATE", ELM_PREFS_TYPE_DATE,
                                   "LABEL", ELM_PREFS_TYPE_LABEL,
                                   "PAGE", ELM_PREFS_TYPE_PAGE,
                                   "TEXT", ELM_PREFS_TYPE_TEXT,
                                   "TEXTAREA", ELM_PREFS_TYPE_TEXTAREA,
                                   "RESET", ELM_PREFS_TYPE_RESET,
                                   "SAVE", ELM_PREFS_TYPE_SAVE,
                                   "SEPARATOR", ELM_PREFS_TYPE_SEPARATOR,
                                   "SWALLOW", ELM_PREFS_TYPE_SWALLOW,
                                   NULL);

   switch (current_item->type)
     {
      case ELM_PREFS_TYPE_ACTION:
      case ELM_PREFS_TYPE_LABEL:
      case ELM_PREFS_TYPE_RESET:
      case ELM_PREFS_TYPE_SEPARATOR:
      case ELM_PREFS_TYPE_SWALLOW:
        current_item->editable = EINA_FALSE;
        current_item->persistent = EINA_FALSE;
        break;

      case ELM_PREFS_TYPE_INT:
        current_item->spec.i.max = INT_MAX;
        current_item->spec.i.min = INT_MIN;
        break;

      case ELM_PREFS_TYPE_FLOAT:
        current_item->spec.f.max = FLT_MAX;
        current_item->spec.f.min = -FLT_MAX;
        break;

      case ELM_PREFS_TYPE_DATE:
      {
         time_t t = time(NULL);
         struct tm *lt = localtime(&t);

         current_item->spec.d.def.y = lt->tm_year + 1900;
         current_item->spec.d.def.m = lt->tm_mon + 1;
         current_item->spec.d.def.d = lt->tm_mday;

         current_item->spec.d.min.y = 1900;
         current_item->spec.d.min.m = 1;
         current_item->spec.d.min.d = 1;

         current_item->spec.d.max.y = 10000 - 1900;
         current_item->spec.d.max.m = 1;
         current_item->spec.d.max.d = 1;
      }
      break;

      case ELM_PREFS_TYPE_TEXT:
      case ELM_PREFS_TYPE_TEXTAREA:
        current_item->spec.s.length.max = INT_MAX;
        break;

      default:
        break;
     }
}

static void
st_collection_page_items_item_label(void)
{
   check_arg_count(1);

   current_item->label = parse_str(0);
}

static void
st_collection_page_items_item_icon(void)
{
   check_arg_count(1);

   current_item->icon = parse_str(0);
}

static void
st_collection_page_items_item_persistent(void)
{
   check_arg_count(1);

   current_item->persistent = parse_bool(0);
}

static void
st_collection_page_items_item_editable(void)
{
   check_arg_count(1);

   current_item->editable = parse_bool(0);
}

static void
st_collection_page_items_item_visible(void)
{
   check_arg_count(1);

   current_item->visible = parse_bool(0);
}

static void
st_collection_page_items_item_source(void)
{
   check_arg_count(1);

   current_item->spec.p.source = parse_str(0);
}

static void
st_collection_page_items_item_style(void)
{
   check_arg_count(1);

   current_item->style = parse_str(0);
}

static void
st_collection_page_items_item_widget(void)
{
   check_arg_count(1);

   current_item->widget = parse_str(0);
}

static void
ob_collection_page_items_item_bool(void)
{
   //TODO: check if current item type match
}

static void
st_collection_page_items_item_bool_default(void)
{
   check_arg_count(1);

   current_item->spec.b.def = parse_bool(0);
}

static void
ob_collection_page_items_item_int(void)
{
   //TODO: check if current item type match
}

static void
st_collection_page_items_item_int_default(void)
{
   check_arg_count(1);

   current_item->spec.i.def = parse_int(0);
}

static void
st_collection_page_items_item_int_max(void)
{
   check_arg_count(1);

   current_item->spec.i.max = parse_int(0);
}

static void
st_collection_page_items_item_int_min(void)
{
   check_arg_count(1);

   current_item->spec.i.min = parse_int(0);
}

static void
ob_collection_page_items_item_float(void)
{
   //TODO: check if current item type match
}

static void
st_collection_page_items_item_float_default(void)
{
   check_arg_count(1);

   current_item->spec.f.def = parse_float(0);
}

static void
st_collection_page_items_item_float_max(void)
{
   check_arg_count(1);

   current_item->spec.f.max = parse_float(0);
}

static void
st_collection_page_items_item_float_min(void)
{
   check_arg_count(1);

   current_item->spec.f.min = parse_float(0);
}

static void
ob_collection_page_items_item_date(void)
{
   //TODO: check if current item type match
}

static void
st_collection_page_items_item_date_default(void)
{
   check_min_arg_count(1);

   if (params_min_check(1))
     {
        check_arg_count(3);

        current_item->spec.d.def.y = parse_int_range(0, 1900, 10000);
        current_item->spec.d.def.m = parse_int_range(1, 1, 12);
        current_item->spec.d.def.d = parse_int_range(2, 1, 31);
     }
   else
     {
        const char *date = parse_str(0);
        if (!strcasecmp(date, "today"))
          {
             time_t t = time(NULL);
             struct tm *lt = localtime(&t);

             current_item->spec.d.def.y = lt->tm_year + 1900;
             current_item->spec.d.def.m = lt->tm_mon + 1;
             current_item->spec.d.def.d = lt->tm_mday;
          }
        free((void *)date);
     }
}

static void
st_collection_page_items_item_date_max(void)
{
   check_min_arg_count(1);

   if (params_min_check(1))
     {
        check_arg_count(3);

        current_item->spec.d.max.y = parse_int_range(0, 1900, 10000);
        current_item->spec.d.max.m = parse_int_range(1, 1, 12);
        current_item->spec.d.max.d = parse_int_range(2, 1, 31);
     }
   else
     {
        const char *date = parse_str(0);
        if (!strcasecmp(date, "today"))
          {
             time_t t = time(NULL);
             struct tm *lt = localtime(&t);

             current_item->spec.d.max.y = lt->tm_year + 1900;
             current_item->spec.d.max.m = lt->tm_mon + 1;
             current_item->spec.d.max.d = lt->tm_mday;
          }
        free((void *)date);
     }
}

static void
st_collection_page_items_item_date_min(void)
{
   check_min_arg_count(1);

   if (params_min_check(1))
     {
        check_arg_count(3);

        current_item->spec.d.min.y = parse_int_range(0, 1900, 10000);
        current_item->spec.d.min.m = parse_int_range(1, 1, 12);
        current_item->spec.d.min.d = parse_int_range(2, 1, 31);
     }
   else
     {
        const char *date = parse_str(0);
        if (!strcasecmp(date, "today"))
          {
             time_t t = time(NULL);
             struct tm *lt = localtime(&t);

             current_item->spec.d.min.y = lt->tm_year + 1900;
             current_item->spec.d.min.m = lt->tm_mon + 1;
             current_item->spec.d.min.d = lt->tm_mday;
          }
        free((void *)date);
     }
}

static void
ob_collection_page_items_item_text(void)
{
   //TODO: check if current item type match
}

static void
ob_collection_page_items_item_textarea(void)
{
   //todo: check if current item type match
}

static void
st_collection_page_items_item_string_placeholder(void)
{
   check_arg_count(1);

   current_item->spec.s.placeholder = parse_str(0);
}

static void
st_collection_page_items_item_string_default(void)
{
   check_arg_count(1);

   current_item->spec.s.def = parse_str(0);
}

static void
st_collection_page_items_item_string_accept(void)
{
   check_arg_count(1);

   current_item->spec.s.accept = parse_str(0);

   check_regex(current_item->spec.s.accept);
}

static void
st_collection_page_items_item_string_deny(void)
{
   check_arg_count(1);

   current_item->spec.s.deny = parse_str(0);

   check_regex(current_item->spec.s.deny);
}

static void
st_collection_page_items_item_string_max(void)
{
   check_arg_count(1);

   current_item->spec.s.length.max = parse_int(0);
}

static void
st_collection_page_items_item_string_min(void)
{
   check_arg_count(1);

   current_item->spec.s.length.min = parse_int(0);
}
