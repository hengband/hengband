
/*! デバッグ時にnestのモンスター情報を確認するための構造体 / A struct for nest monster information with cheat_hear */
typedef struct
{
	MONRACE_IDX r_idx;
	bool used;
}
nest_mon_info_type;

extern bool build_type5(void);
extern bool build_type6(void);
extern bool build_type13(void);
