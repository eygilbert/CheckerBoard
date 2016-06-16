#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "board.h"
#include "move_api.h"
#include "Fen.h"


int parse_fen(char *buf, BOARD *board, int *ret_color)
{
	int square, square2, s;
	int color;
	int king;
	char *lastp;

	if (*buf == '"')
		++buf;

	/* Get the color. */
	if (toupper(*buf) == 'B')
		*ret_color = BLACK;
	else if (toupper(*buf) == 'W')
		*ret_color = WHITE;
	else {
		printf("Bad color in parse_fen()\n");
		return(1);
	}

	memset(board, 0, sizeof(BOARD));

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
				color = WHITE;
			else if (toupper(*buf) == 'B')
				color = BLACK;
			else
				return(1);
			++buf;
		}
		king = 0;
		if (toupper(*buf) == 'K') {
			king = 1;
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
				board->pieces[color] |= square0_to_bitmask(s - 1);
				if (king)
					board->king |= square0_to_bitmask(s - 1);
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


void print_fen(BOARD *board, int color, char *buf)
{
	int len;
	unsigned int mask, sq;

	len = sprintf(buf, "%c", color == BLACK ? 'B' : 'W');

	/* Print the white men and kings. */
	len += sprintf(buf + len, ":W");
	for (mask = board->white; mask; ) {

		/* Peel off the lowest set bit in mask. */
		sq = mask & -(int)mask;
		if (sq & board->king)
			len += sprintf(buf + len, "K%d", 1 + bitmask_to_square0(sq));
		else
			len += sprintf(buf + len, "%d", 1 + bitmask_to_square0(sq));
		mask = mask & (mask - 1);
		if (mask)
			len += sprintf(buf + len, ",");
	}

	/* Print the black men and kings. */
	len += sprintf(buf + len, ":B");
	for (mask = board->black; mask; ) {

		/* Peel off the lowest set bit in mask. */
		sq = mask & -(int)mask;
		if (sq & board->king)
			len += sprintf(buf + len, "K%d", 1 + bitmask_to_square0(sq));
		else
			len += sprintf(buf + len, "%d", 1 + bitmask_to_square0(sq));
		mask = mask & (mask - 1);
		if (mask)
			len += sprintf(buf + len, ",");
	}
}


