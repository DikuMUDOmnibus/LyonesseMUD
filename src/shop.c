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
*   File: shop.c                                        Part of CircleMUD *
*  Usage: shopkeepers: loading config files, spec procs.                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/***
 * The entire shop rewrite for Circle 3.0 was done by Jeff Fink.  Thanks Jeff!
 ***/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "shop.h"
#include "constants.h"

/* External variables */
extern TIME_INFO_DATA time_info;

/* Forward/External function declarations */
ACMD(do_tell);
ACMD(do_action);
ACMD(do_echo);
ACMD(do_say);
void sort_keeper_objs(CHAR_DATA *keeper, int shop_nr);

/* Local variables */
SHOP_DATA *shop_index;
int top_shop = -1;
int cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_puke;

/* local functions */
char *read_shop_message(int mnum, room_vnum shr, FILE *shop_f, const char *why);
int read_type_list(FILE *shop_f, SHOP_BUY_DATA *list, int new_format, int max);
int read_list(FILE *shop_f, SHOP_BUY_DATA *list, int new_format, int max, int type);
void shopping_list(char *arg, CHAR_DATA *ch, CHAR_DATA *keeper, int shop_nr);
void shopping_value(char *arg, CHAR_DATA *ch, CHAR_DATA *keeper, int shop_nr);
void shopping_sell(char *arg, CHAR_DATA *ch, CHAR_DATA *keeper, int shop_nr);
OBJ_DATA *get_selling_obj(CHAR_DATA *ch, char *name, CHAR_DATA *keeper, int shop_nr, int msg);
OBJ_DATA *slide_obj(OBJ_DATA *obj, CHAR_DATA *keeper, int shop_nr);
void shopping_buy(char *arg, CHAR_DATA *ch, CHAR_DATA *keeper, int shop_nr);
OBJ_DATA *get_purchase_obj(CHAR_DATA *ch, char *arg, CHAR_DATA *keeper, int shop_nr, int msg);
OBJ_DATA *get_hash_obj_vis(CHAR_DATA *ch, char *name, OBJ_DATA *list);
OBJ_DATA *get_slide_obj_vis(CHAR_DATA *ch, char *name, OBJ_DATA *list);
void boot_the_shops(FILE *shop_f, char *filename, int rec_count);
void assign_the_shopkeepers(void);
char *customer_string(int shop_nr, int detailed);
void list_all_shops(CHAR_DATA *ch);
void handle_detailed_list(char *buf, char *buf1, CHAR_DATA *ch);
void list_detailed_shop(CHAR_DATA *ch, int shop_nr);
void show_shops(CHAR_DATA *ch, char *arg);
int is_ok_char(CHAR_DATA *keeper, CHAR_DATA *ch, int shop_nr);
int is_open(CHAR_DATA *keeper, int shop_nr, int msg);
int is_ok(CHAR_DATA *keeper, CHAR_DATA *ch, int shop_nr);
void push(STACK_DATA *stack, int pushval);
int top(STACK_DATA *stack);
int pop(STACK_DATA *stack);
void evaluate_operation(STACK_DATA *ops, STACK_DATA *vals);
int find_oper_num(char token);
int evaluate_expression(OBJ_DATA *obj, char *expr);
int trade_with(OBJ_DATA *item, int shop_nr);
int same_obj(OBJ_DATA *obj1, OBJ_DATA *obj2);
int shop_producing(OBJ_DATA *item, int shop_nr);
int transaction_amt(char *arg);
char *times_message(OBJ_DATA *obj, char *name, int num);
int buy_price(OBJ_DATA *obj, int shop_nr);
int sell_price(CHAR_DATA *ch, OBJ_DATA *obj, int shop_nr);
char *list_object(OBJ_DATA *obj, int cnt, int index, int shop_nr);
int ok_shop_room(int shop_nr, ROOM_DATA *room);
SPECIAL(shop_keeper);
int ok_damage_shopkeeper(CHAR_DATA *ch, CHAR_DATA *victim);
int add_to_list(SHOP_BUY_DATA *list, int type, int *len, int *val);
int end_read_list(SHOP_BUY_DATA *list, int len, int error);
void read_line(FILE *shop_f, const char *string, void *data);


/* config arrays */
const char *operator_str[] =
{
  "[({",
  "])}",
  "|+",
  "&*",
  "^'"
};

/* Constant list for printing out who we sell to */
const char *trade_letters[] =
{
  "Good",		/* First, the alignment based ones */
  "Evil",
  "Neutral",
  "Magic User",		/* Then the class based ones */
  "Cleric",
  "Thief",
  "Warrior",
  "Sorcerer",
  "Human",		/* And finally the race based ones */
  "Elf",
  "Dwarf",
  "\n"
};


const char *shop_bits[] =
{
  "WILL_FIGHT",
  "USES_BANK",
  "\n"
};

/* ======================================================================= */

/*
 * Function to take a shop and mob and save any non-unique items to it
 * It saves any object vnums which aren't native producing, good for crashs
 */
void save_shop_nonnative(shop_vnum shop_num, CHAR_DATA *keeper)
{
	FILE *cfile = NULL;
	OBJ_DATA *obj;

	sprintf(buf, "shop_item/%ld", shop_num);
	
	if ((cfile = fopen(buf, "w")) == NULL)
	{
		mudlog("SYSERR: SHP: Can't write new shop_item file.", BRF, LVL_IMPL, TRUE);
		exit(0);
	}
	
	/*
	 * Loops through the keeper's item list, checks if its native or not
	 * if it isnt, it writes the item's VNUM to the file 
	 */
	*buf = '\0';
	*buf2 = '\0';

	sprintf(buf2, "#%ld\n%s\n", shop_num, GET_NAME(keeper));

	for (obj = keeper->first_carrying; obj; obj = obj->next_content)
	{
		if (!shop_producing(obj, shop_num) && !OBJ_FLAGGED(obj, ITEM_UNIQUE))
			sprintf(buf2 + strlen(buf2), "%ld\n", GET_OBJ_VNUM(obj));
	}

	fprintf(cfile, buf2);
	fprintf(cfile, "$\n");
	fclose(cfile);
}

