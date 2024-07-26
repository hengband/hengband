#pragma once
/*!
 * @file wav-reader.h
 * @brief Windows版固有実装(WAVファイル読込)ヘッダ
 */

#include <windows.h>

#include <filesystem>
#include <memory>
#include <mmsystem.h>

/*!
 * WAVファイルの読み込み
 */
class wav_reader {
public:
    wav_reader()
        : hmmio(NULL)
    {
    }
    wav_reader(const wav_reader &) = delete;
    wav_reader &operator=(const wav_reader &) = delete;

    ~wav_reader()
    {
        close();
    }

    /*!
     * WAVファイルを開く
     *
     * @param path ファイルパス
     * @retval true 正常に処理された
     * @retval false 処理エラー
     */
    bool open(const std::filesystem::path &path);

    /*!
     * PCMデータ取得
     *
     * @return PCMデータ
     */
    std::vector<uint8_t> retrieve_data();

    const WAVEFORMATEX *get_waveformat() const
    {
        return &waveformatex;
    }
    void close();

protected:
    HMMIO hmmio;
    MMCKINFO riff_chunk{};
    MMCKINFO fmt_chunk{};
    WAVEFORMATEX waveformatex{};
    MMCKINFO data_chunk{};
    std::vector<uint8_t> buffer;
};
