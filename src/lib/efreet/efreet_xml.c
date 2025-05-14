#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef _WIN32
# include <evil_private.h> /* mmap */
#else
# include <sys/mman.h>
#endif

#include <Ecore_File.h>

/* define macros and variable for using the eina logging system  */
#define EFREET_MODULE_LOG_DOM _efreet_xml_log_dom
static int _efreet_xml_log_dom = -1;

#include "Efreet.h"
#include "efreet_private.h"
#include "efreet_xml.h"

#if 0
static void efreet_xml_dump(Efreet_Xml *xml, int level);
#endif

/**
 * @internal
 * @brief Parses a chunk of XML data to build an Efreet_Xml tree.
 * @param data Pointer to the pointer of the XML data string. This will be advanced as data is consumed.
 * @param size Pointer to the remaining size of the XML data. This will be decremented as data is consumed.
 * @param error Pointer to an integer that will be set to 1 if an error occurs.
 * @return A pointer to the newly created Efreet_Xml node, or NULL on failure or if no more tags are found.
 *
 * This function recursively parses XML data. It first tries to parse a tag,
 * then its attributes. It checks if the tag is empty (e.g., <tag/>).
 * If not empty, it parses any text content and then recursively calls itself
 * to parse child nodes until a closing tag is found.
 */
static Efreet_Xml *efreet_xml_parse(char **data, int *size, int *error);

/**
 * @internal
 * @brief Parses the name of an XML tag from the data.
 * @param data Pointer to the pointer of the XML data string.
 * @param size Pointer to the remaining size of the XML data.
 * @param tag Pointer to a const char* that will store the parsed tag name (as a stringshare).
 * @param error Pointer to an integer that will be set to 1 if an error occurs.
 * @return 1 on success, 0 on failure (e.g., end tag found, no start tag, or parse error).
 *
 * This function searches for the start of a tag '<', skips comments and XML directives,
 * and then extracts the tag name until a non-alphabetic character is encountered.
 */
static int efreet_xml_tag_parse(char **data, int *size,
                                const char **tag, int *error);
/**
 * @internal
 * @brief Parses attributes of an XML tag.
 * @param data Pointer to the pointer of the XML data string.
 * @param size Pointer to the remaining size of the XML data.
 * @param attributes Pointer to a `Efreet_Xml_Attribute**` that will be allocated and filled
 *                   with parsed attributes. The array is NULL-terminated.
 *                   Example: `attributes` will point to an array like:
 *                   `[Efreet_Xml_Attribute*, Efreet_Xml_Attribute*, ..., NULL]`
 *                   where each `Efreet_Xml_Attribute` has `key` and `value` strings.
 * @param error Pointer to an integer that will be set to 1 if an error occurs.
 *
 * This function iterates through the data, identifying attribute key-value pairs
 * (e.g., key="value"). It stops when it encounters '>' or runs out of data.
 * A fixed-size buffer `attr[11]` is used to temporarily store attributes before
 * allocating the final `attributes` array.
 */
static void efreet_xml_attributes_parse(char **data, int *size,
                                        Efreet_Xml_Attribute ***attributes,
                                        int *error);
/**
 * @internal
 * @brief Parses the text content within an XML tag.
 * @param data Pointer to the pointer of the XML data string.
 * @param size Pointer to the remaining size of the XML data.
 * @param text Pointer to a const char* that will store the parsed text content (as a stringshare).
 *
 * This function skips leading whitespace, then reads characters until it encounters
 * a '<' (start of a new tag). Trailing whitespace from the collected text is then removed.
 * The resulting text is stored in `*text`.
 */
static void efreet_xml_text_parse(char **data, int *size, const char **text);

/**
 * @internal
 * @brief Checks if the current XML tag is an empty (self-closing) tag.
 * @param data Pointer to the pointer of the XML data string.
 * @param size Pointer to the remaining size of the XML data.
 * @param error Pointer to an integer that will be set to 1 if an error occurs (e.g., malformed tag).
 * @return 1 if the tag is empty (e.g., <tag/>), 0 if it's an opening tag (e.g., <tag>).
 *
 * This function looks for '/>' to indicate an empty tag or '>' to indicate an opening tag.
 */
static int efreet_xml_tag_empty(char **data, int *size, int *error);
/**
 * @internal
 * @brief Checks for and consumes a closing XML tag.
 * @param data Pointer to the pointer of the XML data string.
 * @param size Pointer to the remaining size of the XML data.
 * @param tag The expected closing tag name.
 * @param error Pointer to an integer that will be set to 1 if an error occurs (e.g., mismatched tag).
 * @return 1 if the correct closing tag is found and consumed, 0 otherwise (e.g., it's not a closing tag, or it's a different tag).
 *
 * This function looks for '</' followed by the provided `tag` name.
 */
