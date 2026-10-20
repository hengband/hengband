#pragma once

#include "util/flag-group.h"
#include <concepts>
#include <initializer_list>

enum class MainWindowRedrawingFlag {
    TITLE, /*!< 称号 */
    LEVEL,
    EXP, /*!< 経験値 */
    ABILITY_SCORE,
    AC,
    HP,
    MP,
    GOLD,
    DEPTH,
    EQUIPPY, /*!< 装備シンボル */
    HEALTH, /*!< モンスターのステータス */
    CUT, /*!< 負傷度 */
    STUN, /*!< 朦朧度 */
    HUNGER, /*!< 空腹度 */
    TIMED_EFFECT,
    UHEALTH, /*!< ペットのステータス */
    ACTION, /*!< プレイヤーの行動状態 */
    SPEED, /*!< 加速 */
    STUDY, /*!< 学習 */
    IMITATION, /*!< ものまね */
    EXTRA, /*!< 拡張ステータス全体 */
    BASIC, /*!< 基本ステータス全体 */
    MAP, /*!< ゲームマップ */
    WIPE, /*!< 画面消去 */
    MAX,
};

enum class SubWindowRedrawingFlag {
    INVENTORY = 0, /*!< 所持品-装備品 */
    EQUIPMENT = 1, /*!< 装備品-所持品 */
    SPELL = 2, /*!< 魔法一覧 */
    PLAYER = 3, /*!< プレイヤーのステータス */
    SIGHT_MONSTERS = 4, /*!< 視界内モンスターの一覧 */
    PETS = 5, /*!< ペットの一覧 */
    MESSAGE = 6, /*!< メッセージログ */
    OVERHEAD = 7, /*!< 周辺の光景 */
    MONSTER_LORE = 8, /*!< モンスターの思い出 */
    ITEM_KNOWLEDGE = 9, /*!< アイテムの知識 */
    DUNGEON = 10, /*!< ダンジョンの地形 */
    SNAPSHOT = 11, /*!< 記念写真 */
    FLOOR_ITEMS = 12, /*!< 床上のアイテム一覧 */
    FOUND_ITEMS = 13, /*!< 発見済みのアイテム一覧 */
    /*!< 14は予約領域 */
    /*!< 15は予約領域 */
    MAX = 16,
};

enum class StatusRecalculatingFlag {
    BONUS, /*!< 能力値修正 */
    TORCH, /*!< 光源半径 */
    HP,
    MP,
    SPELLS, /*!< 魔法学習数 */
    UN_VIEW, /*!< 地形の視界外化 */
    UN_LITE, /*!< 明暗範囲の視界外化 */
    VIEW, /*!< 視界 */
    LITE, /*!< 明暗範囲 */
    MONSTER_LITE, /*!< モンスターの光源範囲 */
    DELAY_VISIBILITY, /*!< 視界の追加更新 */
    MONSTER_STATUSES, /*!< モンスターのステータス */
    DISTANCE, /*!< プレイヤーとモンスターの距離 */
    FLOW, /*!< プレイヤーから各マスへの到達距離 */
    MAX,
};

/*!
 * @brief 所持品の整理を要することを示すフラグ
 * @details 所持品のスロット番号が変わるため、コマンドの区切りでだけ処理する (update_inventory_arrangement() を参照)。
 */
enum class InventoryArrangementFlag {
    AUTO_DESTRUCTION, /*!< アイテムの自動破壊を要する */
    COMBINATION, /*!< アイテムの結合を要する */
    REORDER, /*!< アイテムの並べ替えを要する */
    MAX,
};

/*!
 * @brief RedrawingFlagsUpdater が扱う更新フラグの型
 */
template <typename T>
concept RedrawingFlagType = std::same_as<T, MainWindowRedrawingFlag> || std::same_as<T, SubWindowRedrawingFlag> ||
                            std::same_as<T, StatusRecalculatingFlag> || std::same_as<T, InventoryArrangementFlag>;

class RedrawingFlagsUpdater {
public:
    RedrawingFlagsUpdater(const RedrawingFlagsUpdater &) = delete;
    RedrawingFlagsUpdater(RedrawingFlagsUpdater &&) = delete;
    RedrawingFlagsUpdater &operator=(const RedrawingFlagsUpdater &) = delete;
    RedrawingFlagsUpdater &operator=(RedrawingFlagsUpdater &&) = delete;
    ~RedrawingFlagsUpdater() = default;

