#pragma once
#include <stdint.h>


struct pos {
	uint32_t bm;
	uint32_t bk;
	uint32_t wm;
	uint32_t wk;
};
#if 0
struct move {
	uint32_t bm;
	uint32_t bk;
	uint32_t wm;
	uint32_t wk;
};
#endif
void boardtobitboard(int b[8][8], pos *position);
void boardtocrbitboard(int b[8][8], pos *position);
void bitboardtoboard8(pos *p, int b[8][8]);
