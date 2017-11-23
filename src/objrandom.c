/**************************************************************************
 * #   #   #   ##   #  #  ###   ##   ##  ###       http://www.lyonesse.it *
 * #    # #   #  #  ## #  #    #    #    #                                *
 * #     #    #  #  # ##  ##    #    #   ##   ## ##  #  #  ##             *
 * #     #    #  #  # ##  #      #    #  #    # # #  #  #  # #            *
 * ###   #     ##   #  #  ###  ##   ##   ###  #   #  ####  ##    Ver. 1.0 *
 *                                                                        *
 * -Based on CircleMud & Smaug-     Copyright (c) 2001-2002 by Mithrandir *
 *                                                                        *
 * ********************************************************************** *
 *                                                                        *
 * File: objrandom.c                                                      *
 *                                                                        *
 * Random object creation                                                 *
 *                                                                        *
 ************************************************************************ */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "objrandom.h"

#define TABLESIZE(a)		(sizeof(a)/sizeof(a)[0])

const QUALITY_DATA quality_table[] =
{
  { "bad",			-1},
  { "poor",			0 },
  { "minor",		1 },
  { "good",			2 },
  { "very good",	3 },
  { "supreme",		5 }
};

const RAND_ARMOR_DATA rand_helm_table[] =
{
  { "metal helmet",				5, 0,   ITEM_WEAR_HEAD},
  { "iron helmet",				4, 0,   ITEM_WEAR_HEAD},
  { "steel helmet",				5, 1,   ITEM_WEAR_HEAD},
  { "leather cap",				2, 2,   ITEM_WEAR_HEAD},
  { "mithril helm",				6, 2,   ITEM_WEAR_HEAD},
  { "black visor",				4, 1,   ITEM_WEAR_HEAD},
  { "ivory helm",				4, 2,   ITEM_WEAR_HEAD},
  { "war helm",					7, -1,  ITEM_WEAR_HEAD},
  { "hat",						1, 2,   ITEM_WEAR_HEAD},
  { "golden crown",				1, 3,   ITEM_WEAR_HEAD}
};

const RAND_ARMOR_DATA rand_belt_table[] =
{
  { "leather belt",				2, 1,   ITEM_WEAR_WAIST},
  { "golden girth",				2, 2,   ITEM_WEAR_WAIST},
  { "mithril belt",				5, 2,   ITEM_WEAR_WAIST},
  { "chain mail",				5, -1,  ITEM_WEAR_WAIST},
  { "feather belt",				3, 0,   ITEM_WEAR_WAIST},
  { "silver girth",				2, 0,   ITEM_WEAR_WAIST}
};

const RAND_ARMOR_DATA rand_arms_table[] =
{
  { "metal sleeves",			3, 0,   ITEM_WEAR_ARMS},
  { "iron vambrances",			4, 0,   ITEM_WEAR_ARMS},
  { "mithril sleeves",			3, 2,   ITEM_WEAR_ARMS},
  { "opal sleeves",				2, 1,   ITEM_WEAR_ARMS}
};

const RAND_ARMOR_DATA rand_legs_table[] =
{
  { "leather pants",			3, 0,   ITEM_WEAR_LEGS},
  { "metal greaves",			6, 0,   ITEM_WEAR_LEGS},
  { "metal leggings",			3, 1,   ITEM_WEAR_LEGS}
};

const RAND_ARMOR_DATA rand_hands_table[] =
{
  { "leather gloves",			3, 1,   ITEM_WEAR_HANDS},
  { "iron gauntlets",			4, 0,   ITEM_WEAR_HANDS},
  { "silk gloves",				2, 2,   ITEM_WEAR_HANDS},
  { "white gloves",				3, 0,   ITEM_WEAR_HANDS},
  { "steel gauntlets",			4, -1,  ITEM_WEAR_HANDS}
};

const RAND_ARMOR_DATA rand_feets_table[] =
{
  { "leather boots",			4, 1,   ITEM_WEAR_FEET},
  { "elven boots",				5, 2,   ITEM_WEAR_FEET},
  { "sturdy boots",				3, 0,   ITEM_WEAR_FEET},
  { "white sandals",			1, 1,   ITEM_WEAR_FEET},
  { "leather shoes",			2, 0,   ITEM_WEAR_FEET}
};

const RAND_ARMOR_DATA rand_shield_table[] =
{
  { "large shield",				5, 0,   ITEM_WEAR_SHIELD},
  { "black shield",				4, 1,   ITEM_WEAR_SHIELD},
  { "iron shield",				4, 0,   ITEM_WEAR_SHIELD},
  { "tower shield",				6, -1,  ITEM_WEAR_SHIELD}
};

const RAND_ARMOR_DATA rand_armor_table[] =
{
  { "silk cloak",				3, 1,   ITEM_WEAR_ABOUT},
  { "wool cape",				3, 0,   ITEM_WEAR_ABOUT},
  { "platinum cloak",			4, 1,   ITEM_WEAR_ABOUT},
  { "diamond plate",			10, 0,  ITEM_WEAR_BODY},
  { "green shirt",				2, 3,   ITEM_WEAR_BODY},
  { "iron breastplate",			8, 1,   ITEM_WEAR_BODY},
  { "white vest",				4, 2,   ITEM_WEAR_BODY},
  { "steel platemail",			8, 1,   ITEM_WEAR_BODY},
  { "iron scalemail",			7, 0,   ITEM_WEAR_BODY},
  { "chainmail",				7, 0,   ITEM_WEAR_BODY},
  { "battlesuit",				9, -1,  ITEM_WEAR_BODY},
  { "suit armor",				5, 1,   ITEM_WEAR_BODY},
  { "black robe",				2, 4,   ITEM_WEAR_BODY},
  { "breastplate",				7, 1,   ITEM_WEAR_BODY},
  { "mithril plate",			16, 5,  ITEM_WEAR_BODY},
};

