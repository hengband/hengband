/*!
 * @brief ベースアイテム情報の構造体 / Information about object "kinds", including player knowledge.
 * @date 2019/05/01
 * @author deskull
 * @details
 * ゲーム進行用のセーブファイル上では aware と tried のみ保存対象とすること。と英文ではあるが実際はもっとある様子である。 /
 * Only "aware" and "tried" are saved in the savefile
 */

#include "system/baseitem-info.h"
#include "object/tval-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-bow-types.h"
#include "sv-definition/sv-food-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-potion-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-rod-types.h"
#include "sv-definition/sv-staff-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/angband-exceptions.h"
#include "system/enums/monrace/monrace-id.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include <algorithm>
#include <numeric>
#include <set>
#include <unordered_map>

namespace {
constexpr auto ITEM_NOT_BOW = "This item is not a bow!";
constexpr auto ITEM_NOT_ROD = "This item is not a rod!";
constexpr auto ITEM_NOT_LITE = "This item is not a lite!";
constexpr auto INVALID_BI_ID_FORMAT = "Invalid Baseitem ID is specified! %d";
constexpr auto INVALID_BASEITEM_KEY = "Invalid Baseitem Key is specified! Type: %d, Subtype: %d";
const std::map<MoneyKind, std::string> GOLD_KINDS = {
    { MoneyKind::COPPER, _("銅塊", "copper") },
    { MoneyKind::SILVER, _("銀塊", "silver") },
    { MoneyKind::GARNET, _("ガーネット", "garnets") },
    { MoneyKind::GOLD, _("金塊", "gold") },
    { MoneyKind::OPAL, _("オパール", "opals") },
    { MoneyKind::SAPPHIRE, _("サファイア", "sapphires") },
    { MoneyKind::RUBY, _("ルビー", "rubies") },
    { MoneyKind::DIAMOND, _("ダイヤモンド", "diamonds") },
    { MoneyKind::EMERALD, _("エメラルド", "emeralds") },
    { MoneyKind::MITHRIL, _("ミスリル", "mithril") },
    { MoneyKind::ADAMANTITE, _("アダマンタイト", "adamantite") },
};
const std::map<MonraceId, BaseitemKey> CREEPING_COIN_DROPS = {
    { MonraceId::COPPER_COINS, { ItemKindType::GOLD, 3 } },
    { MonraceId::SILVER_COINS, { ItemKindType::GOLD, 6 } },
    { MonraceId::GOLD_COINS, { ItemKindType::GOLD, 11 } },
    { MonraceId::MITHRIL_COINS, { ItemKindType::GOLD, 17 } },
    { MonraceId::MITHRIL_GOLEM, { ItemKindType::GOLD, 17 } },
    { MonraceId::ADAMANT_COINS, { ItemKindType::GOLD, 18 } },
};
}

bool BaseitemKey::operator==(const BaseitemKey &other) const
{
    return (this->type_value == other.type_value) && (this->subtype_value == other.subtype_value);
}

// @details type_valueに大小があればそれを判定し、同一ならばsubtype_valueの大小を判定する.
bool BaseitemKey::operator<(const BaseitemKey &other) const
{
    if (this->type_value < other.type_value) {
        return true;
    }

    if (this->type_value > other.type_value) {
        return false;
    }

    return this->subtype_value < other.subtype_value;
}

ItemKindType BaseitemKey::tval() const
{
    return this->type_value;
}

std::optional<int> BaseitemKey::sval() const
{
    return this->subtype_value;
}

bool BaseitemKey::is_valid() const
{
    return (this->type_value > ItemKindType::NONE) && this->subtype_value.has_value();
}

bool BaseitemKey::is(ItemKindType tval) const
{
    return this->type_value == tval;
}

/*!
 * @brief 射撃武器に対応する矢/弾薬のベースアイテムIDを返す
 * @return 対応する矢/弾薬のベースアイテムID
 */
