/**
 * @file
 * @brief These functions and macros handle the indirection of Eo object pointers.
 *
 * Eo uses an ID system to refer to objects. This ID needs to be resolved
 * to an actual memory pointer (_Eo_Object*). This file provides the
 * mechanisms for this resolution, including error handling and caching.
 * It supports both thread-local and shared object domains.
 */
#ifndef EO_PTR_INDIRECTION_H
#define EO_PTR_INDIRECTION_H

#include "Eo.h"
#include "eo_private.h"

/* Macro used to obtain the object pointer and return if fails. */

/**
 * @brief Logs an error related to Eo pointer resolution.
 *
 * This function is a utility to centralize error logging for pointer
 * issues, allowing for easier debugging (e.g., setting a breakpoint here).
 *
 * @param obj_id The Eo ID that caused the error.
 * @param func_name The name of the function where the error occurred.
 * @param file The source file where the error occurred.
 * @param line The line number in the source file where the error occurred.
 * @param fmt The format string for the error message.
 * @param ... Additional arguments for the format string.
 */
void _eo_pointer_error(const Eo *obj_id, const char *func_name, const char *file, int line, const char *fmt, ...);

/**
 * @def _EO_POINTER_ERR(obj_id, fmt, ...)
 * @brief Macro to simplify calling _eo_pointer_error with current function context.
 *
 * @param obj_id The Eo ID that caused the error.
 * @param fmt The format string for the error message.
 * @param ... Additional arguments for the format string.
 */
#define _EO_POINTER_ERR(obj_id, fmt, ...) \
  _eo_pointer_error(obj_id, __func__, __FILE__, __LINE__, fmt, __VA_ARGS__)

/**
 * @def EO_OBJ_POINTER(obj_id, obj)
 * @brief Retrieves the internal _Eo_Object pointer from an Eo_Id.
 *
 * Declares a local variable `obj` of type `_Eo_Object*` and initializes it
 * with the resolved pointer for `obj_id`. If resolution fails, an error
 * is logged, but execution continues (obj will be NULL).
 *
 * @param obj_id The Eo_Id to resolve.
 * @param obj The name of the `_Eo_Object*` variable to declare and assign.
 *
 * @par Example:
 * @code
 * void my_function(Eo *my_eo_id) {
 *   EO_OBJ_POINTER(my_eo_id, obj_ptr);
 *   if (!obj_ptr) {
 *     // Handle error, obj_ptr is NULL
 *     return;
 *   }
 *   // Use obj_ptr
 * }
 * @endcode
 */
#define EO_OBJ_POINTER(obj_id, obj) \
   _Eo_Object *obj; \
   do { \
      obj = _eo_obj_pointer_get((Eo_Id)obj_id, __func__, __FILE__, __LINE__); \
   } while (0)

/**
 * @def EO_OBJ_POINTER_PROXY(obj_id, obj)
 * @brief Retrieves the internal _Eo_Object pointer, using provided context for error reporting.
 *
 * Similar to #EO_OBJ_POINTER, but uses `func_name`, `file`, and `line` variables
 * (expected to be in scope) for error reporting instead of `__func__`, `__FILE__`, `__LINE__`.
 * This is useful for proxy functions that resolve pointers on behalf of other functions.
 *
 * @param obj_id The Eo_Id to resolve.
 * @param obj The name of the `_Eo_Object*` variable to declare and assign.
 * @note `func_name`, `file`, and `line` must be defined in the calling scope.
 */
#define EO_OBJ_POINTER_PROXY(obj_id, obj) \
   _Eo_Object *obj; \
   do { \
      obj = _eo_obj_pointer_get((Eo_Id)obj_id, func_name, file, line); \
   } while (0)

/**
 * @def EO_OBJ_POINTER_RETURN_VAL(obj_id, obj, ret)
 * @brief Retrieves the internal _Eo_Object pointer and returns a value on failure.
 *
 * Declares `obj` and initializes it. If pointer resolution fails,
 * an error is logged and the current function returns `ret`.
 *
 * @param obj_id The Eo_Id to resolve.
 * @param obj The name of the `_Eo_Object*` variable to declare and assign.
 * @param ret The value to return if `obj_id` cannot be resolved.
 *
 * @par Example:
 * @code
 * int my_function_with_return(Eo *my_eo_id) {
 *   EO_OBJ_POINTER_RETURN_VAL(my_eo_id, obj_ptr, -1);
 *   // obj_ptr is guaranteed to be non-NULL here if the function hasn't returned.
 *   // Use obj_ptr
 *   return 0;
 * }
 * @endcode
 */
