#include "edje_private.h"

/*
 * ALREADY EXPORTED BY EMBRYO:
 *
 * enum Float_Round_Method {
 *    ROUND, FLOOR, CEIL, TOZERO
 * };
 * enum Float_Angle_Mode {
 *    RADIAN, DEGREES, GRADES
 * };
 *
 * numargs();
 * getarg(arg, index=0);
 * setarg(arg, index=0, value);
 *
 * Float:atof(string[]);
 * Float:fract(Float:value);
 *       round(Float:value, Float_Round_Method:method=ROUND);
 * Float:sqrt(Float:value);
 * Float:pow(Float:value, Float:exponent);
 * Float:log(Float:value, Float:base=10.0);
 * Float:sin(Float:value, Float_Angle_Mode:mode=RADIAN);
 * Float:cos(Float:value, Float_Angle_Mode:mode=RADIAN);
 * Float:tan(Float:value, Float_Angle_Mode:mode=RADIAN);
 * Float:abs(Float:value);
 *       atoi(str[]);
 *       fnmatch(glob[], str[]);
 *       strcmp(str1[], str2[]);
 *       strncmp(str1[], str2[]);
 *       strcpy(dst[], src[]);
 *       strncpy(dst[], src[], n);
 *       strlen(str[]);
 *       strcat(dst[], src[]);
 *       strncat(dst[], src[], n);
 *       strprep(dst[], src[]);
 *       strnprep(dst[], src[], n);
 *       strcut(dst[], str[], n, n2);
 *       snprintf(dst[], dstn, fmt[], ...);
 *       strstr(str[], ndl[]);
 *       strchr(str[], ch[]);
 *       strrchr(str[], ch[]);
 *       rand();
 * Float:randf();
 * Float:seconds();
 *       date(&year, &month, &day, &yearday, &weekday, &hr, &min, &Float:sec);
 *
 */

/* EDJE...
 *
 * implemented so far as examples:
 *
 * enum Msg_Type {
 *    MSG_NONE, MSG_STRING, MSG_INT, MSG_FLOAT, MSG_STRING_SET, MSG_INT_SET,
 *    MSG_FLOAT_SET, MSG_STRING_INT, MSG_INT_FLOAT, MSG_STRING_INT_SET,
 *    MSG_INT_FLOAT_SET
 * };
 *
 * get_int(id)
 * set_int(id, v)
 * Float:get_float (id)
 * set_float(id, Float:v)
 * get_strlen(id)
 * get_str(id, dst[], maxlen)
 * set_str(id, str[])
 * timer(Float:in, fname[], val)
 * cancel_timer(id)
 * reset_timer(id)
 * anim(Float:len, fname[], val)
 * cancel_anim(id)
 * emit(sig[], src[])
 * set_state(part_id, state[], Float:state_val)
 * get_state(part_id, dst[], maxlen, &Float:val)
 * set_tween_state(part_id, Float:tween, state1[], Float:state1_val, state2[], Float:state2_val)
 * play_sample(sample_name, speed, ...)
 * play_tone(tone_name, duration, ...)
 * play_vibration(sample_name, repeat)
 * run_program(program_id)
 * Direction:get_drag_dir(part_id)
 * get_drag(part_id, &Float:dx, &Float:&dy)
 * set_drag(part_id, Float:dx, Float:dy)
 * get_drag_size(part_id, &Float:dx, &Float:&dy)
 * set_drag_size(part_id, Float:dx, Float:dy)
 * set_text(part_id, str[])
 * get_text(part_id, dst[], maxlen)
 * get_min_size(w, h)
 * get_max_size(w, h)
 * set_color_class(class[], r, g, b, a)
 * get_color_class(class[], &r, &g, &b, &a)
 * set_text_class(class[], font[], Float:size)
 * get_text_class(class[], font[], &Float:size)
 * get_drag_step(part_id, &Float:dx, &Float:&dy)
 * set_drag_step(part_id, Float:dx, Float:dy)
 * get_drag_page(part_id, &Float:dx, &Float:&dy)
 * set_drag_page(part_id, Float:dx, Float:dy)
 * get_geometry(part_id, &x, &y, &w, &h)
 * get_mouse(&x, &y)
 * stop_program(program_id)
 * stop_programs_on(part_id)
 * set_min_size(w, h)
 * set_max_size(w, h)
 * send_message(Msg_Type:type, id, ...)
 *
 * count(id)
 * remove(id, n)
 *
 * append_int(id, v)
 * prepend_int(id, v)
 * insert_int(id, n, v)
 * replace_int(id, n, v)
 * fetch_int(id, n)
 *
 * append_str(id, str[])
 * prepend_str(id, str[])
 * insert_str(id, n, str[])
 * replace_str(id, n, str[])
 * fetch_str(id, n, dst[], maxlen)
 *
 * append_float(id, Float:v)
 * prepend_float(id, Float:v)
 * insert_float(id, n, Float:v)
 * replace_float(id, n, Float:v)
 * Float:fetch_float(id, n)
 *
 * custom_state(part_id, state[], Float:state_val = 0.0)
 * set_state_val(part_id, State_Param:param, ...)
 * get_state_val(part_id, State_Param:param, ...)
 *
 * Supported parameters:
 * align[Float:x, Float:y]
 * min[w, h]
 * max[w, h]
 * step[x,y]
 * aspect[Float:min, Float:max]
 * color[r,g,b,a]
 * color2[r,g,b,a]
 * color3[r,g,b,a]
 * aspect_preference
 * rel1[relx,rely]
 * rel1[part_id,part_id]
 * rel1[offx,offy]
 * rel2[relx,relyr]
 * rel2[part_id,part_id]
 * rel2[offx,offy]
 * image[image_id] <- all images have an Id not name in the edje
 * border[l,r,t,b]
 * fill[smooth]
 * fill[pos_relx,pos_rely,pos_offx,pos_offy]
 * fill[sz_relx,sz_rely,sz_offx,sz_offy]
 * color_class
 * text[text]
 * text[text_class]
 * text[font]
 * text[size]
 * text[style]
 * text[fit_x,fit_y]
 * text[min_x,min_y]
 * text[align_x,align_y]
 * visible[on]
 * map_on[on]
 * map_persp[part_id]
 * map_light[part_id]
 * map_rot_center[part_id]
 * map_rot_x[deg]
 * map_rot_y[deg]
 * map_rot_z[deg]
 * map_back_cull[on]
 * map_persp_on[on]
 * persp_zplane[z]
 * persp_focal[z]
 * box[layout]
 * box[fallback_layout]
 * box[Float:align_x, Float:align_y]
 * box[padding_x, padding_y]
 * box[min_x, min_y]
 *
 * ** part_id and program_id need to be able to be "found" from strings
 *
 * get_drag_count(part_id, &Float:dx, &Float:&dy)
 * set_drag_count(part_id, Float:dx, Float:dy)
 * set_drag_confine(part_id, confine_part_id)
 * get_size(&w, &h);
 * resize_request(w, h)
 * get_mouse_buttons()
 * //set_type(part_id, Type:type)
 * //set_effect(part_id, Effect:fx)
 * set_mouse_events(part_id, ev)
 * get_mouse_events(part_id)
 *
 * Pointer_Mode {
 *   POINTER_MODE_AUTOGRAB = 0,
 *   POINTER_MODE_NOGRAB = 1,
 *   POINTER_MODE_NOGREP = 2,
 * }
 *
 * set_pointer_mode(part_id, mode)
 * set_repeat_events(part_id, rep)
 * get_repeat_events(part_id)
 * set_ignore_flags(part_id, flags)
 * get_ignore_flags(part_id)
 * set_mask_flags(part_id, flags)
 * get_mask_flags(part_id)
 *
 * set_focus(part_id, seat_name[])
 * unset_focus(seat_name[])
 *
 * part_swallow(part_id, group_name)
 *
 * external_param_get_int(id, param_name[])
 * external_param_set_int(id, param_name[], value)
 * Float:external_param_get_float(id, param_name[])
 * external_param_set_float(id, param_name[], Float:value)
 * external_param_get_strlen(id, param_name[])
 * external_param_get_str(id, param_name[], value[], value_maxlen)
 * external_param_set_str(id, param_name[], value[])
 * external_param_get_choice_len(id, param_name[])
 * external_param_get_choice(id, param_name[], value[], value_maxlen)
 * external_param_set_choice(id, param_name[], value[])
 * external_param_get_bool(id, param_name[])
 * external_param_set_bool(id, param_name[], value)
 *
 * physics_impulse(part_id, Float:x, Float:y, Float:z)
 * physics_torque_impulse(part_id, Float:x, Float:y, Float:z)
 * physics_force(part_id, Float:x, Float:y, Float:z)
 * physics_torque(part_id, Float:x, Float:y, Float:z)
 * physics_clear_forces(part_id)
 * physics_get_forces(part_id, &Float:x, &Float:y, &Float:z)
 * physics_get_torques(part_id, &Float:x, &Float:y, &Float:z)
 * physics_set_velocity(part_id, Float:x, Float:y, Float:z)
 * physics_get_velocity(part_id, &Float:x, &Float:y, &Float:z)
 * physics_set_ang_velocity(part_id, Float:x, Float:y, Float:z)
 * physics_get_ang_velocity(part_id, &Float:x, &Float:y, &Float:z)
 * physics_stop(part_id)
 * physics_set_rotation(part_id, Float:w, Float:x, Float:y, Float:z)
 * physics_get_rotation(part_id, &Float:w, &Float:x, &Float:y, &Float:z)
 *
 * ADD/DEL CUSTOM OBJECTS UNDER SOLE EMBRYO SCRIPT CONTROL
 *
 */

/**
 * @brief Retrieves an integer variable from Edje's internal variable pool.
 *
 * This function is exposed to Embryo scripts as `get_int(id)`.
 * It fetches an integer value associated with the given ID.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the integer variable to retrieve.
 * @return The integer value associated with the ID, or 0 if not found or on error.
 */
static Embryo_Cell
_edje_embryo_fn_get_int(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   return (Embryo_Cell)_edje_var_int_get(ed, (int)params[1]);
}

/**
 * @brief Sets an integer variable in Edje's internal variable pool.
 *
 * This function is exposed to Embryo scripts as `set_int(id, v)`.
 * It sets an integer value for the given ID.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the integer variable to set.
 *               params[2] is the integer value to set.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_int(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);
   _edje_var_int_set(ed, (int)params[1], (int)params[2]);
   return 0;
}

/**
 * @brief Retrieves a float variable from Edje's internal variable pool.
 *
 * This function is exposed to Embryo scripts as `Float:get_float(id)`.
 * It fetches a float value associated with the given ID.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the float variable to retrieve.
 * @return The float value associated with the ID, converted to an Embryo_Cell.
 *         Returns 0.0 if not found or on error.
 */
static Embryo_Cell
_edje_embryo_fn_get_float(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   float v;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   v = (float)_edje_var_float_get(ed, params[1]);
   return EMBRYO_FLOAT_TO_CELL(v);
}

/**
 * @brief Sets a float variable in Edje's internal variable pool.
 *
 * This function is exposed to Embryo scripts as `set_float(id, Float:v)`.
 * It sets a float value for the given ID.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the float variable to set.
 *               params[2] is the float value (as Embryo_Cell) to set.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_float(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   float v;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);
   v = EMBRYO_CELL_TO_FLOAT(params[2]);
   _edje_var_float_set(ed, (int)params[1], (double)v);
   return 0;
}

/**
 * @brief Retrieves a string variable from Edje's internal variable pool.
 *
 * This function is exposed to Embryo scripts as `get_str(id, dst[], maxlen)`.
 * It fetches a string associated with the given ID and copies it into the
 * destination buffer `dst` provided by the Embryo script, up to `maxlen` characters.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the string variable to retrieve.
 *               params[2] is the Embryo cell address of the destination string buffer.
 *               params[3] is the maximum length of the destination buffer.
 * @return Always 0. The string is written to the `dst` buffer.
 *         If the string is longer than `maxlen`, it is truncated.
 *         If the ID is not found, an empty string is written.
 */
static Embryo_Cell
_edje_embryo_fn_get_str(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *s;

   CHKPARAM(3);
   if (params[3] < 1) return 0;
   ed = embryo_program_data_get(ep);
   s = (char *)_edje_var_str_get(ed, (int)params[1]);
   if (s)
     {
        if ((int)strlen(s) < params[3])
          {
             SETSTR(s, params[2]);
          }
        else
          {
             char *ss;

             ss = alloca(strlen(s) + 1);
             strcpy(ss, s);
             ss[params[3] - 1] = 0;
             SETSTR(ss, params[2]);
          }
     }
   else
     {
        SETSTR("", params[2]);
     }
   return 0;
}

/**
 * @brief Retrieves the length of a string variable from Edje's internal variable pool.
 *
 * This function is exposed to Embryo scripts as `get_strlen(id)`.
 * It returns the length of the string associated with the given ID.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the string variable.
 * @return The length of the string, or 0 if the string is not found or is NULL.
 */
static Embryo_Cell
_edje_embryo_fn_get_strlen(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *s;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   s = (char *)_edje_var_str_get(ed, (int)params[1]);
   if (s)
     {
        return strlen(s);
     }
   return 0;
}

/**
 * @brief Sets a string variable in Edje's internal variable pool.
 *
 * This function is exposed to Embryo scripts as `set_str(id, str[])`.
 * It sets a string value for the given ID.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the string variable to set.
 *               params[2] is the Embryo cell address of the string to set.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_str(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *s;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);
   GETSTR(s, params[2]);
   if (s)
     {
        _edje_var_str_set(ed, (int)params[1], s);
     }
   return 0;
}

/**
 * @brief Gets the number of items in an Edje data collection (list).
 *
 * This function is exposed to Embryo scripts as `count(id)`.
 * It returns the count of elements in the list identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 * @return The number of items in the specified collection.
 */
static Embryo_Cell
_edje_embryo_fn_count(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);

   CHKPARAM(1);

   return (Embryo_Cell)_edje_var_list_count_get(ed, (int)params[1]);
}

/**
 * @brief Removes the Nth item from an Edje data collection (list).
 *
 * This function is exposed to Embryo scripts as `remove(id, n)`.
 * It removes the item at the 0-indexed position `n` from the list
 * identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the 0-indexed position of the item to remove.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_remove(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);

   CHKPARAM(2);

   _edje_var_list_remove_nth(ed, (int)params[1], (int)params[2]);

   return 0;
}

/**
 * @brief Appends an integer to an Edje data collection (list).
 *
 * This function is exposed to Embryo scripts as `append_int(id, v)`.
 * It appends the integer `v` to the end of the list identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the integer value to append.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_append_int(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);

   CHKPARAM(2);

   _edje_var_list_int_append(ed, (int)params[1], (int)params[2]);

   return 0;
}

/**
 * @brief Prepends an integer to an Edje data collection (list).
 *
 * This function is exposed to Embryo scripts as `prepend_int(id, v)`.
 * It prepends the integer `v` to the beginning of the list identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the integer value to prepend.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_prepend_int(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);

   CHKPARAM(2);

   _edje_var_list_int_prepend(ed, (int)params[1], (int)params[2]);

   return 0;
}

/**
 * @brief Inserts an integer into an Edje data collection (list) at a specific position.
 *
 * This function is exposed to Embryo scripts as `insert_int(id, n, v)`.
 * It inserts the integer `v` at the 0-indexed position `n` in the list
 * identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the 0-indexed position at which to insert.
 *               params[3] is the integer value to insert.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_insert_int(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);

   CHKPARAM(3);

   _edje_var_list_int_insert(ed, (int)params[1], (int)params[2],
                             (int)params[3]);

   return 0;
}

/**
 * @brief Replaces an integer in an Edje data collection (list) at a specific position.
 *
 * This function is exposed to Embryo scripts as `replace_int(id, n, v)`.
 * It replaces the integer at the 0-indexed position `n` in the list
 * identified by `id` with the new integer `v`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the 0-indexed position of the item to replace.
 *               params[3] is the new integer value.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_replace_int(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);

   CHKPARAM(3);

   _edje_var_list_nth_int_set(ed, (int)params[1], (int)params[2],
                              (int)params[3]);

   return 0;
}

/**
 * @brief Fetches an integer from an Edje data collection (list) at a specific position.
 *
 * This function is exposed to Embryo scripts as `fetch_int(id, n)`.
 * It retrieves the integer at the 0-indexed position `n` from the list
 * identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the 0-indexed position of the item to fetch.
 * @return The integer value at the specified position, or 0 if out of bounds or on error.
 */
static Embryo_Cell
_edje_embryo_fn_fetch_int(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);

   CHKPARAM(2);

   return _edje_var_list_nth_int_get(ed, (int)params[1],
                                     (int)params[2]);
}

/**
 * @brief Appends a string to an Edje data collection (list).
 *
 * This function is exposed to Embryo scripts as `append_str(id, str[])`.
 * It appends the string `str` to the end of the list identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the Embryo cell address of the string to append.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_append_str(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);
   char *s;

   CHKPARAM(2);

   GETSTR(s, params[2]);
   if (s)
     _edje_var_list_str_append(ed, (int)params[1], s);

   return 0;
}

/**
 * @brief Prepends a string to an Edje data collection (list).
 *
 * This function is exposed to Embryo scripts as `prepend_str(id, str[])`.
 * It prepends the string `str` to the beginning of the list identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the Embryo cell address of the string to prepend.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_prepend_str(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);
   char *s;

   CHKPARAM(2);

   GETSTR(s, params[2]);
   if (s)
     _edje_var_list_str_prepend(ed, (int)params[1], s);

   return 0;
}

/**
 * @brief Inserts a string into an Edje data collection (list) at a specific position.
 *
 * This function is exposed to Embryo scripts as `insert_str(id, n, str[])`.
 * It inserts the string `str` at the 0-indexed position `n` in the list
 * identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the 0-indexed position at which to insert.
 *               params[3] is the Embryo cell address of the string to insert.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_insert_str(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);
   char *s;

   CHKPARAM(3);

   GETSTR(s, params[3]);
   if (s)
     _edje_var_list_str_insert(ed, (int)params[1], (int)params[2], s);

   return 0;
}

/**
 * @brief Replaces a string in an Edje data collection (list) at a specific position.
 *
 * This function is exposed to Embryo scripts as `replace_str(id, n, str[])`.
 * It replaces the string at the 0-indexed position `n` in the list
 * identified by `id` with the new string `str`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the 0-indexed position of the item to replace.
 *               params[3] is the Embryo cell address of the new string.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_replace_str(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);
   char *s;

   CHKPARAM(3);

   GETSTR(s, params[3]);
   if (s)
     _edje_var_list_nth_str_set(ed, (int)params[1], (int)params[2], s);

   return 0;
}

/**
 * @brief Fetches a string from an Edje data collection (list) at a specific position.
 *
 * This function is exposed to Embryo scripts as `fetch_str(id, n, dst[], maxlen)`.
 * It retrieves the string at the 0-indexed position `n` from the list
 * identified by `id` and copies it into the destination buffer `dst`,
 * up to `maxlen` characters.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the 0-indexed position of the item to fetch.
 *               params[3] is the Embryo cell address of the destination string buffer.
 *               params[4] is the maximum length of the destination buffer.
 * @return Always 0. The string is written to the `dst` buffer.
 *         If the string is longer than `maxlen`, it is truncated.
 *         If the item is not found or is not a string, an empty string is written.
 */
static Embryo_Cell
_edje_embryo_fn_fetch_str(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);
   char *s;

   CHKPARAM(4);

   s = (char *)_edje_var_list_nth_str_get(ed, (int)params[1],
                                          (int)params[2]);
   if (s)
     {
        if ((int)strlen(s) < params[4])
          {
             SETSTR(s, params[3]);
          }
        else
          {
             char *ss;

             ss = alloca(strlen(s) + 1);
             strcpy(ss, s);
             ss[params[4] - 1] = 0;
             SETSTR(ss, params[3]);
          }
     }
   else
     {
        SETSTR("", params[3]);
     }

   return 0;
}

/**
 * @brief Appends a float to an Edje data collection (list).
 *
 * This function is exposed to Embryo scripts as `append_float(id, Float:v)`.
 * It appends the float `v` to the end of the list identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the float value (as Embryo_Cell) to append.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_append_float(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);
   float f;

   CHKPARAM(2);

   f = EMBRYO_CELL_TO_FLOAT(params[2]);
   _edje_var_list_float_append(ed, (int)params[1], f);

   return 0;
}

/**
 * @brief Prepends a float to an Edje data collection (list).
 *
 * This function is exposed to Embryo scripts as `prepend_float(id, Float:v)`.
 * It prepends the float `v` to the beginning of the list identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the float value (as Embryo_Cell) to prepend.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_prepend_float(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);
   float f;

   CHKPARAM(2);

   f = EMBRYO_CELL_TO_FLOAT(params[2]);
   _edje_var_list_float_prepend(ed, (int)params[1], f);

   return 0;
}

/**
 * @brief Inserts a float into an Edje data collection (list) at a specific position.
 *
 * This function is exposed to Embryo scripts as `insert_float(id, n, Float:v)`.
 * It inserts the float `v` at the 0-indexed position `n` in the list
 * identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the 0-indexed position at which to insert.
 *               params[3] is the float value (as Embryo_Cell) to insert.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_insert_float(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);
   float f;

   CHKPARAM(3);

   f = EMBRYO_CELL_TO_FLOAT(params[3]);
   _edje_var_list_float_insert(ed, (int)params[1], (int)params[2], f);

   return 0;
}

/**
 * @brief Replaces a float in an Edje data collection (list) at a specific position.
 *
 * This function is exposed to Embryo scripts as `replace_float(id, n, Float:v)`.
 * It replaces the float at the 0-indexed position `n` in the list
 * identified by `id` with the new float `v`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the 0-indexed position of the item to replace.
 *               params[3] is the new float value (as Embryo_Cell).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_replace_float(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);

   CHKPARAM(3);

   _edje_var_list_nth_float_set(ed, (int)params[1], (int)params[2],
                                EMBRYO_CELL_TO_FLOAT(params[3]));

   return 0;
}

/**
 * @brief Fetches a float from an Edje data collection (list) at a specific position.
 *
 * This function is exposed to Embryo scripts as `Float:fetch_float(id, n)`.
 * It retrieves the float at the 0-indexed position `n` from the list
 * identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the data collection.
 *               params[2] is the 0-indexed position of the item to fetch.
 * @return The float value (as Embryo_Cell) at the specified position.
 *         Returns 0.0 if out of bounds or on error.
 */
static Embryo_Cell
_edje_embryo_fn_fetch_float(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);
   float f;

   CHKPARAM(2);

   f = _edje_var_list_nth_float_get(ed, (int)params[1], (int)params[2]);

   return EMBRYO_FLOAT_TO_CELL(f);
}

