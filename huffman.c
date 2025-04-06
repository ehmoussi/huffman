#include "huffman.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <stdint.h>

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
#define PRINT_DEBUG_HUFFMAN_NODE(node_ptr)                                             \
    do                                                                                 \
    {                                                                                  \
        char *dbg_bit_msg = malloc(((node_ptr)->data->code.nbits + 1) * sizeof(char)); \
        display_bit_message(&(node_ptr)->data->code, dbg_bit_msg);                     \
        if ((node_ptr)->data->c != '\0')                                               \
            printf("DEBUG: %c -> %s\n", (node_ptr)->data->c, dbg_bit_msg);             \
        else                                                                           \
            printf("DEBUG: %s\n", dbg_bit_msg);                                        \
        free(dbg_bit_msg);                                                             \
    } while (0)
#else
#define PRINT_DEBUG_HUFFMAN_NODE(node_ptr) \
    do                                     \
    {                                      \
    } while (0)
#endif
#if DEBUG_MODE
#define PRINT_DEBUG_ALPHABET_CODE(char_code_ptr)                                                                                                \
    do                                                                                                                                          \
    {                                                                                                                                           \
        char *dbg_bit_msg = malloc(((char_code_ptr)->code.nbits + 1) * sizeof(char));                                                           \
        display_bit_message(&(char_code_ptr)->code, dbg_bit_msg);                                                                               \
        printf("DEBUG: %c (freq=%ld, nbits=%ld) -> %s\n", (char_code_ptr)->c, (char_code_ptr)->freq, (char_code_ptr)->code.nbits, dbg_bit_msg); \
        free(dbg_bit_msg);                                                                                                                      \
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

/// @brief Structure to organize the alphabet as a binary tree according the bits of the codes
typedef struct AlphabetCodeTree
{
    CharCode **tree;
    size_t length;
    size_t min_nbits;
} AlphabetCodeTree;

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
        free(bit_message->data);
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
void free_alphabet_code(AlphabetCode *alphabet)
{
    for (size_t i = 0; i < alphabet->length; i++)
        free_char_code(&alphabet->chars[i]);
    if (alphabet->chars != NULL)
    {
        free(alphabet->chars);
        alphabet->chars = NULL;
    }
}

///@brief Free the AlphabetCode binary tree structure
///@param alphabet_code_tree Pointer to AlphabetCode structure to free
void free_alphabet_code_tree(AlphabetCodeTree *alphabet_code_tree)
{
    if (alphabet_code_tree->tree != NULL)
        free(alphabet_code_tree->tree);
    alphabet_code_tree->length = 0;
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
    queue->queue = NULL;
    queue->count = 0;
    queue->capacity = 0;
}

/// @brief Frees resources associated with a Huffman priority queue
/// @param queue Pointer to the HuffmanQueue structure to free
void free_huffman_queue_and_content(HuffmanQueue *queue)
{
    for (size_t i = 0; i < queue->capacity; ++i)
        free_huffman_node(queue->queue[i]);
    free_huffman_queue(queue);
}

/// @brief Prints an error message to stderr and exits with the given status
/// @param message Error message to print
/// @param status Exit status code
void exit_error(char *message, int status)
{
    fprintf(stderr, "ERROR: %s\n", message);
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
int copy_bit_message(BitMessage *dest, const BitMessage *src)
{
    assert(dest->data == NULL);
    dest->data = malloc(src->nbytes * sizeof(unsigned char));
    if (dest->data == NULL)
        return STATUS_CODE_ALLOC_FAIL;
    memcpy(dest->data, src->data, src->nbytes);
    dest->nbits = src->nbits;
    dest->nbytes = src->nbytes;
    return 0;
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
        // memset(frequencies, 0, MAX_CHAR * sizeof(size_t));
        for (size_t i = 0; message[i] != '\0'; ++i)
        {
            unsigned char index = (unsigned char)message[i];
            frequencies[index] += 1;
        }
    }
}

/// @brief Function used to compare CharCode entries for sorting by frequency then by lexicographical order of the characters
/// @param a Pointer to the first CharCode to compare
/// @param b Pointer to the second CharCode to compare
/// @return Positive if a's frequency is less than b's, negative if greater, zero if equal
int alphabet_freq_comparator(const void *a, const void *b)
{
    CharCode *char_code_a = (CharCode *)a;
    CharCode *char_code_b = (CharCode *)b;
    if (char_code_a->freq < char_code_b->freq)
        return 1;
    else if (char_code_a->freq > char_code_b->freq)
        return -1;
    else if (char_code_a->c < char_code_b->c)
        return 1;
    else if (char_code_a->c > char_code_b->c)
        return -1;
    return 0;
}

/// @brief Builds an alphabet structure containing characters and their frequencies
/// @param message Null-terminated string to analyze
/// @param alphabet Pointer to AlphabetCode structure to initialize
/// @return status code
int build_alphabet(const char *message, AlphabetCode *alphabet)
{
    if (alphabet->chars != NULL)
        return STATUS_CODE_ALPHABET_NOT_EMPTY;
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
    if (alphabet->chars == NULL)
        return STATUS_CODE_ALLOC_FAIL;
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
    return 0;
}

/// @brief Creates a new Huffman tree node for a character
/// @param char_code Pointer to CharCode structure for the character
/// @return Pointer to the newly created HuffmanNode
int create_huffman_node(CharCode *char_code, HuffmanNode **node)
{
    *node = malloc(sizeof(HuffmanNode));
    if (*node == NULL)
        return STATUS_CODE_ALLOC_FAIL;
    (*node)->data = char_code;
    (*node)->left = NULL;
    (*node)->right = NULL;
    return 0;
}

/// @brief Creates a parent Huffman node with two child nodes
/// @param left Pointer to the left child node
/// @param right Pointer to the right child node
/// @param parent Pointer to the newly created parent node
/// @return status code
int create_parent_huffman_node(HuffmanNode *left, HuffmanNode *right, HuffmanNode **parent)
{
    *parent = malloc(sizeof(HuffmanNode));
    if (*parent == NULL)
        return STATUS_CODE_ALLOC_FAIL;
    (*parent)->data = malloc(sizeof(CharCode));
    if ((*parent)->data == NULL)
        return STATUS_CODE_ALLOC_FAIL;
    (*parent)->data->c = '\0',
    (*parent)->data->freq = (left->data->freq + right->data->freq),
    (*parent)->data->code.data = NULL;
    (*parent)->data->code.nbits = 0;
    (*parent)->data->code.nbytes = 0;
    (*parent)->left = left;
    (*parent)->right = right;
    return 0;
}

/// @brief Adds a node to a Huffman priority queue
/// @param queue Pointer to the HuffmanQueue to add to
/// @param node Pointer to the HuffmanNode to add
/// @return status code
int append_huffman_queue(HuffmanQueue *queue, HuffmanNode *node)
{
    queue->count += 1;
    if ((queue->count) >= queue->capacity)
    {
        size_t capacity = 2 * queue->capacity;
        HuffmanNode **new_queue = realloc(queue->queue, capacity * sizeof(HuffmanNode *));
        if (new_queue == NULL)
        {
            free_huffman_queue_and_content(queue);
            queue->queue = NULL;
            return STATUS_CODE_ALLOC_FAIL;
        }
        queue->queue = new_queue;
        queue->capacity = capacity;
    }
    // Add at the end of the queue
    size_t current_index = queue->count - 1;
    queue->queue[current_index] = node;
    while (current_index > 0)
    {
        size_t parent_index = (current_index - 1) / 2;
        assert(parent_index < queue->count);
        HuffmanNode *parent_node = queue->queue[parent_index];
        if (parent_node->data->freq > node->data->freq ||
            (parent_node->data->c != '\0' &&
             node->data->c != '\0' &&
             parent_node->data->freq == node->data->freq &&
             parent_node->data->c > node->data->c))
        {
            // swap the nodes
            queue->queue[parent_index] = node;
            queue->queue[current_index] = parent_node;
            current_index = parent_index;
        }
        else
            break;
    }
    return 0;
}

/// @brief Removes and returns the node with the lowest frequency (and the lowest character) from the queue
/// @param queue Pointer to the HuffmanQueue to pop from
/// @return Pointer to the HuffmanNode with minimum frequency
HuffmanNode *pop_min_freq_huffman_queue(HuffmanQueue *queue)
{
    HuffmanNode *min_node = queue->queue[0];
    // Replace the root with the last element
    queue->queue[0] = queue->queue[queue->count - 1];
    queue->count -= 1;
    // Keep heap property
    size_t current_index = 0;
    while (current_index < queue->count)
    {
        HuffmanNode *current_node = queue->queue[current_index];
        size_t left_index = 2 * current_index + 1;
        size_t right_index = 2 * current_index + 2;
        HuffmanNode *left_node = NULL;
        if (left_index < queue->count)
            left_node = queue->queue[left_index];
        HuffmanNode *right_node = NULL;
        if (right_index < queue->count)
            right_node = queue->queue[right_index];
        if (left_node != NULL &&
            (right_node == NULL ||
             (right_node != NULL && alphabet_freq_comparator(left_node->data, right_node->data) == 1)) &&
            alphabet_freq_comparator(left_node->data, current_node->data) == 1)
        {
            // swap the nodes
            queue->queue[left_index] = current_node;
            queue->queue[current_index] = left_node;
            current_index = left_index;
        }
        else if (right_node != NULL &&
                 (left_node == NULL ||
                  (left_node != NULL && alphabet_freq_comparator(right_node->data, left_node->data) == 1)) &&
                 alphabet_freq_comparator(right_node->data, current_node->data) == 1)
        {
            // swap the nodes
            queue->queue[right_index] = current_node;
            queue->queue[current_index] = right_node;
            current_index = right_index;
        }
        else
        {
            break;
        }
    }
    return min_node;
}

/// @brief Creates a HuffmanQueue and initializes it with nodes from the alphabet
/// @param alphabet Pointer to the AlphabetCode structure containing character information
/// @param root node of the generated Huffman tree
/// @return status code
int generate_huffman_tree(const AlphabetCode *alphabet, HuffmanNode **root)
{
    HuffmanQueue queue = {
        .queue = calloc(2 * alphabet->length, sizeof(HuffmanNode *)),
        .count = 0,
        .capacity = 2 * alphabet->length,
    };
    if (queue.queue == NULL)
        return STATUS_CODE_ALLOC_FAIL;
    // Add the alphabet in the queue
    for (size_t i = 0; i < alphabet->length; ++i)
    {
        HuffmanNode *node = NULL;
        int status = create_huffman_node(&alphabet->chars[i], &node);
        if (status > 0)
        {
            free_huffman_queue_and_content(&queue);
            return status;
        }
        status = append_huffman_queue(&queue, node);
        if (status > 0)
        {
            free_huffman_queue_and_content(&queue);
            return status;
        }
    }
    PRINT_DEBUG("Add the alphabet in the queue");
    while (queue.count > 1)
    {
        HuffmanNode *left_node = pop_min_freq_huffman_queue(&queue);
        HuffmanNode *right_node = pop_min_freq_huffman_queue(&queue);
        HuffmanNode *parent_node = NULL;
        int status = create_parent_huffman_node(left_node, right_node, &parent_node);
        if (status > 0)
        {
            free_huffman_queue_and_content(&queue);
            return status;
        }
        status = append_huffman_queue(&queue, parent_node);
        if (status > 0)
        {
            free_huffman_queue_and_content(&queue);
            return status;
        }
    }
    *root = pop_min_freq_huffman_queue(&queue);
    free_huffman_queue(&queue);
    return 0;
}

/// @brief Helper function that recursively generates Huffman codes for the given tree
/// @param message_info Pointer to MessageInfo structure to store character codes
/// @param node Current node in the Huffman tree
/// @param code_buffer Buffer to store the current code path
/// @param depth Current depth in the tree, representing code length
/// @return status code
static int _generate_huffman_code(HuffmanNode *node, BitMessage *code_buffer)
{
    if (node == NULL)
        return 0;
    else if (node->left == NULL && node->right == NULL)
    {
        if (code_buffer->nbits == 0)
            add_one_bit_message_value(code_buffer, 0);
        int status = copy_bit_message(&node->data->code, code_buffer);
        if (status > 0)
            return status;
        PRINT_DEBUG_HUFFMAN_NODE(node);
    }
    else
    {
        if (node->left != NULL)
        {
            add_one_bit_message_value(code_buffer, 0);
            int status = _generate_huffman_code(node->left, code_buffer);
            if (status > 0)
                return status;
            remove_one_bit_message_value(code_buffer);
        }
        if (node->right != NULL)
        {
            add_one_bit_message_value(code_buffer, 1);
            int status = _generate_huffman_code(node->right, code_buffer);
            if (status > 0)
                return status;
            remove_one_bit_message_value(code_buffer);
        }
    }
    return 0;
}

/// @brief Function used to compare CharCode entries for sorting by nbits then by lexicographical order of the characters
/// @param a Pointer to the first CharCode to compare
/// @param b Pointer to the second CharCode to compare
/// @return Positive if a's nbits is greater than b's, negative if less, zero if equal
int alphabet_nbits_comparator(const void *a, const void *b)
{
    CharCode *char_code_a = (CharCode *)a;
    CharCode *char_code_b = (CharCode *)b;
    if (char_code_a->code.nbits > char_code_b->code.nbits)
        return 1;
    else if (char_code_a->code.nbits < char_code_b->code.nbits)
        return -1;
    else if (char_code_a->c > char_code_b->c)
        return 1;
    else if (char_code_a->c < char_code_b->c)
        return -1;
    return 0;
}

/// @brief Transforms standard Huffman codes to canonical form
///         (see https://en.wikipedia.org/wiki/Canonical_Huffman_code)
/// @param alphabet Pointer to the AlphabetCode structure
void transform_to_canonical_code(const AlphabetCode *alphabet)
{
    if (alphabet->length == 0)
        return;
    uint64_t current_code = 0;
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
/// @return status code
int generate_huffman_code(const AlphabetCode *alphabet)
{
    PRINT_DEBUG("Start generating huffman code");
    // Create the huffman tree
    HuffmanNode *root = NULL;
    int status = generate_huffman_tree(alphabet, &root);
    if (status > 0)
        return status;
    PRINT_DEBUG("Generate the huffman tree");
    if (root == NULL)
        return STATUS_CODE_TREE_FAIL;
    // Create a code buffer that will traverse the tree and define the code for each node
    BitMessage code_buffer = {
        .data = malloc((alphabet->length + 1) * sizeof(unsigned char)),
        .nbits = 0,
        .nbytes = 0,
    };
    if (code_buffer.data == NULL)
        return STATUS_CODE_ALLOC_FAIL;
    // Create the huffman code by traversing the tree recursively
    status = _generate_huffman_code(root, &code_buffer);
    // Free the tree and the buffer
    free_huffman_node(root);
    free_bit_message(&code_buffer);
    if (status > 0)
        return status;
    // Sort by number of bits of the code
    qsort(alphabet->chars, alphabet->length, sizeof(CharCode), alphabet_nbits_comparator);
    // Transform the huffman to a canonical huffman code
    PRINT_DEBUG("Transform the code to canonical code");
    transform_to_canonical_code(alphabet);
    return 0;
}

/// @brief Encodes the alphabet information as a header for the compressed data
/// @param alphabet Pointer to the AlphabetCode structure
/// @param header Pointer to BitMessage structure to store the encoded header
/// @return status code
int huffman_encode_alphabet(const AlphabetCode *alphabet, BitMessage *header)
{
    // The maximum number of bits is the last one because the alphabet is sorted
    unsigned int max_nbits = alphabet->chars[alphabet->length - 1].code.nbits;
    // Compute the size of the header
    // [[max_nbits][N_0...N_max_nbits][a_0...a_nb_chars]]
    size_t header_size = (max_nbits + 1 + alphabet->length);
    header->nbytes = header_size;
    header->nbits = header_size * sizeof(unsigned char) * CHAR_BIT;
    header->data = calloc(header_size, sizeof(unsigned char));
    if (header->data == NULL)
        return STATUS_CODE_ALLOC_FAIL;
    // Store the maximum number of bits to indicate how many bytes to read after the first one
    header->data[0] = max_nbits;
    size_t current_idx = 1;
    // Encode the alphabet
    for (size_t i = 0; i < alphabet->length; ++i)
    {
        CharCode *char_code = &alphabet->chars[i];
        // next item of the header to count the number of characters with current_idx bits
        while (char_code->code.nbits > current_idx)
            current_idx += 1;
        // Increment the number of characters with the current number of bits
        header->data[current_idx] = ((unsigned char)header->data[current_idx]) + 1;
        // Add the character
        header->data[max_nbits + 1 + i] = char_code->c;
    }
    return 0;
}

/// @brief Encodes a message using Huffman coding based on the provided alphabet
/// @param message Null-terminated string to encode
/// @param alphabet Pointer to the AlphabetCode structure with character codes
/// @param encoded_message Pointer to BitMessage structure to store the encoded message
int huffman_encode_message(const char *message, AlphabetCode *alphabet, BitMessage *encoded_message)
{
    if (encoded_message->data != NULL)
        return STATUS_CODE_ENCODED_MESSAGE_NOT_EMPTY;
    if (alphabet->length == 0)
        return 0;
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
    // Create a lookup table
    CharCode *lookup_alphabet[MAX_CHAR] = {0};
    for (size_t i = 0; i < alphabet->length; ++i)
        lookup_alphabet[(unsigned int)alphabet->chars[i].c] = &alphabet->chars[i];
    // Encode the message
    encoded_message->data = malloc((capacity / CHAR_BIT + 1) * sizeof(unsigned char));
    if (encoded_message->data == NULL)
        return STATUS_CODE_ALLOC_FAIL;
    encoded_message->nbits = 0;
    encoded_message->nbytes = 0;
    for (size_t i = 0; message[i] != '\0'; ++i)
    {
        char c = message[i];
        CharCode *char_code = lookup_alphabet[(unsigned int)c];
        assert(char_code != NULL);
        assert(char_code->c == c);
        for (size_t k = 0; k < char_code->code.nbits; ++k)
        {
            int value = get_bit_message_value(&char_code->code, k);
            add_one_bit_message_value(encoded_message, value);
        }
    }
    return 0;
}

/// @brief Recreates the alphabet from an encoded message header
/// @param encoded_message Pointer to EncodedMessage containing the header
/// @param alphabet Pointer to AlphabetCode structure to store the decoded alphabet
/// @return status code
int huffman_decode_alphabet(const EncodedMessage *encoded_message, AlphabetCode *alphabet)
{
    if (alphabet->chars != NULL)
        return STATUS_CODE_ALPHABET_NOT_EMPTY;
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
            if (alphabet->chars == NULL)
                return STATUS_CODE_ALLOC_FAIL;
            alphabet->length = (size_t)length;
            size_t current_idx = 0;
            size_t current_header_idx = (size_t)max_nbits + 1;
            uint64_t current_code = 0;
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
                    PRINT_DEBUG_ALPHABET_CODE(char_code);
                    current_header_idx += 1;
                    current_code += 1;
                }
                current_code <<= 1;
            }
        }
    }
    return 0;
}

