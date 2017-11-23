/**************************************************************************
 * #   #   #   ##   #  #  ###   ##   ##  ###       http://www.lyonesse.it *
 * #    # #   #  #  ## #  #    #    #    #                                *
 * #     #    #  #  # ##  ##    #    #   ##   ## ##  #  #  ##             *
 * #     #    #  #  # ##  #      #    #  #    # # #  #  #  # #            *
 * ###   #     ##   #  #  ###  ##   ##   ###  #   #  ####  ##    Ver. 1.0 *
 *                                                                        *
 * -Based on CircleMud & Smaug-     Copyright (c) 2001-2002 by Mithrandir *
 *                                                                        *
 * ********************************************************************** */
/* ************************************************************************
*   File: handler.h                                     Part of CircleMUD *
*  Usage: header file: prototypes of handling and utility functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* handling the affected-structures */
void	affect_total(CHAR_DATA *ch);
void	affect_modify(CHAR_DATA *ch, byte loc, sbyte mod, bitvector_t bitv, bool add);
void	affect_to_char(CHAR_DATA *ch, AFFECTED_DATA *af);
void	affect_remove(CHAR_DATA *ch, AFFECTED_DATA *af);
void	affect_from_char(CHAR_DATA *ch, int type);
bool	affected_by_spell(CHAR_DATA *ch, int type);
void	affect_join(CHAR_DATA *ch, AFFECTED_DATA *af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod);

void	print_room_aff( CHAR_DATA *ch, ROOM_DATA *pRoom );
ROOM_AFFECT *get_room_aff(ROOM_DATA *pRoom, int spl);
ROOM_AFFECT *get_room_aff_bitv(ROOM_DATA *pRoom, int bitv);

/* utility */
int		isname(const char *str, const char *namelist);
char	*fname(const char *namelist);
int		get_number(char **name);

/* ******** objects *********** */

OBJ_DATA *obj_to_char(OBJ_DATA *object, CHAR_DATA *ch);
void	obj_from_char(OBJ_DATA *object);

OBJ_DATA *unequip_char(CHAR_DATA *ch, int pos);
void	equip_char(CHAR_DATA *ch, OBJ_DATA *obj, int pos);
int		invalid_align(CHAR_DATA *ch, OBJ_DATA *obj);
int		invalid_class(CHAR_DATA *ch, OBJ_DATA *obj);
int		invalid_race(CHAR_DATA *ch, OBJ_DATA *obj);

OBJ_DATA *get_obj_in_list(char *name, OBJ_DATA *list);
OBJ_DATA *get_obj_in_list_num(int num, OBJ_DATA *list);
OBJ_DATA *get_obj(char *name);
OBJ_DATA *get_obj_num(obj_rnum nr);

OBJ_DATA *obj_to_room(OBJ_DATA *object, ROOM_DATA *room);
void	obj_from_room(OBJ_DATA *object);
OBJ_DATA *obj_to_obj(OBJ_DATA *obj, OBJ_DATA *obj_to);
void	obj_from_obj(OBJ_DATA *obj);
void	object_list_new_owner(OBJ_DATA *list, CHAR_DATA *ch);

void	extract_obj(OBJ_DATA *obj);

OBJ_DATA *clone_object( OBJ_DATA *obj );
OBJ_DATA *group_object( OBJ_DATA *obj1, OBJ_DATA *obj2 );
void	split_obj( OBJ_DATA *obj, int num );
void	separate_obj( OBJ_DATA *obj );
int		get_real_obj_weight( OBJ_DATA *obj );

/* ******* characters ********* */

CHAR_DATA *get_char_room(char *name, int *num, ROOM_DATA *room);
CHAR_DATA *get_char_num(mob_rnum nr);

void	char_from_room(CHAR_DATA *ch);
void	char_to_room(CHAR_DATA *ch, ROOM_DATA *room);
void	char_from_obj(CHAR_DATA *ch);
void	char_to_obj(CHAR_DATA *ch, OBJ_DATA *furn);

void	char_to_ghost(CHAR_DATA *ch);

void	extract_char(CHAR_DATA *ch);
void	extract_char_final(CHAR_DATA *ch);
void	extract_pending_chars(void);

/* find if character can see */
CHAR_DATA *get_player_vis(CHAR_DATA *ch, char *name, int *number, int inroom);
CHAR_DATA *get_char_vis(CHAR_DATA *ch, char *name, int *number, int where);
CHAR_DATA *get_char_room_vis(CHAR_DATA *ch, char *name, int *number);
CHAR_DATA *get_char_world_vis(CHAR_DATA *ch, char *name, int *number);
CHAR_DATA *get_char_elsewhere_vis(CHAR_DATA *ch, ROOM_DATA *pRoom, char *name, int *numbr);

OBJ_DATA *get_obj_in_list_vis_rev(CHAR_DATA *ch, char *name, int *numbr, OBJ_DATA *list);
OBJ_DATA *get_obj_vis(CHAR_DATA *ch, char *name, int *num);
OBJ_DATA *get_obj_in_equip_vis(CHAR_DATA *ch, char *arg, int *number, OBJ_DATA *equipment[]);
int		get_obj_pos_in_equip_vis(CHAR_DATA *ch, char *arg, int *num, OBJ_DATA *equipment[]);


/* find all dots */

int	find_all_dots(char *arg);

#define FIND_INDIV				0
#define FIND_ALL				1
#define FIND_ALLDOT				2


/* Generic Find */

int	generic_find(char *arg, bitvector_t bitvector, CHAR_DATA *ch, CHAR_DATA **tar_ch, OBJ_DATA **tar_obj);

#define FIND_CHAR_ROOM			(1 << 0)
#define FIND_CHAR_WORLD			(1 << 1)
#define FIND_OBJ_INV			(1 << 2)
#define FIND_OBJ_ROOM			(1 << 3)
#define FIND_OBJ_WORLD			(1 << 4)
#define FIND_OBJ_EQUIP			(1 << 5)
#define FIND_BLD_ROOM			(1 << 6)

/* prototypes from fight.c */
void	set_fighting(CHAR_DATA *ch, CHAR_DATA *victim);
void	stop_fighting(CHAR_DATA *ch);
void	stop_follower(CHAR_DATA *ch);
void	hit(CHAR_DATA *ch, CHAR_DATA *victim, int type);
void	forget(CHAR_DATA *ch, CHAR_DATA *victim);
void	remember(CHAR_DATA *ch, CHAR_DATA *victim);
int		damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int attacktype);
int		skill_message(int dam, CHAR_DATA *ch, CHAR_DATA *vict, int attacktype);
