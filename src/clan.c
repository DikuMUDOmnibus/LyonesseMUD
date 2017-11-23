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
/**************************************************************************
 * File: clan.c                       Intended to be used with CircleMUD  *
 * Usage: This is the code for clans                                      *
 * By Mehdi Keddache (Heritsun on Eclipse of Fate eclipse.argy.com 7777)  *
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 * CircleMUD (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "clan.h"

extern const char *pc_class_types[];
extern const char *pc_race_types[];

/* extern functions */
void smash_tilde(char *str);
void create_building( CHAR_DATA *ch, char *arg, int clan );

/* local functions */
ACMD(do_negotiate);
ACMD(do_clan);
ACMD(do_cset);
ACMD(do_politics);
char	*get_diplo_name(sh_int value);
bool	check_membership(CHAR_DATA *vict, CLAN_DATA *pClanan);
int		num_of_vis_clans(void);
void	save_clan_table(void);
void	send_clan_format(CHAR_DATA *ch);
void	do_clan_create(CHAR_DATA *ch, char *arg);
void	do_clan_disband(CHAR_DATA *ch);
void	do_clan_revive(CHAR_DATA *ch);
void	do_clan_enroll(CHAR_DATA *ch, char *arg);
void	do_clan_expel(CHAR_DATA *ch, char *arg);
void	do_clan_demote(CHAR_DATA *ch, char *arg);
void	do_clan_promote(CHAR_DATA *ch, char *arg);
void	do_clan_invite(CHAR_DATA *ch, char *arg);
void	do_clan_bank(CHAR_DATA *ch, char *arg, int action);
void	do_clan_who(CHAR_DATA *ch);
void	do_clan_status(CHAR_DATA *ch);
void	do_clan_info(CHAR_DATA *ch, char *arg);
void	do_clan_money(CHAR_DATA *ch, char *arg, int action);
void	do_clan_application(CHAR_DATA *ch, char *arg);
void	do_clan_diplomat(CHAR_DATA *ch, char *arg);
void	do_clan_magistrate(CHAR_DATA *ch, char *arg);
void	do_clan_hero(CHAR_DATA *ch, char *arg);
void	do_clan_apply(CHAR_DATA *ch, char *arg);
void	do_clan_set(CHAR_DATA *ch, char *arg);

/* global vars & structs */
int num_of_clans;
CLAN_DATA *clans_table;
CLAN_POLITIC_DATA politics_data;

/* tables */
const char *clan_status_table[] =
{
  "Forming",
  "Active",
  "Inactive",
  "Disbanding",
  "Dead",
  "\n"
};

const char *clan_types_table[] =
{
  "Normal",
  "Private",
  "Secret",
  "\n"
};

const char *clan_anti_table[] =
{
  "No Good",
  "No Neutral"
  "No Evil",
  "No Pkiller",
  "No Pthieves",
  "Only Pkiller",
  "Only Pthieves",
  "No Male",
  "No Female",
  "\n"
};


const struct clan_titles clan_rank_table[] =
{
  { {"<Invited>",	"<Invited>",	"<Invited>"} },			/* Invited */
  { {"<Applier>",	"<Applier>",	"<Applier>"} },			/* Applied */
  { {"<Recruit>",	"<Recruit>",	"<Recruit>"} },
  { {"<Adept>",		"<Adept>",		"<Adept>"} },
  { {"<Officer>",	"<Officer>",	"<Officer>"} },
  { {"<Lieutenant>","<Lieutenant>",	"<Lieutenant>"} },
  { {"<Vassal>",	"<Vassal>",		"<Vassal>"} },			/*  6 */ /* MAX ADVANCE */
  { {"<Ambassador>","<Ambassador>",	"<Ambassadress>"} },	/* SPECIAL */
  { {"<Magistrate>","<Magistrate>",	"<Magistrate>"} },		/* SPECIAL */
  { {"<Lord>",		"<Lord>",		"<Lady>"} },			/* SPECIAL */
  { {"<Leader>",	"<Leader>",		"<Leader>"} },			/* 10 */
  { {"<Patron>",	"<Patron>",		"<Patron>"} },			/* God creator */
  { { NULL, NULL, NULL } }
};



/* === Clan Utilities Functions =========================== */

CLAN_DATA *get_clan(int idnum)
{
	CLAN_DATA *pClan;

	if (idnum == -1)
		return (NULL);

	for (pClan = clans_table; pClan; pClan = pClan->next)
	{
		if (pClan->id == idnum)
			break;
	}

	return (pClan);
}

CLAN_DATA *find_clan(char *name)
{
	CLAN_DATA *pClan;

	for (pClan = clans_table; pClan; pClan = pClan->next)
	{
		if (!str_cmp(pClan->name, name))
			break;
	}

	return (pClan);
}


/*
 * return the number of visible (non-secret) clans
 */
int num_of_vis_clans(void)
{
	CLAN_DATA *pClan;
	int num = 0;

	for (pClan = clans_table; pClan; pClan = pClan->next)
	{
		if (!IS_SET(pClan->ctype, CLAN_SECRET))
			num++;
	}

	return (num);
}

/* === Clan Files handling Functions =========================== */

void fwrite_clan(CLAN_DATA *pClan)
{
	FILE *fp;
	char fname[128];

	sprintf(fname, "%s%d.clan", LIB_CLANS, pClan->id );
	if ( !(fp = fopen(fname, "w") ) )
	{
		log( "SYSERR: unable to create clan file %s.", fname );
		return;
	}

	fprintf( fp, "#CLAN\n" );
	fprintf( fp, "ClanID       %hd\n",	pClan->id );
	fprintf( fp, "Name         %s~\n",	pClan->name );
	fprintf( fp, "Abbr         %s\n",	pClan->abbr );
	fprintf( fp, "Leader       %s\n",	pClan->leader );
	fprintf( fp, "Godname      %s\n",	pClan->godname );
	if ( pClan->motto )
		fprintf( fp, "Motto        %s~\n", pClan->motto );
	if ( pClan->warcry )
		fprintf( fp, "Warcry       %s~\n", pClan->warcry );
	fprintf( fp, "Members      %hd\n",	pClan->members );
	fprintf( fp, "Magistr      %hd\n",	pClan->magistrates );
	fprintf( fp, "Ambass       %hd\n",	pClan->ambassadors );
	fprintf( fp, "Heroes       %hd\n",	pClan->heroes );
	fprintf( fp, "AppLevel     %hd\n",	pClan->app_level );
	fprintf( fp, "MaxLevel     %hd\n",	pClan->max_level );
	fprintf( fp, "Status       %hd\n",	pClan->status );
	fprintf( fp, "ClanType     %hd\n",	pClan->ctype );
	fprintf( fp, "AntiClass    %d\n",	pClan->anti_class );
	fprintf( fp, "AntiOther    %d\n",	pClan->anti_other );
	fprintf( fp, "Power        %d\n",	pClan->power );
	fprintf( fp, "Influence    %d\n",	pClan->influence );
	fprintf( fp, "Treasure     %d\n",	pClan->treasure );
	fprintf( fp, "Appfee       %d\n",	pClan->app_fee );
	fprintf( fp, "Dues         %d\n",	pClan->dues );
	fprintf( fp, "Hall         %d\n",	pClan->hall );
	fprintf( fp, "Birth        %d\n",	pClan->birth );
	fprintf( fp, "Obj          %d %d %d\n",
		pClan->objs[0], pClan->objs[1], pClan->objs[2] );
	fprintf( fp, "End\n" );

	fclose(fp);
}

/* Save clan structure to disk */
void save_clans()
{
	CLAN_DATA *pClan;

	log( "Saving clans" );

	for (pClan = clans_table; pClan; pClan = pClan->next)
		fwrite_clan(pClan);
}

