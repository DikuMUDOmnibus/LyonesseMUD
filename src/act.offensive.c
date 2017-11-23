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
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
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
#include "constants.h"

/* extern functions */
int		compute_armor_class(CHAR_DATA *ch);
void	raw_kill(CHAR_DATA *ch);
void	check_killer(CHAR_DATA *ch, CHAR_DATA *vict);
void	fire_missile(CHAR_DATA *ch, char *arg, OBJ_DATA *missile, int pos, int range, EXIT_DATA *pexit);
bool	attack_building(CHAR_DATA *ch, char *arg);
bool	attack_vehicle(CHAR_DATA *ch, char *arg);

/* local functions */
ACMD(do_assist);
ACMD(do_hit);
ACMD(do_kill);
ACMD(do_backstab);
ACMD(do_order);
ACMD(do_flee);
ACMD(do_bash);
ACMD(do_rescue);
ACMD(do_kick);


ACMD(do_assist)
{
	CHAR_DATA *helpee, *opponent;
	
	if (FIGHTING(ch))
	{
		send_to_char("You're already fighting!  How can you assist someone else?\r\n", ch);
		return;
	}
	one_argument(argument, arg);
	
	if (!*arg)
		send_to_char("Whom do you wish to assist?\r\n", ch);
	else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
		send_to_char(NOPERSON, ch);
	else if (helpee == ch)
		send_to_char("You can't help yourself any more than this!\r\n", ch);
	else
	{
		/*
		 * Hit the same enemy the person you're helping is.
		 */
		if (FIGHTING(helpee))
			opponent = FIGHTING(helpee);
		else
			for (opponent = ch->in_room->people;
				opponent && (FIGHTING(opponent) != helpee);
				opponent = opponent->next_in_room);
			
		if (!opponent)
			act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
		else if (!CAN_SEE(ch, opponent))
			act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
		else if (!pkill_ok(ch, opponent))	/* prevent accidental pkill */
			act("Use 'murder' if you really want to attack $N.", FALSE, ch, 0, opponent, TO_CHAR);
		else
		{
			send_to_char("You join the fight!\r\n", ch);
			act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
			act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
			hit(ch, opponent, TYPE_UNDEFINED);
			/* Room Trigger Events */
			if ( ch->in_room->trigger )
				check_room_trigger(ch, TRIG_ACT_FIGHT_START);
		}
	}
}


ACMD(do_hit)
{
	CHAR_DATA *vict;
	char arg[MAX_STRING_LENGTH];
	
	one_argument(argument, arg);
	
	if (!*arg)
	{
		send_to_char("Hit who or what?\r\n", ch);
		return;
	}
	
	// first, check for mobs or players
	if (!(vict = get_char_vis(ch, str_dup(arg), NULL, FIND_CHAR_ROOM)))
	{
		// otherwise, check for buildings
		if ( attack_building(ch, str_dup(arg)) )
			return;

		if ( attack_vehicle(ch, str_dup(arg)) )
			return;

		send_to_char("They don't seem to be here.\r\n", ch);
		return;
	}

	if (vict == ch)
	{
		send_to_char("You hit yourself...OUCH!.\r\n", ch);
		act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
		return;
	}

	if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
	{
		act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	
	if (!pkill_ok(ch, vict))
	{
		if (!IS_NPC(vict) && !IS_NPC(ch))
		{
			if (subcmd != SCMD_MURDER)
			{
				send_to_char("Use 'murder' to hit another player.\r\n", ch);
				return;
			}
			else
				check_killer(ch, vict);
		}
		/* you can't order a charmed pet to attack a player */
		if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
			return;
	}

	if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch)))
	{
		hit(ch, vict, TYPE_UNDEFINED);
		WAIT_STATE(ch, PULSE_VIOLENCE + 2);
		/* Room Trigger Events */
		if ( ch->in_room->trigger )
			check_room_trigger(ch, TRIG_ACT_FIGHT_START);
	}
	else
		send_to_char("You do the best you can!\r\n", ch);
}



ACMD(do_kill)
{
	CHAR_DATA *vict;
	
	if ((GET_LEVEL(ch) < LVL_IMPL) || IS_NPC(ch))
	{
		do_hit(ch, argument, cmd, subcmd);
		return;
	}
	one_argument(argument, arg);
	
	if (!*arg)
		send_to_char("Kill who?\r\n", ch);
	else
	{
		if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
			send_to_char("They aren't here.\r\n", ch);
		else if (ch == vict)
			send_to_char("Your mother would be so sad.. :(\r\n", ch);
		else
		{
			act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
			act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
			act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
			raw_kill(vict);
		}
	}
}



