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
*   File: act.movement.c                                Part of CircleMUD *
*  Usage: movement commands, door handling, & sleep/rest/etc state        *
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
#include "house.h"
#include "constants.h"

/* external variables  */
extern int tunnel_size;

/* external functions */
BUILDING_DATA *find_building_in_room_by_name(ROOM_DATA *pRoom, char *arg);
BUILDING_DATA *find_char_owned_building(CHAR_DATA *ch);
EXIT_DATA *find_building_door(CHAR_DATA *ch, char *doorname, char *building);
int		special(CHAR_DATA *ch, int cmd, char *arg);
int		find_eq_pos(CHAR_DATA *ch, OBJ_DATA *obj, char *arg);
void	death_cry(CHAR_DATA *ch);
void	look_at_wild(CHAR_DATA *ch, bool quick);
void	dismount_char(CHAR_DATA *ch);
void	enter_building(CHAR_DATA *ch, BUILDING_DATA *bld);
void	char_from_building(CHAR_DATA *ch);
void	char_enter_vehicle(CHAR_DATA *ch, VEHICLE_DATA *vehicle);
void	ship_move(CHAR_DATA *ch, int dir, int special_check);
void	encounter(ROOM_DATA *pRoom);

/* local functions */
EXIT_DATA *find_door(CHAR_DATA *ch, const char *type, char *dir, const char *cmdname);
int has_boat(CHAR_DATA *ch);
int has_key(CHAR_DATA *ch, obj_vnum key);
void do_doorcmd(CHAR_DATA *ch, OBJ_DATA *obj, EXIT_DATA *pexit, int scmd);
int ok_pick(CHAR_DATA *ch, obj_vnum keynum, int pickproof, int scmd);
ACMD(do_gen_door);
ACMD(do_enter);
ACMD(do_stand);
ACMD(do_sit);
ACMD(do_rest);
ACMD(do_sleep);
ACMD(do_wake);


bool water_sector(int sect)
{
	if (sect == SECT_WATER_NOSWIM || sect == SECT_RIVER_NAVIGABLE || sect == SECT_SEA)
		return (TRUE);

	/* TODO -- Simple river can be crossed with a check.. */
	if (sect == SECT_RIVER)
		return (TRUE);

	return (FALSE);
}

/* simple function to determine if char can walk on water */
int has_boat(CHAR_DATA *ch)
{
	OBJ_DATA *obj;
	int i;
	
	if (GET_LEVEL(ch) > LVL_IMMORT)
		return (1);
	
	if (AFF_FLAGGED(ch, AFF_WATERWALK))
		return (1);
	
	/* non-wearable boats in inventory will do it */
	for (obj = ch->first_carrying; obj; obj = obj->next_content)
	{
		if (GET_OBJ_TYPE(obj) == ITEM_BOAT && (find_eq_pos(ch, obj, NULL) < 0))
			return (1);
	}
	
	/* and any boat you're wearing will do it too */
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i) && GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_BOAT)
			return (1);
	}
	
	return (0);
}


int calc_movement(int sect_from, int sect_to)
{
	if (!terrain_type[sect_from] || !terrain_type[sect_to])
		return (1);

	return ((terrain_type[sect_from]->movement_loss + terrain_type[sect_to]->movement_loss) / 2);
}

/* do_simple_move assumes
 *    1. That there is no master and no followers.
 *    2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */
