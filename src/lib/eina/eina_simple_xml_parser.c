/* EINA - EFL data type library
 * Copyright (C) 2011 Gustavo Sverzut Barbieri
 *                    Cedric Bail
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "eina_config.h"
#include "eina_private.h"
#include "eina_alloca.h"
#include "eina_log.h"
#include "eina_mempool.h"
#include "eina_stringshare.h"
#include "eina_strbuf.h"
#include "eina_simple_xml_parser.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

static Eina_Mempool *_eina_simple_xml_tag_mp = NULL;
static Eina_Mempool *_eina_simple_xml_attribute_mp = NULL;
static int _eina_simple_xml_log_dom = -1;

static const char EINA_MAGIC_SIMPLE_XML_TAG_STR[] = "Eina Simple XML Tag";
static const char EINA_MAGIC_SIMPLE_XML_DATA_STR[] = "Eina Simple XML Data";
static const char EINA_MAGIC_SIMPLE_XML_ATTRIBUTE_STR[] = "Eina Simple XML Attribute";

#define EINA_MAGIC_CHECK_TAG(d, ...)                            \
  do {                                                          \
     if (!EINA_MAGIC_CHECK(d, EINA_MAGIC_SIMPLE_XML_TAG))       \
       {                                                        \
          EINA_MAGIC_FAIL(d, EINA_MAGIC_SIMPLE_XML_TAG);        \
          return __VA_ARGS__;                                   \
       }                                                        \
  } while(0)

#define EINA_MAGIC_CHECK_DATA(d, ...)                           \
  do {                                                          \
     if (!EINA_MAGIC_CHECK(d, EINA_MAGIC_SIMPLE_XML_DATA))      \
       {                                                        \
          EINA_MAGIC_FAIL(d, EINA_MAGIC_SIMPLE_XML_DATA);       \
          return __VA_ARGS__;                                   \
       }                                                        \
  } while(0)

#define EINA_MAGIC_CHECK_ATTRIBUTE(d, ...)                      \
  do {                                                          \
     if (!EINA_MAGIC_CHECK(d, EINA_MAGIC_SIMPLE_XML_ATTRIBUTE)) \
       {                                                        \
          EINA_MAGIC_FAIL(d, EINA_MAGIC_SIMPLE_XML_ATTRIBUTE);  \
          return __VA_ARGS__;                                   \
       }                                                        \
  } while(0)


#ifndef EINA_LOG_COLOR_DEFAULT
#define EINA_LOG_COLOR_DEFAULT EINA_COLOR_CYAN
#endif

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_eina_simple_xml_log_dom, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_eina_simple_xml_log_dom, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_eina_simple_xml_log_dom, __VA_ARGS__)

/**
 * @internal
 * @brief Finds the first whitespace character in a string segment.
 * @param itr Pointer to the start of the string segment.
 * @param itr_end Pointer to the end of the string segment.
 * @return Pointer to the first whitespace character, or itr_end if none is found.
 */
static inline const char *
_eina_simple_xml_whitespace_find(const char *itr, const char *itr_end)
{
   for (; itr < itr_end; itr++)
     if (isspace((unsigned char)*itr)) break;
   return itr;
}

/**
 * @internal
 * @brief Skips leading whitespace characters in a string segment.
 * @param itr Pointer to the start of the string segment.
 * @param itr_end Pointer to the end of the string segment.
 * @return Pointer to the first non-whitespace character, or itr_end if all are whitespace.
 */
static inline const char *
_eina_simple_xml_whitespace_skip(const char *itr, const char *itr_end)
{
   for (; itr < itr_end; itr++)
     if (!isspace((unsigned char)*itr)) break;
   return itr;
}

/**
 * @internal
 * @brief Skips trailing whitespace characters in a string segment (moves backward).
 * @param itr Pointer to the character *after* the end of the relevant part of the string segment.
 * @param itr_start Pointer to the beginning of the string segment.
 * @return Pointer to the character *after* the last non-whitespace character.
 */
static inline const char *
_eina_simple_xml_whitespace_unskip(const char *itr, const char *itr_start)
{
   for (itr--; itr > itr_start; itr--)
     if (!isspace((unsigned char)*itr)) break;
   return itr + 1;
}

/**
 * @internal
 * @brief Finds the start of an XML tag ('<').
 * @param itr Pointer to the start of the string segment.
 * @param itr_end Pointer to the end of the string segment.
 * @return Pointer to the '<' character, or NULL if not found.
 */
static inline const char *
_eina_simple_xml_tag_start_find(const char *itr, const char *itr_end)
{
   return memchr(itr, '<', itr_end - itr);
}

/**
 * @internal
 * @brief Finds the end of an XML tag ('>' or '<' if nested/error).
 *        Handles quoted attributes to avoid premature termination.
 * @param itr Pointer to the start of the string segment (inside a tag).
 * @param itr_end Pointer to the end of the string segment.
 * @return Pointer to the '>' or '<' character, or NULL if not found.
 */
static inline const char *
_eina_simple_xml_tag_end_find(const char *itr, const char *itr_end)
{
   Eina_Bool inside_quote = EINA_FALSE;
   for (; itr < itr_end; itr++)
     {
        if (*itr == '"') inside_quote = !inside_quote;
        if (!inside_quote)
          {
             if ((*itr == '>') || (*itr == '<')) /* consider < also ends a tag */
               return itr;
          }
     }
   return NULL;
}

/**
 * @internal
 * @brief Finds the end of an XML comment ("-->").
 * @param itr Pointer to the start of the string segment (inside a comment).
 * @param itr_end Pointer to the end of the string segment.
 * @return Pointer to the character *after* "-->", or NULL if not found.
 */
