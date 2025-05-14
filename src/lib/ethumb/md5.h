/**
 * @file md5.h
 * @brief Header file for MD5 hash algorithm functions.
 *
 * This file declares the structures and functions necessary to compute
 * MD5 message digests.
 */
#ifndef _MD5_H_
#define _MD5_H_

#include <stdint.h>
#include <sys/types.h>

/** @brief The number of bytes in an MD5 hash. */
#define MD5_HASHBYTES 16

/**
 * @brief Structure to hold the MD5 context during computation.
 *
 * This structure stores the intermediate state of the MD5 hash computation,
 * including the current hash buffer, the total number of bits processed,
 * and an input buffer for processing data in 64-byte chunks.
 */
typedef struct MD5Context {
	uint32_t buf[4];        /**< Intermediate hash accumulator (A, B, C, D). */
	uint32_t bits[2];       /**< Number of bits processed, modulo 2^64 (lsb first). */
	union
	  {
	     unsigned char s[64]; /**< Input buffer as bytes. */
	     uint32_t i[16];      /**< Input buffer as 32-bit words. */
	  } in;                 /**< Input buffer for 64-byte blocks. */
} MD5_CTX;

/**
 * @brief Initializes an MD5 context.
 * @param context Pointer to the MD5_CTX structure to initialize.
 */
extern void   MD5Init(MD5_CTX *context);

/**
 * @brief Updates the MD5 context with a block of data.
 * @param context Pointer to the MD5_CTX structure.
 * @param buf Pointer to the input data buffer.
 * @param len Length of the input data buffer in bytes.
 */
extern void   MD5Update(MD5_CTX *context,unsigned char const *buf,unsigned len);

/**
 * @brief Finalizes the MD5 computation and produces the hash.
 * @param digest Array of MD5_HASHBYTES (16) bytes to store the resulting hash.
 *               Example: unsigned char hash_result[MD5_HASHBYTES];
 * @param context Pointer to the MD5_CTX structure.
 */
extern void   MD5Final(unsigned char digest[MD5_HASHBYTES], MD5_CTX *context);

/**
 * @brief The core MD5 transformation. Updates the hash buffer with a 16-word block.
 * @param buf The 4-word (128-bit) hash buffer (A, B, C, D).
 *            Example: uint32_t current_hash[4];
 * @param in The 16-word (512-bit) input data block.
 *           Example: uint32_t input_block[16];
 */
extern void   MD5Transform(uint32_t buf[4], uint32_t const in[16]);

/**
 * @brief Convenience function to finalize MD5 and return a hex string.
 * @param context Pointer to the MD5_CTX structure.
 * @param buffer A character buffer to store the null-terminated hex string.
 *               It should be at least 2 * MD5_HASHBYTES + 1 bytes long.
 *               Example: char hex_output[33];
 * @return Pointer to the provided buffer containing the hex string.
 */
extern char  *MD5End(MD5_CTX *, char *);

/**
 * @brief Computes the MD5 hash of a file.
 * @param filename The path to the file.
 * @param buffer A character buffer to store the null-terminated hex string.
 *               It should be at least 2 * MD5_HASHBYTES + 1 bytes long.
 * @return Pointer to the provided buffer containing the hex string, or NULL on error.
 */
extern char  *MD5File(const char *, char *);

/**
 * @brief Computes the MD5 hash of a data buffer.
 * @param data Pointer to the data to hash.
 * @param len Length of the data in bytes.
 * @param buffer A character buffer to store the null-terminated hex string.
 *               It should be at least 2 * MD5_HASHBYTES + 1 bytes long.
 * @return Pointer to the provided buffer containing the hex string.
 */
extern char  *MD5Data (const unsigned char *, unsigned int, char *);

#endif
