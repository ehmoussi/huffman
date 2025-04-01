#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#define MAX_CHAR 256

void count_frequencies(const char *message, size_t *frequencies)
{
    if (message != NULL)
    {
        size_t msg_size = strlen(message);
        for (size_t i = 0; i < msg_size; ++i)
        {
            unsigned char index = (unsigned char)message[i];
            frequencies[index] += 1;
        }
    }
}

void print_frequencies(const size_t *frequencies)
{
    for (int i = 0; i < MAX_CHAR; i++)
    {
        if (frequencies[i] > 0)
        {
            char c = (char)i;
            if (isprint(c))
                printf("%c : %" PRIuPTR "\n", c, frequencies[i]);
            else
                printf("Hex: %x : %" PRIuPTR "\n", c, frequencies[i]);
        }
    }
}

char find_minimum_char(const size_t *frequencies, size_t min_freq)
{
    char min_char = (char)-1;
    size_t current_min = SIZE_MAX;
    for (size_t i = 0; i < MAX_CHAR; i++)
    {
        if (frequencies[i] > min_freq && frequencies[i] < current_min)
        {
            current_min = frequencies[i];
            min_char = (char)i;
        }
    }
    return min_char;
}

int main(void)
{
    size_t frequencies[MAX_CHAR] = {0};
    const char *message = "aabbccddbbeaebdddfffdbffddabbbbbcdefaabbcccccaabbddfffdcecc";
    // Count frequencies
    count_frequencies(message, frequencies);
    print_frequencies(frequencies);
    // Find the character with the minimum frequency greater than a given frequency
    char min_char = find_minimum_char(frequencies, 0);
    printf("min: %c\n", min_char);

    return 0;
}