/*!
 * @file savedata-old-flag-types.h
 * @brief 過去に存在したセーブデータ有無フラグを定義する。古いセーブデータからのマイグレーション用。
 */

#pragma once

//! セーブデータバージョン7でart_flagsをFlagGroupクラスに移行する前のセーブデータ有無フラグ
enum savedata_item_older_than_7_flag_type {
	SAVE_ITEM_OLDER_THAN_7_ART_FLAGS0 = 0x00004000,
	SAVE_ITEM_OLDER_THAN_7_ART_FLAGS1 = 0x00008000,
	SAVE_ITEM_OLDER_THAN_7_ART_FLAGS2 = 0x00010000,
	SAVE_ITEM_OLDER_THAN_7_ART_FLAGS3 = 0x00020000,
	SAVE_ITEM_OLDER_THAN_7_ART_FLAGS4 = 0x10000000,
};
