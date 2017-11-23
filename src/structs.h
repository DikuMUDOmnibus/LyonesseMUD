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
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD	0x030015 /* Major/Minor/Patchlevel - MMmmPP */

/* preamble *************************************************************/

/*
 * Eventually we want to be able to redefine the below to any arbitrary
 * value.  This will allow us to use unsigned data types to get more
 * room and also remove the '< 0' checks all over that implicitly
 * assume these values. -gg 12/17/99
 */
#define NOWHERE					-1				/* nil reference for rooms				*/
#define NOTHING					-1				/* nil reference for objects			*/
#define NOBODY					-1				/* nil reference for mobiles			*/

#define SPECIAL(name) \
	int (name)(CHAR_DATA *ch, void *me, int cmd, char *argument)

#define EVENTFUNC(name) \
	long (name)(void *event_obj)

#define C_FUNC(name) \
	void (name) (DESCRIPTOR_DATA *d, char *arg, void *info)

/* room-related defines *************************************************/

#define	ROOM_HASH				1021

/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH					0
#define EAST					1
#define SOUTH					2
#define WEST					3
#define UP						4
#define DOWN					5
#define NORTHEAST				6
#define SOUTHEAST				7
#define SOUTHWEST				8
#define NORTHWEST				9

/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK				(1 << 0)		/* a Dark								*/
#define ROOM_DEATH				(1 << 1)		/* b Death trap							*/
#define ROOM_NOMOB				(1 << 2)		/* c MOBs not allowed					*/
#define ROOM_INDOORS			(1 << 3)		/* d Indoors							*/
#define ROOM_PEACEFUL			(1 << 4)		/* e Violence not allowed				*/
#define ROOM_SOUNDPROOF			(1 << 5)		/* f Shouts, gossip blocked				*/
#define ROOM_NOTRACK			(1 << 6)		/* g Track won't go through				*/
#define ROOM_NOMAGIC			(1 << 7)		/* h Magic not allowed					*/
#define ROOM_TUNNEL				(1 << 8)		/* i room for only 1 pers				*/
#define ROOM_PRIVATE			(1 << 9)		/* j Can't teleport in					*/
#define ROOM_GODROOM			(1 << 10)		/* k LVL_GOD+ only allowed				*/
#define ROOM_HOUSE				(1 << 11)		/* l (R) Room is a house				*/
#define ROOM_HOUSE_CRASH		(1 << 12)		/* m (R) House needs saving				*/
#define ROOM_ATRIUM				(1 << 13)		/* n (R) The door to a house			*/
#define ROOM_OLC				(1 << 14)		/* o (R) Modifyable/!compress			*/
#define ROOM_BFS_MARK			(1 << 15)		/* p (R) breath-first srch mrk			*/
#define ROOM_WILD_STATIC		(1 << 16)		/* q (R) static wilderness room			*/
#define ROOM_WILD_REMOVE		(1 << 17)		/* r (R) wild room to be deleted		*/
#define ROOM_SAVEROOM			(1 << 18)		/* s room is a loadroom					*/
#define ROOM_BUILDING_WORKS		(1 << 19)		/* t (R) building works in progress...	*/

/* Room affections: used in room_data.room_affs */
#define RAFF_FOG				(1 << 0)		/* a Wall of Fog						*/
#define RAFF_SHIELD				(1 << 1)		/* b Room Shield						*/
#define RAFF_CAMP  				(1 << 2)		/* c Campsite   						*/

/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR				(1 << 0)		/* a Exit is a door						*/
#define EX_CLOSED				(1 << 1)		/* b The door is closed					*/
#define EX_LOCKED				(1 << 2)		/* c The door is locked					*/
#define EX_PICKPROOF			(1 << 3)		/* d Lock can't be picked				*/
#define EX_FALSE				(1 << 4)		/* e Exit is a NOWHERE leading exit		*/
#define EX_HIDDEN				(1 << 5)		/* f Exit is hidden						*/

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE				0				/* Indoors (no map)						*/
#define SECT_CITY				1	/* " */		/* In a city							*/
#define SECT_FIELD				2	/* v */		/* In a field							*/
#define SECT_FOREST				3	/* f */		/* In a forest							*/
#define SECT_HILLS				4	/* c */		/* In the hills							*/
#define SECT_MOUNTAIN			5	/* m */		/* On a mountain						*/
#define SECT_WATER_SWIM			6				/* Swimmable water (no map)				*/
#define SECT_WATER_NOSWIM		7	/* ~ */		/* Water - need a boat 					*/
#define SECT_UNDERWATER			8				/* Underwater (no map)					*/
#define SECT_FLYING				9				/* Wheee! (no map)						*/
#define SECT_LAND				10	/* b */		/* terra brulla							*/
#define SECT_JUNGLE				11	/* F */		/* foresta fitta						*/
#define SECT_FORESTED_HILL		12	/* C */		/* collina boscosa						*/
#define SECT_MOUNTAIN_PEAK		13	/* M */		/* montagna alta						*/
#define SECT_PLATEAU			14	/* A */		/* altopiano							*/
#define SECT_PLAIN				15	/* p */		/* pianura								*/
#define SECT_REEF				16	/* Z */		/* scogliera							*/
#define SECT_SWAMP				17	/* d */		/* palude								*/
#define SECT_RIVER				18	/* a */		/* fiumi e laghi						*/
#define SECT_SEA				19	/* ~ */		/* mari e oceani						*/
#define SECT_BEACH				20	/* s */		/* spiaggia								*/
#define SECT_ARTIC				21	/* G */		/* tundra artica						*/
#define SECT_ROAD				22	/* : */		/* strade e viottoli					*/
#define SECT_BRIDGE				23	/* = */		/* ponti								*/
#define SECT_COAST				24	/* . */		/* costa								*/
#define SECT_WILD_EXIT			25	/* @ */		/* exit from wilderness					*/
#define SECT_ZONE_BORDER		26	/* I */		/* border for a zone					*/
#define SECT_ZONE_INSIDE		27	/*   */		/* inside a zone						*/
#define SECT_PORT				28	/* H */		/* porto								*/
#define SECT_RIVER_NAVIGABLE	29	/* k */		/* fiume percorribile con navi			*/
#define SECT_FORESTED_MOUNTAIN	30	/* T */		/* montagna boscosa           			*/
#define SECT_SHIP_STERN			31				/* ship stern (no map)					*/
#define SECT_SHIP_DECK			32				/* ship deck (no map)					*/
#define SECT_SHIP_BOW			33				/* ship bow (no map)					*/
#define SECT_SHIP_UNDERDK		34				/* ship underdeck (no map)				*/
#define SECT_SHIP_CABIN			35				/* ship cabin (no map)					*/
#define SECT_SHIP_HOLD			36				/* ship hold (no map)					*/
#define SECT_FERRY_DECK			37				/* ferryboat room (no map)				*/
#define SECT_FORD				38	/* u */		/* guado								*/
#define SECT_DESERT				39	/* D */		/* deserto      						*/
#define SECT_SHALLOWS			40	/* - */		/* shallows								*/

#define MAX_SECT				41


/* char and mob-related defines *****************************************/

/* PC classes */
#define CLASS_UNDEFINED			-1
#define CLASS_MAGIC_USER		0
#define CLASS_CLERIC			1
#define CLASS_THIEF				2
#define CLASS_WARRIOR			3
#define CLASS_SORCERER			4

#define NUM_CLASSES				5				/* This must be the number of classes!! */

/* NPC classes (currently unused - feel free to implement!) */
#define CLASS_OTHER				0
#define CLASS_UNDEAD			1
#define CLASS_HUMANOID			2
#define CLASS_ANIMAL			3
#define CLASS_DRAGON			4
#define CLASS_GIANT				5

/* Races */
#define RACE_UNDEFINED			-1
#define RACE_HUMAN				0
#define RACE_ELF				1
#define RACE_DWARF				2

/* Remort code */
#define MULTI_ROOM				NOWHERE
#define MULTI_LEVEL				(LVL_IMMORT - 1)

/* Stats char should start at after multi */
#define MULTI_HP				20
#define MULTI_MANA				50
#define MULTI_MOVE				100

/* Sex */
#define SEX_NEUTRAL				0
#define SEX_MALE				1
#define SEX_FEMALE				2

/* Positions */
#define POS_DEAD				0				/* dead									*/
#define POS_MORTALLYW			1				/* mortally wounded						*/
#define POS_INCAP				2				/* incapacitated						*/
#define POS_STUNNED				3				/* stunned								*/
#define POS_SLEEPING			4				/* sleeping								*/
#define POS_RESTING				5				/* resting								*/
#define POS_SITTING				6				/* sitting								*/
#define POS_FIGHTING			7				/* fighting								*/
#define POS_STANDING			8				/* standing								*/


/* Player flags: used by char_data.char_player_data.act */
#define PLR_KILLER				(1 << 0)		/* a Player is a player-killer			*/
#define PLR_THIEF				(1 << 1)		/* b Player is a player-thief			*/
#define PLR_FROZEN				(1 << 2)		/* c Player is frozen					*/
#define PLR_DONTSET				(1 << 3)		/* d Don't EVER set (ISNPC bit)			*/
#define PLR_WRITING				(1 << 4)		/* e Player writing (board/mail/olc)	*/
#define PLR_MAILING				(1 << 5)		/* f Player is writing mail				*/
#define PLR_CRASH				(1 << 6)		/* g Player needs to be crash-saved		*/
#define PLR_SITEOK				(1 << 7)		/* h Player has been site-cleared		*/
#define PLR_NOSHOUT				(1 << 8)		/* i Player not allowed to shout/goss	*/
#define PLR_NOTITLE				(1 << 9)		/* j Player not allowed to set title	*/
#define PLR_DELETED				(1 << 10)		/* k Player deleted - space reusable	*/
#define PLR_LOADROOM			(1 << 11)		/* l Player uses nonstandard loadroom	*/
#define PLR_NOWIZLIST			(1 << 12)		/* m Player shouldn't be on wizlist		*/
#define PLR_NODELETE			(1 << 13)		/* n Player shouldn't be deleted		*/
#define PLR_INVSTART			(1 << 14)		/* o Player should enter game wizinvis	*/
#define PLR_CRYO				(1 << 15)		/* p Player is cryo-saved (purge prog)	*/
#define PLR_NOTDEADYET			(1 << 16)		/* q (R) Player being extracted.		*/
#define PLR_GHOST				(1 << 17)		/* r (R) Player is a ghost (after dead)	*/
#define PLR_CAMPED				(1 << 18)		/* s (R) must save mounts and vehicles too*/


