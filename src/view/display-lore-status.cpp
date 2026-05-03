#include "view/display-lore-status.h"
#include "locale/japanese.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "monster-race/race-brightness-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "term/term-color-types.h"
#include <tl/optional.hpp>
#include <vector>

void display_monster_hp_ac(lore_type *lore_ptr)
{
    if (!lore_ptr->is_details_known() && !lore_ptr->know_everything) {
        return;
    }

    hooked_roff(format(_("%s^は AC%d の防御力と", "%s^ has an armor rating of %d"), Who::who(lore_ptr->msex).data(), lore_ptr->r_ptr->ac));
    if (lore_ptr->misc_flags.has(MonsterMiscType::FORCE_MAXHP) || (lore_ptr->r_ptr->hit_dice.sides == 1)) {
        auto hp = lore_ptr->r_ptr->hit_dice.maxroll() * (lore_ptr->nightmare ? 2 : 1);
        hooked_roff(format(_(" %d の体力がある。", " and a life rating of %d.  "), std::min(MONSTER_MAXHP, hp)));
    } else {
        auto hit_dice = lore_ptr->r_ptr->hit_dice;
        if (lore_ptr->nightmare) {
            hit_dice.num *= 2;
        }
        hooked_roff(format(
            _(" %s の体力がある。", " and a life rating of %s.  "), hit_dice.to_string().data()));
    }
}

void display_monster_hp_ac_summary(lore_type *lore_ptr)
{
    if (!lore_ptr->is_details_known() && !lore_ptr->know_everything) {
        hooked_roff(_("最大HP:???  AC:??? ",
            "MaxHP:???  AC:??? "));
        return;
    }

    std::string hp_text;
    if (lore_ptr->misc_flags.has(MonsterMiscType::FORCE_MAXHP) || (lore_ptr->r_ptr->hit_dice.sides == 1)) {
        auto hp = lore_ptr->r_ptr->hit_dice.maxroll() * (lore_ptr->nightmare ? 2 : 1);
        hp_text = format("%d", std::min(MONSTER_MAXHP, hp));
    } else {
        auto hit_dice = lore_ptr->r_ptr->hit_dice;
        if (lore_ptr->nightmare) {
            hit_dice.num *= 2;
        }
        hp_text = hit_dice.to_string().data();
    }

    const auto ac = lore_ptr->r_ptr->ac;

    hooked_roff(format(_("最大HP:%s AC:%d ", "MaxHP:%s AC:%d "), hp_text.data(), ac));
}

void display_monster_concrete_abilities(lore_type *lore_ptr)
{
    if (lore_ptr->brightness_flags.has_any_of({ MonsterBrightnessType::HAS_LITE_1, MonsterBrightnessType::HAS_LITE_2 })) {
        lore_ptr->lore_msgs.emplace_back(_("ダンジョンを照らす", "illuminate the dungeon"), TERM_WHITE);
    }

    if (lore_ptr->brightness_flags.has_any_of({ MonsterBrightnessType::HAS_DARK_1, MonsterBrightnessType::HAS_DARK_2 })) {
        lore_ptr->lore_msgs.emplace_back(_("ダンジョンを暗くする", "darken the dungeon"), TERM_L_DARK);
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::OPEN_DOOR)) {
        lore_ptr->lore_msgs.emplace_back(_("ドアを開ける", "open doors"), TERM_WHITE);
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::BASH_DOOR)) {
        lore_ptr->lore_msgs.emplace_back(_("ドアを打ち破る", "bash down doors"), TERM_WHITE);
    }

    if (lore_ptr->r_ptr->feature_flags.has(MonsterFeatureType::CAN_FLY)) {
        lore_ptr->lore_msgs.emplace_back(_("空を飛ぶ", "fly"), TERM_WHITE);
    }

    if (lore_ptr->r_ptr->feature_flags.has(MonsterFeatureType::CAN_SWIM)) {
        lore_ptr->lore_msgs.emplace_back(_("水を渡る", "swim"), TERM_WHITE);
    }

    if (lore_ptr->feature_flags.has(MonsterFeatureType::PASS_WALL)) {
        lore_ptr->lore_msgs.emplace_back(_("壁をすり抜ける", "pass through walls"), TERM_WHITE);
    }

    if (lore_ptr->feature_flags.has(MonsterFeatureType::KILL_WALL)) {
        lore_ptr->lore_msgs.emplace_back(_("壁を掘り進む", "bore through walls"), TERM_WHITE);
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::MOVE_BODY)) {
        lore_ptr->lore_msgs.emplace_back(_("弱いモンスターを押しのける", "push past weaker monsters"), TERM_WHITE);
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::KILL_BODY)) {
        lore_ptr->lore_msgs.emplace_back(_("弱いモンスターを倒す", "destroy weaker monsters"), TERM_WHITE);
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::TAKE_ITEM)) {
        lore_ptr->lore_msgs.emplace_back(_("アイテムを拾う", "pick up objects"), TERM_WHITE);
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::KILL_ITEM)) {
        lore_ptr->lore_msgs.emplace_back(_("アイテムを壊す", "destroy objects"), TERM_WHITE);
    }
}

