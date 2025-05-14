#include "eldbus_private.h"
#include "eldbus_private_types.h"

#include <dbus/dbus-protocol.h>

/**
 * @internal
 * @brief Checks if a D-Bus type is compatible with an Eina_Value_Type.
 *
 * @param dbus_type The D-Bus type character (e.g., 'i', 's', 'a').
 * @param value_type Pointer to the Eina_Value_Type.
 * @return EINA_TRUE if types are compatible, EINA_FALSE otherwise.
 */
static Eina_Bool
_compatible_type(int dbus_type, const Eina_Value_Type *value_type)
{
   switch (dbus_type)
     {
      case 'i':
      case 'h':
         return value_type == EINA_VALUE_TYPE_INT;
      case 's':
      case 'o':
      case 'g':
         return value_type == EINA_VALUE_TYPE_STRING;
      case 'b':
      case 'y':
         return value_type == EINA_VALUE_TYPE_UCHAR;
      case 'n':
         return value_type == EINA_VALUE_TYPE_SHORT;
      case 'q':
         return value_type == EINA_VALUE_TYPE_USHORT;
      case 'u':
         return value_type == EINA_VALUE_TYPE_UINT;
      case 'x':
         return value_type == EINA_VALUE_TYPE_INT64;
      case 't':
         return value_type == EINA_VALUE_TYPE_UINT64;
      case 'd':
         return value_type == EINA_VALUE_TYPE_DOUBLE;
      case 'a':
         return value_type == EINA_VALUE_TYPE_ARRAY;
      case '(':
      case '{':
      case 'e':
      case 'r':
         return value_type == EINA_VALUE_TYPE_STRUCT;
      default:
         ERR("Unknown type %c", dbus_type);
         return EINA_FALSE;
     }
}

