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
 * File: wild.life.c                                                      *
 *                                                                        *
 * Virtual Ecosystem module                                               *
 *                                                                        *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "constants.h"

/* external funcs */
WILD_DATA *get_wd(COORD_DATA *coord);

/* local funcs */
void save_life(WILD_DATA *wd);

/* globals */
LIFE_DATA	*base_life_table[SECT_HASH];		/* default encounters table				*/

/*
 * 
 * Global switch for using encounter charts
 *
 * TRUE     = use encounter charts
 * FALSE    = do not use encounter charts (no mob in wild)
 *
 */
bool		use_life		= TRUE;

/*
 * 
 * Global switch for using custom wildsector chart
 *
 * TRUE     = use custom wildsector chart
 * FALSE    = use global chart base_life_table for all wildsectors
 *
 */
bool		use_custom_life	= TRUE;

/* ================================================================= */

// add the life data to the right hash table
void new_life(LIFE_DATA *pLife, WILD_DATA *wd)
{
	int iHash = pLife->sect % SECT_HASH;

	// add to the right hash table
	if (wd)
	{
		pLife->next				= wd->life_table[iHash];
		wd->life_table[iHash]	= pLife;
	}
	else
	{
		pLife->next				= base_life_table[iHash];
		base_life_table[iHash]	= pLife;
	}
}

/* ================================================================= */

// duplicate default data into the given wildsector
void clone_base_life(WILD_DATA *wd)
{
	LIFE_DATA *pLife, *wLife;
	int iHash;

	for (iHash = 0; iHash < SECT_HASH; iHash++)
	{
		for (pLife = base_life_table[iHash]; pLife; pLife = pLife->next)
		{
			CREATE(wLife, LIFE_DATA, 1);

			// copy numbers
			*wLife			= *pLife;
			// duplicate string
			wLife->name		= str_dup(pLife->name);
			// nullify next pointer
			wLife->next		= NULL;

			new_life(wLife, wd);
		}
	}
}


/* ================================================================= */

/*
 *
 * wandering monster encounter chart -- Base Chart
 *
 * fields:
 *
 * <sector num> <mob vnum> <mob name>~ <max number> <min number> (1)<current number> (2)<regen rate>
 *
 * (1) in base charts always set to 0
 *
 * (2) regen_rate is halved by 100 in loading, so if you want to regenerate 1 mob per mud hour
 * you have to write 100, for 3 mob per hours write 300, for 1 mob every 2 hours write 50,
 * and so on..
 *
 */

// load global default life data
void load_base_life(void)
{
	FILE *fp;
	LIFE_DATA *pLife;
	char fname[128];
	char letter;
	int sect, num;
	float calc;

	use_life = TRUE;

	sprintf(fname, "%slife.data", LIB_DATA);
	if (!(fp = fopen(fname, "r")))
	{
		log("SYSERR: cannot open encounter chart file %s", fname);
		use_life = FALSE;
		return;
	}

	for ( ;; )
	{
		letter = fread_letter( fp );

		if (feof(fp))	letter = '$';

		if ( letter == '$' )
			break;

		if ( letter == '*' )
		{
			fread_to_eol( fp );
			continue;
		}

		/*
		 * supported types..
		 *
		 * M = mobile
		 *   =
		 */
		if ( strchr("M", letter) == NULL )
		{
			log("SYSERR: load_base_life() - invalid type %c.", letter);
			fread_to_eol( fp );
			continue;
		}

		sect = fread_number(fp);
		if (sect < 0 || sect > MAX_SECT)
		{
			log("SYSERR: load_base_life() - invalid sector number %d.", sect);
			fread_to_eol( fp );
			continue;
		}

		CREATE(pLife, LIFE_DATA, 1);
		pLife->next			= NULL;

		pLife->sect			= sect;

		pLife->vnum			= fread_number(fp);
		pLife->name			= fread_string_nospace(fp);
		pLife->num_max		= fread_number(fp);
		pLife->num_min		= fread_number(fp);
		fread_number(fp);	// pLife->num_curr
		num					= fread_number(fp);
		calc				= (float) num / 100;
		pLife->regen_rate	= calc;

		pLife->num_curr		= pLife->num_max;

		new_life(pLife, NULL);
	}

	fclose(fp);
}

/* ================================================================= */

// load a wildsector life file
void load_life(WILD_DATA *wd)
{
	FILE *fp;
	LIFE_DATA *pLife;
	char fname[128];
	char letter;
	int sect, num;
	float calc;

	if (!use_life || !use_custom_life)
		return;

	sprintf(fname, "%slife/%d-%d.life", WLS_PREFIX, wd->coord->y, wd->coord->x);
	if (!(fp = fopen(fname, "r")))
	{
		clone_base_life(wd);
		// save current time
		wd->last_load = time(0);
		// save it
		save_life(wd);
		return;
	}

	wd->last_load = 0;

	for ( ;; )
	{
		letter = fread_letter( fp );

		if (feof(fp))	letter = '$';

		if ( letter == '$' )
			break;

		if ( letter == '*' )
		{
			fread_to_eol( fp );
			continue;
		}

		/*
		 * supported types..
		 *
		 * M = mobile
		 * T = time (last time the wildsector has been loaded)
		 */
		if ( strchr("MT", letter) == NULL )
		{
			log("SYSERR: load_life() - invalid type %c.", letter);
			fread_to_eol( fp );
			continue;
		}

		if ( letter == 'T' )
		{
			wd->last_load	= fread_number(fp);
			continue;
		}

		sect = fread_number(fp);
		if (sect < 0 || sect > MAX_SECT)
		{
			log("SYSERR: load_base_life() - invalid sector number %d.", sect);
			fread_to_eol( fp );
			continue;
		}

		CREATE(pLife, LIFE_DATA, 1);
		pLife->next			= NULL;

		pLife->sect			= sect;

		pLife->vnum			= fread_number(fp);
		pLife->name			= fread_string_nospace(fp);
		pLife->num_max		= fread_number(fp);
		pLife->num_min		= fread_number(fp);
		pLife->num_curr		= fread_number(fp);
		num					= fread_number(fp);
		calc				= (float) num / 100;
		pLife->regen_rate	= calc;

		// add to the wildsector table
		new_life(pLife, wd);

		// update life data...... if needed, of course
		if (wd->last_load && pLife->num_curr < pLife->num_max)
		{
			time_t days = days_passed(wd->last_load);
			
			if (days)
			{
				int nnum = pLife->regen_rate * days;
				
				pLife->num_curr += nnum;
				pLife->num_curr = URANGE(pLife->num_min, pLife->num_curr, pLife->num_max);
			}
		}
	}

	// save current time
	wd->last_load			= time(0);

	fclose(fp);
}

