/**
 * @file
 * @brief A test scheduler for the Exactness testing framework.
 *
 * This program runs a series of tests defined in a list file. It can operate
 * in several modes:
 * - 'init': To generate initial reference screenshots ("golden masters").
 * - 'play': To run tests and compare current output against the golden masters.
 * - 'simulation': To run tests without generating or comparing screenshots,
 *   useful for debugging.
 *
 * It supports parallel execution of tests and generates an HTML report for
 * failures.
 */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include <Ecore_Getopt.h>
#include <Ecore_Evas.h>
#include <Ecore_File.h>

#include "common.h"

#ifdef _WIN32
# include <evil_private.h> /* mkdir */
#endif

#define SCHEDULER_CMD_SIZE 1024

#define ORIG_SUBDIR "orig"
#define CURRENT_SUBDIR "current"

#define BUF_SIZE 1024

#define DBG(...) EINA_LOG_DOM_DBG(_log_domain, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_log_domain, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_domain, __VA_ARGS__)
#define CRI(...) EINA_LOG_DOM_CRIT(_log_domain, __VA_ARGS__)

static int _log_domain = -1; /**< Log domain for Eina logging. */

/**
 * @brief Represents a single test entry in the test list.
 *
 * This structure is part of an Eina_Inlist, forming a queue of tests to be
 * executed.
 */
typedef struct
{
   EINA_INLIST; /**< Eina Inlist macro, makes this struct part of a linked list. */
   char *name; /**< The name of the test, derived from the .exu file name. */
   const char *command; /**< The command to execute for this test. */
   double start_time; /**< Timestamp when the test execution began. */
} List_Entry;

/**
 * @brief Defines the operational modes for the exactness test runner.
 */
typedef enum
{
   RUN_SIMULATION, /**< Run tests to display actions without screenshotting. For debugging. */
   RUN_PLAY,       /**< Run tests and compare screenshots against reference images. */
   RUN_INIT        /**< Run tests to create the initial reference screenshots. */
} Run_Mode;

static unsigned short _running_jobs = 0, /**< Number of currently running test jobs. */
                      _max_jobs = 1; /**< Maximum number of parallel jobs. */
static Eina_List *_base_dirs = NULL; /**< List of directories to search for .exu files. */
static char *_dest_dir; /**< Destination directory for output files and reports. */
static char *_wrap_command = NULL, /**< A command to prefix test executions with (e.g., "valgrind"). */
            *_fonts_dir = NULL; /**< Path to a directory of fonts to be used. */
static int _verbose = 0; /**< Verbosity level. */
static Eina_Bool _scan_objs = EINA_FALSE, /**< If true, scan all objects at every shot (unused). */
                 _disable_screenshots = EINA_FALSE, /**< If true, disable screenshot generation. */
                 _stabilize_shots = EINA_FALSE; /**< If true, wait for frames to stabilize before taking shots. */

static Run_Mode _mode; /**< The current operational mode. */
static List_Entry *_next_test_to_run = NULL; /**< Pointer to the next test to be executed from the queue. */
static unsigned int _tests_executed = 0; /**< Counter for the number of tests executed so far. */

static Eina_List *_errors; /**< A list of tests that failed during execution (e.g., crashed). */
static Eina_List *_compare_errors; /**< A list of image names that failed the visual comparison. */

static Eina_Bool _job_consume();

/**
 * @brief Loads an image from a file into an Exactness_Image structure.
 *
 * This function uses Ecore_Evas to open an image file and then copies its pixel
 * data into a newly allocated Exactness_Image structure. This decouples image
 * data from the Evas canvas, allowing it to be processed independently.
 *
 * @param filename The path to the image file to load.
 * @return A pointer to a new Exactness_Image on success, or NULL on failure.
 *         The caller is responsible for freeing the returned structure using
 *         exactness_image_free().
 */
