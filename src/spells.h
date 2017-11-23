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
*   File: spells.h                                      Part of CircleMUD *
*  Usage: header file: constants and fn prototypes for spell system       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define DEFAULT_STAFF_LVL		12
#define DEFAULT_WAND_LVL		12

#define CAST_UNDEFINED			-1
#define CAST_SPELL				0
#define CAST_POTION				1
#define CAST_WAND				2
#define CAST_STAFF				3
#define CAST_SCROLL				4
#define CAST_OBJ				5

#define MAG_DAMAGE				(1 << 0)
#define MAG_AFFECTS				(1 << 1)
#define MAG_UNAFFECTS			(1 << 2)
#define MAG_POINTS				(1 << 3)
#define MAG_ALTER_OBJS			(1 << 4)
#define MAG_GROUPS				(1 << 5)
#define MAG_MASSES				(1 << 6)
#define MAG_AREAS				(1 << 7)
#define MAG_SUMMONS				(1 << 8)
#define MAG_CREATIONS			(1 << 9)
#define MAG_MANUAL				(1 << 10)
#define MAG_ROOM				(1 << 11)


#define TYPE_UNDEFINED			-1
#define SPELL_RESERVED_DBC		0		/* SKILL NUMBER ZERO -- RESERVED */

/* PLAYER SPELLS -- Numbered from 1 to MAX_SPELLS */

#define SPELL_ARMOR					1
#define SPELL_TELEPORT				2
#define SPELL_BLESS					3
#define SPELL_BLINDNESS				4
#define SPELL_BURNING_HANDS			5
#define SPELL_CALL_LIGHTNING		6
#define SPELL_CHARM					7
#define SPELL_CHILL_TOUCH			8
#define SPELL_CLONE					9
#define SPELL_COLOR_SPRAY			10
#define SPELL_CONTROL_WEATHER		11
#define SPELL_CREATE_FOOD			12
#define SPELL_CREATE_WATER			13
#define SPELL_CURE_BLIND			14
#define SPELL_CURE_CRITIC			15
#define SPELL_CURE_LIGHT			16
#define SPELL_CURSE					17
#define SPELL_DETECT_ALIGN			18
#define SPELL_DETECT_INVIS			19
#define SPELL_DETECT_MAGIC			20
#define SPELL_DETECT_POISON			21
#define SPELL_DISPEL_EVIL			22
#define SPELL_EARTHQUAKE			23
#define SPELL_ENCHANT_WEAPON		24
#define SPELL_ENERGY_DRAIN			25
#define SPELL_FIREBALL				26
#define SPELL_HARM					27
#define SPELL_HEAL					28
#define SPELL_INVISIBLE				29
#define SPELL_LIGHTNING_BOLT		30
#define SPELL_LOCATE_OBJECT			31
#define SPELL_MAGIC_MISSILE			32
#define SPELL_POISON				33
#define SPELL_PROT_FROM_EVIL		34
#define SPELL_REMOVE_CURSE			35
#define SPELL_SANCTUARY				36
#define SPELL_SHOCKING_GRASP		37
#define SPELL_SLEEP					38
#define SPELL_STRENGTH				39
#define SPELL_SUMMON				40
#define SPELL_VENTRILOQUATE			41
#define SPELL_WORD_OF_RECALL		42
#define SPELL_REMOVE_POISON			43
#define SPELL_SENSE_LIFE			44
#define SPELL_ANIMATE_DEAD			45
#define SPELL_DISPEL_GOOD			46
#define SPELL_GROUP_ARMOR			47
#define SPELL_GROUP_HEAL			48
#define SPELL_GROUP_RECALL			49
#define SPELL_INFRAVISION			50
#define SPELL_WATERWALK				51
#define SPELL_WALL_OF_FOG			52
#define SPELL_ROOM_SHIELD			53
#define SPELL_MINOR_TRACK			54
#define SPELL_MAJOR_TRACK			55
#define SPELL_SHIELD				56
#define SPELL_STONE_SKIN			57
#define SPELL_PARALYSIS				58
#define SPELL_REFRESH				59
#define SPELL_INCENDIARY_CLOUD		60
#define SPELL_FIRESHIELD			61
#define SPELL_FLASH					62
#define SPELL_ACID_ARROW			63
#define SPELL_REGENERATION			64
#define SPELL_SHOCKWAVE				65
#define SPELL_FEAR					66