const RAND_WEAPON_DATA rand_weapon_table[] =
{
  { "iron sword",               1, 1, TYPE_SLASH },
  { "green sword",              2, 1, TYPE_SLASH },
  { "long sword",               1, 2, TYPE_SLASH },
  { "long sword",               3, 1, TYPE_SLASH },
  { "black sword",              1, 3, TYPE_SLASH },
  { "steel sword",              2, 1, TYPE_SLASH },
  { "wooden sword",             1, 4, TYPE_SLASH },
  { "black sword",              2, 3, TYPE_SLASH },
  { "ivory sword",              0, 5, TYPE_SLASH },
  { "diamond sword",            3, 3, TYPE_SLASH },
  { "mithril sword",            2, 6, TYPE_SLASH },
  { "longsword",                2, 2, TYPE_SLASH },
  { "longsword",                3, 3, TYPE_SLASH },
  { "longsword",                3, 4, TYPE_SLASH },
  { "broadsword",               3, 5, TYPE_SLASH },
  { "broadsword",               2, 6, TYPE_SLASH },
  { "short dagger",             1, 2, TYPE_PIERCE },
  { "venom dagger",             2, 6, TYPE_PIERCE },
  { "black dagger",             1, 4, TYPE_PIERCE },
  { "iron dagger",              2, 3, TYPE_PIERCE },
  { "steel dagger",             3, 4, TYPE_PIERCE },
  { "large mace",               1, 1, TYPE_POUND },
  { "iron mace",                2, 2, TYPE_POUND },
  { "black mace",               3, 2, TYPE_POUND },
  { "spiked mace",              2, 1, TYPE_POUND },
  { "morningstar",              3, 2, TYPE_POUND },
  { "mithril morningstar",      3, 9, TYPE_POUND },
  { "large spear",              2, 3, TYPE_PIERCE },
  { "dwarven axe",              1, 4, TYPE_SLASH },
  { "large flail",              2, 3, TYPE_POUND },
  { "black whip",               1, 4, TYPE_SLASH },
  { "long halberd",             0, 5, TYPE_SLASH },
  { "iron spear",               3, 3, TYPE_PIERCE },
  { "ebony trident",            1, 2, TYPE_PIERCE },
  { "steel axe",                0, 5, TYPE_SLASH },
  { "dwarven battleaxe",        0, 6, TYPE_SLASH },
  { "iron flail",               3, 3, TYPE_POUND },
  { "fire whip",                2, 4, TYPE_SLASH },
  { "mithril halberd",          1, 6, TYPE_SLASH }
};

const RAND_WORN_DATA rand_worn_table[] =
{
  { "glowing ring",             2, ITEM_WEAR_FINGER,    ITEM_GLOW},
  { "red ring",                 3, ITEM_WEAR_FINGER,    ITEM_HUM},
  { "rilliphar bracelet",       5, ITEM_WEAR_WRIST,     0},
  { "spectral necklace",        1, ITEM_WEAR_NECK,      ITEM_INVISIBLE},
  { "platinum pendant",         2, ITEM_WEAR_NECK,      ITEM_BLESS}
};


/* =============================================================== */
/*                          F U N Z I O N I                        */
/* =============================================================== */

OBJ_DATA *load_rand_obj(int type, int level)
{
	OBJ_DATA *obj = NULL;
	bool is_magic = FALSE;

	if (type == 0)
		type = ITEM_WEAPON;

	if (type > 100)
	{
		type -= 100;
		is_magic = TRUE;
	}

	if (level < 1 || level >= LVL_IMMORT)
		level = LVL_IMMORT - 1;

	switch (type)
	{
	case ITEM_ARMOR:
		obj = create_rand_armor(level, is_magic);
		break;
	case ITEM_ARMOR_SHIELD:
		obj = create_rand_shield(level, is_magic);
		break;
	case ITEM_ARMOR_HELM:
		obj = create_rand_helm(level, is_magic);
		break;
	case ITEM_ARMOR_ARMS:
		obj = create_rand_arms(level, is_magic);
		break;
	case ITEM_ARMOR_LEGS:
		obj = create_rand_legs(level, is_magic);
		break;
	case ITEM_ARMOR_HANDS:
		obj = create_rand_hands(level, is_magic);
		break;
	case ITEM_ARMOR_FEETS:
		obj = create_rand_hands(level, is_magic);
		break;
	case ITEM_ARMOR_BELT:
		obj = create_rand_belt(level, is_magic);
		break;
	case ITEM_WEAPON:
		obj = create_rand_weapon(level, is_magic);
		break;
	case ITEM_WORN:
		obj = create_rand_worn(level, is_magic);
		break;

	case ITEM_SCROLL:
	case ITEM_WAND:
	case ITEM_STAFF:
	case ITEM_POTION:
	case ITEM_CONTAINER:
		/* not yet dude.. */
		break;
	default:
		break;
	}

	return (obj);
}

/* ======================================================== */

OBJ_DATA *base_rand_obj( int level, int qlty )
{
	OBJ_DATA *obj = create_obj();
	int j;

	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_UNIQUE);

	GET_OBJ_LEVEL(obj) = number(MAX(0, level - 3), MIN(LVL_IMMORT, level + 3));

	/* rivedere */
	GET_OBJ_COST(obj) = 100 * (qlty + 1);
	GET_OBJ_RENT(obj) = 100 * (qlty + 1);
	GET_OBJ_WEIGHT(obj) = 3 * (qlty + 1);

	for (j = 0; j < MAX_OBJ_AFF; j++)
	{
		obj->affected[j].location = APPLY_NONE;
		obj->affected[j].modifier = 0;
	}

	return (obj);
}

