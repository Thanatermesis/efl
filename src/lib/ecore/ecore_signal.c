#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include "Ecore.h"
#include "ecore_private.h"

/* make mono happy - this is evil though... */
#undef SIGPWR

/**
 * @internal
 * @struct _Pid_Info
 * @brief Holds information about a process ID and its associated file descriptor.
 *
 * This structure is used to keep track of PIDs for which custom exit
 * information needs to be written to a specific file descriptor.
 */
typedef struct _Pid_Info Pid_Info;

struct _Pid_Info
{
   pid_t pid; /**< The process ID. */
   int fd;    /**< The file descriptor to write exit information to. */
};

/**
 * @internal
 * @brief Delays the sending of an ECORE_EXE_EVENT_DEL event.
 *
 * This function is called by a timer when an executable with piped I/O exits.
 * It ensures that any "last words" from the executable can be read before
 * the DEL event is processed.
 *
 * @param data An Ecore_Exe_Event_Del structure containing details of the exited process.
 * @param event The EFL event that triggered this callback (timer tick).
 */
static void _ecore_signal_exe_exit_delay(void *data, const Efl_Event *event);
/**
 * @internal
 * @brief Handles SIGCHLD signals by calling waitpid() to reap exited child processes.
 *
 * This function is called when a SIGCHLD signal is received, indicating that
 * a child process has changed state (e.g., terminated). It calls waitpid()
 * to get the status of exited children and generates ECORE_EXE_EVENT_DEL events.
 *
 * @param once If EINA_TRUE, only process one child and then return.
 *             If EINA_FALSE, process all available exited children.
 * @param info Signal information associated with the SIGCHLD.
 */
static void _ecore_signal_waitpid(Eina_Bool once, siginfo_t info);
/**
 * @internal
 * @brief Generic free function for Ecore events that simply frees the event data.
 *
 * @param data User data associated with the event (unused).
 * @param event The event data to be freed.
 */
static void _ecore_signal_generic_free(void *data, void *event);

/**
 * @internal
 * @typedef Signal_Handler
 * @brief Defines the signature for a signal handler function.
 *
 * @param sig The signal number.
 * @param si A pointer to a siginfo_t structure containing detailed information about the signal.
 * @param foo Unused context pointer (matches sa_sigaction signature).
 */
typedef void (*Signal_Handler)(int sig, siginfo_t *si, void *foo);

#define NUM_PIPES 5

static int sig_pipe[NUM_PIPES][2] = {{ -1 }}; /**< @internal Array of pipe file descriptors. [0] is read, [1] is write. Used for signal handling. */
static Eo *sig_pipe_handler[NUM_PIPES] = {NULL}; /**< @internal Array of Efl_Loop_Handler objects for reading from signal pipes. */
static Eina_Spinlock sig_pid_lock; /**< @internal Spinlock to protect access to sig_pid_info_list. */
static Eina_List *sig_pid_info_list = NULL; /**< @internal List of Pid_Info structures for custom PID exit handling. */

volatile int pipe_dead = 0; /**< @internal Flag indicating if the signal pipes are considered dead (e.g., during shutdown). */
volatile int exit_signal_received = 0; /**< @internal Flag indicating if an exit signal (SIGQUIT, SIGINT, SIGTERM) has been received. */

/**
 * @internal
 * @struct _Signal_Data
 * @brief Structure to hold signal number and associated siginfo_t.
 *
 * This structure is written to the internal pipes when a signal is caught
 * by the signal handler, to be processed by the main loop.
 */
typedef struct _Signal_Data
{
   int sig;        /**< The signal number. */
   siginfo_t info; /**< The siginfo_t structure associated with the signal. */
} Signal_Data;

/**
 * @internal
 * @brief Reads signal data from the internal pipes and processes them.
 *
 * This function is called when there is data to be read from one of the
 * signal pipes. It reads Signal_Data structures and generates corresponding
 * Ecore events or calls EFL application event callbacks.
 *
 * @param obj The Efl_Loop_Handler object that triggered the read.
 */
