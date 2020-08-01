#include "status/shape-changer.h"
#include "autopick/autopick-reader-writer.h"
#include "birth/birth-body-spec.h"
#include "birth/birth-stat.h"
#include "core/disturbance.h"
#include "core/hp-mp-processor.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "mutation/mutation-investor-remover.h"
#include "player/avatar.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-personality.h"
#include "player/player-sex.h"
#include "player/race-info-table.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "view/display-messages.h"
#ifdef JP
#else
#include "locale/english.h"
#endif

void do_poly_wounds(player_type *creature_ptr)
{
    s16b wounds = creature_ptr->cut;
    s16b hit_p = (creature_ptr->mhp - creature_ptr->chp);
    s16b change = damroll(creature_ptr->lev, 5);
    bool Nasty_effect = one_in_(5);
    if (!(wounds || hit_p || Nasty_effect))
        return;

    msg_print(_("傷がより軽いものに変化した。", "Your wounds are polymorphed into less serious ones."));
    hp_player(creature_ptr, change);
    if (Nasty_effect) {
        msg_print(_("新たな傷ができた！", "A new wound was created!"));
        take_hit(creature_ptr, DAMAGE_LOSELIFE, change / 2, _("変化した傷", "a polymorphed wound"), -1);
        set_cut(creature_ptr, change);
    } else {
        set_cut(creature_ptr, creature_ptr->cut - (change / 2));
    }
}

/*
 * Change player race
 */
void change_race(player_type *creature_ptr, player_race_type new_race, concptr effect_msg)
{
    concptr title = race_info[new_race].title;
    int old_race = creature_ptr->prace;
#ifdef JP
    msg_format("あなたは%s%sに変化した！", effect_msg, title);
#else
    msg_format("You turn into %s %s%s!", (!effect_msg[0] && is_a_vowel(title[0]) ? "an" : "a"), effect_msg, title);
#endif

    chg_virtue(creature_ptr, V_CHANCE, 2);
    if (creature_ptr->prace < 32) {
        creature_ptr->old_race1 |= 1L << creature_ptr->prace;
    } else {
        creature_ptr->old_race2 |= 1L << (creature_ptr->prace - 32);
    }

    creature_ptr->prace = new_race;
    rp_ptr = &race_info[creature_ptr->prace];
    creature_ptr->expfact = rp_ptr->r_exp + cp_ptr->c_exp;

    bool is_special_class = creature_ptr->pclass == CLASS_MONK;
    is_special_class |= creature_ptr->pclass == CLASS_FORCETRAINER;
    is_special_class |= creature_ptr->pclass == CLASS_NINJA;
    bool is_special_race = creature_ptr->prace == RACE_KLACKON;
    is_special_race |= creature_ptr->prace == RACE_SPRITE;
    if (is_special_class && is_special_race)
        creature_ptr->expfact -= 15;

    get_height_weight(creature_ptr);

    if (creature_ptr->pclass == CLASS_SORCERER)
        creature_ptr->hitdie = rp_ptr->r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
    else
        creature_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;

    roll_hitdice(creature_ptr, 0L);
    check_experience(creature_ptr);
    creature_ptr->redraw |= (PR_BASIC);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);

    if (old_race != creature_ptr->prace)
        autopick_load_pref(creature_ptr, FALSE);

    lite_spot(creature_ptr, creature_ptr->y, creature_ptr->x);
}

void do_poly_self(player_type *creature_ptr)
{
    int power = creature_ptr->lev;

    msg_print(_("あなたは変化の訪れを感じた...", "You feel a change coming over you..."));
    chg_virtue(creature_ptr, V_CHANCE, 1);

    if ((power > randint0(20)) && one_in_(3) && (creature_ptr->prace != RACE_ANDROID)) {
        char effect_msg[80] = "";
        player_race_type new_race;

        power -= 10;
        if ((power > randint0(5)) && one_in_(4)) {
            power -= 2;
            if (creature_ptr->psex == SEX_MALE) {
                creature_ptr->psex = SEX_FEMALE;
                sp_ptr = &sex_info[creature_ptr->psex];
                sprintf(effect_msg, _("女性の", "female "));
            } else {
                creature_ptr->psex = SEX_MALE;
                sp_ptr = &sex_info[creature_ptr->psex];
                sprintf(effect_msg, _("男性の", "male "));
            }
        }

        if ((power > randint0(30)) && one_in_(5)) {
            int tmp = 0;
            power -= 15;
            while (tmp < A_MAX) {
                if (one_in_(2)) {
                    (void)dec_stat(creature_ptr, tmp, randint1(6) + 6, one_in_(3));
                    power -= 1;
                }
                tmp++;
            }

            (void)dec_stat(creature_ptr, A_CHR, randint1(6), TRUE);

            if (effect_msg[0]) {
                char tmp_msg[10];
                sprintf(tmp_msg, _("%s", "%s "), effect_msg);
                sprintf(effect_msg, _("奇形の%s", "deformed %s "), tmp_msg);
            } else {
                sprintf(effect_msg, _("奇形の", "deformed "));
            }
        }

        while ((power > randint0(20)) && one_in_(10)) {
            power -= 10;

            if (!lose_mutation(creature_ptr, 0))
                msg_print(_("奇妙なくらい普通になった気がする。", "You feel oddly normal."));
        }

        do {
            new_race = (player_race_type)randint0(MAX_RACES);
        } while ((new_race == creature_ptr->prace) || (new_race == RACE_ANDROID));

        change_race(creature_ptr, new_race, effect_msg);
    }

    if ((power > randint0(30)) && one_in_(6)) {
        int tmp = 0;
        power -= 20;
        msg_format(_("%sの構成が変化した！", "Your internal organs are rearranged!"), creature_ptr->prace == RACE_ANDROID ? "機械" : "内臓");

        while (tmp < A_MAX) {
            (void)dec_stat(creature_ptr, tmp, randint1(6) + 6, one_in_(3));
            tmp++;
        }
        if (one_in_(6)) {
            msg_print(_("現在の姿で生きていくのは困難なようだ！", "You find living difficult in your present form!"));
            take_hit(creature_ptr, DAMAGE_LOSELIFE, damroll(randint1(10), creature_ptr->lev), _("致命的な突然変異", "a lethal mutation"), -1);

            power -= 10;
        }
    }

    if ((power > randint0(20)) && one_in_(4)) {
        power -= 10;

        get_max_stats(creature_ptr);
        roll_hitdice(creature_ptr, 0L);
    }

    while ((power > randint0(15)) && one_in_(3)) {
        power -= 7;
        (void)gain_mutation(creature_ptr, 0);
    }

    if (power > randint0(5)) {
        power -= 5;
        do_poly_wounds(creature_ptr);
    }

    while (power > 0) {
        status_shuffle(creature_ptr);
        power--;
    }
}

