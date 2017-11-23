/**************************************************************************
 * #   #   #   ##   #  #  ###   ##   ##  ###       http://www.lyonesse.it *
 * #    # #   #  #  ## #  #    #    #    #                                *
 * #     #    #  #  # ##  ##    #    #   ##   ## ##  #  #  ##             *
 * #     #    #  #  # ##  #      #    #  #    # # #  #  #  # #            *
 * ###   #     ##   #  #  ###  ##   ##   ###  #   #  ####  ##    Ver. 1.0 *
 *                                                                        *
 * -Based on CircleMud v3.0-    Copyright (c) 1996-2001 by Fabrizio Baldi *
 **************************************************************************/
/**************************************************************************
 * File: room_trigger.c               Intended to be used with CircleMUD  *
 *                                                                        *
 * Usage: code for triggered events on rooms				  *
 *                                                                        *
 * Copyright (C) 1999, 2000 by Fabrizio Baldi                             *
 *                                                                        *
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 * CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "constants.h"

/* external functions */
ROOM_DATA *create_wild_room( COORD_DATA *coord, bool Static );
bitvector_t asciiflag_conv(char *flag);

/* functions in this file */
void activate_teleport(CHAR_DATA *ch, int delay, ROOM_DATA *target_room, char *to_char, char *to_room);
void teleport_char(CHAR_DATA *ch, ROOM_DATA *target_room, char *to_char, char *to_room);
void trigger_teleport(CHAR_DATA *ch, ROOM_DATA *pRoom, int trigger);

void activate_heal(CHAR_DATA *ch, int delay, char *to_char, char *to_room);
void heal_char(CHAR_DATA *ch, char *to_char, char *to_room);
void trigger_heal(CHAR_DATA *ch, ROOM_DATA *pRoom, int trigger);

void trigger_collapse(CHAR_DATA *ch, ROOM_DATA *pRoom, int trigger);
void trigger_explosion(CHAR_DATA *ch, ROOM_DATA *pRoom, int trigger);

EVENTFUNC(teleport_event);
EVENTFUNC(heal_event);

/* ========================================================================= */

/*
 * Explanation of TRIGGER TYPE:
 *
 * TELEPORT:  teleport pc/npc from room to: 1) another room 2) to coord
 * COLLAPSE:  room collapse, making the specified damage to everybody, and
 *            restore itself after the time specified
 *            (usually tunnels or underground rooms)
 * EXPLOSION: room explode, making the specified damage
 *            (usually magic rooms)
 * HEAL:      room heals everybody
 *            (usually holy rooms)
 */

/* ========================================================================= */

int parse_trig_type( char *trigname )
{
	if ( !str_cmp(trigname, "teleport") )
		return (TRIG_TELEPORT);
	if ( !str_cmp(trigname, "collapse") )
		return (TRIG_COLLAPSE);
	if ( !str_cmp(trigname, "explosion") )
		return (TRIG_EXPLOSION);
	if ( !str_cmp(trigname, "heal") )
		return (TRIG_HEAL);

	return (TRIG_NONE);
}

/*
 * Room Trigger event
 *
 * Format:
 *
 * T
 * <type name> <who trig> <timer>
 * <action bitv> <random> <active>
 * <value0> <value1> <value2> <value3>
 *
 * Details:
 *
 * <type name>	 : the name type of trigger, ( es "teleport", "collapse" ) NO "
 * <who trig>    : 1 (Everybody); 2 (PCs); 3 (Mobs)
 * <timer>       : range from 1 (immediately) to MAX_TEL_DELAY
 * <action bitv> : see structs.h & constants.c
 * <random>		 : 0 for an always trigger, otherwise a 1-100 value for random %
 * <active>      : 0 (Not Active); 1 (Active)
 *
 * Details of Value fields:
 *
 * Trigger Type		Value1		Value2		Value3		Value4
 * -----------------------------------------------------------------------------
 * TRIG_TELEPORT 	<room vnum>	<coord y>	<coord x>	<unused>
 * TRIG_COLLAPSE	<room vnum>	<damage>	<timer>		<unused>
 * TRIG_EXPLOSION	<damage>	<unused>	<unused>	<unused>
 * TRIG_HEAL		<unused>	<unused>	<unused>	<unused>
 * -----------------------------------------------------------------------------
 *
 * <room vnum> is the destination room VNUM for teleport
 * <coord y & x> are the destination coords for teleport in wilderness
 * <damage> is the amount of damage suffered by every pc/npc in rooms
 *
 * NOTE: when coords are specified, <room vnum> MUST BE 0
 *       when <room vnum> is specified, <coord y> & <coord x> are IGNORED
 *
 *
 * Example:
 *
 * "Always teleport to room 3001 everybody who rest in room, 4 seconds delayed"
 * T
 * teleport 1 4
 * h 0 1
 * 3001 0 0 0
 *
 * "Heal players (no mobs) that sleep in room, immediately, 50% probs"
 * T
 * heal 2 1
 * i 50 1
 * 0 0 0 0
 *
 */