/*
 * Save clan politics structure to disk
 */
void save_clan_table(void)
{
	FILE *fl;
	
	if (!(fl = fopen(CLAN_DIP_FILE, "wb")))
	{
		log("SYSERR: Unable to open clandata file");
		return;
	}

	fwrite(&politics_data, sizeof(CLAN_POLITIC_DATA), 1, fl);
	fclose(fl);
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

CLAN_DATA *fread_clan(FILE *fp)
{
	CLAN_DATA *pClan;
	char *word;
	bool fMatch;

	CREATE(pClan, CLAN_DATA, 1);
	pClan->name		= NULL;
	pClan->abbr		= NULL;
	pClan->leader	= NULL;
	pClan->godname	= NULL;
	pClan->motto	= NULL;
	pClan->warcry	= NULL;

	for ( ; ; )
	{
		word   = feof( fp ) ? "End" : fread_word( fp );
		fMatch = FALSE;
		
		switch (UPPER(word[0]))
		{
		case '*':
		case '#':
			fMatch = TRUE;
			fread_to_eol( fp );
			break;
		case 'A':
			KEY("Abbr",			pClan->abbr,			str_dup(fread_word(fp)));
			KEY("Ambass",		pClan->ambassadors,		fread_number(fp));
			KEY("AntiClass",	pClan->anti_class,		fread_number(fp));
			KEY("AntiOther",	pClan->anti_other,		fread_number(fp));
			KEY("AppLevel",		pClan->app_level,		fread_number(fp));
			KEY("Appfee",		pClan->app_fee,			fread_number(fp));
			break;

		case 'B':
			KEY("Birth",		pClan->birth,			fread_number(fp));
			break;

		case 'C':
			KEY("ClanID",		pClan->id,				fread_number(fp));
			KEY("ClanType",		pClan->ctype,			fread_number(fp));
			break;

		case 'D':
			KEY("Dues",			pClan->dues,			fread_number(fp));
			break;

		case 'E':
			if (!strcmp(word, "End"))
				return (pClan);
			break;

		case 'G':
			KEY("Godname",		pClan->godname,			str_dup(fread_word(fp)));
			break;

		case 'H':
			KEY("Hall",			pClan->hall,			fread_number(fp));
			KEY("Heroes",		pClan->heroes,			fread_number(fp));
			break;

		case 'I':
			KEY("Influence",	pClan->influence,		fread_number(fp));
			break;

		case 'L':
			KEY("Leader",		pClan->leader,			str_dup(fread_word(fp)));
			break;

		case 'M':
			KEY("Magistr",		pClan->magistrates,		fread_number(fp));
			KEY("MaxLevel",		pClan->max_level,		fread_number(fp));
			KEY("Members",		pClan->members,			fread_number(fp));
			KEY("Motto",		pClan->motto,			fread_string_nospace(fp));
			break;

		case 'N':
			KEY("Name",			pClan->name,			fread_string_nospace(fp));
			break;

		case 'O':
			if (!strcmp(word, "Obj"))
			{
				pClan->objs[0] = fread_number(fp);
				pClan->objs[1] = fread_number(fp);
				pClan->objs[2] = fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'P':
			KEY("Power",		pClan->power,			fread_number(fp));
			break;

		case 'S':
			KEY("Status",		pClan->status,			fread_number(fp));
			break;

		case 'T':
			KEY("Treasure",		pClan->treasure,		fread_number(fp));
			break;

		case 'W':
			KEY("Warcry",		pClan->warcry,			fread_string_nospace(fp));
			break;
		}
	
		if (!fMatch)
			log("fread_clan: clan file has no match: %s", word);
	}
}

/*
 * Boot up initialization of clan & clan politics structure
 */
void init_clans(void)
{
	FILE *fp;
	CLAN_DATA *pClan;
	char fname[128];
	int i = 0;

	num_of_clans	= 0;
	clans_table		= NULL;

	for (i = 1; i < MAX_CLANS; i++)
	{
		sprintf(fname, "%s%d.clan", LIB_CLANS, i);
		if ((fp = fopen(fname, "r")))
		{
			pClan = fread_clan(fp);
			fclose(fp);

			if (!pClan)
			{
				log("SYSERR: init_clans() - error reading clan %d.", i);
				continue;
			}

			num_of_clans++;

			// add to the list
			pClan->next	= clans_table;
			clans_table	= pClan;
		}
	}
	log("  %d clans loaded.", num_of_clans);

	if (!(fp = fopen(CLAN_DIP_FILE, "rb")))
	{
		log("   Clandata file does not exist. Will create a new one");
		save_clan_table();
		return;
	}
	fread(&politics_data, sizeof(CLAN_POLITIC_DATA), 1, fp);
	fclose(fp);

	log( "  politic data for clans loaded." );
	log( "Clans -- DONE");
}

/*
 * Check clan requisite for new members allowance
 */
bool check_membership(CHAR_DATA *vict, CLAN_DATA *pClan )
{
	/* ANTI_CLASS check */
	if (pClan->anti_class)
	{
		if ((IS_SET(pClan->anti_class, CLAN_ANTI_MAGE)     && GET_CLASS(vict) == CLASS_MAGIC_USER) ||
		    (IS_SET(pClan->anti_class, CLAN_ANTI_CLERIC)   && GET_CLASS(vict) == CLASS_CLERIC)     ||
		    (IS_SET(pClan->anti_class, CLAN_ANTI_THIEF)    && GET_CLASS(vict) == CLASS_THIEF)      ||
		    (IS_SET(pClan->anti_class, CLAN_ANTI_SORCERER) && GET_CLASS(vict) == CLASS_SORCERER)   ||
		    (IS_SET(pClan->anti_class, CLAN_ANTI_WARRIOR)  && GET_CLASS(vict) == CLASS_WARRIOR))
			return (FALSE);
	}
	
	/* ANTI_RACE check */
	if (pClan->anti_race)
	{
		if ((IS_SET(pClan->anti_race, CLAN_ANTI_HUMAN) && GET_RACE(vict) == RACE_HUMAN)	||
		    (IS_SET(pClan->anti_race, CLAN_ANTI_ELF)   && GET_RACE(vict) == RACE_ELF)	||
		    (IS_SET(pClan->anti_race, CLAN_ANTI_DWARF) && GET_RACE(vict) == RACE_DWARF))
			return (FALSE);
	}


	/* ANTI_OTHER check */
	if (pClan->anti_other)
	{
		if ((IS_SET(pClan->anti_other, CLAN_ANTI_GOOD)    && IS_GOOD(vict))				||
		    (IS_SET(pClan->anti_other, CLAN_ANTI_NEUTRAL) && IS_NEUTRAL(vict))			||
		    (IS_SET(pClan->anti_other, CLAN_ANTI_EVIL)    && IS_EVIL(vict))				||
		    (IS_SET(pClan->anti_other, CLAN_ANTI_MALE)    && GET_SEX(vict) == SEX_MALE) ||
		    (IS_SET(pClan->anti_other, CLAN_ANTI_FEMALE)  && GET_SEX(vict) == SEX_FEMALE))
			return (FALSE);
	}
		
	/* aggiungere check x PK, PT */

	return (TRUE);
}

/* === Clan User Functions =========================================== */

void send_clan_format(CHAR_DATA *ch)
{
	send_to_char("Clan commands available to you:\n\r"
			"   clan info <clan>\r\n",ch);

	if (GET_LEVEL(ch) >= LVL_CLAN_GOD)
	{
		send_to_char(
			"   clan create      <leader> <clan type> <clan abbrev> 'clan name'\r\n"
			"   clan disband\r\n"
			"   clan enroll      <player>\r\n"
			"   clan expel       <player>\r\n"
			"   clan promote     <player>\r\n"
			"   clan demote      <player>\r\n"
			"   clan invite      <player>\r\n"
			"   clan diplomat    <player>\r\n"
			"   clan magistrate  <player>\r\n"
			"   clan hero        <player>\r\n"
			"   clan withdraw    <amount>\r\n"
			"   clan deposit     <amount>\r\n"
			"   clan set appfee  <amount>\r\n"
			"   clan set dues    <amount>\r\n"
			"   clan set applev  <level>\r\n", ch);
	}
	else
	{
		if (GET_CLAN(ch) == -1)
			send_to_char("   clan apply      <clan>\r\n",ch);
		else if (GET_CLAN_RANK(ch) <= RANK_APPLIER)
			return;
		else
		{
			int r = GET_CLAN_RANK(ch);

			send_to_char("   clan deposit    <amount>\r\n",ch);
			send_to_char("   clan who\r\n",ch);
			send_to_char("   clan status\r\n",ch);

			if (r <= MAX_RANK_ADVANCE)
				send_to_char("   clan resign\r\n", ch);

			if (r >= RANK_PRIVILEGES)
			{
				send_to_char("   clan withdraw   <amount>\r\n", ch);
				send_to_char("   clan enroll     <player>\r\n", ch);
				send_to_char("   clan expel      <player>\r\n", ch);
				send_to_char("   clan promote    <player>\r\n", ch);
				send_to_char("   clan demote     <player>\r\n", ch);
				send_to_char("   clan invite     <player>\r\n", ch);
				send_to_char("   clan set applev <level>\r\n",  ch);
				send_to_char("   clan set appfee <amount>\r\n", ch);
				send_to_char("   clan set dues   <amount>\r\n", ch);
  				send_to_char("   clan diplomat   <player>\r\n", ch);
  				send_to_char("   clan magistrate <player>\r\n", ch);
  				send_to_char("   clan hero       <player>\r\n", ch);
			}
		}
	}
}

/* create a new clan */
void do_clan_create(CHAR_DATA *ch, char *arg)
{
	CLAN_DATA *pClan, *pCl;
	CHAR_DATA *leader = NULL;
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
	char *clanname;
	int i;

	if (!*arg)
	{
		send_to_char("clan create <leader> <clan type> <clan abbrev> 'clan name'\r\n", ch);
		return;
	}

	if (GET_LEVEL(ch) < LVL_CLAN_GOD)
	{
		send_to_char("You are not mighty enough to create new clans!\r\n", ch);
		return;
	}

	if (GET_CLAN(ch) != -1 && GET_LEVEL(ch) < LVL_GRGOD)
	{
		send_to_char("You already are a clan patron.\r\n", ch);
		return;
	}

	if (num_of_clans >= MAX_CLANS - 1)
	{
		send_to_char("Max clans reached. WOW!\r\n",ch);
		return;
	}

	arg = two_arguments(arg, arg1, arg2);
	arg = one_argument(arg, arg3);

	clanname = get_spell_name(arg);
	if (clanname == NULL)
	{
		send_to_char("You need to specify a clan name.\r\n", ch);
		return;
	}

	if (!(*arg2))
	{
		send_to_char("You need to specify the type of the clan, <normal/private/secret>.\r\n", ch);
		return;
	}

	if (!(*arg3))
	{
		send_to_char("You need to specify a clan abbreviation of 5 characters.\r\n", ch);
		return;
	}

	if (!(leader = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM)))
	{
		send_to_char("The leader of the new clan must be present.\r\n",ch);
		return;
	}

	if (strlen(arg3) > 5)
	{
		send_to_char("Clan abbreviation too long! (5 characters max)\r\n", ch);
		return;
	}

	if (strlen(clanname) > CLAN_NAME_L)
	{
		ch_printf(ch, "Clan name too long! (%d characters max)\r\n", CLAN_NAME_L);
		return;
	}

	if (GET_LEVEL(leader) >= LVL_IMMORT)
	{
		send_to_char("You cannot set an immortal as the leader of a clan.\r\n",ch);
		return;
	}

	if (GET_LEVEL(leader) < DEFAULT_APP_LVL)
	{
		ch_printf(ch, "%s is not mighty enough to be a clan leader.\r\n", GET_NAME(leader));
		return;
	}

	if (GET_CLAN(leader) != -1)
	{
		send_to_char("The leader already belongs to a clan!\r\n",ch);
		return;
	}

	if ( find_clan(clanname) )
	{
		send_to_char("That clan name alread exists!\r\n",ch);
		return;
	}

	CREATE(pClan, CLAN_DATA, 1);

	pClan->id			= num_of_clans;

	pClan->abbr			= str_dup( CAP((char *)arg3) );
	pClan->name			= str_dup( CAP((char *)clanname) );
	pClan->leader		= str_dup( GET_NAME(leader) );
	pClan->godname		= str_dup( GET_NAME(ch) );
	pClan->motto		= str_dup( "Long life to the clan!" );
	pClan->warcry		= str_dup( "For the glory of the clan!" );

	pClan->birth		= time(0);
	pClan->members		= 1;
	pClan->ambassadors	= 0;
	pClan->heroes		= 0;
	pClan->magistrates	= 0;
	pClan->power		= GET_LEVEL(leader) * 2;
	pClan->influence	= GET_LEVEL(leader);
	pClan->treasure		= 0;
	pClan->hall			= NOWHERE;
	pClan->app_fee		= 0;
	pClan->dues			= 0;
	pClan->app_level	= DEFAULT_APP_LVL;
	pClan->max_level	= LVL_IMMORT - 1;
	pClan->anti_class	= 0;
	pClan->anti_other	= 0;
	pClan->anti_race	= 0;

	for (i = 0; i < MAX_CLAN_OBJ; i++)
		pClan->objs[i] = NOTHING;

	SET_BIT(pClan->status, CLAN_FORMING);

	if (is_abbrev(arg2, "private"))
		SET_BIT(pClan->ctype, CLAN_PRIVATE);
	else if (is_abbrev(arg2, "secret"))
		SET_BIT(pClan->ctype, CLAN_SECRET);
	else
		SET_BIT(pClan->ctype, CLAN_NORMAL);

	/* set up politics */
	for (pCl = clans_table; pCl; pCl = pCl->next)
	{
		politics_data.diplomacy[pClan->id][pCl->id] = 100;
		politics_data.diplomacy[pCl->id][pClan->id] = 100;
	}

	/* save the new clan */
	fwrite_clan(pClan);

	/* setup the clan god */
	if (GET_LEVEL(ch) < LVL_GRGOD)
	{
		GET_CLAN(ch)		= pClan->id;
		GET_CLAN_RANK(ch)	= RANK_PATRON;
	}

	send_to_char("Clan created.\r\n", ch);
	GET_CLAN(leader)		= pClan->id;
	GET_CLAN_RANK(leader)	= RANK_LEADER;
	save_char(leader, NULL);

	ch_printf(leader, "You are now the leader of the newly created '%s' clan.\r\n", pClan->name);

	/* add to the list */
	pClan->next	= clans_table;
	clans_table	= pClan;

	num_of_clans++;

	log("CLAN: new clan, '%s', created by %s.", pClan->name, GET_NAME(ch));
}


/*
 * functions to disband and revive a clan (Only PATRON)
 *
 */
void do_clan_disband(CHAR_DATA *ch)
{
	CLAN_DATA *pClan;

	if (!(pClan = get_clan(GET_CLAN(ch))))
	{
		send_to_char("Problems with your clan... \r\n", ch);
		return;
	}

	if (IS_SET(pClan->status, CLAN_DISBANDING))
	{
		send_to_char("Your clan is already in the disbandig status.\r\n", ch);
		return;
	}

	SET_BIT(pClan->status, CLAN_DISBANDING);

	send_to_char("You have initiated the disband of your clan.\r\n", ch);
	log("CLAN: Clan %s (%d) set to disbanding status.", pClan->name, pClan->id);
	fwrite_clan( pClan );
}

void do_clan_revive(CHAR_DATA *ch)
{
	CLAN_DATA *pClan;

	if (!(pClan = get_clan(GET_CLAN(ch))))
	{
		send_to_char("Problems with your clan... \r\n", ch);
		return;
	}

	if (!IS_SET(pClan->status, CLAN_DISBANDING))
	{
		send_to_char("Your clan is alive.\r\n", ch);
		return;
	}

	REMOVE_BIT(pClan->status, CLAN_DISBANDING);

	send_to_char("You have revived your clan form disbanding.\r\n", ch);
	log("CLAN: Clan %s (%d) revived from disbanding status.", pClan->name, pClan->id);
	fwrite_clan( pClan );
}

/* ---------------------------------------------------------------------- *
 * Funzioni per arruolare, invitare, promuovere, retrocedere e cacciare   *
 * pg dal clan                                                            *
 * ---------------------------------------------------------------------- */

/*
 * Enroll characters that have applied to the clan
 */
void do_clan_enroll(CHAR_DATA *ch, char *arg)
{
	CLAN_DATA *pClan;
	CHAR_DATA *vict = NULL;
	int immcom = 0;

	if (!(*arg))
	{
		send_clan_format(ch);
		return;
	}

	if (GET_LEVEL(vict) >= LVL_IMMORT) 
	{
		send_to_char("You cannot enroll immortals in clans.\r\n",ch);
		return;
	}

	pClan = get_clan(GET_CLAN(ch));

	if (!(vict = get_char_room_vis(ch, arg, NULL))) 
	{
		send_to_char("Er, Who ??\r\n",ch);
		return;
	}
	
	if (GET_CLAN(vict) != GET_CLAN(ch))
	{
		send_to_char("They didn't request to join your clan.\r\n",ch);
		return;
	}

	if (GET_CLAN_RANK(vict) != RANK_APPLIER)
	{
		act("$N has not applied for membership.", FALSE, ch, NULL, vict, TO_CHAR);
		return;
	}

	if (GET_LEVEL(vict) < pClan->app_level)
	{
		act("$N doesn't have enough experience to be a member of your clan.", FALSE, ch, NULL, vict, TO_CHAR);
		return;
	} 

	if (GET_LEVEL(vict) > pClan->max_level)
	{
		act("$N is too powerful to be a member of your clan.", FALSE, ch, NULL, vict, TO_CHAR);
		return;
	}

	if (!check_membership(vict, pClan))
	{
		act("Sorry, but $N cannot be a member of the clan.", FALSE, ch, NULL, vict, TO_CHAR);
		return;
	}
	
	/* all right, now enroll him/her */
	GET_CLAN_RANK(vict) = RANK_MEMBER_FIRST;
	save_char(vict, NULL);
	pClan->power += GET_LEVEL(vict);
	pClan->members++;
	
	if ( pClan->members >= MIN_CLAN_MEMBERS && IS_SET(pClan->status, CLAN_FORMING) )
	{
		REMOVE_BIT(pClan->status, CLAN_FORMING);
		SET_BIT(pClan->status, CLAN_ACTIVE);
		send_to_char("Your clan has reached the minimum number of members allowed.\r\n"
			 "From now the clan can act as normal.\r\n", ch);
		log( " '%s' (%d) clan is now active.", pClan->name, pClan->id );
	}

	if ( pClan->members == MAX_CLAN_MEMBERS )
	{
		REMOVE_BIT(pClan->status, CLAN_ACTIVE);
		SET_BIT(pClan->status, CLAN_INACTIVE);
		send_to_char("Your clan has reached the maximum number of members allowed.\r\n", ch);
	}

	sprintf(buf, "You've been enrolled in '%s' clan!\r\n", pClan->name );
	send_to_char(buf, vict);
	send_to_char("Done.\r\n", ch);
}


/*
 * Kick off from the clan unwanted members
 */ 
void do_clan_expel(CHAR_DATA *ch, char *arg)
{
	CLAN_DATA *pClan;
	CHAR_DATA *vict = NULL;
	int immcom = 0;

	if (!(*arg))
	{
		send_clan_format(ch);
		return;
	}

	pClan = get_clan( GET_CLAN(ch) );

	if (!(vict = get_char_room_vis(ch, arg, NULL)))
	{
		send_to_char("Er, Who ??\r\n", ch);
		return;
	}

	if (GET_CLAN(vict) != pClan->id)
	{
		send_to_char("They're not in your clan.\r\n",ch);
		return;
	}
	
	if (GET_CLAN_RANK(vict) >= GET_CLAN_RANK(ch))
	{
		send_to_char("You cannot kick out that person.\r\n",ch);
		return;
	}
	
	GET_CLAN(vict)		= -1;
	GET_CLAN_RANK(vict)	= -1;
	save_char(vict, NULL);
	pClan->power		-= GET_LEVEL(vict);
	pClan->members--;

	if (pClan->members < MIN_CLAN_MEMBERS)
	{
		REMOVE_BIT(pClan->status, CLAN_ACTIVE);
		SET_BIT(pClan->status, CLAN_FORMING);
	}
	if (IS_SET(pClan->status, CLAN_INACTIVE))
	{
		REMOVE_BIT(pClan->status, CLAN_INACTIVE);
		SET_BIT(pClan->status, CLAN_ACTIVE);
	}

	send_to_char("You've been kicked out of your clan!\r\n", vict);
	send_to_char("Done.\r\n", ch);
}


/*
 * Demote a clannie
 */
void do_clan_demote(CHAR_DATA *ch, char *arg)
{
	CLAN_DATA *pClan;
	CHAR_DATA *vict = NULL;
	int immcom = 0;

	if (!(*arg))
	{
		send_clan_format(ch);
		return;
	}

	pClan = get_clan(GET_CLAN(ch));

	if (!(vict = get_char_room_vis(ch, arg, NULL)))
	{
		send_to_char("Er, Who ??\r\n", ch);
		return;
	}

	if (GET_CLAN(vict) != pClan->id)
	{
		send_to_char("They're not in your clan.\r\n", ch);
		return;
	}
	
	if (GET_CLAN_RANK(vict) == 0 || GET_CLAN_RANK(vict) == RANK_INVITED)
	{
		send_to_char("They're not enrolled yet.\r\n", ch);
		return;
	}
	
	if (GET_CLAN_RANK(vict) == RANK_MEMBER_FIRST)
	{
		send_to_char("They can't be demoted any further, use expel now.\r\n", ch);
		return;
	}
	
	if (GET_CLAN_RANK(vict) >= GET_CLAN_RANK(ch))
	{
		send_to_char("You cannot demote a person of this rank!\r\n", ch);
		return; 
	}
	
	/* hanlde those special ranks above 6 */
	if (GET_CLAN_RANK(vict) > MAX_RANK_ADVANCE)
	{
		if (GET_CLAN_RANK(vict) == RANK_HERO)
			pClan->heroes--;
		else if (GET_CLAN_RANK(vict) == RANK_MAGISTRATE)
			pClan->magistrates--;
		else if (GET_CLAN_RANK(vict) == RANK_DIPLOMAT)
			pClan->ambassadors--;
		else
		{
			send_to_char("Leader or Patrone cannot be demoted.\r\n", ch);
			return;
		}
		GET_CLAN_RANK(vict) = MAX_RANK_ADVANCE;
	}
	else
		GET_CLAN_RANK(vict)--;
	save_char(vict, NULL);
	send_to_char("You've demoted within your clan!\r\n", vict);
	send_to_char("Done.\r\n", ch);
}


/*
 * Promote a clannie
 */
void do_clan_promote(CHAR_DATA *ch, char *arg)
{
	CLAN_DATA *pClan;
	CHAR_DATA *vict = NULL;

	if (!(*arg))
	{
		send_clan_format(ch);
		return;
	}
	
	pClan = get_clan(GET_CLAN(ch));
	
	if (!(vict = get_char_room_vis(ch, arg, NULL)))
	{
		send_to_char("Er, Who ??\r\n", ch);
		return;
	}
	
	if (GET_CLAN(vict) != pClan->id)
	{
		send_to_char("They're not in your clan.\r\n", ch);
		return;
	}
	
	if (GET_CLAN_RANK(vict) == 0 || GET_CLAN_RANK(vict) == RANK_INVITED)
	{
		send_to_char("They're not enrolled yet.\r\n", ch);
		return;
	}
	
	if (GET_CLAN_RANK(vict) >= GET_CLAN_RANK(ch))
	{
		send_to_char("You cannot promote that person over your rank!\r\n",ch);
		return;
	}
	
	if (GET_CLAN_RANK(vict) == MAX_RANK_ADVANCE)
	{
		send_to_char("You cannot promote someone over the top reachable rank!\r\n",ch);
		return;
	}
	
	GET_CLAN_RANK(vict)++;
	save_char(vict, NULL);
	send_to_char("You've been promoted within your clan!\r\n", vict);
	send_to_char("Done.\r\n", ch);
}



/*
 * Invite someone in your clan
 */
void do_clan_invite(CHAR_DATA *ch, char *arg)
{
	CLAN_DATA *pClan;
	CHAR_DATA *vict = NULL;

	if (!(*arg))
	{
		send_clan_format(ch);
		return;
	}

	pClan = get_clan(GET_CLAN(ch));

	if (!(vict = get_char_room_vis(ch, arg, NULL)))
	{
		send_to_char("Er, Who ??\r\n", ch);
		return;
	}
	
	if (GET_CLAN(vict) > 0)
	{
		sprintf(buf, "%s already belongs to a clan.\r\n", GET_NAME(vict));
		send_to_char(buf, ch);
		return;
	}
	
	if (!check_membership(vict, pClan))
	{
		sprintf(buf, "Sorry, but %s cannot be a member of your clan.\r\n", GET_NAME(vict));
		send_to_char(buf, ch);
		return;
	}
	
	GET_CLAN(vict)		= GET_CLAN(ch);
	GET_CLAN_RANK(vict)	= RANK_INVITED;
	save_char(vict, NULL);
	sprintf(buf, "You've been invited by %s to apply to '%s' clan!\r\n", GET_NAME(ch), pClan->name);
	send_to_char(buf, vict);
	send_to_char("Done.\r\n", ch);
}

/*
 * build a building for the clan
 */
void do_clan_build(CHAR_DATA *ch, char *arg)
{
	if ( !*arg )
	{
		send_to_char("What do you want to build for your clan?\r\n", ch);
		return;
	}

	create_building(ch, arg, GET_CLAN(ch));
}

/* ---------------------------------------------------------------------- */

/*
 * Handling money of the clan
 */
void do_clan_bank(CHAR_DATA *ch, char *arg, int action)
{
	CLAN_DATA *pClan;
	int amount = 0;

	if (!(*arg))
	{
	  send_clan_format(ch);
		return;
	}

	if (GET_CLAN_RANK(ch) < RANK_PRIVILEGES || GET_CLAN_RANK(ch) > RANK_PATRON)
	{
		if (action == CB_WITHDRAW)
		{
			send_to_char("You're not influent enough in the clan to do that!\r\n",ch);
			return;
		}
	}
	
	pClan = get_clan(GET_CLAN(ch));
	
	if (!(*arg))
	{
		if ( action == CB_DEPOSIT )
			send_to_char("How many coins would you deposit?\r\n",ch);
		else
			send_to_char("How many coins would you withdraw?\r\n",ch);
		return;
	}
	
	if (!is_number(arg))
	{
		send_to_char("That's not a monetary amount!\r\n",ch);
		return;
	}
	
	amount = atoi(arg);
	
	if (action == CB_DEPOSIT && get_gold(ch) < amount)
	{
		send_to_char("You do not have that kind of money!\r\n", ch);
		return;
	}
	
	if (action == CB_WITHDRAW && pClan->treasure < amount)
	{
		send_to_char("The clan is not wealthy enough for your needs!\r\n", ch);
		return;
	}
	
	switch (action)
	{
	case CB_WITHDRAW:
		create_amount(amount, ch, NULL, NULL, NULL, FALSE);
		pClan->treasure -= amount;
		send_to_char("You withdraw from the clan's treasure.\r\n", ch);
		break;
	case CB_DEPOSIT:
		sub_gold(ch, amount);
		pClan->treasure += amount;
		send_to_char("You add to the clan's treasure.\r\n", ch);
		break;
	default:
		send_to_char("Problem in command, please report.\r\n", ch);
		break;
	}
	save_char(ch, NULL);
	fwrite_clan(pClan);
}


/* ---------------------------------------------------------------------- */

/*
 * Which members of the clan are playing?
 */
void do_clan_who(CHAR_DATA *ch)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *tch;
	char line_disp[MAX_STRING_LENGTH];

	if (GET_CLAN_RANK(ch) == 0 || GET_CLAN_RANK(ch) == RANK_INVITED)
	{
		send_to_char("You are not been accepted by your clan, yet.\r\n", ch);
		return;
	}

	send_to_char("\r\nList of your clan members\r\n", ch);
	send_to_char("-------------------------\r\n", ch);
	for (d = descriptor_list; d; d = d->next)
	{
		if (d->connected)
			continue;
		if ((tch = d->character))
		{
			if (GET_CLAN(tch) == GET_CLAN(ch) && GET_CLAN_RANK(tch) > 0)
			{
				sprintf(line_disp,"(%20s) %s\r\n", GET_RANK_NAME(tch), GET_NAME(tch));
				send_to_char(line_disp,ch);
			}
		}
	}
}


