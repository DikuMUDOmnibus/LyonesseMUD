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
*   File: handler.c                                     Part of CircleMUD *
*  Usage: internal funcs: moving and finding chars/objs                   *
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
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"

/* external vars */
extern CHAR_DATA		*combat_list;
extern SPELL_INFO_DATA	spell_info[];
extern const char		*MENU;
extern bool				LoadingCharObj;

/* external functions */
ACMD(do_return);
char	*take_name_keyword(CHAR_DATA *ch, CHAR_DATA *viewer);
void	remove_follower(CHAR_DATA *ch);
void	clearMemory(CHAR_DATA *ch);
void	die_follower(CHAR_DATA *ch);
void	check_wild_move(CHAR_DATA *ch, VEHICLE_DATA *vehicle, SHIP_DATA *ship, FERRY_DATA *ferry, ROOM_DATA *pRoomFrom, ROOM_DATA *pRoomTo);
void	wild_remove_dequeue(ROOM_DATA *wRoom);
void	wild_check_for_remove(ROOM_DATA *wRoom);
void	stop_hunting(CHAR_DATA *ch);
void	char_from_building(CHAR_DATA *ch);
void	char_from_ship(CHAR_DATA *ch);
void	mob_from_yoke(CHAR_DATA *mob, VEHICLE_DATA *vehicle);

/* local functions */
int		apply_ac(CHAR_DATA *ch, int eq_pos);
void	update_object(OBJ_DATA *obj, int use);
void	update_char_objects(CHAR_DATA *ch);

/* local vars */
int		extractions_pending		= 0;

/* ================================================================= */

char *fname(const char *namelist)
{
	static char holder[30];
	char *point;
	
	for (point = holder; isalpha(*namelist); namelist++, point++)
		*point = *namelist;
	
	*point = '\0';
	
	return (holder);
}


#define WHITESPACE " \t"

int isname( const char *str, const char *namelist )
{
	char *newlist;
	char *curtok;
	
	newlist = strdup( namelist ); /* make a copy since strtok 'modifies' strings */
	
	for ( curtok = strtok( newlist, WHITESPACE ); curtok; curtok = strtok( NULL, WHITESPACE ) )
	
	if ( curtok && is_abbrev( str, curtok ) )
	{
		free( newlist );
		return ( 1 );
	}
	
	free( newlist );
	return ( 0 );
}


/* *************************************************************** */
/* Affection Routines                                              */
/* *************************************************************** */

/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
	AFFECTED_DATA *af, *next;
	ROOM_AFFECT *raff, *next_raff, *temp;
	CHAR_DATA *i;
	
	for (i = character_list; i; i = i->next)
	{
		for (af = i->affected; af; af = next)
		{
			next = af->next;
			if (af->duration >= 1)
				af->duration--;
			else if (af->duration == -1)	/* No action */
				af->duration = -1;	/* GODs only! unlimited */
			else
			{
				if ((af->type > 0) && (af->type <= MAX_SPELLS))
				{
					if (!af->next || (af->next->type != af->type) ||
						(af->next->duration > 0))
					{
						if (spell_info[af->type].wear_off_msg)
						{
							send_to_char(spell_info[af->type].wear_off_msg, i);
							send_to_char("\r\n", i);
						}
						/* instert here special case spells */
						switch (af->type)
						{
						case SPELL_MINOR_TRACK:
						case SPELL_MAJOR_TRACK:
							stop_hunting(i);
							break;
						}
						affect_remove(i, af);
					}
				}
			}
		}
	}

	for (raff = raff_list; raff; raff = next_raff)
	{
		next_raff = raff->next;

		if (--raff->timer <= 0)
		{
			ROOM_DATA *pRoom = NULL;
			
			if (raff->vroom == NOWHERE && !raff->coord)
				log("SYSERR: room affection without room and coord");
			else
			{
				if (raff->coord)
					pRoom = get_wild_room(raff->coord);
				else
					pRoom = get_room(raff->vroom);

				if (pRoom)
				{
					if (raff->spell > 0 && raff->spell <= MAX_SPELLS)
					{
						/* this room affection has expired */
						send_to_room(spell_info[raff->spell].wear_off_msg, pRoom);
						send_to_room("\r\n", pRoom);
					}
					else if (raff->text)
						send_to_room(raff->text, pRoom);

					/* remove the affection */
					REMOVE_FROM_LIST(raff, pRoom->affections, next_in_room);
					REMOVE_BIT(ROOM_AFFS(pRoom), raff->bitvector);

					/* check for wilderness removal */
					if (IS_WILD(pRoom) && !pRoom->affections)
						wild_check_for_remove(pRoom);
				}
				else
					log("SYSERR: room affection without room");
			}

			if (raff->text)
				free(raff->text);
			REMOVE_FROM_LIST(raff, raff_list, next);
			DISPOSE(raff);
		}
	}

}


void affect_modify(CHAR_DATA *ch, byte loc, sbyte mod, bitvector_t bitv, bool add)
{
	if (add)
		SET_BIT(AFF_FLAGS(ch), bitv);
	else
	{
		REMOVE_BIT(AFF_FLAGS(ch), bitv);
		mod = -mod;
	}
	
	switch (loc)
	{
	case APPLY_NONE:
		break;
		
	case APPLY_STR:
		GET_STR(ch) += mod;
		break;
	case APPLY_DEX:
		GET_DEX(ch) += mod;
		break;
	case APPLY_INT:
		GET_INT(ch) += mod;
		break;
	case APPLY_WIS:
		GET_WIS(ch) += mod;
		break;
	case APPLY_CON:
		GET_CON(ch) += mod;
		break;
	case APPLY_CHA:
		GET_CHA(ch) += mod;
		break;
		
	case APPLY_CLASS:
		/* ??? GET_CLASS(ch) += mod; */
		break;
		
		/*
		* My personal thoughts on these two would be to set the person to the
		* value of the apply.  That way you won't have to worry about people
		* making +1 level things to be imp (you restrict anything that gives
		* immortal level of course).  It also makes more sense to set someone
		* to a class rather than adding to the class number. -gg
		*/
		
	case APPLY_LEVEL:
		/* ??? GET_LEVEL(ch) += mod; */
		break;
		
	case APPLY_AGE:
		ch->player.time.birth -= (mod * SECS_PER_MUD_YEAR);
		break;
		
	case APPLY_CHAR_WEIGHT:
		GET_WEIGHT(ch) += mod;
		break;
		
	case APPLY_CHAR_HEIGHT:
		GET_HEIGHT(ch) += mod;
		break;
		
	case APPLY_MANA:
		GET_MAX_MANA(ch) += mod;
		break;
		
	case APPLY_HIT:
		GET_MAX_HIT(ch) += mod;
		break;
		
	case APPLY_MOVE:
		GET_MAX_MOVE(ch) += mod;
		break;
		
	case APPLY_GOLD:
		break;
		
	case APPLY_EXP:
		break;
		
	case APPLY_AC:
		GET_AC(ch) += mod;
		break;
		
	case APPLY_HITROLL:
		GET_HITROLL(ch) += mod;
		break;
		
	case APPLY_DAMROLL:
		GET_DAMROLL(ch) += mod;
		break;
		
	case APPLY_SAVING_PARA:
		GET_SAVE(ch, SAVING_PARA) += mod;
		break;
		
	case APPLY_SAVING_ROD:
		GET_SAVE(ch, SAVING_ROD) += mod;
		break;
		
	case APPLY_SAVING_PETRI:
		GET_SAVE(ch, SAVING_PETRI) += mod;
		break;
		
	case APPLY_SAVING_BREATH:
		GET_SAVE(ch, SAVING_BREATH) += mod;
		break;
		
	case APPLY_SAVING_SPELL:
		GET_SAVE(ch, SAVING_SPELL) += mod;
		break;
		
	default:
		log("SYSERR: Unknown apply adjust %d attempt (%s, affect_modify).", loc, __FILE__);
		break;	
	}
}



