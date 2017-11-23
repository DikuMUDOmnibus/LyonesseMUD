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
*   File: interpreter.c                                 Part of CircleMUD *
*  Usage: parse user commands, search for specials, call ACMD functions   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __INTERPRETER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"


/* external variables */
extern const char	*class_menu;
extern const char	*race_menu;
extern char			*motd;
extern char			*imotd;
extern char			*background;
extern char			*MENU;
extern char			*WELC_MESSG;
extern char			*START_MESSG;
extern int			circle_restrict;
extern int			no_specials;
extern int			max_bad_pws;

/* external functions */
ROOM_DATA *create_wild_room(COORD_DATA *coord, bool Static);
BUILDING_DATA *find_building(int vnum);
ROOM_DATA *find_room_building(BUILDING_DATA *bld, int room);
SHIP_DATA *find_ship(sh_int vnum);
ROOM_DATA *find_room_ship(SHIP_DATA *ship, int room);
int		parse_class(char arg);
int		parse_race(char arg);
int		special(CHAR_DATA *ch, int cmd, char *arg);
int		isbanned(char *hostname);
int		Valid_Name(char *newname);
int		LoadCharObj(CHAR_DATA *ch);
long	get_ptable_by_name(const char *name);
void	roll_real_abils(CHAR_DATA *ch);
void	wild_map_load( CHAR_DATA *ch, COORD_DATA *coord );
void	echo_on(DESCRIPTOR_DATA *d);
void	echo_off(DESCRIPTOR_DATA *d);
void	do_start(CHAR_DATA *ch);
void	read_aliases(CHAR_DATA *ch);
void	remove_player(int pfilepos);
void	char_to_building(CHAR_DATA *ch, BUILDING_DATA *bld, int room);
void	char_to_ship( CHAR_DATA *ch, SHIP_DATA *ship, int room );

/* local functions */
int perform_dupe_check(DESCRIPTOR_DATA *d);
ALIAS_DATA *find_alias(ALIAS_DATA *alias_list, char *str);
void free_alias(ALIAS_DATA *a);
void perform_complex_alias(TXT_Q *input_q, char *orig, ALIAS_DATA *a);
int perform_alias(DESCRIPTOR_DATA *d, char *orig);
int reserved_word(char *argument);
int _parse_name(char *arg, char *name);

/* prototypes for all do_x functions. */
ACMD(do_action);
ACMD(do_advance);
ACMD(do_alias);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_bash);
ACMD(do_cast);
ACMD(do_color);
ACMD(do_commands);
ACMD(do_consider);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_diagnose);
ACMD(do_display);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_enter);					// HELP updated
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_find);
ACMD(do_flee);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_door);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_gold);					// HELP updated
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_hcontrol);
ACMD(do_help);
ACMD(do_hide);
ACMD(do_hit);					// HELP updated
ACMD(do_house);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_last);
ACMD(do_levels);
ACMD(do_load);
ACMD(do_look);
ACMD(do_not_here);
ACMD(do_order);
ACMD(do_page);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quit);
ACMD(do_reboot);
ACMD(do_remove);
ACMD(do_repair);
ACMD(do_reply);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_save);
ACMD(do_say);
ACMD(do_score);
ACMD(do_send);
ACMD(do_set);
ACMD(do_show);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_split);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_steal);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_use);
ACMD(do_users);
ACMD(do_visible);
ACMD(do_vlist);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_write);
ACMD(do_zoneconn);
ACMD(do_zreset);

/* new commands */
ACMD(do_attrib);				//							// HELP done
ACMD(do_knock);					//							// HELP done
ACMD(do_remort);				//							// HELP done
ACMD(do_scan);					//							// HELP done
ACMD(do_search);				//							// HELP done
ACMD(do_shoot);					//							// HELP done
ACMD(do_memorize);				// new magic system
ACMD(do_copy);					// new magic system
ACMD(do_study);					// new magic system
ACMD(do_enchant);				// new magic system
ACMD(do_mount);					// mount code				// HELP done
ACMD(do_dismount);				// mount code				// HELP done
ACMD(do_tame);					// mount code				// HELP done
ACMD(do_greet);					// introduction code		// HELP done
ACMD(do_forget);				// introduction code		// HELP done
ACMD(do_knowings);				// introduction code		// HELP done
ACMD(do_survey);				// wilderness				// HELP done
ACMD(do_travel);				// wilderness				// HELP done
ACMD(do_camp);					// wilderness
ACMD(do_build);					// buildings
ACMD(do_embark);				// ships					// HELP done
ACMD(do_disembark);				// ships					// HELP done
ACMD(do_ship);					// ships
ACMD(do_drive);					// vehicles
ACMD(do_charge);				// vehicles
ACMD(do_discharge);				// vehicles
ACMD(do_out);					// vehicles
ACMD(do_yoke);					// vehicles
ACMD(do_unyoke);				// vehicles
ACMD(do_clan);					// clans
ACMD(do_ctell);					// clans
ACMD(do_negotiate);				// clans
ACMD(do_politics);				// clans
ACMD(do_trapremove);			// obj traps
ACMD(do_trapstat);				// obj traps
ACMD(do_authorize);				// buildings & ships code

/* new immortal commands */
ACMD(do_newmoney);
ACMD(do_newbook);
ACMD(do_newvehicle);
ACMD(do_newgoods);
ACMD(do_market);
ACMD(do_mkaff);
ACMD(do_goodinfo);
ACMD(do_tp);
ACMD(do_economy);
ACMD(do_life);
ACMD(do_remap);
ACMD(do_wildlist);
ACMD(do_bldsave);
ACMD(do_bldlist);
ACMD(do_courselist);
ACMD(do_shipsetup);
ACMD(do_shipread);
ACMD(do_traplist);

/* temp */
//ACMD(do_objsaveall);
//ACMD(do_mobsaveall);
ACMD(do_newspaper);

