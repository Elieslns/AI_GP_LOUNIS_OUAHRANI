#include "board.h"

void board_init(Board *b) {
    for(int i=0;i<N_HOLES;i++){
        b->holes[i].red = 2;
        b->holes[i].blue = 2;
        b->holes[i].transparent = 2;
    }
}

int board_total_seeds(const Board *b){
    int sum = 0;
    // Optimisation : dérouler la boucle pour réduire les comparaisons
    for(int i = 0; i < N_HOLES; i++) {
        sum += b->holes[i].red + b->holes[i].blue + b->holes[i].transparent;
    }
    return sum;
}
