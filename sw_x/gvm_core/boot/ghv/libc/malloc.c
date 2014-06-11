#include "km_types.h"
#include "assert.h"
#include "bits.h"
#include "printf.h"
#include "string.h"
#include "pagepool.h"
#include "x86.h"
#include "ghv_mem.h"
#include "malloc.h"

#define PAD (2 * sizeof(u32))
#define MINPAYLOAD sizeof(tree_t)

#define SIZE(hdr) (hdr & ~1)
#define ALLOCATED(hdr) (hdr & 1)

#define HDR(chunk)     *((u32 *)chunk - 1)
#define PREVFTR(chunk) *((u32 *)chunk - 2)

#define NEXTCHUNK(chunk) (void *)((addr_t)chunk + SIZE(HDR(chunk))     + PAD)
#define PREVCHUNK(chunk) (void *)((addr_t)chunk - SIZE(PREVFTR(chunk)) - PAD)

#define FTR(chunk) PREVFTR(NEXTCHUNK(chunk))
#define PREVHDR(chunk) HDR(PREVCHUNK(chunk))
#define NEXTHDR(chunk) HDR(NEXTCHUNK(chunk))
#define NEXTFTR(chunk) FTR(NEXTCHUNK(chunk))

typedef struct tree {
    struct tree *left;
    struct tree *right;
} tree_t;

extern u8 _heap_start[];
static u8 *_heap_end;

static lock_t heap_lock;

static tree_t *free_tree;
static u32 free_size;

/* Sort chunks by size, then by address. Prefer lower address */
static inline int
compare_nodes(const void *n1, const void *n2)
{
    int r = SIZE(HDR(n1)) - SIZE(HDR(n2));

    return r ? r : n1 - n2;
}

/* Splay tree code copied right out of Daniel Slator's sample code */

static tree_t *
splay(const tree_t *n, tree_t *t)
{
    tree_t N, *l, *r, *y;
    int c;

    if (t == NULL) return t;

    N.left = N.right = NULL;
    l = r = &N;

    for (;;) {
	c = compare_nodes(n, t);

	if (c < 0) {
	    if (t->left == NULL) break;
	    
	    c = compare_nodes(n, t->left);
	    if (c < 0) {
		y = t->left;				/* rotate right */
		t->left = y->right;
		y->right = t;
		t = y;
		if (t->left == NULL) break;
	    }

	    r->left = t;				/* link right */
	    r = t;
	    t = t->left;
	} else if (c > 0) {
	    if (t->right == NULL) break;

	    c = compare_nodes(n, t->right);
	    if (c > 0) {
		y = t->right;				/* rotate left */
		t->right = y->left;
		y->left = t;
		t = y;
		if (t->right == NULL) break;
	    }

	    l->right = t;				/* link left */
	    l = t;
	    t = t->right;
	} else {
	    break;
	}
    }
    l->right = t->left;                                /* assemble */
    r->left = t->right;
    t->left = N.right;
    t->right = N.left;
    return t;
}

static tree_t *
insert(tree_t *n, tree_t *t)
{
    int c;

    if (t == NULL) {
	n->left = n->right = NULL;
	return n;
    }

    free_size += SIZE(HDR(n));

    t = splay(n, t);

    c = compare_nodes(n, t);
    if (c < 0) {
	n->left = t->left;
	n->right = t;
	t->left = 0;
	return n;
    } else if (c > 0) {
	n->right = t->right;
	n->left = t;
	t->right = 0;
	return n;
    } else {
	assert(c);
	return t;
    }
}

static tree_t *
delete(tree_t *n, tree_t *t)
{
    tree_t *x;

    if (t == NULL) {
	assert(t);
	return NULL;
    }

    free_size -= SIZE(HDR(t));
    
    t = splay(n, t);

    assert(n == t);

    if (n == t) {
	if (t->left == NULL) {
	    x = t->right;
	} else {
	    x = splay(n, t->left);
	    x->right = t->right;
	}

	return x;
    }

    return t;
}

static void init_chunk(void *chunk, u32 size)
{
    tree_t *t = chunk;
    t->left = t->right = NULL;

    HDR(chunk) = size;
    FTR(chunk) = size;
}

static void alloc_heap(u32 size)
{
    u8 *p;

    for (p = _heap_start; p < _heap_start + size; p += VM_PAGE_SIZE) {
	addr_t page = page_alloc();
	if (!page) {
	    break;
	}

	ghv_map((addr_t)p, page);
    }

    _heap_end = p;
}

