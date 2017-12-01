#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "bitboard.h"		/* needed for MAXMOVES */
#include "cb_interface.h"
#include "board46_intf.h"


#ifdef ITALIAN_RULES
static int sqmap[] = {
	 0,
	 5,  6,  7,  8,		/* squares 1 2 3 4 */
	10, 11, 12, 13,		/* squares 5 6 7 8 */
	14, 15, 16, 17,		/* squares 9 10 11 12 */
	19, 20, 21, 22,		/* squares 13, 14, 15 16 */
	23, 24, 25, 26,		/* squares 17, 18, 19 20 */
	28, 29, 30, 31,		/* squares 21, 22, 23 24 */
	32, 33, 34, 35,		/* squares 25, 26, 27 28 */
	37, 38, 39, 40		/* squares 29, 30, 31 32 */
};

/*
 * Return the 1-32 square number corresponding to a board46 index.
 */
int index46_to_square(int index46)
{
	int square;

	square = index46 - (index46 / 9);
	square -= 4;
	return(square + 1);
}

#else

static int sqmap[] = {
	 0,
	 8,  7,  6,  5,		/* squares 1 2 3 4 */
	13, 12, 11, 10,		/* squares 5 6 7 8 */
	17, 16, 15, 14,		/* squares 9 10 11 12 */
	22, 21, 20, 19,		/* squares 13, 14, 15 16 */
	26, 25, 24, 23,		/* squares 17, 18, 19 20 */
	31, 30, 29, 28,		/* squares 21, 22, 23 24 */
	35, 34, 33, 32,		/* squares 25, 26, 27 28 */
	40, 39, 38, 37		/* squares 29, 30, 31 32 */
};

/*
 * Return the 1-32 square number corresponding to a board46 index.
 */
int index46_to_square(int index46)
{
	int square, offset;

	square = index46 - (index46 / 9);
	square -= 5;
	offset = square % 4;
	square -= offset;
	offset = 3 - offset;
	square += offset;
	return(square + 1);
}
#endif

/*
 * Return the board46 index corresponding to a 1-32 square number.
 */
int square_to_index46(int square)
{
	return(sqmap[square]);
}


bool test_index_to_square()
{
	int sq, returnsq, index;

	for (sq = 1; sq <= 32; ++sq) {
		index = square_to_index46(sq);
		returnsq = index46_to_square(index);
		if (sq != returnsq)
			return(false);
	}
	return(true);
}


#define FREE 16
#define OFF_BOARD 0

void reset_board46(int board[46])
{
	int i;

	assert(test_index_to_square());
	for(i = 0; i < 46; ++i)
		board[i] = OFF_BOARD;
	for (i = 5 ; i <= 40; ++i)
   		board[i] = FREE;
	for(i = 9; i <= 36; i += 9)
		board[i] = OFF_BOARD;
}

void get_start_pos(int board46[46], int *color)
{
	int sq, index;

	reset_board46(board46);
	for (sq = 1; sq <= 12; ++sq) {
		index = square_to_index46(sq);
		board46[index] = CB_BLACK | CB_MAN;
		index = square_to_index46(sq + 20);
		board46[index] = CB_WHITE | CB_MAN;
	}

#ifdef ITALIAN_RULES
	*color = CB_WHITE;
#else
	*color = CB_BLACK;
#endif
}


int parse_fen(char *buf, int board46[46], int *ret_color)
{
	int square, square2, s, index;
	int color;
	int piece;
	char *lastp;

	if (*buf == '"')
		++buf;

	/* Get the color. */
	if (toupper(*buf) == 'B')
		*ret_color = CB_BLACK;
	else if (toupper(*buf) == 'W')
		*ret_color = CB_WHITE;
	else {
		printf("Bad color in parse_fen()\n");
		return(1);
	}

	reset_board46(board46);

	++buf;
	lastp = buf;
	if (*buf != ':') {
		printf("Missing ':' in parse_fen()\n");
		return(1);
	}

	while (*buf) {
		if (*buf == ':') {
			++buf;
			if (toupper(*buf) == 'W')
				color = CB_WHITE;
			else if (toupper(*buf) == 'B')
				color = CB_BLACK;
			else
				return(1);
			++buf;
		}
		piece = color | CB_MAN;
		if (toupper(*buf) == 'K') {
			piece = color | CB_KING;
			++buf;
		}
		for (square = 0; isdigit(*buf); ++buf)
			square = 10 * square + (*buf - '0');

		/* Check for range of square numbers. */
		square2 = square;
		if (*buf == '-' && isdigit(buf[1])) {
			++buf;
			for (square2 = 0; isdigit(*buf); ++buf)
				square2 = 10 * square2 + (*buf - '0');
		}

		if (square && square <= square2) {
			for (s = square; s <= square2; ++s) {
				index = square_to_index46(s);
				board46[index] = piece;
			}
		}
		if (*buf == ',')
			++buf;

		/* If we didn't advance in buf, we're done. */
		if (lastp == buf)
			break;
		lastp = buf;
	}
	return(0);
}


int print_fen_pieces(int board46[46], int color, char *buf)
{
	int len, sq, index;
	char *comma;

	comma = "";
	len = sprintf(buf, color == CB_BLACK ? ":B" : ":W");
	for (sq = 1; sq <= 32; ++sq) {
		index = square_to_index46(sq);
		if (board46[index] == (color | CB_KING)) {
			len += sprintf(buf + len, "%sK%d", comma, sq);
			comma = ",";
		}
		else if (board46[index] == (color | CB_MAN)) {
 			len += sprintf(buf + len, "%s%d", comma, sq);
			comma = ",";
		}
	}
	return(len);
}


void print_fen(int board46[46], int color, char *buf)
{
	int len;

	len = sprintf(buf, "%c", color == CB_BLACK ? 'B' : 'W');
	len += print_fen_pieces(board46, CB_BLACK, buf + len);
	len += print_fen_pieces(board46, CB_WHITE, buf + len);
}