#define EO_OBJ_POINTER_RETURN_VAL(obj_id, obj, ret)  \
   _Eo_Object *obj; \
   do { \
      obj = _eo_obj_pointer_get((Eo_Id)obj_id, __func__, __FILE__, __LINE__); \
      if (!obj) return (ret); \
   } while (0)

/**
 * @def EO_OBJ_POINTER_RETURN_VAL_PROXY(obj_id, obj, ret)
 * @brief Retrieves _Eo_Object pointer, returns `ret` on failure, using provided context.
 *
 * Similar to #EO_OBJ_POINTER_RETURN_VAL, but uses `func_name`, `file`, and `line`
 * variables for error reporting.
 *
 * @param obj_id The Eo_Id to resolve.
 * @param obj The name of the `_Eo_Object*` variable to declare and assign.
 * @param ret The value to return if `obj_id` cannot be resolved.
 * @note `func_name`, `file`, and `line` must be defined in the calling scope.
 */
#define EO_OBJ_POINTER_RETURN_VAL_PROXY(obj_id, obj, ret) \
   _Eo_Object *obj; \
   do { \
      obj = _eo_obj_pointer_get((Eo_Id)obj_id, func_name, file, line); \
      if (!obj) return (ret); \
   } while (0)

/**
 * @def EO_OBJ_POINTER_RETURN(obj_id, obj)
 * @brief Retrieves the internal _Eo_Object pointer and returns (void) on failure.
 *
 * Declares `obj` and initializes it. If pointer resolution fails,
 * an error is logged and the current function returns (void).
 *
 * @param obj_id The Eo_Id to resolve.
 * @param obj The name of the `_Eo_Object*` variable to declare and assign.
 */
#define EO_OBJ_POINTER_RETURN(obj_id, obj) \
   _Eo_Object *obj; \
   do { \
      obj = _eo_obj_pointer_get((Eo_Id)obj_id, __func__, __FILE__, __LINE__); \
      if (!obj) return; \
   } while (0)

/**
 * @def EO_OBJ_POINTER_RETURN_PROXY(obj_id, obj)
 * @brief Retrieves _Eo_Object pointer, returns (void) on failure, using provided context.
 *
 * Similar to #EO_OBJ_POINTER_RETURN, but uses `func_name`, `file`, and `line`
 * variables for error reporting.
 *
 * @param obj_id The Eo_Id to resolve.
 * @param obj The name of the `_Eo_Object*` variable to declare and assign.
 * @note `func_name`, `file`, and `line` must be defined in the calling scope.
 */
#define EO_OBJ_POINTER_RETURN_PROXY(obj_id, obj) \
   _Eo_Object *obj; \
   do { \
      obj = _eo_obj_pointer_get((Eo_Id)obj_id, func_name, file, line); \
      if (!obj) return; \
   } while (0)

/**
 * @def EO_OBJ_POINTER_GOTO(obj_id, obj, label)
 * @brief Retrieves the internal _Eo_Object pointer and jumps to `label` on failure.
 *
 * Declares `obj` and initializes it. If pointer resolution fails,
 * an error is logged and execution jumps to `label`.
 *
 * @param obj_id The Eo_Id to resolve.
 * @param obj The name of the `_Eo_Object*` variable to declare and assign.
 * @param label The label to jump to on failure.
 */
#define EO_OBJ_POINTER_GOTO(obj_id, obj, label) \
   _Eo_Object *obj; \
   do { \
      obj = _eo_obj_pointer_get((Eo_Id)obj_id, __func__, __FILE__, __LINE__); \
      if (!obj) goto label; \
   } while (0)

