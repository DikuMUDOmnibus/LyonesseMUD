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
*   File: spell_parser.c                                Part of CircleMUD *
*  Usage: top-level magic routines; outside points of entry to magic sys. *
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
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "constants.h"

#define SINFO spell_info[spellnum]

/* extern functions */
OBJ_SPELLS_DATA *GetCharItemsSpell(CHAR_DATA *ch, int sn);
int compute_armor_class(CHAR_DATA *ch);

/* local globals */
SPELL_INFO_DATA spell_info[TOP_SPELL_DEFINE + 1];
int				spell_sort_info[MAX_SKILLS + 1];

/* local functions */
void say_spell(CHAR_DATA *ch, int spellnum, CHAR_DATA *tch, OBJ_DATA *tobj);
void spello(int spl, const char *name, int max_mana, int min_mana, int mana_change, int minpos, int targets, int violent, int routines, const char *wearoff);
int mag_manacost(CHAR_DATA *ch, int spellnum);
ACMD(do_cast);
void unused_spell(int spl);
void mag_assign_spells(void);

/*
 * This arrangement is pretty stupid, but the number of skills is limited by
 * the playerfile.  We can arbitrarily increase the number of skills by
 * increasing the space in the playerfile. Meanwhile, 200 should provide
 * ample slots for skills.
 */

struct syllable
{
  const char		*org;
  const char		*news;
};


struct syllable syls[] =
{
  {" ", " "},
  {"ar", "abra"},
  {"ate", "i"},
  {"cau", "kada"},
  {"blind", "nose"},
  {"bur", "mosa"},
  {"cu", "judi"},
  {"de", "oculo"},
  {"dis", "mar"},
  {"ect", "kamina"},
  {"en", "uns"},
  {"gro", "cra"},
  {"light", "dies"},
  {"lo", "hi"},
  {"magi", "kari"},
  {"mon", "bar"},
  {"mor", "zak"},
  {"move", "sido"},
  {"ness", "lacri"},
  {"ning", "illa"},
  {"per", "duda"},
  {"ra", "gru"},
  {"re", "candus"},
  {"son", "sabru"},
  {"tect", "infra"},
  {"tri", "cula"},
  {"ven", "nofo"},
  {"word of", "inset"},
  {"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"}, {"g", "t"},
  {"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"},
  {"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"}, {"u", "e"},
  {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"}, {"z", "k"}, {"", ""}
};

const char *unused_spellname = "!UNUSED!"; /* So we can get &unused_spellname */

int mag_manacost(CHAR_DATA *ch, int spellnum)
{
	int iMax, iMin;

	iMax = SINFO.mana_max;
	iMax -= SINFO.mana_change * (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS(ch)]);
	iMin = SINFO.mana_min;

	return (MAX(iMax, iMin));
/*
  return MAX(SINFO.mana_max -
	  (SINFO.mana_change * (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS(ch)])),
	     SINFO.mana_min);
*/
}


/* say_spell erodes buf, buf1, buf2 */
void say_spell(CHAR_DATA *ch, int spellnum, CHAR_DATA *tch, OBJ_DATA *tobj)
{
	CHAR_DATA *i;
	char lbuf[256];
	const char *format;
	int j, ofs = 0;
	
	*buf = '\0';
	strcpy(lbuf, skill_name(spellnum));
	
	while (lbuf[ofs])
	{
		for (j = 0; *(syls[j].org); j++)
		{
			if (!strncmp(syls[j].org, lbuf + ofs, strlen(syls[j].org)))
			{
				strcat(buf, syls[j].news);
				ofs += strlen(syls[j].org);
				break;
			}
		}
		/* i.e., we didn't find a match in syls[] */
		if (!*syls[j].org)
		{
			log("No entry in syllable table for substring of '%s'", lbuf);
			ofs++;
		}
	}
	
	if (tch != NULL && IN_ROOM(tch) == IN_ROOM(ch))
	{
		if (tch == ch)
			format = "$n closes $s eyes and utters the words, '%s'.";
		else
			format = "$n stares at $N and utters the words, '%s'.";
	}
	else if (tobj != NULL &&
		((IN_ROOM(tobj) == IN_ROOM(ch)) || (tobj->carried_by == ch)))
		format = "$n stares at $p and utters the words, '%s'.";
	else
		format = "$n utters the words, '%s'.";
	
	sprintf(buf1, format, skill_name(spellnum));
	sprintf(buf2, format, buf);
	
	for (i = ch->in_room->people; i; i = i->next_in_room)
	{
		if (i == ch || i == tch || !i->desc || !AWAKE(i))
			continue;
		if (GET_CLASS(ch) == GET_CLASS(i))
			perform_act(buf1, ch, tobj, tch, i);
		else
			perform_act(buf2, ch, tobj, tch, i);
	}
	
	if (tch != NULL && tch != ch && IN_ROOM(tch) == IN_ROOM(ch))
	{
		sprintf(buf1, "$n stares at you and utters the words, '%s'.",
			GET_CLASS(ch) == GET_CLASS(tch) ? skill_name(spellnum) : buf);
		act(buf1, FALSE, ch, NULL, tch, TO_VICT);
	}
}


