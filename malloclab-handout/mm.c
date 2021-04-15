/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
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

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "int3",
    /* First member's full name */
    "ddme",
    /* First member's email address */
    "",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static char *mem_heap;  //points to first byte of heap
static char *mem_brk;   //points to last byte of heap plus 1
static char *mem_max_addr;  //max legal heap addr plus 1

#define MAX_HEAP 100
#define MAX_LIST_NUM 18
static void* heap_listp;
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)   //书上有类似实现
{
  if(heap_listp = mem_sbrk((MAX_LIST_NUM * sizeof(intptr_t)+4*WSIZE)==(void *)-1)
    return -1;

  // 将所有空闲链表的头指针初始为NULL
  for (int i = 0; i < MAX_LIST_NUM; ++i) {
      PUT_P(heap_listp + i * sizeof(intptr_t), NULL);
  }
  heap_listp += MAX_LIST_NUM * sizeof(intptr_t);

  PUT(heap_listp,0);
  PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
  PUT(heap_listp + (3*WSIZE), PACK(0, 1));

  if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
    return -1;
  return 0;
}

static void *extend_heap(size_t words)
{
  char *bp;
  size_t size;

  size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
  if((long)(bp = mem_sbrk(size)) == -1)
    return NULL;

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
  return coalesce(bp);
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)            //书上的
{
	size_t align_size; 		/* Adjusted block size */
	size_t extendsize; 	/* Amount to extend heap if no fit */
	char *bp;

	/* Igore spurious requests */
	if (0 == size) {
		return NULL;
	}

	/* Adjusted block size to include overhead and alignment reqs */
	if (size <= DSIZE) {
		align_size = 2 * DSIZE;
	} else {
		align_size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
	}

	/* Search the free list for a fit */
	if ((bp = find_fit(align_size)) != NULL) {
		place(bp, align_size);
		return bp;
	}

	/* No fit found. Get more memory and place the block */
	extendsize = MAX(align_size, CHUNKSIZE);
	if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
		return NULL;
	}
	place(bp, align_size);
	return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  size_t size = GET_SIZE(HDRP(ptr));

  PUT(HDRP(ptr), PACK(size, 0));
  PUT(FTRP(ptr), PACK(size, 0));
  coalesce(ptr);
}