static int efreet_xml_tag_close(char **data, int *size,
                                const char *tag, int *error);

/**
 * @internal
 * @brief Callback function used by Eina_List to free Efreet_Xml nodes.
 * @param data A pointer to an Efreet_Xml node.
 *
 * This function simply calls efreet_xml_del() on the provided data.
 */
static void efreet_xml_cb_attribute_free(void *data);

/**
 * @internal
 * @brief Skips an XML comment block.
 * @param data Pointer to the pointer of the XML data string.
 * @param size Pointer to the remaining size of the XML data.
 *
 * This function advances the `data` pointer past an XML comment,
 * which starts with `<!--` (already consumed by caller) and ends with `-->`.
 */
static void efreet_xml_comment_skip(char **data, int *size);

static int _efreet_xml_init_count = 0;

/**
 * @internal
 * @return Returns > 0 on success or 0 on failure
 * @brief Initialize the XML parser subsystem
 */
int
efreet_xml_init(void)
{
    _efreet_xml_init_count++;
    if (_efreet_xml_init_count > 1) return _efreet_xml_init_count;
    _efreet_xml_log_dom = eina_log_domain_register
      ("efreet_xml", EFREET_DEFAULT_LOG_COLOR);
    if (_efreet_xml_log_dom < 0)
    {
        _efreet_xml_init_count--;
        EINA_LOG_ERR("Efreet: Could not create a log domain for efreet_xml.");
        return _efreet_xml_init_count;
    }
    return _efreet_xml_init_count;
}

/**
 * @internal
 * @returns the number of initializations left for this system
 * @brief Attempts to shut down the subsystem if nothing else is using it
 */
void
efreet_xml_shutdown(void)
{
    _efreet_xml_init_count--;
    if (_efreet_xml_init_count > 0) return;
    eina_log_domain_unregister(_efreet_xml_log_dom);
    _efreet_xml_log_dom = -1;
}

/**
 * @internal
 * @param file The file to parse
 * @return Returns an Efreet_Xml structure for the given file @a file or
 * NULL on failure
 * @brief Parses the given file into an Efreet_Xml structure.
 */
Efreet_Xml *
efreet_xml_new(const char *file)
{
    Efreet_Xml *xml = NULL;
    int size, fd = -1;
    char *tmp, *data = MAP_FAILED;
    struct stat st;
    int error = 0;

    if (!file) return NULL;

    fd = open(file, O_RDONLY);
    if (fd == -1) goto efreet_error;

    if (fstat(fd, &st) < 0) goto efreet_error;
    /* empty file: not an error */
    if (!st.st_size)
      {
         close(fd);
         return NULL;
      }

    /* let's make mmap safe and just get 0 pages for IO erro */
    eina_mmap_safety_enabled_set(EINA_TRUE);

    data = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) goto efreet_error;

    tmp = data;

    error = 0;
    size = st.st_size;
    xml = efreet_xml_parse(&tmp, &size, &error);
    if (!xml || error) goto efreet_error;
    munmap(data, st.st_size);
    close(fd);
    return xml;

efreet_error:
    ERR("could not parse xml file '%s'", file);
    if (data != MAP_FAILED) munmap(data, size);
    if (fd != -1) close(fd);
    if (xml) efreet_xml_del(xml);
    return NULL;
}

/**
 * @internal
 * @param xml The Efree_Xml to free
 * @return Returns no value
 * @brief Frees up the given Efreet_Xml structure
 */
void
efreet_xml_del(Efreet_Xml *xml)
{
    IF_FREE_LIST(xml->children, efreet_xml_cb_attribute_free);

    if (xml->tag) eina_stringshare_del(xml->tag);
    if (xml->attributes)
    {
        Efreet_Xml_Attribute **curr;

        curr = xml->attributes;
        while (*curr)
        {
            eina_stringshare_del((*curr)->key);
            eina_stringshare_del((*curr)->value);

            FREE(*curr);
            curr++;
        }
        FREE(xml->attributes);
    }
    IF_RELEASE(xml->text);
    FREE(xml);
}

/**
 * @param xml The xml struct to work with
 * @param key The attribute key to look for
 * @return Returns the value for the given key, or NULL if none found
 * @brief Retrieves the value for the given attribute key
 */
const char *
efreet_xml_attribute_get(Efreet_Xml *xml, const char *key)
{
    Efreet_Xml_Attribute **curr;

    if (!xml || !key || !xml->attributes) return NULL;

    for (curr = xml->attributes; *curr; curr++)
    {
        if (!strcmp((*curr)->key, key))
            return (*curr)->value;
    }
    return NULL;
}

