#pragma once

typedef enum
{
	SPELL_NAME = 0,
	SPELL_DESC = 1,
	SPELL_INFO = 2,
	SPELL_CAST = 3,
	SPELL_FAIL = 4,
	SPELL_STOP = 5,
	SPELL_CONT = 6
} spell_type;

typedef enum
{
	SPOP_DISPLAY_MES = 0x0001,		// !< スペル処理オプション … メッセージを表示する
	SPOP_NO_UPDATE = 0x0002,		// !< スペル処理オプション … ステータス更新を解決後行う
	SPOP_DEBUG = 0x8000,			// !< スペル処理オプション … デバッグ処理あり
} spell_operation;
