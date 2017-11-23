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
*   File: db.c                                          Part of CircleMUD *
*  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __DB_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"
#include "house.h"
#include "constants.h"
#include "shop.h"

/**************************************************************************
*  declarations of most of the 'global' variables                         *
**************************************************************************/

ROOM_DATA       *World[ROOM_HASH];              /* hash table of rooms                  */
CHAR_DATA       *character_list = NULL;         /* global linked list of chars          */
CHAR_DATA       *mob_proto;                     /* prototypes for mobs                  */
OBJ_DATA        *first_object   = NULL;         /* global linked list of objs           */
OBJ_DATA        *last_object    = NULL;         /* global linked list of objs           */
OBJ_DATA        *obj_proto;                     /* prototypes for objs                  */
INDEX_DATA      *mob_index;                     /* index table for mobile file          */
INDEX_DATA      *obj_index;                     /* index table for object file          */
ZONE_DATA       *zone_table;                    /* zone table                           */
PLR_INDEX_ELEM  *player_table   = NULL;         /* index to plr file                    */
HELP_INDEX_ELEM *help_table     = NULL;         /* the help table                       */
SURVEY_DATA     *survey_table   = NULL;         /* survey descriptions                  */
ROOM_AFFECT     *raff_list      = NULL;         /* global list of room affections       */

TERRAIN_DATA	*terrain_type[MAX_SECT];

int             top_of_world    = 0;            /* ref to top element of world          */
mob_rnum        top_of_mobt     = 0;            /* top of mobile index table            */
obj_rnum        top_of_objt     = 0;            /* top of object index table            */
zone_rnum       top_of_zone_table = 0;          /* top element of zone tab              */
int             top_of_helpt    = 0;            /* top of help index table              */
int             top_of_p_table  = 0;            /* ref to top of table                  */
int             no_mail         = 0;            /* mail disabled?                       */
int             mini_mud        = 0;            /* mini-mud mode?                       */
int             circle_restrict = 0;            /* level of game restriction            */
long            top_idnum       = 0;            /* highest idnum in use                 */
time_t          boot_time       = 0;            /* time of mud boot                     */

ROOM_DATA       *r_mortal_start_room;           /* rnum of mortal start room            */
ROOM_DATA       *r_immort_start_room;           /* rnum of immort start room            */
ROOM_DATA       *r_frozen_start_room;           /* rnum of frozen start room            */

int             Sunlight;                       /* What state the sun is at             */
int             MoonPhase;                      /* What state the moon is at            */

char            *credits        = NULL;         /* game credits                         */
char            *news           = NULL;         /* mud news                             */
char            *motd           = NULL;         /* message of the day - mortals         */
char            *imotd          = NULL;         /* message of the day - immorts         */
char            *GREETINGS      = NULL;         /* opening credits screen               */
char            *help           = NULL;         /* help screen                          */
char            *info           = NULL;         /* info page                            */
char            *wizlist        = NULL;         /* list of higher gods                  */
char            *immlist        = NULL;         /* list of peon gods                    */
char            *background     = NULL;         /* background story                     */
char            *handbook       = NULL;         /* handbook for new immortals           */
char            *policies       = NULL;         /* policies page                        */
char            *newspaper      = NULL;         /* Trade Newspaper                      */

MESSAGE_LIST    fight_messages[MAX_MESSAGES];   /* fighting messages                    */
TIME_INFO_DATA  time_info;                      /* the infomation about the time        */
PLAYER_SPECIAL  dummy_mob;                      /* dummy spec area for mobs             */
RESET_Q_TYPE    reset_q;                        /* queue of zones to be reset           */

/* local functions */
int check_object_spell_number(OBJ_DATA *obj, int val);
int check_object_level(OBJ_DATA *obj, int val);
void setup_dir(FILE *fl, ROOM_DATA *pRoom, int dir);
void setup_wild_exit(FILE *fl, ROOM_DATA *pRoom, int door);
void setup_zone_exit(FILE *fl, ROOM_DATA *pRoom, int door);
void index_boot(int mode);
void discrete_load(FILE *fl, int mode, char *filename);
int check_object(OBJ_DATA *);
void parse_room(FILE *fl, int virtual_nr);
void parse_mobile(FILE *mob_f, int nr);
char *parse_object(FILE *obj_f, int nr);
void load_zones(FILE *fl, char *zonename);
void load_help(FILE *fl);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void build_player_index(void);
int is_empty(zone_rnum zone_nr);
void reset_zone(zone_rnum zone);
int file_to_string(const char *name, char *buf);
int file_to_string_alloc(const char *name, char **buf);
void reboot_wizlists(void);
ACMD(do_reboot);
void boot_world(void);
int count_alias_records(FILE *fl);
int count_hash_records(FILE *fl);
bitvector_t asciiflag_conv(char *flag);
void parse_simple_mob(FILE *mob_f, int i, int nr);
void interpret_espec(const char *keyword, const char *value, int i, int nr);
void parse_espec(char *buf, int i, int nr);
void parse_enhanced_mob(FILE *mob_f, int i, int nr);
void get_one_line(FILE *fl, char *buf);
void check_start_rooms(void);
void renum_world( void );
void renum_zone_table(void);
void log_zone_error(zone_rnum zone, int cmd_no, const char *message);
void reset_time(void);
long get_ptable_by_name(const char *name);
void create_survey_table( void );

/* external functions */
TIME_INFO_DATA *mud_time_passed(time_t t2, time_t t1);
OBJ_DATA *load_rand_obj(int type, int level);
int		hsort(const void *a, const void *b);
int		find_first_step(ROOM_DATA *src, ROOM_DATA *target);
int		find_eq_pos(CHAR_DATA *ch, OBJ_DATA *obj, char *arg);
int		parse_trap_dam( char *damname );
void	paginate_string(char *str, DESCRIPTOR_DATA *d);
void	free_alias(ALIAS_DATA *a);
void	load_messages(void);
void	weather_and_time(int mode);
void	mag_assign_spells(void);
void	boot_social_messages(void);
void	sort_commands(void);
void	sort_spells(void);
void	load_banned(void);
void	Read_Invalid_List(void);
void	boot_the_shops(FILE *shop_f, char *filename, int rec_count);
void	LoadPortalStones(void);
void	WeatherInit(void);
void	ChangeMoon(void);
void	LoadVehiclesTable(void);
void	LoadStableRentIndex(void);
void	load_shop_nonnative(shop_vnum shop_num, CHAR_DATA *keeper);
void	LoadGoodTypes(void);
void	LoadGoods(void);
void	sort_goods_table();
void	LoadTradingPost(void);
void	BootMarkets(void);
void	init_clans(void);
void	setup_trigger(ROOM_DATA *pRoom, FILE *fl);
void	load_base_life(void);

/* Ships */
void	load_ports(void);
void	load_ship_table(void);
void	LoadShips(void);
void	LoadFerryboats(void);

/* wilderness */
ROOM_DATA *create_wild_room(COORD_DATA *coord, bool Static);
WILD_DATA *load_wildmap_sector(COORD_DATA *coord);
COORD_DATA *check_next_step(COORD_DATA *step, int dir);
int		get_sect(COORD_DATA *coord);
void	put_sect(int y, int x, int nSect, bool upMap);
void	setup_wild_static(ROOM_DATA *wRoom, ROOM_DATA *zRoom);
void	load_sectors(void);
/* Buildings */
void	LoadBldTemplate(void);
void	LoadBuildings(void);


/* external vars */
extern DESCRIPTOR_DATA	*descriptor_list;
extern SHOP_DATA		*shop_index;
extern SPELL_INFO_DATA	spell_info[];
extern const char		*unused_spellname;
extern int				no_specials;
extern int				scheck;
extern int				top_shop;
extern room_vnum		mortal_start_room;
extern room_vnum		immort_start_room;
extern room_vnum		frozen_start_room;
extern room_vnum		donation_room_1;

#define READ_SIZE		256

/*************************************************************************
*  routines for booting the system                                       *
*************************************************************************/

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
	file_to_string_alloc(WIZLIST_FILE, &wizlist);
	file_to_string_alloc(IMMLIST_FILE, &immlist);
}


/*
 * Too bad it doesn't check the return values to let the user
 * know about -1 values.  This will result in an 'Okay.' to a
 * 'reload' command even when the string was not replaced.
 * To fix later, if desired. -gg 6/24/99
 */
ACMD(do_reboot)
{
	int i;
	
	one_argument(argument, arg);
	
	if (!str_cmp(arg, "all") || *arg == '*')
	{
		if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
			prune_crlf(GREETINGS);
		file_to_string_alloc(WIZLIST_FILE, &wizlist);
		file_to_string_alloc(IMMLIST_FILE, &immlist);
		file_to_string_alloc(NEWS_FILE, &news);
		file_to_string_alloc(CREDITS_FILE, &credits);
		file_to_string_alloc(MOTD_FILE, &motd);
		file_to_string_alloc(IMOTD_FILE, &imotd);
		file_to_string_alloc(HELP_PAGE_FILE, &help);
		file_to_string_alloc(INFO_FILE, &info);
		file_to_string_alloc(POLICIES_FILE, &policies);
		file_to_string_alloc(HANDBOOK_FILE, &handbook);
		file_to_string_alloc(BACKGROUND_FILE, &background);
		file_to_string_alloc(NEWSPAPER_FILE, &newspaper);
	}
	else if (!str_cmp(arg, "wizlist"))
		file_to_string_alloc(WIZLIST_FILE, &wizlist);
	else if (!str_cmp(arg, "immlist"))
		file_to_string_alloc(IMMLIST_FILE, &immlist);
	else if (!str_cmp(arg, "news"))
		file_to_string_alloc(NEWS_FILE, &news);
	else if (!str_cmp(arg, "credits"))
		file_to_string_alloc(CREDITS_FILE, &credits);
	else if (!str_cmp(arg, "motd"))
		file_to_string_alloc(MOTD_FILE, &motd);
	else if (!str_cmp(arg, "imotd"))
		file_to_string_alloc(IMOTD_FILE, &imotd);
	else if (!str_cmp(arg, "help"))
		file_to_string_alloc(HELP_PAGE_FILE, &help);
	else if (!str_cmp(arg, "info"))
		file_to_string_alloc(INFO_FILE, &info);
	else if (!str_cmp(arg, "policy"))
		file_to_string_alloc(POLICIES_FILE, &policies);
	else if (!str_cmp(arg, "handbook"))
		file_to_string_alloc(HANDBOOK_FILE, &handbook);
	else if (!str_cmp(arg, "background"))
		file_to_string_alloc(BACKGROUND_FILE, &background);
	else if (!str_cmp(arg, "greetings"))
	{
		if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
			prune_crlf(GREETINGS);
	}
	else if (!str_cmp(arg, "xhelp"))
	{
		if (help_table)
		{
			for (i = 0; i <= top_of_helpt; i++)
			{
				if (help_table[i].keyword)
					free(help_table[i].keyword);
				if (help_table[i].entry && !help_table[i].duplicate)
					free(help_table[i].entry);
			}
			free(help_table);
		}
		top_of_helpt = 0;
		index_boot(DB_BOOT_HLP);
	}
	else
	{
		send_to_char("Unknown reload option.\r\n", ch);
		return;
	}
	
	send_to_char(OK, ch);
}

void boot_world(void)
{
	log("Loading the Wilderness.");
	load_sectors();
	
	log("Loading default Encounter Chart.");
	load_base_life();

	log("Loading zone table.");
	index_boot(DB_BOOT_ZON);
	
	log("Loading rooms.");
	index_boot(DB_BOOT_WLD);
	
	log("Renumbering rooms.");
	renum_world();
	
	log("Checking start rooms.");
	check_start_rooms();
	
	log("Loading mobs and generating index.");
	index_boot(DB_BOOT_MOB);
	
	log("Loading objs and generating index.");
	index_boot(DB_BOOT_OBJ);
	
	log("Renumbering zone table.");
	renum_zone_table();
	
	log("Generating survey table.");
	create_survey_table();
	
	log("Loading buildings templates.");
	LoadBldTemplate();

	log("Loading ports table.");
	load_ports();

	log("Loading ships data.");
	load_ship_table();

	log("Loading ships.");
	LoadShips();

	log("Loading ferryboats.");
	LoadFerryboats();

	if (!no_specials)
	{
		log("Loading shops.");
		index_boot(DB_BOOT_SHP);
	}
}


/* body of the booting system */
void boot_db(void)
{
	zone_rnum i;
	
	log("Boot db -- BEGIN.");
	
	log("Resetting the game time:");
	reset_time();
	
	log("Reading news, credits, help, bground, info & motds.");
	file_to_string_alloc(NEWS_FILE, &news);
	file_to_string_alloc(CREDITS_FILE, &credits);
	file_to_string_alloc(MOTD_FILE, &motd);
	file_to_string_alloc(IMOTD_FILE, &imotd);
	file_to_string_alloc(HELP_PAGE_FILE, &help);
	file_to_string_alloc(INFO_FILE, &info);
	file_to_string_alloc(WIZLIST_FILE, &wizlist);
	file_to_string_alloc(IMMLIST_FILE, &immlist);
	file_to_string_alloc(POLICIES_FILE, &policies);
	file_to_string_alloc(HANDBOOK_FILE, &handbook);
	file_to_string_alloc(BACKGROUND_FILE, &background);
	if (file_to_string_alloc(GREETINGS_FILE, &GREETINGS) == 0)
		prune_crlf(GREETINGS);
	file_to_string_alloc(NEWSPAPER_FILE, &newspaper);
	
	log("Loading spell definitions.");
	mag_assign_spells();
	
	boot_world();
	
	log("Loading help entries.");
	index_boot(DB_BOOT_HLP);
	
	log("Generating player index.");
	build_player_index();
	
	log("Loading fight messages.");
	load_messages();
	
	log("Loading social messages.");
	boot_social_messages();
	
	log("Assigning function pointers:");
	
	if (!no_specials)
	{
		log("   Mobiles.");
		assign_mobiles();
		log("   Shopkeepers.");
		assign_the_shopkeepers();
		log("   Objects.");
		assign_objects();
		log("   Rooms.");
		assign_rooms();
	}
	
	log("Assigning spell and skill levels.");
	init_spell_levels();
	
	log("Sorting command list and spells.");
	sort_commands();
	sort_spells();
	
	log("Booting mail system.");
	if (!scan_file()) {
		log("    Mail boot failed -- Mail system disabled");
		no_mail = 1;
	}
	log("Reading banned site and invalid-name list.");
	load_banned();
	Read_Invalid_List();
	
	/* Economy code */
	log("Initializing Economy module.");
	log("  Loading Markets.");
	BootMarkets();

	log("  Loading Goods.");
	LoadGoodTypes();
	LoadGoods();
	sort_goods_table();

	log("  Loading Trading Posts.");
	LoadTradingPost();
	log("  Done.");

	/* Buildings module */
	log("Loading Buildings.");
	LoadBuildings();

	log("Booting clans.");
	init_clans();

	log("Loading Vehicle Prototypes.");
	LoadVehiclesTable();

	log("Loading Stables.");
	LoadStableRentIndex();
	
	log("Loading Portal Stones.");
	LoadPortalStones();

	/* Moved here so the object limit code works. -gg 6/24/98 */
	if (!mini_mud)
	{
		log("Booting houses.");
		House_boot();
	}
	
	for (i = 0; i <= top_of_zone_table; i++)
	{
		log("Resetting #%d: %s (rooms %d-%d).", zone_table[i].number,
			zone_table[i].name, zone_table[i].bot, zone_table[i].top);
		reset_zone(i);
	}
	
	reset_q.head = reset_q.tail = NULL;
	
	boot_time = time(0);
	
	log("Boot db -- DONE.");
}


