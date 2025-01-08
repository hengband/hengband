/*!
 * @brief OSやハードウェアレベルのサポート状況チェック実装
 * @author Hourier
 * @date 2025/01/08
 */

#include "main-win/os-support-checker.h"
#include "locale/language-switcher.h"
#include <intrin.h>
#include <sstream>

/*!
 * @brief CPUの機能チェック (AVX)
 * @return CPUとOSがAVXをサポートしており、BIOSレベルで有効化されていればnullopt。そうでないならエラーメッセージ
 * @details 本ファイル作成現在、Windows 10 22H2、Windows 11 23H2、Windows 11 24H2 の3バージョンがサポート中である。
 * これら全てが、AVX2対応CPUのみをサポートしている。
 * Intel w/Win10：Haswell Refresh / Broadwell (Core i 5000番台)以降
 * Intel w/Win11：Kaby Lake Refresh / Coffee Lake (Core i 8000番台)以降
 * AMD w/Win10：Bulldozer (FX4000番台)以降
 * AMD w/Win11：Raven Ridge / Pinnacle Ridge (Ryzen 2000番台)以降
 *
 * @todo AVX2は64ビットバイナリが推奨である。
 * 32ビットバイナリは将来に亘ってAVXまでの有効化に留める予定。開発チームとして32ビットバイナリのサポート終了時期は未定。
 * 64ビットバイナリを安定的に供給できる見込みが立ったら、こちらはAVX2まで有効化する。
 */
std::optional<std::string> OsSupportChecker::check_avx_enabled()
{
    int cpuInfo[4]{};
    __cpuid(cpuInfo, 1);
    const auto is_cpu_supporting_avx = (cpuInfo[2] & (1 << 28)) != 0;
    const auto is_os_supporting_avx = (cpuInfo[2] & (1 << 27)) != 0;
    if (is_cpu_supporting_avx && is_os_supporting_avx) {
        const auto feature_mask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
        if ((feature_mask & 0x06) == 0x06) {
            return std::nullopt;
        }
    }

#ifdef WIN_DEBUG
    return _("AVX非対応CPUはデバッグモードでのみ起動可能です。", "CPUs which don't support AVX can only be started in Debug mode.");
#else
    std::stringstream ss;
#ifdef JP
    ss << "お使いのコンピュータにはAVX非対応CPUが搭載されています。" << '\n'
       << "マイクロソフト社は既に当該CPU群のサポートを全て打ち切っています。" << '\n'
       << "本ソフトウェアはAVXを必要としており、起動できません。";
#else
    ss << "This CPU doesn't support AVX." << '\n'
       << "Microsoft has dropped support for such CPUs." << '\n'
       << "This software requires AVX and cannot be started.";
#endif
    return ss.str();
#endif
}
