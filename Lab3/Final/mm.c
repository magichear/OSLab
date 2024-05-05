/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 * TODO:
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"


/*explicit free list start*/
#define WSIZE 8
#define DSIZE 16
#define CHUNKSIZE (1 << 12)
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, prev_alloc, alloc) (((size) & ~(1<<1)) | ((prev_alloc << 1) & ~1) | (alloc))
#define PACK_PREV_ALLOC(val, prev_alloc) ((val & ~(1<<1)) | (prev_alloc << 1))
#define PACK_ALLOC(val, alloc) ((val) | (alloc))

#define GET(p) (*(unsigned long *)(p))
#define PUT(p, val) (*(unsigned long *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) ((GET(p) & 0x2) >> 1)

#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) /*only for free blk*/
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE))) /*only when prev_block is free, which can usd*/

#define GET_PRED(bp) (GET(bp))  // 结构 ： 块头 | 上一空闲地址 | 下一空闲地址 | 内容 | 块尾
#define SET_PRED(bp, val) (PUT(bp, val))

#define GET_SUCC(bp) (GET(bp + WSIZE))
#define SET_SUCC(bp, val) (PUT(bp + WSIZE, val))

#define MIN_BLK_SIZE (2 * DSIZE)
/*explicit free list end*/

/* single word (4) or double word (8) alignment */
#define ALIGNMENT DSIZE

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char *heap_listp;
static char *free_listp;

// static void *find_fit(size_t asize);
#if FIRST_FIT
static void *find_fit_first(size_t asize);
#else
static void *find_fit_best(size_t asize);
#endif

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void place(void *bp, size_t asize);
static void add_to_free_list(void *bp);
static void delete_from_free_list(void *bp);
double get_utilization();
void mm_check(const char *);

/*
    TODO:
        完成一个简单的分配器内存使用率统计
        user_malloc_size: 用户申请内存量
        heap_size: 分配器占用内存量
    HINTS:
        1. 在适当的地方修改上述两个变量，细节参考实验文档
        2. 在 get_utilization() 中计算使用率并返回
*/
size_t user_malloc_size = 0;
size_t heap_size = 0;
double get_utilization() {
    if (heap_size == 0) {
        return 0.0;
    }
    return (double)user_malloc_size / heap_size;
}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    free_listp = NULL;

    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1, 1));
    heap_listp += (2 * WSIZE);

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    /* mm_check(__FUNCTION__);*/
    heap_size = CHUNKSIZE;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    /*printf("\n in malloc : size=%u", size);*/
    /*mm_check(__FUNCTION__);*/
    size_t newsize;         /* Adjusted block size */
    size_t extend_size;     /* Amount to extend head if not fit */
    char *bp;

    /* Ignore spurious requesets */
    if (size == 0)
        return NULL;
    /* Adjust block size to include overhead and alignment reqs. */
    newsize = MAX(MIN_BLK_SIZE, ALIGN((size + WSIZE))); 

    /* Search the free list for a fit */
    #if FIRST_FIT
    if ((bp = find_fit_first(newsize)) != NULL)
    {
        place(bp, newsize);
        user_malloc_size += (GET_SIZE(HDRP(bp)) - WSIZE);   // 获取不含头部的分配块的大小
        return bp;
    }
    #else
    if ((bp = find_fit_best(newsize)) != NULL)
    {
        place(bp, newsize);
        user_malloc_size += (GET_SIZE(HDRP(bp)) - WSIZE);
        return bp;
    }
    #endif
    /*no fit found.*/
    extend_size = MAX(newsize, CHUNKSIZE);
    if ((bp = extend_heap(extend_size / WSIZE)) == NULL)
    {
        return NULL;
    }
    place(bp, newsize);
    user_malloc_size += (GET_SIZE(HDRP(bp)) - WSIZE);

    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    void *head_next_bp = NULL;

    PUT(HDRP(bp), PACK(size, prev_alloc, 0));
    PUT(FTRP(bp), PACK(size, prev_alloc, 0));
    /*printf("%s, addr_start=%u, size_head=%u, size_foot=%u\n",*/
    /*    __FUNCTION__, HDRP(bp), (size_t)GET_SIZE(HDRP(bp)), (size_t)GET_SIZE(FTRP(bp)));*/

     /*notify next_block, i am free*/
    head_next_bp = HDRP(NEXT_BLKP(bp));
    PUT(head_next_bp, PACK_PREV_ALLOC(GET(head_next_bp), 0));

    /* add_to_free_list(bp);*/
    user_malloc_size -= (size - WSIZE);
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