ACMD(do_backstab)
{
	CHAR_DATA *vict;

	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BACKSTAB))
	{
		send_to_char("You have no idea how to do that.\r\n", ch);
		return;
	}

	one_argument(argument, buf);

	if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
	{
		send_to_char("Backstab who?\r\n", ch);
		return;
	}
	if (vict == ch)
	{
		send_to_char("How can you sneak up on yourself?\r\n", ch);
		return;
	}
	if (!GET_EQ(ch, WEAR_WIELD))
	{
		send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
		return;
	}
	if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_PIERCE - TYPE_HIT)
	{
		send_to_char("Only piercing weapons can be used for backstabbing.\r\n", ch);
		return;
	}
	if (FIGHTING(vict))
	{
		send_to_char("You can't backstab a fighting person -- they're too alert!\r\n", ch);
		return;
	}

	if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict))
	{
		act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
		act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
		act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
		hit(vict, ch, TYPE_UNDEFINED);
		return;
	}

	if ( AWAKE(vict) && !success(ch, vict, SKILL_BACKSTAB, 0) )
		damage(ch, vict, 0, SKILL_BACKSTAB);
	else
	{
		hit(ch, vict, SKILL_BACKSTAB);
		/* Room Trigger Events */
		if ( ch->in_room->trigger )
			check_room_trigger(ch, TRIG_ACT_FIGHT_START);
	}

	WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
}


ACMD(do_order)
{
	CHAR_DATA *vict;
	FOLLOW_DATA *k;
	char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
	bool found = FALSE;
	
	half_chop(argument, name, message);
	
	if (!*name || !*message)
		send_to_char("Order who to do what?\r\n", ch);
	else if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM)) && !is_abbrev(name, "followers"))
		send_to_char("That person isn't here.\r\n", ch);
	else if (ch == vict)
		send_to_char("You obviously suffer from skitzofrenia.\r\n", ch);
	else
	{
		if (AFF_FLAGGED(ch, AFF_CHARM))
		{
			send_to_char("Your superior would not aprove of you giving orders.\r\n", ch);
			return;
		}
		if (vict)
		{
			sprintf(buf, "$N orders you to '%s'", message);
			act(buf, FALSE, vict, 0, ch, TO_CHAR);
			act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);
			
			if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
				act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
			else
			{
				send_to_char(OK, ch);
				command_interpreter(vict, message);
			}
		}
		else			/* This is order "followers" */
		{
			sprintf(buf, "$n issues the order '%s'.", message);
			act(buf, FALSE, ch, 0, 0, TO_ROOM);
			
			for (k = ch->followers; k; k = k->next)
			{
				if (IN_ROOM(ch) == IN_ROOM(k->follower))
					if (AFF_FLAGGED(k->follower, AFF_CHARM))
					{
						found = TRUE;
						command_interpreter(k->follower, message);
					}
			}
			if (found)
				send_to_char(OK, ch);
			else
				send_to_char("Nobody here is a loyal subject of yours!\r\n", ch);
		}
	}
}



ACMD(do_flee)
{
	EXIT_DATA *pexit;
	CHAR_DATA *was_fighting;
	int i, attempt, loss;
	
	if (GET_POS(ch) < POS_FIGHTING)
	{
		send_to_char("You are in pretty bad shape, unable to flee!\r\n", ch);
		return;
	}
	
	for (i = 0; i < 6; i++)
	{
		attempt = number( 0, NUM_OF_DIRS - 1 );	/* Select a random direction */
		pexit = get_exit( ch->in_room, attempt );

		if (pexit && valid_exit(pexit) && !ROOM_FLAGGED(pexit->to_room, ROOM_DEATH))
		{
			act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);

			was_fighting = FIGHTING(ch);

			if (do_simple_move(ch, attempt, TRUE))
			{
				send_to_char("You flee head over heels.\r\n", ch);
				
				if (was_fighting && !IS_NPC(ch))
				{
					loss = GET_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
					loss *= GET_LEVEL(was_fighting);
					gain_exp(ch, -loss);
					
					if (IS_NPC(was_fighting) && MOB_FLAGGED(was_fighting, MOB_HUNTER))
					{
						HUNTING(was_fighting) = ch;
						SET_BIT(AFF_FLAGS(was_fighting), AFF_HUNTING);
					}
				}
			}
			else
				act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
			return;
		}
	}
	send_to_char("PANIC!  You couldn't escape!\r\n", ch);
}


