#pragma once

/* 人畜無害なenumヘッダを先に読み込む */
#include "system/angband.h"

#include "object/tval-types.h"
#include "player-info/class-types.h"
#include "realm/realm-types.h"
#include "spell/technic-info-table.h"

#include <string>
#include <vector>

/** m_info.txtでMPの無い職業に設定される */
#define SPELL_FIRST_NO_SPELL 99

/*
 * Information about the player's "magic"
 *
 * Note that a player with a "spell_book" of "zero" is illiterate.
 */

struct player_magic {
    ItemKindType spell_book{}; /* Tval of spell books (if any) */
    BIT_FLAGS8 spell_xtra{}; /* Something for later */

    int spell_stat{}; /* Stat for spells (if any)  */
    int spell_type{}; /* Spell type (mage/priest) */

    int spell_first{}; /* Level of first spell */
    int spell_weight{}; /* Weight that hurts spells */

    magic_type info[MAX_MAGIC][32]{}; /* The available spells */
};

extern std::vector<player_magic> m_info;
extern const player_magic *mp_ptr;

struct player_class_info {
    concptr title; /* Type of class */

#ifdef JP
    concptr E_title; /* 英語職業 */
#endif
    int16_t c_adj[6]; /* Class stat modifier */

    int16_t c_dis; /* class disarming */
    int16_t c_dev; /* class magic devices */
    int16_t c_sav; /* class saving throws */
    int16_t c_stl; /* class stealth */
    int16_t c_srh; /* class searching ability */
    int16_t c_fos; /* class searching frequency */
    int16_t c_thn; /* class to hit (normal) */
    int16_t c_thb; /* class to hit (bows) */

    int16_t x_dis; /* extra disarming */
    int16_t x_dev; /* extra magic devices */
    int16_t x_sav; /* extra saving throws */
    int16_t x_stl; /* extra stealth */
    int16_t x_srh; /* extra searching ability */
    int16_t x_fos; /* extra searching frequency */
    int16_t x_thn; /* extra to hit (normal) */
    int16_t x_thb; /* extra to hit (bows) */

    int16_t c_mhp; /* Class hit-dice adjustment */
    int16_t c_exp; /* Class experience factor */

    byte pet_upkeep_div; /* Pet upkeep divider */

    int num;
    int wgt;
    int mul;
};

extern const player_class_info *cp_ptr;
extern const std::vector<player_class_info> class_info;
extern const std::vector<std::vector<std::string_view>> player_titles;
