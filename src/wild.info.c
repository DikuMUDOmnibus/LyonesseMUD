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
 * File: wild.info.c							  *
 *                                                                        *
 * Code for survey, track, scan, and all wild information related funcs   *
 * Code for portal stones                                                 *
 * Code for camping                                                       *
 *                                                                        *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include <math.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "constants.h"

extern TIME_INFO_DATA	time_info;

/* external funcs */
ROOM_DATA *create_wild_room(COORD_DATA *coord, bool Static);
WEATHER_DATA *get_coord_weather(COORD_DATA *coord);
int		list_scanned_chars_to_char(CHAR_DATA *list, CHAR_DATA *ch, int dir, int dist);
void	show_ship_on_map(CHAR_DATA *ch);

/* locals */
ACMD(do_camp);
EVENTFUNC(camp_event);

/* globals */
PSTONE_DATA	*first_pstone	= NULL;
PSTONE_DATA	*last_pstone	= NULL;

/* *************************************************************** */
/*                            M A T H                              */
/* *************************************************************** */

double distance(int chX, int chY, int lmX, int lmY)
{
	double xchange, ychange, distance;
	
	xchange = (chX - lmX);
	xchange *= xchange;
	ychange = (chY - lmY);
	ychange *= ychange;
	
	distance = sqrt((xchange + ychange));

	return (distance);
}

int calc_angle(int chX, int chY, int lmX, int lmY, int *ipDist)
{
	double dDist1, dDist2, dTandeg, dDeg;
	int iNx1 = 0, iNy1 = 0, iNx2, iNy2, iNx3, iNy3, iFinal;

	iNx2 = lmX - chX;
	iNy2 = lmY - chY;
	iNx3 = 0;
	iNy3 = iNy2;
	
	*ipDist = (int) distance( iNx1, iNy1, iNx2, iNy2 );
	
	if ( iNx2 == 0 && iNy2 == 0 )	return ( -1 );
	if ( iNx2 == 0 && iNy2 > 0 )	return ( 180 );
	if ( iNx2 == 0 && iNy2 < 0 )	return ( 0 );
	if ( iNy2 == 0 && iNx2 > 0 )	return ( 90 );
	if ( iNy2 == 0 && iNx2 < 0 )	return ( 270 );
	
	/* ADJACENT */
	dDist1 = distance( iNx1, iNy1, iNx3, iNy3 );
	
	/* OPPOSITE */
	dDist2 = distance( iNx3, iNy3, iNx2, iNy2 );
	
	dTandeg = dDist2 / dDist1;
	dDeg = atan(dTandeg);
	
	iFinal = ( dDeg * 180 ) / 3.14159265358979323846;
	
	if ( iNx2 > 0 && iNy2 > 0 ) 	return ( ( 90 + ( 90 - iFinal ) ) );
	if ( iNx2 > 0 && iNy2 < 0 )		return ( iFinal );
	if ( iNx2 < 0 && iNy2 > 0 )		return ( ( 180 + iFinal ) );
	if ( iNx2 < 0 && iNy2 < 0 )		return ( ( 270 + ( 90 - iFinal ) ) );
	
	return ( -1 );
}

int clock_to_dir( int oclock )
{
	if ( oclock == 12 )					return (NORTH);
	if ( oclock == 6 )					return (SOUTH);
	if ( oclock == 3 )					return (EAST);
	if ( oclock == 9 )					return (WEST);
	if ( oclock == 10 || oclock == 11 )	return (NORTHWEST);
	if ( oclock == 2 || oclock == 1 )	return (NORTHEAST);
	if ( oclock == 4 || oclock == 5 )	return (SOUTHEAST);
	if ( oclock == 7 || oclock == 8 )	return (SOUTHWEST);
	return (NOWHERE);
}

