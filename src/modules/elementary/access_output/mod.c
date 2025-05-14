#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include "Elementary.h"

#include "elm_module_helper.h"

/* to enable this module
export ELM_MODULES="access_output>access/api"
export ELM_ACCESS_MODE=1
 */

static void (*cb_func) (void *data);
static void *cb_data;
static Ecore_Exe *espeak = NULL;
static Ecore_Event_Handler *exe_exit_handler = NULL;
static Eina_Tmpstr *tmpf = NULL;
static int tmpfd = -1;

/**
 * @brief Ecore event handler called when the espeak process exits.
 *
 * This function is registered to be called on ECORE_EXE_EVENT_DEL. It checks
 * if the deleted process is the one we spawned for espeak. If so, it
 * performs cleanup of the temporary file used for text and calls the
 * completion callback.
 *
 * @param data User data, unused.
 * @param type The type of the event, unused.
 * @param event The event info, which is an Ecore_Exe_Event_Del.
 * @return ECORE_CALLBACK_RENEW to keep the handler active for future events.
 */
static Eina_Bool
_exe_del(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Exe_Event_Del *ev = event;

   if ((espeak) && (ev->exe == espeak))
     {
        if (tmpf)
          {
             unlink(tmpf);
             eina_tmpstr_del(tmpf);
             tmpf = NULL;
             close(tmpfd);
             tmpfd = -1;
          }
        espeak = NULL;
        if (cb_func) cb_func(cb_data);
     }
   return ECORE_CALLBACK_RENEW;
}

// module api funcs needed
/**
 * @brief Initializes the accessibility output module.
 *
 * This function is part of the EMODAPI and is called when the module is
 * loaded. It sets up an event handler to listen for the termination of
 * external processes, which is used to manage the espeak TTS process.
 *
 * @param m Module data, unused.
 * @return 1 on success.
 */
EMODAPI int
elm_modapi_init(void *m EINA_UNUSED)
{
   exe_exit_handler =
      ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
                              _exe_del, NULL);
   return 1; // succeed always
}

/**
 * @brief Shuts down the accessibility output module.
 *
 * This function is part of the EMODAPI and is called when the module is
 * unloaded. It cleans up by removing the ecore event handler.
 *
 * @param m Module data, unused.
 * @return 1 on success.
 */
EMODAPI int
elm_modapi_shutdown(void *m EINA_UNUSED)
{
   if (exe_exit_handler)
     {
        ecore_event_handler_del(exe_exit_handler);
        exe_exit_handler = NULL;
     }
   return 1; // succeed always
}

// module fucns for the specific module type
/**
 * @brief Receives text to be spoken and writes it to a temporary file.
 *
 * This function can be called multiple times to accumulate text. A temporary
 * file is created on the first call. Subsequent calls append text to this
 * file. The actual speech synthesis is triggered by out_read_done().
 *
 * @param txt A string of text to be read.
 */
EMODAPI void
out_read(const char *txt)
{
   if (!tmpf)
     {
        mode_t cur_umask;

        cur_umask = umask(S_IRWXO | S_IRWXG);
        tmpfd = eina_file_mkstemp("elm-speak-XXXXXX", &tmpf);
        umask(cur_umask);
        if (tmpfd < 0) return;
     }
   if (write(tmpfd, txt, strlen(txt)) < 0) perror("write to tmpfile (espeak)");
}

/**
 * @brief Triggers speech synthesis for the accumulated text.
 *
 * This function executes the 'espeak' command with the contents of the
 * temporary file. If espeak is already running, it's interrupted before
 * starting a new instance.
 */
EMODAPI void
out_read_done(void)
{
   char buf[PATH_MAX];

   if (espeak)
     {
        ecore_exe_interrupt(espeak);
        espeak = NULL;
     }
   if (tmpf)
     {
        // FIXME: espeak supporets -v XX for voice locale. should provide this
        // based on actual lang/locale
        if (tmpfd >= 0) close(tmpfd);
        tmpfd = -1;
        snprintf(buf, sizeof(buf), "espeak -p 2 -s 120 -k 10 -m -f %s", tmpf);
        espeak = ecore_exe_pipe_run(buf,
                                    ECORE_EXE_NOT_LEADER,
                                    NULL);
     }
}

/**
 * @brief Cancels the current speech synthesis.
 *
 * This function interrupts the running espeak process and cleans up
 * the temporary file.
 */
EMODAPI void
out_cancel(void)
{
   if (espeak)
     {
        ecore_exe_interrupt(espeak);
        espeak = NULL;
     }
   if (tmpf)
     {
        unlink(tmpf);
        eina_tmpstr_del(tmpf);
        tmpf = NULL;
        close(tmpfd);
        tmpfd = -1;
     }
}

/**
 * @brief Sets a callback function to be called when speech is complete.
 *
 * The provided function will be called when the espeak process finishes.
 * This is triggered from the _exe_del handler.
 *
 * @param func The function to call when speech is done.
 * @param data User data to be passed to the callback function.
 */
EMODAPI void
out_done_callback_set(void (*func) (void *data), const void *data)
{
   cb_func = func;
   cb_data = (void *)data;
}

/**
 * @brief Eina module initialization function.
 * @return EINA_TRUE on success.
 */
static Eina_Bool
_module_init(void)
{
   return EINA_TRUE;
}

/**
 * @brief Eina module shutdown function.
 */
static void
_module_shutdown(void)
{
}

EINA_MODULE_INIT(_module_init);
EINA_MODULE_SHUTDOWN(_module_shutdown);
