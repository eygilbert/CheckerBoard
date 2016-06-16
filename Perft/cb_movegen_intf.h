void board_to_b46(BOARD *board, int b[46]);
void board46_to_board(int b[46], BOARD *board);
void domove(int b[46], struct move2 &move);
void undomove(int b[46], struct move2 &move);
void get_start_pos(int board46[46], int *color);
int parse_fen(char *fenstr, int board46[46], int *color);
void print_fen(int board46[46], int color, char *fenbuf);

struct move2 {
	short n;		/* number of entries in move array m[]. */
	int m[12];		/* bits 0 - 7: affected square (index into b[46]). */
					/* bits 8 - 15: piece on square before the move. */
					/* bits 16 - 23: piece on square after the move. */
					/* m[0] has the 'from' square info. */
					/* m[1] has the 'to' square info. */
					/* m[2] and up have the jumped squares info. */
};

int generatemovelist(int b[46], struct move2 movelist[MAXMOVES], int color);
int generatecapturelist(int b[46], struct move2 movelist[MAXMOVES], int color);


inline int krcolor_to_cbcolor(int krcolor)
{
	if (krcolor == BLACK)
		return(CB_BLACK);
	else
		return(CB_WHITE);
}


inline int cbcolor_to_krcolor(int cbcolor)
{
	if (cbcolor == CB_BLACK)
		return(BLACK);
	else
		return(WHITE);
}

