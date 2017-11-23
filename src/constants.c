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
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "interpreter.h"	/* alias_data */

cpp_extern const char *circlemud_version =
	"CircleMUD, version 3.00 beta patchlevel 21";

/* strings corresponding to ordinals/bitvectors in structs.h ***********/


/* (Note: strings for class definitions in class.c instead of here) */


/* cardinal directions */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northeast",
  "southeast",
  "southwest",
  "northwest",
  "\n"
};

int rev_dir[] =
{
  2,
  3,
  0,
  1,
  5,
  4,
  8,    // southwest
  9,    // northwest
  6,    // northeast
  7     // southeast
};


const char *exits_color[] =
{
  "&b&2",		// NORTH
  "&b&3",		// EAST
  "&b&4",		// SOUTH
  "&b&1",		// WEST
  "&b&6",		// UP
  "&b&7",		// DOWN
  "&b&2",		// NORTHEAST
  "&b&3",		// SOUTHEAST
  "&b&4",		// SOUTHWEST
  "&b&1"		// NORTHWEST
};


/* ROOM_x */
const char *room_bits[] =
{
  "DARK",
  "DEATH",
  "NO_MOB",
  "INDOORS",
  "PEACEFUL",
  "SOUNDPROOF",
  "NO_TRACK",
  "NO_MAGIC",
  "TUNNEL",
  "PRIVATE",
  "GODROOM",
  "HOUSE",
  "HCRSH",
  "ATRIUM",
  "OLC",
  "*",				/* BFS MARK */
  "WILD_STATIC",
  "WILD_REMOVE",
  "SAVEROOM",
  "BUILDING_WORKS",
  "\n"
};

/* RAFF_x */
const char *room_affections[] =
{
  "FOG",
  "SHIELD",
  "CAMPSITE",
  "\n"
};

/* EX_x */
const char *exit_bits[] =
{
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "FALSE",
  "HIDDEN",
  "\n"
};


/*
 * SEX_x
 */
const char *genders[] =
{
  "neutral",
  "male",
  "female",
  "\n"
};


/* POS_x */
const char *position_types[] =
{
  "dead",
  "mortally wounded",
  "incapacitated",
  "stunned",
  "sleeping",
  "resting",
  "sitting",
  "fighting",
  "standing",
  "\n"
};


/* PLR_x */
const char *player_bits[] =
{
  "KILLER",
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "NO_WIZL",
  "NO_DEL",
  "INVST",
  "CRYO",
  "DEAD",    /* You should never see this. */
  "GHOST",
  "SAVEALL",
  "\n"
};


/* MOB_x */
const char *action_bits[] =
{
  "SPEC",
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "AWARE",
  "AGGR",
  "STAY-ZONE",
  "WIMPY",
  "AGGR_EVIL",
  "AGGR_GOOD",
  "AGGR_NEUTRAL",
  "MEMORY",
  "HELPER",
  "NO_CHARM",
  "NO_SUMMN",
  "NO_SLEEP",
  "NO_BASH",
  "NO_BLIND",
  "DEAD",    /* You should never see this. */
  "MOUNTABLE",
  "HUNTER",
  "DRAFT_ANIMAL",
  "ENCOUNTER",
  "\n"
};


/* PRF_x */
const char *preference_bits[] =
{
  "BRIEF",
  "COMPACT",
  "DEAF",
  "NO_TELL",
  "D_HP",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "NO_HASS",
  "QUEST",
  "SUMN",
  "NO_REP",
  "LIGHT",
  "C1",
  "C2",
  "NO_WIZ",
  "L1",
  "L2",
  "NO_AUC",
  "NO_GOS",
  "NO_GTZ",
  "RMFLG",
  "WILDTEXT",
  "WILDBLACK",
  "WILDSMALL",
  "NOGREET",
  "\n"
};