/* Mobile flags: used by char_data.char_player_data.act */
#define MOB_SPEC				(1 << 0)		/* a Mob has a callable spec-proc		*/
#define MOB_SENTINEL			(1 << 1)		/* b Mob should not move				*/
#define MOB_SCAVENGER			(1 << 2)		/* c Mob picks up stuff on the ground	*/
#define MOB_ISNPC				(1 << 3)		/* d (R) Automatically set on all Mobs	*/
#define MOB_AWARE				(1 << 4)		/* e Mob can't be backstabbed			*/
#define MOB_AGGRESSIVE			(1 << 5)		/* f Mob auto-attacks everybody nearby	*/
#define MOB_STAY_ZONE			(1 << 6)		/* g Mob shouldn't wander out of zone	*/
#define MOB_WIMPY				(1 << 7)		/* h Mob flees if severely injured		*/
#define MOB_AGGR_EVIL			(1 << 8)		/* i Auto-attack any evil PC's			*/
#define MOB_AGGR_GOOD			(1 << 9)		/* j Auto-attack any good PC's			*/
#define MOB_AGGR_NEUTRAL		(1 << 10)		/* k Auto-attack any neutral PC's		*/
#define MOB_MEMORY				(1 << 11)		/* l remember attackers if attacked		*/
#define MOB_HELPER				(1 << 12)		/* m attack PCs fighting other NPCs		*/
#define MOB_NOCHARM				(1 << 13)		/* n Mob can't be charmed				*/
#define MOB_NOSUMMON			(1 << 14)		/* o Mob can't be summoned				*/
#define MOB_NOSLEEP				(1 << 15)		/* p Mob can't be slept					*/
#define MOB_NOBASH				(1 << 16)		/* q Mob can't be bashed (e.g. trees)	*/
#define MOB_NOBLIND				(1 << 17)		/* r Mob can't be blinded				*/
#define MOB_NOTDEADYET			(1 << 18)		/* s (R) Mob being extracted.			*/
#define MOB_MOUNTABLE			(1 << 19)		/* t Mob can be used as mount			*/
#define MOB_HUNTER				(1 << 20)		/* u Mob can track down characters		*/
#define MOB_DRAFT_ANIMAL		(1 << 21)		/* v mob can be yoked to vehicles		*/
#define MOB_ENCOUNTER			(1 << 22)		/* z (R) Mob is an encounter mob (take care) */

/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF				(1 << 0)		/* a Room descs won't normally be shown	*/
#define PRF_COMPACT				(1 << 1)		/* b No extra CRLF pair before prompts	*/
#define PRF_DEAF				(1 << 2)		/* c Can't hear shouts					*/
#define PRF_NOTELL				(1 << 3)		/* d Can't receive tells				*/
#define PRF_DISPHP				(1 << 4)		/* e Display hit points in prompt		*/
#define PRF_DISPMANA			(1 << 5)		/* f Display mana points in prompt		*/
#define PRF_DISPMOVE			(1 << 6)		/* g Display move points in prompt		*/
#define PRF_AUTOEXIT			(1 << 7)		/* h Display exits in a room			*/
#define PRF_NOHASSLE			(1 << 8)		/* i Aggr mobs won't attack				*/
#define PRF_QUEST				(1 << 9)		/* j On quest							*/
#define PRF_SUMMONABLE			(1 << 10)		/* k Can be summoned					*/
#define PRF_NOREPEAT			(1 << 11)		/* l No repetition of comm commands		*/
#define PRF_HOLYLIGHT			(1 << 12)		/* m Can see in dark					*/
#define PRF_COLOR_1				(1 << 13)		/* n Color (low bit)					*/
#define PRF_COLOR_2				(1 << 14)		/* o Color (high bit)					*/
#define PRF_NOWIZ				(1 << 15)		/* p Can't hear wizline					*/
#define PRF_LOG1				(1 << 16)		/* q On-line System Log (low bit)		*/
#define PRF_LOG2				(1 << 17)		/* r On-line System Log (high bit)		*/
#define PRF_NOAUCT				(1 << 18)		/* s Can't hear auction channel			*/
#define PRF_NOGOSS				(1 << 19)		/* t Can't hear gossip channel			*/
#define PRF_NOGRATZ				(1 << 20)		/* u Can't hear grats channel			*/
#define PRF_ROOMFLAGS			(1 << 21)		/* v Can see room flags (ROOM_x)		*/
#define PRF_WILDTEXT			(1 << 22)		/* w see wild with word instead of graph*/
#define PRF_WILDBLACK			(1 << 23)		/* x see wild symbols in b/w			*/
#define PRF_WILDSMALL			(1 << 24)		/* y see wild reduced to the max		*/
#define PRF_NOGREET				(1 << 25)		/* z Doesn't want to be greeted by others */


/* Affect bits: used in char_data.char_player_data.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_BLIND				(1 << 0)		/* a (R) Char is blind					*/
#define AFF_INVISIBLE			(1 << 1)		/* b Char is invisible					*/
#define AFF_DETECT_ALIGN		(1 << 2)		/* c Char is sensitive to align			*/
#define AFF_DETECT_INVIS		(1 << 3)		/* d Char can see invis chars			*/
#define AFF_DETECT_MAGIC		(1 << 4)		/* e Char is sensitive to magic			*/
#define AFF_SENSE_LIFE			(1 << 5)		/* f Char can sense hidden life			*/
#define AFF_WATERWALK			(1 << 6)		/* g Char can walk on water				*/
#define AFF_SANCTUARY			(1 << 7)		/* h Char protected by sanct.			*/
#define AFF_GROUP				(1 << 8)		/* i (R) Char is grouped				*/
#define AFF_CURSE				(1 << 9)		/* j Char is cursed						*/
#define AFF_INFRAVISION			(1 << 10)		/* k Char can see in dark				*/
#define AFF_POISON				(1 << 11)		/* l (R) Char is poisoned				*/
#define AFF_PROTECT_EVIL		(1 << 12)		/* m Char protected from evil			*/
#define AFF_PROTECT_GOOD		(1 << 13)		/* n Char protected from good			*/
#define AFF_SLEEP				(1 << 14)		/* o (R) Char magically asleep			*/
#define AFF_NOTRACK				(1 << 15)		/* p Char can't be tracked				*/
#define AFF_MEMORIZING			(1 << 16)		/* q (R) char is memorizing a spell		*/
#define AFF_HUNTING 			(1 << 17)		/* r Char/Mob is hunting      			*/
#define AFF_SNEAK				(1 << 18)		/* s Char can move quietly				*/
#define AFF_HIDE				(1 << 19)		/* t Char is hidden						*/
#define AFF_TAMED				(1 << 20)		/* u Char is tamed            			*/
#define AFF_CHARM				(1 << 21)		/* v Char is charmed					*/
#define AFF_PARALYSIS			(1 << 22)		/* w Char is paralyzed					*/
#define AFF_FIRESHIELD			(1 << 23)		/* x Char is surrounded by a fireshield */
#define AFF_REGENERATION		(1 << 24)		/* y Char regen hp faster				*/

#define NUM_AFF_FLAGS			25

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING				0				/* Playing - Nominal state				*/
#define CON_CLOSE				1				/* User disconnect, remove character.	*/
#define CON_GET_NAME			2				/* By what name ..?						*/
#define CON_NAME_CNFRM			3				/* Did I get that right, x?				*/
#define CON_PASSWORD			4				/* Password:							*/
#define CON_NEWPASSWD			5				/* Give me a password for x				*/
#define CON_CNFPASSWD			6				/* Please retype password:				*/
#define CON_QSEX				7				/* Sex?									*/
#define CON_QCLASS				8				/* Class?								*/
#define CON_RMOTD				9				/* PRESS RETURN after MOTD				*/
#define CON_MENU				10				/* Your choice: (main menu)				*/
#define CON_EXDESC				11				/* Enter a new description:				*/
#define CON_CHPWD_GETOLD		12				/* Changing passwd: get old				*/
#define CON_CHPWD_GETNEW		13				/* Changing passwd: get new				*/
#define CON_CHPWD_VRFY			14				/* Verify new password					*/
#define CON_DELCNF1				15				/* Delete confirmation 1				*/
#define CON_DELCNF2				16				/* Delete confirmation 2				*/
#define CON_DISCONNECT			17				/* In-game link loss (leave character)	*/
#define CON_LINE_INPUT			18				/* Question box							*/
#define CON_QRACE				19				/* Race?								*/
#define CON_QROLLSTATS			20				/* Roll stats							*/

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_LIGHT				0
#define WEAR_FINGER_R			1
#define WEAR_FINGER_L			2
#define WEAR_NECK_1				3
#define WEAR_NECK_2				4
#define WEAR_BODY				5
#define WEAR_HEAD				6
#define WEAR_LEGS				7
#define WEAR_FEET				8
#define WEAR_HANDS				9
#define WEAR_ARMS				10
#define WEAR_SHIELD				11
#define WEAR_ABOUT				12
#define WEAR_WAIST				13
#define WEAR_WRIST_R			14
#define WEAR_WRIST_L			15
#define WEAR_WIELD				16
#define WEAR_HOLD				17
#define WEAR_SHOULDERS			18

#define NUM_WEARS				19				/* This must be the # of eq positions!! */


/* object-related defines ********************************************/


/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT				1				/* Item is a light source				*/
#define ITEM_SCROLL				2				/* Item is a scroll						*/
#define ITEM_WAND				3				/* Item is a wand						*/
#define ITEM_STAFF				4				/* Item is a staff						*/
#define ITEM_WEAPON				5				/* Item is a weapon						*/
#define ITEM_FIREWEAPON			6				/* Item is a shooting weapon			*/
/*
 * Legenda campi value[] per ITEM_FIREWEAPON
 *
 * value[0]	= weapon type ( see WEAPON_xxx )
 * value[1]	= range ( 1 - 3 )
 * value[2]	= <unused>
 * value[3]	= <unused>
 */
#define ITEM_MISSILE			7				/* Item is a projectile					*/
/*
 * Legenda campi value[] per ITEM_MISSILE
 *
 * value[0]	= type of ranged missile
 * value[1]	= number of damage dice
 * value[2]	= size of damage dice
 * value[3]	= extra damage
 */
#define ITEM_TREASURE			8				/* Item is a treasure, not gold			*/
#define ITEM_ARMOR				9				/* Item is armor						*/
#define ITEM_POTION				10 				/* Item is a potion						*/
#define ITEM_WORN				11				/* Used for enchanted jewelry..			*/
#define ITEM_OTHER				12				/* Misc object							*/
#define ITEM_TRASH				13				/* Trash - shopkeeps won't buy			*/
#define ITEM_TRAP				14				/* Unimplemented						*/
#define ITEM_CONTAINER			15				/* Item is a container					*/
#define ITEM_NOTE				16				/* Item is note 						*/
#define ITEM_DRINKCON			17				/* Item is a drink container			*/
#define ITEM_KEY				18				/* Item is a key						*/
#define ITEM_FOOD				19				/* Item is food							*/
#define ITEM_MONEY				20				/* Item is money (gold)					*/
#define ITEM_PEN				21				/* Item is a pen						*/
#define ITEM_BOAT				22				/* Item is a boat						*/
#define ITEM_FOUNTAIN			23				/* Item is a fountain					*/
/* used for random item generation */
#define ITEM_ARMOR_SHIELD		24				/* item is a shield						*/
#define ITEM_ARMOR_HELM			25				/* item is a helm						*/
#define ITEM_ARMOR_ARMS			26				/* item is a protecting arms armor		*/
#define ITEM_ARMOR_LEGS			27				/* item is a protecting legs armor		*/
#define ITEM_ARMOR_HANDS		28				/* item is a protecting hands armor		*/
#define ITEM_ARMOR_FEETS		29				/* item is a protecting feets armor		*/
#define ITEM_ARMOR_BELT			30				/* item is a belt						*/
/* end of random item generation dedicated defines */
#define ITEM_FURNITURE			31				/* item is a furniture					*/
/*
 * Legenda campi value[] per ITEM_FURNITURE
 *
 * value[0]	= capacity
 * value[1]	= num of people allowed
 * value[2]	= max pos allowed
 * value[3]	= type of furn
 */
#define ITEM_SPELLBOOK			32				/* item is a spellbook					*/
/*
 * Legenda campi value[] per ITEM_SPELLBOOK
 *
 * value[0]	= type of spellbook
 * value[1]	= <unused>
 * value[2]	= <unused>
 * value[3]	= <unused>
 */
