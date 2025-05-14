/* EINA - EFL data type library
 * Copyright (C) 2023 Carsten Haitzler
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

#include <string.h>

#include "eina_private.h"
#include "eina_sha.h"
#include "eina_cpu.h"

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/


EINA_API void
eina_sha1(const unsigned char *data, int size, unsigned char dst[20])
{
   // SHSH is a circular left shift (rotate left) operation on a 32-bit word.
   // n: number of bits to shift
   // v: value to shift
#define SHSH(n, v) ((((v) << (n)) & 0xffffffff) | ((v) >> (32 - (n))))
   unsigned int digest[5], word[80], wa, wb, wc, wd, we, t;
   unsigned char buf[64]; // Buffer to hold 64-byte (512-bit) chunks of data
   const unsigned char *d;
   int idx, left, i;
   // These are the SHA-1 round constants (K values) for each of the 4 rounds (20 steps each).
   const unsigned int magic[4] =
   {
      0x5a827999, // Rounds  0-19
      0x6ed9eba1, // Rounds 20-39
      0x8f1bbcdc, // Rounds 40-59
      0xca62c1d6  // Rounds 60-79
   };

   idx = 0; // Current position in the 64-byte buffer `buf`
   // Initialize the five SHA-1 hash values (H0 to H4).
   // These are standard initial values for SHA-1.
   digest[0] = 0x67452301;
   digest[1] = 0xefcdab89;
   digest[2] = 0x98badcfe;
   digest[3] = 0x10325476;
   digest[4] = 0xc3d2e1f0;

   memset(buf, 0, sizeof(buf)); // Initialize buffer, important for padding
   // Process data in 64-byte chunks
   for (left = size, d = data; left > 0; left--, d++)
     {
        // If this is the start of a new block and it's the last block (less than 64 bytes remaining),
        // prepare the padding. SHA-1 padding:
        // 1. Append a '1' bit (0x80 byte).
        // 2. Append '0' bits until message length is 448 mod 512.
        // 3. Append original message length as a 64-bit big-endian integer.
        // This implementation handles padding slightly differently by pre-filling the length
        // if the current chunk is the one that will contain the end of the message.
        if ((idx == 0) && (left < 64))
          {
             // Pre-fill the last 8 bytes of the buffer with the original message size in bits.
             // Note: SHA-1 expects message length in bits. Here, `size` is in bytes.
             // So, `size * 8` is the length in bits.
             // This implementation stores `size` (bytes) directly, which is incorrect as per SHA-1 standard
             // if strict compliance is needed (it should be size * 8 and stored in 8 bytes).
             // However, for simplicity or a specific internal use case, it might be intentional.
             // The buffer is 64 bytes. The last 8 bytes are for the length.
             // Data can go up to buf[55]. buf[56] to buf[63] is for length.
             // memset(buf, 0, 60) clears up to index 59.
             memset(buf, 0, 60); // Clear buffer up to where length will be written
             // Store original message length (in bytes, big-endian) in the last 4 bytes of the 64-byte block.
             // SHA-1 standard requires a 64-bit (8-byte) length. This uses 32-bit (4-byte).
             buf[60] = (size >> 24) & 0xff;
             buf[61] = (size >> 16) & 0xff;
             buf[62] = (size >> 8) & 0xff;
             buf[63] = (size) & 0xff;
          }
        buf[idx] = *d; // Copy data byte into current buffer
        idx++;
        // If buffer is full (64 bytes) or this is the last byte of data
        if ((idx == 64) || (left == 1))
          {
             // If it's the last byte of data and the buffer isn't full yet,
             // add the SHA-1 padding marker '1' bit (0x80).
             if ((left == 1) && (idx < 64)) buf[idx] = 0x80;

             // Process the 64-byte chunk (buf)
             // Convert the 64-byte buffer into 16 32-bit words (big-endian)
             for (i = 0; i < 16; i++)
               {
                  word[i]  = (unsigned int)buf[(i * 4)    ] << 24;
                  word[i] |= (unsigned int)buf[(i * 4) + 1] << 16;
                  word[i] |= (unsigned int)buf[(i * 4) + 2] << 8;
                  word[i] |= (unsigned int)buf[(i * 4) + 3];
               }
             // Extend these 16 32-bit words into 80 32-bit words (message schedule)
             for (i = 16; i < 80; i++)
               word[i] = SHSH(1, // Circular left shift by 1
                              word[i - 3 ] ^ word[i - 8 ] ^
                              word[i - 14] ^ word[i - 16]);

             // Initialize hash values for this chunk
             wa = digest[0];
             wb = digest[1];
             wc = digest[2];
             wd = digest[3];
             we = digest[4];

             // Main compression loop (80 rounds)
             for (i = 0; i < 80; i++)
               {
                  unsigned int f; // Function f depends on the round
                  unsigned int k; // Round constant k depends on the round

                  if (i < 20) // Rounds 0-19
                    {
                       // f = (b AND c) OR ((NOT b) AND d)
                       f = (wb & wc) | ((~wb) & wd);
                       k = magic[0];
                    }
                  else if (i < 40) // Rounds 20-39
                    {
                       // f = b XOR c XOR d
                       f = wb ^ wc ^ wd;
                       k = magic[1];
                    }
                  else if (i < 60) // Rounds 40-59
                    {
                       // f = (b AND c) OR (b AND d) OR (c AND d)
                       f = (wb & wc) | (wb & wd) | (wc & wd);
                       k = magic[2];
                    }
                  else // Rounds 60-79 (i < 80)
                    {
                       // f = b XOR c XOR d
                       f = wb ^ wc ^ wd;
                       k = magic[3];
                    }
                  // t = (a <<< 5) + f(b,c,d) + e + W[i] + K[i]
                  t = SHSH(5, wa) + f + we + word[i] + k;
                  we = wd;
                  wd = wc;
                  wc = SHSH(30, wb); // c = b <<< 30
                  wb = wa;
                  wa = t;
               }
             // Add this chunk's hash to result so far
             digest[0] += wa;
             digest[1] += wb;
             digest[2] += wc;
             digest[3] += wd;
             digest[4] += we;
             idx = 0; // Reset buffer index for next chunk
          }
     }

   // Convert the final digest words to network byte order (big-endian)
   // SHA-1 specifies the output as big-endian.
   t = eina_htonl(digest[0]); digest[0] = t;
   t = eina_htonl(digest[1]); digest[1] = t;
   t = eina_htonl(digest[2]); digest[2] = t;
   t = eina_htonl(digest[3]); digest[3] = t;
   t = eina_htonl(digest[4]); digest[4] = t;

   // Copy the 20-byte digest to the destination buffer
   memcpy(dst, digest, 5 * 4);
}
