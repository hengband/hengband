﻿#include "combat/slaying.h"
#include "artifact/fixed-art-types.h"
#include "core/player-redraw-types.h"
#include "mind/mind-samurai.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "player/attack-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "specific-object/torch.h"
#include "spell-realm/spells-hex.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤー攻撃の種族スレイング倍率計算
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param mult 算出前の基本倍率(/10倍)
 * @param flgs スレイフラグ配列
 * @param m_ptr 目標モンスターの構造体参照ポインタ
 * @return スレイング加味後の倍率(/10倍)
 */
MULTIPLY mult_slaying(player_type *player_ptr, MULTIPLY mult, const BIT_FLAGS *flgs, monster_type *m_ptr)
{
    static const struct slay_table_t {
        int slay_flag;
        BIT_FLAGS affect_race_flag;
        MULTIPLY slay_mult;
        size_t flag_offset;
        size_t r_flag_offset;
    } slay_table[] = {
#define OFFSET(X) offsetof(monster_race, X)
        { TR_SLAY_ANIMAL, RF3_ANIMAL, 25, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_KILL_ANIMAL, RF3_ANIMAL, 40, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_SLAY_EVIL, RF3_EVIL, 20, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_KILL_EVIL, RF3_EVIL, 35, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_SLAY_GOOD, RF3_GOOD, 20, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_KILL_GOOD, RF3_GOOD, 35, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_SLAY_HUMAN, RF2_HUMAN, 25, OFFSET(flags2), OFFSET(r_flags2) },
        { TR_KILL_HUMAN, RF2_HUMAN, 40, OFFSET(flags2), OFFSET(r_flags2) },
        { TR_SLAY_UNDEAD, RF3_UNDEAD, 30, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_KILL_UNDEAD, RF3_UNDEAD, 50, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_SLAY_DEMON, RF3_DEMON, 30, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_KILL_DEMON, RF3_DEMON, 50, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_SLAY_ORC, RF3_ORC, 30, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_KILL_ORC, RF3_ORC, 50, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_SLAY_TROLL, RF3_TROLL, 30, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_KILL_TROLL, RF3_TROLL, 50, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_SLAY_GIANT, RF3_GIANT, 30, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_KILL_GIANT, RF3_GIANT, 50, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_SLAY_DRAGON, RF3_DRAGON, 30, OFFSET(flags3), OFFSET(r_flags3) },
        { TR_KILL_DRAGON, RF3_DRAGON, 50, OFFSET(flags3), OFFSET(r_flags3) },
#undef OFFSET
    };

    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    for (size_t i = 0; i < sizeof(slay_table) / sizeof(slay_table[0]); ++i) {
        const struct slay_table_t *p = &slay_table[i];

        if (!has_flag(flgs, p->slay_flag) || !(atoffset(BIT_FLAGS, r_ptr, p->flag_offset) & p->affect_race_flag))
            continue;

        if (is_original_ap_and_seen(player_ptr, m_ptr)) {
            atoffset(BIT_FLAGS, r_ptr, p->r_flag_offset) |= p->affect_race_flag;
        }

        mult = MAX(mult, p->slay_mult);
    }

    return mult;
}

/*!
 * @brief プレイヤー攻撃の属性スレイング倍率計算
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param mult 算出前の基本倍率(/10倍)
 * @param flgs スレイフラグ配列
 * @param m_ptr 目標モンスターの構造体参照ポインタ
 * @return スレイング加味後の倍率(/10倍)
 */
