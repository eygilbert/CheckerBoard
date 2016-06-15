/*
 * Kingsrow Italian move generator.
 * This is an include file so that it can be compiled twice.
 * Once with SAVE_CAPTURE_PATH set to 0, for the normal fast
 * move generator, and once with it set to 1, so that
 * it saves the detailed capture data.
 */

typedef struct {
	int from_shift;
	int jumps_shift;
	BITBOARD move_mask;
	BITBOARD jump_mask;
	void (*black_man_jump_fn)(BOARD *, MOVELIST *, BITBOARD, CAPTURE_INFO *, int);
	void (*black_king_jump_fn)(BOARD *, MOVELIST *, BITBOARD, CAPTURE_INFO *, int, int, int);
	void (*white_man_jump_fn)(BOARD *, MOVELIST *, BITBOARD, CAPTURE_INFO *, int);
	void (*white_king_jump_fn)(BOARD *, MOVELIST *, BITBOARD, CAPTURE_INFO *, int, int, int);
} DIAG_MOVE;

void black_man_jump_even(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count);
void black_man_jump_odd(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count);
void black_king_jump_even(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count,
					int king_capture_count, int earliest_king);
void black_king_jump_odd(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count,
					int king_capture_count, int earliest_king);
void white_man_jump_even(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count);
void white_man_jump_odd(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count);
void white_king_jump_even(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count,
					int king_capture_count, int earliest_king);
void white_king_jump_odd(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count,
					int king_capture_count, int earliest_king);

static DIAG_MOVE fwd_tbl[] = {
	{LF_EVEN_MOVE_SHIFT, LF_EVEN_JUMP_SHIFT, LF_EVEN, LFJ_EVEN, 
		black_man_jump_even, black_king_jump_even, white_man_jump_even, white_king_jump_even},
	{RF_EVEN_MOVE_SHIFT, RF_EVEN_JUMP_SHIFT, RF_EVEN, RFJ_EVEN,
		black_man_jump_even, black_king_jump_even, white_man_jump_even, white_king_jump_even},
	{LF_ODD_MOVE_SHIFT, LF_ODD_JUMP_SHIFT, LF_ODD, LFJ_ODD,
		black_man_jump_odd, black_king_jump_odd, white_man_jump_odd, white_king_jump_odd},
	{RF_ODD_MOVE_SHIFT, RF_ODD_JUMP_SHIFT, RF_ODD, RFJ_ODD,
		black_man_jump_odd, black_king_jump_odd, white_man_jump_odd, white_king_jump_odd},
};

static DIAG_MOVE back_tbl[] = {
	{LB_EVEN_MOVE_SHIFT, LB_EVEN_JUMP_SHIFT, LB_EVEN, LBJ_EVEN,
		black_man_jump_even, black_king_jump_even, white_man_jump_even, white_king_jump_even},
	{RB_EVEN_MOVE_SHIFT, RB_EVEN_JUMP_SHIFT, RB_EVEN, RBJ_EVEN,
		black_man_jump_even, black_king_jump_even, white_man_jump_even, white_king_jump_even},
	{LB_ODD_MOVE_SHIFT, LB_ODD_JUMP_SHIFT, LB_ODD, LBJ_ODD,
		black_man_jump_odd, black_king_jump_odd, white_man_jump_odd, white_king_jump_odd},
	{RB_ODD_MOVE_SHIFT, RB_ODD_JUMP_SHIFT, RB_ODD, RBJ_ODD,
		black_man_jump_odd, black_king_jump_odd, white_man_jump_odd, white_king_jump_odd},
};


#if !SAVE_CAPTURE_PATH
/*
 * Italian version.
 */