OBJ_DATA *create_rand_armor(int level, bool magic)
{
	OBJ_DATA *obj = NULL;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	int qlty = 0, type = 0;

	qlty = number(0, MAX_QLTY);
	type = number(0, TABLESIZE(rand_armor_table) - 1);

	obj = base_rand_obj(level, qlty);

	sprintf(buf, "%s", rand_armor_table[type].name);
	obj->name = str_dup(buf);

	sprintf(buf2, "%s", AN(rand_armor_table[type].name));
	CAP(buf2);

	CREATE(obj->ex_description, EXTRA_DESCR, 1);
	obj->ex_description->next = NULL;
	obj->ex_description->keyword = str_dup(buf);
	sprintf(buf, "%s %s quality %s created by the god of casuality.", buf2, 
			quality_table[qlty].descr, rand_armor_table[type].name);
	obj->ex_description->description = str_dup(buf);

	sprintf(buf, "%s %s", AN(rand_armor_table[type].name), rand_armor_table[type].name);
	obj->short_description = str_dup(buf);

	switch (number(1, 3))
	{
	case 1:
		sprintf(buf, "%s %s lies here.", buf2, rand_armor_table[type].name);
		break;
	case 2:
		sprintf(buf, "%s %s is resting here.", buf2, rand_armor_table[type].name);
		break;
	case 3:
		sprintf(buf, "%s %s has been dropped here.", buf2, rand_armor_table[type].name);
		break;
	}
	obj->description = str_dup(buf);

	GET_OBJ_TYPE(obj) = ITEM_ARMOR;

	GET_OBJ_VAL(obj, 0) = (quality_table[qlty].bonus + rand_armor_table[type].protect_mod + (level / 2)) / 2;
	SET_BIT(GET_OBJ_WEAR(obj), rand_armor_table[type].wear_loc | ITEM_WEAR_TAKE);

	if (magic)
	{
		static int attrib_types[] = {APPLY_STR, APPLY_STR, APPLY_DEX, APPLY_INT, APPLY_INT, APPLY_WIS, APPLY_DEX, APPLY_CON, APPLY_CON };
		static int combat_types[] = {APPLY_HITROLL, APPLY_HITROLL, APPLY_DAMROLL, APPLY_SAVING_SPELL, APPLY_DAMROLL };
		static int power_types[]  = {APPLY_MANA, APPLY_HIT, APPLY_MOVE, APPLY_AC, APPLY_HIT };

		switch (number(1, 3))
		{
		case 1:
			obj->affected[0].location = attrib_types[number(0, TABLESIZE(attrib_types) - 1)];
			obj->affected[0].modifier = number(1,3);
			break;
		case 2:
			obj->affected[0].location = combat_types[number(0, TABLESIZE(combat_types) - 1)];
			obj->affected[0].modifier = number(1, 1 + (level / 6));
			break;
		case 3:
			obj->affected[0].location = power_types[number(0, TABLESIZE(power_types) - 1)];
			obj->affected[0].modifier = MAX(1, number(level / 2, level + 1)) + (quality_table[qlty].bonus / 2);
			break;
		}
		if (obj->affected[0].location == APPLY_AC || obj->affected[0].location == APPLY_SAVING_SPELL )
			obj->affected[0].modifier *= -1;
		
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	}
	return (obj);
}

OBJ_DATA *create_rand_shield(int level, bool magic)
{
	OBJ_DATA *obj = NULL;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	int qlty = 0, type = 0;

	qlty = number(0, MAX_QLTY);
	type = number(0, TABLESIZE(rand_shield_table) -1);

	obj = base_rand_obj(level, qlty);

	sprintf(buf, "%s", rand_shield_table[type].name);
	obj->name = str_dup(buf);

	sprintf(buf2, "%s", AN(rand_shield_table[type].name));
	CAP(buf2);

	CREATE(obj->ex_description, EXTRA_DESCR, 1);
	obj->ex_description->next = NULL;
	obj->ex_description->keyword = str_dup(buf);
	sprintf(buf, "%s %s quality %s created by the god of casuality.", buf2, 
			quality_table[qlty].descr, rand_shield_table[type].name);
	obj->ex_description->description = str_dup(buf);

	sprintf(buf, "%s %s", AN(rand_shield_table[type].name), rand_shield_table[type].name);
	obj->short_description = str_dup(buf);

	switch (number(1, 3))
	{
	case 1:
		sprintf(buf, "%s %s lies here.", buf2, rand_shield_table[type].name);
		break;
	case 2:
		sprintf(buf, "%s %s is resting here.", buf2, rand_shield_table[type].name);
		break;
	case 3:
		sprintf(buf, "%s %s has been dropped here.", buf2, rand_shield_table[type].name);
		break;
	}
	obj->description = str_dup(buf);

	GET_OBJ_TYPE(obj) = ITEM_ARMOR;
	GET_OBJ_VAL(obj, 0) = (quality_table[qlty].bonus + rand_shield_table[type].protect_mod + (level / 2)) / 2;
	SET_BIT(GET_OBJ_WEAR(obj), rand_shield_table[type].wear_loc | ITEM_WEAR_TAKE);

	if (magic)
	{
		static int attrib_types[] = {APPLY_STR, APPLY_STR, APPLY_DEX, APPLY_INT, APPLY_INT, APPLY_WIS, APPLY_DEX, APPLY_CON, APPLY_CON };
		static int combat_types[] = {APPLY_HITROLL, APPLY_HITROLL, APPLY_DAMROLL, APPLY_SAVING_SPELL, APPLY_DAMROLL };
		static int power_types[] = {APPLY_MANA, APPLY_HIT, APPLY_MOVE, APPLY_AC, APPLY_HIT };

		switch (number(1, 3))
		{
		case 1:
			obj->affected[0].location = attrib_types[number(0, TABLESIZE(attrib_types) - 1)];
			obj->affected[0].modifier = number(1,2);
			break;
		case 2:
			obj->affected[0].location = combat_types[number(0, TABLESIZE(combat_types) - 1)];
			obj->affected[0].modifier = number(1, 1 + (level / 6));
			break;
		case 3:
			obj->affected[0].location = power_types[number(0, TABLESIZE(power_types) - 1)];
			obj->affected[0].modifier = MAX(1, number(level / 2, level + 1)) + (quality_table[qlty].bonus / 2);
			break;
		}
		if (obj->affected[0].location == APPLY_AC || obj->affected[0].location == APPLY_SAVING_SPELL )
			obj->affected[0].modifier *= -1;
		
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	}
	return (obj);
}


