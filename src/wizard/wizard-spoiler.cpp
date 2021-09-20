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
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
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
#include <array>
#include <set>
#include <string>

/**
 * @brief 進化ツリーの一番根元となるモンスターのIDのリストを取得する
 *
 * @return 進化ツリーの一番根元となるモンスターのIDのリスト(std::setで、evol_root_sortによりソートされている)
 */
static auto get_mon_evol_roots(void)
{
    std::set<MONRACE_IDX> evol_parents;
    std::set<MONRACE_IDX> evol_children;
    for (const auto &r_ref : r_info) {
        if (r_ref.next_r_idx > 0) {
            evol_parents.emplace(r_ref.idx);
            evol_children.emplace(r_ref.next_r_idx);
        }
    }

    auto evol_root_sort = [](MONRACE_IDX i1, MONRACE_IDX i2) {
        auto &r1 = r_info[i1];
        auto &r2 = r_info[i2];
        if (r1.level != r2.level) {
            return r1.level < r2.level;
        }
        if (r1.mexp != r2.mexp) {
            return r1.mexp < r2.mexp;
        }
        return i1 <= i2;
    };

    std::set<MONRACE_IDX, decltype(evol_root_sort)> evol_roots(evol_root_sort);
    std::set_difference(evol_parents.begin(), evol_parents.end(), evol_children.begin(), evol_children.end(),
        std::inserter(evol_roots, evol_roots.end()));

    return evol_roots;
}

/*!
 * @brief 進化ツリーをスポイラー出力するメインルーチン /
 * Print monsters' evolution information to file
 * @param fname 出力ファイル名
 */
static spoiler_output_status spoil_mon_evol(concptr fname)
{
    char buf[1024];
    path_build(buf, sizeof buf, ANGBAND_DIR_USER, fname);
    spoiler_file = angband_fopen(buf, "w");
    if (!spoiler_file) {
        return spoiler_output_status::SPOILER_OUTPUT_FAIL_FOPEN;
    }

    char title[200];
    put_version(title);
    sprintf(buf, "Monster Spoilers for %s\n", title);
    spoil_out(buf);

    spoil_out("------------------------------------------\n\n");

    for (auto r_idx : get_mon_evol_roots()) {
        auto r_ptr = &r_info[r_idx];
        fprintf(spoiler_file, _("[%d]: %s (レベル%d, '%c')\n", "[%d]: %s (Level %d, '%c')\n"), r_idx, r_ptr->name.c_str(), (int)r_ptr->level, r_ptr->d_char);

        for (auto n = 1; r_ptr->next_r_idx > 0; n++) {
            fprintf(spoiler_file, "%*s-(%d)-> ", n * 2, "", r_ptr->next_exp);
            fprintf(spoiler_file, "[%d]: ", r_ptr->next_r_idx);
            r_ptr = &r_info[r_ptr->next_r_idx];

            fprintf(spoiler_file, _("%s (レベル%d, '%c')\n", "%s (Level %d, '%c')\n"), r_ptr->name.c_str(), (int)r_ptr->level, r_ptr->d_char);
        }

        fputc('\n', spoiler_file);
    }

    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? spoiler_output_status::SPOILER_OUTPUT_FAIL_FCLOSE
                                                                : spoiler_output_status::SPOILER_OUTPUT_SUCCESS;
}

