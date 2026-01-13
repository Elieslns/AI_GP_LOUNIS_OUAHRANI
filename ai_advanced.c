#include "ai_interface.h"
#include "ai.h"
#include "sow_and_capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

// CONSTANTES ET VARIABLES GLOBALES


// Profondeur max qu'on s'autorise à explorer
#define MAX_DEPTH 30

// Nombre de killer moves qu'on garde en mémoire par profondeur
#define MAX_KILLER_MOVES 2

// Valeurs pour l'infini et la victoire
#define INFINITY_SCORE 1000000
#define WIN_SCORE 100000

// Variables pour gérer le temps de réflexion
static clock_t search_end_time;
static bool stop_search = false;

// Tableaux pour les heuristiques de tri 
static AIMove killer_moves[MAX_DEPTH][MAX_KILLER_MOVES];
static int history_scores[16][4];

// Structure pour garder des stats (juste pour nous, pour débugger)
static struct {
    uint64_t nodes_searched;
    uint64_t tt_hits;
    uint64_t tt_cutoffs;
    uint64_t null_cutoffs;
    uint64_t lmr_reductions;
    int max_depth_reached;
    int current_depth;
} stats;


// FONCTIONS UTILITAIRES


// Remet toutes les variables à zéro avant de commencer une nouvelle recherche
static void reset_search(void) {
    memset(killer_moves, 0, sizeof(killer_moves));
    memset(history_scores, 0, sizeof(history_scores));
    memset(&stats, 0, sizeof(stats));
    stop_search = false;
}

// Ajoute un killer move (= coup qui a causé une coupure beta)
static inline void add_killer(AIMove move, int ply) {
    if (ply >= MAX_DEPTH) return;
    // On décale l'ancien killer move pour garder le nouveau
    if (killer_moves[ply][0].hole != move.hole || killer_moves[ply][0].color != move.color) {
        killer_moves[ply][1] = killer_moves[ply][0];
        killer_moves[ply][0] = move;
    }
}

// Met à jour le score historique d'un coup (pour le tri)
static inline void add_history(AIMove move, int depth) {
    int idx = move.hole - 1;
    if (idx >= 0 && idx < 16) {
        // Plus on est profond, plus le coup a de valeur
        history_scores[idx][move.color] += depth * depth;
        // On divise par 2 si les valeurs deviennent trop grandes
        if (history_scores[idx][move.color] > 100000) {
            for (int h = 0; h < 16; h++)
                for (int c = 0; c < 4; c++)
                    history_scores[h][c] /= 2;
        }
    }
}

// Vérifie si un coup est dans la liste des killer moves
static inline bool is_killer(AIMove move, int ply) {
    if (ply >= MAX_DEPTH) return false;
    return (killer_moves[ply][0].hole == move.hole && killer_moves[ply][0].color == move.color) ||
           (killer_moves[ply][1].hole == move.hole && killer_moves[ply][1].color == move.color);
}

// Vérifie si on a dépassé le temps imparti 
// On le fait tous les 4096 noeuds pour ne pas ralentir l'algo avec des appels système
static inline bool check_timeout(void) {
    if ((stats.nodes_searched & 4095) == 0) {
        if (clock() > search_end_time) {
            stop_search = true;
        }
    }
    return stop_search;
}


// ANALYSE DU PLATEAU (aide à l'évaluation)


// Compte simplement les graines chez nous et chez l'adversaire
static void count_seeds(const Board* board, int player, int* my_seeds, int* opp_seeds) {
    *my_seeds = 0;
    *opp_seeds = 0;
    
    for (int i = 0; i < 16; i++) {
        int total = board->holes[i].red + board->holes[i].blue + board->holes[i].transparent;
        bool is_p1_hole = (i % 2 == 0);
        
        if ((player == 1 && is_p1_hole) || (player == 2 && !is_p1_hole)) {
            *my_seeds += total;
        } else {
            *opp_seeds += total;
        }
    }
}

// Compte les trous vulnérables (1 ou 2 graines) qu'on peut se faire manger :(
static int count_vulnerable_holes(const Board* board, int player) {
    int count = 0;
    int start = (player == 1) ? 0 : 1;
    
    for (int i = start; i < 16; i += 2) {
        int total = board->holes[i].red + board->holes[i].blue + board->holes[i].transparent;
        if (total == 1 || total == 2) {
            count++;
        }
    }
    return count;
}

// Compte les trous avec beaucoup de graines (=> pour la stratégie long terme)
static int count_loaded_holes(const Board* board, int player) {
    int count = 0;
    int start = (player == 1) ? 0 : 1;
    
    for (int i = start; i < 16; i += 2) {
        int total = board->holes[i].red + board->holes[i].blue + board->holes[i].transparent;
        if (total >= 6) {
            count++;
        }
    }
    return count;
}

