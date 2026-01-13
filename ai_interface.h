#ifndef AI_INTERFACE_H
#define AI_INTERFACE_H

#include "game.h"
#include "ai.h"


 //Interface commune pour toutes les IA
 //Permet de comparer facilement différentes stratégies par ex

typedef struct AIPlayer {
    const char* name;           // Nom de l'IA 
    const char* description;    
    
    // Obtient le meilleur coup pour cette IA
    
    int (*get_best_move)(GameState* game, int time_limit_ms, AIMove* best_move);
    
    
    //Initialise les structures de l'IA (TT, tables, etc.)
    
    void (*init)(void);
    
    
    // Nettoie la mémoire de l'IA
     
    void (*cleanup)(void);
    

    //Affiche les statistiques de l'IA après un coup
    void (*print_stats)(void);
    
} AIPlayer;

extern AIPlayer AI_Advanced;   // IA Ultra Optimisée

#endif // AI_INTERFACE_H