int compare_spells(const void *x, const void *y)
{
	int a = *(const int *) x, b = *(const int *) y;
	
	return (strcmp(spell_info[a].name, spell_info[b].name));
}

void sort_spells(void)
{
	int a;
	
	/* initialize array, avoiding reserved. */
	for (a = 1; a <= MAX_SKILLS; a++)
		spell_sort_info[a] = a;
	
	qsort(&spell_sort_info[1], MAX_SKILLS, sizeof(int), compare_spells);
}

const char *how_good(int percent)
{
	if (percent < 0)	return (" (error)");
	if (percent == 0)	return (" (not learned)");
	if (percent <= 10)	return (" (awful)");
	if (percent <= 20)	return (" (bad)");
	if (percent <= 40)	return (" (poor)");
	if (percent <= 55)	return (" (average)");
	if (percent <= 70)	return (" (fair)");
	if (percent <= 80)	return (" (good)");
	if (percent <= 85)	return (" (very good)");
	
	return (" (superb)");
}

char *get_spell_name(char *argument)
{
	char *s;
	
	s = strtok(argument, "'");
	s = strtok(NULL, "'");
	
	return (s);
}

/*
 * This function should be used anytime you are not 100% sure that you have
 * a valid spell/skill number.  A typical for() loop would not need to use
 * this because you can guarantee > 0 and <= TOP_SPELL_DEFINE.
 */
const char *skill_name(int num)
{
	if (num > 0 && num <= TOP_SPELL_DEFINE)
		return (spell_info[num].name);
	else if (num == -1)
		return ("UNUSED");
	else
		return ("UNDEFINED");
}

	 
int find_skill_num(char *name)
{
	int index, ok;
	char *temp, *temp2;
	char first[256], first2[256], tempbuf[256];
	
	for (index = 1; index <= TOP_SPELL_DEFINE; index++)
	{
		if (is_abbrev(name, spell_info[index].name))
			return (index);
		
		ok = TRUE;
		temp = any_one_arg(strcpy(tempbuf, spell_info[index].name), first);
		temp2 = any_one_arg(name, first2);

		while (*first && *first2 && ok)
		{
			if (!is_abbrev(first2, first))
				ok = FALSE;
			temp = any_one_arg(temp, first);
			temp2 = any_one_arg(temp2, first2);
		}
		
		if (ok && !*first2)
			return (index);
	}
	
	return (-1);
}


void ImproveSkill(CHAR_DATA *ch, int skill)
{
	int percent = GET_SKILL(ch, skill);
	
	if (number(1, 200) > GET_WIS(ch) + GET_INT(ch))
		return;
	if (percent >= 95 || percent <= 0)
		return;

	percent += number(1, 3);
	
	SET_SKILL(ch, skill, percent);
	ch_printf(ch, "You feel your skill in '%s' improving.", skill_name(skill));
}

/*
 * Core check function for success in using spl/skl
 */
bool success(CHAR_DATA *ch, CHAR_DATA *vict, int skill, int val)
{
	int percent, prob;

	if ( !ch || skill <= 0 || skill > MAX_SKILLS )
		return (FALSE);

	/* LVL_GOD and above always succeeds */
	if ( GET_LEVEL(ch) >= LVL_GOD )
		return (TRUE);

	percent = number(1, 105);
	if (val)
		prob = val;
	else
		prob = GET_SKILL(ch, skill);

	// modifier goes down here...
	switch (skill)
	{
	case SKILL_HIDE:
		prob += dex_app_skill[GET_DEX(ch)].hide;
		break;
	case SKILL_SNEAK:
		prob += dex_app_skill[GET_DEX(ch)].sneak;
		break;
	case SKILL_BASH:
		if (vict && IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOBASH))
		{
			percent = 101;
			prob = 0;
		}
		break;
	case SKILL_KICK:
		percent = ((10 - (compute_armor_class(vict) / 10)) * 2) + number(1, 101);
		break;
	case SKILL_TRACK:
		if (vict && AFF_FLAGGED(vict, AFF_NOTRACK))
		{
			percent = 101;
			prob = 0;
		}
		break;
	}

	if (percent > prob)
		return (FALSE);

	if (!IS_NPC(ch))
		ImproveSkill(ch, skill);

	return (TRUE);
}


/*
 * This function is the very heart of the entire magic system.  All
 * invocations of all types of magic -- objects, spoken and unspoken PC
 * and NPC spells, the works -- all come through this function eventually.
 * This is also the entry point for non-spoken or unrestricted spells.
 * Spellnum 0 is legal but silently ignored here, to make callers simpler.
 */
