#include "allocator.h"
#include "debug_break.h"

/*    Miles Zoltak
      CS 107 11/26/19
      This is my implementation of an explicit free list allocator.
      It will be pretty robust I'm hoping, and will service malloc(),
      realloc(), and free() requests to heap memory.  It will also 
      support in-place realloc and will only search through free blocks
      when allocating memory using a doubly-linked list.
      
      HERE ARE JUST SOME OF MY SILLY THOUGHTS, NOT CRUCIAL BUT NOT WORTHLESS
      Ideally, if I have the time, skill, and strength, I will be creating
      that double linked list in such a way that I can use a binary search
      to search for blocks to further increase the speed of the program.
      I would also use a binary insert to add blocks.  I would hope that this
      wouldn't be so much overhead and maintenance that it would defeat the 
      purpose, but I suppose I'll cross that bridge when I come to it.
      
      Again, additionally, this program also has a validate_heap() function to 
      verify that all things are in order as we go along.
*/

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#define MIN_SIZE 16


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
//for debugging/validate_heap()
static size_t free_count;
static size_t used_count;
static size_t total_count;

//more static variables that are necessary for the explicit stuff i have going on here
static header *free_head;
static header *free_tail;

//rounds a given size up to the nearest multiple or mult, or 16 at the very minimum
size_t roundup(size_t sz, size_t mult) {
    if (sz <= MIN_SIZE) {
        return MIN_SIZE;
    } else {
        return (sz + mult - 1) & ~(mult - 1);
    }
}

//this function appends a free block to the free list, however global variables r updated after
void append_free(header *h) {
    //there are zero free blocks, so free_head is NULL
    if (free_count == 0) {
        free_head = h;

        //make h point back to null, it will be made to point forward to null later
        *((header **)h + 1) = NULL;
    } else {
        //make tail point forward to h
        *((header **)free_tail + 2) = h;

        //make h point back to the former free_tail
        *((header **)h + 1) = free_tail;
    }

    //no matter what, h is gonna point forward to NULL and h is the new free_tail
    *((header **)h + 2) = NULL;
    free_tail = h;
}


//this function removes a free block from the free list, global variables are updated after
void remove_free(header *h) {
    //NOTE: prev and/or next could be NULL if h is free_head or free_tail
    //ALSO: if this ever gets called when free_count == 0 you're gonna be in big trouble
    if (free_count == 0) {
        breakpoint();
    }
    
    header *prev = *((header **)h + 1);
    header *next = *((header **)h + 2);

    if (prev != NULL) {
        //make prev point forward to next
        *((header **)prev + 2) = next;
    }

    if (next != NULL) {
        //make next point back to forward
        *((header **)next + 1) = prev;
    }

    if (h == free_head) {
        free_head = next;
    }

    if (h == free_tail) {
        free_tail = prev;
    }
    
}

//merges all continuous free blocks into one megablock starting at h
size_t many_merge(header *h) {
    header *neighbor = h + 1 + (h->size / sizeof(header));

    //short circuit evaluation will keep us from segfaulting!!!, i think
    if (neighbor < (header *)seg_end && neighbor->free) {
        h->size += sizeof(header) + many_merge(neighbor);
        remove_free(neighbor);
        bytes_used -= sizeof(header);
        free_count--;
        total_count--;
        return h->size;
    } else {
        return h->size;
    }
    
}

//this function adds a header at the end of a given allocated block (if there's room)
size_t add_header(header *block_start, size_t new_size, size_t old_size) {
    //if we have room for a new block, add and format it accordingly
    if (old_size >= new_size + sizeof(header) + MIN_SIZE) {
        header *new_h = block_start + 1 + (new_size /sizeof(header));

        bytes_used += sizeof(header);
        *new_h = (header){old_size - (new_size + sizeof(header)), 1};
        
        append_free(new_h);
        
        //update globals
        free_count++;
        total_count++;

        //merge block following new_h with neighbor if possible
        //merge_blocks(new_h);
        many_merge(new_h);

        //if we CAN add a new header, h will only be new_size bytes long
        return new_size;
    }

    //if we can't add a new header, h is just gonna be the same old size it was
    return old_size;
}

//intialize the heap by setting all the out-of-the-box details like sizes, headers, pointers, etc.
bool myinit(void *heap_start, size_t heap_size) {
    //initialize global counts
    free_count = 0;
    used_count = 0;
    total_count = 0;
    bytes_used = 0;
    
    //simple heap initialization stuff
    seg_start = heap_start;
    seg_size = heap_size;
    seg_end = (char *)seg_start + seg_size;

    //add a header
    header *h = heap_start;
    bytes_used += sizeof(header);
    h->size = seg_size - bytes_used;
    h->free = 1;

    total_count++;
    free_count++;
    
    //initialize the doubly-linked list of free blocks
    free_head = h;
    free_tail = free_head;

    //ptrs to other free blocks are null, represented by size 0 block which isn't possible
    *((header **)h + 1) = NULL;
    *((header **)h + 2) = NULL;

    return true;
}

