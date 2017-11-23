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
*   File: act.item.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
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
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"

/* extern variables */
extern room_vnum donation_room_1;
#if 0
extern room_vnum donation_room_2;  /* uncomment if needed! */
extern room_vnum donation_room_3;  /* uncomment if needed! */
#endif

/* local functions */
int can_take_obj(CHAR_DATA *ch, OBJ_DATA *obj);
void weight_change_object(OBJ_DATA *obj, int weight);
void name_from_drinkcon(OBJ_DATA *obj);
void name_to_drinkcon(OBJ_DATA *obj, int type);
void wear_message(CHAR_DATA *ch, OBJ_DATA *obj, int where);
void perform_wear(CHAR_DATA *ch, OBJ_DATA *obj, int where);
int find_eq_pos(CHAR_DATA *ch, OBJ_DATA *obj, char *arg);
void perform_remove(CHAR_DATA *ch, int pos);
ACMD(do_remove);
ACMD(do_put);
ACMD(do_get);
ACMD(do_drop);
ACMD(do_give);
ACMD(do_drink);
ACMD(do_eat);
ACMD(do_pour);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_grab);
ACMD(do_charge);
ACMD(do_discharge);

/* ********************************************************************* */
/* GET Routines                                                          */
/* ********************************************************************* */
/*
 *  Return if an object can be put in their hands (hands empty?)
 */
bool hand_empty( CHAR_DATA *ch )
{
	if ( GET_EQ(ch, WEAR_HOLD) == NULL )	return (TRUE);
	if ( GET_EQ(ch, WEAR_WIELD) == NULL )	return (TRUE);
	
	return (FALSE);
}

int can_take_obj(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if ( IS_CARRYING_N(ch) + obj->count > CAN_CARRY_N(ch) )
	{
		act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}
	
	if ( (IS_CARRYING_W(ch) + get_real_obj_weight(obj)) > CAN_CARRY_W(ch) )
	{
		act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}
	
	if ( !(CAN_WEAR(obj, ITEM_WEAR_TAKE)) )
	{
		act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}

	// shouldn't be needed because furniture shouldn't be taken, but
	// better safer than sorry.. :-))
	if (GET_OBJ_TYPE(obj) == ITEM_FURNITURE)
	{
		if ( obj->people )
			act("$p: you can't take that, it's occupied!", FALSE, ch, obj, 0, TO_CHAR);
		else
			act("$p: you can't take that.", FALSE, ch, obj, 0, TO_CHAR);
		return (0);
	}

	if ( !hand_empty( ch ) )
	{
		act("$p: your hands are full.", FALSE, ch, obj, 0, TO_CHAR );
		return (0);
	}

	return (1);
}


int get_obj_from_room(CHAR_DATA *ch, OBJ_DATA *obj)
{
	char sbaf[256];

	if ( !can_take_obj(ch, obj) )
		return (0);

	if ( check_trap(ch, obj, TRAP_ACT_GET) )
		return (0);

	/* message to char */
	if ( obj->count > 1 )
		sprintf( sbaf, "You get %d $p.", obj->count );
	else
		strcpy( sbaf, "You get $p." );
	act(sbaf, FALSE, ch, obj, 0, TO_CHAR);

	/* message to room */
	if ( obj->count > 1 )
		strcpy( sbaf, "$n gets some $o." );
	else
		strcpy( sbaf, "$n gets $p." );
	act(sbaf, TRUE, ch, obj, 0, TO_ROOM);

	obj_from_room(obj);
	obj = obj_to_char(obj, ch);
		
	/* Room Trigger Events */
	if ( ch->in_room->trigger )
		check_room_trigger(ch, TRIG_ACT_GET);

	return (1);
}

int get_money_from_room(CHAR_DATA *ch, OBJ_DATA *obj)
{
	char sbaf[256];

	if ( !can_take_obj(ch, obj) )
		return (0);

	if ( check_trap(ch, obj, TRAP_ACT_GET) )
		return (0);

	/* message to char */
	if ( obj->count > 1 )
		sprintf( sbaf, "You get %d $p.", obj->count );
	else
		strcpy( sbaf, "You get $p." );
	act(sbaf, FALSE, ch, obj, 0, TO_CHAR);

	/* message to room */
	if ( obj->count > 1 )
		strcpy( sbaf, "$n gets some $o." );
	else
		strcpy( sbaf, "$n gets $p." );
	act(sbaf, TRUE, ch, obj, 0, TO_ROOM);

	obj_from_room(obj);
	obj = obj_to_char(obj, ch);
		
	/* Room Trigger Events */
	if ( ch->in_room->trigger )
		check_room_trigger(ch, TRIG_ACT_GET);

	return (1);
}

void perform_get_obj_from_container(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *cont, int mode)
{
	if (mode == FIND_OBJ_INV || can_take_obj(ch, obj))
	{
		if (IS_CARRYING_N(ch) + obj->count >= CAN_CARRY_N(ch))
			act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
		else
		{
			char sbaf[256];

			if ( check_trap( ch, obj, TRAP_ACT_GET ) )
				return;

			/* message to char */
			if ( obj->count > 1 )
				sprintf( sbaf, "You get %d $p from $P.", obj->count );
			else
				strcpy( sbaf, "You get $p from $P." );
			act(sbaf, FALSE, ch, obj, cont, TO_CHAR);

			/* message to room */
			if ( obj->count > 1 )
				strcpy( sbaf, "$n gets some $o from $P." );
			else
				strcpy( sbaf, "$n gets $p from $P." );
			act(sbaf, TRUE, ch, obj, cont, TO_ROOM);

			obj_from_obj(obj);
			obj = obj_to_char(obj, ch);
		}
	}
}

void get_obj_from_container(CHAR_DATA *ch, OBJ_DATA *cont, char *arg, int mode, int howmany, bool bNumArg)
{
	OBJ_DATA *obj, *next_obj;
	int obj_dotmode, found = 0;
	
	if (GET_OBJ_TYPE(cont) != ITEM_FURNITURE && OBJVAL_FLAGGED(cont, CONT_CLOSED))
	{
		act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
		return;
	}

	obj_dotmode = find_all_dots(arg);
	
	if (obj_dotmode == FIND_INDIV)
	{
		if (!(obj = get_obj_in_list_vis_rev(ch, arg, NULL, cont->last_content)))
		{
			sprintf(buf, "There doesn't seem to be %s %s in $p.", AN(arg), arg);
			act(buf, FALSE, ch, cont, 0, TO_CHAR);
			return;
		}

		if ( GET_OBJ_TYPE(obj) == ITEM_MONEY )
		{
			if ( bNumArg )
			{
				int amount = GET_OBJ_VAL(obj, 0);
				
				// she wants more than what's there..
				if ( howmany > amount )
				{
					send_to_char("You don't have that much gold.\r\n", ch);
					return;
				}
				// she wants less than what's there..
				else if ( howmany < amount )
				{
					int diff = amount - howmany;
					
					// create a new money obj for the rest..
					create_amount(diff, NULL, NULL, cont, NULL, TRUE);
					// set the value of the existing object
					// to what has to be taken
					GET_OBJ_VAL(obj, 0) = howmany;
					// let's cascade till perform_get_obj_from_container()
				}
				// she wants what's there..
				// let's cascade till perform_get_obj_from_container()
			}
		}
		else
		{
			if ( howmany > obj->count )
			{
				act("There aren't so many $o here.", FALSE, ch, obj, NULL, TO_CHAR);
				return;
			}
			split_obj(obj, howmany);
		}

		perform_get_obj_from_container(ch, obj, cont, mode);
	}
	else
	{
		if (obj_dotmode == FIND_ALLDOT && !*arg)
		{
			send_to_char("Get all of what?\r\n", ch);
			return;
		}
		for (obj = cont->first_content; obj; obj = next_obj)
		{
			next_obj = obj->next_content;

			if (CAN_SEE_OBJ(ch, obj) &&
				(obj_dotmode == FIND_ALL || isname(arg, obj->name)))
			{
				found = 1;
				perform_get_obj_from_container(ch, obj, cont, mode);
			}
		}
		if (!found)
		{
			if (obj_dotmode == FIND_ALL)
				act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
			else
			{
				sprintf(buf, "You can't seem to find any %ss in $p.", arg);
				act(buf, FALSE, ch, cont, 0, TO_CHAR);
			}
		}
	}
}

/*
 * Syntax:
 *
 * - get <obj>
 * - get <num> <obj>
 * - get all.<obj>
 * - get all
 * - get <obj> <container>
 * - get <obj> all.<container>
 * - get <num> <obj> <container>
 * - get <num> <obj> all.<container>
 * - get all.<obj> <container>
 * - get all.<obj> all.<container>
 * - get all <container>
 * - get all all.<container>
 */
