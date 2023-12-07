/*
 * @brief 帰還やテレポート・レベル等、フロアを跨ぐ魔法効果の処理
 * @author Hourier
 * @date 2022/10/10
 */

#include "spell-kind/spells-world.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "dungeon/quest-completion-checker.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-town.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "io/input-key-acceptor.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "market/building-util.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "system/angband-system.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>

/*!
 * @brief プレイヤー及びモンスターをレベルテレポートさせる /
 * Teleport the player one level up or down (random when legal)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx テレポートの対象となるモンスターID(0ならばプレイヤー) / If m_idx <= 0, target is player.
 * @todo cmd-save.h への依存あり。コールバックで何とかしたい
 */
void teleport_level(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    std::string m_name;
    auto see_m = true;
    auto &floor = *player_ptr->current_floor_ptr;
    if (m_idx <= 0) {
        m_name = _("あなた", "you");
    } else {
        auto *m_ptr = &floor.m_list[m_idx];
        m_name = monster_desc(player_ptr, m_ptr, 0);
        see_m = is_seen(player_ptr, m_ptr);
    }

    if (floor.can_teleport_level(m_idx != 0)) {
        if (see_m) {
            msg_print(_("効果がなかった。", "There is no effect."));
        }
        return;
    }

    if ((m_idx <= 0) && player_ptr->anti_tele) {
        msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
        return;
    }

    bool go_up;
    if (randint0(100) < 50) {
        go_up = true;
    } else {
        go_up = false;
    }

    if ((m_idx <= 0) && w_ptr->wizard) {
        if (input_check("Force to go up? ")) {
            go_up = true;
        } else if (input_check("Force to go down? ")) {
            go_up = false;
        }
    }

    const auto &dungeon = floor.get_dungeon_definition();
    if ((ironman_downward && (m_idx <= 0)) || (floor.dun_level <= dungeon.mindepth)) {
#ifdef JP
        if (see_m) {
            msg_format("%s^は床を突き破って沈んでいく。", m_name.data());
        }
#else
        if (see_m) {
            msg_format("%s^ sink%s through the floor.", m_name.data(), (m_idx <= 0) ? "" : "s");
        }
#endif
        if (m_idx <= 0) {
            if (!floor.is_in_dungeon()) {
                floor.set_dungeon_index(ironman_downward ? DUNGEON_ANGBAND : player_ptr->recall_dungeon);
                player_ptr->oldpy = player_ptr->y;
                player_ptr->oldpx = player_ptr->x;
            }

            if (record_stair) {
                exe_write_diary(player_ptr, DiaryKind::TELEPORT_LEVEL, 1);
            }

            if (autosave_l) {
                do_cmd_save_game(player_ptr, true);
            }

            if (!floor.is_in_dungeon()) {
                floor.dun_level = dungeon.mindepth;
                prepare_change_floor_mode(player_ptr, CFM_RAND_PLACE);
            } else {
                prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_DOWN | CFM_RAND_PLACE | CFM_RAND_CONNECT);
            }

            player_ptr->leaving = true;
        }
    } else if (inside_quest(floor.get_quest_id()) || (floor.dun_level >= dungeon.maxdepth)) {
#ifdef JP
        if (see_m) {
            msg_format("%s^は天井を突き破って宙へ浮いていく。", m_name.data());
        }
#else
        if (see_m) {
            msg_format("%s^ rise%s up through the ceiling.", m_name.data(), (m_idx <= 0) ? "" : "s");
        }
#endif

        if (m_idx <= 0) {
            if (record_stair) {
                exe_write_diary(player_ptr, DiaryKind::TELEPORT_LEVEL, -1);
            }

            if (autosave_l) {
                do_cmd_save_game(player_ptr, true);
            }

            prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_UP | CFM_RAND_PLACE | CFM_RAND_CONNECT);

            leave_quest_check(player_ptr);
            floor.quest_number = QuestId::NONE;
            player_ptr->leaving = true;
        }
    } else if (go_up) {
#ifdef JP
        if (see_m) {
            msg_format("%s^は天井を突き破って宙へ浮いていく。", m_name.data());
        }
#else
        if (see_m) {
            msg_format("%s^ rise%s up through the ceiling.", m_name.data(), (m_idx <= 0) ? "" : "s");
        }
#endif

        if (m_idx <= 0) {
            if (record_stair) {
                exe_write_diary(player_ptr, DiaryKind::TELEPORT_LEVEL, -1);
            }

            if (autosave_l) {
                do_cmd_save_game(player_ptr, true);
            }

            prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_UP | CFM_RAND_PLACE | CFM_RAND_CONNECT);
            player_ptr->leaving = true;
        }
    } else {
#ifdef JP
        if (see_m) {
            msg_format("%s^は床を突き破って沈んでいく。", m_name.data());
        }
#else
        if (see_m) {
            msg_format("%s^ sink%s through the floor.", m_name.data(), (m_idx <= 0) ? "" : "s");
        }
#endif

        if (m_idx <= 0) {
            if (record_stair) {
                exe_write_diary(player_ptr, DiaryKind::TELEPORT_LEVEL, 1);
            }
            if (autosave_l) {
                do_cmd_save_game(player_ptr, true);
            }

            prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_DOWN | CFM_RAND_PLACE | CFM_RAND_CONNECT);
            player_ptr->leaving = true;
        }
    }

    if (m_idx <= 0) {
        sound(SOUND_TPLEVEL);
        return;
    }

    auto *m_ptr = &floor.m_list[m_idx];
    QuestCompletionChecker(player_ptr, m_ptr).complete();
    if (record_named_pet && m_ptr->is_named_pet()) {
        const auto m2_name = monster_desc(player_ptr, m_ptr, MD_INDEF_VISIBLE);
        exe_write_diary(player_ptr, DiaryKind::NAMED_PET, RECORD_NAMED_PET_TELE_LEVEL, m2_name);
    }

    delete_monster_idx(player_ptr, m_idx);
    if (see_m) {
        sound(SOUND_TPLEVEL);
    }
}

