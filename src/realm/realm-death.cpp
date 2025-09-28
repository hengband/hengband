#include "realm/realm-death.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "hpmp/hp-mp-processor.h"
#include "player-base/player-class.h"
#include "player-info/race-info.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "realm/realm-types.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-specific-bolt.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/dice.h"

/*!
 * @brief 暗黒領域魔法の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST時は tl::nullopt を返す。
 */
tl::optional<std::string> do_death_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    bool info = mode == SpellProcessType::INFO;
    bool cast = mode == SpellProcessType::CAST;

    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_monsters_nonliving(player_ptr, rad);
        }
    } break;

    case 1: {
        const Dice dice(3 + (plev - 1) / 5, 4);
        POSITION rad = 0;

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            /*
             * A radius-0 ball may (1) be aimed at
             * objects etc., and will affect them;
             * (2) may be aimed at ANY visible
             * monster, unlike a 'bolt' which must
             * travel to the monster.
             */

            fire_ball(player_ptr, AttributeType::HELL_FIRE, dir, dice.roll(), rad);

            if (one_in_(5)) {
                /* Special effect first */
                int effect = randint1(1000);

                if (effect == 666) {
                    fire_ball_hide(player_ptr, AttributeType::DEATH_RAY, dir, plev * 200, 0);
                } else if (effect < 500) {
                    fire_ball_hide(player_ptr, AttributeType::TURN_ALL, dir, plev, 0);
                } else if (effect < 800) {
                    fire_ball_hide(player_ptr, AttributeType::OLD_CONF, dir, plev, 0);
                } else {
                    fire_ball_hide(player_ptr, AttributeType::STUN, dir, plev, 0);
                }
            }
        }
    } break;

    case 2: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_monsters_evil(player_ptr, rad);
        }
    } break;

    case 3: {
        int dam = 10 + plev / 2;
        POSITION rad = 2;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball(player_ptr, AttributeType::POIS, dir, dam, rad);
        }
    } break;

    case 4: {
        int power = plev;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            sleep_monster(player_ptr, dir, plev);
        }
    } break;

    case 5: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_pois(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 6: {
        int power = plev;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fear_monster(player_ptr, dir, plev);
            stun_monster(player_ptr, dir, plev);
        }
    } break;

    case 7: {
        int power = plev;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            control_one_undead(player_ptr, dir, plev);
        }
    } break;

    case 8: {
        const Dice dice(3, 6);
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

            fire_ball(player_ptr, AttributeType::HYPODYNAMIA, dir, dice.roll() + base, rad);
        }
    } break;

    case 9: {
        const Dice dice(8 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_bolt_or_beam(player_ptr, beam_chance(player_ptr), AttributeType::NETHER, dir, dice.roll());
        }
    } break;

    case 10: {
        int dam = (30 + plev) * 2;
        POSITION rad = plev / 10 + 2;

        if (info) {
            return info_damage(dam / 2);
        }

        if (cast) {
            project(player_ptr, 0, rad, player_ptr->y, player_ptr->x, dam, AttributeType::POIS, PROJECT_KILL | PROJECT_ITEM);
        }
    } break;

    case 11: {
        int power = plev + 50;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball_hide(player_ptr, AttributeType::GENOCIDE, dir, power, 0);
        }
    } break;

    case 12: {
        if (cast) {
            brand_weapon(player_ptr, BrandType::BRAND_POISON);
        }
    } break;

    case 13: {
        const Dice dice(1, plev * 2);
        int base = plev * 2;

        if (info) {
            return info_damage(dice, base);
        }

        if (cast) {
            int dam = base + dice.roll();

            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            if (hypodynamic_bolt(player_ptr, dir, dam)) {
                chg_virtue(player_ptr, Virtue::SACRIFICE, -1);
                chg_virtue(player_ptr, Virtue::VITALITY, -1);

                hp_player(player_ptr, dam);

                /*
                 * Gain nutritional sustenance:
                 * 150/hp drained
                 *
                 * A Food ration gives 5000
                 * food points (by contrast)
                 * Don't ever get more than
                 * "Full" this way But if we
                 * ARE Gorged, it won't cure
                 * us
                 */
                dam = player_ptr->food + std::min(5000, 100 * dam);

                /* Not gorged already */
                if (player_ptr->food < PY_FOOD_MAX) {
                    set_food(player_ptr, dam >= PY_FOOD_MAX ? PY_FOOD_MAX - 1 : dam);
                }
            }
        }
    } break;

    case 14: {
        if (cast) {
            animate_dead(player_ptr, 0, player_ptr->y, player_ptr->x);
        }
    } break;

    case 15: {
        int power = plev + 50;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            symbol_genocide(player_ptr, power, true);
        }
    } break;

    case 16: {
        int base = 25;
        const Dice dice(1, 25);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            (void)berserk(player_ptr, base + dice.roll());
        }
    } break;

    case 17: {
        if (info) {
            return KWD_RANDOM;
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            cast_invoke_spirits(player_ptr, dir);
        }
    } break;

    case 18: {
        const Dice dice(4 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_bolt_or_beam(player_ptr, beam_chance(player_ptr), AttributeType::DARK, dir, dice.roll());
        }
    } break;

    case 19: {
        int b_base = 25;
        const Dice b_dice(1, 25);
        int sp_base = plev / 2;
        const Dice sp_dice(1, 20 + plev / 2);

        if (info) {
            return info_duration(b_base, b_dice);
        }

        if (cast) {
            (void)berserk(player_ptr, b_base + b_dice.roll());
            set_acceleration(player_ptr, sp_dice.roll() + sp_base, false);
        }
    } break;

    case 20: {
        if (cast) {
            brand_weapon(player_ptr, BrandType::VAMPIRIC);
        }
    } break;

    case 21: {
        int dam = 100;

        if (info) {
            return format("%s3*%d", KWD_DAM, dam);
        }

        if (cast) {
            int i;

            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            chg_virtue(player_ptr, Virtue::SACRIFICE, -1);
            chg_virtue(player_ptr, Virtue::VITALITY, -1);

            for (i = 0; i < 3; i++) {
                if (hypodynamic_bolt(player_ptr, dir, dam)) {
                    hp_player(player_ptr, dam);
                }
            }
        }
    } break;

    case 22: {
        const Dice dice(1, plev * 3);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            dispel_living(player_ptr, dice.roll());
        }
    } break;

    case 23: {
        int dam = 100 + plev * 2;
        POSITION rad = 4;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball(player_ptr, AttributeType::DARK, dir, dam, rad);
        }
    } break;

    case 24: {
        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            death_ray(player_ptr, dir, plev);
        }
    } break;

    case 25:
        if (cast) {
            cast_summon_undead(player_ptr, (plev * 3) / 2);
        }
        break;

    case 26: {
        if (cast) {
            if (randint1(50) > plev) {
                if (!ident_spell(player_ptr, false)) {
                    return tl::nullopt;
                }
            } else {
                if (!identify_fully(player_ptr, false)) {
                    return tl::nullopt;
                }
            }
        }
    } break;

    case 27: {
        int base = 10 + plev / 2;
        const Dice dice(1, 10 + plev / 2);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_mimic(player_ptr, base + dice.roll(), MimicKindType::VAMPIRE, false);
        }
    } break;

    case 28: {
        if (cast) {
            restore_level(player_ptr);
        }
    } break;

    case 29: {
        int power = plev + 50;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            mass_genocide(player_ptr, power, true);
        }
    } break;

    case 30: {
        int dam = 666;
        POSITION rad = 3;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball(player_ptr, AttributeType::HELL_FIRE, dir, dam, rad);
            take_hit(player_ptr, DAMAGE_USELIFE, 20 + randint1(30), _("地獄の劫火の呪文を唱えた疲労", "the strain of casting Hellfire"));
        }
    } break;

    case 31: {
        int base = plev / 2;
        const Dice dice(1, plev / 2);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_wraith_form(player_ptr, dice.roll() + base, false);
        }
    } break;
    }

    return "";
}
