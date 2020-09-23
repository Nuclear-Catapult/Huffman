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

int main(int argc, uint8_t **argv)
{
    if (argc < 2)
        fprintf(stderr, "usage: ./huffman filename"), exit(1);

    int f = open(argv[1], O_RDONLY);
    if (f == -1)
        fprintf(stderr, "open failed\n"), exit(1);

    uint64_t byte_count[256] = { 0 };
    uint8_t buf[4096];
    int bytes;
    while ((bytes = read(f, buf, 4096)) > 0)
        for (int i = 0; i < bytes; i++)
            byte_count[buf[i]]++;

    if (bytes < 0)
        fprintf(stderr, "read failed\n"), exit(1);

    struct Q_Node front = { 0 };
    int i;
    for (i = 0; i < 256; i++)
        if (byte_count[i]) {
            front.count = byte_count[i];
            front.bst = calloc(sizeof(struct BST_Node), 1);
            front.bst->byte = i;
            break;
        }

    while (++i < 256)
        if (byte_count[i]) {
            struct Q_Node *iter;
            for (
                iter = &front;
                iter->next && iter->next->count < byte_count[i];
                iter = iter->next
            );
            struct Q_Node *new_node = calloc(sizeof(struct Q_Node), 1);
            new_node->count = byte_count[i];
            new_node->bst = calloc(sizeof(struct BST_Node), 1);
            new_node->bst->byte = i;
            new_node->next = iter->next;
            iter->next = new_node;
        }

    // print queue
    for (struct Q_Node *iter = &front; iter; iter = iter->next)
        printf("%d: %llu\n", iter->bst->byte, iter->count);


    close(f);
}

