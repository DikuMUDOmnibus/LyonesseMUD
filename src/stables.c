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
 * File: stables.c                                                        *
 *                                                                        *
 * mounts and vehicles save code                                          *
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

/* externs */
extern VEHICLE_DATA		*first_vehicle;
extern VEHICLE_DATA		*last_vehicle;
extern VEHICLE_INDEX	*vehicle_types[MAX_VEH_TYPE];
extern bool				LoadingCharObj;

/* external functions */
AFFECTED_DATA *aff_by_bitvector( CHAR_DATA *ch, int bitv );
VEHICLE_DATA *new_vehicle(void);
OBJ_DATA *fread_one_obj( FILE *fp, int *location );
int		save_objs(OBJ_DATA *obj, FILE *fp, int mode, int location);
void	mob_to_yoke( CHAR_DATA *mob, VEHICLE_DATA *vehicle );

/* globals */
STABLE_RENT	*first_stable_rent	= NULL;
STABLE_RENT	*last_stable_rent	= NULL;

/* ****************************************************************** */
/* STABLE INDEX routines                                              */
/* ****************************************************************** */

void LoadStableRentIndex(void)
{
	FILE *fp;
	STABLE_RENT *strent;
	char iname[128], *line;
	char name1[128], name2[128], name3[128];

	sprintf(iname, "%sindex", LIB_STABLES);
	if ( !( fp = fopen(iname, "r") ) )
	{
		log("   No Stables actually in game.");
		return;
	}

	for ( ; ; )
	{
		if ( feof(fp) )
			break;

		line = fread_line(fp);
		
		if ( *line == '$' )
			break;

		if ( *line == '*' )
			continue;

		CREATE(strent, STABLE_RENT, 1);

		sscanf(line, "%d %hd %d %s %s %s",
			&strent->stable_vnum, &strent->type, &strent->id_player,
			name1, name2, name3);
		strent->filename	= str_dup(name1);
		strent->playername	= str_dup(name2);
		strent->typedescr	= str_dup(name3);

		LINK(strent, first_stable_rent, last_stable_rent, next, prev);
	}

	fclose(fp);
}

void fwrite_rent_index( void )
{
	FILE *fp;
	STABLE_RENT *strent;
	char iname[128];

	if ( !first_stable_rent )
		return;

	sprintf(iname, "%sindex", LIB_STABLES);
	if ( !( fp = fopen(iname, "w") ) )
	{
		log("SYSERR: fwrite_rent_index() - cannot create index file.");
		return;
	}

	for ( strent = first_stable_rent; strent; strent = strent->next )
		fprintf(fp, "%d %hd %d %s %s %s\n",
			strent->stable_vnum,
			strent->type, strent->id_player, strent->filename,
			strent->playername, strent->typedescr);

	fprintf(fp, "$\n");
	fclose(fp);
}


/* ****************************************************************** */
/* RENT routines                                                      */
/* ****************************************************************** */

void fwrite_rent_mount(CHAR_DATA *mount, FILE *fp)
{
	// save mount data
	fprintf(fp, "#MOUNT\n");
	fprintf(fp, "Mob_vnum      %d\n",	GET_MOB_VNUM(mount));
	fprintf(fp, "Owner_id      %d\n",	MOB_OWNER(mount));
	fprintf(fp, "End\n");
}

void rent_mount(CHAR_DATA *ch, CHAR_DATA *mount)
{
	FILE *fp;
	STABLE_RENT *strent;
	char fname[128];

	if ( IS_NPC(ch) )
		return;

	// here we loose mounts without Vnum
	if ( !IS_NPC(mount) || GET_MOB_VNUM(mount) == NOTHING )
		return;

	CREATE(strent, STABLE_RENT, 1);

	strent->filename	= str_dup(fname);
	strent->id_player	= GET_IDNUM(ch);
	strent->playername	= str_dup(GET_PC_NAME(ch));
	strent->type		= ST_RENT_MOUNT;
	strent->typedescr	= str_dup(GET_PC_NAME(mount));
	strent->stable_vnum	= ch->in_room->number;

	LINK(strent, first_stable_rent, last_stable_rent, next, prev);

	// save mount
	sprintf(fname, "%s%ld.txt", LIB_STABLES, time(0));
	if (!(fp = fopen(fname, "w")))
	{
		log("SYSERR: fwrite_rent_mount() - cannot create file %s", fname);
		return;
	}
	fwrite_rent_mount(mount, fp);
	fprintf(fp, "#END\n");
	fclose(fp);

	fwrite_rent_index();
}


