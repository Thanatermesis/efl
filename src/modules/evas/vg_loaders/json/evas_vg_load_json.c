#include <rlottie_capi.h>
#include "vg_common.h"

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_vg_loader_json_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_vg_loader_json_log_dom, __VA_ARGS__)

static int _evas_vg_loader_json_log_dom = -1;

/**
 * @brief Closes a Vg_File_Data object and frees associated resources.
 *
 * This function releases the Lottie animation data, animation metadata (including markers),
 * the root EFL object, and the Vg_File_Data structure itself.
 *
 * @param vfd Pointer to the Vg_File_Data to close.
 * @return EINA_TRUE on success, EINA_FALSE if vfd is NULL.
 */
static Eina_Bool
evas_vg_load_file_close_json(Vg_File_Data *vfd)
{
   if (!vfd) return EINA_FALSE;

   Lottie_Animation *lot_anim = (Lottie_Animation *) vfd->loader_data;
   lottie_animation_destroy(lot_anim);
   if (vfd->anim_data)
     {
        if (vfd->anim_data->markers)
          {
             Vg_File_Anim_Data_Marker *marker;
             EINA_INARRAY_FOREACH(vfd->anim_data->markers, marker)
                if (marker->name) eina_stringshare_del(marker->name);
             eina_inarray_free(vfd->anim_data->markers);
          }
        free(vfd->anim_data);
     }
   if (vfd->root) efl_unref(vfd->root);
   free(vfd);

   return EINA_TRUE;
}

/**
 * @brief Creates the Efl_Vg_Node structure from the loaded Lottie animation data.
 *
 * This function delegates to vg_common_json_create_vg_node to parse the
 * JSON data (already loaded into vfd->loader_data by evas_vg_load_file_open_json)
 * and construct the vector graphics scene graph.
 *
 * @param vfd Pointer to the Vg_File_Data containing the loaded Lottie animation.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
evas_vg_load_file_data_json(Vg_File_Data *vfd)
{
   return vg_common_json_create_vg_node(vfd);
}

/**
 * @brief Opens a Lottie JSON file and prepares it for rendering.
 *
 * This function loads a Lottie animation from the given Eina_File.
 * It handles both regular files and files mapped into virtual memory.
 * It extracts animation properties like duration, frame count, and markers.
 * The actual parsing of JSON into a scene graph is deferred to
 * evas_vg_load_file_data_json.
 *
 * @param file Pointer to the Eina_File object representing the Lottie JSON file.
 * @param key Optional key, often the filename, used by lottie_animation_from_data.
 * @param error Pointer to an integer to store error codes (currently unused).
 * @return A pointer to a Vg_File_Data structure on success, or NULL on failure.
 *         The Vg_File_Data structure contains:
 *         - loader_data: Pointer to the Lottie_Animation object.
 *         - anim_data: Pointer to Vg_File_Anim_Data if the Lottie file is animated.
 *           - duration: Total duration of the animation in seconds.
 *           - frame_cnt: Total number of frames in the animation.
 *           - markers: An Eina_Inarray of Vg_File_Anim_Data_Marker.
 *             Each marker has:
 *             - name: eina_stringshare_add'ed name of the marker.
 *             - startframe: Starting frame number of the marker.
 *             - endframe: Ending frame number of the marker.
 *         - w: Default width of the animation.
 *         - h: Default height of the animation.
 */
static Vg_File_Data*
evas_vg_load_file_open_json(Eina_File *file,
                            const char *key,
                            int *error EINA_UNUSED)
{
   Vg_File_Data *vfd = calloc(1, sizeof(Vg_File_Data));
   if (!vfd) return NULL;

   Lottie_Animation *lot_anim = NULL;

   //Edje may use virtual memory.
   if (eina_file_virtual(file))
     {
        const char *data = (const char*) eina_file_map_all(file, EINA_FILE_SEQUENTIAL);
        if (!data) goto err;
        //@TODO pass corrct external_resource path.
        lot_anim = lottie_animation_from_data(data, key ? key:eina_file_filename_get(file), " ");
        eina_file_map_free(file, (void *) data);
     }
   else
     lot_anim = lottie_animation_from_file(eina_file_filename_get(file));

   if (!lot_anim)
     {
        WRN("Failed lottie_animation_from_file()");
        goto err;
     }

   unsigned int frame_cnt = lottie_animation_get_totalframe(lot_anim);

   //Support animation
   if (frame_cnt > 1)
     {
        vfd->anim_data = calloc(1, sizeof(Vg_File_Anim_Data));
        if (!vfd->anim_data) goto err;
        vfd->anim_data->duration = lottie_animation_get_duration(lot_anim);
        vfd->anim_data->frame_cnt = frame_cnt;

        // marker information
        const LOTMarkerList *markerlist = lottie_animation_get_markerlist(lot_anim);
        if (markerlist && markerlist->size > 0)
          {
             Vg_File_Anim_Data_Marker *marker;
             int i = 0;
             vfd->anim_data->markers = eina_inarray_new(sizeof(Vg_File_Anim_Data_Marker), 0);
             eina_inarray_resize(vfd->anim_data->markers, markerlist->size);
             EINA_INARRAY_FOREACH(vfd->anim_data->markers, marker)
               {
                  marker->name = eina_stringshare_add(markerlist->ptr[i].name);
                  marker->startframe = markerlist->ptr[i].startframe;
                  marker->endframe = markerlist->ptr[i].endframe;
                  i++;
               }
          }
     }

   //default size
   size_t w, h;
   lottie_animation_get_size(lot_anim, &w, &h);
   vfd->w = (int) w;
   vfd->h = (int) h;

   vfd->loader_data = (void *) lot_anim;

   return vfd;

err:
   if (vfd)
     {
        if (vfd->anim_data) free(vfd->anim_data);
        free(vfd);
     }
   if (lot_anim) lottie_animation_destroy(lot_anim);

   return NULL;
}

static Evas_Vg_Load_Func evas_vg_load_json_func =
{
   evas_vg_load_file_open_json,
   evas_vg_load_file_close_json,
   evas_vg_load_file_data_json
};

/**
 * @brief Initializes the Evas VG loader module for JSON (Lottie) files.
 *
 * This function is called when the module is loaded. It sets up the
 * loader functions and registers a log domain for the module.
 *
 * @param em Pointer to the Evas_Module structure.
 * @return 1 on success, 0 on failure.
 */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;
   em->functions = (void *)(&evas_vg_load_json_func);
   _evas_vg_loader_json_log_dom = eina_log_domain_register
     ("vg-load-json", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_vg_loader_json_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }
   return 1;
}

/**
 * @brief Shuts down the Evas VG loader module for JSON (Lottie) files.
 *
 * This function is called when the module is unloaded. It unregisters
 * the log domain.
 *
 * @param em Pointer to the Evas_Module structure (unused).
 */
static void
module_close(Evas_Module *em EINA_UNUSED)
{
   if (_evas_vg_loader_json_log_dom >= 0)
     {
        eina_log_domain_unregister(_evas_vg_loader_json_log_dom);
        _evas_vg_loader_json_log_dom = -1;
     }
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "json",
   "none",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_VG_LOADER, vg_loader, json);

#ifndef EVAS_STATIC_BUILD_VG_JSON
EVAS_EINA_MODULE_DEFINE(vg_loader, json);
#endif