void setup_trigger( ROOM_DATA *pRoom, FILE *fl )
{
	TRIGGER_DATA *trig;
	char line[256], f1[128], tbuf[MAX_STRING_LENGTH];
	int retval, t[10];
	
	CREATE(trig, TRIGGER_DATA, 1);
	trig->text			= NULL;
	trig->next			= NULL;

	if (!get_line(fl, line))
	{
		log("SYSERR: Expecting trigger data of room #%d but file ended!", pRoom->number);
		DISPOSE(trig);
		exit(1);
	}
	
	if ( (retval = sscanf(line, " %s %d %d ", f1, t, t+1)) != 3)
	{
		log("SYSERR: Format error in first line of room trigger data of room #%d", pRoom->number );
		DISPOSE(trig);
		exit(1);
	}

	trig->type			= parse_trig_type( f1 );
	if ( trig->type == TRIG_NONE )
	{
		log("SYSERR: Unknown trigger type in room trigger data of room #%d", pRoom->number );
		DISPOSE(trig);
		exit(1);
	}
	trig->whotrig		= URANGE(0, t[0], MAX_TRIG_WHO);
	trig->timer			= URANGE(1, t[1], MAX_TRIG_DELAY);

	if (!get_line(fl, line))
	{
		log("SYSERR: Expecting trigger data of room #%d but file ended!", pRoom->number);
		DISPOSE(trig);
		exit(1);
	}
	
	if ( (retval = sscanf(line, " %s %d %d ", f1, t, t+1)) != 3)
	{
		log("SYSERR: Format error in second line of room trigger data of room #%d", pRoom->number );
		DISPOSE(trig);
		exit(1);
	}

	trig->action		= asciiflag_conv(f1);
	if ( t[0] )
		trig->random	= URANGE(1, t[0], 100);
	else
		trig->random	= 101;
	trig->active		= URANGE(0, t[1], 1);

	if (!get_line(fl, line))
	{
		log("SYSERR: Expecting trigger data of room #%d but file ended!", pRoom->number);
		DISPOSE(trig);
		exit(1);
	}
	
	if ( (retval = sscanf(line, " %d %d %d %d ", t, t+1, t+2, t+3)) != 4)
	{
		log("SYSERR: Format error in third line of room trigger data of room #%d", pRoom->number );
		DISPOSE(trig);
		exit(1);
	}

	switch ( trig->type )
	{
	case TRIG_TELEPORT:
		if ( t[0] )
		{
			trig->value[0]	= t[0];		// room vnum
			trig->value[1]	= 0;
			trig->value[2]	= 0;
			trig->value[3]	= 0;
		}
		else
		{
			if ( !check_coord(t[1], t[2]) )
			{
				log("SYSERR: Invalid coord values in room trigger data of room #%d", pRoom->number );
				DISPOSE(trig);
				exit(1);
			}
			trig->value[0]	= 0;
			trig->value[1]	= t[1];		// y coord
			trig->value[2]	= t[2];		// x coord
			trig->value[3]	= 0;
		}
		break;
	case TRIG_COLLAPSE:
		if ( !t[0] )
		{
			log("SYSERR: Missing room vnum in room trigger data of room #%d", pRoom->number );
			DISPOSE(trig);
			exit(1);
		}
		trig->value[0]		= t[0];						// room vnum
		trig->value[1]		= URANGE(1, t[1], 1000);	// max 1000 dam points
		trig->value[2]		= URANGE(1, t[2], 10);		// restore in ? 
		trig->value[3]		= 0;
		break;
	case TRIG_EXPLOSION:
		trig->value[0]		= URANGE(1, t[0], 1000);	// max 1000 dam points
		trig->value[1]		= 0;
		trig->value[2]		= 0;
		trig->value[3]		= 0;
		break;
	case TRIG_HEAL:
	default:
		trig->value[0]		= 0;
		trig->value[1]		= 0;
		trig->value[2]		= 0;
		trig->value[3]		= 0;
		break;
	}

	if ( !trig->text )
	{
		sprintf( tbuf, "%s trigger", trig_type_descr[trig->type]);
		trig->text		= str_dup(tbuf);
	}
	
	/* add to the room list */
	trig->next			= pRoom->trigger;
	pRoom->trigger		= trig;
}

