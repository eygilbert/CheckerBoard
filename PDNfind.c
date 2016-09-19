// pdnfind.c
//
// adds the functionality to search pdn databases:
//
// pdnopen(filename, gametype) 
//	indexes a pdn database 
//
// int pdnfind(struct pos position, std::vector<int> &preview_to_game_index_map);
//	returns the number of games found, and returns the indices of these games in the pdn database in the array list

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlwapi.h>
#include <vector>
#include "standardheader.h"
#include "cb_interface.h"
#include "min_movegen.h"
#include "cbconsts.h"
#include "CBstructs.h"
#include "checkerboard.h"
#include "utility.h"
#include "fen.h"
#include "bitboard.h"
#include "lsb.h"
#include "pdnfind.h"
#include "PDNparser.h"
#include "bitboard.h"


std::vector<PDN_position> pdn_positions;


inline int bitnum_to_square(int bitnum, int gametype)
{
	if (gametype == GT_ITALIAN)
		return(1 + bitnum);
		
	return(1 + 4 * (bitnum / 4) + 3 - (bitnum & 3));
}


void get_fromto_squares(struct pos *pos, struct move *m, int color, int *fromsq, int *tosq)
{
	uint32_t frombb, tobb;

	if (color == CB_BLACK) {
		frombb = (m->bm | m->bk) & (pos->bm | pos->bk);
		tobb = (m->bm | m->bk) & ~(pos->bm | pos->bk);
	}
	else {
		frombb = (m->wm | m->wk) & (pos->wm | pos->wk);
		tobb = (m->wm | m->wk) & ~(pos->wm | pos->wk);
	}
	*fromsq = bitnum_to_square(LSB(frombb), gametype());
	*tosq = bitnum_to_square(LSB(tobb), gametype());
}


void print_fen(struct pos *p, int color, char *buf)
{
	unsigned int mask, sq;

	sprintf(buf, "\n[FEN \"%c", color == CB_BLACK ? 'B' : 'W');

	/* Print the white men and kings. */
	sprintf(buf + strlen(buf), ":W");
	for (mask = p->wm | p->wk; mask; ) {

		/* Peel off the lowest set bit in mask. */
		sq = mask & -(int)mask;
		if (sq & p->wk)
			sprintf(buf + strlen(buf), "K%d", bitnum_to_square(LSB(sq), gametype()));
		else
			sprintf(buf + strlen(buf), "%d", bitnum_to_square(LSB(sq), gametype()));
		mask = mask & (mask - 1);
		if (mask)
			sprintf(buf + strlen(buf), ",");
	}

	/* Print the black men and kings. */
	sprintf(buf + strlen(buf), ":B");
	for (mask = p->bm | p->bk; mask; ) {

		/* Peel off the lowest set bit in mask. */
		sq = mask & -(int)mask;
		if (sq & p->bk)
			sprintf(buf + strlen(buf), "K%d", bitnum_to_square(LSB(sq), gametype()));
		else
			sprintf(buf + strlen(buf), "%d", bitnum_to_square(LSB(sq), gametype()));
		mask = mask & (mask - 1);
		if (mask)
			sprintf(buf + strlen(buf), ",");
	}
	sprintf(buf + strlen(buf), ".\"]");
}


void log_fen(struct pos *p, int color)
{
	char buf[150];

	print_fen(p, color, buf);
	CBlog(buf);
}


void log_moves(struct pos *p, int color, struct move *movelist, int nmoves)
{
	int i, fromsq, tosq;
	char buf[50];

	for (i = 0; i < nmoves; ++i) {
		get_fromto_squares(p, movelist + i, color, &fromsq, &tosq);
		sprintf(buf, "%d-%d", fromsq, tosq);
		CBlog(buf);
	}
}


void reset_pdn_positions()
{
	pdn_positions.clear();
}


