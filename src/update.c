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
 *			      Regular update module			    *
 ****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"



/*
 * Local functions.
 */
int	hit_gain	args( ( CHAR_DATA *ch ) );
int	mana_gain	args( ( CHAR_DATA *ch ) );
int	move_gain	args( ( CHAR_DATA *ch ) );
void	mobile_update	args( ( void ) );
void	weather_update	args( ( void ) );
void	time_update	args( ( void ) );	/* FB */
void	char_update	args( ( void ) );
void	obj_update	args( ( void ) );
void	aggr_update	args( ( void ) );
void	room_act_update	args( ( void ) );
void	obj_act_update	args( ( void ) );
void    quest_update    args( ( void ) ); /* Vassago - quest.c */
void	char_check	args( ( void ) );
void    drunk_randoms	args( ( CHAR_DATA *ch ) );
void    hallucinations	args( ( CHAR_DATA *ch ) );
void	subtract_times	args( ( struct timeval *etime,
				struct timeval *stime ) );

/* weather functions - FB */
void	adjust_vectors		args( ( WEATHER_DATA *weather) );
void	get_weather_echo	args( ( WEATHER_DATA *weather) );
void	get_time_echo		args( ( WEATHER_DATA *weather) );

/*
 * Global Variables
 */

CHAR_DATA *	gch_prev;
OBJ_DATA *	gobj_prev;

CHAR_DATA *	timechar;

char * corpse_descs[] =
   { 
     "%s znajduje sie w ostatniej fazie rozkladu.", 
     "%s roi sie od robactwa.",
     "%s roznosi wokolo fetor rozkladu.",
     "%s obsiadly muchy.",
     "%s lezy tutaj."
   };

extern int      top_exit;

/*
 * Advancement stuff.
 */
void advance_level( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    int add_hp;
    int add_mana;
    int add_move;
    int add_prac;

/*	save_char_obj( ch );*/
    if ( !IS_SET( ch->pcdata->flags, PCFLAG_NOTITLE ) )
    {
	sprintf( buf, "the %s",
	title_table [ch->class] [ch->level] [ch->sex == SEX_FEMALE ? 1 : 0] );
	set_title( ch, buf );
    }
/*    if ( ( ch->level > 39 ) && !IS_PKILL( ch ) )
    {
	if ( xIS_SET( ch->act, PLR_NICE ) )
	    xREMOVE_BIT( ch->act, PLR_NICE );
	SET_BIT( ch->pcdata->flags, PCFLAG_DEADLY );
	send_to_char( "Od teraz bedziesz zabijal innych graczy i byl przez zabijany!\n\r", ch );
    }*/
		    
    
    add_hp	= con_app[get_curr_con(ch)].hitp + number_range(
		    class_table[ch->class]->hp_min,
		    class_table[ch->class]->hp_max );
    add_mana	= class_table[ch->class]->fMana
		    ? number_range(2, (2*get_curr_int(ch)+get_curr_wis(ch))/8)
		    : 0;
    add_mana    = add_mana * class_table[ch->class]->fMana; /* Flash */
    add_move	= number_range( 5, (get_curr_con(ch)+get_curr_dex(ch))/4 );
    add_prac	= wis_app[get_curr_wis(ch)].practice;

    add_hp	= UMAX(  1, add_hp   );
    add_mana	= UMAX(  0, add_mana );
    add_move	= UMAX( 10, add_move );

    /* bonus for deadlies */
    if(IS_PKILL(ch)){
        add_mana = add_mana + add_mana*.3;
        add_move = add_move + add_move*.3;
        add_hp +=1; /* bitch at blod if you don't like this :) */
/*        sprintf(buf,"Gravoc's Pandect steels your sinews.\n\r");
	send_to_char( ch, buf ); */
    }

    ch->max_hit 	+= add_hp;
    ch->max_mana	+= add_mana;
    ch->max_move	+= add_move;
    ch->practice	+= add_prac;

    if ( !IS_NPC(ch) )
	xREMOVE_BIT( ch->act, PLR_BOUGHT_PET );

    if ( ch->level == LEVEL_AVATAR )
    {
	DESCRIPTOR_DATA *d;

	sprintf( buf, "%s zostaje Avatarem!", ch->name );
	for ( d = first_descriptor; d; d = d->next )
	   if ( d->connected == CON_PLAYING && d->character != ch )
	   {
		set_char_color( AT_IMMORT, d->character );
		send_to_char( buf,	d->character );
		send_to_char( "\n\r",	d->character );
	   }
	set_char_color( AT_WHITE, ch );
	do_help( ch, "M_ADVHERO_" );
    }
    if ( ch->level < LEVEL_IMMORTAL )
    {
      if ( IS_VAMPIRE(ch) || IS_DEMON(ch) )
        sprintf( buf,
	  "Otrzymujesz: %d/%d hp, %d/%d bp, %d/%d mv %d/%d prac.\n\r",
	  add_hp,	ch->max_hit,
	  1,	        ch->level + 10,
	  add_move,	ch->max_move,
	  add_prac,	ch->practice
	  );
      else
        sprintf( buf,
	  "Otrzymujesz: %d/%d hp, %d/%d mana, %d/%d mv %d/%d prac.\n\r",
	  add_hp,	ch->max_hit,
 	  add_mana,	ch->max_mana,
	  add_move,	ch->max_move,
	  add_prac,	ch->practice
	  );

      if ( IS_VAMPIRE(ch) || IS_DEMON(ch) )
        sprintf( buf1,
	  "%s zdobywa poziom %d oraz: %d/%d hp, %d/%d bp, %d/%d mv %d/%d prac.\n\r",
	  ch->name,	ch->level,
	  add_hp,	ch->max_hit,
	  1,	        ch->level + 10,
	  add_move,	ch->max_move,
	  add_prac,	ch->practice
	  );
      else
        sprintf( buf1,
	  "%s zdobywa poziom %d oraz: %d/%d hp, %d/%d mana, %d/%d mv %d/%d prac.\n\r",
	  ch->name,	ch->level,
	  add_hp,	ch->max_hit,
 	  add_mana,	ch->max_mana,
	  add_move,	ch->max_move,
	  add_prac,	ch->practice
	  );
      ch->questlevel = 0; /* Flash questy na level */
      if ( ch->level > 15 )
          ch->questpoints -= ch->level * 6;
      set_char_color( AT_WHITE, ch );
      send_to_char( buf, ch );
      log_string ( buf1 );
    }
    return;
}   



void gain_exp( CHAR_DATA *ch, int gain )
{
    int modgain;
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC(ch) || ch->level >= LEVEL_AVATAR )
	return;


    /* Bonus for deadly lowbies */
    modgain = gain;
    if(modgain>0 && IS_PKILL(ch) && ch->level<17){
       if(ch->level<=6){
          sprintf(buf,"Przychylnosc Gravoc`a przyspiesza Twa nauke.\n\r");
          modgain*=1.75;
       }
       if(ch->level<=10 && ch->level>=7){
          sprintf(buf,"Dlonie Gravoc`a przyspieszaja Twa nauke.\n\r");
          modgain*=1.50;
       }
       if(ch->level<=13 && ch->level>=11){
          sprintf(buf,"Spryt Gravoc`a wspomaga Twa nauke.\n\r");
          modgain*=1.25;
       }
       if(ch->level<=16 && ch->level>=14){
          sprintf(buf,"Sila Gravoc`a przyspiesza Twa nauke.\n\r");
          modgain*=1.15;
       }
	send_to_char(buf, ch);
    }

    /* per-race experience multipliers */
    modgain *= ( race_table[ch->race]->exp_multiplier/100.0);

    /* Deadly exp loss floor is exp floor of level */
    if(IS_PKILL(ch)&& modgain<0){
       if( ch->exp + modgain < exp_level(ch, ch->level)){
          modgain = exp_level(ch, ch->level) - ch->exp;
          sprintf(buf,"Gravoc's Pandect protects your insight.\n\r");
       }
    }


    /* xp cap to prevent any one event from giving enuf xp to */
    /* gain more than one level - FB */
    modgain = UMIN(modgain,
    	exp_level(ch, ch->level+2) - exp_level(ch, ch->level+1));
    if ( ch->class != 0 && ch->class != 5 )
        modgain = (modgain * 3) / 4;
    ch->exp = UMAX( 0, ch->exp + modgain );
/*  Flash */
/*    if (ch->level == 1 && ch->exp >= exp_level(ch, ch->level+1))
    {
	ch->exp = (exp_level(ch, (ch->level+1)) - 1);
	return;
    } */

/* Flash koniec */

    if (NEW_AUTH(ch) && ch->exp >= exp_level(ch, ch->level+1))
    {
/*	send_to_char("You can not ascend to a higher level until you are authorized.\n\r", ch); */
	ch->exp = (exp_level(ch, (ch->level+1)) - 1);
	return;
    }
    if (NOT_AUTHED(ch) && ch->exp >= exp_level(ch, 5))
    {
	send_to_char("Nie mozesz zdobyc 5 poziomo poki Twe imie nie zostanie zautoryzowane.\n\r", ch);
	ch->exp = (exp_level(ch, 5) - 1 );
	return;
    }
/*    if (NOT_AUTHED(ch) && ch->exp >= exp_level(ch, ch->level+1))
    {
	send_to_char("Nie mozesz zdobyc wyzszego poziomu, zanim nie otrzymaasz autoryzacji.\n\r", ch);
	ch->exp = (exp_level(ch, (ch->level+1)) - 1);
	return;
    }
Flash */

/* Qp na level - Flash 28.4.1999 */
    if ( ch->level > 14 && ch->exp >= exp_level(ch, ch->level+1) 
	 && ch->questlevel < (( ch->level + 1 ) * 6 ) )
	{
	ch->gold += (ch->exp - exp_level( ch, ch->level+1))/3;
	ch->exp = exp_level(ch, ch->level+1) - 1;
	}
	
    
    while ( ch->level < LEVEL_AVATAR && ch->exp >= exp_level(ch, ch->level+1) )
    {
	set_char_color( AT_WHITE + AT_BLINK, ch );
	ch_printf( ch, "Masz juz odpowiednia ilosc doswiadczenia na poziom %d!\n\r", ++ch->level );
	advance_level( ch );
    }

    return;
}



/*
 * Regeneration stuff.
 */
int hit_gain( CHAR_DATA *ch )
{
    int gain;

    if ( IS_NPC(ch) )
    {
	gain = ch->level * 3 / 2;
    }
    else
    {
	gain = UMIN( 5, ch->level );

	switch ( ch->position )
	{
	case POS_DEAD:	   return 0;
	case POS_MORTAL:   return -1;
	case POS_INCAP:    return -1;
	case POS_STUNNED:  return 1;
	case POS_SLEEPING: gain += get_curr_con(ch) * 2.0;	break;
	case POS_RESTING:  gain += get_curr_con(ch) * 1.25;	break;
	}

        if ( IS_VAMPIRE(ch) || IS_DEMON(ch) ) {
            if ( ch->pcdata->condition[COND_BLOODTHIRST] <= 1 )
		gain /= 2;
	    else
	    if ( ch->pcdata->condition[COND_BLOODTHIRST] >= (8 + ch->level) )
                gain *= 2;
            if ( IS_OUTSIDE( ch ) )
  	    {
    	       switch(time_info.sunlight)
    	       {
    	          case SUN_RISE:
    		  case SUN_SET:
      	            gain /= 2;
      	            break;
    		  case SUN_LIGHT:
      		    gain /= 4;
      		    break;
      	       }
      	    }
        }

	if ( ch->pcdata->condition[COND_FULL]   == 0 )
	    gain /= 2;

	if ( ch->pcdata->condition[COND_THIRST] == 0 )
	    gain /= 2;

    }

    if ( IS_AFFECTED(ch, AFF_POISON) )
	gain /= 4;

    return UMIN(gain, ch->max_hit - ch->hit);
}



int mana_gain( CHAR_DATA *ch )
{
    int gain;

    if ( IS_NPC(ch) )
    {
	gain = ch->level;
    }
    else
    {
	gain = UMIN( 5, ch->level / 2 );

	if ( ch->position < POS_SLEEPING )
	  return 0;
	switch ( ch->position )
	{
	case POS_SLEEPING: gain += get_curr_int(ch) * 3.25;	break;
	case POS_RESTING:  gain += get_curr_int(ch) * 1.75;	break;
	}

	if ( ch->pcdata->condition[COND_FULL]   == 0 )
	    gain /= 2;

	if ( ch->pcdata->condition[COND_THIRST] == 0 )
	    gain /= 2;

    }

    if ( IS_AFFECTED(ch, AFF_POISON) )
	gain /= 4;

    return UMIN(gain, ch->max_mana - ch->mana);
}