void load_shop_nonnative(shop_vnum shop_num, CHAR_DATA *keeper)
{
	FILE *cfile = NULL;
	OBJ_DATA *obj;
	int line_num = 0, placer = 0;
	obj_vnum v_this;
	
	sprintf(buf, "shop_item/%ld", shop_num);
	
	/* Check to see if we have a file for this shop number */
	if ((cfile = fopen(buf, "r")) == NULL)
		return;
	
	// Shop number
	line_num += get_line(cfile, buf);
	if (sscanf(buf, "#%d", &placer) != 1)
	{
		fprintf(stderr, "Format error in shop_item %ld, line %d.\n", shop_num, line_num);
		exit(0);
	}
	
	// Name of shopkeeper
	line_num += get_line(cfile, buf);
	
	// Item list
	while (buf && *buf != '$')
	{
		line_num += get_line(cfile, buf);
		if (!buf || !*buf || *buf == '$')
			break;
		if (sscanf(buf, "%d", &placer) != 1)
		{
			fprintf(stderr, "Format error in shop_item %ld, line %d.\n", shop_num, line_num);
			exit(0);
		}
		v_this = placer;
		obj = read_object(v_this, VIRTUAL);
		slide_obj(obj, keeper, shop_num);
	}
}

/* ======================================================================= */

int is_ok_char(CHAR_DATA *keeper, CHAR_DATA *ch, int shop_nr)
{
	char buf[MAX_INPUT_LENGTH];
	
	if (!CAN_SEE(keeper, ch))
	{
		char actbuf[MAX_INPUT_LENGTH] = MSG_NO_SEE_CHAR;
		do_say(keeper, actbuf, cmd_say, 0);
		return (FALSE);
	}
	if (IS_GOD(ch))
		return (TRUE);
	
	if ((IS_GOOD(ch) && NOTRADE_GOOD(shop_nr)) ||
		(IS_EVIL(ch) && NOTRADE_EVIL(shop_nr)) ||
		(IS_NEUTRAL(ch) && NOTRADE_NEUTRAL(shop_nr)))
	{
		sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_SELL_ALIGN);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}
	
	if (IS_NPC(ch))
		return (TRUE);
	
	if ((IS_MAGIC_USER(ch) && NOTRADE_MAGIC_USER(shop_nr)) ||
		(IS_CLERIC(ch) && NOTRADE_CLERIC(shop_nr)) ||
		(IS_THIEF(ch) && NOTRADE_THIEF(shop_nr)) ||
		(IS_WARRIOR(ch) && NOTRADE_WARRIOR(shop_nr)) ||
		(IS_SORCERER(ch) && NOTRADE_SORCERER(shop_nr)))
	{
		sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_SELL_CLASS);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}

	if ((IS_HUMAN(ch) && NOTRADE_HUMAN(shop_nr)) ||
		(IS_ELF(ch) && NOTRADE_ELF(shop_nr)) ||
		(IS_DWARF(ch) && NOTRADE_DWARF(shop_nr)))
	{
		sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_SELL_RACE);
		do_tell(keeper, buf, cmd_tell, 0);
		return (FALSE);
	}
	return (TRUE);
}


int is_open(CHAR_DATA *keeper, int shop_nr, int msg)
{
	char buf[MAX_INPUT_LENGTH];
	
	*buf = '\0';
	if (SHOP_OPEN1(shop_nr) > time_info.hours)
		strcpy(buf, MSG_NOT_OPEN_YET);
	else if (SHOP_CLOSE1(shop_nr) < time_info.hours)
	{
		if (SHOP_OPEN2(shop_nr) > time_info.hours)
			strcpy(buf, MSG_NOT_REOPEN_YET);
		else if (SHOP_CLOSE2(shop_nr) < time_info.hours)
			strcpy(buf, MSG_CLOSED_FOR_DAY);
	}

	if (!*buf)
		return (TRUE);
	if (msg)
		do_say(keeper, buf, cmd_tell, 0);
	
	return (FALSE);
}


int is_ok(CHAR_DATA *keeper, CHAR_DATA *ch, int shop_nr)
{
	if (is_open(keeper, shop_nr, TRUE))
		return (is_ok_char(keeper, ch, shop_nr));
	else
		return (FALSE);
}


void push(STACK_DATA *stack, int pushval)
{
	S_DATA(stack, S_LEN(stack)++) = pushval;
}


int top(STACK_DATA *stack)
{
	if (S_LEN(stack) > 0)
		return (S_DATA(stack, S_LEN(stack) - 1));
	else
		return (NOTHING);
}


int pop(STACK_DATA *stack)
{
	if (S_LEN(stack) > 0)
		return (S_DATA(stack, --S_LEN(stack)));
	else
	{
		log("SYSERR: Illegal expression %d in shop keyword list.", S_LEN(stack));
		return (0);
	}
}


void evaluate_operation(STACK_DATA *ops, STACK_DATA *vals)
{
	int oper;
	
	if ((oper = pop(ops)) == OPER_NOT)
		push(vals, !pop(vals));
	else
	{
		int val1 = pop(vals), val2 = pop(vals);
		
		/* Compiler would previously short-circuit these. */
		if (oper == OPER_AND)
			push(vals, val1 && val2);
		else if (oper == OPER_OR)
			push(vals, val1 || val2);
	}
}


int find_oper_num(char token)
{
	int index;
	
	for (index = 0; index <= MAX_OPER; index++)
		if (strchr(operator_str[index], token))
			return (index);
	return (NOTHING);
}


int evaluate_expression(OBJ_DATA *obj, char *expr)
{
	STACK_DATA ops, vals;
	char *ptr, *end, name[MAX_INPUT_LENGTH];
	int temp, index;
	
	if (!expr || !*expr)	/* Allows opening ( first. */
		return (TRUE);
	
	ops.len = vals.len = 0;
	ptr = expr;

	while (*ptr)
	{
		if (isspace(*ptr))
			ptr++;
		else
		{
			if ((temp = find_oper_num(*ptr)) == NOTHING)
			{
				end = ptr;
				while (*ptr && !isspace(*ptr) && find_oper_num(*ptr) == NOTHING)
					ptr++;
				strncpy(name, end, ptr - end);
				name[ptr - end] = '\0';
				for (index = 0; *extra_bits[index] != '\n'; index++)
				{
					if (!str_cmp(name, extra_bits[index]))
					{
						push(&vals, OBJ_FLAGGED(obj, 1 << index));
						break;
					}
				}
				if (*extra_bits[index] == '\n')
					push(&vals, isname(name, obj->name));
			}
			else
			{
				if (temp != OPER_OPEN_PAREN)
					while (top(&ops) > temp)
						evaluate_operation(&ops, &vals);
				
				if (temp == OPER_CLOSE_PAREN)
				{
					if ((temp = pop(&ops)) != OPER_OPEN_PAREN)
					{
						log("SYSERR: Illegal parenthesis in shop keyword expression.");
						return (FALSE);
					}
				}
				else
					push(&ops, temp);
				ptr++;
			}
		}
	}

	while (top(&ops) != NOTHING)
		evaluate_operation(&ops, &vals);
	temp = pop(&vals);
	if (top(&vals) != NOTHING)
	{
		log("SYSERR: Extra operands left on shop keyword expression stack.");
		return (FALSE);
	}
	return (temp);
}


