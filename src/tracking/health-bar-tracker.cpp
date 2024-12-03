/*!
 * @brief メインウィンドウ左にあるモンスターのヘルスバーを表示させる対象のモンスターインデックスを管理するクラス
 * @author Hourier
 * @date 2024/06/08
 */

#include "tracking/health-bar-tracker.h"
#include "system/redrawing-flags-updater.h"

HealthBarTracker HealthBarTracker::instance{};

HealthBarTracker &HealthBarTracker::get_instance()
{
    return instance;
}

bool HealthBarTracker::is_tracking() const
{
    return this->tracking_m_idx > 0;
}

bool HealthBarTracker::is_tracking(short m_idx) const
{
    return this->tracking_m_idx == m_idx;
}

short HealthBarTracker::get_trackee() const
{
    return this->tracking_m_idx;
}

void HealthBarTracker::set_trackee(short m_idx)
{
    this->tracking_m_idx = m_idx;
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::HEALTH);
}

void HealthBarTracker::set_flag_if_tracking(short m_idx) const
{
    if (this->is_tracking(m_idx)) {
        RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::HEALTH);
    }
}
