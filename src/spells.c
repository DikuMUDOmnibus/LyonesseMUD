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
*   File: spells.c                                      Part of CircleMUD *
*  Usage: Implementation of "manual spells".  Circle 2.2 spell compat.    *
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
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"


/* external variables */
extern int				mini_mud;

/* external functions */
ACMD(do_flee);
void clearMemory(CHAR_DATA *ch);
void weight_change_object(OBJ_DATA *obj, int weight);
void add_follower(CHAR_DATA *ch, CHAR_DATA *leader);
int mag_savingthrow(CHAR_DATA *ch, int type, int modifier);
void name_to_drinkcon(OBJ_DATA *obj, int type);
void name_from_drinkcon(OBJ_DATA *obj);
int compute_armor_class(CHAR_DATA *ch);

/*
 * Special spells appear below.
 */

ASPELL(spell_create_water)
{
	int water;
	
	if (ch == NULL || obj == NULL)
		return;
	/* level = MAX(MIN(level, LVL_IMPL), 1);	 - not used */
	
	if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON)
	{
		if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0))
		{
			name_from_drinkcon(obj);
			GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
			name_to_drinkcon(obj, LIQ_SLIME);
		}
		else
		{
			water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
			if (water > 0)
			{
				if (GET_OBJ_VAL(obj, 1) >= 0)
					name_from_drinkcon(obj);
				GET_OBJ_VAL(obj, 2) = LIQ_WATER;
				GET_OBJ_VAL(obj, 1) += water;
				name_to_drinkcon(obj, LIQ_WATER);
				weight_change_object(obj, water);
				act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
			}
		}
	}
}


ASPELL(spell_recall)
{
	if (victim == NULL || IS_NPC(victim))
		return;

	if (PLR_FLAGGED(victim, PLR_GHOST))
	{
		send_to_char("Ghosts doesn't recall.\r\n", ch);
		return;
	}

	act("$n disappears.", TRUE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, r_mortal_start_room);
	act("$n appears in the middle of the room.", TRUE, victim, 0, 0, TO_ROOM);
	look_at_room(victim, 0);
}


ASPELL(spell_teleport)
{
	ROOM_DATA *to_room;
	
	if (victim == NULL || IS_NPC(victim))
		return;

	if (PLR_FLAGGED(victim, PLR_GHOST))
	{
		send_to_char("Ghosts cannot be teleported.\r\n", ch);
		return;
	}
	
	do
	{
		to_room = get_room(number(0, top_of_world));
	} while (!to_room || ROOM_FLAGGED(to_room, ROOM_PRIVATE | ROOM_DEATH | ROOM_GODROOM));
	
	act("$n slowly fades out of existence and is gone.", FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, to_room);
	act("$n slowly fades into existence.", FALSE, victim, 0, 0, TO_ROOM);
	look_at_room(victim, 0);
}

#define SUMMON_FAIL "You failed.\r\n"

ASPELL(spell_summon)
{
	if (ch == NULL || victim == NULL)
		return;
	
	if ( !is_char_known(ch, victim) )
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}
	if (GET_LEVEL(victim) > MIN(LVL_IMMORT - 1, level + 3))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}
	
	if (PLR_FLAGGED(victim, PLR_GHOST))
	{
		send_to_char("Ghosts cannot be summoned.\r\n", ch);
		return;
	}

	if (!pkill_ok(ch, victim))
	{
		if (MOB_FLAGGED(victim, MOB_AGGRESSIVE))
		{
			act("As the words escape your lips and $N travels\r\n"
				"through time and space towards you, you realize that $E is\r\n"
				"aggressive and might harm you, so you wisely send $M back.",
				FALSE, ch, 0, victim, TO_CHAR);
			return;
		}
		if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE) &&
			!PLR_FLAGGED(victim, PLR_KILLER))
		{
			sprintf(buf, "%s just tried to summon you to: %s.\r\n"
				"%s failed because you have summon protection on.\r\n"
				"Type NOSUMMON to allow other players to summon you.\r\n",
				GET_NAME(ch), ROOM_NAME(ch),
				(ch->player.sex == SEX_MALE) ? "He" : "She");
			send_to_char(buf, victim);
			
			sprintf(buf, "You failed because %s has summon protection on.\r\n",
				GET_NAME(victim));
			send_to_char(buf, ch);
			
			sprintf(buf, "%s failed summoning %s to %s.",
				GET_NAME(ch), GET_NAME(victim), ROOM_NAME(ch));
			mudlog(buf, BRF, LVL_IMMORT, TRUE);
			return;
		}
	}
	
	if (MOB_FLAGGED(victim, MOB_NOSUMMON) ||
		(IS_NPC(victim) && mag_savingthrow(victim, SAVING_SPELL, 0)))
	{
		send_to_char(SUMMON_FAIL, ch);
		return;
	}
	
	act("$n disappears suddenly.", TRUE, victim, 0, 0, TO_ROOM);
	
	char_from_room(victim);
	char_to_room(victim, ch->in_room);
	
	act("$n arrives suddenly.", TRUE, victim, 0, 0, TO_ROOM);
	act("$n has summoned you!", FALSE, ch, 0, victim, TO_VICT);
	look_at_room(victim, 0);
}



