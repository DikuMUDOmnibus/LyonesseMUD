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
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
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
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"

/* external globals */
extern TIME_DATA		time_info;

/* local functions */
TIME_INFO_DATA *real_time_passed(time_t t2, time_t t1);
TIME_INFO_DATA *mud_time_passed(time_t t2, time_t t1);
void die_follower(CHAR_DATA *ch);
void add_follower(CHAR_DATA *ch, CHAR_DATA *leader);
void prune_crlf(char *txt);


/* creates a random number in interval [from;to] */
int number(int from, int to)
{
	/* error checking in case people call number() incorrectly */
	if (from > to)
	{
		int tmp = from;

		from = to;
		to = tmp;
		log("SYSERR: number() should be called with lowest, then highest. number(%d, %d), not number(%d, %d).", from, to, to, from);
	}
	
	/*
	 * This should always be of the form:
	 *
	 *	((float)(to - from + 1) * rand() / (float)(RAND_MAX + from) + from);
	 *
	 * if you are using rand() due to historical non-randomness of the
	 * lower bits in older implementations.  We always use circle_random()
	 * though, which shouldn't have that problem. Mean and standard
	 * deviation of both are identical (within the realm of statistical
	 * identity) if the rand() implementation is non-broken.
	 */
	return ((circle_random() % (to - from + 1)) + from);
}


/* simulates dice roll */
int dice(int num, int size)
{
	int sum = 0;

	if (size <= 0 || num <= 0)
		return (0);

	while (num-- > 0)
		sum += number(1, size);

	return (sum);
}

bool roll(int val)
{
	int num = number(1, 20);

	if (num == 20 || num > val)
		return (0);

	return (1);
}

/* Find a percentage, with error checking. */
int percentage(int amount, int max)
{
	if (max > 0)
		return ((amount * 100) / max);

	return (amount * 100);
}


int MIN(int a, int b)
{
	return (a < b ? a : b);
}


int MAX(int a, int b)
{
	return (a > b ? a : b);
}


char *CAP(char *txt)
{
	*txt = UPPER(*txt);
	return (txt);
}


/* Create a duplicate of a string */
char *str_dup(const char *source)
{
	char *new_z;
	
	CREATE(new_z, char, strlen(source) + 1);
	return (strcpy(new_z, source));
}


/*
 * Strips \r\n from end of string.
 */
void prune_crlf(char *txt)
{
	int i;
	
	if (txt == NULL)
		return;

	i = strlen(txt) - 1;
	
	while (txt[i] == '\n' || txt[i] == '\r')
		txt[i--] = '\0';
}

void strip_cr(char *buffer)
{
	int rpos, wpos;
	
	if (buffer == NULL)
		return;
	
	for (rpos = 0, wpos = 0; buffer[rpos]; rpos++)
	{
		buffer[wpos] = buffer[rpos];
		wpos += (buffer[rpos] != '\r');
	}
	buffer[wpos] = '\0';
}

int sprintascii(char *out, bitvector_t bits)
{
	char *flags="abcdefghijklmnopqrstuvwxyzABCDEF";
	int i, j = 0;
	
	for (i = 0; flags[i] != '\0'; i++)
		if (bits & (1 << i))
			out[j++] = flags[i];
	
	if (j == 0) /* Didn't write anything. */
		out[j++] = '0';
		
	/* NUL terminate the output string. */
	out[j++] = '\0';
	return (j);
}



/*
 * str_cmp: a case-insensitive version of strcmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different or we reach the end of both.
 */
int str_cmp(const char *arg1, const char *arg2)
{
	int chk, i;
	
	if (arg1 == NULL || arg2 == NULL)
	{
		log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
		return (0);
	}
	
	for (i = 0; arg1[i] || arg2[i]; i++)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);	/* not equal */
		
	return (0);
}


/*
 * strn_cmp: a case-insensitive version of strncmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different, the end of both, or n is reached.
 */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
	int chk, i;
	
	if (arg1 == NULL || arg2 == NULL)
	{
		log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
		return (0);
	}
	
	for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
		if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
			return (chk);	/* not equal */
		
	return (0);
}