// save wildsector life to file
void save_life(WILD_DATA *wd)
{
	FILE *fp;
	LIFE_DATA *pLife;
	char fname[128];
	int iHash, rrate;

	if (!use_life || !use_custom_life)
		return;

	sprintf(fname, "%slife/%d-%d.life", WLS_PREFIX, wd->coord->y, wd->coord->x);
	if (!(fp = fopen(fname, "w")))
	{
		log("SYSERR: save_life() - cannot create life file %s", fname);
		return;
	}
	
	fprintf(fp, "T %ld\n", wd->last_load);

	for (iHash = 0; iHash < SECT_HASH; iHash++)
	{
		for (pLife = wd->life_table[iHash]; pLife; pLife = pLife->next)
		{
			rrate = pLife->regen_rate * 100;

			fprintf(fp,
				"M %hd %d %s~ %hd %hd %hd %d\n",
				pLife->sect, pLife->vnum, pLife->name,
				pLife->num_max, pLife->num_min, pLife->num_curr,
				rrate);
		}
	}

	fprintf(fp, "$\n");
	fclose(fp);
}

/* ================================================================= */

/*  */
void encounter(ROOM_DATA *pRoom)
{
	LIFE_DATA *pLife, *pLifeTable;
	WILD_DATA *wd;
	CHAR_DATA *mob;
	int iHash, tot;

	// no life? no party..
	if (!use_life)				return;
	// no wild? no party..
	if (!IS_WILD(pRoom))		return;
	// 2 chanches out of 20
	if (number(1, 20) > 2)		return;

	// get the wildsector
	if (!(wd = get_wd(pRoom->coord)))
		return;

	iHash = pRoom->sector_type % SECT_HASH;

	if (use_custom_life)
		pLifeTable = wd->life_table[iHash];
	else
		pLifeTable = base_life_table[iHash];

	// get total number of possible beasts in sector
	for (tot = 0, pLife = pLifeTable; pLife; pLife = pLife->next)
	{
		if (pLife->sect == pRoom->sector_type)
			tot += (use_custom_life ? pLife->num_curr : pLife->num_max);
	}

	// no life.. exit
	if (!tot)
		return;

	// each beast take a chance to be loaded in room
	for (pLife = pLifeTable; pLife; pLife = pLife->next)
	{
		if (pLife->sect == pRoom->sector_type)
			if (number(0, 100) < percentage((use_custom_life ? pLife->num_curr : pLife->num_max), tot))
				break;
	}

	// no luck, exit
	if (!pLife)
		return;

	// aaarggghh.. invalid vnum here?? you, bastard!
	if (!(mob = read_mobile(pLife->vnum, VIRTUAL)))
		return;

	if (use_custom_life)
	{
		// _NEVER_ go under minimum.. extinction is not allowed.. :-))
		if (pLife->num_curr > pLife->num_min)
			pLife->num_curr--;
	}

	// set the mob as an encounter mob
	SET_BIT(MOB_FLAGS(mob), MOB_ENCOUNTER);
	// this means 100 seconds of permanence in world... (see mobact.c)
	mob->mob_specials.timer = 10;

	// place mob in room
	char_to_room(mob, pRoom);
}

/* ================================================================= */

ACMD(do_life)
{
	LIFE_DATA *pLife;
	int nsect, iHash;

	one_argument(argument, arg);

	if (!*arg || !is_number(arg))
	{
		send_to_char("Usage: life <sector number>\r\n", ch);
		return;
	}

	nsect = atoi(arg);
	if (nsect < 0 || nsect > MAX_SECT)
	{
		ch_printf(ch, "Valid sector number goes from 0 to %d.\r\n", MAX_SECT);
		return;
	}

	iHash = nsect % SECT_HASH;
	ch_printf(ch, "Encounter table for '%s' sectors.\r\n", terrain_type[nsect]->name);

	for (pLife = base_life_table[iHash]; pLife; pLife = pLife->next)
	{
		if (pLife->sect != nsect)
			continue;

		ch_printf(ch, 
			"Mob vnum: [%6d]  Name: [%-30s]  Max: [%3d]  Min: [%3d]  Regen Rate: [%6.2f]\r\n",
			pLife->vnum, pLife->name, pLife->num_max, pLife->num_min, pLife->regen_rate);
	}
}
