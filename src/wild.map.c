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
 * File: wild.map.c                                                       *
 *                                                                        *
 * Wilderness map code                                                    *
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

/*
 * Legenda:
 *                              %     Palette                         Map
 * Colour                   R   G   B   Idx     Sector               Char
 * ----------------------------------------------------------------------
 * BMP_COASTLINE            0   0   0     0     Coastline               .
 * BMP_ARTIC              255 255 255   215     Artic                   G
 * BMP_SEA                  0  51 255    41     Sea                     _
 * BMP_SHALLOWS             0 204 255   149     Shallows                -
 * BMP_RIVER                0  51 153    39     River                   a
 * BMP_NAVIGABLE_RIVER      0  51 102    38     River (Navigable)       k
 * BMP_HILL               204 153   0   132     Hill                    c
 * BMP_MOUNTAIN           153 102   0    90     Mountain                m
 * BMP_JUNGLE               0 102   0    72     Jungle                  F
 * BMP_FOREST              51 153   0   114     Forest                  f
 * BMP_FORESTED_HILL        0 153 102   110     Forested Hill           C
 * BMP_FIELD               51 204   0   150     Field                   v
 * BMP_PLAIN              204 255   0   204     Plain                   p
 * BMP_LAND               204 204 153   171     Wasteland               b
 * BMP_DESERT             255 204 102   171     Desert                  D
 * BMP_MOUNTAIN_PEAK      204 204 204   172     Mountain Peak           M
 * BMP_BEACH              255 255   0   210     Beach                   s
 * BMP_PLATEAU            255 255 153   213     Plateau                 A
 * BMP_ROAD               255   0   0    30     Road                    :
 * BMP_SWAMP              102 153 153   123     Swamp                   d
 * BMP_REEF               255 204   0   174     Reef                    Z
 * BMP_FORESTED_MOUNTAIN  153 153   0   126     Forested Mountain       T
 * BMP_BRIDGE              51   0   0     6     Bridge                  =
 * BMP_PORT               153   0   0    18     Port                    H
 * BMP_FORD                51 255 255   191     Ford                    u
 * BMP_ZONE_BORDER        255   0 240    34     Zone Border             I
 * BMP_ZONE_INSIDE        255 255 204   214     Zone Inside             
 * BMP_ZONE_EXIT          255 153 255   143     Zone Exit               @
 * ----------------------------------------------------------------------
 */

#define BMP_COASTLINE			0		// 0 0 0
#define BMP_ARTIC				215		// 255 255 255
#define BMP_SEA       			41		// 0 51 255
#define BMP_SHALLOWS   			149		// 0 204 255
#define BMP_RIVER				39		// 0 51	153
#define BMP_NAVIGABLE_RIVER		38		// 0 51 102
#define BMP_HILL       			132		// 204 153 0
#define BMP_MOUNTAIN			90		// 153 102 0
#define BMP_JUNGLE	   			72		// 0 102 0
#define BMP_FOREST				114		// 51 153 0
#define BMP_FORESTED_HILL		110		// 0 153 102
#define BMP_FIELD      			150		// 51 204 0
#define BMP_PLAIN       		204		// 204 255 0
#define BMP_LAND 				171		// 204 204 153
#define BMP_MOUNTAIN_PEAK		172		// 204 204 204
#define BMP_BEACH				210		// 255 255 0
#define BMP_PLATEAU				213		// 255 255 153
#define BMP_ROAD				30		// 255 0 0
#define BMP_SWAMP				123		// 102 153 153
#define BMP_REEF				174		// 255 204 0
#define BMP_FORESTED_MOUNTAIN	126		// 153 153 0
#define BMP_ZONE_BORDER			34		// 255 0 240
#define BMP_ZONE_INSIDE			214		// 255 255 204
#define BMP_ZONE_EXIT			143		// 255 153 255
#define BMP_BRIDGE				6		// 51 0 0
#define BMP_PORT				18		// 153 0 0
#define BMP_FORD				191		// 51 255 255
#define BMP_DESERT				176		// 255 204 102


#define Y_SIZE					WILD_Y
#define X_SIZE					WILD_X

/* extern globals */
extern ROOM_DATA		*wild[WILD_HASH][WILD_HASH];
extern TIME_INFO_DATA	time_info;