bool teleport_level_other(PlayerType *player_ptr)
{
    if (!target_set(player_ptr, TARGET_KILL)) {
        return false;
    }

    const auto &floor = *player_ptr->current_floor_ptr;
    const Pos2D pos(target_row, target_col);
    const auto &grid = floor.get_grid(pos);
    const auto target_m_idx = grid.m_idx;
    if (!target_m_idx) {
        return true;
    }
    if (!grid.has_los()) {
        return true;
    }
    if (!projectable(player_ptr, player_ptr->y, player_ptr->x, target_row, target_col)) {
        return true;
    }

    const auto &monster = floor.m_list[target_m_idx];
    const auto &monrace = monster.get_monrace();
    const auto m_name = monster_desc(player_ptr, &monster, 0);
    msg_format(_("%s^の足を指さした。", "You gesture at %s^'s feet."), m_name.data());

    auto has_immune = monrace.resistance_flags.has_any_of(RFR_EFF_RESIST_NEXUS_MASK) || monrace.resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT);
    if (has_immune || (monrace.flags1 & RF1_QUESTOR) || (monrace.level + randint1(50) > player_ptr->lev + randint1(60))) {
        msg_print(_("しかし効果がなかった！", format("%s^ is unaffected!", m_name.data())));
    } else {
        teleport_level(player_ptr, target_m_idx);
    }

    return true;
}

/*!
 * @brief 町間のテレポートを行うメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return テレポート処理を決定したか否か
 */
bool tele_town(PlayerType *player_ptr)
{
    if (player_ptr->current_floor_ptr->dun_level) {
        msg_print(_("この魔法は地上でしか使えない！", "This spell can only be used on the surface!"));
        return false;
    }

    if (player_ptr->current_floor_ptr->inside_arena || AngbandSystem::get_instance().is_phase_out()) {
        msg_print(_("この魔法は外でしか使えない！", "This spell can only be used outside!"));
        return false;
    }

    screen_save();
    clear_bldg(4, 10);

    auto num = 0;
    const int towns_size = towns_info.size();
    for (auto i = 1; i < towns_size; i++) {
        char buf[80];

        if ((i == VALID_TOWNS) || (i == SECRET_TOWN) || (i == player_ptr->town_num) || !(player_ptr->visit & (1UL << (i - 1)))) {
            continue;
        }

        strnfmt(buf, sizeof(buf), "%c) %-20s", I2A(i - 1), towns_info[i].name.data());
        prt(buf, 5 + i, 5);
        num++;
    }

    if (num == 0) {
        msg_print(_("まだ行けるところがない。", "You have not yet visited any town."));
        msg_print(nullptr);
        screen_load();
        return false;
    }

    prt(_("どこに行きますか:", "Where do you want to go: "), 0, 0);
    char key;
    while (true) {
        key = inkey();

        if (key == ESCAPE) {
            screen_load();
            return false;
        }

        if ((key < 'a') || (key > ('a' + towns_size - 2))) {
            continue;
        }

        const auto town_num = key - 'a' + 1;
        if ((town_num == player_ptr->town_num) || (town_num == VALID_TOWNS) || (town_num == SECRET_TOWN) || !(player_ptr->visit & (1UL << (key - 'a')))) {
            continue;
        }

        break;
    }

    for (POSITION y = 0; y < w_ptr->max_wild_y; y++) {
        for (POSITION x = 0; x < w_ptr->max_wild_x; x++) {
            if (wilderness[y][x].town == (key - 'a' + 1)) {
                player_ptr->wilderness_y = y;
                player_ptr->wilderness_x = x;
            }
        }
    }

    player_ptr->leaving = true;
    player_ptr->teleport_town = true;
    screen_load();
    return true;
}

