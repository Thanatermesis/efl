/**
 * @brief Scales an image region down using a smoothing algorithm.
 *
 * This function implements a downscaling operation with smoothing. It handles
 * images with and without alpha channels, and can apply a color multiplier
 * and a mask. The core logic involves iterating over the destination pixels,
 * and for each destination pixel, sampling and blending multiple source pixels
 * based on pre-calculated x and y point mappings (xpoints, ypoints, xapoints, yapoints).
 *
 * @param dst_ptr Pointer to the destination image data.
 * @param src Pointer to the source image cache entry.
 * @param dst Pointer to the destination image cache entry.
 * @param mul_col Color multiplier (0xffffffff for no multiplication).
 * @param func Pointer to the span function to draw pixels without a mask.
 * @param func2 Pointer to the span function to draw pixels with a color multiplier but no mask (used internally).
 * @param dst_clip_x X-coordinate of the destination clipping region.
 * @param dst_clip_y Y-coordinate of the destination clipping region.
 * @param dst_clip_w Width of the destination clipping region.
 * @param dst_clip_h Height of the destination clipping region.
 * @param src_region_x X-coordinate of the source region.
 * @param src_region_y Y-coordinate of the source region.
 * @param src_w Width of the source image.
 * @param dst_w Width of the destination image.
 * @param xpoints Array of X source coordinates for each destination X.
 *                Essentially, for each column in the destination, this gives the
 *                corresponding starting column in the source.
 * @param ypoints Array of pointers to Y source scanlines for each destination Y.
 *                For each row in the destination, this points to an array (or offset)
 *                that gives the corresponding starting row in the source.
 * @param xapoints Array of X alpha/blending factors for each destination X.
 *                 These are used for horizontal interpolation between source pixels.
 *                 Values are typically in a fixed-point format (e.g., 0-255 or 0- (1 << 14)).
 * @param yapoints Array of Y alpha/blending factors for each destination Y.
 *                 These are used for vertical interpolation between source pixels.
 *                 The higher 16 bits (*yapp >> 16) often represent a primary weight or count (Cy),
 *                 and the lower 16 bits (*yapp & 0xffff) represent a fractional component (yap)
 *                 for blending, typically in a fixed-point format (e.g., 0 - (1 << 14)).
 * @param buf Temporary buffer for storing processed scanlines.
 * @param mask_ie Optional mask image cache entry.
 * @param mask_x X-coordinate offset for the mask.
 * @param mask_y Y-coordinate offset for the mask.
 */
{
   int Cy, j;
   DATA32 *dptr, *pix, *pbuf, **yp;
   DATA8 *mask;
   int r, g, b, a, rr, gg, bb, aa;
   int *xp, xap, yap, pos;
   //int dyy, dxx; // Potentially for adjusting coordinates based on region vs clip offsets
   int w = dst_clip_w; // Store original destination clip width
   int y; // Loop counter for destination rows

   dptr = dst_ptr;
   // Calculate the starting offset in the source image data based on the source region
   pos = (src_region_y * src_w) + src_region_x;
   //dyy = dst_clip_y - dst_region_y; // Unused: intended for y-offset adjustments
   //dxx = dst_clip_x - dst_region_x; // Unused: intended for x-offset adjustments

   xp = xpoints;   // Pointer to current X source coordinate lookup
   yp = ypoints;   // Pointer to current Y source scanline lookup
   xapp = xapoints; // Pointer to current X alpha/blending factor lookup
   yapp = yapoints; // Pointer to current Y alpha/blending factor lookup
   pbuf = buf;     // Pointer to the current position in the temporary scanline buffer

   // Check if the source image has an alpha channel
   if (src->cache_entry.flags.alpha)
     {
        y = 0; // Initialize destination row counter
        // Loop over each row in the destination clipping region
	while (dst_clip_h--)
	  {
            // Cy: Primary weight or count for vertical blending, derived from high bits of yapp
            // yap: Fractional component for vertical blending, derived from low bits of yapp
	    Cy = *yapp >> 16;
	    yap = *yapp & 0xffff; // yap is a fixed point value (0 to (1<<14)-1)

            // Loop over each pixel in the width of the destination clipping region
	    while (dst_clip_w--)
	      {
                // Calculate pointer to the first source pixel contributing to the current destination pixel
		pix = *yp + *xp + pos;

                // Initialize accumulated ARGB values with the first source pixel, weighted by yap
                // >> 10 is likely a normalization factor for the fixed-point yap
		a = (A_VAL(pix) * yap) >> 10;
		r = (R_VAL(pix) * yap) >> 10;
		g = (G_VAL(pix) * yap) >> 10;
		b = (B_VAL(pix) * yap) >> 10;

                // Accumulate color from subsequent source rows, weighted by Cy
                // (1 << 14) is the maximum value for yap, representing full contribution.
                // This loop handles full rows contributing to the destination pixel.
		for (j = (1 << 14) - yap; j > Cy; j -= Cy)
		  {
		    pix += src_w; // Move to the next source row
		    a += (A_VAL(pix) * Cy) >> 10;
		    r += (R_VAL(pix) * Cy) >> 10;
		    g += (G_VAL(pix) * Cy) >> 10;
		    b += (B_VAL(pix) * Cy) >> 10;
		  }
                // Handle the last partial source row contribution, if any
		if (j > 0)
		  {
		    pix += src_w;
		    a += (A_VAL(pix) * j) >> 10;
		    r += (R_VAL(pix) * j) >> 10;
		    g += (G_VAL(pix) * j) >> 10;
		    b += (B_VAL(pix) * j) >> 10;
		  }

                // Horizontal interpolation if xap (horizontal alpha/blending factor) is > 0
		if ((xap = *xapp) > 0) // xap is a fixed point value (0-255)
		  {
                    // Point to the next source pixel in the same row for horizontal blending
		    pix = *yp + *xp + 1 + pos;
                    // Initialize ARGB values for the horizontally adjacent pixel, weighted by yap
		    aa = (A_VAL(pix) * yap) >> 10;
		    rr = (R_VAL(pix) * yap) >> 10;
		    gg = (G_VAL(pix) * yap) >> 10;
		    bb = (B_VAL(pix) * yap) >> 10;
                    // Accumulate from subsequent source rows for the adjacent pixel
		    for (j = (1 << 14) - yap; j > Cy; j -= Cy)
		      {
			pix += src_w;
			aa += (A_VAL(pix) * Cy) >> 10;
			rr += (R_VAL(pix) * Cy) >> 10;
			gg += (G_VAL(pix) * Cy) >> 10;
			bb += (B_VAL(pix) * Cy) >> 10;
		      }
                    // Handle last partial source row for the adjacent pixel
		    if (j > 0)
		      {
			pix += src_w;
			aa += (A_VAL(pix) * j) >> 10;
			rr += (R_VAL(pix) * j) >> 10;
			gg += (G_VAL(pix) * j) >> 10;
			bb += (B_VAL(pix) * j) >> 10;
		      }
                    // Blend the two horizontally sampled pixel columns using xap
                    // >> 8 is likely a normalization for xap (0-255 range)
		    a += ((aa - a) * xap) >> 8;
		    r += ((rr - r) * xap) >> 8;
		    g += ((gg - g) * xap) >> 8;
		    b += ((bb - b) * xap) >> 8;
		  }
                // Store the final blended pixel in the temporary buffer
                // (1 << 3) is for rounding before >> 4 (division by 16, possibly for component range adjustment or averaging)
		*pbuf++ = ARGB_JOIN(((a + (1 << 3)) >> 4),
				    ((r + (1 << 3)) >> 4),
				    ((g + (1 << 3)) >> 4),
				    ((b + (1 << 3)) >> 4));
		xp++;  xapp++; // Move to the next destination column's x-parameters
	      }

            // Process the completed scanline in 'buf'
            if (!mask_ie) // If no mask is provided
              { // Draw the scanline directly or with color multiplication
                func(buf, NULL, mul_col, dptr, w);
              }
            else // If a mask is provided
              {
                 // Calculate pointer to the current row in the mask data
                 mask = mask_ie->image.data8
                    + ((dst_clip_y - mask_y + y) * mask_ie->cache_entry.w)
                    + (dst_clip_x - mask_x);

                 // If color multiplication is needed, apply it to 'buf' first
                 if (mul_col != 0xffffffff) func2(buf, NULL, mul_col, buf, w);
                 // Draw the scanline using the mask
                 func(buf, mask, 0, dptr, w);
              }
            y++; // Increment destination row counter

	    pbuf = buf; // Reset buffer pointer for the next scanline
	    dptr += dst_w;  // Move destination pointer to the next scanline
            dst_clip_w = w; // Reset destination clip width for the next row
	    yp++;  yapp++; // Move to the next destination row's y-parameters
	    xp = xpoints;   // Reset x-parameter pointers to the beginning of their arrays
	    xapp = xapoints;
	  }
     }
   else // Source image does not have an alpha channel (opaque)
     {
#ifdef DIRECT_SCALE
        // Optimization: If source and destination are opaque, no color multiplication, and no mask,
        // then write directly to the destination buffer instead of the temporary 'buf'.
        if ((!src->cache_entry.flags.alpha) && // Already known from outer 'else'
            (!dst->cache_entry.flags.alpha) &&
            (mul_col == 0xffffffff) &&
            (!mask_ie))
	  {
             // Loop over each row in the destination clipping region
	     while (dst_clip_h--)
	       {
		 Cy = *yapp >> 16;
		 yap = *yapp & 0xffff;

		 pbuf = dptr; // Write directly to destination
                 // Loop over each pixel in the width of the destination clipping region
		 while (dst_clip_w--)
		   {
		     pix = *yp + *xp + pos;

                     // Similar RGB calculation as in the alpha case, but alpha is assumed 0xff
		     r = (R_VAL(pix) * yap) >> 10;
		     g = (G_VAL(pix) * yap) >> 10;
		     b = (B_VAL(pix) * yap) >> 10;
		     for (j = (1 << 14) - yap; j > Cy; j -= Cy)
		       {
			 pix += src_w;
			 r += (R_VAL(pix) * Cy) >> 10;
			 g += (G_VAL(pix) * Cy) >> 10;
			 b += (B_VAL(pix) * Cy) >> 10;
		       }
		     if (j > 0)
		       {
			 pix += src_w;
			 r += (R_VAL(pix) * j) >> 10;
			 g += (G_VAL(pix) * j) >> 10;
			 b += (B_VAL(pix) * j) >> 10;
		       }
		     if ((xap = *xapp) > 0)
		       {
			 pix = *yp + *xp + 1 + pos;
			 rr = (R_VAL(pix) * yap) >> 10;
			 gg = (G_VAL(pix) * yap) >> 10;
			 bb = (B_VAL(pix) * yap) >> 10;
			 for (j = (1 << 14) - yap; j > Cy; j -= Cy)
			   {
			     pix += src_w;
			     rr += (R_VAL(pix) * Cy) >> 10;
			     gg += (G_VAL(pix) * Cy) >> 10;
			     bb += (B_VAL(pix) * Cy) >> 10;
			   }
			 if (j > 0)
			   {
			     pix += src_w;
			     rr += (R_VAL(pix) * j) >> 10;
			     gg += (G_VAL(pix) * j) >> 10;
			     bb += (B_VAL(pix) * j) >> 10;
			   }
			 r += ((rr - r) * xap) >> 8;
			 g += ((gg - g) * xap) >> 8;
			 b += ((bb - b) * xap) >> 8;
		       }
                     // Store final pixel directly in destination, with full alpha (0xff)
		     *pbuf++ = ARGB_JOIN(0xff,
					 ((r + (1 << 3)) >> 4),
					 ((g + (1 << 3)) >> 4),
					 ((b + (1 << 3)) >> 4));
		     xp++;  xapp++;
		   }

		 dptr += dst_w;  dst_clip_w = w;
		 yp++;  yapp++;
		 xp = xpoints;
		 xapp = xapoints;
	       }
	  }
	else // Fallback if DIRECT_SCALE conditions are not met (still no source alpha)
#endif
	  {
             y = 0; // Initialize destination row counter
             // Loop over each row in the destination clipping region
	     while (dst_clip_h--)
	       {
		 Cy = *yapp >> 16;
		 yap = *yapp & 0xffff;

                 // Loop over each pixel in the width of the destination clipping region
		 while (dst_clip_w--)
		   {
		     pix = *yp + *xp + pos;

                     // Calculate RGB values, alpha is implicitly full (0xff)
		     r = (R_VAL(pix) * yap) >> 10;
		     g = (G_VAL(pix) * yap) >> 10;
		     b = (B_VAL(pix) * yap) >> 10;
		     for (j = (1 << 14) - yap; j > Cy; j -= Cy)
		       {
			 pix += src_w;
			 r += (R_VAL(pix) * Cy) >> 10;
			 g += (G_VAL(pix) * Cy) >> 10;
			 b += (B_VAL(pix) * Cy) >> 10;
		       }
		     if (j > 0)
		       {
			 pix += src_w;
			 r += (R_VAL(pix) * j) >> 10;
			 g += (G_VAL(pix) * j) >> 10;
			 b += (B_VAL(pix) * j) >> 10;
		       }
		     if ((xap = *xapp) > 0)
		       {
			 pix = *yp + *xp + 1 + pos;
			 rr = (R_VAL(pix) * yap) >> 10;
			 gg = (G_VAL(pix) * yap) >> 10;
			 bb = (B_VAL(pix) * yap) >> 10;
			 for (j = (1 << 14) - yap; j > Cy; j -= Cy)
			   {
			     pix += src_w;
			     rr += (R_VAL(pix) * Cy) >> 10;
			     gg += (G_VAL(pix) * Cy) >> 10;
			     bb += (B_VAL(pix) * Cy) >> 10;
			   }
			 if (j > 0)
			   {
			     pix += src_w;
			     rr += (R_VAL(pix) * j) >> 10;
			     gg += (G_VAL(pix) * j) >> 10;
			     bb += (B_VAL(pix) * j) >> 10;
			   }
			 r += ((rr - r) * xap) >> 8;
			 g += ((gg - g) * xap) >> 8;
			 b += ((bb - b) * xap) >> 8;
		       }
                     // Store the final pixel in the temporary buffer with full alpha
		     *pbuf++ = ARGB_JOIN(0xff,
					 ((r + (1 << 3)) >> 4),
					 ((g + (1 << 3)) >> 4),
					 ((b + (1 << 3)) >> 4));
		     xp++;  xapp++;
		   }

                 // Process the completed scanline in 'buf'
                 if (!mask_ie) // If no mask
                   { // Draw the scanline directly or with color multiplication
                     func(buf, NULL, mul_col, dptr, w);
                   }
                 else // If mask is provided
                   {
                      // Calculate pointer to the current row in the mask data
                      mask = mask_ie->image.data8
                         + ((dst_clip_y - mask_y + y) * mask_ie->cache_entry.w)
                         + (dst_clip_x - mask_x);

                      // If color multiplication is needed, apply it to 'buf' first
                      if (mul_col != 0xffffffff) func2(buf, NULL, mul_col, buf, w);
                      // Draw the scanline using the mask
                      func(buf, mask, 0, dptr, w);
                   }
                 y++; // Increment destination row counter

		 pbuf = buf; // Reset buffer pointer for the next scanline
		 dptr += dst_w;  // Move destination pointer to the next scanline
                 dst_clip_w = w; // Reset destination clip width for the next row
		 yp++;  yapp++; // Move to the next destination row's y-parameters
		 xp = xpoints;   // Reset x-parameter pointers
		 xapp = xapoints;
	       }
	  }
     }
}
