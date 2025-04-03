#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_CHAR 256
#define BITS_PER_BYTE 8
/**
 * @brief Structure representing a bit-level message
 */
typedef struct BitMessage
{
    unsigned char *data;
    size_t nbits;
    size_t nbytes;
} BitMessage;

/// @brief Gets the value of a bit at a specific position in a BitMessage
/// @param bit_message Pointer to the BitMessage structure
/// @param pos Position of the bit to retrieve (zero-indexed)
/// @return The bit value (0 or 1) at the specified position
int get_bit_message_value(const BitMessage *bit_message, size_t pos)
{
    return (bit_message->data[pos / BITS_PER_BYTE] >> ((BITS_PER_BYTE - 1) - (pos % BITS_PER_BYTE))) & 1;
}

/// @brief Add the value of a bit at the end in a BitMessage and updates metadata
/// @param bit_message Pointer to the BitMessage structure to modify
/// @param pos Position of the bit to set (zero-indexed)
/// @param value Value to set (0 or 1)
void add_one_bit_message_value(BitMessage *bit_message, int value)
{
    size_t pos = bit_message->nbits;
    if (value)
        bit_message->data[pos / BITS_PER_BYTE] |= (1 << ((BITS_PER_BYTE - 1) - (pos % BITS_PER_BYTE)));
    else
        bit_message->data[pos / BITS_PER_BYTE] &= ~(1 << ((BITS_PER_BYTE - 1) - (pos % BITS_PER_BYTE)));
    pos += 1;
    bit_message->nbits = pos;
    bit_message->nbytes = pos / BITS_PER_BYTE + 1;
}

/// @brief Creates a deep copy of a BitMessage structure
/// @param dest Pointer to the destination BitMessage structure
/// @param src Pointer to the source BitMessage structure to copy
void copy_bit_message(BitMessage *dest, const BitMessage *src)
{
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
    bit_message->nbytes = bit_message->nbits / BITS_PER_BYTE + 1;
}

/**
 * @brief Displays a bit message as a string of '0' and '1' characters
 * @param bit_message Pointer to the BitMessage structure to display
 * @param data Character array to store the result (must be pre-allocated with enough space)
 */
void display_bit_message(const BitMessage *bit_message, char *data)
{
    for (size_t i = 0; i < bit_message->nbits; ++i)
    {
        unsigned char byte = bit_message->data[i / BITS_PER_BYTE];
        int bit_position = (BITS_PER_BYTE - 1) - (i % BITS_PER_BYTE);
        int bit = (byte >> bit_position) & 1;
        data[i] = bit ? '1' : '0';
    }
    data[bit_message->nbits] = '\0';
}

/// @brief Prints an error message to stderr and exits with the given status
/// @param message Error message to print
/// @param status Exit status code
void exit_error(char *message, int status)
{
    fprintf(stderr, "%s", message);
    exit(status);
}

/// @brief Exits the program due to a failed memory allocation
void exit_failed_allocation()
{
    exit_error("ERROR: failed to allocate\n", 1);
}

/// @brief Count the frequency of each ASCII character for the given message
/// @param message Null-terminated string to analyze for character frequencies
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

/// @brief Node structure for Huffman tree
typedef struct Node
{
    struct Node *left;
    struct Node *right;
    unsigned char c;
    size_t freq;
    BitMessage code;
} Node;

/// @brief Creates a new node with given character and frequency
/// @param c Character to store in the node
/// @param freq Frequency of the character
/// @return Pointer to the newly created node or NULL if allocation fails
Node *create_node(unsigned char c, size_t freq)
{
    Node *node = malloc(sizeof(Node));
    if (node == NULL)
        return NULL;
    node->left = NULL;
    node->right = NULL;
    node->c = c;
    node->freq = freq;
    node->code.data = NULL;
    node->code.nbits = 0;
    node->code.nbytes = 0;
    return node;
}

/// @brief Creates a new node with default values (null character and zero frequency)
/// @return Pointer to the newly created node or NULL if allocation fails
Node *create_default_node()
{
    return create_node('\0', 0);
}

