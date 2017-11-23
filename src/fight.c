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
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
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
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "clan.h"

/* Structures */
CHAR_DATA	*combat_list		= NULL;	/* head of l-list of fighting chars */
CHAR_DATA	*next_combat_list	= NULL;

/* External structures */
extern MESSAGE_LIST			fight_messages[MAX_MESSAGES];
extern CLAN_POLITIC_DATA	politics_data;

extern int pk_allowed;		/* see config.c */
extern int max_exp_gain;	/* see config.c */
extern int max_exp_loss;	/* see config.c */
extern int max_npc_corpse_time, max_pc_corpse_time;

/* External procedures */
ACMD(do_flee);
ACMD(do_say);
OBJ_DATA *get_missile_cont( CHAR_DATA *ch, int type );
char	*fread_action(FILE *fl, int nr);
int		backstab_mult(int level);
int		thaco(int ch_class, int level);
int		ok_damage_shopkeeper(CHAR_DATA *ch, CHAR_DATA *victim);
int		wild_track(CHAR_DATA *ch, CHAR_DATA *vict, bool silent);
int		find_first_step(ROOM_DATA *src, ROOM_DATA *target);
int		kills_limit_xpgain(CHAR_DATA *ch, CHAR_DATA *victim, int exp);
void	start_hunting(CHAR_DATA *ch, CHAR_DATA *vict);
void	diag_char_to_char(CHAR_DATA * i, CHAR_DATA * ch);
void	stop_memorizing(CHAR_DATA *ch);
void	weapon_spells(CHAR_DATA *ch, CHAR_DATA *vict, OBJ_DATA *wpn);
void    brag(CHAR_DATA *ch, CHAR_DATA *victim);

/* local functions */
void perform_group_gain(CHAR_DATA *ch, int base, CHAR_DATA *victim);
void dam_message(int dam, CHAR_DATA *ch, CHAR_DATA *victim, int w_type);
void appear(CHAR_DATA *ch);
void load_messages(void);
void check_killer(CHAR_DATA *ch, CHAR_DATA *vict);
void make_corpse(CHAR_DATA *ch);
void change_alignment(CHAR_DATA *ch, CHAR_DATA *victim);
void death_cry(CHAR_DATA *ch);
void raw_kill(CHAR_DATA *ch);
void die(CHAR_DATA *ch, CHAR_DATA *killer);
void group_gain(CHAR_DATA *ch, CHAR_DATA *victim);
void solo_gain(CHAR_DATA *ch, CHAR_DATA *victim);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
void perform_violence(void);
int compute_armor_class(CHAR_DATA *ch);
int compute_thaco(CHAR_DATA *ch, CHAR_DATA *vict);
int get_weapon_prof(CHAR_DATA *ch, OBJ_DATA *wield);

/* Weapon attack texts */
ATTACK_HIT_TYPE attack_hit_text[] =
{
  {"hit", "hits"},				/* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},	/* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"},		/* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"}				/* 14 */
};


/* The Fight related routines */

void appear(CHAR_DATA *ch)
{
	if (affected_by_spell(ch, SPELL_INVISIBLE))
		affect_from_char(ch, SPELL_INVISIBLE);
	
	REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);
	
	if (GET_LEVEL(ch) < LVL_IMMORT)
		act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
	else
		act("You feel a strange presence as $n appears, seemingly from nowhere.",
			FALSE, ch, 0, 0, TO_ROOM);
}


int compute_armor_class(CHAR_DATA *ch)
{
	OBJ_DATA *wielded = GET_EQ(ch, WEAR_WIELD);
	int armorclass = GET_AC(ch);
	
	if (AWAKE(ch))
		armorclass += dex_app[GET_DEX(ch)].defensive * 10;
	
	if (IS_WARRIOR(ch))
		if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
			armorclass += wpn_prof[get_weapon_prof(ch, wielded)].to_ac * 10;

	return (MAX(-100, armorclass));      /* -100 is lowest */
}


void load_messages(void)
{
	FILE *fl;
	MESSAGE_TYPE *messages;
	char chk[128];
	int i, type;
	
	if (!(fl = fopen(MESS_FILE, "r")))
	{
		log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
		exit(1);
	}
	for (i = 0; i < MAX_MESSAGES; i++)
	{
		fight_messages[i].a_type			= 0;
		fight_messages[i].number_of_attacks	= 0;
		fight_messages[i].msg				= 0;
	}
	
	fgets(chk, 128, fl);
	while (!feof(fl) && (*chk == '\n' || *chk == '*'))
		fgets(chk, 128, fl);
	
	while (*chk == 'M')
	{
		fgets(chk, 128, fl);
		sscanf(chk, " %d\n", &type);
		for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
			(fight_messages[i].a_type); i++);
		if (i >= MAX_MESSAGES)
		{
			log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
			exit(1);
		}
		CREATE(messages, MESSAGE_TYPE, 1);
		fight_messages[i].number_of_attacks++;
		fight_messages[i].a_type = type;
		messages->next = fight_messages[i].msg;
		fight_messages[i].msg = messages;
		
		messages->die_msg.attacker_msg	= fread_action(fl, i);
		messages->die_msg.victim_msg	= fread_action(fl, i);
		messages->die_msg.room_msg		= fread_action(fl, i);
		messages->miss_msg.attacker_msg = fread_action(fl, i);
		messages->miss_msg.victim_msg	= fread_action(fl, i);
		messages->miss_msg.room_msg		= fread_action(fl, i);
		messages->hit_msg.attacker_msg	= fread_action(fl, i);
		messages->hit_msg.victim_msg	= fread_action(fl, i);
		messages->hit_msg.room_msg		= fread_action(fl, i);
		messages->god_msg.attacker_msg	= fread_action(fl, i);
		messages->god_msg.victim_msg	= fread_action(fl, i);
		messages->god_msg.room_msg		= fread_action(fl, i);

		fgets(chk, 128, fl);
		while (!feof(fl) && (*chk == '\n' || *chk == '*'))
			fgets(chk, 128, fl);
	}
	
	fclose(fl);
}


