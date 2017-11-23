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
*   File: magic.c                                       Part of CircleMUD *
*  Usage: low-level functions for magic; spell template code              *
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
#include "interpreter.h"
#include "constants.h"

/* external variables */
extern SPELL_INFO_DATA	spell_info[];
extern int				mini_mud;

/* external functions */
byte	saving_throws(int class_num, int type, int level); /* class.c */
void	clearMemory(CHAR_DATA *ch);
void	weight_change_object(OBJ_DATA *obj, int weight);
void	add_follower(CHAR_DATA *ch, CHAR_DATA *leader);
void	wild_check_for_remove( ROOM_DATA *wRoom );
void	stop_memorizing(CHAR_DATA *ch);

/* local functions */
int mag_materials(CHAR_DATA *ch, int item0, int item1, int item2, int extract, int verbose);
int mag_savingthrow(CHAR_DATA *ch, int type, int modifier);
void perform_mag_groups(int level, CHAR_DATA *ch, CHAR_DATA *tch, int spellnum, int savetype);

/*
 * Negative apply_saving_throw[] values make saving throws better!
 * Then, so do negative modifiers.  Though people may be used to
 * the reverse of that. It's due to the code modifying the target
 * saving throw instead of the random number of the character as
 * in some other systems.
 */
int mag_savingthrow(CHAR_DATA *ch, int type, int modifier)
{
	/* NPCs use warrior tables according to some book */
	int class_sav = CLASS_WARRIOR;
	int save;
	
	if (!IS_NPC(ch))
		class_sav = GET_CLASS(ch);
	
	save = saving_throws(class_sav, type, GET_LEVEL(ch));
	save += GET_SAVE(ch, type);
	save += modifier;
	
	/* Throwing a 0 is always a failure. */
	if (MAX(1, save) < number(0, 99))
		return (TRUE);
	
	/* Oops, failed. Sorry. */
	return (FALSE);
}


/*
 * mag_objectmagic: This is the entry-point for all magic items.  This should
 * only be called by the 'quaff', 'use', 'recite', etc. routines.
 *
 * For reference, object values 0-3:
 * staff  - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * wand   - [0]	level	[1] max charges	[2] num charges	[3] spell num
 * scroll - [0]	level	[1] spell num	[2] spell num	[3] spell num
 * potion - [0] level	[1] spell num	[2] spell num	[3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified;
 * the DikuMUD format did not specify staff and wand levels in the world
 * files (this is a CircleMUD enhancement).
 */
void mag_objectmagic(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	CHAR_DATA *tch = NULL, *next_tch;
	OBJ_DATA *tobj = NULL;
	int i, k;
	
	one_argument(argument, arg);
	
	k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM |
		FIND_OBJ_EQUIP, ch, &tch, &tobj);
	
	switch (GET_OBJ_TYPE(obj))
	{
	case ITEM_STAFF:
		act("You tap $p three times on the ground.", FALSE, ch, obj, 0, TO_CHAR);
		if (obj->action_description)
			act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
		else
			act("$n taps $p three times on the ground.", FALSE, ch, obj, 0, TO_ROOM);
		
		if (GET_OBJ_VAL(obj, 2) <= 0)
		{
			send_to_char("It seems powerless.\r\n", ch);
			act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
		}
		else
		{
			GET_OBJ_VAL(obj, 2)--;
			WAIT_STATE(ch, PULSE_VIOLENCE);
			/* Level to cast spell at. */
			k = GET_OBJ_VAL(obj, 0) ? GET_OBJ_VAL(obj, 0) : DEFAULT_STAFF_LVL;
			
			/*
			 * Problem : Area/mass spells on staves can cause crashes.
			 * Solution: Remove the special nature of area/mass spells on staves.
			 * Problem : People like that behavior.
			 * Solution: We special case the area/mass spells here.
			 */
			if (HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, 3), MAG_MASSES | MAG_AREAS))
			{
				for (i = 0, tch = ch->in_room->people; tch; tch = tch->next_in_room)
					i++;
				while (i-- > 0)
					call_magic(ch, NULL, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
			}
			else
			{
				for (tch = ch->in_room->people; tch; tch = next_tch)
				{
					next_tch = tch->next_in_room;
					if (ch != tch)
						call_magic(ch, tch, NULL, GET_OBJ_VAL(obj, 3), k, CAST_STAFF);
				}
			}
		}
		break;

	case ITEM_WAND:
		if (k == FIND_CHAR_ROOM)
		{
			if (tch == ch)
			{
				act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
			}
			else
			{
				act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
				if (obj->action_description)
					act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
				else
					act("$n points $p at $N.", TRUE, ch, obj, tch, TO_ROOM);
			}
		}
		else if (tobj != NULL)
		{
			act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
			if (obj->action_description)
				act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
			else
				act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
		}
		else if (IS_SET(spell_info[GET_OBJ_VAL(obj, 3)].routines, MAG_AREAS | MAG_MASSES))
		{
			/* Wands with area spells don't need to be pointed. */
			act("You point $p outward.", FALSE, ch, obj, NULL, TO_CHAR);
			act("$n points $p outward.", TRUE, ch, obj, NULL, TO_ROOM);
		}
		else
		{
			act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
			return;
		}
		
		if (GET_OBJ_VAL(obj, 2) <= 0)
		{
			send_to_char("It seems powerless.\r\n", ch);
			act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
			return;
		}
		GET_OBJ_VAL(obj, 2)--;
		WAIT_STATE(ch, PULSE_VIOLENCE);
		if (GET_OBJ_VAL(obj, 0))
			call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 0), CAST_WAND);
		else
			call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, 3), DEFAULT_WAND_LVL, CAST_WAND);
		break;

	case ITEM_SCROLL:
		if (*arg)
		{
			if (!k)
			{
				act("There is nothing to here to affect with $p.", FALSE,
					ch, obj, NULL, TO_CHAR);
				return;
			}
		}
		else
			tch = ch;
		
		act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
		if (obj->action_description)
			act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
		else
			act("$n recites $p.", FALSE, ch, obj, NULL, TO_ROOM);
		
		WAIT_STATE(ch, PULSE_VIOLENCE);
		for (i = 1; i <= 3; i++)
			if (call_magic(ch, tch, tobj, GET_OBJ_VAL(obj, i), GET_OBJ_VAL(obj, 0), CAST_SCROLL) <= 0)
				break;
			
		if (obj != NULL)
			extract_obj(obj);
		break;

	case ITEM_POTION:
		tch = ch;
		act("You quaff $p.", FALSE, ch, obj, NULL, TO_CHAR);
		if (obj->action_description)
			act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
		else
			act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);
		
		WAIT_STATE(ch, PULSE_VIOLENCE);
		for (i = 1; i <= 3; i++)
			if (call_magic(ch, ch, NULL, GET_OBJ_VAL(obj, i), GET_OBJ_VAL(obj, 0), CAST_POTION) <= 0)
				break;
			
		if (obj != NULL)
			extract_obj(obj);
		break;
	default:
		log("SYSERR: Unknown object_type %d in mag_objectmagic.",
			GET_OBJ_TYPE(obj));
		break;
	}
}