ACMD(do_bash)
{
	CHAR_DATA *vict;
	bool check = TRUE;
	
	one_argument(argument, arg);
	
	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BASH))
	{
		send_to_char("You have no idea how.\r\n", ch);
		return;
	}
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
	{
		send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
		return;
	}
	if (!GET_EQ(ch, WEAR_WIELD))
	{
		send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
		return;
	}

	if ( FIGHTING(ch) )
		check = FALSE;

	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
	{
		if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
			vict = FIGHTING(ch);
		else
		{
			send_to_char("Bash who?\r\n", ch);
			return;
		}
	}
	if ( RIDING(vict) )
	{
		send_to_char("Mounted targets are impossible to bash!\r\n", ch);
		return;
	}
	if (vict == ch)
	{
		send_to_char("Aren't we funny today...\r\n", ch);
		return;
	}

	if ( !success(ch, vict, SKILL_BASH, 0) )
	{
		damage(ch, vict, 0, SKILL_BASH);
		GET_POS(ch) = POS_SITTING;
	}
	else
	{
		/*
		 * If we bash a player and they wimp out, they will move to the previous
		 * room before we set them sitting.  If we try to set the victim sitting
		 * first to make sure they don't flee, then we can't bash them!  So now
		 * we only set them sitting if they didn't flee. -gg 9/21/98
		 */
		if (damage(ch, vict, 1, SKILL_BASH) > 0)	/* -1 = dead, 0 = miss */
		{
			WAIT_STATE(vict, PULSE_VIOLENCE);
			if (IN_ROOM(ch) == IN_ROOM(vict))
				GET_POS(vict) = POS_SITTING;
		}
	}

	WAIT_STATE(ch, PULSE_VIOLENCE * 2);

	/* Room Trigger Events */
	if ( check && ch->in_room->trigger )
		check_room_trigger(ch, TRIG_ACT_FIGHT_START);
}


ACMD(do_rescue)
{
	CHAR_DATA *vict, *tmp_ch;
	
	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_RESCUE))
	{
		send_to_char("You have no idea how to do that.\r\n", ch);
		return;
	}
	
	one_argument(argument, arg);
	
	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
	{
		send_to_char("Whom do you want to rescue?\r\n", ch);
		return;
	}
	if (vict == ch)
	{
		send_to_char("What about fleeing instead?\r\n", ch);
		return;
	}
	if (FIGHTING(ch) == vict)
	{
		send_to_char("How can you rescue someone you are trying to kill?\r\n", ch);
		return;
	}
	for (tmp_ch = ch->in_room->people; tmp_ch &&
		(FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room);
	
	if (!tmp_ch)
	{
		act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}
	
	if ( !success(ch, vict, SKILL_RESCUE, 0) )
	{
		send_to_char("You fail the rescue!\r\n", ch);
		return;
	}
	send_to_char("Banzai!  To the rescue...\r\n", ch);
	act("You are rescued by $N, you are confused!", FALSE, vict, 0, ch, TO_CHAR);
	act("$n heroically rescues $N!", FALSE, ch, 0, vict, TO_NOTVICT);
	
	if (FIGHTING(vict) == tmp_ch)	stop_fighting(vict);
	if (FIGHTING(tmp_ch))		stop_fighting(tmp_ch);
	if (FIGHTING(ch))		stop_fighting(ch);
	
	set_fighting(ch, tmp_ch);
	set_fighting(tmp_ch, ch);
	
	WAIT_STATE(vict, 2 * PULSE_VIOLENCE);
}


ACMD(do_kick)
{
	CHAR_DATA *vict;
	
	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_KICK))
	{
		send_to_char("You have no idea how.\r\n", ch);
		return;
	}
	one_argument(argument, arg);
	
	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
	{
		if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
			vict = FIGHTING(ch);
		else
		{
			send_to_char("Kick who?\r\n", ch);
			return;
		}
	}
	if (vict == ch)
	{
		send_to_char("Aren't we funny today...\r\n", ch);
		return;
	}

	if ( !success(ch, vict, SKILL_KICK, 0) )
		damage(ch, vict, 0, SKILL_KICK);
	else
		damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_KICK);
	
	WAIT_STATE(ch, PULSE_VIOLENCE * 3);

	/* Room Trigger Events */
	if ( ch->in_room->trigger )
		check_room_trigger(ch, TRIG_ACT_FIGHT_START);
}


