/**
 * @brief Scales down an image region horizontally using a smooth scaling algorithm.
 *
 * This function implements a horizontal downscaling operation. It processes
 * pixel data from a source image (`src`) and writes the scaled result to a
 * destination buffer (`dst_ptr`). The scaling considers alpha transparency
 * and can apply a color multiplier (`mul_col`) and a mask (`mask_ie`).
 *
 * The core logic involves iterating over destination pixels and, for each,
 * calculating a weighted average of source pixels. The `xpoints`, `xapoints`,
 * `ypoints`, and `yapoints` arrays guide this sampling and weighting process.
 *
 * `xpoints`: Array of x-coordinates in the source image corresponding to each destination pixel.
 * `xapoints`: Array of weights for horizontal interpolation. The higher 16 bits
 *             represent `Cx` (the step/increment for source pixels), and the lower
 *             16 bits represent `xap` (the initial weight for the first source pixel).
 * `ypoints`: Array of pointers to the start of each relevant scanline in the source image.
 * `yapoints`: Array of weights for vertical interpolation.
 *
 * `pos`: Offset to the top-left corner of the source region being processed.
 *
 * The function handles two main cases:
 * 1. Source image has alpha: Alpha blending is performed.
 * 2. Source image has no alpha: Processing is simpler, with an optional
 *    `DIRECT_SCALE` optimization for specific conditions (no alpha in src/dst,
 *    no color multiplication, no mask).
 */
{
   int Cx, j;
   DATA32 *pix, *dptr, *pbuf, **yp;
   DATA8 *mask;
   int r, g, b, a, rr, gg, bb, aa;
   int *xp, xap, yap, pos;
   //int dyy, dxx;
   int w = dst_clip_w;
   int y;

   dptr = dst_ptr;
   pos = (src_region_y * src_w) + src_region_x;
   //dyy = dst_clip_y - dst_region_y;
   //dxx = dst_clip_x - dst_region_x;

   xp = xpoints;// + dxx;
   yp = ypoints;// + dyy;
   xapp = xapoints;// + dxx;
   yapp = yapoints;// + dyy;
   pbuf = buf;

   // Check if the source image has an alpha channel.
   if (src->cache_entry.flags.alpha)
     {
        y = 0;
        // Loop through each destination row.
	while (dst_clip_h--)
	  {
            // Loop through each destination pixel in the current row.
	    while (dst_clip_w--)
	      {
                // Cx: horizontal step in source pixels (fixed point 18.14).
                // xap: initial weight for the first source pixel (fixed point 2.14).
		Cx = *xapp >> 16;
		xap = *xapp & 0xffff;
		pix = *yp + *xp + pos;

                // Accumulate weighted RGBA values from the current source scanline.
                // Operations are scaled by >> 10, effectively dividing by 1024.
		a = (A_VAL(pix) * xap) >> 10;
		r = (R_VAL(pix) * xap) >> 10;
		g = (G_VAL(pix) * xap) >> 10;
		b = (B_VAL(pix) * xap) >> 10;
                // Loop over subsequent source pixels contributing to the current destination pixel.
		for (j = (1 << 14) - xap; j > Cx; j -= Cx)
		  {
		    pix++;
		    a += (A_VAL(pix) * Cx) >> 10;
		    r += (R_VAL(pix) * Cx) >> 10;
		    g += (G_VAL(pix) * Cx) >> 10;
		    b += (B_VAL(pix) * Cx) >> 10;
		  }
                // Handle the last partial contribution.
		if (j > 0)
		  {
		    pix++;
		    a += (A_VAL(pix) * j) >> 10;
		    r += (R_VAL(pix) * j) >> 10;
		    g += (G_VAL(pix) * j) >> 10;
		    b += (B_VAL(pix) * j) >> 10;
		  }
                // If yap (vertical interpolation weight) is > 0, blend with the next scanline.
		if ((yap = *yapp) > 0)
		  {
                    // Point to the corresponding pixel in the next source scanline.
		    pix = *yp + *xp + src_w + pos;
                    // Accumulate weighted RGBA values from the next source scanline.
		    aa = (A_VAL(pix) * xap) >> 10;
		    rr = (R_VAL(pix) * xap) >> 10;
		    gg = (G_VAL(pix) * xap) >> 10;
		    bb = (B_VAL(pix) * xap) >> 10;
		    for (j = (1 << 14) - xap; j > Cx; j -= Cx)
		      {
			pix++;
			aa += (A_VAL(pix) * Cx) >> 10;
			rr += (R_VAL(pix) * Cx) >> 10;
			gg += (G_VAL(pix) * Cx) >> 10;
			bb += (B_VAL(pix) * Cx) >> 10;
		      }
		    if (j > 0)
		      {
			pix++;
			aa += (A_VAL(pix) * j) >> 10;
			rr += (R_VAL(pix) * j) >> 10;
			gg += (G_VAL(pix) * j) >> 10;
			bb += (B_VAL(pix) * j) >> 10;
		      }
                    // Perform vertical interpolation. yap is 0.8 fixed point.
		    a += ((aa - a) * yap) >> 8;
		    r += ((rr - r) * yap) >> 8;
		    g += ((gg - g) * yap) >> 8;
		    b += ((bb - b) * yap) >> 8;
		  }
                // Store the final pixel value, scaling down by >> 4 (divide by 16) with rounding.
		*pbuf++ = ARGB_JOIN(((a + (1 << 3)) >> 4),
				    ((r + (1 << 3)) >> 4),
				    ((g + (1 << 3)) >> 4),
				    ((b + (1 << 3)) >> 4));
		xp++;  xapp++;
	      }

            // After processing a row, apply mask and color multiplication if necessary.
            if (!mask_ie) // No mask
              func(buf, NULL, mul_col, dptr, w);
            else // Apply mask
              {
                 mask = mask_ie->image.data8
                    + ((dst_clip_y - mask_y + y) * mask_ie->cache_entry.w)
                    + (dst_clip_x - mask_x);

                 if (mul_col != 0xffffffff) func2(buf, NULL, mul_col, buf, w);
                 func(buf, mask, 0, dptr, w);
              }
            y++;

	    pbuf = buf;
	    dptr += dst_w;  dst_clip_w = w;
	    yp++;  yapp++;
	    xp = xpoints;// + dxx;
	    xapp = xapoints;// + dxx;
	  }
     }
   else // Source image does not have an alpha channel.
     {
#ifdef DIRECT_SCALE
        // Optimization: If source and destination have no alpha, no color multiplication,
        // and no mask, write directly to the destination buffer.
        if ((!src->cache_entry.flags.alpha) && // This condition is redundant due to the parent `else`
            (!dst->cache_entry.flags.alpha) &&
            (mul_col == 0xffffffff) &&
            (!mask_ie))
	  {
             // Loop through each destination row.
	     while (dst_clip_h--)
	       {
                  pbuf = dptr; // Write directly to destination.

                  // Loop through each destination pixel in the current row.
		  while (dst_clip_w--)
		    {
                      // Cx: horizontal step in source pixels (fixed point 18.14).
                      // xap: initial weight for the first source pixel (fixed point 2.14).
		      Cx = *xapp >> 16;
		      xap = *xapp & 0xffff;
		      pix = *yp + *xp + pos;

                      // Accumulate weighted RGB values from the current source scanline.
                      // Alpha is assumed to be opaque (0xff).
		      r = (R_VAL(pix) * xap) >> 10;
		      g = (G_VAL(pix) * xap) >> 10;
		      b = (B_VAL(pix) * xap) >> 10;
                      // Loop over subsequent source pixels.
		      for (j = (1 << 14) - xap; j > Cx; j -= Cx)
			{
			  pix++;
			  r += (R_VAL(pix) * Cx) >> 10;
			  g += (G_VAL(pix) * Cx) >> 10;
			  b += (B_VAL(pix) * Cx) >> 10;
			}
                      // Handle the last partial contribution.
		      if (j > 0)
			{
			  pix++;
			  r += (R_VAL(pix) * j) >> 10;
			  g += (G_VAL(pix) * j) >> 10;
			  b += (B_VAL(pix) * j) >> 10;
			}
                      // If yap (vertical interpolation weight) is > 0, blend with the next scanline.
		      if ((yap = *yapp) > 0)
			{
                          // Point to the corresponding pixel in the next source scanline.
			  pix = *yp + *xp + src_w + pos;
                          // Accumulate weighted RGB values from the next source scanline.
			  rr = (R_VAL(pix) * xap) >> 10;
			  gg = (G_VAL(pix) * xap) >> 10;
			  bb = (B_VAL(pix) * xap) >> 10;
			  for (j = (1 << 14) - xap; j > Cx; j -= Cx)
			    {
			      pix++;
			      rr += (R_VAL(pix) * Cx) >> 10;
			      gg += (G_VAL(pix) * Cx) >> 10;
			      bb += (B_VAL(pix) * Cx) >> 10;
			    }
			  if (j > 0)
			    {
			      pix++;
			      rr += (R_VAL(pix) * j) >> 10;
			      gg += (G_VAL(pix) * j) >> 10;
			      bb += (B_VAL(pix) * j) >> 10;
			    }
                          // Perform vertical interpolation.
			  r += ((rr - r) * yap) >> 8;
			  g += ((gg - g) * yap) >> 8;
			  b += ((bb - b) * yap) >> 8;
			}
                      // Store the final pixel value with opaque alpha.
		      *pbuf++ = ARGB_JOIN(0xff,
					  ((r + (1 << 3)) >> 4),
					  ((g + (1 << 3)) >> 4),
					  ((b + (1 << 3)) >> 4));
		      xp++;  xapp++;
		    }

                  // Move to the next destination row.
		  dptr += dst_w;  dst_clip_w = w;
		  yp++;  yapp++;
		  xp = xpoints;// + dxx;
		  xapp = xapoints;// + dxx;
	       }
	  }
	else // Fallback if DIRECT_SCALE is not defined or conditions are not met.
#endif
	  {
             y = 0;
             // Loop through each destination row.
	     while (dst_clip_h--)
	       {
                 // Loop through each destination pixel in the current row.
		 while (dst_clip_w--)
		   {
                     // Cx: horizontal step in source pixels (fixed point 18.14).
                     // xap: initial weight for the first source pixel (fixed point 2.14).
		     Cx = *xapp >> 16;
		     xap = *xapp & 0xffff;
		     pix = *yp + *xp + pos;

                     // Accumulate weighted RGB values from the current source scanline.
                     // Alpha is assumed to be opaque (0xff).
		     r = (R_VAL(pix) * xap) >> 10;
		     g = (G_VAL(pix) * xap) >> 10;
		     b = (B_VAL(pix) * xap) >> 10;
                     // Loop over subsequent source pixels.
		     for (j = (1 << 14) - xap; j > Cx; j -= Cx)
		       {
			 pix++;
			 r += (R_VAL(pix) * Cx) >> 10;
			 g += (G_VAL(pix) * Cx) >> 10;
			 b += (B_VAL(pix) * Cx) >> 10;
		       }
                     // Handle the last partial contribution.
		     if (j > 0)
		       {
			 pix++;
			 r += (R_VAL(pix) * j) >> 10;
			 g += (G_VAL(pix) * j) >> 10;
			 b += (B_VAL(pix) * j) >> 10;
		       }
                     // If yap (vertical interpolation weight) is > 0, blend with the next scanline.
		     if ((yap = *yapp) > 0)
		       {
                         // Point to the corresponding pixel in the next source scanline.
			 pix = *yp + *xp + src_w + pos;
                         // Accumulate weighted RGB values from the next source scanline.
			 rr = (R_VAL(pix) * xap) >> 10;
			 gg = (G_VAL(pix) * xap) >> 10;
			 bb = (B_VAL(pix) * xap) >> 10;
			 for (j = (1 << 14) - xap; j > Cx; j -= Cx)
			   {
			     pix++;
			     rr += (R_VAL(pix) * Cx) >> 10;
			     gg += (G_VAL(pix) * Cx) >> 10;
			     bb += (B_VAL(pix) * Cx) >> 10;
			   }
			 if (j > 0)
			   {
			     pix++;
			     rr += (R_VAL(pix) * j) >> 10;
			     gg += (G_VAL(pix) * j) >> 10;
			     bb += (B_VAL(pix) * j) >> 10;
			   }
                         // Perform vertical interpolation.
			 r += ((rr - r) * yap) >> 8;
			 g += ((gg - g) * yap) >> 8;
			 b += ((bb - b) * yap) >> 8;
		       }
                     // Store the final pixel value with opaque alpha into the temporary buffer.
		     *pbuf++ = ARGB_JOIN(0xff,
					 ((r + (1 << 3)) >> 4),
					 ((g + (1 << 3)) >> 4),
					 ((b + (1 << 3)) >> 4));
		     xp++;  xapp++;
		   }

                 // After processing a row, apply mask and color multiplication if necessary.
                 if (!mask_ie) // No mask
                   func(buf, NULL, mul_col, dptr, w);
                 else // Apply mask
                   {
                      mask = mask_ie->image.data8
                         + ((dst_clip_y - mask_y + y) * mask_ie->cache_entry.w)
                         + (dst_clip_x - mask_x);

                      if (mul_col != 0xffffffff) func2(buf, NULL, mul_col, buf, w);
                      func(buf, mask, 0, dptr, w);
                   }
                 y++;

		 pbuf = buf;
		 dptr += dst_w;  dst_clip_w = w;
		 yp++;  yapp++;
		 xp = xpoints;// + dxx;
		 xapp = xapoints;// + dxx;
	       }
	  }
     }
}
