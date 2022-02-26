/*!
 * @brief 巻物を読んだ際の効果処理
 * @date 2020/07/23
 * @author Hourier
 * @todo 長い、要分割
 */

#include "object-use/read/read-execution.h"
#include "action/action-limited.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/show-file.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "object-use/item-use-checker.h"
#include "object-use/read/read-executor-factory.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-enchant.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spells-object.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "store/rumor.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
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
    if (!this->check_can_read()) {
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
    BIT_FLAGS inventory_flags = PU_COMBINE | PU_REORDER | (this->player_ptr->update & PU_AUTODESTROY);
    reset_bits(this->player_ptr->update, PU_COMBINE | PU_REORDER | PU_AUTODESTROY);
    this->change_virtue_as_read(*o_ptr);
    object_tried(o_ptr);
    this->gain_exp_from_item_use(o_ptr, executor->is_identified());
    this->player_ptr->window_flags |= PW_INVEN | PW_EQUIP | PW_PLAYER;
    this->player_ptr->update |= inventory_flags;
    if (!used_up) {
        return;
    }

    sound(SOUND_SCROLL);
    vary_item(this->player_ptr, this->item, -1);
}

bool ObjectReadEntity::check_can_read()
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

void ObjectReadEntity::change_virtue_as_read(ObjectType &o_ref)
{
    if (o_ref.is_aware()) {
        return;
    }

    chg_virtue(this->player_ptr, V_PATIENCE, -1);
    chg_virtue(this->player_ptr, V_CHANCE, 1);
    chg_virtue(this->player_ptr, V_KNOWLEDGE, -1);
}

void ObjectReadEntity::gain_exp_from_item_use(ObjectType *o_ptr, bool is_identified)
{
    if (!is_identified || o_ptr->is_aware()) {
        return;
    }

    object_aware(this->player_ptr, o_ptr);
    auto lev = k_info[o_ptr->k_idx].level;
    gain_exp(this->player_ptr, (lev + (this->player_ptr->lev >> 1)) / this->player_ptr->lev);
}
