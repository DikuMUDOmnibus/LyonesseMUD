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
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"

extern int siteok_everyone;

OBJ_DATA *obj_to_char(OBJ_DATA *object, CHAR_DATA *ch);
void create_new_spellbook(CHAR_DATA *ch, int type);

/* local functions */
void snoop_check(CHAR_DATA *ch);
int parse_class(char arg);
long find_class_bitvector(char arg);
byte saving_throws(int class_num, int type, int level);
int thaco(int class_num, int level);
void roll_real_abils(CHAR_DATA *ch);
void do_start(CHAR_DATA *ch);
int backstab_mult(int level);
int invalid_class(CHAR_DATA *ch, OBJ_DATA *obj);
const char *title_male(int chclass, int level);
const char *title_female(int chclass, int level);

/* Names first */
const char *class_abbrevs[] =
{
  "Mu",
  "Cl",
  "Th",
  "Wa",
  "So",
  "\n"
};

const char *race_abbrevs[] =
{
  "Hum",
  "Elf",
  "Dwa",
  "\n"
};

const char *pc_class_types[] =
{
  "Magic User",
  "Cleric",
  "Thief",
  "Warrior",
  "Sorcerer",
  "\n"
};

const char *pc_race_types[] =
{
  "Human",
  "Elf",
  "Dwarf",
  "\n"
};


/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
"\r\n"
"Select a class:\r\n"
"  [M]agic-user\r\n"
"  [C]leric\r\n"
"  [T]hief\r\n"
"  [W]arrior\r\n"
"  [S]orcerer\r\n";

const char *race_menu =
"\r\n"
"Select a race:\r\n"
"  1) Human\r\n"
"  2) Elf\r\n"
"  3) Dwarf\r\n";

/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */
int parse_class(char arg)
{
	arg = LOWER(arg);
	
	switch (arg)
	{
	case 'm': return CLASS_MAGIC_USER;
	case 'c': return CLASS_CLERIC;
	case 't': return CLASS_THIEF;
	case 'w': return CLASS_WARRIOR;
	case 's': return CLASS_SORCERER;
	default:  return CLASS_UNDEFINED;
	}
}


int parse_race(char arg)
{
	switch (arg)
	{
	case '1': return RACE_HUMAN;
	case '2': return RACE_ELF;
	case '3': return RACE_DWARF;
	default : return RACE_UNDEFINED;
	}
}

/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.
 */
long find_class_bitvector(char arg)
{
	arg = LOWER(arg);
	
	switch (arg)
	{
	case 'm': return (1 << CLASS_MAGIC_USER);
	case 'c': return (1 << CLASS_CLERIC);
	case 't': return (1 << CLASS_THIEF);
	case 'w': return (1 << CLASS_WARRIOR);
	case 's': return (1 << CLASS_SORCERER);
	default:  return 0;
	}
}


long find_race_bitvector(char arg)
{
	switch (arg)
	{
	case '1': return (1 << RACE_HUMAN);
	case '2': return (1 << RACE_ELF);
	case '3': return (1 << RACE_DWARF);
	default : return (0);
	}
}

/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 * 
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 * 
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SPELL	0
#define SKILL	1

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] =
{
  /* MAG	CLE	THE	WAR,	SOR */
  { 95,		95,		85,		80,		95	},		/* learned level */
  { 100,	100,	12,		12,		100	},		/* max per practice */
  { 25,		25,		0,		0,		25	},		/* min per practice */
  { SPELL,	SPELL,	SKILL,	SKILL,	SPELL	}	/* prac name */
};


/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.c if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */
int guild_info[][3] =
{
/* Midgaard */
  {CLASS_MAGIC_USER,	3017,	SCMD_SOUTH},
  {CLASS_SORCERER,		3017,	SCMD_SOUTH},
  {CLASS_CLERIC,		3004,	SCMD_NORTH},
  {CLASS_THIEF,			3027,	SCMD_EAST},
  {CLASS_WARRIOR,		3021,	SCMD_EAST},

/* Brass Dragon */
  {-999 /* all */ ,		5065,	SCMD_WEST},

/* this must go last -- add new guards above! */
  {-1, -1, -1}
};


/* Order of saving throws: PARA, ROD, PETRI, BREATH, SPELL */
/* This matrix determines the base saving throw for each   */
/* class (i.e. the saving throw needed at level 1).        */
int saving_throw_base[NUM_CLASSES][5] =
{
  { 70, 55, 65, 75, 60 },	/* MU */
  { 60, 70, 65, 80, 75 },	/* CL */
  { 65, 70, 60, 80, 75 },	/* TH */
  { 70, 80, 75, 85, 85 },	/* WA */
  { 70, 55, 65, 75, 60 }	/* SO */
};        

