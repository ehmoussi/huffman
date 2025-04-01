#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>

#define MAX_CHAR 256

void exit_error(char *message, int status)
{
    fprintf(stderr, "%s", message);
    exit(status);
}

void exit_failed_allocation()
{
    exit_error("ERROR: failed to allocate\n", 1);
}

/// @brief Count the frequency of each ASCII character for the given message
/// @param message
/// @param frequencies Array of frequencies for MAX_CHAR ASCII characters
void count_frequencies(const char *message, size_t *frequencies)
{
    if (message != NULL)
    {
        for (size_t i = 0; message[i] != '\0'; ++i)
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

/// @brief Structure to hold a character's index and its frequency
/// @param index The ASCII index of the character (0-255)
/// @param freq The frequency of the character
typedef struct Freq
{
    size_t index;
    size_t freq;
} Freq;

/// @brief Comparison function for qsort to sort Freq structures by frequency
/// @param first Pointer to the first Freq structure to compare
/// @param second Pointer to the second Freq structure to compare
/// @return Negative value if first < second, zero if equal, positive if first > second
int freq_compare(const void *first, const void *second)
{
    size_t first_freq = ((Freq *)first)->freq;
    size_t second_freq = ((Freq *)second)->freq;
    return (int)((first_freq > second_freq) - (first_freq < second_freq));
}

/// @brief Sort character indices by their frequency in ascending order
/// @param frequencies Array of frequencies for MAX_CHAR ASCII characters
/// @param sorted_freq_indices Output array to store sorted character indices
/// @param nb_unique_chars Number of unique characters in the message
/// @return 0 on success, 1 on memory allocation failure, 2 on internal inconsistency
int sort_asc_frequencies(const size_t *frequencies, size_t *sorted_freq_indices, size_t nb_unique_chars)
{
    Freq *freqs = malloc(nb_unique_chars * sizeof(Freq));
    if (freqs == NULL)
        return 1;
    size_t current_index = 0;
    for (size_t i = 0; i < MAX_CHAR; i++)
    {
        if (frequencies[i] > 0)
        {
            if (current_index >= nb_unique_chars)
            {
                free(freqs);
                return 2;
            }
            freqs[current_index].index = i;
            freqs[current_index].freq = frequencies[i];
            current_index += 1;
        }
    }
    qsort(freqs, nb_unique_chars, sizeof(Freq), freq_compare);
    for (size_t i = 0; i < nb_unique_chars; i++)
    {
        sorted_freq_indices[i] = freqs[i].index;
    }
    free(freqs);
    return 0;
}

/// @brief Print the sorted frequencies of characters
/// @param frequencies Array of frequencies for MAX_CHAR ASCII characters
/// @param sorted_freq_indices Array of indices sorted by frequency
/// @param nb_unique_chars Number of unique characters in the message
void print_sorted_frequencies(const size_t *frequencies, size_t *sorted_freq_indices, size_t nb_unique_chars)
{
    printf("sorted frequencies: ");
    for (size_t i = 0; i < nb_unique_chars; i++)
    {
        if (i > 0)
            printf(", ");
        size_t freq = frequencies[sorted_freq_indices[i]];
        printf("%" PRIuPTR, freq);
    }
    printf("\n");
}

int main(void)
{
    size_t frequencies[MAX_CHAR] = {0};
    const char *message = "aabbccddbbeaebdddfffdbffddabbbbbcdefaabbcccccaabbddfffdcecc";
    // Count frequencies
    count_frequencies(message, frequencies);
    print_frequencies(frequencies);
    //
    size_t nb_unique_chars = 0;
    for (size_t i = 0; i < MAX_CHAR; ++i)
    {
        if (frequencies[i] > 0)
            nb_unique_chars += 1;
    }
    size_t *sorted_freq_indices = malloc(nb_unique_chars * sizeof(size_t));
    if (sorted_freq_indices == NULL)
        exit_failed_allocation();
    int ret_code = sort_asc_frequencies(frequencies, sorted_freq_indices, nb_unique_chars);
    if (ret_code == 1)
    {
        exit_failed_allocation();
    }
    else if (ret_code == 2)
    {
        free(sorted_freq_indices);
        exit_error("ERROR: Internal inconsistency - calculated unique character count doesn't match actual count.\n", 2);
    }
    print_sorted_frequencies(frequencies, sorted_freq_indices, nb_unique_chars);
    // Find the character with the minimum frequency
    size_t freq_index = sorted_freq_indices[0];
    size_t min_freq = frequencies[freq_index];
    unsigned char min_char = (unsigned char)freq_index;
    if (min_freq > 0)
    {
        printf("min: ");
        print_char_frequency((unsigned char)min_char, min_freq);
    }
    free(sorted_freq_indices);
    return 0;
}