MULTIPLY mult_brand(player_type *player_ptr, MULTIPLY mult, const BIT_FLAGS *flgs, monster_type *m_ptr)
{
    static const struct brand_table_t {
        int brand_flag;
        BIT_FLAGS resist_mask;
        BIT_FLAGS hurt_flag;
    } brand_table[] = {
        { TR_BRAND_ACID, RFR_EFF_IM_ACID_MASK, 0U },
        { TR_BRAND_ELEC, RFR_EFF_IM_ELEC_MASK, 0U },
        { TR_BRAND_FIRE, RFR_EFF_IM_FIRE_MASK, RF3_HURT_FIRE },
        { TR_BRAND_COLD, RFR_EFF_IM_COLD_MASK, RF3_HURT_COLD },
        { TR_BRAND_POIS, RFR_EFF_IM_POIS_MASK, 0U },
    };

    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    for (size_t i = 0; i < sizeof(brand_table) / sizeof(brand_table[0]); ++i) {
        const struct brand_table_t *p = &brand_table[i];

        if (!has_flag(flgs, p->brand_flag))
            continue;

        /* Notice immunity */
        if (r_ptr->flagsr & p->resist_mask) {
            if (is_original_ap_and_seen(player_ptr, m_ptr)) {
                r_ptr->r_flagsr |= (r_ptr->flagsr & p->resist_mask);
            }

            continue;
        }

        /* Otherwise, take the damage */
        if (r_ptr->flags3 & p->hurt_flag) {
            if (is_original_ap_and_seen(player_ptr, m_ptr)) {
                r_ptr->r_flags3 |= p->hurt_flag;
            }

            mult = MAX(mult, 50);
            continue;
        }

        mult = MAX(mult, 25);
    }

    return mult;
}

/*!
 * @brief ダメージにスレイ要素を加える総合処理ルーチン /
 * Extract the "total damage" from a given object hitting a given monster.
 * @param o_ptr 使用武器オブジェクトの構造体参照ポインタ
 * @param tdam 現在算出途中のダメージ値
 * @param m_ptr 目標モンスターの構造体参照ポインタ
 * @param mode 剣術のID
 * @param thrown 投擲処理ならばTRUEを指定する
 * @return 総合的なスレイを加味したダメージ値
 * @note
 * Note that "flasks of oil" do NOT do fire damage, although they\n
 * certainly could be made to do so.  XXX XXX\n
 *\n
 * Note that most brands and slays are x3, except Slay Animal (x2),\n
 * Slay Evil (x2), and Kill dragon (x5).\n
 */
HIT_POINT calc_attack_damage_with_slay(player_type *attacker_ptr, object_type *o_ptr, HIT_POINT tdam, monster_type *m_ptr, combat_options mode, bool thrown)
{
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_flags(attacker_ptr, o_ptr, flgs);
    torch_flags(o_ptr, flgs); /* torches has secret flags */

    if (!thrown) {
        if (attacker_ptr->special_attack & (ATTACK_ACID))
            add_flag(flgs, TR_BRAND_ACID);
        if (attacker_ptr->special_attack & (ATTACK_COLD))
            add_flag(flgs, TR_BRAND_COLD);
        if (attacker_ptr->special_attack & (ATTACK_ELEC))
            add_flag(flgs, TR_BRAND_ELEC);
        if (attacker_ptr->special_attack & (ATTACK_FIRE))
            add_flag(flgs, TR_BRAND_FIRE);
        if (attacker_ptr->special_attack & (ATTACK_POIS))
            add_flag(flgs, TR_BRAND_POIS);
    }

    if (hex_spelling(attacker_ptr, HEX_RUNESWORD))
        add_flag(flgs, TR_SLAY_GOOD);

    MULTIPLY mult = 10;
    switch (o_ptr->tval) {
    case TV_SHOT:
    case TV_ARROW:
    case TV_BOLT:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_DIGGING:
    case TV_LITE: {
        mult = mult_slaying(attacker_ptr, mult, flgs, m_ptr);

        mult = mult_brand(attacker_ptr, mult, flgs, m_ptr);

        if (attacker_ptr->pclass == CLASS_SAMURAI) {
            mult = mult_hissatsu(attacker_ptr, mult, flgs, m_ptr, mode);
        }

        if ((attacker_ptr->pclass != CLASS_SAMURAI) && (has_flag(flgs, TR_FORCE_WEAPON)) && (attacker_ptr->csp > (o_ptr->dd * o_ptr->ds / 5))) {
            attacker_ptr->csp -= (1 + (o_ptr->dd * o_ptr->ds / 5));
            attacker_ptr->redraw |= (PR_MANA);
            mult = mult * 3 / 2 + 20;
        }

        if ((o_ptr->name1 == ART_NOTHUNG) && (m_ptr->r_idx == MON_FAFNER))
            mult = 150;
        break;
    }
    }

    if (mult > 150)
        mult = 150;
    return (tdam * mult / 10);
}