ACMD(do_use)
{
	OBJ_DATA *mag_item;
	
	half_chop(argument, arg, buf);
	if (!*arg)
	{
		sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
		send_to_char(buf2, ch);
		return;
	}
	mag_item = GET_EQ(ch, WEAR_HOLD);
	
	if (!mag_item || !isname(arg, mag_item->name))
	{
		switch (subcmd)
		{
		case SCMD_RECITE:
		case SCMD_QUAFF:
			if (!(mag_item = get_obj_in_list_vis_rev(ch, arg, NULL, ch->last_carrying)))
			{
				sprintf(buf2, "You don't seem to have %s %s.\r\n", AN(arg), arg);
				send_to_char(buf2, ch);
				return;
			}
			break;
		case SCMD_USE:
			sprintf(buf2, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
			send_to_char(buf2, ch);
			return;
		default:
			log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
			return;
		}
	}
	switch (subcmd)
	{
	case SCMD_QUAFF:
		if (GET_OBJ_TYPE(mag_item) != ITEM_POTION)
		{
			send_to_char("You can only quaff potions.\r\n", ch);
			return;
		}
		break;
	case SCMD_RECITE:
		if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL)
		{
			send_to_char("You can only recite scrolls.\r\n", ch);
			return;
		}
		break;
	case SCMD_USE:
		if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
			(GET_OBJ_TYPE(mag_item) != ITEM_STAFF))
		{
			send_to_char("You can't seem to figure out how to use it.\r\n", ch);
			return;
		}
		break;
	}
	
	mag_objectmagic(ch, mag_item, buf);
}


/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle 3.0 use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
int mag_materials(CHAR_DATA *ch, int item0, int item1, int item2, int extract, int verbose)
{
	OBJ_DATA *tobj;
	OBJ_DATA *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;
	
	for (tobj = ch->first_carrying; tobj; tobj = tobj->next_content)
	{
		if ((item0 > 0) && (GET_OBJ_VNUM(tobj) == item0))
		{
			obj0 = tobj;
			item0 = -1;
		}
		else if ((item1 > 0) && (GET_OBJ_VNUM(tobj) == item1))
		{
			obj1 = tobj;
			item1 = -1;
		}
		else if ((item2 > 0) && (GET_OBJ_VNUM(tobj) == item2))
		{
			obj2 = tobj;
			item2 = -1;
		}
	}

	if ((item0 > 0) || (item1 > 0) || (item2 > 0))
	{
		if (verbose)
		{
			switch (number(0, 2))
			{
			case 0:
				send_to_char("A wart sprouts on your nose.\r\n", ch);
				break;
			case 1:
				send_to_char("Your hair falls out in clumps.\r\n", ch);
				break;
			case 2:
				send_to_char("A huge corn develops on your big toe.\r\n", ch);
				break;
			}
		}
		return (FALSE);
	}
	
	if (extract)
	{
		if (item0 < 0)	extract_obj(obj0);
		if (item1 < 0)	extract_obj(obj1);
		if (item2 < 0)	extract_obj(obj2);
	}

	if (verbose)
	{
		send_to_char("A puff of smoke rises from your pack.\r\n", ch);
		act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
	}

	return (TRUE);
}




/*
 * Every spell that does damage comes through here.  This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * -1 = dead, otherwise the amount of damage done.
 */
