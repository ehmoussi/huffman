#!/bin/bash

set -e

if [ "$1" == "debug" ]; then
    echo "Build in DEBUG mode..."
    gcc -g -Wall -Wextra -Werror -pedantic -std=c99 -fstack-protector-all -fsanitize=address -fsanitize=undefined huffman.c -o huffman
else
    echo "Build in PRODUCTION mode..."
    gcc -g -Wall -Wextra -Werror -pedantic -std=c99 -O2 huffman.c -o huffman
fi

./huffman
