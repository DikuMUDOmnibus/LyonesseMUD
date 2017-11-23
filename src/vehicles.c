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
 * File: vehicles.c                                                       *
 *                                                                        *
 * Vehicles code                                                          *
 *                                                                        *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"

/* external functions */
char	*make_bar(long val, long max, long len, int color);
void	free_room(ROOM_DATA *room);
void	rent_vehicle( CHAR_DATA *ch, VEHICLE_DATA *vehicle );
void	check_wild_move(CHAR_DATA *ch, VEHICLE_DATA *vehicle, SHIP_DATA *ship, FERRY_DATA *ferry, ROOM_DATA *pRoomFrom, ROOM_DATA *pRoomTo);
void	wild_check_for_remove( ROOM_DATA *wRoom );
void	wild_remove_dequeue( ROOM_DATA *wRoom );
void	list_obj_to_char(OBJ_DATA *list, CHAR_DATA *ch, int mode, int show, bool RDR);
void	list_char_to_char(CHAR_DATA *list, CHAR_DATA *ch);

/* globals */
VEHICLE_DATA	*first_vehicle	= NULL;
VEHICLE_DATA	*last_vehicle	= NULL;
VEHICLE_INDEX	*vehicle_types[MAX_VEH_TYPE];

/* locals */
bool	can_see_vehicle( CHAR_DATA *ch, VEHICLE_DATA *vehicle );
void	mob_from_yoke( CHAR_DATA *mob, VEHICLE_DATA *vehicle );


/* *************************************************************************** */
/* Create / Destroy Vehicles Routines                                          */
/* *************************************************************************** */

void clear_vehicle( VEHICLE_DATA *pVeh )
{
	if ( pVeh->name )
		STRFREE(pVeh->name);
	if ( pVeh->short_description )
		STRFREE(pVeh->short_description);
	if ( pVeh->description )
		STRFREE(pVeh->description);

	if (pVeh->veh_room)
	{
		free_room(pVeh->veh_room);
		DISPOSE(pVeh->veh_room);
	}

	pVeh->next			= NULL;
	pVeh->prev			= NULL;
	pVeh->in_room		= NULL;
	pVeh->last_room		= NULL;
	pVeh->first_content	= NULL;
	pVeh->last_content	= NULL;
	pVeh->people		= NULL;
	pVeh->wagoner		= NULL;
	pVeh->next_in_room	= NULL;
	pVeh->veh_room		= NULL;
	pVeh->first_yoke	= NULL;
	pVeh->last_yoke		= NULL;
	pVeh->type			= NULL;

	pVeh->flags			= 0;
}


void new_vehicle_room( VEHICLE_DATA *pVeh )
{
	pVeh->veh_room = new_room();

	pVeh->veh_room->sector_type		= SECT_INSIDE;
	pVeh->veh_room->zone			= NOWHERE;

	switch (pVeh->type->vnum)
	{
	case VEH_CART:
		/* set up vehicle's room descriptions */
		pVeh->veh_room->name		= str_dup("Inside a cart");
		pVeh->veh_room->description	= str_dup("You are inside a cart.\r\n");
		break;
	case VEH_WAGON:
		/* set up vehicle's room descriptions */
		pVeh->veh_room->name		= str_dup("Inside a wagon");
		pVeh->veh_room->description	= str_dup("You are inside a four-wheeled wagon.\r\n");
		break;
	default:
		log("SYSERR: new_vehicle_room() - unknown vehicle type %d.", pVeh->type->vnum);
		DISPOSE(pVeh->veh_room);
	}
}


VEHICLE_DATA *new_vehicle(void)
{
	VEHICLE_DATA *pVeh;

	CREATE(pVeh, VEHICLE_DATA, 1);

	clear_vehicle(pVeh);

	return (pVeh);
}

/* create a new vehicle of the given type */
VEHICLE_DATA *init_vehicle( VEHICLE_INDEX *vType )
{
	VEHICLE_DATA *pVeh;

	if (!vType)
	{
		log("SYSERR: init_vehicle() - unused vehicle proto.");
		return (NULL);
	}

	if ( vType->vnum < 0 || vType->vnum >= MAX_VEH_TYPE )
	{
		log("SYSERR: init_vehicle() - wrong vehicle type %d.", vType->vnum);
		return (NULL);
	}

	pVeh = new_vehicle();

	pVeh->type = vType;

	// copy strings
	pVeh->name					= str_dup(vType->name);
	pVeh->short_description		= str_dup(vType->short_description);
	pVeh->description			= str_dup(vType->description);

	// copy value
	pVeh->max_val = vType->value;

	// init some current values, others must be 0
	pVeh->curr_val.health		= pVeh->max_val.health;
	pVeh->curr_val.speed		= pVeh->max_val.speed;

	// if needed, setup inside rooms
	if ( pVeh->max_val.passengers > 0 )
		new_vehicle_room( pVeh );

	/* add to the list */
	LINK(pVeh, first_vehicle, last_vehicle, next, prev);

	return (pVeh);
}

/*
 * extract a vehicle from world
 * mode = 0 - leave mobs and objs in the room it was
 * mode = 1 - extract mobs and objs too
 * mode = 2 - from destroy_vehicle()
 */
