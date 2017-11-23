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
 * File: economy.c                                                        *
 *                                                                        *
 * Economy code                                                           *
 *                                                                        *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include <math.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"

/* external globals */
extern GOOD_DATA	*goods_table[MAX_GOOD];
extern MARKET_GOOD	*GoodsMarkets[MAX_GOOD][MAX_MARKET];
extern TRADING_POST	*first_trading_post;
extern int			top_goods;

/* externa funcs */
GOOD_DATA *get_good(int gnum);
GOOD_DATA *get_good_by_name(char *gname);
MARKET_DATA *find_market(int vnum);
MARKET_DATA *find_market_by_name(char *mname);
TRADING_POST *find_trading_post(int vnum);
TRP_GOOD *tp_has_goods(TRADING_POST *pTp, int gnum);
OBJ_DATA *create_good_obj(GOOD_DATA *pGood, int amount);
OBJ_DATA *get_good_object(OBJ_DATA *list, int gnum);
bool	get_good_in_market(MARKET_DATA *pMk, int gnum);
int		can_take_obj(CHAR_DATA *ch, OBJ_DATA *obj);
int		Season(void);
char	*get_spell_name(char *argument);
void	SaveGoodsMarketsTable(bool bTime);
void	LoadGoodsMarketsTable(void);
void	UpdateMarketAffections(void);
void	RollMarketAffections(void);
void	SaveMarkets(void);
void	SaveTradingPost(void);


#define COEF_DIV		2000

/* locals */
int calc_trade_value(int gnum, int mnum);
void calc_price(MARKET_DATA *pMk, GOOD_DATA *pGood, MARKET_GOOD *pGM);

/* =============================================================== */

/* calcolo del consumo e della produzione */
void consume_produce_goods(void)
{
	MARKET_DATA *pMk;
	GOOD_DATA *pGood;
	MARKET_GOOD *pGM;
	TRADING_POST *pTp;
	TRP_GOOD *tGood;
	double gprod, cons, rand_cons;
	int qty;

	// ciclo in tutti i TP in tutti i MA
	for (pTp = first_trading_post; pTp; pTp = pTp->next)
	{
		// inizializza MA
		pMk = pTp->market;

		// ciclo in tutte le GTP del TP
		for (tGood = pTp->first_tpgood; tGood; tGood = tGood->next)
		{
			// se non trova la GO, passa alla GTP successiva
			if (!(pGood = get_good(tGood->goods_vnum)))
				continue;

			// se la GO non e' assegnata a nessun mercato, passa alla GTP successiva
			if (!pGood->mkvnum)
				continue;

			// inizializza GM, se non c'e' passa alla GTP successiva
			if (!(pGM = GoodsMarkets[pGood->vnum][pMk->vnum]))
				continue;

			cons = 0; rand_cons = 0; qty = 0;

			// get base production
			gprod = (double) pGood->gtype->prod_avg[Season()];
			// calc consumption
			cons = (double) (pGood->gtype->cons_speed * tGood->quantity);
				
			// se la GO e' a stock esegue la produzione interna
			if (tGood->stock)
			{
				int tmp1;
				double var2, var3;

				gprod *= pMk->var_mod.prod_var;
				// calcola Random_Consumption
				var3 = (double) abs(cons - gprod);
				if (var3 < 0.3)
					var3 = 0.3;
				tmp1 = number(-1, 1);
				var2 = ((double) tmp1) * ((double) cons) / var3;
				rand_cons = ((double) cons) + var2;

				// calcola la nuova quantita' della GTP
				var3 = (double) (2 * (tGood->quantity - tGood->prev_qty) / 3);
				var2 = ((double) tGood->quantity) + var3 + (((double)gprod) - rand_cons);
				qty = MAX(0, (int) var2);
			}
			// Produzione osmotica
			else
			{
				// inizializza MA relativo al mercato di origine
				MARKET_DATA *orig_mk = find_market(pGood->mkvnum);
				int iDist, tmp1;
				double var1, var2, var3;

				// se non esiste il MA, passa alla GTP successiva
				if (!orig_mk)
					continue;

				// calcola la distanza tra il MA di origine ed il MA in cui sono
				iDist = (int)distance(orig_mk->heart.x, orig_mk->heart.y, pMk->heart.x, pMk->heart.y);

				// calcola GM.Commercial_productivity
				gprod *= orig_mk->var_mod.prod_var;
				var3 = (double) gprod / COEF_DIV * iDist;
				var1 = (double) gprod - var3;
				var2 = (double) (var1 * pGM->comm_closure * pMk->var_mod.closure_var);
 				pGM->comm_prod = (float) var2;

				// calcola Random_Consumption
				var3 = abs(cons - pGM->comm_prod);
				if (var3 < 0.3)
					var3 = 0.3;
				tmp1 = number(-1, 1);
				var2 = (double) (tmp1 * cons / var3);
				rand_cons = (double) (cons + var2);

				// calcola la nuova quantita' della GTP
				var3 = (double) (2 * (tGood->quantity - tGood->prev_qty) / 3);
				var2 = (double) (tGood->quantity + var3 + (pGM->comm_prod - rand_cons));
				qty = MAX(0, (int) var2);
			}

			// aggiorna la quantita' del GM
			pGM->qty = MAX(0, pGM->qty - tGood->quantity);
			pGM->qty += qty;

			// aggiorna la quantita' della GTP
			tGood->prev_qty	= tGood->quantity;
			tGood->quantity = qty;
		}
	}
}


