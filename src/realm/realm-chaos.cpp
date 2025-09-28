#include "realm/realm-chaos.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "player-base/player-class.h"
#include "player/attack-defense-types.h"
#include "realm/realm-types.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-chaos.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/shape-changer.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-getter.h"
#include "util/dice.h"
#include "view/display-messages.h"

/*!
 * @brief カオス領域魔法の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST時は tl::nullopt を返す。
 */
tl::optional<std::string> do_chaos_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    bool info = mode == SpellProcessType::INFO;
    bool cast = mode == SpellProcessType::CAST;

    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0: {
        const Dice dice(3 + ((plev - 1) / 5), 4);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, AttributeType::MISSILE, dir, dice.roll());
        }
    } break;

    case 1: {
        POSITION rad = 1;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            destroy_doors_touch(player_ptr);
        }
    } break;

    case 2: {
        const Dice dice(2, plev / 2);
        POSITION rad = (plev / 10) + 1;

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            lite_area(player_ptr, dice.roll(), rad);
        }
    } break;

    case 3: {
        if (cast) {
            if (!(player_ptr->special_attack & ATTACK_CONFUSE)) {
                msg_print(_("あなたの手は光り始めた。", "Your hands start glowing."));
                player_ptr->special_attack |= ATTACK_CONFUSE;
                RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
            }
        }
    } break;

    case 4: {
        const Dice dice(3, 5);
        POSITION rad = (plev < 30) ? 2 : 3;
        int base;

        if (PlayerClass(player_ptr).is_wizard()) {
            base = plev + plev / 2;
        } else {
            base = plev + plev / 4;
        }

        if (info) {
            return info_damage(dice, base);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball(player_ptr, AttributeType::MISSILE, dir, dice.roll() + base, rad);

            /*
             * Shouldn't actually use MANA, as
             * it will destroy all items on the
             * floor
             */
        }
    } break;

    case 5: {
        const Dice dice(8 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_bolt_or_beam(player_ptr, beam_chance(player_ptr), AttributeType::FIRE, dir, dice.roll());
        }
    } break;

    case 6: {
        const Dice dice(8 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball(player_ptr, AttributeType::DISINTEGRATE, dir, dice.roll(), 0);
        }
    } break;

    case 7: {
        POSITION range = plev * 5;

        if (info) {
            return info_range(range);
        }

        if (cast) {
            teleport_player(player_ptr, range, TELEPORT_SPONTANEOUS);
        }
    } break;

    case 8: {
        if (info) {
            return KWD_RANDOM;
        }

        if (cast) {

            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            cast_wonder(player_ptr, dir);
        }
    } break;

    case 9: {
        const Dice dice(10 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_bolt_or_beam(player_ptr, beam_chance(player_ptr), AttributeType::CHAOS, dir, dice.roll());
        }
    } break;

    case 10: {
        int dam = 60 + plev;
        POSITION rad = plev / 10 + 2;

        if (info) {
            return info_damage(dam / 2);
        }

        if (cast) {
            msg_print(_("ドーン！部屋が揺れた！", "BOOM! Shake the room!"));
            project(player_ptr, 0, rad, player_ptr->y, player_ptr->x, dam, AttributeType::SOUND, PROJECT_KILL | PROJECT_ITEM);
        }
    } break;

    case 11: {
        const Dice dice(11 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_beam(player_ptr, AttributeType::MANA, dir, dice.roll());
        }
    } break;

    case 12: {
        int dam = plev + 55;
        POSITION rad = 2;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball(player_ptr, AttributeType::FIRE, dir, dam, rad);
        }
    } break;

    case 13: {
        int power = plev;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_beam(player_ptr, AttributeType::AWAY_ALL, dir, power);
        }
    } break;

    case 14: {
        int base = 12;
        const Dice dice(1, 4);

        if (cast) {
            destroy_area(player_ptr, player_ptr->y, player_ptr->x, base + dice.roll(), false);
        }
    } break;

    case 15: {
        int dam = plev * 2 + 99;
        POSITION rad = plev / 5;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball(player_ptr, AttributeType::CHAOS, dir, dam, rad);
        }
    } break;

    case 16: {
        int power = plev;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            poly_monster(player_ptr, dir, plev);
        }
    } break;

    case 17: {
        const Dice dice(5 + plev / 10, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            for (const auto &dir : Direction::directions_8()) {
                fire_beam(player_ptr, AttributeType::ELEC, dir, dice.roll());
            }
        }
    } break;

    case 18: {
        int power = 90;

        if (info) {
            return info_power(power);
        }
        if (cast) {
            if (!recharge(player_ptr, power)) {
                return tl::nullopt;
            }
        }
    } break;

    case 19: {
        int dam = plev + 70;
        POSITION rad = 3 + plev / 40;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball(player_ptr, AttributeType::DISINTEGRATE, dir, dam, rad);
        }
    } break;

    case 20: {
        int base = 15;
        const Dice dice(1, 20);

        if (info) {
            return info_delay(base, dice);
        }

        if (cast) {
            reserve_alter_reality(player_ptr, dice.roll() + base);
        }
    } break;

    case 21: {
        int dam = 120 + plev * 2;
        POSITION rad = 2;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            msg_print(_("ロケット発射！", "You launch a rocket!"));
            fire_rocket(player_ptr, AttributeType::ROCKET, dir, dam, rad);
        }
    } break;

    case 22: {
        if (cast) {
            brand_weapon(player_ptr, BrandType::CHAOTIC);
        }
    } break;

    case 23: {
        if (cast) {
            cast_summon_demon(player_ptr, (plev * 3) / 2);
        }
    } break;

    case 24: {
        const Dice dice(9 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }
            fire_beam(player_ptr, AttributeType::GRAVITY, dir, dice.roll());
        }
    } break;

    case 25: {
        int dam = plev * 2;
        POSITION rad = 2;

        if (info) {
            return info_multi_damage(dam);
        }

        if (cast) {
            cast_meteor(player_ptr, dam, rad);
        }
    } break;

    case 26: {
        int dam = 300 + 3 * plev;
        POSITION rad = 8;

        if (info) {
            return info_damage(dam / 2);
        }

        if (cast) {
            fire_ball(player_ptr, AttributeType::FIRE, Direction::self(), dam, rad);
        }
    } break;

    case 27: {
        if (info) {
            return format("%s150 / 250", KWD_DAM);
        }

        if (cast) {
            call_chaos(player_ptr);
        }
    } break;

    case 28: {
        if (cast) {
            if (!input_check(_("変身します。よろしいですか？", "You will polymorph yourself. Are you sure? "))) {
                return tl::nullopt;
            }
            do_poly_self(player_ptr);
        }
    } break;

    case 29: {
        int dam = 300 + plev * 4;
        POSITION rad = 4;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }
            fire_ball(player_ptr, AttributeType::MANA, dir, dam, rad);
        }
    } break;

    case 30: {
        int dam = player_ptr->chp;
        POSITION rad = 2;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball(player_ptr, AttributeType::CHAOS, dir, dam, rad);
        }
    } break;

    case 31: {
        if (info) {
            return format("%s3 * 175", KWD_DAM);
        }

        if (cast) {
            call_the_void(player_ptr);
        }
    } break;
    }

    return "";
}
