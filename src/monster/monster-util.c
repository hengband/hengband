#include "monster/monster-util.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/wild.h"
#include "grid/grid.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags4.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "mspell/mspell-mask-definitions.h"
#include "spell/summon-types.h"
#include "system/alloc-entries.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"

typedef enum dungeon_mode_type {
    DUNGEON_MODE_AND = 1,
    DUNGEON_MODE_NAND = 2,
    DUNGEON_MODE_OR = 3,
    DUNGEON_MODE_NOR = 4,
} dungeon_mode_type;

MONSTER_IDX hack_m_idx = 0; /* Hack -- see "process_monsters()" */
MONSTER_IDX hack_m_idx_ii = 0;

/*!
 * @var chameleon_change_m_idx
 * @brief カメレオンの変身先モンスターIDを受け渡すためのグローバル変数
 * @todo 変数渡しの問題などもあるができればchameleon_change_m_idxのグローバル変数を除去し、関数引き渡しに移行すること
 */
int chameleon_change_m_idx = 0;

/*!
 * @var summon_specific_type
 * @brief 召喚条件を指定するグローバル変数 / Hack -- the "type" of the current "summon specific"
 * @todo summon_specific_typeグローバル変数の除去と関数引数への代替を行う
 */
summon_type summon_specific_type = SUMMON_NONE;

/*!
 * @brief 指定されたモンスター種族がダンジョンの制限にかかるかどうかをチェックする / Some dungeon types restrict the possible monsters.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param r_idx チェックするモンスター種族ID
 * @return 召喚条件が一致するならtrue / Return TRUE is the monster is OK and FALSE otherwise
 */
