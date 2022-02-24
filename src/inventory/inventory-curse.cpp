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
#include "player-base/player-race.h"
#include "player-info/race-types.h"
#include "player/player-damage.h"
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
const EnumClassFlagGroup<CurseTraitType> TRC_P_FLAG_MASK({ CurseTraitType::TY_CURSE, CurseTraitType::DRAIN_EXP, CurseTraitType::ADD_L_CURSE, CurseTraitType::ADD_H_CURSE, CurseTraitType::CALL_ANIMAL, CurseTraitType::CALL_DEMON,
    CurseTraitType::CALL_DRAGON, CurseTraitType::COWARDICE, CurseTraitType::TELEPORT, CurseTraitType::DRAIN_HP, CurseTraitType::DRAIN_MANA, CurseTraitType::CALL_UNDEAD, CurseTraitType::BERS_RAGE, CurseTraitType::PERSISTENT_CURSE });
const EnumClassFlagGroup<CurseSpecialTraitType> TRCS_P_FLAG_MASK({ CurseSpecialTraitType::TELEPORT_SELF, CurseSpecialTraitType::CHAINSWORD });
}

static bool is_specific_curse(CurseTraitType flag)
{
    switch (flag) {
    case CurseTraitType::ADD_L_CURSE:
    case CurseTraitType::ADD_H_CURSE:
    case CurseTraitType::DRAIN_HP:
    case CurseTraitType::DRAIN_MANA:
    case CurseTraitType::CALL_ANIMAL:
    case CurseTraitType::CALL_DEMON:
    case CurseTraitType::CALL_DRAGON:
    case CurseTraitType::CALL_UNDEAD:
    case CurseTraitType::COWARDICE:
    case CurseTraitType::LOW_MELEE:
    case CurseTraitType::LOW_AC:
    case CurseTraitType::HARD_SPELL:
    case CurseTraitType::FAST_DIGEST:
    case CurseTraitType::SLOW_REGEN:
    case CurseTraitType::BERS_RAGE:
    case CurseTraitType::PERSISTENT_CURSE:
        return true;
    default:
        return false;
    }
}

static void choise_cursed_item(CurseTraitType flag, ObjectType *o_ptr, int *choices, int *number, int item_num)
{
    if (!is_specific_curse(flag))
        return;

    tr_type cf = TR_STR;
    auto flgs = object_flags(o_ptr);
    switch (flag) {
    case CurseTraitType::ADD_L_CURSE:
        cf = TR_ADD_L_CURSE;
        break;
    case CurseTraitType::ADD_H_CURSE:
        cf = TR_ADD_H_CURSE;
        break;
    case CurseTraitType::DRAIN_HP:
        cf = TR_DRAIN_HP;
        break;
    case CurseTraitType::DRAIN_MANA:
        cf = TR_DRAIN_MANA;
        break;
    case CurseTraitType::CALL_ANIMAL:
        cf = TR_CALL_ANIMAL;
        break;
    case CurseTraitType::CALL_DEMON:
        cf = TR_CALL_DEMON;
        break;
    case CurseTraitType::CALL_DRAGON:
        cf = TR_CALL_DRAGON;
        break;
    case CurseTraitType::CALL_UNDEAD:
        cf = TR_CALL_UNDEAD;
        break;
    case CurseTraitType::COWARDICE:
        cf = TR_COWARDICE;
        break;
    case CurseTraitType::LOW_MELEE:
        cf = TR_LOW_MELEE;
        break;
    case CurseTraitType::LOW_AC:
        cf = TR_LOW_AC;
        break;
    case CurseTraitType::HARD_SPELL:
        cf = TR_HARD_SPELL;
        break;
    case CurseTraitType::FAST_DIGEST:
        cf = TR_FAST_DIGEST;
        break;
    case CurseTraitType::SLOW_REGEN:
        cf = TR_SLOW_REGEN;
        break;
    case CurseTraitType::BERS_RAGE:
        cf = TR_BERS_RAGE;
        break;
    case CurseTraitType::PERSISTENT_CURSE:
        cf = TR_PERSISTENT_CURSE;
        break;
    case CurseTraitType::VUL_CURSE:
        cf = TR_VUL_CURSE;
        break;
    default:
        break;
    }

    if (flgs.has_not(cf))
        return;

    choices[*number] = item_num;
    (*number)++;
}

