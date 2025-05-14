/* EIO - EFL data type library
 * Copyright (C) 2010 Enlightenment Developers:
 *           Cedric Bail <cedric.bail@free.fr>
 *           Vincent "caro" Torri  <vtorri at univ-evry dot fr>
 *           Stephen "okra" Houston <UnixTitan@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#include "eio_private.h"
#include "Eio.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

/**
 * @cond LOCAL
 */

/**
 * @brief Worker thread function to open an Eet file.
 *
 * This function is executed in a separate thread to perform the blocking
 * eet_open() call. It sets the result on the Eio_Eet_Open structure
 * and signals an error if the open operation fails.
 *
 * @param data Pointer to the Eio_Eet_Open structure.
 * @param thread Pointer to the Ecore_Thread executing this job.
 */
static void
_eio_eet_open_job(void *data, Ecore_Thread *thread)
{
   Eio_Eet_Open *eet = data;

   eet->result = eet_open(eet->filename, eet->mode);
   if (!eet->result) eio_file_thread_error(&eet->common, thread);
}

/**
 * @brief Frees resources associated with an Eio_Eet_Open operation.
 *
 * This function is called to clean up an Eio_Eet_Open structure,
 * releasing the filename stringshare and the Eio_File base structure.
 *
 * @param eet Pointer to the Eio_Eet_Open structure to free.
 */
static void
_eio_eet_open_free(Eio_Eet_Open *eet)
{
   if (eet->filename) eina_stringshare_del(eet->filename);
   eio_file_free((Eio_File *)eet);
}

/**
 * @brief Main loop callback executed after a successful Eet file open.
 *
 * This function is called in the main Ecore loop when the _eio_eet_open_job
 * completes successfully. It invokes the user-provided eet_cb callback
 * and then frees the Eio_Eet_Open structure.
 *
 * @param data Pointer to the Eio_Eet_Open structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_open_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Open *eet = data;

   eet->eet_cb((void*) eet->common.data, &eet->common, eet->result);
   _eio_eet_open_free(eet);
}

/**
 * @brief Main loop callback executed if an Eet file open is cancelled or fails.
 *
 * This function is called in the main Ecore loop if the _eio_eet_open_job
 * is cancelled or encounters an error. It calls the generic eio_file_error()
 * to notify the user via the error_cb and then frees the Eio_Eet_Open structure.
 *
 * @param data Pointer to the Eio_Eet_Open structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_open_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Open *eet = data;

   eio_file_error(&eet->common);
   _eio_eet_open_free(eet);
}

/**
 * @brief Worker thread function to close an Eet file.
 *
 * This function is executed in a separate thread to perform the blocking
 * eet_close() call. It sets the error status on the Eio_Eet_Simple
 * structure if the close operation fails.
 *
 * @param data Pointer to the Eio_Eet_Simple structure.
 * @param thread Pointer to the Ecore_Thread executing this job.
 */
static void
_eio_eet_close_job(void *data, Ecore_Thread *thread)
{
   Eio_Eet_Simple *eet = data;

   eet->error = eet_close(eet->ef);
   if (eet->error != EET_ERROR_NONE) eio_file_thread_error(&eet->common, thread);
}

/**
 * @brief Worker thread function to sync (flush) an Eet file.
 *
 * This function is executed in a separate thread to perform the blocking
 * eet_sync() call. It sets the error status on the Eio_Eet_Simple
 * structure if the sync operation fails.
 *
 * @param data Pointer to the Eio_Eet_Simple structure.
 * @param thread Pointer to the Ecore_Thread executing this job.
 */
static void
_eio_eet_sync_job(void *data, Ecore_Thread *thread)
{
   Eio_Eet_Simple *eet = data;

   eet->error = eet_sync(eet->ef);
   if (eet->error != EET_ERROR_NONE) eio_file_thread_error(&eet->common, thread);
}

