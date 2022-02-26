/*!
 * @brief モンスターからプレイヤーへの直接攻撃処理
 * @date 2020/05/23
 * @author Hourier
 */

#include "monster-attack/monster-attack-player.h"
#include "cmd-action/cmd-attack.h"
#include "cmd-action/cmd-pet.h"
#include "combat/attack-accuracy.h"
#include "combat/attack-criticality.h"
#include "combat/aura-counterattack.h"
#include "combat/combat-options-type.h"
#include "combat/hallucination-attacks-table.h"
#include "core/disturbance.h"
#include "core/player-update-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/geometry.h"
#include "inventory/inventory-slot-types.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "mind/mind-samurai.h"
#include "monster-attack/monster-attack-describer.h"
#include "monster-attack/monster-attack-effect.h"
#include "monster-attack/monster-attack-switcher.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-damage.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "object-hook/hook-armor.h"
#include "object/item-tester-hooker.h"
#include "pet/pet-fall-off.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/player-damage.h"
#include "player/player-skill.h"
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "system/angband.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-stun.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx 打撃を行うモンスターのID
 */
MonsterAttackPlayer::MonsterAttackPlayer(PlayerType *player_ptr, short m_idx)
    : m_idx(m_idx)
    , m_ptr(&player_ptr->current_floor_ptr->m_list[m_idx])
    , method(RaceBlowMethodType::NONE)
    , effect(RaceBlowEffectType::NONE)
    , do_silly_attack(one_in_(2) && player_ptr->hallucinated)
    , player_ptr(player_ptr)
{
}

/*!
 * @brief モンスターからプレイヤーへの打撃処理 / Attack the player via physical attacks.
 */
void MonsterAttackPlayer::make_attack_normal()
{
    if (!this->check_no_blow()) {
        return;
    }

    auto *r_ptr = &r_info[this->m_ptr->r_idx];
    this->rlev = ((r_ptr->level >= 1) ? r_ptr->level : 1);
    monster_desc(this->player_ptr, this->m_name, this->m_ptr, 0);
    monster_desc(this->player_ptr, this->ddesc, this->m_ptr, MD_WRONGDOER_NAME);
    if (PlayerClass(this->player_ptr).samurai_stance_is(SamuraiStanceType::IAI)) {
        msg_format(_("相手が襲いかかる前に素早く武器を振るった。", "You took sen, drew and cut in one motion before %s moved."), this->m_name);
        if (do_cmd_attack(this->player_ptr, this->m_ptr->fy, this->m_ptr->fx, HISSATSU_IAI)) {
            return;
        }
    }

    auto can_activate_kawarimi = randint0(55) < (this->player_ptr->lev * 3 / 5 + 20);
    if (can_activate_kawarimi && kawarimi(this->player_ptr, true)) {
        return;
    }

    this->blinked = false;
    if (this->process_monster_blows()) {
        return;
    }

    this->postprocess_monster_blows();
}

/*!
 * @brief 能力値の実値を求める
 * @param raw PlayerTypeに格納されている生値
 * @return 実値
 * @details AD&Dの記法に則り、19以上の値を取らなくしているので、格納方法が面倒
 */
int MonsterAttackPlayer::stat_value(const int raw)
{
    if (raw <= 18) {
        return raw;
    }

    return (raw - 18) / 10 + 18;
}

bool MonsterAttackPlayer::check_no_blow()
{
    auto *r_ptr = &r_info[this->m_ptr->r_idx];
    if (r_ptr->behavior_flags.has(MonsterBehaviorType::NEVER_BLOW)) {
        return false;
    }

    if (d_info[this->player_ptr->dungeon_idx].flags.has(DungeonFeatureType::NO_MELEE)) {
        return false;
    }

    return is_hostile(this->m_ptr);
}

/*!
 * @brief モンスターからプレイヤーへの打撃処理本体
 * @return 打撃に反応してプレイヤーがその場から離脱したかどうか
 */