void fwrite_rent_vehicle(VEHICLE_DATA *vehicle, FILE *fp)
{
	fprintf(fp, "\n#VEHICLE\n");
	fprintf(fp, "Name          %s\n",		vehicle->name);
	fprintf(fp, "Type          %hd\n",		vehicle->type->vnum);
	fprintf(fp, "Owner_id      %d\n",		vehicle->owner_id);
	if ( vehicle->short_description )
		fprintf(fp, "Shortdescr    %s~\n",	vehicle->short_description);
	if ( vehicle->description )
		fprintf(fp, "Descr         %s~\n",	vehicle->description);
	fprintf(fp, "Val_max       %hd %hd %hd %hd %hd\n",		
		vehicle->max_val.capacity, vehicle->max_val.health, vehicle->max_val.speed,
		vehicle->max_val.passengers, vehicle->max_val.draft_mobs);
	/*
	 * doesn't save curr_val.passengers and curr_val.draft_mobs
	 * because there mustn't be passengers inside when we save
	 * and draft_mobs is handled by mob_to_yoke while reloading
	 */
	fprintf(fp, "Val_curr      %hd %hd %hd\n",		
		vehicle->curr_val.capacity, vehicle->curr_val.health, vehicle->curr_val.speed);

	fprintf(fp, "Flags         %d\n",		vehicle->flags);

	// save vehicle hithced mobs
	if ( vehicle->first_yoke )
	{
		YOKE_DATA *yoke;
		AFFECTED_DATA *aff;
		int adur;

		for ( yoke = vehicle->first_yoke; yoke; yoke = yoke->next )
		{
			// here we loose yoked mobs without Vnum
			if ( GET_MOB_VNUM(yoke->mob) == NOTHING )
				continue;

			if ( ( aff = aff_by_bitvector(yoke->mob, AFF_TAMED) ) )
				adur = aff->duration;
			else
				adur = 0;
			
			fprintf(fp, "Mob_vnum      %d %d\n",	GET_MOB_VNUM(yoke->mob), adur);
		}
	}

	fprintf(fp, "End\n");

	// save vehicle contents
	if ( vehicle->first_content )
		save_objs(vehicle->last_content, fp, 1, 0);
}

void rent_vehicle(CHAR_DATA *ch, VEHICLE_DATA *vehicle)
{
	FILE *fp;
	STABLE_RENT *strent;
	char fname[128];

	if ( IS_NPC(ch) )
		return;

	CREATE(strent, STABLE_RENT, 1);

	strent->filename	= str_dup(fname);
	strent->id_player	= GET_IDNUM(ch);
	strent->playername	= str_dup(GET_PC_NAME(ch));
	strent->type		= ST_RENT_VEHICLE;
	strent->typedescr	= str_dup(vehicle->name);
	strent->stable_vnum	= ch->in_room->number;

	LINK(strent, first_stable_rent, last_stable_rent, next, prev);

	// save vehicle
	sprintf(fname, "%s%ld.txt", LIB_STABLES, time(0));
	if (!(fp = fopen(fname, "w")))
	{
		log("SYSERR: rent_vehicle() - cannot create file %s", fname);
		return;
	}
	fwrite_rent_vehicle(vehicle, fp);
	fprintf(fp, "#END\n");
	fclose(fp);

	fwrite_rent_index();
}


/* ****************************************************************** */
/* UNRENT routines                                                    */
/* ****************************************************************** */

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


/* read rented mount data */
CHAR_DATA *fread_rent_mount( FILE *fp, ROOM_DATA *sRoom )
{
	CHAR_DATA *mount = NULL;
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

		case 'E':
			if ( !strcmp(word, "End") )
				return (mount);
			break;

		case 'M':
			if ( !strcmp(word, "Mob_vnum") )
			{
				int vnum = fread_number(fp);

				if ( !(mount = read_mobile(vnum, VIRTUAL)) )
				{
					log("SYSERR: fread_rent_mount() - cannot find mob (vnum %d).", vnum);
					return (NULL);
				}

				char_to_room(mount, sRoom);

				fMatch = TRUE;
				break;
			}

		case 'O':
			if ( !strcmp(word, "Owner_id") )
			{
				if ( mount )
					MOB_OWNER(mount) = fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;
		}

		if ( !fMatch )
			log( "fread_rent_mount: no match: %s", word );
	}

	return (mount);
}

