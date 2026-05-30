#ifndef TRINARY_MEM_H
#define TRINARY_MEM_H

#include "trit.h"

#define ROM_SIZE    4096
#define RAM_SIZE    4096
#define STACK_SIZE  512
#define STACK_BASE  (ROM_SIZE + RAM_SIZE - STACK_SIZE)

typedef struct {
    Tryte *cells;
    size_t size;
} Memory;

Memory *mem_create(size_t size);
void mem_free(Memory *mem);
Tryte mem_read(Memory *mem, size_t addr);
void mem_write(Memory *mem, size_t addr, Tryte val);
void mem_load(Memory *mem, size_t addr, const Tryte *data, size_t count);
void mem_dump(Memory *mem, size_t start, size_t end, FILE *out);
int mem_valid_addr(Memory *mem, size_t addr);

#endif