/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */
void affect_total(CHAR_DATA *ch)
{
	AFFECTED_DATA *af;
	int i, j;
	
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
			for (j = 0; j < MAX_OBJ_AFF; j++)
				affect_modify(ch, GET_EQ(ch, i)->affected[j].location,
					GET_EQ(ch, i)->affected[j].modifier,
					GET_EQ(ch, i)->obj_flags.bitvector, FALSE);
	}
	
	for (af = ch->affected; af; af = af->next)
		affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);
	
	ch->aff_abils = ch->real_abils;
	
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
			for (j = 0; j < MAX_OBJ_AFF; j++)
				affect_modify(ch, GET_EQ(ch, i)->affected[j].location,
					GET_EQ(ch, i)->affected[j].modifier,
					GET_EQ(ch, i)->obj_flags.bitvector, TRUE);
	}
	
	
	for (af = ch->affected; af; af = af->next)
		affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
	
	/* Make certain values are between 0..25, not < 0 and not > 25! */
	
	i = (IS_NPC(ch) || IS_IMMORTAL(ch) ? 25 : 18);
	
	GET_DEX(ch) = MAX(0, MIN(GET_DEX(ch), i));
	GET_INT(ch) = MAX(0, MIN(GET_INT(ch), i));
	GET_WIS(ch) = MAX(0, MIN(GET_WIS(ch), i));
	GET_CON(ch) = MAX(0, MIN(GET_CON(ch), i));
	GET_CHA(ch) = MAX(0, MIN(GET_CHA(ch), i));
	GET_STR(ch) = MAX(0, MIN(GET_STR(ch), i));
}



/*
 * Insert an affect_type in a char_data structure
 * Automatically sets apropriate bits and apply's
 */
void affect_to_char(CHAR_DATA *ch, AFFECTED_DATA *af)
{
	AFFECTED_DATA *affected_alloc;
	
	CREATE(affected_alloc, AFFECTED_DATA, 1);
	
	*affected_alloc			= *af;

	affected_alloc->next	= ch->affected;
	ch->affected			= affected_alloc;
	
	affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
	affect_total(ch);
}



/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */
void affect_remove(CHAR_DATA *ch, AFFECTED_DATA *af)
{
	AFFECTED_DATA *temp;
	
	if (ch->affected == NULL)
	{
		core_dump();
		return;
	}
	
	affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);
	REMOVE_FROM_LIST(af, ch->affected, next);
	free(af);
	affect_total(ch);
}



/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_char(CHAR_DATA *ch, int type)
{
	AFFECTED_DATA *hjp, *next;
	
	for (hjp = ch->affected; hjp; hjp = next)
	{
		next = hjp->next;
		if (hjp->type == type)
			affect_remove(ch, hjp);
	}
}



/*
 * Return TRUE if a char is affected by a spell (SPELL_XXX),
 * FALSE indicates not affected.
 */
bool affected_by_spell(CHAR_DATA *ch, int type)
{
	AFFECTED_DATA *hjp;
	
	for (hjp = ch->affected; hjp; hjp = hjp->next)
		if (hjp->type == type)
			return (TRUE);
		
	return (FALSE);
}


AFFECTED_DATA *aff_by_bitvector( CHAR_DATA *ch, int bitv )
{
	AFFECTED_DATA *hjp;
	
	for (hjp = ch->affected; hjp; hjp = hjp->next)
		if ( hjp->bitvector == bitv )
			break;
		
	return (hjp);
}

void affect_join(CHAR_DATA *ch, AFFECTED_DATA *af, bool add_dur, bool avg_dur, bool add_mod, bool avg_mod)
{
	AFFECTED_DATA *hjp, *next;
	bool found = FALSE;
	
	for (hjp = ch->affected; !found && hjp; hjp = next)
	{
		next = hjp->next;
		
		if ((hjp->type == af->type) && (hjp->location == af->location))
		{
			if (add_dur)
				af->duration += hjp->duration;
			if (avg_dur)
				af->duration /= 2;
			
			if (add_mod)
				af->modifier += hjp->modifier;
			if (avg_mod)
				af->modifier /= 2;
			
			affect_remove(ch, hjp);
			affect_to_char(ch, af);
			found = TRUE;
		}
	}

	if (!found)
		affect_to_char(ch, af);
}


/* **************************************************************** */
/* Room Affection Routines											*/
/* **************************************************************** */

/*
 * Display all the affections in a room
 */
void print_room_aff( CHAR_DATA *ch, ROOM_DATA *pRoom )
{
	ROOM_AFFECT *raff;

	if (!ch || !ch->desc || !ch->in_room)
		return;

	if (!pRoom)
		pRoom = ch->in_room;

	for (raff = pRoom->affections; raff; raff = raff->next_in_room)
	{
		if ( raff->bitvector == RAFF_FOG )
			send_to_char("Your view is obscured by a thick fog.\r\n", ch);

		if ( raff->bitvector == RAFF_SHIELD )
			send_to_char("Surrounding the area is a shimmering shield, distorting everything you see.\r\n",ch);

		if ( raff->bitvector == RAFF_CAMP )
		{
			if (raff->timer > 50)
				send_to_char("A new campfire blares brightly..heating the whole area.\r\n",ch);
			else if (raff->timer > 40)
				send_to_char("A campfire burns here, steadily shooting off flame and smoke.\r\n",ch);
			else if (raff->timer > 30)
				send_to_char("A campfire sputters here...under the flames the timber glows hot orange.\r\n",ch);
			else if (raff->timer > 20)
				send_to_char("An old campfire is here..the flames are out but the timber glows hot red.\r\n",ch);
			else if (raff->timer > 10)
				send_to_char("An old campfire is here...its timber is almost out..on its last breath.\r\n",ch);
			else
				send_to_char("An old campfire is here..no flames or hot timber remain.\r\n",ch);
			
		}
	}
}


ROOM_AFFECT *get_room_aff(ROOM_DATA *pRoom, int spl)
{
	ROOM_AFFECT *raff;

	if (!pRoom->affections)
		return (NULL);

	for (raff = pRoom->affections; raff; raff = raff->next_in_room)
	{
		if (raff->spell == spl)
			break;
	}

	return (raff);
}


ROOM_AFFECT *get_room_aff_bitv(ROOM_DATA *pRoom, int bitv)
{
	ROOM_AFFECT *raff;

	if (!pRoom->affections)
		return (NULL);

	for (raff = pRoom->affections; raff; raff = raff->next_in_room)
	{
 		if (raff->bitvector == bitv)
			break;
	}

	return (raff);
}

/* **************************************************************** */

/* 
 * dismount_char() / fr: Daniel Koepke (dkoepke@california.com)
 *   If a character is mounted on something, we dismount them.  If
 *   someone is mounting our character, then we dismount that someone.
 *   This is used for cleaning up after a mount is cancelled by
 *   something (either intentionally or by death, etc.)
 */
void dismount_char(CHAR_DATA *ch)
{
	if (RIDING(ch))
	{
		RIDDEN_BY(RIDING(ch))	= NULL;
		RIDING(ch)				= NULL;
	}
	
	if (RIDDEN_BY(ch))
	{
		RIDING(RIDDEN_BY(ch))	= NULL;
		RIDDEN_BY(ch)			= NULL;
	}
}