void display_monster_abilities(lore_type *lore_ptr)
{
    if (lore_ptr->lore_msgs.empty()) {
        return;
    }

    hooked_roff(format(_("%s^は", "%s^"), Who::who(lore_ptr->msex).data()));
    for (int n = 0; const auto &[msg, color] : lore_ptr->lore_msgs) {
#ifdef JP
        if (n != std::ssize(lore_ptr->lore_msgs) - 1) {
            const auto verb = conjugate_jverb(msg, JVerbConjugationType::AND);
            hook_c_roff(color, verb);
            hooked_roff("、");
        } else {
            hook_c_roff(color, msg);
        }
#else
        if (n == 0) {
            hooked_roff(" can ");
        } else if (n < std::ssize(lore_ptr->lore_msgs) - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" and ");
        }

        hook_c_roff(color, msg);
#endif
        n++;
    }

    hooked_roff(_("ことができる。", ".  "));
}

void display_monster_constitutions(lore_type *lore_ptr)
{
    if (lore_ptr->r_ptr->feature_flags.has(MonsterFeatureType::AQUATIC)) {
        hooked_roff(format(_("%s^は水中に棲んでいる。", "%s^ lives in water.  "), Who::who(lore_ptr->msex).data()));
    }

    if (lore_ptr->brightness_flags.has_any_of({ MonsterBrightnessType::SELF_LITE_1, MonsterBrightnessType::SELF_LITE_2 })) {
        hooked_roff(format(_("%s^は光っている。", "%s^ is shining.  "), Who::who(lore_ptr->msex).data()));
    }

    if (lore_ptr->brightness_flags.has_any_of({ MonsterBrightnessType::SELF_DARK_1, MonsterBrightnessType::SELF_DARK_2 })) {
        hook_c_roff(TERM_L_DARK, format(_("%s^は暗黒に包まれている。", "%s^ is surrounded by darkness.  "), Who::who(lore_ptr->msex).data()));
    }

    if (lore_ptr->misc_flags.has(MonsterMiscType::INVISIBLE)) {
        hooked_roff(format(_("%s^は透明で目に見えない。", "%s^ is invisible.  "), Who::who(lore_ptr->msex).data()));
    }

    if (lore_ptr->misc_flags.has(MonsterMiscType::COLD_BLOOD)) {
        hooked_roff(format(_("%s^は冷血動物である。", "%s^ is cold blooded.  "), Who::who(lore_ptr->msex).data()));
    }

    if (lore_ptr->misc_flags.has(MonsterMiscType::EMPTY_MIND)) {
        hooked_roff(format(_("%s^はテレパシーでは感知できない。", "%s^ is not detected by telepathy.  "), Who::who(lore_ptr->msex).data()));
    } else if (lore_ptr->misc_flags.has(MonsterMiscType::WEIRD_MIND)) {
        hooked_roff(format(_("%s^はまれにテレパシーで感知できる。", "%s^ is rarely detected by telepathy.  "), Who::who(lore_ptr->msex).data()));
    }

    if (lore_ptr->misc_flags.has(MonsterMiscType::MULTIPLY)) {
        hook_c_roff(TERM_L_UMBER, format(_("%s^は爆発的に増殖する。", "%s^ breeds explosively.  "), Who::who(lore_ptr->msex).data()));
    }

    if (lore_ptr->misc_flags.has(MonsterMiscType::REGENERATE)) {
        hook_c_roff(TERM_L_WHITE, format(_("%s^は素早く体力を回復する。", "%s^ regenerates quickly.  "), Who::who(lore_ptr->msex).data()));
    }

    if (lore_ptr->misc_flags.has(MonsterMiscType::RIDING)) {
        hook_c_roff(TERM_SLATE, format(_("%s^に乗ることができる。", "%s^ is suitable for riding.  "), Who::who(lore_ptr->msex).data()));
    }

    if (lore_ptr->misc_flags.has(MonsterMiscType::STALKER)) {
        hook_c_roff(TERM_L_RED, format(_("%s^はプレイヤーの背後に忍び寄ることがある。", "%s^ stalks the player.  "), Who::who(lore_ptr->msex).data()));
    }
}

void display_monster_concrete_weakness(lore_type *lore_ptr)
{
    if (lore_ptr->resistance_flags.has(MonsterResistanceType::HURT_ROCK)) {
        lore_ptr->lore_msgs.emplace_back(_("岩を除去するもの", "rock remover"), TERM_UMBER);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::HURT_LITE)) {
        lore_ptr->lore_msgs.emplace_back(_("明るい光", "bright light"), TERM_YELLOW);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::HURT_FIRE)) {
        lore_ptr->lore_msgs.emplace_back(_("炎", "fire"), TERM_RED);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::HURT_COLD)) {
        lore_ptr->lore_msgs.emplace_back(_("冷気", "cold"), TERM_L_WHITE);
    }
}

void display_monster_weakness(lore_type *lore_ptr)
{
    if (lore_ptr->lore_msgs.empty()) {
        return;
    }

    hooked_roff(format(_("%s^には", "%s^"), Who::who(lore_ptr->msex).data()));
    for (int n = 0; const auto &[msg, color] : lore_ptr->lore_msgs) {
#ifdef JP
        if (n != 0) {
            hooked_roff("や");
        }
#else
        if (n == 0) {
            hooked_roff(" is hurt by ");
        } else if (n < std::ssize(lore_ptr->lore_msgs) - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" and ");
        }
#endif
        hook_c_roff(color, msg);
        n++;
    }

    hooked_roff(_("でダメージを与えられる。", ".  "));
}

