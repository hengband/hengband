#pragma once
/*!
 * @file wav-reader.h
 * @brief Windows版固有実装(WAVファイル読込)ヘッダ
 */

#include <windows.h>
#include <mmsystem.h>

class wav_reader {
public:
    wav_reader()
        : hmmio(NULL)
        , buffer(nullptr)
    {
    }
    ~wav_reader()
    {
        close();
    }

    bool open(char *filename);
    /*!
     * PCMデータ取得
     * @details 呼び出し元でdeleteすること
     * @return PCMデータ
     */
    const BYTE* read_data();
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
    BYTE *buffer;
};
