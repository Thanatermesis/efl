/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.  This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MD5Init, call MD5Update as
 * needed on buffers full of bytes, and then call MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>		/* for memcpy() */
#include "md5.h"

#if (__BYTE_ORDER == 1234)
#define byteReverse(buf, len)	/* Nothing */
#else
/**
 * @brief Reverses the byte order of an array of 32-bit words.
 *
 * This function is used to convert data between little-endian and
 * big-endian formats. It operates in-place.
 * @param buf Pointer to the buffer of 32-bit words (represented as uchars).
 * @param longs The number of 32-bit words in the buffer.
 */
void byteReverse(unsigned char *buf, unsigned longs);

/*
 * Note: this code is harmless on little-endian machines.
 */
/**
 * @brief Reverses the byte order of an array of 32-bit words.
 *
 * This function converts an array of 32-bit unsigned integers from
 * one endianness to another (e.g., little-endian to big-endian or vice-versa)
 * by reversing the byte order within each 32-bit word.
 * The operation is performed in-place.
 *
 * @param buf Pointer to the beginning of the byte array. The array is treated
 *            as a sequence of `longs` 32-bit unsigned integers.
 * @param longs The number of 32-bit words to process.
 */
void byteReverse(unsigned char *buf, unsigned longs)
{
    uint32_t t;
    do {
	t = (uint32_t) ((unsigned) buf[3] << 8 | buf[2]) << 16 |
	    ((unsigned) buf[1] << 8 | buf[0]);
	*(uint32_t *) buf = t;
	buf += 4;
    } while (--longs);
}
#endif

/**
 * @brief Initializes the MD5 context structure.
 *
 * Sets the initial hash values (magic initialization constants) and
 * resets the bit count. This must be called before any calls to MD5Update.
 *
 * @param ctx Pointer to the MD5_CTX structure to be initialized.
 *            The `buf` array will be filled with initial hash values,
 *            and `bits` will be set to 0.
 */
void MD5Init(MD5_CTX *ctx)
{
    ctx->buf[0] = 0x67452301; /* A */
    ctx->buf[1] = 0xefcdab89; /* B */
    ctx->buf[2] = 0x98badcfe; /* C */
    ctx->buf[3] = 0x10325476; /* D */

    ctx->bits[0] = 0;
    ctx->bits[1] = 0;
}

/**
 * @brief Processes a chunk of data and updates the MD5 context.
 *
 * This function can be called multiple times to process data in segments.
 * It updates the internal state of the MD5 computation (bit count,
 * input buffer, and intermediate hash).
 *
 * @param ctx Pointer to the MD5_CTX structure. This structure holds the
 *            current state of the MD5 computation and will be updated.
 * @param buf Pointer to the input data buffer.
 * @param len Length of the input data buffer in bytes.
 */
void MD5Update(MD5_CTX *ctx, unsigned char const *buf, unsigned len)
{
    uint32_t t;

    /* Update bitcount */

    t = ctx->bits[0];
    if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t)
	ctx->bits[1]++;		/* Carry from low to high */
    ctx->bits[1] += len >> 29;

    t = (t >> 3) & 0x3f;	/* Bytes already in shsInfo->data */

    /* Handle any leading odd-sized chunks */

    if (t) {
	unsigned char *p = ctx->in.s + t;

	t = 64 - t;
	if (len < t) {
	    memcpy(p, buf, len);
	    return;
	}
	memcpy(p, buf, t);
	byteReverse(ctx->in.s, 16);
	MD5Transform(ctx->buf, ctx->in.i);
	buf += t;
	len -= t;
    }
    /* Process data in 64-byte chunks */

    while (len >= 64) {
	memcpy(ctx->in.s, buf, 64);
	byteReverse(ctx->in.s, 16);
	MD5Transform(ctx->buf, ctx->in.i);
	buf += 64;
	len -= 64;
    }

    /* Handle any remaining bytes of data. */

    memcpy(ctx->in.s, buf, len);
}

