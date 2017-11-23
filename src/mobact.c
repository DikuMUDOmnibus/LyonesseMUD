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
*   File: mobact.c                                      Part of CircleMUD *
*  Usage: Functions for generating intelligent (?) behavior in mobiles    *
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
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"
#include "constants.h"


/* external globals */
extern int no_specials;

/* external functions */
ACMD(do_say);
ACMD(do_get);
ACMD(do_action);
int	hunt_victim(CHAR_DATA *ch);
int	compute_armor_class(CHAR_DATA *ch);

/* local functions */
void mobile_activity(void);
void clearMemory(CHAR_DATA *ch);
void check_switch_opponent(CHAR_DATA *ch);
bool aggressive_mob_on_a_leash(CHAR_DATA *slave, CHAR_DATA *master, CHAR_DATA *attack);

#define MOB_AGGR_TO_ALIGN (MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD)


void mobile_activity(void)
{
	CHAR_DATA *ch, *next_ch, *vict;
	OBJ_DATA *obj, *best_obj;
	MEMORY_REC *names;
	int found, max;
	
	for (ch = character_list; ch; ch = next_ch)
	{
		next_ch = ch->next;
		
		if (!IS_MOB(ch))
			continue;
		
		/* Examine call for special procedure */
		if (MOB_FLAGGED(ch, MOB_SPEC) && !no_specials)
		{
			if (mob_index[GET_MOB_RNUM(ch)].func == NULL)
			{
				log("SYSERR: %s (#%d): Attempting to call non-existing mob function.",
					GET_NAME(ch), GET_MOB_VNUM(ch));
				REMOVE_BIT(MOB_FLAGS(ch), MOB_SPEC);
			}
			else
			{
				char actbuf[MAX_INPUT_LENGTH] = "";
				if ((mob_index[GET_MOB_RNUM(ch)].func) (ch, ch, 0, actbuf))
					continue;		/* go to next char */
			}
		}
		
		/* If the mob is asleep exit here.. */
		if (!AWAKE(ch))
			continue;
		
		/* let fighting mob be smarter */
		if (FIGHTING(ch))
		{
			check_switch_opponent(ch);
			continue;
		}

		/* take care of encountered mobs */
		if (MOB_FLAGGED(ch, MOB_ENCOUNTER) && GET_POS(ch) == POS_STANDING)
		{
			if (--ch->mob_specials.timer < 1)
			{
				act("$n sneaks away.", FALSE, ch, NULL, NULL, TO_ROOM);
				extract_char(ch);
				continue;
			}
		}

		/* Scavenger (picking up objects) */
		if (MOB_FLAGGED(ch, MOB_SCAVENGER) && !FIGHTING(ch) && AWAKE(ch))
		{
			if (ch->in_room->first_content && !number(0, 10))
			{
				max = 1;
				best_obj = NULL;
				for (obj = ch->in_room->first_content; obj; obj = obj->next_content)
				{
					if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max)
					{
						best_obj = obj;
						max = GET_OBJ_COST(obj);
					}
				}
				if (best_obj != NULL)
				{
					obj_from_room(best_obj);
					best_obj = obj_to_char(best_obj, ch);
					act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);
				}
			}
		}
		
		/* Aggressive Mobs */
		if (MOB_FLAGGED(ch, MOB_AGGRESSIVE | MOB_AGGR_TO_ALIGN))
		{
			found = FALSE;
			for (vict = ch->in_room->people; vict && !found; vict = vict->next_in_room)
			{
				if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
					continue;
				if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
					continue;
				if (MOB_FLAGGED(ch, MOB_AGGRESSIVE) ||
					(MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
					(MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
					(MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict)))
				{
					/* Can a master successfully control the charmed monster? */
					if (aggressive_mob_on_a_leash(ch, ch->master, vict))
						continue;

					hit(ch, vict, TYPE_UNDEFINED);
					found = TRUE;
				}
			}
		}
		
		/* Mob Memory */
		if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch))
		{
			found = FALSE;
			for (vict = ch->in_room->people; vict && !found; vict = vict->next_in_room)
			{
				if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
					continue;
				
				for (names = MEMORY(ch); names && !found; names = names->next)
				{
					if (names->id != GET_IDNUM(vict))
						continue;
					
					/* Can a master successfully control the charmed monster? */
					if (aggressive_mob_on_a_leash(ch, ch->master, vict))
						continue;
					
					found = TRUE;
					act("'Hey!  You're the fiend that attacked me!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
					hit(ch, vict, TYPE_UNDEFINED);
				}
			}
		}
		
		/*
		 * Charmed Mob Rebellion
		 *
		 * In order to rebel, there need to be more charmed monsters
		 * than the person can feasibly control at a time.  Then the
		 * mobiles have a chance based on the charisma of their leader.
		 *
		 * 1-4 = 0, 5-7 = 1, 8-10 = 2, 11-13 = 3, 14-16 = 4, 17-19 = 5, etc.
 		 */
		if (AFF_FLAGGED(ch, AFF_CHARM) && ch->master && num_followers_charmed(ch->master) > (GET_CHA(ch->master) - 2) / 3)
		{
			if (!aggressive_mob_on_a_leash(ch, ch->master, ch->master))
			{
				if (CAN_SEE(ch, ch->master) && !PRF_FLAGGED(ch->master, PRF_NOHASSLE))
					hit(ch, ch->master, TYPE_UNDEFINED);
				stop_follower(ch);
			}
		}

		/* Helper Mobs */
		if (MOB_FLAGGED(ch, MOB_HELPER) && !AFF_FLAGGED(ch, AFF_BLIND | AFF_CHARM))
		{
			found = FALSE;
			for (vict = ch->in_room->people; vict && !found; vict = vict->next_in_room)
			{
				if (ch == vict || !IS_NPC(vict) || !FIGHTING(vict))
					continue;
				if (IS_NPC(FIGHTING(vict)) || ch == FIGHTING(vict))
					continue;
				
				act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
				hit(ch, FIGHTING(vict), TYPE_UNDEFINED);
				found = TRUE;
			}
		}

		/* Hunting Mobs */
		if (AFF_FLAGGED(ch, AFF_HUNTING) && HUNTING(ch) && !FIGHTING(ch))
		{
			if (hunt_victim(ch))
				continue;
		}

		/* Mob Movement */
		if (!MOB_FLAGGED(ch, MOB_SENTINEL) && !MOB_FLAGGED(ch, MOB_ENCOUNTER) && GET_POS(ch) == POS_STANDING)
		{
			int door, max_doors;
			
			if (IN_WILD(ch))
			{
				door = number(0, 30);
				max_doors = NUM_OF_DIRS;
			}
			else
			{
				door = number(0, 18);
				max_doors = NUM_OF_EX_DIRS;
			}
			
			if (door < max_doors)
			{
				EXIT_DATA *pexit = get_exit(ch->in_room, door);
				if (pexit && pexit->to_room && !ROOM_FLAGGED(pexit->to_room, ROOM_NOMOB | ROOM_DEATH))
				{
					if (!MOB_FLAGGED(ch, MOB_STAY_ZONE) || 
						(pexit->to_room->zone == ch->in_room->zone))
					{
						perform_move(ch, door, 1);
					}
				}
			}
		}

		/* Add new mobile actions here */
	}
}