static void
_ecore_signal_pipe_read(Eo *obj)
{
   Signal_Data sdata;
   int ret;

   if (pipe_dead) return;
   for (unsigned int i = 0; i < NUM_PIPES; i++)
     {
        while (1)
          {
             ret = read(sig_pipe[i][0], &sdata, sizeof(sdata));

             /* read as many signals as we can, trying again if we get interrupted */
             if ((ret != sizeof(sdata)) && (errno != EINTR)) break;
             switch (sdata.sig)
               {
                case SIGPIPE:
                  break;
                case SIGALRM:
                  break;
                case SIGCHLD:
                  _ecore_signal_waitpid(EINA_FALSE, sdata.info);
                  break;
                case SIGUSR1:
                case SIGUSR2:
                    {
                       Ecore_Event_Signal_User *e = _ecore_event_signal_user_new();
                       if (e)
                         {
                            if (sdata.sig == SIGUSR1) e->number = 1;
                            else e->number = 2;
                            e->data = sdata.info;
                            ecore_event_add(ECORE_EVENT_SIGNAL_USER, e,
                                            _ecore_signal_generic_free, NULL);
                         }
                       Eo *loop = efl_provider_find(obj, EFL_LOOP_CLASS);
                       if (loop)
                         {
                            if (sdata.sig == SIGUSR1)
                              efl_event_callback_call(loop, EFL_APP_EVENT_SIGNAL_USR1, NULL);
                            else
                              efl_event_callback_call(loop, EFL_APP_EVENT_SIGNAL_USR2, NULL);
                         }
                    }
                  break;
                case SIGHUP:
                    {
                       Ecore_Event_Signal_Hup *e = _ecore_event_signal_hup_new();
                       if (e)
                         {
                            e->data = sdata.info;
                            ecore_event_add(ECORE_EVENT_SIGNAL_HUP, e,
                                            _ecore_signal_generic_free, NULL);
                         }
                       Eo *loop = efl_provider_find(obj, EFL_LOOP_CLASS);
                       if (loop)
                         efl_event_callback_call(loop, EFL_APP_EVENT_SIGNAL_HUP, NULL);
                    }
                  break;
                case SIGQUIT:
                case SIGINT:
                case SIGTERM:
                    {
                       Ecore_Event_Signal_Exit *e = _ecore_event_signal_exit_new();
                       if (e)
                         {
                            if (sdata.sig == SIGQUIT) e->quit = 1;
                            else if (sdata.sig == SIGINT) e->interrupt = 1;
                            else e->terminate = 1;
                            e->data = sdata.info;
                            ecore_event_add(ECORE_EVENT_SIGNAL_EXIT, e,
                                            _ecore_signal_generic_free, NULL);
                         }
                       Eo *loop = efl_provider_find(obj, EFL_LOOP_CLASS);
                       if (loop)
                         efl_event_callback_call(loop, EFL_LOOP_EVENT_QUIT, NULL);
                    }
                  break;
          #ifdef SIGPWR
                case SIGPWR:
                    {
                       Ecore_Event_Signal_Power *e = _ecore_event_signal_power_new();
                       if (e)
                         {
                            e->data = sdata.info;
                            ecore_event_add(ECORE_EVENT_SIGNAL_POWER, e,
                                            _ecore_signal_generic_free, NULL);
                         }
                    }
                  break;
          #endif
                default:
                  break;
               }
        }
     }
}

/**
 * @internal
 * @brief Callback function for EFL_LOOP_HANDLER_EVENT_READ on signal pipes.
 *
 * This function is invoked by the main loop when one of the signal pipes
 * has data available for reading. It calls _ecore_signal_pipe_read to
 * process the incoming signal data.
 *
 * @param data User data associated with the callback (unused).
 * @param event The EFL_LOOP_HANDLER_EVENT_READ event.
 */
static void
_ecore_signal_cb_read(void *data EINA_UNUSED, const Efl_Event *event EINA_UNUSED)
{
   _ecore_signal_pipe_read(event->object);
}

/**
 * @internal
 * @brief The actual signal handler function installed via sigaction.
 *
 * This function is executed in the signal handler context when a registered
 * signal is caught. It writes the signal information (Signal_Data) to the
 * internal pipes to be processed by the main loop. It also sets the
 * `exit_signal_received` flag for exit signals.
 *
 * @param sig The signal number that was caught.
 * @param si A pointer to a siginfo_t structure containing detailed information about the signal.
 * @param foo Unused context pointer (matches sa_sigaction signature).
 */
