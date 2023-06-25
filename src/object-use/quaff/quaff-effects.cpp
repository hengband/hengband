/*
 * @brief 薬を飲んだ時の効果処理
 * @date 2022/03/10
 * @author Hourier
 */

#include "object-use/quaff/quaff-effects.h"
#include "avatar/avatar.h"
#include "birth/birth-stat.h"
#include "game-option/birth-options.h"
#include "mutation/mutation-investor-remover.h"
#include "object/object-info.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/race-info.h"
#include "player-info/self-info.h"
#include "player/attack-defense-types.h"
#include "player/digestion-processor.h"
#include "player/eldritch-horror.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "spell-kind/spells-detection.h"
#include "spell-kind/spells-floor.h"
#include "spell-kind/spells-perception.h"
#include "spell-kind/spells-teleport.h"
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
#include "system/angband.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/player-acceleration.h"
#include "timed-effect/player-poison.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

QuaffEffects::QuaffEffects(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

bool QuaffEffects::influence(const ItemEntity &o_ref)
{
    if (o_ref.bi_key.tval() != ItemKindType::POTION) {
        return false;
    }

    switch (o_ref.bi_key.sval().value()) {
    case SV_POTION_WATER:
        msg_print(_("口の中がさっぱりした。", "That was refreshing."));
        msg_print(_("のどの渇きが少しおさまった。", "You feel less thirsty."));
        return true;
    case SV_POTION_APPLE_JUICE:
        msg_print(_("甘くてサッパリとしていて、とてもおいしい。", "It's sweet, refreshing and very tasty."));
        msg_print(_("のどの渇きが少しおさまった。", "You feel less thirsty."));
        return true;
    case SV_POTION_SLIME_MOLD:
        msg_print(_("なんとも不気味な味だ。", "That was strange."));
        msg_print(_("のどの渇きが少しおさまった。", "You feel less thirsty."));
        return true;
    case SV_POTION_SLOWNESS:
        return BadStatusSetter(this->player_ptr).set_deceleration(randint1(25) + 15, false);
    case SV_POTION_SALT_WATER:
        return this->salt_water();
    case SV_POTION_POISON:
        return this->poison();
    case SV_POTION_BLINDNESS:
        return this->blindness();
    case SV_POTION_BOOZE:
        return this->booze();
        break;
    case SV_POTION_SLEEP:
        return this->sleep();
    case SV_POTION_LOSE_MEMORIES:
        return this->lose_memories();
    case SV_POTION_RUINATION:
        return this->ruination();
    case SV_POTION_DEC_STR:
        return do_dec_stat(this->player_ptr, A_STR);
    case SV_POTION_DEC_INT:
        return do_dec_stat(this->player_ptr, A_INT);
    case SV_POTION_DEC_WIS:
        return do_dec_stat(this->player_ptr, A_WIS);
    case SV_POTION_DEC_DEX:
        return do_dec_stat(this->player_ptr, A_DEX);
    case SV_POTION_DEC_CON:
        return do_dec_stat(this->player_ptr, A_CON);
    case SV_POTION_DEC_CHR:
        return do_dec_stat(this->player_ptr, A_CHR);
    case SV_POTION_DETONATIONS:
        return this->detonation();
    case SV_POTION_DEATH:
        return this->death();
    case SV_POTION_INFRAVISION:
        return set_tim_infra(this->player_ptr, this->player_ptr->tim_infra + 100 + randint1(100), false);
    case SV_POTION_DETECT_INVIS:
        return set_tim_invis(this->player_ptr, this->player_ptr->tim_invis + 12 + randint1(12), false);
    case SV_POTION_SLOW_POISON: {
        const auto player_poison = this->player_ptr->effects()->poison();
        return BadStatusSetter(this->player_ptr).set_poison(player_poison->current() / 2);
    }
    case SV_POTION_CURE_POISON:
        return BadStatusSetter(this->player_ptr).set_poison(0);
    case SV_POTION_BOLDNESS:
        return BadStatusSetter(this->player_ptr).set_fear(0);
    case SV_POTION_SPEED:
        return this->speed();
    case SV_POTION_RESIST_HEAT:
        return set_oppose_fire(this->player_ptr, this->player_ptr->oppose_fire + randint1(10) + 10, false);
    case SV_POTION_RESIST_COLD:
        return set_oppose_cold(this->player_ptr, this->player_ptr->oppose_cold + randint1(10) + 10, false);
    case SV_POTION_HEROISM:
        return heroism(this->player_ptr, 25);
    case SV_POTION_BESERK_STRENGTH:
        return berserk(this->player_ptr, randint1(25) + 25);
    case SV_POTION_CURE_LIGHT:
        return cure_light_wounds(this->player_ptr, 2, 8);
    case SV_POTION_CURE_SERIOUS:
        return cure_serious_wounds(this->player_ptr, 4, 8);
    case SV_POTION_CURE_CRITICAL:
        return cure_critical_wounds(this->player_ptr, damroll(6, 8));
    case SV_POTION_HEALING:
        return cure_critical_wounds(this->player_ptr, 300);
    case SV_POTION_STAR_HEALING:
        return cure_critical_wounds(this->player_ptr, 1200);
    case SV_POTION_LIFE:
        return life_stream(this->player_ptr, true, true);
    case SV_POTION_RESTORE_MANA:
        return restore_mana(this->player_ptr, true);
    case SV_POTION_RESTORE_EXP:
        return restore_level(this->player_ptr);
    case SV_POTION_RES_STR:
        return do_res_stat(this->player_ptr, A_STR);
    case SV_POTION_RES_INT:
        return do_res_stat(this->player_ptr, A_INT);
    case SV_POTION_RES_WIS:
        return do_res_stat(this->player_ptr, A_WIS);
    case SV_POTION_RES_DEX:
        return do_res_stat(this->player_ptr, A_DEX);
    case SV_POTION_RES_CON:
        return do_res_stat(this->player_ptr, A_CON);
    case SV_POTION_RES_CHR:
        return do_res_stat(this->player_ptr, A_CHR);
    case SV_POTION_INC_STR:
        return do_inc_stat(this->player_ptr, A_STR);
    case SV_POTION_INC_INT:
        return do_inc_stat(this->player_ptr, A_INT);
    case SV_POTION_INC_WIS:
        return do_inc_stat(this->player_ptr, A_WIS);
    case SV_POTION_INC_DEX:
        return do_inc_stat(this->player_ptr, A_DEX);
    case SV_POTION_INC_CON:
        return do_inc_stat(this->player_ptr, A_CON);
    case SV_POTION_INC_CHR:
        return do_inc_stat(this->player_ptr, A_CHR);
    case SV_POTION_POLY_SELF:
        do_poly_self(this->player_ptr);
        return true;
    case SV_POTION_AUGMENTATION:
        return this->augmentation();
    case SV_POTION_ENLIGHTENMENT:
        return this->enlightenment();
    case SV_POTION_STAR_ENLIGHTENMENT:
        return this->star_enlightenment();
    case SV_POTION_SELF_KNOWLEDGE:
        msg_print(_("自分自身のことが少しは分かった気がする...", "You begin to know yourself a little better..."));
        msg_print(nullptr);
        self_knowledge(this->player_ptr);
        return true;
    case SV_POTION_EXPERIENCE:
        return this->experience();
    case SV_POTION_RESISTANCE:
        return this->resistance();
    case SV_POTION_CURING:
        return true_healing(this->player_ptr, 50);
    case SV_POTION_INVULNERABILITY:
        (void)set_invuln(this->player_ptr, this->player_ptr->invuln + randint1(4) + 4, false);
        return true;
    case SV_POTION_NEW_LIFE:
        return this->new_life();
    case SV_POTION_NEO_TSUYOSHI:
        return this->neo_tsuyoshi();
    case SV_POTION_TSUYOSHI:
        return this->tsuyoshi();
    case SV_POTION_POLYMORPH:
        return this->polymorph();
    default:
        return false;
    }
}

/*!
 * @brief 塩水の薬
 * @return 常にtrue
 */
bool QuaffEffects::salt_water()
{
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
    (void)bss.set_poison(0);
    (void)bss.mod_paralysis(4);
    return true;
}

/*!
 * @brief 毒の薬
 * @return 毒の効果を受けたらtrue
 */
bool QuaffEffects::poison()
{
    if (has_resist_pois(this->player_ptr) || is_oppose_pois(this->player_ptr)) {
        return false;
    }

    return BadStatusSetter(this->player_ptr).mod_poison(randint0(15) + 10);
}

/*!
 * @brief 盲目の薬
 * @return 盲目になったらtrue
 */
bool QuaffEffects::blindness()
{
    if (has_resist_blind(this->player_ptr)) {
        return false;
    }

    return BadStatusSetter(this->player_ptr).mod_blindness(randint0(100) + 100);
}

/*!
 * @brief 酔っ払いの薬
 * @return カオス耐性があるかその他の一部確率でFALSE、それ以外はTRUE
 */
bool QuaffEffects::booze()
{
    auto ident = false;
    auto is_monk = PlayerClass(this->player_ptr).equals(PlayerClassType::MONK);
    if (!is_monk) {
        chg_virtue(this->player_ptr, Virtue::HARMONY, -1);
    } else if (!has_resist_conf(this->player_ptr)) {
        set_bits(this->player_ptr->special_attack, ATTACK_SUIKEN);
    }

    BadStatusSetter bss(this->player_ptr);
    if (!has_resist_conf(this->player_ptr) && bss.set_confusion(randint0(20) + 15)) {
        ident = true;
    }

    if (has_resist_chaos(this->player_ptr)) {
        return ident;
    }

    if (one_in_(2) && bss.mod_hallucination(randint0(150) + 150)) {
        ident = true;
    }

    if (is_monk || !one_in_(13)) {
        return ident;
    }

    ident = true;
    if (one_in_(3)) {
        lose_all_info(this->player_ptr);
    } else {
        wiz_dark(this->player_ptr);
    }

    (void)teleport_player_aux(this->player_ptr, 100, false, i2enum<teleport_flags>(TELEPORT_NONMAGICAL | TELEPORT_PASSIVE));
    wiz_dark(this->player_ptr);
    msg_print(_("知らない場所で目が醒めた。頭痛がする。", "You wake up somewhere with a sore head..."));
    msg_print(_("何も思い出せない。どうやってここへ来たのかも分からない！", "You can't remember a thing or how you got here!"));
    return ident;
}

/*!
 * @brief 眠りの薬
 * @return 麻痺したか否か
 */
bool QuaffEffects::sleep()
{
    if (this->player_ptr->free_act) {
        return false;
    }

    msg_print(_("あなたは眠ってしまった。", "You fall asleep."));
    if (ironman_nightmare) {
        msg_print(_("恐ろしい光景が頭に浮かんできた。", "A horrible vision enters your mind."));
        sanity_blast(this->player_ptr, nullptr, false);
    }

    return BadStatusSetter(this->player_ptr).mod_paralysis(randint0(4) + 4);
}

/*!
 * @brief 記憶喪失の薬
 * @return 経験値が下がったか
 */
bool QuaffEffects::lose_memories()
{
    if (this->player_ptr->hold_exp || (this->player_ptr->exp <= 0)) {
        return false;
    }

    msg_print(_("過去の記憶が薄れていく気がする。", "You feel your memories fade."));
    chg_virtue(this->player_ptr, Virtue::KNOWLEDGE, -5);
    lose_exp(this->player_ptr, this->player_ptr->exp / 4);
    return true;
}

/*!
 * @brief 破滅の薬
 * @return 常にtrue
 */
bool QuaffEffects::ruination()
{
    msg_print(_("身も心も弱ってきて、精気が抜けていくようだ。", "Your nerves and muscles feel weak and lifeless!"));
    take_hit(this->player_ptr, DAMAGE_LOSELIFE, damroll(10, 10), _("破滅の薬", "a potion of Ruination"));
    (void)dec_stat(this->player_ptr, A_DEX, 25, true);
    (void)dec_stat(this->player_ptr, A_WIS, 25, true);
    (void)dec_stat(this->player_ptr, A_CON, 25, true);
    (void)dec_stat(this->player_ptr, A_STR, 25, true);
    (void)dec_stat(this->player_ptr, A_CHR, 25, true);
    (void)dec_stat(this->player_ptr, A_INT, 25, true);
    return true;
}

/*!
 * @brief 爆発の薬 / Fumble ramble
 * @return 常にtrue
 */
bool QuaffEffects::detonation()
{
    msg_print(_("体の中で激しい爆発が起きた！", "Massive explosions rupture your body!"));
    take_hit(this->player_ptr, DAMAGE_NOESCAPE, damroll(50, 20), _("爆発の薬", "a potion of Detonation"));
    BadStatusSetter bss(this->player_ptr);
    (void)bss.mod_stun(75);
    (void)bss.mod_cut(5000);
    return true;
}

/*!
 * @brief 死の薬
 * @return 常にtrue
 */
bool QuaffEffects::death()
{
    chg_virtue(this->player_ptr, Virtue::VITALITY, -1);
    chg_virtue(this->player_ptr, Virtue::UNLIFE, 5);
    msg_print(_("死の予感が体中を駆けめぐった。", "A feeling of Death flows through your body."));
    take_hit(this->player_ptr, DAMAGE_LOSELIFE, 5000, _("死の薬", "a potion of Death"));
    return true;
}

/*!
 * @brief スピードの薬
 * @return 加速したらtrue、加速効果が切れていない状態で重ね飲みしたらfalse
 */
bool QuaffEffects::speed()
{
    if (this->player_ptr->effects()->acceleration()->is_fast()) {
        (void)mod_acceleration(this->player_ptr, 5, false);
        return false;
    }

    return set_acceleration(this->player_ptr, randint1(25) + 15, false);
}

/*!
 * @brief 増強の薬
 * @return アビリティスコアのどれか1つでも向上したらtrue
 */
bool QuaffEffects::augmentation()
{
    auto ident = false;
    if (do_inc_stat(this->player_ptr, A_STR)) {
        ident = true;
    }

    if (do_inc_stat(this->player_ptr, A_INT)) {
        ident = true;
    }

    if (do_inc_stat(this->player_ptr, A_WIS)) {
        ident = true;
    }

    if (do_inc_stat(this->player_ptr, A_DEX)) {
        ident = true;
    }

    if (do_inc_stat(this->player_ptr, A_CON)) {
        ident = true;
    }

    if (do_inc_stat(this->player_ptr, A_CHR)) {
        ident = true;
    }

    return ident;
}

/*!
 * @brief 啓蒙の薬
 * @return 常にtrue
 */
bool QuaffEffects::enlightenment()
{
    msg_print(_("自分の置かれている状況が脳裏に浮かんできた...", "An image of your surroundings forms in your mind..."));
    chg_virtue(this->player_ptr, Virtue::KNOWLEDGE, 1);
    chg_virtue(this->player_ptr, Virtue::ENLIGHTEN, 1);
    wiz_lite(this->player_ptr, false);
    return true;
}

/*!
 * @brief *啓蒙*の薬
 * @return 常にtrue
 */
bool QuaffEffects::star_enlightenment()
{
    msg_print(_("更なる啓蒙を感じた...", "You begin to feel more enlightened..."));
    chg_virtue(this->player_ptr, Virtue::KNOWLEDGE, 1);
    chg_virtue(this->player_ptr, Virtue::ENLIGHTEN, 2);
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
    return true;
}

/*!
 * @brief 経験の薬
 * @return 経験値が増えたらtrue
 */
bool QuaffEffects::experience()
{
    if (PlayerRace(this->player_ptr).equals(PlayerRaceType::ANDROID)) {
        return false;
    }

    chg_virtue(this->player_ptr, Virtue::ENLIGHTEN, 1);
    if (this->player_ptr->exp >= PY_MAX_EXP) {
        return false;
    }

    auto ee = (this->player_ptr->exp / 2) + 10;
    constexpr int max_exp = 100000;
    if (ee > max_exp) {
        ee = max_exp;
    }

    msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
    gain_exp(this->player_ptr, ee);
    return true;
}

/*!
 * @brief 耐性の薬
 * @return 経験値が増えたらtrue
 */
bool QuaffEffects::resistance()
{
    (void)set_oppose_acid(this->player_ptr, this->player_ptr->oppose_acid + randint1(20) + 20, false);
    (void)set_oppose_elec(this->player_ptr, this->player_ptr->oppose_elec + randint1(20) + 20, false);
    (void)set_oppose_fire(this->player_ptr, this->player_ptr->oppose_fire + randint1(20) + 20, false);
    (void)set_oppose_cold(this->player_ptr, this->player_ptr->oppose_cold + randint1(20) + 20, false);
    (void)set_oppose_pois(this->player_ptr, this->player_ptr->oppose_pois + randint1(20) + 20, false);
    return true;
}

/*!
 * @brief 新生の薬
 * @return 常にtrue
 */
bool QuaffEffects::new_life()
{
    roll_hitdice(this->player_ptr, SPOP_NONE);
    get_max_stats(this->player_ptr);
    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::BONUS);
    lose_all_mutations(this->player_ptr);
    return true;
}

