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
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"

/* extern variables */
extern HELP_INDEX_ELEM	*help_table;
extern TIME_INFO_DATA	time_info;
extern room_vnum		donation_room_1;
extern int				top_of_helpt;
extern char				*help;
extern char				*credits;
extern char				*news;
extern char				*info;
extern char				*motd;
extern char				*imotd;
extern char				*wizlist;
extern char				*immlist;
extern char				*policies;
extern char				*handbook;
extern char				*class_abbrevs[];
extern const char		*pc_class_types[];
extern const char		*pc_race_types[];

/* extern functions */
ACMD(do_action);
ACMD(do_insult);
TIME_INFO_DATA *real_time_passed(time_t t2, time_t t1);
BUILDING_DATA *find_building_in_room_by_name(ROOM_DATA *pRoom, char *arg);
SHIP_DATA *find_ship_in_room_by_name(ROOM_DATA *pRoom, char *name);
WEATHER_DATA *get_room_weather(ROOM_DATA *pRoom);
bool	comp_obj(CHAR_DATA *ch, OBJ_DATA *obj);
bool	donation_compare_obj(OBJ_DATA *obj, CHAR_DATA *ch);
bool	hitched_mob(CHAR_DATA *ch);
char	*title_male(int chclass, int level);
char	*title_female(int chclass, int level);
int		compute_armor_class(CHAR_DATA *ch);
int		hunt_victim(CHAR_DATA *ch);
int		exp_to_level(int level);
int		tot_exp_to_level(int level);
long	find_class_bitvector(char arg);
long	find_race_bitvector(char arg);
void	show_map(CHAR_DATA *ch);
void	ReadSpellbook(CHAR_DATA *ch, OBJ_DATA *obj, char *argument);
void	list_building_to_char(BUILDING_DATA *bldlist, CHAR_DATA *ch);
void	show_building_cond(CHAR_DATA *ch, BUILDING_DATA *bld);
void	list_ship_to_char(SHIP_DATA *Ships, CHAR_DATA *ch);
void	wild_scan(CHAR_DATA *ch);
int		look_at_vehicle(CHAR_DATA *ch, VEHICLE_DATA *vehicle);

/* local functions */
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
ACMD(do_commands);
ACMD(do_exits);
char	*find_exdesc(char *word, EXTRA_DESCR *list);
int		sort_commands_helper(const void *a, const void *b);
void	print_object_location(int num, OBJ_DATA *obj, CHAR_DATA *ch, int recur);
void	show_obj_to_char(OBJ_DATA *object, CHAR_DATA *ch, int mode, bool RDR);
void	list_obj_to_char(OBJ_DATA *list, CHAR_DATA *ch, int mode, int show, bool RDR);
void	perform_mortal_where(CHAR_DATA *ch, char *arg);
void	perform_immort_where(CHAR_DATA *ch, char *arg);
void	sort_commands(void);
void	diag_char_to_char(CHAR_DATA *i, CHAR_DATA *ch);
void	look_at_char(CHAR_DATA *i, CHAR_DATA *ch);
void	list_one_char(CHAR_DATA *i, CHAR_DATA *ch, int num);
void	list_char_to_char(CHAR_DATA *list, CHAR_DATA *ch);
void	do_auto_exits(CHAR_DATA *ch);
void	look_in_direction(CHAR_DATA *ch, int dir);
void	look_in_obj(CHAR_DATA *ch, char *arg);
void	look_at_target(CHAR_DATA *ch, char *arg);

/* local globals */
int	*cmd_sort_info;
int	boot_high	= 0;


/*
 * Mode = 0  = from look_at_room()
 * Mode = 1  = from look_at_char() for inventory
 * Mode = 2  = from look_in_obj()
 * Mode = 3  = from do_equipment() and look_at_char() for equipment
 * Mode = 4  = from do_inventory()
 * Mode = 5  = from look_at_target() for obj with NO extra description
 * Mode = 6  = from look_at_target() for obj flags only
 * Mode = 7  = from look_at_vehicle()
 */
