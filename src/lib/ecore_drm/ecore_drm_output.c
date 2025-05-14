/* Portions of this code have been derived from Weston
 *
 * Copyright © 2008-2012 Kristian Høgsberg
 * Copyright © 2010-2012 Intel Corporation
 * Copyright © 2010-2011 Benjamin Franzke
 * Copyright © 2011-2012 Collabora, Ltd.
 * Copyright © 2010 Red Hat <mjg@redhat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "ecore_drm_private.h"
#include <ctype.h>

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
   "DisplayPort", "HDMI-A", "HDMI-B", "TV", "eDP", "Virtual",
   "DSI",
};

EAPI int ECORE_DRM_EVENT_OUTPUT = 0; /**< Event type for output plug/unplug events */

/**
 * @internal
 * @brief Frees an Ecore_Drm_Event_Output structure.
 *
 * This function is called by Ecore when an ECORE_DRM_EVENT_OUTPUT event
 * is no longer needed. It frees the memory allocated for the event structure
 * and its stringshare members.
 *
 * @param data User data associated with the event (unused).
 * @param event Pointer to the Ecore_Drm_Event_Output structure to free.
 */
static void
_ecore_drm_output_event_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Drm_Event_Output *e = event;

   eina_stringshare_del(e->make);
   eina_stringshare_del(e->model);
   eina_stringshare_del(e->name);
   free(event);
}

/**
 * @internal
 * @brief Creates and sends an ECORE_DRM_EVENT_OUTPUT event.
 *
 * This function is called when an output is plugged or unplugged. It
 * populates an Ecore_Drm_Event_Output structure with information from the
 * given Ecore_Drm_Output and adds it to the Ecore event queue.
 *
 * @param output The output that triggered the event.
 * @param plug EINA_TRUE if the output was plugged in, EINA_FALSE if unplugged.
 */
static void
_ecore_drm_output_event_send(const Ecore_Drm_Output *output, Eina_Bool plug)
{
   Ecore_Drm_Event_Output *e;

   if (!(e = calloc(1, sizeof(Ecore_Drm_Event_Output)))) return;
   e->plug = plug;
   e->id = output->crtc_id;

   if (output->current_mode)
     {
        e->w = output->current_mode->width;
        e->h = output->current_mode->height;
        e->refresh = output->current_mode->refresh;
     }
   else if (output->crtc)
     {
        e->w = output->crtc->width;
        e->h = output->crtc->height;
     }

   e->x = output->x;
   e->y = output->y;
   e->phys_width = output->phys_width;
   e->phys_height = output->phys_height;
   e->subpixel_order = output->subpixel;
   e->make = eina_stringshare_ref(output->make);
   e->model = eina_stringshare_ref(output->model);
   e->name = eina_stringshare_ref(output->name);
   e->transform = 0;
   ecore_event_add(ECORE_DRM_EVENT_OUTPUT, e,
                   _ecore_drm_output_event_free, NULL);
}

/**
 * @internal
 * @brief Retrieves a DRM property by name for a given connector.
 *
 * Iterates through the properties of a connector and returns the one
 * matching the specified name.
 *
 * @param fd The DRM device file descriptor.
 * @param conn The DRM connector.
 * @param name The name of the property to retrieve.
 * @return A pointer to the drmModePropertyPtr if found, otherwise NULL.
 *         The caller is responsible for freeing the returned property using
 *         drmModeFreeProperty().
 */
static drmModePropertyPtr
_ecore_drm_output_property_get(int fd, drmModeConnectorPtr conn, const char *name)
{
   drmModePropertyPtr prop;
   int i = 0;

   for (; i < conn->count_props; i++)
     {
        if (!(prop = drmModeGetProperty(fd, conn->props[i])))
          continue;

        if (!strcmp(prop->name, name)) return prop;

        drmModeFreeProperty(prop);
     }

   return NULL;
}

/**
 * @internal
 * @brief Parses a string from EDID data.
 *
 * Copies up to 12 bytes from the EDID data into the text buffer.
 * It replaces non-printable characters with '-' and ensures the string
 * is null-terminated. If more than 4 characters are replaced, the
 * resulting string is considered invalid and set to empty.
 *
 * @param data Pointer to the EDID data block containing the string.
 * @param text Output buffer (should be at least 13 bytes) to store the parsed string.
 */
static void
_ecore_drm_output_edid_parse_string(const uint8_t *data, char text[])
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
 * @brief Parses EDID data to extract monitor information.
 *
 * Extracts PNP ID, serial number, monitor name, and EISA ID from the
 * raw EDID data.
 *
 * @param output The Ecore_Drm_Output to populate with EDID information.
 * @param data Raw EDID data buffer.
 * @param len Length of the EDID data buffer.
 * @return 0 on success, -1 on failure (e.g., invalid EDID header).
 */
static int
_ecore_drm_output_edid_parse(Ecore_Drm_Output *output, const uint8_t *data, size_t len)
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
          _ecore_drm_output_edid_parse_string(&data[i + 5], output->edid.monitor);
        else if (data[i + 3] == EDID_DESCRIPTOR_DISPLAY_PRODUCT_SERIAL_NUMBER)
          _ecore_drm_output_edid_parse_string(&data[i + 5], output->edid.serial);
        else if (data[i + 3] == EDID_DESCRIPTOR_ALPHANUMERIC_DATA_STRING)
          _ecore_drm_output_edid_parse_string(&data[i + 5], output->edid.eisa);
     }

   return 0;
}

/**
 * @internal
 * @brief Finds and parses the EDID information for an output.
 *
 * Retrieves the EDID blob property from the DRM connector, duplicates it,
 * and then parses it to populate the make and model fields of the
 * Ecore_Drm_Output structure.
 *
 * @param output The Ecore_Drm_Output to populate.
 * @param conn The DRM connector to get EDID from.
 */
static void
_ecore_drm_output_edid_find(Ecore_Drm_Output *output, drmModeConnector *conn)
{
   drmModePropertyBlobPtr blob = NULL;
   drmModePropertyPtr prop;
   int i = 0, ret = 0;

   for (; i < conn->count_props && !blob; i++)
     {
        if (!(prop = drmModeGetProperty(output->dev->drm.fd, conn->props[i])))
          continue;
        if ((prop->flags & DRM_MODE_PROP_BLOB) &&
            (!strcmp(prop->name, "EDID")))
          {
             blob = drmModeGetPropertyBlob(output->dev->drm.fd,
                                           conn->prop_values[i]);
          }
        drmModeFreeProperty(prop);
        if (blob) break;
     }

   if (!blob) return;

   output->edid_blob = eina_memdup(blob->data, blob->length, 1);

   ret = _ecore_drm_output_edid_parse(output, blob->data, blob->length);
   if (!ret)
     {
        if (output->edid.pnp[0] != '\0')
          eina_stringshare_replace(&output->make, output->edid.pnp);
        if (output->edid.monitor[0] != '\0')
          eina_stringshare_replace(&output->model, output->edid.monitor);
        /* if (output->edid.serial[0] != '\0') */
        /*   eina_stringshare_replace(&output->serial, output->edid.serial); */
     }

   drmModeFreePropertyBlob(blob);
}

/**
 * @internal
 * @brief Placeholder for software rendering logic.
 *
 * This function is intended to handle rendering when hardware acceleration
 * is not available or not used. Currently, it's a stub.
 *
 * @param output The output to render on.
 */
static void
_ecore_drm_output_software_render(Ecore_Drm_Output *output)
{
   if (!output) return;
   if (!output->current_mode) return;
}