/* reset the time in the game from file */
void reset_time(void)
{
	FILE *bgtime;
	time_t beginning_of_time = 0;
	
	if ((bgtime = fopen(TIME_FILE, "r")) == NULL)
		log("SYSERR: Can't read from '%s' time file.", TIME_FILE);
	else
	{
		fscanf(bgtime, "%ld\n", &beginning_of_time);
		fclose(bgtime);
	}
	if (beginning_of_time == 0)
		beginning_of_time = 650336715;
	
	time_info = *mud_time_passed(time(0), beginning_of_time);

	ChangeMoon();
	
	if (time_info.hours == 4)
	{
		if (MoonPhase != MOON_NEW)
			Sunlight = MOON_SET;
		else
			Sunlight = SUN_DARK;
	}
	else if (time_info.hours == 5)		Sunlight = SUN_RISE;
	else if (time_info.hours <= 20)		Sunlight = SUN_LIGHT;
	else if (time_info.hours == 21)		Sunlight = SUN_SET;
	else if (time_info.hours <= 23)		Sunlight = SUN_DARK;
	else if (time_info.hours == 24)
	{
		if (MoonPhase != MOON_NEW)
			Sunlight = MOON_RISE;
	}
	else
	{
		if (MoonPhase != MOON_NEW)
			Sunlight = MOON_LIGHT;
	}
	
	log("   Current Gametime: %dH %dD %dM %dY.", time_info.hours,
		time_info.day, time_info.month, time_info.year);

	WeatherInit();
}


/* Write the time in 'when' to the MUD-time file. */
void save_mud_time(TIME_INFO_DATA *when)
{
	FILE *bgtime;
	
	if ((bgtime = fopen(TIME_FILE, "w")) == NULL)
		log("SYSERR: Can't write to '%s' time file.", TIME_FILE);
	else
	{
		fprintf(bgtime, "%ld\n", mud_time_to_secs(when));
		fclose(bgtime);
	}
}


/*
 * Thanks to Andrey (andrey@alex-ua.com) for this bit of code, although I
 * did add the 'goto' and changed some "while()" into "do { } while()".
 *	-gg 6/24/98 (technically 6/25/98, but I care not.)
 */
int count_alias_records(FILE *fl)
{
	char key[READ_SIZE], next_key[READ_SIZE];
	char line[READ_SIZE], *scan;
	int total_keywords = 0;
	
	/* get the first keyword line */
	get_one_line(fl, key);
	
	while (*key != '$')
	{
		/* skip the text */
		do
		{
			get_one_line(fl, line);
			if (feof(fl))
				goto ackeof;
		} while (*line != '#');
		
		/* now count keywords */
		scan = key;
		do
		{
			scan = one_word(scan, next_key);
			if (*next_key)
				++total_keywords;
		} while (*next_key);
		
		/* get next keyword line (or $) */
		get_one_line(fl, key);
		
		if (feof(fl))
			goto ackeof;
	}
	
	return (total_keywords);
	
	/* No, they are not evil. -gg 6/24/98 */
ackeof:	
	log("SYSERR: Unexpected end of help file.");
	exit(1);	/* Some day we hope to handle these things better... */
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE *fl)
{
	char buf[128];
	int count = 0;
	
	while (fgets(buf, 128, fl))
		if (*buf == '#')
			count++;
		
	return (count);
}



void index_boot(int mode)
{
	FILE *index, *db_file;
	const char *index_filename, *prefix = NULL;	/* NULL or egcs 1.1 complains */
	int rec_count = 0, size[2];
	
	switch (mode)
	{
	case DB_BOOT_WLD:
		prefix = WLD_PREFIX;
		break;
	case DB_BOOT_MOB:
		prefix = MOB_PREFIX;
		break;
	case DB_BOOT_OBJ:
		prefix = OBJ_PREFIX;
		break;
	case DB_BOOT_ZON:
		prefix = ZON_PREFIX;
		break;
	case DB_BOOT_SHP:
		prefix = SHP_PREFIX;
		break;
	case DB_BOOT_HLP:
		prefix = HLP_PREFIX;
		break;
	default:
		log("SYSERR: Unknown subcommand %d to index_boot!", mode);
		exit(1);
	}
	
	if (mini_mud)
		index_filename = MINDEX_FILE;
	else
		index_filename = INDEX_FILE;
	
	sprintf(buf2, "%s%s", prefix, index_filename);
	
	if (!(index = fopen(buf2, "r"))) 
	{
		log("SYSERR: opening index file '%s': %s", buf2, strerror(errno));
		exit(1);
	}
	
	/* first, count the number of records in the file so we can malloc */
	fscanf(index, "%s\n", buf1);
	while (*buf1 != '$')
	{
		sprintf(buf2, "%s%s", prefix, buf1);
		if (!(db_file = fopen(buf2, "r")))
		{
			log("SYSERR: File '%s' listed in '%s/%s': %s", buf2, prefix,
				index_filename, strerror(errno));
			fscanf(index, "%s\n", buf1);
			continue;
		}
		else
		{
			if (mode == DB_BOOT_ZON)
				rec_count++;
			else if (mode == DB_BOOT_HLP)
				rec_count += count_alias_records(db_file);
			else
				rec_count += count_hash_records(db_file);
		}
		
		fclose(db_file);
		fscanf(index, "%s\n", buf1);
	}
	
	/* Exit if 0 records, unless this is shops */
	if (!rec_count)
	{
		if (mode == DB_BOOT_SHP)
			return;
		log("SYSERR: boot error - 0 records counted in %s/%s.", prefix,
			index_filename);
		exit(1);
	}
	
	/*
	 * NOTE: "bytes" does _not_ include strings or other later malloc'd things.
	 */
	switch (mode)
	{
	case DB_BOOT_WLD:
		size[0] = sizeof(ROOM_DATA) * rec_count;
		log("   %d rooms, %d bytes.", rec_count, size[0]);
		break;
	case DB_BOOT_MOB:
		CREATE(mob_proto, CHAR_DATA, rec_count);
		CREATE(mob_index, INDEX_DATA, rec_count);
		size[0] = sizeof(INDEX_DATA) * rec_count;
		size[1] = sizeof(CHAR_DATA) * rec_count;
		log("   %d mobs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
		break;
	case DB_BOOT_OBJ:
		CREATE(obj_proto, OBJ_DATA, rec_count);
		CREATE(obj_index, INDEX_DATA, rec_count);
		size[0] = sizeof(INDEX_DATA) * rec_count;
		size[1] = sizeof(OBJ_DATA) * rec_count;
		log("   %d objs, %d bytes in index, %d bytes in prototypes.", rec_count, size[0], size[1]);
		break;
	case DB_BOOT_ZON:
		CREATE(zone_table, ZONE_DATA, rec_count);
		size[0] = sizeof(ZONE_DATA) * rec_count;
		log("   %d zones, %d bytes.", rec_count, size[0]);
		break;
	case DB_BOOT_HLP:
		CREATE(help_table, HELP_INDEX_ELEM, rec_count);
		size[0] = sizeof(HELP_INDEX_ELEM) * rec_count;
		log("   %d entries, %d bytes.", rec_count, size[0]);
		break;
	}
	
	rewind(index);
	fscanf(index, "%s\n", buf1);
	while (*buf1 != '$')
	{
		sprintf(buf2, "%s%s", prefix, buf1);
		if (!(db_file = fopen(buf2, "r")))
		{
			log("SYSERR: %s: %s", buf2, strerror(errno));
			exit(1);
		}
		switch (mode)
		{
		case DB_BOOT_WLD:
		case DB_BOOT_OBJ:
		case DB_BOOT_MOB:
			discrete_load(db_file, mode, buf2);
			break;
		case DB_BOOT_ZON:
			load_zones(db_file, buf2);
			break;
		case DB_BOOT_HLP:
			/*
			 * If you think about it, we have a race here.  Although, this is the
			 * "point-the-gun-at-your-own-foot" type of race.
			 */
			load_help(db_file);
			break;
		case DB_BOOT_SHP:
			boot_the_shops(db_file, buf2, rec_count);
			break;
		}
		
		fclose(db_file);
		fscanf(index, "%s\n", buf1);
	}
	fclose(index);
	
	/* sort the help index */
	if (mode == DB_BOOT_HLP)
	{
		qsort(help_table, top_of_helpt, sizeof(HELP_INDEX_ELEM), hsort);
		top_of_helpt--;
	}
}


void discrete_load(FILE *fl, int mode, char *filename)
{
	int nr = -1, last;
	char line[READ_SIZE];
	
	const char *modes[] = {"world", "mob", "obj"};
	
	for (;;)
	{
		/*
		 * we have to do special processing with the obj files because they have
		 * no end-of-record marker :(
		 */
		if (mode != DB_BOOT_OBJ || nr < 0)
		{
			if (!get_line(fl, line))
			{
				if (nr == -1)
					log("SYSERR: %s file %s is empty!", modes[mode], filename);
				else
				{
					log("SYSERR: Format error in %s after %s #%d\n"
						"...expecting a new %s, but file ended!\n"
						"(maybe the file is not terminated with '$'?)", filename,
						modes[mode], nr, modes[mode]);
				}
				exit(1);
			}
		}

		if (*line == '$')
			return;
			
		if (*line == '#')
		{
			last = nr;
			if (sscanf(line, "#%d", &nr) != 1)
			{
				log("SYSERR: Format error after %s #%d", modes[mode], last);
				exit(1);
			}

			/* NB: MAX 99999 ZONES!!! */
			if (nr >= 9999999)
				return;
			else
			{
				switch (mode)
				{ 
				case DB_BOOT_WLD:
					parse_room(fl, nr);
					break;
				case DB_BOOT_MOB:
					parse_mobile(fl, nr);
					break;
				case DB_BOOT_OBJ:
					strcpy(line, parse_object(fl, nr));
					break;
				}
			}
		}
		else
		{
			log("SYSERR: Format error in %s file %s near %s #%d", modes[mode],
				filename, modes[mode], nr);
			log("SYSERR: ... offending line: '%s'", line);
			exit(1);
		}
	}
}

bitvector_t asciiflag_conv(char *flag)
{
	bitvector_t flags = 0;
	int is_number = 1;
	char *p;
	
	for (p = flag; *p; p++)
	{
		if (islower(*p))
			flags |= 1 << (*p - 'a');
		else if (isupper(*p))
			flags |= 1 << (26 + (*p - 'A'));
		
		if (!isdigit(*p))
			is_number = 0;
	}
	
	if (is_number)
		flags = atol(flag);
	
	return (flags);
}


ROOM_DATA *new_room( void )
{
	ROOM_DATA *pRoom;

	CREATE( pRoom, ROOM_DATA, 1 );

	pRoom->affections		= NULL;
	pRoom->first_content	= NULL;
	pRoom->last_content		= NULL;
	pRoom->coord			= NULL;
	pRoom->description		= NULL;
	pRoom->ex_description	= NULL;
	pRoom->first_exit		= NULL;
	pRoom->func				= NULL;
	pRoom->last_exit		= NULL;
	pRoom->name				= NULL;
	pRoom->next				= NULL;
	pRoom->people			= NULL;
	pRoom->portal_stone		= NULL;
	pRoom->action			= NULL;
	pRoom->buildings		= NULL;
	pRoom->ships			= NULL;
	pRoom->vehicles			= NULL;
	pRoom->ferryboat		= NULL;
	pRoom->trigger			= NULL;
	pRoom->light			= 0;
	pRoom->room_flags		= 0;
	pRoom->number			= NOWHERE;
	pRoom->sector_type		= NOWHERE;
	pRoom->zone				= NOWHERE;

	return ( pRoom );
}


/* load the rooms */
void parse_room(FILE *fl, int virtual_nr)
{
	ROOM_DATA *pRoom = NULL;
	EXTRA_DESCR *new_descr;
	static int room_nr = 0, zone = 0;
	int t[10], iHash;
	char line[READ_SIZE], flags[128];
	
	sprintf(buf2, "room #%d", virtual_nr);
	
	if (virtual_nr < zone_table[zone].bot)
	{
		log("SYSERR: Room #%d is below zone %d.", virtual_nr, zone);
		exit(1);
	}
	
	while (virtual_nr > zone_table[zone].top)
	{
		if (++zone > top_of_zone_table)
		{
			log("SYSERR: Room %d is outside of any zone.", virtual_nr);
			exit(1);
		}
	}

	if ( get_room( virtual_nr ) != NULL )
	{
		log( "Load_rooms: vnum %d duplicated.", virtual_nr );
		exit( 1 );
	}

	pRoom = new_room();

	pRoom->zone			= zone;
	pRoom->number		= virtual_nr;
	pRoom->name			= fread_string(fl, buf2);
	pRoom->description	= fread_string(fl, buf2);
	
	if (!get_line(fl, line))
	{
		log("SYSERR: Expecting roomflags/sector type of room #%d but file ended!",
			virtual_nr);
		exit(1);
	}
	
	if (sscanf(line, " %d %s %d ", t, flags, t + 2) != 3)
	{
		log("SYSERR: Format error in roomflags/sector type of room #%d",
			virtual_nr);
		exit(1);
	}

	/* t[0] is the zone number; ignored with the zone-file system */
	pRoom->room_flags	= asciiflag_conv(flags);
	pRoom->sector_type	= t[2];
	
	sprintf(buf,"SYSERR: Format error in room #%d (expecting D/E/S)",virtual_nr);
	
	for (;;) 
	{
		if (!get_line(fl, line))
		{
			log("%s", buf);
			exit(1);
		}

		if ( *line == 'S' )
			break;

		switch (*line)
		{
		case 'D':
			setup_dir(fl, pRoom, atoi(line + 1));
			break;

		/* exit to the wilderness -- setup all rooms around */
		case 'W':
			setup_wild_exit(fl, pRoom, atoi(line + 1));
			break;

		/* one way exit to the wilderness -- no entrance from the wilderness */
		case 'X':
			setup_zone_exit(fl, pRoom, atoi(line + 1));
			break;

		case 'E':
			CREATE(new_descr, EXTRA_DESCR, 1);
			new_descr->keyword		= fread_string(fl, buf2);
			new_descr->description	= fread_string(fl, buf2);
			
			new_descr->next			= pRoom->ex_description;
			pRoom->ex_description	= new_descr;
			break;

		// Room Trigger
		case 'T':
			setup_trigger( pRoom, fl );
			break;

		default:
			log("%s", buf);
			exit(1);
		}
	}

	iHash			= virtual_nr % ROOM_HASH;
	pRoom->next		= World[iHash];
	World[iHash]	= pRoom;
	top_of_world++;
}



/* read direction data */
void setup_dir(FILE *fl, ROOM_DATA *pRoom, int dir)
{
	EXIT_DATA *pexit;
	int t[5];
	char line[READ_SIZE], dflags[128];
	
	if ( dir < 0 || dir > NUM_OF_DIRS )
	{
		log( "SYSERR: setup_dir(): vnum %d has bad door number %d.", pRoom->number, dir );
		exit( 1 );
	}

	sprintf(buf2, "room #%d, direction D%d", pRoom->number, dir);
	
	pexit = make_exit(pRoom, NULL, dir);

	pexit->vdir			= dir;
	pexit->description	= fread_string(fl, buf2);
	pexit->keyword		= fread_string(fl, buf2);
	
	if (!get_line(fl, line))
	{
		log("SYSERR: Format error, %s", buf2);
		exit(1);
	}
	if (sscanf(line, " %s %d %d ", dflags, t + 1, t + 2) != 3)
	{
		log("SYSERR: Format error, %s", buf2);
		exit(1);
	}

	pexit->exit_info	= asciiflag_conv(dflags);

	/*
	if (t[0] == 1)
		pexit->exit_info = EX_ISDOOR;
	else if (t[0] == 2)
		pexit->exit_info = EX_ISDOOR | EX_PICKPROOF;
	else
		pexit->exit_info = 0;
	*/

	/* sanity check */
	if ( pexit->exit_info && !EXIT_FLAGGED( pexit, EX_ISDOOR ) )
		SET_BIT( pexit->exit_info, EX_ISDOOR );
	
	pexit->key			= t[1];
	pexit->vnum			= t[2];
}


/* controlla che l'uscita sia piazzata sul bordo della zona.. */
int check_wild_zone_border(zone_rnum zone, int y, int x )
{
	if ( zone_table[zone].wild.z_start.y == 0 && zone_table[zone].wild.z_start.x == 0 &&
	     zone_table[zone].wild.z_end.y == 0 && zone_table[zone].wild.z_end.x == 0 )
		return (TRUE);

	if (y != zone_table[zone].wild.z_start.y && y != zone_table[zone].wild.z_end.y &&
	    x != zone_table[zone].wild.z_start.x && x != zone_table[zone].wild.z_end.x)
		return (FALSE);

	return (TRUE);
}

void setup_wild_exit(FILE *fl, ROOM_DATA *pRoom, int door)
{
	ROOM_DATA *wRoom;
	EXIT_DATA *pexit;
	char flags[128];
	char *ln;
	int x1, x2, x3;
	
	if ( door < 0 || door > NUM_OF_DIRS )
	{
		log( "SYSERR: setup_wild_exit(): vnum %d has bad door number %d.", pRoom->number, door );
		exit( 1 );
	}

	pexit = make_exit( pRoom, NULL, door );

	pexit->description	= fread_string( fl, buf2 );
	pexit->keyword		= fread_string( fl, buf2 );
	pexit->exit_info	= 0;
	
	ln = fread_line( fl );
	x1=x2=x3=0;
	sscanf( ln, "%s %d %d %d ", &flags, &x1, &x2, &x3 );
	
	if ( !check_coord( x2, x3 ) )
	{
		log( "SYSERR: setup_wild_exit(): invalid coordinates in wild exit for room %d", pRoom->number);
		exit(1);
	}
	
	if ( !check_wild_zone_border( pRoom->zone, x2, x3 ) )
	{
		log("SYSERR: setup_wild_exit() - exit to wild from room %d is not placed in zone %d border", pRoom->number, zone_table[pRoom->zone].number );
		exit(1);
	}
	
	pexit->key			= x1;
	CREATE( pexit->coord, COORD_DATA, 1 );
	pexit->coord->y		= x2;
	pexit->coord->x		= x3;
	pexit->vnum			= NOWHERE;
	pexit->vdir			= door;
	pexit->exit_info	= asciiflag_conv( flags );
	
	/* sanity check */
	if ( pexit->exit_info && !EXIT_FLAGGED( pexit, EX_ISDOOR ) )
		SET_BIT( pexit->exit_info, EX_ISDOOR );
	
	/* carichiamo il settore wild */
	load_wildmap_sector( pexit->coord );
	
	/* controlliamo che l'uscita non dia su un settore non valido.. */
	if ( !check_next_step( pexit->coord, door ) )
	{
		log( "SYSERR: setup_wild_exit(): cannot place wild exit from room %d", pRoom->number);
		exit(1);
	}
	
	/* creiamo la stanza wild che contiene l'uscita */
	wRoom = create_wild_room( pexit->coord, TRUE );
	if ( SECT(wRoom) != SECT_WILD_EXIT )
	{
		put_sect( wRoom->coord->y, wRoom->coord->x, SECT_WILD_EXIT, TRUE );
		SECT(wRoom)	= SECT_WILD_EXIT;
	}
	setup_wild_static( wRoom, pRoom );
	pexit->to_room		= find_wild_room( wRoom, door, FALSE );
	pexit->coord->y		= pexit->to_room->coord->y;
	pexit->coord->x		= pexit->to_room->coord->x;
}

/* create an one-way exit from a zone to the wilderness */
void setup_zone_exit(FILE *fl, ROOM_DATA *pRoom, int door)
{
	ROOM_DATA *wRoom;
	EXIT_DATA *pexit;
	COORD_DATA *rdest;
	char flags[128];
	char *ln;
	int x1, x2, x3;
	
	if ( door < 0 || door > NUM_OF_DIRS )
	{
		log( "SYSERR: setup_zone_exit(): vnum %d has bad door number %d.", pRoom->number, door );
		exit( 1 );
	}

	pexit = make_exit( pRoom, NULL, door );

	pexit->description	= fread_string( fl, buf2 );
	pexit->keyword		= fread_string( fl, buf2 );
	pexit->exit_info	= 0;
	
	ln = fread_line( fl );
	x1=x2=x3=0;
	sscanf( ln, "%s %d %d %d ", &flags, &x1, &x2, &x3 );
	
	if ( !check_coord( x2, x3 ) )
	{
		log( "SYSERR: setup_zone_exit(): invalid coordinates in wild exit for room %d", pRoom->number);
		exit(1);
	}
	
	pexit->key			= x1;
	CREATE( pexit->coord, COORD_DATA, 1 );
	pexit->coord->y		= x2;
	pexit->coord->x		= x3;
	pexit->vnum			= NOWHERE;
	pexit->vdir			= door;
	pexit->exit_info	= asciiflag_conv( flags );
	
	/* sanity check */
	if ( pexit->exit_info && !EXIT_FLAGGED( pexit, EX_ISDOOR ) )
		SET_BIT( pexit->exit_info, EX_ISDOOR );
	
	/* carichiamo il settore wild */
	load_wildmap_sector( pexit->coord );
	
	/* controlliamo che l'uscita non dia su un settore non valido.. */
	if (!(rdest = check_next_step(pexit->coord, door)))
	{
		log( "SYSERR: setup_zone_exit(): cannot place wild exit from room %d", pRoom->number);
		exit(1);
	}
	
	/* creiamo la stanza wild che contiene l'uscita */
	wRoom = create_wild_room( pexit->coord, TRUE );
	/*
	if ( SECT(wRoom) != SECT_WILD_EXIT )
	{
		put_sect( wRoom->coord->y, wRoom->coord->x, SECT_WILD_EXIT, TRUE );
		SECT(wRoom)	= SECT_WILD_EXIT;
	}
	*/

	//pexit->to_room		= find_wild_room( wRoom, door, FALSE );
	pexit->to_room		= create_wild_room(rdest, TRUE);
	pexit->coord->y		= pexit->to_room->coord->y;
	pexit->coord->x		= pexit->to_room->coord->x;
}

/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
	if (!(r_mortal_start_room = get_room(mortal_start_room)))
	{
		log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
		exit(1);
	}
	if (!(r_immort_start_room = get_room(immort_start_room)))
	{
		if (!mini_mud)
			log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
		r_immort_start_room = r_mortal_start_room;
	}
	if (!(r_frozen_start_room = get_room(frozen_start_room)))
	{
		if (!mini_mud)
			log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
		r_frozen_start_room = r_mortal_start_room;
	}
}


