#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_CHAR 256
#define BITS_PER_BYTE 8

/// @brief Structure representing a bit-level message
typedef struct BitMessage
{
    unsigned char *data;
    size_t nbits;
    size_t nbytes;
} BitMessage;

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

/// @brief Structure to hold character, frequency pair and Huffman code
typedef struct MessageChar
{
    char c;
    size_t freq;
    BitMessage code;
    BitMessage canonical_code;
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
    // TODO: Replace by a min heap queue
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

/// @brief Creates a MessageInfo structure from frequencies of characters in a message
/// @param frequencies Array of frequencies for each ASCII character (size MAX_CHAR)
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
            message_info.chars[current_idx].canonical_code.data = NULL;
            message_info.chars[current_idx].canonical_code.nbits = 0;
            message_info.chars[current_idx].canonical_code.nbytes = 0;
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
        if (message_info->chars[i].canonical_code.data != NULL)
            free(message_info->chars[i].canonical_code.data);
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

/// @brief Encodes a message header containing code lengths and symbol information
/// @param message_info Pointer to MessageInfo structure with canonical codes
/// @param header Pointer to BitMessage structure to store the header information
void huffman_encode_header(const MessageInfo *message_info, BitMessage *header)
{
    // The maximum number of bits is the last one because message_info has been sorted when
    // building the canonical code
    unsigned int max_nbits = message_info->chars[message_info->length - 1].canonical_code.nbits;
    size_t nmemb = (max_nbits + 1 + message_info->length);
    header->nbytes = nmemb;
    header->nbits = nmemb * sizeof(unsigned char);
    header->data = calloc(nmemb, sizeof(unsigned char));
    header->data[0] = max_nbits;
    size_t current_idx = 1;
    for (size_t i = 0; i < message_info->length; ++i)
    {
        MessageChar *message_char = &message_info->chars[i];
        if (message_char->canonical_code.nbits > current_idx)
            current_idx += 1;                                                       // next item of the header to count the number of characters with current_idx bits
        header->data[current_idx] = ((unsigned char)header->data[current_idx]) + 1; // Increment the count
        header->data[max_nbits + 1 + i] = message_char->c;                          // Add the character
    }
}

/// @brief Structure to hold a Huffman-encoded message with its header
typedef struct EncodedMessage
{
    BitMessage header;
    BitMessage message;
} EncodedMessage;

/// @brief Frees all resources associated with an EncodedMessage
/// @param encoded_message Pointer to the EncodedMessage structure to free
void free_encoded_message(EncodedMessage *encoded_message)
{
    free_bit_message(&encoded_message->header);
    free_bit_message(&encoded_message->message);
}

/// @brief Encodes a message using Huffman coding based on MessageInfo
/// @param message Null-terminated string to encode
/// @param message_info Pointer to MessageInfo containing character codes
/// @param encoded_message Pointer to BitMessage structure to store the encoded result
void huffman_encode(const char *message, const MessageInfo *message_info, EncodedMessage *encoded_message)
{
    // Add the header
    huffman_encode_header(message_info, &encoded_message->header);
    // Add the message
    size_t capacity = 0;
    // Count the size of the encoded message
    for (size_t i = 0; message[i] != '\0'; ++i)
    {
        for (size_t j = 0; j < message_info->length; ++j)
        {
            if (message_info->chars[j].c == message[i])
                capacity += message_info->chars[j].canonical_code.nbits;
        }
    }
    encoded_message->message.data = malloc((capacity / BITS_PER_BYTE + 1) * sizeof(unsigned char));
    encoded_message->message.nbits = 0;
    encoded_message->message.nbytes = 0;
    // Add the message
    for (size_t i = 0; message[i] != '\0'; ++i)
    {
        char c = message[i];
        for (size_t j = 0; j < message_info->length; j++)
        {
            MessageChar *message_char = &message_info->chars[j];
            if (message_char->c == c)
            {
                for (size_t k = 0; k < message_char->canonical_code.nbits; ++k)
                {
                    int value = get_bit_message_value(&message_char->canonical_code, k);
                    add_one_bit_message_value(&encoded_message->message, value);
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
char *huffman_decode(const BitMessage *encoded_message, const MessageInfo *message_info)
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
        for (size_t j = 0; j < message_info->length; ++j)
        {
            MessageChar *message_char = &message_info->chars[j];
            // Search for the character
            size_t k = 0;
            while (
                k < message_char->code.nbits &&
                (get_bit_message_value(encoded_message, start + k) ==
                 get_bit_message_value(&message_char->code, k)))
                k++;
            // If all the bits are equal then we have found the character
            found_char = (k == message_char->code.nbits);
            if (found_char)
            {
                decoded_message[current_idx] = message_char->c;
                start += message_char->code.nbits;
                current_idx += 1;
                break;
            }
        }
        if (!found_char)
            exit_error("ERROR: The header of the encoded message is corrupt. Can't decode a character.", 4);
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

/// @brief Compares two MessageChar structures for sorting by code length and character value
/// @param first Pointer to the first MessageChar to compare
/// @param second Pointer to the second MessageChar to compare
/// @return -1 if first comes before second, 1 if second comes before first, 0 if equal.
///         Sort by the number of bits and then in lexicographic order using the ASCII character
int code_comparator(const void *first, const void *second)
{
    MessageChar *mess_char_first = (MessageChar *)first;
    MessageChar *mess_char_second = (MessageChar *)second;
    if (mess_char_first->code.nbits > mess_char_second->code.nbits)
        return 1;
    else if (mess_char_first->code.nbits < mess_char_second->code.nbits)
        return -1;
    else if (mess_char_first->c > mess_char_second->c)
        return 1;
    else if (mess_char_first->c < mess_char_second->c)
        return -1;
    else
        return 0;
}

/// @brief Sorts the message characters and builds canonical Huffman codes
/// @param message_info Pointer to MessageInfo structure to update with canonical codes
void build_canonical_code(MessageInfo *message_info)
{
    // Sort the alphabet code
    qsort(message_info->chars, message_info->length, sizeof(MessageChar), code_comparator);
    // Build canonical code
    unsigned int current_code = 0;
    size_t prev_nbits = message_info->chars[0].code.nbits;
    for (size_t i = 0; i < message_info->length; ++i)
    {
        size_t nbits = message_info->chars[i].code.nbits;
        size_t nbytes = message_info->chars[i].code.nbytes;
        message_info->chars[i].canonical_code.data = calloc(nbytes, sizeof(unsigned char));
        message_info->chars[i].canonical_code.nbits = nbits;
        message_info->chars[i].canonical_code.nbytes = nbytes;
        if (nbits > prev_nbits)
        {
            current_code <<= (nbits - prev_nbits);
            prev_nbits = nbits;
        }
        for (size_t j = 0; j < nbits; j++)
        {
            int bit_value = (current_code >> (nbits - 1 - j)) & 1;
            if (bit_value)
                message_info->chars[i].canonical_code.data[j / BITS_PER_BYTE] |= (1 << ((BITS_PER_BYTE - 1) - (j % BITS_PER_BYTE)));
        }
        current_code++;
    }
}

/// @brief Extracts message information from an encoded message header
/// @param encoded_message Pointer to EncodedMessage containing the header
/// @param message_info Pointer to MessageInfo structure to populate
void create_message_info_from_header(const EncodedMessage *encoded_message, MessageInfo *message_info)
{
    // Find the total number of unique characters
    unsigned int length = 0;
    const BitMessage *header = &encoded_message->header;
    if (header->nbytes > 0)
    {
        size_t max_nbits = (size_t)header->data[0];
        if (header->nbytes > (max_nbits + 1))
        {
            for (unsigned int i = 0; i < max_nbits; ++i)
                length += (unsigned int)header->data[i + 1];
            // fill the message info
            message_info->chars = malloc(length * sizeof(MessageChar));
            message_info->length = (size_t)length;
            size_t current_idx = 0;
            size_t current_header_idx = (size_t)max_nbits + 1;
            unsigned int current_code = 0;
            for (size_t i = 1; i < (max_nbits + 1); ++i)
            {
                size_t nchars = (size_t)header->data[i];
                size_t nbits = (size_t)i;
                size_t nbytes = nbits / BITS_PER_BYTE + 1;
                for (unsigned int j = 0; j < nchars; ++j)
                {
                    MessageChar *message_char = &message_info->chars[current_idx];
                    message_char->c = header->data[current_header_idx];
                    message_char->code.data = calloc(nbytes, sizeof(unsigned char));
                    message_char->code.nbits = nbits;
                    message_char->code.nbytes = nbytes;
                    message_char->canonical_code.data = NULL;
                    message_char->canonical_code.nbits = 0;
                    message_char->canonical_code.nbytes = 0;
                    message_char->freq = 0;
                    for (size_t k = 0; k < nbits; k++)
                    {
                        int bit_value = (current_code >> (nbits - 1 - k)) & 1;
                        if (bit_value)
                            message_char->code.data[k / BITS_PER_BYTE] |= (1 << ((BITS_PER_BYTE - 1) - (k % BITS_PER_BYTE)));
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

int main(void)
{
    const char *message = "aabbccddbbeaebdddfffdbffddabbbbbcdefaabbcccccaabbddfffdcecc";
    // const char *message = "AAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCDDDDDDDDDDDD";
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
    // Build the canonical code
    build_canonical_code(&message_info);
    // Print the alphabet code
    char data[8] = {0};
    char canonical_data[8] = {0};
    for (size_t i = 0; i < message_info.length; ++i)
    {
        display_bit_message(&message_info.chars[i].code, data);
        display_bit_message(&message_info.chars[i].canonical_code, canonical_data);
        printf("%c: %s -> %s\n", message_info.chars[i].c, data, canonical_data);
    }
    // Encode the message
    EncodedMessage encoded_message = {
        .header = {.data = NULL, .nbits = 0, .nbytes = 0},
        .message = {.data = NULL, .nbits = 0, .nbytes = 0}};
    huffman_encode(message, &message_info, &encoded_message);
    char encoded_message_data[512] = {0};
    printf(
        "%" PRIuPTR " bytes used instead of %" PRIuPTR "\n",
        (encoded_message.header.nbytes + encoded_message.message.nbytes), strlen(message));
    display_bit_message(&encoded_message.message, encoded_message_data);
    printf("encoded message: %s\n", encoded_message_data);
    MessageInfo decoded_message_info = {.chars = NULL, .length = 0};
    create_message_info_from_header(&encoded_message, &decoded_message_info);
    // Print the alphabet code decoded from the header
    for (size_t i = 0; i < decoded_message_info.length; ++i)
    {
        display_bit_message(&decoded_message_info.chars[i].code, data);
        printf("%c: %s\n", decoded_message_info.chars[i].c, data);
    }
    char *decoded_message = huffman_decode(&encoded_message.message, &decoded_message_info);
    if (decoded_message == NULL)
    {
        // free messages
        free_encoded_message(&encoded_message);
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
    // TODO: Remove because the canonical code is different from the code
    // for (size_t i = 0; encoded_message_ref[i] != '\0'; ++i)
    //     assert(encoded_message_data[i] == encoded_message_ref[i]);
    // Test decode
    assert(strlen(decoded_message) == strlen(message));
    for (size_t i = 0; message[i] != '\0'; ++i)
        assert(decoded_message[i] == message[i]);
    // free messages
    free_encoded_message(&encoded_message);
    free(decoded_message);
    // free the tree
    free_node(root);
    // free message info
    free_message_info(&message_info);
    free_message_info(&decoded_message_info);
    return 0;
}