    static RedrawingFlagsUpdater &get_instance();

    bool any_main() const;
    bool any_sub() const;
    bool any_stats() const;

    template <RedrawingFlagType Flag>
    bool has(Flag flag) const
    {
        return flags_of<Flag>(*this).has(flag);
    }

    template <RedrawingFlagType Flag>
    bool has_any_of(const EnumClassFlagGroup<Flag> &flags) const
    {
        return flags_of<Flag>(*this).has_any_of(flags);
    }

    template <RedrawingFlagType Flag>
    bool has_any_of(std::initializer_list<Flag> flags) const
    {
        return this->has_any_of(EnumClassFlagGroup<Flag>(flags));
    }

    template <RedrawingFlagType Flag>
    void set_flag(Flag flag)
    {
        flags_of<Flag>(*this).set(flag);
    }

    template <RedrawingFlagType Flag>
    void set_flags(const EnumClassFlagGroup<Flag> &flags)
    {
        flags_of<Flag>(*this).set(flags);
    }

    template <RedrawingFlagType Flag>
    void set_flags(std::initializer_list<Flag> flags)
    {
        this->set_flags(EnumClassFlagGroup<Flag>(flags));
    }

    template <RedrawingFlagType Flag>
    void reset_flag(Flag flag)
    {
        flags_of<Flag>(*this).reset(flag);
    }

    template <RedrawingFlagType Flag>
    void reset_flags(const EnumClassFlagGroup<Flag> &flags)
    {
        flags_of<Flag>(*this).reset(flags);
    }

    template <RedrawingFlagType Flag>
    void reset_flags(std::initializer_list<Flag> flags)
    {
        this->reset_flags(EnumClassFlagGroup<Flag>(flags));
    }

    void request_inventory_arrangement();
    void request_whole_main_window_redrawing();
    void request_whole_status_recalculation();
    void request_map_redrawing();
    void request_monster_update_and_map_redrawing();
    void request_sight_recalculation();
    void request_item_lists_redrawing();
    void request_carried_item_lists_redrawing();
    void request_floor_item_lists_redrawing();
    void request_spell_info_redrawing();
    void request_inscription_change_updates();
    void request_equipment_change_updates();
    void request_riding_status_redrawing();
    void request_hp_mp_redrawing();
    void request_gold_redrawing();

    void fill_up_sub_flags();
    EnumClassFlagGroup<SubWindowRedrawingFlag> get_sub_intersection(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags) const;

private:
    RedrawingFlagsUpdater() = default;

    static RedrawingFlagsUpdater instance;

    EnumClassFlagGroup<MainWindowRedrawingFlag> main_window_flags{};
    EnumClassFlagGroup<SubWindowRedrawingFlag> sub_window_flags{};
    EnumClassFlagGroup<StatusRecalculatingFlag> status_flags{};
    EnumClassFlagGroup<InventoryArrangementFlag> inventory_arrangement_flags{};

    /*!
     * @brief フラグの型に対応するフラグの集合を得る
     * @tparam Flag フラグの型
     * @tparam Self RedrawingFlagsUpdater (const の有無を問わない)
     * @param self 更新フラグ
     * @return フラグの集合への参照 (self が const なら const 参照)
     */
    template <RedrawingFlagType Flag, typename Self>
    static auto &flags_of(Self &self)
    {
        if constexpr (std::same_as<Flag, MainWindowRedrawingFlag>) {
            return self.main_window_flags;
        } else if constexpr (std::same_as<Flag, SubWindowRedrawingFlag>) {
            return self.sub_window_flags;
        } else if constexpr (std::same_as<Flag, StatusRecalculatingFlag>) {
            return self.status_flags;
        } else if constexpr (std::same_as<Flag, InventoryArrangementFlag>) {
            return self.inventory_arrangement_flags;
        } else {
            // static_assert(false) を受け付けないコンパイラのため、テンプレート引数に依存する偽の式にする
            static_assert(!std::same_as<Flag, Flag>, "RedrawingFlagType に足したフラグの型を flags_of() にも足すこと");
        }
    }
};