static bool restrict_monster_to_dungeon(player_type *player_ptr, MONRACE_IDX r_idx)
{
    DUNGEON_IDX d_idx = player_ptr->dungeon_idx;
    dungeon_type *d_ptr = &d_info[d_idx];
    monster_race *r_ptr = &r_info[r_idx];

    if (d_ptr->flags1 & DF1_CHAMELEON) {
        if (chameleon_change_m_idx)
            return TRUE;
    }

    if (d_ptr->flags1 & DF1_NO_MAGIC) {
        if (r_idx != MON_CHAMELEON && r_ptr->freq_spell && !(r_ptr->flags4 & RF4_NOMAGIC_MASK) && !(r_ptr->a_ability_flags1 & RF5_NOMAGIC_MASK)
            && !(r_ptr->a_ability_flags2 & RF6_NOMAGIC_MASK))
            return FALSE;
    }

    if (d_ptr->flags1 & DF1_NO_MELEE) {
        if (r_idx == MON_CHAMELEON)
            return TRUE;
        if (!(r_ptr->flags4 & (RF4_BOLT_MASK | RF4_BEAM_MASK | RF4_BALL_MASK))
            && !(r_ptr->a_ability_flags1
                & (RF5_BOLT_MASK | RF5_BEAM_MASK | RF5_BALL_MASK | RF5_CAUSE_1 | RF5_CAUSE_2 | RF5_CAUSE_3 | RF5_CAUSE_4 | RF5_MIND_BLAST | RF5_BRAIN_SMASH))
            && !(r_ptr->a_ability_flags2 & (RF6_BOLT_MASK | RF6_BEAM_MASK | RF6_BALL_MASK)))
            return FALSE;
    }

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (d_ptr->flags1 & DF1_BEGINNER) {
        if (r_ptr->level > floor_ptr->dun_level)
            return FALSE;
    }

    if (d_ptr->special_div >= 64)
        return TRUE;
    if (summon_specific_type && !(d_ptr->flags1 & DF1_CHAMELEON))
        return TRUE;

    byte a;
    switch (d_ptr->mode) {
    case DUNGEON_MODE_AND: {
        if (d_ptr->mflags1) {
            if ((d_ptr->mflags1 & r_ptr->flags1) != d_ptr->mflags1)
                return FALSE;
        }

        if (d_ptr->mflags2) {
            if ((d_ptr->mflags2 & r_ptr->flags2) != d_ptr->mflags2)
                return FALSE;
        }

        if (d_ptr->mflags3) {
            if ((d_ptr->mflags3 & r_ptr->flags3) != d_ptr->mflags3)
                return FALSE;
        }

        if (d_ptr->mflags4) {
            if ((d_ptr->mflags4 & r_ptr->flags4) != d_ptr->mflags4)
                return FALSE;
        }

        if (d_ptr->m_a_ability_flags1) {
            if ((d_ptr->m_a_ability_flags1 & r_ptr->a_ability_flags1) != d_ptr->m_a_ability_flags1)
                return FALSE;
        }

        if (d_ptr->m_a_ability_flags2) {
            if ((d_ptr->m_a_ability_flags2 & r_ptr->a_ability_flags2) != d_ptr->m_a_ability_flags2)
                return FALSE;
        }

        if (d_ptr->mflags7) {
            if ((d_ptr->mflags7 & r_ptr->flags7) != d_ptr->mflags7)
                return FALSE;
        }

        if (d_ptr->mflags8) {
            if ((d_ptr->mflags8 & r_ptr->flags8) != d_ptr->mflags8)
                return FALSE;
        }

        if (d_ptr->mflags9) {
            if ((d_ptr->mflags9 & r_ptr->flags9) != d_ptr->mflags9)
                return FALSE;
        }

        if (d_ptr->mflagsr) {
            if ((d_ptr->mflagsr & r_ptr->flagsr) != d_ptr->mflagsr)
                return FALSE;
        }

        for (a = 0; a < 5; a++)
            if (d_ptr->r_char[a] && (d_ptr->r_char[a] != r_ptr->d_char))
                return FALSE;

        return TRUE;
    }
    case DUNGEON_MODE_NAND: {
        if (d_ptr->mflags1) {
            if ((d_ptr->mflags1 & r_ptr->flags1) != d_ptr->mflags1)
                return TRUE;
        }

        if (d_ptr->mflags2) {
            if ((d_ptr->mflags2 & r_ptr->flags2) != d_ptr->mflags2)
                return TRUE;
        }

        if (d_ptr->mflags3) {
            if ((d_ptr->mflags3 & r_ptr->flags3) != d_ptr->mflags3)
                return TRUE;
        }

        if (d_ptr->mflags4) {
            if ((d_ptr->mflags4 & r_ptr->flags4) != d_ptr->mflags4)
                return TRUE;
        }

        if (d_ptr->m_a_ability_flags1) {
            if ((d_ptr->m_a_ability_flags1 & r_ptr->a_ability_flags1) != d_ptr->m_a_ability_flags1)
                return TRUE;
        }

        if (d_ptr->m_a_ability_flags2) {
            if ((d_ptr->m_a_ability_flags2 & r_ptr->a_ability_flags2) != d_ptr->m_a_ability_flags2)
                return TRUE;
        }

        if (d_ptr->mflags7) {
            if ((d_ptr->mflags7 & r_ptr->flags7) != d_ptr->mflags7)
                return TRUE;
        }

        if (d_ptr->mflags8) {
            if ((d_ptr->mflags8 & r_ptr->flags8) != d_ptr->mflags8)
                return TRUE;
        }

        if (d_ptr->mflags9) {
            if ((d_ptr->mflags9 & r_ptr->flags9) != d_ptr->mflags9)
                return TRUE;
        }

        if (d_ptr->mflagsr) {
            if ((d_ptr->mflagsr & r_ptr->flagsr) != d_ptr->mflagsr)
                return TRUE;
        }

        for (a = 0; a < 5; a++)
            if (d_ptr->r_char[a] && (d_ptr->r_char[a] != r_ptr->d_char))
                return TRUE;

        return FALSE;
    }
    case DUNGEON_MODE_OR: {
        if (r_ptr->flags1 & d_ptr->mflags1)
            return TRUE;
        if (r_ptr->flags2 & d_ptr->mflags2)
            return TRUE;
        if (r_ptr->flags3 & d_ptr->mflags3)
            return TRUE;
        if (r_ptr->flags4 & d_ptr->mflags4)
            return TRUE;
        if (r_ptr->a_ability_flags1 & d_ptr->m_a_ability_flags1)
            return TRUE;
        if (r_ptr->a_ability_flags2 & d_ptr->m_a_ability_flags2)
            return TRUE;
        if (r_ptr->flags7 & d_ptr->mflags7)
            return TRUE;
        if (r_ptr->flags8 & d_ptr->mflags8)
            return TRUE;
        if (r_ptr->flags9 & d_ptr->mflags9)
            return TRUE;
        if (r_ptr->flagsr & d_ptr->mflagsr)
            return TRUE;
        for (a = 0; a < 5; a++)
            if (d_ptr->r_char[a] == r_ptr->d_char)
                return TRUE;

        return FALSE;
    }
    case DUNGEON_MODE_NOR: {
        if (r_ptr->flags1 & d_ptr->mflags1)
            return FALSE;
        if (r_ptr->flags2 & d_ptr->mflags2)
            return FALSE;
        if (r_ptr->flags3 & d_ptr->mflags3)
            return FALSE;
        if (r_ptr->flags4 & d_ptr->mflags4)
            return FALSE;
        if (r_ptr->a_ability_flags1 & d_ptr->m_a_ability_flags1)
            return FALSE;
        if (r_ptr->a_ability_flags2 & d_ptr->m_a_ability_flags2)
            return FALSE;
        if (r_ptr->flags7 & d_ptr->mflags7)
            return FALSE;
        if (r_ptr->flags8 & d_ptr->mflags8)
            return FALSE;
        if (r_ptr->flags9 & d_ptr->mflags9)
            return FALSE;
        if (r_ptr->flagsr & d_ptr->mflagsr)
            return FALSE;
        for (a = 0; a < 5; a++)
            if (d_ptr->r_char[a] == r_ptr->d_char)
                return FALSE;

        return TRUE;
    }
    }

    return TRUE;
}

