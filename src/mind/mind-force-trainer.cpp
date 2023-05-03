#include "mind/mind-force-trainer.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "effect/attribute-types.h"
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
#include "monster-race/race-brightness-mask.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "pet/pet-util.h"
#include "player-base/player-class.h"
#include "player-info/equipment-info.h"
#include "player-info/force-trainer-data-type.h"
#include "player/player-damage.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell/summon-types.h"
#include "status/temporary-resistance.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "view/display-messages.h"

/*!
 * @brief 練気術師が「練気」で溜めた気の量を返す
 * @param player_ptr プレイヤーの参照ポインタ
 * @return 現在溜まっている気の量
 */
int32_t get_current_ki(PlayerType *player_ptr)
{
    auto data = PlayerClass(player_ptr).get_specific_data<force_trainer_data_type>();

    return data ? data->ki : 0;
}

/*!
 * @brief 練気術師において、気を溜める
 * @param player_ptr プレイヤーの参照ポインタ
 * @param is_reset TRUEなら気の量をkiにセットし、FALSEなら加減算を行う
 * @param ki 気の量
 */
void set_current_ki(PlayerType *player_ptr, bool is_reset, int32_t ki)
{
    auto data = PlayerClass(player_ptr).get_specific_data<force_trainer_data_type>();
    if (!data) {
        return;
    }

    if (is_reset) {
        data->ki = ki;
        return;
    }

    data->ki += ki;
}

bool clear_mind(PlayerType *player_ptr)
{
    if (total_friends) {
        msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
        return false;
    }

    msg_print(_("少し頭がハッキリした。", "You feel your head clear a little."));

    player_ptr->csp += (3 + player_ptr->lev / 20);
    if (player_ptr->csp >= player_ptr->msp) {
        player_ptr->csp = player_ptr->msp;
        player_ptr->csp_frac = 0;
    }

    player_ptr->redraw |= (PR_MP);
    return true;
}

/*!
 * @brief 光速移動の継続時間をセットする / Set "lightspeed", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 */
void set_lightspeed(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return;
    }

    if (player_ptr->wild_mode) {
        v = 0;
    }

    if (v) {
        if (player_ptr->lightspeed && !do_dec) {
            if (player_ptr->lightspeed > v) {
                return;
            }
        } else if (!player_ptr->lightspeed) {
            msg_print(_("非常に素早く動けるようになった！", "You feel yourself moving extremely fast!"));
            notice = true;
            chg_virtue(player_ptr, Virtue::PATIENCE, -1);
            chg_virtue(player_ptr, Virtue::DILIGENCE, 1);
        }
    } else {
        if (player_ptr->lightspeed) {
            msg_print(_("動きの素早さがなくなったようだ。", "You feel yourself slow down."));
            notice = true;
        }
    }

    player_ptr->lightspeed = v;

    if (!notice) {
        return;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    player_ptr->update |= (PU_BONUS);
    handle_stuff(player_ptr);
}

/*!
 * @brief 一時的闘気のオーラの継続時間をセットする / Set "tim_sh_touki", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_sh_force(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_sh_touki && !do_dec) {
            if (player_ptr->tim_sh_touki > v) {
                return false;
            }
        } else if (!player_ptr->tim_sh_touki) {
            msg_print(_("体が闘気のオーラで覆われた。", "You are enveloped by an aura of the Force!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_sh_touki) {
            msg_print(_("闘気が消えた。", "The aura of the Force disappeared."));
            notice = true;
        }
    }

    player_ptr->tim_sh_touki = v;
    player_ptr->redraw |= (PR_TIMED_EFFECT);

    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    handle_stuff(player_ptr);
    return true;
}

/*!
 * @brief 衝波
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 命中したらTRUE
 */
bool shock_power(PlayerType *player_ptr)
{
    int boost = get_current_ki(player_ptr);
    if (heavy_armor(player_ptr)) {
        boost /= 2;
    }

    project_length = 1;
    DIRECTION dir;
    if (!get_aim_dir(player_ptr, &dir)) {
        return false;
    }

    POSITION y = player_ptr->y + ddy[dir];
    POSITION x = player_ptr->x + ddx[dir];
    PLAYER_LEVEL plev = player_ptr->lev;
    int dam = damroll(8 + ((plev - 5) / 4) + boost / 12, 8);
    fire_beam(player_ptr, AttributeType::MISSILE, dir, dam);
    if (!player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
        return true;
    }

    POSITION ty = y, tx = x;
    POSITION oy = y, ox = x;
    MONSTER_IDX m_idx = player_ptr->current_floor_ptr->grid_array[y][x].m_idx;
    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    auto *r_ptr = &monraces_info[m_ptr->r_idx];
    const auto m_name = monster_desc(player_ptr, m_ptr, 0);

    if (randint1(r_ptr->level * 3 / 2) > randint0(dam / 2) + dam / 2) {
        msg_format(_("%sは飛ばされなかった。", "%s^ was not blown away."), m_name.data());
        return true;
    }

    for (int i = 0; i < 5; i++) {
        y += ddy[dir];
        x += ddx[dir];
        if (is_cave_empty_bold(player_ptr, y, x)) {
            ty = y;
            tx = x;
        } else {
            break;
        }
    }

    bool is_shock_successful = ty != oy;
    is_shock_successful |= tx != ox;
    if (is_shock_successful) {
        return true;
    }

    msg_format(_("%sを吹き飛ばした！", "You blow %s away!"), m_name.data());
    player_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = 0;
    player_ptr->current_floor_ptr->grid_array[ty][tx].m_idx = m_idx;
    m_ptr->fy = ty;
    m_ptr->fx = tx;

    update_monster(player_ptr, m_idx, true);
    lite_spot(player_ptr, oy, ox);
    lite_spot(player_ptr, ty, tx);

    if (r_ptr->brightness_flags.has_any_of(ld_mask)) {
        player_ptr->update |= (PU_MONSTER_LITE);
    }

    return true;
}

