// PDNparser.c
// a PDN parser library
// by martin fierz with some help by ed gilbert
// originally written on 5th may 2001
#include <stdio.h>
#include "standardheader.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"
#include "PDNparser.h"
#include "utility.h"

#define NEMESIS // enables detection of comments in round braces ( )

int PDNparseGetnumberofgames(char *filename)
{
	// returns the number of games in a PDN file
	char *buffer;
	std::string game;
	char *p;
	int ngames;
	READ_TEXT_FILE_ERROR_TYPE etype;

	buffer = read_text_file(filename, etype);
	if (buffer == nullptr)
		return -1;

	p = buffer;
	ngames = 0;
	while (PDNparseGetnextgame(&p, game))
		++ngames;

	free(buffer);
	return(ngames);
}

inline bool is_pdnquote(uint8_t c)
{
	if (c == '"')
		return(true);
	if (c & 0x80) {
		if (c == UTF8_LEFT_DBLQUOTE || c == UTF8_RIGHT_DBLQUOTE)
			return(true);
	}

	return(false);
}

/*
 * Append length characters from src to dest.
 */
void string_append(std::string &dest, char *src, size_t length)
{
	char savechar;

	/* Temporarily terminate src so we can treat src as a null-terminated C string. */
	savechar = src[length];
	src[length] = 0;
	dest += src;
	src[length] = savechar;
}

int PDNparseGetnextgame(char **start, std::string &game)
{

	/* searches a game in buffer, starting at **start. a 
		game is defined as everything between **start and
		the first occurrence of one of the four game 
		terminators (1-0 0-1 1/2-1/2 *). since the game 
		terminators also appear in headers [HEADER], 
		getnextgame skips headers. it also skips comments {COMMENT}
		if the function succeeds, **start points to the next character
		after the game returned in *game.
		*/

	// new 15. 8. 2002: try to recognize the next set of headers as terminators.
	// new 6.9. 2002: the way it was up to now, pdnparsenextgame would just
	// run infinitely on the last game!
	char *p;
	char *p_org;
	int headersdone = 0;

	game.clear();
	if ((*start) == 0)
		return 0;

	p = (*start);
	p_org = p;
	while (*p != 0) {

		/* skip headers */
		if (*p == '[' && !headersdone) {
			p++;
			while (*p != ']' && *p != 0) {

				/* Ignore anything inside quotes (e.g. ']') within headers. */
				if (is_pdnquote(*p)) {
					++p;
					while (!is_pdnquote(*p) && *p != 0) {
						++p;
					}

					if (*p == 0)
						break;
				}

				p++;
			}
		}

		if (*p == 0)
			break;

		/* skip comments */
		if (*p == '{') {
			p++;
			while (*p != '}' && *p != 0) {
				p++;
			}
		}

#ifdef NEMESIS
		// skip comments, nemesis style
		if (*p == '(') {
			p++;
			while (*p != ')' && *p != 0) {
				p++;
			}
		}
#endif
		if (*p == 0)
			break;

		// try to detect whether we are through with the headers
		if (isdigit((uint8_t) * p))
			headersdone = 1;

		/* check for game terminators*/
		if (p[0] == '[' && headersdone) {
			p--;
			string_append(game, *start, (p - *start));
			*start = p;
			return (int)(p - p_org);
		}

		if (p[0] == '1' && p[1] == '-' && p[2] == '0') {
			p += 3;
			string_append(game, *start, (p -*start));
			*start = p;
			return (int)(p - p_org);
		}

		if (p[0] == '0' && p[1] == '-' && p[2] == '1' && !isdigit((uint8_t) p[3])) {
			p += 3;
			string_append(game, *start, (p - *start));
			*start = p;
			return (int)(p - p_org);
		}

		if (p[0] == '*') {
			p++;
			string_append(game, *start, (p - *start));
			*start = p;
			return (int)(p - p_org);
		}

		if (p[0] == '1' && p[1] == '/' && p[2] == '2' && p[3] == '-' && p[4] == '1' && p[5] == '/' && p[6] == '2') {
			p += 7;
			string_append(game, *start, (p - *start));
			*start = p;
			return (int)(p - p_org);
		}

		p++;
	}

	if (headersdone) {
		string_append(game, *start, (p - *start));
		*start = p;
		return (int)(p - p_org);
	}

	return 0;
}