void extract_vehicle( VEHICLE_DATA *vehicle, int mode )
{
	// destroyed vehicles don't loose theirs freights
	if (mode != 2)
	{
		if ( vehicle->first_content )
		{
			OBJ_DATA *obj, *next_obj = NULL;
			
			for ( obj = vehicle->first_content; obj; obj = next_obj )
			{
				next_obj = obj->next_content;
				if (mode == 1)
					extract_obj(obj);
				else
				{
					obj_from_vehicle(obj);
					obj_to_room(obj, vehicle->in_room);
				}
			}
		}
	}

	if ( vehicle->people )
	{
		CHAR_DATA *ppl, *next_ppl = NULL;

		for ( ppl = vehicle->people; ppl; ppl = next_ppl )
		{
			next_ppl = ppl->next_in_vehicle;
			char_from_vehicle(ppl);
			char_from_room(ppl);
			char_to_room(ppl, vehicle->in_room);
		}
	}

	if ( vehicle->first_yoke )
	{
		YOKE_DATA *yoke, *next_yoke = NULL;

		for ( yoke = vehicle->first_yoke; yoke; yoke = next_yoke )
		{
			next_yoke = yoke->next;

			if (mode == 1)
				extract_char_final(yoke->mob);
			else
				mob_from_yoke(yoke->mob, vehicle);
		}
	}

	if ( vehicle->veh_room )
	{
		free_room( vehicle->veh_room );
		DISPOSE(vehicle->veh_room);
	}

	if ( vehicle->wagoner )
		stop_be_wagoner( vehicle->wagoner );

	// destroyed vehicles must remain in place....
	if (mode == 2)
		return;

	if ( vehicle->in_room )
		vehicle_from_room(vehicle);

	UNLINK(vehicle, first_vehicle, last_vehicle, next, prev);

	clear_vehicle(vehicle);

	DISPOSE(vehicle);
}

/* *************************************************************** */
/* Finding Vehicles Routines                                       */
/* *************************************************************** */

/* find the vehicle searching the entire world */
VEHICLE_DATA *find_vehicle(char *name)
{
	VEHICLE_DATA *pVeh;
	int fnum;

	if (!name)
		return (NULL);

	if (!(fnum = get_number(&name)))
		return (NULL);

	for (pVeh = first_vehicle; pVeh; pVeh = pVeh->next)
	{
		if (isname(name, pVeh->name) || !str_cmp(name, "vehicle"))
			if (--fnum == 0)
				break;
	}

	return (pVeh);

}

/* find the vehicle in room */
VEHICLE_DATA *find_vehicle_in_room_by_name( CHAR_DATA *ch, char *name )
{
	VEHICLE_DATA *pVeh;
	int fnum;

	if ( !ch->in_room->vehicles )
		return (NULL);

	if ( !name )
		return (NULL);

	if ( !( fnum = get_number(&name) ) )
		return (NULL);

	for ( pVeh = ch->in_room->vehicles; pVeh; pVeh = pVeh->next_in_room )
	{
		if ( isname( name, pVeh->name ) || !str_cmp( name, "vehicle" ) )
			if ( can_see_vehicle(ch, pVeh) )
				if ( --fnum == 0 )
					break;
	}

	return (pVeh);
}

bool empty_vehicle( VEHICLE_DATA *vehicle, bool bQuick )
{
	if ( vehicle->wagoner )			return (FALSE);
	if ( vehicle->people )			return (FALSE);

	if ( bQuick )					return (TRUE);

	if ( vehicle->first_yoke )		return (FALSE);
	if ( vehicle->first_content )	return (FALSE);

	return (TRUE);
}

/* *************************************************************** */
/* Enter/Exit Vehicle Routines                                     */
/* *************************************************************** */

void char_to_vehicle( CHAR_DATA *ch, VEHICLE_DATA *vehicle )
{
	if ( !ch || !vehicle )
	{
		log("SYSERR: char_to_vehicle() - NULL pointer.");
		return;
	}

	ch->in_vehicle		= vehicle;

	char_to_room(ch, vehicle->veh_room);

	/* add to the ppl list */
	ch->next_in_vehicle	= vehicle->people;
	vehicle->people		= ch;

	vehicle->curr_val.passengers++;
}

void char_from_vehicle( CHAR_DATA *ch )
{
	CHAR_DATA *temp;

	if ( !ch )
	{
		log("SYSERR: char_from_vehicle() - NULL pointer.");
		return;
	}

	if ( !ch->in_vehicle )
		return;

	ch->in_vehicle->curr_val.passengers--;

	REMOVE_FROM_LIST(ch, ch->in_vehicle->people, next_in_vehicle);
	ch->in_vehicle		= NULL;
	ch->next_in_vehicle	= NULL;
}

