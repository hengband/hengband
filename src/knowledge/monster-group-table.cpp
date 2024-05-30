/*
 * @brief シンボルテキストの配列群
 * @date 2020/03/08
 * @author Hourier
 */

#include "knowledge/monster-group-table.h"
#include "locale/language-switcher.h"

/*!
 * @todo 元々t (町人)がいなかったが、問題ないのか？
 * Description of each monster group.
 */
const std::vector<std::string> monster_group_text = {
    _("ユニーク", "Uniques"),
    _("乗馬可能なモンスター", "Ridable monsters"),
    _("賞金首", "Wanted monsters"),
    _("アンバーの王族", "Amberite"),
    _("アリ", "Ant"),
    _("コウモリ", "Bat"),
    _("ムカデ", "Centipede"),
    _("ドラゴン", "Dragon"),
    _("目玉", "Floating Eye"),
    _("ネコ", "Feline"),
    _("ゴーレム", "Golem"),
    _("標準人間型生物", "Hobbit/Elf/Dwarf"),
    _("ベトベト", "Icky Thing"),
    _("ゼリー", "Jelly"),
    _("コボルド", "Kobold"),
    _("水棲生物", "Aquatic monster"),
    _("モルド", "Mold"),
    _("ナーガ", "Naga"),
    _("オーク", "Orc"),
    _("人間", "Person/Human"),
    _("四足獣", "Quadruped"),
    _("ネズミ", "Rodent"),
    _("スケルトン", "Skeleton"),
    _("デーモン", "Demon"),
    _("ボルテックス", "Vortex"),
    _("イモムシ/大群", "Worm/Worm-Mass"),
    _("イーク", "Yeek"),
    _("ゾンビ/ミイラ", "Zombie/Mummy"),
    _("天使", "Angel"),
    _("鳥", "Bird"),
    _("犬", "Canine"),
    _("エレメンタル", "Elemental"),
    _("トンボ", "Dragon Fly"),
    _("ゴースト", "Ghost"),
    _("雑種", "Hybrid"),
    _("昆虫", "Insect"),
    _("ヘビ", "Snake"),
    _("キラー・ビートル", "Killer Beetle"),
    _("リッチ", "Lich"),
    _("多首の爬虫類", "Multi-Headed Reptile"),
    _("謎の生物", "Mystery Living"),
    _("オーガ", "Ogre"),
    _("巨大人間型生物", "Giant Humanoid"),
    _("クイルスルグ", "Quylthulg"),
    _("爬虫類/両生類", "Reptile/Amphibian"),
    _("蜘蛛/サソリ/ダニ", "Spider/Scorpion/Tick"),
    _("トロル", "Troll"),
    _("バンパイア", "Vampire"),
    _("ワイト/レイス/等", "Wight/Wraith/etc"),
    _("ゾーン/ザレン/等", "Xorn/Xaren/etc"),
    _("イエティ", "Yeti"),
    _("ハウンド", "Zephyr Hound"),
    _("ミミック", "Mimic"),
    _("壁/植物/気体", "Wall/Plant/Gas"),
    _("おばけキノコ", "Mushroom patch"),
    _("球体", "Ball"),
    _("プレイヤー", "Player"),
};

/*
 * Symbols of monsters in each group. Note the "Uniques" group
 * is handled differently.
 */
const std::vector<std::string> monster_group_char = { "Uniques", "Riding", "Wanted", "Amberites", "a", "b", "c", "dD", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
    "pt", "q", "r", "s", "uU", "v", "w", "y", "z", "A", "B", "C", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "V", "W", "X",
    "Y", "Z", "!$&()+./=>?[\\]`{|~", "#%", ",", "*", "@" };