int PDNparseGetnextheader(const char **start, char *header)
{
	/* getnextheader */

	/* searches a header in buffer, starting at **start. a header
	is defined as the next complete string which is enclosed 
	between square brackets [ HEADER ]. 
	if no header is found, getnextheader
	returns 0. 
	if a header is found, getnextheader sets **start
	to the next character after the header.
	the header is returned in *header */
	const char *p, *q;
	int i, quotecount;

	if (*start == 0)
		return 0;
	p = *start;
	while (*p != '[' && *p != 0)
		p++;

	/* if no opening brace is found... */
	if (*p == 0)
		return 0;

	q = p + 1;
	i = 0;
	quotecount = 0;
	while ((quotecount < 2 || *q != ']') && *q != 0) {
		header[i] = *q;
		if (*q == '"')
			++quotecount;
		q++;
		i++;
		if (i >= MAXNAME)
			return(0);
	}

	// terminate header with a 0
	header[i] = 0;

	/* if no closing brace is found */
	if (*q == 0)
		return 0;

	/* ok, we have found a header it is written to *header, now 
		we set the start pointer which tells where to continue searching*/
	*start = q + 1;
	return 1;
}

int PDNparseGetnexttag(const char **start, char *tag)
{
	/* getnexttag */

	/* searches a tag in buffer, starting at **start. a tag
	is defined as the next complete string which is enclosed 
	between "s - "TAG". 
	if no tag is found, getnexttag
	returns 0. 
	if a tag is found, getnexttag sets **start
	to the next character after the header.
	the tag is returned in *tag */
	const char *p, *q;
	int i;

	if ((*start) == 0)
		return 0;
	p = (*start);
	while (!is_pdnquote(*p) && *p != 0)
		p++;

	/* if no opening " is found... */
	if (*p == 0)
		return 0;

	q = p + 1;
	i = 0;
	while (!is_pdnquote(*q) && *q != 0) {
		tag[i] = *q;
		q++;
		i++;
	}

	tag[i] = 0;

	/* if no closing " is found */
	if (*q == 0)
		return 0;

	/* ok, we have found a tag, it is written to *tag, now 
		we set the start pointer */
	(*start) = q + 1;
	return 1;
}

inline int is_pdnspace(uint8_t val)
{
	return(isspace(val) || val == UTF8_NOBREAK_SPACE);
}

inline int is_pdn_move_sep(int val)
{
	return(val == '-' || tolower(val) == 'x');
}

inline void trim_trailing_whitespace(char *buf, int len)
{
	char *p;

	for (p = buf + len; p > buf && is_pdnspace(*(p - 1)); --p)
		;
	*p = 0;
}

/*
 * Extract the next token from a pdn buffer.
 * A token is one of:
 *	- a header start or end bracket [ or ]
 *	- a name in a header, such as the word Date in [Date "1/2/1945"]
 *	- a quoted value in a header, such as the string "1/2/1945" in the above example.
 *	- a move.  Examples 21-18 2x11x18 1 - 5
 *	- a comment in curly braces or parentheses, such as {this move loses}
 *	- a game result, such as * or 1/2-1/2
 *
 * If a token is found, it is copied into the token buffer, and *start
 * is set to point to the next character after the token.
 *
 * The function return value is true if a token is successfully parsed.
 * If we reach the end of the pdn buffer and found nothing but whitespace, the return value is false.
 */
