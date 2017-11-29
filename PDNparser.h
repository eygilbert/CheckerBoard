#pragma once
#include <string>
#include <vector>
#include "CBstructs.h"


#define UTF8_LEFT_DBLQUOTE 147
#define UTF8_RIGHT_DBLQUOTE 148
#define UTF8_NOBREAK_SPACE 160

typedef enum {
	PDN_IDLE, PDN_READING_FROM, PDN_WAITING_SEP, PDN_WAITING_TO, PDN_READING_TO, PDN_WAITING_OPTIONAL_TO,
	PDN_WAITING_OPTIONAL_SEP, PDN_CURLY_COMMENT, PDN_NEMESIS_COMMENT, PDN_FLUFF, PDN_QUOTED_VALUE, PDN_DONE
} PDN_PARSE_STATE;

int PDNparseGetnextgame(char **start, std::string &game);		/* gets whats between **start and game terminator */
int PDNparseGetnextheader(const char **start,char *header);		/* gets whats betweeen [] from **start */
int PDNparseGetnexttag(const char **start,char *tag);			/* gets whats between "" from **start */
int PDNparseMove(char *token, Squarelist &move);				/* gets move as a list of squares. */
int PDNparseGetnexttoken(const char **start, char *token);		/* gets the next token from **start */
int PDNparseGetnextPDNtoken(const char ** start, char *token);
int PDNparseGetnumberofgames(char *filename);