/* extern funcs */
PSTONE_DATA *get_pstone_by_coord( COORD_DATA *coord );
WEATHER_DATA *get_coord_weather(COORD_DATA *coord);
ROOM_AFFECT *get_room_aff_bitv(ROOM_DATA *pRoom, int bitv);
int		get_sect( COORD_DATA *coord );
int		calc_angle( int chX, int chY, int lmX, int lmY, int *ipDistan );
int		angle_to_dir( int iAngle );
int		distance_to_index( int iDistance );
double	distance( int chX, int chY, int lmX, int lmY );
void	list_obj_to_char(OBJ_DATA *list, CHAR_DATA *ch, int mode, int show);
void	list_char_to_char(CHAR_DATA *list, CHAR_DATA *ch);
void	list_building_to_char(BUILDING_DATA *bldlist, CHAR_DATA *ch);
void	list_ship_to_char( SHIP_DATA *Ships, CHAR_DATA *ch );

/* local globals */
char bmpmap[Y_SIZE][X_SIZE+3];

/* =============================================================== */

/* =================================================== */
/*                      M A P P A                      */
/* =================================================== */
int char_to_sect(char charsect)
{
	int nsect = 0;

	switch (charsect)
	{
	case 'v': nsect = SECT_FIELD;				break;
	case 'b': nsect = SECT_LAND;				break;
	case 'f': nsect = SECT_FOREST;				break;
	case 'F': nsect = SECT_JUNGLE;				break;
	case 'c': nsect = SECT_HILLS;				break;
	case 'C': nsect = SECT_FORESTED_HILL;		break;
	case 'm': nsect = SECT_MOUNTAIN;			break;
	case 'M': nsect = SECT_MOUNTAIN_PEAK;		break;
	case 'T': nsect = SECT_FORESTED_MOUNTAIN;	break;
	case 'A': nsect = SECT_PLATEAU;				break;
	case 'p': nsect = SECT_PLAIN;				break;
	case 'd': nsect = SECT_SWAMP;				break;
	case 'Z': nsect = SECT_REEF;				break;
	case 'a': nsect = SECT_RIVER;				break;
	case 'k': nsect = SECT_RIVER_NAVIGABLE;		break;
	case '~': case '_': nsect = SECT_SEA;		break;
	case 's': nsect = SECT_BEACH;				break;
	case 'G': nsect = SECT_ARTIC;				break;
	case ':': nsect = SECT_ROAD;				break;
	case '=': nsect = SECT_BRIDGE;				break;
	case '.': nsect = SECT_COAST;				break;
	case 'I': nsect = SECT_ZONE_BORDER;			break;
	case '@': nsect = SECT_WILD_EXIT;			break;
	case ' ': nsect = SECT_ZONE_INSIDE;			break;		/* hmmmm */
	case '"': nsect = SECT_CITY;				break;
	case 'H': nsect = SECT_PORT;				break;
	case 'u': nsect = SECT_FORD;				break;
	case 'D': nsect = SECT_DESERT;				break;
	case '-': nsect = SECT_SHALLOWS;			break;
		
	default:
		log("SYSERR: char_to_sect() unknown symbol sector '%c'\n", charsect);
		nsect = SECT_FIELD;
		break;
	}

	return (nsect);
}

char sect_to_char(int sectnum)
{
	char nsect;

	switch (sectnum)
	{
	case SECT_FIELD:				nsect = 'v'; 	break;
	case SECT_LAND:					nsect = 'b'; 	break;
	case SECT_FOREST:				nsect = 'f';	break;
	case SECT_JUNGLE:				nsect = 'F';	break;
	case SECT_HILLS:				nsect = 'c';	break;
	case SECT_FORESTED_HILL:		nsect = 'C';	break;
	case SECT_MOUNTAIN:				nsect = 'm';	break;
	case SECT_MOUNTAIN_PEAK:		nsect = 'M';	break;
	case SECT_FORESTED_MOUNTAIN:	nsect = 'T';	break;
	case SECT_PLATEAU:				nsect = 'A';	break;
	case SECT_PLAIN:				nsect = 'p';	break;
	case SECT_SWAMP:				nsect = 'd';	break;
	case SECT_REEF:					nsect = 'Z';	break;
	case SECT_RIVER:				nsect = 'a';	break;
	case SECT_RIVER_NAVIGABLE:		nsect = 'k';	break;
	case SECT_SEA:					nsect = '_';	break;
	case SECT_BEACH:				nsect = 's';	break;
	case SECT_ARTIC:				nsect = 'G';	break;
	case SECT_ROAD:					nsect = ':' ;	break;
	case SECT_BRIDGE:				nsect = '=';	break;
	case SECT_COAST:				nsect = '.';	break;
	case SECT_ZONE_BORDER:			nsect = 'I';	break;
	case SECT_ZONE_INSIDE:			nsect = ' ';	break;
	case SECT_WILD_EXIT:			nsect = '@';	break;
	case SECT_CITY:					nsect = '"';	break;
	case SECT_PORT:					nsect = 'H';	break;
	case SECT_FORD:					nsect = 'u';	break;
	case SECT_SHALLOWS:				nsect = '-';	break;
	case SECT_DESERT:				nsect = 'D';	break;

	default:
		log( "SYSERR: sect_to_char() unknown sector." );
		nsect = 'v';
		break;
	}

	return (nsect);
}