int PDNparseGetnextPDNtoken(const char **start, char *token)
{
	int len = 0;
	PDN_PARSE_STATE state;
	int tokentype = PDN_DONE;
	const char *p, *possible_end, *tok_start;

	/* Skip past leading white space. */
	p = (*start);
	while (is_pdnspace(*p))
		++p;

	if (!*p)
		return(0);

	state = PDN_IDLE;
	possible_end = 0;
	tok_start = p;
	while (*p && state != PDN_DONE) {
		switch (state) {
		case PDN_IDLE:
			/* We are only in idle until we see the first non-space. */
			if (isdigit((uint8_t)*p))
				state = PDN_READING_FROM;
			else if (*p == '{')
				state = PDN_CURLY_COMMENT;
#ifdef NEMESIS
			else if (*p == '(')
				state = PDN_NEMESIS_COMMENT;
#endif
			else if (!is_pdnspace(*p))
				state = PDN_FLUFF;
			++p;
			break;

		case PDN_FLUFF:
			/* We saw something that is not strict pdn.  Keep tossing this stuff
			 * until we see something we recognize, then return the fluff as a non-move.
			 */
			tokentype = PDN_FLUFF;
			if (isdigit((uint8_t)*p) || *p == '{' || *p == '(' || is_pdnquote(*p)) {
				state = PDN_DONE;
				len = (int)(p - tok_start);
				memcpy(token, tok_start, len);
				token[len] = 0;
				*start = p;
			}
			else
				++p;
			break;

		case PDN_CURLY_COMMENT:
			/* Reading a comment in curly braces. */
			if (*p == '}') {
				state = PDN_DONE;
				++p;
				len = (int)(p - tok_start);
				memcpy(token, tok_start, len);
				token[len] = 0;
				*start = p;
			}
			else
				++p;
			break;

#ifdef NEMESIS

		/* check for comment nemesis-style*/
		case PDN_NEMESIS_COMMENT:
			/* Reading a comment in parenthesis. */
			if (*p == ')') {
				state = PDN_DONE;
				++p;
				len = (int)(p - tok_start);
				memcpy(token, tok_start, len);
				token[len] = 0;
				*start = p;
			}
			else
				++p;
			break;
#endif

		case PDN_READING_FROM:
			if (isdigit((uint8_t)*p))
				++p;

			/* If we get a forward slash then its a good chance we have a game draw result. */
			else if (*p == '/' && strncmp(p - 1, "1/2-1/2", 7) == 0) {
				state = PDN_DONE;
				len = 7;
				memcpy(token, p - 1, len);
				token[len] = 0;
				*start = p + 6;
			}
			else
				state = PDN_WAITING_SEP;
			break;

		case PDN_WAITING_SEP:
			/* Here we allow white space or a move separator.  Anything else
			 * means its not a move and we call it fluff.
			 */
			if (is_pdnspace(*p))
				++p;
			else if (is_pdn_move_sep(*p)) {
				++p;
				state = PDN_WAITING_TO;
			}
			else {

				/* Its not a space or move separator, so this is non-pdn fluff. */
				state = PDN_FLUFF;
			}
			break;

		case PDN_WAITING_TO:
			/* Here we allow white space or a move number.  Anything else 
			 * means its not a move and we call it fluff.
			 */
			if (isdigit((uint8_t)*p)) {
				++p;
				state = PDN_READING_TO;
			}
			else if (is_pdnspace(*p))
				++p;
			else {

				/* Its not a space or move separator, so this is non-pdn fluff. */
				state = PDN_FLUFF;
			}
			break;

		case PDN_READING_TO:
			if (isdigit((uint8_t)*p))
				++p;
			else if (is_pdn_move_sep(*p)) {
				possible_end = p;	/* Remember in case this was the end of the move. */
				++p;
				state = PDN_WAITING_OPTIONAL_TO;
			}
			else if (is_pdnspace(*p)) {

				/* This is normally the end of the move, but maybe its just a space
				 * before another jump move separator. 
				 */
				possible_end = p;	/* Remember in case this was the end of the move. */
				++p;
				state = PDN_WAITING_OPTIONAL_SEP;
			}
			else {

				/* Finished reading a valid move. */
				state = PDN_DONE;
				len = (int)(p - tok_start);
				memcpy(token, tok_start, len);
				token[len] = 0;
				*start = p;
			}
			break;

		case PDN_WAITING_OPTIONAL_SEP:
			if (is_pdnspace(*p))
				++p;
			else if (is_pdn_move_sep(*p)) {
				++p;
				state = PDN_WAITING_OPTIONAL_TO;
			}
			else {

				/* No move separator, roll back to the end of move. */
				state = PDN_DONE;
				len = (int)(possible_end - tok_start);
				memcpy(token, tok_start, len);
				token[len] = 0;
				*start = possible_end;
			}
			break;

		case PDN_WAITING_OPTIONAL_TO:
			/* Here we allow white space or a move number.  Anything else 
			 * means its not more jump moves and we roll back to the end of the valid move.
			 */
			if (isdigit((uint8_t)(*p))) {

				/* This is now part of a good move, so we can cancel any previous rollback point. */
				possible_end = 0;
				++p;
				state = PDN_READING_TO;
			}
			else if (is_pdnspace(*p))
				++p;
			else {

				/* We did not get another 'to' square.  Return the valid move that we already passed. */
				state = PDN_DONE;
				len = (int)(possible_end - tok_start);
				memcpy(token, tok_start, len);
				token[len] = 0;
				*start = possible_end;
			}
			break;
		}
	}

	/* We hit the end of the pdn buffer while parsing.
	 * Finish up whatever we were doing.
	 */
	if (!*p) {
		if
		(
			state == PDN_WAITING_SEP ||
			state == PDN_WAITING_TO ||
			state == PDN_WAITING_OPTIONAL_SEP ||
			state == PDN_WAITING_OPTIONAL_TO
		) {
			if (possible_end) {
				len = (int)(possible_end - tok_start);
				memcpy(token, tok_start, len);
				token[len] = 0;
				*start = possible_end;
			}
			else {
				len = (int)(p - tok_start);
				memcpy(token, tok_start, len);
				token[len] = 0;
				*start = p;
			}

			/* Trim any trailing whitespace in token. */
			trim_trailing_whitespace(token, len);
			return(tokentype);
		}

		len = (int)(p - tok_start);
		memcpy(token, tok_start, len);
		token[len] = 0;
		*start = p;
	}

	/* Trim any trailing whitespace in token. */
	trim_trailing_whitespace(token, len);

	return(tokentype);
}