/**
 * @brief Creates a timer that calls an Embryo function after a specified delay.
 *
 * This function is exposed to Embryo scripts as `timer(Float:in, fname[], val)`.
 * It schedules the Embryo function `fname` to be called after `in` seconds,
 * passing `val` as an argument to it.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the delay in seconds (float, as Embryo_Cell).
 *               params[2] is the Embryo cell address of the function name string.
 *               params[3] is an integer value to pass to the timer function.
 * @return A unique ID for the created timer, or 0 on failure.
 */
static Embryo_Cell
_edje_embryo_fn_timer(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *fname = NULL;
   float f;
   double in;
   int val;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   GETSTR(fname, params[2]);
   if ((!fname)) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   in = (double)f;
   val = params[3];
   return _edje_var_timer_add(ed, in, fname, val);
}

/**
 * @brief Cancels a previously created timer.
 *
 * This function is exposed to Embryo scripts as `cancel_timer(id)`.
 * It cancels the timer identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the timer to cancel.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_cancel_timer(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int id;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   id = params[1];
   if (id <= 0) return 0;
   _edje_var_timer_del(ed, id);
   return 0;
}

/**
 * @brief Resets a previously created timer.
 *
 * This function is exposed to Embryo scripts as `reset_timer(id)`.
 * It resets the timer identified by `id`, causing it to restart its countdown.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the timer to reset.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_reset_timer(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int id;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   id = params[1];
   if (id <= 0) return 0;
   _edje_var_timer_reset(ed, id);
   return 0;
}

/**
 * @brief Creates an animation that calls an Embryo function repeatedly over a duration.
 *
 * This function is exposed to Embryo scripts as `anim(Float:len, fname[], val)`.
 * It schedules the Embryo function `fname` to be called repeatedly for `len` seconds.
 * The function `fname` will receive the current animation position (0.0 to 1.0)
 * and `val` as arguments.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the duration of the animation in seconds (float, as Embryo_Cell).
 *               params[2] is the Embryo cell address of the function name string.
 *               params[3] is an integer value to pass to the animation function.
 * @return A unique ID for the created animation, or 0 on failure.
 */
static Embryo_Cell
_edje_embryo_fn_anim(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *fname = NULL;
   float f;
   double len;
   int val;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   GETSTR(fname, params[2]);
   if ((!fname)) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   len = (double)f;
   val = params[3];
   return _edje_var_anim_add(ed, len, fname, val);
}

/**
 * @brief Cancels a previously created animation.
 *
 * This function is exposed to Embryo scripts as `cancel_anim(id)`.
 * It cancels the animation identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the animation to cancel.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_cancel_anim(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int id;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   id = params[1];
   if (id <= 0) return 0;
   _edje_var_anim_del(ed, id);
   return 0;
}

/**
 * @brief Calculates a mapped animation position based on a tweening mode.
 *
 * This function is exposed to Embryo scripts as
 * `get_anim_pos_map(Float:pos, Tween_Mode_Type:tween, Float:v1, Float:v2, &Float:ret)`.
 * It takes a linear position `pos` (0.0 to 1.0) and applies a tweening
 * function (`tween` type with optional parameters `v1`, `v2`) to it,
 * storing the result in `ret`.
 *
 * @param ep The Embryo program instance (unused).
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the input position (0.0 to 1.0, float as Embryo_Cell).
 *               params[2] is the `Ecore_Pos_Map` tweening mode type (integer).
 *               params[3] is the first optional float parameter `v1` for the tween mode.
 *               params[4] is the second optional float parameter `v2` for the tween mode.
 *               params[5] is the Embryo cell address to store the resulting mapped position (float).
 * @return Always 0. The result is stored via the `ret` parameter.
 */
static Embryo_Cell
_edje_embryo_fn_get_anim_pos_map(Embryo_Program *ep, Embryo_Cell *params)
{
   double pos;
   Ecore_Pos_Map tween;
   double v1, v2;

   CHKPARAM(5);
   pos = EMBRYO_CELL_TO_FLOAT(params[1]);
   tween = params[2];
   v1 = EMBRYO_CELL_TO_FLOAT(params[3]);
   v2 = EMBRYO_CELL_TO_FLOAT(params[4]);

   switch (tween)
     {
      case ECORE_POS_MAP_LINEAR:
      case ECORE_POS_MAP_ACCELERATE:
      case ECORE_POS_MAP_DECELERATE:
      case ECORE_POS_MAP_SINUSOIDAL:
        pos = ecore_animator_pos_map(pos, tween, 0, 0);
        break;

      case ECORE_POS_MAP_ACCELERATE_FACTOR:
        pos = ecore_animator_pos_map(pos, ECORE_POS_MAP_ACCELERATE_FACTOR,
                                     v1, 0);
        break;

      case ECORE_POS_MAP_DECELERATE_FACTOR:
        pos = ecore_animator_pos_map(pos, ECORE_POS_MAP_DECELERATE_FACTOR,
                                     v1, 0);
        break;

      case ECORE_POS_MAP_SINUSOIDAL_FACTOR:
        pos = ecore_animator_pos_map(pos, ECORE_POS_MAP_SINUSOIDAL_FACTOR,
                                     v1, 0);
        break;

      case ECORE_POS_MAP_DIVISOR_INTERP:
      case ECORE_POS_MAP_BOUNCE:
      case ECORE_POS_MAP_SPRING:
        pos = ecore_animator_pos_map(pos, tween, v1, v2);
        break;

      default:
        break;
     }

   SETFLOAT(pos, params[5]);

   return 0;
}

/**
 * @brief Sets the minimum size of the Edje object.
 *
 * This function is exposed to Embryo scripts as `set_min_size(Float:w, Float:h)`.
 * It sets the minimum width `w` and height `h` for the current Edje object's collection.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the minimum width (float, as Embryo_Cell).
 *               params[2] is the minimum height (float, as Embryo_Cell).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_min_size(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   float f = 0.0;
   double w = 0.0, h = 0.0;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   w = (double)f;
   f = EMBRYO_CELL_TO_FLOAT(params[2]);
   h = (double)f;

   if (w < 0.0) w = 0.0;
   if (h < 0.0) h = 0.0;
   ed->collection->prop.min.w = w;
   ed->collection->prop.min.h = h;
   ed->recalc_call = EINA_TRUE;
   ed->dirty = EINA_TRUE;
#ifdef EDJE_CALC_CACHE
   ed->all_part_change = EINA_TRUE;
#endif
   _edje_recalc(ed);
   return 0;
}

/**
 * @brief Sets the maximum size of the Edje object.
 *
 * This function is exposed to Embryo scripts as `set_max_size(Float:w, Float:h)`.
 * It sets the maximum width `w` and height `h` for the current Edje object's collection.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the maximum width (float, as Embryo_Cell).
 *               params[2] is the maximum height (float, as Embryo_Cell).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_max_size(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   float f = 0.0;
   double w = 0.0, h = 0.0;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);
   f = EMBRYO_CELL_TO_FLOAT(params[1]);
   w = (double)f;
   f = EMBRYO_CELL_TO_FLOAT(params[2]);
   h = (double)f;

   if (w < 0.0) w = 0.0;
   if (h < 0.0) h = 0.0;
   ed->collection->prop.max.w = w;
   ed->collection->prop.max.h = h;
   ed->recalc_call = EINA_TRUE;
   ed->dirty = EINA_TRUE;
#ifdef EDJE_CALC_CACHE
   ed->all_part_change = EINA_TRUE;
#endif
   _edje_recalc(ed);

   return 0;
}

/**
 * @brief Stops a running or pending Edje program.
 *
 * This function is exposed to Embryo scripts as `stop_program(program_id)`.
 * It stops the Edje program identified by `program_id`. This includes
 * currently running instances and any pending (timer-delayed) instances.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the program to stop.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_stop_program(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int program_id = 0;
   Edje_Running_Program *runp;
   Edje_Pending_Program *pp;
   Eina_List *l, *ll;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   program_id = params[1];
   if (program_id < 0) return 0;

   ed->walking_actions = EINA_TRUE;

   EINA_LIST_FOREACH(ed->actions, l, runp)
     if (program_id == runp->program->id)
       _edje_program_end(ed, runp);
   EINA_LIST_FOREACH_SAFE(ed->pending_actions, l, ll, pp)
     if (program_id == pp->program->id)
       {
          ed->pending_actions = eina_list_remove_list(ed->pending_actions, l);
          ecore_timer_del(pp->timer);
          free(pp);
       }

   ed->walking_actions = EINA_FALSE;

   return 0;
}

/**
 * @brief Stops all Edje programs currently acting on a specific part.
 *
 * This function is exposed to Embryo scripts as `stop_programs_on(part_id)`.
 * It finds the part identified by `part_id` and stops any program
 * currently running on it, as well as any pending programs targeting it.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_stop_programs_on(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;

   int part_id = 0;
   Edje_Real_Part *rp;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   if (rp)
     {
        Eina_List *l, *ll, *lll;
        Edje_Pending_Program *pp;
        Edje_Program_Target *pt;
        /* there is only ever 1 program acting on a part at any time */
        if (rp->program) _edje_program_end(ed, rp->program);
        EINA_LIST_FOREACH_SAFE(ed->pending_actions, l, ll, pp)
          {
             EINA_LIST_FOREACH(pp->program->targets, lll, pt)
               if (pt->id == part_id)
                 {
                    ed->pending_actions = eina_list_remove_list(ed->pending_actions, l);
                    ecore_timer_del(pp->timer);
                    free(pp);
                    break;
                 }
          }
     }
   return 0;
}

/**
 * @brief Gets the current mouse pointer coordinates relative to the Edje object.
 *
 * This function is exposed to Embryo scripts as `get_mouse(&x, &y)`.
 * It retrieves the canvas X and Y coordinates of the mouse pointer and
 * adjusts them to be relative to the Edje object's origin.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the Embryo cell address to store the X coordinate (integer).
 *               params[2] is the Embryo cell address to store the Y coordinate (integer).
 * @return Always 0. The coordinates are stored via the `x` and `y` parameters.
 */
static Embryo_Cell
_edje_embryo_fn_get_mouse(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   Evas_Coord x = 0, y = 0;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);
   evas_pointer_canvas_xy_get(ed->base.evas, &x, &y);
   x -= ed->x;
   y -= ed->y;
   SETINT((int)x, params[1]);
   SETINT((int)y, params[2]);
   return 0;
}

/**
 * @brief Gets the current state of mouse buttons.
 *
 * This function is exposed to Embryo scripts as `get_mouse_buttons()`.
 * It returns a bitmask representing the currently pressed mouse buttons
 * on the Evas canvas associated with the Edje object.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells (params[0] is the number of arguments, expected to be 0).
 * @return A bitmask of pressed mouse buttons (e.g., 1 for button 1, 2 for button 2, 4 for button 3).
 */
static Embryo_Cell
_edje_embryo_fn_get_mouse_buttons(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;

   CHKPARAM(0);
   ed = embryo_program_data_get(ep);
   return evas_pointer_button_down_mask_get(ed->base.evas);
}

/**
 * @brief Emits an Edje signal.
 *
 * This function is exposed to Embryo scripts as `emit(sig[], src[])`.
 * It triggers an Edje signal with the given signal string `sig` and
 * source string `src`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the Embryo cell address of the signal string.
 *               params[2] is the Embryo cell address of the source string.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_emit(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *sig = NULL, *src = NULL;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);
   GETSTR(sig, params[1]);
   GETSTR(src, params[2]);
   if ((!sig) || (!src)) return 0;
   _edje_emit(ed, sig, src);
   return 0;
}

/**
 * @brief Gets the ID of an Edje part by its name.
 *
 * This function is exposed to Embryo scripts as `get_part_id(part[])`.
 * It searches for a part with the given name `part[]` within the current
 * Edje object's collection and returns its numerical ID.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the Embryo cell address of the part name string.
 * @return The ID of the part if found, otherwise -1.
 */
static Embryo_Cell
_edje_embryo_fn_get_part_id(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   Edje_Part_Collection *col;
   Edje_Part **part;
   char *p;
   unsigned int i;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   GETSTR(p, params[1]);
   if (!p) return -1;
   col = ed->collection;
   if (!col) return -1;
   part = col->parts;
   for (i = 0; i < col->parts_count; i++, part++)
     {
        if (!(*part)->name) continue;
        if (!strcmp((*part)->name, p)) return (*part)->id;
     }
   return -1;
}

/**
 * @brief Gets the ID of an image resource by its name.
 *
 * This function is exposed to Embryo scripts as `get_image_id(img[])`.
 * It searches for an image with the given name `img[]` within the Edje file's
 * image directory and returns its numerical ID.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the Embryo cell address of the image name string.
 * @return The ID of the image if found, otherwise -1.
 */
static Embryo_Cell
_edje_embryo_fn_get_image_id(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   Edje_File *file;
   Edje_Image_Directory *dir;
   Edje_Image_Directory_Entry *dirent;
   char *p;
   unsigned int i;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   GETSTR(p, params[1]);
   if (!p) return -1;
   file = ed->file;
   if (!file) return -1;
   dir = file->image_dir;
   dirent = dir->entries;
   for (i = 0; i < dir->entries_count; i++, dirent++)
     {
        if (!dirent->entry) continue;
        if (!strcmp(dirent->entry, p)) return dirent->id;
     }
   return -1;
}

/**
 * @brief Gets the ID of an Edje program by its name.
 *
 * This function is exposed to Embryo scripts as `get_program_id(program[])`.
 * It searches for an Edje program with the given name `program[]` within the
 * current collection and returns its numerical ID.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the Embryo cell address of the program name string.
 * @return The ID of the program if found, otherwise -1.
 */
static Embryo_Cell
_edje_embryo_fn_get_program_id(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   Edje_Program **prog;
   char *p;
   int i;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   GETSTR(p, params[1]);
   if (!p) return -1;
   prog = ed->collection->patterns.table_programs;
   if (!prog) return -1;
   for (i = 0; i < ed->collection->patterns.table_programs_size; i++, prog++)
     {
        if (!(*prog)->name) continue;
        if (!strcmp((*prog)->name, p)) return (*prog)->id;
     }
   return -1;
}

/**
 * @brief Plays a sound sample.
 *
 * This function is exposed to Embryo scripts as `play_sample(sample_name, speed, ...)`.
 * It plays the sound sample identified by `sample_name` at the given `speed`.
 * An optional channel can be specified.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the total size of parameters in bytes.
 *               params[1] is the Embryo cell address of the sample name string.
 *               params[2] is the playback speed (float, as Embryo_Cell).
 *               params[3] (optional) is the channel to play on (integer).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_play_sample(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *sample_name = NULL;
   float speed = 1.0;
   int channel = 0;

   if (params[0] < (int)(sizeof(Embryo_Cell) * 2))
     return 0;
   ed = embryo_program_data_get(ep);
   GETSTR(sample_name, params[1]);
   if ((!sample_name)) return 0;
   speed = EMBRYO_CELL_TO_FLOAT(params[2]);
   if (params[0] == (int)(sizeof(Embryo_Cell) * 3))
     GETINT(channel, params[3]);
   _edje_multisense_internal_sound_sample_play(ed, sample_name,
                                               (double)speed, channel);
   return 0;
}

/**
 * @brief Plays a sound tone.
 *
 * This function is exposed to Embryo scripts as `play_tone(tone_name, duration, ...)`.
 * It plays the sound tone identified by `tone_name` for the given `duration`.
 * An optional channel can be specified.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the total size of parameters in bytes.
 *               params[1] is the Embryo cell address of the tone name string.
 *               params[2] is the duration in seconds (float, as Embryo_Cell).
 *               params[3] (optional) is the channel to play on (integer).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_play_tone(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *tone_name = NULL;
   float duration = 0.1;
   int channel = 0;

   if (params[0] < (int)(sizeof(Embryo_Cell) * 2))
     return 0;
   ed = embryo_program_data_get(ep);
   GETSTR(tone_name, params[1]);
   if ((!tone_name)) return 0;
   duration = EMBRYO_CELL_TO_FLOAT(params[2]);
   if (params[0] == (int)(sizeof(Embryo_Cell) * 3))
     GETINT(channel, params[3]);
   _edje_multisense_internal_sound_tone_play(ed, tone_name,
                                             (double)duration, channel);
   return 0;
}

/**
 * @brief Plays a vibration sample.
 *
 * This function is exposed to Embryo scripts as `play_vibration(sample_name, repeat)`.
 * It plays the vibration sample identified by `sample_name`, repeating it `repeat` times.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the total size of parameters in bytes.
 *               params[1] is the Embryo cell address of the sample name string.
 *               params[2] (optional) is the number of repetitions (integer, default 10).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_play_vibration(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *sample_name = NULL;
   int repeat = 10;

   if (params[0] < (int)(sizeof(Embryo_Cell) * 2)) return 0;
   ed = embryo_program_data_get(ep);
   GETSTR(sample_name, params[1]);
   if ((!sample_name)) return 0;

   if (params[0] == (int)(sizeof(Embryo_Cell) * 2))
     GETINT(repeat, params[2]);

   _edje_multisense_internal_vibration_sample_play(ed, sample_name,
                                                   repeat);
   return 0;
}

/**
 * @brief Sets the state of an Edje part.
 *
 * This function is exposed to Embryo scripts as `set_state(part_id, state[], Float:state_val)`.
 * It applies the specified `state` (e.g., "default", "clicked") with `state_val`
 * to the part identified by `part_id`. Any existing program on the part is stopped.
 * The part's position is reset with a linear tween.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments (must be 2 or 3).
 *               params[1] is the ID of the part.
 *               params[2] is the Embryo cell address of the state name string.
 *               params[3] (optional) is the state value (float, as Embryo_Cell, default 0.0).
 * @return 0 on success, -1 if the wrong number of parameters is provided.
 */
static Embryo_Cell
_edje_embryo_fn_set_state(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *state = NULL;
   int part_id = 0;
   float f = 0.0;
   double value = 0.0;
   Edje_Real_Part *rp;

   if (!(HASNPARAMS(2) || HASNPARAMS(3))) return -1;

   ed = embryo_program_data_get(ep);
   GETSTR(state, params[2]);
   if ((!state)) return 0;
   part_id = params[1];
   if (part_id < 0) return 0;
   if (HASNPARAMS(3))
     {
        f = EMBRYO_CELL_TO_FLOAT(params[3]);
        value = (double)f;
     }
   else
     value = 0.0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   if (rp)
     {
        if (rp->program) _edje_program_end(ed, rp->program);
        _edje_part_description_apply(ed, rp, state, value, NULL, 0.0);
        _edje_part_pos_set(ed, rp, EDJE_TWEEN_MODE_LINEAR, ZERO, ZERO, ZERO,
                           ZERO, ZERO);
        _edje_recalc(ed);
     }
   return 0;
}

/**
 * @brief Sets the state of an Edje part with animation parameters.
 *
 * This function is exposed to Embryo scripts as
 * `set_state_anim(part_id, state_name[], Float:state_val, anim_type, Float:tween_time, ...)`.
 * It applies the specified `state_name` with `state_val` to the part identified by `part_id`.
 * The transition to this state is animated according to `anim_type`, `tween_time`, and
 * other optional animation parameters (v1, v2, v3, v4, "CURRENT" flag).
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 *               params[2] is the Embryo cell address of the state name string.
 *               params[3] is the state value (float, as Embryo_Cell).
 *               params[4] is the animation type (Edje_Tween_Mode).
 *               params[5] is the tween duration (float, as Embryo_Cell).
 *               params[6+] Optional parameters depending on anim_type:
 *                        - For factor-based tweens (ACCELERATE_FACTOR, etc.):
 *                          params[6]: factor (Float:v1)
 *                          params[7] (optional): "CURRENT" string
 *                        - For interpolator-based tweens (DIVISOR_INTERP, BOUNCE, SPRING):
 *                          params[6]: v1 (e.g., divisor for DIVISOR_INTERP)
 *                          params[7]: v2 (e.g., count for BOUNCE/SPRING)
 *                          params[8] (optional): "CURRENT" string
 *                        - For CUBIC_BEZIER:
 *                          params[6]: x1
 *                          params[7]: y1
 *                          params[8]: x2 (or "CURRENT" if 12 params)
 *                          params[9]: y2 (or x2 if 10 params)
 *                          params[10] (optional): y2 if 10 params
 *                          params[11] (optional): "CURRENT" string if 12 params
 *                        - For simple tweens (LINEAR, ACCELERATE, DECELERATE, SINUSOIDAL):
 *                          params[6] (optional): "CURRENT" string
 * @return 0 on success, -1 on parsing error or if wrong number of parameters.
 */
static Embryo_Cell
_edje_embryo_fn_set_state_anim(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *state = NULL;
   int part_id = 0;
   float f = 0.0;
   double value = 0.0;
   char *tmp = NULL;
   Edje_Real_Part *rp;
   int anim_type = 0;
   double tween = 0.0, v1 = 0.0, v2 = 0.0, v3 = 0.0, v4 = 0.0;

   if (HASNPARAMS(4)) return -1;

   ed = embryo_program_data_get(ep);
   GETSTR(state, params[2]);
   if ((!state)) return 0;
   part_id = params[1];
   if (part_id < 0) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[3]);
   value = (double)f;
   anim_type = params[4];
   f = EMBRYO_CELL_TO_FLOAT(params[5]);
   tween = (double)f;
   if ((anim_type >= EDJE_TWEEN_MODE_LINEAR) &&
       (anim_type <= EDJE_TWEEN_MODE_DECELERATE))
     {
        if (HASNPARAMS(6))
          {
             GETSTR(tmp, params[6]);
             if ((tmp) && (!strcmp(tmp, "CURRENT")))
               anim_type |= EDJE_TWEEN_MODE_OPT_FROM_CURRENT;
          }
     }
   else if ((anim_type >= EDJE_TWEEN_MODE_ACCELERATE_FACTOR) &&
            (anim_type <= EDJE_TWEEN_MODE_SINUSOIDAL_FACTOR))
     {
        if (HASNPARAMS(7))
          {
             GETSTR(tmp, params[7]);
             if ((tmp) && (!strcmp(tmp, "CURRENT")))
               anim_type |= EDJE_TWEEN_MODE_OPT_FROM_CURRENT;
          }
        else if (HASNPARAMS(5))
          {
             ERR("parse error. Need 6th parameter to set factor");
             return -1;
          }
        GETFLOAT_T(v1, params[6]);
     }
   else if ((anim_type >= EDJE_TWEEN_MODE_DIVISOR_INTERP) &&
            (anim_type <= EDJE_TWEEN_MODE_SPRING))
     {
        if (HASNPARAMS(8))
          {
             GETSTR(tmp, params[8]);
             if ((tmp) && (!strcmp(tmp, "CURRENT")))
               anim_type |= EDJE_TWEEN_MODE_OPT_FROM_CURRENT;
          }
        else if (HASNPARAMS(5))
          {
             ERR("parse error.Need 6th and 7th parameters to set factor and counts");
             return -1;
          }
        GETFLOAT_T(v1, params[6]);
        GETFLOAT_T(v2, params[7]);
     }
   else if (anim_type == EDJE_TWEEN_MODE_CUBIC_BEZIER)
     {
        if (HASNPARAMS(12))
          {
             GETSTR(tmp, params[8]);
             if ((tmp) && (!strcmp(tmp, "CURRENT")))
               anim_type |= EDJE_TWEEN_MODE_OPT_FROM_CURRENT;
          }
        else if (HASNPARAMS(5))
          {
             ERR("parse error.Need 6th, 7th, 9th and 10th parameters to set x1, y1, x2 and y2");
             return -1;
          }
        if (HASNPARAMS(10))
          {
             GETFLOAT_T(v1, params[6]);
             GETFLOAT_T(v2, params[7]);
             GETFLOAT_T(v3, params[9]);
             GETFLOAT_T(v4, params[10]);
          }
        else
          {
             GETFLOAT_T(v1, params[6]);
             GETFLOAT_T(v2, params[7]);
             GETFLOAT_T(v3, params[8]);
             GETFLOAT_T(v4, params[9]);
          }
     }
   rp = ed->table_parts[part_id % ed->table_parts_size];
   if (!rp) return 0;
   _edje_part_description_apply(ed, rp, NULL, 0.0, state, value);
   _edje_part_pos_set(ed, rp, anim_type, FROM_DOUBLE(tween), v1, v2,
                      v3, v4);
   _edje_recalc(ed);
   return 0;
}

