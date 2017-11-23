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
 * File: buildings.c                                                      *
 *                                                                        *
 * Buildings code                                                         *
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
#include "clan.h"

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


/* external functions */
SPECIAL(tradingpost);
ROOM_DATA *create_wild_room(COORD_DATA *coord, bool Static);
OBJ_DATA *fread_one_obj(FILE *fp, int *location);
TRADING_POST *find_trading_post(int vnum);
bitvector_t asciiflag_conv(char *flag);
int		save_objs(OBJ_DATA *obj, FILE *fp, int mode, int location);
int		sprintascii(char *out, bitvector_t bits);
void	strip_cr(char *buffer);
void	setup_trigger( ROOM_DATA *pRoom, FILE *fl );

/* globals */
BUILDING_TYPE	*building_template[NUM_BUILDING_TYPE];	/* array of building templates	*/
BUILDING_DATA	*building_list[BLD_HASH];				/* global hash table		*/
BLD_WORKS		*first_build_work	= NULL;
BLD_WORKS		*last_build_work	= NULL;

int				num_of_bld_templ	= 0;
int				top_of_buildings	= 0;
int				last_works_num		= 0;

bool			building_loaded		= FALSE;

/* local functions */
void SaveBuildings(void);

/* ================================================================= */

/* ***************************************************************** */
/* Code for Display Informations of a building                       */    
/* ***************************************************************** */

/* display the condition of a building */
void show_building_cond(CHAR_DATA *ch, BUILDING_DATA *bld)
{
	int stat, cond = percentage(bld->curr_val.health, bld->max_val.health);

	const char *building_health_descr[] = 
	{
		"is in perfect conditions",				// 0
		"looks normal",							// 1
		"is slightly damaged",					// 2
		"looks damaged",						// 3
		"is heavily damaged",					// 4
		"needs immediate repairings",			// 5
		"is almost ruined",						// 6
		"seems that could collapse at any time"	// 7
	};

	if		(cond < 5)	stat = 7;
	else if (cond < 10)	stat = 6;
	else if (cond < 20)	stat = 5;
	else if (cond < 35)	stat = 4;
	else if (cond < 50)	stat = 3;
	else if (cond < 70)	stat = 2;
	else if (cond < 85)	stat = 1;
	else				stat = 0;

	ch_printf(ch, "%s %s (%d / %d).\r\n",
		bld->description,
		building_health_descr[stat], bld->curr_val.health, bld->max_val.health);
}

/* show a list of buildings in the room */
void list_building_to_char(BUILDING_DATA *bldlist, CHAR_DATA *ch)
{
	BUILDING_DATA *bld, *blist[20];
	int num = 0, counter, locate_list[20];
	bool found;

	if (!ch)	return;

	for (bld = bldlist; bld; bld = bld->next_in_room)
	{
		if (!bld->type || !bld->type->name || !bld->description)
			continue;

		if (num < 20)
		{
			found = FALSE;
			for (counter = 0; counter < num && !found; counter++)
			{
				if (bld->type->vnum == blist[counter]->type->vnum &&
				    BUILDING_FLAGS(bld) == BUILDING_FLAGS(blist[counter]))
				{
					locate_list[counter]++;
					found = TRUE;
				}
			}
			if (!found)
			{
				blist[num] = bld;
				locate_list[num] = 1;
				num++;
			}
		}
		else
			ch_printf(ch, "You see %s.\r\n", bld->description);
	}

	if (num)
	{
		for (counter = 0; counter < num; counter++)
		{
			if (locate_list[counter] > 1)
				ch_printf(ch, "You see %s (%d).\r\n", blist[counter]->description, locate_list[counter]);
			else
				ch_printf(ch, "You see %s.\r\n", blist[counter]->description);
		}
	}

	if (ROOM_FLAGGED(ch->in_room, ROOM_BUILDING_WORKS))
		send_to_char("There are building works here.\r\n", ch);
}

/* ================================================================= */
/* Code for handling chars in & out of a building                    */    
/* ================================================================= */

/* place a char inside a building */
void char_to_building(CHAR_DATA *ch, BUILDING_DATA *bld, int room)
{
	int room_num = room;

	if (room_num < 0 || room_num >= bld->size)
		room_num = bld->type->entrance;

	ch->in_building	= bld;
	char_to_room(ch, bld->rooms[room_num]);

	ch->next_in_building	= bld->people;
	bld->people				= ch;
}

/* take out a char from the building he's inside */
void char_from_building(CHAR_DATA *ch)
{
	CHAR_DATA *temp;

	if (!ch || !ch->in_building)
		return;

	REMOVE_FROM_LIST(ch, ch->in_building->people, next_in_building);
	ch->in_building			= NULL;
	ch->next_in_building	= NULL;
}


/* ================================================================= */
/* Code for handling authorization                                   */    
/* ================================================================= */

/* search the authorized ppl array for the idnum */
AUTH_DATA *get_bld_auth(BUILDING_DATA *bld, long idnum)
{
	AUTH_DATA *bAuth;

	if (!bld || !bld->auth)
		return (NULL);

	for (bAuth = bld->auth; bAuth != NULL; bAuth = bAuth->next)
	{
		if (bAuth->id_num == idnum)
			break;
	}

	return (bAuth);
}

/*
 * if bCanAuthCheck is FALSE, check if char can enter the building
 *
 * if bCanAuthCheck is TRUE, check if char can toggle entrance authorization
 * to other people
 *
 */
bool can_enter_bld(CHAR_DATA *ch, BUILDING_DATA *bld, bool bCanAuthCheck)
{
	if ( !ch || !bld )
		return (FALSE);

	if (IS_IMMORTAL(ch))
		return (TRUE);

	if (bld->can_enter == BLD_ENT_FREE)
	{
		if (bCanAuthCheck)
		{
			send_to_char("You don't need to set/revoke authorization for free entrance buildings.\r\n", ch);
			return (FALSE);
		}
		return (TRUE);
	}

	/* owner of the building */
	if (bld->owner_type == BLD_OWN_CHAR && GET_IDNUM(ch) == bld->owner_id)
		return (TRUE);

	/* member of the clan */
	if (bld->owner_type == BLD_OWN_CLAN && GET_CLAN(ch) == bld->owner_id)
	{
		if (bCanAuthCheck)
		{
			// only privileged members can authorize to enter
			if (GET_CLAN_RANK(ch) >= RANK_PRIVILEGES)
				return (TRUE);
			else
				return (FALSE);
		}

		if (GET_CLAN_RANK(ch) < RANK_MEMBER_FIRST)
			return (FALSE);

		return (TRUE);
	}

	if (bCanAuthCheck)
		return (FALSE);

	/* ppl authorized to enter */
	if (get_bld_auth(bld, GET_IDNUM(ch)))
		return (TRUE);

	return (FALSE);
}

/* entering a building.. called from do_enter in act.movement.c */
void enter_building(CHAR_DATA *ch, BUILDING_DATA *bld)
{
	EXIT_DATA *pexit;

	if (!ch || !bld)
		return;

	if (BUILDING_FLAGGED(bld, BLD_F_RUIN))
	{
		send_to_char("You cannot enter in a ruined building.\r\n", ch);
		return;
	}
	
	// special case
	if (bld->type->vnum != BLD_STABLE)
	{
		if (RIDING(ch))
		{
			send_to_char("You cannot enter a building while mounted.\r\n", ch );
			return;
		}
		if (WAGONER(ch))
		{
			send_to_char("You cannot enter a building while driving a vehicle.\r\n", ch );
			return;
		}
	}

	if (!(pexit = get_exit(bld->rooms[bld->type->entrance], SOUTH)))
	{
		log( "SYSERR: enter_building() - cannot find entrance to building %d.", bld->vnum );
		return;
	}

	if (EXIT_FLAGGED(pexit, EX_CLOSED))
	{
		if (pexit->keyword)
			ch_printf(ch, "The %s is closed.\r\n", pexit->keyword );
		else
			send_to_char("It's closed.\r\n", ch );
		return;
	}

	if (!can_enter_bld(ch, bld, FALSE))
	{
		ch_printf(ch, "You are not authorized to enter %s.\r\n", bld->description);
		return;
	}

	act("You enter $b.\r\n", FALSE, ch, NULL, bld, TO_CHAR);
	act("$n enters $b.", FALSE, ch, NULL, bld, TO_ROOM); 

	char_from_room(ch);
	char_to_building(ch, bld, NOWHERE);

	if (RIDING(ch))
	{
		char_from_room(RIDING(ch));
		char_to_building(RIDING(ch), bld, NOWHERE);
	}

	act("$n enters.", FALSE, ch, NULL, NULL, TO_ROOM); 

	look_at_room(ch, TRUE);
}

/*
 * give vict the authorization to enter bld
 * or
 * revoke to vict the authorization to enter bld
 */
