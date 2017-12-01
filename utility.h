#pragma once
#include <Windows.h>
#include <cstdarg>
#include "CBstructs.h"

enum READ_TEXT_FILE_ERROR_TYPE {
	RTF_NO_ERROR, RTF_FILE_ERROR, RTF_MALLOC_ERROR
};

int builtingametype(void);
void CBlog(char *text);
void cblog(const char *fmt, ...);
void checklevelmenu(CBoptions *options, HMENU hmenu, int resource);
int extract_path(char *name, char *path);
int FENtoclipboard(HWND hwnd, int board8[8][8], int color, int gametype);
int fileispresent(char *filename);
int getopening(CBoptions *CBoptions);
int getthreeopening(int n, CBoptions *CBoptions);
int initcolorstruct(HWND hwnd, CHOOSECOLOR *ccs, int index);
int logtofile(char *filename, char *str, char *mode);
int num_3move_ballots(CBoptions *options);
int PDNtoclipboard(HWND hwnd, PDNgame &game);
void setmenuchecks(CBoptions *CBoptions, HMENU hmenu);
char *textfromclipboard(HWND hwnd, char *str);
int texttoclipboard(const char *text);
double timelevel_to_time(int level);
int timelevel_to_token(int level);
int timetoken_to_level(int token);
double timetoken_to_time(int token);
void toggle(int *x);
int writefile(char *filename, char *mode, char *fmt, ...);
char *read_text_file(char *filename, READ_TEXT_FILE_ERROR_TYPE &etype);
