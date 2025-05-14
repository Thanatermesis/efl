#include "ecore_drm2_private.h"

#define INSIDE(x, y, xx, yy, ww, hh) \
   (((x) < ((xx) + (ww))) && ((y) < ((yy) + (hh))) && \
       ((x) >= (xx)) && ((y) >= (yy)))

#define EDID_DESCRIPTOR_ALPHANUMERIC_DATA_STRING 0xfe
#define EDID_DESCRIPTOR_DISPLAY_PRODUCT_NAME 0xfc
#define EDID_DESCRIPTOR_DISPLAY_PRODUCT_SERIAL_NUMBER 0xff
#define EDID_OFFSET_DATA_BLOCKS 0x36
#define EDID_OFFSET_LAST_BLOCK 0x6c
#define EDID_OFFSET_PNPID 0x08
#define EDID_OFFSET_SERIAL 0x0c

static const char *conn_types[] =
{
   "None", "VGA", "DVI-I", "DVI-D", "DVI-A",
   "Composite", "S-Video", "LVDS", "Component", "DIN",
   "DisplayPort", "HDMI-A", "HDMI-B", "TV", "eDP", "Virtual", "DSI",
};

/**
 * @internal
 * @brief Prints debug information for an output.
 *
 * This function logs various properties of the given Ecore_Drm2_Output
 * and its associated drmModeConnector.
 *
 * @param output The Ecore_Drm2_Output to debug.
 * @param conn The drmModeConnector associated with the output.
 */
static void
_output_debug(Ecore_Drm2_Output *output, const drmModeConnector *conn)
{
   Eina_List *l;
   Ecore_Drm2_Output_Mode *omode;
   Ecore_Drm2_Plane_State *pstate;

   DBG("Created New Output At %d,%d", output->x, output->y);
   DBG("\tCrtc Pos: %d %d", output->ocrtc->x, output->ocrtc->y);
   DBG("\tCrtc: %d", output->crtc_id);
   DBG("\tConn: %d", output->conn_id);
   DBG("\tName: %s", output->name);
   DBG("\tMake: %s", output->make);
   DBG("\tModel: %s", output->model);
   DBG("\tSerial: %s", output->serial);
   DBG("\tCloned: %d", output->cloned);
   DBG("\tPrimary: %d", output->primary);
   DBG("\tConnected: %d", output->connected);
   DBG("\tEnabled: %d", output->enabled);

   if (output->backlight.path)
     {
        DBG("\tBacklight");
        switch (output->backlight.type)
          {
           case ECORE_DRM2_BACKLIGHT_RAW:
             DBG("\t\tType: Raw");
             break;
           case ECORE_DRM2_BACKLIGHT_PLATFORM:
             DBG("\t\tType: Platform");
             break;
           case ECORE_DRM2_BACKLIGHT_FIRMWARE:
             DBG("\t\tType: Firmware");
             break;
          }
        DBG("\t\tPath: %s", output->backlight.path);
     }

   EINA_LIST_FOREACH(output->plane_states, l, pstate)
     DBG("\tPossible Plane: %d", pstate->obj_id);

   EINA_LIST_FOREACH(output->modes, l, omode)
     {
        DBG("\tAdded Mode: %dx%d@%d%s%s%s",
            omode->width, omode->height, omode->refresh,
            (omode->flags & DRM_MODE_TYPE_PREFERRED) ? ", preferred" : "",
            (omode->flags & DRM_MODE_TYPE_DEFAULT) ? ", current" : "",
            (conn->count_modes == 0) ? ", built-in" : "");
     }
}

/**
 * @internal
 * @brief Frees an Ecore_Drm2_Event_Output_Changed event.
 *
 * This function is a callback used by the ecore event system to free
 * the memory allocated for an Ecore_Drm2_Event_Output_Changed event,
 * including its stringshared members.
 *
 * @param data User data (unused).
 * @param event The event to free.
 */
static void
_cb_output_event_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Drm2_Event_Output_Changed *ev;

   ev = event;
   eina_stringshare_del(ev->make);
   eina_stringshare_del(ev->model);
   eina_stringshare_del(ev->name);
   free(ev);
}

/**
 * @internal
 * @brief Sends an ECORE_DRM2_EVENT_OUTPUT_CHANGED event.
 *
 * This function creates and populates an Ecore_Drm2_Event_Output_Changed
 * event based on the current state of the given Ecore_Drm2_Output,
 * and then adds it to the ecore event queue.
 *
 * @param output The Ecore_Drm2_Output whose state has changed.
 */
static void
_output_event_send(Ecore_Drm2_Output *output)
{
   Ecore_Drm2_Event_Output_Changed *ev;

   ev = calloc(1, sizeof(Ecore_Drm2_Event_Output_Changed));
   if (!ev) return;

   ev->id = output->crtc_id;

   ev->x = output->x;
   ev->y = output->y;
   if (output->current_mode)
     {
        ev->w = output->current_mode->width;
        ev->h = output->current_mode->height;
        ev->refresh = output->current_mode->refresh;
     }
   else
     {
        ev->w = output->ocrtc->width;
        ev->h = output->ocrtc->height;
        ev->refresh = 0;
     }

   ev->phys_width = output->pw;
   ev->phys_height = output->ph;

   ev->scale = output->scale;
   ev->subpixel = output->subpixel;
   ev->transform = output->transform;
   ev->connected = output->connected;
   ev->enabled = output->enabled;

   ev->name = eina_stringshare_ref(output->name);
   ev->make = eina_stringshare_ref(output->make);
   ev->model = eina_stringshare_ref(output->model);

   ecore_event_add(ECORE_DRM2_EVENT_OUTPUT_CHANGED, ev,
                   _cb_output_event_free, NULL);
}

/**
 * @internal
 * @brief Parses a string from EDID data.
 *
 * This function extracts a string from a given EDID data block,
 * ensuring it's null-terminated and printable. Non-printable
 * characters are replaced with '-'. If too many characters are
 * non-printable, the string is considered invalid and emptied.
 *
 * @param data Pointer to the EDID data block (13 bytes expected, 12 for string + null).
 * @param text Output buffer to store the parsed string (should be at least 13 bytes).
 */
static void
_output_edid_parse_string(const uint8_t *data, char text[])
{
   int i = 0, rep = 0;

   strncpy(text, (const char *)data, 12);

   for (; text[i] != '\0'; i++)
     {
        if ((text[i] == '\n') || (text[i] == '\r'))
          {
             text[i] = '\0';
             break;
          }
     }

   for (i = 0; text[i] != '\0'; i++)
     {
        if (!isprint(text[i]))
          {
             text[i] = '-';
             rep++;
          }
     }

   if (rep > 4) text[0] = '\0';
}

/**
 * @internal
 * @brief Parses EDID data for an output.
 *
 * This function extracts PNP ID, serial number, monitor name, and EISA ID
 * from the raw EDID data.
 *
 * @param output The Ecore_Drm2_Output to populate with EDID information.
 * @param data Pointer to the raw EDID data.
 * @param len Length of the EDID data.
 * @return 0 on success, -1 on failure (e.g., invalid EDID header).
 */
static int
_output_edid_parse(Ecore_Drm2_Output *output, const uint8_t *data, size_t len)
{
   int i = 0;
   uint32_t serial;

   if (len < 128) return -1;
   if ((data[0] != 0x00) || (data[1] != 0xff)) return -1;

   output->edid.pnp[0] = 'A' + ((data[EDID_OFFSET_PNPID + 0] & 0x7c) / 4) - 1;
   output->edid.pnp[1] =
     'A' + ((data[EDID_OFFSET_PNPID + 0] & 0x3) * 8) +
     ((data[EDID_OFFSET_PNPID + 1] & 0xe0) / 32) - 1;
   output->edid.pnp[2] = 'A' + (data[EDID_OFFSET_PNPID + 1] & 0x1f) - 1;
   output->edid.pnp[3] = '\0';

   serial = (uint32_t) data[EDID_OFFSET_SERIAL + 0];
   serial += (uint32_t) data[EDID_OFFSET_SERIAL + 1] * 0x100;
   serial += (uint32_t) data[EDID_OFFSET_SERIAL + 2] * 0x10000;
   serial += (uint32_t) data[EDID_OFFSET_SERIAL + 3] * 0x1000000;
   if (serial > 0)
     sprintf(output->edid.serial, "%lu", (unsigned long)serial);

   for (i = EDID_OFFSET_DATA_BLOCKS; i <= EDID_OFFSET_LAST_BLOCK; i += 18)
     {
        if (data[i] != 0) continue;
        if (data[i + 2] != 0) continue;

        if (data[i + 3] == EDID_DESCRIPTOR_DISPLAY_PRODUCT_NAME)
          _output_edid_parse_string(&data[i + 5], output->edid.monitor);
        else if (data[i + 3] == EDID_DESCRIPTOR_DISPLAY_PRODUCT_SERIAL_NUMBER)
          _output_edid_parse_string(&data[i + 5], output->edid.serial);
        else if (data[i + 3] == EDID_DESCRIPTOR_ALPHANUMERIC_DATA_STRING)
          _output_edid_parse_string(&data[i + 5], output->edid.eisa);
     }

   return 0;
}

/**
 * @internal
 * @brief Finds and parses EDID information for an output using atomic KMS state.
 *
 * This function retrieves EDID data from the connector's atomic state
 * and then parses it to populate the output's make, model, and serial.
 *
 * @param output The Ecore_Drm2_Output to update.
 */
static void
_output_edid_atomic_find(Ecore_Drm2_Output *output)
{
   Ecore_Drm2_Connector_State *cstate;
   int ret = 0;

   cstate = output->conn_state;

   ret = _output_edid_parse(output, cstate->edid.data, cstate->edid.len);
   if (!ret)
     {
        if (output->edid.pnp[0] != '\0')
          eina_stringshare_replace(&output->make, output->edid.pnp);
        if (output->edid.monitor[0] != '\0')
          eina_stringshare_replace(&output->model, output->edid.monitor);
        if (output->edid.serial[0] != '\0')
          eina_stringshare_replace(&output->serial, output->edid.serial);
     }
}

/**
 * @internal
 * @brief Finds and parses EDID information for an output using legacy KMS.
 *
 * This function retrieves the EDID blob property from the connector,
 * parses it, and updates the output's make, model, and serial.
 *
 * @param output The Ecore_Drm2_Output to update.
 * @param conn The drmModeConnector associated with the output.
 */