static void *extend_heap(size_t words)
{
    /*get heap_brk*/
    char *old_heap_brk = mem_sbrk(0);
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(old_heap_brk));

    /*printf("\nin extend_heap prev_alloc=%u\n", prev_alloc);*/
    char *bp;
    size_t size;
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    PUT(HDRP(bp), PACK(size, prev_alloc, 0)); /*last free block*/
    PUT(FTRP(bp), PACK(size, prev_alloc, 0));

    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 0, 1)); /*break block*/

    heap_size += size;

    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    /*add_to_free_list(bp);*/
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    /*
        TODO:
            将 bp 指向的空闲块 与 相邻块合并
            结合前一块及后一块的分配情况，共有 4 种可能性
            分别完成相应case下的 数据结构维护逻辑
    */
    if (prev_alloc && next_alloc) /* 前后都是已分配的块 */
    {   // 修改后一个块的头部
        PUT(HDRP(NEXT_BLKP(bp)), PACK_PREV_ALLOC(GET(HDRP(NEXT_BLKP(bp))), 0));
        // 修改自身
        PUT(HDRP(bp), PACK_ALLOC(GET(HDRP(bp)), 0));
        PUT(FTRP(bp), PACK_ALLOC(GET(HDRP(bp)), 0));
        add_to_free_list(bp);   // 将新的空闲块加入空闲链表
    }
    else if (prev_alloc && !next_alloc) /*前块已分配，后块空闲*/
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));     // 加上后块的大小
        delete_from_free_list(NEXT_BLKP(bp));      // 合并之后空闲链表中就不存在单独的后块了
        PUT(HDRP(bp), PACK(size, 1, 0));           // 向头部和尾部添加大小、前邻居块分配、自身空闲信息
        PUT(FTRP(bp), PACK(size, 1, 0));           // 先修改头部，size改变了才方便修改尾部
        // 再往后的块无需修改
        add_to_free_list(bp);   // 将新的空闲块加入空闲链表
    }
    else if (!prev_alloc && next_alloc) /*前块空闲，后块已分配*/
    {   // 前块需要被合并时，可以不用修改空闲链表（因为指针位置是固定的）
        bp = PREV_BLKP(bp);        // 移动指针到前块位置
        size += GET_SIZE(HDRP(bp));
        PUT(HDRP(bp), PACK(size, GET_PREV_ALLOC(HDRP(bp)), 0));
        PUT(FTRP(bp), PACK(size, GET_PREV_ALLOC(HDRP(bp)), 0));
        // 修改后块
        PUT(HDRP(NEXT_BLKP(bp)), PACK_PREV_ALLOC(GET(HDRP(NEXT_BLKP(bp))), 0));
    }
    else /*前后都是空闲块*/
    {
        size = size + GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_from_free_list(NEXT_BLKP(bp));
        bp = PREV_BLKP(bp);     // 并非通过空闲块获取地址，所以先删除没有影响
        PUT(HDRP(bp), PACK(size, GET_PREV_ALLOC(HDRP(bp)), 0));
        PUT(FTRP(bp), PACK(size, GET_PREV_ALLOC(HDRP(bp)), 0)); 
        // 再往后的块无需修改
    }
    return bp;
}

#if FIRST_FIT
static void *find_fit_first(size_t asize)
{
    /* 
        首次匹配算法
        TODO:
            遍历 freelist， 找到第一个合适的空闲块后返回
        
        HINT: asize 已经计算了块头部的大小
    */
    void *bp = NULL;
    for (bp = free_listp; bp != NULL; bp = (void *)GET_SUCC(bp)) {  // SUCC 是下一个空闲块
        if (asize <= GET_SIZE(HDRP(bp))) {  // 空闲块剩余空间大于asize即可
            return bp;
        }
    }
    return NULL; // 运行至此则没有合适空闲块
}

#else
static void* find_fit_best(size_t asize) {
    /* 
        最佳配算法
        TODO:
            遍历 freelist， 找到最合适的空闲块，返回
        
        HINT: asize 已经计算了块头部的大小
    */
    void *bp, *best_fit = NULL;
    size_t min_val = ~0;       // 先定义为无符号整数最大值
    size_t cur_val = 0;        // 存储空闲块剩余空间大小
    /* 遍历一次，找出最小值 */
    for (bp = free_listp; bp != NULL; bp = (void *)GET_SUCC(bp)) {
        cur_val = GET_SIZE(HDRP(bp)) - asize;
        if ((GET_SIZE(HDRP(bp)) > asize) && (cur_val < min_val)) {    // 找到更小的值
            best_fit = bp;
            min_val = cur_val;
        }
    }
    return best_fit; // 没有找到会返回NULL
}
#endif