/* Mob Memory Routines */

/* make ch remember victim */
void remember(CHAR_DATA *ch, CHAR_DATA *victim)
{
	MEMORY_REC *tmp;
	bool present = FALSE;
	
	if (!IS_NPC(ch) || IS_NPC(victim) || PRF_FLAGGED(victim, PRF_NOHASSLE))
		return;
	
	for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
	{
		if (tmp->id == GET_IDNUM(victim))
			present = TRUE;
	}
	
	if (!present)
	{
		CREATE(tmp, MEMORY_REC, 1);
		tmp->next = MEMORY(ch);
		tmp->id = GET_IDNUM(victim);
		MEMORY(ch) = tmp;
	}
}


/* make ch forget victim */
void forget(CHAR_DATA *ch, CHAR_DATA *victim)
{
	MEMORY_REC *curr, *prev = NULL;
	
	if (!(curr = MEMORY(ch)))
		return;
	
	while (curr && curr->id != GET_IDNUM(victim))
	{
		prev = curr;
		curr = curr->next;
	}
	
	if (!curr)
		return;			/* person wasn't there at all. */
	
	if (curr == MEMORY(ch))
		MEMORY(ch) = curr->next;
	else
		prev->next = curr->next;
	
	free(curr);
}


/* erase ch's memory */
void clearMemory(CHAR_DATA *ch)
{
	MEMORY_REC *curr, *next;
	
	curr = MEMORY(ch);
	
	while (curr)
	{
		next = curr->next;
		free(curr);
		curr = next;
	}
	
	MEMORY(ch) = NULL;
}

