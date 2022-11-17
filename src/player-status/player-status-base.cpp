#include "player-status/player-status-base.h"
#include "inventory/inventory-slot-types.h"
#include "object/object-flags.h"
#include "player/player-status.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤーの各ステータス計算用のクラス
 * @param player_ptr プレイヤーの参照ポインタ
 * @details
 * * コンストラクタでplayer_ptrをセット。メンバ変数を0クリア。
 */
PlayerStatusBase::PlayerStatusBase(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
    this->set_locals(); /* 初期化。基底クラスの0クリアが呼ばれる。*/
}

/*!
 * @brief 該当する値を計算して取得する。
 * @details
 * * 派生クラスからset_locals()をコールして初期値、上限、下限をセット。
 * * 各要素毎に計算した値を初期値に単純に加算し、上限と下限で丸める。
 */
int16_t PlayerStatusBase::get_value()
{
    this->set_locals(); /* 計算前に値のセット。派生クラスの値がセットされる。*/
    int16_t pow = this->default_value;

    pow += this->action_bonus();
    pow += this->stance_bonus();
    pow += this->class_base_bonus();
    pow += this->class_bonus();
    pow += this->equipments_bonus();
    pow += this->inventory_weight_bonus();
    pow += this->mutation_bonus();
    pow += this->personality_bonus();
    pow += this->race_bonus();
    pow += this->riding_bonus();
    pow += this->time_effect_bonus();
    pow = this->set_exception_bonus(pow);

    if ((pow > this->max_value)) {
        pow = this->max_value;
    }

    if (pow < this->min_value) {
        pow = this->min_value;
    }

    return pow;
}

/*!
 * @brief 修正値が0でないところにビットを立てて返す。
 * @return 判定結果のBIT_FLAGS
 */
BIT_FLAGS PlayerStatusBase::get_all_flags()
{
    this->set_locals(); /* 計算前に値のセット。派生クラスの値がセットされる。*/
    auto flags = equipments_flags(this->tr_flag);
    if (this->class_bonus() != 0) {
        set_bits(flags, FLAG_CAUSE_CLASS);
    }

    if (this->race_bonus() != 0) {
        set_bits(flags, FLAG_CAUSE_RACE);
    }

    if (this->stance_bonus() != 0) {
        set_bits(flags, FLAG_CAUSE_STANCE);
    }

    if (this->mutation_bonus() != 0) {
        set_bits(flags, FLAG_CAUSE_MUTATION);
    }

    if (this->time_effect_bonus() != 0) {
        set_bits(flags, FLAG_CAUSE_MAGIC_TIME_EFFECT);
    }

    if (this->personality_bonus() != 0) {
        set_bits(flags, FLAG_CAUSE_PERSONALITY);
    }

    if (this->riding_bonus() != 0) {
        set_bits(flags, FLAG_CAUSE_RIDING);
    }

    if (this->inventory_weight_bonus() != 0) {
        set_bits(flags, FLAG_CAUSE_INVEN_PACK);
    }

    if (this->action_bonus() != 0) {
        set_bits(flags, FLAG_CAUSE_ACTION);
    }

    return flags;
}

/*!
 * @brief 修正値が1以上のところにビットを立てて返す。
 * @return 判定結果のBIT_FLAGS
 */
BIT_FLAGS PlayerStatusBase::get_good_flags()
{
    this->set_locals(); /* 計算前に値のセット。派生クラスの値がセットされる。*/
    auto flags = equipments_flags(this->tr_flag);
    if (this->class_bonus() > 0) {
        set_bits(flags, FLAG_CAUSE_CLASS);
    }

    if (this->race_bonus() > 0) {
        set_bits(flags, FLAG_CAUSE_RACE);
    }

    if (this->stance_bonus() > 0) {
        set_bits(flags, FLAG_CAUSE_STANCE);
    }

    if (this->mutation_bonus() > 0) {
        set_bits(flags, FLAG_CAUSE_MUTATION);
    }

    if (this->time_effect_bonus() > 0) {
        set_bits(flags, FLAG_CAUSE_MAGIC_TIME_EFFECT);
    }

    if (this->personality_bonus() > 0) {
        set_bits(flags, FLAG_CAUSE_PERSONALITY);
    }

    if (this->riding_bonus() > 0) {
        set_bits(flags, FLAG_CAUSE_RIDING);
    }

    if (this->inventory_weight_bonus() > 0) {
        set_bits(flags, FLAG_CAUSE_INVEN_PACK);
    }

    if (this->action_bonus() > 0) {
        set_bits(flags, FLAG_CAUSE_ACTION);
    }

    return flags;
}

/*!
 * @brief 修正値が-1以下のところにビットを立てて返す。
 * @return 判定結果のBIT_FLAGS
 */