/*!
 * @brief 現実変容処理
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void reserve_alter_reality(PlayerType *player_ptr, TIME_EFFECT turns)
{
    if (player_ptr->current_floor_ptr->inside_arena || ironman_downward) {
        msg_print(_("何も起こらなかった。", "Nothing happens."));
        return;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (player_ptr->alter_reality || turns == 0) {
        player_ptr->alter_reality = 0;
        msg_print(_("景色が元に戻った...", "The view around you returns to normal..."));
        rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        return;
    }

    player_ptr->alter_reality = turns;
    msg_print(_("回りの景色が変わり始めた...", "The view around you begins to change..."));
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
}

/*!
 * @brief これまでに入ったダンジョンの一覧を表示し、選択させる。
 * @param note ダンジョンに施す処理記述
 * @param y コンソールY座標
 * @param x コンソールX座標
 * @return 選択されたダンジョンID
 */
static DUNGEON_IDX choose_dungeon(concptr note, POSITION y, POSITION x)
{
    DUNGEON_IDX select_dungeon;
    if (lite_town || vanilla_town || ironman_downward) {
        if (max_dlv[DUNGEON_ANGBAND]) {
            return DUNGEON_ANGBAND;
        } else {
            msg_format(_("まだ%sに入ったことはない。", "You haven't entered %s yet."), dungeons_info[DUNGEON_ANGBAND].name.data());
            msg_print(nullptr);
            return 0;
        }
    }

    std::vector<DUNGEON_IDX> dun;

    screen_save();
    for (const auto &d_ref : dungeons_info) {
        char buf[80];
        bool seiha = false;

        if (d_ref.idx == 0 || !d_ref.maxdepth) {
            continue;
        }
        if (!max_dlv[d_ref.idx]) {
            continue;
        }
        if (MonsterRace(d_ref.final_guardian).is_valid()) {
            if (!monraces_info[d_ref.final_guardian].max_num) {
                seiha = true;
            }
        } else if (max_dlv[d_ref.idx] == d_ref.maxdepth) {
            seiha = true;
        }

        constexpr auto fmt = _("      %c) %c%-12s : 最大 %d 階", "      %c) %c%-16s : Max level %d");
        strnfmt(buf, sizeof(buf), fmt, static_cast<char>('a' + dun.size()), seiha ? '!' : ' ', d_ref.name.data(), (int)max_dlv[d_ref.idx]);
        prt(buf, y + dun.size(), x);
        dun.push_back(d_ref.idx);
    }

    if (dun.empty()) {
        prt(_("      選べるダンジョンがない。", "      No dungeon is available."), y, x);
    }

    prt(format(_("どのダンジョン%sしますか:", "Which dungeon do you %s?: "), note), 0, 0);
    while (true) {
        auto i = inkey();
        if ((i == ESCAPE) || dun.empty()) {
            screen_load();
            return 0;
        }
        if (i >= 'a' && i < static_cast<char>('a' + dun.size())) {
            select_dungeon = dun[i - 'a'];
            break;
        } else {
            bell();
        }
    }
    screen_load();

    return select_dungeon;
}

/*!
 * @brief プレイヤーの帰還発動及び中止処理 /
 * Recall the player to town or dungeon
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param turns 発動までのターン数
 * @return 常にTRUEを返す
 * @todo Recall the player to the last visited town when in the wilderness
 */