void display_monster_concrete_resistances(lore_type *lore_ptr)
{
    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_ACID)) {
        lore_ptr->lore_msgs.emplace_back(_("酸", "acid"), TERM_GREEN);
    } else if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ACID)) {
        lore_ptr->lore_msgs.emplace_back(_("酸", "acid"), TERM_GREEN);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_ELEC)) {
        lore_ptr->lore_msgs.emplace_back(_("稲妻", "lightning"), TERM_BLUE);
    } else if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ELEC)) {
        lore_ptr->lore_msgs.emplace_back(_("稲妻", "lightning"), TERM_BLUE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_FIRE)) {
        lore_ptr->lore_msgs.emplace_back(_("炎", "fire"), TERM_RED);
    } else if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_FIRE)) {
        lore_ptr->lore_msgs.emplace_back(_("炎", "fire"), TERM_RED);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_COLD)) {
        lore_ptr->lore_msgs.emplace_back(_("冷気", "cold"), TERM_L_WHITE);
    } else if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_COLD)) {
        lore_ptr->lore_msgs.emplace_back(_("冷気", "cold"), TERM_L_WHITE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_POISON)) {
        lore_ptr->lore_msgs.emplace_back(_("毒", "poison"), TERM_L_GREEN);
    } else if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_POISON)) {
        lore_ptr->lore_msgs.emplace_back(_("毒", "poison"), TERM_L_GREEN);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_LITE)) {
        lore_ptr->lore_msgs.emplace_back(_("閃光", "light"), TERM_YELLOW);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_DARK)) {
        lore_ptr->lore_msgs.emplace_back(_("暗黒", "dark"), TERM_L_DARK);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_NETHER)) {
        lore_ptr->lore_msgs.emplace_back(_("地獄", "nether"), TERM_L_DARK);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_WATER)) {
        lore_ptr->lore_msgs.emplace_back(_("水", "water"), TERM_BLUE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_PLASMA)) {
        lore_ptr->lore_msgs.emplace_back(_("プラズマ", "plasma"), TERM_L_RED);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_SHARDS)) {
        lore_ptr->lore_msgs.emplace_back(_("破片", "shards"), TERM_L_UMBER);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_SOUND)) {
        lore_ptr->lore_msgs.emplace_back(_("轟音", "sound"), TERM_ORANGE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_CHAOS)) {
        lore_ptr->lore_msgs.emplace_back(_("カオス", "chaos"), TERM_VIOLET);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_NEXUS)) {
        lore_ptr->lore_msgs.emplace_back(_("因果混乱", "nexus"), TERM_VIOLET);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_DISENCHANT)) {
        lore_ptr->lore_msgs.emplace_back(_("劣化", "disenchantment"), TERM_VIOLET);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_FORCE)) {
        lore_ptr->lore_msgs.emplace_back(_("フォース", "force"), TERM_UMBER);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_INERTIA)) {
        lore_ptr->lore_msgs.emplace_back(_("遅鈍", "inertia"), TERM_SLATE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_TIME)) {
        lore_ptr->lore_msgs.emplace_back(_("時間逆転", "time"), TERM_L_BLUE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_GRAVITY)) {
        lore_ptr->lore_msgs.emplace_back(_("重力", "gravity"), TERM_SLATE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_METEOR)) {
        lore_ptr->lore_msgs.emplace_back(_("隕石", "meteor"), TERM_UMBER);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        lore_ptr->lore_msgs.emplace_back(_("あらゆる攻撃", "all"), TERM_YELLOW);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT) && lore_ptr->r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        lore_ptr->lore_msgs.emplace_back(_("テレポート", "teleportation"), TERM_ORANGE);
    }
}

void display_monster_resistances(lore_type *lore_ptr)
{
    if (lore_ptr->lore_msgs.empty()) {
        return;
    }

    hooked_roff(format(_("%s^は", "%s^"), Who::who(lore_ptr->msex).data()));
    for (int n = 0; const auto &[msg, color] : lore_ptr->lore_msgs) {
#ifdef JP
        if (n != 0) {
            hooked_roff("と");
        }
#else
        if (n == 0) {
            hooked_roff(" resists ");
        } else if (n < std::ssize(lore_ptr->lore_msgs) - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" and ");
        }
#endif
        hook_c_roff(color, msg);
        n++;
    }

    hooked_roff(_("の耐性を持っている。", ".  "));
}

void display_monster_evolution_summary(lore_type *lore_ptr)
{
    if (!lore_ptr->r_ptr->r_can_evolve && !lore_ptr->know_everything) {
        return;
    }

    const auto &monrace_next = lore_ptr->r_ptr->get_next();
    if (monrace_next.is_valid()) {
        hooked_roff(format(_("進化:%s ", "evolve:%s "), monrace_next.name.data()));
    } else if (lore_ptr->r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        hooked_roff(_("進化:なし ", "evolve:none "));
    }
}

