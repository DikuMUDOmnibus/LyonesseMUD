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
 * File: wild.ships.c                                                     *
 *                                                                        *
 * Ships and Navigation code                                              *
 *                                                                        *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "constants.h"
#include "screen.h"
#include "interpreter.h"
#include "handler.h"
#include "spells.h"

/* external functions */
ROOM_DATA *create_wild_room( COORD_DATA *coord, bool Static );
OBJ_DATA *fread_one_obj( FILE *fp, int *location );
int		save_objs(OBJ_DATA *obj, FILE *fp, int mode, int location);
char	*make_bar(long val, long max, long len, int color);
void	put_sect( int y, int x, int nSect, bool upMap );
void	look_at_wild( CHAR_DATA *ch, bool quick );
void	check_wild_move(CHAR_DATA *ch, VEHICLE_DATA *vehicle, SHIP_DATA *ship, FERRY_DATA *ferry, ROOM_DATA *pRoomFrom, ROOM_DATA *pRoomTo);
void	wild_check_for_remove( ROOM_DATA *wRoom );
void	wild_remove_dequeue( ROOM_DATA *wRoom );
int		get_sail_dir( SHIP_DATA *ship, COORD_DATA *cto, bool silent );
int		calc_angle( int chX, int chY, int lmX, int lmY, int *ipDistan );
int		angle_to_dir( int iAngle );
long	event_time(EVENT_DATA *event);
bitvector_t asciiflag_conv(char *flag);


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

/* globals */
SHIP_TYPE		*ship_table[NUM_SHIP_TYPE];
SHIP_DATA		*first_ship			= NULL;
SHIP_DATA		*last_ship			= NULL;
PORT_DATA		*port_table			= NULL;
COURSE_DATA		*course_table		= NULL;
COURSE_DATA		*rev_course_table	= NULL;
FERRY_DATA		*first_ferry		= NULL;
FERRY_DATA		*last_ferry			= NULL;

int				top_ship_num		= 0;
int				top_ship_table		= 0;
int				top_port			= 0;

/* local functions */
EVENTFUNC(ship_sail_event);
PORT_DATA *get_port_by_name( char *name );
PORT_DATA *get_port_by_coord( COORD_DATA *coord );
COURSE_DATA *get_course( COORD_DATA *orig, PORT_DATA *port_dest );
COURSE_DATA *get_course_by_vnum( int vnum );
COURSE_STEP *get_course_step( COURSE_DATA *course, COORD_DATA *coord );
void check_ship_speed( SHIP_DATA *ship );
void load_ship_table( void );
void load_ports( void );
void engage_course( CHAR_DATA *ch, COURSE_DATA *course, bool recover );
bool knows_course( CHAR_DATA *ch, int cnum );
char *get_port_name( COORD_DATA *coord );
/* local ferry functions */
EVENTFUNC(ferry_move_event);
void ferry_move(void);


/* ******************************************************************* */
/* Utilities for retrieving ship data                                  */
/* ******************************************************************* */
SHIP_DATA *find_ship( sh_int vnum )
{
	SHIP_DATA *ship;

	if ( vnum < 0 )
		return (NULL);

	for ( ship = first_ship; ship; ship = ship->next )
		if ( ship->vnum == vnum )
			return (ship);

	return (NULL);
}

SHIP_DATA *find_ship_in_room_by_name( ROOM_DATA *pRoom, char *name )
{
	SHIP_DATA *ship;
	int fnum;

	if ( !pRoom || !pRoom->ships )
		return (NULL);

	if ( !name )
		return (pRoom->ships);

	if ( !( fnum = get_number(&name) ) )
		return (NULL);

	for ( ship = pRoom->ships; ship; ship = ship->next_in_room )
		if ( isname( name, ship->name ) || !str_cmp( name, "ship" ) )
			if ( --fnum == 0 )
				break;

	return (ship);
}

ROOM_DATA *find_room_ship( SHIP_DATA *ship, int room )
{
	if ( room < 1 || room > ship->type->size )
		return (NULL);

	return ( ship->rooms[room] );
}


CHAR_DATA *find_char_in_ship( SHIP_DATA *ship, long idnum )
{
	CHAR_DATA *ppl;

	for ( ppl = ship->people; ppl; ppl = ppl->next_in_vehicle )
		if ( GET_IDNUM(ppl) == idnum )
			break;

	return (ppl);
}

/* ******************************************************************* */
/* Ship Authorization code                                             */
/* ******************************************************************* */
AUTH_DATA *get_ship_auth( SHIP_DATA *ship, long idnum )
{
	AUTH_DATA *pAuth;
	bool aboard = FALSE;

	if ( !ship || !ship->authorized)
		return (NULL);

	for (pAuth = ship->authorized; pAuth; pAuth = pAuth->next)
	{
		if ( pAuth->id_num == idnum )
			break;
	}

	return (pAuth);
}

bool can_go_aboard( CHAR_DATA *ch, SHIP_DATA *ship )
{
	if ( !ch || !ship )
		return (FALSE);

	/* immortals can go aboard */
	if ( IS_IMMORTAL(ch) )
		return (TRUE);

	/* owner of the ship */
	if ( GET_IDNUM(ch) == ship->idowner )
		return (TRUE);

	/* captain of the ship */
	if ( GET_IDNUM(ch) == ship->idcaptain )
		return (TRUE);

	/* ppl authorized to go aboard */
	if ( get_ship_auth( ship, GET_IDNUM(ch) ) )
		return (TRUE);

	return (FALSE);
}

void auth_aboard( SHIP_DATA *ship, CHAR_DATA *ch, CHAR_DATA *vict, int mode )
{
	AUTH_DATA *pAuth;

	if ( !ship || !vict )
		return;

	/* authorize */
	if ( mode == 1 )
	{
		if ( get_ship_auth( ship, GET_IDNUM(vict) ) )
		{
			ch_printf( ch, "%s is already authorized to go aboard.\r\n", PERS(vict, ch) );
			return;
		}

		CREATE(pAuth, AUTH_DATA, 1);
		pAuth->id_num		= GET_IDNUM(vict);
		pAuth->name			= str_dup(GET_NAME(vict));

		pAuth->next			= ship->authorized;
		ship->authorized	= pAuth;

		ch_printf(ch, "%s is now authorized to go aboard of '%s'.\r\n",
			PERS(vict, ch), ship->name);
		ch_printf(vict, "You are now authorized to go aboard of '%s'.\r\n",
			ship->name);
	}
	/* revoke authorization */
	else
	{
		AUTH_DATA *temp;

		if (!(pAuth = get_ship_auth(ship, GET_IDNUM(vict))))
		{
			ch_printf( ch, "%s is already not authorized to go aboard.\r\n", PERS(vict, ch) );
			return;
		}

		if ( find_char_in_ship( ship, GET_IDNUM(vict) ) )
		{
			send_to_char("You cannot revoke authorization to someone that's aboard.\r\n", ch);
			return;
		}

		REMOVE_FROM_LIST(pAuth, ship->authorized, next);
		pAuth->next	= NULL;
		STRFREE(pAuth->name);
		DISPOSE(pAuth);

		ch_printf(ch, "%s now is not authorized to go aboard of '%s'.\r\n",
			PERS(vict, ch), ship->name);
		ch_printf(vict, "Authorization to go aboard of '%s' has been revoked.\r\n",
			ship->name);
	}
}


/* ******************************************************************* */
/* Character/Ship/Room low-level interaction                           */
/* ******************************************************************* */

/* To be called when a pc enter a ship */
void char_to_ship( CHAR_DATA *ch, SHIP_DATA *ship, int room )
{
	int room_num = room;

	if ( room_num < 0 || room_num > ship->type->size)
		room_num = 0;

	ch->in_ship			= ship;

	char_to_room(ch, ship->rooms[room_num]);

	ch->next_in_vehicle	= ship->people;
	ship->people		= ch;

	/* add ch weight to the ship */
	ship->curr_val.loads	+= GET_WEIGHT(ch) + IS_CARRYING_W(ch);
	check_ship_speed( ship );
}

/* To be called when a pc exit a ship */
void char_from_ship( CHAR_DATA *ch )
{
	CHAR_DATA *temp;

	/* subtract ch weight from the ship */
	ch->in_ship->curr_val.loads -= GET_WEIGHT(ch) + IS_CARRYING_W(ch);
	check_ship_speed( ch->in_ship );

	REMOVE_FROM_LIST(ch, ch->in_ship->people, next_in_vehicle);
	ch->in_ship					= NULL;
	ch->next_in_vehicle			= NULL;
	ch->player_specials->athelm	= FALSE;
}

/* remove ship from room */
void ship_from_room( SHIP_DATA *ship )
{
	SHIP_DATA *temp;

	if ( !ship )
	{
		log("SYSERR: NULL character or NULL room in ship_from_room");
		exit(1);
	}

	ship->last_room		= ship->in_room;

	REMOVE_FROM_LIST(ship, ship->in_room->ships, next_in_room);
	ship->in_room		= NULL;
	ship->next_in_room	= NULL;

	wild_check_for_remove(ship->last_room);
}

/* place ship on room */
void ship_to_room( SHIP_DATA *ship, ROOM_DATA *room )
{
	if (room == NULL )
	{
		log("SYSERR: NULL is not a room pointer.  Sorry.");
		return;
	}
	if ( ship == NULL )
	{
		log("SYSERR: NULL is not a ship pointer.  Sorry.");
		return;
	}

	ship->next_in_room	= room->ships;
	room->ships			= ship;
	ship->in_room		= room;

	/* Handle Wild Sectors */
	if ( IS_WILD(room) )
	{
		check_wild_move( NULL, NULL, ship, NULL, ship->last_room, room );

		if (ROOM_FLAGGED(room, ROOM_WILD_REMOVE))
			wild_remove_dequeue(room);
	}
}


/* ******************************************************************* */
/*                      C O M M U N I C A T I O N S                    */
/* ******************************************************************* */
/* Send a message to everyone on the ship */
void send_to_ship( char *msg, SHIP_DATA *ship )
{
	CHAR_DATA *ch;

	if (!ship)
		return;

	for (ch = ship->people; ch; ch = ch->next_in_vehicle)
		send_to_char(msg, ch);
}

/* Send a message to everyone on the ship's deck */
void send_to_deck( char *msg, SHIP_DATA *ship )
{
	CHAR_DATA *ch;

	for ( ch = ship->people; ch; ch = ch->next_in_vehicle )
		if ( ON_DECK(ch) )
			send_to_char( msg, ch );
}

/* ship tell */
int is_stell_ok( CHAR_DATA *vict )
{
	if (!IS_NPC(vict) && !vict->desc)        /* linkless */
		return (FALSE);

	if (PLR_FLAGGED(vict, PLR_WRITING))
		return (FALSE);

	if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF))
		return (FALSE);

	return (TRUE);
}

ACMD(do_stell)
{
	CHAR_DATA *ppl;

	skip_spaces(&argument);
	one_argument( argument, arg );

	if ( !ch->in_ship )
	{
		send_to_char( "You aren't embarked on a ship.\r\n", ch );
		return;
	}

	if (!*arg)
	{
		send_to_char("What do you want to tell to people on ship?\r\n", ch);
		return;
	}

	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
	{
		send_to_char("You can't tell other people while you have notell on.\r\n", ch);
		return;
	}
	
	if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
	{
		send_to_char("The walls seem to absorb your words.\r\n", ch);
		return;
	}

	if (PRF_FLAGGED(ch,PRF_NOREPEAT))
		sprintf(buf, OK);
	else
		sprintf(buf, "&1You tell to everyone on ship, '%s'&0\r\n", arg );
	send_to_char(buf, ch);

	sprintf( buf, "&1$n tells to ship, '%s'&0", arg);

	for ( ppl = ch->in_ship->people; ppl; ppl = ppl->next_in_vehicle )
	{
		if ( ppl == ch )
			continue;

		if ( !is_stell_ok(ppl) )
			continue;

		act(buf, FALSE, ch, 0, ppl, TO_VICT | TO_SLEEP);
	}
}


