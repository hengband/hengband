#include "system/baseitem/baseitem-record.h"

/*!
 * @brief 未鑑定名があるか否かを返す
 *
 * 薬、ロッド、杖、魔法棒、巻物、指輪、アミュレットが該当するが、このクラスからはベースアイテム種別が分からないのでこのメソッドで判定する.
 * @return 未鑑定名の有無
 */
bool BaseitemRecord::is_apparent() const
{
    return this->appearance_id > 0;
}

short BaseitemRecord::get_appearance_id() const
{
    return this->appearance_id;
}

void BaseitemRecord::set_appearance_id(short new_appearance_id)
{
    this->appearance_id = new_appearance_id;
}

bool BaseitemRecord::is_tried() const
{
    return this->tried;
}

/*!
 * @brief 試行状態を変える
 * @param state trueなら試行済、falseなら未試行に変える
 */
void BaseitemRecord::mark_trial(bool state)
{
    this->tried = state;
}

bool BaseitemRecord::is_aware() const
{
    return this->aware;
}

/*!
 * @brief 鑑定状態を変える
 * @param state trueなら鑑定済、falseなら未鑑定に変える
 */
void BaseitemRecord::mark_awareness(bool state)
{
    this->aware = state;
}