int call_magic(CHAR_DATA *caster, CHAR_DATA *cvict, OBJ_DATA *ovict,
	       int spellnum, int level, int casttype)
{
	int savetype;
	
	if (spellnum < 1 || spellnum > TOP_SPELL_DEFINE)
		return (0);
	
	if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_NOMAGIC))
	{
		send_to_char("Your magic fizzles out and dies.\r\n", caster);
		act("$n's magic fizzles out and dies.", FALSE, caster, 0, 0, TO_ROOM);
		return (0);
	}
	if (ROOM_FLAGGED(IN_ROOM(caster), ROOM_PEACEFUL) &&
		(SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)))
	{
		send_to_char("A flash of white light fills the room, dispelling your "
			"violent magic!\r\n", caster);
		act("White light from no particular source suddenly fills the room, "
			"then vanishes.", FALSE, caster, 0, 0, TO_ROOM);
		return (0);
	}

	/* determine the type of saving throw */
	switch (casttype)
	{
	case CAST_STAFF:
	case CAST_SCROLL:
	case CAST_POTION:
	case CAST_WAND:
	case CAST_OBJ:
		savetype = SAVING_ROD;
		break;
	case CAST_SPELL:
		savetype = SAVING_SPELL;
		break;
	default:
		savetype = SAVING_BREATH;
		break;
	}
	
	
	if (IS_SET(SINFO.routines, MAG_DAMAGE))
	{
		if (mag_damage(level, caster, cvict, spellnum, savetype) == -1)
			return (-1);	/* Successful and target died, don't cast again. */
	}
		
	if (IS_SET(SINFO.routines, MAG_AFFECTS))
		mag_affects(level, caster, cvict, spellnum, savetype);
		
	if (IS_SET(SINFO.routines, MAG_UNAFFECTS))
		mag_unaffects(level, caster, cvict, spellnum, savetype);
		
	if (IS_SET(SINFO.routines, MAG_POINTS))
		mag_points(level, caster, cvict, spellnum, savetype);
		
	if (IS_SET(SINFO.routines, MAG_ALTER_OBJS))
		mag_alter_objs(level, caster, ovict, spellnum, savetype);
		
	if (IS_SET(SINFO.routines, MAG_GROUPS))
		mag_groups(level, caster, spellnum, savetype);
		
	if (IS_SET(SINFO.routines, MAG_MASSES))
		mag_masses(level, caster, spellnum, savetype);
		
	if (IS_SET(SINFO.routines, MAG_AREAS))
		mag_areas(level, caster, spellnum, savetype);
		
	if (IS_SET(SINFO.routines, MAG_SUMMONS))
		mag_summons(level, caster, ovict, spellnum, savetype);
		
	if (IS_SET(SINFO.routines, MAG_CREATIONS))
		mag_creations(level, caster, spellnum);
		
	if (IS_SET(SINFO.routines, MAG_ROOM))
		mag_rooms(level, caster, spellnum);
		
	if (IS_SET(SINFO.routines, MAG_MANUAL))
	{
		switch (spellnum)
		{
		case SPELL_CHARM:			MANUAL_SPELL(spell_charm);			break;
		case SPELL_CREATE_WATER:	MANUAL_SPELL(spell_create_water);	break;
		case SPELL_DETECT_POISON:	MANUAL_SPELL(spell_detect_poison);	break;
		case SPELL_ENCHANT_WEAPON:	MANUAL_SPELL(spell_enchant_weapon);	break;
		case SPELL_IDENTIFY:		MANUAL_SPELL(spell_identify);		break;
		case SPELL_LOCATE_OBJECT:	MANUAL_SPELL(spell_locate_object);	break;
		case SPELL_SUMMON:			MANUAL_SPELL(spell_summon);			break;
		case SPELL_WORD_OF_RECALL:	MANUAL_SPELL(spell_recall);			break;
		case SPELL_TELEPORT:		MANUAL_SPELL(spell_teleport);		break;
		case SPELL_FEAR:			MANUAL_SPELL(spell_fear);			break;
		}
	}
	
	return (1);
}


/*
 * cast_spell is used generically to cast any spoken spell, assuming we
 * already have the target char/obj and spell number.  It checks all
 * restrictions, etc., prints the words, etc.
 *
 * Entry point for NPC casts.  Recommended entry point for spells cast
 * by NPCs via specprocs.
 *
 * 09-10-2001 - Modified to handle casting spells thru enchanted items.
 */
