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
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
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
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"


/* external variables */
extern int max_exp_gain;
extern int max_exp_loss;
extern int idle_rent_time;
extern int idle_max_level;
extern int idle_void;
extern int use_autowiz;
extern int min_wizlist_lev;
extern int free_rent;

/* external functions */
char	*title_male(int chclass, int level);
char	*title_female(int chclass, int level);
int		tot_exp_to_level(int level);
void	update_char_objects(CHAR_DATA *ch);
void	reboot_wizlists(void);

/* local functions */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
void check_autowiz(CHAR_DATA *ch);

/* When age < 15 return the value p0 */
/* When age in 15..29 calculate the line between p1 & p2 */
/* When age in 30..44 calculate the line between p2 & p3 */
/* When age in 45..59 calculate the line between p3 & p4 */
/* When age in 60..79 calculate the line between p4 & p5 */
/* When age >= 80 return the value p6 */
int graf(int age, int p0, int p1, int p2, int p3, int p4, int p5, int p6)
{
	if (age < 15)
		return (p0);					/* < 15   */
	else if (age <= 29)
		return (p1 + (((age - 15) * (p2 - p1)) / 15));	/* 15..29 */
	else if (age <= 44)
		return (p2 + (((age - 30) * (p3 - p2)) / 15));	/* 30..44 */
	else if (age <= 59)
		return (p3 + (((age - 45) * (p4 - p3)) / 15));	/* 45..59 */
	else if (age <= 79)
		return (p4 + (((age - 60) * (p5 - p4)) / 20));	/* 60..79 */
	else
		return (p6);					/* >= 80 */
}


/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int mana_gain(CHAR_DATA *ch)
{
	int gain;
	
	if (IS_NPC(ch))
	{
		/* Neat and fast */
		gain = GET_LEVEL(ch);
	}
	else
	{
		gain = graf(age(ch)->year, 4, 8, 12, 16, 12, 10, 8);
		
		/* Class calculations */
		
		/* Skill/Spell calculations */
		
		/* Position calculations    */
		switch (GET_POS(ch))
		{
		case POS_SLEEPING:
			gain *= 2;
			break;
		case POS_RESTING:
			gain += (gain / 2);	/* Divide by 2 */
			break;
		case POS_SITTING:
			gain += (gain / 4);	/* Divide by 4 */
			break;
		}
		
		if (IS_MAGIC_USER(ch) || IS_CLERIC(ch) || IS_SORCERER(ch))
		{
			if ( ch->in_obj && OBJ_FLAGGED(ch->in_obj, ITEM_FAST_MAGIC) )
				gain *= 10;
			else
				gain *= 2;
		}
		
		if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
			gain /= 4;
	}
	
	if (AFF_FLAGGED(ch, AFF_POISON))
		gain /= 4;
	
	return (gain);
}


/* Hitpoint gain pr. game hour */
int hit_gain(CHAR_DATA *ch)
{
	int gain;
	
	if (IS_NPC(ch))
	{
		/* Neat and fast */
		gain = GET_LEVEL(ch);
	}
	else
	{
		
		gain = graf(age(ch)->year, 8, 12, 20, 32, 16, 10, 4);
		
		/* Class/Level calculations */
		
		/* Skill/Spell calculations */
		
		/* Position calculations    */
		
		switch (GET_POS(ch))
		{
		case POS_SLEEPING:
			gain += (gain / 2);	/* Divide by 2 */
			break;
		case POS_RESTING:
			gain += (gain / 4);	/* Divide by 4 */
			break;
		case POS_SITTING:
			gain += (gain / 8);	/* Divide by 8 */
			break;
		}
		
		if (IS_MAGIC_USER(ch) || IS_CLERIC(ch) || IS_SORCERER(ch))
			gain /= 2;	/* Ouch. */
		
		if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
			gain /= 4;
	}
	
	if (AFF_FLAGGED(ch, AFF_REGENERATION))
		gain *= 2;

	if (AFF_FLAGGED(ch, AFF_POISON))
		gain /= 4;
	
	return (gain);
}