int canjump(BOARD *board, int color)
{
	BITBOARD free;
	BITBOARD mask;
	BITBOARD bk, wk;
	BITBOARD bm, wm;

	bm = board->black & ~board->king;
	wm = board->white & ~board->king;
	if (color == BLACK) {

		/* Jumps to the left starting at even rows. */
		mask = ((((bm & LFJ_EVEN) << LF_EVEN_MOVE_SHIFT) &
						wm) << LF_EVEN_JUMP_SHIFT);

		/* Jumps to the right starting at even rows. */
		mask |= ((((bm & RFJ_EVEN) << RF_EVEN_MOVE_SHIFT) &
						wm) << RF_EVEN_JUMP_SHIFT);

		/* Jumps to the left starting at odd rows. */
		mask |= ((((bm & LFJ_ODD) << LF_ODD_MOVE_SHIFT) &
						wm) << LF_ODD_JUMP_SHIFT);

		/* Jumps to the right starting at odd rows. */
		mask |= ((((bm & RFJ_ODD) << RF_ODD_MOVE_SHIFT) &
						wm) << RF_ODD_JUMP_SHIFT);

		bk = board->black & board->king;
		if (bk) {

			/* Jumps to the left starting at even rows. */
			mask |= ((((bk & LFJ_EVEN) << LF_EVEN_MOVE_SHIFT) &
							board->white) << LF_EVEN_JUMP_SHIFT);

			/* Jumps to the right starting at even rows. */
			mask |= ((((bk & RFJ_EVEN) << RF_EVEN_MOVE_SHIFT) &
							board->white) << RF_EVEN_JUMP_SHIFT);

			/* Jumps to the left starting at odd rows. */
			mask |= ((((bk & LFJ_ODD) << LF_ODD_MOVE_SHIFT) &
							board->white) << LF_ODD_JUMP_SHIFT);

			/* Jumps to the right starting at odd rows. */
			mask |= ((((bk & RFJ_ODD) << RF_ODD_MOVE_SHIFT) &
							board->white) << RF_ODD_JUMP_SHIFT);

			/* Jumps back to the left starting at 
			 * even rows.
			 */
			mask |= ((((bk & LBJ_EVEN) >> LB_EVEN_MOVE_SHIFT) &
							board->white) >> LB_EVEN_JUMP_SHIFT);

			/* Jumps back to the right starting at 
			 * even rows.
			 */
			mask |= ((((bk & RBJ_EVEN) >> RB_EVEN_MOVE_SHIFT) & 
							board->white) >> RB_EVEN_JUMP_SHIFT);

			/* Jumps back to the left starting at 
			 * odd rows.
			 */
			mask |= ((((bk & LBJ_ODD) >> LB_ODD_MOVE_SHIFT) & 
							board->white) >> LB_ODD_JUMP_SHIFT);

			/* Jumps back to the right starting at 
			 * odd rows.
			 */
			mask |= ((((bk & RBJ_ODD) >> RB_ODD_MOVE_SHIFT) & 
							board->white) >> RB_ODD_JUMP_SHIFT);
		}
	}
	else {
		/* Jumps to the left starting at even rows. */
		mask = ((((wm & LBJ_EVEN) >> LB_EVEN_MOVE_SHIFT) &
						bm) >> LB_EVEN_JUMP_SHIFT);

		/* Jumps to the right starting at even rows. */
		mask |= ((((wm & RBJ_EVEN) >> RB_EVEN_MOVE_SHIFT) &
						bm) >> RB_EVEN_JUMP_SHIFT);

		/* Jumps to the left starting at odd rows. */
		mask |= ((((wm & LBJ_ODD) >> LB_ODD_MOVE_SHIFT) & 
						bm) >> LB_ODD_JUMP_SHIFT);

		/* Jumps to the right starting at odd rows. */
		mask |= ((((wm & RBJ_ODD) >> RB_ODD_MOVE_SHIFT) & 
						bm) >> RB_ODD_JUMP_SHIFT);

		wk = board->white & board->king;
		if (wk) {

			/* Jumps to the left starting at even rows. */
			mask |= ((((wk & LBJ_EVEN) >> LB_EVEN_MOVE_SHIFT) &
							board->black) >> LB_EVEN_JUMP_SHIFT);

			/* Jumps to the right starting at even rows. */
			mask |= ((((wk & RBJ_EVEN) >> RB_EVEN_MOVE_SHIFT) &
							board->black) >> RB_EVEN_JUMP_SHIFT);

			/* Jumps to the left starting at odd rows. */
			mask |= ((((wk & LBJ_ODD) >> LB_ODD_MOVE_SHIFT) & 
							board->black) >> LB_ODD_JUMP_SHIFT);

			/* Jumps to the right starting at odd rows. */
			mask |= ((((wk & RBJ_ODD) >> RB_ODD_MOVE_SHIFT) & 
							board->black) >> RB_ODD_JUMP_SHIFT);

			/* Jumps forward to the left starting at 
			 * even rows.
			 */
			mask |= ((((wk & LFJ_EVEN) << LF_EVEN_MOVE_SHIFT) & 
							board->black) << LF_EVEN_JUMP_SHIFT);

			/* Jumps forward to the right starting at 
			 * even rows.
			 */
			mask |= ((((wk & RFJ_EVEN) << RF_EVEN_MOVE_SHIFT) & 
							board->black) << RF_EVEN_JUMP_SHIFT);

			/* Jumps forward to the left starting at 
			 * odd rows.
			 */
			mask |= ((((wk & LFJ_ODD) << LF_ODD_MOVE_SHIFT) & 
							board->black) << LF_ODD_JUMP_SHIFT);

			/* Jumps forward to the right starting at 
			 * odd rows.
			 */
			mask |= ((((wk & RFJ_ODD) << RF_ODD_MOVE_SHIFT) & 
							board->black) << RF_ODD_JUMP_SHIFT);
		}
	}
	free = ~(board->black | board->white);
	if (mask & free)
		return(1);
	return(0);
}


/*
 * Build a movelist of non-jump moves.
 * Return the number of moves in the list.
 */
int build_nonjump_list(BOARD *board, MOVELIST *movelist, int color)
{
	int t;
	BITBOARD free;
	BITBOARD mask;
	BITBOARD bk, wk;
	BITBOARD from, to;
	DIAG_MOVE *m;
	BOARD *mlp;

	movelist->count = 0;
	free = ~(board->black | board->white);
	if (color == BLACK) {
		for (t = 0, m = fwd_tbl; t < ARRAY_SIZE(fwd_tbl); ++t, ++m) {
			mask = ((board->black & m->move_mask) << m->from_shift) & free;
			for ( ; mask; mask = mask & (mask - 1)) {

				/* Peel off the lowest set bit in mask. */
				to = mask & -(int)mask;
				from = to >> m->from_shift;

				/* Add it to the movelist. First clear the from square. */
				mlp = movelist->board + movelist->count++;
				mlp->black = board->black & ~from;
				mlp->white = board->white;
				mlp->king = board->king & ~from;

				/* Put him on the square where he lands. */
				mlp->black |= to;

				/* See if he was a king. */
				if (board->king & from)
					mlp->king |= to;

				/* See if this made him a king. */
				else if (to & BLACK_KING_RANK_MASK)
					mlp->king |= to;
			}
		}

		bk = board->black & board->king;
		if (bk) {
			for (t = 0, m = back_tbl; t < ARRAY_SIZE(back_tbl); ++t, ++m) {
				mask = ((bk & m->move_mask) >> m->from_shift) & free;
				for ( ; mask; mask = mask & (mask - 1)) {

					/* Peel off the lowest set bit in mask. */
					to = mask & -(int)mask;
					from = to << m->from_shift;

					/* Add it to the movelist. First clear the from square. */
					mlp = movelist->board + movelist->count++;
					mlp->black = board->black & ~from;
					mlp->white = board->white;
					mlp->king = board->king & ~from;

					/* Put him on the square where he lands. */
					mlp->black |= to;

					/* He was a king. */
					mlp->king |= to;
				}
			}
		}
	}
	else {
		for (t = 0, m = back_tbl; t < ARRAY_SIZE(back_tbl); ++t, ++m) {
			mask = ((board->white & m->move_mask) >> m->from_shift) & free;
			for ( ; mask; mask = mask & (mask - 1)) {

				/* Peel off the lowest set bit in mask. */
				to = mask & -(int)mask;
				from = to << m->from_shift;

				/* Add it to the movelist. First clear the from square. */
				mlp = movelist->board + movelist->count++;
				mlp->white = board->white & ~from;
				mlp->black = board->black;
				mlp->king = board->king & ~from;

				/* Put him on the square where he lands. */
				mlp->white |= to;

				/* See if he was a king. */
				if (board->king & from)
					mlp->king |= to;

				/* See if this made him a king. */
				else if (to & WHITE_KING_RANK_MASK)
					mlp->king |= to;
			}
		}

		wk = board->white & board->king;
		if (wk) {
			for (t = 0, m = fwd_tbl; t < ARRAY_SIZE(fwd_tbl); ++t, ++m) {
				mask = ((wk & m->move_mask) << m->from_shift) & free;
				for ( ; mask; mask = mask & (mask - 1)) {

					/* Peel off the lowest set bit in mask. */
					to = mask & -(int)mask;
					from = to >> m->from_shift;

					/* Add it to the movelist. First clear the from square. */
					mlp = movelist->board + movelist->count++;
					mlp->white = board->white & ~from;
					mlp->black = board->black;
					mlp->king = board->king & ~from;

					/* Put him on the square where he lands. */
					mlp->white |= to;

					/* He was a king. */
					mlp->king |= to;
				}
			}
		}
	}
	return(movelist->count);
}
#endif