spoiler_output_status spoil_categorized_mon_desc()
{
    auto status = spoil_mon_desc("mon-desc-ridable.txt", [](const monster_race *r_ptr) { return any_bits(r_ptr->flags7, RF7_RIDING); });
    if (status == spoiler_output_status::SPOILER_OUTPUT_SUCCESS) {
        status = spoil_mon_desc("mon-desc-wildonly.txt", [](const monster_race *r_ptr) { return any_bits(r_ptr->flags8, RF8_WILD_ONLY); });
    }

    if (status == spoiler_output_status::SPOILER_OUTPUT_SUCCESS) {
        status = spoil_mon_desc("mon-desc-town.txt", [](const monster_race *r_ptr) { return any_bits(r_ptr->flags8, RF8_WILD_TOWN); });
    }

    if (status == spoiler_output_status::SPOILER_OUTPUT_SUCCESS) {
        status = spoil_mon_desc("mon-desc-shore.txt", [](const monster_race *r_ptr) { return any_bits(r_ptr->flags8, RF8_WILD_SHORE); });
    }

    if (status == spoiler_output_status::SPOILER_OUTPUT_SUCCESS) {
        status = spoil_mon_desc("mon-desc-ocean.txt", [](const monster_race *r_ptr) { return any_bits(r_ptr->flags8, RF8_WILD_OCEAN); });
    }

    if (status == spoiler_output_status::SPOILER_OUTPUT_SUCCESS) {
        status = spoil_mon_desc("mon-desc-waste.txt", [](const monster_race *r_ptr) { return any_bits(r_ptr->flags8, RF8_WILD_WASTE); });
    }

    if (status == spoiler_output_status::SPOILER_OUTPUT_SUCCESS) {
        status = spoil_mon_desc("mon-desc-wood.txt", [](const monster_race *r_ptr) { return any_bits(r_ptr->flags8, RF8_WILD_WOOD); });
    }

    if (status == spoiler_output_status::SPOILER_OUTPUT_SUCCESS) {
        status = spoil_mon_desc("mon-desc-volcano.txt", [](const monster_race *r_ptr) { return any_bits(r_ptr->flags8, RF8_WILD_VOLCANO); });
    }

    if (status == spoiler_output_status::SPOILER_OUTPUT_SUCCESS) {
        status = spoil_mon_desc("mon-desc-mountain.txt", [](const monster_race *r_ptr) { return any_bits(r_ptr->flags8, RF8_WILD_MOUNTAIN); });
    }

    if (status == spoiler_output_status::SPOILER_OUTPUT_SUCCESS) {
        status = spoil_mon_desc("mon-desc-grass.txt", [](const monster_race *r_ptr) { return any_bits(r_ptr->flags8, RF8_WILD_GRASS); });
    }

    if (status == spoiler_output_status::SPOILER_OUTPUT_SUCCESS) {
        status = spoil_mon_desc("mon-desc-wildall.txt", [](const monster_race *r_ptr) { return any_bits(r_ptr->flags8, RF8_WILD_ALL); });
    }

    return status;
}

const std::array<const std::string_view, 6> wiz_spell_stat = {
    _("腕力", "STR"),
    _("知能", "INT"),
    _("賢さ", "WIS"),
    _("器用さ", "DEX"),
    _("耐久力", "CON"),
    _("魅力", "CHR"),
};

