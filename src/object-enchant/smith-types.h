#pragma once

#include "system/system-variables.h"

/**
 * @brief アイテムに付与できる鍛冶効果の列挙体
 */
enum class SmithEffect : int16_t {
    NONE = 0,
    STR = 1, //!< 腕力
    INT = 2, //!< 知能
    WIS = 3, //!< 賢さ
    DEX = 4, //!< 器用さ
    CON = 5, //!< 耐久力
    CHR = 6, //!< 魅力

    SUST_STR = 10, //!< 腕力維持
    SUST_INT = 11, //!< 知能維持
    SUST_WIS = 12, //!< 賢さ維持
    SUST_DEX = 13, //!< 器用維持
    SUST_CON = 14, //!< 耐久力維持
    SUST_CHR = 15, //!< 魅力維持

    MAGIC_MASTERY = 100, //!< 魔法道具支配
    STEALTH = 101, //!< 隠密
    SEARCH = 102, //!< 探索
    INFRA = 103, //!< 赤外線視力
    TUNNEL = 104, //!< 採掘
    SPEED = 105, //!< スピード
    BLOWS = 106, //!< 追加攻撃

    CHAOTIC = 200, //!< カオス攻撃
    VAMPIRIC = 201, //!< 吸血攻撃
    EARTHQUAKE = 202, //!< 地震発動
    BRAND_POIS = 203, //!< 毒殺
    BRAND_ACID = 204, //!< 溶解
    BRAND_ELEC = 205, //!< 電撃
    BRAND_FIRE = 206, //!< 焼棄
    BRAND_COLD = 207, //!< 凍結
    VORPAL = 208, //!< 切れ味
    XTRA_MIGHT = 209, //!< 強力射
    XTRA_SHOTS = 210, //!< 強速射
    BRAND_MAGIC = 211, //!< 魔術属性攻撃

    IM_ACID = 300, //!< 酸免疫
    IM_ELEC = 301, //!< 電撃免疫
    IM_FIRE = 302, //!< 火炎免疫
    IM_COLD = 303, //!< 冷気免疫
    REFLECT = 304, //!< 反射

    RES_ACID = 400, //!< 耐酸
    RES_ELEC = 401, //!< 耐電撃
    RES_FIRE = 402, //!< 耐火炎
    RES_COLD = 403, //!< 耐冷気
    RES_POIS = 404, //!< 耐毒
    RES_FEAR = 405, //!< 耐恐怖
    RES_LITE = 406, //!< 耐閃光
    RES_DARK = 407, //!< 耐暗黒
    RES_BLIND = 408, //!< 耐盲目
    RES_CONF = 409, //!< 耐混乱
    RES_SOUND = 410, //!< 耐轟音
    RES_SHARDS = 411, //!< 耐破片
    RES_NETHER = 412, //!< 耐地獄
    RES_NEXUS = 413, //!< 耐因果混乱
    RES_CHAOS = 414, //!< 耐カオス
    RES_DISEN = 415, //!< 耐劣化
    RES_WATER = 416, //!< 耐水
    RES_TIME = 417, //!< 耐時間逆転
    RES_CURSE = 418, //!< 耐呪力

    HOLD_EXP = 500, //!< 経験値維持
    FREE_ACT = 501, //!< 麻痺知らず
    WARNING = 502, //!< 警告
    LEVITATION = 503, //!< 浮遊
    SEE_INVIS = 504, //!< 可視透明
    SLOW_DIGEST = 505, //!< 遅消化
    REGEN = 506, //!< 急速回復
    TELEPORT = 507, //!< テレポート
    NO_MAGIC = 508, //!< 反魔法
    LITE = 509, //!< 永久光源
    NO_TELE = 510, //!< 反テレポート

    SLAY_EVIL = 600, //!< 邪悪倍打
    SLAY_ANIMAL = 601, //!< 動物倍打
    SLAY_UNDEAD = 602, //!< 不死倍打
    SLAY_DEMON = 603, //!< 悪魔倍打
    SLAY_ORC = 604, //!< オーク倍打
    SLAY_TROLL = 605, //!< トロル倍打
    SLAY_GIANT = 606, //!< 巨人倍打
    SLAY_DRAGON = 607, //!< 竜倍打
    SLAY_HUMAN = 608, //!< 人間倍打
    SLAY_GOOD = 609, //!< 善良倍打