static void
_output_edid_find(Ecore_Drm2_Output *output, const drmModeConnector *conn)
{
   drmModePropertyBlobPtr blob = NULL;
   drmModePropertyPtr prop;
   int i = 0, ret = 0;

   for (; i < conn->count_props && !blob; i++)
     {
        if (!(prop = sym_drmModeGetProperty(output->fd, conn->props[i])))
          continue;
        if ((prop->flags & DRM_MODE_PROP_BLOB) &&
            (!strcmp(prop->name, "EDID")))
          {
             blob = sym_drmModeGetPropertyBlob(output->fd, conn->prop_values[i]);
          }
        sym_drmModeFreeProperty(prop);
        if (blob) break;
     }

   if (!blob) return;

   output->edid.blob = eina_memdup(blob->data, blob->length, 1);

   ret = _output_edid_parse(output, blob->data, blob->length);
   if (!ret)
     {
        if (output->edid.pnp[0] != '\0')
          eina_stringshare_replace(&output->make, output->edid.pnp);
        if (output->edid.monitor[0] != '\0')
          eina_stringshare_replace(&output->model, output->edid.monitor);
        if (output->edid.serial[0] != '\0')
          eina_stringshare_replace(&output->serial, output->edid.serial);
     }

   sym_drmModeFreePropertyBlob(blob);
}

/**
 * @internal
 * @brief Finds a suitable CRTC index for a given connector.
 *
 * This function iterates through the encoders associated with the connector
 * to find a connected CRTC. It prioritizes connected connectors.
 *
 * @param res Pointer to drmModeRes (DRM resources).
 * @param conn Pointer to drmModeConnector.
 * @param fd File descriptor for the DRM device.
 * @return The index of the CRTC in res->crtcs on success, or -1 if no suitable CRTC is found.
 */
static int
_output_crtc_find(const drmModeRes *res, const drmModeConnector *conn, int fd)
{
   drmModeEncoder *enc;
   uint32_t crtc;
   int i = 0, j = 0;

   /* Skip all disconnected connectors...
    *
    * When a connector is disconnected it still has an encoder id
    * which messes up our output selection code later.  When we support
    * multi-head properly and hotplug becomes a real thing we'll
    * need to revisit this hack (and the crtc assignment code as well)
    */
   if (conn->connection != DRM_MODE_CONNECTED) return -1;

   for (j = 0; j < conn->count_encoders; j++)
     {
        enc = sym_drmModeGetEncoder(fd, conn->encoders[j]);
        if (!enc) continue;

        crtc = enc->crtc_id;
        sym_drmModeFreeEncoder(enc);

        for (i = 0; i < res->count_crtcs; i++)
          if (crtc == res->crtcs[i])
            return i;
     }

   return -1;
}

/**
 * @internal
 * @brief Generates a name for an output.
 *
 * The name is generated based on the connector type and its ID,
 * e.g., "HDMI-A-1", "DP-1".
 *
 * @param conn The drmModeConnector.
 * @return A newly allocated string containing the output name.
 *         The caller is responsible for freeing this string.
 */
static char *
_output_name_get(const drmModeConnector *conn)
{
   char name[DRM_CONNECTOR_NAME_LEN];
   const char *type = NULL;

   if (conn->connector_type < EINA_C_ARRAY_LENGTH(conn_types))
     type = conn_types[conn->connector_type];
   else
     type = "UNKNOWN";

   snprintf(name, sizeof(name), "%s-%d", type, conn->connector_type_id);
   return strdup(name);
}

/**
 * @internal
 * @brief Creates and adds an Ecore_Drm2_Output_Mode to an output.
 *
 * This function converts a drmModeModeInfo structure into an
 * Ecore_Drm2_Output_Mode, calculates the refresh rate, and appends
 * it to the output's list of modes.
 *
 * @param output The Ecore_Drm2_Output to add the mode to.
 * @param info The drmModeModeInfo describing the mode.
 * @return A pointer to the newly created Ecore_Drm2_Output_Mode, or NULL on failure.
 */