void char_enter_vehicle( CHAR_DATA *ch, VEHICLE_DATA *vehicle )
{
	if ( !ch || !vehicle )
	{
		log("SYSERR: char_to_vehicle() - NULL pointer.");
		return;
	}

	if ( RIDING(ch) )
	{
		send_to_char("You cannot enter a vehicle while mounted.\r\n", ch);
		return;
	}

	if ( WAGONER(ch) )
	{
		send_to_char("You cannot enter a vehicle while driving a vehicle.\r\n", ch);
		return;
	}

	if ( ch->in_vehicle )
	{
		send_to_char("You are already inside a vehicle.\r\n", ch);
		return;
	}

	if (IS_SET(vehicle->flags, VEH_DESTROYED))
	{
		send_to_char("You cannot enter a destroyed vehicle.\r\n", ch);
		return;
	}

	if ( vehicle->owner_id != GET_IDNUM(ch) && !IS_IMMORTAL(ch) )
	{
		ch_printf(ch, "%s is not yours that you can use it.\r\n",
			vehicle->short_description);
		return;
	}

	if ( !vehicle->max_val.passengers )
	{
		ch_printf(ch, "%s cannot have passengers.\r\n", vehicle->short_description);
		return;
	}

	if ( vehicle->curr_val.passengers >= vehicle->max_val.passengers )
	{
		send_to_char("It's full.\r\n", ch);
		return;
	}

	// sanity check
	if ( !vehicle->veh_room )
	{
		ch_printf(ch, "Sorry, right now you cannot go inside %s.\r\n", vehicle->short_description);
		return;
	}

	act("You enter $v.", FALSE, ch, NULL, vehicle, TO_CHAR);
	act("$n enters $v.", FALSE, ch, NULL, vehicle, TO_ROOM);

	char_from_room(ch);
	char_to_vehicle(ch, vehicle);
	look_at_room(ch, 0);
}

void char_exit_vehicle( CHAR_DATA *ch )
{
	ROOM_DATA *pRoom;

	if ( !ch )
	{
		log("SYSERR: char_exit_vehicle() - NULL pointer.");
		return;
	}

	if ( !ch->in_vehicle )
		return;

	pRoom = ch->in_vehicle->in_room;

	// TODO - check if char can move in to the room..

	act("You exit from $v.", FALSE, ch, NULL, ch->in_vehicle, TO_CHAR);
	act("$n exits from $v.", FALSE, ch, NULL, ch->in_vehicle, TO_ROOM);

	char_from_vehicle(ch);
	char_from_room(ch);
	char_to_room(ch, pRoom);
	look_at_room(ch, 0);
}

/* *************************************************************** */
/* Wagoner Routines                                                */
/* *************************************************************** */

void start_be_wagoner( CHAR_DATA *ch, VEHICLE_DATA *vehicle )
{
	if ( !ch || !vehicle )
	{
		log("SYSERR: start_be_wagoner() - NULL pointer.");
		return;
	}

	if ( WAGONER(ch) )
	{
		ch_printf(ch, "You're already driving %s.\r\n", WAGONER(ch)->short_description);
		return;
	}

	if ( RIDING(ch) )
	{
		send_to_char("You cannot drive a vehicle while mounted.\r\n", ch);
		return;
	}

	if (IS_SET(vehicle->flags, VEH_DESTROYED))
	{
		send_to_char("You cannot drive a destroyed vehicle.\r\n", ch);
		return;
	}

	if ( vehicle->wagoner )
	{
		ch_printf(ch, "%s is already drived by %s.\r\n",
			vehicle->short_description, PERS(vehicle->wagoner, ch));
		return;
	}

	if ( vehicle->owner_id != GET_IDNUM(ch) && !IS_IMMORTAL(ch) )
	{
		ch_printf(ch, "%s is not yours that you can use it.\r\n",
			vehicle->short_description);
		return;
	}

	WAGONER(ch)				= vehicle;
	WAGONER(ch)->wagoner	= ch;

	act("You climb up and start driving $v.", FALSE, ch, NULL, vehicle, TO_CHAR);
	act("$n climbs up and start driving $v.", FALSE, ch, NULL, vehicle, TO_ROOM);
}

/*
 * messages to char are not inside here because
 * this function is called also by extract_char()
 * and from check inside do_simple_move()
 */
void stop_be_wagoner( CHAR_DATA *ch )
{
	if ( !ch )
	{
		log("SYSERR: stop_be_wagoner() - NULL pointer.");
		return;
	}

	if ( !WAGONER(ch) )
	{
		log("SYSERR: ch is not driving any vehicle.");
		return;
	}

	WAGONER(ch)->wagoner	= NULL;
	WAGONER(ch)				= NULL;
}

/* *************************************************************** */
/* Yokeing Mobs Routines                                           */
/* *************************************************************** */

bool hitched_mob( CHAR_DATA *mob )
{
	if ( !IS_NPC(mob) )
		return (FALSE);

	if ( !mob->mob_specials.hitched_to )
		return (FALSE);

	/* this should never happen!! */
	if ( mob->in_room != mob->mob_specials.hitched_to->in_room )
		return (FALSE);

	return (TRUE);
}

void mob_to_yoke( CHAR_DATA *mob, VEHICLE_DATA *vehicle )
{
	YOKE_DATA *yoke;

	if (!mob || !vehicle)
		return;

	if (IS_SET(vehicle->flags, VEH_DESTROYED))
		return;

	CREATE(yoke, YOKE_DATA, 1);
	yoke->mob						= mob;

	LINK(yoke, vehicle->first_yoke, vehicle->last_yoke, next, prev);

	mob->mob_specials.hitched_to	= vehicle;
	mob->mob_specials.yoke			= yoke;

	vehicle->curr_val.draft_mobs++;
}

