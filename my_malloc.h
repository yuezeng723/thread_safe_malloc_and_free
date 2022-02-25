#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#define true 1
#define false 0
#define word 8
#define NodeSize 24

typedef struct node_t Node;
struct node_t{
    Node * prev;
    Node * next;
    size_t size;//number of bytes allocated, size of the data block
    char data[1];//used to get the address of the data block start. can be used in data
};

Node * globalHead = NULL;
Node * globalTail = NULL;

__thread Node * threadHead = NULL;
__thread Node * threadTail = NULL;


//maintain free list
size_t alignWordSize(size_t size);
void deleteNode(Node * curr, Node ** currHead, Node ** currTail);
void insertNode(Node * curr, Node ** currHead, Node ** currTail);
void splitBlock(Node * curr, size_t alignedSize, Node ** currHead, Node **currTail);
Node * creatMallocBlock(size_t alignedSize, int flag);
void replaceBlock(Node * curr, size_t alignedSize, Node ** currHead, Node **currTail);
void * pointToBlock(void * ptr);
void mergeNode(Node * curr, Node ** currHead, Node ** currTail);
//best-fit thread unsafe malloc and free
void * bf_malloc(size_t size, int flag, Node ** currHead, Node ** currTail);
void bf_free(void * ptr, int flag, Node ** currHead, Node ** currTail);
Node * getBestFit(size_t alignedSize, Node ** currHead, Node ** currTail);
//lock
void * ts_malloc_lock(size_t size);
void ts_free_lock(void * ptr);
//nolock
void * ts_malloc_nolock(size_t size);
void ts_free_nolock(void * ptr);
//calculation
unsigned long get_largest_free_data_segment_size();
unsigned long get_total_free_size();