static Ecore_Drm2_Output_Mode *
_output_mode_add(Ecore_Drm2_Output *output, const drmModeModeInfo *info)
{
   Ecore_Drm2_Output_Mode *mode;
   uint64_t refresh;

   EINA_SAFETY_ON_NULL_RETURN_VAL(info, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL((info->htotal > 0), NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL((info->vtotal > 0), NULL);

   mode = calloc(1, sizeof(Ecore_Drm2_Output_Mode));
   if (!mode) return NULL;

   mode->flags = 0;
   mode->width = info->hdisplay;
   mode->height = info->vdisplay;

   refresh = (info->clock * 1000LL / info->htotal + info->vtotal / 2) /
     info->vtotal;

   if (info->flags & DRM_MODE_FLAG_INTERLACE)
     refresh *= 2;
   if (info->flags & DRM_MODE_FLAG_DBLSCAN)
     refresh /= 2;
   if (info->vscan > 1)
     refresh /= info->vscan;

   mode->refresh = refresh;
   mode->info = *info;

   if (info->type & DRM_MODE_TYPE_PREFERRED)
     mode->flags |= DRM_MODE_TYPE_PREFERRED;

   output->modes = eina_list_append(output->modes, mode);

   return mode;
}

/**
 * @internal
 * @brief Creates and populates the list of modes for an output.
 *
 * This function retrieves all modes reported by the connector and
 * the current mode from the CRTC. It then determines the current,
 * preferred, and best available mode for the output.
 *
 * @param dev The Ecore_Drm2_Device.
 * @param output The Ecore_Drm2_Output to populate with modes.
 * @param conn The drmModeConnector associated with the output.
 */
static void
_output_modes_create(Ecore_Drm2_Device *dev, Ecore_Drm2_Output *output, const drmModeConnector *conn)
{
   int i = 0;
   drmModeCrtc *crtc;
   drmModeEncoder *enc;
   drmModeModeInfo crtc_mode;
   Ecore_Drm2_Output_Mode *omode;
   Ecore_Drm2_Output_Mode *current = NULL, *preferred = NULL, *best = NULL;
   Eina_List *l = NULL;

   memset(&crtc_mode, 0, sizeof(crtc_mode));

   enc = sym_drmModeGetEncoder(dev->fd, conn->encoder_id);
   if (enc)
     {
        crtc = sym_drmModeGetCrtc(dev->fd, enc->crtc_id);
        sym_drmModeFreeEncoder(enc);
        if (!crtc) return;
        if (crtc->mode_valid) crtc_mode = crtc->mode;
        sym_drmModeFreeCrtc(crtc);
     }

   for (i = 0; i < conn->count_modes; i++)
     {
        omode = _output_mode_add(output, &conn->modes[i]);
        if (!omode) continue;
     }

   EINA_LIST_REVERSE_FOREACH(output->modes, l, omode)
     {
        if (!memcmp(&crtc_mode, &omode->info, sizeof(crtc_mode)))
          current = omode;
        if (omode->flags & DRM_MODE_TYPE_PREFERRED)
          preferred = omode;
        best = omode;
     }

   if ((!current) && (crtc_mode.clock != 0))
     {
        current = _output_mode_add(output, &crtc_mode);
        if (!current) goto err;
     }

   if (current) output->current_mode = current;
   else if (preferred) output->current_mode = preferred;
   else if (best) output->current_mode = best;

   if (!output->current_mode) goto err;

   output->current_mode->flags |= DRM_MODE_TYPE_DEFAULT;

   return;

err:
   EINA_LIST_FREE(output->modes, omode)
     free(omode);
}

/**
 * @internal
 * @brief Gets the DPMS property for a connector.
 *
 * This function iterates through the connector's properties to find
 * the "DPMS" property.
 *
 * @param fd File descriptor for the DRM device.
 * @param conn The drmModeConnector.
 * @return A pointer to the drmModePropertyPtr for DPMS, or NULL if not found.
 *         The caller is responsible for freeing the property if not NULL.
 */
static drmModePropertyPtr
_output_dpms_property_get(int fd, const drmModeConnector *conn)
{
   drmModePropertyPtr prop;
   int i = 0;

   for (; i < conn->count_props; i++)
     {
        prop = sym_drmModeGetProperty(fd, conn->props[i]);
        if (!prop) continue;

        if (!strcmp(prop->name, "DPMS")) return prop;

        sym_drmModeFreeProperty(prop);
     }

   return NULL;
}

/**
 * @internal
 * @brief Gets a backlight attribute value.
 *
 * Reads a specific attribute (e.g., "max_brightness", "brightness")
 * from the backlight sysfs path associated with the output.
 *
 * @param output The Ecore_Drm2_Output.
 * @param attr The backlight attribute to read.
 * @return The value of the attribute as a double, or 0.0 on error or if not found.
 */
static double
_output_backlight_value_get(Ecore_Drm2_Output *output, const char *attr)
{
   const char *b = NULL;
   double ret = 0.0;

   if ((!output) || (!output->backlight.path)) return 0.0;

   b = eeze_udev_syspath_get_sysattr(output->backlight.path, attr);
   if (!b) return 0.0;

   ret = strtod(b, NULL);
   if (ret < 0) ret = 0.0;

   return ret;
}

/**
 * @internal
 * @brief Initializes backlight information for an output.
 *
 * This function searches for a suitable backlight device in sysfs
 * based on the connector type. If found, it stores the backlight
 * type, path, max brightness, and current brightness in the output structure.
 *
 * @param output The Ecore_Drm2_Output to initialize backlight for.
 * @param conn_type The connector type (e.g., DRM_MODE_CONNECTOR_LVDS, DRM_MODE_CONNECTOR_eDP).
 */
static void
_output_backlight_init(Ecore_Drm2_Output *output, unsigned int conn_type)
{
   Eina_List *devs, *l;
   const char *dev, *t;
   Eina_Bool found = EINA_FALSE;
   Ecore_Drm2_Backlight_Type type = 0;

   devs = eeze_udev_find_by_filter("backlight", NULL, NULL);

   EINA_LIST_FOREACH(devs, l, dev)
     {
        t = eeze_udev_syspath_get_sysattr(dev, "type");
        if (!t) continue;

        if (!strcmp(t, "raw"))
          type = ECORE_DRM2_BACKLIGHT_RAW;
        else if (!strcmp(t, "platform"))
          type = ECORE_DRM2_BACKLIGHT_PLATFORM;
        else if (!strcmp(t, "firmware"))
          type = ECORE_DRM2_BACKLIGHT_FIRMWARE;

        if ((conn_type == DRM_MODE_CONNECTOR_LVDS) ||
            (conn_type == DRM_MODE_CONNECTOR_eDP) ||
            (type == ECORE_DRM2_BACKLIGHT_RAW))
          found = EINA_TRUE;

        eina_stringshare_del(t);
        if (found) break;
     }

   if (found)
     {
        output->backlight.type = type;
        output->backlight.path = eina_stringshare_add(dev);
        output->backlight.max =
          _output_backlight_value_get(output, "max_brightness");
        output->backlight.value =
          _output_backlight_value_get(output, "brightness");
     }

   EINA_LIST_FREE(devs, dev)
     eina_stringshare_del(dev);
}

/**
 * @internal
 * @brief Initializes the scale and transform for an output.
 *
 * Sets the initial transform and scale for the output. It also adjusts
 * the output's logical width (output->w) and height (output->h) based
 * on the transform and scale, if the output is enabled and has a current mode.
 *
 * @param output The Ecore_Drm2_Output to initialize.
 * @param transform The initial Ecore_Drm2_Transform to apply.
 * @param scale The initial scale factor (e.g., 1, 2).
 */
static void
_output_scale_init(Ecore_Drm2_Output *output, Ecore_Drm2_Transform transform, unsigned int scale)
{
   output->transform = transform;

   if ((output->enabled) && (output->current_mode))
     {
        switch (transform)
          {
           case ECORE_DRM2_TRANSFORM_90:
           case ECORE_DRM2_TRANSFORM_270:
           case ECORE_DRM2_TRANSFORM_FLIPPED_90:
           case ECORE_DRM2_TRANSFORM_FLIPPED_270:
             output->w = output->current_mode->height;
             output->h = output->current_mode->width;
             break;
           case ECORE_DRM2_TRANSFORM_NORMAL:
           case ECORE_DRM2_TRANSFORM_180:
           case ECORE_DRM2_TRANSFORM_FLIPPED:
           case ECORE_DRM2_TRANSFORM_FLIPPED_180:
             output->w = output->current_mode->width;
             output->h = output->current_mode->height;
             break;
           default:
             break;
          }
     }

   output->scale = scale;
   output->w /= scale;
   output->h /= scale;
}

/**
 * @internal
 * @brief Applies a 2D rotation to an Eina_Matrix3.
 *
 * This helper function effectively multiplies the given 3x3 matrix by a
 * 2D rotation matrix defined by (x, y) and (-y, x) for its first two columns.
 * It achieves this by converting to 4x4 matrices for multiplication.
 *
 * @param matrix The Eina_Matrix3 to rotate.
 * @param x The x component of the first basis vector of the rotation.
 * @param y The y component of the first basis vector of the rotation.
 */
static void
_output_matrix_rotate_xy(Eina_Matrix3 *matrix, double x, double y)
{
   Eina_Matrix4 tmp, m;

   eina_matrix4_identity(&tmp);
   eina_matrix4_values_set(&tmp, x, y, 0, 0, -y, x, 0, 0,
                           0, 0, 1, 0, 0, 0, 0, 1);

   eina_matrix3_matrix4_to(&m, matrix);
   eina_matrix4_multiply(&m, &m, &tmp);
   eina_matrix4_matrix3_to(matrix, &m);
}

/**
 * @internal
 * @brief Updates the transformation matrix for an output.
 *
 * This function calculates the 4x4 transformation matrix (`output->matrix`)
 * and its inverse (`output->inverse`) based on the output's position (x, y),
 * logical width (w), logical height (h), transform (rotation/flip), and scale.
 * This matrix can be used to convert coordinates from output space to
 * screen space.
 *
 * The matrix is built by:
 * 1. Translating by -output->x, -output->y.
 * 2. Applying flip-specific translation (if flipped).
 * 3. Applying rotation-specific translation and rotation.
 * 4. Applying scaling.
 *
 * @param output The Ecore_Drm2_Output whose matrix needs updating.
 */
static void
_output_matrix_update(Ecore_Drm2_Output *output)
{
   Eina_Matrix3 m3;

   eina_matrix4_identity(&output->matrix);
   eina_matrix4_matrix3_to(&m3, &output->matrix);
   eina_matrix3_translate(&m3, -output->x, -output->y);

   switch (output->transform)
     {
      case ECORE_DRM2_TRANSFORM_FLIPPED:
      case ECORE_DRM2_TRANSFORM_FLIPPED_90:
      case ECORE_DRM2_TRANSFORM_FLIPPED_180:
      case ECORE_DRM2_TRANSFORM_FLIPPED_270:
        eina_matrix3_translate(&m3, -output->w, 0);
        break;
      default:
        break;
     }

   switch (output->transform)
     {
      case ECORE_DRM2_TRANSFORM_NORMAL:
      case ECORE_DRM2_TRANSFORM_FLIPPED:
      default:
        break;
      case ECORE_DRM2_TRANSFORM_90:
      case ECORE_DRM2_TRANSFORM_FLIPPED_90:
        eina_matrix3_translate(&m3, 0, -output->h);
        _output_matrix_rotate_xy(&m3, 0, 1);
        break;
      case ECORE_DRM2_TRANSFORM_180:
      case ECORE_DRM2_TRANSFORM_FLIPPED_180:
        eina_matrix3_translate(&m3, -output->w, -output->h);
        _output_matrix_rotate_xy(&m3, -1, 0);
        break;
      case ECORE_DRM2_TRANSFORM_270:
      case ECORE_DRM2_TRANSFORM_FLIPPED_270:
        eina_matrix3_translate(&m3, -output->w, 0);
        _output_matrix_rotate_xy(&m3, 0, -1);
        break;
     }

   if (output->scale != 1)
     eina_matrix3_scale(&m3, output->scale, output->scale);

   eina_matrix3_matrix4_to(&output->matrix, &m3);
   eina_matrix4_inverse(&output->inverse, &output->matrix);
}

/**
 * @internal
 * @brief Duplicates an Ecore_Drm2_Crtc_State structure.
 *
 * Allocates memory for a new Ecore_Drm2_Crtc_State and copies the
 * content of the provided state into it.
 *
 * @param state The Ecore_Drm2_Crtc_State to duplicate.
 * @return A pointer to the newly allocated and copied Ecore_Drm2_Crtc_State,
 *         or NULL on allocation failure. The caller is responsible for freeing
 *         the returned state.
 */
static Ecore_Drm2_Crtc_State *
_atomic_state_crtc_duplicate(Ecore_Drm2_Crtc_State *state)
{
   Ecore_Drm2_Crtc_State *cstate;

   cstate = calloc(1, sizeof(Ecore_Drm2_Crtc_State));
   if (!cstate) return NULL;

   memcpy(cstate, state, sizeof(Ecore_Drm2_Crtc_State));

   return cstate;
}

/**
 * @internal
 * @brief Retrieves a copy of a CRTC state by ID from an atomic state snapshot.
 *
 * Searches through the CRTC states in the given Ecore_Drm2_Atomic_State
 * for one matching the specified CRTC ID. If found, a duplicate of that
 * state is returned.
 *
 * @param state The Ecore_Drm2_Atomic_State containing various KMS object states.
 * @param id The CRTC object ID to search for.
 * @return A pointer to a duplicated Ecore_Drm2_Crtc_State if found,
 *         otherwise NULL. The caller is responsible for freeing the returned state.
 */
static Ecore_Drm2_Crtc_State *
_output_crtc_state_get(Ecore_Drm2_Atomic_State *state, unsigned int id)
{
   Ecore_Drm2_Crtc_State *cstate;
   int i = 0;

   for (; i < state->crtcs; i++)
     {
        cstate = &state->crtc_states[i];
        if (cstate->obj_id != id) continue;
        return _atomic_state_crtc_duplicate(cstate);
     }

   return NULL;
}

/**
 * @internal
 * @brief Duplicates an Ecore_Drm2_Connector_State structure.
 *
 * Allocates memory for a new Ecore_Drm2_Connector_State and copies the
 * content of the provided state into it.
 *
 * @param state The Ecore_Drm2_Connector_State to duplicate.
 * @return A pointer to the newly allocated and copied Ecore_Drm2_Connector_State,
 *         or NULL on allocation failure. The caller is responsible for freeing
 *         the returned state.
 */
static Ecore_Drm2_Connector_State *
_atomic_state_conn_duplicate(Ecore_Drm2_Connector_State *state)
{
   Ecore_Drm2_Connector_State *cstate;

   cstate = calloc(1, sizeof(Ecore_Drm2_Connector_State));
   if (!cstate) return NULL;

   memcpy(cstate, state, sizeof(Ecore_Drm2_Connector_State));

   return cstate;
}

/**
 * @internal
 * @brief Retrieves a copy of a connector state by ID from an atomic state snapshot.
 *
 * Searches through the connector states in the given Ecore_Drm2_Atomic_State
 * for one matching the specified connector ID. If found, a duplicate of that
 * state is returned.
 *
 * @param state The Ecore_Drm2_Atomic_State containing various KMS object states.
 * @param id The connector object ID to search for.
 * @return A pointer to a duplicated Ecore_Drm2_Connector_State if found,
 *         otherwise NULL. The caller is responsible for freeing the returned state.
 */
static Ecore_Drm2_Connector_State *
_output_conn_state_get(Ecore_Drm2_Atomic_State *state, unsigned int id)
{
   Ecore_Drm2_Connector_State *cstate;
   int i = 0;

   for (; i < state->conns; i++)
     {
        cstate = &state->conn_states[i];
        if (cstate->obj_id != id) continue;
        return _atomic_state_conn_duplicate(cstate);
     }

   return NULL;
}

/**
 * @internal
 * @brief Duplicates an Ecore_Drm2_Plane_State structure.
 *
 * Allocates memory for a new Ecore_Drm2_Plane_State and copies the
 * content of the provided state into it.
 *
 * @param state The Ecore_Drm2_Plane_State to duplicate.
 * @return A pointer to the newly allocated and copied Ecore_Drm2_Plane_State,
 *         or NULL on allocation failure. The caller is responsible for freeing
 *         the returned state.
 */
static Ecore_Drm2_Plane_State *
_atomic_state_plane_duplicate(Ecore_Drm2_Plane_State *state)
{
   Ecore_Drm2_Plane_State *pstate;

   pstate = calloc(1, sizeof(Ecore_Drm2_Plane_State));
   if (!pstate) return NULL;

   memcpy(pstate, state, sizeof(Ecore_Drm2_Plane_State));

   return pstate;
}

/**
 * @internal
 * @brief Retrieves copies of plane states associated with a CRTC from an atomic state snapshot.
 *
 * Searches through the plane states in the given Ecore_Drm2_Atomic_State.
 * It collects duplicates of plane states that are either directly assigned to
 * the given `crtc_id` or whose `possible_crtcs` mask includes the CRTC
 * indicated by `index`.
 *
 * @param state The Ecore_Drm2_Atomic_State containing various KMS object states.
 * @param crtc_id The CRTC object ID to filter planes for.
 * @param index The index of the CRTC (used for checking `possible_crtcs` mask).
 * @return An Eina_List of duplicated Ecore_Drm2_Plane_State structures.
 *         The caller is responsible for freeing the list and its contents.
 *         Returns NULL if no matching planes are found or on allocation failure.
 */
static Eina_List *
_output_plane_states_get(Ecore_Drm2_Atomic_State *state, unsigned int crtc_id, int index)
{
   Eina_List *states = NULL;
   Ecore_Drm2_Plane_State *pstate;

   int i = 0;

   for (; i < state->planes; i++)
     {
        pstate = &state->plane_states[i];
        if (pstate->cid.value == crtc_id)
          {
             states =
               eina_list_append(states, _atomic_state_plane_duplicate(pstate));
          }
        else if (pstate->mask & (1ULL << index))
          {
             states =
               eina_list_append(states, _atomic_state_plane_duplicate(pstate));
          }
     }

   return states;
}

/**
 * @internal
 * @brief Creates and initializes a new Ecore_Drm2_Output.
 *
 * This function allocates and sets up an Ecore_Drm2_Output structure based on
 * the provided DRM resources and connector information. It finds a CRTC,
 * sets up basic properties (name, make, model, physical size, subpixel order),
 * initializes modes, EDID, backlight, DPMS, gamma, scale, and transform.
 * If atomic mode setting is used, it also retrieves initial CRTC, connector,
 * and plane states.
 *
 * @param dev The Ecore_Drm2_Device.
 * @param res DRM resources (drmModeRes).
 * @param conn The drmModeConnector for this output.
 * @param x The initial X position of the output.
 * @param y The initial Y position of the output.
 * @param[out] w Pointer to store the width of the output's current mode (optional).
 * @param cloned EINA_TRUE if this output is a clone of another.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_output_create(Ecore_Drm2_Device *dev, const drmModeRes *res, const drmModeConnector *conn, int x, int y, int *w, Eina_Bool cloned)
{
   Ecore_Drm2_Output *output;
   int i = 0;
   char *name = NULL;

   if (w) *w = 0;

   i = _output_crtc_find(res, conn, dev->fd);
   if (i < 0) return EINA_FALSE;

   output = calloc(1, sizeof(Ecore_Drm2_Output));
   if (!output) return EINA_FALSE;

   output->fd = dev->fd;
   output->x = x;
   output->y = y;
   output->cloned = cloned;
   output->pw = conn->mmWidth;
   output->ph = conn->mmHeight;

   switch (conn->subpixel)
     {
      case DRM_MODE_SUBPIXEL_UNKNOWN:
        output->subpixel = 0; // WL_OUTPUT_SUBPIXEL_UNKNOWN
        break;
      case DRM_MODE_SUBPIXEL_NONE:
        output->subpixel = 1; // WL_OUTPUT_SUBPIXEL_NONE
        break;
      case DRM_MODE_SUBPIXEL_HORIZONTAL_RGB:
        output->subpixel = 2; // WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB
        break;
      case DRM_MODE_SUBPIXEL_HORIZONTAL_BGR:
        output->subpixel = 3; // WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR
        break;
      case DRM_MODE_SUBPIXEL_VERTICAL_RGB:
        output->subpixel = 4; // WL_OUTPUT_SUBPIXEL_VERTICAL_RGB
        break;
      case DRM_MODE_SUBPIXEL_VERTICAL_BGR:
        output->subpixel = 5; // WL_OUTPUT_SUBPIXEL_VERTICAL_BGR
        break;
      default:
        output->subpixel = 0;
        break;
     }

   name = _output_name_get(conn);
   output->name = eina_stringshare_add(name);
   output->make = eina_stringshare_add("unknown");
   output->model = eina_stringshare_add("unknown");
   output->serial = eina_stringshare_add("unknown");
   free(name);

   output->pipe = i;
   output->crtc_id = res->crtcs[i];
   output->conn_id = conn->connector_id;
   output->conn_type = conn->connector_type;

   output->connected = (conn->connection == DRM_MODE_CONNECTED);

   output->ocrtc = sym_drmModeGetCrtc(dev->fd, output->crtc_id);

   if (_ecore_drm2_use_atomic)
     {
        output->crtc_state =
          _output_crtc_state_get(dev->state, output->crtc_id);
        output->conn_state =
          _output_conn_state_get(dev->state, output->conn_id);
        output->plane_states =
          _output_plane_states_get(dev->state, output->crtc_id, output->pipe);
     }

   output->dpms = _output_dpms_property_get(dev->fd, conn);

   _output_backlight_init(output, conn->connector_type);

   output->gamma = output->ocrtc->gamma_size;

   _output_modes_create(dev, output, conn);

   if (_ecore_drm2_use_atomic)
     _output_edid_atomic_find(output);
   else
     _output_edid_find(output, conn);

   if (output->connected) output->enabled = EINA_TRUE;

   _output_scale_init(output, ECORE_DRM2_TRANSFORM_NORMAL, 1);
   _output_matrix_update(output);

   if (!eina_list_count(dev->outputs))
     output->primary = EINA_TRUE;
   else
     {
        /* temporarily disable other outputs which are not primary */
        output->connected = EINA_FALSE;
        output->enabled = EINA_FALSE;
     }

   dev->outputs = eina_list_append(dev->outputs, output);

   _output_debug(output, conn);

   if ((output->enabled) && (output->current_mode))
     {
        if (w) *w = output->current_mode->width;
     }

   return EINA_TRUE;
}