void mob_from_yoke( CHAR_DATA *mob, VEHICLE_DATA *vehicle )
{
	YOKE_DATA *yoke;

	if ( !mob->mob_specials.hitched_to )
		return;

	if ( !(yoke = mob->mob_specials.yoke) )
		return;
	
	vehicle->curr_val.draft_mobs--;

	UNLINK(yoke, vehicle->first_yoke, vehicle->last_yoke, next, prev);
	yoke->next						= NULL;
	yoke->prev						= NULL;
	yoke->mob						= NULL;
	DISPOSE(yoke);

	mob->mob_specials.hitched_to	= NULL;
	mob->mob_specials.yoke			= NULL;
}

/* *************************************************************** */
/* Vehicle Movement Routines                                       */
/* *************************************************************** */

void vehicle_to_room( VEHICLE_DATA *pVeh, ROOM_DATA *pRoom )
{
	if ( !pVeh || !pRoom )
	{
		log("SYSERR: vehicle_to_room() - invalid pointer.");
		return;
	}

	/* add to the room list */
	pVeh->next_in_room	= pRoom->vehicles;
	pRoom->vehicles		= pVeh;

	pVeh->in_room		= pRoom;

	/* Handle Wild Sectors */
	if ( IS_WILD(pRoom) )
	{
		check_wild_move(NULL, pVeh, NULL, NULL, pVeh->last_room, pRoom);

		if (ROOM_FLAGGED(pRoom, ROOM_WILD_REMOVE))
			wild_remove_dequeue(pRoom);
	}
}

void vehicle_from_room( VEHICLE_DATA *pVeh )
{
	VEHICLE_DATA *temp;

	if ( !pVeh )
	{
		log("SYSERR: NULL vehicle pointer in vehicle_from_room");
		exit(1);
	}

	pVeh->last_room		= pVeh->in_room;

	REMOVE_FROM_LIST(pVeh, pVeh->in_room->vehicles, next_in_room);
	pVeh->in_room		= NULL;
	pVeh->next_in_room	= NULL;

	wild_check_for_remove(pVeh->last_room);
}

bool vehicle_can_move( VEHICLE_DATA *vehicle )
{
	if (!vehicle)								return (FALSE);
	if (!vehicle->first_yoke)					return (FALSE);
	if (!vehicle->wagoner)						return (FALSE);
	if (IS_SET(vehicle->flags, VEH_DESTROYED))	return (FALSE);

	return (TRUE);
}

bool vehicle_can_go( VEHICLE_DATA *pVeh, ROOM_DATA *to_room )
{
	/*
	 * that's an hack.. in future must be handled
	 * in do_simple_move()
	 */
	if ( ROOM_FLAGGED(to_room, ROOM_DEATH) )
		return (FALSE);

	if ( IS_SET(pVeh->flags, VEH_FLY) )
		return (TRUE);

	if ( to_room->sector_type == SECT_ROAD		|| 
	     to_room->sector_type == SECT_FIELD		||
	     to_room->sector_type == SECT_PLAIN		||
	     to_room->sector_type == SECT_CITY		||
	     to_room->sector_type == SECT_LAND		||
	     to_room->sector_type == SECT_BRIDGE	||
	     to_room->sector_type == SECT_FORD		||
	     to_room->sector_type == SECT_PORT)
		return (TRUE);

	return (FALSE);
}


/* *************************************************************** */
/* Goods Storing in Vehicle Routines                               */
/* *************************************************************** */

OBJ_DATA *obj_to_vehicle( OBJ_DATA *obj, VEHICLE_DATA *vehicle )
{
	OBJ_DATA *oret, *tmp_obj;
	
	if (!obj || !vehicle)
	{
		log("SYSERR: obj_to_vehicle() - NULL pointer");
		return (NULL);
	}

	vehicle->curr_val.capacity += get_real_obj_weight(obj);

	for ( tmp_obj = vehicle->first_content; tmp_obj; tmp_obj = tmp_obj->next_content )
	{
		if ( (oret = group_object(tmp_obj, obj)) == tmp_obj )
			return (oret);
	}

	LINK(obj, vehicle->first_content, vehicle->last_content, next_content, prev_content);

	obj->in_vehicle		= vehicle;
	obj->in_room		= NULL;
	obj->in_obj			= NULL;
	obj->carried_by		= NULL;

	return (obj);
}

/* Take a goods object from a vehicle */
void obj_from_vehicle( OBJ_DATA *obj )
{
	if (!obj || !obj->in_vehicle)
	{
		log("SYSERR: obj_from_vehicle() - NULL pointer");
		return;
	}

	obj->in_vehicle->curr_val.capacity -= get_real_obj_weight(obj);

	UNLINK(obj, obj->in_vehicle->first_content, obj->in_vehicle->last_content,
		next_content, prev_content);

	obj->in_obj			= NULL;
	obj->in_room		= NULL;
	obj->carried_by		= NULL;
	obj->in_vehicle		= NULL;
	obj->next_content	= NULL;
	obj->prev_content	= NULL;
}

void vehicle_load_goods( CHAR_DATA *ch, OBJ_DATA *goods, VEHICLE_DATA *vehicle )
{
	if ( !ch || !goods || !vehicle )
	{
		log("SYSERR: vehicle_load_goods() - NULL pointer.");
		return;
	}

	if (IS_SET(vehicle->flags, VEH_DESTROYED))
	{
		send_to_char("You cannot put items in a destroyed vehicle.\r\n", ch);
		return;
	}

	if ( vehicle->curr_val.capacity + get_real_obj_weight(goods) > vehicle->max_val.capacity )
	{
		ch_printf(ch, "%s cannot contain that much.\r\n", vehicle->short_description);
		return;
	}

	obj_from_room(goods);
	obj_to_vehicle(goods, vehicle);

	ch_printf(ch, "You charge %s into %s.\r\n", goods->short_description, vehicle->short_description);
}

