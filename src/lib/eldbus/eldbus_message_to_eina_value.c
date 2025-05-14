#include "eldbus_private.h"
#include "eldbus_private_types.h"

/**
 * @internal
 * @brief Populates an Eina_Value array with basic types from an Eldbus_Message_Iter.
 *
 * This function iterates through an Eldbus_Message_Iter containing an array of basic D-Bus types
 * (integers, strings, booleans, etc.) and appends each element to the provided Eina_Value array.
 *
 * @param type The D-Bus type character of the elements in the array (e.g., 'i', 's', 'b').
 * @param value An initialized Eina_Value of type EINA_VALUE_TYPE_ARRAY, to which elements will be appended.
 * @param iter The Eldbus_Message_Iter positioned at the start of the D-Bus array.
 */
static void _message_iter_basic_array_to_eina_value(char type, Eina_Value *value, Eldbus_Message_Iter *iter);

/**
 * @internal
 * @brief Converts a D-Bus type character to its corresponding Eina_Value_Type.
 *
 * @param type The D-Bus type character (e.g., 'i', 's', 'a', '(', '{').
 * @return The corresponding Eina_Value_Type, or NULL if the type is unknown.
 */
const Eina_Value_Type *
_dbus_type_to_eina_value_type(char type)
{
   switch (type)
     {
      case 'i':
      case 'h':
         return EINA_VALUE_TYPE_INT;
      case 's':
      case 'o':
      case 'g':
         return EINA_VALUE_TYPE_STRING;
      case 'b':
      case 'y':
         return EINA_VALUE_TYPE_UCHAR;
      case 'n':
         return EINA_VALUE_TYPE_SHORT;
      case 'q':
         return EINA_VALUE_TYPE_USHORT;
      case 'u':
         return EINA_VALUE_TYPE_UINT;
      case 'x':
         return EINA_VALUE_TYPE_INT64;
      case 't':
         return EINA_VALUE_TYPE_UINT64;
      case 'd':
         return EINA_VALUE_TYPE_DOUBLE;
      case 'a':
         return EINA_VALUE_TYPE_ARRAY;
      case '(':
      case '{':
      case 'e':
      case 'r':
      case 'v':
         return EINA_VALUE_TYPE_STRUCT;
      default:
         ERR("Unknown type %c", type);
         return NULL;
     }
}

/**
 * @internal
 * @brief Gets the size in bytes of a D-Bus basic type.
 *
 * This is used for calculating memory layout, particularly for structs.
 *
 * @param type The D-Bus type character.
 * @return The size of the type in bytes, or 0 if the type is unknown.
 */
static unsigned int
_type_size(char type)
{
   switch (type)
     {
      case 'i':
      case 'h':
      case 'u':
         return(sizeof(int32_t));
      case 's':
      case 'o':
      case 'g':
         return(sizeof(char *));
      case 'b':
      case 'y':
         return(sizeof(unsigned char));
      case 'n':
      case 'q':
         return(sizeof(int16_t));
      case 'x':
      case 't':
         return(sizeof(int64_t));
      case 'd':
         return(sizeof(double));
      case 'a':
         return(sizeof(Eina_Value_Array));
      case '(':
      case '{':
      case 'e':
      case 'r':
      case 'v':
         return(sizeof(Eina_Value_Struct));
      default:
         ERR("Unknown type %c", type);
         return 0;
     }
}

/**
 * @internal
 * @brief Calculates the memory offset for a D-Bus type, ensuring proper alignment.
 *
 * D-Bus types have alignment requirements. This function calculates the next
 * valid offset for a given type, starting from a base offset.
 * For example, a 4-byte integer (type 'i') must be aligned to a 4-byte boundary.
 * If base is 1 and type is 'i', the returned offset will be 4.
 * If base is 4 and type is 'i', the returned offset will be 4.
 *
 * @param type The D-Bus type character for which to calculate the offset.
 * @param base The current base offset.
 * @return The new offset, aligned for the given type.
 */
