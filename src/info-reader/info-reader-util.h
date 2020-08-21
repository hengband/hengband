#pragma once

#include "system/angband.h"

/*
 * Size of memory reserved for initialization of some arrays
 */
#define FAKE_NAME_SIZE 40 * 1024L /*!< ゲーム情報の種別毎に用意される名前用バッファの容量 */
#define FAKE_TEXT_SIZE 150 * 1024L /*!< ゲーム情報の種別毎に用意されるテキスト用バッファの容量 */
#define FAKE_TAG_SIZE 10 * 1024L /*!< ゲーム情報の種別毎に用意されるタグ用バッファの容量 */

extern int error_idx;
extern int error_line;

typedef struct angband_header angband_header;
bool add_text(u32b *offset, angband_header *head, concptr buf, bool normal_text);
bool add_name(u32b *offset, angband_header *head, concptr buf);
bool add_tag(STR_OFFSET *offset, angband_header *head, concptr buf);
errr grab_one_flag(u32b *flags, concptr names[], concptr what);
byte grab_one_activation_flag(concptr what);
