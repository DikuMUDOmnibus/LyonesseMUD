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
*   File: weather.c                                     Part of CircleMUD *
*  Usage: functions handling time and the weather                         *
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
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "clan.h"

#define MAGIC_PRECIP_START	1190
#define MAGIC_PRECIP_STOP	980

/* external globals */
extern TIME_INFO_DATA		time_info;
extern CLAN_POLITIC_DATA	politics_data;

/* external functions */
WILD_DATA *wild_sector(int y, int x);
SHIP_DATA *find_ship( sh_int vnum );
void	SimulateEconomy(void);
void	UpdateMarketAffections(void);
void	RollMarketAffections(void);
void	SaveMarkets(void);

/* locals */
void weather_and_time(int mode);
void another_hour(int mode);
void WeatherInit(void);
void change_weather(void);
bool LoadWeatherData(void);
void SaveWeatherData(void);
void WriteWeatherInfo(void);

/* Globals */
WEATHER_DATA	Weather[MAP_WEATH_Y][MAP_WEATH_X];

/* currently unused */
const char winddir[NUM_SEASONS]	=
{
  NORTH,
  SOUTHWEST,
  WEST,
  NORTHEAST
};

/* ============================================================== */

int Season(void)
{
	switch (time_info.month)
	{
	default:
		break;
	case  3:
	case  4:
	case  5:
	case  6:
		return (SPRING);
	case  7:
	case  8:
	case  9:
	case 10:
		return (SUMMER);
	case 11:
	case 12:
	case 13:
	case 14:
		return (AUTUMN);
	}

	return (WINTER);
}


void MoonMessage(void)
{
	char tbuf[MAX_STRING_LENGTH];

	switch (MoonPhase)
	{
	case MOON_NEW:
		strcpy(tbuf, "A black disc, devoid of stars, is the only moon on this night.\r\n");
		break;
	case MOON_WAXING_CRESCENT:
		strcpy(tbuf, "The short sliver of moon offers its jagged edge to the night sky.\r\n");
		break;
	case MOON_WAXING_HALF:
		strcpy(tbuf, "The waxing half moon rises for its night journey across the heavens.\r\n");
		break;
	case MOON_WAXING_THREE_QUARTERS:
		strcpy(tbuf, "The waxing three quarter moon rises.\r\n");
		break;
	case MOON_FULL:
		strcpy(tbuf, "The full moon rises, casting a silver glow over the night.\r\n");
		break;
	case MOON_WANING_THREE_QUARTERS:
		strcpy(tbuf, "The grey-silver waning three-quarter moon begins its nocturnal trek.\r\n");
		break;
	case MOON_WANING_HALF:
		strcpy(tbuf, "Amidst an oceanic starry night sky, the waning half moon floats,\r\nits silvery ship adrift on celestial currents.\r\n");
		break;
	case MOON_WANING_CRESCENT:
		strcpy(tbuf, "A curved waning crescent moon graces the night with a few rays of moonlight.\r\n");
		break;
	}

	send_to_outdoor(tbuf);
}

/* moon change day by day */
void ChangeMoon(void)
{
	if		(time_info.day	< 4)		MoonPhase = MOON_NEW;
	else if	(time_info.day	< 8)		MoonPhase = MOON_WAXING_CRESCENT;
	else if	(time_info.day	< 13)		MoonPhase = MOON_WAXING_HALF;
	else if	(time_info.day	< 18)		MoonPhase = MOON_WAXING_THREE_QUARTERS;
	else if	(time_info.day	< 23)		MoonPhase = MOON_FULL;
	else if	(time_info.day	< 27)		MoonPhase = MOON_WANING_THREE_QUARTERS;
	else if	(time_info.day	< 31)		MoonPhase = MOON_WANING_HALF;
	else								MoonPhase = MOON_WANING_CRESCENT;
}


void weather_and_time(int mode)
{
	another_hour(mode);
	if (mode)
	{
		change_weather();
		SaveWeatherData();
		WriteWeatherInfo();
	}
}


