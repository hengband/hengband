## Makefile.am -- Process this file with automake to process Makefile.in

angband_files = \
	ArtifactDefinitions.txt BaseItemDefinitions.txt ClassMagicDefinitions.txt \
	ClassSkillDefinitions.txt DungeonDefinitions.txt EgoDefinitions.txt \
	MonsterRaceDefinitions.txt QuestDefinitionList.txt TerrainDefinitions.txt \
	TownDefinitionList.txt TownPreferences.txt VaultDefinitions.txt \
	WildernessDefinition.txt misc.txt \
	q0000001.txt q0000002.txt q0000003.txt q0000004.txt \
	q0000005.txt q0000006.txt q0000007.txt q0000019.txt \
	q0000020.txt q0000021.txt q0000022.txt q0000023.txt \
	q0000027.txt \
	q0000028.txt q00000ms.txt q000chao.txt q000dekn.txt \
	q000eric.txt q000natu.txt q000sorc.txt \
	q00water.txt q0clone0.txt q0dumpwitness.txt q0fields.txt \
	q0thief1.txt q0thief2.txt q0willow.txt q_oberon.txt \
	q_pref.txt q_random.txt q_serpen.txt q_warg.txt \
	qdarkelf.txt qhaunted.txt \
	towns/01_Outpost_Full.txt towns/01_Outpost_Lite.txt \
	towns/01_Outpost_OnlyAngband.txt towns/02_Telmora.txt \
	towns/03_Morivant.txt towns/04_Angwil.txt towns/05_Zul.txt \
	towns/06_Rlyeh_Obsoleted.txt

EXTRA_DIST = \
  $(angband_files)

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/edit

angband_DATA = \
  $(angband_files)
endif

