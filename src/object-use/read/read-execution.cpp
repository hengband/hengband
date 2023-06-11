/*!
 * @brief 巻物を読んだ際の効果処理
 * @date 2022/02/26
 * @author Hourier
 */

#include "object-use/read/read-execution.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "core/window-redrawer.h"
#include "inventory/inventory-object.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-use/item-use-checker.h"
#include "object-use/read/read-executor-factory.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-status/player-energy.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/experience.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item 読むオブジェクトの所持品ID
 */
ObjectReadEntity::ObjectReadEntity(PlayerType *player_ptr, INVENTORY_IDX item)
    : player_ptr(player_ptr)
    , item(item)
{
}

/*!
 * @brief 巻物を読む
 * @param known 判明済ならばTRUE
 */
void ObjectReadEntity::execute(bool known)
{
    auto *o_ptr = ref_item(this->player_ptr, this->item);
    PlayerEnergy(this->player_ptr).set_player_turn_energy(100);
    if (!this->can_read()) {
        return;
    }

    if (music_singing_any(this->player_ptr)) {
        stop_singing(this->player_ptr);
    }

    SpellHex spell_hex(this->player_ptr);
    if (spell_hex.is_spelling_any() && ((this->player_ptr->lev < 35) || spell_hex.is_casting_full_capacity())) {
        (void)SpellHex(this->player_ptr).stop_all_spells();
    }

    auto executor = ReadExecutorFactory::create(player_ptr, o_ptr, known);
    auto used_up = executor->read();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    using Srf = StatusRecalculatingFlag;
    EnumClassFlagGroup<Srf> flags_srf = { Srf::COMBINATION, Srf::REORDER };
    if (rfu.has(Srf::AUTO_DESTRUCTION)) {
        flags_srf.set(Srf::AUTO_DESTRUCTION);
    }

    rfu.reset_flags(flags_srf);
    this->change_virtue_as_read(*o_ptr);
    object_tried(o_ptr);
    this->gain_exp_from_item_use(o_ptr, executor->is_identified());
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
    };
    rfu.set_flags(flags_swrf);
    rfu.set_flags(flags_srf);
    if (!used_up) {
        return;
    }

    sound(SOUND_SCROLL);
    vary_item(this->player_ptr, this->item, -1);
}

bool ObjectReadEntity::can_read() const
{
    if (cmd_limit_time_walk(this->player_ptr)) {
        return false;
    }

    if (PlayerClass(this->player_ptr).equals(PlayerClassType::BERSERKER)) {
        msg_print(_("巻物なんて読めない。", "You cannot read."));
        return false;
    }

    return ItemUseChecker(this->player_ptr).check_stun(_("朦朧としていて読めなかった！", "You too stunned to read it!"));
}

void ObjectReadEntity::change_virtue_as_read(ItemEntity &o_ref)
{
    if (o_ref.is_aware()) {
        return;
    }

    chg_virtue(this->player_ptr, Virtue::PATIENCE, -1);
    chg_virtue(this->player_ptr, Virtue::CHANCE, 1);
    chg_virtue(this->player_ptr, Virtue::KNOWLEDGE, -1);
}

void ObjectReadEntity::gain_exp_from_item_use(ItemEntity *o_ptr, bool is_identified)
{
    if (!is_identified || o_ptr->is_aware()) {
        return;
    }

    object_aware(this->player_ptr, o_ptr);
    auto lev = o_ptr->get_baseitem().level;
    gain_exp(this->player_ptr, (lev + (this->player_ptr->lev >> 1)) / this->player_ptr->lev);
}