int mag_damage(int level, CHAR_DATA *ch, CHAR_DATA *victim, int spellnum, int savetype)
{
	int dam = 0;
	
	if (ch == NULL || victim == NULL)
		return (0);
	
	switch (spellnum)
	{
	/* Mostly mages */
	case SPELL_ACID_ARROW:
		if (IS_MAGIC_USER(ch) || IS_SORCERER(ch))
		{
			int ndice, counter;

			ndice = 1 + ((GET_LEVEL(ch) / 5) * 2);
			dam = 0;
			for (counter = 0; counter < ndice; counter++)
				dam += dice(1, 4);
		}
		else
			dam = dice(1, 6) + 1;
		break;

	case SPELL_MAGIC_MISSILE:
	case SPELL_CHILL_TOUCH:			/* chill touch also has an affect */
		if (IS_MAGIC_USER(ch) || IS_SORCERER(ch))
			dam = dice(1, 8) + 1;
		else
			dam = dice(1, 6) + 1;
		break;
	case SPELL_BURNING_HANDS:
		if (IS_MAGIC_USER(ch) || IS_SORCERER(ch))
			dam = dice(3, 8) + 3;
		else
			dam = dice(3, 6) + 3;
		break;
	case SPELL_SHOCKING_GRASP:
		if (IS_MAGIC_USER(ch) || IS_SORCERER(ch))
			dam = dice(5, 8) + 5;
		else
			dam = dice(5, 6) + 5;
		break;
	case SPELL_LIGHTNING_BOLT:
		if (IS_MAGIC_USER(ch) || IS_SORCERER(ch))
			dam = dice(7, 8) + 7;
		else
			dam = dice(7, 6) + 7;
		break;
	case SPELL_COLOR_SPRAY:
		if (IS_MAGIC_USER(ch) || IS_SORCERER(ch))
			dam = dice(9, 8) + 9;
		else
			dam = dice(9, 6) + 9;
		break;
	case SPELL_FIREBALL:
		if (IS_MAGIC_USER(ch) || IS_SORCERER(ch))
			dam = dice(11, 8) + 11;
		else
			dam = dice(11, 6) + 11;
		break;
		
	/* Mostly clerics */
	case SPELL_DISPEL_EVIL:
		dam = dice(6, 8) + 6;
		if (IS_EVIL(ch))
		{
			victim = ch;
			dam = GET_HIT(ch) - 1;
		}
		else if (IS_GOOD(victim))
		{
			act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
			return (0);
		}
		break;
	case SPELL_DISPEL_GOOD:
		dam = dice(6, 8) + 6;
		if (IS_GOOD(ch))
		{
			victim = ch;
			dam = GET_HIT(ch) - 1;
		}
		else if (IS_EVIL(victim))
		{
			act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
			return (0);
		}
		break;
		
		
	case SPELL_CALL_LIGHTNING:
		dam = dice(7, 8) + 7;
		break;
		
	case SPELL_HARM:
		dam = dice(8, 8) + 8;
		break;
		
	case SPELL_ENERGY_DRAIN:
		if (GET_LEVEL(victim) <= 2)
			dam = 100;
		else
			dam = dice(1, 10);
		break;

	case SPELL_SHOCKWAVE:
		if (IS_MAGIC_USER(ch) || IS_SORCERER(ch))
			dam = dice(MAX(level, 20), 8);
		else
			dam = dice(MAX(level, 18), 6);

		if (SECT(ch->in_room) == SECT_UNDERWATER)
			dam += dam / 2;
		break;
		
	/* Area spells */
	case SPELL_EARTHQUAKE:
		dam = dice(2, 8) + level;
		break;

	case SPELL_INCENDIARY_CLOUD:
		if (IS_MAGIC_USER(ch) || IS_SORCERER(ch))
			dam = dice(MAX(level, 20), 10);
		else
			dam = dice(MAX(level, 18), 8);
		break;
	}
	
	/* divide damage by two if victim makes his saving throw */
	if (mag_savingthrow(victim, savetype, 0))
		dam /= 2;
	
	/* let's player know how much damage he produces */
	if (!IS_NPC(ch))
		ch_printf(ch, "&b&5Your spell cause %d points of damage.&0\r\n", dam );

	/* and finally, inflict the damage */
	return (damage(ch, victim, dam, spellnum));
}


/*
 * Every spell that does an affect comes through here.  This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
 */

#define MAX_SPELL_AFFECTS 5	/* change if more needed */

