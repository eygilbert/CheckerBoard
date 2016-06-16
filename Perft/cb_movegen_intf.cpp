#include <windows.h>
#include "min_movegen.h"		/* needed for MAXMOVES */
#include "board.h"
#include "move_api.h"
#include "cb_interface.h"
#include "cb_movegen_intf.h"
#include "Fen.h"

static int map[] = {
	8,  7,  6,  5,		/* squares 0 1 2 3 */
	13, 12, 11, 10,		/* squares 4 5 6 7 */
	17, 16, 15, 14,		/* squares 8 9 10 11 */
	22, 21, 20, 19,		/* squares 12, 13, 14, 15 */
	26, 25, 24, 23,		/* squares 16, 17, 18, 19 */
	31, 30, 29, 28,		/* squares 20, 21, 22, 23 */
	35, 34, 33, 32,		/* squares 24, 25, 26, 27 */
	40, 39, 38, 37		/* squares 28, 29, 30, 31 */
};

static int italian_map[] = {
	 5,  6,  7,  8,		/* squares 0 1 2 3 */
	10, 11, 12, 13,		/* squares 4 5 6 7 */
	14, 15, 16, 17,		/* squares 8 9 10 11 */
	19, 20, 21, 22,		/* squares 12, 13, 14, 15 */
	23, 24, 25, 26,		/* squares 16, 17, 18, 19 */
	28, 29, 30, 31,		/* squares 20, 21, 22, 23 */
	32, 33, 34, 35,		/* squares 24, 25, 26, 27 */
	37, 38, 39, 40		/* squares 28, 29, 30, 31 */
};


/* Convert a 0-based square number (0..31) to a b46 array index. */
inline int square0_to_b46_index(int sq0)
{
#ifdef ITALIAN_RULES
	return(italian_map[sq0]);
#else
	return(map[sq0]);
#endif
}


#define FREE 16
#define OFF_BOARD 0

void board_to_b46(BOARD *board, int b[46])
{
	int sq0, index;
	BITBOARD mask;

   for (index = 0; index < 46; ++index)
		b[index] = OFF_BOARD;

	for (sq0 = 0; sq0 < NUMSQUARES; ++sq0) {
		index = square0_to_b46_index(sq0);
		mask = square0_to_bitmask(sq0);
		if (mask & board->black) {
			b[index] = CB_BLACK;
			if (mask & board->king)
				b[index] |= CB_KING;
			else
				b[index] |= CB_MAN;
		}
		else if (mask & board->white) {
			b[index] = CB_WHITE;
			if (mask & board->king)
				b[index] |= CB_KING;
			else
				b[index] |= CB_MAN;
		}
		else
			b[index] = FREE;
	}		
}


void board46_to_board(int b[46], BOARD *board)
{
	int sq0, index;
	BITBOARD mask;

	memset(board, 0, sizeof(BOARD));
	for (sq0 = 0; sq0 < NUMSQUARES; ++sq0) {
		index = square0_to_b46_index(sq0);
		mask = square0_to_bitmask(sq0);
		if (b[index] & CB_BLACK)
			board->black |= mask;
		if (b[index] & CB_WHITE)
			board->white |= mask;
		if (b[index] & CB_KING)
			board->king |= mask;
	}		
}


void get_start_pos(int board46[46], int *color)
{
	BOARD start_board = {0xfff, 0xfff00000, 0};

	board_to_b46(&start_board, board46);
#ifdef ITALIAN_RULES
	*color = CB_WHITE;
#else
	*color = CB_BLACK;
#endif
}


int parse_fen(char *fenstr, int board46[46], int *color)
{
	int status;
	BOARD bb;

	status = parse_fen(fenstr, &bb, color);
	board_to_b46(&bb, board46);
	*color = krcolor_to_cbcolor(*color);
	return(status);
}


void print_fen(int board46[46], int color, char *fenbuf)
{
	BOARD bb;

	board46_to_board(board46, &bb);
	print_fen(&bb, cbcolor_to_krcolor(color), fenbuf);
}


