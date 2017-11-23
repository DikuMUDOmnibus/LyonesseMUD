#ifdef __RCSID__
static char rcsid[] = "$Id: parser.c,v 1.15 1997/11/06 23:42:46 brazil Exp brazil $";
#endif

#include "conf.h"
#include "sysdep.h"

typedef struct
{
  int			type;
  int			value;
} TOKEN;


#define TOP			256
#define WTOP			3
#define NON_TERMINAL		0
#define END_OF_INPUT		10
#define MAX_ANSWER		32767
#define SCR_WIDTH		76

#include "structs.h"
#include "utils.h"

#define NO			0
#define YES			1

/* local functions */
void	parse_pop( TOKEN *working, TOKEN *stack );
int	HasKet( char *s );
char	strip_expression( char *word, char *expression_str, char *offset );
char	get_numbers( char *expression_str, int *bracket_i, int *word_picked );
int	lexical_analyser( char *in_string, struct char_data *ch );
char	lookup_value( char *variable, int *value, CHAR_DATA *ch, ROOM_DATA *room, OBJ_DATA *obj );
char	calculate_value( char *variable, int *value );
int	syntax_analyser( TOKEN *input );
int	top_most_terminal( TOKEN *stack );
long	parser_evaluate_expression( TOKEN *working );
int	power10( int num );
void	clean_up( char *in, CHAR_DATA *ch );
void	parse_push( TOKEN *input, TOKEN *stack );

/* ------------------------------------------------ */

void parse_push(TOKEN * input, TOKEN * stack)
{
	stack[TOP].type++;
	stack[stack[TOP].type].type = input->type;
	stack[stack[TOP].type].value = input->value;
	return;
}

void parse_pop(TOKEN * working, TOKEN * stack)
{
	working[WTOP].type++;
	working[working[WTOP].type].type = stack[stack[TOP].type].type;
	working[working[WTOP].type].value = stack[stack[TOP].type].value;
	stack[TOP].type--;
	return;
}

int HasKet(char *s)
{
	int i;
	
	for (i = 0; s[i] != 0; i++)
		if (s[i] == ']')
			return (1);
		
	return (0);
}

void master_parser(char *buf, CHAR_DATA *ch, ROOM_DATA *room, OBJ_DATA *obj)
{
	char *buf2p, *str[10], *pos[10];
	int wordnum = 0;
	int i, j, k, l, m;
	int bracket_i, word_picked, picked;
	char any_true, offset, result;
	char buf2[1000];
	char tmp[100];
	char expression_str[256];
	int selection[25];
	int all_words, poss_words, p_words, bracket;
	
	bracket = 1;

	/* begin parse */
	while (HasKet (buf))
	{
		wordnum = 0;
		poss_words = 0;
		p_words = 0;
		any_true = NO;
		for (j = 0; (buf[j] != ']') && (buf[j] != '\0'); j++);
		for (i = j; (buf[i] != '[') && (i != 0); i--);
		for (k = 0; k < (j - i - 1); k++)
			buf2[k] = buf[k + i + 1];
		buf2[k] = '\0';
		str[wordnum] = buf2;

		for (buf2p = buf2; *buf2p; buf2p++)
		{
			if (*buf2p == '\\')
			{
				*buf2p = '\0';
				wordnum++;
				str[wordnum] = buf2p + 1;
			}
		}

		for (all_words = 0; all_words < wordnum + 1; all_words++)
		{
			if ((result = strip_expression (str[all_words], expression_str, &offset)))
				str[all_words] += offset;		/* Offset the string past the expression */
			
			switch (result)
			{
			case 0:		/* No expression */
				/* Add to group of possibilities */
				str[poss_words] = str[all_words];
				poss_words++;
				any_true = YES;
				break;
			case 1:		/* A '!' */
				pos[p_words] = str[all_words];	/* Save to check later */
				p_words++;
				break;
			case 2:		/* An expression */
				/* If exp is true */
				if (lexical_analyser(expression_str, ch))
				{
					/* Add to group of possibilities */
					str[poss_words] = str[all_words];
					poss_words++;
					any_true = YES;
				}
				break;
			case 3:		/* A '#' */
				if (get_numbers (expression_str, &bracket_i, &word_picked))
				{
					/*
					 * If the word picked for the 'bracket_i'th bracket was
					 * 'word_picked' then make this a possibility
					 */
					if (selection[bracket_i] == word_picked)
					{
						str[poss_words] = str[all_words];
						poss_words++;
						any_true = YES;
					}
				}
			}		/* End Case */
		}		/* End For */
		
		/*
		 * No go through all the ! expressions and make them possibilities
		 * if none of the other expressions were true
		 */
		if ((!any_true) && (p_words > 0))
		{
			for (all_words = 0; all_words < p_words; all_words++)
			{
				str[poss_words] = pos[all_words];
				poss_words++;
			}
		}
		if (poss_words == 0)
		{
			*tmp = 0;
		}
		else
		{
			picked = (rand () % poss_words);
			selection[bracket] = picked + 1;
			bracket++;
			strcpy (tmp, str[picked]);
		}
		/* Now, between put tmp back into buf where it belongs. */
		for (l = i; l < (i + (int)strlen(tmp)); l++)
			buf[l] = tmp[l - i];
		for (m = l; (buf[m] != '\0'); m++)
			buf[m] = buf[m + ((j + 1) - l)];
		buf[m] = '\0';
		/* Now run buf through cleanup to distribute evenly on lines */
	}

	clean_up(buf, ch);
}