#define ITEM_MISSILE_CONT		33				/* arrow/rocks/bolts container			*/
/*
 * Legenda campi value[] per ITEM_MISSILE_CONT
 *
 * value[0]	= capacity
 * value[1]	= ALWAYS 0 !!!
 * value[2]	= missile types
 * value[3]	= <unused>
 */
#define ITEM_TROPHY				34				/* item is a trophy						*/
#define ITEM_GOODS				35				/* item is a good						*/
/*
 * Legenda campi value[] per ITEM_GOODS
 *
 * value[0]	= vnum of the goods in the goods_table[]
 * value[1]	= vnum of the trading post that sold the goods
 * value[2]	= <unused>
 * value[3]	= <unused>
 */
#define ITEM_MAP				36
/*
 * Legenda campi value[] per ITEM_MAP
 *
 * value[0]	= type of map  (0 = sea course map)
 * value[1]	= vnum of the course
 * value[2]	= <unused>
 * value[3]	= <unused>
 */

#define NUM_ITEM_TYPES			37

/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE			(1 << 0)		/* a Item can be takes					*/
#define ITEM_WEAR_FINGER		(1 << 1)		/* b Can be worn on finger				*/
#define ITEM_WEAR_NECK			(1 << 2)		/* c Can be worn around neck 			*/
#define ITEM_WEAR_BODY			(1 << 3)		/* d Can be worn on body 				*/
#define ITEM_WEAR_HEAD			(1 << 4)		/* e Can be worn on head 				*/
#define ITEM_WEAR_LEGS			(1 << 5)		/* f Can be worn on legs				*/
#define ITEM_WEAR_FEET			(1 << 6)		/* g Can be worn on feet				*/
#define ITEM_WEAR_HANDS			(1 << 7)		/* h Can be worn on hands				*/
#define ITEM_WEAR_ARMS			(1 << 8)		/* i Can be worn on arms				*/
#define ITEM_WEAR_SHIELD		(1 << 9)		/* j Can be used as a shield			*/
#define ITEM_WEAR_ABOUT			(1 << 10)		/* k Can be worn about body 			*/
#define ITEM_WEAR_WAIST 		(1 << 11)		/* l Can be worn around waist 			*/
#define ITEM_WEAR_WRIST			(1 << 12)		/* m Can be worn on wrist 				*/
#define ITEM_WEAR_WIELD			(1 << 13)		/* n Can be wielded						*/
#define ITEM_WEAR_HOLD			(1 << 14)		/* o Can be held						*/
#define ITEM_WEAR_SHOULDER		(1 << 15)		/* p Can be worn on shoulders			*/

/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW				(1 << 0)		/* a Item is glowing					*/
#define ITEM_HUM				(1 << 1)		/* b Item is humming					*/
#define ITEM_NOSAVE				(1 << 2)		/* c Item cannot be saved				*/
#define ITEM_NODONATE			(1 << 3)		/* d Item cannot be donated				*/
#define ITEM_NOINVIS			(1 << 4)		/* e Item cannot be made invis			*/
#define ITEM_INVISIBLE			(1 << 5)		/* f Item is invisible					*/
#define ITEM_MAGIC				(1 << 6)		/* g Item is magical					*/
#define ITEM_NODROP				(1 << 7)		/* h Item is cursed: can't drop			*/
#define ITEM_BLESS				(1 << 8)		/* i Item is blessed					*/
#define ITEM_FAST_MAGIC			(1 << 9)		/* j allow fast mana regen or spell mem */
#define ITEM_IS_SPELLBOOK		(1 << 10)		/* k Item has spellbook data attached	*/
#define ITEM_HAS_SPELLS			(1 << 11)		/* l Item has spell data attached		*/
#define ITEM_HAS_TRAPS 			(1 << 12)		/* m Item has trap data attached		*/
#define ITEM_NODAMAGE			(1 << 13)		/* n Item cannot be damaged				*/
#define ITEM_DONATED			(1 << 14)		/* o Item is a donated items			*/
#define ITEM_UNUSED15			(1 << 15)		/* p                           			*/
#define ITEM_NOSELL				(1 << 16)		/* q Shopkeepers won't touch it			*/
#define ITEM_UNIQUE				(1 << 17)		/* r Item is not in database			*/

/* Anti object flags: used by obj_data.obj_flags.anti_flags */
#define ITEM_ANTI_MORTAL		(1 << 0)		/* a Not usable by non-immortal ppl		*/
#define ITEM_ANTI_GOOD			(1 << 1)		/* b Not usable by good people			*/
#define ITEM_ANTI_EVIL			(1 << 2)		/* c Not usable by evil people			*/
#define ITEM_ANTI_NEUTRAL		(1 << 3)		/* d Not usable by neutral people		*/
#define ITEM_ANTI_MAGIC_USER	(1 << 4)		/* e Not usable by mages				*/
#define ITEM_ANTI_CLERIC		(1 << 5)		/* f Not usable by clerics				*/
#define ITEM_ANTI_THIEF			(1 << 6)		/* g Not usable by thieves				*/
#define ITEM_ANTI_WARRIOR		(1 << 7)		/* h Not usable by warriors				*/
#define ITEM_ANTI_SORCERER		(1 << 8)		/* i Not usable by sorcerers			*/
#define ITEM_ANTI_HUMAN			(1 << 9)		/* j Not usable by humans				*/
#define ITEM_ANTI_ELF			(1 << 10)		/* k Not usable by elves				*/
#define ITEM_ANTI_DWARF			(1 << 11)		/* l Not usable by dwarves				*/

/* types of ranged weapon */
#define WEAPON_BOW				1				/* shoots arrows						*/
#define WEAPON_SLING			2				/* shoots rocks							*/
#define WEAPON_CROSSBOW			3				/* shoots bolts							*/

#define MAX_RANGED_WEAPON		3

/* types of ranged missiles */
#define MISS_ARROW				1				/* bow missile							*/
#define MISS_ROCK				2				/* sling missile						*/
#define MISS_BOLT				3				/* crossbow missile						*/

/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE				0				/* No effect							*/
#define APPLY_STR				1				/* Apply to strength					*/
#define APPLY_DEX				2				/* Apply to dexterity					*/
#define APPLY_INT				3				/* Apply to constitution				*/
#define APPLY_WIS				4				/* Apply to wisdom						*/
#define APPLY_CON				5				/* Apply to constitution				*/
#define APPLY_CHA				6				/* Apply to charisma					*/
#define APPLY_CLASS				7				/* Reserved								*/
#define APPLY_LEVEL				8				/* Reserved								*/
#define APPLY_AGE				9				/* Apply to age							*/
#define APPLY_CHAR_WEIGHT		10				/* Apply to weight						*/
#define APPLY_CHAR_HEIGHT		11				/* Apply to height						*/
#define APPLY_MANA				12				/* Apply to max mana					*/
#define APPLY_HIT				13				/* Apply to max hit points				*/
#define APPLY_MOVE				14				/* Apply to max move points				*/
#define APPLY_GOLD				15				/* Reserved								*/
#define APPLY_EXP				16				/* Reserved								*/
#define APPLY_AC				17				/* Apply to Armor Class					*/
#define APPLY_HITROLL			18				/* Apply to hitroll						*/
#define APPLY_DAMROLL			19				/* Apply to damage roll					*/
#define APPLY_SAVING_PARA		20				/* Apply to save throw: paralz			*/
#define APPLY_SAVING_ROD		21				/* Apply to save throw: rods			*/
#define APPLY_SAVING_PETRI		22				/* Apply to save throw: petrif			*/
#define APPLY_SAVING_BREATH		23				/* Apply to save throw: breath			*/
#define APPLY_SAVING_SPELL		24				/* Apply to save throw: spells			*/

/* Container flags - value[1] */
#define CONT_CLOSEABLE			(1 << 0)		/* a Container can be closed			*/
#define CONT_PICKPROOF			(1 << 1)		/* b Container is pickproof				*/
#define CONT_CLOSED				(1 << 2)		/* c Container is closed				*/
#define CONT_LOCKED				(1 << 3)		/* d Container is locked				*/

/* Types of furniture */
#define FURN_THRONE				1
#define FURN_CHAIR				2
#define FURN_TABLE				3
#define FURN_BOOKCASE			4
#define FURN_BED				5
#define FURN_COUNTER			6
#define FURN_DESK				7

/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER				0
#define LIQ_BEER				1
#define LIQ_WINE				2
#define LIQ_ALE					3
#define LIQ_DARKALE				4
#define LIQ_WHISKY				5
#define LIQ_LEMONADE			6
#define LIQ_FIREBRT				7
#define LIQ_LOCALSPC			8
#define LIQ_SLIME				9
#define LIQ_MILK				10
#define LIQ_TEA					11
#define LIQ_COFFE				12
#define LIQ_BLOOD				13
#define LIQ_SALTWATER			14
#define LIQ_CLEARWATER			15


/* other miscellaneous defines *******************************************/

/* Player conditions */
#define DRUNK					0
#define FULL					1
#define THIRST					2

/* Sun state for Sunlight */
#define SUN_DARK				0
#define SUN_RISE				1
#define SUN_LIGHT				2
#define SUN_SET					3
#define MOON_RISE				4
#define MOON_LIGHT				5
#define MOON_SET				6

/* Moon phases for MoonPhase */
#define MOON_NEW					0
#define MOON_WAXING_CRESCENT		1
#define MOON_WAXING_HALF			2
#define MOON_WAXING_THREE_QUARTERS	3
#define MOON_FULL					4
#define MOON_WANING_THREE_QUARTERS	5
#define MOON_WANING_HALF			6
#define MOON_WANING_CRESCENT		7

/* Seasons */
#define WINTER					0
#define SPRING					1
#define SUMMER					2
#define AUTUMN					3

#define NUM_SEASONS				4

/* ** ITEM TRAPS ************************************************** */
/* trap damage types */
#define TRAP_DAM_NONE			0
#define TRAP_DAM_SLEEP			1
#define TRAP_DAM_TELEPORT		2 
#define TRAP_DAM_FIRE			3
#define TRAP_DAM_COLD			4
#define TRAP_DAM_ACID			5
#define TRAP_DAM_ENERGY			6
#define TRAP_DAM_BLUNT			7
#define TRAP_DAM_PIERCE			8
#define TRAP_DAM_SLASH			9

/* trap triggers */
#define TRAP_ACT_ROOM			(1 << 0)		/* a when ch enter the room				*/
#define TRAP_ACT_OPEN			(1 << 1)		/* b on open							*/
#define TRAP_ACT_CLOSE			(1 << 2)		/* c on close							*/
#define TRAP_ACT_GET			(1 << 3)		/* d on get								*/
#define TRAP_ACT_PUT			(1 << 4)		/* e on put								*/
#define TRAP_ACT_DROP			(1 << 5)		/* f on drop							*/
#define TRAP_ACT_GIVE			(1 << 6)		/* g on give							*/
#define TRAP_ACT_LOOKIN			(1 << 7)		/* f on look inside						*/
#define TRAP_ACT_USE			(1 << 8)		/* h when used (wear, wield, etc.)		*/


/* *************** ROOM TRIGGERS ***********************************************/
/* type of triggered events */
#define TRIG_NONE				0				/* No trigger dude!						*/
#define TRIG_TELEPORT			1				/* TELEPORT Event						*/
#define TRIG_COLLAPSE			2				/* COLLAPSE Event						*/
#define TRIG_EXPLOSION			3				/* EXPLOSION Event						*/
#define TRIG_HEAL				4				/* HEAL Event							*/

