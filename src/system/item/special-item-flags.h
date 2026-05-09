#pragma once

enum class SpecialItemFlag : short {
    SENSE = 0, //!< 簡易鑑定
    XXX01 = 1,
    EMPTY = 2, //!< 魔道具の空判定.
    KNOWN = 3, //!< 鑑定済
    STORE = 4, //!< 店頭購入(注：宝物庫報酬で使い回している).
    FULL_KNOWN = 5, //!< *鑑定* 済.
    XXX06 = 6,
    BROKEN = 7, //!< 呪い・マイナスエゴなど無価値品.
    MAX,
};