/*
 * Build a movelist of jump moves.
 * Return the number of moves in the list.
 */
#if SAVE_CAPTURE_PATH
int build_jump_list_path(BOARD *board, MOVELIST *movelist, int color, CAPTURE_INFO cap[KRMAXMOVES])
{
#else
int build_jump_list(BOARD *board, MOVELIST *movelist, int color)
{
	CAPTURE_INFO cap[KRMAXMOVES];
#endif
	int i;
	BITBOARD free;
	BITBOARD mask;
	BITBOARD bk, wk;
	BITBOARD from, jumps, lands;
	DIAG_MOVE *m;
	BOARD jboard;
	int king_capture_count;
	int earliest_king;

	movelist->count = 0;
	free = ~(board->black | board->white);
	if (color == BLACK) {
		for (i = 0, m = fwd_tbl; i < ARRAY_SIZE(fwd_tbl); ++i, ++m) {
			mask = ((((board->black & m->jump_mask) << m->from_shift)
								& board->white) << m->jumps_shift) & free;
			for ( ; mask; mask = mask & (mask - 1)) {

				/* Peel off the lowest set bit in mask. */
				lands = mask & -(int)mask;
				jumps = lands >> m->jumps_shift;
				from = jumps >> m->from_shift;

				/* Men cannot jump kings in Italian checkers. */
				if (from & ~board->king)
					if (jumps & board->king)
						continue;

				/* Found a jump.  Log this part of the move,
				 * and make the move on a copy of the board to be
				 * passed along to the recursion function.
				 * First clear the from square and jumped squares.
				 */
				jboard.black = board->black & ~from;
				jboard.white = board->white & ~jumps;
				jboard.king = board->king & ~jumps;

				/* Put him on the square where he lands. */
				jboard.black |= lands;

				/* See if he is a king.  He could either be a man or a king
				 * because we were moving forward.
				 */
				if (board->king & from) {
					jboard.king &= ~from;
					jboard.king |= lands;
					if (jumps & board->king) {
						king_capture_count = 1;
						earliest_king = 1;
					}
					else {
						king_capture_count = 0;
						earliest_king = 0;
					}
#if SAVE_CAPTURE_PATH
					cap[movelist->count].path[0] = jboard;
#endif
					(*m->black_king_jump_fn)(&jboard, movelist, lands,
								cap, 1, king_capture_count, earliest_king);
				}
				else {
					/* Man jumping.  See if this made him a king. */
					if (lands & BLACK_KING_RANK_MASK) {
						jboard.king |= lands;

						/* This terminates the jump. */
						cap[movelist->count].capture_count = 1;
						cap[movelist->count].earliest_king = 0;
						cap[movelist->count].king_capturing = 0;
						cap[movelist->count].king_capture_count = 0;
#if SAVE_CAPTURE_PATH
						cap[movelist->count].path[0] = jboard;
#endif
						movelist->board[movelist->count++] = jboard;
					}
					else {
						/* Man jumping a man. */
#if SAVE_CAPTURE_PATH
						cap[movelist->count].path[0] = jboard;
#endif
						(*m->black_man_jump_fn)(&jboard, movelist, lands, cap, 1);
					}
				}
			}
		}			
				
		bk = board->black & board->king;
		if (bk) {
			for (i = 0, m = back_tbl; i < ARRAY_SIZE(back_tbl); ++i, ++m) {
				mask = ((((bk & m->jump_mask) >> m->from_shift) & 
								board->white) >> m->jumps_shift) & free;

				for ( ; mask; mask = mask & (mask - 1)) {

					/* Peel off the lowest set bit in mask. */
					lands = mask & -(int)mask;
					jumps = lands << m->jumps_shift;
					from = jumps << m->from_shift;

					/* Found a jump.  Log this part of the move,
					 * and make the move on a copy of the board to be
					 * passed along to the recursion function.
					 * We know this piece is a king because we jumped backwards.
					 */
					jboard.black = board->black & ~from;
					jboard.white = board->white & ~jumps;
					jboard.king = (board->king | lands) & ~jumps;

					/* Put him on the square where he lands. */
					jboard.king &= ~from;
					jboard.black |= lands;

					if (jumps & board->king) {
						king_capture_count = 1;
						earliest_king = 1;
					}
					else {
						king_capture_count = 0;
						earliest_king = 0;
					}
#if SAVE_CAPTURE_PATH
					cap[movelist->count].path[0] = jboard;
#endif
					(*m->black_king_jump_fn)(&jboard, movelist, lands,
								cap, 1, king_capture_count, earliest_king);
				}
			}			
		}
	}
	else {
		for (i = 0, m = back_tbl; i < ARRAY_SIZE(back_tbl); ++i, ++m) {
			mask = ((((board->white & m->jump_mask) >> m->from_shift) &
							board->black) >> m->jumps_shift) & free;

			for ( ; mask; mask = mask & (mask - 1)) {

				/* Peel off the lowest set bit in mask. */
				lands = mask & -(int)mask;
				jumps = lands << m->jumps_shift;
				from = jumps << m->from_shift;

				/* Men cannot jump kings in Italian checkers. */
				if (from & ~board->king)
					if (jumps & board->king)
						continue;

				/* Found a jump.  Log this part of the move,
				 * and make the move on a copy of the board to be
				 * passed along to the recursion function.
				 * First clear the from square and jumped squares.
				 */
				jboard.white = board->white & ~from;
				jboard.black = board->black & ~jumps;
				jboard.king = board->king & ~jumps;

				/* Put him on the square where he lands. */
				jboard.white |= lands;

				/* See if he is a king.  He could either be a man or a king
				 * because we were moving forward.
				 */
				if (board->king & from) {
					jboard.king &= ~from;
					jboard.king |= lands;
					if (jumps & board->king) {
						king_capture_count = 1;
						earliest_king = 1;
					}
					else {
						king_capture_count = 0;
						earliest_king = 0;
					}
#if SAVE_CAPTURE_PATH
					cap[movelist->count].path[0] = jboard;
#endif
					(*m->white_king_jump_fn)(&jboard, movelist, lands,
								cap, 1, king_capture_count, earliest_king);
				}
				else {
					/* See if this made him a king. */
					if (lands & WHITE_KING_RANK_MASK) {
						jboard.king |= lands;

						/* This terminates the jump. */
						cap[movelist->count].capture_count = 1;
						cap[movelist->count].earliest_king = 0;
						cap[movelist->count].king_capturing = 0;
						cap[movelist->count].king_capture_count = 0;
#if SAVE_CAPTURE_PATH
						cap[movelist->count].path[0] = jboard;
#endif
						movelist->board[movelist->count++] = jboard;
					}
					else {
						/* Man jumping a man. */
#if SAVE_CAPTURE_PATH
						cap[movelist->count].path[0] = jboard;
#endif
						(*m->white_man_jump_fn)(&jboard, movelist, lands,
											cap, 1);
					}
				}
			}
		}

		wk = board->white & board->king;
		if (wk) {
			for (i = 0, m = fwd_tbl; i < ARRAY_SIZE(fwd_tbl); ++i, ++m) {
				mask = ((((wk & m->jump_mask) << m->from_shift) &
								board->black) << m->jumps_shift) & free;
				for ( ; mask; mask = mask & (mask - 1)) {

					/* Peel off the lowest set bit in mask. */
					lands = mask & -(int)mask;
					jumps = lands >> m->jumps_shift;
					from = jumps >> m->from_shift;

					/* Found a jump.  Log this part of the move,
					 * and make the move on a copy of the board to be
					 * passed along to the recursion function.
					 * We know this piece is a king because we jumped backwards.
					 */
					jboard.white = board->white & ~from;
					jboard.black = board->black & ~jumps;
					jboard.king = (board->king | lands) & ~jumps;

					/* Put him on the square where he lands. */
					jboard.king &= ~from;
					jboard.white |= lands;

					if (jumps & board->king) {
						king_capture_count = 1;
						earliest_king = 1;
					}
					else {
						king_capture_count = 0;
						earliest_king = 0;
					}
#if SAVE_CAPTURE_PATH
					cap[movelist->count].path[0] = jboard;
#endif
					(*m->white_king_jump_fn)(&jboard, movelist, lands,
								cap, 1, king_capture_count, earliest_king);
				}
			}
		}
	}

	/* Filter the capture list by Italian rules of precedence.
	 *	1. The maximum number of pieces must be captured.
	 *	2. If several are the same in 1, it must be done with the king.
	 *	3. If several are the same in 1 and 2, it must take as many kings as possible.
	 *	4. If several are the same in 1, 2, and 3, it must take each king as early as possible.
	 */
	if (movelist->count > 1) {

		/* Filter by rule 1. */
		for (i = 1; i < movelist->count; ++i) {
			if (cap[i].capture_count > cap[0].capture_count) {

				/* This move has higher precedence than those before it.
				 * Move it and those above it down to index 0.
				 */
				memmove(movelist->board, movelist->board + i,
							(movelist->count - i) * sizeof(movelist->board[0]));
				memmove(cap, cap + i, 
							(movelist->count - i) * sizeof(cap[0]));
				movelist->count -= i;
				i = 0;
			}
			else if (cap[i].capture_count < cap[0].capture_count) {

				/* This move has lower precedence than those before it, delete it. */
				memmove(movelist->board + i, movelist->board + i + 1,
							(movelist->count - i - 1) * sizeof(movelist->board[0]));
				memmove(cap + i, cap + i + 1, 
							(movelist->count - i - 1) * sizeof(cap[0]));
				--movelist->count;
				--i;
			}
		}

		/* Filter by rule 2. */
		for (i = 1; i < movelist->count; ++i) {
			if (cap[i].king_capturing > cap[0].king_capturing) {

				/* This move has higher precedence than those before it.
				 * Move it and those above it down to index 0.
				 */
				memmove(movelist->board, movelist->board + i,
							(movelist->count - i) * sizeof(movelist->board[0]));
				memmove(cap, cap + i, 
							(movelist->count - i) * sizeof(cap[0]));
				movelist->count -= i;
				i = 0;
			}
			else if (cap[i].king_capturing < cap[0].king_capturing) {

				/* This move has lower precedence than those before it, delete it. */
				memmove(movelist->board + i, movelist->board + i + 1,
							(movelist->count - i - 1) * sizeof(movelist->board[0]));
				memmove(cap + i, cap + i + 1, 
							(movelist->count - i - 1) * sizeof(cap[0]));
				--movelist->count;
				--i;
			}
		}

		/* Filter by rule 3. */
		for (i = 1; i < movelist->count; ++i) {
			if (cap[i].king_capture_count > cap[0].king_capture_count) {

				/* This move has higher precedence than those before it.
				 * Move it and those above it down to index 0.
				 */
				memmove(movelist->board, movelist->board + i,
							(movelist->count - i) * sizeof(movelist->board[0]));
				memmove(cap, cap + i, 
							(movelist->count - i) * sizeof(cap[0]));
				movelist->count -= i;
				i = 0;
			}
			else if (cap[i].king_capture_count < cap[0].king_capture_count) {

				/* This move has lower precedence than those before it, delete it. */
				memmove(movelist->board + i, movelist->board + i + 1,
							(movelist->count - i - 1) * sizeof(movelist->board[0]));
				memmove(cap + i, cap + i + 1, 
							(movelist->count - i - 1) * sizeof(cap[0]));
				--movelist->count;
				--i;
			}
		}

		/* Filter by rule 4. */
		for (i = 1; i < movelist->count; ++i) {
			if (cap[i].earliest_king < cap[0].earliest_king) {

				/* This move has higher precedence than those before it.
				 * Move it and those above it down to index 0.
				 */
				memmove(movelist->board, movelist->board + i,
							(movelist->count - i) * sizeof(movelist->board[0]));
				memmove(cap, cap + i, 
							(movelist->count - i) * sizeof(cap[0]));
				movelist->count -= i;
				i = 0;
			}
			else if (cap[i].earliest_king > cap[0].earliest_king) {

				/* This move has lower precedence than those before it, delete it. */
				memmove(movelist->board + i, movelist->board + i + 1,
							(movelist->count - i - 1) * sizeof(movelist->board[0]));
				memmove(cap + i, cap + i + 1, 
							(movelist->count - i - 1) * sizeof(cap[0]));
				--movelist->count;
				--i;
			}
		}
	}
	return(movelist->count);
}


static inline void assign_path(BOARD *board, CAPTURE_INFO *cap,
							 int capture_count, int movecount)
{
#if SAVE_CAPTURE_PATH
	cap[movecount].path[capture_count] = *board;
#endif
}


static inline void copy_path(CAPTURE_INFO *cap, MOVELIST *movelist)
{
#if SAVE_CAPTURE_PATH
	/* Copy the path to the next potential jump move. */
	memcpy(cap[movelist->count + 1].path, cap[movelist->count].path, sizeof(cap[0].path));
#endif
}


#define BLACK_KING_JUMP(mask, from_shift, jumps_shift, shift_oper, func, \
	 cap, capture_count, king_capture_count, earliest_king) \
	jumps = ((from & mask) shift_oper from_shift) & board->white; \
	lands = (jumps shift_oper jumps_shift) & free; \
	if (lands) { \
		int kcc, ek; \
\
		if (jumps & board->king) { \
			kcc = king_capture_count + 1; \
			ek = earliest_king + (1 << capture_count); \
		} \
		else { \
			kcc = king_capture_count; \
			ek = earliest_king; \
		} \
\
		/* Found a jump. \
		 * Make the move on a copy of the board to be \
		 * passed along to the recursion function. \
		 * First clear the from square and jumped squares. \
		 */ \
		jboard.black = board->black & ~(from | jumps); \
		jboard.white = board->white & ~(from | jumps); \
		jboard.king = board->king & ~(from | jumps); \
\
		/* Put him on the square where he lands. */ \
		jboard.black |= lands; \
		jboard.king |= lands; \
		assign_path(&jboard, cap, capture_count, movelist->count); \
		func(&jboard, movelist, lands, cap, capture_count + 1, kcc, ek); \
		found_jump = 1; \
	}

