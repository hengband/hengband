/*!
 * @brief 巻物を読んだ際の効果処理
 * @date 2020/07/23
 * @author Hourier
 * @todo 長い、要分割
 */

#include "object-use/read-execution.h"
#include "action/action-limited.h"
#include "artifact/fixed-art-types.h"
#include "avatar/avatar.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/show-file.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player-info/equipment-info.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "spell-kind/magic-item-recharger.h"
#include "spell-kind/spells-curse-removal.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-enchant.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-genocide.h"
#include "spell-kind/spells-grid.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-neighbor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spell-types.h"
#include "spell/spells-object.h"
#include "spell/spells-summon.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "store/rumor.h"
#include "sv-definition/sv-scroll-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "view/display-messages.h"

/*!
 * @brief 巻物を読むコマンドのサブルーチン
 * Read a scroll (from the pack or floor).
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param item 読むオブジェクトの所持品ID
 * @param known 判明済ならばTRUE
 * @details
 * <pre>
 * Certain scrolls can be "aborted" without losing the scroll.  These
 * include scrolls with no effects but recharge or identify, which are
 * cancelled before use.  XXX Reading them still takes a turn, though.
 * </pre>
 */
void exe_read(player_type *creature_ptr, INVENTORY_IDX item, bool known)
{
    int k, used_up, ident, lev;
    object_type *o_ptr;
    o_ptr = ref_item(creature_ptr, item);
    PlayerEnergy(creature_ptr).set_player_turn_energy(100);
    if (cmd_limit_time_walk(creature_ptr))
        return;

    if (creature_ptr->pclass == CLASS_BERSERKER) {
        msg_print(_("巻物なんて読めない。", "You cannot read."));
        return;
    }

    if (music_singing_any(creature_ptr))
        stop_singing(creature_ptr);

    if (hex_spelling_any(creature_ptr) && ((creature_ptr->lev < 35) || hex_spell_fully(creature_ptr)))
        stop_hex_spell_all(creature_ptr);

    ident = false;
    lev = k_info[o_ptr->k_idx].level;
    used_up = true;
    if (o_ptr->tval == TV_SCROLL) {
        switch (o_ptr->sval) {
        case SV_SCROLL_DARKNESS: {
            if (!has_resist_blind(creature_ptr) && !has_resist_dark(creature_ptr))
                (void)set_blind(creature_ptr, creature_ptr->blind + 3 + randint1(5));

            if (unlite_area(creature_ptr, 10, 3))
                ident = true;

            break;
        }
        case SV_SCROLL_AGGRAVATE_MONSTER: {
            msg_print(_("カン高くうなる様な音が辺りを覆った。", "There is a high pitched humming noise."));
            aggravate_monsters(creature_ptr, 0);
            ident = true;
            break;
        }
        case SV_SCROLL_CURSE_ARMOR: {
            if (curse_armor(creature_ptr))
                ident = true;

            break;
        }
        case SV_SCROLL_CURSE_WEAPON: {
            k = 0;
            if (has_melee_weapon(creature_ptr, INVEN_MAIN_HAND)) {
                k = INVEN_MAIN_HAND;
                if (has_melee_weapon(creature_ptr, INVEN_SUB_HAND) && one_in_(2))
                    k = INVEN_SUB_HAND;
            } else if (has_melee_weapon(creature_ptr, INVEN_SUB_HAND))
                k = INVEN_SUB_HAND;

            if (k && curse_weapon_object(creature_ptr, false, &creature_ptr->inventory_list[k]))
                ident = true;

            break;
        }
        case SV_SCROLL_SUMMON_MONSTER: {
            for (k = 0; k < randint1(3); k++) {
                if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_NONE,
                        PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)) {
                    ident = true;
                }
            }

            break;
        }
        case SV_SCROLL_SUMMON_UNDEAD: {
            for (k = 0; k < randint1(3); k++) {
                if (summon_specific(creature_ptr, 0, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_UNDEAD,
                        PM_ALLOW_GROUP | PM_ALLOW_UNIQUE | PM_NO_PET)) {
                    ident = true;
                }
            }

            break;
        }
        case SV_SCROLL_SUMMON_PET: {
            if (summon_specific(
                    creature_ptr, -1, creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_NONE, PM_ALLOW_GROUP | PM_FORCE_PET))
                ident = true;

            break;
        }
        case SV_SCROLL_SUMMON_KIN: {
            if (summon_kin_player(creature_ptr, creature_ptr->lev, creature_ptr->y, creature_ptr->x, PM_FORCE_PET | PM_ALLOW_GROUP))
                ident = true;

            break;
        }
        case SV_SCROLL_TRAP_CREATION: {
            if (trap_creation(creature_ptr, creature_ptr->y, creature_ptr->x))
                ident = true;

            break;
        }
        case SV_SCROLL_PHASE_DOOR: {
            teleport_player(creature_ptr, 10, TELEPORT_SPONTANEOUS);
            ident = true;
            break;
        }
        case SV_SCROLL_TELEPORT: {
            teleport_player(creature_ptr, 100, TELEPORT_SPONTANEOUS);
            ident = true;
            break;
        }
        case SV_SCROLL_TELEPORT_LEVEL: {
            (void)teleport_level(creature_ptr, 0);
            ident = true;
            break;
        }
        case SV_SCROLL_WORD_OF_RECALL: {
            if (!recall_player(creature_ptr, randint0(21) + 15))
                used_up = false;

            ident = true;
            break;
        }
        case SV_SCROLL_IDENTIFY: {
            if (!ident_spell(creature_ptr, false))
                used_up = false;

            ident = true;
            break;
        }
        case SV_SCROLL_STAR_IDENTIFY: {
            if (!identify_fully(creature_ptr, false))
                used_up = false;

            ident = true;
            break;
        }
        case SV_SCROLL_REMOVE_CURSE: {
            if (remove_curse(creature_ptr))
                ident = true;

            break;
        }
        case SV_SCROLL_STAR_REMOVE_CURSE: {
            if (remove_all_curse(creature_ptr))
                ident = true;

            break;
        }
        case SV_SCROLL_ENCHANT_ARMOR: {
            ident = true;
            if (!enchant_spell(creature_ptr, 0, 0, 1))
                used_up = false;

            break;
        }
        case SV_SCROLL_ENCHANT_WEAPON_TO_HIT: {
            if (!enchant_spell(creature_ptr, 1, 0, 0))
                used_up = false;

            ident = true;
            break;
        }
        case SV_SCROLL_ENCHANT_WEAPON_TO_DAM: {
            if (!enchant_spell(creature_ptr, 0, 1, 0))
                used_up = false;

            ident = true;
            break;
        }
        case SV_SCROLL_STAR_ENCHANT_ARMOR: {
            if (!enchant_spell(creature_ptr, 0, 0, randint1(3) + 2))
                used_up = false;

            ident = true;
            break;
        }
        case SV_SCROLL_STAR_ENCHANT_WEAPON: {
            if (!enchant_spell(creature_ptr, randint1(3), randint1(3), 0))
                used_up = false;

            ident = true;
            break;
        }
        case SV_SCROLL_RECHARGING: {
            if (!recharge(creature_ptr, 130))
                used_up = false;

            ident = true;
            break;
        }
        case SV_SCROLL_MUNDANITY: {
            ident = true;
            if (!mundane_spell(creature_ptr, false))
                used_up = false;

            break;
        }
        case SV_SCROLL_LIGHT: {
            if (lite_area(creature_ptr, damroll(2, 8), 2))
                ident = true;
            break;
        }

        case SV_SCROLL_MAPPING: {
            map_area(creature_ptr, DETECT_RAD_MAP);
            ident = true;
            break;
        }
        case SV_SCROLL_DETECT_GOLD: {
            if (detect_treasure(creature_ptr, DETECT_RAD_DEFAULT) || detect_objects_gold(creature_ptr, DETECT_RAD_DEFAULT))
                ident = true;

            break;
        }
        case SV_SCROLL_DETECT_ITEM: {
            if (detect_objects_normal(creature_ptr, DETECT_RAD_DEFAULT))
                ident = true;

            break;
        }
        case SV_SCROLL_DETECT_TRAP: {
            if (detect_traps(creature_ptr, DETECT_RAD_DEFAULT, known))
                ident = true;

            break;
        }
        case SV_SCROLL_DETECT_DOOR: {
            if (detect_doors(creature_ptr, DETECT_RAD_DEFAULT) || detect_stairs(creature_ptr, DETECT_RAD_DEFAULT))
                ident = true;

            break;
        }
        case SV_SCROLL_DETECT_INVIS: {
            if (detect_monsters_invis(creature_ptr, DETECT_RAD_DEFAULT))
                ident = true;

            break;
        }
        case SV_SCROLL_SATISFY_HUNGER: {
            if (set_food(creature_ptr, PY_FOOD_MAX - 1))
                ident = true;

            break;
        }
        case SV_SCROLL_BLESSING: {
            if (set_blessed(creature_ptr, creature_ptr->blessed + randint1(12) + 6, false))
                ident = true;

            break;
        }
        case SV_SCROLL_HOLY_CHANT: {
            if (set_blessed(creature_ptr, creature_ptr->blessed + randint1(24) + 12, false))
                ident = true;
            break;
        }

        case SV_SCROLL_HOLY_PRAYER: {
            if (set_blessed(creature_ptr, creature_ptr->blessed + randint1(48) + 24, false))
                ident = true;

            break;
        }
        case SV_SCROLL_MONSTER_CONFUSION: {
            if (!(creature_ptr->special_attack & ATTACK_CONFUSE)) {
                msg_print(_("手が輝き始めた。", "Your hands begin to glow."));
                creature_ptr->special_attack |= ATTACK_CONFUSE;
                creature_ptr->redraw |= PR_STATUS;
                ident = true;
            }

            break;
        }
        case SV_SCROLL_PROTECTION_FROM_EVIL: {
            k = 3 * creature_ptr->lev;
            if (set_protevil(creature_ptr, creature_ptr->protevil + randint1(25) + k, false))
                ident = true;

            break;
        }
        case SV_SCROLL_RUNE_OF_PROTECTION: {
            create_rune_protection_one(creature_ptr);
            ident = true;
            break;
        }
        case SV_SCROLL_TRAP_DOOR_DESTRUCTION: {
            if (destroy_doors_touch(creature_ptr))
                ident = true;

            break;
        }
        case SV_SCROLL_STAR_DESTRUCTION: {
            if (destroy_area(creature_ptr, creature_ptr->y, creature_ptr->x, 13 + randint0(5), false))
                ident = true;
            else
                msg_print(_("ダンジョンが揺れた...", "The dungeon trembles..."));

            break;
        }
        case SV_SCROLL_DISPEL_UNDEAD: {
            if (dispel_undead(creature_ptr, 80))
                ident = true;

            break;
        }
        case SV_SCROLL_SPELL: {
            if ((creature_ptr->pclass == CLASS_WARRIOR) || (creature_ptr->pclass == CLASS_IMITATOR) || (creature_ptr->pclass == CLASS_MINDCRAFTER)
                || (creature_ptr->pclass == CLASS_SORCERER) || (creature_ptr->pclass == CLASS_ARCHER) || (creature_ptr->pclass == CLASS_MAGIC_EATER)
                || (creature_ptr->pclass == CLASS_RED_MAGE) || (creature_ptr->pclass == CLASS_SAMURAI) || (creature_ptr->pclass == CLASS_BLUE_MAGE)
                || (creature_ptr->pclass == CLASS_CAVALRY) || (creature_ptr->pclass == CLASS_BERSERKER) || (creature_ptr->pclass == CLASS_SMITH)
                || (creature_ptr->pclass == CLASS_MIRROR_MASTER) || (creature_ptr->pclass == CLASS_NINJA) || (creature_ptr->pclass == CLASS_SNIPER))
                break;

            creature_ptr->add_spells++;
            creature_ptr->update |= PU_SPELLS;
            ident = true;
            break;
        }
        case SV_SCROLL_GENOCIDE: {
            (void)symbol_genocide(creature_ptr, 300, true);
            ident = true;
            break;
        }
        case SV_SCROLL_MASS_GENOCIDE: {
            (void)mass_genocide(creature_ptr, 300, true);
            ident = true;
            break;
        }
        case SV_SCROLL_ACQUIREMENT: {
            acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, 1, true, false, false);
            ident = true;
            break;
        }
        case SV_SCROLL_STAR_ACQUIREMENT: {
            acquirement(creature_ptr, creature_ptr->y, creature_ptr->x, randint1(2) + 1, true, false, false);
            ident = true;
            break;
        }
        case SV_SCROLL_FIRE: {
            fire_ball(creature_ptr, GF_FIRE, 0, 666, 4);
            if (!(is_oppose_fire(creature_ptr) || has_resist_fire(creature_ptr) || has_immune_fire(creature_ptr)))
                take_hit(creature_ptr, DAMAGE_NOESCAPE, 50 + randint1(50), _("炎の巻物", "a Scroll of Fire"));

            ident = true;
            break;
        }
        case SV_SCROLL_ICE: {
            fire_ball(creature_ptr, GF_ICE, 0, 777, 4);
            if (!(is_oppose_cold(creature_ptr) || has_resist_cold(creature_ptr) || has_immune_cold(creature_ptr)))
                take_hit(creature_ptr, DAMAGE_NOESCAPE, 100 + randint1(100), _("氷の巻物", "a Scroll of Ice"));

            ident = true;
            break;
        }
        case SV_SCROLL_CHAOS: {
            fire_ball(creature_ptr, GF_CHAOS, 0, 1000, 4);
            if (!has_resist_chaos(creature_ptr))
                take_hit(creature_ptr, DAMAGE_NOESCAPE, 111 + randint1(111), _("ログルスの巻物", "a Scroll of Logrus"));

            ident = true;
            break;
        }
        case SV_SCROLL_RUMOR: {
            msg_print(_("巻物にはメッセージが書かれている:", "There is message on the scroll. It says:"));
            msg_print(NULL);
            display_rumor(creature_ptr, true);
            msg_print(NULL);
            msg_print(_("巻物は煙を立てて消え去った！", "The scroll disappears in a puff of smoke!"));
            ident = true;
            break;
        }
        case SV_SCROLL_ARTIFACT: {
            ident = true;
            if (!artifact_scroll(creature_ptr))
                used_up = false;

            break;
        }
        case SV_SCROLL_RESET_RECALL: {
            ident = true;
            if (!reset_recall(creature_ptr))
                used_up = false;

            break;
        }
        case SV_SCROLL_AMUSEMENT: {
            ident = true;
            amusement(creature_ptr, creature_ptr->y, creature_ptr->x, 1, false);
            break;
        }
        case SV_SCROLL_STAR_AMUSEMENT: {
            ident = true;
            amusement(creature_ptr, creature_ptr->y, creature_ptr->x, randint1(2) + 1, false);
            break;
        }
        }
    } else if (o_ptr->name1 == ART_GHB) {
        msg_print(_("私は苦労して『グレーター・ヘル=ビースト』を倒した。", "I had a very hard time to kill the Greater hell-beast, "));
        msg_print(_("しかし手に入ったのはこのきたないＴシャツだけだった。", "but all I got was this lousy t-shirt!"));
        used_up = false;
    } else if (o_ptr->name1 == ART_POWER) {
        msg_print(_("「一つの指輪は全てを統べ、", "'One Ring to rule them all, "));
        msg_print(NULL);
        msg_print(_("一つの指輪は全てを見つけ、", "One Ring to find them, "));
        msg_print(NULL);
        msg_print(_("一つの指輪は全てを捕らえて", "One Ring to bring them all "));
        msg_print(NULL);
        msg_print(_("暗闇の中に繋ぎとめる。」", "and in the darkness bind them.'"));
        used_up = false;
    } else if (o_ptr->tval == TV_PARCHMENT) {
        concptr q;
        GAME_TEXT o_name[MAX_NLEN];
        char buf[1024];
        screen_save();
        q = format("book-%d_jp.txt", o_ptr->sval);
        describe_flavor(creature_ptr, o_name, o_ptr, OD_NAME_ONLY);
        path_build(buf, sizeof(buf), ANGBAND_DIR_FILE, q);
        (void)show_file(creature_ptr, true, buf, o_name, 0, 0);
        screen_load();
        used_up = false;
    }

    BIT_FLAGS inventory_flags = PU_COMBINE | PU_REORDER | (creature_ptr->update & PU_AUTODESTROY);
    creature_ptr->update &= ~(PU_COMBINE | PU_REORDER | PU_AUTODESTROY);

    if (!(o_ptr->is_aware())) {
        chg_virtue(creature_ptr, V_PATIENCE, -1);
        chg_virtue(creature_ptr, V_CHANCE, 1);
        chg_virtue(creature_ptr, V_KNOWLEDGE, -1);
    }

    object_tried(o_ptr);
    if (ident && !o_ptr->is_aware()) {
        object_aware(creature_ptr, o_ptr);
        gain_exp(creature_ptr, (lev + (creature_ptr->lev >> 1)) / creature_ptr->lev);
    }

    creature_ptr->window_flags |= PW_INVEN | PW_EQUIP | PW_PLAYER;
    creature_ptr->update |= inventory_flags;
    if (!used_up)
        return;

    sound(SOUND_SCROLL);
    vary_item(creature_ptr, item, -1);
}