/// @brief Create a binary tree using the code bits of the alphabet
/// @param alphabet Pointer to AlphabetCode structure to store the decoded alphabet
/// @param alphabet_code_tree Pointer to AlphabetCodeTree structure
/// @return status code
int create_alphabet_code_tree(const AlphabetCode *alphabet, AlphabetCodeTree *alphabet_code_tree)
{
    if (alphabet_code_tree->tree != NULL)
        return STATUS_CODE_ALPHABET_CODE_TREE_NOT_EMPTY;
    // Compute the number of nodes of the binary tree
    size_t min_nbits = 0;
    size_t max_nbits = 0;
    if (alphabet->length > 0)
    {
        min_nbits = alphabet->chars[0].code.nbits;
        max_nbits = alphabet->chars[alphabet->length - 1].code.nbits;
    }
    alphabet_code_tree->min_nbits = min_nbits;
    alphabet_code_tree->length = 1;
    for (size_t i = 0; i < (max_nbits + 1); ++i)
        alphabet_code_tree->length *= 2;
    alphabet_code_tree->length -= 1;
    // Fill the binary tree
    alphabet_code_tree->tree = calloc(alphabet_code_tree->length, sizeof(CharCode *));
    if (alphabet_code_tree->tree == NULL)
        return STATUS_CODE_ALLOC_FAIL;
    for (size_t i = 0; i < alphabet->length; ++i)
    {
        CharCode *char_code = &alphabet->chars[i];
        size_t pos = 0;
        for (size_t j = 0; j < char_code->code.nbits; ++j)
            pos = 2 * pos + 1 + (size_t)get_bit_message_value(&char_code->code, j);
        assert(pos < alphabet_code_tree->length);
        alphabet_code_tree->tree[pos] = char_code;
    }
    return 0;
}