void update_pos(CHAR_DATA *victim)
{
	if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
		return;
	else if ( GET_HIT(victim) > 0 && !AFF_FLAGGED(victim, AFF_PARALYSIS) )
		GET_POS(victim) = POS_STANDING;
	else if (GET_HIT(victim) <= -11)
		GET_POS(victim) = POS_DEAD;
	else if (GET_HIT(victim) <= -6)
		GET_POS(victim) = POS_MORTALLYW;
	else if (GET_HIT(victim) <= -3)
		GET_POS(victim) = POS_INCAP;
	else
		GET_POS(victim) = POS_STUNNED;
}


/*
 * clan_at_war
 *
 * FALSE	if ch and vict are NOT members of clans at war
 * TRUE		if ch and vict are members of clans at war or ch or vict are NPCs
 */
bool clan_at_war(CHAR_DATA *ch, CHAR_DATA *vict)
{
	if ( IS_NPC(ch) || IS_NPC(vict) )
		return (TRUE);

	/* Check to see if ch & victim are in clans, and enemies */
	if (GET_CLAN(ch) != -1 && GET_CLAN(vict) != -1 && GET_CLAN(ch) != GET_CLAN(vict))
	{
		if (politics_data.diplomacy[GET_CLAN(ch)][GET_CLAN(vict)] < -450)
			return (TRUE);    
	}

	return (FALSE);
}

/* check to prevent pkilling */
bool pkill_ok(CHAR_DATA *ch, CHAR_DATA *opponent)
{
	if ( ch == opponent )
		return (TRUE);

	if ( !pk_allowed && !IS_NPC(ch) && !IS_NPC(opponent) &&
	     !clan_at_war(ch, opponent) )
		return (FALSE);
	
	return (TRUE);
}

void check_killer(CHAR_DATA *ch, CHAR_DATA *vict)
{
	char buf[256];
	
	if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
		return;
	if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
		return;
	
	SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
	sprintf(buf, "PC Killer bit set on %s for initiating attack on %s at %s.",
		GET_NAME(ch), GET_NAME(vict), ROOM_NAME(vict));
	mudlog(buf, BRF, LVL_IMMORT, TRUE);
	send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(CHAR_DATA *ch, CHAR_DATA *vict)
{
	if (ch == vict)
		return;
	
	if (FIGHTING(ch))
	{
		core_dump();
		return;
	}
	
	ch->next_fighting	= combat_list;
	combat_list			= ch;
	
	if (AFF_FLAGGED(ch, AFF_SLEEP))
		affect_from_char(ch, SPELL_SLEEP);
	
	FIGHTING(ch)		= vict;
	GET_POS(ch)			= POS_FIGHTING;
	
	/* handle ppl using furniture */
	if ( ch->in_obj )	char_from_obj(ch);
	if ( vict->in_obj )	char_from_obj(vict);

	/* check for memorizing chars.. */
	if ( !IS_NPC(vict) && AFF_FLAGGED(vict, AFF_MEMORIZING) )
		stop_memorizing(vict);

	if (!pkill_ok(ch, vict))
		check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void stop_fighting(CHAR_DATA *ch)
{
	CHAR_DATA *temp;
	
	if (ch == next_combat_list)
		next_combat_list = ch->next_fighting;
	
	REMOVE_FROM_LIST(ch, combat_list, next_fighting);
	ch->next_fighting	= NULL;
	FIGHTING(ch)		= NULL;

	GET_POS(ch)			= POS_STANDING;

	update_pos(ch);
}



void make_corpse(CHAR_DATA *ch)
{
	OBJ_DATA *corpse, *o;
	char bufcor[MAX_STRING_LENGTH];
	int i;
	
	corpse = create_obj();
	
	corpse->item_number			= NOTHING;
	corpse->in_room				= NULL;

	sprintf( bufcor, "corpse %s", IS_NPC(ch) ? GET_PC_NAME(ch) : unknown_name(ch) );
	corpse->name				= str_dup(bufcor);
	
	sprintf(buf2, "The corpse of %s is lying here.", unknown_name(ch));
	corpse->description			= str_dup(buf2);
	
	sprintf(buf2, "the corpse of %s", unknown_name(ch));
	corpse->short_description	= str_dup(buf2);
	
	GET_OBJ_TYPE(corpse)		= ITEM_CONTAINER;
	GET_OBJ_WEAR(corpse)		= ITEM_WEAR_TAKE;
	GET_OBJ_EXTRA(corpse)		= ITEM_NODONATE;
	GET_OBJ_VAL(corpse, 0)		= 0;	/* You can't store stuff in a corpse */
	GET_OBJ_VAL(corpse, 3)		= 1;	/* corpse identifier */
	GET_OBJ_WEIGHT(corpse)		= GET_WEIGHT(ch) + IS_CARRYING_W(ch);
	GET_OBJ_RENT(corpse)		= 100000;

	if (IS_NPC(ch))
	{
		GET_OBJ_TIMER(corpse)	= max_npc_corpse_time;
		GET_OBJ_VAL(corpse, 1)	= 0;
	}
	else
	{
		GET_OBJ_TIMER(corpse)	= max_pc_corpse_time;
		GET_OBJ_VAL(corpse, 1)	= GET_IDNUM(ch);
	}
	
	/* transfer character's inventory to the corpse */
	corpse->first_content		= ch->first_carrying;
	corpse->last_content		= ch->last_carrying;

	for (o = corpse->first_content; o; o = o->next_content)
		o->in_obj = corpse;
	object_list_new_owner(corpse, NULL);
	
	/* transfer character's equipment to the corpse */
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
			obj_to_obj(unequip_char(ch, i), corpse);
	}

	/* make gold coins from gold value (Mob only) */
	if (IS_NPC(ch) && GET_GOLD(ch) > 0)
		create_amount(GET_GOLD(ch), NULL, NULL, corpse, NULL, FALSE);

	ch->first_carrying	= NULL;
	ch->last_carrying	= NULL;
	
	IS_CARRYING_N(ch)	= 0;
	IS_CARRYING_W(ch)	= 0;
	
	obj_to_room(corpse, ch->in_room);
}


/* When ch kills victim */
void change_alignment(CHAR_DATA *ch, CHAR_DATA *victim)
{
	/*
	 * new alignment change algorithm: if you kill a monster with alignment A,
	 * you move 1/16th of the way to having alignment -A.  Simple and fast.
	 */
	GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / 16;
}



void death_cry(CHAR_DATA *ch)
{
	EXIT_DATA *pexit;
	
	act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);
	
	for ( pexit = ch->in_room->first_exit; pexit; pexit = pexit->next )
	{
		if ( valid_exit( pexit ) )
			send_to_room("Your blood freezes as you hear someone's death cry.\r\n",
				     pexit->to_room);
	}
}