/**
 * @brief Main loop callback for successful simple Eet operations (close, sync).
 *
 * This function is called in the main Ecore loop when a simple Eet operation
 * (like close or sync) completes successfully. It invokes the user-provided
 * done_cb callback and then frees the Eio_Eet_Simple structure.
 *
 * @param data Pointer to the Eio_Eet_Simple structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_simple_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Simple *eet = data;

   eet->common.done_cb((void*) eet->common.data, &eet->common);
   eio_file_free((Eio_File *)eet);
}

/**
 * @brief Main loop callback if a simple Eet operation (close, sync) is cancelled or fails.
 *
 * This function is called in the main Ecore loop if a simple Eet operation
 * (like close or sync) is cancelled or encounters an error. It invokes the
 * user-provided error_cb with the Eet_Error code and then frees the
 * Eio_Eet_Simple structure.
 *
 * @param data Pointer to the Eio_Eet_Simple structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_simple_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Simple *eet = data;

   eet->error_cb((void*) eet->common.data, &eet->common, eet->error);
   eio_file_free((Eio_File *)eet);
}

/**
 * @brief Worker thread function to write data to an Eet file using a cipher.
 *
 * This function is executed in a separate thread to perform the blocking
 * eet_data_write_cipher() call. It writes structured data, optionally
 * compressed and encrypted, to the Eet file. Sets the result (bytes written)
 * on the Eio_Eet_Write structure and signals an error if the write fails.
 *
 * @param data Pointer to the Eio_Eet_Write structure.
 * @param thread Pointer to the Ecore_Thread executing this job.
 */
static void
_eio_eet_data_write_cipher_job(void *data, Ecore_Thread *thread)
{
   Eio_Eet_Write *ew = data;

   ew->result = eet_data_write_cipher(ew->ef, ew->edd,
                                      ew->name, ew->cipher_key,
                                      ew->write_data,
                                      ew->compress);
   if (ew->result == 0) eio_file_thread_error(&ew->common, thread);
}

/**
 * @brief Frees resources associated with an Eio_Eet_Write operation (ciphered).
 *
 * This function is called to clean up an Eio_Eet_Write structure,
 * releasing stringshares for the entry name and cipher key, and
 * the Eio_File base structure.
 * It is used for both eet_data_write_cipher and eet_write_cipher operations.
 *
 * @param ew Pointer to the Eio_Eet_Write structure to free.
 */
static void
_eio_eet_write_cipher_free(Eio_Eet_Write *ew)
{
   eina_stringshare_del(ew->name);
   eina_stringshare_del(ew->cipher_key);
   eio_file_free((Eio_File *)ew);
}

/**
 * @brief Main loop callback after successful Eet data write with cipher.
 *
 * This function is called in the main Ecore loop when the
 * _eio_eet_data_write_cipher_job completes successfully. It invokes the
 * user-provided done_cb callback with the number of bytes written
 * and then frees the Eio_Eet_Write structure.
 *
 * @param data Pointer to the Eio_Eet_Write structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_data_write_cipher_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Write *ew = data;

   ew->done_cb((void*) ew->common.data, &ew->common, ew->result);
   _eio_eet_write_cipher_free(ew);
}

/**
 * @brief Main loop callback if Eet data write with cipher is cancelled or fails.
 *
 * This function is called in the main Ecore loop if the
 * _eio_eet_data_write_cipher_job is cancelled or encounters an error.
 * It calls the generic eio_file_error() to notify the user via the error_cb
 * and then frees the Eio_Eet_Write structure.
 *
 * @param data Pointer to the Eio_Eet_Write structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_data_write_cipher_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Write *ew = data;

   eio_file_error(&ew->common);
   _eio_eet_write_cipher_free(ew);
}

/**
 * @brief Worker thread function to write image data to an Eet file.
 *
 * This function is executed in a separate thread to perform the blocking
 * eet_data_image_write_cipher() call. It handles image-specific parameters
 * like dimensions, alpha, compression, quality, and lossiness.
 * Sets the result (success/failure indicator) on the Eio_Eet_Image_Write
 * structure and signals an error if the write fails.
 *
 * @param data Pointer to the Eio_Eet_Image_Write structure.
 * @param thread Pointer to the Ecore_Thread executing this job.
 */
static void
_eio_eet_image_write_job(void *data, Ecore_Thread *thread)
{
   Eio_Eet_Image_Write *eiw = data;

   eiw->result = eet_data_image_write_cipher(eiw->ef, eiw->name, eiw->cipher_key,
                                             eiw->write_data,
                                             eiw->w,
                                             eiw->h,
                                             eiw->alpha,
                                             eiw->compress,
                                             eiw->quality,
                                             eiw->lossy);
   if (!eiw->result) eio_file_thread_error(&eiw->common, thread);
}

