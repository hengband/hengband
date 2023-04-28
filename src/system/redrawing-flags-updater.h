#pragma once

#include "util/flag-group.h"

enum class MainWindowRedrawingFlag {
    MISC, /*!< 種族と職業 */
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
    INVENTORY, /*!< 所持品-装備品 */
    EQUIPMENT, /*!< 装備品-所持品 */
    SPELL, /*!< 魔法一覧 */
    PLAYER, /*!< プレイヤーのステータス */
    SIGHT_MONSTERS, /*!< 視界内モンスターの一覧 */
    MESSAGE, /*!< メッセージログ */
    OVERHEAD, /*!< 周辺の光景 */
    MONSTER_LORE, /*!< モンスターの思い出 */
    ITEM_KNOWLEDGTE, /*!< アイテムの知識 */
    DUNGEON, /*!< ダンジョンの地形 */
    SNAPSHOT, /*!< 記念写真 */
    FLOOR_ITEMS, /*!< 床上のアイテム一覧 */
    FOUND_ITEMS, /*!< 発見済みのアイテム一覧 */
    MAX,
};

enum class StatusRedrawingFlag {
    BONUS, /*!< 能力値修正 */
    TORCH, /*!< 光源半径 */
    HP,
    MP,
    SPELLS, /*!< 魔法学習数 */
    COMBINATION, /*!< アイテム処理フラグ: アイテムの結合を要する */
    REORDER, /*!< アイテム処理フラグ: アイテムの並び替えを要する */
    AUTO_DESTRUCTION, /*!< アイテム処理フラグ: アイテムの自動破壊を要する */
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

    bool has(MainWindowRedrawingFlag flag) const;
    bool has(SubWindowRedrawingFlag flag) const;
    bool has(StatusRedrawingFlag flag) const;

    bool has_any_of(const EnumClassFlagGroup<MainWindowRedrawingFlag> &flags) const;
    bool has_any_of(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags) const;
    bool has_any_of(const EnumClassFlagGroup<StatusRedrawingFlag> &flags) const;

    void set_flag(MainWindowRedrawingFlag flag);
    void set_flag(SubWindowRedrawingFlag flag);
    void set_flag(StatusRedrawingFlag flag);

    void set_flags(const EnumClassFlagGroup<MainWindowRedrawingFlag> &flags);
    void set_flags(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags);
    void set_flags(const EnumClassFlagGroup<StatusRedrawingFlag> &flags);

    void reset_flag(MainWindowRedrawingFlag flag);
    void reset_flag(SubWindowRedrawingFlag flag);
    void reset_flag(StatusRedrawingFlag flag);

    void reset_flags(const EnumClassFlagGroup<MainWindowRedrawingFlag> &flags);
    void reset_flags(const EnumClassFlagGroup<SubWindowRedrawingFlag> &flags);
    void reset_flags(const EnumClassFlagGroup<StatusRedrawingFlag> &flags);

    void fill_up_sub_flags();

private:
    RedrawingFlagsUpdater() = default;

    static RedrawingFlagsUpdater instance;

    EnumClassFlagGroup<MainWindowRedrawingFlag> main_window_flags{};
    EnumClassFlagGroup<SubWindowRedrawingFlag> sub_window_flags{};
    EnumClassFlagGroup<StatusRedrawingFlag> status_flags{};
};
