#include "status/shape-changer.h"
#include "autopick/autopick-reader-writer.h"
#include "avatar/avatar.h"
#include "birth/birth-body-spec.h"
#include "birth/birth-stat.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "hpmp/hp-mp-processor.h"
#include "mutation/mutation-investor-remover.h"
#include "player-info/class-info.h"
#include "player/player-damage.h"
#include "player/player-personality.h"
#include "player/player-sex.h"
#include "player/player-status.h"
#include "player/race-info-table.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "view/display-messages.h"
#ifdef JP
#else
#include "locale/english.h"
#endif

void do_poly_wounds(player_type *player_ptr)
{
    auto wounds = player_ptr->cut;
    int16_t hit_p = (player_ptr->mhp - player_ptr->chp);
    int16_t change = damroll(player_ptr->lev, 5);
    auto nasty_effect = one_in_(5);
    if ((wounds == 0) && (hit_p == 0) && !nasty_effect)
        return;

    msg_print(_("傷がより軽いものに変化した。", "Your wounds are polymorphed into less serious ones."));
    hp_player(player_ptr, change);
    BadStatusSetter bss(player_ptr);
    if (!nasty_effect) {
        (void)bss.cut(player_ptr->cut - (change / 2));
        return;
    }

    msg_print(_("新たな傷ができた！", "A new wound was created!"));
    take_hit(player_ptr, DAMAGE_LOSELIFE, change / 2, _("変化した傷", "a polymorphed wound"));
    (void)bss.cut(change);
}

/*
 * Change player race
 */
void change_race(player_type *player_ptr, player_race_type new_race, concptr effect_msg)
{
    concptr title = race_info[enum2i(new_race)].title;
    player_race_type old_race = player_ptr->prace;
#ifdef JP
    msg_format("あなたは%s%sに変化した！", effect_msg, title);
#else
    msg_format("You turn into %s %s%s!", (is_a_vowel((effect_msg[0]) ? effect_msg[0] : title[0]) ? "an" : "a"), effect_msg, title);
#endif

    chg_virtue(player_ptr, V_CHANCE, 2);
    if (enum2i(player_ptr->prace) < 32) {
        player_ptr->old_race1 |= 1UL << enum2i(player_ptr->prace);
    } else {
        player_ptr->old_race2 |= 1UL << (enum2i(player_ptr->prace) - 32);
    }

    player_ptr->prace = new_race;
    rp_ptr = &race_info[enum2i(player_ptr->prace)];
    player_ptr->expfact = rp_ptr->r_exp + cp_ptr->c_exp;

    bool is_special_class = player_ptr->pclass == CLASS_MONK;
    is_special_class |= player_ptr->pclass == CLASS_FORCETRAINER;
    is_special_class |= player_ptr->pclass == CLASS_NINJA;
    bool is_special_race = player_ptr->prace == player_race_type::KLACKON;
    is_special_race |= player_ptr->prace == player_race_type::SPRITE;
    if (is_special_class && is_special_race)
        player_ptr->expfact -= 15;

    get_height_weight(player_ptr);

    if (player_ptr->pclass == CLASS_SORCERER)
        player_ptr->hitdie = rp_ptr->r_mhp / 2 + cp_ptr->c_mhp + ap_ptr->a_mhp;
    else
        player_ptr->hitdie = rp_ptr->r_mhp + cp_ptr->c_mhp + ap_ptr->a_mhp;

    roll_hitdice(player_ptr, SPOP_NONE);
    check_experience(player_ptr);
    player_ptr->redraw |= (PR_BASIC);
    player_ptr->update |= (PU_BONUS);
    handle_stuff(player_ptr);

    if (old_race != player_ptr->prace)
        autopick_load_pref(player_ptr, false);

    lite_spot(player_ptr, player_ptr->y, player_ptr->x);
}

void do_poly_self(player_type *player_ptr)
{
    int power = player_ptr->lev;

    msg_print(_("あなたは変化の訪れを感じた...", "You feel a change coming over you..."));
    chg_virtue(player_ptr, V_CHANCE, 1);

    if ((power > randint0(20)) && one_in_(3) && (player_ptr->prace != player_race_type::ANDROID)) {
        char effect_msg[80] = "";
        char sex_msg[32] = "";
        player_race_type new_race;

        power -= 10;
        if ((power > randint0(5)) && one_in_(4)) {
            power -= 2;
            if (player_ptr->psex == SEX_MALE) {
                player_ptr->psex = SEX_FEMALE;
                sp_ptr = &sex_info[player_ptr->psex];
                sprintf(sex_msg, _("女性の", "female"));
            } else {
                player_ptr->psex = SEX_MALE;
                sp_ptr = &sex_info[player_ptr->psex];
                sprintf(sex_msg, _("男性の", "male"));
            }
        }

        if ((power > randint0(30)) && one_in_(5)) {
            int tmp = 0;
            power -= 15;
            while (tmp < A_MAX) {
                if (one_in_(2)) {
                    (void)dec_stat(player_ptr, tmp, randint1(6) + 6, one_in_(3));
                    power -= 1;
                }
                tmp++;
            }

            (void)dec_stat(player_ptr, A_CHR, randint1(6), true);

            if (sex_msg[0]) {
                sprintf(effect_msg, _("奇形の%s", "deformed %s "), sex_msg);
            } else {
                sprintf(effect_msg, _("奇形の", "deformed "));
            }
        }

        while ((power > randint0(20)) && one_in_(10)) {
            power -= 10;

            if (!lose_mutation(player_ptr, 0))
                msg_print(_("奇妙なくらい普通になった気がする。", "You feel oddly normal."));
        }

        do {
            new_race = (player_race_type)randint0(MAX_RACES);
        } while ((new_race == player_ptr->prace) || (new_race == player_race_type::ANDROID));

        change_race(player_ptr, new_race, effect_msg);
    }

    if ((power > randint0(30)) && one_in_(6)) {
        int tmp = 0;
        power -= 20;
        msg_format(_("%sの構成が変化した！", "Your internal organs are rearranged!"), player_ptr->prace == player_race_type::ANDROID ? "機械" : "内臓");

        while (tmp < A_MAX) {
            (void)dec_stat(player_ptr, tmp, randint1(6) + 6, one_in_(3));
            tmp++;
        }
        if (one_in_(6)) {
            msg_print(_("現在の姿で生きていくのは困難なようだ！", "You find living difficult in your present form!"));
            take_hit(player_ptr, DAMAGE_LOSELIFE, damroll(randint1(10), player_ptr->lev), _("致命的な突然変異", "a lethal mutation"));

            power -= 10;
        }
    }

    if ((power > randint0(20)) && one_in_(4)) {
        power -= 10;

        get_max_stats(player_ptr);
        roll_hitdice(player_ptr, SPOP_NONE);
    }

    while ((power > randint0(15)) && one_in_(3)) {
        power -= 7;
        (void)gain_mutation(player_ptr, 0);
    }

    if (power > randint0(5)) {
        power -= 5;
        do_poly_wounds(player_ptr);
    }

    while (power > 0) {
        status_shuffle(player_ptr);
        power--;
    }
}