/**
 * @brief Frees resources associated with an Eio_Eet_Image_Write operation.
 *
 * This function is called to clean up an Eio_Eet_Image_Write structure,
 * releasing stringshares for the entry name and cipher key, and
 * the Eio_File base structure.
 *
 * @param eiw Pointer to the Eio_Eet_Image_Write structure to free.
 */
static void
_eio_eet_image_write_free(Eio_Eet_Image_Write *eiw)
{
   eina_stringshare_del(eiw->name);
   eina_stringshare_del(eiw->cipher_key);
   eio_file_free(&eiw->common);
}

/**
 * @brief Main loop callback after successful Eet image write.
 *
 * This function is called in the main Ecore loop when the
 * _eio_eet_image_write_job completes successfully. It invokes the
 * user-provided done_cb callback with the result of the operation
 * and then frees the Eio_Eet_Image_Write structure.
 *
 * @param data Pointer to the Eio_Eet_Image_Write structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_image_write_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Image_Write *eiw = data;

   eiw->done_cb((void*) eiw->common.data, &eiw->common, eiw->result);
   _eio_eet_image_write_free(eiw);
}

/**
 * @brief Main loop callback if Eet image write is cancelled or fails.
 *
 * This function is called in the main Ecore loop if the
 * _eio_eet_image_write_job is cancelled or encounters an error.
 * It calls the generic eio_file_error() to notify the user via the error_cb
 * and then frees the Eio_Eet_Image_Write structure.
 *
 * @param data Pointer to the Eio_Eet_Image_Write structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_image_write_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Image_Write *eiw = data;

   eio_file_error(&eiw->common);
   _eio_eet_image_write_free(eiw);
}

/**
 * @brief Worker thread function to write raw data to an Eet file using a cipher.
 *
 * This function is executed in a separate thread to perform the blocking
 * eet_write_cipher() call. It writes a block of raw data, optionally
 * compressed and encrypted. Sets the result (success/failure indicator)
 * on the Eio_Eet_Write structure and signals an error if the write fails.
 *
 * @param data Pointer to the Eio_Eet_Write structure.
 * @param thread Pointer to the Ecore_Thread executing this job.
 */
static void
_eio_eet_write_job(void *data, Ecore_Thread *thread)
{
   Eio_Eet_Write *ew = data;

   ew->result = eet_write_cipher(ew->ef,
                                 ew->name, ew->write_data,
                                 ew->size, ew->compress,
                                 ew->cipher_key);
   if (!ew->result) eio_file_thread_error(&ew->common, thread);
}

/**
 * @brief Main loop callback after successful generic Eet write.
 *
 * This function is called in the main Ecore loop when the
 * _eio_eet_write_job completes successfully. It invokes the
 * user-provided done_cb callback with the result of the operation
 * and then frees the Eio_Eet_Write structure using _eio_eet_write_cipher_free.
 *
 * @param data Pointer to the Eio_Eet_Write structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_write_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Write *ew = data;

   ew->done_cb((void*) ew->common.data, &ew->common, ew->result);
   _eio_eet_write_cipher_free(ew);
}

/**
 * @brief Main loop callback if generic Eet write is cancelled or fails.
 *
 * This function is called in the main Ecore loop if the
 * _eio_eet_write_job is cancelled or encounters an error.
 * It calls the generic eio_file_error() to notify the user via the error_cb
 * and then frees the Eio_Eet_Write structure using _eio_eet_write_cipher_free.
 *
 * @param data Pointer to the Eio_Eet_Write structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_write_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Write *ew = data;

   eio_file_error(&ew->common);
   _eio_eet_write_cipher_free(ew);
}

/**
 * @brief Worker thread function to read structured data from an Eet file using a cipher.
 *
 * This function is executed in a separate thread to perform the blocking
 * eet_data_read_cipher() call. It reads structured data, potentially
 * decrypting it if a cipher_key is provided. Sets the result (pointer to
 * the read data) on the Eio_Eet_Read structure and signals an error if
 * the read fails.
 *
 * @param data Pointer to the Eio_Eet_Read structure.
 * @param thread Pointer to the Ecore_Thread executing this job.
 */
