/**************************************************************************
 * #   #   #   ##   #  #  ###   ##   ##  ###       http://www.lyonesse.it *
 * #    # #   #  #  ## #  #    #    #    #                                *
 * #     #    #  #  # ##  ##    #    #   ##   ## ##  #  #  ##             *
 * #     #    #  #  # ##  #      #    #  #    # # #  #  #  # #            *
 * ###   #     ##   #  #  ###  ##   ##   ###  #   #  ####  ##    Ver. 1.0 *
 *                                                                        *
 * -Based on CircleMud & Smaug-     Copyright (c) 2001-2002 by Mithrandir *
 *                                                                        *
 * ********************************************************************** *
 *                                                                        *
 * File: obj_trap.c                                                       *
 *                                                                        *
 * Trapped items code                                                     *
 *                                                                        *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "spells.h"


/* ======================================================================= */

int parse_trap_dam( char *damname )
{
	if (!str_cmp(damname, "sleep"))		return (TRAP_DAM_SLEEP);
	if (!str_cmp(damname, "teleport"))	return (TRAP_DAM_TELEPORT);
	if (!str_cmp(damname, "fire"))		return (TRAP_DAM_FIRE);
	if (!str_cmp(damname, "cold"))		return (TRAP_DAM_COLD);
	if (!str_cmp(damname, "acid"))		return (TRAP_DAM_ACID);
	if (!str_cmp(damname, "energy"))	return (TRAP_DAM_ENERGY);
	if (!str_cmp(damname, "blunt"))		return (TRAP_DAM_BLUNT);
	if (!str_cmp(damname, "pierce"))	return (TRAP_DAM_PIERCE);
	if (!str_cmp(damname, "slash"))		return (TRAP_DAM_SLASH);

	return (TRAP_DAM_NONE);
}