/*!
 * @brief ネオ・つよしスペシャルの薬
 * @return 常にtrue
 */
bool QuaffEffects::neo_tsuyoshi()
{
    (void)BadStatusSetter(this->player_ptr).hallucination(0);
    (void)set_tsuyoshi(this->player_ptr, this->player_ptr->tsuyoshi + randint1(100) + 100, false);
    return true;
}

/*!
 * @brief つよしスペシャルの薬
 * @return 常にtrue
 */
bool QuaffEffects::tsuyoshi()
{
    msg_print(_("「オクレ兄さん！」", "Brother OKURE!"));
    msg_print(nullptr);
    this->player_ptr->tsuyoshi = 1;
    (void)set_tsuyoshi(this->player_ptr, 0, true);
    if (!has_resist_chaos(this->player_ptr)) {
        (void)BadStatusSetter(this->player_ptr).hallucination(50 + randint1(50));
    }

    return true;
}

/*!
 * @brief 自己変容の薬
 * @return 突然変異を得たか失ったらtrue、そのままならfalse
 */
bool QuaffEffects::polymorph()
{
    if (this->player_ptr->muta.any() && one_in_(23)) {
        lose_all_mutations(this->player_ptr);
        return false;
    }

    auto ident = false;
    do {
        if (one_in_(2)) {
            if (gain_mutation(this->player_ptr, 0)) {
                ident = true;
            }
        } else if (lose_mutation(this->player_ptr, 0)) {
            ident = true;
        }
    } while (!ident || one_in_(2));
    return ident;
}
