/*!
 * @brief 呪術の処理実装 / Hex code
 * @date 2014/01/14
 * @author
 * 2014 Deskull rearranged comment for Doxygen.\n
 * @details
 * magic_num1\n
 * 0: Flag bits of spelling spells\n
 * 1: Flag bits of despelled spells\n
 * 2: Revange damage\n
 * magic_num2\n
 * 0: Number of spelling spells\n
 * 1: Type of revenge\n
 * 2: Turn count for revenge\n
 */

#include "realm/realm-hex.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-item/cmd-quaff.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-requester.h"
#include "monster-race/monster-race.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "player/attack-defense-types.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "realm/realm-hex-numbers.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spell-types.h"
#include "spell/spells-execution.h"
#include "spell/spells-status.h"
#include "spell/technic-info-table.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/grid-selector.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

#ifdef JP
#else
#include "player-info/equipment-info.h"
#endif

/*!
 * @brief 呪術領域の武器呪縛の対象にできる武器かどうかを返す。 / An "item_tester_hook" for offer
 * @param o_ptr オブジェクト構造体の参照ポインタ
 * @return 呪縛可能な武器ならばTRUEを返す
 */
static bool item_tester_hook_weapon_except_bow(player_type *player_ptr, const object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    switch (o_ptr->tval) {
    case TV_SWORD:
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_DIGGING: {
        return true;
    }

    default:
        break;
    }

    return false;
}

/*!
 * @brief 呪術領域魔法の各処理を行う
 * @param spell 魔法ID
 * @param mode 処理内容 (SPELL_NAME / SPELL_DESC / SPELL_INFO / SPELL_CAST / SPELL_CONT / SPELL_STOP)
 * @return SPELL_NAME / SPELL_DESC / SPELL_INFO 時には文字列ポインタを返す。SPELL_CAST / SPELL_CONT / SPELL_STOP 時はNULL文字列を返す。
 */