/*
 * We're in the middle of a jump on even rows.
 * If we've reached the end of the jump, then add this board to the movelist.
 * If not, call again recursively.
 */
static void black_king_jump_even(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count,
					int king_capture_count, int earliest_king)
{
	BITBOARD jumps;
	BITBOARD lands;
	BITBOARD free;
	int found_jump;
	BOARD jboard;

	found_jump = 0;
	free = ~(board->black | board->white);

	BLACK_KING_JUMP(LFJ_EVEN, LF_EVEN_MOVE_SHIFT, LF_EVEN_JUMP_SHIFT, <<, black_king_jump_even,
				cap, capture_count, king_capture_count, earliest_king);
	BLACK_KING_JUMP(RFJ_EVEN, RF_EVEN_MOVE_SHIFT, RF_EVEN_JUMP_SHIFT, <<, black_king_jump_even,
				cap, capture_count, king_capture_count, earliest_king);

	BLACK_KING_JUMP(LBJ_EVEN, LB_EVEN_MOVE_SHIFT, LB_EVEN_JUMP_SHIFT, >>, black_king_jump_even,
				cap, capture_count, king_capture_count, earliest_king);
	BLACK_KING_JUMP(RBJ_EVEN, RB_EVEN_MOVE_SHIFT, RB_EVEN_JUMP_SHIFT, >>, black_king_jump_even,
				cap, capture_count, king_capture_count, earliest_king);

	/* If we didn't find any move jumps, then add the board that was
	 * passed in to the movelist.
	 */
	if (!found_jump) {
		cap[movelist->count].capture_count = capture_count;
		cap[movelist->count].king_capturing = 1;
		cap[movelist->count].king_capture_count = king_capture_count;
		cap[movelist->count].earliest_king = earliest_king;

		/* Copy the path to the next potential jump move. */
		copy_path(cap, movelist);
		movelist->board[movelist->count++] = *board;
	}
}


