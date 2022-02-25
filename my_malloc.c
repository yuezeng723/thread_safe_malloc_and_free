#include "my_malloc.h"

pthread_mutex_t lock;

//align size passed in to the multiplier of a word size
//This system's word size is 8
size_t alignWordSize(size_t size){
    size_t alignedSize = ((((size - 1)>>3)<<3)+8);
    return alignedSize;
}

//delete Node in the free list
void deleteNode(Node * curr, Node ** head, Node ** tail){
    //no Node in List
    if (*head == NULL){
        return;
    }
    //one Node in List
    if (curr == *head && curr == *tail){
        *head = NULL;
        *tail = NULL;
    }
    //at least two Nodes in List
    ///delete head
    else if (curr == *head){
        *head = curr->next;
        (*head)->prev = NULL;
        curr->next = NULL;
    }
    ///delete tail
    else if (curr == *tail){
        *tail = curr->prev;
        (*tail)->next = NULL;
        curr->prev = NULL;
    }
    ///delte middle
    else {
        curr->prev->next = curr->next;
        curr->next->prev = curr->prev;
        curr->prev = NULL;
        curr->next = NULL;
    }
}

//insert Node into free list accoring to memory address order
void insertNode(Node * curr, Node ** head, Node ** tail){
    //insert the first Node
    if (*head == NULL){
        *head = curr;
        *tail = curr;
        return;
    }
    //insert to head
    if (curr < *head){
        curr->prev = NULL;
        curr->next = *head;
        (*head)->prev = curr;
        *head = curr;
        return;
    }
    //insert to tail
    else if (curr > *tail){
        curr->prev = *tail;
        curr->next = NULL;
        (*tail)->next = curr;
        *tail = curr;
        return;
    }
    //insert to middle
    else {
        Node * ptr = *head;
        while(ptr != NULL){
            //insert the curr after the Node ptr
            if (curr < ptr){
                curr->next = ptr;
                curr->prev = ptr->prev;
                ptr->prev->next = curr;
                ptr->prev = curr;
                return;
            }
            ptr = ptr->next;
        }
    }
}

//split remaining free memory in block which is allocated and found by FF
void splitBlock(Node * curr, size_t alignedSize, Node ** head, Node ** tail){
    char * temp = (char* )curr;
    temp += NodeSize + alignedSize;
    Node * newNode = (Node *)temp;
    //initialize newly splited free Node
    newNode->size = curr->size - NodeSize - alignedSize;
    newNode->prev = NULL;
    newNode->next = NULL;
    //update curr Node
    curr->size = alignedSize;
    //update the free list
    deleteNode(curr, head, tail);
    insertNode(newNode, head, tail);
    mergeNode(newNode, head, tail);
}

//creat and initialize allocated memory block in term of Node
Node * creatMallocBlock(size_t alignedSize, int flag){
    Node * newNode = NULL;
    void * breakPoint = NULL;
    if (flag == false){
        pthread_mutex_lock(&lock);
        newNode = sbrk(0);
        breakPoint = sbrk(alignedSize + NodeSize);
        pthread_mutex_unlock(&lock);
    }else{
        newNode = sbrk(0);
        breakPoint = sbrk(alignedSize + NodeSize);
    }
    if (breakPoint == (void * ) -1){ return NULL;}
    newNode->size = alignedSize;
    newNode->prev = NULL;
    newNode->next = NULL;
    return newNode;
}


//when malloc memory, replace Node in the free list found by FF
void replaceBlock(Node * curr, size_t alignedSize, Node ** head, Node ** tail){
    if ( curr->size - alignedSize >= NodeSize + word){
        splitBlock(curr, alignedSize, head, tail);
        return;
    }
    //curr->size = alignedSize;
    //printf("malloc size is %ld\n", curr->size);
    deleteNode(curr, head, tail);
}

//translate the address pointer to the pointer of Node
void * pointToBlock(void * ptr){
    char * curr = ptr;
    curr -= NodeSize;
    return (void *)curr;
}

