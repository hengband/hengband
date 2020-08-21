#include "spell-kind/spells-genocide.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/quest.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/write-diary.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster-floor/monster-remover.h"
#include "monster/monster-status.h"
#include "player-info/avatar.h"
#include "player/player-damage.h"
#include "system/floor-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief モンスターへの単体抹殺処理サブルーチン / Delete a non-unique/non-quest monster
 * @param m_idx 抹殺するモンスターID
 * @param power 抹殺の威力
 * @param player_cast プレイヤーの魔法によるものならば TRUE
 * @param dam_side プレイヤーへの負担ダメージ量(1d(dam_side))
 * @param spell_name 抹殺効果を起こした魔法の名前
 * @return 効力があった場合TRUEを返す
 */
bool genocide_aux(player_type *caster_ptr, MONSTER_IDX m_idx, int power, bool player_cast, int dam_side, concptr spell_name)
{
    monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    if (is_pet(m_ptr) && !player_cast)
        return FALSE;

    bool resist = FALSE;
    if (r_ptr->flags1 & (RF1_UNIQUE | RF1_QUESTOR))
        resist = TRUE;
    else if (r_ptr->flags7 & RF7_UNIQUE2)
        resist = TRUE;
    else if (m_idx == caster_ptr->riding)
        resist = TRUE;
    else if ((caster_ptr->current_floor_ptr->inside_quest && !random_quest_number(caster_ptr, caster_ptr->current_floor_ptr->dun_level))
        || caster_ptr->current_floor_ptr->inside_arena || caster_ptr->phase_out)
        resist = TRUE;
    else if (player_cast && (r_ptr->level > randint0(power)))
        resist = TRUE;
    else if (player_cast && (m_ptr->mflag2 & MFLAG2_NOGENO))
        resist = TRUE;
    else {
        if (record_named_pet && is_pet(m_ptr) && m_ptr->nickname) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(caster_ptr, m_name, m_ptr, MD_INDEF_VISIBLE);
            exe_write_diary(caster_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_GENOCIDE, m_name);
        }

        delete_monster_idx(caster_ptr, m_idx);
    }

    if (resist && player_cast) {
        bool see_m = is_seen(caster_ptr, m_ptr);
        GAME_TEXT m_name[MAX_NLEN];
        monster_desc(caster_ptr, m_name, m_ptr, 0);
        if (see_m) {
            msg_format(_("%^sには効果がなかった。", "%^s is unaffected."), m_name);
        }

        if (monster_csleep_remaining(m_ptr)) {
            (void)set_monster_csleep(caster_ptr, m_idx, 0);
            if (m_ptr->ml) {
                msg_format(_("%^sが目を覚ました。", "%^s wakes up."), m_name);
            }
        }

        if (is_friendly(m_ptr) && !is_pet(m_ptr)) {
            if (see_m) {
                msg_format(_("%sは怒った！", "%^s gets angry!"), m_name);
            }

            set_hostile(caster_ptr, m_ptr);
        }

        if (one_in_(13))
            m_ptr->mflag2 |= MFLAG2_NOGENO;
    }

    if (player_cast) {
        take_hit(caster_ptr, DAMAGE_GENO, randint1(dam_side), format(_("%^sの呪文を唱えた疲労", "the strain of casting %^s"), spell_name), -1);
    }

    move_cursor_relative(caster_ptr->y, caster_ptr->x);
    caster_ptr->redraw |= (PR_HP);
    caster_ptr->window |= (PW_PLAYER);
    handle_stuff(caster_ptr);
    term_fresh();

    int msec = delay_factor * delay_factor * delay_factor;
    term_xtra(TERM_XTRA_DELAY, msec);

    return !resist;
}

/*!
 * @brief モンスターへのシンボル抹殺処理ルーチン / Delete all non-unique/non-quest monsters of a given "type" from the level
 * @param power 抹殺の威力
 * @param player_cast プレイヤーの魔法によるものならば TRUE
 * @return 効力があった場合TRUEを返す
 */
bool symbol_genocide(player_type *caster_ptr, int power, bool player_cast)
{
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    bool is_special_floor = floor_ptr->inside_quest && !random_quest_number(caster_ptr, floor_ptr->dun_level);
    is_special_floor |= caster_ptr->current_floor_ptr->inside_arena;
    is_special_floor |= caster_ptr->phase_out;
    if (is_special_floor) {
        msg_print(_("何も起きないようだ……", "It seems nothing happen here..."));
        return FALSE;
    }

    char typ;
    while (!get_com(_("どの種類(文字)のモンスターを抹殺しますか: ", "Choose a monster race (by symbol) to genocide: "), &typ, FALSE))
        ;
    bool result = FALSE;
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (!monster_is_valid(m_ptr))
            continue;
        if (r_ptr->d_char != typ)
            continue;

        result |= genocide_aux(caster_ptr, i, power, player_cast, 4, _("抹殺", "Genocide"));
    }

    if (result) {
        chg_virtue(caster_ptr, V_VITALITY, -2);
        chg_virtue(caster_ptr, V_CHANCE, -1);
    }

    return result;
}

/*!
 * @brief モンスターへの周辺抹殺処理ルーチン / Delete all nearby (non-unique) monsters
 * @param power 抹殺の威力
 * @param player_cast プレイヤーの魔法によるものならば TRUE
 * @return 効力があった場合TRUEを返す
 */
bool mass_genocide(player_type *caster_ptr, int power, bool player_cast)
{
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    bool is_special_floor = floor_ptr->inside_quest && !random_quest_number(caster_ptr, floor_ptr->dun_level);
    is_special_floor |= caster_ptr->current_floor_ptr->inside_arena;
    is_special_floor |= caster_ptr->phase_out;
    if (is_special_floor) {
        return FALSE;
    }

    bool result = FALSE;
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr))
            continue;
        if (m_ptr->cdis > MAX_SIGHT)
            continue;

        result |= genocide_aux(caster_ptr, i, power, player_cast, 3, _("周辺抹殺", "Mass Genocide"));
    }

    if (result) {
        chg_virtue(caster_ptr, V_VITALITY, -2);
        chg_virtue(caster_ptr, V_CHANCE, -1);
    }

    return result;
}

/*!
 * @brief アンデッド・モンスターへの周辺抹殺処理ルーチン / Delete all nearby (non-unique) undead
 * @param power 抹殺の威力
 * @param player_cast プレイヤーの魔法によるものならば TRUE
 * @return 効力があった場合TRUEを返す
 */
bool mass_genocide_undead(player_type *caster_ptr, int power, bool player_cast)
{
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    bool is_special_floor = floor_ptr->inside_quest && !random_quest_number(caster_ptr, floor_ptr->dun_level);
    is_special_floor |= caster_ptr->current_floor_ptr->inside_arena;
    is_special_floor |= caster_ptr->phase_out;
    if (is_special_floor) {
        return FALSE;
    }

    bool result = FALSE;
    for (MONSTER_IDX i = 1; i < caster_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[i];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (!monster_is_valid(m_ptr))
            continue;
        if (!(r_ptr->flags3 & RF3_UNDEAD))
            continue;
        if (m_ptr->cdis > MAX_SIGHT)
            continue;

        result |= genocide_aux(caster_ptr, i, power, player_cast, 3, _("アンデッド消滅", "Annihilate Undead"));
    }

    if (result) {
        chg_virtue(caster_ptr, V_UNLIFE, -2);
        chg_virtue(caster_ptr, V_CHANCE, -1);
    }

    return result;
}