/**
 * @internal
 * @brief Finds an available CRTC for a given connector.
 *
 * Iterates through encoders associated with the connector and their
 * possible CRTCs to find one that is not already allocated.
 *
 * @param dev The Ecore_Drm_Device.
 * @param res The DRM resources.
 * @param conn The DRM connector.
 * @return The index of a suitable CRTC in res->crtcs, or -1 if none is found.
 */
static int
_ecore_drm_output_crtc_find(Ecore_Drm_Device *dev, drmModeRes *res, drmModeConnector *conn)
{
   drmModeEncoder *enc;
   unsigned int p;
   int i, j;

   /* We did not find an existing encoder + crtc combination. Loop through all of them until we
    * find the first working combination */
   for (j = 0; j < conn->count_encoders; j++)
     {
        /* get the encoder on this connector */
        if (!(enc = drmModeGetEncoder(dev->drm.fd, conn->encoders[j])))
          {
             WRN("Failed to get encoder");
             continue;
          }

        p = enc->possible_crtcs;
        drmModeFreeEncoder(enc);

	/* Walk over all CRTCs */
        for (i = 0; i < res->count_crtcs; i++)
          {
             /* Does the CRTC match the list of possible CRTCs from the encoder? */
             if ((p & (1 << i)) &&
                 (!(dev->crtc_allocator & (1 << res->crtcs[i]))))
               {
                  return i;
               }
          }
     }

   return -1;
}

/**
 * @internal
 * @brief Creates an Ecore_Drm_Output_Mode from drmModeModeInfo and adds it to the output.
 *
 * Calculates the refresh rate and populates an Ecore_Drm_Output_Mode structure.
 * The new mode is then appended to the output's list of modes.
 *
 * @param output The Ecore_Drm_Output to add the mode to.
 * @param info The drmModeModeInfo structure describing the mode.
 * @return A pointer to the newly created Ecore_Drm_Output_Mode, or NULL on allocation failure.
 *         The returned mode is owned by the output's modes list.
 */
