## Makefile.am -- Process this file with automake to process Makefile.in

angband_files = \
	ArtifactDefinitions.txt BaseItemDefinitions.txt ClassMagicDefinitions.txt \
	ClassSkillDefinitions.txt DungeonDefinitions.txt EgoDefinitions.txt \
	GridFeatureDefinitions.txt MonsterRaceDefinitions.txt QuestDefinitionList.txt \
	TownDefinitionList.txt WildernessDefinition.txt \
	misc.txt \
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
	t0000001.txt t0000002.txt t0000003.txt t0000004.txt t0000005.txt \
	t0000006.txt t_basic.txt t_lite.txt \
	t_pref.txt v_info.txt \

EXTRA_DIST = \
  $(angband_files)

if SET_GID
angbanddir = @DEFAULT_LIB_PATH@/edit

angband_DATA = \
  $(angband_files)
endif

