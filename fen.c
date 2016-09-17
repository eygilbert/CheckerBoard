#include <windows.h>
#include <stdio.h>
#include "cb_interface.h"
#include "min_movegen.h"
#include "CBstructs.h"
#include "CheckerBoard.h"
#include "coordinates.h"
#include "fen.h"


/*
 * Return true if the string looks like a fen position.
 */
int is_fen(char *buf)
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
int FENtoboard8(int board[8][8], char *buf, int *poscolor, int gametype)
{
	int square, square2, s;
	int color, piece;
	int i, j;
	char *lastp;

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
	lastp = buf;
	while (*buf) {
		if (*buf == ':') {
			++buf;
			if (toupper(*buf) == 'W')
				color = CB_WHITE;
			else if (toupper(*buf) == 'B')
				color = CB_BLACK;
			else
				return(0);
			++buf;
		}

		piece = CB_MAN;
		if (toupper(*buf) == 'K') {
			piece = CB_KING;
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
				numbertocoors(s, &i, &j, gametype);
				board[i][j] = piece | color;
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


void board8toFEN(int board[8][8],char *p,int color, int gametype)
	{
	int i,j,number;
	char s[256];
	/* prints a FEN string into p derived from board */
	/* sample FEN string:
		"W:W18,20,23,K25:B02,06,09."*/
	sprintf(p,"");

	if(color==CB_BLACK)
		sprintf(s,"B:W");
	else
		sprintf(s,"W:W");
	strcat(p,s);
	for(j=0;j<=7;j++)
		{
		for(i=7;i>=0;i--)
			{
			sprintf(s,"");
			number=coorstonumber(i,j, gametype);
			if(board[i][j]==(CB_WHITE|CB_MAN))
				sprintf(s,"%i,",number);
			if(board[i][j]==(CB_WHITE|CB_KING))
				sprintf(s,"K%i,",number);
			strcat(p,s);
			}
		}
	/* remove last comma */
	p[strlen(p)-1]=0;
	sprintf(s,":B");
	strcat(p,s);
	for(j=0;j<=7;j++)
		{
		for(i=7;i>=0;i--)
			{
			sprintf(s,"");
			number=coorstonumber(i,j, gametype);
			if(board[i][j]==(CB_BLACK|CB_MAN))
				sprintf(s,"%i,",number);
			if(board[i][j]==(CB_BLACK|CB_KING))
				sprintf(s,"K%i,",number);
			strcat(p,s);
			}
		}
		p[strlen(p)-1]=0;
	}