OBJ_DATA *create_rand_arms(int level, bool magic)
{
	OBJ_DATA *obj = NULL;
	char buf[MAX_STRING_LENGTH];
	int qlty = 0, type = 0;

	qlty = number(0, MAX_QLTY);
	type = number(0, TABLESIZE(rand_arms_table) -1);

	obj = base_rand_obj(level, qlty);

	sprintf(buf, "%s", rand_arms_table[type].name);
	obj->name = str_dup(buf);

	CREATE(obj->ex_description, EXTRA_DESCR, 1);
	obj->ex_description->next = NULL;
	obj->ex_description->keyword = str_dup(buf);
	sprintf(buf, "Some %s quality %s created by the god of casuality.",
			quality_table[qlty].descr, rand_arms_table[type].name);
	obj->ex_description->description = str_dup(buf);

	sprintf(buf, "some %s", rand_arms_table[type].name);
	obj->short_description = str_dup(buf);

	switch (number(1, 3))
	{
	case 1:
		sprintf(buf, "Some %s lies here.", rand_arms_table[type].name);
		break;
	case 2:
		sprintf(buf, "Some %s is resting here.", rand_arms_table[type].name);
		break;
	case 3:
		sprintf(buf, "Some %s has been dropped here.", rand_arms_table[type].name);
		break;
	}
	obj->description = str_dup(buf);

	GET_OBJ_TYPE(obj) = ITEM_ARMOR;
	GET_OBJ_VAL(obj, 0) = (quality_table[qlty].bonus + rand_arms_table[type].protect_mod + (level / 2)) / 2;
	SET_BIT(GET_OBJ_WEAR(obj), rand_arms_table[type].wear_loc | ITEM_WEAR_TAKE);

	if (magic)
	{
		static int attrib_types[] = {APPLY_STR, APPLY_STR, APPLY_DEX, APPLY_INT, APPLY_INT, APPLY_WIS, APPLY_DEX, APPLY_CON, APPLY_CON };
		static int combat_types[] = {APPLY_HITROLL, APPLY_HITROLL, APPLY_DAMROLL, APPLY_SAVING_SPELL, APPLY_DAMROLL };
		static int power_types[]  = {APPLY_MANA, APPLY_HIT, APPLY_MOVE, APPLY_AC, APPLY_HIT };

		switch (number(1, 3))
		{
		case 1:
			obj->affected[0].location = attrib_types[number(0, TABLESIZE(attrib_types) - 1)];
			obj->affected[0].modifier = number(1,2);
			break;
		case 2:
			obj->affected[0].location = combat_types[number(0, TABLESIZE(combat_types) - 1)];
			obj->affected[0].modifier = number(1, 1 + (level / 6));
			break;
		case 3:
			obj->affected[0].location = power_types[number(0, TABLESIZE(power_types) - 1)];
			obj->affected[0].modifier = MAX(1, number(level / 2, level + 1)) + (quality_table[qlty].bonus / 2);
			break;
		}
		if (obj->affected[0].location == APPLY_AC || obj->affected[0].location == APPLY_SAVING_SPELL )
			obj->affected[0].modifier *= -1;
		
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	}
	return (obj);
}


OBJ_DATA *create_rand_legs(int level, bool magic)
{
	OBJ_DATA *obj = NULL;
	char buf[MAX_STRING_LENGTH];
	int qlty = 0, type = 0;

	qlty = number(0, MAX_QLTY);
	type = number(0, TABLESIZE(rand_legs_table) -1);

	obj = base_rand_obj(level, qlty);

	sprintf(buf, "%s", rand_legs_table[type].name);
	obj->name = str_dup(buf);

	CREATE(obj->ex_description, EXTRA_DESCR, 1);
	obj->ex_description->next = NULL;
	obj->ex_description->keyword = str_dup(buf);
	sprintf(buf, "Some %s quality %s created by the god of casuality.",
			quality_table[qlty].descr, rand_legs_table[type].name);
	obj->ex_description->description = str_dup(buf);

	sprintf(buf, "some %s", rand_legs_table[type].name);
	obj->short_description = str_dup(buf);

	switch (number(1, 3))
	{
	case 1:
		sprintf(buf, "Some %s lies here.", rand_legs_table[type].name);
		break;
	case 2:
		sprintf(buf, "Some %s is resting here.", rand_legs_table[type].name);
		break;
	case 3:
		sprintf(buf, "Some %s has been dropped here.", rand_legs_table[type].name);
		break;
	}
	obj->description = str_dup(buf);

	GET_OBJ_TYPE(obj) = ITEM_ARMOR;
	GET_OBJ_VAL(obj, 0) = (quality_table[qlty].bonus + rand_legs_table[type].protect_mod + (level / 2)) / 2;
	SET_BIT(GET_OBJ_WEAR(obj), rand_legs_table[type].wear_loc | ITEM_WEAR_TAKE);

	if (magic)
	{
		static int attrib_types[] = {APPLY_STR, APPLY_STR, APPLY_DEX, APPLY_INT, APPLY_INT, APPLY_WIS, APPLY_DEX, APPLY_CON, APPLY_CON };
		static int combat_types[] = {APPLY_HITROLL, APPLY_HITROLL, APPLY_DAMROLL, APPLY_SAVING_SPELL, APPLY_DAMROLL };
		static int power_types[]  = {APPLY_MANA, APPLY_HIT, APPLY_MOVE, APPLY_AC, APPLY_HIT };

		switch (number(1, 3))
		{
		case 1:
			obj->affected[0].location = attrib_types[number(0, TABLESIZE(attrib_types) - 1)];
			obj->affected[0].modifier = number(1,2);
			break;
		case 2:
			obj->affected[0].location = combat_types[number(0, TABLESIZE(combat_types) - 1)];
			obj->affected[0].modifier = number(1, 1 + (level / 6));
			break;
		case 3:
			obj->affected[0].location = power_types[number(0, TABLESIZE(power_types) - 1)];
			obj->affected[0].modifier = MAX(1, number(level / 2, level + 1)) + (quality_table[qlty].bonus / 2);
			break;
		}
		if (obj->affected[0].location == APPLY_AC || obj->affected[0].location == APPLY_SAVING_SPELL )
			obj->affected[0].modifier *= -1;
		
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	}
	return (obj);
}


