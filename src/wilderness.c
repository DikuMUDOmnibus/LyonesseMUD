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
 * File: wilderness.c                                                     *
 *                                                                        *
 * Wilderness with dynamic rooms code                                     *
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
#include "constants.h"

/* external funcs */
PSTONE_DATA *get_pstone_by_coord( COORD_DATA *coord );
int		char_to_sect( char charsect );
char	sect_to_char( int sectnum );
void	show_full_weath_map( CHAR_DATA *ch );
void	load_life(WILD_DATA *wd);
void	save_life(WILD_DATA *wd);

/* local functions */
WILD_DATA *load_wildmap_sector(COORD_DATA *coord);
WILD_DATA *get_wd(COORD_DATA *coord);
WILD_DATA *wild_sector(int y, int x);
void	check_wild_move(CHAR_DATA *ch, VEHICLE_DATA *vehicle, SHIP_DATA *ship, FERRY_DATA *ferry, ROOM_DATA *pRoomFrom, ROOM_DATA *pRoomTo);
void	wild_map_load(CHAR_DATA *ch, COORD_DATA *coord);
void	remove_exit_to_room(ROOM_DATA *wRoom);
void	update_wild_life(int y, int x, int oper);

/* Globals */
ROOM_DATA	*wild[WILD_HASH][WILD_HASH];
WILD_DATA	*kWorld[WDATA_HASH][WDATA_HASH];
int			loaded_rooms = 0;

/* Global lists */
WILD_REMOVE		*wild_remove_list = NULL;

/* ************************************************************** */

void free_ex_descriptions(EXTRA_DESCR *head)
{
	EXTRA_DESCR *thised, *next_one;
	
	if (!head)
	{
		log("free_ex_descriptions: NULL pointer or NULL data.");
		return;
	}
	
	for (thised = head; thised; thised = next_one)
	{
		next_one = thised->next;
		if (thised->keyword)
			free(thised->keyword);
		if (thised->description)
			free(thised->description);
		free(thised);
	}
}

int free_room_strings(ROOM_DATA *room)
{
	/* Free descriptions. */
	if (room->name)
		free(room->name);
	if (room->description)
		free(room->description);

	if (room->ex_description)
		free_ex_descriptions(room->ex_description);
	
	return (TRUE);
}

void free_room(ROOM_DATA *room)
{
	EXIT_DATA *pexit, *pex_next = NULL;

	free_room_strings(room);
	
	/* Free exits */
	for ( pexit = room->first_exit; pexit; pexit = pex_next )
	{
		pex_next = pexit->next;
		extract_exit( room, pexit );
	}
}

/* *************************************************************** */
/*            W I L D   S E C T O R S   H A N D L I N G            */
/* *************************************************************** */

void update_sect_file( WILD_DATA *wd )
{
	FILE *fp;
	char fname[128];
	register int fy;

	if ( !wd )
		return;

	/* update map file */
	sprintf( fname, "%smap/%d-%d.map", WLS_PREFIX, wd->coord->y, wd->coord->x );
	if ( !( fp = fopen( fname, "w" ) ) )
	{
		log( "SYSERR: cannot create map file %d %d", wd->coord->y, wd->coord->x );
		return;
	}

	for ( fy = 0; fy < MAP_SIZE; fy++ )
		fprintf( fp, "%s\n", wd->ws_map[fy] );
	fclose(fp);
}

int get_sect( COORD_DATA *coord )
{
	WILD_DATA *wd;
	COORD_DATA csect;

	csect.y = coord->y / MAP_SIZE * MAP_SIZE;
	csect.x = coord->x / MAP_SIZE * MAP_SIZE;

	if ( !( wd = wild_sector( csect.y, csect.x ) ) )
		wd = load_wildmap_sector( &csect );

	csect.y = abs( coord->y - csect.y );
	csect.x = abs( coord->x - csect.x );

	return ( char_to_sect( wd->ws_map[csect.y][csect.x] ) );
}


void put_sect( int y, int x, int nSect, bool upMap )
{
	WILD_DATA *wd;
	int ty, tx, fy, fx;

	ty = y / MAP_SIZE * MAP_SIZE;
	tx = x / MAP_SIZE * MAP_SIZE;

	if ( !( wd = wild_sector( ty, tx ) ) )
	{
		log ( "SYSERR: wild sector %d %d not loaded.", y, x );
		return;
	}

	fy = abs( y - ty );
	fx = abs( x - tx );

	wd->ws_map[fy][fx] = sect_to_char( nSect );

	if ( upMap )
		update_sect_file( wd );
}


