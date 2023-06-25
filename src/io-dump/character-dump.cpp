#include "io-dump/character-dump.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "cmd-building/cmd-building.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-town.h"
#include "game-option/birth-options.h"
#include "game-option/game-play-options.h"
#include "inventory/inventory-slot-types.h"
#include "io-dump/player-status-dump.h"
#include "io-dump/special-class-dump.h"
#include "io/mutations-dump.h"
#include "io/write-diary.h"
#include "knowledge/knowledge-quests.h"
#include "main/angband-headers.h"
#include "market/arena-info-table.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "object/object-info.h"
#include "pet/pet-util.h"
#include "player-info/alignment.h"
#include "player/player-status-flags.h"
#include "player/player-status-table.h"
#include "player/race-info-table.h"
#include "realm/realm-names-table.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/angband-version.h"
#include "system/building-type-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/z-form.h"
#include "util/enum-converter.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <numeric>
#include <string>

/*!
 * @brief プレイヤーのペット情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_aux_pet(PlayerType *player_ptr, FILE *fff)
{
    bool pet = false;
    bool pet_settings = false;
    for (int i = player_ptr->current_floor_ptr->m_max - 1; i >= 1; i--) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[i];

        if (!m_ptr->is_valid()) {
            continue;
        }
        if (!m_ptr->is_pet()) {
            continue;
        }
        pet_settings = true;
        if (!m_ptr->is_named() && (player_ptr->riding != i)) {
            continue;
        }
        if (!pet) {
            fprintf(fff, _("\n\n  [主なペット]\n\n", "\n\n  [Leading Pets]\n\n"));
            pet = true;
        }

        const auto pet_name = monster_desc(player_ptr, m_ptr, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
        fprintf(fff, "%s\n", pet_name.data());
    }

    if (!pet_settings) {
        return;
    }

    fprintf(fff, _("\n\n  [ペットへの命令]\n", "\n\n  [Command for Pets]\n"));

    fprintf(fff, _("\n ドアを開ける:                       %s", "\n Pets open doors:                    %s"),
        (player_ptr->pet_extra_flags & PF_OPEN_DOORS) ? "ON" : "OFF");

    fprintf(fff, _("\n アイテムを拾う:                     %s", "\n Pets pick up items:                 %s"),
        (player_ptr->pet_extra_flags & PF_PICKUP_ITEMS) ? "ON" : "OFF");

    fprintf(fff, _("\n テレポート系魔法を使う:             %s", "\n Allow teleport:                     %s"),
        (player_ptr->pet_extra_flags & PF_TELEPORT) ? "ON" : "OFF");

    fprintf(fff, _("\n 攻撃魔法を使う:                     %s", "\n Allow cast attack spell:            %s"),
        (player_ptr->pet_extra_flags & PF_ATTACK_SPELL) ? "ON" : "OFF");

    fprintf(fff, _("\n 召喚魔法を使う:                     %s", "\n Allow cast summon spell:            %s"),
        (player_ptr->pet_extra_flags & PF_SUMMON_SPELL) ? "ON" : "OFF");

    fprintf(fff, _("\n プレイヤーを巻き込む範囲魔法を使う: %s", "\n Allow involve player in area spell: %s"),
        (player_ptr->pet_extra_flags & PF_BALL_SPELL) ? "ON" : "OFF");

    fputc('\n', fff);
}

/*!
 * @brief クエスト情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_aux_quest(PlayerType *player_ptr, FILE *fff)
{
    fprintf(fff, _("\n\n  [クエスト情報]\n", "\n\n  [Quest Information]\n"));

    const auto &quest_list = QuestList::get_instance();
    std::vector<QuestId> quest_numbers;
    for (const auto &[q_idx, quest] : quest_list) {
        quest_numbers.push_back(q_idx);
    }
    int dummy;
    ang_sort(player_ptr, quest_numbers.data(), &dummy, quest_numbers.size(), ang_sort_comp_quest_num, ang_sort_swap_quest_num);

    fputc('\n', fff);
    do_cmd_knowledge_quests_completed(player_ptr, fff, quest_numbers);
    fputc('\n', fff);
    do_cmd_knowledge_quests_failed(player_ptr, fff, quest_numbers);
    fputc('\n', fff);
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

    if (!w_ptr->total_winner) {
        fprintf(fff, _("\n  [死ぬ直前のメッセージ]\n\n", "\n  [Last Messages]\n\n"));
        for (int i = std::min(message_num(), 30); i >= 0; i--) {
            fprintf(fff, "> %s\n", message_str(i)->data());
        }

        fputc('\n', fff);
        return;
    }

    if (player_ptr->last_message.empty()) {
        return;
    }

    fprintf(fff, _("\n  [*勝利*メッセージ]\n\n", "\n  [*Winning* Message]\n\n"));
    fprintf(fff, "  %s\n", player_ptr->last_message.data());
    fputc('\n', fff);
}

/*!
 * @brief 帰還場所情報をファイルにダンプする
 * @param fff ファイルポインタ
 */