/// @brief Decodes a Huffman-encoded message using the provided alphabet
/// @param encoded_message Pointer to BitMessage containing the encoded data
/// @param alphabet Pointer to AlphabetCode structure with character codes
/// @param decoded_message Point to the decoded message
/// @return status code
int huffman_decode_message(const BitMessage *encoded_message, const AlphabetCodeTree *alphabet_code_tree, char **decoded_message)
{
    if (*decoded_message != NULL)
        return STATUS_CODE_DECODED_MESSAGE_NOT_EMPTY;
    // Compute the capacity
    size_t capacity = 0;
    if (alphabet_code_tree->min_nbits > 0)
        capacity = encoded_message->nbits / alphabet_code_tree->min_nbits;
    *decoded_message = malloc(capacity * sizeof(char));
    if (*decoded_message == NULL)
        return STATUS_CODE_ALLOC_FAIL;
    size_t start = 0;
    size_t current_idx = 0;
    while (start < encoded_message->nbits)
    {
        // Search for the character
        size_t k = 0;
        size_t pos = 0;
        while (alphabet_code_tree->tree[pos] == NULL && pos < alphabet_code_tree->length)
        {
            pos = 2 * pos + 1 + get_bit_message_value(encoded_message, start + k);
            k += 1;
        }
        CharCode *char_code = alphabet_code_tree->tree[pos];
        if (char_code != NULL)
        {
            (*decoded_message)[current_idx] = char_code->c;
            start += char_code->code.nbits;
            current_idx += 1;
        }
        else
        {
            return STATUS_CODE_HEADER_CORRUPT;
        }
        // If the current index is greater than the current capacity
        // Then we reallocate the decoded message
        if (current_idx >= capacity)
        {
            capacity = 2 * capacity;
            char *new_decoded_message = realloc(*decoded_message, capacity);
            if (new_decoded_message == NULL)
            {
                free(*decoded_message);
                *decoded_message = NULL;
                return STATUS_CODE_ALLOC_FAIL;
            }
            *decoded_message = new_decoded_message;
        }
    }
    (*decoded_message)[current_idx] = '\0';
    return 0;
}

