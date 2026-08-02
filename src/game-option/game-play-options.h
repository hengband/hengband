#pragma once

#include "system/angband.h"

extern bool stack_force_notes; /* Merge inscriptions when stacking */
extern bool stack_force_costs; /* Merge discounts when stacking */
extern bool expand_list; /* Expand the power of the list commands */
extern bool allow_smallest_floor; //!< 最小面積(金鉱/迷宮と同じ)のフロアを他ダンジョンでも生成可能にする.
extern bool always_small_floor; //!< 常に小さいフロアを生成する(金鉱4ブロック分以下のフロア生成しか許可しない。但しLARGE/LARGESTフラグがついていれば対象外).
extern bool allow_largest_floor; //!< 最大面積のフロアを他ダンジョンでも生成可能にする.
extern bool always_large_floor; //!< 常に大きいフロアを生成する(4ブロック分以上のフロア生成しか許可しない。但しSMALL/SMALLESTフラグがついていれば対象外).
extern bool allow_arena_floor; //!< 空っぽの「アリーナ」レベルの生成を可能にする.
extern bool bound_walls_perm; /* Boundary walls become 'permanent wall' */
extern bool last_words; /* Leave last words when your character dies */
extern bool auto_dump; /* Dump a character record automatically */
extern bool send_score; /* Send score dump to the world score server */
extern bool allow_debug_opts; /* Allow use of debug/cheat options */
