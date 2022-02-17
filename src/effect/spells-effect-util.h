#pragma once

#include "system/angband.h"

extern int rakubadam_m; /*!< 振り落とされた際のダメージ量 */
extern int rakubadam_p; /*!< 落馬した際のダメージ量 */

extern short cap_mon;
extern byte cap_mspeed;
extern short cap_hp;
extern short cap_maxhp;
extern STR_OFFSET cap_nickname;

extern bool sukekaku;
extern int project_length; /*!< 投射の射程距離 */

extern int project_m_n; /*!< 魔法効果範囲内にいるモンスターの数 */
extern POSITION project_m_x; /*!< 処理中のモンスターX座標 */
extern POSITION project_m_y; /*!< 処理中のモンスターY座標 */
extern POSITION monster_target_x; /*!< モンスターの攻撃目標X座標 */
extern POSITION monster_target_y; /*!< モンスターの攻撃目標Y座標 */
