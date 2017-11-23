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
 * File: magic2.c                                                         *
 *                                                                        *
 * - Spellbook system                                                     *
 * - New magic system based upon spell memorization                       *
 * - Study/Copy skill (for acquiring spells from objects)                 *
 * - Enchant skill                                                        *
 *                                                                        *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"

/* external variables */
extern SPELL_INFO_DATA		spell_info[];
extern int			prac_params[4][NUM_CLASSES];


#define SINFO			spell_info[spellnum]
#define LEARNED(ch)		(prac_params[0][(int)GET_CLASS(ch)])


/* ************************************************************** */
/* Spellbooks Code                                                */
/* ************************************************************** */

/* ================================================================ */
/* Low-level functions to create or destroy pages and books         */
/* ================================================================ */

/* instert in a book a new page with a random spells */
void add_rand_book_page(SPELLBOOK *book)
{
	BOOK_PAGE *page;
	int spellnum = number(1, LAST_SPELL);

	CREATE(page, BOOK_PAGE, 1);
	page->spellnum		= spellnum;
	page->status		= number(60, 90);
	page->flags			= PAGE_WRITTEN;
	page->spellname		= str_dup(SINFO.name);

	LINK(page, book->first_page, book->last_page, next, prev);
	book->num_of_pages++;
	book->num_of_spells++;
}

/* initialize a new blank page for a book */
BOOK_PAGE *new_book_page(void)
{
	BOOK_PAGE *page;

	CREATE(page, BOOK_PAGE, 1);
	page->spellnum		= NOTHING;
	page->status		= number(80, 100);
	page->flags			= PAGE_BLANK;
	page->spellname		= NULL;

	return (page);
}

/* remove a page from a book */
void remove_book_page(SPELLBOOK *book, BOOK_PAGE *page)
{
	if ( page->spellname )
		STRFREE(page->spellname);
	UNLINK(page, book->first_page, book->last_page, next, prev);
	DISPOSE(page);
	book->num_of_pages--;
}

/* initialize a new spellbook */
SPELLBOOK *new_spellbook(OBJ_DATA *obj, int type, bool rand)
{
	SPELLBOOK *book;

	CREATE(book, SPELLBOOK, 1);
	book->first_page	= NULL;
	book->last_page		= NULL;
	book->num_of_pages	= 0;
	book->num_of_spells	= 0;
	book->type			= type;

	if ( rand )
	{
		int p;

		for ( p = 0; p < MIN_PAGES; p++ )
			add_rand_book_page( book );
	}

	return (book);
}

/* ================================================================ */
/* Spellbook info                                                   */
/* ================================================================ */

int find_page_cond( int percent )
{
	int num;

	if		(percent >= 100)	num = 0;
	else if (percent > 80)		num = 1;
	else if (percent > 65)		num = 2;
	else if (percent > 40)		num = 3;
	else if (percent > 25)		num = 4;
	else if (percent >  9)		num = 5;
	else if (percent >  0)		num = 6;
	else						num = 7;

	return (num);
}

/* lists all the pages in a spellbook */
void ListSpellsInBook(CHAR_DATA *ch, SPELLBOOK *book, bool dampage)
{
	BOOK_PAGE *page, *next_page = NULL;
	char bbuf[MAX_STRING_LENGTH];
	int pn, cond;

	*bbuf = '\0';

	send_to_char("The spellbook contains the following spells:\r\n", ch);
	for ( pn = 0, page = book->first_page; page; page = next_page, pn++ )
	{
		next_page = page->next;

		/* ruined pages are unreadables */
		if ( page->flags == PAGE_RUINED )
			strcat(bbuf, " - an unreadable page");
		else if ( page->flags == PAGE_BLANK )
		{
			cond = find_page_cond(percentage(page->status, 100));
			sprintf(bbuf+strlen(bbuf), " - a blank page that looks %s",
				obj_cond_table[cond]);
		}
		else
		{
			cond = find_page_cond(percentage(page->status, 100));
			sprintf(bbuf+strlen(bbuf), " - %s %s page that contains: '&b&3%s&0'",
				AN(obj_cond_table[cond]), obj_cond_table[cond], page->spellname);
		}
		strcat(bbuf, ".\r\n");

		/* take a chance to ruin the page */
		if ( dampage && !number(0, 5) )
		{
			page->status--;

			if ( page->status <= 0 )
			{
				remove_book_page(book, page);
				send_to_char("The page crumbles under your fingers.\r\n",ch);
			}
			else if ( page->status < 5 )
				page->flags = PAGE_RUINED;
		}
	}

	page_string(ch->desc, bbuf, 1);
}

/*
 * called from look_read in act.informative.c
 *
 * read the spellbook
 */