void auth_enter(BUILDING_DATA *bld, CHAR_DATA *ch, CHAR_DATA *vict, int mode)
{
	AUTH_DATA *bAuth;

	if ( !bld || !vict )
		return;

	if (bld->can_enter == BLD_ENT_FREE)
	{
		send_to_char("You don't need to set/revoke authorization for free entrance buildings.\r\n", ch);
		return;
	}

	/* authorize */
	if (mode == 1)
	{
		if (get_bld_auth(bld, GET_IDNUM(vict)))
		{
			ch_printf(ch, "%s is already authorized to enter.\r\n", PERS(vict, ch));
			return;
		}

		CREATE(bAuth, AUTH_DATA, 1);
		bAuth->id_num	= GET_IDNUM(vict);
		bAuth->name		= str_dup(GET_NAME(vict));

		// add to the list
		bAuth->next		= bld->auth;
		bld->auth		= bAuth;

		ch_printf(ch, "%s is now authorized to enter %s.\r\n",
			PERS(vict, ch), bld->description);
		act("You are now authorized to enter $b.", FALSE, vict, NULL, bld, TO_CHAR);
	}
	/* revoke authorization */
	else
	{
		AUTH_DATA *temp;

		if (!(bAuth = get_bld_auth(bld, GET_IDNUM(vict))))
		{
			ch_printf(ch, "%s is already not authorized to enter.\r\n", PERS(vict, ch));
			return;
		}

		if (vict->in_building && vict->in_building->vnum == bld->vnum)
		{
			send_to_char("You cannot revoke authorization to someone that's currently inside.\r\n", ch);
			return;
		}

		REMOVE_FROM_LIST(bAuth, bld->auth, next);
		STRFREE(bAuth->name);
		DISPOSE(bAuth);

		ch_printf(ch, "%s now is not authorized to enter %s.\r\n",
			PERS(vict, ch), bld->description);
		act("You no longer can enter $b.", FALSE, vict, NULL, bld, TO_CHAR);
	}
}

/* ================================================================= */
/* Code for finding buildings, buildings rooms, buildings doors, etc */    
/* ================================================================= */

/* find a building in the entire world, by vnum */
BUILDING_DATA *find_building(int vnum)
{
	BUILDING_DATA *bld;
	int iHash;

	if (vnum < 0)
		return (NULL);

	iHash = vnum % BLD_HASH;

	for (bld = building_list[iHash]; bld; bld = bld->next)
	{
		if (bld->vnum == vnum)
			break;
	}

	return (bld);
}

/* find a building in the given room, by building name */
BUILDING_DATA *find_building_in_room_by_name(ROOM_DATA *pRoom, char *arg)
{
	BUILDING_DATA *bld = NULL;
	int fnum = 1;

	if (!pRoom || !pRoom->buildings || !*arg)
		return (NULL);

	if (!(fnum = get_number(&arg)))
		return (NULL);

	for (bld = pRoom->buildings; bld; bld = bld->next_in_room)
	{
		if (bld->keyword)
			if (isname(arg, bld->keyword))
				if (--fnum == 0)
					break;
	}

	return (bld);
}

/* find the first building owned by char in char room */
BUILDING_DATA *find_char_owned_building(CHAR_DATA *ch)
{
	BUILDING_DATA *bld;

	if (!ch || !ch->in_room || !ch->in_room->buildings)
		return (NULL);

	for (bld = ch->in_room->buildings; bld; bld = bld->next_in_room)
	{
		if (bld->owner_type == BLD_OWN_CHAR)
			if (bld->owner_id == GET_IDNUM(ch))
				break;
	}

	return (bld);
}

/* find a room inside a building */
ROOM_DATA *find_room_building(BUILDING_DATA *bld, int room)
{
	if (room < 0 || room > bld->size)
		return (NULL);

	if (BUILDING_FLAGGED(bld, BLD_F_RUIN))
		return (NULL);

	return (bld->rooms[room]);
}

/* find the room that has the entrance/exit from the building */
EXIT_DATA *find_building_door(CHAR_DATA *ch, char *doorname, char *building)
{
	BUILDING_DATA *bld;
	EXIT_DATA *pexit;

	if (!ch || !ch->in_room->buildings)
		return (NULL);

	if (!(bld = find_building_in_room_by_name(ch->in_room, building)))
		return (NULL);

	if (BUILDING_FLAGGED(bld, BLD_F_RUIN))
		return (NULL);

	pexit = find_exit(bld->rooms[bld->type->entrance], SOUTH);

	if (pexit && isname(doorname, pexit->keyword))
		return (pexit);

	return (NULL);
}

/* find the coord of the room where the building is placed */
COORD_DATA *get_bld_coord(int vnum)
{
	COORD_DATA *here;
	BUILDING_DATA *bld = find_building(vnum);

	if (!bld)
		return (NULL);

	CREATE(here, COORD_DATA, 1);

	if (IS_WILD(bld->in_room))
		*here = *bld->in_room->coord;
	else
	{
		*here = zone_table[bld->in_room->zone].wild.z_start;
		if (here->y == 0 && here->x == 0)
			here = NULL;
	}

	return (here);
}

/* ================================================================= */
/* Code for managing building templates                              */    
/* ================================================================= */

/* display a list of valid buildings templates */
void show_valid_list(CHAR_DATA *ch)
{
	char buf[MAX_STRING_LENGTH];
	int t;

	strcpy(buf, "Valids building type names are:\r\n");
	for (t = 0; t < num_of_bld_templ; t++)
		sprintf(buf + strlen(buf), "%s\r\n", building_template[t]->name);
	send_to_char(buf, ch);
}

/* get the building template, by vnum */
BUILDING_TYPE *get_bld_template(int tnum)
{
	int t;

	for (t = 0; t < num_of_bld_templ; t++)
	{
		if (building_template[t])
			if (building_template[t]->vnum == tnum)
				break;
	}

	if (t >= num_of_bld_templ)
		return (NULL);

	return (building_template[t]);
}

/* get the building template index, by name */
int get_bld_template_by_name(char *bname)
{
	int t;

	for (t = 0; t < num_of_bld_templ; t++)
	{
		if (building_template[t])
			if (is_abbrev(bname, building_template[t]->name))
				break;
	}

	if (t >= num_of_bld_templ)
		return (NOTHING);

	return (t);
}


/* read from file one building template */
BUILDING_TYPE *fread_bld_template(FILE *fp)
{
	BUILDING_TYPE *bType;
	char *word;
	bool fMatch;
	sh_int vnmob;

	CREATE(bType, BUILDING_TYPE, 1);

	bType->name			= NULL;
	bType->cmds_list	= NULL;
	bType->cost			= 0;
	bType->entrance		= 0;
	bType->size			= 0;
	bType->vnum			= NOTHING;

	for (vnmob = 0; vnmob < BLD_MAX_MOB; vnmob++)
		bType->vmob[vnmob]	= NOBODY;

	vnmob = 0;

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
			KEY("Cost",			bType->cost,			fread_number(fp));
			if (!strcmp(word, "Cmd"))
			{
				BUILDING_CMD *bCmd;
				fMatch = TRUE;

				CREATE(bCmd, BUILDING_CMD, 1);

				bCmd->cmd	= fread_letter(fp);

				if (strchr("MO", bCmd->cmd) == NULL)
				{
					log("SYSERR: fread_bld_template() - invalid command in building template %d.", bType->vnum);
					exit(1);
				}

				bCmd->arg[0]	= fread_number(fp);
				bCmd->arg[1]	= fread_number(fp);
				bCmd->arg[2]	= fread_number(fp);
				bCmd->arg[3]	= fread_number(fp);

				// add to the list
				bCmd->next		= bType->cmds_list;
				bType->cmds_list	= bCmd;

				break;
			}
			break;

		case 'E':
			KEY("Entrance",		bType->entrance,		fread_number(fp));
			if (!strcmp(word, "End"))
			{
				if (!bType->name || !bType->size || bType->vnum == NOTHING)
				{
					if (bType->name)
						free(bType->name);
					DISPOSE(bType);
					return (NULL);
				}

				return (bType);
			}
			break;

		case 'N':
			KEY("Name",			bType->name,			fread_string_nospace(fp));
			break;

		case 'R':
			if (!strcmp(word, "Rng_Defense"))
			{
				bType->range_min.defense	= fread_number(fp);
				bType->range_max.defense	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if (!strcmp(word, "Rng_Health"))
			{
				bType->range_min.health		= fread_number(fp);
				bType->range_max.health		= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if (!strcmp(word, "Rng_Att_MOB"))
			{
				bType->range_min.mob_attack	= fread_number(fp);
				bType->range_max.mob_attack	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if (!strcmp(word, "Rng_Att_NPC"))
			{
				bType->range_min.npc_attack	= fread_number(fp);
				bType->range_max.npc_attack	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'S':
			KEY("Size",			bType->size,			fread_number(fp));
			break;

		case 'V':
			KEY("Vnum",			bType->vnum,			fread_number(fp));
			if (!strcmp(word, "Vmob"))
			{
				mob_vnum vnmobnum = fread_number(fp);

				if ( vnmob >= BLD_MAX_MOB )
					log("SYSERR: fread_bld_template() - too many defending mobs.");
				else
					bType->vmob[vnmob++]	= vnmobnum;

				fMatch = TRUE;
				break;
			}
			break;
		}

		if (!fMatch)
			log("fread_bld_template: no match: %s", word);
	}

	if (bType->name)
		free(bType->name);
	DISPOSE(bType);

	return (NULL);
}

/*
 * load all buildings templates
 *
 * called from boot_world() in db.c
 */
void LoadBldTemplate(void)
{
	FILE *fp;
	char indname[256];
	char letter;
	char *word;

	sprintf(indname, "%sbuildings.txt", BLD_PREFIX);
	if (!(fp = fopen(indname, "r")))
	{
		log("SYSERR: Cannot open buildings index file %s.", indname);
		return;
	}

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
			log("LoadBldTemplate(): # not found.");
			break;
		}
		
		word = fread_word(fp);

		if (!str_cmp(word, "BUILDING"))
		{
			if (num_of_bld_templ >= NUM_BUILDING_TYPE)
			{
				log("LoadBldTemplate: more template than NUM_BUILDING_TYPE %d", NUM_BUILDING_TYPE);
				fclose(fp);
				return;
			}
			building_template[num_of_bld_templ++] = fread_bld_template(fp);
		}
		else if (!str_cmp(word, "END"))
			break;
		else
		{
			log("LoadBldTemplate: bad section %s.", word);
			continue;
		}
	}

	fclose(fp);
}