// On essaie de voir si on peut capturer des graines au prochain coup
static int evaluate_capture_potential(const Board* board, int player) {
    int potential = 0;
    AIMove moves[64];
    int n = generate_legal_moves(board, player, moves);
    
    // On regarde juste les 10 premiers coups pour pas perdre trop de temps
    for (int i = 0; i < n && i < 10; i++) { 
        int gain = quick_predict_score(board, moves[i].hole - 1, moves[i].color, player);
        if (gain > potential) {
            potential = gain;
        }
    }
    return potential;
}


// FONCTION D'ÉVALUATION
// basée sur plusieurs critères stratégiques : 

// on donne une note au plateau
static int evaluate(const Board* board, int player, const int scores[3], int ply) {
    int opponent = (player == 1) ? 2 : 1;
    
    // Gestion des fins de partie (victoire/défaite immédiate)
    if (scores[player] >= 49) return WIN_SCORE - ply; // On préfère gagner vite
    if (scores[opponent] >= 49) return -WIN_SCORE + ply;
    if (scores[player] == 48 && scores[opponent] == 48) return 0; // Match nul
    
    int my_seeds = 0, opp_seeds = 0;
    count_seeds(board, player, &my_seeds, &opp_seeds);
    int total_seeds = my_seeds + opp_seeds;
    int total_captured = scores[player] + scores[opponent];
    
    // S'il reste peu de graines, c'est celui qui a le plus de points qui gagne
    if (total_seeds < 10) {
        if (scores[player] > scores[opponent]) return WIN_SCORE - ply;
        if (scores[opponent] > scores[player]) return -WIN_SCORE + ply;
        return 0;
    }
    
    int eval = 0;
    bool early_game = (total_captured < 20);  
    
    // Bonus pour encourager l'IA à jouer
    eval += early_game ? 30 : 15;
    
    // 1. La différence de score (=> critère principal)
    int score_diff = scores[player] - scores[opponent];
    if (early_game) eval += score_diff * 100;
    else if (total_captured < 60) eval += score_diff * 150;
    else eval += score_diff * 200; // En fin de partie, chaque point compte double 
    
    // 2. On essaie de garder des graines chez nous (= défense)
    eval += (my_seeds - opp_seeds) * 10;
    
    // 3. Mobilité (avoir plus de choix de coups que l'adversaire)
    AIMove my_moves[64], opp_moves[64];
    int my_mobility = generate_legal_moves(board, player, my_moves);
    int opp_mobility = generate_legal_moves(board, opponent, opp_moves);
    eval += (my_mobility - opp_mobility) * (early_game ? 12 : 8);
    
    // 4. Pénalité pour les trous vulnérables
    int my_vulnerable = count_vulnerable_holes(board, player);
    int opp_vulnerable = count_vulnerable_holes(board, opponent);
    eval += (opp_vulnerable - my_vulnerable) * 15;
    
    // 5. Bonus pour les gros trous (stratégie long terme)
    int my_loaded = count_loaded_holes(board, player);
    int opp_loaded = count_loaded_holes(board, opponent);
    eval += (my_loaded - opp_loaded) * 10;
    
    // 6. Potentiel de capture immédiat
    int my_capture_potential = evaluate_capture_potential(board, player);
    int opp_capture_potential = evaluate_capture_potential(board, opponent);
    eval += (my_capture_potential - opp_capture_potential) * (early_game ? 25 : 15);
    
    // 7. En début de partie, on essaie d'avoir des trous actifs (2 à 10 graines)
    if (early_game) {
        int my_active_holes = 0;
        int start = (player == 1) ? 0 : 1;
        for (int i = start; i < 16; i += 2) {
            int t = board->holes[i].red + board->holes[i].blue + board->holes[i].transparent;
            if (t >= 2 && t <= 10) my_active_holes++;
        }
        eval += my_active_holes * 5;
    }
    
    // 8. Bonus/Malus pour les seuils de score importants
    if (scores[player] >= 40) eval += (scores[player] - 39) * 100;
    if (scores[opponent] >= 40) eval -= (scores[opponent] - 39) * 100;
    
    // On évite d'avoir trop peu de graines quand l'adversaire en a beaucoup (famine ?)
    if (my_seeds < 5 && opp_seeds > 10) eval -= 400;
    if (opp_seeds < 5 && my_seeds > 10) eval += 300;
    
    if (early_game && my_seeds > opp_seeds + 3) eval += 50;
    
    return eval;
}


// TRI DES COUPS 