static unsigned int
_type_offset(char type, unsigned base)
{
   unsigned size, padding;
   size = _type_size(type);
   if (size == 0)
     return base;
   if (!(base % size))
     return base;
   padding = (base > size) ? base - size : size - base;
   return base + padding;
}

/**
 * @internal
 * @brief Converts a D-Bus array from an Eldbus_Message_Iter to an Eina_Value.
 *
 * This function handles arrays of basic types, arrays of structs, arrays of variants,
 * and arrays of arrays.
 *
 * For an array of structs, e.g., "a(is)", the Eina_Value will be an array
 * where each element is an Eina_Value struct with members "arg0" (int) and "arg1" (string).
 * Example: `[ { "arg0": 1, "arg1": "foo" }, { "arg0": 2, "arg1": "bar" } ]`
 *
 * For an array of arrays, e.g., "aai", the Eina_Value will be an array
 * where each element is another Eina_Value array of integers.
 * Example: `[ [1, 2], [3, 4, 5] ]`
 *
 * @param iter The Eldbus_Message_Iter positioned at the D-Bus array.
 * @return A new Eina_Value of type EINA_VALUE_TYPE_ARRAY containing the converted
 *         array elements, or NULL on failure. The caller is responsible for freeing
 *         the returned Eina_Value.
 */
static Eina_Value *
_message_iter_array_to_eina_value(Eldbus_Message_Iter *iter)
{
   Eina_Value *array_value;
   char *sig;

   sig = eldbus_message_iter_signature_get(iter);
   DBG("array of %s", sig);
   array_value = eina_value_array_new(_dbus_type_to_eina_value_type(sig[0]), 0);
   if (sig[0] == '(' || sig[0] == '{' || sig[0] == 'v')
     {
        Eldbus_Message_Iter *entry;

        if (sig[0] == '{')
          sig[0] = 'e';
        else if (sig[0] == '(')
          sig[0] = 'r';

        while (eldbus_message_iter_get_and_next(iter, sig[0], &entry))
          {
             Eina_Value *data = _message_iter_struct_to_eina_value(entry);
             Eina_Value_Struct st;
             eina_value_get(data, &st);
             eina_value_array_append(array_value, st);
             eina_value_free(data);
          }
     }
   else if (sig[0] == 'a')
     {
        Eldbus_Message_Iter *entry;
        while (eldbus_message_iter_get_and_next(iter, sig[0], &entry))
          {
             Eina_Value *data = _message_iter_array_to_eina_value(entry);
             Eina_Value_Array inner_array;
             eina_value_get(data, &inner_array);
             eina_value_array_append(array_value, inner_array);
             eina_value_free(data);
          }
     }
   else
     _message_iter_basic_array_to_eina_value(sig[0], array_value, iter);

   DBG("return array of %s", sig);
   free(sig);
   return array_value;
}

static void
_message_iter_basic_array_to_eina_value(char type, Eina_Value *value, Eldbus_Message_Iter *iter)
{
   switch (type)
    {
       case 'i':
       case 'h'://fd
         {
            int32_t i;
            while (eldbus_message_iter_get_and_next(iter, type, &i))
              eina_value_array_append(value, i);
            break;
         }
       case 's':
       case 'o'://object path
       case 'g'://signature
         {
            const char *txt;
            while (eldbus_message_iter_get_and_next(iter, type, &txt))
              eina_value_array_append(value, txt);
            break;
         }
       case 'y'://byte
         {
            unsigned char byte;
            while (eldbus_message_iter_get_and_next(iter, type, &byte))
              eina_value_array_append(value, byte);
            break;
         }
       case 'b'://boolean
         {
            uint32_t boolean;
            while (eldbus_message_iter_get_and_next(iter, type, &boolean))
              eina_value_array_append(value, (uint8_t)boolean);
            break;
         }
       case 'n'://int16
         {
            int16_t i;
            while (eldbus_message_iter_get_and_next(iter, type, &i))
              eina_value_array_append(value, i);
            break;
         }
       case 'q'://uint16
         {
            uint16_t i;
            while (eldbus_message_iter_get_and_next(iter, type, &i))
              eina_value_array_append(value, i);
            break;
         }
       case 'u'://uint32
         {
            uint32_t i;
            while (eldbus_message_iter_get_and_next(iter, type, &i))
              eina_value_array_append(value, i);
            break;
         }
       case 'x'://int64
         {
            int64_t i;
            while (eldbus_message_iter_get_and_next(iter, type, &i))
              eina_value_array_append(value, i);
            break;
         }
       case 't'://uint64
         {
            uint64_t i;
            while (eldbus_message_iter_get_and_next(iter, type, &i))
              eina_value_array_append(value, i);
            break;
         }
       case 'd'://double
         {
            double d;
            while (eldbus_message_iter_get_and_next(iter, type, &d))
              eina_value_array_append(value, d);
            break;
         }
    }
}

