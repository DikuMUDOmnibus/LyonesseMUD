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
*   File: db.h                                          Part of CircleMUD *
*  Usage: header file for database handling                               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD				0
#define DB_BOOT_MOB				1
#define DB_BOOT_OBJ				2
#define DB_BOOT_ZON				3
#define DB_BOOT_SHP				4
#define DB_BOOT_HLP				5

#if defined(CIRCLE_MACINTOSH)
#define LIB_WORLD		":world:"
#define LIB_TEXT		":text:"
#define LIB_TEXT_HELP	":text:help:"
#define LIB_MISC		":misc:"
#define LIB_ETC			":etc:"
#define LIB_PLRTEXT		":plrtext:"
#define LIB_PLROBJS		":plrobjs:"
#define LIB_PLRALIAS	":plralias:"
#define LIB_HOUSE		":house:"
#define SLASH			":"
#elif defined(CIRCLE_AMIGA) || defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS) || defined(CIRCLE_ACORN) || defined(CIRCLE_VMS)
#define LIB_WORLD		"world/"
#define LIB_TEXT		"text/"
#define LIB_TEXT_HELP	"text/help/"
#define LIB_MISC		"misc/"
#define LIB_ETC			"etc/"
#define LIB_PLRTEXT		"plrtext/"
#define LIB_PLROBJS		"plrobjs/"
#define LIB_PLRALIAS	"plralias/"
#define LIB_PLAYERS 	"plrsave/"
#define LIB_HOUSE		"house/"
#define LIB_STABLES		"stables/"
#define LIB_BUILDINGS	"buildings/"
#define LIB_GOODS		"goods/"
#define LIB_CLANS		"clans/"
#define LIB_SHIPS		"ships/"
#define LIB_AREA		"area/"
#define LIB_DATA		"data/"

#define SLASH			"/"
#else
#error "Unknown path components."
#endif

#define SUF_OBJS		"objs"
#define SUF_TEXT		"text"
#define SUF_ALIAS		"alias"
#define SUF_PLAYERS		"txt"

#if defined(CIRCLE_AMIGA)
#define FASTBOOT_FILE   "/.fastboot"			/* autorun: boot without sleep			*/
#define KILLSCRIPT_FILE "/.killscript"			/* autorun: shut mud down				*/
#define PAUSE_FILE      "/pause"				/* autorun: don't restart mud			*/
#elif defined(CIRCLE_MACINTOSH)
#define FASTBOOT_FILE	"::.fastboot"			/* autorun: boot without sleep			*/
#define KILLSCRIPT_FILE	"::.killscript"			/* autorun: shut mud down				*/
#define PAUSE_FILE		"::pause"				/* autorun: don't restart mud			*/
#else
#define FASTBOOT_FILE   "../.fastboot"			/* autorun: boot without sleep			*/
#define KILLSCRIPT_FILE "../.killscript"		/* autorun: shut mud down				*/
#define PAUSE_FILE      "../pause"				/* autorun: don't restart mud			*/
#endif

/* names of various files and directories */
#define INDEX_FILE		"index"					/* index of world files				*/
#define MINDEX_FILE		"index.mini"			/* ... and for mini-mud-mode		*/
#define WLD_PREFIX		LIB_WORLD"wld"SLASH		/* room definitions					*/
#define MOB_PREFIX		LIB_WORLD"mob"SLASH		/* monster prototypes				*/
#define OBJ_PREFIX		LIB_WORLD"obj"SLASH		/* object prototypes				*/
#define ZON_PREFIX		LIB_WORLD"zon"SLASH		/* zon defs & command tables		*/
#define SHP_PREFIX		LIB_WORLD"shp"SLASH		/* shop definitions					*/
#define HLP_PREFIX		LIB_TEXT"help"SLASH		/* for HELP <keyword>				*/
#define WLS_PREFIX      LIB_WORLD"wls"SLASH     /* wilderness definitions			*/
#define BLD_PREFIX		LIB_WORLD"bld"SLASH		/* For building templates			*/
#define SHIP_PREFIX		LIB_WORLD"ship"SLASH	/* For ship templates				*/

#define CREDITS_FILE	LIB_TEXT"credits"		/* for the 'credits' command		*/
#define NEWS_FILE		LIB_TEXT"news"			/* for the 'news' command			*/
#define MOTD_FILE		LIB_TEXT"motd"			/* messages of the day / mortal		*/
#define IMOTD_FILE		LIB_TEXT"imotd"			/* messages of the day / immort		*/
#define GREETINGS_FILE	LIB_TEXT"greetings"		/* The opening screen.				*/
#define HELP_PAGE_FILE	LIB_TEXT_HELP"screen"	/* for HELP <CR>					*/
#define INFO_FILE		LIB_TEXT"info"			/* for INFO							*/
#define WIZLIST_FILE	LIB_TEXT"wizlist"		/* for WIZLIST						*/
#define IMMLIST_FILE	LIB_TEXT"immlist"		/* for IMMLIST						*/
#define BACKGROUND_FILE	LIB_TEXT"background"	/* for the background story			*/
#define POLICIES_FILE	LIB_TEXT"policies"		/* player policies/rules			*/
#define HANDBOOK_FILE	LIB_TEXT"handbook"		/* handbook for new immorts			*/
#define NEWSPAPER_FILE	LIB_TEXT"newspaper"		/* Trade Newspaper					*/