// On donne une note à chaque coup pour les trier
// Le but = examiner les meilleurs coups en premier pour l'élagage alpha-beta
static int score_move(AIMove move, AIMove tt_move, int ply, const Board* board, int player) {

    // Si c'est le coup qui vient de la Table de Transposition, c'est le meilleur
    if (tt_move.hole != 0 && move.hole == tt_move.hole && move.color == tt_move.color) {
        return 10000000;
    }
    
    // Si le coup permet de capturer, on lui donne un gros score
    int capture = quick_predict_score(board, move.hole - 1, move.color, player);
    if (capture > 0) {
        return 5000000 + capture * 10000;
    }
    
    // Si c'est un "killer move"
    if (is_killer(move, ply)) {
        return 4000000;
    }
    
    // Sinon on utilise l'historique
    int idx = move.hole - 1;
    if (idx >= 0 && idx < 16) {
        return history_scores[idx][move.color];
    }
    
    return 0;
}

// Fonction de tri (insertion sort, suffisant ici)
static void sort_moves(AIMove* moves, int n, AIMove tt_move, int ply, const Board* board, int player) {
    int scores[64];
    for (int i = 0; i < n; i++) {
        scores[i] = score_move(moves[i], tt_move, ply, board, player);
    }
    
    for (int i = 1; i < n; i++) {
        int key_score = scores[i];
        AIMove key_move = moves[i];
        int j = i - 1;
        while (j >= 0 && scores[j] < key_score) {
            scores[j + 1] = scores[j];
            moves[j + 1] = moves[j];
            j--;
        }
        scores[j + 1] = key_score;
        moves[j + 1] = key_move;
    }
}

// ALGORITHME NEGAMAX


static int negamax(GameState* game, int depth, int alpha, int beta, int ply, 
                   AIMove* best_move, bool allow_null) {
    
    if (check_timeout()) return 0;
    
    stats.nodes_searched++;
    if (ply > stats.max_depth_reached) stats.max_depth_reached = ply;
    
    // 1. On regarde dans la Table de Transposition (TT)
    uint64_t hash = zobrist_hash(&game->board, game->current, game->score);
    TTEntry tt_entry;
    AIMove tt_move = {0, RED};
    
    if (tt_probe(hash, depth, alpha, beta, &tt_entry)) {
        stats.tt_hits++;
        if (best_move && tt_entry.best_move.hole != 0) *best_move = tt_entry.best_move;
        stats.tt_cutoffs++;
        return tt_entry.score;
    }
    if (tt_entry.best_move.hole != 0) tt_move = tt_entry.best_move;
    
    // Conditions d'arrêt (fin de partie ou profondeur max atteinte)
    if (game_over(game) || depth <= 0) {
        int eval = evaluate(&game->board, game->current, game->score, ply);
        tt_store(hash, depth, eval, EXACT, (AIMove){0, RED});
        return eval;
    }
    
    // Null Move Pruning 
    // On essaie de "ne rien faire". Si on est toujours gagnant, la position est très forte.
    if (allow_null && depth >= 3 && ply > 0) {
        int my_seeds = 0, opp_seeds = 0;
        count_seeds(&game->board, game->current, &my_seeds, &opp_seeds);
        
        // On ne le fait pas si on a peu de graines = risqué
        if (my_seeds > 5) {
            GameState null_game = *game;
            null_game.current = (game->current == 1) ? 2 : 1;
            
            // On réduit la profondeur pour cette recherche
            int R = 2 + depth / 4;
            AIMove dummy;
            int null_score = -negamax(&null_game, depth - 1 - R, -beta, -beta + 1, 
                                      ply + 1, &dummy, false);
            
            if (stop_search) return 0;
            if (null_score >= beta) {
                stats.null_cutoffs++;
                return beta; // Coupure Beta
            }
        }
    }
    
    // Génération des coups
    AIMove moves[64];
    int n = generate_legal_moves(&game->board, game->current, moves);
    
    // Pas de coups possibles => on évalue
    if (n == 0) return evaluate(&game->board, game->current, game->score, ply);
    
    // Tri des coups pour optimiser l'élagage
    sort_moves(moves, n, tt_move, ply, &game->board, game->current);
    
    int original_alpha = alpha;
    AIMove local_best = moves[0];
    int best_score = -INFINITY_SCORE;
    
    // Boucle sur les coups
    for (int i = 0; i < n; i++) {
        GameState child = *game;
        if (!apply_move(&child, moves[i])) continue;
        
        int score;
        AIMove dummy;
        

        // Les coups tardifs dans la liste sont probablement mauvais, on les cherche moins profond
        bool do_full_search = true;
        
        if (i >= 3 && depth >= 3 && !is_killer(moves[i], ply)) {
            int capture = quick_predict_score(&game->board, moves[i].hole - 1, 
                                              moves[i].color, game->current);
            // On ne réduit pas si c'est une capture
            if (capture == 0) {
                int R = 1 + i / 6;
                stats.lmr_reductions++;
                score = -negamax(&child, depth - 1 - R, -alpha - 1, -alpha, 
                                 ply + 1, &dummy, true);
                if (stop_search) return 0;
                // Si le score est intéressant, on devra refaire une recherche complète
                do_full_search = (score > alpha);
            }
        }
        
        if (do_full_search) {
            // Principal Variation Search 
            if (i == 0) {
                score = -negamax(&child, depth - 1, -beta, -alpha, ply + 1, &dummy, true);
            } else {
                // Recherche avec fenêtre nulle
                score = -negamax(&child, depth - 1, -alpha - 1, -alpha, ply + 1, &dummy, true);
                if (stop_search) return 0;
                if (score > alpha && score < beta) {
                    // Si ça échoue, recherche complète
                    score = -negamax(&child, depth - 1, -beta, -alpha, ply + 1, &dummy, true);
                }
            }
        }
        
        if (stop_search) return 0;
        
        if (score > best_score) {
            best_score = score;
            local_best = moves[i];
        }
        if (score > alpha) {
            alpha = score;
            // Mise à jour de l'historique et des killers
            add_history(moves[i], depth);
            add_killer(moves[i], ply);
        }
        if (alpha >= beta) {
            // Coupure Beta
            tt_store(hash, depth, best_score, LOWER_BOUND, local_best);
            if (best_move) *best_move = local_best;
            return best_score;
        }
    }
    
    // Sauvegarde dans la TT
    TTEntryType type = (best_score <= original_alpha) ? UPPER_BOUND : EXACT;
    tt_store(hash, depth, best_score, type, local_best);
    
    if (best_move) *best_move = local_best;
    return best_score;
}

