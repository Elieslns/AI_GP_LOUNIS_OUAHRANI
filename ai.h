#ifndef AI_H
#define AI_H

#include <stdbool.h>
#include <stdint.h>
#include "board.h"
#include "move.h"
#include "game.h"

// Structure simple pour stocker un coup
typedef struct {
    int hole;
    Color color;
} AIMove;

// Un coup avec sa note (pour trier les coups)
typedef struct {
    AIMove move;
    int score;
} ScoredMove;

// Paramètres de configuration de l'IA (si besoin d'évol future)
typedef struct {
    int search_depth;      
    int time_limit_ms;     
    bool use_adaptive;     
    bool use_transposition_table;  
} AIConfig;

// TABLE DE TRANSPOSITION 


// Indique le type de score stocké
typedef enum {
    EXACT,       // Score exact
    LOWER_BOUND, // Score minimum garanti (Alpha)
    UPPER_BOUND  // Score maximum garanti (Beta)
} TTEntryType;

// Ce qu'on stocke dans la mémoire
typedef struct {
    uint64_t zobrist_key;   // Signature unique du plateau
    int depth;              // Profondeur du calcul
    int score;              // Score trouvé
    TTEntryType type;       // Type de résultat
    AIMove best_move;       // Meilleur coup à jouer
    bool valid;             // Case occupée ?
} TTEntry;

// Taille de la table : 1 million d'entrées
#define TT_SIZE (1 << 20)

typedef struct {
    TTEntry* entries;
    uint64_t hits;        // Stats : trouvés
    uint64_t misses;      // Stats : ratés
    uint64_t collisions;  // Stats : conflits
} TranspositionTable;

// --- Fonctions de gestion de la mémoire ---

void tt_init(void);      // Allouer
void tt_cleanup(void);   // Libérer
void tt_clear(void);     // Vider

// Hashage Zobrist
uint64_t zobrist_hash(const Board* board, int player, const int scores[3]);

// Vérifier si une position existe
bool tt_probe(uint64_t zobrist_key, int depth, int alpha, int beta, TTEntry* result);

// Sauvegarder une position
void tt_store(uint64_t zobrist_key, int depth, int score, TTEntryType type, AIMove best_move);

// --- Fonctions utilitaires ---

// Générer les coups possibles
int generate_legal_moves(const Board* board, int player, AIMove* moves);

// Simuler un coup (renvoie false si impossible)
bool apply_move(GameState* game, AIMove move);

// Debug affichage couleur
const char* color_to_string(Color c);

#endif // AI_H