ACMD(do_get)
{
	OBJ_DATA *obj;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	bool bNumArg = FALSE;
	int onum = 1, dotmode;

	if ( !*argument )
	{
		send_to_char("Get what?\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg1);
	
	if ( is_number(arg1) )
	{
		bNumArg = TRUE;
		onum = atoi(arg1);
		if ( onum < 1 )
		{
			send_to_char("Don't be stupid!\r\n", ch);
			return;
		}
		argument = one_argument(argument, arg1);
	}

	argument = one_argument(argument, arg2);
	if ( !str_cmp( arg2, "from" ) )
		argument = one_argument(argument, arg2);

	/*
	 * ok.. now:
	 * arg1 holds obj name or 'all'
	 * arg2 holds container name ( if any )
	 */

	/* no container, get from room or from furniture */
	if ( !*arg2 )
	{
		dotmode = find_all_dots(arg1);
	
		// get <obj> or get <num> <obj>
		if (dotmode == FIND_INDIV)
		{
			if ( !(obj = get_obj_in_list_vis_rev(ch, arg1, NULL, ch->in_room->last_content)) )
			{
				OBJ_DATA *furn;

				for ( furn = ch->in_room->first_content; furn; furn = furn->next_content )
				{
					if ( GET_OBJ_TYPE(furn) != ITEM_FURNITURE )
						continue;

					if ( ( obj = get_obj_in_list_vis_rev(ch, arg1, NULL, furn->last_content) ) )
					{
						perform_get_obj_from_container(ch, obj, furn, FIND_OBJ_ROOM);
						return;
					}
				}

				ch_printf(ch, "You don't see %s %s here..\r\n", AN(arg1), arg1);
				return;
			}

			if ( GET_OBJ_TYPE(obj) == ITEM_MONEY )
			{
				if ( bNumArg )
				{
					int amount = GET_OBJ_VAL(obj, 0);
					
					// she wants get more than what's there..
					if ( onum > amount )
					{
						send_to_char("There isn't that much gold.\r\n", ch);
						return;
					}
					// she wants get less than what's there..
					else if ( onum < amount )
					{
						int diff = amount - onum;
						
						// create a new money obj for the rest..
						create_amount(diff, NULL, ch->in_room, NULL, NULL, TRUE);
						// set the value of the existing object
						// to what has to be taken
						GET_OBJ_VAL(obj, 0) = onum;
						// let's cascade till get_obj_from_room()
					}
					// she wants get what's there..
					// let's cascade till get_obj_from_room()
				}
			}
			else
			{
				if ( onum > obj->count )
				{
					act("There aren't so many $o here.", FALSE, ch, obj, NULL, TO_CHAR);
					return;
				}
				split_obj( obj, onum );
			}
			get_obj_from_room(ch, obj);
		}
		// get all.<obj> or get all
		else
		{
			OBJ_DATA *next_obj;
			bool found = FALSE;

			if (dotmode == FIND_ALLDOT && !*arg1)
			{
				send_to_char("Get all of what?\r\n", ch);
				return;
			}
			
			for (obj = ch->in_room->first_content; obj; obj = next_obj)
			{
				next_obj = obj->next_content;
			
				if ( GET_OBJ_TYPE(obj) == ITEM_FURNITURE )
				{
					OBJ_DATA *fobj, *fobj_next = NULL;

					for (fobj = obj->first_content; fobj; fobj = fobj_next)
					{
						if (CAN_SEE_OBJ(ch, fobj) &&
							(dotmode == FIND_ALL || isname(arg1, fobj->name)))
						{
							found = TRUE;
							perform_get_obj_from_container(ch, fobj, obj, FIND_OBJ_ROOM);
						}
					}
				}
				else
				{
					if (CAN_SEE_OBJ(ch, obj) &&
						(dotmode == FIND_ALL || isname(arg1, obj->name)))
					{
						found = TRUE;
						get_obj_from_room(ch, obj);
					}
				}
			}
			
			if (!found)
			{
				if (dotmode == FIND_ALL)
					send_to_char("There doesn't seem to be anything here.\r\n", ch);
				else
					ch_printf(ch, "You don't see any %ss here.\r\n", arg1);
			}
			
		}
	}
	/* get from container, either in room or inventory */
	else
	{
		OBJ_DATA *cont;
		CHAR_DATA *tmp_char;
		int mode, cont_dotmode = find_all_dots(arg2);
		
		if (cont_dotmode == FIND_INDIV)
		{
			mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
			if (!cont)
			{
				sprintf(buf, "You don't have %s %s.\r\n", AN(arg2), arg2);
				send_to_char(buf, ch);
			}
			else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER && 
					GET_OBJ_TYPE(cont) != ITEM_MISSILE_CONT &&
					GET_OBJ_TYPE(cont) != ITEM_FURNITURE)
				act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
			else
				get_obj_from_container(ch, cont, arg1, mode, onum, bNumArg);
		}
		else
		{
			bool found = FALSE;
			
			if (cont_dotmode == FIND_ALLDOT && !*arg2)
			{
				send_to_char("Get from all of what?\r\n", ch);
				return;
			}
			
			for (cont = ch->first_carrying; cont; cont = cont->next_content)
			{
				if (CAN_SEE_OBJ(ch, cont) &&
					(cont_dotmode == FIND_ALL || isname(arg2, cont->name)))
				{
					if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER ||
					    GET_OBJ_TYPE(cont) == ITEM_MISSILE_CONT ||
					    GET_OBJ_TYPE(cont) == ITEM_FURNITURE)
					{
						found = TRUE;
						get_obj_from_container(ch, cont, arg1, FIND_OBJ_INV, onum, bNumArg);
					}
					else if (cont_dotmode == FIND_ALLDOT)
					{
						found = TRUE;
						act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
					}
				}
			}
			
			for (cont = ch->in_room->first_content; cont; cont = cont->next_content)
			{
				if (CAN_SEE_OBJ(ch, cont) &&
					(cont_dotmode == FIND_ALL || isname(arg2, cont->name)))
				{
					if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER ||
					    GET_OBJ_TYPE(cont) == ITEM_MISSILE_CONT ||
					    GET_OBJ_TYPE(cont) == ITEM_FURNITURE)
					{
						get_obj_from_container(ch, cont, arg1, FIND_OBJ_ROOM, onum, bNumArg);
						found = TRUE;
					}
					else if (cont_dotmode == FIND_ALLDOT)
					{
						act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
						found = TRUE;
					}
				}
			}
			
			if (!found)
			{
				if (cont_dotmode == FIND_ALL)
					send_to_char("You can't seem to find any containers.\r\n", ch);
				else
				{
					sprintf(buf, "You can't seem to find any %ss here.\r\n", arg2);
					send_to_char(buf, ch);
				}
			}
		}
	}
}

/* ********************************************************************* */
/* DROP Routines                                                         */
/* ********************************************************************* */

void perform_drop_obj(CHAR_DATA *ch, OBJ_DATA *obj, const char *sname, byte mode)
{
	char sbaf[MAX_STRING_LENGTH];
	
	if (IS_MORTAL(ch))
	{
		if (OBJ_FLAGGED(obj, ITEM_NODROP))
		{
			sprintf(sbaf, "You can't %s $p, it must be CURSED!", sname);
			act(sbaf, FALSE, ch, obj, 0, TO_CHAR);
			return;
		}
		
		if ( check_trap( ch, obj, TRAP_ACT_DROP ) )
			return;
	}

	/* Message to char */
	if ( obj->count > 1 )
		sprintf(sbaf, "You %s %d $o.", sname, obj->count);
	else
		sprintf(sbaf, "You %s $p.", sname);
	act(sbaf, FALSE, ch, obj, 0, TO_CHAR);

	/* message to room */
	if ( obj->count > 1 )
		sprintf(sbaf, "$n %ss some $o.", sname, obj->count);
	else
		sprintf(sbaf, "$n %ss $p.", sname);
	act(sbaf, TRUE, ch, obj, 0, TO_ROOM);

	obj_from_char(obj);

	if ( mode == SCMD_JUNK )
		extract_obj(obj);
	else if ( mode == SCMD_DONATE )
	{
		SET_BIT(GET_OBJ_EXTRA(obj), ITEM_DONATED);
		obj = obj_to_room(obj, get_room(donation_room_1));
		act("$p suddenly appears in a puff a smoke!", FALSE, NULL, obj, NULL, TO_ROOM);
	}
	else
	{
		obj = obj_to_room(obj, ch->in_room);

		/* Room Trigger Events */
		if ( ch->in_room->trigger )
			check_room_trigger(ch, TRIG_ACT_DROP);
	}

	return;
}

/*
 * Syntax:
 *
 * - drop <obj>
 * - drop <num> <obj>
 * - drop all.<obj>
 * - drop all
 *
 * same syntax for 'junk' and 'donate'
 *
 */
ACMD(do_drop)
{
	OBJ_DATA *obj, *next_obj;
	char arg1[MAX_INPUT_LENGTH];
	const char *sname;
	byte mode = SCMD_DROP;
	int onum = 1, dotmode;
	bool bNumArg = FALSE;

	switch (subcmd)
	{
	// junk
	case SCMD_JUNK:
		sname = "junk";
		mode = SCMD_JUNK;
		break;
	// donate
	case SCMD_DONATE:
		if ( !get_room(donation_room_1) )
		{
			send_to_char("Sorry, you can't donate anything right now.\r\n", ch);
			return;
		}
		sname = "donate";
		mode = SCMD_DONATE;
		break;
	// drop
	default:
		sname = "drop";
		mode = SCMD_DROP;
		break;
	}

	if ( !*argument )
	{
		ch_printf(ch, "What do you want to %s?\r\n", sname);
		return;
	}

	argument = one_argument(argument, arg1);
	
	if ( is_number(arg1) )
	{
		bNumArg = TRUE;
		onum = atoi(arg1);
		if ( onum < 1 )
		{
			send_to_char("Don't be stupid!\r\n", ch);
			return;
		}
		argument = one_argument(argument, arg1);
	}

	dotmode = find_all_dots(arg1);

	/* drop <obj> or drop <num> <obj> */
	if ( dotmode == FIND_INDIV )
	{
		if (!(obj = get_obj_in_list_vis_rev(ch, arg1, NULL, ch->last_carrying)))
		{
			sprintf(buf, "You don't seem to have any %ss.\r\n", arg1);
			send_to_char(buf, ch);
		}
		else
		{
			if ( GET_OBJ_TYPE(obj) == ITEM_MONEY )
			{
				if ( bNumArg )
				{
					int amount = GET_OBJ_VAL(obj, 0);
					
					// she wants drop more than what she has..
					if ( onum > amount )
					{
						send_to_char("You don't have that much gold.\r\n", ch);
						return;
					}
					// she wants drop less than what she has..
					else if ( onum < amount )
					{
						int diff = amount - onum;
						
						// subtract weight for money rest
						// it will be added again by obj_to_char() called from create_amount()
						IS_CARRYING_W(ch) -= MAX(1, (diff * GET_OBJ_WEIGHT(obj)) / 10);
						// create a new money obj for the rest..
						create_amount(diff, ch, NULL, NULL, NULL, TRUE);
						// set the value of the existing object
						// to what has to be dropped
						GET_OBJ_VAL(obj, 0) = onum;
						// let's cascade till perform_drop_obj()
					}
					// she wants drop all of them
					// let's cascade till perform_drop_obj()
				}
			}
			else
			{
				if ( onum > obj->count )
				{
					ch_printf(ch, "You don't have so many %s.\r\n", arg1);
					return;
				}
				split_obj( obj, onum );
			}
		
			perform_drop_obj( ch, obj, sname, mode );
		}
	}
	/* drop all.<obj> */
	else if (dotmode == FIND_ALLDOT)
	{ 
		bool found = FALSE;

		if (!*arg1)
		{
			send_to_char("What do you want to drop all of?\r\n", ch);
			return;
		}

		if (!ch->first_carrying)
		{
			send_to_char("You don't seem to be carrying anything.\r\n", ch);
			return;
		}

		for (obj = ch->first_carrying; obj; obj = next_obj)
		{
			next_obj = obj->next_content;

			if (CAN_SEE_OBJ(ch, obj) && isname(arg1, obj->name))
			{
				found = TRUE;
				perform_drop_obj(ch, obj, sname, mode);
			}
		}

		if (!found)
			ch_printf(ch, "You don't seem to have any %ss.\r\n", arg1);
	}
	/* drop all */
	else
	{
		/* Can't junk or donate all */
		if (mode == SCMD_JUNK || mode == SCMD_DONATE)
		{
			if (mode == SCMD_JUNK)
				send_to_char("Go to the dump if you want to junk EVERYTHING!\r\n", ch);
			else
				send_to_char("Go do the donation room if you want to donate EVERYTHING!\r\n", ch);
			return;
		}

		if (!ch->first_carrying)
		{
			send_to_char("You don't seem to be carrying anything.\r\n", ch);
			return;
		}

		for (obj = ch->first_carrying; obj; obj = next_obj)
		{
			next_obj = obj->next_content;
				
			if (CAN_SEE_OBJ(ch, obj))
				perform_drop_obj(ch, obj, sname, mode);
		}
	}
}