void trap_damage(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_TRAP_DATA *trap)
{
	CHAR_DATA *wch;
	int dam = 0, level;
	
	act("You hear a strange noise......", FALSE, ch, 0, 0, TO_ROOM);
	act("You hear a strange noise......", FALSE, ch, 0, 0, TO_CHAR);

	trap->charges--;

	level = URANGE(1, GET_OBJ_LEVEL(obj), LVL_IMMORT);

	switch (trap->dam_type)
	{
	case TRAP_DAM_SLEEP: 
		if ( !trap->whole_room )
		{
			if (AFF_FLAGGED(ch, AFF_SLEEP))
				return;
			call_magic(ch, ch, NULL, SPELL_SLEEP, level, CAST_OBJ);
		}
		else
		{
			for (wch = ch->in_room->people; wch; wch = wch->next_in_room)
			{
				if (AFF_FLAGGED(wch, AFF_SLEEP) || IS_IMMORTAL(wch))
					continue;
				call_magic(wch, wch, NULL, SPELL_SLEEP, level, CAST_OBJ);
			} 
		}
		return;

	case TRAP_DAM_FIRE:
		if ( !trap->whole_room )
		{
			act("A fireball shoots out of $p and hits $n!", FALSE, ch, obj, 0, TO_ROOM);
			act("A fireball shoots out of $p and hits you!", FALSE, ch, obj, 0, TO_CHAR);
		}
		else
		{
			act("A fireball shoots out of $p and hits everyone in the room!", FALSE, ch, obj, 0, TO_ROOM);
			act("A fireball shoots out of $p and hits everyone in the room!", FALSE, ch, obj, 0, TO_CHAR);
		}
		dam = level * 4;
		break;
		
		
	case TRAP_DAM_COLD:
		if ( !trap->whole_room )
		{
			act("A blast of frost from $p hits $n!", FALSE, ch, obj, 0, TO_ROOM);
			act("A blast of frost from $p hits you!", FALSE, ch, obj, 0, TO_CHAR);
		}
		else
		{
			act("A blast of frost from $p fills the room freezing you!", FALSE, ch, obj, 0, TO_ROOM);
			act("A blast of frost from $p fills the room freezing you!", FALSE, ch, obj, 0, TO_CHAR);
		}
		dam = level * 5;
		break;
		
	case TRAP_DAM_ACID:
		if ( !trap->whole_room )
		{
			act("A blast of acid erupts from $p, burning your skin!", FALSE, ch, obj, 0, TO_CHAR);
			act("A blast of acid erupts from $p, burning $n's skin!", FALSE, ch, obj, 0, TO_ROOM);
		}
		else
		{
			act("A blast of acid erupts from $p, burning your skin!", FALSE, ch, obj, 0, TO_ROOM);
			act("A blast of acid erupts from $p, burning your skin!", FALSE, ch, obj, 0, TO_CHAR);
		}
		dam = level * 6;
		break;
		
	case TRAP_DAM_ENERGY:
		if ( !trap->whole_room )
		{
			act("A pulse of energy from $p zaps $n!", FALSE, ch, obj, 0, TO_ROOM);
			act("A pulse of energy from $p zaps you!", FALSE, ch, obj, 0, TO_CHAR);
		}
		else
		{
			act("A pulse of energy from $p zaps you!", FALSE, ch, obj, 0, TO_ROOM);
			act("A pulse of energy from $p zaps you!", FALSE, ch, obj, 0, TO_CHAR);
		}
		dam = level * 3;
		break;
		
	case TRAP_DAM_BLUNT:
		if ( !trap->whole_room )
		{
			act("$n sets off a trap on $p and is hit by a blunt object!", FALSE, ch, obj, 0, TO_ROOM);
			act("You are hit by a blunt object from $p!", FALSE, ch, obj, 0, TO_CHAR);
		}
		else
		{
			act("$n sets off a trap on $p and you are hit by a flying object!", FALSE, ch, obj, 0, TO_ROOM);
			act("You are hit by a blunt object from $p!", FALSE, ch, obj, 0, TO_CHAR);
		}
		dam = (10 * level) + GET_AC(ch);
		break;
	
	case TRAP_DAM_PIERCE:
		if ( !trap->whole_room )
		{
			act("$n sets of a trap on $p and is pierced in the chest!", FALSE, ch, obj, 0, TO_ROOM);
			act("You set off a trap on $p and are pierced through the chest!", FALSE, ch, obj, 0, TO_CHAR);
		}
		else
		{
			act("$n sets off a trap on $p and you are hit by a piercing object!", FALSE, ch, obj, 0, TO_ROOM); 
			act("You set off a trap on $p and are pierced through the chest!", FALSE, ch, obj, 0, TO_CHAR);
		} 
		dam = (10 * level) + GET_AC(ch);
		break;

	case TRAP_DAM_SLASH:
		if ( !trap->whole_room )
		{
			act("$n just got slashed by a trap on $p.", FALSE, ch, obj, 0, TO_ROOM);
			act("You just got slashed by a trap on $p!", FALSE, ch, obj, 0, TO_CHAR);
		}
		else
		{
			act("$n set off a trap releasing a blade that slashes you!", FALSE, ch, obj, 0, TO_ROOM);
			send_to_char("You set off a trap releasing blades around the room..\r\n", ch);
			send_to_char("One of the blades slashes you in the chest!\r\n", ch);
		}
		dam = (10 * level) + GET_AC(ch);
		break;
	default:
		return;
	}
	
	/* 
	 * Do the damage
	 */
	if ( !trap->whole_room )
	{
		damage(ch, ch, dam, TYPE_UNDEFINED);
	}
	else
	{
		CHAR_DATA *wch_next = NULL;

		for (wch = ch->in_room->people; wch; wch = wch_next)
		{
			wch_next = wch->next_in_room;

			if (trap->dam_type == TRAP_DAM_BLUNT  ||
			    trap->dam_type == TRAP_DAM_PIERCE ||
			    trap->dam_type == TRAP_DAM_SLASH)
				dam = (10 * level) + GET_AC(wch);
			
			damage(wch, wch, dam, TYPE_UNDEFINED);

		}
	}
}

bool check_trap(CHAR_DATA *ch, OBJ_DATA *obj, int action)
{
	OBJ_TRAP_DATA *trap;
	bool trig = FALSE;

	if (!ch || !obj)
		return (FALSE);

	if (!obj->special || !OBJ_FLAGGED(obj, ITEM_HAS_TRAPS))
		return (FALSE);

	for (trap = (OBJ_TRAP_DATA *) obj->special; trap; trap = trap->next)
	{
		if (IS_SET(trap->action, action))
			break;
	}

	if (!trap)
		return (FALSE);

	if (trap->charges <= 0)
		return (FALSE);

	trap_damage(ch, obj, trap);

	return (TRUE);
}

/* ======================================================================= */

