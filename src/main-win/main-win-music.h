#pragma once

#include <array>
#include <filesystem>
#include <tl/optional.hpp>
#include <windows.h>

class CfgData;
extern bool use_pause_music_inactive;
extern std::filesystem::path ANGBAND_DIR_XTRA_MUSIC;
extern tl::optional<CfgData> music_cfg_data;

namespace main_win_music {
void load_music_prefs();
void stop_music();
bool play_music(int type, int val);
void play_music_scene(int val);
void pause_music();
void resume_music();
void set_music_volume(int volume);
void on_mci_notify(WPARAM wFlags, LONG lDevID, int volume);

/*! 音量 100%,90%,…,10% それぞれに割り当てる実際の値(音量の指定可能範囲:0～1000) */
constexpr std::array<int, 10> VOLUME_TABLE = { { 1000, 800, 600, 450, 350, 250, 170, 100, 50, 20 } };
}
