#pragma once

/*
 * @file object-type-definition.h
 * @brief アイテム定義の構造体とエンティティ処理定義
 * @author Hourier
 * @date 2021/05/02
 */

#include "object-enchant/trc-types.h"
#include "object/tval-types.h"
#include "system/angband.h"
#include "system/system-variables.h"
#include "util/flag-group.h"

struct player_type;
typedef struct object_type {
    KIND_OBJECT_IDX k_idx{}; /*!< Kind index (zero if "dead") */
    POSITION iy{}; /*!< Y-position on map, or zero */
    POSITION ix{}; /*!< X-position on map, or zero */
    IDX stack_idx{}; /*!< このアイテムを含むアイテムリスト内の位置(降順) */
    tval_type tval{}; /*!< Item type (from kind) */

    OBJECT_SUBTYPE_VALUE sval{}; /*!< Item sub-type (from kind) */
    PARAMETER_VALUE pval{}; /*!< Item extra-parameter */
    DISCOUNT_RATE discount{}; /*!< Discount (if any) */
    ITEM_NUMBER number{}; /*!< Number of items */
    WEIGHT weight{}; /*!< Item weight */
    ARTIFACT_IDX name1{}; /*!< Artifact type, if any */
    EGO_IDX name2{}; /*!< Ego-Item type, if any */

    XTRA8 xtra1{}; /*!< Extra info type (now unused) */
    XTRA16 xtra2{}; /*!< エゴ/アーティファクトの発動ID / Extra info activation index */
    XTRA8 xtra3{}; /*!< 複数の使用用途 捕らえたモンスターの速度，付加した特殊なエッセンスID / Extra info for weaponsmith */
    XTRA16 xtra4{}; /*!< 複数の使用用途 光源の残り寿命、あるいは捕らえたモンスターの現HP / Extra info fuel or captured monster's current HP */
    XTRA16 xtra5{}; /*!< 複数の使用用途 捕らえたモンスターの最大HP / Extra info captured monster's max HP */

    HIT_PROB to_h{}; /*!< Plusses to hit */
    HIT_POINT to_d{}; /*!< Plusses to damage */
    ARMOUR_CLASS to_a{}; /*!< Plusses to AC */
    ARMOUR_CLASS ac{}; /*!< Normal AC */

    DICE_NUMBER dd{}; /*!< Damage dice/nums */
    DICE_SID ds{}; /*!< Damage dice/sides */
    TIME_EFFECT timeout{}; /*!< Timeout Counter */
    byte ident{}; /*!< Special flags  */
    byte marked{}; /*!< Object is marked */
    uint16_t inscription{}; /*!< Inscription index */
    uint16_t art_name{}; /*!< Artifact name (random artifacts) */
    byte feeling{}; /*!< Game generated inscription number (eg, pseudo-id) */

    TrFlags art_flags{}; /*!< Extra Flags for ego and artifacts */
    EnumClassFlagGroup<TRC> curse_flags{}; /*!< Flags for curse */
    MONSTER_IDX held_m_idx{}; /*!< アイテムを所持しているモンスターID (いないなら 0) / Monster holding us (if any) */
    int artifact_bias{}; /*!< ランダムアーティファクト生成時のバイアスID */

    void wipe();
    void copy_from(object_type *j_ptr);
    void prep(KIND_OBJECT_IDX ko_idx);
} object_type;
