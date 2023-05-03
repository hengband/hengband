## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

angband_files = \
	ArtifactDefinitions.txt BaseitemDefinitions.txt ClassMagicDefinitions.txt \
	ClassSkillDefinitions.txt DungeonDefinitions.txt EgoDefinitions.txt \
	MonsterRaceDefinitions.txt QuestDefinitionList.txt QuestPreferences.txt \
	TerrainDefinitions.txt TownDefinitionList.txt TownPreferences.txt \
	VaultDefinitions.txt WildernessDefinition.txt

EXTRA_DIST = \
  $(angband_files)

SUBDIRS = quests towns

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/edit

angband_DATA = \
  $(angband_files)
endif

