#ifndef __EOLIAN_DATABASE_H
#define __EOLIAN_DATABASE_H

#include <setjmp.h>

#include <Eolian.h>

extern int _eolian_log_dom;
extern Eina_Prefix *_eolian_prefix;

#ifdef CRI
#undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_eolian_log_dom, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_eolian_log_dom, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_eolian_log_dom, __VA_ARGS__)

#ifdef INF
#undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_eolian_log_dom, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_eolian_log_dom, __VA_ARGS__)

/**
 * @struct _Eolian_Unit
 * @brief Represents a compilation unit in Eolian, typically corresponding to a single .eo or .eot file.
 *
 * An Eolian_Unit stores all Eolian objects (classes, typedecls, constants, etc.)
 * defined within that file. It also maintains links to its parent Eolian_State
 * and any child units (dependencies).
 */
struct _Eolian_Unit
{
   const char    *file; /**< The (short) name of the file this unit represents, e.g., "my_class.eo". Stringshared. */
   Eolian_State  *state; /**< Pointer to the global Eolian_State this unit belongs to. */
   Eina_Hash     *children; /**< Hash table of child Eolian_Unit structures, keyed by their file names. These are units included/imported by this unit. */
   Eina_Hash     *classes; /**< Hash table of Eolian_Class objects defined in this unit, keyed by their full names. */
   Eina_Hash     *constants; /**< Hash table of Eolian_Constant objects defined in this unit, keyed by their full names. */
   Eina_Hash     *errors; /**< Hash table of Eolian_Error objects defined in this unit, keyed by their full names. */
   Eina_Hash     *aliases; /**< Hash table of Eolian_Typedecl (aliases/typedefs) defined in this unit, keyed by their full names. */
   Eina_Hash     *structs; /**< Hash table of Eolian_Typedecl (structs) defined in this unit, keyed by their full names. */
   Eina_Hash     *enums; /**< Hash table of Eolian_Typedecl (enums) defined in this unit, keyed by their full names. */
   Eina_Hash     *objects; /**< Hash table of all Eolian_Object instances defined in this unit, keyed by their full names. This is a superset including objects from the above hashes. */
   unsigned short version; /**< The Eolian language version used by this unit. */
};

/**
 * @struct _Eolian_State_Area
 * @brief A container within Eolian_State for organizing Eolian objects, typically for different processing stages.
 *
 * An Eolian_State_Area holds a main Eolian_Unit (which acts as a global scope for that area)
 * and several hash tables that categorize Eolian objects by their type and the file they
 * originated from. This is used for both the 'main' (final, validated) database and the
 * 'staging' (intermediate parsing results) area.
 */
typedef struct _Eolian_State_Area
{
   Eolian_Unit unit; /**< The main, virtual unit for this area. Objects not belonging to a specific file unit might be placed here. */

   Eina_Hash *units; /**< Hash table of all Eolian_Unit structures in this area, keyed by their file names. */

   Eina_Hash *classes_f;   /**< Hash table mapping file names (stringshared) to the Eolian_Class defined in that file. Assumes one class per .eo file. */
   Eina_Hash *aliases_f;   /**< Hash table mapping file names (stringshared) to Eina_List of Eolian_Typedecl (aliases) defined in that file. */
   Eina_Hash *structs_f;   /**< Hash table mapping file names (stringshared) to Eina_List of Eolian_Typedecl (structs) defined in that file. */
   Eina_Hash *enums_f;     /**< Hash table mapping file names (stringshared) to Eina_List of Eolian_Typedecl (enums) defined in that file. */
   Eina_Hash *constants_f; /**< Hash table mapping file names (stringshared) to Eina_List of Eolian_Constant objects defined in that file. */
   Eina_Hash *errors_f;    /**< Hash table mapping file names (stringshared) to Eina_List of Eolian_Error objects defined in that file. */
   Eina_Hash *objects_f;   /**< Hash table mapping file names (stringshared) to Eina_List of all Eolian_Object instances defined in that file. */
} Eolian_State_Area;

/**
 * @struct _Eolian_State
 * @brief Represents the global state of an Eolian parsing session.
 *
 * The Eolian_State holds all parsed data, divided into a 'main' area for
 * validated objects and a 'staging' area for intermediate results. It also
 * manages error and panic handling, lists of known .eo/.eot files, and
 * deferred parsing tasks.
 */
struct _Eolian_State
{
   Eolian_State_Area main;    /**< Area for storing the final, validated Eolian database. */
   Eolian_State_Area staging; /**< Area for intermediate parsing results before validation and merging. */

