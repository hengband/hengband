#include "info-reader/feature-reader.h"
#include "floor/wild.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "info-reader/feature-info-tokens-table.h"
#include "info-reader/info-reader-util.h"
#include "info-reader/parse-error-types.h"
#include "main/angband-headers.h"
#include "room/door-definition.h"
#include "term/gameterm.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*! 地形タグ情報から地形IDを得られなかった場合にtrueを返す */
static bool feat_tag_is_not_found = false;

/*!
 * @brief テキストトークンを走査してフラグを一つ得る（地形情報向け） /
 * Grab one flag in an feature_type from a textual string
 * @param f_ptr 地形情報を保管する先の構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return 見つけたらtrue
 */
static bool grab_one_feat_flag(feature_type *f_ptr, std::string_view what)
{
    if (info_grab_one_flag(f_ptr->flags, f_info_flags, what))
        return true;

    msg_format(_("未知の地形フラグ '%s'。", "Unknown feature flag '%s'."), what.data());
    return false;
}

/*!
 * @brief テキストトークンを走査してフラグ(ステート)を一つ得る（地形情報向け2） /
 * Grab an action in an feature_type from a textual string
 * @param f_ptr 地形情報を保管する先の構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @param count ステートの保存先ID
 * @return 見つけたらtrue
 */
static bool grab_one_feat_action(feature_type *f_ptr, std::string_view what, int count)
{
    if (auto it = f_info_flags.find(what); it != f_info_flags.end()) {
        f_ptr->state[count].action = static_cast<FF_FLAGS_IDX>(it->second);
        return true;
    }

    msg_format(_("未知の地形アクション '%s'。", "Unknown feature action '%s'."), what.data());
    return false;
}

