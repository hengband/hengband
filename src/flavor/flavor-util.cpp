#include "flavor/flavor-util.h"
#include "flavor/flag-inscriptions-table.h"
#include "object-enchant/tr-flags.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "sv-definition/sv-food-types.h"
#include "system/artifact-type-definition.h"
#include "system/item-entity.h"
#include "util/string-processor.h"
#include <sstream>

static bool has_lite_flag(const TrFlags &flags)
{
    return flags.has(TR_LITE_1) || flags.has(TR_LITE_2) || flags.has(TR_LITE_3);
}

static bool has_dark_flag(const TrFlags &flags)
{
    return flags.has(TR_LITE_M1) || flags.has(TR_LITE_M2) || flags.has(TR_LITE_M3);
}

/*!
 * @brief get_inscriptionのサブセットとしてアイテムの特性フラグを表す文字列を返す
 * @param fi_vec 参照する特性表示記号テーブル
 * @param flags 対応するアイテムの特性フラグ
 * @param is_kanji trueならば漢字記述/falseならば英語記述
 * @return アイテムの特性フラグを表す文字列
 */
static std::string inscribe_flags_aux(const std::vector<flag_insc_table> &fi_vec, const TrFlags &flags, [[maybe_unused]] bool is_kanji)
{
    std::stringstream ss;

    for (const auto &fi : fi_vec) {
        if (flags.has(fi.flag) && (!fi.except_flag || flags.has_not(*fi.except_flag))) {
            const auto flag_str = _(is_kanji ? fi.japanese : fi.english, fi.english);
            ss << flag_str;
        }
    }

    return ss.str();
}

/*!
 * @brief オブジェクトの特性表示記号テーブル1つに従いオブジェクトの特性フラグ配列に1つでも該当の特性があるかを返す / Special variation of has_flag for
 * auto-inscription
 * @param fi_vec 参照する特性表示記号テーブル
 * @param flags 対応するオブジェクトのフラグ文字列
 * @return 1つでも該当の特性があったらTRUEを返す。
 */
static bool has_flag_of(const std::vector<flag_insc_table> &fi_vec, const TrFlags &flags)
{
    for (const auto &fi : fi_vec) {
        if (flags.has(fi.flag) && (!fi.except_flag || flags.has_not(*fi.except_flag))) {
            return true;
        }
    }

    return false;
}

/*!
 * @brief アイテムの特性短縮表記をまとめて提示する。
 * @param item 特性短縮表記を得たいアイテムの参照
 * @param is_kanji trueならば漢字表記 / falseなら英語表記
 * @param all falseならばベースアイテム上で明らかなフラグは省略する
 * @return アイテムの特性短縮表記
 */
std::string get_ability_abbreviation(const ItemEntity &item, bool is_kanji, bool all)
{
    auto flags = item.get_flags();
    if (!all) {
        const auto &baseitem = item.get_baseitem();
        flags.reset(baseitem.flags);

        if (item.is_fixed_artifact()) {
            const auto &artifact = item.get_fixed_artifact();
            flags.reset(artifact.flags);
        }

        if (item.is_ego()) {
            const auto &ego = item.get_ego();
            flags.reset(ego.flags);
        }
    }

    if (has_dark_flag(flags)) {
        if (flags.has(TR_LITE_1)) {
            flags.reset(TR_LITE_1);
        }

        if (flags.has(TR_LITE_2)) {
            flags.reset(TR_LITE_2);
        }

        if (flags.has(TR_LITE_3)) {
            flags.reset(TR_LITE_3);
        }
    } else if (has_lite_flag(flags)) {
        flags.set(TR_LITE_1);
        if (flags.has(TR_LITE_2)) {
            flags.reset(TR_LITE_2);
        }

        if (flags.has(TR_LITE_3)) {
            flags.reset(TR_LITE_3);
        }
    }

    std::stringstream ss;
    auto prev_tellp = ss.tellp();

    if (has_flag_of(flag_insc_plus, flags) && is_kanji) {
        ss << '+';
    }

    ss << inscribe_flags_aux(flag_insc_plus, flags, is_kanji);

    if (has_flag_of(flag_insc_immune, flags)) {
        if (!is_kanji && (ss.tellp() != prev_tellp)) {
            ss << ';';
            prev_tellp = ss.tellp();
        }

        ss << '*';
    }

    ss << inscribe_flags_aux(flag_insc_immune, flags, is_kanji);

    if (has_flag_of(flag_insc_vuln, flags)) {
        if (!is_kanji && (ss.tellp() != prev_tellp)) {
            ss << ';';
            prev_tellp = ss.tellp();
        }

        ss << 'v';
    }

    ss << inscribe_flags_aux(flag_insc_vuln, flags, is_kanji);

    if (has_flag_of(flag_insc_resistance, flags)) {
        if (is_kanji) {
            ss << 'r';
        } else if (ss.tellp() != prev_tellp) {
            ss << ';';
            prev_tellp = ss.tellp();
        }
    }

    ss << inscribe_flags_aux(flag_insc_resistance, flags, is_kanji);

    if (has_flag_of(flag_insc_misc, flags) && (ss.tellp() != prev_tellp)) {
        ss << ';';
        prev_tellp = ss.tellp();
    }

    ss << inscribe_flags_aux(flag_insc_misc, flags, is_kanji);

    if (has_flag_of(flag_insc_aura, flags)) {
        ss << '[';
    }

    ss << inscribe_flags_aux(flag_insc_aura, flags, is_kanji);

    if (has_flag_of(flag_insc_brand, flags)) {
        ss << '|';
    }

    ss << inscribe_flags_aux(flag_insc_brand, flags, is_kanji);

    if (has_flag_of(flag_insc_kill, flags)) {
        ss << "/X";
    }

    ss << inscribe_flags_aux(flag_insc_kill, flags, is_kanji);

    if (has_flag_of(flag_insc_slay, flags)) {
        ss << '/';
    }

    ss << inscribe_flags_aux(flag_insc_slay, flags, is_kanji);

    if (is_kanji) {
        if (has_flag_of(flag_insc_esp1, flags) || has_flag_of(flag_insc_esp2, flags)) {
            ss << '~';
        }

        ss << inscribe_flags_aux(flag_insc_esp1, flags, is_kanji)
           << inscribe_flags_aux(flag_insc_esp2, flags, is_kanji);
    } else {
        if (has_flag_of(flag_insc_esp1, flags)) {
            ss << '~';
        }

        ss << inscribe_flags_aux(flag_insc_esp1, flags, is_kanji);

        if (has_flag_of(flag_insc_esp2, flags)) {
            ss << '~';
        }

        ss << inscribe_flags_aux(flag_insc_esp2, flags, is_kanji);
    }

    if (has_flag_of(flag_insc_sust, flags)) {
        ss << '(';
    }

    ss << inscribe_flags_aux(flag_insc_sust, flags, is_kanji);

    return ss.str();
}

