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
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __ACT_OTHER_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"

/* extern variables */
extern SPELL_INFO_DATA	spell_info[];
extern const char	*class_abbrevs[];
extern const char	*pc_class_types[];
extern const char	*pc_race_types[];
extern int			free_rent;
extern int			pt_allowed;
extern int			max_filesize;
extern int			nameserver_is_slow;
extern int			auto_save;
extern int			track_through_doors;

/* extern procedures */
ACMD(do_gen_comm);
SPECIAL(shop_keeper);
bool	water_sector(int sect);
bool	forget_spell(CHAR_DATA *ch, char *argument);
int		find_first_step(ROOM_DATA *src, ROOM_DATA *target);
int		wild_track(CHAR_DATA *ch, CHAR_DATA *vict, bool silent);
int		has_boat(CHAR_DATA *ch);
void	list_skills(CHAR_DATA *ch, bool known);
void	appear(CHAR_DATA *ch);
void	write_aliases(CHAR_DATA *ch);
void	perform_immort_vis(CHAR_DATA *ch);
void	die(CHAR_DATA *ch, CHAR_DATA *killer);
void	add_follower(CHAR_DATA *ch, CHAR_DATA *leader);
void	dismount_char(CHAR_DATA *ch);
void	mount_char(CHAR_DATA *ch, CHAR_DATA *mount);
void	list_skills_vict(CHAR_DATA *ch, CHAR_DATA *viewer);;
void	CharLoseAllItems(CHAR_DATA *ch);


/* ================================================================== */

ACMD(do_quit)
{
	if (IS_NPC(ch) || !ch->desc)
		return;
	
	if (subcmd != SCMD_QUIT && GET_LEVEL(ch) < LVL_IMMORT)
		send_to_char("You have to type quit--no less, to quit!\r\n", ch);
	else if (GET_POS(ch) == POS_FIGHTING)
		send_to_char("No way!  You're fighting for your life!\r\n", ch);
	else if (GET_POS(ch) < POS_STUNNED)
	{
		send_to_char("You die before your time...\r\n", ch);
		die(ch, NULL);
	}
	else
	{
		ROOM_DATA *loadroom = ch->in_room;
		
		act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
		sprintf(buf, "%s has quit the game.", GET_NAME(ch));
		mudlog(buf, NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE);
		send_to_char("Goodbye, friend.. Come back soon!\r\n", ch);
		
		/* Ghost player that quit lose all items */
		if (PLR_FLAGGED(ch, PLR_GHOST))
			CharLoseAllItems(ch);

		if (get_room_aff_bitv(loadroom, RAFF_CAMP))
			SET_BIT(PLR_FLAGS(ch), PLR_CAMPED);

		SaveCharObj(ch, TRUE);
		
		/* If someone is quitting in their house, let them load back there */
		if ( ROOM_FLAGGED(loadroom, ROOM_HOUSE) || ROOM_FLAGGED(loadroom, ROOM_SAVEROOM))
			save_char(ch, loadroom);
		else if (PLR_FLAGGED(ch, PLR_CAMPED))
			save_char(ch, loadroom);
		else
			save_char(ch, NULL);

		extract_char(ch);
	}
}



ACMD(do_save)
{
	if (IS_NPC(ch) || !ch->desc)
		return;
	
	if (PLR_FLAGGED(ch, PLR_GHOST))
	{
		send_to_char("Ghosts don't save.. return to the real world first!\r\n", ch);
		return;
	}

	/* Only tell the char we're saving if they actually typed "save" */
	if (cmd)
	{
		/*
		 * This prevents item duplication by two PC's using coordinated saves
		 * (or one PC with a house) and system crashes. Note that houses are
		 * still automatically saved without this enabled. This code assumes
		 * that guest immortals aren't trustworthy. If you've disabled guest
		 * immortal advances from mortality, you may want < instead of <=.
		 */
		if (auto_save && GET_LEVEL(ch) <= LVL_IMMORT)
		{
			send_to_char("Saving aliases.\r\n", ch);
			write_aliases(ch);
			return;
		}
		sprintf(buf, "Saving %s and aliases.\r\n", GET_NAME(ch));
		send_to_char(buf, ch);
	}
	
	write_aliases(ch);
	save_char(ch, NULL);
	SaveCharObj(ch, FALSE);

	if (ROOM_FLAGGED(ch->in_room, ROOM_HOUSE_CRASH))
		House_crashsave(ch->in_room->number);
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
	send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}



ACMD(do_sneak)
{
	AFFECTED_DATA af;
	
	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_SNEAK))
	{
		send_to_char("You have no idea how to do that.\r\n", ch);
		return;
	}
	send_to_char("Okay, you'll try to move silently for a while.\r\n", ch);
	if (AFF_FLAGGED(ch, AFF_SNEAK))
		affect_from_char(ch, SKILL_SNEAK);
	
	if ( !success(ch, NULL, SKILL_SNEAK, 0) )
		return;
	
	af.type			= SKILL_SNEAK;
	af.duration		= GET_LEVEL(ch);
	af.modifier		= 0;
	af.location		= APPLY_NONE;
	af.bitvector	= AFF_SNEAK;
	affect_to_char(ch, &af);
}



ACMD(do_hide)
{
	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_HIDE))
	{
		send_to_char("You have no idea how to do that.\r\n", ch);
		return;
	}
	
	send_to_char("You attempt to hide yourself.\r\n", ch);
	
	if (AFF_FLAGGED(ch, AFF_HIDE))
		REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

	if ( !success(ch, NULL, SKILL_HIDE, 0) )
		return;
	
	SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
}




