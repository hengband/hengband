#include "main/scene-table-monster.h"
#include "dungeon/quest.h"
#include "main/music-definitions-table.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"

scene_type_list scene_table_monster;

using scene_monster_func = bool (*)(player_type *player_ptr, monster_type *m_ptr, scene_type *value);

static bool scene_monster(player_type *player_ptr, monster_type *m_ptr, scene_type *value)
{
    (void)player_ptr;
    if (m_ptr->mflag2.has(MFLAG2::KAGE)) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_SHADOWER;
        return true;
    } else {
        value->type = TERM_XTRA_MUSIC_MONSTER;
        value->val = m_ptr->ap_r_idx;
        return true;
    }
}

static bool scene_unique(player_type *player_ptr, monster_type *m_ptr, scene_type *value)
{
    (void)player_ptr;
    auto ap_r_ptr = &r_info[m_ptr->ap_r_idx];

    if (any_bits(ap_r_ptr->flags1, RF1_UNIQUE)) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_UNIQUE;
        return true;
    }

    return false;
}

static bool scene_unkown(player_type *player_ptr, monster_type *m_ptr, scene_type *value)
{
    (void)player_ptr;
    auto ap_r_ptr = &r_info[m_ptr->ap_r_idx];

    if (ap_r_ptr->r_tkills == 0) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_UNKNOWN_MONSTER;
        return true;
    }

    return false;
}

static bool scene_high_level(player_type *player_ptr, monster_type *m_ptr, scene_type *value)
{
    auto ap_r_ptr = &r_info[m_ptr->ap_r_idx];

    if (ap_r_ptr->r_tkills && (ap_r_ptr->level >= player_ptr->lev)) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_HIGHER_LEVEL_MONSTER;
        return true;
    }

    return false;
}

/*! モンスターBGMのフォールバック設定。
 * 先頭から適用する（先にある方を優先する）。
 */
std::vector<scene_monster_func> playfallback_monster = {
    // scene_monster : あやしい影 or モンスターID
    scene_monster,
    // scene_unique : ユニークモンスター判定
    scene_unique,
    // scene_unkown : 未知のモンスター判定
    scene_unkown,
    // scene_high_level : 高レベルのモンスター判定
    scene_high_level,
};

void refresh_monster_scene(player_type *player_ptr, const std::vector<MONSTER_IDX> &monster_list)
{
    // リスト先頭のモンスター用BGMを設定する
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[monster_list.front()];

    scene_table_monster.resize(playfallback_monster.size());
    auto ite = playfallback_monster.begin();

    for (auto &item : scene_table_monster) {
        if (!(*ite)(player_ptr, m_ptr, &item)) {
            // Note -- 特に定義を設けていないが、type = 0は無効な値とする。
            item.type = 0;
            item.val = 0;
        }
        ite++;
    }
}

scene_type_list &get_monster_scene_type_list()
{
    return scene_table_monster;
}