static Ecore_Drm_Output_Mode *
_ecore_drm_output_mode_add(Ecore_Drm_Output *output, drmModeModeInfo *info)
{
   Ecore_Drm_Output_Mode *mode;
   uint64_t refresh;

   /* try to allocate space for mode */
   if (!(mode = malloc(sizeof(Ecore_Drm_Output_Mode))))
     {
        ERR("Could not allocate space for mode");
        return NULL;
     }

   mode->flags = 0;
   mode->width = info->hdisplay;
   mode->height = info->vdisplay;

   refresh = (info->clock * 1000LL / info->htotal + info->vtotal / 2) / info->vtotal;
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

/* XXX: this code is currently unused comment out until needed
static double
_ecore_drm_output_brightness_get(Ecore_Drm_Backlight *backlight)
{
   const char *brightness = NULL;
   double ret;

   if (!(backlight) || !(backlight->device))
     return 0;

   brightness = eeze_udev_syspath_get_sysattr(backlight->device, "brightness");
   if (!brightness) return 0;

   ret = strtod(brightness, NULL);
   if (ret < 0) ret = 0;

   return ret;
}

static double
_ecore_drm_output_actual_brightness_get(Ecore_Drm_Backlight *backlight)
{
   const char *brightness = NULL;
   double ret;

   if (!(backlight) || !(backlight->device))
     return 0;

   brightness = eeze_udev_syspath_get_sysattr(backlight->device, "actual_brightness");
   if (!brightness) return 0;

   ret = strtod(brightness, NULL);
   if (ret < 0) ret = 0;

   return ret;
}

static double
_ecore_drm_output_max_brightness_get(Ecore_Drm_Backlight *backlight)
{
   const char *brightness = NULL;
   double ret;

   if (!(backlight) || !(backlight->device))
     return 0;

   brightness = eeze_udev_syspath_get_sysattr(backlight->device, "max_brightness");
   if (!brightness) return 0;

   ret = strtod(brightness, NULL);
   if (ret < 0) ret = 0;

   return ret;
}

static double
_ecore_drm_output_brightness_set(Ecore_Drm_Backlight *backlight, double brightness_val)
{
   Eina_Bool ret = EINA_FALSE;

   if (!(backlight) || !(backlight->device))
     return ret;

   ret = eeze_udev_syspath_set_sysattr(backlight->device, "brightness", brightness_val);

   return ret;
}
*/

/**
 * @internal
 * @brief Initializes backlight control for an output.
 *
 * Searches for backlight devices (either "backlight" or "leds" subsystem)
 * associated with the DRM device path. It prioritizes "raw", "platform",
 * or "firmware" types, especially for LVDS or eDP connectors.
 *
 * @param output The Ecore_Drm_Output for which to initialize backlight.
 * @param conn_type The connector type (e.g., DRM_MODE_CONNECTOR_LVDS).
 * @return A pointer to an allocated Ecore_Drm_Backlight structure if a
 *         suitable backlight device is found, otherwise NULL. The caller is
 *         responsible for freeing this structure using
 *         _ecore_drm_output_backlight_shutdown().
 */
static Ecore_Drm_Backlight *
_ecore_drm_output_backlight_init(Ecore_Drm_Output *output, uint32_t conn_type)
{
   Ecore_Drm_Backlight *backlight = NULL;
   Ecore_Drm_Backlight_Type type = 0;
   Eina_List *devs, *l;
   Eina_Bool found = EINA_FALSE;
   const char *device, *devtype;

   if (!(devs = eeze_udev_find_by_filter("backlight", NULL, output->dev->drm.path)))
     devs = eeze_udev_find_by_filter("leds", NULL, output->dev->drm.path);

   if (!devs) return NULL;

   EINA_LIST_FOREACH(devs, l, device)
     {
        if (!(devtype = eeze_udev_syspath_get_sysattr(device, "type")))
          continue;

        if (!strcmp(devtype, "raw"))
          type = ECORE_DRM_BACKLIGHT_RAW;
        else if (!strcmp(devtype, "platform"))
          type = ECORE_DRM_BACKLIGHT_PLATFORM;
        else if (!strcmp(devtype, "firmware"))
          type = ECORE_DRM_BACKLIGHT_FIRMWARE;

        if ((conn_type == DRM_MODE_CONNECTOR_LVDS) ||
            (conn_type == DRM_MODE_CONNECTOR_eDP) ||
            (type == ECORE_DRM_BACKLIGHT_RAW))
          found = EINA_TRUE;

        eina_stringshare_del(devtype);
        if (found) break;
     }

   if (found)
     {
        if ((backlight = calloc(1, sizeof(Ecore_Drm_Backlight))))
          {
             backlight->type = type;
             backlight->device = eina_stringshare_add(device);
          }
     }

   EINA_LIST_FREE(devs, device)
     eina_stringshare_del(device);

   return backlight;
}

/**
 * @internal
 * @brief Shuts down backlight control and frees associated resources.
 *
 * @param backlight The Ecore_Drm_Backlight structure to free.
 */
static void
_ecore_drm_output_backlight_shutdown(Ecore_Drm_Backlight *backlight)
{
   if (!backlight) return;

   if (backlight->device)
     eina_stringshare_del(backlight->device);

   free(backlight);
}

/**
 * @internal
 * @brief Converts a DRM subpixel order value to an Ecore/Wayland equivalent.
 *
 * @param subpixel The DRM_MODE_SUBPIXEL_* value.
 * @return An integer representing the subpixel order (intended to map to
 *         Wayland's wl_output_subpixel enum, though comments indicate
 *         direct mapping, e.g., 0 for UNKNOWN, 1 for NONE, etc.).
 */
static int
_ecore_drm_output_subpixel_get(int subpixel)
{
   switch (subpixel)
     {
      case DRM_MODE_SUBPIXEL_UNKNOWN:
        return 0; // WL_OUTPUT_SUBPIXEL_UNKNOWN;
      case DRM_MODE_SUBPIXEL_NONE:
        return 1; //WL_OUTPUT_SUBPIXEL_NONE;
      case DRM_MODE_SUBPIXEL_HORIZONTAL_RGB:
        return 2; //WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB;
      case DRM_MODE_SUBPIXEL_HORIZONTAL_BGR:
        return 3; // WL_OUTPUT_SUBPIXEL_HORIZONTAL_BGR;
      case DRM_MODE_SUBPIXEL_VERTICAL_RGB:
        return 4; // WL_OUTPUT_SUBPIXEL_VERTICAL_RGB;
      case DRM_MODE_SUBPIXEL_VERTICAL_BGR:
        return 5; //WL_OUTPUT_SUBPIXEL_VERTICAL_BGR;
      default:
        return 0; // WL_OUTPUT_SUBPIXEL_UNKNOWN;
     }
}

/**
 * @internal
 * @brief Retrieves and stores information about planes available for an output.
 *
 * Iterates through all planes reported by DRM, filters those usable with
 * the output's CRTC, and extracts properties like plane type and supported
 * rotations.
 *
 * @param output The Ecore_Drm_Output to populate with plane information.
 */
static void
_ecore_drm_output_planes_get(Ecore_Drm_Output *output)
{
   Ecore_Drm_Device *dev;
   Ecore_Drm_Plane *eplane;
   drmModePlaneRes *pres;
   unsigned int i = 0, j = 0;
   int k = 0;

   dev = output->dev;
   pres = drmModeGetPlaneResources(dev->drm.fd);
   if (!pres) return;

   for (; i < pres->count_planes; i++)
     {
        drmModePlane *plane;
        drmModeObjectPropertiesPtr props;
        int type = -1;

        plane = drmModeGetPlane(dev->drm.fd, pres->planes[i]);
        if (!plane) continue;

        if (!(plane->possible_crtcs & (1 << output->crtc_index)))
          goto free_plane;

        props =
          drmModeObjectGetProperties(dev->drm.fd, plane->plane_id,
                                     DRM_MODE_OBJECT_PLANE);
        if (!props) goto free_plane;

        eplane = calloc(1, sizeof(Ecore_Drm_Plane));
        if (!eplane) goto free_plane;

        eplane->id = plane->plane_id;

        for (j = 0; type == -1 && j < props->count_props; j++)
          {
             drmModePropertyPtr prop;

             prop = drmModeGetProperty(dev->drm.fd, props->props[j]);
             if (!prop) continue;

             if (!strcmp(prop->name, "type"))
               {
                  eplane->type = props->prop_values[j];
                  if (eplane->type == ECORE_DRM_PLANE_TYPE_PRIMARY)
                    output->primary_plane_id = eplane->id;
               }
             else if (!strcmp(prop->name, "rotation"))
               {
                  output->rotation_prop_id = props->props[j];
                  eplane->rotation = props->prop_values[j];

                  for (k = 0; k < prop->count_enums; k++)
                    {
                       int r = -1;

                       if (!strcmp(prop->enums[k].name, "rotate-0"))
                         r = ECORE_DRM_PLANE_ROTATION_NORMAL;
                       else if (!strcmp(prop->enums[k].name, "rotate-90"))
                         r = ECORE_DRM_PLANE_ROTATION_90;
                       else if (!strcmp(prop->enums[k].name, "rotate-180"))
                         r = ECORE_DRM_PLANE_ROTATION_180;
                       else if (!strcmp(prop->enums[k].name, "rotate-270"))
                         r = ECORE_DRM_PLANE_ROTATION_270;
                       else if (!strcmp(prop->enums[k].name, "reflect-x"))
                         r = ECORE_DRM_PLANE_ROTATION_REFLECT_X;
                       else if (!strcmp(prop->enums[k].name, "reflect-y"))
                         r = ECORE_DRM_PLANE_ROTATION_REFLECT_Y;

                       if (r != -1)
                         {
                            eplane->supported_rotations |= r;
                            eplane->rotation_map[ffs(r)] =
                              1 << prop->enums[k].value;
                         }
                    }
               }

             drmModeFreeProperty(prop);
          }

        drmModeFreeObjectProperties(props);

        output->planes = eina_list_append(output->planes, eplane);

free_plane:
        drmModeFreePlane(plane);
     }
}

/**
 * @internal
 * @brief Creates and initializes an Ecore_Drm_Output structure.
 *
 * This function is responsible for allocating an Ecore_Drm_Output, finding a
 * suitable CRTC for it, populating its properties (name, make, model, modes,
 * physical size, subpixel order, etc.), initializing backlight if applicable,
 * and parsing EDID information.
 *
 * @param dev The Ecore_Drm_Device this output belongs to.
 * @param res DRM resources.
 * @param conn The DRM connector for this output.
 * @param x The initial x-coordinate for this output on the desktop.
 * @param y The initial y-coordinate for this output on the desktop.
 * @param cloned EINA_TRUE if this output is a clone of another, EINA_FALSE otherwise.
 * @return A pointer to the newly created Ecore_Drm_Output, or NULL on failure.
 */
static Ecore_Drm_Output *
_ecore_drm_output_create(Ecore_Drm_Device *dev, drmModeRes *res, drmModeConnector *conn, int x, int y, Eina_Bool cloned)
{
   Ecore_Drm_Output *output;
   int i = -1;
   char name[DRM_CONNECTOR_NAME_LEN];
   const char *type;
   drmModeCrtc *crtc;
   drmModeEncoder *enc;
   drmModeModeInfo crtc_mode;
   Ecore_Drm_Output_Mode *mode, *current = NULL, *preferred = NULL, *best = NULL;
   Eina_List *l;

   /* try to find a crtc for this connector */
   i = _ecore_drm_output_crtc_find(dev, res, conn);
   if (i < 0) return NULL;

   /* try to allocate space for new output */
   if (!(output = calloc(1, sizeof(Ecore_Drm_Output)))) return NULL;

   output->x = x;
   output->y = y;
   output->dev = dev;
   output->cloned = cloned;
   output->phys_width = conn->mmWidth;
   output->phys_height = conn->mmHeight;
   output->subpixel = _ecore_drm_output_subpixel_get(conn->subpixel);

   output->make = eina_stringshare_add("UNKNOWN");
   output->model = eina_stringshare_add("UNKNOWN");
   output->name = eina_stringshare_add("UNKNOWN");

   output->connected = (conn->connection == DRM_MODE_CONNECTED);
   output->enabled = output->connected;
   output->conn_type = conn->connector_type;
   if (conn->connector_type < ALEN(conn_types))
     type = conn_types[conn->connector_type];
   else
     type = "UNKNOWN";

   snprintf(name, sizeof(name), "%s-%d", type, conn->connector_type_id);
   eina_stringshare_replace(&output->name, name);

   output->crtc_index = i;
   output->crtc_id = res->crtcs[i];
   output->pipe = i;
   dev->crtc_allocator |= (1 << output->crtc_id);
   output->conn_id = conn->connector_id;
   dev->conn_allocator |= (1 << output->conn_id);

   /* store original crtc so we can restore VT settings */
   output->crtc = drmModeGetCrtc(dev->drm.fd, output->crtc_id);

   /* get if dpms is supported */
   output->dpms = _ecore_drm_output_property_get(dev->drm.fd, conn, "DPMS");

   memset(&crtc_mode, 0, sizeof(crtc_mode));

   /* get the encoder currently driving this connector */
   if ((enc = drmModeGetEncoder(dev->drm.fd, conn->encoder_id)))
     {
        crtc = drmModeGetCrtc(dev->drm.fd, enc->crtc_id);
        drmModeFreeEncoder(enc);
        if (!crtc) goto err;
        if (crtc->mode_valid) crtc_mode = crtc->mode;
        drmModeFreeCrtc(crtc);
     }

   for (i = 0; i < conn->count_modes; i++)
     {
        if (!(mode = _ecore_drm_output_mode_add(output, &conn->modes[i])))
          goto err;
     }

   EINA_LIST_REVERSE_FOREACH(output->modes, l, mode)
     {
        if (!memcmp(&crtc_mode, &mode->info, sizeof(crtc_mode)))
          current = mode;
        if (mode->flags & DRM_MODE_TYPE_PREFERRED)
          preferred = mode;
        best = mode;
     }

   if ((!current) && (crtc_mode.clock != 0))
     {
        if (!(current = _ecore_drm_output_mode_add(output, &crtc_mode)))
          goto err;
     }

   if (current) output->current_mode = current;
   else if (preferred) output->current_mode = preferred;
   else if (best) output->current_mode = best;

   if (!output->current_mode) goto err;

   output->current_mode->flags |= DRM_MODE_TYPE_DEFAULT;

   /* try to init backlight */
   output->backlight =
     _ecore_drm_output_backlight_init(output, conn->connector_type);

   /* parse edid */
   _ecore_drm_output_edid_find(output, conn);

   /* TODO: implement support for LCMS ? */
   output->gamma = output->crtc->gamma_size;

   dev->outputs = eina_list_append(dev->outputs, output);

   /* NB: 'primary' output property is not supported in HW, so we need to
    * implement it via software. As such, the First output which gets
    * listed via libdrm will be assigned 'primary' until user changes
    * it via config */
   if (eina_list_count(dev->outputs) == 1)
     output->primary = EINA_TRUE;

   DBG("Created New Output At %d,%d", output->x, output->y);
   DBG("\tCrtc Pos: %d %d", output->crtc->x, output->crtc->y);
   DBG("\tCrtc: %d", output->crtc_id);
   DBG("\tConn: %d", output->conn_id);
   DBG("\tMake: %s", output->make);
   DBG("\tModel: %s", output->model);
   DBG("\tName: %s", output->name);
   DBG("\tCloned: %d", output->cloned);
   DBG("\tPrimary: %d", output->primary);

   EINA_LIST_FOREACH(output->modes, l, mode)
     {
        DBG("\tAdded Mode: %dx%d@%.1f%s%s%s",
            mode->width, mode->height, (mode->refresh / 1000.0),
            (mode->flags & DRM_MODE_TYPE_PREFERRED) ? ", preferred" : "",
            (mode->flags & DRM_MODE_TYPE_DEFAULT) ? ", current" : "",
            (conn->count_modes == 0) ? ", built-in" : "");
     }

   _ecore_drm_output_planes_get(output);

   return output;

err:
   EINA_LIST_FREE(output->modes, mode)
     free(mode);
   drmModeFreeProperty(output->dpms);
   drmModeFreeCrtc(output->crtc);
   dev->crtc_allocator &= ~(1 << output->crtc_id);
   dev->conn_allocator &= ~(1 << output->conn_id);
   eina_stringshare_del(output->name);
   eina_stringshare_del(output->model);
   eina_stringshare_del(output->make);
   free(output);
   return NULL;
}

/**
 * @internal
 * @brief Frees an Ecore_Drm_Output structure and its associated resources.
 *
 * This includes shutting down backlight, turning off the hardware cursor,
 * attempting to restore the original CRTC state, and freeing mode lists,
 * stringshares, and DRM properties. If a page flip is pending, destruction
 * is deferred.
 *
 * @param output The Ecore_Drm_Output to free.
 */
static void
_ecore_drm_output_free(Ecore_Drm_Output *output)
{
   Ecore_Drm_Output_Mode *mode;
   Ecore_Drm_Plane *plane;

   /* check for valid output */
   if (!output) return;

   if (output->pending_flip)
     {
        output->pending_destroy = EINA_TRUE;
        return;
     }

   /* delete the backlight struct */
   if (output->backlight)
     _ecore_drm_output_backlight_shutdown(output->backlight);

   /* turn off hardware cursor */
   drmModeSetCursor(output->dev->drm.fd, output->crtc_id, 0, 0, 0);

   /* restore crtc state */
   if (output->crtc)
     {
        if (drmModeSetCrtc(output->dev->drm.fd, output->crtc->crtc_id,
                           output->crtc->buffer_id, output->crtc->x, output->crtc->y,
                           &output->conn_id, 1, &output->crtc->mode))
          {
             ERR("Failed to restore Crtc state for output %s: %m", output->name);
          }
     }

   /* free modes */
   EINA_LIST_FREE(output->modes, mode)
     free(mode);

   EINA_LIST_FREE(output->planes, plane)
     free(plane);

   /* free strings */
   if (output->name) eina_stringshare_del(output->name);
   if (output->model) eina_stringshare_del(output->model);
   if (output->make) eina_stringshare_del(output->make);

   if (output->dpms) drmModeFreeProperty(output->dpms);
   if (output->crtc) drmModeFreeCrtc(output->crtc);

   free(output);
}

void
_ecore_drm_output_frame_finish(Ecore_Drm_Output *output)
{
   if (!output) return;

   if (output->need_repaint) ecore_drm_output_repaint(output);

   output->repaint_scheduled = EINA_FALSE;
}

void
_ecore_drm_output_fb_release(Ecore_Drm_Output *output, Ecore_Drm_Fb *fb)
{
   if ((!output) || (!fb)) return;

   if ((fb->mmap) &&
       (fb != output->dev->dumb[0]) && (fb != output->dev->dumb[1]))
     ecore_drm_fb_destroy(fb);
}

void
_ecore_drm_output_repaint_start(Ecore_Drm_Output *output)
{
   unsigned int fb;

   /* DBG("Output Repaint Start"); */

   if (!output) return;
   if (output->pending_destroy) return;

   if (!output->dev->current)
     {
        /* DBG("\tNo Current FB"); */
        goto finish;
     }

   fb = output->dev->current->id;
   if (drmModePageFlip(output->dev->drm.fd, output->crtc_id, fb,
                       DRM_MODE_PAGE_FLIP_EVENT, output) < 0)
     {
        ERR("Could not schedule output page flip event");
        goto finish;
     }

   return;

finish:
   _ecore_drm_output_frame_finish(output);
}

void
_ecore_drm_outputs_update(Ecore_Drm_Device *dev)
{
   drmModeRes *res;
   drmModeConnector *conn;
   int i = 0, x = 0, y = 0;
   Ecore_Drm_Output *output;
   uint32_t connected = 0, disconnects = 0;

   /* try to get drm resources */
   if (!(res = drmModeGetResources(dev->drm.fd))) return;

   /* find any new connects */
   for (; i < res->count_connectors; i++)
     {
        int conn_id;

        conn_id = res->connectors[i];

        /* try to get the connector */
        if (!(conn = drmModeGetConnector(dev->drm.fd, conn_id)))
          continue;

        /* test if connected */
        if (conn->connection != DRM_MODE_CONNECTED) goto next;

        connected |= (1 << conn_id);

        if (!(dev->conn_allocator & (1 << conn_id)))
          {
             if (dev->outputs)
               {
                  Ecore_Drm_Output *last;

                  if ((last = eina_list_last_data_get(dev->outputs)))
                    x = last->x + last->current_mode->width;
                  else
                    x = 0;
               }
             else
               x = 0;

             /* try to create a new output */
             /* NB: hotplugged outputs will be set to cloned by default */
             if (!(output =
                   _ecore_drm_output_create(dev, res, conn, x, y, EINA_TRUE)))
               goto next;
          }
next:
        drmModeFreeConnector(conn);
     }

   drmModeFreeResources(res);

   /* find any disconnects */
   disconnects = (dev->conn_allocator & ~connected);
   if (disconnects)
     {
        Eina_List *l;

        EINA_LIST_FOREACH(dev->outputs, l, output)
          {
             if (disconnects & (1 << output->conn_id))
               {
                  disconnects &= ~(1 << output->conn_id);
                  _ecore_drm_output_event_send(output, EINA_FALSE);
               }
          }
     }
}

void
_ecore_drm_output_render_enable(Ecore_Drm_Output *output)
{
   Ecore_Drm_Device *dev;
   Ecore_Drm_Output_Mode *mode;
   /* int x = 0, y = 0; */

   EINA_SAFETY_ON_NULL_RETURN(output);
   EINA_SAFETY_ON_NULL_RETURN(output->dev);
   EINA_SAFETY_ON_NULL_RETURN(output->current_mode);

   if (!output->enabled) return;

   dev = output->dev;

   if (!dev->current)
     {
        /* schedule repaint */
        /* NB: this will trigger a redraw at next idle */
        output->need_repaint = EINA_TRUE;
        return;
     }

   ecore_drm_output_dpms_set(output, DRM_MODE_DPMS_ON);

   /* if (!output->cloned) */
   /*   { */
   /*      x = output->x; */
   /*      y = output->y; */
   /*   } */

   mode = output->current_mode;
   if (drmModeSetCrtc(dev->drm.fd, output->crtc_id, dev->current->id,
                      output->x, output->y,
                      &output->conn_id, 1, &mode->info) < 0)
     {
        ERR("Failed to set Mode %dx%d for Output %s: %m",
            mode->width, mode->height, output->name);
     }
}

void
_ecore_drm_output_render_disable(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN(output);

   output->need_repaint = EINA_FALSE;
   if (!output->enabled) return;
   ecore_drm_output_cursor_size_set(output, 0, 0, 0);
   ecore_drm_output_dpms_set(output, DRM_MODE_DPMS_OFF);
}

/* public functions */

/**
 * @defgroup Ecore_Drm_Output_Group Ecore DRM Output
 *
 * Functions to manage DRM outputs.
 *
 */

/**
 * @brief Creates and initializes all outputs for a DRM device.
 * @ingroup Ecore_Drm_Output_Group
 *
 * Iterates through all connectors reported by the DRM device, and for each
 * connected one, it attempts to create an Ecore_Drm_Output.
 * Outputs are laid out horizontally by default.
 *
 * @param dev The Ecore_Drm_Device to create outputs for.
 * @return EINA_TRUE on success (at least one output created), EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_drm_outputs_create(Ecore_Drm_Device *dev)
{
   Eina_Bool ret = EINA_TRUE;
   Ecore_Drm_Output *output = NULL;
   drmModeConnector *conn;
   drmModeRes *res;
   int i = 0, x = 0, y = 0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(dev->drm.fd < 0, EINA_FALSE);

   /* DBG("Create outputs for %d", dev->drm.fd); */

   /* get the resources */
   if (!(res = drmModeGetResources(dev->drm.fd)))
     {
        ERR("Could not get resources for drm card");
        return EINA_FALSE;
     }

   if (!(dev->crtcs = calloc(res->count_crtcs, sizeof(unsigned int))))
     {
        ERR("Could not allocate space for crtcs");
        /* free resources */
        drmModeFreeResources(res);
        return EINA_FALSE;
     }

   dev->crtc_count = res->count_crtcs;
   memcpy(dev->crtcs, res->crtcs, sizeof(unsigned int) * res->count_crtcs);

   dev->min_width = res->min_width;
   dev->min_height = res->min_height;
   dev->max_width = res->max_width;
   dev->max_height = res->max_height;

   /* DBG("Dev Size"); */
   /* DBG("\tMin Width: %u", res->min_width); */
   /* DBG("\tMin Height: %u", res->min_height); */
   /* DBG("\tMax Width: %u", res->max_width); */
   /* DBG("\tMax Height: %u", res->max_height); */

   for (i = 0; i < res->count_connectors; i++)
     {
        /* get the connector */
        if (!(conn = drmModeGetConnector(dev->drm.fd, res->connectors[i])))
          continue;

        /* if (conn->connection != DRM_MODE_CONNECTED) goto next; */

        /* create output for this connector */
        if (!(output =
              _ecore_drm_output_create(dev, res, conn, x, y, EINA_FALSE)))
          goto next;

        x += output->current_mode->width;

next:
        /* free the connector */
        drmModeFreeConnector(conn);
     }

   ret = EINA_TRUE;
   if (!dev->outputs)
     ret = EINA_FALSE;

   /* free resources */
   drmModeFreeResources(res);

   return ret;
}

