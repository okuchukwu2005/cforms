CC = gcc
FILE = main.c
EXE = main
SDL = -lSDL2 -lSDL2_ttf

all: $(EXE)

$(EXE): $(FILE)
		$(CC) $(FILE) -o $(EXE) $(SDL)

clean:
	rm -f $(EXE)
