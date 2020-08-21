#include "mind/mind-force-trainer.h"
#include "cmd-action/cmd-pet.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-numbers.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "player-info/avatar.h"
#include "player/player-damage.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell/spell-types.h"
#include "spell/summon-types.h"
#include "status/temporary-resistance.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "view/display-messages.h"

/*!
 * @brief 練気術師が「練気」で溜めた気の量を返す
 * @param caster_ptr プレーヤーの参照ポインタ
 * @return 現在溜まっている気の量
 */
MAGIC_NUM1 get_current_ki(player_type *caster_ptr) { return caster_ptr->magic_num1[0]; }

/*!
 * @brief 練気術師において、気を溜める
 * @param caster_ptr プレーヤーの参照ポインタ
 * @param is_reset TRUEなら気の量をkiにセットし、FALSEなら加減算を行う
 * @param ki 気の量
 * @return なし
 */
void set_current_ki(player_type *caster_ptr, bool is_reset, MAGIC_NUM1 ki)
{
    if (is_reset) {
        caster_ptr->magic_num1[0] = ki;
        return;
    }

    caster_ptr->magic_num1[0] += ki;
}

bool clear_mind(player_type *creature_ptr)
{
    if (total_friends) {
        msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
        return FALSE;
    }

    msg_print(_("少し頭がハッキリした。", "You feel your head clear a little."));

    creature_ptr->csp += (3 + creature_ptr->lev / 20);
    if (creature_ptr->csp >= creature_ptr->msp) {
        creature_ptr->csp = creature_ptr->msp;
        creature_ptr->csp_frac = 0;
    }

    creature_ptr->redraw |= (PR_MANA);
    return TRUE;
}

/*!
 * @brief 光速移動の継続時間をセットする / Set "lightspeed", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return なし
 */
void set_lightspeed(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return;

    if (creature_ptr->wild_mode)
        v = 0;

    if (v) {
        if (creature_ptr->lightspeed && !do_dec) {
            if (creature_ptr->lightspeed > v)
                return;
        } else if (!creature_ptr->lightspeed) {
            msg_print(_("非常に素早く動けるようになった！", "You feel yourself moving extremely fast!"));
            notice = TRUE;
            chg_virtue(creature_ptr, V_PATIENCE, -1);
            chg_virtue(creature_ptr, V_DILIGENCE, 1);
        }
    } else {
        if (creature_ptr->lightspeed) {
            msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
            notice = TRUE;
        }
    }

    creature_ptr->lightspeed = v;

    if (!notice)
        return;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    handle_stuff(creature_ptr);
}

/*!
 * @brief 一時的闘気のオーラの継続時間をセットする / Set "tim_sh_touki", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_sh_force(player_type *creature_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = FALSE;
    v = (v > 10000) ? 10000 : (v < 0) ? 0 : v;

    if (creature_ptr->is_dead)
        return FALSE;

    if (v) {
        if (creature_ptr->tim_sh_touki && !do_dec) {
            if (creature_ptr->tim_sh_touki > v)
                return FALSE;
        } else if (!creature_ptr->tim_sh_touki) {
            msg_print(_("体が闘気のオーラで覆われた。", "You are enveloped by an aura of the Force!"));
            notice = TRUE;
        }
    } else {
        if (creature_ptr->tim_sh_touki) {
            msg_print(_("闘気が消えた。", "The aura of the Force disappeared."));
            notice = TRUE;
        }
    }

    creature_ptr->tim_sh_touki = v;
    creature_ptr->redraw |= (PR_STATUS);

    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    handle_stuff(creature_ptr);
    return TRUE;
}

/*!
 * @brief 衝波
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 命中したらTRUE
 */
