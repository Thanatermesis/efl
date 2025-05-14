#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>

#include "ecore_private.h"

#define MY_CLASS EFL_THREADIO_CLASS

/**
 * @brief Private data structure for Efl_ThreadIO.
 *
 * This structure holds the input and output data pointers for thread communication.
 */
typedef struct _Efl_ThreadIO_Data Efl_ThreadIO_Data;

struct _Efl_ThreadIO_Data
{
   void *indata;  /**< Pointer to the input data. This data is typically set by the main thread and read by the worker thread. */
   void *outdata; /**< Pointer to the output data. This data is typically set by the worker thread and read by the main thread. */
};

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

/**
 * @brief Sets the input data for the thread operation.
 *
 * @param[in] obj The Efl_ThreadIO object.
 * @param[in] pd The private data for the Efl_ThreadIO object.
 * @param[in] data A pointer to the data to be used as input. The type and structure
 *                 of this data are defined by the user of this class.
 */
EOLIAN static void
_efl_threadio_indata_set(Eo *obj EINA_UNUSED, Efl_ThreadIO_Data *pd, void *data)
{
   pd->indata = data;
}

/**
 * @brief Gets the input data for the thread operation.
 *
 * @param[in] obj The Efl_ThreadIO object.
 * @param[in] pd The private data for the Efl_ThreadIO object.
 * @return A pointer to the input data.
 */
EOLIAN static void *
_efl_threadio_indata_get(const Eo *obj EINA_UNUSED, Efl_ThreadIO_Data *pd)
{
   return pd->indata;
}

/**
 * @brief Sets the output data from the thread operation.
 *
 * @param[in] obj The Efl_ThreadIO object.
 * @param[in] pd The private data for the Efl_ThreadIO object.
 * @param[in] data A pointer to the data that is the result of the thread's operation.
 *                 The type and structure of this data are defined by the user of this class.
 */
EOLIAN static void
_efl_threadio_outdata_set(Eo *obj EINA_UNUSED, Efl_ThreadIO_Data *pd, void *data)
{
   pd->outdata = data;
}

/**
 * @brief Gets the output data from the thread operation.
 *
 * @param[in] obj The Efl_ThreadIO object.
 * @param[in] pd The private data for the Efl_ThreadIO object.
 * @return A pointer to the output data.
 */
EOLIAN static void *
_efl_threadio_outdata_get(const Eo *obj EINA_UNUSED, Efl_ThreadIO_Data *pd)
{
   return pd->outdata;
}

//////////////////////////////////////////////////////////////////////////

#include "efl_threadio.eo.c"