static inline const char *
_eina_simple_xml_tag_comment_end_find(const char *itr, const char *itr_end)
{
   for (; itr < itr_end; itr++)
     if ((*itr == '-') &&
         ((itr + 1 < itr_end) && (*(itr + 1) == '-')) &&
         ((itr + 2 < itr_end) && (*(itr + 2) == '>')))
       return itr + 2;
   return NULL;
}

/**
 * @internal
 * @brief Finds the end of a CDATA section ("]]>").
 * @param itr Pointer to the start of the string segment (inside a CDATA section).
 * @param itr_end Pointer to the end of the string segment.
 * @return Pointer to the character *after* "]]>", or NULL if not found.
 */
static inline const char *
_eina_simple_xml_tag_cdata_end_find(const char *itr, const char *itr_end)
{
   for (; itr < itr_end; itr++)
     if ((*itr == ']') &&
         ((itr + 1 < itr_end) && (*(itr + 1) == ']')) &&
         ((itr + 2 < itr_end) && (*(itr + 2) == '>')))
       return itr + 2;
   return NULL;
}

/**
 * @internal
 * @brief Finds the end of a DOCTYPE child declaration ('>').
 * @param itr Pointer to the start of the string segment (inside a DOCTYPE child).
 * @param itr_end Pointer to the end of the string segment.
 * @return Pointer to the '>' character, or NULL if not found.
 */
static inline const char *
_eina_simple_xml_tag_doctype_child_end_find(const char *itr, const char *itr_end)
{
   for (; itr < itr_end; itr++)
     if (*itr == '>')
       return itr;
   return NULL;
}

/**
 * @endcond
 */

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/


/**
 * @internal
 * @brief Initialize the simple xml parser module.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function sets up the simple xml parser module of Eina. It is called by
 * eina_init().
 *
 * @see eina_init()
 */
Eina_Bool
eina_simple_xml_init(void)
{
   const char *choice, *tmp;

   _eina_simple_xml_log_dom = eina_log_domain_register("eina_simple_xml",
                                                       EINA_LOG_COLOR_DEFAULT);
   if (_eina_simple_xml_log_dom < 0)
     {
        EINA_LOG_ERR("Could not register log domain: eina_simple_xml");
        return EINA_FALSE;
     }

#ifdef EINA_DEFAULT_MEMPOOL
   choice = "pass_through";
#else
   choice = "chained_mempool";
#endif
   tmp = getenv("EINA_MEMPOOL");
   if (tmp && tmp[0])
      choice = tmp;

   _eina_simple_xml_tag_mp = eina_mempool_add
         (choice, "simple_xml_tag", NULL,
          sizeof(Eina_Simple_XML_Node_Tag), 32);
   if (!_eina_simple_xml_tag_mp)
     {
        ERR("Mempool for simple_xml_tag cannot be allocated in init.");
        goto on_init_fail;
     }

   _eina_simple_xml_attribute_mp = eina_mempool_add
         (choice, "simple_xml_attribute", NULL,
          sizeof(Eina_Simple_XML_Attribute), 8);
   if (!_eina_simple_xml_attribute_mp)
     {
        ERR("Mempool for simple_xml_attribute cannot be allocated in init.");
        eina_mempool_del(_eina_simple_xml_tag_mp);
        goto on_init_fail;
     }

#define EMS(n) eina_magic_string_static_set(n, n ## _STR)
   EMS(EINA_MAGIC_SIMPLE_XML_TAG);
   EMS(EINA_MAGIC_SIMPLE_XML_DATA);
   EMS(EINA_MAGIC_SIMPLE_XML_ATTRIBUTE);
#undef EMS

   return EINA_TRUE;

on_init_fail:
   eina_log_domain_unregister(_eina_simple_xml_log_dom);
   _eina_simple_xml_log_dom = -1;
   return EINA_FALSE;
}

/**
 * @internal
 * @brief Shut down the simple xml parser module.
 *
 * @return #EINA_TRUE on success, #EINA_FALSE on failure.
 *
 * This function shuts down the simple xml parser module set
 * up by eina_simple_xml_init(). It is called by
 * eina_shutdown().
 *
 * @see eina_shutdown()
 */
Eina_Bool
eina_simple_xml_shutdown(void)
{
   eina_mempool_del(_eina_simple_xml_attribute_mp);
   eina_mempool_del(_eina_simple_xml_tag_mp);

   eina_log_domain_unregister(_eina_simple_xml_log_dom);
   _eina_simple_xml_log_dom = -1;
   return EINA_TRUE;
}


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/


