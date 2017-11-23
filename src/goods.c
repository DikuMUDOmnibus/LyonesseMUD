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
 * File: goods.c                                                          *
 *                                                                        *
 * Goods code                                                             *
 * Trading Post code                                                      *
 * Market code                                                            *
 * Market Affections code                                                 *
 *                                                                        *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"

#define IS_VALID_GOOD(sn)							\
				((sn) >= 0 && (sn) < top_goods		\
					&& goods_table[(sn)]			\
					&& goods_table[(sn)]->name)

#define IS_VALID_TYPE(sn)							\
				((sn) >= 0 && (sn) < top_good_type	\
					&& good_types[(sn)]				\
					&& good_types[(sn)]->name)

#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)					\
				if (!str_cmp(word, literal))		\
				{									\
				    field  = value;					\
				    fMatch = TRUE;					\
				    break;							\
				}

/* external funcs */
SPECIAL(tradingpost);
char	*get_spell_name(char *argument);
int		calc_trade_value(int gnum, int mnum);
int		Season(void);
void	fwrite_building(BUILDING_DATA *bld);
void	calc_prod(MARKET_DATA *pMk, GOOD_DATA *pGood, TRP_GOOD *tGood, MARKET_GOOD *pGM);
void	calc_price(MARKET_DATA *pMk, GOOD_DATA *pGood, MARKET_GOOD *pGM);


/* globals */
MARKET_DATA		*first_market		= NULL;
MARKET_DATA		*last_market		= NULL;
TRADING_POST	*first_trading_post	= NULL;
TRADING_POST	*last_trading_post	= NULL;
GOOD_TYPE		*good_types[MAX_TYPE_CODE];
GOOD_DATA		*goods_table[MAX_GOOD];
MARKET_GOOD		*GoodsMarkets[MAX_GOOD][MAX_MARKET];

int		top_goods		= 0;
int		top_good_type	= 0;
int		top_tp_vnum		= 0;
int		reset_markets	= 0;

/* local functions */
MARKET_DATA *find_market(int vnum);
void SaveGoodsMarketsTable(bool bTime);
void set_mk_aff(int mnum, int nwhat, int bitv, int durat, float value);


/* =========================================================================== */
/* Goods Type Code                                                             */
/* =========================================================================== */

/* *************************************************************************** */
/* Goods Type finding code                                                     */
/* *************************************************************************** */

GOOD_TYPE *get_good_type(int vnum)
{
	int gtn;

	for (gtn = 0; gtn < top_good_type; gtn++)
	{
		if (!IS_VALID_TYPE(gtn))
			continue;

		if (good_types[gtn]->vnum == vnum)
			break;
	}

	if (gtn >= top_good_type)
		return (NULL);

	return (good_types[gtn]);
}

/* *************************************************************************** */
/* Goods Type creation code                                                    */
/* *************************************************************************** */

GOOD_TYPE *new_good_type(void)
{
	GOOD_TYPE *gtype;
	int x;

	CREATE(gtype, GOOD_TYPE, 1);

	gtype->name			= NULL;
	gtype->vnum			= NOTHING;
	gtype->elasticity	= 1;
	gtype->cons_speed	= 1.00;

	for (x = 0; x < NUM_SEASONS; x++)
		gtype->prod_avg[x] = 0;

	return (gtype);
}


/* *************************************************************************** */
/* Goods Type load code                                                        */
/* *************************************************************************** */

GOOD_TYPE *fread_one_gtype(FILE *fp)
{
	GOOD_TYPE *gtype = new_good_type();
	char *word;
	bool fMatch;

	for ( ; ; )
	{
		word   = feof(fp) ? "End" : fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0]))
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol( fp );
			break;

		case 'C':
			if (!str_cmp(word, "Cons_speed"))
			{
				int num = fread_number(fp);
				float calc = (float)num / 100;

				gtype->cons_speed = calc;

				fMatch = TRUE;
				break;
			}
			break;

		case 'E':
			KEY("Elast",		gtype->elasticity,	fread_number(fp));
			if (!strcmp(word, "End"))
				return (gtype);

		case 'N':
			KEY("Name",			gtype->name,		str_dup(fread_word(fp)));
			break;

		case 'P':
			if (!strcmp(word, "Prod"))
			{
				int x;

				for (x = 0; x < NUM_SEASONS; x++)
					gtype->prod_avg[x] = fread_number(fp);

				fMatch = TRUE;
				break;
			}
			break;

		case 'V':
			KEY("Vnum",			gtype->vnum,		fread_number(fp));
			break;

		}
	}

	return (NULL);
}

void LoadGoodTypes(void)
{
	FILE *fp;
	char fname[128];
	char letter;
	char *word;
	int i;

	sprintf(fname, "%sgoodtypes.data", LIB_GOODS);
	if (!(fp = fopen(fname, "r")))
	{
		log("SYSERR: LoadGoodTypes() - cannot open good types file %s.", fname);
		return;
	}

	/* Pre-initialize goods_table with blanks */
	for (i = 0; i < MAX_TYPE_CODE; i++)
		good_types[i] = NULL;

	top_good_type = 0;

	for ( ; ; )
	{
		letter = fread_letter(fp);
		if (letter == '*')
		{
			fread_to_eol(fp);
			continue;
		}
		
		if (feof(fp))
			break;

		if (letter != '#')
		{
			log("SYSERR: LoadGoodTypes() - # not found.");
			break;
		}
		
		word = fread_word(fp);

		if (!strcmp(word, "TYPE"))
		{
			if (top_good_type >= MAX_TYPE_CODE)
			{
				log("SYSERR: LoadGoodTypes() - more good types than MAX_TYPE_CODE %d", MAX_TYPE_CODE);
				fclose(fp);
				exit(1);
			}
			good_types[top_good_type] = fread_one_gtype(fp);
			top_good_type++;
			continue;
		}
		else if (!strcmp(word, "END"))	// Done
			break;
		else
		{
			log("SYSERR: LoadGoodTypes() - bad section %s", word);
			continue;
		}
	}

	fclose(fp);
}


/* =========================================================================== */
/* Goods Code                                                                  */
/* =========================================================================== */

/* *************************************************************************** */
/* Goods creation code                                                         */
/* *************************************************************************** */

GOOD_DATA *new_good(void)
{
	GOOD_DATA *pGood;

	CREATE(pGood, GOOD_DATA, 1);
	pGood->name			= NULL;
	pGood->unit			= NULL;
	pGood->short_descr	= NULL;
	pGood->gtype		= NULL;
	pGood->code			= 0;
	pGood->cost			= 0;
	pGood->life			= -1;
	pGood->vnum			= 0;
	pGood->mkvnum		= 0;
	pGood->weight		= 0;
	pGood->docg			= 0;

	return (pGood);
}

void clear_goods(GOOD_DATA *pGood)
{
	if (pGood->name)		STRFREE(pGood->name);
	if (pGood->unit)		STRFREE(pGood->unit);
	if (pGood->short_descr)	STRFREE(pGood->short_descr);
	pGood->gtype = NULL;
}


/* *************************************************************************** */
/* Sort Goods Table                                                            */
/* *************************************************************************** */

/* Function used by qsort to sort goods */
int goods_comp(GOOD_DATA **g1, GOOD_DATA **g2)
{
	GOOD_DATA *pGood1 = (*g1);
	GOOD_DATA *pGood2 = (*g2);
	
	if (!pGood1 && pGood2)
		return (1);
	if (pGood1 && !pGood2)
		return (-1);
	if (!pGood1 && !pGood2)
		return (0);
	return (str_cmp(pGood1->name, pGood2->name));
}

/*
 * Ordina la tabella delle abilita' con qsort
 */
void sort_goods_table(void)
{
	log("  Sorting Goods Table.");
	qsort( &goods_table[0], MAX_GOOD - 1, sizeof(GOOD_DATA *),
		(int(*)(const void *, const void *)) goods_comp );
}

/* *************************************************************************** */
/* Load Goods from file code                                                   */
/* *************************************************************************** */

int parse_g_code(char *cname)
{
	if (!str_cmp(cname, "corn"))	return (TYPE_CORN);
	if (!str_cmp(cname, "flour"))	return (TYPE_FLOUR);
	if (!str_cmp(cname, "fabric"))	return (TYPE_FABRIC);
	if (!str_cmp(cname, "hide"))	return (TYPE_HIDE);
	if (!str_cmp(cname, "dye"))		return (TYPE_DYE);
	if (!str_cmp(cname, "metal"))	return (TYPE_METAL);
	if (!str_cmp(cname, "ore"))		return (TYPE_ORE);
	if (!str_cmp(cname, "spice"))	return (TYPE_SPICE);
	if (!str_cmp(cname, "sugar"))	return (TYPE_SUGAR);
	if (!str_cmp(cname, "oil"))		return (TYPE_OIL);
	if (!str_cmp(cname, "wood"))	return (TYPE_WOOD);

	return (NOTHING);
}

