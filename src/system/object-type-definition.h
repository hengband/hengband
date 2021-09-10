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

#include "object-enchant/tr-flags.h"

struct player_type;
typedef struct object_type {
    KIND_OBJECT_IDX k_idx{}; /*!< Kind index (zero if "dead") */
    POSITION iy{}; /*!< Y-position on map, or zero */
    POSITION ix{}; /*!< X-position on map, or zero */
    IDX stack_idx{}; /*!< このアイテムを含むアイテムリスト内の位置(降順) */
    tval_type tval{}; /*!< Item type (from kind) */

    OBJECT_SUBTYPE_VALUE sval{}; /*!< Item sub-type (from kind) */
    PARAMETER_VALUE pval{}; /*!< Item extra-parameter */
    byte discount{}; /*!< ゲーム中の値引き率 (0～100) / Discount (if any) */
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
    bool is_weapon() const;
    bool is_weapon_ammo() const;
    bool is_weapon_armour_ammo() const;
    bool is_melee_weapon() const;
    bool is_melee_ammo() const;
    bool is_wearable() const;
    bool is_equipment() const;
    bool is_orthodox_melee_weapons() const;
    bool is_broken_weapon() const;
    bool is_throwable() const;
    bool is_wieldable_in_etheir_hand() const;
    bool refuse_enchant_weapon() const;
    bool allow_enchant_weapon() const;
    bool allow_enchant_melee_weapon() const;
    bool allow_two_hands_wielding() const;
    bool is_ammo() const;
    bool is_convertible() const;
    bool is_lance() const;
    bool is_armour() const;
    bool is_rare() const;
    bool is_ego() const;
    bool is_smith() const;
    bool is_artifact() const;
    bool is_fixed_artifact() const;
    bool is_random_artifact() const;
    bool is_nameless() const;
    bool is_valid() const;
    bool is_broken() const;
    bool is_cursed() const;
    bool is_held_by_monster() const;
    bool is_known() const;
    bool is_fully_known() const;
    bool is_aware() const;
    bool is_tried() const;
    bool is_potion() const;
    bool is_readable() const;
    bool can_refill_lantern() const;
    bool can_refill_torch() const;
    bool is_rechargeable() const;
    bool is_offerable() const;
    bool is_activatable() const;
} object_type;