/* AFF_x */
const char *affected_bits[] =
{
  "BLIND",				// 0
  "INVIS",
  "DET-ALIGN",
  "DET-INVIS",
  "DET-MAGIC",
  "SENSE-LIFE",			// 5
  "WATWALK",
  "SANCT",
  "GROUP",
  "CURSE",
  "INFRA",				// 10
  "POISON",
  "PROT-EVIL",
  "PROT-GOOD",
  "SLEEP",
  "NO_TRACK",			// 15
  "MEMORIZING",
  "HUNTING",
  "SNEAK",
  "HIDE",
  "TAMED",				// 20
  "CHARM",
  "PARALYSIS",
  "FIRESHIELD",
  "REGENERATION",		// 24
  "\n"
};


/* CON_x */
const char *connected_types[] =
{
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Disconnecting",
  "Line Input",
  "Select race",
  "Roll Stats",
  "\n"
};


const char *wear_where[] =
{
  "&b&2<used as light>          &0",
  "&b&2<worn on finger>         &0",
  "&b&2<worn on finger>         &0",
  "&b&2<worn around neck>       &0",
  "&b&2<worn around neck>       &0",
  "&b&2<worn on body>           &0",
  "&b&2<worn on head>           &0",
  "&b&2<worn on legs>           &0",
  "&b&2<worn on feet>           &0",
  "&b&2<worn on hands>          &0",
  "&b&2<worn on arms>           &0",
  "&b&2<worn as shield>         &0",
  "&b&2<worn about body>        &0",
  "&b&2<worn about waist>       &0",
  "&b&2<worn around wrist>      &0",
  "&b&2<worn around wrist>      &0",
  "&b&2<wielded>                &0",
  "&b&2<held>                   &0",
  "&b&2<worn on shoulders>      &0"
};


/* WEAR_x - for stat */
const char *equipment_types[] =
{
  "Used as light",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "Worn on shoulders",
  "\n"
};


/* ITEM_x (ordinal object types) */
const char *item_types[] =
{
  "UNDEFINED",		// 0
  "LIGHT",			// 1
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",			// 5
  "FIRE WEAPON",
  "MISSILE",
  "TREASURE",
  "ARMOR",
  "POTION",			// 10
  "WORN",
  "OTHER",
  "TRASH",
  "TRAP",
  "CONTAINER",		// 15
  "NOTE",
  "LIQ CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",			// 20
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "ARMOR_SHIELD",
  "ARMOR_HELM",		// 25
  "ARMOR_ARMS",
  "ARMOR_LEGS",
  "ARMOR_HANDS",
  "ARMOR_FEETS",
  "ARMOR_BELT",		// 30
  "FURNITURE",
  "SPELLBOOK",
  "MISSILE CONT",
  "TROPHY",
  "GOODS",			// 35
  "MAP",
  "\n"
};


/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] =
{
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "SHOULDERS",
  "\n"
};


/* ITEM_x (extra bits) */
const char *extra_bits[] =
{
  "GLOW",
  "HUM",
  "NO_RENT",
  "NO_DONATE",
  "NO_INVIS",
  "INVISIBLE",
  "MAGIC",
  "NO_DROP",
  "BLESS",
  "FAST_MAGIC",
  "IS_SPELLBOOK",
  "HAS_SPELLS",
  "HAS_TRAPS",
  "NODAMAGE",
  "DONATED",
  "UNUSED15",
  "NO_SELL",
  "UNIQUE",
  "\n"
};


/* ITEM_x (anti bits) */
const char *anti_bits[] =
{
  "ANTI_MORTAL",
  "ANTI_GOOD",
  "ANTI_EVIL",
  "ANTI_NEUTRAL",
  "ANTI_MAGE",
  "ANTI_CLERIC",
  "ANTI_THIEF",
  "ANTI_WARRIOR",
  "ANTI_SORCERER",
  "ANTI_HUMAN",
  "ANTI_ELF",
  "ANTI_DWARF",
  "\n"
};