/* This is the rate at which saving throws improve, *10 */
/* So, if a saving throw improves at a rate of 1.4 per  */
/* level, it is entered into this matrix as -14.        */
int saving_throw_improve[NUM_CLASSES][5] =
{
  { -14, -16, -17, -17, -17 },	/* MU */
  { -16, -15, -15, -15, -15 },	/* CL */
  { -10, -20, -10, -10, -20 },	/* TH */
  { -21, -21, -21, -25, -21 },	/* WA */
  { -14, -16, -17, -17, -17 }	/* SO */
};

/*
 * Saving throws for:
 * MCTW
 *   PARA, ROD, PETRI, BREATH, SPELL
 *     Levels 0-40
 *
 * Do not forget to change extern declaration in magic.c if you add to this.
 */

byte saving_throws(int class_num, int type, int level)
{
	float mod;
	
	mod = (saving_throw_improve[class_num][type] / 10.0) * (level - 1);
	
	return (saving_throw_base[class_num][type] + mod);
}

/* THAC0 for classes and levels.  (To Hit Armor Class 0) */
int thaco(int class_num, int level)
{
	int t = 100;			/* Default THAC0 */

	if (level > 0)
	{
		switch (class_num)
		{
		case CLASS_MAGIC_USER:	t = 20 - ((level - 1) / 3);			break;
		case CLASS_CLERIC:		t = 20 - (((level - 1) / 3) * 2);	break;
		case CLASS_THIEF:		t = 20 - ((level - 1) / 2);			break;
		case CLASS_WARRIOR:		t = 20 - (level - 1);				break;
		case CLASS_SORCERER:	t = 20 - ((level - 1) / 3);			break;
		default:
			log("SYSERR: Unknown class in thac0 chart.");			break;
		}
	}

	if (t < 1) /* Nobody can have a THAC0 better than 1! */
		t = 1;

	return (t);
}


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(CHAR_DATA *ch)
{
	int i, j, k, temp;
	ubyte table[6];
	ubyte rolls[4];
	
	for (i = 0; i < 6; i++)
		table[i] = 0;
	
	for (i = 0; i < 6; i++)
	{
		for (j = 0; j < 4; j++)
			rolls[j] = number(1, 6);
		
		temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
			MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));
		
		for (k = 0; k < 6; k++)
			if (table[k] < temp)
			{
				temp ^= table[k];
				table[k] ^= temp;
				temp ^= table[k];
			}
	}
	
	switch (GET_CLASS(ch))
	{
	case CLASS_MAGIC_USER:
		ch->real_abils.intel	= table[0];
		ch->real_abils.wis		= table[1];
		ch->real_abils.dex		= table[2];
		ch->real_abils.str		= table[3];
		ch->real_abils.con		= table[4];
		ch->real_abils.cha		= table[5];
		break;
	case CLASS_CLERIC:
		ch->real_abils.wis		= table[0];
		ch->real_abils.intel	= table[1];
		ch->real_abils.str		= table[2];
		ch->real_abils.dex		= table[3];
		ch->real_abils.con		= table[4];
		ch->real_abils.cha		= table[5];
		break;
	case CLASS_THIEF:
		ch->real_abils.dex		= table[0];
		ch->real_abils.str		= table[1];
		ch->real_abils.con		= table[2];
		ch->real_abils.intel	= table[3];
		ch->real_abils.wis		= table[4];
		ch->real_abils.cha		= table[5];
		break;
	case CLASS_WARRIOR:
		ch->real_abils.str		= table[0];
		ch->real_abils.dex		= table[1];
		ch->real_abils.con		= table[2];
		ch->real_abils.wis		= table[3];
		ch->real_abils.intel	= table[4];
		ch->real_abils.cha		= table[5];
		break;
	case CLASS_SORCERER:
		ch->real_abils.intel	= table[0];
		ch->real_abils.dex		= table[1];
		ch->real_abils.str		= table[2];
		ch->real_abils.wis		= table[3];
		ch->real_abils.con		= table[4];
		ch->real_abils.cha		= table[5];
	}

	switch (GET_RACE(ch))
	{
	case RACE_HUMAN:
		break;
	case RACE_ELF:
		ch->real_abils.dex += 1;
		ch->real_abils.con -= 1;
		break;
	case RACE_DWARF:
		ch->real_abils.con += 2;
		ch->real_abils.intel -= 1;
		ch->real_abils.cha -= 1;
		break;
	}
	
	ch->aff_abils = ch->real_abils;
}


