// CheckerBoard.h contains definitions relevant
// for the file CheckerBoard.c.  common constants
// relevant for the entire program are defined in
// CBconsts.h

#pragma once
#include <vector>

// version 
#define VERSION "1.73e"
#define PLACE "October 31, 2017"

#define OP_BOARD 0			// different opening decks
#define OP_MAILPLAY 1
#define OP_BARRED 2
#define OP_CTD 3

#define NUMBUTTONS 23		//number of buttons in toolbar
#define MAXUSERBOOK 10000

#define SLEEPTIME 20		// number of ms to sleep
#define AUTOSLEEPTIME 20	// run autothread at 50 Hz

#define OF_SAVEGAME 0
#define OF_LOADGAME 1
#define OF_SAVEASHTML 2
#define OF_USERBOOK 3
#define OF_BOOKFILE 4		/* Opening book filenme. */

#define MAXPIECESET 16


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

// window functions
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WindowFunc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK CB_edit_func(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// thread functions
DWORD AutoThreadFunc(LPVOID param);
DWORD SearchThreadFunc(LPVOID param);

// my functions in alphabetical list
void abortengine();
int addmovetouserbook(int b[8][8], CBmove *move);
void add_piecesets_to_menu(HMENU hmenu);
void appendmovetolist(CBmove &move);
int builtinislegal(int board[8][8], int color, int from, int to, CBmove *move);
int changeCBstate(int newstate);
HWND CreateAToolBar(HWND hwndParent);
int createcheckerboard(HWND hwnd);
bool doload(PDNgame *PDNgame, const char *gamestring, int *color, int board8[8][8], std::string &errormsg);
int domove(CBmove m, int b[8][8]);
int dostats(int result, int movecount, int gamenumber, emstats_t *stats);
void emlog_filename(char *filename);
void empdn_filename(char *filename);
void emprogress_filename(char *filename);
void emstats_filename(char *filename);
int enginecommand(char command[256], char reply[ENGINECOMMAND_REPLY_SIZE]);
int enginename(char str[MAXNAME]);
void get_game_clocks(double *black_clock, double *white_clock);
void get_pdnsearch_stats(std::vector<gamepreview> &previews, RESULT &res);
int get_startcolor(int gametype);
int getfilename(char filename[255], int what);
int getanimationbusy(void);
int getenginebusy(void);
int getenginestarting(void);
int getmovelist(int color, CBmove movelist[MAXMOVES], int b[8][8], int *isjump);
int gametype(void);
int handlegamereplace(int replaceindex, char *databasename);
int handlesetupcc(int *color);
int handletimer(void);
int handle_lbuttondown(int x, int y);
int handle_rbuttondown(int x, int y);
void InitCheckerBoard(int b[8][8]);
void initengines(void);
int is_mirror_gametype(int gametype);
void loadengines(char *pri_fname, char *sec_fname);
HWND InitHeader(HWND hwnd);
void InitStatus(HWND hwnd);
int loadgamefromPDNstring(int gameindex, char *dbstring);
int loadnextgame(void);
int loadpreviousgame(void);
char *loadPDNdbstring(char *dbname);
int makeanalysisfile(char *filename);
bool match_is_resumable(void);
void move4tonotation(CBmove, char str[80]);
void newgame(void);
int num_ballots(void);
void PDNgametoPDNstring(PDNgame &game, std::string &pdnstring, char *lineterm);
bool pdntogame(PDNgame &game, int startposition[8][8], int startcolor, std::string &errormsg);
int read_match_stats(void);
void reset_match_stats(void);
void setcurrentengine(int engine);
int SetMenuLanguage(int language);
int selectgame(int how);
int setanimationbusy(int value);
int setenginebusy(int value);
int setenginestarting(int value);
int showfile(char *filename);
int start3move(int opening_index);
int undomove(CBmove m, int b[8][8]);

extern char CBdirectory[MAX_PATH];	// holds the directory from where CB is started:
extern char CBdocuments[MAX_PATH];

#define random(x) (rand() % x);

#define TOGGLEBOOK 1000
#define TOGGLEMODE 1001
#define TOGGLEENGINE 1002
#define LOADENGINES 1003

#define GAMENEW 101
#define GAME3MOVE 102
#define GAMELOAD 103
#define GAMESAVE 104
#define GAMEINFO 105
#define GAMEANALYZE 106
#define GAMECOPY 107
#define GAMEPASTE 108
#define GAMEEXIT 109
#define GAMEDATABASE 110
#define START3MOVE 111
#define DOSAVE 112
#define GAMEREPLACE 113
#define GAMEFIND 114
#define GAMEFINDCR 123
#define GAMEFINDTHEME 115
#define GAMESAVEASHTML 116
#define DOSAVEHTML 117
#define DIAGRAM 118
#define GAMEFINDPLAYER 119
#define SEARCHMASK 119
#define GAME_FENTOCLIPBOARD 120
#define GAME_FENFROMCLIPBOARD 121
#define SELECTUSERBOOK 122
#define RE_SEARCH 124
#define LOADNEXT 125
#define LOADPREVIOUS 126
#define GAMEANALYZEPDN 127
#define SAMPLEDIAGRAM 128

#define MOVESPLAY 201
#define MOVESBACK 202
#define MOVESFORWARD 203
#define MOVESBACKALL 204
#define MOVESFORWARDALL 205
#define MOVESCOMMENT 207
#define INTERRUPTENGINE 206
#define ABORTENGINE 208

// options menu
#define LEVELEXACT 340
#define LEVELINSTANT 341
#define LEVEL01S 357
#define LEVEL02S 358
#define LEVEL05S 359
#define LEVEL1S 342
#define LEVEL2S 343
#define LEVEL5S 344
#define LEVEL10S 356
#define LEVEL15S 345
#define LEVEL30S 346
#define LEVEL1M 347
#define LEVEL2M 348
#define LEVEL5M 349
#define LEVEL15M 350
#define LEVEL30M 351
#define LEVELINFINITE 352
#define LEVELINCREMENT 353
#define LEVELADDTIME 354
#define LEVELSUBTRACTTIME 355
#define PIECESET 380

#define OPTIONSHIGHLIGHT 322
#define OPTIONSSOUND 323
#define OPTIONSPRIORITY 324
#define OPTIONS3MOVE 40001
#define OPTIONSDIRECTORIES 40002
#define OPTIONSUSERBOOK 325
#define OPTIONSLANGUAGEENGLISH 370
#define OPTIONSLANGUAGEESPANOL 371
#define OPTIONSLANGUAGEITALIANO 372
#define OPTIONSLANGUAGEDEUTSCH 373
#define OPTIONSLANGUAGEFRANCAIS 374
#define DISPLAYINVERT 308
#define DISPLAYNUMBERS 309
#define DISPLAYMIRROR 3091
#define CM_NORMAL 310
#define CM_ANALYSIS 311
#define GOTONORMAL 312
#define CM_AUTOPLAY 313
#define CM_2PLAYER 315
#define ENGINEVSENGINE 316

#define COLORBOARDNUMBERS 330
#define COLORHIGHLIGHT 331

#define BOOKMODE_VIEW 360
#define BOOKMODE_ADD 361
#define BOOKMODE_DELETE 362


// help menu

#define HELPHELP 401
#define HELPABOUT 402
#define HELPCHECKERSINANUTSHELL 403
#define HELPHOMEPAGE 405
#define PROBLEMOFTHEDAY 406
#define ONLINEUPGRADE 407

#define SETUPMODE 501
#define SETUPCLEAR 502
#define SETUPBLACK 503
#define SETUPWHITE 504
#define SETUPCC 505

#define ENGINESELECT 600
#define ENGINEABOUT 602
#define ENGINEHELP 603
#define ENGINEOPTIONS 604

#define CM_ENGINEMATCH 800
#define CM_ADDCOMMENT 802
#define ENGINEEVAL 803
#define CM_ENGINECOMMAND 804
#define CM_RUNTESTSET 805
#define CM_HANDICAP 806

#define CM_BUILDEGDB 900

#define IDTB_BMP 300
#define ID_TOOLBAR 200
