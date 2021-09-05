#include "wizard/monster-info-spoiler.h"
#include "io/files-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-flags8.h"
#include "system/angband-version.h"
#include "system/monster-race-definition.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/sort.h"
#include "util/string-processor.h"
#include "view/display-lore.h"
#include "view/display-messages.h"

/*!
 * @brief シンボル職の記述名を返す /
 * Extract a textual representation of an attribute
 * @param r_ptr モンスター種族の構造体ポインタ
 * @return シンボル職の記述名
 */
static concptr attr_to_text(monster_race *r_ptr)
{
    if (any_bits(r_ptr->flags1, RF1_ATTR_CLEAR)) {
        return _("透明な", "Clear");
    }

    if (any_bits(r_ptr->flags1, RF1_ATTR_MULTI)) {
        return _("万色の", "Multi");
    }

    if (any_bits(r_ptr->flags1, RF1_ATTR_SEMIRAND)) {
        return _("準ランダムな", "S.Rand");
    }

    switch (r_ptr->d_attr) {
    case TERM_DARK:
        return _("黒い", "Dark");
    case TERM_WHITE:
        return _("白い", "White");
    case TERM_SLATE:
        return _("青灰色の", "Slate");
    case TERM_ORANGE:
        return _("オレンジの", "Orange");
    case TERM_RED:
        return _("赤い", "Red");
    case TERM_GREEN:
        return _("緑の", "Green");
    case TERM_BLUE:
        return _("青い", "Blue");
    case TERM_UMBER:
        return _("琥珀色の", "Umber");
    case TERM_L_DARK:
        return _("灰色の", "L.Dark");
    case TERM_L_WHITE:
        return _("明るい青灰色の", "L.Slate");
    case TERM_VIOLET:
        return _("紫の", "Violet");
    case TERM_YELLOW:
        return _("黄色の", "Yellow");
    case TERM_L_RED:
        return _("明るい赤の", "L.Red");
    case TERM_L_GREEN:
        return _("明るい緑の", "L.Green");
    case TERM_L_BLUE:
        return _("明るい青の", "L.Blue");
    case TERM_L_UMBER:
        return _("明るい琥珀色の", "L.Umber");
    }

    return _("変な色の", "Icky");
}

spoiler_output_status spoil_mon_desc(concptr fname, std::function<bool(const monster_race *)> filter_monster)
{
    player_type dummy;
    uint16_t why = 2;
    MONRACE_IDX *who;
    char buf[1024];
    char nam[MAX_MONSTER_NAME + 10]; // ユニークには[U] が付くので少し伸ばす
    char lev[80];
    char rar[80];
    char spd[80];
    char ac[80];
    char hp[80];
    char symbol[80];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);
    spoiler_file = angband_fopen(buf, "w");
    if (!spoiler_file) {
        return spoiler_output_status::SPOILER_OUTPUT_FAIL_FOPEN;
    }

    char title[200];
    put_version(title);

    C_MAKE(who, max_r_idx, MONRACE_IDX);
    fprintf(spoiler_file, "Monster Spoilers for %s\n", title);
    fprintf(spoiler_file, "------------------------------------------\n\n");
    fprintf(spoiler_file, "%-45.45s%4s %4s %4s %7s %7s  %19.19s\n", "Name", "Lev", "Rar", "Spd", "Hp", "Ac", "Visual Info");
    fprintf(spoiler_file, "%-45.45s%4s %4s %4s %7s %7s  %4.19s\n",
        "---------------------------------------------"
        "----"
        "----------",
        "---", "---", "---", "-----", "-----", "-------------------");

    int n = 0;
    for (auto i = 1; i < max_r_idx; i++) {
        monster_race *r_ptr = &r_info[i];
        if (!r_ptr->name.empty())
            who[n++] = (int16_t)i;
    }

    ang_sort(&dummy, who, &why, n, ang_sort_comp_hook, ang_sort_swap_hook);
    for (auto i = 0; i < n; i++) {
        monster_race *r_ptr = &r_info[who[i]];
        concptr name = r_ptr->name.c_str();
        if (filter_monster && !filter_monster(r_ptr)) {
            continue;
        }

        char name_buf[41];
        angband_strcpy(name_buf, name, sizeof(name_buf));
        name += strlen(name_buf);

        if (any_bits(r_ptr->flags7, RF7_KAGE)) {
            continue;
        }

        if (any_bits(r_ptr->flags1, RF1_UNIQUE)) {
            sprintf(nam, "[U] %s", name_buf);
        } else if (any_bits(r_ptr->flags7, RF7_NAZGUL)) {
            sprintf(nam, "[N] %s", name_buf);
        } else {
            sprintf(nam, _("    %s", "The %s"), name_buf);
        }

        sprintf(lev, "%d", (int)r_ptr->level);
        sprintf(rar, "%d", (int)r_ptr->rarity);
        if (r_ptr->speed >= 110) {
            sprintf(spd, "+%d", (r_ptr->speed - 110));
        } else {
            sprintf(spd, "-%d", (110 - r_ptr->speed));
        }

        sprintf(ac, "%d", r_ptr->ac);
        if (any_bits(r_ptr->flags1, RF1_FORCE_MAXHP) || (r_ptr->hside == 1)) {
            sprintf(hp, "%d", r_ptr->hdice * r_ptr->hside);
        } else {
            sprintf(hp, "%dd%d", r_ptr->hdice, r_ptr->hside);
        }

        sprintf(symbol, "%ld", (long)(r_ptr->mexp));
        sprintf(symbol, "%s '%c'", attr_to_text(r_ptr), r_ptr->d_char);
        fprintf(spoiler_file, "%-45.45s%4s %4s %4s %7s %7s  %19.19s\n", nam, lev, rar, spd, hp, ac, symbol);

        while (*name != '\0') {
            angband_strcpy(name_buf, name, sizeof(name_buf));
            name += strlen(name_buf);
            fprintf(spoiler_file, "    %s\n", name_buf);
        }
    }

    fprintf(spoiler_file, "\n");
    C_KILL(who, max_r_idx, int16_t);
    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? spoiler_output_status::SPOILER_OUTPUT_FAIL_FCLOSE
                                                                : spoiler_output_status::SPOILER_OUTPUT_SUCCESS;
}

