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
#include "io/input-key-acceptor.h"
#include "object-enchant/trc-types.h"
#include "player-info/avatar.h"
#include "player-info/base-status-info.h"
#include "player-info/body-improvement-info.h"
#include "player-info/class-ability-info.h"
#include "player-info/mutation-info.h"
#include "player-info/race-ability-info.h"
#include "player-info/resistance-info.h"
#include "player-info/self-info-util.h"
#include "player-info/weapon-effect-info.h"
#include "player/attack-defense-types.h"
#include "term/screen-processor.h"
#include "view/display-self-info.h"

static void set_bad_status_info(player_type *creature_ptr, self_info_type *self_ptr)
{
    if (creature_ptr->blind)
        self_ptr->info[self_ptr->line++] = _("あなたは目が見えない。", "You cannot see.");

    if (creature_ptr->confused)
        self_ptr->info[self_ptr->line++] = _("あなたは混乱している。", "You are confused.");

    if (creature_ptr->afraid)
        self_ptr->info[self_ptr->line++] = _("あなたは恐怖に侵されている。", "You are terrified.");

    if (creature_ptr->cut)
        self_ptr->info[self_ptr->line++] = _("あなたは出血している。", "You are bleeding.");

    if (creature_ptr->stun)
        self_ptr->info[self_ptr->line++] = _("あなたはもうろうとしている。", "You are stunned.");

    if (creature_ptr->poisoned)
        self_ptr->info[self_ptr->line++] = _("あなたは毒に侵されている。", "You are poisoned.");

    if (creature_ptr->image)
        self_ptr->info[self_ptr->line++] = _("あなたは幻覚を見ている。", "You are hallucinating.");
}

static void set_curse_info(player_type *creature_ptr, self_info_type *self_ptr)
{
    if (creature_ptr->cursed & TRC_TY_CURSE)
        self_ptr->info[self_ptr->line++] = _("あなたは邪悪な怨念に包まれている。", "You carry an ancient foul curse.");

    if (creature_ptr->cursed & TRC_AGGRAVATE)
        self_ptr->info[self_ptr->line++] = _("あなたはモンスターを怒らせている。", "You aggravate monsters.");

    if (creature_ptr->cursed & TRC_DRAIN_EXP)
        self_ptr->info[self_ptr->line++] = _("あなたは経験値を吸われている。", "You occasionally lose experience for no reason.");

    if (creature_ptr->cursed & TRC_SLOW_REGEN)
        self_ptr->info[self_ptr->line++] = _("あなたの回復力は非常に遅い。", "You regenerate slowly.");

    if (creature_ptr->cursed & TRC_ADD_L_CURSE)
        self_ptr->info[self_ptr->line++] = _("あなたの弱い呪いは増える。", "Your weak curses multiply."); /* 暫定的 -- henkma */

    if (creature_ptr->cursed & TRC_ADD_H_CURSE)
        self_ptr->info[self_ptr->line++] = _("あなたの強い呪いは増える。", "Your heavy curses multiply."); /* 暫定的 -- henkma */

    if (creature_ptr->cursed & TRC_CALL_ANIMAL)
        self_ptr->info[self_ptr->line++] = _("あなたは動物に狙われている。", "You attract animals.");

    if (creature_ptr->cursed & TRC_CALL_DEMON)
        self_ptr->info[self_ptr->line++] = _("あなたは悪魔に狙われている。", "You attract demons.");

    if (creature_ptr->cursed & TRC_CALL_DRAGON)
        self_ptr->info[self_ptr->line++] = _("あなたはドラゴンに狙われている。", "You attract dragons.");

    if (creature_ptr->cursed & TRC_COWARDICE)
        self_ptr->info[self_ptr->line++] = _("あなたは時々臆病になる。", "You are subject to cowardice.");

    if (creature_ptr->cursed & TRC_TELEPORT)
        self_ptr->info[self_ptr->line++] = _("あなたの位置はひじょうに不安定だ。", "Your position is very uncertain.");

    if (creature_ptr->cursed & TRC_LOW_MELEE)
        self_ptr->info[self_ptr->line++] = _("あなたの武器は攻撃を外しやすい。", "Your weapon causes you to miss blows.");

    if (creature_ptr->cursed & TRC_LOW_AC)
        self_ptr->info[self_ptr->line++] = _("あなたは攻撃を受けやすい。", "You are subject to be hit.");

    if (creature_ptr->cursed & TRC_LOW_MAGIC)
        self_ptr->info[self_ptr->line++] = _("あなたは魔法を失敗しやすい。", "Your spells fail more frequently.");

    if (creature_ptr->cursed & TRC_FAST_DIGEST)
        self_ptr->info[self_ptr->line++] = _("あなたはすぐお腹がへる。", "You have a good appetite.");

    if (creature_ptr->cursed & TRC_DRAIN_HP)
        self_ptr->info[self_ptr->line++] = _("あなたは体力を吸われている。", "You occasionally lose hit points for no reason.");

    if (creature_ptr->cursed & TRC_DRAIN_MANA)
        self_ptr->info[self_ptr->line++] = _("あなたは魔力を吸われている。", "You occasionally lose spell points for no reason.");
}

