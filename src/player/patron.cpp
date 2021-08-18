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
#include "player-info/equipment-info.h"
#include "player/player-class.h"
#include "player/player-damage.h"
#include "player/player-race-types.h"
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
/*!
 * @brief カオスパトロン名テーブル
 */
const concptr chaos_patrons[MAX_PATRON] = { "スローター", "マベロード", "チャードロス", "ハイオンハーン", "キシオムバーグ",

    "ピアレー", "バラン", "アリオッチ", "イーカー", "ナージャン",

    "バロ", "コーン", "スラーネッシュ", "ナーグル", "ティーンチ",

    "カイン" };
#else
const concptr chaos_patrons[MAX_PATRON] = { "Slortar", "Mabelode", "Chardros", "Hionhurn", "Xiombarg",

    "Pyaray", "Balaan", "Arioch", "Eequor", "Narjhan",

    "Balo", "Khorne", "Slaanesh", "Nurgle", "Tzeentch",

    "Khaine" };
#endif

/*!
 * @brief カオスパトロンの報酬能力値テーブル
 */
const int chaos_stats[MAX_PATRON] = {
    A_CON, /* Slortar */
    A_CON, /* Mabelode */
    A_STR, /* Chardros */
    A_STR, /* Hionhurn */
    A_STR, /* Xiombarg */

    A_INT, /* Pyaray */
    A_STR, /* Balaan */
    A_INT, /* Arioch */
    A_CON, /* Eequor */
    A_CHR, /* Narjhan */

    -1, /* Balo */
    A_STR, /* Khorne */
    A_CHR, /* Slaanesh */
    A_CON, /* Nurgle */
    A_INT, /* Tzeentch */

    A_STR, /* Khaine */
};

/*!
 * @brief カオスパトロンの報酬テーブル
 */
