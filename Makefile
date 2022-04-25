NAME=proj2
CC=gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
LFLAGS=-pthread

all:
	$(CC) $(CFLAGS) $(NAME).c -o $(NAME) $(LFLAGS)