#include "vg_common.h"

/**
 * @file
 * @brief This file implements the Evas Vector Graphics (VG) saver module for the SVG format.
 * It converts the internal VG representation (Vg_File_Data) into an SVG file.
 */

/** @brief Log domain for the SVG saver module. */
static int _evas_vg_saver_svg_log_dom = -1;

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_vg_saver_svg_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_vg_saver_svg_log_dom, __VA_ARGS__)

/**
 * @brief Appends SVG style attributes to a string buffer based on the provided style properties.
 *
 * This function formats fill color, fill rule, fill opacity, stroke color,
 * stroke width, and stroke linecap attributes according to SVG standards.
 *
 * @param style Pointer to the Svg_Style_Property structure containing style information.
 * @param buf Pointer to the Eina_Strbuf where the SVG style attributes will be appended.
 */
static void
printf_style(Svg_Style_Property *style, Eina_Strbuf *buf)
{
   // Append fill color if any component is non-zero
   if ((style->fill.paint.r) || (style->fill.paint.g) || (style->fill.paint.b))
     eina_strbuf_append_printf(buf, " fill=\"#%02X%02X%02X\" ", style->fill.paint.r, style->fill.paint.g, style->fill.paint.b);
   // Append fill rule if it's even-odd
   if (style->fill.fill_rule == EFL_GFX_FILL_RULE_ODD_EVEN)
     eina_strbuf_append_printf(buf, " fill-rule=\"evenodd\" ");
   // Append fill opacity if not fully opaque
   if (style->fill.opacity != 255)
     eina_strbuf_append_printf(buf, " fill-opacity=\"%f\"", style->fill.opacity / 255.0);
   // Append stroke color if any component is non-zero
   if ((style->stroke.paint.r) || (style->stroke.paint.g) || (style->stroke.paint.b))
     eina_strbuf_append_printf(buf, " stroke=\"#%02X%02X%02X\" ", style->stroke.paint.r, style->stroke.paint.g, style->stroke.paint.b);
   // Append stroke width if non-zero
   if (EINA_DBL_NONZERO(style->stroke.width))
     eina_strbuf_append_printf(buf, " stroke-width=\"%f\" ", style->stroke.width);
   // Append stroke linecap style
   if (style->stroke.cap == EFL_GFX_CAP_ROUND)
     eina_strbuf_append_printf(buf, " stroke-linecap=\"round\" ");
   else if (style->stroke.cap == EFL_GFX_CAP_SQUARE)
     eina_strbuf_append_printf(buf, " stroke-linecap=\"square\" ");
}

/**
 * @brief Recursively traverses the Svg_Node tree and generates the corresponding SVG markup.
 *
 * This function handles different node types (document root, group, path)
 * and converts their properties (transformations, path commands, styles)
 * into SVG elements and attributes.
 *
 * @param parent The current Svg_Node being processed.
 * @param buf The Eina_Strbuf to append the generated SVG markup to.
 */
