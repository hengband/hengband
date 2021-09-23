#include "birth/game-play-initializer.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "info-reader/fixed-map-parser.h"
#include "inventory/inventory-slot-types.h"
#include "market/arena.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "object/object-kind.h"
#include "pet/pet-util.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/digestion-processor.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "world/world.h"

/*!
 * @brief ベースアイテム構造体の鑑定済みフラグをリセットする。
 * @details
 * 不具合対策で0からリセットする(セーブは0から)
 */
static void k_info_reset(void)
{
    for (auto &k_ref : k_info) {
        k_ref.tried = false;
        k_ref.aware = false;
    }
}

/*!
 * @brief プレイヤー構造体の内容を初期値で消去する(名前を除く) / Clear all the global "character" data (without name)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 少し長いが、これ1つで処理が完結しているので分割は見送る
 */
void player_wipe_without_name(player_type *player_ptr)
{
#ifdef SET_UID
    int uid = player_ptr->player_uid;
#endif
    auto tmp = *player_ptr;
    if (player_ptr->last_message)
        string_free(player_ptr->last_message);

    *player_ptr = {};

    // TODO: キャラ作成からゲーム開始までに  current_floor_ptr を参照しなければならない処理は今後整理して外す。
    player_ptr->current_floor_ptr = &floor_info;
    //! @todo std::make_shared の配列対応版は C++20 から
    player_ptr->inventory_list = std::shared_ptr<object_type[]>{ new object_type[INVEN_TOTAL] };
    for (int i = 0; i < 4; i++)
        strcpy(player_ptr->history[i], "");

    for (int i = 0; i < max_q_idx; i++) {
        quest_type *const q_ptr = &quest[i];
        q_ptr->status = QUEST_STATUS_UNTAKEN;
        q_ptr->cur_num = 0;
        q_ptr->max_num = 0;
        q_ptr->type = 0;
        q_ptr->level = 0;
        q_ptr->r_idx = 0;
        q_ptr->complev = 0;
        q_ptr->comptime = 0;
    }

    player_ptr->inven_cnt = 0;
    player_ptr->equip_cnt = 0;
    for (int i = 0; i < INVEN_TOTAL; i++)
        (&player_ptr->inventory_list[i])->wipe();

    for (auto &a_ref : a_info) {
        a_ref.cur_num = 0;
    }

    k_info_reset();
    for (auto &r_ref : r_info) {
        if (r_ref.idx == 0) {
            continue;
        }
        r_ref.cur_num = 0;
        r_ref.max_num = 100;
        if (r_ref.flags1 & RF1_UNIQUE)
            r_ref.max_num = 1;
        else if (r_ref.flags7 & RF7_NAZGUL)
            r_ref.max_num = MAX_NAZGUL_NUM;

        r_ref.r_pkills = 0;
        r_ref.r_akills = 0;
    }

    player_ptr->food = PY_FOOD_FULL - 1;
    if (player_ptr->pclass == CLASS_SORCERER) {
        player_ptr->spell_learned1 = player_ptr->spell_learned2 = 0xffffffffL;
        player_ptr->spell_worked1 = player_ptr->spell_worked2 = 0xffffffffL;
    } else {
        player_ptr->spell_learned1 = player_ptr->spell_learned2 = 0L;
        player_ptr->spell_worked1 = player_ptr->spell_worked2 = 0L;
    }

    player_ptr->spell_forgotten1 = player_ptr->spell_forgotten2 = 0L;
    for (int i = 0; i < 64; i++)
        player_ptr->spell_order[i] = 99;

    player_ptr->learned_spells = 0;
    player_ptr->add_spells = 0;
    player_ptr->knowledge = 0;
    player_ptr->mutant_regenerate_mod = 100;

    cheat_peek = false;
    cheat_hear = false;
    cheat_room = false;
    cheat_xtra = false;
    cheat_know = false;
    cheat_live = false;
    cheat_save = false;
    cheat_diary_output = false;
    cheat_turn = false;
    cheat_immortal = false;

    w_ptr->total_winner = false;
    player_ptr->timewalk = false;
    player_ptr->panic_save = 0;

    w_ptr->noscore = 0;
    w_ptr->wizard = false;
    player_ptr->wait_report_score = false;
    player_ptr->pet_follow_distance = PET_FOLLOW_DIST;
    player_ptr->pet_extra_flags = (PF_TELEPORT | PF_ATTACK_SPELL | PF_SUMMON_SPELL);

    for (const auto &d_ref : d_info)
        max_dlv[d_ref.idx] = 0;

    player_ptr->visit = 1;
    player_ptr->wild_mode = false;

    for (int i = 0; i < MAX_SPELLS; i++) {
        player_ptr->magic_num1[i] = 0;
        player_ptr->magic_num2[i] = 0;
    }

    player_ptr->max_plv = player_ptr->lev = 1;
    player_ptr->arena_number = 0;
    player_ptr->current_floor_ptr->inside_arena = false;
    player_ptr->current_floor_ptr->inside_quest = 0;
    for (int i = 0; i < MAX_MANE; i++) {
        player_ptr->mane_spell[i] = RF_ABILITY::MAX;
        player_ptr->mane_dam[i] = 0;
    }

    player_ptr->mane_num = 0;
    player_ptr->exit_bldg = true;
    player_ptr->today_mon = 0;
    update_gambling_monsters(player_ptr);
    player_ptr->muta.clear();

    for (int i = 0; i < 8; i++)
        player_ptr->virtues[i] = 0;

    player_ptr->dungeon_idx = 0;
    if (vanilla_town || ironman_downward) {
        player_ptr->recall_dungeon = DUNGEON_ANGBAND;
    } else {
        player_ptr->recall_dungeon = DUNGEON_GALGALS;
    }

    memcpy(player_ptr->name, tmp.name, sizeof(tmp.name));

#ifdef SET_UID
    player_ptr->player_uid = uid;
#endif
}