/* ********************************************************************* */
/* PUT Routines                                                          */
/* ********************************************************************* */

void perform_put_obj(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *cont)
{
	int onum = obj->count;

	if (GET_OBJ_WEIGHT(cont) + get_real_obj_weight(obj) > GET_OBJ_VAL(cont, 0))
	{
		act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
		return;
	}

	if ( check_trap( ch, obj, TRAP_ACT_PUT ) )
		return;

	/* message to room */
	if ( onum > 1 )
		act("$n puts some $o in $P.", TRUE, ch, obj, cont, TO_ROOM);
	else
		act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);
	
	obj_from_char(obj);
	obj = obj_to_obj(obj, cont);
	
	/* Yes, I realize this is strange until we have auto-equip on rent. -gg */
	if (OBJ_FLAGGED(obj, ITEM_NODROP) && !OBJ_FLAGGED(cont, ITEM_NODROP))
	{
		SET_BIT(GET_OBJ_EXTRA(cont), ITEM_NODROP);
		act("You get a strange feeling as you put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
	}
	else
	{
		char sbaf[MAX_STRING_LENGTH];
		
		/* message to char */
		if ( onum > 1 )
			sprintf(sbaf, "You put %d $o in $P.", onum);
		else
			strcpy(sbaf, "You put $p in $P.");
		act(sbaf, FALSE, ch, obj, cont, TO_CHAR);
	}
}

/*
 * Syntax:
 *
 * - put <obj> <container>
 * - put <num> <obj> <container>
 * - put all.<obj> <container>
 * - put all <container>
 */
ACMD(do_put)
{
	OBJ_DATA *obj, *cont;
	CHAR_DATA *tmp_char;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	bool bNumArg = FALSE;
	int onum = 1, obj_dotmode, cont_dotmode;

	if ( !*argument )
	{
		send_to_char("Put what in what?\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg1);
	
	if ( is_number(arg1) )
	{
		bNumArg = TRUE;
		onum = atoi(arg1);
		if ( onum < 1 )
		{
			send_to_char("Don't be stupid!\r\n", ch);
			return;
		}
		argument = one_argument(argument, arg1);
	}

	argument = one_argument(argument, arg2);
	/* skip destination words */
	if ( !str_cmp(arg2, "into") || !str_cmp(arg2, "in") || !str_cmp(arg2, "inside") )
		argument = one_argument(argument, arg2);

	obj_dotmode		= find_all_dots(arg1);
	cont_dotmode	= find_all_dots(arg2);

	if ( !*arg1 )
	{
		send_to_char("Put what in what?\r\n", ch);
		return;
	}

	if ( !*arg2 )
	{
		sprintf(buf, "What do you want to put %s in?\r\n",
			((obj_dotmode == FIND_INDIV) ? "it" : "them"));
		send_to_char(buf, ch);
	}

	if (cont_dotmode != FIND_INDIV)
	{
		send_to_char("You can only put things into one container at a time.\r\n", ch);
		return;
	}

	generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);

	if (!cont)
	{
		sprintf(buf, "You don't see %s %s here.\r\n", AN(arg2), arg2);
		send_to_char(buf, ch);
		return;
	}

	if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER &&
	    GET_OBJ_TYPE(cont) != ITEM_MISSILE_CONT &&
	    GET_OBJ_TYPE(cont) != ITEM_FURNITURE)
	{
		act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
		return;
	}

	if (GET_OBJ_TYPE(cont) != ITEM_FURNITURE && OBJVAL_FLAGGED(cont, CONT_CLOSED))
	{
		send_to_char("You'd better open it first!\r\n", ch);
		return;
	}

	/* put <obj> <container> or put <num> <obj> <container> */
	if (obj_dotmode == FIND_INDIV)
	{
		if (!(obj = get_obj_in_list_vis_rev(ch, arg1, NULL, ch->last_carrying)))
		{
			sprintf(buf, "You aren't carrying %s %s.\r\n", AN(arg1), arg1);
			send_to_char(buf, ch);
			return;
		}
		
		if (obj == cont)
		{
			send_to_char("You attempt to fold it into itself, but fail.\r\n", ch);
			return;
		}

		/* special check for missile container */
		if (GET_OBJ_TYPE(cont) == ITEM_MISSILE_CONT && 
		    (GET_OBJ_TYPE(obj) != ITEM_MISSILE || 
		    (GET_OBJ_TYPE(obj) == ITEM_MISSILE && GET_OBJ_VAL(obj, 0) != GET_OBJ_VAL(cont, 2) ) ) )
		{
			sprintf(buf, "You can put only %s into %s.\r\n",
				missile_cont_table[GET_OBJ_VAL(cont, 2)],
				cont->short_description );
			send_to_char(buf, ch);
			return;
		}

		if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
		{
			if ( bNumArg )
			{
				int amount = GET_OBJ_VAL(obj, 0);
				
				// she wants put more than what she has..
				if ( onum > amount )
				{
					send_to_char("You don't have that much gold.\r\n", ch);
					return;
				}
				// she wants put less than what she has..
				else if ( onum < amount )
				{
					int diff = amount - onum;
					
					// subtract weight for money rest
					// it will be added again by obj_to_char() called from create_amount()
					IS_CARRYING_W(ch) -= MAX(1, (diff * GET_OBJ_WEIGHT(obj)) / 10);
					// create a new money obj for the rest..
					create_amount(diff, ch, NULL, NULL, NULL, TRUE);
					// set the value of the existing object
					// to what has to be dropped
					GET_OBJ_VAL(obj, 0) = onum;
					// let's cascade till perform_put_obj()
				}
				// she wants put all of them
				// let's cascade till perform_put_obj()
			}
		}
		else
		{
			if ( onum > obj->count )
			{
				ch_printf(ch, "You don't have so many %s.\r\n", arg1);
				return;
			}
			split_obj(obj, onum);
		}

		separate_obj(cont);

		perform_put_obj(ch, obj, cont);
	}
	/* put all.<obj> <container> or put all <container> */
	else
	{
		OBJ_DATA *next_obj;
		bool found = FALSE, missile_check = FALSE;

		/* special check for missile container */
		if (GET_OBJ_TYPE(cont) == ITEM_MISSILE_CONT)
		{
			if (obj_dotmode == FIND_ALL)
			{
				send_to_char("Sorry, you can't do that.\r\n", ch);
				return;
			}
			missile_check = TRUE;
		}

		separate_obj(cont);

		for (obj = ch->first_carrying; obj; obj = next_obj)
		{
			next_obj = obj->next_content;
			
			if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
				(obj_dotmode == FIND_ALL || isname(arg1, obj->name)))
			{
				if (missile_check)
				{
					if (GET_OBJ_TYPE(obj) != ITEM_MISSILE || 
					    (GET_OBJ_TYPE(obj) == ITEM_MISSILE && GET_OBJ_VAL(obj, 0) != GET_OBJ_VAL(cont, 2)) ) 
						continue;
				}

				found = TRUE;
				perform_put_obj(ch, obj, cont);
			}
		}
		if (!found)
		{
			if (obj_dotmode == FIND_ALL)
				send_to_char("You don't seem to have anything to put in it.\r\n", ch);
			else
			{
				sprintf(buf, "You don't seem to have any %ss.\r\n", arg1);
				send_to_char(buf, ch);
			}
		}

	}

}

