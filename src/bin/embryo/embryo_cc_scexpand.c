/**
 * @file embryo_cc_scexpand.c
 * @brief Byte Pair Encoding (BPE) decompression functions.
 *
 * This file implements the decompression logic for data compressed using
 * Byte Pair Encoding. It's adapted from Philip Gage's original work
 * to operate on strings rather than files.
 *
 * @copyright Copyright 1996 Philip Gage
 * @note The decompressor has been modified by Thiadmer Riemersma
 *       to accept a string as input, instead of a complete file.
 *       Byte Pair Compression appeared in the September 1997
 *       issue of C/C++ Users Journal. The original source code
 *       may still be found at the web site of the magazine (www.cuj.com).
 */

#include "embryo_cc_sc.h"

#define STACKSIZE 16 /**< Defines the maximum depth of the character pair stack. */

/**
 * @brief Decompresses a BPE encoded source string into a destination buffer.
 *
 * This function takes a BPE-encoded string (`source`) and expands it into
 * the `dest` buffer. The expansion uses a `pairtable` which maps encoded
 * pair-representing bytes (values > 127) back to their original two-byte sequences.
 *
 * @param dest Pointer to the destination buffer where the decompressed string will be written.
 *             The buffer should be large enough to hold the decompressed string plus a null terminator.
 * @param source Pointer to the null-terminated BPE-encoded source string.
 * @param maxlen The maximum number of bytes (including the null terminator) that can be written to `dest`.
 *               If `maxlen` is 1, only the null terminator is written. If `maxlen` is 0 or negative,
 *               behavior is undefined (though current logic effectively treats it as 1).
 * @param pairtable A 2D array [128][2] used as a lookup table for BPE pairs.
 *                  `pairtable[c - 128][0]` and `pairtable[c - 128][1]` give the two
 *                  characters for an encoded byte `c` (where `c > 127`).
 *                  Example: If `pairtable[0]` is `{'A', 'B'}`, then the encoded byte `128`
 *                  expands to "AB".
 * @return The total length of the decompressed string, including the null terminator.
 *         This is the length the string *would* have, even if `maxlen` caused truncation.
 */
int
strexpand(char *dest, unsigned char *source, int maxlen, unsigned char pairtable[128][2])
{
   unsigned char       stack[STACKSIZE]; /**< Stack to hold bytes of pairs to be processed.
                                           *   When an encoded pair (byte > 127) is encountered,
                                           *   its constituent bytes are pushed onto this stack
                                           *   (second byte first, then first byte) to be
                                           *   processed in LIFO order. */
   short               c, top = 0;     /**< `c` holds the current byte being processed.
                                           *   `top` is the stack pointer for `stack`. */
   int                 len;            /**< Tracks the length of the decompressed string. */

   len = 1;			/* already 1 byte for '\0' */ /* Initialize length to 1 to account for the null terminator. */
   for (;;) /* Loop indefinitely until a break condition is met. */
     {
	/* Pop byte from stack or read byte from the input string */
	if (top) /* If the stack is not empty, pop a byte from it. */
	  c = stack[--top];
	else if ((c = *(unsigned char *)source++) == '\0') /* Otherwise, read a byte from the source string.
	                                                     * If it's the null terminator, end of input. */
	  break;

	/* Push pair on stack or output byte to the output string */
	if (c > 127) /* If the byte value is > 127, it represents an encoded pair. */
	  {
	     /* Push the second byte of the pair, then the first byte onto the stack.
	      * This ensures they are popped in the correct (first, then second) order. */
	     stack[top++] = pairtable[c - 128][1];
	     stack[top++] = pairtable[c - 128][0];
	  }
	else /* Otherwise, the byte is a literal character. */
	  {
	     len++; /* Increment the length of the decompressed string. */
	     if (maxlen > 1) /* If there's space in the destination buffer (more than just for '\0'). */
	       {
		  *dest++ = (char)c; /* Write the character to the destination. */
		  maxlen--;          /* Decrement available space. */
	       }
	  }
     }
   *dest = '\0'; /* Null-terminate the destination string. */
   return len;   /* Return the total calculated length of the decompressed string. */
}
