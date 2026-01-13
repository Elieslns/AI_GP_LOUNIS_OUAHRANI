# Projet Awalé - Bot de LOUNIS Elies et Sofiane OUAHRANI KHALDI

Ce projet est une **Intelligence Artificielle de niveau compétition** pour une variante complexe de l'Awalé (16 trous, graines colorées), codée en C.

## Utilisation Rapide (Exécutable fourni)

L'exécutable est déjà compilé et prêt à l'emploi : **`bot_lounis_ouahrani.exe`** 

### Lancer un match (avec l'Arbitre Java)
Le bot communique via l'entrée/sortie standard et nécessite l'arbitre Java pour gérer la partie.

1. **Compiler l'arbitre** (si ce n'est pas déjà fait) :
   ```bash
   javac Arbitre.java
   ```

2. **Lancer le match** (Bot vs Bot) :
   ```bash
   # Syntaxe : java Arbitre <Joueur1> <Joueur2>
   java Arbitre <Joueur1> <Joueur2>
   ```

   > **Note :** Vous pouvez remplacer l'un des bots par un autre programme.

---

## Règles du Jeu (Variante Spécifique)

Cette variante diffère considérablement de l'Oware classique.

### Plateau
- **16 trous** au total (numérotés de 1 à 16).
- **Joueur 1 (Impair)** : Contrôle les trous 1, 3, 5, ..., 15.
- **Joueur 2 (Pair)** : Contrôle les trous 2, 4, 6, ..., 16.
- **Sens du jeu** : Horaire (16 -> 1 -> 2 ...).

### Graines & Couleurs
Chaque trou contient initialement **6 graines** : 2 Rouges (R), 2 Bleues (B), 2 Transparentes (T).

1. **Graines Rouges (R)** : Semées dans **tous** les trous.
2. **Graines Bleues (B)** : Semées **uniquement** dans les trous de l'adversaire.
3. **Graines Transparentes (T)** :
   - Le joueur choisit de les jouer **comme des Rouges** (TR) ou **comme des Bleues** (TB).
   - Elles sont toujours distribuées **en premier** (avant les autres graines de la même couleur).

### Capture
- Une capture a lieu si le dernier trou semé contient **2 ou 3 graines** (après semis).
- **Captures multiples** : Si le trou précédent remplit aussi la condition, il est capturé, et ainsi de suite (rafle).
- Il est possible de capturer dans son propre camp.
- **Famine** : Affamer l'adversaire (lui prendre toutes ses graines ou l'empêcher de jouer) est **AUTORISÉ**.

### Fin de Partie
- Un joueur atteint **49 graines** ou plus.
- Il reste **moins de 10 graines** sur le plateau (le joueur avec le plus de graines gagne).
- Limite de **400 coups** atteinte.

---

## Stratégie de l'IA

Notre bot utilise des algorithmes avancés pour optimiser ses décisions en moins de 2 secondes :

1. **Negamax & Alpha-Beta** : Recherche arborescente optimisée pour anticiper les coups adverses sans tout calculer.
2. **Iterative Deepening** : Recherche progressive (prof 1, puis 2, etc.) pour garantir de toujours avoir un coup à jouer même en cas de "timeout".
3. **Table de Transposition** : Mémorisation des plateaux déjà vus (via Zobrist Hashing) pour ne jamais calculer deux fois la même chose.
4. **Heuristiques** :
   - Différence de score (priorité absolue).
   - Mobilité (garder un maximum d'options).
   - Sécurité (éviter de laisser des trous à 1 ou 2 graines).
   - Famine (pénaliser les positions où l'on a trop peu de graines).


---

## Structure des Fichiers

Voici une description rapide des fichiers clés pour vous repérer dans le code :

- **`main.c`** : Point d'entrée du programme. Gère la boucle de jeu, la communication avec l'Arbitre (via `stdin`/`stdout`) et la gestion du temps (timer).
- **`ai_advanced.c`** : Cœur de l'intelligence artificielle. Contient l'algorithme Negamax, Alpha-Beta, toutes les optimisations (Zobrist, NMP, LMR) et la fonction d'évaluation.
- **`ai.c`** : Fonctions utilitaires de base (génération des clés de Zobrist, structure des coups, helpers).
- **`awale_game_macros.h`** : (Supprimé / Intégré dans game.h)
- **`bot_lounis_ouahrani.exe`** : L'exécutable final du bot (compilé pour Windows).
- **`Arbitre.java`** : Programme Java (fourni) qui sert d'interface graphique et de maître du jeu pour faire jouer les bots.

---

## Développement (Compilation)

Si vous souhaitez modifier le code source et recompiler le bot vous-même.

### Pré-requis
- Un compilateur C (`gcc`, `clang` ou `mingw` sur Windows).
- `make` (optionnel, mais recommandé).

### Commandes Makefile

- **Compiler** :
  ```bash
  make
  ```
  Crée le fichier `bot_lounis_ouahrani.exe`.

- **Nettoyer** :
  ```bash
  make clean
  ```
  Supprime les fichiers `.o` et `.exe`.

---

## Auteurs
- **LOUNIS Elies**
- **OUAHRANI KHALDI Sofiane**