/// @brief Encodes a message using Huffman coding
/// @param message Null-terminated string to encode
/// @param encoded_message Pointer to EncodedMessage structure to store the result
/// @return Error code of the encoding, 0 if success, > 0 otherwise
int huffman_encode(const char *message, EncodedMessage *encoded_message)
{
    PRINT_DEBUG("START Encoding");
    if (strlen(message) == 0)
        return 0;
    // Build the alphabet of the message
    AlphabetCode alphabet = {.chars = NULL, .length = 0};
    int status = build_alphabet(message, &alphabet);
    if (status > 0)
    {
        free_alphabet_code(&alphabet);
        return status;
    }
    PRINT_DEBUG("build the alphabet");
    // Generate the huffman code for each character of the alphabet
    status = generate_huffman_code(&alphabet);
    if (status > 0)
    {
        free_alphabet_code(&alphabet);
        return status;
    }
    PRINT_DEBUG("generate the huffman code for the alphabet");
    // Encode the alphabet
    status = huffman_encode_alphabet(&alphabet, &encoded_message->header);
    if (status > 0)
    {
        free_alphabet_code(&alphabet);
        return status;
    }
    PRINT_DEBUG("encode the alphabet");
    // Exit if the encoding of the alphabet has failed
    if (encoded_message->header.data == NULL)
    {
        free_alphabet_code(&alphabet);
        return STATUS_CODE_HEADER_FAIL;
    }
    // Encode the message
    status = huffman_encode_message(message, &alphabet, &encoded_message->message);
    free_alphabet_code(&alphabet);
    if (status > 0)
        return status;
    PRINT_DEBUG("finish encoding the message");
    return 0;
}

/// @brief Decodes a Huffman-encoded message
/// @param encoded_message Pointer to EncodedMessage structure containing encoded data
/// @param decoded_message Dynamically allocated string containing the decoded message
/// @return status code
int huffman_decode(const EncodedMessage *encoded_message, char **decoded_message)
{
    // Decode the alphabet from the header of the encoded message
    AlphabetCode alphabet = {.chars = NULL, .length = 0};
    int status = huffman_decode_alphabet(encoded_message, &alphabet);
    if (status > 0)
        return status;
    // Create a tree from the alphabet
    AlphabetCodeTree alphabet_code_tree = {.tree = NULL, .length = 0};
    status = create_alphabet_code_tree(&alphabet, &alphabet_code_tree);
    if (status > 0)
    {
        free_alphabet_code(&alphabet);
        free_alphabet_code_tree(&alphabet_code_tree);
        return status;
    }
    // Decode the message using the HuffmanTree
    status = huffman_decode_message(&encoded_message->message, &alphabet_code_tree, decoded_message);
    if (status > 0 && *decoded_message != NULL)
    {
        free(*decoded_message);
        *decoded_message = NULL;
    }
    free_alphabet_code(&alphabet);
    free_alphabet_code_tree(&alphabet_code_tree);
    return status;
}