/* ======================= R A N G E D   W E A P O N S ======================= */

/* find if ch wear a missile container of the given type */
OBJ_DATA *get_missile_cont( CHAR_DATA *ch, int type )
{
	OBJ_DATA *obj;

	if ( type == WEAPON_BOW )
		obj = GET_EQ(ch, WEAR_SHOULDERS);
	else
		obj = GET_EQ(ch, WEAR_WAIST);

	if ( !obj || GET_OBJ_TYPE(obj) != ITEM_MISSILE_CONT || GET_OBJ_VAL(obj, 2) != type )
		return (NULL);

	return (obj);
}

/* return a missile of the given type */
OBJ_DATA *get_missile( CHAR_DATA *ch, int type )
{
	OBJ_DATA *cont, *missile;
	char baf[MAX_STRING_LENGTH];

	if ( !ch || type < 1 || type > MAX_RANGED_WEAPON )
		return (NULL);

	cont = get_missile_cont( ch, type );

	if ( !cont )
	{
		sprintf( baf, "You are not wearing %s %s container.\r\n",
			AN(missile_cont_table[type]), missile_cont_table[type] );
		send_to_char(baf, ch );
		return (NULL);
	}

	if ( !cont->first_content )
	{
		sprintf( baf, "$p is out of %s.", missile_cont_table[type] );
		act( baf, FALSE, ch, cont, NULL, TO_CHAR );
		return (NULL);
	}

	missile = cont->first_content;
	separate_obj(missile);
	obj_from_obj(missile);

	return (missile);
}

/* Ranged Weapon */
ACMD(do_shoot)
{ 
	OBJ_DATA *missile, *wielded;
	EXIT_DATA *pexit;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	int range, type, skill;
	
	two_arguments(argument, arg1, arg2);
	
	if (!*arg1 || !*arg2)
	{
		send_to_char("You should try: shoot <someone> <direction>\r\n", ch);
		return;
	}
	
	if ( !(wielded = GET_EQ(ch, WEAR_WIELD) ) )
	{ 
		send_to_char("You aren't wielding anything!\r\n", ch);
		return;
	}
	
	if ( GET_OBJ_TYPE(wielded) != ITEM_FIREWEAPON )
	{ 
		send_to_char("You aren't wielding a shooting weapon!\r\n", ch);
		return;
	}

	type = GET_OBJ_VAL(wielded, 0);

	switch (type)
	{
	default:				skill = -1;						break;
	case WEAPON_BOW:		skill = SKILL_WEAPON_BOW;		break;
	case WEAPON_SLING:		skill = SKILL_WEAPON_SLING;		break;
	case WEAPON_CROSSBOW:	skill = SKILL_WEAPON_CROSSBOW;	break;
	}

	if ( skill == -1 )
	{
		send_to_char( "Unknown shooting weapon -- contact an Immortal.\r\n", ch );
		return;
	}

	if ( !GET_SKILL(ch, skill) )
	{
		send_to_char("You don't know how.\r\n", ch );
		return;
	}

	if ( RIDING(ch) )
	{
		if ( GET_SKILL(ch, skill) < 90 )
		{
			send_to_char("You are not skilled enough to shoot while mounted.\r\n", ch);
			return;
		}
	}

	/* get the right missile */
	if ( ! (missile = get_missile( ch, type ) ) )
		return;

	/* find the first door */
	if ( ( pexit = find_door( ch, NULL, arg2, "shoot" ) ) == NULL )
		return;

	/* check for CLOSED, FALSE or HIDDEN exits */
	if ( !valid_exit( pexit ) )
	{
		if ( EXIT_FLAGGED(pexit, EX_CLOSED) )
			send_to_char("Something blocks the way!\r\n", ch);
		else
			send_to_char("That's not a valid direction.\r\n", ch );
		return;
	}
	
	range = URANGE(1, GET_OBJ_VAL(wielded, 1), 3);
	obj_to_obj(missile, wielded);

	/* shoot the thing */
	fire_missile(ch, arg1, missile, WEAR_HOLD, range, pexit);
}
