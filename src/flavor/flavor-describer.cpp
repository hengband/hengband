/*!
 * @brief 武器/防具/アクセサリアイテムにおける、耐性やスレイ等の表記
 * @date 2020/07/06
 * @author Hourier
 */

#include "flavor/flavor-describer.h"
#include "combat/shoot.h"
#include "flavor/flag-inscriptions-table.h"
#include "flavor/flavor-util.h"
#include "flavor/named-item-describer.h"
#include "flavor/object-flavor-types.h"
#include "game-option/text-display-options.h"
#include "grid/trap.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-sniper.h"
#include "mind/mind-weaponsmith.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trg-types.h"
#include "object-hook/hook-quest.h"
#include "object/object-flags.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player/player-status-table.h"
#include "player/player-status.h"
#include "smith/object-smith.h"
#include "smith/smith-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "window/display-sub-window-items.h"
#include <sstream>

static void check_object_known_aware(flavor_type *flavor_ptr)
{
    flavor_ptr->tr_flags = object_flags(flavor_ptr->o_ptr);
    if (flavor_ptr->o_ptr->is_aware()) {
        flavor_ptr->aware = true;
    }

    if (flavor_ptr->o_ptr->is_known()) {
        flavor_ptr->known = true;
    }

    if (flavor_ptr->aware && ((flavor_ptr->mode & OD_NO_FLAVOR) || plain_descriptions)) {
        flavor_ptr->flavor = false;
    }

    if ((flavor_ptr->mode & OD_STORE) || (flavor_ptr->o_ptr->ident & IDENT_STORE)) {
        flavor_ptr->flavor = false;
        flavor_ptr->aware = true;
        flavor_ptr->known = true;
    }

    if (flavor_ptr->mode & OD_FORCE_FLAVOR) {
        flavor_ptr->aware = false;
        flavor_ptr->flavor = true;
        flavor_ptr->known = false;
        flavor_ptr->flavor_k_ptr = flavor_ptr->k_ptr;
    }
}

static std::string describe_chest_trap(const ItemEntity &item)
{
    auto trap_kinds = chest_traps[item.pval];
    if (trap_kinds.count() >= 2) {
        return _("(マルチ・トラップ)", " (Multiple Traps)");
    }

    auto trap_kind = trap_kinds.first();
    if (!trap_kind.has_value()) {
        return _("(施錠)", " (Locked)");
    }

    switch (trap_kind.value()) {
    case ChestTrapType::LOSE_STR:
        return _("(毒針)", " (Poison Needle)");
    case ChestTrapType::LOSE_CON:
        return _("(毒針)", " (Poison Needle)");
    case ChestTrapType::POISON:
        return _("(ガス・トラップ)", " (Gas Trap)");
    case ChestTrapType::PARALYZE:
        return _("(ガス・トラップ)", " (Gas Trap)");
    case ChestTrapType::EXPLODE:
        return _("(爆発装置)", " (Explosive Device)");
    case ChestTrapType::SUMMON:
    case ChestTrapType::BIRD_STORM:
    case ChestTrapType::E_SUMMON:
    case ChestTrapType::H_SUMMON:
        return _("(召喚のルーン)", " (Summoning Runes)");
    case ChestTrapType::RUNES_OF_EVIL:
        return _("(邪悪なルーン)", " (Gleaming Black Runes)");
    case ChestTrapType::ALARM:
        return _("(警報装置)", " (Alarm)");
    case ChestTrapType::SCATTER:
        return _("(アイテム散乱)", " (Scatter)");
    case ChestTrapType::MAX:
        throw("Invalid chest trap type is specified!");
    }

    return "";
}

static std::string describe_chest(const ItemEntity &item, const describe_option_type &opt)
{
    if (item.bi_key.tval() != ItemKindType::CHEST) {
        return "";
    }

    if (!opt.known) {
        return "";
    }

    if (!item.pval) {
        return _("(空)", " (empty)");
    }

    if (item.pval < 0) {
        if (chest_traps[0 - item.pval].any()) {
            return _("(解除済)", " (disarmed)");
        } else {
            return _("(非施錠)", " (unlocked)");
        }
    }

    return describe_chest_trap(item);
}

