#pragma once

#include "system/angband.h"

#include "object-enchant/trg-types.h"
#include "object/tval-types.h"
#include "util/flag-group.h"
#include "system/system-variables.h"

#include <string>
#include <vector>

typedef struct object_kind {
    std::string name; /*!< ベースアイテム名参照のためのネームバッファオフセット値 / Name (offset) */
    std::string text; /*!< 解説テキスト参照のためのネームバッファオフセット値 / Text (offset) */
    std::string flavor_name; /*!< 未確定名参照のためのネームバッファオフセット値 / Flavor name (offset) */

    tval_type tval{}; /*!< ベースアイテム種別の大項目値 Object type */
    OBJECT_SUBTYPE_VALUE sval{}; /*!< ベースアイテム種別の小項目値 Object sub type */

    PARAMETER_VALUE pval{}; /*!< ベースアイテムのpval（能力修正共通値） Object extra info */

    HIT_PROB to_h{}; /*!< ベースアイテムの命中修正値 / Bonus to hit */
    HIT_POINT to_d{}; /*!< ベースアイテムのダメージ修正値 / Bonus to damage */
    ARMOUR_CLASS to_a{}; /*!< ベースアイテムのAC修正値 / Bonus to armor */

    ARMOUR_CLASS ac{}; /*!< ベースアイテムのAC基本値 /  Base armor */

    DICE_NUMBER dd{};
    DICE_SID ds{}; /*!< ダメージダイスの数と大きさ / Damage dice/sides */

    WEIGHT weight{}; /*!< ベースアイテムの重量 / Weight */

    PRICE cost{}; /*!< ベースアイテムの基本価値 / Object "base cost" */

    TrFlags flags{}; /*!< ベースアイテムの基本特性ビット配列 / Flags */

    EnumClassFlagGroup<TRG> gen_flags; /*!< ベースアイテムの生成特性ビット配列 / flags for generate */

    DEPTH locale[4]{}; /*!< ベースアイテムの生成階テーブル / Allocation level(s) */
    PROB chance[4]{}; /*!< ベースアイテムの生成確率テーブル / Allocation chance(s) */

    DEPTH level{}; /*!< ベースアイテムの基本生成階 / Level */
    BIT_FLAGS8 extra{}; /*!< その他色々のビットフラグ配列 / Something */

    TERM_COLOR d_attr{}; /*!< デフォルトのアイテムシンボルカラー / Default object attribute */
    SYMBOL_CODE d_char{}; /*!< デフォルトのアイテムシンボルアルファベット / Default object character */

    TERM_COLOR x_attr{}; /*!< 設定変更後のアイテムシンボルカラー /  Desired object attribute */
    SYMBOL_CODE x_char{}; /*!< 設定変更後のアイテムシンボルアルファベット /  Desired object character */

    IDX flavor{}; /*!< 未鑑定名の何番目を当てるか(0は未鑑定名なし) / Special object flavor (or zero) */
    bool easy_know{}; /*!< ベースアイテムが初期からベース名を判断可能かどうか / This object is always known (if aware) */
    bool aware{}; /*!< ベースアイテムが鑑定済かどうか /  The player is "aware" of the item's effects */
    bool tried{}; /*!< ベースアイテムを未鑑定のまま試したことがあるか /  The player has "tried" one of the items */

    int16_t act_idx{}; /*!< 発動能力のID /  Activative ability index */
} object_kind;

extern std::vector<object_kind> k_info;
extern KIND_OBJECT_IDX max_k_idx;

struct object_type;;
SYMBOL_CODE object_char(object_type *o_ptr);
