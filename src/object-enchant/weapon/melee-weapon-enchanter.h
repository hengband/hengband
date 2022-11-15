#pragma once

#include "object-enchant/weapon/abstract-weapon-enchanter.h"
#include "system/angband.h"
#include "util/flag-group.h"

/*! 近接攻撃武器の強化種別列挙体 */
enum class MeleeWeaponEnchantType {
    ONLY_MUNDANITY, //<! エンチャントを行わずデフォルト修正値で生成する。他のフラグがあっても全て無視される。
    MOD_SLAYING_BONUS, //<! 殺戮修正ボーナスの増減を行う
    ENABLE_BECOME_EGO, //<! エゴアイテムの生成を許可
    ENABLE_BECOME_RANDOM_ARTIFACT, //<! ランダムアーティファクトの生成を許可
    MAX,
};

using MeleeWeaponEnchantFlags = EnumClassFlagGroup<MeleeWeaponEnchantType>;

/*! 通常の近接武器強化フラグ(殺戮修正増減・エゴ化・ランダムアーティファクト化を許可) */
inline const MeleeWeaponEnchantFlags MELEE_WEAPON_NORMAL_ENCHANT_FLAGS{
    MeleeWeaponEnchantType::MOD_SLAYING_BONUS,
    MeleeWeaponEnchantType::ENABLE_BECOME_EGO,
    MeleeWeaponEnchantType::ENABLE_BECOME_RANDOM_ARTIFACT
};

class ObjectType;
class PlayerType;
class MeleeWeaponEnchanter : public AbstractWeaponEnchanter {
public:
    virtual ~MeleeWeaponEnchanter() = default;

    void apply_magic() final override;

protected:
    MeleeWeaponEnchanter(PlayerType *player_ptr, ObjectType *o_ptr, DEPTH level, int power);

    PlayerType *player_ptr;

private:
    virtual MeleeWeaponEnchantFlags enchant_flags() const = 0;

    void strengthen();
};
