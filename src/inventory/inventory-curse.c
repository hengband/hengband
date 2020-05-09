#include "angband.h"
#include "inventory/inventory-curse.h"
#include "object-flavor.h"
#include "spells-summon.h"
#include "player-damage.h"
#include "player-move.h"
#include "spell/spells2.h"
#include "spell/spells3.h"
#include "files.h"
#include "object-curse.h"
#include "artifact.h"
#include "object-hook.h"
#include "player-effects.h"
#include "object/object-kind.h"

/*!
 * @brief 現在呪いを保持している装備品を一つランダムに探し出す / Choose one of items that have cursed flag
 * @param flag 探し出したい呪いフラグ配列
 * @return 該当の呪いが一つでもあった場合にランダムに選ばれた装備品のオブジェクト構造体参照ポインタを返す。\n
 * 呪いがない場合NULLを返す。
 */
object_type* choose_cursed_obj_name(player_type* player_ptr, BIT_FLAGS flag)
{
    int choices[INVEN_TOTAL - INVEN_RARM];
    int number = 0;
    if (!(player_ptr->cursed & flag))
        return NULL;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type* o_ptr = &player_ptr->inventory_list[i];
        if (o_ptr->curse_flags & flag) {
            choices[number] = i;
            number++;
        } else if ((flag == TRC_ADD_L_CURSE) || (flag == TRC_ADD_H_CURSE) ||
            (flag == TRC_DRAIN_HP) || (flag == TRC_DRAIN_MANA) ||
            (flag == TRC_CALL_ANIMAL) || (flag == TRC_CALL_DEMON) ||
            (flag == TRC_CALL_DRAGON) || (flag == TRC_CALL_UNDEAD) ||
            (flag == TRC_COWARDICE) || (flag == TRC_LOW_MELEE) ||
            (flag == TRC_LOW_AC) || (flag == TRC_LOW_MAGIC) ||
            (flag == TRC_FAST_DIGEST) || (flag == TRC_SLOW_REGEN)) {
            u32b cf = 0L;
            BIT_FLAGS flgs[TR_FLAG_SIZE];
            object_flags(o_ptr, flgs);
            switch (flag) {
            case TRC_ADD_L_CURSE:
                cf = TR_ADD_L_CURSE;
                break;
            case TRC_ADD_H_CURSE:
                cf = TR_ADD_H_CURSE;
                break;
            case TRC_DRAIN_HP:
                cf = TR_DRAIN_HP;
                break;
            case TRC_DRAIN_MANA:
                cf = TR_DRAIN_MANA;
                break;
            case TRC_CALL_ANIMAL:
                cf = TR_CALL_ANIMAL;
                break;
            case TRC_CALL_DEMON:
                cf = TR_CALL_DEMON;
                break;
            case TRC_CALL_DRAGON:
                cf = TR_CALL_DRAGON;
                break;
            case TRC_CALL_UNDEAD:
                cf = TR_CALL_UNDEAD;
                break;
            case TRC_COWARDICE:
                cf = TR_COWARDICE;
                break;
            case TRC_LOW_MELEE:
                cf = TR_LOW_MELEE;
                break;
            case TRC_LOW_AC:
                cf = TR_LOW_AC;
                break;
            case TRC_LOW_MAGIC:
                cf = TR_LOW_MAGIC;
                break;
            case TRC_FAST_DIGEST:
                cf = TR_FAST_DIGEST;
                break;
            case TRC_SLOW_REGEN:
                cf = TR_SLOW_REGEN;
                break;
            default:
                break;
            }

            if (have_flag(flgs, cf)) {
                choices[number] = i;
                number++;
            }
        }
    }

    return &player_ptr->inventory_list[choices[randint0(number)]];
}

/*!
 * @brief 10ゲームターンが進行するごとに装備効果の発動判定を行う処理
 * / Handle curse effects once every 10 game turns
 * @return なし
 */