EINA_API Eina_Bool
eina_simple_xml_parse(const char *buf, unsigned buflen, Eina_Bool strip, Eina_Simple_XML_Cb func, const void *data)
{
   const char *itr = buf, *itr_end = buf + buflen;

   if (!buf) return EINA_FALSE;
   if (!func) return EINA_FALSE;

#define CB(type, start, end)                                            \
   do                                                                   \
     {                                                                  \
        size_t _sz = end - start;                                       \
        Eina_Bool _ret;                                                 \
        _ret = func((void*)data, type, start, start - buf, _sz);        \
        if (!_ret) return EINA_FALSE;                                   \
     }                                                                  \
   while (0)

   while (itr < itr_end)
     {
        if (itr[0] == '<')
          {
             if (itr + 1 >= itr_end)
               {
                  CB(EINA_SIMPLE_XML_ERROR, itr, itr_end);
                  return EINA_FALSE;
               }
             else
               {
                  Eina_Simple_XML_Type type;
                  size_t toff;
                  const char *p;

                  if (itr[1] == '/')
                    {
                       type = EINA_SIMPLE_XML_CLOSE;
                       toff = 1;
                    }
                  else if (itr[1] == '?')
                    {
                       type = EINA_SIMPLE_XML_PROCESSING;
                       toff = 1;
                    }
                  else if (itr[1] == '!')
                    {
                       if ((itr + sizeof("<!DOCTYPE>") - 1 < itr_end) &&
                           (!memcmp(itr + 2, "DOCTYPE",
                                    sizeof("DOCTYPE") - 1)) &&
                           ((itr[2 + sizeof("DOCTYPE") - 1] == '>') ||
                            (isspace((unsigned char)itr[2 + sizeof("DOCTYPE") - 1]))))
                         {
                            type = EINA_SIMPLE_XML_DOCTYPE;
                            toff = sizeof("!DOCTYPE") - 1;
                         }
                       else if ((itr + sizeof("<!---->") - 1 < itr_end) &&
                                (!memcmp(itr + 2, "--", sizeof("--") - 1)))
                         {
                            type = EINA_SIMPLE_XML_COMMENT;
                            toff = sizeof("!--") - 1;
                         }
                       else if ((itr + sizeof("<![CDATA[]]>") - 1 < itr_end) &&
                                (!memcmp(itr + 2, "[CDATA[",
                                         sizeof("[CDATA[") - 1)))
                         {
                            type = EINA_SIMPLE_XML_CDATA;
                            toff = sizeof("![CDATA[") - 1;
                         }
                       else if (itr + sizeof("<!>") - 1 < itr_end)
                         {
                            type = EINA_SIMPLE_XML_DOCTYPE_CHILD;
                            toff = sizeof("!") - 1;
                         }
                       else
                         {
                            type = EINA_SIMPLE_XML_OPEN;
                            toff = 0;
                         }
                    }
                  else
                    {
                       type = EINA_SIMPLE_XML_OPEN;
                       toff = 0;
                    }

                  if (type == EINA_SIMPLE_XML_CDATA)
                    p = _eina_simple_xml_tag_cdata_end_find(itr + 1 + toff, itr_end);
                  else if (type == EINA_SIMPLE_XML_DOCTYPE_CHILD)
                    p = _eina_simple_xml_tag_doctype_child_end_find(itr + 1 + toff, itr_end);
                  else if (type == EINA_SIMPLE_XML_COMMENT)
                    p = _eina_simple_xml_tag_comment_end_find(itr + 1 + toff, itr_end);
                  else
                    p = _eina_simple_xml_tag_end_find(itr + 1 + toff, itr_end);

                  if ((p) && (*p == '<'))
                    {
                       type = EINA_SIMPLE_XML_ERROR;
                       toff = 0;
                    }

                  if (p)
                    {
                       const char *start, *end;

                       start = itr + 1 + toff;
                       end = p;

                       switch (type)
                         {
                          case EINA_SIMPLE_XML_OPEN:
                             if (p[-1] == '/')
                               {
                                  type = EINA_SIMPLE_XML_OPEN_EMPTY;
                                  end--;
                               }
                             break;
                          case EINA_SIMPLE_XML_CDATA:
                             if (!memcmp(p - 2, "]]", 2)) end -= 2;
                             break;
                          case EINA_SIMPLE_XML_PROCESSING:
                             if (p[-1] == '?') end--;
                             break;
                          case EINA_SIMPLE_XML_COMMENT:
                             if (!memcmp(p - 2, "--", 2)) end -= 2;
                             break;
                          case EINA_SIMPLE_XML_OPEN_EMPTY:
                          case EINA_SIMPLE_XML_CLOSE:
                          case EINA_SIMPLE_XML_DATA:
                          case EINA_SIMPLE_XML_ERROR:
                          case EINA_SIMPLE_XML_DOCTYPE:
                          case EINA_SIMPLE_XML_DOCTYPE_CHILD:
                          case EINA_SIMPLE_XML_IGNORED:
                             break;
                         }

                       if ((strip) && (type != EINA_SIMPLE_XML_ERROR) && (type != EINA_SIMPLE_XML_CDATA))
                         {
                            start = _eina_simple_xml_whitespace_skip
                              (start, end);
                            end = _eina_simple_xml_whitespace_unskip
                              (end, start + 1);
                         }

                       CB(type, start, end);

                       if (type != EINA_SIMPLE_XML_ERROR)
                         itr = p + 1;
                       else
                         itr = p;
                    }
                  else
                    {
                       CB(EINA_SIMPLE_XML_ERROR, itr, itr_end);
                       return EINA_FALSE;
                    }
               }
          }
        else
          {
             const char *p, *end;

             if (strip)
               {
                  p = _eina_simple_xml_whitespace_skip(itr, itr_end);
                  if (p)
                    {
                       CB(EINA_SIMPLE_XML_IGNORED, itr, p);
                       itr = p;
                    }
               }

             p = _eina_simple_xml_tag_start_find(itr, itr_end);
             if (!p) p = itr_end;

             end = p;
             if (strip)
               end = _eina_simple_xml_whitespace_unskip(end, itr);

             if (itr != end)
               CB(EINA_SIMPLE_XML_DATA, itr, end);

             if ((strip) && (end < p))
               CB(EINA_SIMPLE_XML_IGNORED, end, p);

             itr = p;
          }
     }

#undef CB

   return EINA_TRUE;
}

