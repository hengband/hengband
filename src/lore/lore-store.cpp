/*!
 * @brief モンスターの思い出を記憶する処理
 * @date 2020/06/09
 * @author Hourier
 */

#include "lore/lore-store.h"
#include "core/window-redrawer.h"
#include "monster-race/race-flags1.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h" //!< @todo 違和感、m_ptr は外から与えることとしたい.
#include "system/player-type-definition.h"

/*!
 * @brief モンスターの調査による思い出補完処理 / Learn about a monster (by "probing" it)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param r_idx 補完されるモンスター種族ID
 * @return 明らかになった情報の度数
 * @details
 * Return the number of new flags learnt.  -Mogami-
 */
int lore_do_probe(player_type *player_ptr, MONRACE_IDX r_idx)
{
    int n = 0;
    monster_race *r_ptr = &r_info[r_idx];
    if (r_ptr->r_wake != MAX_UCHAR)
        n++;
    if (r_ptr->r_ignore != MAX_UCHAR)
        n++;
    r_ptr->r_wake = r_ptr->r_ignore = MAX_UCHAR;

    for (int i = 0; i < 4; i++) {
        if (r_ptr->blow[i].effect || r_ptr->blow[i].method) {
            if (r_ptr->r_blows[i] != MAX_UCHAR)
                n++;
            r_ptr->r_blows[i] = MAX_UCHAR;
        }
    }

    byte tmp_byte = (((r_ptr->flags1 & RF1_DROP_4D2) ? 8 : 0) + ((r_ptr->flags1 & RF1_DROP_3D2) ? 6 : 0) + ((r_ptr->flags1 & RF1_DROP_2D2) ? 4 : 0)
        + ((r_ptr->flags1 & RF1_DROP_1D2) ? 2 : 0) + ((r_ptr->flags1 & RF1_DROP_90) ? 1 : 0) + ((r_ptr->flags1 & RF1_DROP_60) ? 1 : 0));

    if (!(r_ptr->flags1 & RF1_ONLY_GOLD)) {
        if (r_ptr->r_drop_item != tmp_byte)
            n++;
        r_ptr->r_drop_item = tmp_byte;
    }
    if (!(r_ptr->flags1 & RF1_ONLY_ITEM)) {
        if (r_ptr->r_drop_gold != tmp_byte)
            n++;
        r_ptr->r_drop_gold = tmp_byte;
    }

    if (r_ptr->r_cast_spell != MAX_UCHAR)
        n++;
    r_ptr->r_cast_spell = MAX_UCHAR;

    for (int i = 0; i < 32; i++) {
        if (!(r_ptr->r_flags1 & (1UL << i)) && (r_ptr->flags1 & (1UL << i)))
            n++;
        if (!(r_ptr->r_flags2 & (1UL << i)) && (r_ptr->flags2 & (1UL << i)))
            n++;
        if (!(r_ptr->r_flags3 & (1UL << i)) && (r_ptr->flags3 & (1UL << i)))
            n++;
        if (!(r_ptr->r_flagsr & (1UL << i)) && (r_ptr->flagsr & (1UL << i)))
            n++;
    }

    auto ability_flags = r_ptr->ability_flags;
    n += ability_flags.reset(r_ptr->r_ability_flags).count();

    r_ptr->r_flags1 = r_ptr->flags1;
    r_ptr->r_flags2 = r_ptr->flags2;
    r_ptr->r_flags3 = r_ptr->flags3;
    r_ptr->r_flagsr = r_ptr->flagsr;
    r_ptr->r_ability_flags = r_ptr->ability_flags;

    if (!r_ptr->r_can_evolve)
        n++;
    r_ptr->r_can_evolve = true;

    if (player_ptr->monster_race_idx == r_idx) {
        player_ptr->window_flags |= (PW_MONSTER);
    }

    return n;
}

/*!
 * @brief モンスターの撃破に伴うドロップ情報の記憶処理 / Take note that the given monster just dropped some treasure
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスター情報のID
 * @param num_item 手に入れたアイテム数
 * @param num_gold 手に入れた財宝の単位数
 */
void lore_treasure(player_type *player_ptr, MONSTER_IDX m_idx, ITEM_NUMBER num_item, ITEM_NUMBER num_gold)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    if (!is_original_ap(m_ptr))
        return;

    if (num_item > r_ptr->r_drop_item)
        r_ptr->r_drop_item = num_item;
    if (num_gold > r_ptr->r_drop_gold)
        r_ptr->r_drop_gold = num_gold;

    if (r_ptr->flags1 & (RF1_DROP_GOOD))
        r_ptr->r_flags1 |= (RF1_DROP_GOOD);
    if (r_ptr->flags1 & (RF1_DROP_GREAT))
        r_ptr->r_flags1 |= (RF1_DROP_GREAT);
    if (player_ptr->monster_race_idx == m_ptr->r_idx)
        player_ptr->window_flags |= (PW_MONSTER);
}