/*
 * Strips the expression out of the word passed to it, and returns the
 * expression in expression_str, offset is the value that must be added
 * to the string passed to strip out the expression.  Also returns a
 * char
 * representing what kind of expression there was:
 * 0 - No Expression,  1 - !,  2 - ?,  3 - #
 */
char strip_expression(char *word, char *expression_str, char *offset)
{
	char type, spaces;
	int i;
	char buf0[128];
	
	spaces = 0;
	
	while (*word == ' ')				/* Skip spaces */
	{
		word++;
		spaces++;
	}
	
	if (*word != '!' && *word != '?' && *word != '#')
		return (0);				/* No expression */
	
	/* Zip through string until we hit the ':' or end */
	for (i = 0; word[i] != ':' && word[i]; i++);
	
	if (word[i] == ':')
		word[i] = 0;				/* Set end of string */
	else						/* No ':' found */
	{
		sprintf(buf0, "SPARSER: %c found without a matching ':'", word[i]);
		log (buf0);
		return (0);
	}
	if (*word == '!')
		type = 1;
	else if (*word == '?')
		type = 2;
	else
		type = 3;
	
	word++;						/* Skip over first character */
	strcpy (expression_str, word);
	*offset = spaces + i + 1;
	return (type);
}

char get_numbers(char *expression_str, int *bracket_i, int *word_picked)
{
	char number[3], new_str[20], *exp_str;
	int i;
	char buf0[128];
	
	i = 0;
	exp_str = new_str;
	while (*expression_str)				/* Eat spaces, put in exp_str */
	{
		if (*expression_str != ' ')
		{
			new_str[i] = *expression_str;
			i++;
		}
		expression_str++;
	}
	new_str[i] = 0;
	i = 0;
	while (*exp_str && isdigit (*exp_str) && i < 3)	/* Get first number */
	{
		number[i] = *exp_str;
		i++;
		exp_str++;
	}
	if (*exp_str == 0)				/* If we're at  end of string already - error */
	{
		log ("SPARSER: # Statement doesn't have 2 numbers.");
		return (0);
	}
	if (i == 3 && isdigit (*exp_str))		/* If number is bigger than 3 digits - error */
	{
		log ("SPARSER: First number too large in # statement.");
		return (0);
	}
	if (*exp_str != ',')				/* If numbers not separated by comma - error */
	{
		sprintf(buf0, "SPARSER: Illegal character '%c' in # statement.", *exp_str);
		log (buf0);
		return (0);
	}
	if (i == 0)
	{
		log ("SPARSER: Missing first number in # statement.");
		return (0);
	}
	/* Otherwise, convert to a number */
	number[i] = 0;
	calculate_value (number, bracket_i);
	exp_str++;					/* Skip by comma */
	i = 0;
	while (*exp_str && isdigit (*exp_str) && i < 3)	/* Get second number */
	{
		number[i] = *exp_str;
		i++;
		exp_str++;
	}
	if (*exp_str == 0 && i == 0)			/* If we're at  end of string  - error */
	{
		log ("SPARSER: Second number missing in # statement.");
		return (0);
	}
	if (i == 3 && isdigit (*exp_str))		/* If number is bigger than 3 digits - error */
	{
		log ("SPARSER: Second number too large in # statement.");
		return (0);
	}
	if (*exp_str != 0)				/* Illegal character */
	{
		sprintf(buf0, "SPARSER: Illegal character '%c' in # statement.", *exp_str);
		log (buf0);
		return (0);
	}
	/*Otherwise, convert second number */
	number[i] = 0;
	calculate_value (number, word_picked);
	return (1);
}


/* Lexical Analyser:  Takes in an input string, converts to tokens, passes
   tokens onto
   syntax analyser, then calculates and returns answer */