/* ================================================================= */
/* Code for placing and removing buildings in/from room              */    
/* ================================================================= */

/* place a building into a wild or zone room */
void building_to_room(BUILDING_DATA *bld, ROOM_DATA *pRoom)
{
	ROOM_DATA *room;

	if (!bld)
		return;

	if (!pRoom && !bld->coord && bld->vnum_room == NOWHERE)
		return;

	// a room exists.. place the building there..
	if (pRoom)
	{
		bld->vnum_room		= NOTHING;
		bld->coord			= NULL;

		if (IS_WILD(pRoom))
		{
			if (!bld->coord)
				CREATE(bld->coord, COORD_DATA, 1);
			
			bld->coord->y	= GET_RY(pRoom);
			bld->coord->x	= GET_RX(pRoom);
			
			SET_BIT(ROOM_FLAGS(pRoom), ROOM_WILD_STATIC);
		}
		else
			bld->vnum_room	= pRoom->number;

		/* add to the room list */
		bld->next_in_room	= pRoom->buildings;
		pRoom->buildings	= bld;

		bld->in_room		= pRoom;

		return;
	}

	// zone room
	if (bld->vnum_room != NOWHERE)
	{
		if (!(room = get_room(bld->vnum_room)))
		{
			log("SYSERR: Cannot place building to room %d.", bld->vnum_room);
			return;
		}
		bld->coord	= NULL;
	}
	// wild room
	else
	{
		if (!(room = create_wild_room(bld->coord, TRUE)))
		{
			log("SYSERR: Cannot place building to coord %d %d.",
				bld->coord->y, bld->coord->x);
			return;
		}
		bld->vnum_room = NOWHERE;
	}

	/* add to the room list */
	bld->next_in_room	= room->buildings;
	room->buildings		= bld;

	bld->in_room		= room;
}


/* ================================================================= */
/* Code for creating new buildings                                   */    
/* ================================================================= */

/* allocate memory and initialize data for a new building */
BUILDING_DATA *new_building(void)
{
	BUILDING_DATA *bld;
	int xm;

	CREATE(bld, BUILDING_DATA, 1);

	bld->vnum			= NOWHERE;
	bld->type			= NULL;
	bld->next			= NULL;
	bld->next_in_room	= NULL;
	bld->in_room		= NULL;
	bld->coord			= NULL;
	bld->people			= NULL;
	bld->auth			= NULL;
	bld->keyword		= NULL;
	bld->description	= NULL;
	bld->trp			= NULL;
	bld->cmds_list		= NULL;
	bld->vnum_room		= NOWHERE;
	bld->can_enter		= NOTHING;
	bld->owner_type		= NOTHING;
	bld->owner_id		= NOTHING;
	bld->flags			= 0;

	for (xm = 0; xm < BLD_MAX_MOB; xm++)
		bld->vmob[xm]	= NOBODY;

	return (bld);
}

/* make the building inside */
bool create_building_rooms(BUILDING_DATA *bld)
{
	FILE *fp;
	EXTRA_DESCR *new_descr;
	char fname[128];
	char letter, *flags;
	int room, door;

	sprintf(fname, "%s%d.bld", BLD_PREFIX, bld->type->vnum);
	if (!(fp = fopen(fname, "r")))
	{
		log("SYSERR: Unable to open building template file %s.", fname);
		return (FALSE);
	}

	CREATE(bld->rooms, ROOM_DATA *, bld->type->size);
	for (room = 0; room < bld->type->size; room++)
	{
		CREATE(bld->rooms[room], ROOM_DATA, 1);
		bld->rooms[room]->number = room;
		bld->rooms[room]->zone = BUILDING_ZONE;

		CREATE(bld->rooms[room]->extra_data, ROOM_EXTRA, 1);
		bld->rooms[room]->extra_data->vnum = bld->vnum;
	}

	// in entrance room must always be light
	bld->rooms[0]->light = 1;

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
			log("create_building_rooms(): # not found.");
			break;
		}
		
		room = fread_number(fp);

		if (room < 0 || room > bld->type->size)
			break;

		bld->rooms[room]->number		= room;
		bld->rooms[room]->name			= fread_string_nospace(fp);
		bld->rooms[room]->description	= fread_string_nospace(fp);
		flags							= fread_word(fp);
		bld->rooms[room]->room_flags	= asciiflag_conv(flags);
		bld->rooms[room]->sector_type	= fread_number(fp);

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
					log( "create_building_rooms: vnum %d has bad door number %d.", room, door );
					break;
				}
				else
				{
					EXIT_DATA *pexit;
					char flags[128];
					char *ln;
					int x1, x2;
					
					pexit = make_exit(bld->rooms[room], NULL, door);
					pexit->description	= fread_string_nospace(fp);
					pexit->keyword		= fread_string_nospace(fp);
					ln					= fread_line(fp);
					x1=x2=0;
					sscanf( ln, "%s %d %d ", &flags, &x1, &x2 );
					
					pexit->key			= x1;
					pexit->vnum			= x2;
					pexit->vdir			= door;
					pexit->exit_info	= asciiflag_conv(flags);
					
					pexit->to_room		= bld->rooms[pexit->vnum];

					/* sanity check */
					if (pexit->exit_info && !EXIT_FLAGGED(pexit, EX_ISDOOR))
						SET_BIT(pexit->exit_info, EX_ISDOOR);
				}
				break;

			case 'E':
				CREATE(new_descr, EXTRA_DESCR, 1);
				new_descr->keyword		= fread_string(fp, buf2);
				new_descr->description	= fread_string(fp, buf2);
				new_descr->next			= bld->rooms[room]->ex_description;
				bld->rooms[room]->ex_description = new_descr;
				break;
			/* Room Trigger */
			case 'T':
				setup_trigger( bld->rooms[room], fp );
				break;
			}
		}

		if (letter == '$')
			break;
	}

	fclose(fp);
	return (TRUE);
}


void exec_bld_cmds(BUILDING_DATA *bld)
{
	BUILDING_CMD *bCmd;
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	ROOM_DATA *bRoom;
	int x;
	
	for (bCmd = bld->type->cmds_list; bCmd; bCmd = bCmd->next)
	{
		switch (bCmd->cmd)
		{
		default:
			log("SYSERR: exec_bld_cmds() - unknown command in building %d.", bld->vnum);
			bCmd->cmd = '*';
			continue;
		case '*':
			continue;
		case 'M':
			if (!(bRoom = bld->rooms[bCmd->arg[2]]))
			{
				log("SYSERR: exec_bld_cmds() - invalid room vnum %d in building %d.", bCmd->arg[2], bld->vnum);
				bCmd->cmd = '*';
				continue;
			}
			for (x = 0; x < bCmd->arg[1]; x++)
			{
				if (!(mob = read_mobile(bCmd->arg[0], VIRTUAL)))
				{
					log("SYSERR: exec_bld_cmds() - invalid mob vnum %d in building %d.", bCmd->arg[0], bld->vnum);
					bCmd->cmd = '*';
					break;
				}
				char_to_room(mob, bRoom);
			}
			break;

		case 'O':
			if (!(bRoom = bld->rooms[bCmd->arg[2]]))
			{
				log("SYSERR: exec_bld_cmds() - invalid room vnum %d in building %d.", bCmd->arg[2], bld->vnum);
				bCmd->cmd = '*';
				continue;
			}
			
			for (x = 0; x < bCmd->arg[1]; x++)
			{
				if (!(obj = read_object(bCmd->arg[0], VIRTUAL)))
				{
					log("SYSERR: exec_bld_cmds() - invalid obj vnum %d in building %d.", bCmd->arg[0], bld->vnum);
					bCmd->cmd = '*';
					break;
				}
				obj_to_room(obj, bRoom);
			}
			break;
		}
	}
}