WILD_DATA *get_wd(COORD_DATA *coord)
{
	WILD_DATA *wd = NULL;
	int y, x, wy, wx;

	if (!coord)
		return (NULL);

	y	= coord->y / MAP_SIZE * MAP_SIZE;
	x	= coord->x / MAP_SIZE * MAP_SIZE;
	wy	= y % WDATA_HASH;
	wx	= x % WDATA_HASH;

	for (wd = kWorld[wy][wx]; wd; wd = wd->next)
	{
		if (wd->coord->y == y && wd->coord->x == x)
			break;
	}

	return (wd);
}

WILD_DATA *wild_sector(int y, int x)
{
	WILD_DATA *wd;
	int wy = y % WDATA_HASH;
	int wx = x % WDATA_HASH;

	for (wd = kWorld[wy][wx]; wd; wd = wd->next)
	{
		if (wd->coord->y == y && wd->coord->x == x)
			break;
	}

	return (wd);
}

void unload_wildmap_sector( WILD_DATA *wd )
{
	WILD_DATA *temp;
	int y = wd->coord->y % WDATA_HASH;
	int x = wd->coord->x % WDATA_HASH;

	// save life data -- checks are inside save_life()
	save_life(wd);

	REMOVE_FROM_LIST( wd, kWorld[y][x], next );
	DISPOSE(wd->coord);
	DISPOSE(wd);
}


WILD_DATA *load_wildmap_sector( COORD_DATA *coord )
{
	FILE *fp;
	WILD_DATA *wd;
	char fname[128], line[MAP_SIZE+3];
	int y, x, ww, wy, wx;
	
	y = coord->y / MAP_SIZE * MAP_SIZE;
	x = coord->x / MAP_SIZE * MAP_SIZE;

	if ( ( wd = wild_sector(y, x) ) )
		return (wd);

	CREATE( wd, WILD_DATA, 1 );
	CREATE( wd->coord, COORD_DATA, 1 );
	wd->coord->y		= y;
	wd->coord->x		= x;

	/* read map file */
	sprintf( fname, "%smap/%d-%d.map", WLS_PREFIX, y, x );
	if ( !( fp = fopen( fname, "r" ) ) )
	{
		log( "SYSERR: load_wildmap_sector() : cannot find map file %d-%d.map", y, x );
		return (NULL);
	}
	
	for ( ww = 0; ww < MAP_SIZE && !feof(fp); ww++ )
	{
		get_line( fp, line );
		strcpy( wd->ws_map[ww], line );
	}
	fclose(fp);

	/* add to hash table */
	wy = y % WDATA_HASH;
	wx = x % WDATA_HASH;

	wd->next			= kWorld[wy][wx];
	kWorld[wy][wx]		= wd;

	// load life data, or clone base_life_table for wd
	load_life(wd);

	return (wd);
}


/* *************************************************************** */
/*                          C H E C K S                            */
/* *************************************************************** */

bool check_coord(int y, int x)
{
	if (y < 0 || y >= WILD_Y) return (FALSE);
	if (x < 0 || x >= WILD_X) return (FALSE);

	return (TRUE);
}

bool invalid_sect( COORD_DATA *coord )
{
	int nSect = get_sect( coord );

	if ( nSect == SECT_ZONE_BORDER )
		return ( TRUE );

	if ( nSect == SECT_ZONE_INSIDE )
		return ( TRUE );

	return ( FALSE );
}

/* *************************************************************** */
/*                  R O O M   H A N D L I N G                      */
/* *************************************************************** */

/* find an existing wild room located at "coord" */
ROOM_DATA *get_wild_room( COORD_DATA *coord )
{
	ROOM_DATA *wRoom;
	int xHash, yHash;

	yHash = coord->y % WILD_HASH;
	xHash = coord->x % WILD_HASH;

	for ( wRoom = wild[yHash][xHash]; wRoom; wRoom = wRoom->next )
	{
		if ( wRoom->coord->y == coord->y && wRoom->coord->x == coord->x )
			return ( wRoom );
	}

	return (NULL);
}

