#include "mutation/mutation-processor.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "dungeon/dungeon.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "hpmp/hp-mp-processor.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster/monster-status.h"
#include "mutation/mutation-flag-types.h"
#include "mutation/mutation-investor-remover.h"
#include "object/lite-processor.h"
#include "player-info/equipment-info.h"
#include "player/digestion-processor.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-lite.h"
#include "spell-kind/spells-sight.h"
#include "spell-kind/spells-teleport.h"
#include "spell-kind/spells-world.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spell-types.h"
#include "spell/summon-types.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

static bool get_hack_dir(player_type *creature_ptr, DIRECTION *dp)
{
    *dp = 0;
    char command;
    DIRECTION dir = 0;
    while (!dir) {
        concptr p = target_okay(creature_ptr)
            ? _("方向 ('5'でターゲットへ, '*'でターゲット再選択, ESCで中断)? ", "Direction ('5' for target, '*' to re-target, Escape to cancel)? ")
            : _("方向 ('*'でターゲット選択, ESCで中断)? ", "Direction ('*' to choose a target, Escape to cancel)? ");
        if (!get_com(p, &command, true))
            break;

        if (use_menu && (command == '\r'))
            command = 't';

        switch (command) {
        case 'T':
        case 't':
        case '.':
        case '5':
        case '0':
            dir = 5;
            break;
        case '*':
        case ' ':
        case '\r':
            if (target_set(creature_ptr, TARGET_KILL))
                dir = 5;

            break;
        default:
            dir = get_keymap_dir(command);
            break;
        }

        if ((dir == 5) && !target_okay(creature_ptr))
            dir = 0;

        if (!dir)
            bell();
    }

    if (!dir)
        return false;

    command_dir = dir;
    if (creature_ptr->confused)
        dir = ddd[randint0(8)];

    if (command_dir != dir)
        msg_print(_("あなたは混乱している。", "You are confused."));

    *dp = dir;
    return true;
}

/*!
 * @brief 10ゲームターンが進行するごとに突然変異の発動判定を行う処理
 * / Handle mutation effects once every 10 game turns
 */
