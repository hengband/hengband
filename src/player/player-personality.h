#pragma once

#include "player/player-personalities-types.h"
#include "system/angband.h"

typedef struct player_personality {
	concptr title;			/* Type of personality */

#ifdef JP
	concptr E_title;		/* 英語性格 */
#endif

	s16b a_adj[6];		/* ersonality stat bonuses */

	s16b a_dis;			/* personality disarming */
	s16b a_dev;			/* personality magic devices */
	s16b a_sav;			/* personality saving throw */
	s16b a_stl;			/* personality stealth */
	s16b a_srh;			/* personality search ability */
	s16b a_fos;			/* personality search frequency */
	s16b a_thn;			/* personality combat (normal) */
	s16b a_thb;			/* personality combat (shooting) */

	s16b a_mhp;			/* Race hit-dice modifier */

	byte no;			/* の */
	byte sex;			/* seibetu seigen */
} player_personality;

extern const player_personality personality_info[MAX_PERSONALITIES];
extern const player_personality *ap_ptr;