GOOD_DATA *fread_one_good(FILE *fp)
{
	GOOD_DATA *pGood = new_good();
	char *word;
	bool fMatch;

	for ( ; ; )
	{
		word   = feof(fp) ? "End" : fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0]))
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol( fp );
			break;

		case 'C':
			if (!strcmp(word, "Code"))
			{
				//pGood->code = parse_g_code(fread_word(fp));
				pGood->code		= fread_number(fp);
				pGood->gtype	= get_good_type(pGood->code);
				fMatch = TRUE;
				break;
			}
			KEY("Cost",		pGood->cost,		fread_number(fp));
			break;

		case 'E':
			if (!strcmp(word, "End"))
			{
				if (!pGood->gtype)
				{
					log("SYSERR: good %d without type.", pGood->vnum);
					exit(1);
					return (NULL);
				}

				if (!pGood->short_descr || !*pGood->short_descr)
				{
					char gname[MAX_STRING_LENGTH];

					sprintf(gname, "%s %s", pGood->unit, pGood->name);
					pGood->short_descr = str_dup(gname);
				}

				return (pGood);
			}
			break;

		case 'L':
			KEY("Life",			pGood->life,		fread_number(fp));
			break;

		case 'M':
			KEY("Market",		pGood->mkvnum,		fread_number(fp));
			break;

		case 'N':
			KEY("Name",			pGood->name,		fread_string_nospace(fp));
			break;

		case 'S':
			KEY("Short_descr",	pGood->short_descr,	fread_string_nospace(fp));
			break;

		case 'U':
			KEY("Unit",			pGood->unit,		str_dup(fread_word(fp)));
			break;

		case 'V':
			KEY("Vnum",			pGood->vnum,		fread_number(fp));
			break;

		case 'W':
			KEY("Weight",		pGood->weight,		fread_number(fp));
			break;
		}

		if (!fMatch)
			log("fread_one_good: no match: %s", word);
	}

	return (NULL);
}

void LoadGoods(void)
{
	FILE *fp;
	char fname[128];
	char letter;
	char *word;
	int i;

	sprintf(fname, "%sgoods.data", LIB_GOODS);
	if (!(fp = fopen(fname, "r")))
	{
		log("SYSERR: LoadGoods() - cannot open goods file %s.", fname);
		return;
	}

	/* Pre-init the goods_table with blank goods */
	for (i = 0; i < MAX_GOOD; i++)
		goods_table[i] = NULL;

	top_goods = 0;

	for ( ; ; )
	{
		letter = fread_letter(fp);
		if (letter == '*')
		{
			fread_to_eol(fp);
			continue;
		}
		
		if (feof(fp))
			break;

		if (letter != '#')
		{
			log("SYSERR: LoadGoods() - # not found.");
			break;
		}
		
		word = fread_word(fp);

		if (!strcmp(word, "GOODS"))
		{
			if (top_goods >= MAX_GOOD)
			{
				log("SYSERR: LoadGoods() - more goods than MAX_GOOD %d", MAX_GOOD);
				fclose(fp);
				return;
			}
			goods_table[top_goods] = fread_one_good(fp);

			top_goods++;
			continue;
		}
		else if (!strcmp(word, "END"))	// Done
			break;
		else
		{
			log("SYSERR: LoadGoods() - bad section %s", word);
			continue;
		}
	}

	fclose(fp);
}


/* *************************************************************************** */
/* Goods finding code                                                          */
/* *************************************************************************** */

/* Check for exact matches only */
int bsearch_goods_exact(const char *name, int first, int top)
{
	int sn;
	
	for (;;)
	{
		sn = (first + top) >> 1;
		if (!IS_VALID_GOOD(sn))
			return (-1);
		if (!str_cmp(name, goods_table[sn]->name))
			return (sn);
		if (first >= top)
			return (-1);
		if (strcmp(name, goods_table[sn]->name) < 1)
			top = sn - 1;
		else
			first = sn + 1;
	}

	return (-1);
}

GOOD_DATA *get_good(int gnum)
{
	int gn;

	for (gn = 0; gn < top_goods; gn++)
	{
		if (!IS_VALID_GOOD(gn))
			continue;

		if (goods_table[gn]->vnum == gnum)
			break;
	}

	if (gn >= top_goods)
		return (NULL);

	return (goods_table[gn]);
}


GOOD_DATA *get_good_by_name(char *gname)
{
	int gn;

	if ((gn = bsearch_goods_exact(gname, 0, top_goods - 1)) == -1)
		return (NULL);

	return (goods_table[gn]);
}

/*
 * recursive reverse search for goods object <gnum>
 */
OBJ_DATA *get_good_object(OBJ_DATA *list, int gnum)
{
	OBJ_DATA *obj = NULL;

	for (obj = list; obj; obj = obj->prev_content)
	{
		if (GET_OBJ_TYPE(obj) == ITEM_GOODS)
		{
			if (GET_OBJ_VAL(obj, 0) == gnum)
				break;
		}

		if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
			get_good_object(obj->last_content, gnum);
	}

	return (obj);
}

/* *************************************************************************** */
/* Goods Display Routines                                                      */
/* *************************************************************************** */

void list_goods(CHAR_DATA *ch, int mode)
{
	char gbuf[MAX_STRING_LENGTH*2];
	int gn;

	strcpy(gbuf,
		"List of goods table:\r\n"
		"-----------------------------------------------\r\n"
		"Name                       Unit        Cost\r\n"
		"-----------------------------------------------\r\n"
		);

	for (gn = 0; gn < top_goods; gn++)
	{
		if (!IS_VALID_GOOD(gn))
			continue;

		if (mode)
		{
			if (goods_table[gn]->cost < mode)
				continue;
		}

		sprintf(gbuf + strlen(gbuf),
			"%-25s  %-10s  %4d  %5d  %5d  %5d\r\n",
			goods_table[gn]->name, goods_table[gn]->unit,
			goods_table[gn]->cost);
	}

	page_string(ch->desc, gbuf, 1);
}

/* *************************************************************************** */
/* Goods object creation code                                                  */
/* *************************************************************************** */

OBJ_DATA *create_good_obj(GOOD_DATA *pGood, int amount)
{
	OBJ_DATA *obj;
	char buf[MAX_STRING_LENGTH];
	
	if (amount < 0)
		amount = 1;

	obj = create_obj();
	
	// setup goods name
	sprintf(buf, "%s %s", pGood->name, pGood->unit);
	if (obj->name)
		free(obj->name);
	obj->name = str_dup(buf);
	
	// setup goods short description
	sprintf(buf, "%s %s of %s", AN(pGood->unit), pGood->unit, pGood->name);
	if (obj->short_description)
		free(obj->short_description);
	obj->short_description = str_dup(buf);
	
	// setup goods description
	sprintf(buf, "%s %s of %s was placed here.",
		UAN(pGood->unit), pGood->unit, pGood->name);
	if (obj->description)
		free(obj->description);
	obj->description = str_dup(buf);
	
	obj->action_description	= NULL;

	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_UNIQUE);
	SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
	
	GET_OBJ_TYPE(obj)		= ITEM_GOODS;
	GET_OBJ_WEIGHT(obj)		= pGood->weight;
	GET_OBJ_COST(obj)		= pGood->cost;
	GET_OBJ_TIMER(obj)		= pGood->life;
	GET_OBJ_QUALITY(obj)	= number(30, 80);

	obj->count				= amount;

	GET_OBJ_VAL(obj, 0)	= pGood->vnum;
	
	return (obj);
}


ACMD(do_newgoods)
{
	OBJ_DATA *obj;
	GOOD_DATA *pGood;
	char *g;

	/* get goods name */
	g = get_spell_name(argument);
	if (g == NULL)
	{
		send_to_char("Goods names must be enclosed in the Holy Magic Symbols: '\r\n", ch);
		return;
	}
	argument = strtok(NULL, "\0");

	// show the entire list of goods
	if (!str_cmp(g, "list"))
	{
		int mode;

		if (*argument)
		{
			one_argument(argument, arg);
			if ( !*arg || !is_number(arg) )
				mode = 0;
			else
				mode = atoi(arg);
		}
		else
			mode = 0;

		list_goods(ch, mode);
		return;
	}

	if (!(pGood = get_good_by_name(g)))
	{
		ch_printf(ch, "There is no goods called '%s'.\r\n", g);
		return;
	}

	obj = create_good_obj(pGood, 1);
	obj = obj_to_room(obj, ch->in_room);

	ch_printf(ch, "You create %s %s of %s out of the blue.\r\n",
		AN(pGood->unit), pGood->unit, pGood->name);
	sprintf(buf, "$n creates %s %s of %s out of the blue.\r\n",
		AN(pGood->unit), pGood->unit, pGood->name);

	act(buf, FALSE, ch, NULL, NULL, TO_ROOM);
}


/* =========================================================================== */
/* Trading Post Code                                                           */
/* =========================================================================== */

/* *************************************************************************** */
/* Trading Post finding code                                                   */
/* *************************************************************************** */