/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void renum_world( void )
{
	ROOM_DATA *pRoom;
	EXIT_DATA *pexit, *pexit_next, *rev_exit;
	int iHash;
	
	for ( iHash = 0; iHash < ROOM_HASH; iHash++ )
	{
		for ( pRoom = World[iHash]; pRoom; pRoom = pRoom->next )
		{
			bool fexit;
			
			fexit = FALSE;
			for ( pexit = pRoom->first_exit; pexit; pexit = pexit_next )
			{
				pexit_next	= pexit->next;
				pexit->rvnum	= pRoom->number;

				if ( EXIT_FLAGGED( pexit, EX_FALSE ) )
					continue;

				if (( pexit->vnum <= 0 ||
				    ( pexit->to_room = get_room( pexit->vnum ) ) == NULL ) &&
				    pexit->coord == NULL )
				{
					if ( pexit->vnum == NOWHERE )
						SET_BIT( pexit->exit_info, EX_FALSE );
					else
					{
						log( "Fix_exits: room %d, exit %s leads to bad vnum (%d)",
							pRoom->number, dirs[pexit->vdir], pexit->vnum );
					
						log( "Deleting %s exit in room %d", dirs[pexit->vdir],
							pRoom->number );
						extract_exit( pRoom, pexit );
					}
				}
				else
					fexit = TRUE;
			}
			if ( !fexit )
				SET_BIT( pRoom->room_flags, ROOM_NOMOB );
		}
	}
	
	/* Set all the rexit pointers 	-Thoric */
	for ( iHash = 0; iHash < ROOM_HASH; iHash++ )
	{
		for ( pRoom = World[iHash]; pRoom; pRoom = pRoom->next )
		{
			for ( pexit = pRoom->first_exit; pexit; pexit = pexit->next )
			{
				if ( EXIT_FLAGGED( pexit, EX_FALSE ) )
					continue;

				if ( pexit->to_room && !pexit->rexit )
				{
					rev_exit = get_exit_to( pexit->to_room, rev_dir[pexit->vdir], pRoom->number );
					if ( rev_exit )
					{
						pexit->rexit	= rev_exit;
						rev_exit->rexit	= pexit;
					}
				}
			}
		}
	}
	
	return;
}


#define ZCMD	zone_table[zone].cmd[cmd_no]
#define ZWILD	zone_table[zone].wild

/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void)
{
	int cmd_no;
	room_vnum a, b, c, olda, oldb, oldc;
	zone_rnum zone;
	char buf[128];
	
	for (zone = 0; zone <= top_of_zone_table; zone++)
	{
		/*
		 * gestione della visualizzazione delle zone nella wild...
		 *
		 * le zone sono rappresentate come black box, mentre i confini sono
		 * rappresentati da I bianche brillanti
		 */
		if (ZWILD.z_start.y == 0 && ZWILD.z_start.x == 0 && ZWILD.z_end.y == 0 && ZWILD.z_end.x == 0)
		{
			if (ZWILD.dist != -1)
				log("WILD: zone [%d] '%s' has no place in wild map.", zone_table[zone].number,
					zone_table[zone].name);
		}
		else if (ZWILD.z_start.y != ZWILD.z_end.y || ZWILD.z_start.x != ZWILD.z_end.x)
		{
			ROOM_DATA *pRoom;
			COORD_DATA coord;
			
			if (!check_coord(ZWILD.z_start.y, ZWILD.z_start.x))
			{
				log("SYSERR: invalid wild start coordinates for zone %d", zone_table[zone].number);
				exit(1);
			}

			if (!check_coord(ZWILD.z_end.y, ZWILD.z_end.x))
			{
				log("SYSERR: invalid wild end coordinates for zone %d", zone_table[zone].number);
				exit(1);
			}

			for ( coord.y = ZWILD.z_start.y; coord.y <= ZWILD.z_end.y; coord.y++ )
			{
				for ( coord.x = ZWILD.z_start.x; coord.x <= ZWILD.z_end.x; coord.x++ )
				{
					pRoom = get_wild_room( &coord );

					// saltiamo le uscite...
					if ( get_sect( &coord ) == SECT_WILD_EXIT )
					{
						if ( pRoom && SECT(pRoom) != SECT_WILD_EXIT )
							SECT(pRoom) = SECT_WILD_EXIT;
						continue;
					}

					// primo caso: le righe superiori ed inferiori..
					if (coord.y == ZWILD.z_start.y || coord.y == ZWILD.z_end.y ||
					    coord.x == ZWILD.z_start.x || coord.x == ZWILD.z_end.x)
					{
						if ( get_sect( &coord ) != SECT_ZONE_BORDER )
							put_sect( coord.y, coord.x, SECT_ZONE_BORDER, TRUE );

						if ( pRoom && SECT(pRoom) != SECT_ZONE_BORDER )
							SECT(pRoom) = SECT_ZONE_BORDER;
					}
					// ultimo caso: l'interno
					else
					{
						if ( get_sect( &coord ) != SECT_ZONE_INSIDE )
							put_sect( coord.y, coord.x, SECT_ZONE_INSIDE, TRUE );

						if ( pRoom && SECT(pRoom) != SECT_ZONE_INSIDE )
							SECT(pRoom) = SECT_ZONE_INSIDE;
					}
				}
			}
		}
		/* fine gestione della visualizzazione delle zone nella wild...*/

		for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
		{
			a = b = c = 0;
			olda = ZCMD.arg1;
			oldb = ZCMD.arg2;
			oldc = ZCMD.arg3;
			switch (ZCMD.command)
			{
			case 'M':
				a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
				if (ZCMD.arg3 != NOWHERE)
					c = ZCMD.arg3;
				break;
			case 'O':
				a = ZCMD.arg1 = real_object(ZCMD.arg1);
				if (ZCMD.arg3 != NOWHERE)
					c = ZCMD.arg3;
				break;
			case 'G':
				a = ZCMD.arg1 = real_object(ZCMD.arg1);
				break;
			case 'E':
				a = ZCMD.arg1 = real_object(ZCMD.arg1);
				break;
			case 'P':
				a = ZCMD.arg1 = real_object(ZCMD.arg1);
				c = ZCMD.arg3 = real_object(ZCMD.arg3);
				break;
			case 'D':
				if (ZCMD.arg1 != NOWHERE)
					a = ZCMD.arg1;
				break;
			case 'R': /* rem obj from room */
				if (ZCMD.arg1 != NOWHERE)
					a = ZCMD.arg1;
				b = ZCMD.arg2 = real_object(ZCMD.arg2);
				break;
			case 'H':
				if (ZCMD.arg3 != NOWHERE)
					a = ZCMD.arg3;
				break;
			}
			if (a == NOWHERE || b == NOWHERE || c == NOWHERE)
			{
				if (!mini_mud)
				{
					sprintf(buf,  "Invalid vnum %d, cmd disabled",
					a == NOWHERE ? olda : b == NOWHERE ? oldb : oldc);
					log_zone_error(zone, cmd_no, buf);
				}
				ZCMD.command = '*';
			}
		}
	}
}



void parse_simple_mob(FILE *mob_f, int i, int nr)
{
	int j, t[10];
	char line[READ_SIZE];
	
	mob_proto[i].real_abils.str		= 11;
	mob_proto[i].real_abils.intel	= 11;
	mob_proto[i].real_abils.wis		= 11;
	mob_proto[i].real_abils.dex		= 11;
	mob_proto[i].real_abils.con		= 11;
	mob_proto[i].real_abils.cha		= 11;
	
	mob_proto[i].mob_specials.maxfactor	= 25;
	
	if (!get_line(mob_f, line))
	{
		log("SYSERR: Format error in mob #%d, file ended after S flag!", nr);
		exit(1);
	}
	
	if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ",
		t, t+1, t+2, t+3, t+4, t+5, t+6, t+7, t+8) != 9)
	{
		log("SYSERR: Format error in mob #%d, first line after S flag\n"
			"...expecting line of form '# # # #d#+# #d#+#'", nr);
		exit(1);
	}
	
	GET_LEVEL(mob_proto + i)		= t[0];
	GET_HITROLL(mob_proto + i)		= 20 - t[1];
	GET_AC(mob_proto + i)			= 10 * t[2];
	
	/* max hit = 0 is a flag that H, M, V is xdy+z */
	GET_MAX_HIT(mob_proto + i)		= 0;
	GET_MAX_MANA(mob_proto + i)		= 10;
	GET_MAX_MOVE(mob_proto + i)		= 50;

	GET_HIT(mob_proto + i)			= t[3];
	GET_MANA(mob_proto + i)			= t[4];
	GET_MOVE(mob_proto + i)			= t[5];
		
	GET_NDD(mob_proto + i)			= t[6];
	GET_SDD(mob_proto + i)			= t[7];
	GET_DAMROLL(mob_proto + i)		= t[8];
	
	if (!get_line(mob_f, line))
	{
		log("SYSERR: Format error in mob #%d, second line after S flag\n"
			"...expecting line of form '# #', but file ended!", nr);
		exit(1);
	}
	
	if (sscanf(line, " %d %d ", t, t + 1) != 2)
	{
		log("SYSERR: Format error in mob #%d, second line after S flag\n"
			"...expecting line of form '# #'", nr);
		exit(1);
	}
	
	GET_GOLD(mob_proto + i)			= t[0];
	GET_EXP(mob_proto + i)			= t[1];
	
	if (!get_line(mob_f, line))
	{
		log("SYSERR: Format error in last line of mob #%d\n"
			"...expecting line of form '# # #', but file ended!", nr);
		exit(1);
	}
	
	if (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) != 3)
	{
		log("SYSERR: Format error in last line of mob #%d\n"
			"...expecting line of form '# # #'", nr);
		exit(1);
	}
	
	GET_POS(mob_proto + i)			= t[0];
	GET_DEFAULT_POS(mob_proto + i)	= t[1];
	GET_SEX(mob_proto + i)			= t[2];
	
	GET_CLASS(mob_proto + i)		= 0;
	GET_RACE(mob_proto + i)			= 0;
	GET_WEIGHT(mob_proto + i)		= 200;
	GET_HEIGHT(mob_proto + i)		= 198;
	
	/*
	 * these are now save applies; base save numbers for MOBs are now from
	 * the warrior save table.
	 */
	for (j = 0; j < 5; j++)
		GET_SAVE(mob_proto + i, j)	= 0;
}