int cast_spell(CHAR_DATA *ch, OBJ_SPELLS_DATA *oSpell, CHAR_DATA *tch,
	       OBJ_DATA *tobj, int spellnum)
{
	int levelnum = 0;

	if (spellnum < 0 || spellnum > TOP_SPELL_DEFINE)
	{
		log("SYSERR: cast_spell trying to call spellnum %d/%d.\n", spellnum,
			TOP_SPELL_DEFINE);
		return (0);
	}
	
	if (GET_POS(ch) < SINFO.min_position)
	{
		switch (GET_POS(ch))
		{
		case POS_SLEEPING:
			send_to_char("You dream about great magical powers.\r\n", ch);
			break;
		case POS_RESTING:
			send_to_char("You cannot concentrate while resting.\r\n", ch);
			break;
		case POS_SITTING:
			send_to_char("You can't do this sitting!\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("Impossible!  You can't concentrate enough!\r\n", ch);
			break;
		default:
			send_to_char("You can't do much of anything like this!\r\n", ch);
			break;
		}
		return (0);
	}
	if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == tch))
	{
		send_to_char("You are afraid you might hurt your master!\r\n", ch);
		return (0);
	}
	if ((tch != ch) && IS_SET(SINFO.targets, TAR_SELF_ONLY))
	{
		send_to_char("You can only cast this spell upon yourself!\r\n", ch);
		return (0);
	}
	if ((tch == ch) && IS_SET(SINFO.targets, TAR_NOT_SELF))
	{
		send_to_char("You cannot cast this spell upon yourself!\r\n", ch);
		return (0);
	}
	if (IS_SET(SINFO.routines, MAG_GROUPS) && !AFF_FLAGGED(ch, AFF_GROUP))
	{
		send_to_char("You can't cast this spell if you're not in a group!\r\n",ch);
		return (0);
	}
	if (oSpell && oSpell->percent < number(0, 101))
	{
		send_to_char("You fail in drawing the spell from the enchanted item.\r\n", ch);
		return (0);
	}

	send_to_char(OK, ch);
	say_spell(ch, spellnum, tch, tobj);
	
	if (oSpell)
		levelnum = oSpell->level;
	else
		levelnum = GET_LEVEL(ch);

	return (call_magic(ch, tch, tobj, spellnum, levelnum, CAST_SPELL));
}

/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell().
 */
