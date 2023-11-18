// SPDX-License-Identifier: BSD-3-Clause

#include "osmem.h"
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

int first_time;

static size_t CHUNK = 131072;

static struct block_meta *heap_start;

struct block_meta *get_last_block(void)
{
		struct block_meta *last = heap_start;

		while (last->next)
			last = last->next;


	return last;
}

void init_block_data(struct block_meta *block, size_t size, int status,
struct block_meta *prev, struct block_meta *next)
{
			block->size = size;
			block->status = status;
			block->prev = prev;
			block->next = next;
}

void *insert_to_end(struct block_meta *new_allocate, struct block_meta *last)
{
			last->next = new_allocate;
			if (new_allocate->prev)
				new_allocate->prev = last;
		return (void *)(new_allocate + 1);
}

struct block_meta *find_fit(size_t size)
{
	struct block_meta *header = heap_start;

	while (header) {
		if (header->status == STATUS_FREE && header->size >= size)
			return header;

		header = header->next;
	}

		return NULL;
}

void coalesce(void)
{
	struct block_meta *header = heap_start;

	if (!header)
		return;

	while (header)
	{
		if (header->status == STATUS_FREE) {
			if (header->next && header->next->status == STATUS_FREE) {
				header->size += header->next->size + STRUCTURE_SIZE;
				header->next = header->next->next;
				if (header->next)
					header->next->prev = header;
			}

			if (header->prev && header->prev->status == STATUS_FREE) {
				header->prev->size += header->size + STRUCTURE_SIZE;
				header->prev->next = header->next;
				if (header->next)
					header->next->prev = header->prev;
			}
		}
		header = header->next;
	}
}

void *preallocate(void)
{
	struct block_meta *preallocate = (struct block_meta *)sbrk(131072);

	init_block_data(preallocate, 131072 - STRUCTURE_SIZE, STATUS_ALLOC, NULL, NULL);

	heap_start = preallocate;

	return (void *)(preallocate + 1);
}

void *alloc_big_chunks(size_t size)
{
struct block_meta *big_allocate = mmap(NULL, ALIGN(size) + STRUCTURE_SIZE,
PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	init_block_data(big_allocate, ALIGN(size), STATUS_MAPPED, NULL, NULL);

		heap_start = big_allocate;
	return (void *)(big_allocate + 1);
}

void *expand_memory(size_t size, struct block_meta *last_block)
{
		struct block_meta *expend_mem = (struct block_meta *)sbrk(ALIGN(size) - last_block->size);

		expend_mem = last_block;
		init_block_data(expend_mem, ALIGN(size), STATUS_ALLOC, expend_mem->prev, expend_mem->next);

	return (void *)(expend_mem + 1);
}

void split_block(struct block_meta **block, size_t size)
{
	if ((*block)->size >= ALIGN(size) + STRUCTURE_SIZE + 8) {
		struct block_meta *new_block = (struct block_meta *)((void *)(*block) + ALIGN(size) + STRUCTURE_SIZE);

		// Initialize the new block data
		init_block_data(new_block, (*block)->size - ALIGN(size) - STRUCTURE_SIZE, STATUS_FREE, (*block), (*block)->next);
		// Update the current block data to the new payload
		init_block_data((*block), ALIGN(size), STATUS_ALLOC, (*block)->prev, new_block);

		if (new_block->next)
			new_block->next->prev = new_block;

		} else {
			(*block)->status = STATUS_ALLOC;
		}
}


void *os_malloc(size_t size)
{
	if (size <= 0)
		return NULL;


	// If the size is smaller than the chunk size, use sbrk
		if (ALIGN(size) + STRUCTURE_SIZE < CHUNK) {
			// If it is the first time, preallocate the memory
			if (first_time == 0) {
				first_time = 1;
				return preallocate();

			} else {
				// If it is not the first time, try to find an empty space to fit

				struct block_meta *empty_block = find_fit(ALIGN(size));

				// If there is no empty space, expend the memory if the last block is free
				if (!empty_block) {
				struct block_meta *last = get_last_block();

				if (last->status == STATUS_FREE)
					return expand_memory(size, last);


				// If there is no empty space, allocate new memory
				struct block_meta *new_allocate = (struct block_meta *)sbrk(ALIGN(size) + STRUCTURE_SIZE);

				init_block_data(new_allocate, ALIGN(size), STATUS_ALLOC, last, NULL);

				// Add the new block to the end of the list
				return insert_to_end(new_allocate, last);

			} else {
				// If there is an empty space, check if it can be split
				split_block(&empty_block, size);
				return (void *)(empty_block + 1);
			}
		}

	} else {
		// If the size is bigger than the chunk size, use mmap
		return alloc_big_chunks(size);
	}
}

void os_free(void *ptr)
{
	if (ptr == NULL)
		return;


	struct block_meta *block = (struct block_meta *)ptr - 1;

	if (block->status == STATUS_MAPPED) {
		block->status = STATUS_FREE;
		munmap(block, block->size + STRUCTURE_SIZE);

	} else if (block->status == STATUS_ALLOC) {
		block->status = STATUS_FREE;
		coalesce();
	}
}

void *os_calloc(size_t nmemb, size_t size)
{
if (nmemb <= 0 || size <= 0)
	return NULL;

    size_t total_size = nmemb * size;
	CHUNK = 4096;

	void *ptr = os_malloc(total_size);

	if (ptr != NULL)
		memset(ptr, 0, total_size);


	CHUNK = 131072;

	return ptr;
}

void *os_realloc(void *ptr, size_t size)
{
	if (ptr == NULL)
		return os_malloc(size);


	if (size <= 0)
	{
		os_free(ptr);
		return NULL;
	}


	struct block_meta *block = (struct block_meta *)ptr - 1;

	if (block->status == STATUS_FREE)
		return NULL;

	if (block->status == STATUS_MAPPED) {
		void* new_address = os_malloc(size);

		if (new_address == NULL)
		{
			DIE(new_address == NULL, "os_realloc");
			return NULL;
		}

		size_t size_to_copy = block->size < ALIGN(size) ? block->size : ALIGN(size);

		memcpy(new_address, ptr, size_to_copy);
		os_free(ptr);

		return new_address;
}

	// STATUS ALLOC

	// Split

	if (ALIGN(size) <= block->size)
	{
		coalesce();
		split_block(&block, size);

		return (void *)(ptr);
	}

	// Expand Memory

	if (block->next == NULL)
	{
        void *new_mem = expand_memory(size, block);

        if (new_mem == NULL)
		{
            DIE(new_mem == NULL, "os_realloc");
            return NULL;
        }

        return (void *)ptr;
    }


		 if (block->next->status == STATUS_FREE)
		{
			coalesce();
            return (void *)ptr;
    	}

	if (block->size < ALIGN(size))
	{
        void *new_address = os_malloc(size);

        if (new_address == NULL) {
            DIE(new_address == NULL, "os_realloc");
            return NULL;
        }

        memcpy(new_address, ptr, block->size);

        os_free(ptr);

        return (void *)new_address;
    }

	return (void *) ptr;
}