static bool should_show_ac_bonus(const ItemEntity &item)
{
    return (item.ac != 0) || item.is_protector();
}

static bool should_show_slaying_bonus(const ItemEntity &item)
{
    if (object_flags(&item).has(TR_SHOW_MODS)) {
        return true;
    }

    if (item.is_smith() && (Smith::object_effect(&item) == SmithEffectType::SLAY_GLOVE)) {
        return true;
    }

    if ((item.to_h != 0) && (item.to_d != 0)) {
        return true;
    }

    if (item.is_weapon_ammo()) {
        return true;
    }

    if (item.bi_key.tval() == ItemKindType::RING) {
        const auto &baseitem = baseitems_info[item.bi_id];
        const auto base_has_no_bonus = (baseitem.to_h == 0) && (baseitem.to_d == 0);
        const auto item_has_bonus = (item.to_h != 0) || (item.to_d != 0);
        if (base_has_no_bonus && item_has_bonus) {
            return true;
        }
    }

    return false;
}

static std::string describe_weapon_dice(PlayerType *player_ptr, const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.known && object_is_quest_target(player_ptr->current_floor_ptr->quest_number, &item)) {
        return "";
    }

    const auto is_bonus = (player_ptr->riding > 0) && item.is_lance();
    const auto bonus = is_bonus ? 2 : 0;
    return format(" (%dd%d)", item.dd + bonus, item.ds);
}

static std::string describe_bow_power(PlayerType *player_ptr, const ItemEntity &item, const describe_option_type &opt)
{
    auto power = item.get_arrow_magnification();
    const auto tr_flags = object_flags(&item);
    if (tr_flags.has(TR_XTRA_MIGHT)) {
        power++;
    }

    std::stringstream ss;
    ss << format(" (x%d)", power);

    auto num_fire = 100;
    if (none_bits(opt.mode, OD_DEBUG)) {
        num_fire = calc_num_fire(player_ptr, &item);
    } else {
        if (tr_flags.has(TR_XTRA_SHOTS)) {
            num_fire += 100;
        }
    }

    if ((num_fire == 0) || (power <= 0) || !opt.known) {
        return ss.str();
    }

    const auto fire_rate = item.get_bow_energy() / num_fire;
    ss << format(" (%d.%dturn)", fire_rate / 100, fire_rate % 100);

    return ss.str();
}

static std::string describe_weapon_dice_or_bow_power(PlayerType *player_ptr, const ItemEntity &item, const describe_option_type &opt)
{
    switch (item.bi_key.tval()) {
    case ItemKindType::SHOT:
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::DIGGING:
        return describe_weapon_dice(player_ptr, item, opt);
    case ItemKindType::BOW:
        return describe_bow_power(player_ptr, item, opt);
    default:
        return "";
    }
}

static std::string describe_accuracy_and_damage_bonus(const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.known) {
        return "";
    }

    if (should_show_slaying_bonus(item)) {
        return format(" (%+d,%+d)", item.to_h, item.to_d);
    }

    if (item.to_h != 0) {
        return format(" (%+d)", item.to_h);
    }

    if (item.to_d != 0) {
        return format(" (%+d)", item.to_d);
    }

    return "";
}

static std::string describe_fire_energy(PlayerType *player_ptr, const ItemEntity &ammo, const ItemEntity &bow, const describe_option_type &opt, int avgdam)
{
    const auto energy_fire = bow.get_bow_energy();
    if (player_ptr->num_fire == 0) {
        return "0";
    }

    const auto avgdam_per_turn = avgdam * player_ptr->num_fire * 100 / energy_fire;

    std::stringstream ss;
    ss << avgdam_per_turn
       << (show_ammo_detail ? "/turn" : "");
    if (!show_ammo_crit_ratio) {
        return ss.str();
    }

    const auto ammo_bonus = opt.known ? ammo.to_h : 0;
    const auto bow_bonus = bow.is_known() ? bow.to_h : 0;
    const auto percent = calc_crit_ratio_shot(player_ptr, ammo_bonus, bow_bonus);

    ss << format("/%d.%02d%s", percent / 100, percent % 100, show_ammo_detail ? "% crit" : "%");

    return ss.str();
}