char *show_sect(CHAR_DATA *ch, char sect, COORD_DATA *coord, bool plain, int iDist)
{
	ROOM_DATA *pRoom;
	char simbmap[20];
	static char simbolo[20];
	int nSect = char_to_sect(sect);

	simbolo[0] = '\0';

	if (PRF_FLAGGED(ch, PRF_WILDBLACK))
		strcpy(simbmap, terrain_type[nSect]->mapbw);
	else if (PRF_FLAGGED(ch, PRF_WILDTEXT))
		strcpy(simbmap, terrain_type[nSect]->map2);
	else
		strcpy(simbmap, terrain_type[nSect]->map);
	
	if (plain || PRF_FLAGGED(ch, PRF_WILDBLACK))
	{
		strcpy(simbolo, simbmap);
		return (simbolo);
	}

	/* show portal stones */
	if (get_pstone_by_coord(coord))
	{
		strcpy(simbolo, "&W&1&&&0");
		return (simbolo);
	}

	if (!(pRoom = get_wild_room(coord)))
	{
		strcpy(simbolo, simbmap);
		return (simbolo);
	}

	// Edifici - cambia il colore di background
	if ( (pRoom->buildings || ROOM_FLAGGED(pRoom, ROOM_BUILDING_WORKS)) && SECT(pRoom) != SECT_CITY)
	{
		strcpy(simbolo, "&M");
	}
	// Incantesimi - cambia il colore di background
	else if (pRoom->affections)
	{
		if (!get_room_aff_bitv(pRoom, RAFF_CAMP))
			strcpy(simbolo, "&Y");
	}

	// statically show ppl and mobs on map */
	if (pRoom->people && iDist <= RADIUS_SMALL)
	{
		if (pRoom->people->next_in_room)
		{
			CHAR_DATA *ppl;
			int count = 0, seen = 0;

			for (ppl = pRoom->people; ppl; ppl = ppl->next_in_room)
			{
				if (CAN_SEE(ch, ppl))
				{
					count++;
					seen = IS_NPC(ppl) ? 1 : 2;
				}
			}

			if (count)
			{
				// she can see only one.. mob or pg?
				if (count == 1 && seen)
				{
					// mob in red
					if (seen == 1)
						strcpy(simbmap, "&b&1#");
					// ppl in magenta
					else
						strcpy(simbmap, "&b&5#");
				}
				// 2 or more.. blue #..
				else
					strcpy(simbmap, "&b&4#");
			}
		}
		// only one..
		else if (CAN_SEE(ch, pRoom->people))
		{
			// mob in red
			if (IS_NPC(pRoom->people))
				strcpy(simbmap, "&b&1#");
			// ppl in magenta
			else
				strcpy(simbmap, "&b&5#");
		}
	}

	strcat(simbolo, simbmap);
	strcat(simbolo, "&0");

	return (simbolo);
}