/**
 * @brief Gets the current state of an Edje part.
 *
 * This function is exposed to Embryo scripts as `get_state(part_id, dst[], maxlen, &Float:val)`.
 * It retrieves the name and value of the current state of the part identified by `part_id`.
 * The state name is copied into `dst` (up to `maxlen`), and the state value is stored in `val`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 *               params[2] is the Embryo cell address of the destination string buffer for the state name.
 *               params[3] is the maximum length of the destination buffer.
 *               params[4] is the Embryo cell address to store the state value (float).
 * @return Always 0. State name and value are returned via output parameters.
 *         If no description is chosen, an empty string and 0.0 are returned.
 */
static Embryo_Cell
_edje_embryo_fn_get_state(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;
   const char *s;

   CHKPARAM(4);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   if (rp->chosen_description)
     {
        SETFLOAT(rp->chosen_description->state.value, params[4]);
        s = rp->chosen_description->state.name;
        if (s)
          {
             if ((int)strlen(s) < params[3])
               {
                  SETSTR(s, params[2]);
               }
             else
               {
                  char *ss;

                  ss = alloca(strlen(s) + 1);
                  strcpy(ss, s);
                  ss[params[3] - 1] = 0;
                  SETSTR(ss, params[2]);
               }
          }
        else
          {
             SETSTR("", params[2]);
          }
     }
   else
     {
        SETFLOAT(0.0, params[4]);
        SETSTR("", params[2]);
     }
   return 0;
}

/**
 * @brief Sets a part to an intermediate state between two defined states using linear tweening.
 *
 * This function is exposed to Embryo scripts as
 * `set_tween_state(part_id, Float:tween, state1[], Float:state1_val, state2[], Float:state2_val)`.
 * It positions a part at an interpolated point `tween` (0.0 to 1.0) between
 * `state1` (at `state1_val`) and `state2` (at `state2_val`).
 * Any existing program on the part is stopped.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 *               params[2] is the tween factor (0.0 to 1.0, float as Embryo_Cell).
 *               params[3] is the Embryo cell address of the first state name string.
 *               params[4] is the value for the first state (float, as Embryo_Cell).
 *               params[5] is the Embryo cell address of the second state name string.
 *               params[6] is the value for the second state (float, as Embryo_Cell).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_tween_state(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *state1 = NULL, *state2 = NULL;
   int part_id = 0;
   float f = 0.0;
   double tween = 0.0, value1 = 0.0, value2 = 0.0;
   Edje_Real_Part *rp;

   CHKPARAM(6);
   ed = embryo_program_data_get(ep);
   GETSTR(state1, params[3]);
   GETSTR(state2, params[5]);
   if ((!state1) || (!state2)) return 0;
   part_id = params[1];
   if (part_id < 0) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[2]);
   tween = (double)f;
   f = EMBRYO_CELL_TO_FLOAT(params[4]);
   value1 = (double)f;
   f = EMBRYO_CELL_TO_FLOAT(params[6]);
   value2 = (double)f;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   if (rp)
     {
        if (rp->program) _edje_program_end(ed, rp->program);
        _edje_part_description_apply(ed, rp, state1, value1, state2, value2);
        _edje_part_pos_set(ed, rp, EDJE_TWEEN_MODE_LINEAR, FROM_DOUBLE(tween),
                           ZERO, ZERO, ZERO, ZERO);
        _edje_recalc(ed);
     }
   return 0;
}

/**
 * @brief Sets a part to an intermediate state between two defined states with animation parameters.
 *
 * This function is exposed to Embryo scripts as
 * `set_tween_state_anim(part_id, state1_name[], Float:state1_val, state2_name[], Float:state2_val, anim_type, Float:tween_pos, ...)`.
 * It transitions a part to an interpolated point `tween_pos` (0.0 to 1.0) between
 * `state1_name` (at `state1_val`) and `state2_name` (at `state2_val`).
 * The transition itself is animated according to `anim_type` and other optional
 * animation parameters (v1, v2, v3, v4, "CURRENT" flag).
 * Any existing program on the part is stopped.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 *               params[2] is the Embryo cell address of the first state name string.
 *               params[3] is the value for the first state (float, as Embryo_Cell).
 *               params[4] is the Embryo cell address of the second state name string.
 *               params[5] is the value for the second state (float, as Embryo_Cell).
 *               params[6] is the animation type (Edje_Tween_Mode) for the transition.
 *               params[7] is the target tween position (0.0 to 1.0, float as Embryo_Cell) between state1 and state2.
 *               params[8+] Optional parameters depending on anim_type, similar to _edje_embryo_fn_set_state_anim:
 *                        - For factor-based tweens:
 *                          params[8]: factor (Float:v1)
 *                          params[9] (optional): "CURRENT" string
 *                        - For interpolator-based tweens:
 *                          params[8]: v1
 *                          params[9]: v2
 *                          params[10] (optional): "CURRENT" string
 *                        - For CUBIC_BEZIER:
 *                          params[8]: x1
 *                          params[9]: y1
 *                          params[10]: x2 (or "CURRENT" if 12 params)
 *                          params[11]: y2 (or x2 if 12 params)
 *                          params[12] (optional): y2 if 12 params
 *                        - For simple tweens:
 *                          params[8] (optional): "CURRENT" string
 * @return 0 on success, -1 on parsing error or if wrong number of parameters.
 */
static Embryo_Cell
_edje_embryo_fn_set_tween_state_anim(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *tmp = NULL;
   char *state1 = NULL, *state2 = NULL;
   int part_id = 0;
   int anim_type = 0;
   float f = 0.0;
   double tween = 0.0, value1 = 0.0, value2 = 0.0, v1 = 0.0, v2 = 0.0, v3 = 0.0, v4 = 0.0;
   Edje_Real_Part *rp;

   if (HASNPARAMS(6)) return -1;
   ed = embryo_program_data_get(ep);
   GETSTR(state1, params[2]);
   GETSTR(state2, params[4]);
   if ((!state1) || (!state2)) return 0;
   part_id = params[1];
   anim_type = params[6];
   if (part_id < 0) return 0;
   f = EMBRYO_CELL_TO_FLOAT(params[3]);
   value1 = (double)f;
   f = EMBRYO_CELL_TO_FLOAT(params[5]);
   value2 = (double)f;
   f = EMBRYO_CELL_TO_FLOAT(params[7]);
   tween = (double)f;
   if ((anim_type >= EDJE_TWEEN_MODE_LINEAR) &&
       (anim_type <= EDJE_TWEEN_MODE_DECELERATE))
     {
        if (HASNPARAMS(8))
          {
             GETSTR(tmp, params[8]);
             if (!tmp) return 0;
             if (!strcmp(tmp, "CURRENT"))
               anim_type |= EDJE_TWEEN_MODE_OPT_FROM_CURRENT;
          }
     }
   else if ((anim_type >= EDJE_TWEEN_MODE_ACCELERATE_FACTOR) &&
            (anim_type <= EDJE_TWEEN_MODE_SINUSOIDAL_FACTOR))
     {
        if (HASNPARAMS(9))
          {
             GETSTR(tmp, params[9]);
             if (!tmp) return 0;
             if (!strcmp(tmp, "CURRENT"))
               anim_type |= EDJE_TWEEN_MODE_OPT_FROM_CURRENT;
          }
        else if (HASNPARAMS(7))
          {
             ERR("parse error. Need 8th parameter to set factor");
             return -1;
          }
        GETFLOAT_T(v1, params[8]);
     }
   else if ((anim_type >= EDJE_TWEEN_MODE_DIVISOR_INTERP) &&
            (anim_type <= EDJE_TWEEN_MODE_SPRING))
     {
        if (HASNPARAMS(10))
          {
             GETSTR(tmp, params[10]);
             if (!tmp) return 0;
             if (!strcmp(tmp, "CURRENT"))
               anim_type |= EDJE_TWEEN_MODE_OPT_FROM_CURRENT;
          }
        else if (HASNPARAMS(7))
          {
             ERR("parse error.Need 8th and 9th parameters to set factor and counts");
             return -1;
          }
        GETFLOAT_T(v1, params[8]);
        GETFLOAT_T(v2, params[9]);
     }
   else if (anim_type == EDJE_TWEEN_MODE_CUBIC_BEZIER)
     {
        if (HASNPARAMS(12))
          {
             GETSTR(tmp, params[10]);
             if (!tmp) return 0;
             if (!strcmp(tmp, "CURRENT"))
               anim_type |= EDJE_TWEEN_MODE_OPT_FROM_CURRENT;
          }
        else if (HASNPARAMS(7))
          {
             ERR("parse error.Need 8th, 9th, 10th and 11th parameters to set x1, y1, x2 and y2");
             return -1;
          }
        if (HASNPARAMS(12))
          {
             GETFLOAT_T(v1, params[8]);
             GETFLOAT_T(v2, params[9]);
             GETFLOAT_T(v3, params[11]);
             GETFLOAT_T(v4, params[12]);
          }
        else
          {
             GETFLOAT_T(v1, params[8]);
             GETFLOAT_T(v2, params[9]);
             GETFLOAT_T(v3, params[10]);
             GETFLOAT_T(v4, params[11]);
          }
     }
   rp = ed->table_parts[part_id % ed->table_parts_size];
   if (!rp) return 0;

   if (rp->program) _edje_program_end(ed, rp->program);
   _edje_part_description_apply(ed, rp, state1, value1, state2, value2);
   _edje_part_pos_set(ed, rp, anim_type, FROM_DOUBLE(tween),
                      v1, v2, v3, v4);
   _edje_recalc(ed);
   return 0;
}

/**
 * @brief Runs an Edje program.
 *
 * This function is exposed to Embryo scripts as `run_program(program_id)`.
 * It executes the Edje program identified by `program_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the program to run.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_run_program(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int program_id = 0;
   Edje_Program *pr;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   program_id = params[1];
   if (program_id < 0) return 0;
   pr = ed->collection->patterns.table_programs[program_id % ed->collection->patterns.table_programs_size];
   if (pr)
     {
        _edje_program_run(ed, pr, 0, "", "", NULL);
     }
   return 0;
}

/**
 * @brief Gets the drag direction of a draggable part.
 *
 * This function is exposed to Embryo scripts as `Direction:get_drag_dir(part_id)`.
 * It returns the drag direction (e.g., EDJE_DRAG_DIR_X, EDJE_DRAG_DIR_XY)
 * for the part identified by `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the draggable part.
 * @return The `Edje_Drag_Dir` enum value representing the drag direction.
 *         Returns 0 if the part is not found or not draggable.
 */
static Embryo_Cell
_edje_embryo_fn_get_drag_dir(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   return edje_object_part_drag_dir_get(ed->obj, rp->part->name);
}

/**
 * @brief Gets the current drag amount of a draggable part.
 *
 * This function is exposed to Embryo scripts as `get_drag(part_id, &Float:dx, &Float:dy)`.
 * It retrieves the current drag displacement (dx, dy) for the part
 * identified by `part_id`. The values are typically between 0.0 and 1.0.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the draggable part.
 *               params[2] is the Embryo cell address to store the X drag amount (float).
 *               params[3] is the Embryo cell address to store the Y drag amount (float).
 * @return Always 0. Drag amounts are returned via output parameters.
 */
static Embryo_Cell
_edje_embryo_fn_get_drag(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;
   double dx = 0.0, dy = 0.0;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   edje_object_part_drag_value_get(ed->obj, rp->part->name, &dx, &dy);
   SETFLOAT(dx, params[2]);
   SETFLOAT(dy, params[3]);

   return 0;
}

/**
 * @brief Sets the current drag amount of a draggable part.
 *
 * This function is exposed to Embryo scripts as `set_drag(part_id, Float:dx, Float:dy)`.
 * It sets the current drag displacement to (dx, dy) for the part
 * identified by `part_id`. The values are typically between 0.0 and 1.0.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the draggable part.
 *               params[2] is the X drag amount (float, as Embryo_Cell).
 *               params[3] is the Y drag amount (float, as Embryo_Cell).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_drag(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   edje_object_part_drag_value_set(ed->obj, rp->part->name,
                                   (double)EMBRYO_CELL_TO_FLOAT(params[2]),
                                   (double)EMBRYO_CELL_TO_FLOAT(params[3]));
   return 0;
}

/**
 * @brief Gets the size of a draggable part.
 *
 * This function is exposed to Embryo scripts as `get_drag_size(part_id, &Float:dx, &Float:dy)`.
 * It retrieves the size (dx, dy) of the draggable area for the part
 * identified by `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the draggable part.
 *               params[2] is the Embryo cell address to store the width (float).
 *               params[3] is the Embryo cell address to store the height (float).
 * @return Always 0. Drag size is returned via output parameters.
 */
static Embryo_Cell
_edje_embryo_fn_get_drag_size(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;
   double dx = 0.0, dy = 0.0;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   edje_object_part_drag_size_get(ed->obj, rp->part->name, &dx, &dy);
   SETFLOAT(dx, params[2]);
   SETFLOAT(dy, params[3]);

   return 0;
}

/**
 * @brief Sets the size of a draggable part.
 *
 * This function is exposed to Embryo scripts as `set_drag_size(part_id, Float:dx, Float:dy)`.
 * It sets the size of the draggable area to (dx, dy) for the part
 * identified by `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the draggable part.
 *               params[2] is the width (float, as Embryo_Cell).
 *               params[3] is the height (float, as Embryo_Cell).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_drag_size(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   edje_object_part_drag_size_set(ed->obj, rp->part->name,
                                  (double)EMBRYO_CELL_TO_FLOAT(params[2]),
                                  (double)EMBRYO_CELL_TO_FLOAT(params[3]));
   return 0;
}

/**
 * @brief Sets the text of a text part.
 *
 * This function is exposed to Embryo scripts as `set_text(part_id, str[])`.
 * It sets the text content of the TEXT or TEXTBLOCK part identified by `part_id`
 * to the given string `str`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the text part.
 *               params[2] is the Embryo cell address of the text string to set.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_text(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;
   char *s;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   GETSTR(s, params[2]);
   if (s)
     {
        edje_object_part_text_set(ed->obj, rp->part->name, s);
     }
   return 0;
}

/**
 * @brief Gets the text of a text part.
 *
 * This function is exposed to Embryo scripts as `get_text(part_id, dst[], maxlen)`.
 * It retrieves the text content of the TEXT or TEXTBLOCK part identified by `part_id`
 * and copies it into the destination buffer `dst`, up to `maxlen` characters.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the text part.
 *               params[2] is the Embryo cell address of the destination string buffer.
 *               params[3] is the maximum length of the destination buffer.
 * @return Always 0. The text is written to the `dst` buffer.
 *         If the text is longer than `maxlen`, it is truncated.
 *         If the part is not a text part or has no text, an empty string is written.
 */
static Embryo_Cell
_edje_embryo_fn_get_text(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;
   char *s;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   s = (char *)edje_object_part_text_get(ed->obj, rp->part->name);
   if (s)
     {
        if ((int)strlen(s) < params[3])
          {
             SETSTR(s, params[2]);
          }
        else
          {
             char *ss;

             ss = alloca(strlen(s) + 1);
             strcpy(ss, s);
             ss[params[3] - 1] = 0;
             SETSTR(ss, params[2]);
          }
     }
   else
     {
        SETSTR("", params[2]);
     }
   return 0;
}

/**
 * @brief Gets the minimum size of the Edje object.
 *
 * This function is exposed to Embryo scripts as `get_min_size(&w, &h)`.
 * It retrieves the minimum width `w` and height `h` that the Edje object
 * can be resized to.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the Embryo cell address to store the minimum width (integer).
 *               params[2] is the Embryo cell address to store the minimum height (integer).
 * @return Always 0. Minimum size is returned via output parameters.
 */
static Embryo_Cell
_edje_embryo_fn_get_min_size(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   Evas_Coord w = 0, h = 0;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);
   edje_object_size_min_get(ed->obj, &w, &h);
   SETINT(w, params[1]);
   SETINT(h, params[2]);
   return 0;
}

/**
 * @brief Gets the maximum size of the Edje object.
 *
 * This function is exposed to Embryo scripts as `get_max_size(&w, &h)`.
 * It retrieves the maximum width `w` and height `h` that the Edje object
 * can be resized to.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the Embryo cell address to store the maximum width (integer).
 *               params[2] is the Embryo cell address to store the maximum height (integer).
 * @return Always 0. Maximum size is returned via output parameters.
 */
static Embryo_Cell
_edje_embryo_fn_get_max_size(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   Evas_Coord w = 0, h = 0;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);
   edje_object_size_max_get(ed->obj, &w, &h);
   SETINT(w, params[1]);
   SETINT(h, params[2]);
   return 0;
}

/**
 * @brief Gets the RGBA values of a defined color class.
 *
 * This function is exposed to Embryo scripts as `get_color_class(class[], &r, &g, &b, &a)`.
 * It retrieves the red, green, blue, and alpha components of the color class
 * specified by `class[]`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the Embryo cell address of the color class name string.
 *               params[2] is the Embryo cell address to store the red component (0-255).
 *               params[3] is the Embryo cell address to store the green component (0-255).
 *               params[4] is the Embryo cell address to store the blue component (0-255).
 *               params[5] is the Embryo cell address to store the alpha component (0-255).
 * @return 0 if the color class is found and values are retrieved, otherwise 0 (no explicit error return).
 *         If the class is not found, the output parameters are not modified.
 */
static Embryo_Cell
_edje_embryo_fn_get_color_class(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   Edje_Color_Class *c_class;
   char *class;

   CHKPARAM(5);
   ed = embryo_program_data_get(ep);
   GETSTR(class, params[1]);
   if (!class) return 0;
   c_class = _edje_color_class_recursive_find(ed, class);
   if (!c_class) return 0;
   SETINT(c_class->r, params[2]);
   SETINT(c_class->g, params[3]);
   SETINT(c_class->b, params[4]);
   SETINT(c_class->a, params[5]);
   return 0;
}

/**
 * @brief Sets the RGBA values of a defined color class.
 *
 * This function is exposed to Embryo scripts as `set_color_class(class[], r, g, b, a)`.
 * It sets the red, green, blue, and alpha components for the color class
 * specified by `class[]`. This also sets outline and shadow colors to the same values.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the Embryo cell address of the color class name string.
 *               params[2] is the red component (0-255).
 *               params[3] is the green component (0-255).
 *               params[4] is the blue component (0-255).
 *               params[5] is the alpha component (0-255).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_color_class(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *class;

   CHKPARAM(5);
   ed = embryo_program_data_get(ep);
   GETSTR(class, params[1]);
   if (!class) return 0;
   edje_object_color_class_set(ed->obj, class, params[2], params[3], params[4],
                               params[5], params[2], params[3], params[4],
                               params[5], params[2], params[3], params[4],
                               params[5]);
   return 0;
}

/**
 * @brief Sets the font and size for a defined text class.
 *
 * This function is exposed to Embryo scripts as `set_text_class(class[], font[], Float:size)`.
 * It sets the font name and font size for the text class specified by `class[]`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the Embryo cell address of the text class name string.
 *               params[2] is the Embryo cell address of the font name string.
 *               params[3] is the font size (float, as Embryo_Cell).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_text_class(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *class, *font;
   Evas_Font_Size fsize;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   GETSTR(class, params[1]);
   GETSTR(font, params[2]);
   if ( !class || !font ) return 0;
   fsize = (Evas_Font_Size)EMBRYO_CELL_TO_FLOAT(params[3]);
   edje_object_text_class_set(ed->obj, class, font, fsize);
   return 0;
}

/**
 * @brief Gets the font and size of a defined text class.
 *
 * This function is exposed to Embryo scripts as `get_text_class(class[], font[], &Float:size)`.
 * It retrieves the font name and font size for the text class specified by `class[]`.
 * The font name is copied into the `font[]` buffer.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the Embryo cell address of the text class name string.
 *               params[2] is the Embryo cell address of the destination string buffer for the font name.
 *                        (Note: The size of this buffer is not passed, ensure it's large enough.)
 *               params[3] is the Embryo cell address to store the font size (float).
 * @return 0 if the text class is found and values are retrieved, otherwise 0 (no explicit error return).
 *         If the class is not found, the output parameters are not modified.
 */
static Embryo_Cell
_edje_embryo_fn_get_text_class(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *class;
   Edje_Text_Class *t_class;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   GETSTR(class, params[1]);
   if (!class) return 0;
   t_class = _edje_text_class_find(ed, class);
   if (!t_class) return 0;
   SETSTR((char *)t_class->font, params[2]);
   SETFLOAT(t_class->size, params[3]);
   return 0;
}

/**
 * @brief Gets the step values for a draggable part.
 *
 * This function is exposed to Embryo scripts as `get_drag_step(part_id, &Float:dx, &Float:dy)`.
 * It retrieves the X and Y step values (dx, dy) for the draggable part
 * identified by `part_id`. Step values control how much the drag value changes per pixel moved.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the draggable part.
 *               params[2] is the Embryo cell address to store the X step value (float).
 *               params[3] is the Embryo cell address to store the Y step value (float).
 * @return Always 0. Step values are returned via output parameters.
 */
static Embryo_Cell
_edje_embryo_fn_get_drag_step(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;
   double dx = 0.0, dy = 0.0;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   edje_object_part_drag_step_get(ed->obj, rp->part->name, &dx, &dy);
   SETFLOAT(dx, params[2]);
   SETFLOAT(dy, params[3]);

   return 0;
}

