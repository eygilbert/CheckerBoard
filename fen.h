#pragma once
#include <string>

int is_fen(const char *buf);
int FENtoboard8(Board8x8 board, const char *fenstr, int *color, int gametype);
void board8toFEN(const Board8x8 board, std::string &fenstr, int color, int gametype);
void board8toFEN(const Board8x8 board, char *fenstr, int color, int gametype);