void raw_kill(CHAR_DATA *ch)
{
	if (FIGHTING(ch))
		stop_fighting(ch);
	
	while (ch->affected)
		affect_remove(ch, ch->affected);
	
	death_cry(ch);
	
	if (IS_NPC(ch))
	{
		make_corpse(ch);
		extract_char(ch);
	}
	else
	{
		char_to_ghost(ch);
		send_to_char("You feel yourself being pulled into a bright light...\r\n", ch);
		char_from_room(ch);
		char_to_room(ch, get_room(10001));
		GET_HIT(ch)		= GET_MAX_HIT(ch);
		GET_MANA(ch)	= GET_MAX_MANA(ch);
		GET_MOVE(ch)	= GET_MAX_MOVE(ch);
		update_pos(ch);
		look_at_room(ch, 0);
		SET_BIT(PLR_FLAGS(ch), PLR_GHOST);
	}
}



void die(CHAR_DATA *ch, CHAR_DATA *killer)
{
	if (killer && ch != killer)
	{
		/* aggiorniamo i contatori delle uccisioni */
		if (!IS_NPC(ch))
		{
			if (IS_NPC(killer))
				GET_MOB_DEATHS(ch)++;
			else
			{
				GET_PLR_DEATHS(ch)++;
				GET_PLR_KILLS(killer)++;
			}
		}
		else
			if (!IS_NPC(killer))
				GET_MOB_KILLS(killer)++;
	}

	gain_exp(ch, -(GET_EXP(ch) / 5));
	if (!IS_NPC(ch))
		REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);
	raw_kill(ch);
}



void perform_group_gain(CHAR_DATA *ch, int base, CHAR_DATA *victim)
{
	int share, exp_after_lim;
	
	if (!IS_NPC(ch) && IS_NPC(victim))
		exp_after_lim = kills_limit_xpgain(ch, victim, base);

	share = MIN(max_exp_gain, MAX(1, exp_after_lim));

	if (share > 1)
	{
		sprintf(buf2, "You receive your share of experience -- %d points.\r\n", share);
		send_to_char(buf2, ch);
	}
	else
		send_to_char("You receive your share of experience -- one measly little point!\r\n", ch);
	
	gain_exp(ch, share);
	change_alignment(ch, victim);
}


void group_gain(CHAR_DATA *ch, CHAR_DATA *victim)
{
	CHAR_DATA *k;
	FOLLOW_DATA *f;
	int tot_members, base, tot_gain;
	
	if (!(k = ch->master))
		k = ch;
	
	if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
		tot_members = 1;
	else
		tot_members = 0;
	
	for (f = k->followers; f; f = f->next)
	{
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
			tot_members++;
	}
	
	/* round up to the next highest tot_members */
	tot_gain = (GET_EXP(victim) / 3) + tot_members - 1;
	
	/* prevent illegal xp creation when killing players */
	if (!IS_NPC(victim))
		tot_gain = MIN(max_exp_loss * 2 / 3, tot_gain);
	
	if (tot_members >= 1)
		base = MAX(1, tot_gain / tot_members);
	else
		base = 0;
	
	if (AFF_FLAGGED(k, AFF_GROUP) && k->in_room == ch->in_room)
		perform_group_gain(k, base, victim);
	
	for (f = k->followers; f; f = f->next)
	{
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower->in_room == ch->in_room)
			perform_group_gain(f->follower, base, victim);
	}
}


void solo_gain(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int exp;
	
	//exp = MIN(max_exp_gain, GET_EXP(victim) / 3);

	exp = GET_EXP(victim) / 3;
	
	/* Limit up XP gain according on amount of kills of current mob */
	if (!IS_NPC(ch) && IS_NPC(victim))
		exp = kills_limit_xpgain(ch, victim, exp);
	
	exp = MIN(max_exp_gain, exp);
	
	/* Calculate level-difference bonus */
	if (IS_NPC(ch))
		exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
	else
		exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
	
	exp = MAX(exp, 1);
	
	if (exp > 1)
	{
		sprintf(buf2, "You receive %d experience points.\r\n", exp);
		send_to_char(buf2, ch);
	}
	else
		send_to_char("You receive one lousy experience point.\r\n", ch);
	
	gain_exp(ch, exp);
	change_alignment(ch, victim);
}