/**
 * @internal
 * @brief Appends an Eina_Value array to an Eldbus_Message_Iter.
 *
 * This function handles arrays of basic types, structs, and nested arrays.
 *
 * @param type The D-Bus signature string for the array (e.g., "ai" for array of integers,
 *             "a(ss)" for array of structs containing two strings, "aa{sv}" for an array of array of dictionary entries).
 *             The first character must be 'a'.
 * @param value_array Pointer to the Eina_Value containing the array.
 * @param iter Pointer to the Eldbus_Message_Iter to append to.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_array_append(const char *type, const Eina_Value *value_array, Eldbus_Message_Iter *iter)
{
   Eldbus_Message_Iter *array;
   Eina_Bool ok = eldbus_message_iter_arguments_append(iter, type, &array);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ok, EINA_FALSE);
   DBG("array of type %c", type[1]);
   switch (type[1])
     {
      case '{':
      case '(':
        {
           unsigned i = strlen(type+2);//remove 'a()' of len a(sv)
           char *entry_sig = malloc(sizeof(char) * i);
           memcpy(entry_sig, type+2, i);
           entry_sig[i-1] = 0;

           for (i = 0; i < eina_value_array_count(value_array); i++)
             {
                Eina_Value st;
                Eldbus_Message_Iter *entry;
                eina_value_array_value_get(value_array, i, &st);
                eldbus_message_iter_arguments_append(array, type+1, &entry);
                _message_iter_from_eina_value_struct(entry_sig, entry, &st);
                eldbus_message_iter_container_close(array, entry);
                eina_value_flush(&st);
             }
           free(entry_sig);
           break;
        }
      case 'a':
        {
           unsigned i;
           for (i = 0; i < eina_value_array_count(value_array); i++)
             {
                Eina_Value inner_array;
                Eldbus_Message_Iter *sub_array;
                eina_value_array_value_get(value_array, i, &inner_array);
                eldbus_message_iter_arguments_append(array, type+1, &sub_array);
                _array_append(type+1, &inner_array, sub_array);
                eldbus_message_iter_container_close(array, sub_array);
                eina_value_flush(&inner_array);
             }
           break;
        }
      case 'v':
        {
           ERR("Variant not supported.");
           return EINA_FALSE;
        }
      case 'i':
      case 'h'://fd
        {
           int32_t z;
           unsigned i;
           for (i = 0; i < eina_value_array_count(value_array); i++)
             {
                eina_value_array_get(value_array, i, &z);
                eldbus_message_iter_basic_append(array, type[1], z);
             }
           break;
        }
      case 's':
      case 'o'://object path
      case 'g'://signature
        {
           const char *txt;
           unsigned i;
           for (i = 0; i < eina_value_array_count(value_array); i++)
             {
                eina_value_array_get(value_array, i, &txt);
                eldbus_message_iter_basic_append(array, type[1], txt);
             }
           break;
        }
      case 'y'://byte
        {
           unsigned char z;
           unsigned i;
           for (i = 0; i < eina_value_array_count(value_array); i++)
             {
                eina_value_array_get(value_array, i, &z);
                eldbus_message_iter_basic_append(array, type[1], z);
             }
           break;
        }
      case 'b'://boolean
        {
           unsigned char z;
           unsigned i;
           for (i = 0; i < eina_value_array_count(value_array); i++)
             {
                eina_value_array_get(value_array, i, &z);
                eldbus_message_iter_basic_append(array, type[1], (uint32_t)z);
             }
           break;
        }
      case 'n'://int16
        {
           int16_t z;
           unsigned i;
           for (i = 0; i < eina_value_array_count(value_array); i++)
             {
                eina_value_array_get(value_array, i, &z);
                eldbus_message_iter_basic_append(array, type[1], z);
             }
           break;
        }
      case 'q'://uint16
        {
           uint16_t z;
           unsigned i;
           for (i = 0; i < eina_value_array_count(value_array); i++)
             {
                eina_value_array_get(value_array, i, &z);
                eldbus_message_iter_basic_append(array, type[1], z);
             }
           break;
        }
      case 'u'://uint32
        {
           uint32_t z;
           unsigned i;
           for (i = 0; i < eina_value_array_count(value_array); i++)
             {
                eina_value_array_get(value_array, i, &z);
                eldbus_message_iter_basic_append(array, type[1], z);
             }
           break;
        }
      case 'x'://int64
        {
           int64_t z;
           unsigned i;
           for (i = 0; i < eina_value_array_count(value_array); i++)
             {
                eina_value_array_get(value_array, i, &z);
                eldbus_message_iter_basic_append(array, type[1], z);
             }
           break;
        }
      case 't'://uint64
        {
           uint64_t z;
           unsigned i;
           for (i = 0; i < eina_value_array_count(value_array); i++)
             {
                eina_value_array_get(value_array, i, &z);
                eldbus_message_iter_basic_append(array, type[1], z);
             }
           break;
        }
      case 'd'://double
        {
           double z;
           unsigned i;
           for (i = 0; i < eina_value_array_count(value_array); i++)
             {
                eina_value_array_get(value_array, i, &z);
                eldbus_message_iter_basic_append(array, type[1], z);
             }
           break;
        }
      default:
        {
           ERR("Unknown type %c", type[1]);
           return EINA_FALSE;
        }
     }
   eldbus_message_iter_container_close(iter, array);
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Appends a basic D-Bus type from a field in an Eina_Value struct to an Eldbus_Message_Iter.
 *
 * @param type The D-Bus basic type character (e.g., 'i', 's', 'b').
 * @param value Pointer to the Eina_Value struct.
 * @param desc Pointer to the Eina_Value_Struct_Desc describing the struct.
 * @param idx The index of the member within the struct description.
 * @param iter Pointer to the Eldbus_Message_Iter to append to.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_basic_append_value_struct(char type, const Eina_Value *value, const Eina_Value_Struct_Desc *desc, unsigned idx, Eldbus_Message_Iter *iter)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL(
            _compatible_type(type, desc->members[idx].type), EINA_FALSE);
   switch (type)
     {
      case 'i'://int
      case 'h'://fd
        {
           int32_t i;
           eina_value_struct_get(value, desc->members[idx].name, &i);
           eldbus_message_iter_basic_append(iter, type, i);
           break;
        }
      case 's':
      case 'o'://object path
      case 'g'://signature
        {
           const char *txt;
           eina_value_struct_get(value, desc->members[idx].name, &txt);
           eldbus_message_iter_basic_append(iter, type, txt);
           break;
        }
      case 'y'://byte
        {
           unsigned char byte;
           eina_value_struct_get(value, desc->members[idx].name, &byte);
           eldbus_message_iter_basic_append(iter, type, byte);
           break;
        }
      case 'b'://boolean
        {
           unsigned char boolean;
           eina_value_struct_get(value, desc->members[idx].name, &boolean);
           eldbus_message_iter_basic_append(iter, type, (uint32_t)boolean);
           break;
        }
      case 'n'://int16
        {
           int16_t i;
           eina_value_struct_get(value, desc->members[idx].name, &i);
           eldbus_message_iter_basic_append(iter, type, i);
           break;
        }
      case 'q'://uint16
        {
           uint16_t i;
           eina_value_struct_get(value, desc->members[idx].name, &i);
           eldbus_message_iter_basic_append(iter, type, i);
           break;
        }
      case 'u'://uint32
        {
           uint32_t i;
           eina_value_struct_get(value, desc->members[idx].name, &i);
           eldbus_message_iter_basic_append(iter, type, i);
           break;
        }
      case 'x'://int64
        {
           int64_t i;
           eina_value_struct_get(value, desc->members[idx].name, &i);
           eldbus_message_iter_basic_append(iter, type, i);
           break;
        }
      case 't'://uint64
        {
           uint64_t i;
           eina_value_struct_get(value, desc->members[idx].name, &i);
           eldbus_message_iter_basic_append(iter, type, i);
           break;
        }
      case 'd'://double
        {
           double d;
           eina_value_struct_get(value, desc->members[idx].name, &d);
           eldbus_message_iter_basic_append(iter, type, d);
           break;
        }
      default:
        ERR("Unexpected type %c", type);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Converts an Eina_Value (expected to be a struct) to D-Bus arguments and appends them to an Eldbus_Message_Iter.
 *
 * This function iterates through the D-Bus signature and the Eina_Value struct members,
 * appending each corresponding value. It handles basic types, arrays, and nested structs.
 *
 * @param signature The D-Bus signature string for the struct's contents (e.g., "is(s)", "a{sv}u").
 *                  This is the signature *inside* the struct parentheses.
 * @param iter Pointer to the Eldbus_Message_Iter to append the D-Bus arguments to.
 *             If the Eina_Value represents a D-Bus struct, this iterator should be the one opened for that struct.
 * @param value Pointer to the Eina_Value, which must be of type EINA_VALUE_TYPE_STRUCT.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
_message_iter_from_eina_value_struct(const char *signature, Eldbus_Message_Iter *iter, const Eina_Value *value)
{
   unsigned i;
   DBusSignatureIter signature_iter;
   Eina_Bool r = EINA_TRUE;
   char *type;
   Eina_Value_Struct st;

   EINA_SAFETY_ON_FALSE_RETURN_VAL(
            eina_value_type_get(value) == EINA_VALUE_TYPE_STRUCT, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(
      eina_value_pget(value, &st), EINA_FALSE);

   dbus_signature_iter_init(&signature_iter, signature);
   i = 0;
   while ((type = dbus_signature_iter_get_signature(&signature_iter)))
     {
        DBG("type: %s", type);
        if (type[0] != 'v' && !type[1])
          r = _basic_append_value_struct(type[0], value, st.desc, i, iter);
        else if (type[0] == 'a')
          {
             Eina_Value value_array;

             EINA_SAFETY_ON_FALSE_RETURN_VAL(
                      _compatible_type(type[0], st.desc->members[i].type),
                      EINA_FALSE);
             eina_value_struct_value_get(value, st.desc->members[i].name,
                                         &value_array);
             r = _array_append(type, &value_array, iter);
             eina_value_flush(&value_array);
          }
        else if (type[0] == '(')
          {
             Eina_Value inner_st;
             Eldbus_Message_Iter *sub_iter;
             char *sub_sig;
             unsigned len = strlen(type+1) -1;
             sub_sig = alloca(sizeof(char) * len);
             memcpy(sub_sig, type+1, len);
             sub_sig[len-1] = 0;
             EINA_SAFETY_ON_FALSE_RETURN_VAL(
                         _compatible_type(type[0], st.desc->members[i].type),
                         EINA_FALSE);
             eina_value_struct_value_get(value, st.desc->members[i].name, &inner_st);
             eldbus_message_iter_arguments_append(iter, type, &sub_iter);
             r = _message_iter_from_eina_value_struct(sub_sig, sub_iter,
                                                      &inner_st);
             eldbus_message_iter_container_close(iter, sub_iter);
          }
        else if (type[0] == 'v')
          {
             ERR("Variant not supported");
             r = EINA_FALSE;
          }
        else
          {
             ERR("Unknown type %c", type[0]);
             r = EINA_FALSE;
          }
        i++;
        dbus_free(type);
        if (!r || !dbus_signature_iter_next(&signature_iter)) break;
     }
   return r;
}

/**
 * @brief Populates an Eldbus_Message with arguments from an Eina_Value.
 *
 * This function is typically used when constructing a new message to be sent.
 * The Eina_Value is expected to be a struct, where each member of the struct
 * corresponds to an argument in the D-Bus signature.
 *
 * Example:
 * If signature is "is" and Eina_Value is a struct with an int and a string:
 * Eina_Value *val = eina_value_struct_new(desc_is, 123, "hello");
 * eldbus_message_from_eina_value("is", msg, val);
 * eina_value_free(val);
 *
 * If signature is "a(ss)" and Eina_Value is a struct containing an array of structs (each with two strings):
 * Eina_Value *val_struct = eina_value_struct_new(desc_main_struct); // desc for a struct that holds an array
 * Eina_Value *val_array = eina_value_array_new(desc_struct_ss_member, 0); // desc for the (ss) struct
 * Eina_Value *inner_st1 = eina_value_struct_new(desc_struct_ss_member, "key1", "val1");
 * eina_value_array_append(val_array, inner_st1);
 * eina_value_free(inner_st1);
 * // ... append more inner_st...
 * eina_value_struct_set(val_struct, "array_field_name", val_array);
 * eldbus_message_from_eina_value("a(ss)", msg, val_struct); // Assuming val_struct has one field which is the array
 * // Or, if the Eina_Value *is* the array directly (less common for top-level message arguments but possible):
 * // eldbus_message_from_eina_value("a(ss)", msg, val_array); // This would use _message_iter_from_eina_value_struct path
 * eina_value_free(val_array);
 * eina_value_free(val_struct);
 *
 * @param signature The D-Bus signature string for all arguments in the message.
 * @param msg Pointer to the Eldbus_Message to populate.
 * @param value Pointer to the Eina_Value (typically a struct) containing the data.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
EAPI Eina_Bool
eldbus_message_from_eina_value(const char *signature, Eldbus_Message *msg, const Eina_Value *value)
{
   Eldbus_Message_Iter *iter;
   EINA_SAFETY_ON_NULL_RETURN_VAL(signature, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(msg, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(value, EINA_FALSE);

   iter = eldbus_message_iter_get(msg);
   EINA_SAFETY_ON_NULL_RETURN_VAL(iter, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(iter->writable, EINA_FALSE);

   return _message_iter_from_eina_value_struct(signature, iter, value);
}

/**
 * @internal
 * @brief Appends a basic D-Bus type from an Eina_Value to an Eldbus_Message_Iter.
 *
 * This function is used when the Eina_Value directly holds a basic type
 * (not a struct member).
 *
 * @param type The D-Bus basic type character (e.g., 'i', 's', 'b').
 * @param value Pointer to the Eina_Value containing the basic type.
 * @param iter Pointer to the Eldbus_Message_Iter to append to.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
static Eina_Bool
_basic_append_value(char type, const Eina_Value *value, Eldbus_Message_Iter *iter)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(value, EINA_FALSE);
   const Eina_Value_Type *value_type = eina_value_type_get(value);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(_compatible_type(type, value_type), EINA_FALSE);
   switch (type)
     {
      case 'i'://int
      case 'h'://fd
        {
           int32_t i;
           eina_value_get(value, &i);
           eldbus_message_iter_basic_append(iter, type, i);
           break;
        }
      case 's':
      case 'o'://object path
      case 'g'://signature
        {
           const char *txt;
           eina_value_get(value, &txt);
           eldbus_message_iter_basic_append(iter, type, txt);
           break;
        }
      case 'y'://byte
        {
           unsigned char byte;
           eina_value_get(value, &byte);
           eldbus_message_iter_basic_append(iter, type, byte);
           break;
        }
      case 'b'://boolean
        {
           unsigned char boolean;
           eina_value_get(value, &boolean);
           eldbus_message_iter_basic_append(iter, type, (uint32_t)boolean);
           break;
        }
      case 'n'://int16
        {
           int16_t i;
           eina_value_get(value, &i);
           eldbus_message_iter_basic_append(iter, type, i);
           break;
        }
      case 'q'://uint16
        {
           uint16_t i;
           eina_value_get(value, &i);
           eldbus_message_iter_basic_append(iter, type, i);
           break;
        }
      case 'u'://uint32
        {
           uint32_t i;
           eina_value_get(value, &i);
           eldbus_message_iter_basic_append(iter, type, i);
           break;
        }
      case 'x'://int64
        {
           int64_t i;
           eina_value_get(value, &i);
           eldbus_message_iter_basic_append(iter, type, i);
           break;
        }
      case 't'://uint64
        {
           uint64_t i;
           eina_value_get(value, &i);
           eldbus_message_iter_basic_append(iter, type, i);
           break;
        }
      case 'd'://double
        {
           double d;
           eina_value_get(value, &d);
           eldbus_message_iter_basic_append(iter, type, d);
           break;
        }
      default:
        ERR("Unexpected type %c", type);
        return EINA_FALSE;
     }
   return EINA_TRUE;
}

/**
 * @internal
 * @brief Converts an Eina_Value to D-Bus arguments and appends them to an Eldbus_Message_Iter.
 *
 * This function serves as a dispatcher. If the Eina_Value is a struct or array,
 * it calls _message_iter_from_eina_value_struct. Otherwise, it assumes the Eina_Value
 * represents a single basic type that matches the provided signature and uses
 * _basic_append_value. This is less common for top-level message construction,
 * which usually involves structs, but can be used for simpler cases or internally.
 *
 * @param signature The D-Bus signature string. If the Eina_Value is not a struct/array,
 *                  this signature is expected to be for a single basic type (e.g., "i", "s").
 *                  If the Eina_Value is a struct or array, this signature is passed to
 *                  _message_iter_from_eina_value_struct.
 * @param iter Pointer to the Eldbus_Message_Iter to append the D-Bus arguments to.
 * @param value Pointer to the Eina_Value.
 * @return EINA_TRUE on success, EINA_FALSE on failure.
 */
