#include "inventory/inventory-curse.h"
#include "artifact/fixed-art-types.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "object-enchant/item-feeling.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/object-flags.h"
#include "perception/object-perception.h"
#include "player/player-damage.h"
#include "player/player-race-types.h"
#include "player/player-status-flags.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-teleport.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

#define TRC_P_FLAG_MASK                                                                                                                                        \
    (TRC_TELEPORT_SELF | TRC_CHAINSWORD | TRC_TY_CURSE | TRC_DRAIN_EXP | TRC_ADD_L_CURSE | TRC_ADD_H_CURSE | TRC_CALL_ANIMAL | TRC_CALL_DEMON                  \
        | TRC_CALL_DRAGON | TRC_COWARDICE | TRC_TELEPORT | TRC_DRAIN_HP | TRC_DRAIN_MANA | TRC_CALL_UNDEAD)

static bool is_specific_curse(BIT_FLAGS flag)
{
    return (flag == TRC_ADD_L_CURSE) || (flag == TRC_ADD_H_CURSE) || (flag == TRC_DRAIN_HP) || (flag == TRC_DRAIN_MANA) || (flag == TRC_CALL_ANIMAL)
        || (flag == TRC_CALL_DEMON) || (flag == TRC_CALL_DRAGON) || (flag == TRC_CALL_UNDEAD) || (flag == TRC_COWARDICE) || (flag == TRC_LOW_MELEE)
        || (flag == TRC_LOW_AC) || (flag == TRC_LOW_MAGIC) || (flag == TRC_FAST_DIGEST) || (flag == TRC_SLOW_REGEN);
}

static void choise_cursed_item(player_type *creature_ptr, BIT_FLAGS flag, object_type *o_ptr, int *choices, int *number, int item_num)
{
    if (!is_specific_curse(flag))
        return;

    tr_type cf = 0;
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_flags(creature_ptr, o_ptr, flgs);
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

    if (!has_flag(flgs, (long)cf))
        return;

    choices[*number] = item_num;
    (*number)++;
}

/*!
 * @brief 現在呪いを保持している装備品を一つランダムに探し出す / Choose one of items that have cursed flag
 * @param flag 探し出したい呪いフラグ配列
 * @return 該当の呪いが一つでもあった場合にランダムに選ばれた装備品のオブジェクト構造体参照ポインタを返す。\n
 * 呪いがない場合NULLを返す。
 */
object_type *choose_cursed_obj_name(player_type *creature_ptr, BIT_FLAGS flag)
{
    int choices[INVEN_TOTAL - INVEN_RARM];
    int number = 0;
    if (!(creature_ptr->cursed & flag))
        return NULL;

    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &creature_ptr->inventory_list[i];
        if (o_ptr->curse_flags & flag) {
            choices[number] = i;
            number++;
            continue;
        }

        choise_cursed_item(creature_ptr, flag, o_ptr, choices, &number, i);
    }

    return &creature_ptr->inventory_list[choices[randint0(number)]];
}

/*!
 * @brief 呪われている、トランプエゴ等による装備品由来のテレポートを実行する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void curse_teleport(player_type *creature_ptr)
{
    if (((creature_ptr->cursed & TRC_TELEPORT_SELF) == 0) || !one_in_(200))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    object_type *o_ptr;
    int i_keep = 0, count = 0;
    for (inventory_slot_type i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        BIT_FLAGS flgs[TR_FLAG_SIZE];
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (!has_flag(flgs, TR_TELEPORT))
            continue;

        if (o_ptr->inscription && angband_strchr(quark_str(o_ptr->inscription), '.'))
            continue;

        count++;
        if (one_in_(count))
            i_keep = i;
    }

    o_ptr = &creature_ptr->inventory_list[i_keep];
    describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    msg_format(_("%sがテレポートの能力を発動させようとしている。", "Your %s is activating teleportation."), o_name);
    if (get_check_strict(creature_ptr, _("テレポートしますか？", "Teleport? "), CHECK_OKAY_CANCEL)) {
        disturb(creature_ptr, FALSE, TRUE);
        teleport_player(creature_ptr, 50, TELEPORT_SPONTANEOUS);
    } else {
        msg_format(_("%sに{.}(ピリオド)と銘を刻むと発動を抑制できます。", "You can inscribe {.} on your %s to disable random teleportation. "), o_name);
        disturb(creature_ptr, TRUE, TRUE);
    }
}

/*!
 * @details 元々呪い効果の発揮ルーチン中にいたので、整合性保持のためここに置いておく
 */
