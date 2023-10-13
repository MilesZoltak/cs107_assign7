#include "allocator.h"
#include "debug_break.h"

#include <stdbool.h>

/*    Miles Zoltak
      CS 107 11/21/19
      This is my implementation of an implicit free list allocator.
      It will be used as a not-terribly-robust heap allocator that will service
      malloc(), realloc(), and free() requests to a block of heap memory.  In 
      addition, it also does some overhead heap-maintenance such as validating
      the heap as it stands to make sure nothing terribly has gone awry with the
      structure/contents of the heap.
*/

//heap header represents a block header for heap memory
typedef struct header {
    size_t data;
} header;

//just a few global variables (boo!) to get things going
static void *seg_start;
static size_t seg_size;
static size_t bytes_used;

//rounds a given size up to the nearest multiple of mult
size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

//this function adds a header at the END of a given allocated block
void add_header(header *block_start, size_t new_size, size_t old_size) {
    //the "+ 1" increments by one header's width
    //new_size / sizeof(header) allows that expression to be the number of BYTES, not "units"
    header *new_h = block_start + 1 + (new_size / sizeof(header));

    //if there's enough room for a block AND a payload before the next existing block, add header
    if (old_size > new_size + sizeof(header)) {
        bytes_used += sizeof(header);
        *new_h = (header){old_size - (new_size + sizeof(header)), 1};
    }
}

//initialize the heap to start and have a specified size, as long as a header to identify it
bool myinit(void *heap_start, size_t heap_size) {
    //initialize that big ol' block o' memory...
    seg_start = heap_start;
    seg_size = heap_size;

    //...BUT ALSO GIVE IT A HEADER
    header *h = heap_start;
    bytes_used += 8;
    h->size = seg_size - bytes_used;
    h->free = 1;
    
    return true;
}

void *mymalloc(size_t requested_size) {
    //we round for alignment reasons, and make space for the header
    size_t needed = roundup(requested_size, ALIGNMENT);

    //make sure we have enough space before we get into the nitty gritty
    if (needed + bytes_used > seg_size) {
        return NULL;
    }

    //there are a couple of factors that go into a malloc
    /* 1. begin searching the heap block by block for a free one that is big enough
       2. when you find one, cap it off with a header at the end for the block that follows ours
       3. then return the pointer to the block (NOT THE HEADER)
    */
    header *h = seg_start;
    //THIS ISN'T A HEADER SO DON'T DEREFERENCE BUT IT MAKES THE COMPILER HAPPY FOR THE COMPARISON
    header *end = (header *)seg_start + (seg_size / sizeof(header));
    while (h < end) {
        if (h->size >= needed && h->free) {
            add_header(h, needed, h->size);
            bytes_used += needed;
            *h = (header){needed, 0};
            return h + 1;
        }
    }

    //if we get to the end of the heap without finding a block that fits, we return NULL :/
    return NULL;
}

void myfree(void *ptr) {
    header *h = (header *)ptr - 1;
    h->free = 1;
}

void *myrealloc(void *old_ptr, size_t new_size) {
    return NULL;
}

bool validate_heap() {
    /* TODO: remove the line below and implement this to 
     * check your internal structures!
     * Return true if all is ok, or false otherwise.
     * This function is called periodically by the test
     * harness to check the state of the heap allocator.
     * You can also use the breakpoint() function to stop
     * in the debugger - e.g. if (something_is_wrong) breakpoint();
     */
    //traverse heap to see if blocks are properly aligned
    header *h = seg_start;
    while (h < (header *)seg_start + (seg_size / sizeof(header))) {
        if ((size_t)h & (ALIGNMENT - 1)) {
            return false;
        }
        h = h + 1 + (h->size / sizeof(header));
    }
    return true;
}
