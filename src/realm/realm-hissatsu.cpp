#include "realm/realm-hissatsu.h"
#include "artifact/fixed-art-types.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-action/cmd-spell.h"
#include "cmd-item/cmd-throw.h"
#include "combat/combat-options-type.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon-flag-types.h"
#include "effect/attribute-types.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/feature-flag-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "mind/mind-ninja.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "player-info/equipment-info.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell/technic-info-table.h"
#include "status/bad-status-setter.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/grid-selector.h"
#include "target/projection-path-calculator.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 剣術の各処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param spell 剣術ID
 * @param mode 処理内容 (SpellProcessType::NAME / SPELL_DESC / SpellProcessType::CAST)
 * @return SpellProcessType::NAME / SPELL_DESC 時には文字列ポインタを返す。SpellProcessType::CAST時はnullptr文字列を返す。
 */
concptr do_hissatsu_spell(PlayerType *player_ptr, SPELL_IDX spell, SpellProcessType mode)
{
    bool name = mode == SpellProcessType::NAME;
    bool desc = mode == SpellProcessType::DESCRIPTION;
    bool cast = mode == SpellProcessType::CAST;

    DIRECTION dir;
    PLAYER_LEVEL plev = player_ptr->lev;

    switch (spell) {
    case 0:
        if (name) {
            return _("飛飯綱", "Tobi-Izuna");
        }
        if (desc) {
            return _("2マス離れたところにいるモンスターを攻撃する。", "Attacks a monster two squares away.");
        }

        if (cast) {
            project_length = 2;
            if (!get_aim_dir(player_ptr, &dir)) {
                return nullptr;
            }

            project_hook(player_ptr, AttributeType::ATTACK, dir, HISSATSU_2, PROJECT_STOP | PROJECT_KILL);
        }
        break;

    case 1:
        if (name) {
            return _("五月雨斬り", "3-Way Attack");
        }
        if (desc) {
            return _("3方向に対して攻撃する。", "Attacks in 3 directions at one time.");
        }

        if (cast) {
            DIRECTION cdir;
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            for (cdir = 0; cdir < 8; cdir++) {
                if (cdd[cdir] == dir) {
                    break;
                }
            }

            if (cdir == 8) {
                return nullptr;
            }

            y = player_ptr->y + ddy_cdd[cdir];
            x = player_ptr->x + ddx_cdd[cdir];
            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
            } else {
                msg_print(_("攻撃は空を切った。", "You attack the empty air."));
            }

            y = player_ptr->y + ddy_cdd[(cdir + 7) % 8];
            x = player_ptr->x + ddx_cdd[(cdir + 7) % 8];
            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
            } else {
                msg_print(_("攻撃は空を切った。", "You attack the empty air."));
            }

            y = player_ptr->y + ddy_cdd[(cdir + 1) % 8];
            x = player_ptr->x + ddx_cdd[(cdir + 1) % 8];
            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
            } else {
                msg_print(_("攻撃は空を切った。", "You attack the empty air."));
            }
        }
        break;

    case 2:
        if (name) {
            return _("ブーメラン", "Boomerang");
        }
        if (desc) {
            return _(
                "武器を手元に戻ってくるように投げる。戻ってこないこともある。", "Throws current weapon. It'll return to your hand unless the action failed.");
        }

        if (cast) {
            if (!ThrowCommand(player_ptr).do_cmd_throw(1, true, -1)) {
                return nullptr;
            }
        }
        break;

    case 3:
        if (name) {
            return _("焔霊", "Burning Strike");
        }
        if (desc) {
            return _("火炎耐性のないモンスターに大ダメージを与える。", "Attacks a monster with more damage unless it has resistance to fire.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_FIRE);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }
        }
        break;

    case 4:
        if (name) {
            return _("殺気感知", "Detect Ferocity");
        }
        if (desc) {
            return _("近くの思考することができるモンスターを感知する。", "Detects all monsters except the mindless in your vicinity.");
        }

        if (cast) {
            detect_monsters_mind(player_ptr, DETECT_RAD_DEFAULT);
        }
        break;

    case 5:
        if (name) {
            return _("みね打ち", "Strike to Stun");
        }
        if (desc) {
            return _("相手にダメージを与えないが、朦朧とさせる。", "Attempts to stun a monster next to you.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_MINEUCHI);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }
        }
        break;

    case 6:
        if (name) {
            return _("カウンター", "Counter");
        }
        if (desc) {
            return _("相手に攻撃されたときに反撃する。反撃するたびにMPを消費。",
                "Prepares to counterattack. When attacked by a monster, strikes back using SP each time.");
        }

        if (cast) {
            if (player_ptr->riding) {
                msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
                return nullptr;
            }
            msg_print(_("相手の攻撃に対して身構えた。", "You prepare to counterattack."));
            player_ptr->counter = true;
        }
        break;

    case 7:
        if (name) {
            return _("払い抜け", "Harainuke");
        }
        if (desc) {
            return _("攻撃した後、反対側に抜ける。",
                "In one action, attacks a monster with your weapons normally and then moves to the space beyond the monster if that space is not blocked.");
        }

        if (cast) {
            POSITION y, x;

            if (player_ptr->riding) {
                msg_print(_("乗馬中には無理だ。", "You cannot do it when riding."));
                return nullptr;
            }

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }

            if (dir == 5) {
                return nullptr;
            }
            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (!player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }

            do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);

            if (!player_can_enter(player_ptr, player_ptr->current_floor_ptr->grid_array[y][x].feat, 0) || is_trap(player_ptr, player_ptr->current_floor_ptr->grid_array[y][x].feat)) {
                break;
            }

            y += ddy[dir];
            x += ddx[dir];

            if (player_can_enter(player_ptr, player_ptr->current_floor_ptr->grid_array[y][x].feat, 0) && !is_trap(player_ptr, player_ptr->current_floor_ptr->grid_array[y][x].feat) && !player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                msg_print(nullptr);
                (void)move_player_effect(player_ptr, y, x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP);
            }
        }
        break;

    case 8:
        if (name) {
            return _("サーペンツタン", "Serpent's Tongue");
        }
        if (desc) {
            return _("毒耐性のないモンスターに大ダメージを与える。", "Attacks a monster with more damage unless it has resistance to poison.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_POISON);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }
        }
        break;

    case 9:
        if (name) {
            return _("斬魔剣弐の太刀", "Zammaken");
        }
        if (desc) {
            return _("生命のない邪悪なモンスターに大ダメージを与えるが、他のモンスターには全く効果がない。",
                "Attacks an evil unliving monster with great damage. Has no effect on other monsters.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_ZANMA);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }
        }
        break;

    case 10:
        if (name) {
            return _("裂風剣", "Wind Blast");
        }
        if (desc) {
            return _("攻撃した相手を後方へ吹き飛ばす。", "Attacks an adjacent monster and blows it away.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }
            if (dungeons_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_MELEE)) {
                return "";
            }
            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                int i;
                POSITION ty = y, tx = x;
                POSITION oy = y, ox = x;
                MONSTER_IDX m_idx = player_ptr->current_floor_ptr->grid_array[y][x].m_idx;
                auto *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
                GAME_TEXT m_name[MAX_NLEN];

                monster_desc(player_ptr, m_name, m_ptr, 0);

                for (i = 0; i < 5; i++) {
                    y += ddy[dir];
                    x += ddx[dir];
                    if (is_cave_empty_bold(player_ptr, y, x)) {
                        ty = y;
                        tx = x;
                    } else {
                        break;
                    }
                }
                if ((ty != oy) || (tx != ox)) {
                    msg_format(_("%sを吹き飛ばした！", "You blow %s away!"), m_name);
                    player_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = 0;
                    player_ptr->current_floor_ptr->grid_array[ty][tx].m_idx = m_idx;
                    m_ptr->fy = ty;
                    m_ptr->fx = tx;

                    update_monster(player_ptr, m_idx, true);
                    lite_spot(player_ptr, oy, ox);
                    lite_spot(player_ptr, ty, tx);

                    if (monraces_info[m_ptr->r_idx].flags7 & (RF7_LITE_MASK | RF7_DARK_MASK)) {
                        player_ptr->update |= (PU_MON_LITE);
                    }
                }
            }
        }
        break;

    case 11:
        if (name) {
            return _("刀匠の目利き", "Judge");
        }
        if (desc) {
            return _("武器・防具を1つ識別する。レベル45以上で武器・防具の能力を完全に知ることができる。",
                "Identifies a weapon or armor. *Identifies* the item at level 45.");
        }

        if (cast) {
            if (plev > 44) {
                if (!identify_fully(player_ptr, true)) {
                    return nullptr;
                }
            } else {
                if (!ident_spell(player_ptr, true)) {
                    return nullptr;
                }
            }
        }
        break;

    case 12:
        if (name) {
            return _("破岩斬", "Rock Smash");
        }
        if (desc) {
            return _("岩を壊し、岩石系のモンスターに大ダメージを与える。", "Breaks rock or greatly damages a monster made of rocks.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_HAGAN);
            }

            if (!cave_has_flag_bold(player_ptr->current_floor_ptr, y, x, TerrainCharacteristics::HURT_ROCK)) {
                break;
            }

            /* Destroy the feature */
            cave_alter_feat(player_ptr, y, x, TerrainCharacteristics::HURT_ROCK);
            player_ptr->update |= (PU_FLOW);
        }
        break;

    case 13:
        if (name) {
            return _("乱れ雪月花", "Midare-Setsugekka");
        }
        if (desc) {
            return _("攻撃回数が増え、冷気耐性のないモンスターに大ダメージを与える。",
                "Attacks a monster with an increased number of attacks and more damage unless it has resistance to cold.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_COLD);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }
        }
        break;

    case 14:
        if (name) {
            return _("急所突き", "Spot Aiming");
        }
        if (desc) {
            return _("モンスターを一撃で倒す攻撃を繰り出す。失敗すると1点しかダメージを与えられない。",
                "Attempts to kill a monster instantly. If that fails, causes only 1HP of damage.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_KYUSHO);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }
        }
        break;

    case 15:
        if (name) {
            return _("魔神斬り", "Majingiri");
        }
        if (desc) {
            return _("会心の一撃で攻撃する。攻撃がかわされやすい。", "Attempts to attack with a critical hit, but this attack is easy to evade for a monster.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_MAJIN);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }
        }
        break;

    case 16:
        if (name) {
            return _("捨て身", "Desperate Attack");
        }
        if (desc) {
            return _("強力な攻撃を繰り出す。次のターンまでの間、食らうダメージが増える。",
                "Attacks with all of your power, but all damage you take will be doubled for one turn.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_SUTEMI);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }
            player_ptr->sutemi = true;
        }
        break;

    case 17:
        if (name) {
            return _("雷撃鷲爪斬", "Lightning Eagle");
        }
        if (desc) {
            return _("電撃耐性のないモンスターに非常に大きいダメージを与える。", "Attacks a monster with more damage unless it has resistance to electricity.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_ELEC);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }
        }
        break;

    case 18:
        if (name) {
            return _("入身", "Rush Attack");
        }
        if (desc) {
            return _("素早く相手に近寄り攻撃する。", "Steps close to a monster and attacks at the same time.");
        }

        if (cast) {
            if (!rush_attack(player_ptr, nullptr)) {
                return nullptr;
            }
        }
        break;

    case 19:
        if (name) {
            return _("赤流渦", "Bloody Maelstrom");
        }

        if (desc) {
            return _("自分自身も傷を作りつつ、その傷が深いほど大きい威力で全方向の敵を攻撃できる。生きていないモンスターには効果がない。",
                "Attacks all adjacent monsters with power corresponding to your cuts. Then increases your cuts. Has no effect on unliving monsters.");
        }

        if (cast) {
            POSITION y = 0, x = 0;
            auto current_cut = player_ptr->effects()->cut()->current();
            short new_cut = current_cut < 300 ? current_cut + 300 : current_cut * 2;
            (void)BadStatusSetter(player_ptr).set_cut(new_cut);
            for (dir = 0; dir < 8; dir++) {
                y = player_ptr->y + ddy_ddd[dir];
                x = player_ptr->x + ddx_ddd[dir];
                auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
                auto *m_ptr = &player_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
                if ((g_ptr->m_idx == 0) || (!m_ptr->ml && !cave_has_flag_bold(player_ptr->current_floor_ptr, y, x, TerrainCharacteristics::PROJECT))) {
                    continue;
                }

                if (monster_living(m_ptr->r_idx)) {
                    do_cmd_attack(player_ptr, y, x, HISSATSU_SEKIRYUKA);
                    continue;
                }

                GAME_TEXT m_name[MAX_NLEN];
                monster_desc(player_ptr, m_name, m_ptr, 0);
                msg_format(_("%sには効果がない！", "%s is unharmed!"), m_name);
            }
        }

        break;
    case 20:
        if (name) {
            return _("激震撃", "Earthquake Blow");
        }
        if (desc) {
            return _("地震を起こす。", "Shakes dungeon structure, and results in random swapping of floors and walls.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_QUAKE);
            } else {
                earthquake(player_ptr, player_ptr->y, player_ptr->x, 10, 0);
            }
        }
        break;

    case 21:
        if (name) {
            return _("地走り", "Crack");
        }
        if (desc) {
            return _("衝撃波のビームを放つ。", "Fires a shock wave as a beam.");
        }

        if (cast) {
            int total_damage = 0, basedam, i;
            ObjectType *o_ptr;
            if (!get_aim_dir(player_ptr, &dir)) {
                return nullptr;
            }
            msg_print(_("武器を大きく振り下ろした。", "You swing your weapon downward."));
            for (i = 0; i < 2; i++) {
                int damage;

                if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i)) {
                    break;
                }
                o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + i];
                basedam = (o_ptr->dd * (o_ptr->ds + 1)) * 50;
                damage = o_ptr->to_d * 100;
                auto flgs = object_flags(o_ptr);

                // @todo ヴォーパルの多重定義.
                if (o_ptr->is_specific_artifact(FixedArtifactId::VORPAL_BLADE) || o_ptr->is_specific_artifact(FixedArtifactId::CHAINSWORD)) {
                    /* vorpal blade */
                    basedam *= 5;
                    basedam /= 3;
                } else if (flgs.has(TR_VORPAL)) {
                    /* vorpal flag only */
                    basedam *= 11;
                    basedam /= 9;
                }
                damage += basedam;
                damage *= player_ptr->num_blow[i];
                total_damage += damage / 200;
                if (i) {
                    total_damage = total_damage * 7 / 10;
                }
            }
            fire_beam(player_ptr, AttributeType::FORCE, dir, total_damage);
        }
        break;

    case 22:
        if (name) {
            return _("気迫の雄叫び", "War Cry");
        }
        if (desc) {
            return _("視界内の全モンスターに対して轟音の攻撃を行う。さらに、近くにいるモンスターを怒らせる。",
                "Damages all monsters in sight with sound. Aggravates nearby monsters.");
        }

        if (cast) {
            msg_print(_("雄叫びをあげた！", "You roar!"));
            project_all_los(player_ptr, AttributeType::SOUND, randint1(plev * 3));
            aggravate_monsters(player_ptr, 0);
        }
        break;

    case 23:
        if (name) {
            return _("無双三段", "Musou-Sandan");
        }
        if (desc) {
            return _("強力な3段攻撃を繰り出す。", "Attacks with three powerful strikes.");
        }

        if (cast) {
            int i;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            for (i = 0; i < 3; i++) {
                POSITION y, x;
                POSITION ny, nx;
                MONSTER_IDX m_idx;
                grid_type *g_ptr;
                monster_type *m_ptr;

                y = player_ptr->y + ddy[dir];
                x = player_ptr->x + ddx[dir];
                g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];

                if (g_ptr->m_idx) {
                    do_cmd_attack(player_ptr, y, x, HISSATSU_3DAN);
                } else {
                    msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                    return nullptr;
                }

                if (dungeons_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_MELEE)) {
                    return "";
                }

                /* Monster is dead? */
                if (!g_ptr->m_idx) {
                    break;
                }

                ny = y + ddy[dir];
                nx = x + ddx[dir];
                m_idx = g_ptr->m_idx;
                m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];

                /* Monster cannot move back? */
                if (!monster_can_enter(player_ptr, ny, nx, &monraces_info[m_ptr->r_idx], 0)) {
                    /* -more- */
                    if (i < 2) {
                        msg_print(nullptr);
                    }
                    continue;
                }

                g_ptr->m_idx = 0;
                player_ptr->current_floor_ptr->grid_array[ny][nx].m_idx = m_idx;
                m_ptr->fy = ny;
                m_ptr->fx = nx;

                update_monster(player_ptr, m_idx, true);

                /* Redraw the old spot */
                lite_spot(player_ptr, y, x);

                /* Redraw the new spot */
                lite_spot(player_ptr, ny, nx);

                /* Player can move forward? */
                if (player_can_enter(player_ptr, g_ptr->feat, 0)) {
                    if (!move_player_effect(player_ptr, y, x, MPE_FORGET_FLOW | MPE_HANDLE_STUFF | MPE_DONT_PICKUP)) {
                        break;
                    }
                } else {
                    break;
                }

                /* -more- */
                if (i < 2) {
                    msg_print(nullptr);
                }
            }
        }
        break;

    case 24:
        if (name) {
            return _("吸血鬼の牙", "Vampire's Fang");
        }
        if (desc) {
            return _("攻撃した相手の体力を吸いとり、自分の体力を回復させる。生命を持たないモンスターには通じない。",
                "Attacks with vampiric strikes which absorb HP from a monster and heal you. Has no effect on unliving monsters.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_DRAIN);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }
        }
        break;

    case 25:
        if (name) {
            return _("幻惑", "Moon Dazzling");
        }
        if (desc) {
            return _("視界内の起きている全モンスターに朦朧、混乱、眠りを与えようとする。", "Attempts to stun, confuse and put to sleep all waking monsters.");
        }

        if (cast) {
            msg_print(_("武器を不規則に揺らした．．．", "You irregularly wave your weapon..."));
            project_all_los(player_ptr, AttributeType::ENGETSU, plev * 4);
        }
        break;

    case 26:
        if (name) {
            return _("百人斬り", "Hundred Slaughter");
        }
        if (desc) {
            return _("連続して入身でモンスターを攻撃する。攻撃するたびにMPを消費。MPがなくなるか、モンスターを倒せなかったら百人斬りは終了する。",
                "Performs a series of rush attacks. The series continues as long as the attacked monster dies and you have sufficient SP.");
        }

        if (cast) {
            const int mana_cost_per_monster = 8;
            bool is_new = true;
            bool mdeath;

            do {
                if (!rush_attack(player_ptr, &mdeath)) {
                    break;
                }
                if (is_new) {
                    /* Reserve needed mana point */
                    player_ptr->csp -= technic_info[REALM_HISSATSU - MIN_TECHNIC][26].smana;
                    is_new = false;
                } else {
                    player_ptr->csp -= mana_cost_per_monster;
                }

                if (!mdeath) {
                    break;
                }
                command_dir = 0;

                player_ptr->redraw |= PR_MANA;
                handle_stuff(player_ptr);
            } while (player_ptr->csp > mana_cost_per_monster);

            if (is_new) {
                return nullptr;
            }

            /* Restore reserved mana */
            player_ptr->csp += technic_info[REALM_HISSATSU - MIN_TECHNIC][26].smana;
        }
        break;

    case 27:
        if (name) {
            return _("天翔龍閃", "Dragonic Flash");
        }
        if (desc) {
            return _("視界内の場所を指定して、その場所と自分の間にいる全モンスターを攻撃し、その場所に移動する。",
                "Runs toward given location while attacking all monsters on the path.");
        }

        if (cast) {
            POSITION y, x;

            if (!tgt_pt(player_ptr, &x, &y)) {
                return nullptr;
            }

            if (!cave_player_teleportable_bold(player_ptr, y, x, TELEPORT_SPONTANEOUS) || (distance(y, x, player_ptr->y, player_ptr->x) > MAX_SIGHT / 2) || !projectable(player_ptr, player_ptr->y, player_ptr->x, y, x)) {
                msg_print(_("失敗！", "You cannot move to that place!"));
                break;
            }
            if (player_ptr->anti_tele) {
                msg_print(_("不思議な力がテレポートを防いだ！", "A mysterious force prevents you from teleporting!"));
                break;
            }
            project(player_ptr, 0, 0, y, x, HISSATSU_ISSEN, AttributeType::ATTACK, PROJECT_BEAM | PROJECT_KILL);
            teleport_player_to(player_ptr, y, x, TELEPORT_SPONTANEOUS);
        }
        break;

    case 28:
        if (name) {
            return _("二重の剣撃", "Twin Slash");
        }
        if (desc) {
            return _("1ターンで2度攻撃を行う。", "Attack twice in one turn.");
        }

        if (cast) {
            POSITION x, y;

            if (!get_rep_dir(player_ptr, &dir, false)) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
                if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                    handle_stuff(player_ptr);
                    do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
                }
            } else {
                msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
                return nullptr;
            }
        }
        break;

    case 29:
        if (name) {
            return _("虎伏絶刀勢", "Kofuku-Zettousei");
        }
        if (desc) {
            return _("強力な攻撃を行い、近くの場所にも効果が及ぶ。", "Performs a powerful attack which even affects nearby monsters.");
        }

        if (cast) {
            int total_damage = 0, basedam, i;
            POSITION y, x;
            ObjectType *o_ptr;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (dungeons_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_MELEE)) {
                msg_print(_("なぜか攻撃することができない。", "Something prevents you from attacking."));
                return "";
            }
            msg_print(_("武器を大きく振り下ろした。", "You swing your weapon downward."));
            for (i = 0; i < 2; i++) {
                int damage;
                if (!has_melee_weapon(player_ptr, INVEN_MAIN_HAND + i)) {
                    break;
                }
                o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND + i];
                basedam = (o_ptr->dd * (o_ptr->ds + 1)) * 50;
                damage = o_ptr->to_d * 100;
                auto flgs = object_flags(o_ptr);

                // @todo ヴォーパルの多重定義.
                if (o_ptr->is_specific_artifact(FixedArtifactId::VORPAL_BLADE) || o_ptr->is_specific_artifact(FixedArtifactId::CHAINSWORD)) {
                    /* vorpal blade */
                    basedam *= 5;
                    basedam /= 3;
                } else if (flgs.has(TR_VORPAL)) {
                    /* vorpal flag only */
                    basedam *= 11;
                    basedam /= 9;
                }
                damage += basedam;
                damage += player_ptr->to_d[i] * 100;
                damage *= player_ptr->num_blow[i];
                total_damage += (damage / 100);
            }
            project(player_ptr, 0, (cave_has_flag_bold(player_ptr->current_floor_ptr, y, x, TerrainCharacteristics::PROJECT) ? 5 : 0), y, x, total_damage * 3 / 2, AttributeType::METEOR,
                PROJECT_KILL | PROJECT_JUMP | PROJECT_ITEM);
        }
        break;

    case 30:
        if (name) {
            return _("慶雲鬼忍剣", "Keiun-Kininken");
        }
        if (desc) {
            return _("自分もダメージをくらうが、相手に非常に大きなダメージを与える。アンデッドには特に効果がある。",
                "Attacks a monster with extremely powerful damage, but you also take some damage. Hurts an undead monster greatly.");
        }

        if (cast) {
            POSITION y, x;

            if (!get_direction(player_ptr, &dir, false, false)) {
                return nullptr;
            }
            if (dir == 5) {
                return nullptr;
            }

            y = player_ptr->y + ddy[dir];
            x = player_ptr->x + ddx[dir];

            if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
                do_cmd_attack(player_ptr, y, x, HISSATSU_UNDEAD);
            } else {
                msg_print(_("その方向にはモンスターはいません。", "There is no monster."));
                return nullptr;
            }
            take_hit(player_ptr, DAMAGE_NOESCAPE, 100 + randint1(100), _("慶雲鬼忍剣を使った衝撃", "exhaustion on using Keiun-Kininken"));
        }
        break;

    case 31:
        if (name) {
            return _("切腹", "Harakiri");
        }
        if (desc) {
            return _("「武士道とは、死ぬことと見つけたり。」", "'Bushido, the way of warriors, is found in death'");
        }

        if (cast) {
            int i;
            if (!get_check(_("本当に自殺しますか？", "Do you really want to commit suicide? "))) {
                return nullptr;
            }
            /* Special Verification for suicide */
            prt(_("確認のため '@' を押して下さい。", "Please verify SUICIDE by typing the '@' sign: "), 0, 0);

            flush();
            i = inkey();
            prt("", 0, 0);
            if (i != '@') {
                return nullptr;
            }
            if (w_ptr->total_winner) {
                take_hit(player_ptr, DAMAGE_FORCE, 9999, "Seppuku");
                w_ptr->total_winner = true;
            } else {
                msg_print(_("武士道とは、死ぬことと見つけたり。", "The meaning of bushido is found in death."));
                take_hit(player_ptr, DAMAGE_FORCE, 9999, "Seppuku");
            }
        }
        break;
    }

    return "";
}