EINA_API const char *
eina_simple_xml_tag_attributes_find(const char *buf, unsigned buflen)
{
   const char *itr = buf, *itr_end = buf + buflen;

   for (; itr < itr_end; itr++)
     {
        if (!isspace((unsigned char)*itr))
          {
             /* user skip tagname and already gave it the attributes */
             if (*itr == '=')
               return buf;
          }
        else
          {
             itr = _eina_simple_xml_whitespace_skip(itr + 1, itr_end);
             if (itr == itr_end)
               return NULL;
             return itr;
          }
     }

   return NULL;
}

EINA_API Eina_Bool
eina_simple_xml_attributes_parse(const char *buf, unsigned buflen, Eina_Simple_XML_Attribute_Cb func, const void *data)
{
   const char *itr = buf, *itr_end = buf + buflen;
   char *tmpbuf = alloca(buflen + 1);

   if (!buf) return EINA_FALSE;
   if (!func) return EINA_FALSE;

   while (itr < itr_end)
     {
        const char *p = _eina_simple_xml_whitespace_skip(itr, itr_end);
        const char *key, *key_end, *value, *value_end;
        char *tval;

        if (p == itr_end) return EINA_TRUE;

        key = p;
        for (key_end = key; key_end < itr_end; key_end++)
          if ((*key_end == '=') || (isspace((unsigned char)*key_end))) break;
        if (key_end == itr_end) return EINA_FALSE;
        if (key_end == key) continue;

        if (*key_end == '=') value = key_end + 1;
        else
          {
             value = memchr(key_end, '=', itr_end - key_end);
             if (!value) return EINA_FALSE;
             value++;
          }
        for (; value < itr_end; value++)
          if (!isspace((unsigned char)*value)) break;
        if (value == itr_end) return EINA_FALSE;

        if ((*value == '"') || (*value == '\''))
          {
             value_end = memchr(value + 1, *value, itr_end - value);
             if (!value_end) return EINA_FALSE;
             value++;
          }
        else
          {
             value_end = _eina_simple_xml_whitespace_find(value, itr_end);
          }

        memcpy(tmpbuf, key, key_end - key);
        tmpbuf[key_end - key] = '\0';

        tval = tmpbuf + (key_end - key) + 1;
        memcpy(tval, value, value_end - value);
        tval[value_end - value] = '\0';

        if (!func((void*)data, tmpbuf, tval))
          return EINA_FALSE;

        itr = value_end + 1;
     }
   return EINA_TRUE;
}

EINA_API Eina_Bool
eina_simple_xml_attribute_w3c_parse(const char *buf, Eina_Simple_XML_Attribute_Cb func, const void *data)
{
   const char *end;
   char *key;
   char *val;
   char *next;

   if (!buf) return EINA_FALSE;

   end = buf + strlen(buf);
   key = alloca(end - buf + 1);
   val = alloca(end - buf + 1);

   if (buf == end) return EINA_TRUE;

   do
     {
        char *sep = strchr(buf, ':');
        next = strchr(buf, ';');

        key[0] = '\0';
        val[0] = '\0';

        if (next == NULL && sep != NULL)
          {
             memcpy(key, buf, sep - buf);
             key[sep - buf] = '\0';

             memcpy(val, sep + 1, end - sep - 1);
             val[end - sep - 1] = '\0';
          }
        else if (sep < next && sep != NULL)
          {
             memcpy(key, buf, sep - buf);
             key[sep - buf] = '\0';

             memcpy(val, sep + 1, next - sep - 1);
             val[next - sep - 1] = '\0';
          }
        else if (next)
          {
             memcpy(key, buf, next - buf);
             key[next - buf] = '\0';
          }

        if (key[0])
          if (!func((void*) data, key, val))
            return EINA_FALSE;

        buf = next + 1;
     }
   while (next != NULL);

   return EINA_TRUE;
}

/* Node loader *************************************************************/

EINA_API Eina_Simple_XML_Attribute *
eina_simple_xml_attribute_new(Eina_Simple_XML_Node_Tag *parent, const char *key, const char *value)
{
   Eina_Simple_XML_Attribute *attr;

   if (!key) return NULL;

   attr = eina_mempool_malloc(_eina_simple_xml_attribute_mp, sizeof(*attr));
   if (!attr)
     {
        ERR("could not allocate memory for attribute from mempool");
        return NULL;
     }

   EINA_MAGIC_SET(attr, EINA_MAGIC_SIMPLE_XML_ATTRIBUTE);
   attr->parent = parent;
   attr->key = eina_stringshare_add(key);
   attr->value = eina_stringshare_add(value ? value : ""); // Ensure value is not NULL

   if (parent)
     parent->attributes = eina_inlist_append
       (parent->attributes, EINA_INLIST_GET(attr));

   return attr;
}

EINA_API void
eina_simple_xml_attribute_free(Eina_Simple_XML_Attribute *attr)
{
   if (!attr)
     return;

   EINA_MAGIC_CHECK_ATTRIBUTE(attr);

   if (attr->parent)
     attr->parent->attributes = eina_inlist_remove
          (attr->parent->attributes, EINA_INLIST_GET(attr));

   eina_stringshare_del(attr->key);
   eina_stringshare_del(attr->value);
   EINA_MAGIC_SET(attr, EINA_MAGIC_NONE);
   eina_mempool_free(_eina_simple_xml_attribute_mp, attr);
}

/**
 * @internal
 * @brief Frees an Eina_Simple_XML_Node_Data and removes it from its parent.
 * @param node The data node to free.
 * This function is used internally to free various data-type nodes
 * (data, cdata, comment, etc.) by casting them to Eina_Simple_XML_Node_Data.
 */
