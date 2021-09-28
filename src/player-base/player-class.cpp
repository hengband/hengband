/*!
 * @brief プレイヤーの職業クラスに基づく耐性・能力の判定処理等を行うクラス
 * @date 2021/09/08
 * @author Hourier
 * @details 本クラス作成時点で責務に対する余裕はかなりあるので、適宜ここへ移してくること.
 */
#include "player-base/player-class.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-elementalist.h"
#include "player-info/bard-data-type.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/equipment-info.h"
#include "player-info/force-trainer-data-type.h"
#include "player-info/magic-eater-data-type.h"
#include "player-info/mane-data-type.h"
#include "player-info/smith-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player-info/spell-hex-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "realm/realm-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

PlayerClass::PlayerClass(player_type *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief プレイヤーの職業により得られる特性フラグを返す
 */
TrFlags PlayerClass::tr_flags() const
{
    TrFlags flags;
    const auto plev = this->player_ptr->lev;

    switch (this->player_ptr->pclass) {
    case CLASS_WARRIOR: {
        if (plev > 29)
            flags.set(TR_RES_FEAR);
        if (plev > 44)
            flags.set(TR_REGEN);

        break;
    }
    case CLASS_SAMURAI: {
        if (plev > 29)
            flags.set(TR_RES_FEAR);

        break;
    }
    case CLASS_PALADIN: {
        if (plev > 39)
            flags.set(TR_RES_FEAR);

        break;
    }
    case CLASS_CHAOS_WARRIOR: {
        if (plev > 29)
            flags.set(TR_RES_CHAOS);
        if (plev > 39)
            flags.set(TR_RES_FEAR);

        break;
    }
    case CLASS_MONK:
    case CLASS_FORCETRAINER: {
        if ((plev > 9) && !heavy_armor(this->player_ptr))
            flags.set(TR_SPEED);
        if ((plev > 24) && !heavy_armor(this->player_ptr))
            flags.set(TR_FREE_ACT);

        break;
    }
    case CLASS_NINJA: {
        if (heavy_armor(this->player_ptr)) {
            flags.set(TR_SPEED);
        } else {
            if ((!this->player_ptr->inventory_list[INVEN_MAIN_HAND].k_idx || can_attack_with_main_hand(this->player_ptr))
                && (!this->player_ptr->inventory_list[INVEN_SUB_HAND].k_idx || can_attack_with_sub_hand(this->player_ptr)))
                flags.set(TR_SPEED);
            if (plev > 24 && !this->player_ptr->is_icky_wield[0] && !this->player_ptr->is_icky_wield[1])
                flags.set(TR_FREE_ACT);
        }

        flags.set(TR_SLOW_DIGEST);
        flags.set(TR_RES_FEAR);
        if (plev > 19)
            flags.set(TR_RES_POIS);
        if (plev > 24)
            flags.set(TR_SUST_DEX);
        if (plev > 29)
            flags.set(TR_SEE_INVIS);

        break;
    }
    case CLASS_MINDCRAFTER: {
        if (plev > 9)
            flags.set(TR_RES_FEAR);
        if (plev > 19)
            flags.set(TR_SUST_WIS);
        if (plev > 29)
            flags.set(TR_RES_CONF);
        if (plev > 39)
            flags.set(TR_TELEPATHY);

        break;
    }
    case CLASS_BARD: {
        flags.set(TR_RES_SOUND);
        break;
    }
    case CLASS_BERSERKER: {
        flags.set(TR_SUST_STR);
        flags.set(TR_SUST_DEX);
        flags.set(TR_SUST_CON);
        flags.set(TR_REGEN);
        flags.set(TR_FREE_ACT);
        flags.set(TR_SPEED);
        if (plev > 39)
            flags.set(TR_REFLECT);

        break;
    }
    case CLASS_MIRROR_MASTER: {
        if (plev > 39)
            flags.set(TR_REFLECT);

        break;
    }
    case CLASS_ELEMENTALIST:
        if (has_element_resist(this->player_ptr, ElementRealm::FIRE, 1))
            flags.set(TR_RES_FIRE);
        if (has_element_resist(this->player_ptr, ElementRealm::FIRE, 30))
            flags.set(TR_IM_FIRE);
        if (has_element_resist(this->player_ptr, ElementRealm::ICE, 1))
            flags.set(TR_RES_COLD);
        if (has_element_resist(this->player_ptr, ElementRealm::ICE, 30))
            flags.set(TR_IM_COLD);
        if (has_element_resist(this->player_ptr, ElementRealm::SKY, 1))
            flags.set(TR_RES_ELEC);
        if (has_element_resist(this->player_ptr, ElementRealm::SKY, 30))
            flags.set(TR_IM_ELEC);
        if (has_element_resist(this->player_ptr, ElementRealm::SEA, 1))
            flags.set(TR_RES_ACID);
        if (has_element_resist(this->player_ptr, ElementRealm::SEA, 30))
            flags.set(TR_IM_ACID);
        if (has_element_resist(this->player_ptr, ElementRealm::DARKNESS, 1))
            flags.set(TR_RES_DARK);
        if (has_element_resist(this->player_ptr, ElementRealm::DARKNESS, 30))
            flags.set(TR_RES_NETHER);
        if (has_element_resist(this->player_ptr, ElementRealm::CHAOS, 1))
            flags.set(TR_RES_CONF);
        if (has_element_resist(this->player_ptr, ElementRealm::CHAOS, 30))
            flags.set(TR_RES_CHAOS);
        if (has_element_resist(this->player_ptr, ElementRealm::EARTH, 1))
            flags.set(TR_RES_SHARDS);
        if (has_element_resist(this->player_ptr, ElementRealm::EARTH, 30))
            flags.set(TR_REFLECT);
        if (has_element_resist(this->player_ptr, ElementRealm::DEATH, 1))
            flags.set(TR_RES_POIS);
        if (has_element_resist(this->player_ptr, ElementRealm::DEATH, 30))
            flags.set(TR_RES_DISEN);
        break;
    default:
        break;
    }

    return flags;
}

bool PlayerClass::can_resist_stun() const
{
    return (this->player_ptr->pclass == CLASS_BERSERKER) && (this->player_ptr->lev > 34);
}

bool PlayerClass::is_wizard() const
{
    auto is_wizard = this->player_ptr->pclass == CLASS_MAGE;
    is_wizard |= this->player_ptr->pclass == CLASS_HIGH_MAGE;
    is_wizard |= this->player_ptr->pclass == CLASS_SORCERER;
    is_wizard |= this->player_ptr->pclass == CLASS_MAGIC_EATER;
    is_wizard |= this->player_ptr->pclass == CLASS_BLUE_MAGE;
    is_wizard |= this->player_ptr->pclass == CLASS_ELEMENTALIST;
    return is_wizard;
}

bool PlayerClass::lose_balance()
{
    if (this->player_ptr->pclass != CLASS_SAMURAI) {
        return false;
    }

    if (none_bits(this->player_ptr->special_defense, KATA_MASK)) {
        return false;
    }

    reset_bits(this->player_ptr->special_defense, KATA_MASK);
    this->player_ptr->update |= PU_BONUS;
    this->player_ptr->update |= PU_MONSTERS;
    this->player_ptr->redraw |= PR_STATE;
    this->player_ptr->redraw |= PR_STATUS;
    this->player_ptr->action = ACTION_NONE;
    return true;
}

/**
 * @brief プレイヤーの職業にで使用する職業固有データ領域を初期化する
 * @details 事前条件: player_type の職業や領域が決定済みであること
 */
void PlayerClass::init_specific_data()
{
    switch (this->player_ptr->pclass) {
    case CLASS_SMITH:
        this->player_ptr->class_specific_data = std::make_shared<smith_data_type>();
        break;
    case CLASS_FORCETRAINER:
        this->player_ptr->class_specific_data = std::make_shared<force_trainer_data_type>();
        break;
    case CLASS_BLUE_MAGE:
        this->player_ptr->class_specific_data = std::make_shared<bluemage_data_type>();
        break;
    case CLASS_MAGIC_EATER:
        this->player_ptr->class_specific_data = std::make_shared<magic_eater_data_type>();
        break;
    case CLASS_BARD:
        this->player_ptr->class_specific_data = std::make_shared<bard_data_type>();
        break;
    case CLASS_IMITATOR:
        this->player_ptr->class_specific_data = std::make_shared<mane_data_type>();
        break;
    case CLASS_SNIPER:
        this->player_ptr->class_specific_data = std::make_shared<sniper_data_type>();
        break;
    case CLASS_HIGH_MAGE:
        if (this->player_ptr->realm1 == REALM_HEX) {
            this->player_ptr->class_specific_data = std::make_shared<spell_hex_data_type>();
        } else {
            this->player_ptr->class_specific_data = no_class_specific_data();
        }
        break;
    default:
        this->player_ptr->class_specific_data = no_class_specific_data();
        break;
    }
}