int do_simple_move(CHAR_DATA *ch, int dir, int need_specials_check)
{
	EXIT_DATA *pexit;
	char throwaway[MAX_INPUT_LENGTH] = ""; /* Functions assume writable. */
	int need_movement;
	
	if (!(pexit = get_exit(ch->in_room, dir)))
		return (0);

	/*
	 * Check for special routines (North is 1 in command list, but 0 here) Note
	 * -- only check if following; this avoids 'double spec-proc' bug
	 */
	if (need_specials_check && special(ch, dir + 1, throwaway))
		return (0);
	
	/* if they're mounted, are they in the same room w/ their mount(ee)? */
	if (RIDING(ch) && RIDING(ch)->in_room != ch->in_room)
		/* if not in same room, dismount them.. (FB) */
		dismount_char(ch);

	if (WAGONER(ch) && WAGONER(ch)->in_room != ch->in_room)
		stop_be_wagoner(ch);

	/* charmed? */
	if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && ch->in_room == ch->master->in_room)
	{
		send_to_char("The thought of leaving your master makes you weep.\r\n", ch);
		act("$n bursts into tears.", FALSE, ch, 0, 0, TO_ROOM);
		return (0);
	}
	
	if ( WAGONER(ch) )
	{
		// the vehicle could move?
		if ( !vehicle_can_move(WAGONER(ch)) )
		{
			VEHICLE_DATA *pVeh = WAGONER(ch);

			if (pVeh->curr_val.draft_mobs < pVeh->max_val.draft_mobs)
			{
				if (pVeh->max_val.draft_mobs == 1)
					ch_printf(ch, "You need to yoke a draft mob to %s.\r\n",
						pVeh->short_description);
				else
					ch_printf(ch, "You need to yoke %d draft mobs to %s.\r\n",
						pVeh->max_val.draft_mobs, pVeh->short_description);
			}
			else if ( IS_SET(pVeh->flags, VEH_DESTROYED) )
				ch_printf(ch, "The %s is destroyed and cannot move.\r\n", WAGONER(ch)->name);
			else
				ch_printf(ch, "The %s cannot move.\r\n", WAGONER(ch)->name);
			return (0);
		}

		// the vehicle could go there?
		if ( !vehicle_can_go(WAGONER(ch), pexit->to_room) )
		{
			ch_printf(ch, "The %s cannot go there.\r\n", WAGONER(ch)->name);
			return (0);
		}
	}
	else
	{
	/* if this room or the one we're going to needs a boat, check for one */
	if (water_sector(SECT(ch->in_room)) || water_sector(SECT(pexit->to_room)))
	{
		if (RIDING(ch))
		{
			if (!has_boat(RIDING(ch)))
			{
				send_to_char("Your mount need a boat to go there.\r\n", ch);
				return (0);
			}
		}
		else
		{
			if (!has_boat(ch))
			{
				send_to_char("You need a boat to go there.\r\n", ch);
				return (0);
			}
		}
	}
	}
	
	/* move points needed is avg. move loss for src and destination sect type */
	need_movement = calc_movement(SECT(ch->in_room), SECT(pexit->to_room));

	// if mounted, check mount's movement points
	if (RIDING(ch))
	{
		if (GET_MOVE(RIDING(ch)) < need_movement)
		{
			send_to_char("Your mount is too exhausted.\r\n", ch);
			return (0);
		}
	}
	else if (WAGONER(ch))
	{
		// TODO - check (movement * 2 / yoked mobs)  - for all mobs
	}
	// if it's a mount, check movement points
	else if (IS_NPC(ch) && RIDDEN_BY(ch))
	{
		if (GET_MOVE(ch) < need_movement)
		{
			send_to_char("Your mount is too exhausted.\r\n", RIDDEN_BY(ch));
			return (0);
		}
	}
	// player or non-mount mob
	else if (GET_MOVE(ch) < need_movement && !IS_NPC(ch))
	{
		if (need_specials_check && ch->master)
			send_to_char("You are too exhausted to follow.\r\n", ch);
		else
			send_to_char("You are too exhausted.\r\n", ch);
		
		return (0);
	}
	
	if (ROOM_FLAGGED(ch->in_room, ROOM_ATRIUM))
	{
		if ( !House_can_enter(ch, pexit->to_room->number) )
		{
			send_to_char("That's private property -- no trespassing!\r\n", ch);
			return (0);
		}

		if ( RIDING(ch) )
		{
			send_to_char("Enter a house while mounted?? try dismounting first.\r\n", ch);
			return (0);
		}
	}
	
	if ((RIDING(ch) || RIDDEN_BY(ch)) && ROOM_FLAGGED(pexit->to_room, ROOM_TUNNEL))
	{
		send_to_char("There isn't enough room there, while mounted.\r\n", ch);
		return (0);
	}

	if (WAGONER(ch) && ROOM_FLAGGED(pexit->to_room, ROOM_TUNNEL))
	{
		ch_printf(ch, "You cannot drive %s there.\r\n", WAGONER(ch)->short_description);
		return (0);
	}

	if (ROOM_FLAGGED(pexit->to_room, ROOM_TUNNEL) && num_pc_in_room(pexit->to_room) >= tunnel_size)
	{
		if (tunnel_size > 1)
			send_to_char("There isn't enough room for you to go there!\r\n", ch);
		else
			send_to_char("There isn't enough room there for more than one person!\r\n", ch);
		return (0);
	}
	
	/* Mortals and low level gods cannot enter greater god rooms. */
	if (ROOM_FLAGGED(pexit->to_room, ROOM_GODROOM) && GET_LEVEL(ch) < LVL_GRGOD)
	{
		send_to_char("You aren't godly enough to use that room!\r\n", ch);
		return (0);
	}
	
	/* Room Trigger Events */
	if ( ch->in_room->trigger )
		if ( check_room_trigger(ch, TRIG_ACT_EXIT) )
			return (1);

	/* Now we know we're allow to go into the room. */
	if (GET_LEVEL(ch) < LVL_IMMORT)
	{
		if (RIDING(ch))
			GET_MOVE(RIDING(ch)) -= need_movement;
		else if (WAGONER(ch))
		{
		// TODO - subtract (movement * 2 / yoked mobs)  - from all mobs
		}
		else if (!IS_NPC(ch) || (IS_NPC(ch) && RIDDEN_BY(ch)))
			GET_MOVE(ch) -= need_movement;
	}
	
	if (WAGONER(ch))
	{
		sprintf(buf2, "&b&7$n drives $v %s.&0", dirs[dir]);
		act(buf2, TRUE, ch, NULL, WAGONER(ch), TO_NOTVICT);
		
	}
	else if (RIDING(ch))
	{
		if (!AFF_FLAGGED(RIDING(ch), AFF_SNEAK))
		{
			sprintf(buf2, "&b&7$n rides $N %s.&0", dirs[dir]);
			act(buf2, TRUE, ch, NULL, RIDING(ch), TO_NOTVICT);
		}
	}
	else if (!RIDDEN_BY(ch) && !AFF_FLAGGED(ch, AFF_SNEAK))
	{
		sprintf(buf2, "&b&7$n leaves %s.&0", dirs[dir]);
		act(buf2, TRUE, ch, NULL, NULL, TO_ROOM);
	}
	
	/* handle exit from building */
	if (IN_BUILDING(ch) && !IS_BUILDING(pexit->to_room))
		char_from_building(ch);

	/* random encounters */
	if (!IS_NPC(ch) && IS_WILD(pexit->to_room))
		encounter(pexit->to_room);

	char_from_room(ch);
	char_to_room(ch, pexit->to_room);
	
	/* the mount move with his mounter (FB) */
	if (RIDING(ch))
	{
		char_from_room(RIDING(ch));
		char_to_room(RIDING(ch), ch->in_room);
	}
	/* if the mount move, move the rider too */
	else if (RIDDEN_BY(ch))
	{
		char_from_room(RIDDEN_BY(ch));
		char_to_room(RIDDEN_BY(ch), ch->in_room);
	}
	/* move the vehicle and the hitched mob.. */
	else if (WAGONER(ch))
	{
		VEHICLE_DATA *vehicle = WAGONER(ch);

		vehicle_from_room(vehicle);
		vehicle_to_room(vehicle, ch->in_room);

		if (vehicle->first_yoke)
		{
			YOKE_DATA *yoke;

			for (yoke = vehicle->first_yoke; yoke; yoke = yoke->next)
			{
				char_from_room(yoke->mob);
				char_to_room(yoke->mob, ch->in_room);
			}
		}

		if (vehicle->people)
			act("The $V is moving...", FALSE, vehicle->people, NULL, vehicle, TO_ROOM);
	}

	if (WAGONER(ch))
	{
		sprintf(buf, "&b&7$n enters from %s%s driving $v.&0",
			(dir < UP || dir > DOWN ? "the " : ""),
			(dir == UP ? "below": dir == DOWN ? "above" : dirs[rev_dir[dir]]));
		act(buf, TRUE, ch, NULL, WAGONER(ch), TO_ROOM);
	}
	else if (RIDING(ch))
	{
		if (!AFF_FLAGGED(RIDING(ch), AFF_SNEAK))
		{
			sprintf(buf2, "&b&7$n arrives from %s%s, riding $N.&0",
				(dir < UP || dir > DOWN ? "the " : ""),
				(dir == UP ? "below": dir == DOWN ? "above" : dirs[rev_dir[dir]]));
			act(buf2, TRUE, ch, NULL, RIDING(ch), TO_ROOM);
		}
	}
	else if (!RIDDEN_BY(ch) && !AFF_FLAGGED(ch, AFF_SNEAK))
	{
		sprintf(buf, "&b&7$n enters from %s%s.&0",
			(dir < UP || dir > DOWN ? "the " : ""),
			(dir == UP ? "below": dir == DOWN ? "above" : dirs[rev_dir[dir]]));
		act(buf, TRUE, ch, NULL, NULL, TO_ROOM);
	}
	
	if (ch->desc != NULL)
	{
		if ( IN_WILD(ch) && SECT(ch->in_room) != SECT_CITY)
			look_at_wild(ch, FALSE);
		else
			look_at_room(ch, 0);
	}
	
	if (ROOM_FLAGGED(ch->in_room, ROOM_DEATH) && GET_LEVEL(ch) < LVL_IMMORT)
	{
		log_death_trap(ch);
		death_cry(ch);
		extract_char(ch);
		return (0);
	}

	/* Chech for trapped items */
	if (ch->in_room->first_content)
	{
		OBJ_DATA *obj;
		for (obj = ch->in_room->first_content; obj; obj = obj->next_content)
			check_trap(ch, obj, TRAP_ACT_ROOM);
	}

	/* Room Trigger Events */
	if ( ch->in_room->trigger )
		check_room_trigger(ch, TRIG_ACT_ENTER);
	
	return (1);
}