ACMD(do_steal)
{
	CHAR_DATA *vict;
	OBJ_DATA *obj;
	char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
	int percent, eq_pos, pcsteal = 0, ohoh = 0;
	
	if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_STEAL))
	{
		send_to_char("You have no idea how to do that.\r\n", ch);
		return;
	}
	if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
	{
		send_to_char("This room just has such a peaceful, easy feeling...\r\n", ch);
		return;
	}
	
	two_arguments(argument, obj_name, vict_name);
	
	if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM)))
	{
		send_to_char("Steal what from who?\r\n", ch);
		return;
	}
	else if (vict == ch)
	{
		send_to_char("Come on now, that's rather stupid!\r\n", ch);
		return;
	}
	
	/* 101% is a complete failure */
	percent = number(1, 101) - dex_app_skill[GET_DEX(ch)].p_pocket;
	
	if (GET_POS(vict) < POS_SLEEPING)
		percent = -1;		/* ALWAYS SUCCESS, unless heavy object. */
	
	if (!pt_allowed && !IS_NPC(vict))
		pcsteal = 1;
	
	if (!AWAKE(vict))	/* Easier to steal from sleeping people. */
		percent -= 50;
	
	/* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
	if (GET_LEVEL(vict) >= LVL_IMMORT || pcsteal || GET_MOB_SPEC(vict) == shop_keeper)
		percent = 101;		/* Failure */
	
	if (!(obj = get_obj_in_list_vis_rev(ch, obj_name, NULL, vict->last_carrying)))
	{
		for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
		{
			if (GET_EQ(vict, eq_pos) &&
				(isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
				CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos)))
			{
				obj = GET_EQ(vict, eq_pos);
				break;
			}
		}
		
		if (!obj)
		{
			act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
			return;
		}
		else			/* It is equipment */
		{
			if ((GET_POS(vict) > POS_STUNNED))
			{
				send_to_char("Steal the equipment now?  Impossible!\r\n", ch);
				return;
			}
			else
			{
				act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
				act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
				obj = obj_to_char(unequip_char(vict, eq_pos), ch);
			}
		}
	}
	else			/* obj found in inventory */
	{
		
		percent += GET_OBJ_WEIGHT(obj);	/* Make heavy harder */
		
		if (percent > GET_SKILL(ch, SKILL_STEAL))
		{
			ohoh = TRUE;
			send_to_char("Oops..\r\n", ch);
			act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
			act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
		}
		else			/* Steal the item */
		{
			if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))
			{
				if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch))
				{
					separate_obj(obj);
					obj_from_char(obj);
					obj = obj_to_char(obj, ch);
					send_to_char("Got it!\r\n", ch);
				}
			}
			else
				send_to_char("You cannot carry that much.\r\n", ch);
		}
	}
	
	if (ohoh && IS_NPC(vict) && AWAKE(vict))
		hit(vict, ch, TYPE_UNDEFINED);
}


ACMD(do_practice)
{
	if (IS_NPC(ch))
		return;
	
	one_argument(argument, arg);
	
	if (*arg)
	{
		if (IS_IMMORTAL(ch))
		{
			CHAR_DATA *vict;

			if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
				send_to_char(NOPERSON, ch);
			else
				list_skills_vict(vict, ch);
			return;
		}
		send_to_char("You can only practice skills in your guild.\r\n", ch);
	}
	else
		list_skills(ch, TRUE);
}



ACMD(do_visible)
{
	if (GET_LEVEL(ch) >= LVL_IMMORT)
	{
		perform_immort_vis(ch);
		return;
	}
	
	if AFF_FLAGGED(ch, AFF_INVISIBLE)
	{
		appear(ch);
		send_to_char("You break the spell of invisibility.\r\n", ch);
	}
	else
		send_to_char("You are already visible.\r\n", ch);
}



ACMD(do_title)
{
	skip_spaces(&argument);
	delete_doubledollar(argument);
	
	if (IS_NPC(ch))
		send_to_char("Your title is fine... go away.\r\n", ch);
	else if (PLR_FLAGGED(ch, PLR_NOTITLE))
		send_to_char("You can't title yourself -- you shouldn't have abused it!\r\n", ch);
	else if (strstr(argument, "(") || strstr(argument, ")"))
		send_to_char("Titles can't contain the ( or ) characters.\r\n", ch);
	else if (strlen(argument) > MAX_TITLE_LENGTH)
	{
		sprintf(buf, "Sorry, titles can't be longer than %d characters.\r\n",
			MAX_TITLE_LENGTH);
		send_to_char(buf, ch);
	}
	else
	{
		set_title(ch, argument);
		sprintf(buf, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
		send_to_char(buf, ch);
	}
}


ACMD(do_knock)
{
	EXIT_DATA *pexit;
	char arg[MAX_INPUT_LENGTH], arg1[MAX_INPUT_LENGTH];
	
	two_arguments(argument, arg, arg1);
	
	if (!*arg)
	{
		send_to_char("Knock on what?\r\n",ch);
		return;
	}
	
	if ((pexit = find_door(ch, arg, arg1, "knock")))
	{
		sprintf(buf, "$n knocks on the %s.", *pexit->keyword ? pexit->keyword : "door");
		act(buf, FALSE, ch, NULL, NULL, TO_ROOM);
		ch_printf(ch, "You knock on the %s.", *pexit->keyword ? pexit->keyword : "door" );
		
		/* Notify the other side.  */
		if ( pexit->to_room && pexit->rexit && pexit->rexit->to_room == ch->in_room )
			send_to_room("You hear someone knocking.", pexit->to_room );
	}
}


/* ************************************************************* */
/* Group Code                                                    */
/* ************************************************************* */

int perform_group(CHAR_DATA *ch, CHAR_DATA *vict)
{
	if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
		return (0);
	
	SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
	
	if (ch != vict)
		act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);

	act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
	act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
	
	return (1);
}


void print_group(CHAR_DATA *ch)
{
	CHAR_DATA *k;
	FOLLOW_DATA *f;
	
	if (!AFF_FLAGGED(ch, AFF_GROUP))
		send_to_char("But you are not the member of a group!\r\n", ch);
	else
	{
		send_to_char("Your group consists of:\r\n", ch);
		
		k = (ch->master ? ch->master : ch);
		
		if (AFF_FLAGGED(k, AFF_GROUP))
		{
			sprintf(buf, "     [%3dH %3dM %3dV] [%2d %s] $N (Head of group)",
				GET_HIT(k), GET_MANA(k), GET_MOVE(k), GET_LEVEL(k), CLASS_ABBR(k));
			act(buf, FALSE, ch, 0, k, TO_CHAR);
		}
		
		for (f = k->followers; f; f = f->next)
		{
			if (!AFF_FLAGGED(f->follower, AFF_GROUP))
				continue;
			
			sprintf(buf, "     [%3dH %3dM %3dV] [%2d %s] $N", GET_HIT(f->follower),
				GET_MANA(f->follower), GET_MOVE(f->follower),
				GET_LEVEL(f->follower), CLASS_ABBR(f->follower));
			act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
		}
	}
}