#define LAST_SPELL					66	// REMEMBER TO UPDATE THIS!!!

/* Insert new spells up to MAX_SPELLS  500 */

/* PLAYER SKILLS - Numbered from MAX_SPELLS+1 to MAX_SKILLS */
#define SKILL_BACKSTAB				501
#define SKILL_BASH					502
#define SKILL_HIDE					503
#define SKILL_KICK					504
#define SKILL_PICK_LOCK				505
#define SKILL_NAVIGATION			506
#define SKILL_RESCUE				507
#define SKILL_SNEAK					508
#define SKILL_STEAL					509
#define SKILL_TRACK					510
#define SKILL_RIDING				511
#define SKILL_TAME					512
#define SKILL_STUDY					513
#define SKILL_READ_MAGIC			514
#define SKILL_ENCHANT_ITEMS			515
#define SKILL_PRODUCTION			516
#define SKILL_DISARM_TRAPS			517

/* weapon proficiencies */
#define SKILL_WEAPON_BOW			791
#define SKILL_WEAPON_SLING			792
#define SKILL_WEAPON_CROSSBOW		793
#define SKILL_WEAPON_SWORDS			794
#define SKILL_WEAPON_DAGGERS		795
#define SKILL_WEAPON_WHIPS			796
#define SKILL_WEAPON_TALONOUS_ARMS	797
#define SKILL_WEAPON_BLUDGEONS		798
#define SKILL_WEAPON_EXOTICS		799
/* END -- weapon proficiencies */


/* Insert new skills up to MAX_SKILLS  800 */

/*
 *  NON-PLAYER AND OBJECT SPELLS AND SKILLS
 *  The practice levels for the spells and skills below are _not_ recorded
 *  in the playerfile; therefore, the intended use is for spells and skills
 *  associated with objects (such as SPELL_IDENTIFY used with scrolls of
 *  identify) or non-players (such as NPC-only spells).
 */

#define SPELL_IDENTIFY				801
#define SPELL_FIRE_BREATH			802
#define SPELL_GAS_BREATH			803
#define SPELL_FROST_BREATH			804
#define SPELL_ACID_BREATH			805
#define SPELL_LIGHTNING_BREATH		806

#define TOP_SPELL_DEFINE			899
/* NEW NPC/OBJECT SPELLS can be inserted here up to 899 */


/* WEAPON ATTACK TYPES */
#define NUM_ATTACK_TYPES		15

#define TYPE_HIT				900
#define TYPE_STING				901
#define TYPE_WHIP				902
#define TYPE_SLASH				903
#define TYPE_BITE				904
#define TYPE_BLUDGEON			905
#define TYPE_CRUSH				906
#define TYPE_POUND				907
#define TYPE_CLAW				908
#define TYPE_MAUL				909
#define TYPE_THRASH				910
#define TYPE_PIERCE				911
#define TYPE_BLAST				912
#define TYPE_PUNCH				913
#define TYPE_STAB				914

/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_SUFFERING			999



#define SAVING_PARA				0
#define SAVING_ROD				1
#define SAVING_PETRI			2
#define SAVING_BREATH			3
#define SAVING_SPELL			4


