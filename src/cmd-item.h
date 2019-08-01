/* cmd3-item.h */

#define REF_ITEM(P_PTR, FLOOR_PTR, ID) ((ID > 0 ? &(P_PTR)->inventory_list[ID] : &(FLOOR_PTR)->o_list[0 - item]))

extern void do_cmd_inven(void);
extern void do_cmd_equip(void);
extern void do_cmd_wield(void);
extern void do_cmd_takeoff(void);
extern void do_cmd_drop(void);
extern void do_cmd_destroy(void);
extern void do_cmd_observe(void);
extern void do_cmd_uninscribe(void);
extern void do_cmd_inscribe(void);
extern void do_cmd_refill(void);
extern void do_cmd_target(void);
extern void do_cmd_look(void);
extern void do_cmd_locate(void);
extern void do_cmd_query_symbol(void);
extern void do_cmd_use(void);