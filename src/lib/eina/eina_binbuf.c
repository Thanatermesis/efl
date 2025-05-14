#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "eina_private.h"
#include "eina_strbuf_common.h"
#include "eina_binbuf.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/*
 * These macros are used to configure the generic string buffer implementation
 * located in "eina_binbuf_template_c.x" for the Eina_Binbuf type, which
 * handles binary data (unsigned char). They define type-specific details
 * like the data type itself, its size, the structure name, and magic numbers
 * for runtime checking, allowing the template to be compiled for Eina_Binbuf.
 */

#ifdef _STRBUF_DATA_TYPE
# undef _STRBUF_DATA_TYPE
#endif

#ifdef _STRBUF_CSIZE
# undef _STRBUF_CSIZE
#endif

#ifdef _STRBUF_STRUCT_NAME
# undef _STRBUF_STRUCT_NAME
#endif

#ifdef _STRBUF_MAGIC
# undef _STRBUF_MAGIC
#endif

#ifdef _STRBUF_MAGIC_STR
# undef _STRBUF_MAGIC_STR
#endif

#ifdef _FUNC_EXPAND
# undef _FUNC_EXPAND
#endif


#define _STRBUF_DATA_TYPE         unsigned char
#define _STRBUF_CSIZE             sizeof(_STRBUF_DATA_TYPE)
#define _STRBUF_STRUCT_NAME       Eina_Binbuf
#define _STRBUF_MAGIC             EINA_MAGIC_BINBUF
#define _STRBUF_MAGIC_STR         __BINBUF_MAGIC_STR
static const char __BINBUF_MAGIC_STR[] = "Eina Binbuf";

#define _FUNC_EXPAND(y) eina_binbuf_ ## y

#include "eina_binbuf_template_c.x"

EINA_API Eina_Binbuf *
eina_binbuf_manage_new(const unsigned char *str, size_t length, Eina_Bool ro)
{
   Eina_Binbuf *buf;

   if (ro)
     buf = eina_strbuf_common_manage_ro_new(_STRBUF_CSIZE, (void *) str, length);
   else
     buf = eina_strbuf_common_manage_new(_STRBUF_CSIZE, (void *) str, length);
   EINA_MAGIC_SET(buf, _STRBUF_MAGIC);
   return buf;
}

/**
 * @endcond
 */
