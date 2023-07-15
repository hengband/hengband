#pragma once

// MSVCの警告レベルを最大に設定してあるためjson.hppが大量の警告を
// 出力してしまうのでヘッダのインクルード時のみ警告を抑制しておく
#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#include "external-lib/json.hpp"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
