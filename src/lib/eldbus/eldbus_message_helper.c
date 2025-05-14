#include "eldbus_private.h"
#include "eldbus_private_types.h"

EAPI void
eldbus_message_iter_dict_iterate(Eldbus_Message_Iter *array, const char *signature, Eldbus_Dict_Cb_Get cb, const void *data)
{
   // array: An iterator that yields dictionary entries. Typically, this iterator is
   //        obtained by first getting an iterator to a D-Bus array of dictionary
   //        entries (e.g., type "a{sv}") and then recursing into it.
   //        The signature of this 'array' iterator itself would then be the
   //        signature of a single dictionary entry's structure (e.g., "{sv}").
   // signature: The D-Bus signature of the key-value pair within each dictionary
   //            entry (e.g., "sv" for a string key and variant value).
   // cb: Callback function to be invoked for each key-value pair.
   // data: User-provided context data for the callback.

   Eldbus_Message_Iter *entry; // Iterator for the contents of a single dictionary entry
   char *iter_sig;             // Signature of the 'array' iterator (e.g., "{sv}")
   unsigned len;

   // Ensure the main iterator and the expected entry signature are not NULL.
   EINA_SAFETY_ON_FALSE_RETURN(array);
   EINA_SAFETY_ON_NULL_RETURN(signature);

   // Retrieve the D-Bus signature of the elements that 'array' iterates over.
   // For an iterator over dictionary entries {key_type, value_type},
   // this would be "{key_typevalue_type}", e.g., "{sv}".
   iter_sig = eldbus_message_iter_signature_get(array);
   // Calculate the length of the signature string, starting from the character
   // after the initial '{'.
   // e.g., if iter_sig is "{sv}", iter_sig + 1 is "sv}", strlen("sv}") is 3. So len = 3.
   len = strlen(iter_sig + 1);

   // Validate that the provided 'signature' (e.g., "sv") matches the content
   // signature derived from 'array's iterator type.
   // The comparison `strncmp(signature, iter_sig + 1, len - 1)` effectively
   // compares `signature` with the content of `iter_sig` excluding the
   // surrounding curly braces.
   // e.g., if iter_sig is "{sv}" and signature is "sv":
   // iter_sig + 1 is "sv}"
   // len is 3 (strlen("sv}"))
   // len - 1 is 2
   // strncmp("sv", "sv}", 2) compares "sv" with "sv", which should match.
   if (strncmp(signature, iter_sig + 1, len - 1))
     {
        ERR("Unexpected signature, expected is: %s", iter_sig);
        free(iter_sig);
        return;
     }
   free(iter_sig); // iter_sig is no longer needed.

   // Loop through each dictionary entry provided by the 'array' iterator.
   // 'e' (DBUS_TYPE_DICT_ENTRY) signifies a dictionary entry.
   // On each iteration, 'entry' becomes an iterator for the current dictionary
   // entry's contents (key and value). The 'array' iterator is advanced.
   while (eldbus_message_iter_get_and_next(array, 'e', &entry))
     {
        const void *key;        // Pointer to the key of the dictionary entry
        Eldbus_Message_Iter *var; // Iterator for the value of the dictionary entry

        // Extract the key and value from the current dictionary entry ('entry')
        // according to the provided 'signature' (e.g., "sv").
        if (!eldbus_message_iter_arguments_get(entry, signature, &key, &var))
          continue; // Skip if arguments cannot be retrieved (e.g., malformed entry)

        // Invoke the callback with user data, the extracted key, and the value iterator.
        cb((void *)data, key, var);
     }
}