static void set_special_attack_info(player_type *creature_ptr, self_info_type *self_ptr)
{
    if (creature_ptr->special_attack & ATTACK_CONFUSE)
        self_ptr->info[self_ptr->line++] = _("あなたの手は赤く輝いている。", "Your hands are glowing dull red.");

    if (creature_ptr->special_attack & ATTACK_FIRE)
        self_ptr->info[self_ptr->line++] = _("あなたの手は火炎に覆われている。", "You can strike the enemy with flame.");

    if (creature_ptr->special_attack & ATTACK_COLD)
        self_ptr->info[self_ptr->line++] = _("あなたの手は冷気に覆われている。", "You can strike the enemy with cold.");

    if (creature_ptr->special_attack & ATTACK_ACID)
        self_ptr->info[self_ptr->line++] = _("あなたの手は酸に覆われている。", "You can strike the enemy with acid.");

    if (creature_ptr->special_attack & ATTACK_ELEC)
        self_ptr->info[self_ptr->line++] = _("あなたの手は電撃に覆われている。", "You can strike the enemy with electoric shock.");

    if (creature_ptr->special_attack & ATTACK_POIS)
        self_ptr->info[self_ptr->line++] = _("あなたの手は毒に覆われている。", "You can strike the enemy with poison.");
}

