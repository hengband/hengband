/*!
 * @brief ベースアイテムの定義
 * @date 2024/11/16
 * @author deskull, Hourier
 */

#pragma once

#include "object-enchant/tr-flags.h"
#include "object-enchant/trg-types.h"
#include "system/baseitem/baseitem-key.h"
#include "util/dice.h"
#include "util/flag-group.h"
#include "view/display-symbol.h"
#include <array>
#include <string>

enum class ItemKindType : short;
enum class RandomArtActType : short;
class BaseitemDefinition {
public:
    BaseitemDefinition();
    BaseitemDefinition(BaseitemDefinition &) = delete;
    BaseitemDefinition &operator=(BaseitemDefinition &) = delete;
    BaseitemDefinition(BaseitemDefinition &&) = default;
    BaseitemDefinition &operator=(BaseitemDefinition &&) = delete;

    std::string name; /*!< ベースアイテム名 */
    std::string text; /*!< 解説テキスト */
    std::string flavor_name; /*!< 未確定名 */

    BaseitemKey bi_key;

    short pval{}; /*!< ベースアイテムのpval（能力修正共通値） Object extra info */

    short to_h{}; /*!< ベースアイテムの命中修正値 / Bonus to hit */
    int to_d{}; /*!< ベースアイテムのダメージ修正値 / Bonus to damage */
    short to_a{}; /*!< ベースアイテムのAC修正値 / Bonus to armor */
    short ac{}; /*!< ベースアイテムのAC基本値 /  Base armor */

    Dice damage_dice{}; /*!< ダメージダイス */

    int weight{}; /*!< ベースアイテムの重量 / Weight */
    int cost{}; /*!< ベースアイテムの基本価値 / Object "base cost" */
    TrFlags flags{}; /*!< ベースアイテムの基本特性ビット配列 / Flags */
    EnumClassFlagGroup<ItemGenerationTraitType> gen_flags; /*!< ベースアイテムの生成特性ビット配列 / flags for generate */

    int level{}; /*!< ベースアイテムの基本生成階 / Level */

    struct alloc_table {
        int level; /*!< ベースアイテムの生成階 */
        short chance; /*!< ベースアイテムの生成確率 */
    };

    std::array<alloc_table, 4> alloc_tables{}; /*!< ベースアイテムの生成テーブル */
    bool easy_know{}; /*!< ベースアイテムが初期からベース名を判断可能かどうか / This object is always known (if aware) */
    RandomArtActType act_idx{}; /*!< 発動能力のID /  Activative ability index */

    bool is_valid() const;
    std::string stripped_name() const;
    bool order_cost(const BaseitemDefinition &other) const;
    void decide_easy_know();

    const DisplaySymbol &get_symbol() const;
    void init_symbol(const DisplaySymbol &ds);
    void init_color(uint8_t color);
    void init_character(char character);

private:
    DisplaySymbol symbol_definition; //!< 定義上のシンボル (色/文字).
};
