/**
 * @brief Get the basic Evas_Image object from this object (widget).
 *
 * This function allows one to get the underlying Evas object of type Image
 * from this elementary widget. It can be useful to do things like get the
 * pixel data, save the image to a file, etc.
 *
 * @note Be careful to not manipulate it, as it is under control of elementary.
 *
 * @param[in] obj The object.
 *
 * @return The inlined image object or @c null.
 *
 * @ingroup Elm_Plug_Group
 */
EAPI Efl_Canvas_Object *
elm_plug_image_object_get(const Elm_Plug *obj)
{
   return elm_obj_plug_image_object_get(obj);
}
/**
 * @brief Connect a plug widget to service provided by socket image.
 *
 * @param[in] obj The object.
 * @param[in] svcname The service name to connect to set up by the socket.
 * @param[in] svcnum The service number to connect to (set up by socket).
 * @param[in] svcsys Boolean to set if the service is a system one or not (set
 * up by socket).
 *
 * @return @c true on success, @c false on error.
 *
 * @ingroup Elm_Plug_Group
 */
EAPI Eina_Bool
elm_plug_connect(Elm_Plug *obj, const char *svcname, int svcnum, Eina_Bool svcsys)
{
   return elm_obj_plug_connect(obj, svcname, svcnum, svcsys);
}
