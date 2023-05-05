/*!
 * @brief プレイヤーの職業クラスに基づく耐性・能力の判定処理等を行うクラス
 * @date 2021/09/08
 * @author Hourier
 * @details PlayerRaceからPlayerClassへの依存はあるが、逆は依存させないこと.
 */
#include "player-base/player-class.h"
#include "core/player-redraw-types.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-elementalist.h"
#include "player-info/bard-data-type.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/equipment-info.h"
#include "player-info/force-trainer-data-type.h"
#include "player-info/magic-eater-data-type.h"
#include "player-info/mane-data-type.h"
#include "player-info/monk-data-type.h"
#include "player-info/ninja-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player-info/smith-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player-info/spell-hex-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "realm/realm-types.h"
#include "status/action-setter.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"

PlayerClass::PlayerClass(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

bool PlayerClass::equals(PlayerClassType type) const
{
    return this->player_ptr->pclass == type;
}

/*!
 * @brief プレイヤーの職業により得られる特性フラグを返す
 */
TrFlags PlayerClass::tr_flags() const
{
    TrFlags flags;
    const auto plev = this->player_ptr->lev;

    switch (this->player_ptr->pclass) {
    case PlayerClassType::WARRIOR: {
        if (plev > 29) {
            flags.set(TR_RES_FEAR);
        }
        if (plev > 44) {
            flags.set(TR_REGEN);
        }

        break;
    }
    case PlayerClassType::SAMURAI: {
        if (plev > 29) {
            flags.set(TR_RES_FEAR);
        }

        break;
    }
    case PlayerClassType::PALADIN: {
        if (plev > 39) {
            flags.set(TR_RES_FEAR);
        }

        break;
    }
    case PlayerClassType::CHAOS_WARRIOR: {
        if (plev > 29) {
            flags.set(TR_RES_CHAOS);
        }
        if (plev > 39) {
            flags.set(TR_RES_FEAR);
        }

        break;
    }
    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER: {
        if ((plev > 9) && !heavy_armor(this->player_ptr)) {
            flags.set(TR_SPEED);
        }
        if ((plev > 24) && !heavy_armor(this->player_ptr)) {
            flags.set(TR_FREE_ACT);
        }

        break;
    }
    case PlayerClassType::NINJA: {
        if (heavy_armor(this->player_ptr)) {
            flags.set(TR_SPEED);
        } else {
            if (this->has_ninja_speed()) {
                flags.set(TR_SPEED);
            }

            if (plev > 24 && !this->player_ptr->is_icky_wield[0] && !this->player_ptr->is_icky_wield[1]) {
                flags.set(TR_FREE_ACT);
            }
        }

        flags.set(TR_SLOW_DIGEST);
        flags.set(TR_RES_FEAR);
        if (plev > 19) {
            flags.set(TR_RES_POIS);
        }
        if (plev > 24) {
            flags.set(TR_SUST_DEX);
        }
        if (plev > 29) {
            flags.set(TR_SEE_INVIS);
        }

        break;
    }
    case PlayerClassType::MINDCRAFTER: {
        if (plev > 9) {
            flags.set(TR_RES_FEAR);
        }
        if (plev > 19) {
            flags.set(TR_SUST_WIS);
        }
        if (plev > 29) {
            flags.set(TR_RES_CONF);
        }
        if (plev > 39) {
            flags.set(TR_TELEPATHY);
        }

        break;
    }
    case PlayerClassType::BARD: {
        flags.set(TR_RES_SOUND);
        break;
    }
    case PlayerClassType::BERSERKER: {
        flags.set(TR_SUST_STR);
        flags.set(TR_SUST_DEX);
        flags.set(TR_SUST_CON);
        flags.set(TR_REGEN);
        flags.set(TR_FREE_ACT);
        flags.set(TR_SPEED);
        if (plev > 39) {
            flags.set(TR_REFLECT);
        }

        break;
    }
    case PlayerClassType::MIRROR_MASTER: {
        if (plev > 39) {
            flags.set(TR_REFLECT);
        }

        break;
    }
    case PlayerClassType::ELEMENTALIST:
        if (has_element_resist(this->player_ptr, ElementRealmType::FIRE, 1)) {
            flags.set(TR_RES_FIRE);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::FIRE, 30)) {
            flags.set(TR_IM_FIRE);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::ICE, 1)) {
            flags.set(TR_RES_COLD);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::ICE, 30)) {
            flags.set(TR_IM_COLD);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::SKY, 1)) {
            flags.set(TR_RES_ELEC);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::SKY, 30)) {
            flags.set(TR_IM_ELEC);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::SEA, 1)) {
            flags.set(TR_RES_ACID);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::SEA, 30)) {
            flags.set(TR_IM_ACID);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::DARKNESS, 1)) {
            flags.set(TR_RES_DARK);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::DARKNESS, 30)) {
            flags.set(TR_RES_NETHER);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::CHAOS, 1)) {
            flags.set(TR_RES_CONF);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::CHAOS, 30)) {
            flags.set(TR_RES_CHAOS);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::EARTH, 1)) {
            flags.set(TR_RES_SHARDS);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::EARTH, 30)) {
            flags.set(TR_REFLECT);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::DEATH, 1)) {
            flags.set(TR_RES_POIS);
        }
        if (has_element_resist(this->player_ptr, ElementRealmType::DEATH, 30)) {
            flags.set(TR_RES_DISEN);
        }
        break;
    default:
        break;
    }

    return flags;
}

