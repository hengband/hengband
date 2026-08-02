#include "dungeon/quest.h"
#include "artifact/fixed-art-types.h"
#include "core/asking-player.h"
#include "floor/floor-mode-changer.h"
#include "game-option/play-record-options.h"
#include "io/write-diary.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "player-status/player-energy.h"
#include "player/player-status.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/dungeon/quest-list.h"
#include "system/floor/floor-info.h" // @todo 相互参照、将来的に削除する.
#include "system/grid-type-definition.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "view/display-messages.h"
#include "world/world.h"
#ifdef JP
#else
#include "locale/english.h"
#endif

/*!
 * @brief クエスト突入時のメッセージテーブル
 */
namespace {
const std::vector<std::string> quest_entered_messages = {
    _("床にメッセージが刻まれている:", "You find the following inscription in the floor"),
    _("壁にメッセージが刻まれている:", "You see a message inscribed in the wall"),
    _("メッセージを見つけた:", "There is a sign saying"),
    _("何かが階段の上に書いてある:", "Something is written on the staircase"),
    _("巻物を見つけた。メッセージが書いてある:", "You find a scroll with the following message"),
};
}

/*!
 * @brief JSONC 化済み固定クエストの説明文を quest_text_lines へ収集する
 * @param quest_id 対象クエストID
 * @return JSONC のレイアウトから説明文を設定した場合 true、未変換で従来経路に委ねる場合 false
 * @details 旧 ?:[LEQ/EQU $QUESTnn ...] / [EQU $QUEST_TYPEnn ...] をクエストの現在状態で再現する。
 * 条件を満たすブロックの行を上限まで追加する (通常はステータスで排他となり1ブロックのみ該当)。
 */
bool populate_quest_text_lines(QuestId quest_id)
{
    // 旧経路と同じく、対象クエストにレイアウトが無い場合でも表示バッファは必ず空にする
    // (残すと直前に表示したクエストの説明文が別クエストに紛れて表示され得る)。
    quest_text_lines.clear();
    const auto fixed_map = QuestFixedMapList::get_instance().find(quest_id);
    if (!fixed_map) {
        return false;
    }

    // テキスト表示のみ: 静的メタデータは再適用しない (実行時に変化した type 等を壊さないため。
    // メタデータの確立は受託時(get_questinfo do_init)・フロア生成・reset_all・ロードで行う)
    const auto &quest = QuestList::get_instance().get_quest(quest_id);
    for (const auto &block : fixed_map->descriptions) {
        if (block.status_at_most && (enum2i(quest.status) > enum2i(*block.status_at_most))) {
            continue;
        }
        if (block.status_equals && (quest.status != *block.status_equals)) {
            continue;
        }
        if (block.quest_type && (quest.type != *block.quest_type)) {
            continue;
        }

        const auto &lines = _(block.lines_ja, block.lines_en);
        for (const auto &line : lines) {
            if (std::ssize(quest_text_lines) >= QUEST_TEST_LINES_MAX) {
                return true;
            }
            quest_text_lines.push_back(line);
        }
    }

    return true;
}

/*!
 * @brief JSONC 化済みクエストの静的メタデータを QuestType へ適用する (旧 INIT_ASSIGN 相当)
 * @param quest_id 対象クエストID
 */
void assign_json_quest_metadata(QuestId quest_id)
{
    const auto fixed_map = QuestFixedMapList::get_instance().find(quest_id);
    if (fixed_map) {
        auto &quest = QuestList::get_instance().get_quest(quest_id);
        // QUESTOR 付与を含む静的メタデータを確立し、報酬の確定・アーティファクト予約は受託時のみ行う。
        apply_quest_metadata(*fixed_map, quest);
        resolve_quest_reward(*fixed_map, quest);
    }
}

/*!
 * @brief ランダムクエストの討伐ユニークを決める / Determine the random quest uniques
 * @param quest クエスト構造体への参照
 */
void determine_random_questor(PlayerType *player_ptr, QuestType &quest)
{
    get_mon_num_prep_enum(player_ptr, MonraceHook::QUEST);
    const auto &monraces = MonraceList::get_instance();
    MonraceId r_idx;
    while (true) {
        r_idx = get_mon_num(player_ptr, 0, quest.level + 5 + randint1(quest.level / 10), PM_ARENA);
        if (monraces.can_unify_separate(r_idx)) {
            continue;
        }

        const auto &monrace = monraces.get_monrace(r_idx);
        if (monrace.level > (quest.level + (quest.level / 20))) {
            break;
        }
    }

    quest.r_idx = r_idx;
}