static void *coalesce(void *bp)
{
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if(prev_alloc && next_alloc)      //对应书上三种情况
    return bp;
  else if(prev_alloc && !next_alloc){
    size +=GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }
  else if(!prev_alloc && next_alloc){
    size +=GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  else{
    size += GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

	insert_list(bp);  //最后把空闲空间放回链表
  return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	size_t align_size;
	void *oldptr = ptr;
	void *newptr;

	if ( size == 0) {
		free(oldptr);
		return NULL;
	}
  //数据对齐
	if (size <= DSIZE) {
		align_size = 2 * DSIZE;
	} else {
		align_size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
	}
	if (align_size == GET_SIZE(HDRP(oldptr))) {
		return oldptr;
	}
  //可以用相邻空闲空间加上原本的
  size_t next_size=GET_SIZE(HDRP(NEXT_BLKP(oldptr)));
  size_t old_size=GET_SIZE(HDRP(oldptr))
  int next_alloc=GET_ALLOC(HDRP(NEXT_BLKP(oldptr)));
	if (!next_alloc && align_size < old_size+next_size) {
    delete_list(NEXT_BLKP(oldptr));
		newptr = oldptr;
    PUT(HDRP(newptr), PACK(next_size + old_size, 1));
    PUT(FTRP(newptr), PACK(next_size + old_size, 1));
		return newptr;
	}

	/* 从heap的其他地方寻找 */
	newptr = mm_malloc(size);
	if (NULL == newptr)
		return NULL;
	memmove(newptr, oldptr, size);
	mm_free(oldptr);

	return newptr;
}

//看分配的块是否还需要分割，如果过大就分两半
static void *place(void *bp, size_t size) {
    int max_size = GET_SIZE(HDRP(bp));
    int delta_size = max_size - size;
    delete_list(bp);
    // 如剩余部分少于最小块大小, 不做分割，在ucore中是不到两倍不分割
    if (delta_size < 2*DSIZE) {
        PUT(HDRP(bp), PACK(max_size, 1));
        PUT(FTRP(bp), PACK(max_size, 1));
        return bp;
    }

    // 否则需要分割，并将分割后的空闲块加到空闲链表
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(delta_size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(delta_size, 0));
		insert_list(bp);
    return bp;
}

void insert_list(void *bp)
{
	int index;
	size_t size;
	size = GET_SIZE(HDRP(bp));
	index = getListOffset(size);
    /*当前链表表头指向NULL*/
	if (GET_PTR(heap_listp + WSIZE * index) == NULL) {
		PUT_PTR(heap_listp + WSIZE * index, bp);
		PUT_PTR(bp, NULL);
		PUT_PTR((unsigned int *)bp + 1, NULL);
	} else {    /*当前链表已经有元素，插入到该链表表头，新的第一个元素与原来第一个元素连接*/
		PUT_PTR(bp, GET_PTR(heap_listp + WSIZE * index));
		PUT_PTR(GET_PTR(heap_listp + WSIZE * index) + 1, bp);  	
    PUT_PTR((unsigned int *)bp + 1, NULL);
		PUT_PTR(heap_listp + WSIZE * index, bp);
	}
}

void delete_list(void *bp)
{
    int index;
    size_t size;
    size = GET_SIZE(HDRP(bp));
    index = getListOffset(size);
    if (GET_PTR(bp) == NULL && GET_PTR((unsigned int *)bp + 1) == NULL) { 
        /* 后继next为NULL，前驱prev也为NULL,表明这个链表仅含一个结点。
        然后我们将合适大小对应的头指针设置为NULL*/
        PUT_PTR(heap_listp + WSIZE * index, NULL);
    } else if (GET_PTR(bp) == NULL && GET_PTR((unsigned int *)bp + 1) != NULL) {
        /*当前链表有多个结点，是最后一个。
        通过prev指针得到前一个块，再减去(unsigned int)1，就得到了指向next的指针，
        再将next指向NULL*/
        PUT_PTR( (GET_PTR( (unsigned int*)GET_PTR((unsigned int *)bp + 1) - 1 )), NULL );
        PUT_PTR(GET_PTR((unsigned int *)bp + 1), NULL);  //bp前驱指针prev=NULL
    } else if (GET_PTR(bp) != NULL && GET_PTR((unsigned int *)bp + 1) == NULL){
        /*当前链表有多个结点，是第一个
        第一条语句将相应大小的头指针指向了bp的next*/
        PUT_PTR(heap_listp + WSIZE * index, GET_PTR(bp));
        PUT_PTR(GET_PTR(bp) + 1, NULL); //prev=NULL
    } else if (GET_PTR(bp) != NULL && GET_PTR((unsigned int *)bp + 1) != NULL) {
        /*当前链表有多个节点，为中间结点
        第一条前一个块的next指向了当前块的next*/
        PUT_PTR(GET_PTR((unsigned int *)bp + 1), GET_PTR(bp));
        PUT_PTR(GET_PTR(bp) + 1, GET_PTR((unsigned int*)bp + 1));//bp->prev = bp->next
    }
}


void *find_fit(size_t size)   //找到合适大小空闲空间
{
	int index;
	index = getListOffset(size);
	unsigned int *ptr;

	/* 最合适的空闲链表没有满足要求到空闲块就到下一个空闲链表寻找*/
	while (index < 18) {
		ptr = GET_PTR(heap_listp + 4 * index);
		while (ptr != NULL) {
			if (GET_SIZE(HDRP(ptr)) >= size) {
				return (void *)ptr;
			}
			ptr = GET_PTR(ptr);
		}
		index++;
	}
	return NULL;
}


int getListOffset(size_t size)
{
	if (size <= (1<<4)) {
		return 0;
	} else if (size <= (1<<5)) {
		return 1;
	} else if (size <= (1<<6)) {
		return 2;
	} else if (size <= (1<<7)) {
		return 3;
	} else if (size <= (1<<8)) {
		return 4;
	} else if (size <= (1<<9)) {
		return 5;
	} else if (size <= (1<<10)) {
		return 6;
	} else if (size <= (1<<11)) {
		return 7;
	} else if (size <= (1<<12)) {
		return 8;
	} else if (size <= (1<<13)) {
		return 9;
	} else if (size <= (1<<14)) {
		return 10;
	} else if (size <= (1<<15)) {
		return 11;
	} else if (size <= (1<<16)) {
		return 12;
	} else if (size <= (1<<17)) {
		return 13;
	} else if (size <= (1<<18)) {
		return 14;
	} else if (size <= (1<<19)) {
		return 15;
	} else if (size <= (1<<20)) {
		return 16;
	} else {
		return 17;
	}
}