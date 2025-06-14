## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

angband_files = \
	ArtifactDefinitions.jsonc BaseitemDefinitions.jsonc ClassMagicDefinitions.jsonc \
	ClassSkillDefinitions.txt DungeonDefinitions.txt EgoDefinitions.txt \
	MonraceDefinitions.jsonc MonsterMessages.jsonc QuestDefinitionList.txt \
	QuestPreferences.txt SpellDefinitions.jsonc TerrainDefinitions.txt \
	TownDefinitionList.txt TownPreferences.txt VaultDefinitions.txt \
	WildernessDefinition.txt

EXTRA_DIST = \
  $(angband_files)

SUBDIRS = quests towns

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/edit

angband_DATA = \
  $(angband_files)
endif