int perform_move(CHAR_DATA *ch, int dir, int need_specials_check)
{
	ROOM_DATA *was_in;
	EXIT_DATA *pexit;
	FOLLOW_DATA *k, *next;
	
	if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS || FIGHTING(ch))
		return (0);

	if (ch->in_vehicle)
	{
		send_to_char("You are inside a vehicle, let it move for you.\r\n", ch);
		return (0);
	}

	if (!(pexit = get_exit(ch->in_room, dir)))
	{
		send_to_char("Alas, you cannot go that way...\r\n", ch);
		return (0);
	}

	if (EXIT_FLAGGED(pexit, EX_CLOSED))
	{
		if (pexit->keyword)
		{
			sprintf(buf2, "The %s seems to be closed.\r\n", fname(pexit->keyword));
			send_to_char(buf2, ch);
		}
		else
			send_to_char("It seems to be closed.\r\n", ch);
		return (0);
	}

	// leaving a shielded room?
	if (ROOM_AFFECTED(ch->in_room, RAFF_SHIELD))
	{
		char sbaf[MAX_STRING_LENGTH];

		sprintf(sbaf, "The magic field surrounding this room stops $n to go %s.", dirs[dir]);
		act(sbaf, FALSE, ch, NULL, NULL, TO_ROOM);

		if (!IS_NPC(ch))
			send_to_char("The magic field surrounding this room stops you.\r\n", ch);

		return (0);
	}

	// wanna enter into a shielded room?
	if (ROOM_AFFECTED(pexit->to_room, RAFF_SHIELD))
	{
		char sbaf[MAX_STRING_LENGTH];
	
		sprintf(sbaf, "$n tries to go %s, but is stopped by a magic field.", dirs[dir]);
		act(sbaf, TRUE, ch, NULL, NULL, TO_ROOM);

		if (!IS_NPC(ch))
			send_to_char("That room is protected by a strong magic field.\r\n", ch);

		return (0);
	}

	/* let's see if the guy has enough money to be transported by the ferryboat */
	if (!IS_NPC(ch) && SECT(pexit->to_room) == SECT_FERRY_DECK && ch->in_room->ferryboat)
	{
		ch_printf(ch, "To be carried by the ferryboat you must pay %d gold coins.\r\n",
			ch->in_room->ferryboat->cost);

		if (IS_MORTAL(ch))
		{
			if (get_gold(ch) < ch->in_room->ferryboat->cost)
			{ 
				send_to_char("That you don't have!!!\r\n", ch);
				return (0);
			}
			else
				sub_gold(ch, ch->in_room->ferryboat->cost);
		}
	}

	if (!ch->followers)
		return (do_simple_move(ch, dir, need_specials_check));
	
	was_in = ch->in_room;
	if (!do_simple_move(ch, dir, need_specials_check))
		return (0);
	
	for (k = ch->followers; k; k = next)
	{
		next = k->next;
		if ((k->follower->in_room == was_in) &&
			(GET_POS(k->follower) >= POS_STANDING))
		{
			act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
			perform_move(k->follower, dir, 1);
		}
	}

	return (1);
}


