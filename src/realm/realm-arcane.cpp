#include "realm/realm-arcane.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "effect/attribute-types.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "player/digestion-processor.h"
#include "realm/realm-types.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-arcane.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/element-resistance.h"
#include "status/sight-setter.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/dice.h"
#include "view/display-messages.h"

/*!
 * @brief 秘術領域魔法の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST時は std::nullopt を返す。
 */
std::optional<std::string> do_arcane_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    bool info = mode == SpellProcessType::INFO;
    bool cast = mode == SpellProcessType::CAST;

    DIRECTION dir;
    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0: {
        const Dice dice(3 + (plev - 1) / 5, 3);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, AttributeType::ELEC, dir, dice.roll());
        }
    } break;

    case 1: {
        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            wizard_lock(player_ptr, dir);
        }
    } break;

    case 2: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_monsters_invis(player_ptr, rad);
        }
    } break;

    case 3: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_monsters_normal(player_ptr, rad);
        }
    } break;

    case 4: {
        POSITION range = 10;

        if (info) {
            return info_range(range);
        }

        if (cast) {
            teleport_player(player_ptr, range, TELEPORT_SPONTANEOUS);
        }
    } break;

    case 5: {
        const Dice dice(2, plev / 2);
        POSITION rad = plev / 10 + 1;

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            lite_area(player_ptr, dice.roll(), rad);
        }
    } break;

    case 6: {
        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            destroy_door(player_ptr, dir);
        }
    } break;

    case 7: {
        const Dice dice(2, 8);

        if (info) {
            return info_heal(dice);
        }
        if (cast) {
            (void)cure_light_wounds(player_ptr, dice.roll());
        }
    } break;

    case 8: {
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

    case 9: {
        if (cast) {
            phlogiston(player_ptr);
        }
    } break;

    case 10: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_treasure(player_ptr, rad);
            detect_objects_gold(player_ptr, rad);
        }
    } break;

    case 11: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_objects_magic(player_ptr, rad);
        }
    } break;

    case 12: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_objects_normal(player_ptr, rad);
        }
    } break;

    case 13:
        if (cast) {
            (void)BadStatusSetter(player_ptr).set_poison(0);
        }

        break;

    case 14: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_cold(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 15: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_fire(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 16: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_elec(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 17: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_acid(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 18: {
        const Dice dice(4, 8);

        if (info) {
            return info_heal(dice);
        }
        if (cast) {
            (void)cure_serious_wounds(player_ptr, dice.roll());
        }
    } break;

    case 19: {
        POSITION range = plev * 5;

        if (info) {
            return info_range(range);
        }

        if (cast) {
            teleport_player(player_ptr, range, TELEPORT_SPONTANEOUS);
        }
    } break;

    case 20: {
        if (cast) {
            if (!ident_spell(player_ptr, false)) {
                return std::nullopt;
            }
        }
    } break;

    case 21: {
        const Dice dice(1, 30);
        int base = 20;

        if (info) {
            return info_damage(dice, base);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            wall_to_mud(player_ptr, dir, base + dice.roll());
        }
    } break;

    case 22: {
        const Dice dice(6, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            msg_print(_("光線が放たれた。", "A line of light appears."));
            lite_line(player_ptr, dir, dice.roll());
        }
    } break;

    case 23: {
        if (cast) {
            set_food(player_ptr, PY_FOOD_MAX - 1);
        }
    } break;

    case 24: {
        int base = 24;
        const Dice dice(1, 24);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_invis(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 25: {
        if (cast) {
            if (!summon_specific(player_ptr, player_ptr->y, player_ptr->x, plev, SUMMON_ELEMENTAL, (PM_ALLOW_GROUP | PM_FORCE_PET))) {
                msg_print(_("エレメンタルは現れなかった。", "No elementals arrive."));
            }
        }
    } break;

    case 26: {
        if (cast) {
            if (!input_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)"))) {
                return std::nullopt;
            }
            teleport_level(player_ptr, 0);
        }
    } break;

    case 27: {
        int power = plev;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            fire_beam(player_ptr, AttributeType::AWAY_ALL, dir, power);
        }
    } break;

    case 28: {
        int dam = 75 + plev;
        POSITION rad = 2;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            constexpr static auto element_types = {
                AttributeType::FIRE,
                AttributeType::ELEC,
                AttributeType::COLD,
                AttributeType::ACID,
            };

            const auto type = rand_choice(element_types);
            fire_ball(player_ptr, type, dir, dam, rad);
        }
    } break;

    case 29: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_all(player_ptr, rad);
        }
    } break;

    case 30: {
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

    case 31: {
        int base = 25;
        const Dice dice(1, 30);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            chg_virtue(player_ptr, Virtue::KNOWLEDGE, 1);
            chg_virtue(player_ptr, Virtue::ENLIGHTEN, 1);

            wiz_lite(player_ptr, false);

            if (!player_ptr->telepathy) {
                set_tim_esp(player_ptr, dice.roll() + base, false);
            }
        }
    } break;
    }

    return "";
}