/**
 * @brief Frees an Ecore_Drm_Output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * This is a public wrapper around _ecore_drm_output_free().
 *
 * @param output The Ecore_Drm_Output to free.
 */
EAPI void
ecore_drm_output_free(Ecore_Drm_Output *output)
{
   _ecore_drm_output_free(output);
}

/**
 * @brief Sets the hardware cursor for an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @param handle The buffer handle for the cursor image (0 to hide).
 * @param w The width of the cursor.
 * @param h The height of the cursor.
 */
EAPI void
ecore_drm_output_cursor_size_set(Ecore_Drm_Output *output, int handle, int w, int h)
{
   EINA_SAFETY_ON_NULL_RETURN(output);
   if (!output->enabled) return;
   drmModeSetCursor(output->dev->drm.fd, output->crtc_id, handle, w, h);
}

/**
 * @brief Enables an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * Marks the output as enabled, sets DPMS to ON, and sends a plug event.
 *
 * @param output The Ecore_Drm_Output to enable.
 * @return EINA_TRUE on success.
 */
EAPI Eina_Bool
ecore_drm_output_enable(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);

   output->enabled = EINA_TRUE;
   ecore_drm_output_dpms_set(output, DRM_MODE_DPMS_ON);

   _ecore_drm_output_event_send(output, EINA_TRUE);

   return EINA_TRUE;
}

