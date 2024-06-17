#pragma once

#include "spell/technic-info-table.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

/** ClassMagicDefinitions.txtでMPの無い職業に設定される */
#define SPELL_FIRST_NO_SPELL 99

/*
 * Information about the player's "magic"
 *
 * Note that a player with a "spell_book" of "zero" is illiterate.
 */

enum class ItemKindType : short;
struct player_magic {
    ItemKindType spell_book{}; /* Tval of spell books (if any) */
    bool has_glove_mp_penalty{}; /* 籠手装備によるMP減少 */
    bool has_magic_fail_rate_cap{}; /* 魔法失率の5%キャップ */
    bool is_spell_trainable{}; /* 魔法熟練度 */

    int spell_stat{}; /* Stat for spells (if any)  */
    int spell_type{}; /* Spell type (mage/priest) */

    int spell_first{}; /* Level of first spell */
    int spell_weight{}; /* Weight that hurts spells */

    magic_type info[MAX_MAGIC][32]{}; /* The available spells */
};

extern std::vector<player_magic> class_magics_info;
extern const player_magic *mp_ptr;

struct player_class_info {
    std::string title; //!< 職種

#ifdef JP
    std::string E_title; //!< 職種 (英語表記)
#endif
    short c_adj[6]; /* Class stat modifier */

    short c_dis; /* class disarming */
    short c_dev; /* class magic devices */
    short c_sav; /* class saving throws */
    short c_stl; /* class stealth */
    short c_srh; /* class searching ability */
    short c_fos; /* class searching frequency */
    short c_thn; /* class to hit (normal) */
    short c_thb; /* class to hit (bows) */

    short x_dis; /* extra disarming */
    short x_dev; /* extra magic devices */
    short x_sav; /* extra saving throws */
    short x_stl; /* extra stealth */
    short x_srh; /* extra searching ability */
    short x_fos; /* extra searching frequency */
    short x_thn; /* extra to hit (normal) */
    short x_thb; /* extra to hit (bows) */

    short c_mhp; /* Class hit-dice adjustment */
    short c_exp; /* Class experience factor */

    uint8_t pet_upkeep_div; /* Pet upkeep divider */

    int num;
    int wgt;
    int mul;
};

enum class PlayerClassType : short;
extern const player_class_info *cp_ptr;
extern const std::map<PlayerClassType, player_class_info> class_info;
extern const std::map<PlayerClassType, std::vector<std::string>> player_titles;
