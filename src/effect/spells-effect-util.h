#pragma once

#include "system/angband.h"
#include <string>

extern int rakubadam_m; /*!< 振り落とされた際のダメージ量 */
extern int rakubadam_p; /*!< 落馬した際のダメージ量 */

enum class MonsterRaceId : int16_t;

class CapturedMonsterType {
public:
    CapturedMonsterType();
    MonsterRaceId r_idx;
    byte speed = STANDARD_SPEED;
    short current_hp = 0;
    short max_hp = 0;
    std::string nickname = "";
};

extern bool sukekaku;
extern int project_length; /*!< 投射の射程距離 */

extern int project_m_n; /*!< 魔法効果範囲内にいるモンスターの数 */
extern POSITION project_m_x; /*!< 処理中のモンスターX座標 */
extern POSITION project_m_y; /*!< 処理中のモンスターY座標 */
extern POSITION monster_target_x; /*!< モンスターの攻撃目標X座標 */
extern POSITION monster_target_y; /*!< モンスターの攻撃目標Y座標 */
