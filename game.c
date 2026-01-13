#include <stdio.h>
#include <stdlib.h>
#include "game.h"
#include "move.h"
#include "sow_and_capture.h"


void game_init(GameState *g) {
    board_init(&g->board);
    g->score[1] = 0; // Joueur 1
    g->score[2] = 0; // Joueur 2
    g->current = 1;   // Commencer avec le joueur 1
    g->total_moves = 0; // Reset compteur total (limite 400)
}

// Fonction pour vérifier si un joueur peut jouer 
static int can_player_move(const GameState *g, int player) {

    int start = (player == 1) ? 0 : 1; // J1: indices pairs, J2: indices impairs
    
    for(int i = start; i < N_HOLES; i += 2) {
        // Vérifie s'il y a des graines dans ce trou
        if(g->board.holes[i].red > 0 || 
           g->board.holes[i].blue > 0 || 
           g->board.holes[i].transparent > 0) {
            return 1; // Le joueur peut jouer
        }
    }
    return 0; // Le joueur ne peut pas jouer
}

// Fonction pour vérifier les conditions de fin de partie
static int check_end_conditions(const GameState *g) {
    int total_seeds = board_total_seeds(&g->board);
    
    // Limite de 400 coups total (200 par joueur)
    if (g->total_moves >= 400) {
        return 1; // Match nul par limite de coups
    }
    

    // Si strictement moins de 10 graines restent, fin de partie immédiate
    if(total_seeds < 10) {
        return 1;
    }
    
    // Si un joueur a 49 graines ou plus, il gagne
    if(g->score[1] >= 49 || g->score[2] >= 49) {
        return 1;
    }
    
    // Si les deux joueurs ont 48 graines, match nul
    if(g->score[1] == 48 && g->score[2] == 48) {
        return 1;
    }
    
    return 0;
}

// Variable globale pour activer le debug
static int DEBUG_MODE = 0;

void set_debug_mode(int enabled) {
    DEBUG_MODE = enabled;
}

// Fonction pour vérifier l'affamation (capture de toutes les graines adverses)
static void check_affamation(GameState *g, int player) {
    int opponent = (player == 1) ? 2 : 1;
    int opponent_seeds = 0;
    
 
    if(DEBUG_MODE) {
        printf("\n=== VÉRIFICATION AFFAMATION ===\n");
        printf("   Joueur actif: %d\n", player);
        printf("   Adversaire: %d\n", opponent);
    }
    
    // Compter les graines de l'adversaire
    for(int i = 0; i < N_HOLES; i++) {
        // Vérifier si ce trou appartient à l'adversaire

        int is_opponent_hole = (opponent == 1) ? (i % 2 == 0) : (i % 2 == 1);
        
        if(is_opponent_hole) {
            int seeds_in_hole = g->board.holes[i].red + 
                               g->board.holes[i].blue + 
                               g->board.holes[i].transparent;
            if(DEBUG_MODE && seeds_in_hole > 0) {
                printf("   Trou %d (adversaire): %d graines\n", i+1, seeds_in_hole);
            }
            opponent_seeds += seeds_in_hole;
        }
    }
    
    if(DEBUG_MODE) {
        printf("   Total graines adversaire: %d\n", opponent_seeds);
    }
    
    // Si l'adversaire n'a plus de graines, capturer toutes les siennes (du joueur actif)
    if(opponent_seeds == 0) {
        if(DEBUG_MODE) {
            printf("AFFAMATION DÉTECTÉE ! L'adversaire n'a plus de graines.\n");
            printf(" → Capture de TOUTES les graines du joueur %d\n", player);
        }
        
        int captured = 0;
        for(int i = 0; i < N_HOLES; i++) {
            // Vérifier si ce trou appartient au joueur actif
            int is_player_hole = (player == 1) ? (i % 2 == 0) : (i % 2 == 1);
            
            if(is_player_hole) {
                int seeds_in_hole = g->board.holes[i].red + 
                                   g->board.holes[i].blue + 
                                   g->board.holes[i].transparent;
                if(DEBUG_MODE && seeds_in_hole > 0) {
                    printf("   Trou %d: capture %d graines\n", i+1, seeds_in_hole);
                }
                captured += seeds_in_hole;
                g->board.holes[i].red = 0;
                g->board.holes[i].blue = 0;
                g->board.holes[i].transparent = 0;
            }
        }
        g->score[player] += captured;
        
        if(DEBUG_MODE) {
            printf("   Total capturé: %d graines\n", captured);
            printf("   Nouveau score joueur %d: %d\n", player, g->score[player]);
            printf("====================================\n\n");
        }
    } else if(DEBUG_MODE) {
        printf(" Pas d'affamation (adversaire a encore des graines)\n");
        printf("====================================\n\n");
    }
}

int game_move(GameState *g, const char *move_str) {
    int hole;
    Color color;
    
    // Parse le coup
    if(!parse_move(move_str, &hole, &color)) {
        return 0; // Coup invalide
    }
    
    // Vérifie que le coup est valide
    if(!valid_move(&g->board, hole, color, g->current)) {
        return 0; // Coup invalide
    }
    
    // Exécute le semis et la capture
    int score_gain = 0;
    if(!sow_and_capture(&g->board, hole, color, &score_gain, g->current)) {
        return 0; // Erreur lors du semis
    }
    
    // Incrémente le compteur total de coups
    g->total_moves++;
    

    // Ajouter les graines capturées au score
    g->score[g->current] += score_gain;
    
    // Vérifie l'affamation
    check_affamation(g, g->current);
    
    // Vérifie les conditions de fin de partie
    if(check_end_conditions(g)) {
        return 1; // Coup valide, mais partie terminée
    }
    
    // Passe au joueur suivant
    g->current = (g->current == 1) ? 2 : 1;
    
    // Si le joueur suivant ne peut pas jouer, continue avec l'autre
    if(!can_player_move(g, g->current)) {
        check_affamation(g, g->current); // On capture tout le reste
        return 1;
    }
    
    return 1; // Coup valide
}

int game_over(const GameState *g) {
    return check_end_conditions(g);
}

// Fonction pour obtenir le résultat de la partie
int get_game_result(const GameState *g) {
    if(g->score[1] > g->score[2]) {
        return 1; // Joueur 1 gagne
    } else if(g->score[2] > g->score[1]) {
        return 2; // Joueur 2 gagne
    } else {
        return 0; // Match nul
    }
}

// Fonction pour obtenir le score d'un joueur
int get_player_score(const GameState *g, int player) {
    if(player == 1 || player == 2) {
        return g->score[player];
    }
    return 0;
}
