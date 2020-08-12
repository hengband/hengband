#include "monster/monster-status-setter.h"
#include "dungeon/quest-completion-checker.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/smart-learn-types.h"
#include "player/avatar.h"
#include "system/monster-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief モンスターをペットにする
 * @param player_type プレーヤーへの参照ポインタ
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void set_pet(player_type *player_ptr, monster_type *m_ptr)
{
    check_quest_completion(player_ptr, m_ptr);
    m_ptr->smart |= SM_PET;
    if (!(r_info[m_ptr->r_idx].flags3 & (RF3_EVIL | RF3_GOOD)))
        m_ptr->sub_align = SUB_ALIGN_NEUTRAL;
}

/*!
 * @brief モンスターを敵に回す
 * Makes the monster hostile towards the player
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void set_hostile(player_type *player_ptr, monster_type *m_ptr)
{
    if (player_ptr->phase_out)
        return;

    m_ptr->smart &= ~SM_PET;
    m_ptr->smart &= ~SM_FRIENDLY;
}

/*!
 * @brief モンスターを怒らせる
 * Anger the monster
 * @param m_ptr モンスター情報構造体の参照ポインタ
 * @return なし
 */
void anger_monster(player_type *player_ptr, monster_type *m_ptr)
{
    if (player_ptr->phase_out)
        return;
    if (!is_friendly(m_ptr))
        return;

    GAME_TEXT m_name[MAX_NLEN];

    monster_desc(player_ptr, m_name, m_ptr, 0);
    msg_format(_("%^sは怒った！", "%^s gets angry!"), m_name);
    set_hostile(player_ptr, m_ptr);
    chg_virtue(player_ptr, V_INDIVIDUALISM, 1);
    chg_virtue(player_ptr, V_HONOUR, -1);
    chg_virtue(player_ptr, V_JUSTICE, -1);
    chg_virtue(player_ptr, V_COMPASSION, -1);
}
