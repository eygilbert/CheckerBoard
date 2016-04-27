// utility.c
//
// part of checkerboard
//
// implements various utility functions

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shlobj.h>
#include "standardheader.h"
#include "CBstructs.h"
#include "CBconsts.h"
#include "CheckerBoard.h"
#include "coordinates.h"
#include "utility.h"

// the following array describes the ACF three-move-deck. three[n][0-1-2] are
// the three move numbers which have to be executed after generating the movelist
// in all three positions. n+1 is the number used in the ACF-deck. 
// three[n][4] is the qualifier of the opening, 0,1,2,3 for normal, mailplay and probably lost, 3
// is also normal, but in CTD

int three[174][4]=
	{{0,6,0,0},{0,6,1,0},{0,6,2,2},{0,4,0,0},{0,5,2,3},{0,5,3,1},{0,5,4,3},{0,5,5,3},{0,5,6,1},{0,5,7,3},
	{0,2,1,0},{0,2,2,3},{0,2,3,2},{0,2,4,0},{0,2,5,3},{0,2,6,1},{0,2,7,0},{0,3,1,0},{0,3,2,0},{0,3,3,0},
	{0,3,4,1},{0,3,6,0},{0,0,1,0},{0,0,2,0},{0,0,3,3},{0,0,4,2},{0,0,5,0},{0,0,6,0},{0,1,1,0},{0,1,2,0},
	{0,1,3,0},{0,1,4,0},{0,1,5,0},{0,1,6,1},{0,1,7,2},{1,4,1,0},{1,4,2,3},{1,4,4,0},{1,4,5,0},{1,5,1,0},
	{1,5,3,0},{1,5,4,0},{1,5,5,0},{1,5,6,2},{1,5,0,2},{1,2,0,3},{1,3,2,0},{1,3,4,2},{1,3,6,0},{1,3,1,3},

	{1,0,2,0},{1,0,4,2},{1,0,5,0},{1,0,6,0},{1,1,2,0},{1,1,4,0},{1,1,5,0},{1,1,6,3},{2,4,2,3},{2,4,3,1},
	{2,4,4,1},{2,4,5,1},{2,4,0,3},{2,5,1,3},{2,5,2,3},{2,5,4,0},{2,5,5,0},{2,5,6,3},{2,2,0,0},{2,3,2,3},
	{2,3,3,3},{2,3,5,3},{2,3,6,0},{2,3,1,3},{2,0,2,3},{2,0,3,0},{2,0,5,2},{2,0,6,0},{2,0,1,3},{2,1,2,0},
	{2,1,3,0},{2,1,5,0},{2,1,6,0},{2,1,1,3},{3,6,2,3},{3,6,3,3},{3,6,4,3},{3,6,5,2},{3,6,6,3},{3,6,0,0},
	{3,4,2,3},{3,4,3,0},{3,4,4,3},{3,4,5,2},{3,4,6,0},{3,4,1,3},{3,5,0,0},{3,2,1,3},{3,2,2,0},{3,2,4,0},

	{3,2,5,0},{3,2,6,3},{3,3,1,0},{3,3,2,0},{3,3,5,1},{3,0,0,0},{3,1,2,0},{3,1,3,0},{3,1,6,2},{3,1,1,3},
	{4,6,3,0},{4,6,4,0},{4,6,5,0},{4,6,6,2},{4,6,1,3},{4,4,3,0},{4,4,4,0},{4,4,0,0},{4,4,1,0},{4,5,0,0},
	{4,2,2,0},{4,2,4,0},{4,2,5,0},{4,2,6,3},{4,2,0,3},{4,3,2,0},{4,3,3,0},{4,3,4,0},{4,0,0,0},{4,1,3,0},
	{4,1,7,3},{4,1,0,0},{5,6,2,3},{5,6,3,0},{5,6,4,3},{5,6,5,0},{5,6,6,2},{5,6,1,0},{5,4,2,0},{5,4,3,0},
	{5,4,4,1},{5,4,1,0},{5,5,2,3},{5,5,3,0},{5,5,7,2},{5,5,0,0},{5,5,1,0},{5,2,2,3},{5,2,3,0},{5,2,5,0},

	{5,2,6,0},{5,2,1,0},{5,3,0,1},{5,0,1,3},{5,0,2,0},{5,0,6,2},{5,0,0,0},{5,1,1,3},{5,1,0,0},{6,6,3,3},
	{6,6,4,3},{6,6,0,3},{6,6,1,0},{6,4,0,0},{6,4,1,0},{6,5,0,3},{6,5,1,3},{6,2,4,2},{6,2,0,3},{6,2,1,0},
	{6,3,0,2},{6,0,0,0},{6,1,1,0},{6,1,5,1}};

