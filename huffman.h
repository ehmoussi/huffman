
#ifndef _HUFFMAN_H
#define _HUFFMAN_H 1

#include <stdlib.h>

#define _STATUS_CODE_TREE_FAIL 1
#define _STATUS_CODE_HEADER_FAIL 2
#define _STATUS_CODE_HEADER_CORRUPT 3
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

/// @brief Displays a bit message as a string of '0' and '1' characters
/// @param bit_message Pointer to the BitMessage structure to display
/// @param data Character array to store the result (must be pre-allocated with enough space)
void display_bit_message(const BitMessage *, char *);

/// @brief Frees all resources associated with an EncodedMessage
/// @param encoded_message Pointer to the EncodedMessage structure to free
void free_encoded_message(EncodedMessage *);

/// @brief Encodes a message using Huffman coding
/// @param message Null-terminated string to encode
/// @return Dynamically allocated string containing the encoded message or NULL if the encoding failed
void huffman_encode(const char *, EncodedMessage *);

/// @brief Decodes a Huffman-encoded message back to its original form
/// @param encoded_message Null-terminated string of an encoded message
/// @return Dynamically allocated string containing the decoded message or NULL if the decoding failed
char *huffman_decode(const EncodedMessage *);

#endif // HUFFMAN included