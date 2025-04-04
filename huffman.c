#include "huffman.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

#if DEBUG_MODE
#define PRINT_DEBUG(msg)              \
    do                                \
    {                                 \
        printf("DEBUG: %s\n", (msg)); \
    } while (0)
#else
#define PRINT_DEBUG(msg) \
    do                   \
    {                    \
    } while (0)
#endif
#if DEBUG_MODE
#define PRINT_DEBUG_HUFFMAN_NODE(node_ptr)                                 \
    do                                                                     \
    {                                                                      \
        char dbg_bit_msg[128];                                             \
        display_bit_message(&(node_ptr)->data->code, dbg_bit_msg);         \
        if ((node_ptr)->data->c != '\0')                                   \
            printf("DEBUG: %c -> %s\n", (node_ptr)->data->c, dbg_bit_msg); \
        else                                                               \
            printf("DEBUG: %s\n", dbg_bit_msg);                            \
    } while (0)
#else
#define PRINT_DEBUG_HUFFMAN_NODE(node_ptr) \
    do                                     \
    {                                      \
    } while (0)
#endif
#if DEBUG_MODE
#define PRINT_DEBUG_ALPHABET_CODE(char_code_ptr)                      \
    do                                                                \
    {                                                                 \
        char dbg_bit_msg[128];                                        \
        display_bit_message(&(char_code_ptr)->code, dbg_bit_msg);     \
        printf("DEBUG: %c -> %s\n", (char_code_ptr)->c, dbg_bit_msg); \
    } while (0)
#else
#define PRINT_DEBUG_ALPHABET_CODE(char_code_ptr) \
    do                                           \
    {                                            \
    } while (0)
#endif

/// @brief Structure to hold character, frequency and Huffman code
typedef struct CharCode
{
    char c;
    size_t freq;
    BitMessage code;
} CharCode;

/// @brief Structure to hold message information for Huffman encoding
typedef struct AlphabetCode
{
    CharCode *chars;
    size_t length;
} AlphabetCode;

