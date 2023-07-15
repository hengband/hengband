/*!
 * @file wav-reader.cpp
 * @brief Windows版固有実装(WAVファイル読込)
 */

#include "main-win/wav-reader.h"
#include "main-win/main-win-utils.h"

bool wav_reader::open(char *filename)
{
    close();

    this->hmmio = ::mmioOpenW(to_wchar(filename).wc_str(), NULL, MMIO_READ);
    if (this->hmmio == NULL) {
        return false;
    }

    MMRESULT mmresult;
    LONG read_size;
    LONG readed_size;

    this->riff_chunk.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    mmresult = ::mmioDescend(this->hmmio, &this->riff_chunk, NULL, MMIO_FINDRIFF);
    if (mmresult != MMSYSERR_NOERROR) {
        return false;
    }

    this->fmt_chunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
    mmresult = ::mmioDescend(this->hmmio, &this->fmt_chunk, &this->riff_chunk, MMIO_FINDCHUNK);
    if (mmresult != MMSYSERR_NOERROR) {
        return false;
    }

    if (this->fmt_chunk.cksize > sizeof(this->waveformatex)) {
        return false;
    }
    read_size = this->fmt_chunk.cksize;
    readed_size = ::mmioRead(this->hmmio, (HPSTR) & this->waveformatex, read_size);
    if (readed_size != read_size) {
        return false;
    }
    if (this->waveformatex.wFormatTag != WAVE_FORMAT_PCM) {
        return false;
    }
    mmresult = ::mmioAscend(this->hmmio, &this->fmt_chunk, 0);
    if (mmresult != MMSYSERR_NOERROR) {
        return false;
    }

    this->data_chunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
    mmresult = ::mmioDescend(this->hmmio, &this->data_chunk, &riff_chunk, MMIO_FINDCHUNK);
    if (mmresult != MMSYSERR_NOERROR) {
        return false;
    }

    this->buffer.reset(new BYTE[data_chunk.cksize]);
    read_size = this->data_chunk.cksize;
    readed_size = ::mmioRead(this->hmmio, (HPSTR)this->buffer.get(), read_size);
    if (readed_size != read_size) {
        return false;
    }

    return true;
}

BYTE *wav_reader::read_data()
{
    return this->buffer.release();
}

void wav_reader::close()
{
    if (this->hmmio != NULL) {
        ::mmioClose(this->hmmio, 0);
        this->hmmio = NULL;
    }
}
