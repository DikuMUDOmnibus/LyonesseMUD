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
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
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

typedef SPECIAL(*proctype);

/*   external vars  */
extern TIME_INFO_DATA	time_info;
extern SPELL_INFO_DATA	spell_info[];
extern int		guild_info[][3];
extern int		spell_sort_info[MAX_SKILLS + 1];
extern int		prac_params[4][NUM_CLASSES];

/* extern functions */
ACMD(do_drop);
ACMD(do_gen_door);
ACMD(do_say);
ACMD(do_action);
const char *how_good(int percent);
int	AddNewSpell(CHAR_DATA *ch, int spellnum);
void	add_follower(CHAR_DATA *ch, CHAR_DATA *leader);
void	CharLoseAllItems(CHAR_DATA *ch);

/* external specials */
SPECIAL(shop_keeper);		// shop.c
SPECIAL(postmaster);		// mail.c
SPECIAL(gen_board);		// boards.c

/* local functions */
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(snake);
SPECIAL(warrior_class);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(resurrectionist);
SPECIAL(bank);
SPECIAL(id_a_tron);
SPECIAL(stables);
void list_skills(CHAR_DATA *ch, bool known);
void npc_steal(CHAR_DATA *ch, CHAR_DATA *victim);

typedef struct	spec_list		SPEC_LIST;

struct spec_list
{
  char		*name;
  SPECIAL(*func);
};

SPEC_LIST spec_names[] =
{
  {"postmaster",		postmaster},
  {"cityguard",			cityguard},
  {"guild_guard",		guild_guard},
  {"guild",				guild},
  {"puff",				puff},
  {"fido",				fido},
  {"janitor",			janitor},
  {"snake",				snake},
  {"thief",				thief},
  {"magic_user",		magic_user},
  {"bank",				bank},
  {"gen_board",			gen_board},
  {"dump",				dump},
  {"pet_shops",			pet_shops},
  {"shop_keeper",		shop_keeper},
  {"id_a_tron",			id_a_tron},
  {"stables",			stables},
  {"warrior_class",		warrior_class},
  {"resurrectionist",	resurrectionist},
  {"<error>",			NULL}
};


/* Get the name of a special proc. */
char *get_spec_name(SPECIAL(func))
{
	int i = 0;
	
	if (!func)
		return ("None");
	
	for (; spec_names[i].func && spec_names[i].func != func; i++);
	
	return (spec_names[i].name);
}

/* Get a pointer to the function when you have a name */
proctype get_spec_proc(char *name)
{
	int i = 0;
	
	for (; spec_names[i].func && str_cmp(name, spec_names[i].name); i++);
	
	return (spec_names[i].func);
}

/* Show all specprocs */
void list_spec_procs(CHAR_DATA *ch)
{
	int i = 0;
	
	for (; spec_names[i].func; i++)
	{
		ch_printf(ch, "%-20s", spec_names[i].name);
		if (i % 3 == 2)
			send_to_char("\r\n", ch);
		else
			send_to_char("\t", ch);
	}
	send_to_char("\r\n", ch);
}

/* ================================================================== */
ACMD(do_test)
{
	one_argument(argument, arg);

	if ( get_spec_proc(arg) )
		send_to_char(OK, ch);
	else
		ch_printf(ch, "%s: non-existant specproc.\r\n", arg);
}


/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

#define LEARNED_LEVEL			0				/* % known which is considered "learned" */
#define MAX_PER_PRAC			1				/* max percent gain in skill per practice */
#define MIN_PER_PRAC			2				/* min percent gain in skill per practice */
#define PRAC_TYPE				3				/* should it say 'spell' or 'skill'?	 */

#define LEARNED(ch)				(prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch)				(prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch)				(prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])
#define SPLSKL(ch)				(prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])

void list_skills_vict(CHAR_DATA *ch, CHAR_DATA *viewer)
{
	char buf[MAX_STRING_LENGTH];
	int i, sortpos;
	
	sprintf(buf, "%s knows of the following %ss:\r\n", PERS(ch, viewer), SPLSKL(ch));
	
	for (sortpos = 1; sortpos <= MAX_SKILLS; sortpos++)
	{
		i = spell_sort_info[sortpos];

		if (strlen(buf) >= MAX_STRING_LENGTH - 32)
		{
			strcat(buf, "**OVERFLOW**\r\n");
			break;
		}

		if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)])
			sprintf(buf+strlen(buf), "%-20s [%d] %s\r\n",
				skill_name(i), GET_SKILL(ch, i), how_good(GET_SKILL(ch, i)));
	}
	
	page_string(viewer->desc, buf, 1);
}

void list_skills(CHAR_DATA *ch, bool known)
{
	char buf[MAX_STRING_LENGTH];
	int i, sortpos;
	
	ch_printf(ch, "You know of the following %ss:\r\n", SPLSKL(ch));
	
	*buf = '\0';

	for (sortpos = 1; sortpos <= MAX_SKILLS; sortpos++)
	{
		i = spell_sort_info[sortpos];

		if (strlen(buf) >= MAX_STRING_LENGTH - 32)
		{
			strcat(buf, "**OVERFLOW**\r\n");
			break;
		}

		if (known && !GET_SKILL(ch, i))
			continue;

		if (GET_LEVEL(ch) >= spell_info[i].min_level[(int) GET_CLASS(ch)])
			sprintf(buf+strlen(buf), "%-20s %s\r\n", skill_name(i), how_good(GET_SKILL(ch, i)));
	}
	
	if ( !*buf )
		strcpy(buf, "None!\r\n");

	page_string(ch->desc, buf, 1);
}

