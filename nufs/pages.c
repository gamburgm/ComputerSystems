// based on cs3650 starter code

#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>

#include "pages.h"
#include "util.h"
#include "bitmap.h"
#include "inode.h"

const int PAGE_COUNT = 256;
const int NUFS_SIZE  = 4096 * 256; // 1MB

static int   pages_fd   = -1;
static void* pages_base =  0;

void
pages_init(const char* path)
{
	struct stat st;
	int result = stat(path, &st);

    pages_fd = open(path, O_CREAT | O_RDWR, 0644);
    assert(pages_fd != -1);

    int rv = ftruncate(pages_fd, NUFS_SIZE);
    assert(rv == 0);

    pages_base = mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, pages_fd, 0);
    assert(pages_base != MAP_FAILED);

	if (result == 0) {
		return;
	}

    void* pbm = get_pages_bitmap();
    bitmap_put(pbm, 0, 1);
	bitmap_put(pbm, 1, 1);
	bitmap_put(pbm, 2, 1);
	bitmap_put(pbm, 3, 1);
	bitmap_put(pbm, 4, 1);
	bitmap_put(pbm, 5, 1);
	bitmap_put(pbm, 6, 1);
	void* ibm = get_inode_bitmap();
	bitmap_put(ibm, 0, 1);
	bitmap_put(ibm, 1, 1);
	bitmap_put(ibm, 2, 1);
	bitmap_put(ibm, 3, 1);
	bitmap_put(ibm, 4, 1);
	bitmap_put(ibm, 5, 1);
	bitmap_put(ibm, 6, 1);

	inode* root = get_inode(6);


	// should root have one reference or zero references to start?
	root->refs = 1;
	root->mode = 040755;
	root->size = 0;
	// ptrs correspond with the datablock, which I'm indexing by page
	// by 1, I mean the data block stored in Page 1 of the filesystem
	root->ptrs[0] = 6;
	// is this a good way of signifying that this doesn't point anywhere?
	root->ptrs[1] = 0;
	root->iptr = 0;
	printf("success!\n");
}

void
pages_free()
{
    int rv = munmap(pages_base, NUFS_SIZE);
    assert(rv == 0);
}

void*
pages_get_page(int pnum)
{
    return pages_base + 4096 * pnum;
}

void*
get_pages_bitmap()
{
    return pages_get_page(0);
}

void*
get_inode_bitmap()
{
    uint8_t* page = pages_get_page(0);
    return (void*)(page + 32);
}

int
alloc_page()
{
    void* pbm = get_pages_bitmap();

    for (int ii = 7; ii < PAGE_COUNT; ++ii) {
        if (!bitmap_get(pbm, ii)) {
            bitmap_put(pbm, ii, 1);
            printf("+ alloc_page() -> %d\n", ii);
            return ii;
        }
    }

    return -1;
}

void
free_page(int pnum)
{
    printf("+ free_page(%d)\n", pnum);
    void* pbm = get_pages_bitmap();
    bitmap_put(pbm, pnum, 0);
}