/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 *
 * CASE		: Requires a parameter through 'value'.
 * BOOL_CASE	: Being specified at all is its value.
 */

#define CASE(test)	\
	if (value && !matched && !str_cmp(keyword, test) && (matched = TRUE))

#define BOOL_CASE(test)	\
	if (!value && !matched && !str_cmp(keyword, test) && (matched = TRUE))

#define RANGE(low, high)	\
	(num_arg = MAX((low), MIN((high), (num_arg))))

void interpret_espec(const char *keyword, const char *value, int i, int nr)
{
	int num_arg = 0, matched = FALSE;
	
	/*
	* If there isn't a colon, there is no value.  While Boolean options are
	* possible, we don't actually have any.  Feel free to make some.
	*/
	if (value)
		num_arg = atoi(value);
	
	CASE("BareHandAttack")
	{
		RANGE(0, 99);
		GET_ATTACK(mob_proto + i) = num_arg;
	}
	
	CASE("Str")
	{
		RANGE(3, 25);
		mob_proto[i].real_abils.str = num_arg;
	}
	
	CASE("Int")
	{
		RANGE(3, 25);
		mob_proto[i].real_abils.intel = num_arg;
	}
	
	CASE("Wis")
	{
		RANGE(3, 25);
		mob_proto[i].real_abils.wis = num_arg;
	}
	
	CASE("Dex")
	{
		RANGE(3, 25);
		mob_proto[i].real_abils.dex = num_arg;
	}
	
	CASE("Con")
	{
		RANGE(3, 25);
		mob_proto[i].real_abils.con = num_arg;
	}
	
	CASE("Cha")
	{
		RANGE(3, 25);
		mob_proto[i].real_abils.cha = num_arg;
	}

	CASE("MaxFactor")
	{
		RANGE(1, 255);
		mob_proto[i].mob_specials.maxfactor = num_arg;
	}

	CASE("NpcClass")
	{
	}

	if (!matched)
	{
		log("SYSERR: Warning: unrecognized espec keyword %s in mob #%d",
			keyword, nr);
	}    
}

#undef CASE
#undef BOOL_CASE
#undef RANGE

void parse_espec(char *buf, int i, int nr)
{
	char *ptr;
	
	if ((ptr = strchr(buf, ':')) != NULL)
	{
		*(ptr++) = '\0';
		while (isspace(*ptr))
			ptr++;
	}
	interpret_espec(buf, ptr, i, nr);
}


void parse_enhanced_mob(FILE *mob_f, int i, int nr)
{
	char line[READ_SIZE];
	
	parse_simple_mob(mob_f, i, nr);
	
	while (get_line(mob_f, line))
	{
		if (!strcmp(line, "E"))	/* end of the enhanced section */
			return;
		else if (*line == '#')	/* we've hit the next mob, maybe? */
		{
			log("SYSERR: Unterminated E section in mob #%d", nr);
			exit(1);
		}
		else
			parse_espec(line, i, nr);
	}
	
	log("SYSERR: Unexpected end of file reached after mob #%d", nr);
	exit(1);
}


void parse_mobile(FILE *mob_f, int nr)
{
	static int i = 0;
	int j, t[10];
	char line[READ_SIZE], *tmpptr, letter;
	char f1[128], f2[128];
	
	mob_index[i].vnum		= nr;
	mob_index[i].number		= 0;
	mob_index[i].func		= NULL;
	
	clear_char(mob_proto + i);
	
	/*
	 * Mobiles should NEVER use anything in the 'player_specials' structure.
	 * The only reason we have every mob in the game share this copy of the
	 * structure is to save newbie coders from themselves. -gg 2/25/98
	 */
	mob_proto[i].player_specials = &dummy_mob;
	sprintf(buf2, "mob vnum %d", nr);
	
	/***** String data *****/
	GET_PC_NAME(mob_proto + i)		= fread_string(mob_f, buf2);
	tmpptr = GET_SDESC(mob_proto + i) = fread_string(mob_f, buf2);
	if (tmpptr && *tmpptr)
	{
		if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
			!str_cmp(fname(tmpptr), "the"))
			*tmpptr = LOWER(*tmpptr);
	}
	GET_LDESC(mob_proto + i)		= fread_string(mob_f, buf2);
	GET_DDESC(mob_proto + i)		= fread_string(mob_f, buf2);
	GET_TITLE(mob_proto + i)		= NULL;
	
	/* *** Numeric data *** */
	if (!get_line(mob_f, line))
	{
		log("SYSERR: Format error after string section of mob #%d\n"
			"...expecting line of form '# # # {S | E}', but file ended!", nr);
		exit(1);
	}
	
	if (sscanf(line, "%s %s %d %c", f1, f2, t + 2, &letter) != 4)
	{
		log("SYSERR: Format error after string section of mob #%d\n"
			"...expecting line of form '# # # {S | E}'", nr);
		exit(1);
	}

	MOB_FLAGS(mob_proto + i)		= asciiflag_conv(f1);
	SET_BIT(MOB_FLAGS(mob_proto + i), MOB_ISNPC);
	AFF_FLAGS(mob_proto + i)		= asciiflag_conv(f2);
	GET_ALIGNMENT(mob_proto + i)	= t[2];
	
	/* Rather bad to load mobiles with this bit already set. */
	if (MOB_FLAGGED(mob_proto + i, MOB_NOTDEADYET))
	{
		log("SYSERR: Mob #%d has reserved bit MOB_NOTDEADYET set.", nr);
		REMOVE_BIT(MOB_FLAGS(mob_proto + i), MOB_NOTDEADYET);
	}
	
	/* AGGR_TO_ALIGN is ignored if the mob is AGGRESSIVE. */
	if (MOB_FLAGGED(mob_proto + i, MOB_AGGRESSIVE) && MOB_FLAGGED(mob_proto + i, MOB_AGGR_GOOD | MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL))
		log("SYSERR: Mob #%d both Aggressive and Aggressive_to_Alignment.", nr);
	
	switch (UPPER(letter))
	{
	case 'S':	/* Simple monsters */
		parse_simple_mob(mob_f, i, nr);
		break;
	case 'E':	/* Circle3 Enhanced monsters */
		parse_enhanced_mob(mob_f, i, nr);
		break;
		/* add new mob types here.. */
	default:
		log("SYSERR: Unsupported mob type '%c' in mob #%d", letter, nr);
		exit(1);
	}
	
	mob_proto[i].aff_abils			= mob_proto[i].real_abils;
	
	for (j = 0; j < NUM_WEARS; j++)
		mob_proto[i].equipment[j]	= NULL;
	
	mob_proto[i].nr					= i;
	mob_proto[i].desc				= NULL;
	
	top_of_mobt = i++;
}