void ReadSpellbook(CHAR_DATA *ch, OBJ_DATA *obj, char *argument)
{
	SPELLBOOK *book;

	if ( !obj->special || !OBJ_FLAGGED(obj, ITEM_IS_SPELLBOOK) )
		return;

	if ( percentage(GET_OBJ_COND(obj), GET_OBJ_MAXCOND(obj)) < 10 )
	{
		send_to_char("The book is too damaged to be readable.\r\n", ch);
		return;
	}

	book = (SPELLBOOK *) obj->special;

	ch_printf(ch, "It's a spellbook with %d pages over a maximum of %d.\r\n",
		book->num_of_pages, book_types[book->type].max_num_pages);

	ListSpellsInBook(ch, book, TRUE);

	/* take a chance to damage the book */
	check_damage_obj(ch, obj, 4);
}

/* ================================================================ */
/* Spellbook checks                                                 */
/* ================================================================ */

/* check to see if the spellbook contains the spell */
BOOK_PAGE *BookHasSpell(SPELLBOOK *book, int sn)
{
	BOOK_PAGE *page = NULL;

	for ( page = book->first_page; page; page = page->next )
	{
		if ( page->flags == PAGE_RUINED )
			continue;

		if ( page->spellnum == sn )
			break;
	}

	return (page);
}


void MakePageBlank(BOOK_PAGE *page)
{
	if ( !page )
		return;

	if (page->spellname)
		STRFREE(page->spellname);
	page->spellnum	= NOTHING;
	page->flags	= PAGE_BLANK;
}

/* check for a blank page into the spellbook */
int BookHasBlankPage(SPELLBOOK *book)
{
	BOOK_PAGE *page = NULL;
	int count = 0;

	for ( page = book->first_page; page; page = page->next )
	{
		if ( page->flags == PAGE_RUINED )
			continue;

		if ( page->flags == PAGE_BLANK )
			count++;
	}

	return (count);
}

/*
 * recursive search of a spellbook into inventory
 * and carried containers
 *
 * returns SPELLBOOK data of object
 */
SPELLBOOK *FindSpellbook(OBJ_DATA *list, long owner_id)
{
	SPELLBOOK *book = NULL;
	OBJ_DATA *obj;

	for ( obj = list; obj; obj = obj->next_content )
	{
		if ( obj->special && OBJ_FLAGGED(obj, ITEM_IS_SPELLBOOK) &&
			GET_OBJ_OWNER(obj) == owner_id )
		{
			book = (SPELLBOOK *) obj->special;
			break;
		}

		if ( GET_OBJ_TYPE(obj) == ITEM_CONTAINER )
		{
			if ( (book = FindSpellbook(obj->first_content, owner_id)) )
				break;
		}
	}

	return ( book );
}


/*
 * recursive search of a spellbook into inventory
 * and carried containers
 *
 * returns the object
 */
OBJ_DATA *FindObjSpellbook( OBJ_DATA *list, long owner_id )
{
	OBJ_DATA *obj;

	for ( obj = list; obj; obj = obj->next_content )
	{
		if ( obj->special && OBJ_FLAGGED(obj, ITEM_IS_SPELLBOOK) &&
			GET_OBJ_OWNER(obj) == owner_id )
			break;

		if ( GET_OBJ_TYPE(obj) == ITEM_CONTAINER )
		{
			if ( (obj = FindObjSpellbook(obj->first_content, owner_id)) )
				break;
		}
	}

	return ( obj );
}


/* add a blank page to the spellbook */
bool AddBlankPage( SPELLBOOK *book )
{
	BOOK_PAGE *page;
	
	if ( book->num_of_pages >= book_types[book->type].max_num_pages )
		return (FALSE);

	page = new_book_page();

	LINK(page, book->first_page, book->last_page, next, prev);
	book->num_of_pages++;

	return (TRUE);
}

/* add the spell to the spellbook */
bool AddSpellToBook(CHAR_DATA *ch, SPELLBOOK *book, int sn)
{
	BOOK_PAGE *page;

	for (page = book->first_page; page; page = page->next)
	{
		if (page->flags == PAGE_RUINED)	continue;
		if (page->flags == PAGE_BLANK)	break;
	}

	if (!page)
	{
		send_to_char("There isn't a blank page to write on.\r\n", ch);
		return (FALSE);
	}

	/* take a chance to ruin the page */
	if ( !number(0, 5) )
	{
		page->status--;
		
		if ( page->status <= 0 )
		{
			remove_book_page(book, page);
			send_to_char("The page crumbles under your fingers.\r\n",ch);
			return (FALSE);
		}
		else if ( page->status < 5 )
		{
			page->flags = PAGE_RUINED;
			return (FALSE);
		}
	}

	page->spellnum		= sn;
	if (page->spellname)
		STRFREE(page->spellname);
	page->spellname		= str_dup(skill_name(sn));
	page->flags			= PAGE_WRITTEN;

	book->num_of_spells++;

	return (TRUE);
}

int AddNewSpell(CHAR_DATA *ch, int spellnum)
{
	SPELLBOOK *sbook;

	if ( !(sbook = FindSpellbook(ch->first_carrying, GET_IDNUM(ch))) )
	{
		send_to_char("You need one of yours spellbook.\r\n", ch);
		return (1);
	}
	
	if ( !BookHasSpell(sbook, spellnum ) )
	{
		if ( !BookHasBlankPage(sbook) )
		{
			if ( !AddBlankPage(sbook) )
			{
				send_to_char("You can't add any more pages to this spellbook.\r\n", ch);
				return (1);
			}
		}
		AddSpellToBook(ch, sbook, spellnum);
	}

	return (0);
}