void display_monster_evolution(lore_type *lore_ptr)
{
    if (!lore_ptr->r_ptr->r_can_evolve && !lore_ptr->know_everything) {
        return;
    }

    const auto &monrace_next = lore_ptr->r_ptr->get_next();
    if (monrace_next.is_valid()) {
        hooked_roff(format(_("%s^は経験を積むと、", "%s^ will evolve into "), Who::who(lore_ptr->msex).data()));
        hook_c_roff(TERM_YELLOW, format("%s", monrace_next.name.data()));
        hooked_roff(_(format("に進化する。"), format(" when %s gets enough experience.  ", Who::who(lore_ptr->msex).data())));
    } else if (lore_ptr->r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        hooked_roff(format(_("%sは進化しない。", "%s won't evolve.  "), Who::who(lore_ptr->msex).data()));
    }
}

void display_monster_concrete_immunities(lore_type *lore_ptr)
{
    if (lore_ptr->resistance_flags.has(MonsterResistanceType::NO_STUN)) {
        lore_ptr->lore_msgs.emplace_back(_("朦朧としない", "stunned"), TERM_ORANGE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::NO_FEAR)) {
        lore_ptr->lore_msgs.emplace_back(_("恐怖を感じない", "frightened"), TERM_SLATE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::NO_CONF)) {
        lore_ptr->lore_msgs.emplace_back(_("混乱しない", "confused"), TERM_L_UMBER);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::NO_SLEEP)) {
        lore_ptr->lore_msgs.emplace_back(_("眠らされない", "slept"), TERM_BLUE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT) && lore_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        lore_ptr->lore_msgs.emplace_back(_("テレポートされない", "teleported"), TERM_ORANGE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::NO_INSTANTLY_DEATH) || lore_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        lore_ptr->lore_msgs.emplace_back(_("即死しない", "instantly killed"), TERM_L_DARK);
    }
}

void display_monster_immunities(lore_type *lore_ptr)
{
    if (lore_ptr->lore_msgs.empty()) {
        return;
    }

    hooked_roff(format(_("%s^は", "%s^"), Who::who(lore_ptr->msex).data()));
    for (int n = 0; const auto &[msg, color] : lore_ptr->lore_msgs) {
#ifdef JP
        if (n != 0) {
            hooked_roff("し、");
        }
#else
        if (n == 0) {
            hooked_roff(" cannot be ");
        } else if (n < std::ssize(lore_ptr->lore_msgs) - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" or ");
        }
#endif
        hook_c_roff(color, msg);
        n++;
    }

    hooked_roff(_("。", ".  "));
}

void display_monster_alert_summary(lore_type *lore_ptr)
{
    auto alert = ((int)lore_ptr->r_ptr->r_wake * (int)lore_ptr->r_ptr->r_wake) > lore_ptr->r_ptr->sleep;
    alert |= lore_ptr->r_ptr->r_ignore == MAX_UCHAR;
    alert |= (lore_ptr->r_ptr->sleep == 0) && (lore_ptr->r_ptr->r_tkills >= 10);
    alert |= lore_ptr->know_everything;
    if (!alert) {
        return;
    }
    hooked_roff(format(_("警戒度:%d/255 感知範囲:%dft ", "alertness:%d notice:%dft"), 255 - lore_ptr->r_ptr->sleep, 10 * lore_ptr->r_ptr->aaf));
}

void display_monster_alert(lore_type *lore_ptr)
{
    auto alert = ((int)lore_ptr->r_ptr->r_wake * (int)lore_ptr->r_ptr->r_wake) > lore_ptr->r_ptr->sleep;
    alert |= lore_ptr->r_ptr->r_ignore == MAX_UCHAR;
    alert |= (lore_ptr->r_ptr->sleep == 0) && (lore_ptr->r_ptr->r_tkills >= 10);
    alert |= lore_ptr->know_everything;
    if (!alert) {
        return;
    }

    std::string action;
    if (lore_ptr->r_ptr->sleep > 200) {
        action = _("を無視しがちであるが", "prefers to ignore");
    } else if (lore_ptr->r_ptr->sleep > 95) {
        action = _("に対してほとんど注意を払わないが", "pays very little attention to");
    } else if (lore_ptr->r_ptr->sleep > 75) {
        action = _("に対してあまり注意を払わないが", "pays little attention to");
    } else if (lore_ptr->r_ptr->sleep > 45) {
        action = _("を見過ごしがちであるが", "tends to overlook");
    } else if (lore_ptr->r_ptr->sleep > 25) {
        action = _("をほんの少しは見ており", "takes quite a while to see");
    } else if (lore_ptr->r_ptr->sleep > 10) {
        action = _("をしばらくは見ており", "takes a while to see");
    } else if (lore_ptr->r_ptr->sleep > 5) {
        action = _("を幾分注意深く見ており", "is fairly observant of");
    } else if (lore_ptr->r_ptr->sleep > 3) {
        action = _("を注意深く見ており", "is observant of");
    } else if (lore_ptr->r_ptr->sleep > 1) {
        action = _("をかなり注意深く見ており", "is very observant of");
    } else if (lore_ptr->r_ptr->sleep > 0) {
        action = _("を警戒しており", "is vigilant for");
    } else {
        action = _("をかなり警戒しており", "is ever vigilant for");
    }

    constexpr auto fmt = _("%s^は侵入者%s、 %d フィート先から侵入者に気付くことがある。", "%s^ %s intruders, which %s may notice from %d feet.  ");
    const auto who = Who::who(lore_ptr->msex);
    hooked_roff(_(format(fmt, who.data(), action.data(), 10 * lore_ptr->r_ptr->aaf),
        format(fmt, who.data(), action.data(), who.data(), 10 * lore_ptr->r_ptr->aaf)));
}