ACMD(do_follow)
{
	CHAR_DATA *leader;
	
	one_argument(argument, buf);
	
	if (*buf)
	{
		if (!(leader = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
		{
			send_to_char(NOPERSON, ch);
			return;
		}
	}
	else
	{
		send_to_char("Whom do you wish to follow?\r\n", ch);
		return;
	}
	
	/* introduction code */
	if ( !is_char_known(ch, leader ) )
	{
		send_to_char("You cannot follow someone you don't know.\r\n", ch );
		return;
	}
	
	if (ch->master == leader)
	{
		act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
		return;
	}

	if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master))
	{
		act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
	}
	else
	{			/* Not Charmed follow person */
		if (leader == ch)
		{
			if (!ch->master)
			{
				send_to_char("You are already following yourself.\r\n", ch);
				return;
			}
			stop_follower(ch);
		}
		else
		{
			if (circle_follow(ch, leader))
			{
				send_to_char("Sorry, but following in loops is not allowed.\r\n", ch);
				return;
			}

			if (ch->master)
				stop_follower(ch);

			REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
			add_follower(ch, leader);
		}
	}
}


ACMD(do_group)
{
	CHAR_DATA *vict;
	FOLLOW_DATA *f;
	int found;
	
	one_argument(argument, buf);
	
	if (!*buf)
	{
		print_group(ch);
		return;
	}
	
	if (ch->master)
	{
		act("You can not enroll group members without being head of a group.",
			FALSE, ch, 0, 0, TO_CHAR);
		return;
	}
	
	if (!str_cmp(buf, "all"))
	{
		perform_group(ch, ch);
		for (found = 0, f = ch->followers; f; f = f->next)
			found += perform_group(ch, f->follower);
		if (!found)
			send_to_char("Everyone following you is already in your group.\r\n", ch);
		return;
	}
	
	if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
		send_to_char(NOPERSON, ch);
	else if ((vict->master != ch) && (vict != ch))
		act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
	else
	{
		if (!AFF_FLAGGED(vict, AFF_GROUP))
			perform_group(ch, vict);
		else
		{
			if (ch != vict)
				act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);

			act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
			act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);

			REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
		}
	}
}



ACMD(do_ungroup)
{
	FOLLOW_DATA *f, *next_fol;
	CHAR_DATA *tch;
	
	one_argument(argument, buf);
	
	if (!*buf)
	{
		if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP)))
		{
			send_to_char("But you lead no group!\r\n", ch);
			return;
		}
		sprintf(buf2, "%s has disbanded the group.\r\n", GET_NAME(ch));
		for (f = ch->followers; f; f = next_fol)
		{
			next_fol = f->next;
			if (AFF_FLAGGED(f->follower, AFF_GROUP))
			{
				REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
				send_to_char(buf2, f->follower);
				if (!AFF_FLAGGED(f->follower, AFF_CHARM))
					stop_follower(f->follower);
			}
		}
		
		REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
		send_to_char("You disband the group.\r\n", ch);
		return;
	}
	if (!(tch = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
	{
		send_to_char("There is no such person!\r\n", ch);
		return;
	}
	if (tch->master != ch)
	{
		send_to_char("That person is not following you!\r\n", ch);
		return;
	}
	
	if (!AFF_FLAGGED(tch, AFF_GROUP))
	{
		send_to_char("That person isn't in your group.\r\n", ch);
		return;
	}
	
	REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);
	
	act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
	act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
	act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);
	
	if (!AFF_FLAGGED(tch, AFF_CHARM))
		stop_follower(tch);
}


ACMD(do_report)
{
	CHAR_DATA *k;
	FOLLOW_DATA *f;
	
	if (!AFF_FLAGGED(ch, AFF_GROUP))
	{
		send_to_char("But you are not a member of any group!\r\n", ch);
		return;
	}
	sprintf(buf, "%s reports: %d/%dH, %d/%dM, %d/%dV\r\n",
		GET_NAME(ch), GET_HIT(ch), GET_MAX_HIT(ch),
		GET_MANA(ch), GET_MAX_MANA(ch),
		GET_MOVE(ch), GET_MAX_MOVE(ch));
	
	CAP(buf);
	
	k = (ch->master ? ch->master : ch);
	
	for (f = k->followers; f; f = f->next)
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
			send_to_char(buf, f->follower);
	if (k != ch)
		send_to_char(buf, k);
	send_to_char("You report to the group.\r\n", ch);
}


ACMD(do_split)
{
	CHAR_DATA *k;
	FOLLOW_DATA *f;
	int amount, num, share, rest;
	
	if (IS_NPC(ch))
		return;
	
	one_argument(argument, buf);
	
	if (is_number(buf))
	{
		amount = atoi(buf);
		if (amount <= 0)
		{
			send_to_char("Sorry, you can't do that.\r\n", ch);
			return;
		}
		if (amount > get_gold(ch))
		{
			send_to_char("You don't seem to have that much gold to split.\r\n", ch);
			return;
		}
		k = (ch->master ? ch->master : ch);
		
		if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room))
			num = 1;
		else
			num = 0;
		
		for (f = k->followers; f; f = f->next)
		{
			if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
				(!IS_NPC(f->follower)) &&
				(f->follower->in_room == ch->in_room))
				num++;
		}
		
		if (num && AFF_FLAGGED(ch, AFF_GROUP))
		{
			share = amount / num;
			rest = amount % num;
		}
		else
		{
			send_to_char("With whom do you wish to share your gold?\r\n", ch);
			return;
		}
		
		sub_gold(ch, (share * (num - 1)) );
		
		sprintf(buf, "%s splits %d coins; you receive %d.\r\n", GET_NAME(ch), amount, share);
		if (rest)
		{
			sprintf(buf + strlen(buf), "%d coin%s %s not splitable, so %s "
				"keeps the money.\r\n", rest,
				(rest == 1) ? "" : "s",
				(rest == 1) ? "was" : "were",
				GET_NAME(ch));
		}

		if (AFF_FLAGGED(k, AFF_GROUP) && (k->in_room == ch->in_room)
			&& !(IS_NPC(k)) && k != ch)
		{
			add_gold(k, share);
			send_to_char(buf, k);
		}
		for (f = k->followers; f; f = f->next)
		{
			if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
				(!IS_NPC(f->follower)) &&
				(f->follower->in_room == ch->in_room) &&
				f->follower != ch)
			{
				add_gold(f->follower, share);
				send_to_char(buf, f->follower);
			}
		}
		sprintf(buf, "You split %d coins among %d members -- %d coins each.\r\n",
			amount, num, share);
		if (rest)
		{
			sprintf(buf + strlen(buf), "%d coin%s %s not splitable, so you keep "
				"the money.\r\n", rest,
				(rest == 1) ? "" : "s",
				(rest == 1) ? "was" : "were");
			add_gold(ch, rest);
		}
		send_to_char(buf, ch);
	}
	else
	{
		send_to_char("How many coins do you wish to split with your group?\r\n", ch);
		return;
	}
}


