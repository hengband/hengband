#pragma once

struct dt_type {
    int dun_tun_rnd; /*!< ダンジョンの通路方向を掻き回す頻度(一回の試行ごとに%で判定している) */
    int dun_tun_chg; /*!< ダンジョンの通路をクランクさせる頻度(一回の試行ごとに%で判定している) */
    int dun_tun_con; /*!< ダンジョンの通路を継続して引き延ばす頻度(一回の試行ごとに%で判定している) */
    int dun_tun_pen; /*!< ダンジョンの部屋入口にドアを設置する頻度(一回の試行ごとに%で判定している) */
    int dun_tun_jct; /*!< ダンジョンの通路交差地点付近にドアを設置する頻度(一回の試行ごとに%で判定している) */
};

dt_type *initialize_dt_type(dt_type *dt_ptr);
