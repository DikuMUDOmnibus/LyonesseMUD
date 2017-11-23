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
*   File: utils.h                                       Part of CircleMUD *
*  Usage: header file: utility macros and prototypes of utility funcs     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


/* external declarations and prototypes **********************************/

extern FILE *logfile;

#define log			basic_mud_log

/* public functions in utils.c */
void	prune_crlf(char *txt);
void	strip_cr(char *buffer);
int		sprintascii(char *out, bitvector_t bits);
char	*str_dup(const char *source);
int		str_cmp(const char *arg1, const char *arg2);
int		strn_cmp(const char *arg1, const char *arg2, int n);
void	basic_mud_log(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
int		touch(const char *path);
void	mudlog(const char *str, int type, int level, int file);
void	log_death_trap(CHAR_DATA *ch);
int		number(int from, int to);
int		dice(int number, int size);
bool	roll(int val);
int		percentage(int amount, int max);
void	sprintbit(bitvector_t vektor, const char *names[], char *result);
void	sprinttype(int type, const char *names[], char *result);
int		get_line(FILE *fl, char *buf);
int		get_filename(char *orig_name, char *filename, int mode);
time_t	days_passed(time_t last_date);
time_t	mud_time_to_secs(TIME_INFO_DATA *now);
TIME_INFO_DATA *age(CHAR_DATA *ch);
int		num_pc_in_room(ROOM_DATA *room);
int		item_count(CHAR_DATA *ch);
int		room_is_dark(ROOM_DATA *room);

void	core_dump_real(const char *, int);

EXIT_DATA *find_exit( ROOM_DATA *room, sh_int dir );
EXIT_DATA *get_exit( ROOM_DATA *room, sh_int dir );
ROOM_DATA *find_room( ROOM_DATA *room, sh_int dir );
EXIT_DATA *get_exit_to( ROOM_DATA *room, sh_int dir, int vnum );
EXIT_DATA *get_exit_num( ROOM_DATA *room, sh_int count );
EXIT_DATA *get_exit_to_coord( ROOM_DATA *room, sh_int dir, COORD_DATA *coord );
EXIT_DATA *make_exit( ROOM_DATA *pRoom, ROOM_DATA *to_room, sh_int door );
bool	valid_exit( EXIT_DATA *pexit );
void	extract_exit( ROOM_DATA *room, EXIT_DATA *pexit );


#define core_dump()		core_dump_real(__FILE__, __LINE__)

/* random functions in random.c */
void circle_srandom(unsigned long initial_seed);
unsigned long circle_random(void);

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);
char *CAP(char *txt);

/* Followers */
int		num_followers_charmed(CHAR_DATA *ch);
void	die_follower(CHAR_DATA *ch);
void	add_follower(CHAR_DATA *ch, CHAR_DATA *leader);
void	stop_follower(CHAR_DATA *ch);
bool	circle_follow(CHAR_DATA *ch, CHAR_DATA *victim);

/* in spell_parser.c */
bool	success(CHAR_DATA *ch, CHAR_DATA *vict, int skill, int val);

/* in act.informative.c */
void	look_at_room(CHAR_DATA *ch, int mode);

/* in act.movmement.c */
EXIT_DATA *find_door(CHAR_DATA *ch, const char *type, char *dir, const char *cmdname);
bool	water_sector(int sect);
int		do_simple_move(CHAR_DATA *ch, int dir, int following);
int		perform_move(CHAR_DATA *ch, int dir, int following);

/* in limits.c */
int		mana_gain(CHAR_DATA *ch);
int		hit_gain(CHAR_DATA *ch);
int		move_gain(CHAR_DATA *ch);
void	advance_level(CHAR_DATA *ch);
void	set_title(CHAR_DATA *ch, char *title);
void	gain_exp(CHAR_DATA *ch, int gain);
void	gain_exp_regardless(CHAR_DATA *ch, int gain);
void	gain_condition(CHAR_DATA *ch, int condition, int value);
void	check_idling(CHAR_DATA *ch);
void	point_update(void);
void	update_pos(CHAR_DATA *victim);