/*!
 * @brief オブジェクト名の特性短縮表記＋刻み内容を提示する。 / Get object inscription with auto inscription of object flags.
 * @param buff 特性短縮表記を格納する文字列ポインタ
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 */
std::string get_inscription(const ItemEntity &item)
{
    if (!item.is_inscribed()) {
        return {};
    }

    std::stringstream ss;

    if (!item.is_fully_known()) {
        for (std::string_view sv = *item.inscription; !sv.empty(); sv.remove_prefix(1)) {
            if (sv.front() == '#') {
                break;
            }
            if (ss.tellp() > MAX_INSCRIPTION - 1) {
                break;
            }
#ifdef JP
            if (iskanji(sv.front())) {
                ss << sv.front();
                sv.remove_prefix(1);
            }
#endif
            ss << sv.front();
        }

        return ss.str();
    }

    for (std::string_view sv = *item.inscription; !sv.empty(); sv.remove_prefix(1)) {
        switch (sv.front()) {
        case '#':
            return ss.str();
        case '%': {
            const auto start_pos = ss.tellp();
            if (start_pos >= MAX_NLEN) {
                break;
            }

            auto is_kanji = false;
#ifdef JP
            if ((sv.size() > 1) && ('%' == sv[1])) {
                sv.remove_prefix(1);
            } else {
                is_kanji = true;
            }
#endif

            auto all = false;
            if (sv.substr(1, 3) == "all") {
                all = true;
                sv.remove_prefix(3);
            }

            ss << get_ability_abbreviation(item, is_kanji, all);
            if (ss.tellp() == start_pos) {
                ss << ' ';
            }
            break;
        }
        default:
            ss << sv.front();
            break;
        }
    }

    return ss.str();
}

#ifdef JP
/*!
 * @brief アイテムにふさわしい助数詞をつけて数を記述する
 * @param item 数を記述したいアイテムの参照
 * @return 記述した文字列
 */
std::string describe_count_with_counter_suffix(const ItemEntity &item)
{
    std::stringstream ss;
    ss << item.number;

    switch (item.bi_key.tval()) {
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::POLEARM:
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
    case ItemKindType::ROD:
    case ItemKindType::DIGGING:
        ss << "本";
        break;
    case ItemKindType::SCROLL:
        ss << "巻";
        break;
    case ItemKindType::POTION:
        ss << "服";
        break;
    case ItemKindType::LIFE_BOOK:
    case ItemKindType::SORCERY_BOOK:
    case ItemKindType::NATURE_BOOK:
    case ItemKindType::CHAOS_BOOK:
    case ItemKindType::DEATH_BOOK:
    case ItemKindType::TRUMP_BOOK:
    case ItemKindType::ARCANE_BOOK:
    case ItemKindType::CRAFT_BOOK:
    case ItemKindType::DEMON_BOOK:
    case ItemKindType::CRUSADE_BOOK:
    case ItemKindType::MUSIC_BOOK:
    case ItemKindType::HISSATSU_BOOK:
    case ItemKindType::HEX_BOOK:
        ss << "冊";
        break;
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::CLOAK:
        ss << "着";
        break;
    case ItemKindType::SWORD:
    case ItemKindType::HAFTED:
    case ItemKindType::BOW:
        ss << "振";
        break;
    case ItemKindType::BOOTS:
        ss << "足";
        break;

    case ItemKindType::CARD:
        ss << "枚";
        break;

    case ItemKindType::FOOD:
        if (item.bi_key.sval() == SV_FOOD_JERKY) {
            ss << "切れ";
            break;
        }
        [[fallthrough]];
    default:
        if (item.number < 10) {
            ss << "つ";
        } else {
            ss << "個";
        }
        break;
    }

    return ss.str();
}
#endif
