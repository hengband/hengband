#include "player-status/player-status-base.h"
#include "inventory/inventory-slot-types.h"
#include "object/object-flags.h"
#include "player/player-status.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief プレイヤーの各ステータス計算用のクラス
 * @param owner_ptr プレイヤーの参照ポインタ
 * @details
 * * コンストラクタでowner_ptrをセット。メンバ変数を0クリア。
 */
PlayerStatusBase::PlayerStatusBase(player_type *owner_ptr)
{
    this->owner_ptr = owner_ptr;
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

    pow += this->action_value();
    pow += this->battleform_value();
    pow += this->class_base_value();
    pow += this->class_value();
    pow += this->equipments_value();
    pow += this->inventory_weight_value();
    pow += this->mutation_value();
    pow += this->personality_value();
    pow += this->race_value();
    pow += this->riding_value();
    pow += this->time_effect_value();
    pow = this->set_exception_value(pow);

    if ((pow > this->max_value)) {
        pow = this->max_value;
    }

    if (pow < this->min_value)
        pow = this->min_value;

    return pow;
}

/*!
 * @brief 修正値が0でないところにビットを立てて返す。
 * @return 判定結果のBIT_FLAGS
 */
BIT_FLAGS PlayerStatusBase::get_all_flags()
{
    this->set_locals(); /* 計算前に値のセット。派生クラスの値がセットされる。*/
    BIT_FLAGS result = equipments_flags(this->tr_flag);

    if (this->class_value() != 0)
        set_bits(result, FLAG_CAUSE_CLASS);

    if (this->race_value() != 0)
        set_bits(result, FLAG_CAUSE_RACE);

    if (this->battleform_value() != 0)
        set_bits(result, FLAG_CAUSE_BATTLE_FORM);

    if (this->mutation_value() != 0)
        set_bits(result, FLAG_CAUSE_MUTATION);

    if (this->time_effect_value() != 0)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    if (this->personality_value() != 0)
        set_bits(result, FLAG_CAUSE_PERSONALITY);

    if (this->riding_value() != 0)
        set_bits(result, FLAG_CAUSE_RIDING);

    if (this->inventory_weight_value() != 0)
        set_bits(result, FLAG_CAUSE_INVEN_PACK);

    if (this->action_value() != 0)
        set_bits(result, FLAG_CAUSE_ACTION);

    return result;
}

/*!
 * @brief 修正値が1以上のところにビットを立てて返す。
 * @return 判定結果のBIT_FLAGS
 */
BIT_FLAGS PlayerStatusBase::get_good_flags()
{
    this->set_locals(); /* 計算前に値のセット。派生クラスの値がセットされる。*/
    BIT_FLAGS result = equipments_flags(this->tr_flag);

    if (this->class_value() > 0)
        set_bits(result, FLAG_CAUSE_CLASS);

    if (this->race_value() > 0)
        set_bits(result, FLAG_CAUSE_RACE);

    if (this->battleform_value() > 0)
        set_bits(result, FLAG_CAUSE_BATTLE_FORM);

    if (this->mutation_value() > 0)
        set_bits(result, FLAG_CAUSE_MUTATION);

    if (this->time_effect_value() > 0)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    if (this->personality_value() > 0)
        set_bits(result, FLAG_CAUSE_PERSONALITY);

    if (this->riding_value() > 0)
        set_bits(result, FLAG_CAUSE_RIDING);

    if (this->inventory_weight_value() > 0)
        set_bits(result, FLAG_CAUSE_INVEN_PACK);

    if (this->action_value() > 0)
        set_bits(result, FLAG_CAUSE_ACTION);

    return result;
}

/*!
 * @brief 修正値が-1以下のところにビットを立てて返す。
 * @return 判定結果のBIT_FLAGS
 */
BIT_FLAGS PlayerStatusBase::get_bad_flags()
{
    this->set_locals(); /* 計算前に値のセット。派生クラスの値がセットされる。*/
    BIT_FLAGS result = equipments_bad_flags(this->tr_bad_flag);

    if (this->class_value() < 0)
        set_bits(result, FLAG_CAUSE_CLASS);

    if (this->race_value() < 0)
        set_bits(result, FLAG_CAUSE_RACE);

    if (this->battleform_value() < 0)
        set_bits(result, FLAG_CAUSE_BATTLE_FORM);

    if (this->mutation_value() < 0)
        set_bits(result, FLAG_CAUSE_MUTATION);

    if (this->time_effect_value() < 0)
        set_bits(result, FLAG_CAUSE_MAGIC_TIME_EFFECT);

    if (this->personality_value() < 0)
        set_bits(result, FLAG_CAUSE_PERSONALITY);

    if (this->riding_value() < 0)
        set_bits(result, FLAG_CAUSE_RIDING);

    if (this->inventory_weight_value() < 0)
        set_bits(result, FLAG_CAUSE_INVEN_PACK);

    if (this->action_value() < 0)
        set_bits(result, FLAG_CAUSE_ACTION);

    return result;
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
    object_type *o_ptr;
    TrFlags flgs;
    BIT_FLAGS result = 0L;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &owner_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(owner_ptr, o_ptr, flgs);

        if (has_flag(flgs, check_flag))
            set_bits(result, convert_inventory_slot_type_to_flag_cause(static_cast<inventory_slot_type>(i)));
    }
    return result;
}

/*!
 * @brief 判定するflagを持ち、pvalが負の装備品に対応するBIT_FLAGSを返す
 * @param check_flag 判定するtr_type
 * @return 判定結果のBIT_FLAGS
 */
BIT_FLAGS PlayerStatusBase::equipments_bad_flags(tr_type check_flag)
{
    object_type *o_ptr;
    TrFlags flgs;
    BIT_FLAGS result = 0L;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        o_ptr = &owner_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(owner_ptr, o_ptr, flgs);

        if (has_flag(flgs, check_flag)) {
            if (o_ptr->pval < 0) {
                set_bits(result, convert_inventory_slot_type_to_flag_cause(static_cast<inventory_slot_type>(i)));
            }
        }
    }
    return result;
}

/*!
 * @brief this->tr_flagを持つ装備品のpval合計値を返す
 * @return 該当するfalgを持つ全装備のpvalの合計値
 */
int16_t PlayerStatusBase::equipments_value()
{
    this->set_locals(); /* 計算前に値のセット。派生クラスの値がセットされる。*/
    int16_t result = 0;
    for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &owner_ptr->inventory_list[i];
        TrFlags flgs;
        object_flags(owner_ptr, o_ptr, flgs);

        if (!o_ptr->k_idx)
            continue;
        if (has_flag(flgs, this->tr_flag))
            result += o_ptr->pval;
    }
    return result;
}

int16_t PlayerStatusBase::race_value()
{
    return 0;
}
int16_t PlayerStatusBase::class_value()
{
    return 0;
}
int16_t PlayerStatusBase::class_base_value()
{
    return 0;
}
int16_t PlayerStatusBase::personality_value()
{
    return 0;
}
int16_t PlayerStatusBase::time_effect_value()
{
    return 0;
}
int16_t PlayerStatusBase::battleform_value()
{
    return 0;
}
int16_t PlayerStatusBase::mutation_value()
{
    return 0;
}
int16_t PlayerStatusBase::riding_value()
{
    return 0;
}
int16_t PlayerStatusBase::inventory_weight_value()
{
    return 0;
}
int16_t PlayerStatusBase::action_value()
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
int16_t PlayerStatusBase::set_exception_value(int16_t value)
{
    return value;
}
