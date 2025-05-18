#include "io-dump/character-dump.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "inventory/inventory-slot-types.h"
#include "io-dump/player-status-dump.h"
#include "io-dump/special-class-dump.h"
#include "io/mutations-dump.h"
#include "io/write-diary.h"
#include "knowledge/knowledge-quests.h"
#include "main/angband-headers.h"
#include "market/arena-entry.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "object/object-info.h"
#include "pet/pet-util.h"
#include "player-info/alignment.h"
#include "player/player-realm.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/race-info-table.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/angband-system.h"
#include "system/building-type-definition.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/inner-game-data.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/services/dungeon-service.h"
#include "term/gameterm.h"
#include "term/z-form.h"
#include "util/buffer-shaper.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <fmt/format.h>
#include <numeric>
#include <range/v3/view.hpp>
#include <string>

/*!
 * @brief プレイヤーのペット情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_aux_pet(PlayerType *player_ptr, FILE *fff)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    auto pet = false;
    auto pet_settings = false;
    for (auto i = floor.m_max - 1; i >= 1; i--) {
        const auto &monster = floor.m_list[i];
        if (!monster.is_valid()) {
            continue;
        }

        if (!monster.is_pet()) {
            continue;
        }

        pet_settings = true;
        if (!monster.is_named() && !monster.is_riding()) {
            continue;
        }

        if (!pet) {
            fmt::println(fff, _("\n\n  [主なペット]\n", "\n\n  [Leading Pets]\n"));
            pet = true;
        }

        const auto pet_name = monster_desc(player_ptr, monster, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
        fmt::println(fff, "{}", pet_name);
    }

    if (!pet_settings) {
        return;
    }

    fmt::println(fff, _("\n\n  [ペットへの命令]", "\n\n  [Command for Pets]"));

    fmt::print(fff, _("\n ドアを開ける:                       {}", "\n Pets open doors:                    {}"),
        (player_ptr->pet_extra_flags & PF_OPEN_DOORS) ? "ON" : "OFF");

    fmt::print(fff, _("\n アイテムを拾う:                     {}", "\n Pets pick up items:                 {}"),
        (player_ptr->pet_extra_flags & PF_PICKUP_ITEMS) ? "ON" : "OFF");

    fmt::print(fff, _("\n テレポート系魔法を使う:             {}", "\n Allow teleport:                     {}"),
        (player_ptr->pet_extra_flags & PF_TELEPORT) ? "ON" : "OFF");

    fmt::print(fff, _("\n 攻撃魔法を使う:                     {}", "\n Allow cast attack spell:            {}"),
        (player_ptr->pet_extra_flags & PF_ATTACK_SPELL) ? "ON" : "OFF");

    fmt::print(fff, _("\n 召喚魔法を使う:                     {}", "\n Allow cast summon spell:            {}"),
        (player_ptr->pet_extra_flags & PF_SUMMON_SPELL) ? "ON" : "OFF");

    fmt::print(fff, _("\n プレイヤーを巻き込む範囲魔法を使う: {}", "\n Allow involve player in area spell: {}"),
        (player_ptr->pet_extra_flags & PF_BALL_SPELL) ? "ON" : "OFF");

    fmt::print(fff, "\n");
}

/*!
 * @brief クエスト情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_aux_quest(PlayerType *player_ptr, FILE *fff)
{
    fmt::println(fff, _("\n\n  [クエスト情報]", "\n\n  [Quest Information]"));
    const auto &quests = QuestList::get_instance();
    const auto quest_ids = quests.get_sorted_quest_ids();
    fmt::print(fff, "\n");
    do_cmd_knowledge_quests_completed(player_ptr, fff, quest_ids);
    fmt::print(fff, "\n");
    do_cmd_knowledge_quests_failed(player_ptr, fff, quest_ids);
    fmt::print(fff, "\n");
}

/*!
 * @brief 死の直前メッセージ並びに遺言をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_aux_last_message(PlayerType *player_ptr, FILE *fff)
{
    if (!player_ptr->is_dead) {
        return;
    }

    if (!AngbandWorld::get_instance().total_winner) {
        fmt::println(fff, _("\n  [死ぬ直前のメッセージ]\n", "\n  [Last Messages]\n"));
        constexpr auto msg_line_max = 30;
        constexpr auto msg_width = 80;
        std::vector<std::string> msg_lines;
        for (auto i = 0; i < message_num() && std::size(msg_lines) < msg_line_max; ++i) {
            const auto msg = message_str(i);
            auto lines = shape_buffer(*msg, msg_width);

            msg_lines.insert(msg_lines.end(),
                std::make_move_iterator(lines.rbegin()), std::make_move_iterator(lines.rend()));
        }

        std::reverse(msg_lines.begin(), msg_lines.end());
        for (const auto &line : msg_lines) {
            fmt::println(fff, "> {}", line);
        }

        fmt::print(fff, "\n");
        return;
    }

    if (player_ptr->last_message.empty()) {
        return;
    }

    fmt::println(fff, _("\n  [*勝利*メッセージ]\n", "\n  [*Winning* Message]\n"));
    fmt::println(fff, "  {}\n", player_ptr->last_message);
}

/*!
 * @brief 帰還場所情報をファイルにダンプする
 * @param fff ファイルポインタ
 */