/* in fight.c */
bool	pkill_ok(CHAR_DATA *ch, CHAR_DATA *opponent);

/* interpreter.c */
void    line_input(DESCRIPTOR_DATA *d, const char *prompt, C_FUNC(*callback), void *info);

/* objects.c */
bool	sub_gold( CHAR_DATA *ch, int amount );
char	*numberize( int n );
int		get_gold( CHAR_DATA *ch );
int		find_objcond_index(OBJ_DATA *obj);
void	check_damage_obj(CHAR_DATA *ch, OBJ_DATA *obj, int chance);
void	add_gold( CHAR_DATA *ch, int amount );
void	update_money( OBJ_DATA *obj );
void	create_amount(int amount, CHAR_DATA *pMob, ROOM_DATA *pRoom, OBJ_DATA *pObj, VEHICLE_DATA *pVeh, bool SkipGroup);

/* in wilderness.c */
ROOM_DATA *find_wild_room( ROOM_DATA *wRoom, int dir, bool makenew );
ROOM_DATA *get_wild_room( COORD_DATA *coord );
bool	check_coord(int y, int x);
bool	invalid_sect( COORD_DATA *coord );

/* in act.ohter.c */
bool	is_char_known(CHAR_DATA *ch, CHAR_DATA *victim);
char	*unknown_name(CHAR_DATA *ch);
char	*PERS(CHAR_DATA *target, CHAR_DATA *viewer);
char	*take_name( CHAR_DATA *ch, CHAR_DATA *viewer);
void	know_add_load( CHAR_DATA *ch, long idnum, char *chname );

/* events.c */
EVENT_DATA *event_create(EVENTFUNC(*func), void *event_obj, long when);
void	event_cancel(EVENT_DATA *event);

/* in obj_trap.c */
bool	check_trap(CHAR_DATA *ch, OBJ_DATA *obj, int action);

/* vehicles.c */
VEHICLE_DATA *find_vehicle(char *name);
VEHICLE_DATA *find_vehicle_in_room_by_name(CHAR_DATA *ch, char *name);
OBJ_DATA *obj_to_vehicle(OBJ_DATA *obj, VEHICLE_DATA *vehicle);
bool	vehicle_can_move(VEHICLE_DATA *vehicle);
bool	vehicle_can_go(VEHICLE_DATA *pVeh, ROOM_DATA *to_room);
bool	hitched_mob(CHAR_DATA *ch);
void	list_vehicle_to_char(VEHICLE_DATA *vlist, CHAR_DATA *ch);
void	vehicle_to_room(VEHICLE_DATA *pVeh, ROOM_DATA *pRoom);
void	vehicle_from_room(VEHICLE_DATA *pVeh);
void	obj_from_vehicle(OBJ_DATA *obj);
void	char_to_vehicle(CHAR_DATA *ch, VEHICLE_DATA *vehicle);
void	char_from_vehicle(CHAR_DATA *ch);
void	stop_be_wagoner(CHAR_DATA *ch);

/* room_trigger.c */
bool	check_room_trigger(CHAR_DATA *ch, int trigger);

/* wild.ships.c */
FERRY_DATA *get_ferry(int vnum);

/* parser.c */
void master_parser(char *buf, CHAR_DATA *ch, ROOM_DATA *room, OBJ_DATA *obj);

/* wild.info.c */
double distance(int chX, int chY, int lmX, int lmY);

/* various constants *****************************************************/

/* defines for mudlog() */
#define OFF						0
#define BRF						1
#define NRM						2
#define CMP						3

/* get_filename() */
#define OBJS_FILE				0
#define ETEXT_FILE				1
#define ALIAS_FILE				2
#define PLR_FILE				3


/* breadth-first searching */
#define BFS_ERROR				-1
#define BFS_ALREADY_THERE		-2
#define BFS_NO_PATH				-3

/*
 * XXX: These constants should be configurable. See act.informative.c
 *	and utils.c for other places to change.
 */