Eina_Bool
_message_iter_from_eina_value(const char *signature, Eldbus_Message_Iter *iter, const Eina_Value *value)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(value, EINA_FALSE);

   const Eina_Value_Type *value_type = eina_value_type_get(value);
   if (EINA_VALUE_TYPE_STRUCT == value_type || EINA_VALUE_TYPE_ARRAY == value_type)
     return _message_iter_from_eina_value_struct(signature, iter, value);

   Eina_Bool result = EINA_TRUE;
   DBusSignatureIter signature_iter;
   dbus_signature_iter_init(&signature_iter, signature);
   char *type;
   while ((type = dbus_signature_iter_get_signature(&signature_iter)))
     {
        DBG("type: %s", type);
        if (DBUS_TYPE_VARIANT != type[0] && DBUS_TYPE_INVALID == type[1])
          result = _basic_append_value(type[0], value, iter);
        else if (DBUS_TYPE_ARRAY == type[0] ||
                 DBUS_STRUCT_BEGIN_CHAR == type[0] ||
                 DBUS_TYPE_VARIANT == type[0])
          {
             ERR("Not a basic type");
             result = EINA_FALSE;
          }
        else
          {
             ERR("Unknown type %c", type[0]);
             result = EINA_FALSE;
          }
        dbus_free(type);
        if (!result || !dbus_signature_iter_next(&signature_iter)) break;
     }
   return result;
}
