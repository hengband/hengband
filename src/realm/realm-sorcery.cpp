#include "realm/realm-sorcery.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "effect/attribute-types.h"
#include "player-info/self-info.h"
#include "realm/realm-types.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-fetcher.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-sorcery.h"
#include "spell/spells-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/sight-setter.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/dice.h"
#include "view/display-messages.h"

/*!
 * @brief 仙術領域魔法の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST時は tl::nullopt を返す。
 */
tl::optional<std::string> do_sorcery_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
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
            detect_monsters_normal(player_ptr, rad);
        }
    } break;

    case 1: {
        POSITION range = 10;

        if (info) {
            return info_range(range);
        }

        if (cast) {
            teleport_player(player_ptr, range, TELEPORT_SPONTANEOUS);
        }
    } break;

    case 2: {
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
        PLAYER_LEVEL power = (plev * 3) / 2;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            confuse_monster(player_ptr, dir, power);
        }
    } break;

    case 5: {
        POSITION range = plev * 5;

        if (info) {
            return info_range(range);
        }

        if (cast) {
            teleport_player(player_ptr, range, TELEPORT_SPONTANEOUS);
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

            sleep_monster(player_ptr, dir, plev);
        }
    } break;

    case 7: {
        int power = plev * 4;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            if (!recharge(player_ptr, power)) {
                return tl::nullopt;
            }
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
        if (cast) {
            if (!ident_spell(player_ptr, false)) {
                return tl::nullopt;
            }
        }
    } break;

    case 10: {
        int power = plev;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            slow_monster(player_ptr, dir, plev);
        }
    } break;

    case 11: {
        int power = plev;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            sleep_monsters(player_ptr, plev);
        }
    } break;

    case 12: {
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

    case 13: {
        int base = plev;
        const Dice dice(1, 20 + plev);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_acceleration(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 14: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_all(player_ptr, rad);
        }
    } break;

    case 15: {
        if (cast) {
            if (!identify_fully(player_ptr, false)) {
                return tl::nullopt;
            }
        }
    } break;

    case 16: {
        POSITION rad = DETECT_RAD_DEFAULT;

        if (info) {
            return info_radius(rad);
        }

        if (cast) {
            detect_objects_normal(player_ptr, rad);
            detect_treasure(player_ptr, rad);
            detect_objects_gold(player_ptr, rad);
        }
    } break;

    case 17: {
        int power = plev;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            charm_monster(player_ptr, dir, plev);
        }
    } break;

    case 18: {
        int base = 25;
        const Dice dice(1, 30);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_esp(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 19: {
        if (cast) {
            if (!tele_town(player_ptr)) {
                return tl::nullopt;
            }
        }
    } break;

    case 20: {
        if (cast) {
            self_knowledge(player_ptr);
        }
    } break;

    case 21: {
        if (cast) {
            if (!input_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)"))) {
                return tl::nullopt;
            }
            teleport_level(player_ptr, 0);
        }
    } break;

    case 22: {
        int base = 15;
        const Dice dice(1, 20);

        if (info) {
            return info_delay(base, dice);
        }

        if (cast) {
            if (!recall_player(player_ptr, dice.roll() + base)) {
                return tl::nullopt;
            }
        }
    } break;

    case 23: {
        POSITION range = plev / 2 + 10;

        if (info) {
            return info_range(range);
        }

        if (cast) {
            msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
            if (!dimension_door(player_ptr)) {
                return tl::nullopt;
            }
        }
    } break;

    case 24: {
        if (cast) {
            probing(player_ptr);
        }
    } break;

    case 25: {
        const Dice dice(7, 7);
        int base = plev;

        if (info) {
            return info_damage(dice, base);
        }

        if (cast) {
            create_rune_explosion(player_ptr, player_ptr->y, player_ptr->x);
        }
    } break;

    case 26: {
        WEIGHT weight = plev * 15;

        if (info) {
            return info_weight(weight);
        }

        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fetch_item(player_ptr, dir, weight, false);
        }
    } break;

    case 27: {
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

    case 28: {
        int power = plev * 2;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            charm_monsters(player_ptr, power);
        }
    } break;

    case 29: {
        if (cast) {
            if (!alchemy(player_ptr)) {
                return tl::nullopt;
            }
        }
    } break;

    case 30: {
        int power = plev * 4;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            banish_monsters(player_ptr, power);
        }
    } break;

    case 31: {
        int base = 4;
        const Dice dice(1, 4);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_invuln(player_ptr, dice.roll() + base, false);
        }
    } break;
    }

    return "";
}