static void occur_chainsword_effect(player_type *creature_ptr)
{
    if (((creature_ptr->cursed & TRC_CHAINSWORD) == 0) || !one_in_(CHAINSWORD_NOISE))
        return;

    char noise[1024];
    if (!get_rnd_line(_("chainswd_j.txt", "chainswd.txt"), 0, noise))
        msg_print(noise);
    disturb(creature_ptr, FALSE, FALSE);
}

static void curse_drain_exp(player_type *creature_ptr)
{
    if ((creature_ptr->prace == RACE_ANDROID) || ((creature_ptr->cursed & TRC_DRAIN_EXP) == 0) || !one_in_(4))
        return;

    creature_ptr->exp -= (creature_ptr->lev + 1) / 2;
    if (creature_ptr->exp < 0)
        creature_ptr->exp = 0;

    creature_ptr->max_exp -= (creature_ptr->lev + 1) / 2;
    if (creature_ptr->max_exp < 0)
        creature_ptr->max_exp = 0;

    check_experience(creature_ptr);
}

static void multiply_low_curse(player_type *creature_ptr)
{
    if (((creature_ptr->cursed & TRC_ADD_L_CURSE) == 0) || !one_in_(2000))
        return;

    object_type *o_ptr;
    o_ptr = choose_cursed_obj_name(creature_ptr, TRC_ADD_L_CURSE);
    BIT_FLAGS new_curse = get_curse(creature_ptr, 0, o_ptr);
    if ((o_ptr->curse_flags & new_curse))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    o_ptr->curse_flags |= new_curse;
    msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
    o_ptr->feeling = FEEL_NONE;
    creature_ptr->update |= (PU_BONUS);
}

static void multiply_high_curse(player_type *creature_ptr)
{
    if (((creature_ptr->cursed & TRC_ADD_H_CURSE) == 0) || !one_in_(2000))
        return;

    object_type *o_ptr;
    o_ptr = choose_cursed_obj_name(creature_ptr, TRC_ADD_H_CURSE);
    BIT_FLAGS new_curse = get_curse(creature_ptr, 1, o_ptr);
    if ((o_ptr->curse_flags & new_curse))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    o_ptr->curse_flags |= new_curse;
    msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
    o_ptr->feeling = FEEL_NONE;
    creature_ptr->update |= (PU_BONUS);
}

static void curse_call_monster(player_type *creature_ptr)
{
    const int call_type = PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET;
    const int obj_desc_type = OD_OMIT_PREFIX | OD_NAME_ONLY;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if ((creature_ptr->cursed & TRC_CALL_ANIMAL) && one_in_(2500)) {
        if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, floor_ptr->dun_level, SUMMON_ANIMAL, call_type)) {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_CALL_ANIMAL), obj_desc_type);
            msg_format(_("%sが動物を引き寄せた！", "Your %s has attracted an animal!"), o_name);
            disturb(creature_ptr, FALSE, TRUE);
        }
    }

    if ((creature_ptr->cursed & TRC_CALL_DEMON) && one_in_(1111)) {
        if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, floor_ptr->dun_level, SUMMON_DEMON, call_type)) {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_CALL_DEMON), obj_desc_type);
            msg_format(_("%sが悪魔を引き寄せた！", "Your %s has attracted a demon!"), o_name);
            disturb(creature_ptr, FALSE, TRUE);
        }
    }

    if ((creature_ptr->cursed & TRC_CALL_DRAGON) && one_in_(800)) {
        if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, floor_ptr->dun_level, SUMMON_DRAGON, call_type)) {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_CALL_DRAGON), obj_desc_type);
            msg_format(_("%sがドラゴンを引き寄せた！", "Your %s has attracted an dragon!"), o_name);
            disturb(creature_ptr, FALSE, TRUE);
        }
    }

    if ((creature_ptr->cursed & TRC_CALL_UNDEAD) && one_in_(1111)) {
        if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, floor_ptr->dun_level, SUMMON_UNDEAD, call_type)) {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_CALL_UNDEAD), obj_desc_type);
            msg_format(_("%sが死霊を引き寄せた！", "Your %s has attracted an undead!"), o_name);
            disturb(creature_ptr, FALSE, TRUE);
        }
    }
}