/*
 * clannie's status 
 */
void do_clan_status(CHAR_DATA *ch)
{
	CLAN_DATA *pClan;
	char line_disp[MAX_STRING_LENGTH];

	if (IS_IMMORTAL(ch))
	{
		send_to_char("You are an immortal and cannot join any clan!\r\n", ch);
		return;
	}

	pClan = get_clan(GET_CLAN(ch));

	if (GET_CLAN_RANK(ch) == 0)
		sprintf(line_disp, "You applied to '%s'\r\n", pClan->name);
	else if (GET_CLAN_RANK(ch) == RANK_INVITED)
		sprintf(line_disp, "You have been invited to apply to '%s'\r\n", pClan->name);
	else
		sprintf(line_disp, "You are %s (Rank %d) of '%s' (ID %d)\r\n",
				GET_RANK_NAME(ch), GET_CLAN_RANK(ch), pClan->name, pClan->id);

	send_to_char(line_disp, ch);
}


/*
 * List of clans (no args) or detailed infos about the clan
 */
void do_clan_info(CHAR_DATA *ch, char *arg)
{
	CLAN_DATA *pClan;
	int i = 0;
	char buf9[10];
	char ctypes[128], cstat[128], cant1[128], cant2[128], cant3[128];

	if (num_of_clans == 0)
	{
		send_to_char("No clans have formed yet.\r\n",ch);
		return;
	}

	strcpy(buf, "Clan list:\r\n");

	if (!(*arg))
	{
		for (pClan = clans_table; pClan; pClan = pClan->next)
		{
			strcpy(buf9, "   ");

			if (IS_SET(pClan->ctype, CLAN_SECRET))
			{
				if (GET_LEVEL(ch) < LVL_GRGOD && GET_CLAN(ch) != pClan->id)
					continue;
				else
					strcpy(buf9, "(S)");
			}

			if (IS_SET(pClan->ctype, CLAN_PRIVATE))
				strcpy(buf9, "(P)");

			sprintf(buf + strlen(buf),
				"[%-2d] %s %-35s %-5s Members: %3d  Power: %5d  Appfee: %d\r\n", 
				pClan->id, buf9, pClan->name, pClan->abbr, pClan->members,
				pClan->power, pClan->app_fee);
		}

		page_string(ch->desc,buf, 1);
		return;
	}
	else
	{
		if ( !(pClan = find_clan(arg) ) )
		{
			send_to_char("Unknown clan.\r\n", ch);
			return;
		}
	}

	if (IS_SET(pClan->ctype, CLAN_SECRET) && GET_LEVEL(ch) < LVL_GRGOD &&
		GET_CLAN(ch) != pClan->id )
	{
		send_to_char("Unknown Clan.\r\n", ch);
		return;
	}

	if ( pClan->ctype )
		sprintbit( pClan->ctype, clan_types_table, ctypes );
	else
		strcpy( ctypes, "None");
	if ( pClan->status )
		sprintbit( pClan->status, clan_status_table, cstat );
	else
		strcpy( cstat, "None");
	if ( pClan->anti_class )
		sprintbit( pClan->anti_class, pc_class_types, cant1 );
	else
		strcpy( cant1, "None");
	if ( pClan->anti_race )
		sprintbit( pClan->anti_race, pc_race_types, cant2 );
	else
		strcpy( cant2, "None");
	if ( pClan->anti_other )
		sprintbit( pClan->anti_other, clan_anti_table, cant3 );
	else
		strcpy( cant3, "None");

	sprintf(buf,
		"\r\n&b&7Info for clan&0 <<&b&3%s&0>>:\r\n\r\n"
		"Abbreviation       : %s\r\n"
		"Leader             : %s\r\n"
		"Members            : %d\r\n"
		"Power              : %d\r\n"
		"Treasure           : %d\r\n"
		"Application fee    : %d gold coins\r\n"
		"Monthly Dues       : %d gold coins\r\n"
		"Application level  : %d\r\n"
		"Max level          : %d\r\n"
		"Clan Type          : %s\r\n"
		"Clan Status        : %s\r\n"
		"Class restrictions : %s\r\n"
		"Race restrictions  : %s\r\n"
		"Other restrictions : %s\r\n",
		pClan->name, pClan->abbr, pClan->leader,
		pClan->members, pClan->power, pClan->treasure,
		pClan->app_fee, pClan->dues, pClan->app_level,
		pClan->max_level, ctypes, cstat, cant1, cant2, cant3
		);
	send_to_char(buf, ch);
}