/*
 * We're in the middle of a jump on odd rows.
 * If we've reached the end of the jump, then add this board to the movelist.
 * If not, call again recursively.
 */
static void black_king_jump_odd(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count,
					int king_capture_count, int earliest_king)
{
	BITBOARD jumps;
	BITBOARD lands;
	BITBOARD free;
	int found_jump;
	BOARD jboard;

	found_jump = 0;
	free = ~(board->black | board->white);

	BLACK_KING_JUMP(LFJ_ODD, LF_ODD_MOVE_SHIFT, LF_ODD_JUMP_SHIFT, <<, black_king_jump_odd,
				cap, capture_count, king_capture_count, earliest_king);
	BLACK_KING_JUMP(RFJ_ODD, RF_ODD_MOVE_SHIFT, RF_ODD_JUMP_SHIFT, <<, black_king_jump_odd,
				cap, capture_count, king_capture_count, earliest_king);

	BLACK_KING_JUMP(LBJ_ODD, LB_ODD_MOVE_SHIFT, LB_ODD_JUMP_SHIFT, >>, black_king_jump_odd,
				cap, capture_count, king_capture_count, earliest_king);
	BLACK_KING_JUMP(RBJ_ODD, RB_ODD_MOVE_SHIFT, RB_ODD_JUMP_SHIFT, >>, black_king_jump_odd,
				cap, capture_count, king_capture_count, earliest_king);

	/* If we didn't find any move jumps, then add the board that was
	 * passed in to the movelist.
	 */
	if (!found_jump) {
		cap[movelist->count].capture_count = capture_count;
		cap[movelist->count].king_capturing = 1;
		cap[movelist->count].king_capture_count = king_capture_count;
		cap[movelist->count].earliest_king = earliest_king;

		/* Copy the path to the next potential jump move. */
		copy_path(cap, movelist);
		movelist->board[movelist->count++] = *board;
	}
}