static std::string describe_ammo_detail(PlayerType *player_ptr, const ItemEntity &ammo, const ItemEntity &bow, const describe_option_type &opt)
{
    auto avgdam = ammo.dd * (ammo.ds + 1) * 10 / 2;
    auto tmul = bow.get_arrow_magnification();
    if (bow.is_known()) {
        avgdam += (bow.to_d * 10);
    }

    if (opt.known) {
        avgdam += (ammo.to_d * 10);
    }

    if (player_ptr->xtra_might) {
        tmul++;
    }

    tmul = tmul * (100 + static_cast<int>(adj_str_td[player_ptr->stat_index[A_STR]]) - 128);
    avgdam *= tmul;
    avgdam /= (100 * 10);
    avgdam = boost_concentration_damage(player_ptr, avgdam);

    if (avgdam < 0) {
        avgdam = 0;
    }
    const auto crit_avgdam = calc_expect_crit_shot(player_ptr, ammo.weight, ammo.to_h, bow.to_h, avgdam);

    std::stringstream ss;
    ss << " (";

    if (show_ammo_no_crit) {
        ss << avgdam << (show_ammo_detail ? "/shot " : "/");
    }

    ss << crit_avgdam
       << (show_ammo_no_crit ? (show_ammo_detail ? "/crit " : "/")
                             : (show_ammo_detail ? "/shot " : "/"))
       << describe_fire_energy(player_ptr, ammo, bow, opt, crit_avgdam)
       << ")";

    return ss.str();
}

static std::string describe_spike_detail(PlayerType *player_ptr)
{
    auto avgdam = player_ptr->mighty_throw ? (1 + 3) : 1;
    avgdam += ((player_ptr->lev + 30) * (player_ptr->lev + 30) - 900) / 55;
    const auto energy_fire = 100 - player_ptr->lev;
    const auto avgdam_per_turn = 100 * avgdam / energy_fire;

    return format(" (%d/%d)", avgdam, avgdam_per_turn);
}

static std::string describe_known_item_ac(const ItemEntity &item)
{
    if (should_show_ac_bonus(item)) {
        return format(" [%d,%+d]", item.ac, item.to_a);
    }

    if (item.to_a == 0) {
        return "";
    }

    return format(" [%+d]", item.to_a);
}

static std::string describe_ac(const ItemEntity &item, const describe_option_type &opt)
{
    if (opt.known) {
        return describe_known_item_ac(item);
    }

    if (!should_show_ac_bonus(item)) {
        return "";
    }

    return format(" [%d]", item.ac);
}

static std::string describe_charges_staff_wand(const ItemEntity &item)
{
    std::string staff_num;
    if ((item.bi_key.tval() == ItemKindType::STAFF) && (item.number > 1)) {
        staff_num = format("%dx ", item.number);
    }

    const auto charge_str = _("回分", (item.pval != 1 ? " charge" : " charges"));
    return format(" (%s%d%s)", staff_num.data(), item.pval, charge_str);
}

static std::string describe_charges_rod(const ItemEntity &item)
{
    if (item.timeout <= 0) {
        return "";
    }

    if (item.number <= 1) {
        return _("(充填中)", " (charging)");
    }

    const auto timeout_per_one = baseitems_info[item.bi_id].pval;
    if (timeout_per_one <= 0) {
        return "";
    }

    auto num_of_charging = (item.timeout + (timeout_per_one - 1)) / timeout_per_one;
    if (num_of_charging > item.number) {
        num_of_charging = item.number;
    }

    return format(" (%d%s)", num_of_charging, _("本 充填中", " charging"));
}