/* mud-life time */
#define SECS_PER_MUD_HOUR		75
#define SECS_PER_MUD_DAY		(24 * SECS_PER_MUD_HOUR)
#define SECS_PER_MUD_MONTH		(35 * SECS_PER_MUD_DAY)
#define SECS_PER_MUD_YEAR		(17 * SECS_PER_MUD_MONTH)

#define MUD_DAYS_PER_YEAR		595

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN		60
#define SECS_PER_REAL_HOUR		(60 * SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY		(24 * SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR		(365 * SECS_PER_REAL_DAY)


/* string utils **********************************************************/


#define YESNO(a)				((a) ? "YES" : "NO")
#define ONOFF(a)				((a) ? "ON" : "OFF")

#define LOWER(c)				(((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
#define UPPER(c)				(((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )

#define ISNEWL(ch)				((ch) == '\n' || (ch) == '\r') 

/* See also: ANA, SANA */
#define AN(string)				(strchr("aeiouAEIOU", *string) ? "an" : "a")
#define UAN(string)				(strchr("aeiouAEIOU", *string) ? "An" : "A")

#define URANGE(a, b, c)			((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))

/* memory utils **********************************************************/


#define CREATE(result, type, number)  do {\
	if ((number) * sizeof(type) <= 0)	\
		log("SYSERR: Zero bytes or less requested at %s:%d.", __FILE__, __LINE__);	\
	if (!((result) = (type *) calloc ((number), sizeof(type))))	\
		{ perror("SYSERR: malloc failure"); abort(); } } while(0)

#define RECREATE(result,type,number) do {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("SYSERR: realloc failure"); abort(); } } while(0)

/*
 * the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }					\


/* basic bitvector utils *************************************************/


#define IS_SET(flag,bit)		((flag) & (bit))
#define SET_BIT(var,bit)		((var) |= (bit))
#define REMOVE_BIT(var,bit)		((var) &= ~(bit))
#define TOGGLE_BIT(var,bit)		((var) = (var) ^ (bit))

/*
 * Accessing player specific data structures on a mobile is a very bad thing
 * to do.  Consider that changing these variables for a single mob will change
 * it for every other single mob in the game.  If we didn't specifically check
 * for it, 'wimpy' would be an extremely bad thing for a mob to do, as an
 * example.  If you really couldn't care less, change this to a '#if 0'.
 */
