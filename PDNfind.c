// pdnfind.c
//
// adds the functionality to search pdn databases:
//
// pdnopen(filename) 
//	indexes a pdn database 
//
// int pdnfind(struct pos position, int list[MAXGAMES])
//	returns the number of games found, and returns the indices of these games in the pdn database in the array list

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlwapi.h>
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


static struct PDNlistentry *positions = NULL;
static int npos; // holds the number of positions to search


inline int bitnum_to_square(int bitnum, int gametype)
{
	if (gametype == GT_ITALIAN)
		return(1 + bitnum);
		
	return(1 + 4 * (bitnum / 4) + 3 - (bitnum & 3));
}


void get_fromto_squares(struct pos *pos, struct move *m, int color, int *fromsq, int *tosq)
{
	int32 frombb, tobb;

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


int pdnfindreset(void)
	{
	if(positions != NULL)
		{
		free(positions);
		positions = NULL;
		}
	npos=0;
	return 1;
	}

int pdnfind(struct pos *p, int color, int list[MAXGAMES], RESULT *r)
	{
	// pdnfind populates a list of indexes in the pdn database which 
	// contain the current position, i.e. list[0] is the first index
	// where the current position occurs, list[1] the second etc.
	// it returns the number of games found.

	int i;
	int found = 0; // number of games found
	int32 black,white,kings;
	FILE *fp;
	char FEN[256];
	char filename[MAX_PATH];
	int b[8][8];

	if(positions == NULL)
		return 0;

	black = p->bm|p->bk;
	white = p->wm|p->wk;
	kings = p->bk|p->wk;

	r->win = 0;
	r->loss = 0;
	r->draw = 0;

	strcpy(filename, CBdocuments);
	PathAppend(filename, "pdnfind.txt");
	fp = fopen(filename,"w");
	bitboardtoboard8(p, b);
	board8toFEN(b, FEN, color, gametype());
	fprintf(fp,"%s", FEN);
	fclose(fp);

	for(i=0;i<npos;i++)
		{
		if((positions[i].black == black) && (positions[i].white == white) && 
			(positions[i].kings == kings) && (positions[i].color == (int32)color) )
			{
			
			list[found] = positions[i].gameindex;
			found++;
			//fprintf(fp,"found position in game #%i at position #%i\n",positions[i].gameindex,i);
			if(positions[i].result == CB_WIN)
				r->win++;
			if(positions[i].result == CB_LOSS)
				r->loss++;
			if(positions[i].result == CB_DRAW)
				r->draw++;
			if(found==MAXGAMES)
				break;
			}
		}

	return found;
	}

int pdnfindtheme(struct pos *p, int list[MAXGAMES])
	{
	// finds a "theme" in a game.
	// only if the "theme" is on the board for at least num plies.

	int num=4;
	int i;
	int found = 0; // number of games found
	int32 black,white,kings;
	int existsingame[MAXGAMES];
	
	if(positions == NULL)
		return 0;

	for(i=0;i<MAXGAMES;i++)
		existsingame[i]=0;

	black = p->bm|p->bk;
	white = p->wm|p->wk;
	kings = p->bk|p->wk;

	for(i=0;i<npos;i++)
		{
		if(((positions[i].black&black) == black) && ((positions[i].white&white) == white) && ((positions[i].kings&kings) == kings))
			{
			//count how often this theme occurs in one game
			existsingame[positions[i].gameindex]++;
			}
		}
	
	for(i=0;i<MAXGAMES;i++)
		{
		if(existsingame[i]>num)
			{
			list[found] = i;
			found++;
			if(found==MAXGAMES)
				break;
			}
		}
	return found;
	}


int pdnopen(char filename[256], int gametype)
	{
	// parses a pdn file and makes it ready to be used by PDNfind 
	// the games are read and the struct PDNlistentry positions is used
	// to store all games.
	// PDNlistentry contains the game index in the database, so it can
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
	int result=CB_UNKNOWN;
	int win=0,loss=0,draw=0,unknown=0;
	char FEN[255];
	char setup[255];
	int board8[8][8];
	size_t filesize;
	
	// get number of games in PDN
	games_in_pdn = PDNparseGetnumberofgames(filename);

	// allocate memory for database positions. 
	// hans' 22'000 game archive has about 1.2 million positions, i.e. 54 
	// a typical game might have 80 moves. allocate 80x the number of games
	maxpos = 1000 + 80*games_in_pdn;
	
	if(positions == NULL)
		{
		positions = (struct PDNlistentry *)calloc(maxpos, sizeof(struct PDNlistentry));
		if(positions == NULL)
			return 0;
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
		positions[npos].black = p.bm|p.bk;
		positions[npos].white = p.wm|p.wk;
		positions[npos].kings = p.bk|p.wk;
		positions[npos].gameindex = gamenumber;
		positions[npos].result = result;
		positions[npos].color = color;
		npos++;
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
			positions[npos].black = p.bm|p.bk;
			positions[npos].white = p.wm|p.wk;
			positions[npos].kings = p.bk|p.wk;
			positions[npos].gameindex = gamenumber;
			positions[npos].result = result;
			positions[npos].color = color;
		
			ply++;
			npos++;
			if(npos>=maxpos)
				{
				free(buffer);
				return 0;
				}
			} // end game
		gamenumber++;
		if(gamenumber >= MAXGAMES)
			break;
		}


	free(buffer);
	// TODO: the following lines will cause trouble on new windows systems unless CB is run as admin
	/*strcpy(pdnopenname, CBdirectory);
	PathAppend(pdnopenname, "pdnopen.txt");
	fp = fopen(pdnopenname,"w");
	if(fp != NULL) {
		fprintf(fp,"games %i positions %i",gamenumber, n);
		fprintf(fp,"/ngames_in_pdn %i, maxpos guess %i", games_in_pdn, maxpos);
		fclose(fp);
	}*/
	return 1;
	}