ACMD(do_cast)
{
	CHAR_DATA *tch = NULL;
	OBJ_DATA *tobj = NULL;
	OBJ_SPELLS_DATA *oSpell = NULL;
	char *s, *t;
	sh_int nospell = 0;
	int mana, spellnum, i, target = 0;
	
	if (IS_NPC(ch))
		return;
	
	if ( !CASTING_CLASS(ch) && !MEMMING_CLASS(ch) )
	{
		send_to_char("You don't know how to use arcane powers.\r\n", ch);
		return;
	}

	if ( AFF_FLAGGED(ch, AFF_MEMORIZING) )
	{
		send_to_char("You can't use magic while memorizing spells.\r\n", ch);
		return;
	}
		
	/* get: blank, spell name, target name */
	s = strtok(argument, "'");
	if (s == NULL)
	{
		send_to_char("Cast what where?\r\n", ch);
		return;
	}
	s = strtok(NULL, "'");
	if (s == NULL)
	{
		send_to_char("Spell names must be enclosed in the Holy Magic Symbols: '\r\n", ch);
		return;
	}
	t = strtok(NULL, "\0");
	
	/* spellnum = search_block(s, spells, 0); */
	spellnum = find_skill_num(s);
	
	if ((spellnum < 1) || (spellnum >= MAX_SPELLS))
	{
		send_to_char("Cast what?!?\r\n", ch);
		return;
	}
	
	if ( MEMMING_CLASS(ch) )
	{
		if ( !MEMORIZED(ch, spellnum) )
			nospell = 1;
	}
	else
	{
		// for memming classes this check is in do_memorize()
		if (GET_LEVEL(ch) < SINFO.min_level[(int) GET_CLASS(ch)])
		{
			send_to_char("You do not know that spell!\r\n", ch);
			return;
		}
		
		if (GET_SKILL(ch, spellnum) == 0)
			nospell = 2;
	}

	/*
	 * doesn't have the spell memorized or doesn't knows the spell..
	 * is she using an item that holds the spell??
	 */
	if ( nospell )
	{
		if ( !(oSpell = GetCharItemsSpell(ch, spellnum)) )
		{
			if ( nospell == 1 )
				send_to_char("You haven't that spell ready.\r\n", ch);
			else
				send_to_char("You are unfamiliar with that spell.\r\n", ch);
			return;
		}
	}

	if ( RIDING(ch) && GET_SKILL(ch, spellnum) < 90)
	{
		send_to_char("You aren't skilled enough to cast that spell while mounted.\r\n", ch);
		return;
	}

	/* Find the target */
	if (t != NULL)
	{
		argument = one_argument(strcpy(arg, t), t);
		skip_spaces(&t);
	}

	if (IS_SET(SINFO.targets, TAR_IGNORE))
		target = TRUE;
	else if (t != NULL && *t)
	{
		if (!target && (IS_SET(SINFO.targets, TAR_CHAR_ROOM)))
		{
			if ((tch = get_char_vis(ch, t, NULL, FIND_CHAR_ROOM)) != NULL)
				target = TRUE;
		}
		if (!target && IS_SET(SINFO.targets, TAR_CHAR_WORLD))
		{
			if ((tch = get_char_vis(ch, t, NULL, FIND_CHAR_WORLD)) != NULL)
				target = TRUE;
		}
		
		if (!target && IS_SET(SINFO.targets, TAR_OBJ_INV))
		{
			if ((tobj = get_obj_in_list_vis_rev(ch, t, NULL, ch->last_carrying)) != NULL)
				target = TRUE;
		}
		
		if (!target && IS_SET(SINFO.targets, TAR_OBJ_EQUIP))
		{
			for (i = 0; !target && i < NUM_WEARS; i++)
			{
				if (GET_EQ(ch, i) && isname(t, GET_EQ(ch, i)->name))
				{
					tobj = GET_EQ(ch, i);
					target = TRUE;
				}
			}
		}

		if (!target && IS_SET(SINFO.targets, TAR_OBJ_ROOM))
		{
			if ((tobj = get_obj_in_list_vis_rev(ch, t, NULL, ch->in_room->last_content)) != NULL)
				target = TRUE;
		}
		
		if (!target && IS_SET(SINFO.targets, TAR_OBJ_WORLD))
		{
			if ((tobj = get_obj_vis(ch, t, NULL)) != NULL)
				target = TRUE;
		}

		/* target is a char in an adiacent room */
		if (!target && IS_SET(SINFO.targets, TAR_CHAR_RANGED))
		{
			EXIT_DATA *pexit = NULL;
			char dir[MAX_INPUT_LENGTH];

			one_argument(argument, dir);

			if (*dir)
				if ((pexit = find_door(ch, NULL, dir, "cast")))
					if ((tch = get_char_elsewhere_vis(ch, pexit->to_room, t, NULL)) != NULL)
						target = TRUE;
		}
	}
	else			/* if target string is empty */
	{
		if (!target && IS_SET(SINFO.targets, TAR_FIGHT_SELF))
		{
			if (FIGHTING(ch) != NULL)
			{
				tch = ch;
				target = TRUE;
			}
		}
		if (!target && IS_SET(SINFO.targets, TAR_FIGHT_VICT))
		{
			if (FIGHTING(ch) != NULL)
			{
				tch = FIGHTING(ch);
				target = TRUE;
			}
		}
		/* if no target specified, and the spell isn't violent, default to self */
		if (!target && IS_SET(SINFO.targets, TAR_CHAR_ROOM) && !SINFO.violent)
		{
			tch = ch;
			target = TRUE;
		}
		if (!target)
		{
			sprintf(buf, "Upon %s should the spell be cast?\r\n",
				IS_SET(SINFO.targets, TAR_OBJ_ROOM | TAR_OBJ_INV | TAR_OBJ_WORLD | TAR_OBJ_EQUIP) ? "what" : "who");
			send_to_char(buf, ch);
			return;
		}
	}
	
	if (target && (tch == ch) && SINFO.violent)
	{
		send_to_char("You shouldn't cast that on yourself -- could be bad for your health!\r\n", ch);
		return;
	}
	if (!target)
	{
		send_to_char("Cannot find the target of your spell!\r\n", ch);
		return;
	}

	if (CASTING_CLASS(ch))
	{
		mana = mag_manacost(ch, spellnum);
		if ((mana > 0) && (GET_MANA(ch) < mana) && IS_MORTAL(ch))
		{
			send_to_char("You haven't the energy to cast that spell!\r\n", ch);
			return;
		}
	}

	/* You throws the dice and you takes your chances.. 101% is total failure */
	if (!success(ch, NULL, spellnum, (oSpell != NULL ? oSpell->percent : 0)))
	{
		WAIT_STATE(ch, PULSE_VIOLENCE);
		
		if (!tch || !skill_message(0, ch, tch, spellnum))
			send_to_char("You lost your concentration!\r\n", ch);

		if (!nospell)
		{
			if (CASTING_CLASS(ch) && mana > 0)
				GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - (mana / 2)));
			else if (MEMMING_CLASS(ch))
				MEMORIZED(ch, spellnum)--;
		}
		
		if (SINFO.violent && tch && IS_NPC(tch))
			hit(tch, ch, TYPE_UNDEFINED);
	}
	else	/* cast spell returns 1 on success; subtract mana & set waitstate */
	{
		if (cast_spell(ch, oSpell, tch, tobj, spellnum))
		{
			WAIT_STATE(ch, PULSE_VIOLENCE);

			if (!nospell)
			{
				if (CASTING_CLASS(ch) && mana > 0)
					GET_MANA(ch) = MAX(0, MIN(GET_MAX_MANA(ch), GET_MANA(ch) - mana));
				else if (MEMMING_CLASS(ch))
					MEMORIZED(ch, spellnum)--;
			}
			/* Room Trigger Events */
			if ( ch->in_room->trigger )
				check_room_trigger(ch, TRIG_ACT_CAST);
		}
	}
}



void spell_level(int spell, int chclass, int level)
{
	int bad = 0;
	
	if (spell < 0 || spell > TOP_SPELL_DEFINE)
	{
		log("SYSERR: attempting assign to illegal spellnum %d/%d", spell, TOP_SPELL_DEFINE);
		return;
	}
	
	if (chclass < 0 || chclass >= NUM_CLASSES)
	{
		log("SYSERR: assigning '%s' to illegal class %d/%d.", skill_name(spell),
			chclass, NUM_CLASSES - 1);
		bad = 1;
	}
	
	if (level < 1 || level > LVL_IMPL)
	{
		log("SYSERR: assigning '%s' to illegal level %d/%d.", skill_name(spell), level, LVL_IMPL);
		bad = 1;
	}
	
	if (!bad)    
		spell_info[spell].min_level[chclass] = level;
}