int pdnfind(struct pos *p, int color, std::vector<int> &preview_to_game_index_map, RESULT *r)
	{
	// pdnfind populates a list of game indexes in the pdn database which 
	// contain the current position, i.e. preview_to_game_index_map[0] is the first game index
	// where the current position occurs, preview_to_game_index_map[1] the second etc.
	// it returns the number of games found.

	int i;
	int nfound; // number of games found
	uint32_t black, white, kings;
	char FEN[256];
	int b[8][8];

	if (pdn_positions.size() == 0)
		return 0;

	black = p->bm|p->bk;
	white = p->wm|p->wk;
	kings = p->bk|p->wk;

	r->win = 0;
	r->loss = 0;
	r->draw = 0;

	nfound = 0;
	for (i = 0; i < (int)pdn_positions.size(); ++i) {
		if ((pdn_positions[i].black == black) && (pdn_positions[i].white == white) && 
			(pdn_positions[i].kings == kings) && (pdn_positions[i].color == (unsigned int)color)) {
			
			/* Avoid adding the same game multiple times when it has repeated positions. */
			if (nfound > 0 && preview_to_game_index_map[nfound - 1] == pdn_positions[i].gameindex)
				continue;

			preview_to_game_index_map.push_back(pdn_positions[i].gameindex);
			nfound++;
			if (pdn_positions[i].result == CB_WIN)
				r->win++;
			if (pdn_positions[i].result == CB_LOSS)
				r->loss++;
			if (pdn_positions[i].result == CB_DRAW)
				r->draw++;
		}
	}

	/* Log the results. */
	bitboardtoboard8(p, b);
	board8toFEN(b, FEN, color, gametype());
	cblog("pdnfind(): \"%s\", %d games found\n", FEN, nfound);
	return nfound;
}


int pdnfindtheme(struct pos *p, std::vector<int> &preview_to_game_index_map)
	{
	// finds a "theme" in a game.
	// only if the "theme" is on the board for at least minplies.

	const int minplies = 4;
	int i;
	int nfound;
	uint32_t black, white, kings;
	int ngames;
	std::vector<unsigned short> histogram;
	
	if (pdn_positions.size() == 0)
		return 0;

	/* The last entry in pdn_positins has the gameindex of the last game. */
	ngames = (pdn_positions.end() - 1)->gameindex + 1;
	histogram.assign(ngames, 0);

	black = p->bm|p->bk;
	white = p->wm|p->wk;
	kings = p->bk|p->wk;

	for (i = 0; i < (int)pdn_positions.size(); i++) {
		if (((pdn_positions[i].black & black) == black) && 
					((pdn_positions[i].white & white) == white) && 
					((pdn_positions[i].kings & kings) == kings)) {
			//count how often this theme occurs in one game
			histogram[pdn_positions[i].gameindex]++;
		}
	}
	
	nfound = 0;
	for (i = 0; i < histogram.size(); ++i) {
		if (histogram[i] > minplies) {
			preview_to_game_index_map.push_back(i);
			nfound++;
		}
	}

	cblog("pdnfindtheme(): %d matching games found\n", nfound);
	return nfound;
}