static Exactness_Image *
_image_load(const char *filename)
{
   int w, h;
   Evas_Load_Error err;
   Ecore_Evas *ee = ecore_evas_new(NULL, 0, 0, 100, 100, NULL);
   Eo *e = ecore_evas_get(ee);

   Eo *img = evas_object_image_add(e);
   evas_object_image_file_set(img, filename, NULL);
   err = evas_object_image_load_error_get(img);
   if (err != EVAS_LOAD_ERROR_NONE)
     {
        CRI("Failed to load image");
        return NULL;
     }

   Exactness_Image *ex_img = malloc(sizeof(*ex_img));
   int len;
   evas_object_image_size_get(img, &w, &h);
   ex_img->w = w;
   ex_img->h = h;
   len = w * h * 4;
   ex_img->pixels = malloc(len);
   memcpy(ex_img->pixels, evas_object_image_data_get(img, EINA_FALSE), len);

   ecore_evas_free(ee);
   return ex_img;
}

/**
 * @brief Saves an Exactness_Image to a file.
 *
 * A temporary Evas canvas is created to handle the image saving process.
 *
 * @param ex_img The image data to save.
 * @param output The path to the output file.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_image_save(Exactness_Image *ex_img, const char *output)
{
   Ecore_Evas *ee;
   Eo *e, *img;
   Eina_Bool ret = EINA_TRUE;
   ee = ecore_evas_new(NULL, 0, 0, 100, 100, NULL);
   e = ecore_evas_get(ee);
   img = evas_object_image_add(e);
   evas_object_image_size_set(img, ex_img->w, ex_img->h);
   evas_object_image_data_set(img, ex_img->pixels);
   if (!evas_object_image_save(img, output, NULL, NULL))
      ret = EINA_FALSE;
   ecore_evas_free(ee);
   return ret;
}

/**
 * @brief Compares two image files and generates a diff image on mismatch.
 *
 * It compares an image from a reference ('orig') directory with a an image
 * from the 'current' test run. If a difference is found, a composite diff
_image
 * is saved and the test name is added to the `_compare_errors` list.
 *
 * @param orig_dir The directory containing the original reference images.
 * @param ent_name The filename of the image to compare (e.g., "test_name_001.png").
 * @return EINA_TRUE if the images are different, EINA_FALSE otherwise.
 */
static Eina_Bool
_file_compare(const char *orig_dir, const char *ent_name)
{
   Eina_Bool result = EINA_FALSE;
   Exactness_Image *img1, *img2, *imgO = NULL;
   char *filename1 = alloca(strlen(orig_dir) + strlen(ent_name) + 20);
   char *filename2 = alloca(strlen(_dest_dir) + strlen(ent_name) + 20);
   sprintf(filename1, "%s/%s", orig_dir, ent_name);
   sprintf(filename2, "%s/%s/%s", _dest_dir, CURRENT_SUBDIR, ent_name);

   img1 = _image_load(filename1);
   img2 = _image_load(filename2);

   if (exactness_image_compare(img1, img2, &imgO))
     {
        char *buf = alloca(strlen(_dest_dir) + strlen(ent_name));
        sprintf(buf, "%s/%s/comp_%s", _dest_dir, CURRENT_SUBDIR, ent_name);
        if (!_image_save(imgO, buf))
          goto cleanup;
        _compare_errors = eina_list_append(_compare_errors, strdup(ent_name));
        result = EINA_TRUE;
     }
cleanup:
   exactness_image_free(img1);
   exactness_image_free(img2);
   exactness_image_free(imgO);
   return result;
}

/**
 * @brief Unpacks images from an .exu test unit file into a directory.
 *
 * Reads an Exactness_Unit file and saves all embedded images as individual PNG
 * files in the specified directory. The output filenames are constructed from
 * the test name and a sequence number (e.g., "mytest_001.png").
 *
 * @param exu_path Path to the .exu file.
 * @param dir The directory where the unpacked PNG images will be saved.
 * @param ent_name The base name for the output image files (usually the test name).
 */