static void push_tag(std::vector<std::string> &tags, const char *msg)
{
    tags.emplace_back(msg);
}

void display_monster_behavior_summary(lore_type *lore_ptr)
{
    if (!lore_ptr->is_details_known() && !lore_ptr->know_everything && lore_ptr->lore_msgs.empty()) {
        return;
    }
    std::vector<std::string> tags;
    // --- behavior_flags (MonsterBehaviorType) ---
    const auto &bf = lore_ptr->behavior_flags;
    if (bf.has(MonsterBehaviorType::NEVER_MOVE)) {
        push_tag(tags, _("不動", "NEVER_MOVE"));
    }
    if (bf.has(MonsterBehaviorType::OPEN_DOOR)) {
        push_tag(tags, _("扉開", "OPEN_DOOR"));
    }
    if (bf.has(MonsterBehaviorType::BASH_DOOR)) {
        push_tag(tags, _("扉壊", "BASH_DOOR"));
    }
    if (bf.has(MonsterBehaviorType::MOVE_BODY)) {
        push_tag(tags, _("他者移動", "MOVE_BODY"));
    }
    if (bf.has(MonsterBehaviorType::KILL_BODY)) {
        push_tag(tags, _("他者攻撃", "KILL_BODY"));
    }
    if (bf.has(MonsterBehaviorType::TAKE_ITEM)) {
        push_tag(tags, _("アイテム拾得", "TAKE_ITEM"));
    }
    if (bf.has(MonsterBehaviorType::KILL_ITEM)) {
        push_tag(tags, _("アイテム破壊", "KILL_ITEM"));
    }
    if (bf.has(MonsterBehaviorType::RAND_MOVE_25) && bf.has(MonsterBehaviorType::RAND_MOVE_50)) {
        push_tag(tags, _("乱歩75", "RAND_MOVE_75"));
    } else {
        if (bf.has(MonsterBehaviorType::RAND_MOVE_25)) {
            push_tag(tags, _("乱歩25", "RAND_MOVE_25"));
        }
        if (bf.has(MonsterBehaviorType::RAND_MOVE_50)) {
            push_tag(tags, _("乱歩50", "RAND_MOVE_50"));
        }
    }
    if (bf.has(MonsterBehaviorType::FRIENDLY)) {
        push_tag(tags, _("友好", "FRIENDLY"));
    }

    const auto &mf = lore_ptr->misc_flags;
    if (mf.has(MonsterMiscType::FORCE_DEPTH)) {
        push_tag(tags, _("深度固定", "FORCE_DEPTH"));
    }
    if (mf.has(MonsterMiscType::HAS_FRIENDS)) {
        push_tag(tags, _("群体", "HAS_FRIENDS"));
    }
    if (mf.has(MonsterMiscType::ESCORT)) {
        push_tag(tags, _("護衛", "ESCORT"));
    }
    if (mf.has(MonsterMiscType::MORE_ESCORT)) {
        push_tag(tags, _("護衛多", "MORE_ESCORT"));
    }
    if (mf.has(MonsterMiscType::RIDING)) {
        push_tag(tags, _("乗馬可", "RIDING"));
    }
    if (mf.has(MonsterMiscType::INVISIBLE)) {
        push_tag(tags, _("透明", "INVISIBLE"));
    }
    if (mf.has(MonsterMiscType::COLD_BLOOD)) {
        push_tag(tags, _("冷血", "COLD_BLOOD"));
    }
    if (mf.has(MonsterMiscType::KAGE)) {
        push_tag(tags, _("影", "KAGE"));
    }
    if (mf.has(MonsterMiscType::CHAMELEON)) {
        push_tag(tags, _("変身", "CHAMELEON"));
    }
    if (mf.has(MonsterMiscType::TANUKI)) {
        push_tag(tags, _("狸", "TANUKI"));
    }
    if (mf.has(MonsterMiscType::ELDRITCH_HORROR)) {
        push_tag(tags, _("狂気", "ELDRITCH_HORROR"));
    }
    if (mf.has(MonsterMiscType::MULTIPLY)) {
        push_tag(tags, _("増殖", "MULTIPLY"));
    }
    if (mf.has(MonsterMiscType::REGENERATE)) {
        push_tag(tags, _("再生", "REGENERATE"));
    }
    if (mf.has(MonsterMiscType::EMPTY_MIND)) {
        push_tag(tags, _("精神無", "EMPTY_MIND"));
    }
    if (mf.has(MonsterMiscType::WEIRD_MIND)) {
        push_tag(tags, _("精神歪", "WEIRD_MIND"));
    }

    const auto &ff = lore_ptr->feature_flags;
    if (ff.has(MonsterFeatureType::AQUATIC)) {
        push_tag(tags, _("水棲", "AQUATIC"));
    }
    if (ff.has(MonsterFeatureType::CAN_SWIM)) {
        push_tag(tags, _("水渡", "CAN_SWIM"));
    }
    if (ff.has(MonsterFeatureType::CAN_FLY)) {
        push_tag(tags, _("飛行", "CAN_FLY"));
    }
    if (ff.has(MonsterFeatureType::PASS_WALL)) {
        push_tag(tags, _("壁抜け", "PASS_WALL"));
    }
    if (ff.has(MonsterFeatureType::KILL_WALL)) {
        push_tag(tags, _("壁破壊", "KILL_WALL"));
    }
    if (tags.empty()) {
        return;
    }
    hooked_roff(_("[特性] ", "[Traits] "));

    for (auto &[msg, color] : lore_ptr->lore_msgs) {
        hook_c_roff(color, msg);
    }
    for (size_t i = 0; i < tags.size(); i++) {
        if (i > 0) {
            hooked_roff(" | ");
        }
        hooked_roff(tags[i]);
    }
    hooked_roff("\n");
    lore_ptr->lore_msgs.clear();
}

