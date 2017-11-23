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
*   File: comm.h                                        Part of CircleMUD *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define NUM_RESERVED_DESCS	8

/* comm.c */
void	send_to_all(const char *messg);
void	send_to_char(const char *messg, CHAR_DATA *ch);
void	send_to_room(const char *messg, ROOM_DATA *room);
void	send_to_outdoor(const char *messg);
void	send_to_wildsect(WILD_DATA *wd, const char *messg);
void	ch_printf( CHAR_DATA *ch, char *fmt, ... );
void	perform_to_all(const char *messg, CHAR_DATA *ch);
void	close_socket(DESCRIPTOR_DATA *d);

void	perform_act(const char *orig, CHAR_DATA *ch,
		OBJ_DATA *obj, const void *vict_obj, const CHAR_DATA *to);

void	act(const char *str, int hide_invisible, CHAR_DATA *ch,
		OBJ_DATA *obj, const void *vict_obj, int type);

#define TO_ROOM		1
#define TO_VICT		2
#define TO_NOTVICT	3
#define TO_CHAR		4
#define TO_SLEEP	128	/* to char, even if sleeping */

/* I/O functions */
int	write_to_descriptor(socket_t desc, const char *txt);
void	write_to_q(const char *txt, TXT_Q *queue, int aliased);
void	write_to_output(const char *txt, DESCRIPTOR_DATA *d);
void	string_add(DESCRIPTOR_DATA *d, char *str);
void	string_write(DESCRIPTOR_DATA *d, char **txt, size_t len, long mailto, void *data);

#define PAGE_LENGTH	22
#define PAGE_WIDTH	80
void	page_string(DESCRIPTOR_DATA *d, char *str, int keep_internal);

#define SEND_TO_Q(messg, desc)  write_to_output((messg), desc)

typedef RETSIGTYPE sigfunc(int);

