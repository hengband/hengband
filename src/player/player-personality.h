#pragma once

#include "player/player-personality-types.h"
#include "system/angband.h"

typedef struct player_personality {
	concptr title;			/* Type of personality */

#ifdef JP
	concptr E_title;		/* 英語性格 */
#endif

	short a_adj[6];		/* ersonality stat bonuses */

	short a_dis;			/* personality disarming */
	short a_dev;			/* personality magic devices */
	short a_sav;			/* personality saving throw */
	short a_stl;			/* personality stealth */
	short a_srh;			/* personality search ability */
	short a_fos;			/* personality search frequency */
	short a_thn;			/* personality combat (normal) */
	short a_thb;			/* personality combat (shooting) */

	short a_mhp;			/* Race hit-dice modifier */

	byte no;			/* の */
	byte sex;			/* seibetu seigen */
} player_personality;

extern const player_personality personality_info[MAX_PERSONALITIES];
extern const player_personality *ap_ptr;
