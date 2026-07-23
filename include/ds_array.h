#pragma once
#ifdef DS_ARRAY_IMPLEMENT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARR_MIN_CAP_BYTES 64  // 最小开辟 64 字节

#define A_GET_META_PTR(a)     ((int *)((char *)(a) - 2 * sizeof(int)))
#define A_GET_CAP(a)          (*(A_GET_META_PTR(a)))
#define A_GET_SIZE(a)         (*(A_GET_META_PTR(a) + 1))

#define array_size(a)        ((a) ? A_GET_SIZE(a) : 0)                 
#define array_capacity(a)    ((a) ? A_GET_CAP(a) : 0)                     
#define array_len(a)         ((a) ? A_GET_SIZE(a) / sizeof(*(a)) : 0)     
#define array_is_empty(a)    (array_size(a) == 0)


static inline void* __my_array_realloc(void *a, int new_cap, int new_size) {
    size_t total_bytes = 2 * sizeof(int) + new_cap;
    
    void *real_ptr = a ? A_GET_META_PTR(a) : NULL;
    void *new_real_ptr = realloc(real_ptr, total_bytes);
    if (!new_real_ptr) return NULL;
    
    int *meta = (int *)new_real_ptr;
    meta[0] = new_cap;   
    meta[1] = new_size;  
    
    return (void *)((char *)new_real_ptr + 2 * sizeof(int));
}

static inline void* array_strict_init(size_t init_bytes) {
    void *a = __my_array_realloc(NULL, init_bytes, init_bytes);
    if (a && init_bytes > 0) {
        memset(a, 0, init_bytes);
    }
    return a;
}


static inline void* array_init(size_t init_bytes) {
    size_t init_cap = init_bytes + (init_bytes >> 1); 
    if (init_cap < ARR_MIN_CAP_BYTES) init_cap = ARR_MIN_CAP_BYTES;
    
    void *a = __my_array_realloc(NULL, init_cap, init_bytes);
    if (a && init_bytes > 0) {
        memset(a, 0, init_bytes);
    }
    return a;
}

static inline void array_free(void *a) {
    if (a) {
        free(A_GET_META_PTR(a));
    }
}

/* ------------------ 泛型 Push 实现 ------------------ */

static inline void __my_array_push(void **a_ptr, int data_size, const void* val) {
    if (!a_ptr || !*a_ptr) return;
    
    void *a = *a_ptr;
    int size = A_GET_SIZE(a); // 当前字节数
    int cap = A_GET_CAP(a);   // 当前容量字节数
    
    // 检查字节空间是否足够
    if (size + data_size > cap) {
        int new_cap = cap + (cap >> 1); // 1.5 倍扩容
        if (new_cap < size + data_size) {
            new_cap = size + data_size;
        }
        a = __my_array_realloc(a, new_cap, size);
        if (!a) return;
        *a_ptr = a;
    }
    
    memcpy((char *)a + size, val, data_size);
    A_GET_SIZE(a) = size + data_size;
}

// array_push(arr, 100))
#define array_push(a, val) do { \
    __typeof__(*(a)) __tmp = (val); \
    __my_array_push((void**)&(a), sizeof(*(a)), &__tmp); \
} while(0)

#endif