int angle_to_dir( int iAngle )
{
	int cDir;

	if		(iAngle == -1)		cDir = 13;
	else if (iAngle >= 360)		cDir = 12;
	else if (iAngle >= 330)		cDir = 11;
	else if (iAngle >= 300)		cDir = 10;
	else if (iAngle >= 270)		cDir = 9;
	else if (iAngle >= 240)		cDir = 8;
	else if (iAngle >= 210)		cDir = 7;
	else if (iAngle >= 180)		cDir = 6;
	else if (iAngle >= 150)		cDir = 5;
	else if (iAngle >= 120)		cDir = 4;
	else if (iAngle >= 90)		cDir = 3;
	else if (iAngle >= 60)		cDir = 2;
	else if (iAngle >= 30)		cDir = 1;
	else 						cDir = 12;
	
	return (cDir);
}

int distance_to_index( int iDist )
{
	int iMes;

	if		(iDist > 200)		iMes = 0;
	else if (iDist > 150)		iMes = 1;
	else if (iDist > 100)		iMes = 2;
	else if (iDist > 75)		iMes = 3;
	else if (iDist > 50)		iMes = 4;
	else if (iDist > 25)		iMes = 5;
	else if (iDist > 15)		iMes = 6;
	else if (iDist > 10)		iMes = 7;
	else if (iDist > 5)			iMes = 8;
	else if (iDist > 1)			iMes = 9;
	else 						iMes = 10;

	return (iMes);

}

/* =================================================== */
/*                    S U R V E Y                      */
/* =================================================== */
ACMD(do_survey)
{
	SURVEY_DATA *sd;
	bool count = FALSE;
	int ry, rx, iDist, iAngle, dmes, ames;

	if (!IN_WILD(ch) && !ON_DECK(ch) && !IN_FERRY(ch))
	{
		send_to_char("You can use survey only in the wilderness.\r\n", ch);
		return;
	}
	
	if (AFF_FLAGGED(ch, AFF_BLIND))
	{
		send_to_char("You can't see a damn thing, you're blind!\r\n", ch);
		return;
	}
	
	/*
	 * survey called from a ship deck shows only
	 * ships around
	 */
	if (ON_DECK(ch))
	{
		show_ship_on_map(ch);
		return;
	}
	else if (IN_FERRY(ch))
	{
		FERRY_DATA *pFerry = get_ferry(ch->in_room->extra_data->vnum);

		ry = GET_RY(pFerry->in_room);
		rx = GET_RX(pFerry->in_room);
	}
	else
	{
		ry = GET_RY(ch->in_room);
		rx = GET_RX(ch->in_room);
	}

	for ( sd = survey_table; sd; sd = sd->next )
	{
		iDist = (int) distance(rx, ry, sd->coord->x, sd->coord->y);

		if ( iDist > MAX_SURVEY_DIST )
			continue;

		count = TRUE;

		iAngle = calc_angle( rx, ry, sd->coord->x, sd->coord->y, &iDist );

		ames = angle_to_dir( iAngle );
		dmes = distance_to_index( iDist );

		if ( dmes == 13)
			sprintf(buf,"In the immediate area, %s\r\n", sd->descriz ? sd->descriz : "<NULL PLEASE REPORT>");
		else	
			sprintf(buf,"At %d o'clock, %s %s\r\n", ames, dist_descr[(int) dmes], sd->descriz ? sd->descriz : "<NULL PLEASE REPORT>");
		send_to_char(buf,ch);
	}

	if ( !count )
		send_to_char("You don't see anything nearby.\r\n", ch);
}


/* =================================================== */
/*                     T R A C K                       */
/* =================================================== */

int wild_track( CHAR_DATA *ch, CHAR_DATA *vict, bool silent )
{
	int dist, angle, dir;

	/* safeguard */
	if ( !ch || !vict )
		return (NOWHERE);

	angle = calc_angle( GET_X(ch), GET_Y(ch), GET_X(vict), GET_Y(vict), &dist );

	if ( dist > MAX_TRACK_DIST )
	{
		if ( !silent )
			send_to_char("You sense no trail.\r\n", ch);
		return (NOWHERE);
	}

	dir = clock_to_dir(angle_to_dir(angle));

	if ( dir == 13 )
		dir = NOWHERE;

	if ( !silent )
		ch_printf(ch, "You sense a trail %s%s&0 from here!\r\n",
			exits_color[dir], dirs[dir]);

	return (dir);
}


