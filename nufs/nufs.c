// based on cs3650 starter code

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <bsd/string.h>
#include <assert.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include "pages.h"
#include "inode.h"
#include "util.h"
#include "directory.h"
#include "slist.h"

// implementation for: man 2 access
// Checks if a file exists.
int
nufs_access(const char *path, int mask)
{
	if (streq("/", path)) {
		return 0;
	}

	// get the root node and data block
	inode* path_node = get_inode(6);
	void* start_page = pages_get_page(6);

	slist* split_path = s_split(path, '/');
	slist* head = split_path;
	// skip root because we're starting at root
	split_path = split_path->next;

	int idx = 0;
	// iterate through each directory entry in each directory down the path
	// until path is found or not
	while (split_path && idx < path_node->size) {
		direntry* actual_page = (direntry*) (start_page + idx);
		
		if (streq(actual_page->name, split_path->data)) {
			idx = 0;
			split_path = split_path->next;
			path_node = get_inode(actual_page->inum);
			start_page = pages_get_page(path_node->ptrs[0]);
			continue;
		}

		idx += sizeof(direntry);
	}

	if (split_path == 0) {
		s_free(head);
		return 0;
	}

	s_free(head);
    printf("access(%s, %04o) -> negative\n", path, mask);
	return -ENOENT;
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nufs_getattr(const char *path, struct stat *st)
{
	if (streq(path, "")) {
		printf("getattr(%s) -> -ENOENT\n", path);
		return -ENOENT;
	}

	// get the root inode and data block
	inode* path_node = get_inode(6);
	void* start_page = pages_get_page(6);
	
	// loop through root block until path either found or reach end
	int ino = 6;
    int rv = 0;
	if (!streq(path, "/")) {
		int idx = 0;
		// int found = 0;
	
		slist* split_path = s_split(path, '/');
		slist* head = split_path;
		split_path = split_path->next;

		while (split_path != 0 && idx < path_node->size) {
			direntry* actual_page = (direntry*) (start_page + idx);

			//if found, set inode to appropriate value
			if (strcmp(actual_page->name, split_path->data) == 0) {
				ino = actual_page->inum;
				path_node = get_inode(ino);
				start_page = pages_get_page(path_node->ptrs[0]);
				idx = 0;
				split_path = split_path->next;
				continue;
			}
			idx += sizeof(direntry);
		}
		if (split_path != 0) {
			// is this correct?
			s_free(head);
			printf("getattr failed: %s\n", path);
			return -ENOENT;
		}
		s_free(head);
	}
	// initialize stat with the appropriate values
	inode* selected_node = get_inode(ino);

	st->st_ino   = ino;
	st->st_mode  = path_node->mode;
	st->st_nlink = path_node->refs;
	st->st_size  = path_node->size;
	st->st_uid   = getuid();
	st->st_atim  = path_node->atime;
	st->st_mtim  = path_node->mtime;

    printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode, st->st_size);
    return 0;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
	if (nufs_access(path, 1) != 0) {
		printf("readdir(%s) -> -ENOENT\n", path);
		return -ENOENT;
	}
    struct stat st;
    int rv;


	rv = nufs_getattr(path, &st);
    filler(buf, ".", &st, offset);
    assert(rv == 0);
	
	// loop through the directory and call 'filler' for every directory entry
	int idx = offset;
	inode* selected_node = get_inode(st.st_ino);
	void* page_contents  = pages_get_page(selected_node->ptrs[0]);

	while (idx < st.st_size) {
		struct stat new;
		direntry* new_entry = (direntry*) (page_contents + idx);
		
		// adding the size of DIR_NAME here
		// I do all of this just to call getattr: is it worth it? Can I get around it?
		char* entry_path = malloc(strlen(path) + 48);
		*entry_path = 0;
		strcat(entry_path, path);
		join_to_path(entry_path, new_entry->name);

		rv = nufs_getattr(entry_path, &new);

		free(entry_path);

		rv = filler(buf, new_entry->name, &new, 0);
		if (rv != 0) {
			break;
		}
		// originally added string size and sizeof uint8_t
		idx += sizeof(direntry);
	}


    printf("readdir(%s) -> %d\n", path, rv);
    return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nufs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	if (nufs_access(path, 0) == 0) {
		return -EEXIST;
	}
	// initialize inode and data block
	int rv;
	int ino = alloc_inode();
	assert(ino >= 0);

	inode* allocated_node = get_inode(ino);
	rv = alloc_page();
	assert(rv >= 0);

	// set new inode to appropriate values associated with data block
	allocated_node->ptrs[0] = rv;
	allocated_node->ptrs[1] = allocated_node->iptr = 0;
	allocated_node->size = 0;
	allocated_node->mode = mode;
	allocated_node->refs = 1;
	
	// add the new file to the directory
	inode* root = get_inode(6);
	void* start_page = pages_get_page(6);

	slist* split_path = s_split(path, '/');
	slist* head = split_path;
	split_path = split_path->next;
	int num = -1;
	int idx = 0;

	// find the directory that is supposed to contain the new file
	while (split_path->next != 0 && idx < root->size) {
		direntry* actual_page = (direntry*) (start_page + idx);

		if (streq(actual_page->name, split_path->data)) {
			idx = 0;
			split_path = split_path->next;
			num = actual_page->inum;
			root = get_inode(num);
			start_page = pages_get_page(root->ptrs[0]);
			continue;
		}
		idx += sizeof(direntry);
	}

	if (split_path->next != 0) {
		s_free(head);
		return -ENOENT;
	}

	start_page = pages_get_page(root->ptrs[0]);
	start_page += root->size;

	direntry* root_entries = (direntry*) start_page;
	// maybe do &root_entires->name?
	memcpy(root_entries, split_path->data, strlen(split_path->data) + 1);
	root_entries->inum = ino;
	root->size += sizeof(direntry);

	s_free(head);

    printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
	rv = 0;
    return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nufs_mkdir(const char *path, mode_t mode)
{
    int rv = nufs_mknod(path, mode | 040000, 0);
    printf("mkdir(%s) -> %d\n", path, rv);
    return rv;
}

int
nufs_unlink(const char *path)
{
	if (nufs_access(path, 1) != 0) {
		printf("unlink(%s) -> -ENOENT\n", path);
		return -ENOENT;
	}

	// get inode and page of associated path
	struct stat st;
	nufs_getattr(path, &st);
	inode* selected_node = get_inode(st.st_ino);
	selected_node->refs -= 1;
	
	// free those spots in their associated bitmaps
	if (selected_node->refs == 0) {
		free_page(selected_node->ptrs[0]);
		free_inode(st.st_ino);
	}

	// remove the entry from the directory that it is stored in
	int idx = 0;
	void* start_page   = pages_get_page(6);
	inode* start_inode = get_inode(6); 

	slist* split_path = s_split(path, '/');
	slist* head = split_path;
	split_path = split_path->next;

	// find the parent directory of the file specified by path
	while (split_path->next != 0 && idx < start_inode->size) {
		direntry* actual_page = (direntry*) (start_page + idx);

		if (streq(actual_page->name, split_path->data)) {
			idx = 0;
			start_inode = get_inode(actual_page->inum);
			start_page = pages_get_page(start_inode->ptrs[0]);
			split_path = split_path->next;
			continue;
		}
		idx += sizeof(direntry);
	}
	
	if (split_path->next != 0) {
		s_free(head);
		return -ENOENT;
	}

	direntry* actual_page = (direntry*) (start_page + idx);
	memcpy(actual_page, actual_page + 1, start_inode->size - idx - sizeof(direntry));
	start_inode->size -= sizeof(direntry);
	s_free(head);

    int rv = 0;
    return rv;
}

int
nufs_link(const char *from, const char *to)
{
	if (nufs_access(from, 1) != 0) {
		return -ENOENT;
	}

	if (nufs_access(to, 1) == 0) {
		return -EEXIST;
	}

	struct stat st;
	int rv = nufs_getattr(from, &st);
	assert(rv == 0);

	inode* selected_node = get_inode(st.st_ino);

	inode* root_inode = get_inode(6);
	void* root_page   = pages_get_page(6);

	slist* split_path = s_split(to, '/');
	slist* head = split_path;
	split_path = split_path->next;

	// find the directory that is supposed to contain 'to'
	int idx = 0;
	while (split_path->next && idx < root_inode->size) {
		direntry* actual_page = (direntry*) (root_page + idx);

		if (streq(split_path->next->data, actual_page->name)) {
			idx = 0;
			split_path = split_path->next;
			root_inode = get_inode(actual_page->inum);
			root_page = pages_get_page(root_inode->ptrs[0]);
			continue;
		}
		idx += sizeof(direntry);
	}

	if (split_path->next != 0) {
		s_free(head);
		return -ENOENT;
	}

	direntry* directory = (direntry*) (root_page + root_inode->size);
	memcpy(directory, split_path->data, strlen(split_path->data) + 1);
	directory->inum = st.st_ino;
	root_inode->size += sizeof(direntry);
	selected_node->refs += 1;

	s_free(head);

	rv = 0;
    printf("link(%s => %s) -> %d\n", from, to, rv);
	return rv;
}

int
nufs_rmdir(const char *path)
{
	if (nufs_access(path, 1) != 0) {
		return -ENOENT;
	}

	struct stat st;
	int rv = nufs_getattr(path, &st);

	inode* selected_node = get_inode(st.st_ino);

	if (selected_node->size != 0) {
		// no documentation I could find on what's supposed to happen here,
		// so I just return this
		return -1;
	}

	nufs_unlink(path);

    rv = 0;
    printf("rmdir(%s) -> %d\n", path, rv);
    return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nufs_rename(const char *from, const char *to)
{
	if (nufs_access(from, 1) != 0) {
		return -ENOENT;
	}

	if (nufs_access(to, 1) == 0) {
		return -EEXIST;
	}
	
	
	nufs_link(from, to);
	nufs_unlink(from);

	return 0;
}

int
nufs_chmod(const char *path, mode_t mode)
{
	if (nufs_access(path, 1) != 0) {
		return -ENOENT;
	}

	struct stat st;
	int rv = nufs_getattr(path, &st);
	assert(rv == 0);

	inode* selected_node = get_inode(st.st_ino);

	selected_node->mode = mode;

    printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
    return rv;
}

int
nufs_truncate(const char *path, off_t size)
{
	if (nufs_access(path, 1) != 0) {
		printf("truncate(%s, %ld bytes) -> -ENOENT\n", path, size);
		return -ENOENT;
	}

	struct stat st;
	nufs_getattr(path, &st);

	inode* selected_node = get_inode(st.st_ino);
	
	selected_node->size = size;
    printf("truncate(%s, %ld bytes) -> 0\n", path, size);
	return 0;
}

// this is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
int
nufs_open(const char *path, struct fuse_file_info *fi)
{
	if (nufs_access(path, 1) != 0) {
		return -ENOENT;
	}

    int rv = 0;
    printf("open(%s) -> %d\n", path, rv);
    return rv;
}

// Actually read data
int
nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if (nufs_access(path, 1) != 0) {
		return -ENOENT;
	}

	struct stat st;
	nufs_getattr(path, &st);

	inode* selected_node = get_inode(st.st_ino);

	// calculate how much will be read
	int distance_to_eof = (selected_node->size - offset < 0 ? 0 : selected_node->size - offset);
	
	int copy_length = (size < distance_to_eof ? size : distance_to_eof);

	// copy selected read bytes into buffer
	strncpy(buf, (char*) pages_get_page(selected_node->ptrs[0]), copy_length);
    printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, copy_length);
	return copy_length;
}

