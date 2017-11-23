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
 * File: objects.c                                                        *
 *                                                                        *
 * - Item condition                                                       *
 * - Item damage code                                                     *
 * - Item repair code                                                     *
 * - Money as objects                                                     *
 *                                                                        *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"

/* external globals */
extern bool LoadingCharObj;

/* external functions */
BUILDING_DATA *find_building_in_room_by_name(ROOM_DATA *pRoom, char *arg);
void	repair_building(CHAR_DATA *ch, BUILDING_DATA *bld);


/* ================================================================== */

int find_objcond_index(OBJ_DATA *obj)
{
	int num = 0, percent = 0;
	
	/* handle indestructable items */
	if (GET_OBJ_MAXCOND(obj) == -1 || OBJ_FLAGGED(obj, ITEM_NODAMAGE) )
		percent = 100;
	else
		percent = percentage(GET_OBJ_COND(obj), GET_OBJ_MAXCOND(obj));
	
	if		(percent >= 100)	num = 0;
	else if (percent > 80)		num = 1;
	else if (percent > 65)		num = 2;
	else if (percent > 40)		num = 3;
	else if (percent > 25)		num = 4;
	else if (percent >  9)		num = 5;
	else if (percent >  0)		num = 6;
	else						num = 7;
	
	return (num);
}
  

void make_scrap(CHAR_DATA *ch, OBJ_DATA *obj)
{
	OBJ_DATA *scrap = NULL, *cont = NULL;
	
	if ( !ch || !obj || (ch->in_room == NULL) )
		return;
	
	act("$p falls to the ground in scraps.", TRUE, ch, obj, 0, TO_CHAR);
	act("$p falls to the ground in scraps.", TRUE, ch, obj, 0, TO_ROOM);
	
	scrap = create_obj();
	
	scrap->name				= str_dup("scrap pile");
	sprintf(buf, "Scraps from %s lie in a pile.", obj->short_description);
	scrap->description		= str_dup(buf);
	scrap->short_description= str_dup("a pile of scraps");
	
	scrap->carried_by		= NULL;
	scrap->first_content	= NULL;
	scrap->last_content		= NULL;
	scrap->ex_description	= NULL;
	scrap->next				= NULL;
	scrap->prev				= NULL;
	scrap->next_content		= NULL;
	scrap->prev_content		= NULL;
	
	SET_BIT(scrap->obj_flags.wear_flags, ITEM_WEAR_TAKE);
	
	SET_BIT(scrap->obj_flags.extra_flags, ITEM_NODONATE | ITEM_NOSELL | ITEM_NOSAVE | ITEM_UNIQUE);
	
	while (obj->first_content)
	{
		cont = obj->first_content;
		obj_from_obj(cont);
		obj_to_room(cont, ch->in_room);
	}
	
	obj_to_room(scrap, ch->in_room);
	extract_obj(obj);
}


/*
 * return -1 if object is destroyed
 */
int damage_obj(CHAR_DATA *ch, OBJ_DATA *obj, int damage)
{
	int damres = 0;
	
	if (!ch || !obj || damage < 0)
		return (0);
	
	/* undestructable items exit here.. */
	if (GET_OBJ_MAXCOND(obj) == -1 || OBJ_FLAGGED(obj, ITEM_NODAMAGE) )
		return (0);
	
	/* magic items are strong */
	if (OBJ_FLAGGED(obj, ITEM_MAGIC) && !number(0, 3))
		return (0);
	
	/* blessed items too, and better */
	if (OBJ_FLAGGED(obj, ITEM_BLESS) && !number(0, 2))
		return (0);
	
	/* hmmm.. here or in check_damage_obj?? */
	separate_obj(obj);

	GET_OBJ_COND(obj) -= damage;
	GET_OBJ_COND(obj) = URANGE(0, GET_OBJ_COND(obj), GET_OBJ_MAXCOND(obj));
	
	/* Check for item falling apart */
	if (GET_OBJ_COND(obj) == 0)
	{
		make_scrap(ch, obj);
		damres = -1;
	}
	else
	{
		sprintf(buf, "%s got damaged.\r\n", obj->short_description);
		send_to_char(buf, ch);
	}
	
	return (damres);
}     