/* calcolo della domanda e dei prezzi */
void calc_demands_and_prices(FILE *fp)
{
	MARKET_DATA *pMk;
	GOOD_DATA *pGood;
	MARKET_GOOD *pGM;
	int mnum, gnum;
	int avg_qty, iDist;
	double tmpqty, negqty;
	int buyprice;

	// scorre tutti i MA
	for (mnum = 0; mnum < MAX_MARKET; mnum++)
	{
		// ottiene MA
		if (!(pMk = find_market(mnum)))
			continue;

		// skip unused markets
		if (!pMk->size) 
			continue;

		// scorre tutte le GO
		for (gnum = 0; gnum < MAX_GOOD; gnum++)
		{
			// ottiene GO
			if (!(pGood = get_good(gnum)))
				continue;
			// skip unused goods
			if (!pGood->mkvnum)
				continue;
			// ottiene GM
			if (!(pGM = GoodsMarkets[gnum][mnum]))
			{
				if (fp)
				{
					buyprice = calc_trade_value(gnum, mnum);

					//fprintf(fp, "Good: %d - Market %d - (Stock: %d) - P:No  - Qty: [%5d]  Sell Price: [%5d]  Buy Price: [%5d]\n",
					//	pGood->vnum, pMk->vnum, (pGood->market == mnum ? 1 : 0), 0, 0, buyprice);
					fprintf(fp, "%d;%d;%d;No;000.00;000.00;%d;%d;%d\n",
						pGood->vnum, pMk->vnum, (pGood->mkvnum == mnum ? 1 : 0), 0, 0, buyprice);
				}
				continue;
			}

			avg_qty = 0; iDist = 0;

			// GO interno
			if (pGood->mkvnum == mnum)
			{
				// calcola Market_Good_Medium_quantity_internal
				avg_qty = pGood->gtype->prod_avg[Season()] * pMk->var_mod.prod_var / pGood->gtype->cons_speed * pGM->total_tp;
			}
			// GO osmotico
			else
			{
				// ottiene MA di origine (solo per il calcolo della distanza)
				MARKET_DATA *orig_mk = find_market(pGood->mkvnum);

				// calcola Market_Good_Medium_quantity_external
				avg_qty = pGM->comm_prod / pGood->gtype->cons_speed * pGM->total_tp;
				// calcola la distanza tra i MA
				iDist = (int)distance(orig_mk->heart.x, orig_mk->heart.y, pMk->heart.x, pMk->heart.y);
			}

			// calcola la domanda
			if ( avg_qty == 0 )
				pGM->demand = 10;
			else
			{
				tmpqty = - pGM->qty;
				tmpqty /= avg_qty;
				negqty = exp(tmpqty);
				pGM->demand = (float) (10 * negqty);
			}

			// calcola il prezzo di vendita
			pGM->price = pGood->cost * pMk->var_mod.price_var + pGood->gtype->elasticity * (pGM->demand / 10) * pGood->cost + pGood->cost * pGM->good_appet * iDist / (COEF_DIV * 5 / 4) * pGood->gtype->elasticity;

			if (fp)
			{
				// Prezzo PG
				buyprice = pGM->price - pGood->cost * pGood->gtype->elasticity * (pGM->demand / 10);
				
				if (buyprice < 0)
					buyprice = pGood->cost / 2;

				//fprintf(fp, "Good: %d - Market %d - (Stock: %d) - P:Yes - Qty: [%5d]  Sell Price: [%5d]  Buy Price: [%5d]\n",
				//	pGood->vnum, pMk->vnum, (pGood->market == mnum ? 1 : 0), pGM->qty, pGM->price, buyprice);
				fprintf(fp, "%d;%d;%d;Yes;%d;%d;%d\n",
					pGood->vnum, pMk->vnum, (pGood->mkvnum == mnum ? 1 : 0), pGM->qty, pGM->price, buyprice);
			}
		}
	}
}

