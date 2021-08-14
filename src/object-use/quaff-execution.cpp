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
#include "player-info/self-info.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/eldritch-horror.h"
#include "player/mimic-info-table.h"
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
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return カオス耐性があるかその他の一部確率でFALSE、それ以外はTRUE
 */
static bool booze(player_type *creature_ptr)
{
    bool ident = false;
    if (creature_ptr->pclass != CLASS_MONK)
        chg_virtue(creature_ptr, V_HARMONY, -1);
    else if (!has_resist_conf(creature_ptr))
        creature_ptr->special_attack |= ATTACK_SUIKEN;

    if (!has_resist_conf(creature_ptr) && set_confused(creature_ptr, randint0(20) + 15)) {
        ident = true;
    }

    if (has_resist_chaos(creature_ptr)) {
        return ident;
    }

    if (one_in_(2) && set_image(creature_ptr, creature_ptr->image + randint0(150) + 150)) {
        ident = true;
    }

    if (one_in_(13) && (creature_ptr->pclass != CLASS_MONK)) {
        ident = true;
        if (one_in_(3))
            lose_all_info(creature_ptr);
        else
            wiz_dark(creature_ptr);

        (void)teleport_player_aux(creature_ptr, 100, false, static_cast<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
        wiz_dark(creature_ptr);
        msg_print(_("知らない場所で目が醒めた。頭痛がする。", "You wake up somewhere with a sore head..."));
        msg_print(_("何も思い出せない。どうやってここへ来たのかも分からない！", "You can't remember a thing or how you got here!"));
    }

    return ident;
}

/*!
 * @brief 爆発の薬の効果処理 / Fumble ramble
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 常にTRUE
 */
static bool detonation(player_type *creature_ptr)
{
    msg_print(_("体の中で激しい爆発が起きた！", "Massive explosions rupture your body!"));
    take_hit(creature_ptr, DAMAGE_NOESCAPE, damroll(50, 20), _("爆発の薬", "a potion of Detonation"));
    (void)set_stun(creature_ptr, creature_ptr->stun + 75);
    (void)set_cut(creature_ptr, creature_ptr->cut + 5000);
    return true;
}

/*!
 * @brief 薬を飲むコマンドのサブルーチン /
 * Quaff a potion (from the pack or the floor)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param item 飲む薬オブジェクトの所持品ID
 * @details
 * 効果発動のあと、食料タイプによって空腹度を少し充足する。
 */
void exe_quaff_potion(player_type *creature_ptr, INVENTORY_IDX item)
{
    object_type *o_ptr;
    object_type forge;
    object_type *q_ptr;

    PlayerEnergy(creature_ptr).set_player_turn_energy(100);

    if (creature_ptr->timewalk) {
        if (flush_failure)
            flush();

        msg_print(_("瓶から水が流れ出てこない！", "The potion doesn't flow out from the bottle."));
        sound(SOUND_FAIL);
        return;
    }

    if (music_singing_any(creature_ptr))
        stop_singing(creature_ptr);

    if (hex_spelling_any(creature_ptr) && !hex_spelling(creature_ptr, HEX_INHAIL))
        stop_hex_spell_all(creature_ptr);

    o_ptr = ref_item(creature_ptr, item);
    q_ptr = &forge;
    q_ptr->copy_from(o_ptr);
    q_ptr->number = 1;
    vary_item(creature_ptr, item, -1);
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
            if (set_slow(creature_ptr, randint1(25) + 15, false))
                ident = true;
            break;

        case SV_POTION_SALT_WATER:
            msg_print(_("うぇ！思わず吐いてしまった。", "The potion makes you vomit!"));

            switch (player_race_food(creature_ptr)) {
            case PlayerRaceFood::RATION:
            case PlayerRaceFood::WATER:
            case PlayerRaceFood::BLOOD:
                (void)set_food(creature_ptr, PY_FOOD_STARVE - 1);
                break;
            default:
                break;
            }

            (void)set_poisoned(creature_ptr, 0);
            (void)set_paralyzed(creature_ptr, creature_ptr->paralyzed + 4);
            ident = true;
            break;

        case SV_POTION_POISON:
            if (!(has_resist_pois(creature_ptr) || is_oppose_pois(creature_ptr))) {
                if (set_poisoned(creature_ptr, creature_ptr->poisoned + randint0(15) + 10)) {
                    ident = true;
                }
            }
            break;

        case SV_POTION_BLINDNESS:
            if (!has_resist_blind(creature_ptr)) {
                if (set_blind(creature_ptr, creature_ptr->blind + randint0(100) + 100)) {
                    ident = true;
                }
            }
            break;

        case SV_POTION_BOOZE:
            ident = booze(creature_ptr);
            break;

        case SV_POTION_SLEEP:
            if (!creature_ptr->free_act) {
                msg_print(_("あなたは眠ってしまった。", "You fall asleep."));

                if (ironman_nightmare) {
                    msg_print(_("恐ろしい光景が頭に浮かんできた。", "A horrible vision enters your mind."));

                    /* Have some nightmares */
                    sanity_blast(creature_ptr, NULL, false);
                }
                if (set_paralyzed(creature_ptr, creature_ptr->paralyzed + randint0(4) + 4)) {
                    ident = true;
                }
            }
            break;

        case SV_POTION_LOSE_MEMORIES:
            if (!creature_ptr->hold_exp && (creature_ptr->exp > 0)) {
                msg_print(_("過去の記憶が薄れていく気がする。", "You feel your memories fade."));
                chg_virtue(creature_ptr, V_KNOWLEDGE, -5);

                lose_exp(creature_ptr, creature_ptr->exp / 4);
                ident = true;
            }
            break;

        case SV_POTION_RUINATION:
            msg_print(_("身も心も弱ってきて、精気が抜けていくようだ。", "Your nerves and muscles feel weak and lifeless!"));
            take_hit(creature_ptr, DAMAGE_LOSELIFE, damroll(10, 10), _("破滅の薬", "a potion of Ruination"));

            (void)dec_stat(creature_ptr, A_DEX, 25, true);
            (void)dec_stat(creature_ptr, A_WIS, 25, true);
            (void)dec_stat(creature_ptr, A_CON, 25, true);
            (void)dec_stat(creature_ptr, A_STR, 25, true);
            (void)dec_stat(creature_ptr, A_CHR, 25, true);
            (void)dec_stat(creature_ptr, A_INT, 25, true);
            ident = true;
            break;

        case SV_POTION_DEC_STR:
            if (do_dec_stat(creature_ptr, A_STR))
                ident = true;
            break;

        case SV_POTION_DEC_INT:
            if (do_dec_stat(creature_ptr, A_INT))
                ident = true;
            break;

        case SV_POTION_DEC_WIS:
            if (do_dec_stat(creature_ptr, A_WIS))
                ident = true;
            break;

        case SV_POTION_DEC_DEX:
            if (do_dec_stat(creature_ptr, A_DEX))
                ident = true;
            break;

        case SV_POTION_DEC_CON:
            if (do_dec_stat(creature_ptr, A_CON))
                ident = true;
            break;

        case SV_POTION_DEC_CHR:
            if (do_dec_stat(creature_ptr, A_CHR))
                ident = true;
            break;

        case SV_POTION_DETONATIONS:
            ident = detonation(creature_ptr);
            break;

        case SV_POTION_DEATH:
            chg_virtue(creature_ptr, V_VITALITY, -1);
            chg_virtue(creature_ptr, V_UNLIFE, 5);
            msg_print(_("死の予感が体中を駆けめぐった。", "A feeling of Death flows through your body."));
            take_hit(creature_ptr, DAMAGE_LOSELIFE, 5000, _("死の薬", "a potion of Death"));
            ident = true;
            break;

        case SV_POTION_INFRAVISION:
            if (set_tim_infra(creature_ptr, creature_ptr->tim_infra + 100 + randint1(100), false)) {
                ident = true;
            }
            break;

        case SV_POTION_DETECT_INVIS:
            if (set_tim_invis(creature_ptr, creature_ptr->tim_invis + 12 + randint1(12), false)) {
                ident = true;
            }
            break;

        case SV_POTION_SLOW_POISON:
            if (set_poisoned(creature_ptr, creature_ptr->poisoned / 2))
                ident = true;
            break;

        case SV_POTION_CURE_POISON:
            if (set_poisoned(creature_ptr, 0))
                ident = true;
            break;

        case SV_POTION_BOLDNESS:
            if (set_afraid(creature_ptr, 0))
                ident = true;
            break;

        case SV_POTION_SPEED:
            if (!creature_ptr->fast) {
                if (set_fast(creature_ptr, randint1(25) + 15, false))
                    ident = true;
            } else {
                (void)set_fast(creature_ptr, creature_ptr->fast + 5, false);
            }
            break;

        case SV_POTION_RESIST_HEAT:
            if (set_oppose_fire(creature_ptr, creature_ptr->oppose_fire + randint1(10) + 10, false)) {
                ident = true;
            }
            break;

        case SV_POTION_RESIST_COLD:
            if (set_oppose_cold(creature_ptr, creature_ptr->oppose_cold + randint1(10) + 10, false)) {
                ident = true;
            }
            break;

        case SV_POTION_HEROISM:
            ident = heroism(creature_ptr, 25);
            break;

        case SV_POTION_BESERK_STRENGTH:
            ident = berserk(creature_ptr, randint1(25) + 25);
            break;

        case SV_POTION_CURE_LIGHT:
            ident = cure_light_wounds(creature_ptr, 2, 8);
            break;

        case SV_POTION_CURE_SERIOUS:
            ident = cure_serious_wounds(creature_ptr, 4, 8);
            break;

        case SV_POTION_CURE_CRITICAL:
            ident = cure_critical_wounds(creature_ptr, damroll(6, 8));
            break;

        case SV_POTION_HEALING:
            ident = cure_critical_wounds(creature_ptr, 300);
            break;

        case SV_POTION_STAR_HEALING:
            ident = cure_critical_wounds(creature_ptr, 1200);
            break;

        case SV_POTION_LIFE:
            ident = life_stream(creature_ptr, true, true);
            break;

        case SV_POTION_RESTORE_MANA:
            ident = restore_mana(creature_ptr, true);
            break;

        case SV_POTION_RESTORE_EXP:
            if (restore_level(creature_ptr))
                ident = true;
            break;

        case SV_POTION_RES_STR:
            if (do_res_stat(creature_ptr, A_STR))
                ident = true;
            break;

        case SV_POTION_RES_INT:
            if (do_res_stat(creature_ptr, A_INT))
                ident = true;
            break;

        case SV_POTION_RES_WIS:
            if (do_res_stat(creature_ptr, A_WIS))
                ident = true;
            break;

        case SV_POTION_RES_DEX:
            if (do_res_stat(creature_ptr, A_DEX))
                ident = true;
            break;

        case SV_POTION_RES_CON:
            if (do_res_stat(creature_ptr, A_CON))
                ident = true;
            break;

        case SV_POTION_RES_CHR:
            if (do_res_stat(creature_ptr, A_CHR))
                ident = true;
            break;

        case SV_POTION_INC_STR:
            if (do_inc_stat(creature_ptr, A_STR))
                ident = true;
            break;

        case SV_POTION_INC_INT:
            if (do_inc_stat(creature_ptr, A_INT))
                ident = true;
            break;

        case SV_POTION_INC_WIS:
            if (do_inc_stat(creature_ptr, A_WIS))
                ident = true;
            break;

        case SV_POTION_INC_DEX:
            if (do_inc_stat(creature_ptr, A_DEX))
                ident = true;
            break;

        case SV_POTION_INC_CON:
            if (do_inc_stat(creature_ptr, A_CON))
                ident = true;
            break;

        case SV_POTION_INC_CHR:
            if (do_inc_stat(creature_ptr, A_CHR))
                ident = true;
            break;

        case SV_POTION_POLY_SELF:
            do_poly_self(creature_ptr);
            ident = true;
            break;

        case SV_POTION_AUGMENTATION:
            if (do_inc_stat(creature_ptr, A_STR))
                ident = true;
            if (do_inc_stat(creature_ptr, A_INT))
                ident = true;
            if (do_inc_stat(creature_ptr, A_WIS))
                ident = true;
            if (do_inc_stat(creature_ptr, A_DEX))
                ident = true;
            if (do_inc_stat(creature_ptr, A_CON))
                ident = true;
            if (do_inc_stat(creature_ptr, A_CHR))
                ident = true;
            break;

        case SV_POTION_ENLIGHTENMENT:
            msg_print(_("自分の置かれている状況が脳裏に浮かんできた...", "An image of your surroundings forms in your mind..."));
            chg_virtue(creature_ptr, V_KNOWLEDGE, 1);
            chg_virtue(creature_ptr, V_ENLIGHTEN, 1);
            wiz_lite(creature_ptr, false);
            ident = true;
            break;

        case SV_POTION_STAR_ENLIGHTENMENT:
            msg_print(_("更なる啓蒙を感じた...", "You begin to feel more enlightened..."));
            chg_virtue(creature_ptr, V_KNOWLEDGE, 1);
            chg_virtue(creature_ptr, V_ENLIGHTEN, 2);
            msg_print(NULL);
            wiz_lite(creature_ptr, false);
            (void)do_inc_stat(creature_ptr, A_INT);
            (void)do_inc_stat(creature_ptr, A_WIS);
            (void)detect_traps(creature_ptr, DETECT_RAD_DEFAULT, true);
            (void)detect_doors(creature_ptr, DETECT_RAD_DEFAULT);
            (void)detect_stairs(creature_ptr, DETECT_RAD_DEFAULT);
            (void)detect_treasure(creature_ptr, DETECT_RAD_DEFAULT);
            (void)detect_objects_gold(creature_ptr, DETECT_RAD_DEFAULT);
            (void)detect_objects_normal(creature_ptr, DETECT_RAD_DEFAULT);
            identify_pack(creature_ptr);
            self_knowledge(creature_ptr);
            ident = true;
            break;

        case SV_POTION_SELF_KNOWLEDGE:
            msg_print(_("自分自身のことが少しは分かった気がする...", "You begin to know yourself a little better..."));
            msg_print(NULL);
            self_knowledge(creature_ptr);
            ident = true;
            break;

        case SV_POTION_EXPERIENCE:
            if (creature_ptr->prace == player_race_type::ANDROID)
                break;
            chg_virtue(creature_ptr, V_ENLIGHTEN, 1);
            if (creature_ptr->exp < PY_MAX_EXP) {
                EXP ee = (creature_ptr->exp / 2) + 10;
                if (ee > 100000L)
                    ee = 100000L;
                msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
                gain_exp(creature_ptr, ee);
                ident = true;
            }
            break;

        case SV_POTION_RESISTANCE:
            (void)set_oppose_acid(creature_ptr, creature_ptr->oppose_acid + randint1(20) + 20, false);
            (void)set_oppose_elec(creature_ptr, creature_ptr->oppose_elec + randint1(20) + 20, false);
            (void)set_oppose_fire(creature_ptr, creature_ptr->oppose_fire + randint1(20) + 20, false);
            (void)set_oppose_cold(creature_ptr, creature_ptr->oppose_cold + randint1(20) + 20, false);
            (void)set_oppose_pois(creature_ptr, creature_ptr->oppose_pois + randint1(20) + 20, false);
            ident = true;
            break;

        case SV_POTION_CURING:
            if (true_healing(creature_ptr, 50))
                ident = true;
            break;

        case SV_POTION_INVULNERABILITY:
            (void)set_invuln(creature_ptr, creature_ptr->invuln + randint1(4) + 4, false);
            ident = true;
            break;

        case SV_POTION_NEW_LIFE:
            roll_hitdice(creature_ptr, SPOP_NONE);
            get_max_stats(creature_ptr);
            creature_ptr->update |= PU_BONUS;
            lose_all_mutations(creature_ptr);
            ident = true;
            break;

        case SV_POTION_NEO_TSUYOSHI:
            (void)set_image(creature_ptr, 0);
            (void)set_tsuyoshi(creature_ptr, creature_ptr->tsuyoshi + randint1(100) + 100, false);
            ident = true;
            break;

        case SV_POTION_TSUYOSHI:
            msg_print(_("「オクレ兄さん！」", "Brother OKURE!"));
            msg_print(NULL);
            creature_ptr->tsuyoshi = 1;
            (void)set_tsuyoshi(creature_ptr, 0, true);
            if (!has_resist_chaos(creature_ptr)) {
                (void)set_image(creature_ptr, 50 + randint1(50));
            }
            ident = true;
            break;

        case SV_POTION_POLYMORPH:
            if (creature_ptr->muta.any() && one_in_(23)) {
                lose_all_mutations(creature_ptr);
            } else {
                do {
                    if (one_in_(2)) {
                        if (gain_mutation(creature_ptr, 0))
                            ident = true;
                    } else if (lose_mutation(creature_ptr, 0))
                        ident = true;
                } while (!ident || one_in_(2));
            }
            break;
        }
    }

    if (is_specific_player_race(creature_ptr, player_race_type::SKELETON)) {
        msg_print(_("液体の一部はあなたのアゴを素通りして落ちた！", "Some of the fluid falls through your jaws!"));
        (void)potion_smash_effect(creature_ptr, 0, creature_ptr->y, creature_ptr->x, q_ptr->k_idx);
    }
    creature_ptr->update |= (PU_COMBINE | PU_REORDER);

    if (!(object_is_aware(q_ptr))) {
        chg_virtue(creature_ptr, V_PATIENCE, -1);
        chg_virtue(creature_ptr, V_CHANCE, 1);
        chg_virtue(creature_ptr, V_KNOWLEDGE, -1);
    }

    /* The item has been tried */
    object_tried(q_ptr);

    /* An identification was made */
    if (ident && !object_is_aware(q_ptr)) {
        object_aware(creature_ptr, q_ptr);
        gain_exp(creature_ptr, (lev + (creature_ptr->lev >> 1)) / creature_ptr->lev);
    }

    creature_ptr->window_flags |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

    if (is_specific_player_race(creature_ptr, player_race_type::SKELETON))
        return; //!< @note スケルトンは水分で飢えを満たせない

    switch (player_race_food(creature_ptr)) {
    case PlayerRaceFood::WATER:
        msg_print(_("水分を取り込んだ。", "You are moistened."));
        set_food(creature_ptr, MIN(creature_ptr->food + q_ptr->pval + MAX(0, q_ptr->pval * 10) + 2000, PY_FOOD_MAX - 1));
        break;
    case PlayerRaceFood::OIL:
        if (q_ptr->tval == TV_FLASK) {
            msg_print(_("オイルを補給した。", "You replenish yourself with the oil."));
            set_food(creature_ptr, creature_ptr->food + 5000);
        } else {
            set_food(creature_ptr, creature_ptr->food + ((q_ptr->pval) / 20));
        }
        break;
    case PlayerRaceFood::BLOOD:
        (void)set_food(creature_ptr, creature_ptr->food + (q_ptr->pval / 10));
        break;
    case PlayerRaceFood::MANA:
    case PlayerRaceFood::CORPSE:
        set_food(creature_ptr, creature_ptr->food + ((q_ptr->pval) / 20));
        break;
    default:
        (void)set_food(creature_ptr, creature_ptr->food + q_ptr->pval);
        break;
    }
}
