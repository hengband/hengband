/*!
 * @brief 呪術の処理実装 / Hex code
 * @date 2014/01/14
 * @author
 * 2014 Deskull rearranged comment for Doxygen.\n
 */

#include "realm/realm-hex.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-item/cmd-quaff.h"
#include "core/asking-player.h"
#include "effect/attribute-types.h"
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
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "object/object-flags.h"
#include "player/attack-defense-types.h"
#include "player/player-skill.h"
#include "player/player-status.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "spell/spells-execution.h"
#include "spell/spells-status.h"
#include "spell/technic-info-table.h"
#include "status/action-setter.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
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
 * @brief 呪術領域魔法の各処理を行う
 * @param spell 魔法ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO / SpellProcessType::CAST / SPELL_CONT / SpellProcessType::STOP)
 * @return SpellProcessType::NAME / SPELL_DESC / SpellProcessType::INFO 時には文字列を返す。SpellProcessType::CAST / SPELL_CONT / SpellProcessType::STOP 時は std::nullopt を返す。
 */
std::optional<std::string> do_hex_spell(PlayerType *player_ptr, spell_hex_type spell, SpellProcessType mode)
{
    auto name = mode == SpellProcessType::NAME;
    auto description = mode == SpellProcessType::DESCRIPTION;
    auto info = mode == SpellProcessType::INFO;
    auto cast = mode == SpellProcessType::CAST;
    auto continuation = mode == SpellProcessType::CONTNUATION;
    auto stop = mode == SpellProcessType::STOP;
    auto should_continue = true;
    int power;
    switch (spell) {
        /*** 1st book (0-7) ***/
    case HEX_BLESS:
        if (name) {
            return _("邪なる祝福", "Evily blessing");
        }
        if (description) {
            return _("祝福により攻撃精度と防御力が上がる。", "Attempts to increase +to_hit of a weapon and AC");
        }
        if (cast) {
            if (!player_ptr->blessed) {
                msg_print(_("高潔な気分になった！", "You feel righteous!"));
            }
        }
        if (stop) {
            if (!player_ptr->blessed) {
                msg_print(_("高潔な気分が消え失せた。", "The prayer has expired."));
            }
        }
        break;

    case HEX_CURE_LIGHT:
        if (name) {
            return _("軽傷の治癒", "Cure light wounds");
        }
        if (description) {
            return _("HPや傷を少し回復させる。", "Heals cuts and HP a little.");
        }
        if (info) {
            return info_heal(1, 10, 0);
        }
        if (cast) {
            msg_print(_("気分が良くなってくる。", "You feel a little better."));
        }
        if (cast || continuation) {
            (void)cure_light_wounds(player_ptr, 1, 10);
        }
        break;

    case HEX_DEMON_AURA:
        if (name) {
            return _("悪魔のオーラ", "Demonic aura");
        }
        if (description) {
            return _("炎のオーラを身にまとい、回復速度が速くなる。", "Gives fire aura and regeneration.");
        }
        if (cast) {
            msg_print(_("体が炎のオーラで覆われた。", "You are enveloped by a fiery aura!"));
        }
        if (stop) {
            msg_print(_("炎のオーラが消え去った。", "The fiery aura disappeared."));
        }
        break;

    case HEX_STINKING_MIST:
        if (name) {
            return _("悪臭霧", "Stinking mist");
        }
        if (description) {
            return _("視界内のモンスターに微弱量の毒のダメージを与える。", "Deals a little poison damage to all monsters in your sight.");
        }
        power = player_ptr->lev / 2 + 5;
        if (info) {
            return info_damage(1, power, 0);
        }
        if (cast || continuation) {
            project_all_los(player_ptr, AttributeType::POIS, randint1(power));
        }
        break;

    case HEX_XTRA_MIGHT:
        if (name) {
            return _("腕力強化", "Extra might");
        }
        if (description) {
            return _("術者の腕力を上昇させる。", "Attempts to increase your strength.");
        }
        if (cast) {
            msg_print(_("何だか力が湧いて来る。", "You feel stronger."));
        }
        break;

    case HEX_CURSE_WEAPON:
        if (name) {
            return _("武器呪縛", "Curse weapon");
        }

        if (description) {
            return _("装備している武器を呪う。", "Curses your weapon.");
        }

        if (cast) {
            const auto q = _("どれを呪いますか？", "Which weapon do you curse?");
            const auto s = _("武器を装備していない。", "You're not wielding a weapon.");
            short item;
            auto *o_ptr = choose_object(player_ptr, &item, q, s, (USE_EQUIP), FuncItemTester(&ItemEntity::is_melee_weapon));
            if (o_ptr == nullptr) {
                return "";
            }

            const auto item_name = describe_flavor(player_ptr, o_ptr, OD_NAME_ONLY);
            auto f = object_flags(o_ptr);

            if (!get_check(format(_("本当に %s を呪いますか？", "Do you curse %s, really？"), item_name.data()))) {
                return "";
            }

            if (!one_in_(3) && (o_ptr->is_fixed_or_random_artifact() || f.has(TR_BLESSED))) {
                msg_format(_("%s は呪いを跳ね返した。", "%s resists the effect."), item_name.data());
                if (one_in_(3)) {
                    if (o_ptr->to_d > 0) {
                        o_ptr->to_d -= randint1(3) % 2;
                        if (o_ptr->to_d < 0) {
                            o_ptr->to_d = 0;
                        }
                    }
                    if (o_ptr->to_h > 0) {
                        o_ptr->to_h -= randint1(3) % 2;
                        if (o_ptr->to_h < 0) {
                            o_ptr->to_h = 0;
                        }
                    }
                    if (o_ptr->to_a > 0) {
                        o_ptr->to_a -= randint1(3) % 2;
                        if (o_ptr->to_a < 0) {
                            o_ptr->to_a = 0;
                        }
                    }
                    msg_format(_("%s は劣化してしまった。", "Your %s was disenchanted!"), item_name.data());
                }
            } else {
                int curse_rank = 0;
                msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), item_name.data());
                o_ptr->curse_flags.set(CurseTraitType::CURSED);

                if (o_ptr->is_fixed_or_random_artifact() || o_ptr->is_ego()) {

                    if (one_in_(3)) {
                        o_ptr->curse_flags.set(CurseTraitType::HEAVY_CURSE);
                    }
                    if (one_in_(666)) {
                        o_ptr->curse_flags.set(CurseTraitType::TY_CURSE);
                        if (one_in_(666)) {
                            o_ptr->curse_flags.set(CurseTraitType::PERMA_CURSE);
                        }

                        o_ptr->art_flags.set(TR_AGGRAVATE);
                        o_ptr->art_flags.set(TR_VORPAL);
                        o_ptr->art_flags.set(TR_VAMPIRIC);
                        msg_print(_("血だ！血だ！血だ！", "Blood, Blood, Blood!"));
                        curse_rank = 2;
                    }
                }

                o_ptr->curse_flags.set(get_curse(curse_rank, o_ptr));
            }

            RedrawingFlagsUpdater::get_instance().set_flag(StatusRedrawingFlag::BONUS);
            should_continue = false;
        }
        break;

    case HEX_DETECT_EVIL:
        if (name) {
            return _("邪悪感知", "Evil detection");
        }
        if (description) {
            return _("周囲の邪悪なモンスターを感知する。", "Detects evil monsters.");
        }
        if (info) {
            return info_range(MAX_PLAYER_SIGHT);
        }
        if (cast) {
            msg_print(_("邪悪な生物の存在を感じ取ろうとした。", "You sense the presence of evil creatures."));
        }
        break;

    case HEX_PATIENCE: {
        if (name) {
            return _("我慢", "Patience");
        }

        if (description) {
            return _("数ターン攻撃を耐えた後、受けたダメージを地獄の業火として周囲に放出する。", "Bursts hell fire strongly after enduring damage for a few turns.");
        }

        SpellHex spell_hex(player_ptr);
        power = std::min(200, spell_hex.get_revenge_power() * 2);
        if (info) {
            return info_damage(0, 0, power);
        }

        if (cast) {
            int a = 3 - (player_ptr->pspeed - 100) / 10;
            byte r = 3 + randint1(3) + std::max(0, std::min(3, a));

            if (spell_hex.get_revenge_turn() > 0) {
                msg_print(_("すでに我慢をしている。", "You are already biding your time for vengeance."));
                return std::nullopt;
            }

            spell_hex.set_revenge_type(SpellHexRevengeType::PATIENCE);
            spell_hex.set_revenge_turn(r, true);
            spell_hex.set_revenge_power(0, true);
            msg_print(_("じっと耐えることにした。", "You decide to endure damage for future retribution."));
            should_continue = false;
        }

        if (continuation) {
            POSITION rad = 2 + (power / 50);
            spell_hex.set_revenge_turn(1, false);
            if ((spell_hex.get_revenge_turn() == 0) || (power >= 200)) {
                msg_print(_("我慢が解かれた！", "My patience is at an end!"));
                if (power) {
                    project(player_ptr, 0, rad, player_ptr->y, player_ptr->x, power, AttributeType::HELL_FIRE, (PROJECT_STOP | PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL));
                }

                if (w_ptr->wizard) {
                    msg_format(_("%d点のダメージを返した。", "You return %d damage."), power);
                }

                spell_hex.set_revenge_type(SpellHexRevengeType::NONE);
                spell_hex.set_revenge_turn(0, true);
                spell_hex.set_revenge_power(0, true);
            }
        }
        break;
    }

        /*** 2nd book (8-15) ***/
    case HEX_ICE_ARMOR:
        if (name) {
            return _("氷の鎧", "Armor of ice");
        }
        if (description) {
            return _("氷のオーラを身にまとい、防御力が上昇する。", "Surrounds you with an icy aura and gives a bonus to AC.");
        }
        if (cast) {
            msg_print(_("体が氷の鎧で覆われた。", "You are enveloped by icy armor!"));
        }
        if (stop) {
            msg_print(_("氷の鎧が消え去った。", "The icy armor disappeared."));
        }
        break;

    case HEX_CURE_SERIOUS:
        if (name) {
            return _("重傷の治癒", "Cure serious wounds");
        }
        if (description) {
            return _("体力や傷を多少回復させる。", "Heals cuts and HP.");
        }
        if (info) {
            return info_heal(2, 10, 0);
        }
        if (cast) {
            msg_print(_("気分が良くなってくる。", "You feel better."));
        }
        if (cast || continuation) {
            (void)cure_serious_wounds(player_ptr, 2, 10);
        }
        break;

    case HEX_INHALE: {
        if (name) {
            return _("薬品吸入", "Inhale potion");
        }

        if (description) {
            return _("呪文詠唱を中止することなく、薬の効果を得ることができる。", "Quaffs a potion without canceling spell casting.");
        }

        SpellHex spell_hex(player_ptr);
        if (cast) {
            spell_hex.set_casting_flag(HEX_INHALE);
            do_cmd_quaff_potion(player_ptr);
            spell_hex.reset_casting_flag(HEX_INHALE);
            should_continue = false;
        }

        break;
    }
    case HEX_VAMP_MIST:
        if (name) {
            return _("衰弱の霧", "Hypodynamic mist");
        }
        if (description) {
            return _("視界内のモンスターに微弱量の衰弱属性のダメージを与える。", "Deals a little life-draining damage to all monsters in your sight.");
        }
        power = (player_ptr->lev / 2) + 5;
        if (info) {
            return info_damage(1, power, 0);
        }
        if (cast || continuation) {
            project_all_los(player_ptr, AttributeType::HYPODYNAMIA, randint1(power));
        }
        break;

    case HEX_RUNESWORD:
        if (name) {
            return _("魔剣化", "Swords to runeswords");
        }
        if (description) {
            return _("武器の攻撃力を上げる。切れ味を得、呪いに応じて与えるダメージが上昇し、善良なモンスターに対するダメージが2倍になる。",
                "Gives vorpal ability to your weapon. Increases damage from your weapon acccording to curse of your weapon.");
        }
        if (cast) {
#ifdef JP
            msg_print("あなたの武器が黒く輝いた。");
#else
            if (!empty_hands(player_ptr, false)) {
                msg_print("Your weapons glow bright black.");
            } else {
                msg_print("Your weapon glows bright black.");
            }
#endif
        }
        if (stop) {
#ifdef JP
            msg_print("武器の輝きが消え去った。");
#else
            msg_format("Your weapon%s.", (empty_hands(player_ptr, false)) ? " no longer glows" : "s no longer glow");
#endif
        }
        break;

    case HEX_CONFUSION:
        if (name) {
            return _("混乱の手", "Touch of confusion");
        }
        if (description) {
            return _("攻撃した際モンスターを混乱させる。", "Confuses a monster when you attack.");
        }
        if (cast) {
            msg_print(_("あなたの手が赤く輝き始めた。", "Your hands glow bright red."));
        }
        if (stop) {
            msg_print(_("手の輝きがなくなった。", "Your hands no longer glow."));
        }
        break;

    case HEX_BUILDING:
        if (name) {
            return _("肉体強化", "Building up");
        }
        if (description) {
            return _(
                "術者の腕力、器用さ、耐久力を上昇させる。攻撃回数の上限を 1 増加させる。", "Attempts to increases your strength, dexterity and constitusion.");
        }
        if (cast) {
            msg_print(_("身体が強くなった気がした。", "You feel your body is more developed now."));
        }
        break;

    case HEX_ANTI_TELE:
        if (name) {
            return _("反テレポート結界", "Anti teleport barrier");
        }
        if (description) {
            return _("視界内のモンスターのテレポートを阻害するバリアを張る。", "Obstructs all teleportations by monsters in your sight.");
        }
        power = player_ptr->lev * 3 / 2;
        if (info) {
            return info_power(power);
        }
        if (cast) {
            msg_print(_("テレポートを防ぐ呪いをかけた。", "You feel anyone can not teleport except you."));
        }
        break;

        /*** 3rd book (16-23) ***/
    case HEX_SHOCK_CLOAK:
        if (name) {
            return _("衝撃のクローク", "Cloak of shock");
        }
        if (description) {
            return _("電気のオーラを身にまとい、動きが速くなる。", "Gives lightning aura and a bonus to speed.");
        }
        if (cast) {
            msg_print(_("体が稲妻のオーラで覆われた。", "You are enveloped by an electrical aura!"));
        }
        if (stop) {
            msg_print(_("稲妻のオーラが消え去った。", "The electrical aura disappeared."));
        }
        break;

    case HEX_CURE_CRITICAL:
        if (name) {
            return _("致命傷の治癒", "Cure critical wounds");
        }
        if (description) {
            return _("体力や傷を回復させる。", "Heals cuts and HP greatly.");
        }
        if (info) {
            return info_heal(4, 10, 0);
        }
        if (cast) {
            msg_print(_("気分が良くなってくる。", "You feel much better."));
        }
        if (cast || continuation) {
            (void)cure_critical_wounds(player_ptr, damroll(4, 10));
        }
        break;

    case HEX_RECHARGE:
        if (name) {
            return _("呪力封入", "Recharging");
        }
        if (description) {
            return _("魔法の道具に魔力を再充填する。", "Recharges a magic device.");
        }
        power = player_ptr->lev * 2;
        if (info) {
            return info_power(power);
        }
        if (cast) {
            if (!recharge(player_ptr, power)) {
                return std::nullopt;
            }
            should_continue = false;
        }
        break;

    case HEX_RAISE_DEAD:
        if (name) {
            return _("死者復活", "Animate Dead");
        }
        if (description) {
            return _("死体を蘇らせてペットにする。", "Raises corpses and skeletons from dead.");
        }
        if (cast) {
            msg_print(_("死者への呼びかけを始めた。", "You start to call the dead.!"));
        }
        if (cast || continuation) {
            animate_dead(player_ptr, 0, player_ptr->y, player_ptr->x);
        }
        break;

    case HEX_CURSE_ARMOUR:
        if (name) {
            return _("防具呪縛", "Curse armor");
        }
        if (description) {
            return _("装備している防具に呪いをかける。", "Curse a piece of armour that you are wielding.");
        }
        if (cast) {
            const auto q = _("どれを呪いますか？", "Which piece of armour do you curse?");
            const auto s = _("防具を装備していない。", "You're not wearing any armor.");
            OBJECT_IDX item;
            auto *o_ptr = choose_object(player_ptr, &item, q, s, (USE_EQUIP), FuncItemTester(&ItemEntity::is_protector));
            if (!o_ptr) {
                return "";
            }

            o_ptr = &player_ptr->inventory_list[item];
            const auto item_name = describe_flavor(player_ptr, o_ptr, OD_NAME_ONLY);
            auto f = object_flags(o_ptr);

            if (!get_check(format(_("本当に %s を呪いますか？", "Do you curse %s, really？"), item_name.data()))) {
                return "";
            }

            if (!one_in_(3) && (o_ptr->is_fixed_or_random_artifact() || f.has(TR_BLESSED))) {
                msg_format(_("%s は呪いを跳ね返した。", "%s resists the effect."), item_name.data());
                if (one_in_(3)) {
                    if (o_ptr->to_d > 0) {
                        o_ptr->to_d -= randint1(3) % 2;
                        if (o_ptr->to_d < 0) {
                            o_ptr->to_d = 0;
                        }
                    }
                    if (o_ptr->to_h > 0) {
                        o_ptr->to_h -= randint1(3) % 2;
                        if (o_ptr->to_h < 0) {
                            o_ptr->to_h = 0;
                        }
                    }
                    if (o_ptr->to_a > 0) {
                        o_ptr->to_a -= randint1(3) % 2;
                        if (o_ptr->to_a < 0) {
                            o_ptr->to_a = 0;
                        }
                    }
                    msg_format(_("%s は劣化してしまった。", "Your %s was disenchanted!"), item_name.data());
                }
            } else {
                int curse_rank = 0;
                msg_format(_("恐怖の暗黒オーラがあなたの%sを包み込んだ！", "A terrible black aura blasts your %s!"), item_name.data());
                o_ptr->curse_flags.set(CurseTraitType::CURSED);

                if (o_ptr->is_fixed_or_random_artifact() || o_ptr->is_ego()) {

                    if (one_in_(3)) {
                        o_ptr->curse_flags.set(CurseTraitType::HEAVY_CURSE);
                    }
                    if (one_in_(666)) {
                        o_ptr->curse_flags.set(CurseTraitType::TY_CURSE);
                        if (one_in_(666)) {
                            o_ptr->curse_flags.set(CurseTraitType::PERMA_CURSE);
                        }

                        o_ptr->art_flags.set(TR_AGGRAVATE);
                        o_ptr->art_flags.set(TR_RES_POIS);
                        o_ptr->art_flags.set(TR_RES_DARK);
                        o_ptr->art_flags.set(TR_RES_NETHER);
                        msg_print(_("血だ！血だ！血だ！", "Blood, Blood, Blood!"));
                        curse_rank = 2;
                    }
                }

                o_ptr->curse_flags.set(get_curse(curse_rank, o_ptr));
            }

            RedrawingFlagsUpdater::get_instance().set_flag(StatusRedrawingFlag::BONUS);
            should_continue = false;
        }
        break;

    case HEX_SHADOW_CLOAK:
        if (name) {
            return _("影のクローク", "Cloak of shadow");
        }
        if (description) {
            return _("影のオーラを身にまとい、敵に影のダメージを与える。", "Gives aura of shadow.");
        }
        if (cast) {
            auto *o_ptr = &player_ptr->inventory_list[INVEN_OUTER];

            if (!o_ptr->is_valid()) {
                msg_print(_("クロークを身につけていない！", "You are not wearing a cloak."));
                return std::nullopt;
            } else if (!o_ptr->is_cursed()) {
                msg_print(_("クロークは呪われていない！", "Your cloak is not cursed."));
                return std::nullopt;
            } else {
                msg_print(_("影のオーラを身にまとった。", "You are enveloped by a shadowy aura!"));
            }
        }
        if (continuation) {
            auto *o_ptr = &player_ptr->inventory_list[INVEN_OUTER];

            if ((!o_ptr->is_valid()) || (!o_ptr->is_cursed())) {
                exe_spell(player_ptr, REALM_HEX, spell, SpellProcessType::STOP);
                SpellHex spell_hex(player_ptr);
                spell_hex.reset_casting_flag(spell);
                if (!spell_hex.is_spelling_any()) {
                    set_action(player_ptr, ACTION_NONE);
                }
            }
        }
        if (stop) {
            msg_print(_("影のオーラが消え去った。", "The shadowy aura disappeared."));
        }
        break;

    case HEX_PAIN_TO_MANA:
        if (name) {
            return _("苦痛を魔力に", "Pain to mana");
        }
        if (description) {
            return _("視界内のモンスターに精神ダメージ与え、魔力を吸い取る。", "Deals psychic damage to all monsters in sight and drains some mana.");
        }
        power = player_ptr->lev * 3 / 2;
        if (info) {
            return info_damage(1, power, 0);
        }
        if (cast || continuation) {
            project_all_los(player_ptr, AttributeType::PSI_DRAIN, randint1(power));
        }
        break;

    case HEX_EYE_FOR_EYE:
        if (name) {
            return _("目には目を", "Eye for an eye");
        }
        if (description) {
            return _("打撃や魔法で受けたダメージを、攻撃元のモンスターにも与える。", "Returns same damage which you got to the monster which damaged you.");
        }
        if (cast) {
            msg_print(_("復讐したい欲望にかられた。", "You feel very vengeful."));
        }
        break;

        /*** 4th book (24-31) ***/
    case HEX_ANTI_MULTI:
        if (name) {
            return _("反増殖結界", "Anti multiply barrier");
        }
        if (description) {
            return _("その階の増殖するモンスターの増殖を阻止する。", "Obstructs all multiplying by monsters on entire floor.");
        }
        if (cast) {
            msg_print(_("増殖を阻止する呪いをかけた。", "You feel anyone can not multiply."));
        }
        break;

    case HEX_RESTORE:
        if (name) {
            return _("全復活", "Restoration");
        }
        if (description) {
            return _("経験値を徐々に復活し、減少した能力値を回復させる。", "Restores experience and status.");
        }
        if (cast) {
            msg_print(_("体が元の活力を取り戻し始めた。", "You feel your lost status starting to return."));
        }
        if (cast || continuation) {
            bool flag = false;
            int d = (player_ptr->max_exp - player_ptr->exp);
            int r = (player_ptr->exp / 20);
            int i;

            if (d > 0) {
                if (d < r) {
                    player_ptr->exp = player_ptr->max_exp;
                } else {
                    player_ptr->exp += r;
                }

                /* Check the experience */
                check_experience(player_ptr);

                flag = true;
            }

            auto &rfu = RedrawingFlagsUpdater::get_instance();
            for (i = A_STR; i < A_MAX; i++) {
                if (player_ptr->stat_cur[i] < player_ptr->stat_max[i]) {
                    if (player_ptr->stat_cur[i] < 18) {
                        player_ptr->stat_cur[i]++;
                    } else {
                        player_ptr->stat_cur[i] += 10;
                    }

                    if (player_ptr->stat_cur[i] > player_ptr->stat_max[i]) {
                        player_ptr->stat_cur[i] = player_ptr->stat_max[i];
                    }

                    rfu.set_flag(StatusRedrawingFlag::BONUS);
                    flag = true;
                }
            }

            if (!flag) {
                const auto spell_name = exe_spell(player_ptr, REALM_HEX, HEX_RESTORE, SpellProcessType::NAME);
                msg_format(_("%sの呪文の詠唱をやめた。", "Finish casting '%s^'."), spell_name->data());
                SpellHex spell_hex(player_ptr);
                spell_hex.reset_casting_flag(HEX_RESTORE);
                if (!spell_hex.is_spelling_any()) {
                    set_action(player_ptr, ACTION_NONE);
                }

                const auto flags = {
                    StatusRedrawingFlag::BONUS,
                    StatusRedrawingFlag::HP,
                    StatusRedrawingFlag::MP,
                    StatusRedrawingFlag::SPELLS,
                };
                rfu.set_flags(flags);
                rfu.set_flag(MainWindowRedrawingFlag::EXTRA);
                return "";
            }
        }
        break;

    case HEX_DRAIN_CURSE:
        if (name) {
            return _("呪力吸収", "Drain curse power");
        }
        if (description) {
            return _("呪われた装備品の呪いを吸収して魔力を回復する。", "Drains curse on your equipment and heals SP a little.");
        }
        if (cast) {
            OBJECT_IDX item;
            concptr s, q;
            ItemEntity *o_ptr;

            q = _("どの装備品から吸収しますか？", "Which cursed equipment do you drain mana from?");
            s = _("呪われたアイテムを装備していない。", "You have no cursed equipment.");

            o_ptr = choose_object(player_ptr, &item, q, s, (USE_EQUIP), FuncItemTester(&ItemEntity::is_cursed));
            if (!o_ptr) {
                return "";
            }

            auto f = object_flags(o_ptr);

            player_ptr->csp += (player_ptr->lev / 5) + randint1(player_ptr->lev / 5);
            if (f.has(TR_TY_CURSE) || o_ptr->curse_flags.has(CurseTraitType::TY_CURSE)) {
                player_ptr->csp += randint1(5);
            }
            if (player_ptr->csp > player_ptr->msp) {
                player_ptr->csp = player_ptr->msp;
            }

            if (o_ptr->curse_flags.has(CurseTraitType::PERMA_CURSE)) {
                /* Nothing */
            } else if (o_ptr->curse_flags.has(CurseTraitType::HEAVY_CURSE)) {
                if (one_in_(7)) {
                    msg_print(_("呪いを全て吸い取った。", "A heavy curse vanished."));
                    o_ptr->curse_flags.clear();
                }
            } else if (o_ptr->curse_flags.has(CurseTraitType::CURSED) && one_in_(3)) {
                msg_print(_("呪いを全て吸い取った。", "A curse vanished."));
                o_ptr->curse_flags.clear();
            }

            should_continue = false;
        }
        break;

    case HEX_VAMP_BLADE:
        if (name) {
            return _("吸血の刃", "Swords to vampires");
        }
        if (description) {
            return _("吸血属性で攻撃する。", "Gives vampiric ability to your weapon.");
        }
        if (cast) {
#ifdef JP
            msg_print("あなたの武器が血を欲している。");
#else
            if (!empty_hands(player_ptr, false)) {
                msg_print("Your weapons want more blood now.");
            } else {
                msg_print("Your weapon wants more blood now.");
            }
#endif
        }
        if (stop) {
#ifdef JP
            msg_print("武器の渇望が消え去った。");
#else
            msg_format("Your weapon%s less thirsty now.", (empty_hands(player_ptr, false)) ? " is" : "s are");
#endif
        }
        break;

    case HEX_STUN_MONSTERS:
        if (name) {
            return _("朦朧の言葉", "Word of stun");
        }
        if (description) {
            return _("視界内のモンスターを朦朧とさせる。", "Stuns all monsters in your sight.");
        }
        power = player_ptr->lev * 4;
        if (info) {
            return info_power(power);
        }
        if (cast || continuation) {
            stun_monsters(player_ptr, power);
        }
        break;

    case HEX_SHADOW_MOVE:
        if (name) {
            return _("影移動", "Moving into shadow");
        }
        if (description) {
            return _("モンスターの隣のマスに瞬間移動する。", "Teleports you close to a monster.");
        }
        if (cast) {
            int i, dir;
            POSITION y, x;
            bool flag;

            for (i = 0; i < 3; i++) {
                if (!tgt_pt(player_ptr, &x, &y)) {
                    return "";
                }

                flag = false;

                const auto *floor_ptr = player_ptr->current_floor_ptr;
                for (dir = 0; dir < 8; dir++) {
                    int dy = y + ddy_ddd[dir];
                    int dx = x + ddx_ddd[dir];
                    if (dir == 5) {
                        continue;
                    }
                    if (floor_ptr->grid_array[dy][dx].m_idx) {
                        flag = true;
                    }
                }

                const auto dist = distance(y, x, player_ptr->y, player_ptr->x);
                if (!is_cave_empty_bold(player_ptr, y, x) || floor_ptr->grid_array[y][x].is_icky() || (dist > player_ptr->lev + 2)) {
                    msg_print(_("そこには移動できない。", "Can not teleport to there."));
                    continue;
                }
                break;
            }

            if (flag && randint0(player_ptr->lev * player_ptr->lev / 2)) {
                teleport_player_to(player_ptr, y, x, TELEPORT_SPONTANEOUS);
            } else {
                msg_print(_("おっと！", "Oops!"));
                teleport_player(player_ptr, 30, TELEPORT_SPONTANEOUS);
            }

            should_continue = false;
        }
        break;

    case HEX_ANTI_MAGIC:
        if (name) {
            return _("反魔法結界", "Anti magic barrier");
        }
        if (description) {
            return _("視界内のモンスターの魔法を阻害するバリアを張る。", "Obstructs all magic spells of monsters in your sight.");
        }
        power = player_ptr->lev * 3 / 2;
        if (info) {
            return info_power(power);
        }
        if (cast) {
            msg_print(_("魔法を防ぐ呪いをかけた。", "You feel anyone can not cast spells except you."));
        }
        break;

    case HEX_REVENGE: {
        if (name) {
            return _("復讐の宣告", "Revenge sentence");
        }

        if (description) {
            return _("数ターン後にそれまで受けたダメージに応じた威力の地獄の劫火の弾を放つ。", "Fires a ball of hell fire to try avenging damage from a few turns.");
        }

        SpellHex spell_hex(player_ptr);
        power = spell_hex.get_revenge_power();
        if (info) {
            return info_damage(0, 0, power);
        }

        if (cast) {
            byte r;
            int a = 3 - (player_ptr->pspeed - 100) / 10;
            r = 1 + randint1(2) + std::max(0, std::min(3, a));

            if (spell_hex.get_revenge_turn() > 0) {
                msg_print(_("すでに復讐は宣告済みだ。", "You've already declared your revenge."));
                return std::nullopt;
            }

            spell_hex.set_revenge_type(SpellHexRevengeType::REVENGE);
            spell_hex.set_revenge_turn(r, true);
            msg_format(_("あなたは復讐を宣告した。あと %d ターン。", "You declare your revenge. %d turns left."), r);
            should_continue = false;
        }

        if (continuation) {
            spell_hex.set_revenge_turn(1, false);
            if (spell_hex.get_revenge_turn() == 0) {
                DIRECTION dir;

                if (power) {
                    command_dir = 0;

                    do {
                        msg_print(_("復讐の時だ！", "Time for revenge!"));
                    } while (!get_aim_dir(player_ptr, &dir));

                    fire_ball(player_ptr, AttributeType::HELL_FIRE, dir, power, 1);

                    if (w_ptr->wizard) {
                        msg_format(_("%d点のダメージを返した。", "You return %d damage."), power);
                    }
                } else {
                    msg_print(_("復讐する気が失せた。", "You are not in the mood for revenge."));
                }

                spell_hex.set_revenge_power(0, true);
            }
        }

        break;
    }
    case HEX_MAX:
        break;
    }

    if (cast && should_continue) {
        SpellHex spell_hex(player_ptr);
        spell_hex.set_casting_flag(spell);
        if (player_ptr->action != ACTION_SPELL) {
            set_action(player_ptr, ACTION_SPELL);
        }
    }

    if (!info) {
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        const auto flags_srf = {
            StatusRedrawingFlag::BONUS,
            StatusRedrawingFlag::HP,
            StatusRedrawingFlag::MP,
            StatusRedrawingFlag::SPELLS,
        };
        rfu.set_flags(flags_srf);
        const auto flags_mwrf = {
            MainWindowRedrawingFlag::EXTRA,
            MainWindowRedrawingFlag::HP,
            MainWindowRedrawingFlag::MP,
        };
        rfu.set_flags(flags_mwrf);
    }

    return "";
}
