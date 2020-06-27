#pragma once

typedef enum special_defence {
	DEFENSE_ACID = 0x00000001, /*!< プレイヤーのステータス:酸免疫 */
    DEFENSE_ELEC = 0x00000002, /*!< プレイヤーのステータス:電撃免疫 */
    DEFENSE_FIRE = 0x00000004, /*!< プレイヤーのステータス:火炎免疫 */
    DEFENSE_COLD = 0x00000008, /*!< プレイヤーのステータス:冷気免疫 */
    DEFENSE_POIS = 0x00000010, /*!< プレイヤーのステータス:毒免疫 */
    KAMAE_GENBU = 0x00000020, /*!< プレイヤーのステータス:玄武の構え */
    KAMAE_BYAKKO = 0x00000040, /*!< プレイヤーのステータス:白虎の構え */
    KAMAE_SEIRYU = 0x00000080, /*!< プレイヤーのステータス:青竜の構え */
    KAMAE_SUZAKU = 0x00000100, /*!< プレイヤーのステータス:朱雀の構え */
    KATA_IAI = 0x00000200, /*!< プレイヤーのステータス:居合 */
    KATA_FUUJIN = 0x00000400, /*!< プレイヤーのステータス:風塵 */
    KATA_KOUKIJIN = 0x00000800, /*!< プレイヤーのステータス:降鬼陣 */
    KATA_MUSOU = 0x00001000, /*!< プレイヤーのステータス:無想 */
    NINJA_KAWARIMI = 0x00002000, /*!< プレイヤーのステータス:変わり身 */
    NINJA_S_STEALTH = 0x00004000, /*!< プレイヤーのステータス:超隠密 */
} special_defence;

#define MAX_KAMAE 4 /*!< 修行僧の構え最大数 */
#define KAMAE_MASK (KAMAE_GENBU | KAMAE_BYAKKO | KAMAE_SEIRYU | KAMAE_SUZAKU) /*!< 修行僧の構えビット配列 */

#define MAX_KATA 4 /*!< 修行僧の型最大数 */
#define KATA_MASK (KATA_IAI | KATA_FUUJIN | KATA_KOUKIJIN | KATA_MUSOU) /*!< 修行僧の型ビット配列 */
