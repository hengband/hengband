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
#include "player/player-status.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-teleport.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

namespace {
const EnumClassFlagGroup<TRC> TRC_P_FLAG_MASK({ TRC::TY_CURSE, TRC::DRAIN_EXP, TRC::ADD_L_CURSE, TRC::ADD_H_CURSE, TRC::CALL_ANIMAL, TRC::CALL_DEMON, TRC::CALL_DRAGON,
    TRC::COWARDICE, TRC::TELEPORT, TRC::DRAIN_HP, TRC::DRAIN_MANA, TRC::CALL_UNDEAD, TRC::BERS_RAGE });
const EnumClassFlagGroup<TRCS> TRCS_P_FLAG_MASK({ TRCS::TELEPORT_SELF, TRCS::CHAINSWORD });
}

static bool is_specific_curse(TRC flag)
{
    switch (flag) {
    case TRC::ADD_L_CURSE:
    case TRC::ADD_H_CURSE:
    case TRC::DRAIN_HP:
    case TRC::DRAIN_MANA:
    case TRC::CALL_ANIMAL:
    case TRC::CALL_DEMON:
    case TRC::CALL_DRAGON:
    case TRC::CALL_UNDEAD:
    case TRC::COWARDICE:
    case TRC::LOW_MELEE:
    case TRC::LOW_AC:
    case TRC::HARD_SPELL:
    case TRC::FAST_DIGEST:
    case TRC::SLOW_REGEN:
    case TRC::BERS_RAGE:
        return true;
    default:
        return false;
    }
}