TrFlags PlayerClass::stance_tr_flags() const
{
    TrFlags flags;

    switch (this->get_samurai_stance()) {
    case SamuraiStanceType::FUUJIN:
        if (!this->player_ptr->effects()->blindness()->is_blind()) {
            flags.set(TR_REFLECT);
        }
        break;
    case SamuraiStanceType::MUSOU:
        flags.set({ TR_RES_ACID, TR_RES_ELEC, TR_RES_FIRE, TR_RES_COLD, TR_RES_POIS });
        flags.set({ TR_REFLECT, TR_RES_FEAR, TR_RES_LITE, TR_RES_DARK, TR_RES_BLIND, TR_RES_CONF,
            TR_RES_SOUND, TR_RES_SHARDS, TR_RES_NETHER, TR_RES_NEXUS, TR_RES_CHAOS, TR_RES_DISEN,
            TR_RES_WATER, TR_RES_TIME, TR_RES_CURSE });
        flags.set({ TR_HOLD_EXP, TR_FREE_ACT, TR_LEVITATION, TR_LITE_1, TR_SEE_INVIS, TR_TELEPATHY, TR_SLOW_DIGEST, TR_REGEN });
        flags.set({ TR_SH_FIRE, TR_SH_ELEC, TR_SH_COLD });
        flags.set({ TR_SUST_STR, TR_SUST_INT, TR_SUST_WIS, TR_SUST_DEX, TR_SUST_CON, TR_SUST_CHR });
        break;
    case SamuraiStanceType::KOUKIJIN:
        flags.set({ TR_VUL_ACID, TR_VUL_ELEC, TR_VUL_FIRE, TR_VUL_COLD });
        break;
    default:
        break;
    }

    switch (this->get_monk_stance()) {
    case MonkStanceType::GENBU:
        flags.set(TR_REFLECT);
        break;
    case MonkStanceType::SUZAKU:
        flags.set(TR_LEVITATION);
        break;
    case MonkStanceType::SEIRYU:
        flags.set({ TR_RES_ACID, TR_RES_ELEC, TR_RES_FIRE, TR_RES_COLD, TR_RES_POIS });
        flags.set({ TR_SH_FIRE, TR_SH_ELEC, TR_SH_COLD });
        flags.set(TR_LEVITATION);
        break;
    default:
        break;
    }

    return flags;
}

bool PlayerClass::has_stun_immunity() const
{
    return this->equals(PlayerClassType::BERSERKER) && (this->player_ptr->lev > 34);
}

bool PlayerClass::has_poison_resistance() const
{
    return this->equals(PlayerClassType::NINJA) && (this->player_ptr->lev > 44);
}

/*!
 * @brief 加速ボーナスのある種族かを返す
 * @return 加速ボーナスのある種族か否か
 * @details
 * 種族と職業の両方で特性による加速が得られる場合、重複して加速することはない.
 * 代りに経験値補正が軽くなる.
 */
bool PlayerClass::has_additional_speed() const
{
    auto has_additional_speed = this->equals(PlayerClassType::MONK);
    has_additional_speed |= this->equals(PlayerClassType::FORCETRAINER);
    has_additional_speed |= this->equals(PlayerClassType::NINJA);
    return has_additional_speed;
}

