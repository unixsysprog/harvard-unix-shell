/* controlflow.c
 *
 * "if" processing is done with two state variables
 *    if_state and if_result
 */
#include	<stdio.h>
#include        "controlflow.h"
#include	"smsh.h"
#include	"string.h"

enum states   { NEUTRAL, WANT_THEN, THEN_BLOCK, ELSE_BLOCK };
enum results  { SUCCESS, FAIL };
enum keywords { NONE, K_IF, K_THEN, K_ELSE, K_FI, };
struct kw { char *str; int tok; };

struct if_struct *if_struct;
struct if_struct *new_if_struct(struct if_struct *);
struct if_struct *prev_if_struct(struct if_struct *);
int ok_per_if_struct(struct if_struct *);
	
struct kw words[] = { {"if", K_IF}, {"then", K_THEN}, {"else", K_ELSE},
			 {"fi", K_FI}, {NULL,0} };
int is_a_control_word(char *s);

static int if_state  = NEUTRAL;
static int if_result = SUCCESS;
static int last_stat = 0;

int	syn_err(char *);

int ok_to_execute()
/*
 * purpose: determine the shell should execute a command
 * returns: 1 for yes, 0 for no
 * details: if in THEN_BLOCK and if_result was SUCCESS then yes
 *          if in THEN_BLOCK and if_result was FAIL    then no
 *          if in WANT_THEN  then syntax error (sh is different)
 */
{
	int	rv = 1;		/* default is positive */

	if ( if_state == WANT_THEN ){
		rv = 0;
	} else if (if_struct != (struct if_struct *) NULL) {
		rv = ok_per_if_struct(if_struct);
	}
	return rv;
}

int ok_per_if_struct(struct if_struct *if_struct)
{
	int ok = 1;
	while (if_struct != (struct if_struct *) NULL) {
		ok = ok && (if_struct->if_result == SUCCESS ? if_struct->block == THEN_BLOCK : if_struct->block == ELSE_BLOCK);
		if_struct = if_struct->prev;
	}
	return ok;
}

int is_control_command(char *s)
/*
 * purpose: boolean to report if the command is a shell control command
 * returns: 0 or 1
 */
{
    return ( is_a_control_word(s) != NONE );
}

int
is_a_control_word(char *s)
{
	int	i;

	for(i=0; words[i].str != NULL ; i++ )
	{
		if ( strcmp(s, words[i].str) == 0 )
			return words[i].tok;
	}
	return NONE;
}

struct if_struct *new_if_struct(struct if_struct *curr_if_struct)
{
	struct if_struct *new_if_struct = malloc(sizeof(struct if_struct));
	new_if_struct->prev = curr_if_struct;
	return new_if_struct;
}

struct if_struct *prev_if_struct(struct if_struct *curr_if_struct)
{
	struct if_struct *temp = curr_if_struct;
	curr_if_struct = curr_if_struct->prev;
	free(temp);
	return curr_if_struct;
}

int do_control_command(char **args)
/*
 * purpose: Process "if", "then", "fi" - change state or detect error
 * returns: 0 if ok, -1 for syntax error
 *   notes: I would have put returns all over the place, Barry says "no"
 */
{
	char	*cmd = args[0];
	int	rv = -1;

	if( strcmp(cmd,"if")==0 ){
		if ( if_state == WANT_THEN )
			rv = syn_err("if unexpected");
		else {
			last_stat = process(args+1);
			if_struct = new_if_struct(if_struct);
			if_struct->if_result = (last_stat == 0 ? SUCCESS : FAIL );
			if_state = WANT_THEN;
			rv = 0;
		}
	}
	else if ( strcmp(cmd,"then")==0 ){
		if ( if_state != WANT_THEN )
			rv = syn_err("then unexpected");
		else {
			if_state = if_struct->block = THEN_BLOCK;
			rv = 0;
		}
	}
	else if ( strcmp(cmd,"fi")==0 ){
		if ( (if_state != THEN_BLOCK) && (if_state != ELSE_BLOCK) )
			rv = syn_err("fi unexpected");
		else {
			if_struct = prev_if_struct(if_struct);
			if (if_struct == (struct if_struct *) NULL) {
				if_state = NEUTRAL;
				rv = 0;
			} else {
				if_state = if_struct->block;
				rv = 0;
			}
		}
	}
	else if ( strcmp(cmd,"else") == 0 ){
		if ( if_state != THEN_BLOCK )
			rv = syn_err("else unexpected");
		else {
			if_state = if_struct->block = ELSE_BLOCK;
			rv = 0;
		}
	}
	else 
		fatal("internal error processing:", cmd, 2);
	return rv;
}

int syn_err(char *msg)
/* purpose: handles syntax errors in control structures
 * details: resets state to NEUTRAL
 * returns: -1 in interactive mode. Should call fatal in scripts
 */
{
	if_state = NEUTRAL;
	fprintf(stderr,"syntax error: %s\n", msg);
	return -1;
}
