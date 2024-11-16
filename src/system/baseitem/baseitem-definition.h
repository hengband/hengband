#pragma once

#include "object-enchant/tr-flags.h"
#include "object-enchant/trg-types.h"
#include "object/tval-types.h"
#include "system/angband.h"
#include "system/baseitem/baseitem-key.h"
#include "util/dice.h"
#include "util/enum-range.h"
#include "util/flag-group.h"
#include "view/display-symbol.h"
#include <array>
#include <map>
#include <optional>
#include <string>
#include <vector>

enum class MoneyKind {
    COPPER,
    SILVER,
    GARNET,
    GOLD,
    OPAL,
    SAPPHIRE,
    RUBY,
    DIAMOND,
    EMERALD,
    MITHRIL,
    ADAMANTITE,
    MAX,
};

constexpr EnumRange<MoneyKind> MONEY_KIND_RANGE(MoneyKind::COPPER, MoneyKind::MAX);

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

enum class ItemKindType : short;
enum class MonraceId : short;
class BaseitemList {
public:
    BaseitemList(BaseitemList &&) = delete;
    BaseitemList(const BaseitemList &) = delete;
    BaseitemList &operator=(const BaseitemList &) = delete;
    BaseitemList &operator=(BaseitemList &&) = delete;
    ~BaseitemList() = default;

    static BaseitemList &get_instance();
    BaseitemInfo &get_baseitem(const short bi_id);
    const BaseitemInfo &get_baseitem(const short bi_id) const;

    std::vector<BaseitemInfo>::iterator begin();
    std::vector<BaseitemInfo>::const_iterator begin() const;
    std::vector<BaseitemInfo>::iterator end();
    std::vector<BaseitemInfo>::const_iterator end() const;
    std::vector<BaseitemInfo>::reverse_iterator rbegin();
    std::vector<BaseitemInfo>::const_reverse_iterator rbegin() const;
    std::vector<BaseitemInfo>::reverse_iterator rend();
    std::vector<BaseitemInfo>::const_reverse_iterator rend() const;
    size_t size() const;
    bool empty() const;
    void resize(size_t new_size);
    void shrink_to_fit();

    std::optional<int> lookup_creeping_coin_drop_offset(MonraceId monrace_id) const;
    short lookup_baseitem_id(const BaseitemKey &bi_key) const;
    const BaseitemInfo &lookup_baseitem(const BaseitemKey &bi_key) const;
    int calc_num_gold_subtypes() const;
    const BaseitemInfo &lookup_gold(int target_offset) const;
    int lookup_gold_offset(short bi_id) const;

    void reset_all_visuals();
    void reset_identification_flags();
    void mark_common_items_as_aware();
    void shuffle_flavors();

private:
    BaseitemList() = default;

    static BaseitemList instance;
    std::vector<BaseitemInfo> baseitems{};

    short exe_lookup(const BaseitemKey &bi_key) const;
    const std::map<BaseitemKey, short> &create_baseitem_index_chache() const;
    const std::map<ItemKindType, std::vector<int>> &create_baseitems_cache() const;
    int lookup_gold_offset(const BaseitemKey &finding_bi_key) const;
    const std::map<MoneyKind, std::vector<BaseitemKey>> &create_sorted_golds() const;
    std::map<MoneyKind, std::vector<BaseitemKey>> create_unsorted_golds() const;

    BaseitemInfo &lookup_baseitem(const BaseitemKey &bi_key);
    void shuffle_flavors(ItemKindType tval);
};