/// @brief Node in a Huffman tree containing data and child pointers
typedef struct HuffmanNode
{
    CharCode *data;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

/// @brief Priority queue for Huffman nodes used during tree construction
typedef struct HuffmanQueue
{
    HuffmanNode **queue;
    size_t count;
    size_t capacity;

} HuffmanQueue;

/// @brief Frees all resources associated with a BitMessage
/// @param bit_message Pointer to the BitMessage structure to free
void free_bit_message(BitMessage *bit_message)
{
    if (bit_message->data != NULL)
    {
        free(bit_message->data);
    }
    bit_message->data = NULL;
    bit_message->nbits = 0;
    bit_message->nbytes = 0;
}

///@brief Frees all dynamically allocated memory in a CharCode structure
///@param char_code Pointer to CharCode structure to free
void free_char_code(CharCode *char_code)
{
    free_bit_message(&char_code->code);
    char_code->c = '\0';
    char_code->freq = 0;
}

///@brief Frees all dynamically allocated memory in a AlphabetCode structure
///@param alphabet Pointer to AlphabetCode structure to free
void free_alphabet_code(const AlphabetCode *alphabet)
{
    for (size_t i = 0; i < alphabet->length; i++)
        free_char_code(&alphabet->chars[i]);
    free(alphabet->chars);
}

/// @brief Frees resources associated with an encoded message
/// @param encoded_message Pointer to the EncodedMessage structure to free
void free_encoded_message(EncodedMessage *encoded_message)
{
    free_bit_message(&encoded_message->header);
    free_bit_message(&encoded_message->message);
}

/// @brief Frees resources associated with a Huffman tree node and its children
/// @param node Pointer to the HuffmanNode structure to free
void free_huffman_node(HuffmanNode *node)
{
    if (node == NULL)
        return;
    // Remove only the data of the parent nodes
    // The data of the characters are stored in the alphabet and will be free there
    if ((node->left != NULL || node->right != NULL) && node->data != NULL)
    {
        free_char_code(node->data);
        free(node->data);
        node->data = NULL;
    }
    HuffmanNode *left = node->left;
    node->left = NULL;
    HuffmanNode *right = node->right;
    node->right = NULL;
    free(node);
    free_huffman_node(left);
    free_huffman_node(right);
}

/// @brief Frees resources associated with a Huffman priority queue
/// @param queue Pointer to the HuffmanQueue structure to free
void free_huffman_queue(HuffmanQueue *queue)
{
    free(queue->queue);
    queue->count = 0;
    queue->capacity = 0;
}

/// @brief Prints an error message to stderr and exits with the given status
/// @param message Error message to print
/// @param status Exit status code
void exit_error(char *message, int status)
{
    fprintf(stderr, "ERROR: %s", message);
    exit(status);
}

/// @brief Gets the value of a bit at a specific position in a BitMessage
/// @param bit_message Pointer to the BitMessage structure
/// @param pos Position of the bit to retrieve (zero-indexed)
/// @return The bit value (0 or 1) at the specified position
int get_bit_message_value(const BitMessage *bit_message, size_t pos)
{
    return (bit_message->data[pos / CHAR_BIT] >> ((CHAR_BIT - 1) - (pos % CHAR_BIT))) & 1;
}

/// @brief Add the value of a bit at the end in a BitMessage and updates metadata
/// @param bit_message Pointer to the BitMessage structure to modify
/// @param pos Position of the bit to set (zero-indexed)
/// @param value Value to set (0 or 1)
void add_one_bit_message_value(BitMessage *bit_message, int value)
{
    size_t pos = bit_message->nbits;
    if (value)
        bit_message->data[pos / CHAR_BIT] |= (1 << ((CHAR_BIT - 1) - (pos % CHAR_BIT)));
    else
        bit_message->data[pos / CHAR_BIT] &= ~(1 << ((CHAR_BIT - 1) - (pos % CHAR_BIT)));
    pos += 1;
    bit_message->nbits = pos;
    bit_message->nbytes = pos / CHAR_BIT + 1;
}

/// @brief Creates a deep copy of a BitMessage structure
/// @param dest Pointer to the destination BitMessage structure
/// @param src Pointer to the source BitMessage structure to copy
void copy_bit_message(BitMessage *dest, const BitMessage *src)
{
    assert(dest->data == NULL);
    dest->data = malloc(src->nbytes * sizeof(unsigned char));
    memcpy(dest->data, src->data, src->nbytes);
    dest->nbits = src->nbits;
    dest->nbytes = src->nbytes;
}

/// @brief Removes the last bit from a BitMessage and updates metadata
/// @param bit_message Pointer to the BitMessage structure to modify
void remove_one_bit_message_value(BitMessage *bit_message)
{
    bit_message->nbits -= 1;
    bit_message->nbytes = bit_message->nbits / CHAR_BIT + 1;
}

/// @brief Converts a BitMessage to a readable string representation
/// @param bit_message Pointer to the BitMessage structure to display
/// @param data Character array to store the string representation
void display_bit_message(const BitMessage *bit_message, char *data)
{
    for (size_t i = 0; i < bit_message->nbits; ++i)
    {
        unsigned char byte = bit_message->data[i / CHAR_BIT];
        int bit_position = (CHAR_BIT - 1) - (i % CHAR_BIT);
        int bit = (byte >> bit_position) & 1;
        data[i] = bit ? '1' : '0';
    }
    data[bit_message->nbits] = '\0';
}

/// @brief Count the frequency of each ASCII character for the given message
/// @param message Null-terminated string to analyze for character frequencies
/// @param frequencies Array of frequencies for MAX_CHAR ASCII characters
void count_frequencies(const char *message, size_t *frequencies)
{
    if (message != NULL)
    {
        // Set the array to zero
        memset(frequencies, 0, MAX_CHAR * sizeof(char));
        for (size_t i = 0; message[i] != '\0'; ++i)
        {
            unsigned char index = (unsigned char)message[i];
            frequencies[index] += 1;
        }
    }
}

/// @brief Function used to compare CharCode entries for sorting by frequency
/// @param a Pointer to the first CharCode to compare
/// @param b Pointer to the second CharCode to compare
/// @return Negative if a's frequency is less than b's, positive if greater, zero if equal
int alphabet_freq_comparator(const void *a, const void *b)
{
    CharCode *char_code_a = (CharCode *)a;
    CharCode *char_code_b = (CharCode *)b;
    return (int)(char_code_a->freq < char_code_b->freq) - (char_code_a->freq > char_code_b->freq);
}

/// @brief Builds an alphabet structure containing characters and their frequencies
/// @param message Null-terminated string to analyze
/// @param alphabet Pointer to AlphabetCode structure to initialize
void build_alphabet(const char *message, AlphabetCode *alphabet)
{
    size_t frequencies[MAX_CHAR] = {0};
    count_frequencies(message, frequencies);
    size_t length = 0;
    for (size_t i = 0; i < MAX_CHAR; i++)
    {
        if (frequencies[i] > 0)
            length += 1;
    }
    alphabet->length = length;
    alphabet->chars = malloc(length * sizeof(CharCode));
    size_t char_idx = 0;
    for (size_t i = 0; i < MAX_CHAR; i++)
    {
        if (frequencies[i] > 0)
        {
            CharCode *char_code = &alphabet->chars[char_idx];
            char_code->c = (unsigned char)i;
            char_code->freq = frequencies[i];
            // Set the huffman code to NULL
            char_code->code.data = NULL;
            char_code->code.nbits = 0;
            char_code->code.nbytes = 0;
            char_idx += 1;
        }
    }
    // Sort the alphabet by the frequencies
    qsort(alphabet->chars, alphabet->length, sizeof(CharCode), alphabet_freq_comparator);
}

/// @brief Creates a new Huffman tree node for a character
/// @param char_code Pointer to CharCode structure for the character
/// @return Pointer to the newly created HuffmanNode
HuffmanNode *create_huffman_node(CharCode *char_code)
{
    HuffmanNode *node = malloc(sizeof(HuffmanNode));
    node->data = char_code;
    node->left = NULL;
    node->right = NULL;
    return node;
}

/// @brief Creates a parent Huffman node with two child nodes
/// @param left Pointer to the left child node
/// @param right Pointer to the right child node
/// @return Pointer to the newly created parent node
HuffmanNode *create_parent_huffman_node(HuffmanNode *left, HuffmanNode *right)
{
    HuffmanNode *node = malloc(sizeof(HuffmanNode));
    node->data = malloc(sizeof(CharCode));
    node->data->c = '\0',
    node->data->freq = (left->data->freq + right->data->freq),
    node->data->code.data = NULL;
    node->data->code.nbits = 0;
    node->data->code.nbytes = 0;
    node->left = left;
    node->right = right;
    return node;
}

/// @brief Adds a node to a Huffman priority queue
/// @param queue Pointer to the HuffmanQueue to add to
/// @param node Pointer to the HuffmanNode to add
void append_huffman_queue(HuffmanQueue *queue, HuffmanNode *node)
{
    size_t index = 0;
    while (index < queue->capacity)
    {
        if (queue->queue[index] == NULL)
            break;
        index += 1;
    }
    if (index >= queue->capacity)
    {
        size_t capacity = 2 * queue->capacity;
        queue->queue = realloc(queue->queue, capacity * sizeof(HuffmanNode *));
        memset(queue->queue + queue->capacity, 0, queue->capacity * sizeof(HuffmanNode *));
    }
    queue->queue[index] = node;
    queue->count += 1;
}

/// @brief Removes and returns the node with the lowest frequency from the queue
/// @param queue Pointer to the HuffmanQueue to pop from
/// @return Pointer to the HuffmanNode with minimum frequency
HuffmanNode *pop_min_freq_huffman_queue(HuffmanQueue *queue)
{
    HuffmanNode *node = NULL;
    int index = -1;
    for (int i = 0; i < (int)queue->capacity; ++i)
    {
        HuffmanNode *current_node = queue->queue[i];
        if (current_node != NULL && (index == -1 || current_node->data->freq < node->data->freq))
        {
            index = i;
            node = current_node;
        }
    }
    if (index >= 0)
    {
        queue->count -= 1;
        queue->queue[index] = NULL;
    }
    return node;
}

/// @brief Creates a HuffmanQueue and initializes it with nodes from the alphabet
/// @param alphabet Pointer to the AlphabetCode structure containing character information
/// @return Root node of the generated Huffman tree
HuffmanNode *generate_huffman_tree(const AlphabetCode *alphabet)
{
    HuffmanQueue queue = {
        .queue = calloc(2 * alphabet->length, sizeof(HuffmanNode *)),
        .count = 0,
        .capacity = 2 * alphabet->length,
    };
    // Add the alphabet in the queue
    for (size_t i = 0; i < alphabet->length; ++i)
    {
        HuffmanNode *node = create_huffman_node(&alphabet->chars[i]);
        append_huffman_queue(&queue, node);
    }
    PRINT_DEBUG("Add the alphabet in the queue");
    while (queue.count > 1)
    {
        HuffmanNode *left_node = pop_min_freq_huffman_queue(&queue);
        HuffmanNode *right_node = pop_min_freq_huffman_queue(&queue);
        HuffmanNode *parent_node = create_parent_huffman_node(left_node, right_node);
        append_huffman_queue(&queue, parent_node);
    }
    HuffmanNode *root = pop_min_freq_huffman_queue(&queue);
    free_huffman_queue(&queue);
    return root;
}

/// @brief Helper function that recursively generates Huffman codes for the given tree
/// @param message_info Pointer to MessageInfo structure to store character codes
/// @param node Current node in the Huffman tree
/// @param code_buffer Buffer to store the current code path
/// @param depth Current depth in the tree, representing code length
static void _generate_huffman_code(HuffmanNode *node, BitMessage *code_buffer)
{
    if (node == NULL)
        return;
    else if (node->left == NULL && node->right == NULL)
    {
        copy_bit_message(&node->data->code, code_buffer);
        PRINT_DEBUG_HUFFMAN_NODE(node);
    }
    else
    {
        if (node->left != NULL)
        {
            add_one_bit_message_value(code_buffer, 0);
            _generate_huffman_code(node->left, code_buffer);
            remove_one_bit_message_value(code_buffer);
        }
        if (node->right != NULL)
        {
            add_one_bit_message_value(code_buffer, 1);
            _generate_huffman_code(node->right, code_buffer);
            remove_one_bit_message_value(code_buffer);
        }
    }
}

/// @brief Transforms standard Huffman codes to canonical form
///         (see https://en.wikipedia.org/wiki/Canonical_Huffman_code)
/// @param alphabet Pointer to the AlphabetCode structure
void transform_to_canonical_code(const AlphabetCode *alphabet)
{
    if (alphabet->length == 0)
        return;
    unsigned int current_code = 0;
    size_t prev_nbits = alphabet->chars[0].code.nbits;
    for (size_t i = 0; i < alphabet->length; ++i)
    {
        size_t nbits = alphabet->chars[i].code.nbits;
        size_t nbytes = alphabet->chars[i].code.nbytes;
        memset(alphabet->chars[i].code.data, 0, nbytes * sizeof(unsigned char));
        if (nbits > prev_nbits)
        {
            current_code <<= (nbits - prev_nbits);
            prev_nbits = nbits;
        }
        for (size_t j = 0; j < nbits; j++)
        {
            int bit_value = (current_code >> (nbits - 1 - j)) & 1;
            if (bit_value)
                alphabet->chars[i].code.data[j / CHAR_BIT] |= (1 << ((CHAR_BIT - 1) - (j % CHAR_BIT)));
        }
        PRINT_DEBUG_ALPHABET_CODE(&alphabet->chars[i]);
        current_code++;
    }
}

/// @brief Generates Huffman codes for all characters in the alphabet
/// @param alphabet Pointer to the AlphabetCode structure
void generate_huffman_code(const AlphabetCode *alphabet)
{
    PRINT_DEBUG("Start generating huffman code");
    // Create the huffman tree
    HuffmanNode *root = generate_huffman_tree(alphabet);
    PRINT_DEBUG("Generate the huffman tree");
    if (root == NULL)
    {
        free_alphabet_code(alphabet);
        free_huffman_node(root);
        exit_error("Generation of the Huffman tree has failed", _STATUS_CODE_TREE_FAIL);
    }
    // Create a code buffer that will traverse the tree and define the code for each node
    BitMessage code_buffer = {
        .data = malloc((alphabet->length + 1) * sizeof(unsigned char)),
        .nbits = 0,
        .nbytes = 0,
    };
    // Create the huffman code by traversing the tree recursively
    _generate_huffman_code(root, &code_buffer);
    // Free the tree and the buffer
    free_huffman_node(root);
    free_bit_message(&code_buffer);
    // Transform the huffman to a canonical huffman code
    transform_to_canonical_code(alphabet);
}

/// @brief Encodes the alphabet information as a header for the compressed data
/// @param alphabet Pointer to the AlphabetCode structure
/// @param header Pointer to BitMessage structure to store the encoded header
void huffman_encode_alphabet(const AlphabetCode *alphabet, BitMessage *header)
{
    // The maximum number of bits is the last one because the alphabet is sorted
    unsigned int max_nbits = alphabet->chars[alphabet->length - 1].code.nbits;
    // Compute the size of the header
    // [[max_nbits][N_0...N_max_nbits][a_0...a_nb_chars]]
    size_t header_size = (max_nbits + 1 + alphabet->length);
    header->nbytes = header_size;
    header->nbits = header_size * sizeof(unsigned char);
    header->data = calloc(header_size, sizeof(unsigned char));
    // Store the maximum number of bits to indicate how many bytes to read after the first one
    header->data[0] = max_nbits;
    size_t current_idx = 1;
    // Encode the alphabet
    for (size_t i = 0; i < alphabet->length; ++i)
    {
        CharCode *char_code = &alphabet->chars[i];
        // next item of the header to count the number of characters with current_idx bits
        if (char_code->code.nbits > current_idx)
            current_idx += 1;
        // Increment the number of characters with the current number of bits
        header->data[current_idx] = ((unsigned char)header->data[current_idx]) + 1;
        // Add the character
        header->data[max_nbits + 1 + i] = char_code->c;
    }
}

/// @brief Encodes a message using Huffman coding based on the provided alphabet
/// @param message Null-terminated string to encode
/// @param alphabet Pointer to the AlphabetCode structure with character codes
/// @param encoded_message Pointer to BitMessage structure to store the encoded message
void huffman_encode_message(const char *message, AlphabetCode *alphabet, BitMessage *encoded_message)
{
    if (alphabet->length == 0)
        return;
    size_t capacity = 0;
    // Count the size of the encoded message
    for (size_t i = 0; message[i] != '\0'; ++i)
    {
        for (size_t j = 0; j < alphabet->length; ++j)
        {
            if (alphabet->chars[j].c == message[i])
                capacity += alphabet->chars[j].code.nbits;
        }
    }
    encoded_message->data = malloc((capacity / CHAR_BIT + 1) * sizeof(unsigned char));
    encoded_message->nbits = 0;
    encoded_message->nbytes = 0;
    for (size_t i = 0; message[i] != '\0'; ++i)
    {
        char c = message[i];
        // Iterate in reverse to have the greater frequency first since the alphabet is sorted
        // in ascending order according to the frequency
        for (int j = alphabet->length - 1; j >= 0; j--)
        {
            CharCode *char_code = &alphabet->chars[j];
            if (char_code->c == c)
            {
                for (size_t k = 0; k < char_code->code.nbits; ++k)
                {
                    int value = get_bit_message_value(&char_code->code, k);
                    add_one_bit_message_value(encoded_message, value);
                }
                break;
            }
        }
    }
}

/// @brief Recreates the alphabet from an encoded message header
/// @param encoded_message Pointer to EncodedMessage containing the header
/// @param alphabet Pointer to AlphabetCode structure to store the decoded alphabet
void huffman_decode_alphabet(const EncodedMessage *encoded_message, AlphabetCode *alphabet)
{
    unsigned int length = 0;
    const BitMessage *header = &encoded_message->header;
    if (header->nbytes > 0)
    {
        // Retrieve the maximum number of bits
        size_t max_nbits = (size_t)header->data[0];
        if (header->nbytes > (max_nbits + 1))
        {
            // Find the total number of unique characters
            for (unsigned int i = 0; i < max_nbits; ++i)
                length += (unsigned int)header->data[i + 1];
            // fill the message info
            alphabet->chars = malloc(length * sizeof(CharCode));
            alphabet->length = (size_t)length;
            size_t current_idx = 0;
            size_t current_header_idx = (size_t)max_nbits + 1;
            unsigned int current_code = 0;
            for (size_t i = 1; i < (max_nbits + 1); ++i)
            {
                size_t nchars = (size_t)header->data[i];
                size_t nbits = (size_t)i;
                size_t nbytes = nbits / CHAR_BIT + 1;
                for (unsigned int j = 0; j < nchars; ++j)
                {
                    CharCode *char_code = &alphabet->chars[current_idx];
                    char_code->c = header->data[current_header_idx];
                    char_code->code.data = calloc(nbytes, sizeof(unsigned char));
                    char_code->code.nbits = nbits;
                    char_code->code.nbytes = nbytes;
                    char_code->freq = 0;
                    for (size_t k = 0; k < nbits; k++)
                    {
                        int bit_value = (current_code >> (nbits - 1 - k)) & 1;
                        if (bit_value)
                            char_code->code.data[k / CHAR_BIT] |= (1 << ((CHAR_BIT - 1) - (k % CHAR_BIT)));
                    }
                    current_idx += 1;
                    current_header_idx += 1;
                    current_code += 1;
                }
                current_code <<= (i - 1);
            }
        }
    }
}

/// @brief Decodes a Huffman-encoded message using the provided alphabet
/// @param encoded_message Pointer to BitMessage containing the encoded data
/// @param alphabet Pointer to AlphabetCode structure with character codes
/// @return Dynamically allocated string containing the decoded message
char *huffman_decode_message(const BitMessage *encoded_message, const AlphabetCode *alphabet)
{
    // Compute the capacity
    size_t capacity = encoded_message->nbytes;
    char *decoded_message = malloc(capacity * sizeof(char));
    size_t start = 0;
    size_t current_idx = 0;
    int found_char = 0;
    for (; start < encoded_message->nbits;)
    {
        // TODO: Replace by a look up table
        for (size_t j = 0; j < alphabet->length; ++j)
        {
            CharCode *char_code = &alphabet->chars[j];
            // Search for the character
            size_t k = 0;
            while (
                k < char_code->code.nbits &&
                (get_bit_message_value(encoded_message, start + k) ==
                 get_bit_message_value(&char_code->code, k)))
                k++;
            // If all the bits are equal then we have found the character
            found_char = (k == char_code->code.nbits);
            if (found_char)
            {
                decoded_message[current_idx] = char_code->c;
                start += char_code->code.nbits;
                current_idx += 1;
                break;
            }
        }
        if (!found_char)
        {
            free(decoded_message);
            exit_error(
                "The header of the encoded message is corrupt. Can't decode a character.",
                _STATUS_CODE_HEADER_CORRUPT);
        }
        // If the current index is greater than the current capacity
        // Then we reallocate the decoded message
        if (current_idx >= capacity)
        {
            capacity = 2 * capacity;
            decoded_message = realloc(decoded_message, capacity);
        }
    }
    decoded_message[current_idx] = '\0';
    return decoded_message;
}

/// @brief Encodes a message using Huffman coding
/// @param message Null-terminated string to encode
/// @param encoded_message Pointer to EncodedMessage structure to store the result
void huffman_encode(const char *message, EncodedMessage *encoded_message)
{
    PRINT_DEBUG("START Encoding");
    if (strlen(message) == 0)
        return;
    // Build the alphabet of the message
    AlphabetCode alphabet = {.chars = NULL, .length = 0};
    build_alphabet(message, &alphabet);
    PRINT_DEBUG("build the alphabet");
    // Generate the huffman code for each character of the alphabet
    generate_huffman_code(&alphabet);
    PRINT_DEBUG("generate the huffman code for the alphabet");
    // Encode the alphabet
    huffman_encode_alphabet(&alphabet, &encoded_message->header);
    PRINT_DEBUG("encode the alphabet");
    // Exit if the encoding of the alphabet has failed
    if (encoded_message->header.data == NULL)
    {
        free_alphabet_code(&alphabet);
        exit_error("Failed to encode the alphabet code of the message.", _STATUS_CODE_HEADER_FAIL);
    }
    // Encode the message
    huffman_encode_message(message, &alphabet, &encoded_message->message);
    PRINT_DEBUG("encode the message");
    free_alphabet_code(&alphabet);
}

/// @brief Decodes a Huffman-encoded message
/// @param encoded_message Pointer to EncodedMessage structure containing encoded data
/// @return Dynamically allocated string containing the decoded message
char *huffman_decode(const EncodedMessage *encoded_message)
{
    char *decoded_message;
    if (encoded_message->header.data == NULL)
        return "";
    AlphabetCode alphabet = {.chars = NULL, .length = 0};
    huffman_decode_alphabet(encoded_message, &alphabet);
    decoded_message = huffman_decode_message(&encoded_message->message, &alphabet);
    free_alphabet_code(&alphabet);
    return decoded_message;
}