/**
 * @internal
 * @brief Finds an Ecore_Drm2_Output by its connector ID.
 *
 * Iterates through the list of outputs in the Ecore_Drm2_Device
 * to find an output matching the given connector ID.
 *
 * @param dev The Ecore_Drm2_Device.
 * @param id The connector ID to search for.
 * @return A pointer to the Ecore_Drm2_Output if found, otherwise NULL.
 */
static Ecore_Drm2_Output *
_output_find_by_con(Ecore_Drm2_Device *dev, uint32_t id)
{
   Ecore_Drm2_Output *output;
   Eina_List *l;

   EINA_LIST_FOREACH(dev->outputs, l, output)
     if (output->conn_id == id) return output;

   return NULL;
}

/**
 * @internal
 * @brief Updates the state of all outputs for a device.
 *
 * This function is typically called in response to a hotplug event.
 * It refreshes the DRM resources, identifies connected connectors,
 * creates new Ecore_Drm2_Output instances for newly connected outputs,
 * and updates the connected/enabled status of existing outputs.
 * An ECORE_DRM2_EVENT_OUTPUT_CHANGED event is sent for each output
 * whose state might have changed.
 *
 * @param dev The Ecore_Drm2_Device whose outputs need updating.
 */
static void
_outputs_update(Ecore_Drm2_Device *dev)
{
   Ecore_Drm2_Output *output;
   Eina_List *l, *ll;
   drmModeRes *res;
   drmModeConnector *conn;
   uint32_t *connected;
   int i = 0, x = 0, y = 0;

   res = sym_drmModeGetResources(dev->fd);
   if (!res) return;

   connected = calloc(res->count_connectors, sizeof(uint32_t));
   if (!connected)
     {
        sym_drmModeFreeResources(res);
        return;
     }

   for (i = 0; i < res->count_connectors; i++)
     {
        conn = sym_drmModeGetConnector(dev->fd, res->connectors[i]);
        if (!conn) continue;

        if (conn->connection != DRM_MODE_CONNECTED) goto next;

        connected[i] = res->connectors[i];
        if (!_output_find_by_con(dev, res->connectors[i]))
          {
             if (dev->outputs)
               {
                  Ecore_Drm2_Output *last;

                  last = eina_list_last_data_get(dev->outputs);
                  if (last) x = last->x + last->current_mode->width;
                  else x = 0;
               }
             else
               x = 0;

             if (!_output_create(dev, res, conn, x, y, NULL, EINA_TRUE))
               goto next;
          }

next:
        sym_drmModeFreeConnector(conn);
     }

   sym_drmModeFreeResources(res);

   EINA_LIST_FOREACH_SAFE(dev->outputs, l, ll, output)
     {
        Eina_Bool disconnected = EINA_TRUE;

        for (i = 0; i < res->count_connectors; i++)
          if (connected[i] == output->conn_id)
            {
               disconnected = EINA_FALSE;
               break;
            }

        if (disconnected)
          {
             output->connected = EINA_FALSE;
             output->enabled = EINA_FALSE;
          }
        else
          {
             output->connected = EINA_TRUE;
             output->enabled = EINA_TRUE;
          }

        _output_event_send(output);
     }
   free(connected);
}

/**
 * @internal
 * @brief Callback for udev output events (hotplug).
 *
 * This function is triggered by eeze when a DRM-related udev event occurs
 * (e.g., monitor connected or disconnected). It calls _outputs_update
 * to refresh the output list and their states.
 *
 * @param device The udev device path (unused).
 * @param event The type of udev event (unused).
 * @param data User data, expected to be an Ecore_Drm2_Device pointer.
 * @param watch The eeze udev watch (unused).
 */
static void
_cb_output_event(const char *device EINA_UNUSED, Eeze_Udev_Event event EINA_UNUSED, void *data, Eeze_Udev_Watch *watch EINA_UNUSED)
{
   Ecore_Drm2_Device *dev;

   dev = data;
   _outputs_update(dev);
}

/**
 * @internal
 * @brief Destroys an Ecore_Drm2_Output instance.
 *
 * Frees all resources associated with an Ecore_Drm2_Output, including
 * atomic state information (if used), mode list, backlight path,
 * stringshared names, EDID blob, DPMS property, and the output structure itself.
 *
 * @param dev The Ecore_Drm2_Device (unused, but part of a potential callback signature).
 * @param output The Ecore_Drm2_Output to destroy.
 */
static void
_output_destroy(Ecore_Drm2_Device *dev EINA_UNUSED, Ecore_Drm2_Output *output)
{
   Ecore_Drm2_Output_Mode *mode;
   Ecore_Drm2_Plane *plane;
   Ecore_Drm2_Plane_State *pstate;

   if (_ecore_drm2_use_atomic)
     {
        if (output->prep.atomic_req)
          sym_drmModeAtomicFree(output->prep.atomic_req);

        EINA_LIST_FREE(output->plane_states, pstate)
          free(pstate);

        EINA_LIST_FREE(output->planes, plane)
          free(plane);

        free(output->conn_state);
        free(output->crtc_state);
     }

   EINA_LIST_FREE(output->modes, mode)
     {
        if (mode->id)
          sym_drmModeDestroyPropertyBlob(output->fd, mode->id);
        free(mode);
     }

   eina_stringshare_del(output->backlight.path);
   eina_stringshare_del(output->name);
   eina_stringshare_del(output->make);
   eina_stringshare_del(output->model);
   eina_stringshare_del(output->serial);
   eina_stringshare_del(output->relative.to);

   if (output->flip_timeout) ecore_timer_del(output->flip_timeout);

   sym_drmModeFreeProperty(output->dpms);
   free(output->edid.blob);

   free(output);
}

/**
 * @internal
 * @brief Determines the vblank flags based on the output's CRTC pipe.
 *
 * This function is used to select the correct vblank flags for DRM calls,
 * particularly in multi-GPU or multi-CRTC scenarios.
 * - If pipe > 1, it uses DRM_VBLANK_HIGH_CRTC_MASK.
 * - If pipe == 1, it uses DRM_VBLANK_SECONDARY.
 * - If pipe == 0 (or less), it uses 0 (primary or default).
 *
 * @param output The Ecore_Drm2_Output.
 * @return The DRM vblank flags appropriate for the output's pipe.
 */