void show_obj_to_char(OBJ_DATA *object, CHAR_DATA *ch, int mode, bool RDR)
{
	char buf[MAX_STRING_LENGTH];
	bool show_flags = TRUE;

	*buf = '\0';
	
	switch (mode)
	{
	case 0:			/* in room */
		if (object->description)
		{
			if ( RDR )
			{
				/* check to see if the item is too powerful for the player */
				if (GET_OBJ_LEVEL(object) > GET_LEVEL(ch) ||
					(invalid_align(ch, object) || invalid_class(ch, object)))
					strcpy(buf, "&b&1X&0 ");
				else if (donation_compare_obj(object, ch))
					strcpy(buf, "&b&2*&0 ");
				else
					strcpy(buf, "  ");
			}

			if (object->count > 1)
				sprintf(buf + strlen(buf), "(%d) ", object->count);
			else if (GET_OBJ_TYPE(object) == ITEM_MONEY)
				sprintf(buf + strlen(buf), "[%d] ", GET_OBJ_VAL(object, 0));

			strcat(buf, object->description);

			/* special cases -- FURNITURES */
			if (GET_OBJ_TYPE(object) == ITEM_FURNITURE && object->first_content)
			{
				OBJ_DATA *pObj = NULL;
				
				if (OBJVAL_FLAGGED(object, CONT_CLOSED))

				sprintf(buf+strlen(buf), "\r\n    On it you see:\r\n");
				
				for (pObj = object->first_content; pObj; pObj = pObj->next_content)
				{
					if (!CAN_SEE_OBJ(ch, pObj))
						continue;
					
					if (pObj->count > 1)
						sprintf(buf+strlen(buf), "    - (%d) %s\r\n",
							pObj->count, pObj->short_description);
					else if (GET_OBJ_TYPE(pObj) == ITEM_MONEY)
						sprintf(buf+strlen(buf), "    - [%d] %s\r\n",
							GET_OBJ_VAL(pObj, 0), pObj->short_description);
					else
						sprintf(buf+strlen(buf), "    - %s\r\n", pObj->short_description);
				}
			}
		}
		break;

	case 1:			/* look_at_char */
	case 2:			/* look_in_obj */
	case 7:			/* look_at_vehicle */
		if (object->short_description)
		{
			if (object->count > 1)
				sprintf(buf, "(%d) ", object->count);
			else if (GET_OBJ_TYPE(object) == ITEM_MONEY)
				sprintf(buf, "[%d] ", GET_OBJ_VAL(object, 0));
			
			strcat(buf, object->short_description);
		}
		break;

	case 3:			/* equipment */		
		if (object->short_description)
			strcpy(buf, object->short_description);
		break;

	case 4:			/* inventory */
		if (object->short_description)
		{
			if (GET_OBJ_LEVEL(object) > GET_LEVEL(ch) ||
				(invalid_align(ch, object) || invalid_class(ch, object)))
				strcpy(buf, "&b&1X&0 ");
			else
				strcpy(buf, "  ");
			
			if ( GET_OBJ_OWNER(object) == GET_IDNUM(ch) )
				strcat(buf, "&b&6");
			if ( object->count > 1 )
				sprintf(buf + strlen(buf), "(%d) ", object->count);
			else if ( GET_OBJ_TYPE(object) == ITEM_MONEY )
				sprintf(buf + strlen(buf), "[%d] ", GET_OBJ_VAL(object, 0));
			
			strcat(buf, object->short_description);
		}
		break;

	case 5:			/* look_at_target() for obj with NO extra description */
		if ( !IS_CORPSE(object) )
			sprintf(buf, "It looks %s [%d/%d].\r\n",
				obj_cond_table[find_objcond_index(object)],
				GET_OBJ_COND(object), GET_OBJ_MAXCOND(object));
		
		if (GET_OBJ_TYPE(object) != ITEM_DRINKCON)
			strcat(buf, "You see nothing special..");
		else
			strcat(buf, "It looks like a drink container.");

		show_flags = FALSE;
		break;
	}

	if (show_flags)
	{
		if (OBJ_FLAGGED(object, ITEM_INVISIBLE))
			strcat(buf, " &b&6(invisible)&0");
		if (OBJ_FLAGGED(object, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
			strcat(buf, " &b&4..It glows blue!&0");
		if (OBJ_FLAGGED(object, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
			strcat(buf, " &b&3..It glows yellow!&0");
		if (OBJ_FLAGGED(object, ITEM_GLOW))
			strcat(buf, " &b&7..It has a soft glowing aura!&0");
		if (OBJ_FLAGGED(object, ITEM_HUM))
			strcat(buf, " &b&5..It emits a faint humming sound!&0");
	}

	if (!*buf)
		return;

	/*
	 * needed in case of listing of objs on furnitures;
	 * if the furniture is the last object in the room list
	 * without this check two \r\n would be placed...
	 */
	if (buf[strlen(buf)-1] != '\n')
		strcat(buf, "\r\n");
	strcat(buf, "&0");
	page_string(ch->desc, buf, TRUE);
}


void list_obj_to_char(OBJ_DATA *list, CHAR_DATA *ch, int mode, int show, bool RDR)
{
	OBJ_DATA *i;
	bool found = FALSE;
	
	for (i = list; i; i = i->next_content)
	{
		if (CAN_SEE_OBJ(ch, i) && !i->people)
		{
			show_obj_to_char(i, ch, mode, RDR);
			found = TRUE;
		}
	}

	if (!found && show)
		send_to_char(" Nothing.\r\n", ch);
}



void diag_char_to_char(CHAR_DATA *i, CHAR_DATA *ch)
{
	char buf[MAX_STRING_LENGTH];
	int percent;
	
	if (GET_MAX_HIT(i) > 0)
		percent = percentage(GET_HIT(i), GET_MAX_HIT(i));
	else
		percent = -1;		/* How could MAX_HIT be < 1?? */
	
	sprintf(buf, "&b&5%s", PERS(i, ch));
	
	if (percent >= 100)
		strcat(buf, " is in excellent condition.&0\r\n");
	else if (percent >= 90)
		strcat(buf, " has a few scratches.&0\r\n");
	else if (percent >= 83)
		strcat(buf, " has a nasty looking welt on the forehead.&0\r\n");
	else if (percent >= 76)
		strcat(buf, " has some small wounds and bruises.&0\r\n");
	else if (percent >= 69)
		strcat(buf, " has some minor wounds.&0\r\n");
	else if (percent >= 62)
		strcat(buf, " winces in pain.&0\r\n");
	else if (percent >= 55)
		strcat(buf, " has quite a few wounds.&0\r\n");
	else if (percent >= 48)
		strcat(buf, " grimaces with pain.&0\r\n");
	else if (percent >= 41)
		strcat(buf, " has some nasty wounds and bleeding cuts.&0\r\n");
	else if (percent >= 36)
		strcat(buf, " has some large, gaping wounds.&0\r\n");
	else if (percent >= 29)
		strcat(buf, " has many grievous wounds.&0\r\n");
	else if (percent >= 22)
		strcat(buf, " screams in agony.&0\r\n");
	else if (percent >= 15)
		strcat(buf, " is covered with blood from oozing wounds.&0\r\n");
	else if (percent >= 8)
		strcat(buf, " pales visibly as death nears.&0\r\n");
	else if (percent >= 3)
		strcat(buf, " barely clings to life.&0\r\n");
	else
		strcat(buf, " is bleeding awfully from big wounds.&0\r\n");
	
	send_to_char(buf, ch);
}


void look_at_char(CHAR_DATA *i, CHAR_DATA *ch)
{
	int j, found;
	
	if (!ch->desc)
		return;
	
	if (GET_DDESC(i))
		send_to_char(GET_DDESC(i), ch);
	else
		act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

	if (PLR_FLAGGED(i, PLR_GHOST))
		ch_printf(ch, "&b&5%s is a ghost!", PERS(i, ch));
	else
		diag_char_to_char(i, ch);

	if (i->in_obj)
	{
		char sbaf[MAX_STRING_LENGTH];

		sprintf(sbaf, "$e is %s on $p.", position_types[(int) GET_POS(i)]);
		act(sbaf, FALSE, i, i->in_obj, ch, TO_VICT);
	}
	else if ( RIDING(i) )
		act("$e is mounted upon $N.", FALSE, i, NULL, ch, TO_VICT);
	else if ( IS_NPC(i) && RIDDEN_BY(i) )
		act("$e is mounted by $N.", FALSE, i, NULL, ch, TO_VICT);

	found = FALSE;
	for (j = 0; !found && j < NUM_WEARS; j++)
	{
		if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
			found = TRUE;
	}

	if (found)
	{
		send_to_char("\r\n", ch);	/* act() does capitalization. */
		act("$n is using:", FALSE, i, 0, ch, TO_VICT);
		for (j = 0; j < NUM_WEARS; j++)
		{
			if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
			{
				send_to_char(wear_where[j], ch);
				show_obj_to_char(GET_EQ(i, j), ch, 3, FALSE);
			}
		}
	} 
	
	if (ch != i && (IS_THIEF(ch) || GET_LEVEL(ch) >= LVL_IMMORT))
	{
		act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
		list_obj_to_char(i->first_carrying, ch, 1, FALSE, FALSE);
	}
}


void list_one_char(CHAR_DATA *i, CHAR_DATA *ch, int num)
{
	const char *positions[] =
	{
	  " is lying here, dead.",
	  " is lying here, mortally wounded.",
	  " is lying here, incapacitated.",
	  " is lying here, stunned.",
	  " is sleeping here.",
	  " is resting here.",
	  " is sitting here.",
	  "!FIGHTING!",
	  " is standing here."
	};
	
	if (IS_NPC(i) && GET_LDESC(i) && GET_POS(i) == GET_DEFAULT_POS(i))
	{
		if (AFF_FLAGGED(i, AFF_INVISIBLE))
			strcpy(buf, "*");
		else
			*buf = '\0';
		
		if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
		{
			if (IS_EVIL(i))
				strcat(buf, "&b&1(Red Aura)&0 ");
			else if (IS_GOOD(i))
				strcat(buf, "&b&4(Blue Aura)&0 ");
		}
		
		strcat(buf, "&b&2");
		strcat(buf, GET_LDESC(i));
		
		if (num > 1)
		{
			while ((buf[strlen(buf)-1]=='\r')  ||
				(buf[strlen(buf)-1]=='\n') ||
				(buf[strlen(buf)-1]==' '))
				buf[strlen(buf)-1] = '\0';
			
			sprintf(buf2," [&0&b&6%d&0&b&2]\r\n", num);
			strcat(buf, buf2);
		}
		
		strcat(buf, "&0");
		send_to_char(buf, ch);
		
		if (AFF_FLAGGED(i, AFF_SANCTUARY))
			act("   &b&7$e glows with a bright light!&0", FALSE, i, 0, ch, TO_VICT);
		if (AFF_FLAGGED(i, AFF_BLIND))
			act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
		
		return;
	}

	if (IS_NPC(i))
	{
		strcpy(buf, GET_SDESC(i));
		CAP(buf);
	}
	else
	{
		sprintf(buf, "&b&3%s&0", PERS(i, ch));
		// TODO -- this has to be tested with many players.....
		if (is_char_known(ch, i))
			sprintf(buf + strlen(buf), " &b&2%s&0", GET_TITLE(i));
	}
	
	if (PLR_FLAGGED(i, PLR_GHOST))
		strcat(buf, " (&b&6ghost&0)");
	if (AFF_FLAGGED(i, AFF_INVISIBLE))
		strcat(buf, " (invisible)");
	if (AFF_FLAGGED(i, AFF_HIDE))
		strcat(buf, " (hidden)");
	if (!IS_NPC(i) && !i->desc)
		strcat(buf, " (linkless)");
	if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
		strcat(buf, " (writing)");

	// using a furniture
	if ( i->in_obj )
	{
		sprintf(buf + strlen(buf), " is here %s on %s.",
			position_types[(int) GET_POS(i)], i->in_obj->short_description);
	}
	// mounted
	else if (RIDING(i) && RIDING(i)->in_room == i->in_room)
	{
		strcat(buf, " is here, mounted upon ");
		strcat(buf, PERS(RIDING(i), ch));
		
		// non-fighting
		if (GET_POS(i) != POS_FIGHTING)
			strcat(buf, ".");
		// fighting
		else
		{
			if (FIGHTING(i))
			{
				strcat(buf, ", fighting ");
				if (FIGHTING(i) == ch)
					strcat(buf, "YOU!");
				else
				{
					if (i->in_room == FIGHTING(i)->in_room)
						strcat(buf, PERS(FIGHTING(i), ch));
					else
						strcat(buf, "someone who has already left");
					strcat(buf, "!");
				}
			}
			else                      /* NIL fighting pointer */
				strcat(buf, ", struggling with thin air.");
		}
	}
	// non-fighting
	else if (GET_POS(i) != POS_FIGHTING)
		strcat(buf, positions[(int) GET_POS(i)]);
	// fighting
	else
	{
		if (FIGHTING(i))
		{
			strcat(buf, " is here, fighting ");
			if (FIGHTING(i) == ch)
				strcat(buf, "YOU!");
			else
			{
				if (i->in_room == FIGHTING(i)->in_room)
					strcat(buf, PERS(FIGHTING(i), ch));
				else
					strcat(buf, "someone who has already left");
				strcat(buf, "!");
			}
		}
		else			/* NIL fighting pointer */
			strcat(buf, " is here struggling with thin air.");
	}
	
	if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
	{
		if (IS_EVIL(i))
			strcat(buf, "&b&1(Red Aura)&0 ");
		else if (IS_GOOD(i))
			strcat(buf, "&b&4(Blue Aura)&0 ");
	}
	strcat(buf, "&0\r\n");
	send_to_char(buf, ch);
	
	if (AFF_FLAGGED(i, AFF_SANCTUARY))
		act("   &b&7$e glows with a bright light!&0", FALSE, i, 0, ch, TO_VICT);
	if (AFF_FLAGGED(i, AFF_FIRESHIELD))
		act("   &b&7$e is surrounded by burning flames!&0", FALSE, i, 0, ch, TO_VICT);
}



void list_char_to_char(CHAR_DATA *list, CHAR_DATA *ch)
{
	CHAR_DATA *i, *plr_list[100];
	int num = 0, counter, locate_list[100], found = FALSE;
	
	for (i = list; i; i = i->next_in_room)
	{
		if ( i != ch && CAN_SEE(ch, i) )
		{
			if (RIDDEN_BY(i) && RIDDEN_BY(i)->in_room == i->in_room)
				continue;
			// we don't see directly wagoning people
			if (WAGONER(i) && WAGONER(i)->in_room == i->in_room)
				continue;
			// we don't see directly dragging mobs
			if (IS_NPC(i) && hitched_mob(i))
				continue;

			if (num < 100)
			{
				found = FALSE;
				for (counter = 0; (counter < num && !found); counter++)
				{
					if (i->nr == plr_list[counter]->nr &&
						/*
						 * for odd reasons, you may want to seperate same nr mobs by factors
						 * other than position, the aff flags, and the fighters. (perhaps you want
						 * to list switched chars differently.. or polymorphed chars?)
						 */
						(GET_POS(i) == GET_POS(plr_list[counter])) &&
						(AFF_FLAGS(i) == AFF_FLAGS(plr_list[counter])) &&
						(FIGHTING(i) == FIGHTING(plr_list[counter])) &&
						!strcmp(GET_NAME(i), GET_NAME(plr_list[counter])))
					{
						locate_list[counter] += 1;
						found = TRUE;
					}
				}
				if (!found)
				{
					plr_list[num] = i;
					locate_list[num] = 1;
					num++;
				}
			}
			else
				list_one_char(i,ch,0);
		}
	}

	if (num)
	{
		for (counter = 0; counter < num; counter++)
		{
			if (locate_list[counter] > 1)
				list_one_char(plr_list[counter], ch, locate_list[counter]);
			else
				list_one_char(plr_list[counter], ch, 0);
		}
	}
}


void do_auto_exits(CHAR_DATA *ch)
{
	EXIT_DATA *pexit;
	
	*buf = '\0';
	
	for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
	{
		if (!EXIT_FLAGGED(pexit, EX_CLOSED) && !EXIT_FLAGGED(pexit, EX_HIDDEN))
			sprintf(buf + strlen(buf), "%c ", LOWER(*dirs[pexit->vdir]));
	}
	sprintf(buf2, "%s[ Exits: %s]%s\r\n", CCCYN(ch, C_NRM),
		*buf ? buf : "None! ", CCNRM(ch, C_NRM));
	
	send_to_char(buf2, ch);
}


ACMD(do_exits)
{
	EXIT_DATA *pexit;
	char buf[MAX_STRING_LENGTH];
	
	buf[0] = '\0';
	
	if (AFF_FLAGGED(ch, AFF_BLIND))
	{
		send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
		return;
	}

	if (IN_WILD(ch))
	{
		send_to_char( "There are exits all around you.\r\n", ch );
		return;
	}

	for (pexit = ch->in_room->first_exit; pexit; pexit = pexit->next)
	{
		if (EXIT_FLAGGED(pexit, EX_FALSE)  ||
		    EXIT_FLAGGED(pexit, EX_CLOSED) ||
		    EXIT_FLAGGED(pexit, EX_HIDDEN))
			continue;

		if (IS_IMMORTAL(ch))
			sprintf(buf2, "%-5s - [%5d] %s\r\n",
				dirs[pexit->vdir], pexit->vnum, pexit->to_room->name );
		else
		{
			sprintf(buf2, "%-5s - ", dirs[pexit->vdir]);
			if (IS_DARK(pexit->to_room) && !CAN_SEE_IN_DARK(ch))
				strcat(buf2, "Too dark to tell\r\n");
			else
			{
				strcat(buf2, ROOM_RNAME(pexit->to_room) );
				strcat(buf2, "\r\n");
			}
		}
		strcat(buf, CAP(buf2));
	}

	send_to_char("Obvious exits:\r\n", ch);
	
	if (!*buf && ch->in_vehicle)
		strcpy(buf, "<&6out&0>\r\n");

	if (*buf)
		send_to_char(buf, ch);
	else
		send_to_char(" None.\r\n", ch);
}

/* Stampa le uscite dalla stanza - FAB 11-97 */
void show_exits( CHAR_DATA *ch )
{
	EXIT_DATA *pexit;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	
	buf2[0] = '\0';

	for (pexit = ch->in_room->first_exit; pexit; pexit = pexit->next)
	{
		if (EXIT_FLAGGED(pexit, EX_HIDDEN))
			continue;

		if (EXIT_FLAGGED(pexit, EX_CLOSED))
			sprintf(buf2 + strlen(buf2), "[%s%s&0] ", exits_color[pexit->vdir], dirs[pexit->vdir]);
		else if (EXIT_FLAGGED(pexit, EX_FALSE))
			sprintf(buf2 + strlen(buf2), "<%s%s&0> ", exits_color[pexit->vdir], dirs[pexit->vdir]);
		else
			sprintf(buf2 + strlen(buf2), "%s%s&0 ", exits_color[pexit->vdir], dirs[pexit->vdir]);
	}

	if ( !*buf2 && ch->in_vehicle )
		strcat(buf2, "<&6out&0> ");

	if (*buf2)
		sprintf(buf, "Uscite: %s&0\r\n", buf2);
	else
		strcpy(buf, "Uscite: &b&1Nessuna&0\r\n");
	
	send_to_char(buf, ch);
}

void look_at_room(CHAR_DATA *ch, int ignore_brief)
{
	char *roomdesc = NULL;

	if (!ch->desc)
		return;
	
	if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch))
	{
		send_to_char("It is pitch black...\r\n", ch);
		return;
	}
	
	if (AFF_FLAGGED(ch, AFF_BLIND))
	{
		send_to_char("You see nothing but infinite darkness...\r\n", ch);
		return;
	}

	if ( ROOM_AFFECTED(ch->in_room, RAFF_FOG) && !IS_IMMORTAL(ch) )
	{
		send_to_char("Your view is obscured by a thick fog.\r\n", ch);
		return;
	}

	send_to_char(CCCYN(ch, C_NRM), ch);
	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS))
	{
		sprintbit(ROOM_FLAGS(ch->in_room), room_bits, buf);
		if ( ch->in_room->zone == WILD_ZONE )
			sprintf(buf2, "[%5d %5d] %s [ %s]",
				GET_Y(ch), GET_X(ch), ROOM_NAME(ch), buf);
		else
			sprintf(buf2, "[%5d] %s [ %s]", ch->in_room->number, ROOM_NAME(ch), buf);
		send_to_char(buf2, ch);
	}
	else
		send_to_char(ROOM_NAME(ch), ch);
	
	send_to_char(CCNRM(ch, C_NRM), ch);
	send_to_char("\r\n", ch);

	if ((IN_WILD(ch) && SECT(ch->in_room) != SECT_CITY) || IN_FERRY(ch) )
		show_map( ch );
	
	if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief || ROOM_FLAGGED(ch->in_room, ROOM_DEATH))
	{
		char buf[MAX_STRING_LENGTH];

		roomdesc = strdup( ROOM_DESC(ch) );
		master_parser(roomdesc, ch, NULL, NULL);
		sprintf(buf, "&b&3%s&0\r\n", roomdesc);
		send_to_char(buf, ch);
	}
	
	/* autoexits */
	if ( !IS_NPC(ch) && !IN_WILD(ch) )
	{
		if ( PRF_FLAGGED(ch, PRF_AUTOEXIT) )
			do_auto_exits( ch );
		else
			show_exits( ch );
	}

	if ( ch->in_room->portal_stone )
		ch_printf(ch, "%s\r\n", ch->in_room->portal_stone->description);

	if (ch->in_room->ferryboat)
		send_to_char("&b&4You see a ferryboat. Go UP to embark.&0\r\n", ch);

	/* list rooms contents */
	list_building_to_char(ch->in_room->buildings, ch);
	list_ship_to_char(ch->in_room->ships, ch);
	list_vehicle_to_char(ch->in_room->vehicles, ch);
	if (ch->in_room->number == donation_room_1)
		list_obj_to_char(ch->in_room->first_content, ch, 0, FALSE, TRUE);
	else
		list_obj_to_char(ch->in_room->first_content, ch, 0, FALSE, FALSE);
	list_char_to_char(ch->in_room->people, ch);

	send_to_char(CCNRM(ch, C_NRM), ch);

	if (AFF_FLAGGED(ch, AFF_HUNTING))
		hunt_victim(ch);
}