char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
	static char buf[256];
	char *cp = buf;
	
	for (; *str; str++)
	{
		if (*str == '#')
		{
			switch (*(++str))
			{
			case 'W':
				for (; *weapon_plural; *(cp++) = *(weapon_plural++));
				break;
			case 'w':
				for (; *weapon_singular; *(cp++) = *(weapon_singular++));
				break;
			default:
				*(cp++) = '#';
				break;
			}
		}
		else
			*(cp++) = *str;
		
		*cp = 0;
	}				/* For */
	
	return (buf);
}


/* message for doing damage with a weapon */
void dam_message(int dam, CHAR_DATA *ch, CHAR_DATA *victim, int w_type)
{
	char *buf;
	int msgnum;
	
	static struct dam_weapon_type
	{
	  const char	*to_room;
	  const char	*to_char;
	  const char	*to_victim;
	} dam_weapons[] =
	{
		/* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */
		{
			"$n tries to #w $N, but misses.",	/* 0: 0     */
			"You try to #w $N, but miss.",
			"$n tries to #w you, but misses."
		},
		{
			"$n tickles $N as $e #W $M.",	/* 1: 1..2  */
			"You tickle $N as you #w $M.",
			"$n tickles you as $e #W you."
		},
		{
			"$n barely #W $N.",		/* 2: 3..4  */
			"You barely #w $N.",
			"$n barely #W you."
		},
		{
			"$n #W $N.",			/* 3: 5..6  */
			"You #w $N.",
			"$n #W you."
		},
		{
			"$n #W $N hard.",			/* 4: 7..10  */
			"You #w $N hard.",
			"$n #W you hard."
		},
		{
			"$n #W $N very hard.",		/* 5: 11..14  */
			"You #w $N very hard.",
			"$n #W you very hard."
		},
		{
			"$n #W $N extremely hard.",	/* 6: 15..19  */
			"You #w $N extremely hard.",
			"$n #W you extremely hard."
		},
		{
			"$n massacres $N to small fragments with $s #w.",	/* 7: 19..23 */
			"You massacre $N to small fragments with your #w.",
			"$n massacres you to small fragments with $s #w."
		},
		{
			"$n OBLITERATES $N with $s deadly #w!!",	/* 8: > 23   */
			"You OBLITERATE $N with your deadly #w!!",
			"$n OBLITERATES you with $s deadly #w!!"
		}
	};
	

	w_type -= TYPE_HIT;		/* Change to base of table with text */
	
	if		(dam == 0)	msgnum = 0;
	else if (dam <= 2)	msgnum = 1;
	else if (dam <= 4)	msgnum = 2;
	else if (dam <= 6)	msgnum = 3;
	else if (dam <= 10)	msgnum = 4;
	else if (dam <= 14)	msgnum = 5;
	else if (dam <= 19)	msgnum = 6;
	else if (dam <= 23)	msgnum = 7;
	else				msgnum = 8;
	
	/* damage message to onlookers */
	buf = replace_string(dam_weapons[msgnum].to_room,
		attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);
	
	/* damage message to damager */
	send_to_char(CCYEL(ch, C_CMP), ch);
	buf = replace_string(dam_weapons[msgnum].to_char,
		attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	act(buf, FALSE, ch, NULL, victim, TO_CHAR);
	send_to_char(CCNRM(ch, C_CMP), ch);
	
	/* damage message to damagee */
	send_to_char(CCRED(victim, C_CMP), victim);
	buf = replace_string(dam_weapons[msgnum].to_victim,
		attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
	act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
	send_to_char(CCNRM(victim, C_CMP), victim);
}


/*
 *  message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, CHAR_DATA *ch, CHAR_DATA *vict, int attacktype)
{
	MESSAGE_TYPE *msg;
	OBJ_DATA *weap = GET_EQ(ch, WEAR_WIELD);
	int i, j, nr;
	
	for (i = 0; i < MAX_MESSAGES; i++)
	{
		if (fight_messages[i].a_type == attacktype)
		{
			nr = dice(1, fight_messages[i].number_of_attacks);
			for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
				msg = msg->next;
			
			if (!IS_NPC(vict) && (GET_LEVEL(vict) >= LVL_IMMORT))
			{
				act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
				act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
				act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
			}
			else if (dam != 0)
			{
				/*
				 * Don't	send redundant color codes for TYPE_SUFFERING & other types
				 * of damage without attacker_msg.
				 */
				if (GET_POS(vict) == POS_DEAD)
				{
					if (msg->die_msg.attacker_msg)
					{
						send_to_char(CCYEL(ch, C_CMP), ch);
						act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
						send_to_char(CCNRM(ch, C_CMP), ch);
					}
					
					send_to_char(CCRED(vict, C_CMP), vict);
					act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
					send_to_char(CCNRM(vict, C_CMP), vict);
					
					act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
				}
				else
				{
					if (msg->hit_msg.attacker_msg)
					{
						send_to_char(CCYEL(ch, C_CMP), ch);
						act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
						send_to_char(CCNRM(ch, C_CMP), ch);
					}
					
					send_to_char(CCRED(vict, C_CMP), vict);
					act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
					send_to_char(CCNRM(vict, C_CMP), vict);
					
					act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
				}
			}
			else if (ch != vict)	/* Dam == 0 */
			{
				if (msg->miss_msg.attacker_msg)
				{
					send_to_char(CCYEL(ch, C_CMP), ch);
					act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
					send_to_char(CCNRM(ch, C_CMP), ch);
				}
				
				send_to_char(CCRED(vict, C_CMP), vict);
				act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
				send_to_char(CCNRM(vict, C_CMP), vict);
				
				act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
			}
			return (1);
		}
	}
	return (0);
}