/* 
 * This is the Master Command List(tm).

 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

cpp_extern const COMMAND_INFO cmd_info[] =
{
        { "RESERVED", 0, 0, 0, 0 },     /* this _must_ be first -- for specprocs */
                
        /* directions must come before other commands but after RESERVED */
        { "north"       , POS_STANDING  , do_move       , 0, SCMD_NORTH },
        { "east"        , POS_STANDING  , do_move       , 0, SCMD_EAST },
        { "south"       , POS_STANDING  , do_move       , 0, SCMD_SOUTH },
        { "west"        , POS_STANDING  , do_move       , 0, SCMD_WEST },
        { "up"          , POS_STANDING  , do_move       , 0, SCMD_UP },
        { "down"        , POS_STANDING  , do_move       , 0, SCMD_DOWN },
        { "ne"          , POS_STANDING  , do_move       , 0, SCMD_NORTHEAST },
        { "nw"          , POS_STANDING  , do_move       , 0, SCMD_NORTHWEST },
        { "se"          , POS_STANDING  , do_move       , 0, SCMD_SOUTHEAST },
        { "sw"          , POS_STANDING  , do_move       , 0, SCMD_SOUTHWEST },
        { "northeast"   , POS_STANDING  , do_move       , 0, SCMD_NORTHEAST },
        { "northwest"   , POS_STANDING  , do_move       , 0, SCMD_NORTHWEST },
        { "southeast"   , POS_STANDING  , do_move       , 0, SCMD_SOUTHEAST },
        { "southwest"   , POS_STANDING  , do_move       , 0, SCMD_SOUTHWEST },
        
        /* now, the main list */
        { "attrib"      , POS_DEAD      , do_attrib     , 0, 0 },
        { "at"          , POS_DEAD      , do_at         , LVL_IMMORT, 0 },
        { "advance"     , POS_DEAD      , do_advance    , LVL_IMPL, 0 },
        { "alias"       , POS_DEAD      , do_alias      , 0, 0 },
        { "accuse"      , POS_SITTING   , do_action     , 0, 0 },
        { "analyze"     , POS_STANDING  , do_not_here   , 1, 0 },
        { "applaud"     , POS_RESTING   , do_action     , 0, 0 },
        { "assist"      , POS_FIGHTING  , do_assist     , 1, 0 },
        { "ask"         , POS_RESTING   , do_spec_comm  , 0, SCMD_ASK },
        { "auction"     , POS_SLEEPING  , do_gen_comm   , 0, SCMD_AUCTION },
        { "autoexit"    , POS_DEAD      , do_gen_tog    , 0, SCMD_AUTOEXIT },
        { "authorize"   , POS_STANDING  , do_authorize  , 1, 0 },
        
        { "bounce"      , POS_STANDING  , do_action     , 0, 0 },
        { "backstab"    , POS_STANDING  , do_backstab   , 1, 0 },
        { "ban"         , POS_DEAD      , do_ban        , LVL_GRGOD, 0 },
        { "balance"     , POS_STANDING  , do_not_here   , 1, 0 },
        { "bash"        , POS_FIGHTING  , do_bash       , 1, 0 },
        { "beg"         , POS_RESTING   , do_action     , 0, 0 },
        { "bleed"       , POS_RESTING   , do_action     , 0, 0 },
        { "blush"       , POS_RESTING   , do_action     , 0, 0 },
        { "bow"         , POS_STANDING  , do_action     , 0, 0 },
        { "brb"         , POS_RESTING   , do_action     , 0, 0 },
        { "brief"       , POS_DEAD      , do_gen_tog    , 0, SCMD_BRIEF },
        { "burp"        , POS_RESTING   , do_action     , 0, 0 },
        { "buy"         , POS_STANDING  , do_not_here   , 0, 0 },
        { "bug"         , POS_DEAD      , do_gen_write  , 0, SCMD_BUG },
        { "build"       , POS_STANDING  , do_build      , 1, 0 },
        { "bldlist"     , POS_DEAD      , do_bldlist    , LVL_IMMORT, 0 },
        { "bldsave"     , POS_DEAD      , do_bldsave    , LVL_IMMORT, 0 },
        
        { "cast"        , POS_SITTING   , do_cast       , 1, 0 },
        { "camp"        , POS_STANDING  , do_camp       , 1, 0 },
        { "cackle"      , POS_RESTING   , do_action     , 0, 0 },
        { "charge"      , POS_STANDING  , do_charge     , 0, 0 },
        { "check"       , POS_STANDING  , do_not_here   , 1, 0 },
        { "chuckle"     , POS_RESTING   , do_action     , 0, 0 },
        { "clap"        , POS_RESTING   , do_action     , 0, 0 },
        { "clan"        , POS_SLEEPING  , do_clan       , 1, 0 },
        { "clear"       , POS_DEAD      , do_gen_ps     , 0, SCMD_CLEAR },
        { "close"       , POS_SITTING   , do_gen_door   , 0, SCMD_CLOSE },
        { "cls"         , POS_DEAD      , do_gen_ps     , 0, SCMD_CLEAR },
        { "consider"    , POS_RESTING   , do_consider   , 0, 0 },
        { "color"       , POS_DEAD      , do_color      , 0, 0 },
        { "comfort"     , POS_RESTING   , do_action     , 0, 0 },
        { "comb"        , POS_RESTING   , do_action     , 0, 0 },
        { "commands"    , POS_DEAD      , do_commands   , 0, SCMD_COMMANDS },
        { "compact"     , POS_DEAD      , do_gen_tog    , 0, SCMD_COMPACT },
        { "copy"        , POS_SITTING   , do_copy       , 1, 0 },
        { "cough"       , POS_RESTING   , do_action     , 0, 0 },
        { "credits"     , POS_DEAD      , do_gen_ps     , 0, SCMD_CREDITS },
        { "cringe"      , POS_RESTING   , do_action     , 0, 0 },
        { "cry"         , POS_RESTING   , do_action     , 0, 0 },
        { "ctell"       , POS_SLEEPING  , do_ctell      , 0, 0 },
        { "cuddle"      , POS_RESTING   , do_action     , 0, 0 },
        { "curse"       , POS_RESTING   , do_action     , 0, 0 },
        { "curtsey"     , POS_STANDING  , do_action     , 0, 0 },
        { "courselist"  , POS_DEAD      , do_courselist , LVL_IMMORT, 0 },
        
        { "drop"        , POS_RESTING   , do_drop       , 0, SCMD_DROP },
        { "dance"       , POS_STANDING  , do_action     , 0, 0 },
        { "date"        , POS_DEAD      , do_date       , LVL_IMMORT, SCMD_DATE },
        { "daydream"    , POS_SLEEPING  , do_action     , 0, 0 },
        { "dc"          , POS_DEAD      , do_dc         , LVL_GOD, 0 },
        { "deposit"     , POS_STANDING  , do_not_here   , 1, 0 },
        { "diagnose"    , POS_RESTING   , do_diagnose   , 0, 0 },
        { "discharge"   , POS_STANDING  , do_discharge  , 0, 0 },
        { "disembark"   , POS_STANDING  , do_disembark  , 0, 0 },
        { "dismount"    , POS_STANDING  , do_dismount   , 0, 0 },
        { "display"     , POS_DEAD      , do_display    , 0, 0 },
        { "donate"      , POS_RESTING   , do_drop       , 0, SCMD_DONATE },
        { "drink"       , POS_RESTING   , do_drink      , 0, SCMD_DRINK },
        { "drive"       , POS_STANDING  , do_drive      , 0, 0 },
        { "drool"       , POS_RESTING   , do_action     , 0, 0 },
        
        { "eat"         , POS_RESTING   , do_eat        , 0, SCMD_EAT },
        { "echo"        , POS_SLEEPING  , do_echo       , LVL_IMMORT, SCMD_ECHO },
        { "economy"     , POS_STANDING  , do_economy    , LVL_GOD, 0 },
        { "emote"       , POS_RESTING   , do_echo       , 1, SCMD_EMOTE },
        { ":"           , POS_RESTING   , do_echo       , 1, SCMD_EMOTE },
        { "embrace"     , POS_STANDING  , do_action     , 0, 0 },
        { "embark"      , POS_STANDING  , do_embark     , 0, 0 },
        { "enter"       , POS_STANDING  , do_enter      , 0, 0 },
        { "enchant"     , POS_STANDING  , do_enchant    , 1, 0 },
        { "equipment"   , POS_SLEEPING  , do_equipment  , 0, 0 },
        { "exits"       , POS_RESTING   , do_exits      , 0, 0 },
        { "examine"     , POS_SITTING   , do_examine    , 0, 0 },
        
        { "find"        , POS_SLEEPING  , do_find       , LVL_GOD, 0 },
        { "force"       , POS_SLEEPING  , do_force      , LVL_GOD, 0 },
        { "fart"        , POS_RESTING   , do_action     , 0, 0 },
        { "fill"        , POS_STANDING  , do_pour       , 0, SCMD_FILL },
        { "flee"        , POS_FIGHTING  , do_flee       , 1, 0 },
        { "flip"        , POS_STANDING  , do_action     , 0, 0 },
        { "flirt"       , POS_RESTING   , do_action     , 0, 0 },
        { "follow"      , POS_RESTING   , do_follow     , 0, 0 },
        { "fondle"      , POS_RESTING   , do_action     , 0, 0 },
        { "forget"      , POS_RESTING   , do_forget     , 2, 0 },
        { "freeze"      , POS_DEAD      , do_wizutil    , LVL_FREEZE, SCMD_FREEZE },
        { "french"      , POS_RESTING   , do_action     , 0, 0 },
        { "frown"       , POS_RESTING   , do_action     , 0, 0 },
        { "fume"        , POS_RESTING   , do_action     , 0, 0 },
        
        { "get"         , POS_RESTING   , do_get        , 0, 0 },
        { "gasp"        , POS_RESTING   , do_action     , 0, 0 },
        { "gecho"       , POS_DEAD      , do_gecho      , LVL_GOD, 0 },
        { "give"        , POS_RESTING   , do_give       , 0, 0 },
        { "ginfo"       , POS_STANDING  , do_goodinfo   , LVL_GOD, 0 },
        { "giggle"      , POS_RESTING   , do_action     , 0, 0 },
        { "glare"       , POS_RESTING   , do_action     , 0, 0 },
        { "gold"        , POS_RESTING   , do_gold       , 0, 0 },
        { "goto"        , POS_SLEEPING  , do_goto       , LVL_IMMORT, 0 },
        { "gossip"      , POS_SLEEPING  , do_gen_comm   , 0, SCMD_GOSSIP },
        { "group"       , POS_RESTING   , do_group      , 1, 0 },
        { "grab"        , POS_RESTING   , do_grab       , 0, 0 },
        { "grats"       , POS_SLEEPING  , do_gen_comm   , 0, SCMD_GRATZ },
        { "greet"       , POS_RESTING   , do_greet      , 1, 0 },
        { "grin"        , POS_RESTING   , do_action     , 0, 0 },
        { "groan"       , POS_RESTING   , do_action     , 0, 0 },
        { "grope"       , POS_RESTING   , do_action     , 0, 0 },
        { "grovel"      , POS_RESTING   , do_action     , 0, 0 },
        { "growl"       , POS_RESTING   , do_action     , 0, 0 },
        { "gsay"        , POS_SLEEPING  , do_gsay       , 0, 0 },
        { "gtell"       , POS_SLEEPING  , do_gsay       , 0, 0 },
        
        { "help"        , POS_DEAD      , do_help       , 0, 0 },
        { "handbook"    , POS_DEAD      , do_gen_ps     , LVL_IMMORT, SCMD_HANDBOOK },
        { "hcontrol"    , POS_DEAD      , do_hcontrol   , LVL_GRGOD, 0 },
        { "hiccup"      , POS_RESTING   , do_action     , 0, 0 },
        { "hide"        , POS_RESTING   , do_hide       , 1, 0 },
        { "hit"         , POS_FIGHTING  , do_hit        , 0, SCMD_HIT },
        { "hold"        , POS_RESTING   , do_grab       , 1, 0 },
        { "holler"      , POS_RESTING   , do_gen_comm   , 1, SCMD_HOLLER },
        { "holylight"   , POS_DEAD      , do_gen_tog    , LVL_IMMORT, SCMD_HOLYLIGHT },
        { "hop"         , POS_RESTING   , do_action     , 0, 0 },
        { "house"       , POS_RESTING   , do_house      , 0, 0 },
        { "hug"         , POS_RESTING   , do_action     , 0, 0 },
        
        { "inventory"   , POS_DEAD      , do_inventory  , 0, 0 },
        { "idea"        , POS_DEAD      , do_gen_write  , 0, SCMD_IDEA },
        { "imotd"       , POS_DEAD      , do_gen_ps     , LVL_IMMORT, SCMD_IMOTD },
        { "immlist"     , POS_DEAD      , do_gen_ps     , 0, SCMD_IMMLIST },
        { "info"        , POS_SLEEPING  , do_gen_ps     , 0, SCMD_INFO },
        { "insult"      , POS_RESTING   , do_insult     , 0, 0 },
        { "invis"       , POS_DEAD      , do_invis      , LVL_IMMORT, 0 },
        
        { "junk"        , POS_RESTING   , do_drop       , 0, SCMD_JUNK },
        
        { "kill"        , POS_FIGHTING  , do_kill       , 0, 0 },
        { "kick"        , POS_FIGHTING  , do_kick       , 1, 0 },
        { "kiss"        , POS_RESTING   , do_action     , 0, 0 },
        { "knock"       , POS_STANDING  , do_knock      , 0, 0 },
        { "knowings"    , POS_RESTING   , do_knowings   , 2, 0 },
        
        { "look"        , POS_RESTING   , do_look       , 0, SCMD_LOOK },
        { "laugh"       , POS_RESTING   , do_action     , 0, 0 },
        { "last"        , POS_DEAD      , do_last       , LVL_GOD, 0 },
        { "levels"      , POS_DEAD      , do_levels     , 0, 0 },
        { "list"        , POS_STANDING  , do_not_here   , 0, 0 },
        { "lick"        , POS_RESTING   , do_action     , 0, 0 },
        { "lock"        , POS_SITTING   , do_gen_door   , 0, SCMD_LOCK },
        { "load"        , POS_DEAD      , do_load       , LVL_GOD, 0 },
        { "love"        , POS_RESTING   , do_action     , 0, 0 },
        { "life"        , POS_SLEEPING  , do_life       , LVL_GOD, 0 },
        
        { "memorize"    , POS_RESTING   , do_memorize   , 1, 0 },
        { "market"      , POS_STANDING  , do_market     , 1, 0 },
        { "mkaff"       , POS_STANDING  , do_mkaff      , LVL_GOD, 0 },
        { "moan"        , POS_RESTING   , do_action     , 0, 0 },
        { "motd"        , POS_DEAD      , do_gen_ps     , 0, SCMD_MOTD },
        { "mount"       , POS_STANDING  , do_mount      , 0, 0 },
        { "mail"        , POS_STANDING  , do_not_here   , 1, 0 },
        { "massage"     , POS_RESTING   , do_action     , 0, 0 },
        { "mute"        , POS_DEAD      , do_wizutil    , LVL_GOD, SCMD_SQUELCH },
        { "murder"      , POS_FIGHTING  , do_hit        , 0, SCMD_MURDER },
        
        { "news"        , POS_SLEEPING  , do_gen_ps     , 0, SCMD_NEWS },
        { "negotiate"   , POS_RESTING   , do_negotiate  , 0, 0 },
        { "nibble"      , POS_RESTING   , do_action     , 0, 0 },
        { "nod"         , POS_RESTING   , do_action     , 0, 0 },
        { "noauction"   , POS_DEAD      , do_gen_tog    , 0, SCMD_NOAUCTION },
        { "nogossip"    , POS_DEAD      , do_gen_tog    , 0, SCMD_NOGOSSIP },
        { "nograts"     , POS_DEAD      , do_gen_tog    , 0, SCMD_NOGRATZ },
        { "nogreet"     , POS_DEAD      , do_gen_tog    , 0, SCMD_NOGREET },
        { "nohassle"    , POS_DEAD      , do_gen_tog    , LVL_IMMORT, SCMD_NOHASSLE },
        { "norepeat"    , POS_DEAD      , do_gen_tog    , 0, SCMD_NOREPEAT },
        { "noshout"     , POS_SLEEPING  , do_gen_tog    , 1, SCMD_DEAF },
        { "nosummon"    , POS_DEAD      , do_gen_tog    , 1, SCMD_NOSUMMON },
        { "notell"      , POS_DEAD      , do_gen_tog    , 1, SCMD_NOTELL },
        { "notitle"     , POS_DEAD      , do_wizutil    , LVL_GOD, SCMD_NOTITLE },
        { "nowiz"       , POS_DEAD      , do_gen_tog    , LVL_IMMORT, SCMD_NOWIZ },
        { "nudge"       , POS_RESTING   , do_action     , 0, 0 },
        { "nuzzle"      , POS_RESTING   , do_action     , 0, 0 },
		{ "newspaper"   , POS_DEAD      , do_newspaper  , LVL_GOD, 0 },
        { "newmoney"    , POS_DEAD      , do_newmoney   , LVL_GOD, 0 },
        { "newbook"     , POS_DEAD      , do_newbook    , LVL_GOD, 0 },
        { "newvehicle"  , POS_STANDING  , do_newvehicle , LVL_GOD, 0 },
        { "newgoods"    , POS_STANDING  , do_newgoods   , LVL_GOD, 0 },
        
        { "open"        , POS_SITTING   , do_gen_door   , 0, SCMD_OPEN },
        { "order"       , POS_RESTING   , do_order      , 1, 0 },
        { "offer"       , POS_STANDING  , do_not_here   , 1, 0 },
        { "out"         , POS_STANDING  , do_out        , 0, 0 },
        
        { "put"         , POS_RESTING   , do_put        , 0, 0 },
        { "pat"         , POS_RESTING   , do_action     , 0, 0 },
        { "page"        , POS_DEAD      , do_page       , LVL_GOD, 0 },
        { "pardon"      , POS_DEAD      , do_wizutil    , LVL_GOD, SCMD_PARDON },
        { "peer"        , POS_RESTING   , do_action     , 0, 0 },
        { "pick"        , POS_STANDING  , do_gen_door   , 1, SCMD_PICK },
        { "point"       , POS_RESTING   , do_action     , 0, 0 },
        { "poke"        , POS_RESTING   , do_action     , 0, 0 },
        { "policy"      , POS_DEAD      , do_gen_ps     , 0, SCMD_POLICIES },
        { "politics"    , POS_RESTING   , do_politics   , 0, 0 },
        { "ponder"      , POS_RESTING   , do_action     , 0, 0 },
        { "poofin"      , POS_DEAD      , do_poofset    , LVL_IMMORT, SCMD_POOFIN },
        { "poofout"     , POS_DEAD      , do_poofset    , LVL_IMMORT, SCMD_POOFOUT },
        { "pour"        , POS_STANDING  , do_pour       , 0, SCMD_POUR },
        { "pout"        , POS_RESTING   , do_action     , 0, 0 },
        { "prompt"      , POS_DEAD      , do_display    , 0, 0 },
        { "practice"    , POS_RESTING   , do_practice   , 1, 0 },
        { "pray"        , POS_SITTING   , do_action     , 0, 0 },
        { "puke"        , POS_RESTING   , do_action     , 0, 0 },
        { "punch"       , POS_RESTING   , do_action     , 0, 0 },
        { "purr"        , POS_RESTING   , do_action     , 0, 0 },
        { "purge"       , POS_DEAD      , do_purge      , LVL_GOD, 0 },
        
        { "quaff"       , POS_RESTING   , do_use        , 0, SCMD_QUAFF },
        { "qecho"       , POS_DEAD      , do_qcomm      , LVL_IMMORT, SCMD_QECHO },
        { "quest"       , POS_DEAD      , do_gen_tog    , 0, SCMD_QUEST },
        { "qui"         , POS_DEAD      , do_quit       , 0, 0 },
        { "quit"        , POS_DEAD      , do_quit       , 0, SCMD_QUIT },
        { "qsay"        , POS_RESTING   , do_qcomm      , 0, SCMD_QSAY },
        
        { "reply"       , POS_SLEEPING  , do_reply      , 0, 0 },
        { "rest"        , POS_RESTING   , do_rest       , 0, 0 },
        { "read"        , POS_RESTING   , do_look       , 0, SCMD_READ },
        { "reload"      , POS_DEAD      , do_reboot     , LVL_IMPL, 0 },
        { "recite"      , POS_RESTING   , do_use        , 0, SCMD_RECITE },
        { "receive"     , POS_STANDING  , do_not_here   , 1, 0 },
        { "remove"      , POS_RESTING   , do_remove     , 0, 0 },
        { "remort"      , POS_STANDING  , do_remort     , 1, 0 },
        { "remap"       , POS_DEAD      , do_remap      , LVL_GRGOD, 0 },
        { "rent"        , POS_STANDING  , do_not_here   , 1, 0 },
        { "repair"      , POS_STANDING  , do_repair     , 0, 0 },
        { "report"      , POS_RESTING   , do_report     , 0, 0 },
        { "reroll"      , POS_DEAD      , do_wizutil    , LVL_GRGOD, SCMD_REROLL },
        { "rescue"      , POS_FIGHTING  , do_rescue     , 1, 0 },
        { "restore"     , POS_DEAD      , do_restore    , LVL_GOD, 0 },
        { "resurrect"   , POS_RESTING   , do_not_here   , 0, 0 },
        { "return"      , POS_DEAD      , do_return     , 0, 0 },
        { "roll"        , POS_RESTING   , do_action     , 0, 0 },
        { "roomflags"   , POS_DEAD      , do_gen_tog    , LVL_IMMORT, SCMD_ROOMFLAGS },
        { "ruffle"      , POS_STANDING  , do_action     , 0, 0 },
        
        { "say"         , POS_RESTING   , do_say        , 0, 0 },
        { "'"           , POS_RESTING   , do_say        , 0, 0 },
        { "save"        , POS_SLEEPING  , do_save       , 0, 0 },
        { "score"       , POS_DEAD      , do_score      , 0, 0 },
        { "scan"        , POS_FIGHTING  , do_scan       , 1, 0 },
        { "scream"      , POS_RESTING   , do_action     , 0, 0 },
        { "sell"        , POS_STANDING  , do_not_here   , 0, 0 },
        { "search"      , POS_STANDING  , do_search     , 1, 0 },
        { "send"        , POS_SLEEPING  , do_send       , LVL_GOD, 0 },
        { "set"         , POS_DEAD      , do_set        , LVL_GOD, 0 },
        { "shout"       , POS_RESTING   , do_gen_comm   , 0, SCMD_SHOUT },
        { "shake"       , POS_RESTING   , do_action     , 0, 0 },
        { "ship"        , POS_STANDING  , do_ship       , 0, 0 },
        { "shiver"      , POS_RESTING   , do_action     , 0, 0 },
        { "shoot"       , POS_STANDING  , do_shoot      , 0, 0},
        { "show"        , POS_DEAD      , do_show       , LVL_IMMORT, 0 },
        { "shrug"       , POS_RESTING   , do_action     , 0, 0 },
        { "shutdow"     , POS_DEAD      , do_shutdown   , LVL_IMPL, 0 },
        { "shutdown"    , POS_DEAD      , do_shutdown   , LVL_IMPL, SCMD_SHUTDOWN },
        { "sigh"        , POS_RESTING   , do_action     , 0, 0 },
        { "sing"        , POS_RESTING   , do_action     , 0, 0 },
        { "sip"         , POS_RESTING   , do_drink      , 0, SCMD_SIP },
        { "sit"         , POS_RESTING   , do_sit        , 0, 0 },
        { "skillset"    , POS_SLEEPING  , do_skillset   , LVL_GRGOD, 0 },
        { "sleep"       , POS_SLEEPING  , do_sleep      , 0, 0 },
        { "slap"        , POS_RESTING   , do_action     , 0, 0 },
        { "slowns"      , POS_DEAD      , do_gen_tog    , LVL_IMPL, SCMD_SLOWNS },
        { "smile"       , POS_RESTING   , do_action     , 0, 0 },
        { "smirk"       , POS_RESTING   , do_action     , 0, 0 },
        { "snicker"     , POS_RESTING   , do_action     , 0, 0 },
        { "snap"        , POS_RESTING   , do_action     , 0, 0 },
        { "snarl"       , POS_RESTING   , do_action     , 0, 0 },
        { "sneeze"      , POS_RESTING   , do_action     , 0, 0 },
        { "sneak"       , POS_STANDING  , do_sneak      , 1, 0 },
        { "sniff"       , POS_RESTING   , do_action     , 0, 0 },
        { "snore"       , POS_SLEEPING  , do_action     , 0, 0 },
        { "snowball"    , POS_STANDING  , do_action     , LVL_IMMORT, 0 },
        { "snoop"       , POS_DEAD      , do_snoop      , LVL_GOD, 0 },
        { "snuggle"     , POS_RESTING   , do_action     , 0, 0 },
        { "socials"     , POS_DEAD      , do_commands   , 0, SCMD_SOCIALS },
        { "split"       , POS_SITTING   , do_split      , 1, 0 },
        { "spank"       , POS_RESTING   , do_action     , 0, 0 },
        { "spit"        , POS_STANDING  , do_action     , 0, 0 },
        { "squeeze"     , POS_RESTING   , do_action     , 0, 0 },
        { "stand"       , POS_RESTING   , do_stand      , 0, 0 },
        { "stare"       , POS_RESTING   , do_action     , 0, 0 },
        { "stat"        , POS_DEAD      , do_stat       , LVL_IMMORT, 0 },
        { "steal"       , POS_STANDING  , do_steal      , 1, 0 },
        { "steam"       , POS_RESTING   , do_action     , 0, 0 },
        { "stroke"      , POS_RESTING   , do_action     , 0, 0 },
        { "strut"       , POS_STANDING  , do_action     , 0, 0 },
        { "study"       , POS_SITTING   , do_study      , 1, 0 },
        { "sulk"        , POS_RESTING   , do_action     , 0, 0 },
        { "survey"      , POS_RESTING   , do_survey     , 0, 0 },
        { "switch"      , POS_DEAD      , do_switch     , LVL_GRGOD, 0 },
        { "syslog"      , POS_DEAD      , do_syslog     , LVL_IMMORT, 0 },
        { "shipsetup"   , POS_DEAD      , do_shipsetup  , LVL_IMMORT, 0 },
        { "shipread"    , POS_DEAD      , do_shipread   , LVL_GRGOD, 0 },
        
        { "tell"        , POS_DEAD      , do_tell       , 0, 0 },
        { "tackle"      , POS_RESTING   , do_action     , 0, 0 },
        { "take"        , POS_RESTING   , do_get        , 0, 0 },
        { "tame"        , POS_STANDING  , do_tame       , 0, 0 },
        { "tango"       , POS_STANDING  , do_action     , 0, 0 },
        { "taunt"       , POS_RESTING   , do_action     , 0, 0 },
        { "taste"       , POS_RESTING   , do_eat        , 0, SCMD_TASTE },
        { "teleport"    , POS_DEAD      , do_teleport   , LVL_GOD, 0 },
        { "thank"       , POS_RESTING   , do_action     , 0, 0 },
        { "think"       , POS_RESTING   , do_action     , 0, 0 },
        { "thaw"        , POS_DEAD      , do_wizutil    , LVL_FREEZE, SCMD_THAW },
        { "title"       , POS_DEAD      , do_title      , 0, 0 },
        { "tickle"      , POS_RESTING   , do_action     , 0, 0 },
        { "time"        , POS_DEAD      , do_time       , 0, 0 },
        { "toggle"      , POS_DEAD      , do_toggle     , 0, 0 },
        { "tp"          , POS_STANDING  , do_tp         , LVL_GOD, 0 },
        { "track"       , POS_STANDING  , do_track      , 0, 0 },
        { "trackthru"   , POS_DEAD      , do_gen_tog    , LVL_IMPL, SCMD_TRACK },
        { "transfer"    , POS_SLEEPING  , do_trans      , LVL_GOD, 0 },
        { "travel"      , POS_STANDING  , do_travel     , 0, 0 },
        { "twiddle"     , POS_RESTING   , do_action     , 0, 0 },
        { "typo"        , POS_DEAD      , do_gen_write  , 0, SCMD_TYPO },
		{ "trapremove"  , POS_STANDING  , do_trapremove , 0, 0},
		{ "trapstat"    , POS_STANDING  , do_trapstat	, 0, 0},
        { "traplist"    , POS_DEAD      , do_traplist   , LVL_IMMORT, 0 },
        
        { "unlock"      , POS_SITTING   , do_gen_door   , 0, SCMD_UNLOCK },
        { "ungroup"     , POS_DEAD      , do_ungroup    , 0, 0 },
        { "unban"       , POS_DEAD      , do_unban      , LVL_GRGOD, 0 },
        { "unaffect"    , POS_DEAD      , do_wizutil    , LVL_GOD, SCMD_UNAFFECT },
        { "unyoke"      , POS_STANDING  , do_unyoke     , 0, 0 },
        { "uptime"      , POS_DEAD      , do_date       , LVL_IMMORT, SCMD_UPTIME },
        { "use"         , POS_SITTING   , do_use        , 1, SCMD_USE },
        { "users"       , POS_DEAD      , do_users      , LVL_IMMORT, 0 },
        
        { "value"       , POS_STANDING  , do_not_here   , 0, 0 },
        { "version"     , POS_DEAD      , do_gen_ps     , 0, SCMD_VERSION },
        { "visible"     , POS_RESTING   , do_visible    , 1, 0 },
        { "vlist"       , POS_DEAD      , do_vlist      , LVL_IMMORT, 0 },
        { "vnum"        , POS_DEAD      , do_vnum       , LVL_IMMORT, 0 },
        { "vstat"       , POS_DEAD      , do_vstat      , LVL_IMMORT, 0 },
        
        { "wake"        , POS_SLEEPING  , do_wake       , 0, 0 },
        { "wave"        , POS_RESTING   , do_action     , 0, 0 },
        { "wear"        , POS_RESTING   , do_wear       , 0, 0 },
        { "weather"     , POS_RESTING   , do_weather    , 0, 0 },
        { "who"         , POS_DEAD      , do_who        , 0, 0 },
        { "whoami"      , POS_DEAD      , do_gen_ps     , 0, SCMD_WHOAMI },
        { "where"       , POS_RESTING   , do_where      , 1, 0 },
        { "whisper"     , POS_RESTING   , do_spec_comm  , 0, SCMD_WHISPER },
        { "whine"       , POS_RESTING   , do_action     , 0, 0 },
        { "whistle"     , POS_RESTING   , do_action     , 0, 0 },
        { "wield"       , POS_RESTING   , do_wield      , 0, 0 },
        { "wiggle"      , POS_STANDING  , do_action     , 0, 0 },
        { "wimpy"       , POS_DEAD      , do_wimpy      , 0, 0 },
        { "wink"        , POS_RESTING   , do_action     , 0, 0 },
        { "withdraw"    , POS_STANDING  , do_not_here   , 1, 0 },
        { "wiznet"      , POS_DEAD      , do_wiznet     , LVL_IMMORT, 0 },
        { ";"           , POS_DEAD      , do_wiznet     , LVL_IMMORT, 0 },
        { "wizhelp"     , POS_SLEEPING  , do_commands   , LVL_IMMORT, SCMD_WIZHELP },
        { "wizlist"     , POS_DEAD      , do_gen_ps     , 0, SCMD_WIZLIST },
        { "wizlock"     , POS_DEAD      , do_wizlock    , LVL_IMPL, 0 },
        { "worship"     , POS_RESTING   , do_action     , 0, 0 },
        { "write"       , POS_STANDING  , do_write      , 1, 0 },
        { "wildblack"   , POS_DEAD      , do_gen_tog    , 0, SCMD_WILDBLACK },
        { "wildsmall"   , POS_DEAD      , do_gen_tog    , 0, SCMD_WILDSMALL },
        { "wildtext"    , POS_DEAD      , do_gen_tog    , 0, SCMD_WILDTEXT },
        { "wildlist"    , POS_DEAD      , do_wildlist   , LVL_IMMORT, 0 },
        
        { "yawn"        , POS_RESTING   , do_action     , 0, 0 },
        { "yodel"       , POS_RESTING   , do_action     , 0, 0 },
        { "yoke"        , POS_STANDING  , do_yoke       , 0, 0 },
        
        { "zreset"      , POS_DEAD      , do_zreset     , LVL_GRGOD, 0 },
        { "zoneconn"    , POS_DEAD      , do_zoneconn   , LVL_GRGOD, 0 },

        //{ "objsaveall"        , POS_DEAD      , do_objsaveall, LVL_IMPL, 0 },
        //{ "mobsaveall"        , POS_DEAD      , do_mobsaveall, LVL_IMPL, 0 },
        
        { "\n", 0, 0, 0, 0 }            /* this _must_ be last */
 
};