static void
_exu_imgs_unpack(const char *exu_path, const char *dir, const char *ent_name)
{
   Exactness_Unit *unit = exactness_unit_file_read(exu_path);
   Exactness_Image *img;
   Eina_List *itr;
   Ecore_Evas *ee = ecore_evas_new(NULL, 0, 0, 100, 100, NULL);
   Eo *e = ecore_evas_get(ee);
   int n = 1;
   if (!unit) return;
   EINA_LIST_FOREACH(unit->imgs, itr, img)
     {
        Eo *o = evas_object_image_add(e);
        char *filename = alloca(strlen(dir) + strlen(ent_name) + 20);
        snprintf(filename, PATH_MAX, "%s/%s%c%.3d.png",
              dir, ent_name, SHOT_DELIMITER, n++);
        evas_object_image_size_set(o, img->w, img->h);
        evas_object_image_data_set(o, img->pixels);
        if (!evas_object_image_save(o, filename, NULL, NULL))
          {
             printf("Cannot save widget to <%s>\n", filename);
          }
        efl_del(o);
     }
   ecore_evas_free(ee);
}

/**
 * @brief Manages the comparison phase for a single test.
 *
 * For a given test entry, this function finds its .exu file, unpacks the
 * reference and current images, and then compares each pair of screenshots.
 * It prints the final status (SUCCESS or FAIL) for the test.
 *
 * @param ent The test entry to process.
 */
static void
_run_test_compare(const List_Entry *ent)
{
   char *path = alloca(PATH_MAX);
   char *origdir = alloca(strlen(_dest_dir) + 20);
   const char *base_dir;
   Eina_List *itr;
   int n = 1, nb_fails = 0;
   printf("STATUS %s: COMPARE\n", ent->name);
   EINA_LIST_FOREACH(_base_dirs, itr, base_dir)
     {
        sprintf(path, "%s/%s.exu", base_dir, ent->name);
        if (ecore_file_exists(path))
          {
             char *currentdir;
             sprintf(origdir, "%s/%s/%s", _dest_dir, CURRENT_SUBDIR, ORIG_SUBDIR);
             if (!ecore_file_exists(origdir))
               {
                  if (mkdir(origdir, 0744) < 0)
                    {
                       CRI("Failed to create dir %s\n", origdir);
                       return;
                    }
               }
             _exu_imgs_unpack(path, origdir, ent->name);
             sprintf(path, "%s/%s/%s.exu", _dest_dir, CURRENT_SUBDIR, ent->name);
             currentdir = alloca(strlen(_dest_dir) + 20);
             sprintf(currentdir, "%s/%s", _dest_dir, CURRENT_SUBDIR);
             _exu_imgs_unpack(path, currentdir, ent->name);
             goto found;
          }
     }
found:
   do
     {
        sprintf(path, "%s/%s%c%.3d.png", origdir, ent->name, SHOT_DELIMITER, n);
        if (ecore_file_exists(path))
          {
             sprintf(path, "%s%c%.3d.png", ent->name, SHOT_DELIMITER, n);
             if (_file_compare(origdir, path)) nb_fails++;
          }
        else break;
        n++;
     } while (EINA_TRUE);
   if (!nb_fails)
     {
        double runtime = ecore_time_get() - ent->start_time;
        printf("STATUS %s: END - SUCCESS (time: %.2fs)\n", ent->name, runtime);
     }
   else
      printf("STATUS %s: END - FAIL (%d/%d)\n", ent->name, nb_fails, n - 1);
}

#define CONFIG "ELM_SCALE=1 ELM_FINGER_SIZE=10 " /**< Default environment variables for test execution. */

/**
 * @brief Prepares the full command-line string to execute a test.
 *
 * This function constructs the command to run `exactness_play` with all the
 * necessary arguments based on the current mode and configuration (e.g.,
 * verbosity, custom fonts, output directories).
 *
 * @param[in] ent The test entry for which to prepare the command.
 * @param[out] buf The buffer where the generated command string will be stored.
 *                 Must have a size of at least SCHEDULER_CMD_SIZE.
 * @return EINA_TRUE on success, EINA_FALSE if the test's .exu file cannot be found.
 */