ASPELL(spell_locate_object)
{
	OBJ_DATA *i;
	char name[MAX_INPUT_LENGTH];
	int j;
	
	/*
	 * FIXME: This is broken.  The spell parser routines took the argument
	 * the player gave to the spell and located an object with that keyword.
	 * Since we're passed the object and not the keyword we can only guess
	 * at what the player originally meant to search for. -gg
	 */
	strcpy(name, fname(obj->name));
	j = level / 2;
	
	for (i = first_object; i && (j > 0); i = i->next)
	{
		if (!isname(name, i->name))
			continue;
		
		if (i->carried_by)
			sprintf(buf, "%s is being carried by %s.\r\n",
			i->short_description, PERS(i->carried_by, ch));
		else if (i->in_room != NULL)
			sprintf(buf, "%s is in %s.\r\n", i->short_description, ROOM_NAME(i));
		else if (i->in_obj)
			sprintf(buf, "%s is in %s.\r\n", i->short_description,
			i->in_obj->short_description);
		else if (i->worn_by)
			sprintf(buf, "%s is being worn by %s.\r\n",
			i->short_description, PERS(i->worn_by, ch));
		else
			sprintf(buf, "%s's location is uncertain.\r\n",
			i->short_description);
		
		CAP(buf);
		send_to_char(buf, ch);
		j--;
	}
	
	if (j == level / 2)
		send_to_char("You sense nothing.\r\n", ch);
}



ASPELL(spell_charm)
{
	AFFECTED_DATA af;
	
	if (victim == NULL || ch == NULL)
		return;

	if (victim == ch)
		send_to_char("You like yourself even better!\r\n", ch);
	else if (!IS_NPC(victim) && !PRF_FLAGGED(victim, PRF_SUMMONABLE))
		send_to_char("You fail because SUMMON protection is on!\r\n", ch);
	else if (PLR_FLAGGED(victim, PLR_GHOST))
		send_to_char("Ghosts cannot be charmed.\r\n", ch);
	else if (AFF_FLAGGED(victim, AFF_SANCTUARY))
		send_to_char("Your victim is protected by sanctuary!\r\n", ch);
	else if (MOB_FLAGGED(victim, MOB_NOCHARM))
		send_to_char("Your victim resists!\r\n", ch);
	else if (AFF_FLAGGED(ch, AFF_CHARM))
		send_to_char("You can't have any followers of your own!\r\n", ch);
	else if (AFF_FLAGGED(victim, AFF_CHARM) || level < GET_LEVEL(victim))
		send_to_char("You fail.\r\n", ch);
	/* player charming another player - no legal reason for this */
	else if (!pkill_ok(ch, victim))
		send_to_char("You fail - shouldn't be doing it anyway.\r\n", ch);
	else if (circle_follow(victim, ch))
		send_to_char("Sorry, following in circles can not be allowed.\r\n", ch);
	else if (mag_savingthrow(victim, SAVING_PARA, 0))
		send_to_char("Your victim resists!\r\n", ch);
	else
	{
		if (victim->master)
			stop_follower(victim);
		
		add_follower(victim, ch);
		
		af.type = SPELL_CHARM;
		af.duration = 24 * 2;
		if (GET_CHA(ch))
			af.duration *= GET_CHA(ch);	
		af.duration /= GET_INT(victim);
		
		af.modifier = 0;
		af.location = 0;
		af.bitvector = AFF_CHARM;
		affect_to_char(victim, &af);
		
		act("Isn't $n just such a nice fellow?", FALSE, ch, 0, victim, TO_VICT);
		if (IS_NPC(victim))
		{
			//REMOVE_BIT(MOB_FLAGS(victim), MOB_AGGRESSIVE);
			REMOVE_BIT(MOB_FLAGS(victim), MOB_SPEC);
		}
	}
}



