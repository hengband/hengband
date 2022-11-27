#include "realm/realm-nature.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-object.h"
#include "hpmp/hp-mp-processor.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "object/object-kind-hook.h"
#include "player-attack/player-attack.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/race-types.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-beam.h"
#include "spell-kind/spells-charm.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-realm/spells-nature.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "sv-definition/sv-food-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief 自然領域魔法の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列ポインタを返す。SpellProcessType::CAST時はnullptr文字列を返す。
 */
concptr do_nature_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    bool name = mode == SpellProcessType::NAME;
    bool desc = mode == SpellProcessType::DESCRIPTION;
    bool info = mode == SpellProcessType::INFO;
    bool cast = mode == SpellProcessType::CAST;

    DIRECTION dir;
    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0:
        if (name) {
            return _("モンスター感知", "Detect Creatures");
        }
        if (desc) {
            return _("近くの全ての見えるモンスターを感知する。", "Detects all monsters in your vicinity unless invisible.");
        }

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info) {
                return info_radius(rad);
            }

            if (cast) {
                detect_monsters_normal(player_ptr, rad);
            }
        }
        break;

    case 1:
        if (name) {
            return _("稲妻", "Lightning");
        }
        if (desc) {
            return _("電撃の短いビームを放つ。", "Fires a short beam of lightning.");
        }

        {
            DICE_NUMBER dice = 3 + (plev - 1) / 5;
            DICE_SID sides = 4;
            POSITION range = plev / 6 + 2;

            if (info) {
                return format("%s%dd%d %s%d", KWD_DAM, dice, sides, KWD_RANGE, range);
            }

            if (cast) {
                project_length = range;

                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_beam(player_ptr, AttributeType::ELEC, dir, damroll(dice, sides));
            }
        }
        break;

    case 2:
        if (name) {
            return _("罠と扉感知", "Detect Doors and Traps");
        }
        if (desc) {
            return _("近くの全ての罠と扉を感知する。", "Detects traps, doors, and stairs in your vicinity.");
        }

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info) {
                return info_radius(rad);
            }

            if (cast) {
                detect_traps(player_ptr, rad, true);
                detect_doors(player_ptr, rad);
                detect_stairs(player_ptr, rad);
            }
        }
        break;

    case 3:
        if (name) {
            return _("食糧生成", "Produce Food");
        }
        if (desc) {
            return _("食料を一つ作り出す。", "Produces a Ration of Food.");
        }

        {
            if (cast) {
                ItemEntity forge, *q_ptr = &forge;
                msg_print(_("食料を生成した。", "A food ration is produced."));

                /* Create the food ration */
                q_ptr->prep(lookup_baseitem_id({ ItemKindType::FOOD, SV_FOOD_RATION }));

                /* Drop the object from heaven */
                (void)drop_near(player_ptr, q_ptr, -1, player_ptr->y, player_ptr->x);
            }
        }
        break;

    case 4:
        if (name) {
            return _("日の光", "Daylight");
        }
        if (desc) {
            return _("光源が照らしている範囲か部屋全体を永久に明るくする。", "Lights up nearby area and the inside of a room permanently.");
        }

        {
            DICE_NUMBER dice = 2;
            DICE_SID sides = plev / 2;
            POSITION rad = (plev / 10) + 1;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                lite_area(player_ptr, damroll(dice, sides), rad);

                PlayerRace race(player_ptr);
                if (race.life() == PlayerRaceLifeType::UNDEAD && race.tr_flags().has(TR_VUL_LITE) && !has_resist_lite(player_ptr)) {
                    msg_print(_("日の光があなたの肉体を焦がした！", "The daylight scorches your flesh!"));
                    take_hit(player_ptr, DAMAGE_NOESCAPE, damroll(2, 2), _("日の光", "daylight"));
                }
            }
        }
        break;

    case 5:
        if (name) {
            return _("動物習し", "Animal Taming");
        }
        if (desc) {
            return _("動物1体を魅了する。抵抗されると無効。", "Attempts to charm an animal.");
        }

        {
            int power = plev;

            if (info) {
                return info_power(power);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                charm_animal(player_ptr, dir, plev);
            }
        }
        break;

    case 6:
        if (name) {
            return _("環境への耐性", "Resist Environment");
        }
        if (desc) {
            return _("一定時間、冷気、炎、電撃に対する耐性を得る。装備による耐性に累積する。",
                "Gives resistance to fire, cold and electricity for a while. These resistances can be added to those from equipment for more powerful "
                "resistances.");
        }

        {
            int base = 20;

            if (info) {
                return info_duration(base, base);
            }

            if (cast) {
                set_oppose_cold(player_ptr, randint1(base) + base, false);
                set_oppose_fire(player_ptr, randint1(base) + base, false);
                set_oppose_elec(player_ptr, randint1(base) + base, false);
            }
        }
        break;

    case 7:
        if (name) {
            return _("傷と毒治療", "Cure Wounds & Poison");
        }
        if (desc) {
            return _("怪我を全快させ、毒を体から完全に取り除き、体力を少し回復させる。", "Heals all cuts and poisons. Heals HP a little.");
        }

        {
            DICE_NUMBER dice = 2;
            DICE_SID sides = 8;

            if (info) {
                return info_heal(dice, sides, 0);
            }

            if (cast) {
                BadStatusSetter bss(player_ptr);
                hp_player(player_ptr, damroll(dice, sides));
                (void)bss.set_cut(0);
                (void)bss.set_poison(0);
            }
        }
        break;

    case 8:
        if (name) {
            return _("岩石溶解", "Stone to Mud");
        }
        if (desc) {
            return _("壁を溶かして床にする。", "Turns one rock square to mud.");
        }

        {
            DICE_NUMBER dice = 1;
            DICE_SID sides = 30;
            int base = 20;

            if (info) {
                return info_damage(dice, sides, base);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                wall_to_mud(player_ptr, dir, 20 + randint1(30));
            }
        }
        break;

    case 9:
        if (name) {
            return _("アイス・ボルト", "Frost Bolt");
        }
        if (desc) {
            return _("冷気のボルトもしくはビームを放つ。", "Fires a bolt or beam of cold.");
        }

        {
            DICE_NUMBER dice = 3 + (plev - 5) / 4;
            DICE_SID sides = 8;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }
                fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, AttributeType::COLD, dir, damroll(dice, sides));
            }
        }
        break;

    case 10:
        if (name) {
            return _("自然の覚醒", "Nature Awareness");
        }
        if (desc) {
            return _(
                "周辺の地形を感知し、近くの罠、扉、階段、全てのモンスターを感知する。", "Maps nearby area. Detects all monsters, traps, doors and stairs.");
        }

        {
            int rad1 = DETECT_RAD_MAP;
            int rad2 = DETECT_RAD_DEFAULT;

            if (info) {
                return info_radius(std::max(rad1, rad2));
            }

            if (cast) {
                map_area(player_ptr, rad1);
                detect_traps(player_ptr, rad2, true);
                detect_doors(player_ptr, rad2);
                detect_stairs(player_ptr, rad2);
                detect_monsters_normal(player_ptr, rad2);
            }
        }
        break;

    case 11:
        if (name) {
            return _("ファイア・ボルト", "Fire Bolt");
        }
        if (desc) {
            return _("火炎のボルトもしくはビームを放つ。", "Fires a bolt or beam of fire.");
        }

        {
            DICE_NUMBER dice = 5 + (plev - 5) / 4;
            DICE_SID sides = 8;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }
                fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, AttributeType::FIRE, dir, damroll(dice, sides));
            }
        }
        break;

    case 12:
        if (name) {
            return _("太陽光線", "Ray of Sunlight");
        }
        if (desc) {
            return _("光線を放つ。光りを嫌うモンスターに効果がある。", "Fires a beam of light which damages light-sensitive monsters.");
        }

        {
            DICE_NUMBER dice = 6;
            DICE_SID sides = 8;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }
                msg_print(_("太陽光線が現れた。", "A line of sunlight appears."));
                lite_line(player_ptr, dir, damroll(6, 8));
            }
        }
        break;

    case 13:
        if (name) {
            return _("足かせ", "Entangle");
        }
        if (desc) {
            return _("視界内の全てのモンスターを減速させる。抵抗されると無効。", "Attempts to slow all monsters in sight.");
        }
        {
            int power = plev;
            if (info) {
                return info_power(power);
            }
            if (cast) {
                slow_monsters(player_ptr, plev);
            }
        }
        break;

    case 14:
        if (name) {
            return _("動物召喚", "Summon Animal");
        }
        if (desc) {
            return _("動物を1体召喚する。", "Summons an animal.");
        }

        {
            if (cast) {
                if (!(summon_specific(player_ptr, -1, player_ptr->y, player_ptr->x, plev, SUMMON_ANIMAL_RANGER, (PM_ALLOW_GROUP | PM_FORCE_PET)))) {
                    msg_print(_("動物は現れなかった。", "No animals arrive."));
                }
                break;
            }
        }
        break;

    case 15:
        if (name) {
            return _("薬草治療", "Herbal Healing");
        }
        if (desc) {
            return _("体力を大幅に回復させ、負傷、朦朧状態、毒から全快する。", "Heals HP greatly. Completely cures cuts, poisons and being stunned.");
        }
        {
            int heal = 500;
            if (info) {
                return info_heal(0, 0, heal);
            }
            if (cast) {
                (void)cure_critical_wounds(player_ptr, heal);
            }
        }
        break;

    case 16:
        if (name) {
            return _("階段生成", "Stair Building");
        }
        if (desc) {
            return _("自分のいる位置に階段を作る。", "Creates a staircase which goes down or up.");
        }

        {
            if (cast) {
                stair_creation(player_ptr);
            }
        }
        break;

    case 17:
        if (name) {
            return _("肌石化", "Stone Skin");
        }
        if (desc) {
            return _("一定時間、ACを上昇させる。", "Gives a bonus to AC for a while.");
        }

        {
            int base = 20;
            DICE_SID sides = 30;

            if (info) {
                return info_duration(base, sides);
            }

            if (cast) {
                set_shield(player_ptr, randint1(sides) + base, false);
            }
        }
        break;

    case 18:
        if (name) {
            return _("真・耐性", "Resistance True");
        }
        if (desc) {
            return _("一定時間、酸、電撃、炎、冷気、毒に対する耐性を得る。装備による耐性に累積する。",
                "Gives resistance to fire, cold, electricity, acid and poison for a while. These resistances can be added to those from equipment for more "
                "powerful resistances.");
        }

        {
            int base = 20;

            if (info) {
                return info_duration(base, base);
            }

            if (cast) {
                set_oppose_acid(player_ptr, randint1(base) + base, false);
                set_oppose_elec(player_ptr, randint1(base) + base, false);
                set_oppose_fire(player_ptr, randint1(base) + base, false);
                set_oppose_cold(player_ptr, randint1(base) + base, false);
                set_oppose_pois(player_ptr, randint1(base) + base, false);
            }
        }
        break;

    case 19:
        if (name) {
            return _("森林創造", "Forest Creation");
        }
        if (desc) {
            return _("周囲に木を作り出す。", "Creates trees in all adjacent squares.");
        }

        {
            if (cast) {
                tree_creation(player_ptr, player_ptr->y, player_ptr->x);
            }
        }
        break;

    case 20:
        if (name) {
            return _("動物友和", "Animal Friendship");
        }
        if (desc) {
            return _("視界内の全ての動物を魅了する。抵抗されると無効。", "Attempts to charm all animals in sight.");
        }

        {
            int power = plev * 2;
            if (info) {
                return info_power(power);
            }
            if (cast) {
                charm_animals(player_ptr, power);
            }
        }
        break;

    case 21:
        if (name) {
            return _("試金石", "Stone Tell");
        }
        if (desc) {
            return _("アイテムの持つ能力を完全に知る。", "*Identifies* an item.");
        }

        {
            if (cast) {
                if (!identify_fully(player_ptr, false)) {
                    return nullptr;
                }
            }
        }
        break;

    case 22:
        if (name) {
            return _("石の壁", "Wall of Stone");
        }
        if (desc) {
            return _("自分の周囲に花崗岩の壁を作る。", "Creates granite walls in all adjacent squares.");
        }

        {
            if (cast) {
                wall_stone(player_ptr);
            }
        }
        break;

    case 23:
        if (name) {
            return _("腐食防止", "Protect from Corrosion");
        }
        if (desc) {
            return _("アイテムを酸で傷つかないよう加工する。", "Makes a piece of equipment acid-proof.");
        }

        {
            if (cast) {
                if (!rustproof(player_ptr)) {
                    return nullptr;
                }
            }
        }
        break;

    case 24:
        if (name) {
            return _("地震", "Earthquake");
        }
        if (desc) {
            return _(
                "周囲のダンジョンを揺らし、壁と床をランダムに入れ変える。", "Shakes dungeon structure, and results in random swapping of floors and walls.");
        }

        {
            POSITION rad = 10;

            if (info) {
                return info_radius(rad);
            }

            if (cast) {
                earthquake(player_ptr, player_ptr->y, player_ptr->x, rad, 0);
            }
        }
        break;

    case 25:
        if (name) {
            return _("カマイタチ", "Whirlwind");
        }
        if (desc) {
            return _("全方向に向かって攻撃する。", "Attacks all adjacent monsters.");
        }
        if (cast) {
            massacre(player_ptr);
        }
        break;

    case 26:
        if (name) {
            return _("ブリザード", "Blizzard");
        }
        if (desc) {
            return _("巨大な冷気の球を放つ。", "Fires a huge ball of cold.");
        }

        {
            int dam = 70 + plev * 3 / 2;
            POSITION rad = plev / 12 + 1;

            if (info) {
                return info_damage(0, 0, dam);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_ball(player_ptr, AttributeType::COLD, dir, dam, rad);
            }
        }
        break;

    case 27:
        if (name) {
            return _("稲妻嵐", "Lightning Storm");
        }
        if (desc) {
            return _("巨大な電撃の球を放つ。", "Fires a huge electric ball.");
        }

        {
            int dam = 90 + plev * 3 / 2;
            POSITION rad = plev / 12 + 1;

            if (info) {
                return info_damage(0, 0, dam);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }
                fire_ball(player_ptr, AttributeType::ELEC, dir, dam, rad);
                break;
            }
        }
        break;

    case 28:
        if (name) {
            return _("渦潮", "Whirlpool");
        }
        if (desc) {
            return _("巨大な水の球を放つ。", "Fires a huge ball of water.");
        }

        {
            int dam = 100 + plev * 3 / 2;
            POSITION rad = plev / 12 + 1;

            if (info) {
                return info_damage(0, 0, dam);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }
                fire_ball(player_ptr, AttributeType::WATER, dir, dam, rad);
            }
        }
        break;

    case 29:
        if (name) {
            return _("陽光召喚", "Call Sunlight");
        }
        if (desc) {
            return _("自分を中心とした光の球を発生させる。さらに、その階全体を永久に照らし、ダンジョン内すべてのアイテムを感知する。",
                "Generates ball of light centered on you. Maps and lights whole dungeon level. Knows all objects location.");
        }

        {
            int dam = 150;
            POSITION rad = 8;

            if (info) {
                return info_damage(0, 0, dam / 2);
            }

            if (cast) {
                fire_ball(player_ptr, AttributeType::LITE, 0, dam, rad);
                chg_virtue(player_ptr, V_KNOWLEDGE, 1);
                chg_virtue(player_ptr, V_ENLIGHTEN, 1);
                wiz_lite(player_ptr, false);

                PlayerRace race(player_ptr);
                if (race.life() == PlayerRaceLifeType::UNDEAD && race.tr_flags().has(TR_VUL_LITE) && !has_resist_lite(player_ptr)) {
                    msg_print(_("日光があなたの肉体を焦がした！", "The sunlight scorches your flesh!"));
                    take_hit(player_ptr, DAMAGE_NOESCAPE, 50, _("日光", "sunlight"));
                }
            }
        }
        break;

    case 30:
        if (name) {
            return _("精霊の刃", "Elemental Branding");
        }
        if (desc) {
            return _("武器に炎か冷気の属性をつける。", "Brands current weapon with fire or frost.");
        }

        {
            if (cast) {
                brand_weapon(player_ptr, randint0(2));
            }
        }
        break;

    case 31:
        if (name) {
            return _("自然の脅威", "Nature's Wrath");
        }
        if (desc) {
            return _("近くの全てのモンスターにダメージを与え、地震を起こし、自分を中心とした分解の球を発生させる。",
                "Damages all monsters in sight. Makes quake. Generates disintegration ball centered on you.");
        }

        {
            int d_dam = 4 * plev;
            int b_dam = (100 + plev) * 2;
            POSITION b_rad = 1 + plev / 12;
            POSITION q_rad = 20 + plev / 2;

            if (info) {
                return format("%s%d+%d", KWD_DAM, d_dam, b_dam / 2);
            }

            if (cast) {
                dispel_monsters(player_ptr, d_dam);
                earthquake(player_ptr, player_ptr->y, player_ptr->x, q_rad, 0);
                project(player_ptr, 0, b_rad, player_ptr->y, player_ptr->x, b_dam, AttributeType::DISINTEGRATE, PROJECT_KILL | PROJECT_ITEM);
            }
        }
        break;
    }

    return "";
}