/* ---------------------------------------------------------------------- */
/* Funzioni chiamate da Set */
/* ---------------------------------------------------------------------- */

/*
 * Set the application fee and the monthly dues
 */
void do_clan_money(CHAR_DATA *ch, char *arg, int action)
{
	CLAN_DATA *pClan;
	int amount = 0;

	if (!(*arg))
	{
		send_clan_format(ch);
		return;
	}

	pClan = get_clan(GET_CLAN(ch));
	
	if (!(*arg))
	{
		send_to_char("Set it to how much?\r\n", ch);
		return;
	}
	
	if (!is_number(arg))
	{
		send_to_char("Set it to what?\r\n", ch);
		return;
	}
	
	amount = atoi(arg);
	
	switch (action)
	{
	case CM_APPFEE:
		pClan->app_fee = amount;
		send_to_char("You change the application fee.\r\n", ch);
		break;
	case CM_DUES:
		pClan->dues = amount;
		send_to_char("You change the monthly dues.\r\n", ch);
		break;
	default:
		send_to_char("Problem in command, please report.\r\n", ch);
		break;
	}
	
	fwrite_clan( pClan );
}


/*
 * Set the minimum application level to the clan
 */
void do_clan_application(CHAR_DATA *ch, char *arg)
{
	CLAN_DATA *pClan;
	int applevel;

	if (!(*arg))
	{
		send_clan_format(ch);
		return;
	}

	pClan = get_clan(GET_CLAN(ch));

	if (!(*arg))
	{
		send_to_char("Set to which level?\r\n", ch);
		return;
	}
	
	if (!is_number(arg))
	{
		send_to_char("Set the application level to what?\r\n", ch);
		return;
	}
	
	applevel = atoi(arg);
	
	if (applevel < 2 || applevel > (LVL_IMMORT - 1))
	{
		sprintf(buf, "The application level can go from 2 to %d.\r\n", (LVL_IMMORT - 1)); 
		send_to_char(buf, ch);
		return;
	}
	
	pClan->app_level = applevel;
	fwrite_clan( pClan );
}