/* ******************************************************************* */
/*                        I N F O R M A T I O N S                      */
/* ******************************************************************* */

void list_one_ship( SHIP_DATA *ship, CHAR_DATA *ch )
{
	if ( !ship || !ch )
		return;

	ch_printf( ch, "&b&6%s %s named '&7%s&6' is here at anchor.&0\r\n",
		UAN(ship->type->name), ship->type->name, ship->name );
}

void list_ship_to_char( SHIP_DATA *shiplist, CHAR_DATA *ch )
{
	SHIP_DATA *ship;

	if ( !shiplist || !ch || IS_NPC(ch) )
		return;

	for ( ship = shiplist; ship; ship = ship->next_in_room )
		list_one_ship( ship, ch );
}

/* ******************************************************************* */
/*                           C O M M A N D S                           */
/* ******************************************************************* */
ACMD(do_embark)
{
	SHIP_DATA *ship;

	one_argument(argument, arg);

	if ( !*arg )
	{
		send_to_char( "Embark on which ship?\r\n", ch );
		return;
	}

	if ( ch->in_ship )
	{
		send_to_char( "But you are currently embarked on a ship.. disembark first.\r\n", ch );
		return;
	}

	if ( !ch->in_room->ships )
	{
		send_to_char( "No ships here.\r\n", ch );
		return;
	}

	if ( !( ship = find_ship_in_room_by_name( ch->in_room, arg ) ) )
	{
		ch_printf( ch, "No ship named '%s' around.\r\n", arg );
		return;
	}

	if ( RIDING(ch) )
	{
		send_to_char( "No way you can go aboard while riding.\r\n", ch );
		return;
	}

	if ( WAGONER(ch) )
	{
		send_to_char( "No way you can go aboard while driving a vehicle.\r\n", ch );
		return;
	}

	if ( !can_go_aboard( ch, ship ) )
	{
		ch_printf( ch, "You are not authorized to go aboard of '%s'.\r\n",
			ship->name );
		return;
	}

	char_from_room(ch);
	char_to_ship(ch, ship, 0);

	ch_printf( ch, "You go aboard of '%s'.\r\n", ship->name );
}


ACMD(do_disembark)
{
	ROOM_DATA *pRoom;

	if ( !ch->in_ship )
	{
		send_to_char( "But you aren't aboard a ship.\r\n", ch );
		return;
	}

	if ( !SHIP_FLAGGED(ch->in_ship, SHIP_IN_PORT) )
	{
		send_to_char( "You cannot disembark from a ship that's not in a port.\r\n", ch );
		return;
	}

	if ( SECT(ch->in_room) != SECT_SHIP_STERN )
	{
		send_to_char( "You must be astern to disembark.\r\n", ch );
		return;
	}

	if ( !( pRoom = get_wild_room( ch->in_ship->port ) ) )
	{
		log( "SYSERR: no wilderness room as harbour at coord %d %d.",
			ch->in_ship->port->y, ch->in_ship->port->x );
		send_to_char( "Sorry, at the moment you cannot disembark.\r\n", ch );
		return;
	}

	ch_printf( ch, "You disembark from '%s'.\r\n", ch->in_ship->name );

	char_from_ship(ch);
	char_from_room(ch);
	char_to_room(ch, pRoom);
}

ACMD(do_courselist)
{
	COURSE_DATA *course;
	COURSE_STEP *cstep;
	PORT_DATA *port_orig, *port_dest;
	char cbuf[MAX_STRING_LENGTH];

	if ( !course_table )
	{
		send_to_char("No course loaded.\r\n", ch);
		return;
	}

	strcpy( cbuf, "List of Sea Course:\r\n"
		"---------------------------------------------\r\n" );

	for (course = course_table; course; course = course->next)
	{
		port_orig = get_port_by_coord( course->port_orig );
		port_dest = get_port_by_coord( course->port_end );

		sprintf(cbuf + strlen(cbuf),
			"From Port: %s\r\nTo Port  : %s\r\n",
			port_orig->name, port_dest->name );

		for ( cstep = course->first_step; cstep; cstep = cstep->next )
			sprintf(cbuf + strlen(cbuf),
				"  Step: %d %d\r\n",
				cstep->coord.y, cstep->coord.x );
	}

	strcat( cbuf, "---------------------------------------------\r\n" );
	page_string( ch->desc, cbuf, 1 );
}

/*
 * commands for ships:
 *
 * load		- carica merce a bordo
 * unload	- scarica merce
 * drive	- si mette al timone / lascia il timone
 * anchor	- butta l'ancora / recupera l'ancora
 * sail <porto>	- fa rotta verso un porto
 * repair	- mette in cantiere la nave per riparare i danni
 * auth <ch>	- autorizza un giocatore a salire a bordo
 */
ACMD(do_ship)
{
	SHIP_DATA *ship;
	argument = one_argument(argument, arg);

	if (!ch->in_ship)
	{
		send_to_char( "To be aboard a ship would surely help.. he he\r\n", ch );
		return;
	}

	if ( !*arg )
	{
		send_to_char( 
			"Available commands for ships are:\r\n"
			"-------------------------------------------------------------------"
			"<status>            print informations about the ship\r\n"
			"<drive>             grab/let go the helm to drive the ship\r\n"
			"*<charge>           list goods aboard\r\n"
			"*<load 'good'>      load good aboard\r\n"
			"*<unload 'good'>    unload good from ship\r\n"
			"<anchor>            place/remove anchor\r\n"
			"<sail 'port'>       automatic sailing to the given port destination\r\n"
			"<stop>              stops automatic sailing\r\n"
			"<captain 'name'>    nominate 'name' captain of the ship\r\n"
			"<captain>           revoke captaincy\r\n"
			"-------------------------------------------------------------------\r\n"
			"* = Uninmplemented.\r\n"
			, ch );	
		return;
	}

	ship = ch->in_ship;

	if (is_abbrev(arg, "drive"))
	{
		if ( SECT(ch->in_room) != SECT_SHIP_STERN )
		{
			send_to_char("You must be at the stern to drive a ship.\r\n", ch );
			return;
		}

		if ( GET_IDNUM(ch) != ship->idcaptain )
		{
			send_to_char("You must be the captain to drive the ship.\r\n", ch );
			return;
		}

		if ( !GET_SKILL(ch, SKILL_NAVIGATION) )
		{
			send_to_char( "You have no idea of how to drive a ship.\r\n", ch );
			return;
		}

		if ( GET_SKILL(ch, SKILL_NAVIGATION) < ship->type->skill_min )
		{
			send_to_char("You aren't skilled enough to drive a ship of this size.\r\n", ch);
			return;
		}

		/* first of all, check if ship helmperson and ch are the same */
		if ( ship->helmperson && ship->helmperson != ch )
		{
			send_to_char( "Someone else is driving the ship.\r\n", ch );
			return;
		}

		if ( ch->player_specials->athelm )
		{
			ch->player_specials->athelm	= FALSE;
			ship->helmperson			= NULL;
			send_to_char( "You let off the helm.\r\n", ch );
		}
		else
		{
			ch->player_specials->athelm	= TRUE;
			ship->helmperson			= ch;
			send_to_char( "You grab the helm and start to drive the ship.\r\n", ch );
		}
	}
	else if (is_abbrev(arg, "status"))
	{
		char sflags[128];

		sprintbit(ship->flags, ship_flags, sflags);
		sprintf(buf,
			"---------------------------------------------------------------\r\n"
			"Name   : '%s'\r\nType   : %s\r\nFlags  : %s\r\n",
			ship->name, ship->type->name, sflags );
		send_to_char(buf, ch);
		sprintf(buf,
			"---------------------------------------------------------------\r\n"
			"Power  : %s (%5d/%5d)\r\n"
			"Defense: %s (%5d/%5d)\r\n"
			"Health : %s (%5d/%5d)\r\n"
			"Speed  : %s (%5d/%5d)\r\n"
			"Loads  : %s (%5d/%5d)\r\n"
			"---------------------------------------------------------------\r\n",
			make_bar( ship->curr_val.power, ship->max_val.power, 40, 6 ),
			ship->curr_val.power,	ship->max_val.power,
			make_bar( ship->curr_val.defense, ship->max_val.defense, 40, 6 ),
			ship->curr_val.defense, ship->max_val.defense,
			make_bar( ship->curr_val.health, ship->max_val.health, 40, 6 ),
			ship->curr_val.health,	ship->max_val.health,
			make_bar( ship->curr_val.speed, ship->max_val.speed, 40, 6 ),
			ship->curr_val.speed,	ship->max_val.speed,
			make_bar( ship->curr_val.loads, ship->max_val.loads, 40, 6 ),
			ship->curr_val.loads,	ship->max_val.loads
			);
		send_to_char(buf, ch);
		if ( SHIP_FLAGGED(ship, SHIP_IN_COURSE) )
		{
			sprintf(buf, "&b&2In course for %s harbour, heading to the %s.&0\r\n"
				"Next move in %ld ticks.\r\n",
				get_port_name( ship->course->port_end), dirs[ship->direction],
				event_time(ship->action) );
			send_to_char(buf, ch);
		}
	}
	else if (is_abbrev(arg, "sail"))
	{
		COURSE_DATA *course;
		PORT_DATA *port_dest;
		char arg2[MAX_STRING_LENGTH];

		argument = one_argument( argument, arg2 );

		if ( SHIP_FLAGGED(ship, SHIP_AT_ANCHOR) )
		{
			send_to_char("Anchored ships cannot move.\r\n", ch );
			return;
		}

		if ( SHIP_FLAGGED(ship, SHIP_IN_COURSE) )
		{
			send_to_char("You are already following a course, use <ship stop> to stop the ship.\r\n", ch );
			return;
		}

		/* first of all, check if ship helmperson and ch are the same */
		if ( ship->helmperson && ship->helmperson != ch )
		{
			send_to_char( "Someone else is driving the ship.\r\n", ch );
			return;
		}

		if ( !GET_SKILL(ch, SKILL_NAVIGATION) )
		{
			send_to_char( "You have no idea of how set a course.\r\n", ch );
			return;
		}

		if ( GET_SKILL(ch, SKILL_NAVIGATION) < ship->type->skill_min )
		{
			send_to_char("You aren't skilled enough to drive a ship of this size.\r\n", ch);
			return;
		}

		if ( !*arg2 )
		{
			if ( ship->course )
			{
				ch_printf( ch, "You recover the sailing to '%s' harbour.\r\n",
					get_port_name(ship->course->port_end) );
				engage_course( ch, ship->course, TRUE );
				return;
			}
			send_to_char( "Sail to which port?\r\n", ch );
			return;
		}

		if ( !( port_dest = get_port_by_name( arg2 ) ) )
		{
			ch_printf( ch, "No port called %s is known.\r\n", arg2 );
			return;
		}

		if ( SHIP_FLAGGED(ship, SHIP_IN_PORT) )
		{
			if ( ship->port->y == port_dest->coord->y &&
			     ship->port->x == port_dest->coord->x )
			{
				send_to_char( "The shorter history sail. There you are.\r\n", ch );
				return;
			}
		}

		if ( SHIP_FLAGGED(ship, SHIP_IN_PORT) )
			course = get_course( ship->port, port_dest );
		else
		{
			send_to_char( "You can follow sea courses only when leaving a port.\r\n", ch );
			return;
		}

		if ( !course )
		{
			ch_printf( ch, "There is not a direct course to '%s' harbour.\r\n", arg2 );
			return;
		}

		if ( !knows_course( ch, course->vnum ) )
		{

			ch_printf( ch, "You don't know a course to '%s' harbour.\r\n", arg2 );
			return;
		}

		engage_course( ch, course, FALSE );
	}
	else if (!str_cmp(arg, "stop"))
	{
		if ( !SHIP_FLAGGED(ship, SHIP_IN_COURSE))
		{
			send_to_char( "You aren't following a course.\r\n", ch );
			return;
		}

		event_cancel( ship->action );
		ship->action = NULL;
		REMOVE_BIT(SHIP_FLAGS(ship), SHIP_IN_COURSE);
		send_to_char( "You stop follow the sea course.\r\n", ch );
		send_to_ship("The ship stops.\r\n", ship);
	}
	else if (is_abbrev(arg, "captain"))
	{
		CHAR_DATA *vict;
		ROOM_DATA *pRoom;
		char arg2[MAX_STRING_LENGTH];

		argument = one_argument( argument, arg2 );

		if ( !*arg2 )
		{
			if ( ship->idcaptain != -1 )
			{
				ship->idcaptain = -1;
				send_to_char("The ship is now without a captain.\r\n", ch);
			}
			else
				send_to_char( "Nominate captain who?\r\n", ch );
			return;
		}

		if ( ship->idcaptain != -1 )
		{
			send_to_char("The ship already has a captain.\r\n", ch);
			return;
		}

		if ( !SHIP_FLAGGED(ship, SHIP_IN_PORT) || !( pRoom = get_wild_room(ship->port) ) )
		{
			send_to_char("You can nominate only when ship is at one port.\r\n", ch);
			return;
		}

		if ( !( vict = get_char_room_vis( ch, arg2, NULL ) ) )
		{
			ch_printf( ch, "No one called '%s' around.\r\n", arg2 );
			return;
		}

		/* new introduction system */
		if ( !is_char_known(ch, vict) )
		{
			send_to_char("You cannot nominate captain someone you don't know.\r\n", ch );
			return;
		}

		/* assume command! */
		ship->idcaptain = GET_IDNUM(vict);

		act("$N is now the captain of the ship.", FALSE, ch, NULL, vict, TO_CHAR);
		act("$N is now the captain of the ship.", FALSE, ch, NULL, vict, TO_ROOM);
		ch_printf(vict, "You have been nominated captain of '&7%s&0'. Congratulations!!\r\n",
			ship->name);
	}
	// toggle anchor
	else if (is_abbrev(arg, "anchor"))
	{
		if (ship->idcaptain != GET_IDNUM(ch))
		{
			send_to_char("You must be the captain to place/remove the anchor.\r\n", ch );
			return;
		}

		if (SHIP_FLAGGED(ship, SHIP_AT_ANCHOR))
		{
			REMOVE_BIT(SHIP_FLAGS(ship), SHIP_AT_ANCHOR);
			send_to_char("You remove the anchor.\r\n", ch);
		}
		else
		{
			if ( SHIP_FLAGGED(ship, SHIP_IN_COURSE))
			{
				event_cancel( ship->action );
				ship->action = NULL;
				REMOVE_BIT(SHIP_FLAGS(ship), SHIP_IN_COURSE);
				send_to_char( "You stop follow the sea course.\r\n", ch );
				send_to_ship("The ship stops.\r\n", ship);
			}
			SET_BIT(SHIP_FLAGS(ship), SHIP_AT_ANCHOR);
			send_to_char("You place the anchor.\r\n", ch);
		}
	}
	else
		send_to_char( "Not yet implemented, sorry.\r\n", ch );
}


