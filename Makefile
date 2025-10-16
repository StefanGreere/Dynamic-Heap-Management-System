# compiler setup
CC=gcc
CFLAGS=-Wall -Wextra -std=c99

# define targets
TARGETS = sfl

build: sfl.c
	$(CC) $(CFLAGS) sfl.c -o sfl
run_sfl: build
	./sfl

pack:
	zip -FSr 315CA_GreereStefan_Tema1.zip README Makefile *.c *.h

clean:
	rm -f $(TARGETS)

.PHONY: pack clean build
