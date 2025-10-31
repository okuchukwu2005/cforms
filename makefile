CC = gcc
FILE = main.c
CFLAGS = -Wall
EXE = main
SDL = -lSDL2 -lSDL2_ttf -lSDL2_image -lm

# Declare phony targets
.PHONY: all clean run

all: $(EXE)

$(EXE): $(FILE)
	$(CC) $(FILE) -o $(EXE) $(SDL)

clean:
	rm -f $(EXE)

run: clean all
	./$(EXE)