void malloc_init(void)
{
    void *chunk = _heap_start + PAD;

    alloc_heap(40 << 20); /* 40 MiB */

    init_chunk(chunk, _heap_end - _heap_start - 2*PAD);
    free_tree = chunk;
    free_size = SIZE(HDR(chunk));

    /* guard padding */
    PREVFTR(chunk) = 1;
    NEXTHDR(chunk) = 1;

    lock_init(&heap_lock);
}

static void *
find_best_fit(tree_t *t, u32 size)
{
    tree_t *best = NULL;

    while (1) {
	if (t == NULL) {
	    break;
	}

	if (SIZE(HDR(t)) < size) {
	    t = t->right;
	} else {
	    best = t;
	    t = t->left;
	}
    }

    return best;
}

static void
_heap_check(void)
{
    void *chunk = _heap_start + PAD;

    printf("BEGIN HEAPCHECK\n");

    for (; chunk != _heap_end; chunk = NEXTCHUNK(chunk)) {
	printf("%p, %x, %d\n", chunk, SIZE(HDR(chunk)), ALLOCATED(HDR(chunk)));
	assert(HDR(chunk) == FTR(chunk));
	if (!ALLOCATED(HDR(chunk))) {
	    tree_t *n = chunk;

	    if (n->left) {
		assert(compare_nodes(n->left, n) < 0);
	    }
	    if (n->right) {
		assert(compare_nodes(n->right, n) > 0);
	    }
	}
    }

    printf("END HEAPCHECK\n");
}

static void *
_malloc(u32 size)
{
    void *chunk;
    u32 chunksize;

    size = (size + PAD - 1) & -PAD;
    if (size < MINPAYLOAD) {
	size = MINPAYLOAD;
    }

    chunk = find_best_fit(free_tree, size);
    if (chunk == NULL) {
	return chunk;
    }

    free_tree = delete(chunk, free_tree);

    chunksize = SIZE(HDR(chunk));

    /* split chunk */
    if (SIZE(HDR(chunk)) > size + MINPAYLOAD + PAD) {
	void *newchunk;
	u32 newsize = chunksize - size - PAD;

	init_chunk(chunk, size);

	newchunk = NEXTCHUNK(chunk);
	init_chunk(newchunk, newsize);
	free_tree = insert(newchunk, free_tree);
    }

    HDR(chunk) |= 1;
    FTR(chunk) |= 1;

    return chunk;
}

static void
_free(void *chunk)
{
    if (chunk == NULL) {
	return;
    }

    assert(ALLOCATED(HDR(chunk)));
    HDR(chunk) &= ~1;
    FTR(chunk) &= ~1;

    if (ALLOCATED(PREVFTR(chunk)) && ALLOCATED(NEXTHDR(chunk))) {
	/* allocated left and right, nothing to coalesce */
	free_tree = insert(chunk, free_tree);
    } else if (ALLOCATED(PREVFTR(chunk))) {
	/* coalesce to the right */
	void *next = NEXTCHUNK(chunk);
	u32 newsize = SIZE(HDR(chunk)) + SIZE(HDR(next)) + PAD;

	free_tree = delete(next, free_tree);

	init_chunk(chunk, newsize);

	free_tree = insert(chunk, free_tree);
    } else if (ALLOCATED(NEXTHDR(chunk))) {
	/* coalesce to the left */
	void *prev = PREVCHUNK(chunk);
	u32 newsize = SIZE(HDR(chunk)) + SIZE(HDR(prev)) + PAD;

	free_tree = delete(prev, free_tree);

	init_chunk(prev, newsize);
	free_tree = insert(prev, free_tree);	
    } else {
	/* coalesce to the left and right */
	void *prev = PREVCHUNK(chunk);
	void *next = NEXTCHUNK(chunk);
	u32 newsize = SIZE(HDR(chunk)) + SIZE(HDR(prev)) + SIZE(HDR(next)) + 2*PAD;

	free_tree = delete(prev, free_tree);
	free_tree = delete(next, free_tree);

	init_chunk(prev, newsize);
	free_tree = insert(prev, free_tree);
    }
}

void *malloc(u32 size)
{
    un flags;
    void *chunk;

    spinlock(&heap_lock, flags);
    chunk = _malloc(size);
    spinunlock(&heap_lock, flags);

    if (chunk) {
	memzero(chunk, size);
    }

    return chunk;
}

void free(void *chunk)
{
    un flags;

    spinlock(&heap_lock, flags);
    _free(chunk);
    spinunlock(&heap_lock, flags);
}

void heap_check(void)
{
    un flags;

    spinlock(&heap_lock, flags);
    _heap_check();
    spinunlock(&heap_lock, flags);

}

u32 heap_free_size(void)
{
    return free_size;
}