/* ********************************************************************* */
/* GIVE Routines                                                         */
/* ********************************************************************* */
void perform_give_obj(CHAR_DATA *ch, CHAR_DATA *vict, OBJ_DATA *obj)
{
	char sbaf[MAX_STRING_LENGTH];

	if (OBJ_FLAGGED(obj, ITEM_NODROP))
	{
		act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

	if (IS_CARRYING_N(vict) + obj->count >= CAN_CARRY_N(vict))
	{
		act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	if ( get_real_obj_weight(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict))
	{
		act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
		return;
	}

	if ( check_trap( ch, obj, TRAP_ACT_GIVE ) )
		return;

	/* message to char */
	if ( obj->count > 1 )
		sprintf(sbaf, "You give %d $o to $N.", obj->count);
	else
		strcpy(sbaf, "You give $p to $N.");
	act(sbaf, FALSE, ch, obj, vict, TO_CHAR);

	/* message to vict */
	if ( obj->count > 1 )
		sprintf(sbaf, "$n gives you %d $o.", obj->count);
	else
		strcpy(sbaf, "$n gives you $p.");
	act(sbaf, FALSE, ch, obj, vict, TO_VICT);

	/* message to novict */
	if ( obj->count > 1 )
		act("$n gives some $o to $N.", TRUE, ch, obj, vict, TO_NOTVICT);
	else
		act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);

	obj_from_char(obj);
	obj = obj_to_char(obj, vict);
}

// utility function for give 
CHAR_DATA *find_give_vict(CHAR_DATA *ch, char *arg)
{
	CHAR_DATA *vict;
	
	if (!*arg)
	{
		send_to_char("To who?\r\n", ch);
		return (NULL);
	}
	
	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
	{
		send_to_char(NOPERSON, ch);
		return (NULL);
	}
	
	if (vict == ch)
	{
		send_to_char("What's the point of that?\r\n", ch);
		return (NULL);
	}
	
	return (vict);
}


/*
 * Syntax:
 *
 * give <obj> <victim>
 * give <num> <obj> <victim>
 * give all.<obj> <victim>
 * give all <victim>
 */
ACMD(do_give)
{
	OBJ_DATA *obj;
	CHAR_DATA *vict;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	int onum = 1, dotmode;
	bool bNumArg = FALSE;
	
	if ( !*argument )
	{
		send_to_char("Give what to who?\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg1);
	
	if ( is_number(arg1) )
	{
		bNumArg = TRUE;
		onum = atoi(arg1);
		if ( onum < 1 )
		{
			send_to_char("Don't be stupid!\r\n", ch);
			return;
		}
		argument = one_argument(argument, arg1);
	}

	argument = one_argument(argument, arg2);
	/* skip destination words */
	if ( !str_cmp(arg2, "to") )
		argument = one_argument(argument, arg2);

	if (!*arg2)
	{
		send_to_char("Give what to who?\r\n", ch);
		return;
	}

	if (!(vict = find_give_vict(ch, arg2)))
		return;

	dotmode	= find_all_dots(arg1);

	/* give <obj> <vict> or give <num> <obj> <vict> */
	if ( dotmode == FIND_INDIV )
	{
		if (!(obj = get_obj_in_list_vis_rev(ch, arg1, NULL, ch->last_carrying)))
		{
			ch_printf(ch, "You don't seem to have any %ss.\r\n", arg1);
			return;
		}

		if ( GET_OBJ_TYPE(obj) == ITEM_MONEY )
		{
			if ( bNumArg )
			{
				int amount = GET_OBJ_VAL(obj, 0);
				
				// she wants give more than what she has..
				if ( onum > amount )
				{
					send_to_char("You don't have that much gold.\r\n", ch);
					return;
				}
				// she wants give less than what she has..
				else if ( onum < amount )
				{
					int diff = amount - onum;
					
					// subtract weight for money rest
					// it will be added again by obj_to_char() called from create_amount()
					IS_CARRYING_W(ch) -= MAX(1, (diff * GET_OBJ_WEIGHT(obj)) / 10);
					// create a new money obj for the rest..
					create_amount(diff, ch, NULL, NULL, NULL, TRUE);
					// set the value of the existing object
					// to what has to be dropped
					GET_OBJ_VAL(obj, 0) = onum;
					// let's cascade till perform_give_obj()
				}
				// she wants give all of them
				// let's cascade till perform_give_obj()
			}
		}
		else
		{
			if ( onum > obj->count )
			{
				ch_printf(ch, "You don't have so many %s.\r\n", arg1);
				return;
			}
			split_obj(obj, onum);
		}
		
		perform_give_obj(ch, vict, obj);
	}
	/* give all.<obj> <vict> or give all <vict> */
	else
	{
		OBJ_DATA *next_obj;

		if (dotmode == FIND_ALLDOT && !*arg1)
		{
			send_to_char("Give all of what?\r\n", ch);
			return;
		}
		
		if (!ch->first_carrying)
		{
			send_to_char("You don't seem to be carrying anything.\r\n", ch);
			return;
		}

		for (obj = ch->first_carrying; obj; obj = next_obj)
		{
			next_obj = obj->next_content;
			
			if (CAN_SEE_OBJ(ch, obj) && ((dotmode == FIND_ALL || isname(arg, obj->name))))
				perform_give_obj(ch, vict, obj);
		}
	}
}


/* *********************************************************** */

void weight_change_object(OBJ_DATA *obj, int weight)
{
	OBJ_DATA *tmp_obj;
	CHAR_DATA *tmp_ch;
	
	if (obj->in_room != NULL)
	{
		GET_OBJ_WEIGHT(obj) += weight;
	}
	else if ((tmp_ch = obj->carried_by))
	{
		obj_from_char(obj);
		GET_OBJ_WEIGHT(obj) += weight;
		obj = obj_to_char(obj, tmp_ch);
	}
	else if ((tmp_obj = obj->in_obj))
	{
		obj_from_obj(obj);
		GET_OBJ_WEIGHT(obj) += weight;
		obj = obj_to_obj(obj, tmp_obj);
	}
	else
	{
		log("SYSERR: Unknown attempt to subtract weight from an object.");
	}
}



void name_from_drinkcon(OBJ_DATA *obj)
{
	char *new_name, *cur_name, *next;
	const char *liqname;
	int liqlen, cpylen;
	
	if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
		return;
	
	liqname = drinknames[GET_OBJ_VAL(obj, 2)];
	if (!isname(liqname, obj->name))
	{
		log("SYSERR: Can't remove liquid '%s' from '%s' (%d) item.", liqname, obj->name, obj->item_number);
		return;
	}
	
	liqlen = strlen(liqname);
	CREATE(new_name, char, strlen(obj->name) - strlen(liqname)); /* +1 for NUL, -1 for space */
	
	for (cur_name = obj->name; cur_name; cur_name = next)
	{
		if (*cur_name == ' ')
			cur_name++;
		
		if ((next = strchr(cur_name, ' ')))
			cpylen = next - cur_name;
		else
			cpylen = strlen(cur_name);
		
		if (!strn_cmp(cur_name, liqname, liqlen))
			continue;
		
		if (*new_name)
			strcat(new_name, " ");
		strncat(new_name, cur_name, cpylen);
	}
	
	if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
		free(obj->name);
	obj->name = new_name;
}



void name_to_drinkcon(OBJ_DATA *obj, int type)
{
	char *new_name;
	
	if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
		return;
	
	CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
	sprintf(new_name, "%s %s", obj->name, drinknames[type]);
	if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
		free(obj->name);
	obj->name = new_name;
}



ACMD(do_drink)
{
	OBJ_DATA *temp;
	AFFECTED_DATA af;
	int amount, weight;
	int on_ground = 0;
	
	one_argument(argument, arg);
	
	if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
		return;
	
	if (!*arg)
	{
		send_to_char("Drink from what?\r\n", ch);
		return;
	}

	if (!(temp = get_obj_in_list_vis_rev(ch, arg, NULL, ch->last_carrying)))
	{
		if (!(temp = get_obj_in_list_vis_rev(ch, arg, NULL, ch->in_room->last_content)))
		{
			send_to_char("You can't find it!\r\n", ch);
			return;
		}
		else
			on_ground = 1;
	}

	if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
		(GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN))
	{
		send_to_char("You can't drink from that!\r\n", ch);
		return;
	}

	if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON))
	{
		send_to_char("You have to be holding that to drink from it.\r\n", ch);
		return;
	}

	if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0))
	{
		/* The pig is drunk */
		send_to_char("You can't seem to get close enough to your mouth.\r\n", ch);
		act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
		return;
	}

	if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 0))
	{
		send_to_char("Your stomach can't contain anymore!\r\n", ch);
		return;
	}

	if (!GET_OBJ_VAL(temp, 1))
	{
		send_to_char("It's empty.\r\n", ch);
		return;
	}

	if (!on_ground)
		separate_obj(temp);

	if (subcmd == SCMD_DRINK)
	{
		sprintf(buf, "$n drinks %s from $p.", drinks[GET_OBJ_VAL(temp, 2)]);
		act(buf, TRUE, ch, temp, 0, TO_ROOM);
		
		sprintf(buf, "You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
		send_to_char(buf, ch);
		
		if (drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] > 0)
			amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK];
		else
			amount = number(3, 10);	
	}
	else
	{
		act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
		sprintf(buf, "It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
		send_to_char(buf, ch);
		amount = 1;
	}
	
	amount = MIN(amount, GET_OBJ_VAL(temp, 1));
	
	/* You can't subtract more than the object weighs */
	weight = MIN(amount, GET_OBJ_WEIGHT(temp));
	
	weight_change_object(temp, -weight);	/* Subtract amount */
	
	gain_condition(ch, DRUNK,  drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK]  * amount / 4);
	gain_condition(ch, FULL,   drink_aff[GET_OBJ_VAL(temp, 2)][FULL]   * amount / 4);
	gain_condition(ch, THIRST, drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount / 4);
	
	if (GET_COND(ch, DRUNK) > 10)
		send_to_char("You feel drunk.\r\n", ch);
	
	if (GET_COND(ch, THIRST) > 20)
		send_to_char("You don't feel thirsty any more.\r\n", ch);
	
	if (GET_COND(ch, FULL) > 20)
		send_to_char("You are full.\r\n", ch);
	
	if (GET_OBJ_VAL(temp, 3))	/* The crap was poisoned ! */
	{
		send_to_char("Oops, it tasted rather strange!\r\n", ch);
		act("$n chokes and utters some strange sounds.", TRUE, ch, 0, 0, TO_ROOM);
		
		af.type = SPELL_POISON;
		af.duration = amount * 3;
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.bitvector = AFF_POISON;
		affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
	}

	/* empty the container, and no longer poison. */
	GET_OBJ_VAL(temp, 1) -= amount;
	if (!GET_OBJ_VAL(temp, 1))	/* The last bit */
	{
		name_from_drinkcon(temp);
		GET_OBJ_VAL(temp, 2) = 0;
		GET_OBJ_VAL(temp, 3) = 0;
	}
	return;
}