/**
 * @brief Disables an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * Marks the output as disabled, sets DPMS to OFF, and sends an unplug event.
 *
 * @param output The Ecore_Drm_Output to disable.
 */
EAPI void
ecore_drm_output_disable(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN(output);

   output->enabled = EINA_FALSE;
   ecore_drm_output_dpms_set(output, DRM_MODE_DPMS_OFF);

   _ecore_drm_output_event_send(output, EINA_FALSE);
}

/**
 * @brief Releases a framebuffer associated with an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * If the framebuffer is mmapped and not one of the device's dumb buffers,
 * it is destroyed.
 *
 * @param output The Ecore_Drm_Output.
 * @param fb The Ecore_Drm_Fb to release.
 */
EAPI void
ecore_drm_output_fb_release(Ecore_Drm_Output *output, Ecore_Drm_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN(output);
   EINA_SAFETY_ON_NULL_RETURN(fb);
   _ecore_drm_output_fb_release(output, fb);
}

/**
 * @brief Repaints an output, scheduling a page flip.
 * @ingroup Ecore_Drm_Output_Group
 *
 * This function handles the core repaint logic. If there's no "next"
 * framebuffer prepared, it might fall back to software rendering (currently a stub).
 * Otherwise, it sets the CRTC if the current buffer is different or uninitialized,
 * then schedules a page flip to the "next" framebuffer. It also handles
 * sprite updates and vblank events.
 *
 * @param output The Ecore_Drm_Output to repaint.
 */