/* Some initializations for characters, including initial skills */
void do_start(CHAR_DATA *ch)
{
	OBJ_DATA *obj;
	int i;
	//obj_vnum give_obj[] = {2544, 5423, 3071, 3076, 3081, 3086, 6002, 7211, 3104, 3015, 3015, -1};
	obj_vnum give_obj[] = {3015, 3015, -1};

	GET_LEVEL(ch)		= 1;
	GET_TOT_LEVEL(ch)	= 1;
	GET_EXP(ch)			= 1;
	
	set_title(ch, NULL);
	
	switch (GET_CLASS(ch))
	{	
	case CLASS_MAGIC_USER:
		SET_SKILL(ch, SKILL_STUDY, 15);
		SET_SKILL(ch, SKILL_READ_MAGIC, 10);
		break;

	case CLASS_CLERIC:
		SET_SKILL(ch, SKILL_STUDY, 10);
		SET_SKILL(ch, SKILL_READ_MAGIC, 10);
		break;

	case CLASS_WARRIOR:
		break;
		
	case CLASS_THIEF:
		SET_SKILL(ch, SKILL_SNEAK, 10);
		SET_SKILL(ch, SKILL_HIDE, 5);
		SET_SKILL(ch, SKILL_STEAL, 15);
		SET_SKILL(ch, SKILL_BACKSTAB, 10);
		SET_SKILL(ch, SKILL_PICK_LOCK, 10);
		SET_SKILL(ch, SKILL_TRACK, 10);
		break;

	case CLASS_SORCERER:
		SET_SKILL(ch, SKILL_STUDY, 10);
		SET_SKILL(ch, SKILL_READ_MAGIC, 15);
		create_new_spellbook(ch, BOOK_TRAVELLING);
		break;
	}
	
	advance_level(ch);
	sprintf(buf, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
	mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
	
	GET_HIT(ch)				= GET_MAX_HIT(ch);
	GET_MANA(ch)			= GET_MAX_MANA(ch);
	GET_MOVE(ch)			= GET_MAX_MOVE(ch);
	
	GET_COND(ch, THIRST)	= 24;
	GET_COND(ch, FULL)		= 24;
	GET_COND(ch, DRUNK)		= 0;
	
	ch->player.time.played	= 0;
	ch->player.time.logon	= time(0);
	
	if (siteok_everyone)
		SET_BIT(PLR_FLAGS(ch), PLR_SITEOK);

	SET_BIT(PRF_FLAGS(ch), PRF_COLOR_1);
	SET_BIT(PRF_FLAGS(ch), PRF_COLOR_2);

	/* give them something to start with */
	for (i = 0; give_obj[i] != -1; i++)
	{
		obj = read_object(give_obj[i], VIRTUAL);
		obj_to_char(obj, ch);
	}
}



/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void advance_level(CHAR_DATA *ch)
{
	int add_hp, add_mana = 0, add_move = 0, i;
	
	add_hp = con_app[GET_CON(ch)].hitp;
	
	switch (GET_CLASS(ch))
	{
	case CLASS_MAGIC_USER:
		add_hp		+= number(3, 8);
		add_mana	= number(GET_LEVEL(ch), 1.5 * GET_LEVEL(ch));
		add_mana	= MIN(add_mana, 10);
		add_move	= number(0, 3);
		break;
		
	case CLASS_CLERIC:
		add_hp		+= number(5, 10);
		add_mana	= number(GET_LEVEL(ch), 1.5 * GET_LEVEL(ch));
		add_mana	= MIN(add_mana, 10);
		add_move	= number(2, 5);
		break;
		
	case CLASS_THIEF:
		add_hp		+= number(7, 13);
		add_mana	= 0;
		add_move	= number(3, 6);
		break;
		
	case CLASS_WARRIOR:
		add_hp		+= number(10, 15);
		add_mana	= 0;
		add_move	= number(3, 5);
		break;

	case CLASS_SORCERER:
		add_hp		+= number(4, 9);
		add_mana	= 0;
		add_move	= number(1, 4);
		break;
	}
	
	GET_MAX_HIT(ch)		+= MAX(1, add_hp);
	GET_MAX_MOVE(ch)	+= MAX(1, add_move);
	GET_MAX_MANA(ch)	+= add_mana;
	
	if (IS_MAGIC_USER(ch) || IS_CLERIC(ch) || IS_SORCERER(ch))
		GET_PRACTICES(ch) += MAX(2, wis_app[GET_WIS(ch)].bonus);
	else
		GET_PRACTICES(ch) += MIN(2, MAX(1, wis_app[GET_WIS(ch)].bonus));
	
	if (GET_LEVEL(ch) >= LVL_IMMORT)
	{
		for (i = 0; i < 3; i++)
			GET_COND(ch, i) = (char) -1;
		SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
	}

	snoop_check(ch);
	save_char(ch, NULL);
}


/*
 * This simply calculates the backstab multiplier based on a character's
 * level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
int backstab_mult(int level)
{
	if		(level <= 0)			return (1);	  /* level 0 */
	else if (level <= 7)			return (2);	  /* level 1 - 7 */
	else if (level <= 13)			return (3);	  /* level 8 - 13 */
	else if (level <= 20)			return (4);	  /* level 14 - 20 */
	else if (level <= 28)			return (5);	  /* level 21 - 28 */
	else if (level < LVL_IMMORT)	return (6);	  /* all remaining mortal levels */
	else							return (20);  /* immortals */
}


