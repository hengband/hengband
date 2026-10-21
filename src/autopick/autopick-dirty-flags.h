#pragma once

/*
 * Dirty flag for text editor
 */
constexpr auto DIRTY_ALL = 0x0001;
constexpr auto DIRTY_MODE = 0x0004;
constexpr auto DIRTY_SCREEN = 0x0008;
constexpr auto DIRTY_NOT_FOUND = 0x0010;
constexpr auto DIRTY_NO_SEARCH = 0x0020;
constexpr auto DIRTY_EXPRESSION = 0x0040;
constexpr auto DIRTY_SKIP_INACTIVE = 0x0080;
constexpr auto DIRTY_INACTIVE = 0x0100;
