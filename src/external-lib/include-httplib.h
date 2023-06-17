#pragma once

#include "system/angband.h"

#ifdef WORLD_SCORE

#define CPPHTTPLIB_OPENSSL_SUPPORT

// MSVCの警告レベルを最大に設定してあるためcpp-httplibとOpenSSLのヘッダが
// 大量の警告を出力してしまうのでヘッダのインクルード時のみ警告を抑制しておく
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include "external-lib/httplib.h"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#ifdef WINDOWS
#include <Windows.h>
#endif

#endif // WORLD_SCORE
