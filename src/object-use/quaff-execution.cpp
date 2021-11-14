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
#include "object-use/item-use-checker.h"
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
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 */
ObjectQuaffEntity::ObjectQuaffEntity(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief 薬を飲む.
 * @param item 飲む薬オブジェクトの所持品ID
 * @details
 * 効果発動のあと、食料タイプによって空腹度を少し充足する。
 */
void ObjectQuaffEntity::execute(INVENTORY_IDX item)
{
    PlayerEnergy(this->player_ptr).set_player_turn_energy(100);
    if (!this->check_can_quaff()) {
        return;
    }

    if (music_singing_any(this->player_ptr))
        stop_singing(this->player_ptr);

    SpellHex spell_hex(this->player_ptr);
    if (spell_hex.is_spelling_any() && !spell_hex.is_spelling_specific(HEX_INHALE)) {
        (void)SpellHex(this->player_ptr).stop_all_spells();
    }

    auto *o_ptr = ref_item(this->player_ptr, item);
    object_type forge;
    auto *q_ptr = &forge;
    q_ptr->copy_from(o_ptr);
    q_ptr->number = 1;
    vary_item(this->player_ptr, item, -1);
    sound(SOUND_QUAFF);
    auto ident = false;
    if (q_ptr->tval == ItemKindType::POTION) {
        switch (q_ptr->sval) {
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
            if (BadStatusSetter(this->player_ptr).slowness(randint1(25) + 15, false))
                ident = true;
            break;

        case SV_POTION_SALT_WATER: {
            msg_print(_("うぇ！思わず吐いてしまった。", "The potion makes you vomit!"));
            switch (PlayerRace(this->player_ptr).food()) {
            case PlayerRaceFoodType::RATION:
            case PlayerRaceFoodType::WATER:
            case PlayerRaceFoodType::BLOOD:
                (void)set_food(this->player_ptr, PY_FOOD_STARVE - 1);
                break;
            default:
                break;
            }

            BadStatusSetter bss(this->player_ptr);
            (void)bss.poison(0);
            (void)bss.mod_paralysis(4);
            ident = true;
            break;
        }
        case SV_POTION_POISON:
            if (!(has_resist_pois(this->player_ptr) || is_oppose_pois(this->player_ptr))) {
                if (BadStatusSetter(this->player_ptr).mod_poison(randint0(15) + 10)) {
                    ident = true;
                }
            }
            break;

        case SV_POTION_BLINDNESS:
            if (!has_resist_blind(this->player_ptr)) {
                if (BadStatusSetter(this->player_ptr).mod_blindness(randint0(100) + 100)) {
                    ident = true;
                }
            }
            break;

        case SV_POTION_BOOZE:
            ident = booze();
            break;

        case SV_POTION_SLEEP:
            if (this->player_ptr->free_act) {
                break;
            }

            msg_print(_("あなたは眠ってしまった。", "You fall asleep."));
            if (ironman_nightmare) {
                msg_print(_("恐ろしい光景が頭に浮かんできた。", "A horrible vision enters your mind."));
                sanity_blast(this->player_ptr, nullptr, false);
            }

            if (BadStatusSetter(this->player_ptr).mod_paralysis(randint0(4) + 4)) {
                ident = true;
            }

            break;
        case SV_POTION_LOSE_MEMORIES:
            if (!this->player_ptr->hold_exp && (this->player_ptr->exp > 0)) {
                msg_print(_("過去の記憶が薄れていく気がする。", "You feel your memories fade."));
                chg_virtue(this->player_ptr, V_KNOWLEDGE, -5);

                lose_exp(this->player_ptr, this->player_ptr->exp / 4);
                ident = true;
            }
            break;

        case SV_POTION_RUINATION:
            msg_print(_("身も心も弱ってきて、精気が抜けていくようだ。", "Your nerves and muscles feel weak and lifeless!"));
            take_hit(this->player_ptr, DAMAGE_LOSELIFE, damroll(10, 10), _("破滅の薬", "a potion of Ruination"));

            (void)dec_stat(this->player_ptr, A_DEX, 25, true);
            (void)dec_stat(this->player_ptr, A_WIS, 25, true);
            (void)dec_stat(this->player_ptr, A_CON, 25, true);
            (void)dec_stat(this->player_ptr, A_STR, 25, true);
            (void)dec_stat(this->player_ptr, A_CHR, 25, true);
            (void)dec_stat(this->player_ptr, A_INT, 25, true);
            ident = true;
            break;

        case SV_POTION_DEC_STR:
            if (do_dec_stat(this->player_ptr, A_STR))
                ident = true;
            break;

        case SV_POTION_DEC_INT:
            if (do_dec_stat(this->player_ptr, A_INT))
                ident = true;
            break;

        case SV_POTION_DEC_WIS:
            if (do_dec_stat(this->player_ptr, A_WIS))
                ident = true;
            break;

        case SV_POTION_DEC_DEX:
            if (do_dec_stat(this->player_ptr, A_DEX))
                ident = true;
            break;

        case SV_POTION_DEC_CON:
            if (do_dec_stat(this->player_ptr, A_CON))
                ident = true;
            break;

        case SV_POTION_DEC_CHR:
            if (do_dec_stat(this->player_ptr, A_CHR))
                ident = true;
            break;

        case SV_POTION_DETONATIONS:
            ident = detonation();
            break;

        case SV_POTION_DEATH:
            chg_virtue(this->player_ptr, V_VITALITY, -1);
            chg_virtue(this->player_ptr, V_UNLIFE, 5);
            msg_print(_("死の予感が体中を駆けめぐった。", "A feeling of Death flows through your body."));
            take_hit(this->player_ptr, DAMAGE_LOSELIFE, 5000, _("死の薬", "a potion of Death"));
            ident = true;
            break;

        case SV_POTION_INFRAVISION:
            if (set_tim_infra(this->player_ptr, this->player_ptr->tim_infra + 100 + randint1(100), false)) {
                ident = true;
            }
            break;

        case SV_POTION_DETECT_INVIS:
            if (set_tim_invis(this->player_ptr, this->player_ptr->tim_invis + 12 + randint1(12), false)) {
                ident = true;
            }
            break;

        case SV_POTION_SLOW_POISON:
            if (BadStatusSetter(this->player_ptr).poison(this->player_ptr->poisoned / 2))
                ident = true;
            break;

        case SV_POTION_CURE_POISON:
            if (BadStatusSetter(this->player_ptr).poison(0))
                ident = true;
            break;

        case SV_POTION_BOLDNESS:
            if (BadStatusSetter(this->player_ptr).afraidness(0))
                ident = true;
            break;

        case SV_POTION_SPEED:
            if (!this->player_ptr->fast) {
                if (set_fast(this->player_ptr, randint1(25) + 15, false))
                    ident = true;
            } else {
                (void)set_fast(this->player_ptr, this->player_ptr->fast + 5, false);
            }
            break;

        case SV_POTION_RESIST_HEAT:
            if (set_oppose_fire(this->player_ptr, this->player_ptr->oppose_fire + randint1(10) + 10, false)) {
                ident = true;
            }
            break;

        case SV_POTION_RESIST_COLD:
            if (set_oppose_cold(this->player_ptr, this->player_ptr->oppose_cold + randint1(10) + 10, false)) {
                ident = true;
            }
            break;

        case SV_POTION_HEROISM:
            ident = heroism(this->player_ptr, 25);
            break;

        case SV_POTION_BESERK_STRENGTH:
            ident = berserk(this->player_ptr, randint1(25) + 25);
            break;

        case SV_POTION_CURE_LIGHT:
            ident = cure_light_wounds(this->player_ptr, 2, 8);
            break;

        case SV_POTION_CURE_SERIOUS:
            ident = cure_serious_wounds(this->player_ptr, 4, 8);
            break;

        case SV_POTION_CURE_CRITICAL:
            ident = cure_critical_wounds(this->player_ptr, damroll(6, 8));
            break;

        case SV_POTION_HEALING:
            ident = cure_critical_wounds(this->player_ptr, 300);
            break;

        case SV_POTION_STAR_HEALING:
            ident = cure_critical_wounds(this->player_ptr, 1200);
            break;

        case SV_POTION_LIFE:
            ident = life_stream(this->player_ptr, true, true);
            break;

        case SV_POTION_RESTORE_MANA:
            ident = restore_mana(this->player_ptr, true);
            break;

        case SV_POTION_RESTORE_EXP:
            if (restore_level(this->player_ptr))
                ident = true;
            break;

        case SV_POTION_RES_STR:
            if (do_res_stat(this->player_ptr, A_STR))
                ident = true;
            break;

        case SV_POTION_RES_INT:
            if (do_res_stat(this->player_ptr, A_INT))
                ident = true;
            break;

        case SV_POTION_RES_WIS:
            if (do_res_stat(this->player_ptr, A_WIS))
                ident = true;
            break;

        case SV_POTION_RES_DEX:
            if (do_res_stat(this->player_ptr, A_DEX))
                ident = true;
            break;

        case SV_POTION_RES_CON:
            if (do_res_stat(this->player_ptr, A_CON))
                ident = true;
            break;

        case SV_POTION_RES_CHR:
            if (do_res_stat(this->player_ptr, A_CHR))
                ident = true;
            break;

        case SV_POTION_INC_STR:
            if (do_inc_stat(this->player_ptr, A_STR))
                ident = true;
            break;

        case SV_POTION_INC_INT:
            if (do_inc_stat(this->player_ptr, A_INT))
                ident = true;
            break;

        case SV_POTION_INC_WIS:
            if (do_inc_stat(this->player_ptr, A_WIS))
                ident = true;
            break;

        case SV_POTION_INC_DEX:
            if (do_inc_stat(this->player_ptr, A_DEX))
                ident = true;
            break;

        case SV_POTION_INC_CON:
            if (do_inc_stat(this->player_ptr, A_CON))
                ident = true;
            break;

        case SV_POTION_INC_CHR:
            if (do_inc_stat(this->player_ptr, A_CHR))
                ident = true;
            break;

        case SV_POTION_POLY_SELF:
            do_poly_self(this->player_ptr);
            ident = true;
            break;

        case SV_POTION_AUGMENTATION:
            if (do_inc_stat(this->player_ptr, A_STR))
                ident = true;
            if (do_inc_stat(this->player_ptr, A_INT))
                ident = true;
            if (do_inc_stat(this->player_ptr, A_WIS))
                ident = true;
            if (do_inc_stat(this->player_ptr, A_DEX))
                ident = true;
            if (do_inc_stat(this->player_ptr, A_CON))
                ident = true;
            if (do_inc_stat(this->player_ptr, A_CHR))
                ident = true;
            break;

        case SV_POTION_ENLIGHTENMENT:
            msg_print(_("自分の置かれている状況が脳裏に浮かんできた...", "An image of your surroundings forms in your mind..."));
            chg_virtue(this->player_ptr, V_KNOWLEDGE, 1);
            chg_virtue(this->player_ptr, V_ENLIGHTEN, 1);
            wiz_lite(this->player_ptr, false);
            ident = true;
            break;

        case SV_POTION_STAR_ENLIGHTENMENT:
            msg_print(_("更なる啓蒙を感じた...", "You begin to feel more enlightened..."));
            chg_virtue(this->player_ptr, V_KNOWLEDGE, 1);
            chg_virtue(this->player_ptr, V_ENLIGHTEN, 2);
            msg_print(nullptr);
            wiz_lite(this->player_ptr, false);
            (void)do_inc_stat(this->player_ptr, A_INT);
            (void)do_inc_stat(this->player_ptr, A_WIS);
            (void)detect_traps(this->player_ptr, DETECT_RAD_DEFAULT, true);
            (void)detect_doors(this->player_ptr, DETECT_RAD_DEFAULT);
            (void)detect_stairs(this->player_ptr, DETECT_RAD_DEFAULT);
            (void)detect_treasure(this->player_ptr, DETECT_RAD_DEFAULT);
            (void)detect_objects_gold(this->player_ptr, DETECT_RAD_DEFAULT);
            (void)detect_objects_normal(this->player_ptr, DETECT_RAD_DEFAULT);
            identify_pack(this->player_ptr);
            self_knowledge(this->player_ptr);
            ident = true;
            break;

        case SV_POTION_SELF_KNOWLEDGE:
            msg_print(_("自分自身のことが少しは分かった気がする...", "You begin to know yourself a little better..."));
            msg_print(nullptr);
            self_knowledge(this->player_ptr);
            ident = true;
            break;

        case SV_POTION_EXPERIENCE:
            if (this->player_ptr->prace == PlayerRaceType::ANDROID)
                break;
            chg_virtue(this->player_ptr, V_ENLIGHTEN, 1);
            if (this->player_ptr->exp < PY_MAX_EXP) {
                EXP ee = (this->player_ptr->exp / 2) + 10;
                if (ee > 100000L)
                    ee = 100000L;
                msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
                gain_exp(this->player_ptr, ee);
                ident = true;
            }
            break;

        case SV_POTION_RESISTANCE:
            (void)set_oppose_acid(this->player_ptr, this->player_ptr->oppose_acid + randint1(20) + 20, false);
            (void)set_oppose_elec(this->player_ptr, this->player_ptr->oppose_elec + randint1(20) + 20, false);
            (void)set_oppose_fire(this->player_ptr, this->player_ptr->oppose_fire + randint1(20) + 20, false);
            (void)set_oppose_cold(this->player_ptr, this->player_ptr->oppose_cold + randint1(20) + 20, false);
            (void)set_oppose_pois(this->player_ptr, this->player_ptr->oppose_pois + randint1(20) + 20, false);
            ident = true;
            break;

        case SV_POTION_CURING:
            if (true_healing(this->player_ptr, 50))
                ident = true;
            break;

        case SV_POTION_INVULNERABILITY:
            (void)set_invuln(this->player_ptr, this->player_ptr->invuln + randint1(4) + 4, false);
            ident = true;
            break;

        case SV_POTION_NEW_LIFE:
            roll_hitdice(this->player_ptr, SPOP_NONE);
            get_max_stats(this->player_ptr);
            this->player_ptr->update |= PU_BONUS;
            lose_all_mutations(this->player_ptr);
            ident = true;
            break;

        case SV_POTION_NEO_TSUYOSHI:
            (void)BadStatusSetter(this->player_ptr).hallucination(0);
            (void)set_tsuyoshi(this->player_ptr, this->player_ptr->tsuyoshi + randint1(100) + 100, false);
            ident = true;
            break;

        case SV_POTION_TSUYOSHI:
            msg_print(_("「オクレ兄さん！」", "Brother OKURE!"));
            msg_print(nullptr);
            this->player_ptr->tsuyoshi = 1;
            (void)set_tsuyoshi(this->player_ptr, 0, true);
            if (!has_resist_chaos(this->player_ptr)) {
                (void)BadStatusSetter(this->player_ptr).hallucination(50 + randint1(50));
            }
            ident = true;
            break;

        case SV_POTION_POLYMORPH:
            if (this->player_ptr->muta.any() && one_in_(23)) {
                lose_all_mutations(this->player_ptr);
            } else {
                do {
                    if (one_in_(2)) {
                        if (gain_mutation(this->player_ptr, 0))
                            ident = true;
                    } else if (lose_mutation(this->player_ptr, 0))
                        ident = true;
                } while (!ident || one_in_(2));
            }
            break;
        }
    }

    if (PlayerRace(this->player_ptr).equals(PlayerRaceType::SKELETON)) {
        msg_print(_("液体の一部はあなたのアゴを素通りして落ちた！", "Some of the fluid falls through your jaws!"));
        (void)potion_smash_effect(this->player_ptr, 0, this->player_ptr->y, this->player_ptr->x, q_ptr->k_idx);
    }

    this->player_ptr->update |= PU_COMBINE | PU_REORDER;
    if (!(q_ptr->is_aware())) {
        chg_virtue(this->player_ptr, V_PATIENCE, -1);
        chg_virtue(this->player_ptr, V_CHANCE, 1);
        chg_virtue(this->player_ptr, V_KNOWLEDGE, -1);
    }

    /* The item has been tried */
    object_tried(q_ptr);

    /* An identification was made */
    if (ident && !q_ptr->is_aware()) {
        object_aware(this->player_ptr, q_ptr);
        gain_exp(this->player_ptr, (k_info[q_ptr->k_idx].level + (this->player_ptr->lev >> 1)) / this->player_ptr->lev);
    }

    this->player_ptr->window_flags |= (PW_INVEN | PW_EQUIP | PW_PLAYER);

    if (PlayerRace(this->player_ptr).equals(PlayerRaceType::SKELETON))
        return; //!< @note スケルトンは水分で飢えを満たせない

    switch (PlayerRace(this->player_ptr).food()) {
    case PlayerRaceFoodType::WATER:
        msg_print(_("水分を取り込んだ。", "You are moistened."));
        set_food(this->player_ptr, std::min<short>(this->player_ptr->food + q_ptr->pval + std::max<short>(0, q_ptr->pval * 10) + 2000, PY_FOOD_MAX - 1));
        break;
    case PlayerRaceFoodType::OIL:
        if (q_ptr->tval == ItemKindType::FLASK) {
            msg_print(_("オイルを補給した。", "You replenish yourself with the oil."));
            set_food(this->player_ptr, this->player_ptr->food + 5000);
        } else {
            set_food(this->player_ptr, this->player_ptr->food + ((q_ptr->pval) / 20));
        }
        break;
    case PlayerRaceFoodType::BLOOD:
        (void)set_food(this->player_ptr, this->player_ptr->food + (q_ptr->pval / 10));
        break;
    case PlayerRaceFoodType::MANA:
    case PlayerRaceFoodType::CORPSE:
        set_food(this->player_ptr, this->player_ptr->food + ((q_ptr->pval) / 20));
        break;
    default:
        (void)set_food(this->player_ptr, this->player_ptr->food + q_ptr->pval);
        break;
    }
}

bool ObjectQuaffEntity::check_can_quaff()
{
    if (this->player_ptr->timewalk) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("瓶から水が流れ出てこない！", "The potion doesn't flow out from the bottle."));
        sound(SOUND_FAIL);
        return false;
    }

    return ItemUseChecker(this->player_ptr).check_stun(_("朦朧としていて瓶の蓋を開けられなかった！", "You were not able to quaff it by the stun!"));
}

