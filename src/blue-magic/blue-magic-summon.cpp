/*!
 * @file blue-magic-summon.cpp
 * @brief 青魔法の召喚系スペル定義
 */

#include "blue-magic/blue-magic-summon.h"
#include "blue-magic/blue-magic-util.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

bool cast_blue_summon_kin(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("援軍を召喚した。", "You summon one of your kin."));
    for (int k = 0; k < 1; k++) {
        if (summon_kin_player(player_ptr, bmc_ptr->summon_lev, player_ptr->y, player_ptr->x, (bmc_ptr->pet ? PM_FORCE_PET : PM_NONE))) {
            if (!bmc_ptr->pet)
                msg_print(_("召喚された仲間は怒っている！", "The summoned companion is angry!"));
        } else {
            bmc_ptr->no_trump = true;
        }
    }

    return true;
}

bool cast_blue_summon_cyber(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("サイバーデーモンを召喚した！", "You summon a Cyberdemon!"));
    for (int k = 0; k < 1; k++) {
        if (summon_specific(player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_CYBER, bmc_ptr->p_mode)) {
            if (!bmc_ptr->pet)
                msg_print(_("召喚されたサイバーデーモンは怒っている！", "The summoned Cyberdemon is angry!"));
        } else {
            bmc_ptr->no_trump = true;
        }
    }

    return true;
}

bool cast_blue_summon_monster(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("仲間を召喚した。", "You summon help."));
    for (int k = 0; k < 1; k++) {
        if (summon_specific(player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_NONE, bmc_ptr->p_mode)) {
            if (!bmc_ptr->pet)
                msg_print(_("召喚されたモンスターは怒っている！", "The summoned monster is angry!"));
        } else {
            bmc_ptr->no_trump = true;
        }
    }

    return true;
}

bool cast_blue_summon_monsters(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("モンスターを召喚した！", "You summon monsters!"));
    for (int k = 0; k < bmc_ptr->plev / 15 + 2; k++) {
        if (summon_specific(player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_NONE, (bmc_ptr->p_mode | bmc_ptr->u_mode))) {
            if (!bmc_ptr->pet)
                msg_print(_("召喚されたモンスターは怒っている！", "The summoned monsters are angry!"));
        } else {
            bmc_ptr->no_trump = true;
        }
    }

    return true;
}

bool cast_blue_summon_ant(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("アリを召喚した。", "You summon ants."));
    if (summon_specific(
            player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_ANT, (PM_ALLOW_GROUP | bmc_ptr->p_mode))) {
        if (!bmc_ptr->pet)
            msg_print(_("召喚されたアリは怒っている！", "The summoned ants are angry!"));
    } else {
        bmc_ptr->no_trump = true;
    }

    return true;
}

bool cast_blue_summon_spider(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("蜘蛛を召喚した。", "You summon spiders."));
    if (summon_specific(
            player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_SPIDER, (PM_ALLOW_GROUP | bmc_ptr->p_mode))) {
        if (!bmc_ptr->pet)
            msg_print(_("召喚された蜘蛛は怒っている！", "The summoned spiders are angry!"));
    } else {
        bmc_ptr->no_trump = true;
    }

    return true;
}

bool cast_blue_summon_hound(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("ハウンドを召喚した。", "You summon hounds."));
    if (summon_specific(
            player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_HOUND, (PM_ALLOW_GROUP | bmc_ptr->p_mode))) {
        if (!bmc_ptr->pet)
            msg_print(_("召喚されたハウンドは怒っている！", "The summoned hounds are angry!"));
    } else {
        bmc_ptr->no_trump = true;
    }

    return true;
}

bool cast_blue_summon_hydra(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("ヒドラを召喚した。", "You summon a hydras."));
    if (summon_specific(
            player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_HYDRA, (bmc_ptr->g_mode | bmc_ptr->p_mode))) {
        if (!bmc_ptr->pet)
            msg_print(_("召喚されたヒドラは怒っている！", "The summoned hydras are angry!"));
    } else {
        bmc_ptr->no_trump = true;
    }

    return true;
}

bool cast_blue_summon_angel(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("天使を召喚した！", "You summon an angel!"));
    if (summon_specific(
            player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_ANGEL, (bmc_ptr->g_mode | bmc_ptr->p_mode))) {
        if (!bmc_ptr->pet)
            msg_print(_("召喚された天使は怒っている！", "The summoned angel is angry!"));
    } else {
        bmc_ptr->no_trump = true;
    }

    return true;
}