static void dump_aux_recall(FILE *fff)
{
    fmt::println(fff, _("\n  [帰還場所]\n", "\n  [Recall Depth]\n"));
    for (const auto &known_dungeon : DungeonService::build_known_dungeons(DungeonMessageFormat::DUMP)) {
        fmt::print(fff, "{}", known_dungeon);
    }
}

/*!
 * @brief オプション情報をファイルにダンプする
 * @param fff ファイルポインタ
 */
static void dump_aux_options(FILE *fff)
{
    fprintf(fff, _("\n  [オプション設定]\n", "\n  [Option Settings]\n"));
    if (preserve_mode) {
        fmt::print(fff, _("\n 保存モード:         ON", "\n Preserve Mode:      ON"));
    } else {
        fmt::print(fff, _("\n 保存モード:         OFF", "\n Preserve Mode:      OFF"));
    }

    if (ironman_small_levels) {
        fmt::print(fff, _("\n 小さいダンジョン:   ALWAYS", "\n Small Levels:       ALWAYS"));
    } else if (always_small_levels) {
        fmt::print(fff, _("\n 小さいダンジョン:   ON", "\n Small Levels:       ON"));
    } else if (small_levels) {
        fmt::print(fff, _("\n 小さいダンジョン:   ENABLED", "\n Small Levels:       ENABLED"));
    } else {
        fmt::print(fff, _("\n 小さいダンジョン:   OFF", "\n Small Levels:       OFF"));
    }

    if (vanilla_town) {
        fmt::print(fff, _("\n 元祖の町のみ:       ON", "\n Vanilla Town:       ON"));
    } else if (lite_town) {
        fmt::print(fff, _("\n 小規模な町:         ON", "\n Lite Town:          ON"));
    }

    if (ironman_shops) {
        fmt::print(fff, _("\n 店なし:             ON", "\n No Shops:           ON"));
    }

    if (ironman_downward) {
        fmt::print(fff, _("\n 階段を上がれない:   ON", "\n Diving Only:        ON"));
    }

    if (ironman_rooms) {
        fmt::print(fff, _("\n 普通でない部屋:     ON", "\n Unusual Rooms:      ON"));
    }

    if (ironman_nightmare) {
        fmt::print(fff, _("\n 悪夢モード:         ON", "\n Nightmare Mode:     ON"));
    }

    if (ironman_empty_levels) {
        fmt::print(fff, _("\n アリーナ:           ALWAYS", "\n Arena Levels:       ALWAYS"));
    } else if (empty_levels) {
        fmt::print(fff, _("\n アリーナ:           ENABLED", "\n Arena Levels:       ENABLED"));
    } else {
        fmt::print(fff, _("\n アリーナ:           OFF", "\n Arena Levels:       OFF"));
    }

    fmt::print(fff, "\n");
    if (AngbandWorld::get_instance().noscore) {
        fmt::println(fff, _("\n 何か不正なことをしてしまっています。", "\n You have done something illegal."));
    }

    fmt::print(fff, "\n");
}

/*!
 * @brief 闘技場の情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @details 旧バージョン (v1.5.0.1より前)では何回戦で敗北したか記録していないので、便宜的に1回戦で敗北したことにする.
 */