/*
 * called from do_start() for new sorcerers
 */
void create_new_spellbook(CHAR_DATA *ch, int type)
{
	OBJ_DATA *obj = create_obj();

	GET_OBJ_TYPE(obj)		= ITEM_SPELLBOOK;
	GET_OBJ_COST(obj)		= 1000 * book_types[type].mult;
	GET_OBJ_LEVEL(obj)		= GET_LEVEL(ch);
	GET_OBJ_OWNER(obj)		= GET_IDNUM(ch);
	GET_OBJ_VAL(obj, 0)		= type;
	GET_OBJ_WEIGHT(obj)		= 20 * book_types[type].mult;

	obj->name				= str_dup("book spellbook");
	obj->short_description	= str_dup("a white leather covered book");
	obj->description		= str_dup("A white leather covered book with runes on the front has been left here.");

	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC | ITEM_UNIQUE | ITEM_IS_SPELLBOOK | ITEM_NODONATE);
	SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);

	obj->special = new_spellbook(obj, type, FALSE);
	obj_to_char(obj, ch);
}

/* immortal command to create a new spellbook */
ACMD(do_newbook)
{
	OBJ_DATA *obj = create_obj();

	GET_OBJ_TYPE(obj)		= ITEM_SPELLBOOK;
	GET_OBJ_COST(obj)		= 1000 * book_types[BOOK_BOOK].mult;
	GET_OBJ_LEVEL(obj)		= GET_LEVEL(ch);
	GET_OBJ_OWNER(obj)		= GET_IDNUM(ch);
	GET_OBJ_VAL(obj, 0)		= BOOK_BOOK;
	GET_OBJ_WEIGHT(obj)		= 20 * book_types[BOOK_BOOK].mult;

	if (obj->name)
		free(obj->name);
	obj->name				= str_dup("book spellbook");

	if (obj->short_description)
		free(obj->short_description);
	obj->short_description	= str_dup("a red leather covered book");

	if (obj->description)
		free(obj->description);
	obj->description		= str_dup("A red leather covered book with runes on the front has been left here.");

	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_MAGIC | ITEM_UNIQUE | ITEM_IS_SPELLBOOK);
	SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);

	obj->special = new_spellbook(obj, BOOK_BOOK, TRUE);

	obj_to_char(obj, ch);
	send_to_char("You create a new spellbook out of the blue.\r\n", ch);
}


/* *************************************************************** */
/* Magic System based upon memorized spells and spellbooks         */
/* *************************************************************** */

/* list of memorized spells */
void ListMemSpells(CHAR_DATA *ch)
{
	char buf[MAX_STRING_LENGTH];
	int spellnum;

	if ( IS_NPC(ch) )
		return;

	send_to_char("You have memorized the following spells:\r\n", ch);

	*buf = '\0';
	for (spellnum = 1; spellnum <= MAX_SPELLS; spellnum++)
	{
		if ( MEMORIZED(ch, spellnum) )
			sprintf(buf+strlen(buf), " [%d] '&b&3%s&0'\r\n", 
				MEMORIZED(ch, spellnum), SINFO.name);
	}

	if ( !*buf )
		strcat(buf, "None.\r\n");

	page_string(ch->desc, buf, 1);
}

/* return the maximum number of spell memorizable by ch */
int MaxSpellsCanMem(CHAR_DATA *ch)
{
	int num;

	if ( !ch || IS_NPC(ch) )
		return (0);

	num = (int) GET_LEVEL(ch);
	num += (int) int_app[GET_INT(ch)].memspl;
	
	return (URANGE(1, num, MAX_MEM_SPELLS));
}

/* return the number of spells memorized by ch */
int NumSpellsMem(CHAR_DATA *ch)
{
	register int sn;
	int num;

	if ( !ch || IS_NPC(ch) )
		return (0);

	for ( sn = 1; sn <= MAX_SPELLS; sn++ )
	{
		if ( MEMORIZED(ch, sn) )
			num += MEMORIZED(ch, sn);
	}

	return (num);
}

/*
 * return TRUE if ch can memorize 'num' 'sn' spell
 * FALSE otherwise
 */
bool CanMemSpell(CHAR_DATA *ch, int sn, int num)
{
	if ( !ch || IS_NPC(ch) )
		return (FALSE);

	if ( MEMORIZED(ch, sn) + num <= MAX_SAME_SPELL )
		return (TRUE);

	return (FALSE);
}


/* stop the memorization of a spell */
void stop_memorizing(CHAR_DATA *ch)
{
	if ( !ch || !ch->action || !AFF_FLAGGED(ch, AFF_MEMORIZING) )
		return;

	event_cancel(ch->action);
	REMOVE_BIT(AFF_FLAGS(ch), AFF_MEMORIZING);

	send_to_char("You stop memorizing your spell.\r\n", ch);	
}

