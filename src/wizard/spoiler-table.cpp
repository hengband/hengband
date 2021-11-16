#include "wizard/spoiler-table.h"

/* The basic items categorized by type */
const std::vector<grouper> group_item_list = {
    { { ItemKindType::SHOT, ItemKindType::ARROW, ItemKindType::BOLT }, _("射撃物", "Ammo") },
    { { ItemKindType::BOW }, _("弓", "Bows") },
    { { ItemKindType::DIGGING, ItemKindType::POLEARM, ItemKindType::HAFTED, ItemKindType::SWORD }, _("武器", "Weapons") },
    { { ItemKindType::SOFT_ARMOR, ItemKindType::HARD_ARMOR, ItemKindType::DRAG_ARMOR }, _("防具 (体)", "Armour (Body)") },
    { { ItemKindType::BOOTS, ItemKindType::GLOVES, ItemKindType::HELM, ItemKindType::CROWN, ItemKindType::SHIELD, ItemKindType::CLOAK }, _("防具 (その他)", "Armour (Misc)") },

    { { ItemKindType::LITE }, _("光源", "Light Sources") },
    { { ItemKindType::AMULET }, _("アミュレット", "Amulets") },
    { { ItemKindType::RING }, _("指輪", "Rings") },
    { { ItemKindType::STAFF }, _("杖", "Staffs") },
    { { ItemKindType::WAND }, _("魔法棒", "Wands") },
    { { ItemKindType::ROD }, _("ロッド", "Rods") },
    { { ItemKindType::SCROLL }, _("巻物", "Scrolls") },
    { { ItemKindType::POTION }, _("薬", "Potions") },
    { { ItemKindType::FOOD }, _("食料", "Food") },

    { { ItemKindType::LIFE_BOOK }, _("魔法書 (生命)", "Books (Life)") },
    { { ItemKindType::SORCERY_BOOK }, _("魔法書 (仙術)", "Books (Sorcery)") },
    { { ItemKindType::NATURE_BOOK }, _("魔法書 (自然)", "Books (Nature)") },
    { { ItemKindType::CHAOS_BOOK }, _("魔法書 (カオス)", "Books (Chaos)") },
    { { ItemKindType::DEATH_BOOK }, _("魔法書 (暗黒)", "Books (Death)") },
    { { ItemKindType::TRUMP_BOOK }, _("魔法書 (トランプ)", "Books (Trump)") },
    { { ItemKindType::ARCANE_BOOK }, _("魔法書 (秘術)", "Books (Arcane)") },
    { { ItemKindType::CRAFT_BOOK }, _("魔法書 (匠)", "Books (Craft)") },
    { { ItemKindType::DEMON_BOOK }, _("魔法書 (悪魔)", "Books (Daemon)") },
    { { ItemKindType::CRUSADE_BOOK }, _("魔法書 (破邪)", "Books (Crusade)") },
    { { ItemKindType::MUSIC_BOOK }, _("歌集", "Song Books") },
    { { ItemKindType::HISSATSU_BOOK }, _("武芸の書", "Books (Kendo)") },
    { { ItemKindType::HEX_BOOK }, _("魔法書 (呪術)", "Books (Hex)") },

    { { ItemKindType::WHISTLE }, _("笛", "Whistle") },
    { { ItemKindType::CAPTURE }, _("キャプチャー・ボール", "Capture Ball") },
    { { ItemKindType::CARD }, _("エクスプレスカード", "Express Card") },

    { { ItemKindType::CHEST }, _("箱", "Chests") },

    { { ItemKindType::FIGURINE }, _("人形", "Magical Figurines") },
    { { ItemKindType::STATUE }, _("像", "Statues") },
    { { ItemKindType::CORPSE }, _("死体", "Corpses") },

    { { ItemKindType::SKELETON, ItemKindType::BOTTLE, ItemKindType::JUNK, ItemKindType::SPIKE, ItemKindType::FLASK, ItemKindType::PARCHMENT }, _("その他", "Misc") },
};

/* The artifacts categorized by type */
const std::vector<grouper> group_artifact_list = {
    { { ItemKindType::SWORD }, _("刀剣", "Edged Weapons") },
    { { ItemKindType::POLEARM }, _("槍/斧", "Polearms") },
    { { ItemKindType::HAFTED }, _("鈍器", "Hafted Weapons") },
    { { ItemKindType::DIGGING }, _("シャベル/つるはし", "Shovels/Picks") },
    { { ItemKindType::BOW }, _("飛び道具", "Bows") },
    { { ItemKindType::ARROW, ItemKindType::BOLT }, _("矢", "Ammo") },

    { { ItemKindType::SOFT_ARMOR, ItemKindType::HARD_ARMOR, ItemKindType::DRAG_ARMOR }, _("鎧", "Body Armor") },

    { { ItemKindType::CLOAK }, _("クローク", "Cloaks") },
    { { ItemKindType::SHIELD, ItemKindType::CARD }, _("盾", "Shields") },
    { { ItemKindType::HELM, ItemKindType::CROWN }, _("兜/冠", "Helms/Crowns") },
    { { ItemKindType::GLOVES }, _("籠手", "Gloves") },
    { { ItemKindType::BOOTS }, _("靴", "Boots") },

    { { ItemKindType::LITE }, _("光源", "Light Sources") },
    { { ItemKindType::AMULET }, _("アミュレット", "Amulets") },
    { { ItemKindType::RING }, _("指輪", "Rings") },
};