/* ******************************************************************** *
 *                         N A V I G A T I O N							*
 * ******************************************************************** *
 *																		*
 * Legenda of Ship Movement Functions									*
 *																		*
 * check_ship_speed:	calculate ship speed							*
 * ship_can_sail:		check to see if the ship can sail				*
 * engage_course:		inits the automatic sailing						*
 * ship_to_port:		handle the arrival in a port					*
 * ship_move:			called from perform_move() - manual sailing		*
 * create_sail_event:	setup the delayed sail event					*
 * ship_sail_event:		the delayed sail event							*
 *																		*
 * ******************************************************************** */

/*
 * When ch and goods are loaded into ship, they affect ship speed as:
 *
 * - no affection if < of 50% max_val.loads
 * - reduction of 1 pt speed between 51-70% (Min speed is always 1)
 * - reduction of 2 pt speed between 71-80% (Min speed is always 1)
 * - reduction of 3 pt speed between 81-90% (Min speed is always 1)
 * - reduction of 4 pt speed between 91-99% (Min speed is always 1)
 * - 100% speed is always 1
 * - > 100% speed is 0 -- ship can't move
 */
void check_ship_speed( SHIP_DATA *ship )
{
	int plusw, malus, orig;
	char msg[MAX_STRING_LENGTH];

	if ( !ship )
		return;

	orig = ship->curr_val.speed;
	plusw = percentage( ship->curr_val.loads, ship->max_val.loads );

	/* calculate speed penality */
	if		( plusw < 50 )		malus = 0;
	else if ( plusw < 70 )		malus = 1;
	else if ( plusw < 80 )		malus = 2;
	else if ( plusw < 90 )		malus = 3;
	else if ( plusw < 99 )		malus = 4;
	else if ( plusw == 100 )	malus = ship->max_val.speed - 1;
	else	// > 100%
	{
		send_to_ship( "The ship cannot move.. too weight aboard!\r\n", ship );
		ship->curr_val.speed = 0;
		return;
	}

	/* set up current speed */
	ship->curr_val.speed = MAX( 1, ship->max_val.speed - malus );

	if ( orig == ship->curr_val.speed )
		return;

	sprintf(msg, "The ship speed capacity is now %d nodes.\r\n", ship->curr_val.speed );
	send_to_ship( msg, ship );
}


bool ship_can_sail( CHAR_DATA *ch, SHIP_DATA *ship )
{
	/* check speed */
	if ( ship->curr_val.speed <= 0 )
	{
		send_to_char( "The ship cannot sail, too weight aboard.\r\n", ch );
		return (FALSE);
	}

	/* check health status (but only before leaving a port) */
	if ( percentage(ship->curr_val.health, ship->max_val.health) < 10 &&
		SHIP_FLAGGED(ship, SHIP_IN_PORT) )
	{
		send_to_char( "The ship is too damaged to leave the port.\r\n", ch );
		return (FALSE);
	}

	if (SHIP_FLAGGED(ship, SHIP_AT_ANCHOR))
	{
		send_to_char("Anchored ships cannot move.\r\n", ch);
		return (FALSE);
	}

	return (TRUE);
}


void ship_to_port( SHIP_DATA *ship, ROOM_DATA *pRoom )
{
	PORT_DATA *port;
	
	if ( !( port = get_port_by_coord( pRoom->coord ) ) )
	{
		log( "SYSERR: no port found at coord %d %d.", GET_RY(pRoom), GET_RX(pRoom) );
		send_to_ship( "Argh.. there is a problem, call an Immortal.\r\n", ship );
		return;
	}
	
	SET_BIT(SHIP_FLAGS(ship), SHIP_IN_PORT);
	REMOVE_BIT(SHIP_FLAGS(ship), SHIP_SAIL);

	if ( SHIP_FLAGGED(ship, SHIP_IN_COURSE) )
	{
		REMOVE_BIT(SHIP_FLAGS(ship), SHIP_IN_COURSE);
		ship->course = NULL;
		DISPOSE(ship->cstep);
	}

	ship->action = NULL;

	CREATE(ship->port, COORD_DATA, 1);
	ship->port->y = GET_RY(pRoom);
	ship->port->x = GET_RX(pRoom);
	
	sprintf( buf, "%s sails to %s.\r\n", ship->name, port->name );
	send_to_room( buf, ship->in_room );
	
	ship_from_room( ship );
	ship_to_room( ship, pRoom );
	
	sprintf( buf, "%s sails from the %s.\r\n", ship->name, dirs[rev_dir[ship->direction]] );
	send_to_room( buf, ship->in_room );
	
	sprintf( buf, "You enter %s harbour.\r\n", port->name );
	send_to_ship( buf, ship );
}

void create_sail_event( SHIP_DATA *ship, sh_int type )
{
	SAIL_EVENT *sail;
	int delay;

	if ( !ship )
		return;

	if ( ship->curr_val.speed <= 0 )
		return;

	if ( ship->action )
		return;

	CREATE( sail, SAIL_EVENT, 1 );
	sail->ship	= ship;
	sail->type	= type;

	delay = SHIP_SPEED_BASE - ship->curr_val.speed;

	ship->action = event_create(ship_sail_event, sail, delay);
}

void engage_course( CHAR_DATA *ch, COURSE_DATA *course, bool recover )
{
	SHIP_DATA *ship;
	EXIT_DATA *pexit;
	int dir;

	if ( !ch || !course )
		return;

	ship = ch->in_ship;

	if ( ship->action )
	{
		send_to_char( "You can't sail at the moment.\r\n", ch );
		return;
	}

	if ( !ship_can_sail( ch, ship ) )
		return;

	if ( !recover )
	{
		ship->course	= course;
		ship->cstep		= course->first_step;
	}

	dir = get_sail_dir( ship, &ship->cstep->coord, FALSE );

	if ( !( pexit = get_exit( ship->in_room, dir ) ) )
	{
		send_to_char("Alas, you cannot go that way...\r\n", ch);
		ship->course	= NULL;
		ship->cstep		= NULL;
		return;
	}

	// here.. if ship recover course immediately out of a port, we must handle..
	/* the ship enter a port */
	if ( SECT(pexit->to_room) == SECT_PORT )
	{
		ship_to_port( ship, pexit->to_room );
		return;		
	}

	if ( !water_sector(SECT(pexit->to_room)) )
	{
		send_to_char("You can't go on landmasses with a ship.\r\n", ch);
		ship->course	= NULL;
		ship->cstep		= NULL;
		return;
	}

	SET_BIT(SHIP_FLAGS(ship), SHIP_IN_COURSE);

	/* first move out of the port */
	if ( SECT(ship->in_room) == SECT_PORT )
	{
		REMOVE_BIT(SHIP_FLAGS(ship), SHIP_IN_PORT);
		SET_BIT(SHIP_FLAGS(ship), SHIP_SAIL);
		DISPOSE(ship->port);
	}

	/*
	dir = get_sail_dir( ship, &ship->cstep->coord, FALSE );

	if ( !( pexit = get_exit( ship->in_room, dir ) ) )
	{
		send_to_char("Alas, you cannot go that way...\r\n", ch);
		return;
	}

	if ( !water_sector(SECT(pexit->to_room)) )
	{
		send_to_char("You can't go on landmasses with a ship.\r\n", ch);
		return;
	}
	*/
	
	/* save the direction for automatic sailing */
	ship->direction = dir;

	create_sail_event(ship, SAIL_COURSE);
}