/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE *obj_f, int nr)
{
	EXTRA_DESCR *new_descr;
	static int i = 0;
	static char line[READ_SIZE];
	int t[10], j, retval;
	char *tmpptr;
	char f1[READ_SIZE], f2[READ_SIZE], f3[READ_SIZE], f4[READ_SIZE];
	
	obj_index[i].vnum				= nr;
	obj_index[i].number				= 0;
	obj_index[i].func				= NULL;
	
	clear_object(obj_proto + i);
	obj_proto[i].item_number		= i;
	
	sprintf(buf2, "object #%d", nr);
	
	/* *** string data *** */
	if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL)
	{
		log("SYSERR: Null obj name or format error at or near %s", buf2);
		exit(1);
	}

	tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);

	if (tmpptr && *tmpptr)
	{
		if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") ||
		    !str_cmp(fname(tmpptr), "the"))
			*tmpptr = LOWER(*tmpptr);
	}
	
	tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
	if (tmpptr && *tmpptr)
		CAP(tmpptr);
	obj_proto[i].action_description = fread_string(obj_f, buf2);
	
	/* *** numeric data *** */
	if (!get_line(obj_f, line))
	{
		log("SYSERR: Expecting first numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, " %d %s %s %s %s", t, f1, f2, f3, f4)) != 5)
	{
		log("SYSERR: Format error in first numeric line (expecting 5 args, got %d), %s", retval, buf2);
		exit(1);
	}

	GET_OBJ_TYPE(obj_proto + i)		= t[0];
	GET_OBJ_EXTRA(obj_proto + i)	= asciiflag_conv(f1);
	GET_OBJ_WEAR(obj_proto + i)		= asciiflag_conv(f2);
	GET_OBJ_PERM(obj_proto + i)		= asciiflag_conv(f3);
	GET_OBJ_ANTI(obj_proto + i)		= asciiflag_conv(f4);
	
	if (!get_line(obj_f, line))
	{
		log("SYSERR: Expecting second numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, "%d %d %d %d", t, t + 1, t + 2, t + 3)) != 4)
	{
		log("SYSERR: Format error in second numeric line (expecting 4 args, got %d), %s", retval, buf2);
		exit(1);
	}

	GET_OBJ_VAL(obj_proto + i, 0)	= t[0];
	GET_OBJ_VAL(obj_proto + i, 1)	= t[1];
	GET_OBJ_VAL(obj_proto + i, 2)	= t[2];
	GET_OBJ_VAL(obj_proto + i, 3)	= t[3];
	
	if (!get_line(obj_f, line))
	{
		log("SYSERR: Expecting third numeric line of %s, but file ended!", buf2);
		exit(1);
	}
	if ((retval = sscanf(line, "%d %d %d %d %d %d", t, t+1, t+2, t+3, t+4, t+5)) != 6)
	{
		log("SYSERR: Format error in third numeric line (expecting 6 args, got %d), %s", retval, buf2);
		exit(1);
	}
	
	GET_OBJ_WEIGHT(obj_proto + i)	= t[0];
	GET_OBJ_COST(obj_proto + i)		= t[1];
	GET_OBJ_RENT(obj_proto + i)		= t[2];
	GET_OBJ_LEVEL(obj_proto + i)	= t[3];
	GET_OBJ_COND(obj_proto + i)		= t[4];
	GET_OBJ_MAXCOND(obj_proto + i)	= t[5];

	/* check to make sure that weight of containers exceeds curr. quantity */
	if (GET_OBJ_TYPE(obj_proto + i) == ITEM_DRINKCON || GET_OBJ_TYPE(obj_proto + i) == ITEM_FOUNTAIN)
	{
		if (GET_OBJ_WEIGHT(obj_proto + i) < GET_OBJ_VAL(obj_proto + i, 1))
			GET_OBJ_WEIGHT(obj_proto + i) = GET_OBJ_VAL(obj_proto + i, 1) + 5;
	}
	
	/* *** extra descriptions and affect fields *** */
	
	for (j = 0; j < MAX_OBJ_AFF; j++)
	{
		obj_proto[i].affected[j].location = APPLY_NONE;
		obj_proto[i].affected[j].modifier = 0;
	}
	
	strcat(buf2, ", after numeric constants\n"
		"...expecting 'E', 'A', '$', or next object number");
	j = 0;
	
	for (;;)
	{
		if (!get_line(obj_f, line))
		{
			log("SYSERR: Format error in %s", buf2);
			exit(1);
		}
		switch (*line)
		{
		case 'E':
			CREATE(new_descr, EXTRA_DESCR, 1);
			new_descr->keyword			= fread_string(obj_f, buf2);
			new_descr->description		= fread_string(obj_f, buf2);

			new_descr->next				= obj_proto[i].ex_description;
			obj_proto[i].ex_description = new_descr;
			break;
		case 'A':
			if (j >= MAX_OBJ_AFF)
			{
				log("SYSERR: Too many A fields (%d max), %s", MAX_OBJ_AFF, buf2);
				exit(1);
			}
			if (!get_line(obj_f, line))
			{
				log("SYSERR: Format error in 'A' field, %s\n"
					"...expecting 2 numeric constants but file ended!", buf2);
				exit(1);
			}
			
			if ((retval = sscanf(line, " %d %d ", t, t + 1)) != 2)
			{
				log("SYSERR: Format error in 'A' field, %s\n"
					"...expecting 2 numeric arguments, got %d\n"
					"...offending line: '%s'", buf2, retval, line);
				exit(1);
			}
			
			obj_proto[i].affected[j].location = t[0];
			obj_proto[i].affected[j].modifier = t[1];
			
			j++;
			break;

		case 'S':  /* Weapon Spells */
		{
			OBJ_SPELLS_DATA *ospell, *spells_list = NULL;

			if ( obj_proto[i].special )
			{
				if ( !OBJ_FLAGGED((obj_proto + i), ITEM_HAS_SPELLS) )
				{
					log("SYSERR: Object %d already has special data attached.", nr);
					exit(1);
				}
				spells_list = (OBJ_SPELLS_DATA *) obj_proto[i].special;
			}

			if (!get_line(obj_f,line))
			{
				log("SYSERR: Format error in 'S' field, %s.  Expecting numeric constants, but file ended!",buf2);
				exit(1);
			}

			if ( (retval = sscanf(line, " %d %d %d ", t, t+1, t+2)) != 3 )
			{
				log("SYSERR: Format error in 'S' field, %s  expecting 3 numeric args, got %d.  line: '%s'", buf2, retval, line);
				exit(1);
			}

			CREATE(ospell, OBJ_SPELLS_DATA, 1);
			ospell->spellnum	= t[0];
			ospell->level		= t[1];
			ospell->percent		= t[2];

			ospell->next		= spells_list;
			spells_list			= ospell;

			obj_proto[i].special= spells_list;
			SET_BIT(GET_OBJ_EXTRA(obj_proto + i), ITEM_HAS_SPELLS);

			break;
		}

		/*
		 * Trap set on object
		 *
		 * Format:
		 *
		 * T
		 * <action> <dam type> <charges> <whole_room>
		 *
		 * action     : the trigger ( see structs.h )
		 * dam type   : type of damage (name, not number)
		 * charges    : how many charges the trap has
		 * whole_room : 1 (TRUE) affect everybody in the room,
		 *              0 (FALSE) affect only the char that activate the trap
		 *
		 * Example:
		 *
		 * T
		 * df fire 2 1
		 *
		 * this is a trap that:
		 * - is triggered by GET or DROP
		 * - damage type is FIRE
		 * - has 2 charges
		 * - affects everybody in the room
		 */
		case 'T':
		{
			OBJ_TRAP_DATA *trap, *traps_list = NULL;

			if ( obj_proto[i].special )
			{
				if ( !OBJ_FLAGGED((obj_proto + i), ITEM_HAS_TRAPS) )
				{
					log("SYSERR: Object %d already has special data attached.", nr);
					exit(1);
				}
				traps_list = (OBJ_TRAP_DATA *) obj_proto[i].special;
			}

			if (!get_line(obj_f, line))
			{
				log("SYSERR: Format error in 'T' field, %s\n...expecting 3 constants but file ended!", buf2);
				exit(1);
			}	
			if ((retval = sscanf(line, "%s %s %d %d", f1, f2, t, t+1)) != 4)
			{
				log("SYSERR: Format error in 'T' field, %s\n...expecting 4 args, got %d", buf2, retval);
				exit(1);
			}
			
			CREATE(trap, OBJ_TRAP_DATA, 1);
			trap->action		= asciiflag_conv(f1);
			trap->dam_type		= parse_trap_dam(f2);
			trap->charges		= URANGE(1, t[0], 10);
			trap->whole_room	= ( t[1] == 0 ? FALSE : TRUE );

			trap->next			= traps_list;
			traps_list			= trap;

			obj_proto[i].special= traps_list;
			SET_BIT(GET_OBJ_EXTRA(obj_proto + i), ITEM_HAS_TRAPS);

			break;
		}

		case '$':
		case '#':
			check_object(obj_proto + i);
			top_of_objt = i++;
			return (line);
		default:
			log("SYSERR: Format error in (%c): %s", *line, buf2);
			exit(1);
		}
	}
}


#define Z		zone_table[zone]
#define ZONE_WORDS	"MOGEPDRHIJZ"
#define ZONE_SKIPLINE	5

/* load the zone table and command tables */
void load_zones(FILE *fl, char *zonename)
{
	static zone_rnum zone = 0;
	int cmd_no, num_of_cmds = 0, line_num = 0, tmp, error;
	char *ptr, buf[READ_SIZE], zname[READ_SIZE];
	
	strcpy(zname, zonename);
	
	/* Skip first ZONE_SKIPLINE lines lest we mistake the zone name for a command. */
	for (tmp = 0; tmp < ZONE_SKIPLINE; tmp++)
		get_line(fl, buf);

	/*  More accurate count. Previous was always 4 or 5 too high. -gg 2001/1/17
	 *  Note that if a new zone command is added to reset_zone(), this string
	 *  will need to be updated to suit. - ae.
	 */
	while (get_line(fl, buf))
	{
		if ((strchr(ZONE_WORDS, buf[0]) && buf[1] == ' ') || (buf[0] == 'S' && buf[1] == '\0'))
			num_of_cmds++;
	}

	rewind(fl);
	
	if (num_of_cmds == 0)
	{
		log("SYSERR: %s is empty!", zname);
		exit(1);
	}
	else
		CREATE(Z.cmd, RESET_COM, num_of_cmds);
	
	line_num += get_line(fl, buf);
	
	if (sscanf(buf, "#%hd", &Z.number) != 1)
	{
		log("SYSERR: Format error in %s, line %d", zname, line_num);
		exit(1);
	}
	sprintf(buf2, "beginning of zone #%d", Z.number);

	line_num += get_line(fl, buf);
	if ((ptr = strchr(buf, '~')) != NULL)	/* take off the '~' if it's there */
		*ptr = '\0';
	Z.name = str_dup(buf);
	
	Z.wild.survey	= fread_string_nospace( fl );
	Z.wild.flyto	= fread_string_nospace( fl );
	line_num++;

	line_num += get_line(fl, buf);
	if (sscanf(buf, " %d %hd %hd %hd %hd ",
			&Z.wild.dist,
			&Z.wild.z_start.y, &Z.wild.z_start.x,
			&Z.wild.z_end.y, &Z.wild.z_end.x ) != 5)
	{
		log("SYSERR: Format error in 5-constant line of %s", zname);
		exit(1);
	}
	
	line_num += get_line(fl, buf);
	if (sscanf(buf, " %hd %hd %d %d ", &Z.bot, &Z.top, &Z.lifespan, &Z.reset_mode) != 4) {
		log("SYSERR: Format error in numeric constant line of %s", zname);
		exit(1);
	}
	if (Z.bot > Z.top)
	{
		log("SYSERR: Zone %d bottom (%d) > top (%d).", Z.number, Z.bot, Z.top);
		exit(1);
	}
	
	cmd_no = 0;
	
	for (;;)
	{
		if ((tmp = get_line(fl, buf)) == 0)
		{
			log("SYSERR: Format error in %s - premature end of file", zname);
			exit(1);
		}
		line_num += tmp;
		ptr = buf;
		skip_spaces(&ptr);
		
		if ((ZCMD.command = *ptr) == '*')
			continue;
		
		ptr++;
		
		if (ZCMD.command == 'S' || ZCMD.command == '$')
		{
			ZCMD.command = 'S';
			break;
		}
		error = 0;
		if (strchr("MOEPDH", ZCMD.command) == NULL)
		{	/* a 3-arg command */
			if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
				error = 1;
		}
		else
		{
			if (sscanf(ptr, " %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3) != 4)
				error = 1;
		}
		
		ZCMD.if_flag = tmp;
		
		if (error)
		{
			log("SYSERR: Format error in %s, line %d: '%s'", zname, line_num, buf);
			exit(1);
		}
		ZCMD.line = line_num;
		cmd_no++;
	}
	
	if (num_of_cmds != cmd_no + 1)
	{
		log("SYSERR: Zone command count mismatch for %s. Estimated: %d, Actual: %d", zname, num_of_cmds, cmd_no + 1);
		exit(1);
	}
	
	top_of_zone_table = zone++;
}

#undef Z


void get_one_line(FILE *fl, char *buf)
{
	if (fgets(buf, READ_SIZE, fl) == NULL)
	{
		log("SYSERR: error reading help file: not terminated with $?");
		exit(1);
	}
	
	buf[strlen(buf) - 1] = '\0'; /* take off the trailing \n */
}


void load_help(FILE *fl)
{
	HELP_INDEX_ELEM el;
	char key[READ_SIZE+1], next_key[READ_SIZE+1], entry[32384];
	char line[READ_SIZE+1], *scan;
	
	/* get the first keyword line */
	get_one_line(fl, key);
	while (*key != '$')
	{
		/* read in the corresponding help entry */
		strcpy(entry, strcat(key, "\r\n"));
		get_one_line(fl, line);
		while (*line != '#')
		{
			strcat(entry, strcat(line, "\r\n"));
			get_one_line(fl, line);
		}
		
		/* now, add the entry to the index with each keyword on the keyword line */
		el.duplicate = 0;
		el.entry = str_dup(entry);
		scan = one_word(key, next_key);
		while (*next_key)
		{
			el.keyword = str_dup(next_key);
			help_table[top_of_helpt++] = el;
			el.duplicate++;
			scan = one_word(scan, next_key);
		}
		
		/* get next keyword line (or $) */
		get_one_line(fl, key);
	}
}


int hsort(const void *a, const void *b)
{
	const HELP_INDEX_ELEM *a1, *b1;
	
	a1 = (const HELP_INDEX_ELEM *) a;
	b1 = (const HELP_INDEX_ELEM *) b;
	
	return (str_cmp(a1->keyword, b1->keyword));
}


/*************************************************************************
*  procedures for resetting, both play-time and boot-time	 	 *
*************************************************************************/

int vnum_mobile(char *searchname, CHAR_DATA *ch)
{
	int nr, found = 0;
	
	for (nr = 0; nr <= top_of_mobt; nr++)
	{
		if (isname(searchname, GET_PC_NAME(mob_proto + nr)))
		{
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
				mob_index[nr].vnum,
				GET_SDESC(mob_proto + nr));
			send_to_char(buf, ch);
		}
	}
	
	return (found);
}

int vnum_object(char *searchname, CHAR_DATA *ch)
{
	int nr, found = 0;
	
	for (nr = 0; nr <= top_of_objt; nr++)
	{
		if (isname(searchname, obj_proto[nr].name))
		{
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
				obj_index[nr].vnum,
				obj_proto[nr].short_description);
			send_to_char(buf, ch);
		}
	}

	return (found);
}

int vnum_weapon(int attacktype, CHAR_DATA *ch)
{
	int nr, found = 0;
	
	for (nr = 0; nr <= top_of_objt; nr++)
	{
		if (obj_proto[nr].obj_flags.type_flag == ITEM_WEAPON && obj_proto[nr].obj_flags.value[3] == attacktype)
		{
			sprintf(buf, "%3d. [%5d] %s\r\n", ++found,
				obj_index[nr].vnum,
				obj_proto[nr].short_description);
			send_to_char(buf, ch);
		}
	}

	return (found);
}


/* create a character, and add it to the char list */
CHAR_DATA *create_char(void)
{
	CHAR_DATA *ch;
	
	CREATE(ch, CHAR_DATA, 1);
	clear_char(ch);

	ch->next		= character_list;
	character_list	= ch;
	
	return (ch);
}


/* create a new mobile from a prototype */
CHAR_DATA *read_mobile(mob_vnum nr, int type) /* and mob_rnum */
{
	mob_rnum i;
	CHAR_DATA *mob;
	
	if (type == VIRTUAL)
	{
		if ((i = real_mobile(nr)) < 0)
		{
			log("WARNING: Mobile vnum %d does not exist in database.", nr);
			return (NULL);
		}
	}
	else
		i = nr;
	
	CREATE(mob, CHAR_DATA, 1);
	clear_char(mob);

	*mob					= mob_proto[i];

	mob->next				= character_list;
	character_list			= mob;
	
	if (!mob->points.max_hit)
	{
		mob->points.max_hit = dice(mob->points.hit, mob->points.mana) +
			mob->points.move;
	}
	else
		mob->points.max_hit = number(mob->points.hit, mob->points.mana);
	
	mob->points.hit			= mob->points.max_hit;
	mob->points.mana		= mob->points.max_mana;
	mob->points.move		= mob->points.max_move;
	
	mob->player.time.birth	= time(0);
	mob->player.time.played = 0;
	mob->player.time.logon	= time(0);
	
	mob_index[i].number++;
	
	return (mob);
}


/* create an object, and add it to the object list */
OBJ_DATA *create_obj(void)
{
	OBJ_DATA *obj;
	
	CREATE(obj, OBJ_DATA, 1);
	clear_object(obj);

	LINK(obj, first_object, last_object, next, prev);
	
	return (obj);
}


/* create a new object from a prototype */
OBJ_DATA *read_object(obj_vnum nr, int type) /* and obj_rnum */
{
	OBJ_DATA *obj;
	obj_rnum i;
	
	if (nr < 0)
	{
		log("SYSERR: Trying to create obj with negative (%d) num!", nr);
		return (NULL);
	}
	if (type == VIRTUAL)
	{
		if ((i = real_object(nr)) < 0)
		{
			log("Object (V) %d does not exist in database.", nr);
			return (NULL);
		}
	}
	else
		i = nr;
	
	CREATE(obj, OBJ_DATA, 1);
	clear_object(obj);

	*obj = obj_proto[i];

	LINK(obj, first_object, last_object, next, prev);
	
	obj_index[i].number++;
	
	return (obj);
}


void make_maze(zone_vnum zone)
{
	ROOM_DATA *pRoom, *pNextRoom;
	room_vnum room, next_room;
	int card[400], temp, x, y, dir;
	int num, test, r_back;
	int vnum = zone_table[zone].number;
	
	for (test = 0; test < 400; test++)
	{
		card[test] = test;
		temp = test;
		dir = temp / 100;
		temp = temp - (dir * 100);
		x = temp / 10;
		temp = temp - (x * 10);
		y = temp;
		room = (vnum * 100) + (x * 10) + y;
		pRoom = get_room( room );
		if ((x == 0) && (dir == 0))
			continue;
		if ((y == 9) && (dir == 1))
			continue;
		if ((x == 9) && (dir == 2))
			continue;
		if ((y == 0) && (dir == 3))
			continue;
		if ( get_exit( pRoom, dir ) )
			extract_exit( pRoom, get_exit( pRoom, dir ) );
		REMOVE_BIT(ROOM_FLAGS(pRoom), ROOM_NOTRACK);
	}

	for (x = 0; x < 399; x++)
	{
		y = number(0, 399);
		temp = card[y];
		card[y] = card[x];
		card[x] = temp;
	}
	
	for (num = 0; num < 400; num++)
	{
		temp = card[num];
		dir = temp / 100;
		temp = temp - (dir * 100);
		x = temp / 10;
		temp = temp - (x * 10);
		y = temp;
		room = (vnum * 100) + (x * 10) + y;
		pRoom = get_room( room );
		r_back = room;
		if ((x == 0) && (dir == 0))
			continue;
		if ((y == 9) && (dir == 1))
			continue;
		if ((x == 9) && (dir == 2))
			continue;
		if ((y == 0) && (dir == 3))
			continue;
		if ( get_exit( pRoom, dir ) ) 
			continue;
		switch(dir)
		{
		case 0:
			next_room = r_back - 10;
			break;
		case 1:
			next_room = r_back + 1;
			break;
		case 2:
			next_room = r_back + 10;
			break;
		case 3:
			next_room = r_back - 1;
			break;
		}
		pNextRoom = get_room( next_room );
		test = find_first_step( pRoom, pNextRoom );
		switch (test)
		{
		case BFS_ERROR:
			log("Maze making error.");
			break;
		case BFS_ALREADY_THERE:
			log("Maze making error.");
			break;
		case BFS_NO_PATH:
			make_exit( pRoom, pNextRoom, dir );
			make_exit( pNextRoom, pRoom, rev_dir[dir] );
			break;
		}
	}

	for ( num = 0; num < 100; num++ )
	{
		room = (vnum * 100) + num;
		pRoom = get_room( room );
		/* 
		 * Remove the next line if you want to be able to track your way through
		 * the maze
		 */
		SET_BIT(ROOM_FLAGS(pRoom), ROOM_NOTRACK);
		REMOVE_BIT(ROOM_FLAGS(pRoom), ROOM_BFS_MARK);
	}
}


#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
	RESET_Q_ELEM *update_u, *temp;
	static int timer = 0;
	int i;
	char buf[128];
	
	/* jelson 10/22/92 */
	if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60)
	{
		/* one minute has passed */
		/*
		 * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
		 * factor of 60
		 */
		timer = 0;
		
		/* since one minute has passed, increment zone ages */
		for (i = 0; i <= top_of_zone_table; i++)
		{
			if (zone_table[i].age < zone_table[i].lifespan &&
				zone_table[i].reset_mode)
				(zone_table[i].age)++;
			
			if (zone_table[i].age >= zone_table[i].lifespan &&
				zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode)
			{
				/* enqueue zone */
				
				CREATE(update_u, RESET_Q_ELEM, 1);
				
				update_u->zone_to_reset = i;
				update_u->next = 0;
				
				if (!reset_q.head)
					reset_q.head = reset_q.tail = update_u;
				else
				{
					reset_q.tail->next = update_u;
					reset_q.tail = update_u;
				}
				
				zone_table[i].age = ZO_DEAD;
			}
		}
	}	/* end - one minute has passed */


	/* dequeue zones (if possible) and reset */
	/* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
	for (update_u = reset_q.head; update_u; update_u = update_u->next)
	{
		if (zone_table[update_u->zone_to_reset].reset_mode == 2 ||
			is_empty(update_u->zone_to_reset))
		{
			reset_zone(update_u->zone_to_reset);
			sprintf(buf, "Auto zone reset: %s", zone_table[update_u->zone_to_reset].name);
			mudlog(buf, CMP, LVL_GOD, FALSE);
			/* dequeue */
			if (update_u == reset_q.head)
				reset_q.head = reset_q.head->next;
			else
			{
				for (temp = reset_q.head; temp->next != update_u;
				temp = temp->next);
				
				if (!update_u->next)
					reset_q.tail = temp;
				
				temp->next = update_u->next;
			}
			
			free(update_u);
			break;
		}
	}
}

void log_zone_error(zone_rnum zone, int cmd_no, const char *message)
{
	char buf[256];
	
	sprintf(buf, "SYSERR: zone file: %s", message);
	mudlog(buf, NRM, LVL_GOD, TRUE);
	
	sprintf(buf, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d",
		ZCMD.command, zone_table[zone].number, ZCMD.line);
	mudlog(buf, NRM, LVL_GOD, TRUE);
}