TRADING_POST *find_trading_post(int vnum)
{
	TRADING_POST *pTp;

	if (vnum < 1)
		return (NULL);

	for (pTp = first_trading_post; pTp; pTp = pTp->next)
	{
		if (pTp->vnum == vnum)
			break;
	}

	return (pTp);
}

TRP_GOOD *tp_has_goods(TRADING_POST *pTp, int gnum)
{
	TRP_GOOD *tGood;

	for (tGood = pTp->first_tpgood; tGood; tGood = tGood->next)
		if (tGood->goods_vnum == gnum)
			break;

	return (tGood);
}

/* *************************************************************************** */
/* Trading Post creation code                                                  */
/* *************************************************************************** */

TRADING_POST *new_trading_post(void)
{
	TRADING_POST *pTp;

	CREATE(pTp, TRADING_POST, 1);
	pTp->next			= NULL;
	pTp->prev			= NULL;
	pTp->next_in_market	= NULL;
	pTp->market			= NULL;
	pTp->first_tpgood	= NULL;
	pTp->last_tpgood	= NULL;
	pTp->vnum			= NOTHING;
	pTp->type			= TP_GCLOSE;

	// add to the list
	LINK(pTp, first_trading_post, last_trading_post, next, prev);

	return (pTp);
}

/* *************************************************************************** */
/* Load Trading Post from file code                                            */
/* *************************************************************************** */

TRADING_POST *fread_one_tp(FILE *fp)
{
	TRADING_POST *tp = new_trading_post();
	char *word;
	bool fMatch;

	for ( ; ; )
	{
		word   = feof(fp) ? "End" : fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0]))
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol(fp);
			break;

		case 'E':
			if (!strcmp(word, "End"))
				return (tp);
			break;

		case 'G':
			if (!strcmp(word, "Good"))
			{
				TRP_GOOD *tGood;
				GOOD_DATA *pGood;
				int vnum;

				fMatch = TRUE;

				vnum = fread_number(fp);

				if (!(pGood = get_good(vnum)))
				{
					log("SYSERR: fread_one_tp() - wrong good vnum %d.", vnum);
					exit(1);
				}

				CREATE(tGood, TRP_GOOD, 1);
				tGood->next			= NULL;
				tGood->prev			= NULL;
				tGood->goods_vnum	= vnum;

				tGood->quantity		= fread_number(fp);
				tGood->prev_qty		= fread_number(fp);
				tGood->stock		= fread_number(fp);

				// add good to tp
				LINK(tGood, tp->first_tpgood, tp->last_tpgood, next, prev);

				// assign unassigned goods to the tp market
				if (tGood->stock && tp->market && !pGood->mkvnum)
					pGood->mkvnum	= tp->market->vnum;

				// setup GM if needed
				if (!GoodsMarkets[pGood->vnum][tp->market->vnum])
				{
					log("SYSERR: fread_one_tp() - missing GoodsMarkets data for good %d - market %d.", pGood->vnum, tp->market->vnum);
					exit(1);
				//	CREATE(GoodsMarkets[pGood->vnum][tp->market->vnum], MARKET_GOOD, 1);
				}

				// setup GM.total_tp
				GoodsMarkets[pGood->vnum][tp->market->vnum]->total_tp++;
				// increase GM.qty
				GoodsMarkets[pGood->vnum][tp->market->vnum]->qty += tGood->quantity;

				break;
			}
			break;

		case 'M':
			if (!strcmp(word, "Market"))
			{
				int mnum = fread_number(fp);

				fMatch = TRUE;

				if (mnum == -1)
				{
					log("ECONOMY: Trading Post %d without market vnum.", tp->vnum);
					exit(1);
				}

				if (!(tp->market = find_market(mnum)))
				{
					log("ECONOMY: Trading Post %d unknown market vnum %d.", tp->vnum, mnum);
					exit(1);
				}

				// increase count of tp
				tp->market->num_of_tp++;

				// add to market tp list
				tp->next_in_market	= tp->market->tp_list;
				tp->market->tp_list	= tp;

				break;
			}
			break;

		case 'T':
			KEY("Type",	tp->type,	fread_number(fp));
			break;

		case 'V':
			KEY("Vnum",	tp->vnum,	fread_number(fp));
			break;

		}

		if (!fMatch)
			log("fread_one_tp: no match: %s", word);
	}

	return (NULL);
}

void LoadTradingPost(void)
{
	FILE *fp;
	char fname[128];
	char letter;
	char *word;

	sprintf(fname, "%stradingpost.data", LIB_GOODS);
	if (!(fp = fopen(fname, "r")))
	{
		log("SYSERR: LoadTradingPost() - cannot open file %s.", fname);
		return;
	}

	top_tp_vnum = 0;

	for ( ; ; )
	{
		letter = fread_letter(fp);
		if (letter == '*')
		{
			fread_to_eol(fp);
			continue;
		}
		
		if (feof(fp))
			break;

		if (letter != '#')
		{
			log("SYSERR: LoadTradingPost() - # not found.");
			break;
		}
		
		word = fread_word(fp);

		if (!strcmp(word, "TRADINGPOST"))
		{
			TRADING_POST *pTp;

			if (!(pTp = fread_one_tp(fp)))
			{
				log("SYSERR: LoadTradingPost() - error in reading trading post.");
				fclose(fp);
				exit(1);
			}

			if (pTp->vnum > top_tp_vnum)
				top_tp_vnum = pTp->vnum;
		}
		else if (!strcmp( word, "END"))		// Done
			break;
		else
		{
			log("SYSERR: LoadTradingPost() - bad section %s", word);
			continue;
		}
	}

	fclose(fp);

	if (reset_markets)
		SaveGoodsMarketsTable(0);
}

/* *************************************************************************** */
/* Save Trading Post to file code                                              */
/* *************************************************************************** */

void SaveTradingPost(void)
{
	FILE *fp;
	TRADING_POST *tp;
	TRP_GOOD *tGood;
	char fname[128];

	if (!first_trading_post)
		return;

	sprintf(fname, "%stradingpost.data", LIB_GOODS);
	if (!(fp = fopen(fname, "w")))
	{
		log("SYSERR: SaveTradingPost() - cannot open file %s.", fname);
		return;
	}

	for (tp = first_trading_post; tp; tp = tp->next)
	{
		fprintf(fp, "#TRADINGPOST\n");
		fprintf(fp, "Vnum          %d\n", tp->vnum);
		fprintf(fp, "Market        %d\n", tp->market ? tp->market->vnum : -1);
		fprintf(fp, "Type          %d\n", tp->type);

		for (tGood = tp->first_tpgood; tGood; tGood = tGood->next)
		{
			if (tGood->quantity <= 0 && tGood->prev_qty <= 0 && !tGood->stock)
				continue;

			fprintf(fp, "Good          %d %d %d %d\n",
				tGood->goods_vnum, tGood->quantity, tGood->prev_qty, tGood->stock);
		}
		fprintf(fp, "End\n\n");
	}

	fprintf(fp, "#END\n");
	fclose(fp);
}


/* *************************************************************************** */
/* IMMS Command                                                                */
/* *************************************************************************** */

const char *tp_type_desc[] =
{
  "closed",
  "restricted",
  "limited",
  "free"
};

SHIP_DATA *find_ship( sh_int vnum );

MARKET_DATA *locate_bld_market(BUILDING_DATA *pBld)
{
	MARKET_DATA *pMk = NULL;
	int iDist = 0;

	if (!pBld->in_room)
		return (NULL);

	if (IS_WILD(pBld->in_room))
	{
		for (pMk = first_market; pMk; pMk = pMk->next)
		{
			iDist = (int) distance(GET_X(pBld), GET_Y(pBld), pMk->heart.x, pMk->heart.y);

			if (iDist <= pMk->size)
				break;
		}
	}
	else
	{
		COORD_DATA coord;

		coord = zone_table[pBld->in_room->zone].wild.z_start;

		if (coord.y == 0 && coord.x == 0)
		{
			log("SYSERR: cannot retrieve wild position for zone %d.", zone_table[pBld->in_room->zone].number);
			return (NULL);
		}

		for (pMk = first_market; pMk; pMk = pMk->next)
		{
			iDist = (int) distance(coord.x, coord.y, pMk->heart.x, pMk->heart.y);

			if (iDist <= pMk->size)
				break;
		}
	}

	return (pMk);
}


