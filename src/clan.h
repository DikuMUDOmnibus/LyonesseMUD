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
*   File: clan.h                                   Part of CircleMUD      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
**************************************************************************/

typedef struct	clan_data			CLAN_DATA;
typedef struct	politics_data_type	CLAN_POLITIC_DATA;

#define MAX_CLANS				20
#define MIN_CLAN_MEMBERS		5
#define MAX_CLAN_MEMBERS		50

#define RANK_INVITED			0
#define RANK_APPLIER			1
#define RANK_MEMBER_FIRST		2

#define MAX_RANK_ADVANCE		6
#define RANK_PRIVILEGES			8

#define RANK_DIPLOMAT			7
#define RANK_MAGISTRATE			8
#define RANK_HERO				9
#define RANK_LEADER				10
#define RANK_PATRON				11

#define LVL_CLAN_GOD			LVL_GOD
#define DEFAULT_APP_LVL			8

#define CLAN_NAME_L				128

#define MAX_CLAN_OBJ			3

#define GET_RANK_NAME(ch)		(clan_rank_table[GET_CLAN_RANK(ch)].title_of_rank[GET_SEX(ch)])

#define CM_DUES					1
#define CM_APPFEE				2

#define CB_DEPOSIT				1
#define CB_WITHDRAW				2

/* Clan class restrictions: used in clan_data.anti_class */
#define CLAN_ANTI_MAGE			(1 << 0)
#define CLAN_ANTI_CLERIC		(1 << 1)
#define CLAN_ANTI_THIEF			(1 << 2)
#define CLAN_ANTI_WARRIOR		(1 << 3)
#define CLAN_ANTI_SORCERER		(1 << 4)

/* Clan race restrictions: used in clan_data.anti_race */
#define CLAN_ANTI_HUMAN			(1 << 0)
#define CLAN_ANTI_ELF			(1 << 1)
#define CLAN_ANTI_DWARF			(1 << 2)

/* Clan others restrictions: used in clan_data.anti_other */
#define CLAN_ANTI_GOOD			(1 << 0)
#define CLAN_ANTI_NEUTRAL		(1 << 1)
#define CLAN_ANTI_EVIL			(1 << 2)
#define CLAN_ANTI_PK			(1 << 3)	/* no Player Killer in the clan			*/
#define CLAN_ANTI_PT			(1 << 4)	/* no Player Thief in the clan			*/
#define CLAN_ONLY_PK			(1 << 5)	/* only Player Killer in the clan		*/
#define CLAN_ONLY_PT			(1 << 6)	/* only Player Thief in the clan		*/
#define CLAN_ANTI_MALE			(1 << 7)
#define CLAN_ANTI_FEMALE		(1 << 8)

/* Clan status: used in clan_data.status */
#define CLAN_FORMING			(1 << 0)	/* less than MIN_CLAN_MEMBERS clannies	*/
#define CLAN_ACTIVE				(1 << 1)	/* normal status of a clan				*/
#define CLAN_INACTIVE			(1 << 2)	/* reached the MAX_CLAN_MEMBERS number	*/
#define CLAN_DISBANDING			(1 << 3)	/* a clan that's going to die			*/
#define CLAN_DEAD				(1 << 4)	/* a dead clan							*/

/* Clan types: used in clan_data.ctype */
#define CLAN_NORMAL				(1 << 0)
#define CLAN_PRIVATE			(1 << 1)	/* a clan that does not accept new members freely	*/
#define CLAN_SECRET				(1 << 2)	/* a secret clan						*/

struct clan_data
{
  CLAN_DATA			*next;
  char				*name;					/* name of the clan						*/
  char				*abbr;					/* abbreviation (MAX 5 CHARS)			*/
  char				*leader;				/* name of the clan leader				*/
  char				*godname;				/* name of the sponsor god				*/
  char				*motto;					/* il motto del clan					*/
  char				*warcry;				/* il grido di guerra del clan			*/
  sh_int			id;						/* unique number of the clan			*/
  sh_int			members;				/* number of members of the clan		*/
  sh_int			magistrates;			/* number of magistrates in the clan	*/
  sh_int			ambassadors;			/* number of diplomats in the clan		*/
  sh_int			heroes;					/* number of heroes in the clan			*/
  sh_int			app_level;				/* min level to join the clan			*/
  sh_int			max_level;				/* max level to be member of the clan	*/
  sh_int			status;					/* status of the clan					*/
  sh_int			ctype;					/* type of clan							*/
  int				anti_class;
  int				anti_race;
  int				anti_other;
  int				power;					/* military power of the clan			*/
  int				influence;				/* political influence of the clan		*/
  int				treasure;				/* ricchezza del clan					*/
  int				app_fee;				/* gold required to join the clan		*/
  int				dues;					/* quota mensile						*/
  int				hall;					/* vnum of main clan building			*/
  obj_vnum			objs[MAX_CLAN_OBJ];		/* vnum of personalized clan objs		*/
  time_t			birth;					/* data di formazione del clan			*/
};


struct politics_data_type
{
  sh_int			diplomacy[MAX_CLANS][MAX_CLANS];
  bool				daily_negotiate_table[MAX_CLANS][MAX_CLANS];
  bool				end_current_state[MAX_CLANS][MAX_CLANS];
};


struct clan_titles
{
  char				*title_of_rank[3];
};

/* functions */
void save_clans(void);
void init_clans(void);
CLAN_DATA *get_clan(int clan_id);
CLAN_DATA *find_clan(char *name);