/* Assumption - NULL obj means check all equipment */
void check_damage_obj(CHAR_DATA *ch, OBJ_DATA *obj, int chance)
{
	OBJ_DATA *dobj = NULL;
	bool done = FALSE;
	int damage_pos, stop = 0;
	
	/* Increase wear and tear to stimulate economy... */
	chance *= 2;
	
	if ( number(1, 100) > chance )
		return;

	/* Null object means damage a random equipped one... */
	if (!obj)
	{
		while (!done && stop <= 30)
		{
			damage_pos = number(1, NUM_WEARS);
			dobj = GET_EQ(ch, damage_pos);
			if (dobj)
			{ 
				done = TRUE;
				damage_obj(ch, dobj, dice(1, 3));
			}
			stop++;
		}
	}
	else
		damage_obj(ch, obj, dice(1, 3));
}


void repair_object(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (GET_OBJ_COND(obj) <= 0)
	{
		act("$p cannot be repaired.", FALSE, ch, obj, NULL, TO_CHAR);
		return;
	}

	if (GET_OBJ_COND(obj) == GET_OBJ_MAXCOND(obj))
	{
		act("$p does not need reparations.", FALSE, ch, obj, NULL, TO_CHAR);
		return;
	}

	if (!roll(GET_DEX(ch)))
	{
		send_to_char("You fail.\r\n", ch);
		return;
	}

	GET_OBJ_COND(obj)++;
	act("You make some reparations to $p.", FALSE, ch, obj, NULL, TO_CHAR);
	act("$n makes some reparations to $p.", FALSE, ch, obj, NULL, TO_ROOM);
}

/*
 * this now handle building repairing too... FAB 8-dec-01
 */
ACMD(do_repair)
{
	OBJ_DATA *obj;
	BUILDING_DATA *bld;
	VEHICLE_DATA *pVeh;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("Usage: repair <object>\r\n   or: repair <building>\r\n", ch);
		send_to_char("You can repair items in your inventory or buildings in the room.\r\n", ch);
		return;
	}

	if ((obj = get_obj_in_list_vis_rev(ch, arg, NULL, ch->last_carrying)))
	{
		repair_object(ch, obj);
		return;
	}

	if ((bld = find_building_in_room_by_name(ch->in_room, str_dup(arg))))
	{
		repair_building(ch, bld);
		return;
	}

	if ((pVeh = find_vehicle_in_room_by_name(ch, str_dup(arg))))
	{
	}

	ch_printf(ch, "You don't see %s %s here..\r\n", AN(arg), arg);
}

/* **************************************************************** */
/* Money Objects                                                    */
/* **************************************************************** */
const char *ones_numerals[10] =
{
    "zero",
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "nine"
};

const char *tens_numerals[10] =
{
    "-",
    "-",
    "twenty",
    "thirty",
    "forty",
    "fifty",
    "sixty",
    "seventy",
    "eighty",
    "ninety"
};

const char *meta_numerals[4] =
{
    "hundred",
    "thousand",
    "million",
    "billion"
};

const char *special_numbers[10] =
{
    "ten",
    "eleven",
    "twelve",
    "thirteen",
    "fourteen",
    "fifteen",
    "sixteen",
    "seventeen",
    "eighteen",
    "nineteen"
};


/* ================================================================ */

/* Take a number like 43 and make it forty-three */
char *numberize(int n)
{
	static char sbuf[MAX_STRING_LENGTH];
	sh_int digits[3];
	int t = abs(n);
	
	sbuf[0] = '\0';
	
	/* Special cases (10-19) */
	if (n >= 10 && n <= 19)
	{
		sprintf(sbuf, "%s", special_numbers[n-10]);
		return (sbuf);
	}
	
	if (n < 10 && n >= 0)
	{
		sprintf(sbuf, "%s", ones_numerals[n]);
		return (sbuf);
	}
	
	/* Over maximum handled by this function */
	if (n >= 10000 || n < 0)
	{
		sprintf(sbuf, "%d", n);
		return (sbuf);
	}
	
	digits[3] = t / 1000;
	t -= 1000 * digits[3];
	digits[2] = t / 100;
	t -= 100 * digits[2];
	digits[1] = t / 10;
	t -= 10 * digits[1];
	digits[0] = t;
	
	if (digits[3] > 0)
	{
		sprintf(sbuf + strlen(sbuf), "%s", ones_numerals[digits[3]]);
		sprintf(sbuf + strlen(sbuf), " thousand ");
	}
	
	if (digits[2] > 0)
	{
		sprintf(sbuf + strlen(sbuf), "%s", ones_numerals[digits[2]]);
		sprintf(sbuf + strlen(sbuf), " hundred ");
	}
	
	if (digits[1] > 0)
	{
		sprintf(sbuf + strlen(sbuf), "%s", tens_numerals[digits[1]]);
		if (digits[0] > 0 ) sprintf( sbuf + strlen(sbuf), "-");
	}
	
	if (digits[0] > 0)
	{
		sprintf(sbuf + strlen(sbuf), "%s", ones_numerals[digits[0]]);
	}
	
	if (sbuf[(t = strlen(sbuf)-1)] == ' ')  sbuf[t] = '\0';
	if (sbuf[(t = strlen(sbuf)-1)] == ' ')  sbuf[t] = '\0';
	return (sbuf);
}