#define ZONE_ERROR(message) \
	{ log_zone_error(zone, cmd_no, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
void reset_zone(zone_rnum zone)
{
	CHAR_DATA *mob = NULL;
	OBJ_DATA *obj, *obj_to;
	ROOM_DATA *pRoom;
	EXIT_DATA *pexit;
	int cmd_no, last_cmd = 0;

	for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
	{
		
		if (ZCMD.if_flag && !last_cmd)
			continue;
		
		/*  This is the list of actual zone commands.  If any new
		 *  zone commands are added to the game, be certain to update
		 *  the list of commands in load_zone() so that the counting
		 *  will still be correct. - ae.
		 */
		switch (ZCMD.command)
		{
		case '*':			/* ignore command */
			last_cmd = 0;
			break;
			
		case 'M':			/* read a mobile */
			if (mob_index[ZCMD.arg1].number < ZCMD.arg2)
			{
				int shop_nr;

				if (!(mob = read_mobile(ZCMD.arg1, REAL)))
				{
					log("SYSERR: cannot load mob %d in zone %d",
						mob_index[ZCMD.arg1].number, zone_table[zone].number);
					ZCMD.command = '*';
					last_cmd = 0;
					break;
				}
		
				if (!(pRoom = get_room(ZCMD.arg3)))
				{
					log("SYSERR: cannot get room %d for 'M' command in zone %d, mob %d",
						ZCMD.arg3, zone_table[zone].number, mob_index[ZCMD.arg1].number);
					ZCMD.command = '*';
					last_cmd = 0;
					break;
				}
				
				char_to_room(mob, pRoom);
				last_cmd = 1;

				/*
				 * Parts for loading non-native items in shops, if mob
				 * load was true, see if this mob is a shopkeeper, if it
				 * is then we should try the load function
				 */
				for (shop_nr = 0; shop_nr <= top_shop; shop_nr++)
				{
					if (shop_index[shop_nr].keeper == mob->nr)
						break;
				}
				if (shop_nr < top_shop)
					load_shop_nonnative(shop_nr, mob);
			}
			else
				last_cmd = 0;
			break;
			
		case 'O':			/* read an object */
			if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
			{
				if (ZCMD.arg3 >= 0)
				{
					if (!(obj = read_object(ZCMD.arg1, REAL)))
					{
						log("SYSERR: cannot load obj %d in zone %d, cmd 'O'",
							obj_index[ZCMD.arg1].number, zone_table[zone].number);
						ZCMD.command = '*';
						last_cmd = 0;
						break;
					}

					if (!(pRoom = get_room(ZCMD.arg3)))
					{
						log("SYSERR: cannot get room %d for 'O' command in zone %d, obj %d",
							ZCMD.arg3, zone_table[zone].number, obj_index[ZCMD.arg1].number);
						ZCMD.command = '*';
						last_cmd = 0;
						break;
					}

					if ( pRoom->number == donation_room_1 )
						SET_BIT(GET_OBJ_EXTRA(obj), ITEM_DONATED);
					obj = obj_to_room(obj, pRoom);
					last_cmd = 1;
				}
				else
				{
					obj = read_object(ZCMD.arg1, REAL);
					obj->in_room = NULL;
					last_cmd = 1;
				}
			}
			else
				last_cmd = 0;
			break;
			
		case 'P':			/* object to object */
			if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
			{
				if (!(obj = read_object(ZCMD.arg1, REAL)))
				{
					log("SYSERR: cannot load obj %d in zone %d, cmd 'P'",
						obj_index[ZCMD.arg1].number, zone_table[zone].number);
					ZCMD.command = '*';
					last_cmd = 0;
					break;
				}

				if (!(obj_to = get_obj_num(ZCMD.arg3)))
				{
					ZONE_ERROR("target obj not found, command disabled");
					ZCMD.command = '*';
					last_cmd = 0;
					break;
				}

				obj = obj_to_obj(obj, obj_to);
				last_cmd = 1;
			}
			else
				last_cmd = 0;
			break;
			
		case 'G':			/* obj_to_char */
			if (!mob)
			{
				ZONE_ERROR("attempt to give obj to non-existant mob, command disabled");
				ZCMD.command = '*';
				break;
			}
			if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
			{
				obj = read_object(ZCMD.arg1, REAL);
				obj = obj_to_char(obj, mob);
				last_cmd = 1;
			}
			else
				last_cmd = 0;
			break;
			
		case 'E':			/* object to equipment list */
			if (!mob)
			{
				ZONE_ERROR("trying to equip non-existant mob, command disabled");
				ZCMD.command = '*';
				break;
			}

			if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
			{
				if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS)
				{
					ZONE_ERROR("invalid equipment pos number");
				}
				else
				{
					obj = read_object(ZCMD.arg1, REAL);
					equip_char(mob, obj, ZCMD.arg3);
					last_cmd = 1;
				}
			}
			else
				last_cmd = 0;
			break;
			
		case 'R': /* rem obj from room */
			if ( !( pRoom = get_room( ZCMD.arg1 ) ) )
			{
				last_cmd = 0;
				break;
			}
			if ((obj = get_obj_in_list_num(ZCMD.arg2, pRoom->first_content)) != NULL)
				extract_obj(obj);
			last_cmd = 1;
			break;
			
			
		case 'D':			/* set state of door */
			if ( !( pRoom = get_room( ZCMD.arg1 ) ) )
			{
				last_cmd = 0;
				break;
			}
			if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS || !( pexit = find_exit( pRoom, ZCMD.arg2 ) ) )
			{
				ZONE_ERROR("exit does not exist, command disabled");
				ZCMD.command = '*';
			}
			else
			{
				switch (ZCMD.arg3)
				{
				case 0:
					REMOVE_BIT(EXIT_FLAGS(pexit), EX_LOCKED);
					REMOVE_BIT(EXIT_FLAGS(pexit), EX_CLOSED);
					break;
				case 1:
					SET_BIT(EXIT_FLAGS(pexit), EX_CLOSED);
					REMOVE_BIT(EXIT_FLAGS(pexit), EX_LOCKED);
					break;
				case 2:
					SET_BIT(EXIT_FLAGS(pexit), EX_LOCKED);
					SET_BIT(EXIT_FLAGS(pexit), EX_CLOSED);
					break;
				case 3:
					SET_BIT(EXIT_FLAGS(pexit), EX_HIDDEN);
					break;
				}
			}
			last_cmd = 1;
			break;
			
		/* load a random obj in room x */
		case 'H':
			if (ZCMD.arg3 >= 0)
			{
				if ( ( pRoom = get_room( ZCMD.arg3 ) ) )
				{
					if ( ( obj = load_rand_obj(ZCMD.arg1, ZCMD.arg2) ) )
					{
						if ( pRoom->number == donation_room_1 )
							SET_BIT(GET_OBJ_EXTRA(obj), ITEM_DONATED);
						obj = obj_to_room(obj, pRoom);
					}
				}
				last_cmd = 1;
			}
			else
			{
				obj = load_rand_obj(ZCMD.arg1, ZCMD.arg2);
				obj->in_room = NULL;
				last_cmd = 1;
			}
			break;
			
		/* random obj to char */
		case 'I':
			if (!mob)
			{
				ZONE_ERROR("trying to equip non-existant mob, command disabled");
				ZCMD.command = '*';
				break;
			} 
			if ( ( obj = load_rand_obj(ZCMD.arg1, GET_LEVEL(mob)) ) )
				obj = obj_to_char(obj, mob);
			last_cmd = 1;
			break;
			
		/* Random Obj to char equip */
		case 'J':
			if (!mob)
			{
				ZONE_ERROR("trying to equip non-existant mob, command disabled");
				ZCMD.command = '*';
				break;
			} 
			if ( ( obj = load_rand_obj(ZCMD.arg1, GET_LEVEL(mob)) ) )
				equip_char(mob, obj, find_eq_pos(mob, obj, NULL));
			last_cmd = 1;
			break;

		case 'Z':
			make_maze(zone);
			break;

		default:
			ZONE_ERROR("unknown cmd in reset table; cmd disabled");
			ZCMD.command = '*';
			break;
		} 
	}
  
	zone_table[zone].age = 0;
}



/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(zone_rnum zone_nr)
{
	DESCRIPTOR_DATA *i;
	
	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) != CON_PLAYING)
			continue;
		if (IN_ROOM(i->character) == NULL)
			continue;
		if (GET_LEVEL(i->character) >= LVL_IMMORT)
			continue;
		if (i->character->in_room->zone != zone_nr)
			continue;
		
		return (0);
	}
	
	return (1);
}





/************************************************************************
*  stuff related to the player table 					*
*************************************************************************/

long get_ptable_by_name(const char *name)
{
	register long i;
	
	for (i = 0; i <= top_of_p_table; i++)
		if (!str_cmp((player_table + i)->name, name))
			return (i);
		
	return (-1);
}


long get_id_by_name(const char *name)
{
	int i;
	
	for (i = 0; i <= top_of_p_table; i++)
		if (!strcmp(player_table[i].name, name))
			return (player_table[i].id);
		
	return (-1);
}


char *get_name_by_id(long id)
{
	int i;
	
	for (i = 0; i <= top_of_p_table; i++)
		if (player_table[i].id == id)
			return (player_table[i].name);
		
	return (NULL);
}


/*
 * Create a new entry in the in-memory index table for the player file.
 * If the name already exists, by overwriting a deleted character, then
 * we re-use the old position.
 */
int create_entry(char *name)
{
	int i, pos;
	
	/* no table */
	if (top_of_p_table == -1)
	{
		CREATE(player_table, PLR_INDEX_ELEM, 1);
		pos = top_of_p_table = 0;
	}
	/* new name */
	else if ((pos = get_ptable_by_name(name)) == -1)
	{
		i = ++top_of_p_table + 1;
		
		RECREATE(player_table, PLR_INDEX_ELEM, i);
		pos = top_of_p_table;
	}
	
	CREATE(player_table[pos].name, char, strlen(name) + 1);
	
	// copy lowercase equivalent of name to table field
	for (i = 0; (player_table[pos].name[i] = LOWER(name[i])); i++);
		
	return (pos);
}


/* generate index table for the player file */
void build_player_index(void)
{
	FILE *plr_index;
	int rec_count = 0, i;
	char index_name[40], line[256];//, bits[64];
	char arg2[80];
	
	sprintf(index_name, "%s", PLR_INDEX_FILE);
	if ( !( plr_index = fopen( index_name, "r" ) ) )
	{
		top_of_p_table = -1;
		log( "No player index file!  First new char will be IMP!" );
		return;
	}
	
	while ( get_line( plr_index, line ) )
	{
		if ( *line != '~' )
			rec_count++;
	}

	rewind(plr_index);
		
	if ( rec_count == 0 )
	{
		player_table = NULL;
		return;
	}
	
	CREATE(player_table, PLR_INDEX_ELEM, rec_count);

	for ( i = 0; i < rec_count; i++ )
	{
		get_line(plr_index, line);
		sscanf(line, "%ld %s", &player_table[i].id, arg2);
		
		CREATE(player_table[i].name, char, strlen(arg2) + 1);
		strcpy(player_table[i].name, arg2);
		
		top_idnum = MAX(top_idnum, player_table[i].id);
	}
	
	top_of_p_table = rec_count - 1;

	fclose(plr_index);
}

/* This function necessary to save a seperate ascii player index */
void save_player_index(void)
{
	FILE *index_file;
	int i;
	
	if ( !( index_file = fopen(PLR_INDEX_FILE, "w") ) )
	{
		log("SYSERR:  Could not write player index file");
		return;
	}
	
	for (i = 0; i <= top_of_p_table; i++)
	{
		if (*player_table[i].name)
		{
			fprintf(index_file, "%ld %s\n",
				player_table[i].id,
				player_table[i].name);
		}
	}
		
	fprintf(index_file, "~\n");
	fclose(index_file);
}

void remove_player(int pfilepos)
{
	char pfile_name[128];
	
	if (!*player_table[pfilepos].name)
		return;

	/* delete char file */
	if ( get_filename(player_table[pfilepos].name, pfile_name, PLR_FILE) )
		remove(pfile_name);
	
	/* delete char objs file */
	if ( get_filename(player_table[pfilepos].name, pfile_name, OBJS_FILE) )
		remove(pfile_name);

	/* delete char alias file */
	if ( get_filename(player_table[pfilepos].name, pfile_name, ALIAS_FILE) )
		remove(pfile_name);

	/* Unlink any other player-owned files here if you have them  */
	
	player_table[pfilepos].name[0] = '\0';
	save_player_index();
}

/************************************************************************
*  funcs of a (more or less) general utility nature			*
************************************************************************/

/*
 * Read a string from file fp using str_dup
 */
char *fread_string_nospace( FILE *fp )
{
	char buf[MAX_STRING_LENGTH];
	char *plast;
	char c;
	int ln;
	
	plast = buf;
	buf[0] = '\0';
	ln = 0;
	
	/*
	 * Skip blanks.
	 * Read first char.
	 */
	do
	{
		if ( feof(fp) )
		{
			log("fread_string_no_hash: EOF encountered on read.\r\n");
			return ( str_dup("") );
		}
		c = getc( fp );
	} while ( isspace(c) );
	
	if ( ( *plast++ = c ) == '~' )
		return ( str_dup( "" ) );
	
	for ( ;; )
	{
		if ( ln >= (MAX_STRING_LENGTH - 1) )
		{
			log( "fread_string_nospace: string too long" );
			*plast = '\0';
			return ( str_dup( buf ) );
		}
		switch ( *plast = getc( fp ) )
		{
		default:
			plast++; ln++;
			break;
			
		case EOF:
			log( "Fread_string_nospace: EOF" );
			*plast = '\0';
			return ( str_dup(buf) );
			break;
			
		case '\n':
			plast++;  ln++;
			*plast++ = '\r';  ln++;
			break;
			
		case '\r':
			break;
			
		case '~':
			*plast = '\0';
			return ( str_dup( buf ) );
		}
	}
}

/*
 * Read a letter from a file.
 */
char fread_letter( FILE *fp )
{
	char c;
	
	do
	{
		if ( feof(fp) )
		{
			log("fread_letter: EOF encountered on read.");
			return ('\0');
		}
		c = getc( fp );
	} while ( isspace(c) );
	
	return (c);
}



/*
 * Read a number from a file.
 */
int fread_number( FILE *fp )
{
	int number;
	bool sign;
	char c;
	
	do
	{
		if ( feof(fp) )
		{
			log("fread_number: EOF encountered on read.");
			return (0);
		}
		c = getc( fp );
	} while ( isspace(c) );
	
	number = 0;
	
	sign   = FALSE;
	if ( c == '+' )
	{
		c = getc( fp );
	}
	else if ( c == '-' )
	{
		sign = TRUE;
		c = getc( fp );
	}
	
	if ( !isdigit(c) )
	{
		log( "Fread_number: bad format. (%c)", c );
		return (0);
	}
	
	while ( isdigit(c) )
	{
		if ( feof(fp) )
		{
			log("fread_number: EOF encountered on read.");
			return (number);
		}
		number = number * 10 + c - '0';
		c      = getc( fp );
	}
	
	if ( sign )
		number = 0 - number;
	
	if ( c == '|' )
		number += fread_number( fp );
	else if ( c != ' ' )
		ungetc( c, fp );
	
	return (number);
}


/*
 * Read to end of line (for comments).
 */
void fread_to_eol( FILE *fp )
{
	char c;
	
	do
	{
		if ( feof(fp) )
		{
			log("fread_to_eol: EOF encountered on read.");
			return;
		}
		c = getc( fp );
	} while ( c != '\n' && c != '\r' );
	
	do
	{
		c = getc( fp );
	}
	while ( c == '\n' || c == '\r' );
	
	ungetc( c, fp );
	return;
}

/*
 * Read to end of line into static buffer			-Thoric
 */
char *fread_line( FILE *fp )
{
	static char line[MAX_STRING_LENGTH];
	char *pline;
	char c;
	int ln;
	
	pline = line;
	line[0] = '\0';
	ln = 0;
	
	/*
	 * Skip blanks.
	 * Read first char.
	 */
	do
	{
		if ( feof(fp) )
		{
			log("fread_line: EOF encountered on read.");
			strcpy(line, "");
			return (line);
		}
		c = getc( fp );
	} while ( isspace(c) );
	
	ungetc( c, fp );
	do
	{
		if ( feof(fp) )
		{
			log("fread_line: EOF encountered on read.");
			*pline = '\0';
			return line;
		}
		c = getc( fp );
		*pline++ = c; ln++;
		if ( ln >= (MAX_STRING_LENGTH - 1) )
		{
			log( "fread_line: line too long" );
			break;
		}
	} while ( c != '\n' && c != '\r' );
	
	do
	{
		c = getc( fp );
	} while ( c == '\n' || c == '\r' );
	
	ungetc( c, fp );
	*pline = '\0';
	return (line);
}



/*
 * Read one word (into static buffer).
 */
char *fread_word( FILE *fp )
{
	static char word[MAX_INPUT_LENGTH];
	char *pword;
	char cEnd;
	
	do
	{
		if ( feof(fp) )
		{
			log("fread_word: EOF encountered on read.");
			word[0] = '\0';
			return (word);
		}
		cEnd = getc( fp );
	} while ( isspace( cEnd ) );
	
	if ( cEnd == '\'' || cEnd == '"' )
	{
		pword   = word;
	}
	else
	{
		word[0] = cEnd;
		pword   = word+1;
		cEnd    = ' ';
	}
	
	for ( ; pword < word + MAX_INPUT_LENGTH; pword++ )
	{
		if ( feof(fp) )
		{
			log("fread_word: EOF encountered on read.");
			*pword = '\0';
			return (word);
		}
		*pword = getc( fp );
		if ( cEnd == ' ' ? isspace(*pword) : *pword == cEnd )
		{
			if ( cEnd == ' ' )
				ungetc( *pword, fp );
			*pword = '\0';
			return (word);
		}
	}
	
	log( "Fread_word: word too long" );
	//exit(1);
	return (NULL);
}