SPECIAL(guild)
{
	int skill_num, percent;
	
	if (IS_NPC(ch) || !CMD_IS("practice"))
		return (0);
	
	skip_spaces(&argument);
	
	if (!*argument)
	{
		if (!GET_PRACTICES(ch))
			send_to_char("You have no practice sessions remaining.\r\n", ch);
		else
			ch_printf(ch, "You have %d practice session%s remaining.\r\n",
				GET_PRACTICES(ch), (GET_PRACTICES(ch) == 1 ? "" : "s"));
		list_skills(ch, FALSE);
		return (1);
	}

	if (GET_PRACTICES(ch) <= 0)
	{
		send_to_char("You do not seem to be able to practice now.\r\n", ch);
		return (1);
	}
	
	skill_num = find_skill_num(argument);
	
	if (skill_num < 1 ||
		GET_LEVEL(ch) < spell_info[skill_num].min_level[(int) GET_CLASS(ch)])
	{
		sprintf(buf, "You do not know of that %s.\r\n", SPLSKL(ch));
		send_to_char(buf, ch);
		return (1);
	}

	if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
	{
		send_to_char("You are already learned in that area.\r\n", ch);
		return (1);
	}

	if ( MEMMING_CLASS(ch) )
	{
		if (AddNewSpell(ch, skill_num))
			return (1);
	}

	send_to_char("You practice for a while...\r\n", ch);
	GET_PRACTICES(ch)--;
	
	percent = GET_SKILL(ch, skill_num);
	percent += MIN(MAXGAIN(ch), MAX(MINGAIN(ch), int_app[GET_INT(ch)].learn));
	
	SET_SKILL(ch, skill_num, MIN(LEARNED(ch), percent));

	if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
		send_to_char("You are now learned in that area.\r\n", ch);
	
	return (1);
}



SPECIAL(dump)
{
	OBJ_DATA *k;
	int value = 0;
	
	for (k = ch->in_room->first_content; k; k = ch->in_room->first_content)
	{
		act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
		extract_obj(k);
	}
	
	if (!CMD_IS("drop"))
		return (0);
	
	do_drop(ch, argument, cmd, 0);
	
	for (k = ch->in_room->first_content; k; k = ch->in_room->first_content)
	{
		act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
		value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
		extract_obj(k);
	}
	
	if (value)
	{
		send_to_char("You are awarded for outstanding performance.\r\n", ch);
		act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);
		
		if (GET_LEVEL(ch) < 3)
			gain_exp(ch, value);
		else
			add_gold(ch, value);
	}
	return (1);
}

/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */

/*******************************
*         Mob Skills           *
*******************************/

void do_mob_bash(CHAR_DATA *ch, CHAR_DATA *vict)
{
	int hit_roll = 0, to_hit = 0;
	
	hit_roll = number (1,100) + GET_STR(ch);
	to_hit = (100 - (int) (100*GET_LEVEL(ch)/250));
	if (GET_LEVEL(vict) >= LVL_IMMORT)  
		hit_roll = 0;
	
	if (hit_roll < to_hit)
	{
		GET_POS(ch) = POS_SITTING;
		damage(ch, vict, 0, SKILL_BASH);
	}
	else
	{
		GET_POS(vict) = POS_SITTING;
		damage(ch, vict, GET_LEVEL(ch), SKILL_BASH);
		WAIT_STATE(vict, PULSE_VIOLENCE * 2);
		WAIT_STATE(ch, PULSE_VIOLENCE * 3);
	}
}

void do_mob_disarm(CHAR_DATA *ch, CHAR_DATA *vict)
{
	OBJ_DATA *weap; 
	int hit_roll = 0, to_hit = 0;
	
	hit_roll = number (1, 100) + GET_DEX(ch);
	to_hit = (100 - (int) (100 * GET_LEVEL(ch) / 250));
	if (GET_LEVEL(vict) >= LVL_IMMORT)  
		hit_roll = 0;
	
	if (!(weap = GET_EQ(FIGHTING(ch), WEAR_WIELD)))
	{
		send_to_char("Nope, sorry\r\n", ch);
		return;      
	}
	
	if (hit_roll < to_hit)
	{
		if (GET_EQ(vict, WEAR_WIELD))
		{
			act("&3You disarm $p from $N's hand!&0", FALSE, ch, weap, FIGHTING(ch), TO_CHAR);
			act("&3$n disarms $p from your hand!&0", FALSE, ch, weap, FIGHTING(ch), TO_VICT); 
			act("&3$n disarms $p from $N's hand!&0", FALSE, ch, weap, FIGHTING(ch), TO_NOTVICT);
			obj_to_room(unequip_char(FIGHTING(ch), WEAR_WIELD), vict->in_room);
		}
		else
		{
			act("&3You fail to disarm $N&0", FALSE, ch, weap, FIGHTING(ch), TO_CHAR);
			act("&3$n fails to disarm you&0", FALSE, ch, weap, FIGHTING(ch), TO_VICT);
			act("&3$n fails to disarm $N&0", FALSE, ch, weap, FIGHTING(ch), TO_NOTVICT);
		}
	}
	WAIT_STATE(ch, PULSE_VIOLENCE);
}

