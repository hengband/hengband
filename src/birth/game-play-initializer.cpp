#include "birth/game-play-initializer.h"
#include "info-reader/fixed-map-parser.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "inventory/inventory-slot-types.h"
#include "market/arena.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "object/object-kind.h"
#include "pet/pet-util.h"
#include "player/digestion-processor.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
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
    for (int i = 0; i < max_k_idx; i++) {
        object_kind *k_ptr = &k_info[i];
        k_ptr->tried = false;
        k_ptr->aware = false;
    }
}

/*!
 * @brief プレイヤー構造体の内容を初期値で消去する(名前を除く) / Clear all the global "character" data (without name)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @details 少し長いが、これ1つで処理が完結しているので分割は見送る
 */
void player_wipe_without_name(player_type *creature_ptr)
{
    player_type tmp;

#ifdef SET_UID
    int uid = creature_ptr->player_uid;
#endif
    COPY(&tmp, creature_ptr, player_type);
    if (creature_ptr->last_message)
        string_free(creature_ptr->last_message);

    if (creature_ptr->inventory_list != nullptr)
        C_KILL(creature_ptr->inventory_list, INVEN_TOTAL, object_type);

    (void)WIPE(creature_ptr, player_type);

    //TODO: キャラ作成からゲーム開始までに  current_floor_ptr を参照しなければならない処理は今後整理して外す。
    creature_ptr->current_floor_ptr = &floor_info;
    C_MAKE(creature_ptr->inventory_list, INVEN_TOTAL, object_type);
    for (int i = 0; i < 4; i++)
        strcpy(creature_ptr->history[i], "");

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

    creature_ptr->inven_cnt = 0;
    creature_ptr->equip_cnt = 0;
    for (int i = 0; i < INVEN_TOTAL; i++)
        (&creature_ptr->inventory_list[i])->wipe();

    for (int i = 0; i < max_a_idx; i++) {
        artifact_type *a_ptr = &a_info[i];
        a_ptr->cur_num = 0;
    }

    k_info_reset();
    for (int i = 1; i < max_r_idx; i++) {
        monster_race *r_ptr = &r_info[i];
        r_ptr->cur_num = 0;
        r_ptr->max_num = 100;
        if (r_ptr->flags1 & RF1_UNIQUE)
            r_ptr->max_num = 1;
        else if (r_ptr->flags7 & RF7_NAZGUL)
            r_ptr->max_num = MAX_NAZGUL_NUM;

        r_ptr->r_pkills = 0;
        r_ptr->r_akills = 0;
    }

    creature_ptr->food = PY_FOOD_FULL - 1;
    if (creature_ptr->pclass == CLASS_SORCERER) {
        creature_ptr->spell_learned1 = creature_ptr->spell_learned2 = 0xffffffffL;
        creature_ptr->spell_worked1 = creature_ptr->spell_worked2 = 0xffffffffL;
    } else {
        creature_ptr->spell_learned1 = creature_ptr->spell_learned2 = 0L;
        creature_ptr->spell_worked1 = creature_ptr->spell_worked2 = 0L;
    }

    creature_ptr->spell_forgotten1 = creature_ptr->spell_forgotten2 = 0L;
    for (int i = 0; i < 64; i++)
        creature_ptr->spell_order[i] = 99;

    creature_ptr->learned_spells = 0;
    creature_ptr->add_spells = 0;
    creature_ptr->knowledge = 0;
    creature_ptr->mutant_regenerate_mod = 100;

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

    current_world_ptr->total_winner = false;
    creature_ptr->timewalk = false;
    creature_ptr->panic_save = 0;

    current_world_ptr->noscore = 0;
    current_world_ptr->wizard = false;
    creature_ptr->wait_report_score = false;
    creature_ptr->pet_follow_distance = PET_FOLLOW_DIST;
    creature_ptr->pet_extra_flags = (PF_TELEPORT | PF_ATTACK_SPELL | PF_SUMMON_SPELL);

    for (int i = 0; i < current_world_ptr->max_d_idx; i++)
        max_dlv[i] = 0;

    creature_ptr->visit = 1;
    creature_ptr->wild_mode = false;

    for (int i = 0; i < MAX_SPELLS; i++) {
        creature_ptr->magic_num1[i] = 0;
        creature_ptr->magic_num2[i] = 0;
    }

    creature_ptr->max_plv = creature_ptr->lev = 1;
    creature_ptr->arena_number = 0;
    creature_ptr->current_floor_ptr->inside_arena = false;
    creature_ptr->current_floor_ptr->inside_quest = 0;
    for (int i = 0; i < MAX_MANE; i++) {
        creature_ptr->mane_spell[i] = RF_ABILITY::MAX;
        creature_ptr->mane_dam[i] = 0;
    }

    creature_ptr->mane_num = 0;
    creature_ptr->exit_bldg = true;
    creature_ptr->today_mon = 0;
    update_gambling_monsters(creature_ptr);
    creature_ptr->muta.clear();

    for (int i = 0; i < 8; i++)
        creature_ptr->virtues[i] = 0;

    creature_ptr->dungeon_idx = 0;
    if (vanilla_town || ironman_downward) {
        creature_ptr->recall_dungeon = DUNGEON_ANGBAND;
    } else {
        creature_ptr->recall_dungeon = DUNGEON_GALGALS;
    }

    memcpy(creature_ptr->name, tmp.name, sizeof(tmp.name));

#ifdef SET_UID
    creature_ptr->player_uid = uid;
#endif
}

/*!
 * @brief ダンジョン内部のクエストを初期化する / Initialize random quests and final quests
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
void init_dungeon_quests(player_type *creature_ptr)
{
    int number_of_quests = MAX_RANDOM_QUEST - MIN_RANDOM_QUEST + 1;
    init_flags = INIT_ASSIGN;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    floor_ptr->inside_quest = MIN_RANDOM_QUEST;
    parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
    floor_ptr->inside_quest = 0;
    for (int i = MIN_RANDOM_QUEST + number_of_quests - 1; i >= MIN_RANDOM_QUEST; i--) {
        quest_type *q_ptr = &quest[i];
        monster_race *quest_r_ptr;
        q_ptr->status = QUEST_STATUS_TAKEN;
        determine_random_questor(creature_ptr, q_ptr);
        quest_r_ptr = &r_info[q_ptr->r_idx];
        quest_r_ptr->flags1 |= RF1_QUESTOR;
        q_ptr->max_num = 1;
    }

    init_flags = INIT_ASSIGN;
    floor_ptr->inside_quest = QUEST_OBERON;
    parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
    quest[QUEST_OBERON].status = QUEST_STATUS_TAKEN;

    floor_ptr->inside_quest = QUEST_SERPENT;
    parse_fixed_map(creature_ptr, "q_info.txt", 0, 0, 0, 0);
    quest[QUEST_SERPENT].status = QUEST_STATUS_TAKEN;
    floor_ptr->inside_quest = 0;
}

/*!
 * @brief ゲームターンを初期化する / Reset turn
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @details アンデッド系種族は開始時刻を夜からにする / Undead start just sunset
 * @details
 */
void init_turn(player_type *creature_ptr)
{
    if (player_race_life(creature_ptr) == PlayerRaceLife::UNDEAD) {
        current_world_ptr->game_turn = (TURNS_PER_TICK * 3 * TOWN_DAWN) / 4 + 1;
        current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * MAX_DAYS + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    } else {
        current_world_ptr->game_turn = 1;
        current_world_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    }

    current_world_ptr->dungeon_turn = 1;
    current_world_ptr->dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
}