/* APPLY_x */
const char *apply_types[] =
{
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "CHA",
  "CLASS",
  "LEVEL",
  "AGE",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "MAXMANA",
  "MAXHIT",
  "MAXMOVE",
  "GOLD",
  "EXP",
  "ARMOR",
  "HITROLL",
  "DAMROLL",
  "SAVING_PARA",
  "SAVING_ROD",
  "SAVING_PETRI",
  "SAVING_BREATH",
  "SAVING_SPELL",
  "\n"
};


/* CONT_x */
const char *container_bits[] =
{
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};


/* FURN_x */
const char *furn_types[] =
{
  "<NONE>",		// 0
  "Throne",		// 1
  "Chair",
  "Table",
  "Bookcase",
  "Bed",		// 5
  "Counter",
  "Desk",		// 7
  "\n"
};

const char *prac_types[] =
{
  "spell",
  "skill"
};

/* constants for liquids ******************************************/

/* LIQ_x */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "\n"
};

/* one-word alias for each drink */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
int drink_aff[][3] =
{
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13}
};


/* color of the various drinks */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "\n"
};


/*
 * level of fullness for drink containers
 * Not used in sprinttype() so no \n.
 */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};


/* str, int, wis, dex, con applies **************************************/


/* [ch] strength apply (all) */
cpp_extern const struct str_app_type str_app[] =
{
  {-5, -4, 0, 0},	/* str = 0 */
  {-5, -4, 3, 1},	/* str = 1 */
  {-3, -2, 3, 2},
  {-3, -1, 10, 3},
  {-2, -1, 25, 4},
  {-2, -1, 55, 5},	/* str = 5 */
  {-1, 0, 80, 6},
  {-1, 0, 90, 7},
  {0, 0, 100, 8},
  {0, 0, 100, 9},
  {0, 0, 115, 10},	/* str = 10 */
  {0, 0, 115, 11},
  {0, 0, 140, 12},
  {0, 0, 140, 13},
  {0, 0, 170, 14},
  {0, 0, 170, 15},	/* str = 15 */
  {0, 1, 195, 16},
  {1, 1, 220, 18},
  {1, 2, 255, 20},	/* str = 18 */
  {1, 3, 280, 22},
  {2, 3, 305, 24},	/* str = 20 */
  {2, 4, 330, 26},
  {2, 5, 380, 28},
  {3, 6, 480, 30},
  {3, 7, 640, 40},
  {4, 8, 700, 50}	/* str = 25 */
};



/* [dex] skill apply (thieves only) */
cpp_extern const struct dex_skill_type dex_app_skill[] =
{
  {-99, -99, -90, -99, -60},	/* dex = 0 */
  {-90, -90, -60, -90, -50},	/* dex = 1 */
  {-80, -80, -40, -80, -45},
  {-70, -70, -30, -70, -40},
  {-60, -60, -30, -60, -35},
  {-50, -50, -20, -50, -30},	/* dex = 5 */
  {-40, -40, -20, -40, -25},
  {-30, -30, -15, -30, -20},
  {-20, -20, -15, -20, -15},
  {-15, -10, -10, -20, -10},
  {-10, -5, -10, -15, -5},		/* dex = 10 */
  {-5, 0, -5, -10, 0},
  {0, 0, 0, -5, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},				/* dex = 15 */
  {0, 5, 0, 0, 0},
  {5, 10, 0, 5, 5},
  {10, 15, 5, 10, 10},			/* dex = 18 */
  {15, 20, 10, 15, 15},
  {15, 20, 10, 15, 15},			/* dex = 20 */
  {20, 25, 10, 15, 20},
  {20, 25, 15, 20, 20},
  {25, 25, 15, 20, 20},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25}			/* dex = 25 */
};



/* [dex] apply (all) */
cpp_extern const struct dex_app_type dex_app[] =
{
  {-7, -7, 6},		/* dex = 0 */
  {-6, -6, 5},		/* dex = 1 */
  {-4, -4, 5},
  {-3, -3, 4},
  {-2, -2, 3},
  {-1, -1, 2},		/* dex = 5 */
  {0, 0, 1},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},		/* dex = 10 */
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, -1},		/* dex = 15 */
  {1, 1, -2},
  {2, 2, -3},
  {2, 2, -4},		/* dex = 18 */
  {3, 3, -4},
  {3, 3, -4},		/* dex = 20 */
  {4, 4, -5},
  {4, 4, -5},
  {4, 4, -5},
  {5, 5, -6},
  {5, 5, -6}		/* dex = 25 */
};



