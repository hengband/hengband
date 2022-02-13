#pragma once

#include "system/angband.h"

struct stat_bar {
    TERM_COLOR attr;
    concptr sstr;
    concptr lstr;
};

/* 画面下部に表示する状態表示定義ID / Data structure for status bar */
enum bar_definition_type {
    BAR_TSUYOSHI = 0, /*!< 下部ステータス表示: オクレ兄さん状態 */
    BAR_HALLUCINATION = 1, /*!< 下部ステータス表示: 幻覚 */
    BAR_BLINDNESS = 2, /*!< 下部ステータス表示: 盲目 */
    BAR_PARALYZE = 3, /*!< 下部ステータス表示: 麻痺 */
    BAR_CONFUSE = 4, /*!< 下部ステータス表示: 混乱 */
    BAR_POISONED = 5, /*!< 下部ステータス表示: 毒 */
    BAR_AFRAID = 6, /*!< 下部ステータス表示: 恐怖 */
    BAR_LEVITATE = 7, /*!< 下部ステータス表示: 浮遊 */
    BAR_REFLECTION = 8, /*!< 下部ステータス表示: 反射 */
    BAR_PASSWALL = 9, /*!< 下部ステータス表示: 壁抜け */
    BAR_WRAITH = 10, /*!< 下部ステータス表示: 幽体化 */
    BAR_PROTEVIL = 11, /*!< 下部ステータス表示: 対邪悪結界 */
    BAR_KAWARIMI = 12, /*!< 下部ステータス表示: 変わり身 */
    BAR_MAGICDEFENSE = 13, /*!< 下部ステータス表示: 魔法の鎧 */
    BAR_EXPAND = 14, /*!< 下部ステータス表示: 横伸び */
    BAR_STONESKIN = 15, /*!< 下部ステータス表示: 石肌化 */
    BAR_MULTISHADOW = 16, /*!< 下部ステータス表示: 影分身 */
    BAR_REGMAGIC = 17, /*!< 下部ステータス表示: 魔法防御 */
    BAR_ULTIMATE = 18, /*!< 下部ステータス表示: 究極の耐性 */
    BAR_INVULN = 19, /*!< 下部ステータス表示: 無敵化 */
    BAR_IMMACID = 20, /*!< 下部ステータス表示: 酸免疫 */
    BAR_RESACID = 21, /*!< 下部ステータス表示: 酸耐性 */
    BAR_IMMELEC = 22, /*!< 下部ステータス表示: 電撃免疫 */
    BAR_RESELEC = 23, /*!< 下部ステータス表示: 電撃耐性 */
    BAR_IMMFIRE = 24, /*!< 下部ステータス表示: 火炎免疫 */
    BAR_RESFIRE = 25, /*!< 下部ステータス表示: 火炎耐性 */
    BAR_IMMCOLD = 26, /*!< 下部ステータス表示: 冷気免疫 */
    BAR_RESCOLD = 27, /*!< 下部ステータス表示: 冷気耐性 */
    BAR_RESPOIS = 28, /*!< 下部ステータス表示: 毒耐性 */
    BAR_RESNETH = 29, /*!< 下部ステータス表示: 地獄耐性 */
    BAR_RESTIME = 30, /*!< 下部ステータス表示: 時間逆転耐性 */
    BAR_DUSTROBE = 31, /*!< 下部ステータス表示: 破片オーラ */
    BAR_SHFIRE = 32, /*!< 下部ステータス表示: 火炎オーラ */
    BAR_TOUKI = 33, /*!< 下部ステータス表示: 闘気 */
    BAR_SHHOLY = 34, /*!< 下部ステータス表示: 聖なるオーラ */
    BAR_EYEEYE = 35, /*!< 下部ステータス表示: 目には目を */
    BAR_BLESSED = 36, /*!< 下部ステータス表示: 祝福 */
    BAR_HEROISM = 37, /*!< 下部ステータス表示: 士気高揚 */
    BAR_BERSERK = 38, /*!< 下部ステータス表示: 狂戦士化 */
    BAR_ATTKFIRE = 39, /*!< 下部ステータス表示: 焼棄スレイ */
    BAR_ATTKCOLD = 40, /*!< 下部ステータス表示: 冷凍スレイ */
    BAR_ATTKELEC = 41, /*!< 下部ステータス表示: 電撃スレイ */
    BAR_ATTKACID = 42, /*!< 下部ステータス表示: 溶解スレイ */
    BAR_ATTKPOIS = 43, /*!< 下部ステータス表示: 毒殺スレイ */
    BAR_ATTKCONF = 44, /*!< 下部ステータス表示: 混乱打撃 */
    BAR_SENSEUNSEEN = 45, /*!< 下部ステータス表示: 透明視 */
    BAR_TELEPATHY = 46, /*!< 下部ステータス表示: テレパシー */
    BAR_REGENERATION = 47, /*!< 下部ステータス表示: 急回復 */
    BAR_INFRAVISION = 48, /*!< 下部ステータス表示: 赤外線視力 */
    BAR_STEALTH = 49, /*!< 下部ステータス表示: 隠密 */
    BAR_SUPERSTEALTH = 50, /*!< 下部ステータス表示: 超隠密 */
    BAR_RECALL = 51, /*!< 下部ステータス表示: 帰還待ち */
    BAR_ALTER = 52, /*!< 下部ステータス表示: 現実変容待ち */
    BAR_SHCOLD = 53, /*!< 下部ステータス表示: 冷気オーラ */
    BAR_SHELEC = 54, /*!< 下部ステータス表示: 電撃オーラ */
    BAR_SHSHADOW = 55, /*!< 下部ステータス表示: 影のオーラ */
    BAR_MIGHT = 56, /*!< 下部ステータス表示: 腕力強化 */
    BAR_BUILD = 57, /*!< 下部ステータス表示: 肉体強化 */
    BAR_ANTIMULTI = 58, /*!< 下部ステータス表示: 反増殖 */
    BAR_ANTITELE = 59, /*!< 下部ステータス表示: 反テレポート */
    BAR_ANTIMAGIC = 60, /*!< 下部ステータス表示: 反魔法 */
    BAR_PATIENCE = 61, /*!< 下部ステータス表示: 我慢 */
    BAR_REVENGE = 62, /*!< 下部ステータス表示: 宣告 */
    BAR_RUNESWORD = 63, /*!< 下部ステータス表示: 魔剣化 */
    BAR_VAMPILIC = 64, /*!< 下部ステータス表示: 吸血 */
    BAR_CURE = 65, /*!< 下部ステータス表示: 回復 */
    BAR_ESP_EVIL = 66, /*!< 下部ステータス表示: 邪悪感知 */
    BAR_NIGHTSIGHT = 67, /*!< 下部ステータス表示: 暗視 */
};

#define MAX_STAT_BARS 69

extern stat_bar stat_bars[MAX_STAT_BARS];
