/*!
 * @file unix-sound.cpp
 * @brief Unix版固有実装(効果音)
 * @details posix_spawnp()を使用し非同期的にSEを再生する｡
 * 生成した子プロセスは終了時Linux環境下でゾンビプロセスとなってしまうため､
 * 溢れないよう再生時にcleanup_processes()で都度回収する｡
 * @note 設計の都合上1~QueueLimit個のゾンビプロセスを残してしまう｡
 */
#include "main-unix/unix-sound.h"
#include "main-unix/unix-cfg-reader.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "term/z-term.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <queue>
#include <ranges>
#include <simpleini/SimpleIni.h>
#include <spawn.h>
#include <sys/wait.h>
#include <tl/optional.hpp>
#include <vector>

extern char **environ;
namespace unix_sound {

/*
 * "sound.cfg" data
 */
static tl::optional<CfgData> sound_cfg_data;

static std::filesystem::path sound_player;

static std::filesystem::path ANGBAND_DIR_XTRA_SOUND;
static std::queue<pid_t> sound_pids;
static constexpr auto QueueLimit = 8;

/*!
 * @brief action-valに対応する[Sound]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf 使用しない
 * @return 対応するキー名を返す
 */
static tl::optional<std::string>
sound_key_at(int index)
{
    const auto sk = i2enum<SoundKind>(index);
    if (sk >= SoundKind::MAX) {
        return tl::nullopt;
    }

    return sound_names.at(sk);
}

/*!
 * @brief cleanup finished child processes
 *
 */
void cleanup_processes()
{
    while (!sound_pids.empty()) {
        auto status = 0;
        auto pid = sound_pids.front();
        auto ret = waitpid(pid, &status, WNOHANG);
        if (ret > 0) {
            sound_pids.pop(); // Process finished
        } else if (ret == 0) {
            break; // Front process still running

        } else { // error handling
            auto err = errno;
            if (err == ECHILD) {
                sound_pids.pop(); // Invalid pid
            } else {
                break; // Unexpected error; avoid spinning
            }
        }
    }
    return;
}

/*!
 * @brief 効果音の設定を読み込む。
 * @retval 正常に設定を読み込めたらtrueを返す
 * @details
 * "sound_debug.cfg"ファイルを優先して読み込む。無ければ"sound.cfg"ファイルを読み込む。
 * この処理は複数回実行されることを想定していない。複数回実行した場合には前回読み込まれた設定のメモリは解放されない。
 */
bool init_sound(std::filesystem::path sound_path, std::filesystem::path player)
{
    ANGBAND_DIR_XTRA_SOUND = sound_path;

    if (!std::filesystem::exists(player)) {
        return false;
    }

    sound_player = player;
    CfgReader reader(sound_path, { "sound_debug.cfg", "sound.cfg" });
    sound_cfg_data = reader.read_sections({ { "Sound", TERM_XTRA_SOUND, sound_key_at } });
    if (!sound_cfg_data.has_value()) {
        return false;
    }
    return true;
}

void finalize_sound()
{
    cleanup_processes();
    return;
}

/*!
 * @brief 指定の効果音を鳴らす。
 * @param val see SoundKind
 * @retval true 正常終了
 * @retval false 正常終了以外
 */
bool play_sound(int val)
{
    if (!sound_cfg_data) {
        return false;
    }
    if ((val < 0) || (val >= enum2i(SoundKind::MAX))) {
        return false;
    }

    if (!sound_pids.empty()) {
        cleanup_processes();
    }

    if (sound_pids.size() > QueueLimit) {
        return false;
    }

    auto filename = sound_cfg_data->get_rand(TERM_XTRA_SOUND, val);
    if (!filename) {
        return false;
    }

    auto path = path_build(ANGBAND_DIR_XTRA_SOUND, *filename);
    auto sound_player_pid = 0;

    auto argv_sound_player_buf = sound_player.string() | std::ranges::to<std::vector>();
    argv_sound_player_buf.push_back('\0');
    auto argv_path_sound_buf = path.string() | std::ranges::to<std::vector>();
    argv_path_sound_buf.push_back('\0');
    //  argv must has null end
    char *const argv[] = { argv_sound_player_buf.data(), argv_path_sound_buf.data(), nullptr };
    // explicitly pass environment
    char **envp = environ;
    auto ret = posix_spawnp(&sound_player_pid, sound_player.c_str(), nullptr, nullptr, argv, envp);
    if (ret != 0) { // failed to spawn process
        return false;
    }

    sound_pids.push(sound_player_pid);
    return true;
}
};