/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */
int damage(CHAR_DATA *ch, CHAR_DATA *victim, int dam, int attacktype)
{
	ROOM_DATA *vict_room = NULL;
	bool missile = FALSE;

	if (GET_POS(victim) <= POS_DEAD)
	{
		/* This is "normal"-ish now with delayed extraction. -gg 3/15/2001 */
		if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
			return (-1);
		
		log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
			GET_NAME(victim), victim->in_room->number, GET_NAME(ch));
		die(victim, NULL);
		return (-1);			/* -je, 7/7/92 */
	}
	
	if (ch->in_room != victim->in_room)
	{
		missile = TRUE;
		vict_room = victim->in_room;
	}

	/* peaceful rooms */
	if (ch != victim && ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
	{
		send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
		return (0);
	}
	
	/* shopkeeper protection */
	if (!ok_damage_shopkeeper(ch, victim))
		return (0);
	
	/* You can't damage an immortal! */
	if (IS_IMMORTAL(victim))
		dam = 0;
	
	if (victim != ch && !missile)
	{
		/* Start the attacker fighting the victim */
		if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
			set_fighting(ch, victim);
		
		/* Start the victim fighting the attacker */
		if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL))
		{
			set_fighting(victim, ch);
			if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
				remember(victim, ch);
		}
	}
	
	/* If you attack a pet, it hates your guts */
	if (victim->master == ch)
		stop_follower(victim);
	
	/* If the attacker is invisible, he becomes visible */
	if (AFF_FLAGGED(ch, AFF_INVISIBLE | AFF_HIDE))
		appear(ch);
	
	/* Cut damage in half if victim has sanct, to a minimum 1 */
	if (AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
		dam /= 2;
	
	/* Check for PK if this is not a PK MUD */
	if ( !pkill_ok(ch, victim) )
	{
		check_killer(ch, victim);
		if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
			dam = 0;
	}
	
	/* Set the maximum damage per round and subtract the hit points */
	dam = MAX(MIN(dam, 200), 0);
	GET_HIT(victim) -= dam;
	
	/* Being hit destroys equipment... */
	if (dam > 0)
		check_damage_obj(victim, NULL, 7);

	update_pos(victim);
	
	/*
	 * skill_message sends a message from the messages file in lib/misc.
	 * dam_message just sends a generic "You hit $n extremely hard.".
	 * skill_message is preferable to dam_message because it is more
	 * descriptive.
	 * 
	 * If we are _not_ attacking with a weapon (i.e. a spell), always use
	 * skill_message. If we are attacking with a weapon: If this is a miss or a
	 * death blow, send a skill_message if one exists; if not, default to a
	 * dam_message. Otherwise, always send a dam_message.
	 */
	if (attacktype != -1)
	{
	if (!IS_WEAPON(attacktype) || missile)
		skill_message(dam, ch, victim, attacktype);
	else
	{
		if (GET_POS(victim) == POS_DEAD || dam == 0)
		{
			if (!skill_message(dam, ch, victim, attacktype))
				dam_message(dam, ch, victim, attacktype);
		}
		else
			dam_message(dam, ch, victim, attacktype);
	}
	}
	
	/* Use send_to_char -- act() doesn't send message if you are DEAD. */
	switch (GET_POS(victim))
	{
	case POS_MORTALLYW:
		act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
		send_to_char("You are mortally wounded, and will die soon, if not aided.\r\n", victim);
		break;
	case POS_INCAP:
		act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
		send_to_char("You are incapacitated an will slowly die, if not aided.\r\n", victim);
		break;
	case POS_STUNNED:
		act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
		send_to_char("You're stunned, but will probably regain consciousness again.\r\n", victim);
		break;
	case POS_DEAD:
		act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
		send_to_char("You are dead!  Sorry...\r\n", victim);
		break;
		
	default:			/* >= POSITION SLEEPING */
		if (dam > (GET_MAX_HIT(victim) / 4))
			send_to_char("That really did HURT!\r\n", victim);
		
		if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4))
		{
			sprintf(buf2, "%sYou wish that your wounds would stop BLEEDING so much!%s\r\n",
				CCRED(victim, C_SPR), CCNRM(victim, C_SPR));
			send_to_char(buf2, victim);
			if (ch != victim && MOB_FLAGGED(victim, MOB_WIMPY))
				do_flee(victim, NULL, 0, 0);
		}
		if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
			GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0)
		{
			send_to_char("You wimp out, and attempt to flee!\r\n", victim);
			do_flee(victim, NULL, 0, 0);
		}
		break;
	}
	
	/* Help out poor linkless people who are attacked */
	if (victim != ch && !IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED)
	{
		do_flee(victim, NULL, 0, 0);
		if (!FIGHTING(victim))
		{
			act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
			GET_WAS_IN(victim) = victim->in_room;
			char_from_room(victim);
			char_to_room(victim, get_room(0));
		}
	}
	
	/* stop someone from fighting if they're stunned or worse */
	if (GET_POS(victim) <= POS_STUNNED && FIGHTING(victim) != NULL)
		stop_fighting(victim);
	
	/* Uh oh.  Victim died. */
	if (GET_POS(victim) == POS_DEAD)
	{
		if (ch != victim && (IS_NPC(victim) || victim->desc))
		{
			if (AFF_FLAGGED(ch, AFF_GROUP))
				group_gain(ch, victim);
			else
				solo_gain(ch, victim);
		}
		
		if (ch != victim && !IS_NPC(victim))
		{
			sprintf(buf2, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), ROOM_NAME(victim));
			mudlog(buf2, BRF, LVL_IMMORT, TRUE);
			if (IS_NPC(ch))
				brag(ch, victim);
			if (MOB_FLAGGED(ch, MOB_MEMORY))
				forget(ch, victim);
		}
		die(victim, ch);
		return (-1);
	}
	/* mob reaction to a ranged attack */
	else if (victim != ch && missile)
	{	
		if (IS_NPC(victim) && !FIGHTING(victim) && GET_POS(victim) > POS_STUNNED)
		{
			/* can remember so charge! */
			if (MOB_FLAGGED(victim, MOB_MEMORY))
			{
				remember(victim, ch);
				sprintf(buf, "$n bellows in pain!");
				act(buf, FALSE, victim, 0, 0, TO_ROOM);

				if (GET_POS(victim) == POS_STANDING)
				{
					int dir, moved = 0;

					if (IN_WILD(victim))
						dir = wild_track(ch, victim, TRUE);
					else
						dir = find_first_step(victim->in_room, ch->in_room);
					
					if (dir != NOWHERE)
					{
						if (!do_simple_move(victim, dir, 1))
							act("$n stumbles while trying to run!", FALSE, victim, 0, 0, TO_ROOM);
						else
							moved = 1;
					}

					if (!moved && MOB_FLAGGED(victim, MOB_HUNTER))
						start_hunting(victim, ch);
				}
			}
			/* can't remember so try to run away if not already fleed */
			else
			{
				if (vict_room == victim->in_room)
					do_flee(victim, NULL, 0, 0);
			}
		}
	}

	return (dam);
}


