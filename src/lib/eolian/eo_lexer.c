#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>
#include <locale.h>

#include <setjmp.h>
#include <assert.h>

#include "eo_lexer.h"
#include "eolian_priv.h"

/**
 * @internal
 * @brief Stores the number of remaining bytes for a multi-byte UTF-8 character.
 * This is used by next_char() to correctly advance over UTF-8 sequences.
 */
static int lastbytes = 0;

/**
 * @internal
 * @brief Advances the lexer to the next character in the input stream.
 *
 * This function handles moving past single-byte ASCII characters as well as
 * multi-byte UTF-8 characters. It updates the current character in `ls->current`,
 * and adjusts the character-aware column number `ls->icolumn`. For single-byte
 * characters, it also updates the token-aware `ls->column`. The `lastbytes`
 * static variable is used to track progress through a multi-byte sequence.
 *
 * @param ls The lexer instance.
 */
static void
next_char(Eo_Lexer *ls)
{
   int nb;
   Eina_Bool end = EINA_FALSE;

   if (ls->stream == ls->stream_end)
     {
        end = EINA_TRUE;
        ls->current = '\0';
     }
   else
     ls->current = *(ls->stream++);

   nb = lastbytes;
   if (!nb && end) nb = 1;
   if (!nb) eina_unicode_utf8_next_get(ls->stream - 1, &nb);

   if (nb == 1)
     {
        nb = 0;
        ++ls->icolumn;
        ls->column = ls->icolumn;
     }
   else --nb;

   lastbytes = nb;
}

#define KW(x) #x
#define KWAT(x) "@" #x
#define KWH(x) "#" #x

static const char * const tokens[] =
{
   "==", "!=", ">=", "<=", "&&", "||", "<<", ">>",
   "<doc>", "<string>", "<char>", "<number>", "<value>"
};

static const char * const keywords[] = { KEYWORDS };

static const char * const ctypes[] =
{
   "signed char", "unsigned char", "char", "short", "unsigned short", "int",
   "unsigned int", "long", "unsigned long", "long long", "unsigned long long",

   "int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t",
   "int64_t", "uint64_t", "int128_t", "uint128_t",

   "size_t", "ssize_t", "intptr_t", "uintptr_t", "ptrdiff_t",

   "time_t",

   "float", "double",

   "Eina_Bool",

   "Eina_Slice", "Eina_Rw_Slice",

   "void",

   "Eina_Accessor *", "Eina_Array *", "Eina_Future *", "Eina_Iterator *",
   "Eina_List *",
   "Eina_Value", "Eina_Value *", "Eina_Binbuf *", "Efl_Event *",
   "char *", "const char *", "Eina_Stringshare *", "Eina_Strbuf *",

   "Eina_Hash *",
   "void *",

   "function",
};

#undef KW
#undef KWAT
#undef KWH

#define is_newline(c) ((c) == '\n' || (c) == '\r')

/**
 * @internal
 * @brief Global hash map for fast keyword lookup. Maps string keywords to their enum values.
 */
static Eina_Hash *keyword_map = NULL;

/**
 * @internal
 * @brief Reports a fatal parsing error and aborts the process.
 *
 * This function constructs a detailed error message, including the line of
 * code where the error occurred and a caret pointing to the specific column.
 * The formatted error is logged to the Eolian state, and then `longjmp` is
 * used to unwind the stack and return control to the error handler set up
 * in `eo_lexer_new`.
 *
 * @param ls The lexer instance.
 * @param fmt The printf-style format string for the error message.
 * @param ... Arguments for the format string.
 */