   Eolian_Panic_Cb panic;      /**< Callback function for fatal errors. */
   Eina_Stringshare *panic_msg; /**< Message for the current panic, if any. */
   jmp_buf jmp_env;            /**< Jump buffer for longjmp on panic. */

   Eolian_Error_Cb error;      /**< Callback function for recoverable errors and warnings. */
   void *error_data;           /**< User data for the error callback. */

   Eina_Hash *filenames_eo;  /**< Hash table mapping short .eo filenames to their full paths. Keys are char*, values are char* (owned by hash). */
   Eina_Hash *filenames_eot; /**< Hash table mapping short .eot filenames to their full paths. Keys are char*, values are char* (owned by hash). */

   Eina_Hash *defer; /**< Hash table of filenames (char*) that need to be parsed. Values indicate if they are direct dependencies. */
};

/**
 * @struct _Eolian_Object
 * @brief Base structure for all Eolian language constructs.
 *
 * This structure provides common properties for all Eolian elements like
 * classes, functions, types, etc. It includes information about the object's
 * origin (file, line, column), its name, C representation name, type,
 * and state flags like validation status and beta status. It also includes
 * a reference counter for memory management.
 */
struct _Eolian_Object
{
   Eolian_Unit *unit;        /**< Pointer to the Eolian_Unit this object belongs to. */
   Eina_Stringshare *file;   /**< The (short) name of the file where this object is defined. Stringshared. */
   Eina_Stringshare *name;   /**< The fully qualified Eolian name of the object (e.g., "My.Namespace.MyClass"). Stringshared. */
   Eina_Stringshare *c_name; /**< The C language equivalent name for this object (e.g., "my_namespace_myclass_get"). Stringshared. */
   int line;                 /**< Line number where the object is defined in its source file. */
   int column;               /**< Column number where the object is defined in its source file. */
   int refcount;             /**< Reference count for managing the object's lifecycle. */
   Eolian_Object_Type type;   /**< The type of this Eolian object (e.g., class, function, typedecl). */
   Eina_Bool validated :1;   /**< Flag indicating if this object has passed validation checks. */
   Eina_Bool is_beta   :1;   /**< Flag indicating if this object is marked as beta. */
};

/**
 * @internal
 * @brief Increments the reference count of an Eolian_Object.
 * @param obj The Eolian_Object to ref.
 */
static inline void
eolian_object_ref(Eolian_Object *obj)
{
   ++obj->refcount;
}

/**
 * @internal
 * @brief Decrements the reference count of an Eolian_Object.
 * @param obj The Eolian_Object to unref.
 * @return EINA_TRUE if the object still has references, EINA_FALSE otherwise (implying it might be freed).
 */
static inline Eina_Bool
eolian_object_unref(Eolian_Object *obj)
{
   return (--obj->refcount > 0);
}

/**
 * @internal
 * @brief Adds an Eolian_Object to a hash and increments its reference count.
 * @param obj The Eolian_Object to add.
 * @param name The name (key) for the hash.
 * @param hash The Eina_Hash to add the object to.
 */
static inline void
eolian_object_add(Eolian_Object *obj, Eina_Stringshare *name, Eina_Hash *hash)
{
   eina_hash_add(hash, name, obj);
   eolian_object_ref(obj);
}

/**
 * @internal
 * @brief Macro to add an Eolian object to both the staging unit's member hash and the target unit's member hash.
 * This is a convenience macro used when adding typed objects (like classes, constants) to their respective
 * collections in both the current parsing unit and the global staging unit.
 * @param tunit The target Eolian_Unit.
 * @param name The name of the object.
 * @param obj Pointer to the Eolian object (e.g., Eolian_Class*, Eolian_Constant*). Its `base` member is used.
 * @param memb The member name in Eolian_Unit and Eolian_State_Area.unit that holds the hash for this object type (e.g., `classes`, `constants`).
 */
#define EOLIAN_OBJECT_ADD(tunit, name, obj, memb) \
{ \
   eolian_object_add(&obj->base, name, tunit->state->staging.unit.memb); \
   eolian_object_add(&obj->base, name, tunit->memb); \
}

/**
 * @internal
 * @brief Logs a message associated with an Eolian_State using a va_list.
 * This function formats a message and passes it to the state's error callback.
 * @param state The Eolian_State.
 * @param obj Optional Eolian_Object associated with the message (for location info).
 * @param fmt The format string.
 * @param args The va_list of arguments for the format string.
 */
static inline void eolian_state_vlog(const Eolian_State *state, const Eolian_Object *obj, const char *fmt, va_list args) EINA_ARG_NONNULL(1, 3);