/**
 * @def EO_OBJ_POINTER_GOTO_PROXY(obj_id, obj, label)
 * @brief Retrieves _Eo_Object pointer, jumps to `label` on failure, using provided context.
 *
 * Similar to #EO_OBJ_POINTER_GOTO, but uses `func_name`, `file`, and `line`
 * variables for error reporting.
 *
 * @param obj_id The Eo_Id to resolve.
 * @param obj The name of the `_Eo_Object*` variable to declare and assign.
 * @param label The label to jump to on failure.
 * @note `func_name`, `file`, and `line` must be defined in the calling scope.
 */
#define EO_OBJ_POINTER_GOTO_PROXY(obj_id, obj, label) \
   _Eo_Object *obj; \
   do { \
      obj = _eo_obj_pointer_get((Eo_Id)obj_id, func_name, file, line); \
      if (!obj) goto label; \
   } while (0)

/**
 * @def EO_CLASS_POINTER(klass_id, klass)
 * @brief Retrieves the internal _Efl_Class pointer from an Eo_Id (representing a class).
 *
 * Declares a local variable `klass` of type `_Efl_Class*` and initializes it
 * with the resolved class pointer for `klass_id`. If resolution fails, an error
 * is logged, but execution continues (klass will be NULL).
 *
 * @param klass_id The Eo_Id of the class to resolve.
 * @param klass The name of the `_Efl_Class*` variable to declare and assign.
 */
#define EO_CLASS_POINTER(klass_id, klass)   \
   _Efl_Class *klass; \
   do { \
      klass = _eo_class_pointer_get(klass_id); \
   } while (0)

/**
 * @def EO_CLASS_POINTER_PROXY(klass_id, klass)
 * @brief Retrieves _Efl_Class pointer, using provided context for error reporting.
 *
 * Similar to #EO_CLASS_POINTER. In current implementation, it behaves identically
 * as _eo_class_pointer_get does not take context parameters for error reporting.
 * Kept for consistency with object pointer macros.
 *
 * @param klass_id The Eo_Id of the class to resolve.
 * @param klass The name of the `_Efl_Class*` variable to declare and assign.
 */
#define EO_CLASS_POINTER_PROXY(klass_id, klass)   \
   _Efl_Class *klass; \
   do { \
      klass = _eo_class_pointer_get(klass_id); \
   } while (0)

/**
 * @def EO_CLASS_POINTER_RETURN_VAL(klass_id, klass, ret)
 * @brief Retrieves _Efl_Class pointer and returns a value on failure.
 *
 * Declares `klass` and initializes it. If class pointer resolution fails,
 * an error is logged and the current function returns `ret`.
 *
 * @param klass_id The Eo_Id of the class to resolve.
 * @param klass The name of the `_Efl_Class*` variable to declare and assign.
 * @param ret The value to return if `klass_id` cannot be resolved.
 */
#define EO_CLASS_POINTER_RETURN_VAL(klass_id, klass, ret) \
   _Efl_Class *klass; \
   do { \
      klass = _eo_class_pointer_get(klass_id); \
      if (!klass) { \
         _EO_POINTER_ERR(klass_id, "Class (%p) is an invalid ref.", klass_id); \
         return ret; \
      } \
   } while (0)

/**
 * @def EO_CLASS_POINTER_RETURN_VAL_PROXY(klass_id, klass, ret)
 * @brief Retrieves _Efl_Class pointer, returns `ret` on failure, using provided context.
 *
 * Similar to #EO_CLASS_POINTER_RETURN_VAL. In current implementation, the "proxy"
 * nature (using external func_name, file, line) is not fully utilized for class
 * pointer error reporting as _eo_class_pointer_get itself doesn't take these.
 * The error is logged using _EO_POINTER_ERR which uses current context.
 *
 * @param klass_id The Eo_Id of the class to resolve.
 * @param klass The name of the `_Efl_Class*` variable to declare and assign.
 * @param ret The value to return if `klass_id` cannot be resolved.
 */
#define EO_CLASS_POINTER_RETURN_VAL_PROXY(klass_id, klass, ret) \
   _Efl_Class *klass; \
   do { \
      klass = _eo_class_pointer_get(klass_id); \
      if (!klass) { \
         _EO_POINTER_ERR(klass_id, "Class (%p) is an invalid ref.", klass_id); \
         return ret; \
      } \
   } while (0)