void look_in_direction(CHAR_DATA *ch, int dir)
{
	EXIT_DATA *pexit = find_exit(ch->in_room, dir);

	if (!pexit || EXIT_FLAGGED(pexit, EX_HIDDEN))
	{
		send_to_char("Nothing special there...\r\n", ch);
		return;
	}

	if (pexit->description)
		send_to_char(pexit->description, ch);
	else
		send_to_char("You see nothing special.\r\n", ch);

	if (pexit->keyword)
	{
		if (EXIT_FLAGGED(pexit, EX_CLOSED))
			ch_printf(ch, "The %s is closed.\r\n", fname(pexit->keyword));
		else if (EXIT_FLAGGED(pexit, EX_ISDOOR))
			ch_printf(ch, "The %s is open.\r\n", fname(pexit->keyword));
	}
}



void look_in_obj(CHAR_DATA *ch, char *arg)
{
	OBJ_DATA *obj = NULL;
	CHAR_DATA *dummy = NULL;
	int amt, bits;
	
	if (!*arg)
		send_to_char("Look in what?\r\n", ch);
	else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM |
		FIND_OBJ_EQUIP, ch, &dummy, &obj)))
	{
		sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
		send_to_char(buf, ch);
	}
	else if ( GET_OBJ_TYPE(obj) == ITEM_FURNITURE )
	{
		if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
			send_to_char("It is closed.\r\n", ch);
		else
		{
			if ( check_trap( ch, obj, TRAP_ACT_LOOKIN ) )
				return;

			/*
			send_to_char(fname(obj->name), ch);
			switch (bits)
			{
			case FIND_OBJ_INV:
				send_to_char(" (carried): \r\n", ch);
				break;
			case FIND_OBJ_ROOM:
				send_to_char(" (here): \r\n", ch);
				break;
			case FIND_OBJ_EQUIP:
				send_to_char(" (used): \r\n", ch);
				break;
			}
			*/
			
			list_obj_to_char(obj->first_content, ch, 2, TRUE, FALSE);
		}
	}
	else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
			(GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
			(GET_OBJ_TYPE(obj) != ITEM_MISSILE_CONT) &&
			(GET_OBJ_TYPE(obj) != ITEM_CONTAINER))
		send_to_char("There's nothing inside that!\r\n", ch);
	else
	{
		if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
		{
			if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
				send_to_char("It is closed.\r\n", ch);
			else
			{
				if ( check_trap( ch, obj, TRAP_ACT_LOOKIN ) )
					return;

				send_to_char(fname(obj->name), ch);
				switch (bits)
				{
				case FIND_OBJ_INV:
					send_to_char(" (carried): \r\n", ch);
					break;
				case FIND_OBJ_ROOM:
					send_to_char(" (here): \r\n", ch);
					break;
				case FIND_OBJ_EQUIP:
					send_to_char(" (used): \r\n", ch);
					break;
				}
				
				list_obj_to_char(obj->first_content, ch, 2, TRUE, FALSE);
			}
		}
		else if (GET_OBJ_TYPE(obj) == ITEM_MISSILE_CONT)
		{
			send_to_char(fname(obj->name), ch);
			switch (bits)
			{
			case FIND_OBJ_INV:
				send_to_char(" (carried): \r\n", ch);
				break;
			case FIND_OBJ_ROOM:
				send_to_char(" (here): \r\n", ch);
				break;
			case FIND_OBJ_EQUIP:
				send_to_char(" (used): \r\n", ch);
				break;
			}
			list_obj_to_char(obj->first_content, ch, 2, TRUE, FALSE);
		}
		else		/* item must be a fountain or drink container */
		{
			if ( check_trap( ch, obj, TRAP_ACT_LOOKIN ) )
				return;

			if (GET_OBJ_VAL(obj, 1) <= 0)
				send_to_char("It is empty.\r\n", ch);
			else
			{
				if (GET_OBJ_VAL(obj,0) <= 0 || GET_OBJ_VAL(obj,1)>GET_OBJ_VAL(obj,0))
				{
					sprintf(buf, "Its contents seem somewhat murky.\r\n"); /* BUG */
				}
				else
				{
					amt = (GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0);
					sprinttype(GET_OBJ_VAL(obj, 2), color_liquid, buf2);
					sprintf(buf, "It's %sfull of a %s liquid.\r\n", fullness[amt], buf2);
				}
				send_to_char(buf, ch);
			}
		}
	}
}



char *find_exdesc(char *word, EXTRA_DESCR *list)
{
	EXTRA_DESCR *i;
	
	for (i = list; i; i = i->next)
		if (isname(word, i->keyword))
			return (i->description);
		
	return (NULL);
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
void look_at_target(CHAR_DATA *ch, char *arg)
{
	CHAR_DATA *found_char = NULL;
	OBJ_DATA *obj, *found_obj = NULL;
	int bits, found = FALSE, j, fnum, i = 0;
	char *desc;
	
	if (!ch->desc)
		return;
	
	if (!*arg)
	{
		send_to_char("Look at what?\r\n", ch);
		return;
	}
	
	bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		FIND_CHAR_ROOM, ch, &found_char, &found_obj);
	
	if (!bits)
	{
		BUILDING_DATA *bld = find_building_in_room_by_name(ch->in_room, str_dup(arg));
		SHIP_DATA *ship = find_ship_in_room_by_name(ch->in_room, str_dup(arg));
		VEHICLE_DATA *pVeh = find_vehicle_in_room_by_name(ch, str_dup(arg));

		if (bld)
		{
			if (BUILDING_FLAGGED(bld, BLD_F_RUIN))
				ch_printf(ch, "It's what remains of %s %s.\r\n",
					AN(bld->type->name), bld->type->name);
			else
				show_building_cond(ch, bld);
			return;
		}

		if ( ship )
		{
			ch_printf( ch, "'%s' is a %s ship.\r\n", ship->name, ship->type->name );
			return;
		}

		if (pVeh)
		{
			if (look_at_vehicle(ch, pVeh))
				return;
		}

		if (ch->in_room->ferryboat && isname(arg, "ferryboats"))
		{
			ch_printf(ch, 
				"It's a simple ferryboat that take people and goods across the river.\r\n"
				"It will depart in %d minute%s.\r\n",
				ch->in_room->ferryboat->timer,
				(ch->in_room->ferryboat->timer > 1 ? "s" : ""));
			return;
		}
	}

	/* Is the target a character? */
	if (found_char != NULL)
	{
		look_at_char(found_char, ch);
		if (ch != found_char)
		{
			if (CAN_SEE(found_char, ch))
				act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
			act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
		}
		return;
	}
	
	/* Strip off "number." from 2.foo and friends. */
	if (!(fnum = get_number(&arg)))
	{
		send_to_char("Look at what?\r\n", ch);
		return;
	}
	
	/* Does the argument match an extra desc in the room? */
	if ((desc = find_exdesc(arg, ch->in_room->ex_description)) != NULL && ++i == fnum)
	{
		page_string(ch->desc, desc, FALSE);
		return;
	}
	
	/* Does the argument match an extra desc in the char's equipment? */
	for (j = 0; j < NUM_WEARS && !found; j++)
	{
		if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
		{
			if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL && ++i == fnum)
			{
				send_to_char(desc, ch);
				found = TRUE;
			}
		}
	}
	
	/* Does the argument match an extra desc in the char's inventory? */
	for (obj = ch->first_carrying; obj && !found; obj = obj->next_content)
	{
		if (CAN_SEE_OBJ(ch, obj))
		{
			if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum)
			{
				send_to_char(desc, ch);
				found = TRUE;
			}
		}
	}
	
	/* Does the argument match an extra desc of an object in the room? */
	for (obj = ch->in_room->first_content; obj && !found; obj = obj->next_content)
	{
		if (CAN_SEE_OBJ(ch, obj))
		{
			if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum)
			{
				send_to_char(desc, ch);
				found = TRUE;
			}
		}
	}
	
	/* If an object was found back in generic_find */
	if (bits)
	{
		if (!found)
			show_obj_to_char(found_obj, ch, 5, FALSE);	/* Show no-description */
		else
			show_obj_to_char(found_obj, ch, 6, FALSE);	/* Find hum, glow etc */
	}
	else if (!found)
		send_to_char("You do not see that here.\r\n", ch);
}