void do_mob_kick(CHAR_DATA *ch, CHAR_DATA *vict)
{
	int hit_roll = 0, to_hit = 0;
	
	hit_roll = number (1,100) + GET_STR(ch);
	to_hit = (100 - (int) (100*GET_LEVEL(ch)/250));
	if (GET_LEVEL(vict) >= LVL_IMMORT)  
		hit_roll = 0;
	
	if (hit_roll < to_hit)
	{
		damage(ch, vict, 0, SKILL_KICK);
	}
	else
	{
		damage(ch, vict, GET_LEVEL(ch) / 2, SKILL_KICK);
		WAIT_STATE(vict, PULSE_VIOLENCE * 2);
		WAIT_STATE(ch, PULSE_VIOLENCE * 3);
	}
}

void do_mob_backstab(CHAR_DATA *ch, CHAR_DATA *vict)
{
	int hit_roll = 0, to_hit = 0;
	
	hit_roll = number (1,100) + GET_STR(ch);
	to_hit = (100 - (int) (100*GET_LEVEL(ch)/250));
	if (GET_LEVEL(vict) >= LVL_IMMORT)  
		hit_roll = 0;
	if (FIGHTING(ch))
	{
		act("$N just tried to backstab you during combat!", FALSE, vict, 0, ch, TO_CHAR);
		act("$e notices you. You cannot backstab in combat!", FALSE, vict, 0, ch, TO_VICT);
		act("$N attempts to backstab $n during combat!", FALSE, vict, 0, ch, TO_NOTVICT);
	}
	if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict))
	{
		act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
		act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
		act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
		hit(vict, ch, TYPE_UNDEFINED);
		return;
	}
	
	if (hit_roll < to_hit)
	{
		damage(ch, vict, 0, SKILL_BACKSTAB);
	}
	else
	{
		hit(ch, vict, SKILL_BACKSTAB);
		WAIT_STATE(vict, PULSE_VIOLENCE * 2);
		WAIT_STATE(ch, PULSE_VIOLENCE * 3);
	}
}

void npc_steal(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int gold;
	
	if (IS_NPC(victim))
		return;
	if (GET_LEVEL(victim) >= LVL_IMMORT)
		return;
	if (!CAN_SEE(ch, victim))
		return;
	
	if (AWAKE(victim) && (number(0, GET_LEVEL(ch)) == 0))
	{
		act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
		act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
	}
	else
	{
		/* Steal some gold coins */
		gold = (get_gold(victim) * number(1, 10)) / 100;
		if (gold > 0)
		{
			add_gold(ch, gold);
			sub_gold(victim, gold);
		}
	}
}


SPECIAL(warrior_class)
{
	CHAR_DATA *vict;
	int att_type = 0, hit_roll = 0, to_hit = 0 ;
	
	if (cmd || !AWAKE(ch))
		return (FALSE);
	
	/* if two people are fighting in a room */
	if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room))
	{
		vict = FIGHTING(ch);
		
		if (number(1, 5) == 5)
		{
			act("$n foams at the mouth and growls in anger.", 1, ch, 0, 0, TO_ROOM);
		}
		
		if (GET_POS(ch) == POS_FIGHTING)
		{
			att_type = number(1,40);          
			
			hit_roll = number(1,100) + GET_STR(ch);
			to_hit = (100 - (int) (100 * GET_LEVEL(ch) / 250));

			if (GET_LEVEL(vict) >= LVL_IMMORT)  
				hit_roll = 0;
			
			switch (att_type)
			{
			case 1: case 2: case 3: case 4:
				do_mob_kick(ch, vict);
				break;
			case 5: case 6: case 7:
				do_mob_bash(ch, vict);
				break;
			case 8: case 9: case 10:
				//do_mob_headbutt(ch, vict);
				break;
			case 11: case 12: case 13:
				//do_mob_bearhug(ch, vict);
				break;
			case 14:
				do_mob_disarm(ch, vict);
				break; 
			case 15:
			case 16:
			case 17:
			case 18:
			case 19:
			case 20:
				break;
			default:
				break;
			}
			
		}
	}

	return (FALSE);
}


SPECIAL(thief)
{
	CHAR_DATA *cons;
	
	if (cmd)
		return (FALSE);
	
	if (GET_POS(ch) != POS_STANDING)
		return (FALSE);
	
	for (cons = ch->in_room->people; cons; cons = cons->next_in_room)
		if (!IS_NPC(cons) && (GET_LEVEL(cons) < LVL_IMMORT) && (!number(0, 4)))
		{
			npc_steal(ch, cons);
			return (TRUE);
		}

	return (FALSE);
}