void show_ship_on_map( CHAR_DATA *ch )
{
	COORD_DATA coord;
	ROOM_DATA *pRoom;
	SHIP_DATA *ship;
	char sbuf[MAX_STRING_LENGTH];
	int x, y, xmax, ymax, xmin, ymin;
	int radius = 7;
	int iAngle, iDist, ames, dmes;
	bool showed = FALSE;

	/* some redundant checks */
	if ( !ch || !ch->desc || !ON_DECK(ch) )
		return;

	x = y = xmax = ymax = xmin = ymin = 0;

	y		= GET_Y(ch->in_ship);
	x		= GET_X(ch->in_ship);

	ymin	= y - MOD_Y;
	ymax	= y + MOD_Y;
	xmin	= x - MOD_X;
	xmax	= x + MOD_X;

	for ( coord.y = ymin; coord.y < ymax + 1; coord.y++ )
	{
		for ( coord.x = xmin; coord.x < xmax + 1; coord.x++ )
		{
			if ( coord.x == x && coord.y == y )
				continue;

			if ( !( pRoom = get_wild_room(&coord) ) )
				continue;

			if ( !pRoom->ships )
				continue;

			iAngle = calc_angle( x, y, coord.x, coord.y, &iDist );

			if ( iDist > radius )
				continue;

			ames = angle_to_dir( iAngle );
			dmes = distance_to_index( iDist );

			for ( ship = pRoom->ships; ship; ship = ship->next_in_room )
			{
				if ( SHIP_FLAGGED(ship, SHIP_IN_PORT) )
					continue;

				if ( SHIP_FLAGGED(ship, SHIP_IN_COURSE) )
					sprintf(sbuf, "sailing to the %s", dirs[ship->direction]);
				else if ( SHIP_FLAGGED(ship, SHIP_COMBAT) )
					sprintf(sbuf, "in a combat");
				else if ( SHIP_FLAGGED(ship, SHIP_AT_ANCHOR) )
					sprintf(sbuf, "at anchor");
				else if ( SHIP_FLAGGED(ship, SHIP_SAIL) )
					sprintf(sbuf, "sailing free");

				ch_printf(ch, "At %d o'clock, %s you see %s %s ship, %s.\r\n",
					ames, dist_descr[(int) dmes],
					UAN(ship->type->name), ship->type->name, sbuf);

				showed = TRUE;
			}
		}
	}

	if ( !showed )
		send_to_char("You don't see anything nearby.\r\n", ch);
}

void show_map(CHAR_DATA *ch)
{
	COORD_DATA curr, real;
	FERRY_DATA *pFerry;
	char map[MAX_STRING_LENGTH] = "\0";
	int chy, chx;
	int x, y, xmax, ymax, xmin, ymin;
	int iDist, radius, modx, mody;

	/* some redundant checks */
	if (!ch || !ch->desc || (!IN_WILD(ch) && !ON_DECK(ch) && !IN_FERRY(ch)))
		return;

	if (IN_FERRY(ch))
		 pFerry = get_ferry(ch->in_room->extra_data->vnum);

	x = y = xmax = ymax = xmin = ymin = 0;

	if ( ON_DECK(ch) )
	{
		chy		= GET_Y(ch->in_ship);
		chx		= GET_X(ch->in_ship);
	}
	else if ( IN_FERRY(ch) )
	{
		chy		= GET_Y(pFerry);
		chx		= GET_X(pFerry);
	}
	else
	{
		chy		= GET_Y(ch);
		chx		= GET_X(ch);
	}

	if (PRF_FLAGGED(ch, PRF_WILDSMALL))
	{
		mody	= MOD_SMALL_Y;
		modx	= MOD_SMALL_X;
		radius	= RADIUS_SMALL;
	}
	else
	{
		WEATHER_DATA *sky;
		
		if (ON_DECK(ch))
			sky = get_coord_weather(ch->in_ship->in_room->coord);
		else if (IN_FERRY(ch))
			sky = get_coord_weather(pFerry->in_room->coord);
		else
			sky = get_coord_weather(ch->in_room->coord);

		mody	= MOD_Y;
		modx	= MOD_X;
		radius	= RADIUS_BASE;
	
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
			
			radius = URANGE(RADIUS_SMALL, radius, RADIUS_BASE);
		}
	}

	y		= abs(chy - (chy / MAP_SIZE * MAP_SIZE)) + MAP_SIZE;
	x		= abs(chx - (chx / MAP_SIZE * MAP_SIZE)) + MAP_SIZE;

	ymin	= y - mody;
	ymax	= y + mody;
	xmin	= x - modx;
	xmax	= x + modx;

	real.y = chy - mody;
	
	if (IS_IMMORTAL(ch))
		sprintf(map, "&0[%d %d] [R:%d]\r\n", chy, chx, radius );
	else
		strcpy(map, "&0\r\n");

	for (curr.y = ymin; curr.y < ymax + 1; curr.y++, real.y++)
	{
		real.x = chx - modx;

		for (curr.x = xmin; curr.x < xmax + 1; curr.x++, real.x++)
		{
			if ((iDist = distance(x, y, curr.x, curr.y)) > radius)
				strcat( map, " " );
			else if (curr.x == x && curr.y == y)
			{
				if (PRF_FLAGGED(ch, PRF_WILDBLACK))
					strcat(map, "#");
				else
					strcat(map, "&b&7#&0");
			}
			else
				strcat(map, show_sect(ch, ch->player_specials->wildmap[curr.y][curr.x], &real, FALSE, iDist));
		}

		strcat(map, "&0\r\n");
		send_to_char(map, ch);
		*map = '\0';
	}
}

