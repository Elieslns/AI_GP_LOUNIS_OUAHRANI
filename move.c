#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "move.h"

int parse_move(const char *s, int *hole, Color *c){
    if(s == NULL || strlen(s) < 2) return 0;
    
    // Parser le numéro du trou
    int n = atoi(s);
    if(n < 1 || n > 16) return 0;
    *hole = n - 1;
    
    // Trouver la position de la couleur
    int color_start = 0;
    while(isdigit(s[color_start])) {
        color_start++;
    }
    
    if(color_start == 0) return 0; // Pas de numéro
    
    // Parser la couleur
    char col1 = toupper(s[color_start]);
    char col2 = (color_start + 1 < (int)strlen(s)) ? toupper(s[color_start + 1]) : '\0';
    
    if(col1 == 'R' && col2 == '\0') {
        *c = RED;
    } else if(col1 == 'B' && col2 == '\0') {
        *c = BLUE;
    } else if(col1 == 'T' && col2 == 'R') {
        *c = T_AS_RED;
    } else if(col1 == 'T' && col2 == 'B') {
        *c = T_AS_BLUE;
    } else {
        return 0; // Couleur invalide
    }
    
    return 1;
}

int valid_move(const Board *b, int h, Color c, int p){
    // Vérifie que le trou appartient au joueur
    // Joueur 1 contrôle les trous impairs (1,3,5,7,9,11,13,15) -> indices pairs (0,2,4,6,8,10,12,14)
    // Joueur 2 contrôle les trous pairs (2,4,6,8,10,12,14,16) -> indices impairs (1,3,5,7,9,11,13,15)

    if(p == 1 && h % 2 != 0) return 0;  // J1 doit jouer sur indices pairs 
    if(p == 2 && h % 2 != 1) return 0;  // J2 doit jouer sur indices impairs 
    
    // Vérifie qu'il y a des graines disponibles
    int count = 0;
    switch(c) {
        case RED:
            count = b->holes[h].red;
            break;
        case BLUE:
            count = b->holes[h].blue;
            break;
        case T_AS_RED:
            count = b->holes[h].transparent + b->holes[h].red;
            break;
        case T_AS_BLUE:
            count = b->holes[h].transparent + b->holes[h].blue;
            break;
    }
    
    return count > 0;
}