int lexical_analyser(char *in_string, CHAR_DATA *ch)
{
	TOKEN output[257];
	ROOM_DATA *room = NULL;
	OBJ_DATA *obj = NULL;
	char *c, variable[25], token_found, error_found;
	int  output_i, variable_i, state;
	int value, answer, prev_token;
	int state_to_token_type[22] =
		{ 0, 7, 7, 18, 12, 11, 0, 0, 0, 16, 14,
		  13, 15, 5, 6, 17, 1, 3, 4, 8, 9, 2 };
	
	state = 0;
	variable_i = 0;
	output_i = 0;
	token_found = NO;
	error_found = NO;
	prev_token = 0;
	c = in_string;
	
	while (*c != 0 && error_found == NO)
	{
		if (*c != ' ')			/* Skip spaces */
		{
			switch (state)
			{
			case 0:
				if (isalpha (*c))
				{
					variable[variable_i] = *c;
					variable_i++;
					variable[variable_i] = 0;
					state = 1;
				}
				else if (isdigit (*c))
				{
					variable[variable_i] = *c;
					variable_i++;
					variable[variable_i] = 0;
					state = 2;
				}
				else
				{
					switch (*c)
					{
					case '!':
						state = 3;
						break;
					case '<':
						state = 4;
						break;
					case '>':
						state = 5;
						break;
					case '=':
						state = 6;
						break;
					case '&':
						state = 7;
						break;
					case '|':
						state = 8;
						break;
					case '-':
						state = 15;
						break;
					case '+':
						state = 16;
						break;
					case '*':
						state = 17;
						break;
					case '/':
						state = 18;
						break;
					case '(':
						state = 19;
						break;
					case ')':
						state = 20;
						break;
					default:
						error_found = YES;
					}
				}
				break;		/* end of case 0 */
			case 1:
				if (isalnum (*c))
				{
					variable[variable_i] = *c;
					variable_i++;
					variable[variable_i] = 0;
				}
				else
				{
					variable_i = 0;
					token_found = YES;
				}
				break;
			case 2:
				if (isdigit (*c))
				{
					variable[variable_i] = *c;
					variable_i++;
					variable[variable_i] = 0;
				}
				else
				{
					variable_i = 0;
					token_found = YES;
				}
				break;
			case 3:
			case 4:
			case 5:
				if (*c == '=')
					state += 6;
				else
					token_found = YES;
				break;
			case 6:
				if (*c == '=')
					state = 12;
				else
					error_found = YES;
				break;
			case 7:
				if (*c == '&')
					state = 13;
				else
					error_found = YES;
				break;
			case 8:
				if (*c == '|')
					state = 14;
				else
					error_found = YES;
				break;
			}  		/* end of switch (state) */
		}			/* end of skip if *c == ' ' */

		if (state > 8 || token_found || *(c + 1) == 0)
		{
			value = 0;
			/* IF '-' is the token, make it a binary minus if previous token
			* was ')' or an id/constant */
			if ((state == 15) && ((prev_token == 9) || (prev_token == 7)))
				state = 21;
			if (state == 1)
				error_found = lookup_value(variable, &value, ch, room, obj);
			if (state == 2)
				error_found = calculate_value (variable, &value);
			if (state_to_token_type[state] == 0)
				error_found = YES;
			else
			{
				output[output_i].type = state_to_token_type[state];
				output[output_i].value = value;
				prev_token = output[output_i].type;
				output_i++;
				state = 0;
			}
		}
		if (!token_found)
			c++;
		token_found = NO;
  }	/*end of while */

  if (error_found)
  {
	  /* Log error here */
	  log ("SPARSER: Error found - lexical parse aborted.");
	  answer = 0;
  }
  else
  {
	  /* add end of stream token */
	  output[output_i].type = END_OF_INPUT;
	  output[TOP].type = 0;
	  if (output_i == 1)		/* If just 1 token */
		  answer = output[0].value;
	  else
		  answer = syntax_analyser (output);
  }
  return (answer);
}

/*************************************************************************
*                                                                        *
*  LOOKUP_VALUE:  Compares the string 'variable' passed to it with all   *
*  the strings in 'var_list', if it finds a match, it uses a co-respond- *
*  ing Macro to change the int '*value' passed to it; otherwise it sets  *
*  '*value' to zero.  Returns 1 if no value set, or 0 if all is ok.      *
*                                                                        *
*************************************************************************/
#define NUMBER_OF_VARS  13