/*
 * Calculate the THAC0 of the attacker.
 *
 * 'victim' currently isn't used but you could use it for special cases like
 * weapons that hit evil creatures easier or a weapon that always misses
 * attacking an animal.
 */
int compute_thaco(CHAR_DATA *ch, CHAR_DATA *victim)
{
	int calc_thaco;
	
	if (!IS_NPC(ch))
		calc_thaco = thaco(GET_CLASS(ch), GET_LEVEL(ch));
	else		/* THAC0 for monsters is set in the HitRoll */
		calc_thaco = 20;

	calc_thaco -= str_app[GET_STR(ch)].tohit;
	calc_thaco -= GET_HITROLL(ch);
	calc_thaco -= (GET_INT(ch) - 13) / 1.5;	/* Intelligence helps! */
	calc_thaco -= (GET_WIS(ch) - 13) / 1.5;	/* So does wisdom */
	
	return (calc_thaco);
}


void hit(CHAR_DATA *ch, CHAR_DATA *victim, int type)
{
	OBJ_DATA *wielded = GET_EQ(ch, WEAR_WIELD);
	int w_type, victim_ac, calc_thaco, dam, diceroll, ret;
	int wpnprof;
	
	/* Do some sanity checking, just in case.. */
	if ( !ch || !victim )
		return;

	/* Do some sanity checking, in case someone flees, etc. */
	if (ch->in_room != victim->in_room)
	{
		if (FIGHTING(ch) && FIGHTING(ch) == victim)
			stop_fighting(ch);
		return;
	}
	
	/* Find the weapon type (for display purposes only) */
	if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
		w_type = GET_OBJ_VAL(wielded, 3) + TYPE_HIT;
	else
	{
		if (IS_NPC(ch) && GET_ATTACK(ch) != 0)
			w_type = GET_ATTACK(ch) + TYPE_HIT;
		else
			w_type = TYPE_HIT;
	}
	
	/* Weapon proficiencies */
	if (IS_WARRIOR(ch) && wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
		wpnprof = get_weapon_prof(ch, wielded);
	else
		wpnprof = 0;

	/* Calculate chance of hit. Lower THAC0 is better for attacker. */
	calc_thaco = compute_thaco(ch, victim);
	calc_thaco -= wpn_prof[wpnprof].to_hit;
	
	/* Calculate the raw armor including magic armor.  Lower AC is better for defender. */
	victim_ac = compute_armor_class(victim) / 10;

	if ( RIDING(victim) )
		victim_ac -= 5;
	
	/* roll the die and take your chances... */
	diceroll = number(1, 20);

	/*
	 * Decide whether this is a hit or a miss.
	 *
	 *  Victim asleep = hit, otherwise:
	 *     1   = Automatic miss.
	 *   2..19 = Checked vs. AC.
	 *    20   = Automatic hit.
	 */
	if (diceroll == 20 || !AWAKE(victim))
		dam = TRUE;
	else if (diceroll == 1)
		dam = FALSE;
	else
		dam = (calc_thaco - diceroll <= victim_ac);
	
	if (!dam)
	{
		damage(ch, victim, 0, type == SKILL_BACKSTAB ? SKILL_BACKSTAB : w_type);
		return;
	}

	/* okay, we know the guy has been hit.  now calculate damage. */
	
	/* Start with the damage bonuses: the damroll and strength apply */
	dam = str_app[GET_STR(ch)].todam;
	dam += GET_DAMROLL(ch);
	
	/* Maybe holding arrow? */
	if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
	{
		/* Add weapon-based damage if a weapon is being wielded */
		dam += dice(GET_OBJ_VAL(wielded, 1), GET_OBJ_VAL(wielded, 2));
		dam += wpn_prof[wpnprof].to_dam;
	}
	else
	{
		/* If no weapon, add bare hand damage instead */
		if (IS_NPC(ch))
			dam += dice(GET_NDD(ch), GET_SDD(ch));
		else
			dam += number(0, 2);	/* Max 2 bare hand damage for players */
	}
	
	if ( RIDING(ch) )
		dam += MAX(10, GET_DAMROLL(ch)/10);
	
	/*
	 * Include a damage multiplier if victim isn't ready to fight:
	 *
	 * Position sitting  1.33 x normal
	 * Position resting  1.66 x normal
	 * Position sleeping 2.00 x normal
	 * Position stunned  2.33 x normal
	 * Position incap    2.66 x normal
	 * Position mortally 3.00 x normal
	 *
	 * Note, this is a hack because it depends on the particular
	 * values of the POSITION_XXX constants.
	 */
	if (GET_POS(victim) < POS_FIGHTING)
		dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;
	
	/* at least 1 hp damage min per hit */
	dam = MAX(1, dam);
	
	if (type == SKILL_BACKSTAB)
		ret = damage(ch, victim, dam * backstab_mult(GET_LEVEL(ch)), SKILL_BACKSTAB);
	else
		ret = damage(ch, victim, dam, w_type);
	
	/* fireshield case */
	if ( ret != -1 && type >= TYPE_HIT )
	{
		if (AFF_FLAGGED(victim, AFF_FIRESHIELD) && !AFF_FLAGGED(ch, AFF_FIRESHIELD))
			damage(victim, ch, dam / 2, SPELL_FIREBALL);
	}
	
	/* check for weapon being damaged */
	if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
	{
		/* call for a magical effect from the weapon */
		if (FIGHTING(ch) && victim && FIGHTING(ch) == victim)
			weapon_spells(ch, victim, wielded);
		
		/* check for weapon being damaged */
		check_damage_obj(ch, wielded, 3);
	}
}


bool valid_target(CHAR_DATA *ch, CHAR_DATA *vict)
{
	if ( !ch || !vict )
		return (FALSE);

	if ( MOB_FLAGGED(ch, MOB_NOTDEADYET) )
		return (FALSE);
	if ( MOB_FLAGGED(vict, MOB_NOTDEADYET) )
		return (FALSE);

	if ( PLR_FLAGGED(ch, PLR_NOTDEADYET) )
		return (FALSE);
	if ( PLR_FLAGGED(vict, PLR_NOTDEADYET) )
		return (FALSE);

	return (TRUE);
}



/* control the fights going on.  Called every 2 seconds from comm.c. */
void perform_violence(void)
{
	CHAR_DATA *ch;
	OBJ_DATA *wielded;
	sh_int num_of_attacks = 1, loop_attacks;
	
	for (ch = combat_list; ch; ch = next_combat_list)
	{
		next_combat_list = ch->next_fighting;
		
		if (FIGHTING(ch) == NULL || ch->in_room != FIGHTING(ch)->in_room || !valid_target(ch, FIGHTING(ch)))
		{
			stop_fighting(ch);
			continue;
		}

		if (IS_NPC(ch))
		{
			if (GET_MOB_WAIT(ch) > 0)
			{
				GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
				continue;
			}
			GET_MOB_WAIT(ch) = 0;
			if (GET_POS(ch) < POS_FIGHTING)
			{
				GET_POS(ch) = POS_FIGHTING;
				act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
			}
		}
		
		if (GET_POS(ch) < POS_FIGHTING)
		{
			send_to_char("You can't fight while sitting!!\r\n", ch);
			continue;
		}
		
		/* reset to 1 attack */
		num_of_attacks = 1;
		
		if (IS_WARRIOR(ch))
		{
			wielded = GET_EQ(ch, WEAR_WIELD);
			if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON)
				num_of_attacks += wpn_prof[get_weapon_prof(ch, wielded)].num_of_attacks;
		}
		else
			num_of_attacks += MAX(1, ((int) GET_LEVEL(ch) / 10) - 1);
		
		for (loop_attacks = 0; loop_attacks < num_of_attacks && valid_target(ch, FIGHTING(ch)); loop_attacks++)
			hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
		
		if (valid_target(ch, FIGHTING(ch)))
			diag_char_to_char(FIGHTING(ch), ch);

		if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) && !MOB_FLAGGED(ch, MOB_NOTDEADYET))
		{
			char actbuf[MAX_INPUT_LENGTH] = "";

			(GET_MOB_SPEC(ch)) (ch, ch, 0, actbuf);
		}
	}
}

