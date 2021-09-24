/*!
 * @brief 薬を飲んだ時の各種効果処理
 * @date 2020/07/04
 * @author Hourier
 * @todo 少し長い。switch/case文と効果処理を分離してもいいかも
 */

#include "object-use/quaff-execution.h"
#include "avatar/avatar.h"
#include "birth/birth-stat.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "game-option/birth-options.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-object.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mutation/mutation-investor-remover.h"
#include "object/object-broken.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player-base/player-race.h"
#include "player-info/mimic-info-table.h"
#include "player-info/self-info.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/eldritch-horror.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "realm/realm-hex-numbers.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/body-improvement.h"
#include "status/buff-setter.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "status/shape-changer.h"
#include "status/sight-setter.h"
#include "sv-definition/sv-potion-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief 酔っ払いの薬
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return カオス耐性があるかその他の一部確率でFALSE、それ以外はTRUE
 */
static bool booze(player_type *player_ptr)
{
    bool ident = false;
    if (player_ptr->pclass != CLASS_MONK)
        chg_virtue(player_ptr, V_HARMONY, -1);
    else if (!has_resist_conf(player_ptr))
        player_ptr->special_attack |= ATTACK_SUIKEN;

    BadStatusSetter bss(player_ptr);
    if (!has_resist_conf(player_ptr) && bss.confusion(randint0(20) + 15)) {
        ident = true;
    }

    if (has_resist_chaos(player_ptr)) {
        return ident;
    }

    if (one_in_(2) && bss.mod_hallucination(randint0(150) + 150)) {
        ident = true;
    }

    if (one_in_(13) && (player_ptr->pclass != CLASS_MONK)) {
        ident = true;
        if (one_in_(3))
            lose_all_info(player_ptr);
        else
            wiz_dark(player_ptr);

        (void)teleport_player_aux(player_ptr, 100, false, i2enum<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
        wiz_dark(player_ptr);
        msg_print(_("知らない場所で目が醒めた。頭痛がする。", "You wake up somewhere with a sore head..."));
        msg_print(_("何も思い出せない。どうやってここへ来たのかも分からない！", "You can't remember a thing or how you got here!"));
    }

    return ident;
}

/*!
 * @brief 爆発の薬の効果処理 / Fumble ramble
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 常にTRUE
 */
static bool detonation(player_type *player_ptr)
{
    msg_print(_("体の中で激しい爆発が起きた！", "Massive explosions rupture your body!"));
    take_hit(player_ptr, DAMAGE_NOESCAPE, damroll(50, 20), _("爆発の薬", "a potion of Detonation"));
    BadStatusSetter bss(player_ptr);
    (void)bss.mod_stun(75);
    (void)bss.mod_cut(5000);
    return true;
}

/*!
 * @brief 薬を飲むコマンドのサブルーチン /
 * Quaff a potion (from the pack or the floor)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item 飲む薬オブジェクトの所持品ID
 * @details
 * 効果発動のあと、食料タイプによって空腹度を少し充足する。
 */
void exe_quaff_potion(player_type *player_ptr, INVENTORY_IDX item)
{
    object_type *o_ptr;
    object_type forge;
    object_type *q_ptr;

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    if (player_ptr->timewalk) {
        if (flush_failure)
            flush();

        msg_print(_("瓶から水が流れ出てこない！", "The potion doesn't flow out from the bottle."));
        sound(SOUND_FAIL);
        return;
    }

    if (music_singing_any(player_ptr))
        stop_singing(player_ptr);

    SpellHex spell_hex(player_ptr);
    if (spell_hex.is_spelling_any() && !spell_hex.is_spelling_specific(HEX_INHALE)) {
        (void)SpellHex(player_ptr).stop_all_spells();
    }

    o_ptr = ref_item(player_ptr, item);
    q_ptr = &forge;
    q_ptr->copy_from(o_ptr);
    q_ptr->number = 1;
    vary_item(player_ptr, item, -1);
    sound(SOUND_QUAFF);
    bool ident = false;
    DEPTH lev = k_info[q_ptr->k_idx].level;
    if (q_ptr->tval == TV_POTION) {
        switch (q_ptr->sval) {
            /* 飲みごたえをオリジナルより細かく表現 */
        case SV_POTION_WATER:
            msg_print(_("口の中がさっぱりした。", "That was refreshing."));
            msg_print(_("のどの渇きが少しおさまった。", "You feel less thirsty."));
            ident = true;
            break;

        case SV_POTION_APPLE_JUICE:
            msg_print(_("甘くてサッパリとしていて、とてもおいしい。", "It's sweet, refreshing and very tasty."));
            msg_print(_("のどの渇きが少しおさまった。", "You feel less thirsty."));
            ident = true;
            break;

        case SV_POTION_SLIME_MOLD:
            msg_print(_("なんとも不気味な味だ。", "That was strange."));
            msg_print(_("のどの渇きが少しおさまった。", "You feel less thirsty."));
            ident = true;
            break;

        case SV_POTION_SLOWNESS:
            if (BadStatusSetter(player_ptr).slowness(randint1(25) + 15, false))
                ident = true;
            break;

        case SV_POTION_SALT_WATER: {
            msg_print(_("うぇ！思わず吐いてしまった。", "The potion makes you vomit!"));
            switch (player_race_food(player_ptr)) {
            case PlayerRaceFood::RATION:
            case PlayerRaceFood::WATER:
            case PlayerRaceFood::BLOOD:
                (void)set_food(player_ptr, PY_FOOD_STARVE - 1);
                break;
            default:
                break;
            }

            BadStatusSetter bss(player_ptr);
            (void)bss.poison(0);
            (void)bss.mod_paralysis(4);
            ident = true;
            break;
        }
        case SV_POTION_POISON:
            if (!(has_resist_pois(player_ptr) || is_oppose_pois(player_ptr))) {
                if (BadStatusSetter(player_ptr).mod_poison(randint0(15) + 10)) {
                    ident = true;
                }
            }
            break;

        case SV_POTION_BLINDNESS:
            if (!has_resist_blind(player_ptr)) {
                if (BadStatusSetter(player_ptr).mod_blindness(randint0(100) + 100)) {
                    ident = true;
                }
            }
            break;

        case SV_POTION_BOOZE:
            ident = booze(player_ptr);
            break;

        case SV_POTION_SLEEP:
            if (player_ptr->free_act) {
                break;
            }

            msg_print(_("あなたは眠ってしまった。", "You fall asleep."));
            if (ironman_nightmare) {
                msg_print(_("恐ろしい光景が頭に浮かんできた。", "A horrible vision enters your mind."));
                sanity_blast(player_ptr, nullptr, false);
            }

            if (BadStatusSetter(player_ptr).mod_paralysis(randint0(4) + 4)) {
                ident = true;
            }

            break;
        case SV_POTION_LOSE_MEMORIES:
            if (!player_ptr->hold_exp && (player_ptr->exp > 0)) {
                msg_print(_("過去の記憶が薄れていく気がする。", "You feel your memories fade."));
                chg_virtue(player_ptr, V_KNOWLEDGE, -5);

                lose_exp(player_ptr, player_ptr->exp / 4);
                ident = true;
            }
            break;

        case SV_POTION_RUINATION:
            msg_print(_("身も心も弱ってきて、精気が抜けていくようだ。", "Your nerves and muscles feel weak and lifeless!"));
            take_hit(player_ptr, DAMAGE_LOSELIFE, damroll(10, 10), _("破滅の薬", "a potion of Ruination"));

            (void)dec_stat(player_ptr, A_DEX, 25, true);
            (void)dec_stat(player_ptr, A_WIS, 25, true);
            (void)dec_stat(player_ptr, A_CON, 25, true);
            (void)dec_stat(player_ptr, A_STR, 25, true);
            (void)dec_stat(player_ptr, A_CHR, 25, true);
            (void)dec_stat(player_ptr, A_INT, 25, true);
            ident = true;
            break;

        case SV_POTION_DEC_STR:
            if (do_dec_stat(player_ptr, A_STR))
                ident = true;
            break;

        case SV_POTION_DEC_INT:
            if (do_dec_stat(player_ptr, A_INT))
                ident = true;
            break;

        case SV_POTION_DEC_WIS:
            if (do_dec_stat(player_ptr, A_WIS))
                ident = true;
            break;

        case SV_POTION_DEC_DEX:
            if (do_dec_stat(player_ptr, A_DEX))
                ident = true;
            break;

        case SV_POTION_DEC_CON:
            if (do_dec_stat(player_ptr, A_CON))
                ident = true;
            break;

        case SV_POTION_DEC_CHR:
            if (do_dec_stat(player_ptr, A_CHR))
                ident = true;
            break;

        case SV_POTION_DETONATIONS:
            ident = detonation(player_ptr);
            break;

        case SV_POTION_DEATH:
            chg_virtue(player_ptr, V_VITALITY, -1);
            chg_virtue(player_ptr, V_UNLIFE, 5);
            msg_print(_("死の予感が体中を駆けめぐった。", "A feeling of Death flows through your body."));
            take_hit(player_ptr, DAMAGE_LOSELIFE, 5000, _("死の薬", "a potion of Death"));
            ident = true;
            break;

        case SV_POTION_INFRAVISION:
            if (set_tim_infra(player_ptr, player_ptr->tim_infra + 100 + randint1(100), false)) {
                ident = true;
            }
            break;

        case SV_POTION_DETECT_INVIS:
            if (set_tim_invis(player_ptr, player_ptr->tim_invis + 12 + randint1(12), false)) {
                ident = true;
            }
            break;

        case SV_POTION_SLOW_POISON:
            if (BadStatusSetter(player_ptr).poison(player_ptr->poisoned / 2))
                ident = true;
            break;

        case SV_POTION_CURE_POISON:
            if (BadStatusSetter(player_ptr).poison(0))
                ident = true;
            break;

        case SV_POTION_BOLDNESS:
            if (BadStatusSetter(player_ptr).afraidness(0))
                ident = true;
            break;

        case SV_POTION_SPEED:
            if (!player_ptr->fast) {
                if (set_fast(player_ptr, randint1(25) + 15, false))
                    ident = true;
            } else {
                (void)set_fast(player_ptr, player_ptr->fast + 5, false);
            }
            break;

        case SV_POTION_RESIST_HEAT:
            if (set_oppose_fire(player_ptr, player_ptr->oppose_fire + randint1(10) + 10, false)) {
                ident = true;
            }
            break;

        case SV_POTION_RESIST_COLD:
            if (set_oppose_cold(player_ptr, player_ptr->oppose_cold + randint1(10) + 10, false)) {
                ident = true;
            }
            break;

        case SV_POTION_HEROISM:
            ident = heroism(player_ptr, 25);
            break;

        case SV_POTION_BESERK_STRENGTH:
            ident = berserk(player_ptr, randint1(25) + 25);
            break;

        case SV_POTION_CURE_LIGHT:
            ident = cure_light_wounds(player_ptr, 2, 8);
            break;

        case SV_POTION_CURE_SERIOUS:
            ident = cure_serious_wounds(player_ptr, 4, 8);
            break;

        case SV_POTION_CURE_CRITICAL:
            ident = cure_critical_wounds(player_ptr, damroll(6, 8));
            break;

        case SV_POTION_HEALING:
            ident = cure_critical_wounds(player_ptr, 300);
            break;

        case SV_POTION_STAR_HEALING:
            ident = cure_critical_wounds(player_ptr, 1200);
            break;

        case SV_POTION_LIFE:
            ident = life_stream(player_ptr, true, true);
            break;

        case SV_POTION_RESTORE_MANA:
            ident = restore_mana(player_ptr, true);
            break;

        case SV_POTION_RESTORE_EXP:
            if (restore_level(player_ptr))
                ident = true;
            break;

        case SV_POTION_RES_STR:
            if (do_res_stat(player_ptr, A_STR))
                ident = true;
            break;

        case SV_POTION_RES_INT:
            if (do_res_stat(player_ptr, A_INT))
                ident = true;
            break;

        case SV_POTION_RES_WIS:
            if (do_res_stat(player_ptr, A_WIS))
                ident = true;
            break;

        case SV_POTION_RES_DEX:
            if (do_res_stat(player_ptr, A_DEX))
                ident = true;
            break;

        case SV_POTION_RES_CON:
            if (do_res_stat(player_ptr, A_CON))
                ident = true;
            break;

        case SV_POTION_RES_CHR:
            if (do_res_stat(player_ptr, A_CHR))
                ident = true;
            break;

        case SV_POTION_INC_STR:
            if (do_inc_stat(player_ptr, A_STR))
                ident = true;
            break;

        case SV_POTION_INC_INT:
            if (do_inc_stat(player_ptr, A_INT))
                ident = true;
            break;

        case SV_POTION_INC_WIS:
            if (do_inc_stat(player_ptr, A_WIS))
                ident = true;
            break;

        case SV_POTION_INC_DEX:
            if (do_inc_stat(player_ptr, A_DEX))
                ident = true;
            break;

        case SV_POTION_INC_CON:
            if (do_inc_stat(player_ptr, A_CON))
                ident = true;
            break;

        case SV_POTION_INC_CHR:
            if (do_inc_stat(player_ptr, A_CHR))
                ident = true;
            break;

        case SV_POTION_POLY_SELF:
            do_poly_self(player_ptr);
            ident = true;
            break;

        case SV_POTION_AUGMENTATION:
            if (do_inc_stat(player_ptr, A_STR))
                ident = true;
            if (do_inc_stat(player_ptr, A_INT))
                ident = true;
            if (do_inc_stat(player_ptr, A_WIS))
                ident = true;
            if (do_inc_stat(player_ptr, A_DEX))
                ident = true;
            if (do_inc_stat(player_ptr, A_CON))
                ident = true;
            if (do_inc_stat(player_ptr, A_CHR))
                ident = true;
            break;

        case SV_POTION_ENLIGHTENMENT:
            msg_print(_("自分の置かれている状況が脳裏に浮かんできた...", "An image of your surroundings forms in your mind..."));
            chg_virtue(player_ptr, V_KNOWLEDGE, 1);
            chg_virtue(player_ptr, V_ENLIGHTEN, 1);
            wiz_lite(player_ptr, false);
            ident = true;
            break;

        case SV_POTION_STAR_ENLIGHTENMENT:
            msg_print(_("更なる啓蒙を感じた...", "You begin to feel more enlightened..."));
            chg_virtue(player_ptr, V_KNOWLEDGE, 1);
            chg_virtue(player_ptr, V_ENLIGHTEN, 2);
            msg_print(nullptr);
            wiz_lite(player_ptr, false);
            (void)do_inc_stat(player_ptr, A_INT);
            (void)do_inc_stat(player_ptr, A_WIS);
            (void)detect_traps(player_ptr, DETECT_RAD_DEFAULT, true);
            (void)detect_doors(player_ptr, DETECT_RAD_DEFAULT);
            (void)detect_stairs(player_ptr, DETECT_RAD_DEFAULT);
            (void)detect_treasure(player_ptr, DETECT_RAD_DEFAULT);
            (void)detect_objects_gold(player_ptr, DETECT_RAD_DEFAULT);
            (void)detect_objects_normal(player_ptr, DETECT_RAD_DEFAULT);
            identify_pack(player_ptr);
            self_knowledge(player_ptr);
            ident = true;
            break;

        case SV_POTION_SELF_KNOWLEDGE:
            msg_print(_("自分自身のことが少しは分かった気がする...", "You begin to know yourself a little better..."));
            msg_print(nullptr);
            self_knowledge(player_ptr);
            ident = true;
            break;

        case SV_POTION_EXPERIENCE:
            if (player_ptr->prace == player_race_type::ANDROID)
                break;
            chg_virtue(player_ptr, V_ENLIGHTEN, 1);
            if (player_ptr->exp < PY_MAX_EXP) {
                EXP ee = (player_ptr->exp / 2) + 10;
                if (ee > 100000L)
                    ee = 100000L;
                msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
                gain_exp(player_ptr, ee);
                ident = true;
            }
            break;

        case SV_POTION_RESISTANCE:
            (void)set_oppose_acid(player_ptr, player_ptr->oppose_acid + randint1(20) + 20, false);
            (void)set_oppose_elec(player_ptr, player_ptr->oppose_elec + randint1(20) + 20, false);
            (void)set_oppose_fire(player_ptr, player_ptr->oppose_fire + randint1(20) + 20, false);
            (void)set_oppose_cold(player_ptr, player_ptr->oppose_cold + randint1(20) + 20, false);
            (void)set_oppose_pois(player_ptr, player_ptr->oppose_pois + randint1(20) + 20, false);
            ident = true;
            break;

        case SV_POTION_CURING:
            if (true_healing(player_ptr, 50))
                ident = true;
            break;

        case SV_POTION_INVULNERABILITY:
            (void)set_invuln(player_ptr, player_ptr->invuln + randint1(4) + 4, false);
            ident = true;
            break;

        case SV_POTION_NEW_LIFE:
            roll_hitdice(player_ptr, SPOP_NONE);
            get_max_stats(player_ptr);
            player_ptr->update |= PU_BONUS;
            lose_all_mutations(player_ptr);
            ident = true;
            break;

        case SV_POTION_NEO_TSUYOSHI:
            (void)BadStatusSetter(player_ptr).hallucination(0);
            (void)set_tsuyoshi(player_ptr, player_ptr->tsuyoshi + randint1(100) + 100, false);
            ident = true;
            break;

        case SV_POTION_TSUYOSHI:
            msg_print(_("「オクレ兄さん！」", "Brother OKURE!"));
            msg_print(nullptr);
            player_ptr->tsuyoshi = 1;
            (void)set_tsuyoshi(player_ptr, 0, true);
            if (!has_resist_chaos(player_ptr)) {
                (void)BadStatusSetter(player_ptr).hallucination(50 + randint1(50));
            }
            ident = true;
            break;

        case SV_POTION_POLYMORPH:
            if (player_ptr->muta.any() && one_in_(23)) {
                lose_all_mutations(player_ptr);
            } else {
                do {
                    if (one_in_(2)) {
                        if (gain_mutation(player_ptr, 0))
                            ident = true;
                    } else if (lose_mutation(player_ptr, 0))
                        ident = true;
                } while (!ident || one_in_(2));
            }
            break;
        }
    }

    if (PlayerRace(player_ptr).equals(player_race_type::SKELETON)) {
        msg_print(_("液体の一部はあなたのアゴを素通りして落ちた！", "Some of the fluid falls through your jaws!"));
        (void)potion_smash_effect(player_ptr, 0, player_ptr->y, player_ptr->x, q_ptr->k_idx);
    }
    player_ptr->update |= (PU_COMBINE | PU_REORDER);

    if (!(q_ptr->is_aware())) {
        chg_virtue(player_ptr, V_PATIENCE, -1);
        chg_virtue(player_ptr, V_CHANCE, 1);
        chg_virtue(player_ptr, V_KNOWLEDGE, -1);
    }

    /* The item has been tried */
    object_tried(q_ptr);

    /* An identification was made */
    if (ident && !q_ptr->is_aware()) {
        object_aware(player_ptr, q_ptr);
        gain_exp(player_ptr, (lev + (player_ptr->lev >> 1)) / player_ptr->lev);
    }

    player_ptr->window_flags |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

    if (PlayerRace(player_ptr).equals(player_race_type::SKELETON))
        return; //!< @note スケルトンは水分で飢えを満たせない

    switch (player_race_food(player_ptr)) {
    case PlayerRaceFood::WATER:
        msg_print(_("水分を取り込んだ。", "You are moistened."));
        set_food(player_ptr, MIN(player_ptr->food + q_ptr->pval + MAX(0, q_ptr->pval * 10) + 2000, PY_FOOD_MAX - 1));
        break;
    case PlayerRaceFood::OIL:
        if (q_ptr->tval == TV_FLASK) {
            msg_print(_("オイルを補給した。", "You replenish yourself with the oil."));
            set_food(player_ptr, player_ptr->food + 5000);
        } else {
            set_food(player_ptr, player_ptr->food + ((q_ptr->pval) / 20));
        }
        break;
    case PlayerRaceFood::BLOOD:
        (void)set_food(player_ptr, player_ptr->food + (q_ptr->pval / 10));
        break;
    case PlayerRaceFood::MANA:
    case PlayerRaceFood::CORPSE:
        set_food(player_ptr, player_ptr->food + ((q_ptr->pval) / 20));
        break;
    default:
        (void)set_food(player_ptr, player_ptr->food + q_ptr->pval);
        break;
    }
}
