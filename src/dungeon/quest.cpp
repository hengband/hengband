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
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
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
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h" // @todo 相互参照、将来的に削除する.
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <sstream>
#include <stdexcept>

char quest_text[10][80]; /*!< Quest text */
int quest_text_line; /*!< Current line of the quest text */
QuestId leaving_quest = QuestId::NONE;

/*!
 * @brief クエスト突入時のメッセージテーブル / Array of places to find an inscription
 */
static concptr find_quest_map[] = {
    _("床にメッセージが刻まれている:", "You find the following inscription in the floor"),
    _("壁にメッセージが刻まれている:", "You see a message inscribed in the wall"),
    _("メッセージを見つけた:", "There is a sign saying"),
    _("何かが階段の上に書いてある:", "Something is written on the staircase"),
    _("巻物を見つけた。メッセージが書いてある:", "You find a scroll with the following message"),
};

QuestList &QuestList::get_instance()
{
    static QuestList instance{};
    return instance;
}

QuestType &QuestList::operator[](QuestId id)
{
    return this->quest_data.at(id);
}

const QuestType &QuestList::operator[](QuestId id) const
{
    return this->quest_data.at(id);
}

QuestList::iterator QuestList::begin()
{
    return this->quest_data.begin();
}

QuestList::const_iterator QuestList::begin() const
{
    return this->quest_data.cbegin();
}

QuestList::iterator QuestList::end()
{
    return this->quest_data.end();
}

QuestList::const_iterator QuestList::end() const
{
    return this->quest_data.cend();
}

QuestList::reverse_iterator QuestList::rbegin()
{
    return this->quest_data.rbegin();
}

QuestList::const_reverse_iterator QuestList::rbegin() const
{
    return this->quest_data.crbegin();
}

QuestList::reverse_iterator QuestList::rend()
{
    return this->quest_data.rend();
}

QuestList::const_reverse_iterator QuestList::rend() const
{
    return this->quest_data.crend();
}

QuestList::iterator QuestList::find(QuestId id)
{
    return this->quest_data.find(id);
}

QuestList::const_iterator QuestList::find(QuestId id) const
{
    return this->quest_data.find(id);
}

size_t QuestList::size() const
{
    return this->quest_data.size();
}

/*!
 * @brief クエスト情報初期化のメインルーチン /
 * Initialize quest array
 */
void QuestList::initialize()
{
    if (initialized) {
        return;
    }
    try {
        auto quest_numbers = parse_quest_info(QUEST_DEFINITION_LIST);
        QuestType init_quest{};
        init_quest.status = QuestStatusType::UNTAKEN;
        this->quest_data.insert({ QuestId::NONE, init_quest });
        for (auto q : quest_numbers) {
            this->quest_data.insert({ q, init_quest });
        }
        initialized = true;
    } catch (const std::runtime_error &r) {
        std::stringstream ss;
        ss << _("ファイル読み込みエラー: ", "File loading error: ") << r.what();

        msg_print(ss.str());
        msg_print(nullptr);
        quit(_("クエスト初期化エラー", "Error of quests initializing"));
    }
}

/*!
 * @brief 該当IDが固定クエストかどうかを判定する.
 * @param quest_idx クエストID
 * @return 固定クエストならばTRUEを返す
 */
bool QuestType::is_fixed(QuestId quest_idx)
{
    return (enum2i(quest_idx) < MIN_RANDOM_QUEST) || (enum2i(quest_idx) > MAX_RANDOM_QUEST);
}

bool QuestType::has_reward() const
{
    return this->reward_artifact_idx != FixedArtifactId::NONE;
}

ArtifactType &QuestType::get_reward() const
{
    const auto &artifacts = ArtifactsInfo::get_instance();
    return artifacts.get_artifact(this->reward_artifact_idx);
}

/*!
 * @brief ランダムクエストの討伐ユニークを決める / Determine the random quest uniques
 * @param q_ptr クエスト構造体の参照ポインタ
 */
void determine_random_questor(PlayerType *player_ptr, QuestType *q_ptr)
{
    get_mon_num_prep(player_ptr, mon_hook_quest, nullptr);
    MonsterRaceId r_idx;
    while (true) {
        r_idx = get_mon_num(player_ptr, 0, q_ptr->level + 5 + randint1(q_ptr->level / 10), PM_ARENA);
        const auto &monrace = monraces_info[r_idx];
        if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
            continue;
        }

        if (monrace.flags8 & RF8_NO_QUEST) {
            continue;
        }

        if (monrace.flags1 & RF1_QUESTOR) {
            continue;
        }

        if (monrace.rarity > 100) {
            continue;
        }

        if (monrace.behavior_flags.has(MonsterBehaviorType::FRIENDLY)) {
            continue;
        }

        if (monrace.feature_flags.has(MonsterFeatureType::AQUATIC)) {
            continue;
        }

        if (monrace.wilderness_flags.has(MonsterWildernessType::WILD_ONLY)) {
            continue;
        }

        if (MonraceList::get_instance().can_unify_separate(r_idx)) {
            continue;
        }

        /*
         * Accept monsters that are 2 - 6 levels
         * out of depth depending on the quest level
         */
        if (monrace.level > (q_ptr->level + (q_ptr->level / 20))) {
            break;
        }
    }

    q_ptr->r_idx = r_idx;
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
    w_ptr->update_playtime();
    q_ptr->comptime = w_ptr->play_time;
}

/*!
 * @brief クエストを達成状態にする /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param quest_num 達成状態にしたいクエストのID
 */
