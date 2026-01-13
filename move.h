#ifndef MOVE_H
#define MOVE_H

#include "board.h"

typedef enum { RED, BLUE, T_AS_RED, T_AS_BLUE } Color;

int parse_move(const char *str, int *hole, Color *c);
int valid_move(const Board *b, int hole, Color c, int player);

#endif // MOVE_H