static void
_eina_simple_xml_node_data_free(Eina_Simple_XML_Node_Data *node)
{
   if (node->base.parent)
     node->base.parent->children = eina_inlist_remove
          (node->base.parent->children, EINA_INLIST_GET(&node->base));

   EINA_MAGIC_SET(&node->base, EINA_MAGIC_NONE);
   free(node); // Data nodes are allocated with malloc, not mempool
}

EINA_API Eina_Simple_XML_Node_Tag *
eina_simple_xml_node_tag_new(Eina_Simple_XML_Node_Tag *parent, const char *name)
{
   Eina_Simple_XML_Node_Tag *n;

   if (!name) return NULL;

   n = eina_mempool_malloc(_eina_simple_xml_tag_mp, sizeof(*n));
   if (!n)
     {
        ERR("could not allocate memory for node from mempool");
        return NULL;
     }

   memset(n, 0, sizeof(*n));

   EINA_MAGIC_SET(&n->base, EINA_MAGIC_SIMPLE_XML_TAG);

   n->base.type = EINA_SIMPLE_XML_NODE_TAG;
   n->base.parent = parent;
   n->name = eina_stringshare_add(name);

   if (parent)
     parent->children = eina_inlist_append
       (parent->children, EINA_INLIST_GET(&n->base));

   return n;
}

/**
 * @internal
 * @brief Recursively frees an Eina_Simple_XML_Node_Tag and all its children and attributes.
 * @param tag The tag node to free.
 * This is the core recursive freeing function. It handles freeing child tags
 * by calling itself, and child data nodes by calling _eina_simple_xml_node_data_free.
 * It also frees all attributes associated with the tag.
 */
void
_eina_simple_xml_node_tag_free(Eina_Simple_XML_Node_Tag *tag)
{
   while (tag->children)
     {
        Eina_Simple_XML_Node *n = EINA_INLIST_CONTAINER_GET
          (tag->children, Eina_Simple_XML_Node);
        if (n->type == EINA_SIMPLE_XML_NODE_TAG)
          _eina_simple_xml_node_tag_free((Eina_Simple_XML_Node_Tag *)n);
        else
          _eina_simple_xml_node_data_free((Eina_Simple_XML_Node_Data *)n);
     }

   while (tag->attributes)
     {
        Eina_Simple_XML_Attribute *a = EINA_INLIST_CONTAINER_GET
          (tag->attributes, Eina_Simple_XML_Attribute);
        eina_simple_xml_attribute_free(a);
     }

   if (tag->base.parent)
     tag->base.parent->children = eina_inlist_remove
          (tag->base.parent->children, EINA_INLIST_GET(&tag->base));

   eina_stringshare_del(tag->name);
   EINA_MAGIC_SET(&tag->base, EINA_MAGIC_NONE);
   eina_mempool_free(_eina_simple_xml_tag_mp, tag);
}

EINA_API void
eina_simple_xml_node_tag_free(Eina_Simple_XML_Node_Tag *tag)
{
   if (!tag)
     return;

   EINA_MAGIC_CHECK_TAG(&(tag->base));
   if (tag->base.type != EINA_SIMPLE_XML_NODE_TAG)
     {
        ERR("expected tag node!");
        return;
     }
   _eina_simple_xml_node_tag_free(tag);
}

/**
 * @internal
 * @brief Creates a new generic data node (data, cdata, comment, etc.).
 * @param parent The parent tag node. Can be NULL.
 * @param type The type of the data node (EINA_SIMPLE_XML_NODE_DATA, EINA_SIMPLE_XML_NODE_CDATA, etc.).
 * @param content The content of the data node.
 * @param length The length of the content.
 * @return A newly allocated Eina_Simple_XML_Node_Data, or NULL on error.
 * The actual data is copied into the structure.
 */
static Eina_Simple_XML_Node_Data *
_eina_simple_xml_node_data_new(Eina_Simple_XML_Node_Tag *parent, Eina_Simple_XML_Node_Type type, const char *content, unsigned length)
{
   Eina_Simple_XML_Node_Data *n;

   if (!content) return NULL;

   // Allocate space for the struct and the flexible array member data[]
   n = malloc(sizeof(*n) + length + 1);

   if (!n)
     {
        ERR("could not allocate memory for node");
        return NULL;
     }

   EINA_MAGIC_SET(&n->base, EINA_MAGIC_SIMPLE_XML_DATA);
   n->base.type = type;
   n->base.parent = parent;

   n->length = length;
   memcpy(n->data, content, length);
   n->data[length] = '\0';

   if (parent)
     parent->children = eina_inlist_append
       (parent->children, EINA_INLIST_GET(&n->base));

   return n;
}

EINA_API Eina_Simple_XML_Node_Data *
eina_simple_xml_node_data_new(Eina_Simple_XML_Node_Tag *parent, const char *contents, size_t length)
{
   return _eina_simple_xml_node_data_new
     (parent, EINA_SIMPLE_XML_NODE_DATA, contents, length);
}

EINA_API void
eina_simple_xml_node_data_free(Eina_Simple_XML_Node_Data *node)
{
   if (!node)
     return;

   EINA_MAGIC_CHECK_DATA(&(node->base));
   if (node->base.type != EINA_SIMPLE_XML_NODE_DATA)
     {
        ERR("expected node of type: data!");
        return;
     }
   _eina_simple_xml_node_data_free(node);
}

EINA_API Eina_Simple_XML_Node_CData *
eina_simple_xml_node_cdata_new(Eina_Simple_XML_Node_Tag *parent, const char *contents, size_t length)
{
   return _eina_simple_xml_node_data_new
     (parent, EINA_SIMPLE_XML_NODE_CDATA, contents, length);
}