/* read rented vehicle data */
VEHICLE_DATA *fread_rent_vehicle( FILE *fp, ROOM_DATA *sRoom )
{
	VEHICLE_DATA *vehicle = new_vehicle();
	char *word;
	bool fMatch;

	vehicle_to_room( vehicle, sRoom );

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
			KEY( "Descr",		vehicle->description,	fread_string_nospace(fp) );
			break;

		case 'E':
			if ( !strcmp( word, "End" ) )
			{
				// do some checks.. 
				if ( !vehicle->type )
				{
					log("SYSERR: fread_rent_vehicle() - unknown vehicle type.");
					return (NULL);
				}
				if ( !vehicle->name )
					vehicle->name = str_dup(vehicle->type->name);
				if ( !vehicle->short_description )
					vehicle->short_description = str_dup(vehicle->type->short_description);
				if ( !vehicle->description )
					vehicle->description = str_dup(vehicle->type->description);

				return (vehicle);
			}
			break;

		case 'F':
			KEY( "Flags",		vehicle->flags,			fread_number(fp) );
			break;

		case 'M':
			if ( !strcmp( word, "Mob_vnum" ) )
			{
				CHAR_DATA *mob;
				int vnum, adur;

				vnum = fread_number(fp);
				adur = fread_number(fp);

				if ( (mob = read_mobile(vnum, VIRTUAL)) )
				{
					/* hmm.. calc for elapsed?? */
					if ( adur != 0 )
					{
						AFFECTED_DATA af;
						
						// re-tame the mob..
						af.duration	= adur;
						af.bitvector	= AFF_TAMED;
						affect_join(mob, &af, FALSE, FALSE, FALSE, FALSE);
					}

					char_to_room(mob, sRoom);
					mob_to_yoke(mob, vehicle);
				}
				fMatch = TRUE;
				break;
			}
			break;

		case 'N':
			KEY( "Name",		vehicle->name,			str_dup(fread_word(fp)) );
			break;

		case 'O':
			KEY( "Owner_id",	vehicle->owner_id,		fread_number(fp) );
			break;

		case 'S':
			KEY( "Shortdescr",	vehicle->short_description,	fread_string_nospace(fp) );
			break;

		case 'T':
			if (!str_cmp(word, "Type"))
			{
				int vnum = fread_number(fp);

				if (vnum < 0 || vnum >= MAX_VEH_TYPE)
				{
					log("SYSERR: fread_rent_vehicle() - invalid vehicle type vnum %d.", vnum);
					exit(1);
				}
				if (!vehicle_types[vnum])
				{
					log("SYSERR: fread_rent_vehicle() - invalid vehicle type vnum %d.", vnum);
					exit(1);
				}

				vehicle->type	= vehicle_types[vnum];
				fMatch = TRUE;
				break;
			}
			break;

		case 'V':
			if ( !strcmp( word, "Val_curr" ) )
			{
				vehicle->curr_val.capacity	= fread_number(fp);
				vehicle->curr_val.health	= fread_number(fp);
				vehicle->curr_val.speed		= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if ( !strcmp( word, "Val_max" ) )
			{
				vehicle->max_val.capacity	= fread_number(fp);
				vehicle->max_val.health		= fread_number(fp);
				vehicle->max_val.speed		= fread_number(fp);
				vehicle->max_val.passengers	= fread_number(fp);
				vehicle->max_val.draft_mobs	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;
		}

		if ( !fMatch )
			log( "fread_rent_vehicle: no match: %s", word );
	}

	return (vehicle);
}

#define MAX_BAG_ROWS	5