void another_hour(int mode)
{
	char tbuf[MAX_STRING_LENGTH];
	int rstr = number(1, 3);
	int oldmoon = MoonPhase;
	int x, y;
	static int econ_delay = 6;

	time_info.hours++;
	
	if (mode)
	{
		switch (time_info.hours)
		{
		case 4:
			/* moon new - no moon in the sky can disappear.. */
			if (Sunlight != MOON_RISE)
				break;

			Sunlight = MOON_SET;
			if (rstr == 1)
				strcpy(tbuf, "The moon sets.\r\n");
			else if (rstr == 2)
				strcpy(tbuf, "The moon disappears beneath the horizon.\r\n");
			else
				strcpy(tbuf, "The sky grows dark as the moon sets.\r\n");
			send_to_outdoor(tbuf);
			break;

		case 5:
			Sunlight = SUN_RISE;
			if (rstr == 1)
				strcpy(tbuf, "Dawn blushes the sky with hues of orange and magenta.\r\n");
			else if (rstr == 2)
				strcpy(tbuf, "The day has begun.\r\n");
			else
				strcpy(tbuf, "It is dawn.\r\n");
			send_to_outdoor(tbuf);
			break;

		case 6:
			Sunlight = SUN_LIGHT;
			if (rstr == 1)
				strcpy(tbuf, "The first shafts of sunlight streak along their western path.\r\n");
			else if (rstr == 2)
				strcpy(tbuf, "The sun rises in the east.\r\n");
			else
				strcpy(tbuf, "The sun illuminates the lands.\r\n");
			send_to_outdoor(tbuf);
			for (x = 1; x < MAX_CLANS; x++)
			{
				for (y = 1; y < MAX_CLANS; y++)
					politics_data.daily_negotiate_table[x][y] = FALSE;
			}
			break;

		case 12:
			if (rstr == 1)
				strcpy(tbuf, "It is noon.\r\n");
			else if (rstr == 2)
				strcpy(tbuf, "The sun reaches its apex in the sky.\r\n");
			else
				strcpy(tbuf, "Sunlight beats down upon you from directly above.\r\n");
			send_to_outdoor(tbuf);
			break;

		case 21:
			Sunlight = SUN_SET;
			if (rstr == 1)
				strcpy(tbuf, "The sun slowly disappears in the west.\r\n");
			else if ( rstr == 2 )
				strcpy(tbuf, "The sun dips below the western horizon.\r\n");
			else
				strcpy(tbuf, "The sun sets with a glorious twilight.\r\n");
			send_to_outdoor(tbuf);
			break;

		case 22:
			Sunlight = SUN_DARK;
			if (rstr == 1)
				strcpy(tbuf, "The night has begun.\r\n");
			else if (rstr == 2)
				strcpy(tbuf, "The last bit of twilight fades away to blackness.\r\n");
			else
				strcpy(tbuf, "The darkness of night engulfs the realm.\r\n");
			send_to_outdoor(tbuf);
			break;

		case 23:
			if (MoonPhase != MOON_NEW)
			{
				Sunlight = MOON_RISE;
				MoonMessage();
			}
			break;

		default:
			break;
		}
	}

	// modifiy economy data
	if (!(time_info.hours % econ_delay))
	{
		log("Performing economy simulation -- delay is %d", econ_delay);
		SimulateEconomy();
	}

	if (time_info.hours > 23)	/* Changed by HHS due to bug ??? */
	{
		time_info.hours = 0;
		time_info.day++;

		ChangeMoon();

		// every day update market affections
		UpdateMarketAffections();

		// setup new interval...
		econ_delay = 6 + number(0, 6);
		
		if (time_info.day > 34)
		{
			time_info.day = 0;
			time_info.month++;
			
			// every three months roll automatic market affections
			if (!(time_info.month % 3))
			{
				RollMarketAffections();
				SaveMarkets();
			}

			if (time_info.month > 16)
			{
				time_info.month = 0;
				time_info.year++;
			}
		}
	}
}

/* *************************************************************** */