bool PlayerClass::is_soldier() const
{
    auto is_soldier = this->equals(PlayerClassType::WARRIOR);
    is_soldier |= this->equals(PlayerClassType::ARCHER);
    is_soldier |= this->equals(PlayerClassType::CAVALRY);
    is_soldier |= this->equals(PlayerClassType::BERSERKER);
    is_soldier |= this->equals(PlayerClassType::SMITH);
    return is_soldier;
}

bool PlayerClass::is_wizard() const
{
    auto is_wizard = this->equals(PlayerClassType::MAGE);
    is_wizard |= this->equals(PlayerClassType::HIGH_MAGE);
    is_wizard |= this->equals(PlayerClassType::SORCERER);
    is_wizard |= this->equals(PlayerClassType::MAGIC_EATER);
    is_wizard |= this->equals(PlayerClassType::BLUE_MAGE);
    is_wizard |= this->equals(PlayerClassType::ELEMENTALIST);
    return is_wizard;
}

bool PlayerClass::is_tamer() const
{
    auto is_tamer = this->equals(PlayerClassType::BEASTMASTER);
    is_tamer |= this->equals(PlayerClassType::CAVALRY);
    return is_tamer;
}

bool PlayerClass::can_browse() const
{
    auto can_browse = this->equals(PlayerClassType::MINDCRAFTER);
    can_browse |= this->equals(PlayerClassType::BERSERKER);
    can_browse |= this->equals(PlayerClassType::NINJA);
    can_browse |= this->equals(PlayerClassType::MIRROR_MASTER);
    return can_browse;
}

bool PlayerClass::has_listed_magics() const
{
    auto has_listed_magics = this->can_browse();
    has_listed_magics |= this->equals(PlayerClassType::FORCETRAINER);
    has_listed_magics |= this->equals(PlayerClassType::ELEMENTALIST);
    return has_listed_magics;
}

/*!
 * @brief プレイ日記のタイトルが「最高の肉体を求めて」になり得るクラスを判定する
 * @return 該当のクラスか否か
 */
bool PlayerClass::is_tough() const
{
    auto is_tough = this->equals(PlayerClassType::WARRIOR);
    is_tough |= this->equals(PlayerClassType::MONK);
    is_tough |= this->equals(PlayerClassType::SAMURAI);
    is_tough |= this->equals(PlayerClassType::BERSERKER);
    return is_tough;
}

bool PlayerClass::is_martial_arts_pro() const
{
    auto is_martial_arts_pro = this->equals(PlayerClassType::MONK);
    is_martial_arts_pro |= this->equals(PlayerClassType::FORCETRAINER);
    return is_martial_arts_pro;
}

bool PlayerClass::is_every_magic() const
{
    auto is_every_magic = this->equals(PlayerClassType::SORCERER);
    is_every_magic |= this->equals(PlayerClassType::RED_MAGE);
    return is_every_magic;
}

/*!
 * @brief 「覚えた呪文の数」という概念を持つクラスかをチェックする.
 * @return 呪文の数を持つか否か
 */
bool PlayerClass::has_number_of_spells_learned() const
{
    auto has_number_of_spells_learned = this->equals(PlayerClassType::MAGE);
    has_number_of_spells_learned |= this->equals(PlayerClassType::PRIEST);
    has_number_of_spells_learned |= this->equals(PlayerClassType::ROGUE);
    has_number_of_spells_learned |= this->equals(PlayerClassType::RANGER);
    has_number_of_spells_learned |= this->equals(PlayerClassType::PALADIN);
    has_number_of_spells_learned |= this->equals(PlayerClassType::WARRIOR_MAGE);
    has_number_of_spells_learned |= this->equals(PlayerClassType::CHAOS_WARRIOR);
    has_number_of_spells_learned |= this->equals(PlayerClassType::MONK);
    has_number_of_spells_learned |= this->equals(PlayerClassType::HIGH_MAGE);
    has_number_of_spells_learned |= this->equals(PlayerClassType::TOURIST);
    has_number_of_spells_learned |= this->equals(PlayerClassType::BEASTMASTER);
    has_number_of_spells_learned |= this->equals(PlayerClassType::BARD);
    has_number_of_spells_learned |= this->equals(PlayerClassType::FORCETRAINER);
    return has_number_of_spells_learned;
}