int move_gain( CHAR_DATA *ch )
{
    int gain;

    if ( IS_NPC(ch) )
    {
	gain = ch->level;
    }
    else
    {
	gain = UMAX( 15, 2 * ch->level );

	switch ( ch->position )
	{
	case POS_DEAD:	   return 0;
	case POS_MORTAL:   return -1;
	case POS_INCAP:    return -1;
	case POS_STUNNED:  return 1;
	case POS_SLEEPING: gain += get_curr_dex(ch) * 4.5;	break;
	case POS_RESTING:  gain += get_curr_dex(ch) * 2.5;	break;
	}

        if ( IS_VAMPIRE(ch) || IS_DEMON(ch) ) {
            if ( ch->pcdata->condition[COND_BLOODTHIRST] <= 1 )
		gain /= 2;
	    else
	    if ( ch->pcdata->condition[COND_BLOODTHIRST] >= (8 + ch->level) )
                gain *= 2;
            if ( IS_OUTSIDE( ch ) )
  	    {
    	       switch(time_info.sunlight)
    	       {
    	          case SUN_RISE:
    		  case SUN_SET:
      	            gain /= 2;
      	            break;
    		  case SUN_LIGHT:
      		    gain /= 4;
      		    break;
      	       }
      	    }
        }

	if ( ch->pcdata->condition[COND_FULL]   == 0 )
	    gain /= 2;

	if ( ch->pcdata->condition[COND_THIRST] == 0 )
	    gain /= 2;
    }

    if ( IS_AFFECTED(ch, AFF_POISON) )
	gain /= 4;

    return UMIN(gain, ch->max_move - ch->move);
}


void gain_condition( CHAR_DATA *ch, int iCond, int value )
{
    int condition;
    ch_ret retcode = rNONE;

    if ( value == 0 || IS_NPC(ch) || ch->level >= LEVEL_IMMORTAL || NEW_AUTH( ch ) ) /* new auth */ 
    	return;
/*    if ( value == 0 || IS_NPC(ch) || ch->level >= LEVEL_IMMORTAL || NOT_AUTHED(ch))
	return;
Flash */
    condition				= ch->pcdata->condition[iCond];
    if ( iCond == COND_BLOODTHIRST )
	ch->pcdata->condition[iCond]	= URANGE( 0, condition + value,
                                                10 + ch->level );
    else
	ch->pcdata->condition[iCond]	= URANGE( 0, condition + value, 48 );

    if ( ch->pcdata->condition[iCond] == 0 )
    {
	switch ( iCond )
	{
	case COND_FULL:
          if (( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_VAMPIRE) && ( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_DEMON ))
          {
            set_char_color( AT_HUNGRY, ch );
	    send_to_char( "Umierasz z GLODU!\n\r",  ch );
            act( AT_HUNGRY, "$n umiera z glodu!", ch, NULL, NULL, TO_ROOM);
	    if ( !IS_PKILL(ch) || number_bits(1) == 0 )
		worsen_mental_state( ch, 1 );
	    retcode = damage(ch, ch, 1, TYPE_UNDEFINED);
          }
          break;

	case COND_THIRST:
          if (( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_VAMPIRE) && ( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_DEMON))
          {
            set_char_color( AT_THIRSTY, ch );
	    send_to_char( "Umierasz z PRAGNIENIA!\n\r", ch );
            act( AT_THIRSTY, "$n umiera z pragnienia!", ch, NULL, NULL, TO_ROOM);
	    worsen_mental_state( ch, IS_PKILL(ch) ? 1: 2 );
	    retcode = damage(ch, ch, 2, TYPE_UNDEFINED);
          }
          break;

        case COND_BLOODTHIRST:
          if ( ch->level < LEVEL_IMMORTAL )
          {
            set_char_color( AT_BLOOD, ch );
            send_to_char( "Zaczynasz odczuwac skutki braku krwi!\n\r", ch );
            act( AT_BLOOD, "$n slabnie z braku krwi!", ch,
                 NULL, NULL, TO_ROOM);
	    worsen_mental_state( ch, 2 );
	    retcode = damage(ch, ch, ch->max_hit / 20, TYPE_UNDEFINED);
          }
          break;
	case COND_DRUNK:
	    if ( condition != 0 ) {
                set_char_color( AT_SOBER, ch );
		if (ch->sex == 0)
		send_to_char( "Jestes pijane.\n\r", ch );
		else if (ch->sex == 1)
		send_to_char( "Jestes pijany.\n\r", ch );
		else
		send_to_char( "Jestes pijana.\n\r", ch );
	    }
	    retcode = rNONE;
	    break;
	default:
	    bug( "Gain_condition: invalid condition type %d", iCond );
	    retcode = rNONE;
	    break;
	}
    }

    if ( retcode != rNONE )
	return;

    if ( ch->pcdata->condition[iCond] == 1 )
    {
	switch ( iCond )
	{
	case COND_FULL:
          if (( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_VAMPIRE) && ( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_DEMON))
          {
            set_char_color( AT_HUNGRY, ch );
	    send_to_char( "Naprawde chce Ci sie jesc.\n\r",  ch );
            act( AT_HUNGRY, "Slyszysz jak $x burczy w brzuchu z glodu.", ch, NULL, NULL, TO_ROOM);
	    if ( number_bits(1) == 0 )
		worsen_mental_state( ch, 1 );
          } 
	  break;

	case COND_THIRST:
          if (( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_VAMPIRE) && ( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_DEMON))
          {
            set_char_color( AT_THIRSTY, ch );
	    send_to_char( "Naprawde chce Ci sie pic.\n\r", ch );
	    worsen_mental_state( ch, 1 );
	    if ( ch->sex == SEX_MALE )
	    act( AT_THIRSTY, "$n wyglada na bardzo spragnionego.", ch, NULL, NULL, TO_ROOM);
            else if ( ch->sex == SEX_FEMALE )
	    act( AT_THIRSTY, "$n wyglada na bardzo spragniona.", ch, NULL, NULL, TO_ROOM);
            else
	    act( AT_THIRSTY, "$n wyglada na bardzo spragnione.", ch, NULL, NULL, TO_ROOM);          
	  } 
	  break;

        case COND_BLOODTHIRST:
          if ( ch->level < LEVEL_IMMORTAL )
          {
            set_char_color( AT_BLOOD, ch );
            send_to_char( "Odzczuwasz silna potrzebe wypicia swiezej krwi!\n\r", ch );
            act( AT_BLOOD, "$n wyglada bardzo slabo...", ch,
                 NULL, NULL, TO_ROOM);
	    worsen_mental_state( ch, 1 );
          }
          break;
	case COND_DRUNK:
	    if ( condition != 0 ) {
                set_char_color( AT_SOBER, ch );
		send_to_char( "Czujesz jak opuszcza Cie jasnosc umyslu.\n\r", ch );
            }
	    break;
	}
    }


    if ( ch->pcdata->condition[iCond] == 2 )
    {
	switch ( iCond )
	{
	case COND_FULL:
          if (( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_VAMPIRE) && ( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_DEMON))
          {
            set_char_color( AT_HUNGRY, ch );
	    if (ch->sex == 0)
	    send_to_char( "Jestes glodne.\n\r",  ch );
	    else if (ch->sex == 1)
	    send_to_char( "Jestes glodny.\n\r",  ch );
	    else
	    send_to_char( "Jestes glodna.\n\r",  ch );
          } 
	  break;

	case COND_THIRST:
          if (( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_VAMPIRE) && ( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_DEMON))
          {
            set_char_color( AT_THIRSTY, ch );
	    if (ch->sex == 0)
	    send_to_char( "Jestes spragnione.\n\r", ch );
	    else if (ch->sex == 1)
	    send_to_char( "Jestes spragniony.\n\r", ch );
	    else
	    send_to_char( "Jestes spragniona.\n\r", ch );
          } 
	  break;

        case COND_BLOODTHIRST:
          if ( ch->level < LEVEL_IMMORTAL )
          {
            set_char_color( AT_BLOOD, ch );
            send_to_char( "Odczuwasz silna potrzebe zatopienia klow w czyims gardle.\n\r", ch );
          }  
          break;
	}
    }

    if ( ch->pcdata->condition[iCond] == 3 )
    {
	switch ( iCond )
	{
	case COND_FULL:
          if (( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_VAMPIRE) && ( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_DEMON))
          {
            set_char_color( AT_HUNGRY, ch );
	    send_to_char( "Slabniesz z glodu.\n\r",  ch );
          } 
	  break;

	case COND_THIRST:
          if (( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_VAMPIRE) && ( ch->level < LEVEL_IMMORTAL && ch->class != CLASS_DEMON))
	  {
            set_char_color( AT_THIRSTY, ch );
	    send_to_char( "Przydalo by sie napic czegos orzezwiajacego.\n\r", ch );
          } 
	  break;

        case COND_BLOODTHIRST:
          if ( ch->level < LEVEL_IMMORTAL )
          {
            set_char_color( AT_BLOOD, ch );
            send_to_char( "Czujesz mrowienie w swych klach.\n\r", ch );
          }
          break;
	}
    }

     /*
      *  Race alignment restrictions, h
      */
     if(ch->alignment<race_table[ch->race]->minalign)
     {
	set_char_color( AT_BLOOD, ch );
        send_to_char( "Twoje zachowanie nie jest odpowiednie dla twojej rasy.  Masz klopoty.", ch);
     }

     if(ch->alignment>race_table[ch->race]->maxalign)
     {
	set_char_color( AT_BLOOD, ch );
        send_to_char( "Twoje zachowanie nie jest odpowiednie dla twojej rasy.  Masz klopoty.", ch);
     }

     /* Demon musi byc zly :) -h  */
      if((ch->class == 9) && (ch->level > 5)){
          if(ch->alignment>-500){
            set_char_color( AT_BLOOD, ch );
            send_to_char( "Jestes DEMONEM! Czyn zlo lub poniesiesz konsekwencje!\n\r", ch );
            act( AT_BLOOD, "$n potrzebuje krwi by zmazac swa hanbe.", ch, 
                 NULL, NULL, TO_ROOM); 
            worsen_mental_state( ch, 15 );
            return; 
        } 
     }


     /* Paladins need some restrictions, this is where we crunch 'em -h */
      if((ch->class == 11) && (ch->level > 4)){
        if(ch->alignment<0){
            set_char_color( AT_BLOOD, ch );
            send_to_char( "Czujesz sie zbrukany i masz wyrzuty sumienia!\n\r", ch );
            act( AT_BLOOD, "$n pada krzyzem na ziemie pod ciezarem swych grzechow.", ch, 
                 NULL, NULL, TO_ROOM); 
            worsen_mental_state( ch, 15 );
            return;
        } 
        if(ch->alignment<350){
            set_char_color( AT_BLOOD, ch );
            send_to_char( "Zdradzasz swa wiare, wiec Twoj umysl zaczyna zradzac Ciebie.\n\r", ch );
            act( AT_BLOOD, "$n zastanawia sie nad swymi czynami.", ch, 
                 NULL, NULL, TO_ROOM); 
            worsen_mental_state( ch, 6 );
            return;
       }
    } 

    return;
}



/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Mud cpu time.
 */
