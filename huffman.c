#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

// We only use the "byte" data in leaf nodes, and each node has either
// two or no children. Thus, if "left" is null (no left child), then we know 
// the node doesn't have a right child and can use "byte".
struct BST_Node {
	struct BST_Node* left;
    union {
        struct BST_Node* right;
        uint8_t byte;
    };
};

struct Q_Node {
    struct Q_Node* next;
    struct BST_Node* bst;
    uint64_t count;
};

void insert_Q_Node(struct Q_Node **iter, struct Q_Node *new_node)
{
    while (*iter && (*iter)->count < new_node->count)
        iter = &(*iter)->next;
    new_node->next = *iter;
    *iter = new_node;
}

double entropy(int64_t byte_count[256]);
void print_codes(struct BST_Node *node);

int main(int argc, uint8_t **argv)
{
    if (argc < 2)
        fprintf(stderr, "usage: ./huffman filename\n"), exit(1);

    printf("opening and reading file\n");
    int f = open(argv[1], O_RDONLY);
    if (f == -1)
        fprintf(stderr, "open failed\n"), exit(1);

    uint64_t byte_count[256] = { 0 };
    uint8_t input;
    int bytes;
    while ((bytes = read(f, &input, 1)) == 1)
        byte_count[input]++;
    close(f);
    if (bytes < 0)
        fprintf(stderr, "read failed\n"), exit(1);

    printf("generating priority queue\n");
    struct Q_Node *front = NULL;
    for (int i = 0; i < 256; i++) {
        if (byte_count[i]) {
            struct Q_Node *new_node = calloc(sizeof(struct Q_Node), 1);
            new_node->count = byte_count[i];
            new_node->bst = calloc(sizeof(struct BST_Node), 1);
            new_node->bst->byte = i;
            insert_Q_Node(&front, new_node);
        }
    }

    printf("transforming priority queue into huffman tree\n");
    while (front->next) {
        struct Q_Node *new_node = front;
        struct BST_Node *new_bst = malloc(sizeof(struct BST_Node));
        new_bst->left = new_node->next->bst;
        new_bst->right = new_node->bst;
        new_node->bst = new_bst;
        new_node->count += new_node->next->count;
        front = front->next->next;
        free(new_node->next);
        insert_Q_Node(&front, new_node);
    }   
    struct BST_Node *huffman_tree = front->bst;
    free(front);

    printf("printing codes\n");
    print_codes(huffman_tree);

    printf("entropy: %f\n", (float)entropy(byte_count));
}

#include <ctype.h>

#define BIT_COUNT(x) (sizeof(x)*8)

void _print_codes(struct BST_Node *node, uint64_t code)
{
    if (! node->left) {
        // found leaf node
        printf("0x%x %c\t", node->byte,
                isprint(node->byte) ? node->byte : ' ');
        uint64_t size = BIT_COUNT(code) - __builtin_ffsll(code);
        code >>= __builtin_ffsll(code);
        for (uint64_t i = 0; i < size; i++) {
            printf("%d", code & 1);
            code >>= 1;
        }
        printf("\n");
        return;
    }
    code >>= 1;
    _print_codes(node->left, code);
    code |= 0x8000000000000000;
    _print_codes(node->right, code);
}

inline void print_codes(struct BST_Node *node)
{
    _print_codes(node, 0x8000000000000000);
}

#include <math.h>

// determines how well a huffman tree can compress some data
double entropy(int64_t byte_count[256])
{
    int64_t total_bytes = 0;
    // count bytes in file
    for (int i = 0; i < 256; i++)
        total_bytes += byte_count[i];

    double e = 0;
    for (int i = 0; i < 256; i++)
        if (byte_count[i]) {
            double p = (double)byte_count[i] / total_bytes;
            e += p * (log(p) / log(2));
        }
    return -e;
}