SPECIAL(magic_user)
{
	CHAR_DATA *vict;
	
	if (cmd || GET_POS(ch) != POS_FIGHTING)
		return (FALSE);
	
	/* pseudo-randomly choose someone in the room who is fighting me */
	for (vict = ch->in_room->people; vict; vict = vict->next_in_room)
	{
		if (FIGHTING(vict) == ch && !number(0, 4))
			break;
	}
	
	/* if I didn't pick any of those, then just slam the guy I'm fighting */
	if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
		vict = FIGHTING(ch);
	
	/* Hm...didn't pick anyone...I'll wait a round. */
	if (vict == NULL)
		return (TRUE);
	
	if ((GET_LEVEL(ch) > 13) && (number(0, 10) == 0))
		cast_spell(ch, NULL, vict, NULL, SPELL_POISON);
	
	if ((GET_LEVEL(ch) > 7) && (number(0, 8) == 0))
		cast_spell(ch, NULL, vict, NULL, SPELL_BLINDNESS);
	
	if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0))
	{
		if (IS_EVIL(ch))
			cast_spell(ch, NULL, vict, NULL, SPELL_ENERGY_DRAIN);
		else if (IS_GOOD(ch))
			cast_spell(ch, NULL, vict, NULL, SPELL_DISPEL_EVIL);
	}
	
	if (number(0, 4))
		return (TRUE);
	
	switch (GET_LEVEL(ch))
	{
	case 4:
	case 5:
		cast_spell(ch, NULL, vict, NULL, SPELL_MAGIC_MISSILE);
		break;
	case 6:
	case 7:
		cast_spell(ch, NULL, vict, NULL, SPELL_CHILL_TOUCH);
		break;
	case 8:
	case 9:
		cast_spell(ch, NULL, vict, NULL, SPELL_BURNING_HANDS);
		break;
	case 10:
	case 11:
		cast_spell(ch, NULL, vict, NULL, SPELL_SHOCKING_GRASP);
		break;
	case 12:
	case 13:
		cast_spell(ch, NULL, vict, NULL, SPELL_LIGHTNING_BOLT);
		break;
	case 14:
	case 15:
	case 16:
	case 17:
		cast_spell(ch, NULL, vict, NULL, SPELL_COLOR_SPRAY);
		break;
	default:
		cast_spell(ch, NULL, vict, NULL, SPELL_FIREBALL);
		break;
	}

	return (TRUE);
}


SPECIAL(snake)
{
	if (cmd || GET_POS(ch) != POS_FIGHTING || !FIGHTING(ch))
		return (FALSE);
	
	if (IN_ROOM(FIGHTING(ch)) != IN_ROOM(ch) || number(0, GET_LEVEL(ch)) != 0)
		return (FALSE);
	
	act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
	act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
	call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
	return (TRUE);
}

/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

SPECIAL(guild_guard)
{
	CHAR_DATA *guard = (CHAR_DATA *) me;
	const char *buf = "The guard humiliates you, and blocks your way.\r\n";
	const char *buf2 = "The guard humiliates $n, and blocks $s way.";
	int i;
	bool can_go = FALSE;
	
	if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND) || AFF_FLAGGED(guard, AFF_PARALYSIS))
		return (FALSE);
	
	if (GET_LEVEL(ch) >= LVL_IMMORT)
		return (FALSE);
	
	for (i = 0; guild_info[i][0] != -1; i++)
	{
		if (IS_NPC(ch))
			continue;

		if (ch->in_room->number == guild_info[i][1] &&
		    (cmd != guild_info[i][2] || GET_CLASS(ch) == guild_info[i][0]))
		{
			can_go = TRUE;
			break;
		}
	}

	if (!can_go)
	{
		send_to_char(buf, ch);
		act(buf2, FALSE, ch, 0, 0, TO_ROOM);
		return (TRUE);
	}
	
	return (FALSE);
}



SPECIAL(puff)
{
	char actbuf[MAX_INPUT_LENGTH];
	
	if (cmd)
		return (0);
	
	switch (number(0, 60))
	{
	case 0:
		do_say(ch, strcpy(actbuf, "My god!  It's full of stars!"), 0, 0);
		return (1);
	case 1:
		do_say(ch, strcpy(actbuf, "How'd all those fish get up here?"), 0, 0);
		return (1);
	case 2:
		do_say(ch, strcpy(actbuf, "I'm a very female dragon."), 0, 0);
		return (1);
	case 3:
		do_say(ch, strcpy(actbuf, "I've got a peaceful, easy feeling."), 0, 0);
		return (1);
	default:
		return (0);
	}
}



SPECIAL(fido)
{
	OBJ_DATA *i, *temp, *next_obj;
	
	if (cmd || !AWAKE(ch))
		return (FALSE);
	
	for (i = ch->in_room->first_content; i; i = i->next_content)
	{
		if (IS_CORPSE(i))
		{
			act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
			for (temp = i->first_content; temp; temp = next_obj)
			{
				next_obj = temp->next_content;
				obj_from_obj(temp);
				obj_to_room(temp, ch->in_room);
			}
			extract_obj(i);
			return (TRUE);
		}
	}
	return (FALSE);
}



SPECIAL(janitor)
{
	OBJ_DATA *i;
	
	if (cmd || !AWAKE(ch))
		return (FALSE);
	
	for (i = ch->in_room->first_content; i; i = i->next_content)
	{
		if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
			continue;
		if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
			continue;
		act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
		obj_from_room(i);
		i = obj_to_char(i, ch);
		return (TRUE);
	}
	
	return (FALSE);
}


SPECIAL(cityguard)
{
	CHAR_DATA *tch, *evil, *spittle;
	int max_evil, min_cha;
	
	if (cmd || !AWAKE(ch) || FIGHTING(ch))
		return (FALSE);
	
	max_evil = 1000;
	min_cha = 6;
	spittle = evil = NULL;
	
	for (tch = ch->in_room->people; tch; tch = tch->next_in_room)
	{
		if (!CAN_SEE(ch, tch))
			continue;
	
		if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_KILLER))
		{
			act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
			hit(ch, tch, TYPE_UNDEFINED);
			return (TRUE);
		}

		if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_THIEF))
		{
			act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
			hit(ch, tch, TYPE_UNDEFINED);
			return (TRUE);
		}

		if (FIGHTING(tch) && GET_ALIGNMENT(tch) < max_evil && (IS_NPC(tch) || IS_NPC(FIGHTING(tch))))
		{
			max_evil = GET_ALIGNMENT(tch);
			evil = tch;
		}
		
		if (GET_CHA(tch) < min_cha)
		{
			spittle = tch;
			min_cha = GET_CHA(tch);
		}
	}
	
	if (evil && GET_ALIGNMENT(FIGHTING(evil)) >= 0)
	{
		act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
		hit(ch, evil, TYPE_UNDEFINED);
		return (TRUE);
	}

	/* Reward the socially inept. */
	if (spittle && !number(0, 1))
	{
		static int spit_social;
		
		if (!spit_social)
			spit_social = find_command("spit");
		
		if (spit_social > 0)
		{
			char spitbuf[MAX_NAME_LENGTH + 1];
			
			strncpy(spitbuf, GET_NAME(spittle), sizeof(spitbuf));
			spitbuf[sizeof(spitbuf) - 1] = '\0';
			
			do_action(ch, spitbuf, spit_social, 0);
			return (TRUE);
		}
	}

	return (FALSE);
}


