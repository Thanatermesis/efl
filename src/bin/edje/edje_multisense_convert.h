/**
 * @file
 * @brief Functions for converting sound samples to different formats (FLAC, Ogg/Vorbis)
 *        for Edje's multisense feature.
 */
#ifndef EDJE_SND_CONVERT_H__
# define EDJE_SND_CONVERT_H__
#include "edje_private.h"

#ifdef HAVE_LIBSNDFILE
#include <sndfile.h>

#define SF_CONTAINER(x)    ((x) & SF_FORMAT_TYPEMASK)
#define SF_CODEC(x)        ((x) & SF_FORMAT_SUBMASK)

/**
 * @brief Structure to hold information about the sound encoding process.
 */
typedef struct _Edje_Sound_Encode  Edje_Sound_Encode;

struct _Edje_Sound_Encode /*Encoding information*/
{
   const char *file; /**< The path to the encoded sound file. This will be the original path if no encoding occurred. */
   Eina_Bool encoded; /**< EINA_TRUE if encoding was successful, EINA_FALSE otherwise. */
   char *comp_type; /**< A string describing the compression type used (e.g., "FLAC", "OGG-VORBIS", "RAW PCM"). */
};

/**
 * @brief Encodes a sound sample based on its specified compression type.
 *
 * This function takes a sound file (typically WAV) and an Edje_Sound_Sample
 * structure. Based on the `compression` field in the `sample` structure,
 * it will attempt to encode the sound to FLAC (for lossless) or Ogg/Vorbis
 * (for lossy). If no specific compression is requested or if the required
 * libraries are not available, it will return information pointing to the
 * original file.
 *
 * @param filename The path to the input sound file (e.g., "sound.wav").
 * @param sample Pointer to the Edje_Sound_Sample structure containing details
 *               about the sound, including the desired compression type.
 * @param quality For lossy compression (Ogg/Vorbis), this specifies the
 *                quality level (typically 0.0 to 1.0). Unused for FLAC.
 * @return A pointer to an Edje_Sound_Encode structure containing the results
 *         of the encoding attempt. The caller is responsible for freeing this
 *         structure and its `file` member if it was duplicated.
 *         Returns NULL on critical memory allocation failure (program will likely exit).
 */
Edje_Sound_Encode *_edje_multisense_encode(const char* filename, Edje_Sound_Sample *sample, double quality);

/**
 * @brief Encodes a sound file to FLAC format.
 *
 * @param snd_path The path to the input sound file (e.g., "sound.wav").
 *                 This path will be modified to append ".flac" if encoding is successful.
 * @param sfinfo Information about the input sound file obtained from libsndfile.
 * @return The path to the newly created FLAC file (e.g., "sound.wav.flac"),
 *         or NULL if encoding fails. The returned path is a newly allocated string
 *         if successful and needs to be freed by the caller.
 */
const char *_edje_multisense_encode_to_flac(char *snd_path, SF_INFO sfinfo);

/**
 * @brief Encodes a sound file to Ogg/Vorbis format.
 *
 * @param snd_path The path to the input sound file (e.g., "sound.wav").
 *                 This path will be modified to append ".ogg" if encoding is successful.
 * @param quality The desired quality for Ogg/Vorbis encoding (typically 0.0 to 1.0).
 * @param sfinfo Information about the input sound file obtained from libsndfile.
 * @return The path to the newly created Ogg/Vorbis file (e.g., "sound.wav.ogg"),
 *         or NULL if encoding fails. The returned path is a newly allocated string
 *         if successful and needs to be freed by the caller.
 */
const char *_edje_multisense_encode_to_ogg_vorbis(char *snd_path, double quality, SF_INFO sfinfo);

#endif
#endif
