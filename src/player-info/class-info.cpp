/*!
 * @brief プレイヤーの職業に関する諸テーブル定義
 * @date 2019/04/30
 * @author deskull
 */

#include "player-info/class-info.h"
#include "inventory/inventory-slot-types.h"
#include "player-info/race-info.h"
#include "system/item-entity.h"

/*
 * The magic info
 */
const player_magic *mp_ptr;
std::vector<player_magic> class_magics_info;

const player_class_info *cp_ptr;

/*!
 * @brief 職業情報 /
 * Player Classes
 * @details
 * <pre>
 *      Title,
 *      {STR,INT,WIS,DEX,CON,CHR},
 *      c_dis, c_dev, c_sav, c_stl, c_srh, c_fos, c_thn, c_thb,
 *      x_dis, x_dev, x_sav, x_stl, x_srh, x_fos, x_thn, x_thb,
 *      HD, Exp, pet_upkeep_div
 * </pre>
 */
const std::vector<player_class_info> class_info = {
    {
#ifdef JP
        "戦士",
#endif
        "Warrior",

        { 4, -2, -2, 2, 2, -1 }, 25, 18, 31, 1, 14, 2, 70, 55, 12, 7, 10, 0, 0, 0, 30, 30, 9, 0, 40, 6, 70, 5 },

    {
#ifdef JP
        "メイジ",
#endif
        "Mage",

        { -4, 3, 0, 1, -2, 1 }, 30, 40, 38, 3, 16, 20, 34, 20, 7, 15, 11, 0, 0, 0, 6, 7, 0, 30, 30, 3, 100, 2 },

    {
#ifdef JP
        "プリースト",
#endif
        "Priest",

        { -1, -3, 3, -1, 0, 2 }, 25, 35, 40, 2, 16, 8, 48, 35, 7, 11, 12, 0, 0, 0, 13, 11, 2, 20, 35, 5, 100, 3 },

    {
#ifdef JP
        "盗賊",
#endif
        "Rogue",

        { 2, 1, -2, 3, 1, -1 }, 45, 37, 36, 5, 32, 24, 60, 66, 15, 12, 10, 0, 0, 0, 21, 18, 6, 25, 40, 5, 40, 3 },

    {
#ifdef JP
        "レンジャー",
#endif
        "Ranger",

        { 2, 2, 0, 1, 1, 1 }, 30, 37, 36, 3, 24, 16, 56, 72, 8, 11, 10, 0, 0, 0, 18, 28, 4, 40, 35, 5, 70, 4 },

    {
#ifdef JP
        "パラディン",
#endif
        "Paladin",

        { 3, -3, 1, 0, 2, 2 }, 20, 24, 34, 1, 12, 2, 68, 40, 7, 10, 11, 0, 0, 0, 21, 18, 6, 35, 40, 5, 70, 4 },

    {
#ifdef JP
        "魔法戦士",
#endif
        "Warrior-Mage",

        { 2, 2, 0, 1, 0, 1 }, 30, 35, 36, 2, 18, 16, 50, 25, 7, 10, 10, 0, 0, 0, 15, 11, 4, 40, 35, 5, 70, 3 },

    {
#ifdef JP
        "混沌の戦士",
#endif
        "Chaos-Warrior",

        { 2, 1, -1, 0, 2, -2 }, 20, 25, 34, 1, 14, 12, 65, 40, 7, 11, 10, 0, 0, 0, 20, 17, 6, 25, 40, 5, 70, 4 },

    {
#ifdef JP
        "修行僧",
#endif
        "Monk",

        { 2, -1, 1, 3, 2, 1 }, 45, 34, 36, 5, 32, 24, 64, 60, 15, 11, 10, 0, 0, 0, 18, 18, 6, 30, 35, 5, 60, 3 },

    {
#ifdef JP
        "超能力者",
#endif
        "Mindcrafter",

        { -1, 0, 3, -1, -1, 2 }, /* note: spell stat is Wis */
        30, 33, 38, 3, 22, 16, 50, 40, 10, 11, 10, 0, 0, 0, 14, 18, 2, 25, 35, 5, 100, 3 },

    {
#ifdef JP
        "ハイ=メイジ",
#endif
        "High-Mage",

        { -4, 4, 0, 0, -2, 1 }, 30, 40, 38, 3, 16, 20, 34, 20, 7, 15, 11, 0, 0, 0, 6, 7, 0, 30, 25, 3, 100, 2 },

    {
#ifdef JP
        "観光客",
#endif
        "Tourist", { -1, -1, -1, -1, -1, -1 }, 15, 18, 28, 1, 12, 2, 40, 20, 5, 7, 9, 0, 0, 0, 11, 11, 0, -30, 40, 4, 100, 3 },

    {
#ifdef JP
        "ものまね師",
#endif
        "Imitator", { 0, 1, -1, 2, 0, 1 }, 25, 30, 36, 2, 18, 16, 60, 50, 7, 10, 10, 0, 0, 0, 18, 20, 5, 10, 20, 5, 70, 4 },

    {
#ifdef JP
        "魔獣使い",
#endif
        "BeastMaster", { 1, -1, -1, 1, 0, 2 }, 20, 25, 32, 2, 18, 16, 52, 63, 7, 10, 10, 0, 0, 0, 14, 25, 3, 20, 10, 5, 70, 3 },

    {
#ifdef JP
        "スペルマスター",
#endif
        "Sorcerer",

        { -5, 6, -2, 2, 0, -2 }, 30, 48, 75, 2, 12, 22, 0, 0, 7, 18, 13, 0, 0, 0, 0, 0, 4, 60, 25, 1, 1, 1 },

    {
#ifdef JP
        "アーチャー",
#endif
        "Archer",

        { 2, -1, -1, 2, 1, 0 }, 38, 24, 35, 4, 24, 16, 56, 82, 12, 10, 10, 0, 0, 0, 18, 36, 6, 10, 40, 4, 70, 2 },

    {
#ifdef JP
        "魔道具術師",
#endif
        "Magic-Eater",

        { -1, 2, 1, 2, -2, 1 }, 25, 42, 36, 2, 20, 16, 48, 35, 7, 16, 10, 0, 0, 0, 13, 11, 3, 30, 30, 5, 100, 3 },

    {
#ifdef JP
        "吟遊詩人",
#endif
        "Bard", /* Note : spell stat is Charisma */
        { -2, 1, 2, -1, -2, 4 }, 20, 33, 34, -5, 16, 20, 34, 20, 8, 13, 11, 0, 0, 0, 10, 8, 2, 40, 25, 4, 70, 2 },

    {
#ifdef JP
        "赤魔道師",
#endif
        "Red-Mage",

        { 2, 2, -1, 1, 0, -1 }, 20, 34, 34, 1, 16, 10, 56, 25, 7, 11, 11, 0, 0, 0, 18, 11, 4, 40, 40, 5, 70, 3 },

    {
#ifdef JP
        "剣術家",
#endif
        "Samurai",

        { 3, -2, 1, 2, 1, 0 }, 25, 18, 32, 2, 16, 6, 70, 40, 12, 7, 10, 0, 0, 0, 23, 18, 6, 30, 40, 5, 70, 4 },

    {
#ifdef JP
        "練気術師",
#endif
        "ForceTrainer",

        { 0, -1, 3, 2, 1, 1 }, 30, 34, 38, 4, 32, 24, 50, 40, 10, 11, 11, 0, 0, 0, 14, 15, 2, 35, 40, 4, 60, 2 },

    {
#ifdef JP
        "青魔道師",
#endif
        "Blue-Mage",

        { -4, 4, -1, 1, -2, -1 }, 30, 40, 36, 3, 20, 16, 40, 25, 7, 16, 11, 0, 0, 0, 6, 7, 2, 30, 35, 3, 100, 2 },

    {
#ifdef JP
        "騎兵",
#endif
        "Cavalry", { 2, -2, -2, 2, 2, 0 }, 20, 18, 32, 1, 16, 10, 60, 66, 10, 7, 10, 0, 0, 0, 22, 26, 5, 20, 35, 5, 100, 3 },

    {
#ifdef JP
        "狂戦士",
#endif
        "Berserker",

        { 8, -20, -20, 4, 4, -5 }, -100, -1000, -200, -100, -100, -100, 120, -2000, 0, 0, 0, 0, 0, 0, 50, 0, 11, 60, 255, 6, 70, 7 },

    {
#ifdef JP
        "鍛冶師",
#endif
        "Weaponsmith",

        { 3, -1, -1, 1, 0, -1 }, 30, 28, 28, 1, 20, 10, 60, 45, 10, 10, 10, 0, 0, 0, 21, 15, 6, 30, 40, 5, 150, 5 },
    {
#ifdef JP
        "鏡使い",
#endif
        "Mirror-Master",

        { -2, 3, 1, -1, -2, 1 }, 30, 33, 40, 3, 14, 16, 34, 30, 10, 11, 12, 0, 0, 0, 6, 10, 2, 30, 30, 3, 100, 3 },
    {
#ifdef JP
        "忍者",
#endif
        "Ninja",

        { 0, -1, -1, 3, 2, -1 }, 45, 24, 36, 8, 48, 32, 70, 66, 15, 10, 10, 0, 0, 0, 25, 18, 2, 20, 40, 4, 20, 1 },

    {
#ifdef JP
        "スナイパー",
#endif
        "Sniper",

        { 2, -1, -1, 2, 1, 0 }, 25, 24, 28, 5, 32, 18, 56, 72, 12, 10, 10, 0, 0, 0, 18, 28, 2, 20, 40, 4, 70, 2 },

    {
#ifdef JP
        "元素使い",
#endif
        "Elementalist",

        { -3, 1, 3, 0, -1, 0 }, 30, 40, 38, 3, 16, 20, 34, 20, 7, 15, 11, 0, 0, 0, 6, 7, 0, 30, 25, 3, 100, 2 },
};