/* this is used to unrent mounts from a stable */
void unrent_from_stable( CHAR_DATA *ch, STABLE_RENT *strent )
{
	FILE *fp;
	CHAR_DATA *mount;
	VEHICLE_DATA *vehicle;
	OBJ_DATA *obj, *obj2, *cont_row[MAX_BAG_ROWS];
	char fname[128];
	char letter;
	char *word;
	int location, j;

	sprintf(fname, "%s%s", LIB_STABLES, strent->filename);
	if ( !( fp = fopen(fname, "r") ) )
	{
		log("SYSERR: unrent_from_stable() - cannot open file %s", fname);
		return;
	}

	for (j = 0; j < MAX_BAG_ROWS; j++)
		cont_row[j] = NULL;

	LoadingCharObj = TRUE;

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
			log( "SYSERR: unrent_from_stable: # not found." );
			break;
		}
		
		word = fread_word( fp );

		if ( !strcmp( word, "VEHICLE" ) )
		{
			if ( !( vehicle = fread_rent_vehicle(fp, ch->in_room) ) )
			{
				log( "SYSERR: unrent_from_stable() cannot load vehicle data." );
				send_to_char("There is a problem with your vehcile. Tell an Immortal.\r\n", ch);
				fclose( fp );
				LoadingCharObj = FALSE;
				return;
			}

			LINK(vehicle, first_vehicle, last_vehicle, next, prev);

			continue;
		}
		else if ( !strcmp( word, "MOUNT" ) )
		{
			if ( !( mount = fread_rent_mount(fp, ch->in_room) ) )
			{
				log( "SYSERR: unrent_from_stable() cannot load mob data." );
				send_to_char("There is a problem with your mount. Tell an Immortal.\r\n", ch);
				fclose( fp );
				LoadingCharObj = FALSE;
				return;
			}
		}
		else if ( !strcmp( word, "OBJVEH" ) )
		{
			if ( !vehicle )
			{
				log("SYSERR: unrent_from_stable() - trying to load an object without a vehicle.");
				fclose(fp);
				LoadingCharObj = FALSE;
				return;
			}

			if ( ( obj = fread_one_obj( fp, &location ) ) == NULL )
				continue;
			
			obj = obj_to_vehicle(obj, vehicle);

			for (j = MAX_BAG_ROWS - 1; j > -location; j--)
			{
				if (cont_row[j])
				{
					for (; cont_row[j]; cont_row[j] = obj2)
					{
						obj2 = cont_row[j]->next_content;
						cont_row[j] = obj_to_vehicle( cont_row[j], vehicle );
					}
					cont_row[j] = NULL;
				}
			}
			if (j == -location && cont_row[j])
			{
				if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER ||
					GET_OBJ_TYPE(obj) == ITEM_MISSILE_CONT)
				{
					obj_from_vehicle(obj);
					obj->first_content = NULL;
					obj->last_content = NULL;
					for (; cont_row[j]; cont_row[j] = obj2)
					{
						obj2 = cont_row[j]->next_content;
						obj_to_obj(cont_row[j], obj);
					}
					obj = obj_to_vehicle( obj, vehicle );
				}
				else
				{
					for (; cont_row[j]; cont_row[j] = obj2)
					{
						obj2 = cont_row[j]->next_content;
						cont_row[j] = obj_to_vehicle( cont_row[j], vehicle );
					}
					cont_row[j] = NULL;
				}
			}
			if (location < 0 && location >= -MAX_BAG_ROWS)
			{
				obj_from_vehicle(obj);
				if ((obj2 = cont_row[-location - 1]) != NULL)
				{
					while (obj2->next_content)
						obj2 = obj2->next_content;
					obj2->next_content = obj;
				}
				else
					cont_row[-location - 1] = obj;
			}
		}
		else if ( !strcmp( word, "END" ) )	// Done
			break;
	}

	fclose(fp);

	LoadingCharObj = FALSE;

	/* remove withdrawed vehicle from list */
	UNLINK(strent, first_stable_rent, last_stable_rent, next, prev);
	strent->next = NULL;
	strent->prev = NULL;
	if ( strent->filename )
		STRFREE(strent->filename);
	if ( strent->playername )
		STRFREE(strent->playername);
	if ( strent->typedescr )
		STRFREE(strent->typedescr);
	DISPOSE(strent);

	// delete vehicle file
	remove(fname);
	// write updated index
	fwrite_rent_index();
}


/* ****************************************************************** */
/* Commands                                                           */
/* ****************************************************************** */

ACMD(do_stablerent)
{
	STABLE_RENT *strent;
	char sbuf[MAX_STRING_LENGTH];

	if ( !first_stable_rent )
	{
		send_to_char("No mounts rented in stables at the moment.\r\n", ch);
		return;
	}

	strcpy(sbuf, "List of mounts rented in stables:\r\n");
	for ( strent = first_stable_rent; strent; strent = strent->next )
	{
		sprintf(sbuf + strlen(sbuf),
			"Stable: [%d] - %s (%s) - rented by %s - filename: %s.\r\n",
			strent->stable_vnum,
			st_rent_descr[strent->type],
			strent->typedescr,
			strent->playername,
			strent->filename
			);
	}
	page_string(ch->desc, sbuf, 1);
}

