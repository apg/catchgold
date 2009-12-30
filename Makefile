CC = gcc
CFLAGS = -lncurses -lm -Wall

catchgold: catchgold.c
	$(CC) -o $@ $(CFLAGS) $<

clean:
	rm -rf catchgold core &>/dev/null
