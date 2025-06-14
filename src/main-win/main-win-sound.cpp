/*!
 * @file main-win-sound.cpp
 * @brief Windows版固有実装(効果音)
 */

#define NOMINMAX

#include "main-win/main-win-sound.h"
#include "main-win/main-win-cfg-reader.h"
#include "main-win/main-win-define.h"
#include "main-win/main-win-utils.h"
#include "main-win/wav-reader.h"
#include "main/sound-definitions-table.h"
#include "util/angband-files.h"
#include "util/enum-converter.h"
#include <memory>
#include <mmsystem.h>
#include <queue>
#include <span>

/*
 * Directory name
 */
std::filesystem::path ANGBAND_DIR_XTRA_SOUND;

/*
 * "sound.cfg" data
 */
tl::optional<CfgData> sound_cfg_data;

/*!
 * 効果音データ
 */
struct sound_res {
    sound_res(std::vector<uint8_t> &&_buf)
        : buf(std::move(_buf))
    {
        this->wh.lpData = reinterpret_cast<LPSTR>(this->buf.data());
        this->wh.dwBufferLength = this->buf.size();
        this->wh.dwFlags = 0;
    }
    sound_res(const sound_res &) = delete;
    sound_res &operator=(const sound_res &) = delete;

    ~sound_res()
    {
        if (this->hwo == nullptr) {
            return;
        }

        ::waveOutReset(this->hwo);
        ::waveOutUnprepareHeader(this->hwo, &this->wh, sizeof(WAVEHDR));
        ::waveOutClose(this->hwo);
    }

    HWAVEOUT hwo = NULL;
    /*!
     * PCMデータバッファ
     */
    std::vector<uint8_t> buf;
    WAVEHDR wh{};

    /*!
     * 再生完了判定
     * @retval true 完了
     * @retval false 再生中
     */
    bool isDone() const
    {
        return (this->hwo == NULL) || (this->wh.dwFlags & WHDR_DONE);
    }
};

/*!
 * 効果音リソースの管理キュー
 */
std::queue<std::unique_ptr<sound_res>> sound_queue;

/*!
 * @brief PCMデータの振幅を変調する
 *
 * 引数で与えられたPCMデータの振幅を mult/div 倍に変調する。
 * PCMデータのサンプリングビット数は bits_per_sample 引数で指定する。
 * 変調した値がサンプリングビット数で表現可能な範囲に収まらない場合は表現可能な範囲の最大値/最小値に制限する。
 * サンプリングビット数で有効なのは 8 か 16 のみであり、それ以外の値が指定された場合はなにも行わない。
 *
 * @param bits_per_sample PCMデータのサンプリングビット数 (8 or 16)
 * @param pcm_buf PCMデータ
 * @param mult 振幅変調倍率 mult/div の mult
 * @param div 振幅変調倍率 mult/div の div
 */
static void modulate_amplitude(int bits_per_sample, std::span<uint8_t> pcm_buf, int mult, int div)
{
    auto modulate = [mult, div](auto sample, int standard = 0) {
        using sample_t = decltype(sample);
        constexpr auto min = std::numeric_limits<sample_t>::min();
        constexpr auto max = std::numeric_limits<sample_t>::max();
        const auto diff = sample - standard;
        const auto modulated_sample = std::clamp<int>(standard + diff * mult / div, min, max);
        return static_cast<sample_t>(modulated_sample);
    };

    switch (bits_per_sample) {
    case 8:
        for (auto &sample : pcm_buf) {
            sample = modulate(sample, 128);
        }
        break;

    case 16:
        for (auto i = 0; i < std::ssize(pcm_buf) - 1; i += 2) {
            const auto sample = static_cast<int16_t>(pcm_buf[i] | (pcm_buf[i + 1] << 8));
            const auto modulated_sample = static_cast<uint16_t>(modulate(sample));
            pcm_buf[i + 1] = modulated_sample >> 8;
            pcm_buf[i] = modulated_sample & 0xff;
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
 * @param buf PCMデータバッファ
 * @retval true 正常に処理された
 * @retval false 処理エラー
 */
static bool add_sound_queue(const WAVEFORMATEX *wf, std::vector<uint8_t> &&buf, int volume)
{
    if (buf.empty()) {
        return false;
    }

    // 再生完了データをキューから削除する
    while (!sound_queue.empty()) {
        if (!sound_queue.front()->isDone()) {
            break;
        }
        sound_queue.pop();
    }

    modulate_amplitude(wf->wBitsPerSample, buf, volume, SOUND_VOLUME_MAX);

    auto res = std::make_unique<sound_res>(std::move(buf));

    if (auto mr = ::waveOutOpen(&res->hwo, WAVE_MAPPER, wf, NULL, NULL, CALLBACK_NULL); mr != MMSYSERR_NOERROR) {
        return false;
    }
    if (auto mr = ::waveOutPrepareHeader(res->hwo, &res->wh, sizeof(WAVEHDR)); mr != MMSYSERR_NOERROR) {
        return false;
    }
    if (auto mr = ::waveOutWrite(res->hwo, &res->wh, sizeof(WAVEHDR)); mr != MMSYSERR_NOERROR) {
        return false;
    }

    sound_queue.push(std::move(res));
    while (sound_queue.size() >= 16) {
        sound_queue.pop();
    }

    return true;
}

/*!
 * 指定ファイルを再生する
 *
 * @param path ファイルパス
 * @retval true 正常に処理された
 * @retval false 処理エラー
 */
static bool play_sound_impl(const std::filesystem::path &path, int volume)
{
    wav_reader reader;
    if (!reader.open(path)) {
        return false;
    }

    return add_sound_queue(reader.get_waveformat(), reader.retrieve_data(), volume);
}

/*!
 * @brief action-valに対応する[Sound]セクションのキー名を取得する
 * @param index "term_xtra()"の第2引数action-valに対応する値
 * @param buf 使用しない
 * @return 対応するキー名を返す
 */
static tl::optional<std::string> sound_key_at(int index)
{
    const auto sk = i2enum<SoundKind>(index);
    if (sk >= SoundKind::MAX) {
        return tl::nullopt;
    }

    return sound_names.at(sk);
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
        sound_queue.pop();
    }
}

/*!
 * @brief 指定の効果音を鳴らす。
 * @param val see SoundKind
 * @retval 0 正常終了
 * @retval 1 設定なし
 * @retval -1 PlaySoundの戻り値が正常終了以外
 */
int play_sound(int val, int volume)
{
    if (!sound_cfg_data) {
        return 1;
    }

    auto filename = sound_cfg_data->get_rand(TERM_XTRA_SOUND, val);
    if (!filename) {
        return 1;
    }

    auto path = path_build(ANGBAND_DIR_XTRA_SOUND, *filename);
    if (play_sound_impl(path, volume)) {
        return 0;
    }

    return -1;
}