int get_sail_dir( SHIP_DATA *ship, COORD_DATA *cto, bool silent )
{
	int iDist, iAngle, iDir;

	iDist = (int) distance( GET_X(ship), GET_Y(ship), cto->x, cto->y );

	/*
	if (iDist > MAX_SAIL_DIST)
	{
		if ( SHIP_FLAGGED(ship, SHIP_IN_COURSE ) )
			log("SYSERR: get_sail_dir() - next step (%d %d) is too far [%d].",
				cto->y, cto->x, iDist);

		if ( !silent && ship->helmperson )
			send_to_char("You fail to follow the course.\r\n", ship->helmperson );
		return (NOWHERE);
	}
	*/

	iAngle = calc_angle( GET_X(ship), GET_Y(ship), cto->x, cto->y, &iDist );
	iDir = clock_to_dir(angle_to_dir(iAngle));

	if ( iDir < 0 || iDir > NUM_OF_DIRS )
		iDir = NOWHERE;

	return (iDir);
}


/* =================================================== */
/*                     S C A N                         */
/* =================================================== */

void wild_scan(CHAR_DATA *ch)
{
	COORD_DATA curr, real;
	ROOM_DATA *pRoom;
	int chy, chx;
	int x, y, xmax, ymax, xmin, ymin;
	int iDist, radius, modx, mody, dir;
	int count = 0;

	x = y = xmax = ymax = xmin = ymin = 0;

	chy = GET_Y(ch);
	chx = GET_X(ch);

	mody	= MOD_SMALL_Y + 1;
	modx	= MOD_SMALL_X + 1;

	if (PRF_FLAGGED(ch, PRF_WILDSMALL))
		radius	= MAX_SCAN_DIST - 2;
	else
	{
		WEATHER_DATA *sky = get_coord_weather(ch->in_room->coord);

		radius	= MAX_SCAN_DIST;
	
		if (IS_MORTAL(ch))
		{
			/* modifica raggio visivo in base all'ora... */
			if (time_info.hours < 6 || time_info.hours > 21)
			{	
				radius -= 2;
				
				if (Sunlight == MOON_LIGHT)
				{
					if (MoonPhase < 2 || MoonPhase > 6)
						radius -= 1;
					else if (MoonPhase == MOON_FULL)
						radius += 1;
				}
			}
			
			/* ... e al tempo atmosferico */
			if (sky->precip_rate > 50)
				radius -= 2;
			else if (sky->precip_rate > 20)
				radius -= 1;
			
			radius = URANGE(1, radius, MAX_SCAN_DIST);
		}
	}

	y		= abs(chy - (chy / MAP_SIZE * MAP_SIZE)) + MAP_SIZE;
	x		= abs(chx - (chx / MAP_SIZE * MAP_SIZE)) + MAP_SIZE;

	ymin	= y - mody;
	ymax	= y + mody;
	xmin	= x - modx;
	xmax	= x + modx;

	real.y = chy - mody;
	
	for (curr.y = ymin; curr.y < ymax + 1; curr.y++, real.y++)
	{
		real.x = chx - modx;

		for (curr.x = xmin; curr.x < xmax + 1; curr.x++, real.x++)
		{
			if ((iDist = distance(x, y, curr.x, curr.y)) > radius)
				continue;

			if (!(pRoom = get_wild_room(&real)))
				continue;

			if (!pRoom->people)
				continue;

			if ((dir = wild_track(ch, pRoom->people, TRUE)) == NOWHERE)
				continue;

			count += list_scanned_chars_to_char(pRoom->people, ch, dir, iDist);
		}
	}

	if (count == 0)
		send_to_char("You don't see anyone nearby.\r\n", ch);
}

/* =================================================== */
/* Portal Stone Code                                   */
/* =================================================== */

PSTONE_DATA *get_pstone( int vnum )
{
	PSTONE_DATA *pstone;

	for ( pstone = first_pstone; pstone; pstone = pstone->next )
	{
		if ( vnum == pstone->vnum )
			break;
	}

	return (pstone);
}

PSTONE_DATA *get_pstone_by_name( char *pname )
{
	PSTONE_DATA *pstone;

	for ( pstone = first_pstone; pstone; pstone = pstone->next )
	{
		if ( isname( pname, pstone->name ) )
			break;
	}

	return (pstone);
}