bool MonsterAttackPlayer::process_monster_blows()
{
    auto *r_ptr = &r_info[this->m_ptr->r_idx];
    for (auto ap_cnt = 0; ap_cnt < MAX_NUM_BLOWS; ap_cnt++) {
        this->obvious = false;
        this->damage = 0;
        this->act = nullptr;
        this->effect = r_ptr->blow[ap_cnt].effect;
        this->method = r_ptr->blow[ap_cnt].method;
        this->d_dice = r_ptr->blow[ap_cnt].d_dice;
        this->d_side = r_ptr->blow[ap_cnt].d_side;

        if (!this->check_monster_continuous_attack()) {
            break;
        }

        // effect が RaceBlowEffectType::NONE (無効値)になることはあり得ないはずだが、万一そう
        // なっていたら単に攻撃を打ち切る。
        // r_info.txt の "B:" トークンに effect 以降を書き忘れた場合が該当する。
        if (this->effect == RaceBlowEffectType::NONE) {
            plog("unexpected: MonsterAttackPlayer::effect == RaceBlowEffectType::NONE");
            break;
        }

        if (this->method == RaceBlowMethodType::SHOOT) {
            continue;
        }

        // フレーバーの打撃は必中扱い。それ以外は通常の命中判定を行う。
        this->ac = this->player_ptr->ac + this->player_ptr->to_a;
        bool hit;
        if (this->effect == RaceBlowEffectType::FLAVOR) {
            hit = true;
        } else {
            const int power = mbe_info[enum2i(this->effect)].power;
            hit = check_hit_from_monster_to_player(this->player_ptr, power, this->rlev, monster_stunned_remaining(this->m_ptr));
        }

        if (hit) {
            // 命中した。命中処理と思い出処理を行う。
            // 打撃そのものは対邪悪結界で撃退した可能性がある。
            const bool protect = !this->process_monster_attack_hit();
            this->increase_blow_type_seen(ap_cnt);

            // 撃退成功時はそのまま次の打撃へ移行。
            if (protect) {
                continue;
            }

            // 撃退失敗時は落馬処理、変わり身のテレポート処理を行う。
            check_fall_off_horse(this->player_ptr, this);

            // 変わり身のテレポートが成功したら攻撃を打ち切り、プレイヤーが離脱した旨を返す。
            if (kawarimi(this->player_ptr, false)) {
                return true;
            }
        } else {
            // 命中しなかった。回避時の処理、思い出処理を行う。
            this->process_monster_attack_evasion();
            this->increase_blow_type_seen(ap_cnt);
        }
    }

    // 通常はプレイヤーはその場にとどまる。
    return false;
}

/*!
 * @brief プレイヤー死亡等でモンスターからプレイヤーへの直接攻撃処理を途中で打ち切るかどうかを判定する
 * @return 攻撃続行ならばTRUE、打ち切りになったらFALSE
 */
bool MonsterAttackPlayer::check_monster_continuous_attack()
{
    if (!monster_is_valid(this->m_ptr) || (this->method == RaceBlowMethodType::NONE)) {
        return false;
    }

    auto *r_ptr = &r_info[this->m_ptr->r_idx];
    if (is_pet(this->m_ptr) && r_ptr->kind_flags.has(MonsterKindType::UNIQUE) && (this->method == RaceBlowMethodType::EXPLODE)) {
        this->method = RaceBlowMethodType::HIT;
        this->d_dice /= 10;
    }

    auto is_neighbor = distance(this->player_ptr->y, this->player_ptr->x, this->m_ptr->fy, this->m_ptr->fx) <= 1;
    return this->player_ptr->playing && !this->player_ptr->is_dead && is_neighbor && !this->player_ptr->leaving;
}

/*!
 * @brief モンスターから直接攻撃を1回受けた時の処理
 * @return 対邪悪結界により攻撃が当たらなかったらFALSE、それ以外はTRUE
 * @param this->player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 * @details 最大4 回/モンスター/ターン、このルーチンを通る
 */
bool MonsterAttackPlayer::process_monster_attack_hit()
{
    disturb(this->player_ptr, true, true);
    if (this->effect_protecion_from_evil()) {
        return false;
    }

    this->do_cut = 0;
    this->do_stun = 0;
    describe_monster_attack_method(this);
    this->describe_silly_attacks();
    this->obvious = true;
    this->damage = damroll(this->d_dice, this->d_side);
    if (this->explode) {
        this->damage = 0;
    }

    switch_monster_blow_to_player(this->player_ptr, this);
    this->select_cut_stun();
    this->calc_player_cut();
    this->process_player_stun();
    this->monster_explode();
    process_aura_counterattack(this->player_ptr, this);
    return true;
}

/*!
 * @brief 対邪悪結界が効いている状態で邪悪なモンスターから直接攻撃を受けた時の処理
 * @return briefに書いた条件＋確率が満たされたらTRUE、それ以外はFALSE
 */
