#include "vg_common.h"

static int _evas_vg_loader_eet_log_dom = -1;

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_vg_loader_eet_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_vg_loader_eet_log_dom, __VA_ARGS__)

/**
 * @brief Opens an Eet file and loads vector graphics data associated with a key.
 *
 * This function memory-maps the provided Eet file, reads an Svg_Node structure
 * identified by the given key using the appropriate Eet data descriptor,
 * converts the Svg_Node into a Vg_File_Data structure suitable for Evas,
 * and then cleans up the intermediate Svg_Node.
 *
 * @param file An Eina_File handle representing the Eet file to load.
 * @param key The key identifying the specific vector graphics data within the Eet file.
 * @param error Pointer to an integer where the Evas load error code will be stored.
 *              Possible values include EVAS_LOAD_ERROR_NONE, EVAS_LOAD_ERROR_CORRUPT_FILE,
 *              EVAS_LOAD_ERROR_GENERIC.
 * @return A pointer to a Vg_File_Data structure containing the loaded vector graphics,
 *         or NULL on failure. The caller is responsible for freeing this structure
 *         using evas_vg_load_file_close_eet().
 */
static Vg_File_Data*
evas_vg_load_file_open_eet(Eina_File *file, const char *key, int *error EINA_UNUSED)
{
   Eet_Data_Descriptor *svg_node_eet;
   Svg_Node *node;
   Eet_File *ef = eet_mmap(file);
   if (!ef)
     {
        *error = EVAS_LOAD_ERROR_CORRUPT_FILE;
        return NULL;
     }

   svg_node_eet = vg_common_svg_node_eet();
   node = eet_data_read(ef, svg_node_eet, key);
   eet_close(ef);

   if (!node)
     {
        *error = EVAS_LOAD_ERROR_GENERIC;
     }
   else
     {
        *error = EVAS_LOAD_ERROR_NONE;
     }
   Vg_File_Data *vg_data = vg_common_svg_create_vg_node(node);
   vg_common_svg_node_free(node);
   return vg_data;
}

/**
 * @brief Closes and frees resources associated with loaded Eet vector graphics data.
 *
 * This function unreferences the root Efl_VG node and frees the Vg_File_Data
 * container structure.
 *
 * @param vfd Pointer to the Vg_File_Data structure obtained from
 *            evas_vg_load_file_open_eet().
 * @return EINA_TRUE on success, EINA_FALSE if vfd is NULL.
 */
static Eina_Bool
evas_vg_load_file_close_eet(Vg_File_Data *vfd)
{
   if (!vfd) return EINA_FALSE;

   if (vfd->root) efl_unref(vfd->root);
   free(vfd);

   return EINA_TRUE;
}

/**
 * @brief Placeholder function for loading additional data (unused in this loader).
 *
 * This function is part of the Evas_Vg_Load_Func interface but does not
 * perform any operations for the Eet loader, as all necessary data is
 * loaded during the open phase.
 *
 * @param vfd Pointer to the Vg_File_Data structure.
 * @return Always returns EINA_TRUE.
 */
static Eina_Bool
evas_vg_load_file_data_eet(Vg_File_Data *vfd EINA_UNUSED)
{
   return EINA_TRUE;
}

static Evas_Vg_Load_Func evas_vg_load_eet_func =
{
   evas_vg_load_file_open_eet,
   evas_vg_load_file_close_eet,
   evas_vg_load_file_data_eet
};

/**
 * @brief Initializes the Evas VG loader module for Eet files.
 *
 * Sets up the module functions pointer and registers the log domain
 * for this loader.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_vg_load_eet_func);
   _evas_vg_loader_eet_log_dom = eina_log_domain_register
     ("vg-load-eet", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_vg_loader_eet_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }
   return 1;
}

/**
 * @brief Cleans up the Evas VG loader module for Eet files.
 *
 * Unregisters the log domain used by the module.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_vg_loader_eet_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_vg_loader_eet_log_dom);
        _evas_vg_loader_eet_log_dom = -1;
     }
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "eet",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_VG_LOADER, vg_loader, eet);

#ifndef EVAS_STATIC_BUILD_VG_EET
EVAS_EINA_MODULE_DEFINE(vg_loader, eet);
#endif
