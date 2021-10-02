#pragma once

enum special_defence {
	DEFENSE_ACID = 0x00000001, /*!< プレイヤーのステータス:酸免疫 */
    DEFENSE_ELEC = 0x00000002, /*!< プレイヤーのステータス:電撃免疫 */
    DEFENSE_FIRE = 0x00000004, /*!< プレイヤーのステータス:火炎免疫 */
    DEFENSE_COLD = 0x00000008, /*!< プレイヤーのステータス:冷気免疫 */
    DEFENSE_POIS = 0x00000010, /*!< プレイヤーのステータス:毒免疫 */
    KAMAE_GENBU = 0x00000020, /*!< プレイヤーのステータス:玄武の構え */
    KAMAE_BYAKKO = 0x00000040, /*!< プレイヤーのステータス:白虎の構え */
    KAMAE_SEIRYU = 0x00000080, /*!< プレイヤーのステータス:青竜の構え */
    KAMAE_SUZAKU = 0x00000100, /*!< プレイヤーのステータス:朱雀の構え */
    NINJA_KAWARIMI = 0x00002000, /*!< プレイヤーのステータス:変わり身 */
    NINJA_S_STEALTH = 0x00004000, /*!< プレイヤーのステータス:超隠密 */
};

#define MAX_KAMAE 4 /*!< 修行僧の構え最大数 */
#define KAMAE_MASK (KAMAE_GENBU | KAMAE_BYAKKO | KAMAE_SEIRYU | KAMAE_SUZAKU) /*!< 修行僧の構えビット配列 */