/* log a death trap hit */
void log_death_trap(CHAR_DATA *ch)
{
	char buf[256];
	
	sprintf(buf, "%s hit death trap #%d (%s)", GET_NAME(ch),
		ch->in_room->number, ROOM_NAME(ch));
	mudlog(buf, BRF, LVL_IMMORT, TRUE);
}

/*
 * New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void basic_mud_log(const char *format, ...)
{
	va_list args;
	time_t ct = time(0);
	char *time_s = asctime(localtime(&ct));
	
	if (logfile == NULL)
	{
		puts("SYSERR: Using log() before stream was initialized!");
		return;
	}
	
	if (format == NULL)
		format = "SYSERR: log() received a NULL format.";
	
	time_s[strlen(time_s) - 1] = '\0';
	
	fprintf(logfile, "%-15.15s :: ", time_s + 4);
	
	va_start(args, format);
	vfprintf(logfile, format, args);
	va_end(args);
	
	fprintf(logfile, "\n");
	fflush(logfile);
}


/* the "touch" command, essentially. */
int touch(const char *path)
{
	FILE *fl;
	
	if (!(fl = fopen(path, "a")))
	{
		log("SYSERR: %s: %s", path, strerror(errno));
		return (-1);
	}
	else
	{
		fclose(fl);
		return (0);
	}
}


/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void mudlog(const char *str, int type, int level, int file)
{
	char buf[MAX_STRING_LENGTH], tp;
	DESCRIPTOR_DATA *i;
	
	if (str == NULL)
		return;	/* eh, oh well. */
	if (file)
		log("%s", str);
	if (level < 0)
		return;
	
	sprintf(buf, "[ %s ]\r\n", str);
	
	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
			continue;
		if (GET_LEVEL(i->character) < level)
			continue;
		if (PLR_FLAGGED(i->character, PLR_WRITING))
			continue;
		tp = ((PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) +
			(PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0));
		if (tp < type)
			continue;
		
		send_to_char(CCGRN(i->character, C_NRM), i->character);
		send_to_char(buf, i->character);
		send_to_char(CCNRM(i->character, C_NRM), i->character);
	}
}



/*
 * If you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.
 */
void sprintbit(bitvector_t bitvector, const char *names[], char *result)
{
	long nr;
	
	*result = '\0';
	
	for (nr = 0; bitvector; bitvector >>= 1)
	{
		if (IS_SET(bitvector, 1))
		{
			if (*names[nr] != '\n')
			{
				strcat(result, names[nr]);
				strcat(result, " ");
			}
			else
				strcat(result, "UNDEFINED ");
		}
		if (*names[nr] != '\n')
			nr++;
	}
	
	if (!*result)
		strcpy(result, "NOBITS ");
}