const char *fill[] =
{
  "in",
  "from",
  "with",
  "the",
  "on",
  "at",
  "to",
  "\n"
};

const char *reserved[] =
{
  "a",
  "an",
  "self",
  "me",
  "all",
  "room",
  "someone",
  "something",
  "\n"
};

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(CHAR_DATA *ch, char *argument)
{
	int cmd, length;
	char *line;
	
	REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);
	
	/* just drop to next line for hitting CR */
	skip_spaces(&argument);
	if (!*argument)
		return;
	
	/*
	 * special case to handle one-character, non-alphanumeric commands;
	 * requested by many people so "'hi" or ";godnet test" is possible.
	 * Patch sent by Eric Green and Stefan Wasilewski.
	 */
	if (!isalpha(*argument))
	{
		arg[0] = argument[0];
		arg[1] = '\0';
		line = argument + 1;
	}
	else
		line = any_one_arg(argument, arg);
	
	/* otherwise, find the command */
	for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
	{
		if (!strncmp(cmd_info[cmd].command, arg, length))
			if (GET_LEVEL(ch) >= cmd_info[cmd].minimum_level)
				break;
	}
	
	if (*cmd_info[cmd].command == '\n')
		send_to_char("Huh?!?\r\n", ch);
	else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL)
		send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
	else if (cmd_info[cmd].command_pointer == NULL)
		send_to_char("Sorry, that command hasn't been implemented yet.\r\n", ch);
	else if (IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IMMORT)
		send_to_char("You can't use immortal commands while switched.\r\n", ch);
	else if (GET_POS(ch) < cmd_info[cmd].minimum_position)
	{
		switch (GET_POS(ch))
		{
		case POS_DEAD:
			send_to_char("Lie still; you are DEAD!!! :-(\r\n", ch);
			break;
		case POS_INCAP:
		case POS_MORTALLYW:
			send_to_char("You are in a pretty bad shape, unable to do anything!\r\n", ch);
			break;
		case POS_STUNNED:
			send_to_char("All you can do right now is think about the stars!\r\n", ch);
			break;
		case POS_SLEEPING:
			send_to_char("In your dreams, or what?\r\n", ch);
			break;
		case POS_RESTING:
			send_to_char("Nah... You feel too relaxed to do that..\r\n", ch);
			break;
		case POS_SITTING:
			send_to_char("Maybe you should get on your feet first?\r\n", ch);
			break;
		case POS_FIGHTING:
			send_to_char("No way!  You're fighting for your life!\r\n", ch);
			break;
		}
	}
	else if (no_specials || !special(ch, cmd, line))
		((*cmd_info[cmd].command_pointer) (ch, line, cmd, cmd_info[cmd].subcmd));
}

