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
#include "cbconsts.h"
#include "CBstructs.h"
#include "checkerboard.h"
#include "utility.h"



void boardtobitboard(int board8[8][8], struct pos *p);


/* database values */
#define DRAW 0
#define WIN 1
#define LOSS 2
#define UNKNOWN 3


#include "pdnfind.h"
#include "PDNparser.h"
#include "min_movegen.h"
#include "bitboard.h"

static void movetonotation(struct pos pos,struct move m, char *str, int color);
static int lastbit(int32 x);


/* globals */
static int minbm=0,minwm=0,minbk=0,minwk=0,maxbm=12,maxwm=12,maxwk=12,maxbk=12;
static int minply=0, maxply=20;
static int32 blackmen=0,whitemen=0,blackkings=0,whitekings=0,empty=0;
static int maxwhite=12,maxblack=12,minwhite=0,minblack=0;
static int equal=0;

static struct PDNlistentry *positions = NULL;

static int n=0; // holds the number of positions


int pdnfindreset(void)
	{
	if(positions != NULL)
		{
		free(positions);
		positions = NULL;
		}
	n=0;
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
	board8toFEN(b, FEN, BLACK, 21);
	fprintf(fp,"%s", FEN);
	fclose(fp);

	for(i=0;i<n;i++)
		{
		if((positions[i].black == black) && (positions[i].white == white) && 
			(positions[i].kings == kings) && (positions[i].color == (int32)color) )
			{
			
			list[found] = positions[i].gameindex;
			found++;
			//fprintf(fp,"found position in game #%i at position #%i\n",positions[i].gameindex,i);
			if(positions[i].result == WIN)
				r->win++;
			if(positions[i].result == LOSS)
				r->loss++;
			if(positions[i].result == DRAW)
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

	for(i=0;i<n;i++)
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
	int i,ply,gamenumber=0;
	FILE *fp;
	char *start, *startheader, *starttoken, *buffer, game[GAMESIZE],header[256],token[1024];
	int from,to;
	int from2,to2;
	size_t bytesread;
	struct move movelist[MAXMOVES];
	int moves;
	struct pos p;
	int color=BLACK;
	char notation[256];
	char headername[256],headervalue[256];
	int result=UNKNOWN;
	int win=0,loss=0,draw=0,unknown=0;
	char FEN[255];
	char setup[255];
//	char pdnopenname[MAX_PATH];
	int board8[8][8];
	size_t filesize=0;
	unsigned int square[32] = {SQ1, SQ2, SQ3, SQ4, SQ5, SQ6, SQ7, SQ8, SQ9, SQ10, SQ11, SQ12,
		              SQ13, SQ14, SQ15, SQ16, SQ17, SQ18, SQ19, SQ20, SQ21, SQ22,
					  SQ23, SQ24, SQ25, SQ26, SQ27, SQ28, SQ29, SQ30, SQ31, SQ32};

	
	// get number of games in PDN
	games_in_pdn = PDNparseGetnumberofgames(filename);

	// allocate memory for database positions. 
	// hans' 22'000 game archive has about 1.2 million positions, i.e. 54 
	// a typical game might have 80 moves. allocate 80x the number of games
	maxpos = 1000 + 80*games_in_pdn;
	
	if(positions == NULL)
		{
		positions = (struct PDNlistentry *) malloc(maxpos * sizeof(struct PDNlistentry));
//		positions =  malloc(maxpos * sizeof(struct PDNlistentry));
		if(positions == NULL)
			return 0;
        }

	
	// get size of the file we want to open
	filesize = getfilesize(filename);
	filesize = ((filesize/1024)+1)*1024;
	
	// allocate memory for the file
	buffer = (char *) malloc(filesize);
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
					result=DRAW;
					draw++;
					}
				if(strcmp(headervalue,"\"1-0\"")==0)
					{
					result=WIN;
					win++;
					}
				if(strcmp(headervalue,"\"0-1\"")==0)
					{
					result=LOSS;
					loss++;
					}
				if(strcmp(headervalue,"\"*\"")==0)
					{
					result=UNKNOWN;
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
			color = BLACK;
			ply = 0;
			}
		// save position:
		positions[n].black = p.bm|p.bk;
		positions[n].white = p.wm|p.wk;
		positions[n].kings = p.bk|p.wk;
		positions[n].gameindex = gamenumber;
		positions[n].result = result;
		positions[n].color = color;
		n++;
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
			
			moves=makecapturelist(&p, movelist, color);
			if(moves==0)
				moves=makemovelist(&p,movelist,color);
			for(i=0;i<moves;i++)
				{
				// speed improvement here: we already know our from
				// square, check against the move, and throw out all
				// moves which don't fit the from square.
				if(((movelist[i].bm|movelist[i].bk|movelist[i].wm|movelist[i].wk) & square[from-1]) == 0)
					continue;
				movetonotation(p,movelist[i],notation,color);
				PDNparseTokentonumbers(notation,&from2,&to2);
				if(from==from2 && to==to2)
					{
					// inline domove as its an inner loop.
					p.bm ^= movelist[i].bm;
					p.bk ^= movelist[i].bk;
					p.wm ^= movelist[i].wm;
					p.wk ^= movelist[i].wk;
					color^=CC;
					break;
					}
				}
			// save position:
			positions[n].black = p.bm|p.bk;
			positions[n].white = p.wm|p.wk;
			positions[n].kings = p.bk|p.wk;
			positions[n].gameindex = gamenumber;
			positions[n].result = result;
			positions[n].color = color;
		
			ply++;
			n++;
			if(n>=maxpos)
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


static void movetonotation(struct pos pos,struct move m, char *str, int color)
	{

   /* make a notation out of a move */
   /* m is the move, str the string, color the side to move */

   int from, to;

   char c;

 /*

       WHITE
   	28  29  30  31           32  31  30  29
	 24  25  26  27           28  27  26  25
	   20  21  22  23           24  23  22  21
	 16  17  18  19           20  19  18  17
	   12  13  14  15           16  15  14  13
	  8   9  10  11           12  11  10   9
	    4   5   6   7            8   7   6   5
	  0   1   2   3            4   3   2   1
	      BLACK
*/
	static int square[32]={4,3,2,1,8,7,6,5,
   					 12,11,10,9,16,15,14,13,
                   20,19,18,17,24,23,22,21,
                   28,27,26,25,32,31,30,29}; /* maps bits to checkers notation */

   if(color==BLACK)
   	{
      if(m.wk|m.wm) c='x';
        	else c='-';                      /* capture or normal ? */
      from=(m.bm|m.bk)&(pos.bm|pos.bk);    /* bit set on square from */
      to=  (m.bm|m.bk)&(~(pos.bm|pos.bk));
      from=lastbit(from);
      to=lastbit(to);
      from=square[from];
      to=square[to];
      sprintf(str,"%2i%c%2i",from,c,to);
      }
   else
   	{
      if(m.bk|m.bm) c='x';
      else c='-';                      /* capture or normal ? */
      from=(m.wm|m.wk)&(pos.wm|pos.wk);    /* bit set on square from */
      to=  (m.wm|m.wk)&(~(pos.wm|pos.wk));
      from=lastbit(from);
      to=lastbit(to);
      from=square[from];
      to=square[to];
      sprintf(str,"%2i%c%2i",from,c,to);
      }
   return;
   }

int lastbit(int32 x)
	{
	int i;

	for(i=0;i<32;i++)
		{
		if(x&(1<<i))
			return i;
		}
	
	return -1;
	}