#if 1
/* Subtle bug in the '#var', but works well for now. */
#define CHECK_PLAYER_SPECIAL(ch, var) \
	(*(((ch)->player_specials == &dummy_mob) ? (log("SYSERR: Mob using '"#var"' at %s:%d.", __FILE__, __LINE__), &(var)) : &(var)))
#else
#define CHECK_PLAYER_SPECIAL(ch, var)	(var)
#endif

#define MOB_FLAGS(ch)				((ch)->player.act)
#define PLR_FLAGS(ch)				((ch)->player.act)
#define PRF_FLAGS(ch)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->pref))
#define AFF_FLAGS(ch)				((ch)->player.affected_by)
#define ROOM_FLAGS(loc)				((loc)->room_flags)
#define ROOM_AFFS(loc)				((loc)->room_affs)
#define EXIT_FLAGS(loc)				((loc)->exit_info)

#define SPELL_ROUTINES(spl)			(spell_info[spl].routines)

/* See http://www.circlemud.org/~greerga/todo/todo.009 to eliminate MOB_ISNPC. */
#define IS_NPC(ch)					(IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)					(IS_NPC(ch) && GET_MOB_RNUM(ch) >= 0)

#define MOB_FLAGGED(ch, flag)		(IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag)		(!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag)		(IS_SET(AFF_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag)		(IS_SET(PRF_FLAGS(ch), (flag)))
#define ROOM_FLAGGED(loc, flag)		(IS_SET(ROOM_FLAGS(loc), (flag)))
#define ROOM_AFFECTED(loc, flag)	(IS_SET(ROOM_AFFS(loc), (flag)))
#define EXIT_FLAGGED(exit, flag)	(IS_SET(EXIT_FLAGS(exit), (flag)))
#define OBJVAL_FLAGGED(obj, flag)	(IS_SET(GET_OBJ_VAL((obj), 1), (flag)))
#define OBJWEAR_FLAGGED(obj, flag)	(IS_SET(GET_OBJ_WEAR(obj), (flag)))
#define OBJ_FLAGGED(obj, flag)		(IS_SET(GET_OBJ_EXTRA(obj), (flag)))
#define HAS_SPELL_ROUTINE(spl,flag)	(IS_SET(SPELL_ROUTINES(spl), (flag)))

/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill)		(AFF_FLAGGED((ch), (skill)))

#define PLR_TOG_CHK(ch,flag)		((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag)		((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

#define MULTICLASS(ch)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->multiclass))
#define MULTICLASSED(ch, flag)		(IS_SET(MULTICLASS(ch), (flag)))

#define BUILDING_FLAGS(bld)			((bld)->flags)
#define BUILDING_FLAGGED(bld, flag)	(IS_SET(BUILDING_FLAGS(bld), (flag)))

#define SHIP_FLAGS(ship)			((ship)->flags)
#define SHIP_FLAGGED(ship, flag)	(IS_SET(SHIP_FLAGS(ship), (flag)))

/* room utils ************************************************************/

#define SECT(room)					((room)->sector_type)

#define IS_BUILDING(sub)			((sub)->zone == BUILDING_ZONE)
#define IN_BUILDING(sub)			(IS_BUILDING((sub)->in_room))

#define IS_WILD(sub)				((sub)->zone == WILD_ZONE)
#define IN_WILD(sub)				(IS_WILD((sub)->in_room ))

#define IS_SHIP(sub)				((sub)->zone == SHIP_ZONE)
#define IN_SHIP(sub)				(IS_SHIP((sub)->in_room))
#define IS_DECK(sub)				(SECT((sub)) >= SECT_SHIP_STERN && SECT((sub)) <= SECT_SHIP_BOW)
#define ON_DECK(sub)				(IS_DECK((sub)->in_room))

#define IS_FERRY(sub)				((sub)->zone == FERRY_ZONE)
#define IN_FERRY(sub)				(IS_FERRY((sub)->in_room))

#define S_NAME(sn)					(terrain_type[(sn)] != NULL ? terrain_type[(sn)]->name : "WildRoomName")
#define S_DESC(sn)					(terrain_type[(sn)] != NULL ? terrain_type[(sn)]->description : "WildRoomDesc")

#define GET_RY(sub)					((sub)->coord->y)
#define GET_RX(sub)					((sub)->coord->x)
#define GET_RZ(sub)					((sub)->coord->level)
#define GET_Y(sub)					(GET_RY((sub)->in_room))
#define GET_X(sub)					(GET_RX((sub)->in_room))
#define GET_Z(sub)					(GET_RZ((sub)->in_room))

#define ROOM_RNAME(sub)				(IS_WILD(sub) ? S_NAME( (sub)->sector_type ) : \
										((sub)->name ? (sub)->name : "NoName"))
#define ROOM_RDESC(sub)				(IS_WILD(sub) ? S_DESC( (sub)->sector_type ) : \
										((sub)->description ? (sub)->description : \
											"NoDescription"))
#define ROOM_NAME(sub)				(ROOM_RNAME((sub)->in_room))
#define ROOM_DESC(sub)				(ROOM_RDESC((sub)->in_room))

#define IS_DARK(room)				room_is_dark((room))
#define IS_LIGHT(room)				(!IS_DARK(room))

#define GET_ROOM_SPEC(room)			((room)->func)

/* char utils ************************************************************/


#define IN_ROOM(ch)					((ch)->in_room)
#define GET_WAS_IN(ch)				((ch)->was_in_room)
#define GET_AGE(ch)					(age(ch)->year)
#define GET_PFILEPOS(ch)			((ch)->pfilepos)
#define GET_EQ(ch, i)				((ch)->equipment[i])

#define GET_PC_NAME(ch)				((ch)->player.name)
#define GET_NAME(ch)				(IS_NPC(ch) ? (ch)->player.short_descr : GET_PC_NAME(ch))
#define GET_TITLE(ch)				((ch)->player.title)
#define GET_LEVEL(ch)				((ch)->player.level)
#define GET_PASSWD(ch)				((ch)->player.passwd)
#define GET_CLASS(ch)				((ch)->player.chclass)
#define GET_RACE(ch)				((ch)->player.race)
#define GET_HOME(ch)				((ch)->player.hometown)
#define GET_HEIGHT(ch)				((ch)->player.height)
#define GET_WEIGHT(ch)				((ch)->player.weight)
#define GET_SEX(ch)					((ch)->player.sex)
#define GET_POS(ch)					((ch)->player.position)
#define GET_IDNUM(ch)				((ch)->player.idnum)
#define IS_CARRYING_W(ch)			((ch)->player.carry_weight)
#define IS_CARRYING_N(ch)			((ch)->player.carry_items)
#define FIGHTING(ch)				((ch)->player.fighting)
#define RIDING(ch)					((ch)->player.riding)
#define HUNTING(ch)					((ch)->player.hunting)
#define WAGONER(ch)					((ch)->player.drive_vehicle)
#define GET_SAVE(ch, i)				((ch)->player.apply_saving_throw[i])
#define GET_ALIGNMENT(ch)			((ch)->player.alignment)

#define GET_CLAN(ch)				((ch)->player.clan)
#define GET_CLAN_RANK(ch)			((ch)->player.clan_rank)

#define GET_SDESC(ch)				((ch)->player.short_descr)
#define GET_LDESC(ch)				((ch)->player.long_descr)
#define GET_DDESC(ch)				((ch)->player.description)

#define GET_STR(ch)					((ch)->aff_abils.str)
#define GET_DEX(ch)					((ch)->aff_abils.dex)
#define GET_INT(ch)					((ch)->aff_abils.intel)
#define GET_WIS(ch)					((ch)->aff_abils.wis)
#define GET_CON(ch)					((ch)->aff_abils.con)
#define GET_CHA(ch)					((ch)->aff_abils.cha)

#define GET_EXP(ch)					((ch)->points.exp)
#define GET_AC(ch)					((ch)->points.armor)
#define GET_HIT(ch)					((ch)->points.hit)
#define GET_MAX_HIT(ch)				((ch)->points.max_hit)
#define GET_MOVE(ch)				((ch)->points.move)
#define GET_MAX_MOVE(ch)			((ch)->points.max_move)
#define GET_MANA(ch)				((ch)->points.mana)
#define GET_MAX_MANA(ch)			((ch)->points.max_mana)
#define GET_BANK_GOLD(ch)			((ch)->points.bank_gold)
#define GET_HITROLL(ch)				((ch)->points.hitroll)
#define GET_DAMROLL(ch)				((ch)->points.damroll)

#define GET_REAL_HITROLL(ch)		(str_app[GET_STR(ch)].tohit + GET_HITROLL(ch))
#define GET_REAL_DAMROLL(ch)		(str_app[GET_STR(ch)].todam + GET_DAMROLL(ch))

#define GET_TOT_LEVEL(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->tot_level))
#define GET_COND(ch, i)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->conditions[(i)]))
#define GET_LOADROOM(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->load_room))
#define GET_LOADBUILDING(ch)		CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->load_building))
#define GET_LOADCOORD(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->load_coord))
#define GET_LOAD_Y(ch)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->load_coord->y))
#define GET_LOAD_X(ch)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->load_coord->x))
#define GET_LOADSHIP(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->load_ship))
#define GET_PRACTICES(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->spells_to_learn))
#define GET_INVIS_LEV(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->invis_level))
#define GET_WIMP_LEV(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->wimp_level))
#define GET_FREEZE_LEV(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->freeze_level))
#define GET_BAD_PWS(ch)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->bad_pws))
#define GET_TALK(ch, i)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->talks[i]))
#define POOFIN(ch)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofin))
#define POOFOUT(ch)					CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->poofout))
#define GET_ALIASES(ch)				CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->aliases))
#define GET_LAST_TELL(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->last_tell))
#define GET_MOB_KILLS(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->mob_kills))
#define GET_MOB_DEATHS(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->mob_deaths))
#define GET_PLR_KILLS(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->plr_kills))
#define GET_PLR_DEATHS(ch)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->plr_deaths))

#define MEMORIZED(ch, sn)			((ch)->player_specials->spells_mem[sn])
#define GET_SKILL(ch, i)			CHECK_PLAYER_SPECIAL((ch), ((ch)->player_specials->skills[i]))
#define SET_SKILL(ch, i, pct)		do { CHECK_PLAYER_SPECIAL((ch), (ch)->player_specials->skills[i]) = pct; } while(0)

/* Killing same mobs XP penalties macros */
#define GET_KILLS_VNUM(ch, a)		((ch)->player_specials->kills_vnum[a-1])
#define GET_KILLS_AMOUNT(ch, a)		((ch)->player_specials->kills_amount[a-1])
#define GET_KILLS_CURPOS(ch)		((ch)->player_specials->kills_curpos)

#define GET_MOB_MAXFACTOR(ch)		((ch)->mob_specials.maxfactor)
#define GET_MOB_SPEC(mob)			(IS_MOB(mob) ? mob_index[(mob)->nr].func : NULL)
#define GET_MOB_RNUM(mob)			((mob)->nr)
#define GET_MOB_VNUM(mob)			(IS_MOB(mob) ? mob_index[GET_MOB_RNUM(mob)].vnum : NOBODY)

#define GET_DEFAULT_POS(mob)		((mob)->mob_specials.default_pos)
#define MEMORY(mob)					((mob)->mob_specials.memory)
#define GET_GOLD(mob)				((mob)->mob_specials.gold)
#define RIDDEN_BY(mob)				((mob)->mob_specials.ridden_by)
#define MOB_OWNER(mob)				((mob)->mob_specials.owner_id)

/* Handy macros. */
#define GET_NDD(mob)				((mob)->mob_specials.damnodice)
#define GET_SDD(mob)				((mob)->mob_specials.damsizedice)
#define GET_ATTACK(mob)				((mob)->mob_specials.attack_type)

/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))


