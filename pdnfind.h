#pragma once
#include <vector>


// pdn find structures 

struct PDN_position {
	uint32_t black;
	uint32_t white;
	uint32_t kings;
	unsigned int gameindex:28;
	unsigned int result:2;
	unsigned int color:2;	
};

int pdnfind(pos *position, int color, std::vector<int> &preview_to_game_index_map);
int pdnfindtheme(pos *position, std::vector<int> &preview_to_game_index_map);
int pdnopen(char filename[MAX_PATH], int gametype);

