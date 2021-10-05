#include "player/patron.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-io/cmd-dump.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-slot-types.h"
#include "io/write-diary.h"
#include "mind/mind-chaos-warrior.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race-hook.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h"
#include "object-enchant/object-curse.h"
#include "object/object-kind-hook.h"
#include "player-info/class-info.h"
#include "player-info/equipment-info.h"
#include "player-info/race-types.h"
#include "player/player-damage.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-sight.h"
#include "spell/spell-types.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/base-status.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

#ifdef JP
#define N(JAPANESE, ENGLISH) JAPANESE, ENGLISH
#else
#define N(JAPANESE, ENGLISH) ENGLISH
#endif

const std::vector<Patron> patron_list = {

    Patron(N("スローター", "Slortar"),
        { REW_WRATH, REW_CURSE_WP, REW_CURSE_AR, REW_RUIN_ABL, REW_LOSE_ABL, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_POLY_WND, REW_POLY_SLF, REW_POLY_SLF,
            REW_POLY_SLF, REW_GAIN_ABL, REW_GAIN_ABL, REW_GAIN_EXP, REW_GOOD_OBJ, REW_CHAOS_WP, REW_GREA_OBJ, REW_AUGM_ABL, REW_AUGM_ABL },
        A_CON, p_ptr),

    Patron(N("マベロード", "Mabelode"),
        { REW_WRATH, REW_CURSE_WP, REW_CURSE_AR, REW_H_SUMMON, REW_SUMMON_M, REW_SUMMON_M, REW_IGNORE, REW_IGNORE, REW_POLY_WND, REW_POLY_WND, REW_POLY_SLF,
            REW_HEAL_FUL, REW_HEAL_FUL, REW_GAIN_ABL, REW_SER_UNDE, REW_CHAOS_WP, REW_GOOD_OBJ, REW_GOOD_OBJ, REW_GOOD_OBS, REW_GOOD_OBS },
        A_CON, p_ptr),

    Patron(N("チャードロス", "Chardros"),
        { REW_WRATH, REW_WRATH, REW_HURT_LOT, REW_PISS_OFF, REW_H_SUMMON, REW_SUMMON_M, REW_IGNORE, REW_IGNORE, REW_DESTRUCT, REW_SER_UNDE, REW_GENOCIDE,
            REW_MASS_GEN, REW_MASS_GEN, REW_DISPEL_C, REW_GOOD_OBJ, REW_CHAOS_WP, REW_GOOD_OBS, REW_GOOD_OBS, REW_AUGM_ABL, REW_AUGM_ABL },
        A_STR, p_ptr),

    Patron(N("ハイオンハーン", "Hionhurn"),
        { REW_WRATH, REW_WRATH, REW_CURSE_WP, REW_CURSE_AR, REW_RUIN_ABL, REW_IGNORE, REW_IGNORE, REW_SER_UNDE, REW_DESTRUCT, REW_GENOCIDE, REW_MASS_GEN,
            REW_MASS_GEN, REW_HEAL_FUL, REW_GAIN_ABL, REW_GAIN_ABL, REW_CHAOS_WP, REW_GOOD_OBS, REW_GOOD_OBS, REW_AUGM_ABL, REW_AUGM_ABL },
        A_STR, p_ptr),

    Patron(N("キシオムバーグ", "Xiombarg"),
        { REW_TY_CURSE, REW_TY_CURSE, REW_PISS_OFF, REW_RUIN_ABL, REW_LOSE_ABL, REW_IGNORE, REW_POLY_SLF, REW_POLY_SLF, REW_POLY_WND, REW_POLY_WND,
            REW_GENOCIDE, REW_DISPEL_C, REW_GOOD_OBJ, REW_GOOD_OBJ, REW_SER_MONS, REW_GAIN_ABL, REW_CHAOS_WP, REW_GAIN_EXP, REW_AUGM_ABL, REW_GOOD_OBS },
        A_STR, p_ptr),

    Patron(N("ピアレー", "Pyaray"),
        { REW_WRATH, REW_TY_CURSE, REW_PISS_OFF, REW_H_SUMMON, REW_H_SUMMON, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_POLY_WND, REW_POLY_SLF, REW_POLY_SLF,
            REW_SER_DEMO, REW_HEAL_FUL, REW_GAIN_ABL, REW_GAIN_ABL, REW_CHAOS_WP, REW_DO_HAVOC, REW_GOOD_OBJ, REW_GREA_OBJ, REW_GREA_OBS },
        A_INT, p_ptr),

    Patron(N("バラン", "Balaan"),
        { REW_TY_CURSE, REW_HURT_LOT, REW_CURSE_WP, REW_CURSE_AR, REW_RUIN_ABL, REW_SUMMON_M, REW_LOSE_EXP, REW_POLY_SLF, REW_POLY_SLF, REW_POLY_WND,
            REW_SER_UNDE, REW_HEAL_FUL, REW_HEAL_FUL, REW_GAIN_EXP, REW_GAIN_EXP, REW_CHAOS_WP, REW_GOOD_OBJ, REW_GOOD_OBS, REW_GREA_OBS, REW_AUGM_ABL },
        A_STR, p_ptr),

    Patron(N("アリオッチ", "Arioch"),
        { REW_WRATH, REW_PISS_OFF, REW_RUIN_ABL, REW_LOSE_EXP, REW_H_SUMMON, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_POLY_SLF, REW_POLY_SLF,
            REW_MASS_GEN, REW_SER_DEMO, REW_HEAL_FUL, REW_CHAOS_WP, REW_CHAOS_WP, REW_GOOD_OBJ, REW_GAIN_EXP, REW_GREA_OBJ, REW_AUGM_ABL },
        A_INT, p_ptr),

    Patron(N("イーカー", "Eequor"),
        { REW_WRATH, REW_TY_CURSE, REW_PISS_OFF, REW_CURSE_WP, REW_RUIN_ABL, REW_IGNORE, REW_IGNORE, REW_POLY_SLF, REW_POLY_SLF, REW_POLY_WND, REW_GOOD_OBJ,
            REW_GOOD_OBJ, REW_SER_MONS, REW_HEAL_FUL, REW_GAIN_EXP, REW_GAIN_ABL, REW_CHAOS_WP, REW_GOOD_OBS, REW_GREA_OBJ, REW_AUGM_ABL },
        A_CON, p_ptr),

    Patron(N("ナージャン", "Narjhan"),
        { REW_WRATH, REW_CURSE_AR, REW_CURSE_WP, REW_CURSE_WP, REW_CURSE_AR, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_POLY_SLF, REW_POLY_SLF, REW_POLY_WND,
            REW_HEAL_FUL, REW_HEAL_FUL, REW_GAIN_EXP, REW_AUGM_ABL, REW_GOOD_OBJ, REW_GOOD_OBJ, REW_CHAOS_WP, REW_GREA_OBJ, REW_GREA_OBS },
        A_CHR, p_ptr),

    Patron(N("バロ", "Balo"),
        { REW_WRATH, REW_SER_DEMO, REW_CURSE_WP, REW_CURSE_AR, REW_LOSE_EXP, REW_GAIN_ABL, REW_LOSE_ABL, REW_POLY_WND, REW_POLY_SLF, REW_IGNORE, REW_DESTRUCT,
            REW_MASS_GEN, REW_CHAOS_WP, REW_GREA_OBJ, REW_HURT_LOT, REW_AUGM_ABL, REW_RUIN_ABL, REW_H_SUMMON, REW_GREA_OBS, REW_AUGM_ABL },
        A_RANDOM, p_ptr),

    Patron(N("コーン", "Khorne"),
        { REW_WRATH, REW_HURT_LOT, REW_HURT_LOT, REW_H_SUMMON, REW_H_SUMMON, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_SER_MONS, REW_SER_DEMO, REW_POLY_SLF,
            REW_POLY_WND, REW_HEAL_FUL, REW_GOOD_OBJ, REW_GOOD_OBJ, REW_CHAOS_WP, REW_GOOD_OBS, REW_GOOD_OBS, REW_GREA_OBJ, REW_GREA_OBS },
        A_STR, p_ptr),

    Patron(N("スラーネッシュ", "Slaanesh"),
        { REW_WRATH, REW_PISS_OFF, REW_PISS_OFF, REW_RUIN_ABL, REW_LOSE_ABL, REW_LOSE_EXP, REW_IGNORE, REW_IGNORE, REW_POLY_WND, REW_SER_DEMO, REW_POLY_SLF,
            REW_HEAL_FUL, REW_HEAL_FUL, REW_GOOD_OBJ, REW_GAIN_EXP, REW_GAIN_EXP, REW_CHAOS_WP, REW_GAIN_ABL, REW_GREA_OBJ, REW_AUGM_ABL },
        A_CHR, p_ptr),

    Patron(N("ナーグル", "Nurgle"),
        { REW_WRATH, REW_PISS_OFF, REW_HURT_LOT, REW_RUIN_ABL, REW_LOSE_ABL, REW_LOSE_EXP, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_POLY_SLF, REW_POLY_SLF,
            REW_POLY_WND, REW_HEAL_FUL, REW_GOOD_OBJ, REW_GAIN_ABL, REW_GAIN_ABL, REW_SER_UNDE, REW_CHAOS_WP, REW_GREA_OBJ, REW_AUGM_ABL },
        A_CON, p_ptr),

    Patron(N("ティーンチ", "Tzeentch"),
        { REW_WRATH, REW_CURSE_WP, REW_CURSE_AR, REW_RUIN_ABL, REW_LOSE_ABL, REW_LOSE_EXP, REW_IGNORE, REW_POLY_SLF, REW_POLY_SLF, REW_POLY_SLF, REW_POLY_SLF,
            REW_POLY_WND, REW_HEAL_FUL, REW_CHAOS_WP, REW_GREA_OBJ, REW_GAIN_ABL, REW_GAIN_ABL, REW_GAIN_EXP, REW_GAIN_EXP, REW_AUGM_ABL },
        A_INT, p_ptr),

    Patron(N("カイン", "Khaine"),
        { REW_WRATH, REW_HURT_LOT, REW_PISS_OFF, REW_LOSE_ABL, REW_LOSE_EXP, REW_IGNORE, REW_IGNORE, REW_DISPEL_C, REW_DO_HAVOC, REW_DO_HAVOC, REW_POLY_SLF,
            REW_POLY_SLF, REW_GAIN_EXP, REW_GAIN_ABL, REW_GAIN_ABL, REW_SER_MONS, REW_GOOD_OBJ, REW_CHAOS_WP, REW_GREA_OBJ, REW_GOOD_OBS },
        A_STR, p_ptr),

};