void WeatherInit(void)
{
	sh_int s;
	int y, x;
	
	/* default conditions for season values */
	const char winds[NUM_SEASONS] 	= {25, 12, 8, 40};
	const char precip[NUM_SEASONS]	= {16, 30, 23, 10};
	const char humid[NUM_SEASONS] 	= {40, 75, 20, 14};
	const char temps[NUM_SEASONS] 	= {5, 17, 27, 13};

	/* Weather System */
	log("Initializing Weather System.");

	if (LoadWeatherData())
		return;

	/* get the season */
	s = Season();

	for (y = 0; y < MAP_WEATH_Y; y++)
	{
		for (x = 0; x < MAP_WEATH_X; x++)
		{
			if (s == WINTER)
				Weather[y][x].pressure	= 985;
			else if (s == SUMMER)
				Weather[y][x].pressure	= 1030;
			else
				Weather[y][x].pressure	= 1010;

			Weather[y][x].windspeed		= winds[s];
			Weather[y][x].wind_dir		= winddir[s];
			Weather[y][x].precip_rate	= precip[s];
			Weather[y][x].humidity		= humid[s];
			Weather[y][x].temp			= temps[s];
			Weather[y][x].free_energy	= 10000;
		}
	}

	for (y = 0; y < 500; y++)
		change_weather();

	SaveWeatherData();
	WriteWeatherInfo();
}