/*!
 * @brief 現在呪いを保持している装備品を一つランダムに探し出す / Choose one of items that have cursed flag
 * @param flag 探し出したい呪いフラグ配列
 * @return 該当の呪いが一つでもあった場合にランダムに選ばれた装備品のオブジェクト構造体参照ポインタを返す。\n
 * 呪いがない場合nullptrを返す。
 */
ObjectType *choose_cursed_obj_name(PlayerType *player_ptr, CurseTraitType flag)
{
    int choices[INVEN_TOTAL - INVEN_MAIN_HAND];
    int number = 0;
    if (player_ptr->cursed.has_not(flag))
        return nullptr;

    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (o_ptr->curse_flags.has(flag)) {
            choices[number] = i;
            number++;
            continue;
        }

        choise_cursed_item(flag, o_ptr, choices, &number, i);
    }

    return &player_ptr->inventory_list[choices[randint0(number)]];
}

/*!
 * @brief 呪われている、トランプエゴ等による装備品由来のテレポートを実行する
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void curse_teleport(PlayerType *player_ptr)
{
    if ((player_ptr->cursed_special.has_not(CurseSpecialTraitType::TELEPORT_SELF)) || !one_in_(200))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    ObjectType *o_ptr;
    int i_keep = 0, count = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        auto flgs = object_flags(o_ptr);

        if (flgs.has_not(TR_TELEPORT))
            continue;

        if (o_ptr->inscription && angband_strchr(quark_str(o_ptr->inscription), '.'))
            continue;

        count++;
        if (one_in_(count))
            i_keep = i;
    }

    o_ptr = &player_ptr->inventory_list[i_keep];
    describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    msg_format(_("%sがテレポートの能力を発動させようとしている。", "Your %s tries to teleport you."), o_name);
    if (get_check_strict(player_ptr, _("テレポートしますか？", "Teleport? "), CHECK_OKAY_CANCEL)) {
        disturb(player_ptr, false, true);
        teleport_player(player_ptr, 50, TELEPORT_SPONTANEOUS);
    } else {
        msg_format(_("%sに{.}(ピリオド)と銘を刻むと発動を抑制できます。", "You can inscribe {.} on your %s to disable random teleportation. "), o_name);
        disturb(player_ptr, true, true);
    }
}

/*!
 * @details 元々呪い効果の発揮ルーチン中にいたので、整合性保持のためここに置いておく
 */
static void occur_chainsword_effect(PlayerType *player_ptr)
{
    if ((player_ptr->cursed_special.has_not(CurseSpecialTraitType::CHAINSWORD)) || !one_in_(CHAINSWORD_NOISE))
        return;

    char noise[1024];
    if (!get_rnd_line(_("chainswd_j.txt", "chainswd.txt"), 0, noise))
        msg_print(noise);
    disturb(player_ptr, false, false);
}

static void curse_drain_exp(PlayerType *player_ptr)
{
    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID) || (player_ptr->cursed.has_not(CurseTraitType::DRAIN_EXP)) || !one_in_(4))
        return;

    player_ptr->exp -= (player_ptr->lev + 1) / 2;
    if (player_ptr->exp < 0)
        player_ptr->exp = 0;

    player_ptr->max_exp -= (player_ptr->lev + 1) / 2;
    if (player_ptr->max_exp < 0)
        player_ptr->max_exp = 0;

    check_experience(player_ptr);
}

static void multiply_low_curse(PlayerType *player_ptr)
{
    if ((player_ptr->cursed.has_not(CurseTraitType::ADD_L_CURSE)) || !one_in_(2000))
        return;

    ObjectType *o_ptr;
    o_ptr = choose_cursed_obj_name(player_ptr, CurseTraitType::ADD_L_CURSE);
    auto new_curse = get_curse(0, o_ptr);
    if (o_ptr->curse_flags.has(new_curse))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    o_ptr->curse_flags.set(new_curse);
    msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
    o_ptr->feeling = FEEL_NONE;
    player_ptr->update |= (PU_BONUS);
}

static void multiply_high_curse(PlayerType *player_ptr)
{
    if ((player_ptr->cursed.has_not(CurseTraitType::ADD_H_CURSE)) || !one_in_(2000))
        return;

    ObjectType *o_ptr;
    o_ptr = choose_cursed_obj_name(player_ptr, CurseTraitType::ADD_H_CURSE);
    auto new_curse = get_curse(1, o_ptr);
    if (o_ptr->curse_flags.has(new_curse))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    o_ptr->curse_flags.set(new_curse);
    msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
    o_ptr->feeling = FEEL_NONE;
    player_ptr->update |= (PU_BONUS);
}

