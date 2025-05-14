/**
 * @file
 * @brief This program acts as a client for the elementary_quicklaunch daemon.
 *
 * It connects to a UNIX domain socket managed by the quicklaunch daemon
 * and sends command-line arguments, environment variables, and the current
 * working directory to it. This allows for faster application startup by
 * having a pre-initialized environment.
 * The program can be invoked directly or as `elementary_run`, which
 * affects how arguments are processed.
 */
#include "elementary_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#ifdef HAVE_ENVIRON
# define _GNU_SOURCE 1
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

#ifdef _WIN32
# include <direct.h> /* getcwd */
#endif

#ifdef HAVE_ENVIRON
extern char **environ;
#endif

#define LENGTH_OF_SOCKADDR_UN(s) (strlen((s)->sun_path) + (size_t)(((struct sockaddr_un *)NULL)->sun_path))

/**
 * @brief Main entry point for the elementary_quicklaunch client.
 *
 * Initializes a connection to the elementary_quicklaunch daemon,
 * prepares a data buffer containing arguments, environment variables,
 * and the current working directory, and sends this buffer to the daemon.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, non-zero on failure.
 */
int
main(int argc, char **argv)
{
   int sock, socket_unix_len, i, n;
   struct sockaddr_un socket_unix;
   char buf[PATH_MAX];
   struct stat st;
   char *exe;
   int we_are_elementary_run = 0;
   char *domain;
   char *cwd;
   char *rundir;

   int sargc, slen, envnum;
   unsigned char *sbuf = NULL, *pos;
   char **sargv = NULL;

   if (!getcwd(buf, sizeof(buf) - 1))
     {
        fprintf(stderr, "elementary_quicklaunch: currect working dir too big.\n");
        exit(-1);
     }
   cwd = strdup(buf);
   if (!(domain = getenv("ELM_QUICKLAUNCH_DOMAIN")))
     {
        domain = getenv("WAYLAND_DISPLAY");
        if (!domain) domain = getenv("DISPLAY");
        if (!domain) domain = "unknown";
     }
   rundir = getenv("XDG_RUNTIME_DIR");
   if (!rundir) rundir = "/tmp";
   // Construct the path to the UNIX domain socket.
   // The path is typically in $XDG_RUNTIME_DIR/elm-ql-<uid>/<domain>
   // where <domain> is derived from WAYLAND_DISPLAY, DISPLAY, or "unknown".
   snprintf(buf, sizeof(buf), "%s/elm-ql-%i/%s", rundir, getuid(), domain);
   if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
     {
        perror("elementary_quicklaunch: socket(AF_UNIX, SOCK_STREAM, 0)");
        exit(-1);
     }
   socket_unix.sun_family = AF_UNIX;
   strncpy(socket_unix.sun_path, buf, sizeof(socket_unix.sun_path));
   socket_unix.sun_path[(int)(sizeof(socket_unix.sun_path)/sizeof(socket_unix.sun_path[0])) - 1] = '\0';
   socket_unix_len = LENGTH_OF_SOCKADDR_UN(&socket_unix);
   if (connect(sock, (struct sockaddr *)&socket_unix, socket_unix_len) < 0)
     {
        perror("elementary_quicklaunch: connect(sock, (struct sockaddr *)&socket_unix, socket_unix_len)");
        printf("elementary_quicklaunch: cannot connect to socket '%s'\n", buf);
        exit(1);
     }
   exe = argv[0];
   // If the executable path is not absolute or relative, search for it in PATH.
   // This ensures that we can correctly identify `elementary_run` even if
   // it's called without a full path.
   if (!(((exe[0] == '/')) ||
         ((exe[0] == '.') && (exe[1] == '/')) ||
         ((exe[0] == '.') && (exe[1] == '.') && (exe[2] == '/'))))
     {
        char *path = getenv("PATH");
        int exelen = strlen(argv[0]);
        if (path)
          {
             const char *p, *pp;

             p = path;
             pp = p;
             exe = NULL;
             for (;;)
               {
                  if ((*p == ':') || (!*p))
                    {
                       unsigned int len;

                       len = p - pp;
                       if (len < (sizeof(buf) - exelen - 3))
                         {
                            strncpy(buf, pp, len);
                            strcpy(buf + len, "/");
                            strcpy(buf + len + 1, argv[0]);
                            if (!access(buf, R_OK | X_OK))
                              {
                                 exe = buf;
                                 break;
                              }
                            if (!*p) break;
                            p++;
                            pp = p;
                         }
                    }
                  else
                    {
                       if (!*p) break;
                       p++;
                    }
               }
          }
     }
   // Check if the executable (or the target of a symlink) is "elementary_run".
   // This determines whether the first argument (argv[0]) should be included
   // in the arguments passed to the quicklaunch daemon.
   if (exe)
     {
        if (!lstat(exe, &st))
          {
             if (S_ISLNK(st.st_mode))
               {
                  char buf2[PATH_MAX];

                  ssize_t len = readlink(exe, buf2, sizeof(buf2) - 1);
                  if (len >= 0)
                    {
                       char *p;
                       buf2[len] = 0;
                       p = strrchr(buf2, '/');
                       if (p) p++;
                       else p = buf2;
                       // Case-insensitive comparison for "elementary_run"
                       if (!strncasecmp(p, "elementary_run", 14))
                         we_are_elementary_run = 1;
                    }
               }
          }
     }
   // If running as elementary_run, all arguments (including argv[0]) are passed.
   // Otherwise, argv[0] is skipped.
   if (we_are_elementary_run)
     {
        sargc = argc;
        sargv = argv;
     }
   else
     {
        sargc = argc - 1;
        sargv = &(argv[1]);
     }

   slen = 0;
   envnum = 0;

   // Calculate the total size needed for the data buffer (sbuf).
   // The buffer will contain a header, argument strings, environment strings,
   // and the current working directory string.
   //
   // sbuf structure:
   // [ Header (3 * unsigned long) ]
   // [ Array of offsets for arguments (sargc * unsigned long) ]
   // [ Array of offsets for environment variables (envnum * unsigned long) ]
   // [ Offset for CWD string (1 * unsigned long) ]
   // [ Argument strings (null-terminated) ]
   // [ Environment strings (null-terminated) ]
   // [ CWD string (null-terminated) ]
   //
   // Header structure:
   //   - ((unsigned long *)sbuf)[0]: Total size of data following this field (slen - sizeof(unsigned long)).
   //   - ((unsigned long *)sbuf)[1]: Number of arguments (sargc).
   //   - ((unsigned long *)sbuf)[2]: Number of environment variables (envnum).
   //
   // Offsets:
   //   Each offset is an unsigned long value representing the byte offset
   //   from the beginning of sbuf to the start of the corresponding string.
   //
   // Example for sbuf with 1 arg ("arg1"), 1 env ("ENV=val"), cwd ("/home/user"):
   // [ total_payload_size_UL ] -> e.g., 60 (if UL is 8 bytes)
   // [ 1_UL (sargc)          ]
   // [ 1_UL (envnum)         ]
   // [ offset_to_arg1_UL     ] -> e.g., 40 (points to "arg1\0")
   // [ offset_to_ENV=val_UL  ] -> e.g., 45 (points to "ENV=val\0")
   // [ offset_to_cwd_UL      ] -> e.g., 53 (points to "/home/user\0")
   // [ 'a', 'r', 'g', '1', '\0' ]
   // [ 'E', 'N', 'V', '=', 'v', 'a', 'l', '\0' ]
   // [ '/', 'h', 'o', 'm', 'e', '/', 'u', 's', 'e', 'r', '\0' ]

   // Size for header (total_bytes, argnum, envnum)
   slen += sizeof(unsigned long) * 3;

   // Size for argument offsets and strings
   for (i = 0; i < sargc; i++)
     {
        slen += sizeof(unsigned long); // For offset
        slen += strlen(sargv[i]) + 1;  // For null-terminated string
     }

#ifdef HAVE_ENVIRON
   // Size for environment variable offsets and strings
   for (i = 0; environ[i]; i++)
     {
        slen += sizeof(unsigned long); // For offset
        slen += strlen(environ[i]) + 1;  // For null-terminated string
        envnum++;
     }
#endif

   // Size for CWD offset and string
   slen += sizeof(unsigned long); // For offset
   slen += strlen(cwd) + 1;       // For null-terminated string

   // Allocate buffer on stack using alloca for automatic cleanup.
   sbuf = alloca(slen);

   // Fill in header fields.
   ((unsigned long *)(sbuf))[0] = slen - sizeof(unsigned long); // Total payload size after this field
   ((unsigned long *)(sbuf))[1] = sargc;                       // Number of arguments
   ((unsigned long *)(sbuf))[2] = envnum;                      // Number of environment variables

   // pos points to the beginning of the string data area, after all headers and offsets.
   // The number of offset fields is: sargc (args) + envnum (env) + 1 (cwd).
   // The header itself has 3 unsigned long fields.
   pos = (unsigned char *)(&((((unsigned long *)(sbuf))[3 + sargc + envnum + 1])));
   n = 3; // Current index in the sbuf's unsigned long array, starting after the 3 header fields.

   // Fill in argument offsets and copy argument strings.
   for (i = 0; i < sargc; i++)
     {
        ((unsigned long *)(sbuf))[n] = (unsigned long)((unsigned char *)pos - (unsigned char *)sbuf); // Offset from start of sbuf
        strcpy((char *)pos, sargv[i]);
        pos += strlen(sargv[i]) + 1; // Move pos past the copied string and its null terminator
        n++; // Next offset slot
     }

#ifdef HAVE_ENVIRON
   // Fill in environment variable offsets and copy environment strings.
   for (i = 0; environ[i]; i++)
     {
        ((unsigned long *)(sbuf))[n] = (unsigned long)((unsigned char *)pos - (unsigned char *)sbuf); // Offset
        strcpy((char *)pos, environ[i]);
        pos += strlen(environ[i]) + 1;
        n++;
     }
#endif

   // Fill in CWD offset and copy CWD string.
   ((unsigned long *)(sbuf))[n] = (unsigned long)((unsigned char *)pos - (unsigned char *)sbuf); // Offset
   n++; // Although n is not used after this, it correctly represents the next slot.
   strcpy((char *)pos, cwd);
   // pos += strlen(cwd) + 1; // Not strictly necessary as pos is not used further for writing.

   // Write the entire serialized buffer to the socket.
   if (write(sock, sbuf, slen) < 0)
     printf("elementary_quicklaunch: cannot write to socket '%s'\n", buf);
   close(sock);

   free(cwd);
   return 0;
}
