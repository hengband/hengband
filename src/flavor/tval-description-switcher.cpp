/*!
 * @brief 個々のアイテム種別について、未鑑定名/鑑定後の正式な名前を取得する処理
 * @date 2020/07/07
 * @author Hourier
 */

#include "flavor/tval-description-switcher.h"
#include "flavor/flavor-util.h"
#include "flavor/object-flavor-types.h"
#include "locale/english.h"
#include "locale/localized-string.h"
#include "object-enchant/trg-types.h"
#include "object/tval-types.h"
#include "player/player-realm.h"
#include "realm/realm-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/baseitem/baseitem-record.h"
#include "system/item/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include <array>
#include <fmt/format.h>
#ifdef JP
#else
#include "player-info/class-info.h"
#endif

/*!
 * @brief 表示テンプレートからモンスター種族名のプレースホルダを取り除く
 * @param basename モンスター種族名を含む表示テンプレート
 * @return 種族名のプレースホルダを取り除いた表示テンプレート
 * @details 人形・像・死体類の表示テンプレートは種族名に係る助詞・前置詞を含んでいる
 * (日本語版「#の人形」、英語版「& Magical Figurine~ of #」など)。'#' だけを取り除くと
 * 「の人形」「Magical Figurine of 」のような不自然な名前になるため、併せて取り除く.
 */
static std::string omit_monrace_placeholder(std::string_view basename)
{
#ifdef JP
    static constexpr std::array<std::string_view, 2> patterns = { { "#の", "#" } };
#else
    static constexpr std::array<std::string_view, 3> patterns = { { " of #", "# ", "#" } };
#endif

    std::string result(basename);
    for (const auto &pattern : patterns) {
        result = str_replace(result, pattern, "");
    }

    return result;
}

static std::pair<std::string, std::string> describe_monster_ball(const ItemEntity &item, const describe_option_type &opt)
{
    const auto &basename = item.get_baseitem().name;
    const auto &monrace = item.get_monrace();
    if (!opt.known) {
        return { basename, "" };
    }

    if (!monrace.is_valid()) {
        return { basename, _(" (空)", " (empty)") };
    }

#ifdef JP
    const auto monrace_name = fmt::format(" ({})", monrace.name);
#else
    std::string monrace_name;
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        monrace_name = fmt::format(" ({}{})", (is_a_vowel(monrace.name.data()[0]) ? "an " : "a "), monrace.name);
    } else {
        monrace_name = fmt::format(" ({})", monrace.name);
    }
#endif
    return { basename, monrace_name };
}

/*!
 * @brief 人形・像の名前を記述する
 * @details OD_OMIT_MONRACE 指定時は種族名を省いた記述を返す.
 */
static std::pair<std::string, std::string> describe_statue(const ItemEntity &item, const describe_option_type &opt)
{
    const auto &basename = item.get_baseitem().name;
    if (any_bits(opt.mode, OD_OMIT_MONRACE)) {
        return { omit_monrace_placeholder(basename), "" };
    }

    const auto &monrace = item.get_monrace();
#ifdef JP
    const auto &monrace_name = monrace.name.string();
#else
    std::string monrace_name;
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        monrace_name = fmt::format("{}{}", (is_a_vowel(monrace.name.data()[0]) ? "an " : "a "), monrace.name);
    } else {
        monrace_name = monrace.name;
    }
#endif
    return { basename, monrace_name };
}

/*!
 * @brief 死体・骨の名前を記述する
 * @details OD_OMIT_MONRACE 指定時は種族名を省いた記述を返す.
 */
static std::pair<std::string, std::string> describe_corpse(const ItemEntity &item, const describe_option_type &opt)
{
    if (any_bits(opt.mode, OD_OMIT_MONRACE)) {
        // 種族が未設定ならユニークかどうかも判らないので非ユニーク側の書式を使う
        constexpr std::string_view basename = _("#%", "& # %");
        return { omit_monrace_placeholder(basename), "" };
    }

    const auto &monrace = item.get_monrace();
#ifdef JP
    const auto basename = "#%";
#else
    const auto basename =
        (monrace.kind_flags.has(MonsterKindType::UNIQUE))
            ? "& % of #"
            : "& # %";
#endif
    return { basename, monrace.name.string() };
}

/*!
 * @brief アイテムの未鑑定名を表す文字列を取得する
 *
 * 未鑑定名はゲーム開始時にシャッフルされるが、
 * opt.mode の OD_FORCE_FLAVOR ビットが ON の場合シャッフルされていない未鑑定名を返す
 * （未鑑定名からアイテムがバレるのを防ぐため、シンボルエディタで使用される）
 * OFF の場合シャッフルされた未鑑定名を返す
 *
 * @param item 対象アイテムの ItemEntity オブジェクトへの参照
 * @param opt アイテム表示オプション
 * @return アイテムの未鑑定名を表す文字列
 */