ASPELL(spell_identify)
{
	int i;
	int found;
	
	if (obj)
	{
		send_to_char("You feel informed:\r\n", ch);
		sprintf(buf, "Object '%s', Item type: ", obj->short_description);
		sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
		strcat(buf, buf2);
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
		
		if (obj->obj_flags.bitvector)
		{
			send_to_char("Item will give you following abilities:  ", ch);
			sprintbit(obj->obj_flags.bitvector, affected_bits, buf);
			strcat(buf, "\r\n");
			send_to_char(buf, ch);
		}
		send_to_char("Item is: ", ch);
		sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
		strcat(buf, "\r\n");
		send_to_char(buf, ch);
		
		sprintf(buf, "Weight: %d, Value: %d, Rent: %d\r\n",
			GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj));
		send_to_char(buf, ch);
		
		switch (GET_OBJ_TYPE(obj))
		{
		case ITEM_SCROLL:
		case ITEM_POTION:
			sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);
			
			if (GET_OBJ_VAL(obj, 1) >= 1)
				sprintf(buf + strlen(buf), " %s", skill_name(GET_OBJ_VAL(obj, 1)));
			if (GET_OBJ_VAL(obj, 2) >= 1)
				sprintf(buf + strlen(buf), " %s", skill_name(GET_OBJ_VAL(obj, 2)));
			if (GET_OBJ_VAL(obj, 3) >= 1)
				sprintf(buf + strlen(buf), " %s", skill_name(GET_OBJ_VAL(obj, 3)));
			strcat(buf, "\r\n");
			send_to_char(buf, ch);
			break;
		case ITEM_WAND:
		case ITEM_STAFF:
			sprintf(buf, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);
			sprintf(buf + strlen(buf), " %s\r\n", skill_name(GET_OBJ_VAL(obj, 3)));
			sprintf(buf + strlen(buf), "It has %d maximum charge%s and %d remaining.\r\n",
				GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s",
				GET_OBJ_VAL(obj, 2));
			send_to_char(buf, ch);
			break;
		case ITEM_WEAPON:
			sprintf(buf, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1),
				GET_OBJ_VAL(obj, 2));
			sprintf(buf + strlen(buf), " for an average per-round damage of %.1f.\r\n",
				(((GET_OBJ_VAL(obj, 2) + 1) / 2.0) * GET_OBJ_VAL(obj, 1)));
			send_to_char(buf, ch);
			break;
		case ITEM_ARMOR:
			sprintf(buf, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
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
					send_to_char("Can affect you as :\r\n", ch);
					found = TRUE;
				}
				sprinttype(obj->affected[i].location, apply_types, buf2);
				sprintf(buf, "   Affects: %s By %d\r\n", buf2, obj->affected[i].modifier);
				send_to_char(buf, ch);
			}
		}
	}
	else if (victim)		/* victim */
	{
		sprintf(buf, "Name: %s\r\n", GET_NAME(victim));
		send_to_char(buf, ch);
		if (!IS_NPC(victim))
		{
			sprintf(buf, "%s is %d years, %d months, %d days and %d hours old.\r\n",
				GET_NAME(victim), age(victim)->year, age(victim)->month,
				age(victim)->day, age(victim)->hours);
			send_to_char(buf, ch);
		}
		sprintf(buf, "Height %d cm, Weight %d pounds\r\n",
			GET_HEIGHT(victim), GET_WEIGHT(victim));
		sprintf(buf + strlen(buf), "Level: %d, Hits: %d, Mana: %d\r\n",
			GET_LEVEL(victim), GET_HIT(victim), GET_MANA(victim));
		sprintf(buf + strlen(buf), "AC: %d, Hitroll: %d, Damroll: %d\r\n",
			compute_armor_class(victim), GET_HITROLL(victim), GET_DAMROLL(victim));
		sprintf(buf + strlen(buf), "Str: %d, Int: %d, Wis: %d, Dex: %d, Con: %d, Cha: %d\r\n",
			GET_STR(victim), GET_INT(victim), GET_WIS(victim),
			GET_DEX(victim), GET_CON(victim), GET_CHA(victim));
		send_to_char(buf, ch);
		
	}
}