/**************************************************************************
 * Routines to handle aliasing                                             *
  **************************************************************************/


ALIAS_DATA *find_alias(ALIAS_DATA *alias_list, char *str)
{
	while (alias_list != NULL)
	{
		if (*str == *alias_list->alias)	/* hey, every little bit counts :-) */
			if (!strcmp(str, alias_list->alias))
				return (alias_list);
			
		alias_list = alias_list->next;
	}
	
	return (NULL);
}


void free_alias(ALIAS_DATA *a)
{
	if (a->alias)
		free(a->alias);
	if (a->replacement)
		free(a->replacement);
	free(a);
}


/* The interface to the outside world: do_alias */
ACMD(do_alias)
{
	char *repl;
	ALIAS_DATA *a, *temp;
	
	if (IS_NPC(ch))
		return;
	
	repl = any_one_arg(argument, arg);
	
	if (!*arg)			/* no argument specified -- list currently defined aliases */
	{
		send_to_char("Currently defined aliases:\r\n", ch);
		if ((a = GET_ALIASES(ch)) == NULL)
			send_to_char(" None.\r\n", ch);
		else
		{
			while (a != NULL)
			{
				sprintf(buf, "%-15s %s\r\n", a->alias, a->replacement);
				send_to_char(buf, ch);
				a = a->next;
			}
		}
	}
	else			/* otherwise, add or remove aliases */
	{
		/* is this an alias we've already defined? */
		if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL)
		{
			REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
			free_alias(a);
		}
		/* if no replacement string is specified, assume we want to delete */
		if (!*repl)
		{
			if (a == NULL)
				send_to_char("No such alias.\r\n", ch);
			else
				send_to_char("Alias deleted.\r\n", ch);
		}
		else			/* otherwise, either add or redefine an alias */
		{
			if (!str_cmp(arg, "alias"))
			{
				send_to_char("You can't alias 'alias'.\r\n", ch);
				return;
			}
			CREATE(a, ALIAS_DATA, 1);
			a->alias = str_dup(arg);
			delete_doubledollar(repl);
			a->replacement = str_dup(repl);
			if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
				a->type = ALIAS_COMPLEX;
			else
				a->type = ALIAS_SIMPLE;
			a->next = GET_ALIASES(ch);
			GET_ALIASES(ch) = a;
			send_to_char("Alias added.\r\n", ch);
		}
	}
}