/* Assign the spells on boot up */
void spello(int spl, const char *name, int max_mana, int min_mana,
	int mana_change, int minpos, int targets, int violent, int routines, const char *wearoff)
{
	int i;
	
	for (i = 0; i < NUM_CLASSES; i++)
		spell_info[spl].min_level[i] = LVL_IMMORT;
	spell_info[spl].mana_max		= max_mana;
	spell_info[spl].mana_min		= min_mana;
	spell_info[spl].mana_change		= mana_change;
	spell_info[spl].min_position	= minpos;
	spell_info[spl].targets			= targets;
	spell_info[spl].violent			= violent;
	spell_info[spl].routines		= routines;
	spell_info[spl].name			= name;
	spell_info[spl].wear_off_msg	= wearoff;
}


void unused_spell(int spl)
{
	int i;
	
	for (i = 0; i < NUM_CLASSES; i++)
		spell_info[spl].min_level[i] = LVL_IMPL + 1;
	spell_info[spl].mana_max		= 0;
	spell_info[spl].mana_min		= 0;
	spell_info[spl].mana_change		= 0;
	spell_info[spl].min_position	= 0;
	spell_info[spl].targets			= 0;
	spell_info[spl].violent			= 0;
	spell_info[spl].routines		= 0;
	spell_info[spl].name			= unused_spellname;
}

#define skillo(skill, name) spello(skill, name, 0, 0, 0, 0, 0, 0, 0, NULL);


/*
 * Arguments for spello calls:
 *
 * spellnum, maxmana, minmana, manachng, minpos, targets, violent?, routines.
 *
 * spellnum:  Number of the spell.  Usually the symbolic name as defined in
 * spells.h (such as SPELL_HEAL).
 *
 * spellname: The name of the spell.
 *
 * maxmana :  The maximum mana this spell will take (i.e., the mana it
 * will take when the player first gets the spell).
 *
 * minmana :  The minimum mana this spell will take, no matter how high
 * level the caster is.
 *
 * manachng:  The change in mana for the spell from level to level.  This
 * number should be positive, but represents the reduction in mana cost as
 * the caster's level increases.
 *
 * minpos  :  Minimum position the caster must be in for the spell to work
 * (usually fighting or standing). targets :  A "list" of the valid targets
 * for the spell, joined with bitwise OR ('|').
 *
 * violent :  TRUE or FALSE, depending on if this is considered a violent
 * spell and should not be cast in PEACEFUL rooms or on yourself.  Should be
 * set on any spell that inflicts damage, is considered aggressive (i.e.
 * charm, curse), or is otherwise nasty.
 *
 * routines:  A list of magic routines which are associated with this spell
 * if the spell uses spell templates.  Also joined with bitwise OR ('|').
 *
 * See the CircleMUD documentation for a more detailed description of these
 * fields.
 */

/*
 * NOTE: SPELL LEVELS ARE NO LONGER ASSIGNED HERE AS OF Circle 3.0 bpl9.
 * In order to make this cleaner, as well as to make adding new classes
 * much easier, spell levels are now assigned in class.c.  You only need
 * a spello() call to define a new spell; to decide who gets to use a spell
 * or skill, look in class.c.  -JE 5 Feb 1996
 */

