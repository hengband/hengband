#pragma once

#include "main-win/main-win-cfg-reader.h"
#include <array>
#include <filesystem>

extern std::filesystem::path ANGBAND_DIR_XTRA_SOUND;
extern CfgData *sound_cfg_data;

void load_sound_prefs(void);
void finalize_sound(void);
int play_sound(int val, int volume);

/*! 音量 100%,90%,…,10% それぞれに割り当てる実際の値(効果音の音声データの振幅を volume/SOUND_VOLUME_MAX 倍する) */
constexpr std::array<int, 10> SOUND_VOLUME_TABLE = { { 1000, 800, 600, 450, 350, 250, 170, 100, 50, 20 } };
constexpr auto SOUND_VOLUME_MAX = SOUND_VOLUME_TABLE.front();
