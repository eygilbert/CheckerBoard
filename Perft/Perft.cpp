#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "move_api.h"
#include "Fen.h"
#include "cb_interface.h"
#include "min_movegen.h"
#include "cb_movegen_intf.h"


#define TDIFF(start) (((double)(clock() + 1 - start)) / (double)CLOCKS_PER_SEC)	/* Add 1ms to prevent division by 0. */

INT64 Perft(int board[46], int color, int depth, int ply, int printpos);
void usage();



int build_movelist(int b[46], int color, struct move2 movelist[MAXMOVES])
{
	int movecount;

	movecount = generatecapturelist(b, movelist, color);
	if (!movecount)
		movecount = generatemovelist(b, movelist, color);
	return(movecount);
}


int _tmain(int argc, _TCHAR *argv[])
{
	int i, color, depth;
	int board46[46];
	INT64 nodes;
	int printpos;
	char *p, *fenpos;
	clock_t t0;

	fenpos = 0;
	printpos = 0;
	depth = 12;
	if (argc == 1)
		usage();
	for (i = 1; i < argc; ++i) {
		p = argv[i];
		if (*p == '-') {
			switch (p[1]) {
			case 'd':
				if (p[2])
					depth = atoi(p + 2);
				else {
					++i;
					depth = atoi(argv[i]);
				}
				break;

			case 'f':
				if (p[2])
					fenpos = p + 2;
				else {
					++i;
					fenpos = argv[i];
				}
				break;

			case 'p':
				printpos = 1;
				break;
			}
		}
	}

	if (fenpos) {
		if (parse_fen(fenpos, board46, &color)) {
			printf("Error in parse_fen()\n");
			exit(1);
		}
	}
	else
		get_start_pos(board46, &color);

	for (i = 1; i <= depth; ++i) {
		t0 = clock();
		nodes = Perft(board46, color, i, 0, printpos);
		printf("perft(%d) %I64d nodes, %.2f sec, %.0f knodes/sec\n",
			i, nodes, TDIFF(t0), (double)nodes / (1000.0 * TDIFF(t0)));
	}
	return 0;
}


INT64 Perft(int board[46], int color, int depth, int ply, int printpos)
{
	int nmoves, i;
	INT64 nodes, sumnodes;
	char fenbuf[150];
	struct move2 movelist[MAXMOVES];

	nmoves = build_movelist(board, color, movelist);
	if (depth == 1)
		return(nmoves);

	sumnodes = 0;
    for (i = 0; i < nmoves; ++i) {
		domove(board, movelist[i]);
        nodes = Perft(board, CB_CHANGECOLOR(color), depth - 1, ply + 1, printpos);
		if (ply == 0 && printpos) {
			print_fen(board, color, fenbuf);
			printf("%s; nodes %I64d\n", fenbuf, nodes);
		}
		undomove(board, movelist[i]);
		sumnodes += nodes;
	}
    return(sumnodes);
}


void usage()
{
	char *usagetxt = 
		"usage: perft [options]\n"
		"\n"
		"-d depth           set max depth (default 12)\n"
		"-p                 print first successor positions and counts\n"
		"-f fenstring       set the root position (use FEN string)\n\n";
	printf(usagetxt);
}