bool MonsterAttackPlayer::effect_protecion_from_evil()
{
    auto *r_ptr = &r_info[this->m_ptr->r_idx];
    if ((this->player_ptr->protevil <= 0) || r_ptr->kind_flags.has_not(MonsterKindType::EVIL) || (this->player_ptr->lev < this->rlev) || ((randint0(100) + this->player_ptr->lev) <= 50)) {
        return false;
    }

    if (is_original_ap_and_seen(this->player_ptr, this->m_ptr)) {
        r_ptr->r_kind_flags.set(MonsterKindType::EVIL);
    }

#ifdef JP
    this->abbreviate ? msg_format("撃退した。") : msg_format("%^sは撃退された。", this->m_name);
    this->abbreviate = 1; /* 2回目以降は省略 */
#else
    msg_format("%^s is repelled.", this->m_name);
#endif

    return true;
}

void MonsterAttackPlayer::describe_silly_attacks()
{
    if (this->act == nullptr) {
        return;
    }

    if (this->do_silly_attack) {
#ifdef JP
        this->abbreviate = -1;
#endif
        this->act = silly_attacks[randint0(MAX_SILLY_ATTACK)];
    }

#ifdef JP
    if (this->abbreviate == 0) {
        msg_format("%^sに%s", this->m_name, this->act);
    } else if (this->abbreviate == 1) {
        msg_format("%s", this->act);
    } else {
        /* if (this->abbreviate == -1) */
        msg_format("%^s%s", this->m_name, this->act);
    }

    this->abbreviate = 1; /*2回目以降は省略 */
#else
    msg_format("%^s %s%s", this->m_name, this->act, this->do_silly_attack ? " you." : "");
#endif
}

/*!
 * @brief 切り傷と朦朧が同時に発生した時、片方を無効にする
 */
void MonsterAttackPlayer::select_cut_stun()
{
    if ((this->do_cut == 0) || (this->do_stun == 0)) {
        return;
    }

    if (randint0(100) < 50) {
        this->do_cut = 0;
    } else {
        this->do_stun = 0;
    }
}

void MonsterAttackPlayer::calc_player_cut()
{
    if (this->do_cut == 0) {
        return;
    }

    auto cut_plus = PlayerCut::get_accumulation(this->d_dice * this->d_side, this->damage);
    if (cut_plus > 0) {
        (void)BadStatusSetter(this->player_ptr).mod_cut(cut_plus);
    }
}

/*!
 * @brief 朦朧を蓄積させる
 * @param this->player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスター打撃への参照ポインタ
 * @details
 * 痛恨の一撃ならば朦朧蓄積ランクを1上げる.
 * 2%の確率で朦朧蓄積ランクを1上げる.
 * 肉体のパラメータが合計80を超える水準に強化されていたら朦朧蓄積ランクを1下げる.
 */
void MonsterAttackPlayer::process_player_stun()
{
    if (this->do_stun == 0) {
        return;
    }

    auto total = this->d_dice * this->d_side;
    auto accumulation_rank = PlayerStun::get_accumulation_rank(total, this->damage);
    if (accumulation_rank == 0) {
        return;
    }

    if ((total < this->damage) && (accumulation_rank <= 6)) {
        accumulation_rank++;
    }

    if (one_in_(50)) {
        accumulation_rank++;
    }

    auto str = this->stat_value(this->player_ptr->stat_cur[A_STR]);
    auto dex = this->stat_value(this->player_ptr->stat_cur[A_DEX]);
    auto con = this->stat_value(this->player_ptr->stat_cur[A_CON]);
    auto is_powerful_body = str + dex + con > 80;
    if (is_powerful_body) {
        accumulation_rank--;
    }

    auto stun_plus = PlayerStun::get_accumulation(accumulation_rank);
    if (stun_plus > 0) {
        (void)BadStatusSetter(this->player_ptr).mod_stun(stun_plus);
    }
}

void MonsterAttackPlayer::monster_explode()
{
    if (!this->explode) {
        return;
    }

    sound(SOUND_EXPLODE);
    MonsterDamageProcessor mdp(this->player_ptr, this->m_idx, this->m_ptr->hp + 1, &this->fear, AttributeType::NONE);
    if (mdp.mon_take_hit(nullptr)) {
        this->blinked = false;
        this->alive = false;
    }
}

