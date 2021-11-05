﻿#include "monster-floor/quantum-effect.h"
#include "floor/line-of-sight.h"
#include "monster-floor/monster-death.h"
#include "monster-floor/monster-remover.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"
#include "mspell/assign-monster-spell.h"
#include "mspell/mspell.h"
#include "spell-kind/spells-teleport.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief ユニークでない量子生物を消滅させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param see_m モンスターが視界内にいたらTRUE
 */
static void vanish_nonunique(PlayerType *player_ptr, MONSTER_IDX m_idx, bool see_m)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    if (see_m) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(player_ptr, m_name, m_ptr, 0);
        msg_format(_("%sは消え去った！", "%^s disappears!"), m_name);
    }

    monster_death(player_ptr, m_idx, false);
    delete_monster_idx(player_ptr, m_idx);
    if (is_pet(m_ptr) && !(m_ptr->ml))
        msg_print(_("少しの間悲しい気分になった。", "You feel sad for a moment."));
}

/*!
 * @brief 量子生物ユニークの量子的効果 (ショート・テレポートまたは距離10のテレポート・アウェイ)を実行する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param see_m モンスターが視界内にいたらTRUE
 * @details
 * プレイヤーが量子生物を観測しているか、量子生物がプレイヤーを観測している場合、互いの相対的な位置を確定させる
 * 波動関数の収縮はテレポートではないので反テレポート無効
 * @notes
 * パターンは収縮どころか拡散しているが、この際気にしてはいけない
 * @todo ユニークとプレイヤーとの間でしか効果が発生しない。ユニークとその他のモンスター間では何もしなくてよい？
 */
static void produce_quantum_effect(PlayerType *player_ptr, MONSTER_IDX m_idx, bool see_m)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    bool coherent = los(player_ptr, m_ptr->fy, m_ptr->fx, player_ptr->y, player_ptr->x);
    if (!see_m && !coherent)
        return;

    if (see_m) {
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(player_ptr, m_name, m_ptr, MD_NONE);
        msg_format(_("%sは量子的効果を起こした！", "%^s produced a decoherence!"), m_name);
    } else
        msg_print(_("量子的効果が起こった！", "A decoherence was produced!"));

    bool target = one_in_(2);
    if (target)
        (void)monspell_to_monster(player_ptr, RF_ABILITY::BLINK, m_ptr->fy, m_ptr->fx, m_idx, m_idx, true);
    else
        teleport_player_away(m_idx, player_ptr, 10, true);
}

/*!
 * @brief 量子生物の量子的効果を実行する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param see_m モンスターが視界内にいたらTRUE
 * @return モンスターが量子的効果により消滅したらTRUE
 */
bool process_quantum_effect(PlayerType *player_ptr, MONSTER_IDX m_idx, bool see_m)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if ((r_ptr->flags2 & RF2_QUANTUM) == 0)
        return false;
    if (!randint0(2))
        return false;
    if (randint0((m_idx % 100) + 10))
        return false;

    bool can_disappear = (r_ptr->flags1 & RF1_UNIQUE) == 0;
    can_disappear &= (r_ptr->flags1 & RF1_QUESTOR) == 0;
    if (can_disappear) {
        vanish_nonunique(player_ptr, m_idx, see_m);
        return true;
    }

    produce_quantum_effect(player_ptr, m_idx, see_m);
    return false;
}
