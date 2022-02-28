#pragma once
/*!
 * @file wav-reader.h
 * @brief Windows版固有実装(WAVファイル読込)ヘッダ
 */

#include <windows.h>

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
     * @param filename
     * @retval true 正常に処理された
     * @retval false 処理エラー
     */
    bool open(char *filename);
    /*!
     * PCMデータ取得
     * @details 呼び出し元でdelete[]すること
     * @return PCMデータ
     */
    BYTE *read_data();
    const WAVEFORMATEX *get_waveformat()
    {
        return &waveformatex;
    }
    const MMCKINFO *get_data_chunk()
    {
        return &data_chunk;
    }
    void close();

protected:
    HMMIO hmmio;
    MMCKINFO riff_chunk{};
    MMCKINFO fmt_chunk{};
    WAVEFORMATEX waveformatex{};
    MMCKINFO data_chunk{};
    std::unique_ptr<BYTE[]> buffer;
};
