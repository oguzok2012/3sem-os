#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>

#define MAX_LEVELS 16

typedef struct Block {
    bool is_free;    
    struct Block* next; 
    size_t level;
} Block;


typedef struct Allocator {
    void* memory;       
    size_t total_size;        
    size_t min_block_size;
    Block* free_lists[MAX_LEVELS];
} Allocator;

size_t get_level(size_t size, size_t total_size, size_t min_block_size) {
    size_t level = 0;
    while (min_block_size * (1 << level) < size && total_size >= min_block_size * (1 << (level + 1))) {
        level++;
    }
    return level;
}

Allocator* allocator_create(void* const memory, const size_t size) {
    if (size < 0) return NULL;

    Allocator *allocator = (Allocator *)memory;
    allocator->memory = (unsigned char *)memory + sizeof(Allocator);

    allocator->total_size = size;
    allocator->min_block_size = 16;

    for (int i = 0; i < MAX_LEVELS; i++) {
        allocator->free_lists[i] = NULL;
    }

    Block* initial_block = (Block*)allocator->memory;
    initial_block->is_free = true;
    initial_block->next = NULL;
    allocator->free_lists[MAX_LEVELS - 1] = initial_block;

    return allocator;
}

void allocator_destroy(Allocator* const allocator) {
    if (!allocator) return;
    munmap(allocator->memory, allocator->total_size);
    munmap(allocator, sizeof(Allocator));
}

void split_block(Allocator* allocator, size_t level) {
    if (!allocator || level >= MAX_LEVELS || !allocator->free_lists[level]) return;

    Block* block = allocator->free_lists[level];
    allocator->free_lists[level] = block->next;

    size_t block_size = allocator->min_block_size * (1 << level);
    Block* buddy = (Block*)((char*)block + block_size / 2);

    block->is_free = true;
    buddy->is_free = true;
    buddy->next = allocator->free_lists[level - 1];
    allocator->free_lists[level - 1] = buddy;

    block->next = allocator->free_lists[level - 1];
    allocator->free_lists[level - 1] = block;
}

void* allocator_alloc(Allocator* const allocator, const size_t size) {
    if (!allocator || size == 0 || size > allocator->total_size) return NULL;

    size_t level = get_level(size, allocator->total_size, allocator->min_block_size);

    for (size_t i = level; i < MAX_LEVELS; i++) {
        if (allocator->free_lists[i]) {
            while (i > level) {
                split_block(allocator, i);
                i--;
            }

            Block* block = allocator->free_lists[level];
            allocator->free_lists[level] = block->next;
            block->is_free = false;
            return (void*)block;
        }
    }

    return NULL;
}

void merge_buddies(Allocator* allocator, Block* block) {
    if (!allocator || !block) return;

    while (block->level < MAX_LEVELS - 1) {
        size_t block_size = allocator->min_block_size * (1 << block->level);

        size_t block_offset = (char*)block - (char*)allocator->memory;

        size_t buddy_offset = block_offset ^ block_size;

        Block* buddy = (Block*)((char*)allocator->memory + buddy_offset);

        if ((char*)buddy < (char*)allocator->memory ||
            (char*)buddy >= (char*)allocator->memory + allocator->total_size) {
            break;
        }

        if (!buddy->is_free) {
            break;
        }

        Block** list = &allocator->free_lists[buddy->level];
        while (*list && *list != buddy) {
            list = &(*list)->next;
        }
        if (*list == buddy) {
            *list = buddy->next;
        }

        if ((char*)block > (char*)buddy) {
            block = buddy;
        }

        block->level++;
        block->is_free = true;
    }

    block->next = allocator->free_lists[block->level];
    allocator->free_lists[block->level] = block;
}

void allocator_free(Allocator* const allocator, void* const memory) {
    if (!allocator || !memory) return;

    if ((char*)memory < (char*)allocator->memory ||
        (char*)memory >= (char*)allocator->memory + allocator->total_size) {
        return;
    }

    Block* block = (Block*)memory;
    block->is_free = true;
    merge_buddies(allocator, block);
}
