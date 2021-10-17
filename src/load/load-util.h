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
byte rd_byte();
uint16_t rd_u16b();
int16_t rd_s16b();
uint32_t rd_u32b();
int32_t rd_s32b();
void rd_string(char *str, int max);
void strip_bytes(int n);
bool loading_savefile_version_is_older_than(uint32_t version);

/**
 * @brief ビットフラグデータをFlagGroupに反映させる
 *
 * @tparam FG FlagGroupクラスのテンプレート型引数等を含めた型
 * @tparam BitFlagType ビットフラグデータの型
 * @param flaggroup ビットフラグデータを反映させるFlagGroupオブジェクトの参照
 * @param bitflag ビットフラグデータの内容
 * @param start_pos ビットフラグデータを反映させるFlagGroupオブジェクトのフラグ番号開始位置。省略した場合は0(先頭)。
 * @param count_max ビットフラグデータから反映する最大ビット数。省略した場合はビットフラグデータ型のビット数(全ビットを反映する)。
 */
template <typename FG, typename BitFlagType, size_t BIT_COUNT = sizeof(BitFlagType) * 8>
void migrate_bitflag_to_flaggroup(FG &flaggroup, BitFlagType bitflag, uint start_pos = 0, size_t count_max = BIT_COUNT)
{
    if (start_pos >= flaggroup.size()) {
        return;
    }

    std::bitset<BIT_COUNT> bs(bitflag);
    for (size_t i = 0, count = std::min(flaggroup.size() - start_pos, count_max); i < count; i++) {
        auto f = static_cast<typename FG::flag_type>(start_pos + i);
        flaggroup[f] = bs[i];
    }
}
