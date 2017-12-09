#pragma once
#include <stdint.h>
#include "cb_interface.h"


struct pos {
	uint32_t bm;
	uint32_t bk;
	uint32_t wm;
	uint32_t wk;
};

void boardtobitboard(Board8x8 board, pos *position);
void boardtocrbitboard(Board8x8 board, pos *position);
void bitboardtoboard8(pos *p, Board8x8 board);