OBJ_DATA *create_rand_hands(int level, bool magic)
{
	OBJ_DATA *obj = NULL;
	char buf[MAX_STRING_LENGTH];
	int qlty = 0, type = 0;

	qlty = number(0, MAX_QLTY);
	type = number(0, TABLESIZE(rand_hands_table) -1);

	obj = base_rand_obj(level, qlty);

	sprintf(buf, "%s", rand_hands_table[type].name);
	obj->name = str_dup(buf);

	CREATE(obj->ex_description, EXTRA_DESCR, 1);
	obj->ex_description->next = NULL;
	obj->ex_description->keyword = str_dup(buf);
	sprintf(buf, "Some %s quality %s created by the god of casuality.",
			quality_table[qlty].descr, rand_hands_table[type].name);
	obj->ex_description->description = str_dup(buf);

	sprintf(buf, "some %s", rand_hands_table[type].name);
	obj->short_description = str_dup(buf);

	switch (number(1, 3))
	{
	case 1:
		sprintf(buf, "Some %s lies here.", rand_hands_table[type].name);
		break;
	case 2:
		sprintf(buf, "Some %s is resting here.", rand_hands_table[type].name);
		break;
	case 3:
		sprintf(buf, "Some %s has been dropped here.", rand_hands_table[type].name);
		break;
	}
	obj->description = str_dup(buf);

	GET_OBJ_TYPE(obj) = ITEM_ARMOR;
	GET_OBJ_VAL(obj, 0) = (quality_table[qlty].bonus + rand_hands_table[type].protect_mod + (level / 2)) / 2;
	SET_BIT(GET_OBJ_WEAR(obj), rand_hands_table[type].wear_loc | ITEM_WEAR_TAKE);

	if (magic)
	{
		static int attrib_types[] = {APPLY_STR, APPLY_STR, APPLY_DEX, APPLY_INT, APPLY_INT, APPLY_WIS, APPLY_DEX, APPLY_CON, APPLY_CON };
		static int combat_types[] = {APPLY_HITROLL, APPLY_HITROLL, APPLY_DAMROLL, APPLY_SAVING_SPELL, APPLY_DAMROLL };
		static int power_types[]  = {APPLY_MANA, APPLY_HIT, APPLY_MOVE, APPLY_AC, APPLY_HIT };

		switch (number(1, 3))
		{
		case 1:
			obj->affected[0].location = attrib_types[number(0, TABLESIZE(attrib_types) - 1)];
			obj->affected[0].modifier = number(1,4);
			break;
		case 2:
			obj->affected[0].location = combat_types[number(0, TABLESIZE(combat_types) - 1)];
			obj->affected[0].modifier = number(1, 1 + (level / 6));
			break;
		case 3:
			obj->affected[0].location = power_types[number(0, TABLESIZE(power_types) - 1)];
			obj->affected[0].modifier = MAX(1, number(level / 2, level + 1)) + (quality_table[qlty].bonus / 2);
			break;
		}
		if (obj->affected[0].location == APPLY_AC || obj->affected[0].location == APPLY_SAVING_SPELL )
			obj->affected[0].modifier *= -1;
		
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	}
	return (obj);
}


