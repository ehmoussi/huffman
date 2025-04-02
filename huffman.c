#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>

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
        printf(":%" PRIuPTR, node->freq);
    else
        printf("%c:%" PRIuPTR, node->c, node->freq);
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
            current_idx += 1;
        }
    }
    return message_info;
}

int main(void)
{
    const char *message = "aabbccddbbeaebdddfffdbffddabbbbbcdefaabbcccccaabbddfffdcecc";
    MessageInfo message_info = create_message(message);
    Node *root = generate_tree(&message_info);
    print_node(root);
    printf("\n");
    // free
    free(message_info.chars);
    free_node(root);
    return 0;
}
