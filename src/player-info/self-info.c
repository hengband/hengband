/*!
 * @brief 自己分析処理/ Self knowledge
 * @date 2018/09/07
 * @author deskull
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "player-info/self-info.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/object-flags.h"
#include "player-info/avatar.h"
#include "player-info/body-improvement-info.h"
#include "player-info/class-ability-info.h"
#include "player-info/mutation-info.h"
#include "player-info/race-ability-info.h"
#include "player-info/resistance-info.h"
#include "player-info/self-info-util.h"
#include "player/attack-defense-types.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "player/player-status-flags.h"
#include "realm/realm-names-table.h"
#include "realm/realm-song-numbers.h"
#include "status/element-resistance.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"

void display_life_rating(player_type *creature_ptr, self_info_type *si_ptr)
{
    creature_ptr->knowledge |= KNOW_STAT | KNOW_HPRATE;
    strcpy(si_ptr->plev_buf, "");
    int percent
        = (int)(((long)creature_ptr->player_hp[PY_MAX_LEVEL - 1] * 200L) / (2 * creature_ptr->hitdie + ((PY_MAX_LEVEL - 1 + 3) * (creature_ptr->hitdie + 1))));
    sprintf(si_ptr->plev_buf, _("現在の体力ランク : %d/100", "Your current Life Rating is %d/100."), percent);
    strcpy(si_ptr->buf[0], si_ptr->plev_buf);
    si_ptr->info[si_ptr->line++] = si_ptr->buf[0];
    si_ptr->info[si_ptr->line++] = "";
}

void display_max_base_status(player_type *creature_ptr, self_info_type *si_ptr)
{
    si_ptr->info[si_ptr->line++] = _("能力の最大値", "Limits of maximum stats");
    for (base_status_type v_nr = 0; v_nr < A_MAX; v_nr++) {
        char stat_desc[80];
        sprintf(stat_desc, "%s 18/%d", stat_names[v_nr], creature_ptr->stat_max_max[v_nr] - 18);
        strcpy(si_ptr->s_string[v_nr], stat_desc);
        si_ptr->info[si_ptr->line++] = si_ptr->s_string[v_nr];
    }
}

void display_virtue(player_type *creature_ptr, self_info_type *si_ptr)
{
    si_ptr->info[si_ptr->line++] = "";
    sprintf(si_ptr->plev_buf, _("現在の属性 : %s(%ld)", "Your alignment : %s(%ld)"), your_alignment(creature_ptr), (long int)creature_ptr->align);
    strcpy(si_ptr->buf[1], si_ptr->plev_buf);
    si_ptr->info[si_ptr->line++] = si_ptr->buf[1];
    for (int v_nr = 0; v_nr < 8; v_nr++) {
        GAME_TEXT vir_name[20];
        char vir_desc[80];
        int tester = creature_ptr->virtues[v_nr];
        strcpy(vir_name, virtue[(creature_ptr->vir_types[v_nr]) - 1]);
        sprintf(vir_desc, _("おっと。%sの情報なし。", "Oops. No info about %s."), vir_name);
        if (tester < -100)
            sprintf(vir_desc, _("[%s]の対極 (%d)", "You are the polar opposite of %s (%d)."), vir_name, tester);
        else if (tester < -80)
            sprintf(vir_desc, _("[%s]の大敵 (%d)", "You are an arch-enemy of %s (%d)."), vir_name, tester);
        else if (tester < -60)
            sprintf(vir_desc, _("[%s]の強敵 (%d)", "You are a bitter enemy of %s (%d)."), vir_name, tester);
        else if (tester < -40)
            sprintf(vir_desc, _("[%s]の敵 (%d)", "You are an enemy of %s (%d)."), vir_name, tester);
        else if (tester < -20)
            sprintf(vir_desc, _("[%s]の罪者 (%d)", "You have sinned against %s (%d)."), vir_name, tester);
        else if (tester < 0)
            sprintf(vir_desc, _("[%s]の迷道者 (%d)", "You have strayed from the path of %s (%d)."), vir_name, tester);
        else if (tester == 0)
            sprintf(vir_desc, _("[%s]の中立者 (%d)", "You are neutral to %s (%d)."), vir_name, tester);
        else if (tester < 20)
            sprintf(vir_desc, _("[%s]の小徳者 (%d)", "You are somewhat virtuous in %s (%d)."), vir_name, tester);
        else if (tester < 40)
            sprintf(vir_desc, _("[%s]の中徳者 (%d)", "You are virtuous in %s (%d)."), vir_name, tester);
        else if (tester < 60)
            sprintf(vir_desc, _("[%s]の高徳者 (%d)", "You are very virtuous in %s (%d)."), vir_name, tester);
        else if (tester < 80)
            sprintf(vir_desc, _("[%s]の覇者 (%d)", "You are a champion of %s (%d)."), vir_name, tester);
        else if (tester < 100)
            sprintf(vir_desc, _("[%s]の偉大な覇者 (%d)", "You are a great champion of %s (%d)."), vir_name, tester);
        else
            sprintf(vir_desc, _("[%s]の具現者 (%d)", "You are the living embodiment of %s (%d)."), vir_name, tester);

        strcpy(si_ptr->v_string[v_nr], vir_desc);
        si_ptr->info[si_ptr->line++] = si_ptr->v_string[v_nr];
    }
}

void display_mimic_race_ability(player_type *creature_ptr, self_info_type *si_ptr)
{
    switch (creature_ptr->mimic_form) {
    case MIMIC_DEMON:
    case MIMIC_DEMON_LORD:
        sprintf(si_ptr->plev_buf, _("あなたは %d ダメージの地獄か火炎のブレスを吐くことができる。(%d MP)", "You can nether breathe, dam. %d (cost %d)."),
            3 * creature_ptr->lev, 10 + creature_ptr->lev / 3);

        si_ptr->info[si_ptr->line++] = si_ptr->plev_buf;
        break;
    case MIMIC_VAMPIRE:
        if (creature_ptr->lev <= 1)
            break;

        sprintf(si_ptr->plev_buf, _("あなたは敵から %d-%d HP の生命力を吸収できる。(%d MP)", "You can steal life from a foe, dam. %d-%d (cost %d)."),
            creature_ptr->lev + MAX(1, creature_ptr->lev / 10), creature_ptr->lev + creature_ptr->lev * MAX(1, creature_ptr->lev / 10),
            1 + (creature_ptr->lev / 3));
        si_ptr->info[si_ptr->line++] = si_ptr->plev_buf;
        break;
    }
}

void display_equipment_influence(player_type *creature_ptr, self_info_type *si_ptr)
{
    for (int k = INVEN_RARM; k < INVEN_TOTAL; k++) {
        u32b tflgs[TR_FLAG_SIZE];
        si_ptr->o_ptr = &creature_ptr->inventory_list[k];
        if (si_ptr->o_ptr->k_idx == 0)
            continue;

        object_flags(creature_ptr, si_ptr->o_ptr, tflgs);
        for (int j = 0; j < TR_FLAG_SIZE; j++)
            si_ptr->flags[j] |= tflgs[j];
    }

    if (have_flag(si_ptr->flags, TR_STR))
        si_ptr->info[si_ptr->line++] = _("あなたの腕力は装備によって影響を受けている。", "Your strength is affected by your equipment.");

    if (have_flag(si_ptr->flags, TR_INT))
        si_ptr->info[si_ptr->line++] = _("あなたの知能は装備によって影響を受けている。", "Your intelligence is affected by your equipment.");

    if (have_flag(si_ptr->flags, TR_WIS))
        si_ptr->info[si_ptr->line++] = _("あなたの賢さは装備によって影響を受けている。", "Your wisdom is affected by your equipment.");

    if (have_flag(si_ptr->flags, TR_DEX))
        si_ptr->info[si_ptr->line++] = _("あなたの器用さは装備によって影響を受けている。", "Your dexterity is affected by your equipment.");

    if (have_flag(si_ptr->flags, TR_CON))
        si_ptr->info[si_ptr->line++] = _("あなたの耐久力は装備によって影響を受けている。", "Your constitution is affected by your equipment.");

    if (have_flag(si_ptr->flags, TR_CHR))
        si_ptr->info[si_ptr->line++] = _("あなたの魅力は装備によって影響を受けている。", "Your charisma is affected by your equipment.");

    if (have_flag(si_ptr->flags, TR_STEALTH))
        si_ptr->info[si_ptr->line++] = _("あなたの隠密行動能力は装備によって影響を受けている。", "Your stealth is affected by your equipment.");

    if (have_flag(si_ptr->flags, TR_SEARCH))
        si_ptr->info[si_ptr->line++] = _("あなたの探索能力は装備によって影響を受けている。", "Your searching ability is affected by your equipment.");

    if (have_flag(si_ptr->flags, TR_INFRA))
        si_ptr->info[si_ptr->line++] = _("あなたの赤外線視力は装備によって影響を受けている。", "Your infravision is affected by your equipment.");

    if (have_flag(si_ptr->flags, TR_TUNNEL))
        si_ptr->info[si_ptr->line++] = _("あなたの採掘能力は装備によって影響を受けている。", "Your digging ability is affected by your equipment.");

    if (have_flag(si_ptr->flags, TR_SPEED))
        si_ptr->info[si_ptr->line++] = _("あなたのスピードは装備によって影響を受けている。", "Your speed is affected by your equipment.");

    if (have_flag(si_ptr->flags, TR_BLOWS))
        si_ptr->info[si_ptr->line++] = _("あなたの攻撃速度は装備によって影響を受けている。", "Your attack speed is affected by your equipment.");
}

void set_bad_status_info(player_type *creature_ptr, self_info_type *si_ptr)
{
    if (creature_ptr->blind)
        si_ptr->info[si_ptr->line++] = _("あなたは目が見えない。", "You cannot see.");

    if (creature_ptr->confused)
        si_ptr->info[si_ptr->line++] = _("あなたは混乱している。", "You are confused.");

    if (creature_ptr->afraid)
        si_ptr->info[si_ptr->line++] = _("あなたは恐怖に侵されている。", "You are terrified.");

    if (creature_ptr->cut)
        si_ptr->info[si_ptr->line++] = _("あなたは出血している。", "You are bleeding.");

    if (creature_ptr->stun)
        si_ptr->info[si_ptr->line++] = _("あなたはもうろうとしている。", "You are stunned.");

    if (creature_ptr->poisoned)
        si_ptr->info[si_ptr->line++] = _("あなたは毒に侵されている。", "You are poisoned.");

    if (creature_ptr->image)
        si_ptr->info[si_ptr->line++] = _("あなたは幻覚を見ている。", "You are hallucinating.");
}

void set_curse_info(player_type *creature_ptr, self_info_type *si_ptr)
{
    if (creature_ptr->cursed & TRC_TY_CURSE)
        si_ptr->info[si_ptr->line++] = _("あなたは邪悪な怨念に包まれている。", "You carry an ancient foul curse.");

    if (creature_ptr->cursed & TRC_AGGRAVATE)
        si_ptr->info[si_ptr->line++] = _("あなたはモンスターを怒らせている。", "You aggravate monsters.");

    if (creature_ptr->cursed & TRC_DRAIN_EXP)
        si_ptr->info[si_ptr->line++] = _("あなたは経験値を吸われている。", "You occasionally lose experience for no reason.");

    if (creature_ptr->cursed & TRC_SLOW_REGEN)
        si_ptr->info[si_ptr->line++] = _("あなたの回復力は非常に遅い。", "You regenerate slowly.");

    if (creature_ptr->cursed & TRC_ADD_L_CURSE)
        si_ptr->info[si_ptr->line++] = _("あなたの弱い呪いは増える。", "Your weak curses multiply."); /* 暫定的 -- henkma */

    if (creature_ptr->cursed & TRC_ADD_H_CURSE)
        si_ptr->info[si_ptr->line++] = _("あなたの強い呪いは増える。", "Your heavy curses multiply."); /* 暫定的 -- henkma */

    if (creature_ptr->cursed & TRC_CALL_ANIMAL)
        si_ptr->info[si_ptr->line++] = _("あなたは動物に狙われている。", "You attract animals.");

    if (creature_ptr->cursed & TRC_CALL_DEMON)
        si_ptr->info[si_ptr->line++] = _("あなたは悪魔に狙われている。", "You attract demons.");

    if (creature_ptr->cursed & TRC_CALL_DRAGON)
        si_ptr->info[si_ptr->line++] = _("あなたはドラゴンに狙われている。", "You attract dragons.");

    if (creature_ptr->cursed & TRC_COWARDICE)
        si_ptr->info[si_ptr->line++] = _("あなたは時々臆病になる。", "You are subject to cowardice.");

    if (creature_ptr->cursed & TRC_TELEPORT)
        si_ptr->info[si_ptr->line++] = _("あなたの位置はひじょうに不安定だ。", "Your position is very uncertain.");

    if (creature_ptr->cursed & TRC_LOW_MELEE)
        si_ptr->info[si_ptr->line++] = _("あなたの武器は攻撃を外しやすい。", "Your weapon causes you to miss blows.");

    if (creature_ptr->cursed & TRC_LOW_AC)
        si_ptr->info[si_ptr->line++] = _("あなたは攻撃を受けやすい。", "You are subject to be hit.");

    if (creature_ptr->cursed & TRC_LOW_MAGIC)
        si_ptr->info[si_ptr->line++] = _("あなたは魔法を失敗しやすい。", "Your spells fail more frequently.");

    if (creature_ptr->cursed & TRC_FAST_DIGEST)
        si_ptr->info[si_ptr->line++] = _("あなたはすぐお腹がへる。", "You have a good appetite.");

    if (creature_ptr->cursed & TRC_DRAIN_HP)
        si_ptr->info[si_ptr->line++] = _("あなたは体力を吸われている。", "You occasionally lose hit points for no reason.");

    if (creature_ptr->cursed & TRC_DRAIN_MANA)
        si_ptr->info[si_ptr->line++] = _("あなたは魔力を吸われている。", "You occasionally lose spell points for no reason.");
}