void look_at_wild(CHAR_DATA *ch, bool quick)
{
	if (!ch || !ch->desc || (!IN_WILD(ch) && !ON_DECK(ch) && !IN_FERRY(ch)))
		return;
	
	if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch))
	{
		if (!quick)
			send_to_char("It is pitch black...\r\n", ch);
		return;
	}
	
	if (AFF_FLAGGED(ch, AFF_BLIND))
	{
		if (!quick)
			send_to_char("You see nothing but infinite darkness...\r\n", ch);
		return;
	}

	if (!quick)
	{
		send_to_char("\r\n&b&7", ch);
		send_to_char(ROOM_NAME(ch), ch);
		send_to_char(CCNRM(ch, C_NRM), ch);
		send_to_char("\r\n", ch);
	}

	if (SECT(ch->in_room) != SECT_CITY)
		show_map(ch);

	if (quick)
		return;

	if (ch->in_room->portal_stone)
		ch_printf(ch, "%s\r\n", ch->in_room->portal_stone->description);
	if (ch->in_room->ferryboat)
		send_to_char("&b&4You see a ferryboat.&0\r\n", ch);

	/* now list characters & objects */
	list_building_to_char(ch->in_room->buildings, ch);
	list_ship_to_char(ch->in_room->ships, ch);
	list_vehicle_to_char(ch->in_room->vehicles, ch);
	list_obj_to_char(ch->in_room->first_content, ch, 0, FALSE);
	list_char_to_char(ch->in_room->people, ch);
	send_to_char(CCNRM(ch, C_NRM), ch);
}


/* =================================================== */
/*                   M A P P A   B M P                 */
/* =================================================== */

/*
 * scrive sul file mappa.wls la mappa ottenuta
 * interpretando il file bitmap
 *
 * va da Y_SIZE - 1 a 0 xche' i bitmap sono "rovesciati"
 */
void bmp_to_txt(void)
{
	FILE *fl;
	char fname[256];
	register int y;
	
	sprintf(fname, "%smappa.wls", WLS_PREFIX);
	if (!(fl = fopen(fname, "w")))
	{
		log("Errore in apertura file %s", fname);
		return;
	}
	
	for (y = Y_SIZE - 1; y >= 0; y--)
		fprintf(fl, "%s\n", bmpmap[y]);
	
	fprintf(fl, "$\n");
	fclose(fl);
}

/*
 * legge la mappa in formato bitmap e interpreta i colori
 * traducendoli nei simboli dei settori.
 */
