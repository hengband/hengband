/*!
 * @brief 薬を飲んだ時の各種効果処理
 * @date 2022/03/10
 * @author Hourier
 */

#include "object-use/quaff/quaff-execution.h"
#include "avatar/avatar.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-object.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-use/item-use-checker.h"
#include "object-use/quaff/quaff-effects.h"
#include "object/object-broken.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-race.h"
#include "player-info/mimic-info-table.h"
#include "player-status/player-energy.h"
#include "player/digestion-processor.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/experience.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 */
ObjectQuaffEntity::ObjectQuaffEntity(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief 薬を飲む.
 * @param i_idx 薬のインベントリID
 * @details
 * 効果発動のあと、食料タイプによって空腹度を少し充足する。
 * 但し骸骨は除く
 */
void ObjectQuaffEntity::execute(INVENTORY_IDX i_idx)
{
    if (!this->can_influence()) {
        return;
    }

    auto item = this->copy_object(i_idx);
    vary_item(this->player_ptr, i_idx, -1);
    sound(SOUND_QUAFF);
    auto ident = QuaffEffects(this->player_ptr).influence(item);
    if (PlayerRace(this->player_ptr).equals(PlayerRaceType::SKELETON)) {
        msg_print(_("液体の一部はあなたのアゴを素通りして落ちた！", "Some of the fluid falls through your jaws!"));
        (void)potion_smash_effect(this->player_ptr, 0, this->player_ptr->y, this->player_ptr->x, item.bi_id);
    }

    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flags(flags_srf);
    this->change_virtue_as_quaff(item);
    item.mark_as_tried();
    if (ident && !item.is_aware()) {
        object_aware(this->player_ptr, &item);
        gain_exp(this->player_ptr, (item.get_baseitem().level + (this->player_ptr->lev >> 1)) / this->player_ptr->lev);
    }

    static constexpr auto flags = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
    };
    rfu.set_flags(flags);
    if (PlayerRace(this->player_ptr).equals(PlayerRaceType::SKELETON)) {
        return;
    }

    this->moisten(item);
}

bool ObjectQuaffEntity::can_influence()
{
    PlayerEnergy(this->player_ptr).set_player_turn_energy(100);
    if (!this->can_quaff()) {
        return false;
    }

    if (music_singing_any(this->player_ptr)) {
        stop_singing(this->player_ptr);
    }

    SpellHex spell_hex(this->player_ptr);
    if (spell_hex.is_spelling_any() && !spell_hex.is_spelling_specific(HEX_INHALE)) {
        (void)SpellHex(this->player_ptr).stop_all_spells();
    }

    return true;
}

bool ObjectQuaffEntity::can_quaff()
{
    if (this->player_ptr->timewalk) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("瓶から水が流れ出てこない！", "The potion doesn't flow out from the bottle."));
        sound(SOUND_FAIL);
        return false;
    }

    return ItemUseChecker(this->player_ptr).check_stun(_("朦朧としていて瓶の蓋を開けられなかった！", "You are too stunned to quaff it!"));
}

ItemEntity ObjectQuaffEntity::copy_object(const INVENTORY_IDX i_idx)
{
    auto *tmp_o_ptr = ref_item(this->player_ptr, i_idx);
    auto o_val = *tmp_o_ptr;
    o_val.number = 1;
    return o_val;
}

void ObjectQuaffEntity::moisten(const ItemEntity &o_ref)
{
    switch (PlayerRace(this->player_ptr).food()) {
    case PlayerRaceFoodType::WATER:
        msg_print(_("水分を取り込んだ。", "You are moistened."));
        set_food(this->player_ptr, std::min<short>(this->player_ptr->food + o_ref.pval + std::max<short>(0, o_ref.pval * 10) + 2000, PY_FOOD_MAX - 1));
        return;
    case PlayerRaceFoodType::OIL:
        if (o_ref.bi_key.tval() != ItemKindType::FLASK) {
            set_food(this->player_ptr, this->player_ptr->food + ((o_ref.pval) / 20));
            return;
        }

        msg_print(_("オイルを補給した。", "You replenish yourself with the oil."));
        set_food(this->player_ptr, this->player_ptr->food + 5000);
        return;
    case PlayerRaceFoodType::BLOOD:
        (void)set_food(this->player_ptr, this->player_ptr->food + (o_ref.pval / 10));
        return;
    case PlayerRaceFoodType::MANA:
    case PlayerRaceFoodType::CORPSE:
        set_food(this->player_ptr, this->player_ptr->food + ((o_ref.pval) / 20));
        return;
    default:
        (void)set_food(this->player_ptr, this->player_ptr->food + o_ref.pval);
        return;
    }
}

void ObjectQuaffEntity::change_virtue_as_quaff(const ItemEntity &o_ref)
{
    if (o_ref.is_aware()) {
        return;
    }

    chg_virtue(this->player_ptr, Virtue::PATIENCE, -1);
    chg_virtue(this->player_ptr, Virtue::CHANCE, 1);
    chg_virtue(this->player_ptr, Virtue::KNOWLEDGE, -1);
}