ACMD(do_tp)
{
	TRADING_POST *pTp;
	GOOD_DATA *pGood;
	char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	char bloc[MAX_STRING_LENGTH];
	int tnum, gnum;

	argument = one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char(
			"Usage:\r\n"
			"-----------------------------------------------------------------------\r\n"
			" command                         description\r\n"
			"-----------------------------------------------------------------------\r\n"
			" tp virtual                      List of loaded Trading Posts\r\n"
			" tp list <market vnum>           List of Trading Posts in game (1)\r\n"
			" tp <vnum>                       Detailed infos\r\n"
			" tp <vnum> add <good> <stock>    Add good to the TP goods list (2)\r\n"
			" tp <vnum> remove <good>         Remove good from the TP goods list\r\n"
			" tp new <type>                   Setup a new TP of the given type(3)\r\n"
			" tp delete <vnum>                Destroy given Trading Post\r\n"
			"-----------------------------------------------------------------------\r\n"
			" (1) <market vnum> filter TP listing to those of the given market.\r\n"
			" (2) <stock> can be 0 or 1, depending if the good should be produced in\r\n"
			"     the Trading Post's market. If 0, you can omit the parameter.\r\n"
			" (3) to create a new Trading Post, you must be in a STORE type building\r\n"
			"-----------------------------------------------------------------------\r\n"
			, ch);
		return;
	}

	if (is_abbrev(arg, "virtual"))
	{
		char buf[MAX_STRING_LENGTH];

		strcpy(buf, "List of loaded Trading Posts.\r\n");
		for (pTp = first_trading_post; pTp; pTp = pTp->next)
		{
			sprintf(buf+strlen(buf), " Vnum: [%d]  Market: [%d]  Type: %s\r\n",
				pTp->vnum, (pTp->market ? pTp->market->vnum : NOWHERE), tp_type_desc[pTp->type]);
		}
		page_string(ch->desc, buf, 0);
		return;
	}

	if (is_abbrev(arg, "list"))
	{
		MARKET_DATA *pMk = NULL;
		int mnum;
		
		argument = one_argument(argument, arg);

		if (!*arg || !is_number(arg))
			mnum = 0;
		else
		{
			mnum = atoi(arg);

			if (mnum < 1 || mnum > MAX_MARKET)
				mnum = 0;
			else if	(!(pMk = find_market(mnum)))
				mnum = 0;
		}

		strcpy(buf, "List of Trading Posts in");
		if (mnum)
			sprintf(buf + strlen(buf), " market [%d] '%s'.\r\n", mnum, pMk->name);
		else
			strcat(buf, " game.\r\n");
		send_to_char(buf, ch);

		*buf = '\0';
		for (pTp = first_trading_post; pTp; pTp = pTp->next)
		{
			if (!pTp->market)
				continue;

			if (!pTp->in_room)
				continue;

			if (mnum && pTp->market->vnum != mnum)
				continue;

			// where is this TP ??
			if (IS_BUILDING(pTp->in_room))
				sprintf(bloc, "Building [%d]", pTp->in_room->extra_data->vnum);
			else if (IS_WILD(pTp->in_room))
				sprintf(bloc, "Wilderness [%d %d]", GET_Y(pTp), GET_X(pTp));
			else if (pTp->in_room->number != NOWHERE)
				sprintf(bloc, "Room [%d] '%s'", pTp->in_room->number, ROOM_NAME(pTp));
			else
				strcpy(bloc, "NOWHERE");

			sprintf(buf+strlen(buf), "  [%d]   Type: %s   Location: %s\r\n",
				pTp->vnum, tp_type_desc[pTp->type], bloc);
		}

		if (!*buf)
			send_to_char("No Trading Post found.\r\n", ch);
		else
			page_string(ch->desc, buf, 0);

		return;
	}

	if (is_abbrev(arg, "new"))
	{
		BUILDING_DATA *pBld;
		int nt;

		// building checks....
		if (!IN_BUILDING(ch))
		{
			send_to_char("You must be in a STORE type building to create a new TP.\r\n", ch);
			return;
		}

		pBld = ch->in_building;
		if (!pBld || pBld->type->vnum != BLD_STORE)
		{
			send_to_char("You must be in a STORE type building to create a new TP.\r\n", ch);
			return;
		}

		if (pBld->trp)
		{
			send_to_char("This building already host a Trading Post.\r\n", ch);
			return;
		}

		// TP type checks....
		argument = one_argument(argument, arg);

		if (!*arg)
		{
			send_to_char("You must specify a TP type.\r\nPossible types are:\r\n", ch);

			for (nt = 0; nt < NUM_TP_TYPES; nt++)
				ch_printf(ch, " - %s\r\n", tp_type_desc[nt]);

			return;
		}

		for (nt = 0; nt < NUM_TP_TYPES; nt++)
		{
			if (is_abbrev(arg, tp_type_desc[nt]))
				break;
		}

		if (nt >= NUM_TP_TYPES)
		{
			send_to_char("Unknown TP type.\r\nPossible types are:\r\n", ch);

			for (nt = 0; nt < NUM_TP_TYPES; nt++)
				ch_printf(ch, " - %s\r\n", tp_type_desc[nt]);

			return;
		}

		// setup the new TP
		pTp = new_trading_post();
		pTp->vnum			= ++top_tp_vnum;
		pTp->type			= nt;
		pTp->in_room		= pBld->rooms[ch->in_room->number];
		pTp->market			= locate_bld_market(pBld);
		
		if (pTp->market)
		{
			pTp->next_in_market		= pTp->market->tp_list;
			pTp->market->tp_list	= pTp;
		}

		// assing building tp status
		pBld->trp		= pTp;
		// assign to current building room the special procedure
		pBld->rooms[ch->in_room->number]->func	= tradingpost;

		fwrite_building(pBld);
		SaveTradingPost();

		return;
	}

	if (!str_cmp(arg, "delete"))
	{
		send_to_char("Not yet implemented, sorry.\r\n", ch);
		return;
	}

	if (!is_number(arg))
		return;

	tnum = atoi(arg);

	if (tnum < 0 || tnum > top_tp_vnum)
	{
		ch_printf(ch, "Invalid TP vnum %d.\r\n", tnum);
		return;
	}

	if (!(pTp = find_trading_post(tnum)))
	{
		ch_printf(ch, "Cannot find any TP with vnum %d.\r\n", tnum);
		return;
	}

	argument = two_arguments(argument, arg, arg2);

	// print detailed info about TP
	if (!*arg)
	{
		TRP_GOOD *tGood;
		GOOD_DATA *pGood;
		int price;

		if (pTp->in_room)
		{
			// where is this TP ??
			if (IS_BUILDING(pTp->in_room))
				sprintf(bloc, "Building [%d]", pTp->in_room->extra_data->vnum);
			else if (IS_WILD(pTp->in_room))
				sprintf(bloc, "Wilderness [%d %d]", GET_Y(pTp), GET_X(pTp));
			else if (IS_SHIP(pTp->in_room))
			{
				SHIP_DATA *pShip;
				
				if (!(pShip = find_ship(pTp->in_room->extra_data->vnum)))
					strcpy(bloc, "Ship <ERROR>");
				else
					sprintf(bloc, "Ship [%d] '%s'", pShip->vnum, pShip->name);
			}
			else if (pTp->in_room->number != NOWHERE)
				sprintf(bloc, "Room [%d]", pTp->in_room->number);
			else
				strcpy(bloc, "NOWHERE");
		}
		else
			strcpy(bloc, "NOWHERE");
		
		ch_printf(ch, "Trading Post %d    -- Market: %d '%s'\r\n",
			pTp->vnum,
			(pTp->market ? pTp->market->vnum : NOWHERE),
			(pTp->market ? pTp->market->name : "<ERROR>"));
		ch_printf(ch, "Type: %s   Location: %s\r\n", tp_type_desc[pTp->type], bloc);

		send_to_char("List of Goods in TP:\r\n", ch);
		*buf = '\0';
		for (tGood = pTp->first_tpgood; tGood; tGood = tGood->next)
		{
			if (!(pGood = get_good(tGood->goods_vnum)))
				continue;

			price = 0;

			if (GoodsMarkets[pGood->vnum][pTp->market->vnum])
				price = GoodsMarkets[pGood->vnum][pTp->market->vnum]->price;

			sprintf(buf+strlen(buf), "  Good: [%3d] %-20s - %s  Qty: %4d  Price: %6d\r\n",
				pGood->vnum, pGood->short_descr,
				(tGood->stock ? "X" : " "), tGood->quantity, price);
		}
		page_string(ch->desc, buf, 0);

		return; 
	}

	if (!isname(arg, "add free") || !*arg2 || !is_number(arg2))
	{
		send_to_char("Valid commands for TP are: add, free\r\n", ch);
		return;
	}

	gnum = atoi(arg2);
	if (!(pGood = get_good(gnum)))
	{
		ch_printf(ch, "Invalid good vnum %d.\r\n", gnum);
		return;
	}

	if (is_abbrev(arg, "add"))
	{
		MARKET_GOOD *pGM;
		TRP_GOOD *tGood;
		int stock, x;
	
		if (tp_has_goods(pTp, pGood->vnum))
		{
			send_to_char("Good already present in TP.\r\n", ch);
			return;
		}

		argument = one_argument(argument, arg);

		// get stock flag
		if (!*arg || !is_number(arg))
			stock = 0;
		else
			stock = atoi(arg);

		if (stock != 1)
			stock = 0;

		if (stock && pGood->mkvnum && pGood->mkvnum != pTp->market->vnum)
			stock = 0;

		CREATE(tGood, TRP_GOOD, 1);
		tGood->next				= NULL;
		tGood->prev				= NULL;
		tGood->goods_vnum		= pGood->vnum;
		tGood->prev_qty			= 0;
		tGood->quantity			= pGood->gtype->prod_avg[Season()] * pTp->market->var_mod.prod_var / pGood->gtype->cons_speed;
		tGood->stock			= stock;

		// add good to TP good list
		LINK(tGood, pTp->first_tpgood, pTp->last_tpgood, next, prev);

		// good previously unused.. initialize.
		if (!pGood->mkvnum)
			pGood->mkvnum		= pTp->market->vnum;

		// if not present, create GM data
		if (!(pGM = GoodsMarkets[pGood->vnum][pTp->market->vnum]))
		{ 
			CREATE(pGM, MARKET_GOOD, 1);
			pGM->comm_closure	= 1.00;
			pGM->comm_prod		= 1.00;
			pGM->demand			= 0;
			pGM->good_appet		= 1;
			pGM->price			= pGood->cost;
			pGM->qty			= 0;
			pGM->total_tp		= 0;
			
			GoodsMarkets[pGood->vnum][pTp->market->vnum] = pGM;
		}

		pGM->total_tp++;
		pGM->qty += tGood->quantity;

		// perform ten resets
		for (x = 0; x < 10; x++)
		{
			calc_prod(pTp->market, pGood, tGood, pGM);
			calc_price(pTp->market, pGood, pGM);
		}

		// save new data
		SaveGoodsMarketsTable(0);
		SaveTradingPost();

		send_to_char("Good succesfully added.\r\n", ch);
		return;
	}

	if (is_abbrev(arg, "remove"))
	{
		//MARKET_DATA *pMk;
		TRADING_POST *tpp;
		MARKET_GOOD *pGM;
		TRP_GOOD *tGood;

		if (!(tGood = tp_has_goods(pTp, pGood->vnum)))
		{
			ch_printf(ch, "Good %d is not present in TP %d.\r\n", pGood->vnum, pTp->vnum);
			return;
		}

		if (!(pGM = GoodsMarkets[pGood->vnum][pTp->market->vnum]))
		{
			log("SYSERR: good %d was in TP %d without GM data.\r\n", pGood->vnum, pTp->vnum);
			ch_printf(ch, "Good %d cannot be removed from TP %d.\r\n", pGood->vnum, pTp->vnum);
			return;
		}

		// look in every TP in this market for this good
		for (tpp = pTp->market->tp_list; tpp; tpp = tpp->next_in_market)
		{
			if ( tp_has_goods(tpp, pGood->vnum) )
				break;
		}

		// good is present elsewhere, do nothing..
		if (tpp)
			return;

		// remove GM info
		DISPOSE(GoodsMarkets[pGood->vnum][pTp->market->vnum]);

		// take care of GOOD info


		send_to_char("Good succesfully removed.\r\n", ch);
		return;
	}
}