/*
 * mount_char() / fr: Daniel Koepke (dkoepke@california.com)
 *   Sets _ch_ to mounting _mount_.  This does not make any checks
 *   what-so-ever to see if the _mount_ is mountable, etc.  That is
 *   left up to the calling function.  This does not present any
 *   messages, either.
 */
void mount_char(CHAR_DATA *ch, CHAR_DATA *mount)
{
	RIDING(ch)			= mount;
	RIDDEN_BY(mount)	= ch;
}

/* **************************************************************** */

/* move a player out of a room */
void char_from_room(CHAR_DATA *ch)
{
	CHAR_DATA *temp;
	
	if (ch == NULL || ch->in_room == NULL)
	{
		log("SYSERR: NULL character or NOWHERE in %s, char_from_room", __FILE__);
		return;
	}
	
	if (FIGHTING(ch) != NULL)
		stop_fighting(ch);

	if (ch->in_obj)
		char_from_obj(ch);
	
	if (GET_EQ(ch, WEAR_LIGHT) != NULL)
	{
		if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
			if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))	/* Light is ON */
				ch->in_room->light--;
	}
	
	ch->last_room		= ch->in_room;

	REMOVE_FROM_LIST(ch, ch->in_room->people, next_in_room);
	ch->in_room			= NULL;
	ch->next_in_room	= NULL;

	if ( IS_WILD(ch->last_room) )
		wild_check_for_remove(ch->last_room);
}


/* place a character in a room */
void char_to_room(CHAR_DATA *ch, ROOM_DATA *room)
{
	if (ch == NULL || room == NULL)
	{
		log("SYSERR: Illegal value(s) passed to char_to_room.");
		return;
	}
	
	ch->next_in_room	= room->people;
	room->people		= ch;

	ch->in_room			= room;
	
	if (GET_EQ(ch, WEAR_LIGHT))
	{
		if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
			if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))	/* Light ON */
				room->light++;
	}
	
	/* Stop fighting now, if we left. */
	if (FIGHTING(ch) && IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)))
	{
		stop_fighting(FIGHTING(ch));
		stop_fighting(ch);
	}

	/* Handle Wild Sectors */
	if ( IS_WILD(room) )
	{
		check_wild_move( ch, NULL, NULL, NULL, ch->last_room, room );

		if (ROOM_FLAGGED(room, ROOM_WILD_REMOVE))
			wild_remove_dequeue(room);
	}
}

/* char stop using a furniture obj */
void char_from_obj(CHAR_DATA *ch)
{
	CHAR_DATA *temp;

	if ( !ch || !ch->in_obj )
		return;

	if (FIGHTING(ch) != NULL)
		stop_fighting(ch);
	
	if (GET_EQ(ch, WEAR_LIGHT) != NULL)
	{
		if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
			if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))	/* Light is ON */
				ch->in_obj->light--;
	}

	REMOVE_FROM_LIST(ch, ch->in_obj->people, next_in_obj);
	ch->in_obj			= NULL;
	ch->next_in_obj		= NULL;
}

/* char start using a furniture obj */
void char_to_obj(CHAR_DATA *ch, OBJ_DATA *furn)
{
	if ( !ch || !furn || GET_OBJ_TYPE(furn) != ITEM_FURNITURE )
		return;

	ch->next_in_obj		= furn->people;
	furn->people		= ch;

	ch->in_obj			= furn;

	if (GET_EQ(ch, WEAR_LIGHT))
	{
		if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
			if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2))	/* Light ON */
				furn->light++;
	}

	/* Stop fighting now, if we left. */
	if (FIGHTING(ch) && IN_ROOM(ch) != IN_ROOM(FIGHTING(ch)))
	{
		stop_fighting(FIGHTING(ch));
		stop_fighting(ch);
	}
}

/* give an object to a char   */
OBJ_DATA *obj_to_char(OBJ_DATA *object, CHAR_DATA *ch)
{
	OBJ_DATA *oret, *tmp_obj;

	if ( !object || !ch)
	{
		log("SYSERR: NULL obj (%p) or char (%p) passed to obj_to_char.", object, ch);
		return (NULL);
	}

	IS_CARRYING_W(ch) += get_real_obj_weight(object);
	IS_CARRYING_N(ch) += object->count;
	
	/* set flag for crash-save system, but not on mobs! */
	if (!IS_NPC(ch))
		SET_BIT(PLR_FLAGS(ch), PLR_CRASH);

	if ( !LoadingCharObj )
	{
		for ( tmp_obj = ch->first_carrying; tmp_obj; tmp_obj = tmp_obj->next_content )
		{
			if ( (oret = group_object(tmp_obj, object)) == tmp_obj )
				return (oret);
		}
	}

	LINK(object, ch->first_carrying, ch->last_carrying,
		next_content, prev_content);

	object->carried_by	= ch;
	object->in_room		= NULL;
	object->in_obj		= NULL;
	object->in_vehicle	= NULL;

	return (object);
}


/* take an object from a char */
void obj_from_char(OBJ_DATA *obj)
{
	CHAR_DATA *ch;
	
	if ( !obj )
	{
		log("SYSERR: NULL object passed to obj_from_char.");
		return;
	}

	if ( !( ch = obj->carried_by ) )
	{
		log( "SYSERR: obj_from_char: null ch.");
		return;
	}

	UNLINK(obj, ch->first_carrying, ch->last_carrying,
		next_content, prev_content);
	
	/* set flag for crash-save system, but not on mobs! */
	if (!IS_NPC(obj->carried_by))
		SET_BIT(PLR_FLAGS(obj->carried_by), PLR_CRASH);
	
	IS_CARRYING_W(ch) -= get_real_obj_weight(obj);
	IS_CARRYING_N(ch) -= obj->count;

	obj->carried_by		= NULL;
	obj->in_room		= NULL;
	obj->in_obj			= NULL;
	obj->in_vehicle		= NULL;
	obj->next_content	= NULL;
	obj->prev_content	= NULL;
}



/* Return the effect of a piece of armor in position eq_pos */
int apply_ac(CHAR_DATA *ch, int eq_pos)
{
	int factor;
	
	if (GET_EQ(ch, eq_pos) == NULL)
	{
		core_dump();
		return (0);
	}
	
	if (!(GET_OBJ_TYPE(GET_EQ(ch, eq_pos)) == ITEM_ARMOR))
		return (0);
	
	switch (eq_pos)
	{
	case WEAR_BODY:
		factor = 3;
		break;			/* 30% */
	case WEAR_HEAD:
		factor = 2;
		break;			/* 20% */
	case WEAR_LEGS:
		factor = 2;
		break;			/* 20% */
	default:
		factor = 1;
		break;			/* all others 10% */
	}
	
	return (factor * GET_OBJ_VAL(GET_EQ(ch, eq_pos), 0));
}

int invalid_align(CHAR_DATA *ch, OBJ_DATA *obj)
{
	if (IS_SET(GET_OBJ_ANTI(obj), ITEM_ANTI_EVIL) && IS_EVIL(ch))
		return (TRUE);
	if (IS_SET(GET_OBJ_ANTI(obj), ITEM_ANTI_GOOD) && IS_GOOD(ch))
		return (TRUE);
	if (IS_SET(GET_OBJ_ANTI(obj), ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch))
		return (TRUE);

	return (FALSE);
}

