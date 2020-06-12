#include "view/display-lore-status.h"
#include "locale/japanese.h"
#include "lore/lore-calculator.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "term/term-color-types.h"

void display_monster_hp_ac(lore_type *lore_ptr)
{
    if (!lore_ptr->know_everything && (know_armour(lore_ptr->r_idx) == 0))
        return;

    hooked_roff(format(_("%^sは AC%d の防御力と", "%^s has an armor rating of %d"), wd_he[lore_ptr->msex], lore_ptr->r_ptr->ac));
    if ((lore_ptr->flags1 & RF1_FORCE_MAXHP) || (lore_ptr->r_ptr->hside == 1)) {
        u32b hp = lore_ptr->r_ptr->hdice * (lore_ptr->nightmare ? 2 : 1) * lore_ptr->r_ptr->hside;
        hooked_roff(format(_(" %d の体力がある。", " and a life rating of %d.  "), (s16b)MIN(30000, hp)));
    } else {
        hooked_roff(format(
            _(" %dd%d の体力がある。", " and a life rating of %dd%d.  "), lore_ptr->r_ptr->hdice * (lore_ptr->nightmare ? 2 : 1), lore_ptr->r_ptr->hside));
    }
}

void display_monster_concrete_abilities(lore_type *lore_ptr)
{
    if (lore_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2)) {
        lore_ptr->vp[lore_ptr->vn] = _("ダンジョンを照らす", "illuminate the dungeon");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->flags7 & (RF7_HAS_DARK_1 | RF7_HAS_DARK_2)) {
        lore_ptr->vp[lore_ptr->vn] = _("ダンジョンを暗くする", "darken the dungeon");
        lore_ptr->color[lore_ptr->vn++] = TERM_L_DARK;
    }

    if (lore_ptr->flags2 & RF2_OPEN_DOOR) {
        lore_ptr->vp[lore_ptr->vn] = _("ドアを開ける", "open doors");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->flags2 & RF2_BASH_DOOR) {
        lore_ptr->vp[lore_ptr->vn] = _("ドアを打ち破る", "bash down doors");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->flags7 & RF7_CAN_FLY) {
        lore_ptr->vp[lore_ptr->vn] = _("空を飛ぶ", "fly");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->flags7 & RF7_CAN_SWIM) {
        lore_ptr->vp[lore_ptr->vn] = _("水を渡る", "swim");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->flags2 & RF2_PASS_WALL) {
        lore_ptr->vp[lore_ptr->vn] = _("壁をすり抜ける", "pass through walls");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->flags2 & RF2_KILL_WALL) {
        lore_ptr->vp[lore_ptr->vn] = _("壁を掘り進む", "bore through walls");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->flags2 & RF2_MOVE_BODY) {
        lore_ptr->vp[lore_ptr->vn] = _("弱いモンスターを押しのける", "push past weaker monsters");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->flags2 & RF2_KILL_BODY) {
        lore_ptr->vp[lore_ptr->vn] = _("弱いモンスターを倒す", "destroy weaker monsters");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->flags2 & RF2_TAKE_ITEM) {
        lore_ptr->vp[lore_ptr->vn] = _("アイテムを拾う", "pick up objects");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }

    if (lore_ptr->flags2 & RF2_KILL_ITEM) {
        lore_ptr->vp[lore_ptr->vn] = _("アイテムを壊す", "destroy objects");
        lore_ptr->color[lore_ptr->vn++] = TERM_WHITE;
    }
}

void display_monster_abilities(lore_type *lore_ptr)
{
    if (lore_ptr->vn <= 0)
        return;

    hooked_roff(format(_("%^sは", "%^s"), wd_he[lore_ptr->msex]));
    for (int n = 0; n < lore_ptr->vn; n++) {
#ifdef JP
        if (n != lore_ptr->vn - 1) {
            jverb(lore_ptr->vp[n], lore_ptr->jverb_buf, JVERB_AND);
            hook_c_roff(lore_ptr->color[n], lore_ptr->jverb_buf);
            hooked_roff("、");
        } else {
            hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
        }
#else
        if (n == 0)
            hooked_roff(" can ");
        else if (n < lore_ptr->vn - 1)
            hooked_roff(", ");
        else
            hooked_roff(" and ");

        hook_c_roff(lore_ptr->color[n], lore_ptr->vp[n]);
#endif
    }

    hooked_roff(_("ことができる。", ".  "));
}

void display_monster_constitutions(lore_type *lore_ptr)
{
    if (lore_ptr->flags7 & RF7_AQUATIC)
        hooked_roff(format(_("%^sは水中に棲んでいる。", "%^s lives in water.  "), wd_he[lore_ptr->msex]));

    if (lore_ptr->flags7 & (RF7_SELF_LITE_1 | RF7_SELF_LITE_2))
        hooked_roff(format(_("%^sは光っている。", "%^s is shining.  "), wd_he[lore_ptr->msex]));

    if (lore_ptr->flags7 & (RF7_SELF_DARK_1 | RF7_SELF_DARK_2))
        hook_c_roff(TERM_L_DARK, format(_("%^sは暗黒に包まれている。", "%^s is surrounded by darkness.  "), wd_he[lore_ptr->msex]));

    if (lore_ptr->flags2 & RF2_INVISIBLE)
        hooked_roff(format(_("%^sは透明で目に見えない。", "%^s is invisible.  "), wd_he[lore_ptr->msex]));

    if (lore_ptr->flags2 & RF2_COLD_BLOOD)
        hooked_roff(format(_("%^sは冷血動物である。", "%^s is cold blooded.  "), wd_he[lore_ptr->msex]));

    if (lore_ptr->flags2 & RF2_EMPTY_MIND)
        hooked_roff(format(_("%^sはテレパシーでは感知できない。", "%^s is not detected by telepathy.  "), wd_he[lore_ptr->msex]));
    else if (lore_ptr->flags2 & RF2_WEIRD_MIND)
        hooked_roff(format(_("%^sはまれにテレパシーで感知できる。", "%^s is rarely detected by telepathy.  "), wd_he[lore_ptr->msex]));

    if (lore_ptr->flags2 & RF2_MULTIPLY)
        hook_c_roff(TERM_L_UMBER, format(_("%^sは爆発的に増殖する。", "%^s breeds explosively.  "), wd_he[lore_ptr->msex]));

    if (lore_ptr->flags2 & RF2_REGENERATE)
        hook_c_roff(TERM_L_WHITE, format(_("%^sは素早く体力を回復する。", "%^s regenerates quickly.  "), wd_he[lore_ptr->msex]));

    if (lore_ptr->flags7 & RF7_RIDING)
        hook_c_roff(TERM_SLATE, format(_("%^sに乗ることができる。", "%^s is suitable for riding.  "), wd_he[lore_ptr->msex]));
}
