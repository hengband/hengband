#include "wizard/artifact-bias-table.h"
#include "artifact/random-art-bias-types.h"
#include "locale/language-switcher.h"

/*!
 * @brief ランダムアーティファクトのバイアス名称テーブル
 */
const std::map<RandomArtifactBias, std::string> ARTIFACT_BIAS_NAMES = {
    { RandomArtifactBias::NONE, _("なし", "None") },
    { RandomArtifactBias::ELEC, _("電撃", "Elec") },
    { RandomArtifactBias::POIS, _("毒", "Poison") },
    { RandomArtifactBias::FIRE, _("火炎", "Fire") },
    { RandomArtifactBias::COLD, _("冷気", "Cold") },
    { RandomArtifactBias::ACID, _("酸", "Acid") },
    { RandomArtifactBias::STR, _("腕力", "STR") },
    { RandomArtifactBias::INT, _("知力", "INT") },
    { RandomArtifactBias::WIS, _("賢さ", "WIS") },
    { RandomArtifactBias::DEX, _("器用さ", "DEX") },
    { RandomArtifactBias::CON, _("耐久", "CON") },
    { RandomArtifactBias::CHR, _("魅力", "CHA") },
    { RandomArtifactBias::CHAOS, _("混沌", "Chaos") },
    { RandomArtifactBias::PRIESTLY, _("プリースト", "Priestly") },
    { RandomArtifactBias::NECROMANTIC, _("死霊", "Necromantic") },
    { RandomArtifactBias::LAW, _("法", "Law") },
    { RandomArtifactBias::ROGUE, _("盗賊", "Rogue") },
    { RandomArtifactBias::MAGE, _("メイジ", "Mage") },
    { RandomArtifactBias::WARRIOR, _("戦士", "Warrior") },
    { RandomArtifactBias::RANGER, _("レンジャー", "Ranger") },
};
