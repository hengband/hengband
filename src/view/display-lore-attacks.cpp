#include "view/display-lore-attacks.h"
#include "locale/japanese.h"
#include "lore/combat-types-setter.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "monster-attack/monster-attack-table.h"
#include "system/monrace/monrace-definition.h"
#include "term/term-color-types.h"
#include "util/string-processor.h"
#include <fmt/format.h>
#include <string>
#include <vector>

static std::string build_melee_method_abbrev(const RaceBlowMethodType method)
{
    switch (method) {
    case RaceBlowMethodType::HIT:
        return _("殴", "Hit");
    case RaceBlowMethodType::TOUCH:
        return _("触", "Touch");
    case RaceBlowMethodType::PUNCH:
        return _("拳", "Punch");
    case RaceBlowMethodType::KICK:
        return _("蹴", "Kick");
    case RaceBlowMethodType::CLAW:
        return _("爪", "Claw");
    case RaceBlowMethodType::BITE:
        return _("噛", "Bite");
    case RaceBlowMethodType::STING:
        return _("刺", "Sting");
    case RaceBlowMethodType::SLASH:
        return _("斬", "Slash");
    case RaceBlowMethodType::BUTT:
        return _("角", "Butt");
    case RaceBlowMethodType::CRUSH:
        return _("潰", "Crush");
    case RaceBlowMethodType::ENGULF:
        return _("巻", "Engulf");
    case RaceBlowMethodType::CHARGE:
        return _("請", "Charge");
    case RaceBlowMethodType::CRAWL:
        return _("這", "Crawl");
    case RaceBlowMethodType::DROOL:
        return _("涎", "Drool");
    case RaceBlowMethodType::SPIT:
        return _("吐", "Spit");
    case RaceBlowMethodType::EXPLODE:
        return _("爆", "Explode");
    case RaceBlowMethodType::GAZE:
        return _("視", "Gaze");
    case RaceBlowMethodType::WAIL:
        return _("叫", "Wail");
    case RaceBlowMethodType::SPORE:
        return _("胞", "Spore");
    case RaceBlowMethodType::BEG:
        return _("乞", "Beg");
    case RaceBlowMethodType::INSULT:
        return _("罵", "Insult");
    case RaceBlowMethodType::MOAN:
        return _("唸", "Moan");
    case RaceBlowMethodType::SHOW:
        return _("歌", "Show");
    case RaceBlowMethodType::NONE:
    case RaceBlowMethodType::XXX4:
    case RaceBlowMethodType::MAX:
    default:
        return _("?", "??");
    }
}

static std::string build_melee_effect_abbrev(const RaceBlowEffectType effect)
{
    switch (effect) {
    case RaceBlowEffectType::POISON:
        return _("毒", "+Pois");
    case RaceBlowEffectType::UN_BONUS:
        return _("劣", "+UnBonus");
    case RaceBlowEffectType::UN_POWER:
        return _("充-", "+UnPower");
    case RaceBlowEffectType::EAT_GOLD:
        return _("金-", "Gold-");
    case RaceBlowEffectType::EAT_ITEM:
        return _("物-", "Item-");
    case RaceBlowEffectType::EAT_FOOD:
        return _("食-", "Food-");
    case RaceBlowEffectType::EAT_LITE:
        return _("灯-", "Lite-");
    case RaceBlowEffectType::ACID:
        return _("酸", "+Acid");
    case RaceBlowEffectType::ELEC:
        return _("電", "+Elec");
    case RaceBlowEffectType::FIRE:
        return _("火", "+Fire");
    case RaceBlowEffectType::COLD:
        return _("冷", "+Cold");
    case RaceBlowEffectType::BLIND:
        return _("盲", "+Blind");
    case RaceBlowEffectType::CONFUSE:
        return _("乱", "+Conf");
    case RaceBlowEffectType::TERRIFY:
        return _("恐", "+Fear");
    case RaceBlowEffectType::PARALYZE:
        return _("麻", "+Para");
    case RaceBlowEffectType::LOSE_STR:
        return _("腕-", "Str-");
    case RaceBlowEffectType::LOSE_INT:
        return _("知-", "Int-");
    case RaceBlowEffectType::LOSE_WIS:
        return _("賢-", "Wiz-");
    case RaceBlowEffectType::LOSE_DEX:
        return _("器-", "Dex-");
    case RaceBlowEffectType::LOSE_CON:
        return _("耐-", "Con-");
    case RaceBlowEffectType::LOSE_CHR:
        return _("魅-", "Chr-");
    case RaceBlowEffectType::LOSE_ALL:
        return _("全-", "Stat-");
    case RaceBlowEffectType::SHATTER:
        return _("地震", "+Shatter");
    case RaceBlowEffectType::EXP_10:
    case RaceBlowEffectType::EXP_20:
    case RaceBlowEffectType::EXP_40:
    case RaceBlowEffectType::EXP_80:
        return _("経-", "Exp-");
    case RaceBlowEffectType::DISEASE:
        return _("病", "+Disease");
    case RaceBlowEffectType::TIME:
        return _("時", "+Time");
    case RaceBlowEffectType::DR_LIFE:
        return _("命-", "Life-");
    case RaceBlowEffectType::DR_MANA:
        return _("魔-", "Mana-");
    case RaceBlowEffectType::SUPERHURT:
        return _("強", "SuperHurt");
    case RaceBlowEffectType::INERTIA:
        return _("減速", "Inertia");
    case RaceBlowEffectType::STUN:
        return _("朦朧", "Stun");
    case RaceBlowEffectType::HUNGRY:
        return _("飢-", "Hunger-");
    case RaceBlowEffectType::CHAOS:
        return _("沌", "+Chaos");
    case RaceBlowEffectType::FLAVOR:
    case RaceBlowEffectType::NONE:
    case RaceBlowEffectType::HURT:
    case RaceBlowEffectType::MAX:
    default:
        return "";
    }
}

