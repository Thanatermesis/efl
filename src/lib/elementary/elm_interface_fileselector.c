#ifdef HAVE_CONFIG_H
# include "elementary_config.h" // Provides build-time configuration options for Elementary.
#endif


#include "elm_priv.h" // Includes private headers for Elementary, necessary for internal implementations.
#include "Eio.h"      // Provides Asynchronous I/O operations, potentially used for file system interactions.

#include "elm_interface_fileselector.h" // Declares the fileselector interface itself.

// Include the generated C-bindings for the fileselector Eo interface.
#include "elm_interface_fileselector_eo.h"
// Include the generated implementation for the fileselector Eo interface.
// This typically contains the method table and other boilerplate for the Eo class.
#include "elm_interface_fileselector_eo.c"
