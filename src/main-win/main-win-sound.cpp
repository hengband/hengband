/*!
 * @file main-win-sound.cpp
 * @brief Windows版固有実装(効果音)
 */

#define NOMINMAX

#include "main-win/main-win-sound.h"
#include "main-win/main-win-cfg-reader.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-file-utils.h"
#include "main-win/main-win-utils.h"
#include "main-win/wav-reader.h"
#include "main/sound-definitions-table.h"
#include "util/angband-files.h"
#include <memory>
#include <mmsystem.h>
#include <queue>

/*
 * Directory name
 */
std::filesystem::path ANGBAND_DIR_XTRA_SOUND;

/*
 * "sound.cfg" data
 */
CfgData *sound_cfg_data;

/*!
 * 効果音データ
 */
struct sound_res {
    sound_res(BYTE *_buf)
    {
        buf.reset(_buf);
    }
    sound_res(const sound_res &) = delete;
    sound_res &operator=(const sound_res &) = delete;

    ~sound_res()
    {
        dispose();
    }

    HWAVEOUT hwo = NULL;
    /*!
     * PCMデータバッファ
     */
    std::unique_ptr<BYTE[]> buf;
    WAVEHDR wh = { 0 };

    /*!
     * 再生完了判定
     * @retval true 完了
     * @retval false 再生中
     */
    bool isDone()
    {
        return (this->hwo == NULL) || (this->wh.dwFlags & WHDR_DONE);
    }

    void dispose()
    {
        if (hwo != NULL) {
            ::waveOutReset(hwo);
            ::waveOutUnprepareHeader(hwo, &wh, sizeof(WAVEHDR));
            ::waveOutClose(hwo);
            hwo = NULL;
            wh.lpData = NULL;
        }
    }
};
/*!
 * 効果音リソースの管理キュー
 */
std::queue<sound_res *> sound_queue;

/*!
 * @brief PCMデータの振幅を変調する
 *
 * 引数で与えられたPCMデータの振幅を mult/div 倍に変調する。
 * PCMデータのサンプリングビット数は bits_per_sample 引数で指定する。
 * 変調した値がサンプリングビット数で表現可能な範囲に収まらない場合は表現可能な範囲の最大値/最小値に制限する。
 * サンプリングビット数で有効なのは 8 か 16 のみであり、それ以外の値が指定された場合はなにも行わない。
 *
 * @param bits_per_sample PCMデータのサンプリングビット数 (8 or 16)
 * @param pcm_buf PCMデータバッファ領域へのポインタ
 * @param bufsize PCMデータバッファ領域のサイズ
 * @param mult 振幅変調倍率 mult/div の mult
 * @param div 振幅変調倍率 mult/div の div
 */
static void modulate_amplitude(int bits_per_sample, BYTE *pcm_buf, size_t bufsize, int mult, int div)
{
    auto modulate = [mult, div](auto sample) {
        using sample_t = decltype(sample);
        constexpr auto min = std::numeric_limits<sample_t>::min();
        constexpr auto max = std::numeric_limits<sample_t>::max();
        const auto modulated_sample = std::clamp<int>(sample * mult / div, min, max);
        return static_cast<sample_t>(modulated_sample);
    };

    switch (bits_per_sample) {
    case 8:
        for (auto i = 0U; i < bufsize; ++i) {
            pcm_buf[i] = modulate(pcm_buf[i]);
        }
        break;

    case 16:
        for (auto i = 0U; i < bufsize; i += 2) {
            const auto sample = static_cast<int16_t>((static_cast<uint16_t>(pcm_buf[i + 1]) << 8) | static_cast<uint16_t>(pcm_buf[i]));
            const auto modulated_sample = modulate(sample);
            pcm_buf[i + 1] = static_cast<uint16_t>(modulated_sample) >> 8;
            pcm_buf[i] = static_cast<uint16_t>(modulated_sample) & 0xFF;
        }
        break;

    default:
        break;
    }
}

/*!
 * 効果音の再生と管理キューへの追加.
 *
 * @param wf WAVEFORMATEXへのポインタ
 * @param buf PCMデータバッファ。使用後にdelete[]すること。
 * @param bufsize バッファサイズ
 * @retval true 正常に処理された
 * @retval false 処理エラー
 */
static bool add_sound_queue(const WAVEFORMATEX *wf, BYTE *buf, DWORD bufsize, int volume)
{
    // 再生完了データをキューから削除する
    while (!sound_queue.empty()) {
        auto res = sound_queue.front();
        if (res->isDone()) {
            delete res;
            sound_queue.pop();
            continue;
        }
        break;
    }

    auto res = new sound_res(buf);
    sound_queue.push(res);

    MMRESULT mr = ::waveOutOpen(&res->hwo, WAVE_MAPPER, wf, NULL, NULL, CALLBACK_NULL);
    if (mr != MMSYSERR_NOERROR) {
        return false;
    }

    modulate_amplitude(wf->wBitsPerSample, buf, bufsize, volume, SOUND_VOLUME_MAX);

    WAVEHDR *wh = &res->wh;
    wh->lpData = (LPSTR)buf;
    wh->dwBufferLength = bufsize;
    wh->dwFlags = 0;

    mr = ::waveOutPrepareHeader(res->hwo, wh, sizeof(WAVEHDR));
    if (mr != MMSYSERR_NOERROR) {
        res->dispose();
        return false;
    }

    mr = ::waveOutWrite(res->hwo, wh, sizeof(WAVEHDR));
    if (mr != MMSYSERR_NOERROR) {
        res->dispose();
        return false;
    }

    while (sound_queue.size() >= 16) {
        auto over = sound_queue.front();
        delete over;
        sound_queue.pop();
    }

    return true;
}

/*!
 * 指定ファイルを再生する
 *
 * @param buf ファイル名
 * @retval true 正常に処理された
 * @retval false 処理エラー
 */
static bool play_sound_impl(char *filename, int volume)
{
    wav_reader reader;
    if (!reader.open(filename)) {
        return false;
    }
    auto wf = reader.get_waveformat();

    auto data_buffer = reader.read_data();
    if (data_buffer == NULL) {
        return false;
    }

    return add_sound_queue(wf, data_buffer, reader.get_data_chunk()->cksize, volume);
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

    if (index >= SOUND_MAX) {
        return nullptr;
    }

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
 * @brief 効果音の終了処理
 */
void finalize_sound(void)
{
    while (!sound_queue.empty()) {
        auto res = sound_queue.front();
        delete res;
        sound_queue.pop();
    }
}

/*!
 * @brief 指定の効果音を鳴らす。
 * @param val see sound_type
 * @retval 0 正常終了
 * @retval 1 設定なし
 * @retval -1 PlaySoundの戻り値が正常終了以外
 */
int play_sound(int val, int volume)
{
    auto filename = sound_cfg_data->get_rand(TERM_XTRA_SOUND, val);
    if (!filename) {
        return 1;
    }

    char buf[MAIN_WIN_MAX_PATH];
    path_build(buf, MAIN_WIN_MAX_PATH, ANGBAND_DIR_XTRA_SOUND, filename);
    if (play_sound_impl(buf, volume)) {
        return 0;
    }

    return -1;
}