EAPI void
ecore_drm_output_repaint(Ecore_Drm_Output *output)
{
   Ecore_Drm_Device *dev;
   Ecore_Drm_Sprite *sprite;
   Eina_List *l;
   int ret = 0;

   EINA_SAFETY_ON_NULL_RETURN(output);
   EINA_SAFETY_ON_NULL_RETURN(output->dev);
   EINA_SAFETY_ON_TRUE_RETURN(output->pending_destroy);

   if (!output->enabled) return;

   dev = output->dev;

   /* DBG("Output Repaint: %d %d", output->crtc_id, output->conn_id); */

   /* TODO: assign planes ? */

   if (!dev->next)
     _ecore_drm_output_software_render(output);
   if (!dev->next) return;

   output->need_repaint = EINA_FALSE;

   if ((!dev->current) ||
       (dev->current->stride != dev->next->stride))
     {
        Ecore_Drm_Output_Mode *mode;

        mode = output->current_mode;
        ret = drmModeSetCrtc(dev->drm.fd, output->crtc_id, dev->next->id,
                             0, 0, &output->conn_id, 1, &mode->info);
        if (ret) goto err;

        ecore_drm_output_dpms_set(output, DRM_MODE_DPMS_ON);
     }

   if (drmModePageFlip(dev->drm.fd, output->crtc_id, dev->next->id,
                       DRM_MODE_PAGE_FLIP_EVENT, output) < 0)
     {
        ERR("Could not schedule pageflip: %m");
        DBG("\tCrtc: %d\tConn: %d\tFB: %d",
            output->crtc_id, output->conn_id, dev->next->id);
        goto err;
     }

   output->pending_flip = EINA_TRUE;

   /* TODO: output_cursor_set */

   EINA_LIST_FOREACH(dev->sprites, l, sprite)
     {
        unsigned int flags = 0, id = 0;
        drmVBlank vbl =
          {
             .request.type = (DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT),
             .request.sequence = 1,
          };

        if (((!sprite->current_fb) && (!sprite->next_fb)) ||
            (!ecore_drm_sprites_crtc_supported(output, sprite->crtcs)))
          continue;

        if ((sprite->next_fb) && (!dev->cursors_broken))
          id = sprite->next_fb->id;

        ecore_drm_sprites_fb_set(sprite, id, flags);

        vbl.request.signal = (unsigned long)sprite;
        ret = drmWaitVBlank(dev->drm.fd, &vbl);
        if (ret) ERR("Error Wait VBlank");

        sprite->output = output;
        output->pending_vblank = EINA_TRUE;
     }

   return;

err:
   if (dev->next)
     {
        _ecore_drm_output_fb_release(output, dev->next);
        dev->next = NULL;
     }
}

/**
 * @brief Gets the size of a framebuffer by its ID.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param dev The Ecore_Drm_Device.
 * @param output The ID of the framebuffer.
 * @param[out] w Pointer to store the width.
 * @param[out] h Pointer to store the height.
 */
EAPI void
ecore_drm_output_size_get(Ecore_Drm_Device *dev, int output, int *w, int *h)
{
   drmModeFB *fb;

   if (w) *w = 0;
   if (h) *h = 0;
   EINA_SAFETY_ON_NULL_RETURN(dev);

   if (!(fb = drmModeGetFB(dev->drm.fd, output))) return;
   if (w) *w = fb->width;
   if (h) *h = fb->height;
   drmModeFreeFB(fb);
}

/**
 * @brief Gets the total geometry of all connected and enabled non-cloned outputs.
 * @ingroup Ecore_Drm_Output_Group
 *
 * Calculates the bounding box that encompasses all active, non-cloned outputs.
 * Currently, it seems to sum widths and take max height, which might be
 * specific to a horizontal layout assumption.
 *
 * @param dev The Ecore_Drm_Device.
 * @param[out] x Pointer to store the starting x-coordinate (currently always 0).
 * @param[out] y Pointer to store the starting y-coordinate (currently always 0).
 * @param[out] w Pointer to store the total width.
 * @param[out] h Pointer to store the maximum height.
 */
EAPI void
ecore_drm_outputs_geometry_get(Ecore_Drm_Device *dev, int *x, int *y, int *w, int *h)
{
   Ecore_Drm_Output *output;
   Eina_List *l;
   int ox = 0, oy = 0, ow = 0, oh = 0;

   if (x) *x = 0;
   if (y) *y = 0;
   if (w) *w = 0;
   if (h) *h = 0;
   EINA_SAFETY_ON_NULL_RETURN(dev);

   EINA_LIST_FOREACH(dev->outputs, l, output)
     {
        if ((!output->connected) || (!output->enabled)) continue;
        if (output->cloned) continue;
        ow += MAX(ow, output->current_mode->width);
        oh = MAX(oh, output->current_mode->height);
     }

   if (x) *x = ox;
   if (y) *y = oy;
   if (w) *w = ow;
   if (h) *h = oh;
}

/**
 * @brief Gets the position of an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @param[out] x Pointer to store the x-coordinate.
 * @param[out] y Pointer to store the y-coordinate.
 */
EAPI void
ecore_drm_output_position_get(Ecore_Drm_Output *output, int *x, int *y)
{
   EINA_SAFETY_ON_NULL_RETURN(output);

   if (x) *x = output->x;
   if (y) *y = output->y;
}

/**
 * @brief Gets the current resolution and refresh rate of an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @param[out] w Pointer to store the width.
 * @param[out] h Pointer to store the height.
 * @param[out] refresh Pointer to store the refresh rate in mHz (e.g., 60000 for 60Hz).
 */
EAPI void
ecore_drm_output_current_resolution_get(Ecore_Drm_Output *output, int *w, int *h, unsigned int *refresh)
{
   if (w) *w = 0;
   if (h) *h = 0;
   if (refresh) *refresh = 0;

   EINA_SAFETY_ON_NULL_RETURN(output);

   if (!output->current_mode) return;

   if (w) *w = output->current_mode->width;
   if (h) *h = output->current_mode->height;
   if (refresh) *refresh = output->current_mode->refresh;
}

/**
 * @brief Gets the physical size of an output in millimeters.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @param[out] w Pointer to store the physical width in mm.
 * @param[out] h Pointer to store the physical height in mm.
 */
EAPI void
ecore_drm_output_physical_size_get(Ecore_Drm_Output *output, int *w, int *h)
{
   EINA_SAFETY_ON_NULL_RETURN(output);

   if (w) *w = output->phys_width;
   if (h) *h = output->phys_height;
}

/**
 * @brief Gets the subpixel order of an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @return The subpixel order (maps to Wayland's wl_output_subpixel enum values).
 *         Example: 2 for WL_OUTPUT_SUBPIXEL_HORIZONTAL_RGB.
 */
EAPI unsigned int
ecore_drm_output_subpixel_order_get(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, 0);

   return output->subpixel;
}

/**
 * @brief Gets the model name of an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * This is typically derived from EDID information.
 *
 * @param output The Ecore_Drm_Output.
 * @return A stringshared pointer to the model name. The caller should not free this.
 *         Returns NULL if output is NULL.
 *         Example: "DELL U2412M"
 */
EAPI Eina_Stringshare *
ecore_drm_output_model_get(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);

   return output->model;
}

