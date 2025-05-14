#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <fcntl.h>
#include <unistd.h>

#include <gst/gst.h>

#include <Eina.h>

#include "shmfile.h"
#include "timeout.h"

#define DATA32  unsigned int

//#define GST_DBG

#ifdef GST_DBG
#define D(fmt, args...) fprintf(stderr, fmt, ## args)
#else
#define D(fmt, args...)
#endif

#ifdef WORDS_BIGENDIAN
# define CAPS "video/x-raw,format=ARGB"
#else
# define CAPS "video/x-raw,format=BGRA"
#endif

static GstElement *pipeline = NULL;
static GstElement *sink = NULL;
static gint64      duration = -1;

int   width = 0;
int   height = 0;
void *data = NULL;

/**
 * @brief Initializes the GStreamer pipeline for video decoding.
 *
 * This function sets up a GStreamer pipeline to decode a video file specified
 * by @p filename. It configures the pipeline to output raw video frames in
 * either ARGB or BGRA format, depending on the system's endianness.
 * The pipeline uses `uridecodebin` to handle various URI types and media
 * formats, `typefind` to determine the stream type, `videoconvert` for
 * color space conversion, and `appsink` to make the raw video frames
 * accessible to the application.
 *
 * @param filename The path or URI of the video file to load.
 *                 Example: "/path/to/video.mp4" or "file:///path/to/video.mp4"
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_gst_init(const char *filename)
{
   GstPad              *pad;
   GstCaps             *caps;
   GstStructure        *structure;
   gchar               *descr;
   gchar               *uri;
   GError              *error = NULL;
   GstFormat            format;
   GstStateChangeReturn ret;
//   int                  vidstr = 0;

   if (!filename || !*filename)
     return EINA_FALSE;

   if (!gst_init_check(NULL, NULL, &error))
     return EINA_FALSE;

   if ((*filename == '/') || (*filename == '~'))
     {
        uri = g_filename_to_uri(filename, NULL, NULL);
        if (!uri)
          {
             D("could not create new uri from %s", filename);
             goto unref_pipeline;
          }
     }
   else
     uri = strdup(filename);

   D("Setting file %s\n", uri);

   descr = g_strdup_printf("uridecodebin uri=%s ! typefind ! videoconvert ! "
      " appsink name=sink caps=\"" CAPS "\"", uri);
   pipeline = gst_parse_launch(descr, &error);
   free(uri);

   if (error != NULL)
     {
        D("could not construct pipeline: %s\n", error->message);
        g_error_free (error);
        goto gst_shutdown;
     }
/* needs gst 1.0+
 * also only works on playbin objects!!! this is a uridecodebin!
   g_object_get(G_OBJECT(pipeline),
                "n-video", &vidstr,
                NULL);
   if (vidstr <= 0)
     {
        D("no video stream\n");
        goto gst_shutdown;
     }
*/
   sink = gst_bin_get_by_name (GST_BIN (pipeline), "sink");

   ret = gst_element_set_state (pipeline, GST_STATE_PAUSED);
   switch (ret)
     {
     case GST_STATE_CHANGE_FAILURE:
        D("failed to play the file\n");
        goto unref_pipeline;
     case GST_STATE_CHANGE_NO_PREROLL:
        D("live sources not supported yet\n");
        goto unref_pipeline;
     default:
        break;
     }

   ret = gst_element_get_state((pipeline), NULL, NULL, GST_CLOCK_TIME_NONE);
   if (ret == GST_STATE_CHANGE_FAILURE)
     {
        D("could not complete pause\n");
        goto unref_pipeline;
     }

   format = GST_FORMAT_TIME;
   gst_element_query_duration (pipeline, format, &duration);
   if (duration == -1)
     {
        fprintf(stderr, "duration fetch err\n");
        D("could not retrieve the duration, set it to 1s\n");
        duration = 1 * GST_SECOND;
     }

   pad = gst_element_get_static_pad(sink, "sink");
   if (!pad)
     {
        D("could not retrieve the sink pad\n");
        goto unref_pipeline;
     }

   caps = gst_pad_get_current_caps(pad);
   if (!caps)
     goto unref_pad;

   structure = gst_caps_get_structure(caps, 0);

   if (!gst_structure_get_int(structure, "width", &width))
     goto unref_caps;
   if (!gst_structure_get_int(structure, "height", &height))
     goto unref_caps;

   gst_caps_unref(caps);
   gst_object_unref(pad);

   return EINA_TRUE;

 unref_caps:
   gst_caps_unref(caps);
 unref_pad:
   gst_object_unref(pad);
 unref_pipeline:
   gst_element_set_state (pipeline, GST_STATE_NULL);
   gst_object_unref(pipeline);
 gst_shutdown:
   gst_deinit();

   return EINA_FALSE;
}

/**
 * @brief Shuts down the GStreamer pipeline and deinitializes GStreamer.
 *
 * This function sets the pipeline state to NULL, unreferences the pipeline
 * object, and deinitializes GStreamer to free associated resources.
 */