/*
 * This is basically a mapping of cmd numbers to perform_move indices.
 * It cannot be done in perform_move because perform_move is called
 * by other functions which do not require the remapping.
 */
ACMD(do_move)
{
	if (ch->in_obj)
	{
		act("You get out of $p.", TRUE, ch, ch->in_obj, 0, TO_CHAR);
		act("$n gets out of $p.", TRUE, ch, ch->in_obj, 0, TO_ROOM);
		char_from_obj(ch);
		GET_POS(ch) = POS_STANDING;
	}

	if ( ch->in_ship && ch->player_specials->athelm )
		ship_move( ch, subcmd - 1, 0 );
	else
		perform_move(ch, subcmd - 1, 0);
}


EXIT_DATA *find_door(CHAR_DATA *ch, const char *type, char *dir, const char *cmdname)
{
	EXIT_DATA *pexit;
	int door;
	
	if (*dir)			/* a direction was specified */
	{
		if ((door = search_block(dir, dirs, FALSE)) == -1)	/* Partial Match */
		{
			send_to_char("That's not a direction.\r\n", ch);
			return (NULL);
		}

		if ((pexit = find_exit(ch->in_room, door)))
		{
			if (!pexit->to_room || EXIT_FLAGGED(pexit, EX_HIDDEN))
				return (NULL);

			if (type && pexit->keyword)
			{
				if (isname(type, pexit->keyword))
					return (pexit);
				else
				{
					sprintf(buf2, "I see no %s there.\r\n", type);
					send_to_char(buf2, ch);
					return (NULL);
				}
			}
			else
				return (pexit);
		}
		else
		{
			sprintf(buf2, "I really don't see how you can %s anything there.\r\n", cmdname);
			send_to_char(buf2, ch);
			return (NULL);
		}
	}
	else			/* try to locate the keyword */
	{
		if (!*type || !type)
		{
			sprintf(buf2, "What is it you want to %s?\r\n", cmdname);
			send_to_char(buf2, ch);
			return (NULL);
		}

		for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
		{
			if (EXIT_FLAGGED(pexit, EX_HIDDEN))
				continue;
			if (pexit->keyword && isname(type, pexit->keyword))
				return (pexit);
		}

		sprintf(buf2, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
		send_to_char(buf2, ch);
		return (NULL);
	}
}


int has_key(CHAR_DATA *ch, obj_vnum key)
{
	OBJ_DATA *o;
	
	for (o = ch->first_carrying; o; o = o->next_content)
		if (GET_OBJ_VNUM(o) == key)
			return (1);
		
	if (GET_EQ(ch, WEAR_HOLD))
		if (GET_OBJ_VNUM(GET_EQ(ch, WEAR_HOLD)) == key)
			return (1);
			
	return (0);
}



#define NEED_OPEN		(1 << 0)
#define NEED_CLOSED		(1 << 1)
#define NEED_UNLOCKED	(1 << 2)
#define NEED_LOCKED		(1 << 3)

const char *cmd_door[] =
{
  "open",
  "close",
  "unlock",
  "lock",
  "pick"
};

const int flags_door[] =
{
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_OPEN,
  NEED_CLOSED | NEED_LOCKED,
  NEED_CLOSED | NEED_UNLOCKED,
  NEED_CLOSED | NEED_LOCKED
};


#define OPEN_DOOR(obj, pexit)	((obj) ?					\
		(REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :	\
		(REMOVE_BIT(pexit->exit_info, EX_CLOSED)))
#define CLOSE_DOOR(obj, pexit)	((obj) ?					\
		(SET_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) :		\
		(SET_BIT(pexit->exit_info, EX_CLOSED)))
