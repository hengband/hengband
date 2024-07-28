#include "birth/game-play-initializer.h"
#include "dungeon/quest.h"
#include "floor/floor-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "info-reader/fixed-map-parser.h"
#include "inventory/inventory-slot-types.h"
#include "market/arena-entry.h"
#include "market/arena.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/digestion-processor.h"
#include "player/player-spell-status.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem-info.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/inner-game-data.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "util/enum-range.h"
#include "util/string-processor.h"
#include "world/world.h"
#include <algorithm>
#include <string>

/*!
 * @brief プレイヤー構造体の内容を初期値で消去する(名前を除く) / Clear all the global "character" data (without name)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 少し長いが、これ1つで処理が完結しているので分割は見送る
 */
void player_wipe_without_name(PlayerType *player_ptr)
{
    const std::string backup_name = player_ptr->name;
    *player_ptr = {};

    // TODO: キャラ作成からゲーム開始までに  current_floor_ptr を参照しなければならない処理は今後整理して外す。
    player_ptr->current_floor_ptr = &floor_info;
    //! @todo std::make_shared の配列対応版は C++20 から
    player_ptr->inventory_list = std::shared_ptr<ItemEntity[]>{ new ItemEntity[INVEN_TOTAL] };
    for (int i = 0; i < 4; i++) {
        player_ptr->history[i][0] = '\0';
    }

    auto &quests = QuestList::get_instance();
    for (auto &[quest_id, quest] : quests) {
        quest.status = QuestStatusType::UNTAKEN;
        quest.cur_num = 0;
        quest.max_num = 0;
        quest.type = QuestKindType::NONE;
        quest.level = 0;
        quest.r_idx = MonraceList::empty_id();
        quest.complev = 0;
        quest.comptime = 0;
    }

    player_ptr->inven_cnt = 0;
    player_ptr->equip_cnt = 0;
    for (int i = 0; i < INVEN_TOTAL; i++) {
        (&player_ptr->inventory_list[i])->wipe();
    }

    ArtifactList::get_instance().reset_generated_flags();
    BaseitemList::get_instance().reset_identification_flags();
    for (auto &[_, monrace] : monraces_info) {
        if (!monrace.is_valid()) {
            continue;
        }
        monrace.cur_num = 0;
        monrace.max_num = MAX_MONSTER_NUM;
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            monrace.max_num = MAX_UNIQUE_NUM;
        } else if (monrace.population_flags.has(MonsterPopulationType::NAZGUL)) {
            monrace.max_num = MAX_NAZGUL_NUM;
        } else if (monrace.population_flags.has(MonsterPopulationType::BUNBUN_STRIKER)) {
            monrace.max_num = MAX_BUNBUN_NUM;
        }

        monrace.r_pkills = 0;
        monrace.r_akills = 0;
    }

    player_ptr->food = PY_FOOD_FULL - 1;

    PlayerSpellStatus pss(player_ptr);
    pss.realm1().initialize();
    pss.realm2().initialize();

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

    auto &world = AngbandWorld::get_instance();
    world.total_winner = false;
    player_ptr->timewalk = false;
    player_ptr->panic_save = 0;

    world.noscore = 0;
    world.wizard = false;
    player_ptr->wait_report_score = false;
    player_ptr->pet_follow_distance = PET_FOLLOW_DIST;
    player_ptr->pet_extra_flags = (PF_TELEPORT | PF_ATTACK_SPELL | PF_SUMMON_SPELL);

    for (const auto &d_ref : dungeons_info) {
        max_dlv[d_ref.idx] = 0;
    }

    player_ptr->visit = 1;
    world.set_wild_mode(false);

    player_ptr->max_plv = player_ptr->lev = 1;
    ArenaEntryList::get_instance().reset_entry();
    world.set_arena(true);
    world.knows_daily_bounty = false;
    update_gambling_monsters(player_ptr);
    player_ptr->muta.clear();

    for (int i = 0; i < 8; i++) {
        player_ptr->virtues[i] = 0;
    }

    if (vanilla_town || ironman_downward) {
        player_ptr->recall_dungeon = DUNGEON_ANGBAND;
    } else {
        player_ptr->recall_dungeon = DUNGEON_GALGALS;
    }

    std::copy_n(backup_name.begin(), backup_name.length(), player_ptr->name);
}

/*!
 * @brief ダンジョン内部のクエストを初期化する / Initialize random quests and final quests
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void init_dungeon_quests(PlayerType *player_ptr)
{
    init_flags = INIT_ASSIGN;
    auto &floor = *player_ptr->current_floor_ptr;
    auto &quests = QuestList::get_instance();
    floor.quest_number = QuestId::RANDOM_QUEST1;
    parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
    floor.quest_number = QuestId::NONE;
    for (auto quest_id : RANDOM_QUEST_ID_RANGE) {
        auto &quest = quests.get_quest(quest_id);
        quest.status = QuestStatusType::TAKEN;
        determine_random_questor(player_ptr, quest);
        auto &monrace = quest.get_bounty();
        monrace.misc_flags.set(MonsterMiscType::QUESTOR);
        quest.max_num = 1;
    }

    init_flags = INIT_ASSIGN;
    floor.quest_number = QuestId::OBERON;
    parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
    quests.get_quest(QuestId::OBERON).status = QuestStatusType::TAKEN;

    floor.quest_number = QuestId::SERPENT;
    parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
    quests.get_quest(QuestId::SERPENT).status = QuestStatusType::TAKEN;
    floor.quest_number = QuestId::NONE;
}

/*!
 * @brief ゲームターンを初期化する / Reset turn
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details アンデッド系種族は開始時刻を夜からにする / Undead start just sunset
 * @details
 */
void init_turn(PlayerType *player_ptr)
{
    auto &world = AngbandWorld::get_instance();
    if (PlayerRace(player_ptr).life() == PlayerRaceLifeType::UNDEAD) {
        world.game_turn = (TURNS_PER_TICK * 3 * TOWN_DAWN) / 4 + 1;
    } else {
        world.game_turn = 1;
    }

    InnerGameData::get_instance().init_turn_limit();
    world.dungeon_turn = 1;
    world.dungeon_turn_limit = TURNS_PER_TICK * TOWN_DAWN * (MAX_DAYS - 1) + TURNS_PER_TICK * TOWN_DAWN * 3 / 4;
}