FILE *cblogfile;

extern char g_app_instance_suffix[10];


int initcolorstruct(HWND hwnd, CHOOSECOLOR *ccs, int index)
	{
	COLORREF dCustomColors[16];
	extern struct CBoptions gCBoptions;
	ccs->lStructSize = (DWORD) sizeof(CHOOSECOLOR);
	ccs->hwndOwner = (HWND) hwnd;
	ccs->hInstance = (HWND) NULL;
	ccs->lpCustColors = dCustomColors;
	ccs->Flags = CC_RGBINIT|CC_FULLOPEN;
	ccs->lCustData = 0L;
	ccs->lpfnHook = NULL;
	ccs->lpTemplateName = (LPSTR) NULL;
	ccs->rgbResult = gCBoptions.colors[index];
	return 1;
	}


int FENtoclipboard(HWND hwnd, int board8[8][8], int color, int gametype)
	{
	char *FENstring;

	FENstring = (char *) malloc(GAMEBUFSIZE);
	board8toFEN(board8, FENstring, color, gametype);
	MessageBox(hwnd, FENstring,"printout is",MB_OK);
	texttoclipboard(FENstring);
	free(FENstring);
	return 1;
	}

int PDNtoclipboard(HWND hwnd, struct PDNgame *game)
	{
	char *gamestring;

	// allocate memory for game, print game to memory, call texttoclipboard to 
	// place it on clipboard.
	gamestring = (char *) malloc(GAMEBUFSIZE);
	PDNgametoPDNstring(game,gamestring, "\r\n");
	MessageBox(hwnd,gamestring,"printout is",MB_OK);
	texttoclipboard(gamestring);
	free(gamestring);
	return 1;
	}


int logtofile(char *filename, char *str, char *mode)
	{
	// appends the text <str> to the file <filename>
	FILE *fp;
	int closed = 0;

	fp = fopen(filename, mode);
	if(fp == NULL)
		{
		closed = fcloseall();
		fp = fopen(filename, mode);
		if(fp == NULL)
			return 0;
		}

	fprintf(fp, "\n%s", str);
	fclose(fp);

	return 1;
	}


int texttoclipboard(char *text)
	{
	// generic text-to-clipboard: pass a text, and it gets put
	// on the clipboard
	HGLOBAL hOut;
	char *gamestring;

	// allocate memory for the game string
	hOut = GlobalAlloc(GHND|GMEM_DDESHARE, (DWORD) GAMEBUFSIZE);	 
	// and lock it
	gamestring = (char *) GlobalLock(hOut);	

	sprintf(gamestring,"%s",text);

	GlobalUnlock(hOut);
	if(OpenClipboard(NULL))
		{
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hOut);
		CloseClipboard();
		}

	return 1;
	}

char *textfromclipboard(HWND hwnd, char *str)
	{
	// read a text from the clipboard; return it if it is valid text, NULL otherwise
	// prints an error message in *str if an error occurs
	int loadok = 0;
	char *gamestring = NULL;
	char *p;
	HGLOBAL hIn;
	int i;

	gamestring = (char *) malloc(GAMEBUFSIZE);
	if(gamestring == NULL)
		{
		sprintf(str,"clipboard open failed");
		return 0;
		}

	if(OpenClipboard(hwnd))
		{
		hIn = GetClipboardData(CF_TEXT);
		if(hIn != NULL)
			{
			p =  (char *) GlobalLock(hIn);
			if(p == NULL)
				{
				sprintf(str,"globalloc failed");
				GlobalUnlock(hIn);
				free(gamestring);
				gamestring = NULL;
				CloseClipboard();
				return gamestring;
				}

			// copy data from clipboard 
			for(i=0;i<GAMEBUFSIZE-1;i++)
				{
				gamestring[i]=*p;
				if(*p==0)
					break;
				*p++;
				}
			*p=0;
			gamestring[GAMEBUFSIZE-1] = 0;
			loadok = 1;
			GlobalUnlock(hIn);
			}
		else
			{
			sprintf(str,"no valid clipboard data");
			}
		CloseClipboard();
		}
	return gamestring;
	}

