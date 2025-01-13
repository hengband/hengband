/*!
 * @file main-win-music.cpp
 * @brief Windows版固有実装(BGM)
 */

#include "main-win/main-win-music.h"
#include "dungeon/quest.h"
#include "main-win/main-win-cfg-reader.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-mci.h"
#include "main-win/main-win-mmsystem.h"
#include "main-win/main-win-tokenizer.h"
#include "main-win/main-win-utils.h"
#include "main/music-definitions-table.h"
#include "main/scene-table.h"
#include "main/sound-of-music.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/monrace/monrace-list.h"
#include "term/z-term.h"
#include "util/angband-files.h"
#include "world/world.h"
#include <algorithm>
#include <digitalv.h>
#include <limits>
#include <string>

bool use_pause_music_inactive = false;
static int current_music_type = TERM_XTRA_MUSIC_MUTE;
static int current_music_id = 0;
// current filename being played
static std::filesystem::path current_music_path;

/*
 * Directory name
 */
std::filesystem::path ANGBAND_DIR_XTRA_MUSIC;

/*
 * "music.cfg" data
 */
std::optional<CfgData> music_cfg_data;

namespace main_win_music {

/*!
 * @brief action-valに対応する[Basic]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf 使用しない
 * @return 対応するキー名を返す
 */
static std::optional<std::string> basic_key_at(int index)
{
    if (index >= MUSIC_BASIC_MAX) {
        return std::nullopt;
    }

    return angband_music_basic_name[index];
}

/*!
 * @brief action-valに対応する[Dungeon]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static std::optional<std::string> dungeon_key_at(int index)
{
    if (index >= static_cast<int>(DungeonList::get_instance().size())) {
        return std::nullopt;
    }

    return format("dungeon%03d", index);
}

/*!
 * @brief action-valに対応する[Quest]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static std::optional<std::string> quest_key_at(int index)
{
    const auto &quests = QuestList::get_instance();
    if (index > enum2i(quests.rbegin()->first)) {
        return std::nullopt;
    }

    return format("quest%03d", index);
}

/*!
 * @brief action-valに対応する[Town]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static std::optional<std::string> town_key_at(int index)
{
    if (index >= static_cast<int>(towns_info.size())) {
        return std::nullopt;
    }

    return format("town%03d", index);
}

/*!
 * @brief action-valに対応する[Monster]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static std::optional<std::string> monster_key_at(int index)
{
    if (index >= static_cast<int>(MonraceList::get_instance().size())) {
        return std::nullopt;
    }

    return format("monster%04d", index);
}

/*!
 * @brief BGMの設定を読み込む。
 * @details
 * "music_debug.cfg"ファイルを優先して読み込む。無ければ"music.cfg"ファイルを読み込む。
 * この処理は複数回実行されることを想定していない。複数回実行した場合には前回読み込まれた設定のメモリは解放されない。
 */
void load_music_prefs()
{
    CfgReader reader(ANGBAND_DIR_XTRA_MUSIC, { "music_debug.cfg", "music.cfg" });
    constexpr auto size = 256;
    char device_type[size];
    GetPrivateProfileStringA("Device", "type", "MPEGVideo", device_type, size, reader.get_cfg_path().string().data());
    mci_device_type = to_wchar(device_type).wc_str();

    // clang-format off
    music_cfg_data = reader.read_sections({
        { "Basic", TERM_XTRA_MUSIC_BASIC, basic_key_at },
        { "Dungeon", TERM_XTRA_MUSIC_DUNGEON, dungeon_key_at },
        { "Quest", TERM_XTRA_MUSIC_QUEST, quest_key_at },
        { "Town", TERM_XTRA_MUSIC_TOWN, town_key_at },
        { "Monster", TERM_XTRA_MUSIC_MONSTER, monster_key_at, &has_monster_music }
        });
    // clang-format on

    if (!has_monster_music) {
        int type = TERM_XTRA_MUSIC_BASIC;
        for (int val = MUSIC_BASIC_UNIQUE; val <= MUSIC_BASIC_HIGHER_LEVEL_MONSTER; val++) {
            if (music_cfg_data->has_key(type, val)) {
                has_monster_music = true;
                break;
            }
        }
    }
}

/*
 * Stop a music
 */
void stop_music()
{
    mciSendCommandW(mci_open_parms.wDeviceID, MCI_STOP, MCI_WAIT, 0);
    mciSendCommandW(mci_open_parms.wDeviceID, MCI_CLOSE, MCI_WAIT, 0);
    current_music_type = TERM_XTRA_MUSIC_MUTE;
    current_music_id = 0;
    current_music_path = "";
}

/*
 * Play a music
 */
bool play_music(int type, int val)
{
    if (type == TERM_XTRA_MUSIC_MUTE) {
        stop_music();
        return true;
    }

    if (current_music_type == type && current_music_id == val) {
        return true;
    } // now playing

    if (!music_cfg_data) {
        return false;
    }

    auto filename = music_cfg_data->get_rand(type, val);
    if (!filename) {
        return false;
    } // no setting

    auto path_music = path_build(ANGBAND_DIR_XTRA_MUSIC, *filename);
    if (current_music_type != TERM_XTRA_MUSIC_MUTE) {
        if (current_music_path == path_music) {
            return true;
        }
    } // now playing same file

    current_music_type = type;
    current_music_id = val;
    current_music_path = path_music;

    const auto path_music_str = path_music.wstring();
    mci_open_parms.lpstrDeviceType = mci_device_type.data();
    mci_open_parms.lpstrElementName = path_music_str.data();
    mciSendCommandW(mci_open_parms.wDeviceID, MCI_STOP, MCI_WAIT, 0);
    mciSendCommandW(mci_open_parms.wDeviceID, MCI_CLOSE, MCI_WAIT, 0);
    mciSendCommandW(mci_open_parms.wDeviceID, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT | MCI_NOTIFY, (DWORD)&mci_open_parms);
    // Send MCI_PLAY in the notification event once MCI_OPEN is completed
    return true;
}

/*
 * Pause a music
 */
void pause_music(void)
{
    mciSendCommandW(mci_open_parms.wDeviceID, MCI_PAUSE, MCI_WAIT, 0);
}

/*
 * Resume a music
 */
void resume_music(void)
{
    mciSendCommandW(mci_open_parms.wDeviceID, MCI_RESUME, MCI_WAIT, 0);
}

/*
 * Play a music matches a situation
 */
void play_music_scene(int val)
{
    // リストの先頭から順に再生を試み、再生できたら抜ける
    auto &list = get_scene_type_list(val);
    for (auto &item : list) {
        if (play_music(item.type, item.val)) {
            break;
        }
    }
}

void set_music_volume(int volume)
{
    if (current_music_type == TERM_XTRA_MUSIC_MUTE) {
        return;
    }

    MCI_DGV_SETAUDIO_PARMSW mci_vol{};
    mci_vol.dwItem = MCI_DGV_SETAUDIO_VOLUME;
    mci_vol.dwValue = volume;
    mciSendCommandW(
        mci_open_parms.wDeviceID,
        MCI_SETAUDIO,
        MCI_DGV_SETAUDIO_ITEM | MCI_DGV_SETAUDIO_VALUE,
        (DWORD)&mci_vol);
}

/*
 * Notify event
 */
void on_mci_notify(WPARAM wFlags, LONG lDevID, int volume)
{
    if (wFlags == MCI_NOTIFY_SUCCESSFUL) {
        // play a music (repeat)
        set_music_volume(volume);
        mciSendCommandW(lDevID, MCI_SEEK, MCI_SEEK_TO_START | MCI_WAIT, 0);
        mciSendCommandW(lDevID, MCI_PLAY, MCI_NOTIFY, (DWORD)&mci_play_parms);
    }
}

}