void equip_char(CHAR_DATA *ch, OBJ_DATA *obj, int pos)
{
	int j;
	
	if (pos < 0 || pos >= NUM_WEARS)
	{
		core_dump();
		return;
	}
	
	if (GET_EQ(ch, pos))
	{
		log("SYSERR: Char is already equipped: %s, %s", GET_NAME(ch),
			obj->short_description);
		return;
	}
	if (obj->carried_by)
	{
		log("SYSERR: EQUIP: Obj is carried_by when equip.");
		return;
	}
	if (obj->in_room != NULL)
	{
		log("SYSERR: EQUIP: Obj is in_room when equip.");
		return;
	}
	if (invalid_align(ch, obj) || invalid_class(ch, obj) || invalid_race(ch, obj))
	{
		act("You are zapped by $p and instantly let go of it.", FALSE, ch, obj, 0, TO_CHAR);
		act("$n is zapped by $p and instantly lets go of it.", FALSE, ch, obj, 0, TO_ROOM);
		/* Changed to drop in inventory instead of the ground. */
		obj = obj_to_char(obj, ch);
		return;
	}
	
	GET_EQ(ch, pos)		= obj;
	obj->worn_by		= ch;
	obj->worn_on		= pos;
	
	if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
		GET_AC(ch) -= apply_ac(ch, pos);
	
	if (ch->in_room != NULL)
	{
		if (pos == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
			if (GET_OBJ_VAL(obj, 2))	/* if light is ON */
				ch->in_room->light++;
	}
	else
		log("SYSERR: ch->in_room = NOWHERE when equipping char %s.", GET_NAME(ch));
	
	for (j = 0; j < MAX_OBJ_AFF; j++)
		affect_modify(ch, obj->affected[j].location,
			obj->affected[j].modifier, obj->obj_flags.bitvector, TRUE);

	affect_total(ch);
}



OBJ_DATA *unequip_char(CHAR_DATA *ch, int pos)
{
	int j;
	OBJ_DATA *obj;
	
	if ((pos < 0 || pos >= NUM_WEARS) || GET_EQ(ch, pos) == NULL)
	{
		core_dump();
		return (NULL);
	}
	
	obj = GET_EQ(ch, pos);
	obj->worn_by = NULL;
	obj->worn_on = -1;
	
	if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
		GET_AC(ch) += apply_ac(ch, pos);
	
	if (ch->in_room != NULL)
	{
		if (pos == WEAR_LIGHT && GET_OBJ_TYPE(obj) == ITEM_LIGHT)
			if (GET_OBJ_VAL(obj, 2))	/* if light is ON */
				ch->in_room->light--;
	}
	else
		log("SYSERR: ch->in_room = NOWHERE when unequipping char %s.", GET_NAME(ch));
	
	GET_EQ(ch, pos) = NULL;
	
	for (j = 0; j < MAX_OBJ_AFF; j++)
		affect_modify(ch, obj->affected[j].location,
			obj->affected[j].modifier,
			obj->obj_flags.bitvector, FALSE);
	
	affect_total(ch);
	
	return (obj);
}


int get_number(char **name)
{
	int i;
	char *ppos;
	char number[MAX_INPUT_LENGTH];
	
	*number = '\0';
	
	if ((ppos = strchr(*name, '.')) != NULL)
	{
		*ppos++ = '\0';
		strcpy(number, *name);
		strcpy(*name, ppos);
		
		for (i = 0; *(number + i); i++)
			if (!isdigit(*(number + i)))
				return (0);
			
		return (atoi(number));
	}
	return (1);
}



/* Search a given list for an object number, and return a ptr to that obj */
OBJ_DATA *get_obj_in_list_num(int num, OBJ_DATA *list)
{
	OBJ_DATA *i;
	
	for (i = list; i; i = i->next_content)
		if (GET_OBJ_RNUM(i) == num)
			return (i);
		
	return (NULL);
}



/* search the entire world for an object number, and return a pointer  */
OBJ_DATA *get_obj_num(obj_rnum nr)
{
	OBJ_DATA *i;
	
	for (i = first_object; i; i = i->next)
		if (GET_OBJ_RNUM(i) == nr)
			return (i);
		
	return (NULL);
}



/* search a room for a char, and return a pointer if found..  */
CHAR_DATA *get_char_room(char *name, int *number, ROOM_DATA *room)
{
	CHAR_DATA *i;
	int num;
	
	if (!number)
	{
		number = &num;
		num = get_number(&name);
	}
	
	if (*number == 0)
		return (NULL);
	
	for (i = room->people; i && *number; i = i->next_in_room)
		if (isname(name, GET_PC_NAME(i)))
			if (--(*number) == 0)
				return (i);
			
	return (NULL);
}



/* search all over the world for a char num, and return a pointer if found */
CHAR_DATA *get_char_num(mob_rnum nr)
{
	CHAR_DATA *i;
	
	for (i = character_list; i; i = i->next)
		if (GET_MOB_RNUM(i) == nr)
			return (i);
		
	return (NULL);
}



/* put an object in a room */
OBJ_DATA *obj_to_room(OBJ_DATA * object, ROOM_DATA *room)
{
	OBJ_DATA *oret, *tmp_obj;

	if (!object || !room )
	{
		log("SYSERR: NULL obj (%p) or room (%p) passed to obj_to_room.", object, room);
		return (NULL);
	}

	if (ROOM_FLAGGED(room, ROOM_HOUSE))
		SET_BIT(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);

	if ( !LoadingCharObj )
	{
		for ( tmp_obj = room->first_content; tmp_obj; tmp_obj = tmp_obj->next_content )
		{
			if ( (oret = group_object(tmp_obj, object)) == tmp_obj )
				return (oret);
		}
	}

	LINK(object, room->first_content, room->last_content, next_content, prev_content);

	object->in_room		= room;
	object->in_obj		= NULL;
	object->in_vehicle	= NULL;
	object->carried_by	= NULL;

	return (object);
}


/* Take an object from a room */
void obj_from_room(OBJ_DATA *object)
{
	if (!object || object->in_room == NULL)
	{
		log("SYSERR: NULL object (%p) or obj not in a room (%d) passed to obj_from_room",
			object, object->in_room);
		return;
	}

	UNLINK(object, object->in_room->first_content, object->in_room->last_content,
		next_content, prev_content);

	if (ROOM_FLAGGED(object->in_room, ROOM_HOUSE))
		SET_BIT(ROOM_FLAGS(object->in_room), ROOM_HOUSE_CRASH);

	object->in_obj			= NULL;
	object->in_room			= NULL;
	object->in_vehicle		= NULL;
	object->carried_by		= NULL;
	object->next_content	= NULL;
	object->prev_content	= NULL;
}


/* put an object in an object (quaint)  */
OBJ_DATA *obj_to_obj(OBJ_DATA *obj, OBJ_DATA *obj_to)
{
	OBJ_DATA *oret, *tmp_obj;
	
	if (!obj || !obj_to || obj == obj_to)
	{
		log("SYSERR: NULL object (%p) or same source (%p) and target (%p) obj passed to obj_to_obj.",
			obj, obj, obj_to);
		return (NULL);
	}
	
	for (tmp_obj = obj_to; tmp_obj->in_obj; tmp_obj = tmp_obj->in_obj)
		GET_OBJ_WEIGHT(tmp_obj) += get_real_obj_weight(obj);
	
	/* top level object.  Subtract weight from inventory if necessary. */
	GET_OBJ_WEIGHT(tmp_obj) += get_real_obj_weight(obj);
	if (tmp_obj->carried_by)
		IS_CARRYING_W(tmp_obj->carried_by) += get_real_obj_weight(obj);

	/* check for grouping items */
	if ( !LoadingCharObj )
	{
		for ( tmp_obj = obj_to->first_content; tmp_obj; tmp_obj = tmp_obj->next_content )
		{
			if ( (oret = group_object(tmp_obj, obj)) == tmp_obj )
				return (oret);
		}
	}

	/* add to the list */
	LINK( obj, obj_to->first_content, obj_to->last_content, next_content, prev_content );

	obj->in_obj			= obj_to;
	obj->in_room		= NULL;
	obj->in_vehicle		= NULL;
	obj->carried_by		= NULL;

	return (obj);
}


/* remove an object from an object */
void obj_from_obj(OBJ_DATA *obj)
{
	OBJ_DATA *temp, *obj_from;
	
	if (obj->in_obj == NULL)
	{
		log("SYSERR: (%s): trying to illegally extract obj from obj.", __FILE__);
		return;
	}

	obj_from = obj->in_obj;
	
	UNLINK( obj, obj_from->first_content, obj_from->last_content,
		next_content, prev_content );
	
	/* Subtract weight from containers container */
	for (temp = obj->in_obj; temp->in_obj; temp = temp->in_obj)
		GET_OBJ_WEIGHT(temp) -= get_real_obj_weight(obj);
	
	/* Subtract weight from char that carries the object */
	GET_OBJ_WEIGHT(temp) -= get_real_obj_weight(obj);
	if (temp->carried_by)
		IS_CARRYING_W(temp->carried_by) -= get_real_obj_weight(obj);
	
	obj->in_obj			= NULL;
	obj->in_room		= NULL;
	obj->in_vehicle		= NULL;
	obj->carried_by		= NULL;
	obj->next_content	= NULL;
	obj->prev_content	= NULL;
}


/* Set all carried_by to point to new owner */
void object_list_new_owner(OBJ_DATA *list, CHAR_DATA *ch)
{
	if (list)
	{
		object_list_new_owner(list->first_content, ch);
		object_list_new_owner(list->next_content, ch);
		list->carried_by = ch;
	}
}


/* Extract an object from the world */
void extract_obj(OBJ_DATA *obj)
{
	if (obj->worn_by)
	{
		if (unequip_char(obj->worn_by, obj->worn_on) != obj)
			log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
	}
	if		(obj->in_room)		obj_from_room(obj);
	else if (obj->carried_by)	obj_from_char(obj);
	else if (obj->in_obj)		obj_from_obj(obj);
	else if (obj->in_vehicle)	obj_from_vehicle(obj);
	
	/* Get rid of the contents of the object, as well. */
	while (obj->first_content)
		extract_obj(obj->first_content);
	
	UNLINK(obj, first_object, last_object, next, prev);
		
	if (GET_OBJ_RNUM(obj) >= 0)
		(obj_index[GET_OBJ_RNUM(obj)].number) -= obj->count;

	free_obj(obj);
}


void update_object(OBJ_DATA *obj, int use)
{
	if (GET_OBJ_TIMER(obj) > 0)
		GET_OBJ_TIMER(obj) -= use;
	if (obj->first_content)
		update_object(obj->first_content, use);
	if (obj->next_content)
		update_object(obj->next_content, use);
}


void update_char_objects(CHAR_DATA *ch)
{
	int i;
	
	if (GET_EQ(ch, WEAR_LIGHT) != NULL)
	{
		if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_LIGHT)) == ITEM_LIGHT)
		{
			if (GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2) > 0)
			{
				i = --GET_OBJ_VAL(GET_EQ(ch, WEAR_LIGHT), 2);
				if (i == 1)
				{
					send_to_char("Your light begins to flicker and fade.\r\n", ch);
					act("$n's light begins to flicker and fade.", FALSE, ch, 0, 0, TO_ROOM);
				}
				else if (i == 0)
				{
					send_to_char("Your light sputters out and dies.\r\n", ch);
					act("$n's light sputters out and dies.", FALSE, ch, 0, 0, TO_ROOM);
					ch->in_room->light--;
				}
			}
		}
	}
	
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
			update_object(GET_EQ(ch, i), 2);
	}
	
	if (ch->first_carrying)
		update_object(ch->first_carrying, 1);
}

