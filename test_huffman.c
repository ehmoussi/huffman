#include "huffman.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>

void test_huffman(const char *message)
{
    EncodedMessage encoded_message = {
        .header = {.data = NULL, .nbits = 0, .nbytes = 0},
        .message = {.data = NULL, .nbits = 0, .nbytes = 0}};
    int status = huffman_encode(message, &encoded_message);
    assert(status == 0);
    char data[40960] = {0};
    display_bit_message(&encoded_message.header, data);
    printf("HEADER: %s\n", data);
    display_bit_message(&encoded_message.message, data);
    printf("ENCODED MESSAGE: %s\n", data);
    char *decoded_message = NULL;
    status = huffman_decode(&encoded_message, &decoded_message);
    assert(status == 0);
    printf("DECODED MESSAGE: %s\n", decoded_message);
    assert(strlen(decoded_message) == strlen(message));
    for (size_t i = 0; message[i] != '\0'; ++i)
        assert(decoded_message[i] == message[i]);
    free_encoded_message(&encoded_message);
    if (decoded_message != NULL)
        free(decoded_message);
}

void generate_message(char *buffer, size_t length, unsigned int redundancy)
{
    if (length == 0)
        return;
    unsigned int alphabet_length = 5 + redundancy * 25;
    if (alphabet_length > MAX_CHAR - 1)
        alphabet_length = MAX_CHAR - 1;
    double proba[MAX_CHAR] = {0};
    double proba_sum = 0;
    for (size_t i = 0; i < alphabet_length; ++i)
    {
        proba[i] = 1.0 / (i + 1);
        proba_sum += proba[i];
    }
    for (size_t i = 0; i < alphabet_length; ++i)
        proba[i] /= proba_sum;
    double cum_distrib[MAX_CHAR] = {0};
    cum_distrib[0] = proba[0];
    if (alphabet_length > 1)
    {
        for (size_t i = 1; i < alphabet_length - 1; ++i)
            cum_distrib[i] = cum_distrib[i - 1] + proba[i];
    }
    // Add characters
    srand(time(NULL)); // initialize random seed
    for (size_t i = 0; i < (length - 1); ++i)
    {
        double random = (double)rand() / RAND_MAX;
        unsigned int c = 0;
        while (c < (alphabet_length - 1) && random > cum_distrib[c])
            c++;
        buffer[i] = 'a' + c % 26;
    }
    buffer[length - 1] = '\0';
}

int main(void)
{
    // Test 1
    test_huffman("aabbccddbbeaebdddfffdbffddabbbbbcdefaabbcccccaabbddfffdcecc");
    // Test empty
    test_huffman("");
    // Test two same characters
    test_huffman("a");
    // Test one character
    test_huffman("aa");
    // Test two different characters
    test_huffman("ab");
    // Test random
    char message[500];
    generate_message(message, 500, 10);
    printf("MESSAGE: %s\n", message);
    test_huffman(message);
    return 0;
}