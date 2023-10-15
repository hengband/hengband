#include "view/display-lore-status.h"
#include "locale/japanese.h"
#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-brightness-flags.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "term/term-color-types.h"

void display_monster_hp_ac(lore_type *lore_ptr)
{
    if (!know_armour(lore_ptr->r_idx, lore_ptr->know_everything)) {
        return;
    }

    hooked_roff(format(_("%s^は AC%d の防御力と", "%s^ has an armor rating of %d"), Who::who(lore_ptr->msex), lore_ptr->r_ptr->ac));
    if ((lore_ptr->flags1 & RF1_FORCE_MAXHP) || (lore_ptr->r_ptr->hside == 1)) {
        auto hp = lore_ptr->r_ptr->hdice * (lore_ptr->nightmare ? 2 : 1) * lore_ptr->r_ptr->hside;
        hooked_roff(format(_(" %d の体力がある。", " and a life rating of %d.  "), std::min(MONSTER_MAXHP, hp)));
    } else {
        hooked_roff(format(
            _(" %dd%d の体力がある。", " and a life rating of %dd%d.  "), lore_ptr->r_ptr->hdice * (lore_ptr->nightmare ? 2 : 1), lore_ptr->r_ptr->hside));
    }
}

void display_monster_concrete_abilities(lore_type *lore_ptr)
{
    if (lore_ptr->brightness_flags.has_any_of({ MonsterBrightnessType::HAS_LITE_1, MonsterBrightnessType::HAS_LITE_2 })) {
        lore_ptr->vp[lore_ptr->vn] = _("ダンジョンを照らす", "illuminate the dungeon");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->brightness_flags.has_any_of({ MonsterBrightnessType::HAS_DARK_1, MonsterBrightnessType::HAS_DARK_2 })) {
        lore_ptr->vp[lore_ptr->vn] = _("ダンジョンを暗くする", "darken the dungeon");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_DARK;
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::OPEN_DOOR)) {
        lore_ptr->vp[lore_ptr->vn] = _("ドアを開ける", "open doors");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::BASH_DOOR)) {
        lore_ptr->vp[lore_ptr->vn] = _("ドアを打ち破る", "bash down doors");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->r_ptr->feature_flags.has(MonsterFeatureType::CAN_FLY)) {
        lore_ptr->vp[lore_ptr->vn] = _("空を飛ぶ", "fly");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->r_ptr->feature_flags.has(MonsterFeatureType::CAN_SWIM)) {
        lore_ptr->vp[lore_ptr->vn] = _("水を渡る", "swim");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->feature_flags.has(MonsterFeatureType::PASS_WALL)) {
        lore_ptr->vp[lore_ptr->vn] = _("壁をすり抜ける", "pass through walls");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->feature_flags.has(MonsterFeatureType::KILL_WALL)) {
        lore_ptr->vp[lore_ptr->vn] = _("壁を掘り進む", "bore through walls");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::MOVE_BODY)) {
        lore_ptr->vp[lore_ptr->vn] = _("弱いモンスターを押しのける", "push past weaker monsters");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::KILL_BODY)) {
        lore_ptr->vp[lore_ptr->vn] = _("弱いモンスターを倒す", "destroy weaker monsters");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::TAKE_ITEM)) {
        lore_ptr->vp[lore_ptr->vn] = _("アイテムを拾う", "pick up objects");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->behavior_flags.has(MonsterBehaviorType::KILL_ITEM)) {
        lore_ptr->vp[lore_ptr->vn] = _("アイテムを壊す", "destroy objects");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }
}

void display_monster_abilities(lore_type *lore_ptr)
{
    if (lore_ptr->vn <= 0) {
        return;
    }

    hooked_roff(format(_("%s^は", "%s^"), Who::who(lore_ptr->msex)));
    for (int n = 0; n < lore_ptr->vn; n++) {
#ifdef JP
        if (n != lore_ptr->vn - 1) {
            const auto verb = conjugate_jverb(lore_ptr->vp[n], JVerbConjugationType::AND);
            hook_c_roff(lore_ptr->color[n], verb);
            hooked_roff("、");
        } else {
            hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
        }
#else
        if (n == 0) {
            hooked_roff(" can ");
        } else if (n < lore_ptr->vn - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" and ");
        }

        hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
#endif
    }

    hooked_roff(_("ことができる。", ".  "));
}