/**
 * @brief Completes the MD5 computation and produces the final hash digest.
 *
 * This function pads the input data to a multiple of 64 bytes, appends
 * the original message length, and performs the final transformations.
 * The resulting 16-byte MD5 hash is stored in the `digest` array.
 * After this function is called, the MD5_CTX is cleared for security.
 *
 * @param digest Output array of 16 bytes where the computed MD5 hash
 *               will be stored.
 *               Example: `unsigned char my_hash[16];`
 * @param ctx Pointer to the MD5_CTX structure. The context will be
 *            used to finalize the hash and then cleared.
 */
void MD5Final(unsigned char digest[16], MD5_CTX *ctx)
{
    unsigned count;
    unsigned char *p;

    /* Compute number of bytes mod 64 */
    count = (ctx->bits[0] >> 3) & 0x3F;

    /* Set the first char of padding to 0x80.  This is safe since there is
       always at least one byte free */
    p = ctx->in.s + count;
    *p++ = 0x80;

    /* Bytes of padding needed to make 64 bytes */
    count = 64 - 1 - count;

    /* Pad out to 56 mod 64 */
    if (count < 8) {
	/* Two lots of padding:  Pad the first block to 64 bytes */
	memset(p, 0, count);
	byteReverse(ctx->in.s, 16);
	MD5Transform(ctx->buf, ctx->in.i);

	/* Now fill the next block with 56 bytes */
	memset(ctx->in.s, 0, 56);
    } else {
	/* Pad block to 56 bytes */
	memset(p, 0, count - 8);
    }
    byteReverse(ctx->in.s, 14);

    /* Append length in bits and transform */
    ctx->in.i[14] = ctx->bits[0];
    ctx->in.i[15] = ctx->bits[1];

    MD5Transform(ctx->buf, ctx->in.i);
    byteReverse((unsigned char *) ctx->buf, 4);
    memcpy(digest, ctx->buf, 16);
    memset(ctx, 0, sizeof(MD5_CTX));	/* In case it's sensitive */
}

/** @name MD5 Core Functions
 *  These are the four non-linear functions used in MD5 rounds.
 *  @{
 */
/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
/** @brief MD5 F function: (X AND Y) OR (NOT X AND Z). Optimized version. */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
/** @brief MD5 G function: (X AND Z) OR (Y AND NOT Z). Implemented using F1. */
#define F2(x, y, z) F1(z, x, y)
/** @brief MD5 H function: X XOR Y XOR Z. */
#define F3(x, y, z) (x ^ y ^ z)
/** @brief MD5 I function: Y XOR (X OR NOT Z). */
#define F4(x, y, z) (y ^ (x | ~z))
/** @} */

/**
 * @brief Performs a single step in the MD5 algorithm's rounds.
 *
 * This macro encapsulates the core calculation of an MD5 round:
 * w = w + f(x, y, z) + data
 * w = (w <<< s) | (w >>> (32-s))  (rotate left)
 * w = w + x
 *
 * @param f One of the MD5 core functions (F1, F2, F3, F4).
 * @param w The accumulator (a, b, c, or d) being modified.
 * @param x One of the other accumulators.
 * @param y One of the other accumulators.
 * @param z One of the other accumulators.
 * @param data A 32-bit word from the current input block.
 * @param s The number of bits to rotate left.
 */
#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f(x, y, z) + data,  w = w<<s | w>>(32-s),  w += x )

/**
 * @brief The core MD5 transformation function.
 *
 * Processes one 512-bit (16-word) block of input data and updates the
 * 128-bit (4-word) MD5 buffer. This is the heart of the MD5 algorithm,
 * performing the four rounds of operations.
 *
 * @param buf The 4-word (A, B, C, D) MD5 state buffer. This is updated in place.
 *            Example: `uint32_t state[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};`
 * @param in  The 16-word input data block to be processed.
 *            Example: `uint32_t data_block[16]; // Filled with 64 bytes of input data`
 */
void MD5Transform(uint32_t buf[4], uint32_t const in[16])
{
    register uint32_t a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
    MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
    MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
    MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
    MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
    MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
    MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
    MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
    MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
    MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
    MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
    MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
    MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
    MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
    MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
    MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
    MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
    MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
    MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
    MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
    MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
    MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}