void mag_assign_spells(void)
{
  int i;

  /* Do not change the loop below. */
  for (i = 0; i <= TOP_SPELL_DEFINE; i++)
    unused_spell(i);
  /* Do not change the loop above. */

  spello(SPELL_ANIMATE_DEAD, "animate dead", 35, 10, 3, POS_STANDING,
	TAR_OBJ_ROOM, FALSE, MAG_SUMMONS,
	NULL);

  spello(SPELL_ARMOR, "armor", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel less protected.");

  spello(SPELL_BLESS, "bless", 35, 5, 3, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV, FALSE, MAG_AFFECTS | MAG_ALTER_OBJS,
	"You feel less righteous.");

  spello(SPELL_BLINDNESS, "blindness", 35, 25, 1, POS_STANDING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, FALSE, MAG_AFFECTS,
	"You feel a cloak of blindness dissolve.");

  spello(SPELL_BURNING_HANDS, "burning hands", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_CALL_LIGHTNING, "call lightning", 40, 25, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_CHARM, "charm person", 75, 50, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_MANUAL,
	"You feel more self-confident.");

  spello(SPELL_CHILL_TOUCH, "chill touch", 30, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_AFFECTS,
	"You feel your strength return.");

  spello(SPELL_CLONE, "clone", 80, 65, 5, POS_STANDING,
	TAR_SELF_ONLY, FALSE, MAG_SUMMONS,
	NULL);

  spello(SPELL_COLOR_SPRAY, "color spray", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_CONTROL_WEATHER, "control weather", 75, 25, 5, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_MANUAL,
	NULL);

  spello(SPELL_CREATE_FOOD, "create food", 30, 5, 4, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_CREATIONS,
	NULL);

  spello(SPELL_CREATE_WATER, "create water", 30, 5, 4, POS_STANDING,
	TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE, MAG_MANUAL,
	NULL);

  spello(SPELL_CURE_BLIND, "cure blind", 30, 5, 2, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_UNAFFECTS,
	NULL);

  spello(SPELL_CURE_CRITIC, "cure critic", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS,
	NULL);

  spello(SPELL_CURE_LIGHT, "cure light", 30, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS,
	NULL);

  spello(SPELL_CURSE, "curse", 80, 50, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV, TRUE, MAG_AFFECTS | MAG_ALTER_OBJS,
	"You feel more optimistic.");

  spello(SPELL_DETECT_ALIGN, "detect alignment", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"You feel less aware.");

  spello(SPELL_DETECT_INVIS, "detect invisibility", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"Your eyes stop tingling.");

  spello(SPELL_DETECT_MAGIC, "detect magic", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"The detect magic wears off.");

  spello(SPELL_DETECT_POISON, "detect poison", 15, 5, 1, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL,
	"The detect poison wears off.");

  spello(SPELL_DISPEL_EVIL, "dispel evil", 40, 25, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_DISPEL_GOOD, "dispel good", 40, 25, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_EARTHQUAKE, "earthquake", 40, 25, 3, POS_FIGHTING,
	TAR_IGNORE, TRUE, MAG_AREAS,
	NULL);

  spello(SPELL_ENCHANT_WEAPON, "enchant weapon", 150, 100, 10, POS_STANDING,
	TAR_OBJ_INV, FALSE, MAG_MANUAL,
	NULL);

  spello(SPELL_ENERGY_DRAIN, "energy drain", 40, 25, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE | MAG_MANUAL,
	NULL);

  spello(SPELL_GROUP_ARMOR, "group armor", 50, 30, 2, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL);

  spello(SPELL_FIREBALL, "fireball", 40, 30, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_GROUP_HEAL, "group heal", 80, 60, 5, POS_STANDING,
	TAR_IGNORE, FALSE, MAG_GROUPS,
	NULL);

  spello(SPELL_HARM, "harm", 75, 45, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_HEAL, "heal", 60, 40, 3, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS | MAG_UNAFFECTS,
	NULL);

  spello(SPELL_INFRAVISION, "infravision", 25, 10, 1, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"Your night vision seems to fade.");

  spello(SPELL_INVISIBLE, "invisibility", 35, 25, 1, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_AFFECTS | MAG_ALTER_OBJS,
	"You feel yourself exposed.");

  spello(SPELL_LIGHTNING_BOLT, "lightning bolt", 30, 15, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_LOCATE_OBJECT, "locate object", 25, 20, 1, POS_STANDING,
	TAR_OBJ_WORLD, FALSE, MAG_MANUAL,
	NULL);

  spello(SPELL_MAGIC_MISSILE, "magic missile", 25, 10, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_POISON, "poison", 50, 20, 3, POS_STANDING,
	TAR_CHAR_ROOM | TAR_NOT_SELF | TAR_OBJ_INV, TRUE,
	MAG_AFFECTS | MAG_ALTER_OBJS,
	"You feel less sick.");

  spello(SPELL_PROT_FROM_EVIL, "protection from evil", 40, 10, 3, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"You feel less protected.");

  spello(SPELL_REMOVE_CURSE, "remove curse", 45, 25, 5, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_EQUIP, FALSE,
	MAG_UNAFFECTS | MAG_ALTER_OBJS,
	NULL);

  spello(SPELL_REMOVE_POISON, "remove poison", 40, 8, 4, POS_STANDING,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_UNAFFECTS | MAG_ALTER_OBJS,
	NULL);

  spello(SPELL_SANCTUARY, "sanctuary", 110, 85, 5, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"The white aura around your body fades.");

  spello(SPELL_SENSE_LIFE, "sense life", 20, 10, 2, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"You feel less aware of your surroundings.");

  spello(SPELL_SHOCKING_GRASP, "shocking grasp", 30, 15, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_SLEEP, "sleep", 40, 25, 5, POS_STANDING,
	TAR_CHAR_ROOM, TRUE, MAG_AFFECTS,
	"You feel less tired.");

  spello(SPELL_STRENGTH, "strength", 35, 30, 1, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel weaker.");

  spello(SPELL_SUMMON, "summon", 75, 50, 3, POS_STANDING,
	TAR_CHAR_WORLD | TAR_NOT_SELF, FALSE, MAG_MANUAL,
	NULL);

  spello(SPELL_TELEPORT, "teleport", 75, 50, 3, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
	NULL);

  spello(SPELL_WATERWALK, "waterwalk", 40, 20, 2, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your feet seem less buoyant.");

  spello(SPELL_WORD_OF_RECALL, "word of recall", 20, 10, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_MANUAL,
	NULL);

  /* new spells */

  spello(SPELL_WALL_OF_FOG, "wall of fog", 50, 25, 5, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_ROOM,
	"The fog seams to clear out.");

  spello(SPELL_ROOM_SHIELD, "room shield", 50, 25, 5, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_ROOM,
	"Vertigo slams into your stomach as the magical shield on the area collapses.");

  spello(SPELL_MINOR_TRACK, "minor track", 30, 10, 5, POS_STANDING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"You lose your tracking ability.");

  spello(SPELL_MAJOR_TRACK, "major track", 50, 20, 5, POS_STANDING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your tracking ability fades away.");

  spello(SPELL_SHIELD, "shield", 30, 10, 4, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"Your magic shield fades away happily.");
  
  spello(SPELL_STONE_SKIN, "stone skin", 50, 20, 3, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"Your skin returns to normal.");
  
  spello(SPELL_PARALYSIS, "paralysis", 35, 25, 1, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_AFFECTS,
	"You feel freedom of movement.");

  spello(SPELL_REFRESH, "refresh", 35, 10, 5, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_POINTS,
	NULL);
  
  spello(SPELL_INCENDIARY_CLOUD, "incendiary cloud", 80, 60, 5, POS_FIGHTING,
	TAR_IGNORE, TRUE, MAG_AREAS,
	NULL);

  spello(SPELL_FIRESHIELD, "fireshield", 60, 40, 2, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_SELF_ONLY, FALSE, MAG_AFFECTS,
	"The red glow around your body fades.");

  spello(SPELL_FLASH, "flash", 50, 35, 5, POS_STANDING,
	TAR_CHAR_ROOM | TAR_NOT_SELF, TRUE, MAG_AFFECTS,
	"You feel a cloak of blindness dissolve.");

  spello(SPELL_ACID_ARROW, "acid arrow", 25, 10, 3, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGED, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_REGENERATION, "regeneration", 40, 20, 2, POS_FIGHTING,
	TAR_CHAR_ROOM, FALSE, MAG_AFFECTS,
	"You feel the extra energy fleeing your body.");

  spello(SPELL_SHOCKWAVE, "shockwave", 40, 25, 2, POS_FIGHTING,
        TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_CHAR_RANGED, TRUE, MAG_DAMAGE,
	NULL);

  spello(SPELL_FEAR, "fear", 50, 25, 5, POS_FIGHTING,
	TAR_CHAR_ROOM | TAR_FIGHT_VICT, TRUE, MAG_MANUAL,
	NULL);

  /* NON-castable spells should appear below here. */

  spello(SPELL_IDENTIFY, "identify", 0, 0, 0, 0,
	TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, FALSE, MAG_MANUAL, NULL);

  /*
   * These spells are currently not used, not implemented, and not castable.
   * Values for the 'breath' spells are filled in assuming a dragon's breath.
   */
  spello(SPELL_FIRE_BREATH, "fire breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0, NULL);

  spello(SPELL_GAS_BREATH, "gas breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0, NULL);

  spello(SPELL_FROST_BREATH, "frost breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0, NULL);

  spello(SPELL_ACID_BREATH, "acid breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0, NULL);

  spello(SPELL_LIGHTNING_BREATH, "lightning breath", 0, 0, 0, POS_SITTING,
	TAR_IGNORE, TRUE, 0, NULL);

  /*
   * Declaration of skills - this actually doesn't do anything except
   * set it up so that immortals can use these skills by default.  The
   * min level to use the skill for other classes is set up in class.c.
   */

  skillo(SKILL_BACKSTAB,			"backstab");
  skillo(SKILL_BASH,				"bash");
  skillo(SKILL_HIDE,				"hide");
  skillo(SKILL_KICK,				"kick");
  skillo(SKILL_PICK_LOCK,			"pick lock");
  skillo(SKILL_RESCUE,				"rescue");
  skillo(SKILL_SNEAK,				"sneak");
  skillo(SKILL_STEAL,				"steal");
  skillo(SKILL_TRACK,				"track");

  /* new skills */
  skillo(SKILL_RIDING,				"riding");
  skillo(SKILL_TAME,				"tame");
  skillo(SKILL_STUDY,				"study");
  skillo(SKILL_READ_MAGIC,			"read magic");
  skillo(SKILL_ENCHANT_ITEMS,		"enchant items");
  skillo(SKILL_PRODUCTION,			"produce items");
  skillo(SKILL_DISARM_TRAPS,		"disarm traps");

  skillo(SKILL_WEAPON_BOW,			"bows");
  skillo(SKILL_WEAPON_SLING,		"slings");
  skillo(SKILL_WEAPON_CROSSBOW,		"crossbows");
  skillo(SKILL_WEAPON_SWORDS,		"swords");
  skillo(SKILL_WEAPON_DAGGERS,		"daggers");
  skillo(SKILL_WEAPON_WHIPS,		"whips");
  skillo(SKILL_WEAPON_TALONOUS_ARMS,"talonous arms");
  skillo(SKILL_WEAPON_BLUDGEONS,	"bludgeons");
  skillo(SKILL_WEAPON_EXOTICS,		"exotics");
}

