#pragma once

#include "object-enchant/tr-flags.h"
#include "object-enchant/trg-types.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include <array>
#include <optional>
#include <string>
#include <vector>

enum class ItemKindType : short;
class BaseitemKey {
public:
    BaseitemKey(const ItemKindType type_value, const std::optional<int> &subtype_value = std::nullopt);
    bool operator==(const BaseitemKey &other) const;
    bool operator!=(const BaseitemKey &other) const
    {
        return !(*this == other);
    }

    bool operator<(const BaseitemKey &other) const;
    bool operator>(const BaseitemKey &other) const
    {
        return other < *this;
    }

    bool operator<=(const BaseitemKey &other) const
    {
        return !(*this > other);
    }

    bool operator>=(const BaseitemKey &other) const
    {
        return !(*this < other);
    }

    ItemKindType tval() const;
    std::optional<int> sval() const;

    ItemKindType get_arrow_kind() const;
    bool is_spell_book() const;
    bool is_high_level_book() const;
    bool is_melee_weapon() const;
    bool is_ammo() const;
    bool has_unidentified_name() const;
    bool can_recharge() const;
    bool is_wand_rod() const;
    bool is_wand_staff() const;
    bool is_protector() const;
    bool can_be_aura_protector() const;
    bool is_wearable() const;
    bool is_weapon() const;
    bool is_equipement() const;
    bool is_melee_ammo() const;
    bool is_orthodox_melee_weapon() const;
    bool is_broken_weapon() const;
    bool is_throwable() const;
    bool is_wieldable_in_etheir_hand() const;
    bool is_rare() const;
    short get_bow_energy() const;
    int get_arrow_magnification() const;
    bool is_aiming_rod() const;
    bool is_lite_requiring_fuel() const;
    bool is_junk() const;
    bool is_armour() const;
    bool is_cross_bow() const;

private:
    ItemKindType type_value;
    std::optional<int> subtype_value;

    bool is_mushrooms() const;
};

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

    DICE_NUMBER dd{}; /*!< ダメージダイスの数 / Damage dice */
    DICE_SID ds{}; /*!< ダメージダイスの大きさ / Damage sides */

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

    TERM_COLOR d_attr{}; /*!< デフォルトのアイテムシンボルカラー / Default object attribute */
    char d_char{}; /*!< デフォルトのアイテムシンボルアルファベット / Default object character */
    bool easy_know{}; /*!< ベースアイテムが初期からベース名を判断可能かどうか / This object is always known (if aware) */
    RandomArtActType act_idx{}; /*!< 発動能力のID /  Activative ability index */

    /* @todo ここから下はk_info.txt に依存しないミュータブルなフィールド群なので、将来的に分離予定 */

    TERM_COLOR x_attr{}; /*!< 設定変更後のアイテムシンボルカラー /  Desired object attribute */
    char x_char{}; /*!< 設定変更後のアイテムシンボルアルファベット /  Desired object character */

    IDX flavor{}; /*!< 未鑑定名の何番目を当てるか(0は未鑑定名なし) / Special object flavor (or zero) */
    bool aware{}; /*!< ベースアイテムが鑑定済かどうか /  The player is "aware" of the item's effects */
    bool tried{}; /*!< ベースアイテムを未鑑定のまま試したことがあるか /  The player has "tried" one of the items */
};

extern std::vector<BaseitemInfo> baseitems_info;