/* move gain pr. game hour */
int move_gain(CHAR_DATA *ch)
{
	int gain;
	
	if (IS_NPC(ch))
	{
		/* Neat and fast */
		gain = GET_LEVEL(ch);
	}
	else
	{
		gain = graf(age(ch)->year, 16, 20, 24, 20, 16, 12, 10);
		
		/* Class/Level calculations */
		
		/* Skill/Spell calculations */
		
		
		/* Position calculations    */
		switch (GET_POS(ch))
		{
		case POS_SLEEPING:
			gain += (gain / 2);	/* Divide by 2 */
			break;
		case POS_RESTING:
			gain += (gain / 4);	/* Divide by 4 */
			break;
		case POS_SITTING:
			gain += (gain / 8);	/* Divide by 8 */
			break;
		}
		
		if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
			gain /= 4;
	}
	
	if (AFF_FLAGGED(ch, AFF_POISON))
		gain /= 4;
	
	return (gain);
}



void set_title(CHAR_DATA *ch, char *title)
{
	if (title == NULL)
	{
		if (GET_SEX(ch) == SEX_FEMALE)
			title = title_female(GET_CLASS(ch), GET_LEVEL(ch));
		else
			title = title_male(GET_CLASS(ch), GET_LEVEL(ch));
	}
	
	if (strlen(title) > MAX_TITLE_LENGTH)
		title[MAX_TITLE_LENGTH] = '\0';
	
	if (GET_TITLE(ch) != NULL)
		free(GET_TITLE(ch));
	
	GET_TITLE(ch) = str_dup(title);
}


void check_autowiz(CHAR_DATA *ch)
{
#if defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS)
  if (use_autowiz && GET_LEVEL(ch) >= LVL_IMMORT)
  {
    char buf[128];

#if defined(CIRCLE_UNIX)
    sprintf(buf, "nice ../bin/autowiz %d %s %d %s %d &", min_wizlist_lev,
	    WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE, (int) getpid());
#elif defined(CIRCLE_WINDOWS)
    sprintf(buf, "autowiz %d %s %d %s", min_wizlist_lev,
	    WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE);
#endif /* CIRCLE_WINDOWS */

    mudlog("Initiating autowiz.", CMP, LVL_IMMORT, FALSE);
    system(buf);
    reboot_wizlists();
  }
#endif /* CIRCLE_UNIX || CIRCLE_WINDOWS */
}


/* Killing same mobs XP gain limit function */

/* You can change formulas below if you wish */

#define GET_PERCENT(num, from_v) ((int)((float)(from_v) / 100 * (num)))

/* This formula is responsible for very slow increase of XP penalties   */
#define KILL_UNDER_MF_FORMULA(exp, num_of_kills)                        \
        (MAX(1, GET_PERCENT((101 - num_of_kills), exp)))

/* This formula is responsible for fast increase of XP penalties        */
#define KILL_ABOVE_MF_FORMULA(exp, num_of_kills)                        \
        (MAX(1, GET_PERCENT((float)100/((float)num_of_kills/2+1), exp)))

int kills_limit_xpgain(CHAR_DATA *ch, CHAR_DATA *victim, int exp)
{
	byte current_entry;
	int victim_vnum = GET_MOB_VNUM(victim);
	int new_exp;
	int i;
	
	/* Find corresponding to victim's vnum entry in array or create one */
	
	i = GET_KILLS_CURPOS(ch);
	while (i >= 1 && GET_KILLS_VNUM(ch, i) != victim_vnum)
		--i;

	if (!i)
	{
		if (!GET_KILLS_VNUM(ch, GET_KILLS_CURPOS(ch)))
			/* Array still isn't full - don't scan second part */
			i = GET_KILLS_CURPOS(ch);
		else
		{
			/* Array is full - must scan second part too */
			i = 100;
			while (i > GET_KILLS_CURPOS(ch) && GET_KILLS_VNUM(ch, i) != victim_vnum)
				--i;
		}
		if (i == GET_KILLS_CURPOS(ch))
		{
			/* We came back to search starting point: */
			/* New type of mob killed, add it to list */
			GET_KILLS_VNUM(ch, i)	= victim_vnum;
			GET_KILLS_AMOUNT(ch, i) = 0;
			if (GET_KILLS_CURPOS(ch) < 100)
				GET_KILLS_CURPOS(ch)++;
			else
				GET_KILLS_CURPOS(ch) = 1;
		}
	}
	current_entry = i;
	
	/* Ok, now we have current_entry, pointing to current mob's entry in array */
	/* Lets increment number of kills and reduce XP gain */
	
	if (GET_KILLS_AMOUNT(ch, current_entry) < 60000)
		++GET_KILLS_AMOUNT(ch, current_entry);
	
	if (GET_KILLS_AMOUNT(ch, current_entry) > GET_MOB_MAXFACTOR(victim))
	{
		new_exp = KILL_UNDER_MF_FORMULA(exp, GET_MOB_MAXFACTOR(victim));
		new_exp = KILL_ABOVE_MF_FORMULA(new_exp,
			(GET_KILLS_AMOUNT(ch, current_entry) - GET_MOB_MAXFACTOR(victim)));
	}
	else
	{
		if (GET_KILLS_AMOUNT(ch, current_entry) > 1)
			new_exp = KILL_UNDER_MF_FORMULA(exp, GET_KILLS_AMOUNT(ch, current_entry));
		else
			new_exp = exp; /* MOB was killed for first time, just give the EXP */
	}
	
	return (new_exp);
}


