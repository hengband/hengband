/*!
 * @file audio-win.cpp
 * @brief Windows版固有実装(オーディオ再生)
 */

#include "main-win/audio-win.h"
#include "main-win/main-win-utils.h"

#include <queue>
#include <xaudio2.h>
#include <mfapi.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mmdeviceapi.h>
#include <mferror.h>
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfplay.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

HWND main_window = NULL;

IXAudio2 *xaudio2 = nullptr;
IXAudio2MasteringVoice *master_voice = nullptr;
IMFAttributes *mf_attributes = nullptr;
IMFMediaType *pcm_mediatype = nullptr;

struct sound_res {
    sound_res() = default;
    ~sound_res()
    {
        dispose();
    }

    XAUDIO2_BUFFER buffer = { 0 };
    IXAudio2SourceVoice *source_voice;

    void dispose()
    {
        if (source_voice != nullptr) {
            source_voice->Stop();
            source_voice->DestroyVoice();
            source_voice = nullptr;
        }

        if (buffer.pAudioData != nullptr) {
            delete buffer.pAudioData;
            buffer.pAudioData = nullptr;
        }
    }
};
std::queue<sound_res *> sound_queue;

struct sample_res {
    sample_res() = default;
    ~sample_res()
    {
        dispose();
    }

    IMFSample *mf_sample = nullptr;
    IMFMediaBuffer *media_buffer = nullptr;
    BYTE *sample_data = nullptr;
    int sample_size = 0;

    void dispose()
    {
        if (mf_sample != nullptr) {
            mf_sample->Release();
            mf_sample = nullptr;
        }
        if (media_buffer != nullptr) {
            media_buffer->Release();
            media_buffer = nullptr;
        }
        if (sample_data != nullptr) {
            delete sample_data;
            sample_data = nullptr;
        }
    }
};

static bool get_next(IMFSourceReader *source_reader, sample_res *sample)
{
    DWORD flags = 0;
    HRESULT hresult = source_reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &sample->mf_sample);

    if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
        // end of file
        sample->sample_size = -1;
        return false;
    }

    hresult = sample->mf_sample->ConvertToContiguousBuffer(&sample->media_buffer);
    DWORD sample_size = 0;
    BYTE *buf;
    hresult = sample->media_buffer->Lock(&buf, nullptr, &sample_size);
    sample->sample_data = new byte[sample_size];
    CopyMemory(sample->sample_data, buf, sample_size);
    hresult = sample->media_buffer->Unlock();
    sample->media_buffer->Release();
    sample->media_buffer = nullptr;
    sample->sample_size = sample_size;
    return true;
}

class music_res : public IXAudio2VoiceCallback {
public:
    music_res(const char *filename)
        : music_file(filename)
    {
    }
    ~music_res()
    {
        dispose();
    }

    std::string music_file;
    sample_res sample[2];
    int current_sample = 0;
    sample_res* get_sample() {
        sample_res *result = &sample[current_sample];
        ++current_sample;
        if (current_sample >= _countof(sample))
            current_sample = 0;
        result->dispose();
        return result;
    }

    IMFSourceReader *source_reader = nullptr;
    IMFMediaType *output_mediatype = nullptr;
    WAVEFORMATEX *wave_format = nullptr;
    IXAudio2SourceVoice *source_voice = nullptr;
    void dispose()
    {
        if (source_voice != nullptr) {
            source_voice->Stop();
            source_voice->DestroyVoice();
            source_voice = nullptr;
        }

        if (wave_format != nullptr) {
            ::CoTaskMemFree(wave_format);
            wave_format = nullptr;
        }

        if (output_mediatype != nullptr) {
            output_mediatype->Release();
            output_mediatype = nullptr;
        }

        if (source_reader != nullptr) {
            source_reader->Release();
            source_reader = nullptr;
        }
    }

    void submit_buf()
    {
        sample_res *sample = get_sample();
        if (get_next(source_reader, sample)) {
            XAUDIO2_BUFFER buffer = { 0 };
            buffer.AudioBytes = sample->sample_size;
            buffer.pAudioData = sample->sample_data;
            source_voice->SubmitSourceBuffer(&buffer);
        } else {
            source_voice->Stop();
            // ループ再生がうまくできないため同じファイルを再生する
            ::PostMessageW(main_window, WM_REPEAT_MUSIC, 0, 0);
        }
    }

    void STDMETHODCALLTYPE OnVoiceProcessingPassStart([[maybe_unused]] UINT32 BytesRequired) {}
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd(void) {}
    void STDMETHODCALLTYPE OnStreamEnd(void) {}
    void STDMETHODCALLTYPE OnBufferStart([[maybe_unused]] void *pBufferContext)
    {
        submit_buf();
    }
    void STDMETHODCALLTYPE OnBufferEnd([[maybe_unused]] void *pBufferContext) {}
    void STDMETHODCALLTYPE OnLoopEnd([[maybe_unused]] void *pBufferContext) {}
    void STDMETHODCALLTYPE OnVoiceError([[maybe_unused]] void *pBufferContext, [[maybe_unused]] HRESULT Error) {}
};
std::queue<music_res *> music_queue;

