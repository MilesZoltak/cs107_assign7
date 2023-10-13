#include "allocator.h"
#include "debug_break.h"

/*    Miles Zoltak
      CS 107 11/21/19
      This is my implementation of an implicit free list allocator.
      It will be used as a not-terribly-robust heap allocator that will service
      malloc(), realloc(), and free() requests to a block of heap memory.  In 
      addition, it also does some overhead heap-maintenance such as validating
      the heap as it stands to make sure nothing terribly has gone awry with the
      structure/contents of the heap.  *a not one mymalloc(), it uses first-fit
      searching to find a fitting block of memory
*/

#include <string.h>


//heap header represents a block header for heap memory
typedef struct {
    size_t size: 63;
    size_t free: 1;
} header;

//just a few global variables (boo!) to get things going
static void *seg_start;
static void *seg_end;
static size_t seg_size;
static size_t bytes_used;
//for debugging/validate_heap
static size_t free_count;
static size_t used_count;
static size_t total_count;

//rounds a given size up to the nearest multiple of mult
size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

//this function adds a header at the END of a given allocated block
void add_header(header *h, size_t new_size, size_t old_size) {
    //if there's enough room for a block AND a payload before the next existing block, add header
    if (old_size >= new_size + sizeof(header)) {
        //the "+ 1" increments by one header's width
        //new_size / sizeof(header) allows that expression to be the number of BYTES, not "units"
        header *new_h = h + 1 + (new_size / sizeof(header));
        
        bytes_used += sizeof(header);
        *new_h = (header){old_size - (new_size + sizeof(header)), 1};

        free_count++;
        total_count++;
    }
}

//initialize the heap to start and have a specified size, as long as a header to identify it
bool myinit(void *heap_start, size_t heap_size) {
    //initialize global counts
    free_count = 0;
    used_count = 0;
    total_count = 0;
    bytes_used = 0;
    
    //initialize that big ol' block o' memory...
    seg_start = heap_start;
    seg_size = heap_size;
    seg_end = (char *)seg_start + seg_size;

    //...BUT ALSO GIVE IT A HEADER
    header *h = heap_start;
    bytes_used += sizeof(header);
    h->size = seg_size - bytes_used;
    h->free = 1;
    
    total_count++;
    free_count ++;
    
    return true;
}

void *mymalloc(size_t requested_size) {
    //we round for alignment reasons, and make space for the header
    size_t needed = roundup(requested_size, ALIGNMENT);

    //make sure we have enough space before we get into the nitty gritty
    //OR if size == 0, just return NULL
    if (needed + bytes_used > seg_size || requested_size == 0) {
        return NULL;
    }

    //there are a couple of factors that go into a malloc
    /* 1. begin searching the heap block by block for a free one that is big enough
       2. when you find one, cap it off with a header at the end for the block that follows ours
       3. then return the pointer to the block (NOT THE HEADER)
    */
    header *h = seg_start;
    //THIS ISN'T A HEADER SO DON'T DEREFERENCE
    header *end = seg_end;
    while (h < end) {
        if (h->size >= needed && h->free) {
            add_header(h, needed, h->size);
            bytes_used += needed;
            *h = (header){needed, 0};
            //advance the pointer past the header to the actual data
            h += 1;

            used_count++;
            free_count--;
            return (void *)h;
        }
        h += (h->size + sizeof(header)) / sizeof(header);
    }

    //if we get to the end of the heap without finding a block that fits, we return NULL :/
    return NULL;
}

void myfree(void *ptr) {
    //we can ignore null ptrs
    if (ptr != NULL) {
        header *h = (header *)ptr - 1;
        h->free = 1;
        bytes_used -= h->size;

        free_count++;
        used_count--;
    }
}

void *myrealloc(void *old_ptr, size_t new_size) {
    if (old_ptr != NULL && new_size == 0) {
        myfree(old_ptr);
        return NULL;
    }
    
    void *new_ptr = mymalloc(new_size);
    if (old_ptr != NULL) {
        header *h = (header *)old_ptr - 1;
        int bytes = (h->size < new_size) ? h->size : new_size;
        memcpy(new_ptr, old_ptr, bytes);
        myfree(old_ptr);
    }
    return new_ptr;
}

bool validate_heap() {
    /*traverse heap to check:
      1. if blocks are aligned on 8 byte values
      2. if we have a proper number of free and allocated blocks...
    */
    header *h = seg_start;
    size_t free = 0;
    size_t used = 0;
    size_t total = 0;
    while (h < (header *)seg_start + (seg_size / sizeof(header))) {
        //check alignment
        if ((size_t)h & (ALIGNMENT - 1)) {
            breakpoint();
            return false;
        }
        //check freeness (and increment total_count
        (h->free) ? free++ : used++;
        total++;

        //set h to next header
        h = h + 1 + (h->size / sizeof(header));
    }

    //if we count up a different amount of specific blocks than we tracked during execution
    if (free != free_count || used != used_count || total != total_count) {
        breakpoint();
        return false;
    }

    //if things literally aren't adding up
    if (free + used != total || free_count + used_count != total_count) {
        breakpoint();
        return false;
    }

    //MORE CHALLENGES???

    

    return true;
}
