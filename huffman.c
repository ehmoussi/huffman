#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_CHAR 256

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
    char *code;
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
    if (node->code != NULL)
        free(node->code);
    free(node);
    free_node(left);
    free_node(right);
}

/// @brief Recursively prints a node and its children in a tree-like format
/// @param node Node to print
void print_node(Node *node)
{
    if (node == NULL)
        return;
    if (node->c == '\0')
        printf(":%" PRIuPTR "(%s)", node->freq, node->code);
    else
        printf("%c:%" PRIuPTR "(%s)", node->c, node->freq, node->code);
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
    char *code;
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

/// @brief Creates a MessageInfo structure from a given message
/// @param message Null-terminated string to analyze
/// @return MessageInfo structure containing unique characters and their frequencies
MessageInfo create_message(const char *message)
{
    size_t frequencies[MAX_CHAR] = {0};
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
            message_info.chars[current_idx].code = NULL;
            current_idx += 1;
        }
    }
    return message_info;
}

/// @brief Free the message info allocated data
/// @param message_info Pointer to MessageInfo structure containing character data
void free_message_info(MessageInfo *message_info)
{
    for (size_t i = 0; i < message_info->length; i++)
    {
        if (message_info->chars[i].code != NULL)
            free(message_info->chars[i].code);
    }
    free(message_info->chars);
}

/// @brief Recursively generates Huffman codes for the given tree
/// @param message_info Pointer to MessageInfo structure to store character codes
/// @param node Current node in the Huffman tree
/// @param code Current code string for the path to this node
void generate_huffman(MessageInfo *message_info, Node *node, char *code)
{
    if (node == NULL)
        return;
    size_t n = strlen(code);
    node->code = code;
    if (node->left == NULL && node->right == NULL)
    {
        for (size_t i = 0; i < message_info->length; i++)
        {
            if (message_info->chars[i].c == node->c)
            {
                message_info->chars[i].code = malloc((n + 1) * sizeof(char));
                strcpy(message_info->chars[i].code, code);
                break;
            }
        }
    }
    if (node->left != NULL)
    {
        char *left_code = malloc((n + 2) * sizeof(char));
        strcpy(left_code, code);
        left_code[n] = '0';
        left_code[n + 1] = '\0';
        generate_huffman(message_info, node->left, left_code);
    }
    if (node->right != NULL)
    {
        char *right_code = malloc((n + 2) * sizeof(char));
        strcpy(right_code, code);
        right_code[n] = '1';
        right_code[n + 1] = '\0';
        generate_huffman(message_info, node->right, right_code);
    }
}

/// @brief Encodes a message using Huffman coding based on MessageInfo
/// @param message Null-terminated string to encode
/// @param message_info Pointer to MessageInfo containing character codes
/// @return Dynamically allocated string containing the encoded message
char *huffman_encode(const char *message, const MessageInfo *message_info)
{
    size_t capacity = strlen(message) + 1;
    char *encoded_message = malloc(capacity * sizeof(char));
    size_t current_idx = 0;
    for (size_t i = 0; message[i] != '\0'; ++i)
    {
        char c = message[i];
        for (size_t j = 0; j < message_info->length; j++)
        {
            if (message_info->chars[j].c == c)
            {
                for (size_t k = 0; message_info->chars[j].code[k] != '\0'; ++k)
                {
                    if (current_idx >= capacity)
                    {
                        capacity = 2 * capacity;
                        encoded_message = realloc(encoded_message, capacity * sizeof(char));
                    }
                    encoded_message[current_idx] = message_info->chars[j].code[k];
                    current_idx += 1;
                }
                break;
            }
        }
    }
    encoded_message[current_idx] = '\0';
    return encoded_message;
}

int main(void)
{
    const char *message = "aabbccddbbeaebdddfffdbffddabbbbbcdefaabbcccccaabbddfffdcecc";
    MessageInfo message_info = create_message(message);
    Node *root = generate_tree(&message_info);
    // Huffman coding
    char *code = malloc(1 * sizeof(char));
    code[0] = '\0';
    generate_huffman(&message_info, root, code);
    print_node(root);
    printf("\n");
    // Print the alphabet code
    for (size_t i = 0; i < message_info.length; ++i)
    {
        printf("%c: %s\n", message_info.chars[i].c, message_info.chars[i].code);
    }
    // free the tree
    free_node(root);
    // Encode the message
    char *encoded_message = huffman_encode(message, &message_info);
    // Test
    char *encoded_message_ref = "001001101011111101011010000001000100101011101101100110110110010100110101010101110100011000100110101111111111111110010011010010111011011001111000111111";
    assert(strlen(encoded_message) == strlen(encoded_message_ref));
    for (size_t i = 0; encoded_message_ref[i] != '\0'; ++i)
        assert(encoded_message[i] == encoded_message_ref[i]);
    printf("%s\n", encoded_message);
    free(encoded_message);
    // free
    free_message_info(&message_info);
    return 0;
}