/* New AfterDeath System */
void char_to_ghost(CHAR_DATA *ch)
{
	CHAR_DATA *k, *temp;

	if (ch->followers || ch->master)
		die_follower(ch);
	
	/* stop using a forniture */
	if (ch->in_obj)
		char_from_obj(ch);
	if (ch->in_building)
		char_from_building(ch);
	if (ch->in_ship)
		char_from_ship(ch);

	/* stop using a mount or being used as a mount */
	if (RIDING(ch) || RIDDEN_BY(ch))
		dismount_char(ch);

	if (WAGONER(ch))
		stop_be_wagoner(ch);

	if (FIGHTING(ch))
		stop_fighting(ch);
	
	for (k = combat_list; k; k = temp)
	{
		temp = k->next_fighting;
		if (FIGHTING(k) == ch)
			stop_fighting(k);
	}
	
	/* we can't forget the hunters either... */
	for (temp = character_list; temp; temp = temp->next)
	{
		if (HUNTING(temp) == ch)
			stop_hunting(temp);
	}

	if (ch->action)
	{
		event_cancel(ch->action);
		ch->action = NULL;
	}
}


/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char_final(CHAR_DATA *ch)
{
	CHAR_DATA *k, *temp;
	DESCRIPTOR_DATA *d;
	OBJ_DATA *obj;
	int i;
	
	if (ch->in_room == NULL)
	{
		log("SYSERR: NOWHERE extracting char %s. (%s, extract_char_final)",
			GET_NAME(ch), __FILE__);
		exit(1);
	}
	
	/*
	 * We're booting the character of someone who has switched so first we
	 * need to stuff them back into their own body.  This will set ch->desc
	 * we're checking below this loop to the proper value.
	 */
	if (!IS_NPC(ch) && !ch->desc)
	{
		for (d = descriptor_list; d; d = d->next)
			if (d->original == ch)
			{
				do_return(d->character, NULL, 0, 0);
				break;
			}
	}
	
	if (ch->desc)
	{
		/*
		 * This time we're extracting the body someone has switched into
		 * (not the body of someone switching as above) so we need to put
		 * the switcher back to their own body.
		 *
		 * If this body is not possessed, the owner won't have a
		 * body after the removal so dump them to the main menu.
		 */
		if (ch->desc->original)
			do_return(ch, NULL, 0, 0);
		else
		{
			/*
			 * Now we boot anybody trying to log in with the same character, to
			 * help guard against duping.  CON_DISCONNECT is used to close a
			 * descriptor without extracting the d->character associated with it,
			 * for being link-dead, so we want CON_CLOSE to clean everything up.
			 * If we're here, we know it's a player so no IS_NPC check required.
			 */
			for (d = descriptor_list; d; d = d->next)
			{
				if (d == ch->desc)
					continue;
				if (d->character && GET_IDNUM(ch) == GET_IDNUM(d->character))
					STATE(d) = CON_CLOSE;
			}
			STATE(ch->desc) = CON_MENU;
			SEND_TO_Q(MENU, ch->desc);
		}
	}
	
	/* On with the character's assets... */
	
	if (ch->followers || ch->master)
		die_follower(ch);
	
	/* transfer objects to room, if any */
	while (ch->first_carrying)
	{
		obj = ch->first_carrying;
		obj_from_char(obj);
		obj_to_room(obj, ch->in_room);
	}

	/* transfer equipment to room, if any */
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
			obj_to_room(unequip_char(ch, i), ch->in_room);
	}
	
	/* stop using a forniture */
	if (ch->in_obj)
		char_from_obj(ch);
	if (ch->in_building)
		char_from_building(ch);
	if (ch->in_ship)
		char_from_ship(ch);

	/* stop using a mount or being used as a mount */
	if ( RIDING(ch) || RIDDEN_BY(ch) )
		dismount_char(ch);

	if (WAGONER(ch))
		stop_be_wagoner(ch);

	if (FIGHTING(ch))
		stop_fighting(ch);
	
	for (k = combat_list; k; k = temp)
	{
		temp = k->next_fighting;
		if (FIGHTING(k) == ch)
			stop_fighting(k);
	}

	/* we can't forget the hunters either... */
	for (temp = character_list; temp; temp = temp->next)
	{
		if (HUNTING(temp) == ch)
			stop_hunting(temp);
	}

	if ( ch->action )
	{
		event_cancel(ch->action);
		ch->action = NULL;
	}

	if (ch->in_vehicle)
		char_from_vehicle(ch);

	char_from_room(ch);
	
	if (IS_NPC(ch))
	{
		if (ch->mob_specials.hitched_to)
			mob_from_yoke(ch, ch->mob_specials.hitched_to);

		if (GET_MOB_RNUM(ch) >= 0)		/* prototyped */
			mob_index[GET_MOB_RNUM(ch)].number--;
		clearMemory(ch);
		free_char(ch);
	}
	
	/* Don't free PC, we're a player in the menu now. */
}