// Actually write data
int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if (nufs_access(path, 1) != 0) {
		return -ENOENT;
	}

	struct stat st;
	nufs_getattr(path, &st);

	inode* selected_node = get_inode(st.st_ino);
	
	int total_written = (size < 4096 - offset ? size : 4096 - offset);

	// calculate new size of file after write
	int curr_size = selected_node->size;
	selected_node->size = (curr_size > offset + total_written ? curr_size : offset + total_written);

	void* selected_page = pages_get_page(selected_node->ptrs[0]);

	// copy written data into file
	strncpy((char*) (selected_page + offset), buf, total_written);
    printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, total_written);
	return total_written;
}

// Update the timestamps on a file or directory.
int
nufs_utimens(const char* path, const struct timespec ts[2])
{
	if (nufs_access(path, 1) != 0) {
		return -ENOENT;
	}

	struct stat st;
	nufs_getattr(path, &st);

	inode* selected_node = get_inode(st.st_ino);

	selected_node->atime = ts[0];
	selected_node->mtime = ts[1];

	return 0;
}

// Extended operations
int
nufs_ioctl(const char* path, int cmd, void* arg, struct fuse_file_info* fi,
           unsigned int flags, void* data)
{
    int rv = -1;
    printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
    return rv;
}

int
nufs_symlink(const char* to, const char* from)
{
	if (nufs_access(from, 1) == 0) {
		return -EEXIST;
	}
	if (nufs_access(to, 1) != 0) {
		return -ENOENT;
	}

	nufs_mknod(from, 0120000, 0);

	struct stat st;
	nufs_getattr(from, &st);
	
	inode* selected_node = get_inode(st.st_ino);

	void* selected_page = pages_get_page(selected_node->ptrs[0]);

	strcpy((char*) selected_page, to);

	return 0;
}

