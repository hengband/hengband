#include "room/pit-nest-kinds-table.h"
#include "monster-race/monster-race-hook.h"

/*!nest情報テーブル*/
vault_aux_type nest_types[MAX_PIT_NEST_KINDS] =
{
	{ _("クローン", "clone"),      vault_aux_clone,    vault_prep_clone,   5, 3 },
	{ _("ゼリー", "jelly"),        vault_aux_jelly,    NULL,               5, 6 },
	{ _("シンボル(善)", "symbol good"), vault_aux_symbol_g, vault_prep_symbol, 25, 2 },
	{ _("シンボル(悪)", "symbol evil"), vault_aux_symbol_e, vault_prep_symbol, 25, 2 },
	{ _("ミミック", "mimic"),      vault_aux_mimic,    NULL,              30, 4 },
	{ _("狂気", "lovecraftian"),   vault_aux_cthulhu,  NULL,              70, 2 },
	{ _("犬小屋", "kennel"),       vault_aux_kennel,   NULL,              45, 4 },
	{ _("動物園", "animal"),       vault_aux_animal,   NULL,              35, 5 },
	{ _("教会", "chapel"),         vault_aux_chapel_g, NULL,              75, 4 },
	{ _("アンデッド", "undead"),   vault_aux_undead,   NULL,              75, 5 },
	{ NULL,           NULL,               NULL,               0, 0 },
};

/*!pit情報テーブル*/
vault_aux_type pit_types[MAX_PIT_NEST_KINDS] =
{
	{ _("オーク", "orc"),            vault_aux_orc,      NULL,               5, 6 },
	{ _("トロル", "troll"),          vault_aux_troll,    NULL,              20, 6 },
	{ _("巨人", "giant"),    vault_aux_giant,    NULL,              50, 6 },
	{ _("狂気", "lovecraftian"),     vault_aux_cthulhu,  NULL,              80, 2 },
	{ _("シンボル(善)", "symbol good"), vault_aux_symbol_g, vault_prep_symbol, 70, 1 },
	{ _("シンボル(悪)", "symbol evil"), vault_aux_symbol_e, vault_prep_symbol, 70, 1 },
	{ _("教会", "chapel"),           vault_aux_chapel_g, NULL,              65, 2 },
	{ _("ドラゴン", "dragon"),       vault_aux_dragon,   vault_prep_dragon, 70, 6 },
	{ _("デーモン", "demon"),        vault_aux_demon,    NULL,              80, 6 },
	{ _("ダークエルフ", "dark elf"), vault_aux_dark_elf, NULL,              45, 4 },
	{ NULL,           NULL,               NULL,               0, 0 },
};

const int placing[MAX_MONSTER_PLACE][3] = {
	{ -2, -9, 0 },{ -2, -8, 0 },{ -3, -7, 0 },{ -3, -6, 0 },
	{ +2, -9, 0 },{ +2, -8, 0 },{ +3, -7, 0 },{ +3, -6, 0 },
	{ -2, +9, 0 },{ -2, +8, 0 },{ -3, +7, 0 },{ -3, +6, 0 },
	{ +2, +9, 0 },{ +2, +8, 0 },{ +3, +7, 0 },{ +3, +6, 0 },
	{ -2, -7, 1 },{ -3, -5, 1 },{ -3, -4, 1 },
	{ +2, -7, 1 },{ +3, -5, 1 },{ +3, -4, 1 },
	{ -2, +7, 1 },{ -3, +5, 1 },{ -3, +4, 1 },
	{ +2, +7, 1 },{ +3, +5, 1 },{ +3, +4, 1 },
	{ -2, -6, 2 },{ -2, -5, 2 },{ -3, -3, 2 },
	{ +2, -6, 2 },{ +2, -5, 2 },{ +3, -3, 2 },
	{ -2, +6, 2 },{ -2, +5, 2 },{ -3, +3, 2 },
	{ +2, +6, 2 },{ +2, +5, 2 },{ +3, +3, 2 },
	{ -2, -4, 3 },{ -3, -2, 3 },
	{ +2, -4, 3 },{ +3, -2, 3 },
	{ -2, +4, 3 },{ -3, +2, 3 },
	{ +2, +4, 3 },{ +3, +2, 3 },
	{ -2, -3, 4 },{ -3, -1, 4 },
	{ +2, -3, 4 },{ +3, -1, 4 },
	{ -2, +3, 4 },{ -3, +1, 4 },
	{ +2, +3, 4 },{ +3, +1, 4 },
	{ -2, -2, 5 },{ -3, 0, 5 },{ -2, +2, 5 },
	{ +2, -2, 5 },{ +3, 0, 5 },{ +2, +2, 5 },
	{ -2, -1, 6 },{ -2, +1, 6 },
	{ +2, -1, 6 },{ +2, +1, 6 },
	{ -2, 0, 7 },{ +2, 0, 7 },
	{ 0, 0, -1 }
};