static void curse_cowardice(player_type *creature_ptr)
{
    if (((creature_ptr->cursed & TRC_COWARDICE) == 0) || !one_in_(1500))
        return;

    if (has_resist_fear(creature_ptr))
        return;

    disturb(creature_ptr, FALSE, TRUE);
    msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
    set_afraid(creature_ptr, creature_ptr->afraid + 13 + randint1(26));
}

static void curse_drain_hp(player_type *creature_ptr)
{
    if (((creature_ptr->cursed & TRC_DRAIN_HP) == 0) || !one_in_(666))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_DRAIN_HP), (OD_OMIT_PREFIX | OD_NAME_ONLY));
    msg_format(_("%sはあなたの体力を吸収した！", "Your %s drains HP from you!"), o_name);
    take_hit(creature_ptr, DAMAGE_LOSELIFE, MIN(creature_ptr->lev * 2, 100), o_name, -1);
}

static void curse_drain_mp(player_type *creature_ptr)
{
    if (((creature_ptr->cursed & TRC_DRAIN_MANA) == 0) || (creature_ptr->csp == 0) || !one_in_(666))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC_DRAIN_MANA), (OD_OMIT_PREFIX | OD_NAME_ONLY));
    msg_format(_("%sはあなたの魔力を吸収した！", "Your %s drains mana from you!"), o_name);
    creature_ptr->csp -= MIN(creature_ptr->lev, 50);
    if (creature_ptr->csp < 0) {
        creature_ptr->csp = 0;
        creature_ptr->csp_frac = 0;
    }

    creature_ptr->redraw |= PR_MANA;
}

static void occur_curse_effects(player_type *creature_ptr)
{
    if (((creature_ptr->cursed & TRC_P_FLAG_MASK) == 0) || creature_ptr->phase_out || creature_ptr->wild_mode)
        return;

    curse_teleport(creature_ptr);
    occur_chainsword_effect(creature_ptr);
    if ((creature_ptr->cursed & TRC_TY_CURSE) && one_in_(TY_CURSE_CHANCE)) {
        int count = 0;
        (void)activate_ty_curse(creature_ptr, FALSE, &count);
    }

    curse_drain_exp(creature_ptr);
    multiply_low_curse(creature_ptr);
    multiply_high_curse(creature_ptr);
    curse_call_monster(creature_ptr);
    curse_cowardice(creature_ptr);
    if ((creature_ptr->cursed & TRC_TELEPORT) && one_in_(200) && !creature_ptr->anti_tele) {
        disturb(creature_ptr, FALSE, TRUE);
        teleport_player(creature_ptr, 40, TELEPORT_PASSIVE);
    }

    curse_drain_hp(creature_ptr);
    curse_drain_mp(creature_ptr);
}

/*!
 * @brief 10ゲームターンが進行するごとに装備効果の発動判定を行う処理
 * / Handle curse effects once every 10 game turns
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void execute_cursed_items_effect(player_type *creature_ptr)
{
    occur_curse_effects(creature_ptr);
    if (!one_in_(999) || creature_ptr->anti_magic)
        return;

    object_type *o_ptr = &creature_ptr->inventory_list[INVEN_LITE];
    if (o_ptr->name1 != ART_JUDGE)
        return;

    if (object_is_known(o_ptr))
        msg_print(_("『審判の宝石』はあなたの体力を吸収した！", "The Jewel of Judgement drains life from you!"));
    else
        msg_print(_("なにかがあなたの体力を吸収した！", "Something drains life from you!"));

    take_hit(creature_ptr, DAMAGE_LOSELIFE, MIN(creature_ptr->lev, 50), _("審判の宝石", "the Jewel of Judgement"), -1);
}