/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */
int invalid_class(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (IS_SET(GET_OBJ_ANTI(obj), ITEM_ANTI_MAGIC_USER) && IS_MAGIC_USER(ch))
		return (TRUE);
	
	if (IS_SET(GET_OBJ_ANTI(obj), ITEM_ANTI_CLERIC) && IS_CLERIC(ch))
		return (TRUE);
	
	if (IS_SET(GET_OBJ_ANTI(obj), ITEM_ANTI_THIEF) && IS_THIEF(ch))
		return (TRUE);
	
	if (IS_SET(GET_OBJ_ANTI(obj), ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch))
		return (TRUE);
	
	if (IS_SET(GET_OBJ_ANTI(obj), ITEM_ANTI_SORCERER) && IS_SORCERER(ch))
		return (TRUE);
	
	return (FALSE);
}


int invalid_race(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (IS_SET(GET_OBJ_ANTI(obj), ITEM_ANTI_HUMAN) && IS_HUMAN(ch))
		return (TRUE);
	
	if (IS_SET(GET_OBJ_ANTI(obj), ITEM_ANTI_ELF) && IS_ELF(ch))
		return (TRUE);
	
	if (IS_SET(GET_OBJ_ANTI(obj), ITEM_ANTI_DWARF) && IS_DWARF(ch))
		return (TRUE);

	return (FALSE);
}

