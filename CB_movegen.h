#pragma once
#include <memory.h>
#include <windows.h>
#include "standardheader.h"
#include "cb_interface.h"
#include "CB_movegen.h"

#define MAXMOVES 64		/* More than the number of moves in any position with at most 12 pieces per side. */

int getmovelist(int color, CBmove movelist[MAXMOVES], Board8x8 board, int *isjump);
