#pragma once

#define DETECT_RAD_DEFAULT 30
#define DETECT_RAD_MAP 30
#define DETECT_RAD_ALL 255

enum spell_type {
    SPELL_NAME = 0,
    SPELL_DESCRIPTION = 1,
    SPELL_INFO = 2,
    SPELL_CAST = 3,
    SPELL_FAIL = 4,
    SPELL_STOP = 5,
    SPELL_CONTNUATION = 6
};

enum spell_operation {
    SPOP_NONE = 0x0000U,
    SPOP_DISPLAY_MES = 0x0001U, // !< スペル処理オプション … メッセージを表示する
    SPOP_NO_UPDATE = 0x0002U, // !< スペル処理オプション … ステータス更新を解決後行う
    SPOP_DEBUG = 0x8000U // !< スペル処理オプション … デバッグ処理あり
};

enum teleport_flags {
    TELEPORT_SPONTANEOUS = 0x0000,
    TELEPORT_NONMAGICAL = 0x0001,
    TELEPORT_PASSIVE = 0x0002,
    TELEPORT_DEC_VALOUR = 0x0004
};

enum autogenesis_magical_effect {
    PROJECT_WHO_UNCTRL_POWER = -1, /*!< 魔法効果の自然発生要因: 名状し難い力の解放 */
    PROJECT_WHO_GLASS_SHARDS = -2 /*!< 魔法効果の自然発生要因: 破壊されたガラス地形の破片 */
};