/*
 * Npc taunts slayed player characters.  Text goes out through gossip
 * channel.  Muerte - Telnet://betterbox.net:4000
 */
void brag(CHAR_DATA *ch, CHAR_DATA *vict)
{
	DESCRIPTOR_DATA *i;
	char buf[MAX_STRING_LENGTH], brag[MAX_STRING_LENGTH];
	
	switch (number(0, 11))
	{
	case 0:
		sprintf(brag, "$n brags, '%s was just too easy a kill!'", GET_NAME(vict));
		break;
	case 1:
		sprintf(brag, "$n brags, '%s was a tasty dinner, now who's for desert? Muhaha!'", GET_NAME(vict));
		break;
	case 2:
		sprintf(brag, "$n brags, 'Bahaha! %s should stick to Odif's !'",GET_NAME(vict));
		break;
	case 3:
		sprintf(brag, "$n brags, '%s is now in need of some exp...Muhaha!'", GET_NAME(vict));
		break;
	case 4:
		sprintf(brag, "$n brags, '%s needs a hospital now. Muhaha!'",GET_NAME(vict));
		break;
	case 5:
		sprintf(brag, "$n brags, '%s's mother is a slut!  Muhaha!'", GET_NAME(vict));
		break;
	case 6:
		sprintf(brag, "$n brags, '%s is a punk, hits like a swampfly. Bah.'", GET_NAME(vict));
		break;
	case 7:
		sprintf(brag, "$n brags, '%s, your life force has just run out...Muahaha!'", GET_NAME(vict));
		break;
	case 8:
		sprintf(brag, "$n brags, 'Bah, %s should stick to the newbie zone!'",GET_NAME(vict));
		break;
	case 9:
		sprintf(brag, "$n brags, '%s, give me your daughter's number and I might return your corpse.  Muhaha!'", GET_NAME(vict));
		break;
	case 10:
		sprintf(brag, "$n brags, 'Hey %s!  Come back, you dropped your corpse! Muahaha'", GET_NAME(vict));
		break;
	case 11:
		sprintf(brag, "$n brags, 'I think %s wears pink chainmail.  Fights like a girl!  Muhaha!'", GET_NAME(vict));
		break;
	}

	sprintf(buf, "&b&1%s&0", brag);

	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
			!PRF_FLAGGED(i->character, PRF_NOGOSS) &&
			!PLR_FLAGGED(i->character, PLR_WRITING) &&
			!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF))
		{
			act(brag, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
		}
	}

}

