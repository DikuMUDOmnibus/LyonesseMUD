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
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
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
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "clan.h"

/* extern variables */
extern int level_can_shout;
extern int holler_move_cost;

/* local functions */
void perform_tell(CHAR_DATA *ch, CHAR_DATA *vict, char *arg);
int is_tell_ok(CHAR_DATA *ch, CHAR_DATA *vict);
ACMD(do_say);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
ACMD(do_qcomm);


ACMD(do_say)
{
	skip_spaces(&argument);
	
	if (!*argument)
		send_to_char("Yes, but WHAT do you want to say?\r\n", ch);
	else
	{
		sprintf(buf, "$n says, '%s'", argument);
		act(buf, FALSE, ch, 0, 0, TO_ROOM);
		
		if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else
		{
			delete_doubledollar(argument);
			sprintf(buf, "You say, '%s'\r\n", argument);
			send_to_char(buf, ch);
		}

		/* Room Trigger Events */
		if ( ch->in_room->trigger )
			check_room_trigger(ch, TRIG_ACT_SPEAK);
	}
}


ACMD(do_gsay)
{
	CHAR_DATA *k;
	FOLLOW_DATA *f;
	
	skip_spaces(&argument);
	
	if (!AFF_FLAGGED(ch, AFF_GROUP))
	{
		send_to_char("But you are not the member of a group!\r\n", ch);
		return;
	}

	if (!*argument)
	{
		send_to_char("Yes, but WHAT do you want to group-say?\r\n", ch);
		return;
	}
	
	if (ch->master)
		k = ch->master;
	else
		k = ch;
	
	sprintf(buf, "$n tells the group, '%s'", argument);
	
	if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch))
		act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
	for (f = k->followers; f; f = f->next)
	{
		if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch))
			act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);
	}
	
	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
	else
	{
		sprintf(buf, "You tell the group, '%s'\r\n", argument);
		send_to_char(buf, ch);
	}

	/* Room Trigger Events */
	if ( ch->in_room->trigger )
		check_room_trigger(ch, TRIG_ACT_SPEAK);
}


void perform_tell(CHAR_DATA *ch, CHAR_DATA *vict, char *arg)
{
	sprintf(buf, "&b&2$n tells you, '%s'&0", arg);
	act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
	
	if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
	else
	{
		sprintf(buf, "&b&2You tell $N, '%s'&0", arg);
		act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	}
	
	if (!IS_NPC(vict) && !IS_NPC(ch))
		GET_LAST_TELL(vict) = GET_IDNUM(ch);

	/* Room Trigger Events */
	if ( ch->in_room->trigger )
		check_room_trigger(ch, TRIG_ACT_SPEAK);
}

