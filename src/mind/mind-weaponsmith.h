#pragma once

// @todo 過去のセーブファイルからの移行用コードのコンパイルのためとりあえず残す
// 最終的には鍛冶師ではリファクタリングの前後でのセーブファイル移行を不可にする
#define MIN_SPECIAL_ESSENCE 200

struct player_type;
void do_cmd_kaji(player_type *player_ptr, bool only_browse);