static void choise_cursed_item(player_type *creature_ptr, TRC flag, object_type *o_ptr, int *choices, int *number, int item_num)
{
    if (!is_specific_curse(flag))
        return;

    tr_type cf = TR_STR;
    TrFlags flgs;
    object_flags(creature_ptr, o_ptr, flgs);
    switch (flag) {
    case TRC::ADD_L_CURSE:
        cf = TR_ADD_L_CURSE;
        break;
    case TRC::ADD_H_CURSE:
        cf = TR_ADD_H_CURSE;
        break;
    case TRC::DRAIN_HP:
        cf = TR_DRAIN_HP;
        break;
    case TRC::DRAIN_MANA:
        cf = TR_DRAIN_MANA;
        break;
    case TRC::CALL_ANIMAL:
        cf = TR_CALL_ANIMAL;
        break;
    case TRC::CALL_DEMON:
        cf = TR_CALL_DEMON;
        break;
    case TRC::CALL_DRAGON:
        cf = TR_CALL_DRAGON;
        break;
    case TRC::CALL_UNDEAD:
        cf = TR_CALL_UNDEAD;
        break;
    case TRC::COWARDICE:
        cf = TR_COWARDICE;
        break;
    case TRC::LOW_MELEE:
        cf = TR_LOW_MELEE;
        break;
    case TRC::LOW_AC:
        cf = TR_LOW_AC;
        break;
    case TRC::HARD_SPELL:
        cf = TR_HARD_SPELL;
        break;
    case TRC::FAST_DIGEST:
        cf = TR_FAST_DIGEST;
        break;
    case TRC::SLOW_REGEN:
        cf = TR_SLOW_REGEN;
        break;
    case TRC::BERS_RAGE:
        cf = TR_BERS_RAGE;
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
object_type *choose_cursed_obj_name(player_type *creature_ptr, TRC flag)
{
    int choices[INVEN_TOTAL - INVEN_MAIN_HAND];
    int number = 0;
    if (creature_ptr->cursed.has_not(flag))
        return NULL;

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &creature_ptr->inventory_list[i];
        if (o_ptr->curse_flags.has(flag)) {
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
 */
static void curse_teleport(player_type *creature_ptr)
{
    if ((creature_ptr->cursed_special.has_not(TRCS::TELEPORT_SELF)) || !one_in_(200))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    object_type *o_ptr;
    int i_keep = 0, count = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        TrFlags flgs;
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
    msg_format(_("%sがテレポートの能力を発動させようとしている。", "Your %s tries to teleport you."), o_name);
    if (get_check_strict(creature_ptr, _("テレポートしますか？", "Teleport? "), CHECK_OKAY_CANCEL)) {
        disturb(creature_ptr, false, true);
        teleport_player(creature_ptr, 50, TELEPORT_SPONTANEOUS);
    } else {
        msg_format(_("%sに{.}(ピリオド)と銘を刻むと発動を抑制できます。", "You can inscribe {.} on your %s to disable random teleportation. "), o_name);
        disturb(creature_ptr, true, true);
    }
}

/*!
 * @details 元々呪い効果の発揮ルーチン中にいたので、整合性保持のためここに置いておく
 */
static void occur_chainsword_effect(player_type *creature_ptr)
{
    if ((creature_ptr->cursed_special.has_not(TRCS::CHAINSWORD)) || !one_in_(CHAINSWORD_NOISE))
        return;

    char noise[1024];
    if (!get_rnd_line(_("chainswd_j.txt", "chainswd.txt"), 0, noise))
        msg_print(noise);
    disturb(creature_ptr, false, false);
}

static void curse_drain_exp(player_type *creature_ptr)
{
    if ((creature_ptr->prace == player_race_type::ANDROID) || (creature_ptr->cursed.has_not(TRC::DRAIN_EXP)) || !one_in_(4))
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
    if ((creature_ptr->cursed.has_not(TRC::ADD_L_CURSE)) || !one_in_(2000))
        return;

    object_type *o_ptr;
    o_ptr = choose_cursed_obj_name(creature_ptr, TRC::ADD_L_CURSE);
    auto new_curse = get_curse(creature_ptr, 0, o_ptr);
    if (o_ptr->curse_flags.has(new_curse))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    o_ptr->curse_flags.set(new_curse);
    msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
    o_ptr->feeling = FEEL_NONE;
    creature_ptr->update |= (PU_BONUS);
}

static void multiply_high_curse(player_type *creature_ptr)
{
    if ((creature_ptr->cursed.has_not(TRC::ADD_H_CURSE)) || !one_in_(2000))
        return;

    object_type *o_ptr;
    o_ptr = choose_cursed_obj_name(creature_ptr, TRC::ADD_H_CURSE);
    auto new_curse = get_curse(creature_ptr, 1, o_ptr);
    if (o_ptr->curse_flags.has(new_curse))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(creature_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    o_ptr->curse_flags.set(new_curse);
    msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
    o_ptr->feeling = FEEL_NONE;
    creature_ptr->update |= (PU_BONUS);
}

static void curse_call_monster(player_type *creature_ptr)
{
    const int call_type = PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET;
    const int obj_desc_type = OD_OMIT_PREFIX | OD_NAME_ONLY;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (creature_ptr->cursed.has(TRC::CALL_ANIMAL) && one_in_(2500)) {
        if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, floor_ptr->dun_level, SUMMON_ANIMAL, call_type)) {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC::CALL_ANIMAL), obj_desc_type);
            msg_format(_("%sが動物を引き寄せた！", "Your %s has attracted an animal!"), o_name);
            disturb(creature_ptr, false, true);
        }
    }

    if (creature_ptr->cursed.has(TRC::CALL_DEMON) && one_in_(1111)) {
        if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, floor_ptr->dun_level, SUMMON_DEMON, call_type)) {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC::CALL_DEMON), obj_desc_type);
            msg_format(_("%sが悪魔を引き寄せた！", "Your %s has attracted a demon!"), o_name);
            disturb(creature_ptr, false, true);
        }
    }

    if (creature_ptr->cursed.has(TRC::CALL_DRAGON) && one_in_(800)) {
        if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, floor_ptr->dun_level, SUMMON_DRAGON, call_type)) {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC::CALL_DRAGON), obj_desc_type);
            msg_format(_("%sがドラゴンを引き寄せた！", "Your %s has attracted a dragon!"), o_name);
            disturb(creature_ptr, false, true);
        }
    }

    if (creature_ptr->cursed.has(TRC::CALL_UNDEAD) && one_in_(1111)) {
        if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, floor_ptr->dun_level, SUMMON_UNDEAD, call_type)) {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC::CALL_UNDEAD), obj_desc_type);
            msg_format(_("%sが死霊を引き寄せた！", "Your %s has attracted an undead!"), o_name);
            disturb(creature_ptr, false, true);
        }
    }
}