void set_special_attack_info(player_type *creature_ptr, self_info_type *si_ptr)
{
    if (creature_ptr->special_attack & ATTACK_CONFUSE)
        si_ptr->info[si_ptr->line++] = _("あなたの手は赤く輝いている。", "Your hands are glowing dull red.");

    if (creature_ptr->special_attack & ATTACK_FIRE)
        si_ptr->info[si_ptr->line++] = _("あなたの手は火炎に覆われている。", "You can strike the enemy with flame.");

    if (creature_ptr->special_attack & ATTACK_COLD)
        si_ptr->info[si_ptr->line++] = _("あなたの手は冷気に覆われている。", "You can strike the enemy with cold.");

    if (creature_ptr->special_attack & ATTACK_ACID)
        si_ptr->info[si_ptr->line++] = _("あなたの手は酸に覆われている。", "You can strike the enemy with acid.");

    if (creature_ptr->special_attack & ATTACK_ELEC)
        si_ptr->info[si_ptr->line++] = _("あなたの手は電撃に覆われている。", "You can strike the enemy with electoric shock.");

    if (creature_ptr->special_attack & ATTACK_POIS)
        si_ptr->info[si_ptr->line++] = _("あなたの手は毒に覆われている。", "You can strike the enemy with poison.");
}

void set_esp_info(player_type *creature_ptr, self_info_type *si_ptr)
{
    if (creature_ptr->telepathy)
        si_ptr->info[si_ptr->line++] = _("あなたはテレパシー能力を持っている。", "You have ESP.");

    if (creature_ptr->esp_animal)
        si_ptr->info[si_ptr->line++] = _("あなたは自然界の生物の存在を感じる能力を持っている。", "You sense natural creatures.");

    if (creature_ptr->esp_undead)
        si_ptr->info[si_ptr->line++] = _("あなたはアンデッドの存在を感じる能力を持っている。", "You sense undead.");

    if (creature_ptr->esp_demon)
        si_ptr->info[si_ptr->line++] = _("あなたは悪魔の存在を感じる能力を持っている。", "You sense demons.");

    if (creature_ptr->esp_orc)
        si_ptr->info[si_ptr->line++] = _("あなたはオークの存在を感じる能力を持っている。", "You sense orcs.");

    if (creature_ptr->esp_troll)
        si_ptr->info[si_ptr->line++] = _("あなたはトロルの存在を感じる能力を持っている。", "You sense trolls.");

    if (creature_ptr->esp_giant)
        si_ptr->info[si_ptr->line++] = _("あなたは巨人の存在を感じる能力を持っている。", "You sense giants.");

    if (creature_ptr->esp_dragon)
        si_ptr->info[si_ptr->line++] = _("あなたはドラゴンの存在を感じる能力を持っている。", "You sense dragons.");

    if (creature_ptr->esp_human)
        si_ptr->info[si_ptr->line++] = _("あなたは人間の存在を感じる能力を持っている。", "You sense humans.");

    if (creature_ptr->esp_evil)
        si_ptr->info[si_ptr->line++] = _("あなたは邪悪な生き物の存在を感じる能力を持っている。", "You sense evil creatures.");

    if (creature_ptr->esp_good)
        si_ptr->info[si_ptr->line++] = _("あなたは善良な生き物の存在を感じる能力を持っている。", "You sense good creatures.");

    if (creature_ptr->esp_nonliving)
        si_ptr->info[si_ptr->line++] = _("あなたは活動する無生物体の存在を感じる能力を持っている。", "You sense non-living creatures.");

    if (creature_ptr->esp_unique)
        si_ptr->info[si_ptr->line++] = _("あなたは特別な強敵の存在を感じる能力を持っている。", "You sense unique monsters.");
}