/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */
void init_spell_levels(void)
{
	/* MAGES */
	spell_level(SPELL_MAGIC_MISSILE,		CLASS_MAGIC_USER, 1);
	spell_level(SPELL_SHIELD,				CLASS_MAGIC_USER, 1);
	spell_level(SPELL_DETECT_INVIS,			CLASS_MAGIC_USER, 2);
	spell_level(SPELL_DETECT_MAGIC,			CLASS_MAGIC_USER, 2);
	spell_level(SPELL_CHILL_TOUCH,			CLASS_MAGIC_USER, 3);
	spell_level(SPELL_INFRAVISION,			CLASS_MAGIC_USER, 3);
	spell_level(SPELL_INVISIBLE,			CLASS_MAGIC_USER, 4);
	spell_level(SPELL_ARMOR,				CLASS_MAGIC_USER, 4);
	spell_level(SPELL_BURNING_HANDS,		CLASS_MAGIC_USER, 5);
	spell_level(SKILL_RIDING,				CLASS_MAGIC_USER, 5);
	spell_level(SPELL_LOCATE_OBJECT,		CLASS_MAGIC_USER, 6);
	spell_level(SPELL_STRENGTH,				CLASS_MAGIC_USER, 6);
	spell_level(SPELL_SHOCKING_GRASP,		CLASS_MAGIC_USER, 7);
	spell_level(SPELL_SLEEP,				CLASS_MAGIC_USER, 8);
	spell_level(SPELL_LIGHTNING_BOLT,		CLASS_MAGIC_USER, 9);
	spell_level(SPELL_BLINDNESS,			CLASS_MAGIC_USER, 9);
	spell_level(SPELL_DETECT_POISON,		CLASS_MAGIC_USER, 10);
	spell_level(SPELL_COLOR_SPRAY,			CLASS_MAGIC_USER, 11);
	spell_level(SPELL_ACID_ARROW,   		CLASS_MAGIC_USER, 12);
	spell_level(SPELL_ENERGY_DRAIN,			CLASS_MAGIC_USER, 13);
	spell_level(SPELL_CURSE,				CLASS_MAGIC_USER, 14);
	spell_level(SPELL_POISON,				CLASS_MAGIC_USER, 14);
	spell_level(SPELL_FIREBALL,				CLASS_MAGIC_USER, 15);
	spell_level(SPELL_CHARM,				CLASS_MAGIC_USER, 16);
	spell_level(SPELL_FLASH,				CLASS_MAGIC_USER, 18);
	spell_level(SPELL_PARALYSIS,			CLASS_MAGIC_USER, 20);
	spell_level(SPELL_CLONE,				CLASS_MAGIC_USER, 22);
	spell_level(SPELL_STONE_SKIN,    		CLASS_MAGIC_USER, 25);
	spell_level(SPELL_SHOCKWAVE,			CLASS_MAGIC_USER, 27);
	spell_level(SPELL_INCENDIARY_CLOUD,		CLASS_MAGIC_USER, 30);
	
	/* CLERICS */
	spell_level(SPELL_CURE_LIGHT,			CLASS_CLERIC, 1);
	spell_level(SPELL_ARMOR,				CLASS_CLERIC, 1);
	spell_level(SPELL_CREATE_FOOD,			CLASS_CLERIC, 2);
	spell_level(SPELL_CREATE_WATER,			CLASS_CLERIC, 2);
	spell_level(SPELL_DETECT_POISON,		CLASS_CLERIC, 3);
	spell_level(SPELL_DETECT_ALIGN,			CLASS_CLERIC, 4);
	spell_level(SPELL_CURE_BLIND,			CLASS_CLERIC, 4);
	spell_level(SPELL_BLESS,				CLASS_CLERIC, 5);
	spell_level(SKILL_RIDING,				CLASS_CLERIC, 5);
	spell_level(SPELL_DETECT_INVIS,			CLASS_CLERIC, 6);
	spell_level(SPELL_BLINDNESS,			CLASS_CLERIC, 6);
	spell_level(SPELL_INFRAVISION,			CLASS_CLERIC, 7);
	spell_level(SPELL_PROT_FROM_EVIL,		CLASS_CLERIC, 8);
	spell_level(SPELL_POISON,				CLASS_CLERIC, 8);
	spell_level(SPELL_GROUP_ARMOR,			CLASS_CLERIC, 9);
	spell_level(SPELL_CURE_CRITIC,			CLASS_CLERIC, 9);
	spell_level(SPELL_SUMMON,				CLASS_CLERIC, 10);
	spell_level(SPELL_REMOVE_POISON,		CLASS_CLERIC, 10);
	spell_level(SPELL_REGENERATION,			CLASS_CLERIC, 10);
	spell_level(SPELL_WORD_OF_RECALL,		CLASS_CLERIC, 12);
	spell_level(SPELL_EARTHQUAKE,			CLASS_CLERIC, 12);
	spell_level(SPELL_FLASH,				CLASS_CLERIC, 13);
	spell_level(SPELL_DISPEL_EVIL,			CLASS_CLERIC, 14);
	spell_level(SPELL_DISPEL_GOOD,			CLASS_CLERIC, 14);
	spell_level(SPELL_SANCTUARY,			CLASS_CLERIC, 15);
	spell_level(SPELL_CALL_LIGHTNING,		CLASS_CLERIC, 15);
	spell_level(SPELL_HEAL,					CLASS_CLERIC, 16);
	spell_level(SPELL_CONTROL_WEATHER,		CLASS_CLERIC, 17);
	spell_level(SPELL_SENSE_LIFE,			CLASS_CLERIC, 18);
	spell_level(SPELL_HARM,					CLASS_CLERIC, 19);
	spell_level(SPELL_GROUP_HEAL,			CLASS_CLERIC, 22);
	spell_level(SPELL_REMOVE_CURSE,			CLASS_CLERIC, 26);
	
	/* THIEVES */
	spell_level(SKILL_SNEAK,				CLASS_THIEF, 1);
	spell_level(SKILL_PICK_LOCK,			CLASS_THIEF, 2);
	spell_level(SKILL_BACKSTAB,				CLASS_THIEF, 3);
	spell_level(SKILL_STEAL,				CLASS_THIEF, 4);
	spell_level(SKILL_HIDE,					CLASS_THIEF, 5);
	spell_level(SKILL_RIDING,				CLASS_THIEF, 6);
	spell_level(SKILL_TRACK,				CLASS_THIEF, 7);
	spell_level(SKILL_TAME,					CLASS_THIEF, 8);
	spell_level(SKILL_DISARM_TRAPS,			CLASS_THIEF, 9);
	
	/* WARRIORS */
	spell_level(SKILL_KICK,					CLASS_WARRIOR, 1);
	spell_level(SKILL_RESCUE,				CLASS_WARRIOR, 3);
	spell_level(SKILL_RIDING,				CLASS_WARRIOR, 5);
	spell_level(SKILL_TAME,					CLASS_WARRIOR, 7);
	spell_level(SKILL_TRACK,				CLASS_WARRIOR, 9);
	spell_level(SKILL_BASH,					CLASS_WARRIOR, 12);
	spell_level(SKILL_WEAPON_BOW,			CLASS_WARRIOR, PROF_LVL_START);
	spell_level(SKILL_WEAPON_SLING,			CLASS_WARRIOR, PROF_LVL_START);
	spell_level(SKILL_WEAPON_CROSSBOW,		CLASS_WARRIOR, PROF_LVL_START);
	spell_level(SKILL_WEAPON_SWORDS,        CLASS_WARRIOR, PROF_LVL_START);
	spell_level(SKILL_WEAPON_DAGGERS,       CLASS_WARRIOR, PROF_LVL_START);
	spell_level(SKILL_WEAPON_WHIPS,         CLASS_WARRIOR, PROF_LVL_START);
	spell_level(SKILL_WEAPON_TALONOUS_ARMS, CLASS_WARRIOR, PROF_LVL_START);
	spell_level(SKILL_WEAPON_BLUDGEONS,     CLASS_WARRIOR, PROF_LVL_START);
	spell_level(SKILL_WEAPON_EXOTICS,       CLASS_WARRIOR, PROF_LVL_START);

	/* SORCERERS */
	spell_level(SPELL_MAGIC_MISSILE,		CLASS_SORCERER, 1);
	spell_level(SPELL_DETECT_INVIS,			CLASS_SORCERER, 2);
	spell_level(SPELL_DETECT_MAGIC,			CLASS_SORCERER, 2);
	spell_level(SPELL_CHILL_TOUCH,			CLASS_SORCERER, 3);
	spell_level(SPELL_INFRAVISION,			CLASS_SORCERER, 3);
	spell_level(SPELL_INVISIBLE,			CLASS_SORCERER, 4);
	spell_level(SPELL_ARMOR,				CLASS_SORCERER, 4);
	spell_level(SPELL_BURNING_HANDS,		CLASS_SORCERER, 5);
	spell_level(SKILL_RIDING,				CLASS_SORCERER, 5);
	spell_level(SPELL_LOCATE_OBJECT,		CLASS_SORCERER, 6);
	spell_level(SPELL_STRENGTH,				CLASS_SORCERER, 6);
	spell_level(SPELL_SHOCKING_GRASP,		CLASS_SORCERER, 7);
	spell_level(SPELL_SLEEP,				CLASS_SORCERER, 8);
	spell_level(SPELL_LIGHTNING_BOLT,		CLASS_SORCERER, 9);
	spell_level(SPELL_DETECT_POISON,		CLASS_SORCERER, 10);
	spell_level(SPELL_COLOR_SPRAY,			CLASS_SORCERER, 11);
	spell_level(SPELL_CURSE,				CLASS_SORCERER, 14);
	spell_level(SPELL_POISON,				CLASS_SORCERER, 14);
	spell_level(SPELL_FIREBALL,				CLASS_SORCERER, 15);
	spell_level(SPELL_BLINDNESS,			CLASS_SORCERER, 16);
	spell_level(SPELL_PARALYSIS,			CLASS_SORCERER, 18);
	spell_level(SPELL_ENCHANT_WEAPON,		CLASS_SORCERER, 20);
	spell_level(SKILL_ENCHANT_ITEMS,		CLASS_SORCERER, 22);
	spell_level(SPELL_FIRESHIELD,			CLASS_SORCERER, 25);
	spell_level(SPELL_SHOCKWAVE,			CLASS_SORCERER, 27);
}


