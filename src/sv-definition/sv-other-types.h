/*!
 * @brief enumで表現することが却って高コストになりそうなSV (ベースアイテムのサブタイプ)定義をここに格納する
 * @date 2020/05/28
 * @author Hourier
 */

#pragma once

constexpr auto SV_PHOTO = 50;

/* The "sval" codes for TV_CORPSE */
constexpr auto SV_SKELETON = 0;
constexpr auto SV_CORPSE = 1;

/* The "sval" codes for TV_SHOT/TV_ARROW/TV_BOLT */
constexpr auto SV_AMMO_LIGHT = 0; /* pebbles */
constexpr auto SV_AMMO_NORMAL = 1; /* shots, arrows, bolts */

constexpr auto SV_CHEST_KANDUME = 50; /*!< 箱アイテムの小項目ID: おもちゃのカンヅメ */

/* The "sval" codes for TV_FLASK */
constexpr int SV_FLASK_OIL = 0;