/**
 * @brief Gets the make (manufacturer) of an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * This is typically derived from EDID (PNP ID).
 *
 * @param output The Ecore_Drm_Output.
 * @return A stringshared pointer to the make. The caller should not free this.
 *         Returns NULL if output is NULL.
 *         Example: "DEL"
 */
EAPI Eina_Stringshare *
ecore_drm_output_make_get(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);

   return output->make;
}

/**
 * @brief Sets the DPMS (Display Power Management Signaling) level for an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @param level The DPMS level to set (e.g., DRM_MODE_DPMS_ON, DRM_MODE_DPMS_OFF).
 */
EAPI void
ecore_drm_output_dpms_set(Ecore_Drm_Output *output, int level)
{
   EINA_SAFETY_ON_NULL_RETURN(output);
   EINA_SAFETY_ON_NULL_RETURN(output->dev);
   EINA_SAFETY_ON_NULL_RETURN(output->dpms);

   drmModeConnectorSetProperty(output->dev->drm.fd, output->conn_id,
                               output->dpms->prop_id, level);
}

/**
 * @brief Sets the gamma ramps for an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @param size The size of each gamma ramp array. This must match the
 *             output's gamma_size property.
 * @param r Array of red gamma values.
 * @param g Array of green gamma values.
 * @param b Array of blue gamma values.
 *          Each array should contain 'size' elements.
 *          Example for r, g, b arrays (conceptual, values depend on desired correction):
 *          uint16_t r[256], g[256], b[256];
 *          for (int i=0; i<256; ++i) { r[i] = g[i] = b[i] = (i << 8) | i; } // Linear ramp
 *          ecore_drm_output_gamma_set(output, 256, r, g, b);
 */
EAPI void
ecore_drm_output_gamma_set(Ecore_Drm_Output *output, uint16_t size, uint16_t *r, uint16_t *g, uint16_t *b)
{
   EINA_SAFETY_ON_NULL_RETURN(output);
   EINA_SAFETY_ON_NULL_RETURN(output->dev);
   EINA_SAFETY_ON_NULL_RETURN(output->crtc);

   if (output->gamma != size) return;

   if (drmModeCrtcSetGamma(output->dev->drm.fd, output->crtc_id, size, r, g, b))
     ERR("Failed to set output gamma: %m");
}

/**
 * @brief Gets the CRTC ID associated with an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @return The CRTC ID, or 0 if output is NULL.
 */
EAPI unsigned int
ecore_drm_output_crtc_id_get(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, 0);

   return output->crtc_id;
}

/**
 * @brief Gets the current buffer ID associated with an output's CRTC.
 * @ingroup Ecore_Drm_Output_Group
 *
 * Retrieves the ID of the framebuffer currently being scanned out by the CRTC.
 *
 * @param output The Ecore_Drm_Output.
 * @return The buffer ID, or 0 on failure or if output/dev/crtc is NULL.
 */
EAPI unsigned int
ecore_drm_output_crtc_buffer_get(Ecore_Drm_Output *output)
{
   drmModeCrtc *crtc;
   unsigned int id = 0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->dev, 0);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->crtc, 0);

   if (!(crtc = drmModeGetCrtc(output->dev->drm.fd, output->crtc_id)))
     return 0;

   id = crtc->buffer_id;
   drmModeFreeCrtc(crtc);

   return id;
}

/**
 * @brief Gets the connector ID associated with an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @return The connector ID, or 0 if output is NULL.
 */
EAPI unsigned int
ecore_drm_output_connector_id_get(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, 0);

   return output->conn_id;
}

/**
 * @brief Gets the name of an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * The name is typically formatted as "CONNECTOR_TYPE-ID" (e.g., "HDMI-A-1").
 *
 * @param output The Ecore_Drm_Output.
 * @return A newly allocated string containing the output name. The caller
 *         must free this string. Returns NULL if output is NULL.
 *         Example: "DP-1"
 */
EAPI char *
ecore_drm_output_name_get(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);

   return strdup(output->name);
}

/**
 * @brief Checks if an output is currently connected.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @return EINA_TRUE if connected, EINA_FALSE otherwise or if output is NULL.
 */
EAPI Eina_Bool
ecore_drm_output_connected_get(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);

   return output->connected;
}

/**
 * @brief Gets the connector type of an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @return The connector type (e.g., DRM_MODE_CONNECTOR_DisplayPort,
 *         DRM_MODE_CONNECTOR_HDMIA). Returns 0 if output is NULL.
 */
EAPI unsigned int
ecore_drm_output_connector_type_get(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, 0);

   return output->conn_type;
}

/**
 * @brief Checks if an output has backlight control.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @return EINA_TRUE if backlight control is available, EINA_FALSE otherwise
 *         or if output is NULL.
 */
EAPI Eina_Bool
ecore_drm_output_backlight_get(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   return (output->backlight != NULL);
}

/**
 * @brief Gets the raw EDID data of an output as a hex string.
 * @ingroup Ecore_Drm_Output_Group
 *
 * Retrieves the stored EDID blob (first 128 bytes) and converts it
 * into a hexadecimal string representation.
 *
 * @param output The Ecore_Drm_Output.
 * @return A newly allocated string containing the hex representation of EDID.
 *         The caller must free this string. Returns NULL if output, its
 *         edid_blob is NULL, or on allocation failure.
 *         Example: "00ffffffffffff00..." (256 characters)
 */
EAPI char *
ecore_drm_output_edid_get(Ecore_Drm_Output *output)
{
   char *edid_str = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->edid_blob, NULL);

   edid_str = malloc((128 * 2) + 1);
   if (edid_str)
     {
        unsigned int k, kk;
        const char *hexch = "0123456789abcdef";

        for (kk = 0, k = 0; k < 128; k++)
          {
             edid_str[kk] = hexch[(output->edid_blob[k] >> 4) & 0xf];
             edid_str[kk + 1] = hexch[output->edid_blob[k] & 0xf];
             kk += 2;
          }
        edid_str[kk] = 0;
     }

   return edid_str;
}

/**
 * @brief Gets the list of available modes for an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * Each element in the list is an Ecore_Drm_Output_Mode.
 *
 * @param output The Ecore_Drm_Output.
 * @return A pointer to the Eina_List of modes. The caller should not modify
 *         or free this list or its contents. Returns NULL if output or its
 *         modes list is NULL.
 *         Example list structure:
 *         [
 *           Ecore_Drm_Output_Mode { width=1920, height=1080, refresh=60000, ... },
 *           Ecore_Drm_Output_Mode { width=1280, height=720, refresh=60000, ... }
 *         ]
 */
EAPI Eina_List *
ecore_drm_output_modes_get(Ecore_Drm_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->modes, NULL);

   return output->modes;
}

/**
 * @brief Gets the primary output for a DRM device.
 * @ingroup Ecore_Drm_Output_Group
 *
 * Iterates through the device's outputs and returns the one marked as primary.
 *
 * @param dev The Ecore_Drm_Device.
 * @return The primary Ecore_Drm_Output, or NULL if none is set or dev is NULL.
 */
EAPI Ecore_Drm_Output *
ecore_drm_output_primary_get(Ecore_Drm_Device *dev)
{
   Ecore_Drm_Output *ret;
   const Eina_List *l;

   EINA_SAFETY_ON_NULL_RETURN_VAL(dev, NULL);

   EINA_LIST_FOREACH(dev->outputs, l, ret)
     if (ret->primary) return ret;

   return NULL;
}

/**
 * @brief Sets an output as the primary output for its device.
 * @ingroup Ecore_Drm_Output_Group
 *
 * Unmarks all other outputs on the same device as non-primary.
 *
 * @param output The Ecore_Drm_Output to set as primary.
 */