/* [con] apply (all) */
cpp_extern const struct con_app_type con_app[] =
{
  {-4, 20},		/* con = 0 */
  {-3, 25},		/* con = 1 */
  {-2, 30},
  {-2, 35},
  {-1, 40},
  {-1, 45},		/* con = 5 */
  {-1, 50},
  {0, 55},
  {0, 60},
  {0, 65},
  {0, 70},		/* con = 10 */
  {0, 75},
  {0, 80},
  {0, 85},
  {0, 88},
  {1, 90},		/* con = 15 */
  {2, 95},
  {2, 97},
  {3, 99},		/* con = 18 */
  {3, 99},
  {4, 99},		/* con = 20 */
  {5, 99},
  {5, 99},
  {5, 99},
  {6, 99},
  {6, 99}		/* con = 25 */
};



/* [int] apply (all) */
cpp_extern const struct int_app_type int_app[] =
{
  {3,	0},			/* int = 0 */
  {5,	0},			/* int = 1 */
  {7,	1},
  {8,	1},
  {9,	2},
  {10,	2},			/* int = 5 */
  {11,	3},
  {12,	4},
  {13,	5},
  {15,	6},
  {17,	7},			/* int = 10 */
  {19,	8},
  {22,	9},
  {25,	10},
  {30,	11},
  {35,	12},		/* int = 15 */
  {40,	14},
  {45,	16},
  {50,	17},		/* int = 18 */
  {53,	20},
  {55,	22},		/* int = 20 */
  {56,	25},
  {57,	30},
  {58,	35},
  {59,	40},
  {60,	50}			/* int = 25 */
};


/* [wis] apply (all) */
cpp_extern const struct wis_app_type wis_app[] =
{
  {0},	/* wis = 0 */
  {0},  /* wis = 1 */
  {0},
  {0},
  {0},
  {0},  /* wis = 5 */
  {0},
  {0},
  {0},
  {0},
  {0},  /* wis = 10 */
  {0},
  {2},
  {2},
  {3},
  {3},  /* wis = 15 */
  {3},
  {4},
  {5},	/* wis = 18 */
  {6},
  {6},  /* wis = 20 */
  {6},
  {6},
  {7},
  {7},
  {7}  /* wis = 25 */
};

const char *ctypes[] =
{
  "off",
  "sparse",
  "normal",
  "complete",
  "\n"
};

const char *npc_class_types[] =
{
  "Normal",
  "Undead",
  "\n"
};

/* alignment descriptions */
const char *aligndesc[] =
{
  "angelic",
  "noble",
  "honorable",
  "worthy",
  "neutral",
  "unworthy",
  "evil",
  "ignoble",
  "satanic"
};


const char *dist_descr[] =
{
  "hundreds of miles away in the distance",
  "far off in the skyline",
  "many miles away at great distance",
  "far off many miles away",
  "tens of miles away in the distance",
  "far off in the distance",
  "several miles away",
  "off in the distance",
  "not far from here",
  "in near vicinity",
  "in the immediate area"
};

/* Not used in sprinttype(). */
const char *weekdays[] =
{
  "the Day of the Moon",
  "the Day of the Bull",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the Day of the Great Gods",
  "the Day of the Sun"
};


/* Not used in sprinttype(). */
const char *month_name[] =
{
  "Month of Winter",		/* 0 */
  "Month of the Winter Wolf",
  "Month of the Frost Giant",
  "Month of the Old Forces",
  "Month of the Grand Struggle",
  "Month of the Spring",
  "Month of Nature",
  "Month of Futility",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Heat",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
  "Month of the Great Evil"
};