void process_world_aux_mutation(player_type *creature_ptr)
{
    if (creature_ptr->muta.none() || creature_ptr->phase_out || creature_ptr->wild_mode)
        return;

    if (creature_ptr->muta.has(MUTA::BERS_RAGE) && one_in_(3000)) {
        disturb(creature_ptr, false, true);
        msg_print(_("ウガァァア！", "RAAAAGHH!"));
        msg_print(_("激怒の発作に襲われた！", "You feel a fit of rage coming over you!"));
        (void)set_shero(creature_ptr, 10 + randint1(creature_ptr->lev), false);
        (void)set_afraid(creature_ptr, 0);
    }

    if (creature_ptr->muta.has(MUTA::COWARDICE) && (randint1(3000) == 13)) {
        if (!has_resist_fear(creature_ptr)) {
            disturb(creature_ptr, false, true);
            msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
            set_afraid(creature_ptr, creature_ptr->afraid + 13 + randint1(26));
        }
    }

    if (creature_ptr->muta.has(MUTA::RTELEPORT) && (randint1(5000) == 88)) {
        if (!has_resist_nexus(creature_ptr) && creature_ptr->muta.has_not(MUTA::VTELEPORT) && !creature_ptr->anti_tele) {
            disturb(creature_ptr, false, true);
            msg_print(_("あなたの位置は突然ひじょうに不確定になった...", "Your position suddenly seems very uncertain..."));
            msg_print(nullptr);
            teleport_player(creature_ptr, 40, TELEPORT_PASSIVE);
        }
    }

    if (creature_ptr->muta.has(MUTA::ALCOHOL) && (randint1(6400) == 321)) {
        if (!has_resist_conf(creature_ptr) && !has_resist_chaos(creature_ptr)) {
            disturb(creature_ptr, false, true);
            creature_ptr->redraw |= PR_EXTRA;
            msg_print(_("いひきがもーろーとひてきたきがふる...ヒック！", "You feel a SSSCHtupor cOmINg over yOu... *HIC*!"));
        }

        if (!has_resist_conf(creature_ptr)) {
            (void)set_confused(creature_ptr, creature_ptr->confused + randint0(20) + 15);
        }

        if (!has_resist_chaos(creature_ptr)) {
            if (one_in_(20)) {
                msg_print(nullptr);
                if (one_in_(3))
                    lose_all_info(creature_ptr);
                else
                    wiz_dark(creature_ptr);
                (void)teleport_player_aux(creature_ptr, 100, false, static_cast<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
                wiz_dark(creature_ptr);
                msg_print(_("あなたは見知らぬ場所で目が醒めた...頭が痛い。", "You wake up somewhere with a sore head..."));
                msg_print(_("何も覚えていない。どうやってここに来たかも分からない！", "You can't remember a thing or how you got here!"));
            } else {
                if (one_in_(3)) {
                    msg_print(_("き～れいなちょおちょらとんれいる～", "Thishcischs GooDSChtuff!"));
                    (void)set_image(creature_ptr, creature_ptr->image + randint0(150) + 150);
                }
            }
        }
    }

    if (creature_ptr->muta.has(MUTA::HALLU) && (randint1(6400) == 42)) {
        if (!has_resist_chaos(creature_ptr)) {
            disturb(creature_ptr, false, true);
            creature_ptr->redraw |= PR_EXTRA;
            (void)set_image(creature_ptr, creature_ptr->image + randint0(50) + 20);
        }
    }

    if (creature_ptr->muta.has(MUTA::FLATULENT) && (randint1(3000) == 13)) {
        disturb(creature_ptr, false, true);
        msg_print(_("ブゥーーッ！おっと。", "BRRAAAP! Oops."));
        msg_print(nullptr);
        fire_ball(creature_ptr, GF_POIS, 0, creature_ptr->lev, 3);
    }

    if (creature_ptr->muta.has(MUTA::PROD_MANA) && !creature_ptr->anti_magic && one_in_(9000)) {
        int dire = 0;
        disturb(creature_ptr, false, true);
        msg_print(_("魔法のエネルギーが突然あなたの中に流れ込んできた！エネルギーを解放しなければならない！",
            "Magical energy flows through you! You must release it!"));

        flush();
        msg_print(nullptr);
        (void)get_hack_dir(creature_ptr, &dire);
        fire_ball(creature_ptr, GF_MANA, dire, creature_ptr->lev * 2, 3);
    }

    if (creature_ptr->muta.has(MUTA::ATT_DEMON) && !creature_ptr->anti_magic && (randint1(6666) == 666)) {
        bool pet = one_in_(6);
        BIT_FLAGS mode = PM_ALLOW_GROUP;

        if (pet)
            mode |= PM_FORCE_PET;
        else
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

        if (summon_specific(creature_ptr, (pet ? -1 : 0), creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DEMON, mode)) {
            msg_print(_("あなたはデーモンを引き寄せた！", "You have attracted a demon!"));
            disturb(creature_ptr, false, true);
        }
    }

    if (creature_ptr->muta.has(MUTA::SPEED_FLUX) && one_in_(6000)) {
        disturb(creature_ptr, false, true);
        if (one_in_(2)) {
            msg_print(_("精力的でなくなった気がする。", "You feel less energetic."));

            if (creature_ptr->fast > 0) {
                set_fast(creature_ptr, 0, true);
            } else {
                set_slow(creature_ptr, randint1(30) + 10, false);
            }
        } else {
            msg_print(_("精力的になった気がする。", "You feel more energetic."));

            if (creature_ptr->slow > 0) {
                set_slow(creature_ptr, 0, true);
            } else {
                set_fast(creature_ptr, randint1(30) + 10, false);
            }
        }

        msg_print(nullptr);
    }

    if (creature_ptr->muta.has(MUTA::BANISH_ALL) && one_in_(9000)) {
        disturb(creature_ptr, false, true);
        msg_print(_("突然ほとんど孤独になった気がする。", "You suddenly feel almost lonely."));

        banish_monsters(creature_ptr, 100);
        if (!is_in_dungeon(creature_ptr) && creature_ptr->town_num) {
            int n;
            do {
                n = randint0(MAX_STORES);
            } while ((n == STORE_HOME) || (n == STORE_MUSEUM));

            msg_print(_("店の主人が丘に向かって走っている！", "You see one of the shopkeepers running for the hills!"));
            store_shuffle(creature_ptr, n);
        }
        msg_print(nullptr);
    }

    if (creature_ptr->muta.has(MUTA::EAT_LIGHT) && one_in_(3000)) {
        object_type *o_ptr;

        msg_print(_("影につつまれた。", "A shadow passes over you."));
        msg_print(nullptr);

        if ((creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW) {
            hp_player(creature_ptr, 10);
        }

        o_ptr = &creature_ptr->inventory_list[INVEN_LITE];

        if (o_ptr->tval == TV_LITE) {
            if (!o_ptr->is_fixed_artifact() && (o_ptr->xtra4 > 0)) {
                hp_player(creature_ptr, o_ptr->xtra4 / 20);
                o_ptr->xtra4 /= 2;
                msg_print(_("光源からエネルギーを吸収した！", "You absorb energy from your light!"));
                notice_lite_change(creature_ptr, o_ptr);
            }
        }

        /*
         * Unlite the area (radius 10) around player and
         * do 50 points damage to every affected monster
         */
        unlite_area(creature_ptr, 50, 10);
    }

    if (creature_ptr->muta.has(MUTA::ATT_ANIMAL) && !creature_ptr->anti_magic && one_in_(7000)) {
        bool pet = one_in_(3);
        BIT_FLAGS mode = PM_ALLOW_GROUP;

        if (pet)
            mode |= PM_FORCE_PET;
        else
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

        if (summon_specific(creature_ptr, (pet ? -1 : 0), creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_ANIMAL, mode)) {
            msg_print(_("動物を引き寄せた！", "You have attracted an animal!"));
            disturb(creature_ptr, false, true);
        }
    }

    if (creature_ptr->muta.has(MUTA::RAW_CHAOS) && !creature_ptr->anti_magic && one_in_(8000)) {
        disturb(creature_ptr, false, true);
        msg_print(_("周りの空間が歪んでいる気がする！", "You feel the world warping around you!"));
        msg_print(nullptr);
        fire_ball(creature_ptr, GF_CHAOS, 0, creature_ptr->lev, 8);
    }

    if (creature_ptr->muta.has(MUTA::NORMALITY) && one_in_(5000)) {
        if (!lose_mutation(creature_ptr, 0))
            msg_print(_("奇妙なくらい普通になった気がする。", "You feel oddly normal."));
    }

    if (creature_ptr->muta.has(MUTA::WRAITH) && !creature_ptr->anti_magic && one_in_(3000)) {
        disturb(creature_ptr, false, true);
        msg_print(_("非物質化した！", "You feel insubstantial!"));
        msg_print(nullptr);
        set_wraith_form(creature_ptr, randint1(creature_ptr->lev / 2) + (creature_ptr->lev / 2), false);
    }

    if (creature_ptr->muta.has(MUTA::POLY_WOUND) && one_in_(3000))
        do_poly_wounds(creature_ptr);

    if (creature_ptr->muta.has(MUTA::WASTING) && one_in_(3000)) {
        int which_stat = randint0(A_MAX);
        int sustained = false;

        switch (which_stat) {
        case A_STR:
            if (has_sustain_str(creature_ptr))
                sustained = true;
            break;
        case A_INT:
            if (has_sustain_int(creature_ptr))
                sustained = true;
            break;
        case A_WIS:
            if (has_sustain_wis(creature_ptr))
                sustained = true;
            break;
        case A_DEX:
            if (has_sustain_dex(creature_ptr))
                sustained = true;
            break;
        case A_CON:
            if (has_sustain_con(creature_ptr))
                sustained = true;
            break;
        case A_CHR:
            if (has_sustain_chr(creature_ptr))
                sustained = true;
            break;
        default:
            msg_print(_("不正な状態！", "Invalid stat chosen!"));
            sustained = true;
        }

        if (!sustained) {
            disturb(creature_ptr, false, true);
            msg_print(_("自分が衰弱していくのが分かる！", "You can feel yourself wasting away!"));
            msg_print(nullptr);
            (void)dec_stat(creature_ptr, which_stat, randint1(6) + 6, one_in_(3));
        }
    }

    if (creature_ptr->muta.has(MUTA::ATT_DRAGON) && !creature_ptr->anti_magic && one_in_(3000)) {
        bool pet = one_in_(5);
        BIT_FLAGS mode = PM_ALLOW_GROUP;
        if (pet)
            mode |= PM_FORCE_PET;
        else
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);

        if (summon_specific(creature_ptr, (pet ? -1 : 0), creature_ptr->y, creature_ptr->x, creature_ptr->current_floor_ptr->dun_level, SUMMON_DRAGON, mode)) {
            msg_print(_("ドラゴンを引き寄せた！", "You have attracted a dragon!"));
            disturb(creature_ptr, false, true);
        }
    }

    if (creature_ptr->muta.has(MUTA::WEIRD_MIND) && !creature_ptr->anti_magic && one_in_(3000)) {
        if (creature_ptr->tim_esp > 0) {
            msg_print(_("精神にもやがかかった！", "Your mind feels cloudy!"));
            set_tim_esp(creature_ptr, 0, true);
        } else {
            msg_print(_("精神が広がった！", "Your mind expands!"));
            set_tim_esp(creature_ptr, creature_ptr->lev, false);
        }
    }

    if (creature_ptr->muta.has(MUTA::NAUSEA) && !creature_ptr->slow_digest && one_in_(9000)) {
        disturb(creature_ptr, false, true);
        msg_print(_("胃が痙攣し、食事を失った！", "Your stomach roils, and you lose your lunch!"));
        msg_print(nullptr);
        set_food(creature_ptr, PY_FOOD_WEAK);
        if (music_singing_any(creature_ptr))
            stop_singing(creature_ptr);

        if (hex_spelling_any(creature_ptr))
            stop_hex_spell_all(creature_ptr);
    }

    if (creature_ptr->muta.has(MUTA::WALK_SHAD) && !creature_ptr->anti_magic && one_in_(12000) && !creature_ptr->current_floor_ptr->inside_arena)
        reserve_alter_reality(creature_ptr, randint0(21) + 15);

    if (creature_ptr->muta.has(MUTA::WARNING) && one_in_(1000)) {
        int danger_amount = 0;
        for (MONSTER_IDX monster = 0; monster < creature_ptr->current_floor_ptr->m_max; monster++) {
            monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[monster];
            monster_race *r_ptr = &r_info[m_ptr->r_idx];
            if (!monster_is_valid(m_ptr))
                continue;

            if (r_ptr->level >= creature_ptr->lev) {
                danger_amount += r_ptr->level - creature_ptr->lev + 1;
            }
        }

        if (danger_amount > 100)
            msg_print(_("非常に恐ろしい気がする！", "You feel utterly terrified!"));
        else if (danger_amount > 50)
            msg_print(_("恐ろしい気がする！", "You feel terrified!"));
        else if (danger_amount > 20)
            msg_print(_("非常に心配な気がする！", "You feel very worried!"));
        else if (danger_amount > 10)
            msg_print(_("心配な気がする！", "You feel paranoid!"));
        else if (danger_amount > 5)
            msg_print(_("ほとんど安全な気がする。", "You feel almost safe."));
        else
            msg_print(_("寂しい気がする。", "You feel lonely."));
    }

    if (creature_ptr->muta.has(MUTA::INVULN) && !creature_ptr->anti_magic && one_in_(5000)) {
        disturb(creature_ptr, false, true);
        msg_print(_("無敵な気がする！", "You feel invincible!"));
        msg_print(nullptr);
        (void)set_invuln(creature_ptr, randint1(8) + 8, false);
    }

    if (creature_ptr->muta.has(MUTA::SP_TO_HP) && one_in_(2000)) {
        MANA_POINT wounds = (MANA_POINT)(creature_ptr->mhp - creature_ptr->chp);
        if (wounds > 0) {
            HIT_POINT healing = creature_ptr->csp;
            if (healing > wounds)
                healing = wounds;

            hp_player(creature_ptr, healing);
            creature_ptr->csp -= healing;
            creature_ptr->redraw |= (PR_HP | PR_MANA);
        }
    }

    if (creature_ptr->muta.has(MUTA::HP_TO_SP) && !creature_ptr->anti_magic && one_in_(4000)) {
        HIT_POINT wounds = (HIT_POINT)(creature_ptr->msp - creature_ptr->csp);
        if (wounds > 0) {
            HIT_POINT healing = creature_ptr->chp;
            if (healing > wounds)
                healing = wounds;

            creature_ptr->csp += healing;
            creature_ptr->redraw |= (PR_HP | PR_MANA);
            take_hit(creature_ptr, DAMAGE_LOSELIFE, healing, _("頭に昇った血", "blood rushing to the head"));
        }
    }

    if (creature_ptr->muta.has(MUTA::DISARM) && one_in_(10000)) {
        disturb(creature_ptr, false, true);
        msg_print(_("足がもつれて転んだ！", "You trip over your own feet!"));
        take_hit(creature_ptr, DAMAGE_NOESCAPE, randint1(creature_ptr->wt / 6), _("転倒", "tripping"));
        drop_weapons(creature_ptr);
    }
}

bool drop_weapons(player_type *creature_ptr)
{
    INVENTORY_IDX slot = 0;
    object_type *o_ptr = nullptr;

    if (creature_ptr->wild_mode)
        return false;

    msg_print(nullptr);
    if (has_melee_weapon(creature_ptr, INVEN_MAIN_HAND)) {
        slot = INVEN_MAIN_HAND;
        o_ptr = &creature_ptr->inventory_list[INVEN_MAIN_HAND];

        if (has_melee_weapon(creature_ptr, INVEN_SUB_HAND) && one_in_(2)) {
            o_ptr = &creature_ptr->inventory_list[INVEN_SUB_HAND];
            slot = INVEN_SUB_HAND;
        }
    } else if (has_melee_weapon(creature_ptr, INVEN_SUB_HAND)) {
        o_ptr = &creature_ptr->inventory_list[INVEN_SUB_HAND];
        slot = INVEN_SUB_HAND;
    }

    if ((slot == 0) || o_ptr->is_cursed())
        return false;

    msg_print(_("武器を落としてしまった！", "You drop your weapon!"));
    drop_from_inventory(creature_ptr, slot, 1);
    return true;
}