static void
_gst_shutdown()
{
   gst_element_set_state (pipeline, GST_STATE_NULL);
   gst_object_unref(pipeline);
   gst_deinit();
}

/**
 * @brief Loads a video frame at a specific position into shared memory.
 *
 * This function seeks the GStreamer pipeline to a given position (or the
 * middle of the video if @p pos is negative) and pulls a prerolled sample
 * (video frame). The frame data is then copied into a shared memory segment
 * allocated by `shm_alloc`.
 *
 * @param size_w The desired width for scaling (currently unused).
 * @param size_h The desired height for scaling (currently unused).
 * @param pos The time position in nanoseconds to seek to in the video.
 *            If negative, seeks to the middle of the video.
 *            Example: 1.5 * GST_SECOND (for 1.5 seconds)
 */
static void
_gst_load_image(int size_w EINA_UNUSED, int size_h EINA_UNUSED, double pos)
{
   GstBuffer *buffer;
   GstMapInfo info;
   GstSample *sample;

   D("load image\n");
   if (pos >= 0.0)
     gst_element_seek_simple(pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                             pos);
   else
     gst_element_seek_simple(pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                             duration / 2);
   g_signal_emit_by_name(sink, "pull-preroll", &sample, NULL);

   shm_alloc(width * height * sizeof(DATA32));
   if (!shm_addr) return;
   data = shm_addr;

   buffer = gst_sample_get_buffer (sample);
   gst_buffer_map (buffer, &info, GST_MAP_READ);
   D("load image: %p %d\n", info.data, info.size);

   memcpy(data, info.data, info.size);

   gst_buffer_unmap(buffer, &info);
}

/**
 * @brief Main entry point for the GStreamer video frame extractor.
 *
 * This program takes a video file as input and extracts a video frame,
 * outputting its metadata (width, height, alpha) and pixel data.
 * The pixel data can be written to a shared memory file or directly to stdout.
 *
 * Command-line arguments:
 *   argv[1]: Path to the video file. (Required)
 *   -head: If present, only loads header information (dimensions) and not
 *          the pixel data.
 *   -key <pos_ns>: Specifies the time position in nanoseconds to extract
 *                  the frame from. Example: -key 1500000000 (for 1.5s)
 *   -opt-scale-down-by <factor>: (Not currently used by this loader)
 *   -opt-dpi <dpi>: (Not currently used by this loader)
 *   -opt-size <width> <height>: (Not currently used by this loader)
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return 0 on success, -1 on failure.
 */
int
main(int argc, char **argv)
{
   char *file, *p;
   int i, numonly;
   int size_w = 0, size_h = 0;
   int head_only = 0;
   long long pos = -1.0;

   if (argc < 2) return -1;
   // file is ALWAYS first arg, other options come after
   file = argv[1];
   for (i = 2; i < argc; i++)
     {
        if      (!strcmp(argv[i], "-head"))
           // asked to only load header, not body/data
           head_only = 1;
        else if (!strcmp(argv[i], "-key"))
          {
             i++;
             numonly = 1;
             for (p = argv[i]; *p; p++)
               {
                  if ((*p < '0') || (*p > '9'))
                    {
                       numonly = 0;
                       break;
                    }
               }
             if (numonly) pos = atoll(argv[i]) * 1000000;
             i++;
          }
        else if (!strcmp(argv[i], "-opt-scale-down-by"))
          { // not used by ps loader
             i++;
             // int scale_down = atoi(argv[i]);
          }
        else if (!strcmp(argv[i], "-opt-dpi"))
          {
             i++;
          }
        else if (!strcmp(argv[i], "-opt-size"))
          { // not used by ps loader
             i++;
             size_w = atoi(argv[i]);
             i++;
             size_h = atoi(argv[i]);
          }
     }

   timeout_init(10);

   D("_gst_init_file\n");

   if (!_gst_init(file))
     return -1;
   D("_gst_init done\n");

   if ((pos >= 0) && (pos > duration)) return -1;

   if (!head_only)
     {
        _gst_load_image(size_w, size_h, pos);
     }

   D("size...: %ix%i\n", width, height);
   D("alpha..: 0\n");

   printf("size %i %i\n", width, height);
   printf("alpha 0\n");

   if (!head_only)
     {
#ifdef _WIN32
        if (shm_fd)
#else
        if (shm_fd >= 0)
#endif
          {
             printf("shmfile %s\n", shmfile);
          }
        else
          {
             // could also to "tmpfile %s\n" like shmfile but just
             // a mmaped tmp file on the system
             printf("data\n");
             if (fwrite(data, width * height * sizeof(DATA32), 1, stdout) != 1)
               {
                  shm_free();
                  return -1;
               }
          }
        shm_free();
     }
   else
     printf("done\n");

   _gst_shutdown();
   fflush(stdout);
   return 0;
}