static void
_ecore_signal_callback(int sig, siginfo_t *si, void *foo EINA_UNUSED)
{
   Signal_Data sdata;

   memset(&sdata, 0, sizeof(Signal_Data));
   sdata.sig = sig;
   sdata.info = *si;
   if (sdata.sig >= 0)
     {
        int err = errno;
        if (pipe_dead) return;
        for (unsigned int i = 0; i < NUM_PIPES; i++)
          {
             do
               {
                  err = 0;
                  const ssize_t bytes = write(sig_pipe[i][1], &sdata, sizeof(sdata));
                  if (EINA_UNLIKELY(bytes != sizeof(sdata)))
                    {
                       err = errno;
                       if (err == EINTR)
                         DBG("signal pipe %u full", i);
                       else if (i == NUM_PIPES - 1) //only print errors on last pipe
                         ERR("write() failed: %d: %s", err, strerror(err));
                    }
                  errno = err;
                  /* loop if we got preempted */
               } while (err == EINTR);
             if (!err) break;
          }
     }
   switch (sig)
     {
      case SIGQUIT:
      case SIGINT:
      case SIGTERM:
        exit_signal_received = 1;
        break;
      default: break;
     }
}

/**
 * @internal
 * @brief Sets up a signal handler for a given signal.
 *
 * This function configures and installs a signal handler using sigaction.
 * It sets the SA_RESTART and SA_SIGINFO flags.
 *
 * @param sig The signal number to handle.
 * @param func The Signal_Handler function to be called when the signal occurs.
 */
static void
_ecore_signal_callback_set(int sig, Signal_Handler func)
{
   struct sigaction sa;

   sa.sa_sigaction = func;
   sa.sa_flags = SA_RESTART | SA_SIGINFO;
   sigemptyset(&sa.sa_mask);
   sigaction(sig, &sa, NULL);
}

/**
 * @internal
 * @brief Sets up all necessary signal handlers for Ecore.
 *
 * This function calls _ecore_signal_callback_set for various signals
 * like SIGPIPE, SIGALRM, SIGCHLD, etc., to use _ecore_signal_callback
 * as their handler. It also unblocks these signals for the current thread.
 */
static void
_signalhandler_setup(void)
{
   sigset_t newset;

   _ecore_signal_callback_set(SIGPIPE, _ecore_signal_callback);
   _ecore_signal_callback_set(SIGALRM, _ecore_signal_callback);
   _ecore_signal_callback_set(SIGCHLD, _ecore_signal_callback);
   _ecore_signal_callback_set(SIGUSR1, _ecore_signal_callback);
   _ecore_signal_callback_set(SIGUSR2, _ecore_signal_callback);
   _ecore_signal_callback_set(SIGHUP,  _ecore_signal_callback);
   _ecore_signal_callback_set(SIGQUIT, _ecore_signal_callback);
   _ecore_signal_callback_set(SIGINT,  _ecore_signal_callback);
   _ecore_signal_callback_set(SIGTERM, _ecore_signal_callback);
#ifdef SIGPWR
   _ecore_signal_callback_set(SIGPWR,  _ecore_signal_callback);
#endif

#ifndef _WIN32
   sigemptyset(&newset);
   sigaddset(&newset, SIGPIPE);
   sigaddset(&newset, SIGALRM);
   sigaddset(&newset, SIGCHLD);
   sigaddset(&newset, SIGUSR1);
   sigaddset(&newset, SIGUSR2);
   sigaddset(&newset, SIGHUP);
   sigaddset(&newset, SIGQUIT);
   sigaddset(&newset, SIGINT);
   sigaddset(&newset, SIGTERM);
# ifdef SIGPWR
   sigaddset(&newset, SIGPWR);
# endif
   pthread_sigmask(SIG_UNBLOCK, &newset, NULL);
#endif
}

/**
 * @internal
 * @brief Initializes the signal handling pipes and related structures.
 *
 * This function creates the spinlock for PID info, sets up signal handlers
 * via _signalhandler_setup(), creates the communication pipes, sets them
 * to non-blocking and close-on-exec, and adds Efl_Loop_Handlers to
 * monitor the read ends of these pipes.
 */