int fileispresent(char *filename)
	{
	// returns 1 if a file with name "filename" is present, 0 otherwise

	FILE *fp;
	
	fp = fopen(filename,"r");
	if(fp != NULL)
		{
		fclose(fp);
		return 1;
		}
	else
		return 0;
	}

int checklevelmenu(HMENU hmenu,int item, struct CBoptions *CBoptions)
	{
	int increment;

	CheckMenuItem(hmenu,LEVELINSTANT,MF_UNCHECKED);
	CheckMenuItem(hmenu, LEVEL01S, MF_UNCHECKED);
	CheckMenuItem(hmenu, LEVEL02S, MF_UNCHECKED);
	CheckMenuItem(hmenu, LEVEL05S, MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL1S,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL2S,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL5S,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL10S,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL15S,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL30S,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL1M,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL2M,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL5M,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL15M,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVEL30M,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVELINFINITE,MF_UNCHECKED);
	CheckMenuItem(hmenu,LEVELINCREMENT,MF_UNCHECKED);
	CheckMenuItem(hmenu,item,MF_CHECKED);
	
	if(CBoptions->level==14) 
		increment=1;
	else 
		increment=0;
	
	return increment;
	}

void setmenuchecks(struct CBoptions *CBoptions, HMENU hmenu)
	{
	// set menu checks 
	if(CBoptions->priority)
		CheckMenuItem(hmenu,OPTIONSPRIORITY,MF_CHECKED);
	else
		CheckMenuItem(hmenu,OPTIONSPRIORITY,MF_UNCHECKED);
	
	if(CBoptions->highlight)
     	CheckMenuItem(hmenu,OPTIONSHIGHLIGHT,MF_CHECKED);
	else
		CheckMenuItem(hmenu,OPTIONSHIGHLIGHT,MF_UNCHECKED);
   
	if(CBoptions->sound)
		CheckMenuItem(hmenu,OPTIONSSOUND,MF_CHECKED);
	else
		CheckMenuItem(hmenu,OPTIONSSOUND,MF_UNCHECKED);
	
	if(CBoptions->invert)
		CheckMenuItem(hmenu,DISPLAYINVERT,MF_CHECKED);
	else
		CheckMenuItem(hmenu,DISPLAYINVERT,MF_UNCHECKED);
	
	if(CBoptions->mirror)
		CheckMenuItem(hmenu,DISPLAYMIRROR,MF_CHECKED);
	else
		CheckMenuItem(hmenu,DISPLAYMIRROR,MF_UNCHECKED);
	
	if(CBoptions->exact)
		CheckMenuItem(hmenu,LEVELEXACT,MF_CHECKED);
	else
		CheckMenuItem(hmenu,LEVELEXACT,MF_UNCHECKED);
	
	if(CBoptions->numbers)
		CheckMenuItem(hmenu,DISPLAYNUMBERS,MF_CHECKED);
	else
		CheckMenuItem(hmenu,DISPLAYNUMBERS,MF_UNCHECKED);

	if(CBoptions->userbook)
		CheckMenuItem(hmenu,OPTIONSUSERBOOK,MF_CHECKED);
	else
		CheckMenuItem(hmenu,OPTIONSUSERBOOK,MF_UNCHECKED);
	}


void CBlog(char *str)
{
	TCHAR path[MAX_PATH];

	// open a log file on startup
	if (cblogfile == NULL) {
		sprintf(path, "%s\\CBlog%s.txt", CBdocuments, g_app_instance_suffix);
		cblogfile = fopen(path, "w");
	}

	// the next two statements should never happen, but we check anyway.
	if (str == NULL)
		return;

	if (cblogfile == NULL)
		return;

	fprintf(cblogfile, "\n%s", str);
	fflush(cblogfile);
}