static Eina_Bool
_run_command_prepare(List_Entry *ent, char *buf)
{
   char scn_path[PATH_MAX];
   Eina_Strbuf *sbuf;
   const char *base_dir;
   Eina_List *itr;
   EINA_LIST_FOREACH(_base_dirs, itr, base_dir)
     {
        sprintf(scn_path, "%s/%s.exu", base_dir, ent->name);
        if (ecore_file_exists(scn_path)) goto ok;
     }
   CRI("Test %s not found in the provided base directories\n", ent->name);
   return EINA_FALSE;
ok:
   sbuf = eina_strbuf_new();
   printf("STATUS %s: START\n", ent->name);
   ent->start_time = ecore_time_get();
   eina_strbuf_append_printf(sbuf,
         "%s exactness_play %s %s%s %s%.*s %s%s%s-t '%s' ",
         _wrap_command ? _wrap_command : "",
         _mode == RUN_SIMULATION ? "-s" : "",
         _fonts_dir ? "-f " : "", _fonts_dir ? _fonts_dir : "",
         _verbose ? "-" : "", _verbose, "vvvvvvvvvv",
         _scan_objs ? "--scan-objects " : "",
         _disable_screenshots ? "--disable-screenshots " : "",
         _stabilize_shots ? "--stabilize-shots " : "",
         scn_path
         );

   if (_mode == RUN_PLAY)
      eina_strbuf_append_printf(sbuf, "-o '%s/%s/%s.exu' ", _dest_dir, CURRENT_SUBDIR, ent->name);
   if (_mode == RUN_INIT)
      eina_strbuf_append_printf(sbuf, "-o '%s' ", scn_path);

   if (ent->command)
     {
        eina_strbuf_append(sbuf, "-- ");
        eina_strbuf_append(sbuf, CONFIG);
        eina_strbuf_append(sbuf, ent->command);
     }
   strncpy(buf, eina_strbuf_string_get(sbuf), SCHEDULER_CMD_SIZE-1);
   eina_strbuf_free(sbuf);
   printf("Command: %s\n", buf);
   return EINA_TRUE;
}

/**
 * @brief A callback function executed as a job to perform image comparison.
 *
 * This is scheduled via `ecore_job_add` after a test run completes in 'play'
 * mode. It triggers the comparison, manages job counters, and checks if the
 * main loop should terminate.
 *
 * @param data A pointer to the `List_Entry` of the completed test.
 */
static void
_job_compare(void *data)
{
   _run_test_compare(data);

   _running_jobs--;
   _job_consume();
   /* If all jobs are done. */
   if (!_running_jobs) ecore_main_loop_quit();
}

/**
 * @brief Callback for the ECORE_EXE_EVENT_DEL event, triggered when a test process finishes.
 *
 * This function is the main handler for completed test processes. It checks for
 * execution errors, and for 'play' mode, it schedules the comparison job.
 * It also tries to start the next test in the queue.
 *
 * @param data User data, unused here.
 * @param type The type of the event, unused here.
 * @param event The event information, an `Ecore_Exe_Event_Del` struct.
 * @return ECORE_CALLBACK_RENEW to keep the handler active.
 */
static Eina_Bool
_job_deleted_cb(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Exe_Event_Del *msg = (Ecore_Exe_Event_Del *) event;
   List_Entry *ent = ecore_exe_data_get(msg->exe);

   if ((msg->exit_code != 0) || (msg->exit_signal != 0))
    {
        _errors = eina_list_append(_errors, ent);
     }

   if (_mode == RUN_PLAY)
     {
        ecore_job_add(_job_compare, ent);
     }
   else
     {
        _running_jobs--;
        _job_consume();
        if (!_running_jobs) ecore_main_loop_quit();
     }

   return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Consumes and runs the next available test from the queue.
 *
 * If the number of running jobs is less than the maximum allowed, this function
 * takes the next test from `_next_test_to_run`, prepares its command, and
 * executes it as a child process.
 *
 * @return EINA_TRUE if a new job was started, EINA_FALSE otherwise (e.g.,
 *         queue is empty or max jobs are running).
 */
static Eina_Bool
_job_consume()
{
   static Ecore_Event_Handler *job_del_callback_handler = NULL;
   char buf[SCHEDULER_CMD_SIZE];
   List_Entry *ent = _next_test_to_run;

   if (_running_jobs == _max_jobs) return EINA_FALSE;
   if (!ent) return EINA_FALSE;

   if (_run_command_prepare(ent, buf))
     {
        _running_jobs++;
        _tests_executed++;

        if (!job_del_callback_handler)
          {
             job_del_callback_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                   _job_deleted_cb, NULL);
          }

        if (!ecore_exe_pipe_run(buf, ECORE_EXE_TERM_WITH_PARENT, ent))
          {
             CRI("Failed executing test '%s'\n", ent->name);
          }
     }
   _next_test_to_run = EINA_INLIST_CONTAINER_GET(
         EINA_INLIST_GET(ent)->next, List_Entry);


   return EINA_TRUE;
}