bool PlayerClass::lose_balance()
{
    if (this->player_ptr->pclass != PlayerClassType::SAMURAI) {
        return false;
    }

    if (this->samurai_stance_is(SamuraiStanceType::NONE)) {
        return false;
    }

    this->set_samurai_stance(SamuraiStanceType::NONE);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto flags_srf = {
        StatusRedrawingFlag::BONUS,
        StatusRedrawingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags_srf);
    this->player_ptr->redraw |= PR_ACTION;
    this->player_ptr->redraw |= PR_TIMED_EFFECT;
    this->player_ptr->action = ACTION_NONE;
    return true;
}

SamuraiStanceType PlayerClass::get_samurai_stance() const
{
    auto samurai_data = this->get_specific_data<samurai_data_type>();
    if (!samurai_data) {
        return SamuraiStanceType::NONE;
    }

    return samurai_data->stance;
}

bool PlayerClass::samurai_stance_is(SamuraiStanceType stance) const
{
    return this->get_samurai_stance() == stance;
}

/**
 * @brief 剣術家の型を崩す
 *
 * @param stance_list 崩す型を指定する。取っている型が指定された型に含まれない場合は崩さない。
 */
void PlayerClass::break_samurai_stance(std::initializer_list<SamuraiStanceType> stance_list)
{
    auto samurai_data = this->get_specific_data<samurai_data_type>();
    if (!samurai_data) {
        return;
    }

    for (auto stance : stance_list) {
        if (samurai_data->stance == stance) {
            set_action(player_ptr, ACTION_NONE);
            samurai_data->stance = SamuraiStanceType::NONE;
            break;
        }
    }
}

void PlayerClass::set_samurai_stance(SamuraiStanceType stance) const
{
    auto samurai_data = this->get_specific_data<samurai_data_type>();
    if (!samurai_data) {
        return;
    }

    samurai_data->stance = stance;
}

MonkStanceType PlayerClass::get_monk_stance() const
{
    auto monk_data = this->get_specific_data<monk_data_type>();
    if (!monk_data) {
        return MonkStanceType::NONE;
    }

    return monk_data->stance;
}

bool PlayerClass::monk_stance_is(MonkStanceType stance) const
{
    return this->get_monk_stance() == stance;
}

void PlayerClass::set_monk_stance(MonkStanceType stance) const
{
    auto monk_data = this->get_specific_data<monk_data_type>();
    if (!monk_data) {
        return;
    }

    monk_data->stance = stance;
}

/**
 * @brief プレイヤーの職業にで使用する職業固有データ領域を初期化する
 * @details 事前条件: PlayerType の職業や領域が決定済みであること
 */
void PlayerClass::init_specific_data()
{
    switch (this->player_ptr->pclass) {
    case PlayerClassType::SMITH:
        this->player_ptr->class_specific_data = std::make_shared<smith_data_type>();
        break;
    case PlayerClassType::FORCETRAINER:
        this->player_ptr->class_specific_data = std::make_shared<force_trainer_data_type>();
        break;
    case PlayerClassType::BLUE_MAGE:
        this->player_ptr->class_specific_data = std::make_shared<bluemage_data_type>();
        break;
    case PlayerClassType::MAGIC_EATER:
        this->player_ptr->class_specific_data = std::make_shared<magic_eater_data_type>();
        break;
    case PlayerClassType::BARD:
        this->player_ptr->class_specific_data = std::make_shared<bard_data_type>();
        break;
    case PlayerClassType::IMITATOR:
        this->player_ptr->class_specific_data = std::make_shared<mane_data_type>();
        break;
    case PlayerClassType::SNIPER:
        this->player_ptr->class_specific_data = std::make_shared<sniper_data_type>();
        break;
    case PlayerClassType::SAMURAI:
        this->player_ptr->class_specific_data = std::make_shared<samurai_data_type>();
        break;
    case PlayerClassType::MONK:
        this->player_ptr->class_specific_data = std::make_shared<monk_data_type>();
        break;
    case PlayerClassType::NINJA:
        this->player_ptr->class_specific_data = std::make_shared<ninja_data_type>();
        break;
    case PlayerClassType::HIGH_MAGE:
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

bool PlayerClass::has_ninja_speed() const
{
    auto has_ninja_speed_main = !this->player_ptr->inventory_list[INVEN_MAIN_HAND].is_valid();
    has_ninja_speed_main |= can_attack_with_main_hand(this->player_ptr);
    auto has_ninja_speed_sub = !this->player_ptr->inventory_list[INVEN_SUB_HAND].is_valid();
    has_ninja_speed_sub |= can_attack_with_sub_hand(this->player_ptr);
    return has_ninja_speed_main && has_ninja_speed_sub;
}
