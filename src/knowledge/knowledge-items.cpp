/*!
 * @brief 既知のアイテムとアーティファクトを表示する
 * @date 2020/04/23
 * @author Hourier
 */

#include "knowledge/knowledge-items.h"
#include "core/show-file.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/special-options.h"
#include "io-dump/dump-util.h"
#include "io/input-key-acceptor.h"
#include "io/temp-file.h"
#include "knowledge/item-group-table.h"
#include "perception/identification.h"
#include "system/angband-exceptions.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/artifact/artifact-record.h"
#include "system/baseitem/baseitem-config.h"
#include "system/baseitem/baseitem-configs.h"
#include "system/baseitem/baseitem-record.h"
#include "system/baseitem/baseitem-records.h"
#include "system/baseitem/baseitem-service.h"
#include "system/floor/floor-info.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "tracking/baseitem-tracker.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <fmt/format.h>
#include <memory>
#include <string>
#include <vector>

/*!
 * @brief 入手済の固定アーティファクト一覧を一時ファイルへ保存して表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param mode 表示モード
 */
void do_cmd_knowledge_artifacts(PlayerType *player_ptr, ArtifactKnowledgeMode mode)
{
    TempFile temp_file;
    if (const auto &error_message = temp_file.get_error_message(); error_message) {
        msg_print(*error_message);
        return;
    }

    const auto &records = ArtifactRecords::get_instance();
    std::vector<FixedArtifactId> fa_ids;
    std::string title;
    switch (mode) {
    case ArtifactKnowledgeMode::KNOWN:
        fa_ids = records.collect_known_ids();
        title = _("既知の伝説のアイテム", "Known Artifacts");
        break;
    case ArtifactKnowledgeMode::IDENTIFIED:
        fa_ids = records.collect_identified_ids();
        title = _("鑑定済の伝説のアイテム", "Identified Artifacts");
        break;
    default:
        THROW_EXCEPTION(std::logic_error, fmt::format("Invalid ArtifactKnowledgeMode: {}", enum2i(mode)));
    }

    const auto &artifacts = ArtifactList::get_instance();
    std::stable_sort(fa_ids.begin(), fa_ids.end(), [&artifacts](auto x, auto y) {
        return artifacts.order(x, y);
    });
    std::vector<std::string> lines;
    for (const auto fa_id : fa_ids) {
        const auto &artifact = artifacts.get_artifact(fa_id);
        constexpr auto template_basename = _("     {}", "     The {}");
        ItemEntity item(artifact.bi_key);
        item.fa_id = fa_id;
        item.set_identification_flag(IdentificationFlag::STORE);
        const auto item_name = describe_flavor(player_ptr, item, (OD_OMIT_PREFIX | OD_NAME_ONLY));
        lines.push_back(fmt::format(template_basename, item_name));
    }

    temp_file.write_lines(lines);
    if (const auto &error_message = temp_file.get_error_message(); error_message) {
        msg_print(*error_message);
        return;
    }

    FileDisplayer(player_ptr->name).display(true, temp_file.get_path().string(), 0, 0, title);
}

/*
 * Display the objects in a group.
 */
static void display_object_list(int col, int row, int per_page, const std::vector<short> &bi_ids, int current_bi_id, int top_bi_id, bool visual_only)
{
    const auto is_wizard = AngbandWorld::get_instance().wizard;
    const auto &baseitems = BaseitemList::get_instance();
    const auto &baseitem_records = BaseitemRecords::get_instance();
    const auto &baseitem_configs = BaseitemConfigs::get_instance();
    const auto &empty_symbol = BaseitemService::get_dummy_symbol();
    auto i = 0;
    for (; i < per_page && top_bi_id + i < static_cast<int>(bi_ids.size()); i++) {
        const auto bi_id = bi_ids[top_bi_id + i];
        const auto &baseitem = baseitems.get_baseitem(bi_id);
        const auto &baseitem_record = baseitem_records.get_record(bi_id);
        const auto &baseitem_config = baseitem_configs.get_config(bi_id);
        TERM_COLOR attr = ((baseitem_record.is_aware() || visual_only) ? TERM_WHITE : TERM_SLATE);
        byte cursor = ((baseitem_record.is_aware() || visual_only) ? TERM_L_BLUE : TERM_BLUE);
        const auto has_flavor = baseitem_record.is_apparent();
        const auto appearance_id = baseitem_record.get_appearance_id();
        const auto &flavor_baseitem = !visual_only && has_flavor ? baseitems.get_baseitem(appearance_id) : baseitem;
        const auto &flavor_config = !visual_only && has_flavor ? baseitem_configs.get_config(appearance_id) : baseitem_config;

        attr = ((i + top_bi_id == current_bi_id) ? cursor : attr);
        const auto is_flavor_only = has_flavor && (visual_only || !baseitem_record.is_aware());
        const auto item_name = is_flavor_only ? flavor_baseitem.flavor_name : baseitem.stripped_name();
        c_prt(attr, item_name.data(), row + i, col);
        if (per_page == 1) {
            c_prt(attr, format("%02x/%02x", flavor_config.get_color(), flavor_config.get_character()), row + i, (is_wizard || visual_only) ? 64 : 68);
        }

        if (is_wizard || visual_only) {
            c_prt(attr, format("%d", bi_id), row + i, 70);
        }

        const auto ds = flavor_baseitem.is_valid() ? flavor_config.get_symbol() : empty_symbol;
        term_queue_bigchar(use_bigtile ? 76 : 77, row + i, { ds, {} });
    }

    for (; i < per_page; i++) {
        term_erase(col, row + i);
    }
}

