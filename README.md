# Memory-Allocator


# Summary 

This memory allocator is a basic implementation of dynamic memory management system, providing functionalities similar to malloc, free, calloc, and realloc. The allocator is designed to work with both small and large memory allocations, utilizing different strategies based on the size of the requested memory.

# Usage 

To use the memory allocator, include the "osmem.h" header file in your C program and link it with the corresponding source file. The provided functions (os_malloc, os_free, os_calloc, os_realloc) can then be used for dynamic memory management.

# Functions 


1. void *os_malloc(size_t size)
Allocates a block of memory of the specified size. The implementation uses different strategies based on the size of the requested memory.

2. void os_free(void *ptr)
Frees the memory block pointed to by the given pointer. The implementation handles both mapped and allocated memory blocks.

3. void *os_calloc(size_t nmemb, size_t size)
Allocates memory for an array of elements, each initialized to zero. Similar to os_malloc but initializes the allocated memory to zero.

4. void *os_realloc(void *ptr, size_t size)
Changes the size of the memory block pointed to by the given pointer. The implementation handles various scenarios, including reallocating, splitting, and expanding memory.

# Memory Allocation Strategies
    
    Small Memory Allocation (< CHUNK size):
Uses sbrk to allocate memory.
Preallocates a chunk of memory on the first call.
Finds a fit in existing free blocks or expands the memory.
Large Memory Allocation (>= CHUNK size):
Uses mmap to allocate memory.
Memory Coalescing:
Combines adjacent free memory blocks to prevent fragmentation.
Block Splitting:
Splits a large memory block into two, with one part allocated and the other part marked as free.