void look_read( CHAR_DATA *ch, char *argument )
{
	OBJ_DATA *obj;
	char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	half_chop(argument, arg, arg2);

	/* first try to read something in inventory */
	if ( !(obj = get_obj_in_list_vis_rev(ch, str_dup(arg), NULL, ch->last_carrying) ) )
	{
		/* then try to read something in room */
		if ( !(obj = get_obj_in_list_vis_rev(ch, str_dup(arg), NULL, ch->in_room->last_content) ) )
		{
			/* last try to read something inside an object */
			if ( ch->in_obj )
			{
				if ( !(obj = get_obj_in_list_vis_rev(ch, str_dup(arg), NULL, ch->in_obj->last_content) ) )
				{
					look_at_target(ch, arg);
					return;
				}
			}
			else
			{
				look_at_target(ch, arg);
				return;
			}
		}
	}

	if (OBJ_FLAGGED(obj, ITEM_IS_SPELLBOOK))
	{
		ReadSpellbook(ch, obj, arg2);
		return;
	}

	if ( GET_OBJ_TYPE(obj) == ITEM_NOTE )
	{
		char buf[MAX_STRING_LENGTH];

		if (obj->action_description)
		{
			strcpy(buf, "There is something written upon it:\r\n\r\n");
			strcat(buf, obj->action_description);
			page_string(ch->desc, buf, 1);
		}
		else
			send_to_char("It's blank.\r\n", ch);
		return;
	}

	act("You try to read $o, but fail..", FALSE, ch, obj, NULL, TO_CHAR);
}


ACMD(do_look)
{
	char arg2[MAX_INPUT_LENGTH];
	int look_type;
	
	if (!ch->desc)
		return;
	
	if (GET_POS(ch) < POS_SLEEPING)
		send_to_char("You can't see anything but stars!\r\n", ch);
	else if (AFF_FLAGGED(ch, AFF_BLIND))
		send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
	else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch))
	{
		send_to_char("It is pitch black...\r\n", ch);
		list_char_to_char(ch->in_room->people, ch);	/* glowing red eyes */
	}
	else if ( ROOM_AFFECTED(ch->in_room, RAFF_FOG) && !IS_IMMORTAL(ch) )
		send_to_char("Your view is obscured by a thick fog.\r\n", ch);
	else
	{
		half_chop(argument, arg, arg2);
		
		if (subcmd == SCMD_READ)
		{
			if (!*arg)
				send_to_char("Read what?\r\n", ch);
			else
				look_read(ch, argument);
			return;
		}

		if (!*arg)			/* "look" alone, without an argument at all */
			look_at_room(ch, 1);
		else if (is_abbrev(arg, "in"))
			look_in_obj(ch, arg2);
		/* did the char type 'look <direction>?' */
		else if ((look_type = search_block(arg, dirs, FALSE)) >= 0)
			look_in_direction(ch, look_type);
		else if (is_abbrev(arg, "at"))
			look_at_target(ch, arg2);
		else if (is_abbrev(arg, "out") && ch->in_vehicle)
		{
			ROOM_DATA *pRoom = ch->in_room;
			
			ch->in_room = ch->in_vehicle->in_room;
			look_at_room(ch, 1);
			ch->in_room = pRoom;
		}
		else
			look_at_target(ch, arg);
	}
}



ACMD(do_examine)
{
	CHAR_DATA *tmp_char;
	OBJ_DATA *tmp_object;
	char tempsave[MAX_INPUT_LENGTH];
	
	one_argument(argument, arg);
	
	if (!*arg)
	{
		send_to_char("Examine what?\r\n", ch);
		return;
	}
	
	if ( ROOM_AFFECTED(ch->in_room, RAFF_FOG) && !IS_IMMORTAL(ch) )
	{
		send_to_char("Your view is obscured by a thick fog.\r\n", ch);
		return;
	}

	/* look_at_target() eats the number. */
	look_at_target(ch, strcpy(tempsave, arg));
	
	generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);
	
	if (tmp_object)
	{
		if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
			(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
			(GET_OBJ_TYPE(tmp_object) == ITEM_MISSILE_CONT) ||
			(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER))
		{
			send_to_char("When you look inside, you see:\r\n", ch);
			look_in_obj(ch, arg);
		}

		if ( GET_OBJ_TYPE(tmp_object) == ITEM_FURNITURE )
		{
			send_to_char("On it you see:\r\n", ch);
			look_in_obj(ch, arg);
		}

		/* Anyone using this furniture? */
		if (GET_OBJ_TYPE(tmp_object) == ITEM_FURNITURE && tmp_object->people)
		{
			char buf[MAX_STRING_LENGTH];
			int found;

			strcpy(buf, "It is being used as furniture by:");
			
			for (found = 0, tmp_char = tmp_object->people; tmp_char; tmp_char = tmp_char->next_in_obj)
			{
				if ( !CAN_SEE(ch, tmp_char) )
					continue;
				
				sprintf(buf + strlen(buf), "%s %s", found++ ? "," : "", PERS(tmp_char, ch));

				if (strlen(buf) >= 62)
				{
					if (tmp_char->next_in_obj)
						strcat(buf, ",\r\n");
					else
						strcat(buf, "\r\n");
					*buf = found = 0;
				}
			}
			
			if (*buf)
				send_to_char(strcat(buf, "\r\n"), ch);
		}
	}
}

ACMD(do_gold)
{
	int amount = get_gold(ch);

	if (amount == 0)
		send_to_char("You're broke!\r\n", ch);
	else if (amount == 1)
		send_to_char("You have one miserable little gold coin.\r\n", ch);
	else
	{
		sprintf(buf, "You have %s [%d] gold coins.\r\n", numberize(amount), amount);
		send_to_char(buf, ch);
	}
}


char *make_bar(long val, long max, long len, int color)
{
	char bar[MAX_INPUT_LENGTH] = "\0";
	int i, n;
	
	if (color < 1 || color > 7)
		color = 6;

	if (val >= max)
	{
		sprintf(bar, "&b&%d", color);
		for (i = 0; i < len; i++)
			strcat(bar, "X");
	}
	else
	{
		sprintf(bar, "&b&%d", color);
		for (i = (val * len) / max, n = 0; n < i; n++)
			strcat(bar, "X");

		sprintf(bar + strlen(bar), "&0&%d", color);
		while ((n++) < len)
			strcat(bar, "X");
	}
	strcat(bar, "&0");

	return (str_dup(bar));
}

ACMD(do_score)
{
	char buf[MAX_STRING_LENGTH], bbar[MAX_STRING_LENGTH];
	int exptolevel, expinlevel;
	
	if (IS_NPC(ch))
		return;
	
	sprintf(buf, "\r\n&b&4%s %s", GET_NAME(ch), GET_TITLE(ch));
	if (PLR_FLAGGED(ch, PLR_GHOST))
		strcat(buf, "&b&6 [Ghost]&0");
	strcat(buf, "\r\n");
	sprintf(buf + strlen(buf), "&0Level %-2d %-10s %-10s    &b&4Age:&0 %d years old\r\n", 
		GET_LEVEL(ch), pc_race_types[(int)GET_RACE(ch)], pc_class_types[(int)GET_CLASS(ch)], GET_AGE(ch));
	
	sprintf(buf + strlen(buf), "&b&4Str:&0 %2d                           &b&4Armor Class:&0 %d\r\n", GET_STR(ch), compute_armor_class(ch));
	sprintf(buf + strlen(buf), "&b&4Int:&0 %2d                           &b&4Hitroll:&0 %d\r\n", GET_INT(ch), GET_REAL_HITROLL(ch));
	sprintf(buf + strlen(buf), "&b&4Wis:&0 %2d                           &b&4Damroll:&0 %d\r\n", GET_WIS(ch), GET_REAL_DAMROLL(ch));
	sprintf(buf + strlen(buf), "&b&4Dex:&0 %2d\r\n", GET_DEX(ch));
	sprintf(buf + strlen(buf), "&b&4Con:&0 %2d                           &b&4Practice Sessions:&0 %d\r\n", GET_CON(ch), GET_PRACTICES(ch));
	sprintf(buf + strlen(buf), "&b&4Cha:&0 %2d                           &b&4Coins:&0 %d\r\n", GET_CHA(ch), get_gold(ch));
	send_to_char(buf, ch);

	if (GET_LEVEL(ch) == LVL_IMPL)
	{
		exptolevel = 0;
		expinlevel = 0;
	}
	else
	{
		expinlevel = GET_EXP(ch) - tot_exp_to_level(GET_LEVEL(ch));
		exptolevel = exp_to_level(GET_LEVEL(ch) + 1) - expinlevel;
	}
	
	sprintf(buf, "&b&3Experience Points:&0 %-7d        &b&3XP to level:&0 %d\r\n",
		GET_EXP(ch), exptolevel);
	send_to_char(buf, ch);

	if (GET_LEVEL(ch) == LVL_IMPL)
		strcpy(buf, make_bar(100, 100, 60, 6));
	else
		strcpy(buf, make_bar(expinlevel, exp_to_level(GET_LEVEL(ch) + 1), 60, 6));
	strcat(buf, "\r\n");
	send_to_char(buf, ch);

	strcpy(bbar, make_bar(GET_HIT(ch), GET_MAX_HIT(ch), 45, 2));
	sprintf(buf, "&b&4HP  :&0 %3d&b&4(&0%3d&b&4)&0 %s\r\n",
		GET_HIT(ch), GET_MAX_HIT(ch), bbar);
	send_to_char(buf, ch);

	strcpy(bbar, make_bar(GET_MANA(ch), GET_MAX_MANA(ch), 45, 2));
	sprintf(buf, "&b&4Mana:&0 %3d&b&4(&0%3d&b&4)&0 %s\r\n",
		GET_MANA(ch), GET_MAX_MANA(ch), bbar);
	send_to_char(buf, ch);

	strcpy(bbar, make_bar(GET_MOVE(ch), GET_MAX_MOVE(ch), 45, 2));
	sprintf(buf, "&b&4Move:&0 %3d&b&4(&0%3d&b&4)&0 %s\r\n",
		GET_MOVE(ch), GET_MAX_MOVE(ch), bbar);
	send_to_char(buf, ch);

	*buf = '\0';
	if (RIDING(ch))
	{
		sprintf(buf, "You are mounted upon %s.\r\n", GET_NAME(RIDING(ch)));
	}
	else if (WAGONER(ch))
	{
		sprintf(buf, "You are driving %s.\r\n", WAGONER(ch)->short_description);
	}
	else
	{
	switch (GET_POS(ch))
	{
	case POS_DEAD:
		strcat(buf, "&b&1You are DEAD!&0\r\n");
		break;
	case POS_MORTALLYW:
		strcat(buf, "&2You are &b&1mortally wounded&0&2!  You should seek help!&0\r\n");
		break;
	case POS_INCAP:
		strcat(buf, "&2You are &b&1incapacitated&0&2, slowly fading away...&0\r\n");
		break;
	case POS_STUNNED:
		strcat(buf, "&2You are &b&3stunned!&0  &2You can't move!&0\r\n");
		break;
	case POS_SLEEPING:
		strcat(buf, "&2You are&0 sleeping&2.&0\r\n");
		break;
	case POS_RESTING:
		if (AFF_FLAGGED(ch, AFF_MEMORIZING))
			strcat(buf, "&2You are&0 resting&2 while memorizing a spell.&0\r\n");
		else
			strcat(buf, "&2You are&0 resting&2.&0\r\n");
		break;
	case POS_SITTING:
		strcat(buf, "&2You are&0 sitting&2.&0\r\n");
		break;
	case POS_FIGHTING:
		if (FIGHTING(ch))
			sprintf(buf, "&2You are &b&6fighting&0 &b&3%s.&0\r\n", PERS(FIGHTING(ch), ch));
		else
			strcat(buf, "&2You are &b&6fighting&0 &2thin air.&0\r\n");
		break;
	case POS_STANDING:
		strcat(buf, "&2You are&0 standing&2.&0\r\n");
		break;
	default:
		strcat(buf, "&2You are &4floating&2.&0\r\n");
		break;
	}
	}

	if (GET_COND(ch, DRUNK) > 10)
		strcat(buf, "&b&3You are intoxicated&0&2.&0\r\n");
	
	if (GET_COND(ch, FULL) == 0)
		strcat(buf, "&b&6You are hungry.&0\r\n");
	
	if (GET_COND(ch, THIRST) == 0)
		strcat(buf, "&b&6You are thirsty.&0\r\n");

	send_to_char(buf, ch);
}

