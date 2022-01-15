#include <string.h>
#include <unistd.h>
#include "malloc_2.h"
#define MAX_SIZE 100000000
#define HIST_MAX 128
#define MIN_GAP 128

typedef struct MallocMetaData {
    size_t size;
    bool is_free;
    MallocMetaData* next;
    MallocMetaData* prev;
    MallocMetaData* next_hist;
    MallocMetaData* prev_hist;
}MMD;

class BlockList{
private:
    MMD* block_list;
    MMD** hist;
public:
    BlockList():block_list(NULL){
        hist[HIST_MAX] = {NULL};
    };
    void insertBlock(MMD *block);
    void removeBlockFromHist(MMD *block);
    void splitBlock(MMD* block, size_t size);
    void freeBlock(void *ptr);
    void* allocateBlock(size_t size);

    MMD* get_mmd(void* p);
    size_t numFreeBlocks();
    size_t numFreeBytes();
    size_t numTotalBlocks();
    size_t numTotalBytes();
};


MMD* BlockList::get_mmd(void* p){
    return (MMD*)((char*)p - sizeof(MMD));
}

void BlockList::removeBlockFromHist(MMD *block) {
    int hist_index = block->size/1000;

    // block is head
    if(block->prev_hist == NULL){
        hist[hist_index] = block->next_hist;
        block->next_hist = NULL;
        return;
    }
    // block is tail
    if(block->next_hist == NULL){
        block->prev_hist->next_hist = NULL;
        block->prev_hist = NULL;
        return;
    }
    // block in the middle
    block->prev_hist->next_hist = block->next_hist;
    block->next_hist->prev_hist = block->prev_hist;
    block->prev_hist = NULL;
    block->next_hist = NULL;
}

void* BlockList::allocateBlock(size_t size){

    int hist_index = size/1000;
    MMD* block_found = NULL;

    // Find block if available
    for(int i = hist_index; i <= HIST_MAX; i++){
        MMD *block = hist[i];
        while(block) {
            if (block->is_free && size <= block->size) {
                block_found = block;
                break;
            }
            block = block->next;
        }
        if(block_found)
            break;
    }

    if(block_found){
        removeBlockFromHist(block_found);
        if(size + MIN_GAP + sizeof(MMD) <= block_found->size) {
            splitBlock(block_found, size);
        }
        block_found->is_free = false;
        return block_found;

    } else {
        // allocate more space from heap
        size_t alloc_size = size + sizeof(MMD);
        void* program_break = sbrk(alloc_size);
        if(program_break == (void*)-1)
            return NULL;

        MMD* new_block = (MMD*)program_break;
        new_block->size = size;
        new_block->is_free = false;
        new_block->next = NULL;
        new_block->prev = NULL;
        new_block->next_hist = NULL;
        new_block->prev_hist = NULL;
        insertBlock(new_block);

        return program_break;
    }
}

void BlockList::splitBlock(MMD *block, size_t size) {
    MMD* new_block = block + size + sizeof(MMD);
    new_block->size = block->size - size - sizeof(MMD);
    new_block->is_free = true;

    // insert to block_list

    // try to merge

    // insert to histohram
}
void BlockList::insertBlock(MMD *block){
    MMD* tail = block_list, *prev = NULL;
    while(tail){
        prev = tail;
        tail = tail->next;
    }
    if(prev == NULL){
        block_list = block;
    } else {
        prev->next = block;
        block->prev = prev;
    }

}

void BlockList::freeBlock(void *ptr){
    MMD* block = get_mmd(ptr);
    block->is_free = true;
}

size_t BlockList::numFreeBlocks(){
    MMD *temp = block_list;
    size_t count = 0;
    while(temp){
        if(temp->is_free) count++;
        temp = temp->next;

    }
    return count;
}

size_t BlockList::numFreeBytes(){
    MMD* temp = block_list;
    size_t free_bytes = 0;
    while(temp){
        if(temp->is_free)
            free_bytes += temp->size;
        temp = temp->next;
    }
    return free_bytes;
}

size_t BlockList::numTotalBlocks(){
    MMD *temp = block_list;
    size_t count = 0;
    while(temp){
        count++;
        temp = temp->next;
    }
    return count;
}

size_t BlockList::numTotalBytes(){
    MMD* temp = block_list;
    size_t free_bytes = 0;
    while(temp){
        free_bytes += temp->size;
        temp = temp->next;
    }
    return free_bytes;
}


// ******** Main Functions **********

BlockList bl = BlockList();

void* smalloc(size_t size){
    if(size == 0 || size > MAX_SIZE)
        return NULL;

    void* allocated = bl.allocateBlock(size);
    if(allocated == NULL)
        return NULL;
    return (char*)allocated + sizeof(MMD);

}

void* scalloc(size_t num, size_t size){
    void* p = smalloc(num * size);
    if(p == NULL)
        return NULL;
    memset(p, 0, num*size);
    return p;
}

void sfree(void* p){
    if(p == NULL)
        return;
    bl.freeBlock(p);
}

void* srealloc(void* oldp, size_t size){
    if(size == 0 || size > MAX_SIZE)
        return NULL;

    if(oldp == NULL)
        return smalloc(size);

    MMD* old_block = bl.get_mmd(oldp);
    size_t old_size = old_block->size;
    if(size <= old_size)
        return oldp;

    void* newp = smalloc(size);
    if(newp == NULL)
        return NULL;
    memcpy(newp, oldp, old_size);
    sfree(oldp);
    return newp;
}

size_t _num_free_blocks(){
    return bl.numFreeBlocks();
}

size_t _num_free_bytes(){
    return bl.numFreeBytes();
}

size_t _num_allocated_blocks(){
    return bl.numTotalBlocks();
}

size_t _num_allocated_bytes(){
    return bl.numTotalBytes();
}

size_t _num_meta_data_bytes(){
    return bl.numTotalBlocks() * sizeof(MMD);
}

size_t _size_meta_data(){
    return sizeof(MMD);
}
