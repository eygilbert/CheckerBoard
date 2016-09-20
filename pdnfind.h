#pragma once
#include <vector>


// pdn find structures 

struct PDN_position
	{
	uint32_t black;
	uint32_t white;
	uint32_t kings;
	unsigned int gameindex:28;
	unsigned int result:2;
	unsigned int color:2;	
	};

int pdnfind(struct pos *position, int color, std::vector<int> &preview_to_game_index_map);
int pdnfindtheme(struct pos *position, std::vector<int> &preview_to_game_index_map);
int pdnopen(char filename[256], int gametype);
void reset_pdn_positions();


/* square definitions: a piece on square n in normal checkers notation can be accessed with SQn*/
#define SQ1  0x00000008
#define SQ2  0x00000004
#define SQ3  0x00000002
#define SQ4  0x00000001
#define SQ5  0x00000080
#define SQ6  0x00000040
#define SQ7  0x00000020
#define SQ8  0x00000010
#define SQ9  0x00000800
#define SQ10 0x00000400
#define SQ11 0x00000200
#define SQ12 0x00000100
#define SQ13 0x00008000
#define SQ14 0x00004000
#define SQ15 0x00002000
#define SQ16 0x00001000
#define SQ17 0x00080000
#define SQ18 0x00040000
#define SQ19 0x00020000
#define SQ20 0x00010000
#define SQ21 0x00800000
#define SQ22 0x00400000
#define SQ23 0x00200000
#define SQ24 0x00100000
#define SQ25 0x08000000
#define SQ26 0x04000000
#define SQ27 0x02000000
#define SQ28 0x01000000
#define SQ29 0x80000000
#define SQ30 0x40000000
#define SQ31 0x20000000
#define SQ32 0x10000000