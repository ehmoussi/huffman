#include <stdio.h>
#include <string.h>
#include <ctype.h>

void count_frequencies(const char *message, int *frequencies)
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

int main(void)
{
    int frequencies[256] = {0};
    const char *message = "aabbccddbbeaebdddfffdbffddabbbbbcdefaabbcccccaabbddfffdcecc\t";
    count_frequencies(message, frequencies);
    for (int i = 0; i < 256; i++)
    {
        if (frequencies[i] > 0)
        {
            char c = (char)i;
            if (isprint(c))
                printf("%c : %d\n", c, frequencies[i]);
            else
                printf("Hex: %x : %d\n", c, frequencies[i]);
        }
    }
    return 0;
}