void parse_bmp(void)
{
	FILE *fl;
	char fname[256];
	register int x, y, c;
	fpos_t pos;
	
	sprintf(fname, "%slyonesse.bmp", WLS_PREFIX);
	
	/* Si legge le descrizioni base dei vari settori */
	if (!(fl = fopen(fname, "rb")))
	{
		if (errno != ENOENT)
			log("fatal error opening: %s", fname);
		else
			log("%s file does not exist", fname);
		return;
	}
	
	/*
	 * salta i primi 1077 caratteri del file bmp
	 * che contengono l'header dell'immagine
	 */
	pos = 1078;
	if (fsetpos(fl, &pos) != 0)
		return;
	
	/* azzera bmpmap[][] */
	for (y = 0; y < Y_SIZE; y++)
		strcpy( bmpmap[y], "\0" );

	y = 0;
	
	/* legge il primo carattere della mappa */
	c = fgetc(fl);
	
	/* loop per tutta la mappa */
	while (!feof(fl))
	{
		/* loop per una riga di X_SIZE */
		for (x = 0; x < X_SIZE && !feof(fl); x++)
		{
			switch (c)
			{
			case BMP_SEA:				strcat(bmpmap[y], "_");	break;
			case BMP_SHALLOWS:			strcat(bmpmap[y], "-");	break;
			case BMP_RIVER:				strcat(bmpmap[y], "a");	break;
			case BMP_NAVIGABLE_RIVER:	strcat(bmpmap[y], "k");	break;
			case BMP_ARTIC:				strcat(bmpmap[y], "G");	break;
			case BMP_JUNGLE:			strcat(bmpmap[y], "F");	break;
			case BMP_FOREST:			strcat(bmpmap[y], "f");	break;
			case BMP_FORESTED_HILL:		strcat(bmpmap[y], "C");	break;
			case BMP_FIELD:				strcat(bmpmap[y], "v");	break;
			case BMP_PLAIN:				strcat(bmpmap[y], "p");	break;
			case BMP_LAND:				strcat(bmpmap[y], "b");	break;
			case BMP_DESERT:			strcat(bmpmap[y], "D");	break;
			case BMP_BEACH:				strcat(bmpmap[y], "s");	break;
			case BMP_PLATEAU:			strcat(bmpmap[y], "A");	break;
			case BMP_COASTLINE:			strcat(bmpmap[y], ".");	break;
			case BMP_ROAD:				strcat(bmpmap[y], ":");	break;
			case BMP_SWAMP:				strcat(bmpmap[y], "d");	break;
			case BMP_MOUNTAIN:			strcat(bmpmap[y], "m");	break;
			case BMP_HILL:				strcat(bmpmap[y], "c");	break;
			case BMP_MOUNTAIN_PEAK:		strcat(bmpmap[y], "M");	break;
			case BMP_REEF:				strcat(bmpmap[y], "Z");	break;
			case BMP_FORESTED_MOUNTAIN:	strcat(bmpmap[y], "T");	break;
			case BMP_BRIDGE:			strcat(bmpmap[y], "=");	break;
			case BMP_PORT:				strcat(bmpmap[y], "H");	break;
			case BMP_FORD:				strcat(bmpmap[y], "u");	break;
			case BMP_ZONE_BORDER:		strcat(bmpmap[y], "I");	break;
			case BMP_ZONE_INSIDE:		strcat(bmpmap[y], " ");	break;
			case BMP_ZONE_EXIT:			strcat(bmpmap[y], "@");	break;

			default:
				strcat(bmpmap[y], "E");
				break;
			}
			c = fgetc(fl);
		}
		
		if (++y > Y_SIZE)
		{
			log("SYSERR: parse_bmp(): out of Y space");
			fclose(fl);
			return;
		}
	}
	
	/* chiude il flusso */
	fclose(fl);
	
	/* scrive la mappa in formato .txt */
	bmp_to_txt();
}


ACMD(do_bmpmap)
{
	parse_bmp();
	send_to_char(OK, ch);
}

/* =============================================================== */
/* Funzioni di Generazione delle Mappe 20x20                       */
/* =============================================================== */
int get_longline(FILE *fl, char *buf)
{
	char temp[WILD_X+3];
	int lines = 0;
	
	do
	{
		fgets(temp, WILD_X+3, fl);
		if (feof(fl))
			return (0);
		lines++;
	} while (*temp == '*' || *temp == '\n');
	
	temp[strlen(temp) - 1] = '\0';
	strcpy(buf, temp);
	return (lines);
}


void create_submaps(void)
{
	FILE *fp, *fl;
	char fmapname[128], fname[80];
	char line[MAP_SIZE][WILD_X+5];
	int lastx, lasty, x, y, mapx, mapy;
	
	lastx = lasty = x = y = mapx = mapy = 0;
	
	sprintf(fmapname, "%smappa.wls", WLS_PREFIX);
	if (!(fp = fopen(fmapname, "r")))
	{
		log("SYSERR: unable to open file %s", fmapname);
		return;
	}
	
	for ( ; ; )
	{
		for (y = 0; y < MAP_SIZE; y++)
			get_longline(fp, line[y]);
		
		for ( ; ; )
		{
			sprintf(fname, "%smap/%d-%d.map", WLS_PREFIX, lasty, mapx);
			if (!(fl = fopen(fname, "w")))
			{
				log("SYSERR: impossibile creare file mappa");
				fclose(fp);
				return;
			}
			
			for (y = mapy; y < mapy + MAP_SIZE; y++)
			{
				for (x = mapx; x < mapx + MAP_SIZE; x++)
					fprintf(fl, "%c", line[y][x]);
				fprintf(fl, "\n");
			}
			
			fclose (fl);
			
			mapy = 0;
			mapx += MAP_SIZE;
			if (mapx >= (WILD_X-1))
			{
				mapx = 0;
				break;
			}
		}
		
		lasty += MAP_SIZE;
		if (lasty >= (WILD_Y-1))
			break;
		
		fclose(fl);
	}
	fclose(fp);
}

ACMD(do_remap)
{
	parse_bmp();
	create_submaps();
	send_to_char(OK, ch);
}