ACMD(do_eat)
{
	OBJ_DATA *food;
	AFFECTED_DATA af;
	int amount;
	
	one_argument(argument, arg);
	
	if (IS_NPC(ch))		/* Cannot use GET_COND() on mobs. */
		return;
	
	if (!*arg)
	{
		send_to_char("Eat what?\r\n", ch);
		return;
	}

	if (!(food = get_obj_in_list_vis_rev(ch, arg, NULL, ch->last_carrying)))
	{
		sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
		send_to_char(buf, ch);
		return;
	}

	if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
		(GET_OBJ_TYPE(food) == ITEM_FOUNTAIN)))
	{
		do_drink(ch, argument, 0, SCMD_SIP);
		return;
	}

	if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && (GET_LEVEL(ch) < LVL_GOD))
	{
		send_to_char("You can't eat THAT!\r\n", ch);
		return;
	}

	if (GET_COND(ch, FULL) > 20)		/* Stomach full */
	{
		send_to_char("You are too full to eat more!\r\n", ch);
		return;
	}

	separate_obj(food);

	if (subcmd == SCMD_EAT)
	{
		act("You eat $p.", FALSE, ch, food, 0, TO_CHAR);
		act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
	}
	else
	{
		act("You nibble a little bit of $p.", FALSE, ch, food, 0, TO_CHAR);
		act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
	}
	
	amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, 0) : 1);
	
	gain_condition(ch, FULL, amount);
	
	if (GET_COND(ch, FULL) > 20)
		send_to_char("You are full.\r\n", ch);
	
	if (GET_OBJ_VAL(food, 3) && (GET_LEVEL(ch) < LVL_IMMORT))
	{
		/* The crap was poisoned ! */
		send_to_char("Oops, that tasted rather strange!\r\n", ch);
		act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);
		
		af.type = SPELL_POISON;
		af.duration = amount * 2;
		af.modifier = 0;
		af.location = APPLY_NONE;
		af.bitvector = AFF_POISON;
		affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
	}

	if (subcmd == SCMD_EAT)
		extract_obj(food);
	else
	{
		if (!(--GET_OBJ_VAL(food, 0)))
		{
			send_to_char("There's nothing left now.\r\n", ch);
			extract_obj(food);
		}
	}
}


ACMD(do_pour)
{
	OBJ_DATA *from_obj = NULL, *to_obj = NULL;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	int amount;
	
	two_arguments(argument, arg1, arg2);
	
	if (subcmd == SCMD_POUR)
	{
		if (!*arg1)
		{		/* No arguments */
			send_to_char("From what do you want to pour?\r\n", ch);
			return;
		}
		if (!(from_obj = get_obj_in_list_vis_rev(ch, arg1, NULL, ch->last_carrying)))
		{
			send_to_char("You can't find it!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON)
		{
			send_to_char("You can't pour from that!\r\n", ch);
			return;
		}
	}

	if (subcmd == SCMD_FILL)
	{
		if (!*arg1)
		{		/* no arguments */
			send_to_char("What do you want to fill?  And what are you filling it from?\r\n", ch);
			return;
		}
		if (!(to_obj = get_obj_in_list_vis_rev(ch, arg1, NULL, ch->last_carrying)))
		{
			send_to_char("You can't find it!\r\n", ch);
			return;
		}
		if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON)
		{
			act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
			return;
		}
		if (!*arg2)		/* no 2nd argument */
		{
			/*
			int nsect = ch->in_room->sector_type;
			if (water_sector(nsect) && nsect != SECT_SEA)
			{
			fill_from_water_around(ch);
			}
			*/
			act("What do you want to fill $p from?", FALSE, ch, to_obj, 0, TO_CHAR);
			return;
		}
		if (!(from_obj = get_obj_in_list_vis_rev(ch, arg2, NULL, ch->in_room->last_content)))
		{
			sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
			send_to_char(buf, ch);
			return;
		}
		if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN)
		{
			act("You can't fill something from $p.", FALSE, ch, from_obj, 0, TO_CHAR);
			return;
		}
	}

	if (GET_OBJ_VAL(from_obj, 1) == 0)
	{
		act("The $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
		return;
	}

	if (subcmd == SCMD_POUR)	/* pour */
	{
		if (!*arg2)
		{
			send_to_char("Where do you want it?  Out or in what?\r\n", ch);
			return;
		}
		if (!str_cmp(arg2, "out"))
		{
			separate_obj(from_obj);

			act("$n empties $p.", TRUE, ch, from_obj, 0, TO_ROOM);
			act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);
			
			weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1)); /* Empty */
			
			name_from_drinkcon(from_obj);
			GET_OBJ_VAL(from_obj, 1) = 0;
			GET_OBJ_VAL(from_obj, 2) = 0;
			GET_OBJ_VAL(from_obj, 3) = 0;
			
			return;
		}
		if (!(to_obj = get_obj_in_list_vis_rev(ch, arg2, NULL, ch->last_carrying)))
		{
			send_to_char("You can't find it!\r\n", ch);
			return;
		}
		if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
			(GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN))
		{
			send_to_char("You can't pour anything into that.\r\n", ch);
			return;
		}
	}

	if (to_obj == from_obj)
	{
		send_to_char("A most unproductive effort.\r\n", ch);
		return;
	}

	if ((GET_OBJ_VAL(to_obj, 1) != 0) &&
		(GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2)))
	{
		send_to_char("There is already another liquid in it!\r\n", ch);
		return;
	}

	if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0)))
	{
		send_to_char("There is no room for more.\r\n", ch);
		return;
	}

	if (subcmd == SCMD_POUR)
	{
		sprintf(buf, "You pour the %s into the %s.",
			drinks[GET_OBJ_VAL(from_obj, 2)], arg2);
		send_to_char(buf, ch);
	}

	if (subcmd == SCMD_FILL)
	{
		act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
		act("$n gently fills $p from $P.", TRUE, ch, to_obj, from_obj, TO_ROOM);
	}

	separate_obj(from_obj);
	separate_obj(to_obj);

	/* New alias */
	if (GET_OBJ_VAL(to_obj, 1) == 0)
		name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));
	
	/* First same type liq. */
	GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);
	
	/* Then how much to pour */
	GET_OBJ_VAL(from_obj, 1) -= (amount =
		(GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));
	
	GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);
	
	if (GET_OBJ_VAL(from_obj, 1) < 0)	/* There was too little */
	{
		GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
		amount += GET_OBJ_VAL(from_obj, 1);
		name_from_drinkcon(from_obj);
		GET_OBJ_VAL(from_obj, 1) = 0;
		GET_OBJ_VAL(from_obj, 2) = 0;
		GET_OBJ_VAL(from_obj, 3) = 0;
	}

	/* Then the poison boogie */
	GET_OBJ_VAL(to_obj, 3) =
		(GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));
	
	/* And the weight boogie */
	weight_change_object(from_obj, -amount);
	weight_change_object(to_obj, amount);	/* Add weight */
}



void wear_message(CHAR_DATA *ch, OBJ_DATA *obj, int where)
{
	const char *wear_messages[][2] =
	{
		{"$n lights $p and holds it.",
		"You light $p and hold it."},
			
		{"$n slides $p on to $s right ring finger.",
		"You slide $p on to your right ring finger."},
		
		{"$n slides $p on to $s left ring finger.",
		"You slide $p on to your left ring finger."},
		
		{"$n wears $p around $s neck.",
		"You wear $p around your neck."},
		
		{"$n wears $p around $s neck.",
		"You wear $p around your neck."},
		
		{"$n wears $p on $s body.",
		"You wear $p on your body."},
		
		{"$n wears $p on $s head.",
		"You wear $p on your head."},
		
		{"$n puts $p on $s legs.",
		"You put $p on your legs."},
		
		{"$n wears $p on $s feet.",
		"You wear $p on your feet."},
		
		{"$n puts $p on $s hands.",
		"You put $p on your hands."},
		
		{"$n wears $p on $s arms.",
		"You wear $p on your arms."},
		
		{"$n straps $p around $s arm as a shield.",
		"You start to use $p as a shield."},
		
		{"$n wears $p about $s body.",
		"You wear $p around your body."},
		
		{"$n wears $p around $s waist.",
		"You wear $p around your waist."},
		
		{"$n puts $p on around $s right wrist.",
		"You put $p on around your right wrist."},
		
		{"$n puts $p on around $s left wrist.",
		"You put $p on around your left wrist."},
		
		{"$n wields $p.",
		"You wield $p."},
		
		{"$n grabs $p.",
		"You grab $p."},

		{"$n wear $p over $s shoulders.",
		"You wear $p over your shoulders."}
	};
	
	act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
	act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
}