/*
 * Q: Why do we do this?
 * A: Because trying to iterate over the character
 *    list with 'ch = ch->next' does bad things if
 *    the current character happens to die. The
 *    trivial workaround of 'vict = next_vict'
 *    doesn't work if the _next_ person in the list
 *    gets killed, for example, by an area spell.
 *
 * Q: Why do we leave them on the character_list?
 * A: Because code doing 'vict = vict->next' would
 *    get really confused otherwise.
 */
void extract_char(CHAR_DATA *ch)
{
	if (IS_NPC(ch))
		SET_BIT(MOB_FLAGS(ch), MOB_NOTDEADYET);
	else
		SET_BIT(PLR_FLAGS(ch), PLR_NOTDEADYET);
	
	extractions_pending++;
}


/*
 * I'm not particularly pleased with the MOB/PLR
 * hoops that have to be jumped through but it
 * hardly calls for a completely new variable.
 * Ideally it would be its own list, but that
 * would change the '->next' pointer, potentially
 * confusing some code. Ugh. -gg 3/15/2001
 *
 * NOTE: This doesn't handle recursive extractions.
 */
void extract_pending_chars(void)
{
	CHAR_DATA *vict, *next_vict, *prev_vict;
	
	if (extractions_pending < 0)
		log("SYSERR: Negative (%d) extractions pending.", extractions_pending);
	
	for (vict = character_list, prev_vict = NULL; vict && extractions_pending; vict = next_vict)
	{
		next_vict = vict->next;
		
		if (MOB_FLAGGED(vict, MOB_NOTDEADYET))
			REMOVE_BIT(MOB_FLAGS(vict), MOB_NOTDEADYET);
		else if (PLR_FLAGGED(vict, PLR_NOTDEADYET))
			REMOVE_BIT(PLR_FLAGS(vict), PLR_NOTDEADYET);
		else
		{
			/* Last non-free'd character to continue chain from. */
			prev_vict = vict;
			continue;
		}
		
		extract_char_final(vict);
		extractions_pending--;
		
		if (prev_vict)
			prev_vict->next = next_vict;
		else
			character_list = next_vict;
	}
	
	if (extractions_pending > 0)
		log("SYSERR: Couldn't find %d extractions as counted.", extractions_pending);
	
	extractions_pending = 0;
}


/* ***********************************************************************
* Here follows high-level versions of some earlier routines, ie functions*
* which incorporate the actual player-data                               *.
*********************************************************************** */

CHAR_DATA *get_player_vis(CHAR_DATA *ch, char *name, int *number, int inroom)
{
	CHAR_DATA *i;
	int num;
	
	if (!number)
	{
		number = &num;
		num = get_number(&name);
	}
	
	for (i = character_list; i; i = i->next)
	{
		if (IS_NPC(i))
			continue;
		if (inroom == FIND_CHAR_ROOM && i->in_room != ch->in_room)
			continue;
		if (!isname(name, take_name(i, ch)))
			continue;
		if (!CAN_SEE(ch, i))
			continue;
		if (--(*number) != 0)
			continue;
		return (i);
	}
	
	return (NULL);
}


CHAR_DATA *get_char_room_vis(CHAR_DATA *ch, char *name, int *number)
{
	CHAR_DATA *i;
	int num;
	
	if (!number)
	{
		number = &num;
		num = get_number(&name);
	}
	
	/* JE 7/18/94 :-) :-) */
	if (!str_cmp(name, "self") || !str_cmp(name, "me"))
		return (ch);
	
	/* 0.<name> means PC with name */
	if (*number == 0)
		return (get_player_vis(ch, name, NULL, FIND_CHAR_ROOM));
	
	for (i = ch->in_room->people; i && *number; i = i->next_in_room)
	{
		if (isname(name, take_name_keyword(i, ch)))
			if (CAN_SEE(ch, i))
				if (--(*number) == 0)
					return (i);
	}
				
	return (NULL);
}


CHAR_DATA *get_char_world_vis(CHAR_DATA *ch, char *name, int *number)
{
	CHAR_DATA *i;
	int num;
	
	if (!number)
	{
		number = &num;
		num = get_number(&name);
	}
	
	if ((i = get_char_room_vis(ch, name, number)) != NULL)
		return (i);
	
	if (*number == 0)
		return get_player_vis(ch, name, NULL, 0);
	
	for (i = character_list; i && *number; i = i->next)
	{
		if (ch->in_room == i->in_room)
			continue;
		if (!isname(name, take_name_keyword(i, ch)))
			continue;
		if (!CAN_SEE(ch, i))
			continue;
		if (--(*number) != 0)
			continue;
		return (i);
	}

	return (NULL);
}


CHAR_DATA *get_char_vis(CHAR_DATA *ch, char *name, int *number, int where)
{
	if (where == FIND_CHAR_ROOM)
		return get_char_room_vis(ch, name, number);
	else if (where == FIND_CHAR_WORLD)
		return get_char_world_vis(ch, name, number);
	else
		return (NULL);
}


CHAR_DATA *get_char_elsewhere_vis(CHAR_DATA *ch, ROOM_DATA *pRoom, char *name, int *numbr)
{
	CHAR_DATA *i;
	ROOM_DATA *rtemp;
	int num;

	if ( !pRoom )
		return (NULL);

	if (!numbr)
	{
		numbr = &num;
		num = get_number(&name);
	}
	
	/* JE 7/18/94 :-) :-) */
	if (!str_cmp(name, "self") || !str_cmp(name, "me"))
		return (ch);
	
	rtemp = ch->in_room;
	ch->in_room = pRoom;

	for (i = ch->in_room->people; i && *numbr; i = i->next_in_room)
	{
		if (isname(name, take_name_keyword(i, ch)))
			if (CAN_SEE(ch, i))
				if (--(*numbr) == 0)
				{
					ch->in_room = rtemp;
					return (i);
				}
	}

	ch->in_room = rtemp;
	return (NULL);
}


OBJ_DATA *get_obj_in_list_vis_rev(CHAR_DATA *ch, char *name, int *numbr, OBJ_DATA *list)
{
	OBJ_DATA *i;
	int num, count;
	
	if (!numbr)
	{
		numbr = &num;
		num = get_number(&name);
	}
	
	if (*numbr == 0)
		return (NULL);
	
	count = 0;
	for (i = list; i; i = i->prev_content)
	{
		if (isname(name, i->name))
			if (CAN_SEE_OBJ(ch, i))
				if ((count += i->count) >= (*numbr))
					return (i);
	}

	return (NULL);
}