void sprinttype(int type, const char *names[], char *result)
{
	int nr = 0;
	
	while (type && *names[nr] != '\n')
	{
		type--;
		nr++;
	}
	
	if (*names[nr] != '\n')
		strcpy(result, names[nr]);
	else
		strcpy(result, "UNDEFINED");
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
TIME_INFO_DATA *real_time_passed(time_t t2, time_t t1)
{
	static TIME_INFO_DATA now;
	long secs;
	
	secs = t2 - t1;
	
	now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
	secs -= SECS_PER_REAL_HOUR * now.hours;
	
	now.day = (secs / SECS_PER_REAL_DAY);			/* 0..34 days  */
	/* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */
	
	now.month	= -1;
	now.year	= -1;
	
	return (&now);
}

/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
TIME_INFO_DATA *mud_time_passed(time_t t2, time_t t1)
{
	static TIME_INFO_DATA now;
	long secs;
	
	secs = t2 - t1;
	
	now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
	secs -= SECS_PER_MUD_HOUR * now.hours;
	
	now.day = (secs / SECS_PER_MUD_DAY) % 35;		/* 0..34 days  */
	secs -= SECS_PER_MUD_DAY * now.day;
	
	now.month = (secs / SECS_PER_MUD_MONTH) % 17;	/* 0..16 months */
	secs -= SECS_PER_MUD_MONTH * now.month;
	
	now.year = (secs / SECS_PER_MUD_YEAR);			/* 0..XX? years */
	
	return (&now);
}

time_t days_passed(time_t last_date)
{
	TIME_INFO_DATA *tdiff = NULL;
	time_t days = 0;

	tdiff = mud_time_passed(time(0), last_date);

	if (tdiff->year > 0)
		days += tdiff->year  * 35 * 24;
	if (tdiff->month > 0)
		days += tdiff->month * 24;
	days += tdiff->day;

	return (days);
}

time_t mud_time_to_secs(TIME_INFO_DATA *now)
{
	time_t when = 0;
	
	when += now->year  * SECS_PER_MUD_YEAR;
	when += now->month * SECS_PER_MUD_MONTH;
	when += now->day   * SECS_PER_MUD_DAY;
	when += now->hours * SECS_PER_MUD_HOUR;
	
	return (time(NULL) - when);
}


TIME_INFO_DATA *age(CHAR_DATA *ch)
{
	static TIME_INFO_DATA player_age;
	
	player_age = *mud_time_passed(time(0), ch->player.time.birth);
	
	player_age.year += 17;	/* All players start at 17 */
	
	return (&player_age);
}


/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(CHAR_DATA *ch, CHAR_DATA *victim)
{
	CHAR_DATA *k;
	
	for (k = victim; k; k = k->master)
	{
		if (k == ch)
			return (TRUE);
	}
	
	return (FALSE);
}



/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(CHAR_DATA *ch)
{
	FOLLOW_DATA *j, *k;
	
	if (ch->master == NULL)
	{
		core_dump();
		return;
	}
	
	if (AFF_FLAGGED(ch, AFF_CHARM))
	{
		act("You realize that $N is a jerk!", FALSE, ch, 0, ch->master, TO_CHAR);
		act("$n realizes that $N is a jerk!", FALSE, ch, 0, ch->master, TO_NOTVICT);
		act("$n hates your guts!", FALSE, ch, 0, ch->master, TO_VICT);
		if (affected_by_spell(ch, SPELL_CHARM))
			affect_from_char(ch, SPELL_CHARM);
	}
	else
	{
		act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
		act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
		act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);
	}
	
	if (ch->master->followers->follower == ch)	/* Head of follower-list? */
	{
		k = ch->master->followers;
		ch->master->followers = k->next;
		free(k);
	}
	else			/* locate follower who is not head of list */
	{
		for (k = ch->master->followers; k->next->follower != ch; k = k->next);
		
		j = k->next;
		k->next = j->next;
		free(j);
	}
	
	ch->master = NULL;
	REMOVE_BIT(AFF_FLAGS(ch), AFF_CHARM | AFF_GROUP);
}


int num_followers_charmed(CHAR_DATA *ch)
{
	FOLLOW_DATA *lackey;
	int total = 0;
	
	for (lackey = ch->followers; lackey; lackey = lackey->next)
		if (AFF_FLAGGED(lackey->follower, AFF_CHARM) && lackey->follower->master == ch)
			total++;
		
	return (total);
}


