/*!
 * @file wav-reader.cpp
 * @brief Windows版固有実装(WAVファイル読込)
 */

#include "main-win/wav-reader.h"
#include "main-win/main-win-utils.h"

bool wav_reader::open(char *filename)
{
    close();

    hmmio = ::mmioOpenW(to_wchar(filename).wc_str(), NULL, MMIO_READ);
    if (hmmio == NULL)
        return false;

    MMRESULT mmresult;
    LONG read_size;
    LONG readed_size;

    riff_chunk.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    mmresult = ::mmioDescend(hmmio, &riff_chunk, NULL, MMIO_FINDRIFF);
    if (mmresult != MMSYSERR_NOERROR)
        return false;

    fmt_chunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
    mmresult = ::mmioDescend(hmmio, &fmt_chunk, &riff_chunk, MMIO_FINDCHUNK);
    if (mmresult != MMSYSERR_NOERROR)
        return false;

    if (fmt_chunk.cksize > sizeof(waveformatex))
        return false;
    read_size = fmt_chunk.cksize;
    readed_size = ::mmioRead(hmmio, (HPSTR)&waveformatex, read_size);
    if (readed_size != read_size)
        return false;
    mmresult = ::mmioAscend(hmmio, &fmt_chunk, 0);
    if (mmresult != MMSYSERR_NOERROR)
        return false;

    data_chunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
    mmresult = ::mmioDescend(hmmio, &data_chunk, &riff_chunk, MMIO_FINDCHUNK);
    if (mmresult != MMSYSERR_NOERROR)
        return false;

    buffer = new BYTE[data_chunk.cksize];
    read_size = data_chunk.cksize;
    readed_size = ::mmioRead(hmmio, (HPSTR)buffer, data_chunk.cksize);
    if (readed_size != read_size) {
        delete buffer;
        buffer = NULL;
        return false;
    }

    return true;
}

const BYTE *wav_reader::read_data()
{
    auto result = buffer;
    buffer = nullptr;
    return result;
}

void wav_reader::close()
{
    if (buffer != nullptr) {
        delete buffer;
        buffer = NULL;
    }

    if (hmmio != NULL) {
        ::mmioClose(hmmio, MMIO_FHOPEN);
        hmmio = NULL;
    }
}
