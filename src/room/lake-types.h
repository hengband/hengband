#pragma once

/* 池型地形の生成ID / Room types for generate_lake() */
enum lake_type {
    LAKE_T_LAVA = 1, /*!< 池型地形ID: 溶岩 */
    LAKE_T_WATER = 2, /*!< 池型地形ID: 池 */
    LAKE_T_CAVE = 3, /*!< 池型地形ID: 空洞 */
    LAKE_T_EARTH_VAULT = 4, /*!< 池型地形ID: 地属性VAULT */
    LAKE_T_AIR_VAULT = 5, /*!< 池型地形ID: 風属性VAULT */
    LAKE_T_WATER_VAULT = 6, /*!< 池型地形ID: 水属性VAULT */
    LAKE_T_FIRE_VAULT = 7, /*!< 池型地形ID: 火属性VAULT */
};
