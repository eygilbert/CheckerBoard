int coortonumber(coor c, int gametype);
void numbertocoors(int number, int *x, int *y, int gametype);
int coorstonumber(int x, int y, int gametype);
void coorstocoors(int *x, int *y, int invert, int mirror);

inline bool is_valid_board8_square(int x, int y)
{
	return(((x + y + 1) % 2) != 0);
}