/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */
#define EXP_MAX  10000000

/* 
 * Default titles of male characters.
 */
const char *title_male(int chclass, int level)
{
	if (level <= 0 || level > LVL_IMPL)
		return ("the Man");
	if (level == LVL_IMPL)
		return ("the Implementor");

	switch (chclass)
	{
	case CLASS_MAGIC_USER:
		if		(level < 10)	return ("the Apprentice of Magic");
		else if	(level < 20)	return ("the Wizard");
		else if (level < 30)	return ("the Archmage");
		else					return ("the Immortal Archmage");
		break;

	case CLASS_CLERIC:
		if		(level < 10)	return ("the Acolyte");
		else if	(level < 20)	return ("the Priest");
		else if (level < 30)	return ("the Bishop");
		else					return ("the Immortal Patriarch");
		break;

	case CLASS_THIEF:
		if		(level < 10)	return ("the Cut-Purse");
		else if	(level < 20)	return ("the Rogue");
		else if (level < 30)	return ("the Thief");
		else					return ("the Immortal Thief");
		break;

	case CLASS_WARRIOR:
		if		(level < 10)	return ("the Fighter");
		else if	(level < 20)	return ("the Warrior");
		else if (level < 30)	return ("the Swordmaster");
		else					return ("the Immortal Swordmaster");
		break;

	case CLASS_SORCERER:
		if		(level < 10)	return ("the Spell Student");
		else if	(level < 20)	return ("the Magician");
		else if (level < 30)	return ("the Sorcerer");
		else					return ("the Immortal Sorcerer");
		break;
	}

	/* Default title for classes which do not have titles defined */
	return ("the Classless");
}


