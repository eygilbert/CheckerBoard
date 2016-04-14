
// CBconsts.h defines constants that are used throughout many
// source files

// bitmap size
#define BMPSIZE 128 

// board representations
#define FREE 0
#define WHITE 1
#define BLACK 2
#define CHANGECOLOR 3
#define MAN 4
#define KING 8

// return values of getmove
// and also defines for the result in PDNgame 
#define DRAW 0
#define WIN 1
#define LOSS 2
#define UNKNOWN 3

// game type definitions
#define GT_ENGLISH 21
#define GT_ITALIAN 22
#define GT_SPANISH 24
#define GT_RUSSIAN 25
#define GT_BRAZILIAN 26
#define GT_CZECH 29

// language definitions
#define ENGLISH 0
#define ESPANOL 1
#define ITALIANO 2
#define DEUTSCH 3
#define FRANCAIS 4

#define NEMESIS // parse nemesis-style comments ()
#define MAXMOVES 28

#define GAMEBUFSIZE 65536
