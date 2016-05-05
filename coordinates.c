// coordinates.c
//
// part of checkerboard
//
// this module takes care of all kinds of coordinate transformations:
// from board number to x,y coordinates and vice versa, taking into 
// account what kind of game type is being played.

#include <windows.h>
#include "standardheader.h"
#include "cb_interface.h"
#include "min_movegen.h"
#include "CBstructs.h"
#include "CBconsts.h"
#include "coordinates.h"

int coortonumber(struct coor coor, int gametype)
	{
	// given board coordinates x and y, this function returns the board number in
   	// standard checkers notation 
	int number;

	switch (gametype)
		{
		case GT_ITALIAN:
			// italian rules
			number=1;
			number+=4*(coor.y);
			number+=(coor.x)/2;
			break;
		case GT_SPANISH:
			// spanish rules
			number=1;
			number+=4*(7-coor.y);
			number+= (7-coor.x) / 2;
			break;
		case GT_CZECH:
			// TODO: make sure this is correct for czech rules
			number=1;
			number+=4*(coor.y);
			number+=(coor.x)/2;
			number = 33-number;
			break;
		default:
			number=0;
			number+=4*(coor.y+1);
			number-=(coor.x/2);
		}
	return number;
   }

int coorstonumber(int x,int y, int gametype)
	{
	// takes coordinates x and y, gametype, and returns the associated board number
	struct coor c;
	c.x=x;
	c.y=y;
	return(coortonumber(c,gametype));
	}

void numbertocoors(int number,int *x, int *y, int gametype)
	{
	// given a board number this function returns the coordinates 
	// whoa, this has to be fixed for spanish / italian / etc.!
	switch (gametype)
		{
		case GT_ITALIAN:
			number--;				// number e 0...31
			*y=number/4;			// *y e 0...7
			*x=2*((number%4));		// *x e {0,2,4,6}
			if(((*y) % 2))			// adjust x on odd rows
				(*x)++;
			break;

		case GT_SPANISH:
			number--;
			*y = number/4;
			*y = 7-*y;
			*x=2*(3-(number%4));	// *x e {0,2,4,6}
			if(((*y) % 2))			// adjust x on odd rows
				(*x)++;
			break;

		case GT_CZECH:				// TODO: check that this is correct!
			number--;				// number e 0...31
			number = 33 - number;
			*y=number/4;			// *y e 0...7
			*x=2*((number%4));		// *x e {0,2,4,6}
			if(((*y) % 2))			// adjust x on odd rows
				(*x)++;
			break;

		default:
			number--;
			*y=number/4;
			*x=2*(3-number%4);
			if((*y) % 2)
				(*x)++;
		}
	}

void coorstocoors(int *x, int *y, int invert, int mirror)
	{
	// given coordinates x and y on the screen, this function converts them to internal
	// representation of the board based on whether the board is inverted or mirrored
	if(invert) 	
		{
		*x=7-*x;
		*y=7-*y;
		}
	if(mirror)	
		*x=7-*x;
	}