/* ******************************************************************** */

ACMD(do_wimpy)
{
	int wimp_lev;
	
	/* 'wimp_level' is a player_special. -gg 2/25/98 */
	if (IS_NPC(ch))
		return;
	
	one_argument(argument, arg);
	
	if (!*arg)
	{
		if (GET_WIMP_LEV(ch))
		{
			sprintf(buf, "Your current wimp level is %d hit points.\r\n", GET_WIMP_LEV(ch));
			send_to_char(buf, ch);
		}
		else
			send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
		return;
	}

	if (isdigit(*arg))
	{
		if ((wimp_lev = atoi(arg)) != 0)
		{
			if (wimp_lev < 0)
				send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
			else if (wimp_lev > GET_MAX_HIT(ch))
				send_to_char("That doesn't make much sense, now does it?\r\n", ch);
			else if (wimp_lev > (GET_MAX_HIT(ch) / 2))
				send_to_char("You can't set your wimp level above half your hit points.\r\n", ch);
			else
			{
				sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n",
					wimp_lev);
				send_to_char(buf, ch);
				GET_WIMP_LEV(ch) = wimp_lev;
			}
		}
		else
		{
			send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
			GET_WIMP_LEV(ch) = 0;
		}
	}
	else
		send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);
}


ACMD(do_display)
{
	size_t i;
	
	if (IS_NPC(ch))
	{
		send_to_char("Mosters don't need displays.  Go away.\r\n", ch);
		return;
	}
	skip_spaces(&argument);
	
	if (!*argument)
	{
		send_to_char("Usage: prompt { { H | M | V } | all | none }\r\n", ch);
		return;
	}
	if (!str_cmp(argument, "on") || !str_cmp(argument, "all"))
		SET_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);
	else if (!str_cmp(argument, "off") || !str_cmp(argument, "none"))
		REMOVE_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);
	else
	{
		REMOVE_BIT(PRF_FLAGS(ch), PRF_DISPHP | PRF_DISPMANA | PRF_DISPMOVE);
		
		for (i = 0; i < strlen(argument); i++)
		{
			switch (LOWER(argument[i]))
			{
			case 'h':
				SET_BIT(PRF_FLAGS(ch), PRF_DISPHP);
				break;
			case 'm':
				SET_BIT(PRF_FLAGS(ch), PRF_DISPMANA);
				break;
			case 'v':
				SET_BIT(PRF_FLAGS(ch), PRF_DISPMOVE);
				break;
			default:
				send_to_char("Usage: prompt { { H | M | V } | all | none }\r\n", ch);
				return;
			}
		}
	}
	
	send_to_char(OK, ch);
}



ACMD(do_gen_write)
{
	FILE *fl;
	char *tmp, buf[MAX_STRING_LENGTH];
	const char *filename;
	struct stat fbuf;
	time_t ct;
	
	switch (subcmd)
	{
	case SCMD_BUG:
		filename = BUG_FILE;
		break;
	case SCMD_TYPO:
		filename = TYPO_FILE;
		break;
	case SCMD_IDEA:
		filename = IDEA_FILE;
		break;
	default:
		return;
	}
	
	ct = time(0);
	tmp = asctime(localtime(&ct));
	
	if (IS_NPC(ch))
	{
		send_to_char("Monsters can't have ideas - Go away.\r\n", ch);
		return;
	}
	
	skip_spaces(&argument);
	delete_doubledollar(argument);
	
	if (!*argument)
	{
		send_to_char("That must be a mistake...\r\n", ch);
		return;
	}
	sprintf(buf, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);
	mudlog(buf, CMP, LVL_IMMORT, FALSE);
	
	if (stat(filename, &fbuf) < 0)
	{
		perror("SYSERR: Can't stat() file");
		return;
	}
	if (fbuf.st_size >= max_filesize)
	{
		send_to_char("Sorry, the file is full right now.. try again later.\r\n", ch);
		return;
	}
	if (!(fl = fopen(filename, "a")))
	{
		perror("SYSERR: do_gen_write");
		send_to_char("Could not open the file.  Sorry.\r\n", ch);
		return;
	}
	fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
		ch->in_room->number, argument);
	fclose(fl);
	send_to_char("Okay.  Thanks!\r\n", ch);
}



