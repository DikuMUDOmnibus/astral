 /****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *                        Alias module header information                   *
 ****************************************************************************/
/******************************************************
            Desolation of the Dragon MUD II
      (C) 1997, 1998  Jesse DeFer and Heath Leach
 http://dotd.mudservices.com  dotd@dotd.mudservices.com 
 ******************************************************/

typedef struct	alias_type		ALIAS_DATA;

struct	alias_type
{
    ALIAS_DATA *	next;
    ALIAS_DATA *	prev;
    char *		name;
    char * 		cmd;
};

DECLARE_DO_FUN( do_alias	);

void	free_aliases		args( ( CHAR_DATA *ch ) );
ALIAS_DATA *find_alias		args( ( CHAR_DATA *ch, char *argument ) );
bool	check_alias			args( ( CHAR_DATA *ch, char *command, char *argument ) );