PSTONE_DATA *get_pstone_by_coord( COORD_DATA *coord )
{
	PSTONE_DATA *pstone;

	for ( pstone = first_pstone; pstone; pstone = pstone->next )
	{
		if ( coord->y == pstone->coord.y && coord->x == pstone->coord.x )
			break;
	}

	return (pstone);
}

void show_travel( CHAR_DATA *ch )
{
	int pn;

	if ( !ch->in_room->portal_stone )
	{
		send_to_char("You can't Travel if in room there isn't a portal stone.\r\n", ch);
		return;
	}

	ch_printf(ch, "From the portal stone '&b&1%s&0' you can Travel to:\r\n",
		ch->in_room->portal_stone->name);
	for ( pn = 0; pn < MAX_LNK_STONE; pn++ )
	{
		if ( ch->in_room->portal_stone->link_to[pn] )
		{
			PSTONE_DATA *pstone = get_pstone(ch->in_room->portal_stone->link_to[pn]);

			if ( pstone )
				ch_printf(ch, "[%d] &b&1%-25s&0\t Keyword: %-15s\t Coord: %d %d\r\n",
					pstone->vnum,
					pstone->short_descr, pstone->name,
					pstone->coord.y, pstone->coord.x);
		}
	}
}

void stone_travel( CHAR_DATA *ch, char *argument )
{
	PSTONE_DATA *pstone;
	ROOM_DATA *pRoom;
	char arg[MAX_STRING_LENGTH];
	int pn;

	one_argument( argument, arg );

	if ( !*arg )
	{
		show_travel(ch);
		return;
	}

	if ( !ch->in_room->portal_stone )
	{
		send_to_char("You can't Travel if in room there isn't a portal stone.\r\n", ch);
		return;
	}

	if ( !(pstone = get_pstone_by_name(arg)) )
	{
		ch_printf(ch, "In this world there isn't a portal stone called '%s'.\r\n", arg);
		return;
	}

	for ( pn = 0; pn < MAX_LNK_STONE; pn++ )
	{
		if ( ch->in_room->portal_stone->link_to[pn] == pstone->vnum )
			break;
	}

	if ( pn >= MAX_LNK_STONE )
	{
		ch_printf(ch, "You can't Travel to '%s' from here.\r\n", arg);
		return;
	}

	if ( !( pRoom = get_wild_room( &pstone->coord ) ) )
		pRoom = create_wild_room( &pstone->coord, FALSE );
	
	if ( !pRoom->portal_stone )
		pRoom->portal_stone = pstone;

	act("As you touch the portal stone, a flash of pure white light surrounds you, and you Travel...",
		FALSE, ch, NULL, NULL, TO_CHAR);
	act("As $n touch the portal stone, a flash of pure white light surrounds $m, and $s Travel...",
		FALSE, ch, NULL, NULL, TO_ROOM);

	char_from_room(ch);
	char_to_room(ch, pRoom);

	// transport mount too
	if ( RIDING(ch) )
	{
		char_from_room(RIDING(ch));
		char_to_room(RIDING(ch), pRoom);
	}
	// transport vehicle too
	else if (WAGONER(ch))
	{
		vehicle_from_room(WAGONER(ch));
		vehicle_to_room(WAGONER(ch), ch->in_room);
	}

	act("You arrive at your destination, and suddenly loose your grip to realty...",
		FALSE, ch, NULL, NULL, TO_CHAR);
	act("$n suddenly appears from nowhere, lying here unconscious...",
		FALSE, ch, NULL, NULL, TO_ROOM);

	if ( IS_MORTAL(ch) )
		GET_POS(ch) = POS_STUNNED;
	else
		look_at_room(ch, TRUE);
}

ACMD(do_travel)
{
	stone_travel(ch, argument);
}

/* ************************************************************ */

PSTONE_DATA *new_pstone( void )
{
	PSTONE_DATA *pstone;
	int pn;

	CREATE(pstone, PSTONE_DATA, 1);
	pstone->next		= NULL;
	pstone->prev		= NULL;
	pstone->name		= NULL;
	pstone->short_descr	= NULL;
	pstone->description	= NULL;
	pstone->vnum		= NOTHING;
	
	for ( pn = 0; pn < MAX_LNK_STONE; pn++ )
		pstone->link_to[pn] = 0;

	LINK(pstone, first_pstone, last_pstone, next, prev);

	return (pstone);
}

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