ACMD(do_attrib)
{
	TIME_INFO_DATA playing_time;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	int num, rmob;

	if (IS_NPC(ch))
		return;

	sprintf(buf, "\r\n&b&4%s %s", GET_NAME(ch), GET_TITLE(ch));
	if (PLR_FLAGGED(ch, PLR_GHOST))
		strcat(buf, "&b&6 [Ghost]");
	strcat(buf, "&0\r\n");
	sprintf(buf + strlen(buf), "Level: %d  Race: %s  Current Class: %-10s\r\n", 
		GET_LEVEL(ch), pc_race_types[(int)GET_RACE(ch)], pc_class_types[(int)GET_CLASS(ch)]);
	send_to_char(buf, ch);

	if (MULTICLASS(ch))
	{
		int c, found = 0;
		
		sprintf(buf, "Total Level: %d, Completed Classes:", GET_TOT_LEVEL(ch));
		for (c = 0; c < NUM_CLASSES; c++)
		{
			if (GET_CLASS(ch) == c) continue;
			
			if (MULTICLASSED(ch, (1 << c)))
			{
				sprinttype(c, pc_class_types, buf2);
				sprintf(buf+strlen(buf), "%s %s", found++ ? "," : "", buf2);
			}
		}
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
	}

	sprintf(buf, "&2You have killed&0 %d &2mobs and have been killed&0 %d &2times by mobs.&0\r\n",
		GET_MOB_KILLS(ch), GET_MOB_DEATHS(ch));
	sprintf(buf + strlen(buf), "&2You have killed&0 %d &2players and have been killed&0 %d &2times by other players.&0\r\n",
		GET_PLR_KILLS(ch), GET_PLR_DEATHS(ch));
	send_to_char(buf, ch);

	sprintf(buf, "&2Your weight is &0%d&2 stones, your global weight is &0%d&2 stones.&0\r\n", GET_WEIGHT(ch), GET_WEIGHT(ch) + IS_CARRYING_W(ch) );
	sprintf(buf + strlen(buf), "&2You are carrying&0 %d&2/&0%d &2items with a weight of&0 %d&2/&0%d &2stones.&0\r\n",
		IS_CARRYING_N(ch), CAN_CARRY_N(ch), IS_CARRYING_W(ch), CAN_CARRY_W(ch));
	send_to_char(buf, ch);
	
	playing_time = *real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);
	sprintf(buf, "&2You have been playing for&0 %d &2days and&0 %d &2hours.&0\r\n",
		playing_time.day, playing_time.hours);
	
	if      ( GET_ALIGNMENT(ch) >  900 ) num = 0;
	else if ( GET_ALIGNMENT(ch) >  700 ) num = 1;
	else if ( GET_ALIGNMENT(ch) >  350 ) num = 2;
	else if ( GET_ALIGNMENT(ch) >  100 ) num = 3;
	else if ( GET_ALIGNMENT(ch) > -100 ) num = 4;
	else if ( GET_ALIGNMENT(ch) > -350 ) num = 5;
	else if ( GET_ALIGNMENT(ch) > -700 ) num = 6;
	else if ( GET_ALIGNMENT(ch) > -900 ) num = 7;
	else                                 num = 8;
	
	sprintf(buf + strlen(buf), "&2You are&0 %s&2.&0\r\n", aligndesc[num]);

	/* now list spell affections */
	if (AFF_FLAGGED(ch, AFF_BLIND))
		strcat(buf, "&b&7You have been blinded!&0\r\n");
	
	if (AFF_FLAGGED(ch, AFF_INVISIBLE))
		strcat(buf, "&b&6You are invisible.&0\r\n");
	
	if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))
		strcat(buf, "&b&2You are sensitive to the presence of invisible things.&0\r\n");
	
	if (AFF_FLAGGED(ch, AFF_SANCTUARY))
		strcat(buf, "&b&7You are protected by Sanctuary.&0\r\n");
	
	if (AFF_FLAGGED(ch, AFF_POISON))
		strcat(buf, "&b&3You are poisoned!&0\r\n");
	
	if (AFF_FLAGGED(ch, AFF_CHARM))
		strcat(buf, "&b&1You have been charmed!&0\r\n");
	
	if (affected_by_spell(ch, SPELL_ARMOR))
		strcat(buf, "&b&2You feel protected.&0\r\n");
	
	if (AFF_FLAGGED(ch, AFF_INFRAVISION))
		strcat(buf, "&b&2Your eyes are glowing red.&0\r\n");

	if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
		strcat(buf, "&0You are summonable by other players.\r\n");

	if (PRF_FLAGGED(ch, PRF_NOGREET))
		strcat(buf, "&0You are not interested in making new knowings.\r\n");
	
	send_to_char(buf, ch);

	strcpy(buf, "You have killed these mobs (last 100 max):\r\n");
	*buf2 = '\0';
	for (num = 1; num < 101; num++)
	{
		if (GET_KILLS_VNUM(ch, num) == 0)
			continue;

		if ((rmob = real_mobile(GET_KILLS_VNUM(ch, num))) == NOBODY)
			continue;

		sprintf(buf2+strlen(buf2), "  %s [%d]\r\n",
			GET_NAME(mob_proto + rmob), GET_KILLS_AMOUNT(ch, num));
	}

	if (*buf2)
	{
		strcat(buf, buf2);
		if (buf[strlen(buf)-1] != '\n')
			strcat(buf, "\r\n");
		page_string(ch->desc, buf, 1);
	}
}


ACMD(do_inventory)
{
	send_to_char("You are carrying:\r\n", ch);
	list_obj_to_char(ch->first_carrying, ch, 4, TRUE, FALSE);
}


ACMD(do_equipment)
{
	int i, found = 0;
	
	send_to_char("You are using:\r\n", ch);
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
		{
			if (CAN_SEE_OBJ(ch, GET_EQ(ch, i)))
			{
				send_to_char(wear_where[i], ch);
				show_obj_to_char(GET_EQ(ch, i), ch, 3, FALSE);
				found = TRUE;
			}
			else
			{
				send_to_char(wear_where[i], ch);
				send_to_char("Something.\r\n", ch);
				found = TRUE;
			}
		}
	}

	if (!found)
		send_to_char(" Nothing.\r\n", ch);
}


ACMD(do_time)
{
	const char *suf;
	int weekday, day;
	
	sprintf(buf, "It is %d o'clock %s, on ",
		((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)),
		((time_info.hours >= 12) ? "pm" : "am"));
	
	day = time_info.day + 1;	/* day in [1..35] */
	
	/* 35 days in a month, 7 days a week */
	weekday = ((35 * time_info.month) + day) % 7;
	
	strcat(buf, weekdays[weekday]);
	strcat(buf, "\r\n");
	send_to_char(buf, ch);
	
	/*
	 * Peter Ajamian <peter@PAJAMIAN.DHS.ORG> supplied the following as a fix
	 * for a bug introduced in the ordinal display that caused 11, 12, and 13
	 * to be incorrectly displayed as 11st, 12nd, and 13rd.  Nate Winters
	 * <wintersn@HOTMAIL.COM> had already submitted a fix, but it hard-coded a
	 * limit on ordinal display which I want to avoid.	-dak
	 */
	
	suf = "th";
	
	if (((day % 100) / 10) != 1)
	{
		switch (day % 10)
		{
		case 1:
			suf = "st";
			break;
		case 2:
			suf = "nd";
			break;
		case 3:
			suf = "rd";
			break;
		}
	}
	
	sprintf(buf, "The %d%s Day of the %s, Year %d.\r\n",
		day, suf, month_name[time_info.month], time_info.year);
	
	send_to_char(buf, ch);
}


ACMD(do_weather)
{
	WEATHER_DATA *cond;

	if (!OUTSIDE(ch))
	{
		send_to_char("You have no feeling about the weather at all.\r\n", ch);
		return;
	}

	if (!(cond = get_room_weather(ch->in_room)))
		return;

	if (time_info.hours < 5 || time_info.hours > 23)
	{
		if (cond->precip_rate < 30 && cond->humidity < 55)
		{
			if (MoonPhase != MOON_NEW)
				ch_printf(ch, "The nocturnal sky, full of stars, is graced by a %s.\r\n", moon_look[MoonPhase]);
			else
				send_to_char("Only stars in the sky, the moon is a black shape that cover some of them.\r\n", ch);
		}
	}

	*buf = '\0';
	if (cond->precip_rate)
	{
		if (cond->temp <= 0)
			strcat(buf, "It's snowing");
		else
			strcat(buf, "It's raining");

		if	(cond->precip_rate > 65) strcat(buf, " extremely hard");
		else if (cond->precip_rate > 50) strcat(buf, " very hard");
		else if (cond->precip_rate > 30) strcat(buf, " hard");
		else if (cond->precip_rate < 15) strcat(buf, " lightly");
		strcat(buf, ", ");
	}
	else
	{
		if	(cond->humidity > 80)	strcat(buf, "It's very cloudy, ");
		else if (cond->humidity > 55)	strcat(buf, "It's cloudy, ");
		else if (cond->humidity > 25)	strcat(buf, "It's partly cloudy, ");
		else if (cond->humidity)	strcat(buf, "It's mostly clear, ");
		else				strcat(buf, "It's clear, ");
	}

	strcat(buf, "the air is ");
	if	(cond->temp > 60)		strcat(buf, "boiling, ");
	else if (cond->temp > 52)		strcat(buf, "incredibly hot, ");
	else if (cond->temp > 37)		strcat(buf, "very, very hot, ");
	else if (cond->temp > 29)		strcat(buf, "very hot, ");
	else if (cond->temp > 25)		strcat(buf, "hot, ");
	else if (cond->temp > 18)		strcat(buf, "warm, ");
	else if (cond->temp > 9)		strcat(buf, "mild, ");
	else if (cond->temp > 1)		strcat(buf, "cool, ");
	else if (cond->temp > -5)		strcat(buf, "cold, ");
	else if (cond->temp > -10)		strcat(buf, "freezing, ");
	else if (cond->temp > -20)		strcat(buf, "well past freezing, ");
	else					strcat(buf, "numbingly frozen, ");
	
	strcat(buf, "and ");
	if (cond->windspeed <= 0)		strcat(buf, "there is absolutely no wind");
	else if (cond->windspeed < 10)		strcat(buf, "calm");
	else if (cond->windspeed < 20)		strcat(buf, "breezy");
	else if (cond->windspeed < 35)		strcat(buf, "windy");
	else if (cond->windspeed < 50)		strcat(buf, "very windy");
	else if (cond->windspeed < 70)		strcat(buf, "very, very windy");
	else if (cond->windspeed < 100)		strcat(buf, "there is a gale blowing");
	else					strcat(buf, "the wind is unbelievable");
	strcat(buf, ".\r\n");
	send_to_char(buf, ch);

	if (IS_IMMORTAL(ch))
		ch_printf(ch, "PR: %d  Hm: %d  Tm: %d  Wn: %d  En: %d\r\n",
			cond->precip_rate, cond->humidity, cond->temp, cond->windspeed,
			cond->free_energy);

	if (CASTING_CLASS(ch) || MEMMING_CLASS(ch))
	{
		if (cond->free_energy > 40000)
			send_to_char("Wow! This place is bursting with energy!\r\n", ch);
		else if (cond->free_energy > 30000)
			send_to_char("The environs tingle your magical senses.\r\n", ch);
		else if (cond->free_energy > 20000)
			send_to_char("The area is rich with energy.\r\n", ch);
		else if (cond->free_energy < 5000)
			send_to_char("There is almost no magical energy here.\r\n", ch);
		else if (cond->free_energy < 1000)
			send_to_char("Your magical senses are dulled by the scarceness of energy here.\r\n", ch);
	}
}


