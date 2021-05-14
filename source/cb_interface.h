#pragma once
#define NOMINMAX
#include <Windows.h>
#include <stdint.h>

/* Definitions shared between CheckerBoard and an engine.
 * Include this file in a program that interfaces
 * to CheckerBoard or a compatible engine.
 */

/* Piece types on Board8x8, used by getmove(), islegal(). */
#define CB_WHITE 1
#define CB_BLACK 2
#define CB_MAN 4
#define CB_KING 8

/* Return the opposite of color (CB_BLACK or CB_WHITE). */
#define CB_CHANGECOLOR(color) ((color) ^ (CB_WHITE | CB_BLACK))

/* Return values of getmove() */
#define CB_DRAW 0
#define CB_WIN 1
#define CB_LOSS 2
#define CB_UNKNOWN 3

/* enginecommand "get book", and "set book" values. */
#define CB_BOOK_NONE 0
#define CB_BOOK_ALL_KINDS_MOVES 1
#define CB_BOOK_GOOD_MOVES 2
#define CB_BOOK_BEST_MOVES 3

/* getmove() 'info' argument bit definitions. */
#define CB_RESET_MOVES 1
#define CB_EXACT_TIME 2

/* Bits 2 and 3 of 'info' argument define the incremental time control mode and units.
 * When shifted to a range from 0 to 3, their meanings are:
 *	0	incremental time is disabled
 *	1	incremental time is in units of 0.001 sec
 *	2	incremental time is in units of 0.01 sec
 *	3	incremental time is in units of 0.1 sec
 * The upper 16 bits in moreinfo are the remaining time, and the lower 16 bits are the time increment,
 * both in the units described above.
 */
#define CB_INCR_TIME_SHIFT 2

/* gametype definitions for response to enginecommand "get gametype". */
#define GT_ENGLISH 21
#define GT_ITALIAN 22
#define GT_SPANISH 24
#define GT_RUSSIAN 25
#define GT_BRAZILIAN 26
#define GT_CZECH 29

#define ENGINECOMMAND_REPLY_SIZE 1024

struct coor {
	int x;
	int y;
};

struct CBmove {
	int jumps;				/* number of pieces jumped. */
	int newpiece;			/* piece type that lands on the 'to' square. */
	int oldpiece;			/* piece type that disappears from the 'from' square. */
	coor from;				/* coordinates of the from piece in 8x8 notation. */
	coor to;				/* coordinates of the to piece in 8x8 notation. */
	coor path[12];			/* intermediate path coordinates of the moving pieces. */
							/* Starts at path[1]; path[0] is not used. */
	coor del[12];			/* squares of pieces that are captured. */
	int delpiece[12];		/* piece type of pieces that are captured. */
};

typedef int Board8x8[8][8];

/* Function pointer types of engine interface functions. */
typedef INT (WINAPI *CB_GETMOVE)(Board8x8 board, int color, double maxtime, char str[1024], int *playnow, int info, int unused, CBmove *move);
typedef INT (WINAPI *CB_GETSTRING)(char str[255]);		/* engine name, engine help */
typedef unsigned int (WINAPI *CB_GETGAMETYPE)(void);	/* return GT_ENGLISH, GT_ITALIAN, ... */
typedef INT (WINAPI *CB_ISLEGAL)(Board8x8 board, int color, int from, int to, CBmove *move);
typedef INT (WINAPI *CB_ENGINECOMMAND)(const char *command, char reply[ENGINECOMMAND_REPLY_SIZE]);


/*
 * If incremental time control is enabled, extract the increment and remaining times that are
 * packed into info and moreinfo, and the function return value is true.
 * Return false if incremental time control is disabled.
 */
inline bool get_incremental_times(int info, int moreinfo, double *increment, double *remaining)
{
	int tcmode;
	double mult[] = {0.001, 0.01, 0.1};

	tcmode = (info >> CB_INCR_TIME_SHIFT) & 3;
	if (tcmode == 0)
		return(false);

	*increment = (uint16_t)(moreinfo & 0xffff) * mult[tcmode - 1];
	*remaining = (uint16_t)(moreinfo >> 16) * mult[tcmode - 1];
	return(true);
}

/* 
 * Given a square number (1..32), return the x,y coordinates for a Board8x8.
 */
inline void numbertocoors(int number, int *x, int *y, int gametype)
{
	switch (gametype) {
	case GT_ITALIAN:
		number--;						// number e 0...31
		*y = number / 4;				// *y e 0...7
		*x = 2 * ((number % 4));		// *x e {0,2,4,6}
		if (((*y) % 2))					// adjust x on odd rows
			(*x)++;
		break;

	case GT_SPANISH:
		number--;
		*y = number / 4;
		*y = 7 - *y;
		*x = 2 * (3 - (number % 4));	// *x e {0,2,4,6}
		if (((*y) % 2))					// adjust x on odd rows
			(*x)++;
		break;

	case GT_CZECH:						// TODO: check that this is correct!
		number--;						// number e 0...31
		number = 33 - number;
		*y = number / 4;				// *y e 0...7
		*x = 2 * ((number % 4));		// *x e {0,2,4,6}
		if (((*y) % 2))					// adjust x on odd rows
			(*x)++;
		break;

	default:
		number--;
		*y = number / 4;
		*x = 2 * (3 - number % 4);
		if ((*y) % 2)
			(*x)++;
	}
}

inline void numbertocoors(int number, coor *c, int gametype)
{
	numbertocoors(number, &c->x, &c->y, gametype);
}

/*
 * Give the x,y coordinates for a Board8x8, return the square number (1..32).
 */
inline int coorstonumber(int x, int y, int gametype)
{
	int number;

	switch (gametype) {
	case GT_ITALIAN:
		// italian rules
		number = 1;
		number += 4 * y;
		number += x / 2;
		break;

	case GT_SPANISH:
		// spanish rules
		number = 1;
		number += 4 * (7 - y);
		number += (7 - x) / 2;
		break;

	case GT_CZECH:
		// TODO: make sure this is correct for czech rules
		number = 1;
		number += 4 * y;
		number += x / 2;
		number = 33 - number;
		break;

	default:
		number = 0;
		number += 4 * (y + 1);
		number -= x / 2;
	}

	return number;
}