/* ========================================================================= */

/*
 * Core function of the room trigger events
 */
bool check_room_trigger(CHAR_DATA *ch, int trigger)
{
	ROOM_DATA *pRoom;
	bool okwho = TRUE;

	/* no ch? no party! :-)) */
	if ( !ch )
		return (FALSE);

	pRoom = ch->in_room;

	if ( !pRoom->trigger )
		return (FALSE);

	/* trigger is active? */
	if ( !pRoom->trigger->active )
		return (FALSE);

	/* trigger is activate by THIS action? */
	if ( !IS_SET(pRoom->trigger->action, trigger) )
		return (FALSE);

	/* can ch activate the trigger? */
	switch ( pRoom->trigger->whotrig )
	{
	default:
	case 0:
		break;
	case 1:
		if (IS_NPC(ch))		okwho = FALSE;
		break;
	case 2:
		if (!IS_NPC(ch))	okwho = FALSE;
		break;
	}

	if ( !okwho )
		return (FALSE);

	/* nothing happens if number is greater than chance of the trigger */
	if ( number(0, 100) > pRoom->trigger->random )
		return (FALSE);

	switch ( pRoom->trigger->type )
	{
	default:
		log( "SYSERR: check_room_trigger() : unknown room trigger type." );
		return (FALSE);
	case TRIG_TELEPORT:
		trigger_teleport(ch, pRoom, trigger);
		break;
	case TRIG_HEAL:
		trigger_heal(ch, pRoom, trigger);
		break;
	case TRIG_COLLAPSE:
	case TRIG_EXPLOSION:
		return (FALSE);
	}
	
	return (TRUE);
}

/* ========================================================================= */

void activate_teleport(CHAR_DATA *ch, int delay, ROOM_DATA *target_room, char *to_char, char *to_room)
{
	ROOM_TELEPORT *rte = NULL;

	CREATE(rte, ROOM_TELEPORT, 1);

	rte->ch				= ch;
	rte->orig_room		= ch->in_room;
	rte->delay			= delay;
	rte->target			= target_room;
	rte->msg_to_char	= str_dup(to_char);
	rte->msg_to_room	= str_dup(to_room);

	event_create(teleport_event, rte, 1);
}


void teleport_char(CHAR_DATA *ch, ROOM_DATA *target_room, char *to_char, char *to_room)
{
	send_to_char(to_char, ch);
	act(to_room, TRUE, ch, NULL, NULL, TO_ROOM);

	char_from_room(ch);
	char_to_room(ch, target_room);

	/* # TODO # should teleport away horse and vehicles too... */
	look_at_room( ch, TRUE );
}


