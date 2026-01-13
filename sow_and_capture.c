#include <stdio.h>
#include <stdlib.h>
#include "sow_and_capture.h"

// Structure pour représenter une graine à semer
typedef struct {
    int type; // 0=R, 1=B, 2=T
    int count;
} SeedToSow;

// --- Fonctions utilitaires ---

// Vérifie si un trou appartient au joueur courant
static inline int is_player_hole(int hole, int player) {
    // Joueur 1 : indices pairs (0,2,4...) -> trous 1,3,5...
    // Joueur 2 : indices impairs (1,3,5...) -> trous 2,4,6...
    return (hole & 1) == (player - 1);
}

// Compte total des graines dans un trou
static inline int get_total_seeds(const Board *b, int hole) {
    return b->holes[hole].red + b->holes[hole].blue + b->holes[hole].transparent;
}

// Vide le trou de départ selon les règles de couleur
static inline void empty_source_hole(Board *b, int hole, Color c) {
    switch(c) {
        case RED: b->holes[hole].red = 0; break;
        case BLUE: b->holes[hole].blue = 0; break;
        case T_AS_RED: 
            // Si on joue TR, on prend les transparentes ET les rouges
            b->holes[hole].transparent = 0;
            b->holes[hole].red = 0; 
            break;
        case T_AS_BLUE: 
            // Si on joue TB, on prend les transparentes ET les bleues
            b->holes[hole].transparent = 0;
            b->holes[hole].blue = 0; 
            break;
    }
}

// Ajoute une graine dans un trou
static inline void sow_seed(Board *b, int hole, int seed_type) {
    switch(seed_type) {
        case SEED_TYPE_RED: b->holes[hole].red++; break;
        case SEED_TYPE_BLUE: b->holes[hole].blue++; break;
        case SEED_TYPE_TRANSPARENT: b->holes[hole].transparent++; break;
    }
}

// Vide un trou et renvoie le nombre de graines capturées
static int capture_hole(Board *b, int hole) {
    int captured = b->holes[hole].red + b->holes[hole].blue + b->holes[hole].transparent;
    b->holes[hole].red = 0;
    b->holes[hole].blue = 0;
    b->holes[hole].transparent = 0;
    return captured;
}

// FONCTION PRINCIPALE : Joue un coup complet (Semis + Capture) 
int sow_and_capture(Board *b, int start, Color c, int *score_gain, int player) {
    *score_gain = 0;
    
    // 1. On compte combien de graines on a en main
    int red = 0, blue = 0, trans = 0;
    
    switch(c) {
        case RED: red = b->holes[start].red; break;
        case BLUE: blue = b->holes[start].blue; break;
        case T_AS_RED: 
            trans = b->holes[start].transparent;
            red = b->holes[start].red; 
            break;
        case T_AS_BLUE: 
            trans = b->holes[start].transparent;
            blue = b->holes[start].blue; 
            break;
    }
    
    int total_seeds = red + blue + trans;
    if(total_seeds == 0) return 0; // Rien à jouer
    
    // 2. On vide le trou de départ
    empty_source_hole(b, start, c);
    
    // 3. Le Semis
    // On sème dans l'ordre : Transparentes -> Rouges -> Bleues
    
    int current_hole = start;
    
    // A. Semis des Transparentes
    for(int i = 0; i < trans; i++) {
        do {
            current_hole = (current_hole + 1) % N_HOLES;
            // On saute le trou de départ
            // Si on joue en BLEU (TB), on saute aussi nos propres trous
        } while (current_hole == start || (c == T_AS_BLUE && is_player_hole(current_hole, player))); 
        sow_seed(b, current_hole, SEED_TYPE_TRANSPARENT);
    }
    
    // B. Semis des Rouges
    for(int i = 0; i < red; i++) {
        do {
            current_hole = (current_hole + 1) % N_HOLES;
        } while (current_hole == start); // Les rouges vont partout sauf départ
        sow_seed(b, current_hole, SEED_TYPE_RED);
    }
    
    // C. Semis des Bleues
    if (blue > 0) {
        for(int i = 0; i < blue; i++) {
            do {
                current_hole = (current_hole + 1) % N_HOLES;
                // Les bleues sautent le départ ET nos propres trous
            } while (current_hole == start || is_player_hole(current_hole, player));
            sow_seed(b, current_hole, SEED_TYPE_BLUE);
        }
    }
    
    // 4. La Capture 
    // On part du dernier trou semé et on recule
    int capture_pos = current_hole;
    
    // Sécurité pour éviter les boucles infinies
    int checked_holes = 0;
    
    while(checked_holes < N_HOLES) {
        int total = get_total_seeds(b, capture_pos);
        
        // Règle : capture si 2 ou 3 graines
        if(total == 2 || total == 3) {
            *score_gain += capture_hole(b, capture_pos);
            // On recule d'une case (modulo 16)
            capture_pos = (capture_pos - 1 + N_HOLES) % N_HOLES;
        } else {
            break; // La chaîne de capture s'arrête
        }
        checked_holes++;
    }
    
    return 1;
}

// PRÉDICTION RAPIDE POUR L'IA
// Simule le déplacement sans modifier le plateau pour savoir où on atterrit
int quick_predict_score(const Board *b, int start, Color c, int player) {
    int red = 0, blue = 0, trans = 0;
    
    switch(c) {
        case RED: red = b->holes[start].red; break;
        case BLUE: blue = b->holes[start].blue; break;
        case T_AS_RED: 
            trans = b->holes[start].transparent;
            red = b->holes[start].red; 
            break;
        case T_AS_BLUE: 
            trans = b->holes[start].transparent;
            blue = b->holes[start].blue; 
            break;
    }
    
    int total_seeds = red + blue + trans;
    if(total_seeds == 0) return 0;

    // Simulation du déplacement pour trouver le trou d'arrivée
    int current_hole = start;
    
    // 1. Transparentes
    for(int i = 0; i < trans; i++) {
        do {
            current_hole = (current_hole + 1) % N_HOLES;
        } while (current_hole == start || (c == T_AS_BLUE && is_player_hole(current_hole, player)));
    }
    
    // 2. Rouges
    for(int i = 0; i < red; i++) {
        do {
            current_hole = (current_hole + 1) % N_HOLES;
        } while (current_hole == start);
    }
    
    // 3. Bleues
    if (blue > 0) {
        for(int i = 0; i < blue; i++) {
            do {
                current_hole = (current_hole + 1) % N_HOLES;
            } while (current_hole == start || is_player_hole(current_hole, player));
        }
    }
    
    // On regarde juste si le dernier trou permet une capture
    // = juste une estimation, on ne simule pas la chaîne complète pour gagner du temps
    int existing_seeds = get_total_seeds(b, current_hole);
    int future_seeds = existing_seeds + 1; // +1 car on vient d'en poser une
    
    if (future_seeds == 2 || future_seeds == 3) {
        return future_seeds; 
    }
    
    return 0;
}