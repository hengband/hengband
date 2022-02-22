#pragma once

/*
 * project()関数に用いられる、遠隔攻撃特性ビットフラグ / Bit flags for the "project()" function
 */
enum effect_characteristics {
    PROJECT_NONE = 0x0000,
    PROJECT_JUMP = 0x0001, /*!< 発動者からの軌跡を持たず、指定地点に直接発生する(予め置いたトラップ、上空からの発生などのイメージ) / Jump directly to the target location (this is a hack) */
    PROJECT_BEAM = 0x0002, /*!< ビーム範囲を持つ。 / Work as a beam weapon (affect every grid passed through) */
    PROJECT_THRU = 0x0004, /*!< 目標地点に到達しても射程と遮蔽の限り引き延ばす。 / Continue "through" the target (used for "bolts"/"beams") */
    PROJECT_STOP = 0x0008, /*!< 道中にプレイヤーかモンスターがいた時点で到達地点を更新して停止する(壁や森はPROJECT_DISIがない限り最初から貫通しない) */
    PROJECT_GRID = 0x0010, /*!< 射程内の地形に影響を及ぼす / Affect each grid in the "blast area" in some way */
    PROJECT_ITEM = 0x0020, /*!< 射程内のアイテムに影響を及ぼす / Affect each object in the "blast area" in some way */
    PROJECT_KILL = 0x0040, /*!< 射程内のモンスターに影響を及ぼす / Affect each monster in the "blast area" in some way */
    PROJECT_HIDE = 0x0080, /*!< 画面上にフィードバック表示させない / Disable "visual" feedback from projection */
    PROJECT_DISI = 0x0100, /*!< 永久壁でない壁を破壊する / Disintegrate non-permanent features */
    PROJECT_PLAYER = 0x0200, /*!< プレイヤー自身をターゲットにする (騎乗中) / Main target is player (used for riding player) */
    PROJECT_AIMED = 0x0400, /*!<  / Target is only player or monster, so don't affect another. Depend on PROJECT_PLAYER. (used for minimum (rad == 0) balls on riding player) */
    PROJECT_REFLECTABLE = 0x0800, /*!< 反射可能(ボルト系魔法に利用) / Refrectable spell attacks (used for "bolts") */
    PROJECT_NO_HANGEKI = 0x1000, /*!< 反撃させない / Avoid counter attacks of monsters */
    PROJECT_PATH = 0x2000, /*!< 軌跡の表示 / Only used for printing project path */
    PROJECT_FAST = 0x4000, /*!< 炸裂するまで画面上に表示しない / Hide "visual" of flying bolts until blast */
    PROJECT_LOS = 0x8000, /*!< 視線が通っているか(？) / Line of sight */
    PROJECT_BREATH = 0x10000 /*!< ブレスである / Breath */
};