/* called from another_hour() in weather.c */
void SimulateEconomy(void)
{
	consume_produce_goods();
	calc_demands_and_prices(NULL);
	SaveGoodsMarketsTable(0);
	SaveMarkets();
	SaveTradingPost();
}

/* Immortal command to force X resets */
ACMD(do_economy)
{
	char arg[MAX_INPUT_LENGTH];
	int cn, count = 0;

	one_argument(argument, arg);

	if (!*arg || !is_number(arg)) 
		count = 1;
	else
		count = atoi(arg);
	count++;

	for (cn = 1; cn < count; cn++)
	{
		if (!(cn % 4))
			UpdateMarketAffections();

		consume_produce_goods();
		calc_demands_and_prices(NULL);

		if (!(cn % 102))
			RollMarketAffections();
	}
	
	SaveGoodsMarketsTable(0);
	SaveMarkets();
	SaveTradingPost();
}

/* ============================================================= */

/* =========================================================================== */
/* Trading code                                                                */
/* =========================================================================== */

void list_goods_in_tp(CHAR_DATA *ch)
{
	TRADING_POST *tp = NULL;
	TRP_GOOD *tGood;
	GOOD_DATA *pGood;
	MARKET_GOOD *pGM;
	char gname[MAX_STRING_LENGTH];

	if (!ch->in_building)
		return;

	if (!(tp = ch->in_building->trp))
		return;

	if (!tp->first_tpgood)
	{
		send_to_char("Nothing at stock at the moment.\r\n", ch);
		return;
	}

	send_to_char(
		"The following goods are available:\r\n"
		"-----------------------------------------------------------------\r\n"
		"name            description                   qty     cost\r\n"
		"-----------------------------------------------------------------\r\n"
		, ch);

	for (tGood = tp->first_tpgood; tGood; tGood = tGood->next)
	{
		if (!tGood->quantity)
			continue;

		if (!(pGood = get_good(tGood->goods_vnum)))
			continue;

		if (!(pGM = GoodsMarkets[pGood->vnum][tp->market->vnum]))
			continue;

		sprintf(gname, "%s %s of %s", AN(pGood->unit), pGood->unit, pGood->name);
		ch_printf(ch, "%s%-15s&0 %-25s     %3d     %4d [%4d]\r\n",
			tGood->stock ? "&b&7" : "",
			pGood->name,
			gname, tGood->quantity,
			pGM->price, pGood->cost);
	}

	send_to_char("-----------------------------------------------------------------\r\n", ch);
}

