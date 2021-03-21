/*!
 * 守護神(ヴァラ)に関する処理
 * マンウェ: 耐電、電オーラ
 * ウルモ: 耐酸、耐水
 * アウレ: +腕
 * オロメ: +速
 * マンドス: 耐獄
 * イルモ:
 * トゥルカス: +耐
 * ヴァルダ: 光源
 * ヤヴァンナ:
 * ニエンナ:
 * エステ: 急回復、遅消化
 * ヴァイレ:
 * ヴァーナ:
 * ネスサ:
 */

#include "player/patron-valar.h"
#include "object-enchant/tr-types.h"
#include "player/player-race.h"
#include "racial/racial-util.h"
#include "realm/realm-types.h"
#include <array>

enum valar_type : int {
	VAR_MANWE = 0,
	VAR_ULMO = 1,
	VAR_AULE = 2,
	VAR_OROME = 3,
	VAR_MANDOS = 4,
	VAR_IRMO = 5,
	VAR_TULKAS = 6,
	VAR_VARDA = 7,
	VAR_YAVANNA = 8,
	VAR_NIENNA = 9,
	VAR_ESTE = 10,
	VAR_VAIRE = 11,
	VAR_VANA = 12,
	VAR_NESSA = 13,
	MAX_VAR = 14,
};

// clang-format off
std::array<std::string_view, MAX_VAR> valar_names = {
    _("マンウェ", "Manwe"), // 風
    _("ウルモ", "Ulmo"), // 水
    _("アウレ", "Aule"), //鍛冶
    _("オロメ", "Orome"), // 狩人
    _("マンドス", "Mandos"), // 運命
    _("イルモ", "Irmo"), // 夢
    _("トゥルカス", "Tulkas"), // 力
    _("ヴァルダ", "Varda"), // 星(エルベレス)
    _("ヤヴァンナ", "Yavanna"), //大地
    _("ニエンナ", "Nienna"), //嘆き
    _("エステ", "Este"), // 癒し
    _("ヴァイレ", "Vaire"), // 織姫
    _("ヴァーナ", "Vana"), // 常若
    _("ネスサ", "Nessa"), // 踊り
};

static bool is_allowed_class(player_type* creature_ptr)
{
	return creature_ptr->pclass == CLASS_PALADIN;
}

static bool is_allowed_realm(player_type* creature_ptr)
{
	return creature_ptr->realm1 == REALM_LIFE || creature_ptr->realm1 == REALM_CRUSADE;
}

concptr get_vara_name(player_type *creature_ptr)
{
	if (!is_allowed_class(creature_ptr) || !is_allowed_realm(creature_ptr))
		return NULL;

	return valar_names.at(creature_ptr->realm1).data();
}

void switch_vara_racial(player_type* creature_ptr, rc_type* rc_ptr)
{
	if (!is_allowed_class(creature_ptr) || !is_allowed_realm(creature_ptr))
		return;

	strcpy(rc_ptr->power_desc[rc_ptr->num].racial_name, _("祈り", "Praying"));
    rc_ptr->power_desc[rc_ptr->num].min_level = 1;
    rc_ptr->power_desc[rc_ptr->num].cost = 0;
    rc_ptr->power_desc[rc_ptr->num].stat = A_WIS;
    rc_ptr->power_desc[rc_ptr->num].fail = 0;
    rc_ptr->power_desc[rc_ptr->num++].number = -10;

	switch (creature_ptr->realm1)
	{
	case VAR_MANWE:
		break;
	case VAR_ULMO:
		break;
	case VAR_AULE:
		break;
	case VAR_OROME:
		break;
	case VAR_MANDOS:
		break;
	case VAR_IRMO:
		break;
	case VAR_TULKAS:
		break;
	case VAR_VARDA:
		break;
	case VAR_YAVANNA:
		break;
	case VAR_NIENNA:
		break;
	case VAR_ESTE:
		break;
	case VAR_VAIRE:
		break;
	case VAR_VANA:
		break;
	case VAR_NESSA:
		break;
	default:
		break;
	}
}

/*!
 * @brief クラスパワーを実行
 * @param creature_ptr プレイヤー情報への参照ポインタ
 * @return 実行したらTRUE、しなかったらFALSE
 */
bool switch_element_execution(player_type* creature_ptr, int comm)
{
	if (comm == -10) {
		return TRUE;
	}
}