const patron_reward chaos_rewards[MAX_PATRON][20] = {
    /* Slortar the Old: */
    { REW_WRATH, REW_CURSE_WP, REW_CURSE_AR, REW_RUIN_ABL, REW_LOSE_ABL, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_POLY_WND, REW_POLY_SLF, REW_POLY_SLF,
        REW_POLY_SLF, REW_GAIN_ABL, REW_GAIN_ABL, REW_GAIN_EXP, REW_GOOD_OBJ, REW_CHAOS_WP, REW_GREA_OBJ, REW_AUGM_ABL, REW_AUGM_ABL },

    /* Mabelode the Faceless: */
    { REW_WRATH, REW_CURSE_WP, REW_CURSE_AR, REW_H_SUMMON, REW_SUMMON_M, REW_SUMMON_M, REW_IGNORE, REW_IGNORE, REW_POLY_WND, REW_POLY_WND, REW_POLY_SLF,
        REW_HEAL_FUL, REW_HEAL_FUL, REW_GAIN_ABL, REW_SER_UNDE, REW_CHAOS_WP, REW_GOOD_OBJ, REW_GOOD_OBJ, REW_GOOD_OBS, REW_GOOD_OBS },

    /* Chardros the Reaper: */
    { REW_WRATH, REW_WRATH, REW_HURT_LOT, REW_PISS_OFF, REW_H_SUMMON, REW_SUMMON_M, REW_IGNORE, REW_IGNORE, REW_DESTRUCT, REW_SER_UNDE, REW_GENOCIDE,
        REW_MASS_GEN, REW_MASS_GEN, REW_DISPEL_C, REW_GOOD_OBJ, REW_CHAOS_WP, REW_GOOD_OBS, REW_GOOD_OBS, REW_AUGM_ABL, REW_AUGM_ABL },

    /* Hionhurn the Executioner: */
    { REW_WRATH, REW_WRATH, REW_CURSE_WP, REW_CURSE_AR, REW_RUIN_ABL, REW_IGNORE, REW_IGNORE, REW_SER_UNDE, REW_DESTRUCT, REW_GENOCIDE, REW_MASS_GEN,
        REW_MASS_GEN, REW_HEAL_FUL, REW_GAIN_ABL, REW_GAIN_ABL, REW_CHAOS_WP, REW_GOOD_OBS, REW_GOOD_OBS, REW_AUGM_ABL, REW_AUGM_ABL },

    /* Xiombarg the Sword-Queen: */
    { REW_TY_CURSE, REW_TY_CURSE, REW_PISS_OFF, REW_RUIN_ABL, REW_LOSE_ABL, REW_IGNORE, REW_POLY_SLF, REW_POLY_SLF, REW_POLY_WND, REW_POLY_WND, REW_GENOCIDE,
        REW_DISPEL_C, REW_GOOD_OBJ, REW_GOOD_OBJ, REW_SER_MONS, REW_GAIN_ABL, REW_CHAOS_WP, REW_GAIN_EXP, REW_AUGM_ABL, REW_GOOD_OBS },

    /* Pyaray the Tentacled Whisperer of Impossible Secretes: */
    { REW_WRATH, REW_TY_CURSE, REW_PISS_OFF, REW_H_SUMMON, REW_H_SUMMON, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_POLY_WND, REW_POLY_SLF, REW_POLY_SLF,
        REW_SER_DEMO, REW_HEAL_FUL, REW_GAIN_ABL, REW_GAIN_ABL, REW_CHAOS_WP, REW_DO_HAVOC, REW_GOOD_OBJ, REW_GREA_OBJ, REW_GREA_OBS },

    /* Balaan the Grim: */
    { REW_TY_CURSE, REW_HURT_LOT, REW_CURSE_WP, REW_CURSE_AR, REW_RUIN_ABL, REW_SUMMON_M, REW_LOSE_EXP, REW_POLY_SLF, REW_POLY_SLF, REW_POLY_WND, REW_SER_UNDE,
        REW_HEAL_FUL, REW_HEAL_FUL, REW_GAIN_EXP, REW_GAIN_EXP, REW_CHAOS_WP, REW_GOOD_OBJ, REW_GOOD_OBS, REW_GREA_OBS, REW_AUGM_ABL },

    /* Arioch, Duke of Hell: */
    { REW_WRATH, REW_PISS_OFF, REW_RUIN_ABL, REW_LOSE_EXP, REW_H_SUMMON, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_POLY_SLF, REW_POLY_SLF,
        REW_MASS_GEN, REW_SER_DEMO, REW_HEAL_FUL, REW_CHAOS_WP, REW_CHAOS_WP, REW_GOOD_OBJ, REW_GAIN_EXP, REW_GREA_OBJ, REW_AUGM_ABL },

    /* Eequor, Blue Lady of Dismay: */
    { REW_WRATH, REW_TY_CURSE, REW_PISS_OFF, REW_CURSE_WP, REW_RUIN_ABL, REW_IGNORE, REW_IGNORE, REW_POLY_SLF, REW_POLY_SLF, REW_POLY_WND, REW_GOOD_OBJ,
        REW_GOOD_OBJ, REW_SER_MONS, REW_HEAL_FUL, REW_GAIN_EXP, REW_GAIN_ABL, REW_CHAOS_WP, REW_GOOD_OBS, REW_GREA_OBJ, REW_AUGM_ABL },

    /* Narjhan, Lord of Beggars: */
    { REW_WRATH, REW_CURSE_AR, REW_CURSE_WP, REW_CURSE_WP, REW_CURSE_AR, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_POLY_SLF, REW_POLY_SLF, REW_POLY_WND,
        REW_HEAL_FUL, REW_HEAL_FUL, REW_GAIN_EXP, REW_AUGM_ABL, REW_GOOD_OBJ, REW_GOOD_OBJ, REW_CHAOS_WP, REW_GREA_OBJ, REW_GREA_OBS },

    /* Balo the Jester: */
    { REW_WRATH, REW_SER_DEMO, REW_CURSE_WP, REW_CURSE_AR, REW_LOSE_EXP, REW_GAIN_ABL, REW_LOSE_ABL, REW_POLY_WND, REW_POLY_SLF, REW_IGNORE, REW_DESTRUCT,
        REW_MASS_GEN, REW_CHAOS_WP, REW_GREA_OBJ, REW_HURT_LOT, REW_AUGM_ABL, REW_RUIN_ABL, REW_H_SUMMON, REW_GREA_OBS, REW_AUGM_ABL },

    /* Khorne the Bloodgod: */
    { REW_WRATH, REW_HURT_LOT, REW_HURT_LOT, REW_H_SUMMON, REW_H_SUMMON, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_SER_MONS, REW_SER_DEMO, REW_POLY_SLF,
        REW_POLY_WND, REW_HEAL_FUL, REW_GOOD_OBJ, REW_GOOD_OBJ, REW_CHAOS_WP, REW_GOOD_OBS, REW_GOOD_OBS, REW_GREA_OBJ, REW_GREA_OBS },

    /* Slaanesh: */
    { REW_WRATH, REW_PISS_OFF, REW_PISS_OFF, REW_RUIN_ABL, REW_LOSE_ABL, REW_LOSE_EXP, REW_IGNORE, REW_IGNORE, REW_POLY_WND, REW_SER_DEMO, REW_POLY_SLF,
        REW_HEAL_FUL, REW_HEAL_FUL, REW_GOOD_OBJ, REW_GAIN_EXP, REW_GAIN_EXP, REW_CHAOS_WP, REW_GAIN_ABL, REW_GREA_OBJ, REW_AUGM_ABL },

    /* Nurgle: */
    { REW_WRATH, REW_PISS_OFF, REW_HURT_LOT, REW_RUIN_ABL, REW_LOSE_ABL, REW_LOSE_EXP, REW_IGNORE, REW_IGNORE, REW_IGNORE, REW_POLY_SLF, REW_POLY_SLF,
        REW_POLY_WND, REW_HEAL_FUL, REW_GOOD_OBJ, REW_GAIN_ABL, REW_GAIN_ABL, REW_SER_UNDE, REW_CHAOS_WP, REW_GREA_OBJ, REW_AUGM_ABL },

    /* Tzeentch: */
    { REW_WRATH, REW_CURSE_WP, REW_CURSE_AR, REW_RUIN_ABL, REW_LOSE_ABL, REW_LOSE_EXP, REW_IGNORE, REW_POLY_SLF, REW_POLY_SLF, REW_POLY_SLF, REW_POLY_SLF,
        REW_POLY_WND, REW_HEAL_FUL, REW_CHAOS_WP, REW_GREA_OBJ, REW_GAIN_ABL, REW_GAIN_ABL, REW_GAIN_EXP, REW_GAIN_EXP, REW_AUGM_ABL },

    /* Khaine: */
    { REW_WRATH, REW_HURT_LOT, REW_PISS_OFF, REW_LOSE_ABL, REW_LOSE_EXP, REW_IGNORE, REW_IGNORE, REW_DISPEL_C, REW_DO_HAVOC, REW_DO_HAVOC, REW_POLY_SLF,
        REW_POLY_SLF, REW_GAIN_EXP, REW_GAIN_ABL, REW_GAIN_ABL, REW_SER_MONS, REW_GOOD_OBJ, REW_CHAOS_WP, REW_GREA_OBJ, REW_GOOD_OBS }
};

