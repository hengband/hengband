#include "status/temporary-resistance.h"
#include "core/disturbance.h"
#include "core/stuff-handler.h"
#include "game-option/disturbance-options.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"

/*!
 * @brief 一時的浮遊の継続時間をセットする / Set "tim_levitation", notice observable changes
 * @param v 継続時間
 * @param do_dec 現在の継続時間より長い値のみ上書きする
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す。
 */
bool set_tim_levitation(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_levitation && !do_dec) {
            if (player_ptr->tim_levitation > v) {
                return false;
            }
        } else if (!player_ptr->tim_levitation) {
            msg_print(_("体が宙に浮き始めた。", "You begin to fly!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_levitation) {
            msg_print(_("もう宙に浮かべなくなった。", "You stop flying."));
            notice = true;
        }
    }

    player_ptr->tim_levitation = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_ultimate_res(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->ult_res && !do_dec) {
            if (player_ptr->ult_res > v) {
                return false;
            }
        } else if (!player_ptr->ult_res) {
            msg_print(_("あらゆることに対して耐性がついた気がする！", "You feel resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->ult_res) {
            msg_print(_("あらゆることに対する耐性が薄れた気がする。", "You feel less resistant"));
            notice = true;
        }
    }

    player_ptr->ult_res = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_nether(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;

    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_res_nether && !do_dec) {
            if (player_ptr->tim_res_nether > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_nether) {
            msg_print(_("地獄の力に対して耐性がついた気がする！", "You feel nether-resistant!"));
            notice = true;
        }
    }

    else {
        if (player_ptr->tim_res_nether) {
            msg_print(_("地獄の力に対する耐性が薄れた気がする。", "You feel less nether-resistant"));
            notice = true;
        }
    }

    player_ptr->tim_res_nether = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_time(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_res_time && !do_dec) {
            if (player_ptr->tim_res_time > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_time) {
            msg_print(_("時間逆転の力に対して耐性がついた気がする！", "You feel time-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_time) {
            msg_print(_("時間逆転の力に対する耐性が薄れた気がする。", "You feel less time-resistant"));
            notice = true;
        }
    }

    player_ptr->tim_res_time = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_lite(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_res_lite && !do_dec) {
            if (player_ptr->tim_res_lite > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_lite) {
            msg_print(_("閃光の力に対して耐性がついた気がする！", "You feel lite-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_lite) {
            msg_print(_("閃光の力に対する耐性が薄れた気がする。", "You feel less lite-resistant"));
            notice = true;
        }
    }

    player_ptr->tim_res_lite = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_dark(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }

    if (v) {
        if (player_ptr->tim_res_dark && !do_dec) {
            if (player_ptr->tim_res_dark > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_dark) {
            msg_print(_("暗黒の力に対して耐性がついた気がする！", "You feel dark-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_dark) {
            msg_print(_("暗黒の力に対する耐性が薄れた気がする。", "You feel less dark-resistant"));
            notice = true;
        }
    }

    player_ptr->tim_res_dark = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }

    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_shard(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }
    if (v) {
        if (player_ptr->tim_res_shard && !do_dec) {
            if (player_ptr->tim_res_shard > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_shard) {
            msg_print(_("破片の力に対して耐性がついた気がする！", "You feel shard-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_shard) {
            msg_print(_("破片の力に対する耐性が薄れた気がする。", "You feel less shard-resistant"));
            notice = true;
        }
    }
    player_ptr->tim_res_shard = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }
    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_blind(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }
    if (v) {
        if (player_ptr->tim_res_blind && !do_dec) {
            if (player_ptr->tim_res_blind > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_blind) {
            msg_print(_("盲目の力に対して耐性がついた気がする！", "You feel blind-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_blind) {
            msg_print(_("盲目の力に対する耐性が薄れた気がする。", "You feel less blind-resistant"));
            notice = true;
        }
    }
    player_ptr->tim_res_blind = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }
    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_conf(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }
    if (v) {
        if (player_ptr->tim_res_conf && !do_dec) {
            if (player_ptr->tim_res_conf > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_conf) {
            msg_print(_("混乱の力に対して耐性がついた気がする！", "You feel confusion-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_conf) {
            msg_print(_("混乱の力に対する耐性が薄れた気がする。", "You feel less confusion-resistant"));
            notice = true;
        }
    }
    player_ptr->tim_res_conf = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }
    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_sound(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }
    if (v) {
        if (player_ptr->tim_res_sound && !do_dec) {
            if (player_ptr->tim_res_sound > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_sound) {
            msg_print(_("轟音の力に対して耐性がついた気がする！", "You feel sound-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_sound) {
            msg_print(_("轟音の力に対する耐性が薄れた気がする。", "You feel less sound-resistant"));
            notice = true;
        }
    }
    player_ptr->tim_res_sound = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }
    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_nexus(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }
    if (v) {
        if (player_ptr->tim_res_nexus && !do_dec) {
            if (player_ptr->tim_res_nexus > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_nexus) {
            msg_print(_("次元の力に対して耐性がついた気がする！", "You feel nexus-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_nexus) {
            msg_print(_("次元の力に対する耐性が薄れた気がする。", "You feel less nexus-resistant"));
            notice = true;
        }
    }
    player_ptr->tim_res_nexus = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }
    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_chaos(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }
    if (v) {
        if (player_ptr->tim_res_chaos && !do_dec) {
            if (player_ptr->tim_res_chaos > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_chaos) {
            msg_print(_("混沌の力に対して耐性がついた気がする！", "You feel chaos-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_chaos) {
            msg_print(_("混沌の力に対する耐性が薄れた気がする。", "You feel less chaos-resistant"));
            notice = true;
        }
    }
    player_ptr->tim_res_chaos = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }
    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_disenchant(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }
    if (v) {
        if (player_ptr->tim_res_disenchant && !do_dec) {
            if (player_ptr->tim_res_disenchant > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_disenchant) {
            msg_print(_("劣化の力に対して耐性がついた気がする！", "You feel disenchantment-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_disenchant) {
            msg_print(_("劣化の力に対する耐性が薄れた気がする。", "You feel less disenchantment-resistant"));
            notice = true;
        }
    }
    player_ptr->tim_res_disenchant = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }
    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_water(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }
    if (v) {
        if (player_ptr->tim_res_water && !do_dec) {
            if (player_ptr->tim_res_water > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_water) {
            msg_print(_("水の力に対して耐性がついた気がする！", "You feel water-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_water) {
            msg_print(_("水の力に対する耐性が薄れた気がする。", "You feel less water-resistant"));
            notice = true;
        }
    }
    player_ptr->tim_res_water = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }
    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_fear(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }
    if (v) {
        if (player_ptr->tim_res_fear && !do_dec) {
            if (player_ptr->tim_res_fear > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_fear) {
            msg_print(_("恐怖の力に対して耐性がついた気がする！", "You feel fear-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_fear) {
            msg_print(_("恐怖の力に対する耐性が薄れた気がする。", "You feel less fear-resistant"));
            notice = true;
        }
    }
    player_ptr->tim_res_fear = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }
    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}

bool set_tim_res_curse(PlayerType *player_ptr, TIME_EFFECT v, bool do_dec)
{
    bool notice = false;
    v = (v > 10000) ? 10000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->is_dead) {
        return false;
    }
    if (v) {
        if (player_ptr->tim_res_curse && !do_dec) {
            if (player_ptr->tim_res_curse > v) {
                return false;
            }
        } else if (!player_ptr->tim_res_curse) {
            msg_print(_("呪いの力に対して耐性がついた気がする！", "You feel curse-resistant!"));
            notice = true;
        }
    } else {
        if (player_ptr->tim_res_curse) {
            msg_print(_("呪いの力に対する耐性が薄れた気がする。", "You feel less curse-resistant"));
            notice = true;
        }
    }
    player_ptr->tim_res_curse = v;
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (!notice) {
        return false;
    }
    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    rfu.set_flag(StatusRecalculatingFlag::BONUS);
    handle_stuff(player_ptr);
    return true;
}
