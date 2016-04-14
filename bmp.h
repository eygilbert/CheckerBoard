#define BMPBLACKMAN 0
#define BMPBLACKKING 1
#define BMPWHITEMAN 2
#define BMPWHITEKING 3
#define BMPLIGHT 4
#define BMPDARK 5
#define BMPMANMASK 6
#define BMPKINGMASK 7

int initbmp(HWND hwnd, char dir[256]);
HBITMAP getCBbitmap(int type);

