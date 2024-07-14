#include "realm/realm-demon.h"
#include "cmd-action/cmd-spell.h"
#include "effect/attribute-types.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "player-base/player-class.h"
#include "player-info/race-info.h"
#include "player/player-damage.h"
#include "realm/realm-types.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-pet.h"
#include "spell-kind/spells-sight.h"
#include "spell-realm/spells-demon.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "status/temporary-resistance.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/dice.h"
#include "view/display-messages.h"

/*!
 * @brief 悪魔領域魔法の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST時は std::nullopt を返す。
 */
std::optional<std::string> do_daemon_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    bool info = mode == SpellProcessType::INFO;
    bool cast = mode == SpellProcessType::CAST;

    DIRECTION dir;
    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0: {
        const Dice dice(3 + (plev - 1) / 5, 4);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, AttributeType::MISSILE, dir, dice.roll());
        }
    } break;

    case 1: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_monsters_nonliving(player_ptr, rad);
        }
    } break;

    case 2: {
        int base = 12;
        const Dice dice(1, 12);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_blessed(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 3: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_fire(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 4: {
        const Dice dice(6 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            fire_bolt_or_beam(player_ptr, beam_chance(player_ptr), AttributeType::FIRE, dir, dice.roll());
        }
    } break;

    case 5: {
        if (cast) {
            if (!summon_specific(player_ptr, player_ptr->y, player_ptr->x, (plev * 3) / 2, SUMMON_MANES, (PM_ALLOW_GROUP | PM_FORCE_PET))) {
                msg_print(_("古代の死霊は現れなかった。", "No Manes arrive."));
            }
        }
    } break;

    case 6: {
        const Dice dice(6, 6);
        POSITION rad = (plev < 30) ? 2 : 3;
        int base;

        if (PlayerClass(player_ptr).is_wizard()) {
            base = plev * 3;
        } else {
            base = plev * 2 + plev / 2;
        }

        if (info) {
            return info_damage(dice, base);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            fire_ball(player_ptr, AttributeType::NETHER, dir, dice.roll() + base, rad);
        }
    } break;

    case 7: {
        int base = 25;
        const Dice dice(1, 25);
        if (info) {
            return info_duration(base, dice);
        }
        if (cast) {
            heroism(player_ptr, dice.roll() + base);
        }
    } break;

    case 8: {
        POSITION rad = DETECT_RAD_MAP;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            map_area(player_ptr, rad);
        }
    } break;

    case 9: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_res_nether(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 10: {
        const Dice dice(11 + (plev - 5) / 4, 8);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            fire_bolt_or_beam(player_ptr, beam_chance(player_ptr), AttributeType::PLASMA, dir, dice.roll());
        }
    } break;

    case 11: {
        int dam = plev + 55;
        POSITION rad = 2;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            fire_ball(player_ptr, AttributeType::FIRE, dir, dam, rad);
        }
    } break;

    case 12: {
        if (cast) {
            brand_weapon(player_ptr, 1);
        }
    } break;

    case 13: {
        int dam = plev * 5;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }
            fire_bolt(player_ptr, AttributeType::FIRE, dir, dam);
        }
    } break;

    case 14: {
        if (cast) {
            cast_summon_demon(player_ptr, plev * 2 / 3 + randint1(plev / 2));
        }
    } break;

    case 15: {
        int sides1 = plev * 2;
        int sides2 = plev * 2;

        if (info) {
            return format("%sd%d+d%d", KWD_DAM, sides1, sides2);
        }

        if (cast) {
            dispel_monsters(player_ptr, randint1(sides1));
            dispel_good(player_ptr, randint1(sides2));
        }
    } break;

    case 16: {
        int base = 30;
        const Dice dice(1, 25);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_esp(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 17: {
        TIME_EFFECT base = 20;
        const Dice dice(1, 20);
        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            const auto dur = static_cast<TIME_EFFECT>(dice.roll() + base);
            set_oppose_fire(player_ptr, dur, false);
            set_oppose_cold(player_ptr, dur, false);
            set_tim_sh_fire(player_ptr, dur, false);
            (void)BadStatusSetter(player_ptr).set_fear(0);
            break;
        }

        break;
    }
    case 18: {
        int dam = (55 + plev) * 2;
        POSITION rad = 3;

        if (info) {
            return info_damage(dam / 2);
        }

        if (cast) {
            fire_ball(player_ptr, AttributeType::FIRE, 0, dam, rad);
            fire_ball_hide(player_ptr, AttributeType::LAVA_FLOW, 0, 2 + randint1(2), rad);
        }
    } break;

    case 19: {
        int dam = plev * 3 / 2 + 80;
        POSITION rad = 2 + plev / 40;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            fire_ball(player_ptr, AttributeType::PLASMA, dir, dam, rad);
        }
    } break;

    case 20: {
        int base = 10 + plev / 2;
        const Dice dice(1, 10 + plev / 2);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_mimic(player_ptr, base + dice.roll(), MimicKindType::DEMON, false);
        }
    } break;

    case 21: {
        int dam = 200 + plev * 2;
        POSITION rad = 4;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }
            fire_ball(player_ptr, AttributeType::FIRE, dir, dam, rad);
        }
    } break;

    case 22: {
        int dam = 100 + plev * 2;
        POSITION rad = 4;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }
            fire_ball(player_ptr, AttributeType::NEXUS, dir, dam, rad);
        }
    } break;

    case 23: {
        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            } else {
                msg_print(_("<破滅の手>を放った！", "You invoke the Hand of Doom!"));
            }

            fire_ball_hide(player_ptr, AttributeType::HAND_DOOM, dir, plev * 2, 0);
        }
    } break;

    case 24: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_res_time(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 25: {
        int dam = 50 + plev;
        int power = 20 + plev;
        POSITION rad = 3 + plev / 20;

        if (info) {
            return format("%s%d+%d", KWD_DAM, dam / 2, dam / 2);
        }

        if (cast) {
            fire_ball(player_ptr, AttributeType::CHAOS, 0, dam, rad);
            fire_ball(player_ptr, AttributeType::CONFUSION, 0, dam, rad);
            fire_ball(player_ptr, AttributeType::CHARM, 0, power, rad);
        }
    } break;

    case 26: {
        if (cast) {
            discharge_minion(player_ptr);
        }
    } break;

    case 27: {
        if (cast) {
            if (!cast_summon_greater_demon(player_ptr)) {
                return std::nullopt;
            }
        }
    } break;

    case 28: {
        int dam = plev * 15;
        POSITION rad = plev / 5;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            fire_ball(player_ptr, AttributeType::FIRE, dir, dam, rad);
        }
    } break;

    case 29: {
        int dam = plev * 15;
        POSITION rad = plev / 5;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            fire_ball(player_ptr, AttributeType::NETHER, dir, dam, rad);
        }
    } break;

    case 30: {
        int dam = 600;
        POSITION rad = 0;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            fire_ball_hide(player_ptr, AttributeType::BLOOD_CURSE, dir, dam, rad);
            take_hit(player_ptr, DAMAGE_USELIFE, 20 + randint1(30), _("血の呪い", "Blood curse"));
        }
    } break;

    case 31: {
        int base = 15;
        const Dice dice(1, 15);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_mimic(player_ptr, base + dice.roll(), MimicKindType::DEMON_LORD, false);
        }
    } break;
    }

    return "";
}
