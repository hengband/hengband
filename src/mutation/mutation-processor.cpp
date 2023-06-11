#include "mutation/mutation-processor.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "effect/attribute-types.h"
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
#include "object/tval-types.h"
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
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "timed-effect/player-acceleration.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/player-deceleration.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

static bool get_hack_dir(PlayerType *player_ptr, DIRECTION *dp)
{
    *dp = 0;
    char command;
    DIRECTION dir = 0;
    while (!dir) {
        concptr p = target_okay(player_ptr)
                        ? _("方向 ('5'でターゲットへ, '*'でターゲット再選択, ESCで中断)? ", "Direction ('5' for target, '*' to re-target, Escape to cancel)? ")
                        : _("方向 ('*'でターゲット選択, ESCで中断)? ", "Direction ('*' to choose a target, Escape to cancel)? ");
        if (!get_com(p, &command, true)) {
            break;
        }

        if (use_menu && (command == '\r')) {
            command = 't';
        }

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
            if (target_set(player_ptr, TARGET_KILL)) {
                dir = 5;
            }

            break;
        default:
            dir = get_keymap_dir(command);
            break;
        }

        if ((dir == 5) && !target_okay(player_ptr)) {
            dir = 0;
        }

        if (!dir) {
            bell();
        }
    }

    if (!dir) {
        return false;
    }

    command_dir = dir;
    if (player_ptr->effects()->confusion()->is_confused()) {
        dir = ddd[randint0(8)];
    }

    if (command_dir != dir) {
        msg_print(_("あなたは混乱している。", "You are confused."));
    }

    *dp = dir;
    return true;
}

/*!
 * @brief 10ゲームターンが進行するごとに突然変異の発動判定を行う処理
 * / Handle mutation effects once every 10 game turns
 */