/* ================================================================ */

void update_money(OBJ_DATA *obj)
{
	EXTRA_DESCR *new_descr;
	char mbuf[MAX_STRING_LENGTH], mbuf1[MAX_STRING_LENGTH];
	char mbuf2[MAX_STRING_LENGTH], mbuf3[MAX_STRING_LENGTH];
	int amount;
	
	if (GET_OBJ_TYPE(obj) != ITEM_MONEY)
	{
		log("SYSERR: update_money: item is not of type money.");
		return;
	}
	
	if ((amount = GET_OBJ_VAL(obj, 0)) <= 0)
		return;

	CREATE(new_descr, EXTRA_DESCR, 1);

	if (amount > 1)
	{
		if		(amount <= 10)			strcpy( mbuf, "a few"               );
		else if (amount <= 100)			strcpy( mbuf, "a small pile of"     );
		else if (amount <= 1000)		strcpy( mbuf, "a pile of"           );
		else if (amount <= 10000)		strcpy( mbuf, "a large pile of"     );
		else if (amount <= 100000)		strcpy( mbuf, "an heaping mound of" );
		else if (amount <= 1000000)		strcpy( mbuf, "a small hill of"     );
		else if (amount <= 10000000)	strcpy( mbuf, "a mountain of"       );
		else							strcpy( mbuf, "a whole shitload of" );

		strcpy(mbuf1, "coins gold");
		if (obj->name)
			free(obj->name);
		obj->name = str_dup(mbuf1);

		sprintf(mbuf2, "%s gold coins", mbuf);
		if (obj->short_description)
			free(obj->short_description);
		obj->short_description = str_dup(mbuf2);

		sprintf(mbuf3, "%s gold coins is lying here.", mbuf);
		if (obj->description)
			free(obj->description);
		obj->description = str_dup(CAP(mbuf3));

		new_descr->keyword = str_dup(mbuf1);
		if (amount < 10)
			sprintf(mbuf, "There are %s gold coins.", numberize(amount));
		else if (amount < 100)
			sprintf(mbuf, "There are about %s gold coins.", numberize(10 * (amount / 10)));
		else if (amount < 1000)
			sprintf(mbuf, "It looks to be about %s gold coins.", numberize(100 * (amount / 100)));
		else if (amount < 100000)
			sprintf(mbuf, "You guess there are, maybe, %d gold coins.",
				1000 * ((amount / 1000) + number(0, (amount / 1000))));
		else
			strcpy(mbuf, "There are a LOT of gold coins.");
		new_descr->description = str_dup(mbuf);
	}
	else
	{
		strcpy(mbuf1, "coin gold");
		if (obj->name)
			free(obj->name);
		obj->name = str_dup(mbuf1);
		
		strcpy(mbuf2, "one gold coin");
		if (obj->short_description)
			free(obj->short_description);
		obj->short_description = str_dup(mbuf2);
		
		strcpy(mbuf3, "One miserable gold coin is lying here.");
		if (obj->description)
			free(obj->description);
		obj->description = str_dup(CAP(mbuf3));
		
		new_descr->keyword = str_dup(mbuf1);
		strcpy(mbuf, "It's just one miserable little gold coin.");
		new_descr->description = str_dup(mbuf);
	}

	new_descr->next		= NULL;
	obj->ex_description	= new_descr;
}