void check_switch_opponent(CHAR_DATA *ch)
{
	CHAR_DATA *tmp, *newone = NULL;
	OBJ_DATA *wielded;
	int best = 0, weight = 0;
	int dam1, dam2;

	for (tmp = ch->in_room->people; tmp; tmp = tmp->next_in_room)
	{
		if (tmp == ch || tmp == FIGHTING(ch) || IS_NPC(tmp) || FIGHTING(tmp) != ch)
			continue;

		switch (GET_CLASS(tmp))
		{
		case CLASS_WARRIOR:		weight = -2;	break;
		case CLASS_THIEF:		weight = 0;		break;
		case CLASS_CLERIC:		weight = 2;		break;
		case CLASS_SORCERER:	weight = 4;		break;
		case CLASS_MAGIC_USER:	weight = 6;		break;
		}

		/* compares current opponent and a potential new one */
		weight += GET_LEVEL(FIGHTING(ch)) - GET_LEVEL(tmp);
		weight += GET_REAL_HITROLL(FIGHTING(ch)) - GET_REAL_HITROLL(tmp);
		weight += GET_REAL_DAMROLL(FIGHTING(ch)) - GET_REAL_DAMROLL(tmp);

		if ( compute_armor_class(FIGHTING(ch)) > compute_armor_class(tmp) )
			weight += 10;
		else
			weight -= 10;

		/* calculate max amount of damage of weapons */
		if ( (wielded = GET_EQ(FIGHTING(ch), WEAR_WIELD)) && GET_OBJ_TYPE(wielded) == ITEM_WEAPON )
			dam1 = GET_OBJ_VAL(wielded, 2) * GET_OBJ_VAL(wielded, 1);
		if ( (wielded = GET_EQ(tmp, WEAR_WIELD)) && GET_OBJ_TYPE(wielded) == ITEM_WEAPON )
			dam2 = GET_OBJ_VAL(wielded, 2) * GET_OBJ_VAL(wielded, 1);
		weight += dam1 - dam2;

		if (AFF_FLAGGED(FIGHTING(ch), AFF_SANCTUARY))
			weight += 10;
		if (AFF_FLAGGED(FIGHTING(ch), AFF_FIRESHIELD))
			weight += 50;

		if (AFF_FLAGGED(tmp, AFF_SANCTUARY))
			weight -= 10;
		if (AFF_FLAGGED(tmp, AFF_FIRESHIELD))
			weight -= 50;

		if ( weight > best )
		{
			best = weight;
			newone = tmp;
		}
	}
	
	/* have we found an easier opponent ? */
	if (newone)
	{
		act("$n sees that $N has joined the fight, and decides $E is a much better prey!",
			FALSE, ch, NULL, newone, TO_ROOM );
		do_say(ch, "Easier meat, ha ha!", 0, 0 );
		stop_fighting(ch);
		set_fighting(ch, newone);
	}
}


/*
 * An aggressive mobile wants to attack something.  If
 * they're under the influence of mind altering PC, then
 * see if their master can talk them out of it, eye them
 * down, or otherwise intimidate the slave.
 */
bool aggressive_mob_on_a_leash(CHAR_DATA *slave, CHAR_DATA *master, CHAR_DATA *attack)
{
	static int snarl_cmd;
	int dieroll;
	
	if (!master || !AFF_FLAGGED(slave, AFF_CHARM))
		return (FALSE);
	
	if (!snarl_cmd)
		snarl_cmd = find_command("snarl");
	
	/* Sit. Down boy! HEEEEeeeel! */
	dieroll = number(1, 20);
	if (dieroll != 1 && (dieroll == 20 || dieroll > 10 - GET_CHA(master) + GET_INT(slave)))
	{
		if (snarl_cmd && attack && !number(0, 3))
		{
			char victbuf[MAX_NAME_LENGTH + 1];
			
			strncpy(victbuf, GET_NAME(attack), sizeof(victbuf));
			victbuf[sizeof(victbuf) - 1] = '\0';
			
			do_action(slave, victbuf, snarl_cmd, 0);
		}
		
		/* Success! But for how long? Hehe. */
		return (TRUE);
	}
	
	/* So sorry, now you're a player killer... Tsk tsk. */
	return (FALSE);
}

