#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#define MAX_CHAR 256

/// @brief Count the frequency of each ASCII character for the given message
/// @param message
/// @param frequencies Array of frequencies for MAX_CHAR ASCII characters
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

/// @brief Print the character and its frequency. If the character is not printable
////       then its hexadecimal representation is used and specified.
/// @param c character to print
/// @param frequency frequency of the character
void print_char_frequency(unsigned char c, size_t frequency)
{
    if (isprint(c))
        printf("%c : %" PRIuPTR "\n", c, frequency);
    else
        printf("Hex: %x : %" PRIuPTR "\n", c, frequency);
}

/// @brief Print all the characters and its frequency when > 0 if the character
////       is not printable the hexadecimal is print instead and specified
/// @param frequencies Array of frequencies for MAX_CHAR ASCII characters
void print_frequencies(const size_t *frequencies)
{
    for (size_t i = 0; i < MAX_CHAR; i++)
    {
        if (frequencies[i] > 0)
        {
            unsigned char c = (unsigned char)i;
            print_char_frequency(c, frequencies[i]);
        }
    }
}

/// @brief Find the character with the minimum frequency and a certain threshold
/// @param frequencies Array of frequencies for MAX_CHAR ASCII characters
/// @param threshold threshold above which which the search is done
/// @return the minimum ASCII character or -1 if none was found
int find_minimum_char(const size_t *frequencies, size_t threshold)
{
    int min_char = -1;
    size_t current_min = SIZE_MAX;
    for (size_t i = 0; i < MAX_CHAR; i++)
    {
        if (frequencies[i] > threshold && frequencies[i] < current_min)
        {
            current_min = frequencies[i];
            min_char = (int)i;
        }
    }
    return min_char;
}

int main(void)
{
    size_t frequencies[MAX_CHAR] = {0};
    const char *message = "aabbccddbbeaebdddfffdbffddabbbbbcdefaabbcccccaabbddfffdcecc\t";
    // Count frequencies
    count_frequencies(message, frequencies);
    print_frequencies(frequencies);
    // Find the character with the minimum frequency greater than a given frequency
    int min_char = find_minimum_char(frequencies, 0);
    print_char_frequency((unsigned char)min_char, frequencies[(size_t)min_char]);
    return 0;
}