/* Weapon Proficiecies */
int get_weapon_prof(CHAR_DATA *ch, OBJ_DATA *wield)
{
	int bonus = 0, learned = 0, type = -1;
	
	/* No mobs and non-warriors here */
	if (!IS_WARRIOR(ch))
		return (bonus);
	
	/* Check for proficiencies only if characters could have learned it */
	if (GET_LEVEL(ch) < PROF_LVL_START)
		return (bonus);
	
	switch (GET_OBJ_VAL(wield, 3) + TYPE_HIT)
	{
	case TYPE_SLASH:
		type = SKILL_WEAPON_SWORDS;
		break;
		
	case TYPE_STING:
	case TYPE_PIERCE:
	case TYPE_STAB:
		type = SKILL_WEAPON_DAGGERS;
		break;
		
	case TYPE_THRASH:
	case TYPE_WHIP:
		type = SKILL_WEAPON_WHIPS;
		break;
		
	case TYPE_CLAW:
		type = SKILL_WEAPON_TALONOUS_ARMS;
		break;
		
	case TYPE_BLUDGEON:
	case TYPE_MAUL:
	case TYPE_POUND:
	case TYPE_CRUSH:
		type = SKILL_WEAPON_BLUDGEONS;
		break;
		
	case TYPE_HIT:
	case TYPE_PUNCH:
	case TYPE_BITE:
	case TYPE_BLAST:
		type = SKILL_WEAPON_EXOTICS;
		break;
		
	default:
		type = -1;
		break;
	}
	
	if (type != -1)
	{
		learned = GET_SKILL(ch, type);

		if		(learned == 0)		bonus = 10;
		else if (learned <= 20)		bonus = 1;
		else if (learned <= 40)		bonus = 2;
		else if (learned <= 60)		bonus = 3;
		else if (learned <= 80)		bonus = 4;
		else if (learned <= 85)		bonus = 5;
		else if (learned <= 90)		bonus = 6;
		else if (learned <= 95)		bonus = 7;
		else if (learned <= 99)		bonus = 8;
		else						bonus = 9;
	}

	return (bonus);
}

