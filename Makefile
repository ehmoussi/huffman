CC = gcc
DEF_CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99

MODE ?= release

ifeq ($(MODE),debug)
	CFLAGS = -DDEBUG_MODE -g -fstack-protector-all -fsanitize=address -fsanitize=undefined $(DEF_CFLAGS)
else
	CFLAGS =  $(DEF_CFLAGS) -O2
endif

all: clean test_huffman

test_huffman: huffman.o test_huffman.o
	$(CC) $(CFLAGS) huffman.o test_huffman.o -o test_huffman

huffman.o: huffman.c
	$(CC) -c $(CFLAGS) huffman.c -o huffman.o

test_huffman.o: test_huffman.c
	$(CC) -c $(CFLAGS) test_huffman.c -o test_huffman.o

clean:
	rm -rf *.o test_huffman


