#include "ai.h"
#include "sow_and_capture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


// GESTION DE LA MÉMOIRE (Table de Transpo)
// On veut stocker les positions qu'on a déjà calculées pour ne pas
// perdre de temps à refaire le même travail si on retombe dessus.


// On prévoit un max de graines pour le hachage pour éviter les débordements
#define MAX_SEEDS_PER_HOLE 50

// Tableaux de nombres aléatoires pour le Zobrist Hashing
// Chaque pièce, chaque trou, chaque score a un nombre unique associé
static uint64_t zobrist_keys[N_HOLES][3][MAX_SEEDS_PER_HOLE];
static uint64_t zobrist_player[2];        
static uint64_t zobrist_scores[3][100];   
static uint64_t zobrist_half_moves[101];  

// La table principale
static TranspositionTable tt;

// Graine pour notre générateur de nombres aléatoires rapide 
static uint64_t xorshift64_state = 0x123456789ABCDEF0ULL;

// Petit générateur aléatoire très rapide (plus rapide que rand())
// https://en.wikipedia.org/wiki/Xorshift

static uint64_t xorshift64(void) {
    uint64_t x = xorshift64_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    xorshift64_state = x;
    return x;
}

// Initialise tout le système de mémoire au début du programme
void tt_init(void) {
    // On mélange la graine avec l'heure pour que ce soit différent à chaque fois
    xorshift64_state = (uint64_t)time(NULL) ^ 0x123456789ABCDEF0ULL;
    
    // On remplit les tableaux avec des nombres aléatoires
    for (int hole = 0; hole < N_HOLES; hole++) {
        for (int seed_type = 0; seed_type < 3; seed_type++) {
            for (int count = 0; count < MAX_SEEDS_PER_HOLE; count++) {
                zobrist_keys[hole][seed_type][count] = xorshift64();
            }
        }
    }
    
    zobrist_player[0] = xorshift64();
    zobrist_player[1] = xorshift64();
    
    for (int player = 0; player < 3; player++) {
        for (int score = 0; score < 100; score++) {
            zobrist_scores[player][score] = xorshift64();
        }
    }
    
    for (int i = 0; i <= 100; i++) {
        zobrist_half_moves[i] = xorshift64();
    }
    
    // On alloue la mémoire pour la table de transposition
    tt.entries = (TTEntry*)calloc(TT_SIZE, sizeof(TTEntry));
    if (!tt.entries) exit(1); // Erreur critique si plus de RAM
    
    tt.hits = 0; tt.misses = 0; tt.collisions = 0;
}

// Libère la mémoire à la fin
void tt_cleanup(void) {
    if (tt.entries) {
        free(tt.entries);
        tt.entries = NULL;
    }
}

// Vide la table (=> si on veut reset entre deux parties par ex)
void tt_clear(void) {
    if (tt.entries) {
        memset(tt.entries, 0, TT_SIZE * sizeof(TTEntry));
        tt.hits = 0; tt.misses = 0; tt.collisions = 0;
    }
}

// Calcule l'identifiant unique (Hash) d'un plateau de jeu
// Si deux plateaux sont identiques, ils auront le même Hash
uint64_t zobrist_hash(const Board* board, int player, const int scores[3]) {
    uint64_t hash = 0;
    
    // On combine les valeurs de chaque trou
    for (int i = 0; i < N_HOLES; i++) {
        const Hole* h = &board->holes[i];
        
        if (h->red > 0 && h->red < MAX_SEEDS_PER_HOLE) 
            hash ^= zobrist_keys[i][0][h->red];
        if (h->blue > 0 && h->blue < MAX_SEEDS_PER_HOLE) 
            hash ^= zobrist_keys[i][1][h->blue];
        if (h->transparent > 0 && h->transparent < MAX_SEEDS_PER_HOLE) 
            hash ^= zobrist_keys[i][2][h->transparent];
    }
    
    // On ajoute l'info du joueur courant
    hash ^= zobrist_player[player - 1];
    
    // On ajoute l'info du score
    for (int p = 1; p <= 2; p++) {
        if (scores[p] > 0 && scores[p] < 100) 
            hash ^= zobrist_scores[p][scores[p]];
    }
    
    return hash;
}