#define MAX_TRIG_TYPE			4

/* triggered events are activated by.. */
#define TRIG_WHO_ALL			0				/* Trigger by everybody					*/
#define TRIG_WHO_PC				1				/* Trigger by pc						*/
#define TRIG_WHO_NPC			2				/* Trigger by mob						*/

#define MAX_TRIG_WHO			2

/* action that activate the trigger */
#define TRIG_ACT_ENTER			(1 << 0)		/* a when enter in room					*/
#define TRIG_ACT_EXIT			(1 << 1)		/* b when exit from room				*/
#define TRIG_ACT_CAST			(1 << 2)		/* c when cast a spell					*/
#define TRIG_ACT_FIGHT_START	(1 << 3)		/* d when start a fight					*/
#define TRIG_ACT_FIGHT_END		(1 << 4)		/* e when start a fight					*/
#define TRIG_ACT_GET			(1 << 5)		/* f when get something					*/
#define TRIG_ACT_DROP			(1 << 6)		/* g when drop something				*/
#define TRIG_ACT_REST			(1 << 7)		/* h when rest							*/
#define TRIG_ACT_SLEEP			(1 << 8)		/* i when sleep							*/
#define TRIG_ACT_SPEAK			(1 << 9)		/* j when speak							*/

#define MAX_TRIG_ACT			TRIG_ACT_SPEAK
#define NUM_TRIG_ACT			10

#define MAX_TRIG_DELAY			10


/* other #defined constants **********************************************/

/*
 * **DO**NOT** blindly change the number of levels in your MUD merely by
 * changing these numbers and without changing the rest of the code to match.
 * Other changes throughout the code are required.  See coding.doc for
 * details.
 *
 * LVL_IMPL should always be the HIGHEST possible immortal level, and
 * LVL_IMMORT should always be the LOWEST immortal level.  The number of
 * mortal levels will always be LVL_IMMORT - 1.
 */
#define LVL_IMPL				34
#define LVL_GRGOD				33
#define LVL_GOD					32
#define LVL_IMMORT				31

/* Level of the 'freeze' command */
#define LVL_FREEZE				LVL_GRGOD

#define NUM_OF_EX_DIRS			6				/* number of directions in a room (nsewud)	*/
#define NUM_OF_DIRS				10				/* number of directions in a room (nsewud++++) */
#define MAGIC_NUMBER			(0x06)			/* Arbitrary number that won't be in a string */

/*
 * OPT_USEC determines how many commands will be processed by the MUD per
 * second and how frequently it does socket I/O.  A low setting will cause
 * actions to be executed more frequently but will increase overhead due to
 * more cycling to check.  A high setting (e.g. 1 Hz) may upset your players
 * as actions (such as large speedwalking chains) take longer to be executed.
 * You shouldn't need to adjust this.
 */
#define OPT_USEC				100000			/* 10 passes per second */
#define PASSES_PER_SEC			(1000000 / OPT_USEC)
#define RL_SEC					* PASSES_PER_SEC

#define PULSE_ZONE				(10 RL_SEC)
#define PULSE_MOBILE			(10 RL_SEC)
#define PULSE_VIOLENCE			( 2 RL_SEC)
#define PULSE_AUTOSAVE			(60 RL_SEC)
#define PULSE_IDLEPWD			(15 RL_SEC)
#define PULSE_SANITY			(30 RL_SEC)
#define PULSE_USAGE				(5 * 60 RL_SEC)	 /* 5 mins */
#define PULSE_TIMESAVE			(30 * 60 RL_SEC) /* should be >= SECS_PER_MUD_HOUR	*/
#define PULSE_WILDREM			(30 RL_SEC)
#define PULSE_FERRYBOAT			(60 RL_SEC)

/* Variables for the output buffering system */
#define MAX_SOCK_BUF            (12 * 1024)		/* Size of kernel's sock buf			*/
#define MAX_PROMPT_LENGTH       96				/* Max length of prompt					*/
#define GARBAGE_SPACE			32				/* Space for **OVERFLOW** etc			*/
#define SMALL_BUFSIZE			1024			/* Static output buffer size			*/
/* Max amount of output that can be buffered */
#define LARGE_BUFSIZE			(MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

#define HISTORY_SIZE			5				/* Keep last 5 commands.				*/
#define MAX_STRING_LENGTH		8192
#define MAX_INPUT_LENGTH		256				/* Max length per *line* of input		*/
#define MAX_RAW_INPUT_LENGTH	512				/* Max size of *raw* input				*/
#define MAX_MESSAGES			60
#define MAX_NAME_LENGTH			20				/* Max name length						*/
#define MAX_PWD_LENGTH			10				/* Max password length					*/
#define MAX_TITLE_LENGTH		80				/* Max title length						*/
#define HOST_LENGTH				30
#define EXDSCR_LENGTH			240
#define MAX_TONGUE				3				/* Max number of tongue					*/
#define MAX_SPELLS				500				/* Max number of spells					*/
#define MAX_SKILLS				800				/* Max number of skills					*/
#define MAX_AFFECT				32				/* Max character affection				*/
#define MAX_OBJ_AFF				6				/* Max object affection					*/

/*
 * A MAX_PWD_LENGTH of 10 will cause BSD-derived systems with MD5 passwords
 * and GNU libc 2 passwords to be truncated.  On BSD this will enable anyone
 * with a name longer than 5 character to log in with any password.  If you
 * have such a system, it is suggested you change the limit to 20.
 *
 * Please note that this will erase your player files.  If you are not
 * prepared to do so, simply erase these lines but heed the above warning.
 */
#if defined(HAVE_UNSAFE_CRYPT) && MAX_PWD_LENGTH == 10
#error You need to increase MAX_PWD_LENGTH to at least 20.
#error See the comment near these errors for more explanation.
#endif

/**********************************************************************
* Structures                                                          *
**********************************************************************/


typedef signed char			sbyte;
typedef unsigned char		ubyte;
typedef signed short int	sh_int;
typedef unsigned short int	ush_int;
#if !defined(__cplusplus)	/* Anyone know a portable method? */
typedef char				bool;
#endif

#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)	/* Hm, sysdep.h? */
typedef char				byte;
#endif

/* Various virtual (human-reference) number types. */
typedef int	room_vnum;
typedef int	obj_vnum;
typedef int	mob_vnum;
typedef int	zone_vnum;
typedef int	shop_vnum;

/* Various real (array-reference) number types. */
typedef int	obj_rnum;
typedef int	mob_rnum;
typedef int	zone_rnum;
typedef int	shop_rnum;


/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 * 'unsigned long long' will give you at least 64 bits if you have GCC.
 *
 * Since we don't want to break the pfiles, you'll have to search throughout
 * the code for "bitvector_t" and change them yourself if you'd like this
 * extra flexibility.
 */
typedef unsigned long int	bitvector_t;

/* ======================================================================= */

typedef struct	coord_data				COORD_DATA;
typedef struct	extra_descr_data		EXTRA_DESCR;

typedef struct	obj_flag_data			OBJ_FLAG_DATA;
typedef struct	obj_affected_type		OBJ_AFFECTED;
typedef struct	obj_spells_data			OBJ_SPELLS_DATA;
typedef struct	obj_trap_data			OBJ_TRAP_DATA;
typedef struct	obj_data				OBJ_DATA;

typedef struct	terrain_data			TERRAIN_DATA;

typedef struct	room_direction_data		EXIT_DATA;
typedef struct	room_affection_data		ROOM_AFFECT;
typedef struct	room_extra_data			ROOM_EXTRA;
typedef struct	room_trigger_data		TRIGGER_DATA;
typedef struct	room_data				ROOM_DATA;

typedef struct	memory_rec_struct		MEMORY_REC;
typedef struct	time_info_data			TIME_INFO_DATA;
typedef struct	time_data				TIME_DATA;

typedef struct	char_player_data		PLAYER_DATA;
typedef struct	char_ability_data		CHAR_ABIL_DATA;
typedef struct	char_point_data			CHAR_POINT_DATA;
typedef struct	known_data				KNOWN_DATA;
typedef struct	known_course			KNOWN_COURSE;
typedef struct	player_special_data		PLAYER_SPECIAL;
typedef struct	mob_special_data		MOB_SPECIAL;
typedef struct	affected_type			AFFECTED_DATA;
typedef struct	follow_type				FOLLOW_DATA;
typedef struct	char_data				CHAR_DATA;

typedef struct	txt_block				TXT_BLOCK;
typedef struct	txt_q					TXT_Q;
typedef struct	descriptor_data			DESCRIPTOR_DATA;

typedef struct	msg_type				MSG_TYPE;
typedef struct	message_type			MESSAGE_TYPE;
typedef struct	message_list			MESSAGE_LIST;

typedef struct	weather_data			WEATHER_DATA;
typedef struct	weapon_prof_data		WEAPON_PROF;
typedef struct	index_data				INDEX_DATA;
typedef struct	who_list				WHO_LIST;

typedef struct	q_element_data			Q_ELEM_DATA;
typedef struct	queue_data				QUEUE_DATA;
typedef struct	event_data				EVENT_DATA;

typedef struct	book_page_data			BOOK_PAGE;
typedef struct	spellbook_data			SPELLBOOK;
typedef struct	book_type_data			BOOK_TYPE;

typedef struct	wild_remove				WILD_REMOVE;
typedef struct	life_data				LIFE_DATA;
typedef struct	wild_data				WILD_DATA;
typedef struct	survey_data				SURVEY_DATA;
typedef struct	pstone_data				PSTONE_DATA;

typedef struct	yoke_data				YOKE_DATA;
typedef struct	vehicle_points_data		VEHICLE_PTS;
typedef struct	vehicle_index			VEHICLE_INDEX;
typedef struct	vehicle_data			VEHICLE_DATA;
typedef struct	stable_rent_data		STABLE_RENT;

typedef struct	auth_data				AUTH_DATA;

typedef struct	building_points_data	BUILDING_PTS;
typedef struct	building_commands		BUILDING_CMD;
typedef struct	building_type_data		BUILDING_TYPE;
typedef struct	building_works			BLD_WORKS;
typedef struct	building_data			BUILDING_DATA;

typedef struct	ship_value_data			SHIP_VAL_DATA;
typedef struct	ship_type_data			SHIP_TYPE;
typedef struct	ship_data				SHIP_DATA;
typedef struct	port_data				PORT_DATA;
typedef struct	course_data				COURSE_DATA;
typedef struct	course_step				COURSE_STEP;
typedef struct	ferry_data				FERRY_DATA;

typedef struct	good_type_data			GOOD_TYPE;
typedef struct	good_data				GOOD_DATA;
typedef struct	trading_post_data		TRADING_POST;
typedef struct	trp_good_data			TRP_GOOD;
typedef struct	market_vars				MARKET_VARS;
typedef struct	market_affections		MARKET_AFF;
typedef struct	market_data				MARKET_DATA;
typedef struct	market_good_data		MARKET_GOOD;


/* db.h */
typedef struct	reset_com				RESET_COM;
typedef struct	zone_data				ZONE_DATA;
typedef struct	zone_wild_data			ZONE_WILD;
typedef struct	reset_q_element			RESET_Q_ELEM;
typedef struct	reset_q_type			RESET_Q_TYPE;
typedef struct	player_index_element	PLR_INDEX_ELEM;
typedef struct	help_index_element		HELP_INDEX_ELEM;
typedef struct	ban_list_element		BAN_LIST_ELEM;

/* interpreter.h */
typedef struct	command_info			COMMAND_INFO;
typedef struct	alias_data				ALIAS_DATA;