void fread_pstone( FILE *fp )
{
	PSTONE_DATA *pstone = new_pstone();
	char *word;
	bool fMatch;
	int pn = 0;

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
			if ( !strcmp(word, "Coord") )
			{
				pstone->coord.y	= fread_number(fp);
				pstone->coord.x	= fread_number(fp);

				fMatch = TRUE;
				break;
			}
			break;

		case 'D':
			KEY("Description",	pstone->description,	fread_string_nospace(fp));
			break;

		case 'E':
			if ( !strcmp(word, "End") )
				return;

		case 'L':
			if ( !strcmp(word, "LinkStone") )
			{
				fMatch = TRUE;

				if ( pn >= MAX_LNK_STONE )
				{
					log("SYSERR: fread_pstone() - too many linked stone for stone %d.", pstone->vnum);
					break;
				}

				pstone->link_to[pn++] = fread_number(fp);
				break;
			}
			break;

		case 'N':
			KEY("Name",			pstone->name,			str_dup(fread_word(fp)));
			break;

		case 'S':
			KEY("Shortdescr",	pstone->short_descr,	fread_string_nospace(fp));
			break;

		case 'V':
			KEY("Vnum",			pstone->vnum,			fread_number(fp));
			break;

		default:
			log("SYSERR: Unknown word %s in pstone file", word);
			break;
		}

		if ( !fMatch )
			log( "fread_pstone(): no match: %s", word );
	}
}

void LoadPortalStones( void )
{
	FILE *fp;
	char fname[128];
	char letter;
	char *word;

	sprintf(fname, "%spstone.txt", WLS_PREFIX);
	if ( !( fp = fopen(fname, "r") ) )
	{
		log("   No Portal Stones defined.");
		return;
	}

	for ( ; ; )
	{
		letter = fread_letter( fp );
		if ( letter == '*' )
		{
			fread_to_eol( fp );
			continue;
		}
		
		if ( feof( fp ) )
			break;

		if ( letter != '#' )
		{
			log( "SYSERR: LoadPortalStones() - # not found." );
			break;
		}
		
		word = fread_word( fp );

		if ( !strcmp( word, "PSTONE" ) )
			fread_pstone(fp);
		else if ( !strcmp( word, "END" ) )	// Done
			break;
		else
		{
			log("SYSERR: LoadPortalStones() - bad section %s", word );
			continue;
		}
	}

	fclose(fp);
}


/* ************************************************************ */

void fwrite_pstone( FILE *fp, PSTONE_DATA *pstone )
{
	int pn;

	fprintf(fp, "#PSTONE\n");
	fprintf(fp, "Vnum            %d\n", pstone->vnum);
	fprintf(fp, "Coord           %hd %hd\n", pstone->coord.y, pstone->coord.x);
	if ( pstone->name )
		fprintf(fp, "Name            %s\n", pstone->name);
	if ( pstone->short_descr )
		fprintf(fp, "Shortdescr      %s~\n", pstone->short_descr);
	if ( pstone->description )
		fprintf(fp, "Description     %s~\n", pstone->description);
	
	for ( pn = 0; pn < MAX_LNK_STONE; pn++ )
	{
		if ( pstone->link_to[pn] )
			fprintf(fp, "LinkStone       %hd\n", pstone->link_to[pn]);
	}
	fprintf(fp, "End\n\n");
}

void SavePortalStones( void )
{
	FILE *fp;
	PSTONE_DATA *pstone;
	char fname[128];

	if ( !first_pstone )
		return;

	sprintf(fname, "%spstone.txt", WLS_PREFIX);
	if ( !( fp = fopen(fname, "w") ) )
		return;

	for ( pstone = first_pstone; pstone; pstone = pstone->next )
		fwrite_pstone(fp, pstone);
	
	fprintf(fp, "#END\n");
	fclose(fp);
}

/* ================================================================ */