/* 
 * Default titles of female characters.
 */
const char *title_female(int chclass, int level)
{
	if (level <= 0 || level > LVL_IMPL)
		return ("the Woman");
	if (level == LVL_IMPL)
		return ("the Implementress");

	switch (chclass)
	{
	case CLASS_MAGIC_USER:
		if		(level < 10)	return ("the Apprentice of Magic");
		else if	(level < 20)	return ("the Witch");
		else if (level < 30)	return ("the Archwitch");
		else					return ("the Immortal Archwitch");
		break;

	case CLASS_CLERIC:
		if		(level < 10)	return ("the Acolyte");
		else if	(level < 20)	return ("the Priestess");
		else if (level < 30)	return ("the Bishop");
		else					return ("the Immortal Priestess");
		break;

	case CLASS_THIEF:
		if		(level < 10)	return ("the Cut-Purse");
		else if	(level < 20)	return ("the Rogue");
		else if (level < 30)	return ("the Thief");
		else					return ("the Immortal Thief");
		break;

	case CLASS_WARRIOR:
		if		(level < 10)	return ("the Fighter");
		else if	(level < 20)	return ("the Warrior");
		else if (level < 30)	return ("the Swordmistress");
		else					return ("the Immortal Swordmistress");
		break;

	case CLASS_SORCERER:
		if		(level < 10)	return ("the Spell Student");
		else if	(level < 20)	return ("the Magician");
		else if (level < 30)	return ("the Sorceress");
		else					return ("the Immortal Sorceress");
		break;
	}

	/* Default title for classes which do not have titles defined */
	return "the Classless";
}

/* ********************************************************************* */
/* Remort Code                                                           */
/* ********************************************************************* */

/* Multi message - modify this message to set multi message */
char *multi_message =
  "You have multied, and must begin anew. However, you will retain knowledge\r\n"
  "of your previous skills and spells, you just will not be able to use them\r\n"
  "until you reach the appropriate level.\r\n";

int can_multi(CHAR_DATA *ch, int newclass)
{
	/* Is wanted class same as current class? */
	if (newclass == GET_CLASS(ch))
	{
		sprintf(buf, "But you are a %s!!!\r\n", pc_class_types[(int)GET_CLASS(ch)]);
		send_to_char(buf, ch);
		return (FALSE);
	}
	
	/* Has char already completed this class? */
	if (MULTICLASSED(ch, (1 << newclass)))
	{
		send_to_char("You can not repeat a class already completed.\r\n", ch);
		return (FALSE);
	}
	
	if (GET_LEVEL(ch) < MULTI_LEVEL)
	{
		sprintf(buf, "You are only level %d, you must be at least level %d before you can multi.\r\n",
			GET_LEVEL(ch), MULTI_LEVEL);
		send_to_char(buf, ch);
		return (FALSE);
	}
	
	/* Is the char in the appropriate room? */
	if (MULTI_ROOM != NOWHERE)
	{
		ROOM_DATA *pRoom;
		
		if ((pRoom = get_room(MULTI_ROOM)) && ch->in_room != pRoom)
		{
			send_to_char("You are not in the correct room to multi!\r\n", ch);
			return (FALSE);
		}
	}
	
	return (TRUE);
}