static unsigned int
_output_vblank_pipe(Ecore_Drm2_Output *output)
{
   if (output->pipe > 1)
     return ((output->pipe << DRM_VBLANK_HIGH_CRTC_SHIFT) &
             DRM_VBLANK_HIGH_CRTC_MASK);
   else if (output->pipe > 0)
     return DRM_VBLANK_SECONDARY;
   else
     return 0;
}

/**
 * @brief Creates and initializes all outputs for a DRM device.
 *
 * This function discovers all available connectors on the DRM device,
 * creates an Ecore_Drm2_Output for each connected one, and initializes them.
 * It also sets up a udev watch for hotplug events.
 *
 * @param device The Ecore_Drm2_Device to create outputs for.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * @see _output_create()
 * @see _cb_output_event()
 */
EAPI Eina_Bool
ecore_drm2_outputs_create(Ecore_Drm2_Device *device)
{
   drmModeConnector *conn;
   drmModeRes *res;
   int i = 0, x = 0, y = 0, w = 0;
   int events = 0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(device, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((device->fd < 0), EINA_FALSE);

   res = sym_drmModeGetResources(device->fd);
   if (!res) return EINA_FALSE;

   device->crtcs = calloc(res->count_crtcs, sizeof(uint32_t));
   if (!device->crtcs) goto err;

   device->min.width = res->min_width;
   device->min.height = res->min_height;
   device->max.width = res->max_width;
   device->max.height = res->max_height;

   device->num_crtcs = res->count_crtcs;
   memcpy(device->crtcs, res->crtcs, sizeof(uint32_t) * res->count_crtcs);

   for (i = 0; i < res->count_connectors; i++)
     {
        conn = sym_drmModeGetConnector(device->fd, res->connectors[i]);
        if (!conn) continue;

        if (!_output_create(device, res, conn, x, y, &w, EINA_FALSE))
          goto next;

        x += w;

next:
        sym_drmModeFreeConnector(conn);
     }

   if (eina_list_count(device->outputs) < 1) goto err;

   sym_drmModeFreeResources(res);

   events = (EEZE_UDEV_EVENT_ADD | EEZE_UDEV_EVENT_REMOVE |
             EEZE_UDEV_EVENT_CHANGE);

   device->watch =
     eeze_udev_watch_add(EEZE_UDEV_TYPE_DRM, events, _cb_output_event, device);

   return EINA_TRUE;

err:
   sym_drmModeFreeResources(res);
   return EINA_FALSE;
}

/**
 * @brief Destroys all outputs associated with a DRM device.
 *
 * This function iterates through all Ecore_Drm2_Output instances
 * managed by the device and calls _output_destroy() for each one.
 * It also cleans up the udev watch if present.
 *
 * @param device The Ecore_Drm2_Device whose outputs are to be destroyed.
 *
 * @see _output_destroy()
 */
EAPI void
ecore_drm2_outputs_destroy(Ecore_Drm2_Device *device)
{
   Ecore_Drm2_Output *output;

   EINA_SAFETY_ON_NULL_RETURN(device);

   EINA_LIST_FREE(device->outputs, output)
     _output_destroy(device, output);
}

/**
 * @brief Gets the list of outputs for a DRM device.
 *
 * @param device The Ecore_Drm2_Device.
 * @return A const Eina_List of Ecore_Drm2_Output pointers.
 *         The list is owned by the device and should not be modified or freed.
 *         Returns NULL if the device is NULL.
 *
 * Example:
 * @code
 * Ecore_Drm2_Device *dev = ecore_drm2_device_open(...);
 * const Eina_List *outputs = ecore_drm2_outputs_get(dev);
 * Ecore_Drm2_Output *output;
 * Eina_List *l;
 * EINA_LIST_FOREACH(outputs, l, output)
 *   {
 *      // Do something with output
 *   }
 * @endcode
 */
EAPI const Eina_List *
ecore_drm2_outputs_get(Ecore_Drm2_Device *device)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(device, NULL);
   return device->outputs;
}

/**
 * @brief Gets the current DPMS (Display Power Management Signaling) level of an output.
 *
 * @param output The Ecore_Drm2_Output.
 * @return The DPMS level (e.g., DRM_MODE_DPMS_ON, DRM_MODE_DPMS_STANDBY,
 *         DRM_MODE_DPMS_SUSPEND, DRM_MODE_DPMS_OFF), or -1 on error.
 */
EAPI int
ecore_drm2_output_dpms_get(Ecore_Drm2_Output *output)
{
   drmModeObjectProperties *props;
   drmModePropertyRes *prop;
   int val = -1;
   unsigned int i;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, -1);

   props =
     sym_drmModeObjectGetProperties(output->fd, output->conn_id,
                                    DRM_MODE_OBJECT_CONNECTOR);
   if (!props) return -1;

   for (i = 0; i < props->count_props; i++)
     {
        prop = sym_drmModeGetProperty(output->fd, props->props[i]);
        if (!prop) continue;

        if (!strcmp(prop->name, "DPMS"))
          val = props->prop_values[i];

        sym_drmModeFreeProperty(prop);
     }

   sym_drmModeFreeObjectProperties(props);

   return val;
}

/**
 * @brief Sets the DPMS (Display Power Management Signaling) level of an output.
 *
 * @param output The Ecore_Drm2_Output.
 * @param level The desired DPMS level (e.g., DRM_MODE_DPMS_ON,
 *              DRM_MODE_DPMS_OFF).
 *
 * If setting DPMS to ON (level 0), this function may also trigger a flip
 * to ensure the display is active.
 */
EAPI void
ecore_drm2_output_dpms_set(Ecore_Drm2_Output *output, int level)
{
   EINA_SAFETY_ON_NULL_RETURN(output);
   EINA_SAFETY_ON_TRUE_RETURN(!output->enabled);

   sym_drmModeConnectorSetProperty(output->fd, output->conn_id,
                                   output->dpms->prop_id, level);

   if (level == 0) /* DPMS on */
     ecore_drm2_fb_flip(NULL, output);
}

/**
 * @brief Gets the EDID (Extended Display Identification Data) of an output as a hex string.
 *
 * Retrieves the raw EDID data (first 128 bytes) associated with the output
 * and converts it into a hexadecimal string.
 *
 * @param output The Ecore_Drm2_Output.
 * @return A newly allocated string containing the EDID data in hexadecimal format (256 characters + null terminator).
 *         The caller is responsible for freeing this string.
 *         Returns NULL on error or if EDID data is unavailable.
 *
 * Example return value: "00ffffffffffff00..." (256 hex characters)
 */
EAPI char *
ecore_drm2_output_edid_get(Ecore_Drm2_Output *output)
{
   char *edid_str = NULL;
   unsigned char *blob;
   unsigned char fallback_blob[128];

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);

   if (_ecore_drm2_use_atomic)
     blob = output->conn_state->edid.data;
   else
     {
        EINA_SAFETY_ON_NULL_RETURN_VAL(output->edid.blob, NULL);
        blob = output->edid.blob;
     }
   if (!blob)
     {
        memset(fallback_blob, 0, sizeof(fallback_blob));
        blob = fallback_blob;
     }

   edid_str = malloc((128 * 2) + 1);
   if (edid_str)
     {
        unsigned int k, kk;
        const char *hexch = "0123456789abcdef";

        for (kk = 0, k = 0; k < 128; k++)
          {
             edid_str[kk] = hexch[(blob[k] >> 4) & 0xf];
             edid_str[kk + 1] = hexch[blob[k] & 0xf];
             kk += 2;
          }
        edid_str[kk] = 0;
     }

   return edid_str;
}

/**
 * @brief Checks if an output has an associated backlight control.
 *
 * @param output The Ecore_Drm2_Output.
 * @return EINA_TRUE if a backlight control path is available for this output,
 *         EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_drm2_output_backlight_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   return (output->backlight.path != NULL);
}

/**
 * @brief Finds an enabled output that contains the given screen coordinates.
 *
 * Iterates through the enabled outputs of the device and checks if the
 * point (x, y) falls within the output's current mode dimensions and position.
 *
 * @param device The Ecore_Drm2_Device.
 * @param x The X coordinate.
 * @param y The Y coordinate.
 * @return A pointer to the Ecore_Drm2_Output containing the coordinates,
 *         or NULL if no such enabled output is found.
 */
EAPI Ecore_Drm2_Output *
ecore_drm2_output_find(Ecore_Drm2_Device *device, int x, int y)
{
   Eina_List *l;
   Ecore_Drm2_Output *output;

   EINA_SAFETY_ON_NULL_RETURN_VAL(device, NULL);

   EINA_LIST_FOREACH(device->outputs, l, output)
     {
        int ox, oy, ow, oh;

        if (!output->enabled) continue;

        ox = output->x;
        oy = output->y;
        ow = output->current_mode->width;
        oh = output->current_mode->height;

        if (INSIDE(x, y, ox, oy, ow, oh))
          return output;
     }

   return NULL;
}

/**
 * @brief Gets the DPI (Dots Per Inch) of an output.
 *
 * Calculates the horizontal and vertical DPI based on the output's
 * current mode resolution and physical dimensions (from EDID).
 *
 * @param output The Ecore_Drm2_Output.
 * @param[out] xdpi Pointer to store the horizontal DPI. Can be NULL.
 * @param[out] ydpi Pointer to store the vertical DPI. Can be NULL.
 */
EAPI void
ecore_drm2_output_dpi_get(Ecore_Drm2_Output *output, int *xdpi, int *ydpi)
{
   EINA_SAFETY_ON_NULL_RETURN(output);
   EINA_SAFETY_ON_TRUE_RETURN(!output->enabled);

   if (xdpi)
     *xdpi = ((25.4 * (output->current_mode->width)) / output->pw);

   if (ydpi)
     *ydpi = ((25.4 * (output->current_mode->height)) / output->ph);
}

/**
 * @brief Gets the CRTC ID associated with an output.
 *
 * @param output The Ecore_Drm2_Output.
 * @return The CRTC ID, or 0 if output is NULL.
 */
EAPI unsigned int
ecore_drm2_output_crtc_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, 0);
   return output->crtc_id;
}

/**
 * @brief Gets the most recently submitted or active framebuffer for an output.
 *
 * This function checks for framebuffers in the following order:
 * 1. Pending framebuffer (submitted but not yet flipped to).
 * 2. Current framebuffer (currently being displayed).
 * 3. Next framebuffer (prepared for the next flip, if different from pending).
 *
 * @param output The Ecore_Drm2_Output.
 * @return A pointer to the Ecore_Drm2_Fb, or NULL if no framebuffer is associated
 *         or output is NULL.
 */
