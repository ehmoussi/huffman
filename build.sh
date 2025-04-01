#!/bin/bash

set -xe


gcc -Wall -Werror huffman.c -o huffman && \
./huffman