#define BLACK_MAN_JUMP(mask, from_shift, jumps_shift, func, cap, capture_count) \
	jumps = ((from & mask) << from_shift) & board->white & ~board->king; \
	lands = (jumps << jumps_shift) & free; \
	if (lands) { \
\
		/* Found a jump. \
		 * Make the move on a copy of the board to be \
		 * passed along to the recursion function. \
		 * First clear the from square and jumped squares. \
		 */ \
		jboard.black = board->black & ~from; \
		jboard.white = board->white & ~jumps; \
		jboard.king = board->king & ~jumps; \
\
		/* Put him on the square where he lands. */ \
		jboard.black |= lands; \
\
		/* See if this made him a king. */ \
		if (lands & BLACK_KING_RANK_MASK) { \
			jboard.king |= lands; \
\
			/* This terminates the jump. */ \
			cap[movelist->count].capture_count = capture_count + 1; \
			cap[movelist->count].king_capturing = 0; \
			cap[movelist->count].king_capture_count = 0; \
			cap[movelist->count].earliest_king = 0; \
			assign_path(&jboard, cap, capture_count, movelist->count); \
			copy_path(cap, movelist); \
			movelist->board[movelist->count++] = jboard; \
		} \
		else { \
			assign_path(&jboard, cap, capture_count, movelist->count); \
			func(&jboard, movelist, lands, cap, capture_count + 1); \
		} \
		found_jump = 1; \
	}

	
	
/*
 * We're in the middle of a jump on even rows.
 * If we've reached the end of the jump, then add this board to the movelist.
 * If not, call again recursively.
 */
static void black_man_jump_even(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap,	int capture_count)

{
	BITBOARD jumps;
	BITBOARD lands;
	BITBOARD free;
	int found_jump;
	BOARD jboard;

	found_jump = 0;
	free = ~(board->black | board->white);

	BLACK_MAN_JUMP(LFJ_EVEN, LF_EVEN_MOVE_SHIFT, LF_EVEN_JUMP_SHIFT, black_man_jump_even, cap, capture_count);
	BLACK_MAN_JUMP(RFJ_EVEN, RF_EVEN_MOVE_SHIFT, RF_EVEN_JUMP_SHIFT, black_man_jump_even, cap, capture_count);

	/* If we didn't find any move jumps, then add the board that was
	 * passed in to the movelist.
	 */
	if (!found_jump) {
		cap[movelist->count].capture_count = capture_count;
		cap[movelist->count].king_capturing = 0;
		cap[movelist->count].king_capture_count = 0;
		cap[movelist->count].earliest_king = 0;

		/* Copy the path to the next potential jump move. */
		copy_path(cap, movelist);
		movelist->board[movelist->count++] = *board;
	}
}


/*
 * We're in the middle of a jump on odd rows.
 * If we've reached the end of the jump, then add this board to the movelist.
 * If not, call again recursively.
 */
static void black_man_jump_odd(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap,	int capture_count)
{
	BITBOARD jumps;
	BITBOARD lands;
	BITBOARD free;
	int found_jump;
	BOARD jboard;

	found_jump = 0;
	free = ~(board->black | board->white);

	BLACK_MAN_JUMP(LFJ_ODD, LF_ODD_MOVE_SHIFT, LF_ODD_JUMP_SHIFT, black_man_jump_odd, cap, capture_count);
	BLACK_MAN_JUMP(RFJ_ODD, RF_ODD_MOVE_SHIFT, RF_ODD_JUMP_SHIFT, black_man_jump_odd, cap, capture_count);

	/* If we didn't find any move jumps, then add the board that was
	 * passed in to the movelist.
	 */
	if (!found_jump) {
		cap[movelist->count].capture_count = capture_count;
		cap[movelist->count].king_capturing = 0;
		cap[movelist->count].king_capture_count = 0;
		cap[movelist->count].earliest_king = 0;

		/* Copy the path to the next potential jump move. */
		copy_path(cap, movelist);
		movelist->board[movelist->count++] = *board;
	}
}


