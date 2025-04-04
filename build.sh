#!/bin/bash

set -e

if [ "$1" == "debug" ]; then
    echo "Build in DEBUG mode..."
    ARGS="-g -Wall -Wextra -Werror -pedantic -std=c99 -fstack-protector-all -fsanitize=address -fsanitize=undefined"
else
    echo "Build in PRODUCTION mode..."
    ARGS="-Wall -Wextra -Werror -pedantic -std=c99 -O2"
fi

gcc -c ${ARGS} huffman.c -o huffman.o
gcc ${ARGS} huffman.o test_huffman.c -o test_huffman