void display_monster_constitutions(lore_type *lore_ptr)
{
    if (lore_ptr->r_ptr->feature_flags.has(MonsterFeatureType::AQUATIC)) {
        hooked_roff(format(_("%s^は水中に棲んでいる。", "%s^ lives in water.  "), Who::who(lore_ptr->msex)));
    }

    if (lore_ptr->brightness_flags.has_any_of({ MonsterBrightnessType::SELF_LITE_1, MonsterBrightnessType::SELF_LITE_2 })) {
        hooked_roff(format(_("%s^は光っている。", "%s^ is shining.  "), Who::who(lore_ptr->msex)));
    }

    if (lore_ptr->brightness_flags.has_any_of({ MonsterBrightnessType::SELF_DARK_1, MonsterBrightnessType::SELF_DARK_2 })) {
        hook_c_roff(TERM_L_DARK, format(_("%s^は暗黒に包まれている。", "%s^ is surrounded by darkness.  "), Who::who(lore_ptr->msex)));
    }

    if (lore_ptr->flags2 & RF2_INVISIBLE) {
        hooked_roff(format(_("%s^は透明で目に見えない。", "%s^ is invisible.  "), Who::who(lore_ptr->msex)));
    }

    if (lore_ptr->flags2 & RF2_COLD_BLOOD) {
        hooked_roff(format(_("%s^は冷血動物である。", "%s^ is cold blooded.  "), Who::who(lore_ptr->msex)));
    }

    if (lore_ptr->flags2 & RF2_EMPTY_MIND) {
        hooked_roff(format(_("%s^はテレパシーでは感知できない。", "%s^ is not detected by telepathy.  "), Who::who(lore_ptr->msex)));
    } else if (lore_ptr->flags2 & RF2_WEIRD_MIND) {
        hooked_roff(format(_("%s^はまれにテレパシーで感知できる。", "%s^ is rarely detected by telepathy.  "), Who::who(lore_ptr->msex)));
    }

    if (lore_ptr->flags2 & RF2_MULTIPLY) {
        hook_c_roff(TERM_L_UMBER, format(_("%s^は爆発的に増殖する。", "%s^ breeds explosively.  "), Who::who(lore_ptr->msex)));
    }

    if (lore_ptr->flags2 & RF2_REGENERATE) {
        hook_c_roff(TERM_L_WHITE, format(_("%s^は素早く体力を回復する。", "%s^ regenerates quickly.  "), Who::who(lore_ptr->msex)));
    }

    if (lore_ptr->flags7 & RF7_RIDING) {
        hook_c_roff(TERM_SLATE, format(_("%s^に乗ることができる。", "%s^ is suitable for riding.  "), Who::who(lore_ptr->msex)));
    }
}

void display_monster_concrete_weakness(lore_type *lore_ptr)
{
    if (lore_ptr->resistance_flags.has(MonsterResistanceType::HURT_ROCK)) {
        lore_ptr->vp[lore_ptr->vn] = _("岩を除去するもの", "rock remover");
        lore_ptr->color[lore_ptr->vn++] = TERM_UMBER;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::HURT_LITE)) {
        lore_ptr->vp[lore_ptr->vn] = _("明るい光", "bright light");
        lore_ptr->color[lore_ptr->vn++] = TERM_YELLOW;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::HURT_FIRE)) {
        lore_ptr->vp[lore_ptr->vn] = _("炎", "fire");
        lore_ptr->color[lore_ptr->vn++] = TERM_RED;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::HURT_COLD)) {
        lore_ptr->vp[lore_ptr->vn] = _("冷気", "cold");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_WHITE;
    }
}

void display_monster_weakness(lore_type *lore_ptr)
{
    if (lore_ptr->vn <= 0) {
        return;
    }

    hooked_roff(format(_("%s^には", "%s^"), Who::who(lore_ptr->msex)));
    for (int n = 0; n < lore_ptr->vn; n++) {
#ifdef JP
        if (n != 0) {
            hooked_roff("や");
        }
#else
        if (n == 0) {
            hooked_roff(" is hurt by ");
        } else if (n < lore_ptr->vn - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" and ");
        }
#endif
        hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
    }

    hooked_roff(_("でダメージを与えられる。", ".  "));
}