#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog)
{
	long result;
	
	const char *tog_messages[][2] =
	{
		{"You are now safe from summoning by other players.\r\n",
		"You may now be summoned by other players.\r\n"},
		{"Nohassle disabled.\r\n",
		"Nohassle enabled.\r\n"},
		{"Brief mode off.\r\n",
		"Brief mode on.\r\n"},
		{"Compact mode off.\r\n",
		"Compact mode on.\r\n"},
		{"You can now hear tells.\r\n",
		"You are now deaf to tells.\r\n"},
		{"You can now hear auctions.\r\n",
		"You are now deaf to auctions.\r\n"},
		{"You can now hear shouts.\r\n",
		"You are now deaf to shouts.\r\n"},
		{"You can now hear gossip.\r\n",
		"You are now deaf to gossip.\r\n"},
		{"You can now hear the congratulation messages.\r\n",
		"You are now deaf to the congratulation messages.\r\n"},
		{"You can now hear the Wiz-channel.\r\n",
		"You are now deaf to the Wiz-channel.\r\n"},
		{"You are no longer part of the Quest.\r\n",
		"Okay, you are part of the Quest!\r\n"},
		{"You will no longer see the room flags.\r\n",
		"You will now see the room flags.\r\n"},
		{"You will now have your communication repeated.\r\n",
		"You will no longer have your communication repeated.\r\n"},
		{"HolyLight mode off.\r\n",
		"HolyLight mode on.\r\n"},
		{"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
		"Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
		{"Autoexits disabled.\r\n",
		"Autoexits enabled.\r\n"},
		{"Will no longer track through doors.\r\n",
		"Will now track through doors.\r\n"},
		{"Wilderness now display symbols instead of words.\r\n",
		"Wilderness now display words instead of symbols.\r\n"},
		{"Wilderness is now colorized.\r\n",
		"Wilderness is now in black and white.\r\n"},
		{"Wilderness is now drawn in normal size.\r\n",
		"Wilderness is now drawn reduced to the max .\r\n"},
		{"You will now accept greets from unknown players.\r\n",
		"You will no longer accept greets from unknown players.\r\n"}
	};
	
	
	if (IS_NPC(ch))
		return;
	
	switch (subcmd)
	{
	case SCMD_NOSUMMON:
		result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
		break;
	case SCMD_NOHASSLE:
		result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
		break;
	case SCMD_BRIEF:
		result = PRF_TOG_CHK(ch, PRF_BRIEF);
		break;
	case SCMD_COMPACT:
		result = PRF_TOG_CHK(ch, PRF_COMPACT);
		break;
	case SCMD_NOTELL:
		result = PRF_TOG_CHK(ch, PRF_NOTELL);
		break;
	case SCMD_NOAUCTION:
		result = PRF_TOG_CHK(ch, PRF_NOAUCT);
		break;
	case SCMD_DEAF:
		result = PRF_TOG_CHK(ch, PRF_DEAF);
		break;
	case SCMD_NOGOSSIP:
		result = PRF_TOG_CHK(ch, PRF_NOGOSS);
		break;
	case SCMD_NOGRATZ:
		result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
		break;
	case SCMD_NOWIZ:
		result = PRF_TOG_CHK(ch, PRF_NOWIZ);
		break;
	case SCMD_QUEST:
		result = PRF_TOG_CHK(ch, PRF_QUEST);
		break;
	case SCMD_ROOMFLAGS:
		result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
		break;
	case SCMD_NOREPEAT:
		result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
		break;
	case SCMD_HOLYLIGHT:
		result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
		break;
	case SCMD_SLOWNS:
		result = (nameserver_is_slow = !nameserver_is_slow);
		break;
	case SCMD_AUTOEXIT:
		result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
		break;
	case SCMD_TRACK:
		result = (track_through_doors = !track_through_doors);
		break;
	case SCMD_WILDBLACK:
		result = PRF_TOG_CHK(ch, PRF_WILDBLACK);
		break;
	case SCMD_WILDSMALL:
		result = PRF_TOG_CHK(ch, PRF_WILDSMALL);
		break;
	case SCMD_WILDTEXT:
		result = PRF_TOG_CHK(ch, PRF_WILDTEXT);
		break;
	case SCMD_NOGREET:
		result = PRF_TOG_CHK(ch, PRF_NOGREET);
		break;
		
	default:
		log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
		return;
	}
	
	if (result)
		send_to_char(tog_messages[subcmd][TOG_ON], ch);
	else
		send_to_char(tog_messages[subcmd][TOG_OFF], ch);
	
	return;
}

/* ************************************************************* */
/* Trophy Code                                                   */
/* ************************************************************* */

bool is_trophy_known(CHAR_DATA *ch, long idnum)
{
	KNOWN_DATA *pKnow;

	if ( !ch || idnum < 0 )
		return (FALSE);

	if (IS_NPC(ch) || IS_IMMORTAL(ch) || GET_IDNUM(ch) == idnum)
		return (TRUE);

	if (!ch->player_specials->first_known)
		return (FALSE);

	for (pKnow = ch->player_specials->first_known; pKnow; pKnow = pKnow->next)
	{
		if (pKnow->idnum == idnum)
			break;
	}

	if (pKnow)
		return (TRUE);

	return (FALSE);
}

/*
 * take off head of a killed enemy corpse
 */
ACMD(do_trophy)
{
	OBJ_DATA *corpse, *obj;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument, arg);

	if (!*arg)
	{
		send_to_char("Usage: trophy <corpse>.\r\n", ch);
		return;
	}

	if (!(corpse = get_obj_in_list_vis_rev(ch, arg, NULL, ch->in_room->last_content)))
	{
		ch_printf(ch, "You don't see %s %s here..\r\n", AN(arg), arg);
		return;
	}

	if (!IS_CORPSE(corpse))
	{
		send_to_char("You can take trophies only from corpses.\r\n", ch);
		return;
	}

	if (GET_OBJ_VAL(corpse, 1) == 0)
	{
		send_to_char("You cannot take a trophy from a mob's corpse.\r\n", ch);
		return;
	}

	if ( !GET_EQ(ch, WEAR_WIELD) ||
	     GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD)) != ITEM_WEAPON ||
	     GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), 3) != TYPE_SLASH )
	{
		send_to_char("You need a slashing weapon to take a trophy.\r\n", ch);
		return;
	}

	if (!roll(GET_DEX(ch)))
	{
		send_to_char("You fail.\r\n", ch);
		return;
	}

	/* ok.. let's make a trophy */
	obj = create_obj();

	GET_OBJ_TYPE(obj)	= ITEM_TROPHY;
	GET_OBJ_COST(obj)	= 1000;
	GET_OBJ_LEVEL(obj)	= 1;
	GET_OBJ_OWNER(obj)	= GET_IDNUM(ch);
	GET_OBJ_WEIGHT(obj)	= 2;
	
	SET_BIT(GET_OBJ_EXTRA(obj), ITEM_UNIQUE | ITEM_NODONATE | ITEM_NOINVIS);
	SET_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);

	if (obj->name)
		free(obj->name);
	obj->name = str_dup("trophy head");
	
	if (obj->short_description)
		free(obj->short_description);
	obj->short_description = str_dup("a corpse's head taken as trophy");

	if (obj->description)
		free(obj->description);
	obj->description = str_dup("A head taken as trophy as been left here.");

	obj_to_char(obj, ch);

	send_to_char("You split off the corpse's head as a trophy.\r\n", ch);
	act("$n splits off the corpse's head as a trophy.", FALSE, ch, NULL, NULL, TO_ROOM); 
}


/* ************************************************************* */
/* Introduction Code                                             */
/* ************************************************************* */