ACMD(do_help)
{
	int chk, bot, top, mid, minlen;
	
	if (!ch->desc)
		return;
	
	skip_spaces(&argument);
	
	if (!*argument)
	{
		page_string(ch->desc, help, 0);
		return;
	}
	if (!help_table)
	{
		send_to_char("No help available.\r\n", ch);
		return;
	}
	
	bot = 0;
	top = top_of_helpt;
	minlen = strlen(argument);
	
	for (;;)
	{
		mid = (bot + top) / 2;
		
		if (bot > top)
		{
			send_to_char("There is no help on that word.\r\n", ch);
			return;
		}
		else if (!(chk = strn_cmp(argument, help_table[mid].keyword, minlen)))
		{
			/* trace backwards to find first matching entry. Thanks Jeff Fink! */
			while ((mid > 0) &&
				(!(chk = strn_cmp(argument, help_table[mid - 1].keyword, minlen))))
				mid--;
			page_string(ch->desc, help_table[mid].entry, 0);
			return;
		}
		else
		{
			if (chk > 0)
				bot = mid + 1;
			else
				top = mid - 1;
		}
	}
}



/* ******************* W H O *********************** */

char *which_color( CHAR_DATA *ch )
{
	if (GET_LEVEL(ch) >= LVL_IMMORT)	strcpy(buf2, "&b&1");
	else if (GET_LEVEL(ch) > 20)		strcpy(buf2, "&b&3");
	else if (GET_LEVEL(ch) > 10)		strcpy(buf2, "&b&2");
	else								strcpy(buf2, "&b&5");

	return (buf2);
}

char *which_abbr( CHAR_DATA *ch )
{
	if		(GET_LEVEL(ch) == LVL_IMPL)		strcpy(buf3, "IMP");
	else if (GET_LEVEL(ch) > LVL_IMMORT)	strcpy(buf3, "GOD");
	else if (GET_LEVEL(ch) == LVL_IMMORT)	strcpy(buf3, "AVT");
	else									strcpy(buf3, CLASS_ABBR(ch));

	return (buf3);
}



WHO_LIST *add_to_who( WHO_LIST *head, char *str, CHAR_DATA *ch )
{
	WHO_LIST *tmp, *prev = NULL, *to_add = NULL;
	
	if (str && ch)
	{
		CREATE(to_add, WHO_LIST, 1);
		to_add->name = str_dup(str);
		to_add->level = GET_TOT_LEVEL(ch);
		to_add->next = NULL;
	}
	else
	{
		log("SYSERR: NULL str or ch in add_to_who");
		return (NULL);
	}
	
	if (!head)
		return (to_add);
	
	for (tmp = head; tmp; tmp = tmp->next)
	{
		if (to_add->level > tmp->level)   /* ok, add record */
		{
			if (prev)
				prev->next = to_add;
			if (head == tmp)
				head = to_add;
			to_add->next = tmp;
			return ( head );
		}
		prev = tmp;
	}
	prev->next = to_add;
	return ( head );
}


void output_who( WHO_LIST *head, CHAR_DATA *ch )
{
	WHO_LIST *tmp, *next;
	char elenco[MAX_STRING_LENGTH];
	
	if ( !head )
	{
		log("SYSERR: output_who: hey, no head?");
		return;
	}
	
	strcpy(elenco, "\r\nPlayers\r\n----------------------------------------------------------------\r\n");
	
	for (tmp = head; tmp; tmp = next)
	{
		next = tmp->next;
		strcat(elenco, tmp->name);
		
		if (!tmp || !tmp->name)
			log("SYSERR: output_who: trying to free invalid tmp->name");
		else
		{
			free(tmp->name);
		}
		if (!tmp)
			log("SYSERR: output_who: trying to free invalid tmp struct");
		else
		{
			free(tmp);
		}
	}
	
	page_string(ch->desc, elenco, 1);
}


#define WHO_FORMAT \
"format: who [minlev[-maxlev]] [-n name] [-c classlist] [-r racelist] [-o] [-q] [-z]\r\n"

ACMD(do_who)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *tch;
	WHO_LIST *who_head = NULL;
	char name_search[MAX_INPUT_LENGTH], clan_num[5];
	char mode;
	size_t i;
	int low = 0, high = LVL_IMPL, localwho = 0, questwho = 0;
	int showclass = 0, showrace = 0, outlaws = 0, num_can_see = 0;
	int who_room = 0;
		
	skip_spaces(&argument);
	strcpy(buf, argument);
	name_search[0] = '\0';
	clan_num[0] = '\0';
	
	while (*buf)
	{
		half_chop(buf, arg, buf1);
		if (isdigit(*arg))
		{
			sscanf(arg, "%d-%d", &low, &high);
			strcpy(buf, buf1);
		}
		else if (*arg == '-')
		{
			mode = *(arg + 1);       // just in case; we destroy arg in the switch
			switch (mode)
			{
			case 'o':
				outlaws = 1;
				strcpy(buf, buf1);
				break;
			case 'z':
				localwho = 1;
				strcpy(buf, buf1);
				break;
			case 'q':
				questwho = 1;
				strcpy(buf, buf1);
				break;
			case 'l':
				half_chop(buf1, arg, buf);
				sscanf(arg, "%d-%d", &low, &high);
				break;
			case 'n':
				half_chop(buf1, name_search, buf);
				break;
			case 'r':
				half_chop(buf1, arg, buf);
				for (i = 0; i < strlen(arg); i++)
					showrace |= find_race_bitvector(arg[i]);
				break;
			case 'c':
				half_chop(buf1, arg, buf);
				for (i = 0; i < strlen(arg); i++)
					showclass |= find_class_bitvector(arg[i]);
				break;
			default:
				send_to_char(WHO_FORMAT, ch);
				return;
			}				// end of switch
			
		}
		else
		{
			send_to_char(WHO_FORMAT, ch);
			return;
		}
	}
	
	for (d = descriptor_list; d; d = d->next)
	{
		if (STATE(d) != CON_PLAYING)
			continue;
		
		if (d->original)
			tch = d->original;
		else if (!(tch = d->character))
			continue;
		
		if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
			!strstr(GET_TITLE(tch), name_search))
			continue;
		if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
			continue;
		if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) && !PLR_FLAGGED(tch, PLR_THIEF))
			continue;
		if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
			continue;
		if (localwho && ch->in_room->zone != tch->in_room->zone)
			continue;
		if (who_room && (tch->in_room != ch->in_room))
			continue;
		if (showclass && !(showclass & (1 << GET_CLASS(tch))))
			continue;
		if (showrace && !(showrace & (1 << GET_RACE(tch))))
			continue;
		
		num_can_see++;
		
		sprintf(buf, "%s[%3d %2d %s %s]&0 &b&6%s",
				which_color(tch),
				GET_TOT_LEVEL(tch),
				GET_LEVEL(tch),
				which_abbr(tch),
				(GET_SEX(tch) == SEX_FEMALE ? "F" : "M"),
				PERS(tch,ch));

		if (GET_INVIS_LEV(tch))
			sprintf(buf + strlen(buf), " [i%d]", GET_INVIS_LEV(tch));
		else if (AFF_FLAGGED(tch, AFF_INVISIBLE))
			strcat(buf, " [invisible]");

		if (IN_WILD(tch))
			strcat(buf, " [Wilderness]");
		
		if (FIGHTING(tch))
			strcat(buf, " [Fighting]");
		
		if (AFF_FLAGGED(tch, AFF_GROUP))
			strcat(buf, " [In Group]");
		
		if (PLR_FLAGGED(tch, PLR_MAILING))
			strcat(buf, " [Mailing]");
		
		if (PLR_FLAGGED(tch, PLR_WRITING))
			strcat(buf, " [Writing]");

		if (PLR_FLAGGED(tch, PLR_GHOST))
			strcat(buf, " [Ghost]");
		
		if (PRF_FLAGGED(tch, PRF_DEAF))
			strcat(buf, " [Deaf]");
		if (PRF_FLAGGED(tch, PRF_NOTELL))
			strcat(buf, " [Notell]");
		if (PRF_FLAGGED(tch, PRF_QUEST))
			strcat(buf, " [Quest]");
		if (PLR_FLAGGED(tch, PLR_THIEF))
			strcat(buf, " [THIEF]");
		if (PLR_FLAGGED(tch, PLR_KILLER))
			strcat(buf, " [KILLER]");
		
		strcat(buf, "&0\r\n");
		
		who_head = add_to_who(who_head, buf, tch);
	}
	
	if (who_head)
		output_who(who_head, ch);
	
	strcpy(buf, "----------------------------------------------------------------");
	
	if (num_can_see == 0)
		sprintf(buf + strlen(buf), "\r\nNo-one at all!\r\n");
	else if (num_can_see == 1)
		sprintf(buf + strlen(buf), "\r\n&b&7One lonely character displayed.&0\r\n");
	else
		sprintf(buf + strlen(buf), "\r\n&b&7[%d] Total players visible to you.&0\r\n", num_can_see);
	
	if (num_can_see > boot_high)
		boot_high = num_can_see;
	
	sprintf(buf + strlen(buf), "&b&1Today's Record:&0 &b&7%d&0 &b&4 Players Online.&0\r\n", boot_high);
	
	page_string(ch->desc, buf, 1);
	
}
/* ******************* W H O *********************** */


#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-c classlist] [-o] [-p]\r\n"