/*!
 * @brief 一部の打撃種別の場合のみ、避けた旨のメッセージ表示と盾技能スキル向上を行う
 * @param this->player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
void MonsterAttackPlayer::process_monster_attack_evasion()
{
    switch (this->method) {
    case RaceBlowMethodType::HIT:
    case RaceBlowMethodType::TOUCH:
    case RaceBlowMethodType::PUNCH:
    case RaceBlowMethodType::KICK:
    case RaceBlowMethodType::CLAW:
    case RaceBlowMethodType::BITE:
    case RaceBlowMethodType::STING:
    case RaceBlowMethodType::SLASH:
    case RaceBlowMethodType::BUTT:
    case RaceBlowMethodType::CRUSH:
    case RaceBlowMethodType::ENGULF:
    case RaceBlowMethodType::CHARGE:
        this->describe_attack_evasion();
        this->gain_armor_exp();
        this->damage = 0;
        return;
    default:
        return;
    }
}

void MonsterAttackPlayer::describe_attack_evasion()
{
    if (!this->m_ptr->ml) {
        return;
    }

    disturb(this->player_ptr, true, true);
#ifdef JP
    auto is_suiken = any_bits(this->player_ptr->special_attack, ATTACK_SUIKEN);
    if (this->abbreviate) {
        msg_format("%sかわした。", is_suiken ? "奇妙な動きで" : "");
    } else {
        msg_format("%s%^sの攻撃をかわした。", is_suiken ? "奇妙な動きで" : "", this->m_name);
    }

    this->abbreviate = 1; /* 2回目以降は省略 */
#else
    msg_format("%^s misses you.", this->m_name);
#endif
}

void MonsterAttackPlayer::gain_armor_exp()
{
    const auto o_ptr_mh = &this->player_ptr->inventory_list[INVEN_MAIN_HAND];
    const auto o_ptr_sh = &this->player_ptr->inventory_list[INVEN_SUB_HAND];
    if (!o_ptr_mh->is_armour() && !o_ptr_sh->is_armour()) {
        return;
    }

    auto cur = this->player_ptr->skill_exp[PlayerSkillKindType::SHIELD];
    auto max = s_info[enum2i(this->player_ptr->pclass)].s_max[PlayerSkillKindType::SHIELD];
    if (cur >= max) {
        return;
    }

    auto *r_ptr = &r_info[this->m_ptr->r_idx];
    auto target_level = r_ptr->level;
    short increment = 0;
    if ((cur / 100) < target_level) {
        auto addition = (cur / 100 + 15) < target_level ? (target_level - (cur / 100 + 15)) : 0;
        increment += 1 + addition;
    }

    this->player_ptr->skill_exp[PlayerSkillKindType::SHIELD] = std::min<short>(max, cur + increment);
    this->player_ptr->update |= (PU_BONUS);
}

/*!
 * @brief モンスターの打撃情報を蓄積させる
 * @param ap_cnt モンスターの打撃 N回目
 * @details どの敵が何をしてきたか正しく認識できていなければ情報を蓄積しない.
 * 非自明な類の打撃については、そのダメージが 0 ならば基本的に知識が増えない.
 * 但し、既に一定以上の知識があれば常に知識が増える(何をされたのか察知できる).
 */
void MonsterAttackPlayer::increase_blow_type_seen(const int ap_cnt)
{
    if (!is_original_ap_and_seen(this->player_ptr, this->m_ptr) || this->do_silly_attack) {
        return;
    }

    auto *r_ptr = &r_info[this->m_ptr->r_idx];
    if (!this->obvious && (this->damage == 0) && (r_ptr->r_blows[ap_cnt] <= 10)) {
        return;
    }

    if (r_ptr->r_blows[ap_cnt] < MAX_UCHAR) {
        r_ptr->r_blows[ap_cnt]++;
    }
}

void MonsterAttackPlayer::postprocess_monster_blows()
{
    SpellHex spell_hex(this->player_ptr, this);
    spell_hex.store_vengeful_damage(this->get_damage);
    spell_hex.eyes_on_eyes();
    musou_counterattack(this->player_ptr, this);
    spell_hex.thief_teleport();
    auto *r_ptr = &r_info[this->m_ptr->r_idx];
    if (this->player_ptr->is_dead && (r_ptr->r_deaths < MAX_SHORT) && !this->player_ptr->current_floor_ptr->inside_arena) {
        r_ptr->r_deaths++;
    }

    if (this->m_ptr->ml && this->fear && this->alive && !this->player_ptr->is_dead) {
        sound(SOUND_FLEE);
        msg_format(_("%^sは恐怖で逃げ出した！", "%^s flees in terror!"), this->m_name);
    }

    PlayerClass(this->player_ptr).break_samurai_stance({ SamuraiStanceType::IAI });
}
