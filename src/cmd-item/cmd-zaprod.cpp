#include "cmd-item/cmd-zaprod.h"
#include "action/action-limited.h"
#include "effect/attribute-types.h"
#include "floor/floor-object.h"
#include "object-enchant/special-object-flags.h"
#include "object-use/zaprod-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-specific-bolt.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/buff-setter.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "sv-definition/sv-rod-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief ロッドの効果を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sval オブジェクトのsval
 * @param dir 発動目標の方向ID
 * @param use_charge チャージを消費したかどうかを返す参照ポインタ
 * @param powerful 強力発動上の処理ならばTRUE
 * @return 発動により効果内容が確定したならばTRUEを返す
 */
int rod_effect(PlayerType *player_ptr, int sval, int dir, bool *use_charge, bool powerful)
{
    int ident = false;
    PLAYER_LEVEL lev = powerful ? player_ptr->lev * 2 : player_ptr->lev;
    POSITION detect_rad = powerful ? DETECT_RAD_DEFAULT * 3 / 2 : DETECT_RAD_DEFAULT;
    POSITION rad = powerful ? 3 : 2;

    /* Analyze the rod */
    switch (sval) {
    case SV_ROD_DETECT_TRAP: {
        if (detect_traps(player_ptr, detect_rad, dir == 0)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_DETECT_DOOR: {
        if (detect_doors(player_ptr, detect_rad)) {
            ident = true;
        }
        if (detect_stairs(player_ptr, detect_rad)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_IDENTIFY: {
        if (powerful) {
            if (!identify_fully(player_ptr, false)) {
                *use_charge = false;
            }
        } else {
            if (!ident_spell(player_ptr, false)) {
                *use_charge = false;
            }
        }
        ident = true;
        break;
    }

    case SV_ROD_RECALL: {
        if (!recall_player(player_ptr, randint0(21) + 15)) {
            *use_charge = false;
        }
        ident = true;
        break;
    }

    case SV_ROD_ILLUMINATION: {
        if (lite_area(player_ptr, damroll(2, 8), (powerful ? 4 : 2))) {
            ident = true;
        }
        break;
    }

    case SV_ROD_MAPPING: {
        map_area(player_ptr, powerful ? DETECT_RAD_MAP * 3 / 2 : DETECT_RAD_MAP);
        ident = true;
        break;
    }

    case SV_ROD_DETECTION: {
        detect_all(player_ptr, detect_rad);
        ident = true;
        break;
    }

    case SV_ROD_PROBING: {
        probing(player_ptr);
        ident = true;
        break;
    }

    case SV_ROD_CURING: {
        if (true_healing(player_ptr, 0)) {
            ident = true;
        }
        if (set_shero(player_ptr, 0, true)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_HEALING: {
        if (cure_critical_wounds(player_ptr, powerful ? 750 : 500)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_RESTORATION: {
        if (restore_level(player_ptr)) {
            ident = true;
        }
        if (restore_all_status(player_ptr)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_SPEED: {
        if (set_acceleration(player_ptr, randint1(30) + (powerful ? 30 : 15), false)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_PESTICIDE: {
        if (dispel_monsters(player_ptr, powerful ? 8 : 4)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_TELEPORT_AWAY: {
        int distance = MAX_PLAYER_SIGHT * (powerful ? 8 : 5);
        if (teleport_monster(player_ptr, dir, distance)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_DISARMING: {
        if (disarm_trap(player_ptr, dir)) {
            ident = true;
        }
        if (powerful && disarm_traps_touch(player_ptr)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_LITE: {
        int dam = damroll((powerful ? 12 : 6), 8);
        msg_print(_("青く輝く光線が放たれた。", "A line of blue shimmering light appears."));
        (void)lite_line(player_ptr, dir, dam);
        ident = true;
        break;
    }

    case SV_ROD_SLEEP_MONSTER: {
        if (sleep_monster(player_ptr, dir, lev)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_SLOW_MONSTER: {
        if (slow_monster(player_ptr, dir, lev)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_HYPODYNAMIA: {
        if (hypodynamic_bolt(player_ptr, dir, 70 + 3 * lev / 2)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_POLYMORPH: {
        if (poly_monster(player_ptr, dir, lev)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_ACID_BOLT: {
        fire_bolt_or_beam(player_ptr, 10, AttributeType::ACID, dir, damroll(6 + lev / 7, 8));
        ident = true;
        break;
    }

    case SV_ROD_ELEC_BOLT: {
        fire_bolt_or_beam(player_ptr, 10, AttributeType::ELEC, dir, damroll(4 + lev / 9, 8));
        ident = true;
        break;
    }

    case SV_ROD_FIRE_BOLT: {
        fire_bolt_or_beam(player_ptr, 10, AttributeType::FIRE, dir, damroll(7 + lev / 6, 8));
        ident = true;
        break;
    }

    case SV_ROD_COLD_BOLT: {
        fire_bolt_or_beam(player_ptr, 10, AttributeType::COLD, dir, damroll(5 + lev / 8, 8));
        ident = true;
        break;
    }

    case SV_ROD_ACID_BALL: {
        fire_ball(player_ptr, AttributeType::ACID, dir, 60 + lev, rad);
        ident = true;
        break;
    }

    case SV_ROD_ELEC_BALL: {
        fire_ball(player_ptr, AttributeType::ELEC, dir, 40 + lev, rad);
        ident = true;
        break;
    }

    case SV_ROD_FIRE_BALL: {
        fire_ball(player_ptr, AttributeType::FIRE, dir, 70 + lev, rad);
        ident = true;
        break;
    }

    case SV_ROD_COLD_BALL: {
        fire_ball(player_ptr, AttributeType::COLD, dir, 50 + lev, rad);
        ident = true;
        break;
    }

    case SV_ROD_HAVOC: {
        call_chaos(player_ptr);
        ident = true;
        break;
    }

    case SV_ROD_STONE_TO_MUD: {
        int dam = powerful ? 40 + randint1(60) : 20 + randint1(30);
        if (wall_to_mud(player_ptr, dir, dam)) {
            ident = true;
        }
        break;
    }

    case SV_ROD_AGGRAVATE: {
        aggravate_monsters(player_ptr, 0);
        ident = true;
        break;
    }
    }
    return ident;
}

/*!
 * @brief ロッドを使うコマンドのメインルーチン /
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_zap_rod(PlayerType *player_ptr)
{
    if (player_ptr->wild_mode) {
        return;
    }

    if (cmd_limit_arena(player_ptr)) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });

    constexpr auto q = _("どのロッドを振りますか? ", "Zap which rod? ");
    constexpr auto s = _("使えるロッドがない。", "You have no rod to zap.");
    short i_idx;
    if (!choose_object(player_ptr, &i_idx, q, s, (USE_INVEN | USE_FLOOR), TvalItemTester(ItemKindType::ROD))) {
        return;
    }

    ObjectZapRodEntity(player_ptr).execute(i_idx);
}
