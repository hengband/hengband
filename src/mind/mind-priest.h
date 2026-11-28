#pragma once

#include "object-enchant/tr-types.h"

/*! 武器に祝福を与えようとした結果 */
enum class BlessWeaponResult {
    REJECTED, //!< もう一方の神の祝福を受けた武器に拒まれた
    DISTURBED, //!< 呪いに邪魔されて祝福できなかった
    ALREADY_BLESSED, //!< 既に祝福されていた
    BLESSED, //!< 祝福した
    RESISTED, //!< 武器に抵抗されて祝福できなかった
};

/*! 武器に祝福を与えようとした結果と、その途中で武器に起きたこと */
struct BlessWeaponOutcome {
    BlessWeaponResult result; //!< 結果
    bool is_curse_removed; //!< 呪いを解いた
    bool is_disenchanted; //!< 抵抗されて修正値が下がった
};

class ItemEntity;
class PlayerType;
tr_type get_priest_blessing(PlayerType *player_ptr);
BlessWeaponOutcome apply_blessing_to_weapon(ItemEntity &item, tr_type blessing);
bool bless_weapon(PlayerType *player_ptr);
