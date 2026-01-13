# ==========================================
# Makefile pour le Bot de Tournoi Awalé
# ==========================================

# Compilateur
# Si vous êtes sur Windows, gardez "gcc"
# Si vous compilez pour Windows depuis Mac, mettez "x86_64-w64-mingw32-gcc"
CC = x86_64-w64-mingw32-gcc 
#on a mis x86_64-w64-mingw32-gcc pour compiler en cross-compilation depuis mac vers windows car nous utilisons un mac

# Options de compilation
# -O3       : Optimisation maximale pour la vitesse :)
# -static   : Rend l'exécutable portable (important pour le tournoi)
# -DNDEBUG  : Désactive les assertions et debugs pour la performance
CFLAGS = -Wall -Wextra -std=c99 -O3 -static -DNDEBUG

# Nom de l'exécutable final
TARGET = bot_lounis_ouahrani.exe

# Fichiers sources du bot
SRCS = main.c game.c board.c move.c sow_and_capture.c ai.c ai_advanced.c

# Transformation automatique .c -> .o
OBJS = $(SRCS:.c=.o)

# --- Règles de compilation ---

all: $(TARGET)

# Création de l'exécutable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compilation des fichiers objets
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean