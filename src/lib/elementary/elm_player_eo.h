#ifndef _ELM_PLAYER_EO_H_
#define _ELM_PLAYER_EO_H_

#ifndef _ELM_PLAYER_EO_CLASS_TYPE
#define _ELM_PLAYER_EO_CLASS_TYPE

/**
 * @brief Typedef for the Elm_Player Evas Object (Eo) type.
 * @ingroup Elm_Player
 */
typedef Eo Elm_Player;

#endif

#ifndef _ELM_PLAYER_EO_TYPES
#define _ELM_PLAYER_EO_TYPES

/**
 * @brief Placeholder for future Elm_Player specific types.
 * @ingroup Elm_Player
 */

#endif
/** Elementary player class
 *
 * @ingroup Elm_Player
 */
#define ELM_PLAYER_CLASS elm_player_class_get()

EWAPI const Efl_Class *elm_player_class_get(void) EINA_CONST;

/**
 * @brief Event descriptor for the "forward,clicked" event.
 * This event is triggered when the forward button on the player is clicked.
 */
EWAPI extern const Efl_Event_Description _ELM_PLAYER_EVENT_FORWARD_CLICKED;

/** Called when forward was clicked
 *
 * @ingroup Elm_Player
 */
#define ELM_PLAYER_EVENT_FORWARD_CLICKED (&(_ELM_PLAYER_EVENT_FORWARD_CLICKED))

/**
 * @brief Event descriptor for the "info,clicked" event.
 * This event is triggered when the info button on the player is clicked.
 */
EWAPI extern const Efl_Event_Description _ELM_PLAYER_EVENT_INFO_CLICKED;

/** Called when info was clicked
 *
 * @ingroup Elm_Player
 */
#define ELM_PLAYER_EVENT_INFO_CLICKED (&(_ELM_PLAYER_EVENT_INFO_CLICKED))

/**
 * @brief Event descriptor for the "next,clicked" event.
 * This event is triggered when the next button on the player is clicked.
 */
EWAPI extern const Efl_Event_Description _ELM_PLAYER_EVENT_NEXT_CLICKED;

/** Called when next was clicked
 *
 * @ingroup Elm_Player
 */
#define ELM_PLAYER_EVENT_NEXT_CLICKED (&(_ELM_PLAYER_EVENT_NEXT_CLICKED))

/**
 * @brief Event descriptor for the "pause,clicked" event.
 * This event is triggered when the pause button on the player is clicked.
 */
EWAPI extern const Efl_Event_Description _ELM_PLAYER_EVENT_PAUSE_CLICKED;

/** Called when pause was clicked
 *
 * @ingroup Elm_Player
 */
#define ELM_PLAYER_EVENT_PAUSE_CLICKED (&(_ELM_PLAYER_EVENT_PAUSE_CLICKED))

/**
 * @brief Event descriptor for the "play,clicked" event.
 * This event is triggered when the play button on the player is clicked.
 */
EWAPI extern const Efl_Event_Description _ELM_PLAYER_EVENT_PLAY_CLICKED;

/** Called when play was clicked
 *
 * @ingroup Elm_Player
 */
#define ELM_PLAYER_EVENT_PLAY_CLICKED (&(_ELM_PLAYER_EVENT_PLAY_CLICKED))

/**
 * @brief Event descriptor for the "prev,clicked" event.
 * This event is triggered when the previous button on the player is clicked.
 */
EWAPI extern const Efl_Event_Description _ELM_PLAYER_EVENT_PREV_CLICKED;

/** Called when previous was clicked
 *
 * @ingroup Elm_Player
 */
#define ELM_PLAYER_EVENT_PREV_CLICKED (&(_ELM_PLAYER_EVENT_PREV_CLICKED))

/**
 * @brief Event descriptor for the "rewind,clicked" event.
 * This event is triggered when the rewind button on the player is clicked.
 */
EWAPI extern const Efl_Event_Description _ELM_PLAYER_EVENT_REWIND_CLICKED;

/** Called when rewind was clicked
 *
 * @ingroup Elm_Player
 */
#define ELM_PLAYER_EVENT_REWIND_CLICKED (&(_ELM_PLAYER_EVENT_REWIND_CLICKED))

/**
 * @brief Event descriptor for the "quality,clicked" event.
 * This event is triggered when the quality setting button on the player is clicked.
 */
EWAPI extern const Efl_Event_Description _ELM_PLAYER_EVENT_QUALITY_CLICKED;

/** Called when quality was clicked
 *
 * @ingroup Elm_Player
 */
#define ELM_PLAYER_EVENT_QUALITY_CLICKED (&(_ELM_PLAYER_EVENT_QUALITY_CLICKED))

/**
 * @brief Event descriptor for the "eject,clicked" event.
 * This event is triggered when the eject button on the player is clicked.
 */
EWAPI extern const Efl_Event_Description _ELM_PLAYER_EVENT_EJECT_CLICKED;

/** Called when eject was clicked
 *
 * @ingroup Elm_Player
 */
#define ELM_PLAYER_EVENT_EJECT_CLICKED (&(_ELM_PLAYER_EVENT_EJECT_CLICKED))

/**
 * @brief Event descriptor for the "volume,clicked" event.
 * This event is triggered when the volume control on the player is clicked.
 */
EWAPI extern const Efl_Event_Description _ELM_PLAYER_EVENT_VOLUME_CLICKED;

/** Called when volume was clicked
 *
 * @ingroup Elm_Player
 */
#define ELM_PLAYER_EVENT_VOLUME_CLICKED (&(_ELM_PLAYER_EVENT_VOLUME_CLICKED))

/**
 * @brief Event descriptor for the "mute,clicked" event.
 * This event is triggered when the mute button on the player is clicked.
 */
EWAPI extern const Efl_Event_Description _ELM_PLAYER_EVENT_MUTE_CLICKED;

/** Called when mute was clicked
 *
 * @ingroup Elm_Player
 */
#define ELM_PLAYER_EVENT_MUTE_CLICKED (&(_ELM_PLAYER_EVENT_MUTE_CLICKED))

#endif