OBJ_DATA *create_rand_feets(int level, bool magic)
{
	OBJ_DATA *obj = NULL;
	char buf[MAX_STRING_LENGTH];
	int qlty = 0, type = 0;

	qlty = number(0, MAX_QLTY);
	type = number(0, TABLESIZE(rand_feets_table) -1);

	obj = base_rand_obj(level, qlty);

	sprintf(buf, "%s", rand_feets_table[type].name);
	obj->name = str_dup(buf);

	CREATE(obj->ex_description, EXTRA_DESCR, 1);
	obj->ex_description->next = NULL;
	obj->ex_description->keyword = str_dup(buf);
	sprintf(buf, "Some %s quality %s created by the god of casuality.",
			quality_table[qlty].descr, rand_feets_table[type].name);
	obj->ex_description->description = str_dup(buf);

	sprintf(buf, "some %s", rand_feets_table[type].name);
	obj->short_description = str_dup(buf);

	switch (number(1, 3))
	{
	case 1:
		sprintf(buf, "Some %s lies here.", rand_feets_table[type].name);
		break;
	case 2:
		sprintf(buf, "Some %s is resting here.", rand_feets_table[type].name);
		break;
	case 3:
		sprintf(buf, "Some %s has been dropped here.", rand_feets_table[type].name);
		break;
	}
	obj->description = str_dup(buf);

	GET_OBJ_TYPE(obj) = ITEM_ARMOR;
	GET_OBJ_VAL(obj, 0) = (quality_table[qlty].bonus + rand_feets_table[type].protect_mod + (level / 2)) / 2;
	SET_BIT(GET_OBJ_WEAR(obj), rand_feets_table[type].wear_loc | ITEM_WEAR_TAKE);

	if (magic)
	{
		static int attrib_types[] = {APPLY_STR, APPLY_STR, APPLY_DEX, APPLY_INT, APPLY_INT, APPLY_WIS, APPLY_DEX, APPLY_CON, APPLY_CON };
		static int combat_types[] = {APPLY_HITROLL, APPLY_HITROLL, APPLY_DAMROLL, APPLY_SAVING_SPELL, APPLY_DAMROLL };
		static int power_types[]  = {APPLY_MANA, APPLY_HIT, APPLY_MOVE, APPLY_AC, APPLY_HIT };

		switch (number(1, 3))
		{
		case 1:
			obj->affected[0].location = attrib_types[number(0, TABLESIZE(attrib_types) - 1)];
			obj->affected[0].modifier = number(1,2);
			break;
		case 2:
			obj->affected[0].location = combat_types[number(0, TABLESIZE(combat_types) - 1)];
			obj->affected[0].modifier = number(1, 1 + (level / 6));
			break;
		case 3:
			obj->affected[0].location = power_types[number(0, TABLESIZE(power_types) - 1)];
			obj->affected[0].modifier = MAX(1, number(level / 2, level + 1)) + (quality_table[qlty].bonus / 2);
			break;
		}
		if (obj->affected[0].location == APPLY_AC || obj->affected[0].location == APPLY_SAVING_SPELL )
			obj->affected[0].modifier *= -1;
		
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	}
	return (obj);
}


OBJ_DATA *create_rand_belt(int level, bool magic)
{
	OBJ_DATA *obj = NULL;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	int qlty = 0, type = 0;

	qlty = number(0, MAX_QLTY);
	type = number(0, TABLESIZE(rand_belt_table) -1);

	obj = base_rand_obj(level, qlty);

	sprintf(buf, "%s", rand_belt_table[type].name);
	obj->name = str_dup(buf);

	sprintf(buf2, "%s", AN(rand_belt_table[type].name));
	CAP(buf2);

	CREATE(obj->ex_description, EXTRA_DESCR, 1);
	obj->ex_description->next = NULL;
	obj->ex_description->keyword = str_dup(buf);
	sprintf(buf, "%s %s quality %s created by the god of casuality.", buf2, 
			quality_table[qlty].descr, rand_belt_table[type].name);
	obj->ex_description->description = str_dup(buf);

	sprintf(buf, "%s %s", AN(rand_belt_table[type].name), rand_belt_table[type].name);
	obj->short_description = str_dup(buf);

	switch (number(1, 3)) {
	case 1:
		sprintf(buf, "%s %s lies here.", buf2, rand_belt_table[type].name);
		break;
	case 2:
		sprintf(buf, "%s %s is resting here.", buf2, rand_belt_table[type].name);
		break;
	case 3:
		sprintf(buf, "%s %s has been dropped here.", buf2, rand_belt_table[type].name);
		break;
	}
	obj->description = str_dup(buf);

	GET_OBJ_TYPE(obj) = ITEM_ARMOR;
	GET_OBJ_VAL(obj, 0) = (quality_table[qlty].bonus + rand_belt_table[type].protect_mod + (level / 2)) / 2;
	SET_BIT(GET_OBJ_WEAR(obj), rand_belt_table[type].wear_loc | ITEM_WEAR_TAKE);

	if (magic)
	{
		static int attrib_types[] = {APPLY_STR, APPLY_STR, APPLY_DEX, APPLY_INT, APPLY_INT, APPLY_WIS, APPLY_DEX, APPLY_CON, APPLY_CON };
		static int combat_types[] = {APPLY_HITROLL, APPLY_HITROLL, APPLY_DAMROLL, APPLY_SAVING_SPELL, APPLY_DAMROLL };
		static int power_types[]  = {APPLY_MANA, APPLY_HIT, APPLY_MOVE, APPLY_AC, APPLY_HIT };

		switch (number(1, 3))
		{
		case 1:
			obj->affected[0].location = attrib_types[number(0, TABLESIZE(attrib_types) - 1)];
			obj->affected[0].modifier = number(1,2);
			break;
		case 2:
			obj->affected[0].location = combat_types[number(0, TABLESIZE(combat_types) - 1)];
			obj->affected[0].modifier = number(1, 1 + (level / 6));
			break;
		case 3:
			obj->affected[0].location = power_types[number(0, TABLESIZE(power_types) - 1)];
			obj->affected[0].modifier = MAX(1, number(level / 2, level + 1)) + (quality_table[qlty].bonus / 2);
			break;
		}
		if (obj->affected[0].location == APPLY_AC || obj->affected[0].location == APPLY_SAVING_SPELL )
			obj->affected[0].modifier *= -1;
		
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	}
	return (obj);
}