/*!
 * @brief ダンジョン内部のクエストを初期化する / Initialize random quests and final quests
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void init_dungeon_quests(player_type *player_ptr)
{
    int number_of_quests = MAX_RANDOM_QUEST - MIN_RANDOM_QUEST + 1;
    init_flags = INIT_ASSIGN;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->inside_quest = MIN_RANDOM_QUEST;
    parse_fixed_map(player_ptr, "q_info.txt", 0, 0, 0, 0);
    floor_ptr->inside_quest = 0;
    for (int i = MIN_RANDOM_QUEST + number_of_quests - 1; i >= MIN_RANDOM_QUEST; i--) {
        quest_type *q_ptr = &quest[i];
        monster_race *quest_r_ptr;
        q_ptr->status = QUEST_STATUS_TAKEN;
        determine_random_questor(player_ptr, q_ptr);
        quest_r_ptr = &r_info[q_ptr->r_idx];
        quest_r_ptr->flags1 |= RF1_QUESTOR;
        q_ptr->max_num = 1;
    }

    init_flags = INIT_ASSIGN;
    floor_ptr->inside_quest = QUEST_OBERON;
    parse_fixed_map(player_ptr, "q_info.txt", 0, 0, 0, 0);
    quest[QUEST_OBERON].status = QUEST_STATUS_TAKEN;

    floor_ptr->inside_quest = QUEST_SERPENT;
    parse_fixed_map(player_ptr, "q_info.txt", 0, 0, 0, 0);
    quest[QUEST_SERPENT].status = QUEST_STATUS_TAKEN;
    floor_ptr->inside_quest = 0;
}

/*!
 * @brief ゲームターンを初期化する / Reset turn
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details アンデッド系種族は開始時刻を夜からにする / Undead start just sunset
 * @details
 */
void init_turn(player_type *player_ptr)
{
    if (player_race_life(player_ptr) == PlayerRaceLife::UNDEAD) {
        w_ptr->game_turn = (TURNS_PER_TICK * 3 * TOWN_DAWN) / 4 + 1;
        w_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * MAX_DAYS + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    } else {
        w_ptr->game_turn = 1;
        w_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    }

    w_ptr->dungeon_turn = 1;
    w_ptr->dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
}
