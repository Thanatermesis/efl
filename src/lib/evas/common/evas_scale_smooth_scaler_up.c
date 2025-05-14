/**
 * @brief Scales an image region up using a smooth scaling algorithm.
 *
 * This function takes a source image region and scales it up to a destination
 * region. It handles various rendering operations (blend, copy) and can
 * optionally use an alpha mask. MMX and NEON optimizations are used if
 * available.
 *
 * @param src The source image surface.
 * @param dst The destination image surface.
 * @param src_region_x The x-coordinate of the top-left corner of the source region.
 * @param src_region_y The y-coordinate of the top-left corner of the source region.
 * @param src_region_w The width of the source region.
 * @param src_region_h The height of the source region.
 * @param dst_region_x The x-coordinate of the top-left corner of the destination region.
 * @param dst_region_y The y-coordinate of the top-left corner of the destination region.
 * @param dst_region_w The width of the destination region.
 * @param dst_region_h The height of the destination region.
 * @param dst_clip_x The x-coordinate of the top-left corner of the destination clipping region.
 * @param dst_clip_y The y-coordinate of the top-left corner of the destination clipping region.
 * @param dst_clip_w The width of the destination clipping region.
 * @param dst_clip_h The height of the destination clipping region.
 * @param mul_col The multiplication color. If 0xffffffff, no color multiplication is performed.
 * @param render_op The rendering operation to perform (e.g., _EVAS_RENDER_BLEND, _EVAS_RENDER_COPY).
 * @param mask_ie Optional alpha mask image.
 * @param mask_x The x-coordinate offset for the mask.
 * @param mask_y The y-coordinate offset for the mask.
 * @param dst_ptr Pointer to the destination image data, pre-offset to (dst_clip_x, dst_clip_y).
 * @param dst_w The width of the destination image (stride).
 * @param src_w The width of the source image (stride).
 */
{
   int         srx = src_region_x, sry = src_region_y;
   int         srw = src_region_w, srh = src_region_h;
   int         drx = dst_region_x, dry = dst_region_y;
   int         drw = dst_region_w, drh = dst_region_h;

   int         dsxx, dsyy, sxx, syy, sx, sy;
   int         cx, cy;
   int         direct_scale = 0, buf_step = 0;

   DATA32      *psrc, *pdst, *pdst_end;
   DATA32      *buf, *pbuf, *pbuf_end;
   DATA8       *mask;
   RGBA_Gfx_Func  func = NULL, func2 = NULL;

   /* Prevent excessively large scaling operations that could lead to overflow. */
   if ((src_region_w > SCALE_SIZE_MAX) ||
       (src_region_h > SCALE_SIZE_MAX)) return;

   /* Initialize destination pointer and end pointer for the clipping region. */
   pdst = dst_ptr;  // it's been set at (dst_clip_x, dst_clip_y)
   pdst_end = pdst + (dst_clip_h * dst_w);

   /* Determine if direct scaling (writing directly to destination buffer) is possible.
    * This is an optimization for specific cases:
    * - Blending an opaque source without color multiplication or an alpha mask.
    * - Copying a source without color multiplication or an alpha mask.
    */
   if (mul_col == 0xffffffff && !mask_ie)
     {
	if ((render_op == _EVAS_RENDER_BLEND) && !src->cache_entry.flags.alpha)
	  { direct_scale = 1;  buf_step = dst->cache_entry.w; }
	else if (render_op == _EVAS_RENDER_COPY)
	  {
	    direct_scale = 1;  buf_step = dst->cache_entry.w;
	    if (src->cache_entry.flags.alpha)
		dst->cache_entry.flags.alpha = 1;
	  }
     }
   /* If direct scaling is not possible, allocate a temporary line buffer
    * and select the appropriate compositing function.
    */
   if (!direct_scale)
     {
	buf = alloca(dst_clip_w * sizeof(DATA32)); // Temporary buffer for one scanline
        if (!mask_ie) // No alpha mask
          {
             if (mul_col != 0xffffffff) // With color multiplication
               func = evas_common_gfx_func_composite_pixel_color_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, mul_col, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
             else // No color multiplication
               func = evas_common_gfx_func_composite_pixel_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
          }
        else // With alpha mask
          {
             if (mul_col != 0xffffffff) // With color multiplication and alpha mask
               {
                  // First, composite with mask, then apply color
                  func = evas_common_gfx_func_composite_pixel_mask_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
                  func2 = evas_common_gfx_func_composite_pixel_color_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, mul_col, dst->cache_entry.flags.alpha, dst_clip_w, EVAS_RENDER_COPY);
               }
             else // With alpha mask, no color multiplication
               func = evas_common_gfx_func_composite_pixel_mask_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
          }
     }
   else // Direct scaling: use destination buffer directly
	buf = pdst;

   /* Calculate fixed-point increments for stepping through source pixels.
    * dsxx: source x increment per destination x pixel (16.16 fixed point).
    * dsyy: source y increment per destination y pixel (16.16 fixed point).
    */
   if ((srw > 1) && (drw > 1))
	dsxx = ((srw - 1) << 16) / (drw - 1);
   else
	dsxx = (srw << 16) / drw;
   if ((srh > 1) && (drh > 1))
	dsyy = ((srh - 1) << 16) / (drh - 1);
   else
	dsyy = (srh << 16) / drh;

   /* Calculate initial source coordinates (sxx, syy) for the top-left
    * of the destination clip region, adjusted by destination region offset.
    * cx, cy: offset from destination region to destination clip region.
    */
   cx = dst_clip_x - drx;
   cy = dst_clip_y - dry;

   sxx = (dsxx * cx);
   syy = (dsyy * cy);

   sy = syy >> 16; // Integer part of initial source y coordinate

   /* Optimized path for 1D vertical scaling (drh == srh).
    * Source height is the same as destination height.
    * Only horizontal scaling is performed.
    */
   if (drh == srh)
     {
	int  sxx0 = sxx; // Store initial sxx for each scanline
        int y = 0; // Counter for mask y-offset calculation
	psrc = src->image.data + (src_w * (sry + cy)) + srx; // Initial source pointer for the first scanline
	while (pdst < pdst_end) // Loop through destination scanlines
	  {
	    pbuf = buf;  pbuf_end = buf + dst_clip_w; // Initialize temporary buffer pointers
	    sxx = sxx0; // Reset sxx for the current scanline
#ifdef SCALE_USING_MMX
	    pxor_r2r(mm0, mm0); // Zero out mm0 (used for unpacking)
	    MOV_A2R(ALPHA_255, mm5) // Load 255 into mm5 (for alpha in interpolation)
#endif
	      while (pbuf < pbuf_end) // Loop through pixels in the scanline
		{
		  DATA32   p0, p1; // Source pixels for interpolation
		  int      ax;     // Horizontal interpolation factor (0-256)

		  sx = (sxx >> 16); // Integer part of source x coordinate
		  ax = 1 + ((sxx - (sx << 16)) >> 8); // Fractional part for interpolation
		  p0 = p1 = *(psrc + sx); // Get the primary source pixel
		  if ((sx + 1) < srw) // Check boundary for the second source pixel
		    p1 = *(psrc + sx + 1);
#ifdef SCALE_USING_MMX
		  MOV_P2R(p0, mm1, mm0) // Move p0 to mm1
		    if (p0 | p1) // Optimization: only interpolate if pixels are not both black
		      {
			MOV_A2R(ax, mm3) // Move interpolation factor to mm3
			MOV_P2R(p1, mm2, mm0) // Move p1 to mm2
			INTERP_256_R2R(mm3, mm2, mm1, mm5) // Interpolate: mm1 = (p1 * ax + p0 * (256-ax)) / 256
		       }
		  MOV_R2P(mm1, *pbuf, mm0) // Store result in buffer
		  pbuf++;
#else
		  if (p0 | p1) // Optimization: only interpolate if pixels are not both black
		    p0 = INTERP_256(ax, p1, p0); // Standard C interpolation
		  *pbuf++ = p0; // Store result in buffer
#endif
		  sxx += dsxx; // Increment source x coordinate
		}
	    /* Blend the processed scanline from 'buf' to the destination 'pdst'. */
	    if (!direct_scale)
              {
                 if (!mask_ie) // No alpha mask
                   func(buf, NULL, mul_col, pdst, dst_clip_w);
                 else // With alpha mask
                   {
                      mask = mask_ie->image.data8 // Calculate mask pointer for current line
                         + ((dst_clip_y - mask_y + y) * mask_ie->cache_entry.w)
                         + (dst_clip_x - mask_x);

                      if (mul_col != 0xffffffff) func2(buf, NULL, mul_col, buf, dst_clip_w); // Apply color if needed (func2 is EVAS_RENDER_COPY)
                      func(buf, mask, 0, pdst, dst_clip_w); // Composite with mask
                   }
                 y++; // Increment y for mask calculation
              }

	    pdst += dst_w; // Move to the next destination scanline
	    psrc += src_w; // Move to the next source scanline (since drh == srh)
	    buf += buf_step; // Advance buffer pointer if direct_scale is on (buf_step is dst_w)
	  }

	goto done_scale_up; // Scaling finished for this case
     }
   /* Optimized path for 1D horizontal scaling (drw == srw).
    * Source width is the same as destination width.
    * Only vertical scaling is performed.
    */
   else if (drw == srw)
     {
	DATA32  *ps = src->image.data + (src_w * sry) + srx + cx; // Base source pointer for the column
        int y = 0; // Counter for mask y-offset calculation

	while (pdst < pdst_end) // Loop through destination scanlines
	  {
	    int        ay; // Vertical interpolation factor (0-256)

	    sy = syy >> 16; // Integer part of source y coordinate
	    psrc = ps + (sy * src_w); // Source pointer for the current line to interpolate from
	    ay = 1 + ((syy - (sy << 16)) >> 8); // Fractional part for vertical interpolation
#ifdef SCALE_USING_MMX
	    pxor_r2r(mm0, mm0);
	    MOV_A2R(ALPHA_255, mm5)
	    MOV_A2R(ay, mm4) // Load vertical interpolation factor into mm4
#endif
	    pbuf = buf;  pbuf_end = buf + dst_clip_w; // Initialize temporary buffer pointers
	    while (pbuf < pbuf_end) // Loop through pixels in the scanline
	      {
		DATA32  p0 = *psrc, p2 = p0; // Source pixels for vertical interpolation (p0 from current line, p2 from next line)

		if ((sy + 1) < srh) // Check boundary for the second source pixel (from next line)
		  p2 = *(psrc + src_w);
#ifdef SCALE_USING_MMX
		MOV_P2R(p0, mm1, mm0)
		if (p0 | p2)
		  {
		    MOV_P2R(p2, mm2, mm0)
		    INTERP_256_R2R(mm4, mm2, mm1, mm5) // Interpolate vertically
		  }
		MOV_R2P(mm1, *pbuf, mm0)
		pbuf++;
#else
		if (p0 | p2)
		  p0 = INTERP_256(ay, p2, p0); // Standard C vertical interpolation
		*pbuf++ = p0;
#endif
		psrc++; // Move to next pixel in the source line (since drw == srw)
	      }
	    /* Blend the processed scanline from 'buf' to the destination 'pdst'. */
	    if (!direct_scale)
              {
                 if (!mask_ie)
                   func(buf, NULL, mul_col, pdst, dst_clip_w);
                 else
                   {
                      mask = mask_ie->image.data8
                         + ((dst_clip_y - mask_y + y) * mask_ie->cache_entry.w)
                         + (dst_clip_x - mask_x);

                      if (mul_col != 0xffffffff) func2(buf, NULL, mul_col, buf, dst_clip_w);
                      func(buf, mask, 0, pdst, dst_clip_w);
                   }
                 y++;
              }
	    pdst += dst_w; // Move to the next destination scanline
	    syy += dsyy;   // Increment source y coordinate
	    buf += buf_step; // Advance buffer pointer if direct_scale is on
	  }
	goto done_scale_up; // Scaling finished for this case
     }

     /* General 2D scaling path (bilinear interpolation).
      * This handles cases where both width and height are scaled.
      */
     {
	DATA32  *ps = src->image.data + (src_w * sry) + srx; // Base source pointer
	int     sxx0 = sxx; // Store initial sxx for each scanline
        int     y = 0;    // Counter for mask y-offset calculation

	while (pdst < pdst_end) // Loop through destination scanlines
	  {
	    int   ay; // Vertical interpolation factor

	    sy = syy >> 16; // Integer part of source y coordinate
	    psrc = ps + (sy * src_w); // Source pointer for the current row of pixels
	    ay = 1 + ((syy - (sy << 16)) >> 8); // Fractional part for vertical interpolation
#ifdef SCALE_USING_MMX
	    MOV_A2R(ay, mm4) // Load vertical interpolation factor
	    pxor_r2r(mm0, mm0);
	    MOV_A2R(ALPHA_255, mm5)
#elif defined SCALE_USING_NEON
            uint16x8_t vay = vdupq_n_u16(ay); // NEON: duplicate 'ay' for vector operations
#endif
	    pbuf = buf;  pbuf_end = buf + dst_clip_w; // Initialize temporary buffer pointers
	    sxx = sxx0; // Reset sxx for the current scanline
#ifdef SCALE_USING_NEON
            /* NEON optimization processes 2 pixels per iteration.
             * pa[2][4] holds source pixels for two destination pixels:
             * For 1st dest pixel: pa[0][0]=p0, pa[0][1]=p1, pa[0][2]=p2, pa[0][3]=p3
             * For 2nd dest pixel: pa[1][0]=p0', pa[1][1]=p1', pa[1][2]=p2', pa[1][3]=p3'
             * where p0,p1 are from current source line, p2,p3 are from next source line.
             */
	    while (pbuf+1 < pbuf_end) // 2 iterations only for NEON
#else
	    while (pbuf < pbuf_end) // Loop through pixels in the scanline
#endif
	      {
		int     ax;     // Horizontal interpolation factor
		DATA32  *p, *q; // Pointers to source pixels (current and next line)
#ifdef SCALE_USING_NEON
                int     ax1;    // Horizontal interpolation factor for the second pixel (NEON)
                DATA32  *p1_neon, *q1_neon; // Renamed to avoid conflict with p1 in non-NEON block
                uint32x2x2_t vp0, vp1; // NEON vectors for pixel data
                uint16x8_t vax;        // NEON vector for ax
                uint16x8_t vax1;       // NEON vector for ax1
                DATA32 pa[2][4];       // Array to hold 4 source pixels for 2 destination pixels
#else
		DATA32  p0, p1, p2, p3; // Source pixels (p0,p1 current line; p2,p3 next line)
#endif

		sx = sxx >> 16; // Integer part of source x coordinate
		ax = 1 + ((sxx - (sx << 16)) >> 8); // Fractional part for horizontal interpolation
		p = psrc + sx;  // Pointer to p0 = *(psrc + sx)
                q = p + src_w;  // Pointer to p2 = *(psrc + sx + src_w)
#ifdef SCALE_USING_NEON
                // Load source pixels for the first destination pixel
                pa[0][0] = pa[0][1] = pa[0][2] = pa[0][3] = *p; // Initialize with p0
                if ((sx + 1) < srw) // Boundary check for p1
                  pa[0][1] = *(p + 1);
                if ((sy + 1) < srh) // Boundary check for p2, p3
                  {
                    pa[0][2] = *q;  pa[0][3] = pa[0][2]; // Initialize p3 with p2
                    if ((sx + 1) < srw) // Boundary check for p3
                      pa[0][3] = *(q + 1);
                  }
                vax = vdupq_n_u16(ax); // Duplicate ax for vector operation
                vp0.val[0] = vld1_u32(&pa[0][0]); // Load {p0, p1} (actually loads 2xDATA32, so {p0,p1} if aligned)
                vp0.val[1] = vld1_u32(&pa[0][2]); // Load {p2, p3}
                sxx += dsxx; // Increment source x for next destination pixel

                // Load source pixels for the second destination pixel
                sx = sxx >> 16;
                ax1 = 1 + ((sxx - (sx << 16)) >> 8);
                p1_neon = psrc + sx; q1_neon = p1_neon + src_w; // Use distinct names
                pa[1][0] = pa[1][1] = pa[1][2] = pa[1][3] = *p1_neon;
                if ((sx + 1) < srw)
                  pa[1][1] = *(p1_neon + 1);
                if ((sy + 1) < srh)
                  {
                    pa[1][2] = *q1_neon;  pa[1][3] = pa[1][2];
                    if ((sx + 1) < srw)
                      pa[1][3] = *(q1_neon + 1);
                  }
                vax1 = vdupq_n_u16(ax1);
                vp1.val[0] = vld1_u32(&pa[1][0]);
                vp1.val[1] = vld1_u32(&pa[1][2]);
#else
		p0 = p1 = p2 = p3 = *p; // Initialize with p0
		if ((sx + 1) < srw) // Boundary check for p1
		  p1 = *(p + 1);
		if ((sy + 1) < srh) // Boundary check for p2, p3
		  {
		    p2 = *q;  p3 = p2; // Initialize p3 with p2
		    if ((sx + 1) < srw) // Boundary check for p3
		      p3 = *(q + 1);
		  }
#endif
#ifdef SCALE_USING_MMX
                /* MMX Bilinear Interpolation:
                 * 1. Interpolate (p0, p1) using ax -> res1
                 * 2. Interpolate (p2, p3) using ax -> res2
                 * 3. Interpolate (res1, res2) using ay -> final_pixel
                 */
		MOV_A2R(ax, mm6) // Load horizontal interpolation factor
		MOV_P2R(p0, mm1, mm0) // mm1 = p0
		if (p0 | p1) // Optimization
		  {
		    MOV_P2R(p1, mm2, mm0) // mm2 = p1
		    INTERP_256_R2R(mm6, mm2, mm1, mm5) // mm1 = interp(ax, p1, p0)
		  }
		MOV_P2R(p2, mm2, mm0) // mm2 = p2
		if (p2 | p3) // Optimization
		  {
		    MOV_P2R(p3, mm3, mm0) // mm3 = p3
		    INTERP_256_R2R(mm6, mm3, mm2, mm5) // mm2 = interp(ax, p3, p2)
		  }
		INTERP_256_R2R(mm4, mm2, mm1, mm5) // mm1 = interp(ay, result2, result1)
		MOV_R2P(mm1, *pbuf, mm0) // Store final pixel
		pbuf++;
#elif defined SCALE_USING_NEON
                    /* NEON Bilinear Interpolation (processes two output pixels):
                     * Step 1: Horizontal interpolation for two sets of pixels
                     *   - Interpolate (pa[0][0], pa[0][1]) using ax  -> h_interp0_top
                     *   - Interpolate (pa[0][2], pa[0][3]) using ax  -> h_interp0_bottom
                     *   - Interpolate (pa[1][0], pa[1][1]) using ax1 -> h_interp1_top
                     *   - Interpolate (pa[1][2], pa[1][3]) using ax1 -> h_interp1_bottom
                     *   This is done by:
                     *   vzip rearranges {p0,p1,p2,p3} to {p0,p2,p1,p3}
                     *   vsubl calculates {p1-p0, p3-p2} (widened to u16)
                     *   vmulq multiplies by ax
                     *   vshrn shifts right by 8 (division by 256)
                     *   vadd adds back to {p0,p2}
                     *   Result: vp0.val[0] contains {h_interp0_top, h_interp0_bottom}
                     *           vp1.val[0] contains {h_interp1_top, h_interp1_bottom}
                     *
                     * Step 2: Vertical interpolation
                     *   - Interpolate (h_interp0_top, h_interp0_bottom) using ay -> final_pixel0
                     *   - Interpolate (h_interp1_top, h_interp1_bottom) using ay -> final_pixel1
                     *   This is done similarly:
                     *   vzip rearranges to {h_interp0_top, h_interp1_top, h_interp0_bottom, h_interp1_bottom}
                     *   vsubl calculates differences
                     *   vmulq multiplies by vay (vector of ay)
                     *   vshrn shifts
                     *   vadd adds back
                     *   Result: vp0.val[0] contains {final_pixel0, final_pixel1}
                     */
                    // Horizontal interpolation for first set of 4 pixels (p0,p1,p2,p3 for first output pixel)
                vp0 = vzip_u32(vp0.val[0], vp0.val[1]); // {p0,p1},{p2,p3} -> {p0,p2},{p1,p3}
                uint16x8_t vtmpq = vsubl_u8(vreinterpret_u8_u32(vp0.val[1]), vreinterpret_u8_u32(vp0.val[0])); // (p1-p0), (p3-p2)
                vp0.val[0] = vreinterpret_u32_u8(vadd_u8(vreinterpret_u8_u32(vp0.val[0]), vshrn_n_u16(vmulq_u16(vtmpq, vax), 8))); // p0 + (p1-p0)*ax; p2 + (p3-p2)*ax
                // Horizontal interpolation for second set of 4 pixels (for second output pixel)
                vp1 = vzip_u32(vp1.val[0], vp1.val[1]);
                vtmpq = vsubl_u8(vreinterpret_u8_u32(vp1.val[1]), vreinterpret_u8_u32(vp1.val[0]));
                vp1.val[0] = vreinterpret_u32_u8(vadd_u8(vreinterpret_u8_u32(vp1.val[0]), vshrn_n_u16(vmulq_u16(vtmpq, vax1), 8)));
                // Vertical interpolation across the two horizontally interpolated results
                vp0 = vzip_u32(vp0.val[0], vp1.val[0]); // {h_interp0_top,h_interp0_bottom},{h_interp1_top,h_interp1_bottom} -> {h_interp0_top,h_interp1_top},{h_interp0_bottom,h_interp1_bottom}
                vtmpq = vsubl_u8(vreinterpret_u8_u32(vp0.val[1]), vreinterpret_u8_u32(vp0.val[0])); // (h_interp0_bottom - h_interp0_top), (h_interp1_bottom - h_interp1_top)
                vp0.val[0] = vreinterpret_u32_u8(vadd_u8(vreinterpret_u8_u32(vp0.val[0]), vshrn_n_u16(vmulq_u16(vtmpq, vay), 8))); // final_pixel0, final_pixel1
                vst1_u32(pbuf, vp0.val[0]); // Store 2 resulting pixels
                pbuf += 2;
#else
                /* Standard C Bilinear Interpolation:
                 * 1. Interpolate (p0, p1) using ax -> p0'
                 * 2. Interpolate (p2, p3) using ax -> p2'
                 * 3. Interpolate (p0', p2') using ay -> final_pixel
                 */
		if (p0 | p1) // Optimization
		  p0 = INTERP_256(ax, p1, p0); // Horizontal interp for current line
		if (p2 | p3) // Optimization
		  p2 = INTERP_256(ax, p3, p2); // Horizontal interp for next line
		if (p0 | p2) // Optimization
		  p0 = INTERP_256(ay, p2, p0); // Vertical interp between the two results
		*pbuf++ = p0; // Store final pixel
#endif
		sxx += dsxx; // Increment source x coordinate
	      }
#if defined SCALE_USING_NEON
              /* Handle remaining pixel if dst_clip_w is odd.
               * This part uses standard C bilinear interpolation for the last pixel.
               */
              if (pbuf < pbuf_end) // For non-even length case
                {
                  int     ax_rem; // Renamed to avoid conflict
                  DATA32  *p_rem, *q_rem; // Renamed
                  DATA32  p0_rem, p1_rem, p2_rem, p3_rem; // Renamed

                  sx = sxx >> 16;
                  ax_rem = 1 + ((sxx - (sx << 16)) >> 8);
                  p_rem = psrc + sx;  q_rem = p_rem + src_w;
                  p0_rem = p1_rem = p2_rem = p3_rem = *p_rem;
                  if ((sx + 1) < srw)
                    p1_rem = *(p_rem + 1);
                  if ((sy + 1) < srh)
                    {
                       p2_rem = *q_rem;  p3_rem = p2_rem;
                       if ((sx + 1) < srw)
                         p3_rem = *(q_rem + 1);
                    }
                  if (p0_rem | p1_rem)
                    p0_rem = INTERP_256(ax_rem, p1_rem, p0_rem);
                  if (p2_rem | p3_rem)
                    p2_rem = INTERP_256(ax_rem, p3_rem, p2_rem);
                  if (p0_rem | p2_rem)
                    p0_rem = INTERP_256(ay, p2_rem, p0_rem);
                  *pbuf++ = p0_rem;
                  sxx += dsxx;
                }
#endif
	    /* Blend the processed scanline from 'buf' to the destination 'pdst'. */
	    if (!direct_scale)
              {
                 if (!mask_ie)
                   func(buf, NULL, mul_col, pdst, dst_clip_w);
                 else
                   {
                      mask = mask_ie->image.data8
                         + ((dst_clip_y - mask_y + y) * mask_ie->cache_entry.w)
                         + (dst_clip_x - mask_x);

                      if (mul_col != 0xffffffff) func2(buf, NULL, mul_col, buf, dst_clip_w);
                      func(buf, mask, 0, pdst, dst_clip_w);
                   }
                 y++;
              }

	    pdst += dst_w; // Move to the next destination scanline
	    syy += dsyy;   // Increment source y coordinate
	    buf += buf_step; // Advance buffer pointer if direct_scale is on
	  }
     }
   done_scale_up: // Label for goto statements, indicating scaling is complete
   return;
}