ACMD(do_trapremove)
{
	OBJ_DATA *obj = NULL;
	OBJ_TRAP_DATA *trap_list, *trap, *next_trap = NULL, *temp;

	argument = one_argument(argument, arg);

	if (!GET_SKILL(ch, SKILL_DISARM_TRAPS))
	{
		send_to_char("You don't know how to disarm traps.\r\n", ch);
		return;
	}

	if (!*arg)
	{
		send_to_char("From which object do you want to remove a trap?\r\n", ch);
		return;
	}

	if ( !( obj = get_obj_in_list_vis_rev(ch, arg, NULL, ch->in_room->last_content) ) )
	{
		ch_printf(ch, "You don't see %s %s here.\r\n", AN(arg), arg);
		return;
	}

	if ( !obj->special || !OBJ_FLAGGED(obj, ITEM_HAS_TRAPS) )
	{
		ch_printf(ch, "You don't find any traps on %s.\r\n", obj->short_description);
		return;
	}

	for ( trap = ( trap_list = (OBJ_TRAP_DATA *)obj->special ); trap; trap = next_trap )
	{
		next_trap = trap->next;

		if (success(ch, NULL, SKILL_DISARM_TRAPS, 0))
		{
			send_to_char("You successfully remove the trap.\r\n", ch);
			REMOVE_FROM_LIST(trap, trap_list, next);
			trap->next = NULL;
			DISPOSE(trap);
		}
		else
		{
			send_to_char("You set off the trap!\r\n", ch);
			if (trap->charges > 0)
				trap_damage(ch, obj, trap);
			else
				send_to_char("Luckily, it has no more charges.\r\n", ch);
		}
	}

	// if there is no traps left on the object, remove the trapped flag
	if (!(obj->special = trap_list))
		REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_HAS_TRAPS);
}


ACMD(do_trapstat)
{
	OBJ_DATA *obj;
	OBJ_TRAP_DATA *trap;
	char tbuf[MAX_STRING_LENGTH], aflags[MAX_STRING_LENGTH];

	if (!GET_SKILL(ch, SKILL_DISARM_TRAPS))
	{
		send_to_char("You have no idea on how to handle traps.\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg);
	
	if (!*arg)
	{
		send_to_char("Which object do you want to analyze?\r\n", ch);
		return;
	}
	
	// look first in room ...
	if (!(obj = get_obj_in_list_vis_rev(ch, str_dup(arg), NULL, ch->in_room->last_content)))
	{
		// ... and then in inventory
		if (!(obj = get_obj_in_list_vis_rev(ch, str_dup(arg), NULL, ch->last_carrying)))
		{
			sprintf(buf, "You don't see %s %s here.\r\n", AN(arg), arg);
			send_to_char(buf, ch);
			return;
		}
	}

	if ( !obj->special || !OBJ_FLAGGED(obj, ITEM_HAS_TRAPS) )
	{
		send_to_char("That object has no trap.\r\n", ch);
		return;
	}

	send_to_char("&b&6Object is trapped:&0\r\n", ch);

	for ( trap = (OBJ_TRAP_DATA *)obj->special; trap; trap = trap->next )
	{
		sprintbit( trap->action, trap_act_descr, aflags );
		sprintf(tbuf, " Dam Type: %s, Charges: %d, Trigger: %s\r\n",
			trap_dam_descr[trap->dam_type], trap->charges, aflags);
		send_to_char(tbuf, ch);
	}
}

// Imm command
ACMD(do_traplist)
{
	OBJ_DATA *obj;
	bool found = FALSE;
	
	for (obj = first_object; obj; obj = obj->next)
	{
		
		if ( !obj->special || !OBJ_FLAGGED(obj, ITEM_HAS_TRAPS) || !CAN_SEE_OBJ(ch, obj))
			continue;
		
		found = TRUE;
		
		if (obj->carried_by)
			sprintf(buf, "%s is being carried by %s.\r\n",
				obj->short_description, PERS(obj->carried_by, ch));
		else if (obj->in_room != NULL)
			sprintf(buf, "%s is in %s.\r\n", obj->short_description,
				ROOM_NAME(obj));
		else if (obj->in_obj)
			sprintf(buf, "%s is in %s.\r\n", obj->short_description,
				obj->in_obj->short_description);
		else if (obj->worn_by)
			sprintf(buf, "%s is being worn by %s.\r\n",
				obj->short_description, PERS(obj->worn_by, ch));
		else
			sprintf(buf, "%s's location is uncertain.\r\n",
				obj->short_description);
		
		CAP(buf);
		send_to_char(buf, ch);
	}
	
	if (!found)
		send_to_char("No trapped items found.\r\n", ch);
	
	return;
}