/*!
 * @brief 自己分析処理(Nethackからのアイデア) / self-knowledge... idea from nethack.
 * @return なし
 * @details
 * <pre>
 * Useful for determining powers and
 * resistences of items.  It saves the screen, clears it, then starts listing
 * attributes, a screenful at a time.  (There are a LOT of attributes to
 * list.  It will probably take 2 or 3 screens for a powerful character whose
 * using several artifacts...) -CFT
 *
 * It is now a lot more efficient. -BEN-
 *
 * See also "identify_fully()".
 *
 * Use the "show_file()" method, perhaps.
 * </pre>
 */
void self_knowledge(player_type *creature_ptr)
{
    self_info_type tmp_si;
    self_info_type *si_ptr = initialize_self_info_type(&tmp_si);
    display_life_rating(creature_ptr, si_ptr);
    chg_virtue(creature_ptr, V_KNOWLEDGE, 1);
    chg_virtue(creature_ptr, V_ENLIGHTEN, 1);
    display_max_base_status(creature_ptr, si_ptr);
    display_virtue(creature_ptr, si_ptr);
    si_ptr->info[si_ptr->line++] = "";
    if (creature_ptr->mimic_form)
        display_mimic_race_ability(creature_ptr, si_ptr);
    else
        set_race_ability_info(creature_ptr, si_ptr);

    set_class_ability_info(creature_ptr, si_ptr);
    set_mutation_info_1(creature_ptr, si_ptr);
    set_mutation_info_2(creature_ptr, si_ptr);
    set_mutation_info_3(creature_ptr, si_ptr);
    set_bad_status_info(creature_ptr, si_ptr);
    set_curse_info(creature_ptr, si_ptr);
    set_body_improvement_info_1(creature_ptr, si_ptr);
    set_special_attack_info(creature_ptr, si_ptr);
    switch (creature_ptr->action) {
    case ACTION_SEARCH:
        si_ptr->info[si_ptr->line++] = _("あなたはひじょうに注意深く周囲を見渡している。", "You are looking around very carefully.");
        break;
    }

    set_body_improvement_info_2(creature_ptr, si_ptr);
    set_esp_info(creature_ptr, si_ptr);
    set_body_improvement_info_3(creature_ptr, si_ptr);
    set_element_resistance_info(creature_ptr, si_ptr);
    set_high_resistance_info(creature_ptr, si_ptr);
    if (creature_ptr->resist_fear) {
        si_ptr->info[si_ptr->line++] = _("あなたは全く恐怖を感じない。", "You are completely fearless.");
    }
    if (creature_ptr->resist_blind) {
        si_ptr->info[si_ptr->line++] = _("あなたの目は盲目への耐性を持っている。", "Your eyes are resistant to blindness.");
    }
    if (creature_ptr->resist_time) {
        si_ptr->info[si_ptr->line++] = _("あなたは時間逆転への耐性を持っている。", "You are resistant to time.");
    }

    if (creature_ptr->sustain_str) {
        si_ptr->info[si_ptr->line++] = _("あなたの腕力は維持されている。", "Your strength is sustained.");
    }
    if (creature_ptr->sustain_int) {
        si_ptr->info[si_ptr->line++] = _("あなたの知能は維持されている。", "Your intelligence is sustained.");
    }
    if (creature_ptr->sustain_wis) {
        si_ptr->info[si_ptr->line++] = _("あなたの賢さは維持されている。", "Your wisdom is sustained.");
    }
    if (creature_ptr->sustain_con) {
        si_ptr->info[si_ptr->line++] = _("あなたの耐久力は維持されている。", "Your constitution is sustained.");
    }
    if (creature_ptr->sustain_dex) {
        si_ptr->info[si_ptr->line++] = _("あなたの器用さは維持されている。", "Your dexterity is sustained.");
    }
    if (creature_ptr->sustain_chr) {
        si_ptr->info[si_ptr->line++] = _("あなたの魅力は維持されている。", "Your charisma is sustained.");
    }

    display_equipment_influence(creature_ptr, si_ptr);

    /* Access the current weapon */
    si_ptr->o_ptr = &creature_ptr->inventory_list[INVEN_RARM];

    /* Analyze the weapon */
    if (si_ptr->o_ptr->k_idx) {
        /* Indicate Blessing */
        if (have_flag(si_ptr->flags, TR_BLESSED)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は神の祝福を受けている。", "Your weapon has been blessed by the gods.");
        }

        if (have_flag(si_ptr->flags, TR_CHAOTIC)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器はログルスの徴の属性をもつ。", "Your weapon is branded with the Sign of Logrus.");
        }

        if (have_flag(si_ptr->flags, TR_IMPACT)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は打撃で地震を発生することができる。", "The impact of your weapon can cause earthquakes.");
        }

        if (have_flag(si_ptr->flags, TR_VORPAL)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は非常に鋭い。", "Your weapon is very sharp.");
        }

        if (have_flag(si_ptr->flags, TR_VAMPIRIC)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は敵から生命力を吸収する。", "Your weapon drains life from your foes.");
        }

        /* Special "Attack Bonuses" */
        if (have_flag(si_ptr->flags, TR_BRAND_ACID)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は敵を溶かす。", "Your weapon melts your foes.");
        }
        if (have_flag(si_ptr->flags, TR_BRAND_ELEC)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は敵を感電させる。", "Your weapon shocks your foes.");
        }
        if (have_flag(si_ptr->flags, TR_BRAND_FIRE)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は敵を燃やす。", "Your weapon burns your foes.");
        }
        if (have_flag(si_ptr->flags, TR_BRAND_COLD)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は敵を凍らせる。", "Your weapon freezes your foes.");
        }
        if (have_flag(si_ptr->flags, TR_BRAND_POIS)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は敵を毒で侵す。", "Your weapon poisons your foes.");
        }

        /* Special "slay" si_ptr->flags */
        if (have_flag(si_ptr->flags, TR_KILL_ANIMAL)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は動物の天敵である。", "Your weapon is a great bane of animals.");
        } else if (have_flag(si_ptr->flags, TR_SLAY_ANIMAL)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は動物に対して強い力を発揮する。", "Your weapon strikes at animals with extra force.");
        }
        if (have_flag(si_ptr->flags, TR_KILL_EVIL)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は邪悪なる存在の天敵である。", "Your weapon is a great bane of evil.");
        } else if (have_flag(si_ptr->flags, TR_SLAY_EVIL)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は邪悪なる存在に対して強い力を発揮する。", "Your weapon strikes at evil with extra force.");
        }
        if (have_flag(si_ptr->flags, TR_KILL_HUMAN)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は人間の天敵である。", "Your weapon is a great bane of humans.");
        } else if (have_flag(si_ptr->flags, TR_SLAY_HUMAN)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は人間に対して特に強い力を発揮する。", "Your weapon is especially deadly against humans.");
        }
        if (have_flag(si_ptr->flags, TR_KILL_UNDEAD)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器はアンデッドの天敵である。", "Your weapon is a great bane of undead.");
        } else if (have_flag(si_ptr->flags, TR_SLAY_UNDEAD)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器はアンデッドに対して神聖なる力を発揮する。", "Your weapon strikes at undead with holy wrath.");
        }
        if (have_flag(si_ptr->flags, TR_KILL_DEMON)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器はデーモンの天敵である。", "Your weapon is a great bane of demons.");
        } else if (have_flag(si_ptr->flags, TR_SLAY_DEMON)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器はデーモンに対して神聖なる力を発揮する。", "Your weapon strikes at demons with holy wrath.");
        }
        if (have_flag(si_ptr->flags, TR_KILL_ORC)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器はオークの天敵である。", "Your weapon is a great bane of orcs.");
        } else if (have_flag(si_ptr->flags, TR_SLAY_ORC)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器はオークに対して特に強い力を発揮する。", "Your weapon is especially deadly against orcs.");
        }
        if (have_flag(si_ptr->flags, TR_KILL_TROLL)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器はトロルの天敵である。", "Your weapon is a great bane of trolls.");
        } else if (have_flag(si_ptr->flags, TR_SLAY_TROLL)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器はトロルに対して特に強い力を発揮する。", "Your weapon is especially deadly against trolls.");
        }
        if (have_flag(si_ptr->flags, TR_KILL_GIANT)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は巨人の天敵である。", "Your weapon is a great bane of giants.");
        } else if (have_flag(si_ptr->flags, TR_SLAY_GIANT)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器は巨人に対して特に強い力を発揮する。", "Your weapon is especially deadly against giants.");
        }
        /* Special "kill" si_ptr->flags */
        if (have_flag(si_ptr->flags, TR_KILL_DRAGON)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器はドラゴンの天敵である。", "Your weapon is a great bane of dragons.");
        } else if (have_flag(si_ptr->flags, TR_SLAY_DRAGON)) {
            si_ptr->info[si_ptr->line++] = _("あなたの武器はドラゴンに対して特に強い力を発揮する。", "Your weapon is especially deadly against dragons.");
        }

        if (have_flag(si_ptr->flags, TR_FORCE_WEAPON))
            si_ptr->info[si_ptr->line++] = _("あなたの武器はMPを使って攻撃する。", "Your weapon causes greate damages using your MP.");

        if (have_flag(si_ptr->flags, TR_THROW))
            si_ptr->info[si_ptr->line++] = _("あなたの武器は投げやすい。", "Your weapon can be thrown well.");
    }

    screen_save();
    for (int k = 1; k < 24; k++)
        prt("", k, 13);

    prt(_("        あなたの状態:", "     Your Attributes:"), 1, 15);
    int k = 2;
    for (int j = 0; j < si_ptr->line; j++) {
        prt(si_ptr->info[j], k++, 15);

        /* Every 20 entries (lines 2 to 21), start over */
        if ((k != 22) || (j + 1 >= si_ptr->line))
            continue;

        prt(_("-- 続く --", "-- more --"), k, 15);
        inkey();
        for (; k > 2; k--)
            prt("", k, 15);
    }

    prt(_("[何かキーを押すとゲームに戻ります]", "[Press any key to continue]"), k, 13);
    inkey();
    screen_load();
}