/* Called when a character that follows/is followed dies */
void die_follower(CHAR_DATA *ch)
{
	FOLLOW_DATA *j, *k;
	
	if (ch->master)
		stop_follower(ch);
	
	for (k = ch->followers; k; k = j)
	{
		j = k->next;
		stop_follower(k->follower);
	}
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(CHAR_DATA *ch, CHAR_DATA *leader)
{
	FOLLOW_DATA *k;
	
	if (ch->master)
	{
		core_dump();
		return;
	}
	
	ch->master = leader;
	
	CREATE(k, FOLLOW_DATA, 1);
	
	k->follower = ch;
	k->next = leader->followers;
	leader->followers = k;
	
	act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
	if (CAN_SEE(leader, ch))
		act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
	act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE *fl, char *buf)
{
	char temp[256];
	int lines = 0;
	int sl;
	
	do
	{
		if (!fgets(temp, 256, fl))
			return (0);
		lines++;
	} while (*temp == '*' || *temp == '\n');
	
	/* Last line of file doesn't always have a \n, but it should. */
	sl = strlen(temp);
	if (sl > 0 && temp[sl - 1] == '\n')
		temp[sl - 1] = '\0';
	
	strcpy(buf, temp);
	return (lines);
}


int get_filename(char *orig_name, char *filename, int mode)
{
	const char *prefix, *middle, *suffix;
	char name[64], *ptr;
	
	if (orig_name == NULL || *orig_name == '\0' || filename == NULL)
	{
		log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
			orig_name, filename);
		return (0);
	}
	
	switch (mode)
	{
	case OBJS_FILE:
		prefix = LIB_PLROBJS;
		suffix = SUF_OBJS;
		break;
	case ALIAS_FILE:
		prefix = LIB_PLRALIAS;
		suffix = SUF_ALIAS;
		break;
	case ETEXT_FILE:
		prefix = LIB_PLRTEXT;
		suffix = SUF_TEXT;
		break;
	case PLR_FILE:
		prefix = LIB_PLAYERS;
		suffix = SUF_PLAYERS;
		break;
	default:
		return (0);
	}
	
	strcpy(name, orig_name);
	for (ptr = name; *ptr; ptr++)
		*ptr = LOWER(*ptr);
	
	switch (LOWER(*name))
	{
	case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
		middle = "A-E";
		break;
	case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
		middle = "F-J";
		break;
	case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
		middle = "K-O";
		break;
	case 'p':  case 'q':  case 'r':  case 's':  case 't':
		middle = "P-T";
		break;
	case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
		middle = "U-Z";
		break;
	default:
		middle = "ZZZ";
		break;
	}
	
	sprintf(filename, "%s%s"SLASH"%s.%s", prefix, middle, name, suffix);
	return (1);
}


int num_pc_in_room(ROOM_DATA *room)
{
	CHAR_DATA *ch;
	int i = 0;
	
	for (ch = room->people; ch != NULL; ch = ch->next_in_room)
		if (!IS_NPC(ch))
			i++;
		
	return (i);
}


/* Will return the number of items in the container. */
int count_contents(OBJ_DATA *container)
{
	OBJ_DATA *obj;
	int count = 0;
	
	if (container->first_content)
	{
		for (obj = container->first_content; obj; obj = obj->next_content, count += obj->count)
			if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->first_content)
				count += count_contents(obj);
	}		
	
	return (count);
}

/* Will return the number of items that the character owns. */
int item_count(CHAR_DATA *ch)
{
	OBJ_DATA *obj;
	int i, count = 0;
	
	for (i = 0; i < NUM_WEARS; i++)
	{
		if (GET_EQ(ch, i))
		{
			count++;
			if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_CONTAINER &&
			    GET_EQ(ch, i)->first_content)
				count += count_contents(GET_EQ(ch, i));
		}
	}
	
	if (ch->first_carrying)
	{
		for (obj = ch->first_carrying; obj; obj = obj->next_content, count += obj->count)
			if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER && obj->first_content)
				count += count_contents(obj);
	}
	
	return (count);
}

/*
 * Rules (unless overridden by ROOM_DARK):
 *
 * Inside and City rooms are always lit.
 * Outside rooms are dark at sunset and night.
 */
int room_is_dark(ROOM_DATA *room)
{
	if (!room)
	{
		log("room_is_dark: Invalid room pointer.");
		return (FALSE);
	}
	
	if (room->light)
		return (FALSE);
	
	if (ROOM_FLAGGED(room, ROOM_DARK))
		return (TRUE);
	
	if (SECT(room) == SECT_INSIDE || SECT(room) == SECT_CITY)
		return (FALSE);
	
	if (Sunlight == SUN_DARK)
		return (TRUE);

	if (Sunlight >= SUN_SET)
	{
		if (Sunlight == MOON_LIGHT && MoonPhase == MOON_FULL)
			return (FALSE);

		return (TRUE);
	}
	
	return (FALSE);
}