/// @brief Creates a parent node from two child nodes
/// @param left Pointer to the left child node
/// @param right Pointer to the right child node
/// @return Pointer to the newly created parent node with frequency equal to sum of children
///         and the null terminator as the character
Node *create_parent_node(Node *left, Node *right)
{
    Node *parent_node = create_node('\0', left->freq + right->freq);
    if (parent_node == NULL)
        return NULL;
    parent_node->left = left;
    parent_node->right = right;
    return parent_node;
}

/// @brief Comparison function for nodes used in qsort
/// @param first Pointer to the first node pointer
/// @param second Pointer to the second node pointer
/// @return Negative if first frequency < second, positive if first > second, 0 if equal
int node_compare(const void *first, const void *second)
{
    const Node *first_node = *(Node **)first;
    const Node *second_node = *(Node **)second;
    if (first_node == NULL && second_node == NULL)
        return 0;
    if (second_node == NULL)
        return -1;
    if (first_node == NULL)
        return 1;
    size_t first_freq = first_node->freq;
    size_t second_freq = second_node->freq;
    return (int)((first_freq > second_freq) - (first_freq < second_freq));
}

/// @brief Sorts an array of node pointers by frequency
/// @param nodes Array of node pointers to sort
/// @param nb_nodes Number of nodes in the array
void sort_nodes(Node **nodes, size_t nb_nodes)
{
    qsort(nodes, nb_nodes, sizeof(Node *), node_compare);
}

/// @brief Recursively frees a node and all its children
/// @param node Root node to free
void free_node(Node *node)
{
    if (node == NULL)
        return;
    Node *left = node->left;
    node->left = NULL;
    Node *right = node->right;
    node->right = NULL;
    if (node->code.data != NULL)
        free(node->code.data);
    free(node);
    free_node(left);
    free_node(right);
}

/// @brief Recursively prints a node and its children in a tree-like format
/// @param node Node to print
void print_node(const Node *node)
{
    if (node == NULL)
        return;
    char data[8] = {0};
    display_bit_message(&node->code, data);
    if (node->c == '\0')
        printf(":%" PRIuPTR "(%s)", node->freq, data);
    else
        printf("%c:%" PRIuPTR "(%s)", node->c, node->freq, data);
    if (node->left != NULL || node->right != NULL)
    {
        printf(" {");
        print_node(node->left);
        printf(", ");
        print_node(node->right);
        printf("}");
    }
}

/// @brief Structure to hold character and frequency pair for Huffman encoding
typedef struct MessageChar
{
    char c;
    size_t freq;
    BitMessage code;
} MessageChar;

/// @brief Structure to hold message information for Huffman encoding
typedef struct MessageInfo
{
    MessageChar *chars;
    size_t length;
} MessageInfo;

/// @brief Generates a Huffman tree from message information
/// @param message_info Pointer to MessageChar structure containing character data
/// @return Root node of the generated Huffman tree
Node *generate_tree(const MessageInfo *message_info)
{
    size_t max_nodes = 2 * message_info->length - 1;
    Node **nodes = malloc(max_nodes * sizeof(Node *));
    if (nodes == NULL)
        exit_failed_allocation();
    memset(nodes, 0, max_nodes * sizeof(Node *));
    size_t current_nb_nodes = 0;
    for (size_t i = 0; i < message_info->length; i++)
    {
        nodes[current_nb_nodes] = create_node(message_info->chars[i].c, message_info->chars[i].freq);
        if (nodes[current_nb_nodes] == NULL)
            exit_failed_allocation();
        current_nb_nodes += 1;
    }
    sort_nodes(nodes, current_nb_nodes);
    while (current_nb_nodes > 1)
    {
        Node *left = nodes[0];
        Node *right = nodes[1];
        Node *parent = create_parent_node(left, right);
        if (parent == NULL)
        {
            // free before exiting if the allocation failed
            for (size_t i = 0; i < current_nb_nodes; i++)
            {
                free_node(nodes[i]);
            }
            free(nodes);
            exit_failed_allocation();
        }
        nodes[0] = parent;
        nodes[1] = NULL;
        sort_nodes(nodes, current_nb_nodes);
        current_nb_nodes -= 1;
    }
    Node *root = nodes[0];
    free(nodes);
    return root;
}