void change_weather(void)
{
	WEATHER_DATA *cond;
	WILD_DATA *wd;
	COORD_DATA csect;
	int y, x, magic;
	int old_wind, old_temp, old_precip, season_num;
	int pbreak;
	
	season_num = Season();

	for (y = 0; y < MAP_WEATH_Y; y++)
	{
		for (x = 0; x < MAP_WEATH_X; x++)
		{
			cond			= &Weather[y][x];

			old_temp		= cond->temp;
			old_precip		= cond->precip_rate;
			old_wind		= cond->windspeed;
		
			cond->free_energy	= URANGE(3000, cond->free_energy, 50000);

			switch (season_num)
			{
			case WINTER:
				// wind speed
				if		(cond->windspeed < 5)	cond->windspeed += 5;
				else if (cond->windspeed > 60)	cond->windspeed -= 5;
				else							cond->windspeed += number(-6, 6);

				// temperature
				if		(cond->temp < -10)		cond->temp += 1;
				else if (cond->temp > 15)		cond->temp -= 2;
				else							cond->temp += number(-3, 2);

				// humidity
				if		(cond->humidity < 30)	cond->humidity += 3;
				else if (cond->humidity > 70)	cond->humidity -= 2;
				else							cond->humidity += number(-9, 9);

				if (old_precip > 55)			cond->precip_rate -= 10;

				pbreak = 985;
				break;

			case SPRING:
				// wind speed
				if (cond->windspeed > 40)		cond->windspeed -= 5;
				else							cond->windspeed += number(-2, 2);

				// temperature
				if		(cond->temp < 7)		cond->temp += 2;
				else if (cond->temp > 26)		cond->temp -= 2;
				else							cond->temp += number(-2, 2);

				// humidity
				if		(cond->humidity < 40)	cond->humidity += 3;
				else if (cond->humidity > 80)	cond->humidity -= 2;
				else							cond->humidity += number(-8, 8);

				if (old_precip > 65)			cond->precip_rate -= 10;

				pbreak = 1000;
				break;

			case SUMMER:
				// wind speed
				if (cond->windspeed > 25)		cond->windspeed -= 5;
				else							cond->windspeed += number(-2, 1);

				// temperature
				if		(cond->temp < 24)		cond->temp += 3;
				else if (cond->temp > 46)		cond->temp -= 2;
				else							cond->temp += number(-2, 5);

				// humidity
				if		(cond->humidity < 13)	cond->humidity += 3;
				else if (cond->humidity > 91)	cond->humidity -= 2;
				else							cond->humidity += number(-5, 4);

				if (old_precip > 45)			cond->precip_rate -= 10;

				pbreak = 1015;
				break;

			case AUTUMN:
				// wind speed
				if		(cond->windspeed < 15)	cond->windspeed += 5;
				else if (cond->windspeed > 80)	cond->windspeed -= 5;
				else							cond->windspeed += number(-6, 6);
				
				// temperature
				if		(cond->temp < -3)		cond->temp += 2;
				else if (cond->temp > 14)		cond->temp -= 2;
				else							cond->temp += number(-3, 3);

				// humidity
				if (cond->humidity > 40)		cond->humidity -= 3;
				else							cond->humidity += number(-3, 2);

				if (old_precip > 20)			cond->precip_rate -= 8;

				pbreak = 1000;
				break;

			default:
				break;
			}

			cond->windspeed			= URANGE(0, cond->windspeed, 200);
			cond->wind_dir			= winddir[season_num];

			cond->free_energy		+= cond->windspeed * 10;

			if (cond->free_energy > 20000)
				cond->windspeed		+= number(-10, -1);
			cond->windspeed			= URANGE(0, cond->windspeed, 200);

			cond->humidity			= URANGE(0, cond->humidity, 100);
			
			cond->pressure_change	+= number(-3, 3);
			cond->pressure_change	= URANGE(-8, cond->pressure_change, 8);
			cond->pressure			+= cond->pressure_change;
			cond->pressure			= URANGE(960, cond->pressure, 1040);
			
			cond->free_energy		+= cond->pressure_change;

			/* The numbers that follow are truly magic since  */
			/* they have little bearing on reality and are an */
			/* attempt to create a mix of precipitation which */
			/* will seem reasonable for a specified climate   */
			/* without any complicated formulae that could    */
			/* cause a performance hit. To get more specific  */
			/* or exacting would certainly not be "Diku..."   */
			
			magic = ((1240 - cond->pressure) * cond->humidity >> 4) +
				cond->temp + old_precip * 2 + (cond->free_energy - 10000) / 100;

			// find out which wild sector is
			csect.y = y * MAP_SIZE;
			csect.x = x * MAP_SIZE;
			wd = wild_sector(csect.y, csect.x);
			
			if (old_precip == 0)
			{
				if (magic > MAGIC_PRECIP_START)
				{
					cond->precip_rate += number(1, 10);
					if (cond->temp > 0)
						send_to_wildsect(wd, "It begins to rain.\r\n");
					else
						send_to_wildsect(wd, "It starts to snow.\r\n");
				}
				else if (!old_wind && cond->windspeed)
					send_to_wildsect(wd, "The wind begins to blow\r\n");
				else if (cond->windspeed - old_wind > 10)
					send_to_wildsect(wd, "The wind picks up some.\r\n");
				else if (cond->windspeed - old_wind < -10)
					send_to_wildsect(wd, "The wind calms down a bit.\r\n");
				else if (cond->windspeed > 60)
				{
					if (cond->temp > 50)
						send_to_wildsect(wd, "A violent scorching wind blows hard in the face of any poor travellers in the area.\r\n");
					else if (cond->temp > 21)
						send_to_wildsect(wd, "A hot wind gusts wildly through the area.\r\n");
					else if (cond->temp > 0)
						send_to_wildsect(wd, "A fierce wind cuts the air like a razor-sharp knife.\r\n");
					else if (cond->temp > -10)
						send_to_wildsect(wd, "A freezing gale blasts through the area.\r\n");
					else
						send_to_wildsect(wd, "An icy wind drains the warmth from all in sight.\r\n");
				}
				else if (cond->windspeed > 25)
				{
					if (cond->temp > 50)
						send_to_wildsect(wd, "A hot, dry breeze blows languidly around.\r\n");
					else if (cond->temp > 22)
						send_to_wildsect(wd, "A warm pocket of air is rolling through here.\r\n");
					else if (cond->temp > 10)
						send_to_wildsect(wd, "It's breezy.\r\n");
					else if (cond->temp > 2)
						send_to_wildsect(wd, "A cool breeze wafts by.\r\n");
					else if (cond->temp > -5)
						send_to_wildsect(wd, "A slight wind blows a chill into living tissue.\r\n");
					else if (cond->temp > -15)
						send_to_wildsect(wd, "A freezing wind blows gently, but firmly against all obstacles in the area.\r\n");
					else
						send_to_wildsect(wd, "The wind isn't very strong here, but the cold makes it quite noticeable.\r\n");
				}
				else if (cond->temp > 52)
					send_to_wildsect(wd, "It's hotter than anyone could imagine.\r\n");
				else if (cond->temp > 37)
					send_to_wildsect(wd, "It's really, really hot here. A slight breeze would really improve things.\r\n");
				else if (cond->temp > 25)
					send_to_wildsect(wd, "It's hot out here.\r\n");
				else if (cond->temp > 19)
					send_to_wildsect(wd, "It's nice and warm out.\r\n");
				else if (cond->temp > 9)
					send_to_wildsect(wd, "It's mild out today.\r\n");
				else if (cond->temp > 1)
					send_to_wildsect(wd, "It's cool out here.\r\n");
				else if (cond->temp > -5)
					send_to_wildsect(wd, "It's a bit nippy here.\r\n");
				else if (cond->temp > -20)
					send_to_wildsect(wd, "It's cold!\r\n");
				else if (cond->temp > -25)
					send_to_wildsect(wd, "It's really c-c-c-cold!!\r\n");
				else
					send_to_wildsect(wd, "Better get inside - this is too cold for man or -most- beasts.\r\n");
			}
			else if (magic < MAGIC_PRECIP_STOP || (old_precip == 1 && cond->pressure > pbreak))
			{
				cond->precip_rate = 0;
				if (old_temp > 0)
					send_to_wildsect(wd, "The rain stops.\r\n");
				else
					send_to_wildsect(wd, "It stops snowing.\r\n");
			}
			else
			{
				/* Still precip'ing, update the rate */
				sh_int pchange = 0;

				// slow rate
				if (cond->pressure >= pbreak)
				{
					if (cond->windspeed > 50)
						pchange += number(-3, 1);
					else if (cond->windspeed > 20)
						pchange += number(-2, 2);
					else
						pchange += number(-1, 2);

					// check humidity
					if (cond->humidity < 30)
						pchange += number(-4, 2);
					else if (cond->humidity < 60)
						pchange += number(-3, 2);
					else
						pchange += number(-2, 2);
				}
				// increase rate
				else
				{
					if (cond->windspeed > 50)
						pchange += number(-2, 1);
					else if (cond->windspeed > 20)
						pchange += number(-1, 1);
					else
						pchange += number(0, 2);

					// check humidity
					if (cond->humidity < 30)
						pchange += number(-2, 2);
					else if (cond->humidity < 60)
						pchange += number(-1, 2);
					else
						pchange += number(-1, 3);
				}

				/* energy adjustment */
				if (cond->free_energy > 10000)
					pchange += number(-3, 4);
				else
					pchange += number(-4, 2);

				cond->precip_change	+= pchange;
				cond->precip_change	= URANGE(-10, cond->precip_change, 10);

				cond->precip_rate	+= cond->precip_change;
				cond->precip_rate	= URANGE(1, cond->precip_rate, 100);

				cond->free_energy	-= cond->precip_rate * 3 - abs(cond->precip_change);
				
				/* Check rain->snow or snow->rain */
				if (old_temp > 0 && cond->temp <= 0)
					send_to_wildsect(wd, "The rain turns to snow.\r\n");
				else if (old_temp <= 0 && cond->temp > 0)
					send_to_wildsect(wd, "The snow turns to a cold rain.\r\n");
				else if (cond->precip_change > 5)
				{
					if (cond->temp > 0)
						send_to_wildsect(wd, "It rains a bit harder.\r\n");
					else
						send_to_wildsect(wd, "The snow is coming down faster now.\r\n");
				}
				else if (cond->precip_change < -5)
				{
					if (cond->temp > 0)
						send_to_wildsect(wd, "The rain is falling less heavily now.\r\n");
					else
						send_to_wildsect(wd, "The snow has let up a little.\r\n");
				}
				else if (cond->temp > 0)
				{
					if (cond->precip_rate > 80)
					{
						if (cond->windspeed > 80)
							send_to_wildsect(wd, "There's a hurricane out here!\r\n");
						else if (cond->windspeed > 40)
							send_to_wildsect(wd, "The wind and the rain are nearly too much to handle.\r\n");
						else
							send_to_wildsect(wd, "It's raining really hard right now.\r\n");
					}
					else if (cond->precip_rate > 50)
					{
						if (cond->windspeed > 60)
							send_to_wildsect(wd, "What a rainstorm!\r\n");
						else if (cond->windspeed > 30)
							send_to_wildsect(wd, "The wind is lashing this wild rain seemingly straight into your face.\r\n");
						else
							send_to_wildsect(wd, "It's raining pretty hard.\r\n");
					}
					else if (cond->precip_rate > 30)
					{
						if (cond->windspeed > 50)
							send_to_wildsect(wd, "A respectable rain is being thrashed about by a vicious wind.\r\n");
						else if (cond->windspeed > 25)
							send_to_wildsect(wd, "It's rainy and windy but, altogether not too uncomfortable.\r\n");
						else
							send_to_wildsect(wd, "Hey, it's raining...\r\n");
					}
					else if (cond->precip_rate > 10)
					{
						if (cond->windspeed > 50)
							send_to_wildsect(wd, "The light rain here is nearly unnoticeable compared to the horrendous wind.\r\n");
						else if (cond->windspeed > 24)
							send_to_wildsect(wd, "A light rain is being driven fiercely by the wind.\r\n");
						else
							send_to_wildsect(wd, "It's raining lightly.\r\n");
					}
					else if (cond->windspeed > 55)
					
						send_to_wildsect(wd, "A few drops of rain are falling admidst a fierce windstorm.\r\n");
					else if (cond->windspeed > 30)
						send_to_wildsect(wd, "The wind and a bit of rain hint at the possibility of a storm.\r\n");
					else
						send_to_wildsect(wd, "A light drizzle is falling here.\r\n");
				}
				else if (cond->precip_rate > 70)
				{
					if (cond->windspeed > 50)
						send_to_wildsect(wd, "This must be the worst blizzard ever.\r\n");
					else if (cond->windspeed > 25)
						send_to_wildsect(wd, "There's a blizzard out here, making it quite difficult to see.\r\n");
					else
						send_to_wildsect(wd, "It's snowing very hard.\r\n");
				}
				else if (cond->precip_rate > 40)
				{
					if (cond->windspeed > 60)
						send_to_wildsect(wd, "The heavily falling snow is being whipped up to a frenzy by a ferocious wind.\r\n");
					else if (cond->windspeed > 35)
						send_to_wildsect(wd, "A heavy snow is being blown randomly about by a brisk wind.\r\n");
					else if (cond->windspeed > 18)
						send_to_wildsect(wd, "Drifts in the snow are being formed by the wind.\r\n");
					else
						send_to_wildsect(wd, "The snow's coming down pretty fast now.\r\n");
				}
				else if (cond->precip_rate > 19)
				{
					if (cond->windspeed > 70)
						send_to_wildsect(wd, "The snow wouldn't be too bad, except for the awful wind blowing it in every possible directon.\r\n");
					else if (cond->windspeed > 45)
						send_to_wildsect(wd, "There's a minor blizzard here, more wind than snow.\r\n");
					else if (cond->windspeed > 12)
						send_to_wildsect(wd, "Snow is being blown about by a stiff breeze.\r\n");
					else
						send_to_wildsect(wd, "It is snowing here.\r\n");
				}
				else if (cond->windspeed > 60)
					send_to_wildsect(wd, "A light snow is being tossed about by a fierce wind.\r\n");
				else if (cond->windspeed > 42)
					send_to_wildsect(wd, "A lightly falling snow is being driven by a strong wind.\r\n");
				else if (cond->windspeed > 18)
					send_to_wildsect(wd, "A light snow is falling admidst an unsettled wind.\r\n");
				else
					send_to_wildsect(wd, "It is lightly snowing.\r\n");
			}
		}
	}
}