#define UNLOCK_DOOR(obj, pexit)	((obj) ?					\
		(REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :	\
		(REMOVE_BIT(pexit->exit_info, EX_LOCKED)))
#define LOCK_DOOR(obj, pexit)	((obj) ?					\
		(SET_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) :		\
		(SET_BIT(pexit->exit_info, EX_LOCKED)))

void do_doorcmd(CHAR_DATA *ch, OBJ_DATA *obj, EXIT_DATA *pexit, int scmd)
{
	ROOM_DATA *other_room = NULL;
	EXIT_DATA *back; //, *pexit;
	
	if ( !obj && !pexit )
		return;
	
	sprintf(buf, "$n %ss ", cmd_door[scmd]);
	if ( obj )
		back = NULL;
	else
	{
		back = NULL;
		if (pexit && ((other_room = pexit->to_room) != NULL))
		{
			if ((back = pexit->rexit ) != NULL)
				if (back->to_room != ch->in_room)
					back = NULL;
		}
	}
	
	switch (scmd)
	{
	case SCMD_OPEN:
		OPEN_DOOR(obj, pexit);
		if (back)
			OPEN_DOOR(obj, back);
		if (obj)
			check_trap(ch, obj, TRAP_ACT_OPEN);
		send_to_char(OK, ch);
		break;
	case SCMD_CLOSE:
		CLOSE_DOOR(obj, pexit);
		if (back)
			CLOSE_DOOR(obj, back);
		if (obj)
			check_trap(ch, obj, TRAP_ACT_CLOSE);
		send_to_char(OK, ch);
		break;
	case SCMD_UNLOCK:
		UNLOCK_DOOR(obj, pexit);
		if (back)
			UNLOCK_DOOR(obj, back);
		send_to_char("*Click*\r\n", ch);
		break;
	case SCMD_LOCK:
		LOCK_DOOR(obj, pexit);
		if (back)
			LOCK_DOOR(obj, back);
		send_to_char("*Click*\r\n", ch);
		break;
	case SCMD_PICK:
		LOCK_DOOR(obj, pexit);
		if (back)
			LOCK_DOOR(obj, back);
		send_to_char("The lock quickly yields to your skills.\r\n", ch);
		strcpy(buf, "$n skillfully picks the lock on ");
		break;
	}
	
	/* Notify the room */
	sprintf(buf + strlen(buf), "%s%s.", ((obj) ? "" : "the "),
		(obj) ? "$p" : (pexit->keyword ? "$F" : "door"));
	if (!(obj) || (obj->in_room != NULL))
		act(buf, FALSE, ch, obj, obj ? 0 : pexit->keyword, TO_ROOM);
	
	/* Notify the other room */
	if ((scmd == SCMD_OPEN || scmd == SCMD_CLOSE) && back)
	{
		sprintf(buf, "The %s is %s%s from the other side.",
			(back->keyword ? fname(back->keyword) : "door"), cmd_door[scmd],
			(scmd == SCMD_CLOSE) ? "d" : "ed");
		if (pexit->to_room->people)
		{
			act(buf, FALSE, pexit->to_room->people, 0, 0, TO_ROOM);
			act(buf, FALSE, pexit->to_room->people, 0, 0, TO_CHAR);
		}
	}
}


int ok_pick(CHAR_DATA *ch, obj_vnum keynum, int pickproof, int scmd)
{
	int percent = number(1, 101);
	
	if (scmd == SCMD_PICK)
	{
		if (keynum < 0)
			send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
		else if (pickproof)
			send_to_char("It resists your attempts to pick it.\r\n", ch);
		else if (percent > GET_SKILL(ch, SKILL_PICK_LOCK))
			send_to_char("You failed to pick the lock.\r\n", ch);
		else
			return (1);
		return (0);
	}
	return (1);
}

#define DOOR_IS_OPENABLE(ch, obj, pexit)															\
			( (obj) ?																				\
            ( GET_OBJ_TYPE(obj) == ITEM_CONTAINER && OBJVAL_FLAGGED(obj, CONT_CLOSEABLE) ) :		\
            ( EXIT_FLAGGED(pexit, EX_ISDOOR) ) )

#define DOOR_IS_OPEN(ch, obj, pexit)					\
			((obj) ?									\
			(!OBJVAL_FLAGGED(obj, CONT_CLOSED)) :		\
			(!EXIT_FLAGGED(pexit, EX_CLOSED)))
#define DOOR_IS_UNLOCKED(ch, obj, pexit)				\
			((obj) ?									\
			(!OBJVAL_FLAGGED(obj, CONT_LOCKED)) :		\
			(!EXIT_FLAGGED(pexit, EX_LOCKED)))
#define DOOR_IS_PICKPROOF(ch, obj, pexit)				\
			((obj) ?									\
			(OBJVAL_FLAGGED(obj, CONT_PICKPROOF)) :		\
			(EXIT_FLAGGED(pexit, EX_PICKPROOF)))

#define DOOR_IS_CLOSED(ch, obj, pexit)	(!(DOOR_IS_OPEN(ch, obj, pexit)))
#define DOOR_IS_LOCKED(ch, obj, pexit)	(!(DOOR_IS_UNLOCKED(ch, obj, pexit)))
#define DOOR_KEY(ch, obj, pexit)		((obj) ? (GET_OBJ_VAL(obj, 2)) : (pexit->key))