static void
throw(Eo_Lexer *ls, const char *fmt, ...)
{
   const char *ln = ls->stream_line, *end = ls->stream_end;
   Eina_Strbuf *buf = eina_strbuf_new();
   int i;
   va_list ap;
   va_start(ap, fmt);
   eina_strbuf_append_vprintf(buf, fmt, ap);
   va_end(ap);
   eina_strbuf_append(buf, "\n ");
   while (ln != end && !is_newline(*ln))
     eina_strbuf_append_char(buf,*(ln++));
   eina_strbuf_append_char(buf, '\n');
   for (i = 0; i < ls->column; ++i)
     eina_strbuf_append_char(buf, ' ');
   eina_strbuf_append(buf, "^\n");
   Eolian_Object tmp;
   memset(&tmp, 0, sizeof(Eolian_Object));
   tmp.unit = ls->unit;
   tmp.file = ls->source;
   tmp.line = ls->line_number;
   tmp.column = ls->column;
   eolian_state_log_obj(ls->state, &tmp, "%s", eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   longjmp(ls->err_jmp, EO_LEXER_ERROR_NORMAL);
}

void
eo_lexer_init(void)
{
   unsigned int i;
   if (keyword_map) return;
   keyword_map = eina_hash_string_superfast_new(NULL);
   for (i = 0; i < (sizeof(keywords) / sizeof(keywords[0])); ++i)
     eina_hash_add(keyword_map, keywords[i], (void *)(size_t)(i + 1));
}

void
eo_lexer_shutdown(void)
{
   if (keyword_map)
     {
        eina_hash_free(keyword_map);
        keyword_map = NULL;
     }
}

/**
 * @internal
 * @brief Gets the string representation of a given token.
 *
 * For most tokens, this function defers to eo_lexer_token_to_str(). However,
 * for a `TOK_VALUE` token (an identifier or keyword), it copies the actual
 * string value from the token itself into the provided buffer.
 *
 * @param ls The lexer instance.
 * @param token The token identifier.
 * @param[out] buf The buffer to store the string representation.
 */
static void
txt_token(Eo_Lexer *ls, int token, char *buf)
{
   if (token == TOK_VALUE)
     memcpy(buf, ls->t.value.s, strlen(ls->t.value.s) + 1);
   else
     return eo_lexer_token_to_str(token, buf);
}

void eo_lexer_lex_error   (Eo_Lexer *ls, const char *msg, int token);
void eo_lexer_syntax_error(Eo_Lexer *ls, const char *msg);

/**
 * @internal
 * @brief Processes a newline character and updates the lexer's line and column state.
 *
 * This function handles both LF (`\n`) and CRLF (`\r\n`) line endings. It
 * increments the internal and token-aware line numbers and resets the column
 * counters. It also updates `ls->stream_line` to point to the beginning of the
 * new line for error reporting.
 *
 * @param ls The lexer instance.
 */
static void next_line(Eo_Lexer *ls)
{
   int old = ls->current;
   assert(is_newline(ls->current));
   ls->stream_line = ls->stream;
   next_char(ls);
   if (is_newline(ls->current) && ls->current != old)
     {
       next_char(ls);
       ls->stream_line = ls->stream;
     }
   if (++ls->iline_number >= INT_MAX)
     eo_lexer_syntax_error(ls, "chunk has too many lines");
   ls->line_number = ls->iline_number;
   ls->icolumn = ls->column = 0;
}

/**
 * @internal
 * @brief Skips whitespace characters (excluding newlines) on the current line.
 * @param ls The lexer instance.
 */
static void skip_ws(Eo_Lexer *ls)
{
   while (isspace(ls->current) && !is_newline(ls->current))
     next_char(ls);
}

/**
 * @internal
 * @brief Advances to the next line and skips any leading whitespace.
 * A convenience function combining next_line() and skip_ws().
 * @param ls The lexer instance.
 */
static void next_line_ws(Eo_Lexer *ls)
{
   next_line(ls);
   skip_ws(ls);
}

/**
 * @internal
 * @brief Helper for parsing aligned multi-line comments.
 *
 * In documentation blocks, it's common to align stars on each line, like:
 * @code
 * /*
 *  * Some text.
 *  * More text.
 *  *\/
 * @endcode
 * This function checks if the current character is a `*` at the expected
 * indentation level (`ccol`). If so, it consumes the star and any subsequent
 * whitespace. It also detects the end of a comment (`*` followed by `/`).
 *
 * @param ls The lexer instance.
 * @param ccol The column where a leading `*` is expected.
 * @param[out] term Set to EINA_TRUE if the comment termination sequence `*`\/ is found.
 * @return EINA_TRUE if a star was found and skipped, EINA_FALSE otherwise.
 */
static Eina_Bool
should_skip_star(Eo_Lexer *ls, int ccol, Eina_Bool *term)
{
   Eina_Bool had_star = EINA_FALSE;
   if (ls->column == ccol && ls->current == '*')
     {
        had_star = EINA_TRUE;
        next_char(ls);
        if (ls->current == '/')
          {
             next_char(ls);
             *term = EINA_TRUE;
             return EINA_FALSE;
          }
        skip_ws(ls);
     }
   return had_star;
}

/**
 * @internal
 * @brief Reads a multi-line C-style comment (`/* ... *` /`).
 *
 * This function parses the content of a long comment, handling newlines and
 * optionally stripping leading aligned asterisks on each line (using
 * should_skip_star()). The comment content is stored in the lexer's
 * shared buffer `ls->buff`. It stops upon reaching the `*` / terminator.
 *
 * @param ls The lexer instance.
 * @param ccol The starting column of the comment, used for aligning asterisks.
 */
static void
read_long_comment(Eo_Lexer *ls, int ccol)
{
   Eina_Bool had_star = EINA_FALSE, had_nl = EINA_FALSE;
   eina_strbuf_reset(ls->buff);

   if (is_newline(ls->current))
     {
        Eina_Bool term = EINA_FALSE;
        had_nl = EINA_TRUE;
        next_line_ws(ls);
        had_star = should_skip_star(ls, ccol, &term);
        if (term) goto cend;
     }

   for (;;)
     {
        if (!ls->current)
          eo_lexer_lex_error(ls, "unfinished long comment", -1);
        if (ls->current == '*')
          {
             next_char(ls);
             if (ls->current == '/')
               {
                  next_char(ls);
                  break;
               }
             eina_strbuf_append_char(ls->buff, '*');
          }
        else if (is_newline(ls->current))
          {
             eina_strbuf_append_char(ls->buff, '\n');
             next_line_ws(ls);
             if (!had_nl)
               {
                  Eina_Bool term = EINA_FALSE;
                  had_nl = EINA_TRUE;
                  had_star = should_skip_star(ls, ccol, &term);
                  if (term) break;
               }
             else if (had_star && ls->column == ccol && ls->current == '*')
               {
                  next_char(ls);
                  if (ls->current == '/')
                    {
                       next_char(ls);
                       break;
                    }
                  skip_ws(ls);
                }
          }
        else
          {
             eina_strbuf_append_char(ls->buff, ls->current);
             next_char(ls);
          }
     }
cend:
   eina_strbuf_trim(ls->buff);
}

enum Doc_Tokens {
    DOC_MANGLED = -2, DOC_UNFINISHED = -1, DOC_TEXT = 0, DOC_SINCE = 1
};

/**
 * @internal
 * @brief Processes a class name referenced within a documentation comment.
 *
 * When a documentation comment refers to a class (e.g., `My.Foo`), this
 * function is called. It normalizes the class name to its corresponding
 * filename (e.g., `my_foo.eo`) and, if that file exists, it queues it for
 * deferred parsing. This ensures that all referenced types are available
 * later without creating circular dependencies during the initial parse.
 *
 * @param ls The lexer instance.
 * @param cname The class name as it appears in the documentation.
 */
static void
doc_ref_class(Eo_Lexer *ls, const char *cname)
{
   size_t clen = strlen(cname);
   char *buf = alloca(clen + 4);
   memcpy(buf, cname, clen);
   buf[clen] = '\0';
   for (char *p = buf; *p; ++p)
     {
        if (*p == '.')
          *p = '_';
        else
          *p = tolower(*p);
     }
   memcpy(buf + clen, ".eo", sizeof(".eo"));
   if (!eina_hash_find(ls->state->filenames_eo, buf))
     return;
   /* ref'd classes do not become dependencies */
   database_defer(ls->state, buf, EINA_FALSE);
}

/**
 * @internal
 * @brief Parses a reference tag (like `@ref`) within a documentation block.
 *
 * This function is triggered when an `@` is encountered. It parses what
 * follows as a potential reference to another declaration (e.g., a class,
 * method, or property). It calls doc_ref_class() on parts of the name to
 * ensure dependencies are registered. It also stores debug information about
 * the location of the reference.
 *
 * @param ls The lexer instance.
 * @param doc The documentation object being built, to which reference debug
 *            info will be added.
 */
static void
doc_ref(Eo_Lexer *ls, Eolian_Documentation *doc)
{
   const char *st = ls->stream, *ste = ls->stream_end;
   size_t rlen = 0;
   while ((st != ste) && ((*st == '.') || (*st == '_') || isalnum(*st)))
     {
        ++st;
        ++rlen;
     }
   if ((rlen > 1) && (*(st - 1) == '.'))
     --rlen;
   if (!rlen)
     return;
   if (*ls->stream == '.')
     return;

   char *buf = alloca(rlen + 1);
   memcpy(buf, ls->stream, rlen);
   buf[rlen] = '\0';

   /* actual full class name */
   doc_ref_class(ls, buf);

   /* it's definitely a reference, add debug info
    * 20 bits for line and 12 bits for column, good enough
    */
   doc->ref_dbg = eina_list_append(doc->ref_dbg,
     (void *)(size_t)((ls->line_number & 0xFFFFF) | (((ls->column + 1) & 0xFFF) << 20)));

   /* method name at the end */
   char *end = strrchr(buf, '.');
   if (!end)
     return;
   *end = '\0';
   doc_ref_class(ls, buf);

   /* .get or .set at the end, handle possible property */
   if (strcmp(end + 1, "get") && strcmp(end + 1, "set"))
     return;
   end = strrchr(buf, '.');
   if (!end)
     return;
   *end = '\0';
   doc_ref_class(ls, buf);
}

/**
 * @internal
 * @brief The core lexer for the content inside a documentation block (`[[...]]`).
 *
 * This function processes the text within a doc comment. It handles:
 * - Paragraph breaks (two or more newlines).
 * - Escaped terminators (`\\]]`).
 * - `@since` tags.
 * - `@ref`-style references to other code elements.
 * - The final `]]` terminator.
 *
 * It returns special tokens from the `Doc_Tokens` enum to guide the calling
 * function (`read_doc`).
 *
 * @param ls The lexer instance.
 * @param doc The documentation object being built.
 * @param[out] term Set to EINA_TRUE when the `]]` terminator is found.
 * @param[out] since Set to EINA_TRUE when an `@since` tag is found.
 * @return A `Doc_Tokens` value indicating what was parsed (e.g., DOC_TEXT).
 */
static int
doc_lex(Eo_Lexer *ls, Eolian_Documentation *doc, Eina_Bool *term, Eina_Bool *since)
{
   int tokret = -1;
   eina_strbuf_reset(ls->buff);
   *since = EINA_FALSE;
   for (;;) switch (ls->current)
     {
      /* error case */
      case '\0':
        return DOC_UNFINISHED;
      /* newline case: if two or more newlines are present, new paragraph
       * if only one newline is present, append space to the text buffer
       * when starting new paragraph, reset doc continutation
       */
      case '\n':
      case '\r':
        next_line(ls);
        skip_ws(ls);
        if (!is_newline(ls->current))
          {
             eina_strbuf_append_char(ls->buff, ' ');
             continue;
          }
        while (is_newline(ls->current))
          next_line_ws(ls);
        tokret = DOC_TEXT;
        goto exit_with_token;
      /* escape case: for any \X, output \X
       * except for \\]], then output just ]]
       */
      case '\\':
        next_char(ls);
        if (ls->current == ']')
          {
             next_char(ls);
             if (ls->current == ']')
               {
                  next_char(ls);
                  eina_strbuf_append(ls->buff, "]]");
               }
             else
               eina_strbuf_append(ls->buff, "\\]");
          }
        else
          eina_strbuf_append_char(ls->buff, '\\');
        continue;
      /* terminating case */
      case ']':
        next_char(ls);
        if (ls->current == ']')
          {
             /* terminate doc */
             tokret = DOC_TEXT;
             goto terminated;
          }
        eina_strbuf_append_char(ls->buff, ']');
        continue;
      /* references and @since */
      case '@':
        if ((size_t)(ls->stream_end - ls->stream) >= (sizeof("since")) &&
            !memcmp(ls->stream, "since ", sizeof("since")))
          {
             next_char(ls);
             *since = EINA_TRUE;
             for (size_t i = 0; i < sizeof("since"); ++i)
               next_char(ls);
             skip_ws(ls);
             tokret = DOC_TEXT;
             goto exit_with_token;
          }
        doc_ref(ls, doc);
        eina_strbuf_append_char(ls->buff, '@');
        next_char(ls);
        /* in-class references */
        if (ls->klass && ls->current == '.')
          {
             next_char(ls);
             if (isalpha(ls->current) || ls->current == '_')
               eina_strbuf_append(ls->buff, ls->klass->base.name);
             eina_strbuf_append_char(ls->buff, '.');
          }
        continue;
      /* default case - append character */
      default:
        eina_strbuf_append_char(ls->buff, ls->current);
        next_char(ls);
        continue;
     }
terminated:
   next_char(ls);
   *term = EINA_TRUE;
exit_with_token:
   eina_strbuf_trim(ls->buff);
   return tokret;
}

/**
 * @internal
 * @brief Parses the version string following an `@since` tag in documentation.
 *
 * After `doc_lex` identifies an `@since` tag, this function is called to read
 * the version identifier that follows (e.g., "1.2.0"). It performs basic
 * validation and expects the documentation block to terminate immediately after.
 *
 * @param ls The lexer instance.
 * @return `DOC_SINCE` on success, or `DOC_MANGLED`/`DOC_UNFINISHED` on error.
 */
static int
read_since(Eo_Lexer *ls)
{
   eina_strbuf_reset(ls->buff);
   while (ls->current && (ls->current == '.' ||
                          ls->current == '_' ||
                          isalnum(ls->current)))
     {
        eina_strbuf_append_char(ls->buff, ls->current);
        next_char(ls);
     }
   if (!eina_strbuf_length_get(ls->buff))
     return DOC_UNFINISHED;
   skip_ws(ls);
   while (is_newline(ls->current))
     next_line_ws(ls);
   if (ls->current != ']')
     return DOC_MANGLED;
   next_char(ls);
   if (ls->current != ']')
     return DOC_MANGLED;
   next_char(ls);
   return DOC_SINCE;
}

/**
 * @internal
 * @brief Handles a fatal error during documentation parsing.
 *
 * This function is a specialized error handler. It cleans up any partially
 * allocated documentation structures (`doc`, `buf`) before calling the
 * main lexer error function (`eo_lexer_lex_error`), which will then `longjmp`.
 *
 * @param ls The lexer instance.
 * @param msg The error message.
 * @param doc The partially built documentation object to free.
 * @param buf An auxiliary string buffer to free.
 */
void doc_error(Eo_Lexer *ls, const char *msg, Eolian_Documentation *doc, Eina_Strbuf *buf)
{
   eina_stringshare_del(doc->summary);
   eina_stringshare_del(doc->description);
   eina_list_free(doc->ref_dbg);
   free(doc);
   eina_strbuf_free(buf);
   eo_lexer_lex_error(ls, msg, -1);
}

/**
 * @internal
 * @brief Parses an entire documentation block (`[[...]]`) into an Eolian_Documentation object.
 *
 * This function is called when the `[[` token is encountered. It orchestrates
 * the documentation parsing process by repeatedly calling `doc_lex` and
 * `read_since`. It distinguishes between the summary (the first paragraph) and
 * the full description (subsequent paragraphs) and populates the
 * `Eolian_Documentation` structure, which is then attached to the `TOK_DOC` token.
 *
 * @param ls The lexer instance.
 * @param[out] tok The token to which the parsed documentation object will be attached.
 * @param line The starting line number of the documentation block.
 * @param column The starting column number of the documentation block.
 */
static void
read_doc(Eo_Lexer *ls, Eo_Token *tok, int line, int column)
{
   Eolian_Documentation *doc = calloc(1, sizeof(Eolian_Documentation));
   if (!doc)
     longjmp(ls->err_jmp, EO_LEXER_ERROR_OOM);

   doc->base.unit = ls->unit;
   doc->base.file = ls->filename;
   doc->base.line = line;
   doc->base.column = column;
   doc->base.type = EOLIAN_OBJECT_DOCUMENTATION;

   Eina_Strbuf *rbuf = eina_strbuf_new();

   Eina_Bool term = EINA_FALSE, since = EINA_FALSE;
   while (!term)
     {
        int read;
        if (since)
          {
             read = read_since(ls);
             term = EINA_TRUE;
          }
        else
          read = doc_lex(ls, doc, &term, &since);
        switch (read)
          {
           case DOC_MANGLED:
             doc_error(ls, "mangled documentation", doc, rbuf);
             return;
           case DOC_UNFINISHED:
             doc_error(ls, "unfinished documentation", doc, rbuf);
             return;
           case DOC_TEXT:
             if (!eina_strbuf_length_get(ls->buff))
               continue;
             if (!doc->summary)
               doc->summary = eina_stringshare_add(eina_strbuf_string_get(ls->buff));
             else
               {
                  if (eina_strbuf_length_get(rbuf))
                    eina_strbuf_append(rbuf, "\n\n");
                  eina_strbuf_append(rbuf, eina_strbuf_string_get(ls->buff));
               }
             break;
           case DOC_SINCE:
             doc->since = eina_stringshare_add(eina_strbuf_string_get(ls->buff));
             break;
          }
     }

   if (eina_strbuf_length_get(rbuf))
     doc->description = eina_stringshare_add(eina_strbuf_string_get(rbuf));
   if (!doc->since && ls->klass && ls->klass->doc)
     doc->since = eina_stringshare_ref(ls->klass->doc->since);
   eina_strbuf_free(rbuf);
   tok->value.doc = doc;
}

/**
 * @internal
 * @brief Reports an error related to an invalid escape sequence.
 *
 * This is a helper function to format and report errors encountered while
 * parsing escape sequences in strings or character literals. It constructs a
 * message showing the invalid sequence before calling `eo_lexer_lex_error`.
 *
 * @param ls The lexer instance.
 * @param c An array of characters forming the invalid sequence.
 * @param n The number of characters in `c`.
 * @param msg The error description (e.g., "hexadecimal digit expected").
 */
static void
esc_error(Eo_Lexer *ls, int *c, int n, const char *msg)
{
   int i;
   eina_strbuf_reset(ls->buff);
   eina_strbuf_append_char(ls->buff, '\\');
   for (i = 0; i < n && c[i]; ++i)
     eina_strbuf_append_char(ls->buff, c[i]);
   eo_lexer_lex_error(ls, msg, TOK_STRING);
}

/**
 * @internal
 * @brief Converts a hexadecimal character to its integer value.
 * @param c The character to convert (e.g., 'a', 'F', '7').
 * @return The integer value (0-15).
 */
static int
hex_val(int c)
{
   if (c >= 'a') return c - 'a' + 10;
   if (c >= 'A') return c - 'A' + 10;
   return c - '0';
}

/**
 * @internal
 * @brief Reads a two-digit hexadecimal escape sequence (e.g., `\xAB`).
 * @param ls The lexer instance.
 * @return The integer value of the escaped character.
 */
static int
read_hex_esc(Eo_Lexer *ls)
{
   int c[3] = { 'x' };
   int i, r = 0;
   for (i = 1; i < 3; ++i)
     {
        next_char(ls);
        c[i] = ls->current;
        if (!isxdigit(c[i]))
          esc_error(ls, c, i + 1, "hexadecimal digit expected");
        r = (r << 4) + hex_val(c[i]);
     }
   return r;
}

/**
 * @internal
 * @brief Reads a one to three-digit decimal/octal escape sequence (e.g., `\123`).
 * @param ls The lexer instance.
 * @return The integer value of the escaped character.
 */
static int
read_dec_esc(Eo_Lexer *ls)
{
   int c[3];
   int i, r = 0;
   for (i = 0; i < 3 && isdigit(ls->current); ++i)
     {
        c[i] = ls->current;
        r = r * 10 + (c[i] - '0');
        next_char(ls);
     }
   if (r > UCHAR_MAX)
     esc_error(ls, c, i, "decimal escape too large");
   return r;
}

/**
 * @internal
 * @brief Parses an escape sequence and appends the resulting character to the lexer buffer.
 *
 * This function is called after a `\` is found inside a string or character
 * literal. It handles standard escapes (`\n`, `\t`, etc.), hexadecimal escapes
 * (`\xHH`), and decimal/octal escapes (`\123`). The interpreted character is
 * appended to `ls->buff`.
 *
 * @param ls The lexer instance.
 */
static void
read_escape(Eo_Lexer *ls)
{
   switch (ls->current)
     {
      case 'a': eina_strbuf_append_char(ls->buff, '\a'); next_char(ls); break;
      case 'b': eina_strbuf_append_char(ls->buff, '\b'); next_char(ls); break;
      case 'f': eina_strbuf_append_char(ls->buff, '\f'); next_char(ls); break;
      case 'n': eina_strbuf_append_char(ls->buff, '\n'); next_char(ls); break;
      case 'r': eina_strbuf_append_char(ls->buff, '\r'); next_char(ls); break;
      case 't': eina_strbuf_append_char(ls->buff, '\t'); next_char(ls); break;
      case 'v': eina_strbuf_append_char(ls->buff, '\v'); next_char(ls); break;
      case 'x':
        eina_strbuf_append_char(ls->buff, read_hex_esc(ls));
        next_char(ls);
        break;
      case '\n': case '\r':
        next_line(ls);
        eina_strbuf_append_char(ls->buff, '\n');
        break;
      case '\\': case '"': case '\'':
        eina_strbuf_append_char(ls->buff, ls->current);
        break;
      case '\0':
        break;
      default:
        if (!isdigit(ls->current))
          esc_error(ls, &ls->current, 1, "invalid escape sequence");
        eina_strbuf_append_char(ls->buff, read_dec_esc(ls));
        break;
     }
}

/**
 * @internal
 * @brief Reads a double-quoted string literal.
 *
 * This function parses a string literal from the input stream, starting after
 * the opening `"`. It processes characters and escape sequences (using
 * `read_escape`) until it finds the closing `"`. The resulting string is
 * stored as a shared string in the token's value.
 *
 * @param ls The lexer instance.
 * @param[out] tok The token to store the parsed string value.
 */
static void
read_string(Eo_Lexer *ls, Eo_Token *tok)
{
   eina_strbuf_reset(ls->buff);
   eina_strbuf_append_char(ls->buff, '"');
   next_char(ls);
   while (ls->current != '"') switch (ls->current)
     {
      case '\0':
        eo_lexer_lex_error(ls, "unfinished string", -1);
        break;
      case '\n': case '\r':
        eo_lexer_lex_error(ls, "unfinished string", TOK_STRING);
        break;
      case '\\':
        {
           next_char(ls);
           read_escape(ls);
           break;
        }
      default:
        eina_strbuf_append_char(ls->buff, ls->current);
        next_char(ls);
     }
   eina_strbuf_append_char(ls->buff, ls->current);
   next_char(ls);
   tok->value.s = eina_stringshare_add_length(eina_strbuf_string_get(ls->buff) + 1,
                                (unsigned int)eina_strbuf_length_get(ls->buff) - 2);
}

/**
 * @internal
 * @brief Determines the specific numerical type based on suffixes.
 *
 * After a number has been read as a string, this function inspects the
 * following characters for type suffixes (like `f`, `u`, `l`, `ll`, `ull`).
 *
 * @param ls The lexer instance.
 * @param is_float EINA_TRUE if the number is known to be a float, EINA_FALSE otherwise.
 * @return A `NUM_*` enum value from `eo_lexer.h` indicating the type.
 */
static int
get_type(Eo_Lexer *ls, Eina_Bool is_float)
{
   if (is_float)
     {
        if (ls->current == 'f' || ls->current == 'F')
          {
             next_char(ls);
             return NUM_FLOAT;
          }
        return NUM_DOUBLE;
     }
   if (ls->current == 'u' || ls->current == 'U')
     {
        next_char(ls);
        if (ls->current == 'l' || ls->current == 'L')
          {
             next_char(ls);
             if (ls->current == 'l' || ls->current == 'L')
               {
                  next_char(ls);
                  return NUM_ULLONG;
               }
             return NUM_ULONG;
          }
        return NUM_UINT;
     }
   if (ls->current == 'l' || ls->current == 'L')
     {
        next_char(ls);
        if (ls->current == 'l' || ls->current == 'L')
          {
             next_char(ls);
             return NUM_LLONG;
          }
        return NUM_LONG;
     }
   return NUM_INT;
}

/**
 * @internal
 * @brief Replaces the decimal point character in the lexer's number buffer.
 *
 * This is a locale-handling helper. C requires '.', but some locales use ','.
 * If a float parse fails, this function is used to swap the decimal point
 * character in the buffered number string to try parsing again with the
 * locale-specific character.
 *
 * @param ls The lexer instance.
 * @param prevdecp The decimal point character to be replaced.
 */
static void
replace_decpoint(Eo_Lexer *ls, char prevdecp)
{
   if (ls->decpoint == prevdecp) return;
   char *bufs = eina_strbuf_string_steal(ls->buff);
   char *p = bufs;
   while ((p = strchr(p, prevdecp))) *p = ls->decpoint;
   eina_strbuf_append(ls->buff, bufs);
   free(bufs);
}

/**
 * @internal
 * @brief Attempts to parse a floating-point number using the locale-specific decimal point.
 *
 * This function is a fallback for when `write_val` fails to parse a float.
 * It replaces the `.` with the system's locale-defined decimal point (e.g., `,`)
 * and retries the conversion using `strtof` or `strtod`.
 *
 * @param ls The lexer instance.
 * @param[out] tok The token to store the parsed value.
 * @param type The numerical type (`NUM_FLOAT` or `NUM_DOUBLE`).
 */
static void
write_val_with_decpoint(Eo_Lexer *ls, Eo_Token *tok, int type)
{
   struct lconv *lc = localeconv();
   char prev = ls->decpoint;
   ls->decpoint = lc ? lc->decimal_point[0] : '.';
   if (ls->decpoint == prev)
     {
        eo_lexer_lex_error(ls, "malformed number", TOK_NUMBER);
        return;
     }
   replace_decpoint(ls, prev);
   char *end = NULL;
   if (type == NUM_FLOAT)
     tok->value.f = strtof(eina_strbuf_string_get(ls->buff), &end);
   else if (type == NUM_DOUBLE)
     tok->value.d = strtod(eina_strbuf_string_get(ls->buff), &end);
   if (end && end[0])
     eo_lexer_lex_error(ls, "malformed number", TOK_NUMBER);
   tok->kw = type;
}

/**
 * @internal
 * @brief Converts a number string from the buffer into a numeric value in the token.
 *
 * This is the main number-parsing function. It first calls `get_type` to
 * determine the exact numerical type from suffixes. Then, it uses the
 * appropriate `strto*` function (e.g., `strtoul`, `strtod`) to convert the
 * string in `ls->buff` into a binary representation, which is stored in the
 * `tok->value` union. For floating-point numbers, it has a fallback to
 * `write_val_with_decpoint` to handle different locales.
 *
 * @param ls The lexer instance.
 * @param[out] tok The token to store the parsed value.
 * @param is_float EINA_TRUE if the number involves a `.` or exponent.
 */
static void
write_val(Eo_Lexer *ls, Eo_Token *tok, Eina_Bool is_float)
{
   int type = get_type(ls, is_float);
   char *end = NULL;
   if (is_float)
     {
        replace_decpoint(ls, '.');
        if (type == NUM_FLOAT)
          tok->value.f = strtof(eina_strbuf_string_get(ls->buff), &end);
        else if (type == NUM_DOUBLE)
          tok->value.d = strtod(eina_strbuf_string_get(ls->buff), &end);
     }
   else
     {
        const char *str = eina_strbuf_string_get(ls->buff);
        /* signed is always in the same memory location */
        if (type == NUM_INT || type == NUM_UINT)
          tok->value.u = strtoul(str, &end, 0);
        else if (type == NUM_LONG || type == NUM_ULONG)
          tok->value.ul = strtoul(str, &end, 0);
        else if (type == NUM_LLONG || type == NUM_ULLONG)
          tok->value.ull = strtoull(str, &end, 0);
     }
   if (end && end[0])
     {
        if (is_float)
          {
             write_val_with_decpoint(ls, tok, type);
             return;
          }
        eo_lexer_lex_error(ls, "malformed number", TOK_NUMBER);
     }
   tok->kw = type;
}

/**
 * @internal
 * @brief Parses the exponent part of a floating-point number (e.g., `e+10`, `P-2`).
 *
 * Appends the exponent character (`e`, `E`, `p`, `P`), an optional sign, and
 * the exponent digits to the lexer's buffer.
 *
 * @param ls The lexer instance.
 */
static void
write_exp(Eo_Lexer *ls)
{
   eina_strbuf_append_char(ls->buff, ls->current);
   next_char(ls);
   if (ls->current == '+' || ls->current == '-')
     {
        eina_strbuf_append_char(ls->buff, ls->current);
        next_char(ls);
        while (isdigit(ls->current))
          {
             eina_strbuf_append_char(ls->buff, ls->current);
             next_char(ls);
          }
     }
}

/**
 * @internal
 * @brief Reads a hexadecimal number literal (integer or float).
 *
 * This function is called after a `0x` prefix is seen. It reads a sequence of
 * hexadecimal digits. If a `.` is encountered, it's treated as a hexadecimal
 * float, which must have a binary exponent (e.g., `0x1.Ap2`). The final
 * string is then converted by `write_val`.
 *
 * @param ls The lexer instance.
 * @param[out] tok The token to store the parsed number.
 */
static void
read_hex_number(Eo_Lexer *ls, Eo_Token *tok)
{
   Eina_Bool is_float = EINA_FALSE;
   while (isxdigit(ls->current) || ls->current == '.')
     {
        eina_strbuf_append_char(ls->buff, ls->current);
        if (ls->current == '.') is_float = EINA_TRUE;
        next_char(ls);
     }
   if (is_float && (ls->current != 'p' && ls->current != 'P'))
     {
        eo_lexer_lex_error(ls, "hex float literals require an exponent",
                           TOK_NUMBER);
     }
   if (ls->current == 'p' || ls->current == 'P')
     {
        is_float = EINA_TRUE;
         write_exp(ls);
     }
   write_val(ls, tok, is_float);
}

/**
 * @internal
 * @brief Reads a decimal, octal, or hexadecimal number literal.
 *
 * This is the entry point for number parsing. It reads a sequence of digits,
 * possibly including a decimal point. If the number starts with `0`, it could
 * be octal or, if followed by `x` or `X`, it dispatches to `read_hex_number`.
 * It also handles floating-point exponents (`e` or `E`). The collected string
 * is then converted by `write_val`.
 *
 * @param ls The lexer instance.
 * @param[out] tok The token to store the parsed number.
 */
static void
read_number(Eo_Lexer *ls, Eo_Token *tok)
{
   Eina_Bool is_float = eina_strbuf_string_get(ls->buff)[0] == '.';
   if (ls->current == '0' && !is_float)
     {
        eina_strbuf_append_char(ls->buff, ls->current);
        next_char(ls);
        if (ls->current == 'x' || ls->current == 'X')
          {
             eina_strbuf_append_char(ls->buff, ls->current);
             next_char(ls);
             read_hex_number(ls, tok);
             return;
          }
     }
   while (isdigit(ls->current) || ls->current == '.')
     {
        eina_strbuf_append_char(ls->buff, ls->current);
        if (ls->current == '.') is_float = EINA_TRUE;
        next_char(ls);
     }
   if (ls->current == 'e' || ls->current == 'E')
     {
        is_float = EINA_TRUE;
         write_exp(ls);
     }
   write_val(ls, tok, is_float);
}

/**
 * @internal
 * @brief The main lexer function; reads the next token from the input stream.
 *
 * This function is the heart of the lexer. It's a state machine implemented
 * as a `switch` on the current input character. It's responsible for:
 * - Skipping whitespace and comments.
 * - Recognizing and parsing multi-character operators (e.g., `==`, `<<`).
 * - Dispatching to specialized functions for complex tokens like strings
 *   (`read_string`), numbers (`read_number`), and documentation blocks
 *   (`read_doc`).
 * - Parsing identifiers and checking if they are keywords.
 * - Returning single-character tokens for all other symbols.
 *
 * @param ls The lexer instance.
 * @param[out] tok The token structure to be filled with information about the
 *                 next token found.
 * @return The token identifier (from `enum Tokens` or an ASCII value), or -1 on EOF.
 */
static int
lex(Eo_Lexer *ls, Eo_Token *tok)
{
   eina_strbuf_reset(ls->buff);
   tok->value.s = NULL;
   for (;;) switch (ls->current)
     {
      case '\n':
      case '\r':
        next_line(ls);
        continue;
      case '/':
        {
           next_char(ls);
           if (ls->current == '*')
             {
                int ccol = ls->column;
                next_char(ls);
                if (ls->current == '@')
                  {
                     eo_lexer_lex_error(ls, "old style documentation comment", -1);
                     return -1; /* unreachable */
                  }
                read_long_comment(ls, ccol);
                continue;
             }
           else if (ls->current != '/') return '/';
           next_char(ls);
           while (ls->current && !is_newline(ls->current))
             next_char(ls);
           continue;
        }
      case '[':
        {
           int dline = ls->line_number, dcol = ls->column;
           const char *sline = ls->stream_line;
           next_char(ls);
           if (ls->current != '[') return '[';
           next_char(ls);
           read_doc(ls, tok, dline, dcol);
           ls->column = dcol + 1;
           /* doc is the only potentially multiline token */
           ls->line_number = dline;
           ls->stream_line = sline;
           return TOK_DOC;
        }
      case '\0':
        return -1;
      case '=':
        next_char(ls);
        if (!ls->expr_mode || (ls->current != '=')) return '=';
        next_char(ls);
        --ls->column;
        return TOK_EQ;
      case '!':
        next_char(ls);
        if (!ls->expr_mode || (ls->current != '=')) return '!';
        next_char(ls);
        --ls->column;
        return TOK_NQ;
      case '>':
        next_char(ls);
        if (!ls->expr_mode) return '>';
        if (ls->current == '=')
          {
             next_char(ls);
             --ls->column;
             return TOK_GE;
          }
        else if (ls->current == '>')
          {
             next_char(ls);
             --ls->column;
             return TOK_RSH;
          }
        return '>';
      case '<':
        next_char(ls);
        if (!ls->expr_mode) return '<';
        if (ls->current == '=')
          {
             next_char(ls);
             --ls->column;
             return TOK_LE;
          }
        else if (ls->current == '<')
          {
             next_char(ls);
             --ls->column;
             return TOK_LSH;
          }
        return '<';
      case '&':
        next_char(ls);
        if (!ls->expr_mode || (ls->current != '&')) return '&';
        next_char(ls);
        --ls->column;
        return TOK_AND;
      case '|':
        next_char(ls);
        if (!ls->expr_mode || (ls->current != '|')) return '|';
        next_char(ls);
        --ls->column;
        return TOK_OR;
      case '"':
        {
           int dcol = ls->column;
           if (!ls->expr_mode)
             {
                next_char(ls);
                return '"';
             }
           /* strings are not multiline for now at least */
           read_string(ls, tok);
           ls->column = dcol + 1;
           return TOK_STRING;
        }
      case '\'':
        {
           int dcol = ls->column;
           next_char(ls);
           if (!ls->expr_mode) return '\'';
           if (ls->current == '\\')
             {
                next_char(ls);
                eina_strbuf_reset(ls->buff);
                read_escape(ls);
                tok->value.c = (char)*eina_strbuf_string_get(ls->buff);
             }
           else
             {
                tok->value.c = ls->current;
                next_char(ls);
             }
           if (ls->current != '\'')
             eo_lexer_lex_error(ls, "unfinished character", TOK_CHAR);
           next_char(ls);
           ls->column = dcol + 1;
           return TOK_CHAR;
        }
      case '.':
        {
           int dcol = ls->column;
           next_char(ls);
           if (!isdigit(ls->current)) return '.';
           eina_strbuf_reset(ls->buff);
           eina_strbuf_append_char(ls->buff, '.');
           read_number(ls, tok);
           ls->column = dcol + 1;
           return TOK_NUMBER;
        }
      default:
        {
           if (isspace(ls->current))
             {
                assert(!is_newline(ls->current));
                next_char(ls);
                continue;
             }
           else if (isdigit(ls->current))
             {
                int col = ls->column;
                eina_strbuf_reset(ls->buff);
                read_number(ls, tok);
                ls->column = col + 1;
                return TOK_NUMBER;
             }
           if (ls->current && (isalnum(ls->current)
               || ls->current == '@' || ls->current == '#' || ls->current == '_'))
             {
                int col = ls->column;
                Eina_Bool pfx_kw = (ls->current == '@') || (ls->current == '#');
                const char *str;
                eina_strbuf_reset(ls->buff);
                do
                  {
                     eina_strbuf_append_char(ls->buff, ls->current);
                     next_char(ls);
                  }
                while (ls->current && (isalnum(ls->current)
                       || ls->current == '_'));
                str     = eina_strbuf_string_get(ls->buff);
                tok->kw = (int)(uintptr_t)eina_hash_find(keyword_map,
                                                        str);
                ls->column = col + 1;
                tok->value.s = eina_stringshare_add(str);
                if (pfx_kw && tok->kw == 0)
                  eo_lexer_syntax_error(ls, "invalid keyword");
                return TOK_VALUE;
             }
           else
             {
                int c = ls->current;
                next_char(ls);
                return c;
             }
        }
     }
}

/**
 * @internal
 * @brief Extracts the basename of a file from its full path.
 *
 * This function handles both `/` and `\` path separators.
 *
 * @param ls The lexer instance, containing the source path.
 * @return A stringshared representation of the filename.
 */
static const char *
get_filename(Eo_Lexer *ls)
{
   const char *fslash = strrchr(ls->source, '/');
   const char *bslash = strrchr(ls->source, '\\');
   if (fslash || bslash)
     return eina_stringshare_add((fslash > bslash) ? (fslash + 1) : (bslash + 1));
   return eina_stringshare_ref(ls->source);
}

/**
 * @internal
 * @brief Callback function to free an `Eolian_Object` node.
 *
 * This function is used by the `eina_hash` that tracks nodes allocated during
 * parsing. If parsing is aborted, this function is called for each node in
 * the hash to ensure proper cleanup. It delegates to the appropriate
 * `database_*_del` function based on the object's type.
 *
 * @param obj The Eolian object to free.
 */
static void
_node_free(Eolian_Object *obj)
{
#if 0
   /* for when we have a proper node allocator and collect on shutdown */
   if (obj->refcount > 1)
     {
        eolian_state_log(obj->state, "node %p (type %d, name %s at %s:%d:%d)"
                         " dangling ref (count: %d)", obj, obj->type, obj->name,
                         obj->file, obj->line, obj->column, obj->refcount);
     }
#endif
   switch (obj->type)
     {
      case EOLIAN_OBJECT_CLASS:
        database_class_del((Eolian_Class *)obj);
        break;
      case EOLIAN_OBJECT_TYPEDECL:
        database_typedecl_del((Eolian_Typedecl *)obj);
        break;
      case EOLIAN_OBJECT_TYPE:
        database_type_del((Eolian_Type *)obj);
        break;
      case EOLIAN_OBJECT_CONSTANT:
        database_constant_del((Eolian_Constant *)obj);
        break;
      case EOLIAN_OBJECT_EXPRESSION:
        database_expr_del((Eolian_Expression *)obj);
        break;
      default:
        /* normally unreachable, just for debug */
        assert(0);
        break;
     }
}

/**
 * @internal
 * @brief Initializes the lexer for a given input source file.
 *
 * This function is called by `eo_lexer_new`. It opens and memory-maps the
 * specified source file, initializes the lexer's internal state (stream
 * pointers, line/column counters, etc.), and prepares it for parsing. It also
 * handles the UTF-8 BOM if present at the beginning of the file.
 *
 * @param ls The lexer instance to initialize.
 * @param state The global Eolian state.
 * @param source The path to the source file.
 */
static void
eo_lexer_set_input(Eo_Lexer *ls, Eolian_State *state, const char *source)
{
   Eina_File *f = eina_file_open(source, EINA_FALSE);
   if (!f)
     {
        eolian_state_log(state, "%s", strerror(errno));
        longjmp(ls->err_jmp, EO_LEXER_ERROR_NORMAL);
     }
   ls->lookahead.token = -1;
   ls->state           = state;
   ls->buff            = eina_strbuf_new();
   ls->handle          = f;
   ls->stream          = eina_file_map_all(f, EINA_FILE_RANDOM);
   ls->stream_end      = ls->stream + eina_file_size_get(f);
   ls->stream_line     = ls->stream;
   ls->source          = eina_stringshare_add(source);
   ls->filename        = get_filename(ls);
   ls->iline_number    = ls->line_number = 1;
   ls->icolumn         = ls->column = -1;
   ls->decpoint        = '.';
   ls->nodes           = eina_hash_pointer_new(EINA_FREE_CB(_node_free));
   next_char(ls);

   Eolian_Unit *ncunit = calloc(1, sizeof(Eolian_Unit));
   if (!ncunit)
     {
        eo_lexer_free(ls);
        eolian_state_panic(state, "out of memory");
     }
   ls->unit = ncunit;
   database_unit_init(state, ncunit, ls->filename);
   eina_hash_add(state->staging.units, ls->filename, ncunit);

   if (ls->current != 0xEF)
     return;
   next_char(ls);
   if (ls->current != 0xBB)
     return;
   next_char(ls);
   if (ls->current != 0xBF)
     return;
   next_char(ls);
}

Eolian_Object *
eo_lexer_node_new(Eo_Lexer *ls, size_t objsize)
{
   Eolian_Object *obj = calloc(1, objsize);
   if (!obj)
     longjmp(ls->err_jmp, EO_LEXER_ERROR_OOM);
   eina_hash_add(ls->nodes, &obj, obj);
   eolian_object_ref(obj);
   return obj;
}

Eolian_Object *
eo_lexer_node_release(Eo_Lexer *ls, Eolian_Object *obj)
{
   /* just for debug */
   assert(eina_hash_find(ls->nodes, &obj) && (obj->refcount >= 1));
   (void)eolian_object_unref(obj);
   eina_hash_set(ls->nodes, &obj, NULL);
   return obj;
}

/**
 * @internal
 * @brief Frees any dynamically allocated memory associated with a token.
 *
 * Most tokens don't own memory, but some do:
 * - `TOK_VALUE` and `TOK_STRING` hold a `Eina_Stringshare`.
 * - `TOK_DOC` holds a pointer to an `Eolian_Documentation` struct.
 * This function checks the token type and frees the associated resources
 * accordingly.
 *
 * @param tok The token to clean up.
 */
static void
_free_tok(Eo_Token *tok)
{
   if (tok->token < START_CUSTOM || tok->token == TOK_NUMBER ||
                                    tok->token == TOK_CHAR)
     return;
   if (tok->token == TOK_DOC)
     {
        /* free doc */
        if (!tok->value.doc) return;
        eina_stringshare_del(tok->value.doc->summary);
        eina_stringshare_del(tok->value.doc->description);
        free(tok->value.doc);
        tok->value.doc = NULL;
        return;
     }
   eina_stringshare_del(tok->value.s);
   tok->value.s = NULL;
}

void
eo_lexer_dtor_push(Eo_Lexer *ls, Eina_Free_Cb free_cb, void *data)
{
   Eo_Lexer_Dtor *dt = malloc(sizeof(Eo_Lexer_Dtor));
   if (!dt)
     {
        free_cb(data);
        longjmp(ls->err_jmp, EO_LEXER_ERROR_OOM);
     }
   dt->free_cb = free_cb;
   dt->data = data;
   ls->dtors = eina_list_prepend(ls->dtors, dt);
}

void
eo_lexer_dtor_pop(Eo_Lexer *ls)
{
   Eo_Lexer_Dtor *dt = eina_list_data_get(ls->dtors);
   ls->dtors = eina_list_remove_list(ls->dtors, ls->dtors);
   dt->free_cb(dt->data);
   free(dt);
}

void
eo_lexer_free(Eo_Lexer *ls)
{
   if (!ls) return;
   if (ls->source  ) eina_stringshare_del(ls->source);
   if (ls->filename) eina_stringshare_del(ls->filename);
   if (ls->buff    ) eina_strbuf_free    (ls->buff);
   if (ls->handle  ) eina_file_close     (ls->handle);

   _free_tok(&ls->t);
   eo_lexer_context_clear(ls);

   Eo_Lexer_Dtor *dtor;
   EINA_LIST_FREE(ls->dtors, dtor)
     dtor->free_cb(dtor->data);

   eina_hash_free(ls->nodes);

   free(ls);
}

Eo_Lexer *
eo_lexer_new(Eolian_State *state, const char *source)
{
   volatile Eo_Lexer *ls = calloc(1, sizeof(Eo_Lexer));
   if (!ls)
     eolian_state_panic(state, "out of memory");

   if (!setjmp(((Eo_Lexer *)(ls))->err_jmp))
     {
        eo_lexer_set_input((Eo_Lexer *) ls, state, source);
        return (Eo_Lexer *) ls;
     }
   eo_lexer_free((Eo_Lexer *) ls);
   return NULL;
}

int
eo_lexer_get(Eo_Lexer *ls)
{
   _free_tok(&ls->t);
   if (ls->lookahead.token >= 0)
     {
        ls->t               = ls->lookahead;
        ls->lookahead.token = -1;
        return ls->t.token;
     }
   ls->t.kw = 0;
   return (ls->t.token = lex(ls, &ls->t));
}

int
eo_lexer_lookahead(Eo_Lexer *ls)
{
   assert (ls->lookahead.token < 0);
   ls->lookahead.kw = 0;
   eo_lexer_context_push(ls);
   ls->lookahead.token = lex(ls, &ls->lookahead);
   eo_lexer_context_restore(ls);
   eo_lexer_context_pop(ls);
   return ls->lookahead.token;
}

void
eo_lexer_lex_error(Eo_Lexer *ls, const char *msg, int token)
{
   if (token)
     {
        char buf[256];
        txt_token(ls, token, buf);
        throw(ls, "%s near '%s'", msg, buf);
     }
   else
     throw(ls, "%s", msg);
}

void
eo_lexer_syntax_error(Eo_Lexer *ls, const char *msg)
{
   eo_lexer_lex_error(ls, msg, ls->t.token);
}

void
eo_lexer_token_to_str(int token, char *buf)
{
   if (token < 0)
     {
        memcpy(buf, "<eof>", 6);
     }
   else if (token < START_CUSTOM)
     {
        assert((unsigned char)token == token);
        if (iscntrl(token))
          sprintf(buf, "char(%d)", token);
        else
          sprintf(buf, "%c", token);
     }
   else
     {
        const char *v;
        size_t idx = token - START_CUSTOM;
        size_t tsz = sizeof(tokens) / sizeof(tokens[0]);
        if (idx >= tsz)
          v = keywords[idx - tsz];
        else
          v = tokens[idx];
        memcpy(buf, v, strlen(v) + 1);
     }
}

const char *
eo_lexer_keyword_str_get(int kw)
{
   return keywords[kw - 1];
}

Eina_Bool
eo_lexer_is_type_keyword(int kw)
{
   return (kw >= KW_byte && kw < KW_true);
}

int
eo_lexer_keyword_str_to_id(const char *kw)
{
   return (int)(uintptr_t)eina_hash_find(keyword_map, kw);
}

const char *
eo_lexer_get_c_type(int kw)
{
   if (!eo_lexer_is_type_keyword(kw)) return NULL;
   return ctypes[kw - KW_byte];
}

/**
 * @internal
 * @brief Checks if a token type is one that holds a stringshare value.
 *
 * This is a helper for context management to know when to ref/unref the
 * stringshare pointer in the token's value union.
 *
 * @param t The token type identifier.
 * @return EINA_TRUE if the token type is `TOK_STRING` or `TOK_VALUE`.
 */
static Eina_Bool
_eo_is_tokstr(int t) {
    return (t == TOK_STRING) || (t == TOK_VALUE);
}

void
eo_lexer_context_push(Eo_Lexer *ls)
{
   Lexer_Ctx *ctx = malloc(sizeof(Lexer_Ctx));
   if (!ctx)
     longjmp(ls->err_jmp, EO_LEXER_ERROR_OOM);
   ctx->line = ls->line_number;
   ctx->column = ls->column;
   ctx->linestr = ls->stream_line;
   ctx->token = ls->t;
   if (_eo_is_tokstr(ctx->token.token))
     eina_stringshare_ref(ctx->token.value.s);
   ls->saved_ctxs = eina_list_prepend(ls->saved_ctxs, ctx);
}

void
eo_lexer_context_pop(Eo_Lexer *ls)
{
   Lexer_Ctx *ctx = (Lexer_Ctx*)eina_list_data_get(ls->saved_ctxs);
   if (_eo_is_tokstr(ctx->token.token))
     eina_stringshare_del(ctx->token.value.s);
   free(ctx);
   ls->saved_ctxs = eina_list_remove_list(ls->saved_ctxs, ls->saved_ctxs);
}

void
eo_lexer_context_restore(Eo_Lexer *ls)
{
   if (!eina_list_count(ls->saved_ctxs)) return;
   Lexer_Ctx *ctx = (Lexer_Ctx*)eina_list_data_get(ls->saved_ctxs);
   ls->line_number = ctx->line;
   ls->column      = ctx->column;
   ls->stream_line = ctx->linestr;
   if (_eo_is_tokstr(ls->t.token))
     eina_stringshare_del(ls->t.value.s);
   ls->t = ctx->token;
   if (_eo_is_tokstr(ls->t.token))
     eina_stringshare_ref(ls->t.value.s);
}

void
eo_lexer_context_clear(Eo_Lexer *ls)
{
   Lexer_Ctx *ctx;
   EINA_LIST_FREE(ls->saved_ctxs, ctx) free(ctx);
}