void vehicle_unload_goods( CHAR_DATA *ch, OBJ_DATA *goods, VEHICLE_DATA *vehicle )
{
	if ( !ch || !goods || !vehicle )
	{
		log("SYSERR: vehicle_unload_goods() - NULL pointer.");
		return;
	}

	obj_from_vehicle(goods);
	obj_to_room(goods, ch->in_room);

	ch_printf(ch, "You discharge %s from %s.\r\n", goods->short_description, vehicle->short_description);
}

/* *************************************************************** */
/* Vehicle Display Routines                                        */
/* *************************************************************** */

/* True if char can see vehicle */
bool can_see_vehicle( CHAR_DATA *ch, VEHICLE_DATA *vehicle )
{
	if ( !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_HOLYLIGHT) )
		return (TRUE);
	
	if ( AFF_FLAGGED( ch, AFF_BLIND ) )
		return (FALSE);
	
	if ( IS_DARK(vehicle->in_room) && !AFF_FLAGGED(ch, AFF_INFRAVISION) )
		return (FALSE);

	if ( IS_SET(vehicle->flags, VEH_INVISIBLE) && !AFF_FLAGGED(ch, AFF_DETECT_INVIS) )
		return (FALSE);
	
	return (TRUE);
}

void list_vehicle_to_char( VEHICLE_DATA *vlist, CHAR_DATA *ch )
{
	VEHICLE_DATA *pVeh;
	char sbaf[MAX_STRING_LENGTH];

	for ( pVeh = vlist; pVeh; pVeh = pVeh->next_in_room )
	{
		if ( WAGONER(ch) && WAGONER(ch) == pVeh )
			continue;

		// needed because of "look out" command..
		if ( ch->in_vehicle && ch->in_vehicle == pVeh )
			continue;

		if ( can_see_vehicle(ch, pVeh) )
		{
			if (IS_SET(pVeh->flags, VEH_DESTROYED))
				sprintf(sbaf, "&b&5You see a destroyed %s", pVeh->name);
			else
				sprintf(sbaf, "&b&5You see %s", pVeh->short_description);

			if (pVeh->wagoner)
				sprintf(sbaf+strlen(sbaf), ", drived by %s", PERS(pVeh->wagoner, ch));

			if (pVeh->first_yoke)
			{
				YOKE_DATA *yoke;
				
				strcat(sbaf, ", dragged by ");

				for ( yoke = pVeh->first_yoke; yoke; yoke = yoke->next )
					sprintf(sbaf+strlen(sbaf), "%s%s",
						GET_NAME(yoke->mob),
						(yoke->next ? ", " : " ")
						);
			}
			strcat(sbaf, ".&0\r\n");

			send_to_char(sbaf, ch);
		}
	}
}


int look_at_vehicle(CHAR_DATA *ch, VEHICLE_DATA *vehicle)
{
	char sbaf[MAX_STRING_LENGTH];

	if (!can_see_vehicle(ch, vehicle))
		return (0);

	sprintf(sbaf,
		"%s%s\r\n"
		"Capacity : %s (%5d/%5d)\r\n"
		"Health   : %s (%5d/%5d)\r\n"
		,
		vehicle->description,
		(IS_SET(vehicle->flags, VEH_DESTROYED) ? " [destroyed]" : ""),
		make_bar(vehicle->curr_val.capacity, vehicle->max_val.capacity, 40, 6),
		vehicle->curr_val.capacity, vehicle->max_val.capacity,
		make_bar(vehicle->curr_val.health, vehicle->max_val.health, 40, 6),
		vehicle->curr_val.health, vehicle->max_val.health
		);

	send_to_char(sbaf, ch);

	if (vehicle->people)
		list_char_to_char(vehicle->people, ch);

	if (vehicle->first_content)
	{
		send_to_char("When you look inside you see:\r\n", ch);
		list_obj_to_char(vehicle->first_content, ch, 7, 1, FALSE);
	}

	return (1);
}


const char *veh_flags_descr[] =
{
  "FLYING",
  "INVISIBLE",
  "DESTROYED",
  "\n"
};

void stat_vehicle(CHAR_DATA *ch, VEHICLE_DATA *vehicle)
{
	char sbaf[MAX_STRING_LENGTH], dflags[128];

	if (!can_see_vehicle(ch, vehicle))
		return;

	sprintbit(vehicle->flags, veh_flags_descr, dflags);
	sprintf(sbaf,
		"%s%s\r\n"
		"Flags    : %s\r\n"
		"Capacity : %s (%5d/%5d)\r\n"
		"Health   : %s (%5d/%5d)\r\n"
		,
		vehicle->description,
		(IS_SET(vehicle->flags, VEH_DESTROYED) ? " [destroyed]" : ""),
		dflags,
		make_bar(vehicle->curr_val.capacity, vehicle->max_val.capacity, 40, 6),
		vehicle->curr_val.capacity, vehicle->max_val.capacity,
		make_bar(vehicle->curr_val.health, vehicle->max_val.health, 40, 6),
		vehicle->curr_val.health, vehicle->max_val.health
		);

	send_to_char(sbaf, ch);

	if (vehicle->people)
		list_char_to_char(vehicle->people, ch);

	if (vehicle->first_content)
	{
		send_to_char("When you look inside you see:\r\n", ch);
		list_obj_to_char(vehicle->first_content, ch, 7, 1, FALSE);
	}
}

