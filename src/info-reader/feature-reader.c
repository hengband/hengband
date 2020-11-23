#include "info-reader/feature-reader.h"
#include "floor/wild.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "info-reader/feature-info-tokens-table.h"
#include "info-reader/parse-error-types.h"
#include "main/angband-headers.h"
#include "room/door-definition.h"
#include "term/gameterm.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*! 地形タグ情報から地形IDを得られなかった場合にTRUEを返す */
static bool feat_tag_is_not_found = FALSE;

/*!
 * @brief テキストトークンを走査してフラグを一つ得る（地形情報向け） /
 * Grab one flag in an feature_type from a textual string
 * @param f_ptr 地形情報を保管する先の構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @return エラーコード
 */
static errr grab_one_feat_flag(feature_type *f_ptr, concptr what)
{
    for (int i = 0; i < FF_FLAG_MAX; i++) {
        if (streq(what, f_info_flags[i])) {
            add_flag(f_ptr->flags, i);
            return 0;
        }
    }

    msg_format(_("未知の地形フラグ '%s'。", "Unknown feature flag '%s'."), what);
    return PARSE_ERROR_GENERIC;
}

/*!
 * @brief テキストトークンを走査してフラグ(ステート)を一つ得る（地形情報向け2） /
 * Grab an action in an feature_type from a textual string
 * @param f_ptr 地形情報を保管する先の構造体参照ポインタ
 * @param what 参照元の文字列ポインタ
 * @param count ステートの保存先ID
 * @return エラーコード
 */
static errr grab_one_feat_action(feature_type *f_ptr, concptr what, int count)
{
    for (FF_FLAGS_IDX i = 0; i < FF_FLAG_MAX; i++) {
        if (streq(what, f_info_flags[i])) {
            f_ptr->state[count].action = i;
            return 0;
        }
    }

    msg_format(_("未知の地形アクション '%s'。", "Unknown feature action '%s'."), what);
    return PARSE_ERROR_GENERIC;
}

/*!
 * @brief 地形情報(f_info)のパース関数 /
 * Initialize the "f_info" array, by parsing an ascii "template" file
 * @param buf テキスト列
 * @param head ヘッダ構造体
 * @return エラーコード
 */
