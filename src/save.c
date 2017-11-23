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
 * File: save.c                                                           *
 *                                                                        *
 * Players load & save routines                                           *
 *                                                                        *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "spells.h"

extern SPELL_INFO_DATA	spell_info[TOP_SPELL_DEFINE + 1];
extern VEHICLE_DATA		*first_vehicle;
extern VEHICLE_DATA		*last_vehicle;
extern int				max_obj_save;	/* change in config.c */

/* external functions */
SPELLBOOK *new_spellbook(OBJ_DATA *obj, int type, bool rand);
BOOK_PAGE *new_book_page(void);
VEHICLE_DATA *fread_rent_vehicle( FILE *fp, ROOM_DATA *sRoom );
CHAR_DATA *fread_rent_mount( FILE *fp, ROOM_DATA *sRoom );
bitvector_t asciiflag_conv(char *flag);
long	get_ptable_by_name(const char *name);
void	fwrite_rent_vehicle(VEHICLE_DATA *vehicle, FILE *fp);
void	extract_vehicle(VEHICLE_DATA *vehicle, int mode);
int		learn_course(CHAR_DATA *ch, int cnum);

/* locals */
OBJ_DATA *fread_one_obj( FILE *fp, int *location );
int fwrite_one_obj( OBJ_DATA *obj, FILE *fp, int mode, int location );

#define LOC_INVENTORY	0
#define MAX_BAG_ROWS	5

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


bool LoadingCharObj;

/* ======================================================================= */

