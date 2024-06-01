#pragma once

#include "system/angband.h"
#include "util/flag-group.h"

enum class FloorChangeMode : short {
    UP, //!< 上のフロアへ行く (階層は浅くなる).
    DOWN, //!< 下のフロアへ行く (階層は深くなる).
    SHAFT, //!< 坑道である (2階層分移動する).
    RANDOM_PLACE, //!< 移動先フロアにランダム配置される.
    RANDOM_CONNECT, //!< 移動先フロアの階段にランダム接続される.
    SAVE_FLOORS, //!< 保存済フロアに移動する.
    NO_RETURN, //!< 帰還などで移動元に戻らない.
    FIRST_FLOOR, //!< 荒野からダンジョンの一番浅い階層に移動する.
    MAX,
};

class FloorChangeModesStore {
public:
    ~FloorChangeModesStore() = default;
    FloorChangeModesStore(const FloorChangeModesStore &) = delete;
    FloorChangeModesStore(FloorChangeModesStore &&) = delete;
    FloorChangeModesStore &operator=(const FloorChangeModesStore &) = delete;
    FloorChangeModesStore &operator=(FloorChangeModesStore &&) = delete;
    static FloorChangeModesStore &get_instace();

    EnumClassFlagGroup<FloorChangeMode> *operator->();
    const EnumClassFlagGroup<FloorChangeMode> *operator->() const;

private:
    static FloorChangeModesStore instance;
    FloorChangeModesStore() = default;

    EnumClassFlagGroup<FloorChangeMode> flag_change_modes;
};
