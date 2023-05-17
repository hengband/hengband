#include "cmd-item/cmd-zapwand.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "core/window-redrawer.h"
#include "effect/attribute-types.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "inventory/player-inventory.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/special-object-flags.h"
#include "object-use/zapwand-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-specific-bolt.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-status.h"
#include "status/action-setter.h"
#include "status/experience.h"
#include "sv-definition/sv-wand-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"

/*!
 * @brief 魔法棒の効果を発動する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sval オブジェクトのsval
 * @param dir 発動の方向ID
 * @param powerful 強力発動上の処理ならばTRUE
 * @param magic 魔道具術上の処理ならばTRUE
 * @return 発動により効果内容が確定したならばTRUEを返す
 */
bool wand_effect(PlayerType *player_ptr, int sval, int dir, bool powerful, bool magic)
{
    bool ident = false;
    PLAYER_LEVEL lev = powerful ? player_ptr->lev * 2 : player_ptr->lev;
    POSITION rad = powerful ? 3 : 2;

    /* XXX Hack -- Wand of wonder can do anything before it */
    if (sval == SV_WAND_WONDER) {
        int vir = virtue_number(player_ptr, Virtue::CHANCE);
        sval = randint0(SV_WAND_WONDER);

        if (vir) {
            if (player_ptr->virtues[vir - 1] > 0) {
                while (randint1(300) < player_ptr->virtues[vir - 1]) {
                    sval++;
                }
                if (sval > SV_WAND_COLD_BALL) {
                    sval = randint0(4) + SV_WAND_ACID_BALL;
                }
            } else {
                while (randint1(300) < (0 - player_ptr->virtues[vir - 1])) {
                    sval--;
                }
                if (sval < SV_WAND_HEAL_MONSTER) {
                    sval = randint0(3) + SV_WAND_HEAL_MONSTER;
                }
            }
        }
        if (sval < SV_WAND_TELEPORT_AWAY) {
            chg_virtue(player_ptr, Virtue::CHANCE, 1);
        }
    }

    /* Analyze the wand */
    switch (sval) {
    case SV_WAND_HEAL_MONSTER: {
        int dam = damroll((powerful ? 20 : 10), 10);
        if (heal_monster(player_ptr, dir, dam)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_HASTE_MONSTER: {
        if (speed_monster(player_ptr, dir, lev)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_CLONE_MONSTER: {
        if (clone_monster(player_ptr, dir)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_TELEPORT_AWAY: {
        int distance = MAX_PLAYER_SIGHT * (powerful ? 8 : 5);
        if (teleport_monster(player_ptr, dir, distance)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_DISARMING: {
        if (disarm_trap(player_ptr, dir)) {
            ident = true;
        }
        if (powerful && disarm_traps_touch(player_ptr)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_TRAP_DOOR_DEST: {
        if (destroy_door(player_ptr, dir)) {
            ident = true;
        }
        if (powerful && destroy_doors_touch(player_ptr)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_STONE_TO_MUD: {
        int dam = powerful ? 40 + randint1(60) : 20 + randint1(30);
        if (wall_to_mud(player_ptr, dir, dam)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_LITE: {
        int dam = damroll((powerful ? 12 : 6), 8);
        msg_print(_("青く輝く光線が放たれた。", "A line of blue shimmering light appears."));
        (void)lite_line(player_ptr, dir, dam);
        ident = true;
        break;
    }

    case SV_WAND_SLEEP_MONSTER: {
        if (sleep_monster(player_ptr, dir, lev)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_SLOW_MONSTER: {
        if (slow_monster(player_ptr, dir, lev)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_CONFUSE_MONSTER: {
        if (confuse_monster(player_ptr, dir, lev)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_FEAR_MONSTER: {
        if (fear_monster(player_ptr, dir, lev)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_HYPODYNAMIA: {
        if (hypodynamic_bolt(player_ptr, dir, 80 + lev)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_POLYMORPH: {
        if (poly_monster(player_ptr, dir, lev)) {
            ident = true;
        }
        break;
    }

    case SV_WAND_STINKING_CLOUD: {
        fire_ball(player_ptr, AttributeType::POIS, dir, 12 + lev / 4, rad);
        ident = true;
        break;
    }

    case SV_WAND_MAGIC_MISSILE: {
        fire_bolt_or_beam(player_ptr, 20, AttributeType::MISSILE, dir, damroll(2 + lev / 10, 6));
        ident = true;
        break;
    }

    case SV_WAND_ACID_BOLT: {
        fire_bolt_or_beam(player_ptr, 20, AttributeType::ACID, dir, damroll(6 + lev / 7, 8));
        ident = true;
        break;
    }

    case SV_WAND_CHARM_MONSTER: {
        if (charm_monster(player_ptr, dir, std::max<short>(20, lev))) {
            ident = true;
        }
        break;
    }

    case SV_WAND_FIRE_BOLT: {
        fire_bolt_or_beam(player_ptr, 20, AttributeType::FIRE, dir, damroll(7 + lev / 6, 8));
        ident = true;
        break;
    }

    case SV_WAND_COLD_BOLT: {
        fire_bolt_or_beam(player_ptr, 20, AttributeType::COLD, dir, damroll(5 + lev / 8, 8));
        ident = true;
        break;
    }

    case SV_WAND_ACID_BALL: {
        fire_ball(player_ptr, AttributeType::ACID, dir, 60 + 3 * lev / 4, rad);
        ident = true;
        break;
    }

    case SV_WAND_ELEC_BALL: {
        fire_ball(player_ptr, AttributeType::ELEC, dir, 40 + 3 * lev / 4, rad);
        ident = true;
        break;
    }

    case SV_WAND_FIRE_BALL: {
        fire_ball(player_ptr, AttributeType::FIRE, dir, 70 + 3 * lev / 4, rad);
        ident = true;
        break;
    }

    case SV_WAND_COLD_BALL: {
        fire_ball(player_ptr, AttributeType::COLD, dir, 50 + 3 * lev / 4, rad);
        ident = true;
        break;
    }

    case SV_WAND_WONDER: {
        msg_print(_("おっと、謎の魔法棒を始動させた。", "Oops.  Wand of wonder activated."));
        break;
    }

    case SV_WAND_DRAGON_FIRE: {
        fire_breath(player_ptr, AttributeType::FIRE, dir, (powerful ? 300 : 200), 3);
        ident = true;
        break;
    }

    case SV_WAND_DRAGON_COLD: {
        fire_breath(player_ptr, AttributeType::COLD, dir, (powerful ? 270 : 180), 3);
        ident = true;
        break;
    }

    case SV_WAND_DRAGON_BREATH: {
        int dam;
        AttributeType typ;

        switch (randint1(5)) {
        case 1:
            dam = 240;
            typ = AttributeType::ACID;
            break;
        case 2:
            dam = 210;
            typ = AttributeType::ELEC;
            break;
        case 3:
            dam = 240;
            typ = AttributeType::FIRE;
            break;
        case 4:
            dam = 210;
            typ = AttributeType::COLD;
            break;
        default:
            dam = 180;
            typ = AttributeType::POIS;
            break;
        }

        if (powerful) {
            dam = (dam * 3) / 2;
        }

        fire_breath(player_ptr, typ, dir, dam, 3);

        ident = true;
        break;
    }

    case SV_WAND_DISINTEGRATE: {
        fire_ball(player_ptr, AttributeType::DISINTEGRATE, dir, 200 + randint1(lev * 2), rad);
        ident = true;
        break;
    }

    case SV_WAND_ROCKETS: {
        msg_print(_("ロケットを発射した！", "You launch a rocket!"));
        fire_rocket(player_ptr, AttributeType::ROCKET, dir, 250 + lev * 3, rad);
        ident = true;
        break;
    }

    case SV_WAND_STRIKING: {
        fire_bolt(player_ptr, AttributeType::METEOR, dir, damroll(15 + lev / 3, 13));
        ident = true;
        break;
    }

    case SV_WAND_GENOCIDE: {
        fire_ball_hide(player_ptr, AttributeType::GENOCIDE, dir, magic ? lev + 50 : 250, 0);
        ident = true;
        break;
    }
    }
    return ident;
}

/*!
 * @brief 魔法棒を使うコマンドのメインルーチン /
 */
void do_cmd_aim_wand(PlayerType *player_ptr)
{
    OBJECT_IDX item;
    concptr q, s;

    if (player_ptr->wild_mode) {
        return;
    }
    if (cmd_limit_arena(player_ptr)) {
        return;
    }
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });

    q = _("どの魔法棒で狙いますか? ", "Aim which wand? ");
    s = _("使える魔法棒がない。", "You have no wand to aim.");
    if (!choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), TvalItemTester(ItemKindType::WAND))) {
        return;
    }

    ObjectZapWandEntity(player_ptr).execute(item);
}
