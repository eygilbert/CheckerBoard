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
#include "CBstructs.h"
#include "CBconsts.h"
#include "coordinates.h"

int coortonumber(coor c, int gametype)
{
	// takes coordinates x and y, gametype, and returns the associated board number
	return(coorstonumber(c.x, c.y, gametype));
}

void coorstocoors(int *x, int *y, int invert, int mirror)
{
	// given coordinates x and y on the screen, this function converts them to internal
	// representation of the board based on whether the board is inverted or mirrored
	if (invert) {
		*x = 7 - *x;
		*y = 7 - *y;
	}

	if (mirror)
		*x = 7 - *x;
}