/* create a wild room located at "coord" */
ROOM_DATA *create_wild_room( COORD_DATA *coord, bool Static )
{
	ROOM_DATA *wRoom;
	int xHash, yHash;

	/* sanity checks */
	if ((wRoom = get_wild_room(coord)) != NULL)
	{
		if (Static && !ROOM_FLAGGED(wRoom, ROOM_WILD_STATIC))
		{
			SET_BIT(ROOM_FLAGS(wRoom), ROOM_WILD_STATIC);
			SET_BIT(ROOM_FLAGS(wRoom), ROOM_NOMAGIC);
		}

		return (wRoom);
	}

	/* create the new room */
	wRoom = new_room();

	/* Initialize vars */
	wRoom->sector_type	= get_sect(coord);
	wRoom->zone			= WILD_ZONE;

	CREATE(wRoom->coord, COORD_DATA, 1);
	wRoom->coord->y		= coord->y;
	wRoom->coord->x		= coord->x;

	if (Static)
	{
		SET_BIT(ROOM_FLAGS(wRoom), ROOM_WILD_STATIC);
		SET_BIT(ROOM_FLAGS(wRoom), ROOM_NOMAGIC);
	}

	/* check for portal stone to place in */
	wRoom->portal_stone	= get_pstone_by_coord( coord );

	/* add to the hash table */
	yHash				= coord->y % WILD_HASH;
	xHash				= coord->x % WILD_HASH;
	wRoom->next			= wild[yHash][xHash];
	wild[yHash][xHash]	= wRoom;

	update_wild_life(coord->y, coord->x, WS_ADD_ROOM);

	loaded_rooms++;
	return (wRoom);
}

/* delete a wild room */
int destroy_wild_room( ROOM_DATA *wRoom )
{
	ROOM_DATA *temp;
	int xHash, yHash;

	/* sanity checks */
	if ( !wRoom || !IS_WILD( wRoom ) )		return (0);
	if ( !wRoom->coord )					return (0);
	if ( !get_wild_room(wRoom->coord) )		return (0);
	if ( wRoom->buildings )					return (0);
	if ( wRoom->people )					return (0);
	if ( wRoom->ships )						return (0);
	if ( wRoom->vehicles )					return (0);
	if ( wRoom->ferryboat )					return (0);
	if ( wRoom->first_content )				return (0);

	/* remove all exits to this room */
	remove_exit_to_room( wRoom );

	update_wild_life( wRoom->coord->y, wRoom->coord->x, WS_REM_ROOM );

	/* remove from the hash table */
	yHash = wRoom->coord->y % WILD_HASH;
	xHash = wRoom->coord->x % WILD_HASH;
	REMOVE_FROM_LIST( wRoom, wild[yHash][xHash], next );

	/* destroy room */
	free_room(wRoom);

	/* nullify all pointers */
	wRoom->buildings		= NULL;
	wRoom->ships			= NULL;
	wRoom->vehicles			= NULL;
	wRoom->ferryboat		= NULL;
	wRoom->first_content	= NULL;
	wRoom->last_content		= NULL;
	wRoom->people			= NULL;
	wRoom->ex_description	= NULL;
	wRoom->first_exit		= NULL;
	wRoom->last_exit		= NULL;
	wRoom->func				= NULL;
	wRoom->coord			= NULL;
	wRoom->next				= NULL;

	DISPOSE( wRoom );

	loaded_rooms--;
	return (1);
}

void wild_remove_update( void )
{
	WILD_REMOVE *wrem, *wrem_next, *temp;

	if ( !wild_remove_list )
		return;
		
	for ( wrem = wild_remove_list; wrem; wrem = wrem_next )
	{
		wrem_next = wrem->next;
		
		if ( wrem->timer-- <= 0 )
		{
			if ( destroy_wild_room( wrem->wRoom ) )
			{
				REMOVE_FROM_LIST( wrem, wild_remove_list, next );
				DISPOSE( wrem );
			}
			else
			{
				if ( wrem->wRoom )
				{
					log( "SYSERR: Unable to remove wild room %d %d -- will try next pass.",
						wrem->wRoom->coord->y, wrem->wRoom->coord->x );
				}
				wrem->timer = 1;
			}
		}
	}
}

