/*!
 * @brief スポイラー出力処理 (行数の都合でモンスター進化ツリーもここに入っている)
 * @date 2014/02/17
 * @author
 * Copyright (c) 1997 Ben Harrison, and others
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2013 Deskull rearranged comment for Doxygen.
 * 2020 Hourier rearranged for decreasing lines.
 */

#include "wizard/wizard-spoiler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "object/object-kind-hook.h"
#include "player-info/class-info.h"
#include "realm/realm-names-table.h"
#include "spell/spells-execution.h"
#include "spell/spells-util.h"
#include "system/angband-version.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "wizard/fixed-artifacts-spoiler.h"
#include "wizard/items-spoiler.h"
#include "wizard/monster-info-spoiler.h"
#include "wizard/spoiler-util.h"
#include <algorithm>
#include <array>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <string_view>

static constexpr std::array<std::string_view, 6> wiz_spell_stat = { {
    _("腕力", "STR"),
    _("知能", "INT"),
    _("賢さ", "WIS"),
    _("器用さ", "DEX"),
    _("耐久力", "CON"),
    _("魅力", "CHR"),
} };

/**
 * @brief 進化ツリーの一番根元となるモンスターのIDのリストを取得する
 *
 * @return 進化ツリーの一番根元となるモンスターのIDのリスト(std::setで、evol_root_sortによりソートされている)
 */
static auto get_mon_evol_roots()
{
    std::set<MonsterRaceId> evol_parents;
    std::set<MonsterRaceId> evol_children;
    for (const auto &[r_idx, r_ref] : monraces_info) {
        if (MonsterRace(r_ref.next_r_idx).is_valid()) {
            evol_parents.emplace(r_ref.idx);
            evol_children.emplace(r_ref.next_r_idx);
        }
    }

    auto evol_root_sort = [](MonsterRaceId i1, MonsterRaceId i2) {
        auto &r1 = monraces_info[i1];
        auto &r2 = monraces_info[i2];
        if (r1.level != r2.level) {
            return r1.level < r2.level;
        }
        if (r1.mexp != r2.mexp) {
            return r1.mexp < r2.mexp;
        }
        return i1 <= i2;
    };

    std::set<MonsterRaceId, decltype(evol_root_sort)> evol_roots(evol_root_sort);
    std::set_difference(evol_parents.begin(), evol_parents.end(), evol_children.begin(), evol_children.end(),
        std::inserter(evol_roots, evol_roots.end()));

    return evol_roots;
}

/*!
 * @brief 進化ツリーをスポイラー出力するメインルーチン
 *
 * fprintf() に'%' (壁)を渡すとフォーマット指定子扱いされるので、関数の外でフォーマットしてはいけない.
 */