/* =========================================================================== */
/* Markets Code                                                                */
/* =========================================================================== */

/* *************************************************************************** */
/* Market finding code                                                         */
/* *************************************************************************** */

MARKET_DATA *find_market(int vnum)
{
	MARKET_DATA *pMk = NULL;

	for (pMk = first_market; pMk; pMk = pMk->next)
	{
		if (pMk->vnum == vnum)
			break;
	}

	return (pMk);
}


MARKET_DATA *find_market_by_name(char *mname)
{
	MARKET_DATA *pMk = NULL;

	for (pMk = last_market; pMk; pMk = pMk->prev)
	{
		if (isname(mname, pMk->name))
			break;
	}

	return (pMk);
}

/* return the lowest price of a given good in a given market */
bool get_good_in_market(MARKET_DATA *pMk, int gnum)
{
	TRADING_POST *pTp;
	bool found = FALSE;

	// loop thru each market's trading post
	for (pTp = pMk->tp_list; pTp; pTp = pTp->next_in_market)
	{
		if (tp_has_goods(pTp, gnum))
		{
			found = TRUE;
			break;
		}
	}

	return (found);
}

/*
 * trova il numero di merci del tipo specificato che
 * vengono prodotte nel mercato indicato
 */
int get_market_gtype_prod(MARKET_DATA *pMk, int gtype)
{
	TRADING_POST *pTp;
	TRP_GOOD *tGood;
	GOOD_DATA *pGood;
	int amount = 0;

	// loop thru each trading post in given market
	for (pTp = pMk->tp_list; pTp; pTp = pTp->next_in_market)
	{
		// loop thru goods in trading post
		for (tGood = pTp->first_tpgood; tGood; tGood = tGood->next)
		{
			if (!(pGood = get_good(tGood->goods_vnum)))
				continue;

			if (pGood->code == gtype && tGood->stock)
				amount++;
		}
	}

	return (amount);
}

/* *************************************************************************** */
/* Market creation code                                                        */
/* *************************************************************************** */

MARKET_DATA *new_market(void)
{
	MARKET_DATA *pMk = NULL;

	CREATE(pMk, MARKET_DATA, 1);

	pMk->next					= NULL;
	pMk->prev					= NULL;
	pMk->name					= NULL;
	pMk->tp_list				= NULL;
	pMk->affect					= NULL;
	pMk->vnum					= NOTHING;
	pMk->affections				= 0;
	pMk->num_of_tp				= 0;
	pMk->var_real.prod_var		= 1.00;
	pMk->var_real.price_var		= 1.00;
	pMk->var_real.closure_var	= 1.00;
	pMk->var_mod.prod_var		= 1.00;
	pMk->var_mod.price_var		= 1.00;
	pMk->var_mod.closure_var	= 1.00;

	// add to the global list
	LINK(pMk, first_market, last_market, next, prev);

	return (pMk);
}

/* *************************************************************************** */
/* Load Market from file code                                                  */
/* *************************************************************************** */

MARKET_DATA *fread_one_market(FILE *fp)
{
	MARKET_DATA *pMk = new_market();
	char *word;
	bool fMatch;

	for ( ; ; )
	{
		word   = feof(fp) ? "End" : fread_word(fp);
		fMatch = FALSE;

		switch (UPPER(word[0]))
		{
		case '*':
			fMatch = TRUE;
			fread_to_eol(fp);
			break;

		case 'A':
			if (!strcmp(word, "Affection"))
			{
				int bitv, nwhat, dur, tmp;
				float value;

				bitv		= fread_number(fp);
				nwhat		= fread_number(fp);
				dur			= fread_number(fp);
				tmp			= fread_number(fp);
				value = (float) tmp / 100;

				set_mk_aff(pMk->vnum, nwhat, bitv, dur, value);

				fMatch = TRUE;
				break;
			}
			break;

		case 'H':
			if (!strcmp(word, "Heart"))
			{
				pMk->heart.y	= fread_number(fp);
				pMk->heart.x	= fread_number(fp);
				fMatch = TRUE;
				break;
			}
			break;

		case 'E':
			if (!strcmp(word, "End"))
				return (pMk);
			break;

		case 'N':
			KEY("Name",				pMk->name,				str_dup(fread_word(fp)));
			break;

		case 'S':
			KEY("Size",				pMk->size,				fread_number(fp));
			break;

		case 'V':
			KEY("Vnum",				pMk->vnum,				fread_number(fp));
			if (!strcmp(word, "Var_real_clos"))
			{
				int num;
				float calc;

				num = fread_number(fp);
				calc = (float)num / 100;
				pMk->var_real.closure_var	= calc;
				fMatch = TRUE;
				break;
			}
			if (!strcmp(word, "Var_real_prod"))
			{
				int num;
				float calc;

				num = fread_number(fp);
				calc = (float)num / 100;
				pMk->var_real.prod_var		= calc;
				fMatch = TRUE;
				break;
			}
			if (!strcmp(word, "Var_real_price"))
			{
				int num;
				float calc;

				num = fread_number(fp);
				calc = (float)num / 100;
				pMk->var_real.price_var		= calc;
				fMatch = TRUE;
				break;
			}
			/*
			if (!strcmp(word, "Var_mod_clos"))
			{
				int num;
				float calc;

				num = fread_number(fp);
				calc = (float)num / 100;
				pMk->var_mod.closure_var	= calc;
				fMatch = TRUE;
				break;
			}
			if (!strcmp(word, "Var_mod_prod"))
			{
				int num;
				float calc;

				num = fread_number(fp);
				calc = (float)num / 100;
				pMk->var_mod.prod_var		= calc;
				fMatch = TRUE;
				break;
			}
			if (!strcmp(word, "Var_mod_price"))
			{
				int num;
				float calc;

				num = fread_number(fp);
				calc = (float)num / 100;
				pMk->var_mod.price_var		= calc;
				fMatch = TRUE;
				break;
			}
			*/
			break;
		}

		if (!fMatch)
			log("fread_one_market: no match: %s", word);
	}

	return (NULL);
}