/**
 * @brief Starts the test execution scheduler.
 *
 * This function kicks off the testing process by calling `_job_consume()`
 * repeatedly to fill all available parallel job slots. The actual scheduling
 * logic continues in the event-driven callbacks.
 */
static void
_scheduler_run()
{
   while (_job_consume());
}

/**
 * @brief Loads a test list from a file.
 *
 * Parses a file where each line specifies a test. The format for each line is:
 * `test_name command_and_arguments`
 * Lines starting with '#' or empty lines are ignored.
 *
 * @param filename The path to the test list file.
 * @return A pointer to the head of an `Eina_Inlist` of `List_Entry`
 *         structures, or NULL on failure. The list should be freed with
 *         `_list_file_free()`.
 */
static List_Entry *
_list_file_load(const char *filename)
{
   List_Entry *ret = NULL;
   char buf[BUF_SIZE] = "";
   FILE *file;
   file = fopen(filename, "r");
   if (!file)
     {
        perror("Failed opening list file");
        return NULL;
     }

   while (fgets(buf, BUF_SIZE, file))
     {
        /* Skip comment/empty lines. */
        if ((*buf == '#') || (*buf == '\n') || (!*buf))
           continue;

        char *tmp;
        List_Entry *cur = calloc(1, sizeof(*cur));
        cur->name = strdup(buf);

        /* Set the command to the second half and put a \0 in between. */
        tmp = strchr(cur->name, ' ');
        if (tmp)
          {
             *tmp = '\0';
             cur->command = tmp + 1;
          }
        else
          {
             /* FIXME: error. */
             cur->command = "";
          }

        /* Replace the newline char with a \0. */
        tmp = strchr(cur->command, '\n');
        if (tmp)
          {
             *tmp = '\0';
          }

        ret = EINA_INLIST_CONTAINER_GET(
              eina_inlist_append(EINA_INLIST_GET(ret), EINA_INLIST_GET(cur)),
              List_Entry);
     }

   fclose(file);
   return ret;
}

/**
 * @brief Frees all memory associated with a test list.
 *
 * Iterates through the `Eina_Inlist` of `List_Entry` and frees each entry's
 * allocated memory.
 *
 * @param list The head of the list to be freed.
 */
static void
_list_file_free(List_Entry *list)
{
   while (list)
     {
        List_Entry *ent = list;
        list = EINA_INLIST_CONTAINER_GET(EINA_INLIST_GET(list)->next,
              List_Entry);

        free(ent->name);
        free(ent);
        /* we don't free ent->command because it's allocated together. */
     }
}

/**
 * @brief Comparison function for sorting `List_Entry` items by name.
 *
 * Used with `eina_list_sort()` to sort the list of tests that had
 * execution errors.
 *
 * @param a The first list entry.
 * @param b The second list entry.
 * @return An integer less than, equal to, or greater than zero if `a->name`
 *         is found, respectively, to be less than, to match, or be greater
 *         than `b->name`.
 */
static int
_errors_sort_cb(List_Entry *a, List_Entry *b)
{
   return strcmp(a->name, b->name);
}