OBJ_DATA *create_rand_helm(int level, bool magic)
{
	OBJ_DATA *obj = NULL;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	int qlty = 0, type = 0;

	qlty = number(0, MAX_QLTY);
	type = number(0, TABLESIZE(rand_helm_table) -1);

	obj = base_rand_obj(level, qlty);

	sprintf(buf, "%s", rand_helm_table[type].name);
	obj->name = str_dup(buf);

	sprintf(buf2, "%s", AN(rand_helm_table[type].name));
	CAP(buf2);

	CREATE(obj->ex_description, EXTRA_DESCR, 1);
	obj->ex_description->next = NULL;
	obj->ex_description->keyword = str_dup(buf);
	sprintf(buf, "%s %s quality %s created by the god of casuality.", buf2, 
			quality_table[qlty].descr, rand_helm_table[type].name);
	obj->ex_description->description = str_dup(buf);

	sprintf(buf, "%s %s", AN(rand_helm_table[type].name), rand_helm_table[type].name);
	obj->short_description = str_dup(buf);

	switch (number(1, 3))
	{
	case 1:
		sprintf(buf, "%s %s lies here.", buf2, rand_helm_table[type].name);
		break;
	case 2:
		sprintf(buf, "%s %s is resting here.", buf2, rand_helm_table[type].name);
		break;
	case 3:
		sprintf(buf, "%s %s has been dropped here.", buf2, rand_helm_table[type].name);
		break;
	}
	obj->description = str_dup(buf);

	GET_OBJ_TYPE(obj) = ITEM_ARMOR;
	GET_OBJ_VAL(obj, 0) = (quality_table[qlty].bonus + rand_helm_table[type].protect_mod + (level / 2)) / 2;
	SET_BIT(GET_OBJ_WEAR(obj), rand_helm_table[type].wear_loc | ITEM_WEAR_TAKE);

	if (magic)
	{
		static int attrib_types[] = {APPLY_STR, APPLY_STR, APPLY_DEX, APPLY_INT, APPLY_INT, APPLY_WIS, APPLY_DEX, APPLY_CON, APPLY_CON };
		static int combat_types[] = {APPLY_HITROLL, APPLY_HITROLL, APPLY_DAMROLL, APPLY_SAVING_SPELL, APPLY_DAMROLL };
		static int power_types[]  = {APPLY_MANA, APPLY_HIT, APPLY_MOVE, APPLY_AC, APPLY_HIT };

		switch (number(1, 3))
		{
		case 1:
			obj->affected[0].location = attrib_types[number(0, TABLESIZE(attrib_types) - 1)];
			obj->affected[0].modifier = number(1,2);
			break;
		case 2:
			obj->affected[0].location = combat_types[number(0, TABLESIZE(combat_types) - 1)];
			obj->affected[0].modifier = number(1, 1 + (level / 6));
			break;
		case 3:
			obj->affected[0].location = power_types[number(0, TABLESIZE(power_types) - 1)];
			obj->affected[0].modifier = MAX(1, number(level / 2, level + 1)) + (quality_table[qlty].bonus / 2);
			break;
		}
		if (obj->affected[0].location == APPLY_AC || obj->affected[0].location == APPLY_SAVING_SPELL )
			obj->affected[0].modifier *= -1;
		
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	}
	return (obj);
}

OBJ_DATA *create_rand_worn(int level, bool magic)
{
	OBJ_DATA *obj = NULL;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	int qlty = 0, type = 0;

	qlty = number(0, 5);
	type = number(0, TABLESIZE(rand_worn_table) -1);

	obj = base_rand_obj(level, qlty);

	sprintf(buf, "%s", rand_worn_table[type].name);
	obj->name = str_dup(buf);

	sprintf(buf2, "%s", AN(rand_worn_table[type].name));
	CAP(buf2);

	CREATE(obj->ex_description, EXTRA_DESCR, 1);
	obj->ex_description->next = NULL;
	obj->ex_description->keyword = str_dup(buf);
	sprintf(buf, "%s %s quality %s created by the gods.", buf2, 
			quality_table[qlty].descr, rand_worn_table[type].name);
	obj->ex_description->description = str_dup(buf);

	sprintf(buf, "%s %s", AN(rand_worn_table[type].name), rand_worn_table[type].name);
	obj->short_description = str_dup(buf);

	sprintf(buf, "%s %s has been left here.", buf2, rand_worn_table[type].name);
	obj->description = str_dup(buf);

	GET_OBJ_TYPE(obj) = ITEM_WORN;
	SET_BIT(GET_OBJ_EXTRA(obj), rand_worn_table[type].extra_flags);
	SET_BIT(GET_OBJ_WEAR(obj), rand_worn_table[type].wear_loc | ITEM_WEAR_TAKE);

	if (magic && rand_worn_table[type].magic_mod > 0)
	{
		static int attrib_types[] = { APPLY_STR, APPLY_STR, APPLY_DEX, APPLY_INT, APPLY_CON, APPLY_WIS, APPLY_INT, APPLY_DEX, APPLY_CON };
		static int combat_types[] = { APPLY_HITROLL, APPLY_HITROLL, APPLY_DAMROLL, APPLY_DAMROLL };
		static int power_types[]  = { APPLY_MANA, APPLY_HIT, APPLY_AC, APPLY_AC, APPLY_HIT };

		switch (number(1, 3))
		{
		case 1:
			obj->affected[0].location = attrib_types[number(0, TABLESIZE(attrib_types) - 1)];
			obj->affected[0].modifier = number(1, rand_worn_table[type].magic_mod);
			break;
		case 2:
			obj->affected[0].location = combat_types[number(0, TABLESIZE(combat_types) - 1)];
			obj->affected[0].modifier = number(1, level / 8 + 1);
			break;
		case 3:
			obj->affected[0].location = power_types[number(0, TABLESIZE(power_types) - 1)];
			obj->affected[0].modifier = MAX(1, number(level / 2, level)) + (quality_table[qlty].bonus / 2);
			break;
		}
		if (obj->affected[0].location == APPLY_AC || obj->affected[0].location == APPLY_SAVING_SPELL )
			obj->affected[0].modifier *= -1;
		
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	}

	return (obj);
}