/*
 * Valid numeric replacements are only $1 .. $9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "$*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(TXT_Q *input_q, char *orig, ALIAS_DATA *a)
{
	TXT_Q temp_queue;
	char *tokens[NUM_TOKENS], *temp, *write_point;
	int num_of_tokens = 0, num;
	
	/* First, parse the original string */
	temp = strtok(strcpy(buf2, orig), " ");
	while (temp != NULL && num_of_tokens < NUM_TOKENS)
	{
		tokens[num_of_tokens++] = temp;
		temp = strtok(NULL, " ");
	}
	
	/* initialize */
	write_point = buf;
	temp_queue.head = temp_queue.tail = NULL;
	
	/* now parse the alias */
	for (temp = a->replacement; *temp; temp++)
	{
		if (*temp == ALIAS_SEP_CHAR)
		{
			*write_point = '\0';
			buf[MAX_INPUT_LENGTH - 1] = '\0';
			write_to_q(buf, &temp_queue, 1);
			write_point = buf;
		}
		else if (*temp == ALIAS_VAR_CHAR)
		{
			temp++;
			if ((num = *temp - '1') < num_of_tokens && num >= 0)
			{
				strcpy(write_point, tokens[num]);
				write_point += strlen(tokens[num]);
			}
			else if (*temp == ALIAS_GLOB_CHAR)
			{
				strcpy(write_point, orig);
				write_point += strlen(orig);
			}
			else if ((*(write_point++) = *temp) == '$')	/* redouble $ for act safety */
				*(write_point++) = '$';
		}
		else
			*(write_point++) = *temp;
	}
	
	*write_point = '\0';
	buf[MAX_INPUT_LENGTH - 1] = '\0';
	write_to_q(buf, &temp_queue, 1);
	
	/* push our temp_queue on to the _front_ of the input queue */
	if (input_q->head == NULL)
		*input_q = temp_queue;
	else
	{
		temp_queue.tail->next = input_q->head;
		input_q->head = temp_queue.head;
	}
}


/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(DESCRIPTOR_DATA *d, char *orig)
{
	char first_arg[MAX_INPUT_LENGTH], *ptr;
	ALIAS_DATA *a, *tmp;
	
	/* Mobs don't have alaises. */
	if (IS_NPC(d->character))
		return (0);
	
	/* bail out immediately if the guy doesn't have any aliases */
	if ((tmp = GET_ALIASES(d->character)) == NULL)
		return (0);
	
	/* find the alias we're supposed to match */
	ptr = any_one_arg(orig, first_arg);
	
	/* bail out if it's null */
	if (!*first_arg)
		return (0);
	
	/* if the first arg is not an alias, return without doing anything */
	if ((a = find_alias(tmp, first_arg)) == NULL)
		return (0);
	
	if (a->type == ALIAS_SIMPLE)
	{
		strcpy(orig, a->replacement);
		return (0);
	}
	else
	{
		perform_complex_alias(&d->input, ptr, a);
		return (1);
	}
}



/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, const char **list, int exact)
{
	int i, l;
	
	/*  We used to have \r as the first character on certain array items to
	 *  prevent the explicit choice of that point.  It seems a bit silly to
	 *  dump control characters into arrays to prevent that, so we'll just
	 *  check in here to see if the first character of the argument is '!',
	 *  and if so, just blindly return a '-1' for not found. - ae.
	 */
	if (*arg == '!')
		return (-1);
	
	/* Make into lower case, and get length of string */
	for (l = 0; *(arg + l); l++)
		*(arg + l) = LOWER(*(arg + l));
	
	if (exact)
	{
		for (i = 0; **(list + i) != '\n'; i++)
			if (!strcmp(arg, *(list + i)))
				return (i);
	}
	else
	{
		if (!l)
			l = 1;			/* Avoid "" to match the first available string */

		for (i = 0; **(list + i) != '\n'; i++)
			if (!strncmp(arg, *(list + i), l))
				return (i);
	}
	
	return (-1);
}


int is_number(const char *str)
{
	while (*str)
		if (!isdigit(*(str++)))
			return (0);
		
	return (1);
}

/*
 * Function to skip over the leading spaces of a string.
 */
void skip_spaces(char **string)
{
	for (; **string && isspace(**string); (*string)++);
}


/*
 * Given a string, change all instances of double dollar signs ($$) to
 * single dollar signs ($).  When strings come in, all $'s are changed
 * to $$'s to avoid having users be able to crash the system if the
 * inputted string is eventually sent to act().  If you are using user
 * input to produce screen output AND YOU ARE SURE IT WILL NOT BE SENT
 * THROUGH THE act() FUNCTION (i.e., do_gecho, do_title, but NOT do_say),
 * you can call delete_doubledollar() to make the output look correct.
 *
 * Modifies the string in-place.
 */
char *delete_doubledollar(char *string)
{
	char *read, *write;
	
	/* If the string has no dollar signs, return immediately */
	if ((write = strchr(string, '$')) == NULL)
		return (string);
	
	/* Start from the location of the first dollar sign */
	read = write;
	
	
	while (*read)   /* Until we reach the end of the string... */
	{
		if ((*(write++) = *(read++)) == '$') /* copy one char */
			if (*read == '$')
				read++; /* skip if we saw 2 $'s in a row */
	}
	
	*write = '\0';
	
	return (string);
}


int fill_word(char *argument)
{
	return (search_block(argument, fill, TRUE) >= 0);
}


int reserved_word(char *argument)
{
	return (search_block(argument, reserved, TRUE) >= 0);
}


/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
	char *begin = first_arg;
	
	if (!argument)
	{
		log("SYSERR: one_argument received a NULL pointer!");
		*first_arg = '\0';
		return (NULL);
	}
	
	do
	{
		skip_spaces(&argument);
		
		first_arg = begin;
		while (*argument && !isspace(*argument))
		{
			*(first_arg++) = LOWER(*argument);
			argument++;
		}
		
		*first_arg = '\0';
	} while (fill_word(begin));
	
	return (argument);
}


/*
 * one_word is like one_argument, except that words in quotes ("") are
 * considered one word.
 */