#define IDEA_FILE		LIB_MISC"ideas"			/* for the 'idea'-command			*/
#define TYPO_FILE		LIB_MISC"typos"			/*         'typo'					*/
#define BUG_FILE		LIB_MISC"bugs"			/*         'bug'					*/
#define MESS_FILE		LIB_MISC"messages"		/* damage messages					*/
#define SOCMESS_FILE	LIB_MISC"socials"		/* messages for social acts			*/
#define XNAME_FILE		LIB_MISC"xnames"		/* invalid name substrings			*/

#define MAIL_FILE		LIB_ETC"plrmail"		/* for the mudmail system			*/
#define BAN_FILE		LIB_ETC"badsites"		/* for the siteban system			*/
#define HCONTROL_FILE	LIB_ETC"hcontrol"		/* for the house system				*/
#define TIME_FILE		LIB_ETC"time"			/* for calendar system				*/
#define USERCNT_FILE	LIB_ETC"usercnt"		/* for counting users				*/

#define CLAN_DIP_FILE	LIB_CLANS"clandata"		/* stores clan diplomacy data		*/

#define PLR_INDEX_FILE	"plrsave"SLASH"plr_index"

#define SECT_FILE		LIB_DATA"sectors.data"
#define LIFE_FILE		LIB_DATA"life.data"

/* public procedures in db.c */
CHAR_DATA *create_char(void);
CHAR_DATA *read_mobile(mob_vnum nr, int type);
ROOM_DATA *get_room(room_vnum vnum);
ROOM_DATA *new_room( void );
OBJ_DATA *create_obj(void);
OBJ_DATA *read_object(obj_vnum nr, int type);
char	*fread_string(FILE *fl, const char *error);
char	*get_name_by_id(long id);
int		create_entry(char *name);
int		load_char(char *name, CHAR_DATA *ch);
int		vnum_mobile(char *searchname, CHAR_DATA *ch);
int		vnum_object(char *searchname, CHAR_DATA *ch);
int		vnum_weapon(int attacktype, CHAR_DATA *ch);
long	get_id_by_name(const char *name);
void	zone_update(void);
void	boot_db(void);
void	save_mud_time(TIME_INFO_DATA *when);
void	init_char(CHAR_DATA *ch);
void	clear_char(CHAR_DATA *ch);
void	reset_char(CHAR_DATA *ch);
void	free_char(CHAR_DATA *ch);
void	save_player_index(void);
void	SaveAll(bool bQuit);
void	save_char(CHAR_DATA *ch, ROOM_DATA *load_room);
void	SaveCharObj(CHAR_DATA *ch, bool quitting);
void	clear_object(OBJ_DATA *obj);
void	free_obj(OBJ_DATA *obj);
zone_rnum real_zone(zone_vnum vnum);
mob_rnum real_mobile(mob_vnum vnum);
obj_rnum real_object(obj_vnum vnum);


char	*fread_string_nospace( FILE *fp );
char	*fread_line( FILE *fp );
char	*fread_word( FILE *fp );
char	fread_letter( FILE *fp );
int		fread_number( FILE *fp );
void	fread_to_eol( FILE *fp );


/* ========================================= */

#define REAL 0
#define VIRTUAL 1

/* structure for the reset commands */

/*
        *****************************************************************
        *  Commands:                                                    *
        *                                                               *
        *  'D': Set state of door                                       *
        *                                                               *
        *  'E': Obj to char equip                                       *
        *                                                               *
        *  'G': Obj to char                                             *
        *                                                               *
        *  'H': Random Obj to room                                      *
        *       - arg1 : item type (add 100 for magical item)           *
        *       - arg2 : item level                                     *
        *       - arg3 : room vnum                                      *
        *                                                               *
        *  'I': Random Obj to char                                      *
        *       - arg1 : item type (add 100 for magical item)           *
        *       - arg2 : unused                                         *
        *       - arg3 : unused                                         *
        *                                                               *
        *  'J': Random Obj to char equip                                *
        *       - arg1 : item type (add 100 for magical item)           *
        *       - arg2 : unused                                         *
        *       - arg3 : unused                                         *
        *                                                               *
        *  'M': Read a mobile                                           *
        *                                                               *
        *  'O': Read an object                                          *
        *                                                               *
        *  'P': Put obj in obj                                          *
        *                                                               *
        *  'R': Remove obj from room                                    *
        *                                                               *
        *  'Z': Create a maze                                           *
        *       - arg1 : unused                                         *
        *       - arg2 : unused                                         *
        *       - arg3 : unused                                         *
        *                                                               *
        *****************************************************************
*/