/*!
 * @brief クエストの最終状態を記録する(成功or失敗、時間)
 * @param PlayerType プレイヤー情報への参照ポインタ
 * @param q_ptr クエスト情報への参照ポインタ
 * @param stat ステータス(成功or失敗)
 */
void record_quest_final_status(QuestType *q_ptr, short lev, QuestStatusType stat)
{
    q_ptr->status = stat;
    q_ptr->complev = lev;
    auto &world = AngbandWorld::get_instance();
    world.play_time.update();
    q_ptr->comptime = world.play_time.elapsed_sec();
}

/*!
 * @brief クエストを達成状態にする /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param quest_id 達成状態にしたいクエストのID
 */
void complete_quest(PlayerType *player_ptr, QuestId quest_id)
{
    auto &quests = QuestList::get_instance();
    auto &quest = quests.get_quest(quest_id);
    switch (quest.type) {
    case QuestKindType::RANDOM:
        if (record_rand_quest) {
            exe_write_diary_quest(player_ptr, DiaryKind::RAND_QUEST_C, quest_id);
        }
        break;
    default:
        if (record_fix_quest) {
            exe_write_diary_quest(player_ptr, DiaryKind::FIX_QUEST_C, quest_id);
        }
        break;
    }

    record_quest_final_status(&quest, player_ptr->lev, QuestStatusType::COMPLETED);
    if (quest.flags & QUEST_FLAG_SILENT) {
        return;
    }

    play_music(TERM_XTRA_MUSIC_BASIC, MUSIC_BASIC_QUEST_CLEAR);
    msg_print(_("クエストを達成した！", "You just completed your quest!"));
    msg_erase();
}

/*!
 * @brief 特定のアーティファクトを入手した際のクエスト達成処理 /
 * Check for "Quest" completion when a quest monster is killed or charmed.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 入手したオブジェクトの構造体参照ポインタ
 */
void check_find_art_quest_completion(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    const auto &quests = QuestList::get_instance();
    /* Check if completed a quest */
    for (const auto &[quest_id, quest] : quests) {
        auto found_artifact = (quest.type == QuestKindType::FIND_ARTIFACT);
        found_artifact &= (quest.status == QuestStatusType::TAKEN);
        found_artifact &= quest.get_reward().map_or([o_ptr](FixedArtifactId fa_id) { return o_ptr->is_specific_artifact(fa_id); }, false);
        if (found_artifact) {
            complete_quest(player_ptr, quest_id);
        }
    }
}

/*!
 * @brief クエストの導入メッセージを表示する
 * @param quest_id 開始されたクエストのID
 */
void quest_discovery(QuestId quest_id)
{
    auto &quests = QuestList::get_instance();
    auto &quest = quests.get_quest(quest_id);
    const auto &monrace = quest.get_bounty();
    if (!inside_quest(quest_id)) {
        return;
    }

    const auto num_subjugation = quest.max_num;
#ifdef JP
    const auto &name = monrace.name;
#else
    const auto &name = (num_subjugation != 1) ? pluralize(monrace.name) : monrace.name.string();
#endif

    msg_print(rand_choice(quest_entered_messages));
    msg_erase();
    if (num_subjugation != 1) {
        msg_format(_("注意しろ！この階は%d体の%sによって守られている！", "Be warned, this level is guarded by %d %s!"), num_subjugation, name.data());
        return;
    }

    if (!monrace.is_dead_unique()) {
        msg_format(_("注意せよ！この階は%sによって守られている！", "Beware, this level is protected by %s!"), name.data());
        return;
    }

    msg_print(_("この階は以前は誰かによって守られていたようだ…。", "It seems that this level was protected by someone before..."));
    record_quest_final_status(&quest, 0, QuestStatusType::FINISHED);
}

