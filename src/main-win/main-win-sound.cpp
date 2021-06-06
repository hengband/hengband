/*!
 * @file main-win-sound.cpp
 * @brief Windows版固有実装(効果音)
 */

#include "main-win/audio-win.h"
#include "main-win/main-win-sound.h"
#include "main-win/main-win-cfg-reader.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-file-utils.h"
#include "main-win/main-win-utils.h"
#include "main-win/wav-reader.h"
#include "util/angband-files.h"

#include "main/sound-definitions-table.h"

/*
 * Directory name
 */
concptr ANGBAND_DIR_XTRA_SOUND;

/*
 * "sound.cfg" data
 */
CfgData *sound_cfg_data;

static bool play_sound_impl(char *filename)
{
    if (!can_audio())
        return false;

    wav_reader reader;
    if (!reader.open(filename))
        return false;
    auto wf = reader.get_waveformat();

    auto data_buffer = reader.read_data();
    if (data_buffer == nullptr)
        return false;

    return add_sound_queue(wf, data_buffer, reader.get_data_chunk()->cksize);
}

/*!
 * @brief action-valに対応する[Sound]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf 使用しない
 * @return 対応するキー名を返す
 */
static concptr sound_key_at(int index, char *buf)
{
    (void)buf;

    if (index >= SOUND_MAX)
        return NULL;

    return angband_sound_name[index];
}

/*!
 * @brief 効果音の設定を読み込む。
 * @details
 * "sound_debug.cfg"ファイルを優先して読み込む。無ければ"sound.cfg"ファイルを読み込む。
 * この処理は複数回実行されることを想定していない。複数回実行した場合には前回読み込まれた設定のメモリは解放されない。
 */
void load_sound_prefs(void)
{
    CfgReader reader(ANGBAND_DIR_XTRA_SOUND, { "sound_debug.cfg", "sound.cfg" });
    sound_cfg_data = reader.read_sections({ { "Sound", TERM_XTRA_SOUND, sound_key_at } });
}

/*!
 * @brief 指定の効果音を鳴らす。
 * @param val see sound_type
 * @retval 0 正常終了
 * @retval 1 設定なし
 * @retval -1 PlaySoundの戻り値が正常終了以外
 */
errr play_sound(int val)
{
    concptr filename = sound_cfg_data->get_rand(TERM_XTRA_SOUND, val);
    if (!filename) {
        return 1;
    }

    char buf[MAIN_WIN_MAX_PATH];
    path_build(buf, MAIN_WIN_MAX_PATH, ANGBAND_DIR_XTRA_SOUND, filename);

    if (play_sound_impl(buf)) {
        return 0;
    }

    return -1;
}
