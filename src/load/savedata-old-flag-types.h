/*!
 * @file savedata-old-flag-types.h
 * @brief 過去に存在したセーブデータ有無フラグを定義する。古いセーブデータからのマイグレーション用。
 */

#pragma once

//! セーブデータバージョン7でart_flagsをFlagGroupクラスに移行する前のセーブデータ有無フラグ
enum class SavedataItemOlderThan7FlagType {
	ART_FLAGS0 = 0x00004000,
	ART_FLAGS1 = 0x00008000,
	ART_FLAGS2 = 0x00010000,
	ART_FLAGS3 = 0x00020000,
	ART_FLAGS4 = 0x10000000,
};

enum class SavedataItemOlderThan12FlagType {
    XTRA1 = 0x00100000,
    XTRA3 = 0x00400000,
};

enum class SavedataLoreOlderThan10FlagType {
    AURA_FIRE_OLD = 0x00004000,
    AURA_COLD_OLD = 0x00000400,
    AURA_ELEC_OLD = 0x00008000,
};