/* *************************************************************** */
/* Vehicle Proto                                                   */
/* *************************************************************** */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )					\
				if ( !strcmp( word, literal ) )			\
				{										\
				    field  = value;						\
				    fMatch = TRUE;						\
				    break;								\
				}


VEHICLE_INDEX *new_veh_proto(void)
{
	VEHICLE_INDEX *vType = NULL;

	CREATE( vType, VEHICLE_INDEX, 1 );
	vType->name					= NULL;
	vType->short_description	= NULL;
	vType->description			= NULL;
	vType->vnum					= NOTHING;

	return (vType);
}

VEHICLE_INDEX *fread_one_veh_proto( FILE *fp )
{
	VEHICLE_INDEX *vType = new_veh_proto();
	char *word;
	bool fMatch;

	for ( ; ; )
	{
		word   = feof( fp ) ? "End" : fread_word( fp );
		fMatch = FALSE;
		
		switch (UPPER(word[0]))
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol(fp);
			break;

		case 'C':
			KEY("Capacity",			vType->value.capacity,		fread_number(fp));
			break;

		case 'D':
			KEY("Descr",			vType->description,			fread_string_nospace(fp));
			KEY("Draft_mobs",		vType->value.draft_mobs,	fread_number(fp));
			break;

		case 'E':
			if (!str_cmp(word, "End"))
			{
				return (vType);
			}
			break;

		case 'H':
			KEY("Health",			vType->value.health,		fread_number(fp));
			break;

		case 'N':
			KEY("Name",				vType->name,				str_dup(fread_word(fp)));
			break;

		case 'P':
			KEY("Passengers",		vType->value.passengers,	fread_number(fp));
			break;

		case 'S':
			KEY("Short_descr",		vType->short_description,	fread_string_nospace(fp));
			KEY("Speed",			vType->value.speed,			fread_number(fp));
			break;

		case 'V':
			KEY("Vnum",				vType->vnum,				fread_number(fp));
			break;
		}
	}
}

void LoadVehiclesTable(void)
{
	FILE *fp;
	char fname[128];
	char letter;
	char *word;
	int num_of_veh_proto;

	sprintf(fname, "%svehicles.data", LIB_DATA);
	if (!(fp = fopen(fname, "r")))
		return;

	for (num_of_veh_proto = 0; num_of_veh_proto < MAX_VEH_TYPE; num_of_veh_proto++)
		vehicle_types[num_of_veh_proto] = NULL;

	num_of_veh_proto = 0;

	for ( ; ; )
	{
		letter = fread_letter(fp);
		if (letter == '*')
		{
			fread_to_eol(fp);
			continue;
		}
		
		if (letter != '#')
		{
			log("SYSERR: LoadVehiclesTable() - # not found.");
			break;
		}
		
		word = fread_word(fp);

		if (!str_cmp(word, "VEHICLE"))
		{
			if (num_of_veh_proto >= MAX_VEH_TYPE)
			{
				log("SYSERR: LoadVehiclesTable() - more prototypes than MAX_VEH_TYPE %d", MAX_VEH_TYPE);
				fclose(fp);
				return;
			}
			vehicle_types[num_of_veh_proto] = fread_one_veh_proto(fp);
			num_of_veh_proto++;
		}
		else if (!str_cmp(word, "END"))
			break;
		else
		{
			log("SYSERR: LoadVehiclesTable() - bad section %s.", word);
			continue;
		}
	}

	fclose(fp);
}


/* *************************************************************** */
/* Commands                                                        */
/* *************************************************************** */

/* wiz only */
ACMD(do_newvehicle)
{
	VEHICLE_DATA *pVeh;
	char arg[MAX_INPUT_LENGTH];
	int vnum;

	argument = one_argument(argument, arg);

	if ( !*arg )
	{
		send_to_char("Possible vehicles are:\r\n", ch);
		for (vnum = 0; vnum < MAX_VEH_TYPE; vnum++)
		{
			if (!vehicle_types[vnum])
				continue;
			ch_printf(ch, " (%d) %s\r\n", vnum, vehicle_types[vnum]->name);
		}
		send_to_char("Which type of vehicle?\r\n", ch);
		return;
	}

	if ( is_number(arg) )
		vnum = atoi(arg);
	else
	{
		for (vnum = 0; vnum < MAX_VEH_TYPE; vnum++)
		{
			if (!vehicle_types[vnum])
				continue;
			if (is_abbrev(arg, vehicle_types[vnum]->name))
				break;
		}
	}

	if (vnum < 0 || vnum >= MAX_VEH_TYPE || !vehicle_types[vnum])
	{
		ch_printf(ch, "Invalid vehicle type '%s'.\r\n", arg);
		return;
	}

	if ( ( pVeh = init_vehicle( vehicle_types[vnum] ) ) )
	{
		pVeh->owner_id = GET_IDNUM(ch);
		vehicle_to_room(pVeh, ch->in_room);
		ch_printf(ch, "You create %s out of the blue.\r\n", vehicle_types[vnum]->short_description);
	}
}

