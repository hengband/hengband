#pragma once
/*!
 * @file audio-win.h
 * @brief Windows版固有実装(オーディオ再生)ヘッダ
 */

#include <windows.h>
#include <mmsystem.h>

#define WM_REPEAT_MUSIC (WM_APP + 1)

bool can_audio();
void setup_audio(HWND);
void init_audio();
void finalize_audio();
bool add_sound_queue(const WAVEFORMATEX *wf, const BYTE*buf, DWORD bufsize);
bool add_music_queue(const char *filename);
void stop_music_queue();
void pause_music_queue();
void resume_music_queue();
void repeat_music();