/* shop.h */
typedef struct	stack_data				STACK_DATA;

/* In Spells.h */
typedef struct	spell_info_type			SPELL_INFO_DATA;
typedef struct	attack_hit_type			ATTACK_HIT_TYPE;

/* house.h */
typedef struct house_control_rec		HOUSE_CONTROL;

/* ======================================================================= */

/* Coordinate structure: for wilderness usage */
struct coord_data
{
  sh_int		x;
  sh_int		y;
  sh_int		level;
};


/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data
{
  EXTRA_DESCR		*next;						/* Next in list							*/
  char				*keyword;					/* Keyword in look/examine				*/
  char				*description;				/* What to see							*/
};


/* object-related structures ******************************************/


/* object flags; used in obj_data */
struct obj_flag_data
{
  byte				type_flag;					/* Type of item							*/
  int				value[4];					/* Values of the item (see list)		*/
  int 				wear_flags;					/* Where you can wear it				*/
  int 				extra_flags;				/* If it hums, glows, etc.				*/
  int				anti_flags;					/* anti-mage, anti-cleric, anti-evil,etc*/
  int				weight;						/* Weigt what else						*/
  int				cost;						/* Value when sold (gp.)				*/
  int				cost_per_day;				/* Cost to keep pr. real day			*/
  int				timer;						/* Timer for object						*/
  int				level;						/* Level of item						*/
  int				cond;						/* Current item condition				*/
  int				maxcond;					/* Max item condition					*/
  int				quality;					/* Item quality							*/
  long 				bitvector;					/* To set chars bits					*/
  long				owner_id;					/* who is the owner of the object?		*/
};


/* Object affection */
struct obj_affected_type
{
  byte				location;					/* Which ability to change (APPLY_XXX)	*/
  sbyte				modifier;					/* How much it changes by				*/
};


/* Spells attached to items */
struct obj_spells_data
{
  OBJ_SPELLS_DATA	*next;						/* link to the next node				*/
  sh_int			spellnum;					/* number of the spell					*/
  sh_int			level;						/* level at which it will be cast		*/
  sh_int			percent;					/* % of success							*/
};

/* for Trapped Items */
struct obj_trap_data
{
  OBJ_TRAP_DATA		*next;						/* link to the next node				*/
  bool				whole_room;					/* Just ch or the whole room?			*/
  int				action;						/* Action that trigger the trap			*/
  int				dam_type;					/* Which damage cause the trap			*/
  int				charges;					/* How many times the trap act			*/
};


/* ================== Memory Structure for Objects ================== */
struct obj_data
{
  OBJ_DATA			*next;						/* For the object list					*/
  OBJ_DATA			*prev;						/* For the object list					*/
  OBJ_DATA			*next_content;				/* For 'contains' lists					*/
  OBJ_DATA			*prev_content;				/* For 'contains' lists					*/
  OBJ_DATA			*first_content;				/* Contains objects						*/
  OBJ_DATA			*last_content;				/* Contains objects						*/
  OBJ_DATA			*in_obj;					/* In what object NULL when none		*/
  EXTRA_DESCR		*ex_description;			/* extra descriptions					*/
  ROOM_DATA			*in_room;					/* In what room -1 when conta/carr		*/
  CHAR_DATA			*carried_by;				/* Carried by :NULL in room/conta		*/
  CHAR_DATA			*worn_by;					/* Worn by?								*/
  CHAR_DATA			*people;					/* List of people using a furniture obj	*/
  VEHICLE_DATA		*in_vehicle;				/* In which vehicle?					*/
  EVENT_DATA		*action;					/* event for action which takes time	*/
  OBJ_FLAG_DATA		obj_flags;					/* Object information					*/
  OBJ_AFFECTED		affected[MAX_OBJ_AFF];		/* affects								*/
  byte				light;						/* Number of lightsources for furniture	*/
  char				*name;						/* Title of object :get etc.			*/
  char				*description;				/* When in room							*/
  char				*short_description;			/* when worn/carry/in cont.				*/
  char				*action_description;		/* What to write when used				*/
  sh_int			worn_on;					/* Worn where?							*/
  int				count;						/* number of same objects				*/
  obj_vnum			item_number;				/* Where in data-base					*/
  void				*special;					/* for special data attached on items	*/
};
/* ======================================================================= */


/* room-related structures ************************************************/

/* ex sector_type */
struct terrain_data
{
  char				*name;						/* Sector name							*/
  char				*description;				/* Sector description (when entered)	*/
  char				*map;						/* normal map (with symbols)			*/
  char				*map2;						/* map with letters (aAnmvb etc)		*/
  char				*mapbw;						/* map for black and white				*/
  sh_int			movement_loss;				/* cost in movement points				*/
};

/* room's exits data */
struct room_direction_data
{
  EXIT_DATA			*next;						/* next exit in room linked list		*/
  EXIT_DATA			*prev;						/* prev exit in room linked list		*/
  EXIT_DATA			*rexit;						/* Reverse exit pointer					*/
  ROOM_DATA			*to_room;					/* Where direction leads (NULL)			*/
  COORD_DATA		*coord;						/* for wilderness use					*/
  COORD_DATA		*rvcoord;					/* for wilderness use					*/
  char				*description;				/* When look DIR.						*/
  char				*keyword;					/* for open/close						*/
  sh_int			exit_info;					/* Exit info							*/
  int				vdir;						/* Physical "direction"					*/
  obj_vnum			key;						/* Key's number (-1 for no key)			*/
  room_vnum			vnum;						/* VNum of destination					*/
  room_vnum			rvnum;						/* Vnum of room in opposite dir			*/
};

/* room's affections data */
struct room_affection_data
{
  ROOM_AFFECT		*next;
  ROOM_AFFECT		*next_in_room;
  COORD_DATA		*coord;						/* coord of the room (for wild usage)	*/
  room_vnum			vroom;						/* vnum of the room						*/
  char				*text;						/* some useful spot						*/
  int				timer;						/* how many ticks this affection lasts	*/
  int				bitvector;					/* Tells which bits to set (RAFF_xxx)	*/
  int				spell;						/* the spell number						*/
  int				level;						/* level of the spell					*/
  int				value;						/* some useful spot						*/
};

/* extra data (mainly for buildings) */
struct room_extra_data
{
  int				vnum;						/* building/ship vnum					*/
  sh_int			max_hp;						/* max room hp							*/
  sh_int			curr_hp;					/* curr room hp							*/
};

/* room triggered events data */
struct room_trigger_data
{
  TRIGGER_DATA		*next;						/* link to the next global node			*/
  bool				active;						/* is trigger active?					*/
  char				*text;						/* some useful spot						*/
  int				type;						/* type of trigger						*/
  int				whotrig;					/* who trig the event?					*/
  int				action;						/* triggered by which action?			*/
  int				timer;						/* delay for event 						*/
  int				random;						/* event is sure or has random % ?		*/
  int				value[4];					/* see room_trigger.c					*/
};


/* ================== Memory Structure for room ======================= */
struct room_data
{
  ROOM_DATA			*next;
  COORD_DATA		*coord;						/* wilderness							*/
  OBJ_DATA			*first_content;				/* Head of list of items in room		*/
  OBJ_DATA			*last_content;				/* Tail of list of items in room		*/
  CHAR_DATA			*people;					/* List of NPC / PC in room				*/
  EXTRA_DESCR		*ex_description;			/* for examine/look						*/
  EXIT_DATA			*first_exit;				/* Directions							*/
  EXIT_DATA			*last_exit;					/* Directions							*/
  BUILDING_DATA		*buildings;					/* list of buildings in room			*/
  VEHICLE_DATA		*vehicles;					/* list of vehicles in room				*/
  SHIP_DATA			*ships;						/* List of Ships in room				*/
  FERRY_DATA		*ferryboat;					/* one single ferryboat in each room	*/
  ROOM_AFFECT		*affections;				/* list of room affections				*/
  EVENT_DATA		*action;					/* event for action which takes time	*/
  PSTONE_DATA		*portal_stone;				/* a single portal stone in each room	*/
  ROOM_EXTRA		*extra_data;				/* for ship/building rooms				*/
  TRIGGER_DATA		*trigger;					/* For triggered events					*/
  byte				light;						/* Number of lightsources in room		*/
  char				*name;						/* Rooms name 'You are ...'				*/
  char				*description;				/* Shown when entered					*/
  int				sector_type;				/* sector type (move/hide)				*/
  int				room_flags;					/* DEATH,DARK ... etc					*/
  int				room_affs;					/* Room affections						*/
  room_vnum			number;						/* Rooms number	(vnum)					*/
  zone_rnum			zone;						/* Room zone (for resetting)			*/
  SPECIAL(*func);								/* Special procedure					*/
};

/* ====================================================================== */


/* char-related structures ************************************************/

/* memory structure for characters */
struct memory_rec_struct
{
  MEMORY_REC		*next;
  long				id;
};

/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data
{
  int				hours;
  int				day;
  int				month;
  sh_int			year;
};

/* These data contain information about a players time data */
struct time_data
{
  time_t			birth;			/* This represents the characters age                */
  time_t			logon;			/* Time of the last logon (used to calculate played) */
  int				played;			/* This is the total accumulated time played in secs */
};

/* general player-related info, usually PC's and NPC's */
struct char_player_data
{
  CHAR_DATA			*fighting;					/* Opponent								*/
  CHAR_DATA			*riding;					/* PC/NPC is Mounted					*/
  CHAR_DATA			*hunting;					/* Char hunted by this char				*/
  VEHICLE_DATA		*drive_vehicle;				/* Is driving a vehicle?				*/
  TIME_DATA			time;						/* PC's AGE in days						*/
  byte				sex;						/* PC / NPC's sex						*/
  byte				chclass;					/* PC / NPC's class						*/
  byte				race;						/* PC / NPC's race						*/
  byte				level;						/* PC / NPC's level						*/
  byte				position;					/* Standing, fighting, sleeping, etc.	*/
  byte				carry_items;				/* Number of items carried				*/
  sbyte				clan;						/* PC's clan vnum						*/
  sbyte				clan_rank;					/* PC's clan rank 						*/
  ubyte				weight;						/* PC / NPC's weight					*/
  ubyte				height;						/* PC / NPC's height					*/
  char				*name;						/* PC / NPC s name (kill ...  )			*/
  char				*short_descr;				/* for NPC 'actions'					*/
  char				*long_descr;				/* for 'look'							*/
  char				*description;				/* Extra descriptions					*/
  char				*title;						/* PC / NPC's title						*/
  char				passwd[MAX_PWD_LENGTH+1];	/* character's password					*/
  sh_int			apply_saving_throw[5];		/* Saving throw (Bonuses)				*/
  int				hometown;					/* PC s Hometown (zone)					*/
  int				carry_weight;				/* Carried weight						*/
  int				timer;						/* Timer for update						*/
  int				alignment;					/* +-1000 for alignments                */
  long				idnum;						/* player's idnum; -1 for mobiles		*/
  long				act;						/* act flag for NPC's; player flag for PC's */
  long				affected_by;				/* Bitvector for spells/skills affected by */
};

/* Char's abilities */
struct char_ability_data
{
  sh_int			str;
  sh_int			intel;
  sh_int			wis;
  sh_int			dex;
  sh_int			con;
  sh_int			cha;
};