int trade_with(OBJ_DATA *item, int shop_nr)
{
	int counter;
	
	if (GET_OBJ_COST(item) < 1)
		return (OBJECT_NOVAL);
	
	if (OBJ_FLAGGED(item, ITEM_NOSELL)  ||
	    OBJ_FLAGGED(item, ITEM_DONATED) ||
	    GET_OBJ_TYPE(item) == ITEM_MONEY)
		return (OBJECT_NOTOK);
	
	for (counter = 0; SHOP_BUYTYPE(shop_nr, counter) != NOTHING; counter++)
		if (SHOP_BUYTYPE(shop_nr, counter) == GET_OBJ_TYPE(item))
		{
			if (GET_OBJ_VAL(item, 2) == 0 &&
			    (GET_OBJ_TYPE(item) == ITEM_WAND ||
			     GET_OBJ_TYPE(item) == ITEM_STAFF))
				return (OBJECT_DEAD);
			else if (evaluate_expression(item, SHOP_BUYWORD(shop_nr, counter)))
				return (OBJECT_OK);
		}

	return (OBJECT_NOTOK);
}


int same_obj(OBJ_DATA *obj1, OBJ_DATA *obj2)
{
	int index;
	
	if (!obj1 || !obj2)
		return (obj1 == obj2);
	
	if (GET_OBJ_RNUM(obj1) != GET_OBJ_RNUM(obj2))
		return (FALSE);
	
	if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2))
		return (FALSE);
	
	if (GET_OBJ_EXTRA(obj1) != GET_OBJ_EXTRA(obj2))
		return (FALSE);
	
	for (index = 0; index < MAX_OBJ_AFF; index++)
		if ((obj1->affected[index].location != obj2->affected[index].location) ||
		    (obj1->affected[index].modifier != obj2->affected[index].modifier))
			return (FALSE);
		
	return (TRUE);
}


int shop_producing(OBJ_DATA *item, int shop_nr)
{
	int counter;
	
	if (GET_OBJ_RNUM(item) < 0)
		return (FALSE);
	
	for (counter = 0; SHOP_PRODUCT(shop_nr, counter) != NOTHING; counter++)
		if (same_obj(item, &obj_proto[SHOP_PRODUCT(shop_nr, counter)]))
			return (TRUE);
	return (FALSE);
}


int transaction_amt(char *arg)
{
  int num;
  char *buywhat;

  /*
   * If we have two arguments, it means 'buy 5 3', or buy 5 of #3.
   * We don't do that if we only have one argument, like 'buy 5', buy #5.
   * Code from Andrey Fidrya <andrey@ALEX-UA.COM>
   */
  buywhat = one_argument(arg, buf);
  if (*buywhat && *buf && is_number(buf)) {
    num = atoi(buf);
    strcpy(arg, arg + strlen(buf) + 1);
    return (num);
  }
  return (1);
}


char *times_message(OBJ_DATA *obj, char *name, int num)
{
  static char buf[256];
  char *ptr;
  int len;

  if (obj) {
    strcpy(buf, obj->short_description);
    len = strlen(obj->short_description);
  } else {
    if ((ptr = strchr(name, '.')) == NULL)
      ptr = name;
    else
      ptr++;
    len = sprintf(buf, "%s %s", AN(ptr), ptr);
  }

  if (num > 1)
    sprintf(buf + len, " (x %d)", num);
  return (buf);
}


OBJ_DATA *get_slide_obj_vis(CHAR_DATA *ch, char *name, OBJ_DATA *list)
{
  OBJ_DATA *i, *last_match = NULL;
  int j, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp;

  strcpy(tmpname, name);
  tmp = tmpname;
  if (!(number = get_number(&tmp)))
    return (NULL);

  for (i = list, j = 1; i && (j <= number); i = i->next_content)
    if (isname(tmp, i->name))
      if (CAN_SEE_OBJ(ch, i) && !same_obj(last_match, i)) {
	if (j == number)
	  return (i);
	last_match = i;
	j++;
      }
  return (NULL);
}


OBJ_DATA *get_hash_obj_vis(CHAR_DATA *ch, char *name, OBJ_DATA *list)
{
  OBJ_DATA *loop, *last_obj = NULL;
  int index;

  if (is_number(name))
    index = atoi(name);
  else if (is_number(name + 1))
    index = atoi(name + 1);
  else
    return (NULL);

  for (loop = list; loop; loop = loop->next_content)
    if (CAN_SEE_OBJ(ch, loop) && (loop->obj_flags.cost > 0))
      if (!same_obj(last_obj, loop)) {
	if (--index == 0)
	  return (loop);
	last_obj = loop;
      }
  return (NULL);
}


OBJ_DATA *get_purchase_obj(CHAR_DATA *ch, char *arg, CHAR_DATA *keeper, int shop_nr, int msg)
{
	OBJ_DATA *obj;
	char name[MAX_INPUT_LENGTH];
	
	one_argument(arg, name);
	do
	{
		if (*name == '#' || is_number(name))
			obj = get_hash_obj_vis(ch, name, keeper->first_carrying);
		else
			obj = get_slide_obj_vis(ch, name, keeper->first_carrying);
		if (!obj || GET_OBJ_TYPE(obj) == ITEM_MONEY)
		{
			if (msg)
			{
				char buf[MAX_INPUT_LENGTH];

				sprintf(buf, shop_index[shop_nr].no_such_item1, GET_NAME(ch));
				do_tell(keeper, buf, cmd_tell, 0);
			}
			return (NULL);
		}
		if (GET_OBJ_COST(obj) <= 0)
		{
			extract_obj(obj);
			obj = NULL;
		}
	} while (!obj);

	return (NULL);
}


int buy_price(OBJ_DATA *obj, int shop_nr)
{
  return (GET_OBJ_COST(obj) * SHOP_BUYPROFIT(shop_nr));
}