void fread_one_char(CHAR_DATA *ch, FILE *fp)
{
	AFFECTED_DATA af;
	char *word;
	bool fMatch;

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

		case 'A':
			KEY( "Ab_Cha",		ch->real_abils.cha,		fread_number(fp) );
			KEY( "Ab_Con",		ch->real_abils.con,		fread_number(fp) );
			KEY( "Ab_Dex",		ch->real_abils.dex,		fread_number(fp) );
			KEY( "Ab_Int",		ch->real_abils.intel,	fread_number(fp) );
			KEY( "Ab_Str",		ch->real_abils.str,		fread_number(fp) );
			KEY( "Ab_Wis",		ch->real_abils.wis,		fread_number(fp) );
			KEY( "Act",			PLR_FLAGS(ch),			asciiflag_conv(fread_word(fp)) );
			KEY( "AffFlags",	AFF_FLAGS(ch),			asciiflag_conv(fread_word(fp)) );
			if ( !strcmp( word, "Affection" ) )
			{
				af.type			= fread_number(fp);
				af.duration		= fread_number(fp);
				af.modifier		= fread_number(fp);
				af.location		= fread_number(fp);
				af.bitvector	= asciiflag_conv(fread_word(fp));
				affect_to_char(ch, &af);
				fMatch = TRUE;
				break;
			}
			KEY( "Alignment",	GET_ALIGNMENT(ch),		fread_number(fp) );
			KEY( "Armor",		GET_AC(ch),				fread_number(fp) );
			break;
			
		case 'B':
			KEY( "Badpws",		GET_BAD_PWS(ch),		fread_number(fp) );
			KEY( "Bank",		GET_BANK_GOLD(ch),		fread_number(fp) );
			KEY( "Birth",		ch->player.time.birth,	fread_number(fp) );
			break;
			
		case 'C':
			KEY( "Clan",		GET_CLAN(ch),			fread_number(fp) );
			KEY( "ClanRank",	GET_CLAN_RANK(ch),		fread_number(fp) );
			KEY( "Class",		GET_CLASS(ch),			fread_number(fp) );
			KEY( "CondDrunk",	GET_COND(ch, DRUNK),	fread_number(fp) );
			KEY( "CondHunger",	GET_COND(ch, FULL),		fread_number(fp) );
			KEY( "CondThirst",	GET_COND(ch, THIRST),	fread_number(fp) );
			if (!strcmp(word, "Course"))
			{
				learn_course(ch, fread_number(fp));
				fMatch = TRUE;
				break;
			}
			break;
			
		case 'D':
			KEY( "Damroll",		GET_DAMROLL(ch),		fread_number(fp) );
			KEY( "DeathsMob",	GET_MOB_DEATHS(ch),		fread_number(fp) );
			KEY( "DeathsPc",	GET_PLR_DEATHS(ch),		fread_number(fp) );
			KEY( "Descr",		GET_DDESC(ch),			fread_string_nospace(fp) );
			break;
			
		case 'E':
			KEY( "Exp",			GET_EXP(ch),			fread_number(fp) );
			if ( !strcmp( word, "End" ) )
				return;
			break;
			
		case 'F':
			KEY( "FreezeLev",	GET_FREEZE_LEV(ch),		fread_number(fp) );
			break;
			
		case 'H':
			KEY( "Height",		GET_HEIGHT(ch),			fread_number(fp) );
			KEY( "Home",		GET_HOME(ch),			fread_number(fp) );
			KEY( "Host",		ch->player_specials->host,	fread_string_nospace(fp) );
			KEY( "Hitroll",		GET_HITROLL(ch),		fread_number(fp) );
			break;
			
		case 'I':
			KEY( "Id",			GET_IDNUM(ch),			fread_number(fp) );
			KEY( "InvisLev",	GET_INVIS_LEV(ch),		fread_number(fp) );
			break;

		case 'K':
			KEY( "KillsMob",	GET_MOB_KILLS(ch),		fread_number(fp) );
			KEY( "KillsPc",		GET_PLR_KILLS(ch),		fread_number(fp) );
			KEY("KillsCurrPos",	GET_KILLS_CURPOS(ch),	fread_number(fp));
			if (!str_cmp(word, "Kills"))
			{
				int i = fread_number(fp);

				GET_KILLS_VNUM(ch, i)	= fread_number(fp);
				GET_KILLS_AMOUNT(ch, i)	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if (!str_cmp(word, "Knowings"))
			{
				long idnum;
				char chname[256];

				idnum = fread_number(fp);
				strcpy(chname, fread_word(fp));

				know_add_load(ch, idnum, chname);
				
				fMatch = TRUE;
				break;
			}
			break;

		case 'L':
			KEY( "LastLogon",	ch->player.time.logon,	fread_number(fp) );
			KEY( "Level",		GET_LEVEL(ch),			fread_number(fp) );
			KEY( "LevelTot",	GET_TOT_LEVEL(ch),		fread_number(fp) );
			if ( !strcmp( word, "LoadCoord" ) )
			{
				if ( !ch->player_specials->load_coord )
					CREATE( ch->player_specials->load_coord, COORD_DATA, 1 );
				
				GET_LOAD_Y(ch) = fread_number(fp);
				GET_LOAD_X(ch) = fread_number(fp);
				fMatch = TRUE;
				break;
			}
			KEY( "LoadRoom",	GET_LOADROOM(ch),		fread_number(fp) );
			if (!strcmp(word, "LoadBuilding"))
			{
				GET_LOADBUILDING(ch)	= fread_number(fp);
				GET_LOADROOM(ch)		= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if (!strcmp(word, "LoadShip"))
			{
				GET_LOADSHIP(ch)		= fread_number(fp);
				GET_LOADROOM(ch)		= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'M':
			KEY( "Multiclass",		MULTICLASS(ch),		asciiflag_conv(fread_word(fp)) );
			if ( !strcmp( word, "Memorized" ) )
			{
				int sn = fread_number(fp);

				fMatch = TRUE;

				if ( sn < 1 || sn >= MAX_SPELLS )
					break;

				MEMORIZED(ch, sn) = fread_number(fp);

				break;
			}
			break;

		case 'N':
			KEY( "Name",		GET_PC_NAME(ch),		str_dup(fread_word(fp)) );
			break;
			
		case 'P':
			if ( !strcmp( word, "Points_Hit" ) )
			{
				GET_HIT(ch)			= fread_number(fp);
				GET_MAX_HIT(ch)		= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if ( !strcmp( word, "Points_Mana" ) )
			{
				GET_MANA(ch)		= fread_number(fp);
				GET_MAX_MANA(ch)	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if ( !strcmp( word, "Points_Move" ) )
			{
				GET_MOVE(ch)		= fread_number(fp);
				GET_MAX_MOVE(ch)	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			if ( !strcmp( word, "Password" ) )
			{
				char *pw = fread_word(fp);
				strcpy( GET_PASSWD(ch), pw );
				fMatch = TRUE;
				break;
			}
			KEY( "Practices",	GET_PRACTICES(ch),		fread_number(fp) );
			KEY( "Played",		ch->player.time.played,	fread_number(fp) );
			KEY( "PoofIn",		POOFIN(ch),				fread_string_nospace(fp) );
			KEY( "PoofOut",		POOFOUT(ch),			fread_string_nospace(fp) );
			KEY( "PrfFlags",	PRF_FLAGS(ch),			asciiflag_conv(fread_word(fp)) );
			break;

		case 'R':
			KEY( "Race",		GET_RACE(ch),			fread_number(fp) );
			break;

		case 'S':
			if ( !strcmp( word, "SavingThrow" ) )
			{
				GET_SAVE(ch, 0) = fread_number(fp);
				GET_SAVE(ch, 1) = fread_number(fp);
				GET_SAVE(ch, 2) = fread_number(fp);
				GET_SAVE(ch, 3) = fread_number(fp);
				GET_SAVE(ch, 4) = fread_number(fp);
				fMatch = TRUE;
				break;
			}
			KEY( "Sex",			GET_SEX(ch),			fread_number(fp) );
			if ( !strcmp( word, "Skill" ) )
			{
				int sn = fread_number(fp);

				fMatch = TRUE;

				if ( sn < 0 || sn >= MAX_SKILLS )
					break;

				GET_SKILL(ch, sn) = fread_number(fp);

				break;
			}
			break;
			
		case 'T':
			KEY( "Title",		GET_TITLE(ch),			fread_string_nospace(fp) );
			break;
			
		case 'W':
			KEY( "Weight",		GET_WEIGHT(ch),			fread_number(fp) );
			KEY( "Wimp",		GET_WIMP_LEV(ch),		fread_number(fp) );
			break;
			
		default:
			log("SYSERR: Unknown word %s in pfile.", word);
			break;
		}
	}
}

/*
 * Load a char, TRUE if loaded, FALSE if not
 */
int load_char(char *name, CHAR_DATA *ch)
{
	FILE *fp;
	char filename[40];
	int id, i;
	
	if ( ( id = get_ptable_by_name( name ) ) < 0 )
		return ( -1 );
	
	if (!get_filename(name, filename, PLR_FILE))
		return ( -1 );

	if ( !( fp = fopen( filename, "r" ) ) )
	{
		log( "SYSERR: Couldn't open player file %s", filename );
		return ( -1 );
	}
	
	/* character initializations */
	// initializations necessary to keep some things straight
	ch->affected				= NULL;
	
	GET_LOADROOM(ch)			= NOWHERE;
	GET_LOADBUILDING(ch)		= NOWHERE;
	GET_LOADSHIP(ch)			= NOTHING;
	GET_LOADCOORD(ch)			= NULL;

	for (i = 1; i <= MAX_SKILLS; i++)
		GET_SKILL(ch, i)		= 0;

	for (i = 0; i < 5; i++)
		GET_SAVE(ch, i)			= 0;

	for (i = 1; i < 101; i++)
	{
		GET_KILLS_VNUM(ch, i)	= 0;
		GET_KILLS_AMOUNT(ch, i)	= 0;
	}

	GET_SEX(ch)					= 0;
	GET_CLASS(ch)				= 0;
	GET_RACE(ch)				= 0;
	GET_LEVEL(ch)				= 0;
	GET_TOT_LEVEL(ch)			= 0;
	GET_HOME(ch)				= 0;
	GET_HEIGHT(ch)				= 0;
	GET_WEIGHT(ch)				= 0;
	GET_ALIGNMENT(ch)			= 0;
	PLR_FLAGS(ch)				= 0;
	AFF_FLAGS(ch)				= 0;
	MULTICLASS(ch)				= 0;
	GET_INVIS_LEV(ch)			= 0;
	GET_FREEZE_LEV(ch)			= 0;
	GET_WIMP_LEV(ch)			= 0;
	GET_COND(ch, FULL)			= 0;
	GET_COND(ch, THIRST)		= 0;
	GET_COND(ch, DRUNK)			= 0;
	GET_BAD_PWS(ch)				= 0;
	PRF_FLAGS(ch)				= 0;
	GET_PRACTICES(ch)			= 0;
	GET_BANK_GOLD(ch)			= 0;
	GET_EXP(ch)					= 0;
	GET_HITROLL(ch)				= 0;
	GET_DAMROLL(ch)				= 0;
	GET_AC(ch)					= 0;
	GET_HIT(ch)					= 0;
	GET_MAX_HIT(ch)				= 0;
	GET_MANA(ch)				= 0;
	GET_MAX_MANA(ch)			= 0;
	GET_MOVE(ch)				= 0;
	GET_MAX_MOVE(ch)			= 0;
	ch->real_abils.str			= 0;
	ch->real_abils.dex			= 0;
	ch->real_abils.intel		= 0;
	ch->real_abils.wis			= 0;
	ch->real_abils.con			= 0;
	ch->real_abils.cha			= 0;
	GET_KILLS_CURPOS(ch)		= 1;
	GET_MOB_KILLS(ch)			= 0;
	GET_MOB_DEATHS(ch)			= 0;
	GET_PLR_KILLS(ch)			= 0;
	GET_PLR_DEATHS(ch)			= 0;

	GET_CLAN(ch)				= -1;
	GET_CLAN_RANK(ch)			= -1;

	/* end of character initializations */

	// Now read from file.. 
	for ( ; ; )
	{
		char letter;
		char *word;
		
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
			log( "load_char: # not found." );
			break;
		}
		
		word = fread_word( fp );

		if (!str_cmp(word, "PLAYER"))
			fread_one_char(ch, fp);
		else if (!str_cmp(word, "END"))		// Done
			break;
		else
			log("SYSERR: unknown word %s.", word);
	}

	ch->aff_abils = ch->real_abils;

	affect_total(ch);
	
	// initialization for imms
	if (IS_IMMORTAL(ch))
	{
		for (i = 1; i <= MAX_SKILLS; i++)
			GET_SKILL(ch, i)	= 100;

		GET_COND(ch, FULL)		= -1;
		GET_COND(ch, THIRST)	= -1;
		GET_COND(ch, DRUNK)		= -1;
	}
	fclose(fp);
	return (id);
}

/* ======================================================================= */


/*
 * write the vital data of a player to the player file
 */
void save_char(CHAR_DATA *ch, ROOM_DATA *load_room)
{
	FILE *fl;
	AFFECTED_DATA *aff, tmp_aff[MAX_AFFECT];
	OBJ_DATA *char_eq[NUM_WEARS];
	char outname[MAX_NAME_LENGTH+3], buf[MAX_STRING_LENGTH];
	char filename[50];
	char dflags[128], dpref[128], bits[127];
	int i;

	if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
		return;

	/*
	 * This version of save_char allows ch->desc to be null (to allow
	 * "set file" to use it).  This causes the player's last host
	 * and probably last login to null out.
	 */
	if (!PLR_FLAGGED(ch, PLR_LOADROOM))
	{
		if (load_room == NULL)
		{
			GET_LOADCOORD(ch)		= NULL;
			GET_LOADROOM(ch)		= NOWHERE;
			GET_LOADBUILDING(ch)	= NOWHERE;
			GET_LOADSHIP(ch)		= NOWHERE;
		}
		else if ( IS_WILD( load_room ) )
		{
			if ( !GET_LOADCOORD(ch) )
				CREATE( ch->player_specials->load_coord, COORD_DATA, 1 );
			GET_LOAD_Y(ch)			= load_room->coord->y;
			GET_LOAD_X(ch)			= load_room->coord->x;
			GET_LOADROOM(ch)		= NOWHERE;
			GET_LOADSHIP(ch)		= NOTHING;
		}
		else if (IS_BUILDING(load_room))
		{
			GET_LOADROOM(ch)		= load_room->number;
			GET_LOADBUILDING(ch)	= ch->in_building->vnum;
			GET_LOADSHIP(ch)		= NOTHING;
		}
		else if ( IS_SHIP( load_room ) )
		{
			GET_LOADCOORD(ch)		= NULL;
			GET_LOADROOM(ch)		= load_room->number;
			GET_LOADSHIP(ch)		= ch->in_ship->vnum;
			GET_LOADBUILDING(ch)	= NOTHING;
		}
		else
		{
			GET_LOADCOORD(ch)		= NULL;
			GET_LOADROOM(ch)		= load_room->number;
			GET_LOADBUILDING(ch)	= NOWHERE;
			GET_LOADSHIP(ch)		= NOTHING;
		}
	}

	strcpy( outname, GET_PC_NAME(ch) );
	if (!get_filename(outname, filename, PLR_FILE))
		return;

	if ( !( fl = fopen( filename, "w" ) ) )
	{
		log("SYSERR: Couldn't open player file %s for write", filename);
		return;
	}
	
	/* Unaffect everything a character can be affected by */
	for (i = 0; i < NUM_WEARS; i++)
	{
		if ( GET_EQ(ch, i) )
			char_eq[i] = unequip_char(ch, i);
		else
			char_eq[i] = NULL;
	}
	
	for ( aff = ch->affected, i = 0; i < MAX_AFFECT; i++ )
	{
		if (aff)
		{
			tmp_aff[i]				= *aff;
			tmp_aff[i].next			= NULL;
			aff = aff->next;
		}
		else
		{
			tmp_aff[i].type			= 0;
			tmp_aff[i].duration		= 0;
			tmp_aff[i].modifier		= 0;
			tmp_aff[i].location		= 0;
			tmp_aff[i].bitvector	= 0;
			tmp_aff[i].next			= NULL;
		}
	}
	
	/*
	 * remove the affections so that the raw values are stored; otherwise the
	 * effects are doubled when the char logs back in.
	 */
	while (ch->affected)
		affect_remove(ch, ch->affected);

	//ch->aff_abils = ch->real_abils;

	if (PLR_FLAGGED(ch, PLR_NOTDEADYET))
		REMOVE_BIT(PLR_FLAGS(ch), PLR_NOTDEADYET);

	ch->player.time.played += (long) (time(0) - ch->player.time.logon);
	ch->player.time.logon = time(0);

	fprintf(fl, "#PLAYER\n");
	fprintf(fl, "Id           %d\n",		GET_IDNUM(ch));
	if (GET_PC_NAME(ch))
		fprintf(fl, "Name         %s\n",	GET_PC_NAME(ch));
	if (GET_PASSWD(ch))
		fprintf(fl, "Password     %s\n",	GET_PASSWD(ch));
	if (GET_TITLE(ch))
		fprintf(fl, "Title        %s~\n",	GET_TITLE(ch));
	if (GET_DDESC(ch) && *GET_DDESC(ch))
	{
		strcpy(buf, GET_DDESC(ch));
		strip_cr(buf);
		fprintf(fl, "Descr        %s~\n",	buf);
	}

	fprintf(fl, "Sex          %d\n",		GET_SEX(ch)); 
	fprintf(fl, "Class        %d\n",		GET_CLASS(ch)); 
	fprintf(fl, "Race         %d\n",		GET_RACE(ch)); 
	if (MULTICLASS(ch))
	{
		sprintascii(dpref, MULTICLASS(ch));
		fprintf(fl, "Multiclass   %s\n",	dpref);
	}
	fprintf(fl, "Level        %d\n",		GET_LEVEL(ch));
	fprintf(fl, "LevelTot     %d\n",		GET_TOT_LEVEL(ch));

	fprintf(fl, "Clan         %d\n",		GET_CLAN(ch)); 
	fprintf(fl, "ClanRank     %d\n",		GET_CLAN_RANK(ch)); 

	fprintf(fl, "Ab_Str       %hd\n",		ch->real_abils.str);
	fprintf(fl, "Ab_Int       %hd\n",		ch->real_abils.intel);
	fprintf(fl, "Ab_Wis       %hd\n",		ch->real_abils.wis);
	fprintf(fl, "Ab_Dex       %hd\n",		ch->real_abils.dex);
	fprintf(fl, "Ab_Con       %hd\n",		ch->real_abils.con);
	fprintf(fl, "Ab_Cha       %hd\n",		ch->real_abils.cha);
	
	fprintf(fl, "Points_Hit   %hd %hd\n",	GET_HIT(ch), GET_MAX_HIT(ch));
	fprintf(fl, "Points_Mana  %hd %hd\n",	GET_MANA(ch), GET_MAX_MANA(ch));
	fprintf(fl, "Points_Move  %hd %hd\n",	GET_MOVE(ch), GET_MAX_MOVE(ch));
	fprintf(fl, "Armor        %hd\n",		GET_AC(ch));
	fprintf(fl, "Exp          %d\n",		GET_EXP(ch));
	fprintf(fl, "Hitroll      %hd\n",		GET_HITROLL(ch));
	fprintf(fl, "Damroll      %hd\n",		GET_DAMROLL(ch));

	fprintf(fl, "KillsMob     %d\n",		GET_MOB_KILLS(ch) );
	fprintf(fl, "KillsPc      %d\n",		GET_PLR_KILLS(ch) );
	fprintf(fl, "DeathsMob    %d\n",		GET_MOB_DEATHS(ch) );
	fprintf(fl, "DeathsPc     %d\n",		GET_PLR_DEATHS(ch) );

	fprintf(fl, "Home         %d\n",		GET_HOME(ch));
	fprintf(fl, "Birth        %d\n",		ch->player.time.birth);
	fprintf(fl, "Played       %d\n",		ch->player.time.played);
	fprintf(fl, "LastLogon    %d\n",		ch->player.time.logon);
	fprintf(fl, "Height       %d\n",		GET_HEIGHT(ch));
	fprintf(fl, "Weight       %d\n",		GET_WEIGHT(ch));
	fprintf(fl, "Alignment    %d\n",		GET_ALIGNMENT(ch));

	if (PLR_FLAGS(ch))
	{
		sprintascii(dpref, PLR_FLAGS(ch));
		fprintf(fl, "Act          %s\n",	dpref);
	}

	if (GET_PRACTICES(ch))
		fprintf(fl, "Practices    %d\n",	GET_PRACTICES(ch));

	if (GET_BANK_GOLD(ch))
		fprintf(fl, "Bank         %d\n",	GET_BANK_GOLD(ch));

	if ( ch->player_specials->host )
		fprintf(fl, "Host         %s\n",	ch->player_specials->host);

	if ( AFF_FLAGS(ch) )
	{
		sprintascii(bits, AFF_FLAGS(ch));
		fprintf(fl, "AffFlags     %s\n",	bits);
	}

	fprintf(fl, "SavingThrow  %d %d %d %d %d\n",
		GET_SAVE(ch, 0), GET_SAVE(ch, 1), GET_SAVE(ch, 2),
		GET_SAVE(ch, 3), GET_SAVE(ch, 4));

	if (GET_WIMP_LEV(ch))
		fprintf(fl, "Wimp         %d\n",	GET_WIMP_LEV(ch));
	if (GET_FREEZE_LEV(ch))
		fprintf(fl, "FreezeLev    %d\n",	GET_FREEZE_LEV(ch));
	if (GET_INVIS_LEV(ch))
		fprintf(fl, "InvisLev     %d\n",	GET_INVIS_LEV(ch));
	if (GET_LOADBUILDING(ch) != NOWHERE)
		fprintf(fl, "LoadBuilding %d\n",	GET_LOADBUILDING(ch));
	if (GET_LOADROOM(ch) != NOWHERE)
		fprintf(fl, "LoadRoom     %d\n",	GET_LOADROOM(ch));
	if ( GET_LOADCOORD(ch) )
		fprintf(fl, "LoadCoord    %d %d\n", GET_LOAD_Y(ch), GET_LOAD_X(ch) );
	if ( GET_LOADSHIP(ch) != NOTHING )
		fprintf(fl, "LoadShip     %d %d\n", GET_LOADSHIP(ch), GET_LOADROOM(ch));
	if ( GET_LOADBUILDING(ch) != NOTHING )
		fprintf(fl, "LoadBuilding %d %d\n", GET_LOADBUILDING(ch), GET_LOADROOM(ch));

	if (PRF_FLAGS(ch))
	{
		sprintascii(bits, PRF_FLAGS(ch));
		fprintf(fl, "PrfFlags     %s\n",	bits);
	}
	if (GET_BAD_PWS(ch))
		fprintf(fl, "Badpws       %d\n",	GET_BAD_PWS(ch));

	/* affected_type */
	for (i = 0; i < MAX_AFFECT; i++)
	{
		if (tmp_aff[i].type)
		{
			sprintascii( dflags, tmp_aff[i].bitvector );
			fprintf(fl, "Affection    %d %d %d %d %s\n",
				tmp_aff[i].type, tmp_aff[i].duration,
				tmp_aff[i].modifier, tmp_aff[i].location, dflags);
		}
	}

	if (IS_IMMORTAL(ch))
	{
		if ( POOFIN(ch) )	fprintf(fl, "PoofIn       %s~\n", POOFIN(ch));
		if ( POOFOUT(ch) )	fprintf(fl, "PoofOut      %s~\n", POOFOUT(ch));
	}

	/* Mortal only stuffs */
	if (IS_MORTAL(ch))
	{
		if (GET_COND(ch, FULL))
			fprintf(fl, "CondHunger   %d\n", GET_COND(ch, FULL));
		if (GET_COND(ch, THIRST))
			fprintf(fl, "CondThirst   %d\n", GET_COND(ch, THIRST));
		if (GET_COND(ch, DRUNK))
			fprintf(fl, "CondDrunk    %d\n", GET_COND(ch, DRUNK));

		/* Skills */
		for (i = 1; i <= MAX_SKILLS; i++)
		{
			if ( GET_SKILL(ch, i) > 0 )
				fprintf(fl, "Skill        %d %d\n", i, GET_SKILL(ch, i) );
		}

		/* introduction system */
		if ( ch->player_specials->first_known )
		{
			KNOWN_DATA *pKnow;
			
			for ( pKnow = ch->player_specials->first_known; pKnow; pKnow = pKnow->next )
				fprintf( fl, "Knowings     %d %s\n", pKnow->idnum, pKnow->name );
		}

	
		if ( ch->player_specials->courses )
		{
			KNOWN_COURSE *kCourse;

			for (kCourse = ch->player_specials->courses; kCourse; kCourse = kCourse->next)
				fprintf(fl, "Course       %d\n", kCourse->coursenum);
		}
	}

	for ( i = 1; i <= MAX_SPELLS; i++ )
	{
		if ( MEMORIZED(ch, i) )
			fprintf(fl, "Memorized    %d %d\n", i, MEMORIZED(ch, i));
	}

	fprintf(fl, "KillsCurrPos %d\n",		GET_KILLS_CURPOS(ch));
	for (i = 1; i < 101; i++)
	{
		if (GET_KILLS_VNUM(ch, i) == 0)
			continue;

		fprintf(fl, "Kills        %d %d %hd\n", i, GET_KILLS_VNUM(ch, i), GET_KILLS_AMOUNT(ch, i));
	}

	fprintf(fl, "End\n" );

	fprintf(fl, "#END\n" );

	fclose(fl);
	
	/* more char_to_store code to restore affects */
	
	/* add spell and eq affections back in now */
	for (i = 0; i < MAX_AFFECT; i++)
	{
		if (tmp_aff[i].type)
			affect_to_char(ch, &tmp_aff[i]);
	}
	
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (char_eq[i])
			equip_char(ch, char_eq[i], i);
	}
}


/* ************************************************************************* */
/*                             Related Functions                             */
/* ************************************************************************* */



/* ******************************************************************* */
/* Object Save Routine                                                 */
/* ******************************************************************* */

int DeleteCharFile(char *name)
{
	FILE *fl;
	char filename[50];
	
	if (!get_filename(name, filename, OBJS_FILE))
		return (0);
	if (!(fl = fopen(filename, "r")))
	{
		if (errno != ENOENT)		/* if it fails but NOT because of no file */
			log("SYSERR: deleting crash file %s (1): %s", filename, strerror(errno));
		return (0);
	}
	fclose(fl);
	
	/* if it fails, NOT because of no file */
	if (remove(filename) < 0 && errno != ENOENT)
		log("SYSERR: deleting crash file %s (2): %s", filename, strerror(errno));
	
	return (1);
}

/* returns TRUE if item can NOT be saved, FALSE otherwise */
bool ItemNoSave(OBJ_DATA *obj)
{
	if (!obj)
		return (FALSE);
	
	if ( GET_OBJ_TYPE(obj) == ITEM_KEY )
		return (TRUE);
	
	if ( OBJ_FLAGGED(obj, ITEM_NOSAVE) ) 
		return (TRUE);
	
	if ( GET_OBJ_RNUM(obj) <= NOTHING && !OBJ_FLAGGED(obj, ITEM_UNIQUE) )
		return (TRUE);
	
	return (FALSE);
}

/* look for items no-save in inventory and containers */
void ExtractItemNoSave(OBJ_DATA *obj)
{
	if (obj)
	{
		ExtractItemNoSave(obj->first_content);
		ExtractItemNoSave(obj->next_content);
		if (ItemNoSave(obj))
			extract_obj(obj);
	}
}

/* look for equipped items no-save */
void ExtractItemNoSaveEq(CHAR_DATA *ch)
{
	int j;
	
	for (j = 0; j < NUM_WEARS; j++)
	{
		if (GET_EQ(ch, j) == NULL)
			continue;
		
		if (ItemNoSave(GET_EQ(ch, j)))
			// item cannot be saved, will take care later ExtractItemNoSave()
			obj_to_char(unequip_char(ch, j), ch);
		else
			// look for items in equipped containers..
			ExtractItemNoSave(GET_EQ(ch, j));
	}
}

void RestoreWeight(OBJ_DATA *obj)
{
	if (obj)
	{
		RestoreWeight(obj->first_content);
		RestoreWeight(obj->next_content);
		if (obj->in_obj)
			GET_OBJ_WEIGHT(obj->in_obj) += get_real_obj_weight(obj);
	}
}

void ExtractSavedItems(OBJ_DATA * obj)
{
	if (obj)
	{
		ExtractSavedItems(obj->first_content);
		ExtractSavedItems(obj->next_content);
		extract_obj(obj);
	}
}

int save_objs(OBJ_DATA *obj, FILE *fp, int mode, int location)
{
	OBJ_DATA *tmp;
	int result;
	
	if (obj)
	{
		save_objs(obj->prev_content, fp, mode, location);
		save_objs(obj->last_content, fp, mode, MIN(0, location) - 1);

		result = fwrite_one_obj(obj, fp, mode, location);
		
		for (tmp = obj->in_obj; tmp; tmp = tmp->in_obj)
			GET_OBJ_WEIGHT(tmp) -= get_real_obj_weight(obj);
		
		if (!result)
			return (0);
	}
	return (TRUE);
}

void SaveCharObj(CHAR_DATA *ch, bool quitting)
{
	FILE *fp;
	char fname[MAX_STRING_LENGTH];
	int j;

	if ( IS_NPC(ch) )
		return;

	if (!get_filename(GET_NAME(ch), fname, OBJS_FILE))
		return;
	if (!(fp = fopen(fname, "w")))
		return;

	ExtractItemNoSaveEq(ch);
	ExtractItemNoSave(ch->first_carrying);

	for (j = 0; j < NUM_WEARS; j++)
	{
		if ( GET_EQ(ch, j) )
		{
			if (!save_objs(GET_EQ(ch,j), fp, 0, j + 1))
			{
				fclose(fp);
				return;
			}
			if (quitting)
			{
				RestoreWeight(GET_EQ(ch, j));
				ExtractSavedItems(GET_EQ(ch, j));
			}
		}
	}

	if (!save_objs(ch->last_carrying, fp, 0, 0))
	{
		fclose(fp);
		return;
	}

	if (PLR_FLAGGED(ch, PLR_CAMPED))
	{
		CHAR_DATA *mount;
		VEHICLE_DATA *pVeh, *veh_next = NULL;
		// scan the room for mounts and/or vehicles...

		for (mount = ch->in_room->people; mount; mount = mount->next_in_room)
		{
			if (!IS_NPC(mount) || GET_MOB_VNUM(mount) == NOTHING || MOB_OWNER(mount) != GET_IDNUM(ch))
				continue;

			fprintf(fp, "#MOUNT\n");
			fprintf(fp, "Mob_vnum     %d\n", GET_MOB_VNUM(mount));
			fprintf(fp, "End\n");
			extract_char(mount);
		}

		for (pVeh = ch->in_room->vehicles; pVeh; pVeh = veh_next)
		{
			veh_next = pVeh->next_in_room;

			if (pVeh->owner_id != GET_IDNUM(ch))
				continue;
			fwrite_rent_vehicle(pVeh, fp);
			extract_vehicle(pVeh, 1);
		}
	}

	fprintf(fp, "#END\n");
	fclose(fp);

	if (quitting)
		ExtractSavedItems(ch->first_carrying);
}


void CharLoseAllItems(CHAR_DATA *ch)
{
	int j;

	for (j = 0; j < NUM_WEARS; j++)
	{
		if (GET_EQ(ch, j))
		{
			RestoreWeight(GET_EQ(ch, j));
			ExtractSavedItems(GET_EQ(ch, j));
		}
	}

	ExtractSavedItems(ch->first_carrying);

	ch->first_carrying	= NULL;
	ch->last_carrying	= NULL;
}


/*
 * AutoEQ by Burkhard Knopf <burkhard.knopf@informatik.tu-clausthal.de>
 */
void AutoEquip(CHAR_DATA *ch, OBJ_DATA *obj, int location)
{
	int j;
	
	/* Lots of checks... */
	if (location > 0)		/* Was wearing it. */
	{	
		switch (j = (location - 1))
		{
		case WEAR_LIGHT:
			break;
		case WEAR_FINGER_R:
		case WEAR_FINGER_L:
			if (!CAN_WEAR(obj, ITEM_WEAR_FINGER)) /* not fitting :( */
				location = LOC_INVENTORY;
			break;
		case WEAR_NECK_1:
		case WEAR_NECK_2:
			if (!CAN_WEAR(obj, ITEM_WEAR_NECK))
				location = LOC_INVENTORY;
			break;
		case WEAR_BODY:
			if (!CAN_WEAR(obj, ITEM_WEAR_BODY))
				location = LOC_INVENTORY;
			break;
		case WEAR_HEAD:
			if (!CAN_WEAR(obj, ITEM_WEAR_HEAD))
				location = LOC_INVENTORY;
			break;
		case WEAR_LEGS:
			if (!CAN_WEAR(obj, ITEM_WEAR_LEGS))
				location = LOC_INVENTORY;
			break;
		case WEAR_FEET:
			if (!CAN_WEAR(obj, ITEM_WEAR_FEET))
				location = LOC_INVENTORY;
			break;
		case WEAR_HANDS:
			if (!CAN_WEAR(obj, ITEM_WEAR_HANDS))
				location = LOC_INVENTORY;
			break;
		case WEAR_ARMS:
			if (!CAN_WEAR(obj, ITEM_WEAR_ARMS))
				location = LOC_INVENTORY;
			break;
		case WEAR_SHIELD:
			if (!CAN_WEAR(obj, ITEM_WEAR_SHIELD))
				location = LOC_INVENTORY;
			break;
		case WEAR_ABOUT:
			if (!CAN_WEAR(obj, ITEM_WEAR_ABOUT))
				location = LOC_INVENTORY;
			break;
		case WEAR_WAIST:
			if (!CAN_WEAR(obj, ITEM_WEAR_WAIST))
				location = LOC_INVENTORY;
			break;
		case WEAR_WRIST_R:
		case WEAR_WRIST_L:
			if (!CAN_WEAR(obj, ITEM_WEAR_WRIST))
				location = LOC_INVENTORY;
			break;
		case WEAR_WIELD:
			if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
				location = LOC_INVENTORY;
			break;
		case WEAR_HOLD:
			if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
				break;
			if (IS_WARRIOR(ch) && CAN_WEAR(obj, ITEM_WEAR_WIELD) && GET_OBJ_TYPE(obj) == ITEM_WEAPON)
				break;
			location = LOC_INVENTORY;
			break;
		case WEAR_SHOULDERS:
			if ( !CAN_WEAR(obj, ITEM_WEAR_SHOULDER) )
				location = LOC_INVENTORY;
			break;
		default:
			location = LOC_INVENTORY;
		}
		
		if (location > 0)	    /* Wearable. */
		{
			if (!GET_EQ(ch,j))
			{
				/*
				 * Check the characters's alignment to prevent them from being
				 * zapped through the auto-equipping.
				 */
				if (invalid_align(ch, obj) || invalid_class(ch, obj))
					location = LOC_INVENTORY;
				else
					equip_char(ch, obj, j);
			}
			else	/* Oops, saved a player with double equipment? */
			{
				char aeq[128];
				sprintf(aeq, "SYSERR: autoeq: '%s' already equipped in position %d.", GET_NAME(ch), location);
				mudlog(aeq, BRF, LVL_IMMORT, TRUE);
				location = LOC_INVENTORY;
			}
		}
	}

	if (location <= 0)	/* Inventory */
		obj = obj_to_char(obj, ch);
}

int LoadCharObj(CHAR_DATA *ch)
{
	FILE *fp;
	VEHICLE_DATA *vehicle = NULL;
	char fname[MAX_STRING_LENGTH];
	int num_objs = 0, j;
	/* AutoEQ addition. */
	OBJ_DATA *obj, *obj2, *cont_row[MAX_BAG_ROWS], *veh_cont_row[MAX_BAG_ROWS];
	int location;
	
	for (j = 0; j < MAX_BAG_ROWS; j++)
	{
		cont_row[j]			= NULL;
		veh_cont_row[j]		= NULL;
	}
	
	if (!get_filename(GET_NAME(ch), fname, OBJS_FILE))
		return (1);
	if (!(fp = fopen(fname, "r")))
	{
		if (errno != ENOENT)	/* if it fails, NOT because of no file */
		{
			log("SYSERR: READING OBJECT FILE %s (5): %s", fname, strerror(errno));
			send_to_char(
				"\r\n********************* NOTICE *********************\r\n"
				"There was a problem loading your objects from disk.\r\n"
				"Contact a God for assistance.\r\n", ch);
		}
		sprintf(buf, "%s entering game with no equipment.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
		return (1);
	}
	
	if (feof(fp))
	{
		log("SYSERR: LoadCharObj(): %s's rent file was empty!", GET_NAME(ch));
		return (1);
	}

	LoadingCharObj = TRUE;
	
	for ( ; ; )
	{
		char letter;
		char *word;
		
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
			log( "LoadCharObj(): # not found." );
			break;
		}
		
		word = fread_word( fp );
		
		if ( !strcmp( word, "OBJECT" ) )	// Objects
		{
			if ( ( obj = fread_one_obj( fp, &location ) ) == NULL )
				continue;
			
			num_objs += obj->count;

			AutoEquip(ch, obj, location);
			
			/*
			 * What to do with a new loaded item:
			 *
			 * If there's a list with location less than 1 below this, then its
			 * container has disappeared from the file so we put the list back into
			 * the character's inventory. (Equipped items are 0 here.)
			 *
			 * If there's a list of contents with location of 1 below this, then we
			 * check if it is a container:
			 *   - Yes: Get it from the character, fill it, and give it back so we
			 *          have the correct weight.
			 *   -  No: The container is missing so we put everything back into the
			 *          character's inventory.
			 *
			 * For items with negative location, we check if there is already a list
			 * of contents with the same location.  If so, we put it there and if not,
			 * we start a new list.
			 *
			 * Since location for contents is < 0, the list indices are switched to
			 * non-negative.
			 *
			 * This looks ugly, but it works.
			 */
			if (location > 0)		/* Equipped */
			{
				for (j = MAX_BAG_ROWS - 1; j > 0; j--)
				{
					if (cont_row[j])	/* No container, back to inventory. */
					{
						for (; cont_row[j]; cont_row[j] = obj2)
						{
							obj2 = cont_row[j]->next_content;
							obj_to_char(cont_row[j], ch);
						}
						cont_row[j] = NULL;
					}
				}
				if (cont_row[0])	/* Content list existing. */
				{
					if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER ||
					    GET_OBJ_TYPE(obj) == ITEM_MISSILE_CONT)
					{
						/* Remove object, fill it, equip again. */
						obj = unequip_char(ch, location - 1);
						
						for (; cont_row[0]; cont_row[0] = obj2)
						{
							obj2 = cont_row[0]->next_content;
							obj_to_obj(cont_row[0], obj);
						}
						equip_char(ch, obj, location - 1);
					}
					else			/* Object isn't container, empty the list. */
					{
						for (; cont_row[0]; cont_row[0] = obj2)
						{
							obj2 = cont_row[0]->next_content;
							obj_to_char(cont_row[0], ch);
						}
						cont_row[0] = NULL;
					}
				}
			}
			else	/* location <= 0 */
			{
				for (j = MAX_BAG_ROWS - 1; j > -location; j--)
				{
					if (cont_row[j])	/* No container, back to inventory. */
					{
						for (; cont_row[j]; cont_row[j] = obj2)
						{
							obj2 = cont_row[j]->next_content;
							cont_row[j] = obj_to_char(cont_row[j], ch);
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
						obj_from_char(obj);
						obj->first_content	= NULL;
						obj->last_content	= NULL;
						for (; cont_row[j]; cont_row[j] = obj2)
						{
							obj2 = cont_row[j]->next_content;
							obj_to_obj(cont_row[j], obj);
						}
						obj = obj_to_char(obj, ch);	/* Add to inventory first. */
					}
					else	/* Object isn't container, empty content list. */
					{
						for (; cont_row[j]; cont_row[j] = obj2)
						{
							obj2 = cont_row[j]->next_content;
							obj_to_char(cont_row[j], ch);
						}
						cont_row[j] = NULL;
					}
				}
				if (location < 0 && location >= -MAX_BAG_ROWS)
				{
					/*
					 * Let the object be part of the content list but put it at the
					 * list's end.  Thus having the items in the same order as before
					 * the character rented.
					 */
					obj_from_char(obj);
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
		}
		else if (!strcmp(word, "MOUNT"))
		{
			fread_rent_mount(fp, ch->in_room);
		}
		else if (!strcmp(word, "VEHICLE"))
		{
			if ( !( vehicle = fread_rent_vehicle(fp, ch->in_room) ) )
			{
				log( "SYSERR: LoadCharObj() cannot load vehicle data." );
				send_to_char("There is a problem with your vehcile. Tell an Immortal.\r\n", ch);
				fclose( fp );
				LoadingCharObj = FALSE;
				return (1);
			}

			LINK(vehicle, first_vehicle, last_vehicle, next, prev);
		}
		else if (!strcmp(word, "OBJVEH"))
		{
			if ( !vehicle )
			{
				log("SYSERR: LoadCharObj() - trying to load an object without a vehicle.");
				fclose(fp);
				LoadingCharObj = FALSE;
				return (1);
			}

			if ( ( obj = fread_one_obj( fp, &location ) ) == NULL )
				continue;
			
			obj = obj_to_vehicle(obj, vehicle);

			for (j = MAX_BAG_ROWS - 1; j > -location; j--)
			{
				if (veh_cont_row[j])
				{
					for (; veh_cont_row[j]; veh_cont_row[j] = obj2)
					{
						obj2 = veh_cont_row[j]->next_content;
						veh_cont_row[j] = obj_to_vehicle( veh_cont_row[j], vehicle );
					}
					veh_cont_row[j] = NULL;
				}
			}

			if (j == -location && veh_cont_row[j])
			{
				if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER ||
					GET_OBJ_TYPE(obj) == ITEM_MISSILE_CONT)
				{
					obj_from_vehicle(obj);
					obj->first_content = NULL;
					obj->last_content = NULL;
					for (; veh_cont_row[j]; veh_cont_row[j] = obj2)
					{
						obj2 = veh_cont_row[j]->next_content;
						obj_to_obj(veh_cont_row[j], obj);
					}
					obj = obj_to_vehicle( obj, vehicle );
				}
				else
				{
					for (; veh_cont_row[j]; veh_cont_row[j] = obj2)
					{
						obj2 = veh_cont_row[j]->next_content;
						veh_cont_row[j] = obj_to_vehicle( veh_cont_row[j], vehicle );
					}
					veh_cont_row[j] = NULL;
				}
			}

			if (location < 0 && location >= -MAX_BAG_ROWS)
			{
				obj_from_vehicle(obj);
				if ((obj2 = veh_cont_row[-location - 1]) != NULL)
				{
					while (obj2->next_content)
						obj2 = obj2->next_content;
					obj2->next_content = obj;
				}
				else
					veh_cont_row[-location - 1] = obj;
			}
		}
		else if (!strcmp(word, "END"))		// Done
			break;
	}
	
	fclose(fp);

	LoadingCharObj = FALSE;

	/* Little hoarding check. -gg 3/1/98 */
	sprintf(fname, "%s (level %d) has %d object%s (max %d).",
		GET_NAME(ch), GET_LEVEL(ch), num_objs,
		num_objs != 1 ? "s" : "", max_obj_save);
	mudlog(fname, NRM, MAX(GET_INVIS_LEV(ch), LVL_GOD), TRUE);
	
	return (1);
}

/*
 * Write an object
 */
#define IS_VALID_SN(sn)		( (sn) >=0 && (sn) < MAX_SKILLS )

int fwrite_one_obj( OBJ_DATA *obj, FILE *fp, int mode, int location )
{
	EXTRA_DESCR *ed;
	sh_int aff;
	char dflags[128], dwear[128];
	obj_vnum vobj;
	
	switch (mode)
	{
	default:	fprintf( fp, "#OBJECT\n" );		break;
	case 1:		fprintf( fp, "#OBJVEH\n" );		break;
	}

	/* explicit or implicit unique item.. */
	if ( OBJ_FLAGGED(obj, ITEM_UNIQUE) || obj->item_number == NOTHING )
	{
		/* better safe than sorry */
		SET_BIT( GET_OBJ_EXTRA(obj), ITEM_UNIQUE );
		vobj = obj->item_number = NOTHING;

		fprintf( fp, "Vnum         %d\n",		obj->item_number	);
		if ( obj->name )
			fprintf( fp, "Name         %s~\n",	obj->name		);
		if ( obj->short_description )
			fprintf( fp, "ShortDescr   %s~\n",	obj->short_description  );
		if ( obj->description )
			fprintf( fp, "Description  %s~\n",	obj->description	);
		if ( obj->action_description )
			fprintf( fp, "ActionDesc   %s~\n",	obj->action_description );
	}
	/* "normal" item */
	else
	{
		vobj = obj->item_number;

		fprintf( fp, "Vnum         %d\n",		obj_index[vobj].vnum );

		if ( obj->name )
		{
			if ( QUICKMATCH( obj->name, obj_proto[vobj].name ) == 0 )
				fprintf( fp, "Name         %s~\n",	obj->name	     );
		}
		if ( obj->short_description )
		{
			if ( QUICKMATCH( obj->short_description, obj_proto[vobj].short_description ) == 0 )
				fprintf( fp, "ShortDescr   %s~\n",	obj->short_description     );
		}
		if ( obj->description )
		{
			if ( QUICKMATCH( obj->description, obj_proto[vobj].description ) == 0 )
				fprintf( fp, "Description  %s~\n",	obj->description     );
		}
		if ( obj->action_description )
		{
			if ( QUICKMATCH( obj->action_description, obj_proto[vobj].action_description ) == 0 )
				fprintf( fp, "ActionDesc   %s~\n",	obj->action_description     );
		}
	}

	fprintf( fp, "Location     %d\n",	location );

	sprintascii( dflags, GET_OBJ_EXTRA(obj) );
	fprintf( fp, "ExtraFlags   %s\n",	dflags );
	sprintascii( dwear, GET_OBJ_WEAR(obj) );
	fprintf( fp, "WearFlags    %s\n",	dwear );

	if ( OBJ_FLAGGED(obj, ITEM_UNIQUE) )
	{
		fprintf( fp, "ItemType     %d\n",	GET_OBJ_TYPE(obj)	);
		fprintf( fp, "Weight       %d\n",	GET_OBJ_WEIGHT(obj)	);
		fprintf( fp, "Level        %d\n",	GET_OBJ_LEVEL(obj)	);
		fprintf( fp, "Cost         %d\n",	GET_OBJ_COST(obj)	);
	}
	else
	{
		if ( GET_OBJ_TYPE(obj) != obj_proto[obj->item_number].obj_flags.type_flag )
			fprintf( fp, "ItemType     %d\n",	GET_OBJ_TYPE(obj)     );
		if ( GET_OBJ_WEIGHT(obj) != obj_proto[obj->item_number].obj_flags.weight )
			fprintf( fp, "Weight       %d\n",	GET_OBJ_WEIGHT(obj)	     );
		if ( GET_OBJ_LEVEL(obj) != obj_proto[obj->item_number].obj_flags.level )
			fprintf( fp, "Level        %d\n",	GET_OBJ_LEVEL(obj)	     );
		if ( obj->obj_flags.timer )
			fprintf( fp, "Timer        %d\n",	GET_OBJ_TIMER(obj)	     );
		if ( GET_OBJ_COST(obj) != obj_proto[obj->item_number].obj_flags.cost )
			fprintf( fp, "Cost         %d\n",	GET_OBJ_COST(obj)	     );
	}

	fprintf( fp, "Count        %d\n", obj->count );

	fprintf( fp, "Cond_curr    %d\n", GET_OBJ_COND(obj));
	fprintf( fp, "Cond_max     %d\n", GET_OBJ_MAXCOND(obj));

	fprintf( fp, "Quality      %d\n", GET_OBJ_QUALITY(obj) );

	fprintf( fp, "Values       %d %d %d %d\n",
		GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1),
		GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3));
	if (GET_OBJ_OWNER(obj) != NOBODY)
		fprintf( fp, "Owner        %d\n", GET_OBJ_OWNER(obj) );
	
	// save spells
	switch ( GET_OBJ_TYPE(obj) )
	{
	case ITEM_POTION:
	case ITEM_SCROLL:
		if ( IS_VALID_SN( GET_OBJ_VAL(obj, 1) ) )
			fprintf( fp, "Spell 1      '%s'\n",
				spell_info[GET_OBJ_VAL(obj, 1)].name );
		
		if ( IS_VALID_SN( GET_OBJ_VAL(obj, 2) ) )
			fprintf( fp, "Spell 2      '%s'\n",
				spell_info[GET_OBJ_VAL(obj, 2)].name );
	
		if ( IS_VALID_SN( GET_OBJ_VAL(obj, 3) ) )
			fprintf( fp, "Spell 3      '%s'\n",
				spell_info[GET_OBJ_VAL(obj, 3)].name );
		break;
		
	case ITEM_STAFF:
	case ITEM_WAND:
		if ( IS_VALID_SN( GET_OBJ_VAL(obj, 3) ) )
			fprintf( fp, "Spell 3      '%s'\n",
				spell_info[GET_OBJ_VAL(obj, 3)].name );
		
		break;
	}
	
	// Save affections
	for ( aff = 0; aff < MAX_OBJ_AFF; aff++ )
	{
		if ( obj->affected[aff].location == APPLY_NONE )
			continue;

		fprintf( fp, "Affect       %d %d\n",
			obj->affected[aff].location,
			obj->affected[aff].modifier );
	}

	if ( OBJ_FLAGGED(obj, ITEM_UNIQUE) )
	{
		// Save extra descriptions
		for ( ed = obj->ex_description; ed; ed = ed->next )
		{
			fprintf( fp, "ExtraDescr   %s~ %s~\n",
				ed->keyword, ed->description );
		}
	}
	else
	{
		/* TODO - check obj<->proto extra descr */
	}

	// Save special data attached
	if ( obj->special )
	{
		if ( OBJ_FLAGGED(obj, ITEM_HAS_SPELLS) )
		{
			OBJ_SPELLS_DATA *ospell;

			for (ospell = (OBJ_SPELLS_DATA *) obj->special; ospell; ospell = ospell->next)
			{
				if ( ospell->spellnum == 0 )
					continue;
		
				fprintf( fp, "ObjSpell     '%s' %hd %hd\n",
					skill_name(ospell->spellnum),
					ospell->level, ospell->percent );
			}
		}
		else if ( OBJ_FLAGGED(obj, ITEM_HAS_TRAPS) )
		{
			OBJ_TRAP_DATA *trap;

			for (trap = (OBJ_TRAP_DATA *) obj->special; trap; trap = trap->next)
				fprintf(fp, "ObjTrap      %d %d %d %hd\n",
					trap->action, trap->charges,
					trap->dam_type, trap->whole_room);

		}
		else if ( OBJ_FLAGGED(obj, ITEM_IS_SPELLBOOK) )
		{
			SPELLBOOK *book = (SPELLBOOK *) obj->special;
			BOOK_PAGE *page;
			
			fprintf(fp, "Book         %hd\n", book->type);
			
			for (page = book->first_page; page; page = page->next)
				fprintf(fp, "Page         %hd %hd %hd '%s'\n",
					page->status, page->flags, page->spellnum,
					(page->spellname ? page->spellname : "a blank page") );
		}
	}

	fprintf( fp, "End\n\n" );
	return (1);
}

OBJ_DATA *fread_one_obj( FILE *fp, int *location )
{
	OBJ_DATA *obj;
	char *word;
	bool fMatch;
	int fVnum = 0;
	int j = 0;

	CREATE(obj, OBJ_DATA, 1);
	clear_object(obj);
	
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
			KEY( "ActionDesc", obj->action_description, 	fread_string_nospace( fp ) );
			if ( !strcmp( word, "Affect" ) )
			{
				if (j >= MAX_OBJ_AFF)
				{
					fMatch = TRUE;
					break;
				}
				
				obj->affected[j].location	= fread_number( fp );
				obj->affected[j].modifier	= fread_number( fp );
				j++;
				fMatch				= TRUE;
				break;
			}
			break;

		case 'B':
			if ( !strcmp(word, "Book") )
			{
				int type = fread_number(fp);

				if ( obj->special )
					log("SYSERR: special already assigned to obj.");
				else if ( !OBJ_FLAGGED(obj, ITEM_IS_SPELLBOOK) )
					log("SYSERR: book special assigned to a non-book obj.");
				else
				{
					SPELLBOOK *book = new_spellbook(obj, type, FALSE);
					book->type	= type;
					obj->special	= book;
				}

				fMatch = TRUE;
				break;
			}
			break;
			
		case 'C':
			KEY( "Cost",		GET_OBJ_COST(obj),		fread_number( fp ) );
			KEY( "Count",		obj->count,				fread_number( fp ) );
			KEY( "Cond_curr",	GET_OBJ_COND(obj),		fread_number( fp ) );
			KEY( "Cond_max",	GET_OBJ_MAXCOND(obj),	fread_number( fp ) );
			break;
			
		case 'D':
			KEY( "Description",	obj->description,		fread_string_nospace( fp ) );
			break;
			
		case 'E':
			KEY( "ExtraFlags",	GET_OBJ_EXTRA(obj),		asciiflag_conv(fread_word(fp)) );
			
			if ( !strcmp( word, "ExtraDescr" ) )
			{
				EXTRA_DESCR *new_descr;
				
				CREATE(new_descr, EXTRA_DESCR, 1);
				new_descr->keyword	= fread_string_nospace(fp);
				new_descr->description	= fread_string_nospace(fp);
				/* add to the obj extradescr list */
				new_descr->next		= obj->ex_description;
				obj->ex_description	= new_descr;
				fMatch = TRUE;
				break;
			}
			
			if ( !strcmp( word, "End" ) )
			{
				if ( !fVnum )
				{
					if ( obj->name )
						sprintf ( buf, "Fread_one_obj: %s incomplete object.", obj->name );
					else
						sprintf ( buf, "Fread_one_obj: incomplete object." );
					log( buf );
					if ( obj->name )
						STRFREE( obj->name        );
					if ( obj->description )
						STRFREE( obj->description );
					if ( obj->short_description )
						STRFREE( obj->short_description );
					if ( obj->action_description )
						STRFREE( obj->action_description );
					DISPOSE( obj );
					return (NULL);
				}
				else if ( fVnum == 2 )	// unique item
				{
					// make checks for unique items
				}
				return (obj);
			}
			break;
			
		case 'I':
			KEY( "ItemType",	GET_OBJ_TYPE(obj),		fread_number( fp ) );
			break;
			
		case 'L':
			KEY( "Level",		GET_OBJ_LEVEL(obj),		fread_number( fp ) );
			KEY( "Location",	*location,				fread_number( fp ) );
			break;

		case 'O':
			if ( !strcmp( word, "ObjSpell" ) )
			{
				OBJ_SPELLS_DATA *ospell, *spells_list = NULL;
				int sn;

				fMatch = TRUE;
				
				if ( obj->special )
				{
					if ( !OBJ_FLAGGED(obj, ITEM_HAS_SPELLS) )
					{
						log("SYSERR: non-spell special already assigned to obj.");
						break;
					}
					spells_list = (OBJ_SPELLS_DATA *) obj->special;
				}

				sn	= find_skill_num( fread_word( fp ) );

				if ( sn < 0 || sn > MAX_SPELLS )
				{
					log( "Fread_one_obj: unknown skill.", 0 );
					break;
				}

				CREATE(ospell, OBJ_SPELLS_DATA, 1);

				ospell->spellnum	= sn;
				ospell->level		= fread_number(fp);
				ospell->percent		= fread_number(fp);

				ospell->next		= spells_list;
				spells_list			= ospell;

				obj->special		= spells_list;
				SET_BIT(GET_OBJ_EXTRA(obj), ITEM_HAS_SPELLS);

				break;
			}
			if ( !strcmp( word, "ObjTrap" ) )
			{
				OBJ_TRAP_DATA *trap, *traps_list = NULL;

				fMatch = TRUE;
				
				if ( obj->special )
				{
					if ( !OBJ_FLAGGED(obj, ITEM_HAS_TRAPS) )
					{
						log("SYSERR: non-trap special already assigned to obj.");
						break;
					}
					traps_list = (OBJ_TRAP_DATA *) obj->special;
				}

				CREATE(trap, OBJ_TRAP_DATA, 1);

				trap->action		= fread_number(fp);
				trap->charges		= fread_number(fp);
				trap->dam_type		= fread_number(fp);
				trap->whole_room	= fread_number(fp);

				trap->next			= traps_list;
				traps_list			= trap;

				obj->special		= traps_list;
				SET_BIT(GET_OBJ_EXTRA(obj), ITEM_HAS_TRAPS);

				break;
			}
			KEY( "Owner",		GET_OBJ_OWNER(obj),		fread_number( fp ) );
			break;

		case 'P':
			if ( !strcmp(word, "Page") )
			{
				if ( !obj->special || !OBJ_FLAGGED(obj, ITEM_IS_SPELLBOOK) )
					log("SYSERR: book page without special spellbook.");
				else
				{
					SPELLBOOK *book = (SPELLBOOK *) obj->special;
					BOOK_PAGE *page;

					CREATE(page, BOOK_PAGE, 1);

					page->status	= fread_number(fp);
					page->flags		= fread_number(fp);
					page->spellnum	= fread_number(fp);
					page->spellname = str_dup(fread_word(fp));

					LINK(page, book->first_page, book->last_page, next, prev);
					book->num_of_pages++;
					if ( page->spellnum != NOTHING )
						book->num_of_spells++;
					else
						DISPOSE(page->spellname);
				}

				fMatch = TRUE;
				break;
			}
			break;

		case 'Q':
			KEY( "Quality",		GET_OBJ_QUALITY(obj),	fread_number( fp ) );

		case 'N':
			KEY( "Name",		obj->name,				fread_string_nospace( fp ) );
			break;
			
		case 'S':
			KEY( "ShortDescr",	obj->short_description,	fread_string_nospace( fp ) );
			if ( !strcmp( word, "Spell" ) )
			{
				int iValue;
				int sn;
				
				iValue	= fread_number( fp );
				sn	= find_skill_num( fread_word(fp) );

				if ( iValue < 0 || iValue > 5 )
					log( "Fread_one__obj: bad iValue %d.", iValue );
				else if ( sn < 0 )
					log( "Fread_one_obj: unknown skill.", 0 );
				else
					GET_OBJ_VAL(obj, iValue) = sn;
				fMatch = TRUE;
				break;
			}
			break;
			
		case 'T':
			KEY( "Timer",	GET_OBJ_TIMER(obj),	fread_number(fp) );
			break;
			
		case 'V':
			if ( !strcmp( word, "Values" ) )
			{
				GET_OBJ_VAL(obj, 0) = fread_number(fp);
				GET_OBJ_VAL(obj, 1) = fread_number(fp);
				GET_OBJ_VAL(obj, 2) = fread_number(fp);
				GET_OBJ_VAL(obj, 3) = fread_number(fp);
				fMatch = TRUE;
				break;
			}
			
			if ( !strcmp( word, "Vnum" ) )
			{
				int vnum;
				
				vnum = fread_number( fp );
				
				if ( vnum == NOTHING )	// unique item
				{
					obj->item_number = NOTHING;
					LINK(obj, first_object, last_object, next, prev);
					fVnum = 2;
				}
				else if ( ( obj = read_object(vnum, VIRTUAL) ) == NULL )
					fVnum = 0;
				else
					fVnum = 1;
				fMatch = TRUE;
				break;
			}
			break;
			
		case 'W':
			KEY( "WearFlags",	GET_OBJ_WEAR(obj),		asciiflag_conv(fread_word(fp)) );
			KEY( "Weight",		GET_OBJ_WEIGHT(obj),	fread_number( fp ) );
			break;
		}
		
		if ( !fMatch )
		{
			EXTRA_DESCR *ed, *temp;
			
			log( "Fread_one_obj: no match." );
			log( word, 0 );
			fread_to_eol( fp );
			if ( obj->name )
				STRFREE( obj->name        );
			if ( obj->description )
				STRFREE( obj->description );
			if ( obj->short_description )
				STRFREE( obj->short_description );
			
			while ( (ed = obj->ex_description) != NULL )
			{
				if ( ed->keyword )
					STRFREE( ed->keyword );
				if ( ed->description )
					STRFREE( ed->description );
				REMOVE_FROM_LIST(ed, obj->ex_description, next );
				DISPOSE( ed );
			}
			DISPOSE( obj );
			return (NULL);
		}
    }
}


void SaveAll(bool bQuit)
{
	DESCRIPTOR_DATA *d;

	for (d = descriptor_list; d; d = d->next)
	{
		if ((STATE(d) == CON_PLAYING) && !IS_NPC(d->character))
		{
			if (PLR_FLAGGED(d->character, PLR_CRASH))
			{
				SaveCharObj(d->character, bQuit);
				save_char(d->character, NULL);
				REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRASH);
			}
		}
	}
}