void wild_remove_enqueue( ROOM_DATA *wRoom )
{
	WILD_REMOVE *wrem;

	CREATE( wrem, WILD_REMOVE, 1 );

	wrem->wRoom			= wRoom;
	wrem->timer			= 3;		// 90+30 seconds

	if ( SECT(wRoom) == SECT_SEA )
		wrem->timer	= 1;

	// add to the list
	wrem->next			= wild_remove_list;
	wild_remove_list	= wrem;

	SET_BIT( ROOM_FLAGS( wRoom ), ROOM_WILD_REMOVE );
}

void wild_remove_dequeue( ROOM_DATA *wRoom )
{
	WILD_REMOVE *wrem, *wrem_next, *temp;

	if ( !wild_remove_list )
		return;
	if ( !wRoom || !IS_WILD( wRoom) )
		return;
	if ( !wRoom->coord )
		return;
	if ( !ROOM_FLAGGED( wRoom, ROOM_WILD_REMOVE ) )
		return;

	for ( wrem = wild_remove_list; wrem; wrem = wrem_next )
	{
		wrem_next = wrem->next;

		if ( wrem->wRoom == wRoom )
		{
			REMOVE_BIT( ROOM_FLAGS(wRoom), ROOM_WILD_REMOVE );
			REMOVE_FROM_LIST( wrem, wild_remove_list, next );
			DISPOSE( wrem );
		}
	}
}

void wild_check_for_remove( ROOM_DATA *wRoom )
{
	if ( !wRoom || !IS_WILD( wRoom ) )
		return;
	if ( ROOM_FLAGGED( wRoom, ROOM_WILD_STATIC ) ||
	     ROOM_FLAGGED( wRoom, ROOM_WILD_REMOVE ) )
		return;
	if ( !wRoom->coord )
		return;
	if ( !get_wild_room( wRoom->coord ) )
		return;
	if ( wRoom->people )
		return;
	if ( wRoom->first_content )
		return;
	if ( wRoom->buildings )
		return;
	if ( wRoom->ships )
		return;
	if ( wRoom->vehicles )
		return;
	if ( wRoom->ferryboat )
		return;

	wild_remove_enqueue( wRoom );
}

/* *************************************************************** */
/*                  E X I T   H A N D L I N G                      */
/* *************************************************************** */

void remove_exit_to_room(ROOM_DATA *wRoom)
{
	EXIT_DATA *pexit, *p_next, *nexit;
	ROOM_DATA *nRoom;

	for (pexit = wRoom->first_exit; pexit; pexit = p_next)
	{
		p_next	= pexit->next;

		nRoom	= pexit->to_room;
		nexit	= pexit->rexit;

		if (nRoom && nexit)
			extract_exit(nRoom, nexit);
	}
}

void setup_wild_static( ROOM_DATA *wRoom, ROOM_DATA *zRoom )
{
	ROOM_DATA *nRoom;
	COORD_DATA around;
	int dir;

	for ( around.y = wRoom->coord->y - 1; around.y < wRoom->coord->y + 2; around.y++ )
	{
		for ( around.x = wRoom->coord->x - 1; around.x < wRoom->coord->x + 2; around.x++ )
		{
			if ( invalid_sect( &around ) )
				continue;

			if ( ( nRoom = get_wild_room( &around ) ) == NULL )
			{
				nRoom = create_wild_room( &around, TRUE );
			}
			else
			{
				SET_BIT( ROOM_FLAGS( nRoom ), ROOM_WILD_STATIC );
				SET_BIT( ROOM_FLAGS( nRoom ), ROOM_NOMAGIC );
			}
		}
	}

	for ( dir = 0; dir < NUM_OF_DIRS; dir++ )
	{
		if ( dir == UP || dir == DOWN )
			continue;

		if ( ( nRoom = find_wild_room( wRoom, dir, FALSE ) ) )
		{
			if ( !get_exit_to_coord( wRoom, dir, nRoom->coord ) )
				make_exit( wRoom, nRoom, dir );
			if ( !get_exit_to( nRoom, rev_dir[dir], zRoom->number ) )
				make_exit( nRoom, zRoom, rev_dir[dir] );
		}
	}
}

/* *************************************************************** */
/*                      D I R E C T I O N S                        */
/* *************************************************************** */