void display_monster_concrete_resistances(lore_type *lore_ptr)
{
    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_ACID)) {
        lore_ptr->vp[lore_ptr->vn] = _("酸", "acid");
        lore_ptr->color[lore_ptr->vn++] = TERM_GREEN;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_ELEC)) {
        lore_ptr->vp[lore_ptr->vn] = _("稲妻", "lightning");
        lore_ptr->color[lore_ptr->vn++] = TERM_BLUE;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_FIRE)) {
        lore_ptr->vp[lore_ptr->vn] = _("炎", "fire");
        lore_ptr->color[lore_ptr->vn++] = TERM_RED;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_COLD)) {
        lore_ptr->vp[lore_ptr->vn] = _("冷気", "cold");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_WHITE;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::IMMUNE_POISON)) {
        lore_ptr->vp[lore_ptr->vn] = _("毒", "poison");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_GREEN;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_LITE)) {
        lore_ptr->vp[lore_ptr->vn] = _("閃光", "light");
        lore_ptr->color[lore_ptr->vn++] = TERM_YELLOW;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_DARK)) {
        lore_ptr->vp[lore_ptr->vn] = _("暗黒", "dark");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_DARK;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_NETHER)) {
        lore_ptr->vp[lore_ptr->vn] = _("地獄", "nether");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_DARK;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_WATER)) {
        lore_ptr->vp[lore_ptr->vn] = _("水", "water");
        lore_ptr->color[lore_ptr->vn++] = TERM_BLUE;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_PLASMA)) {
        lore_ptr->vp[lore_ptr->vn] = _("プラズマ", "plasma");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_RED;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_SHARDS)) {
        lore_ptr->vp[lore_ptr->vn] = _("破片", "shards");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_UMBER;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_SOUND)) {
        lore_ptr->vp[lore_ptr->vn] = _("轟音", "sound");
        lore_ptr->color[lore_ptr->vn++] = TERM_ORANGE;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_CHAOS)) {
        lore_ptr->vp[lore_ptr->vn] = _("カオス", "chaos");
        lore_ptr->color[lore_ptr->vn++] = TERM_VIOLET;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_NEXUS)) {
        lore_ptr->vp[lore_ptr->vn] = _("因果混乱", "nexus");
        lore_ptr->color[lore_ptr->vn++] = TERM_VIOLET;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_DISENCHANT)) {
        lore_ptr->vp[lore_ptr->vn] = _("劣化", "disenchantment");
        lore_ptr->color[lore_ptr->vn++] = TERM_VIOLET;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_FORCE)) {
        lore_ptr->vp[lore_ptr->vn] = _("フォース", "force");
        lore_ptr->color[lore_ptr->vn++] = TERM_UMBER;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_INERTIA)) {
        lore_ptr->vp[lore_ptr->vn] = _("遅鈍", "inertia");
        lore_ptr->color[lore_ptr->vn++] = TERM_SLATE;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_TIME)) {
        lore_ptr->vp[lore_ptr->vn] = _("時間逆転", "time");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_BLUE;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_GRAVITY)) {
        lore_ptr->vp[lore_ptr->vn] = _("重力", "gravity");
        lore_ptr->color[lore_ptr->vn++] = TERM_SLATE;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_METEOR)) {
        lore_ptr->vp[lore_ptr->vn] = _("隕石", "meteor");
        lore_ptr->color[lore_ptr->vn++] = TERM_UMBER;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_ALL)) {
        lore_ptr->vp[lore_ptr->vn] = _("あらゆる攻撃", "all");
        lore_ptr->color[lore_ptr->vn++] = TERM_YELLOW;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT) && lore_ptr->r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        lore_ptr->vp[lore_ptr->vn] = _("テレポート", "teleportation");
        lore_ptr->color[lore_ptr->vn++] = TERM_ORANGE;
    }
}

void display_monster_resistances(lore_type *lore_ptr)
{
    if (lore_ptr->vn <= 0) {
        return;
    }

    hooked_roff(format(_("%s^は", "%s^"), Who::who(lore_ptr->msex)));
    for (int n = 0; n < lore_ptr->vn; n++) {
#ifdef JP
        if (n != 0) {
            hooked_roff("と");
        }
#else
        if (n == 0) {
            hooked_roff(" resists ");
        } else if (n < lore_ptr->vn - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" and ");
        }
#endif
        hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
    }

    hooked_roff(_("の耐性を持っている。", ".  "));
}

void display_monster_evolution(lore_type *lore_ptr)
{
    if (!lore_ptr->r_ptr->r_can_evolve && !lore_ptr->know_everything) {
        return;
    }

    if (MonsterRace(lore_ptr->r_ptr->next_r_idx).is_valid()) {
        hooked_roff(format(_("%s^は経験を積むと、", "%s^ will evolve into "), Who::who(lore_ptr->msex)));
        hook_c_roff(TERM_YELLOW, format("%s", monraces_info[lore_ptr->r_ptr->next_r_idx].name.data()));

        hooked_roff(_(format("に進化する。"), format(" when %s gets enough experience.  ", Who::who(lore_ptr->msex))));
    } else if (lore_ptr->r_ptr->kind_flags.has_not(MonsterKindType::UNIQUE)) {
        hooked_roff(format(_("%sは進化しない。", "%s won't evolve.  "), Who::who(lore_ptr->msex)));
    }
}