void LoadMarkets(void)
{
	FILE *fp;
	char fname[128];
	char letter;
	char *word;

	sprintf(fname, "%smarkets.data", LIB_GOODS);
	if (!(fp = fopen(fname, "r")))
	{
		log("SYSERR: LoadMarkets() - cannot open file %s.", fname);
		return;
	}

	for ( ; ; )
	{
		letter = fread_letter(fp);
		if (letter == '*')
		{
			fread_to_eol(fp);
			continue;
		}
		
		if (feof(fp))
			break;

		if (letter != '#')
		{
			log("SYSERR: LoadMarkets() - # not found.");
			break;
		}
		
		word = fread_word(fp);

		if (!strcmp(word, "MARKET"))
		{
			if (!fread_one_market(fp))
			{
				log("SYSERR: LoadMarkets() - error in reading market.");
				fclose(fp);
				exit(1);
			}
		}
		else if (!strcmp( word, "END"))		// Done
			break;
		else
		{
			log("SYSERR: LoadMarkets() - bad section %s", word);
			continue;
		}
	}

	fclose(fp);
}

/* *************************************************************************** */
/* Save Market to file code                                                    */
/* *************************************************************************** */

void SaveMarkets(void)
{
	FILE *fp;
	MARKET_DATA *pMk;
	char fname[128];

	if (!first_market)
		return;

	sprintf(fname, "%smarkets.data", LIB_GOODS);
	if (!(fp = fopen(fname, "w")))
	{
		log("SYSERR: SaveMarkets() - cannot open file %s.", fname);
		return;
	}

	for (pMk = first_market; pMk; pMk = pMk->next)
	{
		fprintf(fp, "#MARKET\n");
		fprintf(fp, "Vnum             %d\n",	pMk->vnum);
		fprintf(fp, "Name             %s\n",	pMk->name);
		fprintf(fp, "Heart            %hd %hd\n",	pMk->heart.y, pMk->heart.x);
		fprintf(fp, "Size             %d\n",	pMk->size);
		fprintf(fp, "Var_real_clos    %d\n",	(int)(pMk->var_real.closure_var * 100));
		fprintf(fp, "Var_real_prod    %d\n",	(int)(pMk->var_real.prod_var * 100));
		fprintf(fp, "Var_real_price   %d\n",	(int)(pMk->var_real.price_var * 100));
//		fprintf(fp, "Var_mod_clos     %d\n",	(int)(pMk->var_mod.closure_var * 100));
//		fprintf(fp, "Var_mod_prod     %d\n",	(int)(pMk->var_mod.prod_var * 100));
//		fprintf(fp, "Var_mod_price    %d\n",	(int)(pMk->var_mod.price_var * 100));

		if (pMk->affect)
		{
			MARKET_AFF *pAff = NULL;

			for (pAff = pMk->affect; pAff; pAff = pAff->next)
			{
				fprintf(fp, "Affection        %d %hd %hd %d\n", 
					pAff->bitvector, pAff->what, pAff->duration, (int)(pAff->modifier * 100));
			}
		}
		fprintf(fp, "End\n\n");
	}

	fprintf(fp, "#END\n");
	fclose(fp);
}

/* *************************************************************************** */
/* Market Affections code                                                      */
/* *************************************************************************** */


NAME_NUMBER mk_gdr_aff[] =
{
  {"pestilence",	MKT_PESTILENCE},
  {"insects",		MKT_INSECT},
  {"drought",		MKT_DROUGHT},
  {"flood",			MKT_FLOOD},
  {"war",			MKT_WAR},
  {"\n",			0}
};

NAME_NUMBER mk_gdr_modif[] =
{
  {"production",	MOD_PROD},
  {"price",			MOD_PRICE},
  {"closure",		MOD_CLOSURE},
  {"\n",			0}
};


const char *mk_aff_descr[] =
{
  "EXTRA_PROD",
  "MALUS_PROD",
  "EXTRA_PRICE",
  "MALUS_PRICE",
  "PESTILENCE",
  "INSECTS",
  "DROUGHT",
  "FLOOD",
  "WAR",
  "\n"
};

const char *mk_aff_what[] =
{
  "NONE",
  "PRODUCTION",
  "PRICE",
  "CLOSURE",
  "\n"
};


void affect_to_market(MARKET_DATA *pMk, MARKET_AFF *pAff)
{
	// set affection bitvector
	SET_BIT(pMk->affections, pAff->bitvector);

	// apply value modification
	switch (pAff->what)
	{
	case MOD_CLOSURE:
		pMk->var_mod.closure_var	+= pAff->modifier;
		break;
	case MOD_PROD:
		pMk->var_mod.prod_var		+= pAff->modifier;
		break;
	case MOD_PRICE:
		pMk->var_mod.price_var		+= pAff->modifier;
		break;
	}

	// add to the list
	pAff->next	= pMk->affect;
	pMk->affect	= pAff;
}

void affect_from_market(MARKET_DATA *pMk, MARKET_AFF *pAff)
{
	MARKET_AFF *temp = NULL;

	// remove affection bitvector
	REMOVE_BIT(pMk->affections, pAff->bitvector);

	// undo value modification
	switch (pAff->what)
	{
	case MOD_CLOSURE:
		pMk->var_mod.closure_var	-= pAff->modifier;
		break;
	case MOD_PROD:
		pMk->var_mod.prod_var		-= pAff->modifier;
		break;
	case MOD_PRICE:
		pMk->var_mod.price_var		-= pAff->modifier;
		break;
	}

	// remove from the list
	REMOVE_FROM_LIST(pAff, pMk->affect, next);
	pAff->next		= NULL;
	// take care
	DISPOSE(pAff);
}

MARKET_AFF *market_aff_by_bitv(MARKET_DATA *pMk, int bitv)
{
	MARKET_AFF *pAff = NULL;

	for (pAff = pMk->affect; pAff; pAff = pAff->next)
		if (pAff->bitvector == bitv)
			break;

	return (pAff);
}

void set_mk_aff(int mnum, int nwhat, int bitv, int durat, float value)
{
	MARKET_DATA *pMk;
	MARKET_AFF *pAff = NULL;

	if (!(pMk = find_market(mnum)))
		return;

	pAff = market_aff_by_bitv(pMk, bitv);

	// it's not there... setup a new one
	if (!pAff || pAff->what != nwhat)
	{
		CREATE(pAff, MARKET_AFF, 1);
		pAff->next			= NULL;
		pAff->bitvector		= bitv;
		pAff->duration		= durat;
		pAff->modifier		= value;
		pAff->what			= nwhat;
		
		affect_to_market(pMk, pAff);
	}
	// update existing affection
	else
	{
		if (durat != MUD_DAYS_PER_YEAR)
			pAff->duration	+= durat;
		pAff->modifier		+= value;

		// apply value modification
		switch (pAff->what)
		{
		case MOD_CLOSURE:
			pMk->var_mod.closure_var	+= value;
			break;
		case MOD_PROD:
			pMk->var_mod.prod_var		+= value;
			break;
		case MOD_PRICE:
			pMk->var_mod.price_var		+= value;
			break;
		}
	}
}

// called every mud day
void UpdateMarketAffections(void)
{
	MARKET_DATA *pMk;
	MARKET_AFF *pAff = NULL, *pAff_next = NULL;

	for (pMk = first_market; pMk; pMk = pMk->next)
	{
		for (pAff = pMk->affect; pAff; pAff = pAff_next)
		{
			pAff_next = pAff->next;

			if (pAff->duration == -1)
				continue;

			if (--pAff->duration <= 0)
				affect_from_market(pMk, pAff);
		}
	}

	SaveMarkets();
}

/* automatic affections.. rolled once every three mud months... */
void RollMarketAffections(void)
{
	MARKET_DATA *pMk;
	int type, bitv, dur, nwhat;
	float value = 0;
	//char dflags[128], swhat[128];

	for (pMk = first_market; pMk; pMk = pMk->next)
	{
		if (!pMk->size)
			continue;

		for (type = 0; type < 4; type++)
		{
			if (number(1, 12) > 1)
				continue;
			
			bitv	= (1 << type);
			dur		= MUD_DAYS_PER_YEAR;

			switch (type)
			{
			case 0:	nwhat = MOD_PROD;	value = 0.1F;	break;
			case 1:	nwhat = MOD_PROD;	value = -0.1F;	break;
			case 2:	nwhat = MOD_PRICE;	value = 0.1F;	break;
			case 3:	nwhat = MOD_PRICE;	value = -0.1F;	break;
			}

			set_mk_aff(pMk->vnum, nwhat, bitv, dur, value);

			//sprintbit(bitv, mk_aff_descr, dflags);
			//sprinttype(nwhat, mk_aff_what, swhat);
			//log("Economy - settata auto affection %s (mod %s) sul mercato %d.",
			//	dflags, swhat, pMk->vnum);
		}
	}
}