/* the memorizing event */
EVENTFUNC(mem_event)
{
	MEM_EVENT *me = (MEM_EVENT *) event_obj;
	CHAR_DATA *ch = me->ch;
	int spellnum = me->spell;
	int tnum = me->num;

	ch_printf(ch, "You have finished memorizing '&b&3%s&0'.\r\n", SINFO.name);
	MEMORIZED(ch, spellnum) += tnum;

	ch->action	= NULL;
	REMOVE_BIT(AFF_FLAGS(ch), AFF_MEMORIZING);
	DISPOSE(event_obj);
	return (0);
}

/* command to memorize a spell */
ACMD(do_memorize)
{
	MEM_EVENT *me;
	SPELLBOOK *book;
	char *s, arg1[MAX_INPUT_LENGTH];
	int spellnum, tnum = 1;

	if ( IS_NPC(ch) )
		return;

	if ( !MEMMING_CLASS(ch) )
	{
		send_to_char("You cannot memorize spells.\r\n", ch);
		return;
	}

	if ( ch->action )
	{
		if ( AFF_FLAGGED(ch, AFF_MEMORIZING) )
			send_to_char("You can memorize only one spell at time.\r\n", ch);
		else
			send_to_char("You are too busy to memorize a spell right now.\r\n", ch);
		return;
	}

	/* get: blank, spell name, target name */
	s = get_spell_name(argument);
	if (s == NULL)
	{
		send_to_char("Usage: memorize 'spellname' <num>\r\n", ch);
		ListMemSpells(ch);
		return;
	}

	argument = strtok(NULL, "\0");
	one_argument(argument, arg1);

	if ( GET_POS(ch) != POS_RESTING )
	{
		send_to_char("You must rest before start memorizing spells.\r\n", ch);
		return;
	}

	spellnum = find_skill_num(s);
	
	if ((spellnum < 1) || (spellnum >= MAX_SPELLS))
	{
		send_to_char("Memorize what?!?\r\n", ch);
		return;
	}

	if ( !(book = FindSpellbook(ch->first_carrying, GET_IDNUM(ch)) ) )
	{
		send_to_char("You need a spellbook to memorize spells.\r\n", ch);
		return;
	}

	if ( !BookHasSpell(book, spellnum) )
	{
		send_to_char("This spellbook doesn't contains that spell.\r\n", ch);
		return;
	}

	if (GET_LEVEL(ch) < SINFO.min_level[(int) GET_CLASS(ch)])
	{
		send_to_char("That spell is beyond your actual power!\r\n", ch);
		return;
	}

	if ( *arg1 && is_number(arg1) )
	{ 
		tnum = atoi(arg1);
			
		if ( tnum < 1 || tnum > MAX_SAME_SPELL )
		{
			ch_printf(ch, "You can memorize the same spell only %d times.\r\n", MAX_SAME_SPELL);
			return;
		}
	}
	
	if ( NumSpellsMem(ch) + tnum >= MaxSpellsCanMem(ch) )
	{
		send_to_char("You've already memorized all the spells you can handle at the moment.\r\n", ch);
		return;
	}

	if ( !CanMemSpell(ch, spellnum, tnum) )
	{
		ch_printf(ch, "You cannot memorize '%s' any further.\r\n", SINFO.name);
		return;
	}
	
	if (!(GET_COND(ch, FULL)) || !(GET_COND(ch, THIRST)))
	{
		send_to_char("You can't concentrate with ", ch);
		if (!GET_COND(ch, FULL))
		{
			if (!GET_COND(ch, THIRST))
				send_to_char("your stomach aching and your throat dry.\r\n", ch);
			else
				send_to_char("your stomach rumbling.\r\n", ch);
		}
		else
			send_to_char("your throat this parched.\r\n", ch);
		return;
	}

	send_to_char("You open your spellbook and begin studying it intently.\r\n", ch);
	act("$n opens $s spellbook and begins studying it intently.", TRUE, ch, NULL, NULL, TO_ROOM);

	if ( ch->in_obj && OBJ_FLAGGED(ch->in_obj, ITEM_FAST_MAGIC) )
	{
		ch_printf(ch, "You immediately memorize '&b&3%s&0'.\r\n", SINFO.name);
		MEMORIZED(ch, spellnum) += tnum;
		return;
	}

	CREATE(me, MEM_EVENT, 1);
	me->ch		= ch;
	me->spell	= spellnum;
	me->num		= tnum;

	ch->action	= event_create(mem_event, me, MEM_DELAY * tnum);
	SET_BIT(AFF_FLAGS(ch), AFF_MEMORIZING);
}

/*
 * called from ACMD(do_forget)
 *
 * return TRUE if *argument is a spell name, FALSE otherwise
 */