/* setup data for a yet-to-be created building */
BUILDING_DATA *setup_new_building(BUILDING_TYPE *bType, ROOM_DATA *pRoom, long owner_id, char owner_type)
{
	BUILDING_DATA *bld;
	EXIT_DATA *pexit;
	char buf[MAX_STRING_LENGTH];
	int iHash;

	if (!bType || !pRoom)
	{
		log("SYSERR: setup_new_building() - invalid pointer.");
		return (NULL);
	}

	bld = new_building();

	bld->vnum			= ++top_of_buildings;
	bld->type			= bType;
	bld->size			= bType->size;

	bld->keyword		= str_dup(bType->name);
	sprintf(buf, "%s %s", AN(bType->name), bType->name);
	bld->description	= str_dup(buf);

	/* setup building rooms */
	if (!create_building_rooms(bld))
	{
		log("SYSERR: Setup_new_building() - cannot create building rooms.");
		bld->type = NULL;
		DISPOSE(bld);
		return (NULL);
	}

	if (owner_type == BLD_OWN_CHAR)
	{
		bld->owner_id	= owner_id;
		bld->owner_type	= BLD_OWN_CHAR;
		bld->can_enter	= BLD_ENT_CHAR;
	}
	else
	{
		CLAN_DATA *pClan = get_clan(owner_id);

		if (!pClan)
		{
			log("SYSERR: setup_new_building() - invalid clan number %d.", owner_id);
			bld->type = NULL;
			DISPOSE(bld);
			return (NULL);
		}

		bld->owner_id	= owner_id;
		bld->owner_type	= BLD_OWN_CLAN;
		bld->can_enter	= BLD_ENT_CLAN;

		if (bType->vnum == BLD_CASTLE)
		{
			if (pClan->hall == NOWHERE)
			{
				SET_BIT(bld->flags, BLD_F_CLANHALL);
				pClan->hall = bld->vnum;
			}
		}
	}

	// everyone should be allowed to enter a trading post or a stable
	if (bld->type->vnum == BLD_STORE || bld->type->vnum == BLD_STABLE)
		bld->can_enter	= BLD_ENT_FREE;

	/* set up defense pts */
	bld->max_val.defense	= number(bType->range_min.defense, bType->range_max.defense);
	bld->curr_val.defense	= bld->max_val.defense;
	/* set up health pts */
	bld->max_val.health		= number(bType->range_min.health, bType->range_max.health);
	bld->curr_val.health	= bld->max_val.health;
	/* set up mob_attack pts */
	if (bType->range_max.mob_attack)
	{
		int xm;

		bld->max_val.mob_attack		= number(bType->range_min.mob_attack, bType->range_max.mob_attack);
		bld->curr_val.mob_attack	= bld->max_val.mob_attack;

		for (xm = 0; xm < BLD_MAX_MOB; xm++)
		{
			if (bType->vmob[xm] == NOBODY)
				continue;
			bld->vmob[xm] = bType->vmob[xm];
		}
	}
	/* set up npc_attack pts */
	if (bType->range_max.npc_attack)
	{
		bld->max_val.npc_attack		= number(bType->range_min.npc_attack, bType->range_max.npc_attack);
		bld->curr_val.npc_attack	= bld->max_val.npc_attack;
	}

	// execute building commands (load objs & mobs)
	exec_bld_cmds(bld);

	/* add to the global hash table */
	iHash					= bld->vnum % BLD_HASH;
	bld->next				= building_list[iHash];
	building_list[iHash]	= bld;

	/* place building into room */
	building_to_room(bld, pRoom);

	/* exit building --> room */
	pexit = make_exit(bld->rooms[bld->type->entrance], NULL, SOUTH);
	pexit->to_room = bld->in_room;
	pexit->keyword = str_dup("door");
	SET_BIT(EXIT_FLAGS(pexit), EX_ISDOOR);

	// forced save
	SaveBuildings();

	return (bld);
}


/* ======================================================================= */
/* Code for building Buildings                                             */
/* ======================================================================= */

/* time needed in each phase for each building rooms.. in mud hours */
int bwks_timing[] =
{
  0,
  4,
  2,
  3
};


/* setup build works a new building */
int build_works_start(BUILDING_TYPE *bType, ROOM_DATA *pRoom, long owner_id, char owner_type)
{
	BLD_WORKS *pWorks;

	CREATE(pWorks, BLD_WORKS, 1);

	pWorks->next			= NULL;
	pWorks->prev			= NULL;

	pWorks->num				= ++last_works_num;

	pWorks->owner_type		= owner_type;
	pWorks->owner_id		= owner_id;
	pWorks->authorized		= FALSE;
	pWorks->phase			= BWKS_BASE;
	pWorks->timer			= bwks_timing[BWKS_BASE] * bType->size;
	pWorks->type			= bType->vnum;

	pWorks->coord			= NULL;
	pWorks->in_room			= NULL;

	if (IS_WILD(pRoom))
	{
		CREATE(pWorks->coord, COORD_DATA, 1);
		pWorks->coord->y	= GET_RY(pRoom);
		pWorks->coord->x	= GET_RX(pRoom);
		SET_BIT(ROOM_FLAGS(pRoom), ROOM_BUILDING_WORKS | ROOM_WILD_STATIC);
	}
	else
	{
		pWorks->in_room		= pRoom;
		SET_BIT(ROOM_FLAGS(pRoom), ROOM_BUILDING_WORKS);
	}

	LINK(pWorks, first_build_work, last_build_work, next, prev);

	return (pWorks->num);
}

void build_works_process(void)
{
	BLD_WORKS *pWorks, *pWorks_next;

	if (!first_build_work)
		return;

	for (pWorks = first_build_work; pWorks; pWorks = pWorks_next)
	{
		pWorks_next	= pWorks->next;

		if (--pWorks->timer <= 0)
		{
			BUILDING_TYPE *bType = get_bld_template(pWorks->type);

			if (++pWorks->phase > BWKS_FINAL)
			{
				BUILDING_DATA *bld;

				if (pWorks->coord)
					pWorks->in_room = get_wild_room(pWorks->coord);

				bld = setup_new_building(bType, pWorks->in_room, pWorks->owner_id, pWorks->owner_type);

				UNLINK(pWorks, first_build_work, last_build_work, next, prev);

				REMOVE_BIT(ROOM_FLAGS(pWorks->in_room), ROOM_BUILDING_WORKS);

				pWorks->in_room = NULL;
				pWorks->next	= NULL;
				pWorks->prev	= NULL;
				DISPOSE(pWorks->coord);
				DISPOSE(pWorks);
			}
			else
				pWorks->timer	= bwks_timing[pWorks->phase] * bType->size;
		}
	}
}

const char *bwks_phase_descr[] =
{
  "none",
  "basement",
  "walls",
  "roof",
  "\n"
};

void build_works_list(CHAR_DATA *ch)
{
	BLD_WORKS *pWorks;

	if (!first_build_work)
	{
		send_to_char("Currently, there are no building works in progress.\r\n", ch);
		return;
	}

	send_to_char("List of build works currently in progress:\r\n", ch);
	
	for ( pWorks = first_build_work; pWorks; pWorks = pWorks->next )
	{
		ch_printf(ch, " %30s   Phase: [%-10s]  Hours left: %d\r\n",
			building_template[pWorks->type]->name,
			bwks_phase_descr[pWorks->phase], pWorks->timer);
	}
}


ACMD(do_listworks)
{
	build_works_list(ch);
}

/* ============================================================= */

void SaveWorksList(void)
{
	FILE *fp;
	BLD_WORKS *pWorks;
	char fname[128];

	if (!first_build_work)
		return;

	sprintf(fname, "%sworkslist.txt", LIB_BUILDINGS);
	if (!(fp = fopen(fname, "w")))
	{
		log("SYSERR: SaveWorksList() - cannot create file %s.", fname);
		return;
	}

	for (pWorks = first_build_work; pWorks; pWorks = pWorks->next)
	{
		fprintf(fp, "Work %d %hd %d %hd %hd %hd %d ",
			pWorks->num,
			pWorks->owner_type, pWorks->owner_id, (pWorks->authorized ? 1 : 0),
			pWorks->phase, pWorks->timer, pWorks->type);
		if (pWorks->coord)
			fprintf(fp, "C %hd %hd", pWorks->coord->y, pWorks->coord->x);
		else
			fprintf(fp, "R %d", pWorks->in_room->number);
		fprintf(fp, "\n");
	}

	fprintf(fp, "$\n");
	fclose(fp);
}


void ReadWorksList(void)
{
	FILE *fp;
	BLD_WORKS *pWorks;
	char fname[128], *word, letter;

	// list already loaded....
	if (first_build_work)
		return;

	// nullify head & tail
	first_build_work	= NULL;
	last_build_work		= NULL;

	last_works_num		= 0;

	sprintf(fname, "%sworkslist.txt", LIB_BUILDINGS);
	if (!(fp = fopen(fname, "r")))
		return;

	for ( ; ; )
	{
		word   = feof(fp) ? "$" : fread_word(fp);
		
		switch (UPPER(word[0]))
		{
		case '*':
			fread_to_eol( fp );
			break;

		case '$':
			fclose(fp);
			return;

		case 'W':
			CREATE(pWorks, BLD_WORKS, 1);

			pWorks->next			= NULL;
			pWorks->prev			= NULL;

			pWorks->num				= fread_number(fp);
			pWorks->owner_type		= fread_number(fp);
			pWorks->owner_id		= fread_number(fp);
			pWorks->authorized		= fread_number(fp);
			pWorks->phase			= fread_number(fp);
			pWorks->timer			= fread_number(fp);
			pWorks->type			= fread_number(fp);

			letter = fread_letter(fp);

			pWorks->coord			= NULL;
			pWorks->in_room			= NULL;

			if (letter == 'C')
			{
				CREATE(pWorks->coord, COORD_DATA, 1);
				pWorks->coord->y	= fread_number(fp);
				pWorks->coord->x	= fread_number(fp);

				pWorks->in_room		= create_wild_room(pWorks->coord, TRUE);
			}
			else
				pWorks->in_room		= get_room(fread_number(fp));

			LINK(pWorks, first_build_work, last_build_work, next, prev);

			if (last_works_num < pWorks->num)
				last_works_num = pWorks->num;
		}
	}

	fclose(fp);
}