static std::string flavor_name_of(const ItemEntity &item, const describe_option_type &opt)
{
    const auto &baseitems = BaseitemList::get_instance();
    const auto &baseitem = item.get_baseitem();
    const auto &record = item.get_baseitem_record();
    return any_bits(opt.mode, OD_FORCE_FLAVOR)
               ? baseitem.flavor_name
               : baseitems.get_baseitem(record.get_appearance_id()).flavor_name;
}

static std::pair<std::string, std::string> describe_amulet(const ItemEntity &item, const describe_option_type &opt)
{
    const auto &baseitem = item.get_baseitem();
    if (opt.aware && (item.is_fixed_artifact() || baseitem.gen_flags.has(ItemGenerationTraitType::INSTA_ART))) {
        return { baseitem.name, "" };
    }

    if (!opt.flavor) {
        return { _("%のアミュレット", "& Amulet~ of %"), "" };
    } else if (opt.aware) {
        return { _("%の#アミュレット", "& # Amulet~ of %"), flavor_name_of(item, opt) };
    } else {
        return { _("#アミュレット", "& # Amulet~"), flavor_name_of(item, opt) };
    }
}

static std::pair<std::string, std::string> describe_ring(const ItemEntity &item, const describe_option_type &opt)
{
    const auto &baseitem = item.get_baseitem();
    if (opt.aware && (item.is_fixed_artifact() || baseitem.gen_flags.has(ItemGenerationTraitType::INSTA_ART))) {
        return { baseitem.name, "" };
    }

    if (!opt.flavor) {
        return { _("%の指輪", "& Ring~ of %"), "" };
    } else if (opt.aware) {
        return { _("%の#指輪", "& # Ring~ of %"), flavor_name_of(item, opt) };
    } else {
        return { _("#指輪", "& # Ring~"), flavor_name_of(item, opt) };
    }
}

static std::pair<std::string, std::string> describe_staff(const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.flavor) {
        return { _("%の杖", "& Staff~ of %"), "" };
    } else if (opt.aware) {
        return { _("%の#杖", "& # Staff~ of %"), flavor_name_of(item, opt) };
    } else {
        return { _("#杖", "& # Staff~"), flavor_name_of(item, opt) };
    }
}

static std::pair<std::string, std::string> describe_wand(const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.flavor) {
        return { _("%の魔法棒", "& Wand~ of %"), "" };
    } else if (opt.aware) {
        return { _("%の#魔法棒", "& # Wand~ of %"), flavor_name_of(item, opt) };
    } else {
        return { _("#魔法棒", "& # Wand~"), flavor_name_of(item, opt) };
    }
}

static std::pair<std::string, std::string> describe_rod(const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.flavor) {
        return { _("%のロッド", "& Rod~ of %"), "" };
    } else if (opt.aware) {
        return { _("%の#ロッド", "& # Rod~ of %"), flavor_name_of(item, opt) };
    } else {
        return { _("#ロッド", "& # Rod~"), flavor_name_of(item, opt) };
    }
}

static std::pair<std::string, std::string> describe_scroll(const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.flavor) {
        return { _("%の巻物", "& Scroll~ of %"), "" };
    } else if (opt.aware) {
        return { _("「#」と書かれた%の巻物", "& Scroll~ titled \"#\" of %"), flavor_name_of(item, opt) };
    } else {
        return { _("「#」と書かれた巻物", "& Scroll~ titled \"#\""), flavor_name_of(item, opt) };
    }
}

static std::pair<std::string, std::string> describe_potion(const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.flavor) {
        return { _("%の薬", "& Potion~ of %"), "" };
    } else if (opt.aware) {
        return { _("%の#薬", "& # Potion~ of %"), flavor_name_of(item, opt) };
    } else {
        return { _("#薬", "& # Potion~"), flavor_name_of(item, opt) };
    }
}

static std::pair<std::string, std::string> describe_food(const ItemEntity &item, const describe_option_type &opt)
{
    const auto &baseitem = item.get_baseitem();
    if (baseitem.flavor_name.empty()) {
        return { baseitem.name, "" };
    }

    if (!opt.flavor) {
        return { _("%のキノコ", "& Mushroom~ of %"), "" };
    } else if (opt.aware) {
        return { _("%の#キノコ", "& # Mushroom~ of %"), flavor_name_of(item, opt) };
    } else {
        return { _("#キノコ", "& # Mushroom~"), flavor_name_of(item, opt) };
    }
}

