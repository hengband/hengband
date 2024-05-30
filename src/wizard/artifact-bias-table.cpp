#include "wizard/artifact-bias-table.h"
#include "artifact/random-art-bias-types.h"
#include "locale/language-switcher.h"

/*!
 * @brief ランダムアーティファクトのバイアス名称テーブル
 */
const std::map<random_art_bias_type, std::string> ARTIFACT_BIAS_NAMES = {
    { BIAS_NONE, _("なし", "None") },
    { BIAS_ELEC, _("電撃", "Elec") },
    { BIAS_POIS, _("毒", "Poison") },
    { BIAS_FIRE, _("火炎", "Fire") },
    { BIAS_COLD, _("冷気", "Cold") },
    { BIAS_ACID, _("酸", "Acid") },
    { BIAS_STR, _("腕力", "STR") },
    { BIAS_INT, _("知力", "INT") },
    { BIAS_WIS, _("賢さ", "WIS") },
    { BIAS_DEX, _("器用さ", "DEX") },
    { BIAS_CON, _("耐久", "CON") },
    { BIAS_CHR, _("魅力", "CHA") },
    { BIAS_CHAOS, _("混沌", "Chaos") },
    { BIAS_PRIESTLY, _("プリースト", "Priestly") },
    { BIAS_NECROMANTIC, _("死霊", "Necromantic") },
    { BIAS_LAW, _("法", "Law") },
    { BIAS_ROGUE, _("盗賊", "Rogue") },
    { BIAS_MAGE, _("メイジ", "Mage") },
    { BIAS_WARRIOR, _("戦士", "Warrior") },
    { BIAS_RANGER, _("レンジャー", "Ranger") },
};