void mobile_update( void )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *ch;
    EXIT_DATA *pexit;
    int door;
    ch_ret     retcode;

    retcode = rNONE;

    /* Examine all mobs. */
    for ( ch = last_char; ch; ch = gch_prev )
    {
	set_cur_char( ch );
	if ( ch == first_char && ch->prev )
	{
	    bug( "mobile_update: first_char->prev != NULL... fixed", 0 );
	    ch->prev = NULL;
	}
	  
	gch_prev = ch->prev;
	
	if ( gch_prev && gch_prev->next != ch )
	{
	    sprintf( buf, "FATAL: Mobile_update: %s->prev->next doesn't point to ch.",
		ch->name );
	    bug( buf, 0 );	    
	    bug( "Short-cutting here", 0 );
	    gch_prev = NULL;
	    ch->prev = NULL;
	    do_shout( ch, "Thoric mowi, 'Przygotujcie na najgorsze!'" );
	}

	if ( !IS_NPC(ch) )
	{
	    drunk_randoms(ch);
	    hallucinations(ch);
	    continue;
	}

	if ( !ch->in_room
	||   IS_AFFECTED(ch, AFF_CHARM)
	||   IS_AFFECTED(ch, AFF_PARALYSIS) )
	    continue;

/* Clean up 'animated corpses' that are not charmed' - Scryn */

        if ( ch->pIndexData->vnum == 5 && !IS_AFFECTED(ch, AFF_CHARM) )
	{
	  if(ch->in_room->first_person)
	    act(AT_MAGIC, "$n zmienia sie w proch, z ktorego powstalo.", ch, NULL, NULL, TO_ROOM);
          
   	  if(IS_NPC(ch)) /* Guard against purging switched? */
	    extract_char(ch, TRUE);
	  continue;
	}

	if ( !xIS_SET( ch->act, ACT_RUNNING )
	&&   !xIS_SET( ch->act, ACT_SENTINEL )
	&&   !ch->fighting && ch->hunting )
	{
	  WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
	  /* Commented out temporarily to avoid spam - Scryn */
/*	  sprintf( buf, "%s hunting %s from %s.", ch->name,
	  	ch->hunting->name,
		ch->in_room->name );
	  log_string( buf );*/
	  hunt_victim( ch );
	  continue;
	}  

	/* Examine call for special procedure */
	if ( !xIS_SET( ch->act, ACT_RUNNING )
	&&    ch->spec_fun )
	{
	    if ( (*ch->spec_fun) ( ch ) )
		continue;
	    if ( char_died(ch) )
		continue;
	}

	/* Check for mudprogram script on mob */
	if ( HAS_PROG( ch->pIndexData, SCRIPT_PROG ) )
	{
	    mprog_script_trigger( ch );
	    continue;
	}

	if ( ch != cur_char )
	{
	    bug( "Mobile_update: ch != cur_char after spec_fun", 0 );
	    continue;
	}

	/* That's all for sleeping / busy monster */
	if ( ch->position != POS_STANDING )
	    continue;

	if ( xIS_SET(ch->act, ACT_MOUNTED ) )
	{
	    if ( xIS_SET(ch->act, ACT_AGGRESSIVE)
	    ||   xIS_SET(ch->act, ACT_META_AGGR) )
		do_emote( ch, "parska i warczy." );
	    continue;
	}

	if ( IS_SET(ch->in_room->room_flags, ROOM_SAFE )
	&&  (xIS_SET(ch->act, ACT_AGGRESSIVE) || xIS_SET(ch->act, ACT_META_AGGR)) )
	    do_emote( ch, "toczy wokolo wscieklym wzrokiem." );


	/* MOBprogram random trigger */
	if ( ch->in_room->area->nplayer > 0 )
	{
	    mprog_random_trigger( ch );
	    if ( char_died(ch) )
		continue;
	    if ( ch->position < POS_STANDING )
	        continue;
	}

        /* MOBprogram hour trigger: do something for an hour */
        mprog_hour_trigger(ch);

	if ( char_died(ch) )
	  continue;

	rprog_hour_trigger(ch);
	if ( char_died(ch) )
	  continue;

	if ( ch->position < POS_STANDING )
	  continue;

	/* Scavenge */
	if ( xIS_SET(ch->act, ACT_SCAVENGER)
	&&   ch->in_room->first_content
	&&   number_bits( 2 ) == 0 )
	{
	    OBJ_DATA *obj;
	    OBJ_DATA *obj_best;
	    int max;

	    max         = 1;
	    obj_best    = NULL;
	    for ( obj = ch->in_room->first_content; obj; obj = obj->next_content )
	    {
		if ( CAN_WEAR(obj, ITEM_TAKE) && obj->cost > max 
		&& !IS_OBJ_STAT( obj, ITEM_BURIED ) )
		{
		    obj_best    = obj;
		    max         = obj->cost;
		}
	    }

	    if ( obj_best )
	    {
		obj_from_room( obj_best );
		obj_to_char( obj_best, ch );
		act( AT_ACTION, "$n podnosi $v.", ch, obj_best, NULL, TO_ROOM );
	    }
	}

	/* Wander */
	if ( !xIS_SET(ch->act, ACT_RUNNING)
	&&   !xIS_SET(ch->act, ACT_SENTINEL)
	&&   !xIS_SET(ch->act, ACT_PROTOTYPE)
	&& ( door = number_bits( 5 ) ) <= 9
	&& ( pexit = get_exit(ch->in_room, door) ) != NULL
	&&   pexit->to_room
	&&   !IS_SET(pexit->exit_info, EX_CLOSED)
	&&   !IS_SET(pexit->to_room->room_flags, ROOM_NO_MOB)
	&&   !IS_SET(pexit->to_room->room_flags, ROOM_DEATH)
	&& ( !xIS_SET(ch->act, ACT_STAY_AREA)
	||   pexit->to_room->area == ch->in_room->area ) )
	{
	    retcode = move_char( ch, pexit, 0 );
						/* If ch changes position due
						to it's or someother mob's
						movement via MOBProgs,
						continue - Kahn */
	    if ( char_died(ch) )
	      continue;
	    if ( retcode != rNONE || xIS_SET(ch->act, ACT_SENTINEL)
	    ||    ch->position < POS_STANDING )
	        continue;
	}

	/* Flee */
	if ( ch->hit < ch->max_hit / 2
	&& ( door = number_bits( 4 ) ) <= 9
	&& ( pexit = get_exit(ch->in_room,door) ) != NULL
	&&   pexit->to_room
	&&   !IS_SET(pexit->exit_info, EX_CLOSED)
	&&   !IS_SET(pexit->to_room->room_flags, ROOM_NO_MOB) )
	{
	    CHAR_DATA *rch;
	    bool found;

	    found = FALSE;
	    for ( rch  = ch->in_room->first_person;
		  rch;
		  rch  = rch->next_in_room )
	    {
		if ( is_fearing(ch, rch) )
		{
		    switch( number_bits(2) )
		    {
			case 0:
			  sprintf( buf, "Odejdz ode mnie, %s!", rch->nwol );
			  break;
			case 1:
			  sprintf( buf, "Zostaw mnie, %s!", rch->nwol );
			  break;
			case 2:
			  sprintf( buf, "%s chce mnie zabic!  Pomocy!", rch->name );
			  break;
			case 3:
			  sprintf( buf, "Niech ktos mnie uratuje przed %s!", rch->nnar );
			  break;
		    }
		    do_yell( ch, buf );
		    found = TRUE;
		    break;
		}
	    }
	    if ( found )
		retcode = move_char( ch, pexit, 0 );
	}
    }

    return;
}



/*
 * Update all chars, including mobs.
 * This function is performance sensitive.
 */
void char_update( void )
{   
    CHAR_DATA *ch;
    CHAR_DATA *ch_save;
    sh_int save_count = 0;

    ch_save	= NULL;
    for ( ch = last_char; ch; ch = gch_prev )
    {
	if ( ch == first_char && ch->prev )
	{
	    bug( "char_update: first_char->prev != NULL... fixed", 0 );
	    ch->prev = NULL;
	}
	gch_prev = ch->prev;
	set_cur_char( ch );
	if ( gch_prev && gch_prev->next != ch )
	{
	    bug( "char_update: ch->prev->next != ch", 0 );
	    return;
	}

        /*
	 *  Do a room_prog rand check right off the bat
	 *   if ch disappears (rprog might wax npc's), continue
	 */
	if(!IS_NPC(ch))
	    rprog_random_trigger( ch );

	if( char_died(ch) )
	    continue;

	if(IS_NPC(ch))
	    mprog_time_trigger(ch);   

	if( char_died(ch) )
	    continue;

	rprog_time_trigger(ch);

	if( char_died(ch) )
	    continue;

	/*
	 * See if player should be auto-saved.
	 */
	if ( !IS_NPC(ch)
	&& ( !ch->desc || ch->desc->connected == CON_PLAYING )
	&&    ch->level >= 2
	&&    current_time - ch->save_time > (sysdata.save_frequency*60) )
	    ch_save	= ch;
	else
	    ch_save	= NULL;

	if ( ch->position >= POS_STUNNED )
	{
	    if ( ch->hit  < ch->max_hit )
		ch->hit  += hit_gain(ch);

	    if ( ch->mana < ch->max_mana )
		ch->mana += mana_gain(ch);

	    if ( ch->move < ch->max_move )
		ch->move += move_gain(ch);
	}

	if ( ch->position == POS_STUNNED )
	    update_pos( ch );

      /*   Morph timer expires */

      	if ( ch->morph ) {
           if ( ch->morph->timer > 0 )
	   {
            	    ch->morph->timer--;
           	    if ( ch->morph->timer == 0 )
            	   	 do_unmorph_char( ch );
	   }
  	}

        /* To make people with a nuisance's flags life difficult 
         * --Shaddai
         */

        if ( !IS_NPC(ch) && ch->pcdata->nuisance )
        {
	    long int temp;
    
	    if ( ch->pcdata->nuisance->flags < MAX_NUISANCE_STAGE )
	    {
	     temp = ch->pcdata->nuisance->max_time-ch->pcdata->nuisance->time;
 	     temp *= ch->pcdata->nuisance->flags;
	     temp /= MAX_NUISANCE_STAGE;
	     temp += ch->pcdata->nuisance->time;
	     if ( temp < current_time )
		    ch->pcdata->nuisance->flags++;
	    }
        }

	if ( !IS_NPC(ch) && ch->level < LEVEL_IMMORTAL )
	{
	    OBJ_DATA *obj;

	    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
	    &&   obj->item_type == ITEM_LIGHT
	    &&   obj->value[2] > 0 )
	    {
		if ( --obj->value[2] == 0 && ch->in_room )
		{
		    ch->in_room->light -= obj->count;
		    if (ch->in_room->light < 0 )
				ch->in_room->light=0;
		    act( AT_ACTION, "$p wygasa.", ch, obj, NULL, TO_ROOM );
		    act( AT_ACTION, "$p wygasa.", ch, obj, NULL, TO_CHAR );
		    if ( obj->serial == cur_obj )
		      global_objcode = rOBJ_EXPIRED;
		    extract_obj( obj );
		}
	    }


	    if ( ++ch->timer >= 12 ) 
	    {
		if ( ch->timer == 12 && ch->in_room )
		{
		/*
		    ch->was_in_room = ch->in_room;
		    */
		    if ( ch->fighting )
			stop_fighting( ch, TRUE );
		    act( AT_ACTION, "$n rozplywa sie w powietrzu.",
			ch, NULL, NULL, TO_ROOM );
		    send_to_char( "Rozplywasz sie w pustce.\n\r", ch );
		    if ( IS_SET( sysdata.save_flags, SV_IDLE ) )
			save_char_obj( ch );
		    char_from_room( ch );
		    char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) );
		}
	    }

	    if ( ch->pcdata->condition[COND_DRUNK] > 8 )
		worsen_mental_state( ch, ch->pcdata->condition[COND_DRUNK]/8 );
	    if ( ch->pcdata->condition[COND_FULL] > 1 )
	    {
		switch( ch->position )
		{
		    case POS_SLEEPING:  better_mental_state( ch, 4 );	break;
		    case POS_RESTING:   better_mental_state( ch, 3 );	break;
		    case POS_SITTING:   better_mental_state( ch, 2 );	break;
		    case POS_MOUNTED:   better_mental_state( ch, 2 );	break;
		    case POS_STANDING:  better_mental_state( ch, 1 );	break;
		    case POS_FIGHTING:
    		    case POS_EVASIVE:
    		    case POS_DEFENSIVE:
    		    case POS_AGGRESSIVE:
    		    case POS_BERSERK:
			if ( number_bits(2) == 0 )
			    better_mental_state( ch, 1 );
			break;
		}
	    }
	    if ( ch->pcdata->condition[COND_THIRST] > 1 )
	    {
		switch( ch->position )
		{
		    case POS_SLEEPING:  better_mental_state( ch, 5 );	break;
		    case POS_RESTING:   better_mental_state( ch, 3 );	break;
		    case POS_SITTING:	better_mental_state( ch, 2 );	break;
		    case POS_MOUNTED:   better_mental_state( ch, 2 );	break;
		    case POS_STANDING:  better_mental_state( ch, 1 );	break;
		    case POS_FIGHTING:
    		    case POS_EVASIVE:
    		    case POS_DEFENSIVE:
    		    case POS_AGGRESSIVE:
    		    case POS_BERSERK:
			if ( number_bits(2) == 0 )
			    better_mental_state( ch, 1 );
			break;
		}
	    }
