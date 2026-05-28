#ifndef MEMORY_H
#define MEMORY_H

#include <libultra/types.h>
#include "types.h"
#include <string.h>

#define MEMORY_POOL_LEFT  0
#define MEMORY_POOL_RIGHT 1

struct AllocOnlyPool {
    s32 totalSpace;
    s32 usedSpace;
    u8 *startPtr;
    u8 *freePtr;
};

struct MemoryPool;

struct OffsetSizePair {
    u32 offset;
    u32 size;
};

struct DmaTable {
    u32 count;
    u8 *srcAddr;
    struct OffsetSizePair anim[1]; // dynamic size
};

struct DmaHandlerList {
    struct DmaTable *dmaTable;
    void *currentAddr;
    void *bufTarget;
};

#ifndef INCLUDED_FROM_MEMORY_C
// Declaring this variable extern_s puts it in the wrong place in the bss order
// when this file is included from memory.c (first instead of last). Hence,
// ifdef hack. It was very likely subject to bss reordering originally.
extern_s struct MemoryPool *gEffectsMemoryPool;
#endif

extern_s uintptr_t set_segment_base_addr(s32 segment, void *addr);
extern_s void *get_segment_base_addr(s32 segment);
extern_s void *segmented_to_virtual(const void *addr);
extern_s void *virtual_to_segmented(u32 segment, const void *addr);
extern_s void move_segment_table_to_dmem(void);

extern_s void main_pool_init(void *start, void *end);
extern_s void *main_pool_alloc(u32 size, u32 side);
extern_s u32 main_pool_free(void *addr);
extern_s void *main_pool_realloc(void *addr, u32 size);
extern_s u32 main_pool_available(void);
extern_s u32 main_pool_push_state(void);
extern_s u32 main_pool_pop_state(void);

#define load_segment(...)
#define load_to_fixed_pool_addr(...)
#define load_segment_decompress(...)
#define load_segment_decompress_heap(...)
#define load_engine_code_segment(...)

extern_s struct AllocOnlyPool *alloc_only_pool_init(u32 size, u32 side);
extern_s void *alloc_only_pool_alloc(struct AllocOnlyPool *pool, s32 size);
extern_s struct AllocOnlyPool *alloc_only_pool_resize(struct AllocOnlyPool *pool, u32 size);

extern_s struct MemoryPool *mem_pool_init(u32 size, u32 side);
extern_s void *mem_pool_alloc(struct MemoryPool *pool, u32 size);
extern_s void mem_pool_free(struct MemoryPool *pool, void *addr);

extern_s void *alloc_display_list(u32 size);
extern_s void setup_dma_table_list(struct DmaHandlerList *list, void *srcAddr, void *buffer);
extern_s s32 load_patchable_table(struct DmaHandlerList *list, s32 index);

#endif // MEMORY_H