static std::string describe_pval_type(const ItemEntity &item)
{
    const auto tr_flags = object_flags(&item);
    if (tr_flags.has(TR_HIDE_TYPE)) {
        return "";
    }

    if (tr_flags.has(TR_SPEED)) {
        return _("加速", " to speed");
    }

    if (tr_flags.has(TR_BLOWS)) {
        return _("攻撃", ((std::abs(item.pval) == 1) ? " attack" : " attacks"));
    }

    if (tr_flags.has(TR_STEALTH)) {
        return _("隠密", " to stealth");
    }

    if (tr_flags.has(TR_SEARCH)) {
        return _("探索", " to searching");
    }

    if (tr_flags.has(TR_INFRA)) {
        return _("赤外線視力", " to infravision");
    }

    return "";
}

static std::string describe_pval(const ItemEntity &item)
{
    const auto tr_flags = object_flags(&item);
    if (tr_flags.has_none_of(TR_PVAL_FLAG_MASK)) {
        return "";
    }

    const auto pval_type = describe_pval_type(item);
    return format(" (%+d%s)", item.pval, pval_type.data());
}

static std::string describe_lamp_life(const ItemEntity &item)
{
    const auto &bi_key = item.bi_key;
    if ((bi_key.tval() != ItemKindType::LITE) || (item.is_fixed_artifact() || (bi_key.sval() == SV_LITE_FEANOR))) {
        return "";
    }

    const auto fuel_magnification = item.ego_idx == EgoType::LITE_LONG ? 2 : 1;
    std::stringstream ss;
    ss << _("(", " (with ")
       << fuel_magnification * item.fuel
       << _("ターンの寿命)", " turns of light)");

    return ss.str();
}

/*!
 * @brief 杖や光源等、寿命のあるアイテムの残り回数やターン表記
 * @param アイテム表記への参照ポインタ
 */
static std::string describe_remaining(const ItemEntity &item, const describe_option_type &opt)
{
    if (!opt.known) {
        return "";
    }

    std::stringstream ss;
    const auto tval = item.bi_key.tval();
    if (item.is_wand_staff()) {
        ss << describe_charges_staff_wand(item);
    } else if (tval == ItemKindType::ROD) {
        ss << describe_charges_rod(item);
    }

    ss << describe_pval(item)
       << describe_lamp_life(item);
    if (item.timeout && (tval != ItemKindType::ROD)) {
        ss << _("(充填中)", " (charging)");
    }

    return ss.str();
}

static void decide_item_feeling(flavor_type *flavor_ptr)
{
    flavor_ptr->fake_insc_buf[0] = '\0';
    if (flavor_ptr->o_ptr->feeling) {
        strcpy(flavor_ptr->fake_insc_buf, game_inscriptions[flavor_ptr->o_ptr->feeling]);
        return;
    }

    if (flavor_ptr->o_ptr->is_cursed() && (flavor_ptr->known || (flavor_ptr->o_ptr->ident & IDENT_SENSE))) {
        strcpy(flavor_ptr->fake_insc_buf, _("呪われている", "cursed"));
        return;
    }

    const auto tval = flavor_ptr->o_ptr->bi_key.tval();
    auto unidentifiable = tval == ItemKindType::RING;
    unidentifiable |= tval == ItemKindType::AMULET;
    unidentifiable |= tval == ItemKindType::LITE;
    unidentifiable |= tval == ItemKindType::FIGURINE;
    if (unidentifiable && flavor_ptr->aware && !flavor_ptr->known && !(flavor_ptr->o_ptr->ident & IDENT_SENSE)) {
        strcpy(flavor_ptr->fake_insc_buf, _("未鑑定", "unidentified"));
        return;
    }

    if (!flavor_ptr->known && (flavor_ptr->o_ptr->ident & IDENT_EMPTY)) {
        strcpy(flavor_ptr->fake_insc_buf, _("空", "empty"));
        return;
    }

    if (!flavor_ptr->aware && flavor_ptr->o_ptr->is_tried()) {
        strcpy(flavor_ptr->fake_insc_buf, _("未判明", "tried"));
    }
}

