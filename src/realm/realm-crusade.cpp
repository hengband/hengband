#include "realm/realm-crusade.h"
#include "blue-magic/blue-magic-caster.h"
#include "cmd-action/cmd-spell.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/floor-util.h"
#include "hpmp/hp-mp-processor.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
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
#include "status/temporary-resistance.h"
#include "system/floor/floor-info.h"
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
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST時は tl::nullopt を返す。
 */
tl::optional<std::string> do_crusade_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    const auto info = mode == SpellProcessType::INFO;
    const auto cast = mode == SpellProcessType::CAST;

    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0: {
        const Dice dice(3 + (plev - 1) / 5, 4);
        if (info) {
            return info_damage(dice);
        }
        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }
            fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, AttributeType::LITE, dir, dice.roll());
        }
    } break;

    case 1: {
        const POSITION rad = DETECT_RAD_DEFAULT;
        if (info) {
            return info_radius(rad);
        }
        if (cast) {
            detect_monsters_evil(player_ptr, rad);
        }
    } break;

    case 2: {
        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            destroy_door(player_ptr, dir);
        }
    } break;

    case 3: {
        const auto base = 20;
        const Dice dice(1, 20);
        if (info) {
            return info_duration(base, dice);
        }
        if (cast) {
            set_tim_res_lite(player_ptr, dice.roll() + base, false);
            set_tim_res_dark(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 4: {
        const Dice dice(6 + (plev - 5) / 4, 8);
        if (info) {
            return info_damage(dice);
        }
        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }
            fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, AttributeType::ELEC, dir, dice.roll());
        }
    } break;

    case 5: {
        const POSITION range = 25 + plev / 2;
        if (info) {
            return info_range(range);
        }
        if (cast) {
            teleport_player(player_ptr, range, TELEPORT_SPONTANEOUS);
        }
    } break;

    case 6: {
        if (cast) {
            BadStatusSetter bss(player_ptr);
            (void)bss.set_cut(0);
            (void)bss.set_poison(0);
            (void)bss.set_stun(0);
        }
    } break;

    case 7: {
        if (cast) {
            (void)remove_curse(player_ptr);
        }
    } break;

    case 8: {
        const auto base = 25;
        const Dice dice(1, 25);
        if (info) {
            return info_duration(base, dice);
        }
        if (cast) {
            (void)heroism(player_ptr, dice.roll() + base);
        }
    } break;

    case 9: {
        const auto power = MAX_PLAYER_SIGHT * 5;
        if (info) {
            return info_power(power);
        }
        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }
            fire_ball(player_ptr, AttributeType::AWAY_EVIL, dir, power, 0);
        }
    } break;

    case 10: {
        const Dice dice(3, 6);
        const POSITION rad = (plev < 30) ? 2 : 3;
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
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }
            fire_ball(player_ptr, AttributeType::HOLY_FIRE, dir, dice.roll() + base, rad);
        }
    } break;

    case 11: {
        const auto base = 20;
        const Dice dice(1, 20);
        if (info) {
            return info_duration(base, dice);
        }
        if (cast) {
            set_tim_emission(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 12: {
        const auto base = 25;
        const Dice dice(1, 3 * plev);
        if (info) {
            return info_duration(base, dice);
        }
        if (cast) {
            BodyImprovement(player_ptr).set_protection(dice.roll() + base);
        }
    } break;

    case 13: {
        if (cast) {
            (void)remove_all_curse(player_ptr);
        }
    } break;

    case 14: {
        const auto dam = plev * 3;
        if (info) {
            return info_damage(dam);
        }
        if (cast) {
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }
            fire_bolt(player_ptr, AttributeType::LITE, dir, dam);
        }
    } break;

    case 15: {
        const auto dam_sides = plev * 6;
        const auto heal = 100;
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
    } break;

    case 16: {
        const auto base = 20;
        const Dice dice(1, 20);
        if (info) {
            return info_duration(base, dice);
        }
        if (cast) {
            set_tim_sh_holy(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 17: {
        const auto base = 20;
        const Dice dice(1, 20);
        if (info) {
            return info_duration(base, dice);
        }
        if (cast) {
            set_tim_exorcism(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 18: {
        if (cast) {
            brand_weapon(player_ptr, 13);
        }
    } break;

    case 19: {
        const auto base = 10 + plev / 2;
        const Dice dice(1, 10 + plev / 2);
        if (info) {
            return info_duration(base, dice);
        }
        if (cast) {
            set_mimic(player_ptr, base + dice.roll(), MimicKindType::ANGEL, false);
        }
    } break;

    case 20: {
        const auto dam = 200 + plev * 2;
        if (info) {
            return info_damage(dam);
        }
        if (cast) {
            const POSITION rad = 4;
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }
            fire_ball(player_ptr, AttributeType::ELEC, dir, dam, rad);
        }
    } break;

    case 21: {
        const auto dam = 100 + plev * 2;
        if (info) {
            return info_damage(dam);
        }
        if (cast) {
            const POSITION rad = 4;
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }

            fire_ball(player_ptr, AttributeType::LITE, dir, dam, rad);
        }
    } break;

    case 22: {
        if (cast) {
            cast_blue_dispel(player_ptr);
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
        const auto power = 100;
        if (info) {
            return info_power(power);
        }
        if (cast) {
            if (banish_evil(player_ptr, power)) {
                msg_print(_("神聖な力が邪悪を打ち払った！", "The holy power banishes evil!"));
            }
        }
    } break;

    case 25: {
        const auto base = 12;
        const Dice dice(1, 4);
        if (cast) {
            destroy_area(player_ptr, player_ptr->y, player_ptr->x, base + dice.roll(), false);
        }
    } break;

    case 26: {
        const auto base = 10;
        const Dice dice(1, 10);
        if (info) {
            return info_duration(base, dice);
        }
        if (cast) {
            set_tim_eyeeye(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 27: {
        const auto base = 20;
        const Dice dice(1, 20);
        if (info) {
            return info_duration(base, dice);
        }
        if (cast) {
            set_tim_imm_dark(player_ptr, dice.roll() + base, false);
        }
    } break;

    case 28: {
        const auto dam = (plev * 3 + 25) * 16 / 7;
        if (info) {
            return info_damage(dam);
        }
        if (cast) {
            const POSITION rad = 7;
            const auto dir = get_aim_dir(player_ptr);
            fire_ball(player_ptr, AttributeType::DISINTEGRATE, dir, dam, rad);
        }
    } break;

    case 29: {
        const auto dam = plev * 15;
        if (info) {
            return info_damage(dam);
        }
        if (cast) {
            const POSITION rad = plev / 5;
            const auto dir = get_aim_dir(player_ptr);
            if (!dir) {
                return tl::nullopt;
            }
            fire_ball(player_ptr, AttributeType::ELEC, dir, dam, rad);
        }
    } break;

    case 30: {
        const auto b_dam = plev * 11;
        const auto d_dam = plev * 4;
        const auto heal = 100;
        const auto power = plev * 4;
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

    case 31: {
        const auto base = 10 + plev / 2;
        const Dice dice(1, 10 + plev / 2);
        if (info) {
            return info_duration(base, dice);
        }
        if (cast) {
            set_mimic(player_ptr, base + dice.roll(), MimicKindType::DEMIGOD, false);
        }
    } break;
    }

    return "";
}