/* Char's points */
struct char_point_data
{
  sh_int			mana;
  sh_int			max_mana;					/* Max mana for PC/NPC					*/
  sh_int			hit;
  sh_int			max_hit;					/* Max hit for PC/NPC					*/
  sh_int			move;
  sh_int			max_move;					/* Max move for PC/NPC					*/
  sh_int			armor;						/* Internal -100..100, ext. -10..10 AC	*/
  int				bank_gold;					/* Gold the char has in a bank account	*/
  int				exp;						/* The total experience of the player	*/
  sh_int			hitroll;					/* Any bonus or penalty to the hit roll	*/
  sh_int			damroll;					/* Any bonus or penalty to the dam roll	*/
};

/* introduction code */
struct known_data
{
  KNOWN_DATA		*next;						/* next node in list					*/
  KNOWN_DATA		*prev;						/* prev node in list					*/
  char				*name;						/* by which name he's known				*/
  long				idnum;						/* idnum of the known player			*/
};

struct known_course
{
  KNOWN_COURSE		*next;
  int				coursenum;
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs.
 */
struct player_special_data
{
  ALIAS_DATA		*aliases;					/* Character's aliases					*/
  KNOWN_DATA		*first_known;				/* introduction code					*/
  KNOWN_DATA		*last_known;				/* introduction code					*/
  KNOWN_COURSE		*courses;					/* known sail courses					*/
  COORD_DATA		*load_coord;				/* Which coord to place char in			*/
  char				*poofin;					/* Description on arrival of a god.     */
  char				*poofout;					/* Description upon a god's exit.       */
  char				*host;						/* player host							*/
  char				**wildmap;					/* player wild map						*/
  bool				talks[MAX_TONGUE];			/* PC s Tongues 0 for NPC				*/
  bool				athelm;						/* Is manouvering a ship				*/
  byte				skills[MAX_SKILLS+1];		/* array of skills plus skill 0			*/
  byte				spells_mem[MAX_SPELLS+1];	/* array of memorized spells			*/
  byte				PADDING0;					/* used to be spells_to_learn			*/
  byte				freeze_level;				/* Level of god who froze char, if any	*/
  sbyte				conditions[3];				/* Drunk, full, thirsty					*/
  ubyte				bad_pws;					/* number of bad password attemps		*/
  sh_int			invis_level;				/* level of invisibility				*/
  int				wimp_level;					/* Below this # of hit points, flee!	*/
  int				spells_to_learn;			/* How many can you learn yet this level*/
  int				mob_kills;					/* how many mobs has killed?			*/
  int				mob_deaths;					/* how many times has been killed by mobs?*/
  int				plr_kills;					/* how many players has killed?			*/
  int				plr_deaths;					/* how many times has been killed by plr?*/
  int				load_building;				/* Which building to place char in		*/
  int				load_ship;					/* which ship to place char in			*/
  int				tot_level;					/* total level							*/
  long				multiclass;					/* bitvector of completed classes		*/
  long				last_tell;					/* idnum of last tell from				*/
  long				pref;						/* preference flags for PC's.			*/
  room_vnum			load_room;					/* Which room to place char in			*/
  /* Killing same mobs XP penalties arrays */
  mob_vnum			kills_vnum[100];			/* Virtual numbers of the mobs killed	*/
  ush_int			kills_amount[100];			/* Number of mobs of that type killed	*/
  byte				kills_curpos;				/* Current position in array			*/
};

/* Specials used by NPCs, not PCs */
struct mob_special_data
{
  CHAR_DATA			*ridden_by;					/* mob being mounted					*/
  VEHICLE_DATA		*hitched_to;				/* Is mob dragging a vehicle?			*/
  YOKE_DATA			*yoke;						/* for yokeing pointers list			*/
  MEMORY_REC		*memory;					/* List of attackers to remember		*/
  byte				default_pos;				/* Default position for NPC				*/
  byte				damnodice;					/* The number of damage dice's			*/
  byte				damsizedice;				/* The size of the damage dice's		*/
  byte				maxfactor;					/* Max number of kills of mob allowed	*/
  sh_int			timer;						/* used to extract encounter mobs		*/
  int				attack_type;				/* The Attack Type Bitvector for NPC's	*/
  int				gold;						/* Money carried						*/
  long				owner_id;					/* Mob is owned by this pc				*/
};

/* An affect structure */
struct affected_type
{
  AFFECTED_DATA		*next;
  byte				location;					/* Tells which ability to change(APPLY_XXX) */
  sbyte				modifier;					/* This is added to apropriate ability	*/
  sh_int			type;						/* The type of spell that caused this	*/
  sh_int			duration;					/* For how long its effects will last	*/
  long				bitvector;					/* Tells which bits to set (AFF_XXX)	*/
};

/* Structure used for chars following other chars */
struct follow_type
{
  FOLLOW_DATA		*next;
  CHAR_DATA			*follower;
};

/* ================== Structure for player/non-player ===================== */
struct char_data
{
  CHAR_DATA			*next;						/* For either mobs or ppl-list			*/
  CHAR_DATA			*next_in_room;				/* For room->people list				*/
  CHAR_DATA			*next_in_obj;				/* For obj->people list					*/
  CHAR_DATA			*next_in_building;			/* For building->people list			*/
  CHAR_DATA			*next_in_vehicle;			/* For vehicle/ship->people list		*/
  CHAR_DATA			*next_fighting;				/* For fighting list					*/
  CHAR_DATA			*master;					/* Who is char following?				*/
  FOLLOW_DATA		*followers;					/* List of chars followers				*/
  DESCRIPTOR_DATA	*desc;						/* NULL for mobiles						*/
  OBJ_DATA			*equipment[NUM_WEARS];		/* Equipment array						*/
  OBJ_DATA			*first_carrying;			/* Head of object list					*/
  OBJ_DATA			*last_carrying;				/* Tail of object list					*/
  OBJ_DATA			*in_obj;					/* Furniture objects					*/
  ROOM_DATA			*in_room;					/* Location (real room number)			*/
  ROOM_DATA			*was_in_room;				/* location for linkdead people			*/
  ROOM_DATA			*last_room;
  BUILDING_DATA		*in_building;				/* In which building					*/
  SHIP_DATA			*in_ship;					/* Char is on a ship?					*/
  VEHICLE_DATA		*in_vehicle;				/* Is inside a vehicle?					*/
  PLAYER_SPECIAL	*player_specials;			/* PC specials							*/
  AFFECTED_DATA		*affected;					/* affected by what spells				*/
  EVENT_DATA		*action;					/* event for action which takes time	*/
  PLAYER_DATA		player;						/* Normal data							*/
  CHAR_ABIL_DATA	real_abils;					/* Abilities without modifiers			*/
  CHAR_ABIL_DATA	aff_abils;					/* Abils with spells/stones/etc			*/
  CHAR_POINT_DATA	points;						/* Points								*/
  MOB_SPECIAL		mob_specials;				/* NPC specials							*/
  int				pfilepos;					/* playerfile pos						*/
  int				wait;						/* wait for how many loops				*/
  mob_rnum			nr;							/* Mob's rnum							*/
};
/* ====================================================================== */


/* descriptor-related structures ******************************************/

struct txt_block
{
  TXT_BLOCK			*next;
  char				*text;
  int				aliased;
};

struct txt_q
{
  TXT_BLOCK			*head;
  TXT_BLOCK			*tail;
};

struct descriptor_data
{
  DESCRIPTOR_DATA	*next;						/* link to next descriptor				*/
  DESCRIPTOR_DATA	*snooping;					/* Who is this char snooping			*/
  DESCRIPTOR_DATA	*snoop_by;					/* And who is snooping this char		*/
  CHAR_DATA			*character;					/* linked to char						*/
  CHAR_DATA			*original;					/* original char if switched			*/
  TXT_BLOCK			*large_outbuf;				/* ptr to large buffer, if we need it	*/
  TXT_Q				input;						/* q of unprocessed input				*/
  socket_t			descriptor;					/* file descriptor for socket			*/
  char				host[HOST_LENGTH+1];		/* hostname								*/
  byte				bad_pws;					/* number of bad pw attemps this login	*/
  byte				idle_tics;					/* tics idle at password prompt			*/
  int				connected;					/* mode of 'connectedness'				*/
  int				desc_num;					/* unique num assigned to desc			*/
  time_t			login_time;					/* when the person connected			*/
  char				*showstr_head;				/* for keeping track of an internal str	*/
  char				**showstr_vector;			/* for paging through texts				*/
  int				showstr_count;				/* number of pages to page through		*/
  int				showstr_page;				/* which page are we currently showing?	*/
  char				**str;						/* for the modify-str system			*/
  size_t			max_str;					/*		-								*/
  long				mail_to;					/* name for mail system					*/
  int				has_prompt;					/* is the user at a prompt?             */
  char				inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input				*/
  char				last_input[MAX_INPUT_LENGTH]; /* the last input						*/
  char				small_outbuf[SMALL_BUFSIZE];  /* standard output buffer				*/
  char				*output;					/* ptr to the current output buffer		*/
  char				**history;					/* History of commands, for ! mostly.	*/
  int				history_pos;				/* Circular array position.				*/
  int				bufptr;						/* ptr to end of current output			*/
  int				bufspace;					/* space left in the output buffer		*/
  C_FUNC(*callback);							/* Call back function					*/
  int				callback_depth;				/* Call back "psuedo-recursiveness" depth */
  void				*c_data;					/* Storage for the Callback function	*/
};


/* other miscellaneous structures ***************************************/

typedef struct name_number			NAME_NUMBER;

struct name_number
{
  char				*name;
  int				number;
};

struct msg_type
{
  char				*attacker_msg;				/* message to attacker					*/
  char				*victim_msg;				/* message to victim					*/
  char				*room_msg;					/* message to room						*/
};

struct message_type
{
  MESSAGE_TYPE		*next;						/* to next messages of this kind.		*/
  MSG_TYPE			die_msg;					/* messages when death					*/
  MSG_TYPE			miss_msg;					/* messages when miss					*/
  MSG_TYPE			hit_msg;					/* messages when hit					*/
  MSG_TYPE			god_msg;					/* messages when hit on god				*/
};

struct message_list
{
  MESSAGE_TYPE		*msg;						/* List of messages.					*/
  int				a_type;						/* Attack type							*/
  int				number_of_attacks;			/* How many attack messages to chose from. */
};

struct dex_skill_type
{
  sh_int			p_pocket;
  sh_int			p_locks;
  sh_int			traps;
  sh_int			sneak;
  sh_int			hide;
};

struct dex_app_type
{
  sh_int			reaction;
  sh_int			miss_att;
  sh_int			defensive;
};

struct str_app_type
{
  sh_int			tohit;						/* To Hit (THAC0) Bonus/Penalty			*/
  sh_int			todam;						/* Damage Bonus/Penalty					*/
  sh_int			carry_w;					/* Maximum weight that can be carrried	*/
  sh_int			wield_w;					/* Maximum weight that can be wielded	*/
};

struct wis_app_type
{
  byte				bonus;						/* how many practices player gains per lev */
};

struct int_app_type
{
  byte				learn;						/* how many % a player learns a spell/skill */
  byte				memspl;						/* bonus on how many spells a player can memorize */
};

struct con_app_type
{
  sh_int			hitp;
  sh_int			shock;
};

#define PROF_LVL_START   3

struct weapon_prof_data
{
  sh_int			to_hit;
  sh_int			to_dam;
  sh_int			to_ac;
  sh_int			num_of_attacks;
};

/* element in monster and object index-tables   */
struct index_data
{
  int				vnum;						/* virtual number of this mob/obj			*/
  int				number;						/* number of existing units of this mob/obj */
  SPECIAL(*func);
};

struct who_list
{
  WHO_LIST			*next;
  char				*name;
  int				level;
};


struct auth_data
{
  AUTH_DATA			*next;						/* next node in authorization list		*/
  long				id_num;						/* player id num						*/
  char				*name;						/* player name (for fast display)		*/
};


/* =================================================================== */
/*                           W E A T H E R                             */
/* =================================================================== */

struct weather_data
{
  sh_int			temp;						// temperature
  sh_int			humidity;					// humidity
  sh_int			precip_rate;				// precipitation rate
  sh_int			precip_change;				// precipitation change
  sh_int			wind_dir;					// wind direction
  sh_int			windspeed;					// wind speed
  sh_int			pressure;					// pressure
  sh_int			pressure_change;			// change of pressure
  ush_int			free_energy;
};

/* =================================================================== */
/*                             E V E N T S                             */
/* =================================================================== */

/* number of queues to use (reduces enqueue cost) */
#define NUM_EVENT_QUEUES		10

struct q_element_data
{
  Q_ELEM_DATA		*next;
  Q_ELEM_DATA		*prev;
  void				*data;
  long				key;
};

struct queue_data
{
  Q_ELEM_DATA		*head[NUM_EVENT_QUEUES];
  Q_ELEM_DATA		*tail[NUM_EVENT_QUEUES];
};

struct event_data
{
  Q_ELEM_DATA		*q_el;
  void				*event_obj;
  EVENTFUNC(*func);
};


/* Things that use events */
typedef struct	ship_sail_event			SAIL_EVENT;
typedef struct	room_teleport_event		ROOM_TELEPORT;
typedef struct	room_heal_event			ROOM_HEAL;
typedef struct	camp_event				CAMP_EVENT;
typedef struct	ferry_move_event		FERRY_EVENT;

struct ship_sail_event
{
  SHIP_DATA			*ship;
  sh_int			type;
};

struct room_teleport_event
{
  CHAR_DATA			*ch;
  ROOM_DATA			*orig_room;
  ROOM_DATA			*target;
  char				*msg_to_char;
  char				*msg_to_room;
  int				delay;
};

struct room_heal_event
{
  CHAR_DATA			*ch;
  ROOM_DATA			*orig_room;
  char				*msg_to_char;
  char				*msg_to_room;
  int				delay;
};

struct camp_event
{
  CHAR_DATA			*ch;
  ROOM_DATA			*pRoom;
};

struct ferry_move_event
{
  FERRY_DATA		*ferry;
  sh_int			dir;
};


/* =================================================================== */
/*                          S P E L L B O O K S                        */
/* =================================================================== */
#define BOOK_TRAVELLING			0
#define BOOK_BOOK				1
#define BOOK_TOME				2

#define PAGE_BLANK				0
#define PAGE_WRITTEN			1
#define PAGE_RUINED				2

#define MIN_PAGES				3

struct book_page_data
{
  BOOK_PAGE			*next;						/* pointer to the next page				*/
  BOOK_PAGE			*prev;						/* pointer to the prev page				*/
  char				*spellname;					/* name of the spell					*/
  sh_int			status;						/* page status							*/
  sh_int			flags;						/* page blank/written					*/
  sh_int			spellnum;					/* num of the spell written in the page	*/
};

struct spellbook_data
{
  BOOK_PAGE			*first_page;				/* first book page						*/
  BOOK_PAGE			*last_page;					/* last book page						*/
  sh_int			type;						/* type of spellbook					*/
  sh_int			num_of_spells;				/* how many spells the book contains	*/
  sh_int			num_of_pages;				/* how many pages are in the book		*/
};

struct book_type_data
{
  sh_int			max_num_pages;
  sh_int			mult;
};


/* ==================================================================== */
/*                          W I L D E R N E S S							*/
/* ==================================================================== */
#define WILD_ZONE				5000

#define WILD_Y					2000
#define WILD_X					2000

#define	MAP_SIZE				20
#define	MAP_X					(MAP_SIZE * 3)
#define	MAP_Y					(MAP_SIZE * 3)

#define MAP_WEATH_Y				(WILD_Y / MAP_SIZE)
#define MAP_WEATH_X				(WILD_X / MAP_SIZE)

#define WILD_HASH				41
#define WDATA_HASH				11
#define SECT_HASH				11

#define MAX_SURVEY_DIST			50				/* max distance for survey				*/
#define MAX_TRACK_DIST			20				/* max distance for track				*/
//#define MAX_SAIL_DIST			30				/* max distance for sailing				*/
#define MAX_SCAN_DIST			5				/* max distance for scanning			*/

#define WS_ADD_ROOM				0
#define WS_REM_ROOM				1

#define MAX_LNK_STONE			5

#define MOD_Y					6
#define MOD_X					8
#define MOD_SMALL_Y				2
#define MOD_SMALL_X				3

#define RADIUS_BASE				7
#define RADIUS_SMALL			3


struct wild_remove
{
  WILD_REMOVE		*next;
  ROOM_DATA			*wRoom;
  int				timer;
};

struct life_data
{
  LIFE_DATA			*next;
  char				*name;						/* encounter name						*/
  sh_int			sect;						/* type of terrain						*/
  sh_int			num_max;					/* max number							*/
  sh_int			num_min;					/* min number							*/
  sh_int			num_curr;					/* current number (used only in wd-> )	*/
  int				vnum;						/* mob vnum								*/
  float				regen_rate;					/* how many units repop per mud day		*/
};

/* Wilderness Sector Data */
struct wild_data
{
  WILD_DATA			*next;						/* next node in global list				*/
  COORD_DATA		*coord;						/* coordinates							*/
  LIFE_DATA			*life_table[SECT_HASH];		/* encounters table						*/
  char				ws_map[MAP_SIZE][MAP_SIZE+3]; /* wildsector ascii map				*/
  sh_int			num_of_rooms;				/* number of currently loaded rooms		*/
  time_t			last_load;					/* last time it has been loaded			*/
};

struct survey_data
{
  SURVEY_DATA		*next;
  COORD_DATA		*coord;
  char				*descriz;
};

struct pstone_data
{
  PSTONE_DATA		*next;						/* next node in global dble linked list	*/
  PSTONE_DATA		*prev;						/* next node in global dble linked list	*/
  COORD_DATA		coord;						/* coord where to place the pstone		*/
  char				*name;						/* keyword list for 'travel' cmd		*/
  char				*short_descr;				/* for lists_stone_in_room				*/
  char				*description;				/* for examine, look at, etc			*/
  sh_int			vnum;						/* stone vnum							*/
  sh_int			link_to[MAX_LNK_STONE];		/* vnum of stones linked with this one	*/
};

/* ==================================================================== */
/*                          B U I L D I N G S				*/
/* ==================================================================== */

#define BUILDING_ZONE			5001

#define BLD_HASH				11

/* types of buildings */
#define BLD_HUT					0
#define BLD_HOUSE				1
#define BLD_STABLE				2
#define BLD_STORE				3
#define BLD_WORKSHOP			4
#define BLD_CASTLE				5				/* TODO template!!!!					*/

#define NUM_BUILDING_TYPE		10

/* who own the building? */
#define BLD_OWN_CHAR			1				/* a single char own the building		*/
#define BLD_OWN_CLAN			2				/* a clan building						*/

/* who can enter the building */
#define BLD_ENT_FREE			0				/* everybody can enter					*/
#define BLD_ENT_CHAR			1				/* owner and authorized only			*/
#define BLD_ENT_CLAN			2				/* clan members and authorized only		*/

/* building flags */
#define BLD_F_CLANHALL			(1 << 0)		/* building is the clanhall of a clan	*/
#define BLD_F_RUIN				(1 << 1)		/* building is destroyed				*/

/* max number of defending mobs */
#define BLD_MAX_MOB				5

/* build works phases */
#define BWKS_BASE				1
#define BWKS_WALLS				2
#define BWKS_FINAL				3

struct building_points_data
{
  sh_int			health;
  sh_int			defense;
  sh_int			npc_attack;					/* for auto-defending building ie Towers	*/
  sh_int			mob_attack;					/* for auto-generating defenders mobs		*/
};

/*
 * commands for loading objects, mobs and other things on building
 *
 * valid commands are:
 *
 * Load a mobile:	M <mob vnum> <quantity> <building room vnum> <unused>
 * Load an object:	O <obj vnum> <quantity> <building room vnum> <unused>
 *
 */
struct building_commands
{
  BUILDING_CMD		*next;
  char				cmd;						/* type of command						*/
  int				arg[4];						/* 4 integer arguments					*/
};

struct building_type_data
{
  BUILDING_CMD		*cmds_list;					/* list of "reset" commands				*/
  BUILDING_PTS		range_min;					/* for random bld points generation		*/
  BUILDING_PTS		range_max;					/*           =							*/
  char				*name;						/* name of the building type			*/
  int				vnum;						/* type of building (see above)			*/
  int				size;						/* number of rooms						*/
  int				entrance;					/* entrance position in **rooms array	*/
  int				cost;						/* building cost (in GOLD coins)		*/
  mob_vnum			vmob[BLD_MAX_MOB];			/* vnum of defenders mobs				*/
};


struct building_works
{
  BLD_WORKS			*next;						/* next node in global list				*/
  BLD_WORKS			*prev;						/* prev node in global list				*/
  COORD_DATA		*coord;						/* at which coord we're building?		*/
  ROOM_DATA			*in_room;					/* in which room we're building?		*/
  bool				authorized;					/* are building works authorized ?		*/
  long				owner_id;					/* building owner id					*/
  sh_int			owner_type;					/* this is a clan building or a private one? */
  sh_int			timer;						/* how log before this work phase end?	*/
  sh_int			phase;						/* in which work phase are we?			*/
  int				type;						/* code of building type				*/
  int				num;						/* work number (for contracts)			*/
};


struct building_data
{
  BUILDING_DATA		*next;						/* for next node in global list			*/
  BUILDING_DATA		*next_in_room;				/* for room->buildings list				*/
  BUILDING_CMD		*cmds_list;					/* list of "reset" commands				*/
  COORD_DATA		*coord;						/* for building location in wilderness	*/
  BUILDING_TYPE		*type;						/* prototype of the building			*/
  ROOM_DATA			*in_room;					/* in which room is placed				*/
  ROOM_DATA			**rooms;					/* rooms list							*/
  CHAR_DATA			*people;					/* list of people inside				*/
  TRADING_POST		*trp;						/* for trading post usage				*/
  AUTH_DATA			*auth;						/* list of people authorized to enter	*/
  BUILDING_PTS		max_val;
  BUILDING_PTS		curr_val;
  char				*keyword;					/* for enter, attack, look, etc.		*/
  char				*description;				/* for look at room, etc.				*/
  char				can_enter;					/* who can enter (see above)			*/
  char				owner_type;					/* type of owner (see above)			*/
  sh_int			size;						/* size in rooms of the building		*/
  int				owner_id;					/* player id num or clan vnum			*/
  int				vnum;						/* building vnum						*/
  int				flags;						/* building flags						*/
  mob_vnum			vmob[BLD_MAX_MOB];			/* vnum of defenders mobs				*/
  room_vnum			vnum_room;					/* for building in zones				*/
};

/* ========================================================== */

/* ==================================================================== */
/*                          V E H I C L E S    							*/
/* ==================================================================== */

#define VEH_CART				0
#define VEH_WAGON				1

#define MAX_VEH_TYPE			10

#define VEH_FLY					(1 << 0)
#define VEH_INVISIBLE			(1 << 1)
#define VEH_DESTROYED			(1 << 2)

struct yoke_data
{
  YOKE_DATA			*next;
  YOKE_DATA			*prev;
  CHAR_DATA			*mob;
};


struct vehicle_points_data
{
  sh_int			speed;						/* !UNUSED! how fast it moves			*/
  sh_int			capacity;					/* how much it can contains				*/
  sh_int			health;						/* how much damage it can suffer 		*/
  sh_int			passengers;					/* how many people can go inside		*/
  sh_int			draft_mobs;					/* how many draft animals it can have	*/
};


struct vehicle_index
{
  VEHICLE_PTS		value;
  char				*name;						/* keyword list							*/
  char				*short_description;			/* displayed in look_at_room et similar */
  char				*description;				/* displayed in look_at_target etc.		*/
  sh_int			vnum;						/* vnum of vehicle proto				*/
};


struct vehicle_data
{
  VEHICLE_DATA		*next;						/* next node in global list				*/
  VEHICLE_DATA		*prev;						/* prev node in global list				*/
  VEHICLE_DATA		*next_in_room;				/* next node in room list				*/
  VEHICLE_INDEX		*type;
  ROOM_DATA			*in_room;					/* in which room is?					*/
  ROOM_DATA			*last_room;					/* in which room was?					*/
  ROOM_DATA			*veh_room;					/* room for placing people				*/
  CHAR_DATA			*people;					/* list of people inside the vehicle	*/
  CHAR_DATA			*wagoner;					/* who's driving the vehicle?			*/
  OBJ_DATA			*first_content;				/* list of loaded goods					*/
  OBJ_DATA			*last_content;				/* list of loaded goods 				*/
  YOKE_DATA			*first_yoke;
  YOKE_DATA			*last_yoke;
  VEHICLE_PTS		max_val;
  VEHICLE_PTS		curr_val;
  char				*name;						/* keyword list							*/
  char				*short_description;			/* displayed in look_at_room et similar */
  char				*description;				/* displayed in look_at_target etc.		*/
  int				flags;						/* vehicle bitvector					*/
  long				owner_id;					/* who's the vehicle owner?				*/
};


/* ==================================================================== */
/* Stables for renting mounts                          					*/
/* ==================================================================== */

#define ST_RENT_NONE			0
#define ST_RENT_MOUNT			1
#define ST_RENT_VEHICLE			2

struct stable_rent_data
{
  STABLE_RENT		*next;
  STABLE_RENT		*prev;
  char				*filename;
  char				*playername;
  char				*typedescr;
  sh_int			type;
  room_vnum			stable_vnum;
  long				id_player;
};


/* ==================================================================== */
/*                            S H I P S        				*/
/* ==================================================================== */
#define SHIP_ZONE				5002

#define NUM_SHIP_TYPE			10

#define SHIP_SPEED_BASE			20

#define SAIL_MANUAL				1
#define SAIL_COURSE				2

#define SHIP_IN_PORT			(1 << 0)
#define SHIP_SAIL				(1 << 1)
#define SHIP_COMBAT				(1 << 2)
#define SHIP_AT_ANCHOR			(1 << 3)
#define SHIP_IN_COURSE			(1 << 4)


struct ship_value_data
{
  sh_int			speed;
  sh_int			power;
  sh_int			defense;
  sh_int			equip;						/* crew									*/
  sh_int			loads;						/* load capacity (weight)				*/
  sh_int			health;						/* hp									*/
};

struct ship_type_data
{
  SHIP_VAL_DATA		value;
  char				*name;						/* name of type of ship					*/
  char				*short_descr;				/* short description					*/
  sh_int			skill_min;					/* minimum skill in NAVIGATION required	*/
  int				vnum;						/* vnum of the prototype				*/
  int				size;						/* number of rooms						*/
};


struct ship_data
{
  SHIP_DATA			*next;						/* next node in the global list			*/
  SHIP_DATA			*prev;						/* prev node in the global list			*/
  SHIP_DATA			*next_in_room;				/* next node in the room list			*/
  SHIP_TYPE			*type;						/* type of ship							*/
  CHAR_DATA			*people;					/* who's on the ship					*/
  CHAR_DATA			*helmperson;				/* who's at the helm?					*/
  ROOM_DATA			*in_room;					/* in which room is the ship			*/
  ROOM_DATA			*last_room;					/*										*/
  ROOM_DATA			**rooms;					/* array of ship rooms					*/
  ROOM_DATA			*storeroom;					/* la stiva								*/
  EVENT_DATA		*action;					/* event for action which takes time	*/
  COURSE_DATA		*course;					/* for automatic sailing				*/
  COURSE_STEP		*cstep;						/* current step in automatic sailing	*/
  COORD_DATA		*port;						/* in which port ship is?				*/
  AUTH_DATA			*authorized;				/* list of authorized ppl				*/
  SHIP_VAL_DATA		curr_val;					/* current values						*/
  SHIP_VAL_DATA		max_val;					/* standard values						*/
  char				*name;						/* the name of the ship					*/
  int				vnum;						/* vnum of the ship						*/
  int				direction;					/* naviga verso...						*/
  int				flags;						/* ship bitvector						*/
  long				idowner;					/* player owner of the ship				*/
  long				idcaptain;					/* player captain of the ship			*/
};

struct port_data
{
  PORT_DATA			*next;
  COORD_DATA		*coord;
  char				*name;
};

struct course_step
{
  COURSE_STEP		*next;
  COURSE_STEP		*prev;
  COORD_DATA		coord;
};

struct course_data
{
  COURSE_DATA		*next;						/* next node in the global list			*/
  COORD_DATA		*port_orig;					/* coords of starting port				*/
  COORD_DATA		*port_end;					/* coords of destination port			*/
  COURSE_STEP		*first_step;				/* first step out of the port			*/
  COURSE_STEP		*last_step;					/* last step 							*/
  char				*name;						/* course name							*/
  int				vnum;						/* course vnum							*/
};


#define FERRY_ZONE				5003
#define FERRY_WAIT				10
#define FERRY_SPEED				15

struct ferry_data
{
  FERRY_DATA		*next;						/* next node in global list				*/
  FERRY_DATA		*prev;						/* prev node in global list				*/
  COORD_DATA		*coord;						/* loading coordinates					*/
  CHAR_DATA			*mob;						/* the mob that control the ferry		*/
  ROOM_DATA			*room;						/* ferry room							*/
  ROOM_DATA			*in_room;					/* where it is							*/
  EVENT_DATA		*action;
  char				place;						/* in which site is?					*/
  sh_int			vnum;						/* vnum of the ferry					*/
  sh_int			dir;						/* direction from first to second site	*/
  sh_int			timer;						/* minutes counter						*/
  int				cost;						/* how much it charges to carry ppl		*/
};

/* ==================================================================== */
/* Goods                                               					*/
/* ==================================================================== */

/* Global limits */
#define MAX_GOOD				200
#define MAX_MARKET				20

/* Types of goods */
#define TYPE_CORN				0
#define TYPE_FLOUR				1
#define TYPE_FABRIC				2
#define TYPE_HIDE				3
#define TYPE_DYE				4
#define TYPE_METAL				5
#define TYPE_ORE				6
#define TYPE_SPICE				7
#define TYPE_SUGAR				8
#define TYPE_OIL				9
#define TYPE_WOOD				10

#define MAX_TYPE_CODE			11

/* Types of TP */
#define TP_GCLOSE				0
#define TP_GSTRICT				1
#define TP_GLIMITED				2
#define TP_GFREE				3

#define NUM_TP_TYPES			4

/* Goods <--> Markets data */
struct market_good_data
{
  sh_int			total_tp;					/* in how many tp the good is sold?		*/
  sh_int			good_appet;
  int				qty;						/* market's good quantity				*/
  int				price;
  float				comm_prod;
  float				demand;						/* market's good demand					*/
  float				comm_closure;
};

/* Types of good */
struct good_type_data
{
  char				*name;						/* type of good							*/
  int				vnum;						/* vnum of type							*/
  int				prod_avg[NUM_SEASONS];		/* minimal weekly seasonal production	*/
  int				elasticity;
  float				cons_speed;					/* market's good consumption speed		*/
};

/* Goods data */
struct good_data
{
  GOOD_TYPE			*gtype;						/* type of good							*/
  char				*name;						/* good name							*/
  char				*unit;						/* good unit of measurement				*/
  char				*short_descr;				/* good unit + good name				*/
  sh_int			code;						/* which type of good is				*/
  sh_int			weight;						/* how much one unit weight				*/
  sh_int			life;						/* how much it last						*/
  sh_int			docg;						/* 1 stop osmotic production			*/
  int				cost;						/* how much one unit cost				*/
  int				vnum;						/* good vnum 							*/
  int				mkvnum;						/* source market vnum 					*/
  int				quality;					/* for production code (act.works.c)	*/
};

/* Goods in TP data */
struct trp_good_data
{
  TRP_GOOD			*next;						/* next node in tp goods list			*/
  TRP_GOOD			*prev;						/* prev node in tp goods list			*/
  int				goods_vnum;					/* vnum of the goods					*/
  int				quantity;					/* how many available					*/
  int				prev_qty;					/* qty prev last reset					*/
  int				stock;						/* has this goods at stock?				*/
};

/* Trading Posts data */
struct trading_post_data
{
  TRADING_POST		*next;						/* next node in global list				*/
  TRADING_POST		*prev;						/* prev node in global list				*/
  TRADING_POST		*next_in_market;			/* next node in market list				*/
  MARKET_DATA		*market;					/* pointer to market area				*/
  TRP_GOOD			*first_tpgood;				/* first good in tp list				*/
  TRP_GOOD			*last_tpgood;				/* last good in tp list					*/
  ROOM_DATA			*in_room;					/* in which room is placed?				*/
  int				vnum;						/* vnum of the trading post				*/
  int				type;						/* type of trading post (see TP_Gxxxx)	*/
};


/* Values for markets */
struct market_vars
{
  float				prod_var;
  float				price_var;
  float				closure_var;
};

#define MKT_EXTRA_PROD			(1 << 0)
#define MKT_MALUS_PROD			(1 << 1)
#define MKT_EXTRA_PRICE			(1 << 2)
#define MKT_MALUS_PRICE			(1 << 3)
#define MKT_PESTILENCE			(1 << 4)
#define MKT_INSECT				(1 << 5)
#define MKT_DROUGHT				(1 << 6)
#define MKT_FLOOD				(1 << 7)
#define MKT_WAR					(1 << 8)

#define MOD_PROD				1
#define MOD_PRICE				2
#define MOD_CLOSURE				3

struct market_affections
{
  MARKET_AFF		*next;
  sh_int			duration;					/* For how long its effects will last	*/
  sh_int			what;						/* which value modifies					*/
  float				modifier;					/* value modifier						*/
  long				bitvector;					/* Tells which bits to set (MKT_XXX)	*/
};


/* Markets data */
struct market_data
{
  MARKET_DATA		*next;						/* next node in global list				*/
  MARKET_DATA		*prev;						/* prev node in global list				*/
  TRADING_POST		*tp_list;					/* list of trading posts				*/
  MARKET_AFF		*affect;					/* list of affections					*/
  COORD_DATA		heart;						/* "virtual" tp, market center			*/
  MARKET_VARS		var_real;
  MARKET_VARS		var_mod;
  char				*name;						/* name of the market					*/
  int				vnum;						/* vnum of the market					*/
  int				size;						/* how big the market is? (radius)		*/
  int				num_of_tp;					/* how many tp this market has?			*/
  int				affections;					/* bitvector of affections				*/
};

