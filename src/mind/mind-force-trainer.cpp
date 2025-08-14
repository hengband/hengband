#include "mind/mind-force-trainer.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "effect/attribute-types.h"
#include "effect/spells-effect-util.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "mind/mind-magic-resistance.h"
#include "mind/mind-numbers.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/race-brightness-mask.h"
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
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-getter.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "view/display-messages.h"
#include "world/world.h"

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

    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MP);
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

    if (AngbandWorld::get_instance().is_wild_mode()) {
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

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
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
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
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
    auto boost = get_current_ki(player_ptr);
    if (heavy_armor(player_ptr)) {
        boost /= 2;
    }

    project_length = 1;
    const auto dir = get_aim_dir(player_ptr);
    if (!dir) {
        return false;
    }

    auto pos = player_ptr->get_neighbor(dir);
    PLAYER_LEVEL plev = player_ptr->lev;
    const auto dam = Dice::roll(8 + ((plev - 5) / 4) + boost / 12, 8);
    fire_beam(player_ptr, AttributeType::MISSILE, dir, dam);
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    if (!grid.has_monster()) {
        return true;
    }

    auto pos_target = pos;
    const auto pos_origin = pos;
    const auto m_idx = grid.m_idx;
    auto &monster = floor.m_list[m_idx];
    const auto &monrace = monster.get_monrace();
    const auto m_name = monster_desc(player_ptr, monster, 0);

    if (randint1(monrace.level * 3 / 2) > randint0(dam / 2) + dam / 2) {
        msg_format(_("%sは飛ばされなかった。", "%s^ was not blown away."), m_name.data());
        return true;
    }

    const auto p_pos = player_ptr->get_position();
    for (auto i = 0; i < 5; i++) {
        pos += dir.vec();
        if (floor.is_empty_at(pos) && (pos != p_pos)) {
            pos_target = pos;
        } else {
            break;
        }
    }

    if (pos_target == pos_origin) {
        return true;
    }

    msg_format(_("%sを吹き飛ばした！", "You blow %s away!"), m_name.data());
    floor.get_grid(pos_origin).m_idx = 0;
    floor.get_grid(pos_target).m_idx = m_idx;
    monster.fy = pos_target.y;
    monster.fx = pos_target.x;

    update_monster(player_ptr, m_idx, true);
    lite_spot(player_ptr, pos_origin);
    lite_spot(player_ptr, pos_target);

    if (monrace.brightness_flags.has_any_of(ld_mask)) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
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
    PLAYER_LEVEL plev = player_ptr->lev;
    int boost = get_current_ki(player_ptr);
    if (heavy_armor(player_ptr)) {
        boost /= 2;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    switch (spell) {
    case MindForceTrainerType::SMALL_FORCE_BALL: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        fire_ball(player_ptr, AttributeType::MISSILE, dir, Dice::roll(3 + ((plev - 1) / 5) + boost / 12, 4), 0);
        break;
    }
    case MindForceTrainerType::FLASH_LIGHT:
        (void)lite_area(player_ptr, Dice::roll(2, (plev / 2)), (plev / 10) + 1);
        break;
    case MindForceTrainerType::FLYING_TECHNIQUE:
        set_tim_levitation(player_ptr, randint1(30) + 30 + boost / 5, false);
        break;
    case MindForceTrainerType::KAMEHAMEHA: {
        project_length = plev / 8 + 3;
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        fire_beam(player_ptr, AttributeType::MISSILE, dir, Dice::roll(5 + ((plev - 1) / 5) + boost / 10, 5));
        break;
    }
    case MindForceTrainerType::MAGIC_RESISTANCE:
        set_resist_magic(player_ptr, randint1(20) + 20 + boost / 5, false);
        break;
    case MindForceTrainerType::IMPROVE_FORCE:
        msg_print(_("気を練った。", "You improved the Force."));
        set_current_ki(player_ptr, false, 70 + plev);
        rfu.set_flag(StatusRecalculatingFlag::BONUS);
        if (randint1(get_current_ki(player_ptr)) > (plev * 4 + 120)) {
            msg_print(_("気が暴走した！", "The Force exploded!"));
            fire_ball(player_ptr, AttributeType::MANA, Direction::self(), get_current_ki(player_ptr) / 2, 10);
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
    case MindForceTrainerType::LARGE_FORCE_BALL: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        fire_ball(player_ptr, AttributeType::MISSILE, dir, Dice::roll(10, 6) + plev * 3 / 2 + boost * 3 / 5, (plev < 30) ? 2 : 3);
        break;
    }
    case MindForceTrainerType::DISPEL_MAGIC: {
        const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
        if (!pos) {
            return false;
        }

        const auto &floor = *player_ptr->current_floor_ptr;
        const auto &grid = floor.get_grid(*pos);
        const auto m_idx = grid.m_idx;
        const auto p_pos = player_ptr->get_position();
        const auto is_projectable = projectable(floor, p_pos, *pos);
        if ((m_idx == 0) || !grid.has_los() || !is_projectable) {
            break;
        }

        dispel_monster_status(player_ptr, m_idx);
        break;
    }
    case MindForceTrainerType::SUMMON_GHOST: {
        bool success = false;
        for (int i = 0; i < 1 + boost / 100; i++) {
            if (summon_specific(player_ptr, player_ptr->y, player_ptr->x, plev, SUMMON_PHANTOM, PM_FORCE_PET)) {
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
        fire_ball(player_ptr, AttributeType::FIRE, Direction::self(), 200 + (2 * plev) + boost * 2, 10);
        break;
    case MindForceTrainerType::SUPER_KAMEHAMEHA: {
        const auto dir = get_aim_dir(player_ptr);
        if (!dir) {
            return false;
        }

        fire_beam(player_ptr, AttributeType::MANA, dir, Dice::roll(10 + (plev / 2) + boost * 3 / 10, 15));
        break;
    }
    case MindForceTrainerType::LIGHT_SPEED:
        set_lightspeed(player_ptr, randint1(16) + 16 + boost / 20, false);
        break;
    default:
        msg_print(_("なに？", "Zap?"));
    }

    set_current_ki(player_ptr, true, 0);
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    return true;
}