ItemKindType BaseitemKey::get_arrow_kind() const
{
    if ((this->type_value != ItemKindType::BOW) || !this->subtype_value) {
        THROW_EXCEPTION(std::logic_error, ITEM_NOT_BOW);
    }

    switch (*this->subtype_value) {
    case SV_SLING:
        return ItemKindType::SHOT;
    case SV_SHORT_BOW:
    case SV_LONG_BOW:
    case SV_NAMAKE_BOW:
        return ItemKindType::ARROW;
    case SV_LIGHT_XBOW:
    case SV_HEAVY_XBOW:
        return ItemKindType::BOLT;
    case SV_CRIMSON:
    case SV_HARP:
        return ItemKindType::NO_AMMO;
    default:
        return ItemKindType::NONE;
    }
}

bool BaseitemKey::is_spell_book() const
{
    switch (this->type_value) {
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
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_high_level_book() const
{
    if (!this->is_spell_book()) {
        return false;
    }

    if (this->type_value == ItemKindType::ARCANE_BOOK) {
        return false;
    }

    return this->subtype_value >= 2;
}

bool BaseitemKey::is_melee_weapon() const
{
    switch (this->type_value) {
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_ammo() const
{
    switch (this->type_value) {
    case ItemKindType::SHOT:
    case ItemKindType::ARROW:
    case ItemKindType::BOLT:
        return true;
    default:
        return false;
    }
}

/*
 * @brief 未鑑定名を持つか否かの判定
 * @details FOODはキノコが該当する
 */
bool BaseitemKey::has_unidentified_name() const
{
    switch (this->type_value) {
    case ItemKindType::AMULET:
    case ItemKindType::RING:
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
    case ItemKindType::ROD:
    case ItemKindType::SCROLL:
    case ItemKindType::POTION:
        return true;
    case ItemKindType::FOOD:
        return this->is_mushrooms();
    default:
        return false;
    }
}

bool BaseitemKey::can_recharge() const
{
    switch (this->type_value) {
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
    case ItemKindType::ROD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_wand_rod() const
{
    switch (this->type_value) {
    case ItemKindType::WAND:
    case ItemKindType::ROD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_wand_staff() const
{
    switch (this->type_value) {
    case ItemKindType::WAND:
    case ItemKindType::STAFF:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_protector() const
{
    switch (this->type_value) {
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
    case ItemKindType::SHIELD:
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::can_be_aura_protector() const
{
    switch (this->type_value) {
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_wearable() const
{
    switch (this->type_value) {
    case ItemKindType::BOW:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::HELM:
    case ItemKindType::CROWN:
    case ItemKindType::SHIELD:
    case ItemKindType::CLOAK:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
    case ItemKindType::LITE:
    case ItemKindType::AMULET:
    case ItemKindType::RING:
    case ItemKindType::CARD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_weapon() const
{
    switch (this->type_value) {
    case ItemKindType::BOW:
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_equipement() const
{
    return this->is_wearable() || this->is_ammo();
}

bool BaseitemKey::is_melee_ammo() const
{
    switch (this->type_value) {
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::DIGGING:
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::SHOT:
        return true;
    case ItemKindType::SWORD:
        return this->subtype_value != SV_POISON_NEEDLE;
    default:
        return false;
    }
}

bool BaseitemKey::is_orthodox_melee_weapon() const
{
    switch (this->type_value) {
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::DIGGING:
        return true;
    case ItemKindType::SWORD:
        return this->subtype_value != SV_POISON_NEEDLE;
    default:
        return false;
    }
}

bool BaseitemKey::is_broken_weapon() const
{
    if (this->type_value != ItemKindType::SWORD) {
        return false;
    }

    if (!this->subtype_value) {
        return false;
    }

    switch (*this->subtype_value) {
    case SV_BROKEN_DAGGER:
    case SV_BROKEN_SWORD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_throwable() const
{
    switch (this->type_value) {
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_wieldable_in_etheir_hand() const
{
    switch (this->type_value) {
    case ItemKindType::DIGGING:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::SHIELD:
    case ItemKindType::CAPTURE:
    case ItemKindType::CARD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_rare() const
{
    static const std::unordered_map<ItemKindType, const std::set<int>> rare_table = {
        { ItemKindType::HAFTED, { SV_MACE_OF_DISRUPTION, SV_WIZSTAFF } },
        { ItemKindType::POLEARM, { SV_SCYTHE_OF_SLICING, SV_DEATH_SCYTHE } },
        { ItemKindType::SWORD, { SV_BLADE_OF_CHAOS, SV_DIAMOND_EDGE, SV_POISON_NEEDLE, SV_HAYABUSA } },
        { ItemKindType::SHIELD, { SV_DRAGON_SHIELD, SV_MIRROR_SHIELD } },
        { ItemKindType::HELM, { SV_DRAGON_HELM } },
        { ItemKindType::BOOTS, { SV_PAIR_OF_DRAGON_GREAVE } },
        { ItemKindType::CLOAK, { SV_ELVEN_CLOAK, SV_ETHEREAL_CLOAK, SV_SHADOW_CLOAK, SV_MAGIC_RESISTANCE_CLOAK } },
        { ItemKindType::GLOVES, { SV_SET_OF_DRAGON_GLOVES } },
        { ItemKindType::SOFT_ARMOR, { SV_KUROSHOUZOKU, SV_ABUNAI_MIZUGI } },
        { ItemKindType::HARD_ARMOR, { SV_MITHRIL_CHAIN_MAIL, SV_MITHRIL_PLATE_MAIL, SV_ADAMANTITE_PLATE_MAIL } },
        { ItemKindType::DRAG_ARMOR, { /* Any */ } },
    };

    if (!this->subtype_value) {
        return false;
    }

    if (auto it = rare_table.find(this->type_value); it != rare_table.end()) {
        const auto &svals = it->second;
        return svals.empty() || (svals.find(*this->subtype_value) != svals.end());
    }

    return false;
}

short BaseitemKey::get_bow_energy() const
{
    if ((this->type_value != ItemKindType::BOW) || !this->subtype_value) {
        THROW_EXCEPTION(std::logic_error, ITEM_NOT_BOW);
    }

    switch (*this->subtype_value) {
    case SV_SLING:
        return 8000;
    case SV_NAMAKE_BOW:
        return 7777;
    case SV_LIGHT_XBOW:
        return 12000;
    case SV_HEAVY_XBOW:
        return 13333;
    default:
        return 10000;
    }
}

int BaseitemKey::get_arrow_magnification() const
{
    if ((this->type_value != ItemKindType::BOW) || !this->subtype_value) {
        THROW_EXCEPTION(std::logic_error, ITEM_NOT_BOW);
    }

    switch (*this->subtype_value) {
    case SV_SLING:
    case SV_SHORT_BOW:
        return 2;
    case SV_LONG_BOW:
    case SV_NAMAKE_BOW:
    case SV_LIGHT_XBOW:
        return 3;
    case SV_HEAVY_XBOW:
        return 4;
    default:
        return 0;
    }
}

bool BaseitemKey::is_aiming_rod() const
{
    if ((this->type_value != ItemKindType::ROD) || !this->subtype_value) {
        THROW_EXCEPTION(std::logic_error, ITEM_NOT_ROD);
    }

    switch (*this->subtype_value) {
    case SV_ROD_TELEPORT_AWAY:
    case SV_ROD_DISARMING:
    case SV_ROD_LITE:
    case SV_ROD_SLEEP_MONSTER:
    case SV_ROD_SLOW_MONSTER:
    case SV_ROD_HYPODYNAMIA:
    case SV_ROD_POLYMORPH:
    case SV_ROD_ACID_BOLT:
    case SV_ROD_ELEC_BOLT:
    case SV_ROD_FIRE_BOLT:
    case SV_ROD_COLD_BOLT:
    case SV_ROD_ACID_BALL:
    case SV_ROD_ELEC_BALL:
    case SV_ROD_FIRE_BALL:
    case SV_ROD_COLD_BALL:
    case SV_ROD_STONE_TO_MUD:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_lite_requiring_fuel() const
{
    if ((this->type_value != ItemKindType::LITE) || !this->subtype_value) {
        THROW_EXCEPTION(std::logic_error, ITEM_NOT_LITE);
    }

    switch (*this->subtype_value) {
    case SV_LITE_TORCH:
    case SV_LITE_LANTERN:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_junk() const
{
    switch (this->type_value) {
    case ItemKindType::FLAVOR_SKELETON:
    case ItemKindType::BOTTLE:
    case ItemKindType::JUNK:
    case ItemKindType::STATUE:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_armour() const
{
    switch (this->type_value) {
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::is_cross_bow() const
{
    if ((this->type_value != ItemKindType::BOW) || !this->subtype_value) {
        return false;
    }

    switch (*this->subtype_value) {
    case SV_LIGHT_XBOW:
    case SV_HEAVY_XBOW:
        return true;
    default:
        return false;
    }
}

bool BaseitemKey::should_refuse_enchant() const
{
    return *this == BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE);
}

bool BaseitemKey::is_convertible() const
{
    auto is_convertible = this->is(ItemKindType::JUNK) || this->is(ItemKindType::FLAVOR_SKELETON);
    is_convertible |= *this == BaseitemKey(ItemKindType::MONSTER_REMAINS, SV_SKELETON);
    return is_convertible;
}

bool BaseitemKey::is_fuel() const
{
    auto is_fuel = *this == BaseitemKey(ItemKindType::LITE, SV_LITE_TORCH);
    is_fuel |= *this == BaseitemKey(ItemKindType::LITE, SV_LITE_LANTERN);
    is_fuel |= *this == BaseitemKey(ItemKindType::FLASK, SV_FLASK_OIL);
    return is_fuel;
}

bool BaseitemKey::is_lance() const
{
    auto is_lance = *this == BaseitemKey(ItemKindType::POLEARM, SV_LANCE);
    is_lance |= *this == BaseitemKey(ItemKindType::POLEARM, SV_HEAVY_LANCE);
    return is_lance;
}

bool BaseitemKey::is_readable() const
{
    auto can_read = this->is(ItemKindType::SCROLL);
    can_read |= this->is(ItemKindType::PARCHMENT);
    return can_read;
}

bool BaseitemKey::is_corpse() const
{
    return *this == BaseitemKey(ItemKindType::MONSTER_REMAINS, SV_CORPSE);
}

bool BaseitemKey::is_monster() const
{
    switch (this->type_value) {
    case ItemKindType::FIGURINE:
    case ItemKindType::STATUE:
    case ItemKindType::MONSTER_REMAINS:
    case ItemKindType::CAPTURE:
        return true;
    default:
        return false;
    }
}

/*!
 * @brief 2つのアイテムが同時に「普通の」像であることを示す
 * @param チェック対象のベースアイテムキー
 * @return 両方が写真の時だけfalse、少なくとも片方が「普通の」像ならばtrue、像ですらないならば例外
 */
bool BaseitemKey::are_both_statue(const BaseitemKey &other) const
{
    if ((this->type_value != ItemKindType::STATUE)) {
        THROW_EXCEPTION(std::logic_error, "This item is not a statue!");
    }

    if (other.type_value != ItemKindType::STATUE) {
        THROW_EXCEPTION(std::logic_error, "The other item is not a statue!");
    }

    return (this->subtype_value != SV_PHOTO) || (other.subtype_value != SV_PHOTO);
}

bool BaseitemKey::is_mushrooms() const
{
    if (!this->subtype_value) {
        return false;
    }

    switch (*this->subtype_value) {
    case SV_FOOD_POISON:
    case SV_FOOD_BLINDNESS:
    case SV_FOOD_PARANOIA:
    case SV_FOOD_CONFUSION:
    case SV_FOOD_HALLUCINATION:
    case SV_FOOD_PARALYSIS:
    case SV_FOOD_WEAKNESS:
    case SV_FOOD_SICKNESS:
    case SV_FOOD_STUPIDITY:
    case SV_FOOD_NAIVETY:
    case SV_FOOD_UNHEALTH:
    case SV_FOOD_DISEASE:
    case SV_FOOD_CURE_POISON:
    case SV_FOOD_CURE_BLINDNESS:
    case SV_FOOD_CURE_PARANOIA:
    case SV_FOOD_CURE_CONFUSION:
    case SV_FOOD_CURE_SERIOUS:
    case SV_FOOD_RESTORE_STR:
    case SV_FOOD_RESTORE_CON:
    case SV_FOOD_RESTORING:
        return true;
    default:
        return false;
    }
}

BaseitemInfo::BaseitemInfo()
    : bi_key(ItemKindType::NONE)
    , symbol_definition(DisplaySymbol(0, '\0'))
    , symbol_config(DisplaySymbol(0, '\0'))
{
}

/*!
 * @brief 正常なベースアイテムかを判定する
 * @return 正常なベースアイテムか否か
 * @details ID 0は「何か」という異常アイテム
 * その他、ベースアイテムIDは歴史的事情により歯抜けが多数あり、それらは名前が空欄になるようにオブジェクトを生成している
 * @todo v3.1以降で歯抜けを埋めるようにベースアイテムを追加していきたい (詳細未定)
 */
bool BaseitemInfo::is_valid() const
{
    return (this->idx > 0) && !this->name.empty();
}

/*!
 * @brief ベースアイテム名を返す
 * @return ベースアイテム名
 */
std::string BaseitemInfo::stripped_name() const
{
    const auto tokens = str_split(this->name, ' ');
    std::stringstream ss;
    for (const auto &token : tokens) {
        if (token == "" || token == "~" || token == "&" || token == "#") {
            continue;
        }

        auto offset = 0;
        auto endpos = token.size();
        auto is_kanji = false;
        if (token[0] == '~' || token[0] == '#') {
            offset++;
        }
#ifdef JP
        if (token.size() > 2) {
            is_kanji = iskanji(token[endpos - 2]);
        }

#endif
        if (!is_kanji && (token[endpos - 1] == '~' || token[endpos - 1] == '#')) {
            endpos--;
        }

        ss << token.substr(offset, endpos);
    }

    ss << " ";
    return ss.str();
}

bool BaseitemInfo::order_cost(const BaseitemInfo &other) const
{
    return this->cost < other.cost;
}

/*!
 * @brief 最初から簡易な名称が明らかなベースアイテムにその旨のフラグを立てる
 */
void BaseitemInfo::decide_easy_know()
{
    switch (this->bi_key.tval()) {
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
    case ItemKindType::FLASK:
    case ItemKindType::JUNK:
    case ItemKindType::BOTTLE:
    case ItemKindType::FLAVOR_SKELETON:
    case ItemKindType::SPIKE:
    case ItemKindType::WHISTLE:
    case ItemKindType::FOOD:
    case ItemKindType::POTION:
    case ItemKindType::SCROLL:
    case ItemKindType::ROD:
    case ItemKindType::STATUE:
    case ItemKindType::PARCHMENT:
        this->easy_know = true;
        return;
    default:
        this->easy_know = false;
        return;
    }
}

/*!
 * @brief オブジェクトを試行済にする
 */
void BaseitemInfo::mark_as_tried()
{
    this->tried = true;
}

void BaseitemInfo::mark_as_aware()
{
    this->aware = true;
}

BaseitemList BaseitemList::instance{};

BaseitemList &BaseitemList::get_instance()
{
    return instance;
}

BaseitemInfo &BaseitemList::get_baseitem(const short bi_id)
{
    if ((bi_id < 0) || (bi_id >= static_cast<short>(this->baseitems.size()))) {
        THROW_EXCEPTION(std::logic_error, format(INVALID_BI_ID_FORMAT, bi_id));
    }

    return this->baseitems[bi_id];
}

const BaseitemInfo &BaseitemList::get_baseitem(const short bi_id) const
{
    if ((bi_id < 0) || (bi_id >= static_cast<short>(this->baseitems.size()))) {
        THROW_EXCEPTION(std::logic_error, format(INVALID_BI_ID_FORMAT, bi_id));
    }

    return this->baseitems[bi_id];
}

std::vector<BaseitemInfo>::iterator BaseitemList::begin()
{
    return this->baseitems.begin();
}

std::vector<BaseitemInfo>::const_iterator BaseitemList::begin() const
{
    return this->baseitems.begin();
}

std::vector<BaseitemInfo>::iterator BaseitemList::end()
{
    return this->baseitems.end();
}

std::vector<BaseitemInfo>::const_iterator BaseitemList::end() const
{
    return this->baseitems.end();
}

std::vector<BaseitemInfo>::reverse_iterator BaseitemList::rbegin()
{
    return this->baseitems.rbegin();
}

std::vector<BaseitemInfo>::const_reverse_iterator BaseitemList::rbegin() const
{
    return this->baseitems.rbegin();
}

std::vector<BaseitemInfo>::reverse_iterator BaseitemList::rend()
{
    return this->baseitems.rend();
}

std::vector<BaseitemInfo>::const_reverse_iterator BaseitemList::rend() const
{
    return this->baseitems.rend();
}

size_t BaseitemList::size() const
{
    return this->baseitems.size();
}

bool BaseitemList::empty() const
{
    return this->baseitems.empty();
}

void BaseitemList::resize(size_t new_size)
{
    this->baseitems.resize(new_size);
}

void BaseitemList::shrink_to_fit()
{
    this->baseitems.shrink_to_fit();
}

/*!
 * @brief ベースアイテムキーからIDを引いて返す
 * @param key ベースアイテムキー、但しsvalはランダム(nullopt) の可能性がある
 * @return ベースアイテムID
 * @details ベースアイテムIDが存在しなければ例外
 */
short BaseitemList::lookup_baseitem_id(const BaseitemKey &bi_key) const
{
    const auto sval = bi_key.sval();
    if (sval) {
        return exe_lookup(bi_key);
    }

    static const auto &cache = create_baseitems_cache();
    const auto itr = cache.find(bi_key.tval());
    if (itr == cache.end()) {
        constexpr auto fmt = "Specified ItemKindType has no subtype! %d";
        THROW_EXCEPTION(std::runtime_error, format(fmt, enum2i(bi_key.tval())));
    }

    const auto &svals = itr->second;
    return exe_lookup({ bi_key.tval(), rand_choice(svals) });
}

const BaseitemInfo &BaseitemList::lookup_baseitem(const BaseitemKey &bi_key) const
{
    const auto bi_id = this->lookup_baseitem_id(bi_key);
    return this->baseitems[bi_id];
}

/*!
 * @brief モンスター種族IDから財宝アイテムの価値を引く
 * @param monrace_id モンスター種族ID
 * @return 特定の財宝を落とすならそのアイテムの価値オフセット、一般的な財宝ドロップならばnullopt
 */
std::optional<int> BaseitemList::lookup_creeping_coin_drop_offset(MonraceId monrace_id) const
{
    const auto it = CREEPING_COIN_DROPS.find(monrace_id);
    if (it == CREEPING_COIN_DROPS.end()) {
        return std::nullopt;
    }

    return this->lookup_gold_offset(it->second);
}

/*!
 * @brief ベースアイテム定義群から財宝アイテムの数を計算する
 * @return 財宝を示すベースアイテム数
 */
int BaseitemList::calc_num_gold_subtypes() const
{
    static const auto &golds = this->create_sorted_golds();
    static const auto sum = std::accumulate(golds.begin(), golds.end(), 0,
        [](int count, const auto &pair) {
            return count + pair.second.size();
        });
    return sum;
}

/*!
 * @brief 財宝アイテムの価値からベースアイテムを引く
 * @param target_offset 財宝アイテムの価値
 * @return ベースアイテムID
 * @details 同一の財宝カテゴリ内ならば常に大きいほど価値が高い.
 * カテゴリが異なるならば価値の大小は保証しない. 即ち「最も高い銅貨>最も安い銀貨」はあり得る.
 */
const BaseitemInfo &BaseitemList::lookup_gold(int target_offset) const
{
    auto offset = 0;
    for (const auto &pair : this->create_sorted_golds()) {
        for (const auto &bi_key : pair.second) {
            if (offset == target_offset) {
                return this->get_baseitem(this->exe_lookup(bi_key));
            }

            offset++;
        }
    }

    THROW_EXCEPTION(std::runtime_error, format("Invalid gold offset is specified! %d", target_offset));
}

/*!
 * @brief ベースアイテムIDから財宝アイテムの価値を引く
 * @param bi_id ベースアイテムID
 * @return 財宝アイテムの価値オフセット
 * @details 同一の財宝カテゴリ内ならば常に大きいほど価値が高い.
 * カテゴリが異なるならば価値の大小は保証しない. 即ち「最も高い銅貨>最も安い銀貨」はあり得る.
 */
int BaseitemList::lookup_gold_offset(short bi_id) const
{
    auto offset = 0;
    for (const auto &pair : this->create_sorted_golds()) {
        for (const auto &bi_key : pair.second) {
            if (bi_id == this->exe_lookup(bi_key)) {
                return offset;
            }

            offset++;
        }
    }

    THROW_EXCEPTION(std::runtime_error, format(INVALID_BI_ID_FORMAT, bi_id));
}

void BaseitemList::reset_all_visuals()
{
    for (auto &baseitem : this->baseitems) {
        baseitem.symbol_config = baseitem.symbol_definition;
    }
}

/*!
 * @brief ベースアイテムの鑑定済みフラグをリセットする
 * @details 不具合対策で0からリセットする(セーブは0から)
 */
void BaseitemList::reset_identification_flags()
{
    for (auto &baseitem : this->baseitems) {
        baseitem.tried = false;
        baseitem.aware = false;
    }
}

/*!
 * @brief 未鑑定アイテム種別の内、ゲーム開始時から鑑定済とするアイテムの鑑定済フラグをONにする
 * @todo 食料用の杖は該当種族 (ゴーレム/骸骨/ゾンビ/幽霊)では鑑定済だが、本来はこのメソッドで鑑定済にすべき.
 */
void BaseitemList::mark_common_items_as_aware()
{
    std::vector<BaseitemKey> bi_keys;
    bi_keys.emplace_back(ItemKindType::POTION, SV_POTION_WATER);
    bi_keys.emplace_back(ItemKindType::STAFF, SV_STAFF_NOTHING);
    for (const auto &bi_key : bi_keys) {
        this->lookup_baseitem(bi_key).mark_as_aware();
    }
}

void BaseitemList::shuffle_flavors()
{
    this->shuffle_flavors(ItemKindType::RING);
    this->shuffle_flavors(ItemKindType::AMULET);
    this->shuffle_flavors(ItemKindType::STAFF);
    this->shuffle_flavors(ItemKindType::WAND);
    this->shuffle_flavors(ItemKindType::ROD);
    this->shuffle_flavors(ItemKindType::FOOD);
    this->shuffle_flavors(ItemKindType::POTION);
    this->shuffle_flavors(ItemKindType::SCROLL);
}

/*!
 * @brief ベースアイテムキーに対応するベースアイテムのIDを検索する
 * @param key 検索したいベースアイテムキー
 * @return ベースアイテムID
 * @details ベースアイテムIDが存在しなければ例外
 */
short BaseitemList::exe_lookup(const BaseitemKey &bi_key) const
{
    static const auto &cache = create_baseitem_index_chache();
    const auto itr = cache.find(bi_key);
    if (itr == cache.end()) {
        THROW_EXCEPTION(std::runtime_error, format(INVALID_BASEITEM_KEY, enum2i(bi_key.tval()), *bi_key.sval()));
    }

    return itr->second;
}

/*
 * @brief tvalとbi_key.svalに対応する、BaseitenDefinitions のIDを返すためのキャッシュを生成する
 * @return tvalと(実在する)svalの組み合わせをキーに、ベースアイテムIDを値とした辞書
 */
const std::map<BaseitemKey, short> &BaseitemList::create_baseitem_index_chache() const
{
    static std::map<BaseitemKey, short> cache;
    for (const auto &baseitem : BaseitemList::get_instance()) {
        if (!baseitem.is_valid()) {
            continue;
        }

        const auto &bi_key = baseitem.bi_key;
        cache[bi_key] = baseitem.idx;
    }

    return cache;
}

/*
 * @brief 特定のtvalとランダムなsvalの組み合わせからベースアイテムを選択するためのキャッシュを生成する
 * @return tvalをキーに、svalのリストを値とした辞書
 */
const std::map<ItemKindType, std::vector<int>> &BaseitemList::create_baseitems_cache() const
{
    static std::map<ItemKindType, std::vector<int>> cache;
    for (const auto &baseitem : BaseitemList::get_instance()) {
        if (!baseitem.is_valid()) {
            continue;
        }

        const auto &bi_key = baseitem.bi_key;
        const auto tval = bi_key.tval();
        cache[tval].push_back(*bi_key.sval());
    }

    return cache;
}

/*!
 * @brief ベースアイテムキーから財宝アイテムの価値を引く
 * @param finding_bi_key 探索対象のベースアイテムキー
 * @return 財宝アイテムの価値番号 (大きいほど価値が高い)
 * @details 同一の財宝カテゴリ内ならば常に番号が大きいほど価値も高い.
 * カテゴリが異なるならば価値の大小は保証しない. 即ち「最も高い銅貨>最も安い銀貨」はあり得る.
 */
int BaseitemList::lookup_gold_offset(const BaseitemKey &finding_bi_key) const
{
    auto offset = 0;
    for (const auto &pair : this->create_sorted_golds()) {
        for (const auto &bi_key : pair.second) {
            if (finding_bi_key == bi_key) {
                return offset;
            }

            offset++;
        }
    }

    THROW_EXCEPTION(std::runtime_error, format(INVALID_BASEITEM_KEY, enum2i(finding_bi_key.tval()), *finding_bi_key.sval()));
}

/*!
 * @brief ベースアイテム定義リストから財宝の辞書を作る (価値順)
 * @return 財宝種別をキー、それに対応するベースアイテムキーの配列 (安い順にソート済)を値とした辞書
 */
const std::map<MoneyKind, std::vector<BaseitemKey>> &BaseitemList::create_sorted_golds() const
{
    static std::map<MoneyKind, std::vector<BaseitemKey>> list;
    if (!list.empty()) {
        return list;
    }

    list = this->create_unsorted_golds();
    for (auto &[money_kind, bi_keys] : list) {
        std::stable_sort(bi_keys.begin(), bi_keys.end(),
            [this](const auto &bi_key1, const auto &bi_key2) {
                const auto &baseitem1 = this->lookup_baseitem(bi_key1);
                const auto &baseitem2 = this->lookup_baseitem(bi_key2);
                return baseitem1.order_cost(baseitem2);
            });
    }

    return list;
}

/*!
 * @brief ベースアイテム定義リストから財宝の辞書を作る (ベースアイテムID順)
 * @return 財宝種別をキー、それに対応するベースアイテムキーの配列を値とした辞書
 */
std::map<MoneyKind, std::vector<BaseitemKey>> BaseitemList::create_unsorted_golds() const
{
    std::map<MoneyKind, std::vector<BaseitemKey>> list;
    for (const auto &baseitem : this->baseitems) {
        const auto &bi_key = baseitem.bi_key;
        if (bi_key.tval() != ItemKindType::GOLD) {
            continue;
        }

        for (const auto money_kind : MONEY_KIND_RANGE) {
            if (baseitem.name != GOLD_KINDS.at(money_kind)) {
                continue;
            }

            list[money_kind].push_back(bi_key);
        }
    }

    return list;
}

BaseitemInfo &BaseitemList::lookup_baseitem(const BaseitemKey &bi_key)
{
    const auto bi_id = this->lookup_baseitem_id(bi_key);
    return this->baseitems[bi_id];
}

/*!
 * @brief ベースアイテムの未確定名を共通tval間でシャッフルする
 * @param tval シャッフルしたいtval
 * @details 巻物、各種魔道具などに利用される。
 */
void BaseitemList::shuffle_flavors(ItemKindType tval)
{
    std::vector<std::reference_wrapper<short>> flavors;
    for (auto &baseitem : this->baseitems) {
        if (baseitem.bi_key.tval() != tval) {
            continue;
        }

        if (baseitem.flavor == 0) {
            continue;
        }

        if (baseitem.flags.has(TR_FIXED_FLAVOR)) {
            continue;
        }

        flavors.push_back(baseitem.flavor);
    }

    rand_shuffle(flavors.begin(), flavors.end());
}