/*
	    gain_condition( ch, COND_FULL,   -1 );
*/
	    gain_condition( ch, COND_DRUNK,  -1 );
	    gain_condition( ch, COND_FULL,  -1 + race_table[ch->race]->hunger_mod );

            if (( ch->class == CLASS_VAMPIRE && ch->level >= 10 ) || ( ch->class == CLASS_DEMON && ch->level >= 10 ))
	    {
		if ( time_info.hour < 21 && time_info.hour > 5 )
		  gain_condition( ch, COND_BLOODTHIRST, -1 );
	    }

	    if ( CAN_PKILL( ch ) && ch->pcdata->condition[COND_THIRST] - 9 > 10 )
	      gain_condition( ch, COND_THIRST, -9 );

	    if ( !IS_NPC( ch ) && ch->pcdata->nuisance )
	    {
		int value;
		
		value = ((0 - ch->pcdata->nuisance->flags)*ch->pcdata->nuisance->power);
		gain_condition ( ch, COND_THIRST, value );
		gain_condition ( ch, COND_FULL, --value );
	    }

	    if ( ch->in_room )
	      switch( ch->in_room->sector_type )
	      {
		default:
		    gain_condition( ch, COND_THIRST, -1 + race_table[ch->race]->thirst_mod);  break;
		case SECT_DESERT:
		    gain_condition( ch, COND_THIRST, -3 + race_table[ch->race]->thirst_mod);  break;
		case SECT_UNDERWATER:
		case SECT_OCEANFLOOR:
		    if ( number_bits(1) == 0 )
			gain_condition( ch, COND_THIRST, -1 + race_table[ch->race]->thirst_mod);  break;
	      }

	}
      if ( !IS_NPC(ch) && !IS_IMMORTAL(ch) && ch->pcdata->release_date > 0 &&
           ch->pcdata->release_date <= current_time )
      {
         ROOM_INDEX_DATA *location;
         if ( ch->pcdata->clan )
          location = get_room_index( ch->pcdata->clan->recall );
         else
          location = get_room_index(ROOM_VNUM_TEMPLE);
         if ( !location )
          location = ch->in_room;
         MOBtrigger = FALSE;
         char_from_room(ch);
         char_to_room( ch, location );
         send_to_char( "Bogowie uwalniaja Cie z piekla!\n\r", ch );
         do_look(ch, "auto");
         STRFREE( ch->pcdata->helled_by );
         ch->pcdata->release_date = 0;
         save_char_obj( ch );
      }

	if ( !char_died(ch) )
	{
	    /*
	     * Careful with the damages here,
	     *   MUST NOT refer to ch after damage taken, without checking
	     *   return code and/or char_died as it may be lethal damage.
	     */
	    if ( IS_AFFECTED(ch, AFF_POISON) )
	    {
		act( AT_POISON, "$n slania sie i trzesie.", ch, NULL, NULL, TO_ROOM );
		act( AT_POISON, "Slaniasz sie i trzesiesz.", ch, NULL, NULL, TO_CHAR );
		ch->mental_state = URANGE( 20, ch->mental_state
				 + (IS_NPC(ch) ? 2 : IS_PKILL(ch) ? 3 : 4), 100 );
		damage( ch, ch, 6, gsn_poison );
	    }
/*	    else
	    if ( IS_AFFECTED( ch, AFF_PESTILENCE ) )
	    {
		act( AT_POISON, "Potworna zaraza przezera cialo $d.", ch, NULL, NULL, TO_ROOM );
	    	act( AT_POISON, "Potworna zaraza przezera Twoje cialo!", ch, NULL, NULL, TO_CHAR );
		worsen_mental_state( ch, 10 );
		damage( ch, ch, ch->max_hit / 10, gsn_poison);
	    } 

Pestilence jeszcze nie jest skonczony :P - Crevan */

	    else
	    if ( ch->position == POS_INCAP )
		damage( ch, ch, 1, TYPE_UNDEFINED );
	    else
	    if ( ch->position == POS_MORTAL )
		damage( ch, ch, 4, TYPE_UNDEFINED );
	    if ( char_died(ch) )
		continue;

	    /*
	     * Recurring spell affect
	     */
 	    if ( IS_AFFECTED(ch, AFF_RECURRINGSPELL) )
	    {
		AFFECT_DATA *paf, *paf_next;
		SKILLTYPE *skill;
		bool found = FALSE, died = FALSE;

		for ( paf = ch->first_affect; paf; paf = paf_next )
		{
		    paf_next = paf->next;
		    if ( paf->location == APPLY_RECURRINGSPELL )
		    {
			found = TRUE;
			if ( IS_VALID_SN(paf->modifier)
			&&  (skill=skill_table[paf->modifier]) != NULL
			&&   skill->type == SKILL_SPELL )
			{
			    if ( (*skill->spell_fun)(paf->modifier, ch->level, ch, ch) == rCHAR_DIED
			    ||   char_died(ch) )
			    {
				died = TRUE;
				break;
			    }
			}
		    }
		}
		if ( died )
		    continue;
		if ( !found )
		    xREMOVE_BIT(ch->affected_by, AFF_RECURRINGSPELL);
	    }

	    if ( ch->mental_state >= 30 )
		switch( (ch->mental_state+5) / 10 )
		{
		    case  3:
		    	send_to_char( "Masz goraczke.\n\r", ch );
			act( AT_ACTION, "$n nie wyglada za dobrze.", ch, NULL, NULL, TO_ROOM );
		    	break;
		    case  4:
		    	send_to_char( "Nie czujesz sie zbyt dobrze.\n\r", ch );
			act( AT_ACTION, "$n wyglada troche slabo.", ch, NULL, NULL, TO_ROOM );
		    	break;
		    case  5:
		    	send_to_char( "Potrzebujesz pomocy!\n\r", ch );
			act( AT_ACTION, "Wyglada na to, ze $n potrzebuje pomocy.", ch, NULL, NULL, TO_ROOM );
		    	break;
		    case  6:
		    	send_to_char( "Szukaj pomocy u kleryka.\n\r", ch );
			act( AT_ACTION, "Ktos powinien przyprowadzic kleryka do $c.", ch, NULL, NULL, TO_ROOM );
		    	break;
		    case  7:
		    	send_to_char( "Czujesz, ze nie mozesz usnac...\n\r", ch );
			act( AT_ACTION, "$n nie moze spac spokojnie.", ch, NULL, NULL, TO_ROOM );
		    	break;
		    case  8:
		    	send_to_char( "Wszystko staje sie takie... niezrozumiale.\n\r", ch );
			act( AT_ACTION, "$n zaczyna wygladac szalenczo!", ch, NULL, NULL, TO_ROOM );
		    	break;
		    case  9:
		    	send_to_char( "Jestes JEDYNA osoba we wszechswiecie.\n\r", ch );
			act( AT_ACTION, "$n mowi cos o 'odpowiedzi', 'samotnosci' i innych bzdurach...", ch, NULL, NULL, TO_ROOM );
		    	break;
		    case 10:
		    	send_to_char( "Czujesz, ze koniec jest juz blisko.\n\r", ch );
			act( AT_ACTION, "$n bredzi cos i paple bez sensu...", ch, NULL, NULL, TO_ROOM );
		    	break;
		}
	    if ( ch->mental_state <= -30 )
		switch( (abs(ch->mental_state)+5) / 10 )
		{
		    case  10:
			if ( ch->position > POS_SLEEPING )
			{
			   if ( (ch->position == POS_STANDING
			   ||    ch->position < POS_FIGHTING)
			   &&    number_percent()+10 < abs(ch->mental_state) )
				do_sleep( ch, "" );
			   else
				send_to_char( "Zataczasz sie nieprzytomnie.\n\r", ch );
			}
			break;
		    case   9:
			if ( ch->position > POS_SLEEPING )
			{
			   if ( (ch->position == POS_STANDING
			   ||    ch->position < POS_FIGHTING)
			   &&   (number_percent()+20) < abs(ch->mental_state) )
				do_sleep( ch, "" );
			   else
				send_to_char( "Trudno jest Ci utrzymac otwarte oczy.\n\r", ch );
			}
			break;
		    case   8:
			if ( ch->position > POS_SLEEPING )
			{
			   if ( ch->position < POS_SITTING
			   &&  (number_percent()+30) < abs(ch->mental_state) )
				do_sleep( ch, "" );
			   else
				send_to_char( "Ogarnia Cie sennosc.\n\r", ch );
			}
			break;
		    case   7:
			if ( ch->position > POS_RESTING )
			   send_to_char( "Odczuwasz bezcelowosc swych dzialan.\n\r", ch );
			break;
		    case   6:
			if ( ch->position > POS_RESTING )
			   send_to_char( "You feel sedated.\n\r", ch );
			break;
		    case   5:
			if ( ch->position > POS_RESTING )
			   send_to_char( "Chce Ci sie spac.\n\r", ch );
			break;
		    case   4:
			if ( ch->position > POS_RESTING )
			   send_to_char( "Ogarnia Cie zmeczenie.\n\r", ch );
			break;
		    case   3:
			if ( ch->position > POS_RESTING )
			   send_to_char( "Chcialo by sie odpoczac.\n\r", ch );
			break;
		}
	    if ( ch->timer > 24 )
		do_quit( ch, "" );
	    else
	    if ( ch == ch_save && IS_SET( sysdata.save_flags, SV_AUTO )
	    &&   ++save_count < 5 )	/* save max of 5 per tick */
		save_char_obj( ch );
	}
    }

    return;
}



/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update( void )
{   
    OBJ_DATA *obj;
    sh_int AT_TEMP;

    for ( obj = last_object; obj; obj = gobj_prev )
    {
	CHAR_DATA *rch;
	char *message;

	if ( obj == first_object && obj->prev )
	{
	    bug( "obj_update: first_object->prev != NULL... fixed", 0 );
	    obj->prev = NULL;
	}
	gobj_prev = obj->prev;
	if ( gobj_prev && gobj_prev->next != obj )
	{
	    bug( "obj_update: obj->prev->next != obj", 0 );
	    return;
	}
	set_cur_obj( obj );
	if ( obj->carried_by )
	  oprog_random_trigger( obj ); 
	else
	if( obj->in_room && obj->in_room->area->nplayer > 0 )
	  oprog_random_trigger( obj ); 

        if( obj_extracted(obj) )
	  continue;

	if ( obj->item_type == ITEM_PIPE )
	{
	    if ( IS_SET( obj->value[3], PIPE_LIT ) )
	    {
		if ( --obj->value[1] <= 0 )
		{
		  obj->value[1] = 0;
		  REMOVE_BIT( obj->value[3], PIPE_LIT );
		}
		else
		if ( IS_SET( obj->value[3], PIPE_HOT ) )
		  REMOVE_BIT( obj->value[3], PIPE_HOT );
		else
		{
		  if ( IS_SET( obj->value[3], PIPE_GOINGOUT ) )
		  {
		    REMOVE_BIT( obj->value[3], PIPE_LIT );
		    REMOVE_BIT( obj->value[3], PIPE_GOINGOUT );
		  }
		  else
		    SET_BIT( obj->value[3], PIPE_GOINGOUT );
		}
		if ( !IS_SET( obj->value[3], PIPE_LIT ) )
		  SET_BIT( obj->value[3], PIPE_FULLOFASH );
	    }
	    else
	      REMOVE_BIT( obj->value[3], PIPE_HOT );
	}


/* Corpse decay (npc corpses decay at 8 times the rate of pc corpses) - Narn */

        if ( obj->item_type == ITEM_CORPSE_PC || obj->item_type == ITEM_CORPSE_NPC )
        {
          sh_int timerfrac = UMAX(1, obj->timer - 1);
          if ( obj->item_type == ITEM_CORPSE_PC )
	    timerfrac = (int)(obj->timer / 8 + 1);

	  if ( obj->timer > 0 && obj->value[2] > timerfrac )
	  {
            char buf[MAX_STRING_LENGTH];
/*            char name[MAX_STRING_LENGTH];
            char *bufptr;
            bufptr = one_argument( obj->short_descr, name ); 
            bufptr = one_argument( bufptr, name ); 
            bufptr = one_argument( bufptr, name ); 

	    separate_obj(obj);
            obj->value[2] = timerfrac; 
            sprintf( buf, corpse_descs[ UMIN( timerfrac - 1, 4 ) ], 
                          bufptr ); */
	    sprintf( buf, corpse_descs[ UMIN( timerfrac - 1, 4 ) ], capitalize( obj->short_descr ) );
            STRFREE( obj->description );
            obj->description = STRALLOC( buf ); 
          }  
        }
       
	/* don't let inventory decay */
	if ( IS_OBJ_STAT(obj, ITEM_INVENTORY) )
	  continue;

	/* groundrot items only decay on the ground */
	if ( IS_OBJ_STAT(obj, ITEM_GROUNDROT) && !obj->in_room )
	  continue;

	if ( ( obj->timer <= 0 || --obj->timer > 0 ) )
	  continue;
	
	/* if we get this far, object's timer has expired. */
 
         AT_TEMP = AT_PLAIN;
	 switch ( obj->item_type )
	 {
	 default:
	   message = "$p nagle znika.";
           AT_TEMP = AT_PLAIN;
	   break;
	 case ITEM_CONTAINER:
	   message = "$p rozpada sie ze starosci.";
	   AT_TEMP = AT_OBJECT;
	   break; 
	 case ITEM_PORTAL:
	   message = "$p przestaje istniec w Twojej rzeczywistosci.";
           remove_portal(obj);
	   obj->item_type = ITEM_TRASH;		/* so extract_obj	 */
           AT_TEMP = AT_MAGIC;			/* doesn't remove_portal */
	   break;
	 case ITEM_FOUNTAIN:
	   message = "$p wysycha.";
           AT_TEMP = AT_BLUE;
	   break;
	 case ITEM_CORPSE_NPC:
	   message = "$p rozpada sie w pyl i znika.";
           AT_TEMP = AT_OBJECT;
	   break;
	 case ITEM_CORPSE_PC:
	   message = "$p zmienia sie w kolorowa smuge i znika za choryzontem...";
           AT_TEMP = AT_MAGIC;
	   break;
	 case ITEM_COOK:
	 case ITEM_FOOD:
	   message = "$v porastaja grzyby.";
           AT_TEMP = AT_HUNGRY;
	   break;
         case ITEM_BLOOD:
           message = "$p powoli wsiaka w ziemie.";
           AT_TEMP = AT_BLOOD;
           break;
         case ITEM_BLOODSTAIN:
           message = "$p przysycha do ziemi i powoli znika.";
           AT_TEMP = AT_BLOOD;
	   break;
         case ITEM_SCRAPS:
           message = "$p kruszy sie i rozpada.";
           AT_TEMP = AT_OBJECT;
	   break;
	 case ITEM_FIRE:
	   if (obj->in_room)
	   {
	     --obj->in_room->light;
	     if ( obj->in_room->light < 0 )
			obj->in_room->light = 0;
	   }
	   message = "$p wygasa.";
	   AT_TEMP = AT_FIRE;
	 }

	if ( obj->carried_by )
	{
	    act( AT_TEMP, message, obj->carried_by, obj, NULL, TO_CHAR );
	}
	else if ( obj->in_room
	&&      ( rch = obj->in_room->first_person ) != NULL
	&&	!IS_OBJ_STAT( obj, ITEM_BURIED ) )
	{
	    act( AT_TEMP, message, rch, obj, NULL, TO_ROOM );
            act( AT_TEMP, message, rch, obj, NULL, TO_CHAR );
	}

	if ( obj->serial == cur_obj )
	  global_objcode = rOBJ_EXPIRED;
	extract_obj( obj );
    }
    return;
}


