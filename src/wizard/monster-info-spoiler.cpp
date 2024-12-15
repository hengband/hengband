#include "wizard/monster-info-spoiler.h"
#include "io/files-util.h"
#include "system/angband-system.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include "view/display-lore.h"
#include "view/display-messages.h"
#include "wizard/spoiler-util.h"

/*!
 * @brief シンボル職の記述名を返す
 * @param monrace モンスター種族の構造体ポインタ
 * @return シンボル職の記述名
 * @todo MonraceDefinition のオブジェクトメソッドへ繰り込む
 */
static std::string attr_to_text(const MonraceDefinition &monrace)
{
    if (monrace.visual_flags.has(MonsterVisualType::CLEAR_COLOR)) {
        return _("透明な", "Clear");
    }

    if (monrace.visual_flags.has(MonsterVisualType::MULTI_COLOR)) {
        return _("万色の", "Multi");
    }

    if (monrace.visual_flags.has(MonsterVisualType::RANDOM_COLOR)) {
        return _("準ランダムな", "S.Rand");
    }

    switch (monrace.symbol_definition.color) {
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

SpoilerOutputResultType spoil_mon_desc(std::string_view filename, std::function<bool(const MonraceDefinition *)> filter_monster)
{
    const auto path = path_build(ANGBAND_DIR_USER, filename);
    std::ofstream ofs(path);
    if (!ofs) {
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    constexpr auto fmt_version = "Monster Spoilers for %s\n";
    ofs << format(fmt_version, AngbandSystem::get_instance().build_version_expression(VersionExpression::FULL).data());
    ofs << "------------------------------------------\n\n";
    ofs << format("%-45.45s%4s %4s %4s %7s %7s  %19.19s\n", "Name", "Lev", "Rar", "Spd", "Hp", "Ac", "Visual Info");
    ofs << format("%-45.45s%4s %4s %4s %7s %7s  %4.19s\n",
        "---------------------------------------------"
        "----"
        "----------",
        "---", "---", "---", "-----", "-----", "-------------------");

    const auto &monraces = MonraceList::get_instance();
    std::vector<MonraceId> monrace_ids = monraces.get_valid_monrace_ids();
    std::stable_sort(monrace_ids.begin(), monrace_ids.end(), [&monraces](auto x, auto y) { return monraces.order(x, y); });
    for (auto monrace_id : monrace_ids) {
        const auto &monrace = monraces.get_monrace(monrace_id);
        if (filter_monster && !filter_monster(&monrace)) {
            continue;
        }

        if (monrace.misc_flags.has(MonsterMiscType::KAGE)) {
            continue;
        }

        const auto name = str_separate(monrace.name, 40);
        std::string nam;
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            nam = "[U] ";
        } else if (monrace.population_flags.has(MonsterPopulationType::NAZGUL)) {
            nam = "[N] ";
        } else {
            nam = _("    ", "The ");
        }
        nam.append(name.front());

        const auto lev = format("%d", monrace.level);
        const auto rar = format("%d", (int)monrace.rarity);
        const auto spd = format("%+d", monrace.speed - STANDARD_SPEED);
        const auto ac = format("%d", monrace.ac);
        std::string hp;
        if (monrace.misc_flags.has(MonsterMiscType::FORCE_MAXHP) || (monrace.hit_dice.sides == 1)) {
            hp = format("%d", monrace.hit_dice.maxroll());
        } else {
            hp = monrace.hit_dice.to_string();
        }

        const auto symbol = format("%s '%c'", attr_to_text(monrace).data(), monrace.symbol_definition.character);
        ofs << format("%-45.45s%4s %4s %4s %7s %7s  %19.19s\n",
            nam.data(), lev.data(), rar.data(), spd.data(), hp.data(),
            ac.data(), symbol.data());

        for (auto i = 1U; i < name.size(); ++i) {
            ofs << format("    %s\n", name[i].data());
        }
    }

    ofs << '\n';
    return ofs.good() ? SpoilerOutputResultType::SUCCESSFUL : SpoilerOutputResultType::FILE_CLOSE_FAILED;
}

/*!
 * @brief 関数ポインタ用の出力関数 /
 * Hook function used in spoil_mon_info()
 * @param attr 未使用
 * @param str 文字列参照ポインタ
 */
static void roff_func(TERM_COLOR attr, std::string_view str)
{
    (void)attr;
    spoil_out(str);
}

/*!
 * @brief モンスター詳細情報をスポイラー出力するメインルーチン /
 * Create a spoiler file for monsters (-SHAWN-)
 * @param fname ファイル名
 */
SpoilerOutputResultType spoil_mon_info()
{
    const auto path = path_build(ANGBAND_DIR_USER, "mon-info.txt");
    spoiler_file = angband_fopen(path, FileOpenMode::WRITE);
    if (!spoiler_file) {
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    constexpr auto fmt_version = "Monster Spoilers for %s\n";
    spoil_out(format(fmt_version, AngbandSystem::get_instance().build_version_expression(VersionExpression::FULL).data()));
    spoil_out("------------------------------------------\n\n");

    const auto &monraces = MonraceList::get_instance();
    std::vector<MonraceId> monrace_ids = monraces.get_valid_monrace_ids();
    std::stable_sort(monrace_ids.begin(), monrace_ids.end(), [&monraces](auto x, auto y) { return monraces.order(x, y); });
    for (auto monrace_id : monrace_ids) {
        const auto &monrace = monraces.get_monrace(monrace_id);
        if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
            spoil_out("[U] ");
        } else if (monrace.population_flags.has(MonsterPopulationType::NAZGUL)) {
            spoil_out("[N] ");
        }

        spoil_out(format(_("%s/%s  (", "%s%s ("), monrace.name.data(), _(monrace.name.en_string().data(), ""))); /* ---)--- */
        spoil_out(attr_to_text(monrace));
        spoil_out(format(" '%c')\n", monrace.symbol_definition.character));
        spoil_out("=== ");
        spoil_out(format("Num:%d  ", enum2i(monrace_id)));
        spoil_out(format("Lev:%d  ", (int)monrace.level));
        spoil_out(format("Rar:%d  ", monrace.rarity));
        spoil_out(format("Spd:%+d  ", monrace.speed - STANDARD_SPEED));
        if (monrace.misc_flags.has(MonsterMiscType::FORCE_MAXHP) || (monrace.hit_dice.sides == 1)) {
            spoil_out(format("Hp:%d  ", monrace.hit_dice.maxroll()));
        } else {
            spoil_out(format("Hp:%s  ", monrace.hit_dice.to_string().data()));
        }

        spoil_out(format("Ac:%d  ", monrace.ac));
        spoil_out(format("Exp:%ld\n", (long)(monrace.mexp)));
        output_monster_spoiler(monrace_id, roff_func);
        spoil_out({}, true);
    }

    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? SpoilerOutputResultType::FILE_CLOSE_FAILED
                                                                : SpoilerOutputResultType::SUCCESSFUL;
}