#ifdef JP
Patron::Patron(const char *name, const char *ename, std::vector<patron_reward> reward_table, const player_ability_type boost_stat, player_type *player_ptr)
#else
Patron::Patron(const char *name, std::vector<patron_reward> reward_table, player_ability_type boost_stat, player_type *player_ptr)
#endif
    : name(name)
#ifdef JP
    , ename(ename)
#endif
    , reward_table(std::move(reward_table))
    , boost_stat(boost_stat)
    , player_ptr(player_ptr)
{
}

void Patron::gain_level_reward(int chosen_reward) const
{
    char wrath_reason[32] = "";
    int nasty_chance = 6;
    int type;
    patron_reward effect;
    concptr reward = nullptr;
    GAME_TEXT o_name[MAX_NLEN];

    int count = 0;

    if (!chosen_reward) {
        if (this->player_ptr->suppress_multi_reward)
            return;
        else
            this->player_ptr->suppress_multi_reward = true;
    }

    if (this->player_ptr->lev == 13)
        nasty_chance = 2;
    else if (!(this->player_ptr->lev % 13))
        nasty_chance = 3;
    else if (!(this->player_ptr->lev % 14))
        nasty_chance = 12;

    if (one_in_(nasty_chance))
        type = randint1(20); /* Allow the 'nasty' effects */
    else
        type = randint1(15) + 5; /* Or disallow them */

    if (type < 1)
        type = 1;
    if (type > 20)
        type = 20;
    type--;

    sprintf(wrath_reason, _("%sの怒り", "the Wrath of %s"), this->name.c_str());

    effect = this->reward_table[type];

    if (one_in_(6) && !chosen_reward) {
        msg_format(_("%^sは褒美としてあなたを突然変異させた。", "%^s rewards you with a mutation!"), this->name.c_str());
        (void)gain_mutation(player_ptr, 0);
        reward = _("変異した。", "mutation");
    } else {
        switch (chosen_reward ? chosen_reward : effect) {

        case REW_POLY_SLF:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「汝、新たなる姿を必要とせり！」", "'Thou needst a new form, mortal!'"));

            do_poly_self(player_ptr);
            reward = _("変異した。", "polymorphing");
            break;

        case REW_GAIN_EXP:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「汝は良く行いたり！続けよ！」", "'Well done, mortal! Lead on!'"));

            if (this->player_ptr->prace == PlayerRaceType::ANDROID) {
                msg_print(_("しかし何も起こらなかった。", "But, nothing happens."));
            } else if (this->player_ptr->exp < PY_MAX_EXP) {
                int32_t ee = (this->player_ptr->exp / 2) + 10;
                if (ee > 100000L)
                    ee = 100000L;
                msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));

                gain_exp(player_ptr, ee);
                reward = _("経験値を得た", "experience");
            }
            break;

        case REW_LOSE_EXP:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「下僕よ、汝それに値せず。」", "'Thou didst not deserve that, slave.'"));

            if (this->player_ptr->prace == PlayerRaceType::ANDROID) {
                msg_print(_("しかし何も起こらなかった。", "But, nothing happens."));
            } else {
                lose_exp(player_ptr, this->player_ptr->exp / 6);
                reward = _("経験値を失った。", "losing experience");
            }
            break;

        case REW_GOOD_OBJ:
            msg_format(_("%sの声がささやいた:", "The voice of %s whispers:"), this->name.c_str());
            msg_print(_("「我が与えし物を賢明に使うべし。」", "'Use my gift wisely.'"));

            acquirement(player_ptr, this->player_ptr->y, this->player_ptr->x, 1, false, false, false);
            reward = _("上質なアイテムを手に入れた。", "a good item");
            break;

        case REW_GREA_OBJ:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「我が与えし物を賢明に使うべし。」", "'Use my gift wisely.'"));

            acquirement(player_ptr, this->player_ptr->y, this->player_ptr->x, 1, true, false, false);
            reward = _("高級品のアイテムを手に入れた。", "an excellent item");
            break;

        case REW_CHAOS_WP:
            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「汝の行いは貴き剣に値せり。」", "'Thy deed hath earned thee a worthy blade.'"));
            acquire_chaos_weapon(player_ptr);
            reward = _("(混沌)の武器を手に入れた。", "chaos weapon");
            break;

        case REW_GOOD_OBS:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「汝の行いは貴き報いに値せり。」", "'Thy deed hath earned thee a worthy reward.'"));

            acquirement(player_ptr, this->player_ptr->y, this->player_ptr->x, randint1(2) + 1, false, false, false);
            reward = _("上質なアイテムを手に入れた。", "good items");
            break;

        case REW_GREA_OBS:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「下僕よ、汝の献身への我が惜しみ無き報いを見るがよい。」", "'Behold, mortal, how generously I reward thy loyalty.'"));

            acquirement(player_ptr, this->player_ptr->y, this->player_ptr->x, randint1(2) + 1, true, false, false);
            reward = _("高級品のアイテムを手に入れた。", "excellent items");
            break;

        case REW_TY_CURSE:
            msg_format(_("%sの声が轟き渡った:", "The voice of %s thunders:"), this->name.c_str());
            msg_print(_("「下僕よ、汝傲慢なり。」", "'Thou art growing arrogant, mortal.'"));

            (void)activate_ty_curse(player_ptr, false, &count);
            reward = _("禍々しい呪いをかけられた。", "cursing");
            break;

        case REW_SUMMON_M:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「我が下僕たちよ、かの傲慢なる者を倒すべし！」", "'My pets, destroy the arrogant mortal!'"));

            for (int i = 0, summon_num = randint1(5) + 1; i < summon_num; i++) {
                (void)summon_specific(player_ptr, 0, this->player_ptr->y, this->player_ptr->x, this->player_ptr->current_floor_ptr->dun_level, SUMMON_NONE,
                    (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
            }
            reward = _("モンスターを召喚された。", "summoning hostile monsters");
            break;

        case REW_H_SUMMON:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「汝、より強き敵を必要とせり！」", "'Thou needst worthier opponents!'"));

            activate_hi_summon(player_ptr, this->player_ptr->y, this->player_ptr->x, false);
            reward = _("モンスターを召喚された。", "summoning many hostile monsters");
            break;

        case REW_DO_HAVOC:
            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「死と破壊こそ我が喜びなり！」", "'Death and destruction! This pleaseth me!'"));

            call_chaos(player_ptr);
            reward = _("カオスの力が渦巻いた。", "calling chaos");
            break;

        case REW_GAIN_ABL:
            msg_format(_("%sの声が鳴り響いた:", "The voice of %s rings out:"), this->name.c_str());
            msg_print(_("「留まるのだ、下僕よ。余が汝の肉体を鍛えん。」", "'Stay, mortal, and let me mold thee.'"));

            if (one_in_(3) && !(this->boost_stat != A_RANDOM))
                do_inc_stat(player_ptr, this->boost_stat);
            else
                do_inc_stat(player_ptr, randint0(6));
            reward = _("能力値が上がった。", "increasing a stat");
            break;

        case REW_LOSE_ABL:
            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「下僕よ、余は汝に飽みたり。」", "'I grow tired of thee, mortal.'"));

            if (one_in_(3) && !(this->boost_stat != A_RANDOM))
                do_dec_stat(player_ptr, this->boost_stat);
            else
                (void)do_dec_stat(player_ptr, randint0(6));
            reward = _("能力値が下がった。", "decreasing a stat");
            break;

        case REW_RUIN_ABL:

            msg_format(_("%sの声が轟き渡った:", "The voice of %s thunders:"), this->name.c_str());
            msg_print(_("「汝、謙虚たることを学ぶべし！」", "'Thou needst a lesson in humility, mortal!'"));
            msg_print(_("あなたは以前より弱くなった！", "You feel less powerful!"));

            for (int stat = 0; stat < A_MAX; stat++) {
                (void)dec_stat(player_ptr, stat, 10 + randint1(15), true);
            }
            reward = _("全能力値が下がった。", "decreasing all stats");
            break;

        case REW_POLY_WND:

            msg_format(_("%sの力が触れるのを感じた。", "You feel the power of %s touch you."), this->name.c_str());
            do_poly_wounds(player_ptr);
            reward = _("傷が変化した。", "polymorphing wounds");
            break;

        case REW_AUGM_ABL:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());

            msg_print(_("「我がささやかなる賜物を受けとるがよい！」", "'Receive this modest gift from me!'"));

            for (int stat = 0; stat < A_MAX; stat++) {
                (void)do_inc_stat(player_ptr, stat);
            }
            reward = _("全能力値が上がった。", "increasing all stats");
            break;

        case REW_HURT_LOT:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「苦しむがよい、無能な愚か者よ！」", "'Suffer, pathetic fool!'"));

            fire_ball(player_ptr, GF_DISINTEGRATE, 0, this->player_ptr->lev * 4, 4);
            take_hit(player_ptr, DAMAGE_NOESCAPE, this->player_ptr->lev * 4, wrath_reason);
            reward = _("分解の球が発生した。", "generating disintegration ball");
            break;

        case REW_HEAL_FUL:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            (void)restore_level(player_ptr);
            (void)restore_all_status(player_ptr);
            (void)true_healing(player_ptr, 5000);
            reward = _("体力が回復した。", "healing");
            break;

        case REW_CURSE_WP: {
            inventory_slot_type slot;

            if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(player_ptr, INVEN_SUB_HAND))
                break;
            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「汝、武器に頼ることなかれ。」", "'Thou reliest too much on thy weapon.'"));

            slot = INVEN_MAIN_HAND;
            if (has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
                slot = INVEN_SUB_HAND;
                if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && one_in_(2))
                    slot = INVEN_MAIN_HAND;
            }
            describe_flavor(player_ptr, o_name, &this->player_ptr->inventory_list[slot], OD_NAME_ONLY);
            (void)curse_weapon_object(player_ptr, false, &this->player_ptr->inventory_list[slot]);
            reward = format(_("%sが破壊された。", "destroying %s"), o_name);
            break;
        }

        case REW_CURSE_AR:

            if (!this->player_ptr->inventory_list[INVEN_BODY].k_idx)
                break;
            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「汝、防具に頼ることなかれ。」", "'Thou reliest too much on thine equipment.'"));

            describe_flavor(player_ptr, o_name, &this->player_ptr->inventory_list[INVEN_BODY], OD_NAME_ONLY);
            (void)curse_armor(player_ptr);
            reward = format(_("%sが破壊された。", "destroying %s"), o_name);
            break;

        case REW_PISS_OFF:

            msg_format(_("%sの声がささやいた:", "The voice of %s whispers:"), this->name.c_str());
            msg_print(_("「我を怒りしめた罪を償うべし。」", "'Now thou shalt pay for annoying me.'"));

            switch (randint1(4)) {
            case 1:
                (void)activate_ty_curse(player_ptr, false, &count);
                reward = _("禍々しい呪いをかけられた。", "cursing");
                break;
            case 2:
                activate_hi_summon(player_ptr, this->player_ptr->y, this->player_ptr->x, false);
                reward = _("モンスターを召喚された。", "summoning hostile monsters");
                break;
            case 3:
                if (one_in_(2)) {
                    inventory_slot_type slot;
                    if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(player_ptr, INVEN_SUB_HAND))
                        break;
                    slot = INVEN_MAIN_HAND;
                    if (has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
                        slot = INVEN_SUB_HAND;
                        if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND) && one_in_(2))
                            slot = INVEN_MAIN_HAND;
                    }
                    describe_flavor(player_ptr, o_name, &this->player_ptr->inventory_list[slot], OD_NAME_ONLY);
                    (void)curse_weapon_object(player_ptr, false, &this->player_ptr->inventory_list[slot]);
                    reward = format(_("%sが破壊された。", "destroying %s"), o_name);
                } else {
                    if (!this->player_ptr->inventory_list[INVEN_BODY].k_idx)
                        break;
                    describe_flavor(player_ptr, o_name, &this->player_ptr->inventory_list[INVEN_BODY], OD_NAME_ONLY);
                    (void)curse_armor(player_ptr);
                    reward = format(_("%sが破壊された。", "destroying %s"), o_name);
                }
                break;
            default:
                for (int stat = 0; stat < A_MAX; stat++) {
                    (void)dec_stat(player_ptr, stat, 10 + randint1(15), true);
                }
                reward = _("全能力値が下がった。", "decreasing all stats");
                break;
            }
            break;

        case REW_WRATH:

            msg_format(_("%sの声が轟き渡った:", "The voice of %s thunders:"), this->name.c_str());
            msg_print(_("「死ぬがよい、下僕よ！」", "'Die, mortal!'"));

            take_hit(player_ptr, DAMAGE_LOSELIFE, this->player_ptr->lev * 4, wrath_reason);
            for (int stat = 0; stat < A_MAX; stat++) {
                (void)dec_stat(player_ptr, stat, 10 + randint1(15), false);
            }
            activate_hi_summon(player_ptr, this->player_ptr->y, this->player_ptr->x, false);
            (void)activate_ty_curse(player_ptr, false, &count);
            if (one_in_(2)) {
                inventory_slot_type slot = INVEN_NONE;

                if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND)) {
                    slot = INVEN_MAIN_HAND;
                    if (has_melee_weapon(player_ptr, INVEN_SUB_HAND) && one_in_(2))
                        slot = INVEN_SUB_HAND;
                } else if (has_melee_weapon(player_ptr, INVEN_SUB_HAND))
                    slot = INVEN_SUB_HAND;

                if (slot)
                    (void)curse_weapon_object(player_ptr, false, &this->player_ptr->inventory_list[slot]);
            }
            if (one_in_(2))
                (void)curse_armor(player_ptr);
            break;

        case REW_DESTRUCT:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「死と破壊こそ我が喜びなり！」", "'Death and destruction! This pleaseth me!'"));

            (void)destroy_area(player_ptr, this->player_ptr->y, this->player_ptr->x, 25, false);
            reward = _("ダンジョンが*破壊*された。", "*destruct*ing dungeon");
            break;

        case REW_GENOCIDE:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「我、汝の敵を抹殺せん！」", "'Let me relieve thee of thine oppressors!'"));
            (void)symbol_genocide(player_ptr, 0, false);
            reward = _("モンスターが抹殺された。", "genociding monsters");
            break;

        case REW_MASS_GEN:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), this->name.c_str());
            msg_print(_("「我、汝の敵を抹殺せん！」", "'Let me relieve thee of thine oppressors!'"));

            (void)mass_genocide(player_ptr, 0, false);
            reward = _("モンスターが抹殺された。", "genociding nearby monsters");
            break;

        case REW_DISPEL_C:

            msg_format(_("%sの力が敵を攻撃するのを感じた！", "You can feel the power of %s assault your enemies!"), this->name.c_str());
            (void)dispel_monsters(player_ptr, this->player_ptr->lev * 4);
            break;

        case REW_IGNORE:

            msg_format(_("%sはあなたを無視した。", "%s ignores you."), this->name.c_str());
            break;

        case REW_SER_DEMO:

            msg_format(_("%sは褒美として悪魔の使いをよこした！", "%s rewards you with a demonic servant!"), this->name.c_str());

            if (!summon_specific(player_ptr, -1, this->player_ptr->y, this->player_ptr->x, this->player_ptr->current_floor_ptr->dun_level, SUMMON_DEMON, PM_FORCE_PET))
                msg_print(_("何も現れなかった...", "Nobody ever turns up..."));
            else
                reward = _("悪魔がペットになった。", "a demonic servant");

            break;

        case REW_SER_MONS:
            msg_format(_("%sは褒美として使いをよこした！", "%s rewards you with a servant!"), this->name.c_str());

            if (!summon_specific(player_ptr, -1, this->player_ptr->y, this->player_ptr->x, this->player_ptr->current_floor_ptr->dun_level, SUMMON_NONE, PM_FORCE_PET))
                msg_print(_("何も現れなかった...", "Nobody ever turns up..."));
            else
                reward = _("モンスターがペットになった。", "a servant");

            break;

        case REW_SER_UNDE:
            msg_format(_("%sは褒美としてアンデッドの使いをよこした。", "%s rewards you with an undead servant!"), this->name.c_str());

            if (!summon_specific(player_ptr, -1, this->player_ptr->y, this->player_ptr->x, this->player_ptr->current_floor_ptr->dun_level, SUMMON_UNDEAD, PM_FORCE_PET))
                msg_print(_("何も現れなかった...", "Nobody ever turns up..."));
            else
                reward = _("アンデッドがペットになった。", "an undead servant");

            break;

        default:
            msg_format(_("%sの声がどもった:", "The voice of %s stammers:"), this->name.c_str());
            msg_format(_("「あー、あー、答えは %d/%d。質問は何？」", "'Uh... uh... the answer's %d/%d, what's the question?'"), type, effect);
        }
    }
    if (reward) {
        exe_write_diary(player_ptr, DIARY_DESCRIPTION, 0, format(_("パトロンの報酬で%s", "The patron rewarded you with %s."), reward));
    }
}

void Patron::admire() const
{
    if ((this->player_ptr->pclass == CLASS_CHAOS_WARRIOR) || this->player_ptr->muta.has(MUTA::CHAOS_GIFT)) {
        msg_format(_("%sからの声が響いた。", "The voice of %s booms out:"), this->name.c_str());
        msg_print(_("『よくやった、定命の者よ！』", "'Thou art donst well, mortal!'"));
    }
}