char lookup_value(char *variable, int *value, CHAR_DATA *ch, ROOM_DATA *room, OBJ_DATA *obj)
{
	char *var_list[NUMBER_OF_VARS] =
	{
		"str"     , "int"     , "dex"   , "wis"   , /* 0-3 */ 
		"hour"    , "cha"     , "level" , "align" , /* 4-7 */
		"iscleric", "isthief" , "con"   , "height", /* 8-11 */
		"weight"																		/* 12 */
	};
	int i;
	char buf0[128];
	
	extern TIME_INFO_DATA time_info;
	
	for (i = 0; i < NUMBER_OF_VARS; i++)
	{
		/* If match found, break out of the loop */
		if (!str_cmp(variable, var_list[i]))
			break;
	}
	
	/* Place Macros here to calculate value of variable, 0 is the first
	* variable, etc.  Note, may have to pass ch to this function to find
	* variables relating to character - such as str, dex, int, etc.     */
	switch (i)
	{
	case 0:
		*value = GET_STR (ch);
		break;
	case 1:
		*value = GET_INT (ch);
		break;
	case 2:
		*value = GET_DEX (ch);
		break;
	case 3:
		*value = GET_WIS (ch);
		break;
	case 4:
		*value = time_info.hours;
		break;
	case 5:
		*value = GET_CHA (ch);
		break;
	case 6:
		*value = GET_LEVEL (ch);
		break;
	case 7:
		*value = GET_ALIGNMENT(ch);
		break;
	case 8:
		*value = (GET_CLASS(ch) == CLASS_CLERIC);
		break;
	case 9:
		*value = (GET_CLASS(ch) == CLASS_THIEF);
		break;
	case 10:
		*value = GET_CON(ch);
		break;
	case 11:
		*value = GET_HEIGHT(ch);
		break;
	case 12:
		*value = GET_WEIGHT(ch);
		break;
	default:
		*value = 0;
		/* Log error or whatever here */
		sprintf(buf0, "SPARSER: Couldn't find variable - '%s'", variable);
		log (buf0);
		return (YES);
	}
	return (NO);
}

/*************************************************************************
*                                                                        *
*  CALCULATE_VALUE:  Converts the string 'variable' passed to it into a  *
*  number which it stores in '*variable' - returns YES if the number     *
*  exceeded 32767, and sets the value to 32767, otherwise, it returns NO *
*                                                                        *
*************************************************************************/
char calculate_value(char *variable, int *value)
{
	char was_error;
	int len, i;
	long calc_value;
	
	was_error = NO;
	len = strlen (variable);
	calc_value = 0;

	for (i = len - 1; i >= 0; i--)
	{
		calc_value += ((variable[i] - 48) * power10 (len - i - 1));
		if (calc_value > MAX_ANSWER)
		{
			/* Log error message here */
			log ("SPARSER: Varaible overflow - setting to MAX_ANSWER");
			calc_value = MAX_ANSWER;
			/* was_error = YES ; */
			break;
		}
	}
	*value = calc_value;
	return (was_error);
}

int syntax_analyser(TOKEN * input)
{
	TOKEN stack[257], working[4], answer;
	char f[19] = {0, 11, 11, 13, 13, 5, 3, 16, 0, 16, 0, 9, 9, 9, 9, 7, 7, 14, 14};
	char g[19] = {0, 10, 10, 12, 12, 4, 2, 17, 17, 0, 0, 8, 8, 8, 8, 6, 6, 15, 15};
	
	stack[TOP].type = 0;
	stack[0].type = END_OF_INPUT;
	
	while (input->type != END_OF_INPUT || stack[TOP].type > 1)
	{
		if (f[top_most_terminal (stack)] <= g[input->type])
		{
			parse_push (input, stack);
			input++;
		}
		else
		{
			working[WTOP].type = -1;
			parse_pop(working, stack);
			
			while (f[top_most_terminal (stack)] >= g[working[working[WTOP].type].type])
				parse_pop(working, stack);
			
			if (stack[stack[TOP].type].type == NON_TERMINAL)
				parse_pop(working, stack);
			
			answer.type = NON_TERMINAL;
			answer.value = parser_evaluate_expression (working);
			parse_push(&answer, stack);
		}
	}
	return (answer.value);
}

int top_most_terminal(TOKEN * stack)
{
	int index;
	
	index = stack[TOP].type;
	while (stack[index].type == NON_TERMINAL)
		index--;
	
	return (stack[index].type);
}

