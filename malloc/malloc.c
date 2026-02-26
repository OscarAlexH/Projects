#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
//addes this 
#include <limits.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)     ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr) - 1)

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes     */
   struct _block *next;  /* Pointer to the next _block of allocated memory      */
   bool   free;          /* Is this _block free?                                */
   char   padding[3];    /* Padding: IENTRTMzMjAgU3jMDEAD                       */
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;

#if defined FIT && FIT == 0
   /* First fit */
   //
   // While we haven't run off the end of the linked list and
   // while the current node we point to isn't free or isn't big enough
   // then continue to iterate over the list.  This loop ends either
   // with curr pointing to NULL, meaning we've run to the end of the list
   // without finding a node or it ends pointing to a free node that has enough
   // space for the request.
   // 
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

// \TODO Put your Best Fit code in this #ifdef block
   /** \TODO Implement best fit here */
   //  here I am adding the bets fit code
   #if defined BEST && BEST == 0
{
    // will look for the smallest space 
    int winner_leftover = INT_MAX;
    struct _block *winner = NULL;
    struct _block *prev = NULL;

    // will look thorugh whole list to see if block is large enough and the leftover
    while (curr)
    {
        if (curr->free && curr->size >= size)
        {
            int leftover = (int)(curr->size - size);

            if (leftover < winner_leftover)
            {
                winner_leftover = leftover;
                winner = curr;
            }
        }

        prev = curr;
        curr = curr->next;
    }

    // pointer to the end 
    if (last)
    {
        *last = prev;   
    }

    return winner;
}
#endif




// \TODO Put your Worst Fit code in this #ifdef block
   /** \TODO Implement worst fit here */
   //  this is for the worst fit
   // and it really is just the same as before
   #if defined WORST && WORST == 0
{
    // looks thorugh whole list to find the largest space
    int winner_leftover = -1;
    struct _block *winner = NULL;
    struct _block *prev = NULL;

    // looks trough list if block large enough and finds largest space
    while (curr)
    {
        if (curr->free && curr->size >= size)
        {
            int leftover = (int)(curr->size - size);

            if (leftover > winner_leftover)
            {
                winner_leftover = leftover;
                winner = curr;
            }
        }

        prev = curr;
        curr = curr->next;
    }

    if (last)
    {
        *last = prev;
    }

    return winner;
}
#endif




// \TODO Put your Next Fit code in this #ifdef block
   /** \TODO Implement next fit here */
   //  this is for next fit code 

   #if defined NEXT && NEXT == 0
{
    // uses last allocation space 
    static struct _block *last_alloc = NULL;
    
    // start from last allocation, or beginning if none
    struct _block *start = (last_alloc && last_alloc->next) ? last_alloc->next : heapList;
    curr = start;

    // search from start point to end
    do
    {
        // if its free and large enough updates allocation pointer 
        if (curr && curr->free && curr->size >= size)
        {
            last_alloc = curr;
            if (last) {
                *last = curr;
            }
            return curr;
        }
        
        if (last && curr) {
            *last = curr;
        }
        
        if (curr) {
            curr = curr->next;
        }
        
        // wrap around to beginning
        if (!curr && start != heapList) {
            curr = heapList;
        }
        
    } while (curr && curr != start);

    return NULL;
}
#endif




   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to previous _block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update _block metadata:
      Set the size of the new block and initialize the new block to "free".
      Set its next pointer to NULL since it's now the tail of the linked list.
   */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{
    if (atexit_registered == 0)
    {
        atexit_registered = 1;
        atexit(printStatistics);
    }

    /* Align to 4 bytes */
    size = ALIGN4(size);

    if (size == 0)
    {
        return NULL;
    }

    // update values
    num_mallocs++;
    num_requested += size;

    // Look for free _block.  If a free block isn't found then we need to grow our heap.

    struct _block *last = heapList;
    struct _block *next = findFreeBlock(&last, size);

    /* TODO: If the block found by findFreeBlock is larger than we need then:
            If the leftover space in the new block is greater than the sizeof(_block)+4 then
            split the block.
            If the leftover space in the new block is less than the sizeof(_block)+4 then
            don't split the block.
   */

   //  this is the code that I am going to add for this section 
   // This is when I will be splitting inside of malloc
   //after findfreeblock and before returning

   /* Could not find free _block, so grow heap */

   //resues and grows heap
    if (next != NULL)
    {
        num_reuses++;
    }
    else
    {
        next = growHeap(last, size);

        if (next != NULL)
        {
            num_grows++;
            num_blocks++;

            max_heap += (int)(sizeof(struct _block) + size);
        }
        else
        {
            return NULL;
        }
    }

    // will check for left over space and if can split if its large enough 
    //and initializes new block and free block with that space and updates
    if (next->free && next->size > size)
    {
        size_t leftover = next->size - size - sizeof(struct _block);

        if (leftover >= sizeof(struct _block) + 4)
        {
            struct _block *newBlock =
                (struct _block *)((char *)next + sizeof(struct _block) + size);

            newBlock->size = leftover;
            newBlock->free = true;
            newBlock->next = next->next;

            next->size = size;
            next->next = newBlock;

            num_splits++;
            num_blocks++;
        }
    }

    next->free = false;

    return BLOCK_DATA(next);
}


/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
    if (ptr == NULL)
    {
        return;
    }

    num_frees++;

    /* Make _block as free */
    struct _block *curr = BLOCK_HEADER(ptr);
    assert(curr->free == 0);
    curr->free = true;

   /* TODO: Coalesce free _blocks.  If the next block or previous block 
            are free then combine them with this block being freed.
   */

   //  so this is me trying to do what it asked for me to do
   // this is for the next block code
   if (curr->next && curr->next->free)
    {
        curr->size += sizeof(struct _block) + curr->next->size;
        curr->next = curr->next->next;
        num_coalesces++;
        num_blocks--;
    }

    //  this is to Find previous block
    struct _block *prev = heapList;

    while (prev && prev->next != curr)
    {
        prev = prev->next;
    }

    // if free it merges and removes from list and updates
    if (prev && prev->free)
    {
        prev->size += sizeof(struct _block) + curr->size;
        prev->next = curr->next;
        curr = prev;   
        num_coalesces++;
        num_blocks--;
    }
}

void *calloc( size_t nmemb, size_t size )
{
   // \TODO Implement calloc
   //  here I will be adding calloc
   // sees what bytes is needed and allocates it
   size_t total = nmemb * size;

    void *ptr = malloc(total);
    if (!ptr)
    {
        return NULL;
    }

    memset(ptr, 0, total);
    return ptr;
}

void *realloc( void *ptr, size_t size )
{
   // \TODO Implement realloc
   // going to add the realloc code
   //it can act like makkloc or free
   
    if (ptr == NULL)
    {
        return malloc(size);
    }

    
    if (size == 0)
    {
        free(ptr);
        return NULL;
    }

    size = ALIGN4(size);

    struct _block *old = BLOCK_HEADER(ptr);

    // if new size fits no need to reallocate
    if (old->size >= size)
    {
        return ptr;
    }

    
    // if block to small it will need to allocated larger block it will free the old block
    void *newptr = malloc(size);
    if (!newptr)
    {
        return NULL;
    }

    free(ptr);

    return newptr;
}



/* vim: IENTRTMzMjAgU3ByaW5nIDIwMOO2= ----------------------------------------*/
/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
