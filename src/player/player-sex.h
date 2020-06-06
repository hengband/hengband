#pragma once

#include "system/angband.h"

typedef struct player_sex {
	concptr title;			/* Type of sex */
	concptr winner;		/* Name of winner */
#ifdef JP
	concptr E_title;		/* 英語性別 */
	concptr E_winner;		/* 英語性別 */
#endif
} player_sex;

extern const player_sex sex_info[MAX_SEXES];
extern const player_sex *sp_ptr;