static void persist_curse(PlayerType *player_ptr)
{
    if ((player_ptr->cursed.has_not(CurseTraitType::PERSISTENT_CURSE)) || !one_in_(500))
        return;

    ObjectType *o_ptr;
    o_ptr = choose_cursed_obj_name(player_ptr, CurseTraitType::PERSISTENT_CURSE);
    if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
    o_ptr->curse_flags.set(CurseTraitType::HEAVY_CURSE);
    msg_format(_("悪意に満ちた黒いオーラが%sをとりまいた...", "There is a malignant black aura surrounding your %s..."), o_name);
    o_ptr->feeling = FEEL_NONE;
    player_ptr->update |= (PU_BONUS);
}

static void curse_call_monster(PlayerType *player_ptr)
{
    const int call_type = PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET;
    const int obj_desc_type = OD_OMIT_PREFIX | OD_NAME_ONLY;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (player_ptr->cursed.has(CurseTraitType::CALL_ANIMAL) && one_in_(2500)) {
        if (summon_specific(player_ptr, 0, player_ptr->y, player_ptr->x, floor_ptr->dun_level, SUMMON_ANIMAL, call_type)) {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(player_ptr, o_name, choose_cursed_obj_name(player_ptr, CurseTraitType::CALL_ANIMAL), obj_desc_type);
            msg_format(_("%sが動物を引き寄せた！", "Your %s has attracted an animal!"), o_name);
            disturb(player_ptr, false, true);
        }
    }

    if (player_ptr->cursed.has(CurseTraitType::CALL_DEMON) && one_in_(1111)) {
        if (summon_specific(player_ptr, 0, player_ptr->y, player_ptr->x, floor_ptr->dun_level, SUMMON_DEMON, call_type)) {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(player_ptr, o_name, choose_cursed_obj_name(player_ptr, CurseTraitType::CALL_DEMON), obj_desc_type);
            msg_format(_("%sが悪魔を引き寄せた！", "Your %s has attracted a demon!"), o_name);
            disturb(player_ptr, false, true);
        }
    }

    if (player_ptr->cursed.has(CurseTraitType::CALL_DRAGON) && one_in_(800)) {
        if (summon_specific(player_ptr, 0, player_ptr->y, player_ptr->x, floor_ptr->dun_level, SUMMON_DRAGON, call_type)) {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(player_ptr, o_name, choose_cursed_obj_name(player_ptr, CurseTraitType::CALL_DRAGON), obj_desc_type);
            msg_format(_("%sがドラゴンを引き寄せた！", "Your %s has attracted a dragon!"), o_name);
            disturb(player_ptr, false, true);
        }
    }

    if (player_ptr->cursed.has(CurseTraitType::CALL_UNDEAD) && one_in_(1111)) {
        if (summon_specific(player_ptr, 0, player_ptr->y, player_ptr->x, floor_ptr->dun_level, SUMMON_UNDEAD, call_type)) {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(player_ptr, o_name, choose_cursed_obj_name(player_ptr, CurseTraitType::CALL_UNDEAD), obj_desc_type);
            msg_format(_("%sが死霊を引き寄せた！", "Your %s has attracted an undead!"), o_name);
            disturb(player_ptr, false, true);
        }
    }
}

static void curse_cowardice(PlayerType *player_ptr)
{
    if (player_ptr->cursed.has_not(CurseTraitType::COWARDICE)) {
        return;
    }

    auto *o_ptr = choose_cursed_obj_name(player_ptr, CurseTraitType::COWARDICE);
    auto chance = 1500;
    short duration = 13 + randint1(26);
    if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
        chance = 150;
        duration *= 2;
    }
    
    if (!one_in_(chance) || (has_resist_fear(player_ptr) != 0)) {
        return;
    }

    disturb(player_ptr, false, true);
    msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
    (void)BadStatusSetter(player_ptr).mod_afraidness(duration);
}

/*!
 * @brief 装備による狂戦士化の発作を引き起こす
 * @param player_ptr プレイヤー情報への参照ポインタ
 */