// print info on markets
ACMD(do_market)
{
	MARKET_DATA *pMk;
	MARKET_GOOD *pGM;
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	int gn;

	argument = one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char(
			"Usage:\r\n"
			"-----------------------------------------------------------------------\r\n"
			" command                          description\r\n"
			"-----------------------------------------------------------------------\r\n"
			" market list                      List of markets in game.\r\n"
			" market info                      Show global trade and production info\r\n"
			" market <vnum> <gm>               Detailed info for market <vnum>\r\n"
			" market <name> <gm>               Detailed info for market <name>\r\n"
			"-----------------------------------------------------------------------\r\n"
			" <gm> if any data is present print Goods/Markets info\r\n"
			"-----------------------------------------------------------------------\r\n"
			, ch);
		return;
	}

	if (!str_cmp(arg, "info"))
	{
		extern char *newspaper;

		if (newspaper)
			page_string(ch->desc, newspaper, 0);
		else
			send_to_char("Trade Newspaper not present.\r\n", ch);
		return;
	}

	if (!str_cmp(arg, "list"))
	{
		send_to_char("Current markets in game:\r\n", ch);
		for (pMk = first_market; pMk; pMk = pMk->next)
		{
			if (!pMk->size)
				continue;

			ch_printf(ch, 
				"[%d] %-30s - Center: [%4d %4d]\r\n",
				pMk->vnum, pMk->name, pMk->heart.y, pMk->heart.x);
		}
		return;
	}

	if (is_number(arg))
	{
		if (!(pMk = find_market(atoi(arg))))
		{
			ch_printf(ch, "There isn't a market with vnum %d.\r\n", atoi(arg));
			return;
		}
	}
	else
	{
		if (!(pMk = find_market_by_name(arg)))
		{
			ch_printf(ch, "There isn't a market called '%s'.\r\n", arg);
			return;
		}
	}

	//extra check
	if (!pMk)	return;

	sprintf(buf, "Market [%d] %s\r\n", pMk->vnum, pMk->name);
	sprintf(buf + strlen(buf), "Heart coord: [%4d %4d]  Size: %d\r\n",
		pMk->heart.y, pMk->heart.x, pMk->size);
	sprintf(buf + strlen(buf), "Number of Trading Posts: %d\r\n",
		pMk->num_of_tp);
	sprintf(buf + strlen(buf), "Default Modifier - Prod: [%5.2f]  Price: [%5.2f]  Clos: [%5.2f]\r\n",
		pMk->var_real.prod_var, pMk->var_real.price_var, pMk->var_real.closure_var);
	sprintf(buf + strlen(buf), "Current Modifier - Prod: [%5.2f]  Price: [%5.2f]  Clos: [%5.2f]\r\n",
		pMk->var_mod.prod_var, pMk->var_mod.price_var, pMk->var_mod.closure_var);
	send_to_char(buf, ch);

	if (pMk->affect)
	{
		MARKET_AFF *pAff = NULL;
		char dflags[128], swhat[128];

		sprintf(buf, "Affections:\r\n");

		for (pAff = pMk->affect; pAff; pAff = pAff->next)
		{
			sprintbit(pAff->bitvector, mk_aff_descr, dflags);
			sprinttype(pAff->what, mk_aff_what, swhat);
			sprintf(buf+strlen(buf), "  Bit: [%-12s]  Modif: [%-12s]  Duration: [%4d]  Value: [%5.2f]\r\n",
				dflags, swhat, pAff->duration, pAff->modifier);
		}

		send_to_char(buf, ch);
	}

	argument = one_argument(argument, arg);

	if (!*arg)
		return;

	sprintf(buf, "Goods / Market Info:\r\n");

	for (gn = 0; gn < MAX_GOOD; gn++)
	{
		if (!(pGM = GoodsMarkets[gn][pMk->vnum]))
			continue;

		sprintf(buf+strlen(buf), " Good: [%3d] - Qty:[%5d]  Price:[%5d]  Demand:[%5.2f]\r\n",
			gn, pGM->qty, pGM->price, pGM->demand);
	}

	send_to_char(buf, ch);

}

// allow imms to set gdr affections on markets
ACMD(do_mkaff)
{
	MARKET_DATA *pMk;
	char arg[MAX_INPUT_LENGTH];
	int nr;
	int bitv = 0, modif = 0, dur = 0;
	float value = 0;

	argument = one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("Usage: mkaff <market> <affection> <modif> <duration> <value>\r\n", ch);
		return;
	}

	// get market
	if (is_number(arg))
	{
		if (!(pMk = find_market(atoi(arg))))
		{
			ch_printf(ch, "There isn't a market with vnum %d.\r\n", atoi(arg));
			return;
		}
	}
	else
	{
		if (!(pMk = find_market_by_name(arg)))
		{
			ch_printf(ch, "There isn't a market called '%s'.\r\n", arg);
			return;
		}
	}

	// get affection
	argument = one_argument(argument, arg);
	if (!*arg)
	{
		send_to_char("Available affections are:\r\n", ch);
		
		for (nr = 0; *mk_gdr_aff[nr].name != '\n'; nr++)
			ch_printf(ch, "%s\r\n", mk_gdr_aff[nr].name);
		return;
	}
	
	for (nr = 0; *mk_gdr_aff[nr].name != '\n'; nr++)
	{
		if ( isname(arg, mk_gdr_aff[nr].name) )
			break;
	}
	
	if (*mk_gdr_aff[nr].name == '\n')
	{
		send_to_char("Unknown affection.\r\nAvailable affections are:\r\n", ch);
		
		for (nr = 0; *mk_gdr_aff[nr].name != '\n'; nr++)
			ch_printf(ch, "%s\r\n", mk_gdr_aff[nr].name);
		return;
	}

	bitv = mk_gdr_aff[nr].number;


	// get modifier
	argument = one_argument(argument, arg);
	if (!*arg)
	{
		send_to_char("Available modifiers are:\r\n", ch);
		
		for (nr = 0; *mk_gdr_modif[nr].name != '\n'; nr++)
			ch_printf(ch, "%s\r\n", mk_gdr_modif[nr].name);
		return;
	}
	
	for (nr = 0; *mk_gdr_modif[nr].name != '\n'; nr++)
	{
		if ( isname(arg, mk_gdr_modif[nr].name) )
			break;
	}
	
	if (*mk_gdr_modif[nr].name == '\n')
	{
		send_to_char("Unknown modifier.\r\nAvailable modifiers are:\r\n", ch);
		
		for (nr = 0; *mk_gdr_modif[nr].name != '\n'; nr++)
			ch_printf(ch, "%s\r\n", mk_gdr_modif[nr].name);
		return;
	}

	modif = mk_gdr_modif[nr].number;


	// get duration
	argument = one_argument(argument, arg);
	if (!*arg || !is_number(arg))
	{
		ch_printf(ch, "Duration must be a number from 1 to %d\r\n", MUD_DAYS_PER_YEAR);
		return;
	}

	dur = atoi(arg);
	if (dur < 1 || dur > MUD_DAYS_PER_YEAR)
	{
		ch_printf(ch, "Duration must be a number from 1 to %d\r\n", MUD_DAYS_PER_YEAR);
		return;
	}

	// get value.. handle decimals..
	argument = one_argument(argument, arg);
	if (!*arg)
	{
		send_to_char("Value must be a number from -2.0 to 2.0\r\n", ch);
		return;
	}

	value = atof(arg);

	if (value < -2.0 || value > 2.0)
	{
		send_to_char("Value must be a number from -2.0 to 2.0\r\n", ch);
		return;
	}

	set_mk_aff(pMk->vnum, modif, bitv, dur, value);	
	send_to_char("Done.\r\n", ch);

	SaveMarkets();
}


ACMD(do_goodinfo)
{
	GOOD_DATA *pGood;
	MARKET_DATA *pMk;
	MARKET_GOOD *pGM;
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	int gn, mn, price;

	one_argument(argument, arg);

	if (!*arg || !is_number(arg))
	{
		send_to_char("Usage: ginfo <good vnum>\r\n", ch);
		return;
	}

	gn = atoi(arg);
	
	if (!(pGood = get_good(gn)))
	{
		ch_printf(ch, "There is no good with vnum %d.\r\n", gn);
		return;
	}

	sprintf(buf, "Good: '%s' [%d]\r\n", pGood->name, gn);
	sprintf(buf+strlen(buf), "Unit: %s  Weigth: %d  Cost: %d\r\n",
		pGood->unit, pGood->weight, pGood->cost);
	sprintf(buf+strlen(buf), "Type: (%d) %s\r\n", pGood->gtype->vnum, pGood->gtype->name);
	sprintf(buf+strlen(buf), "Cons_speed: %6.2f   Elasticity: %d\r\n",
		pGood->gtype->cons_speed, pGood->gtype->elasticity);
	sprintf(buf+strlen(buf), "Production: Wn:%d  Sp:%d  Sm:%d  At:%d\r\n",
		pGood->gtype->prod_avg[0], pGood->gtype->prod_avg[1],
		pGood->gtype->prod_avg[2], pGood->gtype->prod_avg[3]);
	send_to_char(buf, ch);

	ch_printf(ch, "Commercial data:\r\n", pGood->name, gn);

	for (mn = 0; mn < MAX_MARKET; mn++)
	{
		if (!(pMk = find_market(mn)))
			continue;
		
		if (!str_cmp(pMk->name, "noname") || !pMk->size)
			continue;
		
		price = calc_trade_value(gn, mn);

		if (!(pGM = GoodsMarkets[gn][mn]))
			sprintf(buf, "  Market: [%3d] %-15s - Qty: [%5d] Sell Price: [%4d] Buy Price: [%4d] Demand: [%6.2f]\r\n",
				mn, pMk->name, 0, 0, price, 0);
		else
			sprintf(buf, "  Market: [%3d] %-15s - Qty: [%5d] Sell Price: [%4d] Buy Price: [%4d] Demand: [%6.2f]\r\n",
				mn, pMk->name, pGM->qty, pGM->price, price, pGM->demand);
		send_to_char(buf, ch);
	}
}