/// @brief Creates a MessageInfo structure from the frequencies of a message
/// @param frequencies Array of the frequencies for each 256 ASCII characters
/// @return MessageInfo structure containing unique characters and their frequencies
MessageInfo create_message(size_t *frequencies)
{
    size_t nb_unique_chars = 0;
    for (size_t i = 0; i < MAX_CHAR; ++i)
    {
        if (frequencies[i] > 0)
            nb_unique_chars += 1;
    }
    // allocate the list of characters
    MessageInfo message_info = {
        .chars = malloc(nb_unique_chars * sizeof(MessageChar)),
        .length = nb_unique_chars};
    if (message_info.chars == NULL)
        exit_failed_allocation();
    size_t current_idx = 0;
    for (size_t i = 0; i < MAX_CHAR; ++i)
    {
        if (frequencies[i] > 0)
        {
            message_info.chars[current_idx].c = (unsigned char)i;
            message_info.chars[current_idx].freq = frequencies[i];
            message_info.chars[current_idx].code.data = NULL;
            message_info.chars[current_idx].code.nbits = 0;
            message_info.chars[current_idx].code.nbytes = 0;
            current_idx += 1;
        }
    }
    return message_info;
}
/**
 * @brief Frees all dynamically allocated memory in a MessageInfo structure
 * @param message_info Pointer to MessageInfo structure to free
 */
void free_message_info(MessageInfo *message_info)
{
    for (size_t i = 0; i < message_info->length; i++)
    {
        if (message_info->chars[i].code.data != NULL)
            free(message_info->chars[i].code.data);
    }
    free(message_info->chars);
}

/// @brief Helper function that recursively generates Huffman codes for the given tree
/// @param message_info Pointer to MessageInfo structure to store character codes
/// @param node Current node in the Huffman tree
/// @param code_buffer Buffer to store the current code path
/// @param depth Current depth in the tree, representing code length
static void _generate_huffman(MessageInfo *message_info, Node *node, BitMessage *code_buffer, size_t depth)
{
    if (node == NULL)
        return;
    copy_bit_message(&node->code, code_buffer);
    if (node->left == NULL && node->right == NULL)
    {
        for (size_t i = 0; i < message_info->length; i++)
        {
            if (message_info->chars[i].c == node->c)
            {
                copy_bit_message(&message_info->chars[i].code, code_buffer);
                break;
            }
        }
    }
    if (node->left != NULL)
    {
        add_one_bit_message_value(code_buffer, 0);
        _generate_huffman(message_info, node->left, code_buffer, depth + 1);
        remove_one_bit_message_value(code_buffer);
    }
    if (node->right != NULL)
    {
        add_one_bit_message_value(code_buffer, 1);
        _generate_huffman(message_info, node->right, code_buffer, depth + 1);
        remove_one_bit_message_value(code_buffer);
    }
}

/// @brief Generates Huffman codes for all nodes in the tree
/// @param message_info Pointer to MessageInfo structure to store character codes
/// @param root Root node of the Huffman tree
void generate_huffman(MessageInfo *message_info, Node *root)
{
    BitMessage code_buffer = {
        .data = malloc((message_info->length + 1) * sizeof(unsigned char)),
        .nbits = 0,
        .nbytes = 0,
    };
    _generate_huffman(message_info, root, &code_buffer, 0);
    free(code_buffer.data);
}

/// @brief Encodes a message using Huffman coding based on MessageInfo
/// @param message Null-terminated string to encode
/// @param message_info Pointer to MessageInfo containing character codes
/// @param encoded_message Pointer to BitMessage structure to store the encoded result
void huffman_encode(const char *message, const MessageInfo *message_info, BitMessage *encoded_message)
{
    size_t capacity = 0;
    for (size_t i = 0; message[i] != '\0'; ++i)
    {
        for (size_t j = 0; j < message_info->length; ++j)
        {
            if (message_info->chars[j].c == message[i])
                capacity += message_info->chars[j].code.nbits;
        }
    }
    encoded_message->data = malloc((capacity / BITS_PER_BYTE + 1) * sizeof(unsigned char));
    encoded_message->nbits = 0;
    encoded_message->nbytes = 0;
    for (size_t i = 0; message[i] != '\0'; ++i)
    {
        char c = message[i];
        for (size_t j = 0; j < message_info->length; j++)
        {
            if (message_info->chars[j].c == c)
            {
                for (size_t k = 0; k < message_info->chars[j].code.nbits; ++k)
                {
                    int value = get_bit_message_value(&message_info->chars[j].code, k);
                    add_one_bit_message_value(encoded_message, value);
                }
                break;
            }
        }
    }
}