COORD_DATA *make_step( COORD_DATA *target, int dir )
{
	COORD_DATA *coord;

	CREATE(coord, COORD_DATA, 1);
	coord->y	= target->y;
	coord->x	= target->x;
	
	switch (dir)
	{
	case NORTH:
		if (--coord->y < 0)			coord->y = WILD_Y - 1;
		break;
	case EAST:
		if (++coord->x >= WILD_X)	coord->x = 0;
		break;
	case SOUTH:
		if (++coord->y >= WILD_Y)	coord->y = 0;
		break;
	case WEST:
		if (--coord->x < 0)			coord->x = WILD_X - 1;
		break;
	case NORTHEAST:
		if (--coord->y < 0)			coord->y = WILD_Y - 1;
		if (++coord->x > WILD_X)	coord->x = 0;
		break;
	case SOUTHEAST:
		if (++coord->y >= WILD_Y)	coord->y = 0;
		if (++coord->x >= WILD_X)	coord->x = 0;
		break;
	case SOUTHWEST:
		if (++coord->y >= WILD_Y)	coord->y = 0;
		if (--coord->x < 0)			coord->x = WILD_X - 1;
		break;
	case NORTHWEST:
		if (--coord->y < 0)			coord->y = WILD_Y - 1;
		if (--coord->x < 0)			coord->x = WILD_X - 1;
		break;
	default:		// handle UP & DOWN too
		DISPOSE(coord);
		break;
	}

	return (coord);
}

COORD_DATA *check_next_step( COORD_DATA *step, int dir )
{
	COORD_DATA *coord;

	if (!(coord = make_step(step, dir)))
		return (NULL);

	if (invalid_sect(coord))
		return (NULL);

	return (coord);
}

/*
 * given a direction, return the target room 
 */
ROOM_DATA *find_wild_room( ROOM_DATA *wRoom, int dir, bool makenew )
{
	COORD_DATA *coord;
	ROOM_DATA *newRoom;
	
	if (!( coord = make_step(wRoom->coord, dir)))
		return (NULL);

	if (invalid_sect(coord))
		return (NULL);

	newRoom = get_wild_room(coord);

	if (!makenew)
		return (newRoom);

	if (!newRoom)
		newRoom = create_wild_room(coord, FALSE);

	make_exit(wRoom, newRoom, dir);
	make_exit(newRoom, wRoom, rev_dir[dir]);

	return (newRoom);
}


/* *************************************************************** */
/*                      W I L D E R N E S S                        */
/* *************************************************************** */

void load_sectors( void )
{
	FILE *fl;
	char fname[128], line[256];
	int nr = -1, last;

	log("   Loading sectors descriptions.");
	
	sprintf(fname, "%s", SECT_FILE);
	
	/* Si legge le descrizioni base dei vari settori */
	if (!(fl = fopen(fname, "r")))
	{
		if (errno != ENOENT)
		{
			log("fatal error opening: %s", fname);
			exit(1);
		}
		else
		{
			log("%s file does not exist", fname);
			exit(1);
		}
	}

	last = 0;
	for (;;)
	{
		if (!get_line(fl, line))
		{
			log("SYSERR: Format error in %s file after #%d (file not terminated with '$'?)\n", fname, nr);
			exit(1);
		}
		
		if (*line == '$') break;
		if (*line == '*') continue;
		
		if (*line == '#') 
		{
			last = nr;
			if (sscanf(line, "#%d", &nr) != 1)
			{
				log("SYSERR: Format error in %s after #%d\n", fname, last);
				exit(1);
			}

			CREATE( terrain_type[nr], TERRAIN_DATA, 1 );

			terrain_type[nr]->name			= fread_string(fl, buf2);
			terrain_type[nr]->description	= fread_string(fl, buf2);
			terrain_type[nr]->movement_loss	= fread_number(fl);
			terrain_type[nr]->map			= str_dup( fread_word( fl ) );
			terrain_type[nr]->map2			= str_dup( fread_word( fl ) );
			terrain_type[nr]->mapbw			= str_dup( fread_word( fl ) );
		}
	}

	fclose( fl );

	/*
	 * check for submaps
	 *
	 * if it's not present the 0-0.map file, it will regenerate all submaps
	 */
	sprintf(fname, "%smap/0-0.map", WLS_PREFIX);
	if (!(fl = fopen(fname, "r")))
	{
		void parse_bmp(void);
		void create_submaps(void);

		log("   Wilderness map not available - regenerating submaps (this will take a while)");
		parse_bmp();
		create_submaps();
		log("   Done.");
	}
	else
		fclose(fl);
}


