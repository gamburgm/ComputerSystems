#include <stdint.h>
#include <assert.h>
#include "bitmap.h"
#include "inode.h"

void
print_inode(inode* node)
{
	return;
}

inode*
get_inode(int inum)
{
	// inodes start at page 1 and go till page 2
	void* inode_space = pages_get_page(1);
	return (inode*) (inode_space + (inum * sizeof(inode)));
}

int
alloc_inode()
{
	void* ibm = get_inode_bitmap();
	for (int ii = 7; ii < 256; ii++) {
		if (!bitmap_get(ibm, ii)) {
			bitmap_put(ibm, ii, 1);
			printf(" + alloc_inode() -> %d\n", ii);
			return ii;
		}
	}
	return -1;
}

void
free_inode(int inum)
{
	printf("+ free_inode(%d)\n", inum);
	void* ibm = get_inode_bitmap();
	bitmap_put(ibm, inum, 0);
}

// what int is this supposed to return?
int
grow_inode(inode* node, int size)
{
	// should this be an if less than return whatever else fail?
	assert(node->size + size <= 4096);
	node->size += size;
	return 0;
}

// same issue as above
int 
shrink_inode(inode* node, int size)
{
	assert(node->size - size >= 0);
	node->size -= size;
	return 0;

}

int 
inode_get_pnum(inode* node, int fpn)
{
	// don't know the purpose of this yet
}