/* calculate trade value */
int calc_trade_value(int gnum, int mnum)
{
	GOOD_DATA *pGood;
	MARKET_DATA *pMk;
	MARKET_GOOD *pGM;
	int price, value = 0;
	float dem;

	// non-existant good exits here
	if (!(pGood = get_good(gnum)))		return (0);
	// unused good exits here
	if (!pGood->mkvnum)					return (0);
	// non-existant market exits here
	if (!(pMk = find_market(mnum)))		return (0);

	/*
	 * missing data for this good in this market..
	 * "simulate" good price in market calc'd with demand = 10;
	 */
	if (!(pGM = GoodsMarkets[pGood->vnum][pMk->vnum]))
	{
		MARKET_DATA *orig_mk = find_market(pGood->mkvnum);
		int iDist = (int)distance(orig_mk->heart.x, orig_mk->heart.y, pMk->heart.x, pMk->heart.y);

		price = pGood->cost * pMk->var_mod.price_var + pGood->gtype->elasticity * 1 * pGood->cost + pGood->cost * 1 * iDist / (COEF_DIV * 5 / 4) * pGood->gtype->elasticity;
		dem = 10;
	}
	else
	{
		price = pGM->price;
		dem = pGM->demand;
	}

	value = price - pGood->cost * pGood->gtype->elasticity * (dem / 10);

	// per evitare i prezzi negativi
	if (value < 0)
		value = pGood->cost / 2;

	return (value);
}


/*
 * buy goods from TP
 *
 * buy 'goods name'
 * buy 'goods name' <amount>
 */
void char_buy_goods(CHAR_DATA *ch, char *argument)
{
	GOOD_DATA *pGood = NULL;
	OBJ_DATA *obj = NULL;
	TRADING_POST *pTp = NULL;
	TRP_GOOD *tGood = NULL;
	MARKET_GOOD *pGM = NULL;
	char arg1[MAX_INPUT_LENGTH], sbaf[256], *g;
	int amount, price = 0;

	/* get: blank, spell name, target name */
	g = get_spell_name(argument);
	if (g == NULL)
	{
		send_to_char("Usage: buy 'good name' <amount>\r\n", ch);
		return;
	}
	argument = strtok(NULL, "\0");

	if (!(pGood = get_good_by_name(g)))
	{
		ch_printf(ch, "There is no good called '%s'.\r\n", g);
		return;
	}

	if (ch->in_building && ch->in_building->trp)
		pTp = ch->in_building->trp;

	if (!pTp)
	{
		send_to_char("You must be in a trading post in order to buy goods.\r\n", ch);
		return;
	}

	if (!(tGood = tp_has_goods(pTp, pGood->vnum)))
	{
		ch_printf(ch, "Here you cannot buy any good called '%s'.\r\n", g);
		return;
	}

	one_argument(argument, arg1);
	if (!*arg1 || !is_number(arg1))
		amount = 1;
	else
		amount = atoi(arg1);

	if (amount > tGood->quantity)
	{
		ch_printf(ch, "We don't have so many '%s'.\r\n", pGood->name);
		return;
	}

	if (!(pGM = GoodsMarkets[pGood->vnum][pTp->market->vnum]))
	{
		send_to_char("Sorry but you cannot buy that now.\r\n", ch);
		return;
	}

	price = pGM->price * amount;

	if (get_gold(ch) < price && !IS_GOD(ch))
	{
		send_to_char("You don't have enough money.\r\n", ch);
		return;	
	}

	obj = create_good_obj(pGood, amount);
	GET_OBJ_COST(obj) = pGM->price;

	if (!can_take_obj(ch, obj))
	{
		extract_obj(obj);
		return;
	}

	if (!IS_GOD(ch))
		sub_gold(ch, price);

	/* message to char */
	if (obj->count > 1)
		sprintf(sbaf, "You pay %d for %d $p.", price, obj->count);
	else
		sprintf(sbaf, "You pay %d for $p.", price);
	act(sbaf, FALSE, ch, obj, NULL, TO_CHAR);

	/* message to room */
	if (obj->count > 1)
		strcpy(sbaf, "$n buys some $o.");
	else
		strcpy(sbaf, "$n buys $p.");
	act(sbaf, TRUE, ch, obj, NULL, TO_ROOM);

	GET_OBJ_VAL(obj, 1)	= pTp->vnum;
	obj = obj_to_char(obj, ch);

	// decrease trading post's goods quantity
	tGood->quantity	-= amount;

	// decrease market quantity
	pGM->qty -= amount;

	SaveTradingPost();
}