#define WHITE_KING_JUMP(mask, from_shift, jumps_shift, shift_oper, func, \
	 cap, capture_count, king_capture_count, earliest_king) \
	jumps = ((from & mask) shift_oper from_shift) & board->black; \
	lands = (jumps shift_oper jumps_shift) & free; \
	if (lands) { \
		int kcc, ek; \
\
		if (jumps & board->king) { \
			kcc = king_capture_count + 1; \
			ek = earliest_king + (1 << capture_count); \
		} \
		else { \
			kcc = king_capture_count; \
			ek = earliest_king; \
		} \
\
		/* Found a jump. \
		 * Make the move on a copy of the board to be \
		 * passed along to the recursion function. \
		 * First clear the from square and jumped squares. \
		 */ \
		jboard.black = board->black & ~(from | jumps); \
		jboard.white = board->white & ~(from | jumps); \
		jboard.king = board->king & ~(from | jumps); \
\
		/* Put him on the square where he lands. */ \
		jboard.white |= lands; \
		jboard.king |= lands; \
		assign_path(&jboard, cap, capture_count, movelist->count); \
		func(&jboard, movelist, lands, cap, capture_count + 1, kcc, ek); \
		found_jump = 1; \
	}

	
/*
 * We're in the middle of a jump on even rows.
 * If we've reached the end of the jump, then add this board to the movelist.
 * If not, call again recursively.
 */
static void white_king_jump_even(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count,
					int king_capture_count, int earliest_king)
{
	BITBOARD jumps;
	BITBOARD lands;
	BITBOARD free;
	int found_jump;
	BOARD jboard;

	found_jump = 0;
	free = ~(board->black | board->white);

	WHITE_KING_JUMP(LFJ_EVEN, LF_EVEN_MOVE_SHIFT, LF_EVEN_JUMP_SHIFT, <<, white_king_jump_even,
				cap, capture_count, king_capture_count, earliest_king);
	WHITE_KING_JUMP(RFJ_EVEN, RF_EVEN_MOVE_SHIFT, RF_EVEN_JUMP_SHIFT, <<, white_king_jump_even,
				cap, capture_count, king_capture_count, earliest_king);

	WHITE_KING_JUMP(LBJ_EVEN, LB_EVEN_MOVE_SHIFT, LB_EVEN_JUMP_SHIFT, >>, white_king_jump_even,
				cap, capture_count, king_capture_count, earliest_king);
	WHITE_KING_JUMP(RBJ_EVEN, RB_EVEN_MOVE_SHIFT, RB_EVEN_JUMP_SHIFT, >>, white_king_jump_even,
				cap, capture_count, king_capture_count, earliest_king);

	/* If we didn't find any move jumps, then add the board that was
	 * passed in to the movelist.
	 */
	if (!found_jump) {
		cap[movelist->count].capture_count = capture_count;
		cap[movelist->count].king_capturing = 1;
		cap[movelist->count].king_capture_count = king_capture_count;
		cap[movelist->count].earliest_king = earliest_king;

		/* Copy the path to the next potential jump move. */
		copy_path(cap, movelist);
		movelist->board[movelist->count++] = *board;
	}
}


/*
 * We're in the middle of a jump on odd rows.
 * If we've reached the end of the jump, then add this board to the movelist.
 * If not, call again recursively.
 */
static void white_king_jump_odd(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count,
					int king_capture_count, int earliest_king)
{
	BITBOARD jumps;
	BITBOARD lands;
	BITBOARD free;
	int found_jump;
	BOARD jboard;

	found_jump = 0;
	free = ~(board->black | board->white);

	WHITE_KING_JUMP(LFJ_ODD, LF_ODD_MOVE_SHIFT, LF_ODD_JUMP_SHIFT, <<, white_king_jump_odd,
				cap, capture_count, king_capture_count, earliest_king);
	WHITE_KING_JUMP(RFJ_ODD, RF_ODD_MOVE_SHIFT, RF_ODD_JUMP_SHIFT, <<, white_king_jump_odd,
				cap, capture_count, king_capture_count, earliest_king);

	WHITE_KING_JUMP(LBJ_ODD, LB_ODD_MOVE_SHIFT, LB_ODD_JUMP_SHIFT, >>, white_king_jump_odd,
				cap, capture_count, king_capture_count, earliest_king);
	WHITE_KING_JUMP(RBJ_ODD, RB_ODD_MOVE_SHIFT, RB_ODD_JUMP_SHIFT, >>, white_king_jump_odd,
				cap, capture_count, king_capture_count, earliest_king);

	/* If we didn't find any move jumps, then add the board that was
	 * passed in to the movelist.
	 */
	if (!found_jump) {
		cap[movelist->count].capture_count = capture_count;
		cap[movelist->count].king_capturing = 1;
		cap[movelist->count].king_capture_count = king_capture_count;
		cap[movelist->count].earliest_king = earliest_king;

		/* Copy the path to the next potential jump move. */
		copy_path(cap, movelist);
		movelist->board[movelist->count++] = *board;
	}
}


#define WHITE_MAN_JUMP(mask, from_shift, jumps_shift, func, cap, capture_count) \
	jumps = ((from & mask) >> from_shift) & board->black & ~board->king; \
	lands = (jumps >> jumps_shift) & free; \
	if (lands) { \
\
		/* Found a jump. \
		 * Make the move on a copy of the board to be \
		 * passed along to the recursion function. \
		 * First clear the from square and jumped squares. \
		 */ \
		jboard.white = board->white & ~from; \
		jboard.black = board->black & ~jumps; \
		jboard.king = board->king & ~jumps; \
\
		/* Put him on the square where he lands. */ \
		jboard.white |= lands; \
\
		/* See if this made him a king. */ \
		if (lands & WHITE_KING_RANK_MASK) { \
			jboard.king |= lands; \
\
			/* This terminates the jump. */ \
			cap[movelist->count].capture_count = capture_count + 1; \
			cap[movelist->count].king_capturing = 0; \
			cap[movelist->count].king_capture_count = 0; \
			cap[movelist->count].earliest_king = 0; \
			assign_path(&jboard, cap, capture_count, movelist->count); \
			copy_path(cap, movelist); \
			movelist->board[movelist->count++] = jboard; \
		} \
		else {\
			assign_path(&jboard, cap, capture_count, movelist->count); \
			func(&jboard, movelist, lands, cap, capture_count + 1); \
		} \
		found_jump = 1; \
	}