#define ARG "arg%u"

typedef struct _Eldbus_Struct_Desc
{
   Eina_Value_Struct_Desc base;
   int refcount;
} Eldbus_Struct_Desc;

static void *
_ops_malloc(const Eina_Value_Struct_Operations *ops EINA_UNUSED, const Eina_Value_Struct_Desc *desc)
{
   Eldbus_Struct_Desc *edesc = (Eldbus_Struct_Desc*)desc;
   edesc->refcount++; // Increment refcount for the descriptor
   DBG("%p refcount=%d", edesc, edesc->refcount);
   return malloc(desc->size);
}

/**
 * @internal
 * @brief Custom free operation for Eina_Value structs created from D-Bus messages.
 *
 * This function decrements the reference count of the Eldbus_Struct_Desc.
 * If the refcount reaches zero, it frees the descriptor and its associated members.
 *
 * @param ops Pointer to the Eina_Value_Struct_Operations (unused).
 * @param desc Pointer to the Eina_Value_Struct_Desc (cast to Eldbus_Struct_Desc).
 * @param memory Pointer to the memory allocated for the struct instance.
 */
static void
_ops_free(const Eina_Value_Struct_Operations *ops EINA_UNUSED, const Eina_Value_Struct_Desc *desc, void *memory)
{
   Eldbus_Struct_Desc *edesc = (Eldbus_Struct_Desc*) desc;
   edesc->refcount--;
   free(memory);
   DBG("%p refcount=%d", edesc, edesc->refcount);
   if (edesc->refcount <= 0)
     {
        unsigned i;
        for (i = 0; i < edesc->base.member_count; i++)
          free((char *)edesc->base.members[i].name);
        free((Eina_Value_Struct_Member *)edesc->base.members);
        free(edesc);
     }
}

static Eina_Value_Struct_Operations operations =
{
   EINA_VALUE_STRUCT_OPERATIONS_VERSION,
   _ops_malloc,
   _ops_free,
   NULL,
   NULL,
   NULL
};

/**
 * @internal
 * @brief Converts a D-Bus struct or variant from an Eldbus_Message_Iter to an Eina_Value.
 *
 * This function iterates through the members of a D-Bus struct (or the single item
 * in a D-Bus variant, which is treated as a struct with one member) and constructs
 * an Eina_Value of type EINA_VALUE_TYPE_STRUCT.
 *
 * Each member of the D-Bus struct will correspond to a member in the Eina_Value struct,
 * named "arg0", "arg1", "arg2", etc., based on its position in the D-Bus struct.
 *
 * For example, a D-Bus struct "(isd)" (int, string, double) will be converted to
 * an Eina_Value struct with three members:
 * - "arg0": EINA_VALUE_TYPE_INT
 * - "arg1": EINA_VALUE_TYPE_STRING
 * - "arg2": EINA_VALUE_TYPE_DOUBLE
 *
 * @param iter The Eldbus_Message_Iter positioned at the D-Bus struct or variant.
 * @return A new Eina_Value of type EINA_VALUE_TYPE_STRUCT representing the
 *         D-Bus struct, or NULL if the iterator is empty or on failure.
 *         The caller is responsible for freeing the returned Eina_Value.
 */
