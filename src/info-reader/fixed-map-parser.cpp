/*!
 * @brief ゲームデータ初期化1 / Initialization (part 1) -BEN-
 * @date 2014/01/28
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * 2014 Deskull rearranged comment for Doxygen
 */

#include "info-reader/fixed-map-parser.h"
#include "dungeon/quest.h"
#include "floor/fixed-map-generator.h"
#include "game-option/birth-options.h"
#include "game-option/runtime-arguments.h"
#include "info-reader/general-parser.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/quest-reader.h"
#include "io/files-util.h"
#include "locale/character-encoding.h"
#include "main/init-error-messages-table.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "player/player-realm.h"
#include "player/process-name.h"
#include "system/angband-exceptions.h"
#include "system/angband-system.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/dungeon/quest-list.h"
#include "system/floor/floor-info.h"
#include "system/gamevalue.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

static concptr variant = "ZANGBAND";

static bool is_valid_town_special(const nlohmann::json &value)
{
    if (!value.is_number_integer()) {
        return false;
    }

    constexpr auto minimum = std::numeric_limits<int16_t>::min();
    constexpr auto maximum = std::numeric_limits<int16_t>::max();
    if (value.is_number_unsigned()) {
        return value.get<uint64_t>() <= static_cast<uint64_t>(maximum);
    }

    const auto special = value.get<int64_t>();
    return special >= minimum && special <= maximum;
}

static parse_error_type load_town_preferences()
{
    if (init_flags & INIT_ONLY_BUILDINGS) {
        return PARSE_ERROR_NONE;
    }

    std::ifstream ifs(path_build(ANGBAND_DIR_EDIT, TOWN_PREFERENCES));
    if (!ifs) {
        return PARSE_ERROR_GENERIC;
    }

    try {
        const auto data = nlohmann::json::parse(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), nullptr, true, true, true);
        if (!data.is_object() || !data.contains("version") || !data["version"].is_number_integer() || data["version"] != 1 ||
            !data.contains("legend") || !data["legend"].is_object() || data["legend"].empty()) {
            return PARSE_ERROR_INVALID_TYPE;
        }

        std::vector<std::pair<unsigned char, dungeon_grid>> legend;
        for (const auto &[symbol, cell_data] : data["legend"].items()) {
            if (symbol.size() != 1 || symbol.front() < '!' || symbol.front() > '~' || !cell_data.is_object() ||
                !cell_data.contains("terrain") || !cell_data["terrain"].is_string() || !cell_data.contains("caveInfo") || !cell_data["caveInfo"].is_array()) {
                return PARSE_ERROR_INVALID_TYPE;
            }
            for (const auto &flag : cell_data["caveInfo"]) {
                if (!flag.is_string()) {
                    return PARSE_ERROR_INVALID_TYPE;
                }
            }
            if (cell_data.contains("special") && !is_valid_town_special(cell_data["special"])) {
                return PARSE_ERROR_INVALID_VALUE;
            }

            QuestLegendCell cell;
            if (const auto err = parse_quest_legend_cell(cell_data, cell); err != PARSE_ERROR_NONE) {
                return err;
            }
            legend.emplace_back(static_cast<unsigned char>(symbol.front()), cell.grid);
        }

        for (const auto &[symbol, grid] : legend) {
            letter[symbol] = grid;
        }
        return PARSE_ERROR_NONE;
    } catch (const nlohmann::json::exception &) {
        return PARSE_ERROR_INVALID_VALUE;
    }
}

/*!
 * @brief 固定マップ (クエスト＆街＆広域マップ)生成時の分岐処理
 * Helper function for "parse_fixed_map()"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sp
 * @param fp
 * @return エラーコード
 */
static std::string parse_fixed_map_expression(PlayerType *player_ptr, char **sp, char *fp)
{
    constexpr char b1 = '[';
    constexpr char b2 = ']';

    char f = ' ';

    char *s = (*sp);

    while (iswspace(*s)) {
        s++;
    }

    char *b = s;
    std::string v = "?o?o?";
    if (*s == b1) {
        std::string t;
        s++;
        t = parse_fixed_map_expression(player_ptr, &s, &f);
        if (t.empty()) {
            /* Nothing */
        } else if (t == "IOR") {
            v = "0";
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
                if (!t.empty() && t != "0") {
                    v = "1";
                }
            }
        } else if (t == "AND") {
            v = "1";
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
                if (!t.empty() && t == "0") {
                    v = "0";
                }
            }
        } else if (t == "NOT") {
            v = "1";
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
                if (!t.empty() && t == "1") {
                    v = "0";
                }
            }
        } else if (t == "EQU") {
            v = "0";
            if (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
            }

            while (*s && (f != b2)) {
                auto p = parse_fixed_map_expression(player_ptr, &s, &f);
                if (t == p) {
                    v = "1";
                }
            }
        } else if (t == "LEQ") {
            v = "1";
            if (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
            }

            while (*s && (f != b2)) {
                auto p = parse_fixed_map_expression(player_ptr, &s, &f);
                if (!p.empty() && atoi(t.data()) > atoi(p.data())) {
                    v = "0";
                }
            }
        } else if (t == "GEQ") {
            v = "1";
            if (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
            }

            while (*s && (f != b2)) {
                auto p = parse_fixed_map_expression(player_ptr, &s, &f);
                if (!p.empty() && atoi(t.data()) < atoi(p.data())) {
                    v = "0";
                }
            }
        } else {
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
            }
        }

        if (f != b2) {
            v = "?x?x?";
        }
        if ((f = *s) != '\0') {
            *s++ = '\0';
        }

        (*fp) = f;
        (*sp) = s;
        return v;
    }