void trigger_teleport(CHAR_DATA *ch, ROOM_DATA *pRoom, int trigger)
{
	ROOM_DATA *target_room;
	COORD_DATA *coord;
	char *to_char, *to_room;

	if ( pRoom->trigger->value[0] )
		target_room = get_room( pRoom->trigger->value[0] );
	else
	{
		CREATE(coord, COORD_DATA, 1);
		coord->y = pRoom->trigger->value[1];
		coord->x = pRoom->trigger->value[2];
		target_room = create_wild_room( coord, FALSE );
	}

	if ( !target_room )
		return;

	/* setup the message for ch and room */
	switch (trigger)
	{
	case TRIG_ACT_ENTER:
		to_char = str_dup("As you enter in the room, a flash of light blind you for a moment\r\nand you feel transported elsewhere.\r\n");
		to_room = str_dup("As $n enter in the room, a flash of light surrounds $m,\r\nand $e is no longer there.");
		break;
	case TRIG_ACT_EXIT:
		to_char = str_dup("As you approach the exit of the room, a flash of light blind you for a moment\r\nand you feel transported elsewhere.\r\n");
		to_room = str_dup("As $n try to exit from the room, a flash of light surrounds $m,\r\nand $e is no longer there.");
		break;
	case TRIG_ACT_CAST:
		to_char = str_dup("As you cast a spell in the room, a flash of light blind you for a moment\r\nand you feel transported elsewhere.\r\n");
		to_room = str_dup("As $n casts a spell in the room, a flash of light surrounds $m,\r\nand $e is no longer there.");
		break;
	case TRIG_ACT_FIGHT_START:
		to_char = str_dup("As you start fighting in the room, a flash of light blind you for a moment\r\nand you feel transported elsewhere.\r\n");
		to_room = str_dup("As $n start to fight in the room, a flash of light surrounds $m,\r\nand $e is no longer there.");
		break;
	case TRIG_ACT_FIGHT_END:
		to_char = str_dup("As you stop fighting in the room, a flash of light blind you for a moment\r\nand you feel transported elsewhere.\r\n");
		to_room = str_dup("As $n stop to fight in the room, a flash of light surrounds $m,\r\nand $e is no longer there.");
		break;
	case TRIG_ACT_GET:
		to_char = str_dup("As you get something from the room, a flash of light blind you for a moment\r\nand you feel transported elsewhere.\r\n");
		to_room = str_dup("As $n get something in the room, a flash of light surrounds $m,\r\nand $e is no longer there.");
		break;
	case TRIG_ACT_DROP:
		to_char = str_dup("As you drop an item in the room, a flash of light blind you for a moment\r\nand you feel transported elsewhere.\r\n");
		to_room = str_dup("As $n drops an item in the room, a flash of light surrounds $m,\r\nand $e is no longer there.");
		break;
	case TRIG_ACT_REST:
		to_char = str_dup("As you sit down in the room, a flash of light blind you for a moment\r\nand you feel transported elsewhere.\r\n");
		to_room = str_dup("As $n sits down in the room, a flash of light surrounds $m,\r\nand $e is no longer there.");
		break;
	case TRIG_ACT_SLEEP:
		to_char = str_dup("As you start sleeping in the room, a flash of light blind you for a moment\r\nand you feel transported elsewhere.\r\n");
		to_room = str_dup("As $n start sleeping in the room, a flash of light surrounds $m,\r\nand $e is no longer there.");
		break;
	case TRIG_ACT_SPEAK:
		to_char = str_dup("As you speak in the room, a flash of light blind you for a moment\r\nand you feel transported elsewhere.\r\n");
		to_room = str_dup("As $n speaks in the room, a flash of light surrounds $m,\r\nand $e is no longer there.");
		break;
	default:
		to_char = str_dup("As you enter in the room, a flash of light blind you for a moment\r\nand you feel transported elsewhere.\r\n");
		to_room = str_dup("As $n enter in the room, a flash of light surrounds $m,\r\nand $e is no longer there.");
		break;
	}

	if (trigger == TRIG_ACT_EXIT)
		teleport_char(ch, target_room, to_char, to_room);
	else
		activate_teleport(ch, pRoom->trigger->timer, target_room, to_char, to_room);
}

/* ========================================================================= */

void trigger_collapse(CHAR_DATA *ch, ROOM_DATA *pRoom, int trigger)
{
	/* TODO */
}

/* ========================================================================= */

void trigger_explosion(CHAR_DATA *ch, ROOM_DATA *pRoom, int trigger)
{
	/* TODO */
}

/* ========================================================================= */

void activate_heal(CHAR_DATA *ch, int delay, char *to_char, char *to_room)
{
	ROOM_HEAL *rhe;

	CREATE(rhe, ROOM_HEAL, 1);

	rhe->ch				= ch;
	rhe->orig_room		= ch->in_room;
	rhe->delay			= delay;
	rhe->msg_to_char	= to_char;
	rhe->msg_to_room	= to_room;

	event_create(heal_event, rhe, 1);
}


void heal_char(CHAR_DATA *ch, char *to_char, char *to_room)
{
	send_to_char(to_char, ch);
	act(to_room, TRUE, ch, NULL, NULL, TO_ROOM);

	GET_HIT(ch)		= GET_MAX_HIT(ch);
	GET_MANA(ch)	= GET_MAX_MANA(ch);
	GET_MOVE(ch)	= GET_MAX_MOVE(ch);
}