static void dump_aux_recall(FILE *fff)
{
    fprintf(fff, _("\n  [帰還場所]\n\n", "\n  [Recall Depth]\n\n"));
    for (const auto &d_ref : dungeons_info) {
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

        fprintf(fff, _("   %c%-12s: %3d 階\n", "   %c%-16s: level %3d\n"), seiha ? '!' : ' ', d_ref.name.data(), (int)max_dlv[d_ref.idx]);
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
        fprintf(fff, _("\n 保存モード:         ON", "\n Preserve Mode:      ON"));
    } else {
        fprintf(fff, _("\n 保存モード:         OFF", "\n Preserve Mode:      OFF"));
    }

    if (ironman_small_levels) {
        fprintf(fff, _("\n 小さいダンジョン:   ALWAYS", "\n Small Levels:       ALWAYS"));
    } else if (always_small_levels) {
        fprintf(fff, _("\n 小さいダンジョン:   ON", "\n Small Levels:       ON"));
    } else if (small_levels) {
        fprintf(fff, _("\n 小さいダンジョン:   ENABLED", "\n Small Levels:       ENABLED"));
    } else {
        fprintf(fff, _("\n 小さいダンジョン:   OFF", "\n Small Levels:       OFF"));
    }

    if (vanilla_town) {
        fprintf(fff, _("\n 元祖の町のみ:       ON", "\n Vanilla Town:       ON"));
    } else if (lite_town) {
        fprintf(fff, _("\n 小規模な町:         ON", "\n Lite Town:          ON"));
    }

    if (ironman_shops) {
        fprintf(fff, _("\n 店なし:             ON", "\n No Shops:           ON"));
    }

    if (ironman_downward) {
        fprintf(fff, _("\n 階段を上がれない:   ON", "\n Diving Only:        ON"));
    }

    if (ironman_rooms) {
        fprintf(fff, _("\n 普通でない部屋:     ON", "\n Unusual Rooms:      ON"));
    }

    if (ironman_nightmare) {
        fprintf(fff, _("\n 悪夢モード:         ON", "\n Nightmare Mode:     ON"));
    }

    if (ironman_empty_levels) {
        fprintf(fff, _("\n アリーナ:           ALWAYS", "\n Arena Levels:       ALWAYS"));
    } else if (empty_levels) {
        fprintf(fff, _("\n アリーナ:           ENABLED", "\n Arena Levels:       ENABLED"));
    } else {
        fprintf(fff, _("\n アリーナ:           OFF", "\n Arena Levels:       OFF"));
    }

    fputc('\n', fff);

    if (w_ptr->noscore) {
        fprintf(fff, _("\n 何か不正なことをしてしまっています。\n", "\n You have done something illegal.\n"));
    }

    fputc('\n', fff);
}