concptr do_hex_spell(player_type *caster_ptr, SPELL_IDX spell, spell_type mode)
{
    bool name = mode == SPELL_NAME;
    bool desc = mode == SPELL_DESC;
    bool info = mode == SPELL_INFO;
    bool cast = mode == SPELL_CAST;
    bool cont = mode == SPELL_CONT;
    bool stop = mode == SPELL_STOP;

    bool add = true;

    PLAYER_LEVEL plev = caster_ptr->lev;
    HIT_POINT power;

    switch (spell) {
        /*** 1st book (0-7) ***/
    case 0:
        if (name)
            return _("邪なる祝福", "Evily blessing");
        if (desc)
            return _("祝福により攻撃精度と防御力が上がる。", "Attempts to increase +to_hit of a weapon and AC");
        if (cast) {
            if (!caster_ptr->blessed) {
                msg_print(_("高潔な気分になった！", "You feel righteous!"));
            }
        }
        if (stop) {
            if (!caster_ptr->blessed) {
                msg_print(_("高潔な気分が消え失せた。", "The prayer has expired."));
            }
        }
        break;

    case 1:
        if (name)
            return _("軽傷の治癒", "Cure light wounds");
        if (desc)
            return _("HPや傷を少し回復させる。", "Heals cuts and HP a little.");
        if (info)
            return info_heal(1, 10, 0);
        if (cast) {
            msg_print(_("気分が良くなってくる。", "You feel a little better."));
        }
        if (cast || cont)
            (void)cure_light_wounds(caster_ptr, 1, 10);
        break;

    case 2:
        if (name)
            return _("悪魔のオーラ", "Demonic aura");
        if (desc)
            return _("炎のオーラを身にまとい、回復速度が速くなる。", "Gives fire aura and regeneration.");
        if (cast) {
            msg_print(_("体が炎のオーラで覆われた。", "You are enveloped by a fiery aura!"));
        }
        if (stop) {
            msg_print(_("炎のオーラが消え去った。", "The fiery aura disappeared."));
        }
        break;

    case 3:
        if (name)
            return _("悪臭霧", "Stinking mist");
        if (desc)
            return _("視界内のモンスターに微弱量の毒のダメージを与える。", "Deals a little poison damage to all monsters in your sight.");
        power = plev / 2 + 5;
        if (info)
            return info_damage(1, power, 0);
        if (cast || cont) {
            project_all_los(caster_ptr, GF_POIS, randint1(power));
        }
        break;

    case 4:
        if (name)
            return _("腕力強化", "Extra might");
        if (desc)
            return _("術者の腕力を上昇させる。", "Attempts to increase your strength.");
        if (cast) {
            msg_print(_("何だか力が湧いて来る。", "You feel stronger."));
        }
        break;

    case 5:
        if (name)
            return _("武器呪縛", "Curse weapon");
        if (desc)
            return _("装備している武器を呪う。", "Curses your weapon.");
        if (cast) {
            OBJECT_IDX item;
            concptr q, s;
            GAME_TEXT o_name[MAX_NLEN];
            object_type *o_ptr;
            TrFlags f;

            q = _("どれを呪いますか？", "Which weapon do you curse?");
            s = _("武器を装備していない。", "You're not wielding a weapon.");

            o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP), FuncItemTester(item_tester_hook_weapon_except_bow, caster_ptr));
            if (!o_ptr)
                return "";

            describe_flavor(caster_ptr, o_name, o_ptr, OD_NAME_ONLY);
            object_flags(o_ptr, f);

            if (!get_check(format(_("本当に %s を呪いますか？", "Do you curse %s, really？"), o_name)))
                return "";

            if (!one_in_(3) && (object_is_artifact(o_ptr) || has_flag(f, TR_BLESSED))) {
                msg_format(_("%s は呪いを跳ね返した。", "%s resists the effect."), o_name);
                if (one_in_(3)) {
                    if (o_ptr->to_d > 0) {
                        o_ptr->to_d -= randint1(3) % 2;
                        if (o_ptr->to_d < 0)
                            o_ptr->to_d = 0;
                    }
                    if (o_ptr->to_h > 0) {
                        o_ptr->to_h -= randint1(3) % 2;
                        if (o_ptr->to_h < 0)
                            o_ptr->to_h = 0;
                    }
                    if (o_ptr->to_a > 0) {
                        o_ptr->to_a -= randint1(3) % 2;
                        if (o_ptr->to_a < 0)
                            o_ptr->to_a = 0;
                    }
                    msg_format(_("%s は劣化してしまった。", "Your %s was disenchanted!"), o_name);
                }
            } else {
                int curse_rank = 0;
                msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), o_name);
                o_ptr->curse_flags.set(TRC::CURSED);

                if (object_is_artifact(o_ptr) || object_is_ego(o_ptr)) {

                    if (one_in_(3))
                        o_ptr->curse_flags.set(TRC::HEAVY_CURSE);
                    if (one_in_(666)) {
                        o_ptr->curse_flags.set(TRC::TY_CURSE);
                        if (one_in_(666))
                            o_ptr->curse_flags.set(TRC::PERMA_CURSE);

                        add_flag(o_ptr->art_flags, TR_AGGRAVATE);
                        add_flag(o_ptr->art_flags, TR_VORPAL);
                        add_flag(o_ptr->art_flags, TR_VAMPIRIC);
                        msg_print(_("血だ！血だ！血だ！", "Blood, Blood, Blood!"));
                        curse_rank = 2;
                    }
                }

                o_ptr->curse_flags.set(get_curse(curse_rank, o_ptr));
            }

            caster_ptr->update |= (PU_BONUS);
            add = false;
        }
        break;

    case 6:
        if (name)
            return _("邪悪感知", "Evil detection");
        if (desc)
            return _("周囲の邪悪なモンスターを感知する。", "Detects evil monsters.");
        if (info)
            return info_range(MAX_SIGHT);
        if (cast) {
            msg_print(_("邪悪な生物の存在を感じ取ろうとした。", "You sense the presence of evil creatures."));
        }
        break;

    case 7:
        if (name)
            return _("我慢", "Patience");
        if (desc)
            return _(
                "数ターン攻撃を耐えた後、受けたダメージを地獄の業火として周囲に放出する。", "Bursts hell fire strongly after enduring damage for a few turns.");
        power = MIN(200, (hex_revenge_power(caster_ptr) * 2));
        if (info)
            return info_damage(0, 0, power);
        if (cast) {
            int a = 3 - (caster_ptr->pspeed - 100) / 10;
            byte r = 3 + randint1(3) + MAX(0, MIN(3, a));

            if (hex_revenge_turn(caster_ptr) > 0) {
                msg_print(_("すでに我慢をしている。", "You are already biding your time for vengeance."));
                return NULL;
            }

            hex_revenge_type(caster_ptr) = 1;
            hex_revenge_turn(caster_ptr) = r;
            hex_revenge_power(caster_ptr) = 0;
            msg_print(_("じっと耐えることにした。", "You decide to endure damage for future retribution."));
            add = false;
        }
        if (cont) {
            POSITION rad = 2 + (power / 50);

            hex_revenge_turn(caster_ptr)--;

            if ((hex_revenge_turn(caster_ptr) <= 0) || (power >= 200)) {
                msg_print(_("我慢が解かれた！", "My patience is at an end!"));
                if (power) {
                    project(caster_ptr, 0, rad, caster_ptr->y, caster_ptr->x, power, GF_HELL_FIRE, (PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL));
                }

                if (current_world_ptr->wizard) {
                    msg_format(_("%d点のダメージを返した。", "You return %d damage."), power);
                }

                /* Reset */
                hex_revenge_type(caster_ptr) = 0;
                hex_revenge_turn(caster_ptr) = 0;
                hex_revenge_power(caster_ptr) = 0;
            }
        }
        break;

        /*** 2nd book (8-15) ***/
    case 8:
        if (name)
            return _("氷の鎧", "Armor of ice");
        if (desc)
            return _("氷のオーラを身にまとい、防御力が上昇する。", "Surrounds you with an icy aura and gives a bonus to AC.");
        if (cast) {
            msg_print(_("体が氷の鎧で覆われた。", "You are enveloped by icy armor!"));
        }
        if (stop) {
            msg_print(_("氷の鎧が消え去った。", "The icy armor disappeared."));
        }
        break;

    case 9:
        if (name)
            return _("重傷の治癒", "Cure serious wounds");
        if (desc)
            return _("体力や傷を多少回復させる。", "Heals cuts and HP.");
        if (info)
            return info_heal(2, 10, 0);
        if (cast) {
            msg_print(_("気分が良くなってくる。", "You feel better."));
        }
        if (cast || cont)
            (void)cure_serious_wounds(caster_ptr, 2, 10);
        break;

    case 10:
        if (name)
            return _("薬品吸入", "Inhale potion");
        if (desc)
            return _("呪文詠唱を中止することなく、薬の効果を得ることができる。", "Quaffs a potion without canceling spell casting.");
        if (cast) {
            casting_hex_flags(caster_ptr) |= (1UL << HEX_INHAIL);
            do_cmd_quaff_potion(caster_ptr);
            casting_hex_flags(caster_ptr) &= ~(1UL << HEX_INHAIL);
            add = false;
        }
        break;

    case 11:
        if (name)
            return _("衰弱の霧", "Hypodynamic mist");
        if (desc)
            return _("視界内のモンスターに微弱量の衰弱属性のダメージを与える。", "Deals a little life-draining damage to all monsters in your sight.");
        power = (plev / 2) + 5;
        if (info)
            return info_damage(1, power, 0);
        if (cast || cont) {
            project_all_los(caster_ptr, GF_HYPODYNAMIA, randint1(power));
        }
        break;

    case 12:
        if (name)
            return _("魔剣化", "Swords to runeswords");
        if (desc)
            return _("武器の攻撃力を上げる。切れ味を得、呪いに応じて与えるダメージが上昇し、善良なモンスターに対するダメージが2倍になる。",
                "Gives vorpal ability to your weapon. Increases damage from your weapon acccording to curse of your weapon.");
        if (cast) {
#ifdef JP
            msg_print("あなたの武器が黒く輝いた。");
#else
            if (!empty_hands(caster_ptr, false))
                msg_print("Your weapons glow bright black.");
            else
                msg_print("Your weapon glows bright black.");
#endif
        }
        if (stop) {
#ifdef JP
            msg_print("武器の輝きが消え去った。");
#else
            msg_format("Your weapon%s.", (empty_hands(caster_ptr, false)) ? " no longer glows" : "s no longer glow");
#endif
        }
        break;

    case 13:
        if (name)
            return _("混乱の手", "Touch of confusion");
        if (desc)
            return _("攻撃した際モンスターを混乱させる。", "Confuses a monster when you attack.");
        if (cast) {
            msg_print(_("あなたの手が赤く輝き始めた。", "Your hands glow bright red."));
        }
        if (stop) {
            msg_print(_("手の輝きがなくなった。", "Your hands no longer glow."));
        }
        break;

    case 14:
        if (name)
            return _("肉体強化", "Building up");
        if (desc)
            return _(
                "術者の腕力、器用さ、耐久力を上昇させる。攻撃回数の上限を 1 増加させる。", "Attempts to increases your strength, dexterity and constitusion.");
        if (cast) {
            msg_print(_("身体が強くなった気がした。", "You feel your body is more developed now."));
        }
        break;

    case 15:
        if (name)
            return _("反テレポート結界", "Anti teleport barrier");
        if (desc)
            return _("視界内のモンスターのテレポートを阻害するバリアを張る。", "Obstructs all teleportations by monsters in your sight.");
        power = plev * 3 / 2;
        if (info)
            return info_power(power);
        if (cast) {
            msg_print(_("テレポートを防ぐ呪いをかけた。", "You feel anyone can not teleport except you."));
        }
        break;

        /*** 3rd book (16-23) ***/
    case 16:
        if (name)
            return _("衝撃のクローク", "Cloak of shock");
        if (desc)
            return _("電気のオーラを身にまとい、動きが速くなる。", "Gives lightning aura and a bonus to speed.");
        if (cast) {
            msg_print(_("体が稲妻のオーラで覆われた。", "You are enveloped by an electrical aura!"));
        }
        if (stop) {
            msg_print(_("稲妻のオーラが消え去った。", "The electrical aura disappeared."));
        }
        break;

    case 17:
        if (name)
            return _("致命傷の治癒", "Cure critical wounds");
        if (desc)
            return _("体力や傷を回復させる。", "Heals cuts and HP greatly.");
        if (info)
            return info_heal(4, 10, 0);
        if (cast) {
            msg_print(_("気分が良くなってくる。", "You feel much better."));
        }
        if (cast || cont)
            (void)cure_critical_wounds(caster_ptr, damroll(4, 10));
        break;

    case 18:
        if (name)
            return _("呪力封入", "Recharging");
        if (desc)
            return _("魔法の道具に魔力を再充填する。", "Recharges a magic device.");
        power = plev * 2;
        if (info)
            return info_power(power);
        if (cast) {
            if (!recharge(caster_ptr, power))
                return NULL;
            add = false;
        }
        break;

    case 19:
        if (name)
            return _("死者復活", "Animate Dead");
        if (desc)
            return _("死体を蘇らせてペットにする。", "Raises corpses and skeletons from dead.");
        if (cast) {
            msg_print(_("死者への呼びかけを始めた。", "You start to call the dead.!"));
        }
        if (cast || cont) {
            animate_dead(caster_ptr, 0, caster_ptr->y, caster_ptr->x);
        }
        break;

    case 20:
        if (name)
            return _("防具呪縛", "Curse armor");
        if (desc)
            return _("装備している防具に呪いをかける。", "Curse a piece of armour that you are wielding.");
        if (cast) {
            OBJECT_IDX item;
            concptr q, s;
            GAME_TEXT o_name[MAX_NLEN];
            object_type *o_ptr;
            TrFlags f;

            q = _("どれを呪いますか？", "Which piece of armour do you curse?");
            s = _("防具を装備していない。", "You're not wearing any armor.");

            o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP), FuncItemTester(object_is_armour));
            if (!o_ptr)
                return "";

            o_ptr = &caster_ptr->inventory_list[item];
            describe_flavor(caster_ptr, o_name, o_ptr, OD_NAME_ONLY);
            object_flags(o_ptr, f);

            if (!get_check(format(_("本当に %s を呪いますか？", "Do you curse %s, really？"), o_name)))
                return "";

            if (!one_in_(3) && (object_is_artifact(o_ptr) || has_flag(f, TR_BLESSED))) {
                msg_format(_("%s は呪いを跳ね返した。", "%s resists the effect."), o_name);
                if (one_in_(3)) {
                    if (o_ptr->to_d > 0) {
                        o_ptr->to_d -= randint1(3) % 2;
                        if (o_ptr->to_d < 0)
                            o_ptr->to_d = 0;
                    }
                    if (o_ptr->to_h > 0) {
                        o_ptr->to_h -= randint1(3) % 2;
                        if (o_ptr->to_h < 0)
                            o_ptr->to_h = 0;
                    }
                    if (o_ptr->to_a > 0) {
                        o_ptr->to_a -= randint1(3) % 2;
                        if (o_ptr->to_a < 0)
                            o_ptr->to_a = 0;
                    }
                    msg_format(_("%s は劣化してしまった。", "Your %s was disenchanted!"), o_name);
                }
            } else {
                int curse_rank = 0;
                msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), o_name);
                o_ptr->curse_flags.set(TRC::CURSED);

                if (object_is_artifact(o_ptr) || object_is_ego(o_ptr)) {

                    if (one_in_(3))
                        o_ptr->curse_flags.set(TRC::HEAVY_CURSE);
                    if (one_in_(666)) {
                        o_ptr->curse_flags.set(TRC::TY_CURSE);
                        if (one_in_(666))
                            o_ptr->curse_flags.set(TRC::PERMA_CURSE);

                        add_flag(o_ptr->art_flags, TR_AGGRAVATE);
                        add_flag(o_ptr->art_flags, TR_RES_POIS);
                        add_flag(o_ptr->art_flags, TR_RES_DARK);
                        add_flag(o_ptr->art_flags, TR_RES_NETHER);
                        msg_print(_("血だ！血だ！血だ！", "Blood, Blood, Blood!"));
                        curse_rank = 2;
                    }
                }

                o_ptr->curse_flags.set(get_curse(curse_rank, o_ptr));
            }

            caster_ptr->update |= (PU_BONUS);
            add = false;
        }
        break;

    case 21:
        if (name)
            return _("影のクローク", "Cloak of shadow");
        if (desc)
            return _("影のオーラを身にまとい、敵に影のダメージを与える。", "Gives aura of shadow.");
        if (cast) {
            object_type *o_ptr = &caster_ptr->inventory_list[INVEN_OUTER];

            if (!o_ptr->k_idx) {
                msg_print(_("クロークを身につけていない！", "You are not wearing a cloak."));
                return NULL;
            } else if (!object_is_cursed(o_ptr)) {
                msg_print(_("クロークは呪われていない！", "Your cloak is not cursed."));
                return NULL;
            } else {
                msg_print(_("影のオーラを身にまとった。", "You are enveloped by a shadowy aura!"));
            }
        }
        if (cont) {
            object_type *o_ptr = &caster_ptr->inventory_list[INVEN_OUTER];

            if ((!o_ptr->k_idx) || (!object_is_cursed(o_ptr))) {
                exe_spell(caster_ptr, REALM_HEX, spell, SPELL_STOP);
                casting_hex_flags(caster_ptr) &= ~(1UL << spell);
                casting_hex_num(caster_ptr)--;
                if (get_singing_song_id(caster_ptr) == 0)
                    set_action(caster_ptr, ACTION_NONE);
            }
        }
        if (stop) {
            msg_print(_("影のオーラが消え去った。", "The shadowy aura disappeared."));
        }
        break;

    case 22:
        if (name)
            return _("苦痛を魔力に", "Pain to mana");
        if (desc)
            return _("視界内のモンスターに精神ダメージ与え、魔力を吸い取る。", "Deals psychic damage to all monsters in sight and drains some mana.");
        power = plev * 3 / 2;
        if (info)
            return info_damage(1, power, 0);
        if (cast || cont) {
            project_all_los(caster_ptr, GF_PSI_DRAIN, randint1(power));
        }
        break;

    case 23:
        if (name)
            return _("目には目を", "Eye for an eye");
        if (desc)
            return _("打撃や魔法で受けたダメージを、攻撃元のモンスターにも与える。", "Returns same damage which you got to the monster which damaged you.");
        if (cast) {
            msg_print(_("復讐したい欲望にかられた。", "You feel very vengeful."));
        }
        break;

        /*** 4th book (24-31) ***/
    case 24:
        if (name)
            return _("反増殖結界", "Anti multiply barrier");
        if (desc)
            return _("その階の増殖するモンスターの増殖を阻止する。", "Obstructs all multiplying by monsters on entire floor.");
        if (cast) {
            msg_print(_("増殖を阻止する呪いをかけた。", "You feel anyone can not multiply."));
        }
        break;

    case 25:
        if (name)
            return _("全復活", "Restoration");
        if (desc)
            return _("経験値を徐々に復活し、減少した能力値を回復させる。", "Restores experience and status.");
        if (cast) {
            msg_print(_("体が元の活力を取り戻し始めた。", "You feel your lost status starting to return."));
        }
        if (cast || cont) {
            bool flag = false;
            int d = (caster_ptr->max_exp - caster_ptr->exp);
            int r = (caster_ptr->exp / 20);
            int i;

            if (d > 0) {
                if (d < r)
                    caster_ptr->exp = caster_ptr->max_exp;
                else
                    caster_ptr->exp += r;

                /* Check the experience */
                check_experience(caster_ptr);

                flag = true;
            }
            for (i = A_STR; i < A_MAX; i++) {
                if (caster_ptr->stat_cur[i] < caster_ptr->stat_max[i]) {
                    if (caster_ptr->stat_cur[i] < 18)
                        caster_ptr->stat_cur[i]++;
                    else
                        caster_ptr->stat_cur[i] += 10;

                    if (caster_ptr->stat_cur[i] > caster_ptr->stat_max[i])
                        caster_ptr->stat_cur[i] = caster_ptr->stat_max[i];
                    caster_ptr->update |= (PU_BONUS);

                    flag = true;
                }
            }

            if (!flag) {
                msg_format(_("%sの呪文の詠唱をやめた。", "Finish casting '%^s'."), exe_spell(caster_ptr, REALM_HEX, HEX_RESTORE, SPELL_NAME));
                casting_hex_flags(caster_ptr) &= ~(1UL << HEX_RESTORE);
                if (cont)
                    casting_hex_num(caster_ptr)--;
                if (casting_hex_num(caster_ptr))
                    caster_ptr->action = ACTION_NONE;

                caster_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
                caster_ptr->redraw |= (PR_EXTRA);

                return "";
            }
        }
        break;

    case 26:
        if (name)
            return _("呪力吸収", "Drain curse power");
        if (desc)
            return _("呪われた装備品の呪いを吸収して魔力を回復する。", "Drains curse on your equipment and heals SP a little.");
        if (cast) {
            OBJECT_IDX item;
            concptr s, q;
            TrFlags f;
            object_type *o_ptr;

            q = _("どの装備品から吸収しますか？", "Which cursed equipment do you drain mana from?");
            s = _("呪われたアイテムを装備していない。", "You have no cursed equipment.");

            o_ptr = choose_object(caster_ptr, &item, q, s, (USE_EQUIP), FuncItemTester(object_is_cursed));
            if (!o_ptr)
                return "";

            object_flags(o_ptr, f);

            caster_ptr->csp += (caster_ptr->lev / 5) + randint1(caster_ptr->lev / 5);
            if (has_flag(f, TR_TY_CURSE) || o_ptr->curse_flags.has(TRC::TY_CURSE))
                caster_ptr->csp += randint1(5);
            if (caster_ptr->csp > caster_ptr->msp)
                caster_ptr->csp = caster_ptr->msp;

            if (o_ptr->curse_flags.has(TRC::PERMA_CURSE)) {
                /* Nothing */
            } else if (o_ptr->curse_flags.has(TRC::HEAVY_CURSE)) {
                if (one_in_(7)) {
                    msg_print(_("呪いを全て吸い取った。", "A heavy curse vanished."));
                    o_ptr->curse_flags.clear();
                }
            } else if (o_ptr->curse_flags.has(TRC::CURSED) && one_in_(3)) {
                msg_print(_("呪いを全て吸い取った。", "A curse vanished."));
                o_ptr->curse_flags.clear();
            }

            add = false;
        }
        break;

    case 27:
        if (name)
            return _("吸血の刃", "Swords to vampires");
        if (desc)
            return _("吸血属性で攻撃する。", "Gives vampiric ability to your weapon.");
        if (cast) {
#ifdef JP
            msg_print("あなたの武器が血を欲している。");
#else
            if (!empty_hands(caster_ptr, false))
                msg_print("Your weapons want more blood now.");
            else
                msg_print("Your weapon wants more blood now.");
#endif
        }
        if (stop) {
#ifdef JP
            msg_print("武器の渇望が消え去った。");
#else
            msg_format("Your weapon%s less thirsty now.", (empty_hands(caster_ptr, false)) ? " is" : "s are");
#endif
        }
        break;

    case 28:
        if (name)
            return _("朦朧の言葉", "Word of stun");
        if (desc)
            return _("視界内のモンスターを朦朧とさせる。", "Stuns all monsters in your sight.");
        power = plev * 4;
        if (info)
            return info_power(power);
        if (cast || cont) {
            stun_monsters(caster_ptr, power);
        }
        break;

    case 29:
        if (name)
            return _("影移動", "Moving into shadow");
        if (desc)
            return _("モンスターの隣のマスに瞬間移動する。", "Teleports you close to a monster.");
        if (cast) {
            int i, dir;
            POSITION y, x;
            bool flag;

            for (i = 0; i < 3; i++) {
                if (!tgt_pt(caster_ptr, &x, &y))
                    return "";

                flag = false;

                for (dir = 0; dir < 8; dir++) {
                    int dy = y + ddy_ddd[dir];
                    int dx = x + ddx_ddd[dir];
                    if (dir == 5)
                        continue;
                    if (caster_ptr->current_floor_ptr->grid_array[dy][dx].m_idx)
                        flag = true;
                }

                if (!is_cave_empty_bold(caster_ptr, y, x) || caster_ptr->current_floor_ptr->grid_array[y][x].is_icky()
                    || (distance(y, x, caster_ptr->y, caster_ptr->x) > plev + 2)) {
                    msg_print(_("そこには移動できない。", "Can not teleport to there."));
                    continue;
                }
                break;
            }

            if (flag && randint0(plev * plev / 2)) {
                teleport_player_to(caster_ptr, y, x, TELEPORT_SPONTANEOUS);
            } else {
                msg_print(_("おっと！", "Oops!"));
                teleport_player(caster_ptr, 30, TELEPORT_SPONTANEOUS);
            }

            add = false;
        }
        break;

    case 30:
        if (name)
            return _("反魔法結界", "Anti magic barrier");
        if (desc)
            return _("視界内のモンスターの魔法を阻害するバリアを張る。", "Obstructs all magic spells of monsters in your sight.");
        power = plev * 3 / 2;
        if (info)
            return info_power(power);
        if (cast) {
            msg_print(_("魔法を防ぐ呪いをかけた。", "You feel anyone can not cast spells except you."));
        }
        break;

    case 31:
        if (name)
            return _("復讐の宣告", "Revenge sentence");
        if (desc)
            return _(
                "数ターン後にそれまで受けたダメージに応じた威力の地獄の劫火の弾を放つ。", "Fires a ball of hell fire to try avenging damage from a few turns.");
        power = hex_revenge_power(caster_ptr);
        if (info)
            return info_damage(0, 0, power);
        if (cast) {
            byte r;
            int a = 3 - (caster_ptr->pspeed - 100) / 10;
            r = 1 + randint1(2) + MAX(0, MIN(3, a));

            if (hex_revenge_turn(caster_ptr) > 0) {
                msg_print(_("すでに復讐は宣告済みだ。", "You've already declared your revenge."));
                return NULL;
            }

            hex_revenge_type(caster_ptr) = 2;
            hex_revenge_turn(caster_ptr) = r;
            msg_format(_("あなたは復讐を宣告した。あと %d ターン。", "You declare your revenge. %d turns left."), r);
            add = false;
        }
        if (cont) {
            hex_revenge_turn(caster_ptr)--;

            if (hex_revenge_turn(caster_ptr) <= 0) {
                DIRECTION dir;

                if (power) {
                    command_dir = 0;

                    do {
                        msg_print(_("復讐の時だ！", "Time for revenge!"));
                    } while (!get_aim_dir(caster_ptr, &dir));

                    fire_ball(caster_ptr, GF_HELL_FIRE, dir, power, 1);

                    if (current_world_ptr->wizard) {
                        msg_format(_("%d点のダメージを返した。", "You return %d damage."), power);
                    }
                } else {
                    msg_print(_("復讐する気が失せた。", "You are not in the mood for revenge."));
                }
                hex_revenge_power(caster_ptr) = 0;
            }
        }
        break;
    }

    /* start casting */
    if ((cast) && (add)) {
        /* add spell */
        casting_hex_flags(caster_ptr) |= 1UL << (spell);
        casting_hex_num(caster_ptr)++;

        if (caster_ptr->action != ACTION_SPELL)
            set_action(caster_ptr, ACTION_SPELL);
    }

    if (!info) {
        caster_ptr->update |= (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
        caster_ptr->redraw |= (PR_EXTRA | PR_HP | PR_MANA);
    }

    return "";
}