ACMD(do_camp)
{
	ROOM_AFFECT *raff;
	int move_needed = 100;
	
	if (!IN_WILD(ch))
	{
		send_to_char("You can make a campsite only in the wilderness.\r\n", ch);
		return;
	}
	
	if ( RIDING(ch) )
	{
		send_to_char("You cannot prepare a camp while mounted.\r\n", ch);
		return;
	}

	if ( WAGONER(ch) )
	{
		send_to_char("You cannot prepare a camp while driving a vehicle.\r\n", ch);
		return;
	}

	if (ch->action)
	{
		send_to_char("You are too busy to do this!\r\n", ch);
		return;
	}
	
	if ((raff = get_room_aff_bitv(ch->in_room, RAFF_CAMP)))
	{
		ROOM_DATA *tRoom = ch->in_room;

		/* refresh the current campsite */
		raff->timer = 60;
		send_to_char( "You throw more wood on the fire and get the campsite ready.\r\n", ch );
		if (!GET_INVIS_LEV(ch))
			act( "$n throws more wood on the fire.", TRUE, ch, 0, 0, TO_ROOM );
		GET_MOVE(ch) /= 2;

		if (!GET_INVIS_LEV(ch))
			act("$n camps, preparing to leave this world.", TRUE, ch, 0, 0, TO_ROOM);
	}
	else
	{
		/* make a new campsite */
		CAMP_EVENT *ce;
		/* create and initialize the camp event */
		CREATE(ce, CAMP_EVENT, 1);
		ce->ch		= ch;
		ce->pRoom	= ch->in_room;

		ch->action = event_create( camp_event, ce, 100 );

		send_to_char("You begin to prepare a campsite...\r\n", ch);
		act("$n begins to prepare a campsite...", FALSE, ch, NULL, NULL, TO_ROOM);
	}
}

EVENTFUNC(camp_event)
{
	CAMP_EVENT *ce = (CAMP_EVENT *) event_obj;
	CHAR_DATA *ch = NULL;
	ROOM_DATA *tRoom;
	ROOM_AFFECT *raff;
	
	/* extract all the info from ce */
	ch			= ce->ch;
	tRoom		= ce->pRoom;
	
	/* make sure we're supposed to be here */
	if (!ch || !ch->desc || !tRoom)
		return (0);
	
	if (!ch->action)
		return (0);
	
	ch->action = NULL;
	
	if (!IN_WILD(ch) || !IS_WILD(tRoom))
	{
		send_to_char("You can prepare campsites only in the wilderness.\r\n", ch);
		return (0);
	}

	if ( RIDING(ch) )
	{
		send_to_char("You cannot prepare a camp while mounted.\r\n", ch);
		return (0);
	}

	if ( WAGONER(ch) )
	{
		send_to_char("You cannot prepare a camp while driving a vehicle.\r\n", ch);
		return (0);
	}
	
	if ( tRoom != ch->in_room )
	{
		send_to_char("You are no longer near where you began the campsite.\r\n", ch);
		return (0);
	}
	
	send_to_char("You complete your campsite, now you can leave this world for awhile.\r\n", ch);
	if (!GET_INVIS_LEV(ch))
		act("$n camps, preparing to leave this world.", TRUE, ch, 0, 0, TO_ROOM);

	free(event_obj);
	
	if ((raff = get_room_aff_bitv(tRoom, RAFF_CAMP)))
		raff->timer			= 60;
	else
	{
		/* create, initialize, and link a room-affection node */
		CREATE(raff, ROOM_AFFECT, 1);
		raff->coord			= NULL;
		raff->vroom			= NOWHERE;
		raff->timer			= 60;
		raff->bitvector		= RAFF_CAMP;
		raff->spell			= 0;
		raff->level			= GET_LEVEL(ch);
		raff->value			= 0;
		raff->text			= "The fire slowly fades and sputters out...\r\n";
		
		CREATE( raff->coord, COORD_DATA, 1 );
		raff->coord->y		= GET_RY(tRoom);
		raff->coord->x		= GET_RX(tRoom);
		
		/* add to the room list */
		raff->next_in_room	= tRoom->affections;
		tRoom->affections	= raff;
		
		/* add to the global list */
		raff->next			= raff_list;
		raff_list			= raff;
	}

	return (0);
}