/**
 * @internal
 * @brief Logs a general message associated with an Eolian_State.
 * @param state The Eolian_State.
 * @param fmt The format string.
 * @param ... Arguments for the format string.
 */
static inline void eolian_state_log(const Eolian_State *state, const char *fmt, ...) EINA_ARG_NONNULL(1, 2) EINA_PRINTF(2, 3);

/**
 * @internal
 * @brief Logs a message associated with a specific Eolian_Object within an Eolian_State.
 * This allows error messages to include file, line, and column information from the object.
 * @param state The Eolian_State.
 * @param obj The Eolian_Object related to the message.
 * @param fmt The format string.
 * @param ... Arguments for the format string.
 */
static inline void eolian_state_log_obj(const Eolian_State *state, const Eolian_Object *obj, const char *fmt, ...) EINA_ARG_NONNULL(1, 2, 3) EINA_PRINTF(3, 4);

/**
 * @internal
 * @brief Triggers a panic state in Eolian.
 * This function formats a panic message, stores it in the Eolian_State, and performs a longjmp
 * to unwind the stack, typically leading to program termination via the panic callback.
 * @param state The Eolian_State.
 * @param fmt The format string for the panic message.
 * @param ... Arguments for the format string.
 */
static inline void eolian_state_panic(Eolian_State *state, const char *fmt, ...) EINA_ARG_NONNULL(1, 2) EINA_PRINTF(2, 3);

static inline void
eolian_state_vlog(const Eolian_State *state, const Eolian_Object *obj,
                 const char *fmt, va_list args)
{
   Eina_Strbuf *sb = eina_strbuf_new();
   eina_strbuf_append_vprintf(sb, fmt, args);
   state->error(obj, eina_strbuf_string_get(sb), state->error_data);
   eina_strbuf_free(sb);
}

static inline void
eolian_state_log(const Eolian_State *state, const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   eolian_state_vlog(state, NULL, fmt, args);
   va_end(args);
}

static inline void
eolian_state_log_obj(const Eolian_State *state, const Eolian_Object *obj,
                     const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   eolian_state_vlog(state, obj, fmt, args);
   va_end(args);
}

static inline void
eolian_state_panic(Eolian_State *state, const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   state->panic_msg = eina_stringshare_vprintf(fmt, args);
   va_end(args);
   longjmp(state->jmp_env, 1);
}

/**
 * @struct _Eolian_Documentation
 * @brief Represents documentation associated with an Eolian object.
 *
 * Stores the summary, detailed description, and "since" version for an
 * Eolian element. It also includes a list for debug references.
 */
struct _Eolian_Documentation
{
   Eolian_Object base;      /**< Base Eolian_Object information. */
   Eina_Stringshare *summary; /**< A brief summary of the documented element. Stringshared. */
   Eina_Stringshare *description; /**< A more detailed description. Stringshared. */
   Eina_Stringshare *since;   /**< Version string indicating when the element was added or last significantly changed. Stringshared. */
   Eina_List *ref_dbg;      /**< List used for debugging references within documentation. */
};

/**
 * @struct _Eolian_Class
 * @brief Represents an Eolian class definition.
 *
 * Contains all information about a class, including its type (regular, interface, mixin),
 * documentation, C code generation prefixes, parent class, extended interfaces,
 * properties, methods, implemented functions, constructors, events, parts, and composite classes.
 */
struct _Eolian_Class
{
   Eolian_Object base;        /**< Base Eolian_Object information. */
   Eolian_Class_Type type;    /**< The type of the class (e.g., regular, interface, mixin). */
   Eolian_Documentation *doc; /**< Documentation for the class. */
   Eina_Stringshare *c_prefix; /**< C prefix for functions generated from this class (e.g., "my_class"). Stringshared. */
   Eina_Stringshare *ev_prefix; /**< Prefix for event names generated from this class. Stringshared. */
   Eina_Stringshare *data_type; /**< Custom data type associated with this class, if any. Stringshared. */
   union {
      Eolian_Class *parent;        /**< Pointer to the parent Eolian_Class, if resolved. */
      Eina_Stringshare *parent_name; /**< Name of the parent class, before resolution. Stringshared. */
   };
   Eina_List *extends;      /**< List of Eolian_Class objects that this class extends (interfaces/mixins). */
   Eina_List *properties;   /**< List of Eolian_Function objects representing properties. */
   Eina_List *methods;      /**< List of Eolian_Function objects representing methods. */
   Eina_List *implements;   /**< List of Eolian_Implement objects detailing implemented functions/properties. */
   Eina_List *constructors; /**< List of Eolian_Constructor objects for this class. */
   Eina_List *events;       /**< List of Eolian_Event objects declared by this class. */
   Eina_List *parts;        /**< List of Eolian_Part objects belonging to this class. */
   Eina_List *composite;    /**< List of Eolian_Class objects that are part of this class's composition. */
   Eina_List *requires;     /**< Internal list of other classes required by this class. */
   Eina_List *callables;    /**< Internal list of callable functions/methods. */
   Eina_Bool class_ctor_enable:1; /**< Flag indicating if a class constructor is enabled. */
   Eina_Bool class_dtor_enable:1; /**< Flag indicating if a class destructor is enabled. */
};