/**
 * @brief Sets the step values for a draggable part.
 *
 * This function is exposed to Embryo scripts as `set_drag_step(part_id, Float:dx, Float:dy)`.
 * It sets the X and Y step values to (dx, dy) for the draggable part
 * identified by `part_id`. Step values control how much the drag value changes per pixel moved.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the draggable part.
 *               params[2] is the X step value (float, as Embryo_Cell).
 *               params[3] is the Y step value (float, as Embryo_Cell).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_drag_step(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   edje_object_part_drag_step_set(ed->obj, rp->part->name,
                                  (double)EMBRYO_CELL_TO_FLOAT(params[2]),
                                  (double)EMBRYO_CELL_TO_FLOAT(params[3]));
   return 0;
}

/**
 * @brief Gets the page values for a draggable part.
 *
 * This function is exposed to Embryo scripts as `get_drag_page(part_id, &Float:dx, &Float:dy)`.
 * It retrieves the X and Y page values (dx, dy) for the draggable part
 * identified by `part_id`. Page values are used for page-by-page scrolling/dragging.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the draggable part.
 *               params[2] is the Embryo cell address to store the X page value (float).
 *               params[3] is the Embryo cell address to store the Y page value (float).
 * @return Always 0. Page values are returned via output parameters.
 */
static Embryo_Cell
_edje_embryo_fn_get_drag_page(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;
   double dx = 0.0, dy = 0.0;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   edje_object_part_drag_page_get(ed->obj, rp->part->name, &dx, &dy);
   SETFLOAT(dx, params[2]);
   SETFLOAT(dy, params[3]);

   return 0;
}

/**
 * @brief Gets the geometry of an Edje part.
 *
 * This function is exposed to Embryo scripts as `get_geometry(part_id, &x, &y, &w, &h)`.
 * It retrieves the X, Y, width, and height of the part identified by `part_id`.
 * Coordinates are relative to the Edje object.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 *               params[2] is the Embryo cell address to store the X coordinate (integer).
 *               params[3] is the Embryo cell address to store the Y coordinate (integer).
 *               params[4] is the Embryo cell address to store the width (integer).
 *               params[5] is the Embryo cell address to store the height (integer).
 * @return Always 0. Geometry is returned via output parameters.
 */
static Embryo_Cell
_edje_embryo_fn_get_geometry(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;
   Evas_Coord x = 0, y = 0, w = 0, h = 0;

   CHKPARAM(5);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   edje_object_part_geometry_get(ed->obj, rp->part->name, &x, &y, &w, &h);
   SETINT(x, params[2]);
   SETINT(y, params[3]);
   SETINT(w, params[4]);
   SETINT(h, params[5]);

   return 0;
}