static void set_esp_info(player_type *creature_ptr, self_info_type *self_ptr)
{
    if (creature_ptr->telepathy)
        self_ptr->info[self_ptr->line++] = _("あなたはテレパシー能力を持っている。", "You have ESP.");

    if (creature_ptr->esp_animal)
        self_ptr->info[self_ptr->line++] = _("あなたは自然界の生物の存在を感じる能力を持っている。", "You sense natural creatures.");

    if (creature_ptr->esp_undead)
        self_ptr->info[self_ptr->line++] = _("あなたはアンデッドの存在を感じる能力を持っている。", "You sense undead.");

    if (creature_ptr->esp_demon)
        self_ptr->info[self_ptr->line++] = _("あなたは悪魔の存在を感じる能力を持っている。", "You sense demons.");

    if (creature_ptr->esp_orc)
        self_ptr->info[self_ptr->line++] = _("あなたはオークの存在を感じる能力を持っている。", "You sense orcs.");

    if (creature_ptr->esp_troll)
        self_ptr->info[self_ptr->line++] = _("あなたはトロルの存在を感じる能力を持っている。", "You sense trolls.");

    if (creature_ptr->esp_giant)
        self_ptr->info[self_ptr->line++] = _("あなたは巨人の存在を感じる能力を持っている。", "You sense giants.");

    if (creature_ptr->esp_dragon)
        self_ptr->info[self_ptr->line++] = _("あなたはドラゴンの存在を感じる能力を持っている。", "You sense dragons.");

    if (creature_ptr->esp_human)
        self_ptr->info[self_ptr->line++] = _("あなたは人間の存在を感じる能力を持っている。", "You sense humans.");

    if (creature_ptr->esp_evil)
        self_ptr->info[self_ptr->line++] = _("あなたは邪悪な生き物の存在を感じる能力を持っている。", "You sense evil creatures.");

    if (creature_ptr->esp_good)
        self_ptr->info[self_ptr->line++] = _("あなたは善良な生き物の存在を感じる能力を持っている。", "You sense good creatures.");

    if (creature_ptr->esp_nonliving)
        self_ptr->info[self_ptr->line++] = _("あなたは活動する無生物体の存在を感じる能力を持っている。", "You sense non-living creatures.");

    if (creature_ptr->esp_unique)
        self_ptr->info[self_ptr->line++] = _("あなたは特別な強敵の存在を感じる能力を持っている。", "You sense unique monsters.");
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
    self_info_type *self_ptr = initialize_self_info_type(&tmp_si);
    display_life_rating(creature_ptr, self_ptr);
    chg_virtue(creature_ptr, V_KNOWLEDGE, 1);
    chg_virtue(creature_ptr, V_ENLIGHTEN, 1);
    display_max_base_status(creature_ptr, self_ptr);
    display_virtue(creature_ptr, self_ptr);
    self_ptr->info[self_ptr->line++] = "";
    if (creature_ptr->mimic_form)
        display_mimic_race_ability(creature_ptr, self_ptr);
    else
        set_race_ability_info(creature_ptr, self_ptr);

    set_class_ability_info(creature_ptr, self_ptr);
    set_mutation_info_1(creature_ptr, self_ptr);
    set_mutation_info_2(creature_ptr, self_ptr);
    set_mutation_info_3(creature_ptr, self_ptr);
    set_bad_status_info(creature_ptr, self_ptr);
    set_curse_info(creature_ptr, self_ptr);
    set_body_improvement_info_1(creature_ptr, self_ptr);
    set_special_attack_info(creature_ptr, self_ptr);
    switch (creature_ptr->action) {
    case ACTION_SEARCH:
        self_ptr->info[self_ptr->line++] = _("あなたはひじょうに注意深く周囲を見渡している。", "You are looking around very carefully.");
        break;
    }

    set_body_improvement_info_2(creature_ptr, self_ptr);
    set_esp_info(creature_ptr, self_ptr);
    set_body_improvement_info_3(creature_ptr, self_ptr);
    set_element_resistance_info(creature_ptr, self_ptr);
    set_high_resistance_info(creature_ptr, self_ptr);
    set_body_improvement_info_4(creature_ptr, self_ptr);
    set_status_sustain_info(creature_ptr, self_ptr);
    set_equipment_influence(creature_ptr, self_ptr);
    set_weapon_effect_info(creature_ptr, self_ptr);
    display_self_info(self_ptr);
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

static concptr report_magic_durations[] = { _("ごく短い間", "for a short time"), _("少しの間", "for a little while"), _("しばらくの間", "for a while"),
    _("多少長い間", "for a long while"), _("長い間", "for a long time"), _("非常に長い間", "for a very long time"),
    _("信じ難いほど長い間", "for an incredibly long time"), _("モンスターを攻撃するまで", "until you hit a monster") };

/*!
 * @brief 現在の一時的効果一覧を返す / Report all currently active magical effects.
 * @return なし
 */
void report_magics(player_type *creature_ptr)
{
    int i = 0;
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

    if (is_shero(creature_ptr)) {
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
    for (int k = 1; k < 24; k++)
        prt("", k, 13);

    prt(_("    現在かかっている魔法     :", "     Your Current Magic:"), 1, 15);
    int k = 2;
    char buf[80];
    for (int j = 0; j < i; j++) {
        sprintf(buf, _("%-28s : 期間 - %s ", "%s %s."), info[j], report_magic_durations[info2[j]]);
        prt(buf, k++, 15);

        /* Every 20 entries (lines 2 to 21), start over */
        if ((k == 22) && (j + 1 < i)) {
            prt(_("-- 続く --", "-- more --"), k, 15);
            inkey();
            for (; k > 2; k--)
                prt("", k, 15);
        }
    }

    prt(_("[何かキーを押すとゲームに戻ります]", "[Press any key to continue]"), k, 13);
    inkey();
    screen_load();
}
