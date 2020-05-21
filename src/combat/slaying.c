#include "system/angband.h"
#include "combat/slaying.h"

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

        if (!have_flag(flgs, p->slay_flag) || !(atoffset(BIT_FLAGS, r_ptr, p->flag_offset) & p->affect_race_flag))
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

        if (!have_flag(flgs, p->brand_flag))
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