/*!
 * @brief 地形情報(f_info)のパース関数 /
 * Initialize the "f_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_f_info(std::string_view buf, angband_header *head)
{
    static feature_type *f_ptr = NULL;
    const auto &tokens = str_split(buf, ':', false, 10);

    if (tokens[0] == "N") {
        // N:index:tag
        if (tokens.size() < 3)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        if (tokens[1].size() == 0 || tokens[2].size() == 0)
            return PARSE_ERROR_GENERIC;

        auto i = std::stoi(tokens[1]);
        if (i < error_idx)
            return PARSE_ERROR_NON_SEQUENTIAL_RECORDS;
        if (i >= head->info_num)
            return PARSE_ERROR_OUT_OF_BOUNDS;

        error_idx = i;
        f_ptr = &f_info[i];
        f_ptr->tag = tokens[2];

        f_ptr->mimic = (FEAT_IDX)i;
        f_ptr->destroyed = (FEAT_IDX)i;
        for (i = 0; i < MAX_FEAT_STATES; i++)
            f_ptr->state[i].action = FF_FLAG_MAX;

    } else if (!f_ptr)
        return PARSE_ERROR_MISSING_RECORD_HEADER;
    else if (tokens[0] == _("J", "E")) {
        // J:name_ja
        // E:name_en
        if (tokens.size() < 2 || tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        f_ptr->name = tokens[1];
    } else if (tokens[0] == _("E", "J")) {
        //pass
    } else if (tokens[0] == "M") {
        // M:mimic_tag
        if (tokens.size() < 2 || tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        f_ptr->mimic_tag = tokens[1];
    } else if (tokens[0] == "G") {
        // G:symbol:color:lite:lite_symbol:lite_color:dark_symbol:dark_color
        if (tokens.size() < 3)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        size_t n;
        char s_char;
        if (tokens[1].size() == 1) {
            s_char = tokens[1][0];
            n = 2;
        } else if (tokens[1].size() == 0 && tokens[2].size() == 0) {
            if (tokens.size() < 4)
                return PARSE_ERROR_TOO_FEW_ARGUMENTS;

            s_char = ':';
            n = 3;
        } else
            return PARSE_ERROR_GENERIC;

        auto s_attr = color_char_to_attr(tokens[n++][0]);
        if (s_attr > 127)
            return PARSE_ERROR_GENERIC;

        f_ptr->d_char[F_LIT_STANDARD] = s_char;
        f_ptr->d_attr[F_LIT_STANDARD] = s_attr;
        if (tokens.size() == n) {
            for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
                f_ptr->d_char[j] = s_char;
                f_ptr->d_attr[j] = s_attr;
            }
        } else if (tokens[n++] == "LIT") {
            apply_default_feat_lighting(f_ptr->d_attr, f_ptr->d_char);

            for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
                auto c_idx = n + (j - F_LIT_NS_BEGIN) * 2;
                auto a_idx = c_idx + 1;
                if (tokens.size() <= (size_t)a_idx)
                    continue;
                if (tokens[c_idx].size() != 1 || tokens[a_idx].size() != 1)
                    continue;

                f_ptr->d_char[j] = tokens[c_idx][0];

                if (tokens[a_idx] == "*") {
                    // pass
                } else if (tokens[a_idx] == "-") {
                    f_ptr->d_attr[j] = s_attr;
                } else {
                    f_ptr->d_attr[j] = color_char_to_attr(tokens[a_idx][0]);
                    if (f_ptr->d_attr[j] > 127)
                        return PARSE_ERROR_GENERIC;
                }
            }
        } else
            return PARSE_ERROR_GENERIC;
    } else if (tokens[0] == "F") {
        // F:flags
        if (tokens.size() < 2 || tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        const auto &flags = str_split(tokens[1], '|', true, 10);
        for (const auto &f : flags) {
            if (f.size() == 0)
                continue;

            const auto &f_tokens = str_split(f, '_', false, 2);
            if (f_tokens.size() == 2) {
                if (f_tokens[0] == "SUBTYPE") {
                    info_set_value(f_ptr->subtype, f_tokens[1]);
                    continue;
                } else if (f_tokens[0] == "POWER") {
                    info_set_value(f_ptr->power, f_tokens[1]);
                    continue;
                }
            }

            if (!grab_one_feat_flag(f_ptr, f))
                return PARSE_ERROR_INVALID_FLAG;
        }
    } else if (tokens[0] == "W") {
        // W:priority
        if (tokens.size() < 2 || tokens[1].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        info_set_value(f_ptr->priority, tokens[1]);
    } else if (tokens[0] == "K") {
        // K:state:feat
        if (tokens.size() < 3)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        if (tokens[1].size() == 0 || tokens[2].size() == 0)
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;

        int i = 0;
        for (; i < MAX_FEAT_STATES; i++) {
            if (f_ptr->state[i].action == FF_FLAG_MAX)
                break;
        }

        if (i == MAX_FEAT_STATES)
            return PARSE_ERROR_GENERIC;

        if (tokens[1] == "DESTROYED")
            f_ptr->destroyed_tag = tokens[2];
        else {
            f_ptr->state[i].action = 0;
            if (!grab_one_feat_action(f_ptr, tokens[1], i))
                return PARSE_ERROR_INVALID_FLAG;

            f_ptr->state[i].result_tag = tokens[2];
        }
    } else
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;

    return PARSE_ERROR_NONE;
}

/*!
 * @brief 地形の汎用定義をタグを通じて取得する /
 * Initialize feature variables
 * @return エラーコード
 */