/* return TRUE if victim is known by ch, FALSE otherwise */
bool is_char_known( CHAR_DATA *ch, CHAR_DATA *victim )
{
	KNOWN_DATA *pKnow;

	if ( !ch || !victim )
	{
		log("SYSERR: NULL pointer passed to is_char_known()");
		return (FALSE);
	}

	if ( IS_NPC(ch) || IS_NPC(victim) ||
	     IS_IMMORTAL(ch) || IS_IMMORTAL(victim) ||
	     ch == victim )
		return (TRUE);

	/* he knows no one */
	if (!ch->player_specials->first_known)
		return (FALSE);

	for ( pKnow = ch->player_specials->first_known; pKnow; pKnow = pKnow->next )
	{
		if ( pKnow->idnum == GET_IDNUM(victim) )
			break;
	}

	if ( pKnow )
		return (TRUE);

	return (FALSE);
}

/* return a name for an unknown player */
char *unknown_name(CHAR_DATA *ch)
{
	char baf[MAX_STRING_LENGTH];
	
	if ( IS_NPC(ch) )
		return (GET_NAME(ch));
	
	sprintf(baf, "%s %s %s", UAN(pc_race_types[(int)GET_CLASS(ch)]), pc_race_types[(int)GET_RACE(ch)], pc_class_types[(int)GET_CLASS(ch)] );
	return (str_dup(baf));
}

char *take_name(CHAR_DATA *ch, CHAR_DATA *viewer)
{
	char baf[MAX_STRING_LENGTH];
	
	if (IS_NPC(ch) || IS_NPC(viewer) ||
	    IS_IMMORTAL(ch) || IS_IMMORTAL(viewer) ||
	    is_char_known(viewer, ch))
		return (GET_NAME(ch));
	
	sprintf(baf, "%s %s", pc_race_types[(int)GET_CLASS(ch)], pc_class_types[(int)GET_CLASS(ch)]);
	return (str_dup(baf));
}

char *take_name_keyword(CHAR_DATA *ch, CHAR_DATA *viewer)
{
	if (IS_NPC(ch))
		return (GET_PC_NAME(ch));
	else
		return (take_name(ch, viewer));
}

char *PERS(CHAR_DATA *target, CHAR_DATA *viewer)
{
	if ( !CAN_SEE(viewer, target) )
		return ("Someone");
	
	if (IS_NPC(target) ||
	    IS_NPC(viewer) ||
	    IS_IMMORTAL(target) ||
	    IS_IMMORTAL(viewer) ||
	    is_char_known(viewer, target))
		return (GET_NAME(target));
	
	return ( unknown_name(target) );
}

void know_add(CHAR_DATA *ch, CHAR_DATA *victim)
{
	KNOWN_DATA *pKnow;

	CREATE(pKnow, KNOWN_DATA, 1);
	pKnow->idnum	= GET_IDNUM(victim);
	pKnow->name		= str_dup(GET_NAME(victim));

	LINK(pKnow, ch->player_specials->first_known,
		ch->player_specials->last_known, next, prev);
}

void know_add_load(CHAR_DATA *ch, long idnum, char *chname)
{
	KNOWN_DATA *pKnow;

	CREATE(pKnow, KNOWN_DATA, 1);
	pKnow->idnum	= idnum;
	pKnow->name		= str_dup(chname);

	LINK(pKnow, ch->player_specials->first_known,
		ch->player_specials->last_known, next, prev);
}

void know_remove(CHAR_DATA *ch, KNOWN_DATA *pKnow)
{
	UNLINK(pKnow, ch->player_specials->first_known,
		ch->player_specials->last_known, next, prev);

	if (pKnow->name)
		STRFREE(pKnow->name);

	pKnow->next = NULL;
	pKnow->prev = NULL;

	DISPOSE(pKnow);
}

void know_del_by_id(CHAR_DATA *ch, long idnum)
{
	KNOWN_DATA *pKnow;

	for (pKnow = ch->player_specials->last_known; pKnow; pKnow = pKnow->prev)
	{
		if (pKnow->idnum == idnum)
			break;
	}

	if (!pKnow)
		return;

	know_remove(ch, pKnow);
}


void know_del_by_name(CHAR_DATA *ch, char *Name)
{
	KNOWN_DATA *pKnow;

	for (pKnow = ch->player_specials->last_known; pKnow; pKnow = pKnow->prev)
	{
		if (!str_cmp(pKnow->name, Name))
			break;
	}

	if (!pKnow)
	{
		ch_printf(ch, "You don't know anyone called '%s'.\r\n", Name);
		return;
	}

	know_remove(ch, pKnow);
	ch_printf(ch, "You forget '%s'\r\n", Name);
}

/*
 * ch is the player greetings
 * victim is the player been greeted
 */
ACMD(do_greet)
{
	CHAR_DATA *victim = NULL;
	
	/* no mobs here... */
	if (IS_NPC(ch))
		return;

	if (IS_IMMORTAL(ch))
	{
		send_to_char("You don't need to greet anyone.\r\n", ch);
		return;
	}

	if (GET_LEVEL(ch) < 2)
	{
		send_to_char("You must be at least at level 2 to introduce yourself to someone.\r\n", ch);
		return;
	}

	if (!*argument)
	{
		send_to_char("Greet whom?\r\n", ch);
		return;
	}
	
	argument = one_argument(argument, arg);
	
	if (!(victim = get_char_room_vis(ch, arg, NULL)))
	{
		send_to_char("Greet whom?\r\n", ch);
		return;
	}
	
	if (victim == ch)
	{
		send_to_char("WOW! You greet yourself.\r\n", ch );
		return;
	}

	if (GET_LEVEL(victim) < 2)
	{
		send_to_char("You cannot introduce yourself to level 1 players.\r\n", ch);
		return;
	}

	if (GET_POS(victim) < POS_RESTING)
	{
		send_to_char("They are not paying any attention to you.\r\n", ch);
		return;
	}

	act("You greet $N.", FALSE, ch, NULL, victim, TO_CHAR);
	
	// check if ch is already known by victim
	if (is_char_known(victim, ch))
	{
		act("$N smiles knowingly.", FALSE, victim, NULL, ch, TO_CHAR);
		return;
		//sprintf(buf, "%s smiles knowingly.\r\n", GET_NAME(victim)));
		//send_to_char(buf, ch);
	}

	if (PRF_FLAGGED(victim, PRF_NOGREET))
	{
		ch_printf(victim, "%s nods in greeting you, but you are not interested.\r\n", PERS(ch, victim));
		ch_printf(ch, "%s's not interested in making new knowings.\r\n", HSSH(victim));
		return;
	}

	// add ch to victim's knowings list
	know_add(victim, ch);
	ch_printf(victim, "%s nods in greeting you.\r\n", GET_NAME(ch));
}