/* ============================================================= */

/* create a new building of the given type */
void create_building(CHAR_DATA *ch, char *arg, int clan_num)
{
	char owner_type;
	bool can_build = TRUE;
	int vnum, gold, cost;
	long owner_id;


	if (!num_of_bld_templ)
	{
		send_to_char("No building can be built right now.\r\n", ch);
		return;
	}

	if (!*arg)
	{
		send_to_char("Usage: build <building type>\r\n", ch);
		show_valid_list(ch);
		return;
	}

	if ((vnum = get_bld_template_by_name(arg)) == NOTHING)
	{
		ch_printf(ch, "Unknown building type '%s'.\r\n", arg);
		show_valid_list(ch);
		return;
	}

	if (!terrain_type[SECT(ch->in_room)])
	{
		send_to_char("You cannot build in this room.\r\n", ch);
		return;
	}

	/* check invalid room sector */
	switch (SECT(ch->in_room))
	{
	case SECT_INSIDE:
	case SECT_WATER_SWIM:
	case SECT_WATER_NOSWIM:
	case SECT_UNDERWATER:
	case SECT_FLYING:
	case SECT_MOUNTAIN_PEAK:
	case SECT_REEF:
	case SECT_SEA:
	case SECT_RIVER:
	case SECT_ROAD:
	case SECT_BRIDGE:
	case SECT_WILD_EXIT:
	case SECT_ZONE_BORDER:
	case SECT_ZONE_INSIDE:
	case SECT_RIVER_NAVIGABLE:
	case SECT_SHIP_STERN:
	case SECT_SHIP_DECK:
	case SECT_SHIP_BOW:
	case SECT_SHIP_UNDERDK:
	case SECT_SHIP_CABIN:
	case SECT_SHIP_HOLD:
	case SECT_FERRY_DECK:
	case SECT_FORD:
	case SECT_SHALLOWS:
		can_build = FALSE;
		break;
	}

	if (!can_build)
	{
		ch_printf(ch, "You cannot build in '%s' rooms.\r\n",
			terrain_type[SECT(ch->in_room)]->name);
		return;
	}
	
	cost = building_template[vnum]->cost;

	if (clan_num == NOTHING)
	{
		owner_id	= GET_IDNUM(ch);
		owner_type	= BLD_OWN_CHAR;

		gold = get_gold(ch) + GET_BANK_GOLD(ch);
		
		if (gold < cost)
			gold = -1;
		else
		{
			if ( get_gold(ch) < cost )
			{
				int diff = cost - get_gold(ch);

				sub_gold(ch, get_gold(ch));
				GET_BANK_GOLD(ch) -= diff;
			}
			else
				sub_gold(ch, cost);
		}
	}
	else
	{
		CLAN_DATA *pClan = get_clan(clan_num);

		if ( !pClan )
		{
			send_to_char("Ooopss.. clan error: contact an immortal.\r\n", ch);
			log("SYSERR: create_building() - invalid clan number %d.", clan_num);
			return;
		}

		owner_id	= clan_num;
		owner_type	= BLD_OWN_CLAN;

		gold		= pClan->treasure;

		if ( gold < cost )
			gold = -1;
		else
			pClan->treasure -= cost;
	}

	if (gold == -1 && IS_MORTAL(ch))
	{
		ch_printf(ch, "You need %d gold coins to build %s %s.\r\n",
			building_template[vnum]->cost, AN(building_template[vnum]->name),
			building_template[vnum]->name);
		return;
	}

	ch_printf(ch, "You pay %d gold coins of building expenses.\r\n",
		building_template[vnum]->cost);

	if (IS_IMMORTAL(ch))
		setup_new_building(building_template[vnum], ch->in_room, owner_id, owner_type);
	else
		build_works_start(building_template[vnum], ch->in_room, owner_id, owner_type);
}