static const Ecore_Getopt optdesc = {
  "exactness",
  "%prog [options] <-p|-i|-s> <test list file>",
  PACKAGE_VERSION,
  "(C) 2013-2020 Enlightenment",
  "BSD",
  "A pixel-perfect test suite for EFL-based applications.\n"
  "\n"
  "This binary allows running the individual tools `exactness_record` and\n"
  "`exactness_play` in batch for a set of tests specified in a test list file.\n"
  "\n"
  "To obtain the reference templates (the \"golden\" files):\n"
  "1.For each tested application, run `exactness_record` to launch the\n"
  "   application and record actions like keystrokes and mouse clicks.\n"
  "   This produces a test file with `.exu` extension.\n"
  "2.Run `exactness` in init mode (-i) to execute the stored actions in all\n"
  "   the tests in the test list file and obtain screenshots.\n"
  "   The screenshots are embedded into the test `.exu` file.\n"
  "\n"
  "To check if the application currently matches the reference templates:\n"
  "1.Run `exactness` in play mode (-p) to execute the stored actions in all\n"
  "   the tests in the test list file and compare the obtained screenshots\n"
  "   with the stored versions (the reference templates).\n"
  "   If mismatches are detected an error report is produced, including\n"
  "   a graphical diff of the screenshots.\n"
  "\n"
  "The test list file contains one line per test. Each line starts with the\n"
  "test file name (without the `.exu` extension), a space and then the command\n"
  "to execute, including parameters if any. # indicates a comment.\n"
  "Example:\n"
  "separator elementary_test -to Separator",
  0,
  {
    ECORE_GETOPT_APPEND('b', "base-dir", "The location of the exu files. Defaults to `./recordings/`.",
      ECORE_GETOPT_TYPE_STR),
    ECORE_GETOPT_STORE_STR('o', "output", "The location of the images. Defaults to `./`."),
    ECORE_GETOPT_STORE_STR('w', "wrap", "Use a custom command to launch the tests (e.g valgrind)."),
    ECORE_GETOPT_STORE_USHORT('j', "jobs", "The number of jobs to run in parallel. Defaults to 1."),
    ECORE_GETOPT_STORE_TRUE('p', "play", "Run in play mode. Actions are executed and obtained "
      "screenshots are compared to the stored version (golden templates)."),
    ECORE_GETOPT_STORE_TRUE('i', "init", "Run in init mode. Actions are executed and obtained "
      "screenshots are stored to be used as golden templates in the future."),
    ECORE_GETOPT_STORE_TRUE('s', "simulation", "Run in simulation mode. Actions are executed and "
      "displayed but no screenshot is obtained. Useful for debugging."),
    ECORE_GETOPT_STORE_TRUE(0, "scan-objects", "Extract information of all the objects at every shot (UNUSED)."),
    ECORE_GETOPT_STORE_TRUE(0, "disable-screenshots", "Disable screenshots. Only checks that actions "
      "can be performed and the application does not crash."),
    ECORE_GETOPT_STORE_STR('f', "fonts-dir", "Specify a directory of the fonts that should be used."),
    ECORE_GETOPT_STORE_TRUE(0, "stabilize-shots", "Wait for the frames to be stable before taking the shots."),
    ECORE_GETOPT_COUNT('v', "verbose", "Turn verbose messages on."),

    ECORE_GETOPT_LICENSE('L', "license"),
    ECORE_GETOPT_COPYRIGHT('C', "copyright"),
    ECORE_GETOPT_VERSION('V', "version"),
    ECORE_GETOPT_HELP('h', "help"),
    ECORE_GETOPT_SENTINEL
  }
};

/**
 * @brief Main entry point of the exactness test scheduler.
 *
 * Parses command-line arguments, initializes the test environment, loads the
 * test list, starts the scheduler, and enters the Ecore main loop. After
 * tests are complete, it prints a summary and generates an HTML report for
 * any failures.
 *
 * @param argc The argument count.
 * @param argv The argument vector.
 * @return 0 on success, 1 on failure.
 */
