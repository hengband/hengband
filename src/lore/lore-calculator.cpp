#include "lore/lore-calculator.h"
#include "game-option/cheat-options.h"
#include "lore/lore-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-flags1.h"
#include "mspell/mspell-damage-calculator.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief ダイス目を文字列に変換する
 * @param base_damage 固定値
 * @param dice_num ダイス数
 * @param dice_side ダイス面
 * @param dice_mult ダイス倍率
 * @param dice_div ダイス除数
 * @param msg 文字列を格納するポインタ
 */
void dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div, char *msg)
{
    char base[80] = "", dice[80] = "", mult[80] = "";

    if (dice_num == 0) {
        sprintf(msg, "%d", base_damage);
        return;
    }

    if (base_damage != 0)
        sprintf(base, "%d+", base_damage);

    if (dice_num == 1)
        sprintf(dice, "d%d", dice_side);
    else
        sprintf(dice, "%dd%d", dice_num, dice_side);

    if (dice_mult != 1 || dice_div != 1) {
        if (dice_div == 1)
            sprintf(mult, "*%d", dice_mult);
        else
            sprintf(mult, "*(%d/%d)", dice_mult, dice_div);
    }

    sprintf(msg, "%s%s%s", base, dice, mult);
}

/*!
 * @brief モンスターのAC情報を得ることができるかを返す / Determine if the "armor" is known
 * @param r_idx モンスターの種族ID
 * @param know_everything 全知フラグ。TRUEを渡すとTRUEが返る。
 * @return 敵のACを知る条件が満たされているならTRUEを返す
 * @details
 * The higher the level, the fewer kills needed.
 */
bool know_armour(MONRACE_IDX r_idx, const bool know_everything)
{
    monster_race *r_ptr = &r_info[r_idx];
    DEPTH level = r_ptr->level;
    MONSTER_NUMBER kills = r_ptr->r_tkills;

    bool known = r_ptr->r_cast_spell == MAX_UCHAR;

    if (know_everything || known)
        return true;
    if (kills > 304 / (4 + level))
        return true;
    if (!(r_ptr->flags1 & RF1_UNIQUE))
        return false;
    if (kills > 304 / (38 + (5 * level) / 4))
        return true;
    return false;
}

/*!
 * @brief モンスターの打撃威力を知ることができるかどうかを返す
 * Determine if the "damage" of the given attack is known
 * @param r_idx モンスターの種族ID
 * @param i 確認したい攻撃手番
 * @return 敵のダメージダイスを知る条件が満たされているならTRUEを返す
 * @details
 * <pre>
 * the higher the level of the monster, the fewer the attacks you need,
 * the more damage an attack does, the more attacks you need
 * </pre>
 */
bool know_damage(MONRACE_IDX r_idx, int i)
{
    monster_race *r_ptr = &r_info[r_idx];
    DEPTH level = r_ptr->level;
    int32_t a = r_ptr->r_blows[i];

    int32_t d1 = r_ptr->blow[i].d_dice;
    int32_t d2 = r_ptr->blow[i].d_side;
    int32_t d = d1 * d2;

    if (d >= ((4 + level) * MAX_UCHAR) / 80)
        d = ((4 + level) * MAX_UCHAR - 1) / 80;
    if ((4 + level) * a > 80 * d)
        return true;
    if (!(r_ptr->flags1 & RF1_UNIQUE))
        return false;
    if ((4 + level) * (2 * a) > 80 * d)
        return true;

    return false;
}

/*!
 * @brief 文字列にモンスターの攻撃力を加える
 * @param r_idx モンスターの種族ID
 * @param SPELL_NUM 呪文番号
 * @param msg 表示する文字列
 */
void set_damage(PlayerType *player_ptr, lore_type *lore_ptr, MonsterAbilityType ms_type, concptr msg)
{
    MONRACE_IDX r_idx = lore_ptr->r_idx;
    int base_damage = monspell_race_damage(player_ptr, ms_type, r_idx, BASE_DAM);
    int dice_num = monspell_race_damage(player_ptr, ms_type, r_idx, DICE_NUM);
    int dice_side = monspell_race_damage(player_ptr, ms_type, r_idx, DICE_SIDE);
    int dice_mult = monspell_race_damage(player_ptr, ms_type, r_idx, DICE_MULT);
    int dice_div = monspell_race_damage(player_ptr, ms_type, r_idx, DICE_DIV);
    char dmg_str[80], dice_str[sizeof(dmg_str) + 10];
    char *tmp = lore_ptr->tmp_msg[lore_ptr->vn];
    dice_to_string(base_damage, dice_num, dice_side, dice_mult, dice_div, dmg_str);
    sprintf(dice_str, "(%s)", dmg_str);

    if (know_armour(r_idx, lore_ptr->know_everything))
        sprintf(tmp, msg, dice_str);
    else
        sprintf(tmp, msg, "");
}

void set_drop_flags(lore_type *lore_ptr)
{
    if (!lore_ptr->know_everything)
        return;

    lore_ptr->drop_gold = lore_ptr->drop_item = (((lore_ptr->r_ptr->flags1 & RF1_DROP_4D2) ? 8 : 0) + ((lore_ptr->r_ptr->flags1 & RF1_DROP_3D2) ? 6 : 0)
        + ((lore_ptr->r_ptr->flags1 & RF1_DROP_2D2) ? 4 : 0) + ((lore_ptr->r_ptr->flags1 & RF1_DROP_1D2) ? 2 : 0)
        + ((lore_ptr->r_ptr->flags1 & RF1_DROP_90) ? 1 : 0) + ((lore_ptr->r_ptr->flags1 & RF1_DROP_60) ? 1 : 0));

    if (lore_ptr->r_ptr->flags1 & RF1_ONLY_GOLD)
        lore_ptr->drop_item = 0;

    if (lore_ptr->r_ptr->flags1 & RF1_ONLY_ITEM)
        lore_ptr->drop_gold = 0;

    lore_ptr->flags1 = lore_ptr->r_ptr->flags1;
    lore_ptr->flags2 = lore_ptr->r_ptr->flags2;
    lore_ptr->flags3 = lore_ptr->r_ptr->flags3;
    lore_ptr->ability_flags = lore_ptr->r_ptr->ability_flags;
    lore_ptr->aura_flags = lore_ptr->r_ptr->aura_flags;
    lore_ptr->flagsr = lore_ptr->r_ptr->flagsr;
}
