#include "huffman.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

void test_huffman(const char *message)
{
    EncodedMessage encoded_message = {
        .header = {.data = NULL, .nbits = 0, .nbytes = 0},
        .message = {.data = NULL, .nbits = 0, .nbytes = 0}};
    huffman_encode(message, &encoded_message);
    char data[256] = {0};
    display_bit_message(&encoded_message.header, data);
    printf("HEADER: %s\n", data);
    display_bit_message(&encoded_message.message, data);
    printf("MESSAGE: %s\n", data);
    char *decoded_message = huffman_decode(&encoded_message);
    assert(strlen(decoded_message) == strlen(message));
    for (size_t i = 0; message[i] != '\0'; ++i)
        assert(decoded_message[i] == message[i]);
    free_encoded_message(&encoded_message);
    free(decoded_message);
}

int main(void)
{
    // Test 1
    const char *message = "aabbccddbbeaebdddfffdbffddabbbbbcdefaabbcccccaabbddfffdcecc";
    test_huffman(message);
    return 0;
}