bool forget_spell( CHAR_DATA *ch, char *argument )
{
	char *spellname = get_spell_name(argument);
	int sn;

	if ( !spellname )
		return (FALSE);

	sn = find_skill_num(spellname);
	if ( sn < 0 || sn >= MAX_SPELLS )
		return (FALSE);

	if ( !MEMORIZED(ch, sn) )
		send_to_char("You don't have that spell memorized.\r\n", ch);
	else
	{
		ch_printf(ch, "You forget the '&b&3%s&0' spell.\r\n", spellname);
		MEMORIZED(ch, sn) = 0;
	}

	return (TRUE);
}


/* ****************************************************** */
/* Study / Copy functions                                 */
/* ****************************************************** */

void LearnOrImprove(CHAR_DATA *ch, int spellnum)
{
	if (spellnum < 0 || spellnum >= MAX_SPELLS)
		return;

	if (GET_SKILL(ch, spellnum) >= LEARNED(ch))
	{
		send_to_char("You are already learned in that area.\r\n", ch);
		return;
	}

	if ( GET_SKILL(ch, spellnum) > 0 )
	{
		GET_SKILL(ch, spellnum) += number(1, 3);
		GET_SKILL(ch, spellnum) = MIN(GET_SKILL(ch, spellnum), LEARNED(ch));
		ch_printf(ch, "You improve your knowledge of the spell '&b&3%s&0'.\r\n", SINFO.name);
		return;
	}

	if ( MEMMING_CLASS(ch) )
	{
		if (AddNewSpell(ch, spellnum))
			return;
		send_to_char("You carefully copy the spell on you spellbook.\r\n", ch);
		act("$n carefully write something on $s spellbook.", TRUE, ch, NULL, NULL, TO_ROOM);
	}

	if (GET_LEVEL(ch) >= SINFO.min_level[(int) GET_CLASS(ch)])
	{
		SET_SKILL(ch, spellnum, (5 + number(1, (int_app[GET_INT(ch)].learn) / 2)));
		ch_printf(ch, "You have learned the art of '&b&3%s&0'.\r\n", SINFO.name);
	}
	else
		ch_printf(ch, "The '&b&3%s&0' spell is beyond your actual power.\r\n", SINFO.name);
}

/*
 * *POWERFUL*
 *
 * allow to study wands, staves, scrolls, spellbooks
 * to learn theirs spells.
 *
 * FB 03-06-2002 -- players now can study maps
 */