void complete_quest(PlayerType *player_ptr, QuestId quest_num)
{
    auto &quest_list = QuestList::get_instance();
    auto *const q_ptr = &quest_list[quest_num];

    switch (q_ptr->type) {
    case QuestKindType::RANDOM:
        if (record_rand_quest) {
            exe_write_diary_quest(player_ptr, DiaryKind::RAND_QUEST_C, quest_num);
        }
        break;
    default:
        if (record_fix_quest) {
            exe_write_diary_quest(player_ptr, DiaryKind::FIX_QUEST_C, quest_num);
        }
        break;
    }

    record_quest_final_status(q_ptr, player_ptr->lev, QuestStatusType::COMPLETED);

    if (q_ptr->flags & QUEST_FLAG_SILENT) {
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
    const auto &quest_list = QuestList::get_instance();
    /* Check if completed a quest */
    for (const auto &[q_idx, quest] : quest_list) {
        auto found_artifact = (quest.type == QuestKindType::FIND_ARTIFACT);
        found_artifact &= (quest.status == QuestStatusType::TAKEN);
        found_artifact &= (o_ptr->is_specific_artifact(quest.reward_artifact_idx));
        if (found_artifact) {
            complete_quest(player_ptr, q_idx);
        }
    }
}

/*!
 * @brief クエストの導入メッセージを表示する / Discover quest
 * @param q_idx 開始されたクエストのID
 */
void quest_discovery(QuestId q_idx)
{
    auto &quest_list = QuestList::get_instance();
    auto *q_ptr = &quest_list[q_idx];
    auto *r_ptr = &monraces_info[q_ptr->r_idx];
    MONSTER_NUMBER q_num = q_ptr->max_num;

    if (!inside_quest(q_idx)) {
        return;
    }

    GAME_TEXT name[MAX_NLEN];
    strcpy(name, (r_ptr->name.data()));

    msg_print(find_quest_map[rand_range(0, 4)]);
    msg_print(nullptr);

    if (q_num != 1) {
#ifdef JP
#else
        plural_aux(name);
#endif
        msg_format(_("注意しろ！この階は%d体の%sによって守られている！", "Be warned, this level is guarded by %d %s!"), q_num, name);
        return;
    }

    bool is_random_quest_skipped = r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
    is_random_quest_skipped &= r_ptr->max_num == 0;
    if (!is_random_quest_skipped) {
        msg_format(_("注意せよ！この階は%sによって守られている！", "Beware, this level is protected by %s!"), name);
        return;
    }

    msg_print(_("この階は以前は誰かによって守られていたようだ…。", "It seems that this level was protected by someone before..."));
    record_quest_final_status(q_ptr, 0, QuestStatusType::FINISHED);
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

    auto &quest_list = QuestList::get_instance();
    auto *q_ptr = &quest_list[leaving_quest];
    bool is_one_time_quest = ((q_ptr->flags & QUEST_FLAG_ONCE) || (q_ptr->type == QuestKindType::RANDOM)) && (q_ptr->status == QuestStatusType::TAKEN);
    if (!is_one_time_quest) {
        return;
    }

    record_quest_final_status(q_ptr, player_ptr->lev, QuestStatusType::FAILED);

    /* Additional settings */
    switch (q_ptr->type) {
    case QuestKindType::TOWER:
        quest_list[QuestId::TOWER1].status = QuestStatusType::FAILED;
        quest_list[QuestId::TOWER1].complev = player_ptr->lev;
        break;
    case QuestKindType::FIND_ARTIFACT:
        q_ptr->get_reward().gen_flags.reset(ItemGenerationTraitType::QUESTITEM);
        break;
    case QuestKindType::RANDOM:
        monraces_info[q_ptr->r_idx].flags1 &= ~(RF1_QUESTOR);
        prepare_change_floor_mode(player_ptr, CFM_NO_RETURN);
        break;
    default:
        break;
    }

    /* Record finishing a quest */
    if (q_ptr->type == QuestKindType::RANDOM) {
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
    auto &quest_list = QuestList::get_instance();
    leaving_quest = player_ptr->current_floor_ptr->quest_number;

    auto &tower1 = quest_list[QuestId::TOWER1];
    bool is_leaving_from_tower = inside_quest(leaving_quest);
    is_leaving_from_tower &= quest_list[leaving_quest].type == QuestKindType::TOWER;
    is_leaving_from_tower &= tower1.status != QuestStatusType::COMPLETED;
    if (!is_leaving_from_tower) {
        return;
    }
    if (quest_list[leaving_quest].type != QuestKindType::TOWER) {
        return;
    }
    tower1.status = QuestStatusType::FAILED;
    tower1.complev = player_ptr->lev;
    w_ptr->update_playtime();
    tower1.comptime = w_ptr->play_time;
}

/*!
 * @brief Player enters a new quest
 */
void exe_enter_quest(PlayerType *player_ptr, QuestId quest_idx)
{
    const auto &quest_list = QuestList::get_instance();
    if (quest_list[quest_idx].type != QuestKindType::RANDOM) {
        player_ptr->current_floor_ptr->dun_level = 1;
    }
    player_ptr->current_floor_ptr->quest_number = quest_idx;

    player_ptr->leaving = true;
}

/*!
 * @brief クエスト入り口にプレイヤーが乗った際の処理 / Do building commands
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_quest(PlayerType *player_ptr)
{
    if (player_ptr->wild_mode) {
        return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    if (!cave_has_flag_bold(player_ptr->current_floor_ptr, player_ptr->y, player_ptr->x, TerrainCharacteristics::QUEST_ENTER)) {
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

    exe_enter_quest(player_ptr, i2enum<QuestId>(player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].special));
}

bool inside_quest(QuestId id)
{
    return id != QuestId::NONE;
}