/*
 * Function to check important stuff happening to a player
 * This function should take about 5% of mud cpu time
 */
void char_check( void )
{
    CHAR_DATA *ch, *ch_next;
    OBJ_DATA *obj;
    EXIT_DATA *pexit;
    static int cnt = 0;
    int door, retcode;

    /* This little counter can be used to handle periodic events */
    cnt = (cnt+1) % SECONDS_PER_TICK;

    for ( ch = first_char; ch; ch = ch_next )
    {
	set_cur_char(ch);
	ch_next = ch->next;
	will_fall(ch, 0);

	if ( char_died( ch ) )
	  continue;

	if ( IS_NPC( ch ) )
	{
	    if ( (cnt & 1) )
		continue;

	    /* running mobs	-Thoric */
	    if ( xIS_SET(ch->act, ACT_RUNNING) )
	    {
		if ( !xIS_SET(ch->act, ACT_SENTINEL)
		&&   !ch->fighting && ch->hunting )
		{
		    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );
		    hunt_victim( ch );
		    continue;
		}

		if ( ch->spec_fun )
		{
		    if ( (*ch->spec_fun) ( ch ) )
			continue;
		    if ( char_died(ch) )
			continue;
		}

		if ( !xIS_SET(ch->act, ACT_SENTINEL)
		&&   !xIS_SET(ch->act, ACT_PROTOTYPE)
		&& ( door = number_bits( 4 ) ) <= 9
		&& ( pexit = get_exit(ch->in_room, door) ) != NULL
		&&   pexit->to_room
		&&   !IS_SET(pexit->exit_info, EX_CLOSED)
		&&   !IS_SET(pexit->to_room->room_flags, ROOM_NO_MOB)
		&&   !IS_SET(pexit->to_room->room_flags, ROOM_DEATH)
		&& ( !xIS_SET(ch->act, ACT_STAY_AREA)
		||   pexit->to_room->area == ch->in_room->area ) )
		{
		    retcode = move_char( ch, pexit, 0 );
		    if ( char_died(ch) )
			continue;
		    if ( retcode != rNONE || xIS_SET(ch->act, ACT_SENTINEL)
		    ||    ch->position < POS_STANDING )
			continue;
		}
	    }
	    continue;
	}
	else
	{
	    if ( ch->mount
	    &&   ch->in_room != ch->mount->in_room )
	    {
		xREMOVE_BIT( ch->mount->act, ACT_MOUNTED );
		ch->mount = NULL;
		ch->position = POS_STANDING;
		send_to_char( "No longer upon your mount, you fall to the ground...\n\rOUCH!\n\r", ch );
	    }

	    if ( ( ch->in_room && ch->in_room->sector_type == SECT_UNDERWATER )
	    || ( ch->in_room && ch->in_room->sector_type == SECT_OCEANFLOOR ) )
	    {
		if ( !IS_AFFECTED( ch, AFF_AQUA_BREATH ) )
		{
		    if ( ch->level < LEVEL_IMMORTAL )
		    {
			int dam;

        		/* Changed level of damage at Brittany's request. -- Narn */	
			dam = number_range( ch->max_hit / 100, ch->max_hit / 50 );
			dam = UMAX( 1, dam );
			if ( number_bits(3) == 0 )
			  send_to_char( "Dlawisz sie i krztusisz, gdy robisz wdech pod woda!\n\r", ch );
			damage( ch, ch, dam, TYPE_UNDEFINED );
		    }
		}
	    }
	
	    if ( char_died( ch ) )
		continue; 

	    if ( ch->in_room
	    && (( ch->in_room->sector_type == SECT_WATER_NOSWIM )
	    ||  ( ch->in_room->sector_type == SECT_WATER_SWIM ) ) )
	    {
		if ( !IS_AFFECTED( ch, AFF_FLYING )
		&& !IS_AFFECTED( ch, AFF_FLOATING ) 
		&& !IS_AFFECTED( ch, AFF_AQUA_BREATH )
		&& !ch->mount )
		{
		    for ( obj = ch->first_carrying; obj; obj = obj->next_content )
		      if ( obj->item_type == ITEM_BOAT ) break;

		    if ( !obj )
		    {
			if ( ch->level < LEVEL_IMMORTAL )
			{
			    int mov;
			    int dam;

			    if ( ch->move > 0 )
			    {
				mov = number_range( ch->max_move / 20, ch->max_move / 5 );
				mov = UMAX( 1, mov );

				if ( ch->move - mov < 0 )
				    ch->move = 0;
				else
				    ch->move -= mov;
			    }
			    else
			    {
				dam = number_range( ch->max_hit / 20, ch->max_hit / 5 );
				dam = UMAX( 1, dam );

				if ( number_bits(3) == 0 )
				   send_to_char( "Struggling with exhaustion, you choke on a mouthful of water.\n\r", ch );
				damage( ch, ch, dam, TYPE_UNDEFINED );
			    }
			}
		    }
		}
	    }

	    /* beat up on link dead players */
	    if ( !ch->desc )
	    {
		CHAR_DATA *wch, *wch_next;

		for ( wch = ch->in_room->first_person; wch; wch = wch_next )
		{
		    wch_next	= wch->next_in_room;

		    if (!IS_NPC(wch)
		    ||   wch->fighting
		    ||   IS_AFFECTED(wch, AFF_CHARM)
		    ||   !IS_AWAKE(wch)
		    || (xIS_SET(wch->act, ACT_WIMPY) && IS_AWAKE(ch) )
		    ||   !can_see( wch, ch ) )
			continue;

		    if ( is_hating( wch, ch ) )
		    {
			found_prey( wch, ch );
			continue;
		    }

		    if ( (!xIS_SET(wch->act, ACT_AGGRESSIVE)
		    &&    !xIS_SET(wch->act, ACT_META_AGGR))
		    ||    xIS_SET(wch->act, ACT_MOUNTED)
		    ||    IS_SET(wch->in_room->room_flags, ROOM_SAFE ) )
			continue;
		    global_retcode = multi_hit( wch, ch, TYPE_UNDEFINED );
		}
	    }
	}
    }
}


/*
 * Aggress.
 *
 * for each descriptor
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function should take 5% to 10% of ALL mud cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 */
void aggr_update( void )
{
    DESCRIPTOR_DATA *d, *dnext;
    CHAR_DATA *wch;
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    CHAR_DATA *victim;
    struct act_prog_data *apdtmp;

#ifdef UNDEFD
  /*
   *  GRUNT!  To do
   *
   */
        if ( IS_NPC( wch ) && wch->mpactnum > 0
	    && wch->in_room->area->nplayer > 0 )
        {
            MPROG_ACT_LIST * tmp_act, *tmp2_act;
	    for ( tmp_act = wch->mpact; tmp_act;
		 tmp_act = tmp_act->next )
	    {
                 oprog_wordlist_check( tmp_act->buf,wch, tmp_act->ch,
				      tmp_act->obj, tmp_act->vo, ACT_PROG );
                 DISPOSE( tmp_act->buf );
            }
	    for ( tmp_act = wch->mpact; tmp_act; tmp_act = tmp2_act )
	    {
                 tmp2_act = tmp_act->next;
                 DISPOSE( tmp_act );
            }
            wch->mpactnum = 0;
            wch->mpact    = NULL;
        }
#endif

    /* check mobprog act queue */
    while ( (apdtmp = mob_act_list) != NULL )
    {
	wch = mob_act_list->vo;
	if ( !char_died(wch) && wch->mpactnum > 0 )
	{
	    MPROG_ACT_LIST * tmp_act;

	    while ( (tmp_act = wch->mpact) != NULL )
	    {
		if ( tmp_act->obj && obj_extracted(tmp_act->obj) )
		  tmp_act->obj = NULL;
		if ( tmp_act->ch && !char_died(tmp_act->ch) )
		  mprog_wordlist_check( tmp_act->buf, wch, tmp_act->ch,
					tmp_act->obj, tmp_act->vo, ACT_PROG );
		wch->mpact = tmp_act->next;
		DISPOSE(tmp_act->buf);
		DISPOSE(tmp_act);
	    }
	    wch->mpactnum = 0;
	    wch->mpact    = NULL;
        }
	mob_act_list = apdtmp->next;
	DISPOSE( apdtmp );
    }


    /*
     * Just check descriptors here for victims to aggressive mobs
     * We can check for linkdead victims in char_check	-Thoric
     */
    for ( d = first_descriptor; d; d = dnext )
    {
	dnext = d->next;
	if ( d->connected != CON_PLAYING || (wch=d->character) == NULL )
	   continue;

	if ( char_died(wch)
	||   IS_NPC(wch)
	||   wch->level >= LEVEL_IMMORTAL
	||  !wch->in_room )
	    continue;

	for ( ch = wch->in_room->first_person; ch; ch = ch_next )
	{
	    int count;

	    ch_next	= ch->next_in_room;

	    if ( !IS_NPC(ch)
	    ||   ch->fighting
	    ||   IS_AFFECTED(ch, AFF_CHARM)
	    ||   !IS_AWAKE(ch)
	    ||   (xIS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch) )
	    ||   !can_see( ch, wch ) )
		continue;

	    if ( is_hating(ch, wch) )
	    {
		found_prey(ch, wch);
		continue;
	    }

	    if ( (!xIS_SET(ch->act, ACT_AGGRESSIVE)
	    &&    !xIS_SET(ch->act, ACT_META_AGGR))
	    ||     xIS_SET(ch->act, ACT_MOUNTED)
	    ||     IS_SET(ch->in_room->room_flags, ROOM_SAFE ) )
		continue;

	    /*
	     * Ok we have a 'wch' player character and a 'ch' npc aggressor.
	     * Now make the aggressor fight a RANDOM pc victim in the room,
	     *   giving each 'vch' an equal chance of selection.
	     *
	     * Depending on flags set, the mob may attack another mob
	     */
	    count	= 0;
	    victim	= NULL;
	    for ( vch = wch->in_room->first_person; vch; vch = vch_next )
	    {
		vch_next = vch->next_in_room;

		if ( (!IS_NPC(vch) || xIS_SET(ch->act, ACT_META_AGGR)
		||    xIS_SET(vch->act, ACT_ANNOYING))
		&&   vch->level < LEVEL_IMMORTAL
		&&   (!xIS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch) )
		&&   can_see( ch, vch ) )
		{
		    if ( number_range( 0, count ) == 0 )
			victim = vch;
		    count++;
		}
	    }

	    if ( !victim )
	    {
		bug( "Aggr_update: null victim.", count );
		continue;
	    }

	    /* backstabbing mobs (Thoric) */
	    if ( IS_NPC(ch) && xIS_SET(ch->attacks, ATCK_BACKSTAB ) )
	    {
		OBJ_DATA *obj;

		if ( !ch->mount
    		&& (obj = get_eq_char( ch, WEAR_WIELD )) != NULL
    		&& (obj->value[3] == 11 || obj->value[3] == 2)
		&& !victim->fighting
		&& victim->hit >= victim->max_hit )
		{
		    check_attacker( ch, victim );
		    WAIT_STATE( ch, skill_table[gsn_backstab]->beats );
		    if ( !IS_AWAKE(victim)
		    ||   number_percent( )+5 < ch->level )
		    {
			global_retcode = multi_hit( ch, victim, gsn_backstab );
			continue;
		    }
		    else
		    {
			global_retcode = damage( ch, victim, 0, gsn_backstab );
			continue;
		    }
		}
	    }
	    global_retcode = multi_hit( ch, victim, TYPE_UNDEFINED );
	}
    }

    return;
}