// ITERATIVE DEEPENING (Recherche itérative)


static int iterative_deepening(GameState* game, int time_ms, AIMove* best_move) {
    clock_t start = clock();
    // On garde une marge de sécurité de 150ms pour pas perdre au temps
    search_end_time = start + ((clock_t)(time_ms - 150) * CLOCKS_PER_SEC) / 1000;
    
    reset_search();
    
    AIMove moves[64];
    int n = generate_legal_moves(&game->board, game->current, moves);
    if (n == 0) {
        best_move->hole = 0;
        return -INFINITY_SCORE;
    }
    
    AIMove current_best = moves[0];
    int current_score = 0;
    
    // On augmente la profondeur petit à petit
    for (int depth = 1; depth <= MAX_DEPTH; depth++) {
        if (clock() >= search_end_time || stop_search) break;
        
        stats.current_depth = depth;
        

        // On réduit la fenêtre de recherche autour du score précédent pour aller plus vite
        int alpha, beta;
        if (depth >= 4) {
            alpha = current_score - 100;
            beta = current_score + 100;
        } else {
            alpha = -INFINITY_SCORE;
            beta = INFINITY_SCORE;
        }
        
        AIMove iter_best = current_best;
        int score = negamax(game, depth, alpha, beta, 0, &iter_best, true);
        
        if (stop_search) break;
        
        // Si le score sort de la fenêtre, on recommence avec l'infini
        if (score <= alpha || score >= beta) {
            score = negamax(game, depth, -INFINITY_SCORE, INFINITY_SCORE, 0, &iter_best, true);
            if (stop_search) break;
        }
        
        current_score = score;
        current_best = iter_best;
        
        // Si on a trouvé une victoire quasi certaine, on arrête
        if (score > WIN_SCORE - 100) break;
    }
    
    *best_move = current_best;
    return current_score;
}


// MAIN / INTERFACE


static int advanced_get_best_move(GameState* game, int time_ms, AIMove* best_move) {
    AIMove moves[64];
    int n = generate_legal_moves(&game->board, game->current, moves);
    
    // Si un seul coup possible, on ne réfléchit pas
    if (n == 1) {
        *best_move = moves[0];
        return 0; 
    }
    if (n == 0) {
        best_move->hole = 0;
        return -INFINITY_SCORE;
    }

    // Sinon on lance la recherche
    int score = iterative_deepening(game, time_ms, best_move);
    return score;
}

static void advanced_init(void) {
    // Rien de spécial à initialiser ici
}

static void advanced_cleanup(void) {}

// Définition de la structure de l'IA pour l'interface
AIPlayer AI_Advanced = {
    .name = "Advanced",
    .description = "Negamax + NMP + LMR + Aspiration + TT",
    .get_best_move = advanced_get_best_move,
    .init = advanced_init,
    .cleanup = advanced_cleanup,

};