void ship_move( CHAR_DATA *ch, int dir, int special_check )
{
	SHIP_DATA *ship;
	EXIT_DATA *pexit;

	if ( !ch || !ch->in_ship || !ch->player_specials->athelm )
		return;

	ship = ch->in_ship;

	if ( dir < 0 || dir >= NUM_OF_DIRS )
	{
		send_to_char("Invalid direction.\r\n", ch );
		return; 
	}

	if ( SHIP_FLAGGED(ship, SHIP_AT_ANCHOR) )
	{
		send_to_char("Anchored ships cannot move.\r\n", ch );
		return;
	}

	/* hmmm.. to check again later */
	if ( ship->action )
	{
		if ( SHIP_FLAGGED(ship, SHIP_IN_COURSE ) )
		{
			event_cancel( ship->action );
			ship->action = NULL;
			REMOVE_BIT(SHIP_FLAGS(ship), SHIP_IN_COURSE);
			send_to_char( "You stop the course sailing.\r\n", ch );
		}
		else
		{
			send_to_char("The ship is currently performing another action.\r\n", ch );
			return;
		}
	}

	if ( !ship_can_sail( ch, ship ) )
		return;

	if ( !( pexit = get_exit( ship->in_room, dir ) ) )
	{
		send_to_char("Alas, you cannot go that way...\r\n", ch);
		return;
	}

	/* the ship enter a port */
	if ( SECT(pexit->to_room) == SECT_PORT )
	{
		ship_to_port( ship, pexit->to_room );
		return;		
	}
	
	if ( !water_sector(SECT(pexit->to_room)) )
	{
		send_to_char("You can't go on landmasses with a ship.\r\n", ch);
		return;
	}
	
	/* first move out of the port */
	if ( SECT(ship->in_room) == SECT_PORT )
	{
		REMOVE_BIT(SHIP_FLAGS(ship), SHIP_IN_PORT);
		SET_BIT(SHIP_FLAGS(ship), SHIP_SAIL);
		DISPOSE(ship->port);
	}

	/* save the direction for automatic sailing */
	ship->direction = dir;

	create_sail_event(ship, SAIL_MANUAL);
}


/* Ship movement.. */
EVENTFUNC(ship_sail_event)
{
	SAIL_EVENT *sail = (SAIL_EVENT *) (event_obj);
	SHIP_DATA *ship;
	EXIT_DATA *pexit;
	CHAR_DATA *ppl;

	ship = sail->ship;

	if ( !ship || !ship->action)
	{
		DISPOSE(event_obj);
		return (0);
	}

	if ( sail->type == SAIL_MANUAL )
		ship->action	= NULL;		/* clear ship action */

	// check for speed
	if ( ship->curr_val.speed <= 0 )
	{
		DISPOSE(event_obj);
		return (0);
	}

	// check for exit
	if ( !( pexit = get_exit( ship->in_room, ship->direction ) ) )
	{
		log("SYSERR: sail_event() -- invalid exit to %d for ship %s.",
			ship->direction, ship->name );
		DISPOSE(event_obj);
		return (0);
	}

	/* the ship enter a port */
	if ( SECT(pexit->to_room) == SECT_PORT )
	{
		ship_to_port( ship, pexit->to_room );
		DISPOSE(event_obj);
		return (0);
	}

	// check for water
	if (!water_sector(SECT(pexit->to_room)))
	{
		log("SYSERR: sail_event() -- not sea exit to %s for ship %s.",
			dirs[ship->direction], ship->name );
		DISPOSE(event_obj);
		return (0);
	}

	sprintf( buf, "%s sails to the %s.\r\n", ship->name, dirs[ship->direction] );
	send_to_room( buf, ship->in_room );

	ship_from_room( ship );
	ship_to_room( ship, pexit->to_room );

	sprintf( buf, "%s sails from the %s.\r\n", ship->name, dirs[rev_dir[ship->direction]] );
	send_to_room( buf, ship->in_room );

	for ( ppl = ship->people; ppl; ppl = ppl->next_in_vehicle )
	{
		if ( ON_DECK(ppl) && !FIGHTING(ppl) )
			look_at_wild(ppl, TRUE);
	}

	if ( sail->type == SAIL_COURSE )
	{
		int dir;
		
		/* have we reached the step? */
		if ( ship->in_room->coord->y == ship->cstep->coord.y &&
		     ship->in_room->coord->x == ship->cstep->coord.x )
		{
			/* check for last step */
			if ( !ship->cstep->next )
			{
				COURSE_STEP *csnew;

				CREATE(csnew, COURSE_STEP, 1);

				csnew->next		= NULL;
				csnew->prev		= NULL;
				csnew->coord.y	= ship->course->port_end->y;
				csnew->coord.x	= ship->course->port_end->x;

				/* next step will be the port itself */
				ship->cstep	= csnew;
			}
			else
				ship->cstep	= ship->cstep->next;
		}
		
		dir = get_sail_dir( ship, &ship->cstep->coord, FALSE );

		if ( dir == NOWHERE )
		{
			log("SYSERR: ship_sail_event() - cannot get a direction for next step %d %d.",
				ship->cstep->coord.y, ship->cstep->coord.x);
			REMOVE_BIT(SHIP_FLAGS(ship), SHIP_IN_COURSE);
			REMOVE_BIT(SHIP_FLAGS(ship), SHIP_SAIL);
			DISPOSE(event_obj);
			return (0);
		}

		ship->direction = dir;

		return (SHIP_SPEED_BASE - ship->curr_val.speed);
	}
	else
	{
		DISPOSE(event_obj);
		return (0);
	}
}

/* ******************************************************************* */
/* Ship making                                                         */
/* ******************************************************************* */

void free_ship( SHIP_DATA *ship )
{
	SHIP_DATA *temp;

	/* cut off from the list, if needed */
	if ( ship->next )
		UNLINK(ship, first_ship, last_ship, next, prev);

	if ( ship->next_in_room && ship->in_room )
		REMOVE_FROM_LIST( ship, ship->in_room->ships, next_in_room );

	if ( ship->people )
	{
		/* transfer people to limbo */
		CHAR_DATA *ppl;

		while (ship->people)
		{
			ppl = ship->people;
			char_from_ship(ppl);
			char_from_room(ppl);
			char_to_room(ppl, get_room(1));
		}
	}

	if ( ship->rooms[0] )
	{
	}

	ship->action		= NULL;
	ship->helmperson	= NULL;
	ship->in_room		= NULL;
	ship->last_room		= NULL;
	ship->next			= NULL;
	ship->next_in_room	= NULL;
	ship->people		= NULL;
	ship->storeroom		= NULL;

	if ( ship->name )
		free(ship->name );
	if ( ship->port )
		free(ship->port );

	ship->authorized	= NULL;
	ship->type			= NULL;

	DISPOSE(ship);
}


SHIP_DATA *new_ship( void )
{
	SHIP_DATA *ship;

	CREATE( ship, SHIP_DATA, 1 );
	ship->name			= NULL;
	ship->in_room		= NULL;
	ship->last_room		= NULL;
	ship->people		= NULL;
	ship->next			= NULL;
	ship->prev			= NULL;
	ship->next_in_room	= NULL;
	ship->action		= NULL;
	ship->storeroom		= NULL;
	ship->helmperson	= NULL;
	ship->port			= NULL;
	ship->type			= NULL;
	ship->authorized	= NULL;
	ship->course		= NULL;
	ship->cstep			= NULL;
	ship->flags			= 0;
	ship->direction		= NOWHERE;
	ship->idowner		= 0;
	ship->idcaptain		= 0;
	ship->vnum			= NOTHING;

	return (ship);
}


/* make the ship inside */
bool create_ship_rooms(SHIP_DATA *ship)
{
	FILE *fp;
	EXTRA_DESCR *new_descr;
	char fname[128];
	char letter, *flags;
	int room, door;

	sprintf(fname, "%s%d.ship", SHIP_PREFIX, ship->type->vnum);
	if (!(fp = fopen(fname, "r")))
	{
		log("SYSERR: Unable to open ship template file %s.", fname);
		return (FALSE);
	}

	CREATE(ship->rooms, ROOM_DATA *, ship->type->size);
	for (room = 0; room < ship->type->size; room++)
	{
		CREATE(ship->rooms[room], ROOM_DATA, 1);
		ship->rooms[room]->number = room;
		ship->rooms[room]->zone = SHIP_ZONE;

		CREATE(ship->rooms[room]->extra_data, ROOM_EXTRA, 1);
		ship->rooms[room]->extra_data->vnum = ship->vnum;
	}

	for ( ;; )
	{
		letter = fread_letter(fp);

		if (feof(fp))
			break;

		if (letter == '$')
			break;

		if (letter == '*')
		{
			fread_to_eol(fp);
			continue;
		}
		
		if (letter != '#')
		{
			log("create_ship_rooms(): # not found.");
			break;
		}
		
		room = fread_number(fp);

		if (room < 0 || room > ship->type->size)
			break;

		ship->rooms[room]->number		= room;
		ship->rooms[room]->name			= fread_string_nospace(fp);
		ship->rooms[room]->description	= fread_string_nospace(fp);
		flags							= fread_word(fp);
		ship->rooms[room]->room_flags	= asciiflag_conv(flags);
		ship->rooms[room]->sector_type	= fread_number(fp);

		if (ship->rooms[room]->sector_type == SECT_SHIP_HOLD)
			ship->storeroom = ship->rooms[room];

		for ( ;; )
		{
			letter = fread_letter(fp);
			if (letter == '*')
			{
				fread_to_eol(fp);
				continue;
			}
			if (letter == 'S' || letter == '$')
				break;

			switch (letter)
			{
			case 'D':
				door = fread_number(fp);
				if ( door < 0 || door > NUM_OF_DIRS )
				{
					log( "create_ship_rooms: vnum %d has bad door number %d.", room, door );
					break;
				}
				else
				{
					EXIT_DATA *pexit;
					char flags[128];
					char *ln;
					int x1, x2;
					
					pexit = make_exit(ship->rooms[room], NULL, door);
					pexit->description	= fread_string_nospace(fp);
					pexit->keyword		= fread_string_nospace(fp);
					ln					= fread_line(fp);
					x1=x2=0;
					sscanf( ln, "%s %d %d ", &flags, &x1, &x2 );
					
					pexit->key			= x1;
					pexit->vnum			= x2;
					pexit->vdir			= door;
					pexit->exit_info	= asciiflag_conv(flags);
					
					pexit->to_room		= ship->rooms[pexit->vnum];

					/* sanity check */
					if (pexit->exit_info && !EXIT_FLAGGED(pexit, EX_ISDOOR))
						SET_BIT(pexit->exit_info, EX_ISDOOR);
				}
				break;

			case 'E':
				CREATE(new_descr, EXTRA_DESCR, 1);
				new_descr->keyword		= fread_string(fp, buf2);
				new_descr->description	= fread_string(fp, buf2);
				new_descr->next			= ship->rooms[room]->ex_description;
				ship->rooms[room]->ex_description = new_descr;
				break;
			}
		}

		if (letter == '$')
			break;
	}

	fclose(fp);
	return (TRUE);
}


SHIP_DATA *create_ship( CHAR_DATA *ch, SHIP_TYPE *tShip, PORT_DATA *port, char *arg)
{
	SHIP_DATA *ship;
	int hp;

	if ( !tShip || !port )
	{
		log( "SYSERR: Create_ship() - no proto or port data" );
		return (NULL);
	}

	ship = new_ship();

	ship->name				= ( *arg ? str_dup(arg) : str_dup(tShip->name) );
	ship->idowner			= GET_IDNUM(ch);
	ship->idcaptain			= GET_IDNUM(ch);
	/* hmmmmm... */
	ship->vnum				= ++top_ship_num;

	ship->type				= tShip;

	CREATE( ship->port, COORD_DATA, 1 );
	ship->port->y			= port->coord->y;
	ship->port->x			= port->coord->x;

	hp = tShip->value.health * tShip->size;
	ship->curr_val.health	= ship->max_val.health	= hp;

	ship->curr_val.defense	= ship->max_val.defense	= tShip->value.defense;
	ship->curr_val.equip	= ship->max_val.equip	= tShip->value.equip;
	ship->curr_val.power	= ship->max_val.power	= tShip->value.power;
	ship->curr_val.speed	= ship->max_val.speed	= tShip->value.speed;
	ship->max_val.loads		= tShip->value.loads;
	ship->curr_val.loads	= 0;

	create_ship_rooms( ship );

	SET_BIT(SHIP_FLAGS(ship), SHIP_IN_PORT);

	/* add to the list */
	LINK(ship, first_ship, last_ship, next, prev);

	return (ship);
}