errr init_feat_variables(void)
{
    feat_none = f_tag_to_index_in_init("NONE");

    feat_floor = f_tag_to_index_in_init("FLOOR");
    feat_rune_protection = f_tag_to_index_in_init("RUNE_PROTECTION");
    feat_rune_explosion = f_tag_to_index_in_init("RUNE_EXPLOSION");
    feat_mirror = f_tag_to_index_in_init("MIRROR");

    feat_door[DOOR_DOOR].open = f_tag_to_index_in_init("OPEN_DOOR");
    feat_door[DOOR_DOOR].broken = f_tag_to_index_in_init("BROKEN_DOOR");
    feat_door[DOOR_DOOR].closed = f_tag_to_index_in_init("CLOSED_DOOR");

    /* Locked doors */
    FEAT_IDX i;
    for (i = 1; i < MAX_LJ_DOORS; i++) {
        int16_t door = f_tag_to_index(format("LOCKED_DOOR_%d", i));
        if (door < 0)
            break;
        feat_door[DOOR_DOOR].locked[i - 1] = door;
    }

    if (i == 1)
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    feat_door[DOOR_DOOR].num_locked = i - 1;

    /* Jammed doors */
    for (i = 0; i < MAX_LJ_DOORS; i++) {
        int16_t door = f_tag_to_index(format("JAMMED_DOOR_%d", i));
        if (door < 0)
            break;
        feat_door[DOOR_DOOR].jammed[i] = door;
    }

    if (!i)
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    feat_door[DOOR_DOOR].num_jammed = i;

    /* Glass doors */
    feat_door[DOOR_GLASS_DOOR].open = f_tag_to_index_in_init("OPEN_GLASS_DOOR");
    feat_door[DOOR_GLASS_DOOR].broken = f_tag_to_index_in_init("BROKEN_GLASS_DOOR");
    feat_door[DOOR_GLASS_DOOR].closed = f_tag_to_index_in_init("CLOSED_GLASS_DOOR");

    /* Locked glass doors */
    for (i = 1; i < MAX_LJ_DOORS; i++) {
        int16_t door = f_tag_to_index(format("LOCKED_GLASS_DOOR_%d", i));
        if (door < 0)
            break;
        feat_door[DOOR_GLASS_DOOR].locked[i - 1] = door;
    }

    if (i == 1)
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    feat_door[DOOR_GLASS_DOOR].num_locked = i - 1;

    /* Jammed glass doors */
    for (i = 0; i < MAX_LJ_DOORS; i++) {
        int16_t door = f_tag_to_index(format("JAMMED_GLASS_DOOR_%d", i));
        if (door < 0)
            break;
        feat_door[DOOR_GLASS_DOOR].jammed[i] = door;
    }

    if (!i)
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    feat_door[DOOR_GLASS_DOOR].num_jammed = i;

    /* Curtains */
    feat_door[DOOR_CURTAIN].open = f_tag_to_index_in_init("OPEN_CURTAIN");
    feat_door[DOOR_CURTAIN].broken = feat_door[DOOR_CURTAIN].open;
    feat_door[DOOR_CURTAIN].closed = f_tag_to_index_in_init("CLOSED_CURTAIN");
    feat_door[DOOR_CURTAIN].locked[0] = feat_door[DOOR_CURTAIN].closed;
    feat_door[DOOR_CURTAIN].num_locked = 1;
    feat_door[DOOR_CURTAIN].jammed[0] = feat_door[DOOR_CURTAIN].closed;
    feat_door[DOOR_CURTAIN].num_jammed = 1;

    /* Stairs */
    feat_up_stair = f_tag_to_index_in_init("UP_STAIR");
    feat_down_stair = f_tag_to_index_in_init("DOWN_STAIR");
    feat_entrance = f_tag_to_index_in_init("ENTRANCE");

    /* Normal traps */
    init_normal_traps();

    /* Special traps */
    feat_trap_open = f_tag_to_index_in_init("TRAP_OPEN");
    feat_trap_armageddon = f_tag_to_index_in_init("TRAP_ARMAGEDDON");
    feat_trap_piranha = f_tag_to_index_in_init("TRAP_PIRANHA");

    /* Rubble */
    feat_rubble = f_tag_to_index_in_init("RUBBLE");

    /* Seams */
    feat_magma_vein = f_tag_to_index_in_init("MAGMA_VEIN");
    feat_quartz_vein = f_tag_to_index_in_init("QUARTZ_VEIN");

    /* Walls */
    feat_granite = f_tag_to_index_in_init("GRANITE");
    feat_permanent = f_tag_to_index_in_init("PERMANENT");

    /* Glass floor */
    feat_glass_floor = f_tag_to_index_in_init("GLASS_FLOOR");

    /* Glass walls */
    feat_glass_wall = f_tag_to_index_in_init("GLASS_WALL");
    feat_permanent_glass_wall = f_tag_to_index_in_init("PERMANENT_GLASS_WALL");

    /* Pattern */
    feat_pattern_start = f_tag_to_index_in_init("PATTERN_START");
    feat_pattern_1 = f_tag_to_index_in_init("PATTERN_1");
    feat_pattern_2 = f_tag_to_index_in_init("PATTERN_2");
    feat_pattern_3 = f_tag_to_index_in_init("PATTERN_3");
    feat_pattern_4 = f_tag_to_index_in_init("PATTERN_4");
    feat_pattern_end = f_tag_to_index_in_init("PATTERN_END");
    feat_pattern_old = f_tag_to_index_in_init("PATTERN_OLD");
    feat_pattern_exit = f_tag_to_index_in_init("PATTERN_EXIT");
    feat_pattern_corrupted = f_tag_to_index_in_init("PATTERN_CORRUPTED");

    /* Various */
    feat_black_market = f_tag_to_index_in_init("BLACK_MARKET");
    feat_town = f_tag_to_index_in_init("TOWN");

    /* Terrains */
    feat_deep_water = f_tag_to_index_in_init("DEEP_WATER");
    feat_shallow_water = f_tag_to_index_in_init("SHALLOW_WATER");
    feat_deep_lava = f_tag_to_index_in_init("DEEP_LAVA");
    feat_shallow_lava = f_tag_to_index_in_init("SHALLOW_LAVA");
    feat_heavy_cold_zone = f_tag_to_index_in_init("HEAVY_COLD_ZONE");
    feat_cold_zone = f_tag_to_index_in_init("COLD_ZONE");
    feat_heavy_electrical_zone = f_tag_to_index_in_init("HEAVY_ELECTRICAL_ZONE");
    feat_electrical_zone = f_tag_to_index_in_init("ELECTRICAL_ZONE");
    feat_deep_acid_puddle = f_tag_to_index_in_init("DEEP_ACID_PUDDLE");
    feat_shallow_acid_puddle = f_tag_to_index_in_init("SHALLOW_ACID_PUDDLE");
    feat_deep_poisonous_puddle = f_tag_to_index_in_init("DEEP_POISONOUS_PUDDLE");
    feat_shallow_poisonous_puddle = f_tag_to_index_in_init("SHALLOW_POISONOUS_PUDDLE");
    feat_dirt = f_tag_to_index_in_init("DIRT");
    feat_grass = f_tag_to_index_in_init("GRASS");
    feat_flower = f_tag_to_index_in_init("FLOWER");
    feat_brake = f_tag_to_index_in_init("BRAKE");
    feat_tree = f_tag_to_index_in_init("TREE");
    feat_mountain = f_tag_to_index_in_init("MOUNTAIN");
    feat_swamp = f_tag_to_index_in_init("SWAMP");

    feat_undetected = f_tag_to_index_in_init("UNDETECTED");

    init_wilderness_terrains();
    return feat_tag_is_not_found ? PARSE_ERROR_UNDEFINED_TERRAIN_TAG : 0;
}