/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl, const char *error)
{
	char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
	char *point;
	int done = 0, length = 0, templength;
	
	*buf = '\0';
	
	do
	{
		if (!fgets(tmp, 512, fl))
		{
			log("SYSERR: fread_string: format error at or near %s", error);
			exit(1);
		}
		/* If there is a '~', end the string; else put an "\r\n" over the '\n'. */
		if ((point = strchr(tmp, '~')) != NULL)
		{
			*point = '\0';
			done = 1;
		}
		else
		{
			point = tmp + strlen(tmp) - 1;
			*(point++) = '\r';
			*(point++) = '\n';
			*point = '\0';
		}
		
		templength = strlen(tmp);
		
		if (length + templength >= MAX_STRING_LENGTH)
		{
			log("SYSERR: fread_string: string too large (db.c)");
			log("%s", error);
			exit(1);
		}
		else
		{
			strcat(buf + length, tmp);
			length += templength;
		}
	} while (!done);
	
	/* allocate space for the new string and copy it */
	if (strlen(buf) > 0)
	{
		CREATE(rslt, char, length + 1);
		strcpy(rslt, buf);
	}
	else
		rslt = NULL;
	
	return (rslt);
}


/* release memory allocated for a char struct */
void free_char(CHAR_DATA *ch)
{
	ALIAS_DATA *a;
	int i;
	
	if (ch->player_specials != NULL && ch->player_specials != &dummy_mob)
	{
		while ((a = GET_ALIASES(ch)) != NULL)
		{
			GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
			free_alias(a);
		}
		if (ch->player_specials->poofin)
			free(ch->player_specials->poofin);
		if (ch->player_specials->poofout)
			free(ch->player_specials->poofout);
		free(ch->player_specials);
		if (IS_NPC(ch))
			log("SYSERR: Mob %s (#%d) had player_specials allocated!", GET_NAME(ch), GET_MOB_VNUM(ch));
	}

	if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == -1))
	{
		/* if this is a player, or a non-prototyped non-player, free all */
		if (GET_PC_NAME(ch))
			free(GET_PC_NAME(ch));
		if (GET_TITLE(ch))
			free(GET_TITLE(ch));
		if (GET_SDESC(ch))
			free(GET_SDESC(ch));
		if (GET_LDESC(ch))
			free(GET_LDESC(ch));
		if (GET_DDESC(ch))
			free(GET_DDESC(ch));
	}
	else if ((i = GET_MOB_RNUM(ch)) >= 0)
	{
		/* otherwise, free strings only if the string is not pointing at proto */
		if (GET_PC_NAME(ch) && GET_PC_NAME(ch) != GET_PC_NAME(mob_proto + i))
			free(GET_PC_NAME(ch));
		if (GET_TITLE(ch) && GET_TITLE(ch) != GET_TITLE(mob_proto + i))
			free(GET_TITLE(ch));
		if (GET_SDESC(ch) && GET_SDESC(ch) != GET_SDESC(mob_proto + i))
			free(GET_SDESC(ch));
		if (GET_LDESC(ch) && GET_LDESC(ch) != GET_LDESC(mob_proto + i))
			free(GET_LDESC(ch));
		if (GET_DDESC(ch) && GET_DDESC(ch) != GET_DDESC(mob_proto + i))
			free(GET_DDESC(ch));
	}
	while (ch->affected)
		affect_remove(ch, ch->affected);
	
	if (ch->desc)
		ch->desc->character = NULL;
	
	free(ch);
}




/* release memory allocated for an obj struct */
void free_obj(OBJ_DATA *obj)
{
	EXTRA_DESCR *thisd, *next_one;
	int nr;
	
	if ((nr = GET_OBJ_RNUM(obj)) == -1)
	{
		if (obj->name)
			free(obj->name);
		if (obj->description)
			free(obj->description);
		if (obj->short_description)
			free(obj->short_description);
		if (obj->action_description)
			free(obj->action_description);
		if (obj->ex_description)
		{
			for (thisd = obj->ex_description; thisd; thisd = next_one)
			{
				next_one = thisd->next;
				if (thisd->keyword)
					free(thisd->keyword);
				if (thisd->description)
					free(thisd->description);
				free(thisd);
			}
		}
	}
	else 
	{
		if (obj->name && obj->name != obj_proto[nr].name)
			free(obj->name);
		if (obj->description && obj->description != obj_proto[nr].description)
			free(obj->description);
		if (obj->short_description && obj->short_description != obj_proto[nr].short_description)
			free(obj->short_description);
		if (obj->action_description && obj->action_description != obj_proto[nr].action_description)
			free(obj->action_description);
		if (obj->ex_description && obj->ex_description != obj_proto[nr].ex_description)
		{
			for (thisd = obj->ex_description; thisd; thisd = next_one)
			{
				next_one = thisd->next;
				if (thisd->keyword)
					free(thisd->keyword);
				if (thisd->description)
					free(thisd->description);
				free(thisd);
			}
		}
	}
	
	free(obj);
}


/*
 * Steps:
 *   1: Make sure no one is using the pointer in paging.
 *   2: Read contents of a text file.
 *   3: Allocate space.
 *   4: Point 'buf' to it.
 *
 * We don't want to free() the string that someone may be
 * viewing in the pager.  page_string() keeps the internal
 * str_dup()'d copy on ->showstr_head and it won't care
 * if we delete the original.  Otherwise, strings are kept
 * on ->showstr_vector but we'll only match if the pointer
 * is to the string we're interested in and not a copy.
 *
 * If someone is reading a global copy we're trying to
 * replace, give everybody using it a different copy so
 * as to avoid special cases.
 */
int file_to_string_alloc(const char *name, char **buf)
{
	DESCRIPTOR_DATA *in_use;
	char temp[MAX_STRING_LENGTH];
	int temppage;
	
	/* Lets not free() what used to be there unless we succeeded. */
	if (file_to_string(name, temp) < 0)
		return (-1);
	
	for (in_use = descriptor_list; in_use; in_use = in_use->next)
	{
		if (!in_use->showstr_count || *in_use->showstr_vector != *buf)
			continue;
		
		/* Let's be nice and leave them at the page they were on. */
		temppage = in_use->showstr_page;
		paginate_string((in_use->showstr_head = str_dup(*in_use->showstr_vector)), in_use);
		in_use->showstr_page = temppage;
	}

	if (*buf)
		free(*buf);
	
	*buf = str_dup(temp);
	return (0);
}


/* read contents of a text file, and place in buf */
int file_to_string(const char *name, char *buf)
{
	FILE *fl;
	char tmp[READ_SIZE + 3];
	int len;
	
	*buf = '\0';
	
	if (!(fl = fopen(name, "r")))
	{
		log("SYSERR: reading %s: %s", name, strerror(errno));
		return (-1);
	}
	
	for (;;)
	{
		if (!fgets(tmp, READ_SIZE, fl))	/* EOF check */
			break;
		if ((len = strlen(tmp)) > 0)
			tmp[len - 1] = '\0'; /* take off the trailing \n */
		strcat(tmp, "\r\n");
		
		if (strlen(buf) + strlen(tmp) + 1 > MAX_STRING_LENGTH)
		{
			log("SYSERR: %s: string too big (%d max)", name, MAX_STRING_LENGTH);
			*buf = '\0';
			fclose(fl);
			return (-1);
		}
		strcat(buf, tmp);
	}
	
	fclose(fl);
	
	return (0);
}



/* clear some of the the working variables of a char */
void reset_char(CHAR_DATA *ch)
{
	int i;
	
	for (i = 0; i < NUM_WEARS; i++)
		GET_EQ(ch, i) = NULL;
	
	ch->followers					= NULL;
	ch->master						= NULL;
	ch->in_room						= NULL;
	ch->in_obj						= NULL;
	ch->in_building					= NULL;
	ch->in_vehicle					= NULL;
	ch->first_carrying				= NULL;
	ch->last_carrying				= NULL;
	ch->next						= NULL;
	ch->next_fighting				= NULL;
	ch->next_in_room				= NULL;
	ch->next_in_obj					= NULL;
	ch->next_in_building			= NULL;
	ch->next_in_vehicle				= NULL;
	ch->action						= NULL;
	FIGHTING(ch)					= NULL;
	ch->player.position				= POS_STANDING;
	ch->mob_specials.default_pos	= POS_STANDING;
	ch->player.carry_weight			= 0;
	ch->player.carry_items			= 0;
	
	if (GET_HIT(ch) <= 0)
		GET_HIT(ch) = 1;
	if (GET_MOVE(ch) <= 0)
		GET_MOVE(ch) = 1;
	if (GET_MANA(ch) <= 0)
		GET_MANA(ch) = 1;
	
	GET_LAST_TELL(ch) = NOBODY;
}



/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(CHAR_DATA *ch)
{
	memset((char *) ch, 0, sizeof(CHAR_DATA));

	ch->in_room						= NULL;
	ch->in_building					= NULL;
	ch->in_vehicle					= NULL;
	ch->in_obj						= NULL;
	GET_PFILEPOS(ch)				= -1;
	GET_MOB_RNUM(ch)				= NOBODY;
	GET_WAS_IN(ch)					= NULL;
	GET_POS(ch)						= POS_STANDING;
	ch->mob_specials.default_pos	= POS_STANDING;

	GET_AC(ch) = 100;		/* Basic Armor */

	if (ch->points.max_mana < 100)
		ch->points.max_mana = 100;
}


void clear_object(OBJ_DATA *obj)
{
	memset((char *) obj, 0, sizeof(OBJ_DATA));
	
	obj->item_number			= NOTHING;
	obj->next					= NULL;
	obj->prev					= NULL;
	obj->in_room				= NULL;
	obj->in_vehicle				= NULL;
	obj->in_obj					= NULL;
	obj->people					= NULL;
	obj->first_content			= NULL;
	obj->last_content			= NULL;
	obj->next_content			= NULL;
	obj->prev_content			= NULL;
	obj->worn_on				= NOWHERE;
	obj->count					= 1;
	obj->action					= NULL;

	obj->special				= NULL;
	obj->obj_flags.owner_id		= NOBODY;

	/* every obj start at full condition (100/100) */
	GET_OBJ_MAXCOND(obj)		= 100;
	GET_OBJ_COND(obj)			= 100;
}


/* initialize a new character only if class is set */
void init_char(CHAR_DATA *ch)
{
	int i;
	
	/* create a player_special structure */
	if (ch->player_specials == NULL)
		CREATE(ch->player_specials, PLAYER_SPECIAL, 1);
	
	/* *** if this is our first player --- he be God *** */
	if (top_of_p_table == 0)
	{
		GET_EXP(ch) = 7000000;
		GET_LEVEL(ch) = LVL_IMPL;
		GET_TOT_LEVEL(ch) = LVL_IMPL * NUM_CLASSES;
	
		for (i = 0; i < NUM_CLASSES; i++)
		{
			if (GET_CLASS(ch) == i)
				continue;
			SET_BIT(MULTICLASS(ch), (1 << i));
		}
		GET_MAX_HIT(ch)			= 500;
		GET_MAX_MANA(ch)		= 500;
		GET_MAX_MOVE(ch)		= 500;

		ch->real_abils.intel	= 25;
		ch->real_abils.wis		= 25;
		ch->real_abils.dex		= 25;
		ch->real_abils.str		= 25;
		ch->real_abils.con		= 25;
		ch->real_abils.cha		= 25;
	}
	else
	{
		GET_MAX_HIT(ch)			= 10;
		GET_MAX_MANA(ch)		= 100;
		GET_MAX_MOVE(ch)		= 80;
	}

	set_title(ch, NULL);
	GET_SDESC(ch)				= NULL;
	GET_LDESC(ch)				= NULL;
	GET_DDESC(ch)				= NULL;
	
	ch->player.hometown			= 1;
	
	ch->player.time.birth		= time(0);
	ch->player.time.played		= 0;
	ch->player.time.logon		= time(0);
	
	for (i = 0; i < MAX_TONGUE; i++)
		GET_TALK(ch, i)		= 0;
	
	/* make favors for sex */
	if (ch->player.sex == SEX_MALE)
	{
		ch->player.weight		= number(120, 180);
		ch->player.height		= number(160, 200);
	}
	else
	{
		ch->player.weight		= number(100, 160);
		ch->player.height		= number(150, 180);
	}

	GET_HIT(ch)					= GET_MAX_HIT(ch);
	GET_MANA(ch)				= GET_MAX_MANA(ch);
	GET_MOVE(ch)				= GET_MAX_MOVE(ch);

	// setup armor class
	GET_AC(ch)					= 100;
	
	if ((i = get_ptable_by_name(GET_NAME(ch))) != -1)
		player_table[i].id = GET_IDNUM(ch) = ++top_idnum;
	else
		log("SYSERR: init_char: Character '%s' not found in player table.", GET_NAME(ch));
	
	for (i = 1; i <= MAX_SKILLS; i++)
	{
		if (GET_LEVEL(ch) < LVL_IMPL)
			SET_SKILL(ch, i, 0);
		else
			SET_SKILL(ch, i, 100);
	}
	
	ch->player.affected_by		= 0;
	
	for (i = 0; i < 5; i++)
		GET_SAVE(ch, i)			= 0;
	
	for (i = 0; i < 3; i++)
		GET_COND(ch, i)			= (GET_LEVEL(ch) == LVL_IMPL ? -1 : 24);
	
	GET_LOADROOM(ch)			= NOWHERE;
	GET_LOADBUILDING(ch)		= NOWHERE;
	GET_LOADSHIP(ch)			= NOWHERE;
	GET_LOADCOORD(ch)			= NULL;

	GET_MOB_KILLS(ch)			= 0;
	GET_MOB_DEATHS(ch)			= 0;
	GET_PLR_KILLS(ch)			= 0;
	GET_PLR_DEATHS(ch)			= 0;
}


ROOM_DATA *get_room(room_vnum vnum)
{
	ROOM_DATA *pRoom;
	int iHash;

	if ( vnum < 0 )
		return (NULL);

	iHash = vnum % ROOM_HASH;

	for ( pRoom = World[iHash]; pRoom; pRoom = pRoom->next )
	{
		if ( pRoom->number == vnum )
			break;
	}

	return (pRoom);
}