int getopening(struct CBoptions *CBoptions)
	/* chooses a 3-move opening at random. */
	{
	int op=0;
	int ok=0;
	
	srand( (unsigned)time( NULL ) );

	while(!ok)
		{
		op=random(174);
		if(three[op][3] == OP_BOARD)         
			{
			if(CBoptions->op_crossboard) 
				ok=1;
			}
		if(three[op][3] == OP_MAILPLAY)
			{
			if(CBoptions->op_mailplay) 
				ok=1;
			}
		if(three[op][3] == OP_BARRED)
			{
			if(CBoptions->op_barred) 
				ok=1;
			}
		}
	return op;
	}

int getthreeopening(int n, struct CBoptions *CBoptions)
	{
	/* n is the number of the game in the engine match. 
		getthreeopening returns the number of the opening that should
		be played in game number n depending on which subset of
		the 3-move-deck is active */
	int i;
	int m;

	/* play every opening twice */
	n = n/2;

	/* makes gamenumber: 1 2 3 4 5 6 7 8
				         n: 0 0 1 1 2 2 3 3 */
	m=-1;
	for(i=0;i<174;i++)
		{
		/* if the opening is part of the current set, increment m */
		// normal if((three[i][3]==op_crossboard) && op_crossboard) m++;
		if((three[i][3]==OP_CTD || three[i][3]==OP_BOARD) && CBoptions->op_crossboard) 
			m++;
		if((three[i][3]==OP_BARRED) && CBoptions->op_barred) 
			m++;
		if((three[i][3]==OP_MAILPLAY) && CBoptions->op_mailplay) 
			m++;
		
		/* after having found N eligible openings, our counter m is set to N-1, so
			that it runs from 0...173 at most */

		if(m==n) return i;
		}
	return -1;
	}
		
void toggle(int *x)
	{
	if(*x==0) 
		*x=1;
	else *x=0;
	}
   
int builtingametype(void)
	{
	return 21;
	}


int FENtoboard8(int board[8][8], char *p, int *color, int gametype)
	{
	/* parses the FEN string in *p and places the result in board8 and color */
	// example FEN string:
	// W:W32,31,30,29,28,27,26,25,24,22,21:B23,12,11,10,8,7,6,5,4,3,2,1.
	// returns 1 on success, 0 on failure.
	char *token;
	char *col,*white,*black;
	char FENstring[256];
	int i,j;
	int number;
	int piece;
	int length;
	char colorchar='x';
	
	
	// find the full stop in the FEN string which terminates it and 
	// replace it with a 0 for termination
	length = (int) strlen(p);
	token = p;
	i = 0;
	while(token[i] != '.' && i<length)
		i++;
	token[i] = 0;

	sprintf(FENstring,"%s",p);

	// detect empty FEN string
	if( strcmp(FENstring,"") == 0)
		return 0;

	/* parse color ,whitestring, blackstring*/
	col = strtok(FENstring,":");

	if(col == NULL)
		return 0;

	if (toupper(col[0]) == 'W')
		*color = WHITE;
	else if (toupper(col[0]) == 'B')
		*color = BLACK;
	else
		return(0);
	
	/* parse position: get white and black strings */
	
	white = strtok(NULL,":");
	if(white == NULL)
		return 0;

	// check whether this was a normal fen string (white first, then black) or vice versa.
	colorchar = white[0];
	if(colorchar == 'B' || colorchar == 'b')
		{
		black = white;
		white = strtok(NULL,":");
		if(white == NULL)
			return 0;
		// reversed fen string
		}
	else
		{
		black=strtok(NULL,":");
		if(black == NULL)
			return 0;
		}
	// example FEN string:
	// W:W32,31,30,29,28,27,26,25,24,22,21:B23,12,11,10,8,7,6,5,4,3,2,1.
	// skip the W and B characters.
	white++;
	black++;
	

	/* reset board */
	for(i=0;i<8;i++)
		{
		for(j=0;j<8;j++)
			board[i][j]=0;
		}

	/* parse white string */
	token = strtok(white,",");

	while( token != NULL )
		{
		/* While there are tokens in "string" */
		/* a token might be 18, or 18K */
		piece = WHITE|MAN;
		if(toupper(token[0]) == 'K')
			{
			piece = WHITE|KING;
			token++;
			}
		number = atoi(token);
		/* ok, piece and number found, transform number to coors */
		numbertocoors(number,&i,&j, gametype);
		board[i][j] = piece;
		/* Get next token: */
		token = strtok( NULL, "," );
		}
	/* parse black string */
	token = strtok(black,",");
	while( token != NULL )
		{
		/* While there are tokens in "string" */
		/* a token might be 18, or 18K */
		piece = BLACK|MAN;
		if(toupper(token[0]) == 'K')
			{
			piece = BLACK|KING;
			token++;
			}
		number = atoi(token);
		/* ok, piece and number found, transform number to coors */
		numbertocoors(number,&i,&j, gametype);
		board[i][j] = piece;
		/* Get next token: */
		token = strtok( NULL, "," );
		}
	return 1;
	}


