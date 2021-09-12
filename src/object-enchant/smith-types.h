#pragma once

#include "system/system-variables.h"

/**
 * @brief アイテムに付与できる鍛冶効果の列挙体
 */
enum class SmithEffect {
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

    MAGIC_MASTERY = 20, //!< 魔法道具支配
    STEALTH = 21, //!< 隠密
    SEARCH = 22, //!< 探索
    INFRA = 23, //!< 赤外線視力
    TUNNEL = 24, //!< 採掘
    SPEED = 25, //!< スピード
    BLOWS = 26, //!< 追加攻撃

    CHAOTIC = 40, //!< カオス攻撃
    VAMPIRIC = 41, //!< 吸血攻撃
    EARTHQUAKE = 42, //!< 地震発動
    BRAND_POIS = 43, //!< 毒殺
    BRAND_ACID = 44, //!< 溶解
    BRAND_ELEC = 45, //!< 電撃
    BRAND_FIRE = 46, //!< 焼棄
    BRAND_COLD = 47, //!< 凍結

    IM_ACID = 60, //!< 酸免疫
    IM_ELEC = 61, //!< 電撃免疫
    IM_FIRE = 62, //!< 火炎免疫
    IM_COLD = 63, //!< 冷気免疫
    REFLECT = 64, //!< 反射

    RES_ACID = 70, //!< 耐酸
    RES_ELEC = 71, //!< 耐電撃
    RES_FIRE = 72, //!< 耐火炎
    RES_COLD = 73, //!< 耐冷気
    RES_POIS = 74, //!< 耐毒
    RES_FEAR = 75, //!< 耐恐怖
    RES_LITE = 76, //!< 耐閃光
    RES_DARK = 77, //!< 耐暗黒
    RES_BLIND = 78, //!< 耐盲目
    RES_CONF = 79, //!< 耐混乱
    RES_SOUND = 80, //!< 耐轟音
    RES_SHARDS = 81, //!< 耐破片
    RES_NETHER = 82, //!< 耐地獄
    RES_NEXUS = 83, //!< 耐因果混乱
    RES_CHAOS = 84, //!< 耐カオス
    RES_DISEN = 85, //!< 耐劣化
    RES_WATER = 86, //!< 耐水
    RES_TIME = 87, //!< 耐時間逆転
    RES_CURSE = 88, //!< 耐呪力

    HOLD_EXP = 100, //!< 経験値維持
    FREE_ACT = 101, //!< 麻痺知らず
    WARNING = 102, //!< 警告
    LEVITATION = 103, //!< 浮遊
    SEE_INVIS = 104, //!< 可視透明
    SLOW_DIGEST = 105, //!< 遅消化
    REGEN = 106, //!< 急速回復
    TELEPORT = 107, //!< テレポート
    NO_MAGIC = 108, //!< 反魔法
    LITE = 109, //!< 永久光源

    SLAY_EVIL = 120, //!< 邪悪倍打
    SLAY_ANIMAL = 121, //!< 動物倍打
    SLAY_UNDEAD = 122, //!< 不死倍打
    SLAY_DEMON = 123, //!< 悪魔倍打
    SLAY_ORC = 124, //!< オーク倍打
    SLAY_TROLL = 125, //!< トロル倍打
    SLAY_GIANT = 126, //!< 巨人倍打
    SLAY_DRAGON = 127, //!< 竜倍打
    SLAY_HUMAN = 128, //!< 人間倍打
    SLAY_GOOD = 129, //!< 善良倍打

    KILL_EVIL = 140, //!< 邪悪倍倍打
    KILL_ANIMAL = 141, //!< 動物倍倍打
    KILL_UNDEAD = 142, //!< 不死倍倍打
    KILL_DEMON = 143, //!< 悪魔倍倍打
    KILL_ORC = 144, //!< オーク倍倍打
    KILL_TROLL = 145, //!< トロル倍倍打
    KILL_GIANT = 146, //!< 巨人倍倍打
    KILL_DRAGON = 147, //!< 竜倍倍打
    KILL_HUMAN = 148, //!< 人間倍倍打
    KILL_GOOD = 149, //!< 善良倍倍打

    TELEPATHY = 160, //!< テレパシー
    ESP_ANIMAL = 161, //!< 動物ESP
    ESP_UNDEAD = 162, //!< 不死ESP
    ESP_DEMON = 163, //!< 悪魔ESP
    ESP_ORC = 164, //!< オークESP
    ESP_TROLL = 165, //!< トロルESP
    ESP_GIANT = 166, //!< 巨人ESP
    ESP_DRAGON = 167, //!< 竜ESP
    ESP_HUMAN = 168, //!< 人間ESP
    ESP_GOOD = 169, //!< 善良ESP

    TMP_RES_ACID = 200, //!< 酸耐性発動
    TMP_RES_ELEC = 201, //!< 電撃耐性発動
    TMP_RES_FIRE = 202, //!< 火炎耐性発動
    TMP_RES_COLD = 203, //!< 冷気耐性発動
    SH_FIRE = 204, //!< 火炎オーラ
    SH_ELEC = 205, //!< 電撃オーラ
    SH_COLD = 206, //!< 冷気オーラ

    RESISTANCE = 220, //!< 全耐性
    SLAY_GLOVE = 221, //!< 殺戮の小手

    ATTACK = 250, //!< 命中/ダメージ強化
    AC = 251, //!< AC強化
    SUSTAIN = 252, //!< 装備保持

    MAX = 256, //!< 鍛冶アイテムの保存領域が1バイトなので、この値未満を割り当てる必要あり
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
    ENCHANT = 10, //!< 武器防具強化
};

/**
 * @brief 鍛冶エッセンスの列挙体
 */
enum class SmithEssence {
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

    ATTACK = 100, //!< 攻撃
    AC = 101, //!< 防御

    MAX = MAX_SPELLS, //!< エッセンス所持量はplayer_type::magic_num1[MAX_SPELLS]に格納するので、この値未満を割り当てる必要あり
};