EAPI Ecore_Drm2_Fb *
ecore_drm2_output_latest_fb_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);
   if (output->pending.fb) return output->pending.fb;
   if (output->current.fb) return output->current.fb;
   return output->next.fb;
}

/**
 * @brief Gets whether an output is marked as primary.
 *
 * @param output The Ecore_Drm2_Output.
 * @return EINA_TRUE if the output is primary, EINA_FALSE otherwise or if output is NULL.
 */
EAPI Eina_Bool
ecore_drm2_output_primary_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   return output->primary;
}

/**
 * @brief Sets an output as primary or not.
 *
 * Note: This function only sets a flag. The caller is responsible for
 * ensuring that only one output is primary if such a constraint is desired.
 *
 * @param output The Ecore_Drm2_Output.
 * @param primary EINA_TRUE to mark as primary, EINA_FALSE otherwise.
 */
EAPI void
ecore_drm2_output_primary_set(Ecore_Drm2_Output *output, Eina_Bool primary)
{
   EINA_SAFETY_ON_NULL_RETURN(output);
   output->primary = primary;
}

/**
 * @brief Gets whether an output is currently enabled.
 *
 * An enabled output is typically connected and has an active mode set.
 *
 * @param output The Ecore_Drm2_Output.
 * @return EINA_TRUE if the output is enabled, EINA_FALSE otherwise or if output is NULL.
 */
EAPI Eina_Bool
ecore_drm2_output_enabled_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   return output->enabled;
}

/**
 * @brief Enables or disables an output.
 *
 * If enabling, sets DPMS to ON.
 * If disabling:
 *  - For atomic KMS, it may trigger a flip with a NULL framebuffer to disable.
 *  - Sets DPMS to OFF.
 *  - Releases any held framebuffer buffers (current, next, pending).
 * An ECORE_DRM2_EVENT_OUTPUT_CHANGED event is sent if the state changes.
 * Does nothing if the output is not connected or already in the desired state.
 *
 * @param output The Ecore_Drm2_Output.
 * @param enabled EINA_TRUE to enable, EINA_FALSE to disable.
 */
EAPI void
ecore_drm2_output_enabled_set(Ecore_Drm2_Output *output, Eina_Bool enabled)
{
   EINA_SAFETY_ON_NULL_RETURN(output);

   if (!output->connected) return;
   if (output->enabled == enabled) return;

   if (enabled)
     {
        output->enabled = enabled;
        ecore_drm2_output_dpms_set(output, DRM_MODE_DPMS_ON);
     }
   else
     {
        if (_ecore_drm2_use_atomic)
          ecore_drm2_fb_flip(NULL, output);

        ecore_drm2_output_dpms_set(output, DRM_MODE_DPMS_OFF);
        output->enabled = enabled;

        if (output->current.fb)
          _ecore_drm2_fb_buffer_release(output, &output->current);

        if (output->next.fb)
          _ecore_drm2_fb_buffer_release(output, &output->next);

        if (output->pending.fb)
          _ecore_drm2_fb_buffer_release(output, &output->pending);
     }

   _output_event_send(output);
}

/**
 * @brief Gets the physical size (in millimeters) of an output.
 *
 * These values are typically read from the EDID.
 *
 * @param output The Ecore_Drm2_Output.
 * @param[out] w Pointer to store the physical width in mm. Can be NULL.
 * @param[out] h Pointer to store the physical height in mm. Can be NULL.
 */
EAPI void
ecore_drm2_output_physical_size_get(Ecore_Drm2_Output *output, int *w, int *h)
{
   if (w) *w = 0;
   if (h) *h = 0;

   EINA_SAFETY_ON_NULL_RETURN(output);

   if (w) *w = output->pw;
   if (h) *h = output->ph;
}

/**
 * @brief Gets the list of available modes for an output.
 *
 * @param output The Ecore_Drm2_Output.
 * @return A const Eina_List of Ecore_Drm2_Output_Mode pointers.
 *         The list is owned by the output and should not be modified or freed.
 *         Returns NULL if output is NULL.
 *
 * Example:
 * @code
 * const Eina_List *modes = ecore_drm2_output_modes_get(output);
 * Ecore_Drm2_Output_Mode *mode;
 * Eina_List *l;
 * EINA_LIST_FOREACH(modes, l, mode)
 *   {
 *      int w, h;
 *      unsigned int refresh, flags;
 *      ecore_drm2_output_mode_info_get(mode, &w, &h, &refresh, &flags);
 *      // Use mode info
 *   }
 * @endcode
 */
EAPI const Eina_List *
ecore_drm2_output_modes_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);
   return output->modes;
}

/**
 * @brief Gets detailed information about a specific display mode.
 *
 * @param mode The Ecore_Drm2_Output_Mode.
 * @param[out] w Pointer to store the width in pixels. Can be NULL.
 * @param[out] h Pointer to store the height in pixels. Can be NULL.
 * @param[out] refresh Pointer to store the refresh rate in Hz. Can be NULL.
 * @param[out] flags Pointer to store mode flags (e.g., DRM_MODE_TYPE_PREFERRED). Can be NULL.
 */
EAPI void
ecore_drm2_output_mode_info_get(Ecore_Drm2_Output_Mode *mode, int *w, int *h, unsigned int *refresh, unsigned int *flags)
{
   if (w) *w = 0;
   if (h) *h = 0;
   if (refresh) *refresh = 0;
   if (flags) *flags = 0;

   EINA_SAFETY_ON_NULL_RETURN(mode);

   if (w) *w = mode->width;
   if (h) *h = mode->height;
   if (refresh) *refresh = mode->refresh;
   if (flags) *flags = mode->flags;
}

/**
 * @internal
 * @brief Sets the display mode for an output using an atomic KMS commit.
 *
 * This function constructs an atomic request to change the mode of the CRTC
 * associated with the output. If `mode` is NULL, it deactivates the CRTC.
 * Otherwise, it creates a blob for the new mode info (if not already created)
 * and sets the CRTC's MODE_ID and ACTIVE properties.
 *
 * @param output The Ecore_Drm2_Output whose mode is to be set.
 * @param mode The Ecore_Drm2_Output_Mode to set. If NULL, the CRTC will be deactivated.
 * @return EINA_TRUE on successful commit, EINA_FALSE on failure.
 */
static Eina_Bool
_output_mode_atomic_set(Ecore_Drm2_Output *output, Ecore_Drm2_Output_Mode *mode)
{
   Ecore_Drm2_Crtc_State *cstate;
   drmModeAtomicReq *req = NULL;
   int ret = 0;

   cstate = output->crtc_state;

   if (mode)
     {
        if (mode->id)
          sym_drmModeDestroyPropertyBlob(output->fd, mode->id);

        ret =
          sym_drmModeCreatePropertyBlob(output->fd, &mode->info,
                                        sizeof(drmModeModeInfo), &mode->id);
        if (ret < 0)
          {
             ERR("Failed to create Mode Property Blob");
             return EINA_FALSE;
          }
     }

   req = sym_drmModeAtomicAlloc();
   if (!req) return EINA_FALSE;

   sym_drmModeAtomicSetCursor(req, 0);

   if (mode)
     {
        cstate->active.value = 1;
        cstate->mode.value = mode->id;
     }
   else
     cstate->active.value = 0;

   ret = sym_drmModeAtomicAddProperty(req, cstate->obj_id, cstate->mode.id,
                                      cstate->mode.value);
   if (ret < 0)
     {
        ERR("Could not add atomic property");
        ret = EINA_FALSE;
        goto err;
     }

   ret = sym_drmModeAtomicAddProperty(req, cstate->obj_id,
                                      cstate->active.id, cstate->active.value);
   if (ret < 0)
     {
        ERR("Could not add atomic property");
        ret = EINA_FALSE;
        goto err;
     }

   if (cstate->background.id)
     {
        ret =
          sym_drmModeAtomicAddProperty(req, cstate->obj_id,
                                       cstate->background.id,
                                       cstate->background.value);
        if (ret < 0)
          {
             ERR("Could not add atomic property");
             ret = EINA_FALSE;
             goto err;
          }
     }

   ret = sym_drmModeAtomicCommit(output->fd, req, DRM_MODE_ATOMIC_ALLOW_MODESET,
                                 output);
   if (ret < 0)
     {
        ERR("Failed to commit atomic Mode: %m");
        ret = EINA_FALSE;
        goto err;
     }
   else
     ret = EINA_TRUE;

err:
   sym_drmModeAtomicFree(req);
   return ret;
}

/**
 * @brief Sets the display mode for an output.
 *
 * This function changes the active display mode of the specified output.
 * It also updates the output's logical position (x, y) and its current_mode pointer.
 *
 * If `mode` is NULL, this function attempts to turn off the output by setting
 * a NULL mode on its CRTC.
 *
 * Internally, this uses either atomic KMS operations (if `_ecore_drm2_use_atomic` is true)
 * via `_output_mode_atomic_set`, or legacy `drmModeSetCrtc`.
 *
 * @param output The Ecore_Drm2_Output whose mode is to be set.
 * @param mode The Ecore_Drm2_Output_Mode to set. If NULL, attempts to disable the output.
 * @param x The new X position for the output.
 * @param y The new Y position for the output.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 *
 * @see _output_mode_atomic_set()
 * @see drmModeSetCrtc()
 */
EAPI Eina_Bool
ecore_drm2_output_mode_set(Ecore_Drm2_Output *output, Ecore_Drm2_Output_Mode *mode, int x, int y)
{
   Eina_Bool ret = EINA_TRUE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((output->fd < 0), EINA_FALSE);

   output->x = x;
   output->y = y;
   output->current_mode = mode;

   if (_ecore_drm2_use_atomic)
     ret = _output_mode_atomic_set(output, mode);
   else
     {
        if (mode)
          {
             unsigned int buffer = 0;

             if (output->current.fb)
               buffer = output->current.fb->id;
             else if (output->next.fb)
               buffer = output->next.fb->id;
             else
               buffer = output->ocrtc->buffer_id;

             if (sym_drmModeSetCrtc(output->fd, output->crtc_id, buffer, 0, 0,
                                    &output->conn_id, 1, &mode->info) < 0)
               {
                  ERR("Failed to set Mode %dx%d for Output %s: %m",
                      mode->width, mode->height, output->name);
                  ret = EINA_FALSE;
               }
          }
        else
          {
             if (sym_drmModeSetCrtc(output->fd, output->crtc_id, 0,
                                    0, 0, 0, 0, NULL) < 0)
               {
                  ERR("Failed to turn off Output %s: %m", output->name);
                  ret = EINA_FALSE;
               }
          }
     }

   return ret;
}

/**
 * @brief Gets the name of an output.
 *
 * The name is typically generated (e.g., "HDMI-A-1", "DP-1").
 *
 * @param output The Ecore_Drm2_Output.
 * @return A newly allocated string containing the output's name.
 *         The caller is responsible for freeing this string.
 *         Returns NULL if output or its name is NULL.
 */