/*!
 * @brief モンスター打撃を方式1(手段ダイス効果)で1行要約して表示する
 */
void display_monster_melee_summary_line(lore_type *lore_ptr)
{
    if (!lore_ptr->is_details_known() && !lore_ptr->know_everything) {
        return;
    }

    // lore_ptr の作業領域(p/q/色)を壊さないよう退避
    const auto *saved_p = lore_ptr->p;
    const auto *saved_q = lore_ptr->q;
    const auto saved_pc = lore_ptr->pc;
    const auto saved_qc = lore_ptr->qc;

    const int max_attack_numbers = 4;
    bool any = false;

    hooked_roff(_("[打撃] ", "[Melee] "));

    for (int m = 0; m < max_attack_numbers; m++) {
        const auto &blow = lore_ptr->monrace->blows[m];
        if (blow.method == RaceBlowMethodType::NONE) {
            continue;
        }

        if (!lore_ptr->know_everything && (lore_ptr->monrace->r_blows[m] == 0)) {
            continue;
        }

        if (any) {
            hooked_roff(" | ");
        }
        any = true;

        // 既存と同じ色決定ロジックを流用
        set_monster_blow_method(lore_ptr, m);
        set_monster_blow_effect(lore_ptr, m);

        const auto method_abbrev = build_melee_method_abbrev(blow.method);
        const auto effect_abbrev = build_melee_effect_abbrev(blow.effect);

        // 手段（pc）
        hook_c_roff(lore_ptr->pc, method_abbrev);

        const bool blow_seen = lore_ptr->know_everything || (lore_ptr->monrace->r_blows[m] != 0);
        const bool dmg_known = lore_ptr->know_everything || lore_ptr->is_blow_damage_known(m);

        // ダイスあり
        if (blow.damage_dice.is_valid()) {
            if (dmg_known) {
                hook_c_roff(TERM_L_WHITE, blow.damage_dice.to_string());
            } else {
                hook_c_roff(TERM_L_WHITE, "??");
            }
            // ダイスなし（触る等）：既知なら 無表記、未知なら ??
        } else {
            if (!blow_seen) {
                hook_c_roff(TERM_L_WHITE, "??");
            }
        }

        // 効果（qc）
        if (!effect_abbrev.empty()) {
            hook_c_roff(lore_ptr->qc, effect_abbrev);
        }
    }

    hooked_roff("\n");

    // 退避復元
    lore_ptr->p = saved_p;
    lore_ptr->q = saved_q;
    lore_ptr->pc = saved_pc;
    lore_ptr->qc = saved_qc;
}

#ifdef JP
/*!
 * @brief [日本語]モンスター打撃の1回分を出力する
 * @param lore_ptr 思い出情報へのポインタ
 * @param attack_numbers 打撃の最大回数
 * @param damage_dice ダメージダイス
 * @param m 打撃の何番目か
 */