void *mymalloc(size_t requested_size) {
    //round for alignment reasons (also header and free block reasons)
    size_t needed = roundup(requested_size, ALIGNMENT);

    //make sure we have enough space before getting into the nitty gritty
    //and also check if they're asking for actual space
    if (needed + bytes_used > seg_size || requested_size == 0) {
        return NULL;
    }

    /*search across only the free blocks*/
    header *h = free_head;

    for (size_t i = 0; i < free_count; i++) {
        if (h->size >= needed) {
            //remove the block from the free list
            remove_free(h);
            free_count--;

            //allocate the block
            size_t h_size = add_header(h, needed, h->size);
            bytes_used += h_size;
            *h = (header){h_size, 0};
            used_count++;
            
            //advance h to the payload and return it
            h++;
            return (void *)h;
        }
        //get update h to be the value it is pointing forward to
        h = *((header **)h + 2);
    }

    //if we never found anything :/
    return NULL;
}

void myfree(void *ptr) {
    //do stuff if we have a legit ptr
    if (ptr != NULL) {
        header *h = (header *)ptr - 1;
        bytes_used -= h->size;
        //merge_blocks(h);
        many_merge(h);
        h->free = 1;
        append_free(h);
        
        free_count++;
        used_count--;
    }
}

void *myrealloc(void *old_ptr, size_t new_size) {
    //DONT FORGET TO DEAL WITH EDGE CASES
    if (old_ptr != NULL && new_size == 0) {
        myfree(old_ptr);
        return NULL;
    }


    //if old_ptr is null, they really just want a malloc
    if (old_ptr == NULL) {
        old_ptr = mymalloc(new_size);
    } else {
        header *h = (header *)old_ptr - 1;
        size_t needed = roundup(new_size, ALIGNMENT);
    
        /*this works in 2 cases
          CASE 1: we are shrinking the payload
          CASE 2: we are growing the payload, but there was enough padding to accommodate
          whatever the case is here, we can just return the same pointer
        */
        if (h->size >= needed) {
            size_t h_size = add_header(h, needed, h->size);
            bytes_used -= (h->size - h_size);
            h->size = h_size;
            return old_ptr;
        }

        //another case where we can still return the same ptr, but it's a little more complicated
        size_t old_size = h->size;
        size_t potential_size = many_merge(h);
        
        if (needed <= potential_size/* && neighbor->free*/) {
            //bytes_used += needed - old_size;
            //extend h to take up its full potential, engulfing the dividing header

            //replace the middle header where it should be
            size_t h_size = add_header(h, needed, potential_size);
            bytes_used -= (old_size - h_size);
            //resize h
            *h = (header){h_size, 0};
            return old_ptr;
        }

        //we can't do in place realloc, so they need a new ptr
        void *new_ptr = mymalloc(new_size);
        size_t bytes = (old_size < new_size) ? old_size : new_size;
        memcpy(new_ptr, old_ptr, bytes);
        myfree(old_ptr);
        /*this is a shoddy patch but it should work, here is some explanation:
          if we try to do in place realloc and we got rid of some adjacent free blocks, we alter
          the header of old_ptr.
          When myfree frees it, it updates the bytes_used by potential_size bytes instead of the 
          old_size.
          So I'm going to add back in that difference.  It will either come to 0 if it wasn't gonna
          be a problem in the first place, or it'll come out to the difference and fix the problem
          so yeah no the best but hey it should work right?
         */
        bytes_used += (potential_size - old_size);
        old_ptr = new_ptr;
    }

    return old_ptr;
}


/*this function is called periodically by testers to check that everything internally is in order*/
bool validate_heap() {
    /*traverse heap to check:
      1. if blocks are aligned on 8 bytes values
      2. no block is smaller than 16 bytes
      3. if we have a proper number of free and allocated blocks
      4. free + used == total (both globally and locally)
    */
    header *h = seg_start;
    size_t free = 0;
    size_t used = 0;
    size_t total = 0;
    size_t used_bytes = 0;

    while (h < (header *)seg_end) {
        //check alignment
        if ((size_t)h & (ALIGNMENT - 1)) {
            breakpoint();
            return false;
        }
        
        //check size
        if (h->size < 16) {
            breakpoint();
            return false;
        }
        //check freeness and increment total
        (h->free) ? free++ : used++;
        total++;

        //add the amount of size of a header (and payload if it is in use)
        used_bytes += sizeof(header);
        if (h->free == 0) {
            used_bytes += h->size;
        }
        
        //set h to next header
        h = h + 1 + h->size / sizeof(header);
    }

    //if local and global block counts don't agree
    if (free != free_count || used != used_count || total != total_count) {
        breakpoint();
        return false;
    }

    /*if things literally aren't adding up
      (we only have 2 check locals OR globals bc they're guaranteed to be the same bc of the above*/
    if (free + used != total) {
        breakpoint();
        return false;
    }

    //if we are tracking the right amount of bytes used
    if (used_bytes != bytes_used) {
        breakpoint();
        return false;
    }
    
    return true;
}