static void
_ecore_signal_pipe_init(void)
{
   eina_spinlock_new(&sig_pid_lock);
   _signalhandler_setup();
   if (sig_pipe[0][0] == -1)
     {
        for (unsigned int i = 0; i < NUM_PIPES; i++)
          {
            if (pipe(sig_pipe[i]) != 0)
              {
                 CRI("failed setting up signal pipes! %s", strerror(errno));
                 for (unsigned int j = 0; j < i; j++)
                   {
                      close(sig_pipe[j][0]);
                      close(sig_pipe[j][1]);
                   }
                 memset(sig_pipe, -1, sizeof(sig_pipe));
                 return;
              }
            eina_file_close_on_exec(sig_pipe[i][0], EINA_TRUE);
            eina_file_close_on_exec(sig_pipe[i][1], EINA_TRUE);
            if (fcntl(sig_pipe[i][0], F_SETFL, O_NONBLOCK) < 0)
              ERR("can't set pipe to NONBLOCK");
            if (fcntl(sig_pipe[i][1], F_SETFL, O_NONBLOCK) < 0)
              ERR("can't set pipe to NONBLOCK");
            efl_add(EFL_LOOP_HANDLER_CLASS, ML_OBJ,
                    efl_loop_handler_fd_set(efl_added, sig_pipe[i][0]),
                    efl_loop_handler_active_set(efl_added, EFL_LOOP_HANDLER_FLAGS_READ),
                    efl_event_callback_add(efl_added, EFL_LOOP_HANDLER_EVENT_READ, _ecore_signal_cb_read, NULL),
                    efl_wref_add(efl_added, &sig_pipe_handler[i])
                    );

          }
     }
}

/**
 * @internal
 * @brief Shuts down the signal handling pipes and cleans up resources.
 *
 * This function closes all pipe file descriptors, deletes the associated
 * Efl_Loop_Handlers, and frees the spinlock.
 */
static void
_ecore_signal_pipe_shutdown(void)
{
   if (sig_pipe[0][0] != -1)
     {
        for (unsigned int i = 0; i < NUM_PIPES; i++)
          {
             close(sig_pipe[i][0]);
             close(sig_pipe[i][1]);
             efl_del(sig_pipe_handler[i]);
          }
     }
   memset(sig_pipe, -1, sizeof(sig_pipe));
   eina_spinlock_free(&sig_pid_lock);
}

/**
 * @internal
 * @brief Callback function registered with ecore_fork_reset_callback_add.
 *
 * This function is called after a fork() to re-initialize the signal
 * handling mechanism (pipes, handlers) in the child process. It first
 * shuts down the existing (copied from parent) pipe setup and then
 * initializes a new one.
 *
 * @param data User data associated with the callback (unused).
 */
static void
_ecore_signal_cb_fork(void *data EINA_UNUSED)
{
   _ecore_signal_pipe_shutdown();
   _ecore_signal_pipe_init();
}

/**
 * @internal
 * @brief Initializes the Ecore signal handling subsystem.
 *
 * Sets up the signal pipes and registers a callback to re-initialize
 * after a fork.
 * This is called once during Ecore initialization.
 */
void
_ecore_signal_init(void)
{
   pipe_dead = 0;
   _ecore_signal_pipe_init();
   ecore_fork_reset_callback_add(_ecore_signal_cb_fork, NULL);
}

/**
 * @internal
 * @brief Shuts down the Ecore signal handling subsystem.
 *
 * Unregisters the fork callback, marks pipes as dead, blocks signals,
 * and shuts down the signal pipes.
 * This is called once during Ecore shutdown.
 */
void
_ecore_signal_shutdown(void)
{
   sigset_t newset;

   ecore_fork_reset_callback_del(_ecore_signal_cb_fork, NULL);
   pipe_dead = 1;
   // we probably should restore.. but not a good idea
   // pthread_sigmask(SIG_SETMASK, &sig_oldset, NULL);
   // at least do not trigger signal callback after shutdown
#ifndef _WIN32
   sigemptyset(&newset);
   sigaddset(&newset, SIGPIPE);
   sigaddset(&newset, SIGALRM);
   sigaddset(&newset, SIGCHLD);
   sigaddset(&newset, SIGUSR1);
   sigaddset(&newset, SIGUSR2);
   sigaddset(&newset, SIGHUP);
   sigaddset(&newset, SIGQUIT);
   sigaddset(&newset, SIGINT);
   sigaddset(&newset, SIGTERM);
# ifdef SIGPWR
   sigaddset(&newset, SIGPWR);
# endif
   pthread_sigmask(SIG_BLOCK, &newset, NULL);
#endif
   _ecore_signal_pipe_shutdown();
   exit_signal_received = 0;
}

