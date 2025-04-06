
#ifndef _HUFFMAN_H
#define _HUFFMAN_H 1

#include <stdlib.h>

#define STATUS_CODE_ALLOC_FAIL 1
#define STATUS_CODE_TREE_FAIL 2
#define STATUS_CODE_ALPHABET_NOT_EMPTY 3
#define STATUS_CODE_ALPHABET_CODE_TREE_NOT_EMPTY 4
#define STATUS_CODE_ENCODED_MESSAGE_NOT_EMPTY 5
#define STATUS_CODE_DECODED_MESSAGE_NOT_EMPTY 6
#define STATUS_CODE_HEADER_FAIL 7
#define STATUS_CODE_HEADER_CORRUPT 8

#define MAX_CHAR 256

/// @brief Structure representing a bit-level message
typedef struct BitMessage
{
    unsigned char *data;
    size_t nbits;
    size_t nbytes;
} BitMessage;

/// @brief Structure to hold a Huffman-encoded message with its header
typedef struct EncodedMessage
{
    BitMessage header;
    BitMessage message;
} EncodedMessage;

void display_bit_message(const BitMessage *, char *);

void free_encoded_message(EncodedMessage *);

int huffman_encode(const char *, EncodedMessage *);

int huffman_decode(const EncodedMessage *, char **);

#endif // HUFFMAN included