EAPI void
ecore_drm_output_primary_set(Ecore_Drm_Output *output)
{
   const Eina_List *l;
   Ecore_Drm_Output *out;

   EINA_SAFETY_ON_NULL_RETURN(output);

   /* unmark all outputs as primary */
   EINA_LIST_FOREACH(output->dev->outputs, l, out)
     out->primary = EINA_FALSE;

   /* mark this output as primary */
   output->primary = EINA_TRUE;
}

/**
 * @brief Gets the configured size of the CRTC associated with an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * This reflects the dimensions of the CRTC's current mode, not necessarily
 * the display's native resolution.
 *
 * @param output The Ecore_Drm_Output.
 * @param[out] width Pointer to store the CRTC width.
 * @param[out] height Pointer to store the CRTC height.
 */
EAPI void
ecore_drm_output_crtc_size_get(Ecore_Drm_Output *output, int *width, int *height)
{
   if (width) *width = 0;
   if (height) *height = 0;

   EINA_SAFETY_ON_NULL_RETURN(output);

   if (width) *width = output->crtc->width;
   if (height) *height = output->crtc->height;
}

/**
 * @brief Checks if a given CRTC ID can be used by an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * This function determines if the specified CRTC is among the possible CRTCs
 * for any encoder connected to the output.
 *
 * @param output The Ecore_Drm_Output.
 * @param crtc The CRTC ID to check.
 * @return EINA_TRUE if the CRTC can be used by the output, EINA_FALSE otherwise.
 */
EAPI Eina_Bool
ecore_drm_output_possible_crtc_get(Ecore_Drm_Output *output, unsigned int crtc)
{
   Ecore_Drm_Device *dev;
   drmModeRes *res;
   drmModeConnector *conn;
   drmModeEncoder *enc;
   int i, j, k;
   unsigned int p;
   Eina_Bool ret = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->dev, EINA_FALSE);

   dev = output->dev;
   EINA_SAFETY_ON_TRUE_RETURN_VAL(dev->drm.fd < 0, EINA_FALSE);

   /* get the resources */
   if (!(res = drmModeGetResources(dev->drm.fd)))
     {
        ERR("Could not get resources for drm card");
        return EINA_FALSE;
     }

   for (i = 0; i < res->count_connectors; i++)
     {
        /* get the connector */
        if (!(conn = drmModeGetConnector(dev->drm.fd, res->connectors[i])))
          continue;

        for (j = 0; j < conn->count_encoders; j++)
          {
             /* get the encoder on this connector */
             if (!(enc = drmModeGetEncoder(dev->drm.fd, conn->encoders[j])))
               {
                  WRN("Failed to get encoder");
                  continue;
               }

             /* get the encoder for given crtc */
             if (enc->crtc_id != crtc) goto next;

             p = enc->possible_crtcs;

             for (k = 0; k < res->count_crtcs; k++)
               {
                  if (res->crtcs[k] != output->crtc_id) continue;
                  if (p & (1 << k))
                    {
                       ret = EINA_TRUE;
                       break;
                    }
               }

next:
             drmModeFreeEncoder(enc);
             if (ret) break;
          }

        /* free the connector */
        drmModeFreeConnector(conn);
        if (ret) break;
     }

   /* free resources */
   drmModeFreeResources(res);

   return ret;
}

/**
 * @brief Sets the mode and position for an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * Configures the CRTC associated with the output to use the specified mode
 * and framebuffer position. If `mode` is NULL, it attempts to turn off the output
 * by setting a NULL mode on the CRTC.
 *
 * @param output The Ecore_Drm_Output to configure.
 * @param mode The Ecore_Drm_Output_Mode to set. If NULL, the output is turned off.
 * @param x The x-coordinate for the framebuffer on the CRTC.
 * @param y The y-coordinate for the framebuffer on the CRTC.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool
ecore_drm_output_mode_set(Ecore_Drm_Output *output, Ecore_Drm_Output_Mode *mode, int x, int y)
{
   Ecore_Drm_Device *dev;
   Eina_Bool ret = EINA_TRUE;
   unsigned int buffer = 0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->dev, EINA_FALSE);

   dev = output->dev;

   output->x = x;
   output->y = y;
   output->current_mode = mode;

   if (mode)
     {
        if (dev->current)
          buffer = dev->current->id;
        else if (dev->next)
          buffer = dev->next->id;
        else
          buffer = output->crtc->buffer_id;

        if (drmModeSetCrtc(dev->drm.fd, output->crtc_id, buffer,
                           output->x, output->y,
                           &output->conn_id, 1, &mode->info) < 0)
          {
             ERR("Failed to set Mode %dx%d for Output %s: %m",
                 mode->width, mode->height, output->name);
             ret = EINA_FALSE;
          }
     }
   else
     {
        if (drmModeSetCrtc(dev->drm.fd, output->crtc_id,
                           0, 0, 0, 0, 0, NULL) < 0)
          {
             ERR("Failed to turn off Output %s: %m", output->name);
             ret = EINA_FALSE;
          }
     }

   return ret;
}

/**
 * @brief Gets the supported rotations for a specific plane type on an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @param type The type of plane (e.g., ECORE_DRM_PLANE_TYPE_PRIMARY).
 * @return A bitmask of supported ECORE_DRM_PLANE_ROTATION_* flags, or -1 (all bits set)
 *         if output is NULL or the plane type is not found.
 *         Example: (ECORE_DRM_PLANE_ROTATION_NORMAL | ECORE_DRM_PLANE_ROTATION_90)
 */
EAPI unsigned int
ecore_drm_output_supported_rotations_get(Ecore_Drm_Output *output, Ecore_Drm_Plane_Type type)
{
   Ecore_Drm_Plane *plane;
   Eina_List *l;
   unsigned int rot = -1;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, rot);

   EINA_LIST_FOREACH(output->planes, l, plane)
     {
        if (plane->type != type) continue;
        rot = plane->supported_rotations;
        break;
     }

   return rot;
}

/**
 * @brief Sets the rotation for a specific plane type on an output.
 * @ingroup Ecore_Drm_Output_Group
 *
 * @param output The Ecore_Drm_Output.
 * @param type The type of plane (e.g., ECORE_DRM_PLANE_TYPE_PRIMARY).
 * @param rotation The rotation to set (one of ECORE_DRM_PLANE_ROTATION_* flags).
 *                 Must be a single, supported rotation value.
 * @return EINA_TRUE on success, EINA_FALSE on failure (e.g., unsupported rotation,
 *         plane not found, or DRM error).
 */
EAPI Eina_Bool
ecore_drm_output_rotation_set(Ecore_Drm_Output *output, Ecore_Drm_Plane_Type type, unsigned int rotation)
{
   Ecore_Drm_Plane *plane;
   Eina_List *l;
   Eina_Bool ret = EINA_FALSE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);

   EINA_LIST_FOREACH(output->planes, l, plane)
     {
        if (plane->type != type) continue;
        if ((plane->supported_rotations & rotation) == 0)
          {
             WRN("Unsupported rotation");
             return EINA_FALSE;
          }

        if (drmModeObjectSetProperty(output->dev->drm.fd,
                                     output->primary_plane_id,
                                     DRM_MODE_OBJECT_PLANE,
                                     output->rotation_prop_id,
                                     plane->rotation_map[ffs(rotation)]) < 0)
          {
             WRN("Failed to set Rotation");
             return EINA_FALSE;
          }
        ret = EINA_TRUE;
        break;
     }

   return ret;
}