flag_desc stat_flags_desc[MAX_STAT_FLAGS_DESCRIPTION] = { { TR_STR, _("腕力", "STR") }, { TR_INT, _("知能", "INT") }, { TR_WIS, _("賢さ", "WIS") },
    { TR_DEX, _("器用さ", "DEX") },
    { TR_CON, _("耐久力", "CON") }, { TR_CHR, _("魅力", "CHR") } };

flag_desc pval_flags1_desc[MAX_PVAL_FLAGS_DESCRIPTION] = { { TR_MAGIC_MASTERY, _("魔法道具使用能力", "Magic Mastery") }, { TR_STEALTH, _("隠密", "Stealth") },
    { TR_SEARCH, _("探索", "Searching") }, { TR_INFRA, _("赤外線視力", "Infravision") }, { TR_TUNNEL, _("採掘", "Tunneling") },
    { TR_BLOWS, _("攻撃回数", "Attacks") }, { TR_SPEED, _("スピード", "Speed") } };

flag_desc slay_flags_desc[MAX_SLAY_FLAGS_DESCRIPTION] = {
    { TR_SLAY_ANIMAL, _("動物", "Animal") },
    { TR_KILL_ANIMAL, _("*動物*", "XAnimal") },
    { TR_SLAY_EVIL, _("邪悪", "Evil") },
    { TR_KILL_EVIL, _("*邪悪*", "XEvil") },
    { TR_SLAY_EVIL, _("善良", "Good") },
    { TR_KILL_EVIL, _("*善良*", "XGood") },
    { TR_SLAY_HUMAN, _("人間", "Human") },
    { TR_KILL_HUMAN, _("*人間*", "XHuman") },
    { TR_SLAY_UNDEAD, _("アンデッド", "Undead") },
    { TR_KILL_UNDEAD, _("*アンデッド*", "XUndead") },
    { TR_SLAY_DEMON, _("悪魔", "Demon") },
    { TR_KILL_DEMON, _("*悪魔*", "XDemon") },
    { TR_SLAY_ORC, _("オーク", "Orc") },
    { TR_KILL_ORC, _("*オーク*", "XOrc") },
    { TR_SLAY_TROLL, _("トロル", "Troll") },
    { TR_KILL_TROLL, _("*トロル*", "XTroll") },
    { TR_SLAY_GIANT, _("巨人", "Giant") },
    { TR_KILL_GIANT, _("*巨人*", "Xgiant") },
    { TR_SLAY_DRAGON, _("ドラゴン", "Dragon") },
    { TR_KILL_DRAGON, _("*ドラゴン*", "Xdragon") },
};

/* Elemental brands for weapons */
flag_desc brand_flags_desc[MAX_BRAND_FLAGS_DESCRIPTION] = {
    { TR_BRAND_ACID, _("溶解", "Acid Brand") },
    { TR_BRAND_ELEC, _("電撃", "Lightning Brand") },
    { TR_BRAND_FIRE, _("焼棄", "Flame Tongue") },
    { TR_BRAND_COLD, _("凍結", "Frost Brand") },
    { TR_BRAND_POIS, _("毒殺", "Poisoned") },
    { TR_FORCE_WEAPON, _("理力", "Force") },
    { TR_CHAOTIC, _("混沌", "Mark of Chaos") },
    { TR_BRAND_MAGIC, _("魔術", "Magical Brand") },
    { TR_VAMPIRIC, _("吸血", "Vampiric") },
    { TR_EARTHQUAKE, _("地震", "Earthquake impact on hit") },
    { TR_VORPAL, _("切れ味", "Very sharp") },
    { TR_IMPACT, _("強撃", "Smash hit") },
};

const flag_desc resist_flags_desc[MAX_RESISTANCE_FLAGS_DESCRIPTION] = {
    { TR_RES_ACID, _("酸", "Acid") },
    { TR_RES_ELEC, _("電撃", "Lightning") },
    { TR_RES_FIRE, _("火炎", "Fire") },
    { TR_RES_COLD, _("冷気", "Cold") },
    { TR_RES_POIS, _("毒", "Poison") },
    { TR_RES_FEAR, _("恐怖", "Fear") },
    { TR_RES_LITE, _("閃光", "Light") },
    { TR_RES_DARK, _("暗黒", "Dark") },
    { TR_RES_BLIND, _("盲目", "Blindness") },
    { TR_RES_CONF, _("混乱", "Confusion") },
    { TR_RES_SOUND, _("轟音", "Sound") },
    { TR_RES_SHARDS, _("破片", "Shards") },
    { TR_RES_NETHER, _("地獄", "Nether") },
    { TR_RES_NEXUS, _("因果混乱", "Nexus") },
    { TR_RES_CHAOS, _("カオス", "Chaos") },
    { TR_RES_DISEN, _("劣化", "Disenchantment") },
    { TR_RES_TIME, _("時間逆転", "Time") },
    { TR_RES_WATER, _("水", "Water") },
    { TR_RES_CURSE, _("呪力", "Curse") },
};