/*
 * Describe fake object
 */
static void desc_obj_fake(PlayerType *player_ptr, short bi_id)
{
    ItemEntity item(bi_id);
    item.set_identification_flag(IdentificationFlag::KNOWN);
    handle_stuff(player_ptr);
    if (screen_object(player_ptr, item, SCROBJ_FAKE_OBJECT | SCROBJ_FORCE_DETAIL)) {
        return;
    }

    msg_print(_("特に変わったところはないようだ。", "You see nothing special."));
    msg_erase();
}

/**
 * @brief Display known objects
 */
void do_cmd_knowledge_objects(PlayerType *player_ptr, bool *need_redraw, bool visual_only, short bi_id)
{
    TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, tl::nullopt);

    std::vector<short> grp_idx;
    bool visual_list = false;
    TERM_COLOR attr_top = 0;
    byte char_left = 0;
    const auto &[wid, hgt] = term_get_size();
    auto browser_rows = hgt - 8;
    auto &baseitem_configs = BaseitemConfigs::get_instance();
    std::vector<short> bi_ids;

    const auto max_element = std::max_element(ITEM_KIND_NAMES_GROUP.begin(), ITEM_KIND_NAMES_GROUP.end(),
        [](auto x, auto y) { return x.length() < y.length(); });
    const int max_length = max_element->length();
    const auto width = wid - (max_length + 3);
    if (bi_id < 0) {
        EnumClassFlagGroup<BaseitemCollectionMode> bcm{ BaseitemCollectionMode::CHECK_CHANCE };
        if (visual_only) {
            bcm.set(BaseitemCollectionMode::VISUAL_ONLY);
        }

        const auto size = static_cast<short>(ITEM_KIND_NAMES_GROUP.size());
        for (short i = 0; i < size; i++) {
            bi_ids = BaseitemService::collect_baseitem_ids(i, bcm);
            if (!bi_ids.empty()) {
                grp_idx.push_back(i);
            }
        }
    } else {
        auto &flavor_config = visual_only ? baseitem_configs.get_config(bi_id) : BaseitemService::get_flavor_config(bi_id);
        bi_ids = { bi_id };
        const auto height = browser_rows - 1;
        auto color = flavor_config.get_color();
        auto character = flavor_config.get_character();
        (void)visual_mode_command('v', &visual_list, height, width, &attr_top, &char_left, &color, &character, need_redraw);
        flavor_config.set_symbol({ color, character });
    }

    EnumClassFlagGroup<BaseitemCollectionMode> bcm{};
    if (visual_only) {
        bcm.set(BaseitemCollectionMode::VISUAL_ONLY);
    }

    short previous_bi_id = bi_id < 0 ? -1 : bi_id;
    short old_grp_cur = -1;
    short grp_cur = 0;
    short grp_top = 0;
    short top_bi_id = 0;
    short current_bi_id = 0;
    auto flag = false;
    auto redraw = true;
    auto column = 0;
    auto &tracker = BaseitemTracker::get_instance();
    const auto &world = AngbandWorld::get_instance();
    const auto &symbols_cb = DisplaySymbolsClipboard::get_instance();
    while (!flag) {
        if (redraw) {
            clear_from(0);

#ifdef JP
            prt(format("%s - アイテム", !visual_only ? "知識" : "表示"), 2, 0);
            if (bi_id < 0) {
                prt("グループ", 4, 0);
            }
            prt("名前", 4, max_length + 3);
            if (world.wizard || visual_only) {
                prt("Idx", 4, 70);
            }
            prt("文字", 4, 74);
#else
            prt(format("%s - objects", !visual_only ? "Knowledge" : "Visuals"), 2, 0);
            if (bi_id < 0) {
                prt("Group", 4, 0);
            }
            prt("Name", 4, max_length + 3);
            if (world.wizard || visual_only) {
                prt("Idx", 4, 70);
            }
            prt("Sym", 4, 75);
#endif

            for (IDX i = 0; i < 78; i++) {
                term_putch(i, 5, { TERM_WHITE, '=' });
            }

            if (bi_id < 0) {
                for (IDX i = 0; i < browser_rows; i++) {
                    term_putch(max_length + 1, 6 + i, { TERM_WHITE, '|' });
                }
            }

            redraw = false;
        }

        if (bi_id < 0) {
            if (grp_cur < grp_top) {
                grp_top = grp_cur;
            }
            if (grp_cur >= grp_top + browser_rows) {
                grp_top = grp_cur - browser_rows + 1;
            }

            std::vector<std::string> tmp_texts = ITEM_KIND_NAMES_GROUP;
            display_group_list(max_length, browser_rows, grp_idx, tmp_texts, grp_cur, grp_top);
            if (!grp_idx.empty() && (old_grp_cur != grp_cur)) {
                old_grp_cur = grp_cur;
                bi_ids = BaseitemService::collect_baseitem_ids(grp_idx[grp_cur], bcm);
                current_bi_id = 0;
                top_bi_id = 0;
            }

            if (!bi_ids.empty()) {
                while (current_bi_id < top_bi_id) {
                    top_bi_id = std::max<short>(0, top_bi_id - browser_rows / 2);
                }

                while (current_bi_id >= top_bi_id + browser_rows) {
                    const auto max_top = std::max<short>(0, static_cast<short>(bi_ids.size()) - browser_rows);
                    top_bi_id = std::min<short>(max_top, top_bi_id + browser_rows / 2);
                }
            }
        }

        if (!visual_list) {
            display_object_list(max_length + 3, 6, browser_rows, bi_ids, current_bi_id, top_bi_id, visual_only);
        } else {
            top_bi_id = current_bi_id;
            display_object_list(max_length + 3, 6, 1, bi_ids, current_bi_id, top_bi_id, visual_only);
            display_visual_list(max_length + 3, 7, browser_rows - 1, wid - (max_length + 3), attr_top, char_left);
        }

#ifdef JP
        prt(format("<方向>%s%s%s, ESC", (!visual_list && !visual_only) ? ", 'r'で詳細を見る" : "", visual_list ? ", ENTERで決定" : ", 'v'でシンボル変更",
                (symbols_cb.symbol != DisplaySymbol()) ? ", 'c', 'p'でペースト" : ", 'c'でコピー"),
            hgt - 1, 0);
#else
        prt(format("<dir>%s%s%s, ESC", (!visual_list && !visual_only) ? ", 'r' to recall" : "", visual_list ? ", ENTER to accept" : ", 'v' for visuals",
                (symbols_cb.symbol != DisplaySymbol()) ? ", 'c', 'p' to paste" : ", 'c' to copy"),
            hgt - 1, 0);
#endif

        if (bi_ids.empty()) {
            if (!column) {
                term_gotoxy(0, 6 + (grp_cur - grp_top));
            }

            const auto ch = inkey();
            switch (ch) {
            case ESCAPE: {
                flag = true;
                break;
            }
            default: {
                browser_cursor(ch, &column, &grp_cur, std::ssize(grp_idx), &current_bi_id, 0);
                break;
            }
            }

            continue;
        }

        const auto bi_id_cursor = bi_ids[current_bi_id];
        if (!visual_only) {
            tracker.set_trackee(bi_id_cursor);
            if (previous_bi_id != bi_id_cursor) {
                handle_stuff(player_ptr);
                previous_bi_id = bi_id_cursor;
            }
        }

        auto &flavor_config = visual_only ? baseitem_configs.get_config(bi_id_cursor) : BaseitemService::get_flavor_config(bi_id_cursor);
        auto color = flavor_config.get_color();
        auto character = flavor_config.get_character();
        if (visual_list) {
            place_visual_list_cursor(max_length + 3, 7, color, character, attr_top, char_left);
        } else if (!column) {
            term_gotoxy(0, 6 + (grp_cur - grp_top));
        } else {
            term_gotoxy(max_length + 3, 6 + (current_bi_id - top_bi_id));
        }

        char ch = inkey();
        const auto height = browser_rows - 1;
        if (visual_mode_command(ch, &visual_list, height, width, &attr_top, &char_left, &color, &character, need_redraw)) {
            flavor_config.set_symbol({ color, character });
            if (bi_id >= 0) {
                switch (ch) {
                case '\n':
                case '\r':
                case ESCAPE:
                    flag = true;
                    break;
                }
            }
            continue;
        }

        switch (ch) {
        case ESCAPE: {
            flag = true;
            break;
        }

        case 'R':
        case 'r': {
            if (!visual_list && !visual_only) {
                desc_obj_fake(player_ptr, bi_id_cursor);
                redraw = true;
            }

            break;
        }

        default: {
            browser_cursor(ch, &column, &grp_cur, std::ssize(grp_idx), &current_bi_id, bi_ids.size());
            break;
        }
        }
    }
}
