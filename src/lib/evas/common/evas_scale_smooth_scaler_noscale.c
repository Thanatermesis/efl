{
   DATA32 *ptr;
   RGBA_Gfx_Func func;

   // Calculate the starting pointer in the source image buffer.
   // This maps the destination clip coordinates to the source image coordinates,
   // considering the source and destination region offsets.
   // (dst_clip_y - dst_region_y + src_region_y) gives the y-offset in source pixels.
   // (dst_clip_x - dst_region_x + src_region_x) gives the x-offset in source pixels.
   ptr = src->image.data + ((dst_clip_y - dst_region_y + src_region_y) * src_w) + (dst_clip_x - dst_region_x) + src_region_x;

   // Select the appropriate graphics function based on whether a color multiplication
   // is needed. 0xffffffff typically means no color modulation (opaque white).
   if (mul_col != 0xffffffff)
     // Get the function for compositing a span of pixels with color multiplication.
     // Parameters typically include source alpha, source alpha sparsity, multiplication color,
     // destination alpha, span width, and render operation.
     func = evas_common_gfx_func_composite_pixel_color_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, mul_col, dst->cache_entry.flags.alpha, dst_clip_w, render_op);
   else
     // Get the function for compositing a span of pixels without color multiplication.
     func = evas_common_gfx_func_composite_pixel_span_get(src->cache_entry.flags.alpha, src->cache_entry.flags.alpha_sparse, dst->cache_entry.flags.alpha, dst_clip_w, render_op);

   // Loop through each row of the destination clip area.
   while (dst_clip_h--)
     {
        // Call the selected graphics function to process one horizontal span (row) of pixels.
        // - ptr: Pointer to the current row in the source image.
        // - NULL: Second source pointer, unused in this specific span function.
        // - mul_col: Multiplication color.
        // - dst_ptr: Pointer to the current row in the destination image.
        // - dst_clip_w: Width of the span to render (destination clip width).
        func(ptr, NULL, mul_col, dst_ptr, dst_clip_w);

        // Advance source pointer to the next row in the source image.
        ptr += src_w;
        // Advance destination pointer to the next row in the destination image.
        dst_ptr += dst_w;
     }
}

/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/