/*!
 * @brief 関数ポインタ用の出力関数 /
 * Hook function used in spoil_mon_info()
 * @param attr 未使用
 * @param str 文字列参照ポインタ
 */
static void roff_func(TERM_COLOR attr, concptr str)
{
    (void)attr;
    spoil_out(str);
}

/*!
 * @brief モンスター詳細情報をスポイラー出力するメインルーチン /
 * Create a spoiler file for monsters (-SHAWN-)
 * @param fname ファイル名
 */
spoiler_output_status spoil_mon_info(concptr fname)
{
    player_type dummy;
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);
    spoiler_file = angband_fopen(buf, "w");
    if (!spoiler_file) {
        return spoiler_output_status::SPOILER_OUTPUT_FAIL_FOPEN;
    }

    char title[200];
    put_version(title);
    sprintf(buf, "Monster Spoilers for %s\n", title);
    spoil_out(buf);
    spoil_out("------------------------------------------\n\n");

    MONRACE_IDX *who;
    C_MAKE(who, max_r_idx, MONRACE_IDX);
    int n = 0;
    for (int i = 1; i < max_r_idx; i++) {
        monster_race *r_ptr = &r_info[i];
        if (!r_ptr->name.empty())
            who[n++] = (int16_t)i;
    }

    uint16_t why = 2;
    ang_sort(&dummy, who, &why, n, ang_sort_comp_hook, ang_sort_swap_hook);
    for (int i = 0; i < n; i++) {
        monster_race *r_ptr = &r_info[who[i]];
        BIT_FLAGS flags1 = r_ptr->flags1;
        if (any_bits(flags1, RF1_UNIQUE)) {
            spoil_out("[U] ");
        } else if (any_bits(r_ptr->flags7, RF7_NAZGUL)) {
            spoil_out("[N] ");
        }

        sprintf(buf, _("%s/%s  (", "%s%s ("), r_ptr->name.c_str(), _(r_ptr->E_name.c_str(), "")); /* ---)--- */
        spoil_out(buf);
        spoil_out(attr_to_text(r_ptr));
        sprintf(buf, " '%c')\n", r_ptr->d_char);
        spoil_out(buf);
        sprintf(buf, "=== ");
        spoil_out(buf);
        sprintf(buf, "Num:%d  ", who[i]);
        spoil_out(buf);
        sprintf(buf, "Lev:%d  ", (int)r_ptr->level);
        spoil_out(buf);
        sprintf(buf, "Rar:%d  ", r_ptr->rarity);
        spoil_out(buf);
        if (r_ptr->speed >= 110) {
            sprintf(buf, "Spd:+%d  ", (r_ptr->speed - 110));
        } else {
            sprintf(buf, "Spd:-%d  ", (110 - r_ptr->speed));
        }

        spoil_out(buf);
        if (any_bits(flags1, RF1_FORCE_MAXHP) || (r_ptr->hside == 1)) {
            sprintf(buf, "Hp:%d  ", r_ptr->hdice * r_ptr->hside);
        } else {
            sprintf(buf, "Hp:%dd%d  ", r_ptr->hdice, r_ptr->hside);
        }

        spoil_out(buf);
        sprintf(buf, "Ac:%d  ", r_ptr->ac);
        spoil_out(buf);
        sprintf(buf, "Exp:%ld\n", (long)(r_ptr->mexp));
        spoil_out(buf);
        output_monster_spoiler(who[i], roff_func);
        spoil_out(nullptr);
    }

    C_KILL(who, max_r_idx, int16_t);
    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? spoiler_output_status::SPOILER_OUTPUT_FAIL_FCLOSE
                                                                : spoiler_output_status::SPOILER_OUTPUT_SUCCESS;
}
