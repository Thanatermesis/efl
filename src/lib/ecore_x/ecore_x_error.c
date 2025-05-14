#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* ifdef HAVE_CONFIG_H */

#include <stdlib.h>

#include "Ecore.h"
#include "ecore_private.h"
#include "ecore_x_private.h"
#include "Ecore_X.h"

static int _ecore_x_error_handle(Display *d,
                                 XErrorEvent *ev);
static int _ecore_x_io_error_handle(Display *d);

/** @internal Global pointer to the user-defined X error handler function. */
static void (*_error_func)(void *data) = NULL;
/** @internal Global pointer to the user-defined data for the X error handler. */
static void *_error_data = NULL;
/** @internal Global pointer to the user-defined X I/O error handler function. */
static void (*_io_error_func)(void *data) = NULL;
/** @internal Global pointer to the user-defined data for the X I/O error handler. */
static void *_io_error_data = NULL;
/** @internal Stores the major request code of the last X error. */
static int _error_request_code = 0;
/** @internal Stores the error code of the last X error. */
static int _error_code = 0;
/** @internal Stores the resource ID associated with the last X error. */
static Ecore_X_ID _error_resource_id = 0;

/**
 * @brief Sets a function to be called when an X protocol error occurs.
 *
 * This function sets a handler that will be called when an X server error
 * is reported. These errors are not fatal, and the application will
 * continue running after the handler is called.
 *
 * The handler function will be passed the @p data pointer given here.
 * Inside the handler, you can use ecore_x_error_request_get(),
 * ecore_x_error_code_get(), and ecore_x_error_resource_id_get() to get
 * more information about the error.
 *
 * @param func The function to call when an X error occurs.
 * @param data A pointer to data to pass to @p func.
 */
EAPI void
ecore_x_error_handler_set(void (*func)(void *data),
                          const void *data)
{
   _error_func = func;
   _error_data = (void *)data;
}

/**
 * @brief Sets a function to be called when a fatal X I/O error occurs.
 *
 * This function sets a handler that will be called when a fatal I/O error
 * occurs with the X server connection. This usually means the X server has
 * been terminated. When this happens, Ecore-X is no longer functional.
 *
 * If no handler is set with this function, Ecore will call `exit(-1)` on a
 * fatal I/O error, which is the default Xlib behavior. An application may
 * wish to set a handler to perform cleanup before exiting.
 *
 * The handler is not expected to return.
 *
 * @param func The function to call when an X I/O error occurs.
 * @param data A pointer to data to pass to @p func.
 */
EAPI void
ecore_x_io_error_handler_set(void (*func)(void *data),
                             const void *data)
{
   _io_error_func = func;
   _io_error_data = (void *)data;
}

/**
 * @brief Retrieves the major request code of the last X error.
 *
 * This function should be called from within an error handler set by
 * ecore_x_error_handler_set(). It returns the major request code
 * of the X protocol request that caused the error.
 *
 * @return The major request code of the failed request.
 */
EAPI int
ecore_x_error_request_get(void)
{
   return _error_request_code;
}

/**
 * @brief Retrieves the error code of the last X error.
 *
 * This function should be called from within an error handler set by
 * ecore_x_error_handler_set(). It returns the error code of the
 * last X error that occurred. These codes are defined in `<X11/X.h>`.
 *
 * @return The error code of the last X error.
 *
 * @see ecore_x_error_handler_set()
 */
//FIXME: Use Ecore_X_Error_Code type when 2.0 is released
EAPI int
ecore_x_error_code_get(void)
{
   return _error_code;
}

/**
 * @brief Retrieves the resource ID associated with the last X error.
 *
 * This function should be called from within an error handler set by
 * ecore_x_error_handler_set(). It returns the XID of the resource
 * (e.g., Window, Pixmap) that was being manipulated when the error occurred.
 *
 * @return The resource ID related to the error.
 *
 * @see ecore_x_error_handler_set()
 */
EAPI Ecore_X_ID
ecore_x_error_resource_id_get(void)
{
   return _error_resource_id;
}