/*
 * value goods in TP
 *
 * value 'goods name'
 * value 'goods name' <amount>
 */
void char_value_goods(CHAR_DATA *ch, char *argument)
{
	GOOD_DATA *goods = NULL;
	OBJ_DATA *obj = NULL;
	TRADING_POST *pTp = NULL;
	char arg1[MAX_INPUT_LENGTH], *g;
	int price, amount;

	/* get: blank, spell name, target name */
	g = get_spell_name(argument);
	if (g == NULL)
	{
		send_to_char("Usage: value 'goods name'\r\n", ch);
		return;
	}
	argument = strtok(NULL, "\0");

	if (!(goods = get_good_by_name(g)))
	{
		ch_printf(ch, "There is no goods called '%s'.\r\n", g);
		return;
	}

	if (ch->in_building && ch->in_building->trp)
		pTp = ch->in_building->trp;

	if (!pTp)
	{
		send_to_char("You must be in a trading post in order to value goods.\r\n", ch);
		return;
	}

	if (!pTp->market)
	{
		log("SYSERR: char_value_goods() - invalid market pointer in TP %d.\r\n", pTp->vnum);
		send_to_char("This trading post has some problems. Contact an Immortal please.\r\n", ch);
		return;
	}

	/* look if char has the goods object in his inventory */
	if (!(obj = get_good_object(ch->last_carrying, goods->vnum)))
	{
		send_to_char("You don't have it.\r\n", ch);
		return;
	}

	one_argument(argument, arg1);
	if (!*arg1 || !is_number(arg1))
		amount = obj->count;
	else
		amount = atoi(arg1);

	if (!(price = calc_trade_value(GET_OBJ_VAL(obj, 0), pTp->market->vnum)))
	{
		log("SYSERR: cannot calculate trade value for good %d.", GET_OBJ_VAL(obj, 0));
		send_to_char("Got a problem, contact an immortal.\r\n", ch);
		return;
	}

	if (amount > 1)
		ch_printf(ch, "You will receive %d gold coins for %d %s.", price * amount, amount, obj->short_description);
	else
		ch_printf(ch, "You will receive %d gold coins for %s.", price, obj->short_description);
}

/*
 * sell goods in TP
 *
 * sell 'goods name' <amount>
 */
