#pragma once
#include <stdint.h>

/* target_set用関数の利用用途フラグ / Bit flags for the "target_set" function */
enum target_type : uint32_t {
    TARGET_KILL = 0x01, /*!< モンスターへの狙いをつける(視界内モンスターのみクエリ対象) / Target monsters */
    TARGET_LOOK = 0x02, /*!< "L"ookコマンド向けの既存情報確認向け(全ての有為な情報をクエリ対象) / Describe grid fully */
    TARGET_XTRA = 0x04, /*!< 現在未使用 / Currently unused flag */
    TARGET_GRID = 0x08, /*!< 全てのマス対象にする(現在未使用) / Select from all grids */
};