/**
 * @def EO_CLASS_POINTER_RETURN(klass_id, klass)
 * @brief Retrieves _Efl_Class pointer and returns (void) on failure.
 *
 * Declares `klass` and initializes it. If class pointer resolution fails,
 * an error is logged and the current function returns (void).
 *
 * @param klass_id The Eo_Id of the class to resolve.
 * @param klass The name of the `_Efl_Class*` variable to declare and assign.
 */
#define EO_CLASS_POINTER_RETURN(klass_id, klass)   \
   _Efl_Class *klass; \
   do { \
      klass = _eo_class_pointer_get(klass_id); \
      if (!klass) { \
         _EO_POINTER_ERR(klass_id, "Class (%p) is an invalid ref.", klass_id); \
         return; \
      } \
   } while (0)

/**
 * @def EO_CLASS_POINTER_RETURN_PROXY(klass_id, klass)
 * @brief Retrieves _Efl_Class pointer, returns (void) on failure, using provided context.
 *
 * Similar to #EO_CLASS_POINTER_RETURN. The "proxy" aspect is limited as with
 * #EO_CLASS_POINTER_RETURN_VAL_PROXY.
 *
 * @param klass_id The Eo_Id of the class to resolve.
 * @param klass The name of the `_Efl_Class*` variable to declare and assign.
 */
#define EO_CLASS_POINTER_RETURN_PROXY(klass_id, klass) \
   _Efl_Class *klass; \
   do { \
      klass = _eo_class_pointer_get(klass_id); \
      if (!klass) { \
         _EO_POINTER_ERR(klass_id, "Class (%p) is an invalid ref.", klass_id); \
         return; \
      } \
   } while (0)

/**
 * @def EO_CLASS_POINTER_GOTO(klass_id, klass, label)
 * @brief Retrieves _Efl_Class pointer and jumps to `label` on failure.
 *
 * Declares `klass` and initializes it. If class pointer resolution fails,
 * an error is logged and execution jumps to `label`.
 *
 * @param klass_id The Eo_Id of the class to resolve.
 * @param klass The name of the `_Efl_Class*` variable to declare and assign.
 * @param label The label to jump to on failure.
 */
#define EO_CLASS_POINTER_GOTO(klass_id, klass, label) \
   _Efl_Class *klass; \
   do { \
      klass = _eo_class_pointer_get(klass_id); \
      if (!klass) goto label; \
   } while (0)

/**
 * @def EO_CLASS_POINTER_GOTO_PROXY(klass_id, klass, label)
 * @brief Retrieves _Efl_Class pointer, jumps to `label` on failure, using provided context.
 *
 * Similar to #EO_CLASS_POINTER_GOTO. The "proxy" aspect is limited.
 *
 * @param klass_id The Eo_Id of the class to resolve.
 * @param klass The name of the `_Efl_Class*` variable to declare and assign.
 * @param label The label to jump to on failure.
 */
#define EO_CLASS_POINTER_GOTO_PROXY(klass_id, klass, label) \
   _Efl_Class *klass; \
   do { \
      klass = _eo_class_pointer_get(klass_id); \
      if (!klass) goto label; \
   } while (0)

/**
 * @def EO_OBJ_DONE(obj_id)
 * @brief Signals that pointer resolution for a shared object is complete.
 *
 * This macro must be called after accessing a shared object obtained via
 * #_eo_obj_pointer_get (or macros using it) to release associated locks.
 * It is a wrapper around _eo_obj_pointer_done().
 *
 * @param obj_id The Eo_Id of the shared object.
 */
#define EO_OBJ_DONE(obj_id) \
   _eo_obj_pointer_done((Eo_Id)obj_id)

#ifdef EFL_DEBUG
/**
 * @brief Debug function to print table data.
 * (Only available in EFL_DEBUG builds)
 * @param tdata Pointer to the table data to print.
 */
static inline void _eo_print(Eo_Id_Table_Data *tdata);
#endif

/** @brief Thread-local storage for Eo ID table data. */
extern Eina_TLS _eo_table_data;

#include "eo_ptr_indirection.x" /* For inline function implementations */

/** @brief Pointer to shared Eo ID data structure. */
extern Eo_Id_Data *_eo_table_data_shared;
/** @brief Pointer to shared Eo ID table data structure. */
extern Eo_Id_Table_Data *_eo_table_data_shared_data;

#endif