void process_world_aux_curse(player_type* creature_ptr)
{
    if ((creature_ptr->cursed & TRC_P_FLAG_MASK) && !creature_ptr->phase_out && !creature_ptr->wild_mode) {
        /*
		 * Hack: Uncursed teleporting items (e.g. Trump Weapons)
		 * can actually be useful!
		 */
        if ((creature_ptr->cursed & TRC_TELEPORT_SELF) && one_in_(200)) {
            GAME_TEXT o_name[MAX_NLEN];
            object_type* o_ptr;
            int i_keep = 0, count = 0;
            for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
                BIT_FLAGS flgs[TR_FLAG_SIZE];
                o_ptr = &creature_ptr->inventory_list[i];
                if (!o_ptr->k_idx)
                    continue;

                object_flags(o_ptr, flgs);

                if (have_flag(flgs, TR_TELEPORT)) {
                    /* {.} will stop random teleportation. */
                    if (!o_ptr->inscription || !my_strchr(quark_str(o_ptr->inscription), '.')) {
                        count++;
                        if (one_in_(count))
                            i_keep = i;
                    }
                }
            }

            o_ptr = &creature_ptr->inventory_list[i_keep];
            object_desc(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
            msg_format(_("%sがテレポートの能力を発動させようとしている。", "Your %s is activating teleportation."), o_name);
            if (get_check_strict(_("テレポートしますか？", "Teleport? "), CHECK_OKAY_CANCEL)) {
                disturb(creature_ptr, FALSE, TRUE);
                teleport_player(creature_ptr, 50, TELEPORT_SPONTANEOUS);
            } else {
                msg_format(_("%sに{.}(ピリオド)と銘を刻むと発動を抑制できます。",
                               "You can inscribe {.} on your %s to disable random teleportation. "),
                    o_name);
                disturb(creature_ptr, TRUE, TRUE);
            }
        }

        if ((creature_ptr->cursed & TRC_CHAINSWORD) && one_in_(CHAINSWORD_NOISE)) {
            char noise[1024];
            if (!get_rnd_line(_("chainswd_j.txt", "chainswd.txt"), 0, noise))
                msg_print(noise);
            disturb(creature_ptr, FALSE, FALSE);
        }

        if ((creature_ptr->cursed & TRC_TY_CURSE) && one_in_(TY_CURSE_CHANCE)) {
            int count = 0;
            (void)activate_ty_curse(creature_ptr, FALSE, &count);
        }

        if (creature_ptr->prace != RACE_ANDROID && ((creature_ptr->cursed & TRC_DRAIN_EXP) && one_in_(4))) {
            creature_ptr->exp -= (creature_ptr->lev + 1) / 2;
            if (creature_ptr->exp < 0)
                creature_ptr->exp = 0;
            creature_ptr->max_exp -= (creature_ptr->lev + 1) / 2;
            if (creature_ptr->max_exp < 0)
                creature_ptr->max_exp = 0;
            check_experience(creature_ptr);
        }

        if ((creature_ptr->cursed & TRC_ADD_L_CURSE) && one_in_(2000)) {
            object_type* o_ptr;
            o_ptr = choose_cursed_obj_name(creature_ptr, TRC_ADD_L_CURSE);
            BIT_FLAGS new_curse = get_curse(0, o_ptr);
            if (!(o_ptr->curse_flags & new_curse)) {
                GAME_TEXT o_name[MAX_NLEN];
                object_desc(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
                o_ptr->curse_flags |= new_curse;
                msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
                o_ptr->feeling = FEEL_NONE;
                creature_ptr->update |= (PU_BONUS);
            }
        }

        if ((creature_ptr->cursed & TRC_ADD_H_CURSE) && one_in_(2000)) {
            object_type* o_ptr;
            o_ptr = choose_cursed_obj_name(creature_ptr, TRC_ADD_H_CURSE);
            BIT_FLAGS new_curse = get_curse(1, o_ptr);
            if (!(o_ptr->curse_flags & new_curse)) {
                GAME_TEXT o_name[MAX_NLEN];

                object_desc(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));

                o_ptr->curse_flags |= new_curse;
                msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
                o_ptr->feeling = FEEL_NONE;

                creature_ptr->update |= (PU_BONUS);
            }
        }

        if ((creature_ptr->cursed & TRC_CALL_ANIMAL) && one_in_(2500)) {
            if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_ANIMAL, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET))) {
                GAME_TEXT o_name[MAX_NLEN];
                object_desc(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_CALL_ANIMAL), (OD_OMIT_PREFIX | OD_NAME_ONLY));
                msg_format(_("%sが動物を引き寄せた！", "Your %s has attracted an animal!"), o_name);
                disturb(creature_ptr, FALSE, TRUE);
            }
        }

        if ((creature_ptr->cursed & TRC_CALL_DEMON) && one_in_(1111)) {
            if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DEMON, (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET))) {
                GAME_TEXT o_name[MAX_NLEN];
                object_desc(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_CALL_DEMON), (OD_OMIT_PREFIX | OD_NAME_ONLY));
                msg_format(_("%sが悪魔を引き寄せた！", "Your %s has attracted a demon!"), o_name);
                disturb(creature_ptr, FALSE, TRUE);
            }
        }

        if ((creature_ptr->cursed & TRC_CALL_DRAGON) && one_in_(800)) {
            if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DRAGON,
                    (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET))) {
                GAME_TEXT o_name[MAX_NLEN];
                object_desc(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_CALL_DRAGON), (OD_OMIT_PREFIX | OD_NAME_ONLY));
                msg_format(_("%sがドラゴンを引き寄せた！", "Your %s has attracted an dragon!"), o_name);
                disturb(creature_ptr, FALSE, TRUE);
            }
        }

        if ((creature_ptr->cursed & TRC_CALL_UNDEAD) && one_in_(1111)) {
            if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_UNDEAD,
                    (PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET))) {
                GAME_TEXT o_name[MAX_NLEN];
                object_desc(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_CALL_UNDEAD), (OD_OMIT_PREFIX | OD_NAME_ONLY));
                msg_format(_("%sが死霊を引き寄せた！", "Your %s has attracted an undead!"), o_name);
                disturb(creature_ptr, FALSE, TRUE);
            }
        }

        if ((creature_ptr->cursed & TRC_COWARDICE) && one_in_(1500)) {
            if (!creature_ptr->resist_fear) {
                disturb(creature_ptr, FALSE, TRUE);
                msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
                set_afraid(creature_ptr, creature_ptr->afraid + 13 + randint1(26));
            }
        }

        if ((creature_ptr->cursed & TRC_TELEPORT) && one_in_(200) && !creature_ptr->anti_tele) {
            disturb(creature_ptr, FALSE, TRUE);
            teleport_player(creature_ptr, 40, TELEPORT_PASSIVE);
        }

        if ((creature_ptr->cursed & TRC_DRAIN_HP) && one_in_(666)) {
            GAME_TEXT o_name[MAX_NLEN];
            object_desc(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_DRAIN_HP), (OD_OMIT_PREFIX | OD_NAME_ONLY));
            msg_format(_("%sはあなたの体力を吸収した！", "Your %s drains HP from you!"), o_name);
            take_hit(creature_ptr, DAMAGE_LOSELIFE, MIN(creature_ptr->lev * 2, 100), o_name, -1);
        }

        if ((creature_ptr->cursed & TRC_DRAIN_MANA) && creature_ptr->csp && one_in_(666)) {
            GAME_TEXT o_name[MAX_NLEN];
            object_desc(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_DRAIN_MANA), (OD_OMIT_PREFIX | OD_NAME_ONLY));
            msg_format(_("%sはあなたの魔力を吸収した！", "Your %s drains mana from you!"), o_name);
            creature_ptr->csp -= MIN(creature_ptr->lev, 50);
            if (creature_ptr->csp < 0) {
                creature_ptr->csp = 0;
                creature_ptr->csp_frac = 0;
            }

            creature_ptr->redraw |= PR_MANA;
        }
    }

    if (one_in_(999) && !creature_ptr->anti_magic) {
        object_type* o_ptr = &creature_ptr->inventory_list[INVEN_LITE];
        if (o_ptr->name1 == ART_JUDGE) {
            if (object_is_known(o_ptr))
                msg_print(_("『審判の宝石』はあなたの体力を吸収した！", "The Jewel of Judgement drains life from you!"));
            else
                msg_print(_("なにかがあなたの体力を吸収した！", "Something drains life from you!"));
            take_hit(creature_ptr, DAMAGE_LOSELIFE, MIN(creature_ptr->lev, 50), _("審判の宝石", "the Jewel of Judgement"), -1);
        }
    }
}