C_FUNC(know_del_all)
{
	if (!str_cmp("yes", arg))
	{
		KNOWN_DATA *pKnow, *p_next = NULL;

		for (pKnow = d->character->player_specials->first_known; pKnow; pKnow = p_next)
		{
			p_next = pKnow->next;
			
			know_remove(d->character, pKnow);
		}

		d->character->player_specials->first_known	= NULL;
		d->character->player_specials->last_known	= NULL;
		save_char(d->character, NULL);
		send_to_char("Now your memory is gone.. you remember nobody.\r\n", d->character);
	}
	else
		send_to_char("Nothing will be done...\r\n", d->character);
}


ACMD(do_forget)
{
	char arg1[MAX_INPUT_LENGTH];
	char *spellname = str_dup(argument);

	/* first try to forget a memorized spell.. */
	if (MEMMING_CLASS(ch) && forget_spell(ch, spellname))
		return;

	argument = one_argument(argument, arg1);

	if (!*arg1)
	{
		send_to_char("Forget <name> or Forget <all>\r\n", ch);
		return;
	}

	if (!ch->player_specials->first_known)
	{
		send_to_char("You know noone!\r\n", ch);
		return;
	}

	if (!str_cmp("all", arg1))
	{
		line_input(ch->desc, "Are you sure you want to forget everyone you know (yes/no) ? ", know_del_all, NULL);
	}
	else
		know_del_by_name(ch, arg1);
}


ACMD(do_knowings)
{
	KNOWN_DATA *pKnow;

	// no mobs here...
	if (IS_NPC(ch))
		return;

	if (IS_IMMORTAL(ch))
	{
		send_to_char("You know everybody.\r\n", ch);
		return;
	}

	if (!ch->player_specials->first_known)
	{
		send_to_char("You haven't made any knowings yet!\r\n", ch);
		return;
	}

	send_to_char("You know the following people:\r\n----------------------------------------\r\n", ch);
	for (pKnow = ch->player_specials->first_known; pKnow; pKnow = pKnow->next)
		ch_printf(ch, "%s\r\n", pKnow->name);
	send_to_char("----------------------------------------\r\n", ch);
}


/* ************************************************************* */
/* Mounts Code (DAK)                                             */
/* ************************************************************* */

