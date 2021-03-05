﻿#include "temporary-resistances.h"
#include "object-enchant/tr-types.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "status/element-resistance.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤーの一時的魔法効果による耐性を返す
 * Prints ratings on certain abilities
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 * @return なし
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
void tim_player_flags(player_type *creature_ptr, BIT_FLAGS *flags)
{
    BIT_FLAGS tmp_effect_flag = FLAG_CAUSE_MAGIC_TIME_EFFECT;
    set_bits(tmp_effect_flag, FLAG_CAUSE_BATTLE_FORM);
    BIT_FLAGS race_class_flag = FLAG_CAUSE_CLASS;
    set_bits(race_class_flag, FLAG_CAUSE_RACE);

    for (int i = 0; i < TR_FLAG_SIZE; i++)
        flags[i] = 0L;

    if (is_oppose_acid(creature_ptr) && none_bits(has_immune_acid(creature_ptr), (race_class_flag | tmp_effect_flag)))
        add_flag(flags, TR_RES_ACID);
    if (is_oppose_elec(creature_ptr) && none_bits(has_immune_elec(creature_ptr), (race_class_flag | tmp_effect_flag)))
        add_flag(flags, TR_RES_ELEC);
    if (is_oppose_fire(creature_ptr) && none_bits(has_immune_fire(creature_ptr), (race_class_flag | tmp_effect_flag)))
        add_flag(flags, TR_RES_FIRE);
    if (is_oppose_cold(creature_ptr) && none_bits(has_immune_cold(creature_ptr), (race_class_flag | tmp_effect_flag)))
        add_flag(flags, TR_RES_COLD);
    if (is_oppose_pois(creature_ptr))
        add_flag(flags, TR_RES_POIS);

    for (int test_flag = 0; test_flag < TR_FLAG_MAX; test_flag++) {
        if (any_bits(get_player_flags(creature_ptr, static_cast<tr_type>(test_flag)), tmp_effect_flag))
            add_flag(flags, test_flag);
    }
}