/*
 * Cannot use this spell on an equipped object or it will mess up the
 * wielding character's hit/dam totals.
 */
ASPELL(spell_enchant_weapon)
{
	int i;
	
	if (ch == NULL || obj == NULL)
		return;
	
	/* Either already enchanted or not a weapon. */
	if (GET_OBJ_TYPE(obj) != ITEM_WEAPON || OBJ_FLAGGED(obj, ITEM_MAGIC))
		return;
	
	/* Make sure no other affections. */
	for (i = 0; i < MAX_OBJ_AFF; i++)
	{
		if (obj->affected[i].location != APPLY_NONE)
			return;
	}
	
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	
	obj->affected[0].location = APPLY_HITROLL;
	obj->affected[0].modifier = 1 + (level >= 18);
	
	obj->affected[1].location = APPLY_DAMROLL;
	obj->affected[1].modifier = 1 + (level >= 20);
	
	if (IS_GOOD(ch))
	{
		SET_BIT(GET_OBJ_ANTI(obj), ITEM_ANTI_EVIL);
		act("$p glows blue.", FALSE, ch, obj, 0, TO_CHAR);
	}
	else if (IS_EVIL(ch))
	{
		SET_BIT(GET_OBJ_ANTI(obj), ITEM_ANTI_GOOD);
		act("$p glows red.", FALSE, ch, obj, 0, TO_CHAR);
	}
	else
		act("$p glows yellow.", FALSE, ch, obj, 0, TO_CHAR);
}


ASPELL(spell_detect_poison)
{
	if (victim)
	{
		if (victim == ch)
		{
			if (AFF_FLAGGED(victim, AFF_POISON))
				send_to_char("You can sense poison in your blood.\r\n", ch);
			else
				send_to_char("You feel healthy.\r\n", ch);
		}
		else
		{
			if (AFF_FLAGGED(victim, AFF_POISON))
				act("You sense that $E is poisoned.", FALSE, ch, 0, victim, TO_CHAR);
			else
				act("You sense that $E is healthy.", FALSE, ch, 0, victim, TO_CHAR);
		}
	}
	
	if (obj)
	{
		switch (GET_OBJ_TYPE(obj))
		{
		case ITEM_DRINKCON:
		case ITEM_FOUNTAIN:
		case ITEM_FOOD:
			if (GET_OBJ_VAL(obj, 3))
				act("You sense that $p has been contaminated.",FALSE,ch,obj,0,TO_CHAR);
			else
				act("You sense that $p is safe for consumption.", FALSE, ch, obj, 0,
				TO_CHAR);
			break;
		default:
			send_to_char("You sense that it should not be consumed.\r\n", ch);
		}
	}
}

ASPELL(spell_fear)
{
	CHAR_DATA *target, *next_target;
	int rooms_to_flee = 0;
	
	if (ch == NULL)
		return;
	
	send_to_char("You radiate an aura of fear into the room!\r\n", ch);
	act("$n is surrounded by an aura of fear!", TRUE, ch, 0, 0, TO_ROOM);
	
	for (target = ch->in_room->people; target; target = next_target)
	{
		next_target = target->next_in_room;
		
		if (target == NULL)
			return;
		
		if (target == ch)
			continue;
		
		if (IS_IMMORTAL(target))
			continue;
		
		if (mag_savingthrow(target, SAVING_SPELL, 0))
		{
			sprintf(buf, "%s is unaffected by the fear!\r\n", GET_NAME(target));
			act(buf, TRUE, ch, 0, 0, TO_ROOM);
			send_to_char("Your victim is not afraid of the likes of you!\r\n", ch);

			if (IS_NPC(target))
				hit(target, ch, TYPE_UNDEFINED);
		}
		else
		{
			for (rooms_to_flee = level / 10; rooms_to_flee > 0; rooms_to_flee--)
			{
				send_to_char("You flee in terror!\r\n", target);
				do_flee(target, "", 0, 0);
			}
		}
	}
}