/* search the entire obj list for an object, and return a pointer  */
OBJ_DATA *get_obj_vis(CHAR_DATA *ch, char *name, int *numbr)
{
	OBJ_DATA *i;
	int num, count;
	
	if (!numbr)
	{
		numbr = &num;
		num = get_number(&name);
	}
	
	if (*numbr == 0)
		return (NULL);
	
	/* scan the entire obj list */
	for (count = 0, i = last_object; i; i = i->prev)
	{
		if (isname(name, i->name))
			if (CAN_SEE_OBJ(ch, i))
				if ((count += i->count) >= (*numbr))
					return (i);
	}
	
	return (NULL);
}


/* search the entire world for an object, and return a pointer  */
OBJ_DATA *get_obj_vis_global(CHAR_DATA *ch, char *name, int *number)
{
	OBJ_DATA *i;
	int num, nnum, count;
	
	if (!number)
	{
		number = &num;
		num = get_number(&name);
	}
	
	if ((*number) == 0)
		return (NULL);
	
	nnum = (*number);
	/* scan items carried */
	if ((i = get_obj_in_list_vis_rev(ch, name, &nnum, ch->last_carrying)) != NULL)
		return (i);
	
	/* scan room */
	nnum = (*number);
	if ((i = get_obj_in_list_vis_rev(ch, name, &nnum, ch->in_room->last_content)) != NULL)
		return (i);
	
	nnum = (*number);
	/* ok.. no luck yet. scan the entire obj list   */
	for (count = 0, i = first_object; i; i = i->next)
	{
		if (isname(name, i->name))
			if (CAN_SEE_OBJ(ch, i))
				if ((count += i->count) >= nnum)
					return (i);
	}
	
	return (NULL);
}


OBJ_DATA *get_obj_in_equip_vis(CHAR_DATA *ch, char *arg, int *number, OBJ_DATA *equipment[])
{
	int j, num;
	
	if (!number)
	{
		number = &num;
		num = get_number(&arg);
	}
	
	if (*number == 0)
		return (NULL);
	
	for (j = 0; j < NUM_WEARS; j++)
		if (equipment[j] && CAN_SEE_OBJ(ch, equipment[j]) && isname(arg, equipment[j]->name))
			if (--(*number) == 0)
				return (equipment[j]);
			
	return (NULL);
}


int get_obj_pos_in_equip_vis(CHAR_DATA *ch, char *arg, int *number, OBJ_DATA *equipment[])
{
	int j, num;
	
	if (!number)
	{
		number = &num;
		num = get_number(&arg);
	}
	
	if (*number == 0)
		return (-1);
	
	for (j = 0; j < NUM_WEARS; j++)
		if (equipment[j] && CAN_SEE_OBJ(ch, equipment[j]) && isname(arg, equipment[j]->name))
			if (--(*number) == 0)
				return (j);
			
	return (-1);
}


/* Generic Find, designed to find any object/character
 *
 * Calling:
 *  *arg     is the pointer containing the string to be searched for.
 *           This string doesn't have to be a single word, the routine
 *           extracts the next word itself.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  **tar_ch Will be NULL if no character was found, otherwise points
 * **tar_obj Will be NULL if no object was found, otherwise points
 *
 * The routine used to return a pointer to the next word in *arg (just
 * like the one_argument routine), but now it returns an integer that
 * describes what it filled in.
 */
int generic_find(char *arg, bitvector_t bitvector, CHAR_DATA *ch,
		     CHAR_DATA **tar_ch, OBJ_DATA **tar_obj)
{
	int i, found, number, nnum;
	char name_val[MAX_INPUT_LENGTH];
	char *name = name_val;
	
	*tar_ch = NULL;
	*tar_obj = NULL;
	
	one_argument(arg, name);
	
	if (!*name)
		return (0);
	if (!(nnum = get_number(&name)))
		return (0);
	
	number = nnum;
	if (IS_SET(bitvector, FIND_CHAR_ROOM))	/* Find person in room */
	{
		if ((*tar_ch = get_char_room_vis(ch, name, &number)) != NULL)
			return (FIND_CHAR_ROOM);
	}
	
	//number = nnum;
	if (IS_SET(bitvector, FIND_CHAR_WORLD))
	{
		if ((*tar_ch = get_char_world_vis(ch, name, &number)) != NULL)
			return (FIND_CHAR_WORLD);
	}
	
	number = nnum;
	if (IS_SET(bitvector, FIND_OBJ_EQUIP))
	{
		for (found = FALSE, i = 0; i < NUM_WEARS && !found; i++)
			if (GET_EQ(ch, i) && isname(name, GET_EQ(ch, i)->name) && --number == 0)
			{
				*tar_obj = GET_EQ(ch, i);
				found = TRUE;
			}
			if (found)
				return (FIND_OBJ_EQUIP);
	}
	
	//number = nnum;
	if (IS_SET(bitvector, FIND_OBJ_INV))
	{
		if ((*tar_obj = get_obj_in_list_vis_rev(ch, name, &number, ch->last_carrying)) != NULL)
			return (FIND_OBJ_INV);
	}
	
	//number = nnum;
	if (IS_SET(bitvector, FIND_OBJ_ROOM))
	{
		if ((*tar_obj = get_obj_in_list_vis_rev(ch, name, &number, ch->in_room->last_content)) != NULL)
			return (FIND_OBJ_ROOM);
	}
	
	//number = nnum;
	if (IS_SET(bitvector, FIND_OBJ_WORLD))
	{
		if ((*tar_obj = get_obj_vis(ch, name, &number)))
			return (FIND_OBJ_WORLD);
	}
	
	return (0);
}


/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg)
{
	if (!strcmp(arg, "all"))
		return (FIND_ALL);
	else if (!strncmp(arg, "all.", 4))
	{
		strcpy(arg, arg + 4);
		return (FIND_ALLDOT);
	}
	else
		return (FIND_INDIV);
}


/*
 * Make a simple clone of an object
 */
OBJ_DATA *clone_object( OBJ_DATA *obj )
{
	OBJ_DATA *clone;
	int af;
	
	if ( !obj ) return (NULL);
	
	clone = create_obj();
	
	clone->item_number	= obj->item_number;
	clone->count		= 1;

	if ( obj->name )
		clone->name					= QUICKLINK( obj->name );
	if ( obj->short_description)
		clone->short_description	= QUICKLINK( obj->short_description );
	if ( obj->description )
		clone->description			= QUICKLINK( obj->description );
	if ( obj->action_description )
		clone->action_description	= QUICKLINK( obj->action_description );
	
	GET_OBJ_LEVEL(clone)			= GET_OBJ_LEVEL(obj);
	GET_OBJ_PERM(clone)				= GET_OBJ_PERM(obj);
	GET_OBJ_TYPE(clone)				= GET_OBJ_TYPE(obj);
	GET_OBJ_EXTRA(clone)			= GET_OBJ_EXTRA(obj);
	GET_OBJ_WEAR(clone)				= GET_OBJ_WEAR(obj);
	GET_OBJ_WEIGHT(clone)			= GET_OBJ_WEIGHT(obj);
	GET_OBJ_COST(clone)				= GET_OBJ_COST(obj);
	GET_OBJ_TIMER(clone)			= GET_OBJ_TIMER(obj);
	GET_OBJ_RENT(clone)				= GET_OBJ_RENT(obj);
	GET_OBJ_COND(clone)				= GET_OBJ_COND(obj);
	GET_OBJ_MAXCOND(clone)			= GET_OBJ_MAXCOND(obj);
	GET_OBJ_QUALITY(clone)			= GET_OBJ_QUALITY(obj);
	
	GET_OBJ_VAL(clone, 0)			= GET_OBJ_VAL(obj, 0);
	GET_OBJ_VAL(clone, 1)			= GET_OBJ_VAL(obj, 1);
	GET_OBJ_VAL(clone, 2)			= GET_OBJ_VAL(obj, 2);
	GET_OBJ_VAL(clone, 3)			= GET_OBJ_VAL(obj, 3);
	
	for ( af = 0; af < MAX_OBJ_AFF; af++ )
	{
		clone->affected[af].location = obj->affected[af].location;
		clone->affected[af].modifier = obj->affected[af].modifier;
	}

	if ( obj->special )
	{
		if ( OBJ_FLAGGED(obj, ITEM_IS_SPELLBOOK) )
		{
			SPELLBOOK *clone_spec;
			
			CREATE(clone_spec, SPELLBOOK, 1);
			*clone_spec	= *((SPELLBOOK *) obj->special);
			
			clone->special	= clone_spec;
		}
		else if ( OBJ_FLAGGED(obj, ITEM_HAS_SPELLS) )
		{
			OBJ_SPELLS_DATA *clone_spec;
			
			CREATE(clone_spec, OBJ_SPELLS_DATA, 1);
			*clone_spec	= *((OBJ_SPELLS_DATA *) obj->special);
			
			clone->special	= clone_spec;
		}
		else  if ( OBJ_FLAGGED(obj, ITEM_HAS_TRAPS) )
		{
			OBJ_TRAP_DATA *clone_spec;
			
			CREATE(clone_spec, OBJ_TRAP_DATA, 1);
			*clone_spec	= *((OBJ_TRAP_DATA *) obj->special);
			
			clone->special	= clone_spec;
		}
		else
			clone->special	= NULL;
	}

	if ( clone->item_number != NOTHING )
		obj_index[clone->item_number].number++;

	return ( clone );
}
 

