/*!
 * @brief プレイヤーの職業クラスに基づく耐性・能力の判定処理等を行うクラス
 * @date 2021/09/08
 * @author Hourier
 */
#include "player-base/player-class.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "inventory/inventory-slot-types.h"
#include "mind/mind-elementalist.h"
#include "player-base/player-race.h"
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
    case PlayerClassType::WARRIOR: {
        if (plev > 29)
            flags.set(TR_RES_FEAR);
        if (plev > 44)
            flags.set(TR_REGEN);

        break;
    }
    case PlayerClassType::SAMURAI: {
        if (plev > 29)
            flags.set(TR_RES_FEAR);

        break;
    }
    case PlayerClassType::PALADIN: {
        if (plev > 39)
            flags.set(TR_RES_FEAR);

        break;
    }
    case PlayerClassType::CHAOS_WARRIOR: {
        if (plev > 29)
            flags.set(TR_RES_CHAOS);
        if (plev > 39)
            flags.set(TR_RES_FEAR);

        break;
    }
    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER: {
        if ((plev > 9) && !heavy_armor(this->player_ptr))
            flags.set(TR_SPEED);
        if ((plev > 24) && !heavy_armor(this->player_ptr))
            flags.set(TR_FREE_ACT);

        break;
    }
    case PlayerClassType::NINJA: {
        if (heavy_armor(this->player_ptr)) {
            flags.set(TR_SPEED);
        } else {
            if ((!this->player_ptr->inventory_list[INVEN_MAIN_HAND].k_idx || can_attack_with_main_hand(this->player_ptr)) && (!this->player_ptr->inventory_list[INVEN_SUB_HAND].k_idx || can_attack_with_sub_hand(this->player_ptr)))
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
    case PlayerClassType::MINDCRAFTER: {
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
        if (plev > 39)
            flags.set(TR_REFLECT);

        break;
    }
    case PlayerClassType::MIRROR_MASTER: {
        if (plev > 39)
            flags.set(TR_REFLECT);

        break;
    }
    case PlayerClassType::ELEMENTALIST:
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

TrFlags PlayerClass::stance_tr_flags() const
{
    TrFlags flags;

    switch (this->get_samurai_stance()) {
    case SamuraiStance::FUUJIN:
        if (!this->player_ptr->blind) {
            flags.set(TR_REFLECT);
        }
        break;
    case SamuraiStance::MUSOU:
        flags.set({ TR_RES_ACID, TR_RES_ELEC, TR_RES_FIRE, TR_RES_COLD, TR_RES_POIS });
        flags.set({ TR_REFLECT, TR_RES_FEAR, TR_RES_LITE, TR_RES_DARK, TR_RES_BLIND, TR_RES_CONF,
            TR_RES_SOUND, TR_RES_SHARDS, TR_RES_NETHER, TR_RES_NEXUS, TR_RES_CHAOS, TR_RES_DISEN,
            TR_RES_WATER, TR_RES_TIME, TR_RES_CURSE });
        flags.set({ TR_HOLD_EXP, TR_FREE_ACT, TR_LEVITATION, TR_LITE_1, TR_SEE_INVIS, TR_TELEPATHY, TR_SLOW_DIGEST, TR_REGEN });
        flags.set({ TR_SH_FIRE, TR_SH_ELEC, TR_SH_COLD });
        flags.set({ TR_SUST_STR, TR_SUST_INT, TR_SUST_WIS, TR_SUST_DEX, TR_SUST_CON, TR_SUST_CHR });
        break;
    case SamuraiStance::KOUKIJIN:
        flags.set({ TR_VUL_ACID, TR_VUL_ELEC, TR_VUL_FIRE, TR_VUL_COLD });
        break;
    default:
        break;
    }

    switch (this->get_monk_stance()) {
    case MonkStance::GENBU:
        flags.set(TR_REFLECT);
        break;
    case MonkStance::SUZAKU:
        flags.set(TR_LEVITATION);
        break;
    case MonkStance::SEIRYU:
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
    return (this->player_ptr->pclass == PlayerClassType::BERSERKER) && (this->player_ptr->lev > 34);
}

bool PlayerClass::is_wizard() const
{
    auto is_wizard = this->player_ptr->pclass == PlayerClassType::MAGE;
    is_wizard |= this->player_ptr->pclass == PlayerClassType::HIGH_MAGE;
    is_wizard |= this->player_ptr->pclass == PlayerClassType::SORCERER;
    is_wizard |= this->player_ptr->pclass == PlayerClassType::MAGIC_EATER;
    is_wizard |= this->player_ptr->pclass == PlayerClassType::BLUE_MAGE;
    is_wizard |= this->player_ptr->pclass == PlayerClassType::ELEMENTALIST;
    return is_wizard;
}

bool PlayerClass::lose_balance()
{
    if (this->player_ptr->pclass != PlayerClassType::SAMURAI) {
        return false;
    }

    if (this->samurai_stance_is(SamuraiStance::NONE)) {
        return false;
    }

    this->set_samurai_stance(SamuraiStance::NONE);
    this->player_ptr->update |= PU_BONUS;
    this->player_ptr->update |= PU_MONSTERS;
    this->player_ptr->redraw |= PR_STATE;
    this->player_ptr->redraw |= PR_STATUS;
    this->player_ptr->action = ACTION_NONE;
    return true;
}

SamuraiStance PlayerClass::get_samurai_stance() const
{
    auto samurai_data = this->get_specific_data<samurai_data_type>();
    if (!samurai_data) {
        return SamuraiStance::NONE;
    }

    return samurai_data->stance;
}

bool PlayerClass::samurai_stance_is(SamuraiStance stance) const
{
    return this->get_samurai_stance() == stance;
}

/**
 * @brief 剣術家の型を崩す
 *
 * @param stance_list 崩す型を指定する。取っている型が指定された型に含まれない場合は崩さない。
 */
void PlayerClass::break_samurai_stance(std::initializer_list<SamuraiStance> stance_list)
{
    auto samurai_data = this->get_specific_data<samurai_data_type>();
    if (!samurai_data) {
        return;
    }

    for (auto stance : stance_list) {
        if (samurai_data->stance == stance) {
            set_action(player_ptr, ACTION_NONE);
            samurai_data->stance = SamuraiStance::NONE;
            break;
        }
    }
}

void PlayerClass::set_samurai_stance(SamuraiStance stance) const
{
    auto samurai_data = this->get_specific_data<samurai_data_type>();
    if (!samurai_data) {
        return;
    }

    samurai_data->stance = stance;
}

MonkStance PlayerClass::get_monk_stance() const
{
    auto monk_data = this->get_specific_data<monk_data_type>();
    if (!monk_data) {
        return MonkStance::NONE;
    }

    return monk_data->stance;
}

bool PlayerClass::monk_stance_is(MonkStance stance) const
{
    return this->get_monk_stance() == stance;
}

void PlayerClass::set_monk_stance(MonkStance stance) const
{
    auto monk_data = this->get_specific_data<monk_data_type>();
    if (!monk_data) {
        return;
    }

    monk_data->stance = stance;
}

/**
 * @brief プレイヤーの職業にで使用する職業固有データ領域を初期化する
 * @details 事前条件: player_type の職業や領域が決定済みであること
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

/*!
 * @brief プレイヤーの職業情報テーブルへのポインタを取得する
 *
 * @return プレイヤーの職業情報テーブルへのポインタ
 */
const player_class_info *PlayerClass::get_info() const
{
    return &class_info[enum2i(this->player_ptr->pclass)];
}

/*!
 * @brief 速度計算 - 職業
 * @return 速度値の増減分
 * @details
 * ** 忍者の装備が重ければ減算(-レベル/10)
 * ** 忍者の装備が適正ならば加算(+3)さらにクラッコン、妖精、いかさま以外なら加算(+レベル/10)
 * ** 錬気術師で装備が重くなくクラッコン、妖精、いかさま以外なら加算(+レベル/10)
 * ** 狂戦士なら加算(+3),レベル20/30/40/50ごとに+1
 */
int16_t PlayerClass::additional_speed() const
{
    SPEED result = 0;

    if (this->player_ptr->pclass == PlayerClassType::NINJA) {
        if (heavy_armor(this->player_ptr)) {
            result -= (this->player_ptr->lev) / 10;
        } else if ((!this->player_ptr->inventory_list[INVEN_MAIN_HAND].k_idx || can_attack_with_main_hand(this->player_ptr)) && (!this->player_ptr->inventory_list[INVEN_SUB_HAND].k_idx || can_attack_with_sub_hand(this->player_ptr))) {
            result += 3;
            if (!(PlayerRace(this->player_ptr).equals(PlayerRaceType::KLACKON) || PlayerRace(this->player_ptr).equals(PlayerRaceType::SPRITE) || (this->player_ptr->ppersonality == PERSONALITY_MUNCHKIN)))
                result += (this->player_ptr->lev) / 10;
        }
    }

    if ((this->player_ptr->pclass == PlayerClassType::MONK || this->player_ptr->pclass == PlayerClassType::FORCETRAINER) && !(heavy_armor(this->player_ptr))) {
        if (!(PlayerRace(this->player_ptr).equals(PlayerRaceType::KLACKON) || PlayerRace(this->player_ptr).equals(PlayerRaceType::SPRITE) || (this->player_ptr->ppersonality == PERSONALITY_MUNCHKIN)))
            result += (this->player_ptr->lev) / 10;
    }

    if (this->player_ptr->pclass == PlayerClassType::BERSERKER) {
        result += 2;
        if (this->player_ptr->lev > 29)
            result++;
        if (this->player_ptr->lev > 39)
            result++;
        if (this->player_ptr->lev > 44)
            result++;
        if (this->player_ptr->lev > 49)
            result++;
    }
    return result;
}

/*!
 * @brief 速度計算 - 型
 * @return 速度値の増減分
 * @details
 * ** 朱雀の構えなら加算(+10)
 */
int16_t PlayerClass::stance_speed() const
{
    int16_t result = 0;
    if (this->monk_stance_is(MonkStance::SUZAKU))
        result += 10;
    return result;
}

/*!
 * @brief 隠密能力計算 - 職業
 * @return 隠密能力
 * @details
 * * 忍者がheavy_armorならば減算(-レベル/10)
 * * 忍者がheavy_armorでなく適正な武器を持っていれば加算(+レベル/10)
 */
int16_t PlayerClass::stealth_value() const
{
    ACTION_SKILL_POWER result = 0;

    const player_class_info *c_ptr = this->get_info();
    result = c_ptr->c_stl + (c_ptr->x_stl * this->player_ptr->lev / 10);

    if (this->player_ptr->pclass == PlayerClassType::NINJA) {
        if (heavy_armor(this->player_ptr)) {
            result -= (this->player_ptr->lev) / 10;
        } else if ((!this->player_ptr->inventory_list[INVEN_MAIN_HAND].k_idx || can_attack_with_main_hand(this->player_ptr)) 
                && (!this->player_ptr->inventory_list[INVEN_SUB_HAND].k_idx || can_attack_with_sub_hand(this->player_ptr))) {
            result += (this->player_ptr->lev) / 10;
        }
    }

    return result;
}


/*!
 * @brief 腕力補正計算 - 型
 * @return 腕力補正値
 * @details
 * * 型による腕力修正値
 * * 降鬼陣で加算(+5)
 * * 白虎の構えで加算(+2)
 * * 朱雀の構えで減算(-2)
 */
int16_t PlayerClass::stance_strength() const
{
    int16_t result = 0;

    if (this->samurai_stance_is(SamuraiStance::KOUKIJIN)) {
        result += 5;
    }

    if (this->monk_stance_is(MonkStance::BYAKKO)) {
        result += 2;
    } else if (this->monk_stance_is(MonkStance::SUZAKU)) {
        result -= 2;
    }

    return result;
}

/*!
 * @brief 知力補正計算 - 型
 * @return 知力補正値
 * @details
 * * 型による知力修正値
 * * 降鬼陣で加算(+5)
 * * 玄武の構えで減算(-1)
 * * 朱雀の構えで加算(+1)
 */
int16_t PlayerClass::stance_intelligence() const
{
    int16_t result = 0;

    if (this->samurai_stance_is(SamuraiStance::KOUKIJIN)) {
        result += 5;
    }

    if (this->monk_stance_is(MonkStance::GENBU)) {
        result -= 1;
    } else if (this->monk_stance_is(MonkStance::SUZAKU)) {
        result += 1;
    }

    return result;
}

/*!
 * @brief 賢さ補正計算 - 型
 * @return 賢さ補正値
 * @details
 * * 型による賢さ修正値
 * * 降鬼陣で加算(+5)
 * * 玄武の構えで減算(-1)
 * * 朱雀の構えで加算(+1)
 */
int16_t PlayerClass::stance_wisdom() const
{
    int16_t result = 0;

    if (this->samurai_stance_is(SamuraiStance::KOUKIJIN)) {
        result += 5;
    }

    if (this->monk_stance_is(MonkStance::GENBU)) {
        result -= 1;
    } else if (this->monk_stance_is(MonkStance::SUZAKU)) {
        result += 1;
    }

    return result;
}

/*!
 * @brief 器用さ補正計算 - 型
 * @return 器用さ補正値
 * @details
 * * 型による器用さ修正値
 * * 降鬼陣で加算(+5)
 * * 白虎の構えで加算(+2)
 * * 玄武の構えで減算(-2)
 * * 朱雀の構えで加算(+2)
 */
int16_t PlayerClass::stance_dexterity() const
{
    int16_t result = 0;

    if (this->samurai_stance_is(SamuraiStance::KOUKIJIN)) {
        result += 5;
    }

    if (this->monk_stance_is(MonkStance::BYAKKO)) {
        result += 2;
    } else if (this->monk_stance_is(MonkStance::GENBU)) {
        result -= 2;
    } else if (this->monk_stance_is(MonkStance::SUZAKU)) {
        result += 2;
    }

    return result;
}


/*!
 * @brief 耐久力補正計算 - 型
 * @return 耐久力補正値
 * @details
 * * 型による耐久力修正値
 * * 降鬼陣で加算(+5)
 * * 白虎の構えで減算(-3)
 * * 玄武の構えで加算(+3)
 * * 朱雀の構えで減算(-2)
 */
int16_t PlayerClass::stance_constitution() const
{
    int16_t result = 0;

    if (this->samurai_stance_is(SamuraiStance::KOUKIJIN)) {
        result += 5;
    }

    if (this->monk_stance_is(MonkStance::BYAKKO)) {
        result -= 3;
    } else if (this->monk_stance_is(MonkStance::GENBU)) {
        result += 3;
    } else if (this->monk_stance_is(MonkStance::SUZAKU)) {
        result -= 2;
    }

    return result;
}


/*!
 * @brief 魅力補正計算 - 型
 * @return 魅力補正値
 * @details
 * * 型による魅力修正値
 * * 降鬼陣で加算(+5)
 */
int16_t PlayerClass::stance_charisma() const
{
    int16_t result = 0;

    if (this->samurai_stance_is(SamuraiStance::KOUKIJIN)) {
        result += 5;
    }

    return result;
}
