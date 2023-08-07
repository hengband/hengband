/*!
 * @brief 個々のアイテム種別について、未鑑定名/鑑定後の正式な名前を取得する処理
 * @date 2020/07/07
 * @author Hourier
 */

#include "flavor/tval-description-switcher.h"
#include "flavor/flavor-util.h"
#include "flavor/object-flavor-types.h"
#include "locale/english.h"
#include "monster-race/monster-race.h"
#include "object-enchant/trg-types.h"
#include "object/tval-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/monster-race-info.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"
#ifdef JP
#else
#include "monster-race/race-flags1.h"
#include "player-info/class-info.h"
#endif

static std::pair<std::string, std::string> describe_monster_ball(const ItemEntity &item, const describe_option_type &opt)
{
    const auto &basename = item.get_baseitem().name;
    const auto r_idx = i2enum<MonsterRaceId>(item.pval);
    auto *r_ptr = &monraces_info[r_idx];
    if (!opt.known) {
        return { basename, "" };
    }

    if (!MonsterRace(r_idx).is_valid()) {
        return { basename, _(" (空)", " (empty)") };
    }

#ifdef JP
    const auto modstr = format(" (%s)", r_ptr->name.data());
#else
    std::string modstr;
    if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        modstr = format(" (%s%s)", (is_a_vowel(r_ptr->name[0]) ? "an " : "a "), r_ptr->name.data());
    } else {
        modstr = format(" (%s)", r_ptr->name.data());
    }
#endif
    return { basename, modstr };
}

static std::pair<std::string, std::string> describe_statue(const ItemEntity &item)
{
    const auto &basename = item.get_baseitem().name;
    const auto r_idx = i2enum<MonsterRaceId>(item.pval);
    auto *r_ptr = &monraces_info[r_idx];
#ifdef JP
    const auto &modstr = r_ptr->name;
#else
    std::string modstr;
    if (r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        modstr = format("%s%s", (is_a_vowel(r_ptr->name[0]) ? "an " : "a "), r_ptr->name.data());
    } else {
        modstr = r_ptr->name;
    }
#endif
    return { basename, modstr };
}

static std::pair<std::string, std::string> describe_corpse(const ItemEntity &item)
{
    const auto r_idx = i2enum<MonsterRaceId>(item.pval);
    auto *r_ptr = &monraces_info[r_idx];
    const auto &modstr = r_ptr->name;
#ifdef JP
    const auto basename = "#%";
#else
    const auto basename =
        (r_ptr->kind_flags.has(MonsterKindType::UNIQUE))
            ? "& % of #"
            : "& # %";
#endif
    return { basename, modstr };
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
    const auto &baseitem = item.get_baseitem();
    return any_bits(opt.mode, OD_FORCE_FLAVOR)
               ? baseitem.flavor_name
               : baseitems_info[baseitem.flavor].flavor_name;
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

static std::pair<std::string, std::string> describe_book_life()
{
#ifdef JP
    return { "生命の魔法書%", "" };
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) {
        return { "& Book~ of Life Magic %", "" };
    } else {
        return { "& Life Spellbook~ %", "" };
    }
#endif
}

static std::pair<std::string, std::string> describe_book_sorcery()
{
#ifdef JP
    return { "仙術の魔法書%", "" };
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) {
        return { "& Book~ of Sorcery %", "" };
    } else {
        return { "& Sorcery Spellbook~ %", "" };
    }
#endif
}

static std::pair<std::string, std::string> describe_book_nature()
{
#ifdef JP
    return { "自然の魔法書%", "" };
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) {
        return { "& Book~ of Nature Magic %", "" };
    } else {
        return { "& Nature Spellbook~ %", "" };
    }
#endif
}

static std::pair<std::string, std::string> describe_book_chaos()
{
#ifdef JP
    return { "カオスの魔法書%", "" };
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) {
        return { "& Book~ of Chaos Magic %", "" };
    } else {
        return { "& Chaos Spellbook~ %", "" };
    }
#endif
}

static std::pair<std::string, std::string> describe_book_death()
{
#ifdef JP
    return { "暗黒の魔法書%", "" };
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) {
        return { "& Book~ of Death Magic %", "" };
    } else {
        return { "& Death Spellbook~ %", "" };
    }
#endif
}

static std::pair<std::string, std::string> describe_book_trump()
{
#ifdef JP
    return { "トランプの魔法書%", "" };
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) {
        return { "& Book~ of Trump Magic %", "" };
    } else {
        return { "& Trump Spellbook~ %", "" };
    }
#endif
}

static std::pair<std::string, std::string> describe_book_arcane()
{
#ifdef JP
    return { "秘術の魔法書%", "" };
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) {
        return { "& Book~ of Arcane Magic %", "" };
    } else {
        return { "& Arcane Spellbook~ %", "" };
    }
#endif
}

static std::pair<std::string, std::string> describe_book_craft()
{
#ifdef JP
    return { "匠の魔法書%", "" };
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) {
        return { "& Book~ of Craft Magic %", "" };
    } else {
        return { "& Craft Spellbook~ %", "" };
    }
#endif
}

static std::pair<std::string, std::string> describe_book_demon()
{
#ifdef JP
    return { "悪魔の魔法書%", "" };
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) {
        return { "& Book~ of Daemon Magic %", "" };
    } else {
        return { "& Daemon Spellbook~ %", "" };
    }
#endif
}

static std::pair<std::string, std::string> describe_book_crusade()
{
#ifdef JP
    return { "破邪の魔法書%", "" };
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) {
        return { "& Book~ of Crusade Magic %", "" };
    } else {
        return { "& Crusade Spellbook~ %", "" };
    }
#endif
}

static std::pair<std::string, std::string> describe_book_hex()
{
#ifdef JP
    return { "呪術の魔法書%", "" };
#else
    if (mp_ptr->spell_book == ItemKindType::LIFE_BOOK) {
        return { "& Book~ of Hex Magic %", "" };
    } else {
        return { "& Hex Spellbook~ %", "" };
    }
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
    case ItemKindType::SKELETON:
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
        return describe_statue(item);
    case ItemKindType::CORPSE:
        return describe_corpse(item);
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
        return describe_book_life();
    case ItemKindType::SORCERY_BOOK:
        return describe_book_sorcery();
    case ItemKindType::NATURE_BOOK:
        return describe_book_nature();
    case ItemKindType::CHAOS_BOOK:
        return describe_book_chaos();
    case ItemKindType::DEATH_BOOK:
        return describe_book_death();
    case ItemKindType::TRUMP_BOOK:
        return describe_book_trump();
    case ItemKindType::ARCANE_BOOK:
        return describe_book_arcane();
    case ItemKindType::CRAFT_BOOK:
        return describe_book_craft();
    case ItemKindType::DEMON_BOOK:
        return describe_book_demon();
    case ItemKindType::CRUSADE_BOOK:
        return describe_book_crusade();
    case ItemKindType::MUSIC_BOOK:
        return { _("歌集%", "& Song Book~ %"), "" };
    case ItemKindType::HISSATSU_BOOK:
        return { _("& 武芸の書%", "Book~ of Kendo %"), "" };
    case ItemKindType::HEX_BOOK:
        return describe_book_hex();
    case ItemKindType::GOLD:
        return { basename, "" };
    default:
        return { _("(なし)", "(nothing)"), "" };
    }
}