/*
 * Parse a substring that may contain a checkers move, e.g. 17-21 or 2x10, or
 * a fully qualified jump move, e.g. 8x15x24x31x22.
 * If a valid move is parsed, return the list of square numbers in vector move, and the function return value is true.
 * If a valid move is not parsed, the function return value is false.
 * Allow white space anywhere between the numbers and the move separator, e.g. 1 - 5
 * Allow non-numeric junk that might be appended to the tail end of the 'to' square, e.g. 17-21,foo.
 * Allow non-numeric junk that might come before the move, as in hello2-6
 */
int PDNparseMove(char *token, squarelist &move)
{
	int i, square;
	int len;
	PDN_PARSE_STATE state;

	len = (int)strlen(token);
	move.size = 0;
	for (i = 0, state = PDN_IDLE; i < len;) {
		switch (state) {
		case PDN_IDLE:
			/* In idle we have't seen the beginning of a move yet.  Skip past anything
			 * that isn't a number.
			 */
			if (isdigit((uint8_t) token[i])) {
				square = token[i] - '0';
				state = PDN_READING_FROM;
			}
			else if (token[i] == '(' || token[i] == '{')
				/* The token parser breaks out these kinds of comments separately, so if we
				 * have one of these then there is no move.
				 */
				return(0);
			++i;
			break;

		case PDN_READING_FROM:
			/* Take more digits of the from square, else defer to the state machine
			 * to handle anything that isn't a digit.
			 */
			if (isdigit((uint8_t) token[i])) {
				square = 10 * square + token[i] - '0';
				++i;
			}
			else if (token[i] == '/')
				return(0);	/* dont allow slashes in moves, its probably a 1/2-1/2. */
			else {
				move.squares[move.size] = square;
				++move.size;
				state = PDN_WAITING_SEP;
			}
			break;

		case PDN_WAITING_SEP:
			/* Ignore anything but a move separator here. */
			if (is_pdn_move_sep(token[i]))
				state = PDN_WAITING_TO;
			++i;
			break;

		case PDN_WAITING_TO:
			/* Ignore anything but a digit here. */
			if (isdigit((uint8_t) token[i])) {
				square = token[i] - '0';
				state = PDN_READING_TO;
			}

			++i;
			break;

		case PDN_READING_TO:
			/* Continue reading the digits of the to square. */
			if (isdigit((uint8_t) token[i])) {
				square = 10 * square + token[i] - '0';
				++i;
			}
			else {
				move.squares[move.size] = square;
				++move.size;
				state = PDN_WAITING_OPTIONAL_SEP;
			}
			break;

		case PDN_WAITING_OPTIONAL_SEP:
			/* We finished reading a to square, but more might be coming if its a multi-jump. */
			if (is_pdn_move_sep(token[i]))
				state = PDN_WAITING_TO;
			++i;
			break;
		}
	}

	/* If we left in a reasonable termination state, then it's a complete and valid move. */
	if (state == PDN_WAITING_OPTIONAL_SEP)
		return(1);
	else if (state == PDN_READING_TO) {
		move.squares[move.size] = square;
		++move.size;
		return(1);
	}
	else
		return(0);			/* not a move. */
}

int PDNparseGetnexttoken(const char **start, char *token)
{
	/*getnexttoken 
	gets the next token in buffer, starting at start. a token
	is defined as the next character sequence till a whitespace.
	a . ends a token
	special case: a curly brace { starting a token makes the
	token last until the curly brace is closed again. 

	if no token is found, getnexttoken
	returns 0. 
	if a token is found, getnexttoken sets **start
	to the next character after the token.
	the token is returned in *token */
	const char *p, *q;
	int i;

	if ((*start) == 0)
		return 0;
	p = (*start);

	// skip leading whitespace characters
	while (is_pdnspace((uint8_t)*p))
		*p++;

	i = 0;
	q = p;

	// check for comment
	if (*p == '{') {

		// comment
		while (*p != '}' && *p != 0) {
			token[i] = *p;
			p++;
			i++;
		}

		*start = p + 1;
		token[i] = '}';
		token[i + 1] = 0;
		return 1;
	}

#ifdef NEMESIS
	// check for comment nemesis-style
	if (*p == '(') {

	// comment
		while (*p != ')' && *p != 0) {
			token[i] = *p;
			p++;
			i++;
		}

		*start = p + 1;
		token[i] = ')';
		token[i + 1] = 0;
		return 1;
	}
#endif
	else {

		// normal token
		while (!is_pdnspace((uint8_t) * p) && *p != 0 && *p != '.') {
			token[i] = *p;
			p++;
			i++;
		}
	}

	// if no end of token is found
	if (*p == 0)
		return 0;

	// if we terminated with a full stop (.) ,add it
	if (*p == '.') {
		token[i] = *p;
		p++;
		i++;
	}

	token[i] = 0;

	// we have found a token, it is written to *token, now
	//	we set the start pointer
	(*start) = p + 1;
	return 1;
}