/* *************************************************************** */
/* Character map & movement handling                               */
/* *************************************************************** */

void generate_ch_map( CHAR_DATA *ch, COORD_DATA *coord )
{
	FILE *fl[3];
	char fname[80], line[3][MAP_SIZE+3];
	char ch_wild[MAP_Y][MAP_X+3];
	int xfl[3], yfl[3], xx, yy, filey, curry;

	if ( coord->y < 0 || coord->y > WILD_Y || coord->x < 0 || coord->x > WILD_X )
		return;

	// quadrato centrale
	yfl[1] = coord->y / MAP_SIZE * MAP_SIZE;
	xfl[1] = coord->x / MAP_SIZE * MAP_SIZE;

	// quadrato superiore sinistro
	if ( yfl[1] - MAP_SIZE < 0 )
		yfl[0] = WILD_Y - MAP_SIZE;
	else
		yfl[0] = yfl[1] - MAP_SIZE;
	if ( xfl[1] - MAP_SIZE < 0 )
		xfl[0] = WILD_X - MAP_SIZE;
	else
		xfl[0] = xfl[1] - MAP_SIZE;

	// quadrato inferiore destro
	if ( yfl[1] + MAP_SIZE > WILD_Y - MAP_SIZE )
		yfl[2] = 0;
	else
		yfl[2] = yfl[1] + MAP_SIZE;
	if ( xfl[1] + MAP_SIZE > WILD_X - MAP_SIZE )
		xfl[2] = 0;
	else
		xfl[2] = xfl[1] + MAP_SIZE;

	curry = 0;

	for ( filey = 0; filey < 3; filey++ )
	{
		for ( xx = 0; xx < 3; xx++ )
		{
			sprintf( fname, "%smap/%d-%d.map", WLS_PREFIX, yfl[filey], xfl[xx] );

			if ( !( fl[xx] = fopen( fname, "r" ) ) )
			{
				log( "SYSERR: generate_ch_map() : cannot open map file %d-%d.map", xfl, yfl );
				send_to_char( "Al momento non e' possibile.. riprova piu' tardi.\r\n", ch );
				return;
			}
		}

		for ( yy = 0; yy < MAP_SIZE; yy++ )
		{
			get_line( fl[0], line[0] );
			get_line( fl[1], line[1] );
			get_line( fl[2], line[2] );

			sprintf( ch_wild[yy+curry], "%s%s%s", line[0], line[1], line[2] );
		}
		curry += MAP_SIZE;

		for ( xx = 0; xx < 3; xx++ )
		{
			fclose ( fl[xx] );
		}
	}

	/* generate char map */
	if ( !ch->player_specials->wildmap )
	{
		CREATE( ch->player_specials->wildmap, char *, MAP_Y );
		for ( yy = 0; yy < MAP_Y; yy++ )
		{
			CREATE( ch->player_specials->wildmap[yy], char, MAP_X );
			ch->player_specials->wildmap[yy] = NULL;
		}
	}

	for ( yy = 0; yy < MAP_Y; yy++ )
	{
		if ( ch->player_specials->wildmap[yy] )
			DISPOSE( ch->player_specials->wildmap[yy] );
		ch->player_specials->wildmap[yy] = str_dup( ch_wild[yy] );
	}
}

/*
 * load wild sector submaps & build char map
 */
void wild_map_load( CHAR_DATA *ch, COORD_DATA *coord )
{
	load_wildmap_sector(coord);
	if (!IS_NPC(ch))
		generate_ch_map(ch, coord);
}

/*
 * update wild sector usage
 */
void update_wild_life( int y, int x, int oper )
{
	WILD_DATA *wd;
	int ty, tx;

	ty = y / MAP_SIZE * MAP_SIZE;
	tx = x / MAP_SIZE * MAP_SIZE;

	if ( !( wd = wild_sector( ty, tx ) ) )
	{
		log( "SYSERR: update_wild_life() attempting update life for non-loaded sector %d %d.", y, x );
		return;
	}

	switch ( oper )
	{
	case WS_ADD_ROOM:
		wd->num_of_rooms++;
		break;
	case WS_REM_ROOM:
		wd->num_of_rooms--;
		/* no rooms here.. can be unload */
		if ( wd->num_of_rooms == 0 )
			unload_wildmap_sector( wd );
		break;
	}
}

