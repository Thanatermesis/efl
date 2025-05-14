#include "vg_common.h"

static int _evas_vg_saver_eet_log_dom = -1;

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_vg_saver_eet_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_vg_saver_eet_log_dom, __VA_ARGS__)

/**
 * @brief Saves vector graphics data to an EET file.
 *
 * This function takes the vector graphics data represented by @p evg_data,
 * converts it into an Svg_Node structure, and then saves this structure
 * into an EET file specified by @p file under the given @p key.
 * Compression can be enabled via the @p compress flag.
 *
 * @param evg_data The vector graphics data to save.
 * @param file The path to the output EET file.
 * @param key The key under which to store the data within the EET file.
 * @param compress A flag indicating whether to compress the data (1 for compression, 0 otherwise).
 * @return EVAS_LOAD_ERROR_NONE on success, or an Evas_Load_Error code on failure.
 */
Evas_Load_Error
evas_vg_save_file_eet(Vg_File_Data *evg_data, const char *file, const char *key, int compress)
{
   Eet_Data_Descriptor *svg_node_eet;
   Svg_Node *root;
   Eet_File *ef;

   ef = eet_open(file, EET_FILE_MODE_WRITE);
   if (!ef)
     return EVAS_LOAD_ERROR_GENERIC;

   svg_node_eet = vg_common_svg_node_eet();
   root = vg_common_svg_create_svg_node(evg_data);
   eet_data_write(ef, svg_node_eet, key, root, compress);
   eet_close(ef);

   vg_common_svg_node_free(root);

   return EVAS_LOAD_ERROR_NONE;
}

/**
 * @brief Structure holding the function pointer for the EET save operation.
 */
static Evas_Vg_Save_Func evas_vg_save_eet_func =
{
   evas_vg_save_file_eet
};

/**
 * @brief Initializes the Evas VG saver module for the EET format.
 *
 * Sets up the module functions and registers the log domain.
 *
 * @param em The Evas module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_vg_save_eet_func);
   _evas_vg_saver_eet_log_dom = eina_log_domain_register
     ("vg-save-eet", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_vg_saver_eet_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }
   return 1;
}

/**
 * @brief Cleans up the Evas VG saver module.
 *
 * Currently, this function does nothing but is required by the module API.
 *
 * @param em The Evas module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   // Currently no cleanup needed, log domain is unregistered automatically.
}

/**
 * @brief Defines the Evas module API for the EET VG saver.
 */
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

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_VG_SAVER, vg_saver, eet);

#ifndef EVAS_STATIC_BUILD_VG_EET
EVAS_EINA_MODULE_DEFINE(vg_saver, eet);
#endif
