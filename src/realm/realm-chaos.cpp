#include "realm/realm-chaos.h"
#include "cmd-action/cmd-spell.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "player-base/player-class.h"
#include "player/attack-defense-types.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-random.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-chaos.h"
#include "spell/spells-diceroll.h"
#include "spell/spells-object.h"
#include "spell/spells-status.h"
#include "spell/spells-summon.h"
#include "status/shape-changer.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * @brief カオス領域魔法の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列ポインタを返す。SpellProcessType::CAST時はnullptr文字列を返す。
 */
concptr do_chaos_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
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
            return _("マジック・ミサイル", "Magic Missile");
        }
        if (desc) {
            return _("弱い魔法の矢を放つ。", "Fires a weak bolt of magic.");
        }

        {
            DICE_NUMBER dice = 3 + ((plev - 1) / 5);
            DICE_SID sides = 4;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_bolt_or_beam(player_ptr, beam_chance(player_ptr) - 10, AttributeType::MISSILE, dir, damroll(dice, sides));
            }
        }
        break;

    case 1:
        if (name) {
            return _("トラップ/ドア破壊", "Trap / Door Destruction");
        }
        if (desc) {
            return _("隣接する罠と扉を破壊する。", "Destroys all doors and traps in adjacent squares.");
        }

        {
            POSITION rad = 1;

            if (info) {
                return info_radius(rad);
            }

            if (cast) {
                destroy_doors_touch(player_ptr);
            }
        }
        break;

    case 2:
        if (name) {
            return _("閃光", "Flash of Light");
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
            }
        }
        break;

    case 3:
        if (name) {
            return _("混乱の手", "Touch of Confusion");
        }
        if (desc) {
            return _("相手を混乱させる攻撃をできるようにする。", "Attempts to confuse the next monster that you hit.");
        }

        {
            if (cast) {
                if (!(player_ptr->special_attack & ATTACK_CONFUSE)) {
                    msg_print(_("あなたの手は光り始めた。", "Your hands start glowing."));
                    player_ptr->special_attack |= ATTACK_CONFUSE;
                    player_ptr->redraw |= (PR_STATUS);
                }
            }
        }
        break;

    case 4:
        if (name) {
            return _("魔力炸裂", "Mana Burst");
        }
        if (desc) {
            return _("魔法の球を放つ。", "Fires a ball of magic.");
        }

        {
            DICE_NUMBER dice = 3;
            DICE_SID sides = 5;
            POSITION rad = (plev < 30) ? 2 : 3;
            int base;

            if (PlayerClass(player_ptr).is_wizard()) {
                base = plev + plev / 2;
            } else {
                base = plev + plev / 4;
            }

            if (info) {
                return info_damage(dice, sides, base);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_ball(player_ptr, AttributeType::MISSILE, dir, damroll(dice, sides) + base, rad);

                /*
                 * Shouldn't actually use MANA, as
                 * it will destroy all items on the
                 * floor
                 */
            }
        }
        break;

    case 5:
        if (name) {
            return _("ファイア・ボルト", "Fire Bolt");
        }
        if (desc) {
            return _("炎のボルトもしくはビームを放つ。", "Fires a bolt or beam of fire.");
        }

        {
            DICE_NUMBER dice = 8 + (plev - 5) / 4;
            DICE_SID sides = 8;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_bolt_or_beam(player_ptr, beam_chance(player_ptr), AttributeType::FIRE, dir, damroll(dice, sides));
            }
        }
        break;

    case 6:
        if (name) {
            return _("力の拳", "Fist of Force");
        }
        if (desc) {
            return _("ごく小さな分解の球を放つ。", "Fires a tiny ball of disintegration.");
        }

        {
            DICE_NUMBER dice = 8 + ((plev - 5) / 4);
            DICE_SID sides = 8;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_ball(player_ptr, AttributeType::DISINTEGRATE, dir, damroll(dice, sides), 0);
            }
        }
        break;

    case 7:
        if (name) {
            return _("テレポート", "Teleport Self");
        }
        if (desc) {
            return _("遠距離のテレポートをする。", "Teleports you a long distance.");
        }

        {
            POSITION range = plev * 5;

            if (info) {
                return info_range(range);
            }

            if (cast) {
                teleport_player(player_ptr, range, TELEPORT_SPONTANEOUS);
            }
        }
        break;

    case 8:
        if (name) {
            return _("ワンダー", "Wonder");
        }
        if (desc) {
            return _("モンスターにランダムな効果を与える。", "Fires something with random effects.");
        }

        {
            if (info) {
                return KWD_RANDOM;
            }

            if (cast) {

                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                cast_wonder(player_ptr, dir);
            }
        }
        break;

    case 9:
        if (name) {
            return _("カオス・ボルト", "Chaos Bolt");
        }
        if (desc) {
            return _("カオスのボルトもしくはビームを放つ。", "Fires a bolt or ball of chaos.");
        }

        {
            DICE_NUMBER dice = 10 + (plev - 5) / 4;
            DICE_SID sides = 8;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_bolt_or_beam(player_ptr, beam_chance(player_ptr), AttributeType::CHAOS, dir, damroll(dice, sides));
            }
        }
        break;

    case 10:
        if (name) {
            return _("ソニック・ブーム", "Sonic Boom");
        }
        if (desc) {
            return _("自分を中心とした轟音の球を発生させる。", "Generates a ball of sound centered on you.");
        }

        {
            int dam = 60 + plev;
            POSITION rad = plev / 10 + 2;

            if (info) {
                return info_damage(0, 0, dam / 2);
            }

            if (cast) {
                msg_print(_("ドーン！部屋が揺れた！", "BOOM! Shake the room!"));
                project(player_ptr, 0, rad, player_ptr->y, player_ptr->x, dam, AttributeType::SOUND, PROJECT_KILL | PROJECT_ITEM);
            }
        }
        break;

    case 11:
        if (name) {
            return _("破滅の矢", "Doom Bolt");
        }
        if (desc) {
            return _("純粋な魔力のビームを放つ。", "Fires a beam of pure mana.");
        }

        {
            DICE_NUMBER dice = 11 + (plev - 5) / 4;
            DICE_SID sides = 8;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_beam(player_ptr, AttributeType::MANA, dir, damroll(dice, sides));
            }
        }
        break;

    case 12:
        if (name) {
            return _("ファイア・ボール", "Fire Ball");
        }
        if (desc) {
            return _("炎の球を放つ。", "Fires a ball of fire.");
        }

        {
            int dam = plev + 55;
            POSITION rad = 2;

            if (info) {
                return info_damage(0, 0, dam);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_ball(player_ptr, AttributeType::FIRE, dir, dam, rad);
            }
        }
        break;

    case 13:
        if (name) {
            return _("テレポート・アウェイ", "Teleport Other");
        }
        if (desc) {
            return _("モンスターをテレポートさせるビームを放つ。抵抗されると無効。", "Teleports all monsters on the line away unless resisted.");
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

                fire_beam(player_ptr, AttributeType::AWAY_ALL, dir, power);
            }
        }
        break;

    case 14:
        if (name) {
            return _("破壊の言葉", "Word of Destruction");
        }
        if (desc) {
            return _("周辺のアイテム、モンスター、地形を破壊する。", "Destroys everything in nearby area.");
        }

        {
            int base = 12;
            DICE_SID sides = 4;

            if (cast) {
                destroy_area(player_ptr, player_ptr->y, player_ptr->x, base + randint1(sides), false);
            }
        }
        break;

    case 15:
        if (name) {
            return _("ログルス発動", "Invoke Logrus");
        }
        if (desc) {
            return _("巨大なカオスの球を放つ。", "Fires a huge ball of chaos.");
        }

        {
            int dam = plev * 2 + 99;
            POSITION rad = plev / 5;

            if (info) {
                return info_damage(0, 0, dam);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_ball(player_ptr, AttributeType::CHAOS, dir, dam, rad);
            }
        }
        break;

    case 16:
        if (name) {
            return _("他者変容", "Polymorph Other");
        }
        if (desc) {
            return _("モンスター1体を変身させる。抵抗されると無効。", "Attempts to polymorph a monster.");
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

                poly_monster(player_ptr, dir, plev);
            }
        }
        break;

    case 17:
        if (name) {
            return _("連鎖稲妻", "Chain Lightning");
        }
        if (desc) {
            return _("全方向に対して電撃のビームを放つ。", "Fires lightning beams in all directions.");
        }

        {
            DICE_NUMBER dice = 5 + plev / 10;
            DICE_SID sides = 8;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                for (dir = 0; dir <= 9; dir++) {
                    fire_beam(player_ptr, AttributeType::ELEC, dir, damroll(dice, sides));
                }
            }
        }
        break;

    case 18:
        if (name) {
            return _("魔力封入", "Arcane Binding");
        }
        if (desc) {
            return _("杖/魔法棒の充填回数を増やすか、充填中のロッドの充填時間を減らす。", "Recharges staffs, wands or rods.");
        }

        {
            int power = 90;

            if (info) {
                return info_power(power);
            }
            if (cast) {
                if (!recharge(player_ptr, power)) {
                    return nullptr;
                }
            }
        }
        break;

    case 19:
        if (name) {
            return _("原子分解", "Disintegrate");
        }
        if (desc) {
            return _("巨大な分解の球を放つ。", "Fires a huge ball of disintegration.");
        }

        {
            int dam = plev + 70;
            POSITION rad = 3 + plev / 40;

            if (info) {
                return info_damage(0, 0, dam);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_ball(player_ptr, AttributeType::DISINTEGRATE, dir, dam, rad);
            }
        }
        break;

    case 20:
        if (name) {
            return _("現実変容", "Alter Reality");
        }
        if (desc) {
            return _("現在の階を再構成する。", "Recreates current dungeon level.");
        }

        {
            int base = 15;
            DICE_SID sides = 20;

            if (info) {
                return info_delay(base, sides);
            }

            if (cast) {
                reserve_alter_reality(player_ptr, randint0(sides) + base);
            }
        }
        break;

    case 21:
        if (name) {
            return _("マジック・ロケット", "Magic Rocket");
        }
        if (desc) {
            return _("ロケットを発射する。", "Fires a magic rocket.");
        }

        {
            int dam = 120 + plev * 2;
            POSITION rad = 2;

            if (info) {
                return info_damage(0, 0, dam);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                msg_print(_("ロケット発射！", "You launch a rocket!"));
                fire_rocket(player_ptr, AttributeType::ROCKET, dir, dam, rad);
            }
        }
        break;

    case 22:
        if (name) {
            return _("混沌の刃", "Chaos Branding");
        }
        if (desc) {
            return _("武器にカオスの属性をつける。", "Makes current weapon a Chaotic weapon.");
        }

        {
            if (cast) {
                brand_weapon(player_ptr, 2);
            }
        }
        break;

    case 23:
        if (name) {
            return _("悪魔召喚", "Summon Demon");
        }
        if (desc) {
            return _("悪魔を1体召喚する。", "Summons a demon.");
        }

        {
            if (cast) {
                cast_summon_demon(player_ptr, (plev * 3) / 2);
            }
        }
        break;

    case 24:
        if (name) {
            return _("重力光線", "Beam of Gravity");
        }
        if (desc) {
            return _("重力のビームを放つ。", "Fires a beam of gravity.");
        }

        {
            DICE_NUMBER dice = 9 + (plev - 5) / 4;
            DICE_SID sides = 8;

            if (info) {
                return info_damage(dice, sides, 0);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }
                fire_beam(player_ptr, AttributeType::GRAVITY, dir, damroll(dice, sides));
            }
        }
        break;

    case 25:
        if (name) {
            return _("流星群", "Meteor Swarm");
        }
        if (desc) {
            return _("自分の周辺に隕石を落とす。", "Causes meteorites to fall down on nearby random locations.");
        }

        {
            int dam = plev * 2;
            POSITION rad = 2;

            if (info) {
                return info_multi_damage(dam);
            }

            if (cast) {
                cast_meteor(player_ptr, dam, rad);
            }
        }
        break;

    case 26:
        if (name) {
            return _("焔の一撃", "Flame Strike");
        }
        if (desc) {
            return _("自分を中心とした超巨大な炎の球を発生させる。", "Generates a huge ball of fire centered on you.");
        }

        {
            int dam = 300 + 3 * plev;
            POSITION rad = 8;

            if (info) {
                return info_damage(0, 0, dam / 2);
            }

            if (cast) {
                fire_ball(player_ptr, AttributeType::FIRE, 0, dam, rad);
            }
        }
        break;

    case 27:
        if (name) {
            return _("混沌召来", "Call Chaos");
        }
        if (desc) {
            return _("ランダムな属性の球やビームを発生させる。", "Generates random kinds of balls or beams.");
        }

        {
            if (info) {
                return format("%s150 / 250", KWD_DAM);
            }

            if (cast) {
                call_chaos(player_ptr);
            }
        }
        break;

    case 28:
        if (name) {
            return _("自己変容", "Polymorph Self");
        }
        if (desc) {
            return _("自分を変身させようとする。", "Polymorphs yourself.");
        }

        {
            if (cast) {
                if (!get_check(_("変身します。よろしいですか？", "You will polymorph yourself. Are you sure? "))) {
                    return nullptr;
                }
                do_poly_self(player_ptr);
            }
        }
        break;

    case 29:
        if (name) {
            return _("魔力の嵐", "Mana Storm");
        }
        if (desc) {
            return _("非常に強力で巨大な純粋な魔力の球を放つ。", "Fires an extremely powerful huge ball of pure mana.");
        }

        {
            int dam = 300 + plev * 4;
            POSITION rad = 4;

            if (info) {
                return info_damage(0, 0, dam);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }
                fire_ball(player_ptr, AttributeType::MANA, dir, dam, rad);
            }
        }
        break;

    case 30:
        if (name) {
            return _("ログルスのブレス", "Breathe Logrus");
        }
        if (desc) {
            return _("非常に強力なカオスの球を放つ。", "Fires an extremely powerful ball of chaos.");
        }

        {
            int dam = player_ptr->chp;
            POSITION rad = 2;

            if (info) {
                return info_damage(0, 0, dam);
            }

            if (cast) {
                if (!get_aim_dir(player_ptr, &dir)) {
                    return nullptr;
                }

                fire_ball(player_ptr, AttributeType::CHAOS, dir, dam, rad);
            }
        }
        break;

    case 31:
        if (name) {
            return _("虚無召来", "Call the Void");
        }
        if (desc) {
            return _("自分の周囲に向かって、ロケット、純粋な魔力の球、放射性廃棄物の球を放つ。ただし、壁に隣接して使用すると広範囲を破壊する。",
                "Fires rockets, mana balls and nuclear waste balls in all directions if you are not adjacent to any walls. Otherwise *destroys* huge area.");
        }
        {
            if (info) {
                return format("%s3 * 175", KWD_DAM);
            }

            if (cast) {
                call_the_void(player_ptr);
            }
        }
        break;
    }

    return "";
}