/*!
 * @brief 魔法効果時間のターン数に基づいて表現IDを返す。
 * @param dur 効果ターン数
 * @return 効果時間の表現ID
 */
static int report_magics_aux(int dur)
{
    if (dur <= 5) {
        return 0;
    } else if (dur <= 10) {
        return 1;
    } else if (dur <= 20) {
        return 2;
    } else if (dur <= 50) {
        return 3;
    } else if (dur <= 100) {
        return 4;
    } else if (dur <= 200) {
        return 5;
    } else {
        return 6;
    }
}

static concptr report_magic_durations[] = {
#ifdef JP
    "ごく短い間", "少しの間", "しばらくの間", "多少長い間", "長い間", "非常に長い間", "信じ難いほど長い間", "モンスターを攻撃するまで"
#else
    "for a short time", "for a little while", "for a while", "for a long while", "for a long time", "for a very long time", "for an incredibly long time",
    "until you hit a monster"
#endif

};

/*!
 * @brief 現在の一時的効果一覧を返す / Report all currently active magical effects.
 * @return なし
 */
void report_magics(player_type *creature_ptr)
{
    int i = 0, j, k;
    char Dummy[80];
    concptr info[128];
    int info2[128];

    if (creature_ptr->blind) {
        info2[i] = report_magics_aux(creature_ptr->blind);
        info[i++] = _("あなたは目が見えない", "You cannot see");
    }
    if (creature_ptr->confused) {
        info2[i] = report_magics_aux(creature_ptr->confused);
        info[i++] = _("あなたは混乱している", "You are confused");
    }
    if (creature_ptr->afraid) {
        info2[i] = report_magics_aux(creature_ptr->afraid);
        info[i++] = _("あなたは恐怖に侵されている", "You are terrified");
    }
    if (creature_ptr->poisoned) {
        info2[i] = report_magics_aux(creature_ptr->poisoned);
        info[i++] = _("あなたは毒に侵されている", "You are poisoned");
    }
    if (creature_ptr->image) {
        info2[i] = report_magics_aux(creature_ptr->image);
        info[i++] = _("あなたは幻覚を見ている", "You are hallucinating");
    }
    if (creature_ptr->blessed) {
        info2[i] = report_magics_aux(creature_ptr->blessed);
        info[i++] = _("あなたは高潔さを感じている", "You feel rightous");
    }
    if (creature_ptr->hero) {
        info2[i] = report_magics_aux(creature_ptr->hero);
        info[i++] = _("あなたはヒーロー気分だ", "You feel heroic");
    }
    if (creature_ptr->shero) {
        info2[i] = report_magics_aux(creature_ptr->shero);
        info[i++] = _("あなたは戦闘狂だ", "You are in a battle rage");
    }
    if (creature_ptr->protevil) {
        info2[i] = report_magics_aux(creature_ptr->protevil);
        info[i++] = _("あなたは邪悪なる存在から守られている", "You are protected from evil");
    }
    if (creature_ptr->shield) {
        info2[i] = report_magics_aux(creature_ptr->shield);
        info[i++] = _("あなたは神秘のシールドで守られている", "You are protected by a mystic shield");
    }
    if (creature_ptr->invuln) {
        info2[i] = report_magics_aux(creature_ptr->invuln);
        info[i++] = _("あなたは無敵だ", "You are invulnerable");
    }
    if (creature_ptr->wraith_form) {
        info2[i] = report_magics_aux(creature_ptr->wraith_form);
        info[i++] = _("あなたは幽体化している", "You are incorporeal");
    }
    if (creature_ptr->special_attack & ATTACK_CONFUSE) {
        info2[i] = 7;
        info[i++] = _("あなたの手は赤く輝いている", "Your hands are glowing dull red.");
    }
    if (creature_ptr->word_recall) {
        info2[i] = report_magics_aux(creature_ptr->word_recall);
        info[i++] = _("この後帰還の詔が発動する", "You are waiting to be recalled");
    }
    if (creature_ptr->alter_reality) {
        info2[i] = report_magics_aux(creature_ptr->alter_reality);
        info[i++] = _("この後現実変容が発動する", "You waiting to be altered");
    }
    if (creature_ptr->oppose_acid) {
        info2[i] = report_magics_aux(creature_ptr->oppose_acid);
        info[i++] = _("あなたは酸への耐性を持っている", "You are resistant to acid");
    }
    if (creature_ptr->oppose_elec) {
        info2[i] = report_magics_aux(creature_ptr->oppose_elec);
        info[i++] = _("あなたは電撃への耐性を持っている", "You are resistant to lightning");
    }
    if (creature_ptr->oppose_fire) {
        info2[i] = report_magics_aux(creature_ptr->oppose_fire);
        info[i++] = _("あなたは火への耐性を持っている", "You are resistant to fire");
    }
    if (creature_ptr->oppose_cold) {
        info2[i] = report_magics_aux(creature_ptr->oppose_cold);
        info[i++] = _("あなたは冷気への耐性を持っている", "You are resistant to cold");
    }
    if (creature_ptr->oppose_pois) {
        info2[i] = report_magics_aux(creature_ptr->oppose_pois);
        info[i++] = _("あなたは毒への耐性を持っている", "You are resistant to poison");
    }
    screen_save();

    /* Erase the screen */
    for (k = 1; k < 24; k++)
        prt("", k, 13);

    /* Label the information */
    prt(_("    現在かかっている魔法     :", "     Your Current Magic:"), 1, 15);

    /* We will print on top of the map (column 13) */
    for (k = 2, j = 0; j < i; j++) {
        /* Show the info */
        sprintf(Dummy, _("%-28s : 期間 - %s ", "%s %s."), info[j], report_magic_durations[info2[j]]);
        prt(Dummy, k++, 15);

        /* Every 20 entries (lines 2 to 21), start over */
        if ((k == 22) && (j + 1 < i)) {
            prt(_("-- 続く --", "-- more --"), k, 15);
            inkey();
            for (; k > 2; k--)
                prt("", k, 15);
        }
    }

    /* Pause */
    prt(_("[何かキーを押すとゲームに戻ります]", "[Press any key to continue]"), k, 13);
    inkey();
    screen_load();
}
