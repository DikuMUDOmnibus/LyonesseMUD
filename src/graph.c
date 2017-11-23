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
*   File: graph.c                                       Part of CircleMUD *
*  Usage: various graph algorithms                                        *
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

typedef struct	bfs_queue_struct	BFS_QUEUE;

struct bfs_queue_struct
{
  BFS_QUEUE			*next;
  ROOM_DATA			*room;
  int				dir;
};


#define MARK(room)		(SET_BIT(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define UNMARK(room)	(REMOVE_BIT(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define IS_MARKED(room)	(ROOM_FLAGGED(room, ROOM_BFS_MARK))


/* external variables */
extern int	track_through_doors;

/* local globals */
static BFS_QUEUE *queuehead = NULL;
static BFS_QUEUE *queuetail = NULL;

/* local functions */
EXIT_DATA *valid_edge(ROOM_DATA *pRoom, int dir);
int		find_first_step(ROOM_DATA *src, ROOM_DATA *target);
void	bfs_enqueue(ROOM_DATA *room, int dir);
void	bfs_dequeue(void);
void	bfs_clear_queue(void);

/* ============================================================== */

EXIT_DATA *valid_edge(ROOM_DATA *pRoom, int dir)
{
	EXIT_DATA *pexit;

	if (IS_WILD(pRoom))
		return (NULL);
	pexit = get_exit(pRoom, dir);
	if ( pexit == NULL || pexit->to_room == NULL || IS_WILD(pexit->to_room) )
		return (NULL);
	if (track_through_doors == FALSE && EXIT_FLAGGED(pexit, EX_CLOSED))
		return (NULL);
	if (ROOM_FLAGGED(pexit->to_room, ROOM_NOTRACK) || IS_MARKED(pexit->to_room))
		return (NULL);
	
	return (pexit);
}

void bfs_enqueue(ROOM_DATA *room, int dir)
{
	BFS_QUEUE *curr;
	
	CREATE(curr, BFS_QUEUE, 1);
	curr->next			= NULL;
	curr->room			= room;
	curr->dir			= dir;
	
	if (queuetail)
	{
		queuetail->next = curr;
		queuetail		= curr;
	}
	else
		queuehead = queuetail = curr;
}


void bfs_dequeue(void)
{
	BFS_QUEUE *curr;
	
	curr = queuehead;
	
	if (!(queuehead = queuehead->next))
		queuetail = NULL;
	free(curr);
}


void bfs_clear_queue(void)
{
	while (queuehead)
		bfs_dequeue();
}


/* 
 * find_first_step: given a source room and a target room, find the first
 * step on the shortest path from the source to the target.
 *
 * Intended usage: in mobile_activity, give a mob a dir to go if they're
 * tracking another mob or a PC.  Or, a 'track' skill for PCs.
 */
int find_first_step(ROOM_DATA *src, ROOM_DATA *target)
{
	ROOM_DATA *pRoom;
	EXIT_DATA *pexit;
	int curr_dir, iHash;
	
	if (!src || !target)
	{
		log("SYSERR: Illegal value %d or %d passed to find_first_step. (%s)", src->number, target->number, __FILE__);
		return (BFS_ERROR);
	}

	if (src == target)
		return (BFS_ALREADY_THERE);
	
	/* clear marks first, some OLC systems will save the mark. */
	for (iHash = 0; iHash < ROOM_HASH; iHash++)
	{
		for (pRoom = World[iHash]; pRoom; pRoom = pRoom->next)
			UNMARK(pRoom);
	}

	MARK(src);
	
	/* first, enqueue the first steps, saving which direction we're going. */
	//for ( pexit = pRoom->first_exit; pexit; pexit->next )
	for (curr_dir = 0; curr_dir < NUM_OF_EX_DIRS; curr_dir++)
	{
		if ((pexit = valid_edge(src, curr_dir)) != NULL)
		{
			MARK(pexit->to_room);
			bfs_enqueue(pexit->to_room, curr_dir);
		}
	}
		
	/* now, do the classic BFS. */
	while (queuehead)
	{
		if (queuehead->room == target)
		{
			curr_dir = queuehead->dir;
			bfs_clear_queue();
			return (curr_dir);
		}
		else
		{
			for (curr_dir = 0; curr_dir < NUM_OF_EX_DIRS; curr_dir++)
			{
				if ((pexit = valid_edge(queuehead->room, curr_dir)) != NULL)
				{
					MARK(pexit->to_room);
					bfs_enqueue(pexit->to_room, queuehead->dir);
				}
			}
			bfs_dequeue();
		}
	}
	
	return (BFS_NO_PATH);
}