void process_world_aux_mutation(PlayerType *player_ptr)
{
    if (player_ptr->muta.none() || player_ptr->phase_out || player_ptr->wild_mode) {
        return;
    }

    BadStatusSetter bss(player_ptr);
    if (player_ptr->muta.has(PlayerMutationType::BERS_RAGE) && one_in_(3000)) {
        disturb(player_ptr, false, true);
        msg_print(_("ウガァァア！", "RAAAAGHH!"));
        msg_print(_("激怒の発作に襲われた！", "You feel a fit of rage coming over you!"));
        (void)set_shero(player_ptr, 10 + randint1(player_ptr->lev), false);
        (void)bss.set_fear(0);
    }

    if (player_ptr->muta.has(PlayerMutationType::COWARDICE) && (randint1(3000) == 13)) {
        if (!has_resist_fear(player_ptr)) {
            disturb(player_ptr, false, true);
            msg_print(_("とても暗い... とても恐い！", "It's so dark... so scary!"));
            (void)bss.mod_fear(13 + randint1(26));
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::RTELEPORT) && (randint1(5000) == 88)) {
        if (!has_resist_nexus(player_ptr) && player_ptr->muta.has_not(PlayerMutationType::VTELEPORT) && !player_ptr->anti_tele) {
            disturb(player_ptr, false, true);
            msg_print(_("あなたの位置は突然ひじょうに不確定になった...", "Your position suddenly seems very uncertain..."));
            msg_print(nullptr);
            teleport_player(player_ptr, 40, TELEPORT_PASSIVE);
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::ALCOHOL) && (randint1(6400) == 321)) {
        if (!has_resist_conf(player_ptr) && !has_resist_chaos(player_ptr)) {
            disturb(player_ptr, false, true);
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::EXTRA);
            msg_print(_("いひきがもーろーとひてきたきがふる...ヒック！", "You feel a SSSCHtupor cOmINg over yOu... *HIC*!"));
        }

        if (!has_resist_conf(player_ptr)) {
            (void)bss.mod_confusion(randint0(20) + 15);
        }

        if (!has_resist_chaos(player_ptr)) {
            if (one_in_(20)) {
                msg_print(nullptr);
                if (one_in_(3)) {
                    lose_all_info(player_ptr);
                } else {
                    wiz_dark(player_ptr);
                }
                (void)teleport_player_aux(player_ptr, 100, false, i2enum<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
                wiz_dark(player_ptr);
                msg_print(_("あなたは見知らぬ場所で目が醒めた...頭が痛い。", "You wake up somewhere with a sore head..."));
                msg_print(_("何も覚えていない。どうやってここに来たかも分からない！", "You can't remember a thing or how you got here!"));
            } else {
                if (one_in_(3)) {
                    msg_print(_("き～れいなちょおちょらとんれいる～", "Thishcischs GooDSChtuff!"));
                    (void)bss.mod_hallucination(randint0(150) + 150);
                }
            }
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::HALLU) && (randint1(6400) == 42)) {
        if (!has_resist_chaos(player_ptr)) {
            disturb(player_ptr, false, true);
            RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::EXTRA);
            (void)bss.mod_hallucination(randint0(50) + 20);
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::FLATULENT) && (randint1(3000) == 13)) {
        disturb(player_ptr, false, true);
        msg_print(_("ブゥーーッ！おっと。", "BRRAAAP! Oops."));
        msg_print(nullptr);
        fire_ball(player_ptr, AttributeType::POIS, 0, player_ptr->lev, 3);
    }

    if (player_ptr->muta.has(PlayerMutationType::PROD_MANA) && !player_ptr->anti_magic && one_in_(9000)) {
        int dire = 0;
        disturb(player_ptr, false, true);
        msg_print(_("魔法のエネルギーが突然あなたの中に流れ込んできた！エネルギーを解放しなければならない！",
            "Magical energy flows through you! You must release it!"));

        flush();
        msg_print(nullptr);
        (void)get_hack_dir(player_ptr, &dire);
        fire_ball(player_ptr, AttributeType::MANA, dire, player_ptr->lev * 2, 3);
    }

    if (player_ptr->muta.has(PlayerMutationType::ATT_DEMON) && !player_ptr->anti_magic && (randint1(6666) == 666)) {
        bool pet = one_in_(6);
        BIT_FLAGS mode = PM_ALLOW_GROUP;

        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(player_ptr, (pet ? -1 : 0), player_ptr->y, player_ptr->x, player_ptr->current_floor_ptr->dun_level, SUMMON_DEMON, mode)) {
            msg_print(_("あなたはデーモンを引き寄せた！", "You have attracted a demon!"));
            disturb(player_ptr, false, true);
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::SPEED_FLUX) && one_in_(6000)) {
        disturb(player_ptr, false, true);
        if (one_in_(2)) {
            msg_print(_("精力的でなくなった気がする。", "You feel less energetic."));
            if (player_ptr->effects()->acceleration()->is_fast()) {
                set_acceleration(player_ptr, 0, true);
            } else {
                (void)bss.set_deceleration(randint1(30) + 10, false);
            }
        } else {
            msg_print(_("精力的になった気がする。", "You feel more energetic."));
            if (player_ptr->effects()->deceleration()->is_slow()) {
                (void)bss.set_deceleration(0, true);
            } else {
                set_acceleration(player_ptr, randint1(30) + 10, false);
            }
        }

        msg_print(nullptr);
    }

    if (player_ptr->muta.has(PlayerMutationType::BANISH_ALL) && one_in_(9000)) {
        disturb(player_ptr, false, true);
        msg_print(_("突然ほとんど孤独になった気がする。", "You suddenly feel almost lonely."));

        banish_monsters(player_ptr, 100);
        if (!player_ptr->current_floor_ptr->is_in_dungeon() && player_ptr->town_num) {
            StoreSaleType sst;
            do {
                sst = i2enum<StoreSaleType>(randint0(MAX_STORES));
            } while ((sst == StoreSaleType::HOME) || (sst == StoreSaleType::MUSEUM));

            msg_print(_("店の主人が丘に向かって走っている！", "You see one of the shopkeepers running for the hills!"));
            store_shuffle(player_ptr, sst);
        }
        msg_print(nullptr);
    }

    if (player_ptr->muta.has(PlayerMutationType::EAT_LIGHT) && one_in_(3000)) {
        msg_print(_("影につつまれた。", "A shadow passes over you."));
        msg_print(nullptr);

        if ((player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW) {
            hp_player(player_ptr, 10);
        }

        auto &item = player_ptr->inventory_list[INVEN_LITE];
        if (item.bi_key.tval() == ItemKindType::LITE) {
            if (!item.is_fixed_artifact() && (item.fuel > 0)) {
                hp_player(player_ptr, item.fuel / 20);
                item.fuel /= 2;
                msg_print(_("光源からエネルギーを吸収した！", "You absorb energy from your light!"));
                notice_lite_change(player_ptr, &item);
            }
        }

        /*
         * Unlite the area (radius 10) around player and
         * do 50 points damage to every affected monster
         */
        unlite_area(player_ptr, 50, 10);
    }

    if (player_ptr->muta.has(PlayerMutationType::ATT_ANIMAL) && !player_ptr->anti_magic && one_in_(7000)) {
        bool pet = one_in_(3);
        BIT_FLAGS mode = PM_ALLOW_GROUP;

        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(player_ptr, (pet ? -1 : 0), player_ptr->y, player_ptr->x, player_ptr->current_floor_ptr->dun_level, SUMMON_ANIMAL, mode)) {
            msg_print(_("動物を引き寄せた！", "You have attracted an animal!"));
            disturb(player_ptr, false, true);
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::RAW_CHAOS) && !player_ptr->anti_magic && one_in_(8000)) {
        disturb(player_ptr, false, true);
        msg_print(_("周りの空間が歪んでいる気がする！", "You feel the world warping around you!"));
        msg_print(nullptr);
        fire_ball(player_ptr, AttributeType::CHAOS, 0, player_ptr->lev, 8);
    }

    if (player_ptr->muta.has(PlayerMutationType::NORMALITY) && one_in_(5000)) {
        if (!lose_mutation(player_ptr, 0)) {
            msg_print(_("奇妙なくらい普通になった気がする。", "You feel oddly normal."));
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::WRAITH) && !player_ptr->anti_magic && one_in_(3000)) {
        disturb(player_ptr, false, true);
        msg_print(_("非物質化した！", "You feel insubstantial!"));
        msg_print(nullptr);
        set_wraith_form(player_ptr, randint1(player_ptr->lev / 2) + (player_ptr->lev / 2), false);
    }

    if (player_ptr->muta.has(PlayerMutationType::POLY_WOUND) && one_in_(3000)) {
        do_poly_wounds(player_ptr);
    }

    if (player_ptr->muta.has(PlayerMutationType::WASTING) && one_in_(3000)) {
        int which_stat = randint0(A_MAX);
        int sustained = false;

        switch (which_stat) {
        case A_STR:
            if (has_sustain_str(player_ptr)) {
                sustained = true;
            }
            break;
        case A_INT:
            if (has_sustain_int(player_ptr)) {
                sustained = true;
            }
            break;
        case A_WIS:
            if (has_sustain_wis(player_ptr)) {
                sustained = true;
            }
            break;
        case A_DEX:
            if (has_sustain_dex(player_ptr)) {
                sustained = true;
            }
            break;
        case A_CON:
            if (has_sustain_con(player_ptr)) {
                sustained = true;
            }
            break;
        case A_CHR:
            if (has_sustain_chr(player_ptr)) {
                sustained = true;
            }
            break;
        default:
            msg_print(_("不正な状態！", "Invalid stat chosen!"));
            sustained = true;
        }

        if (!sustained) {
            disturb(player_ptr, false, true);
            msg_print(_("自分が衰弱していくのが分かる！", "You can feel yourself wasting away!"));
            msg_print(nullptr);
            (void)dec_stat(player_ptr, which_stat, randint1(6) + 6, one_in_(3));
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::ATT_DRAGON) && !player_ptr->anti_magic && one_in_(3000)) {
        bool pet = one_in_(5);
        BIT_FLAGS mode = PM_ALLOW_GROUP;
        if (pet) {
            mode |= PM_FORCE_PET;
        } else {
            mode |= (PM_ALLOW_UNIQUE | PM_NO_PET);
        }

        if (summon_specific(player_ptr, (pet ? -1 : 0), player_ptr->y, player_ptr->x, player_ptr->current_floor_ptr->dun_level, SUMMON_DRAGON, mode)) {
            msg_print(_("ドラゴンを引き寄せた！", "You have attracted a dragon!"));
            disturb(player_ptr, false, true);
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::WEIRD_MIND) && !player_ptr->anti_magic && one_in_(3000)) {
        if (player_ptr->tim_esp > 0) {
            msg_print(_("精神にもやがかかった！", "Your mind feels cloudy!"));
            set_tim_esp(player_ptr, 0, true);
        } else {
            msg_print(_("精神が広がった！", "Your mind expands!"));
            set_tim_esp(player_ptr, player_ptr->lev, false);
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::NAUSEA) && !player_ptr->slow_digest && one_in_(9000)) {
        disturb(player_ptr, false, true);
        msg_print(_("胃が痙攣し、食事を失った！", "Your stomach roils, and you lose your lunch!"));
        msg_print(nullptr);
        set_food(player_ptr, PY_FOOD_WEAK);
        if (music_singing_any(player_ptr)) {
            stop_singing(player_ptr);
        }

        SpellHex spell_hex(player_ptr);
        if (spell_hex.is_spelling_any()) {
            (void)spell_hex.stop_all_spells();
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::WALK_SHAD) && !player_ptr->anti_magic && one_in_(12000) && !player_ptr->current_floor_ptr->inside_arena) {
        reserve_alter_reality(player_ptr, randint0(21) + 15);
    }

    if (player_ptr->muta.has(PlayerMutationType::WARNING) && one_in_(1000)) {
        int danger_amount = 0;
        for (MONSTER_IDX monster = 0; monster < player_ptr->current_floor_ptr->m_max; monster++) {
            auto *m_ptr = &player_ptr->current_floor_ptr->m_list[monster];
            auto *r_ptr = &monraces_info[m_ptr->r_idx];
            if (!m_ptr->is_valid()) {
                continue;
            }

            if (r_ptr->level >= player_ptr->lev) {
                danger_amount += r_ptr->level - player_ptr->lev + 1;
            }
        }

        if (danger_amount > 100) {
            msg_print(_("非常に恐ろしい気がする！", "You feel utterly terrified!"));
        } else if (danger_amount > 50) {
            msg_print(_("恐ろしい気がする！", "You feel terrified!"));
        } else if (danger_amount > 20) {
            msg_print(_("非常に心配な気がする！", "You feel very worried!"));
        } else if (danger_amount > 10) {
            msg_print(_("心配な気がする！", "You feel paranoid!"));
        } else if (danger_amount > 5) {
            msg_print(_("ほとんど安全な気がする。", "You feel almost safe."));
        } else {
            msg_print(_("寂しい気がする。", "You feel lonely."));
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::INVULN) && !player_ptr->anti_magic && one_in_(5000)) {
        disturb(player_ptr, false, true);
        msg_print(_("無敵な気がする！", "You feel invincible!"));
        msg_print(nullptr);
        (void)set_invuln(player_ptr, randint1(8) + 8, false);
    }

    static constexpr auto flags = {
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
    };
    if (player_ptr->muta.has(PlayerMutationType::SP_TO_HP) && one_in_(2000)) {
        MANA_POINT wounds = (MANA_POINT)(player_ptr->mhp - player_ptr->chp);
        if (wounds > 0) {
            int healing = player_ptr->csp;
            if (healing > wounds) {
                healing = wounds;
            }

            hp_player(player_ptr, healing);
            player_ptr->csp -= healing;
            RedrawingFlagsUpdater::get_instance().set_flags(flags);
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::HP_TO_SP) && !player_ptr->anti_magic && one_in_(4000)) {
        int wounds = (int)(player_ptr->msp - player_ptr->csp);
        if (wounds > 0) {
            int healing = player_ptr->chp;
            if (healing > wounds) {
                healing = wounds;
            }

            player_ptr->csp += healing;
            RedrawingFlagsUpdater::get_instance().set_flags(flags);
            take_hit(player_ptr, DAMAGE_LOSELIFE, healing, _("頭に昇った血", "blood rushing to the head"));
        }
    }

    if (player_ptr->muta.has(PlayerMutationType::DISARM) && one_in_(10000)) {
        disturb(player_ptr, false, true);
        msg_print(_("足がもつれて転んだ！", "You trip over your own feet!"));
        take_hit(player_ptr, DAMAGE_NOESCAPE, randint1(player_ptr->wt / 6), _("転倒", "tripping"));
        drop_weapons(player_ptr);
    }
}

bool drop_weapons(PlayerType *player_ptr)
{
    INVENTORY_IDX slot = 0;
    ItemEntity *o_ptr = nullptr;

    if (player_ptr->wild_mode) {
        return false;
    }

    msg_print(nullptr);
    if (has_melee_weapon(player_ptr, INVEN_MAIN_HAND)) {
        slot = INVEN_MAIN_HAND;
        o_ptr = &player_ptr->inventory_list[INVEN_MAIN_HAND];

        if (has_melee_weapon(player_ptr, INVEN_SUB_HAND) && one_in_(2)) {
            o_ptr = &player_ptr->inventory_list[INVEN_SUB_HAND];
            slot = INVEN_SUB_HAND;
        }
    } else if (has_melee_weapon(player_ptr, INVEN_SUB_HAND)) {
        o_ptr = &player_ptr->inventory_list[INVEN_SUB_HAND];
        slot = INVEN_SUB_HAND;
    }

    if ((slot == 0) || o_ptr->is_cursed()) {
        return false;
    }

    msg_print(_("武器を落としてしまった！", "You drop your weapon!"));
    drop_from_inventory(player_ptr, slot, 1);
    return true;
}