/*!
 * @brief 変身効果の継続時間と変身先をセットする / Set "tim_mimic", and "mimic_form", notice observable changes
 * @param v 継続時間
 * @param p 変身内容
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_mimic(player_type *creature_ptr, TIME_EFFECT v, MIMIC_RACE_IDX p, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_mimic && (creature_ptr->mimic_form == p) && !do_dec) {
            if (creature_ptr->tim_mimic > v)
                return FALSE;
        } else if ((!creature_ptr->tim_mimic) || (creature_ptr->mimic_form != p)) {
            msg_print(_("自分の体が変わってゆくのを感じた。", "You feel that your body changes."));
            creature_ptr->mimic_form = p;
            notice = TRUE;
        }
    }

    else {
        if (creature_ptr->tim_mimic) {
            msg_print(_("変身が解けた。", "You are no longer transformed."));
            if (creature_ptr->mimic_form == MIMIC_DEMON)
                set_oppose_fire(creature_ptr, 0, TRUE);
            creature_ptr->mimic_form = 0;
            notice = TRUE;
            p = 0;
        }
    }

    creature_ptr->tim_mimic = v;
    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, TRUE);

    creature_ptr->redraw |= (PR_BASIC | PR_STATUS);
    creature_ptr->update |= (PU_BONUS | PU_HP);

    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 狂戦士化の継続時間をセットする / Set "shero", notice observable changes
 * @param v 継続時間/ 0ならば無条件にリセット
 * @param do_dec FALSEの場合現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_shero(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (creature_ptr->pclass == CLASS_BERSERKER)
        v = 1;
    if (v) {
        if (creature_ptr->shero && !do_dec) {
            if (creature_ptr->shero > v)
                return FALSE;
        } else if (!creature_ptr->shero) {
            msg_print(_("殺戮マシーンになった気がする！", "You feel like a killing machine!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->shero) {
            msg_print(_("野蛮な気持ちが消え失せた。", "You feel less berserk."));
            notice = TRUE;
        }
    }

    creature_ptr->shero = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->update |= (PU_HP);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 幽体化の継続時間をセットする / Set "wraith_form", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_wraith_form(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->wraith_form && !do_dec) {
            if (creature_ptr->wraith_form > v)
                return FALSE;
        } else if (!creature_ptr->wraith_form) {
            msg_print(_("物質界を離れて幽鬼のような存在になった！", "You leave the physical world and turn into a wraith-being!"));
            notice = TRUE;
            chg_virtue(creature_ptr, V_UNLIFE, 3);
            chg_virtue(creature_ptr, V_HONOUR, -2);
            chg_virtue(creature_ptr, V_SACRIFICE, -2);
            chg_virtue(creature_ptr, V_VALOUR, -5);

            creature_ptr->redraw |= (PR_MAP);
            creature_ptr->update |= (PU_MONSTERS);

            creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
        }
    } else {
        if (creature_ptr->wraith_form) {
            msg_print(_("不透明になった感じがする。", "You feel opaque."));
            notice = TRUE;

            creature_ptr->redraw |= (PR_MAP);
            creature_ptr->update |= (PU_MONSTERS);

            creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
        }
    }

    creature_ptr->wraith_form = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief オクレ兄さんの継続時間をセットする / Set "tsuyoshi", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tsuyoshi(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tsuyoshi && !do_dec) {
            if (creature_ptr->tsuyoshi > v)
                return FALSE;
        } else if (!creature_ptr->tsuyoshi) {
            msg_print(_("「オクレ兄さん！」", "Brother OKURE!"));
            notice = TRUE;
            chg_virtue(creature_ptr, V_VITALITY, 2);
        }
    } else {
        if (creature_ptr->tsuyoshi) {
            msg_print(_("肉体が急速にしぼんでいった。", "Your body has quickly shriveled."));

            (void)dec_stat(creature_ptr, A_CON, 20, TRUE);
            (void)dec_stat(creature_ptr, A_STR, 20, TRUE);

            notice = TRUE;
            chg_virtue(creature_ptr, V_VITALITY, -3);
        }
    }

    creature_ptr->tsuyoshi = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->update |= (PU_HP);
    handle_stuff(creature_ptr);
    return TRUE;
}
