﻿#include "main/scene-table-floor.h"
#include "dungeon/quest.h"
#include "main/music-definitions-table.h"
#include "system/floor-type-definition.h"

scene_type_list scene_table_floor;

static bool scene_basic(player_type *player_ptr, scene_type *value)
{
    if (player_ptr->ambush_flag) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_AMBUSH;
        return true;
    }

    if (player_ptr->wild_mode) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_WILD;
        return true;
    }

    if (player_ptr->current_floor_ptr->inside_arena) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_ARENA;
        return true;
    }

    if (player_ptr->phase_out) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_BATTLE;
        return true;
    }

    return false;
}

static bool scene_quest(player_type *player_ptr, scene_type *value)
{
    const QUEST_IDX quest_id = quest_number(player_ptr, player_ptr->current_floor_ptr->dun_level);
    const bool enable = (quest_id > 0);
    if (enable) {
        value->type = TERM_XTRA_MUSIC_QUEST;
        value->val = quest_id;
    }

    return enable;
}

static bool scene_quest_basic(player_type *player_ptr, scene_type *value)
{
    const QUEST_IDX quest_id = quest_number(player_ptr, player_ptr->current_floor_ptr->dun_level);
    const bool enable = (quest_id > 0);
    if (enable) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_QUEST;
    }

    return enable;
}

static bool scene_town(player_type *player_ptr, scene_type *value)
{
    const bool enable = !is_in_dungeon(player_ptr) && (player_ptr->town_num > 0);
    if (enable) {
        value->type = TERM_XTRA_MUSIC_TOWN;
        value->val = player_ptr->town_num;
    }
    return enable;
}

static bool scene_town_basic(player_type *player_ptr, scene_type *value)
{
    const bool enable = !is_in_dungeon(player_ptr) && (player_ptr->town_num > 0);
    if (enable) {
        value->type = TERM_XTRA_MUSIC_BASIC;
        value->val = MUSIC_BASIC_TOWN;
    }
    return enable;
}

static bool scene_field(player_type *player_ptr, scene_type *value)
{
    const bool enable = !is_in_dungeon(player_ptr);
    if (enable) {
        value->type = TERM_XTRA_MUSIC_BASIC;

        if (player_ptr->lev >= 45)
            value->val = MUSIC_BASIC_FIELD3;
        else if (player_ptr->lev >= 25)
            value->val = MUSIC_BASIC_FIELD2;
        else
            value->val = MUSIC_BASIC_FIELD1;
    }
    return enable;
}

static bool scene_dungeon_feeling(player_type *player_ptr, scene_type *value)
{
    const bool enable = (player_ptr->feeling >= 2) && (player_ptr->feeling <= 5);
    if (enable) {
        value->type = TERM_XTRA_MUSIC_BASIC;

        if (player_ptr->feeling == 2)
            value->val = MUSIC_BASIC_DUN_FEEL2;
        else
            value->val = MUSIC_BASIC_DUN_FEEL1;
    }
    return enable;
}

static bool scene_dungeon(player_type *player_ptr, scene_type *value)
{
    const bool enable = (player_ptr->dungeon_idx > 0);
    if (enable) {
        value->type = TERM_XTRA_MUSIC_DUNGEON;
        value->val = player_ptr->dungeon_idx;
    }
    return enable;
}

static bool scene_dungeon_basic(player_type *player_ptr, scene_type *value)
{
    const bool enable = is_in_dungeon(player_ptr);
    if (enable) {
        value->type = TERM_XTRA_MUSIC_BASIC;

        const auto dun_level = player_ptr->current_floor_ptr->dun_level;
        if (dun_level >= 80)
            value->val = MUSIC_BASIC_DUN_HIGH;
        else if (dun_level >= 40)
            value->val = MUSIC_BASIC_DUN_MED;
        else
            value->val = MUSIC_BASIC_DUN_LOW;
    }
    return enable;
}

static bool scene_mute(player_type *player_ptr, scene_type *value)
{
    (void)player_ptr;
    value->type = TERM_XTRA_MUSIC_MUTE;
    value->val = 0;
    return true;
}

using scene_feel_func = bool (*)(player_type *player_ptr, scene_type *value);
/*! シチュエーション選択のフォールバック設定。
 * 先頭から適用する（先にある方を優先する）。
 */
std::vector<scene_feel_func> playfallback_floor = {
    // scene_basic : ambush, wild, arena, battleの判定
    scene_basic,
    // scene_quest : questの判定（クエストの個別BGMを指定）
    scene_quest,
    // scene_quest_basic : questの判定 (Basicのquest指定)
    scene_quest_basic,
    // scene_town : townの判定（町の個別BGMを指定）
    scene_town,
    // scene_town_basic : townの判定 (Basicのtown指定)
    scene_town_basic,
    // scene_field : field1/2/3の判定（プレイヤーレベル25未満、25以上45未満、45以上時の荒野）
    scene_field,
    // scene_dungeon_feeling : feed1/2の判定（ダンジョンの雰囲気が「悪い予感」～「とても危険」、「死の幻」）
    scene_dungeon_feeling,
    // scene_dungeon : dungeonの判定（ダンジョンの個別BGMを指定）
    scene_dungeon,
    // scene_dungeon_basic : dun_low/med/highの判定（ダンジョンレベルが40未満、40以上80未満、80以上）
    scene_dungeon_basic,
    // scene_mute : 最後にミュートを配置する
    scene_mute
};

void refresh_floor_scene(player_type *player_ptr)
{
    scene_table_floor.resize(playfallback_floor.size());
    auto ite = playfallback_floor.begin();

    for (auto &item : scene_table_floor) {
        if (!(*ite)(player_ptr, &item)) {
            // Note -- 特に定義を設けていないが、type = 0は無効な値とする。
            item.type = 0;
            item.val = 0;
        }
        ite++;
    }
}

scene_type_list &get_floor_scene_type_list()
{
    return scene_table_floor;
}
