#include <roaring/memory.h>
#include <roaring/memorypool.h>
#include <stdlib.h>
#include <stdio.h>
// without the following, we get lots of warnings about posix_memalign
#ifndef __cplusplus
extern int posix_memalign(void **__memptr, size_t __alignment, size_t __size);
#endif  //__cplusplus // C++ does not have a well defined signature

//#define MALLOC_LOG

static int memory_pool_inited = 0;
static MemoryPool *mp = NULL;
static MemoryPool *get_memory_pool() {
    if (!memory_pool_inited) {
        mem_size_t max_mem = 32*GB;
        mem_size_t mem_pool_size = 512*MB;
        mp = MemoryPoolInit(max_mem, mem_pool_size);
        memory_pool_inited = 1;
    }
    return mp;
}


// portable version of  posix_memalign
static void *roaring_bitmap_aligned_malloc(size_t alignment, size_t size) {
    return MemoryPoolAlloc(get_memory_pool(), size);
//    return malloc(size);
    void *p;

#ifdef _MSC_VER
    p = _aligned_malloc(size, alignment);
#elif defined(__MINGW32__) || defined(__MINGW64__)
    p = __mingw_aligned_malloc(size, alignment);
#else
    // somehow, if this is used before including "x86intrin.h", it creates an
    // implicit defined warning.
    if (posix_memalign(&p, alignment, size) != 0) return NULL;
#endif
    return p;
}

static void roaring_bitmap_aligned_free(void *memblock) {
    MemoryPoolFree(get_memory_pool(), memblock);
    return;
//    free(memblock);
//    return;
#ifdef _MSC_VER
    _aligned_free(memblock);
#elif defined(__MINGW32__) || defined(__MINGW64__)
    __mingw_aligned_free(memblock);
#else
    free(memblock);
#endif
}

static roaring_memory_t global_memory_hook = {
    .malloc = malloc,
    .realloc = realloc,
    .calloc = calloc,
    .free = free,
    .aligned_malloc = roaring_bitmap_aligned_malloc,
    .aligned_free = roaring_bitmap_aligned_free,
};

void roaring_init_memory_hook(roaring_memory_t memory_hook) {
    global_memory_hook = memory_hook;
}

void* roaring_malloc(size_t n) {
#ifdef MALLOC_LOG
    printf("roaring_malloc(%zu)\n", n);
#endif
    return global_memory_hook.malloc(n);
}

void* roaring_realloc(void* p, size_t new_sz) {
#ifdef MALLOC_LOG
    printf("roaring_realloc(%zu)\n", new_sz);
#endif
    return global_memory_hook.realloc(p, new_sz);
}

void* roaring_calloc(size_t n_elements, size_t element_size) {
#ifdef MALLOC_LOG
    printf("roaring_calloc(%zu)\n", n_elements * element_size);
#endif
    return global_memory_hook.calloc(n_elements, element_size);
}

void roaring_free(void* p) {
    global_memory_hook.free(p);
}

void* roaring_aligned_malloc(size_t alignment, size_t size) {
#ifdef MALLOC_LOG
    printf("roaring_aligned_malloc(%zu)(%zu)\n", size, alignment);
#endif
    return global_memory_hook.aligned_malloc(alignment, size);
}

void roaring_aligned_free(void* p) {
    global_memory_hook.aligned_free(p);
}
