#include "temporary-resistances.h"
#include "object-enchant/tr-types.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "status/element-resistance.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤーの一時的魔法効果による耐性を返す
 * Prints ratings on certain abilities
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param flags フラグを保管する配列
 * @todo
 * xtra1.c周りと多重実装になっているのを何とかする
 */
void tim_player_flags(PlayerType *player_ptr, TrFlags &flags)
{
    BIT_FLAGS tmp_effect_flag = FLAG_CAUSE_MAGIC_TIME_EFFECT;
    set_bits(tmp_effect_flag, FLAG_CAUSE_STANCE);
    BIT_FLAGS race_class_flag = FLAG_CAUSE_CLASS;
    set_bits(race_class_flag, FLAG_CAUSE_RACE);

    flags.clear();

    if (is_oppose_acid(player_ptr) && none_bits(has_immune_acid(player_ptr), (race_class_flag | tmp_effect_flag)))
        flags.set(TR_RES_ACID);
    if (is_oppose_elec(player_ptr) && none_bits(has_immune_elec(player_ptr), (race_class_flag | tmp_effect_flag)))
        flags.set(TR_RES_ELEC);
    if (is_oppose_fire(player_ptr) && none_bits(has_immune_fire(player_ptr), (race_class_flag | tmp_effect_flag)))
        flags.set(TR_RES_FIRE);
    if (is_oppose_cold(player_ptr) && none_bits(has_immune_cold(player_ptr), (race_class_flag | tmp_effect_flag)))
        flags.set(TR_RES_COLD);
    if (is_oppose_pois(player_ptr))
        flags.set(TR_RES_POIS);

    for (int test_flag = 0; test_flag < TR_FLAG_MAX; test_flag++) {
        if (any_bits(get_player_flags(player_ptr, i2enum<tr_type>(test_flag)), tmp_effect_flag))
            flags.set(i2enum<tr_type>(test_flag));
    }
}