#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops)
{
	char buf[MAX_STRING_LENGTH], pet_name[256];
	ROOM_DATA *pet_room;
	CHAR_DATA *pet;
	
	if ( !( pet_room = get_room(ch->in_room->number + 1) ) )
		return (TRUE);
	
	if (CMD_IS("list"))
	{
		send_to_char("Available pets are:\r\n", ch);
		for (pet = pet_room->people; pet; pet = pet->next_in_room)
		{
			if (!IS_NPC(pet))
				continue;
			sprintf(buf, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
			send_to_char(buf, ch);
		}
		return (TRUE);
	}
	else if (CMD_IS("buy"))
	{	
		two_arguments(argument, buf, pet_name);
		
		if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet))
		{
			send_to_char("There is no such pet!\r\n", ch);
			return (TRUE);
		}
		if (get_gold(ch) < PET_PRICE(pet))
		{
			send_to_char("You don't have enough gold!\r\n", ch);
			return (TRUE);
		}
		sub_gold(ch, PET_PRICE(pet));
		
		pet = read_mobile(GET_MOB_RNUM(pet), REAL);
		GET_EXP(pet) = 0;
		SET_BIT(AFF_FLAGS(pet), AFF_CHARM);
		
		if (*pet_name)
		{
			sprintf(buf, "%s %s", GET_PC_NAME(pet), pet_name);
			GET_PC_NAME(pet) = str_dup(buf);
			
			sprintf(buf, "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
				GET_DDESC(pet), pet_name);
			GET_DDESC(pet) = str_dup(buf);
		}
		char_to_room(pet, ch->in_room);
		add_follower(pet, ch);
		
		/* Be certain that pets can't get/carry/use/wield/wear items */
		IS_CARRYING_W(pet) = 1000;
		IS_CARRYING_N(pet) = 100;
		
		send_to_char("May you enjoy your pet.\r\n", ch);
		act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);
		
		return (1);
	}
	/* All commands except list and buy */
	return (0);
}


SPECIAL(resurrectionist)
{
	CHAR_DATA *rmob = (CHAR_DATA *) me;

	if (IS_NPC(ch))
		return (FALSE);

	if (CMD_IS("help") || CMD_IS("list"))
	{
		do_say(rmob,
			"You can be resurrected simply by typing: &b&7resurrect&0.", 0, 0);

		if (GET_SEX(rmob) == SEX_MALE)
		{
			do_say(rmob, 
				"In this case, you will lose all your equipment and will be\r\n"
				"transported to the temple room. If you don't want to lose\r\n"
				"your belongings, you will have to face the Labyrinth of the Dead\r\n"
				"and find your way out alone.", 0, 0);
		}
		return (TRUE);
	}

	if (CMD_IS("resurrect"))
	{
		if (!PLR_FLAGGED(ch, PLR_GHOST))
		{
			do_say(rmob, "Only ghosts can be resurrected.", 0, 0);
			return (TRUE);
		}

		if (GET_SEX(rmob) == SEX_MALE)
			CharLoseAllItems(ch);

		send_to_char("You have been resurrected.\r\n", ch);

		char_from_room(ch);
		char_to_room(ch, r_mortal_start_room);

		REMOVE_BIT(PLR_FLAGS(ch), PLR_GHOST);

		act("$n has been resurrected and sent away from the Labyrinth of the Dead.\r\n",
			FALSE, ch, NULL, NULL, TO_ROOM);
		return (TRUE);
	}

	return (FALSE);
}


/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */


SPECIAL(bank)
{
	int amount;
	
	if (CMD_IS("balance"))
	{
		if (GET_BANK_GOLD(ch) > 0)
			sprintf(buf, "Your current balance is %d coins.\r\n",
			GET_BANK_GOLD(ch));
		else
			sprintf(buf, "You currently have no money deposited.\r\n");
		send_to_char(buf, ch);
		return (1);
	}
	else if (CMD_IS("deposit"))
	{
		if ((amount = atoi(argument)) <= 0)
		{
			send_to_char("How much do you want to deposit?\r\n", ch);
			return (1);
		}
		if (get_gold(ch) < amount)
		{
			send_to_char("You don't have that many coins!\r\n", ch);
			return (1);
		}
		sub_gold(ch, amount);
		GET_BANK_GOLD(ch) += amount;
		sprintf(buf, "You deposit %d coins.\r\n", amount);
		send_to_char(buf, ch);
		act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
		return (1);
	}
	else if (CMD_IS("withdraw"))
	{
		if ((amount = atoi(argument)) <= 0)
		{
			send_to_char("How much do you want to withdraw?\r\n", ch);
			return (1);
		}
		if (GET_BANK_GOLD(ch) < amount)
		{
			send_to_char("You don't have that many coins deposited!\r\n", ch);
			return (1);
		}
		add_gold(ch, amount);
		GET_BANK_GOLD(ch) -= amount;
		sprintf(buf, "You withdraw %d coins.\r\n", amount);
		send_to_char(buf, ch);
		act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
		return (1);
	}
	else
		return (0);
}

