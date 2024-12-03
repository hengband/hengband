#include "dungeon/quest.h"
#include "artifact/fixed-art-types.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-object.h"
#include "game-option/play-record-options.h"
#include "info-reader/fixed-map-parser.h"
#include "io/write-diary.h"
#include "locale/english.h"
#include "main/music-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-util.h"
#include "monster/smart-learn-types.h"
#include "object-enchant/item-apply-magic.h"
#include "object-enchant/trg-types.h"
#include "player-status/player-energy.h"
#include "player/player-personality-types.h"
#include "player/player-status.h"
#include "system/artifact-type-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor-type-definition.h" // @todo 相互参照、将来的に削除する.
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <sstream>
#include <stdexcept>

std::vector<std::string> quest_text_lines; /*!< Quest text */
QuestId leaving_quest = QuestId::NONE;

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
 * @brief 該当IDが固定クエストかどうかを判定する.
 * @param quest_id クエストID
 * @return 固定クエストならばTRUEを返す
 */
bool QuestType::is_fixed(QuestId quest_id)
{
    return (enum2i(quest_id) < MIN_RANDOM_QUEST) || (enum2i(quest_id) > MAX_RANDOM_QUEST);
}

bool QuestType::has_reward() const
{
    return this->reward_fa_id != FixedArtifactId::NONE;
}

ArtifactType &QuestType::get_reward() const
{
    auto &artifacts = ArtifactList::get_instance();
    return artifacts.get_artifact(this->reward_fa_id);
}

/*!
 * @brief 討伐対象モンスターを返す. いなければプレイヤー (無効値の意)
 * @return 討伐対象モンスター
 */
MonraceDefinition &QuestType::get_bounty()
{
    return MonraceList::get_instance().get_monrace(this->r_idx);
}

/*!
 * @brief 討伐対象モンスターを返す. いなければプレイヤー (無効値の意)
 * @return 討伐対象モンスター
 */
const MonraceDefinition &QuestType::get_bounty() const
{
    return MonraceList::get_instance().get_monrace(this->r_idx);
}

QuestList QuestList::instance{};

QuestList &QuestList::get_instance()
{
    return instance;
}

/*!
 * @brief クエストの初期化
 * @details ソフトウェア起動時ではパース関数が動作しないので、各種初期化シーケンス時に遅延初期化する
 */
void QuestList::initialize()
{
    try {
        const auto quest_numbers = parse_quest_info(QUEST_DEFINITION_LIST);
        QuestType quest{};
        quest.status = QuestStatusType::UNTAKEN;
        this->quests.emplace(QuestId::NONE, quest);
        for (const auto q : quest_numbers) {
            this->quests.emplace(q, quest);
        }
    } catch (const std::runtime_error &r) {
        std::stringstream ss;
        ss << _("ファイル読み込みエラー: ", "File loading error: ") << r.what();
        msg_print(ss.str());
        msg_print(nullptr);
        quit(_("クエスト初期化エラー", "Error of quests initializing"));
    }
}

QuestType &QuestList::get_quest(QuestId id)
{
    return this->quests.at(id);
}

const QuestType &QuestList::get_quest(QuestId id) const
{
    return this->quests.at(id);
}

std::map<QuestId, QuestType>::iterator QuestList::begin()
{
    return this->quests.begin();
}

std::map<QuestId, QuestType>::const_iterator QuestList::begin() const
{
    return this->quests.cbegin();
}

std::map<QuestId, QuestType>::iterator QuestList::end()
{
    return this->quests.end();
}

std::map<QuestId, QuestType>::const_iterator QuestList::end() const
{
    return this->quests.cend();
}

std::map<QuestId, QuestType>::reverse_iterator QuestList::rbegin()
{
    return this->quests.rbegin();
}

std::map<QuestId, QuestType>::const_reverse_iterator QuestList::rbegin() const
{
    return this->quests.crbegin();
}

std::map<QuestId, QuestType>::reverse_iterator QuestList::rend()
{
    return this->quests.rend();
}

std::map<QuestId, QuestType>::const_reverse_iterator QuestList::rend() const
{
    return this->quests.crend();
}

std::map<QuestId, QuestType>::iterator QuestList::find(QuestId id)
{
    return this->quests.find(id);
}

std::map<QuestId, QuestType>::const_iterator QuestList::find(QuestId id) const
{
    return this->quests.find(id);
}

size_t QuestList::size() const
{
    return this->quests.size();
}

std::vector<QuestId> QuestList::get_sorted_quest_ids() const
{
    std::vector<QuestId> quest_ids;
    std::transform(++this->quests.begin(), this->quests.end(), std::back_inserter(quest_ids), [](const auto &x) { return x.first; });
    std::stable_sort(quest_ids.begin(), quest_ids.end(), [this](auto x, auto y) { return this->order_completed(x, y); });
    return quest_ids;
}

bool QuestList::order_completed(QuestId id1, QuestId id2) const
{
    const auto &quest1 = this->get_quest(id1);
    const auto &quest2 = this->get_quest(id2);
    return (quest1.comptime != quest2.comptime) ? (quest1.comptime < quest2.comptime) : (quest1.level < quest2.level);
}

/*!
 * @brief ランダムクエストの討伐ユニークを決める / Determine the random quest uniques
 * @param quest クエスト構造体への参照
 */
void determine_random_questor(PlayerType *player_ptr, QuestType &quest)
{
    get_mon_num_prep(player_ptr, mon_hook_quest, nullptr);
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
void record_quest_final_status(QuestType *q_ptr, PLAYER_LEVEL lev, QuestStatusType stat)
{
    q_ptr->status = stat;
    q_ptr->complev = lev;
    auto &world = AngbandWorld::get_instance();
    world.update_playtime();
    q_ptr->comptime = world.play_time;
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
    msg_print(nullptr);
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
        found_artifact &= (o_ptr->is_specific_artifact(quest.reward_fa_id));
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
    msg_print(nullptr);
    if (num_subjugation != 1) {
        msg_format(_("注意しろ！この階は%d体の%sによって守られている！", "Be warned, this level is guarded by %d %s!"), num_subjugation, name.data());
        return;
    }

    auto is_random_quest_skipped = monrace.kind_flags.has(MonsterKindType::UNIQUE);
    is_random_quest_skipped &= monrace.max_num == 0;
    if (!is_random_quest_skipped) {
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
        quest.get_reward().gen_flags.reset(ItemGenerationTraitType::QUESTITEM);
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
    world.update_playtime();
    tower1.comptime = world.play_time;
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
    if (!cave_has_flag_bold(&floor, player_ptr->y, player_ptr->x, TerrainCharacteristics::QUEST_ENTER)) {
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