errr parse_f_info(char *buf, angband_header *head)
{
    static feature_type *f_ptr = NULL;
    int i;
    char *s, *t;
    if (buf[0] == 'N') {
        s = angband_strchr(buf + 2, ':');

        if (s) {
            *s++ = '\0';
        }

        i = atoi(buf + 2);
        if (i <= error_idx)
            return 4;
        if (i >= head->info_num)
            return 2;

        error_idx = i;
        f_ptr = &f_info[i];
        if (s) {
            if (!add_tag(&f_ptr->tag, head, s))
                return 7;
        }

        f_ptr->mimic = (FEAT_IDX)i;
        f_ptr->destroyed = (FEAT_IDX)i;
        for (i = 0; i < MAX_FEAT_STATES; i++)
            f_ptr->state[i].action = FF_FLAG_MAX;
    } else if (!f_ptr) {
        return 3;
    }
#ifdef JP
    else if (buf[0] == 'J') {
        if (!add_name(&f_ptr->name, head, buf + 2))
            return 7;
    } else if (buf[0] == 'E') {
    }
#else
    else if (buf[0] == 'J') {
    } else if (buf[0] == 'E') {
        s = buf + 2;
        if (!add_name(&f_ptr->name, head, s))
            return 7;
    }
#endif
    else if (buf[0] == 'M') {
        STR_OFFSET offset;
        if (!add_tag(&offset, head, buf + 2))
            return PARSE_ERROR_OUT_OF_MEMORY;

        f_ptr->mimic_tag = offset;
    } else if (buf[0] == 'G') {
        int j;
        byte s_attr;
        char char_tmp[F_LIT_MAX];
        if (buf[1] != ':')
            return 1;
        if (!buf[2])
            return 1;
        if (buf[3] != ':')
            return 1;
        if (!buf[4])
            return 1;

        char_tmp[F_LIT_STANDARD] = buf[2];
        s_attr = color_char_to_attr(buf[4]);
        if (s_attr > 127)
            return 1;

        f_ptr->d_attr[F_LIT_STANDARD] = s_attr;
        f_ptr->d_char[F_LIT_STANDARD] = char_tmp[F_LIT_STANDARD];
        if (buf[5] == ':') {
            apply_default_feat_lighting(f_ptr->d_attr, f_ptr->d_char);
            if (!streq(buf + 6, "LIT")) {
                char attr_lite_tmp[F_LIT_MAX - F_LIT_NS_BEGIN];

                if ((F_LIT_MAX - F_LIT_NS_BEGIN) * 2
                    != sscanf(buf + 6, "%c:%c:%c:%c", &char_tmp[F_LIT_LITE], &attr_lite_tmp[F_LIT_LITE - F_LIT_NS_BEGIN], &char_tmp[F_LIT_DARK],
                        &attr_lite_tmp[F_LIT_DARK - F_LIT_NS_BEGIN]))
                    return 1;
                if (buf[F_LIT_MAX * 4 + 1])
                    return 1;

                for (j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
                    switch (attr_lite_tmp[j - F_LIT_NS_BEGIN]) {
                    case '*':
                        /* Use default lighting */
                        break;
                    case '-':
                        /* No lighting support */
                        f_ptr->d_attr[j] = f_ptr->d_attr[F_LIT_STANDARD];
                        break;
                    default:
                        /* Extract the color */
                        f_ptr->d_attr[j] = color_char_to_attr(attr_lite_tmp[j - F_LIT_NS_BEGIN]);
                        if (f_ptr->d_attr[j] > 127)
                            return 1;
                        break;
                    }
                    f_ptr->d_char[j] = char_tmp[j];
                }
            }
        } else if (!buf[5]) {
            for (j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
                f_ptr->d_attr[j] = s_attr;
                f_ptr->d_char[j] = char_tmp[F_LIT_STANDARD];
            }
        } else
            return 1;
    } else if (buf[0] == 'F') {
        for (s = buf + 2; *s;) {
            /* loop */
            for (t = s; *t && (*t != ' ') && (*t != '|'); ++t)
                ;

            if (*t) {
                *t++ = '\0';
                while (*t == ' ' || *t == '|')
                    t++;
            }

            if (1 == sscanf(s, "SUBTYPE_%d", &i)) {
                f_ptr->subtype = (FEAT_SUBTYPE)i;
                s = t;

                continue;
            }

            if (1 == sscanf(s, "POWER_%d", &i)) {
                f_ptr->power = (FEAT_POWER)i;
                s = t;
                continue;
            }

            if (0 != grab_one_feat_flag(f_ptr, s))
                return (PARSE_ERROR_INVALID_FLAG);

            s = t;
        }
    } else if (buf[0] == 'W') {
        int priority;
        if (1 != sscanf(buf + 2, "%d", &priority))
            return (PARSE_ERROR_GENERIC);
        f_ptr->priority = (FEAT_PRIORITY)priority;
    } else if (buf[0] == 'K') {
        STR_OFFSET offset;
        for (i = 0; i < MAX_FEAT_STATES; i++)
            if (f_ptr->state[i].action == FF_FLAG_MAX)
                break;

        if (i == MAX_FEAT_STATES)
            return PARSE_ERROR_GENERIC;

        /* loop */
        for (s = t = buf + 2; *t && (*t != ':'); t++)
            ;

        if (*t == ':')
            *t++ = '\0';

        if (streq(s, "DESTROYED")) {
            if (!add_tag(&offset, head, t))
                return PARSE_ERROR_OUT_OF_MEMORY;

            f_ptr->destroyed_tag = offset;
        } else {
            f_ptr->state[i].action = 0;
            if (0 != grab_one_feat_action(f_ptr, s, i))
                return PARSE_ERROR_INVALID_FLAG;
            if (!add_tag(&offset, head, t))
                return PARSE_ERROR_OUT_OF_MEMORY;

            f_ptr->state[i].result_tag = offset;
        }
    } else {
        return 6;
    }

    return 0;
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
    feat_glyph = f_tag_to_index_in_init("GLYPH");
    feat_explosive_rune = f_tag_to_index_in_init("EXPLOSIVE_RUNE");
    feat_mirror = f_tag_to_index_in_init("MIRROR");

    feat_door[DOOR_DOOR].open = f_tag_to_index_in_init("OPEN_DOOR");
    feat_door[DOOR_DOOR].broken = f_tag_to_index_in_init("BROKEN_DOOR");
    feat_door[DOOR_DOOR].closed = f_tag_to_index_in_init("CLOSED_DOOR");

    /* Locked doors */
    FEAT_IDX i;
    for (i = 1; i < MAX_LJ_DOORS; i++) {
        s16b door = f_tag_to_index(format("LOCKED_DOOR_%d", i));
        if (door < 0)
            break;
        feat_door[DOOR_DOOR].locked[i - 1] = door;
    }

    if (i == 1)
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    feat_door[DOOR_DOOR].num_locked = i - 1;

    /* Jammed doors */
    for (i = 0; i < MAX_LJ_DOORS; i++) {
        s16b door = f_tag_to_index(format("JAMMED_DOOR_%d", i));
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
        s16b door = f_tag_to_index(format("LOCKED_GLASS_DOOR_%d", i));
        if (door < 0)
            break;
        feat_door[DOOR_GLASS_DOOR].locked[i - 1] = door;
    }

    if (i == 1)
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    feat_door[DOOR_GLASS_DOOR].num_locked = i - 1;

    /* Jammed glass doors */
    for (i = 0; i < MAX_LJ_DOORS; i++) {
        s16b door = f_tag_to_index(format("JAMMED_GLASS_DOOR_%d", i));
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
s16b f_tag_to_index(concptr str)
{
    for (u16b i = 0; i < f_head.info_num; i++) {
        if (streq(f_tag + f_info[i].tag, str)) {
            return (s16b)i;
        }
    }

    return -1;
}

/*!
 * @brief 地形タグからIDを得る /
 * Initialize quest array
 * @return 地形ID
 */
s16b f_tag_to_index_in_init(concptr str)
{
    FEAT_IDX feat = f_tag_to_index(str);

    if (feat < 0)
        feat_tag_is_not_found = TRUE;

    return feat;
}

/*!
 * @brief 地形タグからIDを得る /
 * Search for real index corresponding to this fake tag
 * @param feat タグ文字列のオフセット
 * @return 地形ID。該当がないなら-1
 */
static FEAT_IDX search_real_feat(STR_OFFSET feat)
{
    if (feat <= 0) {
        return -1;
    }

    for (FEAT_IDX i = 0; i < f_head.info_num; i++) {
        if (feat == f_info[i].tag) {
            return i;
        }
    }

    msg_format(_("未定義のタグ '%s'。", "%s is undefined."), f_tag + feat);
    return -1;
}

/*!
 * @brief 地形情報の各種タグからIDへ変換して結果を収める /
 * Retouch fake tags of f_info
 * @param head ヘッダ構造体
 * @return なし
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