EINA_API void
eina_simple_xml_node_cdata_free(Eina_Simple_XML_Node_Data *node)
{
   if (!node)
     return;

   EINA_MAGIC_CHECK_DATA(&(node->base));
   if (node->base.type != EINA_SIMPLE_XML_NODE_CDATA)
     {
        ERR("expected node of type: cdata!");
        return;
     }
   _eina_simple_xml_node_data_free(node);
}

EINA_API Eina_Simple_XML_Node_Doctype_Child *
eina_simple_xml_node_doctype_child_new(Eina_Simple_XML_Node_Tag *parent, const char *contents, size_t length)
{
   return _eina_simple_xml_node_data_new
     (parent, EINA_SIMPLE_XML_NODE_DOCTYPE_CHILD, contents, length);
}

EINA_API void
eina_simple_xml_node_doctype_child_free(Eina_Simple_XML_Node_Data *node)
{
   if (!node)
     return;

   EINA_MAGIC_CHECK_DATA(&(node->base));
   if (node->base.type != EINA_SIMPLE_XML_NODE_DOCTYPE_CHILD)
     {
        ERR("expected node of type: doctype child!");
        return;
     }
   _eina_simple_xml_node_data_free(node);
}

EINA_API Eina_Simple_XML_Node_Processing *
eina_simple_xml_node_processing_new(Eina_Simple_XML_Node_Tag *parent, const char *contents, size_t length)
{
   return _eina_simple_xml_node_data_new
     (parent, EINA_SIMPLE_XML_NODE_PROCESSING, contents, length);
}

EINA_API void
eina_simple_xml_node_processing_free(Eina_Simple_XML_Node_Data *node)
{
   if (!node)
     return;

   EINA_MAGIC_CHECK_DATA(&(node->base));
   if (node->base.type != EINA_SIMPLE_XML_NODE_PROCESSING)
     {
        ERR("expected node of type: processing!");
        return;
     }
   _eina_simple_xml_node_data_free(node);
}

EINA_API Eina_Simple_XML_Node_Doctype *
eina_simple_xml_node_doctype_new(Eina_Simple_XML_Node_Tag *parent, const char *contents, size_t length)
{
   return _eina_simple_xml_node_data_new
     (parent, EINA_SIMPLE_XML_NODE_DOCTYPE, contents, length);
}

EINA_API void
eina_simple_xml_node_doctype_free(Eina_Simple_XML_Node_Data *node)
{
   if (!node)
     return;

   EINA_MAGIC_CHECK_DATA(&(node->base));
   if (node->base.type != EINA_SIMPLE_XML_NODE_DOCTYPE)
     {
        ERR("expected node of type: doctype!");
        return;
     }
   _eina_simple_xml_node_data_free(node);
}

EINA_API Eina_Simple_XML_Node_Comment *
eina_simple_xml_node_comment_new(Eina_Simple_XML_Node_Tag *parent, const char *contents, size_t length)
{
   return _eina_simple_xml_node_data_new
     (parent, EINA_SIMPLE_XML_NODE_COMMENT, contents, length);
}

EINA_API void
eina_simple_xml_node_comment_free(Eina_Simple_XML_Node_Data *node)
{
   if (!node)
     return;

   EINA_MAGIC_CHECK_DATA(&(node->base));
   if (node->base.type != EINA_SIMPLE_XML_NODE_COMMENT)
     {
        ERR("expected node of type: comment!");
        return;
     }
   _eina_simple_xml_node_data_free(node);
}

struct eina_simple_xml_node_load_ctxt
{
   Eina_Simple_XML_Node_Root *root;
   Eina_Simple_XML_Node_Tag *current;
};

static Eina_Bool
_eina_simple_xml_attrs_parse(void *data, const char *key, const char *value)
{
   Eina_Simple_XML_Node_Tag *n = data;
   Eina_Simple_XML_Attribute *attr;

   attr = eina_simple_xml_attribute_new(n, key, value);
   return !!attr;
}