/*
 * check if a character, vehicle, ship or ferryboat
 * move between two different wild sectors
 */
void check_wild_move(CHAR_DATA *ch, VEHICLE_DATA *vehicle, SHIP_DATA *ship, FERRY_DATA *ferry,
					 ROOM_DATA *pRoomFrom, ROOM_DATA *pRoomTo)
{
	CHAR_DATA *ppl;
	int quadx[2], quady[2];

	if (!pRoomFrom || !pRoomTo)
		return;

	if (!IS_WILD(pRoomTo))
		return;

	if (!IS_WILD(pRoomFrom))
	{
		if (ch)
			wild_map_load(ch, pRoomTo->coord);
		else if (ship)
		{
			for ( ppl = ship->people; ppl; ppl = ppl->next_in_vehicle )
				wild_map_load( ppl, pRoomTo->coord );
		}
		else if (vehicle)
		{
			if (vehicle->wagoner)
				wild_map_load(vehicle->wagoner, pRoomTo->coord);
			
			for (ppl = vehicle->people; ppl; ppl = ppl->next_in_vehicle)
				wild_map_load(ppl, pRoomTo->coord);
		}
		else if (ferry)
		{
			for ( ppl = ferry->room->people; ppl; ppl = ppl->next_in_room )
				wild_map_load( ppl, pRoomTo->coord );
		}
		return;
	}
	
	quady[0] = GET_RY(pRoomFrom) / MAP_SIZE * MAP_SIZE;
	quadx[0] = GET_RX(pRoomFrom) / MAP_SIZE * MAP_SIZE;
	
	quady[1] = GET_RY(pRoomTo) / MAP_SIZE * MAP_SIZE;
	quadx[1] = GET_RX(pRoomTo) / MAP_SIZE * MAP_SIZE;

	if (quadx[0] != quadx[1] || quady[0] != quady[1])
	{
		if (ch)
			wild_map_load(ch, pRoomTo->coord);
		else if (ship)
		{
			for ( ppl = ship->people; ppl; ppl = ppl->next_in_vehicle )
				wild_map_load( ppl, pRoomTo->coord );
		}
		else if (vehicle)
		{
			if (vehicle->wagoner)
				wild_map_load(vehicle->wagoner, pRoomTo->coord);
			
			for (ppl = vehicle->people; ppl; ppl = ppl->next_in_vehicle)
				wild_map_load(ppl, pRoomTo->coord);
		}
		else if (ferry)
		{
			for ( ppl = ferry->room->people; ppl; ppl = ppl->next_in_room )
				wild_map_load( ppl, pRoomTo->coord );
		}
	}
}


/* =============================================================== */
/* Immortal Commands                                               */
/* =============================================================== */
void do_stat_wild(CHAR_DATA *ch)
{
	WILD_DATA *wd;
	char wbuf[MAX_STRING_LENGTH];
	int y, x;

	strcpy( wbuf, "List of Wilderness Sectors loaded:\r\n" );

	for ( y = 0; y < WDATA_HASH; y++ )
	{
		for ( x = 0; x < WDATA_HASH; x++ )
		{
			if ( kWorld[y][x] )
			{
			for ( wd = kWorld[y][x]; wd; wd = wd->next )
			{
				sprintf( wbuf + strlen(wbuf), "  Coord: [%d] [%d]  Life: %d\r\n",
					wd->coord->y, wd->coord->x, wd->num_of_rooms );
			}
			}
		}
	}
	send_to_char( wbuf, ch );
}

ACMD(do_wildlist)
{
	ROOM_DATA *pRoom;
	char wbuf[MAX_STRING_LENGTH];
	int y, x;

	strcpy(wbuf, "List of Wilderness Rooms loaded:\r\n");

	for (y = 0; y < WILD_HASH; y++)
	{
		for (x = 0; x < WILD_HASH; x++)
		{
			for (pRoom = wild[y][x]; pRoom; pRoom = pRoom->next)
				sprintf(wbuf + strlen(wbuf), " Coord: [%d] [%d]  Sector: %s\r\n",
					pRoom->coord->y, pRoom->coord->x, terrain_type[pRoom->sector_type]->name);
		}
	}
	page_string(ch->desc, wbuf, 1);
}