/* Imms command */
ACMD(do_shipsetup)
{
	SHIP_DATA *ship;
	COORD_DATA *coord;
	ROOM_DATA *pRoom;
	PORT_DATA *port;
	char arg1[MAX_STRING_LENGTH], arg2[MAX_STRING_LENGTH];
	char *shipname;
	int sh_type = 0;

	argument = one_argument( argument, arg );
	argument = two_arguments( argument, arg1, arg2 );

	shipname = strtok(argument, "'");
	
	if (shipname == NULL)
	{
		send_to_char("Which name?\r\n", ch);
		return;
	}
	
	shipname = strtok(NULL, "'");
	if (shipname == NULL)
	{
		send_to_char("Ship names must be enclosed in the Holy Magic Symbols: '\r\n", ch);
		return;
	}

	if ( !*arg || !*arg1 || !*arg2 || !*shipname )
	{
		send_to_char(
			"Usage: shipsetup <type> <y> <x> 'name'\r\n"
			" <type>   = type of ship\r\n"
			" <y> <x>  = port coordinates\r\n"
			" 'name'   = name of the ship\r\n",
			ch );
		return;
	}

	sh_type = atoi(arg);

	if ( sh_type < 0 || sh_type > top_ship_table - 1 )
	{
		ch_printf( ch, "Valid ship types goes from 0 to %d.\r\n", top_ship_table );
		return;
	}

	CREATE(coord, COORD_DATA, 1);
	coord->y = atoi(arg1);
	coord->x = atoi(arg2);

	if ( !check_coord( coord->y, coord->x ) )
		return;

	if ( !( port = get_port_by_coord( coord ) ) )
	{
		ch_printf( ch, "No port found at coord %d %d.\r\n", coord->y, coord->x );
		return;
	}

	pRoom = get_wild_room( coord );
	if ( !pRoom || SECT(pRoom) != SECT_PORT )
	{
		send_to_char("Invalid coordinates.\r\n", ch );
		return;
	}

	ship = create_ship( ch, ship_table[sh_type], port, shipname );
	ship_to_room( ship, pRoom );
}

/* ******************************************************************* */
/* Ship Files handling                                                 */
/* ******************************************************************* */

/* write on file the building inside */
void fwrite_ship_rooms(SHIP_DATA *ship, FILE *fp)
{
	ROOM_DATA *room;
	EXIT_DATA *pexit;
	char dflags[128];
	int num;

	for (num = 0; num < ship->type->size; num++)
	{
		room = ship->rooms[num];

		if (!room) break;

		/* Copy the description and strip off trailing newlines */
		strcpy(buf, room->description ? room->description : "Empty room.");
		strip_cr(buf);

		sprintascii(buf2, room->room_flags);

		fprintf(fp,
			"#%d\n"
			"%s~\n"
			"%s~\n"
			"%s %d\n",
			room->number,
			room->name ? room->name : "Untitled",
			buf, buf2, room->sector_type
			);

		/* Now you write out the exits for the room */
		for (pexit = room->first_exit; pexit; pexit = pexit->next)
		{
			/* check for non-building exits */
			if (!IS_SHIP(pexit->to_room))
				continue;
			
			if (pexit->description)
			{
				strcpy(buf, pexit->description);
				strip_cr(buf);
			}
			else
				*buf = '\0';
			
			if (pexit->keyword)
				strcpy(buf1, pexit->keyword);
			else
				*buf1 = '\0';
			
			/* New door flags handling -- Fab 2000 */
			REMOVE_BIT(EXIT_FLAGS(pexit), EX_LOCKED);
			REMOVE_BIT(EXIT_FLAGS(pexit), EX_CLOSED);
			REMOVE_BIT(EXIT_FLAGS(pexit), EX_HIDDEN);

			/* sanity check */
			if (EXIT_FLAGS(pexit) && !EXIT_FLAGGED(pexit, EX_ISDOOR))
				SET_BIT(EXIT_FLAGS(pexit), EX_ISDOOR);
			
			sprintascii(dflags, pexit->exit_info);
			
			fprintf(fp,
				"D%d\n"
				"%s~\n"
				"%s~\n"
				"%s %d %d\n", pexit->vdir, buf, buf1, dflags,
				pexit->key, ( pexit->to_room != NULL ? pexit->to_room->number : NOWHERE ) );
		}

		if (room->ex_description)
		{
			EXTRA_DESCR *xdesc;
			
			for (xdesc = room->ex_description; xdesc; xdesc = xdesc->next)
			{
				strcpy(buf, xdesc->description);
				strip_cr(buf);
				fprintf(fp,
					"E\n"
					"%s~\n"
					"%s~\n", xdesc->keyword, buf);
			}
		}
		
		fprintf(fp, "S\n");
	}

	/* Write the final line */
	fprintf(fp, "$\n\n");
}

void fwrite_ship( SHIP_DATA *ship )
{
	FILE *fp;
	char fname[128];
	int num;

	sprintf(fname, "%s%d.ship", LIB_SHIPS, ship->vnum);
	if ( !(fp = fopen( fname, "w" ) ) )
	{
		log( "Cannot open ship file %s", fname );
		return;
	}

	fprintf( fp, "#SHIP\n" );
	fprintf( fp, "Vnum           %d\n",	ship->vnum );
	fprintf( fp, "Name           %s~\n",	ship->name );
	fprintf( fp, "Type           %s~\n",	ship->type->name );
	fprintf( fp, "Flags          %d\n",	ship->flags );

	fprintf( fp, "Val_Defense    %hd %hd\n",	ship->curr_val.defense, ship->max_val.defense );
	fprintf( fp, "Val_Equip      %hd %hd\n",	ship->curr_val.equip, ship->max_val.equip );
	fprintf( fp, "Val_Health     %hd %hd\n",	ship->curr_val.health, ship->max_val.health );
	fprintf( fp, "Val_Loads      %hd %hd\n",	ship->curr_val.loads, ship->max_val.loads );
	fprintf( fp, "Val_Power      %hd %hd\n",	ship->curr_val.power, ship->max_val.power );
	fprintf( fp, "Val_Speed      %hd %hd\n",	ship->curr_val.speed, ship->max_val.speed );

	fprintf( fp, "IdOwner        %ld\n",	ship->idowner );
	fprintf( fp, "IdCaptain      %ld\n",	ship->idcaptain );
	if ( ship->authorized )
	{
		AUTH_DATA *pAuth;

		for (pAuth = ship->authorized; pAuth; pAuth = pAuth->next)
			fprintf(fp, "IdAuth         %ld %s\n", pAuth->id_num, pAuth->name);
	}

	if ( SHIP_FLAGGED(ship, SHIP_IN_PORT) )
		fprintf( fp, "Port           %hd %hd\n", ship->port->y, ship->port->x );
	else if ( SHIP_FLAGGED(ship, SHIP_AT_ANCHOR) )
		fprintf( fp, "Anchored       %hd %hd\n", GET_Y(ship), GET_X(ship) );
	else
		fprintf( fp, "Sailing        %hd %hd\n", GET_Y(ship), GET_X(ship) );

	if ( SHIP_FLAGGED(ship, SHIP_IN_COURSE) )
	{
		fprintf( fp, "Course         %d\r\n", ship->course );
		fprintf( fp, "CStep          %hd %hd\r\n", ship->cstep->coord.y, ship->cstep->coord.x );
	}

	fprintf( fp, "End\n\n" );

	fprintf(fp, "#ROOMS\n");
	
	fwrite_ship_rooms(ship, fp);
		
	for (num = 0; num < ship->type->size; num++)
	{
		if (ship->rooms[num] && ship->rooms[num]->first_content)
		{
			fprintf(fp, "#ROOMCONTENT\n");
			fprintf(fp, "Room          %d\n", num);
			save_objs( ship->rooms[num]->last_content, fp, 0, 0);
			fprintf(fp, "End\n\n");
		}
	}

	fprintf( fp, "#END\n" );
	fclose(fp);
}


void SaveShips( void )
{
	FILE *fp;
	SHIP_DATA *ship;
	char fname[128];

	if ( !first_ship )
		return;

	sprintf( fname, "%sindex", LIB_SHIPS );
	if ( !( fp = fopen( fname, "w" ) ) )
	{
		log( "SYSERR: unable to write index file for ships." );
		return;
	}

	for ( ship = first_ship; ship; ship = ship->next )
	{
		if ( ship->vnum == NOTHING )
			continue;

		fprintf( fp, "%d.ship\n", ship->vnum );
		fwrite_ship( ship );
	}

	fprintf( fp, "$\n" );
	fclose(fp);
}


/* READING */

SHIP_TYPE *parse_ship_type( char *tname )
{
	int y;

	for ( y = 0; y < top_ship_table; y++ )
	{
		if ( !ship_table[y] || !ship_table[y]->name )
			continue;

		if ( !str_cmp( tname, ship_table[y]->name ) )
			return ( ship_table[y] );
	}

	return (NULL);
}


