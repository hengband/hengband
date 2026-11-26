#pragma once

#include <filesystem>

namespace unix_music {
void init_music(std::filesystem::path music_path);
void stop_music();
bool play_music(int type, int val);
void play_music_scene(int val);
void pause_music();
void resume_music();
// void set_music_volume(int volume);
// void on_mci_notify(WPARAM wFlags, LONG lDevID, int volume);

/*! 音量 100%,90%,…,10% それぞれに割り当てる実際の値(音量の指定可能範囲:0～1000) */
// constexpr std::array<int, 10> VOLUME_TABLE = { { 1000, 800, 600, 450, 350, 250, 170, 100, 50, 20 } };
}