/* From interp.c */
bool check_social  args( ( CHAR_DATA *ch, char *command, char *argument ) );

/*
 * drunk randoms	- Tricops
 * (Made part of mobile_update	-Thoric)
 */
void drunk_randoms( CHAR_DATA *ch )
{
    CHAR_DATA *rvch = NULL;
    CHAR_DATA *vch;
    sh_int drunk;
    sh_int position;

    if ( IS_NPC( ch ) || ch->pcdata->condition[COND_DRUNK] <= 0 )
	return;

    if ( number_percent() < 30 )
	return;

    drunk = ch->pcdata->condition[COND_DRUNK];
    position = ch->position;
    ch->position = POS_STANDING;

    if ( number_percent() < (2*drunk / 20) )
	check_social( ch, "burp", "" );
    else
    if ( number_percent() < (2*drunk / 20) )
	check_social( ch, "hiccup", "" );
    else
    if ( number_percent() < (2*drunk / 20) )
	check_social( ch, "drool", "" );
    else
    if ( number_percent() < (2*drunk / 20) )
	check_social( ch, "fart", "" );
    else
    if ( drunk > (10+(get_curr_con(ch)/5))
    &&   number_percent() < ( 2 * drunk / 18 ) )
    {
	for ( vch = ch->in_room->first_person; vch; vch = vch->next_in_room )
	    if ( number_percent() < 10 )
		rvch = vch;
	check_social( ch, "puke", (rvch ? rvch->name : "") );
    }

    ch->position = position;
    return;
}

/*
 * Random hallucinations for those suffering from an overly high mentalstate
 * (Hats off to Albert Hoffman's "problem child")	-Thoric
 */
void hallucinations( CHAR_DATA *ch )
{
    if ( ch->mental_state >= 30 && number_bits(5 - (ch->mental_state >= 50) - (ch->mental_state >= 75)) == 0 )
    {
	char *t;

	switch( number_range( 1, UMIN(21, (ch->mental_state+5) / 5)) )
	{
	    default:
	    case  1: t = "Jestes bardzo niespokojny... nie mozesz usiedziec na miejscu.\n\r";		break;
	    case  2: t = "Przeszywaja Cie dreszcze.\n\r";				break;
	    case  3: t = "Masz gesia skorke.\n\r";					break;
	    case  4: t = "Nagle zorientowales sie, ze cos jest bardzo nie w porzadku.\n\r";	break;
	    case  5: t = "Te cholerne male duszki nasmiewaja sie z Ciebie!\n\r";		break;
	    case  6: t = "Slyszysz placz swojej matki...\n\r";			break;
	    case  7: t = "Byles tu juz kiedys, czy nie? Nie jestes pewien...\n\r";	break;
	    case  8: t = "Bolesne wspomnienia z dziecinstwa przeszywaja twoj umysl.\n\r";	break;
	    case  9: t = "Kto wzywa Cie z oddali...\n\r";	break;
	    case 10: t = "Twoja glowa pulsuje... nie mozesz myslec spokojnie.\n\r";	break;
	    case 11: t = "Ziemia pod twoimi stopami...chyba sie trzesie.\n\r";			break;
	    case 12: t = "Nie jestes juz pewien, co jest realne.\n\r";		break;
	    case 13: t = "To wszystko to sen... a moze nie?\n\r";				break;
	    case 14: t = "Slyszysz swoje wnuki blagajace Cie, bys sie nimi zajal.\n\r";	break;
	    case 15: t = "Oni przychodza po Ciebie... by zabrac Cie gdzies daleko...\n\r";	break;
	    case 16: t = "Zaczynasz czuc sie potezny!\n\r";				break;
	    case 17: t = "Jestes lekki jak powietrze... przestworza czekaja na Ciebie.\n\r";	break;
	    case 18: t = "Nagle widzisz przed oczami cale swoje zycie... i swoja przyszlosc...\n\r";	break;
	    case 19: t = "Jestes wszedzie i wszystkim... Wiesz wszystko i widzisz wszystko!\n\r";	break;
	    case 20: t = "Jestes niesmiertelny!\n\r";					break;
	    case 21: t = "Ah... mozliwosci Najwyzszej Istoty... co by tu zrobic?\n\r";	break;
	}
	send_to_char( t, ch );
    }
    return;
}

void tele_update( void )
{
    TELEPORT_DATA *tele, *tele_next;

    if ( !first_teleport )
      return;
    
    for ( tele = first_teleport; tele; tele = tele_next )
    {
	tele_next = tele->next;
	if ( --tele->timer <= 0 )
	{
	    if ( tele->room->first_person )
	    {
		if ( IS_SET( tele->room->room_flags, ROOM_TELESHOWDESC ) )
		  teleport( tele->room->first_person, tele->room->tele_vnum,
			TELE_SHOWDESC | TELE_TRANSALL );
		else
		  teleport( tele->room->first_person, tele->room->tele_vnum,
			TELE_TRANSALL );
	    }
	    UNLINK( tele, first_teleport, last_teleport, next, prev );
	    DISPOSE( tele );
	}
    }
}

#if FALSE
/* 
 * Write all outstanding authorization requests to Log channel - Gorog
 */ 
/* void auth_update( void ) 
{ 
  CHAR_DATA *victim; 
  DESCRIPTOR_DATA *d; 
  char log_buf [MAX_INPUT_LENGTH];
  bool first_time = TRUE;         

  for ( d = first_descriptor; d; d = d->next ) 
      {
      victim = d->character;
      if ( victim && IS_WAITING_FOR_AUTH(victim) )
         {
         if ( first_time )
            {
            first_time = FALSE;
            strcpy (log_buf, "Oczekuje na autoryzacje:" ); 
            to_channel( log_buf, CHANNEL_AUTH, "Auth", 1);
            }
         sprintf( log_buf, " %s@%s new %s %s", victim->name,
            victim->desc->host, race_table[victim->race]->race_name, 
            class_table[victim->class]->who_name ); 
         to_channel( log_buf, CHANNEL_AUTH, "Auth", 1);
         }
      }
} 
#endif

void auth_update( void ) 
{ 
    CHAR_DATA *victim; 
    DESCRIPTOR_DATA *d; 
    char buf [MAX_INPUT_LENGTH], log_buf [MAX_INPUT_LENGTH];
    for ( d = first_descriptor; d; d = d->next ) 
    {
	if ( (victim = d->character) && IS_WAITING_FOR_AUTH(victim) )
	{
	    found_hit = TRUE;
	    sprintf( buf, " %s@%s new %s %s\n\r", victim->name,
		victim->desc->host, race_table[victim->race]->race_name, 
		class_table[victim->class]->who_name );
	    strcat (log_buf, buf);
	}
    }
    if (found_hit)
    {
	to_channel( log_buf, CHANNEL_AUTH, "Auth", 1);
    }
} 
Flash */
/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */

#endif
void update_handler( void )
{
    static  int     pulse_area;
    static  int     pulse_mobile;
    static  int     pulse_violence;
    static  int     pulse_point;
    static  int	    pulse_second;
    struct timeval stime;
    struct timeval etime;

    if ( timechar )
    {
      set_char_color(AT_PLAIN, timechar);
      send_to_char( "Starting update timer.\n\r", timechar );
      gettimeofday(&stime, NULL);
    }
    
    if ( --pulse_area     <= 0 )
    {
	pulse_area	= number_range( PULSE_AREA / 2, 3 * PULSE_AREA / 2 );
	area_update	( );
        quest_update     ( );
    }

    if ( --pulse_mobile   <= 0 )
    {
	pulse_mobile	= PULSE_MOBILE;
	mobile_update  ( );
    }

    if ( --pulse_violence <= 0 )
    {
	pulse_violence	= PULSE_VIOLENCE;
	violence_update	( );
    }

    if ( --pulse_point    <= 0 )
    {
	pulse_point     = number_range( PULSE_TICK * 0.75, PULSE_TICK * 1.25 );

        auth_update     ( );			/* Gorog */
        time_update	( );
	weather_update	( );
	char_update	( );
	obj_update	( );
	clear_vrooms	( );			/* remove virtual rooms */
    }

    if ( --pulse_second   <= 0 )
    {
	pulse_second	= PULSE_PER_SECOND;
	char_check( );
 	reboot_check(0);
    }

    if ( --auction->pulse <= 0 )
    {                                                  
	auction->pulse = PULSE_AUCTION;                     
	auction_update( );
    }

    tele_update( );
    aggr_update( );
    obj_act_update ( );
    room_act_update( );
    clean_obj_queue();		/* dispose of extracted objects */
    clean_char_queue();		/* dispose of dead mobs/quitting chars */
    if ( timechar )
    {
      gettimeofday(&etime, NULL);
      set_char_color(AT_PLAIN, timechar);
      send_to_char( "Update timing complete.\n\r", timechar );
      subtract_times(&etime, &stime);
      ch_printf( timechar, "Timing took %d.%06d seconds.\n\r",
          etime.tv_sec, etime.tv_usec );
      timechar = NULL;
    }
    tail_chain( );
    return;
}


void remove_portal( OBJ_DATA *portal )
{
    ROOM_INDEX_DATA *fromRoom, *toRoom;
    EXIT_DATA *pexit;
    bool found;

    if ( !portal )
    {
	bug( "remove_portal: portal is NULL", 0 );
	return;
    }

    fromRoom = portal->in_room;
    found = FALSE;
    if ( !fromRoom )
    {
	bug( "remove_portal: portal->in_room is NULL", 0 );
	return;
    }

    for ( pexit = fromRoom->first_exit; pexit; pexit = pexit->next )
	if ( IS_SET( pexit->exit_info, EX_PORTAL ) )
	{
	    found = TRUE;
	    break;
	}

    if ( !found )
    {
	bug( "remove_portal: portal not found in room %d!", fromRoom->vnum );
	return;
    }

    if ( pexit->vdir != DIR_PORTAL )
	bug( "remove_portal: exit in dir %d != DIR_PORTAL", pexit->vdir );

    if ( ( toRoom = pexit->to_room ) == NULL )
      bug( "remove_portal: toRoom is NULL", 0 );
 
    extract_exit( fromRoom, pexit );

    return;
}