int
main(int argc, char *argv[])
{
   int ret = 0;
   List_Entry *test_list;
   int args = 0;
   const char *list_file;
   Eina_List *itr;
   const char *base_dir;
   char tmp[PATH_MAX];
   Eina_Bool mode_play = EINA_FALSE, mode_init = EINA_FALSE, mode_simulation = EINA_FALSE;
   Eina_Bool want_quit = EINA_FALSE, scan_objs = EINA_FALSE;
   Ecore_Getopt_Value values[] = {
     ECORE_GETOPT_VALUE_LIST(_base_dirs),
     ECORE_GETOPT_VALUE_STR(_dest_dir),
     ECORE_GETOPT_VALUE_STR(_wrap_command),
     ECORE_GETOPT_VALUE_USHORT(_max_jobs),
     ECORE_GETOPT_VALUE_BOOL(mode_play),
     ECORE_GETOPT_VALUE_BOOL(mode_init),
     ECORE_GETOPT_VALUE_BOOL(mode_simulation),
     ECORE_GETOPT_VALUE_BOOL(scan_objs),
     ECORE_GETOPT_VALUE_BOOL(_disable_screenshots),
     ECORE_GETOPT_VALUE_STR(_fonts_dir),
     ECORE_GETOPT_VALUE_BOOL(_stabilize_shots),
     ECORE_GETOPT_VALUE_INT(_verbose),

     ECORE_GETOPT_VALUE_BOOL(want_quit),
     ECORE_GETOPT_VALUE_BOOL(want_quit),
     ECORE_GETOPT_VALUE_BOOL(want_quit),
     ECORE_GETOPT_VALUE_BOOL(want_quit),
     ECORE_GETOPT_VALUE_NONE
   };

   if (!ecore_evas_init())
      return EXIT_FAILURE;

   _log_domain = eina_log_domain_register("exactness", "red");
   _dest_dir = "./";
   _scan_objs = scan_objs;

   eina_log_abort_on_critical_set(EINA_TRUE);
   eina_log_abort_on_critical_level_set(EINA_LOG_LEVEL_ERR);

   args = ecore_getopt_parse(&optdesc, values, argc, argv);
   if (args < 0)
     {
        fprintf(stderr, "Failed parsing arguments.\n");
        ret = 1;
        goto end;
     }
   else if (want_quit)
     {
        ret = 1;
        goto end;
     }
   else if (args == argc)
     {
        fprintf(stderr, "Expected test list file as the last argument..\n");
        ecore_getopt_help(stderr, &optdesc);
        ret = 1;
        goto end;
     }
   else if (mode_play + mode_init + mode_simulation != 1)
     {
        fprintf(stderr, "Exactly one running mode must be set.\n");
        ecore_getopt_help(stderr, &optdesc);
        ret = 1;
        goto end;
     }

   if (!_base_dirs) _base_dirs = eina_list_append(NULL, "./recordings");

   list_file = argv[args];

   /* Load the list file and start iterating over the records. */
   test_list = _list_file_load(list_file);
   _next_test_to_run = test_list;

   if (!test_list)
     {
        fprintf(stderr, "No matching tests found in list file '%s'\n", list_file);
        ret = 1;
        goto end;
     }

   /* Pre-run summary */
   fprintf(stderr, "Running with settings:\n");
   fprintf(stderr, "\tConcurrent jobs: %d\n", _max_jobs);
   fprintf(stderr, "\tTest list file: %s\n", list_file);
   fprintf(stderr, "\tBase dirs:\n");
   EINA_LIST_FOREACH(_base_dirs, itr, base_dir)
      fprintf(stderr, "\t\t%s\n", base_dir);
   fprintf(stderr, "\tDest dir: %s\n", _dest_dir);

   if (mode_play)
     {
        _mode = RUN_PLAY;
        if (snprintf(tmp, PATH_MAX, "%s/%s", _dest_dir, CURRENT_SUBDIR)
            >= PATH_MAX)
          {
             fprintf(stderr, "Path too long: %s", tmp);
             ret = 1;
             goto end;
          }
        if (!ecore_file_exists(tmp))
          {
             if (mkdir(tmp, 0744) < 0)
               {
                  fprintf(stderr, "Failed to create dir %s", tmp);
                  ret = 1;
                  goto end;
               }
          }
     }
   else if (mode_init)
     {
        _mode = RUN_INIT;
        if (snprintf(tmp, PATH_MAX, "%s/%s", _dest_dir, ORIG_SUBDIR)
            >= PATH_MAX)
          {
             fprintf(stderr, "Path too long: %s", tmp);
             ret = 1;
             goto end;
          }
        if (!ecore_file_exists(tmp))
          {
             if (mkdir(tmp, 0744) < 0)
               {
                  fprintf(stderr, "Failed to create dir %s", tmp);
                  ret = 1;
                  goto end;
               }
          }
     }
   else if (mode_simulation)
     {
        _mode = RUN_SIMULATION;
     }
   _scheduler_run();


   ecore_main_loop_begin();

   /* Results */
   printf("*******************************************************\n");
   if (mode_play && EINA_FALSE)
     {
        List_Entry *list_itr;

        EINA_INLIST_FOREACH(test_list, list_itr)
          {
             _run_test_compare(list_itr);
          }
     }

   printf("Finished executing %u out of %u tests.\n",
         _tests_executed,
         eina_inlist_count(EINA_INLIST_GET(test_list)));
   printf("%u tests executed\n", _tests_executed);
   printf("%u tests had execution errors\n", eina_list_count(_errors));
   printf("%u screenshots failed comparison\n", eina_list_count(_compare_errors));

   /* Sort the errors and the compare_errors. */
   _errors = eina_list_sort(_errors, 0, (Eina_Compare_Cb) _errors_sort_cb);
   _compare_errors = eina_list_sort(_compare_errors, 0, (Eina_Compare_Cb) strcmp);

   if (_errors || _compare_errors)
     {
        FILE *report_file;
        char report_filename[PATH_MAX] = "";
        /* Generate the filename. */
        snprintf(report_filename, PATH_MAX,
              "%s/%s/errors.html",
              _dest_dir, mode_init ? ORIG_SUBDIR : CURRENT_SUBDIR);
        report_file = fopen(report_filename, "w+");
        if (report_file)
          {
             fprintf(report_file,
                   "<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
                   "<html xmlns=\"http://www.w3.org/1999/xhtml\"><head><title>Exactness report</title></head><body>");

             if (_errors)
               {
                  fprintf(report_file,
                        "<h1>Tests that failed execution:</h1><ul>");
                  List_Entry *ent;
                  printf("List of tests that failed execution:\n");
                  EINA_LIST_FOREACH(_errors, itr, ent)
                    {
                       printf("\t* %s\n", ent->name);

                       fprintf(report_file, "<li>%s</li>", ent->name);
                    }
                  fprintf(report_file, "</ul>");
               }

             if (_compare_errors)
               {
                  fprintf(report_file,
                        "<h1>Images that failed comparison: (Original, Current, Diff)</h1><ul>");
                  char *test_name;
                  printf("List of images that failed comparison:\n");
                  EINA_LIST_FREE(_compare_errors, test_name)
                    {
                       Eina_Bool is_from_exu;
                       char origpath[PATH_MAX];
                       snprintf(origpath, PATH_MAX, "%s/%s/orig/%s",
                             _dest_dir, CURRENT_SUBDIR, test_name);
                       is_from_exu = ecore_file_exists(origpath);
                       printf("\t* %s\n", test_name);

                       fprintf(report_file, "<li><h2>%s</h2> <img src='%sorig/%s' alt='Original' /> <img src='%s' alt='Current' /> <img src='comp_%s' alt='Diff' /></li>",
                             test_name, is_from_exu ? "" : "../",
                             test_name, test_name, test_name);
                       free(test_name);
                    }
                  fprintf(report_file, "</ul>");
               }
             fprintf(report_file,
                   "</body></html>");

             printf("Report html: %s\n", report_filename);
             ret = 1;
          }
        else
          {
             perror("Failed opening report file");
          }
     }

   _list_file_free(test_list);
end:
   ecore_evas_shutdown();

   return ret;
}