/**
 * @internal
 * @brief Processes received signals. (Currently a no-op).
 *
 * This function is intended to be called by the Efl_Loop to process signals.
 * However, signal processing is handled by the EFL_LOOP_HANDLER_EVENT_READ
 * callback (_ecore_signal_cb_read) on the signal pipes.
 *
 * @param obj The Efl_Loop object (unused).
 * @param pd The Efl_Loop_Data (unused).
 */
void
_ecore_signal_received_process(Eo *obj EINA_UNUSED, Efl_Loop_Data *pd EINA_UNUSED)
{
   // do nothing - the efl loop handler read event will handle it
}

/**
 * @internal
 * @brief Gets the count of pending signals. (Always returns 0).
 *
 * Signals are processed via pipe reads, not a direct queue count here.
 *
 * @param obj The Efl_Loop object (unused).
 * @param pd The Efl_Loop_Data (unused).
 * @return int Always 0, as signals are handled via pipe events.
 */
int
_ecore_signal_count_get(Eo *obj EINA_UNUSED, Efl_Loop_Data *pd EINA_UNUSED)
{
   // we will always have 0 signals be3cause they will instead be read from
   // a pipe fd and placed in a queue/list that
   // _ecore_signal_received_process() will then walk and process/do
   return 0;
}

/**
 * @internal
 * @brief Acquires the spinlock for PID information list access.
 *
 * This ensures thread-safe access to `sig_pid_info_list`.
 */
void
_ecore_signal_pid_lock(void)
{
   eina_spinlock_take(&sig_pid_lock);
}

/**
 * @internal
 * @brief Releases the spinlock for PID information list access.
 */
void
_ecore_signal_pid_unlock(void)
{
   eina_spinlock_release(&sig_pid_lock);
}

/**
 * @internal
 * @brief Registers a PID and an associated file descriptor for custom exit signal handling.
 *
 * When a child process with the given `pid` exits, its exit information
 * (Ecore_Signal_Pid_Info) will be written to the specified `fd` instead of
 * generating a standard ECORE_EXE_EVENT_DEL.
 * The `sig_pid_lock` should be held before calling this function.
 *
 * @param pid The process ID to monitor.
 * @param fd The file descriptor to write exit information to.
 */
void
_ecore_signal_pid_register(pid_t pid, int fd)
{
   Pid_Info *pi = calloc(1, sizeof(Pid_Info));
   if (!pi) return;
   pi->pid = pid;
   pi->fd = fd;
   sig_pid_info_list = eina_list_append(sig_pid_info_list, pi);
}

/**
 * @internal
 * @brief Unregisters a PID and file descriptor previously registered with _ecore_signal_pid_register().
 *
 * Removes the Pid_Info entry from `sig_pid_info_list`.
 * The `sig_pid_lock` should be held before calling this function.
 *
 * @param pid The process ID to unregister.
 * @param fd The file descriptor associated with the PID.
 */
void
_ecore_signal_pid_unregister(pid_t pid, int fd)
{
   Eina_List *l;
   Pid_Info *pi;

   EINA_LIST_FOREACH(sig_pid_info_list, l, pi)
     {
        if ((pi->pid == pid) && (pi->fd == fd))
          {
             sig_pid_info_list = eina_list_remove_list(sig_pid_info_list, l);
             free(pi);
             return;
          }
     }
}

static void
_ecore_signal_exe_exit_delay(void *data, const Efl_Event *event)
{
   Ecore_Exe_Event_Del *e = data;

   if (!e) return;
   _ecore_exe_doomsday_clock_set(e->exe, NULL);
   ecore_event_add(ECORE_EXE_EVENT_DEL, e,
                   _ecore_exe_event_del_free, NULL);
   efl_del(event->object);
}