/*
 * Nomina ad uno dei rank speciali:
 *
 * - diplomatico (gestisce le relazioni con altri clan)
 * - magistrato (amministratore del clan)
 * - eroe (comandante in guerra)
 */
void do_clan_specialrank(CHAR_DATA *ch, char *rank, char *arg)
{
	CLAN_DATA *pClan;
	CHAR_DATA *vict = NULL;
	
	if (!(*arg))
	{
		send_clan_format(ch);
		return;
	}
	
	pClan = get_clan(GET_CLAN(ch));
	
	if (!(vict = get_char_room_vis(ch, arg, NULL)))
	{
		send_to_char("Er, Who ??\r\n", ch);
		return;
	}
	
	if (GET_CLAN(vict) != pClan->id)
	{
		send_to_char("They're not in your clan.\r\n", ch);
		return;
	}
	
	if (GET_CLAN_RANK(vict) < RANK_MEMBER_FIRST)
	{
		send_to_char("They're not enrolled yet.\r\n", ch);
		return;
	}
	
	if ( !isname( rank, "diplomat magistrate hero" ) )
	{
		log( "SYSERR: clan_specialrank() : unknown special rank %s", rank );
		sprintf( buf, "%s: Rango sconosciuto.\r\n", rank );
	}
	else
	{
		if ( isname( rank, "diplomat") )
		{
			GET_CLAN_RANK(vict) = RANK_DIPLOMAT;
			pClan->ambassadors++;
			sprintf(buf, "%s is now a diplomat of your clan.\r\n", GET_NAME(vict));
			send_to_char("Now you are a diplomat of your clan.\r\n", vict);
		}
		else if ( isname( rank, "magistrate" ) )
		{
			GET_CLAN_RANK(vict) = RANK_MAGISTRATE;
			pClan->magistrates++;
			sprintf(buf, "%s is now a magistrate of your clan.\r\n", GET_NAME(vict));
			send_to_char("Now you are a magistrate of your clan.\r\n", vict);
		}
		else if ( isname( rank, "hero" ) )
		{
			GET_CLAN_RANK(vict) = RANK_HERO;
			pClan->heroes++;
			sprintf(buf, "%s is now a hero of your clan.\r\n", GET_NAME(vict));
			send_to_char("Now you are a hero of your clan.\r\n", vict);
		}
		save_char(vict, NULL);
		fwrite_clan( pClan );
	}
	send_to_char(buf, ch);
}