long parser_evaluate_expression(TOKEN * working)
{
	long answer = 0;
	
	if (working[WTOP].type == 2)				/* 3 operators */
	{
		if (working[0].type == NON_TERMINAL)		/* It's an E op E */
		{
			switch (working[1].type)
			{
			case 1:
				answer = (long) working[2].value + (long) working[0].value;
				if (answer > MAX_ANSWER)
				{
					log ("SPARSER: Answer would exceed Max, truncating...");
					answer = MAX_ANSWER;
				}
				if (answer < -MAX_ANSWER)
				{
					log ("SPARSER: Answer would be below -Max, truncating...");
					answer = -MAX_ANSWER;
				}
				break;
			case 2:
				answer = (long) working[2].value - (long) working[0].value;
				if (answer > MAX_ANSWER)
				{
					log ("SPARSER: Answer would exceed Max, truncating...");
					answer = MAX_ANSWER;
				}
				if (answer < -MAX_ANSWER)
				{
					log ("SPARSER: Answer would be below -Max, truncating...");
					answer = -MAX_ANSWER;
				}
				break;
			case 3:
				answer = (long) working[2].value * (long) working[0].value;
				if (answer > MAX_ANSWER)
				{
					log ("SPARSER: Answer would exceed Max, truncating...");
					answer = MAX_ANSWER;
				}
				if (answer < -MAX_ANSWER)
				{
					log ("SPARSER: Answer would be below -Max, truncating...");
					answer = -MAX_ANSWER;
				}
				break;
			case 4:
				if (working[0].value == 0)
				{
					log ("SPARSER: Cannot divide by zero, setting answer to 0...");
					answer = 0;
				}
				else
					answer = working[2].value / working[0].value;
				break;
			case 5:
				answer = working[2].value && working[0].value;
				break;
			case 6:
				answer = working[2].value || working[0].value;
				break;
			case 11:
				answer = working[2].value > working[0].value;
				break;
			case 12:
				answer = working[2].value < working[0].value;
				break;
			case 13:
				answer = working[2].value >= working[0].value;
				break;
			case 14:
				answer = working[2].value <= working[0].value;
				break;
			case 15:
				answer = working[2].value == working[0].value;
				break;
			case 16:
				answer = working[2].value != working[0].value;
				break;
			}					/* End switch */
		}
		else						/* It's (E)  */
			answer = working[1].value;
	}
	
	if (working[WTOP].type == 1) 				/* 2 operators */
		if (working[1].type == 17) 			/* Unary Minus */
			answer = -working[0].value;
		else															/* ! sign */
			answer = !working[0].value;
		
	if (working[WTOP].type == 0)				/* 1 operator */
		answer = working[0].value;
		
	return (answer);
}

int power10(int num)
{
	int i;
	int value;
	
	value = 1;
	
	for (i = 0; i < num; i++)
		value *= 10;
	
	return (value);
}


#define RETURN_CHAR '|'

void clean_up (char *in, CHAR_DATA *ch)
{
	char buf[1000];
	int notdone, linelen, bufi, ini, wordlen, whitespacelen;
	int exit, word, letter, endspacelen;
	int width = SCR_WIDTH;
	
	notdone = 1;
	linelen = 0;
	bufi = 0;
	ini = 0;
	word = 0;
	strcpy (buf, in);
	
	while (notdone)
	{
		wordlen = 0;
		whitespacelen = 0;
		endspacelen = 0;
		word++;
		while (buf[bufi] == ' ')			/* snarf initial whitespace */
		{
			in[ini++] = buf[bufi++];
			whitespacelen++;
		}
		exit = 0;
		letter = 0;
		while (buf[bufi] != ' ' && !exit)
		{
			letter++;
			switch (buf[bufi])
			{
			case '\n':
			case '\r':
				if (letter > 1 && word > 1)
				{
					in[ini++] = ' ';
					whitespacelen++;
				}
				endspacelen++;
				bufi++;
				while (buf[bufi] == '\n' || buf[bufi] == '\r')
				{
					bufi++;
					endspacelen++;
				}
				exit = 1;
				break;
			case '\0':
				notdone = 0;
				linelen = 0;
				in[ini] = '\0';
				exit = 1;
				break;
			case RETURN_CHAR:
				if (letter > 1)
					exit = 1;
				else
				{
					in[ini++] = '\n';
					in[ini++] = '\r';
					bufi++;
					word = 0;
					linelen = 0;
					endspacelen = 0;
					exit = 1;
				}
				break;
				
			default:
				in[ini++] = buf[bufi++];
				wordlen++;
			}
		}
		linelen += (wordlen + whitespacelen);
		if (linelen >= width)
		{
			bufi -= (wordlen + endspacelen);
			ini -= (wordlen + whitespacelen);
			
			in[ini++] = '\n';
			in[ini++] = '\r';
			linelen = 0;
			word = 0;
		}
	}				/* end outer while */
}