/**
 * @struct _Eolian_Function
 * @brief Represents an Eolian function, method, or property accessor.
 *
 * This structure holds details about a function, including its parameters,
 * return type, scope, and associated documentation. For properties, it
 * distinguishes between get and set accessors.
 */
struct _Eolian_Function
{
   Eolian_Object base;     /**< Base Eolian_Object information for the function (often the 'get' part for properties). */
   Eolian_Object set_base; /**< Base Eolian_Object information specifically for the 'set' part of a property. */
   union { /* lists of Eolian_Function_Parameter */
       Eina_List *params; /**< List of Eolian_Function_Parameter for methods or simple functions. */
       struct {
           Eina_List *prop_values;     /**< Parameters for property values (generic). */
           Eina_List *prop_values_get; /**< Parameters for property 'get' accessor values. */
           Eina_List *prop_values_set; /**< Parameters for property 'set' accessor values. */
           Eina_List *prop_keys;       /**< Parameters for property keys (generic). */
           Eina_List *prop_keys_get;   /**< Parameters for property 'get' accessor keys. */
           Eina_List *prop_keys_set;   /**< Parameters for property 'set'accessor keys. */
       };
   };
   Eolian_Function_Type type;      /**< Type of the function (method, property get, property set). */
   Eolian_Object_Scope get_scope; /**< Scope (public, private, protected, etc.) of the 'get' accessor or method. */
   Eolian_Object_Scope set_scope; /**< Scope of the 'set' accessor. */
   Eolian_Type *get_ret_type;     /**< Return type of the 'get' accessor or method. */
   Eolian_Type *set_ret_type;     /**< Return type of the 'set' accessor (often void or bool). */
   Eolian_Expression *get_ret_val;/**< Default/constant return value for the 'get' accessor, if any. */
   Eolian_Expression *set_ret_val;/**< Default/constant return value for the 'set' accessor, if any. */
   Eolian_Implement *impl;        /**< Pointer to the Eolian_Implement data if this function is an implementation. */
   Eolian_Documentation *get_return_doc; /**< Documentation for the return value of the 'get' accessor/method. */
   Eolian_Documentation *set_return_doc; /**< Documentation for the return value of the 'set' accessor. */
   Eina_List *ctor_of;            /**< List of classes for which this function acts as a constructor. */
   Eolian_Class *klass;           /**< The class this function belongs to. */
   Eina_Bool obj_is_const :1;     /**< True if the object instance ('this') is const for this method. */
   Eina_Bool get_return_no_unused :1; /**< If true, indicates the return value of 'get' should not be unused (e.g., [[nodiscard]]). */
   Eina_Bool set_return_no_unused :1; /**< If true, indicates the return value of 'set' should not be unused. */
   Eina_Bool get_return_move      :1; /**< If true, the return value of 'get' should be moved. */
   Eina_Bool set_return_move      :1; /**< If true, the return value of 'set' should be moved. */
   Eina_Bool get_return_by_ref    :1; /**< If true, the return value of 'get' is by reference. */
   Eina_Bool set_return_by_ref    :1; /**< If true, the return value of 'set' is by reference. */
   Eina_Bool is_static :1;          /**< If true, this is a static function/method. */
};

/**
 * @struct _Eolian_Part
 * @brief Represents a "part" in an Eolian class, typically a named instance of another class.
 *
 * Parts are components that make up a more complex Eolian class. This structure
 * stores the part's name, its class (or class name before resolution), and documentation.
 */
struct _Eolian_Part
{
   Eolian_Object base; /**< Base Eolian_Object information (name of the part). */
   /* when not validated, class name is stored */
   union
   {
      Eina_Stringshare *klass_name; /**< Name of the part's class, before resolution. Stringshared. */
      Eolian_Class *klass;          /**< Pointer to the Eolian_Class of the part, after resolution. */
   };
   Eolian_Documentation *doc; /**< Documentation for this part. */
};