static Eina_Bool
_eina_simple_xml_node_parse(void *data, Eina_Simple_XML_Type type, const char *content, unsigned offset, unsigned length)
{
   struct eina_simple_xml_node_load_ctxt *ctx = data;

   switch (type)
     {
      case EINA_SIMPLE_XML_OPEN: // <tag>
      case EINA_SIMPLE_XML_OPEN_EMPTY: // <tag/>
        {
           Eina_Simple_XML_Node_Tag *n;
           const char *name, *name_end, *attrs;

           // Find where attributes start, if any
           attrs = eina_simple_xml_tag_attributes_find(content, length);
           if (!attrs) // No attributes, tag name is the whole content
             name_end = content + length;
           else // Attributes found, tag name ends before them
             name_end = attrs;

           // Trim trailing whitespace from tag name
           name_end = _eina_simple_xml_whitespace_unskip(name_end, content);

           // Extract and stringshare the tag name
           name = eina_stringshare_add_length(content, name_end - content);
           n = eina_simple_xml_node_tag_new(ctx->current, name);
           eina_stringshare_del(name); // stringshare_add made a copy
           if (!n) return EINA_FALSE; // Allocation failed

           // Parse attributes if they exist
           if (attrs)
             eina_simple_xml_attributes_parse
               (attrs, length - (attrs - content), // Pass only the attribute part
                _eina_simple_xml_attrs_parse, n); // Callback to add attributes to node n

           // If it's an opening tag (not self-closing), it becomes the new current parent
           if (type == EINA_SIMPLE_XML_OPEN)
             ctx->current = n;
        }
        break;

      case EINA_SIMPLE_XML_CLOSE: // </tag>
         if (ctx->current->base.parent) // Check if we are not at the root
           {
              const char *end = _eina_simple_xml_whitespace_unskip
                (content + length, content); // Trim whitespace from closing tag name
              int len;
              len = end - content;
              // Check if the closing tag matches the current open tag name
              // or if it's an empty closing tag like </>
              if ((len == 0) /* </> closes the tag for us. */ ||
                  ((eina_stringshare_strlen(ctx->current->name) == (size_t)len) &&
                   (memcmp(ctx->current->name, content, len) == 0)))
                ctx->current = ctx->current->base.parent; // Move up to the parent tag
              else
                WRN("closed incorrect tag: '%.*s', '%s' was expected!",
                    len, content, ctx->current->name);
           }
         else
           WRN("closed tag '%.*s' but already at document root!",
               length, content);
         break;

      case EINA_SIMPLE_XML_DATA: // text data
         return !!eina_simple_xml_node_data_new
           (ctx->current, content, length);
      case EINA_SIMPLE_XML_CDATA: // <![CDATA[...]]>
         return !!eina_simple_xml_node_cdata_new
           (ctx->current, content, length);
      case EINA_SIMPLE_XML_PROCESSING: // <?...?>
         return !!eina_simple_xml_node_processing_new
           (ctx->current, content, length);
      case EINA_SIMPLE_XML_DOCTYPE: // <!DOCTYPE ...>
         return !!eina_simple_xml_node_doctype_new
           (ctx->current, content, length);
      case EINA_SIMPLE_XML_DOCTYPE_CHILD: // <!ELEMENT ...> or similar inside DOCTYPE
         return !!eina_simple_xml_node_doctype_child_new
           (ctx->current, content, length);
      case EINA_SIMPLE_XML_COMMENT: // <!-- ... -->
         return !!eina_simple_xml_node_comment_new
           (ctx->current, content, length);

      case EINA_SIMPLE_XML_ERROR: // Parser error
         ERR("parser error at offset %u-%u: %.*s",
             offset, length, length, content);
         break;
      case EINA_SIMPLE_XML_IGNORED:
         DBG("ignored contents at offset %u-%u: %.*s",
             offset, length, length, content);
         break;
     }

   return EINA_TRUE;
}

EINA_API Eina_Simple_XML_Node_Root *
eina_simple_xml_node_load(const char *buf, unsigned buflen, Eina_Bool strip)
{
   Eina_Simple_XML_Node_Root *root;
   struct eina_simple_xml_node_load_ctxt ctx;

   if (!buf) return NULL;

   root = eina_mempool_malloc(_eina_simple_xml_tag_mp, sizeof(*root));
   if (!root) return NULL;

   memset(root, 0, sizeof(*root));
   EINA_MAGIC_SET(&root->base, EINA_MAGIC_SIMPLE_XML_TAG); // Root is also a type of tag
   root->base.type = EINA_SIMPLE_XML_NODE_ROOT;

   ctx.root = root; // The overall root of the document being built
   ctx.current = root; // The current parent tag to which new nodes are added
   eina_simple_xml_parse(buf, buflen, strip, _eina_simple_xml_node_parse, &ctx);

   return root;
}

EINA_API void
eina_simple_xml_node_root_free(Eina_Simple_XML_Node_Root *root)
{
   if (!root) return;
   EINA_MAGIC_CHECK_TAG(&(root->base));
   if (root->base.type != EINA_SIMPLE_XML_NODE_ROOT)
     {
        ERR("expected root node!");
        return;
     }
   _eina_simple_xml_node_tag_free(root);
}

/**
 * @internal
 * @brief Appends indentation string to the buffer based on the current level.
 * @param buf The string buffer to append to.
 * @param indent The string to use for one level of indentation (e.g., "  " or "\t").
 * @param level The current indentation level.
 */
static inline void
_eina_simple_xml_node_dump_indent(Eina_Strbuf *buf, const char *indent, unsigned level)
{
   unsigned i, indent_len = strlen(indent);
   for (i = 0; i < level; i++)
     eina_strbuf_append_length(buf, indent, indent_len);
}

/**
 * @internal
 * @brief Appends all attributes of a tag to the string buffer.
 * @param buf The string buffer to append to.
 * @param tag The tag whose attributes are to be appended.
 * Attributes are appended in the format: ` key="value"`.
 */
static void
_eina_simple_xml_node_tag_attributes_append(Eina_Strbuf *buf, Eina_Simple_XML_Node_Tag *tag)
{
   Eina_Simple_XML_Attribute *a;

   EINA_INLIST_FOREACH(tag->attributes, a)
     eina_strbuf_append_printf(buf, " %s=\"%s\"", a->key, a->value);
}

// Forward declaration for the recursive dump function
static void _eina_simple_xml_node_dump_recursive(Eina_Strbuf *buf, Eina_Simple_XML_Node *node, const char *indent, unsigned level);

/**
 * @internal
 * @brief Dumps all children of a tag node to the string buffer.
 * @param buf The string buffer to append to.
 * @param tag The parent tag node.
 * @param indent The indentation string.
 * @param level The current indentation level for children.
 */
static void
_eina_simple_xml_node_children_dump(Eina_Strbuf *buf, Eina_Simple_XML_Node_Tag *tag, const char *indent, unsigned level)
{
   Eina_Simple_XML_Node *node;

   EINA_INLIST_FOREACH(tag->children, node)
     _eina_simple_xml_node_dump_recursive(buf, node, indent, level);
}

/**
 * @internal
 * @brief Recursively dumps an XML node and its children to a string buffer.
 * @param buf The string buffer to append to.
 * @param node The XML node to dump.
 * @param indent The string to use for indentation (e.g., "  "). If NULL, no indentation or newlines are added.
 * @param level The current indentation level.
 */