/*
 * quick exit from a vehicle
 */
ACMD(do_out)
{
	one_argument(argument, arg);

	if ( !ch->in_vehicle )
	{
		send_to_char("You are not inside a vehicle.\r\n", ch);
		return;
	}

	if ( !str_cmp(arg, "save") )
	{
		rent_vehicle(ch, ch->in_vehicle);
		return;
	}

	char_exit_vehicle(ch);
}

/*
 * so she wanna be a wagoner, eh?
 */
ACMD(do_drive)
{
	VEHICLE_DATA *vehicle;
	char arg1[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg1);

	if ( !*arg1 )
	{
		send_to_char(
			"Usage:\r\n"
			"drive <vehicle> - you start drive a vehicle.\r\n"
			"drive stop      - you stop drive a vehicle.\r\n", ch);
		return;
	}

	if ( !str_cmp(arg1, "stop") )
	{
		if ( !( vehicle = WAGONER(ch) ) )
		{
			send_to_char("You are not driving any vehicle.\r\n", ch);
			return;
		}
		
		act("You stop driving $v.", FALSE, ch, NULL, vehicle, TO_CHAR);
		act("$n stops driving $v.", FALSE, ch, NULL, vehicle, TO_ROOM);
		stop_be_wagoner(ch);
	}
	else
	{
		if ( !( vehicle = find_vehicle_in_room_by_name(ch, arg1) ) )
		{
			ch_printf(ch, "You don't see %s %s here..\r\n", AN(arg1), arg1);
			return;
		}

		if (IS_SET(vehicle->flags, VEH_DESTROYED))
		{
			send_to_char("You cannot drive a destroyed vehicle.\r\n", ch);
			return;
		}

		start_be_wagoner(ch, vehicle);
	}
}

/* attach a mob to the vehicle */
ACMD(do_yoke)
{
	CHAR_DATA *mob;
	VEHICLE_DATA *vehicle;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	if ( RIDING(ch) )
	{
		send_to_char("You cannot yoke draft animal to a vehicle while mounted.\r\n", ch);
		return;
	}

	if ( WAGONER(ch) )
	{
		send_to_char("You cannot yoke draft animal to a vehicle while driving the vehicle.\r\n", ch);
		return;
	}

	argument = two_arguments(argument, arg1, arg2);

	if ( !*arg1 || !*arg2 )
	{
		send_to_char("Usage: yoke <mob> <vehicle>\r\n", ch);
		return;
	}

	if ( !( mob = get_char_room_vis(ch, arg1, NULL) ) )
	{
		ch_printf(ch, "You don't see %s %s here.\r\n", AN(arg1), arg1);
		return;
	}

	if ( !IS_NPC(mob) || !MOB_FLAGGED(mob, MOB_DRAFT_ANIMAL))
	{
		send_to_char("You can yoke only draft animal to a vehicle.\r\n", ch);
		return;
	}

	if ( !( vehicle = find_vehicle_in_room_by_name(ch, arg2) ) )
	{
		ch_printf(ch, "You don't see %s %s here.\r\n", AN(arg2), arg2);
		return;
	}

	if (IS_SET(vehicle->flags, VEH_DESTROYED))
	{
		send_to_char("You cannot yoke animals to a destroyed vehicle.\r\n", ch);
		return;
	}

	if ( vehicle->owner_id != GET_IDNUM(ch) && !IS_IMMORTAL(ch) )
	{
		ch_printf(ch, "%s is not yours that you can yoke animals.\r\n",
			vehicle->short_description);
		return;
	}

	if ( !vehicle->max_val.draft_mobs  )
	{
		ch_printf(ch, "You cannot yoke draft animal to %s.\r\n", vehicle->short_description);
		return;
	}

	if ( vehicle->curr_val.draft_mobs >= vehicle->max_val.draft_mobs  )
	{
		ch_printf(ch, "You cannot yoke any more draft animal to %s.\r\n", vehicle->short_description);
		return;
	}

	mob_to_yoke(mob, vehicle);

	ch_printf(ch, "You yoke %s to %s.\r\n", GET_NAME(mob), vehicle->short_description);
	sprintf(buf, "$n yoke $N to %s.", vehicle->short_description);
	act(buf, FALSE, ch, NULL, mob, TO_ROOM);

}

