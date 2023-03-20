﻿/*!
 * @file main-win-music.cpp
 * @brief Windows版固有実装(BGM)
 */

#include "main-win/main-win-music.h"
#include "dungeon/quest.h"
#include "floor/floor-town.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-file-utils.h"
#include "main-win/main-win-mci.h"
#include "main-win/main-win-mmsystem.h"
#include "main-win/main-win-tokenizer.h"
#include "main-win/main-win-utils.h"
#include "main/scene-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "system/dungeon-info.h"
#include "system/monster-race-info.h"
#include "term/z-term.h"
#include "util/angband-files.h"
#include "world/world.h"
#include <algorithm>
#include <digitalv.h>
#include <limits>

bool use_pause_music_inactive = false;
static TERM_XTRA current_music_type = TERM_XTRA::MUSIC_MUTE;
static int current_music_id = 0;
// current filename being played
static char current_music_path[MAIN_WIN_MAX_PATH];

/*
 * Directory name
 */
concptr ANGBAND_DIR_XTRA_MUSIC;

/*
 * "music.cfg" data
 */
CfgData *music_cfg_data;

namespace main_win_music {

/*!
 * @brief action-valに対応する[Basic]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf 使用しない
 * @return 対応するキー名を返す
 */
static concptr basic_key_at(int index, char *buf)
{
    (void)buf;

    if (index >= MUSIC_BASIC_MAX) {
        return nullptr;
    }

    return angband_music_basic_name[index];
}

/*!
 * @brief action-valに対応する[Dungeon]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static concptr dungeon_key_at(int index, char *buf)
{
    if (index >= static_cast<int>(dungeons_info.size())) {
        return nullptr;
    }

    sprintf(buf, "dungeon%03d", index);
    return buf;
}

/*!
 * @brief action-valに対応する[Quest]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static concptr quest_key_at(int index, char *buf)
{
    const auto &quest_list = QuestList::get_instance();
    if (index > enum2i(quest_list.rbegin()->first)) {
        return nullptr;
    }

    sprintf(buf, "quest%03d", index);
    return buf;
}

/*!
 * @brief action-valに対応する[Town]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static concptr town_key_at(int index, char *buf)
{
    if (index >= static_cast<int>(max_towns)) {
        return nullptr;
    }

    sprintf(buf, "town%03d", index);
    return buf;
}

/*!
 * @brief action-valに対応する[Monster]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf バッファ
 * @return 対応するキー名を返す
 */
static concptr monster_key_at(int index, char *buf)
{
    if (index >= static_cast<int>(monraces_info.size())) {
        return nullptr;
    }

    sprintf(buf, "monster%04d", index);
    return buf;
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

    char device_type[256];
    GetPrivateProfileStringA("Device", "type", "MPEGVideo", device_type, _countof(device_type), reader.get_cfg_path());
    mci_device_type = to_wchar(device_type).wc_str();

    // clang-format off
    music_cfg_data = reader.read_sections({
        { "Basic", TERM_XTRA::MUSIC_BASIC, basic_key_at },
        { "Dungeon", TERM_XTRA::MUSIC_DUNGEON, dungeon_key_at },
        { "Quest", TERM_XTRA::MUSIC_QUEST, quest_key_at },
        { "Town", TERM_XTRA::MUSIC_TOWN, town_key_at },
        { "Monster", TERM_XTRA::MUSIC_MONSTER, monster_key_at, &has_monster_music }
        });
    // clang-format on

    if (!has_monster_music) {
        TERM_XTRA type = TERM_XTRA::MUSIC_BASIC;
        for (int val = MUSIC_BASIC_UNIQUE; val <= MUSIC_BASIC_HIGHER_LEVEL_MONSTER; val++) {
            if (music_cfg_data->has_key(enum2i(type), val)) {
                has_monster_music = true;
                break;
            }
        }
    }
}

/*
 * Stop a music
 */
errr stop_music(void)
{
    mciSendCommandW(mci_open_parms.wDeviceID, MCI_STOP, MCI_WAIT, 0);
    mciSendCommandW(mci_open_parms.wDeviceID, MCI_CLOSE, MCI_WAIT, 0);
    current_music_type = TERM_XTRA::MUSIC_MUTE;
    current_music_id = 0;
    strcpy(current_music_path, "\0");
    return 0;
}

/*
 * Play a music
 */
errr play_music(TERM_XTRA type, int val)
{
    if (type == TERM_XTRA::MUSIC_MUTE) {
        return stop_music();
    }

    if (current_music_type == type && current_music_id == val) {
        return 0;
    } // now playing

    concptr filename = music_cfg_data->get_rand(enum2i(type), val);
    if (!filename) {
        return 1;
    } // no setting

    char buf[MAIN_WIN_MAX_PATH];
    path_build(buf, MAIN_WIN_MAX_PATH, ANGBAND_DIR_XTRA_MUSIC, filename);

    if (current_music_type != TERM_XTRA::MUSIC_MUTE) {
        if (0 == strcmp(current_music_path, buf)) {
            return 0;
        }
    } // now playing same file

    current_music_type = type;
    current_music_id = val;
    strcpy(current_music_path, buf);

    to_wchar path(buf);
    mci_open_parms.lpstrDeviceType = mci_device_type.data();
    mci_open_parms.lpstrElementName = path.wc_str();
    mciSendCommandW(mci_open_parms.wDeviceID, MCI_STOP, MCI_WAIT, 0);
    mciSendCommandW(mci_open_parms.wDeviceID, MCI_CLOSE, MCI_WAIT, 0);
    mciSendCommandW(mci_open_parms.wDeviceID, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT | MCI_NOTIFY, (DWORD)&mci_open_parms);
    // Send MCI_PLAY in the notification event once MCI_OPEN is completed
    return 0;
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
errr play_music_scene(int val)
{
    // リストの先頭から順に再生を試み、再生できたら抜ける
    auto &list = get_scene_type_list(val);
    const errr err_sucsess = 0;
    for (auto &item : list) {
        if (main_win_music::play_music(item.type, item.val) == err_sucsess) {
            break;
        }
    }

    return 0;
}

void set_music_volume(int volume)
{
    if (current_music_type == TERM_XTRA::MUSIC_MUTE) {
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