void mag_affects(int level, CHAR_DATA *ch, CHAR_DATA *victim, int spellnum, int savetype)
{
	AFFECTED_DATA af[MAX_SPELL_AFFECTS];
	bool accum_affect = FALSE, accum_duration = FALSE;
	const char *to_vict = NULL, *to_room = NULL;
	int i;

	if (victim == NULL || ch == NULL)
		return;

	for (i = 0; i < MAX_SPELL_AFFECTS; i++)
	{
		af[i].type	= spellnum;
		af[i].bitvector = 0;
		af[i].modifier	= 0;
		af[i].location	= APPLY_NONE;
	}
	
	switch (spellnum)
	{	
	case SPELL_CHILL_TOUCH:
		af[0].location = APPLY_STR;
		if (mag_savingthrow(victim, savetype, 0))
			af[0].duration = 1;
		else
			af[0].duration = 4;
		af[0].modifier = -1;
		accum_duration = TRUE;
		to_vict = "You feel your strength wither!";
		break;
		
	case SPELL_ARMOR:
		af[0].location = APPLY_AC;
		af[0].modifier = -20;
		af[0].duration = 24;
		accum_duration = TRUE;
		to_vict = "You feel someone protecting you.";
		break;
		
	case SPELL_BLESS:
		af[0].location = APPLY_HITROLL;
		af[0].modifier = 2;
		af[0].duration = 6;
		
		af[1].location = APPLY_SAVING_SPELL;
		af[1].modifier = -1;
		af[1].duration = 6;
		
		accum_duration = TRUE;
		to_vict = "You feel righteous.";
		break;
		
	case SPELL_FLASH:
	case SPELL_BLINDNESS:
		if (MOB_FLAGGED(victim,MOB_NOBLIND) || mag_savingthrow(victim, savetype, 0))
		{
			send_to_char("You fail.\r\n", ch);
			return;
		}
		af[0].location = APPLY_HITROLL;
		af[0].modifier = -4;
		af[0].duration = 2;
		af[0].bitvector = AFF_BLIND;
		
		af[1].location = APPLY_AC;
		af[1].modifier = 40;
		af[1].duration = 2;
		af[1].bitvector = AFF_BLIND;
		
		to_room = "$n seems to be blinded!";
		to_vict = "You have been blinded!";
		break;
		
	case SPELL_CURSE:
		if (mag_savingthrow(victim, savetype, 0))
		{
			send_to_char(NOEFFECT, ch);
			return;
		}
		
		af[0].location = APPLY_HITROLL;
		af[0].duration = 1 + (GET_LEVEL(ch) / 2);
		af[0].modifier = -1;
		af[0].bitvector = AFF_CURSE;
		
		af[1].location = APPLY_DAMROLL;
		af[1].duration = 1 + (GET_LEVEL(ch) / 2);
		af[1].modifier = -1;
		af[1].bitvector = AFF_CURSE;
		
		accum_duration = TRUE;
		accum_affect = TRUE;
		to_room = "$n briefly glows red!";
		to_vict = "You feel very uncomfortable.";
		break;
		
	case SPELL_DETECT_ALIGN:
		af[0].duration = 12 + level;
		af[0].bitvector = AFF_DETECT_ALIGN;
		accum_duration = TRUE;
		to_vict = "Your eyes tingle.";
		break;
		
	case SPELL_DETECT_INVIS:
		af[0].duration = 12 + level;
		af[0].bitvector = AFF_DETECT_INVIS;
		accum_duration = TRUE;
		to_vict = "Your eyes tingle.";
		break;
		
	case SPELL_DETECT_MAGIC:
		af[0].duration = 12 + level;
		af[0].bitvector = AFF_DETECT_MAGIC;
		accum_duration = TRUE;
		to_vict = "Your eyes tingle.";
		break;
		
	case SPELL_INFRAVISION:
		af[0].duration = 12 + level;
		af[0].bitvector = AFF_INFRAVISION;
		accum_duration = TRUE;
		to_vict = "Your eyes glow red.";
		to_room = "$n's eyes glow red.";
		break;
		
	case SPELL_INVISIBLE:
		if (!victim)
			victim = ch;
		
		af[0].duration = 12 + (GET_LEVEL(ch) / 4);
		af[0].modifier = -40;
		af[0].location = APPLY_AC;
		af[0].bitvector = AFF_INVISIBLE;
		accum_duration = TRUE;
		to_vict = "You vanish.";
		to_room = "$n slowly fades out of existence.";
		break;
		
	case SPELL_POISON:
		if (mag_savingthrow(victim, savetype, 0))
		{
			send_to_char(NOEFFECT, ch);
			return;
		}
		
		af[0].location = APPLY_STR;
		af[0].duration = GET_LEVEL(ch);
		af[0].modifier = -2;
		af[0].bitvector = AFF_POISON;
		to_vict = "You feel very sick.";
		to_room = "$n gets violently ill!";
		break;
		
	case SPELL_PROT_FROM_EVIL:
		af[0].duration = 24;
		af[0].bitvector = AFF_PROTECT_EVIL;
		accum_duration = TRUE;
		to_vict = "You feel invulnerable!";
		break;
		
	case SPELL_SANCTUARY:
		af[0].duration = 4;
		af[0].bitvector = AFF_SANCTUARY;
		accum_duration = TRUE;
		to_vict = "A white aura momentarily surrounds you.";
		to_room = "$n is surrounded by a white aura.";
		break;
		
	case SPELL_SLEEP:
		if (!pkill_ok(ch, victim))
			return;
		if (MOB_FLAGGED(victim, MOB_NOSLEEP))
			return;
		if (mag_savingthrow(victim, savetype, 0))
			return;
		
		af[0].duration = 4 + (GET_LEVEL(ch) / 4);
		af[0].bitvector = AFF_SLEEP;
		
		if (GET_POS(victim) > POS_SLEEPING)
		{
			send_to_char("You feel very sleepy...  Zzzz......\r\n", victim);
			act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
			GET_POS(victim) = POS_SLEEPING;

			/* check for memorizing chars.. */
			if ( !IS_NPC(victim) && AFF_FLAGGED(victim, AFF_MEMORIZING) )
				stop_memorizing(victim);
		}
		break;
		
	case SPELL_STRENGTH:
		af[0].location	= APPLY_STR;
		af[0].duration	= (GET_LEVEL(ch) / 2) + 4;
		af[0].modifier	= 1 + (level > 18);
		accum_duration	= TRUE;
		accum_affect	= TRUE;
		to_vict = "You feel stronger!";
		break;
		
	case SPELL_SENSE_LIFE:
		af[0].duration	= GET_LEVEL(ch);
		af[0].bitvector = AFF_SENSE_LIFE;
		accum_duration	= TRUE;
		to_vict = "Your feel your awareness improve.";
		break;
		
	case SPELL_WATERWALK:
		af[0].duration	= 24;
		af[0].bitvector = AFF_WATERWALK;
		accum_duration	= TRUE;
		to_vict = "You feel webbing between your toes.";
		break;

	case SPELL_MINOR_TRACK:
		af[0].duration	= level;
		to_vict = "You feel your awareness grow!";
		to_room = "$n's eyes take on an emerald hue for just a moment.";
		break;

	case SPELL_MAJOR_TRACK:
		af[0].duration	= level * 2;
		to_vict = "You feel your awareness grow!";
		to_room = "$n's eyes take on an emerald hue for just a moment.";
		break;

	case SPELL_SHIELD:
		af[0].location = APPLY_AC;
		af[0].modifier = -10;
		af[0].duration = 8 + level;
		to_vict = "You are surrounded by a strong force shield.";
		to_room = "$n is surrounded by a strong force shield.";
		break;

	case SPELL_STONE_SKIN:
		af[0].location = APPLY_AC;
		af[0].modifier = -40;
		af[0].duration = level;
		to_vict = "Your skin turns to a stone-like substance.";
		to_room = "$n's skin turns grey and granite-like.";
		break;

	case SPELL_PARALYSIS:
		if (mag_savingthrow(victim, savetype, 0))
		{
			send_to_char(NOEFFECT, ch);
			return;
		}

		af[0].duration  = 4 + level;
		af[0].bitvector = AFF_PARALYSIS;
		to_vict = "Your limbs freeze in place";
		to_room = "$n is paralyzed!";
		
		GET_POS(victim) = POS_STUNNED;
		break;

	case SPELL_FIRESHIELD:
		af[0].duration	= 3 + number(1, 3);
		af[0].bitvector = AFF_FIRESHIELD;
		to_vict = "You start glowing red.";
		to_room = "$n is surrounded by a glowing red aura.";
		break;

	case SPELL_REGENERATION:
		af[0].duration	= 4;
		af[0].bitvector = AFF_REGENERATION;
		to_vict = "You feel full of phisical energy.";
		break;
	}
	
	/*
	 * If this is a mob that has this affect set in its mob file, do not
	 * perform the affect.  This prevents people from un-sancting mobs
	 * by sancting them and waiting for it to fade, for example.
	 */
	if (IS_NPC(victim) && !affected_by_spell(victim, spellnum))
	{
		for (i = 0; i < MAX_SPELL_AFFECTS; i++)
		{
			if (AFF_FLAGGED(victim, af[i].bitvector))
			{
				send_to_char(NOEFFECT, ch);
				return;
			}
		}
	}

	/*
	 * If the victim is already affected by this spell, and the spell does
	 * not have an accumulative effect, then fail the spell.
	 */
	if (affected_by_spell(victim,spellnum) && !(accum_duration||accum_affect))
	{
		send_to_char(NOEFFECT, ch);
		return;
	}
	
	for (i = 0; i < MAX_SPELL_AFFECTS; i++)
	{
		if (af[i].bitvector || (af[i].location != APPLY_NONE))
			affect_join(victim, af+i, accum_duration, FALSE, accum_affect, FALSE);
	}
	
	if (to_vict != NULL)
		act(to_vict, FALSE, victim, NULL, ch, TO_CHAR);
	if (to_room != NULL)
		act(to_room, TRUE, victim, NULL, ch, TO_ROOM);
}