static void
_eina_simple_xml_node_dump_recursive(Eina_Strbuf *buf, Eina_Simple_XML_Node *node, const char *indent, unsigned level)
{
   switch (node->type)
     {
      case EINA_SIMPLE_XML_NODE_ROOT: // The document root itself is not dumped, only its children
         _eina_simple_xml_node_children_dump
           (buf, (Eina_Simple_XML_Node_Tag *)node, indent, level); // Start with level 0 for children of root
         break;

      case EINA_SIMPLE_XML_NODE_TAG: // <tag attr="val">children</tag> or <tag attr="val"/>
        {
           Eina_Simple_XML_Node_Tag *n = (Eina_Simple_XML_Node_Tag *)node;

           if (indent) _eina_simple_xml_node_dump_indent(buf, indent, level);

           eina_strbuf_append_char(buf, '<');
           eina_strbuf_append_length
               (buf, n->name, eina_stringshare_strlen(n->name));

           if (n->attributes)
             _eina_simple_xml_node_tag_attributes_append(buf, n);

           if (n->children)
             eina_strbuf_append_char(buf, '>');
           else
             eina_strbuf_append_length(buf, "/>", sizeof("/>") - 1);

           if (indent) eina_strbuf_append_char(buf, '\n');

           if (n->children)
             {
                _eina_simple_xml_node_children_dump(buf, n, indent, level + 1);

                if (indent)
                  _eina_simple_xml_node_dump_indent(buf, indent, level);

                eina_strbuf_append_length(buf, "</", sizeof("</") - 1);
                eina_strbuf_append_length
                    (buf, n->name, eina_stringshare_strlen(n->name));
                eina_strbuf_append_char(buf, '>');

                if (indent) eina_strbuf_append_char(buf, '\n');
             }
        }
        break;
      case EINA_SIMPLE_XML_NODE_DATA:
        {
           Eina_Simple_XML_Node_Data *n = (Eina_Simple_XML_Node_Data *)node;

           if (indent) _eina_simple_xml_node_dump_indent(buf, indent, level);
           eina_strbuf_append_length(buf, n->data, n->length);
           if (indent) eina_strbuf_append_char(buf, '\n');
        }
        break;

      case EINA_SIMPLE_XML_NODE_CDATA:
        {
           Eina_Simple_XML_Node_Data *n = (Eina_Simple_XML_Node_Data *)node;

           if (indent) _eina_simple_xml_node_dump_indent(buf, indent, level);
           eina_strbuf_append_length(buf, "<![CDATA[", sizeof("<![CDATA[") - 1);
           eina_strbuf_append_length(buf, n->data, n->length);
           eina_strbuf_append_length(buf, "]]>", sizeof("]]>") - 1);
           if (indent) eina_strbuf_append_char(buf, '\n');
        }
        break;

      case EINA_SIMPLE_XML_NODE_PROCESSING:
        {
           Eina_Simple_XML_Node_Data *n = (Eina_Simple_XML_Node_Data *)node;

           if (indent) _eina_simple_xml_node_dump_indent(buf, indent, level);
           eina_strbuf_append_length(buf, "<?", sizeof("<?") - 1);
           eina_strbuf_append_length(buf, n->data, n->length);
           eina_strbuf_append_length(buf, " ?>", sizeof(" ?>") - 1);
           if (indent) eina_strbuf_append_char(buf, '\n');
        }
        break;

      case EINA_SIMPLE_XML_NODE_DOCTYPE:
        {
           Eina_Simple_XML_Node_Data *n = (Eina_Simple_XML_Node_Data *)node;

           if (indent) _eina_simple_xml_node_dump_indent(buf, indent, level);
           eina_strbuf_append_length
             (buf, "<!DOCTYPE ", sizeof("<!DOCTYPE ") - 1);
           eina_strbuf_append_length(buf, n->data, n->length);
           eina_strbuf_append_char(buf, '>');
           if (indent) eina_strbuf_append_char(buf, '\n');
        }
        break;

      case EINA_SIMPLE_XML_NODE_DOCTYPE_CHILD:
        {
           Eina_Simple_XML_Node_Data *n = (Eina_Simple_XML_Node_Data *)node;

           if (indent) _eina_simple_xml_node_dump_indent(buf, indent, level);
           eina_strbuf_append_length(buf, "<!", sizeof("<!") - 1);
           eina_strbuf_append_length(buf, n->data, n->length);
           eina_strbuf_append_length(buf, ">", sizeof(">") - 1);
           if (indent) eina_strbuf_append_char(buf, '\n');
        }
        break;

      case EINA_SIMPLE_XML_NODE_COMMENT:
        {
           Eina_Simple_XML_Node_Data *n = (Eina_Simple_XML_Node_Data *)node;

           if (indent) _eina_simple_xml_node_dump_indent(buf, indent, level);
           eina_strbuf_append_length(buf, "<!-- ", sizeof("<!-- ") - 1);
           eina_strbuf_append_length(buf, n->data, n->length);
           eina_strbuf_append_length(buf, " -->", sizeof(" -->") - 1);
           if (indent) eina_strbuf_append_char(buf, '\n');
        }
        break;
     }
}

EINA_API char *
eina_simple_xml_node_dump(Eina_Simple_XML_Node *node, const char *indent)
{
   Eina_Strbuf *buf;
   char *ret;

   if (!node) return NULL;

   buf = eina_strbuf_new();
   if (!buf) return NULL;

   _eina_simple_xml_node_dump_recursive(buf, node, indent, 0);

   ret = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return ret;
}