BIT_FLAGS PlayerStatusBase::get_bad_flags()
{
    this->set_locals(); /* 計算前に値のセット。派生クラスの値がセットされる。*/
    auto flags = equipments_bad_flags(this->tr_bad_flag);
    if (this->class_bonus() < 0) {
        set_bits(flags, FLAG_CAUSE_CLASS);
    }

    if (this->race_bonus() < 0) {
        set_bits(flags, FLAG_CAUSE_RACE);
    }

    if (this->stance_bonus() < 0) {
        set_bits(flags, FLAG_CAUSE_STANCE);
    }

    if (this->mutation_bonus() < 0) {
        set_bits(flags, FLAG_CAUSE_MUTATION);
    }

    if (this->time_effect_bonus() < 0) {
        set_bits(flags, FLAG_CAUSE_MAGIC_TIME_EFFECT);
    }

    if (this->personality_bonus() < 0) {
        set_bits(flags, FLAG_CAUSE_PERSONALITY);
    }

    if (this->riding_bonus() < 0) {
        set_bits(flags, FLAG_CAUSE_RIDING);
    }

    if (this->inventory_weight_bonus() < 0) {
        set_bits(flags, FLAG_CAUSE_INVEN_PACK);
    }

    if (this->action_bonus() < 0) {
        set_bits(flags, FLAG_CAUSE_ACTION);
    }

    return flags;
}

void PlayerStatusBase::set_locals()
{
    this->default_value = 0;
    this->min_value = 0;
    this->max_value = 0;
    this->tr_flag = TR_FLAG_MAX;
    this->tr_bad_flag = TR_FLAG_MAX;
}

/*!
 * @brief 判定するflagを持つ装備品に対応するBIT_FLAGSを返す
 * @param check_flag 判定するtr_type
 * @return 判定結果のBIT_FLAGS
 */
BIT_FLAGS PlayerStatusBase::equipments_flags(tr_type check_flag)
{
    BIT_FLAGS flags = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx) {
            continue;
        }

        auto o_flags = object_flags(o_ptr);
        if (o_flags.has(check_flag)) {
            set_bits(flags, convert_inventory_slot_type_to_flag_cause(i2enum<inventory_slot_type>(i)));
        }
    }

    return flags;
}

/*!
 * @brief 判定するflagを持ち、pvalが負の装備品に対応するBIT_FLAGSを返す
 * @param check_flag 判定するtr_type
 * @return 判定結果のBIT_FLAGS
 */
BIT_FLAGS PlayerStatusBase::equipments_bad_flags(tr_type check_flag)
{
    BIT_FLAGS flags = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx) {
            continue;
        }

        auto o_flags = object_flags(o_ptr);
        if (o_flags.has(check_flag)) {
            if (o_ptr->pval < 0) {
                set_bits(flags, convert_inventory_slot_type_to_flag_cause(i2enum<inventory_slot_type>(i)));
            }
        }
    }

    return flags;
}

/*!
 * @brief this->tr_flagを持つ装備品のpval合計値を返す
 * @return 該当するfalgを持つ全装備のpvalの合計値
 */
int16_t PlayerStatusBase::equipments_bonus()
{
    this->set_locals(); /* 計算前に値のセット。派生クラスの値がセットされる。*/
    int16_t bonus = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        auto *o_ptr = &player_ptr->inventory_list[i];
        auto o_flags = object_flags(o_ptr);
        if (!o_ptr->k_idx) {
            continue;
        }

        if (o_flags.has(this->tr_flag)) {
            bonus += o_ptr->pval;
        }
    }

    return bonus;
}

int16_t PlayerStatusBase::race_bonus()
{
    return 0;
}

int16_t PlayerStatusBase::class_bonus()
{
    return 0;
}

int16_t PlayerStatusBase::class_base_bonus()
{
    return 0;
}

int16_t PlayerStatusBase::personality_bonus()
{
    return 0;
}

int16_t PlayerStatusBase::time_effect_bonus()
{
    return 0;
}

int16_t PlayerStatusBase::stance_bonus()
{
    return 0;
}

int16_t PlayerStatusBase::mutation_bonus()
{
    return 0;
}

int16_t PlayerStatusBase::riding_bonus()
{
    return 0;
}

int16_t PlayerStatusBase::inventory_weight_bonus()
{
    return 0;
}

int16_t PlayerStatusBase::action_bonus()
{
    return 0;
}

/*!
 * @brief 値を直接変更する例外処理。
 * @param value 単純加算された修正値の合計
 * @details
 * * 派生クラスで必要とされる例外処理でoverrideされる
 * @return 直接変更された値。このままmin-max処理され最終的なvalueになる。
 */
int16_t PlayerStatusBase::set_exception_bonus(int16_t value)
{
    return value;
}
