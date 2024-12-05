#include "view/display-lore-status.h"
#include "locale/japanese.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "monster-race/race-brightness-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "term/term-color-types.h"

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
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_ELEC)) {
        lore_ptr->lore_msgs.emplace_back(_("稲妻", "lightning"), TERM_BLUE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_FIRE)) {
        lore_ptr->lore_msgs.emplace_back(_("炎", "fire"), TERM_RED);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_COLD)) {
        lore_ptr->lore_msgs.emplace_back(_("冷気", "cold"), TERM_L_WHITE);
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_POISON)) {
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