ACMD(do_gen_door)
{
	OBJ_DATA *obj = NULL;
	CHAR_DATA *victim = NULL;
	EXIT_DATA *pexit = NULL;
	obj_vnum keynum;
	char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
	
	skip_spaces(&argument);
	if (!*argument)
	{
		sprintf(buf, "%s what?\r\n", cmd_door[subcmd]);
		send_to_char(CAP(buf), ch);
		return;
	}
	
	two_arguments(argument, type, dir);

	if (!generic_find(type, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj))
		if (!(pexit = find_building_door(ch, type, dir)))
			pexit = find_door(ch, type, dir, cmd_door[subcmd]);
	
	if ( (obj) || (pexit) )
	{
		keynum = DOOR_KEY(ch, obj, pexit);
		if (!(DOOR_IS_OPENABLE(ch, obj, pexit)))
			act("You can't $F that!", FALSE, ch, 0, cmd_door[subcmd], TO_CHAR);
		else if (!DOOR_IS_OPEN(ch, obj, pexit) &&
			IS_SET(flags_door[subcmd], NEED_OPEN))
			send_to_char("But it's already closed!\r\n", ch);
		else if (!DOOR_IS_CLOSED(ch, obj, pexit) &&
			IS_SET(flags_door[subcmd], NEED_CLOSED))
			send_to_char("But it's currently open!\r\n", ch);
		else if (!(DOOR_IS_LOCKED(ch, obj, pexit)) &&
			IS_SET(flags_door[subcmd], NEED_LOCKED))
			send_to_char("Oh.. it wasn't locked, after all..\r\n", ch);
		else if (!(DOOR_IS_UNLOCKED(ch, obj, pexit)) &&
			IS_SET(flags_door[subcmd], NEED_UNLOCKED))
			send_to_char("It seems to be locked.\r\n", ch);
		else if (!has_key(ch, keynum) && (GET_LEVEL(ch) < LVL_GOD) &&
			((subcmd == SCMD_LOCK) || (subcmd == SCMD_UNLOCK)))
			send_to_char("You don't seem to have the proper key.\r\n", ch);
		else if (ok_pick(ch, keynum, DOOR_IS_PICKPROOF(ch, obj, pexit), subcmd))
			do_doorcmd(ch, obj, pexit, subcmd);
	}
}



ACMD(do_enter)
{
	EXIT_DATA *pexit;

	one_argument(argument, buf);

	if (*buf)			/* an argument was supplied, search for door keyword */
	{
		for (pexit = ch->in_room->first_exit; pexit; pexit = pexit->next)
		{
			if (pexit->keyword)
			{
				if (!str_cmp(pexit->keyword, buf))
				{
					perform_move(ch, pexit->vdir, 1);
					return;
				}
			}
		}

		if (ch->in_room->buildings)
		{
			BUILDING_DATA *bld = find_building_in_room_by_name(ch->in_room, str_dup(buf));

			if (bld)
			{
				enter_building(ch, bld);
				return;
			}
		}

		if ( ch->in_room->vehicles )
		{
			VEHICLE_DATA *vehicle = find_vehicle_in_room_by_name(ch, str_dup(buf));

			if (vehicle)
			{
				char_enter_vehicle(ch, vehicle);
				return;
			}
		}

		sprintf(buf2, "There is no %s here.\r\n", buf);
		send_to_char(buf2, ch);
	}
	else if (ROOM_FLAGGED(ch->in_room, ROOM_INDOORS))
		send_to_char("You are already indoors.\r\n", ch);
	else
	{
		/* try to locate an entrance */
		for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
		{
			if ( pexit->to_room != NULL )
			{
				if (!EXIT_FLAGGED(pexit, EX_CLOSED) &&
					ROOM_FLAGGED(pexit->to_room, ROOM_INDOORS))
				{
					perform_move(ch, pexit->vdir, 1);
					return;
				}
			}
		}

		if (ch->in_room->buildings)
		{
			BUILDING_DATA *bld = find_char_owned_building(ch);

			if (bld)
			{
				enter_building(ch, bld);
				return;
			}
		}

		send_to_char("You can't seem to find anything to enter.\r\n", ch);
	}
}


/* ****************************************************************** */
/* Position handling                                                  */
/* ****************************************************************** */