static void curse_cowardice(player_type *creature_ptr)
{
    if ((creature_ptr->cursed.has_not(TRC::COWARDICE)) || !one_in_(1500))
        return;

    if (has_resist_fear(creature_ptr))
        return;

    disturb(creature_ptr, false, true);
    msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
    set_afraid(creature_ptr, creature_ptr->afraid + 13 + randint1(26));
}

/*!
 * @brief 装備による狂戦士化の発作を引き起こす
 * @param creature_ptr プレイヤー情報への参照ポインタ
 */
static void curse_berserk_rage(player_type *creature_ptr)
{
    if ((creature_ptr->cursed.has_not(TRC::BERS_RAGE)) || !one_in_(1500))
        return;

    disturb(creature_ptr, false, true);
    msg_print(_("ウガァァア！", "RAAAAGHH!"));
    msg_print(_("激怒の発作に襲われた！", "You feel a fit of rage coming over you!"));
    (void)set_shero(creature_ptr, 10 + randint1(creature_ptr->lev), false);
    (void)set_afraid(creature_ptr, 0);
}

static void curse_drain_hp(player_type *creature_ptr)
{
    if ((creature_ptr->cursed.has_not(TRC::DRAIN_HP)) || !one_in_(666))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC::DRAIN_HP), (OD_OMIT_PREFIX | OD_NAME_ONLY));
    msg_format(_("%sはあなたの体力を吸収した！", "Your %s drains HP from you!"), o_name);
    take_hit(creature_ptr, DAMAGE_LOSELIFE, MIN(creature_ptr->lev * 2, 100), o_name);
}

static void curse_drain_mp(player_type *creature_ptr)
{
    if ((creature_ptr->cursed.has_not(TRC::DRAIN_MANA)) || (creature_ptr->csp == 0) || !one_in_(666))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(creature_ptr, o_name, choose_cursed_obj_name(creature_ptr, TRC::DRAIN_MANA), (OD_OMIT_PREFIX | OD_NAME_ONLY));
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
    if ((creature_ptr->cursed.has_none_of(TRC_P_FLAG_MASK) && creature_ptr->cursed_special.has_none_of(TRCS_P_FLAG_MASK)) || creature_ptr->phase_out || creature_ptr->wild_mode)
        return;

    curse_teleport(creature_ptr);
    occur_chainsword_effect(creature_ptr);
    if (creature_ptr->cursed.has(TRC::TY_CURSE) && one_in_(TY_CURSE_CHANCE)) {
        int count = 0;
        (void)activate_ty_curse(creature_ptr, false, &count);
    }

    curse_drain_exp(creature_ptr);
    multiply_low_curse(creature_ptr);
    multiply_high_curse(creature_ptr);
    curse_call_monster(creature_ptr);
    curse_cowardice(creature_ptr);
    curse_berserk_rage(creature_ptr);
    if (creature_ptr->cursed.has(TRC::TELEPORT) && one_in_(200) && !creature_ptr->anti_tele) {
        disturb(creature_ptr, false, true);
        teleport_player(creature_ptr, 40, TELEPORT_PASSIVE);
    }

    curse_drain_hp(creature_ptr);
    curse_drain_mp(creature_ptr);
}

/*!
 * @brief 10ゲームターンが進行するごとに装備効果の発動判定を行う処理
 * / Handle curse effects once every 10 game turns
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
void execute_cursed_items_effect(player_type *creature_ptr)
{
    occur_curse_effects(creature_ptr);
    if (!one_in_(999) || creature_ptr->anti_magic || (one_in_(2) && has_resist_curse(creature_ptr)))
        return;

    object_type *o_ptr = &creature_ptr->inventory_list[INVEN_LITE];
    if (o_ptr->name1 != ART_JUDGE)
        return;

    if (object_is_known(o_ptr))
        msg_print(_("『審判の宝石』はあなたの体力を吸収した！", "The Jewel of Judgement drains life from you!"));
    else
        msg_print(_("なにかがあなたの体力を吸収した！", "Something drains life from you!"));

    take_hit(creature_ptr, DAMAGE_LOSELIFE, MIN(creature_ptr->lev, 50), _("審判の宝石", "the Jewel of Judgement"));
}