#ifndef JP
/*!
 * @brief 職業が生命の魔法書を使うか否かを返す
 * @return 生命の魔法書を使うなら true
 * @details 英語版の魔法書の名前は、生命の魔法書を使う職業なら "Book of ..." の形、それ以外は "... Spellbook" の形で表す。
 * 職業を決める前 (コマンドラインからのスポイラー出力など) は mp_ptr が設定されていないため、
 * 生命の魔法書を使わない職業と同じく扱う。
 */
static bool class_uses_life_book()
{
    return (mp_ptr != nullptr) && (mp_ptr->spell_book == ItemKindType::LIFE_BOOK);
}
#endif

/*!
 * @brief 魔法書の名前を得る
 * @param tval 魔法書の種類
 * @return std::pair<std::string, std::string> {basename, modstr} のペア
 * @details 英語版は、生命の魔法書を使う職業なら "Book of ... Magic" の形 (仙術だけは "Book of Sorcery")、
 * それ以外の職業なら "... Spellbook" の形で表す。
 */
static std::pair<std::string, std::string> describe_spellbook(ItemKindType tval)
{
    const auto realm = PlayerRealm::get_realm_of_book(tval);
    const auto &realm_name = PlayerRealm::get_name(realm);
#ifdef JP
    return { fmt::format("{}の魔法書%", realm_name), "" };
#else
    if (!class_uses_life_book()) {
        return { fmt::format("& {} Spellbook~ %", realm_name), "" };
    }

    if (realm == RealmType::SORCERY) {
        return { "& Book~ of Sorcery %", "" };
    }

    return { fmt::format("& Book~ of {} Magic %", realm_name), "" };
#endif
}

/*!
 * @brief アイテムの tval に従う記述を得る
 *
 * アイテムの tval に従いベース名称(basename)と修正文字列(modstr)を得る。
 * basename と modstr がどのような使われ方をするかは、describe_body() を参照。
 *
 * @param item アイテムへの参照
 * @param opt 記述オプション
 * @return std::pair<std::string, std::string> {basename, modstr} のペア
 */
std::pair<std::string, std::string> switch_tval_description(const ItemEntity &item, const describe_option_type &opt)
{
    const auto &basename = item.get_baseitem().name;

    switch (item.bi_key.tval()) {
    case ItemKindType::NONE:
        return { _("(なし)", "(Nothing)"), "" };
    case ItemKindType::FLAVOR_SKELETON:
    case ItemKindType::BOTTLE:
    case ItemKindType::JUNK:
    case ItemKindType::SPIKE:
    case ItemKindType::FLASK:
    case ItemKindType::CHEST:
    case ItemKindType::WHISTLE:
        return { basename, "" };
    case ItemKindType::CAPTURE:
        return describe_monster_ball(item, opt);
    case ItemKindType::FIGURINE:
    case ItemKindType::STATUE:
        return describe_statue(item, opt);
    case ItemKindType::MONSTER_REMAINS:
        return describe_corpse(item, opt);
    case ItemKindType::SHOT:
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::BOW:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::DIGGING:
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::CLOAK:
    case ItemKindType::CROWN:
    case ItemKindType::HELM:
    case ItemKindType::SHIELD:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::LITE:
        return { basename, "" };
    case ItemKindType::AMULET:
        return describe_amulet(item, opt);
    case ItemKindType::RING:
        return describe_ring(item, opt);
    case ItemKindType::CARD:
        return { basename, "" };
    case ItemKindType::STAFF:
        return describe_staff(item, opt);
    case ItemKindType::WAND:
        return describe_wand(item, opt);
    case ItemKindType::ROD:
        return describe_rod(item, opt);
    case ItemKindType::SCROLL:
        return describe_scroll(item, opt);
    case ItemKindType::POTION:
        return describe_potion(item, opt);
    case ItemKindType::FOOD:
        return describe_food(item, opt);
    case ItemKindType::PARCHMENT:
        return { _("羊皮紙 - %", "& Parchment~ - %"), "" };
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
    case ItemKindType::HEX_BOOK:
        return describe_spellbook(item.bi_key.tval());
    case ItemKindType::MUSIC_BOOK:
        return { _("歌集%", "& Song Book~ %"), "" };
    case ItemKindType::HISSATSU_BOOK:
        return { _("& 武芸の書%", "Book~ of Kendo %"), "" };
    case ItemKindType::GOLD:
        return { basename, "" };
    default:
        return { _("(なし)", "(nothing)"), "" };
    }
}
