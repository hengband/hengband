#include "realm/realm-arcane.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "player/digestion-processor.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-arcane.h"
#include "spell/spell-types.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/element-resistance.h"
#include "status/sight-setter.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief 秘術領域魔法の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SPELL_NAME / SPELL_DESC / SPELL_INFO / SPELL_CAST)
 * @return SPELL_NAME / SPELL_DESC / SPELL_INFO 時には文字列ポインタを返す。SPELL_CAST時はnullptr文字列を返す。
 */
concptr do_arcane_spell(player_type *player_ptr, SPELL_IDX spell, spell_type mode)
{
    bool name = (mode == SPELL_NAME) ? true : false;
    bool desc = (mode == SPELL_DESC) ? true : false;
    bool info = (mode == SPELL_INFO) ? true : false;
    bool cast = (mode == SPELL_CAST) ? true : false;

    DIRECTION dir;
    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0:
        if (name)
            return _("電撃", "Zap");
        if (desc)
            return _("電撃のボルトもしくはビームを放つ。", "Fires a bolt or beam of lightning.");

        {
            DICE_NUMBER dice = 3 + (plev - 1) / 5;
            DICE_SID sides = 3;

            if (info)
                return info_damage(dice, sides, 0);

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir))
                    return nullptr;

                fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, GF_ELEC, dir, damroll(dice, sides));
            }
        }
        break;

    case 1:
        if (name)
            return _("魔法の施錠", "Wizard Lock");
        if (desc)
            return _("扉に鍵をかける。", "Locks a door.");

        {
            if (cast) {
                if (!get_aim_dir(player_ptr, &dir))
                    return nullptr;

                wizard_lock(player_ptr, dir);
            }
        }
        break;

    case 2:
        if (name)
            return _("透明体感知", "Detect Invisibility");
        if (desc)
            return _("近くの透明なモンスターを感知する。", "Detects all invisible monsters in your vicinity.");

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info)
                return info_radius(rad);

            if (cast) {
                detect_monsters_invis(player_ptr, rad);
            }
        }
        break;

    case 3:
        if (name)
            return _("モンスター感知", "Detect Monsters");
        if (desc)
            return _("近くの全ての見えるモンスターを感知する。", "Detects all monsters in your vicinity unless invisible.");

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info)
                return info_radius(rad);

            if (cast) {
                detect_monsters_normal(player_ptr, rad);
            }
        }
        break;

    case 4:
        if (name)
            return _("ショート・テレポート", "Blink");
        if (desc)
            return _("近距離のテレポートをする。", "Teleports you a short distance.");

        {
            POSITION range = 10;

            if (info)
                return info_range(range);

            if (cast) {
                teleport_player(player_ptr, range, TELEPORT_SPONTANEOUS);
            }
        }
        break;

    case 5:
        if (name)
            return _("ライト・エリア", "Light Area");
        if (desc)
            return _("光源が照らしている範囲か部屋全体を永久に明るくする。", "Lights up nearby area and the inside of a room permanently.");

        {
            DICE_NUMBER dice = 2;
            DICE_SID sides = plev / 2;
            POSITION rad = plev / 10 + 1;

            if (info)
                return info_damage(dice, sides, 0);

            if (cast) {
                lite_area(player_ptr, damroll(dice, sides), rad);
            }
        }
        break;

    case 6:
        if (name)
            return _("罠と扉 破壊", "Trap & Door Destruction");
        if (desc)
            return _("一直線上の全ての罠と扉を破壊する。", "Fires a beam which destroy traps and doors.");

        {
            if (cast) {
                if (!get_aim_dir(player_ptr, &dir))
                    return nullptr;

                destroy_door(player_ptr, dir);
            }
        }
        break;

    case 7:
        if (name)
            return _("軽傷の治癒", "Cure Light Wounds");
        if (desc)
            return _("怪我と体力を少し回復させる。", "Heals cuts and HP a little.");

        {
            DICE_NUMBER dice = 2;
            DICE_SID sides = 8;

            if (info)
                return info_heal(dice, sides, 0);
            if (cast)
                (void)cure_light_wounds(player_ptr, dice, sides);
        }
        break;

    case 8:
        if (name)
            return _("罠と扉 感知", "Detect Doors & Traps");
        if (desc)
            return _("近くの全ての罠と扉と階段を感知する。", "Detects traps, doors, and stairs in your vicinity.");

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info)
                return info_radius(rad);

            if (cast) {
                detect_traps(player_ptr, rad, true);
                detect_doors(player_ptr, rad);
                detect_stairs(player_ptr, rad);
            }
        }
        break;

    case 9:
        if (name)
            return _("燃素", "Phlogiston");
        if (desc)
            return _("光源に燃料を補給する。", "Adds more turns of light to a lantern or torch.");

        {
            if (cast) {
                phlogiston(player_ptr);
            }
        }
        break;

    case 10:
        if (name)
            return _("財宝感知", "Detect Treasure");
        if (desc)
            return _("近くの財宝を感知する。", "Detects all treasures in your vicinity.");

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info)
                return info_radius(rad);

            if (cast) {
                detect_treasure(player_ptr, rad);
                detect_objects_gold(player_ptr, rad);
            }
        }
        break;

    case 11:
        if (name)
            return _("魔法 感知", "Detect Enchantment");
        if (desc)
            return _("近くの魔法がかかったアイテムを感知する。", "Detects all magical items in your vicinity.");

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info)
                return info_radius(rad);

            if (cast) {
                detect_objects_magic(player_ptr, rad);
            }
        }
        break;

    case 12:
        if (name)
            return _("アイテム感知", "Detect Objects");
        if (desc)
            return _("近くの全てのアイテムを感知する。", "Detects all items in your vicinity.");

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info)
                return info_radius(rad);

            if (cast) {
                detect_objects_normal(player_ptr, rad);
            }
        }
        break;

    case 13:
        if (name)
            return _("解毒", "Cure Poison");
        if (desc)
            return _("毒を体内から完全に取り除く。", "Cures yourself of any poisons.");

        {
            if (cast) {
                set_poisoned(player_ptr, 0);
            }
        }
        break;

    case 14:
        if (name)
            return _("耐冷", "Resist Cold");
        if (desc)
            return _("一定時間、冷気への耐性を得る。装備による耐性に累積する。",
                "Gives resistance to cold. This resistance can be added to that from equipment for more powerful resistance.");

        {
            int base = 20;

            if (info)
                return info_duration(base, base);

            if (cast) {
                set_oppose_cold(player_ptr, randint1(base) + base, false);
            }
        }
        break;

    case 15:
        if (name)
            return _("耐火", "Resist Fire");
        if (desc)
            return _("一定時間、炎への耐性を得る。装備による耐性に累積する。",
                "Gives resistance to fire. This resistance can be added to that from equipment for more powerful resistance.");

        {
            int base = 20;

            if (info)
                return info_duration(base, base);

            if (cast) {
                set_oppose_fire(player_ptr, randint1(base) + base, false);
            }
        }
        break;

    case 16:
        if (name)
            return _("耐電", "Resist Lightning");
        if (desc)
            return _("一定時間、電撃への耐性を得る。装備による耐性に累積する。",
                "Gives resistance to electricity. This resistance can be added to that from equipment for more powerful resistance.");

        {
            int base = 20;

            if (info)
                return info_duration(base, base);

            if (cast) {
                set_oppose_elec(player_ptr, randint1(base) + base, false);
            }
        }
        break;

    case 17:
        if (name)
            return _("耐酸", "Resist Acid");
        if (desc)
            return _("一定時間、酸への耐性を得る。装備による耐性に累積する。",
                "Gives resistance to acid. This resistance can be added to that from equipment for more powerful resistance.");

        {
            int base = 20;

            if (info)
                return info_duration(base, base);

            if (cast) {
                set_oppose_acid(player_ptr, randint1(base) + base, false);
            }
        }
        break;

    case 18:
        if (name)
            return _("重傷の治癒", "Cure Medium Wounds");
        if (desc)
            return _("怪我と体力を中程度回復させる。", "Heals cuts and HP.");

        {
            DICE_NUMBER dice = 4;
            DICE_SID sides = 8;

            if (info)
                return info_heal(dice, sides, 0);
            if (cast)
                (void)cure_serious_wounds(player_ptr, 4, 8);
        }
        break;

    case 19:
        if (name)
            return _("テレポート", "Teleport");
        if (desc)
            return _("遠距離のテレポートをする。", "Teleports you a long distance.");

        {
            POSITION range = plev * 5;

            if (info)
                return info_range(range);

            if (cast) {
                teleport_player(player_ptr, range, TELEPORT_SPONTANEOUS);
            }
        }
        break;

    case 20:
        if (name)
            return _("鑑定", "Identify");
        if (desc)
            return _("アイテムを識別する。", "Identifies an item.");

        {
            if (cast) {
                if (!ident_spell(player_ptr, false))
                    return nullptr;
            }
        }
        break;

    case 21:
        if (name)
            return _("岩石溶解", "Stone to Mud");
        if (desc)
            return _("壁を溶かして床にする。", "Turns one rock square to mud.");

        {
            DICE_NUMBER dice = 1;
            DICE_SID sides = 30;
            int base = 20;

            if (info)
                return info_damage(dice, sides, base);

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir))
                    return nullptr;

                wall_to_mud(player_ptr, dir, 20 + randint1(30));
            }
        }
        break;

    case 22:
        if (name)
            return _("閃光", "Ray of Light");
        if (desc)
            return _("光線を放つ。光りを嫌うモンスターに効果がある。", "Fires a beam of light which damages to light-sensitive monsters.");

        {
            DICE_NUMBER dice = 6;
            DICE_SID sides = 8;

            if (info)
                return info_damage(dice, sides, 0);

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir))
                    return nullptr;

                msg_print(_("光線が放たれた。", "A line of light appears."));
                lite_line(player_ptr, dir, damroll(6, 8));
            }
        }
        break;

    case 23:
        if (name)
            return _("空腹充足", "Satisfy Hunger");
        if (desc)
            return _("満腹にする。", "Satisfies hunger.");

        {
            if (cast) {
                set_food(player_ptr, PY_FOOD_MAX - 1);
            }
        }
        break;

    case 24:
        if (name)
            return _("透明視認", "See Invisible");
        if (desc)
            return _("一定時間、透明なものが見えるようになる。", "Gives see invisible for a while.");

        {
            int base = 24;

            if (info)
                return info_duration(base, base);

            if (cast) {
                set_tim_invis(player_ptr, randint1(base) + base, false);
            }
        }
        break;

    case 25:
        if (name)
            return _("エレメンタル召喚", "Conjure Elemental");
        if (desc)
            return _("1体のエレメンタルを召喚する。", "Summons an elemental.");
        {
            if (cast) {
                if (!summon_specific(player_ptr, -1, player_ptr->y, player_ptr->x, plev, SUMMON_ELEMENTAL, (PM_ALLOW_GROUP | PM_FORCE_PET))) {
                    msg_print(_("エレメンタルは現れなかった。", "No elementals arrive."));
                }
            }
        }
        break;

    case 26:
        if (name)
            return _("テレポート・レベル", "Teleport Level");
        if (desc)
            return _("瞬時に上か下の階にテレポートする。", "Instantly teleports you up or down a level.");

        {
            if (cast) {
                if (!get_check(_("本当に他の階にテレポートしますか？", "Are you sure? (Teleport Level)")))
                    return nullptr;
                teleport_level(player_ptr, 0);
            }
        }
        break;

    case 27:
        if (name)
            return _("テレポート・モンスター", "Teleport Away");
        if (desc)
            return _("モンスターをテレポートさせるビームを放つ。抵抗されると無効。", "Teleports all monsters on the line away unless resisted.");

        {
            int power = plev;

            if (info)
                return info_power(power);

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir))
                    return nullptr;

                fire_beam(player_ptr, GF_AWAY_ALL, dir, power);
            }
        }
        break;

    case 28:
        if (name)
            return _("元素の球", "Elemental Ball");
        if (desc)
            return _("炎、電撃、冷気、酸のどれかの球を放つ。", "Fires a ball of some elements.");

        {
            HIT_POINT dam = 75 + plev;
            POSITION rad = 2;

            if (info)
                return info_damage(0, 0, dam);

            if (cast) {
                int type;

                if (!get_aim_dir(player_ptr, &dir))
                    return nullptr;

                switch (randint1(4)) {
                case 1:
                    type = GF_FIRE;
                    break;
                case 2:
                    type = GF_ELEC;
                    break;
                case 3:
                    type = GF_COLD;
                    break;
                default:
                    type = GF_ACID;
                    break;
                }

                fire_ball(player_ptr, type, dir, dam, rad);
            }
        }
        break;

    case 29:
        if (name)
            return _("全感知", "Detection");
        if (desc)
            return _("近くの全てのモンスター、罠、扉、階段、財宝、そしてアイテムを感知する。",
                "Detects all monsters, traps, doors, stairs, treasures and items in your vicinity.");

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info)
                return info_radius(rad);

            if (cast) {
                detect_all(player_ptr, rad);
            }
        }
        break;

    case 30:
        if (name)
            return _("帰還の呪文", "Word of Recall");
        if (desc)
            return _("地上にいるときはダンジョンの最深階へ、ダンジョンにいるときは地上へと移動する。",
                "Recalls player from dungeon to town or from town to the deepest level of dungeon.");

        {
            int base = 15;
            DICE_SID sides = 20;

            if (info)
                return info_delay(base, sides);

            if (cast) {
                if (!recall_player(player_ptr, randint0(21) + 15))
                    return nullptr;
            }
        }
        break;

    case 31:
        if (name)
            return _("千里眼", "Clairvoyance");
        if (desc)
            return _("その階全体を永久に照らし、ダンジョン内すべてのアイテムを感知する。さらに、一定時間テレパシー能力を得る。",
                "Maps and lights whole dungeon level. Reveals locations of all objects. Gives telepathy for a while.");

        {
            int base = 25;
            DICE_SID sides = 30;

            if (info)
                return info_duration(base, sides);

            if (cast) {
                chg_virtue(player_ptr, V_KNOWLEDGE, 1);
                chg_virtue(player_ptr, V_ENLIGHTEN, 1);

                wiz_lite(player_ptr, false);

                if (!player_ptr->telepathy) {
                    set_tim_esp(player_ptr, randint1(sides) + base, false);
                }
            }
        }
        break;
    }

    return "";
}