namespace {
struct ResistGroupDef {
    tl::optional<MonsterResistanceType> weak;
    tl::optional<MonsterResistanceType> resist;
    tl::optional<MonsterResistanceType> immune;

    const char *jp_weak;
    const char *jp_resist;
    const char *jp_immune;

    const char *en_weak;
    const char *en_resist;
    const char *en_immune;

    byte color_weak;
    byte color_resist;
    byte color_immune;
};

static void roff_cell(byte color, std::string_view s, int width)
{
    std::string out = format("%-*s", width, s.data());
    hook_c_roff(color, out);
}

static constexpr byte RES_UNKNOWN_COLOR = TERM_SLATE; // ??? の灰色
static constexpr byte RES_ABSENT_COLOR = TERM_SLATE; // --- の灰色

enum class GroupStateKind { WEAK,
    RESIST,
    IMMUNE,
    ABSENT,
    UNKNOWN };

struct GroupState {
    GroupStateKind kind;
    std::string_view label; // 出す文字列（JP or EN）
    byte color;
};

static GroupState get_group_state(const lore_type *lore_ptr, const ResistGroupDef &d)
{
    const bool fully_known = lore_ptr->know_everything || lore_ptr->is_details_known();
    const auto &flags = fully_known ? lore_ptr->r_ptr->resistance_flags : lore_ptr->r_ptr->r_resistance_flags;

    if (d.immune) {
        const auto no_teleport = d.immune.value() == MonsterResistanceType::RESIST_TELEPORT && lore_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
        const auto other_flag = d.immune.value() != MonsterResistanceType::RESIST_TELEPORT;
        const auto no_inst_death = d.immune.value() == MonsterResistanceType::NO_INSTANTLY_DEATH && lore_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE);
        if ((no_teleport || other_flag) && (flags.has(d.immune.value()) || no_inst_death)) {
            return { GroupStateKind::IMMUNE, _(d.jp_immune, d.en_immune), d.color_immune };
        }
    }

    if (d.resist) {
        const auto res_teleport = d.resist.value() == MonsterResistanceType::RESIST_TELEPORT && lore_ptr->r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE);
        const auto other_flag = d.resist.value() != MonsterResistanceType::RESIST_TELEPORT;
        if ((res_teleport || other_flag) && flags.has(d.resist.value())) {
            return { GroupStateKind::RESIST, _(d.jp_resist, d.en_resist), d.color_resist };
        }
    }

    if (d.weak) {
        if (flags.has(d.weak.value())) {
            return { GroupStateKind::WEAK, _(d.jp_weak, d.en_weak), d.color_weak };
        }
    }

    if (fully_known) {
        return { GroupStateKind::ABSENT, "------", RES_ABSENT_COLOR };
    }

    return { GroupStateKind::UNKNOWN, "??????", RES_UNKNOWN_COLOR };
}

