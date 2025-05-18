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
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-requester.h"
#include "object-enchant/object-curse.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-armor.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player/attack-defense-types.h"
#include "player/player-realm.h"
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
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/grid-selector.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/dice.h"
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
    auto info = mode == SpellProcessType::INFO;
    auto cast = mode == SpellProcessType::CAST;
    auto continuation = mode == SpellProcessType::CONTNUATION;
    auto stop = mode == SpellProcessType::STOP;
    auto should_continue = true;
    int power;

    switch (spell) {
        /*** 1st book (0-7) ***/
    case HEX_BLESS:
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

    case HEX_CURE_LIGHT: {
        const Dice dice(1, 10);
        if (info) {
            return info_heal(dice, 0);
        }
        if (cast) {
            msg_print(_("気分が良くなってくる。", "You feel a little better."));
        }
        if (cast || continuation) {
            (void)cure_light_wounds(player_ptr, dice.roll());
        }
        break;
    }
    case HEX_DEMON_AURA: {
        if (cast) {
            msg_print(_("体が炎のオーラで覆われた。", "You are enveloped by a fiery aura!"));
        }
        if (stop) {
            msg_print(_("炎のオーラが消え去った。", "The fiery aura disappeared."));
        }
        break;
    }
    case HEX_STINKING_MIST: {
        const Dice dice(1, player_ptr->lev / 2 + 5);
        if (info) {
            return info_damage(dice);
        }
        if (cast || continuation) {
            project_all_los(player_ptr, AttributeType::POIS, dice.roll());
        }
        break;
    }
    case HEX_XTRA_MIGHT: {
        if (cast) {
            msg_print(_("何だか力が湧いて来る。", "You feel stronger."));
        }
        break;
    }
    case HEX_CURSE_WEAPON: {
        if (cast) {
            constexpr auto q = _("どれを呪いますか？", "Which weapon do you curse?");
            constexpr auto s = _("武器を装備していない。", "You're not wielding a weapon.");
            short i_idx;
            auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, (USE_EQUIP), FuncItemTester(&ItemEntity::is_melee_weapon));
            if (o_ptr == nullptr) {
                return "";
            }

            const auto item_name = describe_flavor(player_ptr, *o_ptr, OD_NAME_ONLY);
            if (!input_check(format(_("本当に %s を呪いますか？", "Do you curse %s, really?"), item_name.data()))) {
                return "";
            }

            if (!one_in_(3) && (o_ptr->is_fixed_or_random_artifact() || o_ptr->get_flags().has(TR_BLESSED))) {
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

            RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
            should_continue = false;
        }
        break;
    }
    case HEX_DETECT_EVIL: {
        if (info) {
            return info_range(MAX_PLAYER_SIGHT);
        }
        if (cast) {
            msg_print(_("邪悪な生物の存在を感じ取ろうとした。", "You sense the presence of evil creatures."));
        }
        break;
    }
    case HEX_PATIENCE: {
        SpellHex spell_hex(player_ptr);
        power = std::min(200, spell_hex.get_revenge_power() * 2);
        if (info) {
            return info_damage(power);
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

                if (AngbandWorld::get_instance().wizard) {
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
    case HEX_ICE_ARMOR: {
        if (cast) {
            msg_print(_("体が氷の鎧で覆われた。", "You are enveloped by icy armor!"));
        }
        if (stop) {
            msg_print(_("氷の鎧が消え去った。", "The icy armor disappeared."));
        }
        break;
    }
    case HEX_CURE_SERIOUS: {
        const Dice dice(2, 10);
        if (info) {
            return info_heal(dice);
        }
        if (cast) {
            msg_print(_("気分が良くなってくる。", "You feel better."));
        }
        if (cast || continuation) {
            (void)cure_serious_wounds(player_ptr, dice.roll());
        }
        break;
    }
    case HEX_INHALE: {
        SpellHex spell_hex(player_ptr);
        if (cast) {
            spell_hex.set_casting_flag(HEX_INHALE);
            do_cmd_quaff_potion(player_ptr);
            spell_hex.reset_casting_flag(HEX_INHALE);
            should_continue = false;
        }

        break;
    }
    case HEX_VAMP_MIST: {
        const Dice dice(1, player_ptr->lev / 2 + 5);
        if (info) {
            return info_damage(dice);
        }
        if (cast || continuation) {
            project_all_los(player_ptr, AttributeType::HYPODYNAMIA, dice.roll());
        }
        break;
    }
    case HEX_RUNESWORD: {
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
    }
    case HEX_CONFUSION: {
        if (cast) {
            msg_print(_("あなたの手が赤く輝き始めた。", "Your hands glow bright red."));
        }
        if (stop) {
            msg_print(_("手の輝きがなくなった。", "Your hands no longer glow."));
        }
        break;
    }
    case HEX_BUILDING: {
        if (cast) {
            msg_print(_("身体が強くなった気がした。", "You feel your body is more developed now."));
        }
        break;
    }
    case HEX_ANTI_TELE: {
        power = player_ptr->lev * 3 / 2;
        if (info) {
            return info_power(power);
        }
        if (cast) {
            msg_print(_("テレポートを防ぐ呪いをかけた。", "You feel anyone can not teleport except you."));
        }
        break;
    }
        /*** 3rd book (16-23) ***/
    case HEX_SHOCK_CLOAK: {
        if (cast) {
            msg_print(_("体が稲妻のオーラで覆われた。", "You are enveloped by an electrical aura!"));
        }
        if (stop) {
            msg_print(_("稲妻のオーラが消え去った。", "The electrical aura disappeared."));
        }
        break;
    }
    case HEX_CURE_CRITICAL: {
        const Dice dice(4, 10);
        if (info) {
            return info_heal(dice);
        }
        if (cast) {
            msg_print(_("気分が良くなってくる。", "You feel much better."));
        }
        if (cast || continuation) {
            (void)cure_critical_wounds(player_ptr, dice.roll());
        }
        break;
    }
    case HEX_RECHARGE: {
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
    }
    case HEX_RAISE_DEAD: {
        if (cast) {
            msg_print(_("死者への呼びかけを始めた。", "You start to call the dead.!"));
        }
        if (cast || continuation) {
            animate_dead(player_ptr, 0, player_ptr->y, player_ptr->x);
        }
        break;
    }
    case HEX_CURSE_ARMOUR: {
        if (cast) {
            constexpr auto q = _("どれを呪いますか？", "Which piece of armour do you curse?");
            constexpr auto s = _("防具を装備していない。", "You're not wearing any armor.");
            short i_idx;
            auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, (USE_EQUIP), FuncItemTester(&ItemEntity::is_protector));
            if (!o_ptr) {
                return "";
            }

            o_ptr = player_ptr->inventory[i_idx].get();
            const auto item_name = describe_flavor(player_ptr, *o_ptr, OD_NAME_ONLY);
            if (!input_check(format(_("本当に %s を呪いますか？", "Do you curse %s, really?"), item_name.data()))) {
                return "";
            }

            if (!one_in_(3) && (o_ptr->is_fixed_or_random_artifact() || o_ptr->get_flags().has(TR_BLESSED))) {
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

            RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
            should_continue = false;
        }
        break;
    }
    case HEX_SHADOW_CLOAK: {
        if (cast) {
            auto *o_ptr = player_ptr->inventory[INVEN_OUTER].get();

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
            auto *o_ptr = player_ptr->inventory[INVEN_OUTER].get();

            if ((!o_ptr->is_valid()) || (!o_ptr->is_cursed())) {
                exe_spell(player_ptr, RealmType::HEX, spell, SpellProcessType::STOP);
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
    }
    case HEX_PAIN_TO_MANA: {
        const Dice dice(1, player_ptr->lev * 3 / 2);
        if (info) {
            return info_damage(dice);
        }
        if (cast || continuation) {
            project_all_los(player_ptr, AttributeType::PSI_DRAIN, dice.roll());
        }
        break;
    }
    case HEX_EYE_FOR_EYE: {
        if (cast) {
            msg_print(_("復讐したい欲望にかられた。", "You feel very vengeful."));
        }
        break;
    }
        /*** 4th book (24-31) ***/
    case HEX_ANTI_MULTI: {
        if (cast) {
            msg_print(_("増殖を阻止する呪いをかけた。", "You feel anyone can not multiply."));
        }
        break;
    }
    case HEX_RESTORE: {
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

                    rfu.set_flag(StatusRecalculatingFlag::BONUS);
                    flag = true;
                }
            }

            if (!flag) {
                const auto &spell_name = PlayerRealm::get_spell_name(RealmType::HEX, HEX_RESTORE);
                msg_format(_("%sの呪文の詠唱をやめた。", "Finish casting '%s^'."), spell_name.data());
                SpellHex spell_hex(player_ptr);
                spell_hex.reset_casting_flag(HEX_RESTORE);
                if (!spell_hex.is_spelling_any()) {
                    set_action(player_ptr, ACTION_NONE);
                }

                static constexpr auto flags = {
                    StatusRecalculatingFlag::BONUS,
                    StatusRecalculatingFlag::HP,
                    StatusRecalculatingFlag::MP,
                    StatusRecalculatingFlag::SPELLS,
                };
                rfu.set_flags(flags);
                rfu.set_flag(MainWindowRedrawingFlag::EXTRA);
                return "";
            }
        }
        break;
    }
    case HEX_DRAIN_CURSE: {
        if (cast) {
            constexpr auto q = _("どの装備品から吸収しますか？", "Which cursed equipment do you drain mana from?");
            constexpr auto s = _("呪われたアイテムを装備していない。", "You have no cursed equipment.");
            short i_idx;
            auto *o_ptr = choose_object(player_ptr, &i_idx, q, s, (USE_EQUIP), FuncItemTester(&ItemEntity::is_cursed));
            if (!o_ptr) {
                return "";
            }

            player_ptr->csp += (player_ptr->lev / 5) + randint1(player_ptr->lev / 5);
            if (o_ptr->get_flags().has(TR_TY_CURSE) || o_ptr->curse_flags.has(CurseTraitType::TY_CURSE)) {
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
    }
    case HEX_VAMP_BLADE: {
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
    }
    case HEX_STUN_MONSTERS: {
        power = player_ptr->lev * 4;
        if (info) {
            return info_power(power);
        }
        if (cast || continuation) {
            stun_monsters(player_ptr, power);
        }
        break;
    }
    case HEX_SHADOW_MOVE:
        if (cast) {
            std::optional<Pos2D> pos_target;
            bool flag;
            for (auto i = 0; i < 3; i++) {
                pos_target = point_target(player_ptr);
                if (!pos_target) {
                    return "";
                }

                flag = false;
                const auto &floor = *player_ptr->current_floor_ptr;
                for (const auto &d : Direction::directions_8()) {
                    const auto pos_neighbor = *pos_target + d.vec();
                    if (floor.get_grid(pos_neighbor).has_monster()) {
                        flag = true;
                    }
                }

                const auto p_pos = player_ptr->get_position();
                const auto dist = Grid::calc_distance(*pos_target, p_pos);
                if (!floor.is_empty_at(*pos_target) || (*pos_target == p_pos) || floor.get_grid(*pos_target).is_icky() || (dist > player_ptr->lev + 2)) {
                    msg_print(_("そこには移動できない。", "Can not teleport to there."));
                    continue;
                }

                break;
            }

            if (flag && randint0(player_ptr->lev * player_ptr->lev / 2)) {
                teleport_player_to(player_ptr, pos_target->y, pos_target->x, TELEPORT_SPONTANEOUS);
            } else {
                msg_print(_("おっと！", "Oops!"));
                teleport_player(player_ptr, 30, TELEPORT_SPONTANEOUS);
            }

            should_continue = false;
        }

        break;
    case HEX_ANTI_MAGIC: {
        power = player_ptr->lev * 3 / 2;
        if (info) {
            return info_power(power);
        }
        if (cast) {
            msg_print(_("魔法を防ぐ呪いをかけた。", "You feel anyone can not cast spells except you."));
        }
        break;
    }
    case HEX_REVENGE: {
        SpellHex spell_hex(player_ptr);
        power = spell_hex.get_revenge_power();
        if (info) {
            return info_damage(power);
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
                if (power) {
                    command_dir = Direction::none();

                    auto dir = Direction::none();
                    do {
                        msg_print(_("復讐の時だ！", "Time for revenge!"));
                        dir = get_aim_dir(player_ptr);
                    } while (!dir);

                    fire_ball(player_ptr, AttributeType::HELL_FIRE, dir, power, 1);

                    if (AngbandWorld::get_instance().wizard) {
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
        static constexpr auto flags_srf = {
            StatusRecalculatingFlag::BONUS,
            StatusRecalculatingFlag::HP,
            StatusRecalculatingFlag::MP,
            StatusRecalculatingFlag::SPELLS,
        };
        rfu.set_flags(flags_srf);
        static constexpr auto flags_mwrf = {
            MainWindowRedrawingFlag::EXTRA,
            MainWindowRedrawingFlag::HP,
            MainWindowRedrawingFlag::MP,
        };
        rfu.set_flags(flags_mwrf);
    }

    return "";
}