/**
 * @struct _Eolian_Function_Parameter
 * @brief Represents a parameter of an Eolian function, method, or property.
 *
 * Stores the parameter's name, type, direction (in, out, inout),
 * default value (if any), and documentation. Also flags for optional,
 * by-reference, and move semantics.
 */
struct _Eolian_Function_Parameter
{
   Eolian_Object base;              /**< Base Eolian_Object information (name of the parameter). */
   Eolian_Type *type;               /**< The Eolian_Type of the parameter. */
   Eolian_Expression *value;        /**< Default value for the parameter, if any, as an Eolian_Expression. */
   Eolian_Documentation *doc;       /**< Documentation for this parameter. */
   Eolian_Parameter_Direction param_dir; /**< Direction of the parameter (in, out, inout). */
   Eina_Bool optional :1;           /**< True if this parameter is optional. */
   Eina_Bool by_ref   :1;           /**< True if this parameter is passed by reference. */
   Eina_Bool move     :1;           /**< True if this parameter should be moved. */
};

typedef enum
{
   EOLIAN_C_TYPE_DEFAULT = 0,
   EOLIAN_C_TYPE_PARAM,
   EOLIAN_C_TYPE_RETURN /**< Type is being used as a return value. */
} Eolian_C_Type_Type;

/**
 * @struct _Eolian_Type
 * @brief Represents a data type in Eolian.
 *
 * This can be a basic built-in type, a user-defined type (class, struct, enum, alias),
 * an error type, or a pointer/iterator to another type. It stores information
 * about constness, pointer status, and move semantics.
 */
struct _Eolian_Type
{
   Eolian_Object base;             /**< Base Eolian_Object information (usually name of the type if it's a named type, or derived). */
   Eolian_Type_Type type;          /**< The general category of the type (e.g., regular, class, void, future, etc.). */
   Eolian_Type_Builtin_Type btype; /**< If it's a built-in type, its specific kind (e.g., int, float, string). */
   Eolian_Type *base_type;        /**< For complex types like pointers or iterators, this points to the underlying type. */
   Eolian_Type *next_type;        /**< For types like Eina_Iterator<T1, T2>, this points to the next type argument (T2). */
   union
   {
      Eolian_Class *klass;    /**< If type is EOLIAN_TYPE_CLASS, points to the Eolian_Class. */
      Eolian_Typedecl *tdecl; /**< If type is EOLIAN_TYPE_TYPEDECL (struct, enum, alias), points to Eolian_Typedecl. */
      Eolian_Error *error;    /**< If type is EOLIAN_TYPE_ERROR, points to Eolian_Error. */
   };
   Eina_Bool is_const  :1; /**< True if the type is const-qualified. */
   Eina_Bool is_ptr    :1; /**< True if the type is a pointer (distinct from object types which are implicitly reference types). */
   Eina_Bool move      :1; /**< True if values of this type should be moved. */
   Eina_Bool ownable   :1; /**< True if this type represents an ownable resource (e.g. an Eolian object, or a string that needs freeing). */
};

/**
 * @struct _Eolian_Typedecl
 * @brief Represents a user-defined type declaration in Eolian (struct, enum, alias/typedef, or function pointer).
 *
 * Stores the kind of type declaration, its underlying base type (for aliases),
 * fields (for structs and enums), associated documentation, and other metadata
 * like legacy names or custom free functions.
 */
struct _Eolian_Typedecl
{
   Eolian_Object base;          /**< Base Eolian_Object information (name of the typedecl). */
   Eolian_Typedecl_Type type;   /**< The kind of type declaration (struct, enum, alias, funcptr). */
   Eolian_Type      *base_type;/**< For aliases, the Eolian_Type it refers to. */
   Eina_Hash        *fields;   /**< For structs/enums, a hash of Eolian_Struct_Type_Field or Eolian_Enum_Type_Field, keyed by field name. */
   Eina_List        *field_list;/**< For structs/enums, an ordered list of their fields. */
   Eolian_Function *function_pointer; /**< For function pointer typedefs, the signature. */
   Eolian_Documentation *doc;   /**< Documentation for this type declaration. */
   Eina_Stringshare *legacy;    /**< Legacy name for this type, if any. Stringshared. */
   Eina_Stringshare *freefunc;  /**< Custom free function for this type, if any. Stringshared. */
   Eina_Bool is_extern :1;      /**< True if this is an external type declaration (body defined elsewhere). */
   Eina_Bool ownable :1;        /**< True if this type (typically a struct) is considered ownable and needs explicit memory management. */
};