int is_tell_ok(CHAR_DATA *ch, CHAR_DATA *vict)
{
	if (ch == vict)
		send_to_char("You try to tell yourself something.\r\n", ch);
	else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
		send_to_char("You can't tell other people while you have notell on.\r\n", ch);
	else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
		send_to_char("The walls seem to absorb your words.\r\n", ch);
	else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
		act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if (PLR_FLAGGED(vict, PLR_WRITING))
		act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF))
		act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
	else
		return (TRUE);
	
	return (FALSE);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
	CHAR_DATA *vict = NULL;
	
	half_chop(argument, buf, buf2);
	
	if (!*buf || !*buf2)
		send_to_char("Who do you wish to tell what??\r\n", ch);
	else if (GET_LEVEL(ch) < LVL_IMMORT && !(vict = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
		send_to_char(NOPERSON, ch);
	else if (GET_LEVEL(ch) >= LVL_IMMORT && !(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
		send_to_char(NOPERSON, ch);
	else if (is_tell_ok(ch, vict))
		perform_tell(ch, vict, buf2);
}


ACMD(do_reply)
{
	CHAR_DATA *tch = character_list;
	
	if (IS_NPC(ch))
		return;
	
	skip_spaces(&argument);
	
	if (GET_LAST_TELL(ch) == NOBODY)
		send_to_char("You have no-one to reply to!\r\n", ch);
	else if (!*argument)
		send_to_char("What is your reply?\r\n", ch);
	else
	{
		/*
		 * Make sure the person you're replying to is still playing by searching
		 * for them.  Note, now last tell is stored as player IDnum instead of
		 * a pointer, which is much better because it's safer, plus will still
		 * work if someone logs out and back in again.
		 */
		
		/*
		 * XXX: A descriptor list based search would be faster although
		 *      we could not find link dead people.  Not that they can
		 *      hear tells anyway. :) -gg 2/24/98
		 */
		while (tch != NULL && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
			tch = tch->next;
		
		if (tch == NULL)
			send_to_char("They are no longer playing.\r\n", ch);
		else if (is_tell_ok(ch, tch))
			perform_tell(ch, tch, argument);
	}
}


ACMD(do_spec_comm)
{
	CHAR_DATA *vict;
	const char *action_sing, *action_plur, *action_others;
	
	switch (subcmd)
	{
	case SCMD_WHISPER:
		action_sing		= "whisper to";
		action_plur		= "whispers to";
		action_others	= "$n whispers something to $N.";
		break;
		
	case SCMD_ASK:
		action_sing		= "ask";
		action_plur		= "asks";
		action_others	= "$n asks $N a question.";
		break;
		
	default:
		action_sing		= "oops";
		action_plur		= "oopses";
		action_others	= "$n is tongue-tied trying to speak with $N.";
		break;
	}
	
	half_chop(argument, buf, buf2);
	
	if (!*buf || !*buf2)
	{
		sprintf(buf, "Whom do you want to %s.. and what??\r\n", action_sing);
		send_to_char(buf, ch);
	}
	else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
		send_to_char(NOPERSON, ch);
	else if (vict == ch)
		send_to_char("You can't get your mouth close enough to your ear...\r\n", ch);
	else
	{
		sprintf(buf, "$n %s you, '%s'", action_plur, buf2);
		act(buf, FALSE, ch, 0, vict, TO_VICT);
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else
		{
			sprintf(buf, "You %s %s, '%s'\r\n", action_sing, PERS(vict, ch), buf2);
			send_to_char(buf, ch);
		}
		act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);

		/* Room Trigger Events */
		if ( ch->in_room->trigger )
			check_room_trigger(ch, TRIG_ACT_SPEAK);
	}
}



#define MAX_NOTE_LENGTH 1000	/* arbitrary */

ACMD(do_write)
{
	OBJ_DATA *paper, *pen = NULL;
	char *papername, *penname;
	
	papername = buf1;
	penname = buf2;
	
	two_arguments(argument, papername, penname);
	
	if (!ch->desc)
		return;
	
	if (!*papername)		/* nothing was delivered */
	{
		send_to_char("Write?  With what?  ON what?  What are you trying to do?!?\r\n", ch);
		return;
	}
	if (*penname)		/* there were two arguments */
	{
		if (!(paper = get_obj_in_list_vis_rev(ch, papername, NULL, ch->last_carrying)))
		{
			sprintf(buf, "You have no %s.\r\n", papername);
			send_to_char(buf, ch);
			return;
		}
		if (!(pen = get_obj_in_list_vis_rev(ch, penname, NULL, ch->last_carrying)))
		{
			sprintf(buf, "You have no %s.\r\n", penname);
			send_to_char(buf, ch);
			return;
		}
	}
	else		/* there was one arg.. let's see what we can find */
	{
		if (!(paper = get_obj_in_list_vis_rev(ch, papername, NULL, ch->last_carrying)))
		{
			sprintf(buf, "There is no %s in your inventory.\r\n", papername);
			send_to_char(buf, ch);
			return;
		}
		if (GET_OBJ_TYPE(paper) == ITEM_PEN)	/* oops, a pen.. */
		{
			pen = paper;
			paper = NULL;
		}
		else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
		{
			send_to_char("That thing has nothing to do with writing.\r\n", ch);
			return;
		}
		/* One object was found.. now for the other one. */
		if (!GET_EQ(ch, WEAR_HOLD))
		{
			sprintf(buf, "You can't write with %s %s alone.\r\n", AN(papername),
				papername);
			send_to_char(buf, ch);
			return;
		}
		if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_HOLD)))
		{
			send_to_char("The stuff in your hand is invisible!  Yeech!!\r\n", ch);
			return;
		}
		if (pen)
			paper = GET_EQ(ch, WEAR_HOLD);
		else
			pen = GET_EQ(ch, WEAR_HOLD);
	}
	
	
	/* ok.. now let's see what kind of stuff we've found */
	if (GET_OBJ_TYPE(pen) != ITEM_PEN)
		act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
	else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
		act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
	else if (paper->action_description)
		send_to_char("There's something written on it already.\r\n", ch);
	else
	{
		/* we can write - hooray! */
		send_to_char("Write your note.  End with '@' on a new line.\r\n", ch);
		act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
		string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, NULL);
	}
}