/* returns the real number of the monster with given virtual number */
mob_rnum real_mobile(mob_vnum vnum)
{
	mob_rnum bot, top, mid;
	
	bot = 0;
	top = top_of_mobt;
	
	/* perform binary search on mob-table */
	for (;;)
	{
		mid = (bot + top) / 2;
		
		if ((mob_index + mid)->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (NOBODY);
		if ((mob_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}


/* returns the real number of the object with given virtual number */
obj_rnum real_object(obj_vnum vnum)
{
	obj_rnum bot, top, mid;
	
	bot = 0;
	top = top_of_objt;
	
	/* perform binary search on obj-table */
	for (;;)
	{
		mid = (bot + top) / 2;
		
		if ((obj_index + mid)->vnum == vnum)
			return (mid);
		if (bot >= top)
			return (NOTHING);
		if ((obj_index + mid)->vnum > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}


/* returns the real number of the zone with given virtual number */
zone_rnum real_zone(room_vnum vnum)
{
	zone_rnum bot, top, mid;
	
	bot = 0;
	top = top_of_zone_table;
	
	/* perform binary search on zone-table */
	for (;;)
	{
		mid = (bot + top) / 2;
		
		if ((zone_table + mid)->number == vnum)
			return (mid);
		if (bot >= top)
			return (NOWHERE);
		if ((zone_table + mid)->number > vnum)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}


/*
 * Extend later to include more checks.
 *
 * TODO: Add checks for unknown bitvectors.
 */
int check_object(OBJ_DATA *obj)
{
	int error = FALSE;
	
	if (GET_OBJ_WEIGHT(obj) < 0 && (error = TRUE))
		log("SYSERR: Object #%d (%s) has negative weight (%d).",
			GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_WEIGHT(obj));
	
	if (GET_OBJ_RENT(obj) < 0 && (error = TRUE))
		log("SYSERR: Object #%d (%s) has negative cost/day (%d).",
			GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_RENT(obj));
	
	sprintbit(GET_OBJ_WEAR(obj), wear_bits, buf);
	if (strstr(buf, "UNDEFINED") && (error = TRUE))
		log("SYSERR: Object #%d (%s) has unknown wear flags.",
			GET_OBJ_VNUM(obj), obj->short_description);
	
	sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
	if (strstr(buf, "UNDEFINED") && (error = TRUE))
		log("SYSERR: Object #%d (%s) has unknown extra flags.",
			GET_OBJ_VNUM(obj), obj->short_description);
	
	sprintbit(obj->obj_flags.bitvector, affected_bits, buf);
	if (strstr(buf, "UNDEFINED") && (error = TRUE))
		log("SYSERR: Object #%d (%s) has unknown affection flags.",
			GET_OBJ_VNUM(obj), obj->short_description);
	
	switch (GET_OBJ_TYPE(obj))
	{
	case ITEM_DRINKCON:
	{
		char onealias[MAX_INPUT_LENGTH], *space = strrchr(obj->name, ' ');
			
		strcpy(onealias, space ? space + 1 : obj->name);
		if (search_block(onealias, drinknames, TRUE) < 0 && (error = TRUE))
			log("SYSERR: Object #%d (%s) doesn't have drink type as last alias. (%s)",
				GET_OBJ_VNUM(obj), obj->short_description, obj->name);
	}
	/* Fall through. */
	case ITEM_FOUNTAIN:
		if (GET_OBJ_VAL(obj, 1) > GET_OBJ_VAL(obj, 0) && (error = TRUE))
			log("SYSERR: Object #%d (%s) contains (%d) more than maximum (%d).",
				GET_OBJ_VNUM(obj), obj->short_description,
				GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 0));
		break;
	case ITEM_SCROLL:
	case ITEM_POTION:
		error |= check_object_level(obj, 0);
		error |= check_object_spell_number(obj, 1);
		error |= check_object_spell_number(obj, 2);
		error |= check_object_spell_number(obj, 3);
		break;
	case ITEM_WAND:
	case ITEM_STAFF:
		error |= check_object_level(obj, 0);
		error |= check_object_spell_number(obj, 3);
		if (GET_OBJ_VAL(obj, 2) > GET_OBJ_VAL(obj, 1) && (error = TRUE))
			log("SYSERR: Object #%d (%s) has more charges (%d) than maximum (%d).",
				GET_OBJ_VNUM(obj), obj->short_description,
				GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 1));
		break;
	}
	
	return (error);
}

int check_object_spell_number(OBJ_DATA *obj, int val)
{
	int error = FALSE;
	const char *spellname;
	
	if (GET_OBJ_VAL(obj, val) == -1)	/* i.e.: no spell */
		return (error);
	
	/*
	 * Check for negative spells, spells beyond the top define, and any
	 * spell which is actually a skill.
	 */
	if (GET_OBJ_VAL(obj, val) < 0)
		error = TRUE;
	if (GET_OBJ_VAL(obj, val) > TOP_SPELL_DEFINE)
		error = TRUE;
	if (GET_OBJ_VAL(obj, val) > MAX_SPELLS && GET_OBJ_VAL(obj, val) <= MAX_SKILLS)
		error = TRUE;
	if (error)
		log("SYSERR: Object #%d (%s) has out of range spell #%d.",
			GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));
	
	/*
	 * This bug has been fixed, but if you don't like the special behavior...
	 */
#if 0
	if (GET_OBJ_TYPE(obj) == ITEM_STAFF && HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS | MAG_MASSES))
		log("... '%s' (#%d) uses %s spell '%s'.",
			obj->short_description,	GET_OBJ_VNUM(obj),
			HAS_SPELL_ROUTINE(GET_OBJ_VAL(obj, val), MAG_AREAS) ? "area" : "mass",
			skill_name(GET_OBJ_VAL(obj, val)));
#endif
	
	if (scheck)		/* Spell names don't exist in syntax check mode. */
		return (error);
	
	/* Now check for unnamed spells. */
	spellname = skill_name(GET_OBJ_VAL(obj, val));
	
	if ((spellname == unused_spellname || !str_cmp("UNDEFINED", spellname)) && (error = TRUE))
		log("SYSERR: Object #%d (%s) uses '%s' spell #%d.",
			GET_OBJ_VNUM(obj), obj->short_description, spellname,
			GET_OBJ_VAL(obj, val));
	
	return (error);
}

int check_object_level(OBJ_DATA *obj, int val)
{
	int error = FALSE;
	
	if ((GET_OBJ_VAL(obj, val) < 0 || GET_OBJ_VAL(obj, val) > LVL_IMPL) && (error = TRUE))
		log("SYSERR: Object #%d (%s) has out of range level #%d.",
			GET_OBJ_VNUM(obj), obj->short_description, GET_OBJ_VAL(obj, val));
	
	return (error);
}

void create_survey_table( void )
{
	SURVEY_DATA *sd;
	zone_rnum zn;

	for (zn = 0; zn < top_of_zone_table; zn++)
	{
		if (!str_cmp(zone_table[zn].wild.survey, "undefined"))
			continue;

		if (zone_table[zn].wild.z_start.y == 0)
			continue;

		CREATE(sd, SURVEY_DATA, 1);
		CREATE(sd->coord, COORD_DATA, 1);

		sd->descriz	= str_dup(zone_table[zn].wild.survey);
		sd->coord->y	= (zone_table[zn].wild.z_start.y + zone_table[zn].wild.z_end.y) / 2;
		sd->coord->x	= (zone_table[zn].wild.z_start.x + zone_table[zn].wild.z_end.x) / 2;

		/* add to the global list */
		sd->next	= survey_table;
		survey_table	= sd;
	}
}

/* ======================================================================= */

#define STRING_TERMINATOR       '~'

/* write OBJECT files */
int save_objects(zone_rnum zone_num)
{
	FILE *fp;
	OBJ_DATA *obj;
	EXTRA_DESCR *ex_desc;
	char owear[128], oextra[128], oanti[128], oflags[128];
	int counter, counter2, realcounter;
	
	if (zone_num < 0 || zone_num > top_of_zone_table)
	{
		log("SYSERR: save_objects: Invalid real zone number %d. (0-%d)", zone_num, top_of_zone_table);
		return (FALSE);
	}
	
	sprintf(buf, "%s/%d.new", OBJ_PREFIX, zone_table[zone_num].number);
	if (!(fp = fopen(buf, "w+")))
	{
		mudlog("SYSERR: Cannot open objects file!", BRF, LVL_IMMORT, TRUE);
		return (FALSE);
	}

	/* Start running through all objects in this zone. */
	for (counter = zone_table[zone_num].number * 100; counter <= zone_table[zone_num].top; counter++)
	{
		if ((realcounter = real_object(counter)) >= 0)
		{
			if ((obj = &obj_proto[realcounter])->action_description)
			{
				buf1[MAX_STRING_LENGTH - 1] = '\0';
				strncpy(buf1, obj->action_description, MAX_STRING_LENGTH - 1);
				strip_cr(buf1);
			}
			else
				*buf1 = '\0';

			sprintascii( oextra, GET_OBJ_EXTRA(obj) );
			sprintascii( owear, GET_OBJ_WEAR(obj) );
			sprintascii( oflags, GET_OBJ_PERM(obj) );
			sprintascii( oanti, GET_OBJ_ANTI(obj) );
			
			fprintf(fp,
				"#%d\n"
				"%s~\n"
				"%s~\n"
				"%s~\n"
				"%s~\n"
				"%d %s %s %s %s\n"
				"%d %d %d %d\n"
				"%d %d %d %d %d %d\n",
				GET_OBJ_VNUM(obj),
				(obj->name && *obj->name) ? obj->name : "undefined",
				(obj->short_description && *obj->short_description) ? obj->short_description : "undefined",
				(obj->description && *obj->description) ? obj->description : "undefined",
				buf1,
				GET_OBJ_TYPE(obj), oextra, owear, oflags, oanti,
				GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3),
				GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj), GET_OBJ_LEVEL(obj),
				GET_OBJ_COND(obj), GET_OBJ_MAXCOND(obj)
				);
			
			/*
			 * Do we have extra descriptions? 
			 */
			if (obj->ex_description)
			{	/* Yes, save them too. */
				for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next)
				{
					/*
					 * Sanity check to prevent nasty protection faults.
					 */
					if (!ex_desc->keyword || !ex_desc->description || !*ex_desc->keyword || !*ex_desc->description)
					{
						mudlog("SYSERR: save_objects: Corrupt ex_desc!", BRF, LVL_IMMORT, TRUE);
						continue;
					}
					buf1[MAX_STRING_LENGTH - 1] = '\0';
					strncpy(buf1, ex_desc->description, MAX_STRING_LENGTH - 1);
					strip_cr(buf1);
					fprintf(fp, "E\n"
						"%s~\n"
						"%s~\n", ex_desc->keyword, buf1);
				}
			}

			/*
			 * Do we have affects? 
			 */
			for (counter2 = 0; counter2 < MAX_OBJ_AFF; counter2++)
			{
				if (obj->affected[counter2].modifier)
					fprintf(fp,
						"A\n"
						"%d %d\n",
						obj->affected[counter2].location,
						obj->affected[counter2].modifier);
			}
			
			if ( obj->special )
			{
			/* Do we have spells? */
			if ( OBJ_FLAGGED(obj, ITEM_HAS_SPELLS))
			{
				OBJ_SPELLS_DATA *oSpell;

				for (oSpell = (OBJ_SPELLS_DATA *) obj->special; oSpell; oSpell = oSpell->next)
					if (oSpell->spellnum)
						fprintf(fp,
							"S\n"
							"%d %d %d\n",
							oSpell->spellnum,
							oSpell->level,
							oSpell->percent);
			}

			/*
			 * Do we have traps? 
			 */
			if ( OBJ_FLAGGED(obj, ITEM_HAS_TRAPS) )
			{
				OBJ_TRAP_DATA *trap;
				char f1[256];

				for (trap = (OBJ_TRAP_DATA *) obj->special; trap; trap = trap->next)
				{
					sprintascii(f1, trap->action);
					fprintf(fp,
						"T\n"
						"%s %s %d %d\n",
						f1, trap_dam_descl[trap->dam_type],
						trap->charges, ( trap->whole_room ? 1 : 0 ) );
				}
			}
			}
		}
	}
	
	/*
	 * Write the final line, close the file.
	 */
	fprintf(fp, "$~\n");
	fclose(fp);
	sprintf(buf2, "%s/%d.obj", OBJ_PREFIX, zone_table[zone_num].number);
	remove(buf2);
	rename(buf, buf2);
	
	return (TRUE);
}

/*
ACMD(do_objsaveall)
{
	zone_rnum zn;

	for ( zn = 0; zn <= top_of_zone_table; zn++ )
		save_objects(zn);

	send_to_char(OK, ch);
}
*/


int write_mobile_espec(mob_vnum mvnum, CHAR_DATA *mob, FILE *fd)
{
	if (GET_ATTACK(mob) != 0)
		fprintf(fd, "BareHandAttack: %d\n", GET_ATTACK(mob));
	if (GET_STR(mob) != 11)
		fprintf(fd, "Str: %d\n", GET_STR(mob));
	if (GET_DEX(mob) != 11)
		fprintf(fd, "Dex: %d\n", GET_DEX(mob));
	if (GET_INT(mob) != 11)
		fprintf(fd, "Int: %d\n", GET_INT(mob));
	if (GET_WIS(mob) != 11)
		fprintf(fd, "Wis: %d\n", GET_WIS(mob));
	fputs("E\n", fd);
	return (TRUE);
}

int write_mobile_record(mob_vnum mvnum, CHAR_DATA *mob, FILE *fd)
{
	char ldesc[MAX_STRING_LENGTH], ddesc[MAX_STRING_LENGTH];
	char maffect[128], mflags[128];
	
	ldesc[MAX_STRING_LENGTH - 1] = '\0';
	ddesc[MAX_STRING_LENGTH - 1] = '\0';
	strip_cr(strncpy(ldesc, GET_LDESC(mob), MAX_STRING_LENGTH - 1));
	strip_cr(strncpy(ddesc, GET_DDESC(mob), MAX_STRING_LENGTH - 1));
	
	sprintascii(mflags, MOB_FLAGS(mob));
	sprintascii(maffect, AFF_FLAGS(mob));

	fprintf(fd,
		"#%d\n"
		"%s%c\n"
		"%s%c\n"
		"%s%c\n"
		"%s%c\n",
		mvnum,
		GET_PC_NAME(mob), STRING_TERMINATOR,
		GET_SDESC(mob), STRING_TERMINATOR,
		ldesc, STRING_TERMINATOR,
		ddesc, STRING_TERMINATOR
		);
	fprintf(fd,
		"%s %s %d E\n"
		"%d %d %d %dd%d+%d %dd%d+%d\n",
		mflags, maffect, GET_ALIGNMENT(mob),
		GET_LEVEL(mob), 20 - GET_HITROLL(mob), GET_AC(mob) / 10, GET_HIT(mob),
		GET_MANA(mob), GET_MOVE(mob), GET_NDD(mob), GET_SDD(mob),
		GET_DAMROLL(mob)
		);
	fprintf(fd,
		"%d %d\n"
		"%d %d %d\n",
		GET_GOLD(mob), GET_EXP(mob),
		GET_POS(mob), GET_DEFAULT_POS(mob), GET_SEX(mob)
		);
	
	if (write_mobile_espec(mvnum, mob, fd) < 0)
		log("SYSERR: Error writing E-specs for mobile #%d.", mvnum);
	
	return (TRUE);
}

void check_mobile_string(mob_vnum i, char **string, const char *dscr)
{
	if (*string == NULL || **string == '\0')
	{
		char smbuf[128];

		sprintf(smbuf, "SYSERR: Mob #%d has an invalid %s.", i, dscr);
		mudlog(smbuf, BRF, LVL_GOD, TRUE);
		if (*string)
			free(*string);
		*string = str_dup("An undefined string.");
	}
}

void check_mobile_strings(CHAR_DATA *mob)
{
	mob_vnum mvnum = mob_index[mob->nr].vnum;
	check_mobile_string(mvnum, &GET_LDESC(mob), "long description");
	check_mobile_string(mvnum, &GET_DDESC(mob), "detailed description");
	check_mobile_string(mvnum, &GET_PC_NAME(mob), "alias list");
	check_mobile_string(mvnum, &GET_SDESC(mob), "short description");
}

int save_mobiles(zone_rnum rznum)
{
	zone_vnum vznum;
	FILE *mobfd;
	room_vnum i;
	mob_rnum rmob;
	int written;
	char mobfname[64], usedfname[64];
	
	if (rznum < 0 || rznum > top_of_zone_table)
	{
		log("SYSERR: save_mobiles: Invalid real zone number %d. (0-%d)", rznum, top_of_zone_table);
		return FALSE;
	}
	
	vznum = zone_table[rznum].number;
	sprintf(mobfname, "%s%d.new", MOB_PREFIX, vznum);
	if ((mobfd = fopen(mobfname, "w")) == NULL)
	{
		mudlog("SYSERR: Cannot open mob file for writing.", BRF, LVL_GOD, TRUE);
		return FALSE;
	}
	
	for (i = vznum * 100; i <= zone_table[rznum].top; i++)
	{
		if ((rmob = real_mobile(i)) == NOBODY)
			continue;
		check_mobile_strings(&mob_proto[rmob]);
		if (write_mobile_record(i, &mob_proto[rmob], mobfd) < 0)
			log("SYSERR: Error writing mobile #%d.", i);
	}
	fputs("$\n", mobfd);
	written = ftell(mobfd);
	fclose(mobfd);
	sprintf(usedfname, "%s%d.mob", MOB_PREFIX, vznum);
	remove(usedfname);
	rename(mobfname, usedfname);
	
	return (written);
}

/*
ACMD(do_mobsaveall)
{
	zone_rnum zn;

	for ( zn = 0; zn <= top_of_zone_table; zn++ )
		save_mobiles(zn);

	send_to_char(OK, ch);
}
*/