int
nufs_readlink(const char* path, char* buf, size_t size) 
{
	if (nufs_access(path, 1) != 0) {
		return -ENOENT;
	}

	struct stat st;
	nufs_getattr(path, &st);

	inode* selected_node = get_inode(st.st_ino);
	void* selected_page = pages_get_page(selected_node->ptrs[0]);

	strcpy(buf, (char*) selected_page);

	return 0;
}

void
nufs_init_ops(struct fuse_operations* ops)
{
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access   = nufs_access;
    ops->getattr  = nufs_getattr;
    ops->readdir  = nufs_readdir;
    ops->mknod    = nufs_mknod;
    ops->mkdir    = nufs_mkdir;
    ops->link     = nufs_link;
    ops->unlink   = nufs_unlink;
	ops->symlink  = nufs_symlink;
	ops->readlink = nufs_readlink;
    ops->rmdir    = nufs_rmdir;
    ops->rename   = nufs_rename;
    ops->chmod    = nufs_chmod;
    ops->truncate = nufs_truncate;
    ops->open	  = nufs_open;
    ops->read     = nufs_read;
    ops->write    = nufs_write;
    ops->utimens  = nufs_utimens;
    ops->ioctl    = nufs_ioctl;
};

struct fuse_operations nufs_ops;

int
main(int argc, char *argv[])
{
    assert(argc > 2 && argc < 6);
	printf("%ld\n", sizeof(inode));
    printf("TODO: mount %s as data file\n", argv[--argc]);
	pages_init(argv[argc]);
    nufs_init_ops(&nufs_ops);
    int ret = fuse_main(argc, argv, &nufs_ops, NULL);
	return ret;
}