/*!
 * @brief プレイヤーの現在の広域マップ座標から得た地勢を元にモンスターの生成条件関数を返す
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 地勢にあったモンスターの生成条件関数
 */
monsterrace_hook_type get_monster_hook(player_type *player_ptr)
{
    if ((player_ptr->current_floor_ptr->dun_level > 0) || (player_ptr->current_floor_ptr->inside_quest > 0))
        return (monsterrace_hook_type)mon_hook_dungeon;

    switch (wilderness[player_ptr->wilderness_y][player_ptr->wilderness_x].terrain) {
    case TERRAIN_TOWN:
        return (monsterrace_hook_type)mon_hook_town;
    case TERRAIN_DEEP_WATER:
        return (monsterrace_hook_type)mon_hook_ocean;
    case TERRAIN_SHALLOW_WATER:
    case TERRAIN_SWAMP:
        return (monsterrace_hook_type)mon_hook_shore;
    case TERRAIN_DIRT:
    case TERRAIN_DESERT:
        return (monsterrace_hook_type)mon_hook_waste;
    case TERRAIN_GRASS:
        return (monsterrace_hook_type)mon_hook_grass;
    case TERRAIN_TREES:
        return (monsterrace_hook_type)mon_hook_wood;
    case TERRAIN_SHALLOW_LAVA:
    case TERRAIN_DEEP_LAVA:
        return (monsterrace_hook_type)mon_hook_volcano;
    case TERRAIN_MOUNTAIN:
        return (monsterrace_hook_type)mon_hook_mountain;
    default:
        return (monsterrace_hook_type)mon_hook_dungeon;
    }
}

/*!
 * @brief 指定された広域マップ座標の地勢を元にモンスターの生成条件関数を返す
 * @return 地勢にあったモンスターの生成条件関数
 */
monsterrace_hook_type get_monster_hook2(player_type *player_ptr, POSITION y, POSITION x)
{
    feature_type *f_ptr = &f_info[player_ptr->current_floor_ptr->grid_array[y][x].feat];
    if (has_flag(f_ptr->flags, FF_WATER))
        return has_flag(f_ptr->flags, FF_DEEP) ? (monsterrace_hook_type)mon_hook_deep_water : (monsterrace_hook_type)mon_hook_shallow_water;

    if (has_flag(f_ptr->flags, FF_LAVA))
        return (monsterrace_hook_type)mon_hook_lava;

    return (monsterrace_hook_type)mon_hook_floor;
}

/*!
 * @brief モンスター生成制限関数最大2つから / Apply a "monster restriction function" to the "monster allocation table"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param monster_hook 制限関数1
 * @param monster_hook2 制限関数2
 * @return エラーコード
 */
errr get_mon_num_prep(player_type *player_ptr, monsterrace_hook_type monster_hook, monsterrace_hook_type monster_hook2)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = 0; i < alloc_race_size; i++) {
        alloc_entry *entry = &alloc_race_table[i];
        entry->prob2 = 0;
        monster_race *r_ptr = &r_info[entry->index];
        if (((monster_hook != NULL) && !((*monster_hook)(player_ptr, entry->index)))
            || ((monster_hook2 != NULL) && !((*monster_hook2)(player_ptr, entry->index))))
            continue;

        if (!player_ptr->phase_out && !chameleon_change_m_idx && summon_specific_type != SUMMON_GUARDIANS) {
            if (r_ptr->flags1 & RF1_QUESTOR)
                continue;

            if (r_ptr->flags7 & RF7_GUARDIAN)
                continue;

            if (((r_ptr->flags1 & RF1_FORCE_DEPTH) != 0) && (r_ptr->level > floor_ptr->dun_level))
                continue;
        }

        entry->prob2 = entry->prob1;
        if (floor_ptr->dun_level && (!floor_ptr->inside_quest || is_fixed_quest_idx(floor_ptr->inside_quest))
            && !restrict_monster_to_dungeon(player_ptr, entry->index) && !player_ptr->phase_out) {
            int hoge = entry->prob2 * d_info[player_ptr->dungeon_idx].special_div;
            entry->prob2 = hoge / 64;
            if (randint0(64) < (hoge & 0x3f))
                entry->prob2++;
        }
    }

    return 0;
}