bool shock_power(player_type *caster_ptr)
{
    int boost = get_current_ki(caster_ptr);
    if (heavy_armor(caster_ptr))
        boost /= 2;

    project_length = 1;
    DIRECTION dir;
    if (!get_aim_dir(caster_ptr, &dir))
        return FALSE;

    POSITION y = caster_ptr->y + ddy[dir];
    POSITION x = caster_ptr->x + ddx[dir];
    PLAYER_LEVEL plev = caster_ptr->lev;
    HIT_POINT dam = damroll(8 + ((plev - 5) / 4) + boost / 12, 8);
    fire_beam(caster_ptr, GF_MISSILE, dir, dam);
    if (!caster_ptr->current_floor_ptr->grid_array[y][x].m_idx)
        return TRUE;

    POSITION ty = y, tx = x;
    POSITION oy = y, ox = x;
    MONSTER_IDX m_idx = caster_ptr->current_floor_ptr->grid_array[y][x].m_idx;
    monster_type *m_ptr = &caster_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    GAME_TEXT m_name[MAX_NLEN];
    monster_desc(caster_ptr, m_name, m_ptr, 0);

    if (randint1(r_ptr->level * 3 / 2) > randint0(dam / 2) + dam / 2) {
        msg_format(_("%sは飛ばされなかった。", "%^s was not blown away."), m_name);
        return TRUE;
    }

    for (int i = 0; i < 5; i++) {
        y += ddy[dir];
        x += ddx[dir];
        if (is_cave_empty_bold(caster_ptr, y, x)) {
            ty = y;
            tx = x;
        } else {
            break;
        }
    }

    bool is_shock_successful = ty != oy;
    is_shock_successful |= tx != ox;
    if (is_shock_successful)
        return TRUE;

    msg_format(_("%sを吹き飛ばした！", "You blow %s away!"), m_name);
    caster_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = 0;
    caster_ptr->current_floor_ptr->grid_array[ty][tx].m_idx = m_idx;
    m_ptr->fy = ty;
    m_ptr->fx = tx;

    update_monster(caster_ptr, m_idx, TRUE);
    lite_spot(caster_ptr, oy, ox);
    lite_spot(caster_ptr, ty, tx);

    if (r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
        caster_ptr->update |= (PU_MON_LITE);

    return TRUE;
}

/*!
 * @brief 練気術の発動 /
 * do_cmd_cast calls this function if the player's class is 'ForceTrainer'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_force_spell(player_type *caster_ptr, mind_force_trainer_type spell)
{
    DIRECTION dir;
    PLAYER_LEVEL plev = caster_ptr->lev;
    int boost = get_current_ki(caster_ptr);
    if (heavy_armor(caster_ptr))
        boost /= 2;

    switch (spell) {
    case SMALL_FORCE_BALL:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_ball(caster_ptr, GF_MISSILE, dir, damroll(3 + ((plev - 1) / 5) + boost / 12, 4), 0);
        break;
    case FLASH_LIGHT:
        (void)lite_area(caster_ptr, damroll(2, (plev / 2)), (plev / 10) + 1);
        break;
    case FLYING_TECHNIQUE:
        set_tim_levitation(caster_ptr, randint1(30) + 30 + boost / 5, FALSE);
        break;
    case KAMEHAMEHA:
        project_length = plev / 8 + 3;
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_beam(caster_ptr, GF_MISSILE, dir, damroll(5 + ((plev - 1) / 5) + boost / 10, 5));
        break;
    case MAGIC_RESISTANCE:
        set_resist_magic(caster_ptr, randint1(20) + 20 + boost / 5, FALSE);
        break;
    case IMPROVE_FORCE:
        msg_print(_("気を練った。", "You improved the Force."));
        set_current_ki(caster_ptr, FALSE, 70 + plev);
        caster_ptr->update |= (PU_BONUS);
        if (randint1(get_current_ki(caster_ptr)) > (plev * 4 + 120)) {
            msg_print(_("気が暴走した！", "The Force exploded!"));
            fire_ball(caster_ptr, GF_MANA, 0, get_current_ki(caster_ptr) / 2, 10);
            take_hit(caster_ptr, DAMAGE_LOSELIFE, caster_ptr->magic_num1[0] / 2, _("気の暴走", "Explosion of the Force"), -1);
        } else
            return TRUE;

        break;
    case AURA_OF_FORCE:
        set_tim_sh_force(caster_ptr, randint1(plev / 2) + 15 + boost / 7, FALSE);
        break;
    case SHOCK_POWER:
        return shock_power(caster_ptr);
        break;
    case LARGE_FORCE_BALL:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_ball(caster_ptr, GF_MISSILE, dir, damroll(10, 6) + plev * 3 / 2 + boost * 3 / 5, (plev < 30) ? 2 : 3);
        break;
    case DISPEL_MAGIC: {
        if (!target_set(caster_ptr, TARGET_KILL))
            return FALSE;

        MONSTER_IDX m_idx = caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
        if ((m_idx == 0) || !player_has_los_bold(caster_ptr, target_row, target_col)
            || !projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col))
            break;

        dispel_monster_status(caster_ptr, m_idx);
        break;
    }
    case SUMMON_GHOST: {
        bool success = FALSE;
        for (int i = 0; i < 1 + boost / 100; i++)
            if (summon_specific(caster_ptr, -1, caster_ptr->y, caster_ptr->x, plev, SUMMON_PHANTOM, PM_FORCE_PET))
                success = TRUE;

        if (success)
            msg_print(_("御用でございますが、御主人様？", "'Your wish, master?'"));
        else
            msg_print(_("何も現れなかった。", "Nothing happen."));

        break;
    }
    case EXPLODING_FLAME:
        fire_ball(caster_ptr, GF_FIRE, 0, 200 + (2 * plev) + boost * 2, 10);
        break;
    case SUPER_KAMEHAMEHA:
        if (!get_aim_dir(caster_ptr, &dir))
            return FALSE;

        fire_beam(caster_ptr, GF_MANA, dir, damroll(10 + (plev / 2) + boost * 3 / 10, 15));
        break;
    case LIGHT_SPEED:
        set_lightspeed(caster_ptr, randint1(16) + 16 + boost / 20, FALSE);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
    }

    set_current_ki(caster_ptr, TRUE, 0);
    caster_ptr->update |= PU_BONUS;
    return TRUE;
}