/**
 * @struct _Eolian_Implement
 * @brief Represents the implementation of a function or property from an interface or parent class.
 *
 * Stores information about which class is implementing a feature from another class (implklass).
 * It can have its own documentation, overriding or supplementing the original.
 * Flags indicate if it's for a property's get/set accessor and if it's pure virtual, auto-generated, or empty.
 */
struct _Eolian_Implement
{
   Eolian_Object base;          /**< Base Eolian_Object information (name of the implemented function/property). */
   const Eolian_Class *klass;     /**< The class that contains this implementation statement. */
   const Eolian_Class *implklass; /**< The class (interface/parent) from which the implemented feature originates. */
   const Eolian_Function *foo_id; /**< Pointer to the Eolian_Function being implemented. */
   Eolian_Documentation *common_doc; /**< Common documentation for this implementation (applies to both get/set if it's a property). */
   Eolian_Documentation *get_doc;    /**< Specific documentation for the 'get' accessor of an implemented property. */
   Eolian_Documentation *set_doc;    /**< Specific documentation for the 'set' accessor of an implemented property. */
   Eina_Bool is_prop_get :1;      /**< True if this implements the 'get' part of a property. */
   Eina_Bool is_prop_set :1;      /**< True if this implements the 'set' part of a property. */
   Eina_Bool get_pure_virtual :1; /**< True if the 'get' accessor is declared pure virtual. */
   Eina_Bool set_pure_virtual :1; /**< True if the 'set' accessor is declared pure virtual. */
   Eina_Bool get_auto: 1;         /**< True if the 'get' accessor is auto-generated by the compiler. */
   Eina_Bool set_auto: 1;         /**< True if the 'set' accessor is auto-generated by the compiler. */
   Eina_Bool get_empty: 1;        /**< True if the 'get' accessor is an empty (no-op) implementation. */
   Eina_Bool set_empty: 1;        /**< True if the 'set' accessor is an empty (no-op) implementation. */
};

/**
 * @struct _Eolian_Constructor
 * @brief Represents a constructor for an Eolian class.
 *
 * Links a function (identified by base.name) to a class as one of its constructors.
 * Can be marked as optional.
 */
struct _Eolian_Constructor
{
   Eolian_Object base;      /**< Base Eolian_Object information (name of the constructor function). */
   const Eolian_Class *klass; /**< The class this constructor belongs to. */
   Eina_Bool is_optional: 1;/**< True if this constructor is optional. */
};

/**
 * @struct _Eolian_Event
 * @brief Represents an event that can be emitted by an Eolian class.
 *
 * Stores the event's name, documentation, the type of data associated with the event (if any),
 * the class it belongs to, its scope, and flags for 'hot' (emitted frequently) or 'restart' behavior.
 */
struct _Eolian_Event
{
   Eolian_Object base;        /**< Base Eolian_Object information (name of the event). */
   Eolian_Documentation *doc; /**< Documentation for this event. */
   Eolian_Type *type;         /**< The Eolian_Type of data associated with this event (can be NULL for no data). */
   Eolian_Class *klass;       /**< The class that declares this event. */
   Eolian_Object_Scope scope; /**< Scope of the event (public, private, etc.). */
   Eina_Bool is_hot  :1;      /**< True if this event is considered "hot" (frequently emitted). */
   Eina_Bool is_restart :1;   /**< True if this event can restart certain operations. */
};

/**
 * @struct _Eolian_Error
 * @brief Represents a named error type in Eolian.
 *
 * Errors can be returned by functions to indicate failure conditions. This structure
 * stores the error's name, a descriptive message, documentation, and a flag
 * indicating if it's an externally defined error.
 */
struct _Eolian_Error
{
   Eolian_Object base;      /**< Base Eolian_Object information (name of the error). */
   Eina_Stringshare *msg;   /**< A descriptive message for this error. Stringshared. */
   Eolian_Documentation *doc; /**< Documentation for this error. */
   Eina_Bool is_extern :1;  /**< True if this error is defined externally (e.g., in C code). */
};

/**
 * @struct _Eolian_Struct_Type_Field
 * @brief Represents a field within an Eolian struct type.
 *
 * Stores the field's name, its Eolian_Type, documentation, and flags
 * for move semantics or by-reference passing (though by-reference is less common for struct fields).
 */