static void curse_berserk_rage(PlayerType *player_ptr)
{
    if (player_ptr->cursed.has_not(CurseTraitType::BERS_RAGE)) {
        return;
    }

    auto *o_ptr = choose_cursed_obj_name(player_ptr, CurseTraitType::BERS_RAGE);
    auto chance = 1500;
    short duration = 10 + randint1(player_ptr->lev);

    if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
        chance = 150;
        duration *= 2;
    }
    
    if (!one_in_(chance)) {
        return;
    }

    disturb(player_ptr, false, true);
    msg_print(_("ウガァァア！", "RAAAAGHH!"));
    msg_print(_("激怒の発作に襲われた！", "You feel a fit of rage coming over you!"));
    (void)set_shero(player_ptr, duration, false);
    (void)BadStatusSetter(player_ptr).afraidness(0);
}

static void curse_drain_hp(PlayerType *player_ptr)
{
    if ((player_ptr->cursed.has_not(CurseTraitType::DRAIN_HP)) || !one_in_(666))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, choose_cursed_obj_name(player_ptr, CurseTraitType::DRAIN_HP), (OD_OMIT_PREFIX | OD_NAME_ONLY));
    msg_format(_("%sはあなたの体力を吸収した！", "Your %s drains HP from you!"), o_name);
    take_hit(player_ptr, DAMAGE_LOSELIFE, std::min(player_ptr->lev * 2, 100), o_name);
}

static void curse_drain_mp(PlayerType *player_ptr)
{
    if ((player_ptr->cursed.has_not(CurseTraitType::DRAIN_MANA)) || (player_ptr->csp == 0) || !one_in_(666))
        return;

    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(player_ptr, o_name, choose_cursed_obj_name(player_ptr, CurseTraitType::DRAIN_MANA), (OD_OMIT_PREFIX | OD_NAME_ONLY));
    msg_format(_("%sはあなたの魔力を吸収した！", "Your %s drains mana from you!"), o_name);
    player_ptr->csp -= std::min<short>(player_ptr->lev, 50);
    if (player_ptr->csp < 0) {
        player_ptr->csp = 0;
        player_ptr->csp_frac = 0;
    }

    player_ptr->redraw |= PR_MANA;
}

static void occur_curse_effects(PlayerType *player_ptr)
{
    if ((player_ptr->cursed.has_none_of(TRC_P_FLAG_MASK) && player_ptr->cursed_special.has_none_of(TRCS_P_FLAG_MASK)) || player_ptr->phase_out
        || player_ptr->wild_mode)
        return;

    curse_teleport(player_ptr);
    occur_chainsword_effect(player_ptr);
    if (player_ptr->cursed.has(CurseTraitType::TY_CURSE) && one_in_(TY_CURSE_CHANCE)) {
        int count = 0;
        (void)activate_ty_curse(player_ptr, false, &count);
    }

    curse_drain_exp(player_ptr);
    multiply_low_curse(player_ptr);
    multiply_high_curse(player_ptr);
    persist_curse(player_ptr);
    curse_call_monster(player_ptr);
    curse_cowardice(player_ptr);
    curse_berserk_rage(player_ptr);
    if (player_ptr->cursed.has(CurseTraitType::TELEPORT) && one_in_(200) && !player_ptr->anti_tele) {
        disturb(player_ptr, false, true);
        teleport_player(player_ptr, 40, TELEPORT_PASSIVE);
    }

    curse_drain_hp(player_ptr);
    curse_drain_mp(player_ptr);
}

/*!
 * @brief 10ゲームターンが進行するごとに装備効果の発動判定を行う処理
 * / Handle curse effects once every 10 game turns
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void execute_cursed_items_effect(PlayerType *player_ptr)
{
    occur_curse_effects(player_ptr);
    if (!one_in_(999) || player_ptr->anti_magic || (one_in_(2) && has_resist_curse(player_ptr)))
        return;

    auto *o_ptr = &player_ptr->inventory_list[INVEN_LITE];
    if (o_ptr->fixed_artifact_idx != ART_JUDGE)
        return;

    if (o_ptr->is_known())
        msg_print(_("『審判の宝石』はあなたの体力を吸収した！", "The Jewel of Judgement drains life from you!"));
    else
        msg_print(_("なにかがあなたの体力を吸収した！", "Something drains life from you!"));

    take_hit(player_ptr, DAMAGE_LOSELIFE, std::min<short>(player_ptr->lev, 50), _("審判の宝石", "the Jewel of Judgement"));
}