void gain_exp(CHAR_DATA *ch, int gain)
{
	int is_altered = FALSE;
	int num_levels = 0;
	char buf[128];
	
	if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT - 1)))
		return;
	
	if (IS_NPC(ch))
	{
		GET_EXP(ch) += gain;
		return;
	}

	if (gain > 0)
	{
		gain = MIN(max_exp_gain, gain);	/* put a cap on the max gain per kill */
		GET_EXP(ch) += gain;
		while (GET_LEVEL(ch) < LVL_IMMORT &&
			GET_EXP(ch) >= tot_exp_to_level(GET_LEVEL(ch) + 1))
		{
			GET_LEVEL(ch) += 1;
			GET_TOT_LEVEL(ch) += 1;
			num_levels++;
			advance_level(ch);
			is_altered = TRUE;
		}
		
		if (is_altered)
		{
			sprintf(buf, "%s advanced %d level%s to level %d.",
				GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s",
				GET_LEVEL(ch));
			mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
			if (num_levels == 1)
				send_to_char("You rise a level!\r\n", ch);
			else
			{
				sprintf(buf, "You rise %d levels!\r\n", num_levels);
				send_to_char(buf, ch);
			}
			set_title(ch, NULL);
			check_autowiz(ch);
		}
	}
	else if (gain < 0)
	{
		gain = MAX(-max_exp_loss, gain);	/* Cap max exp lost per death */
		GET_EXP(ch) += gain;
		if (GET_EXP(ch) < 0)
			GET_EXP(ch) = 0;
	}
}


void gain_exp_regardless(CHAR_DATA *ch, int gain)
{
	int is_altered = FALSE;
	int num_levels = 0;
	
	GET_EXP(ch) += gain;
	if (GET_EXP(ch) < 0)
		GET_EXP(ch) = 0;
	
	if (!IS_NPC(ch))
	{
		while (GET_LEVEL(ch) < LVL_IMPL &&
			GET_EXP(ch) >= tot_exp_to_level(GET_LEVEL(ch) + 1))
		{
			GET_LEVEL(ch) += 1;
			GET_TOT_LEVEL(ch) += 1;
			num_levels++;
			advance_level(ch);
			is_altered = TRUE;
		}
		
		if (is_altered)
		{
			sprintf(buf, "%s advanced %d level%s to level %d.",
				GET_NAME(ch), num_levels, num_levels == 1 ? "" : "s",
				GET_LEVEL(ch));
			mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
			if (num_levels == 1)
				send_to_char("You rise a level!\r\n", ch);
			else
			{
				sprintf(buf, "You rise %d levels!\r\n", num_levels);
				send_to_char(buf, ch);
			}
			set_title(ch, NULL);
			check_autowiz(ch);
		}
	}
}


void gain_condition(CHAR_DATA *ch, int condition, int value)
{
  bool intoxicated;

  if (IS_NPC(ch) || GET_COND(ch, condition) == -1)	/* No change */
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition) {
  case FULL:
    send_to_char("You are hungry.\r\n", ch);
    return;
  case THIRST:
    send_to_char("You are thirsty.\r\n", ch);
    return;
  case DRUNK:
    if (intoxicated)
      send_to_char("You are now sober.\r\n", ch);
    return;
  default:
    break;
  }

}