void perform_wear(CHAR_DATA *ch, OBJ_DATA *obj, int where)
{
	/*
	 * ITEM_WEAR_TAKE is used for objects that do not require special bits
	 * to be put into that position (e.g. you can hold any object, not just
	 * an object with a HOLD bit.)
	 */
	int wear_bitvectors[] =
	{
		ITEM_WEAR_TAKE, ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
		ITEM_WEAR_NECK, ITEM_WEAR_BODY, ITEM_WEAR_HEAD, ITEM_WEAR_LEGS,
		ITEM_WEAR_FEET, ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_SHIELD,
		ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST, ITEM_WEAR_WRIST, ITEM_WEAR_WRIST,
		ITEM_WEAR_WIELD, ITEM_WEAR_TAKE, ITEM_WEAR_SHOULDER
	};
	
	const char *already_wearing[] =
	{
		"You're already using a light.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"You're already wearing something on both of your ring fingers.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"You can't wear anything else around your neck.\r\n",
		"You're already wearing something on your body.\r\n",
		"You're already wearing something on your head.\r\n",
		"You're already wearing something on your legs.\r\n",
		"You're already wearing something on your feet.\r\n",
		"You're already wearing something on your hands.\r\n",
		"You're already wearing something on your arms.\r\n",
		"You're already using a shield.\r\n",
		"You're already wearing something about your body.\r\n",
		"You already have something around your waist.\r\n",
		"YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
		"You're already wearing something around both of your wrists.\r\n",
		"You're already wielding a weapon.\r\n",
		"You're already holding something.\r\n",
		"Yur're already wearing something on your shoulders.\r\n"
	};
	
	/* first, make sure that the wear position is valid. */
	if (!CAN_WEAR(obj, wear_bitvectors[where]))
	{
		act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
		return;
	}

	/* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
	if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) || (where == WEAR_WRIST_R))
	{
		if (GET_EQ(ch, where))
			where++;
	}
		
	if (GET_EQ(ch, where))
	{
		send_to_char(already_wearing[where], ch);
		return;
	}

	if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
	{
		send_to_char("You aren't powerful enough to use that.\r\n", ch);
		return;
	}

	if (check_trap(ch, obj, TRAP_ACT_USE))
		return;

	separate_obj(obj);
	wear_message(ch, obj, where);
	obj_from_char(obj);
	equip_char(ch, obj, where);
}



int find_eq_pos(CHAR_DATA *ch, OBJ_DATA *obj, char *arg)
{
	int where = -1;
	
	const char *keywords[] =
	{
		"!RESERVED!",
		"finger",
		"!RESERVED!",
		"neck",
		"!RESERVED!",
		"body",
		"head",
		"legs",
		"feet",
		"hands",
		"arms",
		"shield",
		"about",
		"waist",
		"wrist",
		"!RESERVED!",
		"!RESERVED!",
		"!RESERVED!",
		"shoulders",
		"\n"
	};
	
	if (!arg || !*arg)
	{
		if (CAN_WEAR(obj, ITEM_WEAR_FINGER))      where = WEAR_FINGER_R;
		if (CAN_WEAR(obj, ITEM_WEAR_NECK))        where = WEAR_NECK_1;
		if (CAN_WEAR(obj, ITEM_WEAR_BODY))        where = WEAR_BODY;
		if (CAN_WEAR(obj, ITEM_WEAR_HEAD))        where = WEAR_HEAD;
		if (CAN_WEAR(obj, ITEM_WEAR_LEGS))        where = WEAR_LEGS;
		if (CAN_WEAR(obj, ITEM_WEAR_FEET))        where = WEAR_FEET;
		if (CAN_WEAR(obj, ITEM_WEAR_HANDS))       where = WEAR_HANDS;
		if (CAN_WEAR(obj, ITEM_WEAR_ARMS))        where = WEAR_ARMS;
		if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))      where = WEAR_SHIELD;
		if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))       where = WEAR_ABOUT;
		if (CAN_WEAR(obj, ITEM_WEAR_WAIST))       where = WEAR_WAIST;
		if (CAN_WEAR(obj, ITEM_WEAR_WRIST))       where = WEAR_WRIST_R;
		if (CAN_WEAR(obj, ITEM_WEAR_SHOULDER))    where = WEAR_SHOULDERS;
	}
	else
	{
		if ((where = search_block(arg, keywords, FALSE)) < 0)
		{
			sprintf(buf, "'%s'?  What part of your body is THAT?\r\n", arg);
			send_to_char(buf, ch);
		}
	}
	
	return (where);
}



ACMD(do_wear)
{
	OBJ_DATA *obj, *next_obj;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	int where, dotmode, items_worn = 0;
	
	two_arguments(argument, arg1, arg2);
	
	if (!*arg1)
	{
		send_to_char("Wear what?\r\n", ch);
		return;
	}
	dotmode = find_all_dots(arg1);
	
	if (*arg2 && (dotmode != FIND_INDIV))
	{
		send_to_char("You can't specify the same body location for more than one item!\r\n", ch);
		return;
	}
	if (dotmode == FIND_ALL)
	{
		for (obj = ch->first_carrying; obj; obj = next_obj)
		{
			next_obj = obj->next_content;
			if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0)
			{
				items_worn++;
				perform_wear(ch, obj, where);
			}
		}
		if (!items_worn)
			send_to_char("You don't seem to have anything wearable.\r\n", ch);
	}
	else if (dotmode == FIND_ALLDOT)
	{
		if (!*arg1)
		{
			send_to_char("Wear all of what?\r\n", ch);
			return;
		}
		if (!(obj = get_obj_in_list_vis_rev(ch, arg1, NULL, ch->last_carrying)))
		{
			sprintf(buf, "You don't seem to have any %ss.\r\n", arg1);
			send_to_char(buf, ch);
		}
		else
		{
			while (obj)
			{
				next_obj = get_obj_in_list_vis_rev(ch, arg1, NULL, obj->prev_content);
				if ((where = find_eq_pos(ch, obj, 0)) >= 0)
					perform_wear(ch, obj, where);
				else
					act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
				obj = next_obj;
			}
		}
	}
	else
	{
		if (!(obj = get_obj_in_list_vis_rev(ch, arg1, NULL, ch->last_carrying)))
		{
			sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
			send_to_char(buf, ch);
		}
		else
		{
			if ((where = find_eq_pos(ch, obj, arg2)) >= 0)
				perform_wear(ch, obj, where);
			else if (!*arg2)
				act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
		}
	}
}



ACMD(do_wield)
{
	OBJ_DATA *obj;
	
	one_argument(argument, arg);
	
	if (!*arg)
		send_to_char("Wield what?\r\n", ch);
	else if (!(obj = get_obj_in_list_vis_rev(ch, arg, NULL, ch->last_carrying)))
	{
		sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
		send_to_char(buf, ch);
	}
	else
	{
		if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
			send_to_char("You can't wield that.\r\n", ch);
		else if (GET_OBJ_WEIGHT(obj) > str_app[GET_STR(ch)].wield_w)
			send_to_char("It's too heavy for you to use.\r\n", ch);
		else
			perform_wear(ch, obj, WEAR_WIELD);
	}
}



ACMD(do_grab)
{
	OBJ_DATA *obj;
	
	one_argument(argument, arg);
	
	if (!*arg)
		send_to_char("Hold what?\r\n", ch);
	else if (!(obj = get_obj_in_list_vis_rev(ch, arg, NULL, ch->last_carrying)))
	{
		sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
		send_to_char(buf, ch);
	}
	else
	{
		if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
			perform_wear(ch, obj, WEAR_LIGHT);
		else
		{
			if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && GET_OBJ_TYPE(obj) != ITEM_WAND &&
				GET_OBJ_TYPE(obj) != ITEM_STAFF && GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
				GET_OBJ_TYPE(obj) != ITEM_POTION)
				send_to_char("You can't hold that.\r\n", ch);
			else
				perform_wear(ch, obj, WEAR_HOLD);
		}
	}
}



void perform_remove(CHAR_DATA *ch, int pos)
{
	OBJ_DATA *obj;
	
	if (!(obj = GET_EQ(ch, pos)))
		log("SYSERR: perform_remove: bad pos %d passed.", pos);
	else if (OBJ_FLAGGED(obj, ITEM_NODROP))
		act("You can't remove $p, it must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
	else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
		act("$p: you can't carry that many items!", FALSE, ch, obj, 0, TO_CHAR);
	else
	{
		obj = obj_to_char(unequip_char(ch, pos), ch);
		act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
	}
}


ACMD(do_remove)
{
	int i, dotmode, found;
	
	one_argument(argument, arg);
	
	if (!*arg)
	{
		send_to_char("Remove what?\r\n", ch);
		return;
	}
	dotmode = find_all_dots(arg);
	
	if (dotmode == FIND_ALL)
	{
		for (found = 0, i = 0; i < NUM_WEARS; i++)
		{
			if (GET_EQ(ch, i))
			{
				perform_remove(ch, i);
				found = 1;
			}
		}
		if (!found)
			send_to_char("You're not using anything.\r\n", ch);
	}
	else if (dotmode == FIND_ALLDOT)
	{
		if (!*arg)
			send_to_char("Remove all of what?\r\n", ch);
		else
		{
			for (found = 0, i = 0; i < NUM_WEARS; i++)
			{
				if (GET_EQ(ch, i) && CAN_SEE_OBJ(ch, GET_EQ(ch, i)) &&
					isname(arg, GET_EQ(ch, i)->name))
				{
					perform_remove(ch, i);
					found = 1;
				}
			}
			if (!found)
			{
				sprintf(buf, "You don't seem to be using any %ss.\r\n", arg);
				send_to_char(buf, ch);
			}
		}
	}
	else
	{
		if ((i = get_obj_pos_in_equip_vis(ch, arg, NULL, ch->equipment)) < 0)
		{
			sprintf(buf, "You don't seem to be using %s %s.\r\n", AN(arg), arg);
			send_to_char(buf, ch);
		}
		else
			perform_remove(ch, i);
	}
}


/* ************************************************************* */
/* Items Comparing Routines                                      */
/* ************************************************************* */

float class_obj_compare(CHAR_DATA *ch, byte location, sbyte modifier)
{
	float value = 0;
	
	switch (GET_CLASS(ch))
	{
	case CLASS_MAGIC_USER:
	case CLASS_SORCERER:
		switch (location)
		{
		case APPLY_INT:
		case APPLY_MANA:
			value = modifier * 2;
			break;
		case APPLY_HIT:
		case APPLY_AC:
			value = modifier * 1.5;
			break;
		case APPLY_STR:
		case APPLY_CON:
		case APPLY_DEX:
		case APPLY_WIS:
			value = modifier;
			break;
		default:
			value = 0.5;
			break;
		}
		break;

	case CLASS_CLERIC:
		switch (location)
		{
		case APPLY_WIS:
		case APPLY_MANA:
			value = modifier * 2;
			break;
		case APPLY_HIT:
		case APPLY_AC:
			value = modifier * 1.5;
			break;
		case APPLY_STR:
		case APPLY_CON:
		case APPLY_DEX:
		case APPLY_INT:
			value = modifier;
			break;
		default:
			value = 0.5;
			break;
		}
		break;

	case CLASS_THIEF:
		switch (location)
		{
		case APPLY_DEX:
			value = modifier * 2;
			break;
		case APPLY_HITROLL:
		case APPLY_DAMROLL:
		case APPLY_AC:
		case APPLY_STR:
			value = modifier * 1.5;
			break;
		case APPLY_CON:
		case APPLY_INT:
		case APPLY_WIS:
			value = modifier;
			break;
		default:
			value = 0.5;
			break;
		}
		break;

	case CLASS_WARRIOR:
		switch (location)
		{
		case APPLY_STR:
		case APPLY_HITROLL:
		case APPLY_DAMROLL:
			value = modifier * 2;
			break;
		case APPLY_AC:
		case APPLY_CON:
		case APPLY_DEX:
			value = modifier * 1.5;
			break;
		case APPLY_INT:
		case APPLY_WIS:
			value = modifier;
			break;
		default:
			value = 0.5;
			break;
		}
		break;
	}
	
	/* additional 0.5 for saving spells */
	if (location == APPLY_SAVING_SPELL)
		value += 0.5;
	
	return (value);
}


/*
 * compare two objects
 * return:
 * 1	--> obj1 is better than obj2
 * 0	--> equals objects
 * -1	--> obj2 is better than obj1
 */
int compare_obj(OBJ_DATA *obj, OBJ_DATA *obj2, CHAR_DATA *ch)
{
	int j;
	float value1 = 0, value2 = 0;
	
	switch (GET_OBJ_TYPE(obj))
	{
	case ITEM_ARMOR:
		value1 = GET_OBJ_VAL(obj, 0);
		value2 = GET_OBJ_VAL(obj2, 0);
		
		for (j = 0; j < MAX_OBJ_AFF; j++)
		{
			if (obj->affected[j].modifier)
			{
				switch (obj->affected[j].location)
				{
				case APPLY_AC:
					value1 += (obj->affected[j].modifier * -1);
					break;
				case APPLY_DEX:
					value1 += obj->affected[j].modifier;
					break;
				default:
					value1 += class_obj_compare(ch, obj->affected[j].location, obj->affected[j].modifier);
					break;
				}
			}
			if (obj2->affected[j].modifier)
			{
				switch (obj2->affected[j].location)
				{
				case APPLY_AC:
					value2 += (obj2->affected[j].modifier * -1);
					break;
				case APPLY_DEX:
					value2 += obj2->affected[j].modifier;
					break;
				default:
					value2 += class_obj_compare(ch, obj2->affected[j].location, obj2->affected[j].modifier);
					break;
				}
			}
		}
		break;
		
	case ITEM_WEAPON:
		value1 = ( (GET_OBJ_VAL(obj, 2) + 1) / 2.0 ) * GET_OBJ_VAL(obj, 1);
		value2 = ( (GET_OBJ_VAL(obj2, 2) + 1) / 2.0 ) * GET_OBJ_VAL(obj2, 1);
		
		for (j = 0; j < MAX_OBJ_AFF; j++)
		{
			if (obj->affected[j].modifier)
			{
				switch (obj->affected[j].location)
				{
				case APPLY_HITROLL:
				case APPLY_DAMROLL:
				case APPLY_STR:
					value1 += obj->affected[j].modifier;
					break;
				default:
					value1 += class_obj_compare(ch, obj->affected[j].location, obj->affected[j].modifier);
					break;
				}
			}
			if (obj2->affected[j].modifier)
			{
				switch (obj2->affected[j].location)
				{
				case APPLY_HITROLL:
				case APPLY_DAMROLL:
				case APPLY_STR:
					value2 += obj2->affected[j].modifier;
					break;
				default:
					value2 += class_obj_compare(ch, obj2->affected[j].location, obj2->affected[j].modifier);
					break;
				}
			}
		}
		break;
		
	case ITEM_LIGHT:
		value1 = ( GET_OBJ_VAL(obj, 2) == -1 ? 1000 : GET_OBJ_VAL(obj, 2) );
		value2 = ( GET_OBJ_VAL(obj2, 2) == -1 ? 1000 : GET_OBJ_VAL(obj2, 2) );
		for (j = 0; j < MAX_OBJ_AFF; j++)
		{
			if (obj->affected[j].modifier)
				value1 += class_obj_compare(ch, obj->affected[j].location, obj->affected[j].modifier);
			if (obj2->affected[j].modifier)
				value2 += class_obj_compare(ch, obj2->affected[j].location, obj2->affected[j].modifier);
		}
		break;
		
	/* rings, bracelets, necklaces, pendants */
	case ITEM_WORN:
		for (j = 0; j < MAX_OBJ_AFF; j++)
		{
			if (obj->affected[j].modifier)
				value1 += class_obj_compare(ch, obj->affected[j].location, obj->affected[j].modifier);
			if (obj2->affected[j].modifier)
				value2 += class_obj_compare(ch, obj2->affected[j].location, obj2->affected[j].modifier);
		}
		break;
		
	default:
		break;
		
	}
	
	if (value1 == value2)		return (0);
	else if (value1 > value2)	return (1);
	else				return (-1);
}

int get_eq_pos(CHAR_DATA *ch, OBJ_DATA *obj)
{
	int where = -1;
	
	if (CAN_WEAR(obj, ITEM_WEAR_FINGER))      where = WEAR_FINGER_R;
	if (CAN_WEAR(obj, ITEM_WEAR_NECK))        where = WEAR_NECK_1;
	if (CAN_WEAR(obj, ITEM_WEAR_BODY))        where = WEAR_BODY;
	if (CAN_WEAR(obj, ITEM_WEAR_HEAD))        where = WEAR_HEAD;
	if (CAN_WEAR(obj, ITEM_WEAR_LEGS))        where = WEAR_LEGS;
	if (CAN_WEAR(obj, ITEM_WEAR_FEET))        where = WEAR_FEET;
	if (CAN_WEAR(obj, ITEM_WEAR_HANDS))       where = WEAR_HANDS;
	if (CAN_WEAR(obj, ITEM_WEAR_ARMS))        where = WEAR_ARMS;
	if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))      where = WEAR_SHIELD;
	if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))       where = WEAR_ABOUT;
	if (CAN_WEAR(obj, ITEM_WEAR_WAIST))       where = WEAR_WAIST;
	if (CAN_WEAR(obj, ITEM_WEAR_WRIST))       where = WEAR_WRIST_R;
	if (CAN_WEAR(obj, ITEM_WEAR_WIELD))       where = WEAR_WIELD;
	
	return (where);
}

