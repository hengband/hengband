#include "realm/realm-sorcery.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "player-info/self-info.h"
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
#include "spell/spell-types.h"
#include "spell/spells-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/sight-setter.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief 仙術領域魔法の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SPELL_NAME / SPELL_DESC / SPELL_INFO / SPELL_CAST)
 * @return SPELL_NAME / SPELL_DESC / SPELL_INFO 時には文字列ポインタを返す。SPELL_CAST時はnullptr文字列を返す。
 */
concptr do_sorcery_spell(player_type *player_ptr, SPELL_IDX spell, spell_type mode)
{
    bool name = mode == SPELL_NAME;
    bool desc = mode == SPELL_DESCRIPTION;
    bool info = mode == SPELL_INFO;
    bool cast = mode == SPELL_CAST;

    DIRECTION dir;
    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0:
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

    case 1:
        if (name)
            return _("ショート・テレポート", "Phase Door");
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

    case 2:
        if (name)
            return _("罠と扉感知", "Detect Doors and Traps");
        if (desc)
            return _("近くの全ての扉と罠を感知する。", "Detects traps, doors, and stairs in your vicinity.");

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

    case 3:
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

    case 4:
        if (name)
            return _("パニック・モンスター", "Confuse Monster");
        if (desc)
            return _("モンスター1体を混乱させる。抵抗されると無効。", "Attempts to confuse a monster.");

        {
            PLAYER_LEVEL power = (plev * 3) / 2;

            if (info)
                return info_power(power);

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir))
                    return nullptr;

                confuse_monster(player_ptr, dir, power);
            }
        }
        break;

    case 5:
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

    case 6:
        if (name)
            return _("スリープ・モンスター", "Sleep Monster");
        if (desc)
            return _("モンスター1体を眠らせる。抵抗されると無効。", "Attempts to put a monster to sleep.");

        {
            int power = plev;

            if (info)
                return info_power(power);

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir))
                    return nullptr;

                sleep_monster(player_ptr, dir, plev);
            }
        }
        break;

    case 7:
        if (name)
            return _("魔力充填", "Recharging");
        if (desc)
            return _("杖/魔法棒の充填回数を増やすか、充填中のロッドの充填時間を減らす。", "Recharges staffs, wands or rods.");

        {
            int power = plev * 4;

            if (info)
                return info_power(power);

            if (cast) {
                if (!recharge(player_ptr, power))
                    return nullptr;
            }
        }
        break;

    case 8:
        if (name)
            return _("魔法の地図", "Magic Mapping");
        if (desc)
            return _("周辺の地形を感知する。", "Maps nearby area.");

        {
            POSITION rad = DETECT_RAD_MAP;

            if (info)
                return info_radius(rad);

            if (cast) {
                map_area(player_ptr, rad);
            }
        }
        break;

    case 9:
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

    case 10:
        if (name)
            return _("スロウ・モンスター", "Slow Monster");
        if (desc)
            return _("モンスター1体を減速させる。抵抗されると無効。", "Attempts to slow a monster.");

        {
            int power = plev;

            if (info)
                return info_power(power);

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir))
                    return nullptr;

                slow_monster(player_ptr, dir, plev);
            }
        }
        break;

    case 11:
        if (name)
            return _("周辺スリープ", "Mass Sleep");
        if (desc)
            return _("視界内の全てのモンスターを眠らせる。抵抗されると無効。", "Attempts to put all monsters in sight to sleep.");

        {
            int power = plev;

            if (info)
                return info_power(power);

            if (cast) {
                sleep_monsters(player_ptr, plev);
            }
        }
        break;

    case 12:
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

    case 13:
        if (name)
            return _("スピード", "Haste Self");
        if (desc)
            return _("一定時間、加速する。", "Hastes you for a while.");

        {
            int base = plev;
            DICE_SID sides = 20 + plev;

            if (info)
                return info_duration(base, sides);

            if (cast) {
                set_fast(player_ptr, randint1(sides) + base, false);
            }
        }
        break;

    case 14:
        if (name)
            return _("真・感知", "Detection True");
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

    case 15:
        if (name)
            return _("真・鑑定", "Identify True");
        if (desc)
            return _("アイテムの持つ能力を完全に知る。", "*Identifies* an item.");

        {
            if (cast) {
                if (!identify_fully(player_ptr, false))
                    return nullptr;
            }
        }
        break;

    case 16:
        if (name)
            return _("物体と財宝感知", "Detect items and Treasure");
        if (desc)
            return _("近くの全てのアイテムと財宝を感知する。", "Detects all treasures and items in your vicinity.");

        {
            POSITION rad = DETECT_RAD_DEFAULT;

            if (info)
                return info_radius(rad);

            if (cast) {
                detect_objects_normal(player_ptr, rad);
                detect_treasure(player_ptr, rad);
                detect_objects_gold(player_ptr, rad);
            }
        }
        break;

    case 17:
        if (name)
            return _("チャーム・モンスター", "Charm Monster");
        if (desc)
            return _("モンスター1体を魅了する。抵抗されると無効。", "Attempts to charm a monster.");

        {
            int power = plev;

            if (info)
                return info_power(power);

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir))
                    return nullptr;

                charm_monster(player_ptr, dir, plev);
            }
        }
        break;

    case 18:
        if (name)
            return _("精神感知", "Sense Minds");
        if (desc)
            return _("一定時間、テレパシー能力を得る。", "Gives telepathy for a while.");

        {
            int base = 25;
            DICE_SID sides = 30;

            if (info)
                return info_duration(base, sides);

            if (cast) {
                set_tim_esp(player_ptr, randint1(sides) + base, false);
            }
        }
        break;

    case 19:
        if (name)
            return _("街移動", "Teleport to town");
        if (desc)
            return _("街へ移動する。地上にいるときしか使えない。", "Instantly teleports you to a town which you choose. Can only be used outdoors.");

        {
            if (cast) {
                if (!tele_town(player_ptr))
                    return nullptr;
            }
        }
        break;

    case 20:
        if (name)
            return _("自己分析", "Self Knowledge");
        if (desc)
            return _("現在の自分の状態を完全に知る。",
                "Gives you useful info regarding your current resistances, the powers of your weapon and maximum limits of your stats.");

        {
            if (cast) {
                self_knowledge(player_ptr);
            }
        }
        break;

    case 21:
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

    case 22:
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

    case 23:
        if (name)
            return _("次元の扉", "Dimension Door");
        if (desc)
            return _("短距離内の指定した場所にテレポートする。", "Teleports you to a given location.");

        {
            POSITION range = plev / 2 + 10;

            if (info)
                return info_range(range);

            if (cast) {
                msg_print(_("次元の扉が開いた。目的地を選んで下さい。", "You open a dimensional gate. Choose a destination."));
                if (!dimension_door(player_ptr))
                    return nullptr;
            }
        }
        break;

    case 24:
        if (name)
            return _("調査", "Probing");
        if (desc)
            return _("モンスターの属性、残り体力、最大体力、スピード、正体を知る。", "Probes all monsters' alignment, HP, speed and their true character.");

        {
            if (cast) {
                probing(player_ptr);
            }
        }
        break;

    case 25:
        if (name)
            return _("爆発のルーン", "Explosive Rune");
        if (desc)
            return _("自分のいる床の上に、モンスターが通ると爆発してダメージを与えるルーンを描く。",
                "Sets a rune under you. The rune will explode when a monster moves on it.");

        {
            DICE_NUMBER dice = 7;
            DICE_SID sides = 7;
            int base = plev;

            if (info)
                return info_damage(dice, sides, base);

            if (cast) {
                create_rune_explosion(player_ptr, player_ptr->y, player_ptr->x);
            }
        }
        break;

    case 26:
        if (name)
            return _("念動力", "Telekinesis");
        if (desc)
            return _("アイテムを自分の足元へ移動させる。", "Pulls a distant item close to you.");

        {
            WEIGHT weight = plev * 15;

            if (info)
                return info_weight(weight);

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir))
                    return nullptr;

                fetch_item(player_ptr, dir, weight, false);
            }
        }
        break;

    case 27:
        if (name)
            return _("千里眼", "Clairvoyance");
        if (desc)
            return _("その階全体を永久に照らし、ダンジョン内すべてのアイテムを感知する。さらに、一定時間テレパシー能力を得る。",
                "Maps and lights whole dungeon level. Knows all objects location. And gives telepathy for a while.");

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

    case 28:
        if (name)
            return _("魅了の視線", "Charm monsters");
        if (desc)
            return _("視界内の全てのモンスターを魅了する。抵抗されると無効。", "Attempts to charm all monsters in sight.");

        {
            int power = plev * 2;

            if (info)
                return info_power(power);

            if (cast) {
                charm_monsters(player_ptr, power);
            }
        }
        break;

    case 29:
        if (name)
            return _("錬金術", "Alchemy");
        if (desc)
            return _("アイテム1つをお金に変える。", "Turns an item into 1/3 of its value in gold.");

        {
            if (cast) {
                if (!alchemy(player_ptr))
                    return nullptr;
            }
        }
        break;

    case 30:
        if (name)
            return _("怪物追放", "Banishment");
        if (desc)
            return _("視界内の全てのモンスターをテレポートさせる。抵抗されると無効。", "Teleports all monsters in sight away unless resisted.");

        {
            int power = plev * 4;

            if (info)
                return info_power(power);

            if (cast) {
                banish_monsters(player_ptr, power);
            }
        }
        break;

    case 31:
        if (name)
            return _("無傷の球", "Globe of Invulnerability");
        if (desc)
            return _("一定時間、ダメージを受けなくなるバリアを張る。切れた瞬間に少しターンを消費するので注意。",
                "Generates a barrier which completely protects you from almost all damage. Takes a few of your turns when the barrier breaks or duration time "
                "is exceeded.");

        {
            int base = 4;

            if (info)
                return info_duration(base, base);

            if (cast) {
                set_invuln(player_ptr, randint1(base) + base, false);
            }
        }
        break;
    }

    return "";
}