/*
 * This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * You still want to call abort() or exit(1) for
 * non-recoverable errors, of course...
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
void core_dump_real(const char *who, int line)
{
  log("SYSERR: Assertion failed at %s:%d!", who, line);

#if 0	/* By default, let's not litter. */
#if defined(CIRCLE_UNIX)
  /* These would be duplicated otherwise...make very sure. */
  fflush(stdout);
  fflush(stderr);
  fflush(logfile);
  /* Everything, just in case, for the systems that support it. */
  fflush(NULL);

  /*
   * Kill the child so the debugger or script doesn't think the MUD
   * crashed.  The 'autorun' script would otherwise run it again.
   */
  if (fork() == 0)
    abort();
#endif
#endif
}

/* ======================= N E W   E X I T   H A N D L I N G ======================= */

bool valid_exit(EXIT_DATA *pexit)
{
	if (!pexit)
		return (FALSE);

	if ( EXIT_FLAGGED(pexit, EX_CLOSED)	||
	     EXIT_FLAGGED(pexit, EX_FALSE)	||
	     EXIT_FLAGGED(pexit, EX_HIDDEN) )
		return (FALSE);

	return (TRUE);
}

char *rev_exit(sh_int vdir)
{
	switch(vdir)
	{
	default: return ("somewhere");
	case 0:  return ("the south");
	case 1:  return ("the west");
	case 2:  return ("the north");
	case 3:  return ("the east");
	case 4:  return ("below");
	case 5:  return ("above");
	case 6:  return ("the southwest");
	case 7:  return ("the southeast");
	case 8:  return ("the northwest");
	case 9:  return ("the northeast");
	}
	
	return ("<???>");
}

/*
 * Function to get the equivelant exit of DIR 0-MAXDIR out of linked list.
 * Made to allow old-style diku-merc exit functions to work.	-Thoric
 */
EXIT_DATA *get_exit(ROOM_DATA *room, sh_int dir)
{
	EXIT_DATA *xit;
	
	if (!room)
	{
		log("SYSERR: Get_exit: NULL room");
		return (NULL);
	}
	
	for (xit = room->first_exit; xit; xit = xit->next )
	{
		if (xit->vdir == dir)
		{
			if (EXIT_FLAGGED(xit, EX_FALSE) ||
			    EXIT_FLAGGED(xit, EX_HIDDEN))
				return (NULL);
			return (xit);
		}
	}

	if (IS_WILD(room) && (dir != UP && dir != DOWN))
	{
		/*
		 * this is the hack.. find_wild_room creates the
		 * destination room and the exits between
		 * these two rooms
		 */
		find_wild_room(room, dir, TRUE);
		
		for (xit = room->first_exit; xit; xit = xit->next)
		{
			if (xit->vdir == dir)
				return (xit);
		}
	}
		
	return (NULL);
}


EXIT_DATA *find_exit(ROOM_DATA *room, sh_int dir)
{
	EXIT_DATA *xit;
	
	if (!room)
	{
		log("SYSERR: Get_exit: NULL room");
		return (NULL);
	}
	
	for (xit = room->first_exit; xit; xit = xit->next )
	{
		if (xit->vdir == dir)
			return (xit);
	}

	return (NULL);
}


ROOM_DATA *find_room( ROOM_DATA *room, sh_int dir )
{
	EXIT_DATA *xit;
	
	if ( !room )
	{
		log( "SYSERR: Get_exit: NULL room" );
		return ( NULL );
	}

	for (xit = room->first_exit; xit; xit = xit->next )
	{
		if ( xit->vdir == dir )
			break;
	}

	if ( !xit || !valid_exit(xit) )
		return ( NULL );

	return ( xit->to_room );
}

/*
 * Function to get an exit, leading the the specified room
 */