ACMD(do_page)
{
	DESCRIPTOR_DATA *d;
	CHAR_DATA *vict;
	
	half_chop(argument, arg, buf2);
	
	if (IS_NPC(ch))
		send_to_char("Monsters can't page.. go away.\r\n", ch);
	else if (!*arg)
		send_to_char("Whom do you wish to page?\r\n", ch);
	else
	{
		sprintf(buf, "\007\007*$n* %s", buf2);
		if (!str_cmp(arg, "all"))
		{
			if (GET_LEVEL(ch) > LVL_GOD)
			{
				for (d = descriptor_list; d; d = d->next)
					if (STATE(d) == CON_PLAYING && d->character)
						act(buf, FALSE, ch, 0, d->character, TO_VICT);
			}
			else
				send_to_char("You will never be godly enough to do that!\r\n", ch);
			return;
		}

		if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) != NULL)
		{
			act(buf, FALSE, ch, 0, vict, TO_VICT);
			if (PRF_FLAGGED(ch, PRF_NOREPEAT))
				send_to_char(OK, ch);
			else
				act(buf, FALSE, ch, 0, vict, TO_CHAR);
		}
		else
			send_to_char("There is no such person in the game!\r\n", ch);
	}
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
	DESCRIPTOR_DATA *i;
	char color_on[24];
	
	/* Array of flags which must _not_ be set in order for comm to be heard */
	int channels[] =
	{
		0,
		PRF_DEAF,
		PRF_NOGOSS,
		PRF_NOAUCT,
		PRF_NOGRATZ,
		0
	};
	
	/*
	 * com_msgs: [0] Message if you can't perform the action because of noshout
	 *           [1] name of the action
	 *           [2] message if you're not on the channel
	 *           [3] a color string.
	 */
	const char *com_msgs[][4] =
	{
		{"You cannot holler!!\r\n",
		"holler",
		"",
		KYEL},
			
		{"You cannot shout!!\r\n",
		"shout",
		"Turn off your noshout flag first!\r\n",
		KYEL},
		
		{"You cannot gossip!!\r\n",
		"gossip",
		"You aren't even on the channel!\r\n",
		KYEL},
		
		{"You cannot auction!!\r\n",
		"auction",
		"You aren't even on the channel!\r\n",
		KMAG},
		
		{"You cannot congratulate!\r\n",
		"congrat",
		"You aren't even on the channel!\r\n",
		KGRN}
	};

	/* to keep pets, etc from being ordered to shout */
	if (!ch->desc)
		return;
	
	if (PLR_FLAGGED(ch, PLR_NOSHOUT))
	{
		send_to_char(com_msgs[subcmd][0], ch);
		return;
	}
	if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF))
	{
		send_to_char("The walls seem to absorb your words.\r\n", ch);
		return;
	}
	/* level_can_shout defined in config.c */
	if (GET_LEVEL(ch) < level_can_shout)
	{
		sprintf(buf1, "You must be at least level %d before you can %s.\r\n",
			level_can_shout, com_msgs[subcmd][1]);
		send_to_char(buf1, ch);
		return;
	}
	/* make sure the char is on the channel */
	if (PRF_FLAGGED(ch, channels[subcmd]))
	{
		send_to_char(com_msgs[subcmd][2], ch);
		return;
	}
	/* skip leading spaces */
	skip_spaces(&argument);
	
	/* make sure that there is something there to say! */
	if (!*argument)
	{
		sprintf(buf1, "Yes, %s, fine, %s we must, but WHAT???\r\n",
			com_msgs[subcmd][1], com_msgs[subcmd][1]);
		send_to_char(buf1, ch);
		return;
	}
	if (subcmd == SCMD_HOLLER)
	{
		if (GET_MOVE(ch) < holler_move_cost)
		{
			send_to_char("You're too exhausted to holler.\r\n", ch);
			return;
		}
		else
			GET_MOVE(ch) -= holler_move_cost;
	}
	/* set up the color on code */
	strcpy(color_on, com_msgs[subcmd][3]);
	
	/* first, set up strings to be given to the communicator */
	if (PRF_FLAGGED(ch, PRF_NOREPEAT))
		send_to_char(OK, ch);
	else
	{
		if (COLOR_LEV(ch) >= C_CMP)
			sprintf(buf1, "%sYou %s, '%s'%s", color_on, com_msgs[subcmd][1],
			argument, KNRM);
		else
			sprintf(buf1, "You %s, '%s'", com_msgs[subcmd][1], argument);
		act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
	}
	
	sprintf(buf, "$n %ss, '%s'", com_msgs[subcmd][1], argument);
	
	/* now send all the strings out */
	for (i = descriptor_list; i; i = i->next)
	{
		if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
			!PRF_FLAGGED(i->character, channels[subcmd]) &&
			!PLR_FLAGGED(i->character, PLR_WRITING) &&
			!ROOM_FLAGGED(i->character->in_room, ROOM_SOUNDPROOF))
		{	
			if (subcmd == SCMD_SHOUT &&
				((ch->in_room->zone != i->character->in_room->zone) ||
				!AWAKE(i->character)))
				continue;
			
			if (COLOR_LEV(i->character) >= C_NRM)
				send_to_char(color_on, i->character);
			act(buf, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
			if (COLOR_LEV(i->character) >= C_NRM)
				send_to_char(KNRM, i->character);
		}
	}

	/* Room Trigger Events */
	if ( ch->in_room->trigger )
		check_room_trigger(ch, TRIG_ACT_SPEAK);
}