static void
_eio_eet_data_read_cipher_job(void *data, Ecore_Thread *thread)
{
   Eio_Eet_Read *er = data;

   er->result = eet_data_read_cipher(er->ef, er->edd,
                                     er->name, er->cipher_key);
   if (!er->result) eio_file_thread_error(&er->common, thread);
}

/**
 * @brief Frees resources associated with an Eio_Eet_Read operation.
 *
 * This function is called to clean up an Eio_Eet_Read structure,
 * releasing stringshares for the entry name and cipher key (if any),
 * and the Eio_File base structure.
 * It is used by various Eet read operations.
 *
 * @param er Pointer to the Eio_Eet_Read structure to free.
 */
static void
_eio_eet_read_free(Eio_Eet_Read *er)
{
   eina_stringshare_del(er->name);
   eina_stringshare_del(er->cipher_key);
   eio_file_free(&er->common);
}

/**
 * @brief Main loop callback after successful Eet structured data read with cipher.
 *
 * This function is called in the main Ecore loop when the
 * _eio_eet_data_read_cipher_job completes successfully. It invokes the
 * user-provided done_cb.eread callback with the read data structure
 * and then frees the Eio_Eet_Read structure.
 *
 * @param data Pointer to the Eio_Eet_Read structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_data_read_cipher_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Read *er = data;

   er->done_cb.eread((void*) er->common.data, &er->common, er->result);
   _eio_eet_read_free(er);
}

/**
 * @brief Main loop callback if Eet structured data read with cipher is cancelled or fails.
 *
 * This function is called in the main Ecore loop if the
 * _eio_eet_data_read_cipher_job is cancelled or encounters an error.
 * It calls the generic eio_file_error() to notify the user via the error_cb
 * and then frees the Eio_Eet_Read structure.
 *
 * @param data Pointer to the Eio_Eet_Read structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_data_read_cipher_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Read *er = data;

   eio_file_error(&er->common);
   _eio_eet_read_free(er);
}

/**
 * @brief Worker thread function to read raw data directly from an Eet file.
 *
 * This function is executed in a separate thread to perform the blocking
 * eet_read_direct() call. It reads a raw block of data for a given entry
 * name and retrieves its size. Sets the result (pointer to the read data)
 * and size on the Eio_Eet_Read structure. Signals an error if the read fails.
 *
 * @param data Pointer to the Eio_Eet_Read structure.
 * @param thread Pointer to the Ecore_Thread executing this job.
 */
static void
_eio_eet_read_direct_job(void *data, Ecore_Thread *thread)
{
   Eio_Eet_Read *er = data;

   er->result = (void*) eet_read_direct(er->ef, er->name, &er->size);
   if (!er->result) eio_file_thread_error(&er->common, thread);
}

/**
 * @brief Main loop callback after successful Eet direct read.
 *
 * This function is called in the main Ecore loop when the
 * _eio_eet_read_direct_job completes successfully. It invokes the
 * user-provided done_cb.data callback with the read data buffer and its size,
 * then frees the Eio_Eet_Read structure.
 *
 * @param data Pointer to the Eio_Eet_Read structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_read_direct_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Read *er = data;

   er->done_cb.data((void*) er->common.data, &er->common,
                    er->result, er->size);
   _eio_eet_read_free(er);
}

/**
 * @brief Generic main loop callback if any Eet read operation is cancelled or fails.
 *
 * This function is called in the main Ecore loop if an Eet read operation
 * (direct, ciphered data, or ciphered raw) is cancelled or encounters an error.
 * It calls the generic eio_file_error() to notify the user via the error_cb
 * and then frees the Eio_Eet_Read structure.
 *
 * @param data Pointer to the Eio_Eet_Read structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_read_cancel(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Read *er = data;

   eio_file_error(&er->common);
   _eio_eet_read_free(er);
}

/**
 * @brief Worker thread function to read raw data from an Eet file using a cipher.
 *
 * This function is executed in a separate thread to perform the blocking
 * eet_read_cipher() call. It reads a raw block of data, potentially
 * decrypting it if a cipher_key is provided, and retrieves its size.
 * Sets the result (pointer to the read data) and size on the Eio_Eet_Read
 * structure. Signals an error if the read fails.
 *
 * @param data Pointer to the Eio_Eet_Read structure.
 * @param thread Pointer to the Ecore_Thread executing this job.
 */