/*
 * We're in the middle of a jump.
 * If we've reached the end of the jump, then add this board to the movelist.
 * If not, call again recursively.
 */
static void white_man_jump_even(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap, int capture_count)
{
	BITBOARD jumps;
	BITBOARD lands;
	BITBOARD free;
	int found_jump;
	BOARD jboard;

	found_jump = 0;
	free = ~(board->black | board->white);

	WHITE_MAN_JUMP(LBJ_EVEN, LB_EVEN_MOVE_SHIFT, LB_EVEN_JUMP_SHIFT, white_man_jump_even, cap, capture_count);
	WHITE_MAN_JUMP(RBJ_EVEN, RB_EVEN_MOVE_SHIFT, RB_EVEN_JUMP_SHIFT, white_man_jump_even, cap, capture_count);

	/* If we didn't find any move jumps, then add the board that was
	 * passed in to the movelist.
	 */
	if (!found_jump) {
		cap[movelist->count].capture_count = capture_count;
		cap[movelist->count].king_capturing = 0;
		cap[movelist->count].king_capture_count = 0;
		cap[movelist->count].earliest_king = 0;

		/* Copy the path to the next potential jump move. */
		copy_path(cap, movelist);
		movelist->board[movelist->count++] = *board;
	}
}


/*
 * We're in the middle of a jump.
 * If we've reached the end of the jump, then add this board to the movelist.
 * If not, call again recursively.
 */
static void white_man_jump_odd(BOARD *board, MOVELIST *movelist, BITBOARD from,
					CAPTURE_INFO *cap,	int capture_count)
{
	BITBOARD jumps;
	BITBOARD lands;
	BITBOARD free;
	int found_jump;
	BOARD jboard;

	found_jump = 0;
	free = ~(board->black | board->white);

	WHITE_MAN_JUMP(LBJ_ODD, LB_ODD_MOVE_SHIFT, LB_ODD_JUMP_SHIFT, white_man_jump_odd, cap, capture_count);
	WHITE_MAN_JUMP(RBJ_ODD, RB_ODD_MOVE_SHIFT, RB_ODD_JUMP_SHIFT, white_man_jump_odd, cap, capture_count);

	/* If we didn't find any move jumps, then add the board that was
	 * passed in to the movelist.
	 */
	if (!found_jump) {
		cap[movelist->count].capture_count = capture_count;
		cap[movelist->count].king_capturing = 0;
		cap[movelist->count].king_capture_count = 0;
		cap[movelist->count].earliest_king = 0;

		/* Copy the path to the next potential jump move. */
		copy_path(cap, movelist);
		movelist->board[movelist->count++] = *board;
	}
}


#if !SAVE_CAPTURE_PATH
/* 
 * Develop masks of the squares that black and white can move to without jumping.
 */
void moveto_squares_ital(BOARD *b, BITBOARD *bm_moveto,
						 BITBOARD *bk_moveto,
						 BITBOARD *wm_moveto,
						 BITBOARD *wk_moveto)
{
	BITBOARD man, king;
	BITBOARD free;

	free = ~(b->black | b->white);
	man = b->black & ~b->king;
	*bm_moveto = ((man & LF_EVEN) << LF_EVEN_MOVE_SHIFT) |
						((man & RF_EVEN) << RF_EVEN_MOVE_SHIFT) |
						((man & LF_ODD) << LF_ODD_MOVE_SHIFT) |
						((man & RF_ODD) << RF_ODD_MOVE_SHIFT);
	king = b->black & b->king;
	*bk_moveto = ((king & LB_EVEN) >> LB_EVEN_MOVE_SHIFT) |
						((king & RB_EVEN) >> RB_EVEN_MOVE_SHIFT) |
						((king & LB_ODD) >> LB_ODD_MOVE_SHIFT) |
						((king & RB_ODD) >> RB_ODD_MOVE_SHIFT) |
						((king & LF_EVEN) << LF_EVEN_MOVE_SHIFT) |
						((king & RF_EVEN) << RF_EVEN_MOVE_SHIFT) |
						((king & LF_ODD) << LF_ODD_MOVE_SHIFT) |
						((king & RF_ODD) << RF_ODD_MOVE_SHIFT);
	*bm_moveto &= free;
	*bk_moveto &= free;

	man = b->white & ~b->king;
	*wm_moveto = ((man & LB_EVEN) >> LB_EVEN_MOVE_SHIFT) |
						((man & RB_EVEN) >> RB_EVEN_MOVE_SHIFT) |
						((man & LB_ODD) >> LB_ODD_MOVE_SHIFT) |
						((man & RB_ODD) >> RB_ODD_MOVE_SHIFT);
	king = b->white & b->king;
	*wk_moveto = ((king & LF_EVEN) << LF_EVEN_MOVE_SHIFT) |
						((king & RF_EVEN) << RF_EVEN_MOVE_SHIFT) |
						((king & LF_ODD) << LF_ODD_MOVE_SHIFT) |
						((king & RF_ODD) << RF_ODD_MOVE_SHIFT) |
						((king & LB_EVEN) >> LB_EVEN_MOVE_SHIFT) |
						((king & RB_EVEN) >> RB_EVEN_MOVE_SHIFT) |
						((king & LB_ODD) >> LB_ODD_MOVE_SHIFT) |
						((king & RB_ODD) >> RB_ODD_MOVE_SHIFT);
	*wm_moveto &= free;
	*wk_moveto &= free;
}
#endif



