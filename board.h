#ifndef BOARD_H
#define BOARD_H

#define N_HOLES 16

typedef struct {
    int red;
    int blue;
    int transparent;
} Hole;

typedef struct {
    Hole holes[N_HOLES];
} Board;

void board_init(Board *b);
int  board_total_seeds(const Board *b);

#endif // BOARD_H