#ifdef JP
    while (iskanji(*s) || (isprint(*s) && !angband_strchr(" []", *s))) {
        if (iskanji(*s)) {
            s++;
        }
        s++;
    }
#else
    while (isprint(*s) && !angband_strchr(" []", *s)) {
        ++s;
    }
#endif
    if ((f = *s) != '\0') {
        *s++ = '\0';
    }

    if (*b != '$') {
        v = b;
        (*fp) = f;
        (*sp) = s;
        return v;
    }

    if (streq(b + 1, "SYS")) {
        v = ANGBAND_SYS;
    } else if (streq(b + 1, "GRAF")) {
        v = ANGBAND_GRAF;
    } else if (streq(b + 1, "MONOCHROME")) {
        if (arg_monochrome) {
            v = "ON";
        } else {
            v = "OFF";
        }
    } else if (streq(b + 1, "RACE")) {
        v = rp_ptr->title.en_string();
    } else if (streq(b + 1, "CLASS")) {
        v = cp_ptr->title.en_string();
    } else if (streq(b + 1, "REALM1")) {
        v = PlayerRealm(player_ptr).realm1().get_name().en_string();
    } else if (streq(b + 1, "REALM2")) {
        v = PlayerRealm(player_ptr).realm2().get_name().en_string();
    } else if (streq(b + 1, "PLAYER")) {
        v = make_player_name_for_expression(player_ptr->name);
    } else if (streq(b + 1, "TOWN")) {
        v = std::to_string(AngbandWorld::get_instance().get_town_index());
    } else if (streq(b + 1, "LEVEL")) {
        v = std::to_string(player_ptr->lev);
    } else if (streq(b + 1, "QUEST_NUMBER")) {
        v = std::to_string(enum2i(player_ptr->current_floor_ptr->quest_number));
    } else if (streq(b + 1, "LEAVING_QUEST")) {
        v = std::to_string(enum2i(leaving_quest));
    } else if (prefix(b + 1, "QUEST_TYPE")) {
        const auto &quests = QuestList::get_instance();
        v = std::to_string(enum2i(quests.get_quest(i2enum<QuestId>(atoi(b + 11))).type));
    } else if (prefix(b + 1, "QUEST")) {
        const auto &quests = QuestList::get_instance();
        v = std::to_string(enum2i(quests.get_quest(i2enum<QuestId>(atoi(b + 6))).status));
    } else if (prefix(b + 1, "RANDOM")) {
        const auto &system = AngbandSystem::get_instance();
        v = std::to_string((static_cast<int>(system.get_seed_town()) % std::stoi(b + 7)));
    } else if (streq(b + 1, "VARIANT")) {
        v = variant;
    } else if (streq(b + 1, "WILDERNESS")) {
        if (vanilla_town) {
            v = "NONE";
        } else if (lite_town) {
            v = "LITE";
        } else {
            v = "NORMAL";
        }
    } else if (streq(b + 1, "IRONMAN_DOWNWARD")) {
        v = (ironman_downward ? "1" : "0");
    }

    (*fp) = f;
    (*sp) = s;
    return v;
}

/*!
 * @brief 固定マップ (クエスト＆街＆広域マップ)をq_info、t_info、w_infoから読み込んでパースする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name ファイル名
 * @param ymin 詳細不明
 * @param xmin 詳細不明
 * @param ymax 詳細不明
 * @param xmax 詳細不明
 * @return エラーコード
 */
parse_error_type parse_fixed_map(PlayerType *player_ptr, std::string_view name, int ymin, int xmin, int ymax, int xmax)
{
    if (name == TOWN_DEFINITION_LIST) {
        if (const auto err = load_town_preferences(); err != PARSE_ERROR_NONE) {
            const auto oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");
            msg_print("Error {} ({}) loading '{}'.", enum2i(err), oops, TOWN_PREFERENCES);
            msg_erase();
            return err;
        }
    }

    const auto path = path_build(ANGBAND_DIR_EDIT, name);
    std::ifstream ifs(path);
    if (!ifs) {
        return PARSE_ERROR_GENERIC;
    }

    auto num = 0;
    parse_error_type err = PARSE_ERROR_NONE;
    bool bypass = false;
    auto x = xmin;
    auto y = ymin;
    qtwg_type tmp_qg;
    qtwg_type *qg_ptr = initialize_quest_generator_type(&tmp_qg, ymin, xmin, ymax, xmax, &y, &x);
    std::string line;
    while (std::getline(ifs, line)) {
        num++;
        line = utf8_to_local(line);
        if (line.empty() || (std::isspace(static_cast<unsigned char>(line.front())) != 0) || line.starts_with('#')) {
            continue;
        }

        if (line.starts_with("?:")) {
            char f;
            auto *s = line.data() + 2;
            auto v = parse_fixed_map_expression(player_ptr, &s, &f);
            bypass = v == "0";
            continue;
        }

        if (bypass) {
            continue;
        }

        qg_ptr->buf = line.data();
        err = generate_fixed_map_floor(player_ptr, qg_ptr, parse_fixed_map);
        if (err != PARSE_ERROR_NONE) {
            const auto oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");
            msg_print("Error {} ({}) at line {} of '{}'.", enum2i(err), oops, num, name);
            msg_print(_("'{}'を解析中。", "Parsing '{}'."), line);
            msg_erase();
            break;
        }
    }

    if (ifs.bad() || (ifs.fail() && !ifs.eof())) {
        constexpr auto fmt = _("ファイルの読み込みに失敗しました ({})", "Failed to read file ({})");
        THROW_EXCEPTION(std::runtime_error, fmt::format(fmt, path.string()));
    }

    return err;
}
