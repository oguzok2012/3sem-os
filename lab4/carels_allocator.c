#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define MIN_BLOCK_SIZE 16
#define MAX_BLOCK_SIZE 2048
#define NUM_PAGES 8  // по 16, 32, 64, .. , 2048

typedef struct FreeBlock {
    struct FreeBlock *next;
} FreeBlock;

typedef struct {
    unsigned char *memory;
    size_t total_size;
    FreeBlock *free_lists[NUM_PAGES];
} Allocator;


static size_t round_up_to_power_of_two(size_t size) {
    size_t rounded = MIN_BLOCK_SIZE;
    while (rounded < size && rounded <= MAX_BLOCK_SIZE) {
        rounded <<= 1;
    }
    return rounded;
}

static int get_size_index(size_t size) {
    size_t block_size = MIN_BLOCK_SIZE;
    int index = 0;
    while (block_size < size && block_size <= MAX_BLOCK_SIZE) {
        block_size <<= 1;
        index++;
    }
    return index;
}

Allocator *allocator_create(void *const memory, size_t size) {
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); // Round up to page size

    Allocator *allocator = (Allocator *)memory;
    allocator->memory = (unsigned char *)memory + sizeof(Allocator);
    allocator->total_size = size - sizeof(Allocator);
    memset(allocator->free_lists, 0, sizeof(allocator->free_lists));

    return allocator;
}

void *allocator_alloc(Allocator *allocator, size_t size) {
    if (size < 0 || size > MAX_BLOCK_SIZE) {
        return NULL;
    }

    size = round_up_to_power_of_two(size);
    int index = get_size_index(size);

    // если в странице есть свободные блоки, то убираем его из списка свободных и возвращаем его
    if (allocator->free_lists[index]) {
        FreeBlock *block = allocator->free_lists[index];
        allocator->free_lists[index] = block->next;
        return (void *)block;
    }

    // если свободных блоков нет, делаем новую страницу
    size_t blocks_per_page = PAGE_SIZE / size;
    unsigned char *new_page = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (new_page == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }

    // разбиваем новую страницу на блоки
    for (size_t i = 1; i < blocks_per_page; i++) {
        FreeBlock *block = (FreeBlock *)(new_page + i * size);
        block->next = allocator->free_lists[index];
        allocator->free_lists[index] = block;
    }

    return (void *)new_page;
}

void allocator_free(Allocator *allocator, void *memory) {
    unsigned char *block_start = (unsigned char *)memory;
    size_t offset = block_start - allocator->memory;

    // определение размера блока по его выравниванию
    size_t size = MIN_BLOCK_SIZE;
    int index = 0;
    while (index < NUM_PAGES && (offset % size != 0 || size > MAX_BLOCK_SIZE)) {
        size <<= 1;
        index++;
    }

    if (index >= NUM_PAGES || size > MAX_BLOCK_SIZE) {
        fprintf(stderr, "Error: the specified pointer does not match the selected block.\n");
        return;
    }

    FreeBlock *block = (FreeBlock *)memory;
    block->next = allocator->free_lists[index];
    allocator->free_lists[index] = block;
}

void allocator_destroy(Allocator *allocator) {
    if (!allocator) return;
    munmap(allocator->memory, allocator->total_size);
    munmap(allocator, sizeof(Allocator));
}