EXIT_DATA *get_exit_to( ROOM_DATA *room, sh_int dir, int vnum )
{
	EXIT_DATA *xit;
	
	if ( !room )
	{
		log( "Get_exit: NULL room" );
		return ( NULL );
	}
	
	for (xit = room->first_exit; xit; xit = xit->next )
		if ( xit->vdir == dir && xit->vnum == vnum )
			return ( xit );
	return ( NULL );
}

/*
 * Function to get the nth exit of a room			-Thoric
 */
EXIT_DATA *get_exit_num( ROOM_DATA *room, sh_int count )
{
	EXIT_DATA *xit;
	int cnt;
	
	if ( !room )
	{
		log( "Get_exit: NULL room" );
		return ( NULL );
	}
	
	for (cnt = 0, xit = room->first_exit; xit; xit = xit->next )
		if ( ++cnt == count )
			return ( xit );
		
	return ( NULL );
}

/*
 * Function to get an exit, leading to the specified coordinates
 */
EXIT_DATA *get_exit_to_coord( ROOM_DATA *room, sh_int dir, COORD_DATA *coord )
{
	EXIT_DATA *xit;
	
	if ( !room )
	{
		log( "Get_exit: NULL room" );
		return ( NULL );
	}
	
	for (xit = room->first_exit; xit; xit = xit->next )
		if ( xit->vdir == dir && 
		     ( xit->coord->x == coord->x && xit->coord->y == coord->y ) )
			return ( xit );
	return ( NULL );
}


EXIT_DATA *make_exit( ROOM_DATA *pRoom, ROOM_DATA *to_room, sh_int door )
{
	EXIT_DATA *pexit, *texit;
	bool broke;
	
	CREATE( pexit, EXIT_DATA, 1 );
	pexit->coord		= NULL;
	pexit->rvcoord		= NULL;
	pexit->vdir			= door;
	pexit->rvnum		= pRoom->number;
	pexit->to_room		= to_room;
	
	if ( to_room )
	{
		if ( pRoom->zone == WILD_ZONE && to_room->zone == WILD_ZONE)
		{
			CREATE( pexit->rvcoord, COORD_DATA, 1 );
			*pexit->rvcoord			= *pRoom->coord;
			CREATE( pexit->coord, COORD_DATA, 1 );
			*pexit->coord			= *to_room->coord;
			texit = get_exit_to_coord( to_room, rev_dir[door], pRoom->coord );
		}
		else
		{
			pexit->vnum = to_room->number;
			texit = get_exit_to( to_room, rev_dir[door], pRoom->number );
		}
		if ( texit )	/* assign reverse exit pointers */
		{
			texit->rexit			= pexit;
			pexit->rexit			= texit;
		}
	}

	broke = FALSE;

	for ( texit = pRoom->first_exit; texit; texit = texit->next )
	{
		if ( door < texit->vdir )
		{
			broke = TRUE;
			break;
		}
	}
	if ( !pRoom->first_exit )
		pRoom->first_exit			= pexit;
	else
	{
		/* keep exits in incremental order - insert exit into list */
		if ( broke && texit )
		{
			if ( !texit->prev )
				pRoom->first_exit	= pexit;
			else
				texit->prev->next	= pexit;
			pexit->prev				= texit->prev;
			pexit->next				= texit;
			texit->prev				= pexit;

			return ( pexit );
		}
		pRoom->last_exit->next		= pexit;
	}
	pexit->next						= NULL;
	pexit->prev						= pRoom->last_exit;
	pRoom->last_exit				= pexit;

	return ( pexit );
}

/*
 * Remove an exit from a room					-Thoric
 */
void extract_exit( ROOM_DATA *room, EXIT_DATA *pexit )
{
	UNLINK( pexit, room->first_exit, room->last_exit, next, prev );
	if ( pexit->rexit )
		pexit->rexit->rexit = NULL;
	if ( pexit->keyword )
		STRFREE( pexit->keyword );
	if ( pexit->description )
		STRFREE( pexit->description );
	
	if ( pexit->coord )
		DISPOSE(pexit->coord);

	DISPOSE( pexit );
}

