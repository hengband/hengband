/*!
 * @brief メインウィンドウ左にあるモンスターのヘルスバーを表示させる対象のモンスターインデックスを管理するクラス
 * @author Hourier
 * @date 2024/06/08
 */

#include "tracking/health-bar-tracker.h"
#include "system/deferred-update-requests.h"
#include "system/monster-entity.h"

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
    DeferredUpdateRequests::get_instance().set_flag(MainWindowRedrawingFlag::HEALTH);
}

void HealthBarTracker::set_flag_if_tracking(short m_idx) const
{
    if (this->is_tracking(m_idx)) {
        DeferredUpdateRequests::get_instance().set_flag(MainWindowRedrawingFlag::HEALTH);
    }
}

/*!
 * @brief モンスターのヘルスバーを表示していれば描き直しを要求する
 * @param m_idx モンスターのフロア内の番号
 * @param monster モンスター
 * @details 追跡しているモンスターのヘルスバーと、乗っているモンスターのヘルスバーが対象
 */
void HealthBarTracker::set_flags_if_shown(short m_idx, const MonsterEntity &monster) const
{
    this->set_flag_if_tracking(m_idx);
    if (monster.is_riding()) {
        DeferredUpdateRequests::get_instance().set_flag(MainWindowRedrawingFlag::UHEALTH);
    }
}