void check_idling(CHAR_DATA *ch)
{
	if (++(ch->player.timer) > idle_void)
	{
		if (GET_WAS_IN(ch) == NULL && ch->in_room != NULL)
		{
			GET_WAS_IN(ch) = ch->in_room;
			if (FIGHTING(ch)) 
			{
				stop_fighting(FIGHTING(ch));
				stop_fighting(ch);
			}
			act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
			send_to_char("You have been idle, and are pulled into a void.\r\n", ch);
			save_char(ch, NULL);
			SaveCharObj(ch, FALSE);
			char_from_room(ch);
			char_to_room(ch, get_room(1));
		}
		else if (ch->player.timer > idle_rent_time)
		{
			if (ch->in_room != NULL)
				char_from_room(ch);
			char_to_room(ch, get_room(3));
			if (ch->desc)
			{
				STATE(ch->desc) = CON_DISCONNECT;
				/*
				 * For the 'if (d->character)' test in close_socket().
				 * -gg 3/1/98 (Happy anniversary.)
				 */
				ch->desc->character = NULL;
				ch->desc = NULL;
			}
			save_char(ch, NULL);
			SaveCharObj(ch, TRUE);
			sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
			mudlog(buf, CMP, LVL_GOD, TRUE);
			extract_char(ch);
		}
	}
}



/* Update PCs, NPCs, and objects */
void point_update(void)
{
	CHAR_DATA *i, *next_char;
	OBJ_DATA *j, *next_thing, *jj, *next_thing2;
	
	/* characters */
	for (i = character_list; i; i = next_char)
	{
		next_char = i->next;
		
		gain_condition(i, FULL, -1);
		gain_condition(i, DRUNK, -1);
		gain_condition(i, THIRST, -1);
		
		if (GET_POS(i) >= POS_STUNNED)
		{
			GET_HIT(i)	= MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
			GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
			GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));

			if (AFF_FLAGGED(i, AFF_POISON))
			{
				if (damage(i, i, 2, SPELL_POISON) == -1)
					continue;	/* Oops, they died. -gg 6/24/98 */
			}

			if (GET_POS(i) <= POS_STUNNED)
				update_pos(i);
		}
		else if (GET_POS(i) == POS_INCAP)
		{
			if (damage(i, i, 1, TYPE_SUFFERING) == -1)
				continue;
		}
		else if (GET_POS(i) == POS_MORTALLYW)
		{
			if (damage(i, i, 2, TYPE_SUFFERING) == -1)
				continue;
		}
		if (!IS_NPC(i))
		{
			update_char_objects(i);
			if (GET_LEVEL(i) < idle_max_level)
				check_idling(i);
		}
	}
	
	/* objects */
	for (j = first_object; j; j = next_thing)
	{
		next_thing = j->next;	/* Next in object list */
		
		/* If this is a corpse */
		if (IS_CORPSE(j))
		{
			/* timer count down */
			if (GET_OBJ_TIMER(j) > 0)
				GET_OBJ_TIMER(j)--;
			
			if (!GET_OBJ_TIMER(j))
			{
				if (j->carried_by)
					act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
				else if ((j->in_room != NULL) && (j->in_room->people))
				{
					act("A quivering horde of maggots consumes $p.",
						TRUE, j->in_room->people, j, 0, TO_ROOM);
					act("A quivering horde of maggots consumes $p.",
						TRUE, j->in_room->people, j, 0, TO_CHAR);
				}
				for (jj = j->first_content; jj; jj = next_thing2)
				{
					next_thing2 = jj->next_content;	/* Next in inventory */
					obj_from_obj(jj);
					
					if (j->in_obj)
						jj = obj_to_obj(jj, j->in_obj);
					else if (j->carried_by)
						jj = obj_to_room(jj, j->carried_by->in_room);
					else if (j->in_room)
						jj = obj_to_room(jj, j->in_room);
					else
						core_dump();
				}
				extract_obj(j);
			}
		}

		if (GET_OBJ_TYPE(j) == ITEM_GOODS && GET_OBJ_TIMER(j) != -1)
		{
			// TODO - handle goods deterioration...
		}
		
		if ( get_real_obj_weight(j) > 0 && (j->in_room != NULL) && water_sector(SECT(j->in_room)) )
		{
			if (j->in_room->people)
			{
				act("$p sinks into the watery depths.", FALSE, j->in_room->people, j, 0, TO_ROOM);
				act("$p sinks into the watery depths.", FALSE, j->in_room->people, j, 0, TO_CHAR);
			}
			extract_obj(j);
		}
	}
}