struct _Eolian_Struct_Type_Field
{
   Eolian_Object     base; /**< Base Eolian_Object information (name of the field). */
   Eolian_Type      *type; /**< The Eolian_Type of this field. */
   Eolian_Documentation *doc; /**< Documentation for this field. */
   Eina_Bool move     :1;  /**< True if this field's value should be moved. */
   Eina_Bool by_ref   :1;  /**< True if this field is accessed by reference (uncommon for direct fields). */
};

/**
 * @struct _Eolian_Enum_Type_Field
 * @brief Represents a field (member) within an Eolian enum type.
 *
 * Stores the enum field's name, a pointer to its parent enum declaration,
 * its associated value (as an Eolian_Expression), documentation, and a flag
 * indicating if its value is explicitly public (used for C generation).
 */
struct _Eolian_Enum_Type_Field
{
   Eolian_Object      base;      /**< Base Eolian_Object information (name of the enum field). */
   Eolian_Typedecl   *base_enum; /**< Pointer to the Eolian_Typedecl (enum) this field belongs to. */
   Eolian_Expression *value;    /**< The value of this enum field, represented as an Eolian_Expression. */
   Eolian_Documentation *doc;   /**< Documentation for this enum field. */
   Eina_Bool is_public_value :1;/**< True if the value of this enum field is considered public for C API generation. */
};

/**
 * @struct _Eolian_Expression
 * @brief Represents an expression in Eolian, used for constant values, enum field values, etc.
 *
 * Can be a literal value (integer, string, boolean, etc.), a reference to another
 * Eolian object (like a constant or enum field), a unary operation, or a binary operation.
 */
struct _Eolian_Expression
{
   Eolian_Object base;          /**< Base Eolian_Object information (often derived or not directly named). */
   Eolian_Expression_Type type;/**< The type of the expression (literal, unary op, binary op, reference). */
   union
   {
      /** @brief Structure for binary operations. */
      struct
      {
         Eolian_Binary_Operator binop; /**< The binary operator (e.g., +, -, *, |, &). */
         Eolian_Expression *lhs;      /**< Left-hand side expression. */
         Eolian_Expression *rhs;      /**< Right-hand side expression. */
      };
      /** @brief Structure for unary operations and literal values. */
      struct
      {
         union
         {
            Eolian_Unary_Operator unop; /**< The unary operator (e.g., -, !, ~). */
            Eolian_Value_Union value;   /**< If type is EOLIAN_EXPR_LITERAL, this holds the actual value. */
         };
         Eolian_Expression *expr;     /**< The sub-expression for unary operations. */
      };
   };
   Eina_Bool weak_lhs :1; /**< For binary operations, indicates if the left-hand side is a weak reference (not yet fully resolved or validated). */
   Eina_Bool weak_rhs :1; /**< For binary operations, indicates if the right-hand side is a weak reference. */
};

/**
 * @struct _Eolian_Constant
 * @brief Represents a named constant in Eolian.
 *
 * Stores the constant's name, its base type, its value (as an Eolian_Expression),
 * documentation, and a flag indicating if it's an externally defined constant.
 */
struct _Eolian_Constant
{
   Eolian_Object         base;      /**< Base Eolian_Object information (name of the constant). */
   Eolian_Type          *base_type; /**< The Eolian_Type of this constant. */
   Eolian_Expression    *value;    /**< The value of this constant, represented as an Eolian_Expression. */
   Eolian_Documentation *doc;       /**< Documentation for this constant. */
   Eina_Bool is_extern :1;          /**< True if this constant is defined externally (e.g., in C code). */
};

char *database_class_to_filename(const char *cname);
Eina_Bool database_validate(const Eolian_Unit *src);
Eina_Bool database_check(const Eolian_State *state);
/* if isdep is EINA_TRUE, parse as a dependency of current unit */
void database_defer(Eolian_State *state, const char *fname, Eina_Bool isdep);

/**
 * @internal
 * @brief Adds a newly created Eolian object to the staging area of the database.
 *
 * This function is called during parsing to register a new object (like a
 * class, constant, etc.) in the appropriate hashes within the staging area of
 * the Eolian state. This includes adding it to the global object list of the
 * staging unit and the per-file object list.
 *
 * @param unit The compilation unit where the object was defined.
 * @param obj The Eolian object to add.
 */
void database_object_add(Eolian_Unit *unit, const Eolian_Object *obj);

/**
 * @internal
 * @brief Frees the memory allocated for an Eolian_Documentation object.
 *
 * This function safely deallocates an Eolian_Documentation structure and all
 * its associated stringshared members (summary, description, since). It also
 * frees the list of debug references.
 *
 * @param doc The documentation object to delete.
 */
void database_doc_del(Eolian_Documentation *doc);