static SpoilerOutputResultType spoil_mon_evol()
{
    const auto &path = path_build(ANGBAND_DIR_USER, "mon-evol.txt");
    spoiler_file = angband_fopen(path, FileOpenMode::WRITE);
    if (!spoiler_file) {
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    std::stringstream ss;
    ss << "Monster Spoilers for " << get_version() << '\n';
    spoil_out(ss.str());
    spoil_out("------------------------------------------\n\n");
    for (auto r_idx : get_mon_evol_roots()) {
        auto r_ptr = &monraces_info[r_idx];
        constexpr auto fmt_before = _("[%d]: %s (レベル%d, '%c')\n", "[%d]: %s (Level %d, '%c')\n");
        fprintf(spoiler_file, fmt_before, enum2i(r_idx), r_ptr->name.data(), (int)r_ptr->level, r_ptr->d_char);
        for (auto n = 1; MonsterRace(r_ptr->next_r_idx).is_valid(); n++) {
            fprintf(spoiler_file, "%*s-(%d)-> ", n * 2, "", r_ptr->next_exp);
            fprintf(spoiler_file, "[%d]: ", enum2i(r_ptr->next_r_idx));
            r_ptr = &monraces_info[r_ptr->next_r_idx];
            constexpr auto fmt_after = _("%s (レベル%d, '%c')\n", "%s (Level %d, '%c')\n");
            fprintf(spoiler_file, fmt_after, r_ptr->name.data(), (int)r_ptr->level, r_ptr->d_char);
        }

        fputc('\n', spoiler_file);
    }

    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? SpoilerOutputResultType::FILE_CLOSE_FAILED
                                                                : SpoilerOutputResultType::SUCCESSFUL;
}

static SpoilerOutputResultType spoil_categorized_mon_desc()
{
    auto status = spoil_mon_desc("mon-desc-ridable.txt", [](const MonsterRaceInfo *r_ptr) { return any_bits(r_ptr->flags7, RF7_RIDING); });
    if (status == SpoilerOutputResultType::SUCCESSFUL) {
        status = spoil_mon_desc("mon-desc-wildonly.txt", [](const MonsterRaceInfo *r_ptr) { return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_ONLY); });
    }

    if (status == SpoilerOutputResultType::SUCCESSFUL) {
        status = spoil_mon_desc("mon-desc-town.txt", [](const MonsterRaceInfo *r_ptr) { return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_TOWN); });
    }

    if (status == SpoilerOutputResultType::SUCCESSFUL) {
        status = spoil_mon_desc("mon-desc-shore.txt", [](const MonsterRaceInfo *r_ptr) { return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_SHORE); });
    }

    if (status == SpoilerOutputResultType::SUCCESSFUL) {
        status = spoil_mon_desc("mon-desc-ocean.txt", [](const MonsterRaceInfo *r_ptr) { return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_OCEAN); });
    }

    if (status == SpoilerOutputResultType::SUCCESSFUL) {
        status = spoil_mon_desc("mon-desc-waste.txt", [](const MonsterRaceInfo *r_ptr) { return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_WASTE); });
    }

    if (status == SpoilerOutputResultType::SUCCESSFUL) {
        status = spoil_mon_desc("mon-desc-wood.txt", [](const MonsterRaceInfo *r_ptr) { return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_WOOD); });
    }

    if (status == SpoilerOutputResultType::SUCCESSFUL) {
        status = spoil_mon_desc("mon-desc-volcano.txt", [](const MonsterRaceInfo *r_ptr) { return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_VOLCANO); });
    }

    if (status == SpoilerOutputResultType::SUCCESSFUL) {
        status = spoil_mon_desc("mon-desc-mountain.txt", [](const MonsterRaceInfo *r_ptr) { return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_MOUNTAIN); });
    }

    if (status == SpoilerOutputResultType::SUCCESSFUL) {
        status = spoil_mon_desc("mon-desc-grass.txt", [](const MonsterRaceInfo *r_ptr) { return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_GRASS); });
    }

    if (status == SpoilerOutputResultType::SUCCESSFUL) {
        status = spoil_mon_desc("mon-desc-wildall.txt", [](const MonsterRaceInfo *r_ptr) { return r_ptr->wilderness_flags.has(MonsterWildernessType::WILD_ALL); });
    }

    return status;
}