//merge the node which is already in the free list
//Node curr is already in the free list
void mergeNode(Node * curr, Node ** head, Node ** tail){
    //only curr in list, dont merge
    if (*head == curr && *tail == curr){
        return;
    }
    //calculate the front and back of Node curr, its front is curr
    char * currBack = (char *)curr;
    currBack += NodeSize + curr->size;
    //merge Node with the next Node. If curr is tail, dont merge
    if (currBack == (char *)curr->next && curr != *tail){
        curr->size += NodeSize + curr->next->size;
        //When curr->next is tail, tail update to curr
        if (curr->next == *tail){
            curr->next = NULL;
            *tail = curr;
        }
        else {
            curr->next->next->prev = curr;
            curr->next = curr->next->next;
        }
    }
    if (curr == *head){return;}
    //merge Node with previous Node.
    ///calculate the back of Node curr->prev
    ///when curr is head, not merge and directly return
    char * currPrevBack = (char *)curr->prev;
    currPrevBack += NodeSize + curr->prev->size;
    if (currPrevBack == (char *)curr && curr != *head){
        curr->prev->size += NodeSize + curr->size;
        //When curr is tail, tail need update to the curr->prev
        if (curr == *tail){
            curr->prev->next = NULL;
            *tail = curr->prev;
        }
        else {
            curr->next->prev = curr->prev;
            curr->prev->next = curr->next;
        }
    }
}

//BF strategy to get the avaliable Node in free list
Node * getBestFit(size_t alignedSize, Node ** head, Node ** tail){
    //traverse free list find the minimal avaliable Node
    Node * bestFit = NULL;
    if (*head == NULL){return NULL;}
    else {
        Node * curr = *head;
        size_t min = curr->size;
        while (curr != NULL){
            if (curr->size > alignedSize && curr->size < min){
                min = curr->size;
                bestFit = curr;
            }
            else if (curr->size == alignedSize){
                return curr;
            }
            curr = curr->next;
        }
    }
    return bestFit;
}

//BF strategy wrapper for malloc
//flag is true for lock; false for no lock
void * bf_malloc(size_t size, int flag, Node ** head, Node ** tail){
    pthread_mutex_lock(&lock);
    void * ptr = sbrk(0);
    if (flag == false){
        pthread_mutex_unlock(&lock);
    }
    size_t alignedSize = alignWordSize(size);//首先对齐待分配size
    if (size == 0 || ptr == (void *) -1){
        if (flag == true){
            pthread_mutex_unlock(&lock);
        }
        return NULL;
    }
    //no Node in free list
    else {
        //BF traverse free list find available Node
        Node * bestFit= getBestFit(alignedSize, head, tail);
        if (bestFit == NULL){
            //No available Node and malloc new space
            Node * newNode = creatMallocBlock(alignedSize, flag);
            if (newNode == NULL){
                if (flag == true){
                    pthread_mutex_unlock(&lock);
                }
                return NULL;
            }
            if (flag == true){
                pthread_mutex_unlock(&lock);
            }
            return newNode->data;
        }
            //replace available Node with size passed in
            replaceBlock(bestFit, alignedSize, head, tail);
            if (flag == true){
                pthread_mutex_unlock(&lock);
            }
            return bestFit->data;
    }
}

//BF strategy wrapper for free
void bf_free(void * ptr, int flag, Node ** head, Node ** tail){
    if (flag == true){
        pthread_mutex_lock(&lock);
    }
    Node * thisBlock = pointToBlock(ptr);
    insertNode(thisBlock, head, tail);
    mergeNode(thisBlock, head, tail);
    if (flag == true){
        pthread_mutex_unlock(&lock);
    }
}

//thread safe malloc use lock
void * ts_malloc_lock(size_t size){
    Node ** head = &globalHead;
    Node ** tail = &globalTail;
   return bf_malloc(size, true, head, tail);
}

//thread safe free use lock
void ts_free_lock(void * ptr){
    Node ** head = &globalHead;
    Node ** tail = &globalTail;
    bf_free(ptr, true, head, tail);
}

//thread safe malloc nolock version. Only use lock for sbrk
void * ts_malloc_nolock(size_t size){
    Node ** head = &threadHead;
    Node ** tail = &threadTail;
   return bf_malloc(size, false, head, tail);
}

//thread safe free nolock version. Only use lock for sbrk
void ts_free_nolock(void * ptr){
    Node ** head = &threadHead;
    Node ** tail = &threadTail;
    bf_free(ptr, false, head, tail);
}