static spoiler_output_status spoil_player_spell(concptr fname)
{
    char buf[1024];

    path_build(buf, sizeof buf, ANGBAND_DIR_USER, fname);
    spoiler_file = angband_fopen(buf, "w");
    if (!spoiler_file) {
        return spoiler_output_status::SPOILER_OUTPUT_FAIL_FOPEN;
    }

    char title[200];
    put_version(title);
    sprintf(buf, "Player Spells for %s\n", title);
    spoil_out(buf);
    spoil_out("------------------------------------------\n\n");

    player_type dummy_p;
    dummy_p.lev = 1;

    for (int c = 0; c < MAX_CLASS; c++) {
        auto class_ptr = &class_info[c];
        sprintf(buf, "[[Class: %s]]\n", class_ptr->title);
        spoil_out(buf);

        auto magic_ptr = &m_info[c];
        concptr book_name = "なし";
        if (magic_ptr->spell_book != 0) {
            object_type book;
            auto o_ptr = &book;
            o_ptr->prep(lookup_kind(magic_ptr->spell_book, 0));
            describe_flavor(&dummy_p, title, o_ptr, OD_NAME_ONLY);
            book_name = title;
            char *s = angband_strchr(book_name, '[');
            *s = '\0';
        }

        sprintf(buf, "BookType:%s Stat:%s Xtra:%x Type:%d Weight:%d\n", book_name, wiz_spell_stat[magic_ptr->spell_stat].data(), magic_ptr->spell_xtra,
            magic_ptr->spell_type, magic_ptr->spell_weight);
        spoil_out(buf);
        if (!magic_ptr->spell_book) {
            spoil_out(_("呪文なし\n\n", "No spells.\n\n"));
            continue;
        }

        for (int16_t r = 1; r < MAX_MAGIC; r++) {
            sprintf(buf, "[Realm: %s]\n", realm_names[r]);
            spoil_out(buf);
            spoil_out("Name                     Lv Cst Dif Exp\n");
            for (SPELL_IDX i = 0; i < 32; i++) {
                auto spell_ptr = &magic_ptr->info[r][i];
                auto spell_name = exe_spell(&dummy_p, r, i, SPELL_NAME);
                sprintf(buf, "%-24s %2d %3d %3d %3d\n", spell_name, spell_ptr->slevel, spell_ptr->smana, spell_ptr->sfail, spell_ptr->sexp);
                spoil_out(buf);
            }
            spoil_out("\n");
        }
    }

    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? spoiler_output_status::SPOILER_OUTPUT_FAIL_FCLOSE
                                                                : spoiler_output_status::SPOILER_OUTPUT_SUCCESS;
}

/*!
 * @brief スポイラー出力を行うコマンドのメインルーチン /
 * Create Spoiler files -BEN-
 */
void exe_output_spoilers(void)
{
    screen_save();
    while (true) {
        auto status = spoiler_output_status::SPOILER_OUTPUT_CANCEL;
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
            status = spoil_obj_desc("obj-desc.txt");
            break;
        case '2':
            status = spoil_fixed_artifact("artifact.txt");
            break;
        case '3':
            status = spoil_mon_desc("mon-desc.txt");
            break;
        case '4':
            status = spoil_categorized_mon_desc();
            break;
        case '5':
            status = spoil_mon_info("mon-info.txt");
            break;
        case '6':
            status = spoil_mon_evol("mon-evol.txt");
            break;
        case '7':
            status = spoil_player_spell("spells.txt");
            break;
        default:
            bell();
            break;
        }

        switch (status) {
        case spoiler_output_status::SPOILER_OUTPUT_FAIL_FOPEN:
            msg_print("Cannot create spoiler file.");
            break;
        case spoiler_output_status::SPOILER_OUTPUT_FAIL_FCLOSE:
            msg_print("Cannot close spoiler file.");
            break;
        case spoiler_output_status::SPOILER_OUTPUT_SUCCESS:
            msg_print("Successfully created a spoiler file.");
            break;
        case spoiler_output_status::SPOILER_OUTPUT_CANCEL:
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
spoiler_output_status output_all_spoilers(void)
{
    auto status = spoil_obj_desc("obj-desc.txt");
    if (status != spoiler_output_status::SPOILER_OUTPUT_SUCCESS)
        return status;

    status = spoil_fixed_artifact("artifact.txt");
    if (status != spoiler_output_status::SPOILER_OUTPUT_SUCCESS)
        return status;

    status = spoil_mon_desc("mon-desc.txt");
    if (status != spoiler_output_status::SPOILER_OUTPUT_SUCCESS)
        return status;

    status = spoil_categorized_mon_desc();
    if (status != spoiler_output_status::SPOILER_OUTPUT_SUCCESS)
        return status;

    status = spoil_mon_info("mon-info.txt");
    if (status != spoiler_output_status::SPOILER_OUTPUT_SUCCESS)
        return status;

    status = spoil_mon_evol("mon-evol.txt");
    if (status != spoiler_output_status::SPOILER_OUTPUT_SUCCESS)
        return status;

    return spoiler_output_status::SPOILER_OUTPUT_SUCCESS;
}