/*
 * If possible group obj2 into obj1				-Thoric
 * This code, along with clone_object, obj->count, and special support
 * for it implemented throughout handler.c and save.c should show improved
 * performance on MUDs with players that hoard tons of potions and scrolls
 * as this will allow them to be grouped together both in memory, and in
 * the player files.
 */
OBJ_DATA *group_object( OBJ_DATA *obj1, OBJ_DATA *obj2 )
{
	if (!obj1 || !obj2)
		return (NULL);

	if (obj1 == obj2)
		return (obj1);

	/* no corpse grouping */
	if (IS_CORPSE(obj1) || IS_CORPSE(obj2))
		return (obj2);

	/* no grouping for item with specials */
	if (obj1->special || obj2->special)
		return (obj2);

	/* money items group differently.. */
	if (GET_OBJ_TYPE(obj1) == GET_OBJ_TYPE(obj2) && GET_OBJ_TYPE(obj1) == ITEM_MONEY)
	{
		GET_OBJ_VAL(obj1, 0) += GET_OBJ_VAL(obj2, 0);

		if (obj1->item_number != NOTHING)
			 /* to be decremented in extract_obj */
			obj_index[obj1->item_number].number += obj2->count;
		
		update_money(obj1);
		extract_obj(obj2);
		return (obj1);
	}

	if (	QUICKMATCH(obj1->name, obj2->name)
		&&  QUICKMATCH(obj1->short_description, obj2->short_description)
		&&  QUICKMATCH(obj1->description, obj2->description)
		&&  GET_OBJ_TYPE(obj1)		== GET_OBJ_TYPE(obj2)
		&&  GET_OBJ_EXTRA(obj1)		== GET_OBJ_EXTRA(obj2)
		&&  GET_OBJ_ANTI(obj1)		== GET_OBJ_ANTI(obj2)
		&&  GET_OBJ_WEAR(obj1)		== GET_OBJ_WEAR(obj2)
		&&  GET_OBJ_WEIGHT(obj1)	== GET_OBJ_WEIGHT(obj2)
		&&  GET_OBJ_COST(obj1)		== GET_OBJ_COST(obj2)
		&&  GET_OBJ_LEVEL(obj1)		== GET_OBJ_LEVEL(obj2)
		&&  GET_OBJ_TIMER(obj1)		== GET_OBJ_TIMER(obj2)
		&&  GET_OBJ_COND(obj1)		== GET_OBJ_COND(obj2)
		&&  GET_OBJ_MAXCOND(obj1)	== GET_OBJ_MAXCOND(obj2)
		&&  GET_OBJ_VAL(obj1, 0)	== GET_OBJ_VAL(obj2, 0)
		&&  GET_OBJ_VAL(obj1, 1)	== GET_OBJ_VAL(obj2, 1)
		&&  GET_OBJ_VAL(obj1, 2)	== GET_OBJ_VAL(obj2, 2)
		&&  GET_OBJ_VAL(obj1, 3)	== GET_OBJ_VAL(obj2, 3)
		&&  GET_OBJ_QUALITY(obj1)	== GET_OBJ_QUALITY(obj2)
		&&  !obj1->ex_description && !obj2->ex_description 
		&&  !obj1->first_content && !obj2->first_content
		&&  obj1->count + obj2->count > 0 ) /* prevent count overflow */
	{
		obj1->count += obj2->count;

		if ( obj1->item_number != NOTHING )
			 /* to be decremented in extract_obj */
			obj_index[obj1->item_number].number += obj2->count;

		// you never know.. 
		if ( GET_OBJ_TYPE(obj1) == ITEM_MONEY )
			update_money(obj1);

		extract_obj( obj2 );
		return (obj1);
	}

	return (obj2);
}

/*
 * Split off a grouped object					-Thoric
 * decreased obj's count to num, and creates a new object containing the rest
 */
void split_obj( OBJ_DATA *obj, int num )
{
	int count = obj->count;
	OBJ_DATA *rest;
	
	if ( count <= num || num == 0 )
		return;
	
	rest = clone_object(obj);

	if ( obj->item_number != NOTHING )
		--obj_index[obj->item_number].number;

	rest->count = obj->count - num;
	obj->count	= num;
	
	if ( GET_OBJ_TYPE(obj) == ITEM_MONEY )
	{
		update_money(obj);
		update_money(rest);
	}

	if ( obj->carried_by )
	{
		LINK(rest, obj->carried_by->first_carrying, obj->carried_by->last_carrying,
			next_content, prev_content);

		rest->carried_by	= obj->carried_by;
		rest->in_room		= NULL;
		rest->in_obj		= NULL;
	}
	else if ( obj->in_room )
	{
		LINK(rest, obj->in_room->first_content, obj->in_room->last_content,
			next_content, prev_content);

		rest->carried_by	= NULL;
		rest->in_room	 	= obj->in_room;
		rest->in_obj	 	= NULL;
	}
	else if ( obj->in_obj )
	{
		LINK(rest, obj->in_obj->first_content, obj->in_obj->last_content,
			next_content, prev_content);

		rest->in_obj		= obj->in_obj;
		rest->in_room		= NULL;
		rest->carried_by	= NULL;
	}
}

void separate_obj( OBJ_DATA *obj )
{
	split_obj( obj, 1 );
}

/*
 * Return real weight of an object, including weight of contents.
 */
int get_real_obj_weight( OBJ_DATA *obj )
{
	int weight;

	if ( !obj )
		return (0);
	
	if ( GET_OBJ_TYPE(obj) == ITEM_MONEY )
		weight = MAX(1, (GET_OBJ_VAL(obj, 0) * GET_OBJ_WEIGHT(obj)) / 10);
	else
		weight = obj->count * GET_OBJ_WEIGHT(obj);
	
	for ( obj = obj->first_content; obj; obj = obj->next_content )
		weight += get_real_obj_weight(obj);
	
	return (weight);
}