EAPI char *
ecore_drm2_output_name_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->name, NULL);
   return strdup(output->name);
}

/**
 * @brief Gets the model name of an output.
 *
 * The model name is typically derived from EDID information.
 *
 * @param output The Ecore_Drm2_Output.
 * @return A newly allocated string containing the output's model name.
 *         The caller is responsible for freeing this string.
 *         Returns NULL if output or its model name is NULL.
 */
EAPI char *
ecore_drm2_output_model_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->model, NULL);
   return strdup(output->model);
}

/**
 * @brief Gets whether an output is physically connected.
 *
 * @param output The Ecore_Drm2_Output.
 * @return EINA_TRUE if the output is connected, EINA_FALSE otherwise or if output is NULL.
 */
EAPI Eina_Bool
ecore_drm2_output_connected_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   return output->connected;
}

/**
 * @brief Gets whether an output is considered cloned.
 *
 * An output is cloned if its `cloned` flag is set or its
 * `relative.mode` is ECORE_DRM2_RELATIVE_MODE_CLONE.
 *
 * @param output The Ecore_Drm2_Output.
 * @return EINA_TRUE if the output is cloned, EINA_FALSE otherwise or if output is NULL.
 */
EAPI Eina_Bool
ecore_drm2_output_cloned_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   return (output->cloned ||
           output->relative.mode == ECORE_DRM2_RELATIVE_MODE_CLONE);
}

/**
 * @brief Gets the connector type of an output.
 *
 * @param output The Ecore_Drm2_Output.
 * @return The DRM connector type (e.g., DRM_MODE_CONNECTOR_HDMIA,
 *         DRM_MODE_CONNECTOR_DisplayPort), or 0 if output is NULL.
 *         See drm_mode.h for a list of connector types.
 */
EAPI unsigned int
ecore_drm2_output_connector_type_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, 0);
   return output->conn_type;
}

/**
 * @brief Checks if a given CRTC can be used by an output.
 *
 * This function determines if the specified `crtc` is among the
 * `possible_crtcs` for any encoder connected to the given `output`.
 *
 * @param output The Ecore_Drm2_Output.
 * @param crtc The CRTC ID to check.
 * @return EINA_TRUE if the CRTC can be used by the output, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_drm2_output_possible_crtc_get(Ecore_Drm2_Output *output, unsigned int crtc)
{
   drmModeRes *res;
   drmModeConnector *conn;
   drmModeEncoder *enc;
   int i = 0, j = 0, k = 0;
   unsigned int p = 0;
   Eina_Bool ret = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((output->fd < 0), EINA_FALSE);

   res = sym_drmModeGetResources(output->fd);
   if (!res) return EINA_FALSE;

   for (; i < res->count_connectors; i++)
     {
        conn = sym_drmModeGetConnector(output->fd, res->connectors[i]);
        if (!conn) continue;

        for (j = 0; j < conn->count_encoders; j++)
          {
             enc = sym_drmModeGetEncoder(output->fd, conn->encoders[j]);
             if (!enc) continue;

             if (enc->crtc_id != crtc) goto next;

             p = enc->possible_crtcs;

             for (k = 0; k < res->count_crtcs; k++)
               {
                  if (res->crtcs[k] != output->crtc_id) continue;

                  if (p & (1ULL << k))
                    {
                       ret = EINA_TRUE;
                       break;
                    }
               }

next:
             sym_drmModeFreeEncoder(enc);
             if (ret) break;
          }

        sym_drmModeFreeConnector(conn);
        if (ret) break;
     }

   sym_drmModeFreeResources(res);

   return ret;
}

/**
 * @brief Sets user data associated with an output.
 *
 * Allows associating arbitrary user-defined data with an Ecore_Drm2_Output.
 *
 * @param o The Ecore_Drm2_Output.
 * @param data Pointer to the user data.
 */
EAPI void
ecore_drm2_output_user_data_set(Ecore_Drm2_Output *o, void *data)
{
   EINA_SAFETY_ON_NULL_RETURN(o);

   o->user_data = data;
}

/**
 * @brief Gets user data associated with an output.
 *
 * @param output The Ecore_Drm2_Output.
 * @return Pointer to the user data, or NULL if no data is set or output is NULL.
 */
EAPI void *
ecore_drm2_output_user_data_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);
   return output->user_data;
}

/**
 * @brief Sets the gamma ramps for an output.
 *
 * Applies new gamma correction values to the CRTC associated with the output.
 * The size of the provided ramps must match the gamma_size supported by the CRTC.
 *
 * @param output The Ecore_Drm2_Output.
 * @param size The number of entries in each gamma ramp array.
 * @param red Array of red channel gamma values.
 * @param green Array of green channel gamma values.
 * @param blue Array of blue channel gamma values.
 *
 * Example for `red`, `green`, `blue` arrays (size = 256):
 * @code
 * uint16_t ramp[256];
 * for (int i = 0; i < 256; i++) ramp[i] = (i << 8) | i; // Linear ramp
 * ecore_drm2_output_gamma_set(output, 256, ramp, ramp, ramp);
 * @endcode
 */
EAPI void
ecore_drm2_output_gamma_set(Ecore_Drm2_Output *output, uint16_t size, uint16_t *red, uint16_t *green, uint16_t *blue)
{
   EINA_SAFETY_ON_NULL_RETURN(output);
   EINA_SAFETY_ON_TRUE_RETURN(output->fd < 0);

   if (output->gamma != size) return;

   if (sym_drmModeCrtcSetGamma(output->fd, output->crtc_id, size,
                               red, green, blue) < 0)
     ERR("Failed to set gamma for Output %s: %m", output->name);
}

/**
 * @brief Gets the supported rotations for an output's primary plane.
 *
 * If atomic mode setting is used, this queries the `IN_FORMATS` property
 * of the primary plane associated with the output to determine supported rotations.
 * Otherwise, it returns a default set of supported rotations (normal, 90, 180, 270).
 *
 * @param output The Ecore_Drm2_Output.
 * @return A bitmask of supported ECORE_DRM2_ROTATION flags
 *         (e.g., ECORE_DRM2_ROTATION_NORMAL | ECORE_DRM2_ROTATION_90).
 *         Returns -1 on error or if output is NULL.
 *
 * The rotation flags map to DRM_MODE_ROTATE_* and DRM_MODE_REFLECT_* bits.
 * For example:
 *  - ECORE_DRM2_ROTATION_NORMAL (1 << 0) -> DRM_MODE_ROTATE_0 (1 << 0)
 *  - ECORE_DRM2_ROTATION_90   (1 << 1) -> DRM_MODE_ROTATE_90 (1 << 1)
 *  - ECORE_DRM2_ROTATION_180  (1 << 2) -> DRM_MODE_ROTATE_180 (1 << 2)
 *  - ECORE_DRM2_ROTATION_270  (1 << 3) -> DRM_MODE_ROTATE_270 (1 << 3)
 *  - ECORE_DRM2_ROTATION_REFLECT_X (1 << 4) -> DRM_MODE_REFLECT_X (1 << 4)
 *  - ECORE_DRM2_ROTATION_REFLECT_Y (1 << 5) -> DRM_MODE_REFLECT_Y (1 << 5)
 */
EAPI int
ecore_drm2_output_supported_rotations_get(Ecore_Drm2_Output *output)
{
   int ret = -1;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, -1);

   if (_ecore_drm2_use_atomic)
     {
        Ecore_Drm2_Plane_State *pstate;
        Eina_List *l;

        EINA_LIST_FOREACH(output->plane_states, l, pstate)
          {
             if (pstate->type.value != DRM_PLANE_TYPE_PRIMARY) continue;
             ret = pstate->supported_rotations;
             break;
          }
     }
   else
     return (ECORE_DRM2_ROTATION_NORMAL | ECORE_DRM2_ROTATION_90 |
             ECORE_DRM2_ROTATION_180 | ECORE_DRM2_ROTATION_270);

   return ret;
}

/**
 * @brief Sets the rotation for an output.
 *
 * This function primarily updates the `output->rotation` field.
 * Hardware plane rotation via atomic KMS is currently disabled in the code
 * (see #if 0 block). If it were enabled, it would attempt to set the
 * 'rotation' property on the primary plane.
 *
 * The actual visual rotation is typically handled by software (compositor)
 * using the value retrieved by `ecore_drm2_output_rotation_get()`.
 *
 * @param output The Ecore_Drm2_Output.
 * @param rotation The desired rotation, a bitmask of ECORE_DRM2_ROTATION flags.
 *                 Only one rotation value (0, 90, 180, 270) should be combined
 *                 with reflection flags if needed.
 * @return EINA_TRUE if the rotation value was set internally. If hardware
 *         rotation were active, it would return EINA_TRUE on successful commit.
 *         Returns EINA_FALSE if output is NULL.
 */
EAPI Eina_Bool
ecore_drm2_output_rotation_set(Ecore_Drm2_Output *output, int rotation)
{
   Eina_Bool ret = EINA_TRUE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);

   output->rotation = rotation;

#if 0
   /* XXX: Disable hardware plane rotation for now as this has broken
    * recently. The break happens because of an invalid argument,
    * ie: the value being sent from pstate->rotation_map ends up being
    * incorrect for some reason. I suspect the breakage to be from
    * kernel drivers (linux 4.20.0) but have not confirmed that version */
   if (_ecore_drm2_use_atomic)
     {
        Eina_List *l;
        Ecore_Drm2_Plane_State *pstate = NULL;
        drmModeAtomicReq *req = NULL;
        int res = 0;
        uint32_t flags = DRM_MODE_ATOMIC_NONBLOCK |
          DRM_MODE_ATOMIC_ALLOW_MODESET;

        EINA_LIST_FOREACH(output->plane_states, l, pstate)
          {
             if (pstate->type.value != DRM_PLANE_TYPE_PRIMARY) continue;

             if ((pstate->supported_rotations & rotation) == 0)
               {
                  WRN("Unsupported rotation");
                  return EINA_FALSE;
               }

             req = sym_drmModeAtomicAlloc();
             if (!req) return EINA_FALSE;

             sym_drmModeAtomicSetCursor(req, 0);

             res = sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                                pstate->rotation.id, rotation);
             if (res < 0) goto err;

             res = sym_drmModeAtomicCommit(output->fd, req, flags, output);
             if (res < 0)
               goto err;
             else
               {
                  ret = EINA_TRUE;
                  pstate->rotation.value = rotation;
               }
          }

err:
        sym_drmModeAtomicFree(req);
     }
#endif

   return ret;
}

/**
 * @brief Gets the current rotation set for an output.
 *
 * @param output The Ecore_Drm2_Output.
 * @return The current rotation value (ECORE_DRM2_ROTATION flags),
 *         or -1 if output is NULL.
 */