void reboot_check( time_t reset )
{
  static char *tmsg[] =
  { "\n\rSwiat przed twoimi oczami zaczyna sie rozmazywac.",
    "\n\rCzujesz, ze ziemia trzesie sie coraz bardziej, w miare jak zbliza sie koniec!",
    "\n\rBlyskawica przecina niebo!",
    "\n\rHuk jak przy uderzeniu pioruna obiega swiat!",
    "\n\rNiebo zrobilo sie czarne jak w samym srodku nocy.",
    "\n\rZauwazyles, ze wokol siebie widzisz coraz mniej zywych istot.",
    "\n\rMorza i oceany na calym swiecie rozszalaly sie.",
    "\n\rMagiczna aura spowijajaca ten swiat wydaje sie byc lekko niestabilna.",
    "\n\rWyczuwasz niewielka zmiane w magii swiata."
  };
  static const int times[] = { 10, 60, 120, 180, 240, 300, 600, 900, 1800 };
  static const int timesize =
      UMIN(sizeof(times)/sizeof(*times), sizeof(tmsg)/sizeof(*tmsg));
  char buf[MAX_STRING_LENGTH];
  static int trun;
  static bool init = FALSE;
  
  if ( !init || reset >= current_time )
  {
    for ( trun = timesize-1; trun >= 0; trun-- )
      if ( reset >= current_time+times[trun] )
        break;
    init = TRUE;
    return;
  }
  
  if ( (current_time % 1800) == 0 )
  {
    sprintf(buf, "%.24s: %d graczy", ctime(&current_time), num_descriptors);
    append_to_file(USAGE_FILE, buf);
    sprintf(buf, "%.24s:  %dptn  %dpll  %dsc %dbr  %d global loot",
	ctime(&current_time),
	sysdata.upotion_val,
	sysdata.upill_val,
	sysdata.scribed_used,
	sysdata.brewed_used,
	sysdata.global_looted );
    append_to_file( ECONOMY_FILE, buf );
  }
  
  if ( new_boot_time_t - boot_time < 60*60*18 &&
      !set_boot_time->manual )
    return;
  
  if ( new_boot_time_t <= current_time )
  {
    CHAR_DATA *vch;
    extern bool mud_down;
    
    if ( auction->item )
    {
/*      sprintf(buf, "Sale of %s has been stopped by mud.",
          auction->item->short_descr);*/
        sprintf(buf, "Aukcja zostala przerwana w zwiazku z rebootem muda.");
      talk_auction(buf);
      obj_to_char(auction->item, auction->seller);
      auction->item = NULL;
      if ( auction->buyer && auction->buyer != auction->seller )
      {
        auction->buyer->gold += auction->bet;
        send_to_char("Twoje pieniadze zostaly Ci zwrocone.\n\r", auction->buyer);
      }
    }
    echo_to_all(AT_YELLOW, "\n\rPotezna magiczna sila wyrzuca Cie z tego swiata do czasu"
        "\n\raz zycie sie w nim nie odrodzi.", ECHOTAR_ALL);
    log_string( "Automatic Reboot" ); 
    for ( vch = first_char; vch; vch = vch->next )
      if ( !IS_NPC(vch) )
        save_char_obj(vch);
    mud_down = TRUE;
    return;
  }
  
  if ( trun != -1 && new_boot_time_t - current_time <= times[trun] )
  {
    echo_to_all(AT_YELLOW, tmsg[trun], ECHOTAR_ALL);
    if ( trun <= 5 )
      sysdata.DENY_NEW_PLAYERS = TRUE;
    --trun;
    return;
  }
  return;
}
  
/* the auction update*/

void auction_update (void)
{
    int tax, pay;
    char buf[MAX_STRING_LENGTH];

    if(!auction->item)
    {
    	if(AUCTION_MEM > 0 && auction->history[0] &&
    			++auction->hist_timer == 6*AUCTION_MEM)
    	{
    		int i;
    		
    		for(i = AUCTION_MEM - 1; i >= 0; i--)
    		{
    			if(auction->history[i])
    			{
    				auction->history[i] = NULL;
    				auction->hist_timer = 0;
    				break;
    			}
    		}
    	}
    	return;
    }

    switch (++auction->going) /* increase the going state */
    {
	case 1 : /* going once */
	case 2 : /* going twice */
	    if (auction->bet > auction->starting)
		sprintf (buf, "%s: po raz %s za %d.", auction->item->short_descr,
			((auction->going == 1) ? "pierwszy" : "drugi"), auction->bet);
	    else
		sprintf (buf, "%s: po raz %s (bez podbicia ceny).", auction->item->short_descr,
			((auction->going == 1) ? "pierwszy" : "drugi"));

	    talk_auction (buf);
	    break;

	case 3 : /* SOLD! */
	    if (!auction->buyer && auction->bet)
	    {
		bug( "Auction code reached SOLD, with NULL buyer, but %d gold bid", auction->bet );
		auction->bet = 0;
	    }
	    if (auction->bet > 0 && auction->buyer != auction->seller)
	    {
		sprintf (buf, "%s kupuje %s za %d.",
			auction->item->short_descr,
			IS_NPC(auction->buyer) ? auction->buyer->short_descr : auction->buyer->name,
			auction->bet);
		talk_auction(buf);

		act(AT_ACTION, "Aukcjoner pojawia sie przed Toba i wrecza Ci $v.",
			auction->buyer, auction->item, NULL, TO_CHAR);
		act(AT_ACTION, "Aukcjoner pojawia sie przed $n, by wreczyc $v.",
			auction->buyer, auction->item, NULL, TO_ROOM);

		if ( (auction->buyer->carry_weight 
		+     get_obj_weight( auction->item ))
		>     can_carry_w( auction->buyer ) )
		{
		    act( AT_PLAIN, "$p is too heavy for you to carry with your current inventory.", auction->buyer, auction->item, NULL, TO_CHAR );
    		    act( AT_PLAIN, "$n is carrying too much to also carry $p, and $e drops it.", auction->buyer, auction->item, NULL, TO_ROOM );
		    obj_to_room( auction->item, auction->buyer->in_room );
		}
		else
		    obj_to_char( auction->item, auction->buyer );
	        pay = (int)auction->bet * 0.9;
		tax = (int)auction->bet * 0.1;
		boost_economy( auction->seller->in_room->area, tax );
                auction->seller->gold += pay; /* give him the money, tax 10 % */
		sprintf(buf, "The auctioneer pays you %d gold, charging an auction fee of %d.\n\r", pay, tax);
		send_to_char(buf, auction->seller);
                auction->item = NULL; /* reset item */
		if ( IS_SET( sysdata.save_flags, SV_AUCTION ) )
		{
		    save_char_obj( auction->buyer );
		    save_char_obj( auction->seller );
		}
            }
            else /* not sold */
            {
                sprintf (buf, "No bids received for %s - removed from auction.\n\r",auction->item->short_descr);
                talk_auction(buf);
                act (AT_ACTION, "The auctioneer appears before you to return $p to you.",
                      auction->seller,auction->item,NULL,TO_CHAR);
                act (AT_ACTION, "The auctioneer appears before $n to return $p to $m.",
                      auction->seller,auction->item,NULL,TO_ROOM);
		if ( (auction->seller->carry_weight
		+     get_obj_weight( auction->item ))
		>     can_carry_w( auction->seller ) )
		{
		    act( AT_PLAIN, "You drop $p as it is just too much to carry"
			" with everything else you're carrying.", auction->seller,
			auction->item, NULL, TO_CHAR );
		    act( AT_PLAIN, "$n drops $p as it is too much extra weight"
			" for $m with everything else.", auction->seller,
			auction->item, NULL, TO_ROOM );
		    obj_to_room( auction->item, auction->seller->in_room );
		}
		else
		    obj_to_char (auction->item,auction->seller);
		tax = (int)auction->item->cost * 0.05;
		boost_economy( auction->seller->in_room->area, tax );
		sprintf(buf, "The auctioneer charges you an auction fee of %d.\n\r", tax );
		send_to_char(buf, auction->seller);
		if ((auction->seller->gold - tax) < 0)
		  auction->seller->gold = 0;
		else
		  auction->seller->gold -= tax;
		if ( IS_SET( sysdata.save_flags, SV_AUCTION ) )
		    save_char_obj( auction->seller );
	    } /* else */
	    auction->item = NULL; /* clear auction */
    } /* switch */
} /* func */

void subtract_times(struct timeval *etime, struct timeval *stime)
{
  etime->tv_sec -= stime->tv_sec;
  etime->tv_usec -= stime->tv_usec;
  while ( etime->tv_usec < 0 )
  {
    etime->tv_usec += 1000000;
    etime->tv_sec--;
  }
  return;
}

/*
 * Function to update weather vectors according to climate
 * settings, random effects, and neighboring areas.
 * Last modified: July 18, 1997
 * - Fireblade
 */
void adjust_vectors(WEATHER_DATA *weather)
{
	NEIGHBOR_DATA *neigh;
	double dT, dP, dW;

	if(!weather)
	{
		bug("adjust_vectors: NULL weather data.", 0);
		return;
	}

	dT = 0;
	dP = 0;
	dW = 0;

	/* Add in random effects */
	dT += number_range(-rand_factor, rand_factor);
	dP += number_range(-rand_factor, rand_factor);
	dW += number_range(-rand_factor, rand_factor);
	
	/* Add in climate effects*/
	dT += climate_factor *
		(((weather->climate_temp - 2)*weath_unit) -
		(weather->temp))/weath_unit;
	dP += climate_factor *
		(((weather->climate_precip - 2)*weath_unit) -
		(weather->precip))/weath_unit;
	dW += climate_factor *
		(((weather->climate_wind - 2)*weath_unit) -
		(weather->wind))/weath_unit;
	
		
	/* Add in effects from neighboring areas */
	for(neigh = weather->first_neighbor; neigh;
			neigh = neigh->next)
	{
		/* see if we have the area cache'd already */
		if(!neigh->address)
		{
			/* try and find address for area */
			neigh->address = get_area(neigh->name);
			
			/* if couldn't find area ditch the neigh */
			if(!neigh->address)
			{
				NEIGHBOR_DATA *temp;
				bug("adjust_weather: "
					"invalid area name.", 0);
				temp = neigh->prev;
				UNLINK(neigh,
				       weather->first_neighbor,
				       weather->last_neighbor,
				       next,
				       prev);
				STRFREE(neigh->name);
				DISPOSE(neigh);
				neigh = temp;
				continue;
			}
		}
		
		dT +=(neigh->address->weather->temp -
		      weather->temp) / neigh_factor;
		dP +=(neigh->address->weather->precip -
		      weather->precip) / neigh_factor;
		dW +=(neigh->address->weather->wind -
		      weather->wind) / neigh_factor;
	}
	
	/* now apply the effects to the vectors */
	weather->temp_vector += (int)dT;
	weather->precip_vector += (int)dP;
	weather->wind_vector += (int)dW;

	/* Make sure they are within the right range */
	weather->temp_vector = URANGE(-max_vector,
		weather->temp_vector, max_vector);
	weather->precip_vector = URANGE(-max_vector,
		weather->precip_vector, max_vector);
	weather->wind_vector = URANGE(-max_vector,
		weather->wind_vector, max_vector);
	
	return;
}

/*
 * function updates weather for each area
 * Last Modified: July 31, 1997
 * Fireblade
 */
void weather_update()
{
	AREA_DATA *pArea;
	DESCRIPTOR_DATA *d;
	int limit;
	
	limit = 3 * weath_unit;
	
	for(pArea = first_area; pArea;
		pArea = (pArea == last_area) ? first_build : pArea->next)
	{
		/* Apply vectors to fields */
		pArea->weather->temp +=
			pArea->weather->temp_vector;
		pArea->weather->precip +=
			pArea->weather->precip_vector;
		pArea->weather->wind +=
			pArea->weather->wind_vector;
		
		/* Make sure they are within the proper range */
		pArea->weather->temp = URANGE(-limit,
			pArea->weather->temp, limit);
		pArea->weather->precip = URANGE(-limit,
			pArea->weather->precip, limit);
		pArea->weather->wind = URANGE(-limit,
			pArea->weather->wind, limit);
		
		/* get an appropriate echo for the area */
		get_weather_echo(pArea->weather);
	}

	for(pArea = first_area; pArea;
		pArea = (pArea == last_area) ? first_build : pArea->next)
	{
		adjust_vectors(pArea->weather);
	}
	
	/* display the echo strings to the appropriate players */
	for(d = first_descriptor; d; d = d->next)
	{
		WEATHER_DATA *weath;
		
		if(d->connected == CON_PLAYING &&
			IS_OUTSIDE(d->character) &&
			!NO_WEATHER_SECT(d->character->in_room->sector_type) &&
			IS_AWAKE(d->character))
		{
			weath = d->character->in_room->area->weather;
			if(!weath->echo)
				continue;
			set_char_color(weath->echo_color, d->character);
			ch_printf(d->character, weath->echo);
		}
	}
	
	return;
}