int pdnopen(char filename[256], int gametype)
	{
	// parses a pdn file and makes it ready to be used by PDNfind 
	// the games are read and the struct PDN_position positions is used
	// to store all games.
	// PDN_position contains the game index in the database, so it can
	// be retrieved from a position

	int games_in_pdn;
	int maxpos;
	int i,ply,gamenumber;
	FILE *fp;
	char *start, *startheader, *starttoken, *buffer, game[GAMESIZE],header[256],token[1024];
	int from,to;
	size_t bytesread;
	struct pos p;
	int color=CB_BLACK;
	char headername[256],headervalue[256];
	int result;
	int win=0,loss=0,draw=0,unknown=0;
	char FEN[255];
	char setup[255];
	int board8[8][8];
	size_t filesize;
	PDN_position position;
	
	// get number of games in PDN
	games_in_pdn = PDNparseGetnumberofgames(filename);

	// Reserve space for database positions. 
	// Not a hard limit. It just makes it more efficient to build the list.
	// hans' 22'000 game archive has about 1.2 million positions, avg 54 pos/game.
	maxpos = 1000 + 54 * games_in_pdn;
	try {
		pdn_positions.clear();
		pdn_positions.reserve(maxpos);
	}
	catch (...) {
		return(0);
	}

	// get size of the file we want to open
	filesize = getfilesize(filename);
	filesize = ((filesize/1024)+1)*1024;
	
	// allocate memory for the file
	buffer = (char *)malloc(filesize);
	if (buffer == NULL)
		return 0;

	// open the file
	fp=fopen(filename,"r");
	if(fp==NULL)
		{
		printf("\ncould not open input file %s\n",filename);
		free(buffer);
		return 0;
		}
	
	// read file into memory 
	bytesread=fread(buffer,1,filesize,fp);
	fclose(fp);
	// set termination 0 so functions wont run out of the buffer
	buffer[bytesread]=0;

	// start parsing
	start = buffer;
	gamenumber = 0;
	while(PDNparseGetnextgame(&start,game)) 
		//pdnparsenextgame puts PDN of one game in "game" and terminates "game" with a 0.					
		{
		// load headers 
		startheader = game;
		// double check zero termination of game
		game[GAMESIZE-1]=0;
		sprintf(setup,"");
		result = CB_UNKNOWN;
		while(PDNparseGetnextheader(&startheader,header))
			{
			sscanf(header,"%s %s",headername,headervalue);
			for(i=0; i<(int)strlen(headername);i++)
				headername[i] = (char)tolower(headername[i]);
			
			if(strcmp(headername,"result")==0)
				{
				if(strcmp(headervalue,"\"1/2-1/2\"")==0)
					{
					result=CB_DRAW;
					draw++;
					}
				if(strcmp(headervalue,"\"1-0\"")==0)
					{
					result=CB_WIN;
					win++;
					}
				if(strcmp(headervalue,"\"0-1\"")==0)
					{
					result=CB_LOSS;
					loss++;
					}
				if(strcmp(headervalue,"\"*\"")==0)
					{
					result=CB_UNKNOWN;
					unknown++;
					}
				}
			
			if(strcmp(headername,"setup")==0)
				sprintf(setup,"%s",headervalue);
			if(strcmp(headername,"fen")==0)
				{
				// headervalue+1 because headervalue contains "W:...", it has these """" thingies in.
				sprintf(FEN,"%s",headervalue+1);
				// pdn requires that fen tag is accompanied by "setup "1"" tag, which is
				// kind of redundant. so i just set that anyway to make it work always.
				sprintf(setup,"\"1\"");
				}
			}

		if(strcmp(setup,"\"1\"")==0)
			{
			FENtoboard8(board8,FEN,&color, gametype);
			// it's a setup position - have to parse FEN!
			boardtobitboard(board8,&p);
			ply=0;
			}
		else
			{
			// set start position 
			p.bk=0;
			p.wk=0;
			p.bm=0x00000FFF;
			p.wm=0xFFF00000;
			bitboardtoboard8(&p, board8);
			color = get_startcolor(gametype);
			ply = 0;
			}
		// save position:
		position.black = p.bm|p.bk;
		position.white = p.wm|p.wk;
		position.kings = p.bk|p.wk;
		position.gameindex = gamenumber;
		position.result = result;
		position.color = color;
		try {
			pdn_positions.push_back(position);
		}
		catch (...) {
			free(buffer);
			return(0);
		}

		// load moves 

		starttoken = startheader;
		while(PDNparseGetnexttoken(&starttoken,token))
			{
			// if it's a move, continue
			if(token[strlen(token)-1]=='.') 
				continue;
			// if it's a comment, continue
			if(token[0]=='{')
				continue;
			// if it's a nemesis-style comment or a variation, continue
			if(token[0]=='(')
				continue;

			PDNparseTokentonumbers(token,&from,&to);
			// we now have the from and to squares of the move in 
			// the variables from, to
			
			// find the move which corresponds to this
			struct CBmove move;
			extern CB_ISLEGAL islegal;
			if (islegal(board8, color, from, to, &move)) {
				domove(move, board8);
				boardtobitboard(board8, &p);
				color = CB_CHANGECOLOR(color);
			}

			// save position:
			position.black = p.bm|p.bk;
			position.white = p.wm|p.wk;
			position.kings = p.bk|p.wk;
			position.gameindex = gamenumber;
			position.result = result;
			position.color = color;
			try {
				pdn_positions.push_back(position);
			}
			catch (...) {
				free(buffer);
				return(0);
			}
		
			ply++;
			} // end game
		gamenumber++;
		}

	cblog("pdnopen(): games %d, positions %zd\n", gamenumber, pdn_positions.size());
	free(buffer);
	return 1;
	}

