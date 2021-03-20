/*!
 * @file main-win-sound.cpp
 * @brief Windows版固有実装(効果音)
 */

#include "main-win/main-win-define.h"
#include "main-win/main-win-file-utils.h"
#include "main-win/main-win-mci.h"
#include "main-win/main-win-mmsystem.h"
#include "main-win/main-win-sound.h"
#include "main-win/main-win-tokenizer.h"
#include "util/angband-files.h"

/*
 * An array of sound file names
 */
concptr sound_file[SOUND_MAX][SAMPLE_SOUND_MAX];

/*
 * Directory name
 */
concptr ANGBAND_DIR_XTRA_SOUND;

void load_sound_prefs(void)
{
    char tmp[MAIN_WIN_MAX_PATH];
    char ini_path[MAIN_WIN_MAX_PATH];
    char wav_path[MAIN_WIN_MAX_PATH];
    char *zz[SAMPLE_SOUND_MAX];

    // FIXME sound.cfgとmusic.cfgに共通する「[Device]tyepe=～」項目の読込先変数が同じため競合する
    // FIXME 効果音再生のPlaySound APIはMCIとは別のため、現在の所は効果音用のデバイスタイプ設定は不要
    path_build(ini_path, MAIN_WIN_MAX_PATH, ANGBAND_DIR_XTRA_SOUND, "sound_debug.cfg");
    if (GetPrivateProfileString("Device", "type", "", mci_device_type, _countof(mci_device_type), ini_path) == 0) {
        path_build(ini_path, MAIN_WIN_MAX_PATH, ANGBAND_DIR_XTRA_SOUND, "sound.cfg");
        GetPrivateProfileString("Device", "type", "", mci_device_type, _countof(mci_device_type), ini_path);
    }

    for (int i = 0; i < SOUND_MAX; i++) {
        GetPrivateProfileString("Sound", angband_sound_name[i], "", tmp, MAIN_WIN_MAX_PATH, ini_path);
        int num = tokenize_whitespace(tmp, SAMPLE_SOUND_MAX, zz);
        for (int j = 0; j < num; j++) {
            path_build(wav_path, MAIN_WIN_MAX_PATH, ANGBAND_DIR_XTRA_SOUND, zz[j]);
            if (check_file(wav_path))
                sound_file[i][j] = string_make(zz[j]);
        }
    }
}

errr play_sound(int val)
{
    char buf[MAIN_WIN_MAX_PATH];
    if ((val < 0) || (val >= SOUND_MAX))
        return 1;

    int i;
    for (i = 0; i < SAMPLE_SOUND_MAX; i++) {
        if (!sound_file[val][i])
            break;
    }

    if (i == 0)
        return 1;

    path_build(buf, 1024, ANGBAND_DIR_XTRA_SOUND, sound_file[val][Rand_external(i)]);
    return (PlaySound(buf, 0, SND_FILENAME | SND_ASYNC));
}