/*
 * solo per diplomatici
 */
ACMD(do_negotiate)
{
	CLAN_DATA *chclan, *tclan;
	char buf[MAX_STRING_LENGTH];
	char arg1[MAX_STRING_LENGTH];
	char arg2[MAX_STRING_LENGTH];
	sh_int target_clan;
	
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	
	if (IS_NPC(ch) || GET_CLAN_RANK(ch) != RANK_DIPLOMAT)
	{
		send_to_char( "This command is for clan diplomats only.\r\n", ch );
		return;
	}
	
	if (!is_number(arg1) || !(*arg2))
	{
		send_to_char( "Syntax: Negotiate <clan number> <peace/war/end>\r\n", ch );
		return;
	}
	
	target_clan = atoi(arg1);
	if ((target_clan < 1) || (target_clan > (MAX_CLANS - 1)))
	{
		send_to_char("That is not a clan!\r\n", ch);
		return;
	}

	if (target_clan == GET_CLAN(ch))
	{
		send_to_char("Well, that will accomplish a lot.. you must be of two minds about the whole thing!\r\n", ch );
		return;
	}

	if ( !(tclan = get_clan(target_clan) ) )
	{ 
		send_to_char("That clan does not exist!\r\n", ch);
		return;
	}

	if (IS_SET(tclan->ctype, CLAN_SECRET))
	{
		send_to_char("That clan does not exist!\r\n", ch);
		return;
	}
	
	chclan = get_clan(GET_CLAN(ch));

	if (IS_SET(chclan->ctype, CLAN_SECRET))
	{
		send_to_char("Yours is a secret clan! You have no needs of negotiations.\r\n", ch);
		return;
	}

	if (politics_data.daily_negotiate_table[chclan->id][tclan->id])
	{
		sprintf(buf, "Your clan has already negotiated with %s today.\r\n", tclan->name);
		send_to_char(buf, ch);
		return;
	}
	
	/* P E A C E */
	if (!str_cmp(arg2, "peace"))
	{
		if (politics_data.diplomacy[chclan->id][tclan->id] < -450)
		{
			if (politics_data.end_current_state[chclan->id][tclan->id] &&
				politics_data.end_current_state[tclan->id][chclan->id])
			{
				politics_data.diplomacy[chclan->id][tclan->id] = -425;
				politics_data.diplomacy[tclan->id][chclan->id] = -425;
				
				send_to_char("You have successfully negotiated an end to this dreaded CLAN WAR. Great Job!!\r\n", ch);
				sprintf(buf, "CLAN: The war between %s and %s has ended. They may no longer PKILL each other!!\r\n",
					chclan->name, tclan->name);
				send_to_all(buf);
				
				politics_data.end_current_state[chclan->id][tclan->id] = FALSE;
				politics_data.end_current_state[tclan->id][chclan->id] = FALSE;

			}
			else
			{
				sprintf(buf, "You are currently at WAR with %s."
					"Both clans must negotiate an end to the war first.\r\n", 
					tclan->name);
				send_to_char(buf, ch);
				return;
			}
		}
		else
		{
			politics_data.diplomacy[chclan->id][tclan->id] += 25;
			politics_data.diplomacy[tclan->id][chclan->id] += 25;
			
			sprintf(buf, "You are requesting a more peaceful state of affairs with %s.\r\n", 
				tclan->name );
			send_to_char(buf, ch);
		}

	}
	/* W A R */
	else if (!str_cmp(arg2, "war"))
	{
		if ((politics_data.diplomacy[chclan->id][tclan->id] - 50) < -450)
		{
			sprintf(buf, "You have started a WAR with %s! Watch out!.\r\n", tclan->name);
			send_to_char(buf, ch);
			
			sprintf(buf, "CLAN: A war has started between %s and %s. They may now PKILL each other!!\r\n",
				chclan->name, tclan->name);
			send_to_all(buf); 
			
			politics_data.diplomacy[chclan->id][tclan->id] = -460;
			politics_data.diplomacy[tclan->id][chclan->id] = -460;
			politics_data.end_current_state[chclan->id][tclan->id] = FALSE;
			politics_data.end_current_state[tclan->id][chclan->id] = FALSE;
		}
		else
		{
			politics_data.diplomacy[chclan->id][tclan->id] -= 50;
			politics_data.diplomacy[tclan->id][chclan->id] -= 50;
			
			sprintf(buf, "You are requesting a more aggressive state of affairs with %s.\r\n", tclan->name);
			send_to_char(buf, ch);
		}

	}
	/* E N D */
	else if (!str_cmp(arg2, "end"))
	{
		if (politics_data.diplomacy[chclan->id][tclan->id] > 450 )
		{
			send_to_char("You are negotiating an end to your alliance.\r\n", ch);
			politics_data.end_current_state[chclan->id][tclan->id] = FALSE;
			politics_data.end_current_state[tclan->id][chclan->id] = FALSE;
			politics_data.diplomacy[chclan->id][tclan->id] = 100;
			politics_data.diplomacy[tclan->id][chclan->id] = 100;
		}

		if (politics_data.diplomacy[chclan->id][tclan->id] < -450 )
		{
			politics_data.end_current_state[chclan->id][tclan->id] = TRUE;
			
			if (politics_data.end_current_state[chclan->id][tclan->id] &&
				politics_data.end_current_state[tclan->id][chclan->id])
				send_to_char("Both clans have successfully negotiated and end to the war.  Negotiate peace to seal your treaty!\r\n", ch);
			else
				send_to_char("You have requested an end to this dreaded war, but the other clan has not yet agreed.\r\n", ch);
		}
		else
		{
			send_to_char("You must be either at war or in an alliance with a clan before you can END it.\r\n", ch);
			return;
		}
	}
	else
	{
		send_to_char( "That is not a legal diplomatic negotiation!\n\r", ch );
		return;
	}

	politics_data.daily_negotiate_table[chclan->id][tclan->id] = TRUE;
	
	/* save table to file here */
	save_clan_table();
}