    KILL_EVIL = 700, //!< 邪悪倍倍打
    KILL_ANIMAL = 701, //!< 動物倍倍打
    KILL_UNDEAD = 702, //!< 不死倍倍打
    KILL_DEMON = 703, //!< 悪魔倍倍打
    KILL_ORC = 704, //!< オーク倍倍打
    KILL_TROLL = 705, //!< トロル倍倍打
    KILL_GIANT = 706, //!< 巨人倍倍打
    KILL_DRAGON = 707, //!< 竜倍倍打
    KILL_HUMAN = 708, //!< 人間倍倍打
    KILL_GOOD = 709, //!< 善良倍倍打

    TELEPATHY = 800, //!< テレパシー
    ESP_ANIMAL = 801, //!< 動物ESP
    ESP_UNDEAD = 802, //!< 不死ESP
    ESP_DEMON = 803, //!< 悪魔ESP
    ESP_ORC = 804, //!< オークESP
    ESP_TROLL = 805, //!< トロルESP
    ESP_GIANT = 806, //!< 巨人ESP
    ESP_DRAGON = 807, //!< 竜ESP
    ESP_HUMAN = 808, //!< 人間ESP
    ESP_GOOD = 809, //!< 善良ESP
    ESP_UNIQUE = 810, //!< ユニークESP

    SH_FIRE = 900, //!< 火炎オーラ
    SH_ELEC = 901, //!< 電撃オーラ
    SH_COLD = 902, //!< 冷気オーラ

    RESISTANCE = 1000, //!< 全耐性
    SLAY_GLOVE = 1001, //!< 殺戮の小手
    EASY_2WEAPON = 1002, //!< 二刀流

    SAVE_EFFECT_MAX = 8192, //!< 鍛冶師の銘付きアイテムに付与された鍛冶効果の保存領域は2バイト。とりあえず余裕を持って8192としておく。

    //! @note これ以降は鍛冶師の銘付きアイテムになる鍛冶効果ではないので、効果番号を保存しなくてよい。
    //  鍛冶効果のリストを管理するためだけのものである。したがって SAVE_EFFECT_MAX 以降に自動的に番号の割り振りを行う
    ACT_QUAKE, //!< 地震発動
    ACT_RES_ACID, //!< 酸耐性発動
    ACT_RES_ELEC, //!< 電撃耐性発動
    ACT_RES_FIRE, //!< 火炎耐性発動
    ACT_RES_COLD, //!< 冷気耐性発動
    ACT_RES_POIS, //!< 毒耐性発動
    ACT_PHASE_DOOR, //!< ショート・テレポート発動
    ACT_TELEPORT, //!< テレポート発動
    ACT_SPEED, //!< 加速発動
    ACT_STONE_MUD, //!< 岩石溶解発動
    ACT_LIGHT, //!< イルミネーション発動
    ACT_REST_EXP, //!< 経験値復活発動
    ACT_REST_ALL, //!< 全ステータスと経験値復活発動
    ACT_PROT_EVIL, //!< 対邪悪結界発動
    ACT_DISP_EVIL, //!< 邪悪退散発動
    ACT_DISP_GOOD, //!< 善良退散発動
    ACT_PESTICIDE, //!< 害虫駆除発動
    ACT_BA_ACID, //!< アシッド・ボール発動
    ACT_BA_ELEC, //!< サンダー・ボール発動
    ACT_BA_FIRE, //!< ファイア・ボール発動
    ACT_BA_COLD, //!< アイス・ボール発動
    ACT_BA_NUKE, //!< 放射能球発動
    ACT_SUNLIGHT, //!< 太陽光線発動
    ACT_DRAIN, //!< 吸血の矢発動
    ACT_CONFUSE, //!< パニック・モンスター発動
    ACT_DETECT_ALL, //!< 全感知発動
    ACT_DETECT_UNIQUE, //!< ユニークモンスター感知発動
    ACT_MAP_LIGHT, //!< 魔法の地図と光発動
    ACT_SATIATE, //!< 空腹充足発動
    ACT_CURE_700, //!< 体力回復発動

    ATTACK, //!< 命中/ダメージ強化
    AC, //!< AC強化
    SUSTAIN, //!< 装備保持
};

/**
 * @brief アイテムに付与できる鍛冶効果のカテゴリ
 */
