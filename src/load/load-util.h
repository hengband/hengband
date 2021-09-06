#pragma once

#include "system/angband.h"

#include <algorithm>
#include <bitset>

extern FILE *loading_savefile;
extern uint32_t loading_savefile_version;
extern byte load_xor_byte;
extern uint32_t v_check;
extern uint32_t x_check;
extern byte kanji_code;

void load_note(concptr msg);
byte sf_get(void);
void rd_byte(byte *ip);
void rd_u16b(uint16_t *ip);
void rd_s16b(int16_t *ip);
void rd_u32b(uint32_t *ip);
void rd_s32b(int32_t *ip);
void rd_string(char *str, int max);
void strip_bytes(int n);
bool loading_savefile_version_is_older_than(uint32_t version);

/**
 * @brief ビットフラグデータをFlagGroupに反映させる
 *
 * @tparam FG FlagGroupクラスのテンプレート型引数等を含めた型
 * @tparam BitFlagType ビットフラグデータの型
 * @param flaggroup ビットフラグデータを反映させるFlagGroupオブジェクトの参照
 * @param bitflags ビットフラグデータの内容
 * @param start_pos ビットフラグデータを反映させるFlagGroupオブジェクトのフラグ番号開始位置
 */
template <typename FG, typename BitFlagType>
void migrate_bitflag_to_flaggroup(FG &flaggroup, BitFlagType bitflags, uint start_pos = 0)
{
    if (start_pos >= flaggroup.size()) {
        return;
    }

    std::bitset<sizeof(BitFlagType) * 8> bs(bitflags);
    for (size_t i = 0, count = std::min(flaggroup.size() - start_pos, bs.size()); i < count; i++) {
        auto f = static_cast<typename FG::flag_type>(start_pos + i);
        flaggroup[f] = bs[i];
    }
}
