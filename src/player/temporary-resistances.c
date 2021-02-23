#include "temporary-resistances.h"
#include "object-enchant/tr-types.h"
#include "player/attack-defense-types.h"
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
    BIT_FLAGS tmp_effect_flag = (0x01U << FLAG_CAUSE_MAGIC_TIME_EFFECT);
    set_bit(tmp_effect_flag, (0x01U << FLAG_CAUSE_BATTLE_FORM));
    BIT_FLAGS race_class_flag = (0x01U << FLAG_CAUSE_CLASS);
    set_bit(race_class_flag, (0x01U << FLAG_CAUSE_RACE));

    for (int i = 0; i < TR_FLAG_SIZE; i++)
        flags[i] = 0L;

    if (is_fast(creature_ptr) || creature_ptr->slow)
        add_flag(flags, TR_SPEED);

    if (creature_ptr->special_attack & ATTACK_ACID)
        add_flag(flags, TR_BRAND_ACID);
    if (creature_ptr->special_attack & ATTACK_ELEC)
        add_flag(flags, TR_BRAND_ELEC);
    if (creature_ptr->special_attack & ATTACK_FIRE)
        add_flag(flags, TR_BRAND_FIRE);
    if (creature_ptr->special_attack & ATTACK_COLD)
        add_flag(flags, TR_BRAND_COLD);
    if (creature_ptr->special_attack & ATTACK_POIS)
        add_flag(flags, TR_BRAND_POIS);

    if (is_oppose_acid(creature_ptr) && !test_bit(has_immune_acid(creature_ptr), (race_class_flag | tmp_effect_flag)))
        add_flag(flags, TR_RES_ACID);
    if (is_oppose_elec(creature_ptr) && !test_bit(has_immune_elec(creature_ptr), (race_class_flag | tmp_effect_flag)))
        add_flag(flags, TR_RES_ELEC);
    if (is_oppose_fire(creature_ptr) && !test_bit(has_immune_fire(creature_ptr), (race_class_flag | tmp_effect_flag)))
        add_flag(flags, TR_RES_FIRE);
    if (is_oppose_cold(creature_ptr) && !test_bit(has_immune_cold(creature_ptr), (race_class_flag | tmp_effect_flag)))
        add_flag(flags, TR_RES_COLD);
    if (is_oppose_pois(creature_ptr))
        add_flag(flags, TR_RES_POIS);

    if (test_bit(has_immune_acid(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_IM_ACID);
    if (test_bit(has_immune_elec(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_IM_ELEC);
    if (test_bit(has_immune_fire(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_IM_FIRE);
    if (test_bit(has_immune_cold(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_IM_COLD);
    
    if (test_bit(has_resist_lite(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_LITE);
    if (test_bit(has_resist_dark(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_DARK);
    if (test_bit(has_resist_blind(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_BLIND);
    if (test_bit(has_resist_conf(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_CONF);
    if (test_bit(has_resist_sound(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_SOUND);
    if (test_bit(has_resist_shard(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_SHARDS);
    if (test_bit(has_resist_neth(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_NETHER);
    if (test_bit(has_resist_nexus(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_NEXUS);
    if (test_bit(has_resist_chaos(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_CHAOS);
    if (test_bit(has_resist_disen(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_DISEN);

    if (test_bit(has_resist_fear(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_FEAR);
    if (test_bit(has_reflect(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_REFLECT);
    if (test_bit(has_sh_fire(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SH_FIRE);
    if (test_bit(has_sh_cold(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SH_COLD);
    if (test_bit(has_sh_elec(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SH_ELEC);

    if (test_bit(has_free_act(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_FREE_ACT);
    if (test_bit(has_see_inv(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SEE_INVIS);
    if (test_bit(has_hold_exp(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_HOLD_EXP);
    
    if (test_bit(has_slow_digest(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SLOW_DIGEST);
    if (test_bit(has_regenerate(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_REGEN);
    if (test_bit(has_levitation(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_LEVITATION);
    if (test_bit(has_lite(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_LITE_1);

    if (test_bit(has_esp_telepathy(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_TELEPATHY);
    
    if (test_bit(has_sustain_str(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SUST_STR);
    if (test_bit(has_sustain_int(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SUST_INT);
    if (test_bit(has_sustain_wis(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SUST_WIS);
    if (test_bit(has_sustain_dex(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SUST_DEX);
    if (test_bit(has_sustain_con(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SUST_CON);
    if (test_bit(has_sustain_chr(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SUST_CHR);

    if (test_bit(has_infra_vision(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_INFRA);
}