/*
 * get weather echo messages according to area weather...
 * stores echo message in weath_data.... must be called before
 * the vectors are adjusted
 * Last Modified: August 10, 1997
 * Fireblade
 */
void get_weather_echo(WEATHER_DATA *weath)
{
	int n;
	int temp, precip, wind;
	int dT, dP, dW;
	int tindex, pindex, windex;
	
	/* set echo to be nothing */
	weath->echo = NULL;
	weath->echo_color = AT_GREY;
	
	/* get the random number */
	n = number_bits(2);
	
	/* variables for convenience */
	temp = weath->temp;
	precip = weath->precip;
	wind = weath->wind;

	dT = weath->temp_vector;
	dP = weath->precip_vector;
	dW = weath->wind_vector;
	
	tindex = (temp + 3*weath_unit - 1)/weath_unit;
	pindex = (precip + 3*weath_unit - 1)/weath_unit;
	windex = (wind + 3*weath_unit - 1)/weath_unit;
	
	/* get the echo string... mainly based on precip */
	switch(pindex)
	{
		case 0:
			if(precip - dP > -2*weath_unit)
			{
				char *echo_strings[4] =
				{
					"The clouds disappear.\n\r",
					"The clouds disappear.\n\r",
					"The sky begins to break through "
						"the clouds.\n\r",
					"The clouds are slowly "
						"evaporating.\n\r"
				};
				
				weath->echo = echo_strings[n];
				weath->echo_color = AT_WHITE;
			}
			break;

		case 1:
			if(precip - dP <= -2*weath_unit)
			{
				char *echo_strings[4] =
				{
					"The sky is getting cloudy.\n\r",
					"The sky is getting cloudy.\n\r",
					"Light clouds cast a haze over "
						"the sky.\n\r",
					"Billows of clouds spread through "
						"the sky.\n\r"
				};
				weath->echo = echo_strings[n];
				weath->echo_color = AT_GREY;
			}
			break;
			
		case 2:
			if(precip - dP > 0)
			{
				if(tindex > 1)
				{
					char *echo_strings[4] =
					{
						"Deszcz cichnie.\n\r",
						"Deszcz cichnie.\n\r",
						"Ulewa powoli zanika.\n\r",
						"Intensywnosc deszczu "
							"spada.\n\r"
					};
					weath->echo = echo_strings[n];
					weath->echo_color = AT_CYAN;
				}
				else
				{
					char *echo_strings[4] =
					{
						"Snieg przestaje padac.\n\r",
						"Snieg przestaje padac.\n\r",
						"Sniezny prysznic powoli "
							"zanika.\n\r",
						"Sniezne platki znikaja "
							"z zimowego nieba.\n\r"
					};
					weath->echo = echo_strings[n];
					weath->echo_color = AT_WHITE;
				}
			}
			break;
			
		case 3:
			if(precip - dP <= 0)
			{
				if(tindex > 1)
				{
					char *echo_strings[4] =
					{
						"Zaczyna padac.\n\r",
						"Zaczyna padac.\n\r",
						"Spadaja na Ciebie drobne "
							"krople deszczu.\n\r",
						"Wokol Ciebie spadaja krople "
							"deszczu.\n\r"
					};
					weath->echo = echo_strings[n];
					weath->echo_color = AT_CYAN;
				}
				else
				{
					char *echo_strings[4] =
					{
						"Zaczyna padac snieg.\n\r",
						"Zaczyna padac snieg.\n\r",
						"Z nieba zaczynaja spadac "
							"krysztalowe "
							"platki.\n\r",
						"Bielutki snieg splywa "
							"powoli z chmur.\n\r"
					};
					weath->echo = echo_strings[n];
					weath->echo_color = AT_WHITE;
				}
			}
			else if(tindex < 2 && temp - dT > -weath_unit)
			{
				char *echo_strings[4] =
				{
					"Robi sie zimniej i deszcz zmienia "
						"sie w bialy snieg.\n\r",
					"Robi sie zimniej i deszcz zmienia "
						"sie w bialy snieg.\n\r",
					"Robi sie zimno i deszcz zamarza.\n\r",
					"Duze platki sniegu zaczynaja padac "
						"wraz z deszczem.\n\r"
				};
				weath->echo = echo_strings[n];
				weath->echo_color = AT_WHITE;
			}
			else if(tindex > 1 && temp - dT <= -weath_unit)
			{
				char *echo_strings[4] =
				{
					"Sniezne platki rozpuszczaja sie "
						"tworzac zimny deszcz.\n\r",
					"Sniezne platki rozpuszczaja sie "
						"tworzac zimny deszcz.\n\r",
					"Padajacy deszcz zmienia sie w zimna mzawke.\n\r",
					"Cieply prad zmienia snieg w niewielki deszcz.\n\r"
				};
				weath->echo = echo_strings[n];
				weath->echo_color = AT_CYAN;
			} 
			break;
		
		case 4:
			if(precip - dP > 2*weath_unit)
			{
				if(tindex > 1)
				{
					char *echo_strings[4] =
					{
						"Gromy cichna w oddali.\n\r",
						"Gromy cichna w oddali.\n\r",
						"Niebo rozjasnia sie, gdy burza "
							"cichnie.\n\r",
						"Blyskawice bledna i burza "
							"powoli slabnie.\n\r"
					};
					weath->echo = echo_strings[n];
					weath->echo_color = AT_GREY;
				}
			}
			else if(tindex < 2 && temp - dT > -weath_unit)
			{
				char *echo_strings[4] =
				{
					"Zimny deszcz zmienia sie w snieg.\n\r",
					"Zimny deszcz zmienia sie w snieg.\n\r",
					"Sniezne platki zajmuja miejsce "
						"chlodnych kropli deszczu.\n\r",
					"Padajacy deszcz powoli zamarza.\n\r"
				};
				weath->echo = echo_strings[n];
				weath->echo_color = AT_WHITE;
			}
			else if(tindex > 1 && temp - dT <= -weath_unit)
			{
				char *echo_strings[4] =
				{
					"Snieg zmienia sie w lodowaty descz.\n\r",
					"Snieg zmienia sie w lodowaty descz.\n\r",
					"Spada na ciebie zimny deszcz, "
						"gdy snieg sie roztapia.\n\r",
					"Snieg zmienia sie w wielka ulewe.\n\r"
				};
				weath->echo = echo_strings[n];
				weath->echo_color = AT_CYAN;
			}
			break;
			
		case 5:
			if(precip - dP <= 2*weath_unit)
			{
				if(tindex > 1)
				{
					char *echo_strings[4] =
					{
						"Blyskawice pojawiaja sie na "
							"niebie.\n\r",
						"Blyskawice pojawiaja sie na "
							"niebie.\n\r",
						"Blysk swiatla przecina "
							"niebo.\n\r",
						"Niebo blyska, a ziemia drzy "
							"w rytmie nadchodzacej "
							"burzy.\n\r"
					};
					weath->echo = echo_strings[n];
					weath->echo_color = AT_YELLOW;
				}
			}
			else if(tindex > 1 && temp - dT <= -weath_unit)
			{
				char *echo_strings[4] =
				{
					"The sky rumbles with thunder as "
						"the snow changes to rain.\n\r",
					"The sky rumbles with thunder as "
						"the snow changes to rain.\n\r",
					"The falling turns to freezing rain "
						"amidst flashes of "
						"lightning.\n\r",
					"The falling snow begins to melt as "
						"thunder crashes overhead.\n\r"
				};
				weath->echo = echo_strings[n];
				weath->echo_color = AT_WHITE;
			}
			else if(tindex < 2 && temp - dT > -weath_unit)
			{
				char *echo_strings[4] =
				{
					"The lightning stops as the rainstorm "
						"becomes a blinding "
						"blizzard.\n\r",
					"The lightning stops as the rainstorm "
						"becomes a blinding "
						"blizzard.\n\r",
					"The thunder dies off as the "
						"pounding rain turns to "
						"heavy snow.\n\r",
					"The cold rain turns to snow and "
						"the lightning stops.\n\r"
				};
				weath->echo = echo_strings[n];
				weath->echo_color = AT_CYAN;
			}
			break;
			
		default:
			bug("echo_weather: invalid precip index");
			weath->precip = 0;
			break;
	}

	return;
}

/*
 * get echo messages according to time changes...
 * some echoes depend upon the weather so an echo must be
 * found for each area
 * Last Modified: August 10, 1997
 * Fireblade
 */
void get_time_echo(WEATHER_DATA *weath)
{
	int n;
	int pindex;
	
	n = number_bits(2);
	pindex = (weath->precip + 3*weath_unit - 1)/weath_unit;
	weath->echo = NULL;
	weath->echo_color = AT_GREY;
	
	switch(time_info.hour)
	{
		case 5:
		{
			char *echo_strings[4] =
			{
				"Dzien sie rozpoczyna.\n\r",
				"Dzien sie rozpoczyna.\n\r",
				"Niebo powoli sie rozjasnia.\n\r",
				"Slonce powoli pojawia sie, rozpoczynajac nowy dzien.\n\r"
			};
			time_info.sunlight = SUN_RISE;
			weath->echo = echo_strings[n];
			weath->echo_color = AT_YELLOW;
			break;
		}
		case 6:
		{
			char *echo_strings[4] =
			{
				"Slonce pojawia sie na wschodzie.\n\r",
				"Slonce pojawia sie na wschodzie.\n\r",
				"Mgliste slonce pojawia sie na wschodzie.\n\r",
				"Wstaje nowy dzien i slonce powoli wznosi sie nad horyzontem.\n\r"
			};
			time_info.sunlight = SUN_LIGHT;
			weath->echo = echo_strings[n];
			weath->echo_color = AT_ORANGE;
			break;
		}
		case 12:
		{
			if(pindex > 0)
			{
				weath->echo = "Jest poludnie.\n\r";
			}
			else
			{
				char *echo_strings[2] =
				{
					"Intensywnosc promieni slonecznych, "
						"wskazuje na to, ze jest poludnie.\n\r",
					"Promienie sloneczne padaja pionowo w dol, "
						"wprost na Twe barki.\n\r"
				};
				weath->echo = echo_strings[n%2];
			}
			time_info.sunlight = SUN_LIGHT;
			weath->echo_color = AT_WHITE;
			break;
		}
		case 19:
		{
			char *echo_strings[4] =
			{
				"Slonce powoli znika na zachodzie.\n\r",
				"Czerwone slonce znika za horyzontem.\n\r",
				"Niebo przyjmuje pomaranczowy kolor, "
					"gdy slonce konczy swa wedrowke.\n\r",
				"Ostatnie promienie slonca oswietlaja Twa twarz"
					".\n\r"
			};
			time_info.sunlight = SUN_SET;
			weath->echo = echo_strings[n];
			weath->echo_color = AT_RED;
			break;
		}
		case 20:
		{
			if(pindex > 0)
			{
				char *echo_strings[2] =
				{
					"Rozpoczyna sie noc.\n\r",
					"Wokol Ciebie zapada polmrok.\n\r"
				};
				weath->echo = echo_strings[n%2];
			}
			else
			{
				char *echo_strings[2] =
				{
					"Ksiezyc delikatnie rozjasnia "
						"nocne niebo.\n\r",
					"Nocne niebo pokryte jest "
						"setkami migoczacych gwiazd.\n\r"
				};
				weath->echo = echo_strings[n%2];
			}
			time_info.sunlight = SUN_DARK;
			weath->echo_color = AT_DBLUE;
			break;
		}
	}
	
	return;
}

/*
 * update the time
 */
void time_update()
{
	AREA_DATA *pArea;
	DESCRIPTOR_DATA *d;
	WEATHER_DATA *weath;
	
	switch(++time_info.hour)
	{
		case 5:
		case 6:
		case 12:
		case 19:
		case 20:
			for(pArea = first_area; pArea;
				pArea = (pArea == last_area) ? first_build : pArea->next)
			{
				get_time_echo(pArea->weather);
			}
			
			for(d = first_descriptor; d; d = d->next)
			{
				if(d->connected == CON_PLAYING &&
					IS_OUTSIDE(d->character) &&
					IS_AWAKE(d->character))
				{
					weath = d->character->in_room->area->weather;
					if(!weath->echo)
						continue;
					set_char_color(weath->echo_color,
						d->character);
					ch_printf(d->character, weath->echo);
				}
			}
			break;
		case 24:
			time_info.hour = 0;
			time_info.day++;
			break;
	}
	
	if(time_info.day >= 30)
	{
		time_info.day = 0;
		time_info.month++;
	}
	
	if(time_info.month >= 17)
	{
		time_info.month = 0;
		time_info.year++;
	}
	
	return;
}