/* called when ch wanna start using a furniture */
void check_furn(CHAR_DATA *ch, char *funame, char *text, int pos)
{
	CHAR_DATA *tch;
	OBJ_DATA *furn;
	char buf[MAX_STRING_LENGTH];
	int num = 0;
	
	if ( ch->in_obj )
		return;
	
	if ( pos == POS_FIGHTING )
	{
		send_to_char("During a fight? Are you MAD?\r\n", ch);
		return;
	}

	if ( GET_POS(ch) == pos )
	{
		switch (pos)
		{
		case POS_STANDING:
			send_to_char("You are already standing.\r\n", ch);
			break;
		case POS_SITTING:
			send_to_char("You're sitting already.\r\n", ch);
			break;
		case POS_RESTING:
			send_to_char("You are already resting.\r\n", ch);
			break;
		case POS_SLEEPING:
			send_to_char("You are already sound asleep.\r\n", ch);
			break;
		}
		return;
	}
	
	/* Can the char see the object requested? */
	if (!(furn = get_obj_in_list_vis_rev(ch, funame, NULL, ch->in_room->last_content)))
	{
		ch_printf(ch, "You don't see %s %s here.\r\n", AN(funame), funame);
		return;
	}
	
	/* Make sure they can actually sit on the object */
	if ((GET_OBJ_TYPE(furn) != ITEM_FURNITURE) || (pos > GET_OBJ_VAL(furn, 2)))
	{
		sprintf(buf, "%s isn't for %s on!\r\n", furn->short_description,
			position_types[(int) pos]);
		CAP(buf);
		send_to_char(buf, ch);
		return;
	}
	
	/* Check if there's room on this piece of furniture */
	for (num = 0, tch = furn->people; tch; tch = tch->next_in_obj, num++);
	
	/* Tell them why they can't fit on */
	if (GET_OBJ_VAL(furn, 1) <= num)
	{
		if (num == 1)
		{
			sprintf(buf, "%s is already %s on %s.\r\n", PERS(furn->people, ch),
				position_types[(int) GET_POS(tch)], furn->short_description);
			ch_printf(furn->people, "%s just tried to %s on you!\r\n", PERS(ch, furn->people), text);
		}
		else
			sprintf(buf, "There are already other people using %s.\r\n",
				furn->short_description);
		send_to_char(buf, ch);
		return;
	}
	
	GET_POS(ch) = pos;
	char_to_obj(ch, furn);
	
	sprintf(buf, "You %s down on $p.", text);
	act(buf, FALSE, ch, furn, NULL, TO_CHAR | TO_SLEEP);
	
	sprintf(buf, "$n %ss down on $p.", text);
	act(buf, TRUE, ch, furn, NULL, TO_ROOM);
}

/* called when ch IS using a furniture and change position */
void furn_pos_change(CHAR_DATA *ch, char *text, int pos)
{
	const char *pos_mess, *how;
	
	if ( !ch->in_obj )
		return;

	if ( GET_POS(ch) == pos )
	{
		switch (pos)
		{
		case POS_STANDING:
			send_to_char("You are already standing.\r\n", ch);
			break;
		case POS_SITTING:
			send_to_char("You're sitting already.\r\n", ch);
			break;
		case POS_RESTING:
			send_to_char("You are already resting.\r\n", ch);
			break;
		case POS_SLEEPING:
			send_to_char("You are already sound asleep.\r\n", ch);
			break;
		}
		return;
	}
	
	if (pos > GET_POS(ch))		pos_mess = "up";
	else				pos_mess = "down";
	
	if (pos == POS_STANDING)	how = "from";
	else				how = "on";
	
	/* Make sure they can actually do this on the object */
	if ( pos > GET_OBJ_VAL(ch->in_obj, 2) && pos != POS_STANDING )
	{
		sprintf(buf, "You can't %s %s %s $p.", text, pos_mess, how);
		act(buf, FALSE, ch, ch->in_obj, NULL, TO_CHAR | TO_SLEEP);
		return;
	}

	sprintf(buf, "You %s %s %s $p.", text, pos_mess, how);
	act(buf, FALSE, ch, ch->in_obj, 0, TO_CHAR | TO_SLEEP);
	
	sprintf(buf, "$n %ss %s %s $p.", text, pos_mess, how);
	act(buf, TRUE, ch, ch->in_obj, 0, TO_ROOM);

	if ( pos == POS_STANDING )
		char_from_obj(ch);

	GET_POS(ch) = pos;
}

void stop_memorizing(CHAR_DATA *ch);