SPECIAL(id_a_tron)
{
	OBJ_DATA *obj;
	int i, found;
	
	if (!CMD_IS("analyze"))
		return (0);
	
	skip_spaces(&argument);
	
	if (!*argument)
	{
		send_to_char("Analyze what?\r\n", ch);
		return (1);
	}
	
	if (!(obj = get_obj_in_list_vis_rev(ch, argument, NULL, ch->last_carrying)))
	{
		sprintf(buf, "You don't seem to have any %ss.\r\n", argument);
		send_to_char(buf, ch);
		return (1);
	}
	
	send_to_char("&b&2The machine chitters to itself for a minute, then whirs into life.&0\r\n", ch);
	if ( !OBJ_FLAGGED(obj, ITEM_DONATED) )
	{
		send_to_char("&b&6This is not a donated item!!\r\n&0", ch);
		return (1);
	}
	send_to_char("&b&6Your divination is complete...\r\n&0", ch);
	
	sprintf(buf, "&b&4Object:&0 &b&7%s&0\r\n", obj->short_description);
	sprintf(buf, "%s&b&4Item Type:&0 &b&7", buf);
	sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
	strcat(buf, buf2);
	strcat(buf, "&0    &b&4Effects:&0 &b&7");
	send_to_char(buf, ch);
	
	//  send_to_char("Item is: ", ch);
	sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
	strcat(buf, "&0\r\n");
	send_to_char(buf, ch);
	
	send_to_char("&b&4Restrictions:&0 &b&7", ch);
	sprintbit(GET_OBJ_ANTI(obj), anti_bits, buf1);
	strcat(buf1,"&0\r\n");
	send_to_char(buf1,ch);
	
	sprintf(buf, "&b&4Equipable Location(s):&0 &b&7");
	sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf2);
	strcat(buf, buf2);
	strcat(buf, "&0\r\n");
	send_to_char(buf, ch);
	
	sprintf(buf, "&b&4Weight: &0&b&7%d&0&b&4, Value: &0&b&7%d&b&4, Rent: &0&b&7%d&b&4, Min Level: &0&b&7%d&0\r\n",
		GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_LEVEL(obj));
	send_to_char(buf, ch);
	
	if (GET_OBJ_PERM(obj))
	{
		char baf1[256];
		
		*baf1 = '\0';

		send_to_char("&b&4Attributes:&0 &b&7", ch);
		if (GET_OBJ_PERM(obj))
			sprintbit(GET_OBJ_PERM(obj), affected_bits, baf1);
		sprintf(buf, "%s&0\r\n", baf1);
		send_to_char(buf, ch);
	}
	
	switch (GET_OBJ_TYPE(obj))
	{
	case ITEM_SCROLL:
	case ITEM_POTION:
		sprintf(buf, "&b&4Level &0&b&7%d&0&b&4 spells of&0&b&7", GET_OBJ_VAL(obj, 0));
		if (GET_OBJ_VAL(obj, 1) >= 1)
			sprintf(buf + strlen(buf), " %s", skill_name(GET_OBJ_VAL(obj, 1)));
		if (GET_OBJ_VAL(obj, 2) >= 1)
			sprintf(buf + strlen(buf), " &0&b&4/&0&b&7 %s", skill_name(GET_OBJ_VAL(obj, 2)));
		if (GET_OBJ_VAL(obj, 3) >= 1)
			sprintf(buf + strlen(buf), " &0&b&4/&0&b&7 %s", skill_name(GET_OBJ_VAL(obj, 3)));
		strcat(buf, "&0\r\n");
		send_to_char(buf, ch);
		break;
		
	case ITEM_WAND:
	case ITEM_STAFF:
		sprintf(buf, "&b&4Level &0&b&7%d&0&b&4 spell of&0&b&7 ", GET_OBJ_VAL(obj, 0));
		
		sprintf(buf + strlen(buf), " %s&0&b&4. ", skill_name(GET_OBJ_VAL(obj, 3)));
		sprintf(buf + strlen(buf), "Holds &0&b&7%d&0&b&4 charge%s and has &0&b&7%d&0&b&4 charge%s left.&0\r\n",
			GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
			GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 2) == 1 ? "" : "s");
		send_to_char(buf, ch);
		break;
		
	case ITEM_WEAPON:
		sprintf(buf, "&b&4Damage Dice of &0&b&7%d&0&b&4d&0&b&7%d&0&b&4",
			GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
		sprintf(buf + strlen(buf), " for an average per-round damage of &0&b&7%.1f&0\r\n",
			(((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
		send_to_char(buf, ch);
		break;
		
	case ITEM_ARMOR:
		sprintf(buf, "&b&4AC-apply of &0&b&7%d&0\r\n", GET_OBJ_VAL(obj, 0));
		send_to_char(buf, ch);
		break;
	}

	found = FALSE;
	for (i = 0; i < MAX_OBJ_AFF; i++)
	{
		if ((obj->affected[i].location != APPLY_NONE) &&
			(obj->affected[i].modifier != 0))
		{
			if (!found)
			{
				send_to_char("&b&4Affects:&0\r\n", ch);
				found = TRUE;
			}
			sprinttype(obj->affected[i].location, apply_types, buf2);
			sprintf(buf, "    &b&7%+d&0&b&4 to&0&b&7 %s&0\r\n", obj->affected[i].modifier, buf2);
			send_to_char(buf, ch);
		}
	}
	return (1);
}


/* ********************************************************************
 *  Special procedures for mounts and vehicles                        *
 ******************************************************************** */
void	dismount_char(CHAR_DATA *ch);
void	rent_mount(CHAR_DATA *ch, CHAR_DATA *mount);
void	unrent_from_stable(CHAR_DATA *ch, STABLE_RENT *strent);
void	perform_tell(CHAR_DATA *ch, CHAR_DATA *vict, char *arg);

VEHICLE_DATA *init_vehicle( VEHICLE_INDEX *vType );
bool	empty_vehicle(VEHICLE_DATA *vehicle, bool bQuick);
void	rent_vehicle(CHAR_DATA *ch, VEHICLE_DATA *vehicle);
void	extract_vehicle(VEHICLE_DATA *vehicle, int mode);

extern STABLE_RENT		*first_stable_rent;
extern STABLE_RENT		*last_stable_rent;

extern VEHICLE_INDEX	*vehicle_types[MAX_VEH_TYPE];

NAME_NUMBER sell_mounts_list[] =
{
	{"horse", 30},
	{"\n", -1}
};

SPECIAL(stables)
{
	CHAR_DATA *stman = (CHAR_DATA *) me;
	char sbuf[MAX_STRING_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
	
	argument = one_argument(argument, arg1);

	if (CMD_IS("help") || (CMD_IS("list") && !*arg1))
	{
		strcpy(sbuf, 
			"Here you can rent your mount or vehicle for 10 gold coins.\r\n"
			"To rent write 'rent <mount>' or 'rent <vehicle>'\r\n"
			"To have a list write 'withdraw' without any argument.\r\n"
			"To withdraw write 'withdraw <mount>' or 'withdraw <vehicle>'"
			);
		perform_tell(stman, ch, sbuf);
		strcpy(sbuf,
			"Of course, here you can buy a fast mount or a strong vehicle.\r\n"
			"write 'list mounts' or 'list vehicles' for a list of availabilities\r\n"
			"write 'buy mount <name>' or 'buy vehicle <name>' to buy what you want."
			);
		perform_tell(stman, ch, sbuf);
		return (1);
	}

	if ( CMD_IS("list") )
	{
		if (is_abbrev(arg1, "mounts"))
		{
			int num;

			strcpy(sbuf, "I have the following mounts for sale:");
			for (num = 0; *sell_mounts_list[num].name != '\n'; num++)
				sprintf(sbuf+strlen(sbuf), "\r\n  %s %s",
					AN(sell_mounts_list[num].name), sell_mounts_list[num].name);
		}
		else if (is_abbrev(arg1, "vehicles"))
		{
			int num;

			strcpy(sbuf, "I have the following vehicles for sale:");
			for (num = 0; num < MAX_VEH_TYPE; num++)
			{
				if (!vehicle_types[num])
					continue;
				sprintf(sbuf+strlen(sbuf), "\r\n  &b&7%10s&0 %s",
					vehicle_types[num]->name,
					vehicle_types[num]->short_description);
			}
		}
		else
			strcpy(sbuf, "I don't understand.");
		perform_tell(stman, ch, sbuf);
		return (1);
	}

	if ( CMD_IS("rent") )
	{
		if ( !*arg1 )
		{
			perform_tell(stman, ch, "Rent what?");
			return (1);
		}

		if ( get_gold(ch) < 10 )
		{
			perform_tell(stman, ch, "It cost 10 gold coins, that you don't have.");
			return (1);
		}

		sub_gold(ch, 10);

		// wanna rent a mount?
		if ( RIDING(ch) && isname(arg1, GET_PC_NAME(RIDING(ch))) )
		{
			CHAR_DATA *mount = RIDING(ch);

			if ( !AFF_FLAGGED(mount, AFF_TAMED) )
			{
				perform_tell(stman, ch, "You can rent only tamed mounts.");
				return (1);
			}

			dismount_char(ch);
			rent_mount(ch, mount);

			perform_tell(stman, ch, "Okay.");
			act("You pay 10 gold coins and the stableman carry away $N.", FALSE, ch, NULL, mount, TO_CHAR);
			act("The stableman carry away $n's $N.", FALSE, ch, NULL, mount, TO_ROOM);

			extract_char_final(mount);
		}
		// wanna rent a vehicle?
		else if ( WAGONER(ch) && isname(arg1, WAGONER(ch)->name) )
		{
			VEHICLE_DATA *vehicle = WAGONER(ch);

			stop_be_wagoner(ch);

			/* here we check only for wagoner and ppl inside */
			if ( !empty_vehicle(vehicle, TRUE) )
			{
				send_to_char("You can rent only emptied vehicles.\r\n", ch);
				return (1);
			}

			rent_vehicle(ch, vehicle);

			act("You pay 10 gold coins and the stableman carry away $v.", FALSE, ch, NULL, vehicle, TO_CHAR);
			act("The stableman carry away $n's $V.", FALSE, ch, NULL, vehicle, TO_ROOM);

			extract_vehicle(vehicle, 1);
		}
		else
		{
			sprintf(sbuf, "What's exactely this '%s' that you want to rent?", arg1);
			perform_tell(stman, ch, sbuf);
		}

		return (1);
	}

	if ( CMD_IS("withdraw") )
	{
		STABLE_RENT *strent;

		if ( !first_stable_rent )
		{
			perform_tell(stman, ch, "You cannot withdraw your belongings at the moment.");
			return (1);
		}

		if ( !*arg1 )
		{
			int r_num = 0;

			strcpy(sbuf, "Here, you have rented the followings:\r\n");
			for ( strent = last_stable_rent; strent; strent = strent->prev )
			{
				// skip other stables..
				if ( strent->stable_vnum != ch->in_room->number )
					continue;

				if ( strent->id_player == GET_IDNUM(ch) )
					sprintf(sbuf + strlen(sbuf), "[%d] %s (%s)\r\n",
						++r_num, strent->typedescr, st_rent_descr[strent->type]);
			}

			if ( !r_num )
				strcat(sbuf, "Nothing.");
			perform_tell(stman, ch, sbuf);
		}
		else
		{
			int fnum;
			char *vname = str_dup(arg1);

			if ( !( fnum = get_number(&vname) ) )
				return (1);

			for ( strent = first_stable_rent; strent; strent = strent->next )
			{
				// skip other stables..
				if ( strent->stable_vnum != ch->in_room->number )
					continue;
				
				if ( strent->id_player == GET_IDNUM(ch) )
				{
					if ( !str_cmp(vname, strent->typedescr) )
						if ( --fnum == 0 )
							break;
				}
			}

			if ( !strent )
			{
				sprintf(sbuf, "Here you don't have %s %s. Try another stable.",	AN(vname), vname);
				perform_tell(stman, ch, sbuf);
				return (1);
			}

			unrent_from_stable(ch, strent);
		}
		return (1);
	}

	if (CMD_IS("buy"))
	{
		char arg2[MAX_INPUT_LENGTH];
		int num, price;

		argument = one_argument(argument, arg2);

		if (!*arg2)
		{
			strcpy(sbuf, "What do you want to buy?");
			perform_tell(stman, ch, sbuf);
			return (1);
		}

		if (is_abbrev(arg1, "mount"))
		{
			CHAR_DATA *mount;
			int vnum = 0;

			for (num = 0; *sell_mounts_list[num].name != '\n'; num++)
			{
				if (is_abbrev(arg2, sell_mounts_list[num].name))
				{
					vnum = sell_mounts_list[num].number;
					break;
				}
			}

			if (!vnum)
			{
				strcpy(sbuf, "I have the following mounts for sale:");
				for (num = 0; *sell_mounts_list[num].name != '\n'; num++)
					sprintf(sbuf+strlen(sbuf), "\r\n  %s", sell_mounts_list[num].name);
				perform_tell(stman, ch, sbuf);
				return (1);
			}

			if (!(mount = read_mobile(vnum, VIRTUAL)))
			{
				strcpy(sbuf, "Right now you can't have it.");
				perform_tell(stman, ch, sbuf);
				return (1);
			}

			// il prezzo dei cavalli e' : mob->esperienza * 3
			price = GET_EXP(mount) * 3;
			
			if ( get_gold(ch) < price )
			{
				sprintf(sbuf, "It cost %d gold coins, that you don't have.", price);
				perform_tell(stman, ch, sbuf);
				return (1);
			}

			sub_gold(ch, price);

			mount->mob_specials.owner_id = GET_IDNUM(ch);
			char_to_room(mount, ch->in_room);

			sprintf(sbuf, "You pay %d gold coins and $N gives you %s.",
				price, mount->player.short_descr);
			act(sbuf, FALSE, ch, NULL, stman, TO_CHAR);

			sprintf(sbuf, "$n pays %d gold coins and $N gives $m %s.",
				price, mount->player.short_descr);
			act(sbuf, FALSE, ch, NULL, stman, TO_ROOM);
		}
		else if (is_abbrev(arg1, "vehicle"))
		{
			VEHICLE_DATA *pVeh;

			for (num = 0; num < MAX_VEH_TYPE; num++)
			{
				if (!vehicle_types[num])
					continue;

				if (is_abbrev(arg2, vehicle_types[num]->name))
					break;
			}

			if (num >= MAX_VEH_TYPE)
			{
				strcpy(sbuf, "I have the following vehicles for sale:");
				for (num = 0; num < MAX_VEH_TYPE; num++)
				{
					if (!vehicle_types[num])
						continue;
					sprintf(sbuf+strlen(sbuf), "\r\n  &b&7%10s&0 %s",
						vehicle_types[num]->name,
						vehicle_types[num]->short_description);
				}
				perform_tell(stman, ch, sbuf);
				return (1);
			}

			// il prezzo dei veicoli e' : veicolo->capacita * 25
			price = vehicle_types[num]->value.capacity * 25;

			if ( get_gold(ch) < price )
			{
				sprintf(sbuf, "It cost %d gold coins, that you don't have.", price);
				perform_tell(stman, ch, sbuf);
				return (1);
			}

			sub_gold(ch, price);

			pVeh = init_vehicle(vehicle_types[num]);
			pVeh->owner_id = GET_IDNUM(ch);

			vehicle_to_room(pVeh, ch->in_room);

			sprintf(sbuf, "You pay %d gold coins and $N gives you %s.", price, pVeh->short_description);
			act(sbuf, FALSE, ch, NULL, stman, TO_CHAR);
			sprintf(sbuf, "$n pays %d gold coins and $N gives $m %s.", price, pVeh->short_description);
			act(sbuf, FALSE, ch, NULL, stman, TO_ROOM);
		}
		else
			perform_tell(stman, ch, "I don't understand.");

		return (1);
	}

	return (0);
}
