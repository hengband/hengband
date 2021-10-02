#pragma once

enum special_defence {
	DEFENSE_ACID = 0x00000001, /*!< プレイヤーのステータス:酸免疫 */
    DEFENSE_ELEC = 0x00000002, /*!< プレイヤーのステータス:電撃免疫 */
    DEFENSE_FIRE = 0x00000004, /*!< プレイヤーのステータス:火炎免疫 */
    DEFENSE_COLD = 0x00000008, /*!< プレイヤーのステータス:冷気免疫 */
    DEFENSE_POIS = 0x00000010, /*!< プレイヤーのステータス:毒免疫 */
    NINJA_KAWARIMI = 0x00002000, /*!< プレイヤーのステータス:変わり身 */
    NINJA_S_STEALTH = 0x00004000, /*!< プレイヤーのステータス:超隠密 */
};