/*!
 * @brief 地形タグからIDを得る /
 * Convert a fake tag to a real feat index
 * @param str タグ文字列
 * @return 地形ID
 */
FEAT_IDX f_tag_to_index(std::string_view str)
{
    for (size_t i = 0; i < f_head.info_num; i++) {
        if (f_info[i].tag == str)
            return (FEAT_IDX)i;
    }

    return -1;
}

/*!
 * @brief 地形タグからIDを得る /
 * Initialize quest array
 * @return 地形ID
 */
int16_t f_tag_to_index_in_init(concptr str)
{
    FEAT_IDX feat = f_tag_to_index(str);

    if (feat < 0)
        feat_tag_is_not_found = true;

    return feat;
}

/*!
 * @brief 地形タグからIDを得る /
 * Search for real index corresponding to this fake tag
 * @param feat タグ文字列のオフセット
 * @return 地形ID。該当がないなら-1
 */
static FEAT_IDX search_real_feat(std::string feat)
{
    if (feat.empty()) {
        return -1;
    }

    for (FEAT_IDX i = 0; i < f_head.info_num; i++) {
        if (feat.compare(f_info[i].tag) == 0) {
            return i;
        }
    }

    msg_format(_("未定義のタグ '%s'。", "%s is undefined."), feat.c_str());
    return -1;
}

/*!
 * @brief 地形情報の各種タグからIDへ変換して結果を収める /
 * Retouch fake tags of f_info
 * @param head ヘッダ構造体
 */
void retouch_f_info(angband_header *head)
{
    for (int i = 0; i < head->info_num; i++) {
        feature_type *f_ptr = &f_info[i];
        FEAT_IDX k = search_real_feat(f_ptr->mimic_tag);
        f_ptr->mimic = k < 0 ? f_ptr->mimic : k;
        k = search_real_feat(f_ptr->destroyed_tag);
        f_ptr->destroyed = k < 0 ? f_ptr->destroyed : k;
        for (FEAT_IDX j = 0; j < MAX_FEAT_STATES; j++) {
            k = search_real_feat(f_ptr->state[j].result_tag);
            f_ptr->state[j].result = k < 0 ? f_ptr->state[j].result : k;
        }
    }
}
