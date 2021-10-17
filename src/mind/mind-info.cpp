#include "mind/mind-info.h"
#include "cmd-action/cmd-spell.h"
#include "locale/japanese.h"
#include "mind/mind-force-trainer.h"
#include "mind/mind-types.h"
#include "player-info/equipment-info.h"
#include "system/player-type-definition.h"

static void switch_mind_mindcrafter(player_type *player_ptr, const PLAYER_LEVEL plev, const int power, char *p)
{
    switch (power) {
    case 0:
        break;
    case 1:
        sprintf(p, " %s%dd%d", KWD_DAM, 3 + ((plev - 1) / 4), 3 + plev / 15);
        break;
    case 2:
        sprintf(p, " %s10", KWD_SPHERE);
        break;
    case 3:
        sprintf(p, " %s%d", KWD_SPHERE, plev * 5);
        break;
    case 4:
        break;
    case 5:
        sprintf(p, " %s%dd8", KWD_DAM, 8 + ((plev - 5) / 4));
        break;
    case 6:
        sprintf(p, " %s%d", KWD_DURATION, plev);
        break;
    case 7:
        break;
    case 8:
        sprintf(p, (plev < 25 ? " %s%d" : " %sd%d"), KWD_DAM, (plev < 25 ? plev * 3 / 2 : plev * ((plev - 5) / 10 + 1)));
        break;
    case 9:
        sprintf(p, " %s10+d%d", KWD_DURATION, plev * 3 / 2);
        break;
#ifdef JP
    case 10:
        sprintf(p, " 最大重量:%d.%dkg", lb_to_kg_integer(plev * 15), lb_to_kg_fraction(plev * 15));
        break;
#else
    case 10:
        sprintf(p, " max wgt %d", plev * 15);
        break;
#endif
    case 11:
        sprintf(p, " %s%dd6", KWD_DAM, plev / 2);
        break;
    case 12:
        sprintf(p, " %sd%d+%d", KWD_DAM, plev * 3, plev * 3);
        break;
    case 13:
        sprintf(p, _(" 行動:%ld回", " %ld acts."), (long int)(player_ptr->csp + 100 - player_ptr->energy_need - 50) / 100);
        break;
    }
}

static void switch_mind_ki(player_type *player_ptr, const PLAYER_LEVEL plev, const int power, char *p)
{
    int boost = get_current_ki(player_ptr);
    if (heavy_armor(player_ptr))
        boost /= 2;

    switch (power) {
    case 0:
        sprintf(p, " %s%dd4", KWD_DAM, 3 + ((plev - 1) / 5) + boost / 12);
        break;
    case 1:
        break;
    case 2:
        sprintf(p, " %s%d+d30", KWD_DURATION, 30 + boost / 5);
        break;
    case 3:
        sprintf(p, " %s%dd5", KWD_DAM, 5 + ((plev - 1) / 5) + boost / 10);
        break;
    case 4:
        sprintf(p, " %s%d+d20", KWD_DURATION, 20 + boost / 5);
        break;
    case 5:
        break;
    case 6:
        sprintf(p, " %s%d+d%d", KWD_DURATION, 15 + boost / 7, plev / 2);
        break;
    case 7:
        sprintf(p, " %s%dd8", KWD_DAM, 8 + ((plev - 5) / 4) + boost / 12);
        break;
    case 8:
        sprintf(p, " %s10d6+%d", KWD_DAM, plev * 3 / 2 + boost * 3 / 5);
        break;
    case 9:
        break;
    case 10:
        sprintf(p, _(" 最大%d体", " max %d"), 1 + boost / 100);
        break;
    case 11:
        sprintf(p, " %s%d", KWD_DAM, 100 + plev + boost);
        break;
    case 12:
        sprintf(p, " %s%dd15", KWD_DAM, 10 + plev / 2 + boost * 3 / 10);
        break;
    case 13:
        sprintf(p, _(" 行動:%d+d16回", " %d+d16 acts"), 16 + boost / 20);
        break;
    }
}