/*!
 * @brief 闘技場の情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_aux_arena(PlayerType *player_ptr, FILE *fff)
{
    if (lite_town || vanilla_town) {
        return;
    }

    const auto arena_number = player_ptr->arena_number;
    if (arena_number < 0) {
        if (arena_number <= ARENA_DEFEATED_OLD_VER) {
            fprintf(fff, _("\n 闘技場: 敗北\n", "\n Arena: Defeated\n"));
        } else {
            constexpr auto mes = _("\n 闘技場: %d回戦で%sの前に敗北\n", "\n Arena: Defeated by %s in the %d%s fight\n");
            const auto &arena = arena_info[-1 - arena_number];
            const auto &arena_monrace = monraces_info[arena.r_idx];
#ifdef JP
            fprintf(fff, mes, -arena_number, arena_monrace.name.data());
#else
            fprintf(fff, mes, arena_monrace.name.data(), -arena_number, get_ordinal_number_suffix(-arena_number).data());
#endif
        }

        fprintf(fff, "\n");
        return;
    }

    if (arena_number > MAX_ARENA_MONS + 2) {
        fprintf(fff, _("\n 闘技場: 真のチャンピオン\n", "\n Arena: True Champion\n"));
        fprintf(fff, "\n");
        return;
    }

    if (arena_number > MAX_ARENA_MONS - 1) {
        fprintf(fff, _("\n 闘技場: チャンピオン\n", "\n Arena: Champion\n"));
        fprintf(fff, "\n");
        return;
    }

    const auto victory_count = arena_number > MAX_ARENA_MONS ? MAX_ARENA_MONS : arena_number;
#ifdef JP
    fprintf(fff, "\n 闘技場: %2d勝\n", victory_count);
#else
    fprintf(fff, "\n Arena: %2d %s\n", victory_count, (arena_number > 1) ? "Victories" : "Victory");
#endif
    fprintf(fff, "\n");
}

/*!
 * @brief 撃破モンスターの情報をファイルにダンプする
 * @param fff ファイルポインタ
 */
static void dump_aux_monsters(PlayerType *player_ptr, FILE *fff)
{
    fprintf(fff, _("\n  [倒したモンスター]\n\n", "\n  [Defeated Monsters]\n\n"));

    /* Allocate the "who" array */
    uint16_t why = 2;
    std::vector<MonsterRaceId> who;

    /* Count monster kills */
    auto norm_total = 0;
    for (const auto &[monrace_id, monrace] : monraces_info) {
        /* Ignore unused index */
        if (!MonsterRace(monrace_id).is_valid()) {
            continue;
        }

        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            auto dead = (monrace.max_num == 0);
            if (dead) {
                norm_total++;

                /* Add a unique monster to the list */
                who.push_back(monrace.idx);
            }

            continue;
        }

        if (monrace.r_pkills > 0) {
            norm_total += monrace.r_pkills;
        }
    }

    /* No monsters is defeated */
    if (norm_total < 1) {
        fprintf(fff, _("まだ敵を倒していません。\n", "You have defeated no enemies yet.\n"));
        return;
    }

    auto uniq_total = static_cast<int>(who.size());
    /* Defeated more than one normal monsters */
    if (uniq_total == 0) {
#ifdef JP
        fprintf(fff, "%d体の敵を倒しています。\n", norm_total);
#else
        fprintf(fff, "You have defeated %d %s.\n", norm_total, norm_total == 1 ? "enemy" : "enemies");
#endif
        return;
    }

    /* Defeated more than one unique monsters */
#ifdef JP
    fprintf(fff, "%d体のユニーク・モンスターを含む、合計%d体の敵を倒しています。\n", uniq_total, norm_total);
#else
    fprintf(fff, "You have defeated %d %s including %d unique monster%s in total.\n", norm_total, norm_total == 1 ? "enemy" : "enemies", uniq_total,
        (uniq_total == 1 ? "" : "s"));