ACMD(do_study)	 /* original study code for Rom 2.4 is by Absalom */
{
	OBJ_DATA *obj;
	BOOK_PAGE *page;
	char *burngone = "$p burns brightly and is gone.";
	char *burnpage = "$p's page glows brightly and the writing disappear.";
	char *s;
	int spellnum;
	
	argument = one_argument(argument, arg);
	
	if (!*arg)
	{
		send_to_char("You can study magical or enchanted objects to acquire new knowledge.\r\n", ch);
		send_to_char(
			"Usage: study <object>\r\n"
			"     : study <spelbook> 'spellname'\r\n"
			"     : study <map obj>\r\n"
			, ch);
		return;
	}
	
	if ( !GET_SKILL(ch, SKILL_STUDY) )
	{
		send_to_char("You have no idea on how to study.\r\n", ch);
		return;
	}

	/* study scrolls, wands and staves */
	if (!(obj = get_obj_in_list_vis_rev(ch, arg, NULL, ch->last_carrying)))
	{
		sprintf(buf, "You don't seem to have %s %s.\r\n", AN(arg), arg);
		send_to_char(buf, ch);
		return;
	}
	
	if (GET_OBJ_TYPE(obj) == ITEM_MAP)
	{
		// sea course map
		if ( GET_OBJ_VAL(obj, 0) == 0 )
		{
			int learn_course(CHAR_DATA *ch, int cnum);
			int ret;
			
			ret = learn_course(ch, GET_OBJ_VAL(obj, 1));
			
			switch (ret)
			{
			case -1:
				log("SYSERR: invalid course vnum in obj.");
				break;
			case 0:
				send_to_char("You already know that course.\r\n", ch);
				break;
			default:
				send_to_char("You learn a new sea course.\r\n", ch);
				break;
			}
		}
		else
		{
			log("SYSERR: unknown map type.");
			send_to_char("This is an unknown map.\r\n", ch);
		}
		return;
	}

	/* list here item type allowed to be studied */
	if (GET_OBJ_TYPE(obj) != ITEM_STAFF  && GET_OBJ_TYPE(obj) != ITEM_WAND &&
	    GET_OBJ_TYPE(obj) != ITEM_SCROLL && GET_OBJ_TYPE(obj) != ITEM_SPELLBOOK)
	{
		send_to_char("You can only study scrolls, wands, staves and spellbooks.\r\n", ch);
		return;
	}

	/* sorcerer check to prevent studying his spellbook */
	if ( MEMMING_CLASS(ch) )
	{
		OBJ_DATA *sbook = FindObjSpellbook(ch->first_carrying, GET_IDNUM(ch));

		if ( (sbook) && sbook == obj )
		{
			send_to_char("Are you stupid??\r\n", ch);
			return;
		}
	}

	separate_obj(obj);

	act("You start studying $p.", TRUE, ch, obj, NULL, TO_CHAR);
	act("$n start studying $p.", TRUE, ch, obj, NULL, TO_ROOM);
	
	if (GET_LEVEL(ch) + 5 < GET_OBJ_LEVEL(obj))
	{
		send_to_char("You cannot glean any knowledge from it.\r\n", ch);
		act(burngone, FALSE, NULL, obj, 0, TO_ROOM);
		extract_obj(obj);
		return;
	}
	
	if (!success(ch, NULL, SKILL_STUDY, 0))
	{
		send_to_char("You fail to extract any knowledge from it.\r\n", ch);
		act(burngone, FALSE, NULL, obj, 0, TO_ROOM);
		extract_obj(obj);
		return;
	}
	
	switch (GET_OBJ_TYPE(obj))
	{
	case ITEM_SCROLL:
		if ( success(ch, NULL, SKILL_READ_MAGIC, 0) )
		{
			int sn;

			for (sn = 1; sn < 4; sn++)
				LearnOrImprove(ch, GET_OBJ_VAL(obj, sn));
		}
		else
			send_to_char("You fail to decifrate the magic writing.\r\n", ch);

		act(burngone, FALSE, NULL, obj, 0, TO_ROOM);
		extract_obj(obj);
		break;

	case ITEM_WAND:
	case ITEM_STAFF:
		LearnOrImprove(ch, GET_OBJ_VAL(obj, 3));
		act(burngone, FALSE, NULL, obj, 0, TO_ROOM);
		extract_obj(obj);
		break;

	case ITEM_SPELLBOOK:
		if ( !obj->special || !OBJ_FLAGGED(obj, ITEM_IS_SPELLBOOK) )
		{
			send_to_char("There is nothing to study in that.\r\n", ch);
			return;
		}

		if ( !*argument )
		{
			send_to_char("You must specify which spell of the spellbook you want to study.\r\n", ch);
			ListSpellsInBook(ch, (SPELLBOOK *) obj->special, TRUE);
			return;
		}

		s = get_spell_name(argument);
		if ( s == NULL )
		{
			send_to_char("You must specify which spell of the spellbook you want to study.\r\n", ch);
			ListSpellsInBook(ch, (SPELLBOOK *) obj->special, TRUE);
			return;
		}

		spellnum = find_skill_num(s);

		if ( !(page = BookHasSpell((SPELLBOOK *) obj->special, spellnum)) )
		{
			send_to_char("The spellbook doesn't contains that spell.\r\n", ch);
			return;
		}

		LearnOrImprove(ch, spellnum);

		act(burnpage, FALSE, ch, obj, NULL, TO_CHAR);
		act(burnpage, FALSE, ch, obj, NULL, TO_ROOM);
		MakePageBlank(page);

		break;
	}
}


/*
 * copy a spell from a scroll to the char spellbook
 */
void CopyScroll(CHAR_DATA *ch, OBJ_DATA *scroll, SPELLBOOK *book)
{
	char *burngone = "$p burns brightly and is gone.";
	int sn, spellnum, written = 0;

	if ( GET_OBJ_VAL(scroll, 1) == -1 )
	{
		send_to_char("The scroll doesn't contains any spell.\r\n", ch);
		act(burngone, FALSE, NULL, scroll, 0, TO_ROOM);
		extract_obj(scroll);
		return;
	}

	for (written = 0, sn = 1; sn < 4; sn++)
	{
		if ((spellnum = GET_OBJ_VAL(scroll, sn)) == -1)
			continue;

		if ( BookHasSpell(book, spellnum) )
		{
			ch_printf(ch, "You skip '%s' because your spellbook already contains this spell.\r\n",
				skill_name(spellnum));
			continue;
		}

		/*
		 * the only reason this return false is there's no blank pages on
		 * the spellbook, so we exit here
		 */
		if ( !AddSpellToBook(ch, book, spellnum) )
			break;

		written++;
	}

	if ( written )
	{
		send_to_char("You successfully copy the spells in your spellbook.\r\n", ch);
		act("$n carefully write something on $s spellbook.", TRUE, ch, NULL, NULL, TO_ROOM);
		act(burngone, FALSE, NULL, scroll, 0, TO_ROOM);
		extract_obj(scroll);
	}
}


/*
 * copy a spell from a spellbook to the char spellbook
 */