void shopping_buy(char *arg, CHAR_DATA *ch, CHAR_DATA *keeper, int shop_nr)
{
	OBJ_DATA *obj, *last_obj = NULL;
	char tempstr[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	int goldamt = 0, buynum, bought = 0;
	
	if (!is_ok(keeper, ch, shop_nr))
		return;
	
	if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop_nr);
	
	if ((buynum = transaction_amt(arg)) < 0)
	{
		sprintf(buf, "%s A negative amount?  Try selling me something.",
			GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	if (!*arg || !buynum)
	{
		sprintf(buf, "%s What do you want to buy??", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
		return;
	
	if (buy_price(obj, shop_nr) > get_gold(ch) && !IS_GOD(ch))
	{
		char actbuf[MAX_INPUT_LENGTH];

		sprintf(buf, shop_index[shop_nr].missing_cash2, GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		
		switch (SHOP_BROKE_TEMPER(shop_nr))
		{
		case 0:
			do_action(keeper, strcpy(actbuf, GET_NAME(ch)), cmd_puke, 0);
			return;
		case 1:
			do_echo(keeper, strcpy(actbuf, "smokes on his joint."), cmd_emote, SCMD_EMOTE);
			return;
		default:
			return;
		}
	}

	if (IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))
	{
		sprintf(buf, "%s: You can't carry any more items.\r\n",
			fname(obj->name));
		send_to_char(buf, ch);
		return;
	}

	if (IS_CARRYING_W(ch) + get_real_obj_weight(obj) > CAN_CARRY_W(ch))
	{
		sprintf(buf, "%s: You can't carry that much weight.\r\n",
			fname(obj->name));
		send_to_char(buf, ch);
		return;
	}

	while (obj && (GET_GOLD(ch) >= buy_price(obj, shop_nr) || IS_GOD(ch))
		&& IS_CARRYING_N(ch) < CAN_CARRY_N(ch) && bought < buynum
		&& IS_CARRYING_W(ch) + get_real_obj_weight(obj) <= CAN_CARRY_W(ch))
	{
		bought++;
		/* Test if producing shop ! */
		if (shop_producing(obj, shop_nr))
			obj = read_object(GET_OBJ_RNUM(obj), REAL);
		else
		{
			obj_from_char(obj);
			SHOP_SORT(shop_nr)--;
		}
		obj = obj_to_char(obj, ch);
		
		goldamt += buy_price(obj, shop_nr);
		if (!IS_GOD(ch))
			sub_gold(ch, buy_price(obj, shop_nr));
		
		last_obj = obj;
		obj = get_purchase_obj(ch, arg, keeper, shop_nr, FALSE);
		if (!same_obj(obj, last_obj))
			break;
	}
	
	if (bought < buynum)
	{
		if (!obj || !same_obj(last_obj, obj))
			sprintf(buf, "%s I only have %d to sell you.", GET_NAME(ch), bought);
		else if (get_gold(ch) < buy_price(obj, shop_nr))
			sprintf(buf, "%s You can only afford %d.", GET_NAME(ch), bought);
		else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
			sprintf(buf, "%s You can only hold %d.", GET_NAME(ch), bought);
		else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
			sprintf(buf, "%s You can only carry %d.", GET_NAME(ch), bought);
		else
			sprintf(buf, "%s Something screwy only gave you %d.", GET_NAME(ch),
			bought);
		do_tell(keeper, buf, cmd_tell, 0);
	}
	
	if (!IS_GOD(ch))
		add_gold(keeper, goldamt);
	
	strcpy(tempstr, times_message(ch->first_carrying, 0, bought));
	sprintf(buf, "$n buys %s.", tempstr);
	act(buf, FALSE, ch, obj, 0, TO_ROOM);
	
	sprintf(buf, shop_index[shop_nr].message_buy, GET_NAME(ch), goldamt);
	do_tell(keeper, buf, cmd_tell, 0);
	sprintf(buf, "You now have %s.\r\n", tempstr);
	send_to_char(buf, ch);
	
	if (SHOP_USES_BANK(shop_nr))
	{
		if (get_gold(keeper) > MAX_OUTSIDE_BANK)
		{
			int amount = get_gold(keeper) - MAX_OUTSIDE_BANK;

			SHOP_BANK(shop_nr) += amount;
			sub_gold(keeper, amount);
		}
	}

	save_shop_nonnative(shop_nr, keeper);
}


OBJ_DATA *get_selling_obj(CHAR_DATA *ch, char *name, CHAR_DATA *keeper, int shop_nr, int msg)
{
  OBJ_DATA *obj;
  char buf[MAX_STRING_LENGTH];
  int result;

  if (!(obj = get_obj_in_list_vis_rev(ch, name, NULL, ch->last_carrying))) {
    if (msg) {
      sprintf(buf, shop_index[shop_nr].no_such_item2, GET_NAME(ch));
      do_tell(keeper, buf, cmd_tell, 0);
    }
    return (NULL);
  }
  if ((result = trade_with(obj, shop_nr)) == OBJECT_OK)
    return (obj);

  switch (result) {
  case OBJECT_NOVAL:
    sprintf(buf, "%s You've got to be kidding, that thing is worthless!",
	    GET_NAME(ch));
    break;
  case OBJECT_NOTOK:
    sprintf(buf, shop_index[shop_nr].do_not_buy, GET_NAME(ch));
    break;
  case OBJECT_DEAD:
    sprintf(buf, "%s %s", GET_NAME(ch), MSG_NO_USED_WANDSTAFF);
    break;
  default:
    log("SYSERR: Illegal return value of %d from trade_with() (%s)",
	    result, __FILE__);	/* Someone might rename it... */
    sprintf(buf, "%s An error has occurred.", GET_NAME(ch));
    break;
  }
  if (msg)
    do_tell(keeper, buf, cmd_tell, 0);
  return (NULL);
}


int sell_price(CHAR_DATA *ch, OBJ_DATA *obj, int shop_nr)
{
  return (GET_OBJ_COST(obj) * SHOP_SELLPROFIT(shop_nr));
}


/*
 * This function is a slight hack!  To make sure that duplicate items are
 * only listed once on the "list", this function groups "identical"
 * objects together on the shopkeeper's inventory list.  The hack involves
 * knowing how the list is put together, and manipulating the order of
 * the objects on the list.  (But since most of DIKU is not encapsulated,
 * and information hiding is almost never used, it isn't that big a deal) -JF
 */
OBJ_DATA *slide_obj(OBJ_DATA *obj, CHAR_DATA *keeper, int shop_nr)
{
	OBJ_DATA *loop;
	int temp;
	
	if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop_nr);
	
	/* Extract the object if it is identical to one produced */
	if (shop_producing(obj, shop_nr))
	{
		temp = GET_OBJ_RNUM(obj);
		extract_obj(obj);
		return (&obj_proto[temp]);
	}

	SHOP_SORT(shop_nr)++;
	loop = keeper->first_carrying;
	obj = obj_to_char(obj, keeper);
	keeper->first_carrying = loop;

	while (loop)
	{
		if (same_obj(obj, loop))
		{
			obj->next_content = loop->next_content;
			loop->next_content = obj;
			return (obj);
		}
		loop = loop->next_content;
	}

	keeper->first_carrying = obj;
	return (obj);
}


void sort_keeper_objs(CHAR_DATA *keeper, int shop_nr)
{
	OBJ_DATA *list = NULL, *temp;
	
	while (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
	{
		temp = keeper->first_carrying;
		obj_from_char(temp);
		temp->next_content = list;
		list = temp;
	}
	
	while (list)
	{
		temp = list;
		list = list->next_content;
		if (shop_producing(temp, shop_nr) &&
			!get_obj_in_list_num(GET_OBJ_RNUM(temp), keeper->first_carrying))
		{
			temp = obj_to_char(temp, keeper);
			SHOP_SORT(shop_nr)++;
		} else
			slide_obj(temp, keeper, shop_nr);
	}
}


void shopping_sell(char *arg, CHAR_DATA *ch, CHAR_DATA *keeper, int shop_nr)
{
	OBJ_DATA *obj;
	char tempstr[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
	int sellnum, sold = 0, goldamt = 0;
	
	if (!(is_ok(keeper, ch, shop_nr)))
		return;
	
	if ((sellnum = transaction_amt(arg)) < 0)
	{
		sprintf(buf, "%s A negative amount?  Try buying something.",
			GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	if (!*arg || !sellnum)
	{
		sprintf(buf, "%s What do you want to sell??", GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	one_argument(arg, name);
	if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
		return;
	
	if (get_gold(keeper) + SHOP_BANK(shop_nr) < sell_price(ch, obj, shop_nr))
	{
		sprintf(buf, shop_index[shop_nr].missing_cash1, GET_NAME(ch));
		do_tell(keeper, buf, cmd_tell, 0);
		return;
	}
	while (obj && get_gold(keeper) + SHOP_BANK(shop_nr) >= sell_price(ch, obj, shop_nr) &&
		sold < sellnum)
	{
		sold++;
		
		goldamt += sell_price(ch, obj, shop_nr);
		sub_gold(keeper, sell_price(ch, obj, shop_nr));
		
		obj_from_char(obj);
		slide_obj(obj, keeper, shop_nr);	/* Seems we don't use return value. */
		obj = get_selling_obj(ch, name, keeper, shop_nr, FALSE);
	}
	
	if (sold < sellnum)
	{
		if (!obj)
			sprintf(buf, "%s You only have %d of those.", GET_NAME(ch), sold);
		else if (get_gold(keeper) + SHOP_BANK(shop_nr) < sell_price(ch, obj, shop_nr))
			sprintf(buf, "%s I can only afford to buy %d of those.", GET_NAME(ch), sold);
		else
			sprintf(buf, "%s Something really screwy made me buy %d.", GET_NAME(ch), sold);
		
		do_tell(keeper, buf, cmd_tell, 0);
	}
	add_gold(ch, goldamt);
	strcpy(tempstr, times_message(0, name, sold));
	sprintf(buf, "$n sells %s.", tempstr);
	act(buf, FALSE, ch, obj, 0, TO_ROOM);
	
	sprintf(buf, shop_index[shop_nr].message_sell, GET_NAME(ch), goldamt);
	do_tell(keeper, buf, cmd_tell, 0);
	sprintf(buf, "The shopkeeper now has %s.\r\n", tempstr);
	send_to_char(buf, ch);
	
	if (get_gold(keeper) < MIN_OUTSIDE_BANK)
	{
		goldamt = MIN(MAX_OUTSIDE_BANK - get_gold(keeper), SHOP_BANK(shop_nr));
		SHOP_BANK(shop_nr) -= goldamt;
		add_gold(keeper, goldamt);
	}
	
	save_shop_nonnative(shop_nr, keeper);
}


void shopping_value(char *arg, CHAR_DATA *ch, CHAR_DATA *keeper, int shop_nr)
{
  OBJ_DATA *obj;
  char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];

  if (!is_ok(keeper, ch, shop_nr))
    return;

  if (!*arg) {
    sprintf(buf, "%s What do you want me to evaluate??", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  one_argument(arg, name);
  if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
    return;

  sprintf(buf, "%s I'll give you %d gold coins for that!", GET_NAME(ch),
	  sell_price(ch, obj, shop_nr));
  do_tell(keeper, buf, cmd_tell, 0);

  return;
}


char *list_object(OBJ_DATA *obj, int cnt, int index, int shop_nr)
{
	static char buf[256];
	char buf2[300], buf3[200];
	
	if (shop_producing(obj, shop_nr))
		strcpy(buf2, "Unlimited   ");
	else
		sprintf(buf2, "%5d       ", cnt);
	sprintf(buf, " %2d)  %s", index, buf2);
	
	/* Compile object name and information */
	strcpy(buf3, obj->short_description);
	if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON && GET_OBJ_VAL(obj, 1))
		sprintf(END_OF(buf3), " of %s", drinks[GET_OBJ_VAL(obj, 2)]);
	
	/* FUTURE: */
	/* Add glow/hum/etc */
	
	if (GET_OBJ_TYPE(obj) == ITEM_WAND || GET_OBJ_TYPE(obj) == ITEM_STAFF)
		if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1))
			strcat(buf3, " (partially used)");
		
	sprintf(buf2, "%-48s %6d\r\n", buf3, buy_price(obj, shop_nr));
	strcat(buf, CAP(buf2));
	return (buf);
}


void shopping_list(char *arg, CHAR_DATA *ch, CHAR_DATA *keeper, int shop_nr)
{
	OBJ_DATA *obj, *last_obj = NULL;
	char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
	int cnt = 0, index = 0, found = FALSE;
	/* cnt is the number of that particular object available */
	
	if (!is_ok(keeper, ch, shop_nr))
		return;
	
	if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
		sort_keeper_objs(keeper, shop_nr);
	
	one_argument(arg, name);
	strcpy(buf, " ##   Available   Item                                               Cost\r\n");
	strcat(buf, "-------------------------------------------------------------------------\r\n");
	if (keeper->first_carrying)
	{
		for (obj = keeper->first_carrying; obj; obj = obj->next_content)
		{
			if (CAN_SEE_OBJ(ch, obj) && GET_OBJ_COST(obj) > 0 &&
				GET_OBJ_TYPE(obj) != ITEM_MONEY )
			{
				if (!last_obj)
				{
					last_obj = obj;
					cnt = 1;
				}
				else if (same_obj(last_obj, obj))
					cnt++;
				else
				{
					index++;
					if (!*name || isname(name, last_obj->name))
					{
						strcat(buf, list_object(last_obj, cnt, index, shop_nr));
						found = TRUE;
					}
					cnt = 1;
					last_obj = obj;
				}
			}
		}
	}
	index++;
	if (!last_obj) /* we actually have nothing in our list for sale, period */
		strcpy(buf, "Currently, there is nothing for sale.\r\n");
	else if (!*name || isname(name, last_obj->name)) /* show last obj */
		strcat(buf, list_object(last_obj, cnt, index, shop_nr));
	else if (!found) /* nothing the char was looking for was found */
		strcpy(buf, "Presently, none of those are for sale.\r\n");
	
	page_string(ch->desc, buf, TRUE);
}


int ok_shop_room(int shop_nr, ROOM_DATA *room)
{
	int index;
	
	for (index = 0; SHOP_ROOM(shop_nr, index) != NULL; index++)
		if (SHOP_ROOM(shop_nr, index) == room)
			return (TRUE);
	return (FALSE);
}


SPECIAL(shop_keeper)
{
	CHAR_DATA *keeper = (CHAR_DATA *) me;
	char argm[MAX_INPUT_LENGTH];
	int shop_nr;
	
	for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
	{
		if (SHOP_KEEPER(shop_nr) == keeper->nr)
			break;
	}
	
	if (shop_nr > top_shop)
		return (FALSE);
	
	if (SHOP_FUNC(shop_nr))	/* Check secondary function */
	{
		if ((SHOP_FUNC(shop_nr)) (ch, me, cmd, argument))
			return (TRUE);
	}
	
	if (keeper == ch)
	{
		if (cmd)
			SHOP_SORT(shop_nr) = 0;	/* Safety in case "drop all" */
		return (FALSE);
	}

	if (!ok_shop_room(shop_nr, ch->in_room))
		return (0);
	
	if (!AWAKE(keeper))
		return (FALSE);
	
	if (CMD_IS("steal"))
	{
		sprintf(argm, "$N shouts '%s'", MSG_NO_STEAL_HERE);
		do_action(keeper, GET_NAME(ch), cmd_slap, 0);
		act(argm, FALSE, ch, 0, keeper, TO_CHAR);
		return (TRUE);
	}
	
	if (CMD_IS("buy"))
	{
		shopping_buy(argument, ch, keeper, shop_nr);
		return (TRUE);
	}
	else if (CMD_IS("sell"))
	{
		shopping_sell(argument, ch, keeper, shop_nr);
		return (TRUE);
	}
	else if (CMD_IS("value"))
	{
		shopping_value(argument, ch, keeper, shop_nr);
		return (TRUE);
	}
	else if (CMD_IS("list"))
	{
		shopping_list(argument, ch, keeper, shop_nr);
		return (TRUE);
	}
	
	return (FALSE);
}


int ok_damage_shopkeeper(CHAR_DATA *ch, CHAR_DATA *victim)
{
  char buf[MAX_INPUT_LENGTH];
  int index;

  /* Prevent "invincible" shopkeepers if they're charmed. */
  if (AFF_FLAGGED(victim, AFF_CHARM))
    return (TRUE);

  if (IS_MOB(victim) && mob_index[GET_MOB_RNUM(victim)].func == shop_keeper)
    for (index = 0; index <= top_shop; index++)
      if (GET_MOB_RNUM(victim) == SHOP_KEEPER(index) && !SHOP_KILL_CHARS(index))
      {
	do_action(victim, GET_NAME(ch), cmd_slap, 0);
	sprintf(buf, "%s %s", GET_NAME(ch), MSG_CANT_KILL_KEEPER);
	do_tell(victim, buf, cmd_tell, 0);
	return (FALSE);
      }
  return (TRUE);
}


/* val == obj_vnum and obj_rnum (?) */
int add_to_list(SHOP_BUY_DATA *list, int type, int *len, int *val)
{
	if (*val != NOTHING)
	{
		if (*len < MAX_SHOP_OBJ)
		{
			if (type == LIST_PRODUCE)
				*val = real_object(*val);
			if (*val != NOTHING)
			{
				BUY_TYPE(list[*len]) = *val;
				BUY_WORD(list[(*len)++]) = 0;
			}
			else
				*val = NOTHING;
			return (FALSE);
		}
		else
			return (TRUE);
	}
	return (FALSE);
}


int end_read_list(SHOP_BUY_DATA *list, int len, int error)
{
	if (error)
		log("SYSERR: Raise MAX_SHOP_OBJ constant in shop.h to %d", len + error);
	BUY_WORD(list[len]) = NULL;
	BUY_TYPE(list[len++]) = NOTHING;
	return (len);
}


void read_line(FILE *shop_f, const char *string, void *data)
{
	if (!get_line(shop_f, buf) || !sscanf(buf, string, data))
	{
		log("SYSERR: Error in shop #%d\n", SHOP_NUM(top_shop));
		exit(1);
	}
}


int read_list(FILE *shop_f, SHOP_BUY_DATA *list, int new_format, int max, int type)
{
	int count, temp, len = 0, error = 0;
	
	if (new_format)
	{
		do
		{
			read_line(shop_f, "%d", &temp);
			error += add_to_list(list, type, &len, &temp);
		} while (temp >= 0);
	}
	else
		for (count = 0; count < max; count++)
		{
			read_line(shop_f, "%d", &temp);
			error += add_to_list(list, type, &len, &temp);
		}
	
	return (end_read_list(list, len, error));
}

/* END_OF inefficient. */
int read_type_list(FILE *shop_f, SHOP_BUY_DATA *list, int new_format, int max)
{
  int index, num, len = 0, error = 0;
  char *ptr;

  if (!new_format)
    return (read_list(shop_f, list, 0, max, LIST_TRADE));
  do {
    fgets(buf, MAX_STRING_LENGTH - 1, shop_f);
    if ((ptr = strchr(buf, ';')) != NULL)
      *ptr = '\0';
    else
      *(END_OF(buf) - 1) = '\0';
    for (index = 0, num = NOTHING; *item_types[index] != '\n'; index++)
      if (!strn_cmp(item_types[index], buf, strlen(item_types[index]))) {
	num = index;
	strcpy(buf, buf + strlen(item_types[index]));
	break;
      }
    ptr = buf;
    if (num == NOTHING) {
      sscanf(buf, "%d", &num);
      while (!isdigit(*ptr))
	ptr++;
      while (isdigit(*ptr))
	ptr++;
    }
    while (isspace(*ptr))
      ptr++;
    while (isspace(*(END_OF(ptr) - 1)))
      *(END_OF(ptr) - 1) = '\0';
    error += add_to_list(list, LIST_TRADE, &len, &num);
    if (*ptr)
      BUY_WORD(list[len - 1]) = str_dup(ptr);
  } while (num >= 0);
  return (end_read_list(list, len, error));
}


char *read_shop_message(int mnum, room_vnum shr, FILE *shop_f, const char *why)
{
  int cht, ss = 0, ds = 0, err = 0;
  char *tbuf;

  if (!(tbuf = fread_string(shop_f, why)))
    return (NULL);

  for (cht = 0; tbuf[cht]; cht++) {
    if (tbuf[cht] != '%')
      continue;

    if (tbuf[cht + 1] == 's')
      ss++;
    else if (tbuf[cht + 1] == 'd' && (mnum == 5 || mnum == 6)) {
      if (ss == 0) {
        log("SYSERR: Shop #%d has %%d before %%s, message #%d.", shr, mnum);
        err++;
      }
      ds++;
    } else if (tbuf[cht + 1] != '%') {
      log("SYSERR: Shop #%d has invalid format '%%%c' in message #%d.", shr, tbuf[cht + 1], mnum);
      err++;
    }
  }

  if (ss > 1 || ds > 1) {
    log("SYSERR: Shop #%d has too many specifiers for message #%d. %%s=%d %%d=%d", shr, mnum, ss, ds);
    err++;
  }

  if (err) {
    free(tbuf);
    return (NULL);
  }
  return (tbuf);
}


void boot_the_shops(FILE *shop_f, char *filename, int rec_count)
{
	SHOP_BUY_DATA list[MAX_SHOP_OBJ + 1];
	char *buf, buf2[MAX_INPUT_LENGTH];
	int temp, count, new_format = FALSE;
	int done = FALSE;
	
	sprintf(buf2, "beginning of shop file %s", filename);
	
	while (!done)
	{
		buf = fread_string(shop_f, buf2);
		if (*buf == '#')		/* New shop */
		{
			sscanf(buf, "#%d\n", &temp);
			sprintf(buf2, "shop #%d in shop file %s", temp, filename);
			free(buf);		/* Plug memory leak! */

			top_shop++;
			if (!top_shop)
				CREATE(shop_index, SHOP_DATA, rec_count);
			
			SHOP_NUM(top_shop) = temp;

			temp = read_list(shop_f, list, new_format, MAX_PROD, LIST_PRODUCE);

			CREATE(shop_index[top_shop].producing, obj_vnum, temp);
			for (count = 0; count < temp; count++)
				SHOP_PRODUCT(top_shop, count) = BUY_TYPE(list[count]);
			
			read_line(shop_f, "%f", &SHOP_BUYPROFIT(top_shop));
			read_line(shop_f, "%f", &SHOP_SELLPROFIT(top_shop));
			
			temp = read_type_list(shop_f, list, new_format, MAX_TRADE);

			CREATE(shop_index[top_shop].type, SHOP_BUY_DATA, temp);
			for (count = 0; count < temp; count++)
			{
				SHOP_BUYTYPE(top_shop, count) = BUY_TYPE(list[count]);
				SHOP_BUYWORD(top_shop, count) = BUY_WORD(list[count]);
			}
			
			shop_index[top_shop].no_such_item1	= read_shop_message(0, SHOP_NUM(top_shop), shop_f, buf2);
			shop_index[top_shop].no_such_item2	= read_shop_message(1, SHOP_NUM(top_shop), shop_f, buf2);
			shop_index[top_shop].do_not_buy		= read_shop_message(2, SHOP_NUM(top_shop), shop_f, buf2);
			shop_index[top_shop].missing_cash1	= read_shop_message(3, SHOP_NUM(top_shop), shop_f, buf2);
			shop_index[top_shop].missing_cash2	= read_shop_message(4, SHOP_NUM(top_shop), shop_f, buf2);
			shop_index[top_shop].message_buy	= read_shop_message(5, SHOP_NUM(top_shop), shop_f, buf2);
			shop_index[top_shop].message_sell	= read_shop_message(6, SHOP_NUM(top_shop), shop_f, buf2);
			
			read_line(shop_f, "%d", &SHOP_BROKE_TEMPER(top_shop));
			read_line(shop_f, "%d", &SHOP_BITVECTOR(top_shop));
			read_line(shop_f, "%hd", &SHOP_KEEPER(top_shop));
			
			SHOP_KEEPER(top_shop)			= real_mobile(SHOP_KEEPER(top_shop));
			read_line(shop_f, "%d", &SHOP_TRADE_WITH(top_shop));
			
			temp = read_list(shop_f, list, new_format, 1, LIST_ROOM);
			CREATE(shop_index[top_shop].in_room, ROOM_DATA *, temp);
			for (count = 0; count < temp; count++)
				SHOP_ROOM(top_shop, count) = get_room(BUY_TYPE(list[count]));
			
			read_line(shop_f, "%d", &SHOP_OPEN1(top_shop));
			read_line(shop_f, "%d", &SHOP_CLOSE1(top_shop));
			read_line(shop_f, "%d", &SHOP_OPEN2(top_shop));
			read_line(shop_f, "%d", &SHOP_CLOSE2(top_shop));
			
			SHOP_BANK(top_shop) = 0;
			SHOP_SORT(top_shop) = 0;
			SHOP_FUNC(top_shop) = NULL;
		}
		else
		{
			if (*buf == '$')		/* EOF */
				done = TRUE;
			else if (strstr(buf, VERSION3_TAG))	/* New format marker */
				new_format = TRUE;
			free(buf);		/* Plug memory leak! */
		}
	}
}


void assign_the_shopkeepers(void)
{
  int index;

  cmd_say = find_command("say");
  cmd_tell = find_command("tell");
  cmd_emote = find_command("emote");
  cmd_slap = find_command("slap");
  cmd_puke = find_command("puke");
  for (index = 0; index <= top_shop; index++) {
    if (SHOP_KEEPER(index) == NOBODY)
      continue;
    if (mob_index[SHOP_KEEPER(index)].func)
      SHOP_FUNC(index) = mob_index[SHOP_KEEPER(index)].func;
    mob_index[SHOP_KEEPER(index)].func = shop_keeper;
  }
}


char *customer_string(int shop_nr, int detailed)
{
  int index, cnt = 1;
  static char buf[256];

  *buf = 0;
  for (index = 0; *trade_letters[index] != '\n'; index++, cnt *= 2)
    if (!(SHOP_TRADE_WITH(shop_nr) & cnt)) {
      if (detailed) {
	if (*buf)
	  strcat(buf, ", ");
	strcat(buf, trade_letters[index]);
      } else
	sprintf(END_OF(buf), "%c", *trade_letters[index]);
    } else if (!detailed)
      strcat(buf, "_");

  return (buf);
}


/* END_OF inefficient */
void list_all_shops(CHAR_DATA *ch)
{
  int shop_nr;

  *buf = '\0';
  for (shop_nr = 0; shop_nr <= top_shop; shop_nr++) {
    if (!(shop_nr % (PAGE_LENGTH - 2))) {
      strcat(buf, " ##   Virtual   Where    Keeper    Buy   Sell   Customers\r\n");
      strcat(buf, "---------------------------------------------------------\r\n");
    }
    sprintf(buf2, "%3d   %6d   %6d    ", shop_nr + 1, SHOP_NUM(shop_nr),
	    SHOP_ROOM(shop_nr, 0)->number);
    if (SHOP_KEEPER(shop_nr) < 0)
      strcpy(buf1, "<NONE>");
    else
      sprintf(buf1, "%6d", mob_index[SHOP_KEEPER(shop_nr)].vnum);
    sprintf(END_OF(buf2), "%s   %3.2f   %3.2f    ", buf1,
	    SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr));
    strcat(buf2, customer_string(shop_nr, FALSE));
    sprintf(END_OF(buf), "%s\r\n", buf2);
  }

  page_string(ch->desc, buf, TRUE);
}


void handle_detailed_list(char *buf, char *buf1, CHAR_DATA *ch)
{
  if (strlen(buf1) + strlen(buf) < 78 || strlen(buf) < 20)
    strcat(buf, buf1);
  else {
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    sprintf(buf, "            %s", buf1);
  }
}


void list_detailed_shop(CHAR_DATA *ch, int shop_nr)
{
  OBJ_DATA *obj;
  CHAR_DATA *k;
  int index;
  ROOM_DATA *temp;

  sprintf(buf, "Vnum:       [%5d], Rnum: [%5d]\r\n", SHOP_NUM(shop_nr),
	  shop_nr + 1);
  send_to_char(buf, ch);

  strcpy(buf, "Rooms:      ");
  for (index = 0; SHOP_ROOM(shop_nr, index) != NULL; index++)
  {
    if (index)
      strcat(buf, ", ");
    if ((temp = SHOP_ROOM(shop_nr, index)) != NULL)
      sprintf(buf1, "%s (#%d)", temp->name, temp->number);
    else
      sprintf(buf1, "<UNKNOWN> (#%d)", SHOP_ROOM(shop_nr, index));
    handle_detailed_list(buf, buf1, ch);
  }
  if (!index)
    send_to_char("Rooms:      None!\r\n", ch);
  else {
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
  }

  strcpy(buf, "Shopkeeper: ");
  if (SHOP_KEEPER(shop_nr) >= 0) {
    sprintf(END_OF(buf), "%s (#%d), Special Function: %s\r\n",
	    GET_NAME(&mob_proto[SHOP_KEEPER(shop_nr)]),
	mob_index[SHOP_KEEPER(shop_nr)].vnum, YESNO(SHOP_FUNC(shop_nr)));
    if ((k = get_char_num(SHOP_KEEPER(shop_nr)))) {
      send_to_char(buf, ch);
      sprintf(buf, "Coins:      [%9d], Bank: [%9d] (Total: %d)\r\n",
	 get_gold(k), SHOP_BANK(shop_nr), get_gold(k) + SHOP_BANK(shop_nr));
    }
  } else
    strcat(buf, "<NONE>\r\n");
  send_to_char(buf, ch);

  strcpy(buf1, customer_string(shop_nr, TRUE));
  sprintf(buf, "Customers:  %s\r\n", (*buf1) ? buf1 : "None");
  send_to_char(buf, ch);

  strcpy(buf, "Produces:   ");
  for (index = 0; SHOP_PRODUCT(shop_nr, index) != NOTHING; index++) {
    obj = &obj_proto[SHOP_PRODUCT(shop_nr, index)];
    if (index)
      strcat(buf, ", ");
    sprintf(buf1, "%s (#%d)", obj->short_description,
	    obj_index[SHOP_PRODUCT(shop_nr, index)].vnum);
    handle_detailed_list(buf, buf1, ch);
  }
  if (!index)
    send_to_char("Produces:   Nothing!\r\n", ch);
  else {
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
  }

  strcpy(buf, "Buys:       ");
  for (index = 0; SHOP_BUYTYPE(shop_nr, index) != NOTHING; index++) {
    if (index)
      strcat(buf, ", ");
    sprintf(buf1, "%s (#%d) ", item_types[SHOP_BUYTYPE(shop_nr, index)],
	    SHOP_BUYTYPE(shop_nr, index));
    if (SHOP_BUYWORD(shop_nr, index))
      sprintf(END_OF(buf1), "[%s]", SHOP_BUYWORD(shop_nr, index));
    else
      strcat(buf1, "[all]");
    handle_detailed_list(buf, buf1, ch);
  }
  if (!index)
    send_to_char("Buys:       Nothing!\r\n", ch);
  else {
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
  }

  sprintf(buf, "Buy at:     [%4.2f], Sell at: [%4.2f], Open: [%d-%d, %d-%d]%s",
     SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr), SHOP_OPEN1(shop_nr),
   SHOP_CLOSE1(shop_nr), SHOP_OPEN2(shop_nr), SHOP_CLOSE2(shop_nr), "\r\n");

  send_to_char(buf, ch);

  sprintbit(SHOP_BITVECTOR(shop_nr), shop_bits, buf1);
  sprintf(buf, "Bits:       %s\r\n", buf1);
  send_to_char(buf, ch);
}


void show_shops(CHAR_DATA *ch, char *arg)
{
  int shop_nr;

  if (!*arg)
    list_all_shops(ch);
  else {
    if (!str_cmp(arg, ".")) {
      for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
	if (ok_shop_room(shop_nr, ch->in_room))
	  break;

      if (shop_nr > top_shop) {
	send_to_char("This isn't a shop!\r\n", ch);
	return;
      }
    } else if (is_number(arg))
      shop_nr = atoi(arg) - 1;
    else
      shop_nr = -1;

    if (shop_nr < 0 || shop_nr > top_shop) {
      send_to_char("Illegal shop number.\r\n", ch);
      return;
    }
    list_detailed_shop(ch, shop_nr);
  }
}