/* ******************************************************************** */
/* Ranged Weapons functions                                             */
/* ******************************************************************** */

void strike_missile(CHAR_DATA *ch, CHAR_DATA *tch, OBJ_DATA *missile, int dir, int attacktype)
{
	ROOM_DATA *vict_room = NULL;
	int dam, alive;
	char victname[MAX_STRING_LENGTH];
	
	dam = dice(GET_OBJ_VAL(missile, 1), GET_OBJ_VAL(missile, 2));
	dam += GET_OBJ_VAL(missile, 3);
	
	strcpy(victname, PERS(tch, ch));
	vict_room = tch->in_room;
	
	alive = damage(ch, tch, dam, attacktype);
	
	send_to_char("&b&2You hit!&0\r\n", ch);
	sprintf(buf, "%s flies in from the %s and %s %s.", missile->short_description,
		dirs[rev_dir[dir]], (alive == -1 ? "kills" : "strikes"), victname);
	send_to_room(buf, vict_room);
	if (alive >= 0 )
	{
		sprintf(buf, "&b&3$P flies in from the %s and hits YOU!&0", dirs[rev_dir[dir]]);
		act(buf, FALSE, tch, 0, missile, TO_CHAR);
	}
	return;
} 


void miss_missile(CHAR_DATA *ch, CHAR_DATA *tch, OBJ_DATA *missile, int dir, int attacktype)
{
	sprintf(buf, "$P flies in from the %s and hits the ground!", dirs[rev_dir[dir]]);
	act(buf, FALSE, tch, 0, missile, TO_ROOM);
	act(buf, FALSE, tch, 0, missile, TO_CHAR);
	send_to_char("You missed!\r\n", ch);
}


void fire_missile(CHAR_DATA *ch, char *arg1, OBJ_DATA *missile, int pos, int range, EXIT_DATA *pexit)
{
	ROOM_DATA *nextroom;
	CHAR_DATA *vict;
	bool shot = FALSE;
	int attacktype, distance, dir;
	
	if ( !pexit )
		return;

	if ( !*arg1 )
	{
		send_to_char("Shoot who?\r\n", ch );
		return;
	}

	dir = pexit->vdir;

	if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
	{
		send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
		return;
	}
	
	if ((nextroom = pexit->to_room) == NULL || !valid_exit(pexit))
	{
		send_to_char("Can't find your target!\r\n", ch);
		return;
	}

	for (distance = 1; nextroom != NULL && distance <= range; distance++)
	{
		for (vict = nextroom->people; vict; vict = vict->next_in_room)
		{
			if ((isname(arg1, take_name(vict, ch))) && (CAN_SEE(ch, vict)))
				break;
		}
		
		if (vict)
		{
			if (missile && ROOM_FLAGGED(vict->in_room, ROOM_PEACEFUL))
			{
				send_to_char("Nah.  Leave them in peace.\r\n", ch);
				return;
			}
			
			switch (GET_OBJ_TYPE(missile))
			{
			case ITEM_MISSILE:
				act("$n aims and fires!", TRUE, ch, 0, 0, TO_ROOM);
				send_to_char("You aim and fire!\r\n", ch);
				switch ( GET_OBJ_VAL(missile, 0) )
				{
				default:	attacktype = TYPE_UNDEFINED;		break;
				case 1:		attacktype = SKILL_WEAPON_BOW;		break;
				case 2:		attacktype = SKILL_WEAPON_SLING;	break;
				case 3:		attacktype = SKILL_WEAPON_CROSSBOW;	break;
				}
				break;
			default:
				attacktype = TYPE_UNDEFINED;
				break;
			}
			
			if (attacktype != TYPE_UNDEFINED)
				shot = success(ch, vict, attacktype, 0);
			else
				shot = FALSE;
			
			if (shot)
				strike_missile(ch, vict, missile, dir, attacktype);
			else
				miss_missile(ch, vict, missile, dir, attacktype);

			extract_obj(missile);
			
			WAIT_STATE(ch, PULSE_VIOLENCE);
			return; 
		} 
		
		nextroom = find_room(pexit->to_room, dir);
	}
	
	send_to_char("Can't find your target!\r\n", ch);

	/* put the missile back into container.. sigh! */
	if (missile)
	{
		OBJ_DATA *cont;
		
		cont = get_missile_cont( ch, GET_OBJ_VAL(missile, 0) );
		
		if ( !cont )
			extract_obj(missile);
		else
		{
			obj_from_obj(missile);
			obj_to_obj(missile, cont);
		}
	}
}
