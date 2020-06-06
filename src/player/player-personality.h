#pragma once

#include "system/angband.h"

#define MAX_SEIKAKU 13 /*!< 性格の最大定義数 */

typedef struct player_seikaku player_seikaku;
struct player_seikaku
{
	concptr title;			/* Type of seikaku */

#ifdef JP
	concptr E_title;		/* 英語性格 */
#endif

	s16b a_adj[6];		/* seikaku stat bonuses */

	s16b a_dis;			/* seikaku disarming */
	s16b a_dev;			/* seikaku magic devices */
	s16b a_sav;			/* seikaku saving throw */
	s16b a_stl;			/* seikaku stealth */
	s16b a_srh;			/* seikaku search ability */
	s16b a_fos;			/* seikaku search frequency */
	s16b a_thn;			/* seikaku combat (normal) */
	s16b a_thb;			/* seikaku combat (shooting) */

	s16b a_mhp;			/* Race hit-dice modifier */

	byte no;			/* の */
	byte sex;			/* seibetu seigen */
};

extern const player_seikaku seikaku_info[MAX_SEIKAKU];

extern const player_seikaku *ap_ptr;
