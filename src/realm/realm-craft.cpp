#include "realm/realm-craft.h"
#include "cmd-action/cmd-spell.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "player-info/self-info.h"
#include "player/digestion-processor.h"
#include "realm/realm-types.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-enchant.h"
#include "spell-kind/spells-perception.h"
#include "spell-realm/spells-craft.h"
#include "spell-realm/spells-trump.h"
#include "spell/range-calc.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/sight-setter.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 匠領域魔法の各処理を行う
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST時は std::nullopt を返す。
 */
std::optional<std::string> do_craft_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    bool info = mode == SpellProcessType::INFO;
    bool cast = mode == SpellProcessType::CAST;

    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0: {
        int base = 100;
        const Dice dice(1, 100);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_infra(player_ptr, base + dice.roll(), false);
        }
    } break;

    case 1: {
        int base = 80;
        const Dice dice(1, 80);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_regen(player_ptr, base + dice.roll(), false);
        }
    } break;

    case 2: {
        if (cast) {
            set_food(player_ptr, PY_FOOD_MAX - 1);
        }
    } break;

    case 3: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_cold(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 4: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_fire(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 5: {
        int base = 25;
        const Dice dice(1, 25);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            (void)heroism(player_ptr, dice.roll() + base);
        }
    } break;

    case 6: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_elec(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 7: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_acid(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 8: {
        int base = 24;
        const Dice dice(1, 24);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_invis(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 9: {
        if (cast) {
            (void)remove_curse(player_ptr);
        }
    } break;

    case 10: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_pois(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 11: {
        int base = 25;
        const Dice dice(1, 25);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            (void)berserk(player_ptr, base + dice.roll());
        }
    } break;

    case 12: {
        if (cast) {
            self_knowledge(player_ptr);
        }
    } break;

    case 13: {
        int base = 3 * plev;
        const Dice dice(1, 25);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_protevil(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 14: {
        if (cast) {
            (void)true_healing(player_ptr, 0);
        }
    } break;

    case 15: {
        int base = plev / 2;
        const Dice dice(1, plev / 2);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            if (!choose_ele_attack(player_ptr, base + dice.roll())) {
                return std::nullopt;
            }
        }
    } break;

    case 16: {
        int base = 25;
        const Dice dice(1, 30);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_esp(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 17: {
        int base = 30;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_shield(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 18: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_oppose_acid(player_ptr, dice.roll() + base, false);
            set_oppose_elec(player_ptr, dice.roll() + base, false);
            set_oppose_fire(player_ptr, dice.roll() + base, false);
            set_oppose_cold(player_ptr, dice.roll() + base, false);
            set_oppose_pois(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 19: {
        int base = plev;
        const Dice dice(1, 20 + plev);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_acceleration(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 20: {
        int base = plev / 2;
        const Dice dice(1, plev / 2);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_pass_wall(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 21: {
        if (cast) {
            pulish_shield(player_ptr);
        }
    } break;

    case 22: {
        if (cast) {
            if (summon_specific(player_ptr, player_ptr->y, player_ptr->x, plev, SUMMON_GOLEM, PM_FORCE_PET)) {
                msg_print(_("ゴーレムを作った。", "You make a golem."));
            } else {
                msg_print(_("うまくゴーレムを作れなかった。", "You couldn't make a golem."));
            }
        }
    } break;

    case 23: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_magicdef(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 24: {
        if (cast) {
            if (!mundane_spell(player_ptr, true)) {
                return std::nullopt;
            }
        }
    } break;

    case 25: {
        if (cast) {
            (void)remove_all_curse(player_ptr);
        }
    } break;

    case 26: {
        if (cast) {
            if (!identify_fully(player_ptr, false)) {
                return std::nullopt;
            }
        }
    } break;

    case 27: {
        if (cast) {
            if (!enchant_spell(player_ptr, randint0(4) + 1, randint0(4) + 1, 0)) {
                return std::nullopt;
            }
        }
    } break;

    case 28: {
        if (cast) {
            if (!enchant_spell(player_ptr, 0, 0, randint0(3) + 2)) {
                return std::nullopt;
            }
        }
    } break;

    case 29: {
        if (cast) {
            brand_weapon(player_ptr, randint0(18));
        }
    } break;

    case 30:
        if (cast) {
            become_living_trump(player_ptr);
        }
        break;

    case 31: {
        int base = 13;
        const Dice dice(1, 13);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            if (!choose_ele_immune(player_ptr, base + dice.roll())) {
                return std::nullopt;
            }
        }
    } break;
    }

    return "";
}