// Cherche si une position existe déjà dans la table
// Renvoie true si on a trouvé quelque chose d'utile
bool tt_probe(uint64_t zobrist_key, int depth, int alpha, int beta, TTEntry* result) {
    uint64_t index = zobrist_key % TT_SIZE;
    TTEntry* entry = &tt.entries[index];
    
    // Vérifie si l'entrée est valide et correspond bien à notre position (pas de collision)
    if (!entry->valid || entry->zobrist_key != zobrist_key) {
        tt.misses++;
        return false;
    }
    
    // Si la position stockée a été calculée moins profondément que ce qu'on veut, ça ne suffit pas
    if (entry->depth < depth) {
        tt.misses++;
        return false;
    }
    
    tt.hits++;
    *result = *entry;
    
    // On vérifie si le score stocké est utilisable par rapport à alpha/beta
    switch (entry->type) {
        case EXACT: return true;
        case LOWER_BOUND: return (entry->score >= beta);
        case UPPER_BOUND: return (entry->score <= alpha);
    }
    return false;
}

// Sauvegarde une position et son score dans la table
void tt_store(uint64_t zobrist_key, int depth, int score, TTEntryType type, AIMove best_move) {
    uint64_t index = zobrist_key % TT_SIZE;
    TTEntry* entry = &tt.entries[index];
    
    // Stratégie de remplacement : on garde toujours la position qui a été cherchée le plus profondément
    if (entry->valid && entry->zobrist_key != zobrist_key) {
        if (entry->depth > depth) {
            tt.collisions++;
            return; // On ne remplace pas une info plus précieuse (= plus profonde)
        }
    }
    
    entry->zobrist_key = zobrist_key;
    entry->depth = depth;
    entry->score = score;
    entry->type = type;
    entry->best_move = best_move;
    entry->valid = true;
}


// LOGIQUE DE JEU GÉNÉRALE
// Fonctions de base utilisées par l'IA pour simuler les coups


// Trouve tous les coups que le joueur a le droit de jouer
int generate_legal_moves(const Board* board, int player, AIMove* moves) {
    int count = 0;
    
    for (int i = 0; i < 16; i++) {
        int hole = i + 1; // 0-15 -> 1-16
        const Hole* h = &board->holes[i];
        
        // On teste chaque couleur
        if (h->red > 0 && valid_move(board, i, RED, player)) {
            moves[count].hole = hole;
            moves[count].color = RED;
            count++;
        }
        if (h->blue > 0 && valid_move(board, i, BLUE, player)) {
            moves[count].hole = hole;
            moves[count].color = BLUE;
            count++;
        }
        if (h->transparent > 0) {
            // Les transparentes peuvent être jouées comme Rouge ou Bleu
            if (valid_move(board, i, T_AS_RED, player)) {
                moves[count].hole = hole;
                moves[count].color = T_AS_RED;
                count++;
            }
            if (valid_move(board, i, T_AS_BLUE, player)) {
                moves[count].hole = hole;
                moves[count].color = T_AS_BLUE;
                count++;
            }
        }
    }
    return count;
}

// Applique un coup sur une copie du jeu (pour simuler le futur)
bool apply_move(GameState* game, AIMove move) {
    Board temp_board = game->board;
    int score_gain = 0;
    int hole_index = move.hole - 1; // On repasse en index tableau 0-15
    
    // On utilise la logique du jeu (semis + captures)
    if (!sow_and_capture(&temp_board, hole_index, move.color, &score_gain, game->current)) {
        return false;
    }
    
    // Si ça marche, on met à jour l'état
    game->board = temp_board;
    game->score[game->current] += score_gain;
    game->current = (game->current == 1) ? 2 : 1; // Changement de joueur
    
    return true;
}

// Petit util pour l'affichage (Debug)
const char* color_to_string(Color c) {
    switch(c) {
        case RED: return "R";
        case BLUE: return "B";
        case T_AS_RED: return "TR";
        case T_AS_BLUE: return "TB";
        default: return "?";
    }
}