static void
_svg_node_printf(Svg_Node *parent, Eina_Strbuf *buf)
{
   int i = 0; // Loop counter for path commands
   double *points; // Pointer to the coordinate data for path commands
   double last_x = 0, last_y = 0; // Track the last point for relative commands
   Efl_Gfx_Path_Command *commands; // Array of path commands (e.g., MOVE_TO, LINE_TO)
   Eina_List *l; // Iterator for child nodes
   Svg_Node *data;

   switch (parent->type)
     {
      case SVG_NODE_DOC:
         // SVG Document Root Node
         eina_strbuf_append_printf(buf, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
         eina_strbuf_append_printf(buf, "<svg viewBox=\"%f %f %f %f\" xmlns=\"http://www.w3.org/2000/svg\""
                " xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n",
                parent->node.doc.vx, parent->node.doc.vy, // Viewbox origin (x, y)
                parent->node.doc.vw, parent->node.doc.vh); // Viewbox size (width, height)
         // Recursively process child nodes
         EINA_LIST_FOREACH(parent->child, l, data)
           {
              _svg_node_printf(data, buf);
           }
         eina_strbuf_append_printf(buf, "</svg>\n");
         break;
      case SVG_NODE_G: // SVG Group Node
         eina_strbuf_append_printf(buf, "<g");
         // Apply transformation matrix if present
         if (parent->transform)
           eina_strbuf_append_printf(buf,
                                     " transform=\"matrix(%f %f %f %f %f %f)\"", // matrix(xx, yx, xy, yy, xz, yz)
                                     parent->transform->xx,
                                     parent->transform->yx,
                                     parent->transform->xy,
                                     parent->transform->yy,
                                     parent->transform->xz,
                                     parent->transform->yz);
         eina_strbuf_append_printf(buf, ">\n");
         // Recursively process child nodes within the group
         EINA_LIST_FOREACH(parent->child, l, data)
           {
              _svg_node_printf(data, buf);
           }
         eina_strbuf_append_printf(buf, "</g>\n");
         break;
      case SVG_NODE_CUSTOME_COMMAND: // SVG Path Node (from custom commands)
         points = parent->node.command.points; // Array of coordinates
         commands = parent->node.command.commands; // Array of path commands
         eina_strbuf_append_printf(buf, "<path d=\""); // Start path data attribute
         // Iterate through path commands and generate SVG path data string
         for (i = 0; i < parent->node.command.commands_count; i++)
           {
              switch (commands[i])
                {
                 case EFL_GFX_PATH_COMMAND_TYPE_END: // End of path definition
                    eina_strbuf_append_printf(buf, "\""); // Close path data attribute
                    // Apply transformation matrix if present
                    if (parent->transform)
                      eina_strbuf_append_printf(buf, " transform=\"matrix(%f %f %f %f %f %f)\"",
                             parent->transform->xx,
                             parent->transform->yx, // matrix(xx, yx, xy, yy, xz, yz)
                             parent->transform->xy,
                             parent->transform->yy,
                             parent->transform->xz,
                             parent->transform->yz);
                    printf_style(parent->style, buf);
                    eina_strbuf_append_printf(buf, "/>\n");
                    // Append style attributes (fill, stroke, etc.)
                    printf_style(parent->style, buf);
                    eina_strbuf_append_printf(buf, "/>\n"); // Close path element
                    break;
                 case EFL_GFX_PATH_COMMAND_TYPE_MOVE_TO:
                    // 'm' command (relative move to)
                    // points array structure for MOVE_TO: [x, y]
                    eina_strbuf_append_printf(buf, "m%f,%f ", points[0] - last_x, points[1] - last_y);
                    last_x = points[0];
                    last_y = points[1];
                    points += 2; // Advance pointer past the 2 coordinates used
                    break;
                 case EFL_GFX_PATH_COMMAND_TYPE_LINE_TO:
                    // 'L' command (absolute line to)
                    // points array structure for LINE_TO: [x, y]
                    eina_strbuf_append_printf(buf, "L%f,%f ", points[0], points[1]);
                    last_x = points[0];
                    last_y = points[1];
                    points += 2; // Advance pointer past the 2 coordinates used
                    break;
                 case EFL_GFX_PATH_COMMAND_TYPE_CUBIC_TO:
                    // 'c' command (relative cubic bezier curve to)
                    // points array structure for CUBIC_TO: [ctrl_x1, ctrl_y1, ctrl_x2, ctrl_y2, end_x, end_y]
                    eina_strbuf_append_printf(buf, "c%f,%f %f,%f %f,%f ",
                           points[0] - last_x, points[1] - last_y, // Relative control point 1
                           points[2] - last_x, points[3] - last_y, // Relative control point 2
                           points[4] - last_x, points[5] - last_y); // Relative end point
                    last_x = points[4];
                    last_y = points[5];
                    points += 6; // Advance pointer past the 6 coordinates used
                    break;
                 case EFL_GFX_PATH_COMMAND_TYPE_CLOSE: // Close path command
                    // 'z' command (close path)
                    eina_strbuf_append_printf(buf, " z ");
                    break;
                 case EFL_GFX_PATH_COMMAND_TYPE_LAST: // Sentinel value, ignore
                 default: // Ignore unknown command types
                    break;
                }
           }
         break;
      default: // Ignore unknown command types
         break;
     }
}

/**
 * @brief Saves the vector graphics data to an SVG file.
 *
 * This is the main entry point for the SVG saver functionality exposed by the module.
 * It converts the internal Vg_File_Data representation into an Svg_Node tree,
 * generates the SVG markup using _svg_node_printf, and writes it to the specified file.
 *
 * @param evg_data Pointer to the internal vector graphics data structure.
 * @param file The path to the output SVG file.
 * @param key Unused parameter (part of the Evas saver function signature).
 * @param compress Unused parameter (part of the Evas saver function signature).
 * @return EVAS_LOAD_ERROR_NONE on success, or an appropriate error code on failure.
 */
Evas_Load_Error
evas_vg_save_file_svg(Vg_File_Data *evg_data, const char *file, const char *key EINA_UNUSED, int compress EINA_UNUSED)
{
   Eina_Strbuf *buf = NULL; // String buffer to build the SVG content
   Svg_Node *root; // Root of the intermediate SVG node tree
   FILE *f = fopen(file, "w+");
   if (!f)
     {
        ERR("Cannot open file '%s' for SVG save", file);
        return EVAS_LOAD_ERROR_GENERIC;
     }

   // Convert internal VG data to an SVG node tree
   root = vg_common_svg_create_svg_node(evg_data);
   // Create a string buffer for the SVG output
   buf = eina_strbuf_new();
   // Generate SVG markup from the node tree
   _svg_node_printf(root, buf);
   // Write the generated SVG string to the file
   fprintf(f, "%s\n", eina_strbuf_string_get(buf));
   // Clean up
   fclose(f);
   eina_strbuf_free(buf);

   return EVAS_LOAD_ERROR_NONE;
}

/** @brief Structure holding the function pointer for the SVG save operation. */
static Evas_Vg_Save_Func evas_vg_save_svg_func =
{
   evas_vg_save_file_svg // The function that performs the save operation
};

/**
 * @brief Initializes the Evas VG saver module for SVG.
 *
 * Called by Evas when the module is loaded. It registers the log domain
 * and sets the module's function pointer.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   // Assign the save function structure to the module
   em->functions = (void *)(&evas_vg_save_svg_func);
   // Register the log domain for this module
   _evas_vg_saver_svg_log_dom = eina_log_domain_register
     ("vg-save-svg", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_vg_saver_svg_log_dom < 0)
     {
        // Log an error if domain registration fails
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }
   return 1; // Success
}

/**
 * @brief Cleans up the Evas VG saver module for SVG.
 *
 * Called by Evas when the module is unloaded. Currently does nothing.
 * (Log domain unregistering is typically handled globally by Eina).
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   // No specific cleanup needed here for this module.
   // Log domain might be unregistered elsewhere if needed.
}

/** @brief Module API structure defining the module's properties and functions. */
static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "svg",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_VG_SAVER, vg_saver, svg);

#ifndef EVAS_STATIC_BUILD_VG_SVG
EVAS_EINA_MODULE_DEFINE(vg_saver, svg);
#endif
