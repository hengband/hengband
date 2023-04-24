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
#include "io/files-util.h"
#include "main/init-error-messages-table.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "realm/realm-names-table.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

static concptr variant = "ZANGBAND";

/*!
 * @brief 固定マップ (クエスト＆街＆広域マップ)生成時の分岐処理
 * Helper function for "parse_fixed_map()"
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sp
 * @param fp
 * @return エラーコード
 */
static concptr parse_fixed_map_expression(PlayerType *player_ptr, char **sp, char *fp)
{
    static std::string tmp;
    char b1 = '[';
    char b2 = ']';

    char f = ' ';

    char *s;
    s = (*sp);

    while (iswspace(*s)) {
        s++;
    }

    char *b;
    b = s;
    concptr v = "?o?o?";
    if (*s == b1) {
        concptr p;
        concptr t;
        s++;
        t = parse_fixed_map_expression(player_ptr, &s, &f);
        if (!*t) {
            /* Nothing */
        } else if (streq(t, "IOR")) {
            v = "0";
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
                if (*t && !streq(t, "0")) {
                    v = "1";
                }
            }
        } else if (streq(t, "AND")) {
            v = "1";
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
                if (*t && streq(t, "0")) {
                    v = "0";
                }
            }
        } else if (streq(t, "NOT")) {
            v = "1";
            while (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
                if (*t && streq(t, "1")) {
                    v = "0";
                }
            }
        } else if (streq(t, "EQU")) {
            v = "0";
            if (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
            }

            while (*s && (f != b2)) {
                p = parse_fixed_map_expression(player_ptr, &s, &f);
                if (streq(t, p)) {
                    v = "1";
                }
            }
        } else if (streq(t, "LEQ")) {
            v = "1";
            if (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
            }

            while (*s && (f != b2)) {
                p = t;
                t = parse_fixed_map_expression(player_ptr, &s, &f);
                if (*t && atoi(p) > atoi(t)) {
                    v = "0";
                }
            }
        } else if (streq(t, "GEQ")) {
            v = "1";
            if (*s && (f != b2)) {
                t = parse_fixed_map_expression(player_ptr, &s, &f);
            }

            while (*s && (f != b2)) {
                p = t;
                t = parse_fixed_map_expression(player_ptr, &s, &f);
                if (*t && atoi(p) < atoi(t)) {
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
        v = _(rp_ptr->E_title, rp_ptr->title);
    } else if (streq(b + 1, "CLASS")) {
        v = _(cp_ptr->E_title, cp_ptr->title);
    } else if (streq(b + 1, "REALM1")) {
        v = _(E_realm_names[player_ptr->realm1], realm_names[player_ptr->realm1]);
    } else if (streq(b + 1, "REALM2")) {
        v = _(E_realm_names[player_ptr->realm2], realm_names[player_ptr->realm2]);
    } else if (streq(b + 1, "PLAYER")) {
        static char tmp_player_name[32];
        char *pn, *tpn;
        for (pn = player_ptr->name, tpn = tmp_player_name; *pn; pn++, tpn++) {
#ifdef JP
            if (iskanji(*pn)) {
                *(tpn++) = *(pn++);
                *tpn = *pn;
                continue;
            }
#endif
            *tpn = angband_strchr(" []", *pn) ? '_' : *pn;
        }

        *tpn = '\0';
        v = tmp_player_name;
    } else if (streq(b + 1, "TOWN")) {
        tmp = std::to_string(player_ptr->town_num);
        v = tmp.data();
    } else if (streq(b + 1, "LEVEL")) {
        tmp = std::to_string(player_ptr->lev);
        v = tmp.data();
    } else if (streq(b + 1, "QUEST_NUMBER")) {
        tmp = std::to_string(enum2i(player_ptr->current_floor_ptr->quest_number));
        v = tmp.data();
    } else if (streq(b + 1, "LEAVING_QUEST")) {
        tmp = std::to_string(enum2i(leaving_quest));
        v = tmp.data();
    } else if (prefix(b + 1, "QUEST_TYPE")) {
        const auto &quest_list = QuestList::get_instance();
        tmp = std::to_string(enum2i(quest_list[i2enum<QuestId>(atoi(b + 11))].type));
        v = tmp.data();
    } else if (prefix(b + 1, "QUEST")) {
        const auto &quest_list = QuestList::get_instance();
        tmp = std::to_string(enum2i(quest_list[i2enum<QuestId>(atoi(b + 6))].status));
        v = tmp.data();
    } else if (prefix(b + 1, "RANDOM")) {
        tmp = std::to_string((int)(w_ptr->seed_town % atoi(b + 7)));
        v = tmp.data();
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
parse_error_type parse_fixed_map(PlayerType *player_ptr, concptr name, int ymin, int xmin, int ymax, int xmax)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_EDIT, name);
    FILE *fp = angband_fopen(buf, FileOpenMode::READ);
    if (fp == nullptr) {
        return PARSE_ERROR_GENERIC;
    }

    int num = -1;
    parse_error_type err = PARSE_ERROR_NONE;
    bool bypass = false;
    int x = xmin;
    int y = ymin;
    qtwg_type tmp_qg;
    qtwg_type *qg_ptr = initialize_quest_generator_type(&tmp_qg, buf, ymin, xmin, ymax, xmax, &y, &x);
    while (angband_fgets(fp, buf, sizeof(buf)) == 0) {
        num++;
        if (!buf[0] || iswspace(buf[0]) || buf[0] == '#') {
            continue;
        }

        if ((buf[0] == '?') && (buf[1] == ':')) {
            char f;
            char *s;
            s = buf + 2;
            concptr v = parse_fixed_map_expression(player_ptr, &s, &f);
            bypass = streq(v, "0");
            continue;
        }

        if (bypass) {
            continue;
        }

        err = generate_fixed_map_floor(player_ptr, qg_ptr, parse_fixed_map);
        if (err != PARSE_ERROR_NONE) {
            break;
        }
    }

    if (err != 0) {
        concptr oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");
        msg_format("Error %d (%s) at line %d of '%s'.", err, oops, num, name);
        msg_format(_("'%s'を解析中。", "Parsing '%s'."), buf);
        msg_print(nullptr);
    }

    angband_fclose(fp);
    return err;
}

static QuestId parse_quest_number_n(const std::vector<std::string> &token)
{
    auto number = i2enum<QuestId>(atoi(token[1].substr(_(0, 1)).data()));
    return number;
}

static QuestId parse_quest_number(const std::vector<std::string> &token)
{
    auto is_quest_none = _(token[1][0] == '$', token[1][0] != '$');
    if (is_quest_none) {
        return QuestId::NONE;
    }

    if (token[2] == "N") {
        return parse_quest_number_n(token);
    }
    return QuestId::NONE;
}

/*!
 * @brief クエスト番号をファイルから読み込んでパースする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param file_name ファイル名
 * @param key_list キーになるQuestIdの配列
 */
static void parse_quest_info_aux(const char *file_name, std::set<QuestId> &key_list_ref)
{

    auto push_set = [&key_list_ref, &file_name](auto q, auto line) {
        if (q == QuestId::NONE) {
            return;
        }

        if (key_list_ref.find(q) != key_list_ref.end()) {
            std::stringstream ss;
            ss << _("重複したQuestID ", "Duplicated Quest Id ") << enum2i(q) << '(' << file_name << ", L" << line << ')';
            throw std::runtime_error(ss.str());
        }

        key_list_ref.insert(q);
    };

    char file_buf[1024];
    path_build(file_buf, sizeof(file_buf), ANGBAND_DIR_EDIT, file_name);
    auto *fp = angband_fopen(file_buf, FileOpenMode::READ);
    if (fp == nullptr) {
        std::stringstream ss;
        ss << _("ファイルが見つかりません (", "File is not found (") << file_name << ')';
        throw std::runtime_error(ss.str());
    }

    char buf[4096];
    auto line_num = 0;
    while (angband_fgets(fp, buf, sizeof(buf)) == 0) {
        line_num++;

        const auto token = str_split(buf, ':', true);

        switch (token[0][0]) {
        case 'Q': {
            auto quest_number = parse_quest_number(token);
            push_set(quest_number, line_num);
            break;
        }
        case '%': {
            parse_quest_info_aux(token[1].data(), key_list_ref);
            break;
        }
        default:
            break;
        }
    }

    angband_fclose(fp);
}

/*!
 * @brief ファイルからパースして作成したクエスト番号配列を返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param file_name ファイル名
 * @return クエスト番号の配列
 */
std::set<QuestId> parse_quest_info(const char *file_name)
{
    std::set<QuestId> key_list;
    parse_quest_info_aux(file_name, key_list);
    return key_list;
}