#define CAN_CARRY_W(ch)				(str_app[GET_STR(ch)].carry_w)
#define CAN_CARRY_N(ch)				(5 + (GET_DEX(ch) >> 1) + (GET_LEVEL(ch) >> 1))
#define AWAKE(ch)					(GET_POS(ch) > POS_SLEEPING)
#define CAN_SEE_IN_DARK(ch) \
   (AFF_FLAGGED(ch, AFF_INFRAVISION) || (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT)))

#define IS_GOOD(ch)					(GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)					(GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch)				(!IS_GOOD(ch) && !IS_EVIL(ch))


/* These three deprecated. */
#define WAIT_STATE(ch, cycle)		do { GET_WAIT_STATE(ch) = (cycle); } while(0)
#define CHECK_WAIT(ch)				((ch)->wait > 0)
#define GET_MOB_WAIT(ch)			GET_WAIT_STATE(ch)
/* New, preferred macro. */
#define GET_WAIT_STATE(ch)			((ch)->wait)


/* descriptor-based utils ************************************************/

/* Hrm, not many.  We should make more. -gg 3/4/99 */
#define STATE(d)					((d)->connected)


/* object utils **********************************************************/


#define GET_OBJ_LEVEL(obj)			((obj)->obj_flags.level)
#define GET_OBJ_PERM(obj)			((obj)->obj_flags.bitvector)
#define GET_OBJ_TYPE(obj)			((obj)->obj_flags.type_flag)
#define GET_OBJ_COST(obj)			((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj)			((obj)->obj_flags.cost_per_day)
#define GET_OBJ_EXTRA(obj)			((obj)->obj_flags.extra_flags)
#define GET_OBJ_ANTI(obj)			((obj)->obj_flags.anti_flags)
#define GET_OBJ_WEAR(obj)			((obj)->obj_flags.wear_flags)
#define GET_OBJ_VAL(obj, val)		((obj)->obj_flags.value[(val)])
#define GET_OBJ_WEIGHT(obj)			((obj)->obj_flags.weight)
#define GET_OBJ_TIMER(obj)			((obj)->obj_flags.timer)
#define GET_OBJ_QUALITY(obj)		((obj)->obj_flags.quality)
#define GET_OBJ_COND(obj)			((obj)->obj_flags.cond)
#define GET_OBJ_MAXCOND(obj)		((obj)->obj_flags.maxcond)
#define GET_OBJ_OWNER(obj)			((obj)->obj_flags.owner_id)

