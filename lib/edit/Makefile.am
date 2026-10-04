## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

angband_files = \
	ArtifactDefinitions.jsonc BaseitemDefinitions.jsonc ClassMagicDefinitions.jsonc \
	ClassSkillDefinitions.jsonc DungeonDefinitions.jsonc EgoDefinitions.jsonc \
	MonraceDefinitions.jsonc MonsterMessages.jsonc QuestPreferences.jsonc \
	SpellDefinitions.jsonc TerrainDefinitions.jsonc \
	TownDefinitionList.jsonc TownPreferences.jsonc VaultDefinitions.jsonc \
	WildernessDefinition.jsonc

EXTRA_DIST = \
  $(angband_files)

SUBDIRS = quests towns

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/edit

angband_DATA = \
  $(angband_files)
endif