/*!
 * @brief 職業とレベル毎のプレイヤー称号テーブル / Class titles for the player.
 * <pre>
 * The player gets a new title every five levels, so each class
 * needs only ten titles total.
 * </pre>
 */
const std::vector<std::vector<std::string_view>> player_titles = {
    /* Warrior */
    {
        _("新参兵", "Rookie"),
        _("兵士", "Soldier"),
        _("傭兵", "Mercenary"),
        _("古参兵", "Veteran"),
        _("剣士", "Swordsman"),
        _("闘士", "Champion"),
        _("英雄", "Hero"),
        _("男爵", "Baron"),
        _("伯爵", "Duke"),
        _("君主", "Lord"),
    },

    /* Mage */
    {
        _("練習生", "Apprentice"),
        _("奇術師", "Trickster"),
        _("幻術師", "Illusionist"),
        _("呪術師", "Spellbinder"),
        _("召霊師", "Evoker"),
        _("召魔師", "Conjurer"),
        _("魔術師", "Warlock"),
        _("魔道師", "Sorcerer"),
        _("イプシシマス", "Ipsissimus"),
        _("大魔道師", "Archmage"),
    },

    /* Priest */
    {
        _("信者", "Believer"),
        _("侍僧", "Acolyte"),
        _("熟練僧", "Adept"),
        _("聖職者", "Curate"),
        _("伝道師", "Canon"),
        _("牧師", "Priest"),
        _("聖人", "High Priest"),
        _("祭司", "Cardinal"),
        _("祭司長", "Inquisitor"),
        _("教皇", "Pope"),
    },

    /* Rogue */
    {
        _("すり", "Cutpurse"),
        _("追いはぎ", "Robber"),
        _("夜盗", "Burglar"),
        _("こそ泥", "Filcher"),
        _("ペテン師", "Sharper"),
        _("ロウシーフ", "Low Thief"),
        _("ハイシーフ", "High Thief"),
        _("マスター", "Master Thief"),
        _("アサシン", "Assassin"),
        _("頭領", "Guildmaster"),
    },

    /* Ranger */
    {
        _("使い走り", "Runner"),
        _("馳夫", "Strider"),
        _("斥候", "Scout"),
        _("狩人", "Courser"),
        _("追跡者", "Tracker"),
        _("先導者", "Guide"),
        _("探険者", "Pathfinder"),
        _("野伏", "Low Ranger"),
        _("野伏頭", "High Ranger"),
        _("野伏の総領", "Ranger Lord"),
    },

    /* Paladin */
    {
        _("勇士", "Gallant"),
        _("衛士", "Keeper"),
        _("保護者", "Protector"),
        _("防衛者", "Defender"),
        _("護衛者", "Warder"),
        _("騎士", "Knight"),
        _("重騎士", "Guardian"),
        _("聖騎士", "Low Paladin"),
        _("上級聖騎士", "High Paladin"),
        _("聖騎士団長", "Paladin Lord"),
    },

    /* Warrior-Mage */
    {
        _("見習い", "Novice"),
        _("徒弟", "Apprentice"),
        _("一人前", "Journeyman"),
        _("古参兵", "Veteran"),
        _("魔術兵士", "Enchanter"),
        _("魔術闘士", "Champion"),
        _("魔術の英雄", "Mage-Hero"),
        _("魔男爵", "Baron Mage"),
        _("戦闘魔術士", "Battlemage"),
        _("知識の守護者", "Wizard Lord"),
    },

    /* Chaos Warrior */
    {
        _("新参兵", "Rookie"),
        _("兵士", "Soldier"),
        _("傭兵", "Mercenary"),
        _("古参兵", "Veteran"),
        _("剣士", "Swordsman"),
        _("闘士", "Champion"),
        _("混沌の英雄", "Chaos Hero"),
        _("混沌の男爵", "Chaos Baron"),
        _("混沌の公爵", "Chaos Duke"),
        _("混沌の王者", "Chaos Lord"),
    },

    /* Monk */
    {
        _("入門者", "Initiate"),
        _("弟子", "Brother"),
        _("直弟子", "Disciple"),
        _("師範代", "Immaculate"),
        _("師範", "Master"),
        _("道場主", "Soft Master"),
        _("名人", "Hard Master"),
        _("大名人", "Flower Master"),
        _("拳聖", "Dragon Master"),
        _("拳神", "Grand Master"),
    },

    /* Mindcrafter */
    {
        _("練習生", "Trainee"),
        _("見習い", "Acolyte"),
        _("熟練士", "Adept"),
        _("熟達士", "Immaculate"),
        _("黙想士", "Contemplator"),
        _("心術士", "Mentalist"),
        _("サイキック", "Psychic"),
        _("サイオニック", "Psionicist"),
        _("超能力者", "Esper"),
        _("精神の支配者", "Mindmaster"),
    },

    /* High Mage */
    {
        _("練習生", "Apprentice"),
        _("奇術師", "Trickster"),
        _("幻術師", "Illusionist"),
        _("呪術師", "Spellbinder"),
        _("召霊師", "Evoker"),
        _("召魔師", "Conjurer"),
        _("魔術師", "Warlock"),
        _("魔道師", "Sorcerer"),
        _("イプシシマス", "Ipsissimus"),
        _("大魔道師", "Archmage"),
    },

    /* Tourist */
    {
        _("プー太郎", "Rambler"),
        _("観光客", "Sightseer"),
        _("周遊旅行者", "Excursionist"),
        _("遍歴者", "Peregrinator"),
        _("旅行者", "Traveler"),
        _("放浪者", "Journeyer"),
        _("航海者", "Voyager"),
        _("探検家", "Explorer"),
        _("冒険家", "Adventurer"),
        _("スペランカー", "Spelunker"),
    },

    /* Imitator */
    {
        _("これから", "Yet"),
        _("いまいち", "Lacks"),
        _("まだまだ", "Still more"),
        _("ぼちぼち", "So so"),
        _("そこそこ", "All right"),
        _("まあまあ", "Not bad"),
        _("なかなか", "Considerable"),
        _("いけいけ", "Go go"),
        _("そうとう", "Sizable"),
        _("えらい", "Great man"),
    },

    /* Beastmaster */
    {
        _("イモリ使い", "Newt Master"),
        _("ヘビ使い", "Snake Master"),
        _("クモ使い", "Spider Master"),
        _("狼使い", "Wolf Master"),
        _("トラ使い", "Tiger Master"),
        _("甲虫使い", "Beetle Master"),
        _("ヒドラ使い", "Hydra Master"),
        _("ハウンド使い", "Hound Master"),
        _("ムーマク使い", "Mumak Master"),
        _("ドラゴン使い", "Dragon Master"),
    },

    /* Sorcerer; same as Mage */
    {
        _("練習生", "Apprentice"),
        _("奇術師", "Trickster"),
        _("幻術師", "Illusionist"),
        _("呪術師", "Spellbinder"),
        _("召霊師", "Evoker"),
        _("召魔師", "Conjurer"),
        _("魔術師", "Warlock"),
        _("魔道師", "Sorcerer"),
        _("イプシシマス", "Ipsissimus"),
        _("大魔道師", "Archmage"),
    },

    /* Archer */
    {
        _("新参兵", "Rookie"),
        _("兵士", "Soldier"),
        _("傭兵", "Mercenary"),
        _("古参兵", "Veteran"),
        _("弩弓士", "Bowman"),
        _("闘士", "Champion"),
        _("英雄", "Hero"),
        _("男爵", "Baron"),
        _("伯爵", "Duke"),
        _("領主", "Lord"),
    },

    /* Magic eater */
    {
        _("無知なる者", "Apprentice"),
        _("入門者", "Beginner"),
        _("奇術師", "Jagguler"),
        _("秘術師", "Skilled"),
        _("秘術師", "Conjurer"),
        _("熟練者", "Magician"),
        _("達人", "Master"),
        _("達人", "Master"),
        _("魔道師", "Wizard"),
        _("全てを知る者", "Almighty"),
    },

    /* Bard */
    {
        _("見習い", "Apprentice"),
        _("作曲家", "Songsmith"),
        _("吟遊詩人", "Bard"),
        _("コンパニオン", "Companion"),
        _("心の癒し手", "Minstrel"),
        _("竪琴師", "Harper"),
        _("伝承の紡ぎ手", "Loreweaver"),
        _("詩神の申し子", "Muse"),
        _("夢紡ぎ", "Dreamweaver"),
        _("マスター", "Master Harper"),
    },

    /* Red Mage */
    {
        _("見習い", "Novice"),
        _("徒弟", "Apprentice"),
        _("一人前", "Journeyman"),
        _("古参兵", "Veteran"),
        _("魔術兵士", "Enchanter"),
        _("魔術闘士", "Champion"),
        _("魔術の英雄", "Mage-Hero"),
        _("魔男爵", "Baron Mage"),
        _("戦闘魔術士", "Battlemage"),
        _("知識の守護者", "Wizard Lord"),
    },

    /* Samurai */
    {
        _("入門者", "Initiate"),
        _("弟子", "Brother"),
        _("直弟子", "Disciple"),
        _("師範代", "Immaculate"),
        _("師範", "Master"),
        _("道場主", "Soft Master"),
        _("名人", "Hard Master"),
        _("大名人", "Flower Master"),
        _("剣聖", "Dragon Master"),
        _("剣神", "Grand Master"),
    },

    /* ForceTrainer */
    {
        _("入門者", "Initiate"),
        _("弟子", "Brother"),
        _("直弟子", "Disciple"),
        _("師範代", "Immaculate"),
        _("師範", "Master"),
        _("道場主", "Soft Master"),
        _("名人", "Hard Master"),
        _("大名人", "Flower Master"),
        _("拳聖", "Dragon Master"),
        _("拳神", "Grand Master"),
    },

    /* Blue Mage */
    {
        _("練習生", "Apprentice"),
        _("奇術師", "Trickster"),
        _("幻術師", "Illusionist"),
        _("呪術師", "Spellbinder"),
        _("召霊師", "Evoker"),
        _("召魔師", "Conjurer"),
        _("魔術師", "Warlock"),
        _("魔道師", "Sorcerer"),
        _("イプシシマス", "Ipsissimus"),
        _("大魔道師", "Archmage"),
    },

    /* Cavalry */
    {
        _("新参兵", "Rookie"),
        _("兵士", "Soldier"),
        _("傭兵", "Mercenary"),
        _("古参兵", "Veteran"),
        _("剣士", "Swordsman"),
        _("闘士", "Champion"),
        _("英雄", "Hero"),
        _("男爵", "Baron"),
        _("伯爵", "Duke"),
        _("領主", "Lord"),
    },

    /* Berserker */
    {
        _("バーサーカー", "Berserker"),
        _("バーサーカー", "Berserker"),
        _("バーサーカー", "Berserker"),
        _("怒りの公爵", "Rage Prince"),
        _("怒りの公爵", "Rage Prince"),
        _("怒りの公爵", "Rage Prince"),
        _("怒りの王", "Rage King"),
        _("怒りの王", "Rage King"),
        _("怒りの王", "Rage King"),
        _("怒りの化身", "God of Rage"),
    },

    /* Weaponsmith */
    {
        _("銅を鍛えし者", "Copper smith"),
        _("鉄を鍛えし者", "Iron smith"),
        _("鋼を鍛えし者", "Steel smith"),
        _("銀を鍛えし者", "Silver smith"),
        _("竜を鍛えし者", "Dragon smith"),
        _("霊を鍛えし者", "Spirit smith"),
        _("魔を鍛えし者", "Magic smith"),
        _("魂を鍛えし者", "Soul smith"),
        _("神を鍛えし者", "God smith"),
        _("全を鍛えし者", "AlmightySmith"),
    },

    /* Mirror Master */
    {
        _("鏡を見る人", "Mirrorstarer"),
        _("鏡磨き", "Mirrorcleaner"),
        _("鏡職人", "Mirrormaker"),
        _("鏡術師", "Mirrormagician"),
        _("鏡導師", "Mirror Guru"),
        _("鏡の賢者", "Mirror Mage"),
        _("鏡の王", "Mirror King"),
        _("鏡の皇帝", "Mirror Emperor"),
        _("鏡の化身", "Mirror Avatar"),
        _("ラフノール王", "Ruffnor King"),
    },

    /* Ninja */
    {
        _("訓練生", "Trainee"),
        _("仕手", "Myrmidon"),
        _("熟達者", "Initiate"),
        _("短刀使い", "Knifer"),
        _("切り裂き", "Bladesman"),
        _("凄腕", "Hashishin"),
        _("漆黒の刃", "Black Dagger"),
        _("闇の一撃", "Shadowstrike"),
        _("暗殺者", "Assassinator"),
        _("死の長き腕", "Death Lord"),
    },

    /* Sniper */
    {
        _("新参兵", "Rookie"),
        _("兵士", "Soldier"),
        _("傭兵", "Mercenary"),
        _("古参兵", "Veteran"),
        _("剣士", "Swordsman"),
        _("闘士", "Champion"),
        _("英雄", "Hero"),
        _("男爵", "Baron"),
        _("伯爵", "Duke"),
        _("領主", "Lord"),
    },

    /* Elementalist */
    {
        _("練習生", "Apprentice"),
        _("奇術師", "Trickster"),
        _("幻術師", "Illusionist"),
        _("呪術師", "Spellbinder"),
        _("召霊師", "Evoker"),
        _("召魔師", "Conjurer"),
        _("魔術師", "Warlock"),
        _("魔道師", "Sorcerer"),
        _("イプシシマス", "Ipsissimus"),
        _("大魔道師", "Archmage"),
    },
};