/*
 * Character ask to be allowed to be members of the clan
 */
void do_clan_apply(CHAR_DATA *ch, char *arg)
{
	CLAN_DATA *pClan;
	
	if (!(*arg))
	{
		send_clan_format(ch);
		return;
	}
	
	if (GET_LEVEL(ch) >= LVL_IMMORT)
	{
		send_to_char("Gods cannot apply for any clan.\r\n", ch);
		return;
	}
	
	if (GET_CLAN(ch) != -1)
	{
		send_to_char("You already belong to a clan!\r\n",ch);
		return;
	}
	
	if (!(pClan = find_clan(arg)))
	{
		send_to_char("Unknown clan.\r\n", ch);
		return;
	}
	
	if (IS_SET(pClan->ctype, CLAN_SECRET) && GET_CLAN_RANK(ch) != RANK_INVITED)
	{
		send_to_char("Unknown clan.\r\n", ch);
		return;
	}
	
	if (IS_SET(pClan->status, CLAN_INACTIVE) || IS_SET(pClan->status, CLAN_DISBANDING))
	{
		send_to_char("Sorry, but the clan does not accept new members at the moment.\r\n", ch);
		return;
	}
	
	if (IS_SET(pClan->ctype, CLAN_PRIVATE) && GET_CLAN_RANK(ch) != RANK_INVITED)
	{
		send_to_char("Sorry, but you have to be invited to join the clan before apply.\r\n", ch);
		return;
	}
	
	if (GET_LEVEL(ch) < pClan->app_level)
	{
		send_to_char("You are not mighty enough to apply to this clan.\r\n",ch);
		return;
	}
	
	if (GET_LEVEL(ch) > pClan->max_level)
	{
		send_to_char("You are too mighty to apply to this clan.\r\n",ch);
		return;
	}
	
	if (!check_membership(ch, pClan))
	{
		send_to_char("Sorry, but you cannot be a member of this clan.\r\n", ch);
		return;
	}
	
	if ( get_gold(ch) < pClan->app_fee)
	{
		send_to_char("You cannot afford the application fee!\r\n", ch);
		return;
	}
	
	sub_gold(ch, pClan->app_fee);
	pClan->treasure += pClan->app_fee;
	fwrite_clan(pClan);

	GET_CLAN(ch) = pClan->id;
	GET_CLAN_RANK(ch) = RANK_APPLIER;
	save_char(ch, NULL);
	send_to_char("You've applied to the clan!\r\n",ch);
}