enum class SmithCategory {
    NONE = 0,
    WEAPON_ATTR = 1, //!< 武器属性
    RESISTANCE = 2, //!< 耐性
    ABILITY = 3, //!< 能力
    PVAL = 4, //!< 数値
    SLAYING = 5, //!< スレイ
    ESP = 6, //!< ESP
    ETC = 7, //!< その他
    ACTIVATION = 8, //!< 発動
    ENCHANT = 10, //!< 武器防具強化
};

/**
 * @brief 鍛冶エッセンスの列挙体
 */
enum class SmithEssence : int16_t {
    NONE = 0,
    STR = 1, //!< 腕力
    INT = 2, //!< 知能
    WIS = 3, //!< 賢さ
    DEX = 4, //!< 器用さ
    CON = 5, //!< 耐久力
    CHR = 6, //!< 魅力
    MAGIC_MASTERY = 7, //!< 魔法道具支配
    STEALTH = 8, //!< 隠密
    SEARCH = 9, //!< 探索
    INFRA = 10, //!< 赤外線視力
    TUNNEL = 11, //!< 採掘
    SPEED = 12, //!< スピード
    BLOWS = 13, //!< 攻撃回数
    CHAOTIC = 14, //!< カオス攻撃
    VAMPIRIC = 15, //!< 吸血攻撃
    EATHQUAKE = 16, //!< 地震
    BRAND_POIS = 17, //!< 毒殺
    BRAND_ACID = 18, //!< 溶解
    BRAND_ELEC = 19, //!< 電撃
    BRAND_FIRE = 20, //!< 焼棄
    BRAND_COLD = 21, //!< 凍結
    SUST_STATUS = 22, //!< 能力値維持(6能力値共通)
    IMMUNITY = 23, //!< 免疫(4元素共通)
    REFLECT = 24, //!< 反射
    FREE_ACT = 25, //!< 麻痺知らず
    HOLD_EXP = 26, //!< 経験値維持
    RES_ACID = 27, //!< 耐酸
    RES_ELEC = 28, //!< 耐電撃
    RES_FIRE = 29, //!< 耐火炎
    RES_COLD = 30, //!< 耐冷気
    RES_POIS = 31, //!< 耐毒
    RES_FEAR = 32, //!< 耐恐怖
    RES_LITE = 33, //!< 耐閃光
    RES_DARK = 34, //!< 耐暗黒
    RES_BLIND = 35, //!< 耐盲目
    RES_CONF = 36, //!< 耐混乱
    RES_SOUND = 37, //!< 耐轟音
    RES_SHARDS = 38, //!< 耐破片
    RES_NETHER = 39, //!< 耐地獄
    RES_NEXUS = 40, //!< 耐因果混乱
    RES_CHAOS = 41, //!< 耐カオス
    RES_DISEN = 42, //!< 耐劣化
    NO_MAGIC = 43, //!< 反魔法
    WARNING = 44, //!< 警告
    LEVITATION = 45, //!< 浮遊
    LITE = 46, //!< 永久光源
    SEE_INVIS = 47, //!< 可視透明
    TELEPATHY = 48, //!< テレパシー
    SLOW_DIGEST = 49, //!< 遅消化
    REGEN = 50, //!< 急速回復
    TELEPORT = 51, //!< テレポート

    SLAY_EVIL = 52, //!< 邪悪倍打
    SLAY_ANIMAL = 53, //!< 動物倍打
    SLAY_UNDEAD = 54, //!< 不死倍打
    SLAY_DEMON = 55, //!< 悪魔倍打
    SLAY_ORC = 56, //!< オーク倍打
    SLAY_TROLL = 57, //!< トロル倍打
    SLAY_GIANT = 58, //!< 巨人倍打
    SLAY_DRAGON = 59, //!< 竜倍打
    SLAY_HUMAN = 60, //!< 人間倍打
    SLAY_GOOD = 61, //!< 善良倍打

    RES_WATER = 62, //!< 耐水
    RES_TIME = 63, //!< 耐時間逆転
    RES_CURSE = 64, //!< 耐呪力
    NO_TELE = 65, //!< 反テレポート
    EASY2_WEAPON = 66, //!< 二刀流
    STRENGTHEN_BOW = 67, //!< 弓強化
    BRAND_MAGIC = 68, //!< 魔術属性攻撃
    UNIQUE = 69, //!< ユニーク

    ATTACK = 100, //!< 攻撃
    AC = 101, //!< 防御
};