/**
 * @brief Sets the page values for a draggable part.
 *
 * This function is exposed to Embryo scripts as `set_drag_page(part_id, Float:dx, Float:dy)`.
 * It sets the X and Y page values to (dx, dy) for the draggable part
 * identified by `part_id`. Page values are used for page-by-page scrolling/dragging.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the draggable part.
 *               params[2] is the X page value (float, as Embryo_Cell).
 *               params[3] is the Y page value (float, as Embryo_Cell).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_drag_page(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   edje_object_part_drag_page_set(ed->obj, rp->part->name,
                                  (double)EMBRYO_CELL_TO_FLOAT(params[2]),
                                  (double)EMBRYO_CELL_TO_FLOAT(params[3]));
   return 0;
}

/**
 * @brief Sends a message from an Edje script to the application or other Edje objects.
 *
 * This function is exposed to Embryo scripts as `send_message(Msg_Type:type, id, ...)`.
 * It constructs and sends a message of a specific `type` with an `id` and
 * variable payload depending on the message type.
 *
 * Message types and their expected parameters:
 * - `EDJE_MESSAGE_NONE`: `id`
 * - `EDJE_MESSAGE_STRING`: `id, string_val[]`
 * - `EDJE_MESSAGE_INT`: `id, int_val`
 * - `EDJE_MESSAGE_FLOAT`: `id, Float:float_val`
 * - `EDJE_MESSAGE_STRING_SET`: `id, string1[], string2[], ...`
 *   - `params[0]` indicates total size, used to determine count of strings.
 * - `EDJE_MESSAGE_INT_SET`: `id, int1, int2, ...`
 *   - `params[0]` indicates total size, used to determine count of ints.
 * - `EDJE_MESSAGE_FLOAT_SET`: `id, Float:float1, Float:float2, ...`
 *   - `params[0]` indicates total size, used to determine count of floats.
 * - `EDJE_MESSAGE_STRING_INT`: `id, string_val[], int_val`
 * - `EDJE_MESSAGE_STRING_FLOAT`: `id, string_val[], Float:float_val`
 * - `EDJE_MESSAGE_STRING_INT_SET`: `id, string_val[], int1, int2, ...`
 *   - `params[0]` indicates total size, used to determine count of ints.
 * - `EDJE_MESSAGE_STRING_FLOAT_SET`: `id, string_val[], Float:float1, Float:float2, ...`
 *   - `params[0]` indicates total size, used to determine count of floats.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the total size of parameters in bytes.
 *               params[1] is the `Edje_Message_Type` (integer).
 *               params[2] is the message ID (integer).
 *               params[3+] are the message payload, varying by type.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_send_message(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   Edje_Message_Type type;
   int id, i, n;
   Embryo_Cell *ptr;

   if (params[0] < (int)(sizeof(Embryo_Cell) * (2))) return 0;
   ed = embryo_program_data_get(ep);
   type = params[1];
   id = params[2];
   switch (type)
     {
      case EDJE_MESSAGE_NONE:
        _edje_util_message_send(ed, EDJE_QUEUE_APP, type, id, NULL);
        break;

      case EDJE_MESSAGE_SIGNAL:
        break;

      case EDJE_MESSAGE_STRING:
      {
         Embryo_Cell *cptr;

         cptr = embryo_data_address_get(ep, params[3]);
         if (cptr)
           {
              Edje_Message_String *emsg;
              int l;
              char *s;

              l = embryo_data_string_length_get(ep, cptr);
              s = alloca(l + 1);
              s[0] = 0;
              embryo_data_string_get(ep, cptr, s);
              emsg = alloca(sizeof(Edje_Message_String));
              emsg->str = s;
              _edje_util_message_send(ed, EDJE_QUEUE_APP, type, id, emsg);
           }
      }
      break;

      case EDJE_MESSAGE_INT:
      {
         Edje_Message_Int *emsg;

         emsg = alloca(sizeof(Edje_Message_Int));
         ptr = embryo_data_address_get(ep, params[3]);
         if (ptr) emsg->val = (int)*ptr;
         else emsg->val = 0;
         _edje_util_message_send(ed, EDJE_QUEUE_APP, type, id, emsg);
      }
      break;

      case EDJE_MESSAGE_FLOAT:
      {
         Edje_Message_Float *emsg;
         float f;

         emsg = alloca(sizeof(Edje_Message_Float));
         ptr = embryo_data_address_get(ep, params[3]);
         if (ptr)
           {
              f = EMBRYO_CELL_TO_FLOAT(*ptr);
              emsg->val = (double)f;
           }
         else
           emsg->val = 0.0;
         _edje_util_message_send(ed, EDJE_QUEUE_APP, type, id, emsg);
      }
      break;

      case EDJE_MESSAGE_STRING_SET:
      {
         Edje_Message_String_Set *emsg;

         n = (params[0] / sizeof(Embryo_Cell)) + 1;
         emsg = alloca(sizeof(Edje_Message_String_Set) + ((n - 3 - 1) * sizeof(char *)));
         emsg->count = n - 3;
         for (i = 3; i < n; i++)
           {
              Embryo_Cell *cptr;

              cptr = embryo_data_address_get(ep, params[i]);
              if (cptr)
                {
                   int l;
                   char *s;

                   l = embryo_data_string_length_get(ep, cptr);
                   s = alloca(l + 1);
                   s[0] = 0;
                   embryo_data_string_get(ep, cptr, s);
                   emsg->str[i - 3] = s;
                }
           }
         _edje_util_message_send(ed, EDJE_QUEUE_APP, type, id, emsg);
      }
      break;

      case EDJE_MESSAGE_INT_SET:
      {
         Edje_Message_Int_Set *emsg;

         n = (params[0] / sizeof(Embryo_Cell)) + 1;
         emsg = alloca(sizeof(Edje_Message_Int_Set) + ((n - 3 - 1) * sizeof(int)));
         emsg->count = n - 3;
         for (i = 3; i < n; i++)
           {
              ptr = embryo_data_address_get(ep, params[i]);
              if (ptr) emsg->val[i - 3] = (int)*ptr;
              else emsg->val[i - 3] = 0;
           }
         _edje_util_message_send(ed, EDJE_QUEUE_APP, type, id, emsg);
      }
      break;

      case EDJE_MESSAGE_FLOAT_SET:
      {
         Edje_Message_Float_Set *emsg;

         n = (params[0] / sizeof(Embryo_Cell)) + 1;
         emsg = alloca(sizeof(Edje_Message_Float_Set) + ((n - 3 - 1) * sizeof(double)));
         emsg->count = n - 3;
         for (i = 3; i < n; i++)
           {
              float f;

              ptr = embryo_data_address_get(ep, params[i]);
              if (ptr)
                {
                   f = EMBRYO_CELL_TO_FLOAT(*ptr);
                   emsg->val[i - 3] = (double)f;
                }
              else
                emsg->val[i - 3] = 0.0;
           }
         _edje_util_message_send(ed, EDJE_QUEUE_APP, type, id, emsg);
      }
      break;

      case EDJE_MESSAGE_STRING_INT:
      {
         Edje_Message_String_Int *emsg;
         Embryo_Cell *cptr;

         cptr = embryo_data_address_get(ep, params[3]);
         if (cptr)
           {
              int l;
              char *s;

              l = embryo_data_string_length_get(ep, cptr);
              s = alloca(l + 1);
              s[0] = 0;
              embryo_data_string_get(ep, cptr, s);
              emsg = alloca(sizeof(Edje_Message_String_Int));
              emsg->str = s;
              ptr = embryo_data_address_get(ep, params[4]);
              if (ptr) emsg->val = (int)*ptr;
              else emsg->val = 0;
              _edje_util_message_send(ed, EDJE_QUEUE_APP, type, id, emsg);
           }
      }
      break;

      case EDJE_MESSAGE_STRING_FLOAT:
      {
         Edje_Message_String_Float *emsg;
         Embryo_Cell *cptr;

         cptr = embryo_data_address_get(ep, params[3]);
         if (cptr)
           {
              int l;
              char *s;
              float f;

              l = embryo_data_string_length_get(ep, cptr);
              s = alloca(l + 1);
              s[0] = 0;
              embryo_data_string_get(ep, cptr, s);
              emsg = alloca(sizeof(Edje_Message_String_Float));
              emsg->str = s;
              ptr = embryo_data_address_get(ep, params[4]);
              if (ptr)
                {
                   f = EMBRYO_CELL_TO_FLOAT(*ptr);
                   emsg->val = (double)f;
                }
              else
                emsg->val = 0.0;
              _edje_util_message_send(ed, EDJE_QUEUE_APP, type, id, emsg);
           }
      }
      break;

      case EDJE_MESSAGE_STRING_INT_SET:
      {
         Edje_Message_String_Int_Set *emsg;
         Embryo_Cell *cptr;

         cptr = embryo_data_address_get(ep, params[3]);
         if (cptr)
           {
              int l;
              char *s;

              l = embryo_data_string_length_get(ep, cptr);
              s = alloca(l + 1);
              s[0] = 0;
              embryo_data_string_get(ep, cptr, s);
              n = (params[0] / sizeof(Embryo_Cell)) + 1;
              emsg = alloca(sizeof(Edje_Message_String_Int_Set) + ((n - 4 - 1) * sizeof(int)));
              emsg->str = s;
              emsg->count = n - 4;
              for (i = 4; i < n; i++)
                {
                   ptr = embryo_data_address_get(ep, params[i]);
                   if (ptr) emsg->val[i - 4] = (int)*ptr;
                   else emsg->val[i - 4] = 0;
                }
              _edje_util_message_send(ed, EDJE_QUEUE_APP, type, id, emsg);
           }
      }
      break;

      case EDJE_MESSAGE_STRING_FLOAT_SET:
      {
         Edje_Message_String_Float_Set *emsg;
         Embryo_Cell *cptr;

         cptr = embryo_data_address_get(ep, params[3]);
         if (cptr)
           {
              int l;
              char *s;

              l = embryo_data_string_length_get(ep, cptr);
              s = alloca(l + 1);
              s[0] = 0;
              embryo_data_string_get(ep, cptr, s);
              n = (params[0] / sizeof(Embryo_Cell)) + 1;
              emsg = alloca(sizeof(Edje_Message_String_Float_Set) + ((n - 4 - 1) * sizeof(double)));
              emsg->str = s;
              emsg->count = n - 4;
              for (i = 4; i < n; i++)
                {
                   float f;

                   ptr = embryo_data_address_get(ep, params[i]);
                   if (ptr)
                     {
                        f = EMBRYO_CELL_TO_FLOAT(*ptr);
                        emsg->val[i - 4] = (double)f;
                     }
                   else
                     emsg->val[i - 4] = 0.0;
                }
              _edje_util_message_send(ed, EDJE_QUEUE_APP, type, id, emsg);
           }
      }
      break;

      default:
        break;
     }
   return 0;
}

/**
 * @brief Creates a new custom state for a part, based on an existing state.
 *
 * This function is exposed to Embryo scripts as `custom_state(part_id, state[], Float:state_val = 0.0)`.
 * It allows a script to define a new, modifiable state named "custom" for a part.
 * This "custom" state is initialized as a copy of an existing `state` definition
 * (e.g., "default") at a specific `state_val`. Once created, properties of this
 * "custom" state can be modified using `set_state_val()`.
 *
 * If a "custom" state already exists for the part, this function does nothing.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 *               params[2] is the Embryo cell address of the base state name string (e.g., "default").
 *               params[3] is the value of the base state (float, as Embryo_Cell).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_custom_state(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);
   Edje_Real_Part *rp;
   Edje_Part_Description_Common *parent, *d = NULL;
   char *name;
   float val;

   CHKPARAM(3);

   if (params[1] < 0)
     return 0;

   if (!(rp = ed->table_parts[params[1] % ed->table_parts_size]))
     return 0;

   /* check whether this part already has a "custom" state */
   if (rp->custom)
     return 0;

   GETSTR(name, params[2]);
   if (!name)
     return 0;

   val = EMBRYO_CELL_TO_FLOAT(params[3]);

   if (!(parent = _edje_part_description_find(ed, rp, name, val, EINA_TRUE)))
     return 0;

   rp->custom = eina_mempool_malloc(_edje_real_part_state_mp, sizeof (Edje_Real_Part_State));
   if (!rp->custom) return 0;

   memset(rp->custom, 0, sizeof (Edje_Real_Part_State));

   /* now create the custom state */
   switch (rp->part->type)
     {
#define ALLOC_DESC(Short, Type, To) \
case EDJE_PART_TYPE_##Short: To = calloc(1, sizeof (Edje_Part_Description_##Type)); break;

#define ALLOC_COPY_DESC(Short, Type, To, Spec)             \
case EDJE_PART_TYPE_##Short:                               \
{                                                          \
   Edje_Part_Description_##Type * tmp;                     \
   Edje_Part_Description_##Type * new;                     \
   tmp = (Edje_Part_Description_##Type *)parent;           \
   new = calloc(1, sizeof (Edje_Part_Description_##Type)); \
   if (!new) break;                                        \
   new->Spec = tmp->Spec;                                  \
   To = &new->common;                                      \
   break;                                                  \
}

        ALLOC_DESC(RECTANGLE, Common, d);
        ALLOC_DESC(SPACER, Common, d);
        ALLOC_DESC(SWALLOW, Common, d);
        ALLOC_DESC(GROUP, Common, d);

        ALLOC_COPY_DESC(IMAGE, Image, d, image);
        ALLOC_COPY_DESC(PROXY, Proxy, d, proxy);
        ALLOC_COPY_DESC(TEXT, Text, d, text);
        ALLOC_COPY_DESC(TEXTBLOCK, Text, d, text);
        ALLOC_COPY_DESC(BOX, Box, d, box);
        ALLOC_COPY_DESC(TABLE, Table, d, table);
        ALLOC_COPY_DESC(EXTERNAL, External, d, external_params);
        ALLOC_COPY_DESC(VECTOR, Vector, d, vg);
     }

   if (!d)
     {
        eina_mempool_free(_edje_real_part_state_mp, rp->custom);
        rp->custom = NULL;
        return 0;
     }

   *d = *parent;

   d->state.name = (char *)eina_stringshare_add("custom");
   d->state.value = 0.0;

   /* make sure all the allocated memory is getting copied,
    * not just referenced
    */
   if (rp->part->type == EDJE_PART_TYPE_IMAGE)
     {
        Edje_Part_Description_Image *img_desc;
        Edje_Part_Description_Image *parent_img_desc;

        img_desc = (Edje_Part_Description_Image *)d;
        parent_img_desc = (Edje_Part_Description_Image *)parent;

        img_desc->image.tweens_count = parent_img_desc->image.tweens_count;
        img_desc->image.tweens = calloc(img_desc->image.tweens_count,
                                        sizeof(Edje_Part_Image_Id *));
        if (img_desc->image.tweens)
          {
             unsigned int i;

             for (i = 0; i < parent_img_desc->image.tweens_count; ++i)
               {
                  Edje_Part_Image_Id *iid_new;

                  iid_new = calloc(1, sizeof(Edje_Part_Image_Id));
                  if (!iid_new) continue;

                  *iid_new = *parent_img_desc->image.tweens[i];

                  img_desc->image.tweens[i] = iid_new;
               }
          }
     }

#define DUP(x) x ? (char *)eina_stringshare_add(x) : NULL
   d->color_class = DUP(d->color_class);

   if (rp->part->type == EDJE_PART_TYPE_TEXT
       || rp->part->type == EDJE_PART_TYPE_TEXTBLOCK)
     {
        Edje_Part_Description_Text *text_desc;

        text_desc = (Edje_Part_Description_Text *)d;

        text_desc->text.text_class = DUP(text_desc->text.text_class);
        text_desc->text.text.str = DUP(edje_string_get(&text_desc->text.text));
        text_desc->text.text.id = 0;
        text_desc->text.text.translated = NULL;
        text_desc->text.domain = DUP(text_desc->text.domain);
        text_desc->text.font.str = DUP(edje_string_get(&text_desc->text.font));
        text_desc->text.font.id = 0;
        text_desc->text.style.str = DUP(edje_string_get(&text_desc->text.style));
        text_desc->text.style.id = 0;
     }
#undef DUP

   rp->custom->description = d;

   return 0;
}

/**
 * @brief Modifies a specific parameter of a part's "custom" state.
 *
 * This function is exposed to Embryo scripts as `set_state_val(part_id, State_Param:p, ...)`.
 * It allows changing individual properties (e.g., alignment, color, text) of the
 * "custom" state previously created for `part_id` by `custom_state()`.
 * The `State_Param:p` determines which property to change, and subsequent parameters
 * provide the new value(s) for that property.
 *
 * Example: `set_state_val(my_part_id, EDJE_STATE_PARAM_COLOR, 255, 0, 0, 255);` // Set color to red
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the total size of parameters in bytes.
 *               params[1] is the ID of the part.
 *               params[2] is the `Edje_State_Param` enum value indicating the property to set.
 *               params[3+] are the value(s) for the specified property. The number and type
 *                          of these values depend on `params[2]`.
 * @return Always 0. Triggers a recalc of the Edje object.
 */
static Embryo_Cell
_edje_embryo_fn_set_state_val(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);
   Edje_Real_Part *rp;
   char *s;

   /* we need at least 3 arguments */
   if (params[0] < (int)(sizeof(Embryo_Cell) * 3))
     return 0;

   if (params[1] < 0)
     return 0;

   if (!(rp = ed->table_parts[params[1] % ed->table_parts_size]))
     return 0;

   /* check whether this part has a "custom" state */
   if (!rp->custom)
     return 0;

   switch (params[2])
     {
      case EDJE_STATE_PARAM_ALIGNMENT:
        CHKPARAM(4);

        GETFLOAT_T(rp->custom->description->align.x, params[3]);
        GETFLOAT_T(rp->custom->description->align.y, params[4]);

        break;

      case EDJE_STATE_PARAM_MIN:
        CHKPARAM(4);

        GETINT(rp->custom->description->min.w, params[3]);
        GETINT(rp->custom->description->min.h, params[4]);

        break;

      case EDJE_STATE_PARAM_MAX:
        CHKPARAM(4);

        GETINT(rp->custom->description->max.w, params[3]);
        GETINT(rp->custom->description->max.h, params[4]);

        break;

      case EDJE_STATE_PARAM_STEP:
        CHKPARAM(4);

        GETINT(rp->custom->description->step.x, params[3]);
        GETINT(rp->custom->description->step.y, params[4]);

        break;

      case EDJE_STATE_PARAM_ASPECT:
        CHKPARAM(4);

        GETFLOAT_T(rp->custom->description->aspect.min, params[3]);
        GETFLOAT_T(rp->custom->description->aspect.max, params[4]);

        break;

      case EDJE_STATE_PARAM_ASPECT_PREF:
        CHKPARAM(3);

        GETINT(rp->custom->description->aspect.prefer, params[3]);

        break;

      case EDJE_STATE_PARAM_COLOR:
        CHKPARAM(6);

        GETINT(rp->custom->description->color.r, params[3]);
        GETINT(rp->custom->description->color.g, params[4]);
        GETINT(rp->custom->description->color.b, params[5]);
        GETINT(rp->custom->description->color.a, params[6]);

        break;

      case EDJE_STATE_PARAM_COLOR2:
        CHKPARAM(6);

        GETINT(rp->custom->description->color2.r, params[3]);
        GETINT(rp->custom->description->color2.g, params[4]);
        GETINT(rp->custom->description->color2.b, params[5]);
        GETINT(rp->custom->description->color2.a, params[6]);

        break;

      case EDJE_STATE_PARAM_COLOR3:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
             (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
           return 0;

         CHKPARAM(6);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         GETINT(text->text.color3.r, params[3]);
         GETINT(text->text.color3.g, params[4]);
         GETINT(text->text.color3.b, params[5]);
         GETINT(text->text.color3.a, params[6]);
         break;
      }

      case EDJE_STATE_PARAM_COLOR_CLASS:
        CHKPARAM(3);

        GETSTR(s, params[3]);
        GETSTREVAS(s, rp->custom->description->color_class);

        break;

      case EDJE_STATE_PARAM_REL1:
        CHKPARAM(4);

        GETFLOAT_T(rp->custom->description->rel1.relative_x, params[3]);
        GETFLOAT_T(rp->custom->description->rel1.relative_y, params[4]);

        break;

      case EDJE_STATE_PARAM_REL1_TO:
        CHKPARAM(4);

        GETINT(rp->custom->description->rel1.id_x, params[3]);
        GETINT(rp->custom->description->rel1.id_y, params[4]);

        break;

      case EDJE_STATE_PARAM_REL1_OFFSET:
        CHKPARAM(4);

        GETINT(rp->custom->description->rel1.offset_x, params[3]);
        GETINT(rp->custom->description->rel1.offset_y, params[4]);

        break;

      case EDJE_STATE_PARAM_REL2:
        CHKPARAM(4);

        GETFLOAT_T(rp->custom->description->rel2.relative_x, params[3]);
        GETFLOAT_T(rp->custom->description->rel2.relative_y, params[4]);

        break;

      case EDJE_STATE_PARAM_REL2_TO:
        CHKPARAM(4);

        GETINT(rp->custom->description->rel2.id_x, params[3]);
        GETINT(rp->custom->description->rel2.id_y, params[4]);

        break;

      case EDJE_STATE_PARAM_REL2_OFFSET:
        CHKPARAM(4);

        GETINT(rp->custom->description->rel2.offset_x, params[3]);
        GETINT(rp->custom->description->rel2.offset_y, params[4]);

        break;

      case EDJE_STATE_PARAM_IMAGE:
      {
         Edje_Part_Description_Image *img;

         if ((rp->part->type != EDJE_PART_TYPE_IMAGE)) return 0;
         CHKPARAM(3);

         img = (Edje_Part_Description_Image *)rp->custom->description;
         GETINT(img->image.id, params[3]);

         break;
      }

      case EDJE_STATE_PARAM_BORDER:
      {
         Edje_Part_Description_Image *img;

         if ((rp->part->type != EDJE_PART_TYPE_IMAGE)) return 0;
         CHKPARAM(6);

         img = (Edje_Part_Description_Image *)rp->custom->description;

         GETINT(img->image.border.l, params[3]);
         GETINT(img->image.border.r, params[4]);
         GETINT(img->image.border.t, params[5]);
         GETINT(img->image.border.b, params[6]);

         break;
      }

      case EDJE_STATE_PARAM_FILL_SMOOTH:
      {
         Edje_Part_Description_Image *img;
         Edje_Part_Description_Proxy *proxy;

         if ((rp->part->type != EDJE_PART_TYPE_IMAGE) && (rp->part->type != EDJE_PART_TYPE_PROXY)) return 0;
         CHKPARAM(3);

         if (rp->part->type == EDJE_PART_TYPE_IMAGE)
           {
              img = (Edje_Part_Description_Image *)rp->custom->description;
              GETINT(img->image.fill.smooth, params[3]);
           }
         else
           {
              proxy = (Edje_Part_Description_Proxy *)rp->custom->description;
              GETINT(proxy->proxy.fill.smooth, params[3]);
           }

         break;
      }

      case EDJE_STATE_PARAM_FILL_POS:
      {
         Edje_Part_Description_Image *img;
         Edje_Part_Description_Proxy *proxy;

         if ((rp->part->type != EDJE_PART_TYPE_IMAGE) && (rp->part->type != EDJE_PART_TYPE_PROXY)) return 0;
         CHKPARAM(6);

         if (rp->part->type == EDJE_PART_TYPE_IMAGE)
           {
              img = (Edje_Part_Description_Image *)rp->custom->description;

              GETFLOAT_T(img->image.fill.pos_rel_x, params[3]);
              GETFLOAT_T(img->image.fill.pos_rel_y, params[4]);
              GETINT(img->image.fill.pos_abs_x, params[5]);
              GETINT(img->image.fill.pos_abs_y, params[6]);
           }
         else
           {
              proxy = (Edje_Part_Description_Proxy *)rp->custom->description;

              GETFLOAT_T(proxy->proxy.fill.pos_rel_x, params[3]);
              GETFLOAT_T(proxy->proxy.fill.pos_rel_y, params[4]);
              GETINT(proxy->proxy.fill.pos_abs_x, params[5]);
              GETINT(proxy->proxy.fill.pos_abs_y, params[6]);
           }

         break;
      }

      case EDJE_STATE_PARAM_FILL_SIZE:
      {
         Edje_Part_Description_Image *img;
         Edje_Part_Description_Proxy *proxy;

         if ((rp->part->type != EDJE_PART_TYPE_IMAGE) && (rp->part->type != EDJE_PART_TYPE_PROXY)) return 0;
         CHKPARAM(6);

         if (rp->part->type == EDJE_PART_TYPE_IMAGE)
           {
              img = (Edje_Part_Description_Image *)rp->custom->description;

              GETFLOAT_T(img->image.fill.rel_x, params[3]);
              GETFLOAT_T(img->image.fill.rel_y, params[4]);
              GETINT(img->image.fill.abs_x, params[5]);
              GETINT(img->image.fill.abs_y, params[6]);
           }
         else
           {
              proxy = (Edje_Part_Description_Proxy *)rp->custom->description;

              GETFLOAT_T(proxy->proxy.fill.rel_x, params[3]);
              GETFLOAT_T(proxy->proxy.fill.rel_y, params[4]);
              GETINT(proxy->proxy.fill.abs_x, params[5]);
              GETINT(proxy->proxy.fill.abs_y, params[6]);
           }

         break;
      }

      case EDJE_STATE_PARAM_TEXT:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
             (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
           return 0;
         CHKPARAM(3);

         GETSTR(s, params[3]);

         text = (Edje_Part_Description_Text *)rp->custom->description;
         GETSTREVAS(s, text->text.text.str);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_CLASS:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
             (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
           return 0;
         CHKPARAM(3);

         GETSTR(s, params[3]);

         text = (Edje_Part_Description_Text *)rp->custom->description;
         GETSTREVAS(s, text->text.text_class);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_FONT:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT)) return 0;
         CHKPARAM(3);

         GETSTR(s, params[3]);

         text = (Edje_Part_Description_Text *)rp->custom->description;
         GETSTREVAS(s, text->text.font.str);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_STYLE:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXTBLOCK)) return 0;
         CHKPARAM(3);

         GETSTR(s, params[3]);

         text = (Edje_Part_Description_Text *)rp->custom->description;
         GETSTREVAS(s, text->text.style.str);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_SIZE:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT)) return 0;
         CHKPARAM(3);

         text = (Edje_Part_Description_Text *)rp->custom->description;
         GETINT(text->text.size, params[3]);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_FIT:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT)) return 0;
         CHKPARAM(4);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         GETINT(text->text.fit_x, params[3]);
         GETINT(text->text.fit_y, params[4]);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_MIN:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
             (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
           return 0;
         CHKPARAM(4);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         GETINT(text->text.min_x, params[3]);
         GETINT(text->text.min_y, params[4]);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_MAX:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
             (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
           return 0;
         CHKPARAM(4);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         GETINT(text->text.max_x, params[3]);
         GETINT(text->text.max_y, params[4]);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_ALIGN:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT)) return 0;
         CHKPARAM(4);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         GETFLOAT_T(text->text.align.x, params[3]);
         GETFLOAT_T(text->text.align.y, params[4]);

         break;
      }

      case EDJE_STATE_PARAM_VISIBLE:
        CHKPARAM(3);

        GETINT(rp->custom->description->visible, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_ON:
        CHKPARAM(3);

        GETINT(rp->custom->description->map.on, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_PERSP:
        CHKPARAM(3);

        GETINT(rp->custom->description->map.id_persp, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_LIGHT:
        CHKPARAM(3);

        GETINT(rp->custom->description->map.id_light, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_ROT_CENTER:
        CHKPARAM(3);

        GETINT(rp->custom->description->map.rot.id_center, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_ROT_X:
        CHKPARAM(3);

        GETFLOAT_T(rp->custom->description->map.rot.x, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_ROT_Y:
        CHKPARAM(3);

        GETFLOAT_T(rp->custom->description->map.rot.y, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_ROT_Z:
        CHKPARAM(3);

        GETFLOAT_T(rp->custom->description->map.rot.z, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_BACK_CULL:
        CHKPARAM(3);

        GETINT(rp->custom->description->map.backcull, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_PERSP_ON:
        CHKPARAM(3);

        GETINT(rp->custom->description->map.persp_on, params[3]);

        break;

      case EDJE_STATE_PARAM_PERSP_ZPLANE:
        CHKPARAM(3);

        GETINT(rp->custom->description->persp.zplane, params[3]);

        break;

      case EDJE_STATE_PARAM_PERSP_FOCAL:
        CHKPARAM(3);

        GETINT(rp->custom->description->persp.focal, params[3]);

        break;

      case EDJE_STATE_PARAM_PROXY_SRC_CLIP:
      {
         Edje_Part_Description_Proxy *proxy;

         if ((rp->part->type != EDJE_PART_TYPE_PROXY)) return 0;
         CHKPARAM(3);

         proxy = (Edje_Part_Description_Proxy *)rp->custom->description;
         GETINT(proxy->proxy.source_clip, params[3]);

         break;
      }

      case EDJE_STATE_PARAM_PROXY_SRC_VISIBLE:
      {
         Edje_Part_Description_Proxy *proxy;

         if ((rp->part->type != EDJE_PART_TYPE_PROXY)) return 0;
         CHKPARAM(3);

         proxy = (Edje_Part_Description_Proxy *)rp->custom->description;
         GETINT(proxy->proxy.source_visible, params[3]);

         break;
      }

      case EDJE_STATE_PARAM_BOX_LAYOUT:
      {
         Edje_Part_Description_Box *box;
         if ((rp->part->type != EDJE_PART_TYPE_BOX)) return 0;
         CHKPARAM(3);

         GETSTR(s, params[3]);
         s = strdup(s);

         box = (Edje_Part_Description_Box *)rp->custom->description;
         box->box.layout = s;

         break;
      }

      case EDJE_STATE_PARAM_BOX_FALLBACK_LAYOUT:
      {
         Edje_Part_Description_Box *box;
         if ((rp->part->type != EDJE_PART_TYPE_BOX)) return 0;
         CHKPARAM(3);

         GETSTR(s, params[3]);
         s = strdup(s);

         box = (Edje_Part_Description_Box *)rp->custom->description;
         box->box.alt_layout = s;

         break;
      }

      case EDJE_STATE_PARAM_BOX_ALIGN:
      {
         Edje_Part_Description_Box *box;
         if ((rp->part->type != EDJE_PART_TYPE_BOX)) return 0;
         CHKPARAM(4);

         box = (Edje_Part_Description_Box *)rp->custom->description;
         GETFLOAT_T(box->box.align.x, params[3]);
         GETFLOAT_T(box->box.align.y, params[4]);

         break;
      }

      case EDJE_STATE_PARAM_BOX_PADDING:
      {
         Edje_Part_Description_Box *box;
         if ((rp->part->type != EDJE_PART_TYPE_BOX)) return 0;
         CHKPARAM(4);

         box = (Edje_Part_Description_Box *)rp->custom->description;
         GETINT(box->box.padding.x, params[3]);
         GETINT(box->box.padding.y, params[4]);

         break;
      }

      case EDJE_STATE_PARAM_BOX_MIN:
      {
         Edje_Part_Description_Box *box;
         if ((rp->part->type != EDJE_PART_TYPE_BOX)) return 0;
         CHKPARAM(4);

         box = (Edje_Part_Description_Box *)rp->custom->description;
         GETINT(box->box.min.h, params[3]);
         GETINT(box->box.min.v, params[4]);

         break;
      }

#ifdef HAVE_EPHYSICS
      case EDJE_STATE_PARAM_PHYSICS_MASS:
        CHKPARAM(3);

        GETFLOAT_T(rp->custom->description->physics.mass, params[3]);

        break;

      case EDJE_STATE_PARAM_PHYSICS_RESTITUTION:
        CHKPARAM(3);

        GETFLOAT_T(rp->custom->description->physics.restitution, params[3]);

        break;

      case EDJE_STATE_PARAM_PHYSICS_FRICTION:
        CHKPARAM(3);

        GETFLOAT_T(rp->custom->description->physics.friction, params[3]);

        break;

      case EDJE_STATE_PARAM_PHYSICS_DAMPING:
        CHKPARAM(4);

        GETFLOAT_T(rp->custom->description->physics.damping.linear, params[3]);
        GETFLOAT_T(rp->custom->description->physics.damping.angular,
                   params[4]);

        break;

      case EDJE_STATE_PARAM_PHYSICS_SLEEP:
        CHKPARAM(4);

        GETFLOAT_T(rp->custom->description->physics.sleep.linear, params[3]);
        GETFLOAT_T(rp->custom->description->physics.sleep.angular, params[4]);

        break;

      case EDJE_STATE_PARAM_PHYSICS_MATERIAL:
        CHKPARAM(3);

        GETINT(rp->custom->description->physics.material, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_DENSITY:
        CHKPARAM(3);

        GETFLOAT_T(rp->custom->description->physics.density, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_HARDNESS:
        CHKPARAM(3);

        GETFLOAT_T(rp->custom->description->physics.hardness, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_IGNORE_PART_POS:
        CHKPARAM(3);

        GETINT(rp->custom->description->physics.ignore_part_pos, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_LIGHT_ON:
        CHKPARAM(3);

        GETINT(rp->custom->description->physics.light_on, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_MOV_FREEDOM_LIN:
        CHKPARAM(5);

        GETINT(rp->custom->description->physics.mov_freedom.lin.x, params[3]);
        GETINT(rp->custom->description->physics.mov_freedom.lin.y, params[4]);
        GETINT(rp->custom->description->physics.mov_freedom.lin.z, params[5]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_MOV_FREEDOM_ANG:
        CHKPARAM(5);

        GETINT(rp->custom->description->physics.mov_freedom.ang.x, params[3]);
        GETINT(rp->custom->description->physics.mov_freedom.ang.y, params[4]);
        GETINT(rp->custom->description->physics.mov_freedom.ang.z, params[5]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_BACK_CULL:
        CHKPARAM(3);

        GETINT(rp->custom->description->physics.backcull, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_Z:
        CHKPARAM(3);

        GETINT(rp->custom->description->physics.z, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_DEPTH:
        CHKPARAM(3);

        GETINT(rp->custom->description->physics.depth, params[3]);
        break;

#endif
      default:
        break;
     }

#ifdef EDJE_CALC_CACHE
   rp->invalidate = EINA_TRUE;
#endif
   ed->dirty = EINA_TRUE;
   return 0;
}

/**
 * @brief Retrieves a specific parameter of a part's "custom" state.
 *
 * This function is exposed to Embryo scripts as `get_state_val(part_id, State_Param:p, ...)`.
 * It allows reading individual properties (e.g., alignment, color, text) of the
 * "custom" state previously created for `part_id` by `custom_state()`.
 * The `State_Param:p` determines which property to read, and subsequent parameters
 * are pointers to where the retrieved value(s) should be stored.
 *
 * Example: `get_state_val(my_part_id, EDJE_STATE_PARAM_COLOR, &r, &g, &b, &a);`
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the total size of parameters in bytes.
 *               params[1] is the ID of the part.
 *               params[2] is the `Edje_State_Param` enum value indicating the property to get.
 *               params[3+] are Embryo cell addresses to store the retrieved value(s).
 *                          The number and type of these depend on `params[2]`.
 *                          For string types, `params[3]` is the destination buffer and `params[4]` is max length.
 * @return Always 0. Values are returned via output parameters.
 */
static Embryo_Cell
_edje_embryo_fn_get_state_val(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed = embryo_program_data_get(ep);
   Edje_Real_Part *rp;
   const char *s;

   /* we need at least 3 arguments */
   if (params[0] < (int)(sizeof(Embryo_Cell) * 3))
     return 0;

   if (params[1] < 0)
     return 0;

   if (!(rp = ed->table_parts[params[1] % ed->table_parts_size]))
     return 0;

   /* check whether this part has a "custom" state */
   if (!rp->custom)
     return 0;

   switch (params[2])
     {
      case EDJE_STATE_PARAM_ALIGNMENT:
        CHKPARAM(4);

        SETFLOAT_T(rp->custom->description->align.x, params[3]);
        SETFLOAT_T(rp->custom->description->align.y, params[4]);

        break;

      case EDJE_STATE_PARAM_MIN:
        CHKPARAM(4);

        SETINT(rp->custom->description->min.w, params[3]);
        SETINT(rp->custom->description->min.h, params[4]);

        break;

      case EDJE_STATE_PARAM_MAX:
        CHKPARAM(4);

        SETINT(rp->custom->description->max.w, params[3]);
        SETINT(rp->custom->description->max.h, params[4]);

        break;

      case EDJE_STATE_PARAM_STEP:
        CHKPARAM(4);

        SETINT(rp->custom->description->step.x, params[3]);
        SETINT(rp->custom->description->step.y, params[4]);

        break;

      case EDJE_STATE_PARAM_ASPECT:
        CHKPARAM(4);

        SETFLOAT_T(rp->custom->description->aspect.min, params[3]);
        SETFLOAT_T(rp->custom->description->aspect.max, params[4]);

        break;

      case EDJE_STATE_PARAM_ASPECT_PREF:
        CHKPARAM(3);

        SETINT(rp->custom->description->aspect.prefer, params[3]);

        break;

      case EDJE_STATE_PARAM_COLOR:
        CHKPARAM(6);

        SETINT(rp->custom->description->color.r, params[3]);
        SETINT(rp->custom->description->color.g, params[4]);
        SETINT(rp->custom->description->color.b, params[5]);
        SETINT(rp->custom->description->color.a, params[6]);

        break;

      case EDJE_STATE_PARAM_COLOR2:
        CHKPARAM(6);

        SETINT(rp->custom->description->color2.r, params[3]);
        SETINT(rp->custom->description->color2.g, params[4]);
        SETINT(rp->custom->description->color2.b, params[5]);
        SETINT(rp->custom->description->color2.a, params[6]);

        break;

      case EDJE_STATE_PARAM_COLOR3:
      {
         Edje_Part_Description_Text *text;

         if (rp->part->type == EDJE_PART_TYPE_TEXT
             || rp->part->type == EDJE_PART_TYPE_TEXTBLOCK)
           return 0;

         CHKPARAM(6);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         SETINT(text->text.color3.r, params[3]);
         SETINT(text->text.color3.g, params[4]);
         SETINT(text->text.color3.b, params[5]);
         SETINT(text->text.color3.a, params[6]);

         break;
      }

      case EDJE_STATE_PARAM_COLOR_CLASS:
        CHKPARAM(4);

        s = rp->custom->description->color_class;
        SETSTRALLOCATE(s);

        break;

      case EDJE_STATE_PARAM_REL1:
        CHKPARAM(4);

        SETFLOAT_T(rp->custom->description->rel1.relative_x, params[3]);
        SETFLOAT_T(rp->custom->description->rel1.relative_y, params[4]);

        break;

      case EDJE_STATE_PARAM_REL1_TO:
        CHKPARAM(4);

        SETINT(rp->custom->description->rel1.id_x, params[3]);
        SETINT(rp->custom->description->rel1.id_y, params[4]);

        break;

      case EDJE_STATE_PARAM_REL1_OFFSET:
        CHKPARAM(4);

        SETINT(rp->custom->description->rel1.offset_x, params[3]);
        SETINT(rp->custom->description->rel1.offset_y, params[4]);

        break;

      case EDJE_STATE_PARAM_REL2:
        CHKPARAM(4);

        SETFLOAT_T(rp->custom->description->rel2.relative_x, params[3]);
        SETFLOAT_T(rp->custom->description->rel2.relative_y, params[4]);

        break;

      case EDJE_STATE_PARAM_REL2_TO:
        CHKPARAM(4);

        SETINT(rp->custom->description->rel2.id_x, params[3]);
        SETINT(rp->custom->description->rel2.id_y, params[4]);

        break;

      case EDJE_STATE_PARAM_REL2_OFFSET:
        CHKPARAM(4);

        SETINT(rp->custom->description->rel2.offset_x, params[3]);
        SETINT(rp->custom->description->rel2.offset_y, params[4]);

        break;

      case EDJE_STATE_PARAM_IMAGE:
      {
         Edje_Part_Description_Image *img;

         if ((rp->part->type != EDJE_PART_TYPE_IMAGE)) return 0;
         CHKPARAM(3);

         img = (Edje_Part_Description_Image *)rp->custom->description;

         SETINT(img->image.id, params[3]);

         break;
      }

      case EDJE_STATE_PARAM_BORDER:
      {
         Edje_Part_Description_Image *img;

         if ((rp->part->type != EDJE_PART_TYPE_IMAGE)) return 0;
         CHKPARAM(6);

         img = (Edje_Part_Description_Image *)rp->custom->description;

         SETINT(img->image.border.l, params[3]);
         SETINT(img->image.border.r, params[4]);
         SETINT(img->image.border.t, params[5]);
         SETINT(img->image.border.b, params[6]);

         break;
      }

      case EDJE_STATE_PARAM_FILL_SMOOTH:
      {
         Edje_Part_Description_Image *img;
         Edje_Part_Description_Proxy *proxy;

         if ((rp->part->type != EDJE_PART_TYPE_IMAGE) && (rp->part->type != EDJE_PART_TYPE_PROXY)) return 0;
         CHKPARAM(3);

         if (rp->part->type == EDJE_PART_TYPE_IMAGE)
           {
              img = (Edje_Part_Description_Image *)rp->custom->description;

              SETINT(img->image.fill.smooth, params[3]);
           }
         else
           {
              proxy = (Edje_Part_Description_Proxy *)rp->custom->description;

              SETINT(proxy->proxy.fill.smooth, params[3]);
           }

         break;
      }

      case EDJE_STATE_PARAM_FILL_POS:
      {
         Edje_Part_Description_Image *img;
         Edje_Part_Description_Proxy *proxy;

         if ((rp->part->type != EDJE_PART_TYPE_IMAGE) && (rp->part->type != EDJE_PART_TYPE_PROXY)) return 0;
         CHKPARAM(6);

         if (rp->part->type == EDJE_PART_TYPE_IMAGE)
           {
              img = (Edje_Part_Description_Image *)rp->custom->description;

              SETFLOAT_T(img->image.fill.pos_rel_x, params[3]);
              SETFLOAT_T(img->image.fill.pos_rel_y, params[4]);
              SETINT(img->image.fill.pos_abs_x, params[5]);
              SETINT(img->image.fill.pos_abs_y, params[6]);
           }
         else
           {
              proxy = (Edje_Part_Description_Proxy *)rp->custom->description;

              SETFLOAT_T(proxy->proxy.fill.pos_rel_x, params[3]);
              SETFLOAT_T(proxy->proxy.fill.pos_rel_y, params[4]);
              SETINT(proxy->proxy.fill.pos_abs_x, params[5]);
              SETINT(proxy->proxy.fill.pos_abs_y, params[6]);
           }

         break;
      }

      case EDJE_STATE_PARAM_FILL_SIZE:
      {
         Edje_Part_Description_Image *img;
         Edje_Part_Description_Proxy *proxy;

         if ((rp->part->type != EDJE_PART_TYPE_IMAGE) && (rp->part->type != EDJE_PART_TYPE_PROXY)) return 0;
         CHKPARAM(6);

         if (rp->part->type == EDJE_PART_TYPE_IMAGE)
           {
              img = (Edje_Part_Description_Image *)rp->custom->description;

              SETFLOAT_T(img->image.fill.rel_x, params[3]);
              SETFLOAT_T(img->image.fill.rel_y, params[4]);
              SETINT(img->image.fill.abs_x, params[5]);
              SETINT(img->image.fill.abs_y, params[6]);
           }
         else
           {
              proxy = (Edje_Part_Description_Proxy *)rp->custom->description;

              SETFLOAT_T(proxy->proxy.fill.rel_x, params[3]);
              SETFLOAT_T(proxy->proxy.fill.rel_y, params[4]);
              SETINT(proxy->proxy.fill.abs_x, params[5]);
              SETINT(proxy->proxy.fill.abs_y, params[6]);
           }

         break;
      }

      case EDJE_STATE_PARAM_TEXT:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
             (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
           return 0;

         CHKPARAM(4);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         s = (char *)text->text.text.str;
         SETSTRALLOCATE(s);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_CLASS:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
             (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
           return 0;

         CHKPARAM(4);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         s = text->text.text_class;
         SETSTRALLOCATE(s);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_FONT:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT)) return 0;

         CHKPARAM(4);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         s = (char *)text->text.font.str;
         SETSTRALLOCATE(s);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_STYLE:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXTBLOCK)) return 0;

         CHKPARAM(4);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         s = (char *)text->text.style.str;
         SETSTRALLOCATE(s);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_SIZE:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT)) return 0;

         CHKPARAM(3);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         SETINT(text->text.size, params[3]);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_FIT:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT)) return 0;
         CHKPARAM(4);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         SETINT(text->text.fit_x, params[3]);
         SETINT(text->text.fit_y, params[4]);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_MIN:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
             (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
           return 0;

         CHKPARAM(4);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         SETINT(text->text.min_x, params[3]);
         SETINT(text->text.min_y, params[4]);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_MAX:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
             (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
           return 0;

         CHKPARAM(4);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         SETINT(text->text.max_x, params[3]);
         SETINT(text->text.max_y, params[4]);

         break;
      }

      case EDJE_STATE_PARAM_TEXT_ALIGN:
      {
         Edje_Part_Description_Text *text;

         if ((rp->part->type != EDJE_PART_TYPE_TEXT)) return 0;

         CHKPARAM(4);

         text = (Edje_Part_Description_Text *)rp->custom->description;

         SETFLOAT_T(text->text.align.x, params[3]);
         SETFLOAT_T(text->text.align.y, params[4]);

         break;
      }

      case EDJE_STATE_PARAM_VISIBLE:
        CHKPARAM(3);

        SETINT(rp->custom->description->visible, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_ON:
        CHKPARAM(3);

        SETINT(rp->custom->description->map.on, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_PERSP:
        CHKPARAM(3);

        SETINT(rp->custom->description->map.id_persp, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_LIGHT:
        CHKPARAM(3);

        SETINT(rp->custom->description->map.id_light, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_ROT_CENTER:
        CHKPARAM(3);

        SETINT(rp->custom->description->map.rot.id_center, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_ROT_X:
        CHKPARAM(3);

        SETFLOAT_T(rp->custom->description->map.rot.x, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_ROT_Y:
        CHKPARAM(3);

        SETFLOAT_T(rp->custom->description->map.rot.y, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_ROT_Z:
        CHKPARAM(3);

        SETFLOAT_T(rp->custom->description->map.rot.z, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_BACK_CULL:
        CHKPARAM(3);

        SETINT(rp->custom->description->map.backcull, params[3]);

        break;

      case EDJE_STATE_PARAM_MAP_PERSP_ON:
        CHKPARAM(3);

        SETINT(rp->custom->description->map.persp_on, params[3]);

        break;

      case EDJE_STATE_PARAM_PERSP_ZPLANE:
        CHKPARAM(3);

        SETINT(rp->custom->description->persp.zplane, params[3]);

        break;

      case EDJE_STATE_PARAM_PERSP_FOCAL:
        CHKPARAM(3);

        SETINT(rp->custom->description->persp.focal, params[3]);

        break;

      case EDJE_STATE_PARAM_PROXY_SRC_CLIP:
      {
         Edje_Part_Description_Proxy *proxy;

         if ((rp->part->type != EDJE_PART_TYPE_PROXY)) return 0;
         CHKPARAM(3);

         proxy = (Edje_Part_Description_Proxy *)rp->custom->description;
         SETINT(proxy->proxy.source_clip, params[3]);

         break;
      }

      case EDJE_STATE_PARAM_PROXY_SRC_VISIBLE:
      {
         Edje_Part_Description_Proxy *proxy;

         if ((rp->part->type != EDJE_PART_TYPE_PROXY)) return 0;
         CHKPARAM(3);

         proxy = (Edje_Part_Description_Proxy *)rp->custom->description;
         SETINT(proxy->proxy.source_visible, params[3]);

         break;
      }

#ifdef HAVE_EPHYSICS
      case EDJE_STATE_PARAM_PHYSICS_MASS:
        CHKPARAM(3);

        SETFLOAT_T(rp->custom->description->physics.mass, params[3]);

        break;

      case EDJE_STATE_PARAM_PHYSICS_RESTITUTION:
        CHKPARAM(3);

        SETFLOAT_T(rp->custom->description->physics.restitution, params[3]);

        break;

      case EDJE_STATE_PARAM_PHYSICS_FRICTION:
        CHKPARAM(3);

        SETFLOAT_T(rp->custom->description->physics.friction, params[3]);

        break;

      case EDJE_STATE_PARAM_PHYSICS_DAMPING:
        CHKPARAM(4);

        SETFLOAT_T(rp->custom->description->physics.damping.linear, params[3]);
        SETFLOAT_T(rp->custom->description->physics.damping.angular,
                   params[4]);

        break;

      case EDJE_STATE_PARAM_PHYSICS_SLEEP:
        CHKPARAM(4);

        SETFLOAT_T(rp->custom->description->physics.sleep.linear, params[3]);
        SETFLOAT_T(rp->custom->description->physics.sleep.angular, params[4]);

        break;

      case EDJE_STATE_PARAM_PHYSICS_MATERIAL:
        CHKPARAM(3);

        SETINT(rp->custom->description->physics.material, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_DENSITY:
        CHKPARAM(3);

        SETFLOAT_T(rp->custom->description->physics.density, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_HARDNESS:
        CHKPARAM(3);

        SETFLOAT_T(rp->custom->description->physics.hardness, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_IGNORE_PART_POS:
        CHKPARAM(3);

        SETINT(rp->custom->description->physics.ignore_part_pos, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_LIGHT_ON:
        CHKPARAM(3);

        SETINT(rp->custom->description->physics.light_on, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_MOV_FREEDOM_LIN:
        CHKPARAM(5);

        SETINT(rp->custom->description->physics.mov_freedom.lin.x, params[3]);
        SETINT(rp->custom->description->physics.mov_freedom.lin.y, params[4]);
        SETINT(rp->custom->description->physics.mov_freedom.lin.z, params[5]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_MOV_FREEDOM_ANG:
        CHKPARAM(5);

        SETINT(rp->custom->description->physics.mov_freedom.ang.x, params[3]);
        SETINT(rp->custom->description->physics.mov_freedom.ang.y, params[4]);
        SETINT(rp->custom->description->physics.mov_freedom.ang.z, params[5]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_BACK_CULL:
        CHKPARAM(3);

        SETINT(rp->custom->description->physics.backcull, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_Z:
        CHKPARAM(3);

        SETINT(rp->custom->description->physics.z, params[3]);
        break;

      case EDJE_STATE_PARAM_PHYSICS_DEPTH:
        CHKPARAM(3);

        SETINT(rp->custom->description->physics.depth, params[3]);
        break;

#endif
      default:
        break;
     }

   return 0;
}

/**
 * @brief Sets the mouse event flags for a specific part.
 *
 * This function is exposed to Embryo scripts as `set_mouse_events(part_id, ev)`.
 * It enables or disables mouse event processing for the part identified by `part_id`.
 * `ev` is a bitmask of flags; 0 typically means no mouse events, >0 means enabled.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 *               params[2] is the mouse event flags (integer).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_mouse_events(Embryo_Program *ep, Embryo_Cell *params)
{
   int part_id = 0;
   Edje *ed;
   Edje_Real_Part *rp;

   CHKPARAM(2);

   part_id = params[1];
   if (part_id < 0) return 0;

   ed = embryo_program_data_get(ep);
   rp = ed->table_parts[part_id % ed->table_parts_size];

   if (rp)
     _edje_real_part_mouse_events_set(ed, rp, params[2]);

   return 0;
}

/**
 * @brief Gets the mouse event flags for a specific part.
 *
 * This function is exposed to Embryo scripts as `get_mouse_events(part_id)`.
 * It retrieves the current mouse event flags for the part identified by `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 * @return The current mouse event flags (integer) for the part, or 0 if part not found.
 */
static Embryo_Cell
_edje_embryo_fn_get_mouse_events(Embryo_Program *ep, Embryo_Cell *params)
{
   int part_id = 0;
   Edje *ed;
   Edje_Real_Part *rp;

   CHKPARAM(1);

   part_id = params[1];
   if (part_id < 0) return 0;

   ed = embryo_program_data_get(ep);
   rp = ed->table_parts[part_id % ed->table_parts_size];

   if (rp)
     return (Embryo_Cell)_edje_var_int_get(ed, (int)_edje_real_part_mouse_events_get(ed, rp));

   return 0;

}

/**
 * @brief Sets the pointer mode for a specific part.
 *
 * This function is exposed to Embryo scripts as `set_pointer_mode(part_id, mode)`.
 * It configures how a part interacts with pointer events, specifically regarding
 * auto-grabbing or not grabbing the pointer.
 *
 * Pointer_Mode {
 *   POINTER_MODE_AUTOGRAB = 0, // Default: part grabs pointer on mouse down
 *   POINTER_MODE_NOGRAB = 1,   // Part does not grab pointer
 *   POINTER_MODE_NOGREP = 2    // Part does not grab pointer and events are not repeated
 * }
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 *               params[2] is the pointer mode (integer, corresponds to Pointer_Mode enum).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_pointer_mode(Embryo_Program *ep, Embryo_Cell *params)
{
   int part_id = 0;
   Edje *ed;
   Edje_Real_Part *rp;

   CHKPARAM(2);

   part_id = params[1];
   if (part_id < 0) return 0;

   ed = embryo_program_data_get(ep);
   rp = ed->table_parts[part_id % ed->table_parts_size];

   if (rp)
     _edje_real_part_pointer_mode_set(ed, rp, params[2]);

   return 0;
}

/**
 * @brief Sets whether a part should repeat events.
 *
 * This function is exposed to Embryo scripts as `set_repeat_events(part_id, rep)`.
 * If `rep` is non-zero, events like mouse button holds might be repeated.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 *               params[2] is the repeat events flag (integer, 0 or 1).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_repeat_events(Embryo_Program *ep, Embryo_Cell *params)
{
   int part_id = 0;
   Edje *ed;
   Edje_Real_Part *rp;

   CHKPARAM(2);

   part_id = params[1];
   if (part_id < 0) return 0;

   ed = embryo_program_data_get(ep);
   rp = ed->table_parts[part_id % ed->table_parts_size];

   if (rp)
     _edje_real_part_repeat_events_set(ed, rp, params[2]);

   return 0;
}

/**
 * @brief Gets whether a part repeats events.
 *
 * This function is exposed to Embryo scripts as `get_repeat_events(part_id)`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 * @return The repeat events flag (integer, 0 or 1) for the part, or 0 if part not found.
 */
static Embryo_Cell
_edje_embryo_fn_get_repeat_events(Embryo_Program *ep, Embryo_Cell *params)
{
   int part_id = 0;
   Edje *ed;
   Edje_Real_Part *rp;

   CHKPARAM(1);

   part_id = params[1];
   if (part_id < 0) return 0;

   ed = embryo_program_data_get(ep);
   rp = ed->table_parts[part_id % ed->table_parts_size];

   if (rp)
     return (Embryo_Cell)_edje_var_int_get(ed, (int)_edje_real_part_repeat_events_get(ed, rp));

   return 0;

}

/**
 * @brief Sets the event ignore flags for a specific part.
 *
 * This function is exposed to Embryo scripts as `set_ignore_flags(part_id, flags)`.
 * `flags` is a bitmask of `Evas_Object_Event_Flags` that specifies which events
 * the part should ignore (pass through to objects below it).
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 *               params[2] is the ignore flags bitmask (integer).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_ignore_flags(Embryo_Program *ep, Embryo_Cell *params)
{
   int part_id = 0;
   Edje *ed;
   Edje_Real_Part *rp;

   CHKPARAM(2);

   part_id = params[1];
   if (part_id < 0) return 0;

   ed = embryo_program_data_get(ep);
   rp = ed->table_parts[part_id % ed->table_parts_size];

   if (rp)
     _edje_real_part_ignore_flags_set(ed, rp, params[2]);

   return 0;
}

/**
 * @brief Gets the event ignore flags for a specific part.
 *
 * This function is exposed to Embryo scripts as `get_ignore_flags(part_id)`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 * @return The current ignore flags bitmask (integer) for the part, or 0 if part not found.
 */
static Embryo_Cell
_edje_embryo_fn_get_ignore_flags(Embryo_Program *ep, Embryo_Cell *params)
{
   int part_id = 0;
   Edje *ed;
   Edje_Real_Part *rp;

   CHKPARAM(1);

   part_id = params[1];
   if (part_id < 0) return 0;

   ed = embryo_program_data_get(ep);
   rp = ed->table_parts[part_id % ed->table_parts_size];

   if (rp)
     return (Embryo_Cell)_edje_var_int_get(ed, (int)_edje_real_part_ignore_flags_get(ed, rp));

   return 0;

}

/**
 * @brief Sets the event mask flags for a specific part.
 * (Note: This function seems to be intended for masking events, but its
 * implementation might be similar to ignore_flags or have a specific nuance
 * within Edje's event handling. The name suggests it might prevent events
 * from propagating further up or down in a specific way.)
 *
 * This function is exposed to Embryo scripts as `set_mask_flags(part_id, flags)`.
 * `flags` is a bitmask that likely influences event propagation.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 *               params[2] is the mask flags bitmask (integer).
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_set_mask_flags(Embryo_Program *ep, Embryo_Cell *params)
{
   int part_id = 0;
   Edje *ed;
   Edje_Real_Part *rp;

   CHKPARAM(2);

   part_id = params[1];
   if (part_id < 0) return 0;

   ed = embryo_program_data_get(ep);
   rp = ed->table_parts[part_id % ed->table_parts_size];

   if (rp)
     _edje_real_part_mask_flags_set(ed, rp, params[2]);

   return 0;
}

/**
 * @brief Gets the event mask flags for a specific part.
 *
 * This function is exposed to Embryo scripts as `get_mask_flags(part_id)`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the part.
 * @return The current mask flags bitmask (integer) for the part, or 0 if part not found.
 */
static Embryo_Cell
_edje_embryo_fn_get_mask_flags(Embryo_Program *ep, Embryo_Cell *params)
{
   int part_id = 0;
   Edje *ed;
   Edje_Real_Part *rp;

   CHKPARAM(1);

   part_id = params[1];
   if (part_id < 0) return 0;

   ed = embryo_program_data_get(ep);
   rp = ed->table_parts[part_id % ed->table_parts_size];

   if (rp)
     return (Embryo_Cell)_edje_var_int_get(ed, (int)_edje_real_part_mask_flags_get(ed, rp));

   return 0;

}

/**
 * @brief Makes a SWALLOW part swallow a new Edje object from the same Edje file.
 *
 * This function is exposed to Embryo scripts as `part_swallow(part_id, group_name)`.
 * It creates a new Edje object using the group `group_name` from the current
 * Edje file and swallows it into the SWALLOW part identified by `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the SWALLOW part.
 *               params[2] is the Embryo cell address of the group name string to load and swallow.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_part_swallow(Embryo_Program *ep, Embryo_Cell *params)
{
   int part_id = 0;
   char *group_name = 0;
   Edje *ed;
   Edje_Real_Part *rp;
   Evas_Object *new_obj;

   CHKPARAM(2);

   part_id = params[1];
   if (part_id < 0) return 0;

   GETSTR(group_name, params[2]);
   if (!group_name) return 0;

   ed = embryo_program_data_get(ep);

   rp = ed->table_parts[part_id % ed->table_parts_size];
   if (!rp) return 0;

   new_obj = edje_object_add(ed->base.evas);
   if (!new_obj) return 0;

   if (!edje_object_file_set(new_obj, ed->file->path, group_name))
     {
        evas_object_del(new_obj);
        return 0;
     }
   edje_object_part_swallow(ed->obj, rp->part->name, new_obj);
   _edje_subobj_register(ed, new_obj);

   return 0;
}

/**
 * @brief Sets the focus to a specific part for a given seat.
 *
 * This function is exposed to Embryo scripts as `set_focus(part_id, seat_name[])`.
 * It directs keyboard focus to the Edje part identified by `part_id`.
 * If `seat_name` is provided, focus is set for that specific seat; otherwise,
 * the default seat is used.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments (1 or 2).
 *               params[1] is the ID of the part to focus.
 *               params[2] (optional) is the Embryo cell address of the seat name string.
 * @return 0 on success, -1 if the wrong number of parameters is provided.
 */
static Embryo_Cell
_edje_embryo_fn_set_focus(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   char *seat_name = NULL;

   if (!(HASNPARAMS(1) || HASNPARAMS(2))) return -1;
   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];
   if (!rp) return 0;

   /* if no seat name is passed, that's fine. it means
      it should be applied to default seat */
   if (HASNPARAMS(2))
     {
        GETSTR(seat_name, params[2]);
        if (!seat_name) return 0;
     }

   _edje_part_focus_set(ed, seat_name, rp);

   return 0;
}

/**
 * @brief Removes focus from any part for a given seat (or default seat).
 *
 * This function is exposed to Embryo scripts as `unset_focus(seat_name[])`.
 * It clears the focus for the specified `seat_name`. If `seat_name` is not
 * provided, focus is cleared for the default seat.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments (0 or 1).
 *               params[1] (optional) is the Embryo cell address of the seat name string.
 * @return 0 on success, -1 if the wrong number of parameters is provided.
 */
static Embryo_Cell
_edje_embryo_fn_unset_focus(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *seat_name = NULL;

   if (!(HASNPARAMS(0) || HASNPARAMS(1))) return -1;
   ed = embryo_program_data_get(ep);

   /* seat name is optional. no seat means
      it should be applied to default seat */
   if (HASNPARAMS(1))
     {
        GETSTR(seat_name, params[1]);
        if (!seat_name) return 0;
     }

   _edje_part_focus_set(ed, seat_name, NULL);

   return 0;
}

/**
 * @brief Gets an integer value from an external parameter of an EXTERNAL part.
 *
 * This function is exposed to Embryo scripts as `external_param_get_int(id, param_name[])`.
 * It retrieves an integer value for the parameter `param_name` from the
 * EXTERNAL part identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the EXTERNAL part.
 *               params[2] is the Embryo cell address of the parameter name string.
 * @return The integer value of the parameter, or 0 if not found or on error.
 */
static Embryo_Cell
_edje_embryo_fn_external_param_get_int(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   Edje_External_Param eep;
   char *param_name;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   GETSTR(param_name, params[2]);
   if (!param_name) return 0;
   eep.name = param_name;
   eep.type = EDJE_EXTERNAL_PARAM_TYPE_INT;
   eep.i = 0;
   _edje_external_param_get(NULL, rp, &eep);
   return eep.i;
}

/**
 * @brief Sets an integer value for an external parameter of an EXTERNAL part.
 *
 * This function is exposed to Embryo scripts as `external_param_set_int(id, param_name[], value)`.
 * It sets an integer `value` for the parameter `param_name` of the
 * EXTERNAL part identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the EXTERNAL part.
 *               params[2] is the Embryo cell address of the parameter name string.
 *               params[3] is the integer value to set.
 * @return Non-zero on success, 0 on failure (e.g., part not found, param not found, type mismatch).
 */
static Embryo_Cell
_edje_embryo_fn_external_param_set_int(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   Edje_External_Param eep;
   char *param_name;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   GETSTR(param_name, params[2]);
   if (!param_name) return 0;
   eep.name = param_name;
   eep.type = EDJE_EXTERNAL_PARAM_TYPE_INT;
   eep.i = params[3];
   return _edje_external_param_set(NULL, rp, &eep);
}

/**
 * @brief Gets a float value from an external parameter of an EXTERNAL part.
 *
 * This function is exposed to Embryo scripts as `Float:external_param_get_float(id, param_name[])`.
 * It retrieves a float (double precision internally) value for the parameter `param_name`
 * from the EXTERNAL part identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the EXTERNAL part.
 *               params[2] is the Embryo cell address of the parameter name string.
 * @return The float value of the parameter (as Embryo_Cell), or 0.0 if not found or on error.
 */
static Embryo_Cell
_edje_embryo_fn_external_param_get_float(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   Edje_External_Param eep;
   char *param_name;
   float v;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   GETSTR(param_name, params[2]);
   if (!param_name) return 0;
   eep.name = param_name;
   eep.type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
   eep.d = 0.0;
   _edje_external_param_get(NULL, rp, &eep);
   v = eep.d;
   return EMBRYO_FLOAT_TO_CELL(v);
}

/**
 * @brief Sets a float value for an external parameter of an EXTERNAL part.
 *
 * This function is exposed to Embryo scripts as `external_param_set_float(id, param_name[], Float:value)`.
 * It sets a float `value` for the parameter `param_name` of the
 * EXTERNAL part identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the EXTERNAL part.
 *               params[2] is the Embryo cell address of the parameter name string.
 *               params[3] is the float value (as Embryo_Cell) to set.
 * @return Non-zero on success, 0 on failure.
 */
static Embryo_Cell
_edje_embryo_fn_external_param_set_float(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   Edje_External_Param eep;
   char *param_name;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   GETSTR(param_name, params[2]);
   if (!param_name) return 0;
   eep.name = param_name;
   eep.type = EDJE_EXTERNAL_PARAM_TYPE_DOUBLE;
   eep.d = EMBRYO_CELL_TO_FLOAT(params[3]);
   return _edje_external_param_set(NULL, rp, &eep);
}

/**
 * @brief Gets the length of a string external parameter of an EXTERNAL part.
 *
 * This function is exposed to Embryo scripts as `external_param_get_strlen(id, param_name[])`.
 * It retrieves the length of the string value for the parameter `param_name`
 * from the EXTERNAL part identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the EXTERNAL part.
 *               params[2] is the Embryo cell address of the parameter name string.
 * @return The length of the string parameter, or 0 if not found, not a string, or on error.
 */
static Embryo_Cell
_edje_embryo_fn_external_param_get_strlen(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   Edje_External_Param eep;
   char *param_name;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   GETSTR(param_name, params[2]);
   if (!param_name) return 0;
   eep.name = param_name;
   eep.type = EDJE_EXTERNAL_PARAM_TYPE_STRING;
   eep.s = NULL;
   _edje_external_param_get(NULL, rp, &eep);
   if (!eep.s) return 0;
   return strlen(eep.s);
}

/**
 * @brief Gets a string value from an external parameter of an EXTERNAL part.
 *
 * This function is exposed to Embryo scripts as `external_param_get_str(id, param_name[], value[], value_maxlen)`.
 * It retrieves the string value for the parameter `param_name` from the
 * EXTERNAL part identified by `id`, copying it into the `value[]` buffer
 * up to `value_maxlen` characters.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the EXTERNAL part.
 *               params[2] is the Embryo cell address of the parameter name string.
 *               params[3] is the Embryo cell address of the destination string buffer.
 *               params[4] is the maximum length of the destination buffer.
 * @return 1 on success, 0 on failure (e.g., param not found, buffer too small, type mismatch).
 *         On failure or if param not found, `value[]` is set to an empty string.
 */
static Embryo_Cell
_edje_embryo_fn_external_param_get_str(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   Edje_External_Param eep;
   char *param_name;
   size_t src_len, dst_len;

   CHKPARAM(4);
   dst_len = params[4];
   if (dst_len < 1) goto error;

   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) goto error;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   GETSTR(param_name, params[2]);
   if (!param_name) return 0;
   eep.name = param_name;
   eep.type = EDJE_EXTERNAL_PARAM_TYPE_STRING;
   eep.s = NULL;
   _edje_external_param_get(NULL, rp, &eep);
   if (!eep.s) goto error;
   src_len = strlen(eep.s);
   if (src_len < dst_len)
     {
        SETSTR(eep.s, params[3]);
     }
   else
     {
        char *tmp = alloca(dst_len);
        memcpy(tmp, eep.s, dst_len - 1);
        tmp[dst_len - 1] = '\0';
        SETSTR(tmp, params[3]);
     }
   return 1;

error:
   SETSTR("", params[3]);
   return 0;
}

/**
 * @brief Sets a string value for an external parameter of an EXTERNAL part.
 *
 * This function is exposed to Embryo scripts as `external_param_set_str(id, param_name[], value[])`.
 * It sets a string `value` for the parameter `param_name` of the
 * EXTERNAL part identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the EXTERNAL part.
 *               params[2] is the Embryo cell address of the parameter name string.
 *               params[3] is the Embryo cell address of the string value to set.
 * @return Non-zero on success, 0 on failure.
 */
static Embryo_Cell
_edje_embryo_fn_external_param_set_str(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   Edje_External_Param eep;
   char *param_name, *val;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   GETSTR(param_name, params[2]);
   if (!param_name) return 0;
   eep.name = param_name;
   eep.type = EDJE_EXTERNAL_PARAM_TYPE_STRING;
   GETSTR(val, params[3]);
   if (!val) return 0;
   eep.s = val;
   return _edje_external_param_set(NULL, rp, &eep);
}

/**
 * @brief Gets the length of the current choice string of an EXTERNAL part's choice parameter.
 *
 * This function is exposed to Embryo scripts as `external_param_get_choice_len(id, param_name[])`.
 * It retrieves the length of the currently selected choice string for the parameter `param_name`
 * (which must be of type EDJE_EXTERNAL_PARAM_TYPE_CHOICE) from the EXTERNAL part
 * identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the EXTERNAL part.
 *               params[2] is the Embryo cell address of the parameter name string.
 * @return The length of the choice string, or 0 if not found, not a choice type, or on error.
 */
static Embryo_Cell
_edje_embryo_fn_external_param_get_choice_len(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   Edje_External_Param eep;
   char *param_name;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   GETSTR(param_name, params[2]);
   if (!param_name) return 0;
   eep.name = param_name;
   eep.type = EDJE_EXTERNAL_PARAM_TYPE_CHOICE;
   eep.s = NULL;
   _edje_external_param_get(NULL, rp, &eep);
   if (!eep.s) return 0;
   return strlen(eep.s);
}

/**
 * @brief Gets the current choice string of an EXTERNAL part's choice parameter.
 *
 * This function is exposed to Embryo scripts as `external_param_get_choice(id, param_name[], value[], value_maxlen)`.
 * It retrieves the currently selected choice string for the parameter `param_name`
 * (which must be of type EDJE_EXTERNAL_PARAM_TYPE_CHOICE) from the EXTERNAL part
 * identified by `id`, copying it into the `value[]` buffer up to `value_maxlen` characters.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the EXTERNAL part.
 *               params[2] is the Embryo cell address of the parameter name string.
 *               params[3] is the Embryo cell address of the destination string buffer.
 *               params[4] is the maximum length of the destination buffer.
 * @return 1 on success, 0 on failure. On failure, `value[]` is set to an empty string.
 */
static Embryo_Cell
_edje_embryo_fn_external_param_get_choice(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   Edje_External_Param eep;
   char *param_name;
   size_t src_len, dst_len;

   CHKPARAM(4);
   dst_len = params[4];
   if (dst_len < 1) goto error;

   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) goto error;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   GETSTR(param_name, params[2]);
   if (!param_name) return 0;
   eep.name = param_name;
   eep.type = EDJE_EXTERNAL_PARAM_TYPE_CHOICE;
   eep.s = NULL;
   _edje_external_param_get(NULL, rp, &eep);
   if (!eep.s) goto error;
   src_len = strlen(eep.s);
   if (src_len < dst_len)
     {
        SETSTR(eep.s, params[3]);
     }
   else
     {
        char *tmp = alloca(dst_len);
        memcpy(tmp, eep.s, dst_len - 1);
        tmp[dst_len - 1] = '\0';
        SETSTR(tmp, params[3]);
     }
   return 1;

error:
   SETSTR("", params[3]);
   return 0;
}

/**
 * @brief Sets the current choice for an EXTERNAL part's choice parameter.
 *
 * This function is exposed to Embryo scripts as `external_param_set_choice(id, param_name[], value[])`.
 * It sets the choice for the parameter `param_name` (which must be of type
 * EDJE_EXTERNAL_PARAM_TYPE_CHOICE) of the EXTERNAL part identified by `id`
 * to the string `value[]`. The `value[]` must be one of the valid choices for that parameter.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the EXTERNAL part.
 *               params[2] is the Embryo cell address of the parameter name string.
 *               params[3] is the Embryo cell address of the choice string to set.
 * @return Non-zero on success, 0 on failure.
 */
static Embryo_Cell
_edje_embryo_fn_external_param_set_choice(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   Edje_External_Param eep;
   char *param_name, *val;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   GETSTR(param_name, params[2]);
   if (!param_name) return 0;
   eep.name = param_name;
   eep.type = EDJE_EXTERNAL_PARAM_TYPE_CHOICE;
   GETSTR(val, params[3]);
   if (!val) return 0;
   eep.s = val;
   return _edje_external_param_set(NULL, rp, &eep);
}

/**
 * @brief Gets a boolean value from an external parameter of an EXTERNAL part.
 *
 * This function is exposed to Embryo scripts as `external_param_get_bool(id, param_name[])`.
 * It retrieves a boolean (integer 0 or 1) value for the parameter `param_name`
 * from the EXTERNAL part identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the EXTERNAL part.
 *               params[2] is the Embryo cell address of the parameter name string.
 * @return The boolean value (0 or 1) of the parameter, or 0 if not found or on error.
 */
static Embryo_Cell
_edje_embryo_fn_external_param_get_bool(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   Edje_External_Param eep;
   char *param_name;

   CHKPARAM(2);
   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   GETSTR(param_name, params[2]);
   if (!param_name) return 0;
   eep.name = param_name;
   eep.type = EDJE_EXTERNAL_PARAM_TYPE_BOOL;
   eep.i = 0;
   _edje_external_param_get(NULL, rp, &eep);
   return eep.i;
}

/**
 * @brief Sets a boolean value for an external parameter of an EXTERNAL part.
 *
 * This function is exposed to Embryo scripts as `external_param_set_bool(id, param_name[], value)`.
 * It sets a boolean `value` (0 or 1) for the parameter `param_name` of the
 * EXTERNAL part identified by `id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the EXTERNAL part.
 *               params[2] is the Embryo cell address of the parameter name string.
 *               params[3] is the boolean value (0 or 1) to set.
 * @return Non-zero on success, 0 on failure.
 */
static Embryo_Cell
_edje_embryo_fn_external_param_set_bool(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id;
   Edje_Real_Part *rp;
   Edje_External_Param eep;
   char *param_name;

   CHKPARAM(3);
   ed = embryo_program_data_get(ep);

   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   GETSTR(param_name, params[2]);
   if (!param_name) return 0;
   eep.name = param_name;
   eep.type = EDJE_EXTERNAL_PARAM_TYPE_BOOL;
   eep.i = params[3];
   return _edje_external_param_set(NULL, rp, &eep);
}

#ifdef HAVE_EPHYSICS
/**
 * @brief Generic helper function to call EPhysics body functions that take three double components.
 *
 * This internal helper is used by various `physics_*_set` and `physics_*_apply`
 * Embryo-exposed functions to reduce code duplication. It retrieves an Edje part,
 * checks if it has an associated EPhysics_Body, converts three float parameters
 * from Embryo cells to doubles, and calls the provided `func` with the body and
 * these three doubles.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments (expected to be 4).
 *               params[1] is the ID of the part with an EPhysics_Body.
 *               params[2] is the X component (float, as Embryo_Cell).
 *               params[3] is the Y component (float, as Embryo_Cell).
 *               params[4] is the Z component (float, as Embryo_Cell).
 * @param func A function pointer to an EPhysics call like `ephysics_body_central_impulse_apply`.
 *             The signature must be `void (*func)(EPhysics_Body *body, double x, double y, double z)`.
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_components_set(Embryo_Program *ep, Embryo_Cell *params, void (*func)(EPhysics_Body *body, double x, double y, double z))
{
   Edje_Real_Part *rp;
   int part_id = 0;
   Edje *ed;

   CHKPARAM(4);

   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;

   rp = ed->table_parts[part_id % ed->table_parts_size];
   if ((rp) && (rp->body))
     {
        double x, y, z;

        x = (double)EMBRYO_CELL_TO_FLOAT(params[2]);
        y = (double)EMBRYO_CELL_TO_FLOAT(params[3]);
        z = (double)EMBRYO_CELL_TO_FLOAT(params[4]);

        func(rp->body, x, y, z);
     }

   return 0;
}

/**
 * @brief Generic helper function to call EPhysics body functions that get three double components.
 *
 * This internal helper is used by various `physics_*_get` Embryo-exposed functions
 * to reduce code duplication. It retrieves an Edje part, checks if it has an
 * associated EPhysics_Body, calls the provided `func` to get three double values,
 * and then sets these values back into the provided Embryo cell addresses as floats.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments (expected to be 4).
 *               params[1] is the ID of the part with an EPhysics_Body.
 *               params[2] is the Embryo cell address to store the X component (float).
 *               params[3] is the Embryo cell address to store the Y component (float).
 *               params[4] is the Embryo cell address to store the Z component (float).
 * @param func A function pointer to an EPhysics call like `ephysics_body_forces_get`.
 *             The signature must be `void (*func)(const EPhysics_Body *body, double *x, double *y, double *z)`.
 * @return Always 0. Values are returned via output parameters.
 */
static Embryo_Cell
_edje_embryo_fn_physics_components_get(Embryo_Program *ep, Embryo_Cell *params, void (*func)(const EPhysics_Body *body, double *x, double *y, double *z))
{
   Edje_Real_Part *rp;
   int part_id = 0;
   Edje *ed;

   CHKPARAM(4);

   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;

   rp = ed->table_parts[part_id % ed->table_parts_size];
   if ((rp) && (rp->body))
     {
        double x, y, z;
        func(rp->body, &x, &y, &z);
        SETFLOAT(x, params[2]);
        SETFLOAT(y, params[3]);
        SETFLOAT(z, params[4]);
     }

   return 0;
}

/**
 * @brief Applies a central impulse to a physics-enabled part.
 *
 * This function is exposed to Embryo scripts as `physics_impulse(part_id, Float:x, Float:y, Float:z)`.
 * It applies an impulse (force over a short time) to the center of mass of the
 * EPhysics_Body associated with the part `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 *               params[2]: X component of impulse
 *               params[3]: Y component of impulse
 *               params[4]: Z component of impulse
 * @return 0 if EPhysics is loaded and function is called, otherwise 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_impulse(Embryo_Program *ep, Embryo_Cell *params)
{
   if (!EPH_LOAD()) return 0;
   return _edje_embryo_fn_physics_components_set(
            ep, params, EPH_CALL(ephysics_body_central_impulse_apply));
}

/**
 * @brief Applies a torque impulse to a physics-enabled part.
 *
 * This function is exposed to Embryo scripts as `physics_torque_impulse(part_id, Float:x, Float:y, Float:z)`.
 * It applies an angular impulse (torque over a short time) to the EPhysics_Body
 * associated with the part `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 *               params[2]: X component of torque impulse
 *               params[3]: Y component of torque impulse
 *               params[4]: Z component of torque impulse
 * @return 0 if EPhysics is loaded and function is called, otherwise 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_torque_impulse(Embryo_Program *ep, Embryo_Cell *params)
{
   if (!EPH_LOAD()) return 0;
   return _edje_embryo_fn_physics_components_set(
            ep, params, EPH_CALL(ephysics_body_torque_impulse_apply));
}

/**
 * @brief Applies a continuous central force to a physics-enabled part.
 *
 * This function is exposed to Embryo scripts as `physics_force(part_id, Float:x, Float:y, Float:z)`.
 * It applies a continuous force to the center of mass of the EPhysics_Body
 * associated with the part `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 *               params[2]: X component of force
 *               params[3]: Y component of force
 *               params[4]: Z component of force
 * @return 0 if EPhysics is loaded and function is called, otherwise 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_force(Embryo_Program *ep, Embryo_Cell *params)
{
   if (!EPH_LOAD()) return 0;
   return _edje_embryo_fn_physics_components_set(
            ep, params, EPH_CALL(ephysics_body_central_force_apply));
}

/**
 * @brief Applies a continuous torque to a physics-enabled part.
 *
 * This function is exposed to Embryo scripts as `physics_torque(part_id, Float:x, Float:y, Float:z)`.
 * It applies a continuous angular force (torque) to the EPhysics_Body
 * associated with the part `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 *               params[2]: X component of torque
 *               params[3]: Y component of torque
 *               params[4]: Z component of torque
 * @return 0 if EPhysics is loaded and function is called, otherwise 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_torque(Embryo_Program *ep, Embryo_Cell *params)
{
   if (!EPH_LOAD()) return 0;
   return _edje_embryo_fn_physics_components_set(
            ep, params, EPH_CALL(ephysics_body_torque_apply));
}

/**
 * @brief Clears all forces and torques acting on a physics-enabled part.
 *
 * This function is exposed to Embryo scripts as `physics_clear_forces(part_id)`.
 * It removes any continuous forces and torques currently applied to the
 * EPhysics_Body associated with the part `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_clear_forces(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje_Real_Part *rp;
   int part_id = 0;
   Edje *ed;

   CHKPARAM(1);

   if (!EPH_LOAD()) return 0;
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;

   rp = ed->table_parts[part_id % ed->table_parts_size];
   if ((rp) && (rp->body))
     EPH_CALL(ephysics_body_forces_clear)(rp->body);

   return 0;
}

/**
 * @brief Gets the total applied forces on a physics-enabled part.
 *
 * This function is exposed to Embryo scripts as `physics_get_forces(part_id, &Float:x, &Float:y, &Float:z)`.
 * It retrieves the sum of all continuous forces currently acting on the
 * EPhysics_Body associated with the part `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 *               params[2]: Embryo cell address for X component of force
 *               params[3]: Embryo cell address for Y component of force
 *               params[4]: Embryo cell address for Z component of force
 * @return 0 if EPhysics is loaded and values are retrieved, otherwise 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_get_forces(Embryo_Program *ep, Embryo_Cell *params)
{
   if (!EPH_LOAD()) return 0;
   return _edje_embryo_fn_physics_components_get(
            ep, params, EPH_CALL(ephysics_body_forces_get));
}

/**
 * @brief Gets the total applied torques on a physics-enabled part.
 *
 * This function is exposed to Embryo scripts as `physics_get_torques(part_id, &Float:x, &Float:y, &Float:z)`.
 * It retrieves the sum of all continuous torques currently acting on the
 * EPhysics_Body associated with the part `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 *               params[2]: Embryo cell address for X component of torque
 *               params[3]: Embryo cell address for Y component of torque
 *               params[4]: Embryo cell address for Z component of torque
 * @return 0 if EPhysics is loaded and values are retrieved, otherwise 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_get_torques(Embryo_Program *ep, Embryo_Cell *params)
{
   if (!EPH_LOAD()) return 0;
   return _edje_embryo_fn_physics_components_get(
            ep, params, EPH_CALL(ephysics_body_torques_get));
}

/**
 * @brief Sets the linear velocity of a physics-enabled part.
 *
 * This function is exposed to Embryo scripts as `physics_set_velocity(part_id, Float:x, Float:y, Float:z)`.
 * It directly sets the linear velocity of the EPhysics_Body associated
 * with the part `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 *               params[2]: X component of linear velocity
 *               params[3]: Y component of linear velocity
 *               params[4]: Z component of linear velocity
 * @return 0 if EPhysics is loaded and velocity is set, otherwise 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_set_velocity(Embryo_Program *ep, Embryo_Cell *params)
{
   if (!EPH_LOAD()) return 0;
   return _edje_embryo_fn_physics_components_set(
            ep, params, EPH_CALL(ephysics_body_linear_velocity_set));
}

/**
 * @brief Gets the linear velocity of a physics-enabled part.
 *
 * This function is exposed to Embryo scripts as `physics_get_velocity(part_id, &Float:x, &Float:y, &Float:z)`.
 * It retrieves the current linear velocity of the EPhysics_Body associated
 * with the part `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 *               params[2]: Embryo cell address for X component of linear velocity
 *               params[3]: Embryo cell address for Y component of linear velocity
 *               params[4]: Embryo cell address for Z component of linear velocity
 * @return 0 if EPhysics is loaded and values are retrieved, otherwise 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_get_velocity(Embryo_Program *ep, Embryo_Cell *params)
{
   if (!EPH_LOAD()) return 0;
   return _edje_embryo_fn_physics_components_get(
            ep, params, EPH_CALL(ephysics_body_linear_velocity_get));
}

/**
 * @brief Sets the angular velocity of a physics-enabled part.
 *
 * This function is exposed to Embryo scripts as `physics_set_ang_velocity(part_id, Float:x, Float:y, Float:z)`.
 * It directly sets the angular velocity of the EPhysics_Body associated
 * with the part `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 *               params[2]: X component of angular velocity (radians/sec)
 *               params[3]: Y component of angular velocity (radians/sec)
 *               params[4]: Z component of angular velocity (radians/sec)
 * @return 0 if EPhysics is loaded and velocity is set, otherwise 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_set_ang_velocity(Embryo_Program *ep, Embryo_Cell *params)
{
   if (!EPH_LOAD()) return 0;
   return _edje_embryo_fn_physics_components_set(
            ep, params, EPH_CALL(ephysics_body_angular_velocity_set));
}

/**
 * @brief Gets the angular velocity of a physics-enabled part.
 *
 * This function is exposed to Embryo scripts as `physics_get_ang_velocity(part_id, &Float:x, &Float:y, &Float:z)`.
 * It retrieves the current angular velocity of the EPhysics_Body associated
 * with the part `part_id`.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 *               params[2]: Embryo cell address for X component of angular velocity (radians/sec)
 *               params[3]: Embryo cell address for Y component of angular velocity (radians/sec)
 *               params[4]: Embryo cell address for Z component of angular velocity (radians/sec)
 * @return 0 if EPhysics is loaded and values are retrieved, otherwise 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_get_ang_velocity(Embryo_Program *ep, Embryo_Cell *params)
{
   if (!EPH_LOAD()) return 0;
   return _edje_embryo_fn_physics_components_get(
            ep, params, EPH_CALL(ephysics_body_angular_velocity_get));
}

/**
 * @brief Stops all motion of a physics-enabled part.
 *
 * This function is exposed to Embryo scripts as `physics_stop(part_id)`.
 * It sets both linear and angular velocities of the EPhysics_Body associated
 * with the part `part_id` to zero and clears any applied forces/torques.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_stop(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje_Real_Part *rp;
   int part_id = 0;
   Edje *ed;

   CHKPARAM(1);

   if (!EPH_LOAD()) return 0;
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;

   rp = ed->table_parts[part_id % ed->table_parts_size];
   if ((rp) && (rp->body))
     EPH_CALL(ephysics_body_stop)(rp->body);

   return 0;
}

/**
 * @brief Sets the rotation of a physics-enabled part using a quaternion.
 *
 * This function is exposed to Embryo scripts as `physics_set_rotation(part_id, Float:w, Float:x, Float:y, Float:z)`.
 * It sets the orientation of the EPhysics_Body associated with the part `part_id`
 * using the provided quaternion components (w, x, y, z). The quaternion will be normalized.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 *               params[2]: W component of the quaternion
 *               params[3]: X component of the quaternion
 *               params[4]: Y component of the quaternion
 *               params[5]: Z component of the quaternion
 * @return Always 0.
 */
static Embryo_Cell
_edje_embryo_fn_physics_set_rotation(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje_Real_Part *rp;
   int part_id = 0;
   Edje *ed;

   CHKPARAM(5);

   if (!EPH_LOAD()) return 0;
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;

   rp = ed->table_parts[part_id % ed->table_parts_size];
   if ((rp) && (rp->body))
     {
        EPhysics_Quaternion quat;
        double w, x, y, z;

        w = (double)EMBRYO_CELL_TO_FLOAT(params[2]);
        x = (double)EMBRYO_CELL_TO_FLOAT(params[3]);
        y = (double)EMBRYO_CELL_TO_FLOAT(params[4]);
        z = (double)EMBRYO_CELL_TO_FLOAT(params[5]);

        EPH_CALL(ephysics_quaternion_set)(&quat, x, y, z, w);
        EPH_CALL(ephysics_quaternion_normalize)(&quat);
        EPH_CALL(ephysics_body_rotation_set)(rp->body, &quat);
     }

   return 0;
}

/**
 * @brief Gets the rotation of a physics-enabled part as a quaternion.
 *
 * This function is exposed to Embryo scripts as `physics_get_rotation(part_id, &Float:w, &Float:x, &Float:y, &Float:z)`.
 * It retrieves the current orientation of the EPhysics_Body associated with
 * the part `part_id` as quaternion components (w, x, y, z).
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[1]: part_id
 *               params[2]: Embryo cell address for W component of the quaternion
 *               params[3]: Embryo cell address for X component of the quaternion
 *               params[4]: Embryo cell address for Y component of the quaternion
 *               params[5]: Embryo cell address for Z component of the quaternion
 * @return Always 0. Quaternion components are returned via output parameters.
 */
static Embryo_Cell
_edje_embryo_fn_physics_get_rotation(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje_Real_Part *rp;
   int part_id = 0;
   Edje *ed;

   CHKPARAM(5);

   if (!EPH_LOAD()) return 0;
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;

   rp = ed->table_parts[part_id % ed->table_parts_size];
   if ((rp) && (rp->body))
     {
        EPhysics_Quaternion quat;
        double w, x, y, z;

        EPH_CALL(ephysics_body_rotation_get)(rp->body, &quat);
        EPH_CALL(ephysics_quaternion_get)(&quat, &x, &y, &z, &w);

        SETFLOAT(w, params[2]);
        SETFLOAT(x, params[3]);
        SETFLOAT(y, params[4]);
        SETFLOAT(z, params[5]);
     }

   return 0;
}

#endif

/**
 * @brief Checks if a SWALLOW part currently contains a swallowed object.
 *
 * This function is exposed to Embryo scripts as `swallow_has_content(part_id)`.
 * It determines if the SWALLOW part identified by `part_id` has an object
 * currently swallowed within it.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the ID of the SWALLOW part.
 * @return 1 if the part is a SWALLOW part and has content, 0 otherwise.
 */
static Embryo_Cell
_edje_embryo_fn_swallow_has_content(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   int part_id = 0;
   Edje_Real_Part *rp;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   part_id = params[1];
   if (part_id < 0) return 0;
   rp = ed->table_parts[part_id % ed->table_parts_size];

   if ((!rp) ||
       (!rp->part) ||
       (rp->part->type != EDJE_PART_TYPE_SWALLOW) ||
       (!rp->typedata.swallow) ||
       (!rp->typedata.swallow->swallowed_object))
      return 0;

   return 1;
}

/**
 * @brief Prints a message to stderr, prefixed with Edje object information.
 *
 * This function is exposed to Embryo scripts as `echo(message[])`.
 * It's primarily a debugging utility to print messages from an Embryo script
 * to the standard error output. The output includes the Edje object's memory
 * address, file path, and group name.
 *
 * @param ep The Embryo program instance.
 * @param params An array of Embryo cells.
 *               params[0] is the number of arguments.
 *               params[1] is the Embryo cell address of the message string to print.
 * @return 0 on success, -1 if the message string is NULL.
 */
static Embryo_Cell
_edje_embryo_fn_echo(Embryo_Program *ep, Embryo_Cell *params)
{
   Edje *ed;
   char *p;

   CHKPARAM(1);
   ed = embryo_program_data_get(ep);
   GETSTR(p, params[1]);
   if (!p) return -1;
   fprintf(stderr,
           "<EDJE ECHO> [%llx | %s:%s]: %s\n",
           (unsigned long long)((uintptr_t)ed),
           ed->path,
           ed->group,
           p);
   return 0;
}

/**
 * @brief Initializes the Embryo script environment for an Edje part collection.
 *
 * This function registers all the Edje-specific native functions (like `get_int`,
 * `set_state`, `emit`, etc.) with the Embryo program associated with the
 * given Edje part collection (`edc`). This makes these functions callable
 * from Embryo scripts within that collection.
 *
 * This is typically called when an Edje object is created or its file/group is set.
 *
 * @param edc Pointer to the Edje_Part_Collection whose script environment is to be initialized.
 */
void
_edje_embryo_script_init(Edje_Part_Collection *edc)
{
   Embryo_Program *ep;

   if (!edc) return;
   if (!edc->script) return;

   ep = edc->script;
   /* first advertise all the edje "script" calls */
   embryo_program_native_call_add(ep, "get_int", _edje_embryo_fn_get_int);
   embryo_program_native_call_add(ep, "set_int", _edje_embryo_fn_set_int);
   embryo_program_native_call_add(ep, "get_float", _edje_embryo_fn_get_float);
   embryo_program_native_call_add(ep, "set_float", _edje_embryo_fn_set_float);
   embryo_program_native_call_add(ep, "get_str", _edje_embryo_fn_get_str);
   embryo_program_native_call_add(ep, "get_strlen", _edje_embryo_fn_get_strlen);
   embryo_program_native_call_add(ep, "set_str", _edje_embryo_fn_set_str);
   embryo_program_native_call_add(ep, "count", _edje_embryo_fn_count);
   embryo_program_native_call_add(ep, "remove", _edje_embryo_fn_remove);
   embryo_program_native_call_add(ep, "append_int", _edje_embryo_fn_append_int);
   embryo_program_native_call_add(ep, "prepend_int", _edje_embryo_fn_prepend_int);
   embryo_program_native_call_add(ep, "insert_int", _edje_embryo_fn_insert_int);
   embryo_program_native_call_add(ep, "replace_int", _edje_embryo_fn_replace_int);
   embryo_program_native_call_add(ep, "fetch_int", _edje_embryo_fn_fetch_int);
   embryo_program_native_call_add(ep, "append_str", _edje_embryo_fn_append_str);
   embryo_program_native_call_add(ep, "prepend_str", _edje_embryo_fn_prepend_str);
   embryo_program_native_call_add(ep, "insert_str", _edje_embryo_fn_insert_str);
   embryo_program_native_call_add(ep, "replace_str", _edje_embryo_fn_replace_str);
   embryo_program_native_call_add(ep, "fetch_str", _edje_embryo_fn_fetch_str);
   embryo_program_native_call_add(ep, "append_float", _edje_embryo_fn_append_float);
   embryo_program_native_call_add(ep, "prepend_float", _edje_embryo_fn_prepend_float);
   embryo_program_native_call_add(ep, "insert_float", _edje_embryo_fn_insert_float);
   embryo_program_native_call_add(ep, "replace_float", _edje_embryo_fn_replace_float);
   embryo_program_native_call_add(ep, "fetch_float", _edje_embryo_fn_fetch_float);

   embryo_program_native_call_add(ep, "timer", _edje_embryo_fn_timer);
   embryo_program_native_call_add(ep, "cancel_timer", _edje_embryo_fn_cancel_timer);
   embryo_program_native_call_add(ep, "reset_timer", _edje_embryo_fn_reset_timer);

   embryo_program_native_call_add(ep, "anim", _edje_embryo_fn_anim);
   embryo_program_native_call_add(ep, "cancel_anim", _edje_embryo_fn_cancel_anim);
   embryo_program_native_call_add(ep, "get_anim_pos_map", _edje_embryo_fn_get_anim_pos_map);

   embryo_program_native_call_add(ep, "emit", _edje_embryo_fn_emit);
   embryo_program_native_call_add(ep, "get_part_id", _edje_embryo_fn_get_part_id);
   embryo_program_native_call_add(ep, "get_image_id", _edje_embryo_fn_get_image_id);
   embryo_program_native_call_add(ep, "get_program_id", _edje_embryo_fn_get_program_id);
   embryo_program_native_call_add(ep, "set_state", _edje_embryo_fn_set_state);
   embryo_program_native_call_add(ep, "get_state", _edje_embryo_fn_get_state);
   embryo_program_native_call_add(ep, "set_tween_state", _edje_embryo_fn_set_tween_state);
   embryo_program_native_call_add(ep, "set_tween_state_anim", _edje_embryo_fn_set_tween_state_anim);
   embryo_program_native_call_add(ep, "run_program", _edje_embryo_fn_run_program);
   embryo_program_native_call_add(ep, "get_drag_dir", _edje_embryo_fn_get_drag_dir);
   embryo_program_native_call_add(ep, "get_drag", _edje_embryo_fn_get_drag);
   embryo_program_native_call_add(ep, "set_drag", _edje_embryo_fn_set_drag);
   embryo_program_native_call_add(ep, "get_drag_size", _edje_embryo_fn_get_drag_size);
   embryo_program_native_call_add(ep, "set_drag_size", _edje_embryo_fn_set_drag_size);
   embryo_program_native_call_add(ep, "set_text", _edje_embryo_fn_set_text);
   embryo_program_native_call_add(ep, "get_text", _edje_embryo_fn_get_text);
   embryo_program_native_call_add(ep, "get_min_size", _edje_embryo_fn_get_min_size);
   embryo_program_native_call_add(ep, "get_max_size", _edje_embryo_fn_get_max_size);
   embryo_program_native_call_add(ep, "get_color_class", _edje_embryo_fn_get_color_class);
   embryo_program_native_call_add(ep, "set_color_class", _edje_embryo_fn_set_color_class);
   embryo_program_native_call_add(ep, "set_text_class", _edje_embryo_fn_set_text_class);
   embryo_program_native_call_add(ep, "get_text_class", _edje_embryo_fn_get_text_class);
   embryo_program_native_call_add(ep, "get_drag_step", _edje_embryo_fn_get_drag_step);
   embryo_program_native_call_add(ep, "set_drag_step", _edje_embryo_fn_set_drag_step);
   embryo_program_native_call_add(ep, "get_drag_page", _edje_embryo_fn_get_drag_page);
   embryo_program_native_call_add(ep, "set_drag_page", _edje_embryo_fn_set_drag_page);
   embryo_program_native_call_add(ep, "get_mouse", _edje_embryo_fn_get_mouse);
   embryo_program_native_call_add(ep, "get_mouse_buttons", _edje_embryo_fn_get_mouse_buttons);
   embryo_program_native_call_add(ep, "stop_program", _edje_embryo_fn_stop_program);
   embryo_program_native_call_add(ep, "stop_programs_on", _edje_embryo_fn_stop_programs_on);
   embryo_program_native_call_add(ep, "set_min_size", _edje_embryo_fn_set_min_size);
   embryo_program_native_call_add(ep, "set_max_size", _edje_embryo_fn_set_max_size);
   embryo_program_native_call_add(ep, "play_sample", _edje_embryo_fn_play_sample);
   embryo_program_native_call_add(ep, "play_tone", _edje_embryo_fn_play_tone);
   embryo_program_native_call_add(ep, "play_vibration", _edje_embryo_fn_play_vibration);
   embryo_program_native_call_add(ep, "send_message", _edje_embryo_fn_send_message);
   embryo_program_native_call_add(ep, "get_geometry", _edje_embryo_fn_get_geometry);
   embryo_program_native_call_add(ep, "custom_state", _edje_embryo_fn_custom_state);
   embryo_program_native_call_add(ep, "set_state_val", _edje_embryo_fn_set_state_val);
   embryo_program_native_call_add(ep, "get_state_val", _edje_embryo_fn_get_state_val);
   embryo_program_native_call_add(ep, "set_state_anim", _edje_embryo_fn_set_state_anim);

   embryo_program_native_call_add(ep, "set_mouse_events", _edje_embryo_fn_set_mouse_events);
   embryo_program_native_call_add(ep, "get_mouse_events", _edje_embryo_fn_get_mouse_events);
   embryo_program_native_call_add(ep, "set_pointer_mode", _edje_embryo_fn_set_pointer_mode);
   embryo_program_native_call_add(ep, "set_repeat_events", _edje_embryo_fn_set_repeat_events);
   embryo_program_native_call_add(ep, "get_repeat_events", _edje_embryo_fn_get_repeat_events);
   embryo_program_native_call_add(ep, "set_ignore_flags", _edje_embryo_fn_set_ignore_flags);
   embryo_program_native_call_add(ep, "get_ignore_flags", _edje_embryo_fn_get_ignore_flags);
   embryo_program_native_call_add(ep, "set_mask_flags", _edje_embryo_fn_set_mask_flags);
   embryo_program_native_call_add(ep, "get_mask_flags", _edje_embryo_fn_get_mask_flags);

   embryo_program_native_call_add(ep, "set_focus", _edje_embryo_fn_set_focus);
   embryo_program_native_call_add(ep, "unset_focus", _edje_embryo_fn_unset_focus);

   embryo_program_native_call_add(ep, "part_swallow", _edje_embryo_fn_part_swallow);

   embryo_program_native_call_add(ep, "external_param_get_int", _edje_embryo_fn_external_param_get_int);
   embryo_program_native_call_add(ep, "external_param_set_int", _edje_embryo_fn_external_param_set_int);
   embryo_program_native_call_add(ep, "external_param_get_float", _edje_embryo_fn_external_param_get_float);
   embryo_program_native_call_add(ep, "external_param_set_float", _edje_embryo_fn_external_param_set_float);
   embryo_program_native_call_add(ep, "external_param_get_strlen", _edje_embryo_fn_external_param_get_strlen);
   embryo_program_native_call_add(ep, "external_param_get_str", _edje_embryo_fn_external_param_get_str);
   embryo_program_native_call_add(ep, "external_param_set_str", _edje_embryo_fn_external_param_set_str);
   embryo_program_native_call_add(ep, "external_param_get_choice_len", _edje_embryo_fn_external_param_get_choice_len);
   embryo_program_native_call_add(ep, "external_param_get_choice", _edje_embryo_fn_external_param_get_choice);
   embryo_program_native_call_add(ep, "external_param_set_choice", _edje_embryo_fn_external_param_set_choice);
   embryo_program_native_call_add(ep, "external_param_get_bool", _edje_embryo_fn_external_param_get_bool);
   embryo_program_native_call_add(ep, "external_param_set_bool", _edje_embryo_fn_external_param_set_bool);

#ifdef HAVE_EPHYSICS
   embryo_program_native_call_add(ep, "physics_impulse", _edje_embryo_fn_physics_impulse);
   embryo_program_native_call_add(ep, "physics_torque_impulse", _edje_embryo_fn_physics_torque_impulse);
   embryo_program_native_call_add(ep, "physics_force", _edje_embryo_fn_physics_force);
   embryo_program_native_call_add(ep, "physics_torque", _edje_embryo_fn_physics_torque);
   embryo_program_native_call_add(ep, "physics_clear_forces", _edje_embryo_fn_physics_clear_forces);
   embryo_program_native_call_add(ep, "physics_get_forces", _edje_embryo_fn_physics_get_forces);
   embryo_program_native_call_add(ep, "physics_get_torques", _edje_embryo_fn_physics_get_torques);
   embryo_program_native_call_add(ep, "physics_set_velocity", _edje_embryo_fn_physics_set_velocity);
   embryo_program_native_call_add(ep, "physics_get_velocity", _edje_embryo_fn_physics_get_velocity);
   embryo_program_native_call_add(ep, "physics_set_ang_velocity", _edje_embryo_fn_physics_set_ang_velocity);
   embryo_program_native_call_add(ep, "physics_get_ang_velocity", _edje_embryo_fn_physics_get_ang_velocity);
   embryo_program_native_call_add(ep, "physics_stop", _edje_embryo_fn_physics_stop);
   embryo_program_native_call_add(ep, "physics_set_rotation", _edje_embryo_fn_physics_set_rotation);
   embryo_program_native_call_add(ep, "physics_get_rotation", _edje_embryo_fn_physics_get_rotation);
#endif

   embryo_program_native_call_add(ep, "swallow_has_content", _edje_embryo_fn_swallow_has_content);
   embryo_program_native_call_add(ep, "echo", _edje_embryo_fn_echo);
}

/**
 * @brief Shuts down and frees the Embryo script environment for an Edje part collection.
 *
 * This function frees the Embryo_Program associated with the given
 * Edje_Part_Collection, provided there are no active recursions into the script.
 * It effectively unloads the script and cleans up its resources.
 *
 * This is typically called when an Edje object is being destroyed or its
 * file/group is changed, and the old collection's script is no longer needed.
 *
 * @param edc Pointer to the Edje_Part_Collection whose script environment is to be shut down.
 */
void
_edje_embryo_script_shutdown(Edje_Part_Collection *edc)
{
   if (!edc) return;
   if (!edc->script) return;
   if (embryo_program_recursion_get(edc->script) > 0) return;
   embryo_program_free(edc->script);
   edc->script = NULL;
}

/**
 * @brief Resets the Embryo virtual machine and re-initializes global variables for an Edje object.
 *
 * This function resets the VM state of the Embryo script associated with the
 * Edje object's current collection. It also re-initializes any global variables
 * defined in the Embryo script to their Edje-specific magic values, allowing
 * them to be used as IDs for Edje data elements (like variables from `data` blocks).
 *
 * This is called, for example, before running an Edje program to ensure a clean
 * script environment. It only proceeds if there are no active recursions into the script.
 *
 * @param ed Pointer to the Edje object whose script is to be reset.
 */
void
_edje_embryo_script_reset(Edje *ed)
{
   if (!ed) return;
   if (!ed->collection) return;
   if (!ed->collection->script) return;
   if (embryo_program_recursion_get(ed->collection->script) > 0) return;
   embryo_program_vm_reset(ed->collection->script);
   _edje_embryo_globals_init(ed);
}

/**
 * @brief Executes a specific Edje program (Embryo function) within an Edje object's script.
 *
 * This function is responsible for running an Edje program, which is essentially
 * an Embryo function (conventionally named `_p<program_id>`). It sets up the
 * Embryo VM, pushes the signal and source strings as parameters to the Embryo
 * function, sets the Edje object as context data for native calls, and then
 * runs the function. It also handles error reporting if the script fails or
 * runs for too long.
 *
 * The name "test_run" might be historical; this is the core execution path for
 * Edje programs triggered by signals.
 *
 * @param ed Pointer to the Edje object.
 * @param pr Pointer to the Edje_Program to be executed.
 * @param sig The signal string that triggered this program run.
 * @param src The source string associated with the signal.
 */
void
_edje_embryo_test_run(Edje *ed, Edje_Program *pr, const char *sig, const char *src)
{
   char fname[128];
   Embryo_Function fn;

   if (!ed) return;
   if (!ed->collection) return;
   if (!ed->collection->script) return;
   embryo_program_vm_push(ed->collection->script);
   _edje_embryo_globals_init(ed);

   //   _edje_embryo_script_reset(ed);
   snprintf(fname, sizeof(fname), "_p%i", pr->id);
   fn = embryo_program_function_find(ed->collection->script, (char *)fname);
   if (fn != EMBRYO_FUNCTION_NONE)
     {
        void *pdata;
        int ret;

        embryo_parameter_string_push(ed->collection->script, (char *)sig);
        embryo_parameter_string_push(ed->collection->script, (char *)src);
        pdata = embryo_program_data_get(ed->collection->script);
        embryo_program_data_set(ed->collection->script, ed);
        /* 5 million instructions is an arbitrary number. on my p4-2.6 here */
        /* IF embryo is ONLY running embryo stuff and NO native calls that's */
        /* about 0.016 seconds, and longer on slower cpu's. if a simple */
        /* embryo script snippet hasn't managed to do its work in 5 MILLION */
        /* embryo virtual machine instructions - something is wrong, or */
        /* embryo is simply being mis-used. Embryo is meant to be minimal */
        /* logic enhancment - not entire applications. this cycle count */
        /* does NOT include time spent in native function calls, that the */
        /* script may call to do the REAL work, so in terms of time this */
        /* will likely end up being much longer than 0.016 seconds - more */
        /* like 0.03 - 0.05 seconds or even more */
        embryo_program_max_cycle_run_set(ed->collection->script, 5000000);
        if (embryo_program_recursion_get(ed->collection->script) && (!ed->collection->script_recursion))
          ERR("You are running Embryo->EDC->Embryo with script program '%s';\n"
              "A run_program runs the '%d'th program '%s' in the group '%s' of file %s;\n"
              "By the power of Grayskull, your previous Embryo stack is now broken!",
              fname, (fn + 1), pr->name, ed->group, ed->path);

        ret = embryo_program_run(ed->collection->script, fn);
        if (ret == EMBRYO_PROGRAM_FAIL)
          {
             ERR("ERROR with embryo script. "
                 "OBJECT NAME: '%s', "
                 "OBJECT FILE: '%s', "
                 "ENTRY POINT: '%s (%s)', "
                 "SIGNAL: '%s', "
                 "SOURCE: '%s', "
                 "ERROR: '%s'",
                 ed->collection->part,
                 ed->file->path,
                 fname, pr->name,
                 sig, src,
                 embryo_error_string_get(embryo_program_error_get(ed->collection->script)));
          }
        else if (ret == EMBRYO_PROGRAM_TOOLONG)
          {
             ERR("ERROR with embryo script. "
                 "OBJECT NAME: '%s', "
                 "OBJECT FILE: '%s', "
                 "ENTRY POINT: '%s (%s)', "
                 "SIGNAL: '%s', "
                 "SOURCE: '%s', "
                 "ERROR: 'Script exceeded maximum allowed cycle count of %i'",
                 ed->collection->part,
                 ed->file->path,
                 fname, pr->name,
                 sig, src,
                 embryo_program_max_cycle_run_get(ed->collection->script));
          }
        embryo_program_data_set(ed->collection->script, pdata);
     }
   embryo_program_vm_pop(ed->collection->script);
}

/**
 * @brief Initializes global variables in an Edje object's Embryo script.
 *
 * Embryo scripts in Edje can define global variables. This function iterates
 * through these global variables and assigns them a "magic" value. This magic
 * value (`EDJE_VAR_MAGIC_BASE + variable_index`) allows Edje's native functions
 * (like `get_int`, `set_str`) to identify which Edje data item (often defined
 * in the `data` block of the EDC file) the script is referring to when it uses
 * that global variable as an ID.
 *
 * For example, if an EDC has:
 * `data { item: "my_value" "0"; }`
 * and the script has:
 * `global my_value_id;`
 * After this function, `my_value_id` in the script will hold a cell that, when
 * passed to `get_int()`, resolves to the Edje data item "my_value".
 *
 * @param ed Pointer to the Edje object whose script globals are to be initialized.
 */
void
_edje_embryo_globals_init(Edje *ed)
{
   int n, i;
   Embryo_Program *ep;

   ep = ed->collection->script;
   n = embryo_program_variable_count_get(ep);
   for (i = 0; i < n; i++)
     {
        Embryo_Cell cell, *cptr;

        cell = embryo_program_variable_get(ep, i);
        if (cell != EMBRYO_CELL_NONE)
          {
             cptr = embryo_data_address_get(ep, cell);
             if (cptr) *cptr = EDJE_VAR_MAGIC_BASE + i;
          }
     }
}

