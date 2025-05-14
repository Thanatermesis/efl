
/**
 * @brief Internal constructor for Efl_Ui_Video_Legacy objects.
 *
 * @param[in] obj The object to construct.
 * @param[in] pd Private data for the object.
 * @return The new object instance.
 */
Efl_Object *_efl_ui_video_legacy_efl_object_constructor(Eo *obj, void *pd);

/**
 * @brief Initializes the Efl_Ui_Video_Legacy class.
 *
 * Sets up the operations and other class-specific data.
 *
 * @param klass The class to initialize.
 * @return @c EINA_TRUE on success, @c EINA_FALSE otherwise.
 */
static Eina_Bool
_efl_ui_video_legacy_class_initializer(Efl_Class *klass)
{
   const Efl_Object_Ops *opsp = NULL;

   const Efl_Object_Property_Reflection_Ops *ropsp = NULL;

#ifndef EFL_UI_VIDEO_LEGACY_EXTRA_OPS
#define EFL_UI_VIDEO_LEGACY_EXTRA_OPS
#endif

   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(efl_constructor, _efl_ui_video_legacy_efl_object_constructor),
      EFL_UI_VIDEO_LEGACY_EXTRA_OPS
   );
   opsp = &ops;

   return efl_class_functions_set(klass, opsp, ropsp);
}

/**
 * @brief Class description for Efl_Ui_Video_Legacy.
 *
 * This structure holds metadata for the Efl_Ui_Video_Legacy class,
 * including its version, name, type, and initializer functions.
 */
static const Efl_Class_Description _efl_ui_video_legacy_class_desc = {
   EO_VERSION, /**< EO_VERSION */
   "Efl.Ui.Video_Legacy", /**< Class name */
   EFL_CLASS_TYPE_REGULAR, /**< Class type */
   0,
   _efl_ui_video_legacy_class_initializer, /**< Class initializer */
   _efl_ui_video_legacy_class_constructor, /**< Class constructor */
   NULL /**< Class destructor */
};

/**
 * @brief Defines the Efl_Ui_Video_Legacy class.
 *
 * This macro effectively registers the Efl_Ui_Video_Legacy class with the Eo
 * system, specifying its description, parent class (EFL_UI_VIDEO_CLASS),
 * and any mixins (ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE).
 */
EFL_DEFINE_CLASS(efl_ui_video_legacy_class_get, &_efl_ui_video_legacy_class_desc, EFL_UI_VIDEO_CLASS, ELM_LAYOUT_MIXIN, EFL_UI_LEGACY_INTERFACE, NULL);