bool can_audio()
{
    return (xaudio2 != nullptr);
}

static void release_xaudio2()
{
    if (master_voice != nullptr) {
        master_voice->DestroyVoice();
        master_voice = nullptr;
    }

    if (xaudio2 != nullptr) {
        xaudio2->Release();
        xaudio2 = nullptr;
    }
}

void finalize_audio()
{
    stop_music_queue();

    while (!sound_queue.empty()) {
        auto res = sound_queue.front();
        delete res;
        sound_queue.pop();
    }

    if (mf_attributes != nullptr) {
        mf_attributes->Release();
        mf_attributes = nullptr;
    }

    if (pcm_mediatype != nullptr) {
        pcm_mediatype->Release();
        pcm_mediatype = nullptr;
    }

    MFShutdown();
    release_xaudio2();
}

void setup_audio(HWND hwnd)
{
    main_window = hwnd;
}

void init_audio()
{
    static bool done_init = false;
    if (done_init)
        return;
    done_init = true;

    HRESULT hr;

    hr = ::XAudio2Create(&xaudio2);
    if (FAILED(hr)) {
        release_xaudio2();
        return;
    }

    hr = xaudio2->CreateMasteringVoice(&master_voice);
    if (FAILED(hr)) {
        release_xaudio2();
        return;
    }

    hr = ::MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        release_xaudio2();
        return;
    }

    hr = ::MFCreateAttributes(&mf_attributes, 1);
    if (FAILED(hr)) {
        release_xaudio2();
        return;
    }

    hr = mf_attributes->SetUINT32(MF_LOW_LATENCY, TRUE);
    if (FAILED(hr)) {
        release_xaudio2();
        return;
    }

    hr = ::MFCreateMediaType(&pcm_mediatype);
    if (FAILED(hr)) {
        release_xaudio2();
        return;
    }
    hr = pcm_mediatype->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    if (FAILED(hr)) {
        release_xaudio2();
        return;
    }
    hr = pcm_mediatype->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    if (FAILED(hr)) {
        release_xaudio2();
        return;
    }
}

bool add_sound_queue(const WAVEFORMATEX *wf, const BYTE *buf, DWORD bufsize)
{
    XAUDIO2_VOICE_STATE state;
    while (!sound_queue.empty()) {
        auto res = sound_queue.front();
        if (res->source_voice == nullptr) {
            delete res;
            sound_queue.pop();
            continue;
        }
        res->source_voice->GetState(&state);
        if (state.pCurrentBufferContext == nullptr || state.SamplesPlayed > 0) {
            delete res;
            sound_queue.pop();
            continue;
        }
        break;
    }

    HRESULT hr;
    IXAudio2SourceVoice *source_voice;
    hr = xaudio2->CreateSourceVoice(&source_voice, wf);
    if (FAILED(hr))
        return false;

    auto res = new sound_res();
    res->source_voice = source_voice;
    auto xbuf = &res->buffer;
    xbuf->Flags = XAUDIO2_END_OF_STREAM;
    xbuf->AudioBytes = bufsize;
    xbuf->pAudioData = buf;
    xbuf->PlayBegin = 0;
    xbuf->PlayLength = xbuf->AudioBytes / wf->nBlockAlign;
    sound_queue.push(res);

    hr = source_voice->SubmitSourceBuffer(xbuf);
    if (FAILED(hr))
        return false;

    hr = res->source_voice->Start();
    if (FAILED(hr))
        return false;

    return true;
}

bool add_music_queue(const char *filename)
{
    stop_music_queue();

    auto res = new music_res(filename);
    music_queue.push(res);

    HRESULT hr;
    hr = ::MFCreateSourceReaderFromURL(to_wchar(filename).wc_str(), mf_attributes, &res->source_reader);
    if (FAILED(hr))
        return false;

    hr = res->source_reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pcm_mediatype);
    if (FAILED(hr))
        return false;

    hr = res->source_reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &res->output_mediatype);
    if (FAILED(hr))
        return false;

    UINT32 wf_size;
    hr = ::MFCreateWaveFormatExFromMFMediaType(res->output_mediatype, &res->wave_format, &wf_size);
    if (FAILED(hr))
        return false;

    hr = xaudio2->CreateSourceVoice(&res->source_voice, res->wave_format, XAUDIO2_VOICE_NOPITCH, XAUDIO2_DEFAULT_FREQ_RATIO, res);
    if (FAILED(hr))
        return false;

    res->submit_buf();
    res->source_voice->Start();

    return true;
}

void stop_music_queue()
{
    while (!music_queue.empty()) {
        auto res = music_queue.front();
        delete res;
        music_queue.pop();
    }
}

void pause_music_queue()
{
    if (xaudio2)
        xaudio2->StopEngine();
}

void resume_music_queue()
{
    if (xaudio2)
        xaudio2->StartEngine();
}

void repeat_music()
{
    if (music_queue.empty())
        return;

    auto res = music_queue.front();
    std::string filename = res->music_file;
    add_music_queue(filename.c_str());
}
