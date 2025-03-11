CC = clang
CFLAGS = -Wall -Wextra -Wnon-gcc -pedantic

fs: fs.c
	@$(CC) $(CFLAGS) -o fs.o