const char *moon_look[] =
{
  "new moon",						// 0
  "waxing crescent moon",
  "waxing half moon",
  "waxing three-quarter moon",
  "full moon",
  "waning three-quarter moon",		// 5
  "waning half moon",
  "waning crescent moon"			// 7
};


const char *obj_cond_table[] =
{
  "perfect",			// 0
  "almost perfect",
  "slightly damaged",
  "damaged",
  "heavily damaged",
  "badly damaged",		// 5
  "barely usable",
  "destroyed"			// 7
};


const char *trig_type_descr[] =
{
  "none",
  "teleport",
  "collapse",
  "explosion",
  "heal"
};

const char *trig_act_descr[] =
{
  "ENTER",
  "EXIT",
  "CAST",
  "FIGHT_START",
  "FIGHT_END",
  "GET",
  "DROP",
  "REST",
  "SLEEP",
  "SPEAK",
  "\n"
};

const char *trig_who_descr[] =
{
  "EVERYBODY",
  "PC",
  "NPC"
};


const char *trap_dam_descr[] =
{
  "NONE",
  "SLEEP",
  "TELEPORT",
  "FIRE",
  "COLD",
  "ACID",
  "ENERGY",
  "BLUNT",
  "PIERCE",
  "SLASH"
};

const char *trap_dam_descl[] =
{
  "none",
  "sleep",
  "teleport",
  "fire",
  "cold",
  "acid",
  "energy",
  "blunt",
  "pierce",
  "slash"
};

const char *trap_act_list[] =
{
  "ENTER_ROOM",
  "OPEN",
  "CLOSE",
  "GET",
  "PUT",
  "DROP",
  "GIVE",
  "LOOK_IN",
  "USE",
  "\n"
};

const char *trap_act_descr[] =
{
  "is set off when someone enter in the room",
  "is set off when opened",
  "is set off when closed",
  "is set off by get",
  "is set off by put",
  "is set off by drop",
  "is set off by give",
  "is set off when looked inside",
  "is set off when used",
  "\n"
};

const BOOK_TYPE book_types[] =
{
  {8, 1},
  {15, 2},
  {26, 4}
};

const char *st_rent_descr[] =
{
  "none",
  "Mount",
  "Vehicle"
};

const char *missile_cont_table[] =
{
  "none",
  "arrows",
  "rocks",
  "bolts"
};

const WEAPON_PROF wpn_prof[] =
{
  { 0,  0,  0,  0},  /* not warriors or level < PROF_LVL_START */
  { 1,  0,  0,  0},  /* prof <= 20   */
  { 2,  1,  0,  0},  /* prof <= 40   */
  { 3,  2,  0,  0},  /* prof <= 60   */
  { 4,  3,  0,  1},  /* prof <= 80   */
  { 5,  4, -1,  2},  /* prof <= 85   */
  { 6,  5, -2,  2},  /* prof <= 90   */
  { 7,  6, -3,  3},  /* prof <= 95   */
  { 8,  7, -4,  3},  /* prof <= 99   */
  { 9,  9, -5,  4},  /* prof == 100  */
  {-2, -2,  0,  0}   /* prof == 0    */
};


/* buildings ************************************************ */

const char *bld_type_descr[] =
{
  "HUT",
  "HOUSE",
  "PALACE",
  "STORE",
  "WORKSHOP",
  "TAVERN",
  "CHURCH",
  "WALL",
  "STRONGHOLD",
  "CASTLE",
  "TOWER",
  "MAGE_TOWER",
  "FARM",
  "\n"
};

const char *bld_flags_descr[] =
{
  "CLANHALL",
  "RUIN",
  "\n"
};

const char *bld_owner_descr[] =
{
  "NONE",
  "CHARACTER",
  "CLAN",
  "\n"
};

const char *bld_enter_descr[] =
{
  "EVERYONE",
  "OWNER_AND_GUESTS",
  "CLAN_MEMBERS",
  "\n"
};

const char *ship_flags[] =
{
  "IN_PORT",
  "SAILING",
  "COMBAT",
  "AT_ANCHOR",
  "IN_COURSE",
  "\n"
};