/* ************************************************************* */

bool LoadWeatherData(void)
{
	FILE *fp;
	char fname[256];
	int y;

	sprintf(fname, "%sweathdata.wls", WLS_PREFIX);
	if ( !( fp = fopen(fname, "rb") ) )
	{
		log("SYSERR: unable to read weather data file.");
		return (FALSE);
	}

	for ( y = 0; y < MAP_WEATH_Y; y++ )
		fread(Weather[y], sizeof(WEATHER_DATA), MAP_WEATH_X, fp);

	fclose(fp);
	return (TRUE);
}

void SaveWeatherData(void)
{
	FILE *fp;
	char fname[256];
	int y;

	sprintf(fname, "%sweathdata.wls", WLS_PREFIX);
	if ( !( fp = fopen(fname, "wb") ) )
	{
		log("SYSERR: unable to write weather data file.");
		return;
	}

	for ( y = 0; y < MAP_WEATH_Y; y++ )
		fwrite(Weather[y], sizeof(WEATHER_DATA), MAP_WEATH_X, fp);

	fclose(fp);
}


void WriteWeatherInfo(void)
{
	FILE *fp;
	WEATHER_DATA *cond;
	char fname[256];
	int y, x;
	
	sprintf(fname, "%sweather.wls", WLS_PREFIX);
	if ( !( fp = fopen(fname, "w") ) )
	{
		log("SYSERR: unable to write new weather file.");
		return;
	}
	
	fprintf(fp, "SEASON: %d\n", Season());
	for (y = 0; y < MAP_WEATH_Y; y++)
	{
		for (x = 0; x < MAP_WEATH_X; x++)
		{
			cond = &Weather[y][x];
			
			fprintf(fp, "[%3d %3d] - ", y * MAP_SIZE, x * MAP_SIZE);
			fprintf(fp, "Temp: %2d  Humidity: %2d  Pressure: %4d  ",
				cond->temp, cond->humidity, cond->pressure);
			fprintf(fp, "Windspeed: %3d  Precip Rate: %3d  ",
				cond->windspeed, cond->precip_rate);
			fprintf(fp, "Pressure change: %2d  Precip change: %3d  FEnergy: %d",
				cond->pressure_change, cond->precip_change, cond->free_energy);
			fprintf(fp, "\n");
		}
	}
	fclose(fp);
}