char *one_word(char *argument, char *first_arg)
{
	char *begin = first_arg;
	
	do
	{
		skip_spaces(&argument);
		
		first_arg = begin;
		
		if (*argument == '\"')
		{
			argument++;
			while (*argument && *argument != '\"')
			{
				*(first_arg++) = LOWER(*argument);
				argument++;
			}
			argument++;
		}
		else
		{
			while (*argument && !isspace(*argument))
			{
				*(first_arg++) = LOWER(*argument);
				argument++;
			}
		}
		
		*first_arg = '\0';
	} while (fill_word(begin));
	
	return (argument);
}


/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
	skip_spaces(&argument);
	
	while (*argument && !isspace(*argument))
	{
		*(first_arg++) = LOWER(*argument);
		argument++;
	}
	
	*first_arg = '\0';
	
	return (argument);
}


/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
	return (one_argument(one_argument(argument, first_arg), second_arg)); /* :-) */
}



/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 *
 * that was dumb.  it shouldn't be symmetrical.  JE 5/1/95
 * 
 * returns 1 if arg1 is an abbreviation of arg2
 */
int is_abbrev(const char *arg1, const char *arg2)
{
	if (!*arg1)
		return (0);
	
	for (; *arg1 && *arg2; arg1++, arg2++)
		if (LOWER(*arg1) != LOWER(*arg2))
			return (0);
		
	if (!*arg1)
		return (1);
	else
		return (0);
}



/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2)
{
	char *temp;
	
	temp = any_one_arg(string, arg1);
	skip_spaces(&temp);
	strcpy(arg2, temp);
}



/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
	int cmd;
	
	for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
		if (!strcmp(cmd_info[cmd].command, command))
			return (cmd);
		
	return (-1);
}


int special(CHAR_DATA *ch, int cmd, char *arg)
{
	OBJ_DATA *i;
	CHAR_DATA *k;
	int j;
	
	/* special in room? */
	if (GET_ROOM_SPEC(ch->in_room) != NULL)
	{
		if (GET_ROOM_SPEC(ch->in_room) (ch, ch->in_room, cmd, arg))
			return (1);
	}
	
	/* special in equipment list? */
	for (j = 0; j < NUM_WEARS; j++)
	{
		if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
			if (GET_OBJ_SPEC(GET_EQ(ch, j)) (ch, GET_EQ(ch, j), cmd, arg))
				return (1);
	}
	
	/* special in inventory? */
	for (i = ch->first_carrying; i; i = i->next_content)
	{
		if (GET_OBJ_SPEC(i) != NULL)
			if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
				return (1);
	}
	
	/* special in mobile present? */
	for (k = ch->in_room->people; k; k = k->next_in_room)
	{
		if (!MOB_FLAGGED(k, MOB_NOTDEADYET))
			if (GET_MOB_SPEC(k) && GET_MOB_SPEC(k) (ch, k, cmd, arg))
				return (1);
	}
	
	/* special in object present? */
	for (i = ch->in_room->first_content; i; i = i->next_content)
	{
		if (GET_OBJ_SPEC(i) != NULL)
			if (GET_OBJ_SPEC(i) (ch, i, cmd, arg))
				return (1);
	}
	
	return (0);
}



/* *************************************************************************
*  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
************************************************************************* */

/*
 * User counter - Muerte - A ButterMud Production 
 * Telnet://betterbox.net:4000
 */
void user_cntr(DESCRIPTOR_DATA *d)
{
	FILE *uc_fp;
	char buf[MAX_STRING_LENGTH];
	long u_cnt = 0;
	
	if ( !(uc_fp = fopen(USERCNT_FILE, "r+b")) )
	{
		if (!(uc_fp = fopen(USERCNT_FILE, "wb")))
			return;
		else
			u_cnt = 1;
	}
	else
	{
		fread(&u_cnt, sizeof (long), 1, uc_fp);
		u_cnt++;
		rewind(uc_fp);
	}
	fwrite(&u_cnt, sizeof (long), 1, uc_fp);
	fclose(uc_fp);
	sprintf(buf, "You are player #%ld to logon since September 20, 2001\r\n", u_cnt);
	SEND_TO_Q(buf, d);
}


int _parse_name(char *arg, char *name)
{
	int i;
	
	/* skip whitespaces */
	for (; isspace(*arg); arg++);
	
	for (i = 0; (*name = *arg); arg++, i++, name++)
		if (!isalpha(*arg))
			return (1);
		
	if (!i)
		return (1);
		
	return (0);
}


#define RECON					1
#define USURP					2
#define UNSWITCH				3

/*
 * XXX: Make immortals 'return' instead of being disconnected when switched
 *      into person returns.  This function seems a bit over-extended too.
 */
int perform_dupe_check(DESCRIPTOR_DATA *d)
{
	DESCRIPTOR_DATA *k, *next_k;
	CHAR_DATA *target = NULL, *ch, *next_ch;
	int mode = 0, id = GET_IDNUM(d->character);
	
	/*
	* Now that this descriptor has successfully logged in, disconnect all
	* other descriptors controlling a character with the same ID number.
	*/
	
	for (k = descriptor_list; k; k = next_k)
	{
		next_k = k->next;
		
		if (k == d)
			continue;
		
		if (k->original && (GET_IDNUM(k->original) == id))    /* switched char */
		{
			SEND_TO_Q("\r\nMultiple login detected -- disconnecting.\r\n", k);
			STATE(k) = CON_CLOSE;
			if (!target)
			{
				target = k->original;
				mode = UNSWITCH;
			}
			if (k->character)
				k->character->desc = NULL;
			k->character = NULL;
			k->original = NULL;
		}
		else if (k->character && (GET_IDNUM(k->character) == id))
		{
			if (!target && STATE(k) == CON_PLAYING)
			{
				SEND_TO_Q("\r\nThis body has been usurped!\r\n", k);
				target = k->character;
				mode = USURP;
			}
			k->character->desc = NULL;
			k->character = NULL;
			k->original = NULL;
			SEND_TO_Q("\r\nMultiple login detected -- disconnecting.\r\n", k);
			STATE(k) = CON_CLOSE;
		}
	}
	
	/*
	 * now, go through the character list, deleting all characters that
	 * are not already marked for deletion from the above step (i.e., in the
	 * CON_HANGUP state), and have not already been selected as a target for
	 * switching into.  In addition, if we haven't already found a target,
	 * choose one if one is available (while still deleting the other
	 * duplicates, though theoretically none should be able to exist).
	 */
	
	for (ch = character_list; ch; ch = next_ch)
	{
		next_ch = ch->next;
		
		if (IS_NPC(ch))
			continue;
		if (GET_IDNUM(ch) != id)
			continue;
		
		/* ignore chars with descriptors (already handled by above step) */
		if (ch->desc)
			continue;
		
		/* don't extract the target char we've found one already */
		if (ch == target)
			continue;
		
		/* we don't already have a target and found a candidate for switching */
		if (!target)
		{
			target = ch;
			mode = RECON;
			continue;
		}
		
		/* we've found a duplicate - blow him away, dumping his eq in limbo. */
		if (ch->in_room != NULL)
			char_from_room(ch);
		char_to_room(ch, get_room(1));
		extract_char(ch);
	}
	
	/* no target for swicthing into was found - allow login to continue */
	if (!target)
		return (0);
	
	/* Okay, we've found a target.  Connect d to target. */
	free_char(d->character); /* get rid of the old char */
	d->character				= target;
	d->character->desc			= d;
	d->original					= NULL;
	d->character->player.timer	= 0;
	REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING);
	REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP);
	STATE(d) = CON_PLAYING;
	
	switch (mode)
	{
	case RECON:
		SEND_TO_Q("Reconnecting.\r\n", d);
		act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
		sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
		break;
	case USURP:
		SEND_TO_Q("You take over your own body, already in use!\r\n", d);
		act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
			"$n's body has been taken over by a new spirit!",
			TRUE, d->character, 0, 0, TO_ROOM);
		sprintf(buf, "%s has re-logged in ... disconnecting old socket.",
			GET_NAME(d->character));
		mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
		break;
	case UNSWITCH:
		SEND_TO_Q("Reconnecting to unswitched char.", d);
		sprintf(buf, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
		break;
	}
	
	return (1);
}



