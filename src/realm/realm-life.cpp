#include "realm/realm-life.h"
#include "cmd-action/cmd-spell.h"
#include "effect/attribute-types.h"
#include "player/digestion-processor.h"
#include "realm/realm-types.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-world.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "status/temporary-resistance.h"
#include "system/floor/floor-info.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/dice.h"

/*!
 * @brief 生命領域魔法の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST時は std::nullopt を返す。
 */
std::optional<std::string> do_life_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    bool info = mode == SpellProcessType::INFO;
    bool cast = mode == SpellProcessType::CAST;

    DIRECTION dir;
    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0: {
        const Dice dice(2, 10);
        if (info) {
            return info_heal(dice);
        }
        if (cast) {
            (void)cure_light_wounds(player_ptr, dice.roll());
        }
    } break;

    case 1: {
        int base = 12;
        const Dice dice(1, 12);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_blessed(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 2: {
        const Dice dice(3 + (plev - 1) / 5, 4);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }
            fire_ball_hide(player_ptr, AttributeType::WOUNDS, dir, dice.roll(), 0);
        }
    } break;

    case 3: {
        const Dice dice(2, plev / 2);
        POSITION rad = plev / 10 + 1;

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            lite_area(player_ptr, dice.roll(), rad);
        }
    } break;

    case 4: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_traps(player_ptr, rad, true);
            detect_doors(player_ptr, rad);
            detect_stairs(player_ptr, rad);
        }
    } break;

    case 5: {
        const Dice dice(4, 10);

        if (info) {
            return info_heal(dice);
        }
        if (cast) {
            (void)cure_serious_wounds(player_ptr, dice.roll());
        }
    } break;

    case 6:
        if (cast) {
            (void)BadStatusSetter(player_ptr).set_poison(0);
        }

        break;

    case 7: {
        if (cast) {
            set_food(player_ptr, PY_FOOD_MAX - 1);
        }
    } break;

    case 8: {
        if (cast) {
            (void)remove_curse(player_ptr);
        }
    } break;

    case 9: {
        const Dice dice(8 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }
            fire_ball_hide(player_ptr, AttributeType::WOUNDS, dir, dice.roll(), 0);
        }
    } break;

    case 10: {
        const Dice dice(8, 10);

        if (info) {
            return info_heal(dice);
        }
        if (cast) {
            (void)cure_critical_wounds(player_ptr, dice.roll());
        }
    } break;

    case 11: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_cold(player_ptr, dice.roll() + base, false);
            set_oppose_fire(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 12: {
        POSITION rad = DETECT_RAD_MAP;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            map_area(player_ptr, rad);
        }
    } break;

    case 13: {
        if (cast) {
            turn_undead(player_ptr);
        }
    } break;

    case 14: {
        int heal = 300;
        if (info) {
            return info_heal(heal);
        }
        if (cast) {
            (void)cure_critical_wounds(player_ptr, heal);
        }
    } break;

    case 15: {
        if (cast) {
            create_rune_protection_one(player_ptr);
        }
    } break;

    case 16: {
        if (cast) {
            (void)remove_all_curse(player_ptr);
        }
    } break;

    case 17: {
        if (cast) {
            if (!ident_spell(player_ptr, false)) {
                return std::nullopt;
            }
        }
    } break;

    case 18: {
        const Dice dice(1, plev * 5);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            dispel_undead(player_ptr, dice.roll());
        }
    } break;

    case 19: {
        int power = plev * 2;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            charm_monsters(player_ptr, power);
        }
    } break;

    case 20: {
        const Dice dice(5 + (plev - 5) / 3, 15);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }
            fire_ball_hide(player_ptr, AttributeType::WOUNDS, dir, dice.roll(), 0);
        }
    } break;

    case 21: {
        int base = 15;
        const Dice dice(1, 20);

        if (info) {
            return info_delay(base, dice);
        }

        if (cast) {
            if (!recall_player(player_ptr, dice.roll() + base)) {
                return std::nullopt;
            }
        }
    } break;

    case 22: {
        int base = 15;
        const Dice dice(1, 20);

        if (info) {
            return info_delay(base, dice);
        }

        if (cast) {
            reserve_alter_reality(player_ptr, dice.roll() + base);
        }
    } break;

    case 23: {
        POSITION rad = 1;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            create_rune_protection_one(player_ptr);
            create_rune_protection_area(player_ptr, player_ptr->y, player_ptr->x);
        }
    } break;

    case 24: {
        if (cast) {
            player_ptr->current_floor_ptr->num_repro += MAX_REPRODUCTION;
        }
    } break;

    case 25: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_all(player_ptr, rad);
        }
    } break;

    case 26: {
        int power = plev + 50;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            mass_genocide_undead(player_ptr, power, true);
        }
    } break;

    case 27: {
        if (cast) {
            wiz_lite(player_ptr, false);
        }
    } break;

    case 28: {
        if (cast) {
            (void)restore_all_status(player_ptr);
            restore_level(player_ptr);
        }
    } break;

    case 29: {
        int heal = 2000;
        if (info) {
            return info_heal(heal);
        }
        if (cast) {
            (void)cure_critical_wounds(player_ptr, heal);
        }
    } break;

    case 30: {
        if (cast) {
            if (!identify_fully(player_ptr, false)) {
                return std::nullopt;
            }
        }
    } break;

    case 31: {
        TIME_EFFECT base = (TIME_EFFECT)plev / 2;
        const Dice dice(1, plev / 2);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            const auto v = static_cast<TIME_EFFECT>(dice.roll() + base);
            set_acceleration(player_ptr, v, false);
            set_oppose_acid(player_ptr, v, false);
            set_oppose_elec(player_ptr, v, false);
            set_oppose_fire(player_ptr, v, false);
            set_oppose_cold(player_ptr, v, false);
            set_oppose_pois(player_ptr, v, false);
            set_ultimate_res(player_ptr, v, false);
        }
    } break;
    }

    return "";
}