/// @brief Decodes a Huffman-encoded message back to its original form
/// @param encoded_message Null-terminated string containing the encoded bits ('0' and '1')
/// @param root Root node of the Huffman tree used for decoding
/// @return Dynamically allocated string containing the decoded message or NULL if the decoding failed
char *huffman_decode(const BitMessage *encoded_message, Node *root)
{
    // Compute the capacity
    size_t capacity = 0;
    Node *current_node = root;
    for (size_t i = 0; i < encoded_message->nbits; i++)
    {
        int value = get_bit_message_value(encoded_message, i);
        if (value)
            current_node = current_node->right;
        else
            current_node = current_node->left;
        if (current_node->left == NULL && current_node->right == NULL)
        {
            capacity += 1;
            current_node = root;
        }
    }
    capacity += 1; // null terminator
    char *decoded_message = malloc(capacity * sizeof(char));
    current_node = root;
    size_t current_idx = 0;
    for (size_t i = 0; i < encoded_message->nbits; i++)
    {
        int value = get_bit_message_value(encoded_message, i);
        if (value)
            current_node = current_node->right;
        else
            current_node = current_node->left;
        if (current_node->left == NULL && current_node->right == NULL)
        {
            decoded_message[current_idx] = current_node->c;
            current_idx += 1;
            current_node = root;
        }
    }
    if (current_node != root)
    {
        // Finished suddenly so the message have not completly decoded
        free(decoded_message);
        return NULL;
    }
    else
    {
        decoded_message[current_idx] = '\0';
        return decoded_message;
    }
}

int main(void)
{
    const char *message = "aabbccddbbeaebdddfffdbffddabbbbbcdefaabbcccccaabbddfffdcecc";
    // Count frequencies
    size_t frequencies[MAX_CHAR] = {0};
    count_frequencies(message, frequencies);
    print_frequencies(frequencies);
    // Create the message info structure
    MessageInfo message_info = create_message(frequencies);
    // Generate the Huffman tree
    Node *root = generate_tree(&message_info);
    // Huffman coding
    generate_huffman(&message_info, root);
    print_node(root);
    printf("\n");
    // Print the alphabet code
    char data[8] = {0};
    for (size_t i = 0; i < message_info.length; ++i)
    {
        display_bit_message(&message_info.chars[i].code, data);
        printf("%c: %s\n", message_info.chars[i].c, data);
    }
    // Encode the message
    BitMessage encoded_message = {.data = NULL, .nbits = 0, .nbytes = 0};
    huffman_encode(message, &message_info, &encoded_message);
    char encoded_message_data[256] = {0};
    display_bit_message(&encoded_message, encoded_message_data);
    printf("encoded message: %s\n", encoded_message_data);
    char *decoded_message = huffman_decode(&encoded_message, root);
    if (decoded_message == NULL)
    {
        // free messages
        free(encoded_message.data);
        // free the tree
        free_node(root);
        // free message info
        free_message_info(&message_info);
        exit_error("ERROR: invalid encoded message", 3);
    }
    printf("decoded message: %s\n", decoded_message);
    // Test encode
    char *encoded_message_ref = "001001101011111101011010000001000100101011101101100110110110010100110101010101110100011000100110101111111111111110010011010010111011011001111000111111";
    assert(strlen(encoded_message_data) == strlen(encoded_message_ref));
    for (size_t i = 0; encoded_message_ref[i] != '\0'; ++i)
        assert(encoded_message_data[i] == encoded_message_ref[i]);
    // Test decode
    assert(strlen(decoded_message) == strlen(message));
    for (size_t i = 0; message[i] != '\0'; ++i)
        assert(decoded_message[i] == message[i]);
    // free messages
    free(encoded_message.data);
    free(decoded_message);
    // free the tree
    free_node(root);
    // free message info
    free_message_info(&message_info);
    return 0;
}
