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
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/digestion-processor.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "util/enum-range.h"
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
void player_wipe_without_name(PlayerType *player_ptr)
{
    auto tmp = *player_ptr;
    if (player_ptr->last_message) {
        string_free(player_ptr->last_message);
    }

    *player_ptr = {};

    // TODO: キャラ作成からゲーム開始までに  current_floor_ptr を参照しなければならない処理は今後整理して外す。
    player_ptr->current_floor_ptr = &floor_info;
    //! @todo std::make_shared の配列対応版は C++20 から
    player_ptr->inventory_list = std::shared_ptr<ObjectType[]>{ new ObjectType[INVEN_TOTAL] };
    for (int i = 0; i < 4; i++) {
        strcpy(player_ptr->history[i], "");
    }

    auto &quest_list = QuestList::get_instance();
    for (auto &[q_idx, q_ref] : quest_list) {
        q_ref.status = QuestStatusType::UNTAKEN;
        q_ref.cur_num = 0;
        q_ref.max_num = 0;
        q_ref.type = QuestKindType::NONE;
        q_ref.level = 0;
        q_ref.r_idx = MonsterRace::empty_id();
        q_ref.complev = 0;
        q_ref.comptime = 0;
    }

    player_ptr->inven_cnt = 0;
    player_ptr->equip_cnt = 0;
    for (int i = 0; i < INVEN_TOTAL; i++) {
        (&player_ptr->inventory_list[i])->wipe();
    }

    for (auto &a_ref : a_info) {
        a_ref.is_generated = false;
    }

    k_info_reset();
    for (auto &[r_idx, r_ref] : r_info) {
        if (!MonsterRace(r_ref.idx).is_valid()) {
            continue;
        }
        r_ref.cur_num = 0;
        r_ref.max_num = 100;
        if (r_ref.kind_flags.has(MonsterKindType::UNIQUE)) {
            r_ref.max_num = 1;
        } else if (r_ref.population_flags.has(MonsterPopulationType::NAZGUL)) {
            r_ref.max_num = MAX_NAZGUL_NUM;
        }

        r_ref.r_pkills = 0;
        r_ref.r_akills = 0;
    }

    player_ptr->food = PY_FOOD_FULL - 1;
    if (PlayerClass(player_ptr).equals(PlayerClassType::SORCERER)) {
        player_ptr->spell_learned1 = player_ptr->spell_learned2 = 0xffffffffL;
        player_ptr->spell_worked1 = player_ptr->spell_worked2 = 0xffffffffL;
    } else {
        player_ptr->spell_learned1 = player_ptr->spell_learned2 = 0L;
        player_ptr->spell_worked1 = player_ptr->spell_worked2 = 0L;
    }

    player_ptr->spell_forgotten1 = player_ptr->spell_forgotten2 = 0L;
    for (int i = 0; i < 64; i++) {
        player_ptr->spell_order[i] = 99;
    }

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

    for (const auto &d_ref : d_info) {
        max_dlv[d_ref.idx] = 0;
    }

    player_ptr->visit = 1;
    player_ptr->wild_mode = false;

    player_ptr->max_plv = player_ptr->lev = 1;
    player_ptr->arena_number = 0;
    player_ptr->current_floor_ptr->inside_arena = false;
    player_ptr->current_floor_ptr->quest_number = QuestId::NONE;

    player_ptr->exit_bldg = true;
    player_ptr->knows_daily_bounty = false;
    update_gambling_monsters(player_ptr);
    player_ptr->muta.clear();

    for (int i = 0; i < 8; i++) {
        player_ptr->virtues[i] = 0;
    }

    player_ptr->dungeon_idx = 0;
    if (vanilla_town || ironman_downward) {
        player_ptr->recall_dungeon = DUNGEON_ANGBAND;
    } else {
        player_ptr->recall_dungeon = DUNGEON_GALGALS;
    }

    memcpy(player_ptr->name, tmp.name, sizeof(tmp.name));

#ifdef SET_UID
    player_ptr->player_uid = tmp.player_uid;
#ifdef SAFE_SETUID
#ifdef SAFE_SETUID_POSIX
    player_ptr->player_euid = tmp.player_euid;
    player_ptr->player_egid = tmp.player_egid;
#endif
#endif
#endif
}

/*!
 * @brief ダンジョン内部のクエストを初期化する / Initialize random quests and final quests
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void init_dungeon_quests(PlayerType *player_ptr)
{
    init_flags = INIT_ASSIGN;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto &quest_list = QuestList::get_instance();
    floor_ptr->quest_number = QuestId::RANDOM_QUEST1;
    parse_fixed_map(player_ptr, "q_info.txt", 0, 0, 0, 0);
    floor_ptr->quest_number = QuestId::NONE;
    for (auto q_idx : EnumRange(QuestId::RANDOM_QUEST1, QuestId::RANDOM_QUEST10)) {
        auto *q_ptr = &quest_list[q_idx];
        monster_race *quest_r_ptr;
        q_ptr->status = QuestStatusType::TAKEN;
        determine_random_questor(player_ptr, q_ptr);
        quest_r_ptr = &r_info[q_ptr->r_idx];
        quest_r_ptr->flags1 |= RF1_QUESTOR;
        q_ptr->max_num = 1;
    }

    init_flags = INIT_ASSIGN;
    floor_ptr->quest_number = QuestId::OBERON;
    parse_fixed_map(player_ptr, "q_info.txt", 0, 0, 0, 0);
    quest_list[QuestId::OBERON].status = QuestStatusType::TAKEN;

    floor_ptr->quest_number = QuestId::SERPENT;
    parse_fixed_map(player_ptr, "q_info.txt", 0, 0, 0, 0);
    quest_list[QuestId::SERPENT].status = QuestStatusType::TAKEN;
    floor_ptr->quest_number = QuestId::NONE;
}

/*!
 * @brief ゲームターンを初期化する / Reset turn
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details アンデッド系種族は開始時刻を夜からにする / Undead start just sunset
 * @details
 */
void init_turn(PlayerType *player_ptr)
{
    if (PlayerRace(player_ptr).life() == PlayerRaceLifeType::UNDEAD) {
        w_ptr->game_turn = (TURNS_PER_TICK * 3 * TOWN_DAWN) / 4 + 1;
        w_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * MAX_DAYS + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    } else {
        w_ptr->game_turn = 1;
        w_ptr->game_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
    }

    w_ptr->dungeon_turn = 1;
    w_ptr->dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
}