/* Possible Targets:

   bit 0 : IGNORE TARGET
   bit 1 : PC/NPC in room
   bit 2 : PC/NPC in world
   bit 3 : Object held
   bit 4 : Object in inventory
   bit 5 : Object in room
   bit 6 : Object in world
   bit 7 : If fighting, and no argument, select tar_char as self
   bit 8 : If fighting, and no argument, select tar_char as victim (fighting)
   bit 9 : If no argument, select self, if argument check that it IS self.
*/
#define TAR_IGNORE				(1 << 0)
#define TAR_CHAR_ROOM			(1 << 1)
#define TAR_CHAR_WORLD			(1 << 2)
#define TAR_FIGHT_SELF			(1 << 3)
#define TAR_FIGHT_VICT			(1 << 4)
#define TAR_SELF_ONLY			(1 << 5) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_NOT_SELF   			(1 << 6) /* Only a check, use with i.e. TAR_CHAR_ROOM */
#define TAR_OBJ_INV				(1 << 7)
#define TAR_OBJ_ROOM			(1 << 8)
#define TAR_OBJ_WORLD			(1 << 9)
#define TAR_OBJ_EQUIP			(1 << 10)
#define TAR_CHAR_RANGED			(1 << 11)

struct spell_info_type
{
  byte				min_position;				/* Position for caster			*/
  int				mana_min;					/* Min amount of mana used by a spell (highest lev) */
  int				mana_max;					/* Max amount of mana used by a spell (lowest lev) */
  int				mana_change;				/* Change in mana used by spell from lev to lev */
  int				min_level[NUM_CLASSES];
  int				routines;
  byte				violent;
  int				targets;					/* See below for use with TAR_XXX	*/
  const char		*name;
  const char		*wear_off_msg;
};


#define SPELL_TYPE_SPELL		0
#define SPELL_TYPE_POTION		1
#define SPELL_TYPE_WAND			2
#define SPELL_TYPE_STAFF		3
#define SPELL_TYPE_SCROLL		4

/* memorized spells */
#define MAX_SAME_SPELL			5
#define MAX_MEM_SPELLS			50
#define MEM_DELAY				(20 RL_SEC)

typedef struct	memorizing_event	MEM_EVENT;

struct memorizing_event
{
  CHAR_DATA			*ch;
  int				spell;
  sh_int			num;
};


/* Attacktypes with grammar */

struct attack_hit_type
{
  const char		*singular;
  const char		*plural;
};


#define ASPELL(spellname)	void spellname(int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj)

#define MANUAL_SPELL(spellname)	spellname(level, caster, cvict, ovict);

ASPELL(spell_create_water);
ASPELL(spell_recall);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm);
ASPELL(spell_information);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_detect_poison);
ASPELL(spell_fear);

/* basic magic calling functions */

int		mag_damage(int level, CHAR_DATA *ch, CHAR_DATA *victim, int spellnum, int savetype);
void	mag_affects(int level, CHAR_DATA *ch, CHAR_DATA *victim, int spellnum, int savetype);
void	mag_groups(int level, CHAR_DATA *ch, int spellnum, int savetype);
void	mag_masses(int level, CHAR_DATA *ch, int spellnum, int savetype);
void	mag_areas(int level, CHAR_DATA *ch, int spellnum, int savetype);
void	mag_summons(int level, CHAR_DATA *ch, OBJ_DATA *obj, int spellnum, int savetype);
void	mag_points(int level, CHAR_DATA *ch, CHAR_DATA *victim, int spellnum, int savetype);
void	mag_unaffects(int level, CHAR_DATA *ch, CHAR_DATA *victim, int spellnum, int type);
void	mag_alter_objs(int level, CHAR_DATA *ch, OBJ_DATA *obj, int spellnum, int type);
void	mag_creations(int level, CHAR_DATA *ch, int spellnum);
void	mag_objectmagic(CHAR_DATA *ch, OBJ_DATA *obj, char *argument);
void	mag_rooms(int level, CHAR_DATA *ch, int spellnum);

int		find_skill_num(char *name);
int		call_magic(CHAR_DATA *caster, CHAR_DATA *cvict, OBJ_DATA *ovict, int spellnum, int level, int casttype);
int		cast_spell(CHAR_DATA *ch, OBJ_SPELLS_DATA *oSpell, CHAR_DATA *tch, OBJ_DATA *tobj, int spellnum);


/* other prototypes */
void	spell_level(int spell, int chclass, int level);
void	init_spell_levels(void);
char	*get_spell_name(char *argument);
const char *skill_name(int num);