bool donation_compare_obj(OBJ_DATA *obj, CHAR_DATA *ch)
{
	OBJ_DATA *obj2 = NULL;
	int where, value = 0;
	
	/* list here objects that must not be evaluated */
	if ( GET_OBJ_TYPE(obj) == ITEM_MONEY ||
	     GET_OBJ_TYPE(obj) == ITEM_FURNITURE )
	     return (FALSE);

	where = get_eq_pos(ch, obj);
	
	if (!(obj2 = GET_EQ(ch, where)))
		return (TRUE);
	
	if (compare_obj(obj, obj2, ch) == 1)
		return (TRUE);
	
	return (FALSE);	
}


ACMD(do_compare)
{
	OBJ_DATA *obj1, *obj2;
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	int value;
	
	two_arguments(argument, arg1, arg2);
	
	if (!*arg1)
	{
		send_to_char("Compare what to what?\n\r", ch);
		return;
	}
	
	if (!(obj1 = get_obj_in_list_vis_rev(ch, arg1, NULL, ch->last_carrying)))
	{
		send_to_char( "You do not have that item.\n\r", ch );
		return;
	}
	
	if (!*arg2)
	{
		for (obj2 = ch->first_carrying; obj2; obj2 = obj2->next_content)
		{
			if ( !(
				(GET_OBJ_TYPE(obj2) == ITEM_WEAPON)	||
				(GET_OBJ_TYPE(obj2) == ITEM_FIREWEAPON) ||
				(GET_OBJ_TYPE(obj2) == ITEM_ARMOR)	||
				(GET_OBJ_TYPE(obj2) == ITEM_WORN) ) &&
				CAN_SEE_OBJ(ch, obj2) &&
				GET_OBJ_TYPE(obj1) == GET_OBJ_TYPE(obj2) &&
				CAN_GET_OBJ(ch, obj2)
			   )
				break;
		}
		
		if (!obj2)
		{
			send_to_char( "You aren't wearing anything comparable.\n\r", ch );
			return;
		}
	}
	else
	{
		if (!(obj2 = get_obj_in_list_vis_rev(ch, arg2, NULL, ch->last_carrying)))
		{
			send_to_char( "You do not have that item.\n\r", ch );
			return;
		}
	}
	
	value = compare_obj(obj1, obj2, ch);
	
	if (value == 0)
		strcpy(buf, "$p and $P look about the same.");
	else if (value == 1)
		strcpy(buf, "$p look better than $P.");
	else
		strcpy(buf, "$p look worse than $P.");
	
	act(buf, FALSE, ch, obj1, obj2, TO_CHAR);
}

/* ********************************************************************* */
/* CHARGE Routines                                                       */
/* ********************************************************************* */

void perform_charge_obj( CHAR_DATA *ch, OBJ_DATA *obj, VEHICLE_DATA *vehicle )
{
	char buf[MAX_STRING_LENGTH];
	int onum = obj->count;

	if ( vehicle->curr_val.capacity + get_real_obj_weight(obj) > vehicle->max_val.capacity )
	{
		ch_printf(ch, "%s cannot contain that much.\r\n", vehicle->short_description);
		return;
	}

	obj_from_char(obj);
	obj = obj_to_vehicle(obj, vehicle);

	/* message to char */
	if ( onum > 1 )
		sprintf(buf, "You charge %d $o into $v.", onum);
	else
		strcpy(buf, "You charge $p into $v.");
	act(buf, FALSE, ch, obj, vehicle, TO_CHAR);

	/* message to room */
	if ( onum > 1 )
		strcpy(buf, "$n charge some $o into $v.");
	else
		strcpy(buf, "$n charge $p into $v.");
	act(buf, FALSE, ch, obj, vehicle, TO_ROOM);
}

/*
 * Syntax:
 *
 * charge <obj> <vehicle>
 * charge <num> <obj> <vehicle>
 * charge all.<obj> <vehicle>
 * charge all <vehicle>
 */