bool cast_blue_summon_demon(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("混沌の宮廷から悪魔を召喚した！", "You summon a demon from the Courts of Chaos!"));
    if (summon_specific(
            player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_DEMON, (bmc_ptr->g_mode | bmc_ptr->p_mode))) {
        if (!bmc_ptr->pet)
            msg_print(_("召喚されたデーモンは怒っている！", "The summoned demon is angry!"));
    } else {
        bmc_ptr->no_trump = true;
    }

    return true;
}

bool cast_blue_summon_undead(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("アンデッドの強敵を召喚した！", "You summon an undead adversary!"));
    if (summon_specific(
            player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_UNDEAD, (bmc_ptr->g_mode | bmc_ptr->p_mode))) {
        if (!bmc_ptr->pet)
            msg_print(_("召喚されたアンデッドは怒っている！", "The summoned undead is angry!"));
    } else {
        bmc_ptr->no_trump = true;
    }

    return true;
}

bool cast_blue_summon_dragon(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("ドラゴンを召喚した！", "You summon a dragon!"));
    if (summon_specific(
            player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_DRAGON, (bmc_ptr->g_mode | bmc_ptr->p_mode))) {
        if (!bmc_ptr->pet)
            msg_print(_("召喚されたドラゴンは怒っている！", "The summoned dragon is angry!"));
    } else {
        bmc_ptr->no_trump = true;
    }

    return true;
}

bool cast_blue_summon_high_undead(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("強力なアンデッドを召喚した！", "You summon a greater undead!"));
    if (summon_specific(player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_HI_UNDEAD,
            (bmc_ptr->g_mode | bmc_ptr->p_mode | bmc_ptr->u_mode))) {
        if (!bmc_ptr->pet)
            msg_print(_("召喚された上級アンデッドは怒っている！", "The summoned greater undead is angry!"));
    } else {
        bmc_ptr->no_trump = true;
    }

    return true;
}

bool cast_blue_summon_high_dragon(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("古代ドラゴンを召喚した！", "You summon an ancient dragon!"));
    if (summon_specific(player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_HI_DRAGON,
            (bmc_ptr->g_mode | bmc_ptr->p_mode | bmc_ptr->u_mode))) {
        if (!bmc_ptr->pet)
            msg_print(_("召喚された古代ドラゴンは怒っている！", "The summoned ancient dragon is angry!"));
    } else {
        bmc_ptr->no_trump = true;
    }

    return true;
}

bool cast_blue_summon_amberite(player_type *player_ptr, bmc_type *bmc_ptr)
{
    msg_print(_("アンバーの王族を召喚した！", "You summon a Lord of Amber!"));
    if (summon_specific(player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_AMBERITES,
            (bmc_ptr->g_mode | bmc_ptr->p_mode | bmc_ptr->u_mode))) {
        if (!bmc_ptr->pet)
            msg_print(_("召喚されたアンバーの王族は怒っている！", "The summoned Lord of Amber is angry!"));
    } else {
        bmc_ptr->no_trump = true;
    }

    return true;
}

bool cast_blue_summon_unique(player_type *player_ptr, bmc_type *bmc_ptr)
{
    int count = 0;
    msg_print(_("特別な強敵を召喚した！", "You summon a special opponent!"));
    for (int k = 0; k < 1; k++) {
        if (summon_specific(player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_UNIQUE,
                (bmc_ptr->g_mode | bmc_ptr->p_mode | PM_ALLOW_UNIQUE))) {
            count++;
            if (!bmc_ptr->pet)
                msg_print(_("召喚されたユニーク・モンスターは怒っている！", "The summoned special opponent is angry!"));
        }
    }

    for (int k = count; k < 1; k++) {
        if (summon_specific(player_ptr, (bmc_ptr->pet ? -1 : 0), player_ptr->y, player_ptr->x, bmc_ptr->summon_lev, SUMMON_HI_UNDEAD,
                (bmc_ptr->g_mode | bmc_ptr->p_mode | PM_ALLOW_UNIQUE))) {
            count++;
            if (!bmc_ptr->pet)
                msg_print(_("召喚された上級アンデッドは怒っている！", "The summoned greater undead is angry!"));
        }
    }

    if (!count)
        bmc_ptr->no_trump = true;

    return true;
}