Eina_Value *
_message_iter_struct_to_eina_value(Eldbus_Message_Iter *iter)
{
   int type;
   Eina_Value *value_st = NULL;
   Eina_Array *st_members = eina_array_new(1);
   unsigned int offset = 0, z;
   char name[16];//arg000 + \0
   Eina_Value_Struct_Member *members;
   Eldbus_Struct_Desc *st_desc;
   Eina_Array *st_values = eina_array_new(1);

   DBG("begin struct");
   st_desc = calloc(1, sizeof(Eldbus_Struct_Desc));
   st_desc->base.version = EINA_VALUE_STRUCT_DESC_VERSION;
   st_desc->base.ops = &operations;

   //create member list
   z = 0;
   while ((type = dbus_message_iter_get_arg_type(&iter->dbus_iterator)) != DBUS_TYPE_INVALID)
     {
        Eina_Value_Struct_Member *m;
        Eina_Value *v;

        m = calloc(1, sizeof(Eina_Value_Struct_Member));
        snprintf(name, 7, ARG, z);
        m->name = strdup(name);
        offset = _type_offset(type, offset);
        m->offset = offset;
        offset += _type_size(type);
        m->type = _dbus_type_to_eina_value_type(type);
        eina_array_push(st_members, m);

        DBG("type = %c", type);
        switch (type)
          {
           case 'i'://int
           case 'h'://fd
             {
                int32_t i;
                v = eina_value_new(EINA_VALUE_TYPE_INT);
                eldbus_message_iter_basic_get(iter, &i);
                eina_value_set(v, i);
                break;
             }
           case 's':
           case 'o'://object path
           case 'g'://signature
             {
                const char *txt;
                v = eina_value_new(EINA_VALUE_TYPE_STRING);
                eldbus_message_iter_basic_get(iter, &txt);
                eina_value_set(v, txt);
                break;
             }
           case 'y'://byte
             {
                unsigned char byte;
                v = eina_value_new(EINA_VALUE_TYPE_UCHAR);
                eldbus_message_iter_basic_get(iter, &byte);
                eina_value_set(v, byte);
                break;
             }
           case 'b'://boolean
             {
                 uint32_t value;
                 v = eina_value_new(EINA_VALUE_TYPE_UCHAR);
                 eldbus_message_iter_basic_get(iter, &value);
                 eina_value_set(v, (uint8_t)value);
                 break;
             }
           case 'n'://int16
             {
                int16_t i;
                v = eina_value_new(EINA_VALUE_TYPE_SHORT);
                eldbus_message_iter_basic_get(iter, &i);
                eina_value_set(v, i);
                break;
             }
           case 'q'://uint16
             {
                uint16_t i;
                v = eina_value_new(EINA_VALUE_TYPE_USHORT);
                eldbus_message_iter_basic_get(iter, &i);
                eina_value_set(v, i);
                break;
             }
           case 'u'://uint32
             {
                uint32_t i;
                v = eina_value_new(EINA_VALUE_TYPE_UINT);
                eldbus_message_iter_basic_get(iter, &i);
                eina_value_set(v, i);
                break;
             }
           case 'x'://int64
             {
                int64_t i;
                v = eina_value_new(EINA_VALUE_TYPE_INT64);
                eldbus_message_iter_basic_get(iter, &i);
                eina_value_set(v, i);
                break;
             }
           case 't'://uint64
             {
                uint64_t i;
                v = eina_value_new(EINA_VALUE_TYPE_UINT64);
                eldbus_message_iter_basic_get(iter, &i);
                eina_value_set(v, i);
                break;
             }
           case 'd'://double
             {
                double d;
                v = eina_value_new(EINA_VALUE_TYPE_DOUBLE);
                eldbus_message_iter_basic_get(iter, &d);
                eina_value_set(v, d);
                break;
             }
           case 'a'://array
             {
                Eldbus_Message_Iter *dbus_array;
                dbus_array = eldbus_message_iter_sub_iter_get(iter);
                v = _message_iter_array_to_eina_value(dbus_array);
                break;
             }
           case '('://struct
           case 'r'://struct
           case 'v'://variant
             {
                Eldbus_Message_Iter *dbus_st;
                dbus_st = eldbus_message_iter_sub_iter_get(iter);
                v = _message_iter_struct_to_eina_value(dbus_st);
                break;
             }
           default:
             ERR("Unexpected type %c", type);
             v = NULL;
          }
        eina_array_push(st_values, v);
        eldbus_message_iter_next(iter);
        z++;
     }

   if (!z)
     {
        free(st_desc);
        goto end;
     }

   members = malloc(eina_array_count(st_members) * sizeof(Eina_Value_Struct_Member));
   for (z = 0; z < eina_array_count(st_members); z++)
     {
        Eina_Value_Struct_Member *m = eina_array_data_get(st_members, z);
        members[z].name = m->name;
        members[z].offset = m->offset;
        members[z].type = m->type;
        free(m);
     }

   //setup
   st_desc->base.members = members;
   st_desc->base.member_count = eina_array_count(st_members);
   st_desc->base.size = offset;
   value_st = eina_value_struct_new((Eina_Value_Struct_Desc *)st_desc);

   //filling with data
   for (z = 0; z < eina_array_count(st_values); z++)
     {
        Eina_Value *v = eina_array_data_get(st_values, z);
        sprintf(name, ARG, z);
        eina_value_struct_value_set(value_st, name, v);
        eina_value_free(v);
     }

end:
   eina_array_free(st_members);
   eina_array_free(st_values);
   DBG("end struct");
   return value_st;
}