/*
 * This function is used to provide services to mag_groups.  This function
 * is the one you should change to add new group spells.
 */
void perform_mag_groups(int level, CHAR_DATA *ch, CHAR_DATA *tch, int spellnum, int savetype)
{
	switch (spellnum)
	{
	case SPELL_GROUP_HEAL:
		mag_points(level, ch, tch, SPELL_HEAL, savetype);
		break;
	case SPELL_GROUP_ARMOR:
		mag_affects(level, ch, tch, SPELL_ARMOR, savetype);
		break;
	case SPELL_GROUP_RECALL:
		spell_recall(level, ch, tch, NULL);
		break;
	}
}


/*
 * Every spell that affects the group should run through here
 * perform_mag_groups contains the switch statement to send us to the right
 * magic.
 *
 * group spells affect everyone grouped with the caster who is in the room,
 * caster last.
 *
 * To add new group spells, you shouldn't have to change anything in
 * mag_groups -- just add a new case to perform_mag_groups.
 */
void mag_groups(int level, CHAR_DATA *ch, int spellnum, int savetype)
{
	CHAR_DATA *tch, *k;
	FOLLOW_DATA *f, *f_next;
	
	if (ch == NULL)
		return;
	
	if (!AFF_FLAGGED(ch, AFF_GROUP))
		return;
	if (ch->master != NULL)
		k = ch->master;
	else
		k = ch;

	for (f = k->followers; f; f = f_next)
	{
		f_next = f->next;
		tch = f->follower;
		if (tch->in_room != ch->in_room)
			continue;
		if (!AFF_FLAGGED(tch, AFF_GROUP))
			continue;
		if (ch == tch)
			continue;
		perform_mag_groups(level, ch, tch, spellnum, savetype);
	}
	
	if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP))
		perform_mag_groups(level, ch, k, spellnum, savetype);
	perform_mag_groups(level, ch, ch, spellnum, savetype);
}


/*
 * mass spells affect every creature in the room except the caster.
 *
 * No spells of this class currently implemented as of Circle 3.0.
 */
