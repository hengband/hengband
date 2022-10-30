## Makefile.am -- Process this file with automake to process Makefile.in

angband_files = \
	001_ThievesHideout.txt 002_TheSewer.txt \
	003_LogrusMaster.txt 004_TheVault.txt \
	005_TheTower_1.txt 006_TheTower_2.txt \
	007_TheTower_3.txt 008_Oberon.txt \
	009_SerpentOfChaos.txt 010_NodeOfSorcery.txt \
	011_NodeOfChaos.txt 012_NodeOfNature.txt \
	014_WargProblem.txt 015_EricStronghold.txt \
	016_AssaultOnMontsalvat.txt 018_WaterCave.txt \
	019_DoomQuest1.txt 020_VaporQuest.txt \
	021_DoomQuest2.txt 022_OrcCamp.txt \
	023_SpawningPits.txt 024_MicroShaft.txt \
	025_HauntedHouse.txt 026_KillingFields.txt \
	027_TheOldCatsle.txt 028_TheRoyalCrypt.txt \
	029_TheMimicTreasure.txt 030_TenguAndDeathSword.txt \
	031_OldManWillow.txt 032_DarkElvenLords.txt \
	033_CloningPits.txt 034_DumpWitness.txt \
	040-049_RandomQuests.txt

EXTRA_DIST = \
	$(angband_files)

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/edit/quests

angband_DATA = \
	$(angband_files)
endif