/* the build command */
ACMD(do_build)
{
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg);
	
	if (!*arg)
	{
		send_to_char("Usage: build <building type>\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg);
	if (!*arg)
	{
		show_valid_list(ch);
		return;
	}
	create_building(ch, arg, NOTHING);
}


/* show the status of a building - IMMS */
void do_stat_building(CHAR_DATA *ch, BUILDING_DATA *bld)
{
	char bflags[128];

	ch_printf(ch, "Vnum       : %d\r\n", bld->vnum);
	ch_printf(ch, "Type       : %s\r\n", bld_type_descr[bld->type->vnum]);
	ch_printf(ch, "Name       : %s\r\n", bld->description);

	if (BUILDING_FLAGGED(bld, BLD_F_RUIN))
		return;

	ch_printf(ch, "Size       : %d\r\n", bld->size);

	if (bld->coord)
		ch_printf(ch, "Location   : %d %d\r\n", GET_RY(bld), GET_RX(bld));
	else
		ch_printf(ch, "Location   : room %d\r\n", bld->vnum_room);

	sprintbit(bld->flags, bld_flags_descr, bflags);
	ch_printf(ch, "Flags      : %s\r\n", bflags);

	ch_printf(ch, "Defense    : %d / %d\r\n", bld->curr_val.defense, bld->max_val.defense);
	ch_printf(ch, "Health     : %d / %d\r\n", bld->curr_val.health, bld->max_val.health);

	ch_printf(ch, "Owner type : %s\r\n", bld_owner_descr[bld->owner_type]);
	ch_printf(ch, "Owner Id   : %d\r\n", bld->owner_id);

	ch_printf(ch, "Enter auth : %s\r\n", bld_enter_descr[bld->can_enter]);

	if (bld->max_val.npc_attack > 0)
		ch_printf(ch, "NPC damage : %d / %d.\r\n",
			bld->curr_val.npc_attack, bld->max_val.npc_attack);

	if (bld->max_val.mob_attack > 0)
	{
		int x;

		ch_printf(ch, "Mobs loaded: %d / %d.\r\n",
			bld->curr_val.mob_attack, bld->max_val.mob_attack);
		for (x = 0; x < BLD_MAX_MOB; x++)
		{
			if (bld->vmob[x] == NOTHING)
				continue;
			ch_printf(ch, "  Mob vnum : %d\r\n", bld->vmob[x]);
		}
	}
}


ACMD(do_bldlist)
{
	BUILDING_DATA *bld;
	char sbaf[MAX_STRING_LENGTH];
	int iHash;

	if (!building_loaded)
	{
		send_to_char("No buildings at all.\r\n", ch);
		return;
	}

	send_to_char("List of building in world:\r\n", ch);
	for (iHash = 0; iHash < BLD_HASH; iHash++)
	{
		for (bld = building_list[iHash]; bld; bld = bld->next)
		{
			if (IS_WILD(bld->in_room))
				sprintf(sbaf, "Wild [%d %d]", GET_Y(bld), GET_X(bld));
			else
				sprintf(sbaf, "Room [%d]", bld->in_room->number);

			ch_printf(ch, "[%d] %-30s  Location: %s\r\n",
				bld->vnum, bld->description, sbaf);
		}
	}
}

/* ================================================================= */
/* Code for saving buildings                                         */    
/* ================================================================= */

/* write on file the building inside */
void fwrite_building_rooms(BUILDING_DATA *bld, FILE *fp)
{
	ROOM_DATA *room;
	EXIT_DATA *pexit;
	char dflags[128];
	int num;

	for (num = 0; num < bld->size; num++)
	{
		room = bld->rooms[num];

		if (!room) break;

		/* Copy the description and strip off trailing newlines */
		strcpy(buf, room->description ? room->description : "Empty room.");
		strip_cr(buf);

		sprintascii(buf2, room->room_flags);

		fprintf(fp,
			"#%d\n"
			"%s~\n"
			"%s~\n"
			"%s %d %d %d\n",
			room->number,
			room->name ? room->name : "Untitled",
			buf, buf2, room->sector_type,
			room->extra_data->curr_hp, room->extra_data->max_hp
			);

		/* Now you write out the exits for the room */
		for (pexit = room->first_exit; pexit; pexit = pexit->next)
		{
			/* check for non-building exits */
			if (!IS_BUILDING(pexit->to_room))
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

/* write on file one building */
void fwrite_building(BUILDING_DATA *bld)
{
	FILE *fp;
	char fname[128];
	int num, xm;

	sprintf(fname, "%s%d.bld", LIB_BUILDINGS, bld->vnum);
	if (!(fp = fopen(fname, "w")))
	{
		log("SYSERR: fwrite_building() - unable to create file %s.", fname);
		return;
	}

	fprintf(fp, "#BUILDING\n");
	fprintf(fp, "Vnum          %d\n",	bld->vnum);
	fprintf(fp, "Type          %d\n",	bld->type->vnum);
	if (bld->coord)
		fprintf(fp, "Coord         %hd %hd\n",	GET_RY(bld), GET_RX(bld));
	if (bld->vnum_room != NOWHERE)
		fprintf(fp, "Vnum_room     %d\n",	bld->vnum_room);
	fprintf(fp, "Size          %hd\n",	bld->size);
	fprintf(fp, "Owner_Id      %ld\n",	bld->owner_id);
	fprintf(fp, "Owner_Type    %d\n",	bld->owner_type);
	fprintf(fp, "Enter_Type    %d\n",	bld->can_enter);

	fprintf(fp, "Defense       %hd %hd\n",	bld->curr_val.defense, bld->max_val.defense);
	fprintf(fp, "Health        %hd %hd\n",	bld->curr_val.health, bld->max_val.health);
	if (bld->max_val.mob_attack > 0)
		fprintf(fp, "Attack_MOB    %hd %hd\n",	bld->curr_val.mob_attack, bld->max_val.mob_attack);
	if (bld->max_val.npc_attack > 0)
		fprintf(fp, "Attack_NPC    %hd %hd\n",	bld->curr_val.npc_attack, bld->max_val.npc_attack);
	if (bld->flags)
		fprintf(fp, "Flags         %d\n",	bld->flags);

	for (xm = 0; xm < BLD_MAX_MOB; xm++)
	{
		if (bld->vmob[xm] == NOBODY)
			continue;

		fprintf(fp, "Vmob          %d\n",	bld->vmob[xm]);
	}

	if (bld->keyword)
		fprintf(fp, "Keywords      %s~\n",	bld->keyword);
	if (bld->description)
		fprintf(fp, "Description   %s~\n",	bld->description);

	if (bld->auth)
	{
		AUTH_DATA *bAuth;

		for (bAuth = bld->auth; bAuth; bAuth = bAuth->next)
			fprintf(fp, "IdAuth          %ld %s\n", bAuth->id_num, bAuth->name);
	}

	if (bld->trp)
		fprintf(fp, "TradingPost   %d\n", bld->trp->vnum);

	fprintf(fp, "End\n\n");

	if (!BUILDING_FLAGGED(bld, BLD_F_RUIN))
	{
		fprintf(fp, "#ROOMS\n");
		
		fwrite_building_rooms(bld, fp);
		
		for (num = 0; num < bld->size; num++)
		{
			if (bld->rooms[num] && bld->rooms[num]->first_content)
			{
				fprintf(fp, "#ROOMCONTENT\n");
				fprintf(fp, "Room          %d\n", num);
				save_objs( bld->rooms[num]->last_content, fp, 0, 0);
				fprintf(fp, "End\n\n");
			}
		}
	}
	
	fprintf(fp, "#END\n");
	fclose(fp);
}

/*
 * save all buildings
 *
 * called from heartbeat() in comm.c
 */
void SaveBuildings(void)
{
	FILE *fp;
	BUILDING_DATA *bld;
	char fname[128];
	int i;

	if (!top_of_buildings)
		return;

	sprintf(fname, "%sindex", LIB_BUILDINGS);
	if (!(fp = fopen(fname, "w")))
	{
		log("SYSERR: SaveBuildings() - unable to create index file %s.", fname);
		return;
	}

	for (i = 0; i < BLD_HASH; i++)
	{
		for (bld = building_list[i]; bld; bld = bld->next)
		{
			if (bld->vnum == NOTHING || !bld->type || bld->type->vnum == NOTHING)
				continue;
			
			fprintf(fp, "%d.bld\n", bld->vnum);
			
			fwrite_building(bld);
		}
	}

	fprintf(fp, "$\n");
	fclose(fp);

	SaveWorksList();
}

/* forced save of all buildings - IMMS */
ACMD(do_bldsave)
{
	SaveBuildings();
	send_to_char(OK, ch);
}


/* ================================================================= */
/* Code for loading saved buildings                                  */    
/* ================================================================= */

#define MAX_BAG_ROWS	5

/* load contents of a building room from file */
void fread_building_contents(BUILDING_DATA *bld, FILE *fp)
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
		
		switch (UPPER(word[0]))
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol(fp);
			break;

		case 'E':
			if (!strcmp(word, "End"))
				return;

		case 'R':
			KEY("Room",		roomnum,		fread_number(fp));
			break;

		case '#':
			// skip #OBJECT line..
			fread_to_eol( fp );
			
			if ((obj = fread_one_obj(fp, &location)) == NULL)
				continue;
			
			obj_to_room(obj, bld->rooms[roomnum]);
			
			for (j = MAX_BAG_ROWS - 1; j > -location; j--)
			{
				if (cont_row[j])	/* No container, back to inventory. */
				{
					for (; cont_row[j]; cont_row[j] = obj2)
					{
						obj2 = cont_row[j]->next_content;
						obj_to_room( cont_row[j], bld->rooms[roomnum] );
					}
					cont_row[j] = NULL;
				}
			}
			if (j == -location && cont_row[j])	/* Content list exists. */
			{
				if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER ||
					GET_OBJ_TYPE(obj) == ITEM_MISSILE_CONT)
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
					obj_to_room(obj, bld->rooms[roomnum]);
				}
				else	/* Object isn't container, empty content list. */
				{
					for (; cont_row[j]; cont_row[j] = obj2)
					{
						obj2 = cont_row[j]->next_content;
						obj_to_room(cont_row[j], bld->rooms[roomnum]);
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
void fread_building_rooms(BUILDING_DATA *bld, FILE *fp)
{
	EXTRA_DESCR *new_descr;
	char letter, *flags;
	int room, door;

	CREATE(bld->rooms, ROOM_DATA *, bld->size);

	for (room = 0; room < bld->size; room++)
	{
		CREATE(bld->rooms[room], ROOM_DATA, 1);
		bld->rooms[room]->zone = BUILDING_ZONE;

		CREATE(bld->rooms[room]->extra_data, ROOM_EXTRA, 1);
		bld->rooms[room]->extra_data->vnum = bld->vnum;
	}

	// in entrance room must always be light
	bld->rooms[0]->light = 1;

	if (bld->trp)
	{
		// assign to first building room the special procedure
		bld->rooms[0]->func	= tradingpost;
		// save room in TP data for reference..
		bld->trp->in_room	= bld->rooms[0];
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
			log("create_building_rooms(): # not found.");
			break;
		}
		
		room = fread_number(fp);

		if (room < 0 || room > bld->type->size)
			break;

		bld->rooms[room]->number				= room;

		bld->rooms[room]->name					= fread_string_nospace(fp);
		bld->rooms[room]->description			= fread_string_nospace(fp);
		flags									= fread_word(fp);
		bld->rooms[room]->room_flags			= asciiflag_conv(flags);
		bld->rooms[room]->sector_type			= fread_number(fp);

		bld->rooms[room]->extra_data->curr_hp	= fread_number(fp);
		bld->rooms[room]->extra_data->max_hp	= fread_number(fp);

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
					log("create_building_rooms: vnum %d has bad door number %d.", room, door);
					break;
				}
				else
				{
					EXIT_DATA *pexit;
					char flags[128];
					char *ln;
					int x1, x2;
					
					pexit = make_exit( bld->rooms[room], NULL, door );
					pexit->description	= fread_string_nospace(fp);
					pexit->keyword		= fread_string_nospace(fp);
					ln					= fread_line(fp);
					x1=x2=0;
					sscanf( ln, "%s %d %d ", &flags, &x1, &x2 );
					
					pexit->key			= x1;
					pexit->vnum			= x2;
					pexit->vdir			= door;
					pexit->exit_info	= asciiflag_conv( flags );
					
					pexit->to_room		= bld->rooms[pexit->vnum];

					/* sanity check */
					if ( pexit->exit_info && !EXIT_FLAGGED( pexit, EX_ISDOOR ) )
						SET_BIT( pexit->exit_info, EX_ISDOOR );
				}
				break;

			case 'E':
				CREATE(new_descr, EXTRA_DESCR, 1);
				new_descr->keyword		= fread_string(fp, buf2);
				new_descr->description	= fread_string(fp, buf2);
				new_descr->next			= bld->rooms[room]->ex_description;
				bld->rooms[room]->ex_description = new_descr;
				break;

			/* Room Trigger */
			case 'T':
				setup_trigger( bld->rooms[room], fp );
				break;
			}
		}

		if (letter == '$')
			break;
	}
}

/* load from file the building data */
BUILDING_DATA *fread_building_data( FILE *fp )
{
	BUILDING_DATA *bld;
	char *word;
	bool fMatch;
	sh_int vnmob = 0;

	bld = new_building();

	for ( ; ; )
	{
		word   = feof( fp ) ? "End" : fread_word( fp );
		fMatch = FALSE;
		
		switch (UPPER(word[0]))
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol( fp );
			break;

		case 'A':
			if (!strcmp(word, "Attack_MOB"))
			{
				bld->curr_val.mob_attack	= fread_number(fp);
				bld->max_val.mob_attack		= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if (!strcmp(word, "Attack_NPC"))
			{
				bld->curr_val.npc_attack	= fread_number(fp);
				bld->max_val.npc_attack		= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'C':
			if (!strcmp(word, "Coord"))
			{
				CREATE(bld->coord, COORD_DATA, 1);

				bld->coord->y				= fread_number(fp);
				bld->coord->x				= fread_number(fp);

				fMatch = TRUE;
				break;
			}
			break;

		case 'D':
			KEY("Description",	bld->description,		fread_string_nospace(fp));
			if (!strcmp(word, "Defense"))
			{
				bld->curr_val.defense		= fread_number(fp);
				bld->max_val.defense		= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'E':
			KEY("Enter_Type",	bld->can_enter,			fread_number(fp));
			if (!strcmp(word, "End"))
			{
				if (!bld->type || (!bld->coord && !bld->vnum_room))
				{
					bld->type	= NULL;
					if (bld->coord)
						DISPOSE(bld->coord);
					DISPOSE(bld);
					return (NULL);
				}

				return (bld);
			}
			break;

		case 'F':
			KEY("Flags",		bld->flags,				fread_number(fp));
			break;

		case 'H':
			if (!strcmp(word, "Health"))
			{
				bld->curr_val.health	= fread_number(fp);
				bld->max_val.health		= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'I':
			if (!strcmp(word, "IdAuth"))
			{
				AUTH_DATA *bAuth;

				CREATE(bAuth, AUTH_DATA, 1);
				bAuth->id_num		= fread_number(fp);
				bAuth->name			= str_dup(fread_word(fp));

				bAuth->next			= bld->auth;
				bld->auth			= bAuth;
				
				fMatch = TRUE;
				break;
			}
			break;

		case 'K':
			KEY("Keywords",		bld->keyword,			fread_string_nospace(fp));
			break;

		case 'O':
			KEY("Owner_Id",		bld->owner_id,			fread_number(fp));
			KEY("Owner_Type",	bld->owner_type,		fread_number(fp));
			break;

		case 'S':
			KEY("Size",			bld->size,				fread_number(fp));
			break;

		case 'T':
			KEY("Type",			bld->type,				get_bld_template(fread_number(fp)));
			if (!strcmp(word, "TradingPost"))
			{
				TRADING_POST *pTp;
				int tpnum;

				fMatch = TRUE;
				tpnum = fread_number(fp);

				if (!(pTp = find_trading_post(tpnum)))
				{
					log("SYSERR: fread_building_data() - invalid trading post vnum %d.", tpnum);
					break;
				}

				// give building trading post status
				bld->trp = pTp;

				break;
			}
			break;

		case 'V':
			KEY("Vnum",			bld->vnum,				fread_number(fp));
			KEY("Vnum_room",	bld->vnum_room,			fread_number(fp));
			if (!strcmp(word, "Vmob"))
			{
				mob_vnum vnmobnum = fread_number(fp);

				if (vnmob >= BLD_MAX_MOB)
					log("SYSERR: fread_building_data() - too many defending mobs.");
				else
					bld->vmob[vnmob++]	= vnmobnum;

				fMatch = TRUE;
				break;
			}
			break;
		}
	}

	return (NULL);
}

/* load from file one building */
BUILDING_DATA *fread_building(char *bname)
{
	FILE *fp;
	BUILDING_DATA *bld = NULL;
	char fname[128];
	char letter;
	char *word;

	sprintf(fname, "%s%s", LIB_BUILDINGS, bname);
	if (!(fp = fopen(fname, "r")))
	{
		log("SYSERR: fread_building() - unable to open file %s.", fname);
		return (NULL);
	}

	for ( ;; )
	{
		letter = fread_letter(fp);
		if (letter == '*')
		{
			fread_to_eol( fp );
			continue;
		}
		
		if (letter != '#')
		{
			log( "fread_building: # not found." );
			break;
		}
		
		word = fread_word(fp);

		if (!strcmp(word, "BUILDING"))
		{
			if (!(bld = fread_building_data(fp)))
			{
				log("SYSERR: fread_building() - error in reading data for building file %s.", bname);
				break;
			}
		}
		else if (!strcmp(word, "ROOMS"))
			fread_building_rooms(bld, fp);
		else if (!strcmp(word, "ROOMCONTENT"))
			fread_building_contents(bld, fp);
		else if (!strcmp(word, "END"))
			break;
		else
		{
			log("fread_building: bad section %s.", word);
			continue;
		}
	}

	fclose(fp);
	return (bld);
}

/*
 * load all buildings
 *
 * called from ???
 */
void LoadBuildings(void)
{
	FILE *fp;
	BUILDING_DATA *bld;
	EXIT_DATA *pexit;
	char fname[128];
	int iHash;

	if (building_loaded)
	{
		log("SYSERR: LoadBuildings() - already loaded.");
		return;
	}

	sprintf(fname, "%sindex", LIB_BUILDINGS);
	if (!(fp = fopen(fname, "r")))
	{
		log("   No building templates defined.");
		return;
	}

	building_loaded				= TRUE;
	top_of_buildings			= 0;

	while (!feof(fp))
	{
		char *bldfname = fread_word( fp );

		if (bldfname[0] == '$')
			break;

		if (!(bld = fread_building(bldfname)))
			continue;

		building_to_room(bld, NULL);

		if (!BUILDING_FLAGGED(bld, BLD_F_RUIN))
		{
			/* exit building --> wilderness */
			pexit = make_exit(bld->rooms[bld->type->entrance], NULL, SOUTH);
			if (bld->vnum_room != NOWHERE)
				pexit->to_room	= get_room(bld->vnum_room);
			else
				pexit->to_room	= get_wild_room(bld->coord);
			pexit->keyword		= str_dup("door");
			SET_BIT(EXIT_FLAGS(pexit), EX_ISDOOR);
		}

		// execute building commands (load objs & mobs)
		exec_bld_cmds(bld);

		/* add to the global hash table */
		iHash					= bld->vnum % BLD_HASH;
		bld->next				= building_list[iHash];
		building_list[iHash]	= bld;

		top_of_buildings++;
	}

	fclose(fp);

	ReadWorksList();
}

/* ======================================================================= */
/* Code for repairing buildings                                            */
/* ======================================================================= */

/* make some reparations */
void repair_building(CHAR_DATA *ch, BUILDING_DATA *bld)
{
	if (BUILDING_FLAGGED(bld, BLD_F_RUIN))
	{
		act("$b: you cannot repair ruins.", FALSE, ch, NULL, bld, TO_CHAR);
		return;
	}

	if (bld->max_val.health == bld->curr_val.health)
	{
		act("$b does not need reparations.", FALSE, ch, NULL, bld, TO_CHAR);
		return;
	}

	if (!roll(GET_DEX(ch)))
	{
		send_to_char("You fail.\r\n", ch);
		return;
	}

	bld->curr_val.health++;
	act("You make sone reparations to $b.", FALSE, ch, NULL, bld, TO_CHAR);
	act("$n makes sone reparations to $b.", FALSE, ch, NULL, bld, TO_ROOM);
}


/* ============================================================================ */
/* Code for attacking, damaging and repairing buildings, plus buildings defense */
/* ============================================================================ */

/*
 * some buildings, in response of an attack,
 * can "generate" some mobs to fight the attacker
 *
 * this code pick a defender mob at random from
 * the list of possible defenders mobs
 */
CHAR_DATA *get_bld_defmob(BUILDING_DATA *bld)
{
	CHAR_DATA *mob;
	int xn, nlast, num;

	for (nlast = -1, xn = 0; xn < BLD_MAX_MOB; xn++)
	{
		if (bld->vmob[xn] == NOBODY)
			break;
		nlast = xn;
	}

	/* no mobs.. hmmmm */
	if (nlast == -1)
	{
		log("SYSERR: get_bld_defmob() - cannot find a valid mob vnum.");
		return (NULL);
	}

	if (nlast == 0)	num = nlast;				// single mob
	else			num = number(0, nlast);		// two or more mobs

	if (!(mob = read_mobile(bld->vmob[num], VIRTUAL)))
	{
		log("SYSERR: get_bld_defmob() - invalid mob vnum %d.", bld->vmob[num]);
		return (NULL);
	}

	return (mob);
}

/*
 * called when a building is destroyed
 */
void damage_people_inside(BUILDING_DATA *bld)
{
	CHAR_DATA *tch, *tch_next = NULL;
	EXIT_DATA *pexit;
	bool saved = FALSE;
	
	pexit = get_exit(bld->rooms[bld->type->entrance], SOUTH);
	if (pexit && !valid_exit(pexit))
		pexit = NULL;

	for (tch = bld->people; tch; tch = tch_next)
	{
		tch_next = tch->next_in_building;

		act("$b collapses and crumbles.", FALSE, tch, NULL, bld, TO_CHAR);

		saved = FALSE;
		/*
		 * if tch is in the entrance room, and if the door is open, and if he makes
		 * a check on dex at -4, arf, he can jump out of the building..
		 */
		if (tch->in_room == bld->rooms[bld->type->entrance] && pexit &&
			roll(GET_DEX(tch) - 4))
		{
			act("You quickly leap out of $b's door one moment before it crumbles.",
				FALSE, tch, NULL, bld, TO_CHAR);
			act("$n quickly leaps out of $b's door one moment before it crumbles.",
				FALSE, tch, NULL, bld, TO_ROOM);
			char_from_building(tch);
			char_from_room(tch);
			char_to_room(tch, bld->in_room);
			act("$n leaps out of $b'door one moment before it crumbles.",
				FALSE, tch, NULL, bld, TO_ROOM);
			continue;
		}

		// take him out so, if he die, we place the corpse OUTSIDE the building
		char_from_building(tch);
		char_from_room(tch);
		char_to_room(tch, bld->in_room);
		
		send_to_char("You are hit by falling rubbles.\r\n", tch);

		// char suffer massive damage.. :-(((
		damage(tch, tch, number(50,100) * (bld->type->vnum + 1), -1);
	}
}

/*
 * adjust destroyed building descriptions with
 * ruins of the correct type
 */
void make_ruins(BUILDING_DATA *bld)
{
	char dbuf[MAX_STRING_LENGTH];

	SET_BIT(BUILDING_FLAGS(bld), BLD_F_RUIN);

	sprintf(dbuf, "ruins %s", bld->type->name);
	if (bld->keyword)
		DISPOSE(bld->keyword);
	bld->keyword = str_dup(dbuf);

	sprintf(dbuf, "the ruins of %s %s", AN(bld->type->name), bld->type->name);
	if (bld->description)
		DISPOSE(bld->description);
	bld->description = str_dup(dbuf);
}

/*
 * return 0 if building is destroyed, >0 otherwise
 */
int damage_building(BUILDING_DATA *bld, int dam)
{
	bld->curr_val.health -= dam;

	if (bld->curr_val.health <= 0)
	{
		bld->curr_val.health = 0;
		damage_people_inside(bld);
		make_ruins(bld);
		fwrite_building(bld);		
		return (0);
	}

	return (1);
}

/* one hit to a building */
void hit_building(CHAR_DATA *ch, BUILDING_DATA *bld)
{
	OBJ_DATA *weapon;
	char dbuf[MAX_STRING_LENGTH];
	int dam;

	if (!ch || !bld)
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

	// subtract building defense
	dam -= bld->curr_val.defense;

	// damage goes from 0 to 100
	dam = URANGE(0, dam, 100);

	// check for damaging the weapon used
	check_damage_obj(ch, weapon, 8);

	/* no damage done */
	if (!dam)
	{
		// message to attacker
		act("You strike $b, but make no damages.", FALSE, ch, NULL, bld, TO_CHAR);
		// message to room
		act("$n strikes $b but nothing happens.", FALSE, ch, NULL, bld, TO_ROOM);

		/*
		 * some buildings can auto-respond to attacks..
		 * ie: towers have npc archers which fires upon attacking ppl
		 */
		if (bld->curr_val.npc_attack)
		{
			damage(ch, ch, bld->curr_val.npc_attack, -1);
			act("You are hit by arrows fired at you from $b.", FALSE, ch, NULL, bld, TO_CHAR);
			act("$n is being hit by arrows fired at $m from $b.", FALSE, ch, NULL, bld, TO_ROOM);
		}
		
		return;
	}
	
	/* damage the building */

	// if 0 the building has been destroyed
	if (!damage_building(bld, dam))
	{
		// message to attacker
		act("Your attack DESTROY $b.", FALSE, ch, NULL, bld, TO_CHAR);
		// message to room
		act("$n DESTROYS $b with $s attack.", FALSE, ch, NULL, bld, TO_ROOM);
		return;
	}
	
	// message to attacker
	sprintf(dbuf, "You strike $b, causing %d damages.", dam);
	act(dbuf, FALSE, ch, NULL, bld, TO_CHAR);
	// message to room
	act("$n strike $b, causing some damage.", FALSE, ch, NULL, bld, TO_ROOM);
	
	/*
	 * some buildings can auto-respond to attacks..
	 * ie: towers could have npc archers which fires upon attacking ppl
	 */
	if (bld->curr_val.npc_attack)
	{
		damage(ch, ch, bld->curr_val.npc_attack, -1);
		act("You are hit by arrows fired at you from $b.", FALSE, ch, NULL, bld, TO_CHAR);
		act("$n is being hit by arrows fired at $m from $b.", FALSE, ch, NULL, bld, TO_ROOM);
	}
	
	/*
	 * some buildings, in response of an attack,
	 * can "generate" mobs to fight the attacker
	 */
	if (bld->curr_val.mob_attack > 0)
	{
		CHAR_DATA *mob = get_bld_defmob(bld);
		
		if (mob)
		{
			bld->curr_val.mob_attack--;
			// message to vict
			sprintf(dbuf, "$N exit from %s and attacks you.", bld->description);
			act(dbuf, FALSE, ch, NULL, mob, TO_CHAR);
			// message to room
			sprintf(dbuf, "$N exit from %s and attacks $n.", bld->description);
			act(dbuf, FALSE, ch, NULL, mob, TO_ROOM);
			
			char_to_room(mob, ch->in_room);
			hit(mob, ch, -1);
		}
	}
}


/* one single attack to a building */
bool attack_building(CHAR_DATA *ch, char *arg)
{
	BUILDING_DATA *bld;

	if (!*arg)
		return (FALSE);

	if (!(bld = find_building_in_room_by_name(ch->in_room, arg)))
		return (FALSE);

	if (BUILDING_FLAGGED(bld, BLD_F_RUIN))
	{
		ch_printf(ch, "%s %s has already been destroyed.\r\n",
			AN(bld->type->name), bld->type->name);
		return (TRUE);
	}

	hit_building(ch, bld);
	return (TRUE);
}

/* ============================================================================ */
/* AUTHORIZE Code                                                               */
/* ============================================================================ */

AUTH_DATA *get_ship_auth( SHIP_DATA *ship, long idnum );
void auth_aboard( SHIP_DATA *ship, CHAR_DATA *ch, CHAR_DATA *vict, int mode );

ACMD(do_authorize)
{
	BUILDING_DATA *pBld = NULL;
	SHIP_DATA *pShip = NULL;
	AUTH_DATA *bAuthList = NULL, *bAuth = NULL;
	CHAR_DATA *vict;
	char arg[MAX_INPUT_LENGTH], sbuf[MAX_STRING_LENGTH];

	if (!ch) return;

	if (!(pBld = ch->in_building) && !(pShip = ch->in_ship))
	{
		send_to_char("You can use this command only when inside a building or aboard a ship.\r\n", ch);
		return;
	}

	argument = one_argument(argument, arg);
	
	if (!*arg)
	{
		send_to_char(
			"Usage:\r\n"
			"-------------------------------------------------------------\r\n"
			"authorize list    show list of authorized people\r\n"
			"authorize <name>  set or revoke authorization to player 'name'\r\n"
			"-------------------------------------------------------------\r\n"
			, ch);
		return;
	}

	if (!str_cmp(arg, "list"))
	{
		if (pBld)
		{
			if (!can_enter_bld(ch, pBld, TRUE))
			{
				send_to_char("You have no rights, go away.\r\n", ch);
				return;
			}
			bAuthList = pBld->auth;
		}
		else if (pShip)
		{
			if (GET_IDNUM(ch) != pShip->idcaptain && GET_IDNUM(ch) != pShip->idowner)
			{
				send_to_char("You must be the captain or the owner to authorize people.\r\n", ch );
				return;
			}
			bAuthList = pShip->authorized;
		}
		else
			bAuthList = NULL;

		if (!bAuthList)
		{
			send_to_char("No one is authorized.\r\n", ch);
			return;
		}

		strcpy(sbuf, "List of authorized people:\r\n");
		for (bAuth = bAuthList; bAuth; bAuth = bAuth->next)
			sprintf(sbuf + strlen(sbuf), "%s\r\n", bAuth->name);

		page_string(ch->desc, sbuf, 0);
		return;
	}

	if (pBld)
	{
		if (!(vict = get_player_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
		{
			ch_printf(ch, "No one called '%' is in the game at the moment.\r\n", arg);
			return;
		}

		/* new introduction system */
		if ( !is_char_known(ch, vict) )
		{
			send_to_char("You cannot authorize someone you don't know.\r\n", ch );
			return;
		}

		if (!can_enter_bld(ch, pBld, TRUE))
		{
			send_to_char("You have no rights, go away.\r\n", ch);
			return;
		}

		if (get_bld_auth(pBld, GET_IDNUM(vict)))
			auth_enter(pBld, ch, vict, 0);
		else
			auth_enter(pBld, ch, vict, 1);
	}
	else if (pShip)
	{
		ROOM_DATA *pRoom;

		if (!SHIP_FLAGGED(pShip, SHIP_IN_PORT) || !(pRoom = get_wild_room(pShip->port)))
		{
			send_to_char("You can authorize people only when ship is at one port.\r\n", ch);
			return;
		}

		if (!(vict = get_char_elsewhere_vis(ch, pRoom, arg, NULL)))
		{
			ch_printf( ch, "No one called '%s' is in the port.\r\n", arg );
			return;
		}

		/* new introduction system */
		if (!is_char_known(ch, vict))
		{
			send_to_char("You cannot authorize someone you don't know.\r\n", ch );
			return;
		}

		if (GET_IDNUM(ch) != pShip->idcaptain && GET_IDNUM(ch) != pShip->idowner)
		{
			send_to_char("You must be the captain or the owner to authorize people.\r\n", ch );
			return;
		}

		/* toggle authorization */
		if (get_ship_auth(pShip, GET_IDNUM(vict)))
			auth_aboard(pShip, ch, vict, 0);
		else
			auth_aboard(pShip, ch, vict, 1);
	}
}