void mag_masses(int level, CHAR_DATA *ch, int spellnum, int savetype)
{
	CHAR_DATA *tch, *tch_next;
	const char *to_char = NULL, *to_room = NULL;
	
	if (ch == NULL)
		return;
	
	/*
	 * to add spells to this fn, just add the message here plus an entry
	 * in mag_affects for the affecting part of the spell.
	 */
	switch (spellnum)
	{
	case SPELL_FLASH:
		to_char = "You gesture and an unbelievable flash of pure light shine all around you!";
		to_room = "$n gestures and everything is enveloped by an unbelievable flash of the purer light!";
		break;
	}

	for (tch = ch->in_room->people; tch; tch = tch_next)
	{
		tch_next = tch->next_in_room;
		
		/*
		 * The skips: 1: the caster
		 *            2: immortals
		 *            3: if no pk on this mud, skips over all players
		 *            4: pets (charmed NPCs)
		 */
		
		if (tch == ch)
			continue;
		if (IS_IMMORTAL(tch))
			continue;
		if (!pkill_ok(ch, tch))
			continue;
		if (!IS_NPC(ch) && IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
			continue;

		mag_affects(level, ch, tch, spellnum, savetype);
	}
}


/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
 */
void mag_areas(int level, CHAR_DATA *ch, int spellnum, int savetype)
{
	CHAR_DATA *tch, *next_tch;
	const char *to_char = NULL, *to_room = NULL;
	
	if (ch == NULL)
		return;
	
	/*
	 * to add spells to this fn, just add the message here plus an entry
	 * in mag_damage for the damaging part of the spell.
	 */
	switch (spellnum)
	{
	case SPELL_EARTHQUAKE:
		to_char = "You gesture and the earth begins to shake all around you!";
		to_room = "$n gracefully gestures and the earth begins to shake violently!";
		break;
	case SPELL_INCENDIARY_CLOUD:
		to_char = "You gesture and the air begins to become fire all around you!";
		to_room = "$n gracefully gestures and the air begins to become fire!";
		break;
	}
	
	if (to_char != NULL)
		act(to_char, FALSE, ch, 0, 0, TO_CHAR);
	if (to_room != NULL)
		act(to_room, FALSE, ch, 0, 0, TO_ROOM);
	
	for (tch = ch->in_room->people; tch; tch = next_tch)
	{
		next_tch = tch->next_in_room;
		
		/*
		 * The skips: 1: the caster
		 *            2: immortals
		 *            3: if no pk on this mud, skips over all players
		 *            4: pets (charmed NPCs)
		 */
		
		if (tch == ch)
			continue;
		if (IS_IMMORTAL(tch))
			continue;
		if (!pkill_ok(ch, tch))
			continue;
		if (!IS_NPC(ch) && IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
			continue;
		
		/* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
		mag_damage(level, ch, tch, spellnum, 1);
	}
}


/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 *
 *  None of these spells are currently implemented in Circle 3.0; these
 *  were taken as examples from the JediMUD code.  Summons can be used
 *  for spells like clone, ariel servant, etc.
 *
 * 10/15/97 (gg) - Implemented Animate Dead and Clone.
 */

/*
 * These use act(), don't put the \r\n.
 */
const char *mag_summon_msgs[] =
{
  "\r\n",
  "$n makes a strange magical gesture; you feel a strong breeze!",
  "$n animates a corpse!",
  "$N appears from a cloud of thick blue smoke!",
  "$N appears from a cloud of thick green smoke!",
  "$N appears from a cloud of thick red smoke!",
  "$N disappears in a thick black cloud!"
  "As $n makes a strange magical gesture, you feel a strong breeze.",
  "As $n makes a strange magical gesture, you feel a searing heat.",
  "As $n makes a strange magical gesture, you feel a sudden chill.",
  "As $n makes a strange magical gesture, you feel the dust swirl.",
  "$n magically divides!",
  "$n animates a corpse!"
};

/*
 * Keep the \r\n because these use send_to_char.
 */
const char *mag_summon_fail_msgs[] =
{
  "\r\n",
  "There are no such creatures.\r\n",
  "Uh oh...\r\n",
  "Oh dear.\r\n",
  "Gosh durnit!\r\n",
  "The elements resist!\r\n",
  "You failed.\r\n",
  "There is no corpse!\r\n"
};

/* These mobiles do not exist. */
#define MOB_MONSUM_I			130
#define MOB_MONSUM_II			140
#define MOB_MONSUM_III			150
#define MOB_GATE_I				160
#define MOB_GATE_II				170
#define MOB_GATE_III			180

/* Defined mobiles. */
#define MOB_ELEMENTAL_BASE		20	/* Only one for now. */
#define MOB_CLONE				10
#define MOB_ZOMBIE				11
#define MOB_AERIALSERVANT		19


void mag_summons(int level, CHAR_DATA *ch, OBJ_DATA *obj, int spellnum, int savetype)
{
	CHAR_DATA *mob = NULL;
	OBJ_DATA *tobj, *next_obj;
	int pfail = 0, msg = 0, fmsg = 0, num = 1, handle_corpse = FALSE, i;
	mob_vnum mob_num;
	
	if (ch == NULL)
		return;
	
	switch (spellnum)
	{
	case SPELL_CLONE:
		msg = 10;
		fmsg = number(2, 6);	/* Random fail message. */
		mob_num = MOB_CLONE;
		pfail = 50;	/* 50% failure, should be based on something later. */
		break;
		
	case SPELL_ANIMATE_DEAD:
		if (obj == NULL || !IS_CORPSE(obj))
		{
			act(mag_summon_fail_msgs[7], FALSE, ch, 0, 0, TO_CHAR);
			return;
		}
		handle_corpse = TRUE;
		msg = 11;
		fmsg = number(2, 6);	/* Random fail message. */
		mob_num = MOB_ZOMBIE;
		pfail = 10;	/* 10% failure, should vary in the future. */
		break;
		
	default:
		return;
	}
	
	if (AFF_FLAGGED(ch, AFF_CHARM))
	{
		send_to_char("You are too giddy to have any followers!\r\n", ch);
		return;
	}
	if (number(0, 101) < pfail)
	{
		send_to_char(mag_summon_fail_msgs[fmsg], ch);
		return;
	}
	for (i = 0; i < num; i++)
	{
		if (!(mob = read_mobile(mob_num, VIRTUAL)))
		{
			send_to_char("You don't quite remember how to make that creature.\r\n", ch);
			return;
		}
		char_to_room(mob, ch->in_room);
		IS_CARRYING_W(mob) = 0;
		IS_CARRYING_N(mob) = 0;
		SET_BIT(AFF_FLAGS(mob), AFF_CHARM);
		if (spellnum == SPELL_CLONE)
		{	/* Don't mess up the proto with strcpy. */
			GET_PC_NAME(mob)	= str_dup(GET_NAME(ch));
			GET_SDESC(mob)		= str_dup(GET_NAME(ch));
		}
		act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM);
		add_follower(mob, ch);
	}
	if (handle_corpse)
	{
		for (tobj = obj->first_content; tobj; tobj = next_obj)
		{
			next_obj = tobj->next_content;
			obj_from_obj(tobj);
			tobj = obj_to_char(tobj, mob);
		}
		extract_obj(obj);
	}
}


void mag_points(int level, CHAR_DATA *ch, CHAR_DATA *victim, int spellnum, int savetype)
{
	int healing = 0, move = 0;
	
	if (victim == NULL)
		return;
	
	switch (spellnum)
	{
	case SPELL_CURE_LIGHT:
		healing = dice(1, 8) + 1 + (level / 4);
		send_to_char("You feel better.\r\n", victim);
		break;
	case SPELL_CURE_CRITIC:
		healing = dice(3, 8) + 3 + (level / 4);
		send_to_char("You feel a lot better!\r\n", victim);
		break;
	case SPELL_HEAL:
		healing = 100 + dice(3, 8);
		send_to_char("A warm feeling floods your body.\r\n", victim);
		break;
	case SPELL_REFRESH:
		move = MAX(dice(level, 4) + level, 25);
		send_to_char("You feel less tired.\r\n", victim);
		break;
	}

	GET_HIT(victim)		= MIN(GET_MAX_HIT(victim), GET_HIT(victim) + healing);
	GET_MOVE(victim)	= MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + move);

	update_pos(victim);
}


void mag_unaffects(int level, CHAR_DATA *ch, CHAR_DATA *victim, int spellnum, int type)
{
	int spell = 0;
	const char *to_vict = NULL, *to_room = NULL;
	
	if (victim == NULL)
		return;
	
	switch (spellnum)
	{
	case SPELL_CURE_BLIND:
	case SPELL_HEAL:
		spell = SPELL_BLINDNESS;
		to_vict = "Your vision returns!";
		to_room = "There's a momentary gleam in $n's eyes.";
		break;
	case SPELL_REMOVE_POISON:
		spell = SPELL_POISON;
		to_vict = "A warm feeling runs through your body!";
		to_room = "$n looks better.";
		break;
	case SPELL_REMOVE_CURSE:
		spell = SPELL_CURSE;
		to_vict = "You don't feel so unlucky.";
		break;
	default:
		log("SYSERR: unknown spellnum %d passed to mag_unaffects.", spellnum);
		return;
	}
	
	if (!affected_by_spell(victim, spell))
	{
		if (spellnum != SPELL_HEAL)		/* 'cure blindness' message. */
			send_to_char(NOEFFECT, ch);
		return;
	}
	
	affect_from_char(victim, spell);
	
	if (to_vict != NULL)
		act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
	if (to_room != NULL)
		act(to_room, TRUE, victim, 0, ch, TO_ROOM);
	
}


void mag_alter_objs(int level, CHAR_DATA *ch, OBJ_DATA *obj, int spellnum, int savetype)
{
	const char *to_char = NULL, *to_room = NULL;
	
	if (obj == NULL)
		return;
	
	switch (spellnum)
	{
	case SPELL_BLESS:
		if (!OBJ_FLAGGED(obj, ITEM_BLESS) &&
			(GET_OBJ_WEIGHT(obj) <= 5 * GET_LEVEL(ch)))
		{
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_BLESS);
			to_char = "$p glows briefly.";
		}
		break;
	case SPELL_CURSE:
		if (!OBJ_FLAGGED(obj, ITEM_NODROP))
		{
			SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NODROP);
			if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
				GET_OBJ_VAL(obj, 2)--;
			to_char = "$p briefly glows red.";
		}
		break;
	case SPELL_INVISIBLE:
		if (!OBJ_FLAGGED(obj, ITEM_NOINVIS | ITEM_INVISIBLE))
		{
			SET_BIT(obj->obj_flags.extra_flags, ITEM_INVISIBLE);
			to_char = "$p vanishes.";
		}
		break;
	case SPELL_POISON:
		if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
			(GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
			(GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, 3))
		{
			GET_OBJ_VAL(obj, 3) = 1;
			to_char = "$p steams briefly.";
		}
		break;
	case SPELL_REMOVE_CURSE:
		if (OBJ_FLAGGED(obj, ITEM_NODROP))
		{
			REMOVE_BIT(obj->obj_flags.extra_flags, ITEM_NODROP);
			if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
				GET_OBJ_VAL(obj, 2)++;
			to_char = "$p briefly glows blue.";
		}
		break;
	case SPELL_REMOVE_POISON:
		if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
			(GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
			(GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, 3))
		{
			GET_OBJ_VAL(obj, 3) = 0;
			to_char = "$p steams briefly.";
		}
		break;
	}
	
	if (to_char == NULL)
		send_to_char(NOEFFECT, ch);
	else
		act(to_char, TRUE, ch, obj, 0, TO_CHAR);
	
	if (to_room != NULL)
		act(to_room, TRUE, ch, obj, 0, TO_ROOM);
	else if (to_char != NULL)
		act(to_char, TRUE, ch, obj, 0, TO_ROOM);
}