ACMD(do_users)
{
	CHAR_DATA *tch;
	DESCRIPTOR_DATA *d;
	char line[200], line2[220], idletime[10], classname[20];
	char state[30], *timeptr, mode;
	char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
	size_t i;
	int low = 0, high = LVL_IMPL, num_can_see = 0;
	int showclass = 0, outlaws = 0, playing = 0, deadweight = 0;
	
	host_search[0] = name_search[0] = '\0';
	
	strcpy(buf, argument);
	while (*buf)
	{
		half_chop(buf, arg, buf1);
		if (*arg == '-')
		{
			mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
			switch (mode)
			{
			case 'o':
			case 'k':
				outlaws = 1;
				playing = 1;
				strcpy(buf, buf1);
				break;
			case 'p':
				playing = 1;
				strcpy(buf, buf1);
				break;
			case 'd':
				deadweight = 1;
				strcpy(buf, buf1);
				break;
			case 'l':
				playing = 1;
				half_chop(buf1, arg, buf);
				sscanf(arg, "%d-%d", &low, &high);
				break;
			case 'n':
				playing = 1;
				half_chop(buf1, name_search, buf);
				break;
			case 'h':
				playing = 1;
				half_chop(buf1, host_search, buf);
				break;
			case 'c':
				playing = 1;
				half_chop(buf1, arg, buf);
				for (i = 0; i < strlen(arg); i++)
					showclass |= find_class_bitvector(arg[i]);
				break;
			default:
				send_to_char(USERS_FORMAT, ch);
				return;
			}				/* end of switch */
			
		}
		else
		{
			send_to_char(USERS_FORMAT, ch);
			return;
		}
	}

	strcpy(line,
		"Num Class   Name         State          Idl Login@   Site\r\n");
	strcat(line,
		"--- ------- ------------ -------------- --- -------- ------------------------\r\n");
	send_to_char(line, ch);
	
	one_argument(argument, arg);
	
	for (d = descriptor_list; d; d = d->next)
	{
		if (STATE(d) != CON_PLAYING && playing)
			continue;
		if (STATE(d) == CON_PLAYING && deadweight)
			continue;
		if (STATE(d) == CON_PLAYING)
		{
			if (d->original)
				tch = d->original;
			else if (!(tch = d->character))
				continue;
			
			if (*host_search && !strstr(d->host, host_search))
				continue;
			if (*name_search && str_cmp(GET_NAME(tch), name_search))
				continue;
			if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
				continue;
			if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) && !PLR_FLAGGED(tch, PLR_THIEF))
				continue;
			if (showclass && !(showclass & (1 << GET_CLASS(tch))))
				continue;
			if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
				continue;
			
			if (d->original)
				sprintf(classname, "[%2d %s]", GET_LEVEL(d->original),
					CLASS_ABBR(d->original));
			else
				sprintf(classname, "[%2d %s]", GET_LEVEL(d->character),
					CLASS_ABBR(d->character));
		}
		else
			strcpy(classname, "   -   ");
		
		timeptr = asctime(localtime(&d->login_time));
		timeptr += 11;
		*(timeptr + 8) = '\0';
		
		if (STATE(d) == CON_PLAYING && d->original)
			strcpy(state, "Switched");
		else
			strcpy(state, connected_types[STATE(d)]);
		
		if (d->character && STATE(d) == CON_PLAYING && GET_LEVEL(d->character) < LVL_GOD)
			sprintf(idletime, "%3d", d->character->player.timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
		else
			strcpy(idletime, "");
		
		sprintf(line, "%3d %-7s %-12s %-14s %-3s %-8s ", d->desc_num, classname,
			d->original && GET_PC_NAME(d->original) ? GET_PC_NAME(d->original) :
		d->character && GET_PC_NAME(d->character) ? GET_PC_NAME(d->character) :
		"UNDEFINED",
			state, idletime, timeptr);
		
		if (d->host && *d->host)
			sprintf(line + strlen(line), "[%s]\r\n", d->host);
		else
			strcat(line, "[Hostname unknown]\r\n");
		
		if (STATE(d) != CON_PLAYING)
		{
			sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
			strcpy(line, line2);
		}
		if (STATE(d) != CON_PLAYING ||
			(STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character)))
		{
			send_to_char(line, ch);
			num_can_see++;
		}
	}
	
	sprintf(line, "\r\n%d visible sockets connected.\r\n", num_can_see);
	send_to_char(line, ch);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
	switch (subcmd)
	{
	case SCMD_CREDITS:
		page_string(ch->desc, credits, 0);
		break;
	case SCMD_NEWS:
		page_string(ch->desc, news, 0);
		break;
	case SCMD_INFO:
		page_string(ch->desc, info, 0);
		break;
	case SCMD_WIZLIST:
		page_string(ch->desc, wizlist, 0);
		break;
	case SCMD_IMMLIST:
		page_string(ch->desc, immlist, 0);
		break;
	case SCMD_HANDBOOK:
		page_string(ch->desc, handbook, 0);
		break;
	case SCMD_POLICIES:
		page_string(ch->desc, policies, 0);
		break;
	case SCMD_MOTD:
		page_string(ch->desc, motd, 0);
		break;
	case SCMD_IMOTD:
		page_string(ch->desc, imotd, 0);
		break;
	case SCMD_CLEAR:
		send_to_char("\033[H\033[J", ch);
		break;
	case SCMD_VERSION:
		ch_printf(ch, "%s\r\n", circlemud_version);
		//send_to_char(strcat(strcpy(buf, circlemud_version), "\r\n"), ch);
		break;
	case SCMD_WHOAMI:
		ch_printf(ch, "%s\r\n", GET_NAME(ch));
		//send_to_char(strcat(strcpy(buf, GET_NAME(ch)), "\r\n"), ch);
		break;
	default:
		log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
		return;
	}
}


void perform_mortal_where(CHAR_DATA *ch, char *arg)
{
	CHAR_DATA *i;
	DESCRIPTOR_DATA *d;
	
	if ( IN_WILD(ch) )
	{
		send_to_char("Not in the wilderness.\r\n", ch);
		return;
	}

	if (!*arg)
	{
		send_to_char("Players in your Zone\r\n--------------------\r\n", ch);
		for (d = descriptor_list; d; d = d->next)
		{
			if (STATE(d) != CON_PLAYING || d->character == ch)
				continue;
			if ((i = (d->original ? d->original : d->character)) == NULL)
				continue;
			if (i->in_room == NULL || !CAN_SEE(ch, i))
				continue;
			if (ch->in_room->zone != i->in_room->zone)
				continue;
			sprintf(buf, "%-20s - %s\r\n", PERS(i,ch), ROOM_NAME(i));
			send_to_char(buf, ch);
		}
	}
	else
	{			/* print only FIRST char, not all. */
		for (i = character_list; i; i = i->next)
		{
			if (i->in_room == NULL || i == ch)
				continue;
			if (!CAN_SEE(ch, i) || i->in_room->zone != ch->in_room->zone)
				continue;
			if (!isname(arg, take_name(i, ch)))
				continue;
			sprintf(buf, "%-25s - %s\r\n", PERS(i,ch), ROOM_NAME(i) );
			send_to_char(buf, ch);
			return;
		}
		send_to_char("No-one around by that name.\r\n", ch);
	}
}


void print_object_location(int num, OBJ_DATA *obj, CHAR_DATA *ch, int recur)
{
	if (num > 0)
		sprintf(buf, "O%3d. %-25s - ", num, obj->short_description);
	else
		sprintf(buf, "%33s", " - ");
	
	if (obj->in_room)
	{
		if ( IN_WILD(obj) )
			sprintf(buf + strlen(buf), "[%5d %5d] %s\r\n",
				GET_Y(obj), GET_X(obj), ROOM_NAME(obj) );
		else
			sprintf(buf + strlen(buf), "[%5d] %s\r\n",
				obj->in_room->number, ROOM_NAME(obj) );
		send_to_char(buf, ch);
	}
	else if (obj->carried_by)
	{
		sprintf(buf + strlen(buf), "carried by %s\r\n",
			PERS(obj->carried_by, ch));
		send_to_char(buf, ch);
	}
	else if (obj->worn_by)
	{
		sprintf(buf + strlen(buf), "worn by %s\r\n",
			PERS(obj->worn_by, ch));
		send_to_char(buf, ch);
	}
	else if (obj->in_obj)
	{
		sprintf(buf + strlen(buf), "inside %s%s\r\n",
			obj->in_obj->short_description, (recur ? ", which is" : " "));
		send_to_char(buf, ch);
		if (recur)
			print_object_location(0, obj->in_obj, ch, recur);
	}
	else
	{
		sprintf(buf + strlen(buf), "in an unknown location\r\n");
		send_to_char(buf, ch);
	}
}



void perform_immort_where(CHAR_DATA *ch, char *arg)
{
	CHAR_DATA *i;
	OBJ_DATA *k;
	DESCRIPTOR_DATA *d;
	int num = 0, found = 0;
	
	if (!*arg)
	{
		send_to_char("Players\r\n-------\r\n", ch);
		for (d = descriptor_list; d; d = d->next)
		{
			if (STATE(d) == CON_PLAYING)
			{
				i = (d->original ? d->original : d->character);
				if (i && CAN_SEE(ch, i) && (i->in_room != NULL))
				{
					char rnumdes[128];

					if ( IN_WILD( d->character ) )
						sprintf( rnumdes, "[%5d %5d]",
							GET_Y(d->character), GET_X(d->character) );
					else
						sprintf( rnumdes, "[%5d]", d->character->in_room->number );

					if (d->original)
						sprintf(buf, "%-20s - %s %s (in %s)\r\n",
							GET_NAME(i), rnumdes,
							ROOM_NAME(d->character), GET_NAME(d->character));
					else
						sprintf(buf, "%-20s - %s %s\r\n", GET_NAME(i),
							rnumdes, ROOM_NAME(i) );
					send_to_char(buf, ch);
				}
			}
		}
	}
	else
	{
		for (i = character_list; i; i = i->next)
		{
			if (CAN_SEE(ch, i) && i->in_room != NULL && isname(arg, GET_PC_NAME(i)))
			{
				found = 1;

				if ( IN_WILD(i) )
				{
					sprintf(buf, "M%3d. %-25s - [%5d %5d] %s\r\n", ++num, GET_NAME(i),
						GET_Y(i), GET_X(i), ROOM_NAME(i) );
				}
				else
				{
					sprintf(buf, "M%3d. %-25s - [%5d] %s\r\n", ++num, GET_NAME(i),
						i->in_room->number, ROOM_NAME(i) );
				}
				send_to_char(buf, ch);
			}
		}
			
		for (num = 0, k = first_object; k; k = k->next)
		{
			if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name))
			{
				found = 1;
				print_object_location(++num, k, ch, TRUE);
			}
		}
			
		if (!found)
			send_to_char("Couldn't find any such thing.\r\n", ch);
	}
}



ACMD(do_where)
{
	one_argument(argument, arg);
	
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		perform_immort_where(ch, arg);
	else
		perform_mortal_where(ch, arg);
}



ACMD(do_levels)
{
	int i;
	
	if (IS_NPC(ch))
	{
		send_to_char("You ain't nothin' but a hound-dog.\r\n", ch);
		return;
	}
	*buf = '\0';
	
	for (i = 1; i < LVL_IMMORT; i++)
	{
		sprintf(buf + strlen(buf), "[%2d] %8d: ", i, exp_to_level(i));
		switch (GET_SEX(ch))
		{
		case SEX_MALE:
		case SEX_NEUTRAL:
			strcat(buf, title_male(GET_CLASS(ch), i));
			break;
		case SEX_FEMALE:
			strcat(buf, title_female(GET_CLASS(ch), i));
			break;
		default:
			send_to_char("Oh dear.  You seem to be sexless.\r\n", ch);
			break;
		}
		strcat(buf, "\r\n");
	}
	/*
	sprintf(buf + strlen(buf), "[%2d] %8d          : Immortality\r\n",
		LVL_IMMORT, level_exp(GET_CLASS(ch), LVL_IMMORT));
	*/
	page_string(ch->desc, buf, 1);
}