bool fread_one_ship( SHIP_DATA *ship, FILE *fp )
{
	char *word;
	bool fMatch;

	for ( ; ; )
	{
		word   = feof( fp ) ? "End" : fread_word( fp );
		fMatch = FALSE;
		
		switch ( UPPER(word[0]) )
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol( fp );
			break;

		case 'A':
			if ( !str_cmp( word, "Anchored" ) )
			{
				ROOM_DATA *pRoom;

				CREATE( ship->port, COORD_DATA, 1 );
				ship->port->y	= fread_number(fp);
				ship->port->x	= fread_number(fp);

				pRoom = create_wild_room( ship->port, FALSE );

				fMatch = TRUE;
				break;
			}
			break;

		case 'C':
			if ( !str_cmp( word, "Course" ) )
			{
				COURSE_DATA *course;
				int vnum = fread_number(fp);

				course = get_course_by_vnum( vnum );

				ship->course = course;
				/* should set on the course flag?? hmmmm */
				SET_BIT(SHIP_FLAGS(ship), SHIP_IN_COURSE);

				fMatch = TRUE;
				break;
			}
			if ( !str_cmp( word, "CStep" ) )
			{
				COORD_DATA coord;

				coord.y = fread_number(fp);
				coord.x = fread_number(fp);

				if ( ship->course )
					ship->cstep = get_course_step( ship->course, &coord );

				fMatch = TRUE;
				break;
			}
			break;

		case 'E':
			if ( !str_cmp( word, "End" ) )
			{
				if ( ship->vnum == NOTHING || !ship->type )
				{
					if ( ship->port )
						DISPOSE(ship->port);
					DISPOSE(ship);
					return (FALSE);
				}

				create_ship_rooms( ship );
				return (TRUE);
			}
			break;


		case 'F':
			KEY( "Flags",		ship->flags,			fread_number(fp) );
			break;

		case 'I':
			KEY( "IdOwner",		ship->idowner,			fread_number(fp) );
			KEY( "IdCaptain",	ship->idcaptain,		fread_number(fp) );
			if ( !str_cmp(word, "IdAuth"))
			{
				AUTH_DATA *pAuth;

				CREATE(pAuth, AUTH_DATA, 1);
				pAuth->id_num		= fread_number(fp);
				pAuth->name			= str_dup(fread_word(fp));

				// add to the list
				pAuth->next		= ship->authorized;
				ship->authorized	= pAuth;
				
				fMatch = TRUE;
				break;
			}
			break;

		case 'N':
			KEY( "Name",		ship->name,				fread_string_nospace(fp) );
			break;

		case 'P':
			if ( !str_cmp( word, "Port" ) )
			{
				CREATE( ship->port, COORD_DATA, 1 );
				ship->port->y	= fread_number(fp);
				ship->port->x	= fread_number(fp);

				fMatch = TRUE;
				break;
			}
			break;

		case 'S':
			if ( !str_cmp( word, "Sailing" ) )
			{
				ROOM_DATA *pRoom;

				CREATE( ship->port, COORD_DATA, 1 );
				ship->port->y	= fread_number(fp);
				ship->port->x	= fread_number(fp);

				pRoom = create_wild_room( ship->port, FALSE );

				fMatch = TRUE;
				break;
			}
			break;

		case 'T':
			if ( !str_cmp( word, "Type" ) )
			{
				ship->type = parse_ship_type( fread_string_nospace(fp) );
				fMatch = TRUE;
				break;
			}
			break;

		case 'V':
			KEY( "Vnum",		ship->vnum,				fread_number(fp) );
			if ( !str_cmp( word, "Val_Defense" ) )
			{
				ship->curr_val.defense	= fread_number(fp);
				ship->max_val.defense	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if ( !str_cmp( word, "Val_Equip" ) )
			{
				ship->curr_val.equip	= fread_number(fp);
				ship->max_val.equip	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if ( !str_cmp( word, "Val_Health" ) )
			{
				ship->curr_val.health	= fread_number(fp);
				ship->max_val.health	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if ( !str_cmp( word, "Val_Loads" ) )
			{
				ship->curr_val.loads	= fread_number(fp);
				ship->max_val.loads	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if ( !str_cmp( word, "Val_Power" ) )
			{
				ship->curr_val.power	= fread_number(fp);
				ship->max_val.power	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if ( !str_cmp( word, "Val_Speed" ) )
			{
				ship->curr_val.speed	= fread_number(fp);
				ship->max_val.speed	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;
		}

		if ( !fMatch )
			log( "fread_ship_type: no match: %s", word );
	}

	return (FALSE);
}

#define MAX_BAG_ROWS	5

/* load contents of a ship room from file */
void fread_ship_contents( SHIP_DATA *ship, FILE *fp )
{
	OBJ_DATA *obj, *obj2, *cont_row[MAX_BAG_ROWS];
	int location, j, roomnum;
	char *word;
	bool fMatch;

	/* Empty all of the container lists (you never know ...) */
	for (j = 0; j < MAX_BAG_ROWS; j++)
		cont_row[j] = NULL;

	for ( ; ; )
	{
		word   = feof( fp ) ? "End" : fread_word( fp );
		fMatch = FALSE;
		
		switch ( UPPER(word[0]) )
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol( fp );
			break;

		case 'E':
			if ( !str_cmp( word, "End" ) )
				return;

		case 'R':
			KEY( "Room",		roomnum,				fread_number(fp) );
			break;

		case '#':
			// skip #OBJECT line..
			fread_to_eol( fp );
			
			if ( ( obj = fread_one_obj( fp, &location ) ) == NULL )
				continue;
			
			obj_to_room( obj, ship->rooms[roomnum] );
			
			for (j = MAX_BAG_ROWS - 1; j > -location; j--)
			{
				if (cont_row[j])	/* No container, back to inventory. */
				{
					for (; cont_row[j]; cont_row[j] = obj2)
					{
						obj2 = cont_row[j]->next_content;
						obj_to_room( cont_row[j], ship->rooms[roomnum] );
					}
					cont_row[j] = NULL;
				}
			}
			if (j == -location && cont_row[j])	/* Content list exists. */
			{
				if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER ||
					GET_OBJ_TYPE(obj) == ITEM_MISSILE_CONT )
				{
					/* Take the item, fill it, and give it back. */
					obj_from_room(obj);
					obj->first_content = NULL;
					obj->last_content = NULL;
					for (; cont_row[j]; cont_row[j] = obj2)
					{
						obj2 = cont_row[j]->next_content;
						obj_to_obj(cont_row[j], obj);
					}
					obj_to_room( obj, ship->rooms[roomnum] );
				}
				else	/* Object isn't container, empty content list. */
				{
					for (; cont_row[j]; cont_row[j] = obj2)
					{
						obj2 = cont_row[j]->next_content;
						obj_to_room( cont_row[j], ship->rooms[roomnum] );
					}
					cont_row[j] = NULL;
				}
			}
			if (location < 0 && location >= -MAX_BAG_ROWS)
			{
				obj_from_room(obj);
				if ((obj2 = cont_row[-location - 1]) != NULL)
				{
					while (obj2->next_content)
						obj2 = obj2->next_content;
					obj2->next_content = obj;
				}
				else
					cont_row[-location - 1] = obj;
			}
			break;
		}
	}
}

/* load from file the building inside */
void fread_ship_rooms(SHIP_DATA *ship, FILE *fp)
{
	EXTRA_DESCR *new_descr;
	char letter, *flags;
	int room, door;

	CREATE(ship->rooms, ROOM_DATA *, ship->type->size);

	for (room = 0; room < ship->type->size; room++)
	{
		CREATE(ship->rooms[room], ROOM_DATA, 1);
		ship->rooms[room]->zone = SHIP_ZONE;

		CREATE(ship->rooms[room]->extra_data, ROOM_EXTRA, 1);
		ship->rooms[room]->extra_data->vnum = ship->vnum;
	}

	for ( ;; )
	{
		letter = fread_letter(fp);

		if (feof(fp))
			break;

		if (letter == '$')
			break;

		if (letter == '*')
		{
			fread_to_eol(fp);
			continue;
		}
		
		if (letter != '#')
		{
			log("fread_ship_rooms(): # not found.");
			break;
		}
		
		room = fread_number(fp);

		if (room < 0 || room > ship->type->size)
			break;

		ship->rooms[room]->number		= room;

		ship->rooms[room]->name			= fread_string_nospace(fp);
		ship->rooms[room]->description	= fread_string_nospace(fp);
		flags							= fread_word(fp);
		ship->rooms[room]->room_flags	= asciiflag_conv(flags);
		ship->rooms[room]->sector_type	= fread_number(fp);

		for (;;)
		{
			letter = fread_letter(fp);
			if (letter == '*')
			{
				fread_to_eol(fp);
				continue;
			}
			if (letter == 'S' || letter == '$')
				break;

			switch (letter)
			{
			case 'D':
				door = fread_number(fp);
				if ( door < 0 || door > NUM_OF_DIRS )
				{
					log("fread_ship_rooms: vnum %d has bad door number %d.", room, door);
					break;
				}
				else
				{
					EXIT_DATA *pexit;
					char flags[128];
					char *ln;
					int x1, x2;
					
					pexit = make_exit( ship->rooms[room], NULL, door );
					pexit->description	= fread_string_nospace(fp);
					pexit->keyword		= fread_string_nospace(fp);
					ln					= fread_line(fp);
					x1=x2=0;
					sscanf( ln, "%s %d %d ", &flags, &x1, &x2 );
					
					pexit->key			= x1;
					pexit->vnum			= x2;
					pexit->vdir			= door;
					pexit->exit_info	= asciiflag_conv( flags );
					
					pexit->to_room		= ship->rooms[pexit->vnum];

					/* sanity check */
					if ( pexit->exit_info && !EXIT_FLAGGED( pexit, EX_ISDOOR ) )
						SET_BIT( pexit->exit_info, EX_ISDOOR );
				}
				break;

			case 'E':
				CREATE(new_descr, EXTRA_DESCR, 1);
				new_descr->keyword		= fread_string(fp, buf2);
				new_descr->description	= fread_string(fp, buf2);
				new_descr->next			= ship->rooms[room]->ex_description;
				ship->rooms[room]->ex_description = new_descr;
				break;
			}
		}

		if (letter == '$')
			break;
	}
}

SHIP_DATA *fread_ship( char *sname ) 
{
	FILE *fp;
	SHIP_DATA *ship;
	char fname[128];
	char letter;
	char *word;

	sprintf( fname, "%s%s", LIB_SHIPS, sname );
	if ( !( fp = fopen(fname, "r") ) )
		return (NULL);

	ship = new_ship();

	for ( ;; )
	{
		letter = fread_letter( fp );
		if ( letter == '*' )
		{
			fread_to_eol( fp );
			continue;
		}
		
		if ( letter != '#' )
		{
			log( "fread_ship: # not found." );
			break;
		}
		
		word = fread_word( fp );
		if ( !str_cmp( word, "SHIP" ) )
		{
			if ( !fread_one_ship( ship, fp ) )
			{
				fclose(fp);
				return (NULL);
			}
		}
		else if (!strcmp(word, "ROOMS"))
			fread_ship_rooms(ship, fp);
		else if ( !str_cmp( word, "ROOMCONTENT" ) )
			fread_ship_contents( ship, fp );
		else if ( !str_cmp( word, "END" ) )
			break;
		else
		{
			log( "fread_ship: bad section." );
			continue;
		}
	}

	fclose( fp );

	return (ship);
}

/* load all saved rooms */
void LoadShips( void )
{
	FILE *fp;
	SHIP_DATA *ship;
	char fname[128];

	sprintf( fname, "%sindex", LIB_SHIPS );

	if ( !( fp = fopen(fname, "r") ) )
		return;

	while (!feof(fp))
	{
		char *shipname = fread_word( fp );

		if ( shipname[0] == '$' )
			break;

		if ( !( ship = fread_ship( shipname ) ) )
			log( "SYSERR: LoadShips() unable to read ship file %s.", shipname );
		else
		{
			ROOM_DATA *pRoom;

			/* add to ship list */
			LINK(ship, first_ship, last_ship, next, prev);

			top_ship_num++;

			/* place in the world */
			if ( SHIP_FLAGGED(ship, SHIP_IN_PORT ) )
				pRoom = get_wild_room( ship->port );
			else if ( SHIP_FLAGGED(ship, SHIP_SAIL) || SHIP_FLAGGED(ship, SHIP_AT_ANCHOR) )
			{
				pRoom = get_wild_room( ship->port );
				DISPOSE( ship->port );			// not a port, destroy
			}
			else
			{
				log( "SYSERR: no coord to place ship %d.", ship->vnum );
				if ( !port_table )
				{
					log( "SYSERR: no ports loaded." );
					free_ship(ship);
					continue;
				}
				pRoom = get_wild_room( port_table->coord );
			}
			
			ship_to_room( ship, pRoom );
		}
	}

	fclose (fp);
}

ACMD(do_shipread)
{
	SHIP_DATA *ship;
	ROOM_DATA *pRoom;

	one_argument(argument, arg);

	if ( !*arg )
		return;

	ship = fread_ship( arg );

	if ( !ship )
	{
		log("SYSERR: unable to load ship file %s.", arg );
		return;
	}

	if ( SHIP_FLAGGED(ship, SHIP_IN_PORT ) )
		pRoom = get_wild_room( ship->port );
	else if ( SHIP_FLAGGED(ship, SHIP_SAIL) || SHIP_FLAGGED(ship, SHIP_AT_ANCHOR) )
	{
		pRoom = get_wild_room( ship->port );
		DISPOSE( ship->port );			// not a port, destroy
	}
	else
	{
		log( "SYSERR: no coord to place ship %d.", ship->vnum );
		if ( !port_table )
		{
			log( "SYSERR: no ports loaded." );
			return;
		}
		pRoom = get_wild_room( port_table->coord );
	}

	ship_to_room( ship, pRoom );
}


/* ******************************************************************* */
/* Ship prototypes                                                     */
/* ******************************************************************* */

SHIP_TYPE *fread_ship_type( FILE *fp )
{
	SHIP_TYPE *tShip;
	char *word;
	bool fMatch;
	
	CREATE( tShip, SHIP_TYPE, 1 );
	tShip->name				= NULL;
	tShip->short_descr		= NULL;
	tShip->vnum				= NOTHING;
	tShip->skill_min		= 1;
	tShip->size				= 0;
	
	tShip->value.defense	= 0;
	tShip->value.equip		= 0;
	tShip->value.health		= 0;
	tShip->value.loads		= 0;
	tShip->value.power		= 0;
	tShip->value.speed		= 0;
	
	for ( ; ; )
	{
		word   = feof( fp ) ? "End" : fread_word( fp );
		fMatch = FALSE;
		
		switch ( UPPER(word[0]) )
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol( fp );
			break;
			
		case 'D':
			KEY( "Defense",		tShip->value.defense,	fread_number(fp) );
			KEY( "Descr",		tShip->short_descr,		fread_string_nospace(fp) );
			break;
			
		case 'E':
			KEY( "Equip",		tShip->value.equip,		fread_number(fp) );
			if ( !str_cmp( word, "End" ) )
			{
				if ( tShip->vnum == NOTHING )
				{
					log("SYSERR: ship prototipe has no vnum.");
					exit(1);
				}
				if ( !tShip->name )
					tShip->name = str_dup("ship error");
				if ( !tShip->short_descr )
					tShip->short_descr = str_dup("Ship Error");

				return (tShip);
			}
			break;
			
		case 'H':
			KEY( "Health",		tShip->value.health,	fread_number(fp) );
			break;
			
		case 'L':
			KEY( "Loads",		tShip->value.loads,		fread_number(fp) );
			break;
			
		case 'N':
			KEY( "Name",		tShip->name,			fread_string_nospace(fp) );
			break;
			
		case 'P':
			KEY( "Power",		tShip->value.power,		fread_number(fp) );
			break;

		case 'S':
			KEY( "Size",		tShip->size,			fread_number(fp) );
			KEY( "Speed",		tShip->value.speed,		fread_number(fp) );
			KEY( "Skill",		tShip->skill_min,		fread_number(fp) );
			break;

		case 'V':
			KEY("Vnum",			tShip->vnum,			fread_number(fp));
			break;
		}			

		if ( !fMatch )
			log( "fread_ship_type: no match: %s", word );
	}

	return (NULL);
}


void load_ship_table( void )
{
	FILE *fp;
	char indname[256];
	char letter;
	char *word;
	
	sprintf(indname, "%sships.txt", SHIP_PREFIX);
	if (!(fp = fopen(indname, "r")))
	{
		log( "Cannot open ship table file %s", indname );
		exit(0);
	}
	
	top_ship_table = 0;
	
	for ( ;; )
	{
		letter = fread_letter( fp );
		if ( letter == '*' )
		{
			fread_to_eol( fp );
			continue;
		}
		
		if ( letter != '#' )
		{
			log( "Load_ship_table: # not found." );
			break;
		}
		
		word = fread_word( fp );
		if ( !str_cmp( word, "SHIP" ) )
		{
			if ( top_ship_table >= NUM_SHIP_TYPE )
			{
				log( "load_ship_table: more ships than NUM_SHIP_TYPE %d", NUM_SHIP_TYPE );
				fclose( fp );
				return;
			}
			ship_table[top_ship_table++] = fread_ship_type( fp );
			continue;
		}
		else
			if ( !str_cmp( word, "END" ) )
				break;
			else
			{
				log( "Load_ship_table: bad section." );
				continue;
			}
	}

	fclose( fp );
}


/* ******************************************************************* */
/* Special Procedures for buying ships                                 */
/* ******************************************************************* */

SPECIAL(dock)
{
	if (CMD_IS("list"))
	{
		int y;

		send_to_char("You can buy the following ships:\r\n", ch);
		for ( y = 0; y < top_ship_table; y++ )
			ch_printf(ch, "  %-20s %d\r\n", ship_table[y]->name,
				ship_table[y]->size * ship_table[y]->value.loads * 20);
		return (1);
	}

	if (CMD_IS("buy"))
	{
		SHIP_TYPE *tShip;
		PORT_DATA *port;
		SHIP_DATA *pShip;
		char arg[MAX_STRING_LENGTH], *shipname;
		int price, money;

		argument = one_argument( argument, arg );
		shipname = strtok(argument, "'");
		
		if (shipname == NULL)
		{
			send_to_char("Which name?\r\n", ch);
			return (1);
		}
		
		shipname = strtok(NULL, "'");
		if (shipname == NULL)
		{
			send_to_char("Ship names must be enclosed in the Holy Magic Symbols: '\r\n", ch);
			return (1);
		}
		
		if ( !*arg || !*shipname )
		{
			send_to_char("Usage: buy <type> 'ship name'\r\n", ch);
			return (1);
		}

		if (!(port = get_port_by_coord(ch->in_room->coord)))
		{
			log( "SYSERR: SPECIAL(dock) - No port found at coord %d %d.\r\n", GET_Y(ch), GET_X(ch) );
			return (1);
		}

		/* get ship type */
		if (!(tShip = parse_ship_type(arg)))
		{
			int y, price;
			
			ch_printf(ch, "Unknown ship type '%s'.\r\nYou can buy the following ships:\r\n", arg);
			for ( y = 0; y < top_ship_table; y++ )
			{
				price = ship_table[y]->size * ship_table[y]->value.loads * 20;
				ch_printf(ch, "  %40s %d\r\n", ship_table[y]->short_descr, price);
			}
			return (1);
		}

		/* Calc price & pay ship */
		price = tShip->size * tShip->value.loads * 20;
		money = get_gold(ch) + GET_BANK_GOLD(ch);

		if ( price > money )
		{
			ch_printf(ch, "You need %d gold coins to buy %s %s.\r\n",
				price, AN(tShip->name), tShip->name);
			if (IS_IMMORTAL(ch))
				price = 0;
			else
				return (1);
		}

		if ( price > get_gold(ch) )
		{
			int diff = price - get_gold(ch);

			sub_gold(ch, get_gold(ch));
			GET_BANK_GOLD(ch) -= diff;
		}
		else
			sub_gold(ch, price);

		pShip = create_ship(ch, tShip, port, shipname);
		ship_to_room(pShip, ch->in_room);

		ch_printf(ch, "Now you are the owner and captain of %s %s named '%s'\r\n",
			AN(tShip->name), tShip->name, pShip->name);

		return (1);
	}

	return (0);
}


/* ******************************************************************* */
/* Ports & Courses                                                     */
/* ******************************************************************* */

PORT_DATA *get_port_by_name( char *name )
{
	PORT_DATA *port;

	if ( !port_table || !name )
		return (NULL);

	for ( port = port_table; port; port = port->next )
		if ( isname( name, port->name ) )
			break;

	return (port);
}

PORT_DATA *get_port_by_coord( COORD_DATA *coord )
{
	PORT_DATA *port;

	if ( !port_table || !coord )
		return (NULL);

	for ( port = port_table; port; port = port->next )
		if ( coord->y == GET_RY(port) && coord->x == GET_RX(port) )
			break;

	return (port);
}

char *get_port_name( COORD_DATA *coord )
{
	PORT_DATA *port = get_port_by_coord( coord );

	if ( !port )
		return ("None");

	return ( port->name );
}


/*
 * given port coordinates and the destination port, returns the course
 */
COURSE_DATA *get_course(COORD_DATA *orig, PORT_DATA *port_dest)
{
	COURSE_DATA *course;

	if ( !course_table )
		return (NULL);

	for ( course = course_table; course; course = course->next )
	{
		if ( course->port_orig->y == orig->y &&
		     course->port_orig->x == orig->x &&
		     course->port_end->y == port_dest->coord->y &&
		     course->port_end->x == port_dest->coord->x )
			break;
	}

	/* if not found, try the reverse course */
	if ( !course )
	{
		for ( course = rev_course_table; course; course = course->next )
		{
			if ( course->port_orig->y == orig->y &&
			     course->port_orig->x == orig->x &&
			     course->port_end->y == port_dest->coord->y &&
			     course->port_end->x == port_dest->coord->x )
				break;
		}
	}

	return (course);
}

COURSE_DATA *get_course_by_vnum( int vnum )
{
	COURSE_DATA *course;

	if ( !course_table )
		return (NULL);

	for ( course = course_table; course; course = course->next )
		if ( course->vnum == vnum )
			break;

	return (course);
}


COURSE_STEP *get_course_step( COURSE_DATA *course, COORD_DATA *coord )
{
	COURSE_STEP *cstep;

	if ( !course || !coord )
		return (NULL);

	for ( cstep = course->first_step; cstep; cstep = cstep->next )
		if ( cstep->coord.y == coord->y && cstep->coord.x == coord->x )
			break;

	return (cstep);
}


COURSE_DATA *fread_course( FILE *fp )
{
	COURSE_DATA *course;
	char *word;
	bool fMatch;

	CREATE( course, COURSE_DATA, 1 );
	course->name		= NULL;
	course->vnum		= NOTHING;
	course->first_step	= NULL;
	course->last_step	= NULL;
	course->next		= NULL;

	for ( ; ; )
	{
		word	= feof( fp ) ? "End" : fread_word( fp );
		fMatch	= FALSE;
		
		switch ( UPPER(word[0]) )
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol( fp );
			break;

		case 'E':
			if ( !str_cmp( word, "End" ) )
			{
				/* add to the list */
				course->next	= course_table;
				course_table	= course;
				
				return (course);
			}
			break;
	
		case 'N':
			KEY( "Name",		course->name,			fread_string_nospace(fp) );
			break;

		case 'P':
			if ( !str_cmp( word, "Port_Dest" ) )
			{
				CREATE( course->port_end, COORD_DATA, 1 );
				course->port_end->y		= fread_number(fp);
				course->port_end->x		= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if ( !str_cmp( word, "Port_Orig" ) )
			{
				CREATE( course->port_orig, COORD_DATA, 1 );
				course->port_orig->y	= fread_number(fp);
				course->port_orig->x	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'S':
			if ( !str_cmp( word, "Step" ) )
			{
				COURSE_STEP *cstep;

				CREATE( cstep, COURSE_STEP, 1 );

				cstep->coord.y			= fread_number(fp);
				cstep->coord.x			= fread_number(fp);

				LINK( cstep, course->first_step, course->last_step, next, prev );

				fMatch = TRUE;
				break;
			}
			break;

		case 'V':
			KEY( "Vnum",		course->vnum,			fread_number(fp) );
			break;
		}
	}

	return (NULL);
}

void ReverseCourse( void )
{
	COURSE_DATA *cnew, *course;
	COURSE_STEP *cstep, *newstep;

	if ( !course_table )
		return;

	log( "Inverting Courses table" );

	for ( course = course_table; course; course = course->next )
	{
		CREATE( cnew, COURSE_DATA, 1 );
		cnew->name				= str_dup(course->name);
		cnew->vnum				= course->vnum;

		CREATE( cnew->port_orig, COORD_DATA, 1 );
		CREATE( cnew->port_end, COORD_DATA, 1 );
		cnew->port_orig->y		= course->port_end->y;
		cnew->port_orig->x		= course->port_end->x;
		cnew->port_end->y		= course->port_orig->y;
		cnew->port_end->x		= course->port_orig->x;

		for ( cstep = course->last_step; cstep; cstep = cstep->prev )
		{
			CREATE( newstep, COURSE_STEP, 1 );
			newstep->coord.y	= cstep->coord.y;
			newstep->coord.x	= cstep->coord.x;

			LINK( newstep, cnew->first_step, cnew->last_step, next, prev );
		}

		cnew->next				= rev_course_table;
		rev_course_table		= cnew;
	}
}


void load_ports( void )
{
	FILE *fp;
	PORT_DATA *port;
	ROOM_DATA *pRoom;
	char fname[128];
	char letter;
	char *word;

	sprintf(fname, "%sharbours.txt", SHIP_PREFIX);
	if (!(fp = fopen(fname, "r")))
	{
		log( "Cannot open ports file %s", fname );
		exit(0);
	}

	for ( ;; )
	{
		letter = fread_letter( fp );
		if ( letter == '*' )
		{
			fread_to_eol( fp );
			continue;
		}
		
		if ( letter != '#' )
		{
			log( "load_ports: # not found." );
			break;
		}
		
		word = fread_word( fp );

		if ( !str_cmp( word, "HARBOUR" ) )
		{
			CREATE(port, PORT_DATA, 1);
			CREATE(port->coord, COORD_DATA, 1);
			port->coord->y		= fread_number(fp);
			port->coord->x		= fread_number(fp);
			port->name			= fread_string_nospace(fp);

			port->next			= port_table;
			port_table			= port;
		}
		else if ( !str_cmp( word, "COURSE" ) )
		{
			fread_course( fp );
		}
		else if ( !str_cmp( word, "END"	) )
			break;
		else
		{
			log( "load_ports: bad section %s.", word );
			continue;
		}
	}

	fclose( fp );

	/* setup wild rooms as harbours */
	for ( port = port_table; port; port = port->next )
	{
		pRoom = create_wild_room(port->coord, TRUE);
		pRoom->func = dock;
		if ( SECT(pRoom) != SECT_PORT )
		{
			put_sect( GET_RY(pRoom), GET_RX(pRoom), SECT_PORT, TRUE );
			SECT(pRoom) = SECT_PORT;
		}
	}

	ReverseCourse();
}

/* ******************************************************************* */
/* Player courses handling                                             */
/* ******************************************************************* */
bool knows_course( CHAR_DATA *ch, int cnum )
{
	KNOWN_COURSE *kCourse;

	if (cnum < 0)
		return (FALSE);

	if (IS_IMMORTAL(ch) || IS_NPC(ch))
		return (TRUE);

	for (kCourse = ch->player_specials->courses; kCourse; kCourse = kCourse->next)
	{
		if (kCourse->coursenum == cnum)
			break;
	}

	return (kCourse ? TRUE : FALSE);
}


int learn_course(CHAR_DATA *ch, int cnum)
{
	KNOWN_COURSE *kCourse;

	if (cnum < 0)
		return (-1);

	if (knows_course(ch, cnum))
		return (0);

	CREATE(kCourse, KNOWN_COURSE, 1);
	kCourse->coursenum	= cnum;

	// add to the list
	kCourse->next					= ch->player_specials->courses;
	ch->player_specials->courses	= kCourse;

	return (1);
}



/* ******************************************************************* */
/*                      F E R R Y B O A T S                           */
/* ******************************************************************* */

FERRY_DATA *get_ferry(int vnum)
{
	FERRY_DATA *pFerry;

	if (!first_ferry)
		return (NULL);

	for (pFerry = first_ferry; pFerry; pFerry = pFerry->next)
	{
		if (pFerry->vnum == vnum)
			break;
	}

	return (pFerry);
}


void ferry_to_port(FERRY_DATA *pFerry)
{
	EXIT_DATA *fexit, *rexit;

	// link ferry to port
	fexit = make_exit(pFerry->room, NULL, DOWN);
	fexit->to_room = pFerry->in_room;
	rexit = make_exit(pFerry->in_room, NULL, UP);
	rexit->to_room = pFerry->room;
}

/* ***************************************************** */

EVENTFUNC(ferry_move_event)
{
	FERRY_EVENT *fmove = (FERRY_EVENT *) (event_obj);
	FERRY_DATA *pFerry;
	EXIT_DATA *pexit;
	CHAR_DATA *ppl;
	ROOM_DATA *last_room;
	sh_int dir;

	pFerry	= fmove->ferry;
	dir	= fmove->dir;

	if (!pFerry || !pFerry->action)
	{
		free(event_obj);
		return (0);
	}

	pexit = get_exit(pFerry->in_room, dir);

	if (!pexit || !pexit->to_room)
	{
		free(event_obj);
		return (0);
	}

	send_to_room("The ferryboat has left.", pFerry->in_room); 

	pFerry->in_room->ferryboat	= NULL;
	last_room					= pFerry->in_room;
	// stanziamolo..
	pFerry->in_room				= pexit->to_room;
	pFerry->in_room->ferryboat	= pFerry;

	send_to_room("The ferryboat has arrived.", pFerry->in_room);

	/* Handle Wild Sectors */
	if ( IS_WILD(pFerry->in_room) )
	{
		check_wild_move( NULL, NULL, NULL, pFerry, last_room, pFerry->in_room );

		if (ROOM_FLAGGED(pFerry->in_room, ROOM_WILD_REMOVE))
			wild_remove_dequeue(pFerry->in_room);
		if (!ROOM_FLAGGED(pFerry->in_room, ROOM_WILD_STATIC))
			SET_BIT(ROOM_FLAGS(pFerry->in_room), ROOM_WILD_STATIC);
	}

	for ( ppl = pFerry->room->people; ppl; ppl = ppl->next_in_room )
	{
		if ( !FIGHTING(ppl) )
			look_at_wild(ppl, TRUE);
	}

	// check for port at next step
	if (SECT(pFerry->in_room) == SECT_PORT)
	{
		send_to_room("The ferryboat has arrived in port. Go down to disembark.", pFerry->room);
		
		// setup ferry data
		pFerry->timer	= FERRY_WAIT + number(-2, 3);
		pFerry->action	= NULL;
		if (dir == pFerry->dir)
			pFerry->place = 2;
		else
			pFerry->place = 1;

		ferry_to_port(pFerry);

		return (0);
	}
	else if (!water_sector(SECT(pexit->to_room)))
	{
		log("SYSERR: ferry %d moved in a non-water sector.", pFerry->vnum);
		free(event_obj);
		return(0);
	}

	send_to_room("The ferryboat moves...", pFerry->room);

	// yet another step
	return (FERRY_SPEED);
}


/*
 * called every minute from heartbeat() in comm.c
 */
void ferry_move(void)
{
	FERRY_DATA *pFerry;
	FERRY_EVENT *fmove;
	sh_int dir;

	if (!first_ferry)
		return;

	for (pFerry = first_ferry; pFerry; pFerry = pFerry->next)
	{
		// if action is not null, the ferry is moving...
		if (pFerry->action)
			continue;

		// countdown to departure
		if (--pFerry->timer > 0)
			continue;

		// reverse dir for sailing from site2 to site1
		if (pFerry->place == 2)
			dir = rev_dir[pFerry->dir];
		else
			dir = pFerry->dir;

		extract_exit(pFerry->room, get_exit(pFerry->room, DOWN));
		extract_exit(pFerry->in_room, get_exit(pFerry->in_room, UP));

		// initialize ferry move event
		CREATE(fmove, FERRY_EVENT, 1);
		fmove->ferry	= pFerry;
		fmove->dir		= dir;

		// create event
		pFerry->action = event_create(ferry_move_event, fmove, FERRY_SPEED);

		send_to_room("The ferryboat leave the port and begin to cross the river.\r\n", pFerry->room);
		send_to_room("The ferryboat leave the port and begin to cross the river.\r\n", pFerry->in_room);
	}
}


/* ********************************************************* */

FERRY_DATA *new_ferry(void)
{
	FERRY_DATA *pFerry;

	CREATE(pFerry, FERRY_DATA, 1);

	pFerry->action				= NULL;
	pFerry->in_room				= NULL;
	pFerry->mob					= NULL;
	pFerry->next				= NULL;
	pFerry->prev				= NULL;
	pFerry->vnum				= NOTHING;
	pFerry->cost				= 0;
	pFerry->dir					= NOWHERE;

	// default values
	pFerry->place				= 1;
	pFerry->timer				= FERRY_WAIT;

	// setup ferry room
	pFerry->room				= new_room();
	pFerry->room->name			= str_dup("Aboard the ferryboat");
	pFerry->room->sector_type	= SECT_FERRY_DECK;
	pFerry->room->zone			= FERRY_ZONE;
	pFerry->room->description	= str_dup("  You are aboard the ferryboat.\r\n");

	CREATE(pFerry->room->extra_data, ROOM_EXTRA, 1);
	pFerry->room->extra_data->max_hp	= 50;
	pFerry->room->extra_data->curr_hp	= 50;

	return (pFerry);
}


bool fread_one_ferry(FERRY_DATA *pFerry, FILE *fp)
{
	char *word;
	bool fMatch;

	for ( ; ; )
	{
		word   = feof( fp ) ? "End" : fread_word( fp );
		fMatch = FALSE;
		
		switch ( UPPER(word[0]) )
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol( fp );
			break;

		case 'C':
			if (!strcmp(word, "Coord"))
			{
				fMatch = TRUE;

				CREATE(pFerry->coord, COORD_DATA, 1);

				pFerry->coord->y = fread_number(fp);
				pFerry->coord->x = fread_number(fp);

				if (!get_port_by_coord(pFerry->coord))
				{
					log("SYSERR: ferryboat must load in a port.");
					return (FALSE);
				}
				break;
			}
			KEY("Cost",			pFerry->cost,			fread_number(fp));
			break;

		case 'D':
			KEY("Dir",			pFerry->dir,			fread_number(fp));
			break;

		case 'E':
			if (!strcmp(word, "End"))
				return (TRUE);
			break;

		case 'M':
			if (!strcmp(word, "Mob"))
			{
				int vnum = fread_number(fp);

				if (!(pFerry->mob = read_mobile(vnum, VIRTUAL)))
					log("SYSERR: ferryboat's mob has invalid vnum %d.", vnum);

				fMatch = TRUE;
				break;	
			}
			break;

		case 'V':
			if (!strcmp(word, "Vnum"))
			{
				pFerry->vnum					= fread_number(fp);
				pFerry->room->extra_data->vnum	= pFerry->vnum;
				fMatch = TRUE;
				break;
			}
			break;
		}
	}

	return (FALSE);
}


FERRY_DATA *fread_ferry( char *sname ) 
{
	FILE *fp;
	FERRY_DATA *pFerry;
	char fname[128];
	char letter;
	char *word;

	sprintf( fname, "%s%s", LIB_SHIPS, sname );
	if ( !( fp = fopen(fname, "r") ) )
		return (NULL);

	pFerry = new_ferry();

	for ( ;; )
	{
		letter = fread_letter( fp );
		if ( letter == '*' )
		{
			fread_to_eol( fp );
			continue;
		}
		
		if ( letter != '#' )
		{
			log( "fread_ferry(): # not found." );
			break;
		}
		
		word = fread_word( fp );
		if ( !str_cmp( word, "FERRY" ) )
		{
			if ( !fread_one_ferry( pFerry, fp ) )
			{
				fclose(fp);
				return (NULL);
			}
		}
		else if (!str_cmp(word, "END"))
			break;
		else
		{
			log( "fread_ferry(): bad section." );
			continue;
		}
	}

	fclose( fp );

	return (pFerry);
}

/* load all saved rooms */
void LoadFerryboats( void )
{
	FILE *fp;
	FERRY_DATA *pFerry;
	char fname[128];

	sprintf(fname, "%sferryindex", LIB_SHIPS);

	if ( !( fp = fopen(fname, "r") ) )
		return;

	while (!feof(fp))
	{
		char ferryname[128];
		
		strcpy(ferryname, fread_word(fp));

		if (ferryname[0] == '$')
			break;

		if (!( pFerry = fread_ferry(ferryname)))
			log("SYSERR: LoadFerryboats() unable to read ferry file %s.", ferryname);
		else
		{
			ROOM_DATA *pRoom;

			// add to the list
			LINK(pFerry, first_ferry, last_ferry, next, prev);

			/* place in the world */
			if (!(pRoom = get_wild_room(pFerry->coord)))
			{
				log("SYSERR: LoadFerryboats() - port room must exist!");
				exit(1);
			}

			pRoom->ferryboat	= pFerry;
			pFerry->in_room		= pRoom;

			if (pFerry->mob)
				char_to_room(pFerry->mob, pFerry->room);

			ferry_to_port(pFerry);
		}
	}

	fclose (fp);
}