void CopySpellbook(CHAR_DATA *ch, OBJ_DATA *obj, SPELLBOOK *chbook, char *argument)
{
	SPELLBOOK *book;
	BOOK_PAGE *page;
	char *s, *burngone = "$p's page glows brightly and the writing disappear.";
	int spellnum, written = 0;

	s = get_spell_name(argument);
	if (s == NULL)
	{
		act("Which spell do you want to copy from $p?", TRUE, ch, obj, NULL, TO_CHAR);
		return;
	}

	book = (SPELLBOOK *) obj->special;

	if ( ( spellnum = find_skill_num(s) ) == NOTHING )
	{
		send_to_char("That spell does not exists.\r\n", ch);
		return;
	}

	if ( BookHasSpell(chbook, spellnum) )
	{
		ch_printf(ch, "Your spellbook already contains the '%s' spell.\r\n",
			skill_name(spellnum));
		return;
	}

	if ( !( page = BookHasSpell(book, spellnum) ) )
	{
		act("$p does not contain that spell.", TRUE, ch, obj, NULL, TO_CHAR);
		return;
	}

	if ( AddSpellToBook(ch, chbook, spellnum) )
	{
		send_to_char("You successfully copy the spells in your spellbook.\r\n", ch);
		act("$n carefully write something on $s spellbook.", TRUE, ch, NULL, NULL, TO_ROOM);
		act(burngone, FALSE, ch, obj, NULL, TO_CHAR);
		MakePageBlank(page);

		check_damage_obj(ch, obj, 8);
	}
}


ACMD(do_copy)
{
	SPELLBOOK *book;
	OBJ_DATA *obj;
	char arg1[MAX_INPUT_LENGTH];

	if ( !MEMMING_CLASS(ch) )
	{
		send_to_char("You have no idea on how to copy spells from objects.", ch);
		if ( CASTING_CLASS(ch) )
			send_to_char(" Try studying it.\r\n", ch);
		send_to_char("\r\n", ch);
		return;
	}
		
	argument = one_argument(argument, arg1);

	if ( !(obj = get_obj_in_list_vis_rev(ch, arg1, NULL, ch->last_carrying)) )
	{
		ch_printf(ch, "You don't have %s %s.\r\n", AN(arg1), arg1);
		return;
	}

	if ( GET_OBJ_TYPE(obj) != ITEM_SCROLL && GET_OBJ_TYPE(obj) != ITEM_SPELLBOOK )
	{
		send_to_char("You can copy spells only from scrolls or spellbooks.\r\n", ch);
		return;
	}

	if ( !(book = FindSpellbook(ch->first_carrying, GET_IDNUM(ch))) )
	{
		send_to_char("You don't have a spellbook to copy the spell to.\r\n", ch);
		return;
	}

	if ( !BookHasBlankPage(book) )
	{
		send_to_char("There isn't a blank page on your spellbook to write on.\r\n", ch);
		return;
	}

	if ( !success(ch, NULL, SKILL_READ_MAGIC, 0) )
	{
		send_to_char("You fail to decifrate the magic writing.\r\n", ch);
		return;
	}

	if ( GET_OBJ_TYPE(obj) == ITEM_SCROLL )
		CopyScroll(ch, obj, book);
	else if ( obj->special && OBJ_FLAGGED(obj, ITEM_IS_SPELLBOOK) )
		CopySpellbook(ch, obj, book, argument);
}


/* *************************************************************** */
/* Code for storing and using spells on magic items                */
/* *************************************************************** */

/*
 * if target object has the given spell stored, return
 * a pointer to the obj spell data
 */
OBJ_SPELLS_DATA *ItemHasSpell(OBJ_DATA *obj, int sn)
{
	OBJ_SPELLS_DATA *oSpell;

	if (!obj || !obj->special || !OBJ_FLAGGED(obj, ITEM_HAS_SPELLS))
		return (NULL);

	for (oSpell = (OBJ_SPELLS_DATA *) obj->special; oSpell; oSpell = oSpell->next)
	{
		if ( oSpell->spellnum == sn )
			break;
	}

	return (oSpell);
}

/*
 * search all char equipped items of type ITEM_WORN
 * that can contains the given spell
 */
OBJ_SPELLS_DATA *GetCharItemsSpell(CHAR_DATA *ch, int sn)
{
	OBJ_SPELLS_DATA *oSpell;

	if ( GET_EQ(ch, WEAR_FINGER_R) && (oSpell = ItemHasSpell(GET_EQ(ch, WEAR_FINGER_R), sn)) )
	{
		check_damage_obj(ch, GET_EQ(ch, WEAR_FINGER_R), 2);
		return (oSpell);
	}

	if ( GET_EQ(ch, WEAR_FINGER_L) && (oSpell = ItemHasSpell(GET_EQ(ch, WEAR_FINGER_L), sn)) )
	{
		check_damage_obj(ch, GET_EQ(ch, WEAR_FINGER_L), 2);
		return (oSpell);
	}

	if ( GET_EQ(ch, WEAR_NECK_1) && (oSpell = ItemHasSpell(GET_EQ(ch, WEAR_NECK_1), sn)) )
	{
		check_damage_obj(ch, GET_EQ(ch, WEAR_NECK_1), 2);
		return (oSpell);
	}

	if ( GET_EQ(ch, WEAR_NECK_2) && (oSpell = ItemHasSpell(GET_EQ(ch, WEAR_NECK_2), sn)) )
	{
		check_damage_obj(ch, GET_EQ(ch, WEAR_NECK_2), 2);
		return (oSpell);
	}

	if ( GET_EQ(ch, WEAR_WRIST_R) && (oSpell = ItemHasSpell(GET_EQ(ch, WEAR_WRIST_R), sn)) )
	{
		check_damage_obj(ch, GET_EQ(ch, WEAR_WRIST_R), 2);
		return (oSpell);
	}

	if ( GET_EQ(ch, WEAR_WRIST_L) && (oSpell = ItemHasSpell(GET_EQ(ch, WEAR_WRIST_L), sn)) )
	{
		check_damage_obj(ch, GET_EQ(ch, WEAR_WRIST_L), 2);
		return (oSpell);
	}

	return (NULL);
}

