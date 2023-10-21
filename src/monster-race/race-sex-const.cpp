#include "monster-race/race-sex-const.h"
#include "system/monster-race-info.h"

/*!
 * @brief 性別が男性を含むか否かを判定
 * @param 性別の変数
 * @return 男性を含むか否か
 */
bool is_male(const MonsterSex sex)
{
    return sex == MonsterSex::MALE;
}

/*!
 * @brief 性別が男性を含むか否かを判定
 * @param 判定するモンスターの参照
 * @return 男性を含むか否か
 */
bool is_male(const MonsterRaceInfo &monrace)
{
    return is_male(monrace.sex);
}

/*!
 * @brief 性別が女性を含むか否かを判定
 * @param 性別の変数
 * @return 女性を含むか否か
 */
bool is_female(const MonsterSex sex)
{
    return sex == MonsterSex::FEMALE;
}

/*!
 *@brief 性別が女性を含むか否かを判定
 * @param 判定するモンスターの参照
 *@return 女性を含むか否か
 */
bool is_female(const MonsterRaceInfo &monrace)
{
    return is_female(monrace.sex);
}