static void
efreet_xml_cb_attribute_free(void *data)
{
    efreet_xml_del(data);
}

#if 0
static void
efreet_xml_dump(Efreet_Xml *xml, int level)
{
    int i;

    for (i = 0; i < level; i++)
        printf("\t");
    printf("<%s", xml->tag);
    if (xml->attributes)
    {
        Efreet_Xml_Attribute **curr;
        for (curr = xml->attributes; *curr; curr++)
            printf(" %s=\"%s\"", (*curr)->key, (*curr)->value);
    }

    if (xml->children)
    {
        Efreet_Xml *child;
        Eina_List *l;

        printf(">");

        EINA_LIST_FOREACH(xml->children, l, child)
            efreet_xml_dump(child, level + 1);

        for (i = 0; i < level; i++)
            printf("\t");
        printf("</%s>", xml->tag);
    }
    else if (xml->text)
        printf(">%s</%s>\n", xml->text, xml->tag);
    else
        printf("/>\n");
}
#endif

static Efreet_Xml *
efreet_xml_parse(char **data, int *size, int *error)
{
    Efreet_Xml *xml, *sub_xml;
    const char *tag = NULL;

    if (*size <= 0) return NULL;

    /* parse this tag */
    if (!efreet_xml_tag_parse(data, size, &(tag), error)) return NULL;
    xml = NEW(Efreet_Xml, 1);
    if (!xml)
    {
        eina_stringshare_del(tag);
        return NULL;
    }

    xml->children = NULL;

    xml->tag = tag;
    efreet_xml_attributes_parse(data, size, &(xml->attributes), error);

    /* Check whether element is empty */
    if (efreet_xml_tag_empty(data, size, error)) return xml;
    efreet_xml_text_parse(data, size, &(xml->text));

    /* Check whether element is closed */
    if (efreet_xml_tag_close(data, size, xml->tag, error)) return xml;

    while ((sub_xml = efreet_xml_parse(data, size, error)))
        xml->children = eina_list_append(xml->children, sub_xml);

    efreet_xml_tag_close(data, size, xml->tag, error);

    return xml;
}

static int
efreet_xml_tag_parse(char **data, int *size, const char **tag, int *error)
{
    const char *start = NULL, *end = NULL;
    char buf[256];
    int buf_size;

    /* Search for tag */
    while (*size > 1)
    {
        /* Check for tag start */
        if (**data == '<')
        {
            /* Check for end tag */
            if (*(*data + 1) == '/') return 0;

            /* skip comments */
            if (*size > 3 && *(*data + 1) == '!' && *(*data + 2) == '-' && *(*data + 3) == '-')
            {
                (*data) += 3;
                (*size) -= 3;
                efreet_xml_comment_skip(data, size);
                continue;
            }

            /* Check for xml directives (and ignore them) */
            else if ((*(*data + 1) != '!') && (*(*data + 1) != '?'))
            {
                (*size)--;
                (*data)++;
                start = *data;
                break;
            }
        }
        (*size)--;
        (*data)++;
    }

    if (!start)
    {
        ERR("missing start tag");
        *error = 1;
        return 0;
    }

    while (*size > 0)
    {
        if (!isalpha(**data))
        {
            end = *data;
            break;
        }
        (*size)--;
        (*data)++;
    }

    if (!end)
    {
        ERR("no end of tag");
        *error = 1;
        return 0;
    }

    buf_size = end - start + 1;
    if (buf_size <= 1)
    {
        ERR("no tag name");
        *error = 1;
        return 0;
    }

    if (buf_size > 256) buf_size = 256;
    memcpy(buf, start, buf_size - 1);
    buf[buf_size - 1] = '\0';
    *tag = eina_stringshare_add(buf);

    return 1;
}

