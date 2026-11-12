/*
 * Perft for CheckerBoard's built-in move generator (CB_movegen.c, English rules).
 * Counts move sequences, as the Simplech/Dama and Kingsrow perft programs do.
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "cb_interface.h"
#include "CB_movegen.h"
#include "fen.h"

static void domove(const CBmove &m, Board8x8 b)
{
	b[m.from.x][m.from.y] = 0;
	b[m.to.x][m.to.y] = m.newpiece;
	for (int i = 0; i < m.jumps; ++i)
		b[m.del[i].x][m.del[i].y] = 0;
}

static int64_t perft(Board8x8 b, int color, int depth)
{
	CBmove movelist[MAXMOVES];
	int isjump, n;
	int64_t nodes;

	n = getmovelist(color, movelist, b, &isjump);
	if (depth == 1)
		return(n);

	nodes = 0;
	for (int i = 0; i < n; ++i) {
		Board8x8 next;

		memcpy(next, b, sizeof(next));
		domove(movelist[i], next);
		nodes += perft(next, CB_CHANGECOLOR(color), depth - 1);
	}
	return(nodes);
}

static double seconds(clock_t t0)
{
	return((clock() - t0) / (double)CLOCKS_PER_SEC);
}

/* Kingsrow's standard English perft positions (Kingsrow Perft -t). */
static int std_tests(void)
{
	struct {
		const char *fen;
		int depth;
		int64_t nodes;
	} tests[] = {
		{"W:B1,2,3,4,5,6,7,8,9,10,11,12:W21,22,23,24,25,26,27,28,29,30,31,32", 13, 1766623630},
		{"B:W20,K19,12,8,7,6:B28,27,25,21,K18,13", 10, 1742866067},
		{"W:W31,28,27,22,21,14,10,9:B24,20,19,15,13,7,3,1", 14, 284301593},
		{"B:W32,29,28,K26,20,K19,K16,15,12,11,7:BK31,K22,21,K18,17,14,13,9,4", 12, 439581991},
	};
	int errors = 0;

	for (int i = 0; i < (int)(sizeof(tests) / sizeof(tests[0])); ++i) {
		Board8x8 b;
		int color;
		clock_t t0;
		int64_t count;

		if (!FENtoboard8(b, tests[i].fen, &color, GT_ENGLISH)) {
			printf("bad FEN %s\n", tests[i].fen);
			return(1);
		}
		printf("%s, depth %d\n", tests[i].fen, tests[i].depth);
		t0 = clock();
		count = perft(b, color, tests[i].depth);
		printf("\t%lld nodes, %.2f sec, expected %lld: %s\n", count, seconds(t0), tests[i].nodes,
			   count == tests[i].nodes ? "OK" : "COUNT ERROR");
		if (count != tests[i].nodes)
			++errors;
	}
	printf(errors ? "%d count errors\n" : "all counts match\n", errors);
	return(errors != 0);
}

static void usage(void)
{
	printf("usage: perft_cb [options]\n"
		   "\n"
		   "-d depth           set max depth (default 10)\n"
		   "-f fenstring       set the root position (use FEN string)\n"
		   "-t                 run the standard tests (Kingsrow's English positions)\n\n");
}

int main(int argc, char *argv[])
{
	Board8x8 b;
	int color, depth = 10;
	const char *fen = "B:W21,22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,11,12";

	if (argc == 1)
		usage();
	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "-t") == 0)
			return(std_tests());
		if (strcmp(argv[i], "-d") == 0 && i + 1 < argc)
			depth = atoi(argv[++i]);
		else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
			fen = argv[++i];
	}

	if (!FENtoboard8(b, fen, &color, GT_ENGLISH)) {
		printf("bad FEN %s\n", fen);
		return(1);
	}
	for (int d = 1; d <= depth; ++d) {
		clock_t t0 = clock();
		int64_t nodes = perft(b, color, d);
		printf("perft(%d) %lld nodes, %.2f sec\n", d, nodes, seconds(t0));
	}
	return(0);
}
