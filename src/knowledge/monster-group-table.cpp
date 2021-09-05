/*
 * @brief シンボルテキストの配列群
 * @date 2020/03/08
 * @author Hourier
 */

#include "knowledge/monster-group-table.h"

/*!
 * @todo 元々t (町人)がいなかったが、問題ないのか？
 * Description of each monster group.
 */
concptr monster_group_text[] = {
#ifdef JP
    "ユニーク", "乗馬可能なモンスター", "賞金首", "アンバーの王族", "アリ", "コウモリ", "ムカデ", "ドラゴン", "目玉", "ネコ", "ゴーレム", "標準人間型生物",
    "ベトベト", "ゼリー", "コボルド", "水棲生物", "モルド", "ナーガ", "オーク", "人間", "四足獣", "ネズミ", "スケルトン", "デーモン", "ボルテックス",
    "イモムシ/大群", "イーク", "ゾンビ/ミイラ", "天使", "鳥", "犬", "エレメンタル", "トンボ", "ゴースト", "雑種", "昆虫", "ヘビ", "キラー・ビートル", "リッチ",
    "多首の爬虫類", "謎の生物", "オーガ", "巨大人間型生物", "クイルスルグ", "爬虫類/両生類", "蜘蛛/サソリ/ダニ", "トロル", "バンパイア", "ワイト/レイス/等",
    "ゾーン/ザレン/等", "イエティ", "ハウンド", "ミミック", "壁/植物/気体", "おばけキノコ", "球体", "プレイヤー",
#else
    "Uniques", "Ridable monsters", "Wanted monsters", "Amberite", "Ant", "Bat", "Centipede", "Dragon", "Floating Eye", "Feline", "Golem", "Hobbit/Elf/Dwarf",
    "Icky Thing", "Jelly", "Kobold", "Aquatic monster", "Mold", "Naga", "Orc", "Person/Human", "Quadruped", "Rodent", "Skeleton", "Demon", "Vortex",
    "Worm/Worm-Mass", "Yeek", "Zombie/Mummy", "Angel", "Bird", "Canine", "Elemental", "Dragon Fly", "Ghost", "Hybrid", "Insect", "Snake", "Killer Beetle",
    "Lich", "Multi-Headed Reptile", "Mystery Living", "Ogre", "Giant Humanoid", "Quylthulg", "Reptile/Amphibian", "Spider/Scorpion/Tick", "Troll", "Vampire",
    "Wight/Wraith/etc", "Xorn/Xaren/etc", "Yeti", "Zephyr Hound", "Mimic", "Wall/Plant/Gas", "Mushroom patch", "Ball", "Player",
#endif
    nullptr
};

/*
 * Symbols of monsters in each group. Note the "Uniques" group
 * is handled differently.
 */
concptr monster_group_char[] = { (char *)-1L, (char *)-2L, (char *)-3L, (char *)-4L, "a", "b", "c", "dD", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
    "pt", "q", "r", "s", "uU", "v", "w", "y", "z", "A", "B", "C", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "V", "W", "X",
    "Y", "Z", "!$&()+./=>?[\\]`{|~", "#%", ",", "*", "@", nullptr };
