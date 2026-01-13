#ifndef SOW_AND_CAPTURE_H
#define SOW_AND_CAPTURE_H

#include "board.h"
#include "move.h"

// Constants for optimization
#define SEED_TYPE_RED 0
#define SEED_TYPE_BLUE 1
#define SEED_TYPE_TRANSPARENT 2

/**
 * Sème les graines depuis le trou donné et gère les captures.
 * 
 * @param b Le plateau de jeu (sera modifié)
 * @param start L'index du trou de départ (0-15)
 * @param c La couleur/type de graines à jouer
 * @param score_gain Pointeur pour stocker le score gagné
 * @param player Le joueur courant (1 ou 2) pour vérifier les captures
 * @return 1 si le coup a pu être joué, 0 sinon (ex: trou vide)
 */
int sow_and_capture(Board *b, int start, Color c, int *score_gain, int player);

/**
 * Estime le gain de score d'un coup SANS modifier le plateau.
 * Beaucoup plus rapide que sow_and_capture + copie de plateau.
 * 
 * @param b Le plateau de jeu (lecture seule)
 * @param start L'index du trou de départ
 * @param c La couleur jouée
 * @param player Le joueur courant
 * @return Le score estimé (peut être approximatif, mais rapide)
 */
int quick_predict_score(const Board *b, int start, Color c, int player);

#endif // SOW_AND_CAPTURE_H