ACMD(do_stand)
{
	if ( AFF_FLAGGED(ch, AFF_MEMORIZING) )
		stop_memorizing(ch);

	/* with "stand" ch leave any furniture she's using */
	if ( ch->in_obj )
	{
		furn_pos_change(ch, "stand", POS_STANDING);
		return;
	}

	switch (GET_POS(ch))
	{
	case POS_STANDING:
		send_to_char("You are already standing.\r\n", ch);
		break;
	case POS_SITTING:
		send_to_char("You stand up.\r\n", ch);
		act("$n clambers to $s feet.", TRUE, ch, 0, 0, TO_ROOM);
		/* Will be sitting after a successful bash and may still be fighting. */
		GET_POS(ch) = FIGHTING(ch) ? POS_FIGHTING : POS_STANDING;
		break;
	case POS_RESTING:
		send_to_char("You stop resting, and stand up.\r\n", ch);
		act("$n stops resting, and clambers on $s feet.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_STANDING;
		break;
	case POS_SLEEPING:
		send_to_char("You have to wake up first!\r\n", ch);
		break;
	case POS_FIGHTING:
		send_to_char("Do you not consider fighting as standing?\r\n", ch);
		break;
	default:
		send_to_char("You stop floating around, and put your feet on the ground.\r\n", ch);
		act("$n stops floating around, and puts $s feet on the ground.",
			TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_STANDING;
		break;
	}
}


ACMD(do_sit)
{
	char arg1[MAX_INPUT_LENGTH];
	
	if ( AFF_FLAGGED(ch, AFF_MEMORIZING) )
		stop_memorizing(ch);

	if (ch->in_obj)
	{
		furn_pos_change(ch, "sit", POS_SITTING);
		return;
	}

	if ( RIDING(ch) )
	{
		send_to_char("Sitting while mounted??\r\n", ch);
		return;
	}

	one_argument(argument, arg1);
	
	if ( !*arg1 )
	{
		switch (GET_POS(ch))
		{
		case POS_STANDING:
			send_to_char("You sit down.\r\n", ch);
			act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SITTING;
			break;
		case POS_SITTING:
			send_to_char("You're sitting already.\r\n", ch);
			break;
		case POS_RESTING:
			send_to_char("You stop resting, and sit up.\r\n", ch);
			act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SITTING;
			break;
		case POS_SLEEPING:
			send_to_char("You have to wake up first.\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Sit down while fighting? Are you MAD?\r\n", ch);
			break;
		default:
			send_to_char("You stop floating around, and sit down.\r\n", ch);
			act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SITTING;
			break;
		}
	}
	else
		check_furn(ch, arg1, "sit", POS_SITTING);
}


ACMD(do_rest)
{
	char arg1[MAX_INPUT_LENGTH];
	
	if (ch->in_obj)
	{
		furn_pos_change(ch, "rest", POS_RESTING);
		return;
	}
	
	if ( RIDING(ch) )
	{
		send_to_char("Resting while mounted??\r\n", ch);
		return;
	}

	one_argument(argument, arg1);
	
	if ( !*arg1 )
	{
		switch (GET_POS(ch))
		{
		case POS_STANDING:
			send_to_char("You sit down and rest your tired bones.\r\n", ch);
			act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_RESTING;
			/* Room Trigger Events */
			if ( ch->in_room->trigger )
				check_room_trigger(ch, TRIG_ACT_REST);
			break;
		case POS_SITTING:
			send_to_char("You rest your tired bones.\r\n", ch);
			act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_RESTING;
			/* Room Trigger Events */
			if ( ch->in_room->trigger )
				check_room_trigger(ch, TRIG_ACT_REST);
			break;
		case POS_RESTING:
			send_to_char("You are already resting.\r\n", ch);
			break;
		case POS_SLEEPING:
			send_to_char("You have to wake up first.\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Rest while fighting?  Are you MAD?\r\n", ch);
			break;
		default:
			send_to_char("You stop floating around, and stop to rest your tired bones.\r\n", ch);
			act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SITTING;
			/* Room Trigger Events */
			if ( ch->in_room->trigger )
				check_room_trigger(ch, TRIG_ACT_REST);
			break;
		}
	}
	else
		check_furn(ch, arg1, "rest", POS_RESTING);
}


ACMD(do_sleep)
{
	char arg1[MAX_INPUT_LENGTH];
	
	if ( AFF_FLAGGED(ch, AFF_MEMORIZING) )
		stop_memorizing(ch);

	if (ch->in_obj)
	{
		furn_pos_change(ch, "sleep", POS_SLEEPING);
		return;
	}
	
	if ( RIDING(ch) )
	{
		send_to_char("Sleeping while mounted??\r\n", ch);
		return;
	}

	one_argument(argument, arg1);
	
	if ( !*arg1 )
	{
		switch (GET_POS(ch))
		{
		case POS_STANDING:
		case POS_SITTING:
		case POS_RESTING:
			send_to_char("You go to sleep.\r\n", ch);
			act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SLEEPING;
			/* Room Trigger Events */
			if ( ch->in_room->trigger )
				check_room_trigger(ch, TRIG_ACT_SLEEP);
			break;
		case POS_SLEEPING:
			send_to_char("You are already sound asleep.\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Sleep while fighting?  Are you MAD?\r\n", ch);
			break;
		default:
			send_to_char("You stop floating around, and lie down to sleep.\r\n", ch);
			act("$n stops floating around, and lie down to sleep.",
				TRUE, ch, 0, 0, TO_ROOM);
			GET_POS(ch) = POS_SLEEPING;
			/* Room Trigger Events */
			if ( ch->in_room->trigger )
				check_room_trigger(ch, TRIG_ACT_SLEEP);
			break;
		}
	}
	else
		check_furn(ch, arg1, "sleep", POS_SLEEPING);
}


ACMD(do_wake)
{
	CHAR_DATA *vict;
	int self = 0;
	
	one_argument(argument, arg);
	if (*arg)
	{
		if (GET_POS(ch) == POS_SLEEPING)
			send_to_char("Maybe you should wake yourself up first.\r\n", ch);
		else if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)) == NULL)
			send_to_char(NOPERSON, ch);
		else if (vict == ch)
			self = 1;
		else if (AWAKE(vict))
			act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
		else if (AFF_FLAGGED(vict, AFF_SLEEP) || AFF_FLAGGED(vict, AFF_PARALYSIS))
			act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
		else if (GET_POS(vict) < POS_SLEEPING)
			act("$E's in pretty bad shape!", FALSE, ch, 0, vict, TO_CHAR);
		else
		{
			act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
			act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
			GET_POS(vict) = POS_SITTING;
		}

		if (!self)
			return;
	}

	if (AFF_FLAGGED(ch, AFF_SLEEP))
		send_to_char("You can't wake up!\r\n", ch);
	else if (GET_POS(ch) > POS_SLEEPING)
		send_to_char("You are already awake...\r\n", ch);
	else
	{
		send_to_char("You awaken, and sit up.\r\n", ch);
		act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
		GET_POS(ch) = POS_SITTING;
	}
}