/*!
 * @brief クエスト階層から離脱する際の処理
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void leave_quest_check(PlayerType *player_ptr)
{
    leaving_quest = player_ptr->current_floor_ptr->quest_number;
    if (!inside_quest(leaving_quest)) {
        return;
    }

    auto &quests = QuestList::get_instance();
    auto &quest = quests.get_quest(leaving_quest);
    auto is_one_time_quest = ((quest.flags & QUEST_FLAG_ONCE) || (quest.type == QuestKindType::RANDOM)) && (quest.status == QuestStatusType::TAKEN);
    if (!is_one_time_quest) {
        return;
    }

    record_quest_final_status(&quest, player_ptr->lev, QuestStatusType::FAILED);

    /* Additional settings */
    switch (quest.type) {
    case QuestKindType::TOWER:
        quests.get_quest(QuestId::TOWER1).status = QuestStatusType::FAILED;
        quests.get_quest(QuestId::TOWER1).complev = player_ptr->lev;
        break;
    case QuestKindType::FIND_ARTIFACT:
        quest.reset_reward();
        break;
    case QuestKindType::RANDOM:
        quest.get_bounty().misc_flags.reset(MonsterMiscType::QUESTOR);
        FloorChangeModesStore::get_instace()->set(FloorChangeMode::NO_RETURN);
        break;
    default:
        break;
    }

    /* Record finishing a quest */
    if (quest.type == QuestKindType::RANDOM) {
        if (record_rand_quest) {
            exe_write_diary_quest(player_ptr, DiaryKind::RAND_QUEST_F, leaving_quest);
        }
        return;
    }

    if (record_fix_quest) {
        exe_write_diary_quest(player_ptr, DiaryKind::FIX_QUEST_F, leaving_quest);
    }
}

/*!
 * @brief 「塔」クエストの各階層から離脱する際の処理
 */
void leave_tower_check(PlayerType *player_ptr)
{
    auto &quests = QuestList::get_instance();
    leaving_quest = player_ptr->current_floor_ptr->quest_number;

    auto &tower1 = quests.get_quest(QuestId::TOWER1);
    auto is_leaving_from_tower = inside_quest(leaving_quest);
    is_leaving_from_tower &= quests.get_quest(leaving_quest).type == QuestKindType::TOWER;
    is_leaving_from_tower &= tower1.status != QuestStatusType::COMPLETED;
    if (!is_leaving_from_tower) {
        return;
    }
    if (quests.get_quest(leaving_quest).type != QuestKindType::TOWER) {
        return;
    }
    tower1.status = QuestStatusType::FAILED;
    tower1.complev = player_ptr->lev;
    auto &world = AngbandWorld::get_instance();
    world.play_time.update();
    tower1.comptime = world.play_time.elapsed_sec();
}

/*!
 * @brief Player enters a new quest
 */
void exe_enter_quest(PlayerType *player_ptr, QuestId quest_id)
{
    const auto &quests = QuestList::get_instance();
    if (quests.get_quest(quest_id).type != QuestKindType::RANDOM) {
        player_ptr->current_floor_ptr->dun_level = 1;
    }
    player_ptr->current_floor_ptr->quest_number = quest_id;
    player_ptr->leaving = true;
}

/*!
 * @brief クエスト入り口にプレイヤーが乗った際の処理 / Do building commands
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_quest(PlayerType *player_ptr)
{
    if (AngbandWorld::get_instance().is_wild_mode()) {
        return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    const auto &floor = *player_ptr->current_floor_ptr;
    if (!floor.has_terrain_characteristics(player_ptr->get_position(), TerrainCharacteristics::QUEST_ENTER)) {
        msg_print(_("ここにはクエストの入口はない。", "You see no quest level here."));
        return;
    }

    msg_print(_("ここにはクエストへの入口があります。", "There is an entry of a quest."));
    if (!input_check(_("クエストに入りますか？", "Do you enter? "))) {
        return;
    }
    if (is_echizen(player_ptr)) {
        msg_print(_("『とにかく入ってみようぜぇ。』", "\"Let's go in anyway.\""));
    } else if (is_chargeman(player_ptr)) {
        msg_print(_("『全滅してやるぞ！』", "\"I'll annihilate THEM!\""));
    }

    player_ptr->oldpy = 0;
    player_ptr->oldpx = 0;
    leave_quest_check(player_ptr);

    exe_enter_quest(player_ptr, i2enum<QuestId>(floor.get_grid(player_ptr->get_position()).special));
}

bool inside_quest(QuestId id)
{
    return id != QuestId::NONE;
}