static void dump_aux_arena(FILE *fff)
{
    if (lite_town || vanilla_town) {
        return;
    }

    const auto &entries = ArenaEntryList::get_instance();
    if (entries.get_defeated_entry() && !entries.is_player_true_victor()) {
        constexpr auto fmt = _("\n 闘技場: {0}で{1}の前に敗北", "\n Arena: Defeated by {1} in {0}");
        const auto &monrace = entries.get_monrace();
        const auto fight_number = entries.get_fight_number(false);
        fmt::println(fff, fmt, fight_number, monrace.name);
        fmt::print(fff, "\n");
        return;
    }

    const auto current_entry = entries.get_current_entry();
    if (current_entry > entries.get_true_max_entries()) {
        fmt::println(fff, _("\n 闘技場: 真のチャンピオン", "\n Arena: True Champion"));
        fmt::print(fff, "\n");
        return;
    }

    const auto max_entries = entries.get_max_entries();
    if (current_entry >= max_entries) {
        fmt::println(fff, _("\n 闘技場: チャンピオン", "\n Arena: Champion"));
        fmt::print(fff, "\n");
        return;
    }

    const auto victory_count = current_entry > max_entries ? max_entries : current_entry;
#ifdef JP
    fprintf(fff, "\n 闘技場: %2d勝\n", victory_count);
#else
    fprintf(fff, "\n Arena: %2d %s\n", victory_count, (current_entry > 1) ? "Victories" : "Victory");
#endif
    fmt::print(fff, "\n");
}

/*!
 * @brief 撃破モンスターの情報をファイルにダンプする
 * @param fff ファイルポインタ
 */
static void dump_aux_monsters(FILE *fff)
{
    fmt::println(fff, _("\n  [倒したモンスター]\n", "\n  [Defeated Monsters]\n"));
    std::vector<MonraceId> monrace_ids;
    const auto &monraces = MonraceList::get_instance();
    auto norm_total = 0;
    for (const auto &[monrace_id, monrace] : monraces) {
        /* Ignore unused index */
        if (!monrace.is_valid()) {
            continue;
        }

        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            if (monrace.is_dead_unique()) {
                norm_total++;

                /* Add a unique monster to the list */
                monrace_ids.push_back(monrace.idx);
            }

            continue;
        }

        if (monrace.r_pkills > 0) {
            norm_total += monrace.r_pkills;
        }
    }

    /* No monsters is defeated */
    if (norm_total < 1) {
        fmt::println(fff, _("まだ敵を倒していません。", "You have defeated no enemies yet."));
        return;
    }

    const int uniq_total = std::ssize(monrace_ids);
    /* Defeated more than one normal monsters */
    if (uniq_total == 0) {
#ifdef JP
        fmt::println(fff, "{}体の敵を倒しています。", norm_total);
#else
        fmt::println(fff, "You have defeated {} {}.", norm_total, norm_total == 1 ? "enemy" : "enemies");
#endif
        return;
    }

    /* Defeated more than one unique monsters */
#ifdef JP
    fmt::println(fff, "{}体のユニーク・モンスターを含む、合計{}体の敵を倒しています。", uniq_total, norm_total);
#else
    fmt::println(fff, "You have defeated {} {} including {} unique monster{} in total.", norm_total, norm_total == 1 ? "enemy" : "enemies", uniq_total, (uniq_total == 1 ? "" : "s"));
#endif

    std::stable_sort(monrace_ids.begin(), monrace_ids.end(), [&monraces](auto x, auto y) { return monraces.order(x, y); });
    fmt::println(fff, _("\n《上位{}体のユニーク・モンスター", "\n< Unique monsters top {} >"), std::min(uniq_total, 10));
    for (auto it = monrace_ids.rbegin(); it != monrace_ids.rend() && std::distance(monrace_ids.rbegin(), it) < 10; it++) {
        const auto &monrace = monraces.get_monrace(*it);
        const auto defeat_level = monrace.defeat_level;
        const auto defeat_time = monrace.defeat_time;
        std::string defeat_info;
        if ((defeat_level > 0) && (defeat_time > 0)) {
            constexpr auto fmt = _(" - レベル%2d - %d:%02d:%02d", " - level %2d - %d:%02d:%02d");
            defeat_info = format(fmt, defeat_level, defeat_time / (60 * 60), (defeat_time / 60) % 60, defeat_time % 60);
        }

        const auto names = str_separate(monrace.name, 40);
        fprintf(fff, _("  %-40s (レベル%3d)%s\n", "  %-40s (level %3d)%s\n"), names.front().data(), monrace.level, defeat_info.data());
        for (const auto &name : names | ranges::views::drop(1)) {
            fmt::println(fff, "  {}", name);
        }
    }
}