static void switch_mind_mirror_master(const PLAYER_LEVEL plev, const int power, char *p)
{
    switch (power) {
    case 0:
        break;
    case 1:
        break;
    case 2:
        sprintf(p, " %s%dd4", KWD_DAM, 3 + ((plev - 1) / 5));
        break;
    case 3:
        sprintf(p, " %s10", KWD_SPHERE);
        break;
    case 4:
        break;
    case 5:
        sprintf(p, " %s%d", KWD_SPHERE, plev * 5);
        break;
    case 6:
        sprintf(p, " %s20+d20", KWD_DURATION);
        break;
    case 7:
        break;
    case 8:
        sprintf(p, " %s%dd8", KWD_DAM, 8 + ((plev - 5) / 4));
        break;
    case 9:
        break;
    case 10:
        sprintf(p, " %s%dd8", KWD_DAM, 11 + (plev - 5) / 4);
        break;
    case 11:
        break;
    case 12:
        sprintf(p, " %s20+d20", KWD_DURATION);
        break;
    case 13:
        sprintf(p, " %s150+d%d", KWD_DAM, plev * 2);
        break;
    case 14:
        break;
    case 15:
        break;
    case 16:
        sprintf(p, " %s%d", KWD_SPHERE, plev / 2 + 10);
        break;
    case 17:
        break;
    case 18:
        sprintf(p, " %s6+d6", KWD_DURATION);
        break;
    case 19:
        sprintf(p, " %s%d", KWD_DAM, plev * 11 + 5);
        break;
    case 20:
        sprintf(p, " %s4+d4", KWD_DURATION);
        break;
    }
}

static void switch_mind_ninja(const PLAYER_LEVEL plev, const int power, char *p)
{
    switch (power) {
    case 0:
    case 1:
        break;
    case 2:
        sprintf(p, " %s10", KWD_SPHERE);
        break;
    case 3:
        break;
    case 4:
        sprintf(p, " %s%d", KWD_SPHERE, plev * 5);
        break;
    case 5:
        sprintf(p, " %s30", KWD_SPHERE);
        break;
    case 6:
    case 7:
        break;
    case 8:
        sprintf(p, " %s20+d20", KWD_DURATION);
        break;
    case 9:
        sprintf(p, " %s%d", KWD_DAM, (50 + plev) / 2);
        break;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        break;
    case 16:
        sprintf(p, " %s%d+d%d", KWD_DURATION, plev / 2, plev / 2);
        break;
    case 17:
        sprintf(p, " %s%d*3", KWD_DAM, (75 + plev * 2 / 3) / 2);
        break;
    case 18:
        sprintf(p, " %s%dd10", KWD_DAM, 6 + plev / 8);
        break;
    case 19:
        sprintf(p, " %s6+d6", KWD_DURATION);
        break;
    }
}

/*!
 * @brief 特殊技能の効果情報をまとめたフォーマットを返す
 * @param p 情報を返す文字列参照ポインタ
 * @param use_mind 職業毎の特殊技能ID
 * @param power モンスター魔法のID
 */
void mindcraft_info(player_type *player_ptr, char *p, mind_kind_type use_mind, int power)
{
    const PLAYER_LEVEL plev = player_ptr->lev;
    strcpy(p, "");
    switch (use_mind) {
    case mind_kind_type::MINDCRAFTER:
        switch_mind_mindcrafter(player_ptr, plev, power, p);
        break;
    case mind_kind_type::KI:
        switch_mind_ki(player_ptr, plev, power, p);
        break;
    case mind_kind_type::MIRROR_MASTER:
        switch_mind_mirror_master(plev, power, p);
        break;
    case mind_kind_type::NINJUTSU:
        switch_mind_ninja(plev, power, p);
        break;
    default:
        break;
    }
}