/**
 * @internal
 * @brief Initializes an Eolian_Unit structure.
 *
 * Allocates and initializes hash tables for all the Eolian object types
 * that can be contained within a unit. Also sets the unit's file, state,
 * and default language version.
 *
 * @param state The global Eolian_State this unit belongs to.
 * @param unit The Eolian_Unit to initialize.
 * @param file The filename this unit represents. Can be NULL for virtual units.
 */
void database_unit_init(Eolian_State *state, Eolian_Unit *unit, const char *file);
/**
 * @internal
 * @brief Frees an Eolian_Unit structure and its contents.
 *
 * This function deallocates an Eolian_Unit that was allocated with `malloc`.
 * It frees all the hash tables, but the objects within them are managed by
 * their reference counts and are freed via hash free callbacks.
 *
 * @param unit The Eolian_Unit to delete.
 */
void database_unit_del(Eolian_Unit *unit);

/**
 * @internal
 * @brief Resolves a documentation reference token to an Eolian object.
 *
 * This function takes a token of type #EOLIAN_DOC_TOKEN_REF and tries to find
 * the Eolian object it refers to. It can resolve references to classes,
 * typedecls, constants, errors, functions, struct fields, enum fields, and events.
 *
 * @param tok The reference token to resolve.
 * @param unit1 The primary compilation unit to search for the object.
 * @param unit2 An optional secondary unit to search if not found in the first.
 * @param data1 On success, this will point to the resolved Eolian_Object. For
 *              members (functions, fields, events), this is the containing object.
 * @param data2 For members (functions, fields, events), this points to the
 *              specific member object. Otherwise, it is unused.
 * @return The Eolian_Object_Type of the resolved object, or #EOLIAN_OBJECT_UNKNOWN
 *         if the reference could not be resolved.
 */
Eolian_Object_Type database_doc_token_ref_resolve(const Eolian_Doc_Token *tok,
                                                  const Eolian_Unit *unit1,
                                                  const Eolian_Unit *unit2,
                                                  const Eolian_Object **data1,
                                                  const Eolian_Object **data2);

/* types */

void database_type_add(Eolian_Unit *unit, Eolian_Typedecl *tp);
void database_struct_add(Eolian_Unit *unit, Eolian_Typedecl *tp);
void database_enum_add(Eolian_Unit *unit, Eolian_Typedecl *tp);
void database_type_del(Eolian_Type *tp);
void database_typedecl_del(Eolian_Typedecl *tp);

void database_type_to_str(const Eolian_Type *tp, Eina_Strbuf *buf, const char *name, Eolian_C_Type_Type ctype, Eina_Bool by_ref);
void database_typedecl_to_str(const Eolian_Typedecl *tp, Eina_Strbuf *buf);

Eolian_Typedecl *database_type_decl_find(const Eolian_Unit *src, const Eolian_Type *tp);

Eina_Bool database_type_is_ownable(const Eolian_Unit *unit, const Eolian_Type *tp, Eina_Bool allow_void, const Eolian_Type **otp);

/* expressions */

typedef void (*Expr_Obj_Cb)(const Eolian_Object *obj, void *data);

Eolian_Value database_expr_eval(const Eolian_Unit *unit, Eolian_Expression *expr, Eolian_Expression_Mask mask, Expr_Obj_Cb cb, void *data);
Eolian_Value database_expr_eval_type(const Eolian_Unit *unit, Eolian_Expression *expr, const Eolian_Type *type, Expr_Obj_Cb cb, void *data);
void database_expr_del(Eolian_Expression *expr);
void database_expr_print(Eolian_Expression *expr);

/* variables */

void database_constant_del(Eolian_Constant *var);
void database_constant_add(Eolian_Unit *unit, Eolian_Constant *var);

/* classes */
void database_class_del(Eolian_Class *cl);

/* functions */
void database_function_del(Eolian_Function *fid);
void database_function_constructor_add(Eolian_Function *func, const Eolian_Class *klass);
Eina_Bool database_function_is_type(Eolian_Function *fid, Eolian_Function_Type ftype);

/* func parameters */
void database_parameter_del(Eolian_Function_Parameter *pdesc);

/* implements */
void database_implement_del(Eolian_Implement *impl);

/* constructors */
void database_constructor_del(Eolian_Constructor *ctor);

/* events */
void database_event_del(Eolian_Event *event);

/* parts */
void database_part_del(Eolian_Part *part);

/* errors */
void database_error_del(Eolian_Error *err);
void database_error_add(Eolian_Unit *unit, Eolian_Error *err);

#endif