static void display_monster_blow_jp(lore_type *lore_ptr, int attack_numbers, const Dice &damage_dice, int m)
{
    if (attack_numbers == 0) {
        hooked_roff(fmt::format("{}は", str_upcase_first(Who::who(lore_ptr->msex))));
    }

    if (damage_dice.is_valid() && (lore_ptr->know_everything || lore_ptr->is_blow_damage_known(m))) {
        hook_c_roff(TERM_L_WHITE, fmt::format(" {} ", damage_dice.to_string()));
        hooked_roff("のダメージで");
    }

    if (!lore_ptr->p) {
        lore_ptr->p = "何か奇妙なことをする";
        lore_ptr->pc = TERM_VIOLET;
    }

    /* XXしてYYし/XXしてYYする/XXし/XXする */
    if (lore_ptr->q != nullptr) {
        const auto verb = conjugate_jverb(lore_ptr->p, JVerbConjugationType::TO);
        hook_c_roff(lore_ptr->pc, verb);
    } else if (attack_numbers != lore_ptr->count - 1) {
        const auto verb = conjugate_jverb(lore_ptr->p, JVerbConjugationType::AND);
        hook_c_roff(lore_ptr->pc, verb);
    } else {
        hook_c_roff(lore_ptr->pc, lore_ptr->p);
    }

    if (lore_ptr->q) {
        if (attack_numbers != lore_ptr->count - 1) {
            const auto verb = conjugate_jverb(lore_ptr->q, JVerbConjugationType::AND);
            hook_c_roff(lore_ptr->qc, verb);
        } else {
            hook_c_roff(lore_ptr->qc, lore_ptr->q);
        }
    }

    if (attack_numbers != lore_ptr->count - 1) {
        hooked_roff("、");
    }
}
#else
/*!
 * @brief [英語]モンスター打撃の1回分を出力する
 * @param lore_ptr 思い出情報へのポインタ
 * @param attack_numbers 打撃の最大回数
 * @param damage_dice ダメージダイス
 * @param m 打撃の何番目か
 */
static void display_monster_blow_en(lore_type *lore_ptr, int attack_numbers, const Dice &damage_dice, int m)
{
    if (attack_numbers == 0) {
        hooked_roff(fmt::format("{} can ", str_upcase_first(Who::who(lore_ptr->msex))));
    } else if (attack_numbers < lore_ptr->count - 1) {
        hooked_roff(", ");
    } else {
        hooked_roff(", and ");
    }

    if (lore_ptr->p == nullptr) {
        lore_ptr->p = "do something weird";
        lore_ptr->pc = TERM_VIOLET;
    }

    hook_c_roff(lore_ptr->pc, lore_ptr->p);
    if (lore_ptr->q != nullptr) {
        hooked_roff(" to ");
        hook_c_roff(lore_ptr->qc, lore_ptr->q);
        if (damage_dice.is_valid() && (lore_ptr->know_everything || lore_ptr->is_blow_damage_known(m))) {
            hooked_roff(" with damage");
            hook_c_roff(TERM_L_WHITE, fmt::format(" {}", damage_dice.to_string()));
        }
    }
}
#endif

/*!
 * @brief モンスター打撃の1回分を出力する(日英切替への踏み台)
 * @param lore_ptr 思い出情報へのポインタ
 * @param m 打撃の何番目か
 * @param attack_numbers 打撃の最大回数
 */
void display_monster_blow(lore_type *lore_ptr, int m, int attack_numbers)
{
    auto display_monster_blows_pf = _(display_monster_blow_jp, display_monster_blow_en);
    display_monster_blows_pf(lore_ptr, attack_numbers, lore_ptr->monrace->blows[m].damage_dice, m);
}

/*!
 * @brief モンスターの思い出に打撃に関する情報を出力する
 * @param lore_ptr 思い出情報へのポインタ
 */
void display_monster_blows(lore_type *lore_ptr)
{
    const int max_attack_numbers = 4;
    for (int m = 0; m < max_attack_numbers; m++) {
        if (lore_ptr->monrace->blows[m].method == RaceBlowMethodType::NONE) {
            continue;
        }

        if (lore_ptr->monrace->r_blows[m] || lore_ptr->know_everything) {
            lore_ptr->count++;
        }
    }

    int attack_numbers = 0;
    for (int m = 0; m < max_attack_numbers; m++) {
        if (lore_ptr->monrace->blows[m].method == RaceBlowMethodType::NONE || (((lore_ptr->monrace->r_blows[m] == 0) && !lore_ptr->know_everything))) {
            continue;
        }

        set_monster_blow_method(lore_ptr, m);
        set_monster_blow_effect(lore_ptr, m);
        display_monster_blow(lore_ptr, m, attack_numbers);
        attack_numbers++;
    }

    if (attack_numbers > 0) {
        hooked_roff(_("。", ".  "));
    } else if (lore_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW)) {
        hooked_roff(fmt::format(_("{}は物理的な攻撃方法を持たない。", "{} has no physical attacks.  "), str_upcase_first(Who::who(lore_ptr->msex))));
    } else {
        hooked_roff(fmt::format(_("{}攻撃については何も知らない。", "Nothing is known about {} attack.  "), Who::whose(lore_ptr->msex)));
    }
}
