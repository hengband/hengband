#pragma once

#include <string_view>
#include <tl/optional.hpp>

enum class AttributeType;
enum class MonsterAbilityType;
class Direction;
class PlayerType;

/*!
 * @brief プレイヤーが使うモンスター魔法の放ち方
 */
enum class MspellProjectionType {
    BREATH, //!< ブレス
    BALL, //!< ボール
    BOLT, //!< ボルト
};

/*!
 * @brief プレイヤーが使うモンスター魔法 (青魔法・ものまね) のうち、ブレス・ボール・ボルトの定義
 * @details ブレスの半径と威力は、青魔法とものまねで求め方が違うので、それぞれの側で決める。
 */
struct MspellProjection {
    MspellProjectionType type; //!< 放ち方
    AttributeType attribute; //!< 属性
    int radius; //!< ボールの半径 (ブレスとボルトでは使わない)
    std::string_view message; //!< 使ったときのメッセージ
};

tl::optional<const MspellProjection &> find_mspell_projection(MonsterAbilityType ability);
void fire_mspell_projection(PlayerType *player_ptr, const MspellProjection &projection, const Direction &dir, int damage, int breath_radius);