void mag_creations(int level, CHAR_DATA *ch, int spellnum)
{
	OBJ_DATA *tobj;
	obj_vnum z;
	
	if (ch == NULL)
		return;
	/* level = MAX(MIN(level, LVL_IMPL), 1); - Hm, not used. */
	
	switch (spellnum)
	{
	case SPELL_CREATE_FOOD:
		z = 10;
		break;
	default:
		send_to_char("Spell unimplemented, it would seem.\r\n", ch);
		return;
	}
	
	if (!(tobj = read_object(z, VIRTUAL)))
	{
		send_to_char("I seem to have goofed.\r\n", ch);
		log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
			spellnum, z);
		return;
	}
	
	tobj = obj_to_char(tobj, ch);
	act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM);
	act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR);
}


void mag_rooms(int level, CHAR_DATA *ch, int spellnum)
{
	ROOM_AFFECT *raff;
	char *to_char = NULL, *to_room = NULL;
	bool accum = FALSE;
	int aff, ticks;
	
	aff = ticks =0;
	
	if (ch == NULL)
		return;
	
	level = MAX(MIN(level, LVL_IMPL), 1);
	
	switch (spellnum)
	{
	case SPELL_WALL_OF_FOG:
		to_char = "You create a fog out of nowhere.";
		to_room = "%n creates a fog out of nowhere.";
		aff		= RAFF_FOG;
		ticks	= 2;
		accum	= TRUE;
		break;
	case SPELL_ROOM_SHIELD:
		to_char = "You create a transparent mist that surround the room.";
		to_room = "$n creates a transparent mist that surround the room.";
		aff		= RAFF_SHIELD;
		ticks	= 3;
		break;
	default:
		log("SYSERR: unknown spellnum %d passed to mag_rooms", spellnum);
		return;
	}

	if ((raff = get_room_aff(ch->in_room, spellnum)))
	{
		if (!accum)
		{
			send_to_char(NOEFFECT, ch);
			return;
		}

		raff->timer += ticks;
	}
	else
	{
		if (ROOM_AFFECTED(ch->in_room, aff))
		{
			send_to_char(NOEFFECT, ch);
			return;
		}

		/* create, initialize, and link a room-affection node */
		CREATE(raff, ROOM_AFFECT, 1);
		raff->coord		= NULL;
		raff->vroom		= ch->in_room->number;
		raff->timer		= ticks;
		raff->bitvector	= aff;
		raff->spell		= spellnum;
		raff->level		= GET_LEVEL(ch);
		raff->value		= 0;
		raff->text		= NULL;
		if (IN_WILD(ch))
		{
			CREATE(raff->coord, COORD_DATA, 1);
			raff->coord->y	= GET_Y(ch);
			raff->coord->x	= GET_X(ch);
		}
		
		/* add to the room list */
		raff->next_in_room	= ch->in_room->affections;
		ch->in_room->affections = raff;
		
		/* set the affection bitv on the room */
		if (aff)
			SET_BIT(ROOM_AFFS(ch->in_room), aff);
		
		/* add to the global list */
		raff->next		= raff_list;
		raff_list		= raff;
	}
	
	if (to_char == NULL)
		send_to_char(NOEFFECT, ch);
	else
		act(to_char, TRUE, ch, 0, 0, TO_CHAR);

	if (to_room != NULL)
		act(to_room, TRUE, ch, 0, 0, TO_ROOM);
	else if (to_char != NULL)
		act(to_char, TRUE, ch, 0, 0, TO_ROOM);
}


void weapon_spells(CHAR_DATA *ch, CHAR_DATA *vict, OBJ_DATA *wpn)
{
	OBJ_SPELLS_DATA *oSpell;
	
	if (!wpn || !wpn->special || !OBJ_FLAGGED(wpn, ITEM_HAS_SPELLS))
		return;

	for (oSpell = (OBJ_SPELLS_DATA *) wpn->special; oSpell; oSpell = oSpell->next)
	{
		/* Do some sanity checking, in case someone flees, etc. */
		/* Can't have enough of 'em.  */
		if (ch->in_room != vict->in_room)
		{
			if (FIGHTING(ch) && FIGHTING(ch) == vict)
				stop_fighting(ch);
			break;
		}

		if (oSpell->percent > number(1, 101))
		{
			act("$p leaps to action with an attack of its own.",TRUE, ch, wpn, NULL, TO_CHAR);
			act("$p leaps to action with an attack of its own.",TRUE, ch, wpn, NULL, TO_ROOM);
			if (call_magic(ch, vict, NULL, oSpell->spellnum, oSpell->level, CAST_OBJ) == -1)
				break;
		}
	}
}