bool recall_player(PlayerType *player_ptr, TIME_EFFECT turns)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (floor.inside_arena || ironman_downward) {
        msg_print(_("何も起こらなかった。", "Nothing happens."));
        return true;
    }

    bool is_special_floor = floor.is_in_dungeon();
    is_special_floor &= max_dlv[floor.dungeon_idx] > floor.dun_level;
    is_special_floor &= !floor.is_in_quest();
    is_special_floor &= !player_ptr->word_recall;
    if (is_special_floor) {
        if (input_check(_("ここは最深到達階より浅い階です。この階に戻って来ますか？ ", "Reset recall depth? "))) {
            max_dlv[floor.dungeon_idx] = floor.dun_level;
            if (record_maxdepth) {
                exe_write_diary(player_ptr, DiaryKind::TRUMP, floor.dungeon_idx, _("帰還のときに", "when recalled from dungeon"));
            }
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (player_ptr->word_recall || turns == 0) {
        player_ptr->word_recall = 0;
        msg_print(_("張りつめた大気が流れ去った...", "A tension leaves the air around you..."));
        rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
        return true;
    }

    if (!floor.is_in_dungeon()) {
        DUNGEON_IDX select_dungeon;
        select_dungeon = choose_dungeon(_("に帰還", "recall"), 2, 14);
        if (!select_dungeon) {
            return false;
        }
        player_ptr->recall_dungeon = select_dungeon;
    }

    player_ptr->word_recall = turns;
    msg_print(_("回りの大気が張りつめてきた...", "The air about you becomes charged..."));
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    return true;
}

bool free_level_recall(PlayerType *player_ptr)
{
    DUNGEON_IDX select_dungeon = choose_dungeon(_("にテレポート", "teleport"), 4, 0);
    if (!select_dungeon) {
        return false;
    }

    const auto &dungeon = dungeons_info[select_dungeon];
    auto max_depth = dungeon.maxdepth;
    if (select_dungeon == DUNGEON_ANGBAND) {
        const auto &quest_list = QuestList::get_instance();
        if (quest_list[QuestId::OBERON].status != QuestStatusType::FINISHED) {
            max_depth = 98;
        } else if (quest_list[QuestId::SERPENT].status != QuestStatusType::FINISHED) {
            max_depth = 99;
        }
    }

    const auto mes = _("%sの何階にテレポートしますか？", "Teleport to which level of %s? ");
    const auto amt = input_quantity(max_depth, format(mes, dungeon.name.data()));
    if (amt <= 0) {
        return false;
    }

    player_ptr->word_recall = 1;
    player_ptr->recall_dungeon = select_dungeon;
    max_dlv[player_ptr->recall_dungeon] = ((amt > dungeon.maxdepth) ? dungeon.maxdepth
                                                                    : ((amt < dungeon.mindepth) ? dungeon.mindepth : amt));
    if (record_maxdepth) {
        exe_write_diary(player_ptr, DiaryKind::TRUMP, select_dungeon, _("トランプタワーで", "at Trump Tower"));
    }

    msg_print(_("回りの大気が張りつめてきた...", "The air about you becomes charged..."));
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    return true;
}

/*!
 * @brief フロア・リセット処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return リセット処理が実際に行われたらTRUEを返す
 */
bool reset_recall(PlayerType *player_ptr)
{
    auto select_dungeon = choose_dungeon(_("をセット", "reset"), 2, 14);
    if (ironman_downward) {
        msg_print(_("何も起こらなかった。", "Nothing happens."));
        return true;
    }

    if (!select_dungeon) {
        return false;
    }

    constexpr auto prompt = _("何階にセットしますか？", "Reset to which level?");
    const auto min_level = dungeons_info[select_dungeon].mindepth;
    const auto max_level = max_dlv[select_dungeon];
    const auto reset_level = input_numerics(prompt, min_level, max_level, player_ptr->current_floor_ptr->dun_level);
    if (!reset_level) {
        return false;
    }

    max_dlv[select_dungeon] = *reset_level;
    if (record_maxdepth) {
        constexpr auto note = _("フロア・リセットで", "using a scroll of reset recall");
        exe_write_diary(player_ptr, DiaryKind::TRUMP, select_dungeon, note);
    }
#ifdef JP
    msg_format("%sの帰還レベルを %d 階にセット。", dungeons_info[select_dungeon].name.data(), *reset_level);
#else
    msg_format("Recall depth set to level %d (%d').", *reset_level, *reset_level * 50);
#endif
    return true;
}