/*!
 * @brief 練気術の発動 /
 * do_cmd_cast calls this function if the player's class is 'ForceTrainer'.
 * @param spell 発動する特殊技能のID
 * @return 処理を実行したらTRUE、キャンセルした場合FALSEを返す。
 */
bool cast_force_spell(PlayerType *player_ptr, MindForceTrainerType spell)
{
    DIRECTION dir;
    PLAYER_LEVEL plev = player_ptr->lev;
    int boost = get_current_ki(player_ptr);
    if (heavy_armor(player_ptr)) {
        boost /= 2;
    }

    switch (spell) {
    case MindForceTrainerType::SMALL_FORCE_BALL:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        fire_ball(player_ptr, AttributeType::MISSILE, dir, damroll(3 + ((plev - 1) / 5) + boost / 12, 4), 0);
        break;
    case MindForceTrainerType::FLASH_LIGHT:
        (void)lite_area(player_ptr, damroll(2, (plev / 2)), (plev / 10) + 1);
        break;
    case MindForceTrainerType::FLYING_TECHNIQUE:
        set_tim_levitation(player_ptr, randint1(30) + 30 + boost / 5, false);
        break;
    case MindForceTrainerType::KAMEHAMEHA:
        project_length = plev / 8 + 3;
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        fire_beam(player_ptr, AttributeType::MISSILE, dir, damroll(5 + ((plev - 1) / 5) + boost / 10, 5));
        break;
    case MindForceTrainerType::MAGIC_RESISTANCE:
        set_resist_magic(player_ptr, randint1(20) + 20 + boost / 5, false);
        break;
    case MindForceTrainerType::IMPROVE_FORCE:
        msg_print(_("気を練った。", "You improved the Force."));
        set_current_ki(player_ptr, false, 70 + plev);
        player_ptr->update |= (PU_BONUS);
        if (randint1(get_current_ki(player_ptr)) > (plev * 4 + 120)) {
            msg_print(_("気が暴走した！", "The Force exploded!"));
            fire_ball(player_ptr, AttributeType::MANA, 0, get_current_ki(player_ptr) / 2, 10);
            auto data = PlayerClass(player_ptr).get_specific_data<force_trainer_data_type>();
            take_hit(player_ptr, DAMAGE_LOSELIFE, data->ki / 2, _("気の暴走", "Explosion of the Force"));
        } else {
            return true;
        }

        break;
    case MindForceTrainerType::AURA_OF_FORCE:
        set_tim_sh_force(player_ptr, randint1(plev / 2) + 15 + boost / 7, false);
        break;
    case MindForceTrainerType::SHOCK_POWER:
        return shock_power(player_ptr);
        break;
    case MindForceTrainerType::LARGE_FORCE_BALL:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        fire_ball(player_ptr, AttributeType::MISSILE, dir, damroll(10, 6) + plev * 3 / 2 + boost * 3 / 5, (plev < 30) ? 2 : 3);
        break;
    case MindForceTrainerType::DISPEL_MAGIC: {
        if (!target_set(player_ptr, TARGET_KILL)) {
            return false;
        }

        MONSTER_IDX m_idx = player_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
        const auto is_projectable = projectable(player_ptr, player_ptr->y, player_ptr->x, target_row, target_col);
        if ((m_idx == 0) || !player_has_los_bold(player_ptr, target_row, target_col) || !is_projectable) {
            break;
        }

        dispel_monster_status(player_ptr, m_idx);
        break;
    }
    case MindForceTrainerType::SUMMON_GHOST: {
        bool success = false;
        for (int i = 0; i < 1 + boost / 100; i++) {
            if (summon_specific(player_ptr, -1, player_ptr->y, player_ptr->x, plev, SUMMON_PHANTOM, PM_FORCE_PET)) {
                success = true;
            }
        }

        if (success) {
            msg_print(_("御用でございますが、御主人様？", "'Your wish, master?'"));
        } else {
            msg_print(_("何も現れなかった。", "Nothing happens."));
        }

        break;
    }
    case MindForceTrainerType::EXPLODING_FLAME:
        fire_ball(player_ptr, AttributeType::FIRE, 0, 200 + (2 * plev) + boost * 2, 10);
        break;
    case MindForceTrainerType::SUPER_KAMEHAMEHA:
        if (!get_aim_dir(player_ptr, &dir)) {
            return false;
        }

        fire_beam(player_ptr, AttributeType::MANA, dir, damroll(10 + (plev / 2) + boost * 3 / 10, 15));
        break;
    case MindForceTrainerType::LIGHT_SPEED:
        set_lightspeed(player_ptr, randint1(16) + 16 + boost / 20, false);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
    }

    set_current_ki(player_ptr, true, 0);
    player_ptr->update |= PU_BONUS;
    return true;
}