ACMD(do_remort)
{
	int newclass;
	
	one_argument(argument, arg);
	
	if (!*arg)
	{
		send_to_char(
			"Usage: multi <class name>\r\n"
			"Valid classes names are:\r\n"
			"[M]agic User\r\n"
			"[C]leric\r\n"
			"[T]hief\r\n"
			"[W]arrior\r\n"
			"[S]orcerer\r\n", ch);
		return;
	}
	
	if ((newclass = parse_class(arg[0])) < 0)
	{
		send_to_char("Improper class name, please try again.\r\n", ch);
		return;
	}
	
	if (!can_multi(ch, newclass))
		return;
	
	SET_BIT(MULTICLASS(ch), (1 << GET_CLASS(ch)));
	GET_TOT_LEVEL(ch)	+= 1;
	GET_LEVEL(ch)		= 1;
	GET_CLASS(ch)		= newclass;
	GET_EXP(ch)			= 1;

	GET_MAX_HIT(ch)		= 10;
	GET_MAX_MANA(ch)	= 100;
	GET_MAX_MOVE(ch)	= 80;

	set_title(ch, NULL);

	switch (GET_CLASS(ch))
	{	
	case CLASS_MAGIC_USER:
		if (!GET_SKILL(ch, SKILL_STUDY))
			SET_SKILL(ch, SKILL_STUDY, 15);
		if (!GET_SKILL(ch, SKILL_READ_MAGIC))
			SET_SKILL(ch, SKILL_READ_MAGIC, 10);
		break;

	case CLASS_CLERIC:
		if (!GET_SKILL(ch, SKILL_STUDY))
			SET_SKILL(ch, SKILL_STUDY, 10);
		if (!GET_SKILL(ch, SKILL_READ_MAGIC))
			SET_SKILL(ch, SKILL_READ_MAGIC, 10);
		break;

	case CLASS_WARRIOR:
		break;
		
	case CLASS_THIEF:
		if (!GET_SKILL(ch, SKILL_SNEAK))
			SET_SKILL(ch, SKILL_SNEAK, 10);
		if (!GET_SKILL(ch, SKILL_HIDE))
			SET_SKILL(ch, SKILL_HIDE, 5);
		if (!GET_SKILL(ch, SKILL_STEAL))
			SET_SKILL(ch, SKILL_STEAL, 15);
		if (!GET_SKILL(ch, SKILL_BACKSTAB))
			SET_SKILL(ch, SKILL_BACKSTAB, 10);
		if (!GET_SKILL(ch, SKILL_PICK_LOCK))
			SET_SKILL(ch, SKILL_PICK_LOCK, 10);
		if (!GET_SKILL(ch, SKILL_TRACK))
			SET_SKILL(ch, SKILL_TRACK, 10);
		break;

	case CLASS_SORCERER:
		if (!GET_SKILL(ch, SKILL_STUDY))
			SET_SKILL(ch, SKILL_STUDY, 10);
		if (!GET_SKILL(ch, SKILL_READ_MAGIC))
			SET_SKILL(ch, SKILL_READ_MAGIC, 15);
		create_new_spellbook(ch, BOOK_TRAVELLING);
		break;
	}

	advance_level(ch);
	sprintf(buf, "%s remorted to level %d", GET_NAME(ch), GET_LEVEL(ch));
	mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
	send_to_char(multi_message, ch);
}

/*
 * this is just a rough layout of the exp needed for a ch
 * to level. recommend you change the modifier, and also
 * the 4000 to something that suits you better.
 */
 
/* Nic Suzor's Exp Formula */
int exp_to_level(int level)
{
	int modifier;
	
	if (level == 1)
		return (0);
	else if (level < 6)		modifier = 1;
	else if (level < 9)		modifier = 2;
	else if (level < 11)	modifier = 5;
	else if (level < 13)	modifier = 6;
	else if (level < 15)	modifier = 7;
	else					modifier = 8;  
	
	return (MIN(20, level - 1) * modifier * 3400);
}

int tot_exp_to_level(int level)
{
	int exp, lvl;

	for (exp = 0, lvl = 1; lvl <= level; lvl++)
		exp += exp_to_level(lvl);

	return (exp);
}