/**
 * @internal
 * @brief Initialize the X error handlers.
 *
 * This function sets the Ecore Xlib internal error handlers
 * for X errors and X I/O errors. It should be called once
 * during Ecore_X initialization.
 */
void
_ecore_x_error_handler_init(void)
{
   XSetErrorHandler((XErrorHandler)_ecore_x_error_handle);
   XSetIOErrorHandler((XIOErrorHandler)_ecore_x_io_error_handle);
}

/**
 * @internal
 * @brief Handles X protocol errors.
 *
 * This function is registered with Xlib as the error handler.
 * It is called when an X protocol error occurs.
 * It stores the error details and calls the user-registered error handler.
 *
 * @param d The display connection where the error occurred.
 * @param ev The XErrorEvent structure containing error details.
 * @return Always returns 0, as required by Xlib error handlers.
 */
static int
_ecore_x_error_handle(Display *d,
                      XErrorEvent *ev)
{
   // If _ecore_xlib_sync is disabled, X errors are expected (e.g. during
   // a sync call to check if a window exists). In this case, we don't
   // want to log them or call the user's error handler.
   if (!_ecore_xlib_sync) goto skip;
   switch (ev->error_code)
     {
      case BadRequest:	/* bad request code */
        ERR("BadRequest");
        break;
      case BadValue:	/* int parameter out of range */
        ERR("BadValue");
        break;
      case BadWindow:	/* parameter not a Window */
        ERR("BadWindow");
        break;
      case BadPixmap:	/* parameter not a Pixmap */
        ERR("BadPixmap");
        break;
      case BadAtom:	/* parameter not an Atom */
        ERR("BadAtom");
        break;
      case BadCursor:	/* parameter not a Cursor */
        ERR("BadCursor");
        break;
      case BadFont:	/* parameter not a Font */
        ERR("BadFont");
        break;
      case BadMatch:	/* parameter mismatch */
        ERR("BadMatch");
        break;
      case BadDrawable:	/* parameter not a Pixmap or Window */
        ERR("BadDrawable");
        break;
      case BadAccess:	/* depending on context */
        ERR("BadAccess");
        break;
      case BadAlloc:	/* insufficient resources */
        ERR("BadAlloc");
        break;
      case BadColor:	/* no such colormap */
        ERR("BadColor");
        break;
      case BadGC:	/* parameter not a GC */
        ERR("BadGC");
        break;
      case BadIDChoice:	/* choice not in range or already used */
        ERR("BadIDChoice");
        break;
      case BadName:	/* font or color name doesn't exist */
        ERR("BadName");
        break;
      case BadLength:	/* Request length incorrect */
        ERR("BadLength");
        break;
      case BadImplementation:	/* server is defective */
        ERR("BadImplementation");
        break;
     }
skip:
   if (d == _ecore_x_disp)
     {
        _error_request_code = ev->request_code;
        _error_code = ev->error_code;
        _error_resource_id = ev->resourceid;
        if (_error_func)
          _error_func(_error_data);
     }
   return 0;
}

/**
 * @internal
 * @brief Handles X I/O errors.
 *
 * This function is registered with Xlib as the I/O error handler.
 * It is called when a fatal I/O error occurs (e.g., the connection
 * to the X server is lost).
 * It calls the user-registered I/O error handler or exits if none is set.
 *
 * @param d The display connection where the I/O error occurred.
 * @return This function should not return if it calls exit(). If the
 *         user handler is called and returns, this function returns 0.
 *         However, Xlib typically expects I/O error handlers to not return.
 */
static int
_ecore_x_io_error_handle(Display *d)
{
   // Only handle errors for the main display connection.
   if (d == _ecore_x_disp)
     {
        if (_io_error_func)
          {
             // Mark the display as disconnected and perform cleanup.
             _ecore_x_disp = NULL;
             _ecore_x_shutdown();
             // Call the user-defined I/O error handler.
             _io_error_func(_io_error_data);
          }
        else
          {
             // If no user handler is set, exit the application.
             // This is the default Xlib behavior.
             exit(-1);
          }
     }

   return 0;
}