OBJ_DATA *make_money(int amount)
{
	OBJ_DATA *obj;
	
	if (amount <= 0)
	{
		log("SYSERR: make_money: zero or negative money %d.", amount);
		amount = 1;
	}

	obj = create_obj();

	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_UNIQUE);
	GET_OBJ_TYPE(obj)	= ITEM_MONEY;
	GET_OBJ_WEAR(obj)	= ITEM_WEAR_TAKE;
	GET_OBJ_VAL(obj, 0)	= amount;
	GET_OBJ_COST(obj)	= 1;
	/* real weight is handled by get_real_obj_weight() */
	GET_OBJ_WEIGHT(obj)	= 1;

	update_money(obj);
	
	return (obj);
}


/*
 * Create a set of 'money' objects.
 */
void create_amount(int amount, CHAR_DATA *pMob, ROOM_DATA *pRoom,
		   OBJ_DATA *pObj, VEHICLE_DATA *pVeh, bool SkipGroup)
{
	OBJ_DATA *obj;
	
	if (amount <= 0)
	{
		log( "Create_amount(): zero or negative money %d.", amount );
		return;
	}
	
	obj = make_money(amount);

	if (SkipGroup)
		LoadingCharObj = TRUE;

	if (pMob)	obj_to_char(obj, pMob);
	if (pRoom)	obj_to_room(obj, pRoom);
	if (pObj)	obj_to_obj(obj, pObj);
	if (pVeh)	obj_to_vehicle(obj, pVeh);

	if (SkipGroup)
		LoadingCharObj = FALSE;
}

/* *************************************************************** */
/* Find gold routines                                              */
/* *************************************************************** */

/* recursive */
int get_all_gold(OBJ_DATA *list)
{
	OBJ_DATA *obj;
	int amount = 0;

	for (obj = list; obj; obj = obj->prev_content)
	{
		if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
		{
			amount += GET_OBJ_VAL(obj, 0);
			continue;
		}
		
		if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
			amount += get_all_gold(obj->last_content);
	}

	return (amount);
}

/* find how much gold coins ch has */
int get_gold(CHAR_DATA *ch)
{
	if (IS_NPC(ch))
		return (GET_GOLD(ch));

	return (get_all_gold(ch->last_carrying));
}

/* *************************************************************** */
/* Add gold routines                                               */
/* *************************************************************** */

/* give coins to char */
void add_gold(CHAR_DATA *ch, int amount)
{
	if (IS_NPC(ch))
		GET_GOLD(ch) += amount;
	else
		create_amount(amount, ch, NULL, NULL, NULL, FALSE);
}


/* *************************************************************** */
/* Subtract gold routines                                          */
/* *************************************************************** */

/*
 * recursive search in char list and containers
 * for gold coins to be extracted..
 */
void sub_all_gold(OBJ_DATA *list, int *amount)
{
	OBJ_DATA *obj, *prev_obj = NULL;

	for (obj = list; obj && (*amount) > 0; obj = prev_obj)
	{
		prev_obj = obj->prev_content;

		if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
		{
			if (GET_OBJ_VAL(obj, 0) > *amount)
			{
				GET_OBJ_VAL(obj, 0) -= *amount;
				update_money(obj);
				(*amount) = 0;
				break;
			}
			else if ( obj->count == *amount )
			{
				extract_obj(obj);
				(*amount) = 0;
				break;
			}
			else
			{
				(*amount) -= GET_OBJ_VAL(obj, 0);
				extract_obj(obj);
				continue;
				// seek for more gold coins..
			}
		}

		if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
			sub_all_gold(obj->last_content, amount);
	}
}

/*
 * take gold coins from char
 *
 * returns:
 * TRUE   if all <amount> coins have been taken from ch
 * FALSE  if ch had less than <amount> coins
 */
bool sub_gold(CHAR_DATA *ch, int amount)
{
	if (amount < 0)
		return (TRUE);

	if (IS_NPC(ch))
		GET_GOLD(ch) -= amount;
	else
		sub_all_gold(ch->last_carrying, &amount);

	if (amount)
		return (FALSE);

	return (TRUE);
}


/* *************************************************************** */
/* Commands (Wizards)                                              */
/* *************************************************************** */

ACMD(do_newmoney)
{
	OBJ_DATA *obj;
	int amount;

	one_argument(argument, arg);

	if ( !*arg || !is_number(arg))
		amount = 1;
	else
		amount = atoi(arg);

	obj = make_money(amount);
	obj = obj_to_room(obj, ch->in_room);
	send_to_char(OK, ch);
}