static const std::vector<ResistGroupDef> &resist_group_defs()
{
    static const std::vector<ResistGroupDef> defs = {
        // 酸
        { MonsterResistanceType::HURT_ACID, MonsterResistanceType::RESIST_ACID, MonsterResistanceType::IMMUNE_ACID,
            "酸弱点", "酸耐性", "酸免疫", "HURT_ACID", "RESIST_ACID", "IMMUNE_ACID",
            TERM_L_RED, TERM_GREEN, TERM_GREEN },

        // 電撃
        { MonsterResistanceType::HURT_ELEC, MonsterResistanceType::RESIST_ELEC, MonsterResistanceType::IMMUNE_ELEC,
            "電弱点", "電耐性", "電免疫", "HURT_ELEC", "RESIST_ELEC", "IMMUNE_ELEC",
            TERM_L_RED, TERM_BLUE, TERM_BLUE },

        // 火炎
        { MonsterResistanceType::HURT_FIRE, MonsterResistanceType::RESIST_FIRE, MonsterResistanceType::IMMUNE_FIRE,
            "火弱点", "火耐性", "火免疫", "HURT_FIRE", "RESIST_FIRE", "IMMUNE_FIRE",
            TERM_L_RED, TERM_RED, TERM_RED },

        // 冷気
        { MonsterResistanceType::HURT_COLD, MonsterResistanceType::RESIST_COLD, MonsterResistanceType::IMMUNE_COLD,
            "冷弱点", "冷耐性", "冷免疫", "HURT_COLD", "RESIST_COLD", "IMMUNE_COLD",
            TERM_L_RED, TERM_L_WHITE, TERM_L_WHITE },

        // 毒
        { MonsterResistanceType::HURT_POISON, MonsterResistanceType::RESIST_POISON, MonsterResistanceType::IMMUNE_POISON,
            "毒弱点", "毒耐性", "毒免疫", "HURT_POISON", "RESIST_POISON", "IMMUNE_POISON",
            TERM_L_RED, TERM_L_GREEN, TERM_L_GREEN },

        // 閃光
        { MonsterResistanceType::HURT_LITE, MonsterResistanceType::RESIST_LITE, tl::nullopt,
            "閃光弱点", "閃光耐性", "閃光免疫", "HURT_LITE", "RESIST_LITE", "IMMUNE_LITE",
            TERM_L_RED, TERM_YELLOW, TERM_YELLOW },

        // 暗黒
        { MonsterResistanceType::HURT_DARK, MonsterResistanceType::RESIST_DARK, tl::nullopt,
            "暗黒弱点", "暗黒耐性", "暗黒免疫", "HURT_DARK", "RESIST_DARK", "IMMUNE_DARK",
            TERM_L_RED, TERM_L_DARK, TERM_L_DARK },

        // 地獄
        { MonsterResistanceType::HURT_NETHER, MonsterResistanceType::RESIST_NETHER, tl::nullopt,
            "地獄弱点", "地獄耐性", "地獄免疫", "HURT_NETHER", "RESIST_NETHER", "IMMUNE_NETHER",
            TERM_L_RED, TERM_L_DARK, TERM_L_DARK },

        // 水
        { MonsterResistanceType::HURT_WATER, MonsterResistanceType::RESIST_WATER, tl::nullopt,
            "水弱点", "水耐性", "水免疫", "HURT_WATER", "RESIST_WATER", "IMMUNE_WATER",
            TERM_L_RED, TERM_BLUE, TERM_BLUE },

        // プラズマ
        { MonsterResistanceType::HURT_PLASMA, MonsterResistanceType::RESIST_PLASMA, tl::nullopt,
            "プラズマ弱点", "プラズマ耐性", "プラズマ免疫", "HURT_PLASMA", "RESIST_PLASMA", "IMMUNE_PLASMA",
            TERM_L_RED, TERM_L_RED, TERM_L_RED },

        // 破片
        { MonsterResistanceType::HURT_SHARDS, MonsterResistanceType::RESIST_SHARDS, tl::nullopt,
            "破片弱点", "破片耐性", "破片免疫", "HURT_SHARDS", "RESIST_SHARDS", "IMMUNE_SHARDS",
            TERM_L_RED, TERM_L_UMBER, TERM_L_UMBER },

        // 轟音
        { MonsterResistanceType::HURT_SOUND, MonsterResistanceType::RESIST_SOUND, tl::nullopt,
            "轟音弱点", "轟音耐性", "轟音免疫", "HURT_SOUND", "RESIST_SOUND", "IMMUNE_SOUND",
            TERM_L_RED, TERM_ORANGE, TERM_ORANGE },

        // カオス
        { MonsterResistanceType::HURT_CHAOS, MonsterResistanceType::RESIST_CHAOS, tl::nullopt,
            "混沌弱点", "混沌耐性", "混沌免疫", "HURT_CHAOS", "RESIST_CHAOS", "IMMUNE_CHAOS",
            TERM_L_RED, TERM_VIOLET, TERM_VIOLET },

        // 因果
        { MonsterResistanceType::HURT_NEXUS, MonsterResistanceType::RESIST_NEXUS, tl::nullopt,
            "因混弱点", "因混耐性", "因混免疫", "HURT_NEXUS", "RESIST_NEXUS", "IMMUNE_NEXUS",
            TERM_L_RED, TERM_VIOLET, TERM_VIOLET },

        // 劣化
        { MonsterResistanceType::HURT_DISENCHANT, MonsterResistanceType::RESIST_DISENCHANT, tl::nullopt,
            "劣化弱点", "劣化耐性", "劣化免疫", "HURT_DISENCHANT", "RESIST_DISENCHANT", "IMMUNE_DISENCHANT",
            TERM_L_RED, TERM_VIOLET, TERM_VIOLET },

        // フォース
        { MonsterResistanceType::HURT_FORCE, MonsterResistanceType::RESIST_FORCE, tl::nullopt,
            "フォース弱点", "フォース耐性", "フォース免疫", "HURT_FORCE", "RESIST_FORCE", "IMMUNE_FORCE",
            TERM_L_RED, TERM_UMBER, TERM_UMBER },

        // 遅鈍（INERTIA）
        { MonsterResistanceType::HURT_INERTIA, MonsterResistanceType::RESIST_INERTIA, tl::nullopt,
            "遅鈍弱点", "遅鈍耐性", "遅鈍免疫", "HURT_INERTIA", "RESIST_INERTIA", "IMMUNE_INERTIA",
            TERM_L_RED, TERM_SLATE, TERM_SLATE },

        // 時間
        { MonsterResistanceType::HURT_TIME, MonsterResistanceType::RESIST_TIME, tl::nullopt,
            "時間逆転弱点", "時間逆転耐性", "時間逆転免疫", "HURT_TIME", "RESIST_TIME", "IMMUNE_TIME",
            TERM_L_RED, TERM_L_BLUE, TERM_L_BLUE },

        // 重力
        { MonsterResistanceType::HURT_GRAVITY, MonsterResistanceType::RESIST_GRAVITY, tl::nullopt,
            "重力弱点", "重力耐性", "重力免疫", "HURT_GRAVITY", "RESIST_GRAVITY", "IMMUNE_GRAVITY",
            TERM_L_RED, TERM_SLATE, TERM_SLATE },

        // 岩
        { MonsterResistanceType::HURT_ROCK, MonsterResistanceType::RESIST_ROCK, tl::nullopt,
            "岩石溶解弱点", "岩石溶解耐性", "岩石溶解免疫", "HURT_ROCK", "RESIST_ROCK", "IMMUNE_ROCK",
            TERM_L_RED, TERM_UMBER, TERM_UMBER },

        // 深淵
        { MonsterResistanceType::HURT_ABYSS, MonsterResistanceType::RESIST_ABYSS, tl::nullopt,
            "深淵弱点", "深淵耐性", "深淵免疫", "HURT_ABYSS", "RESIST_ABYSS", "IMMUNE_ABYSS",
            TERM_L_RED, TERM_L_DARK, TERM_L_DARK },

        // 虚無魔法
        { MonsterResistanceType::HURT_VOID_MAGIC, MonsterResistanceType::RESIST_VOID_MAGIC, tl::nullopt,
            "虚無弱点", "虚無耐性", "虚無免疫", "HURT_VOID_MAGIC", "RESIST_VOID_MAGIC", "IMMUNE_VOID_MAGIC",
            TERM_L_RED, TERM_VIOLET, TERM_VIOLET },

        // 隕石
        { MonsterResistanceType::HURT_METEOR, MonsterResistanceType::RESIST_METEOR, tl::nullopt,
            "隕石弱点", "隕石耐性", "隕石石免", "HURT_METEOR", "RESIST_METEOR", "IMMUNE_METEOR",
            TERM_L_RED, TERM_UMBER, TERM_UMBER },

        // 恐怖
        { tl::nullopt, tl::nullopt, MonsterResistanceType::NO_FEAR,
            "---", "---", "恐怖無効", "---", "---", "NO_FEAR",
            TERM_SLATE, TERM_SLATE, TERM_SLATE },

        // 朦朧
        { tl::nullopt, tl::nullopt, MonsterResistanceType::NO_STUN,
            "---", "---", "朦朧無効", "---", "---", "NO_STUN",
            TERM_ORANGE, TERM_ORANGE, TERM_ORANGE },

        // 混乱
        { tl::nullopt, tl::nullopt, MonsterResistanceType::NO_CONF,
            "---", "---", "混乱無効", "---", "---", "NO_CONF",
            TERM_L_UMBER, TERM_L_UMBER, TERM_L_UMBER },

        // 睡眠
        { tl::nullopt, tl::nullopt, MonsterResistanceType::NO_SLEEP,
            "---", "---", "睡眠無効", "---", "---", "NO_SLEEP",
            TERM_BLUE, TERM_BLUE, TERM_BLUE },

        // テレポート耐性
        { tl::nullopt, MonsterResistanceType::RESIST_TELEPORT, MonsterResistanceType::RESIST_TELEPORT,
            "---", "テレポート耐性", "テレポート無効", "---", "RESIST_TELEPORT", "NO_TELEPORT",
            TERM_ORANGE, TERM_ORANGE, TERM_ORANGE },

        // 即死無効
        { tl::nullopt, tl::nullopt, MonsterResistanceType::NO_INSTANTLY_DEATH,
            "---", "---", "即死無効", "---", "---", "NO_INST_DEATH",
            TERM_L_DARK, TERM_L_DARK, TERM_L_DARK },

        // 全攻撃無効
        { tl::nullopt, tl::nullopt, MonsterResistanceType::RESIST_ALL,
            "---", "---", "全攻撃無効", "---", "---", "RESIST_ALL",
            TERM_YELLOW, TERM_YELLOW, TERM_YELLOW },
    };
    return defs;
}
} // namespace

