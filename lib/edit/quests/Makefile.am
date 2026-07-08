## Makefile.am -- Process this file with automake to process Makefile.in

angband_files = \
	001_ThievesHideout.jsonc 002_TheSewer.jsonc \
	003_LogrusMaster.jsonc 004_TheVault.jsonc \
	005_TheTower_1.jsonc 006_TheTower_2.jsonc \
	007_TheTower_3.jsonc 008_Oberon.jsonc \
	009_SerpentOfChaos.jsonc 010_NodeOfSorcery.jsonc \
	011_NodeOfChaos.jsonc 012_NodeOfNature.jsonc \
	013_NodeOfArcane.jsonc 014_WargProblem.jsonc \
	015_EricStronghold.jsonc 016_AssaultOnMontsalvat.jsonc \
	018_WaterCave.jsonc 019_DoomQuest1.jsonc \
	020_VaporQuest.jsonc 021_DoomQuest2.jsonc \
	022_OrcCamp.jsonc 023_SpawningPits.jsonc \
	024_MicroShaft.jsonc 025_HauntedHouse.jsonc \
	026_KillingFields.jsonc 027_TheOldCatsle.jsonc \
	028_TheRoyalCrypt.jsonc 029_TheMimicTreasure.jsonc \
	030_TenguAndDeathSword.jsonc 031_OldManWillow.jsonc \
	032_DarkElvenLords.jsonc 033_CloningPits.jsonc \
	034_DumpWitness.jsonc 040_RandomQuest.jsonc \
	041_RandomQuest.jsonc 042_RandomQuest.jsonc \
	043_RandomQuest.jsonc 044_RandomQuest.jsonc \
	045_RandomQuest.jsonc 046_RandomQuest.jsonc \
	047_RandomQuest.jsonc 048_RandomQuest.jsonc \
	049_RandomQuest.jsonc

EXTRA_DIST = \
	$(angband_files)

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/edit/quests

angband_DATA = \
	$(angband_files)
endif
