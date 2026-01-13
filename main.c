#include "ai.h"
#include "ai_interface.h"
#include "game.h"
#include "move.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Définition des constantes arbitre
#define TIMEOUT_MS 2000

static GameState game;
static int my_id = 0; // 1 ou 2

// Envoi du coup au format attendu par l'arbitre
static void send_move(AIMove move) {
  const char *c_str = (move.color == RED)        ? "R"
                      : (move.color == BLUE)     ? "B"
                      : (move.color == T_AS_RED) ? "TR"
                                                 : "TB";
  printf("%d%s\n", move.hole, c_str);
  fflush(stdout);
}

// Parsing (Input arbitre 1-16 -> Interne GameState)
static int apply_arbitre_move(const char *str) {
  int hole;
  char temp[32];

  // 1. On sépare les chiffres (le trou)
  int i = 0;
  while (isdigit(str[i]))
    i++;
  if (i == 0)
    return 0; // Pas de chiffre trouvé

  strncpy(temp, str, i);
  temp[i] = '\0';
  hole = atoi(temp); // On a le trou (ex: 4)

  // 2. On regarde la suite (la couleur)
  const char *suffix = str + i;

  // ICI : On vérifie juste si le texte est valide, pas besoin de stocker
  // "color"
  if (strcasecmp(suffix, "r") != 0 && strcasecmp(suffix, "b") != 0 &&
      strcasecmp(suffix, "tr") != 0 && strcasecmp(suffix, "tb") != 0) {
    return 0; // Suffixe inconnu
  }

  // 3. On applique le coup sur le jeu
  // IMPORTANT : On force le joueur courant à être L'ADVERSAIRE avant de jouer
  // son coup
  int opponent = (my_id == 1) ? 2 : 1;
  game.current = opponent;

  // On reconstruit la commande proprement pour être sûr
  char move_cmd[16];
  sprintf(move_cmd, "%d%s", hole, suffix);

  return game_move(&game, move_cmd);
}

int main(void) {
  // Initialisations
  tt_init();
  AI_Advanced.init();
  game_init(&game);

  char input[256];

  // Lecture de l'entrée standard (boucle bloquante)
  while (fgets(input, sizeof(input), stdin) != NULL) {
    // Nettoyage string
    input[strcspn(input, "\r\n")] = '\0';
    if (strlen(input) == 0)
      continue;

    // 1. GESTION FIN DE PARTIE
    if (strcmp(input, "END") == 0 || strncmp(input, "RESULT", 6) == 0) {
      break;
    }

    // 2. GESTION START (Je suis Joueur 1)
    if (strcmp(input, "START") == 0) {
      my_id = 1;
      game.current = 1; // C'est à moi

      // Calcul et envoi
      AIMove best;
      AI_Advanced.get_best_move(&game, TIMEOUT_MS, &best);

      // Appliquer mon coup sur MON plateau local pour rester synchro
      char my_move_str[16];
      sprintf(my_move_str, "%d%s", best.hole,
              (best.color == RED
                   ? "R"
                   : (best.color == BLUE
                          ? "B"
                          : (best.color == T_AS_RED ? "TR" : "TB"))));
      game_move(&game, my_move_str);

      send_move(best);
      continue;
    }

    // 3. GESTION COUP ADVERSE (Je suis Joueur 2 si pas encore défini)
    if (my_id == 0) {
      my_id = 2;
    }

    // Appliquer le coup de l'adversaire
    // Note: apply_arbitre_move gère game.current = opponent internement
    if (!apply_arbitre_move(input)) {
      // Si le coup adverse est invalide (bug adversaire),
      // on continue quand même pour essayer de jouer (l'arbitre tranchera)
      fprintf(stderr, "Err move adverse: %s\n", input);
    }

    // 4. A MOI DE JOUER
    // On force la variable current à MOI pour que l'IA génère les bons coups
    game.current = my_id;

    // Si la partie est finie logiquement, on ne joue pas (attente END)
    if (game_over(&game))
      continue;

    AIMove best;
    AI_Advanced.get_best_move(&game, TIMEOUT_MS, &best);

    // Appliquer mon propre coup localement
    char my_move_str[16];
    sprintf(
        my_move_str, "%d%s", best.hole,
        (best.color == RED
             ? "R"
             : (best.color == BLUE ? "B"
                                   : (best.color == T_AS_RED ? "TR" : "TB"))));
    game_move(&game, my_move_str);

    send_move(best);
  }

  AI_Advanced.cleanup();
  tt_cleanup();
  return 0;
}