#define GET_OBJ_SKILL_MIN(obj)		((obj)->obj_flags.skill_min)
#define GET_OBJ_SKILL_BONUS(obj)	((obj)->obj_flags.skill_bonus)

#define GET_OBJ_RNUM(obj)			((obj)->item_number)
#define GET_OBJ_VNUM(obj)			(GET_OBJ_RNUM(obj) >= 0 ? obj_index[GET_OBJ_RNUM(obj)].vnum : NOTHING)
#define GET_OBJ_SPEC(obj)			((obj)->item_number >= 0 ? (obj_index[(obj)->item_number].func) : NULL)

#define IS_CORPSE(obj)				(GET_OBJ_TYPE(obj) == ITEM_CONTAINER && GET_OBJ_VAL((obj), 3) == 1)

#define CAN_WEAR(obj, part)			(OBJWEAR_FLAGGED((obj), part))

/* compound utilities and other macros **********************************/

/*
 * Used to compute CircleMUD version. To see if the code running is newer
 * than 3.0pl13, you would use: #if _CIRCLEMUD > CIRCLEMUD_VERSION(3,0,13)
 */
#define CIRCLEMUD_VERSION(major, minor, patchlevel) \
	(((major) << 16) + ((minor) << 8) + (patchlevel))

#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

#define ANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouAEIOU", *(obj)->name) ? "an" : "a")