/* ************************************************************* */

COORD_DATA get_wwmap_indexes(COORD_DATA *coord)
{
	COORD_DATA csect;

	/* normalize to wild sector coordinates */
	csect.y = coord->y / MAP_SIZE * MAP_SIZE;
	csect.x = coord->x / MAP_SIZE * MAP_SIZE;

	/* calculates weather map indexes */
	csect.y = URANGE(0, (csect.y / MAP_SIZE), MAP_WEATH_Y - 1);
	csect.x = URANGE(0, (csect.x / MAP_SIZE), MAP_WEATH_X - 1);

	return (csect);
}

/* given a room, returns the new weather in the room's wild sector */
WEATHER_DATA *get_room_weather(ROOM_DATA *pRoom)
{
	COORD_DATA csect;

	if (!pRoom)
		return (NULL);

	if (IS_WILD(pRoom))
		csect = *pRoom->coord;
	else if (IS_SHIP(pRoom))
	{
		SHIP_DATA *ship = find_ship(pRoom->extra_data->vnum);
		csect = *ship->in_room->coord;

	}
	else if (IS_FERRY(pRoom))
	{
		FERRY_DATA *pFerry = get_ferry(pRoom->extra_data->vnum);
		csect = *pFerry->in_room->coord;
	}
	else if (pRoom->zone == NOWHERE)
	{
		return (NULL);
	}
	else
	{
		csect = zone_table[pRoom->zone].wild.z_start;

		// check if zone has a place in the wilderness
		if (csect.y == 0 && csect.x == 0)
			return (NULL);
	}

	csect = get_wwmap_indexes(&csect);

	return (&Weather[csect.y][csect.x]);
}


WEATHER_DATA *get_coord_weather(COORD_DATA *coord)
{
	COORD_DATA csect = get_wwmap_indexes(coord);
	return (&Weather[csect.y][csect.x]);
}