static void
_ecore_signal_waitpid(Eina_Bool once, siginfo_t info)
{
   pid_t pid;
   int status;

   while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
     {
        Ecore_Exe_Event_Del *e = _ecore_exe_event_del_new();

        //FIXME: If this process is set respawn, respawn with a suitable backoff
        // period for those that need too much respawning.
        if (e)
          {
             if (WIFEXITED(status))
               {
                  e->exit_code = WEXITSTATUS(status);
                  e->exited = 1;
               }
             else if (WIFSIGNALED(status))
               {
                  e->exit_signal = WTERMSIG(status);
                  e->signalled = 1;
               }
             e->pid = pid;
             e->exe = _ecore_exe_find(pid);
             e->data = info;  // No need to clone this.
             if ((e->exe) &&
                 (ecore_exe_flags_get(e->exe) &
                  (ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_ERROR)))
               {
                  /* We want to report the Last Words of the exe, so delay this event.
                   * This is twice as relevant for stderr.
                   * There are three possibilities here -
                   *  1 There are no Last Words.
                   *  2 There are Last Words, they are not ready to be read.
                   *  3 There are Last Words, they are ready to be read.
                   *
                   * For 1 we don't want to delay, for 3 we want to delay.
                   * 2 is the problem.  If we check for data now and there
                   * is none, then there is no way to differentiate 1 and 2.
                   * If we don't delay, we may loose data, but if we do delay,
                   * there may not be data and the exit event never gets sent.
                   *
                   * Any way you look at it, there has to be some time passed
                   * before the exit event gets sent.  So the strategy here is
                   * to setup a timer event that will send the exit event after
                   * an arbitrary, but brief, time.
                   *
                   * This is probably paranoid, for the less paraniod, we could
                   * check to see for Last Words, and only delay if there are any.
                   * This has it's own set of problems. */
                  efl_del(_ecore_exe_doomsday_clock_get(e->exe));

                  Efl_Loop_Timer *doomsday_clock =
                    efl_add(EFL_LOOP_TIMER_CLASS, ML_OBJ,
                            efl_loop_timer_interval_set(efl_added, 0.1),
                            efl_event_callback_add
                            (efl_added, EFL_LOOP_TIMER_EVENT_TIMER_TICK,
                             _ecore_signal_exe_exit_delay, e));
                  _ecore_exe_doomsday_clock_set(e->exe, doomsday_clock);
               }
             else ecore_event_add(ECORE_EXE_EVENT_DEL, e,
                                  _ecore_exe_event_del_free, NULL);
          }

        // XXX: this is not brilliant. this ends up running from the main loop
        // reading the signal pipe to handle signals. that means handling
        // exe exits from children will be bottlenecked by how often
        // the main loop can wake up (or well latency may not be great).
        // this should probably have a dedicated thread ythat does a waitpid()
        // and blocks and waits sending results to the resulting pipe
        Eina_List *l, *ll;
        Pid_Info *pi;

        EINA_LIST_FOREACH_SAFE(sig_pid_info_list, l, ll, pi)
          {
             if (pi->pid == pid)
               {
                  Ecore_Signal_Pid_Info pinfo;

                  sig_pid_info_list = eina_list_remove_list
                    (sig_pid_info_list, ll);
                  pinfo.pid = pid;
                  pinfo.info = info;
                  if (WIFEXITED(status))
                    {
                       pinfo.exit_code = WEXITSTATUS(status);
                       pinfo.exit_signal = -1;
                    }
                  else if (WIFSIGNALED(status))
                    {
                       pinfo.exit_code = -1;
                       pinfo.exit_signal = WTERMSIG(status);
                    }
                  if (write(pi->fd, &pinfo, sizeof(Ecore_Signal_Pid_Info))
                      != sizeof(Ecore_Signal_Pid_Info))
                    {
                       ERR("Can't write to custom exe exit info pipe");
                    }
                  free(pi);
                  break;
               }
          }
        if (once) break;
     }
}

/**
 * @internal
 * @brief Frees an Ecore_Event_Signal_* event structure.
 *
 * This function is used as the `free_func` for various signal-related
 * Ecore events added via `ecore_event_add`.
 *
 * @param data User-supplied data (unused in this context).
 * @param event Pointer to the event structure to be freed.
 */
static void
_ecore_signal_generic_free(void *data EINA_UNUSED, void *event)
{
   free(event);
}