void trigger_heal(CHAR_DATA *ch, ROOM_DATA *pRoom, int trigger)
{
	char *to_char, *to_room;

	/* setup the message for ch and room */
	switch (trigger)
	{
	case TRIG_ACT_ENTER:
		to_char = str_dup("As you enter in the room, a flash of light blind you for a moment\r\nand you feel better.\r\n");
		to_room = str_dup("As $n enter in the room, a flash of light surrounds $m.");
		break;
	case TRIG_ACT_EXIT:
		to_char = str_dup("As you approach the exit of the room, a flash of light blind you for a moment\r\nand you feel better.\r\n");
		to_room = str_dup("As $n try to exit from the room, a flash of light surrounds $m.");
		break;
	case TRIG_ACT_CAST:
		to_char = str_dup("As you cast a spell in the room, a flash of light blind you for a moment\r\nand you feel better.\r\n");
		to_room = str_dup("As $n casts a spell in the room, a flash of light surrounds $m.");
		break;
	case TRIG_ACT_FIGHT_START:
		to_char = str_dup("As you start fighting in the room, a flash of light blind you for a moment\r\nand you feel better.\r\n");
		to_room = str_dup("As $n start to fight in the room, a flash of light surrounds $m.");
		break;
	case TRIG_ACT_FIGHT_END:
		to_char = str_dup("As you stop fighting in the room, a flash of light blind you for a moment\r\nand you feel better.\r\n");
		to_room = str_dup("As $n stop to fight in the room, a flash of light surrounds $m.");
		break;
	case TRIG_ACT_GET:
		to_char = str_dup("As you get something from the room, a flash of light blind you for a moment\r\nand you feel better.\r\n");
		to_room = str_dup("As $n get something in the room, a flash of light surrounds $m.");
		break;
	case TRIG_ACT_DROP:
		to_char = str_dup("As you drop an item in the room, a flash of light blind you for a moment\r\nand you feel better.\r\n");
		to_room = str_dup("As $n drops an item in the room, a flash of light surrounds $m.");
		break;
	case TRIG_ACT_REST:
		to_char = str_dup("As you sit down in the room, a flash of light blind you for a moment\r\nand you feel better.\r\n");
		to_room = str_dup("As $n sits down in the room, a flash of light surrounds $m.");
		break;
	case TRIG_ACT_SLEEP:
		to_char = str_dup("As you start sleeping in the room, a flash of light blind you for a moment\r\nand you feel better.\r\n");
		to_room = str_dup("As $n start sleeping in the room, a flash of light surrounds $m.");
		break;
	case TRIG_ACT_SPEAK:
		to_char = str_dup("As you speak in the room, a flash of light blind you for a moment\r\nand you feel better.\r\n");
		to_room = str_dup("As $n speaks in the room, a flash of light surrounds $m.");
		break;
	default:
		to_char = str_dup("As you enter in the room, a flash of light blind you for a moment\r\nand you feel better.\r\n");
		to_room = str_dup("As $n enter in the room, a flash of light surrounds $m.");
		break;
	}

	if (trigger == TRIG_ACT_EXIT)
		heal_char(ch, to_char, to_room);
	else
		activate_heal(ch, pRoom->trigger->timer, to_char, to_room);
}


/* ========================================================================= */
/* EVENTS																	 */
/* ========================================================================= */

EVENTFUNC(teleport_event)
{
	ROOM_TELEPORT *rte = (ROOM_TELEPORT *) event_obj;
	CHAR_DATA *ch = NULL;
	ROOM_DATA *telep_room, *target_room;
	char to_char[MAX_STRING_LENGTH], to_room[MAX_STRING_LENGTH];

	/* extract data */
	ch			= rte->ch;
	telep_room	= rte->orig_room;
	target_room	= rte->target;

	strcpy(to_char, rte->msg_to_char);
	strcpy(to_room, rte->msg_to_room);

	/* make sure we're supposed to be here */
	if (!ch)
	{
		DISPOSE(event_obj);
		return (0);
	}

	if ( !target_room )
	{
		log("SYSERR: target room for teleport for %s no longer exist.", GET_NAME(ch));
		DISPOSE(event_obj);
		return (0);
	}

	if (rte->delay-- > 0)
		return (1);

	/*
	 * ch is no longer in the teleport room...
	 */
	if (ch->in_room != telep_room)
	{
		DISPOSE(event_obj);
		return (0);
	}

	teleport_char(ch, target_room, to_char, to_room);

	DISPOSE(event_obj);
	return (0);
}


EVENTFUNC(heal_event)
{
	ROOM_HEAL *rhe = (ROOM_HEAL *) event_obj;
	CHAR_DATA *ch = NULL;
	ROOM_DATA *orig_room = NULL;
	char to_char[MAX_STRING_LENGTH], to_room[MAX_STRING_LENGTH];

	/* extract data */
	ch			= rhe->ch;
	orig_room	= rhe->orig_room;

	strcpy(to_char, rhe->msg_to_char);
	strcpy(to_room, rhe->msg_to_room);

	/* make sure we're supposed to be here */
	if (!ch)
	{
		DISPOSE(event_obj);
		return (0);
	}

	if (rhe->delay-- > 0)
		return (1);

	/*
	 * ch is no longer in the healing room...
	 */
	if (ch->in_room != orig_room)
	{
		DISPOSE(event_obj);
		return (0);
	}

	heal_char(ch, to_char, to_room);

	DISPOSE(event_obj);
	return (0);
}