void display_monster_resistance_table(lore_type *lore_ptr)
{
    const auto see_reflecting = lore_ptr->r_ptr->r_misc_flags.has(MonsterMiscType::REFLECTING);
    const bool fully_known = lore_ptr->know_everything || lore_ptr->is_details_known();

    if (!fully_known && lore_ptr->r_ptr->r_resistance_flags.none() && !see_reflecting) {
        return;
    }

    const int COL_NUM = _(8, 5);
    const int CELL_WIDTH = _(6, 14);

    hooked_roff(_("[耐性] ", "[Resists] "));

    if (fully_known) {
        if (lore_ptr->r_ptr->misc_flags.has(MonsterMiscType::REFLECTING)) {
            hook_c_roff(TERM_L_WHITE, _("反射 ", "REFLECTING "));
        } else {
            hook_c_roff(RES_ABSENT_COLOR, "------ ");
        }
    } else {
        if (see_reflecting) {
            hook_c_roff(TERM_L_WHITE, _("反射 ", "REFLECTING "));
        } else {
            hook_c_roff(RES_UNKNOWN_COLOR, "?????? ");
        }
    }

    int col = 0;
    for (const auto &d : resist_group_defs()) {
        if (col != 0) {
            hooked_roff(" ");
        }

        const auto st = get_group_state(lore_ptr, d);
        roff_cell(st.color, st.label, CELL_WIDTH);

        col++;
        if (col >= COL_NUM) {
            hooked_roff("\n");
            col = 0;
        }
    }

    if (col != 0) {
        hooked_roff("\n");
    }
}