/* Various macros building up to CAN_SEE */

#define LIGHT_OK(sub)	(!AFF_FLAGGED(sub, AFF_BLIND) && \
   (IS_LIGHT((sub)->in_room) || AFF_FLAGGED((sub), AFF_INFRAVISION)))

#define INVIS_OK(sub, obj) \
 ((!AFF_FLAGGED((obj),AFF_INVISIBLE) || AFF_FLAGGED(sub,AFF_DETECT_INVIS)) && \
 (!AFF_FLAGGED((obj), AFF_HIDE) || AFF_FLAGGED(sub, AFF_SENSE_LIFE)))

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED(sub, PRF_HOLYLIGHT)))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
#define CAN_SEE(sub, obj) (SELF(sub, obj) || \
   ((GET_REAL_LEVEL(sub) >= (IS_NPC(obj) ? 0 : GET_INVIS_LEV(obj))) && \
   IMM_CAN_SEE(sub, obj)))

/* End of CAN_SEE */


#define INVIS_OK_OBJ(sub, obj) \
  (!OBJ_FLAGGED((obj), ITEM_INVISIBLE) || AFF_FLAGGED((sub), AFF_DETECT_INVIS))

/* Is anyone carrying this object and if so, are they visible? */
#define CAN_SEE_OBJ_CARRIER(sub, obj) \
  ((!obj->carried_by || CAN_SEE(sub, obj->carried_by)) &&	\
   (!obj->worn_by || CAN_SEE(sub, obj->worn_by)))

#define MORT_CAN_SEE_OBJ(sub, obj) \
  (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj) && CAN_SEE_OBJ_CARRIER(sub, obj))