void gain_level_reward(player_type *creature_ptr, int chosen_reward)
{
    char wrath_reason[32] = "";
    int nasty_chance = 6;
    int type, effect;
    concptr reward = NULL;
    GAME_TEXT o_name[MAX_NLEN];

    int count = 0;

    if (!chosen_reward) {
        if (creature_ptr->suppress_multi_reward)
            return;
        else
            creature_ptr->suppress_multi_reward = true;
    }

    if (creature_ptr->lev == 13)
        nasty_chance = 2;
    else if (!(creature_ptr->lev % 13))
        nasty_chance = 3;
    else if (!(creature_ptr->lev % 14))
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

    sprintf(wrath_reason, _("%sの怒り", "the Wrath of %s"), chaos_patrons[creature_ptr->chaos_patron]);

    effect = chaos_rewards[creature_ptr->chaos_patron][type];

    if (one_in_(6) && !chosen_reward) {
        msg_format(_("%^sは褒美としてあなたを突然変異させた。", "%^s rewards you with a mutation!"), chaos_patrons[creature_ptr->chaos_patron]);
        (void)gain_mutation(creature_ptr, 0);
        reward = _("変異した。", "mutation");
    } else {
        switch (chosen_reward ? chosen_reward : effect) {

        case REW_POLY_SLF:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「汝、新たなる姿を必要とせり！」", "'Thou needst a new form, mortal!'"));

            do_poly_self(creature_ptr);
            reward = _("変異した。", "polymorphing");
            break;

        case REW_GAIN_EXP:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「汝は良く行いたり！続けよ！」", "'Well done, mortal! Lead on!'"));

            if (creature_ptr->prace == player_race_type::ANDROID) {
                msg_print(_("しかし何も起こらなかった。", "But, nothing happens."));
            } else if (creature_ptr->exp < PY_MAX_EXP) {
                s32b ee = (creature_ptr->exp / 2) + 10;
                if (ee > 100000L)
                    ee = 100000L;
                msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));

                gain_exp(creature_ptr, ee);
                reward = _("経験値を得た", "experience");
            }
            break;

        case REW_LOSE_EXP:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「下僕よ、汝それに値せず。」", "'Thou didst not deserve that, slave.'"));

            if (creature_ptr->prace == player_race_type::ANDROID) {
                msg_print(_("しかし何も起こらなかった。", "But, nothing happens."));
            } else {
                lose_exp(creature_ptr, creature_ptr->exp / 6);
                reward = _("経験値を失った。", "losing experience");
            }
            break;

        case REW_GOOD_OBJ:
            msg_format(_("%sの声がささやいた:", "The voice of %s whispers:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「我が与えし物を賢明に使うべし。」", "'Use my gift wisely.'"));

            acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, 1, false, false, false);
            reward = _("上質なアイテムを手に入れた。", "a good item");
            break;

        case REW_GREA_OBJ:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「我が与えし物を賢明に使うべし。」", "'Use my gift wisely.'"));

            acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, 1, true, false, false);
            reward = _("高級品のアイテムを手に入れた。", "an excellent item");
            break;

        case REW_CHAOS_WP:
            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「汝の行いは貴き剣に値せり。」", "'Thy deed hath earned thee a worthy blade.'"));
            acquire_chaos_weapon(creature_ptr);
            reward = _("(混沌)の武器を手に入れた。", "chaos weapon");
            break;

        case REW_GOOD_OBS:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「汝の行いは貴き報いに値せり。」", "'Thy deed hath earned thee a worthy reward.'"));

            acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, randint1(2) + 1, false, false, false);
            reward = _("上質なアイテムを手に入れた。", "good items");
            break;

        case REW_GREA_OBS:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「下僕よ、汝の献身への我が惜しみ無き報いを見るがよい。」", "'Behold, mortal, how generously I reward thy loyalty.'"));

            acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, randint1(2) + 1, true, false, false);
            reward = _("高級品のアイテムを手に入れた。", "excellent items");
            break;

        case REW_TY_CURSE:
            msg_format(_("%sの声が轟き渡った:", "The voice of %s thunders:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「下僕よ、汝傲慢なり。」", "'Thou art growing arrogant, mortal.'"));

            (void)activate_ty_curse(creature_ptr, false, &count);
            reward = _("禍々しい呪いをかけられた。", "cursing");
            break;

        case REW_SUMMON_M:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「我が下僕たちよ、かの傲慢なる者を倒すべし！」", "'My pets, destroy the arrogant mortal!'"));

            for (int i = 0, summon_num = randint1(5) + 1; i < summon_num; i++) {
                (void)summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_NONE,
                    (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET));
            }
            reward = _("モンスターを召喚された。", "summoning hostile monsters");
            break;

        case REW_H_SUMMON:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「汝、より強き敵を必要とせり！」", "'Thou needst worthier opponents!'"));

            activate_hi_summon(creature_ptr, creature_ptr->y, creature_ptr->x, false);
            reward = _("モンスターを召喚された。", "summoning many hostile monsters");
            break;

        case REW_DO_HAVOC:
            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「死と破壊こそ我が喜びなり！」", "'Death and destruction! This pleaseth me!'"));

            call_chaos(creature_ptr);
            reward = _("カオスの力が渦巻いた。", "calling chaos");
            break;

        case REW_GAIN_ABL:
            msg_format(_("%sの声が鳴り響いた:", "The voice of %s rings out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「留まるのだ、下僕よ。余が汝の肉体を鍛えん。」", "'Stay, mortal, and let me mold thee.'"));

            if (one_in_(3) && !(chaos_stats[creature_ptr->chaos_patron] < 0))
                do_inc_stat(creature_ptr, chaos_stats[creature_ptr->chaos_patron]);
            else
                do_inc_stat(creature_ptr, randint0(6));
            reward = _("能力値が上がった。", "increasing a stat");
            break;

        case REW_LOSE_ABL:
            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「下僕よ、余は汝に飽みたり。」", "'I grow tired of thee, mortal.'"));

            if (one_in_(3) && !(chaos_stats[creature_ptr->chaos_patron] < 0))
                do_dec_stat(creature_ptr, chaos_stats[creature_ptr->chaos_patron]);
            else
                (void)do_dec_stat(creature_ptr, randint0(6));
            reward = _("能力値が下がった。", "decreasing a stat");
            break;

        case REW_RUIN_ABL:

            msg_format(_("%sの声が轟き渡った:", "The voice of %s thunders:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「汝、謙虚たることを学ぶべし！」", "'Thou needst a lesson in humility, mortal!'"));
            msg_print(_("あなたは以前より弱くなった！", "You feel less powerful!"));

            for (int stat = 0; stat < A_MAX; stat++) {
                (void)dec_stat(creature_ptr, stat, 10 + randint1(15), true);
            }
            reward = _("全能力値が下がった。", "decreasing all stats");
            break;

        case REW_POLY_WND:

            msg_format(_("%sの力が触れるのを感じた。", "You feel the power of %s touch you."), chaos_patrons[creature_ptr->chaos_patron]);
            do_poly_wounds(creature_ptr);
            reward = _("傷が変化した。", "polymorphing wounds");
            break;

        case REW_AUGM_ABL:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);

            msg_print(_("「我がささやかなる賜物を受けとるがよい！」", "'Receive this modest gift from me!'"));

            for (int stat = 0; stat < A_MAX; stat++) {
                (void)do_inc_stat(creature_ptr, stat);
            }
            reward = _("全能力値が上がった。", "increasing all stats");
            break;

        case REW_HURT_LOT:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「苦しむがよい、無能な愚か者よ！」", "'Suffer, pathetic fool!'"));

            fire_ball(creature_ptr, GF_DISINTEGRATE, 0, creature_ptr->lev * 4, 4);
            take_hit(creature_ptr, DAMAGE_NOESCAPE, creature_ptr->lev * 4, wrath_reason);
            reward = _("分解の球が発生した。", "generating disintegration ball");
            break;

        case REW_HEAL_FUL:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            (void)restore_level(creature_ptr);
            (void)restore_all_status(creature_ptr);
            (void)true_healing(creature_ptr, 5000);
            reward = _("体力が回復した。", "healing");
            break;

        case REW_CURSE_WP: {
            inventory_slot_type slot;

            if (!has_melee_weapon(creature_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(creature_ptr, INVEN_SUB_HAND))
                break;
            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「汝、武器に頼ることなかれ。」", "'Thou reliest too much on thy weapon.'"));

            slot = INVEN_MAIN_HAND;
            if (has_melee_weapon(creature_ptr, INVEN_SUB_HAND)) {
                slot = INVEN_SUB_HAND;
                if (has_melee_weapon(creature_ptr, INVEN_MAIN_HAND) && one_in_(2))
                    slot = INVEN_MAIN_HAND;
            }
            describe_flavor(creature_ptr, o_name, &creature_ptr->inventory_list[slot], OD_NAME_ONLY);
            (void)curse_weapon_object(creature_ptr, false, &creature_ptr->inventory_list[slot]);
            reward = format(_("%sが破壊された。", "destroying %s"), o_name);
            break;
        }

        case REW_CURSE_AR:

            if (!creature_ptr->inventory_list[INVEN_BODY].k_idx)
                break;
            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「汝、防具に頼ることなかれ。」", "'Thou reliest too much on thine equipment.'"));

            describe_flavor(creature_ptr, o_name, &creature_ptr->inventory_list[INVEN_BODY], OD_NAME_ONLY);
            (void)curse_armor(creature_ptr);
            reward = format(_("%sが破壊された。", "destroying %s"), o_name);
            break;

        case REW_PISS_OFF:

            msg_format(_("%sの声がささやいた:", "The voice of %s whispers:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「我を怒りしめた罪を償うべし。」", "'Now thou shalt pay for annoying me.'"));

            switch (randint1(4)) {
            case 1:
                (void)activate_ty_curse(creature_ptr, false, &count);
                reward = _("禍々しい呪いをかけられた。", "cursing");
                break;
            case 2:
                activate_hi_summon(creature_ptr, creature_ptr->y, creature_ptr->x, false);
                reward = _("モンスターを召喚された。", "summoning hostile monsters");
                break;
            case 3:
                if (one_in_(2)) {
                    inventory_slot_type slot;
                    if (!has_melee_weapon(creature_ptr, INVEN_MAIN_HAND) && !has_melee_weapon(creature_ptr, INVEN_SUB_HAND))
                        break;
                    slot = INVEN_MAIN_HAND;
                    if (has_melee_weapon(creature_ptr, INVEN_SUB_HAND)) {
                        slot = INVEN_SUB_HAND;
                        if (has_melee_weapon(creature_ptr, INVEN_MAIN_HAND) && one_in_(2))
                            slot = INVEN_MAIN_HAND;
                    }
                    describe_flavor(creature_ptr, o_name, &creature_ptr->inventory_list[slot], OD_NAME_ONLY);
                    (void)curse_weapon_object(creature_ptr, false, &creature_ptr->inventory_list[slot]);
                    reward = format(_("%sが破壊された。", "destroying %s"), o_name);
                } else {
                    if (!creature_ptr->inventory_list[INVEN_BODY].k_idx)
                        break;
                    describe_flavor(creature_ptr, o_name, &creature_ptr->inventory_list[INVEN_BODY], OD_NAME_ONLY);
                    (void)curse_armor(creature_ptr);
                    reward = format(_("%sが破壊された。", "destroying %s"), o_name);
                }
                break;
            default:
                for (int stat = 0; stat < A_MAX; stat++) {
                    (void)dec_stat(creature_ptr, stat, 10 + randint1(15), true);
                }
                reward = _("全能力値が下がった。", "decreasing all stats");
                break;
            }
            break;

        case REW_WRATH:

            msg_format(_("%sの声が轟き渡った:", "The voice of %s thunders:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「死ぬがよい、下僕よ！」", "'Die, mortal!'"));

            take_hit(creature_ptr, DAMAGE_LOSELIFE, creature_ptr->lev * 4, wrath_reason);
            for (int stat = 0; stat < A_MAX; stat++) {
                (void)dec_stat(creature_ptr, stat, 10 + randint1(15), false);
            }
            activate_hi_summon(creature_ptr, creature_ptr->y, creature_ptr->x, false);
            (void)activate_ty_curse(creature_ptr, false, &count);
            if (one_in_(2)) {
                inventory_slot_type slot = INVEN_NONE;

                if (has_melee_weapon(creature_ptr, INVEN_MAIN_HAND)) {
                    slot = INVEN_MAIN_HAND;
                    if (has_melee_weapon(creature_ptr, INVEN_SUB_HAND) && one_in_(2))
                        slot = INVEN_SUB_HAND;
                } else if (has_melee_weapon(creature_ptr, INVEN_SUB_HAND))
                    slot = INVEN_SUB_HAND;

                if (slot)
                    (void)curse_weapon_object(creature_ptr, false, &creature_ptr->inventory_list[slot]);
            }
            if (one_in_(2))
                (void)curse_armor(creature_ptr);
            break;

        case REW_DESTRUCT:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「死と破壊こそ我が喜びなり！」", "'Death and destruction! This pleaseth me!'"));

            (void)destroy_area(creature_ptr, creature_ptr->y, creature_ptr->x, 25, false);
            reward = _("ダンジョンが*破壊*された。", "*destruct*ing dungeon");
            break;

        case REW_GENOCIDE:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「我、汝の敵を抹殺せん！」", "'Let me relieve thee of thine oppressors!'"));
            (void)symbol_genocide(creature_ptr, 0, false);
            reward = _("モンスターが抹殺された。", "genociding monsters");
            break;

        case REW_MASS_GEN:

            msg_format(_("%sの声が響き渡った:", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_print(_("「我、汝の敵を抹殺せん！」", "'Let me relieve thee of thine oppressors!'"));

            (void)mass_genocide(creature_ptr, 0, false);
            reward = _("モンスターが抹殺された。", "genociding nearby monsters");
            break;

        case REW_DISPEL_C:

            msg_format(_("%sの力が敵を攻撃するのを感じた！", "You can feel the power of %s assault your enemies!"), chaos_patrons[creature_ptr->chaos_patron]);
            (void)dispel_monsters(creature_ptr, creature_ptr->lev * 4);
            break;

        case REW_IGNORE:

            msg_format(_("%sはあなたを無視した。", "%s ignores you."), chaos_patrons[creature_ptr->chaos_patron]);
            break;

        case REW_SER_DEMO:

            msg_format(_("%sは褒美として悪魔の使いをよこした！", "%s rewards you with a demonic servant!"), chaos_patrons[creature_ptr->chaos_patron]);

            if (!summon_specific(creature_ptr, -1, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DEMON, PM_FORCE_PET))
                msg_print(_("何も現れなかった...", "Nobody ever turns up..."));
            else
                reward = _("悪魔がペットになった。", "a demonic servant");

            break;

        case REW_SER_MONS:
            msg_format(_("%sは褒美として使いをよこした！", "%s rewards you with a servant!"), chaos_patrons[creature_ptr->chaos_patron]);

            if (!summon_specific(creature_ptr, -1, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_NONE, PM_FORCE_PET))
                msg_print(_("何も現れなかった...", "Nobody ever turns up..."));
            else
                reward = _("モンスターがペットになった。", "a servant");

            break;

        case REW_SER_UNDE:
            msg_format(_("%sは褒美としてアンデッドの使いをよこした。", "%s rewards you with an undead servant!"), chaos_patrons[creature_ptr->chaos_patron]);

            if (!summon_specific(creature_ptr, -1, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_UNDEAD, PM_FORCE_PET))
                msg_print(_("何も現れなかった...", "Nobody ever turns up..."));
            else
                reward = _("アンデッドがペットになった。", "an undead servant");

            break;

        default:
            msg_format(_("%sの声がどもった:", "The voice of %s stammers:"), chaos_patrons[creature_ptr->chaos_patron]);
            msg_format(_("「あー、あー、答えは %d/%d。質問は何？」", "'Uh... uh... the answer's %d/%d, what's the question?'"), type, effect);
        }
    }
    if (reward) {
        exe_write_diary(creature_ptr, DIARY_DESCRIPTION, 0, format(_("パトロンの報酬で%s", "The patron rewarded you with %s."), reward));
    }
}

void admire_from_patron(player_type *creature_ptr)
{
    if ((creature_ptr->pclass == CLASS_CHAOS_WARRIOR) || creature_ptr->muta.has(MUTA::CHAOS_GIFT)) {
        msg_format(_("%sからの声が響いた。", "The voice of %s booms out:"), chaos_patrons[creature_ptr->chaos_patron]);
        msg_print(_("『よくやった、定命の者よ！』", "'Thou art donst well, mortal!'"));
    }
}