ACMD(do_mount)
{
	CHAR_DATA *vict;
	char arg[MAX_INPUT_LENGTH];
	
	one_argument(argument, arg);
	
	if (RIDING(ch))
	{
		send_to_char("You are already mounted.\r\n", ch);
		return;
	}

	if (!*arg)
	{
		send_to_char("Mount what?\r\n", ch);
		return;
	}

	if (!GET_SKILL(ch, SKILL_RIDING))
	{
		send_to_char("First you need to learn *how* to ride.\r\n", ch);
		return;
	}

	if (!(vict = get_char_room_vis(ch, arg, NULL)))
	{
		ch_printf(ch, "You don't see %s %s here..\r\n", AN(arg), arg);
		return;
	}

	if (!IS_NPC(vict))
	{
		send_to_char("Ehh... no.\r\n", ch);
		return;
	}

	if (!MOB_FLAGGED(vict, MOB_MOUNTABLE))
	{
		send_to_char("You can't mount that!\r\n", ch);
		return;
	}

	if (MOB_OWNER(vict) && MOB_OWNER(vict) != GET_IDNUM(ch))
	{
		send_to_char("That's not yours.\r\n", ch);
		return;
	}

	if (RIDING(vict) || RIDDEN_BY(vict))
	{
		send_to_char("It is already mounted.\r\n", ch);
		return;
	}
	
	if ( !success(ch, vict, SKILL_RIDING, 0) )
	{
		act("You try to mount $N, but slip and fall off.", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n tries to mount $N, but slips and falls off.", TRUE, ch, NULL, vict, TO_ROOM);
		damage(ch, ch, dice(1, 2), -1);
		return;
	}
	
	act("You mount $N.", FALSE, ch, NULL, vict, TO_CHAR);
	act("$n mounts $N.", TRUE, ch, NULL, vict, TO_ROOM);
	mount_char(ch, vict);
	
	if ( !AFF_FLAGGED(vict, AFF_TAMED) && !success(ch, vict, SKILL_RIDING, 0) )
	{
		act("$N suddenly bucks upwards, throwing you violently to the ground!", FALSE, ch, NULL, vict, TO_CHAR);
		act("$n is thrown to the ground as $N violently bucks!", TRUE, ch, NULL, vict, TO_ROOM);
		dismount_char(ch);
		damage(vict, ch, dice(1,3), -1);
	}
}


ACMD(do_dismount)
{
	if (!RIDING(ch))
	{
		send_to_char("You aren't even riding anything.\r\n", ch);
		return;
	}
	
	if (water_sector(SECT(ch->in_room)) && !has_boat(ch))
	{
		send_to_char("Yah, right, and then drown...\r\n", ch);
		return;
	}
	
	act("You dismount $N.", FALSE, ch, 0, RIDING(ch), TO_CHAR);
	act("$n dismounts $N.", TRUE, ch, 0, RIDING(ch), TO_ROOM);
	dismount_char(ch);
}


ACMD(do_tame)
{
	AFFECTED_DATA af;
	CHAR_DATA *vict;
	char arg[MAX_INPUT_LENGTH];
	
	one_argument(argument, arg);
	
	if (!arg || !*arg)
	{
		send_to_char("Tame who?\r\n", ch);
		return;
	}

	if (!(vict = get_char_room_vis(ch, arg, NULL)))
	{
		send_to_char("They're not here.\r\n", ch);
		return;
	}

	if (FIGHTING(vict) || FIGHTING(ch))
	{
		send_to_char("This is really not a good time to be taming!\r\n", ch);
		return;
	}

	if (!IS_NPC(vict))
	{
		send_to_char("Don't be silly...\r\n", ch);
		return;
	}

	if (!MOB_FLAGGED(vict, MOB_MOUNTABLE))
	{
		send_to_char("It seems that it cannot be tamed...\r\n", ch);
		return;
	}

	if (!GET_SKILL(ch, SKILL_TAME))
	{
		send_to_char("You don't even know how to tame something.\r\n", ch);
		return;
	}

	if ( !success(ch, vict, SKILL_TAME, 0) )
	{
		act( "You fail in taming $N.", FALSE, ch, NULL, vict, TO_CHAR );
		return;
	}
	
	af.type			= 0;
	af.duration		= 24;
	af.modifier		= 0;
	af.location		= APPLY_NONE;
	af.bitvector	= AFF_TAMED;
	affect_join(vict, &af, FALSE, FALSE, FALSE, FALSE);
	
	act("You tame $N.", FALSE, ch, 0, vict, TO_CHAR);
	act("$n tames $N.", FALSE, ch, 0, vict, TO_ROOM);
}


/* ************************************************************* */
/* Track & Hunting code                                          */
/* ************************************************************* */

/*
 * this function check if ch and vict are
 * in the wilderness or not
 * 
 * return TRUE if both ch and vict are in wild
 * return TRUE if both ch and vict are NOT in wild
 * return FALSE otherwise
 */
bool check_wild( CHAR_DATA *ch, CHAR_DATA *vict )
{
	bool both = FALSE;

	if ( IN_WILD(ch) && IN_WILD(vict) )
		both = TRUE;

	if ( !IN_WILD(ch) && !IN_WILD(vict) )
		both = TRUE;

	return (both);
}

/*
 * return 1 if ch has SKILL_TRACK - one single track
 * return 2 if ch has spells      - continuo track
 * return 0 if ch cannot track
 */
int can_track(CHAR_DATA *ch)
{
	if ( GET_SKILL(ch, SKILL_TRACK) )
		return (1);

	if (affected_by_spell(ch, SPELL_MINOR_TRACK) ||
	    affected_by_spell(ch, SPELL_MAJOR_TRACK))
		return (2);

	return (0);
}

void start_hunting(CHAR_DATA *ch, CHAR_DATA *vict)
{
	if ( !ch || !vict )
		return;

	HUNTING(ch) = vict;
	SET_BIT(AFF_FLAGS(ch), AFF_HUNTING);
}

void stop_hunting(CHAR_DATA *ch)
{
	if ( !ch )
		return;

	HUNTING(ch) = NULL;
	REMOVE_BIT(AFF_FLAGS(ch), AFF_HUNTING);
}


ACMD(do_track)
{
	CHAR_DATA *vict;
	int dir, mode;
	
	/* The character must have the track skill. */
	if (IS_NPC(ch) || !(mode = can_track(ch)) )
	{
		send_to_char("You have no idea how.\r\n", ch);
		return;
	}
	one_argument(argument, arg);
	if (!*arg)
	{
		if ( mode == 2 && HUNTING(ch) )
		{
			send_to_char("You stop hunting your current prey.\r\n", ch);
			stop_hunting(ch);
		}
		else
			send_to_char("Whom are you trying to track?\r\n", ch);
		return;
	}
	/* The person can't see the victim. */
	if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
	{
		send_to_char("No one is around by that name.\r\n", ch);
		return;
	}

	if ( !check_wild(ch, vict) )
	{
		send_to_char("You sense no trail.\r\n", ch);
		return;
	}
	
	/*
	 * 101 is a complete failure, no matter what the proficiency.
	 * but only with skill -- not spells.
	 */
	if ( mode == 1 && !success(ch, vict, SKILL_TRACK, 0) )
	{
		send_to_char( "You fail to sense a trail!\r\n", ch );
		return;
	}
	
	if ( IN_WILD(ch) )
	{
		if ( wild_track( ch, vict, FALSE ) != NOWHERE && mode == 2 )
			start_hunting(ch, vict);
		return;
	}
	
	/* They passed the skill check. */
	dir = find_first_step(ch->in_room, vict->in_room);
	
	switch (dir)
	{
	case BFS_ERROR:
		send_to_char("Hmm.. something seems to be wrong.\r\n", ch);
		break;
	case BFS_ALREADY_THERE:
		send_to_char("You're already in the same room!!\r\n", ch);
		break;
	case BFS_NO_PATH:
		ch_printf(ch, "You can't sense a trail to %s from here.\r\n", HMHR(vict));
		break;
	default:	/* Success! */
		ch_printf(ch, "You sense a trail %s%s&0 from here!\r\n",
			exits_color[dir], dirs[dir]);
		if ( mode == 2 )
			start_hunting(ch, vict);
		break;
	}
}


/*
 * return 0 if hunt stops, 1 otherwise
 */
int hunt_victim(CHAR_DATA *ch)
{
	CHAR_DATA *tmp;
	int dir;
	
	if (!ch || !HUNTING(ch) || FIGHTING(ch))
		return (0);
	
	/* make sure the char still exists */
	for (tmp = character_list; tmp; tmp = tmp->next)
	{
		if (HUNTING(ch) == tmp)
			break;
	}
	
	if ( !tmp || !CAN_SEE(ch, HUNTING(ch)) || !check_wild(ch, HUNTING(ch)) )
	{
		if (!IS_NPC(ch))
			send_to_char("You sense no trail.\r\n", ch);
		stop_hunting(ch);
		return (0);
	}

	/*
	 * handle distance:
	 *
	 * wild_track checks for MAX_TRACK_DIST
	 *
	 * in standard world, a track is lost when hunter and hunted
	 * are in different zones
	 */
	if ( IN_WILD(ch) )
		dir = wild_track( ch, HUNTING(ch), TRUE );
	else
	{
		if ( ch->in_room->zone != HUNTING(ch)->in_room->zone )
			dir = NOWHERE;
		else
			dir = find_first_step(ch->in_room, HUNTING(ch)->in_room);
	}

	if (dir < 0)
	{
		if (!IS_NPC(ch))
			ch_printf(ch, "You can't sense a trail to %s from here.\r\n", HMHR(HUNTING(ch)));
		stop_hunting(ch);
		return (0);
	}

	if (!IS_NPC(ch))
		ch_printf(ch, "You sense a trail %s%s&0 from here!\r\n",
			exits_color[dir], dirs[dir]);
	else
	{
		if ( !perform_move(ch, dir, 1) )
			stop_hunting(ch);
		else
			if (ch->in_room == HUNTING(ch)->in_room)
				hit(ch, HUNTING(ch), TYPE_UNDEFINED);
	}

	return (1);
}