static void
efreet_xml_attributes_parse(char **data, int *size,
        Efreet_Xml_Attribute ***attributes, int *error)
{
    Efreet_Xml_Attribute attr[11];
    int i, count = 0;

    memset(attr, 0, sizeof(attr));
    while (*size > 0)
    {
        if (**data == '>')
        {
            (*size)++;
            (*data)--;
            break;
        }
        else if ((count < 10) && (isalpha(**data)))
        {
            /* beginning of key */
            const char *start = NULL, *end = NULL;
            char buf[256];
            int buf_size;

            start = *data;
            while ((*size > 0) && ((isalpha(**data)) || (**data == '_')))
            {
                (*size)--;
                (*data)++;
            }

            end = *data;
            buf_size = end - start + 1;
            if (buf_size <= 1)
            {
                ERR("zero length key");
                goto efreet_error;
            }

            if (buf_size > 256) buf_size = 256;
            memcpy(buf, start, buf_size - 1);
            buf[buf_size - 1] = '\0';
            attr[count].key = eina_stringshare_add(buf);

            /* search for '=', key/value seperator */
            start = NULL;
            while (*size > 0)
            {
                if (**data == '=')
                {
                    start = *data;
                    break;
                }
                (*size)--;
                (*data)++;
            }

            if (!start)
            {
                ERR("missing value for attribute!");
                goto efreet_error;
            }

            /* search for '"', beginning of value */
            start = NULL;
            while (*size > 0)
            {
                if (**data == '"')
                {
                    start = *data;
                    break;
                }
                (*size)--;
                (*data)++;
            }

            if (!start)
            {
                ERR("erroneous value for attribute!");
                goto efreet_error;
            }

            /* skip '"' */
            start++;
            (*size)--;
            (*data)++;

            /* search for '"', end of value */
            end = NULL;
            while (*size > 0)
            {
                if (**data == '"')
                {
                    end = *data;
                    break;
                }
                (*size)--;
                (*data)++;
            }

            if (!end)
            {
                ERR("erroneous value for attribute!");
                goto efreet_error;
            }

            buf_size = end - start + 1;
            if (buf_size <= 1)
            {
                ERR("zero length value");
                goto efreet_error;
            }

            if (buf_size > 256) buf_size = 256;
            memcpy(buf, start, buf_size - 1);
            buf[buf_size - 1] = '\0';
            attr[count].value = eina_stringshare_add(buf);

            count++;
        }

        (*size)--;
        (*data)++;
    }

    *attributes = NEW(Efreet_Xml_Attribute *, count + 1);
    if (!*attributes) goto efreet_error;
    for (i = 0; i < count; i++)
    {
        (*attributes)[i] = malloc(sizeof(Efreet_Xml_Attribute));
        (*attributes)[i]->key = attr[i].key;
        (*attributes)[i]->value = attr[i].value;
    }
    return;

efreet_error:
    while (count >= 0)
    {
        if (attr[count].key) eina_stringshare_del(attr[count].key);
        if (attr[count].value) eina_stringshare_del(attr[count].value);
        count--;
    }
    *error = 1;
    return;
}

static void
efreet_xml_text_parse(char **data, int *size, const char **text)
{
    const char *start = NULL, *end = NULL;
    int buf_size;

    /* skip leading whitespace */
    while (*size > 0)
    {
        if (!isspace(**data))
        {
            start = *data;
            break;
        }
        (*size)--;
        (*data)++;
    }

    if (!start) return;

    /* find next tag */
    while (*size > 0)
    {
        if (**data == '<')
        {
            end = *data;
            break;
        }
        (*size)--;
        (*data)++;
    }
    if (!end) return;

    /* skip trailing whitespace */
    while (isspace(*(end - 1))) end--;

    /* copy text */
    buf_size = end - start + 1;
    if (buf_size <= 1) return;

    *text = eina_stringshare_add_length(start, buf_size - 1);
}

static int
efreet_xml_tag_empty(char **data, int *size, int *error)
{
    while (*size > 1)
    {
        if (**data == '/')
        {
            (*size)--;
            (*data)++;
            if (**data == '>')
            {
                (*size)--;
                (*data)++;
                return 1;
            }
        }
        else if (**data == '>')
        {
            (*size)--;
            (*data)++;
            return 0;
        }
        (*size)--;
        (*data)++;
    }
    ERR("missing end of tag");
    *error = 1;

    return 1;
}

static int
efreet_xml_tag_close(char **data, int *size, const char *tag, int *error)
{
    while (*size > 1)
    {
        if (**data == '<')
        {
            if (*(*data + 1) == '/')
            {
                (*size) -= 2;
                (*data) += 2;
                if ((int)strlen(tag) > *size)
                {
                    ERR("wrong end tag");
                    *error = 1;
                    return 1;
                }
                else
                {
                    char *tmp;
                    tmp = *data;
                    while ((*tag) && (*tmp == *tag))
                    {
                        tmp++;
                        tag++;
                    }

                    if (*tag)
                    {
                        ERR("wrong end tag");
                        *error = 1;
                        return 1;
                    }
                }
                return 1;
            }
            else return 0;
        }
        (*size)--;
        (*data)++;
    }
    return 0;
}

static void
efreet_xml_comment_skip(char **data, int *size)
{
    while (*size > 2)
    {
        if (**data == '-' && *(*data + 1) == '-' && *(*data + 2) == '>')
        {
            (*data) += 3;
            (*size) -= 3;
            return;
        }
        (*data)++;
        (*size)--;
    }
}
