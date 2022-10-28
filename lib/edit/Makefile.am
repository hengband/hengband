## Makefile.am -- Process this file with automake to process Makefile.in

AUTOMAKE_OPTIONS = foreign

angband_files = \
	ArtifactDefinitions.txt BaseItemDefinitions.txt ClassMagicDefinitions.txt \
	ClassSkillDefinitions.txt DungeonDefinitions.txt EgoDefinitions.txt \
	MonsterRaceDefinitions.txt QuestDefinitionList.txt TerrainDefinitions.txt \
	TownDefinitionList.txt TownPreferences.txt VaultDefinitions.txt \
	WildernessDefinition.txt misc.txt \
	q0clone0.txt q0dumpwitness.txt \
	q0thief1.txt q0thief2.txt q0willow.txt \
	q_pref.txt q_random.txt \
	qdarkelf.txt \
	quests/001_ThievesHideout.txt quests/002_TheSewer.txt \
	quests/003_LogrusMaster.txt quests/004_TheVault.txt \
	quests/005_TheTower_1.txt quests/006_TheTower_2.txt \
	quests/007_TheTower_3.txt quests/008_Oberon.txt \
	quests/009_SerpentOfChaos.txt quests/010_NodeOfSorcery.txt \
	quests/011_NodeOfChaos.txt quests/012_NodeOfNature.txt \
	quests/014_WargProblem.txt quests/015_EricStronghold.txt \
	quests/016_AssaultOnMontsalvat.txt quests/018_WaterCave.txt \
	quests/019_DoomQuest1.txt quests/020_VaporQuest.txt \
	quests/021_DoomQuest2.txt quests/022_OrcCamp.txt \
	quests/023_SpawningPits.txt quests/024_MicroShaft.txt \
	quests/025_HauntedHouse.txt quests/026_KillingFields.txt \
	quests/027_TheOldCatsle.txt quests/028_TheRoyalCrypt.txt

EXTRA_DIST = \
  $(angband_files)

SUBDIRS = towns

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/edit

angband_DATA = \
  $(angband_files)
endif