static describe_option_type decide_describe_option(const ItemEntity &item, BIT_FLAGS mode)
{
    describe_option_type opt{};
    opt.mode = mode;
    opt.flavor = true;

    if (item.is_aware()) {
        opt.aware = true;
    }

    if (item.is_known()) {
        opt.known = true;
    }

    if (opt.aware && (any_bits(mode, OD_NO_FLAVOR) || plain_descriptions)) {
        opt.flavor = false;
    }

    if (any_bits(mode, OD_STORE) || any_bits(item.ident, IDENT_STORE)) {
        opt.flavor = false;
        opt.aware = true;
        opt.known = true;
    }

    if (any_bits(mode, OD_FORCE_FLAVOR)) {
        opt.aware = false;
        opt.flavor = true;
        opt.known = false;
    }

    return opt;
}

/*!
 * @brief オブジェクトの各表記を返すメイン関数 / Creates a description of the item "o_ptr", and stores it in "out_val".
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param buf 表記を返すための文字列参照ポインタ
 * @param o_ptr 特性短縮表記を得たいオブジェクト構造体の参照ポインタ
 * @param mode 表記に関するオプション指定
 * @return 現在クエスト達成目的のアイテムならばTRUEを返す
 */
void describe_flavor(PlayerType *player_ptr, char *buf, ItemEntity *o_ptr, BIT_FLAGS mode)
{
    flavor_type tmp_flavor;
    flavor_type *flavor_ptr = initialize_flavor_type(&tmp_flavor, buf, o_ptr, mode);

    check_object_known_aware(flavor_ptr);
    const auto &item = *o_ptr;
    const auto opt = decide_describe_option(item, mode);
    std::stringstream desc_ss;
    desc_ss << describe_named_item(player_ptr, item, opt);

    if (flavor_ptr->mode & OD_NAME_ONLY || o_ptr->bi_id == 0) {
        angband_strcpy(buf, desc_ss.str().data(), MAX_NLEN);
        return;
    }

    desc_ss << describe_chest(item, opt)
            << describe_weapon_dice_or_bow_power(player_ptr, item, opt)
            << describe_accuracy_and_damage_bonus(item, opt);

    if (none_bits(mode, OD_DEBUG)) {
        const auto &bow = player_ptr->inventory_list[INVEN_BOW];
        const auto tval = flavor_ptr->o_ptr->bi_key.tval();
        if ((bow.bi_id != 0) && (tval == bow.get_arrow_kind())) {
            desc_ss << describe_ammo_detail(player_ptr, item, bow, opt);
        } else if (PlayerClass(player_ptr).equals(PlayerClassType::NINJA) && (tval == ItemKindType::SPIKE)) {
            desc_ss << describe_spike_detail(player_ptr);
        }
    }

    desc_ss << describe_ac(item, opt);
    if (any_bits(mode, OD_NAME_AND_ENCHANT)) {
        angband_strcpy(buf, desc_ss.str().data(), MAX_NLEN);
        return;
    }

    desc_ss << describe_remaining(item, opt);
    if (any_bits(mode, OD_OMIT_INSCRIPTION)) {
        angband_strcpy(buf, desc_ss.str().data(), MAX_NLEN);
        return;
    }

    // ここまでのリファクタリングを確認するための暫定措置
    angband_strcpy(flavor_ptr->tmp_val, desc_ss.str().data(), sizeof(flavor_ptr->tmp_val));
    flavor_ptr->t = flavor_ptr->tmp_val + strlen(flavor_ptr->tmp_val);

    display_short_flavors(flavor_ptr);
    decide_item_feeling(flavor_ptr);
    display_item_discount(flavor_ptr);
    display_item_fake_inscription(flavor_ptr);
    angband_strcpy(flavor_ptr->buf, flavor_ptr->tmp_val, MAX_NLEN);
}