struct reset_com
{
  char			command;						/* current command						*/
  bool			if_flag;						/* if TRUE: exe only if preceding exe'd	*/
  int			arg1;							/*										*/
  int			arg2;							/* Arguments to the command				*/
  int			arg3;							/*										*/
  int			line;							/* line number this command appears on	*/
};

struct zone_wild_data
{
  COORD_DATA	z_start;
  COORD_DATA	z_end;
  char			*survey;						/* description when see from wilderness	*/
  char			*flyto;							/* keyword for targeting with dragons	*/
  int			dist;							/* how far is visible?					*/
};

/* zone definition structure. for the 'zone-table'   */
/*
    * Reset mode:
    *   0: Don't reset, and don't update age.
    *   1: Reset if no PC's are located in zone.
    *   2: Just reset.
*/

struct zone_data
{
  RESET_COM		*cmd;							/* command table for reset				*/
  ZONE_WILD		wild;							/* wilderness info for zone				*/
  char			*name;							/* name of this zone					*/
  int			lifespan;						/* how long between resets (minutes)	*/
  int			age;							/* current age of this zone (minutes)	*/
  int			reset_mode;						/* conditions for reset (see below)		*/
  room_vnum		bot;							/* starting room number for this zone	*/
  room_vnum		top;							/* upper limit for rooms in this zone	*/
  zone_vnum		number;							/* virtual number of this zone			*/
};


/* for queueing zones for update   */
struct reset_q_element
{
  RESET_Q_ELEM	*next;
  zone_rnum		zone_to_reset;					/* ref to zone_data						*/
};



/* structure for the update queue     */
struct reset_q_type
{
   RESET_Q_ELEM *head;
   RESET_Q_ELEM *tail;
};


struct player_index_element
{
  char			*name;
  long			id;
};


struct help_index_element
{
  char			*keyword;
  char			*entry;
  int			duplicate;
};


/* don't change these */
#define BAN_NOT 				0
#define BAN_NEW 				1
#define BAN_SELECT				2
#define BAN_ALL					3

#define BANNED_SITE_LENGTH		50

struct ban_list_element
{
  BAN_LIST_ELEM	*next;
  char			site[BANNED_SITE_LENGTH+1];
  char			name[MAX_NAME_LENGTH+1];
  int			type;
  time_t		date;
};


/* global buffering system */

#ifdef __DB_C__
char	buf[MAX_STRING_LENGTH];
char	buf1[MAX_STRING_LENGTH];
char	buf2[MAX_STRING_LENGTH];
char	buf3[MAX_STRING_LENGTH];
char	arg[MAX_STRING_LENGTH];
#else
extern room_vnum		top_of_world;
extern obj_rnum			top_of_objt;
extern mob_rnum			top_of_mobt;
extern zone_rnum		top_of_zone_table;
extern int				top_of_p_table;
extern int				top_idnum;

extern ROOM_DATA		*World[ROOM_HASH];
extern ZONE_DATA		*zone_table;

extern DESCRIPTOR_DATA	*descriptor_list;
extern CHAR_DATA		*character_list;
extern PLAYER_SPECIAL	dummy_mob;

extern INDEX_DATA		*mob_index;
extern CHAR_DATA		*mob_proto;

extern INDEX_DATA		*obj_index;
extern OBJ_DATA			*first_object;
extern OBJ_DATA			*last_object;
extern OBJ_DATA			*obj_proto;

extern TERRAIN_DATA		*terrain_type[MAX_SECT];

extern PLR_INDEX_ELEM	*player_table;

extern SURVEY_DATA		*survey_table;
extern ROOM_AFFECT		*raff_list;

extern ROOM_DATA		*r_mortal_start_room;
extern ROOM_DATA		*r_immort_start_room;
extern ROOM_DATA		*r_frozen_start_room;

extern char				buf[MAX_STRING_LENGTH];
extern char				buf1[MAX_STRING_LENGTH];
extern char				buf2[MAX_STRING_LENGTH];
extern char				buf3[MAX_STRING_LENGTH];
extern char				arg[MAX_STRING_LENGTH];

extern int				Sunlight;
extern int				MoonPhase;
#endif

#ifndef __CONFIG_C__
extern char	*OK;
extern char	*NOPERSON;
extern char	*NOEFFECT;
#endif