void board8toFEN(int board[8][8],char *p,int color, int gametype)
	{
	int i,j,number;
	char s[256];
	/* prints a FEN string into p derived from board */
	/* sample FEN string:
		"W:W18,20,23,K25:B02,06,09."*/
	sprintf(p,"");

	if(color==BLACK)
		sprintf(s,"B:W");
	else
		sprintf(s,"W:W");
	strcat(p,s);
	for(j=7;j>=0;j--)
		{
		for(i=0;i<8;i++)
			{
			sprintf(s,"");
			number=coorstonumber(i,j, gametype);
			if(board[i][j]==(WHITE|MAN))
				sprintf(s,"%i,",number);
			if(board[i][j]==(WHITE|KING))
				sprintf(s,"K%i,",number);
			strcat(p,s);
			}
		}
	/* remove last comma */
	p[strlen(p)-1]=0;
	sprintf(s,":B");
	strcat(p,s);
	for(j=7;j>=0;j--)
		{
		for(i=0;i<8;i++)
			{
			sprintf(s,"");
			number=coorstonumber(i,j, gametype);
			if(board[i][j]==(BLACK|MAN))
				sprintf(s,"%i,",number);
			if(board[i][j]==(BLACK|KING))
				sprintf(s,"K%i,",number);
			strcat(p,s);
			}
		}
	p[strlen(p)-1]='.';
	}


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
 * Separate the filename from the path.
 * Return 0 on success, 1 if no separators are found.
 */
int extract_path(char *name, char *path)
{
	int i, len;

	len = (int)strlen(name);
	for (i = len - 1; i >= 0; --i) {
		if (name[i] == '\\' || name[i] == '/')
			break;
	}

	if (i <= 0)
		return(1);
	strncpy(path, name, i);
	path[i] = 0;
	return(0);
}

/*
void builddb(char *str)
	{
	// call the db generator if there is enough free disk space
	HINSTANCE hinst;
	int error;
	ULARGE_INTEGER FreeBytesAvailable,TotalNumberOfBytes,TotalNumberOfFreeBytes;
	__int64 freebytes;

	GetDiskFreeSpaceEx(NULL,&FreeBytesAvailable,&TotalNumberOfBytes,&TotalNumberOfFreeBytes);
	
	freebytes = (__int64)(FreeBytesAvailable.LowPart) + (((__int64)FreeBytesAvailable.HighPart)<<32);

	if(freebytes < (DWORD64)220000000)
		{
		(str,"not enough free disk space for this operation");
		return;
		}
	else
		sprintf(str,"There is enough free disk space (about %I64i MB) - building database...", freebytes/1024/1024);

	hinst = ShellExecute(NULL,"open","db\\dbgen.bat",NULL,NULL,SW_SHOW);
	error = PtrToLong(hinst);
	if (error <= 32)
		sprintf(str,"error: %i", error);
	
	return;	
	}
	*/