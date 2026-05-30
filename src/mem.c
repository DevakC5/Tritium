#include "mem.h"
#include <stdlib.h>

Memory *mem_create(size_t size) {
    Memory *mem = (Memory *)calloc(1, sizeof(Memory));
    if (!mem) return NULL;
    mem->cells = (Tryte *)calloc(size, sizeof(Tryte));
    if (!mem->cells) { free(mem); return NULL; }
    mem->size = size;
    return mem;
}

void mem_free(Memory *mem) {
    if (mem) {
        free(mem->cells);
        free(mem);
    }
}

Tryte mem_read(Memory *mem, size_t addr) {
    if (addr >= mem->size) { Tryte z; tryte_zero(&z); return z; }
    return mem->cells[addr];
}

void mem_write(Memory *mem, size_t addr, Tryte val) {
    if (addr < mem->size)
        mem->cells[addr] = val;
}

void mem_load(Memory *mem, size_t addr, const Tryte *data, size_t count) {
    for (size_t i = 0; i < count && addr + i < mem->size; i++)
        mem->cells[addr + i] = data[i];
}

void mem_dump(Memory *mem, size_t start, size_t end, FILE *out) {
    if (end > mem->size) end = mem->size;
    char buf[16];
    for (size_t i = start; i < end; i++) {
        tryte_to_str(mem->cells[i], buf, sizeof(buf));
        fprintf(out, "[%04zx] %s (%5ld)\n", i, buf,
                (long)tryte_to_int64(mem->cells[i]));
    }
}

int mem_valid_addr(Memory *mem, size_t addr) {
    return addr < mem->size;
}