EAPI int
ecore_drm2_output_rotation_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, -1);
   return output->rotation;
}

/**
 * @brief Gets the subpixel order of an output.
 *
 * This corresponds to Wayland's wl_output_subpixel enum.
 *
 * @param output The Ecore_Drm2_Output.
 * @return The subpixel order value (e.g., WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB),
 *         or 0 (WL_OUTPUT_SUBPIXEL_UNKNOWN) if output is NULL or unknown.
 *
 * Possible return values (matching Wayland):
 *  - 0: Unknown
 *  - 1: None
 *  - 2: Horizontal RGB
 *  - 3: Horizontal BGR
 *  - 4: Vertical RGB
 *  - 5: Vertical BGR
 */
EAPI unsigned int
ecore_drm2_output_subpixel_get(const Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, 0);
   return output->subpixel;
}

/**
 * @internal
 * @brief Page flip handler for vblank timestamp fallback mechanism.
 *
 * This function is called when a page flip event occurs as part of the
 * `_blanktime_fallback` mechanism. It stores the timestamp of the flip.
 *
 * @param fd File descriptor (unused).
 * @param frame Frame number (unused).
 * @param sec Seconds part of the timestamp.
 * @param usec Microseconds part of the timestamp.
 * @param data User data, expected to be an Ecore_Drm2_Output pointer.
 */
static void
_blank_fallback_handler(int fd EINA_UNUSED, unsigned int frame EINA_UNUSED, unsigned int sec, unsigned int usec, void *data EINA_UNUSED)
{
   Ecore_Drm2_Output *output;

   output = data;
   output->fallback_usec = usec;
   output->fallback_sec = sec;
}

/**
 * @internal
 * @brief Fallback mechanism to get vblank timestamp using a page flip.
 *
 * This function is used when `drmWaitVBlank` fails or returns an invalid timestamp.
 * It attempts to get a timestamp by scheduling a page flip event on the current
 * framebuffer (if available and no other flip is pending).
 * This is a workaround for drivers or situations where `drmWaitVBlank` is unreliable.
 *
 * @param output The Ecore_Drm2_Output.
 * @param sequence The requested vblank sequence (currently only supports 1).
 * @param[out] sec Pointer to store the seconds part of the timestamp.
 * @param[out] usec Pointer to store the microseconds part of the timestamp.
 * @return 0 on success, -1 on failure.
 */
static int
_blanktime_fallback(Ecore_Drm2_Output *output, int sequence, long *sec, long *usec)
{
   drmEventContext ctx;
   int ret;

   /* Too lazy to loop for > 1, and don't want to block for < 1 */
   if (sequence != 1) return -1;

   /* If we got here with a flip waiting to complete we can do nothing. */
   if (output->pending.fb) return -1;

   if (!output->current.fb) return -1;

   memset(&ctx, 0, sizeof(ctx));
   ctx.version = 2;
   ctx.page_flip_handler = _blank_fallback_handler;
   ctx.vblank_handler = NULL;

   ret = sym_drmModePageFlip(output->current.fb->fd, output->crtc_id,
                             output->current.fb->id, DRM_MODE_PAGE_FLIP_EVENT,
                             output);
   if (ret < 0) return -1;
   do
     {
        ret = sym_drmHandleEvent(output->current.fb->fd, &ctx);
     } while (ret != 0 && errno == EAGAIN);
   if (ret < 0) return -1;

   *sec = output->fallback_sec;
   *usec = output->fallback_usec;
   return 0;
}

/**
 * @brief Gets the timestamp of a future vblank event.
 *
 * Attempts to get the timestamp for a specified vblank sequence number
 * using `drmWaitVBlank`. If that fails or returns an invalid timestamp,
 * it tries a fallback mechanism (`_blanktime_fallback`) which uses a page flip.
 *
 * @param output The Ecore_Drm2_Output.
 * @param sequence The desired vblank sequence number (relative to current).
 *                 Typically 1 for the next vblank.
 * @param[out] sec Pointer to store the seconds part of the vblank timestamp.
 * @param[out] usec Pointer to store the microseconds part of the vblank timestamp.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_drm2_output_blanktime_get(Ecore_Drm2_Output *output, int sequence, long *sec, long *usec)
{
   drmVBlank v;
   int ret;
   Eina_Bool success;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(sec, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(usec, EINA_FALSE);

   memset(&v, 0, sizeof(v));
   v.request.type = DRM_VBLANK_RELATIVE;
   v.request.type |= _output_vblank_pipe(output);
   v.request.sequence = sequence;
   ret = sym_drmWaitVBlank(output->fd, &v);
   success = (ret == 0) && (v.reply.tval_sec > 0 || v.reply.tval_usec > 0);
   if (!success)
     {
        ret = _blanktime_fallback(output, sequence, sec, usec);
        if (ret) return EINA_FALSE;
        return EINA_TRUE;
     }

   *sec = v.reply.tval_sec;
   *usec = v.reply.tval_usec;
   return EINA_TRUE;
}

/**
 * @brief Gets current information about an output's geometry and refresh rate.
 *
 * Retrieves the output's current position (x, y), dimensions (width, height),
 * and refresh rate. The dimensions are adjusted based on the output's rotation
 * (e.g., for 90/270 degree rotations, width and height are swapped).
 *
 * @param output The Ecore_Drm2_Output.
 * @param[out] x Pointer to store the X position. Can be NULL.
 * @param[out] y Pointer to store the Y position. Can be NULL.
 * @param[out] w Pointer to store the width in pixels (adjusted for rotation). Can be NULL.
 * @param[out] h Pointer to store the height in pixels (adjusted for rotation). Can be NULL.
 * @param[out] refresh Pointer to store the refresh rate in Hz. Can be NULL.
 */
EAPI void
ecore_drm2_output_info_get(Ecore_Drm2_Output *output, int *x, int *y, int *w, int *h, unsigned int *refresh)
{
   if (x) *x = 0;
   if (y) *y = 0;
   if (w) *w = 0;
   if (h) *h = 0;
   if (refresh) *refresh = 0;

   EINA_SAFETY_ON_NULL_RETURN(output);
   EINA_SAFETY_ON_TRUE_RETURN(!output->current_mode);

   switch (output->rotation)
     {
      case ECORE_DRM2_ROTATION_90:
      case ECORE_DRM2_ROTATION_270:
        if (w) *w = output->current_mode->height;
        if (h) *h = output->current_mode->width;
        break;
      case ECORE_DRM2_ROTATION_NORMAL:
      case ECORE_DRM2_ROTATION_180:
      default:
        if (w) *w = output->current_mode->width;
        if (h) *h = output->current_mode->height;
        break;
     }

   if (refresh) *refresh = output->current_mode->refresh;
   if (x) *x = output->x;
   if (y) *y = output->y;
}

/**
 * @brief Checks if an output has a pending page flip.
 *
 * A pending page flip means a framebuffer has been submitted for display
 * but the display hardware has not yet switched to it.
 *
 * @param output The Ecore_Drm2_Output.
 * @return EINA_TRUE if there is a pending framebuffer, EINA_FALSE otherwise
 *         or if output is NULL.
 */
EAPI Eina_Bool
ecore_drm2_output_pending_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);

   if (output->pending.fb) return EINA_TRUE;

   return EINA_FALSE;
}

/**
 * @brief Sets the relative positioning mode for an output.
 *
 * This is used to describe how an output is positioned relative to another,
 * e.g., cloned, to the right of, etc. This is typically for higher-level
 * display configuration logic.
 *
 * @param output The Ecore_Drm2_Output.
 * @param mode The Ecore_Drm2_Relative_Mode to set.
 */
EAPI void
ecore_drm2_output_relative_mode_set(Ecore_Drm2_Output *output, Ecore_Drm2_Relative_Mode mode)
{
   EINA_SAFETY_ON_NULL_RETURN(output);
   output->relative.mode = mode;
}

/**
 * @brief Gets the relative positioning mode of an output.
 *
 * @param output The Ecore_Drm2_Output.
 * @return The Ecore_Drm2_Relative_Mode of the output,
 *         or ECORE_DRM2_RELATIVE_MODE_UNKNOWN if output is NULL.
 */
EAPI Ecore_Drm2_Relative_Mode
ecore_drm2_output_relative_mode_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, ECORE_DRM2_RELATIVE_MODE_UNKNOWN);
   return output->relative.mode;
}

/**
 * @brief Sets the name of the output to which this output is positioned relatively.
 *
 * Used in conjunction with `ecore_drm2_output_relative_mode_set`.
 * The `relative` string should be the name of another Ecore_Drm2_Output.
 *
 * @param output The Ecore_Drm2_Output.
 * @param relative The name of the output this one is relative to.
 *                 The string is stringshared.
 */
EAPI void
ecore_drm2_output_relative_to_set(Ecore_Drm2_Output *output, const char *relative)
{
   EINA_SAFETY_ON_NULL_RETURN(output);
   eina_stringshare_replace(&output->relative.to, relative);
}

/**
 * @brief Gets the name of the output to which this output is positioned relatively.
 *
 * @param output The Ecore_Drm2_Output.
 * @return A const char pointer to the name of the relative output (stringshared).
 *         Returns NULL if not set or if output is NULL.
 */
EAPI const char *
ecore_drm2_output_relative_to_get(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);
   return output->relative.to;
}

/**
 * @brief Sets the background color for a CRTC using atomic KMS.
 *
 * This function updates the 'BACKGROUND' property of the CRTC associated
 * with the output. The color components are 16-bit values.
 * This requires atomic mode-setting capabilities and that the CRTC
 * supports the 'BACKGROUND' property.
 * After setting the color, it attempts to test the atomic commit.
 *
 * @param output The Ecore_Drm2_Output whose CRTC background color is to be set.
 * @param r Red component (0-65535).
 * @param g Green component (0-65535).
 * @param b Blue component (0-65535).
 * @param a Alpha component (0-65535, typically opaque 65535).
 * @return EINA_TRUE if the background color property was updated and the atomic test commit
 *         was successful (or would be successful), EINA_FALSE otherwise (e.g., no atomic state,
 *         no background property, or commit test failure).
 *
 * @note This function relies on `_fb_atomic_flip_test` to apply the change,
 *       which might not be the most direct way if only background color is changing.
 *       The actual commit happens as part of a flip operation.
 */
EAPI Eina_Bool
ecore_drm2_output_background_color_set(Ecore_Drm2_Output *output, uint64_t r, uint64_t g, uint64_t b, uint64_t a)
{
   Ecore_Drm2_Crtc_State *cstate;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->crtc_state, EINA_FALSE);

   cstate = output->crtc_state;
   if (cstate->background.id)
     {
        cstate->background.value = (a << 48 | b << 32 | g << 16 | r);
        return _fb_atomic_flip_test(output);
     }

   return EINA_FALSE;
}