#endif

    /* Sort the array by dungeon depth of monsters */
    ang_sort(player_ptr, who.data(), &why, uniq_total, ang_sort_comp_hook, ang_sort_swap_hook);
    fprintf(fff, _("\n《上位%d体のユニーク・モンスター》\n", "\n< Unique monsters top %d >\n"), std::min(uniq_total, 10));

    char buf[80];
    for (auto it = who.rbegin(); it != who.rend() && std::distance(who.rbegin(), it) < 10; it++) {
        auto *r_ptr = &monraces_info[*it];
        if (r_ptr->defeat_level && r_ptr->defeat_time) {
            strnfmt(buf, sizeof(buf), _(" - レベル%2d - %d:%02d:%02d", " - level %2d - %d:%02d:%02d"), r_ptr->defeat_level, r_ptr->defeat_time / (60 * 60),
                (r_ptr->defeat_time / 60) % 60, r_ptr->defeat_time % 60);
        } else {
            buf[0] = '\0';
        }

        auto name = str_separate(r_ptr->name, 40);
        fprintf(fff, _("  %-40s (レベル%3d)%s\n", "  %-40s (level %3d)%s\n"), name.front().data(), (int)r_ptr->level, buf);
        for (auto i = 1U; i < name.size(); ++i) {
            fprintf(fff, "  %s\n", name[i].data());
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

    fprintf(fff, _("\n\n あなたは%sとして生まれた。", "\n\n You were born as %s."), race_info[enum2i(player_ptr->start_race)].title);
    for (int i = 0; i < MAX_RACES; i++) {
        if (enum2i(player_ptr->start_race) == i) {
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

        fprintf(fff, _("\n あなたはかつて%sだった。", "\n You were a %s before."), race_info[i].title);
    }

    fputc('\n', fff);
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

    fputc('\n', fff);
    for (int i = 0; i < MAX_MAGIC; i++) {
        if (!(player_ptr->old_realm & 1UL << i)) {
            continue;
        }
        fprintf(fff, _("\n あなたはかつて%s魔法を使えた。", "\n You were able to use %s magic before."), realm_names[i + 1]);
    }

    fputc('\n', fff);
}

/*!
 * @brief 徳の情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_aux_virtues(PlayerType *player_ptr, FILE *fff)
{
    fprintf(fff, _("\n\n  [自分に関する情報]\n\n", "\n\n  [HP-rate & Max stat & Virtues]\n\n"));

    int percent = (int)(((long)player_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) / (2 * player_ptr->hitdie + ((PY_MAX_LEVEL - 1 + 3) * (player_ptr->hitdie + 1))));

#ifdef JP
    if (player_ptr->knowledge & KNOW_HPRATE) {
        fprintf(fff, "現在の体力ランク : %d/100\n\n", percent);
    } else {
        fprintf(fff, "現在の体力ランク : ???\n\n");
    }
    fprintf(fff, "能力の最大値\n");
#else
    if (player_ptr->knowledge & KNOW_HPRATE) {
        fprintf(fff, "Your current Life Rating is %d/100.\n\n", percent);
    } else {
        fprintf(fff, "Your current Life Rating is ???.\n\n");
    }
    fprintf(fff, "Limits of maximum stats\n");
#endif
    for (int v_nr = 0; v_nr < A_MAX; v_nr++) {
        if ((player_ptr->knowledge & KNOW_STAT) || player_ptr->stat_max[v_nr] == player_ptr->stat_max_max[v_nr]) {
            fprintf(fff, "%s 18/%d\n", stat_names[v_nr], player_ptr->stat_max_max[v_nr] - 18);
        } else {
            fprintf(fff, "%s ???\n", stat_names[v_nr]);
        }
    }

    std::string alg = PlayerAlignment(player_ptr).get_alignment_description();
    fprintf(fff, _("\n属性 : %s\n", "\nYour alignment : %s\n"), alg.data());
    fprintf(fff, "\n");
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
        fprintf(fff, _("\n\n  [突然変異]\n\n", "\n\n  [Mutations]\n\n"));
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
        fprintf(fff, _("  [キャラクタの装備]\n\n", "  [Character Equipment]\n\n"));
        for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
            auto item_name = describe_flavor(player_ptr, &player_ptr->inventory_list[i], 0);
            auto is_two_handed = ((i == INVEN_MAIN_HAND) && can_attack_with_sub_hand(player_ptr));
            is_two_handed |= ((i == INVEN_SUB_HAND) && can_attack_with_main_hand(player_ptr));
            if (is_two_handed && has_two_handed_weapons(player_ptr)) {
                item_name = _("(武器を両手持ち)", "(wielding with two-hands)");
            }

            fprintf(fff, "%c) %s\n", index_to_label(i), item_name.data());
        }

        fprintf(fff, "\n\n");
    }

    fprintf(fff, _("  [キャラクタの持ち物]\n\n", "  [Character Inventory]\n\n"));

    for (int i = 0; i < INVEN_PACK; i++) {
        if (!player_ptr->inventory_list[i].is_valid()) {
            break;
        }

        const auto item_name = describe_flavor(player_ptr, &player_ptr->inventory_list[i], 0);
        fprintf(fff, "%c) %s\n", index_to_label(i), item_name.data());
    }

    fprintf(fff, "\n\n");
}

/*!
 * @brief 我が家と博物館のオブジェクト情報をファイルにダンプする
 * @param fff ファイルポインタ
 */
static void dump_aux_home_museum(PlayerType *player_ptr, FILE *fff)
{
    const auto *store_ptr = &towns_info[1].stores[StoreSaleType::HOME];
    if (store_ptr->stock_num) {
        fprintf(fff, _("  [我が家のアイテム]\n", "  [Home Inventory]\n"));
        auto page = 1;
        for (auto i = 0; i < store_ptr->stock_num; i++) {
            if ((i % 12) == 0) {
                fprintf(fff, _("\n ( %d ページ )\n", "\n ( page %d )\n"), page++);
            }

            const auto item_name = describe_flavor(player_ptr, &store_ptr->stock[i], 0);
            fprintf(fff, "%c) %s\n", I2A(i % 12), item_name.data());
        }

        fprintf(fff, "\n\n");
    }

    store_ptr = &towns_info[1].stores[StoreSaleType::MUSEUM];

    if (store_ptr->stock_num == 0) {
        return;
    }

    fprintf(fff, _("  [博物館のアイテム]\n", "  [Museum]\n"));

    auto page = 1;
    for (auto i = 0; i < store_ptr->stock_num; i++) {
        if ((i % 12) == 0) {
            fprintf(fff, _("\n ( %d ページ )\n", "\n ( page %d )\n"), page++);
        }

        const auto item_name = describe_flavor(player_ptr, &store_ptr->stock[i], 0);
        fprintf(fff, "%c) %s\n", I2A(i % 12), item_name.data());
    }

    fprintf(fff, "\n\n");
}

/*!
 * @brief チェックサム情報を出力 / Get check sum in string form
 * @return チェックサム情報の文字列
 */
static std::string get_check_sum(void)
{
    return format("%02x%02x%02x%02x%02x%02x%02x%02x%02x", terrains_header.checksum, baseitems_header.checksum,
        artifacts_header.checksum, egos_header.checksum, monraces_header.checksum, dungeons_header.checksum,
        class_magics_header.checksum, class_skills_header.checksum, vaults_header.checksum);
}

/*!
 * @brief ダンプ出力のメインルーチン
 * Output the character dump to a file
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 * @return エラーコード
 */
void make_character_dump(PlayerType *player_ptr, FILE *fff)
{
    TermCenteredOffsetSetter tos(std::nullopt, std::nullopt);

    fprintf(fff, _("  [%s キャラクタ情報]\n\n", "  [%s Character Dump]\n\n"), get_version().data());

    dump_aux_player_status(player_ptr, fff);
    dump_aux_last_message(player_ptr, fff);
    dump_aux_options(fff);
    dump_aux_recall(fff);
    dump_aux_quest(player_ptr, fff);
    dump_aux_arena(player_ptr, fff);
    dump_aux_monsters(player_ptr, fff);
    dump_aux_virtues(player_ptr, fff);
    dump_aux_race_history(player_ptr, fff);
    dump_aux_realm_history(player_ptr, fff);
    dump_aux_class_special(player_ptr, fff);
    dump_aux_mutations(player_ptr, fff);
    dump_aux_pet(player_ptr, fff);
    fputs("\n\n", fff);
    dump_aux_equipment_inventory(player_ptr, fff);
    dump_aux_home_museum(player_ptr, fff);

    fprintf(fff, _("  [チェックサム: \"%s\"]\n\n", "  [Check Sum: \"%s\"]\n\n"), get_check_sum().data());
}