/* =========================================================================== */
/* Trading Code                                                                */
/* =========================================================================== */

/* *************************************************************************** */
/* Boot Setup Market-Goods data code                                           */
/* *************************************************************************** */

void SaveGoodsMarketsTable(bool bTime)
{
	FILE *fp;
	MARKET_GOOD *pGM;
	GOOD_DATA *pGood;
	MARKET_DATA *pMk;
	char fname[128];
	int gn, mn;

	if (bTime)
	{
		sprintf(fname, "%sgoodsmarkets.txt", LIB_GOODS);
		fp = fopen(fname, "a+");
	}
	else
	{
		sprintf(fname, "%sgoodsmarkets.data", LIB_GOODS);
		fp = fopen(fname, "w");
	}

	if (!fp)
	{
		log("SYSERR: SaveGoodsMarketsTable() - cannot open file %s.", fname);
		return;
	}

	for (mn = 0; mn < MAX_MARKET; mn++)
	{
		pMk = find_market(mn);

		for (gn = 0; gn < MAX_GOOD; gn++)
		{
			if (!(pGM = GoodsMarkets[gn][mn]))
				continue;

			if (!(pGood = get_good(gn)))
				continue;

			if (bTime)
			{
				double calc = 0;
				
				if (pGM->qty && pGM->total_tp)
					calc = (double) pGM->qty / pGM->total_tp;

				fprintf(fp,
				 "G:%ld M:%ld [C:%4.2f P:%4.2f PP:%4.2f] (Stock=%d) : Qty=[%-4ld] Price=[%-4ld] Qty/TP=[%7.2f] Comm_prod=[%7.2f] Demand=[%7.2f]\n",
				 gn, mn, pMk->var_mod.closure_var, pMk->var_mod.prod_var, pMk->var_mod.price_var,
				 (pGood->mkvnum == mn ? 1 : 0),
				 pGM->qty, pGM->price, calc, pGM->comm_prod, pGM->demand);
			}
			else
			{
				fprintf(fp,
				 "#GM %d %d %d %d %d %hd %d %d\n",
				 mn, gn,
				 (int)(pGM->demand * 100), 0, pGM->price, pGM->good_appet,
				 (int)(pGM->comm_prod * 100), (int)(pGM->comm_closure * 100) );
			}
		}
	}

	if (!bTime)
	{
		fprintf(fp, "#END\n");
		fclose(fp);
		return;
	}

	for (mn = 0; mn < MAX_MARKET; mn++)
	{
		for (gn = 0; gn < MAX_GOOD; gn++)
		{
			if (!(pGM = GoodsMarkets[gn][mn]))
				continue;

			{
				double calc = 0;
				
				if (pGM->qty && pGM->total_tp)
					calc = (double) pGM->qty / pGM->total_tp;

				fprintf(fp, "%ld;%ld;%ld;%ld;%f;%f\n",
				 mn, gn,
				 pGM->qty, pGM->price, calc, pGM->comm_prod);
			}
		}
	}

	fprintf(fp, "#END\n");
	fclose(fp);
}


void LoadGoodsMarketsTable(void)
{
	FILE *fp;
	char fname[128];
	char letter;
	char *word;
	int gn, mn;

	sprintf(fname, "%sgoodsmarkets.data", LIB_GOODS);
	if (!(fp = fopen(fname, "r")))
	{
		log("SYSERR: LoadGoodsMarketsTable() - cannot open file %s.", fname);
		reset_markets = 1;
		return;
	}

	for ( ; ; )
	{
		letter = fread_letter(fp);
		if (letter == '*')
		{
			fread_to_eol(fp);
			continue;
		}
		
		if (feof(fp))
			break;

		if (letter != '#')
		{
			log("SYSERR: LoadGoodsMarketsTable() - # not found.");
			break;
		}

		word = fread_word(fp);

		if (!strcmp(word, "GM"))
		{
			int num;
			float calc;

			mn	= fread_number(fp);
			if (mn < 0 || mn > MAX_MARKET)
			{
				log("SYSERR: invalid market number %d.", mn);
				exit(1);
			}

			gn	= fread_number(fp);
			if (gn < 0 || gn > MAX_GOOD)
			{
				log("SYSERR: invalid good number %d.", gn);
				exit(1);
			}

			if (GoodsMarkets[gn][mn])
			{
				log("SYSERR: LoadGoodsMarketsTable() - duplicate GoodsMarkets info for G:%d M:%d.", gn, mn);
				GoodsMarkets[gn][mn]->demand		= 0.0;
				GoodsMarkets[gn][mn]->qty			= 0;
				GoodsMarkets[gn][mn]->price			= 0;
				GoodsMarkets[gn][mn]->comm_closure	= 0.0;
				GoodsMarkets[gn][mn]->comm_prod		= 0.0;
				GoodsMarkets[gn][mn]->good_appet	= 0;
			}
			else
				CREATE(GoodsMarkets[gn][mn], MARKET_GOOD, 1);

			num = fread_number(fp);
			calc = (float)num / 100;
			GoodsMarkets[gn][mn]->demand		= calc;
			GoodsMarkets[gn][mn]->qty			= fread_number(fp);
			GoodsMarkets[gn][mn]->price			= fread_number(fp);
			GoodsMarkets[gn][mn]->good_appet	= fread_number(fp);
			num = fread_number(fp);
			calc = (float)num / 100;
			GoodsMarkets[gn][mn]->comm_prod		= calc;
			num = fread_number(fp);
			calc = (float)num / 100;
			GoodsMarkets[gn][mn]->comm_closure	= calc;
		}
		else if (!strcmp( word, "END"))		// Done
			break;
		else
		{
			log("SYSERR: LoadGoodsMarketsTable() - bad section %s", word);
			continue;
		}
	}

	fclose(fp);
}


void BootMarkets(void)
{
	reset_markets	= 0;

	LoadMarkets();
	LoadGoodsMarketsTable();
}


/*
 * su "mercato x" la produzione e' "0-1 = buona, 1-2= alta, 3-4 = altissima,
 * -1,0 = bassa, -1 -2 = bassissima, -2 -4 = pessima" ed i prezzi sono "0-1 = buoni,
 * 1-2= alti, 3-4 = altissimi, -1,0 = bassi, -1 -2 = bassissimi, -2 -4 = pessimi"...
 * o qualcosa di simile...
 */

extern char *newspaper;

const char *trade_descr[] =
{
	"bad",				// -4 -2
	"low",				// -2 -1
	"poor",				// -1 0
	"normal",			// 0 1
	"good",				// 1 2
	"high"				// 2 4
};

int trade_descr_index(float val)
{
	int idx;

	if		(val < -2)		idx = 0;
	else if (val < -1)		idx = 1;
	else if (val < 0)		idx = 2;
	else if (val <= 1)		idx = 3;
	else if (val < 2)		idx = 4;
	else					idx = 5;

	return (idx);
}

void update_newspaper(void)
{
	FILE *fp;
	MARKET_DATA *pMk;
	int iprod, iprice;

	if (!(fp = fopen(NEWSPAPER_FILE, "w")))
		return;

	fprintf(fp,
		"Market                    Production               Prices\n"
		"---------------------------------------------------------------------------\n");

	for (pMk = first_market; pMk; pMk = pMk->next)
	{
		// skip unused markets
		if (!pMk->size)	continue;

		iprod	= trade_descr_index(pMk->var_mod.prod_var);
		iprice	= trade_descr_index(pMk->var_mod.price_var);

		fprintf(fp, "%-25s %-15s [%6.2f] %-15s [%6.2f]\n",
			pMk->name,
			trade_descr[iprod], pMk->var_mod.prod_var,
			trade_descr[iprice], pMk->var_mod.price_var
			);
	}

	fprintf(fp,
		"---------------------------------------------------------------------------\n");
	fclose(fp);
}

int file_to_string_alloc(const char *name, char **buf);

ACMD(do_newspaper)
{
	update_newspaper();
	file_to_string_alloc(NEWSPAPER_FILE, &newspaper);
}