#define CAN_SEE_OBJ(sub, obj) \
   (MORT_CAN_SEE_OBJ(sub, obj) || (!IS_NPC(sub) && PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEAR((obj), ITEM_WEAR_TAKE) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	(obj)->short_description  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	fname((obj)->name) : "something")


#define CLASS_ABBR(ch)				(IS_NPC(ch) ? "--" : class_abbrevs[(int)GET_CLASS(ch)])
#define RACE_ABBR(ch)				(IS_NPC(ch) ? "---" : race_abbrevs[(int)GET_RACE(ch)])

#define IS_MAGIC_USER(ch)			(!IS_NPC(ch) && (GET_CLASS(ch) == CLASS_MAGIC_USER))
#define IS_CLERIC(ch)				(!IS_NPC(ch) && (GET_CLASS(ch) == CLASS_CLERIC))
#define IS_THIEF(ch)				(!IS_NPC(ch) && (GET_CLASS(ch) == CLASS_THIEF))
#define IS_WARRIOR(ch)				(!IS_NPC(ch) && (GET_CLASS(ch) == CLASS_WARRIOR))
#define IS_SORCERER(ch)				(!IS_NPC(ch) && (GET_CLASS(ch) == CLASS_SORCERER))

#define CASTING_CLASS(ch)			( IS_MAGIC_USER(ch) || IS_CLERIC(ch) ) 
#define MEMMING_CLASS(ch)			( IS_SORCERER(ch) )

#define IS_HUMAN(ch)				(!IS_NPC(ch) && (GET_RACE(ch) == RACE_HUMAN))
#define IS_ELF(ch)					(!IS_NPC(ch) && (GET_RACE(ch) == RACE_ELF))
#define IS_DWARF(ch)				(!IS_NPC(ch) && (GET_RACE(ch) == RACE_DWARF))

#define OUTSIDE(ch)					(!ROOM_FLAGGED((ch)->in_room, ROOM_INDOORS))

#define IS_GOD(ch)					(!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_GOD))
#define IS_IMMORTAL(ch)				(!IS_NPC(ch) && (GET_LEVEL(ch) >= LVL_IMMORT))
#define IS_MORTAL(ch)				(GET_LEVEL(ch) < LVL_IMMORT)

#define IS_WEAPON(type)				(((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* OS compatibility ******************************************************/


/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/*
 * NOCRYPT can be defined by an implementor manually in sysdep.h.
 * CIRCLE_CRYPT is a variable that the 'configure' script
 * automatically sets when it determines whether or not the system is
 * capable of encrypting.
 */
#if defined(NOCRYPT) || !defined(CIRCLE_CRYPT)
#define CRYPT(a,b) (a)
#else
#define CRYPT(a,b) ((char *) crypt((a),(b)))
#endif

/* ========================== S M A U G ============================ */
#define QUICKLINK(point)	str_dup((point))
#define QUICKMATCH(p1, p2)	((p1) && (p2) && (strcmp((p1), (p2)) == 0))

#define DISPOSE(point) 											\
do																\
{																\
  if (!(point))													\
  {																\
	log( "Freeing null pointer %s:%d", __FILE__, __LINE__ );	\
	fprintf( stderr, "DISPOSEing NULL in %s, line %d\n", __FILE__, __LINE__ ); \
  }																\
  else free(point);												\
  point = NULL;													\
} while(0)

#define STRFREE(point)											\
do																\
{																\
  if (!(point))													\
  {																\
	log( "Freeing null pointer %s:%d", __FILE__, __LINE__ );	\
	fprintf( stderr, "STRFREEing NULL in %s, line %d\n", __FILE__, __LINE__ ); \
  }																\
  else free((point));											\
  point = NULL;													\
} while(0)

/* double-linked list handling macros -Thoric */

#define LINK(link, first, last, next, prev)						\
do																\
{																\
    if ( !(first) )												\
      (first)					= (link);						\
    else														\
      (last)->next				= (link);						\
    (link)->next				= NULL;							\
    (link)->prev				= (last);						\
    (last)						= (link);						\
} while(0)

#define INSERT(link, insert, first, next, prev)					\
do																\
{																\
    (link)->prev				= (insert)->prev;				\
    if ( !(insert)->prev )										\
      (first)					= (link);						\
    else														\
      (insert)->prev->next		= (link);						\
    (insert)->prev				= (link);						\
    (link)->next				= (insert);						\
} while(0)

#define UNLINK(link, first, last, next, prev)					\
do																\
{																\
    if ( !(link)->prev )										\
      (first)					= (link)->next;					\
    else														\
      (link)->prev->next		= (link)->next;					\
    if ( !(link)->next )										\
      (last)					= (link)->prev;					\
    else														\
      (link)->next->prev		= (link)->prev;					\
} while(0)
