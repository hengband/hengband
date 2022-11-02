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
    if (r_ptr->visual_flags.has(MonsterVisualType::CLEAR_COLOR)) {
        return _("透明な", "Clear");
    }

    if (r_ptr->visual_flags.has(MonsterVisualType::MULTI_COLOR)) {
        return _("万色の", "Multi");
    }

    if (r_ptr->visual_flags.has(MonsterVisualType::RANDOM_COLOR)) {
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

SpoilerOutputResultType spoil_mon_desc(concptr fname, std::function<bool(const monster_race *)> filter_monster)
{
    PlayerType dummy;
    uint16_t why = 2;
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
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    char title[200];
    put_version(title);

    fprintf(spoiler_file, "Monster Spoilers for %s\n", title);
    fprintf(spoiler_file, "------------------------------------------\n\n");
    fprintf(spoiler_file, "%-45.45s%4s %4s %4s %7s %7s  %19.19s\n", "Name", "Lev", "Rar", "Spd", "Hp", "Ac", "Visual Info");
    fprintf(spoiler_file, "%-45.45s%4s %4s %4s %7s %7s  %4.19s\n",
        "---------------------------------------------"
        "----"
        "----------",
        "---", "---", "---", "-----", "-----", "-------------------");

    std::vector<MonsterRaceId> who;
    for (const auto &[r_idx, r_ref] : monraces_info) {
        if (MonsterRace(r_ref.idx).is_valid() && !r_ref.name.empty()) {
            who.push_back(r_ref.idx);
        }
    }

    ang_sort(&dummy, who.data(), &why, who.size(), ang_sort_comp_hook, ang_sort_swap_hook);
    for (auto r_idx : who) {
        auto *r_ptr = &monraces_info[r_idx];
        if (filter_monster && !filter_monster(r_ptr)) {
            continue;
        }

        if (any_bits(r_ptr->flags7, RF7_KAGE)) {
            continue;
        }

        const auto name = str_separate(r_ptr->name, 40);
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            sprintf(nam, "[U] %s", name.front().data());
        } else if (r_ptr->population_flags.has(MonsterPopulationType::NAZGUL)) {
            sprintf(nam, "[N] %s", name.front().data());
        } else {
            sprintf(nam, _("    %s", "The %s"), name.front().data());
        }

        sprintf(lev, "%d", (int)r_ptr->level);
        sprintf(rar, "%d", (int)r_ptr->rarity);
        sprintf(spd, "%+d", r_ptr->speed - STANDARD_SPEED);
        sprintf(ac, "%d", r_ptr->ac);
        if (any_bits(r_ptr->flags1, RF1_FORCE_MAXHP) || (r_ptr->hside == 1)) {
            sprintf(hp, "%d", r_ptr->hdice * r_ptr->hside);
        } else {
            sprintf(hp, "%dd%d", r_ptr->hdice, r_ptr->hside);
        }

        sprintf(symbol, "%ld", (long)(r_ptr->mexp));
        sprintf(symbol, "%s '%c'", attr_to_text(r_ptr), r_ptr->d_char);
        fprintf(spoiler_file, "%-45.45s%4s %4s %4s %7s %7s  %19.19s\n", nam, lev, rar, spd, hp, ac, symbol);

        for (auto i = 1U; i < name.size(); ++i) {
            fprintf(spoiler_file, "    %s\n", name[i].data());
        }
    }

    fprintf(spoiler_file, "\n");
    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? SpoilerOutputResultType::FILE_CLOSE_FAILED
                                                                : SpoilerOutputResultType::SUCCESSFUL;
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
SpoilerOutputResultType spoil_mon_info(concptr fname)
{
    PlayerType dummy;
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);
    spoiler_file = angband_fopen(buf, "w");
    if (!spoiler_file) {
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    char title[200];
    put_version(title);
    sprintf(buf, "Monster Spoilers for %s\n", title);
    spoil_out(buf);
    spoil_out("------------------------------------------\n\n");

    std::vector<MonsterRaceId> who;
    for (const auto &[r_idx, r_ref] : monraces_info) {
        if (MonsterRace(r_ref.idx).is_valid() && !r_ref.name.empty()) {
            who.push_back(r_ref.idx);
        }
    }

    uint16_t why = 2;
    ang_sort(&dummy, who.data(), &why, who.size(), ang_sort_comp_hook, ang_sort_swap_hook);
    for (auto r_idx : who) {
        auto *r_ptr = &monraces_info[r_idx];
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            spoil_out("[U] ");
        } else if (r_ptr->population_flags.has(MonsterPopulationType::NAZGUL)) {
            spoil_out("[N] ");
        }

        sprintf(buf, _("%s/%s  (", "%s%s ("), r_ptr->name.data(), _(r_ptr->E_name.data(), "")); /* ---)--- */
        spoil_out(buf);
        spoil_out(attr_to_text(r_ptr));
        sprintf(buf, " '%c')\n", r_ptr->d_char);
        spoil_out(buf);
        sprintf(buf, "=== ");
        spoil_out(buf);
        sprintf(buf, "Num:%d  ", enum2i(r_idx));
        spoil_out(buf);
        sprintf(buf, "Lev:%d  ", (int)r_ptr->level);
        spoil_out(buf);
        sprintf(buf, "Rar:%d  ", r_ptr->rarity);
        spoil_out(buf);
        sprintf(buf, "%+d", r_ptr->speed - STANDARD_SPEED);
        spoil_out(buf);
        if (any_bits(r_ptr->flags1, RF1_FORCE_MAXHP) || (r_ptr->hside == 1)) {
            sprintf(buf, "Hp:%d  ", r_ptr->hdice * r_ptr->hside);
        } else {
            sprintf(buf, "Hp:%dd%d  ", r_ptr->hdice, r_ptr->hside);
        }

        spoil_out(buf);
        sprintf(buf, "Ac:%d  ", r_ptr->ac);
        spoil_out(buf);
        sprintf(buf, "Exp:%ld\n", (long)(r_ptr->mexp));
        spoil_out(buf);
        output_monster_spoiler(r_idx, roff_func);
        spoil_out(nullptr);
    }

    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? SpoilerOutputResultType::FILE_CLOSE_FAILED
                                                                : SpoilerOutputResultType::SUCCESSFUL;
}
