allergy: main.c
	gcc main.c -lSDL2 -lSDL2_image -lSDL2_ttf -o allergy -Wall

debug: main.c
	gcc main.c -lSDL2 -lSDL2_image -lSDL2_ttf -DGAME_DEBUG -o debug_allergy 