ACMD(do_charge)
{
	OBJ_DATA *obj;
	VEHICLE_DATA *vehicle;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	bool bNumArg = FALSE;
	int onum = 1;
	int obj_dotmode, cont_dotmode;

	if ( !*argument )
	{
		send_to_char("Charge what in what?\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg1);

	if ( is_number(arg1) )
	{
		bNumArg = TRUE;
		onum = atoi(arg1);
		if ( onum < 1 )
		{
			send_to_char("Don't be stupid!\r\n", ch);
			return;
		}
		argument = one_argument(argument, arg1);
	}
	
	argument = one_argument(argument, arg2);

	/* skip destination words */
	if ( !str_cmp(arg2, "into") || !str_cmp(arg2, "in") || !str_cmp(arg2, "inside") )
		argument = one_argument(argument, arg2);

	obj_dotmode	= find_all_dots(arg1);
	cont_dotmode	= find_all_dots(arg2);

	if ( !*arg1 )
	{
		send_to_char("Charge what in what?\r\n", ch);
		return;
	}

	if ( !*arg2 )
	{
		sprintf(buf, "What do you want to charge %s in?\r\n",
			((obj_dotmode == FIND_INDIV) ? "it" : "them"));
		send_to_char(buf, ch);
	}

	if (obj_dotmode == FIND_ALL)
	{
		send_to_char("You can't charge all into one vehicle.\r\n", ch);
		return;
	}

	if (cont_dotmode != FIND_INDIV)
	{
		send_to_char("You can only put things into one vehicle at a time.\r\n", ch);
		return;
	}

	if ( !( vehicle = find_vehicle_in_room_by_name(ch, arg2) ) )
	{
		sprintf(buf, "You don't see %s %s here.\r\n", AN(arg2), arg2);
		send_to_char(buf, ch);
		return;
	}

	if ( vehicle->owner_id != GET_IDNUM(ch) && !IS_IMMORTAL(ch) )
	{
		ch_printf(ch, "%s is not yours that you can use it.\r\n",
			vehicle->short_description);
		return;
	}

	/* charge <obj> <vehicle> or charge <num> <obj> <vehicle> */
	if ( obj_dotmode == FIND_INDIV )
	{
		if (!(obj = get_obj_in_list_vis_rev(ch, arg1, NULL, ch->last_carrying)))
		{
			sprintf(buf, "You aren't carrying %s %s.\r\n", AN(arg1), arg1);
			send_to_char(buf, ch);
			return;
		}

		if ( GET_OBJ_TYPE(obj) != ITEM_GOODS && IS_MORTAL(ch) )
		{
			send_to_char("You can charge only goods into a vehicle.\r\n", ch);
			return;
		}

		if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
		{
			if ( bNumArg )
			{
				int amount = GET_OBJ_VAL(obj, 0);
				
				// she wants put more than what she has..
				if ( onum > amount )
				{
					send_to_char("You don't have that much gold.\r\n", ch);
					return;
				}
				// she wants put less than what she has..
				else if ( onum < amount )
				{
					int diff = amount - onum;
					
					// subtract weight for money rest
					// it will be added again by obj_to_char() called from create_amount()
					IS_CARRYING_W(ch) -= MAX(1, (diff * GET_OBJ_WEIGHT(obj)) / 10);
					// create a new money obj for the rest..
					create_amount(diff, ch, NULL, NULL, NULL, TRUE);
					// set the value of the existing object
					// to what has to be dropped
					GET_OBJ_VAL(obj, 0) = onum;
					// let's cascade till perform_put_obj()
				}
				// she wants put all of them
				// let's cascade till perform_put_obj()
			}
		}
		else
		{
			if ( onum > obj->count )
			{
				ch_printf(ch, "You don't have so many %s.\r\n", arg1);
				return;
			}
			split_obj(obj, onum);
		}

		perform_charge_obj(ch, obj, vehicle);
	}
	/* charge all.<obj> <vehicle> */
	else
	{
		OBJ_DATA *next_obj;
		bool found = FALSE;

		for (obj = ch->first_carrying; obj; obj = next_obj)
		{
			next_obj = obj->next_content;

			if ( GET_OBJ_TYPE(obj) != ITEM_GOODS && !IS_IMMORTAL(ch) )
				continue;
			
			if (CAN_SEE_OBJ(ch, obj) && isname(arg1, obj->name))
			{
				found = TRUE;
				perform_charge_obj(ch, obj, vehicle);
			}
		}

		if (!found)
		{
			sprintf(buf, "You don't seem to have any %ss.\r\n", arg1);
			send_to_char(buf, ch);
		}
	}
}

/* ********************************************************************* */
/* DISCHARGE Routines                                                    */
/* ********************************************************************* */

void perform_discharge_obj( CHAR_DATA *ch, OBJ_DATA *obj, VEHICLE_DATA *vehicle )
{
	char buf[MAX_STRING_LENGTH];
	int onum = obj->count;

	if ( !can_take_obj(ch, obj) )
		return;

	obj_from_vehicle(obj);
	obj = obj_to_char(obj, ch);

	/* message to char */
	if ( onum > 1 )
		sprintf(buf, "You discharge %d $o from $v.", onum);
	else
		strcpy(buf, "You discharge $p from $v.");
	act(buf, FALSE, ch, obj, vehicle, TO_CHAR);

	/* message to room */
	if ( onum > 1 )
		strcpy(buf, "$n discharge some $o from $v.");
	else
		strcpy(buf, "$n discharge $p from $v.");
	act(buf, FALSE, ch, obj, vehicle, TO_ROOM);
}

/*
 * Syntax:
 *
 * - discharge <obj> <vehicle>
 * - discharge <num> <obj> <vehicle>
 * - discharge all.<obj> <vehicle>
 * - discharge all <vehicle>
 */
ACMD(do_discharge)
{
	OBJ_DATA *obj;
	VEHICLE_DATA *vehicle;
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	bool bNumArg = FALSE;
	int onum = 1;
	int dotmode, cont_dotmode;

	if ( !*argument )
	{
		send_to_char("Discharge what?\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg1);
	
	if ( is_number(arg1) )
	{
		bNumArg = TRUE;
		onum = atoi(arg1);
		if ( onum < 1 )
		{
			send_to_char("Don't be stupid!\r\n", ch);
			return;
		}
		argument = one_argument(argument, arg1);
	}

	argument = one_argument(argument, arg2);
	if ( !str_cmp( arg2, "from" ) )
		argument = one_argument(argument, arg2);

	/*
	 * ok.. now:
	 * arg1 holds obj name or 'all'
	 * arg2 holds vehicle name
	 */

	cont_dotmode = find_all_dots(arg2);

	if ( !*arg2 )
	{
		ch_printf(ch, "Discharge %s from which vehicle?\r\n", arg1);
		return;
	}

	if (cont_dotmode != FIND_INDIV)
	{
		send_to_char("You can't do that.\r\n", ch);
		return;
	}

	if ( !( vehicle = find_vehicle_in_room_by_name(ch, arg2) ) )
	{
		sprintf(buf, "You don't have %s %s.\r\n", AN(arg2), arg2);
		send_to_char(buf, ch);
	}

	if ( vehicle->owner_id != GET_IDNUM(ch) && !IS_IMMORTAL(ch) )
	{
		ch_printf(ch, "%s is not yours that you can use it.\r\n",
			vehicle->short_description);
		return;
	}

	dotmode = find_all_dots(arg1);

	/* discharge <obj> <vehicle> or discharge <num> <obj> <vehicle> */
	if ( dotmode == FIND_INDIV )
	{
		if (!(obj = get_obj_in_list_vis_rev(ch, arg1, NULL, vehicle->last_content)))
		{
			sprintf(buf, "There doesn't seem to be %s %s in $v.", AN(arg1), arg1);
			act(buf, FALSE, ch, NULL, vehicle, TO_CHAR);
			return;
		}

		if ( GET_OBJ_TYPE(obj) == ITEM_MONEY )
		{
			if ( bNumArg )
			{
				int amount = GET_OBJ_VAL(obj, 0);
				
				// she wants more than what's there..
				if (onum > amount)
				{
					act("There isn't that much gold in $v.", FALSE, ch, NULL, vehicle, TO_CHAR);
					return;
				}
				// she wants less than what's there..
				else if (onum < amount)
				{
					int diff = amount - onum;
					
					// create a new money obj for the rest..
					create_amount(diff, NULL, NULL, NULL, vehicle, TRUE);
					// set the value of the existing object
					// to what has to be taken
					GET_OBJ_VAL(obj, 0) = onum;
					// let's cascade till perform_discharge_obj()
				}
				// she wants what's there..
				// let's cascade till perform_discharge_obj()
			}
		}
		else
		{
			if (onum > obj->count)
			{
				act("There aren't so many $o in $v.", FALSE, ch, obj, vehicle, TO_CHAR);
				return;
			}
			
			split_obj(obj, onum);
		}
		
		perform_discharge_obj(ch, obj, vehicle);
	}
	/* discharge all.<obj> <vehicle> or discharge all <vehicle> */
	else
	{
		OBJ_DATA *next_obj;
		bool found = FALSE;

		if (cont_dotmode == FIND_ALLDOT && !*arg2)
		{
			send_to_char("Discharge from all of what?\r\n", ch);
			return;
		}

		for (obj = vehicle->first_content; obj; obj = next_obj)
		{
			next_obj = obj->next_content;

			if (CAN_SEE_OBJ(ch, obj) && 
				(dotmode == FIND_ALL || isname(arg1, obj->name)))
			{
				found = TRUE;
				perform_discharge_obj(ch, obj, vehicle);
			}
		}

		if (!found)
		{
			if (dotmode == FIND_ALL)
				act("$v seems to be empty.", FALSE, ch, NULL, vehicle, TO_CHAR);
			else
			{
				sprintf(buf, "You can't seem to find any %ss in $v.", arg1);
				act(buf, FALSE, ch, NULL, vehicle, TO_CHAR);
			}
		}
	}
}
