#pragma once

#include "object-enchant/tr-flags.h"
#include "object-enchant/trg-types.h"
#include "system/angband.h"
#include "system/baseitem/baseitem-key.h"
#include "util/dice.h"
#include "util/flag-group.h"
#include "view/display-symbol.h"
#include <array>
#include <string>

enum class ItemKindType : short;
enum class RandomArtActType : short;
class BaseitemInfo {
public:
    BaseitemInfo();
    short idx{};

    std::string name; /*!< ベースアイテム名 */
    std::string text; /*!< 解説テキスト */
    std::string flavor_name; /*!< 未確定名 */

    BaseitemKey bi_key;

    PARAMETER_VALUE pval{}; /*!< ベースアイテムのpval（能力修正共通値） Object extra info */

    HIT_PROB to_h{}; /*!< ベースアイテムの命中修正値 / Bonus to hit */
    int to_d{}; /*!< ベースアイテムのダメージ修正値 / Bonus to damage */
    ARMOUR_CLASS to_a{}; /*!< ベースアイテムのAC修正値 / Bonus to armor */
    ARMOUR_CLASS ac{}; /*!< ベースアイテムのAC基本値 /  Base armor */

    Dice damage_dice{}; /*!< ダメージダイス */

    WEIGHT weight{}; /*!< ベースアイテムの重量 / Weight */
    PRICE cost{}; /*!< ベースアイテムの基本価値 / Object "base cost" */
    TrFlags flags{}; /*!< ベースアイテムの基本特性ビット配列 / Flags */
    EnumClassFlagGroup<ItemGenerationTraitType> gen_flags; /*!< ベースアイテムの生成特性ビット配列 / flags for generate */

    DEPTH level{}; /*!< ベースアイテムの基本生成階 / Level */

    struct alloc_table {
        int level; /*!< ベースアイテムの生成階 */
        short chance; /*!< ベースアイテムの生成確率 */
    };

    std::array<alloc_table, 4> alloc_tables{}; /*!< ベースアイテムの生成テーブル */
    DisplaySymbol symbol_definition; //!< 定義上のシンボル (色/文字).
    bool easy_know{}; /*!< ベースアイテムが初期からベース名を判断可能かどうか / This object is always known (if aware) */
    RandomArtActType act_idx{}; /*!< 発動能力のID /  Activative ability index */

    bool is_valid() const;
    std::string stripped_name() const;
    bool order_cost(const BaseitemInfo &other) const;
    void decide_easy_know();

    /* @todo ここから下はBaseitemDefinitions.txt に依存しないミュータブルなフィールド群なので、将来的に分離予定 */

    DisplaySymbol symbol_config; //!< ユーザ個別の設定シンボル (色/文字).

    IDX flavor{}; /*!< 未鑑定名の何番目を当てるか(0は未鑑定名なし) / Special object flavor (or zero) */
    bool aware{}; /*!< ベースアイテムが鑑定済かどうか /  The player is "aware" of the item's effects */
    bool tried{}; /*!< ベースアイテムを未鑑定のまま試したことがあるか /  The player has "tried" one of the items */

    void mark_as_tried();
    void mark_as_aware();
};