/*!
 * @brief 元種族情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_aux_race_history(PlayerType *player_ptr, FILE *fff)
{
    if (!player_ptr->old_race1 && !player_ptr->old_race2) {
        return;
    }

    const auto start_race = InnerGameData::get_instance().get_start_race();
    fmt::print(fff, _("\n\n あなたは{}として生まれた。", "\n\n You were born as {}."), race_info[enum2i(start_race)].title);
    for (auto i = 0; i < MAX_RACES; i++) {
        if (enum2i(start_race) == i) {
            continue;
        }
        if (i < 32) {
            if (!(player_ptr->old_race1 & 1UL << i)) {
                continue;
            }
        } else {
            if (!(player_ptr->old_race2 & 1UL << (i - 32))) {
                continue;
            }
        }

        fmt::print(fff, _("\n あなたはかつて{}だった。", "\n You were a {} before."), race_info[i].title);
    }

    fmt::print(fff, "\n");
}

/*!
 * @brief 元魔法領域情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_aux_realm_history(PlayerType *player_ptr, FILE *fff)
{
    if (player_ptr->old_realm == 0) {
        return;
    }

    fmt::print(fff, "\n");
    for (auto realm : MAGIC_REALM_RANGE) {
        if (!(player_ptr->old_realm & (1UL << (enum2i(realm) - 1)))) {
            continue;
        }
        fmt::print(fff, _("\n あなたはかつて{}魔法を使えた。", "\n You were able to use {} magic before."), PlayerRealm::get_name(realm));
    }

    fmt::print(fff, "\n");
}

/*!
 * @brief 徳の情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_aux_virtues(PlayerType *player_ptr, FILE *fff)
{
    fmt::println(fff, _("\n\n  [自分に関する情報]\n", "\n\n  [HP-rate & Max stat & Virtues]\n"));

#ifdef JP
    if (player_ptr->knowledge & KNOW_HPRATE) {
        fmt::println(fff, "現在の体力ランク : {}/100\n", player_ptr->calc_life_rating());
    } else {
        fmt::println(fff, "現在の体力ランク : ???\n");
    }
    fmt::println(fff, "能力の最大値");
#else
    if (player_ptr->knowledge & KNOW_HPRATE) {
        fmt::println(fff, "Your current Life Rating is {}/100.\n", player_ptr->calc_life_rating());
    } else {
        fmt::println(fff, "Your current Life Rating is ???.\n");
    }
    fmt::println(fff, "Limits of maximum stats");
#endif
    for (auto v_nr = 0; v_nr < A_MAX; v_nr++) {
        if ((player_ptr->knowledge & KNOW_STAT) || player_ptr->stat_max[v_nr] == player_ptr->stat_max_max[v_nr]) {
            fmt::println(fff, "{} 18/{}", stat_names[v_nr], player_ptr->stat_max_max[v_nr] - 18);
        } else {
            fmt::println(fff, "{} ???", stat_names[v_nr]);
        }
    }

    std::string alg = PlayerAlignment(player_ptr).get_alignment_description();
    fmt::println(fff, _("\n属性 : {}", "\nYour alignment : {}"), alg);
    fmt::print(fff, "\n");
    dump_virtues(player_ptr, fff);
}

/*!
 * @brief 突然変異の情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_aux_mutations(PlayerType *player_ptr, FILE *fff)
{
    if (player_ptr->muta.any()) {
        fmt::println(fff, _("\n\n  [突然変異]\n", "\n\n  [Mutations]\n"));
        dump_mutations(player_ptr, fff);
    }
}

/*!
 * @brief 所持品の情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_aux_equipment_inventory(PlayerType *player_ptr, FILE *fff)
{
    if (player_ptr->equip_cnt) {
        fmt::println(fff, _("  [キャラクタの装備]\n", "  [Character Equipment]\n"));
        for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
            auto item_name = describe_flavor(player_ptr, *player_ptr->inventory[i], 0);
            auto is_two_handed = ((i == INVEN_MAIN_HAND) && can_attack_with_sub_hand(player_ptr));
            is_two_handed |= ((i == INVEN_SUB_HAND) && can_attack_with_main_hand(player_ptr));
            if (is_two_handed && has_two_handed_weapons(player_ptr)) {
                item_name = _("(武器を両手持ち)", "(wielding with two-hands)");
            }

            fmt::println(fff, "{}) {}", index_to_label(i), item_name);
        }

        fmt::println(fff, "\n");
    }

    fmt::println(fff, _("  [キャラクタの持ち物]\n", "  [Character Inventory]\n"));
    for (auto i = 0; i < INVEN_PACK; i++) {
        if (!player_ptr->inventory[i]->is_valid()) {
            break;
        }

        const auto item_name = describe_flavor(player_ptr, *player_ptr->inventory[i], 0);
        fmt::println(fff, "{}) {}", index_to_label(i), item_name);
    }

    fmt::println(fff, "\n");
}

/*!
 * @brief 我が家と博物館のオブジェクト情報をファイルにダンプする
 * @param fff ファイルポインタ
 */