ACMD(do_qcomm)
{
	DESCRIPTOR_DATA *i;
	
	if (!PRF_FLAGGED(ch, PRF_QUEST))
	{
		send_to_char("You aren't even part of the quest!\r\n", ch);
		return;
	}
	skip_spaces(&argument);
	
	if (!*argument)
	{
		sprintf(buf, "%s?  Yes, fine, %s we must, but WHAT??\r\n", CMD_NAME,
			CMD_NAME);
		CAP(buf);
		send_to_char(buf, ch);
	}
	else
	{
		if (PRF_FLAGGED(ch, PRF_NOREPEAT))
			send_to_char(OK, ch);
		else
		{
			if (subcmd == SCMD_QSAY)
				sprintf(buf, "You quest-say, '%s'", argument);
			else
				strcpy(buf, argument);
			act(buf, FALSE, ch, 0, argument, TO_CHAR);
		}
		
		if (subcmd == SCMD_QSAY)
			sprintf(buf, "$n quest-says, '%s'", argument);
		else
			strcpy(buf, argument);
		
		for (i = descriptor_list; i; i = i->next)
			if (STATE(i) == CON_PLAYING && i != ch->desc && PRF_FLAGGED(i->character, PRF_QUEST))
				act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
	}
}

ACMD(do_ctell)
{
	DESCRIPTOR_DATA *i;

	if ( GET_CLAN(ch) == 0 )
	{
		send_to_char ("You're not part of a clan.\r\n", ch);
		return;
	}
	
	skip_spaces(&argument);
	one_argument( argument, arg );

	if (!*arg)
	{
		send_to_char("What do you want to tell your clan?\r\n", ch);
		return;
	}
	
	if (PRF_FLAGGED(ch,PRF_NOREPEAT))
		sprintf(buf1, OK);
	else
		sprintf(buf1, "You tell your clan, '%s'\r\n", argument);
	send_to_char (buf1, ch);
	
	for (i = descriptor_list; i; i = i->next)
	{
		if ( GET_CLAN(i->character) == GET_CLAN(ch) )
		{
			if ( strcmp( GET_NAME(i->character), GET_NAME(ch) ) )
			{
				if( GET_CLAN_RANK(ch) == RANK_LEADER )
					sprintf(buf,"[CLAN]: Leader %s says, '%s'.\r\n", GET_NAME(ch), arg);
				else
					sprintf(buf,"[CLAN]: %s says, '%s'.\r\n", PERS(ch, i->character), arg);
				send_to_char(buf, i->character);
			}
		}
	}

	/* Room Trigger Events */
	if ( ch->in_room->trigger )
		check_room_trigger(ch, TRIG_ACT_SPEAK);
}