/* Elemental immunities (along with poison) */
const flag_desc immune_flags_desc[MAX_IMMUNITY_FLAGS_DESCRIPTION] = {
    { TR_IM_ACID, _("酸", "Acid") },
    { TR_IM_ELEC, _("電撃", "Lightning") },
    { TR_IM_FIRE, _("火炎", "Fire") },
    { TR_IM_COLD, _("冷気", "Cold") },
};

/* Sustain stats -  these are given their "own" line in the spoiler file, mainly for simplicity */
const flag_desc sustain_flags_desc[MAX_SUSTAINER_FLAGS_DESCRIPTION] = {
    { TR_SUST_STR, _("腕力", "STR") },
    { TR_SUST_INT, _("知能", "INT") },
    { TR_SUST_WIS, _("賢さ", "WIS") },
    { TR_SUST_DEX, _("器用さ", "DEX") },
    { TR_SUST_CON, _("耐久力", "CON") },
    { TR_SUST_CHR, _("魅力", "CHR") },
};

/* Miscellaneous magic given by an object's "flags2" field */
const flag_desc misc_flags2_desc[] = {
    { TR_THROW, _("投擲", "Throwing") },
    { TR_REFLECT, _("反射", "Reflection") },
    { TR_FREE_ACT, _("麻痺知らず", "Free Action") },
    { TR_HOLD_EXP, _("経験値維持", "Hold Experience") },
};

/* Miscellaneous magic given by an object's "flags3" field */
const flag_desc misc_flags3_desc[MAX_MISC3_FLAGS_DESCRIPTION] = {
    { TR_SH_FIRE, _("火炎オーラ", "Fiery Aura") },
    { TR_SH_ELEC, _("電撃オーラ", "Electric Aura") },
    { TR_SH_COLD, _("冷気オーラ", "Coldly Aura") },
    { TR_SELF_FIRE, _("自傷火炎", "Self Fire") },
    { TR_SELF_ELEC, _("自傷電撃", "Self Elect") },
    { TR_SELF_COLD, _("自傷冷気", "Self Cold") },
    { TR_NO_TELE, _("反テレポート", "Prevent Teleportation") },
    { TR_NO_MAGIC, _("反魔法", "Anti-Magic") },
    { TR_PERSITENT_CURSE, _("執拗呪詛", "Persistent Curse") },
    { TR_LEVITATION, _("浮遊", "Levitation") },
    { TR_SEE_INVIS, _("可視透明", "See Invisible") },
    { TR_TELEPATHY, _("テレパシー", "ESP") },
    { TR_ESP_ANIMAL, _("動物感知", "Sense Animal") },
    { TR_ESP_UNDEAD, _("不死感知", "Sense Undead") },
    { TR_ESP_DEMON, _("悪魔感知", "Sense Demon") },
    { TR_ESP_ORC, _("オーク感知", "Sense Orc") },
    { TR_ESP_TROLL, _("トロル感知", "Sense Troll") },
    { TR_ESP_GIANT, _("巨人感知", "Sense Giant") },
    { TR_ESP_DRAGON, _("ドラゴン感知", "Sense Dragon") },
    { TR_ESP_HUMAN, _("人間感知", "Sense Human") },
    { TR_ESP_EVIL, _("邪悪感知", "Sense Evil") },
    { TR_ESP_GOOD, _("善良感知", "Sense Good") },
    { TR_ESP_NONLIVING, _("無生物感知", "Sense Nonliving") },
    { TR_ESP_UNIQUE, _("ユニーク感知", "Sense Unique") },
    { TR_SLOW_DIGEST, _("遅消化", "Slow Digestion") },
    { TR_REGEN, _("急速回復", "Regeneration") },
    { TR_WARNING, _("警告", "Warning") },
    /*	{ TR_XTRA_MIGHT, _("強力射撃", "Extra Might") }, */
    { TR_XTRA_SHOTS, _("追加射撃", "+1 Extra Shot") }, /* always +1? */
    { TR_DRAIN_EXP, _("経験値吸収", "Drains Experience") },
    { TR_AGGRAVATE, _("反感", "Aggravate") },
    { TR_BLESSED, _("祝福", "Blessed Blade") },
    { TR_DEC_MANA, _("消費魔力減少", "Decrease Mana Consumption Rate") },
    { TR_EASY_SPELL, _("呪文失敗率減少", "Easy Spell") },
    { TR_EASY2_WEAPON, _("二刀流向き", "Dual combat") },
    { TR_SUPPORTIVE, _("攻撃補助", "Support Attack") },
    { TR_RIDING, _("乗馬適正", "Riding") },
    { TR_INVULN_ARROW, _("射撃無効", "Immune Arrow") },
};
