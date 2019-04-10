
/*
 * Flags for change floor mode
 */
#define CFM_UP        	 0x0001  /* Move up */
#define CFM_DOWN      	 0x0002  /* Move down */
#define CFM_LONG_STAIRS  0x0004  /* Randomly occurred long stairs/shaft */
#define CFM_XXX  	 0x0008  /* XXX */
#define CFM_SHAFT     	 0x0010  /* Shaft */
#define CFM_RAND_PLACE   0x0020  /* Arrive at random grid */
#define CFM_RAND_CONNECT 0x0040  /* Connect with random stairs */
#define CFM_SAVE_FLOORS  0x0080  /* Save floors */
#define CFM_NO_RETURN    0x0100  /* Flee from random quest etc... */
#define CFM_FIRST_FLOOR  0x0200  /* Create exit from the dungeon */

 /*
  * Determines if a map location is fully inside the outer walls
  */
#define in_bounds(Y,X) \
   (((Y) > 0) && ((X) > 0) && ((Y) < current_floor_ptr->height-1) && ((X) < current_floor_ptr->width-1))

  /*
   * Determines if a map location is on or inside the outer walls
   */
#define in_bounds2(Y,X) \
   (((Y) >= 0) && ((X) >= 0) && ((Y) < current_floor_ptr->height) && ((X) < current_floor_ptr->width))

   /*
	* Determines if a map location is on or inside the outer walls
	* (unsigned version)
	*/
#define in_bounds2u(Y,X) \
   (((Y) < current_floor_ptr->height) && ((X) < current_floor_ptr->width))

