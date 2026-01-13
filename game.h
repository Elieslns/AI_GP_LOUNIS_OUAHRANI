#ifndef GAME_H
#define GAME_H

#include "board.h"

typedef struct {
    Board board;
    int score[3];   // index 1 et 2 pour joueurs
    int current;    // 1 ou 2
    int half_moves_without_capture; // Compteur pour la règle des 50 coups
    int total_moves; // Compteur total de coups joués (limite 400)
} GameState;

void game_init(GameState *g);
int  game_move(GameState *g, const char *move_str);
int  game_over(const GameState *g);
int  get_game_result(const GameState *g);
int  get_player_score(const GameState *g, int player);

// Mode debug
void set_debug_mode(int enabled);

#endif // GAME_H