OBJ_DATA *create_rand_weapon(int level, bool magic)
{
	OBJ_DATA *obj = NULL;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	int qlty = 0, type = 0;

	qlty = number(0, 5);
	type = number(0, TABLESIZE(rand_weapon_table) -1);

	obj = base_rand_obj(level, qlty);

	sprintf(buf, "%s", rand_weapon_table[type].name);
	obj->name = str_dup(buf);

	sprintf(buf2, "%s", AN(rand_weapon_table[type].name));
	CAP(buf2);

	CREATE(obj->ex_description, EXTRA_DESCR, 1);
	obj->ex_description->next = NULL;
	obj->ex_description->keyword = str_dup(buf);
	sprintf(buf, "%s %s quality %s created by the gods.", buf2, 
			quality_table[qlty].descr, rand_weapon_table[type].name);
	obj->ex_description->description = str_dup(buf);

	sprintf(buf, "%s %s", AN(rand_weapon_table[type].name), rand_weapon_table[type].name);
	obj->short_description = str_dup(buf);

	sprintf(buf, "%s %s lies here.", buf2, rand_weapon_table[type].name);
	obj->description = str_dup(buf);

	GET_OBJ_TYPE(obj) = ITEM_WEAPON;
	SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE | ITEM_WEAR_WIELD);

	GET_OBJ_VAL(obj, 0) = 0;
	GET_OBJ_VAL(obj, 1) = (level / 10) + 2 + (quality_table[qlty].bonus / 2);
	GET_OBJ_VAL(obj, 2) = number(3, 3 + rand_weapon_table[type].damage_mod + (quality_table[qlty].bonus / 3));
	GET_OBJ_VAL(obj, 3) = rand_weapon_table[type].damage_type - TYPE_HIT;

//	if (magic && rand_weapon_table[type].magic_mod > 0) {
	if (magic)
	{
		static int attrib_types[] = { APPLY_STR, APPLY_STR, APPLY_DEX, APPLY_INT, APPLY_STR, APPLY_WIS, APPLY_DEX, APPLY_DEX, APPLY_CON };
		static int combat_types[] = { APPLY_HITROLL, APPLY_HITROLL, APPLY_DAMROLL, APPLY_DAMROLL };
		static int power_types[]  = { APPLY_MANA, APPLY_HIT, APPLY_HIT, APPLY_AC, APPLY_HIT };

		switch (number(1, 3))
		{
		case 1:
			obj->affected[0].location = attrib_types[number(0, TABLESIZE(attrib_types) - 1)];
			obj->affected[0].modifier = number(1, rand_weapon_table[type].magic_mod);
			break;
		case 2:
			obj->affected[0].location = combat_types[number(0, TABLESIZE(combat_types) - 1)];
			obj->affected[0].modifier = number(1, level / 8 + 1);
			break;
		case 3:
			obj->affected[0].location = power_types[number(0, TABLESIZE(power_types) - 1)];
			obj->affected[0].modifier = MAX(1, number(level / 2, level)) + (quality_table[qlty].bonus / 2);
			break;
		}
		if (obj->affected[0].location == APPLY_AC || obj->affected[0].location == APPLY_SAVING_SPELL )
			obj->affected[0].modifier *= -1;
		
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
	}
	return (obj);
}

/* =============================================================== */
/*                           C O M A N D I                         */
/* =============================================================== */
ACMD(do_rand_armor)
{
	OBJ_DATA *obj = NULL;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	bool ismagic = FALSE;
	int level = 0;
	
	argument = two_arguments(argument, arg1, arg2);
	argument = one_argument(argument, arg3);
	
	if (!*arg1)
	{
		send_to_char("Usage: &b&7randarmor <armor type> <level> <magical>&0\r\n\r\n", ch);
		send_to_char("Valid armor types are:\r\n&b&7armor, helm, shield, arms, legs, hands, feets, belt&0\r\n\r\n", ch);
		send_to_char("Valid level value is from 1 to 30.\r\n\r\n", ch);
		send_to_char("If you want a magical armor, write 'magic', otherwise, don't write anything.\r\n", ch);
		return;
	}
	
	if (!*arg2)
		level = number(1, 30);
	else
	{
		level = atoi(arg2);
		if (level < 1 || level > 30)
			level = number(1, 30);
	}
	
	if (!*arg3)
		ismagic = FALSE;
	else
	{
		if (!str_cmp(arg3, "magic"))
			ismagic = TRUE;
	}
		
	if (!str_cmp(arg1, "armor"))
		obj = create_rand_armor(level, ismagic);
	else if (!str_cmp(arg1, "helm"))
		obj = create_rand_helm(level, ismagic);
	else if (!str_cmp(arg1, "shield"))
		obj = create_rand_shield(level, ismagic);
	else if (!str_cmp(arg1, "arms"))
		obj = create_rand_arms(level, ismagic);
	else if (!str_cmp(arg1, "legs"))
		obj = create_rand_legs(level, ismagic);
	else if (!str_cmp(arg1, "hands"))
		obj = create_rand_hands(level, ismagic);
	else if (!str_cmp(arg1, "feets"))
		obj = create_rand_feets(level, ismagic);
	else if (!str_cmp(arg1, "belt"))
		obj = create_rand_belt(level, ismagic);
	else
	{
		send_to_char("Valid armor types are:\r\n&b&7armor, helm, shield, arms, legs, hands, feets, belt&0\r\n\r\n", ch);
		return;
	}
	
	if (obj)
	{
		obj_to_room(obj, IN_ROOM(ch));
		send_to_char("Done.\r\n", ch);
	}
	else 
		log("SYSERR: null obj pointer returned in do_randarmor");
}


ACMD(do_rand_weapon)
{
	OBJ_DATA *obj = NULL;

	obj = create_rand_weapon(number(1, 30), FALSE);
	obj_to_room(obj, IN_ROOM(ch));
	send_to_char("Done.\r\n", ch);
}