ACMD(do_consider)
{
	CHAR_DATA *victim;
	int diff;
	
	one_argument(argument, buf);
	
	if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
	{
		send_to_char("Consider killing who?\r\n", ch);
		return;
	}
	if (victim == ch)
	{
		send_to_char("Easy!  Very easy indeed!\r\n", ch);
		return;
	}
	if (!IS_NPC(victim))
	{
		send_to_char("Would you like to borrow a cross and a shovel?\r\n", ch);
		return;
	}
	diff = (GET_LEVEL(victim) - GET_LEVEL(ch));
	
	if (diff <= -10)
		send_to_char("Now where did that chicken go?\r\n", ch);
	else if (diff <= -5)
		send_to_char("You could do it with a needle!\r\n", ch);
	else if (diff <= -2)
		send_to_char("Easy.\r\n", ch);
	else if (diff <= -1)
		send_to_char("Fairly easy.\r\n", ch);
	else if (diff == 0)
		send_to_char("The perfect match!\r\n", ch);
	else if (diff <= 1)
		send_to_char("You would need some luck!\r\n", ch);
	else if (diff <= 2)
		send_to_char("You would need a lot of luck!\r\n", ch);
	else if (diff <= 3)
		send_to_char("You would need a lot of luck and great equipment!\r\n", ch);
	else if (diff <= 5)
		send_to_char("Do you feel lucky, punk?\r\n", ch);
	else if (diff <= 10)
		send_to_char("Are you mad!?\r\n", ch);
	else if (diff <= 100)
		send_to_char("You ARE mad!\r\n", ch);
}



ACMD(do_diagnose)
{
	CHAR_DATA *vict;
	
	one_argument(argument, buf);
	
	if (*buf)
	{
		if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
			send_to_char(NOPERSON, ch);
		else
			diag_char_to_char(vict, ch);
	}
	else
	{
		if (FIGHTING(ch))
			diag_char_to_char(FIGHTING(ch), ch);
		else
			send_to_char("Diagnose who?\r\n", ch);
	}
}


ACMD(do_color)
{
	int tp;
	
	if (IS_NPC(ch))
		return;
	
	one_argument(argument, arg);
	
	if (!*arg)
	{
		sprintf(buf, "Your current color level is %s.\r\n", ctypes[COLOR_LEV(ch)]);
		send_to_char(buf, ch);
		return;
	}
	if (((tp = search_block(arg, ctypes, FALSE)) == -1))
	{
		send_to_char("Usage: color { Off | Sparse | Normal | Complete }\r\n", ch);
		return;
	}
	REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1 | PRF_COLOR_2);
	SET_BIT(PRF_FLAGS(ch), (PRF_COLOR_1 * (tp & 1)) | (PRF_COLOR_2 * (tp & 2) >> 1));
	
	sprintf(buf, "Your %scolor%s is now %s.\r\n", CCRED(ch, C_SPR),
		CCNRM(ch, C_OFF), ctypes[tp]);
	send_to_char(buf, ch);
}


ACMD(do_toggle)
{
	if (IS_NPC(ch))
		return;

	if (GET_WIMP_LEV(ch) == 0)
		strcpy(buf2, "OFF");
	else
		sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));
	
	if (GET_LEVEL(ch) >= LVL_IMMORT)
	{
		sprintf(buf,
			"      No Hassle: %-3s    "
			"      Holylight: %-3s    "
			"     Room Flags: %-3s\r\n",
			ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
			ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
			ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS))
			);
		send_to_char(buf, ch);
	}
	
	sprintf(buf,
		"Hit Pnt Display: %-3s    "
		"     Brief Mode: %-3s    "
		" Summon Protect: %-3s\r\n"
		
		"   Move Display: %-3s    "
		"   Compact Mode: %-3s    "
		"       On Quest: %-3s\r\n"
		
		"   Mana Display: %-3s    "
		"         NoTell: %-3s    "
		"   Repeat Comm.: %-3s\r\n"
		
		" Auto Show Exit: %-3s    "
		"           Deaf: %-3s    "
		"     Wimp Level: %-3s\r\n"
		
		" Gossip Channel: %-3s    "
		"Auction Channel: %-3s    "
		"  Grats Channel: %-3s\r\n"
		
		"   Wild as Text: %-3s    "
		"  Wild no color: %-3s    "
		"     Wild Small: %-3s\r\n"
		
		"   Allow Greets: %-3s    "
		"    Color Level: %-3s\r\n",

		ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
		ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
		ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),
		
		ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
		ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
		YESNO(PRF_FLAGGED(ch, PRF_QUEST)),
		
		ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)),
		ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
		YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),
		
		ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)),
		YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
		buf2,
		
		ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
		ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
		ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),
		
		YESNO(PRF_FLAGGED(ch, PRF_WILDTEXT)),
		YESNO(PRF_FLAGGED(ch, PRF_WILDBLACK)),
		ONOFF(PRF_FLAGGED(ch, PRF_WILDSMALL)),

		YESNO(!PRF_FLAGGED(ch, PRF_NOGREET)),
		ctypes[COLOR_LEV(ch)]);
	
	send_to_char(buf, ch);
}


int sort_commands_helper(const void *a, const void *b)
{
	return (strcmp(cmd_info[*(const int *)a].command, cmd_info[*(const int *)b].command));
}


void sort_commands(void)
{
	int a, num_of_cmds = 0;
	
	while (cmd_info[num_of_cmds].command[0] != '\n')
		num_of_cmds++;
	num_of_cmds++;	/* \n */
	
	CREATE(cmd_sort_info, int, num_of_cmds);
	
	for (a = 0; a < num_of_cmds; a++)
		cmd_sort_info[a] = a;
	
	/* Don't sort the RESERVED or \n entries. */
	qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}


ACMD(do_commands)
{
	int no, i, cmd_num;
	int wizhelp = 0, socials = 0;
	CHAR_DATA *vict;
	
	one_argument(argument, arg);
	
	if (*arg)
	{
		if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) || IS_NPC(vict))
		{
			send_to_char("Who is that?\r\n", ch);
			return;
		}
		if (GET_LEVEL(ch) < GET_LEVEL(vict))
		{
			send_to_char("You can't see the commands of people above your level.\r\n", ch);
			return;
		}
	}
	else
		vict = ch;
	
	if (subcmd == SCMD_SOCIALS)
		socials = 1;
	else if (subcmd == SCMD_WIZHELP)
		wizhelp = 1;
	
	sprintf(buf, "The following %s%s are available to %s:\r\n",
		wizhelp ? "privileged " : "",
		socials ? "socials" : "commands",
		vict == ch ? "you" : PERS(vict,ch));
	
	/* cmd_num starts at 1, not 0, to remove 'RESERVED' */
	for (no = 1, cmd_num = 1; cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++)
	{
		i = cmd_sort_info[cmd_num];
		
		if (cmd_info[i].minimum_level < 0 || GET_LEVEL(vict) < cmd_info[i].minimum_level)
			continue;
		
		if ((cmd_info[i].minimum_level >= LVL_IMMORT) != wizhelp)
			continue;
		
		if (!wizhelp && socials != (cmd_info[i].command_pointer == do_action || cmd_info[i].command_pointer == do_insult))
			continue;
		
		sprintf(buf + strlen(buf), "%-11s%s", cmd_info[i].command, no++ % 7 == 0 ? "\r\n" : "");
	}
	
	if (no % 7 != 1)
		strcat(buf, "\r\n");
	send_to_char(buf, ch);
}

/* ********************** S C A N ********************** */

const char *how_far[] =
{
  "immediate area",
  "close by",
  "a ways off",
  "far to the",
  "way far to the",
  "way way far to the",
  "a damn long ways off to the",
  "\n"
};

int list_scanned_chars_to_char(CHAR_DATA *list, CHAR_DATA *ch, int dir, int dist)
{
	CHAR_DATA *i;
	char buf[MAX_STRING_LENGTH];
	int count = 0;
	
	for (i = list; i; i = i->next_in_room) 
	{
		if (CAN_SEE(ch, i) && (i != ch))
			count++;
	}
		
	if (!count)
		return (0);
	
	if (dist > 6) dist = 6;

	sprintf(buf, "  %s %s you see:", how_far[dist], (dir >= 0 && dir < NUM_OF_DIRS ? dirs[dir] : ""));
	for (i = list; i; i = i->next_in_room)
	{
		if (!CAN_SEE(ch, i) || (i==ch))
			continue;

		sprintf(buf + strlen(buf), " %s", PERS(i, ch));
	}
	strcat(buf, ".");

	act(buf,FALSE, ch, 0, 0, TO_CHAR);
	return (count);
}


void block_view(CHAR_DATA *ch, char *str, int dir)
{
	if ((str == NULL) || (*str == '\0'))
		return;
	
	sprintf(buf1, "A %-12.12s %-23.16s %s", 
		str, "blocks your view", (dir == 5) ? "below" : dirs[dir]);
	act(buf1,FALSE, ch, 0,0, TO_CHAR);
}


ACMD(do_scan)
{
	EXIT_DATA *pexit;
	int dir, count= 0;
	
	if (AFF_FLAGGED(ch, AFF_BLIND))
	{
		send_to_char("You can't see a damn thing, you're blind!\r\n", ch);
		return;
	}

	if ( GET_CLASS(ch) != CLASS_THIEF && !IS_IMMORTAL(ch) )
	{
		send_to_char("You better leave scanning to thieves.\r\n",ch);
		return;
	}

	send_to_char("You quickly scan the area.\r\n", ch);
	act("$n focuses and remains still...scanning the area...",TRUE, ch, 0,0, TO_ROOM);
	
	if (IN_WILD(ch))
	{
		wild_scan(ch);
		return;
	}

	/* Show the people in the immediate room */
	count += list_scanned_chars_to_char(ch->in_room->people, ch, -1, 0);
	
	/* Find and show the people in neighboring rooms */
	for (dir = 0; dir < NUM_OF_DIRS; dir++)
	{
		if ((pexit = find_exit( ch->in_room, dir )))
		{
			if (EXIT_FLAGGED(pexit, EX_FALSE) || EXIT_FLAGGED(pexit, EX_HIDDEN))
				continue;

			/* Okay, there is an exit in that direction, check to see if its open */
			if (!EXIT_FLAGGED(pexit, EX_CLOSED))
				/* Generate a scan list for that room */
				count += list_scanned_chars_to_char(pexit->to_room->people, ch, dir, 1);
			else
				/* There is something in the way, list it to the char */
				block_view(ch, (pexit->keyword ? pexit->keyword : "something"), dir);
		}
	}

	if (count == 0)
		send_to_char("You don't see anyone nearby.\r\n", ch);
}

/* search for hidden doors */
ACMD(do_search)
{
	EXIT_DATA *pexit;
	int door, chance = 1, found = 0;
	
	if (AFF_FLAGGED(ch, AFF_BLIND))
	{
		send_to_char("You're blind, you can't see a damned thing!", ch);
		return;
	}
	
	if ( GET_CLASS(ch) != CLASS_THIEF && IS_MORTAL(ch) )
	{
		send_to_char("You better leave searching to thieves.\r\n",ch);
		return;
	}

	send_to_char("You begin to search the room...\r\n", ch);
	
	for (door = 0; door < NUM_OF_DIRS; door++)
	{
		if ((pexit = find_exit(ch->in_room, door)))
		{ 
			if (EXIT_FLAGGED(pexit, EX_FALSE))
				continue;

			if (EXIT_FLAGGED(pexit, EX_HIDDEN))
			{
				if (GET_INT(ch) >= 17)
					chance += 1;
				if (number(1, 6) <= chance)
				{
					ch_printf(ch, "&b&7You have found a secret door %s.&0\r\n", dirs[door]);
					REMOVE_BIT(EXIT_FLAGS(pexit), EX_HIDDEN);
					if (pexit->rexit)
						REMOVE_BIT(EXIT_FLAGS(pexit->rexit), EX_HIDDEN);
					found++;
				}
			}
		}
	}

	if (!found)
		send_to_char("You haven't found anything.\r\n", ch);
}