static void dump_aux_home_museum(PlayerType *player_ptr, FILE *fff)
{
    const auto &home = towns_info[1].get_store(StoreSaleType::HOME);
    if (home.stock_num) {
        fmt::println(fff, _("  [我が家のアイテム]", "  [Home Inventory]"));
        auto page = 1;
        for (auto i = 0; i < home.stock_num; i++) {
            if ((i % 12) == 0) {
                fmt::println(fff, _("\n ( {} ページ )", "\n ( page {} )"), page++);
            }

            const auto item_name = describe_flavor(player_ptr, *home.stock[i], 0);
            fmt::println(fff, "{}) {}", I2A(i % 12), item_name);
        }

        fmt::println(fff, "\n");
    }

    const auto &museum = towns_info[1].get_store(StoreSaleType::MUSEUM);
    if (museum.stock_num == 0) {
        return;
    }

    fmt::println(fff, _("  [博物館のアイテム]", "  [Museum]"));
    auto page = 1;
    for (auto i = 0; i < museum.stock_num; i++) {
        if ((i % 12) == 0) {
            fmt::println(fff, _("\n ( {} ページ )", "\n ( page {} )"), page++);
        }

        const auto item_name = describe_flavor(player_ptr, *museum.stock[i], 0);
        fmt::println(fff, "{}) {}", I2A(i % 12), item_name);
    }

    fmt::println(fff, "\n");
}

/*!
 * @brief チェックサム情報を出力
 * @return チェックサム情報の文字列
 */
static std::string get_check_sum()
{
    static constexpr auto headers = {
        &artifacts_header,
        &baseitems_header,
        &class_magics_header,
        &class_skills_header,
        &dungeons_header,
        &egos_header,
        &monraces_header,
        &terrains_header,
        &vaults_header,
    };

    util::SHA256 sha256;
    for (const auto *header : headers) {
        sha256.update(header->digest.data(), header->digest.size());
    }

    return util::to_string(sha256.digest());
}

/*!
 * @brief ダンプ出力のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return エラーコード
 */
void make_character_dump(PlayerType *player_ptr, FILE *fff)
{
    TermCenteredOffsetSetter tos(MAIN_TERM_MIN_COLS, std::nullopt);

    constexpr auto fmt = _("  [{} キャラクタ情報]\n", "  [{} Character Dump]\n");
    fmt::println(fff, fmt, AngbandSystem::get_instance().build_version_expression(VersionExpression::FULL));

    dump_aux_player_status(player_ptr, fff);
    dump_aux_last_message(player_ptr, fff);
    dump_aux_options(fff);
    dump_aux_recall(fff);
    dump_aux_quest(player_ptr, fff);
    dump_aux_arena(fff);
    dump_aux_monsters(fff);
    dump_aux_virtues(player_ptr, fff);
    dump_aux_race_history(player_ptr, fff);
    dump_aux_realm_history(player_ptr, fff);
    dump_aux_class_special(player_ptr, fff);
    dump_aux_mutations(player_ptr, fff);
    dump_aux_pet(player_ptr, fff);
    fputs("\n\n", fff);
    dump_aux_equipment_inventory(player_ptr, fff);
    dump_aux_home_museum(player_ptr, fff);

    // ダンプの幅をはみ出さないように48文字目以降を切り捨てる
    const std::string checksum = get_check_sum().erase(48);
    fmt::println(fff, _("  [チェックサム: \"{}\"]\n", "  [Check Sum: \"{}\"]\n"), checksum);
}