/* deal with newcomers and other non-playing sockets */
void nanny(DESCRIPTOR_DATA *d, char *arg)
{
	ROOM_DATA *lRoom;
	BUILDING_DATA *bld;
	SHIP_DATA *ship;
	char buf[128];
	int player_i, load_result;
	char tmp_name[MAX_INPUT_LENGTH];
	room_vnum load_room;
	
	skip_spaces(&arg);
	
	switch (STATE(d))
	{
	case CON_GET_NAME:		/* wait for input of name */
		if (d->character == NULL)
		{
			CREATE(d->character, CHAR_DATA, 1);
			clear_char(d->character);
			CREATE(d->character->player_specials, PLAYER_SPECIAL, 1);
			d->character->desc = d;
		}

		if (!*arg)
			STATE(d) = CON_CLOSE;
		else
		{
			if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
				strlen(tmp_name) > MAX_NAME_LENGTH || !Valid_Name(tmp_name) ||
				fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) 
			{
				SEND_TO_Q("Invalid name, please try another.\r\nName: ", d);
				return;
			}
			if ((player_i = load_char(tmp_name, d->character)) > -1)
			{
				GET_PFILEPOS(d->character) = player_i;
				
				if (PLR_FLAGGED(d->character, PLR_DELETED))
				{
					/* We get a false positive from the original deleted character. */
					free_char(d->character);
					/* Check for multiple creations... */
					if (!Valid_Name(tmp_name))
					{
						SEND_TO_Q("Invalid name, please try another.\r\nName: ", d);
						return;
					}
					CREATE(d->character, CHAR_DATA, 1);
					clear_char(d->character);
					CREATE(d->character->player_specials, PLAYER_SPECIAL, 1);
					d->character->desc = d;
					CREATE(GET_PC_NAME(d->character), char, strlen(tmp_name) + 1);
					strcpy(GET_PC_NAME(d->character), CAP(tmp_name));
					GET_PFILEPOS(d->character) = player_i;
					sprintf(buf, "Did I get that right, %s (Y/N)? ", tmp_name);
					SEND_TO_Q(buf, d);
					STATE(d) = CON_NAME_CNFRM;
				}
				else
				{
					/* undo it just in case they are set */
					REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING | PLR_MAILING | PLR_CRYO);
					REMOVE_BIT(AFF_FLAGS(d->character), AFF_GROUP);
					SEND_TO_Q("Password: ", d);
					echo_off(d);
					d->idle_tics = 0;
					STATE(d) = CON_PASSWORD;
				}
			}
			else
			{
				/* player unknown -- make new character */
				
				/* Check for multiple creations of a character. */
				if (!Valid_Name(tmp_name))
				{
					SEND_TO_Q("Invalid name, please try another.\r\nName: ", d);
					return;
				}
				CREATE(GET_PC_NAME(d->character), char, strlen(tmp_name) + 1);
				strcpy(GET_PC_NAME(d->character), CAP(tmp_name));
				
				sprintf(buf, "Did I get that right, %s (Y/N)? ", tmp_name);
				SEND_TO_Q(buf, d);
				STATE(d) = CON_NAME_CNFRM;
			}
		}
		break;

	case CON_NAME_CNFRM:		/* wait for conf. of new name    */
		if (UPPER(*arg) == 'Y')
		{
			if (isbanned(d->host) >= BAN_NEW)
			{
				sprintf(buf, "Request for new char %s denied from [%s] (siteban)",
					GET_PC_NAME(d->character), d->host);
				mudlog(buf, NRM, LVL_GOD, TRUE);
				SEND_TO_Q("Sorry, new characters are not allowed from your site!\r\n", d);
				STATE(d) = CON_CLOSE;
				return;
			}
			if (circle_restrict)
			{
				SEND_TO_Q("Sorry, new players can't be created at the moment.\r\n", d);
				sprintf(buf, "Request for new char %s denied from [%s] (wizlock)",
					GET_PC_NAME(d->character), d->host);
				mudlog(buf, NRM, LVL_GOD, TRUE);
				STATE(d) = CON_CLOSE;
				return;
			}
			SEND_TO_Q("New character.\r\n", d);
			sprintf(buf, "Give me a password for %s: ", GET_PC_NAME(d->character));
			SEND_TO_Q(buf, d);
			echo_off(d);
			STATE(d) = CON_NEWPASSWD;
		}
		else if (*arg == 'n' || *arg == 'N')
		{
			SEND_TO_Q("Okay, what IS it, then? ", d);
			STRFREE(GET_PC_NAME(d->character));
			STATE(d) = CON_GET_NAME;
		}
		else
		{
			SEND_TO_Q("Please type Yes or No: ", d);
		}
		break;

	case CON_PASSWORD:		/* get pwd for known player      */
		 /*
		  * To really prevent duping correctly, the player's record should
		  * be reloaded from disk at this point (after the password has been
		  * typed).  However I'm afraid that trying to load a character over
		  * an already loaded character is going to cause some problem down the
		  * road that I can't see at the moment.  So to compensate, I'm going to
		  * (1) add a 15 or 20-second time limit for entering a password, and (2)
		  * re-add the code to cut off duplicates when a player quits.  JE 6 Feb 96
		  */
		
		echo_on(d);    /* turn echo back on */
		
		/* New echo_on() eats the return on telnet. Extra space better than none. */
		SEND_TO_Q("\r\n", d);
		
		if (!*arg)
			STATE(d) = CON_CLOSE;
		else
		{
			if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH))
			{
				sprintf(buf, "Bad PW: %s [%s]", GET_NAME(d->character), d->host);
				mudlog(buf, BRF, LVL_GOD, TRUE);
				GET_BAD_PWS(d->character)++;
				save_char(d->character, NULL);
				if (++(d->bad_pws) >= max_bad_pws)	/* 3 strikes and you're out. */
				{
					SEND_TO_Q("Wrong password... disconnecting.\r\n", d);
					STATE(d) = CON_CLOSE;
				}
				else
				{
					SEND_TO_Q("Wrong password.\r\nPassword: ", d);
					echo_off(d);
				}
				return;
			}
			
			/* Password was correct. */
			load_result = GET_BAD_PWS(d->character);
			GET_BAD_PWS(d->character) = 0;
			d->bad_pws = 0;
			
			if (isbanned(d->host) == BAN_SELECT && !PLR_FLAGGED(d->character, PLR_SITEOK))
			{
				SEND_TO_Q("Sorry, this char has not been cleared for login from your site!\r\n", d);
				STATE(d) = CON_CLOSE;
				sprintf(buf, "Connection attempt for %s denied from %s",
					GET_NAME(d->character), d->host);
				mudlog(buf, NRM, LVL_GOD, TRUE);
				return;
			}
			if (GET_LEVEL(d->character) < circle_restrict)
			{
				SEND_TO_Q("The game is temporarily restricted.. try again later.\r\n", d);
				STATE(d) = CON_CLOSE;
				sprintf(buf, "Request for login denied for %s [%s] (wizlock)",
					GET_NAME(d->character), d->host);
				mudlog(buf, NRM, LVL_GOD, TRUE);
				return;
			}
			/* check and make sure no other copies of this player are logged in */
			if (perform_dupe_check(d))
				return;
			
			if (GET_LEVEL(d->character) >= LVL_IMMORT)
				SEND_TO_Q(imotd, d);
			else
				SEND_TO_Q(motd, d);
			
			user_cntr(d);   /* User Counter - Muerte 10/7/97 */
			
			sprintf(buf, "%s [%s] has connected.", GET_NAME(d->character), d->host);
			mudlog(buf, BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE);
			
			if (load_result)
			{
				sprintf(buf, "\r\n\r\n\007\007\007"
					"%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
					CCRED(d->character, C_SPR), load_result,
					(load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
				SEND_TO_Q(buf, d);
				GET_BAD_PWS(d->character) = 0;
			}
			SEND_TO_Q("\r\n*** PRESS RETURN: ", d);
			STATE(d) = CON_RMOTD;
		}
		break;
		
	case CON_NEWPASSWD:
	case CON_CHPWD_GETNEW:
		if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
			!str_cmp(arg, GET_PC_NAME(d->character)))
		{
			SEND_TO_Q("\r\nIllegal password.\r\n", d);
			SEND_TO_Q("Password: ", d);
			return;
		}
		strncpy(GET_PASSWD(d->character), CRYPT(arg, GET_PC_NAME(d->character)), MAX_PWD_LENGTH);
		*(GET_PASSWD(d->character) + MAX_PWD_LENGTH) = '\0';
		
		SEND_TO_Q("\r\nPlease retype password: ", d);
		if (STATE(d) == CON_NEWPASSWD)
			STATE(d) = CON_CNFPASSWD;
		else
			STATE(d) = CON_CHPWD_VRFY;
		
		break;
		
	case CON_CNFPASSWD:
	case CON_CHPWD_VRFY:
		if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH))
		{
			SEND_TO_Q("\r\nPasswords don't match... start over.\r\n", d);
			SEND_TO_Q("Password: ", d);
			if (STATE(d) == CON_CNFPASSWD)
				STATE(d) = CON_NEWPASSWD;
			else
				STATE(d) = CON_CHPWD_GETNEW;
			return;
		}
		echo_on(d);
		
		if (STATE(d) == CON_CNFPASSWD)
		{
			SEND_TO_Q("\r\nWhat is your sex (M/F)? ", d);
			STATE(d) = CON_QSEX;
		}
		else
		{
			save_char(d->character, NULL);
			SEND_TO_Q("\r\nDone.\r\n", d);
			SEND_TO_Q(MENU, d);
			STATE(d) = CON_MENU;
		}
		
		break;
		
	case CON_QSEX:		/* query sex of new user         */
		switch (*arg)
		{
		case 'm':
		case 'M':
			d->character->player.sex = SEX_MALE;
			break;
		case 'f':
		case 'F':
			d->character->player.sex = SEX_FEMALE;
			break;
		default:
			SEND_TO_Q("That is not a sex..\r\n"
				"What IS your sex? ", d);
			return;
		}
		
		SEND_TO_Q(race_menu, d);
		SEND_TO_Q("\r\nRace: ", d);
		STATE(d) = CON_QRACE;
		break;
		
		case CON_QRACE:
			load_result = parse_race(*arg);
			if (load_result == RACE_UNDEFINED)
			{
				SEND_TO_Q("\r\nThat's not a race.\r\nRace: ", d);
				return;
			}
			else
				GET_RACE(d->character) = load_result;
			
		SEND_TO_Q(class_menu, d);
		SEND_TO_Q("\r\nClass: ", d);
		STATE(d) = CON_QCLASS;
		break;
		
	case CON_QCLASS:
		load_result = parse_class(*arg);
		if (load_result == CLASS_UNDEFINED)
		{
			SEND_TO_Q("\r\nThat's not a class.\r\nClass: ", d);
			return;
		}
		else
			GET_CLASS(d->character) = load_result;
		
		SEND_TO_Q("\r\nPress enter to roll your stats.", d);
		STATE(d) = CON_QROLLSTATS;
		break;
		
	case CON_QROLLSTATS:
		switch (*arg)
		{
		case 'y':
		case 'Y':
			break;
		case 'n':
		case 'N':
		default:
			roll_real_abils(d->character);
			sprintf(buf, "\r\nStr: [%d] Int: [%d] Wis: [%d]"
				" Dex: [%d] Con: [%d] Cha: [%d]",
				GET_STR(d->character), GET_INT(d->character),
				GET_WIS(d->character), GET_DEX(d->character),
				GET_CON(d->character), GET_CHA(d->character));
			SEND_TO_Q(buf, d);
			SEND_TO_Q("\r\n\r\nKeep these stats? (y/N)", d);
			return;
		}

		if (GET_PFILEPOS(d->character) < 0)
			GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));
	
		/* Now GET_NAME() will work properly. */
		/* initialize char */
		init_char(d->character);
		/* save char */
		save_char(d->character, NULL);
		/* rewrite player table */
		save_player_index();

		SEND_TO_Q(motd, d);
		SEND_TO_Q("\r\n*** PRESS RETURN: ", d);
		STATE(d) = CON_RMOTD;

		sprintf(buf, "%s [%s] new player.", GET_NAME(d->character), d->host);
		mudlog(buf, NRM, LVL_IMMORT, TRUE);

		user_cntr(d);   /* User Counter - Muerte 10/7/97 */
		break;
			
	case CON_RMOTD:		/* read CR after printing motd   */
		SEND_TO_Q(MENU, d);
		STATE(d) = CON_MENU;
		break;
			
	case CON_MENU:		/* get selection from main menu  */
		switch (*arg)
		{
		case '0':
			SEND_TO_Q("Goodbye.\r\n", d);
			STATE(d) = CON_CLOSE;
			break;
			
		case '1':
			reset_char(d->character);
			read_aliases(d->character);
			
			if (PLR_FLAGGED(d->character, PLR_INVSTART))
				GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);
			
			/*
			 * We have to place the character in a room before equipping them
			 * or equip_char() will gripe about the person in NOWHERE.
			 */
			bld		= NULL;
			ship	= NULL;
			lRoom	= NULL;
			
			if ( GET_LOADCOORD(d->character) )
			{
				wild_map_load( d->character, GET_LOADCOORD(d->character) );
				lRoom = create_wild_room( GET_LOADCOORD(d->character), FALSE );
			}
			/* load in ship */
			else if ( GET_LOADSHIP(d->character) != NOTHING && GET_LOADROOM(d->character) != NOWHERE )
			{
				ship = find_ship( GET_LOADSHIP(d->character) );
				
				if ( ship )
					lRoom = find_room_ship(ship, GET_LOADROOM(d->character));
			}
			/* load in building */
			else if (GET_LOADBUILDING(d->character) != NOTHING && GET_LOADROOM(d->character) != NOWHERE)
			{
				if ((bld = find_building(GET_LOADBUILDING(d->character))))
					lRoom = find_room_building(bld, GET_LOADROOM(d->character));
			}
			/* normal load */
			else
			{
				if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
					lRoom = get_room(load_room);
			}

			/* If char was saved with NOWHERE, or real_room above failed... */
			if (!lRoom)
			{
				if (GET_LEVEL(d->character) >= LVL_IMMORT)
					lRoom = r_immort_start_room;
				else
					lRoom = r_mortal_start_room;
			}
			
			/* for frozen players, force frozen start room */
			if (PLR_FLAGGED(d->character, PLR_FROZEN))
				lRoom = r_frozen_start_room;
			/* ghost players start into the labyrinth */
			else if (PLR_FLAGGED(d->character, PLR_GHOST))
				lRoom = get_room(10001);
			
			send_to_char(WELC_MESSG, d->character);
			
			/* add to the character list */
			d->character->next = character_list;
			character_list = d->character;
			
			/* put in the room */
			if (bld)
				char_to_building(d->character, bld, lRoom->number);
			else if (ship)
				char_to_ship(d->character, ship, lRoom->number);
			else
				char_to_room(d->character, lRoom);
			
			/* load char objs */
			load_result = LoadCharObj(d->character);
			
			act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);
			STATE(d) = CON_PLAYING;

			if (GET_LEVEL(d->character) == 0)
			{
				do_start(d->character);
				send_to_char(START_MESSG, d->character);
			}
			// char must be saved.. do_start() do it..
			else
				save_char(d->character, NULL);

			// handle those damned campers
			if (PLR_FLAGGED(d->character, PLR_CAMPED))
			{
				REMOVE_BIT(PLR_FLAGS(d->character), PLR_CAMPED);

				if (IS_WILD(lRoom) && !get_room_aff_bitv(lRoom, RAFF_CAMP))
				{
					ROOM_AFFECT *raff;

					/* create, initialize, and link a room-affection node */
					CREATE(raff, ROOM_AFFECT, 1);
					raff->coord		= NULL;
					raff->vroom		= NOWHERE;
					raff->timer		= 2;
					raff->bitvector		= RAFF_CAMP;
					raff->spell		= 0;
					raff->level		= GET_LEVEL(d->character);
					raff->value		= 0;
					raff->text		= "The fire slowly fades and sputters out...\r\n";
					
					CREATE( raff->coord, COORD_DATA, 1 );
					raff->coord->y	= GET_RY(lRoom);
					raff->coord->x	= GET_RX(lRoom);
					
					/* add to the room list */
					raff->next_in_room	= lRoom->affections;
					lRoom->affections	= raff;
					
					/* add to the global list */
					raff->next		= raff_list;
					raff_list		= raff;
				}
			}

			look_at_room(d->character, 0);
			
			if (has_mail(GET_IDNUM(d->character)))
				send_to_char("You have mail waiting.\r\n", d->character);
			/*
			if (load_result == 2)
			{	// rented items lost
				send_to_char("\r\n\007You could not afford your rent!\r\n"
					"Your possesions have been donated to the Salvation Army!\r\n",
					d->character);
			}
			*/

			d->has_prompt = 0;
			break;
			
		case '2':
			if (GET_DDESC(d->character))
			{
				SEND_TO_Q("Old description:\r\n", d);
				SEND_TO_Q(GET_DDESC(d->character), d);
				free(GET_DDESC(d->character));
				GET_DDESC(d->character) = NULL;
			}
			SEND_TO_Q("Enter the new text you'd like others to see when they look at you.\r\n", d);
			SEND_TO_Q("Terminate with a '@' on a new line.\r\n", d);
			d->str = &GET_DDESC(d->character);
			d->max_str = EXDSCR_LENGTH;
			STATE(d) = CON_EXDESC;
			break;
			
		case '3':
			page_string(d, background, 0);
			STATE(d) = CON_RMOTD;
			break;
			
		case '4':
			SEND_TO_Q("\r\nEnter your old password: ", d);
			echo_off(d);
			STATE(d) = CON_CHPWD_GETOLD;
			break;
			
		case '5':
			SEND_TO_Q("\r\nEnter your password for verification: ", d);
			echo_off(d);
			STATE(d) = CON_DELCNF1;
			break;
			
		default:
			SEND_TO_Q("\r\nThat's not a menu choice!\r\n", d);
			SEND_TO_Q(MENU, d);
			break;
	}
    
	break;
    
	case CON_CHPWD_GETOLD:
		if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH))
		{
			echo_on(d);
			SEND_TO_Q("\r\nIncorrect password.\r\n", d);
			SEND_TO_Q(MENU, d);
			STATE(d) = CON_MENU;
		}
		else
		{
			SEND_TO_Q("\r\nEnter a new password: ", d);
			STATE(d) = CON_CHPWD_GETNEW;
		}
		return;
	  
	case CON_DELCNF1:
		echo_on(d);
		if (strncmp(CRYPT(arg, GET_PASSWD(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH))
		{
			SEND_TO_Q("\r\nIncorrect password.\r\n", d);
			SEND_TO_Q(MENU, d);
			STATE(d) = CON_MENU;
		}
		else
		{
			SEND_TO_Q("\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
			  "ARE YOU ABSOLUTELY SURE?\r\n\r\n"
			  "Please type \"yes\" to confirm: ", d);
			STATE(d) = CON_DELCNF2;
		}
		break;
	  
	case CON_DELCNF2:
		if (!strcmp(arg, "yes") || !strcmp(arg, "YES"))
		{
			if (PLR_FLAGGED(d->character, PLR_FROZEN))
			{
				SEND_TO_Q("You try to kill yourself, but the ice stops you.\r\n", d);
				SEND_TO_Q("Character not deleted.\r\n\r\n", d);
				STATE(d) = CON_CLOSE;
				return;
			}
			if (GET_LEVEL(d->character) < LVL_GRGOD)
				SET_BIT(PLR_FLAGS(d->character), PLR_DELETED);
		  
			if ( (player_i = get_ptable_by_name(GET_NAME(d->character))) >= 0 )
				remove_player(player_i);
		  
			sprintf(buf, "Character '%s' deleted!\r\nGoodbye.\r\n", GET_NAME(d->character));
			SEND_TO_Q(buf, d);
			sprintf(buf, "%s (lev %d) has self-deleted.", GET_NAME(d->character), GET_LEVEL(d->character));
			mudlog(buf, NRM, LVL_GOD, TRUE);
			STATE(d) = CON_CLOSE;
			return;
		}
		else
		{
			SEND_TO_Q("\r\nCharacter not deleted.\r\n", d);
			SEND_TO_Q(MENU, d);
			STATE(d) = CON_MENU;
		}
		break;
	  
	/*
	 * It's possible, if enough pulses are missed, to kick someone off
	 * while they are at the password prompt. We'll just defer to let
	 * the game_loop() axe them.
	 */
	case CON_CLOSE:
		break;

	case CON_LINE_INPUT:
		if ( d->callback )
		{
			d->callback(d, arg, d->c_data);
			
			if (--(d->callback_depth) <= 0)
			{
				d->callback		= NULL;		// if the function wasn't chained, clean up
				d->callback_depth	= 0;		// AND wasn't recursive
				d->c_data		= NULL;
				STATE(d)		= CON_PLAYING;
			}
		}
		else
		{
			log("SYSERR: No callback function specified for state CON_LINE_INPUT");
			STATE(d) = CON_PLAYING;
		}
		break;
		
	default:
		log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
			STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
		STATE(d) = CON_DISCONNECT;	/* Safest to do. */
		break;
	}
}

/*
 * Called like so from somewhere that you want to ask a question:
 * line_input(ch->desc, "Do you want to multi-class to warrior (Yes/No)? ",
 *                       parse_multiclass);
 * ONLY USE FROM CON_PLAYING STATE()'s!
 */
void line_input(DESCRIPTOR_DATA *d, const char *prompt, C_FUNC(*callback), void *info)
{
	d->callback		= callback;
	d->callback_depth++;			// Increase depth of possible recursiveness.
	d->c_data		= (char *) info;
	STATE(d)		= CON_LINE_INPUT;
	SEND_TO_Q(prompt, d);
}