void char_sell_goods(CHAR_DATA *ch, char *argument)
{
	GOOD_DATA *pGood = NULL;
	OBJ_DATA *obj = NULL;
	TRADING_POST *pTp = NULL;
	TRP_GOOD *tGood = NULL;
	MARKET_GOOD *pGM = NULL;
	char arg1[MAX_INPUT_LENGTH], lbuf[MAX_STRING_LENGTH], *g;
	int price, amount;

	/* get: blank, spell name, target name */
	g = get_spell_name(argument);
	if (g == NULL)
	{
		send_to_char("Usage: sell 'good name' <amount>\r\n", ch);
		return;
	}
	argument = strtok(NULL, "\0");

	if (!(pGood = get_good_by_name(g)))
	{
		ch_printf(ch, "There is no goods called '%s'.\r\n", g);
		return;
	}

	if (ch->in_building && ch->in_building->trp)
		pTp = ch->in_building->trp;

	if (!pTp)
	{
		send_to_char("You must be in a trading post in order to sell goods.\r\n", ch);
		return;
	}

	if (!pTp->market)
	{
		log("SYSERR: char_sell_goods() - invalid market pointer in TP %d.\r\n", pTp->vnum);
		send_to_char("This trading post has some problems. Contact an Immortal please.\r\n", ch);
		return;
	}

	/* look if char has the goods object in his inventory */
	if (!(obj = get_good_object(ch->last_carrying, pGood->vnum)))
	{
		send_to_char("You don't have it.\r\n", ch);
		return;
	}

	one_argument(argument, arg1);
	if (!*arg1 || !is_number(arg1))
		amount = obj->count;		// default to all
	else
		amount = atoi(arg1);

	if (obj->count > amount)
	{
		ch_printf(ch, "You don't have so many %s.\r\n", pGood->name);
		return;
	}

	if (!(price = calc_trade_value(GET_OBJ_VAL(obj, 0), pTp->market->vnum)))
	{
		log("SYSERR: cannot calculate trade value for good %d.", GET_OBJ_VAL(obj, 0));
		send_to_char("Got a problem, contact an immortal.\r\n", ch);
		return;
	}

	// add good to tp
	if (!(tGood = tp_has_goods(pTp, pGood->vnum)))
	{
		CREATE(tGood, TRP_GOOD, 1);
		tGood->next			= NULL;
		tGood->prev			= NULL;
		tGood->goods_vnum	= pGood->vnum;
		tGood->quantity		= 0;
		tGood->prev_qty		= 0;
		tGood->stock		= 0;

		LINK(tGood, pTp->first_tpgood, pTp->last_tpgood, next, prev);
	}
	tGood->quantity	+= amount;

	// good previously unused.. initialize.
	if (!pGood->mkvnum)
		pGood->mkvnum = pTp->market->vnum;

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

	calc_price(pTp->market, pGood, pGM);

	// message to char
	sprintf(lbuf, "You sell %d $o and receive %d gold coins.", amount, price * amount);
	act(lbuf, FALSE, ch, obj, NULL, TO_CHAR);
	// message to room
	sprintf(lbuf, "$n sells %d $o and receives %d gold coins.", amount, price * amount);
	act(lbuf, FALSE, ch, obj, NULL, TO_ROOM);
	
	create_amount(price * amount, ch, NULL, NULL, NULL, FALSE);
	extract_obj(obj);

	SaveGoodsMarketsTable(0);
	SaveTradingPost();
}

/* *************************************************************** */

SPECIAL(tradingpost)
{
	if (CMD_IS("list"))
	{
		list_goods_in_tp(ch);
		return (1);
	}

	if (CMD_IS("buy"))
	{
		if (!*argument)
			send_to_char("Usage: buy 'goods name' <amount>\r\n", ch);
		else
			char_buy_goods(ch, argument);
		return (1);
	}

	if (CMD_IS("sell"))
	{
		if (!*argument)
			send_to_char("Usage: sell 'goods name' <amount>\r\n", ch);
		else
			char_sell_goods(ch, argument);
		return (1);
	}

	if (CMD_IS("value"))
	{
		if (!*argument)
			send_to_char("Usage: value 'goods name'\r\n", ch);
		else
			char_value_goods(ch, argument);
		return (1);
	}

	return (0);
}

/* ------------------------------------------------------ */
/*
 * I know, I know, these two functions are duplicates of the
 * inner loops of consume_produce_goods() and calc_demands_and_prices()
 * functions above in the code.
 *
 * These below are used by ACMD(do_tp) in goods.c, and are unused in the
 * mentioned functions because the first time I modified the code
 * everything started to go bad (really really bad!).
 *
 * So, inner loops have been restored, and these two continue to be
 * used by ACMD(do_tp) and everyone is happy... silly reason, but... :-))
 *
 */