static void
_eio_eet_read_cipher_job(void *data, Ecore_Thread *thread)
{
   Eio_Eet_Read *er = data;

   er->result = (void*) eet_read_cipher(er->ef, er->name,
                                        &er->size, er->cipher_key);
   if (!er->result) eio_file_thread_error(&er->common, thread);
}

/**
 * @brief Main loop callback after successful Eet raw data read with cipher.
 *
 * This function is called in the main Ecore loop when the
 * _eio_eet_read_cipher_job completes successfully. It invokes the
 * user-provided done_cb.read callback with the read data buffer and its size,
 * then frees the Eio_Eet_Read structure.
 *
 * @param data Pointer to the Eio_Eet_Read structure.
 * @param thread Pointer to the Ecore_Thread (unused).
 */
static void
_eio_eet_read_cipher_end(void *data, Ecore_Thread *thread EINA_UNUSED)
{
   Eio_Eet_Read *er = data;

   er->done_cb.read((void*) er->common.data, &er->common,
                    er->result, er->size);
   _eio_eet_read_free(er);
}

/**
 * @endcond
 */

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/


/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @brief Asynchronously open an Eet file.
 * @param filename The path to the Eet file.
 * @param mode The mode to open the file in (EET_FILE_MODE_READ, EET_FILE_MODE_WRITE, etc.).
 * @param eet_cb Callback function invoked upon successful completion, providing the Eet_File handle.
 * @param error_cb Callback function invoked if an error occurs.
 * @param data Custom data pointer passed to the callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on immediate failure.
 *
 * This function schedules an asynchronous operation to open an Eet file.
 * The result of the operation (either the Eet_File handle or an error)
 * will be delivered through the provided callbacks.
 */
