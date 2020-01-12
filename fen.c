#include <windows.h>
#include <cctype>
#include <stdio.h>
#include "cb_interface.h"
#include "CBstructs.h"
#include "CheckerBoard.h"
#include "coordinates.h"
#include "fen.h"

/*
 * Return true if the string looks like a fen position.
 */
int is_fen(const char *buf)
{
	while (*buf) {
		if (isspace(*buf))
			++buf;
		else if (*buf == '"')
			++buf;
		else
			break;
	}

	if ((toupper(*buf) == 'B' || toupper(*buf) == 'W') && buf[1] == ':')
		return(1);
	else
		return(0);
}

/*
 * Parse a FEN string, return the position in board and color.
 * Return 1 on success, 0 on failure.
 * Updated to PDN 3.0 (see http://pdn.fmjd.org/).
 *		- Don't expect a "." at the end of the FEN string.
 *		- Accept square number ranges. ex: B:B1-12:W21-32
 */
int FENtoboard8(Board8x8 board, const char *buf, int *poscolor, int gametype)
{
	int square, square2, s;
	int color, piece_type;
	int i, j;
	const char *lastp;

	/* Allow possible extraneous stuff at the beginning, since it is used by the clipboard paste handler. */
	lastp = strchr(buf, ':');
	if (!lastp || lastp == buf)
		return(0);

	/* Get the side-to-move color. */
	buf = lastp - 1;
	if (toupper(*buf) == 'B')
		*poscolor = CB_BLACK;
	else if (toupper(*buf) == 'W')
		*poscolor = CB_WHITE;
	else
		return(0);

	/* Reset board. */
	for (i = 0; i < 8; ++i) {
		for (j = 0; j < 8; ++j)
			board[i][j] = 0;
	}

	++buf;
	if (*buf != ':')
		return(0);

	++buf;
	lastp = buf;
	while (*buf) {
		piece_type = CB_MAN;
		if (*buf == '"') {
			++buf;
			continue;
		}
		if (std::toupper(*buf) == 'W') {
			color = CB_WHITE;
			++buf;
			continue;
		}
		if (std::toupper(*buf) == 'B') {
			color = CB_BLACK;
			++buf;
			continue;
		}
		if (std::toupper(*buf) == 'K') {
			piece_type = CB_KING;
			++buf;
		}
		for (square = 0; std::isdigit(*buf); ++buf)
			square = 10 * square + (*buf - '0');

		square2 = square;
		if (*buf == ',' || *buf == ':')
			++buf;

		else if (*buf == '-' && std::isdigit(buf[1])) {
			++buf;
			for (square2 = 0; std::isdigit(*buf); ++buf)
				square2 = 10 * square2 + (*buf - '0');
			if (*buf == ',' || *buf == ':')
				++buf;
		}


		if (square && square <= square2) {
			for (s = square; s <= square2; ++s) {
				numbertocoors(s, &i, &j, gametype);
				board[i][j] = piece_type | color;
			}
		}

		if (*buf == ',')
			++buf;

		/* If we didn't advance in buf, we're done. */
		if (lastp == buf)
			break;
		lastp = buf;
	}

	return(1);
}

void board8toFEN(const Board8x8 board, std::string &fenstr, int color, int gametype)
{
	int i, j, square;

	fenstr = color == CB_BLACK ? "B" : "W";

	/* Add the white pieces. */
	fenstr += ":W";
	for (j = 0; j <= 7; j++) {
		for (i = 7; i >= 0; i--) {
			square = coorstonumber(i, j, gametype);
			if (board[i][j] == (CB_WHITE | CB_MAN))
				fenstr += std::to_string(square) + ",";
			if (board[i][j] == (CB_WHITE | CB_KING))
				fenstr += "K" + std::to_string(square) + ",";
		}
	}

	/* remove last comma */
	if (fenstr[fenstr.size() - 1] == ',')
		fenstr.pop_back();

	/* Add the black pieces. */
	fenstr += ":B";
	for (j = 0; j <= 7; j++) {
		for (i = 7; i >= 0; i--) {
			square = coorstonumber(i, j, gametype);
			if (board[i][j] == (CB_BLACK | CB_MAN))
				fenstr += std::to_string(square) + ",";
			if (board[i][j] == (CB_BLACK | CB_KING))
				fenstr += "K" + std::to_string(square) + ",";
		}
	}

	/* remove last comma */
	if (fenstr[fenstr.size() - 1] == ',')
		fenstr.pop_back();
}

void board8toFEN(const Board8x8 board, char *fenstr, int color, int gametype)
{
	std::string fen;
	board8toFEN(board, fen, color, gametype);
	strcpy(fenstr, fen.c_str());
}