void calc_prod(MARKET_DATA *pMk, GOOD_DATA *pGood, TRP_GOOD *tGood, MARKET_GOOD *pGM)
{
	int cons = 0, qty = 0, gprod;
	int tmp1 = 0;
	double rand_cons = 0;
	double var1 = 0, var2 = 0, var3 = 0;
	
	// get base production
	gprod = pGood->gtype->prod_avg[Season()];

	// calc consumption
	cons = pGood->gtype->cons_speed * tGood->quantity;
				
	// se la GO e' a stock esegue la produzione interna
	if (tGood->stock)
	{
		gprod *= pMk->var_mod.prod_var;
		// calcola Random_Consumption
		var3 = (double) abs(cons - gprod);
		if (var3 < 0.3)
			var3 = 0.3;
		tmp1 = number(-1, 1);
		var2 = (double) (tmp1 * cons / var3);
		rand_cons = (double) (cons + var2);
		
		// calcola la nuova quantita' della GTP
		var3 = (double) (2 * (tGood->quantity - tGood->prev_qty) / 3);
		var2 = (double) (tGood->quantity + var3 + (gprod - rand_cons));
		//var1 = (double) (pMk->var_mod.prod_var * var2);
		qty = MAX(0, (int) var2);
	}
	// Produzione osmotica
	else
	{
		// inizializza MA relativo al mercato di origine
		MARKET_DATA *orig_mk = find_market(pGood->mkvnum);
		int iDist;
		
		// se non esiste il MA, passa alla GTP successiva
		if (!orig_mk)
			return;
		
		// calcola la distanza tra il MA di origine ed il MA in cui sono
		iDist = (int) distance(orig_mk->heart.x, orig_mk->heart.y, pMk->heart.x, pMk->heart.y);
		
		// calcola GM.Commercial_productivity
		gprod *= orig_mk->var_mod.prod_var;
		var3 = (double) gprod / COEF_DIV * iDist;
		var1 = (double) gprod - var3;
		var2 = (double) (var1 * pGM->comm_closure * pMk->var_mod.closure_var);
		pGM->comm_prod = (float) var2;
		
		// calcola Random_Consumption
		var3 = abs(cons - pGM->comm_prod);
		if (var3 < 0.3)
			var3 = 0.3;
		tmp1 = number(-1, 1);
		var2 = (double) (tmp1 * cons / var3);
		rand_cons = (double) (cons + var2);
		
		// calcola la nuova quantita' della GTP
		var3 = (double) (2 * (tGood->quantity - tGood->prev_qty) / 3);
		var2 = (double) (tGood->quantity + var3 + (pGM->comm_prod - rand_cons));
		//var1 = (double) (orig_mk->var_mod.prod_var * var2);
		qty = MAX(0, (int) var2);	
	}
	
	// aggiorna la quantita' del GM
	pGM->qty = MAX(0, pGM->qty - tGood->quantity);
	pGM->qty += qty;
	
	// aggiorna la quantita' della GTP
	tGood->prev_qty	= tGood->quantity;
	tGood->quantity = qty;
}

void calc_price(MARKET_DATA *pMk, GOOD_DATA *pGood, MARKET_GOOD *pGM)
{
	int avg_qty = 0, iDist = 0;
	
	// GO interno
	if (pGood->mkvnum == pMk->vnum)
	{
		// calcola Market_Good_Medium_quantity_internal
		avg_qty = (pGood->gtype->prod_avg[Season()] * pMk->var_mod.prod_var) / pGood->gtype->cons_speed * pGM->total_tp;
	}
	// GO osmotico
	else
	{
		// ottiene MA di origine (solo per il calcolo della distanza)
		MARKET_DATA *orig_mk = find_market(pGood->mkvnum);
		
		// calcola Market_Good_Medium_quantity_external
		avg_qty = pGM->comm_prod / pGood->gtype->cons_speed * pGM->total_tp;
		// calcola la distanza tra i MA
		iDist = (int)distance(orig_mk->heart.x, orig_mk->heart.y, pMk->heart.x, pMk->heart.y);
	}
	
	// calcola la domanda
	if ( avg_qty == 0 )
		pGM->demand = 10;
	else
	{
		double tmpqty, negqty;

		tmpqty = - pGM->qty;
		tmpqty /= avg_qty;
		negqty = exp(tmpqty);
		pGM->demand = (float) (10 * negqty);
	}
	
	// calcola il prezzo di vendita
	pGM->price = pGood->cost * pMk->var_mod.price_var + pGood->gtype->elasticity * (pGM->demand / 10) * pGood->cost + pGood->cost * pGM->good_appet * iDist / (COEF_DIV * 5 / 4) * pGood->gtype->elasticity;
}
