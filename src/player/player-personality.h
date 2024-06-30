#pragma once

#include "locale/localized-string.h"
#include "player/player-personality-types.h"
#include "system/angband.h"

struct player_personality {
    LocalizedString title; /* Type of personality */

    int16_t a_adj[6]; /* ersonality stat bonuses */

    int16_t a_dis; /* personality disarming */
    int16_t a_dev; /* personality magic devices */
    int16_t a_sav; /* personality saving throw */
    int16_t a_stl; /* personality stealth */
    int16_t a_srh; /* personality search ability */
    int16_t a_fos; /* personality search frequency */
    int16_t a_thn; /* personality combat (normal) */
    int16_t a_thb; /* personality combat (shooting) */

    int16_t a_mhp; /* Race hit-dice modifier */

    byte no; /* の */
    byte sex; /* seibetu seigen */
};

extern const player_personality personality_info[MAX_PERSONALITIES];
extern const player_personality *ap_ptr;