/*!
 * @brief 酔っ払いの薬
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return カオス耐性があるかその他の一部確率でFALSE、それ以外はTRUE
 */
bool ObjectQuaffEntity::booze()
{
    bool ident = false;
    if (this->player_ptr->pclass != PlayerClassType::MONK)
        chg_virtue(this->player_ptr, V_HARMONY, -1);
    else if (!has_resist_conf(this->player_ptr))
        this->player_ptr->special_attack |= ATTACK_SUIKEN;

    BadStatusSetter bss(this->player_ptr);
    if (!has_resist_conf(this->player_ptr) && bss.confusion(randint0(20) + 15)) {
        ident = true;
    }

    if (has_resist_chaos(this->player_ptr)) {
        return ident;
    }

    if (one_in_(2) && bss.mod_hallucination(randint0(150) + 150)) {
        ident = true;
    }

    if (one_in_(13) && (this->player_ptr->pclass != PlayerClassType::MONK)) {
        ident = true;
        if (one_in_(3))
            lose_all_info(this->player_ptr);
        else
            wiz_dark(this->player_ptr);

        (void)teleport_player_aux(this->player_ptr, 100, false, i2enum<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
        wiz_dark(this->player_ptr);
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
bool ObjectQuaffEntity::detonation()
{
    msg_print(_("体の中で激しい爆発が起きた！", "Massive explosions rupture your body!"));
    take_hit(this->player_ptr, DAMAGE_NOESCAPE, damroll(50, 20), _("爆発の薬", "a potion of Detonation"));
    BadStatusSetter bss(this->player_ptr);
    (void)bss.mod_stun(75);
    (void)bss.mod_cut(5000);
    return true;
}
