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
    set_bits(tmp_effect_flag, (0x01U << FLAG_CAUSE_BATTLE_FORM));
    BIT_FLAGS race_class_flag = (0x01U << FLAG_CAUSE_CLASS);
    set_bits(race_class_flag, (0x01U << FLAG_CAUSE_RACE));

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

    if (any_bits(has_immune_acid(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_IM_ACID);
    if (any_bits(has_immune_elec(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_IM_ELEC);
    if (any_bits(has_immune_fire(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_IM_FIRE);
    if (any_bits(has_immune_cold(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_IM_COLD);
    
    if (any_bits(has_resist_lite(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_LITE);
    if (any_bits(has_resist_dark(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_DARK);
    if (any_bits(has_resist_blind(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_BLIND);
    if (any_bits(has_resist_conf(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_CONF);
    if (any_bits(has_resist_sound(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_SOUND);
    if (any_bits(has_resist_shard(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_SHARDS);
    if (any_bits(has_resist_neth(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_NETHER);
    if (any_bits(has_resist_nexus(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_NEXUS);
    if (any_bits(has_resist_chaos(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_CHAOS);
    if (any_bits(has_resist_disen(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_DISEN);

    if (any_bits(has_resist_fear(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_RES_FEAR);
    if (any_bits(has_reflect(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_REFLECT);
    if (any_bits(has_sh_fire(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SH_FIRE);
    if (any_bits(has_sh_cold(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SH_COLD);
    if (any_bits(has_sh_elec(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SH_ELEC);

    if (any_bits(has_free_act(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_FREE_ACT);
    if (any_bits(has_see_inv(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SEE_INVIS);
    if (any_bits(has_hold_exp(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_HOLD_EXP);
    
    if (any_bits(has_slow_digest(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SLOW_DIGEST);
    if (any_bits(has_regenerate(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_REGEN);
    if (any_bits(has_levitation(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_LEVITATION);
    if (any_bits(has_lite(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_LITE_1);

    if (any_bits(has_esp_telepathy(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_TELEPATHY);
    
    if (any_bits(has_sustain_str(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SUST_STR);
    if (any_bits(has_sustain_int(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SUST_INT);
    if (any_bits(has_sustain_wis(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SUST_WIS);
    if (any_bits(has_sustain_dex(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SUST_DEX);
    if (any_bits(has_sustain_con(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SUST_CON);
    if (any_bits(has_sustain_chr(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_SUST_CHR);

    if (any_bits(has_infra_vision(creature_ptr), tmp_effect_flag))
        add_flag(flags, TR_INFRA);
}