EIO_API Eio_File *
eio_eet_open(const char *filename,
             Eet_File_Mode mode,
	     Eio_Eet_Open_Cb eet_cb,
	     Eio_Error_Cb error_cb,
	     const void *data)
{
   Eio_Eet_Open *eet;

   EINA_SAFETY_ON_NULL_RETURN_VAL(filename, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(eet_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   eet = eio_common_alloc(sizeof(Eio_Eet_Open));
   EINA_SAFETY_ON_NULL_RETURN_VAL(eet, NULL);

   eet->eet_cb = eet_cb;
   eet->filename = eina_stringshare_add(filename);
   eet->mode = mode;
   eet->result = NULL;

   if (!eio_file_set(&eet->common,
                     NULL,
                     error_cb,
                     data,
                     _eio_eet_open_job,
                     _eio_eet_open_end,
                     _eio_eet_open_cancel))
     return NULL;
   return &eet->common;
}

/**
 * @brief Asynchronously close an Eet file.
 * @param ef The Eet_File handle to close.
 * @param done_cb Callback function invoked upon successful completion.
 * @param error_cb Callback function invoked if an error occurs, providing an Eet_Error code.
 * @param data Custom data pointer passed to the callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on immediate failure.
 *
 * This function schedules an asynchronous operation to close an Eet file.
 * The eet_close() function can block, so this avoids stalling the main loop.
 */
EIO_API Eio_File *
eio_eet_close(Eet_File *ef,
	      Eio_Done_Cb done_cb,
	      Eio_Eet_Error_Cb error_cb,
	      const void *data)
{
   Eio_Eet_Simple *eet;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ef, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   eet = eio_common_alloc(sizeof(Eio_Eet_Simple));
   EINA_SAFETY_ON_NULL_RETURN_VAL(eet, NULL);

   eet->ef = ef;
   eet->error_cb = error_cb;
   eet->error = EET_ERROR_NONE;

   if (!eio_file_set(&eet->common,
                     done_cb,
                     NULL,
                     data,
                     _eio_eet_close_job,
                     _eio_eet_simple_end,
                     _eio_eet_simple_cancel))
     return NULL;
   return &eet->common;
}

/**
 * @brief Asynchronously flush (sync) an Eet file to disk.
 * @param ef The Eet_File handle to flush.
 * @param done_cb Callback function invoked upon successful completion.
 * @param error_cb Callback function invoked if an error occurs, providing an Eet_Error code.
 * @param data Custom data pointer passed to the callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on immediate failure.
 *
 * This function schedules an asynchronous operation to flush any pending writes
 * for the Eet file to the storage medium. This is equivalent to eio_eet_sync().
 */
EIO_API Eio_File *
eio_eet_flush(Eet_File *ef,
	      Eio_Done_Cb done_cb,
	      Eio_Eet_Error_Cb error_cb,
	      const void *data)
{
   Eio_Eet_Simple *eet;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ef, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   eet = eio_common_alloc(sizeof(Eio_Eet_Simple));
   EINA_SAFETY_ON_NULL_RETURN_VAL(eet, NULL);

   eet->ef = ef;
   eet->error_cb = error_cb;
   eet->error = EET_ERROR_NONE;

   if (!eio_file_set(&eet->common,
                     done_cb,
                     NULL,
                     data,
                     _eio_eet_sync_job,
                     _eio_eet_simple_end,
                     _eio_eet_simple_cancel))
     return NULL;
   return &eet->common;
}

/**
 * @brief Asynchronously sync (flush) an Eet file to disk.
 * @param ef The Eet_File handle to sync.
 * @param done_cb Callback function invoked upon successful completion.
 * @param error_cb Callback function invoked if an error occurs, providing an Eet_Error code.
 * @param data Custom data pointer passed to the callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on immediate failure.
 *
 * This function is an alias for eio_eet_flush().
 */
EIO_API Eio_File *
eio_eet_sync(Eet_File *ef,
             Eio_Done_Cb done_cb,
             Eio_Eet_Error_Cb error_cb,
             const void *data)
{
   return eio_eet_flush(ef, done_cb, error_cb, data);
}

/**
 * @brief Asynchronously write structured data to an Eet file with optional encryption.
 * @param ef The Eet_File handle.
 * @param edd The Eet_Data_Descriptor describing the structure of @p write_data.
 * @param name The name of the entry to write in the Eet file.
 * @param cipher_key Optional key for encryption. If @c NULL, no encryption is used.
 * @param write_data Pointer to the data structure to write.
 * @param compress Non-zero to compress the data, 0 otherwise.
 * @param done_cb Callback function invoked upon successful completion, providing the number of bytes written.
 * @param error_cb Callback function invoked if an error occurs.
 * @param user_data Custom data pointer passed to the callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on immediate failure.
 */
EIO_API Eio_File *
eio_eet_data_write_cipher(Eet_File *ef,
			  Eet_Data_Descriptor *edd,
			  const char *name,
			  const char *cipher_key,
			  void *write_data,
			  int compress,
			  Eio_Done_Int_Cb done_cb,
			  Eio_Error_Cb error_cb,
			  const void *user_data)
{
   Eio_Eet_Write *ew;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ef, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(edd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   ew = eio_common_alloc(sizeof(Eio_Eet_Write));
   EINA_SAFETY_ON_NULL_RETURN_VAL(ew, NULL);

   ew->ef = ef;
   ew->edd = edd;
   ew->name = eina_stringshare_add(name);
   ew->cipher_key = eina_stringshare_add(cipher_key);
   ew->write_data = write_data;
   ew->compress = compress;
   ew->done_cb = done_cb;
   ew->result = 0;

   if (!eio_file_set(&ew->common,
                     NULL,
                     error_cb,
                     user_data,
                     _eio_eet_data_write_cipher_job,
                     _eio_eet_data_write_cipher_end,
                     _eio_eet_data_write_cipher_cancel))
     return NULL;
   return &ew->common;
}

/**
 * @brief Asynchronously read structured data from an Eet file with optional decryption.
 * @param ef The Eet_File handle.
 * @param edd The Eet_Data_Descriptor describing the structure of the data to read.
 * @param name The name of the entry to read from the Eet file.
 * @param cipher_key Optional key for decryption. If @c NULL, no decryption is attempted.
 *                   Must match the key used during writing if data was encrypted.
 * @param done_cb Callback function invoked upon successful completion, providing the read data structure.
 *                The returned data structure should be freed by the caller using the appropriate
 *                Eet functions (e.g., based on the Eet_Data_Descriptor).
 * @param error_cb Callback function invoked if an error occurs.
 * @param data Custom data pointer passed to the callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on immediate failure.
 */
EIO_API Eio_File *
eio_eet_data_read_cipher(Eet_File *ef,
			 Eet_Data_Descriptor *edd,
			 const char *name,
			 const char *cipher_key,
			 Eio_Done_ERead_Cb done_cb,
			 Eio_Error_Cb error_cb,
			 const void *data)
{
   Eio_Eet_Read *er;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ef, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(edd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   er = eio_common_alloc(sizeof(Eio_Eet_Read));
   EINA_SAFETY_ON_NULL_RETURN_VAL(er, NULL);

   er->ef = ef;
   er->edd = edd;
   er->name = eina_stringshare_add(name);
   er->cipher_key = eina_stringshare_add(cipher_key);
   er->done_cb.eread = done_cb;

   if (!eio_file_set(&er->common,
                     NULL,
                     error_cb,
                     data,
                     _eio_eet_data_read_cipher_job,
                     _eio_eet_data_read_cipher_end,
                     _eio_eet_data_read_cipher_cancel))
     return NULL;

   return &er->common;
}

/**
 * @brief Asynchronously write image data to an Eet file with optional encryption.
 * @param ef The Eet_File handle.
 * @param name The name of the image entry to write in the Eet file.
 * @param cipher_key Optional key for encryption. If @c NULL, no encryption is used.
 * @param write_data Pointer to the raw image pixel data (ARGB format).
 * @param w Width of the image.
 * @param h Height of the image.
 * @param alpha Non-zero if the image has an alpha channel, 0 otherwise.
 * @param compress Compression level (0-9 for ZLib, or Eet specific image compression flags).
 * @param quality Quality level for lossy compression (e.g., JPEG quality, 1-100).
 * @param lossy Type of lossy encoding (e.g., EET_IMAGE_LOSSLESS, EET_IMAGE_JPEG, EET_IMAGE_ETC1).
 * @param done_cb Callback function invoked upon successful completion, providing a success indicator (bytes written or 1 for success).
 * @param error_cb Callback function invoked if an error occurs.
 * @param user_data Custom data pointer passed to the callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on immediate failure.
 */
EIO_API Eio_File *
eio_eet_data_image_write_cipher(Eet_File *ef,
				const char *name,
				const char *cipher_key,
				void *write_data,
				unsigned int w,
				unsigned int h,
				int alpha,
				int compress,
				int quality,
				int lossy,
				Eio_Done_Int_Cb done_cb,
				Eio_Error_Cb error_cb,
				const void *user_data)
{
   Eio_Eet_Image_Write *eiw;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ef, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   eiw = eio_common_alloc(sizeof(Eio_Eet_Image_Write));
   EINA_SAFETY_ON_NULL_RETURN_VAL(eiw, NULL);

   eiw->ef = ef;
   eiw->name = eina_stringshare_add(name);
   eiw->cipher_key = eina_stringshare_add(cipher_key);
   eiw->write_data = write_data;
   eiw->w = w;
   eiw->h = h;
   eiw->alpha = alpha;
   eiw->compress = compress;
   eiw->quality = quality;
   eiw->lossy = lossy;
   eiw->done_cb = done_cb;
   eiw->result = 0;

   if (!eio_file_set(&eiw->common,
                     NULL,
                     error_cb,
                     user_data,
                     _eio_eet_image_write_job,
                     _eio_eet_image_write_end,
                     _eio_eet_image_write_cancel))
     return NULL;
   return &eiw->common;
}

/**
 * @brief Asynchronously read raw data directly from an Eet file entry.
 * @param ef The Eet_File handle.
 * @param name The name of the entry to read.
 * @param done_cb Callback function invoked upon successful completion, providing the raw data buffer and its size.
 *                The returned data buffer should be freed by the caller using `free()`.
 * @param error_cb Callback function invoked if an error occurs.
 * @param data Custom data pointer passed to the callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on immediate failure.
 *
 * This function reads an Eet entry as a raw block of bytes, without using
 * an Eet_Data_Descriptor. It's suitable for data not stored via Eet's
 * data descriptor mechanism or when the structure is handled externally.
 */
EIO_API Eio_File *
eio_eet_read_direct(Eet_File *ef,
		    const char *name,
		    Eio_Done_Data_Cb done_cb,
		    Eio_Error_Cb error_cb,
		    const void *data)
{
   Eio_Eet_Read *er;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ef, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   er = eio_common_alloc(sizeof(Eio_Eet_Read));
   EINA_SAFETY_ON_NULL_RETURN_VAL(er, NULL);

   er->ef = ef;
   er->name = eina_stringshare_add(name);
   er->cipher_key = NULL;
   er->done_cb.data = done_cb;
   er->result = NULL;

   if (!eio_file_set(&er->common,
                     NULL,
                     error_cb,
                     data,
                     _eio_eet_read_direct_job,
                     _eio_eet_read_direct_end,
                     _eio_eet_read_cancel))
     return NULL;

   return &er->common;
}

/**
 * @brief Asynchronously read raw data from an Eet file entry with optional decryption.
 * @param ef The Eet_File handle.
 * @param name The name of the entry to read.
 * @param cipher_key Optional key for decryption. If @c NULL, no decryption is attempted.
 *                   Must match the key used during writing if data was encrypted.
 * @param done_cb Callback function invoked upon successful completion, providing the raw data buffer and its size.
 *                The returned data buffer should be freed by the caller using `free()`.
 * @param error_cb Callback function invoked if an error occurs.
 * @param data Custom data pointer passed to the callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on immediate failure.
 *
 * This function reads an Eet entry as a raw block of bytes, potentially
 * decrypting it.
 */
EIO_API Eio_File *
eio_eet_read_cipher(Eet_File *ef,
		    const char *name,
		    const char *cipher_key,
		    Eio_Done_Read_Cb done_cb,
		    Eio_Error_Cb error_cb,
		    const void *data)
{
   Eio_Eet_Read *er;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ef, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   er = eio_common_alloc(sizeof(Eio_Eet_Read));
   EINA_SAFETY_ON_NULL_RETURN_VAL(er, NULL);

   er->ef = ef;
   er->name = eina_stringshare_add(name);
   er->cipher_key = eina_stringshare_add(cipher_key);
   er->done_cb.read = done_cb;
   er->result = NULL;

   if (!eio_file_set(&er->common,
                     NULL,
                     error_cb,
                     data,
                     _eio_eet_read_cipher_job,
                     _eio_eet_read_cipher_end,
                     _eio_eet_read_cancel))
     return NULL;
   return &er->common;
}

/**
 * @brief Asynchronously write raw data to an Eet file entry with optional encryption.
 * @param ef The Eet_File handle.
 * @param name The name of the entry to write.
 * @param write_data Pointer to the raw data buffer to write.
 * @param size The size of the data in @p write_data.
 * @param compress Non-zero to compress the data, 0 otherwise.
 * @param cipher_key Optional key for encryption. If @c NULL, no encryption is used.
 * @param done_cb Callback function invoked upon successful completion, providing a success indicator (bytes written or 1 for success).
 * @param error_cb Callback function invoked if an error occurs.
 * @param user_data Custom data pointer passed to the callbacks.
 * @return An Eio_File handle for this operation, or @c NULL on immediate failure.
 *
 * This function writes a raw block of bytes to an Eet entry, optionally
 * compressing and encrypting it.
 */
EIO_API Eio_File *
eio_eet_write_cipher(Eet_File *ef,
		     const char *name,
		     void *write_data,
		     int size,
		     int compress,
		     const char *cipher_key,
		     Eio_Done_Int_Cb done_cb,
		     Eio_Error_Cb error_cb,
		     const void *user_data)
{
   Eio_Eet_Write *ew;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ef, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(name, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(done_cb, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(error_cb, NULL);

   ew = eio_common_alloc(sizeof(Eio_Eet_Write));
   EINA_SAFETY_ON_NULL_RETURN_VAL(ew, NULL);

   ew->ef = ef;
   ew->name = eina_stringshare_add(name);
   ew->cipher_key = eina_stringshare_add(cipher_key);
   ew->write_data = write_data;
   ew->size = size;
   ew->compress = compress;
   ew->done_cb = done_cb;
   ew->result = 0;

   if (!eio_file_set(&ew->common,
                     NULL,
                     error_cb,
                     user_data,
                     _eio_eet_write_job,
                     _eio_eet_write_end,
                     _eio_eet_write_cancel))
     return NULL;
   return &ew->common;
}
