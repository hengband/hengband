#include "realm/realm-crusade.h"
#include "cmd-action/cmd-spell.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/floor-util.h"
#include "hpmp/hp-mp-processor.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-crusade.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/sight-setter.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "util/dice.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 破邪領域魔法の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST時は std::nullopt を返す。
 */
std::optional<std::string> do_crusade_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
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
            fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, AttributeType::ELEC, dir, dice.roll());
        }
    } break;

    case 1: {
        POSITION rad = DETECT_RAD_DEFAULT;
        if (info) {
            return info_radius(rad);
        }
        if (cast) {
            detect_monsters_evil(player_ptr, rad);
        }
    } break;

    case 2:
        if (cast) {
            (void)BadStatusSetter(player_ptr).set_fear(0);
        }

        break;
    case 3: {
        PLAYER_LEVEL power = plev;
        if (info) {
            return info_power(power);
        }
        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }
            fear_monster(player_ptr, dir, power);
        }
    } break;

    case 4: {
        PLAYER_LEVEL power = plev;
        if (info) {
            return info_power(power);
        }
        if (cast) {
            sleep_monsters_touch(player_ptr);
        }
    } break;

    case 5: {
        POSITION range = 25 + plev / 2;
        if (info) {
            return info_range(range);
        }
        if (cast) {
            teleport_player(player_ptr, range, TELEPORT_SPONTANEOUS);
        }
    } break;

    case 6: {
        const Dice dice(3 + (plev - 1) / 9, 2);
        if (info) {
            return info_multi_damage_dice(dice);
        }
        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }
            fire_blast(player_ptr, AttributeType::LITE, dir, dice, 10, 3);
        }
    } break;

    case 7:
        if (cast) {
            BadStatusSetter bss(player_ptr);
            (void)bss.set_cut(0);
            (void)bss.set_poison(0);
            (void)bss.set_stun(0);
        }

        break;

    case 8: {
        int power = MAX_PLAYER_SIGHT * 5;
        if (info) {
            return info_power(power);
        }
        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }
            fire_ball(player_ptr, AttributeType::AWAY_EVIL, dir, power, 0);
        }
    } break;

    case 9: {
        const Dice dice(3, 6);
        POSITION rad = (plev < 30) ? 2 : 3;
        int base;
        PlayerClass pc(player_ptr);
        if (pc.equals(PlayerClassType::PRIEST) || pc.equals(PlayerClassType::HIGH_MAGE) || pc.equals(PlayerClassType::SORCERER)) {
            base = plev + plev / 2;
        } else {
            base = plev + plev / 4;
        }

        if (info) {
            return info_damage(dice, base);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            fire_ball(player_ptr, AttributeType::HOLY_FIRE, dir, dice.roll() + base, rad);
        }
    } break;

    case 10: {
        const Dice dice(1, plev);
        int power = plev;
        if (info) {
            return info_damage(dice);
        }
        if (cast) {
            dispel_undead(player_ptr, dice.roll());
            dispel_demons(player_ptr, dice.roll());
            turn_evil(player_ptr, power);
        }
    } break;

    case 11: {
        if (cast) {
            (void)remove_curse(player_ptr);
        }
    } break;

    case 12: {
        int base = 24;
        const Dice dice(1, 24);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_invis(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 13: {
        int base = 25;
        const Dice dice(1, 3 * plev);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_protevil(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 14: {
        int dam = plev * 5;

        if (info) {
            return info_damage(dam);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }
            fire_bolt(player_ptr, AttributeType::ELEC, dir, dam);
        }
    } break;

    case 15: {
        auto dam_sides = plev * 6;
        auto heal = 100;
        if (info) {
            return format(_("損:1d%d/回%d", "dam:d%d/h%d"), dam_sides, heal);
        }

        if (cast) {
            BadStatusSetter bss(player_ptr);
            dispel_evil(player_ptr, randint1(dam_sides));
            hp_player(player_ptr, heal);
            (void)bss.set_fear(0);
            (void)bss.set_poison(0);
            (void)bss.set_stun(0);
            (void)bss.set_cut(0);
        }

        break;
    }
    case 16: {
        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }

            destroy_door(player_ptr, dir);
        }
    } break;

    case 17: {
        int power = plev * 2;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            if (!get_aim_dir(player_ptr, &dir)) {
                return std::nullopt;
            }
            stasis_evil(player_ptr, dir);
        }
    } break;

    case 18: {
        int base = 20;
        const Dice dice(1, 20);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_sh_holy(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 19: {
        const Dice dice(1, plev * 4);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            dispel_undead(player_ptr, dice.roll());
            dispel_demons(player_ptr, dice.roll());
        }
    } break;

    case 20: {
        const Dice dice(1, plev * 4);

        if (info) {
            return info_damage(dice);
        }

        if (cast) {
            dispel_evil(player_ptr, dice.roll());
        }
    } break;

    case 21: {
        if (cast) {
            brand_weapon(player_ptr, 13);
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

            fire_ball(player_ptr, AttributeType::LITE, dir, dam, rad);
        }
    } break;

    case 23: {
        if (cast) {
            bool pet = !one_in_(3);
            uint32_t flg = 0L;

            if (pet) {
                flg |= PM_FORCE_PET;
            } else {
                flg |= PM_NO_PET;
            }
            if (!(pet && (plev < 50))) {
                flg |= PM_ALLOW_GROUP;
            }

            if (summon_specific(player_ptr, player_ptr->y, player_ptr->x, (plev * 3) / 2, SUMMON_ANGEL, flg)) {
                if (pet) {
                    msg_print(_("「ご用でございますか、ご主人様」", "'What is thy bidding... Master?'"));
                } else {
                    msg_print(_("「我は汝の下僕にあらず！ 悪行者よ、悔い改めよ！」", "Mortal! Repent of thy impiousness."));
                }
            }
        }
    } break;

    case 24: {
        int base = 25;
        const Dice dice(1, 25);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            (void)heroism(player_ptr, dice.roll() + base);
        }
    } break;

    case 25: {
        if (cast) {
            (void)remove_all_curse(player_ptr);
        }
    } break;

    case 26: {
        int power = 100;

        if (info) {
            return info_power(power);
        }

        if (cast) {
            if (banish_evil(player_ptr, power)) {
                msg_print(_("神聖な力が邪悪を打ち払った！", "The holy power banishes evil!"));
            }
        }
    } break;

    case 27: {
        int base = 12;
        const Dice dice(1, 4);

        if (cast) {
            destroy_area(player_ptr, player_ptr->y, player_ptr->x, base + dice.roll(), false);
        }
    } break;

    case 28: {
        int base = 10;
        const Dice dice(1, 10);

        if (info) {
            return info_duration(base, dice);
        }

        if (cast) {
            set_tim_eyeeye(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 29: {
        int dam = plev * 3 + 25;
        POSITION rad = 2;

        if (info) {
            return info_multi_damage(dam);
        }

        if (cast) {
            if (!cast_wrath_of_the_god(player_ptr, dam, rad)) {
                return std::nullopt;
            }
        }
    } break;

    case 30: {
        int b_dam = plev * 11;
        int d_dam = plev * 4;
        int heal = 100;
        int power = plev * 4;

        if (info) {
            return format(_("回%d/損%d+%d", "h%d/dm%d+%d"), heal, d_dam, b_dam / 2);
        }
        if (cast) {
            project(player_ptr, 0, 1, player_ptr->y, player_ptr->x, b_dam, AttributeType::HOLY_FIRE, PROJECT_KILL);
            dispel_monsters(player_ptr, d_dam);
            slow_monsters(player_ptr, plev);
            stun_monsters(player_ptr, power);
            confuse_monsters(player_ptr, power);
            turn_monsters(player_ptr, power);
            stasis_monsters(player_ptr, power);
            hp_player(player_ptr, heal);
        }
    } break;

    case 31:
        if (cast) {
            auto base = 25;
            auto sp_sides = 20 + plev;
            auto sp_base = plev;
            crusade(player_ptr);
            for (auto i = 0; i < 12; i++) {
                auto attempt = 10;
                POSITION my = 0, mx = 0;
                while (attempt--) {
                    scatter(player_ptr, &my, &mx, player_ptr->y, player_ptr->x, 4, PROJECT_NONE);
                    if (is_cave_empty_bold2(player_ptr, my, mx)) {
                        break;
                    }
                }

                if (attempt < 0) {
                    continue;
                }

                summon_specific(player_ptr, my, mx, plev, SUMMON_KNIGHTS, PM_ALLOW_GROUP | PM_FORCE_PET | PM_HASTE);
            }

            set_hero(player_ptr, randint1(base) + base, false);
            set_blessed(player_ptr, randint1(base) + base, false);
            set_acceleration(player_ptr, randint1(sp_sides) + sp_base, false);
            set_protevil(player_ptr, randint1(base) + base, false);
            (void)BadStatusSetter(player_ptr).set_fear(0);
        }

        break;
    }

    return "";
}