static void place(void *bp, size_t asize)
{
    /* 
        TODO:
        将一个空闲块转变为已分配的块

        HINTS:
            1. 若空闲块在分离出一个 asize 大小的使用块后，剩余空间不足空闲块的最小大小，
               则原先整个空闲块应该都分配出去
            2. 若剩余空间仍可作为一个空闲块，则原空闲块被分割为一个已分配块+一个新的空闲块
            3. 空闲块的最小大小已经 #define，或者根据自己的理解计算该值
    */
    size_t total_size = GET_SIZE(HDRP(bp));      // 获取块大小
    
    if ((total_size - asize) > MIN_BLK_SIZE) {   // 剩余空间仍可作为空闲块
        PUT(HDRP(bp), PACK(asize, GET_PREV_ALLOC(HDRP(bp)), 1));    // 设置分配块    
        delete_from_free_list(bp);               // 从空闲链表中删除     
        /* 修改空闲块 */
        PUT(HDRP(NEXT_BLKP(bp)), PACK(total_size-asize, 1, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(total_size-asize, 1, 0));
        add_to_free_list(NEXT_BLKP(bp));
        /* 分配块只有块头，空闲块有块头和块尾 */
        /* 后面的块不需要改动 */
    } 
    else {                                      // 余额太小，应该全部分配
        delete_from_free_list(bp);    
        PUT(HDRP(bp), PACK_ALLOC(GET(HDRP(bp)), 1));    // 修改为已分配
        PUT(HDRP(NEXT_BLKP(bp)), PACK_PREV_ALLOC(GET(HDRP(NEXT_BLKP(bp))), 1));     // 对下一个相邻块设置该块的分配状态
        if (GET_ALLOC(HDRP(NEXT_BLKP(bp))) == 0){      // 相邻块未分配
            PUT(FTRP(NEXT_BLKP(bp)), PACK_PREV_ALLOC(GET(HDRP(NEXT_BLKP(bp))), 1));     // 对下一个相邻块设置该块的分配状态
        }
    }
}

static void add_to_free_list(void *bp)
{
    /*set pred & succ*/
    if (free_listp == NULL) /*free_list empty*/
    {
        SET_PRED(bp, 0);
        SET_SUCC(bp, 0);
        free_listp = bp;
    }
    else
    {
        SET_PRED(bp, 0);
        SET_SUCC(bp, (size_t)free_listp); /*size_t ???*/
        SET_PRED(free_listp, (size_t)bp);
        free_listp = bp;
    }
}

static void delete_from_free_list(void *bp)
{
    size_t prev_free_bp=0;
    size_t next_free_bp=0;
    if (free_listp == NULL)
        return;
    prev_free_bp = GET_PRED(bp);
    next_free_bp = GET_SUCC(bp);

    if (prev_free_bp == next_free_bp && prev_free_bp != 0)
    {
        /*mm_check(__FUNCTION__);*/
        /*printf("\nin delete from list: bp=%u, prev_free_bp=%u, next_free_bp=%u\n", (size_t)bp, prev_free_bp, next_free_bp);*/
    }
    if (prev_free_bp && next_free_bp) /*11*/
    {
        SET_SUCC(prev_free_bp, GET_SUCC(bp));
        SET_PRED(next_free_bp, GET_PRED(bp));
    }
    else if (prev_free_bp && !next_free_bp) /*10*/
    {
        SET_SUCC(prev_free_bp, 0);
    }
    else if (!prev_free_bp && next_free_bp) /*01*/
    {
        SET_PRED(next_free_bp, 0);
        free_listp = (void *)next_free_bp;
    }
    else /*00*/
    {
        free_listp = NULL;
    }
}

void mm_check(const char *function)
{
    /* printf("\n---cur func: %s :\n", function);
     char *bp = free_listp;
     int count_empty_block = 0;
     while (bp != NULL) //not end block;
     {
         count_empty_block++;
         printf("addr_start：%u, addr_end：%u, size_head:%u, size_foot:%u, PRED=%u, SUCC=%u \n", (size_t)bp - WSIZE,
                (size_t)FTRP(bp), GET_SIZE(HDRP(bp)), GET_SIZE(FTRP(bp)), GET_PRED(bp), GET_SUCC(bp));
         ;
         bp = (char *)GET_SUCC(bp);
     }
     printf("empty_block num: %d\n\n", count_empty_block);*/
}