void display_monster_concrete_immunities(lore_type *lore_ptr)
{
    if (lore_ptr->flags3 & RF3_NO_STUN) {
        lore_ptr->vp[lore_ptr->vn] = _("朦朧としない", "stunned");
        lore_ptr->color[lore_ptr->vn++] = TERM_ORANGE;
    }

    if (lore_ptr->flags3 & RF3_NO_FEAR) {
        lore_ptr->vp[lore_ptr->vn] = _("恐怖を感じない", "frightened");
        lore_ptr->color[lore_ptr->vn++] = TERM_SLATE;
    }

    if (lore_ptr->flags3 & RF3_NO_CONF) {
        lore_ptr->vp[lore_ptr->vn] = _("混乱しない", "confused");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_UMBER;
    }

    if (lore_ptr->flags3 & RF3_NO_SLEEP) {
        lore_ptr->vp[lore_ptr->vn] = _("眠らされない", "slept");
        lore_ptr->color[lore_ptr->vn++] = TERM_BLUE;
    }

    if (lore_ptr->resistance_flags.has(MonsterResistanceType::RESIST_TELEPORT) && lore_ptr->r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
        lore_ptr->vp[lore_ptr->vn] = _("テレポートされない", "teleported");
        lore_ptr->color[lore_ptr->vn++] = TERM_ORANGE;
    }
}

void display_monster_immunities(lore_type *lore_ptr)
{
    if (lore_ptr->vn <= 0) {
        return;
    }

    hooked_roff(format(_("%s^は", "%s^"), Who::who(lore_ptr->msex)));
    for (int n = 0; n < lore_ptr->vn; n++) {
#ifdef JP
        if (n != 0) {
            hooked_roff("し、");
        }
#else
        if (n == 0) {
            hooked_roff(" cannot be ");
        } else if (n < lore_ptr->vn - 1) {
            hooked_roff(", ");
        } else {
            hooked_roff(" or ");
        }
#endif
        hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
    }

    hooked_roff(_("。", ".  "));
}

void display_monster_alert(lore_type *lore_ptr)
{
    bool alert = ((int)lore_ptr->r_ptr->r_wake * (int)lore_ptr->r_ptr->r_wake) > lore_ptr->r_ptr->sleep;
    alert |= lore_ptr->r_ptr->r_ignore == MAX_UCHAR;
    alert |= (lore_ptr->r_ptr->sleep == 0) && (lore_ptr->r_ptr->r_tkills >= 10);
    alert |= lore_ptr->know_everything;
    if (!alert) {
        return;
    }

    concptr act;
    if (lore_ptr->r_ptr->sleep > 200) {
        act = _("を無視しがちであるが", "prefers to ignore");
    } else if (lore_ptr->r_ptr->sleep > 95) {
        act = _("に対してほとんど注意を払わないが", "pays very little attention to");
    } else if (lore_ptr->r_ptr->sleep > 75) {
        act = _("に対してあまり注意を払わないが", "pays little attention to");
    } else if (lore_ptr->r_ptr->sleep > 45) {
        act = _("を見過ごしがちであるが", "tends to overlook");
    } else if (lore_ptr->r_ptr->sleep > 25) {
        act = _("をほんの少しは見ており", "takes quite a while to see");
    } else if (lore_ptr->r_ptr->sleep > 10) {
        act = _("をしばらくは見ており", "takes a while to see");
    } else if (lore_ptr->r_ptr->sleep > 5) {
        act = _("を幾分注意深く見ており", "is fairly observant of");
    } else if (lore_ptr->r_ptr->sleep > 3) {
        act = _("を注意深く見ており", "is observant of");
    } else if (lore_ptr->r_ptr->sleep > 1) {
        act = _("をかなり注意深く見ており", "is very observant of");
    } else if (lore_ptr->r_ptr->sleep > 0) {
        act = _("を警戒しており", "is vigilant for");
    } else {
        act = _("をかなり警戒しており", "is ever vigilant for");
    }

    hooked_roff(_(format("%s^は侵入者%s、 %d フィート先から侵入者に気付くことがある。", Who::who(lore_ptr->msex), act, 10 * lore_ptr->r_ptr->aaf),
        format("%s^ %s intruders, which %s may notice from %d feet.  ", Who::who(lore_ptr->msex), act, Who::who(lore_ptr->msex), 10 * lore_ptr->r_ptr->aaf)));
}