/*
 * *POWERFUL*
 *
 * store a spell into an object.
 *
 * that allow to cast that spell thru the object
 */
ACMD(do_enchant)
{
	OBJ_DATA *obj, *pObj;
	OBJ_SPELLS_DATA *oSpell, *spells_list = NULL;
	char *s, arg1[MAX_INPUT_LENGTH];
	int spellnum;

	if ( !GET_SKILL(ch, SKILL_ENCHANT_ITEMS) )
	{
		send_to_char("You don't know how to enchant an item.\r\n", ch);
		return;
	}

	/* get: blank, spell name, target name */
	s = get_spell_name(argument);
	if (s == NULL)
	{
		send_to_char("Usage: enchant 'spellname' <object>\r\n", ch);
		return;
	}
	argument = strtok(NULL, "\0");

	spellnum = find_skill_num(s);
	if ( spellnum < 0 || spellnum >= MAX_SPELLS )
	{
		send_to_char("That spell doesn't exist.\r\n", ch);
		return;
	}

	one_argument(argument, arg1);

	if ( !*arg1 )
	{
		send_to_char("Usage: enchant 'spellname' <object>\r\n", ch);
		return;
	}

	if ( !(obj = get_obj_in_list_vis_rev(ch, arg1, NULL, ch->last_carrying)) )
	{
		ch_printf(ch, "You don't have %s %s.\r\n", AN(arg1), arg1);
		return;
	}

	if ( GET_OBJ_TYPE(obj) != ITEM_WORN && GET_OBJ_TYPE(obj) != ITEM_WEAPON )
	{
		send_to_char("You can enchant, in different ways, only weapons and jewelry.\r\n", ch);
		return;
	}

	if ( GET_OBJ_TYPE(obj) == ITEM_WEAPON && !IS_SET(SINFO.routines, MAG_DAMAGE))
	{
		send_to_char("A weapon can be enchanted only with offensive spells.\r\n", ch);
		return;
	}

	if ( obj->special )
	{
		if (!OBJ_FLAGGED(obj, ITEM_HAS_SPELLS)) 
		{
			act("$p cannot be enchanted.", TRUE, ch, obj, NULL, TO_CHAR);
			return;
		}

		if ( ItemHasSpell(obj, spellnum) )
		{
			act("$p is already enchanted with that spell.", TRUE, ch, obj, NULL, TO_CHAR);
			return;
		}
	}

	separate_obj(obj);

	if (IS_MORTAL(ch))
	{
		if ( !success(ch, NULL, SKILL_ENCHANT_ITEMS, 0) )
		{
			act("You fail in enchanting $p, and it explodes.", FALSE, ch, obj, NULL, TO_CHAR);
			act("$n casts a spell upon $p, but something goes badly wrong.", FALSE, ch, obj, NULL, TO_ROOM);
			damage(ch, ch, number(20, 50), TYPE_UNDEFINED);
			extract_obj(obj);
			return;
		}
	}
	
	pObj = clone_object(obj);

	/* make the object unique */
	if ( !OBJ_FLAGGED(pObj, ITEM_UNIQUE) )
	{
		/* clone obj increase the copy number of the prototype */
		if (pObj->item_number != NOTHING)
			obj_index[pObj->item_number].number--;
		pObj->item_number	= NOTHING;
		SET_BIT(GET_OBJ_EXTRA(pObj), ITEM_UNIQUE);
	}

	if ( pObj->special )
		spells_list = (OBJ_SPELLS_DATA *) pObj->special;

	CREATE(oSpell, OBJ_SPELLS_DATA, 1);
	oSpell->spellnum	= spellnum;
	oSpell->percent		= MIN(100, number(60, 80) + GET_LEVEL(ch));
	oSpell->level		= MAX(1, GET_LEVEL(ch) - number(1, 4));

	oSpell->next		= spells_list;
	spells_list			= oSpell;

	pObj->special		= spells_list;

	SET_BIT(GET_OBJ_EXTRA(pObj), ITEM_MAGIC | ITEM_HAS_SPELLS);

	act("You enchant $p, that glows white for a moment.", TRUE, ch, pObj, NULL, TO_CHAR);
	act("$n enchants $p, that glows white for a moment.", TRUE, ch, pObj, NULL, TO_ROOM);
	extract_obj(obj);
	obj_to_char(pObj, ch);

	/* TODO
	 * character must have drawbacks for enchanting items...
	 */
}
