#include "object/object-flags.h"
#include "object-kind.h"
#include "object-hook.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/object-ego.h"
#include "art-definition/art-sword-types.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include "util/quarks.h"
#include "perception/object-perception.h"

/*
 * The object kind arrays
 */
object_kind *k_info;
char *k_name;
char *k_text;

/*
 * Maximum number of items in k_info.txt
 */
KIND_OBJECT_IDX max_k_idx;

void calc_equipment_status(player_type* creature_ptr) {

	object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    int default_hand = 0;

	if (has_melee_weapon(creature_ptr, INVEN_LARM)) {
        if (!creature_ptr->migite)
            default_hand = 1;
    }

	for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        int bonus_to_h, bonus_to_d;
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(o_ptr, flgs);

        creature_ptr->cursed |= (o_ptr->curse_flags & (0xFFFFFFF0L));
        if (o_ptr->name1 == ART_CHAINSWORD)
            creature_ptr->cursed |= TRC_CHAINSWORD;

        if (have_flag(flgs, TR_INFRA))
            creature_ptr->see_infra += o_ptr->pval;
        if (have_flag(flgs, TR_BLOWS)) {
            if ((i == INVEN_RARM || i == INVEN_RIGHT) && !creature_ptr->ryoute)
                creature_ptr->extra_blows[0] += o_ptr->pval;
            else if ((i == INVEN_LARM || i == INVEN_LEFT) && !creature_ptr->ryoute)
                creature_ptr->extra_blows[1] += o_ptr->pval;
            else {
                creature_ptr->extra_blows[0] += o_ptr->pval;
                creature_ptr->extra_blows[1] += o_ptr->pval;
            }
        }

        if (have_flag(flgs, TR_IMPACT))
            creature_ptr->impact[(i == INVEN_RARM) ? 0 : 1] = TRUE;
        if (have_flag(flgs, TR_AGGRAVATE))
            creature_ptr->cursed |= TRC_AGGRAVATE;
        if (have_flag(flgs, TR_DRAIN_EXP))
            creature_ptr->cursed |= TRC_DRAIN_EXP;
        if (have_flag(flgs, TR_TY_CURSE))
            creature_ptr->cursed |= TRC_TY_CURSE;
        if (have_flag(flgs, TR_ADD_L_CURSE))
            creature_ptr->cursed |= TRC_ADD_L_CURSE;
        if (have_flag(flgs, TR_ADD_H_CURSE))
            creature_ptr->cursed |= TRC_ADD_H_CURSE;
        if (have_flag(flgs, TR_DRAIN_HP))
            creature_ptr->cursed |= TRC_DRAIN_HP;
        if (have_flag(flgs, TR_DRAIN_MANA))
            creature_ptr->cursed |= TRC_DRAIN_MANA;
        if (have_flag(flgs, TR_CALL_ANIMAL))
            creature_ptr->cursed |= TRC_CALL_ANIMAL;
        if (have_flag(flgs, TR_CALL_DEMON))
            creature_ptr->cursed |= TRC_CALL_DEMON;
        if (have_flag(flgs, TR_CALL_DRAGON))
            creature_ptr->cursed |= TRC_CALL_DRAGON;
        if (have_flag(flgs, TR_CALL_UNDEAD))
            creature_ptr->cursed |= TRC_CALL_UNDEAD;
        if (have_flag(flgs, TR_COWARDICE))
            creature_ptr->cursed |= TRC_COWARDICE;
        if (have_flag(flgs, TR_LOW_MELEE))
            creature_ptr->cursed |= TRC_LOW_MELEE;
        if (have_flag(flgs, TR_LOW_AC))
            creature_ptr->cursed |= TRC_LOW_AC;
        if (have_flag(flgs, TR_LOW_MAGIC))
            creature_ptr->cursed |= TRC_LOW_MAGIC;
        if (have_flag(flgs, TR_FAST_DIGEST))
            creature_ptr->cursed |= TRC_FAST_DIGEST;
        if (have_flag(flgs, TR_SLOW_REGEN))
            creature_ptr->cursed |= TRC_SLOW_REGEN;
        if (have_flag(flgs, TR_DEC_MANA))
            creature_ptr->dec_mana = TRUE;
        if (have_flag(flgs, TR_BLESSED))
            creature_ptr->bless_blade = TRUE;
        if (have_flag(flgs, TR_XTRA_MIGHT))
            creature_ptr->xtra_might = TRUE;
        if (have_flag(flgs, TR_SLOW_DIGEST))
            creature_ptr->slow_digest = TRUE;
        if (have_flag(flgs, TR_REGEN))
            creature_ptr->regenerate = TRUE;
        if (have_flag(flgs, TR_TELEPATHY))
            creature_ptr->telepathy = TRUE;
        if (have_flag(flgs, TR_ESP_ANIMAL))
            creature_ptr->esp_animal = TRUE;
        if (have_flag(flgs, TR_ESP_UNDEAD))
            creature_ptr->esp_undead = TRUE;
        if (have_flag(flgs, TR_ESP_DEMON))
            creature_ptr->esp_demon = TRUE;
        if (have_flag(flgs, TR_ESP_ORC))
            creature_ptr->esp_orc = TRUE;
        if (have_flag(flgs, TR_ESP_TROLL))
            creature_ptr->esp_troll = TRUE;
        if (have_flag(flgs, TR_ESP_GIANT))
            creature_ptr->esp_giant = TRUE;
        if (have_flag(flgs, TR_ESP_DRAGON))
            creature_ptr->esp_dragon = TRUE;
        if (have_flag(flgs, TR_ESP_HUMAN))
            creature_ptr->esp_human = TRUE;
        if (have_flag(flgs, TR_ESP_EVIL))
            creature_ptr->esp_evil = TRUE;
        if (have_flag(flgs, TR_ESP_GOOD))
            creature_ptr->esp_good = TRUE;
        if (have_flag(flgs, TR_ESP_NONLIVING))
            creature_ptr->esp_nonliving = TRUE;
        if (have_flag(flgs, TR_ESP_UNIQUE))
            creature_ptr->esp_unique = TRUE;

        if (have_flag(flgs, TR_SEE_INVIS))
            creature_ptr->see_inv = TRUE;
        if (have_flag(flgs, TR_LEVITATION))
            creature_ptr->levitation = TRUE;
        if (have_flag(flgs, TR_FREE_ACT))
            creature_ptr->free_act = TRUE;
        if (have_flag(flgs, TR_HOLD_EXP))
            creature_ptr->hold_exp = TRUE;
        if (have_flag(flgs, TR_WARNING)) {
            if (!o_ptr->inscription || !(angband_strchr(quark_str(o_ptr->inscription), '$')))
                creature_ptr->warning = TRUE;
        }

        if (have_flag(flgs, TR_TELEPORT)) {
            if (object_is_cursed(o_ptr))
                creature_ptr->cursed |= TRC_TELEPORT;
            else {
                concptr insc = quark_str(o_ptr->inscription);

                /* {.} will stop random teleportation. */
                if (o_ptr->inscription && angband_strchr(insc, '.')) {
                } else {
                    creature_ptr->cursed |= TRC_TELEPORT_SELF;
                }
            }
        }

        if (have_flag(flgs, TR_IM_FIRE))
            creature_ptr->immune_fire = TRUE;
        if (have_flag(flgs, TR_IM_ACID))
            creature_ptr->immune_acid = TRUE;
        if (have_flag(flgs, TR_IM_COLD))
            creature_ptr->immune_cold = TRUE;
        if (have_flag(flgs, TR_IM_ELEC))
            creature_ptr->immune_elec = TRUE;

        if (have_flag(flgs, TR_RES_ACID))
            creature_ptr->resist_acid = TRUE;
        if (have_flag(flgs, TR_RES_ELEC))
            creature_ptr->resist_elec = TRUE;
        if (have_flag(flgs, TR_RES_FIRE))
            creature_ptr->resist_fire = TRUE;
        if (have_flag(flgs, TR_RES_COLD))
            creature_ptr->resist_cold = TRUE;
        if (have_flag(flgs, TR_RES_POIS))
            creature_ptr->resist_pois = TRUE;
        if (have_flag(flgs, TR_RES_FEAR))
            creature_ptr->resist_fear = TRUE;
        if (have_flag(flgs, TR_RES_CONF))
            creature_ptr->resist_conf = TRUE;
        if (have_flag(flgs, TR_RES_SOUND))
            creature_ptr->resist_sound = TRUE;
        if (have_flag(flgs, TR_RES_LITE))
            creature_ptr->resist_lite = TRUE;
        if (have_flag(flgs, TR_RES_DARK))
            creature_ptr->resist_dark = TRUE;
        if (have_flag(flgs, TR_RES_CHAOS))
            creature_ptr->resist_chaos = TRUE;
        if (have_flag(flgs, TR_RES_DISEN))
            creature_ptr->resist_disen = TRUE;
        if (have_flag(flgs, TR_RES_SHARDS))
            creature_ptr->resist_shard = TRUE;
        if (have_flag(flgs, TR_RES_NEXUS))
            creature_ptr->resist_nexus = TRUE;
        if (have_flag(flgs, TR_RES_BLIND))
            creature_ptr->resist_blind = TRUE;
        if (have_flag(flgs, TR_RES_NETHER))
            creature_ptr->resist_neth = TRUE;

        if (have_flag(flgs, TR_REFLECT))
            creature_ptr->reflect = TRUE;
        if (have_flag(flgs, TR_SH_FIRE))
            creature_ptr->sh_fire = TRUE;
        if (have_flag(flgs, TR_SH_ELEC))
            creature_ptr->sh_elec = TRUE;
        if (have_flag(flgs, TR_SH_COLD))
            creature_ptr->sh_cold = TRUE;
        if (have_flag(flgs, TR_NO_MAGIC))
            creature_ptr->anti_magic = TRUE;
        if (have_flag(flgs, TR_NO_TELE))
            creature_ptr->anti_tele = TRUE;

        if (have_flag(flgs, TR_SUST_STR))
            creature_ptr->sustain_str = TRUE;
        if (have_flag(flgs, TR_SUST_INT))
            creature_ptr->sustain_int = TRUE;
        if (have_flag(flgs, TR_SUST_WIS))
            creature_ptr->sustain_wis = TRUE;
        if (have_flag(flgs, TR_SUST_DEX))
            creature_ptr->sustain_dex = TRUE;
        if (have_flag(flgs, TR_SUST_CON))
            creature_ptr->sustain_con = TRUE;
        if (have_flag(flgs, TR_SUST_CHR))
            creature_ptr->sustain_chr = TRUE;

        if (o_ptr->name2 == EGO_YOIYAMI)
            creature_ptr->yoiyami = TRUE;
        if (o_ptr->name2 == EGO_2WEAPON)
            creature_ptr->easy_2weapon = TRUE;
        if (o_ptr->name2 == EGO_RING_RES_TIME)
            creature_ptr->resist_time = TRUE;
        if (o_ptr->name2 == EGO_RING_THROW)
            creature_ptr->mighty_throw = TRUE;
        if (have_flag(flgs, TR_EASY_SPELL))
            creature_ptr->easy_spell = TRUE;
        if (o_ptr->name2 == EGO_AMU_FOOL)
            creature_ptr->heavy_spell = TRUE;
        if (o_ptr->name2 == EGO_AMU_NAIVETY)
            creature_ptr->down_saving = TRUE;

        if (o_ptr->tval == TV_CAPTURE)
            continue;

        if (i == INVEN_RARM && has_melee_weapon(creature_ptr, i))
            continue;
        if (i == INVEN_LARM && has_melee_weapon(creature_ptr, i))
            continue;
        if (i == INVEN_BOW)
            continue;

        bonus_to_h = o_ptr->to_h;
        bonus_to_d = o_ptr->to_d;

        if (creature_ptr->pclass == CLASS_NINJA) {
            if (o_ptr->to_h > 0)
                bonus_to_h = (o_ptr->to_h + 1) / 2;
            if (o_ptr->to_d > 0)
                bonus_to_d = (o_ptr->to_d + 1) / 2;
        }

        creature_ptr->to_h_b += (s16b)bonus_to_h;

        if (object_is_known(o_ptr))
            creature_ptr->dis_to_h_b += (s16b)bonus_to_h;

        if ((i == INVEN_LEFT || i == INVEN_RIGHT) && !creature_ptr->ryoute) {
            creature_ptr->to_h[i - INVEN_RIGHT] += (s16b)bonus_to_h;
            creature_ptr->to_d[i - INVEN_RIGHT] += (s16b)bonus_to_d;
            if (object_is_known(o_ptr)) {
                creature_ptr->dis_to_h[i - INVEN_RIGHT] += (s16b)bonus_to_h;
                creature_ptr->dis_to_d[i - INVEN_RIGHT] += (s16b)bonus_to_d;
            }

            continue;
        }

        if (creature_ptr->migite && creature_ptr->hidarite) {
            creature_ptr->to_h[0] += (bonus_to_h > 0) ? (bonus_to_h + 1) / 2 : bonus_to_h;
            creature_ptr->to_h[1] += (bonus_to_h > 0) ? bonus_to_h / 2 : bonus_to_h;
            creature_ptr->to_d[0] += (bonus_to_d > 0) ? (bonus_to_d + 1) / 2 : bonus_to_d;
            creature_ptr->to_d[1] += (bonus_to_d > 0) ? bonus_to_d / 2 : bonus_to_d;
            if (!object_is_known(o_ptr))
                continue;

            creature_ptr->dis_to_h[0] += (bonus_to_h > 0) ? (bonus_to_h + 1) / 2 : bonus_to_h;
            creature_ptr->dis_to_h[1] += (bonus_to_h > 0) ? bonus_to_h / 2 : bonus_to_h;
            creature_ptr->dis_to_d[0] += (bonus_to_d > 0) ? (bonus_to_d + 1) / 2 : bonus_to_d;
            creature_ptr->dis_to_d[1] += (bonus_to_d > 0) ? bonus_to_d / 2 : bonus_to_d;
            continue;
        }

        creature_ptr->to_h[default_hand] += (s16b)bonus_to_h;
        creature_ptr->to_d[default_hand] += (s16b)bonus_to_d;

        if (!object_is_known(o_ptr))
            continue;

        creature_ptr->dis_to_h[default_hand] += (s16b)bonus_to_h;
        creature_ptr->dis_to_d[default_hand] += (s16b)bonus_to_d;
    }
}
