#pragma once
#include <memory.h>
#include <windows.h>
#include "standardheader.h"
#include "cb_interface.h"
#include "CB_movegen.h"

#define MAXMOVES 28

int getmovelist(int color, CBmove movelist[MAXMOVES], Board8x8 board, int *isjump);