ACMD(do_unyoke)
{
	CHAR_DATA *mob;
	VEHICLE_DATA *vehicle;
	char arg1[MAX_INPUT_LENGTH];

	if ( RIDING(ch) )
	{
		send_to_char("You cannot unyoke draft animal from a vehicle while mounted.\r\n", ch);
		return;
	}

	if ( WAGONER(ch) )
	{
		send_to_char("You cannot unyoke draft animal from a vehicle while driving the vehicle.\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg1);

	if ( !*arg1 )
	{
		send_to_char("Usage: unyoke <mob>\r\n", ch);
		return;
	}

	if ( !( mob = get_char_room_vis(ch, arg1, NULL) ) )
	{
		ch_printf(ch, "You don't see %s %s here.\r\n", AN(arg1), arg1);
		return;
	}

	if ( !IS_NPC(mob) || !MOB_FLAGGED(mob, MOB_DRAFT_ANIMAL))
	{
		send_to_char("You can unyoke only draft animal from a vehicle.\r\n", ch);
		return;
	}

	if ( !( vehicle = mob->mob_specials.hitched_to ) )
	{
		ch_printf(ch, "%s isn't yoked to any vehicle.\r\n", GET_NAME(mob));
		return;
	}

	if ( !mob->mob_specials.yoke )
	{
		log("SYSERR: do_unyoke() - yoked mob does not have yoke data.");
		send_to_char("There is a problem, call an Immortal.\r\n", ch);
		return;
	}

	if (IS_SET(vehicle->flags, VEH_DESTROYED))
	{
		send_to_char("You cannot unyoke animals from a destroyed vehicle.\r\n", ch);
		return;
	}

	if ( vehicle->owner_id != GET_IDNUM(ch) && !IS_IMMORTAL(ch) )
	{
		ch_printf(ch, "%s is not yours that you can unyoke animals.\r\n",
			vehicle->short_description);
		return;
	}

	ch_printf(ch, "You unyoke %s from %s.\r\n", GET_NAME(mob), vehicle->short_description);
	sprintf(buf, "$n unyoke $N from %s.", vehicle->short_description);
	act(buf, FALSE, ch, NULL, mob, TO_ROOM);

	mob_from_yoke(mob, vehicle);
}

/* ******************************************************* */
/* Code for repairing and damaging vehicles                */
/* ******************************************************* */

/* make some reparations */
void repair_vehicle(CHAR_DATA *ch, VEHICLE_DATA *pVeh)
{
	if (IS_SET(pVeh->flags, VEH_DESTROYED))
	{
		act("$v: you cannot repair destroyed vehicles.", FALSE, ch, NULL, pVeh, TO_CHAR);
		return;
	}

	if (pVeh->max_val.health == pVeh->curr_val.health)
	{
		act("$v does not need reparations.", FALSE, ch, NULL, pVeh, TO_CHAR);
		return;
	}

	if (!roll(GET_DEX(ch)))
	{
		send_to_char("You fail.\r\n", ch);
		return;
	}

	pVeh->curr_val.health++;
	act("You make sone reparations to $v.", FALSE, ch, NULL, pVeh, TO_CHAR);
	act("$n makes sone reparations to $v.", FALSE, ch, NULL, pVeh, TO_ROOM);
}

void destroy_vehicle(VEHICLE_DATA *pVeh)
{
	SET_BIT(pVeh->flags, VEH_DESTROYED);
	extract_vehicle(pVeh, 2);
}

/*
 * return 0 if vehicle is destroyed, >0 otherwise
 */
int damage_vehicle(VEHICLE_DATA *pVeh, int dam)
{
	if (dam < 0) dam = 0;

	pVeh->curr_val.health	-= dam;

	if (pVeh->curr_val.health <= 0)
	{
		pVeh->curr_val.health = 0;
		destroy_vehicle(pVeh);
		return (0);
	}

	return (1);
}

void hit_vehicle(CHAR_DATA *ch, VEHICLE_DATA *pVeh)
{
	OBJ_DATA *weapon;
	char dbuf[MAX_STRING_LENGTH];
	int dam;

	if (!ch || !pVeh)
		return;

	if (!(weapon = GET_EQ(ch, WEAR_WIELD)))
	{
		send_to_char("You cannot damage a building with your bare hands.\r\n", ch);
		return;
	}

	if (GET_OBJ_TYPE(weapon) != ITEM_WEAPON)
	{
		send_to_char("Using a weapon would surely help.\r\n", ch);
		return;
	}

	// calc damage
	dam = GET_REAL_DAMROLL(ch);
	dam += dice(GET_OBJ_VAL(weapon, 1), GET_OBJ_VAL(weapon, 2));

	// vehicle absorb damage..
	dam -= pVeh->max_val.health;

	// damage goes from 0 to 100
	dam = URANGE(0, dam, 100);

	// check for damaging the weapon used
	check_damage_obj(ch, weapon, 8);

	// no damage done
	if (!dam)
	{
		// message to attacker
		act("You strike $v, but make no damages.", FALSE, ch, NULL, pVeh, TO_CHAR);
		// message to room
		act("$n strikes $v but nothing happens.", FALSE, ch, NULL, pVeh, TO_ROOM);
		return;
	}

	// vehicle has been destroyed..
	if (!damage_vehicle(pVeh, dam))
	{
		// message to attacker
		act("Your attack DESTROY $v.", FALSE, ch, NULL, pVeh, TO_CHAR);
		// message to room
		act("$n DESTROYS $v with $s attack.", FALSE, ch, NULL, NULL, TO_ROOM);
		return;
	}

	// message to attacker
	sprintf(dbuf, "You strike $v, causing %d damages.", dam);
	act(dbuf, FALSE, ch, NULL, pVeh, TO_CHAR);
	// message to room
	act("$n strike $v, causing some damage.", FALSE, ch, NULL, pVeh, TO_ROOM);
}

bool attack_vehicle(CHAR_DATA *ch, char *arg)
{
	VEHICLE_DATA *pVeh;

	if (!ch || !*arg)
		return (FALSE);

	if (!(pVeh = find_vehicle_in_room_by_name(ch, arg)))
		return (FALSE);

	if (IS_SET(pVeh->flags, VEH_DESTROYED))
	{
		ch_printf(ch, "%s has already been destroyed.\r\n", pVeh->short_description);
		return (TRUE);
	}

	hit_vehicle(ch, pVeh);
	return (TRUE);
}