/**
 * @brief Converts an entire Eldbus_Message payload to an Eina_Value.
 *
 * The D-Bus message payload is typically a series of arguments. This function
 * treats these arguments as if they were fields of a single top-level struct.
 * The resulting Eina_Value will be of type EINA_VALUE_TYPE_STRUCT, where
 * each member ("arg0", "arg1", ...) corresponds to an argument from the message.
 *
 * For example, if a message contains an integer and a string, the returned Eina_Value
 * will be a struct: `{ "arg0": <integer_value>, "arg1": "<string_value>" }`.
 *
 * @param msg The Eldbus_Message to convert. Must not be NULL.
 * @return A new Eina_Value representing the message payload, or NULL on failure
 *         (e.g., if the message is NULL or has no arguments). The caller is
 *         responsible for freeing the returned Eina_Value.
 */
EAPI Eina_Value *
eldbus_message_to_eina_value(const Eldbus_Message *msg)
{
   Eldbus_Message_Iter *iter;
   EINA_SAFETY_ON_FALSE_RETURN_VAL(msg, NULL);
   iter = eldbus_message_iter_get(msg);
   EINA_SAFETY_ON_NULL_RETURN_VAL(iter, NULL);
   return _message_iter_struct_to_eina_value(iter);
}

/**
 * @brief Converts a D-Bus iterator pointing to a struct-like type (struct, dict entry, variant) to an Eina_Value.
 *
 * This function is similar to _message_iter_struct_to_eina_value but is exposed
 * as an EAPI function. It's useful when you have an iterator already positioned
 * at a D-Bus struct, dictionary entry ('e'), or variant ('v') and want to convert
 * that specific element to an Eina_Value struct.
 *
 * - For a D-Bus struct `(is)`: results in Eina_Value struct `{ "arg0": <int>, "arg1": <string> }`
 * - For a D-Bus dict entry `{is}` (type 'e'): results in Eina_Value struct `{ "arg0": <int_key>, "arg1": <string_value> }`
 * - For a D-Bus variant `v` containing `is`: results in Eina_Value struct `{ "arg0": <int>, "arg1": <string> }` (assuming the variant's content is a struct)
 *
 * @param iter The Eldbus_Message_Iter positioned at a D-Bus struct, dict entry, or variant. Must not be NULL.
 * @return A new Eina_Value of type EINA_VALUE_TYPE_STRUCT, or NULL on failure.
 *         The caller is responsible for freeing the returned Eina_Value.
 */
EAPI Eina_Value *
eldbus_message_iter_struct_like_to_eina_value(const Eldbus_Message_Iter *iter)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(iter, NULL);
   return _message_iter_struct_to_eina_value((Eldbus_Message_Iter *)iter);
}
