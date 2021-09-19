#pragma once

#include "system/angband.h"

#define DIARY_DIALY				0
#define DIARY_DESCRIPTION		1
#define DIARY_ART				2
#define DIARY_UNIQUE			3
#define DIARY_FIX_QUEST_C		4
#define DIARY_FIX_QUEST_F		5
#define DIARY_RAND_QUEST_C		6
#define DIARY_RAND_QUEST_F		7
#define DIARY_MAXDEAPTH			8
#define DIARY_TRUMP				9
#define DIARY_STAIR				10
#define DIARY_RECALL			11
#define DIARY_TO_QUEST			12
#define DIARY_TELEPORT_LEVEL	13
#define DIARY_BUY				14
#define DIARY_SELL				15
#define DIARY_ARENA				16
#define DIARY_FOUND				17
#define DIARY_LEVELUP			18
#define DIARY_GAMESTART			19
#define DIARY_WIZ_TELE			20
#define DIARY_NAMED_PET			21
#define DIARY_PAT_TELE			22
#define DIARY_ART_SCROLL		23
#define DIARY_WIZARD_LOG		24

#define RECORD_NAMED_PET_NAME        0
#define RECORD_NAMED_PET_UNNAME      1
#define RECORD_NAMED_PET_DISMISS     2
#define RECORD_NAMED_PET_DEATH       3
#define RECORD_NAMED_PET_MOVED       4
#define RECORD_NAMED_PET_LOST_SIGHT  5
#define RECORD_NAMED_PET_DESTROY     6
#define RECORD_NAMED_PET_EARTHQUAKE  7
#define RECORD_NAMED_PET_GENOCIDE    8
#define RECORD_NAMED_PET_WIZ_ZAP     9
#define RECORD_NAMED_PET_TELE_LEVEL  10
#define RECORD_NAMED_PET_BLAST       11
#define RECORD_NAMED_PET_HEAL_LEPER  12
#define RECORD_NAMED_PET_COMPACT     13
#define RECORD_NAMED_PET_LOSE_PARENT 14

extern bool write_level;

struct player_type;
#ifdef JP
#else
concptr get_ordinal_number_suffix(int num);
#endif
errr exe_write_diary(player_type *player_ptr, int type, int num, concptr note);