/* core of clan command */

void do_clan_set(CHAR_DATA *ch, char *arg)
{
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

	half_chop(arg, arg1, arg2);

	if (is_abbrev(arg1, "dues"      )) { do_clan_money(ch, arg2, CM_DUES);   return ;}
	if (is_abbrev(arg1, "appfee"    )) { do_clan_money(ch, arg2, CM_APPFEE); return ;}
	if (is_abbrev(arg1, "applev"    )) { do_clan_application(ch, arg2);      return ;}
	send_clan_format(ch);
}


ACMD(do_clan)
{
	char arg1[MAX_INPUT_LENGTH], *arg2;
	
	arg2 = one_argument(argument, arg1);
	skip_spaces(&arg2);
	
	if (is_abbrev(arg1, "apply" ))	{ do_clan_apply(ch, arg2);	return; }
	if (is_abbrev(arg1, "info"  ))	{ do_clan_info(ch, arg2);	return; }
	
	// no clan member.. handle gods that wanna create a clan
	if (GET_CLAN(ch) == -1 || !get_clan(GET_CLAN(ch)))
	{
		if (GET_LEVEL(ch) >= LVL_CLAN_GOD && isname(arg1, "create"))
			do_clan_create(ch, arg2);
		else
		{ 
			send_to_char("You do not belong to a clan!\r\n",ch);
			send_clan_format(ch);
		}
		return;
	}

	// unranked ppl exit here..
	if (GET_CLAN_RANK(ch) <= RANK_APPLIER)
		return;
	
	/* some checks to see if he can use that command */
	if (isname(arg1, "disband revive magistrate hero") && GET_CLAN_RANK(ch) < RANK_LEADER)
	{
		send_to_char("You're not influent enough in the clan to do that!\r\n", ch);
		return;
	}
	
	if (isname(arg1, "build enroll expel demote promote invite set diplomat") &&
		GET_CLAN_RANK(ch) < RANK_PRIVILEGES)
	{
		send_to_char("You're not influent enough in the clan to do that!\r\n", ch);
		return;
	}
	
	/* Gods or God Patron only */
	if (is_abbrev(arg1, "create"		)) { do_clan_create(ch, arg2);	return; }
	if (is_abbrev(arg1, "disband"		)) { do_clan_disband(ch);		return; }
	if (is_abbrev(arg1, "revive"		)) { do_clan_revive(ch);		return; }
	
	/* Privileged members only */
	if (is_abbrev(arg1, "build"			)) { do_clan_build(ch, arg2);	return; }
	if (is_abbrev(arg1, "demote"		)) { do_clan_demote(ch, arg2);	return; }
	if (is_abbrev(arg1, "enroll"		)) { do_clan_enroll(ch, arg2);	return; }
	if (is_abbrev(arg1, "expel"			)) { do_clan_expel(ch, arg2);	return; }
	if (is_abbrev(arg1, "invite"		)) { do_clan_invite(ch, arg2);	return; }
	if (is_abbrev(arg1, "promote"		)) { do_clan_promote(ch, arg2);	return; }
	if (is_abbrev(arg1, "set"			)) { do_clan_set(ch, arg2);		return; }

	if (isname(arg1, "diplomat magistrate hero"))
	{
		do_clan_specialrank(ch, arg1, arg2);
		return;
	}

	if (is_abbrev(arg1, "withdraw"		)) { do_clan_bank(ch, arg2, CB_WITHDRAW); return ;}
	if (is_abbrev(arg1, "deposit"		)) { do_clan_bank(ch, arg2, CB_DEPOSIT);  return ;}
	
	/* all members */
	if (is_abbrev(arg1, "who"			)) { do_clan_who(ch);		return ;}
	if (is_abbrev(arg1, "status"		)) { do_clan_status(ch);	return ;}

	send_clan_format(ch);
}


/*
 * commands for everyone
 */

/* show what's going on in politic field */
char *get_diplo_name(sh_int value)
{
	char *name = '\0';
	
	if		( value < -450 )	name = "  WAR  ";
	else if ( value < -300 )	name = "HATRED ";
	else if ( value < -150 )	name = "DISLIKE";
	else if ( value < 150 )		name = "NEUTRAL";
	else if ( value < 300 )		name = "RESPECT";
	else if ( value < 450 )		name = " TRUST ";
	else						name = " ALLY  ";
	
	return (name);
}

ACMD(do_politics)
{
	CLAN_DATA *pClan, *pClan2;
	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
	
	if (IS_NPC(ch))
	{
		send_to_char("Not for Npcs.\r\n", ch);
		return;
	}
	
	send_to_char("Current Politics of Lyonesse Mud\r\n\r\n", ch);
	
	if (num_of_vis_clans() < 2)
	{
		send_to_char("Too few clans to have politics, sorry.\r\n", ch);
		return;
	}
	
	buf[0] = '\0';
	buf2[0] = '\0';
	
	sprintf(buf, "        ");
	strcat(buf2, buf);
	
	for (pClan = clans_table; pClan; pClan = pClan->next)
	{
		if (IS_SET(pClan->ctype, CLAN_SECRET))
			continue;
		
		sprintf(buf, " %-5s  ", pClan->abbr);
		strcat(buf2, buf);
	}
	
	buf[0] = '\0';
	sprintf(buf, "\r\n\r\n");
	strcat(buf2, buf);
	
	send_to_char(buf2 , ch);
	
	for (pClan = clans_table; pClan; pClan = pClan->next)
	{
		/* clan symbol here */
		buf[0] = '\0';
		buf2[0] = '\0';
		
		if (IS_SET(pClan->ctype, CLAN_SECRET) && GET_LEVEL(ch) < LVL_GRGOD)
			continue;
		
		sprintf(buf,"%1i %-5s ", pClan->id, pClan->abbr);
		strcat(buf2, buf);
		
		for (pClan2 = clans_table; pClan2; pClan2 = pClan2->next)
		{
			buf[0] = '\0';
			if (IS_SET(pClan2->ctype, CLAN_SECRET) && GET_LEVEL(ch) < LVL_GRGOD)
				continue;
			
			if (pClan->id != pClan2->id)
				sprintf(buf, "%-7s ", get_diplo_name(politics_data.diplomacy[pClan->id][pClan2->id]));
			else
				sprintf(buf, "        ");
			strcat(buf2, buf);
		}
		sprintf(buf, "\n\r\n\r");
		strcat(buf2, buf);
		send_to_char( buf2, ch );
	}
	
	if (GET_CLAN_RANK(ch) == RANK_DIPLOMAT)
	{
		for (pClan = clans_table; pClan; pClan = pClan->next)
		{
			if (politics_data.end_current_state[GET_CLAN(ch)][pClan->id])
			{
				sprintf(buf, "%s has requested an end to your current state of affairs", pClan->name);
				send_to_char(buf, ch);
			}
		}
	}
}

void show_clan_politic( CHAR_DATA *ch, int idnum )
{
	CLAN_DATA *pClan;

	if ( !ch || !get_clan(idnum) )
		return;

	for (pClan = clans_table; pClan; pClan = pClan->next)
	{
		if (pClan->id == idnum)
			continue;
		
		if (IS_SET(pClan->ctype, CLAN_SECRET) && GET_LEVEL(ch) < LVL_GRGOD)
			continue;
		
		ch_printf(ch, "Clan (%d) %s: --> %s <--\r\n",
			pClan->id, pClan->name,
			get_diplo_name(politics_data.diplomacy[idnum][pClan->id]) );
	}
}