static SpoilerOutputResultType spoil_player_spell()
{
    const auto &path = path_build(ANGBAND_DIR_USER, "spells.txt");
    spoiler_file = angband_fopen(path, FileOpenMode::WRITE);
    if (!spoiler_file) {
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    spoil_out(format("Player spells for %s\n", get_version().data()));
    spoil_out("------------------------------------------\n\n");

    PlayerType dummy_p;
    dummy_p.lev = 1;
    for (auto c = 0; c < PLAYER_CLASS_TYPE_MAX; c++) {
        auto class_ptr = &class_info[c];
        spoil_out(format("[[Class: %s]]\n", class_ptr->title));

        auto magic_ptr = &class_magics_info[c];
        std::string book_name = _("なし", "None");
        if (magic_ptr->spell_book != ItemKindType::NONE) {
            ItemEntity book;
            auto o_ptr = &book;
            o_ptr->prep(lookup_baseitem_id({ magic_ptr->spell_book, 0 }));
            book_name = describe_flavor(&dummy_p, o_ptr, OD_NAME_ONLY);
            auto *s = angband_strchr(book_name.data(), '[');
            if (s != nullptr) {
                book_name.erase(s - book_name.data());
            }
        }

        constexpr auto mes = "BookType:%s Stat:%s Xtra:%x Type:%d Weight:%d\n";
        const auto &spell = wiz_spell_stat[magic_ptr->spell_stat];
        spoil_out(format(mes, book_name.data(), spell.data(), magic_ptr->spell_xtra, magic_ptr->spell_type, magic_ptr->spell_weight));
        if (magic_ptr->spell_book == ItemKindType::NONE) {
            spoil_out(_("呪文なし\n\n", "No spells.\n\n"));
            continue;
        }

        for (int16_t r = 1; r < MAX_MAGIC; r++) {
            spoil_out(format("[Realm: %s]\n", realm_names[r]));
            spoil_out("Name                     Lv Cst Dif Exp\n");
            for (SPELL_IDX i = 0; i < 32; i++) {
                auto spell_ptr = &magic_ptr->info[r][i];
                const auto spell_name = exe_spell(&dummy_p, r, i, SpellProcessType::NAME);
                spoil_out(format("%-24s %2d %3d %3d %3d\n", spell_name->data(), spell_ptr->slevel, spell_ptr->smana, spell_ptr->sfail, spell_ptr->sexp));
            }
            spoil_out("\n");
        }
    }

    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? SpoilerOutputResultType::FILE_CLOSE_FAILED
                                                                : SpoilerOutputResultType::SUCCESSFUL;
}

/*!
 * @brief スポイラー出力を行うコマンドのメインルーチン /
 * Create Spoiler files -BEN-
 */
void exe_output_spoilers(void)
{
    screen_save();
    while (true) {
        auto status = SpoilerOutputResultType::CANCELED;
        term_clear();
        prt("Create a spoiler file.", 2, 0);
        prt("(1) Brief Object Info (obj-desc.txt)", 5, 5);
        prt("(2) Brief Artifact Info (artifact.txt)", 6, 5);
        prt("(3) Brief Monster Info (mon-desc.txt)", 7, 5);
        prt("(4) Brief Categorized Monster Info (mon-desc-*.txt)", 8, 5);
        prt("(5) Full Monster Info (mon-info.txt)", 9, 5);
        prt("(6) Monster Evolution Info (mon-evol.txt)", 10, 5);
        prt("(7) Player Spells Info (spells.txt)", 11, 5);
        prt(_("コマンド:", "Command: "), _(18, 12), 0);
        switch (inkey()) {
        case ESCAPE:
            screen_load();
            return;
        case '1':
            status = spoil_obj_desc();
            break;
        case '2':
            status = spoil_fixed_artifact();
            break;
        case '3':
            status = spoil_mon_desc("mon-desc.txt");
            break;
        case '4':
            status = spoil_categorized_mon_desc();
            break;
        case '5':
            status = spoil_mon_info();
            break;
        case '6':
            status = spoil_mon_evol();
            break;
        case '7':
            status = spoil_player_spell();
            break;
        default:
            bell();
            break;
        }

        switch (status) {
        case SpoilerOutputResultType::FILE_OPEN_FAILED:
            msg_print("Cannot create spoiler file.");
            break;
        case SpoilerOutputResultType::FILE_CLOSE_FAILED:
            msg_print("Cannot close spoiler file.");
            break;
        case SpoilerOutputResultType::SUCCESSFUL:
            msg_print("Successfully created a spoiler file.");
            break;
        case SpoilerOutputResultType::CANCELED:
            break;
        }
        msg_erase();
    }
}

/*!
 * @brief 全スポイラー出力を行うコマンドのメインルーチン /
 * Create Spoiler files -BEN-
 * @return 成功時SPOILER_OUTPUT_SUCCESS / 失敗時エラー状態
 */
SpoilerOutputResultType output_all_spoilers()
{
    auto status = spoil_obj_desc();
    if (status != SpoilerOutputResultType::SUCCESSFUL) {
        return status;
    }

    status = spoil_fixed_artifact();
    if (status != SpoilerOutputResultType::SUCCESSFUL) {
        return status;
    }

    status = spoil_mon_desc("mon-desc.txt");
    if (status != SpoilerOutputResultType::SUCCESSFUL) {
        return status;
    }

    status = spoil_categorized_mon_desc();
    if (status != SpoilerOutputResultType::SUCCESSFUL) {
        return status;
    }

    status = spoil_mon_info();
    if (status != SpoilerOutputResultType::SUCCESSFUL) {
        return status;
    }

    status = spoil_mon_evol();
    if (status != SpoilerOutputResultType::SUCCESSFUL) {
        return status;
    }

    return SpoilerOutputResultType::SUCCESSFUL;
}
