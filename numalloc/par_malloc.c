#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include <stdlib.h>
#include <unistd.h>

#include "hmalloc.h"
#include "xmalloc.h"

//the buckets: one for each thread
//this is a pointer instead of an array because we mmap it, according to Professor Tuck's recommendation in class.
__thread void** alloc_space = 0;
//lock for every bucket
pthread_mutex_t mutexes[9];
//thread local variable to make sure there's no re-initialization of buckets
__thread int first_alloc = 1;
//__thread pthread_mutex_t first_alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

//identify the size allocated in a bucket based on the index of that bucket in alloc_space
int
identify_bucket(int idx)
{
	if (idx == 0) return 16;
	if (idx == 1) return 24;
	if (idx == 2) return 32;
	if (idx == 3) return 64;
	if (idx == 4) return 128;
	if (idx == 5) return 256;
	if (idx == 6) return 512;
	if (idx == 7) return 1024;
	if (idx == 8) return 2048;

	return -1;
}

//get the index of a bucket based on the size allocated in a bucket
int
get_bucket(size_t bytes)
{
	if (bytes <= 16)   return 0;
	if (bytes <= 24)   return 1;
	if (bytes <= 32)   return 2;
	if (bytes <= 64)   return 3;
	if (bytes <= 128)  return 4;
	if (bytes <= 256)  return 5;
	if (bytes <= 512)  return 6;
	if (bytes <= 1024) return 7;
	if (bytes <= 2048) return 8;

	return -1;
}

//get the number of allocations that fit in a page inside of the specified bucket index in alloc_space
int 
bit_shift(int bucket)
{
	if (bucket <= 0) return 63;
	if (bucket <= 1) return 63;
	if (bucket <= 2) return 63;
	if (bucket <= 3) return 62;
	if (bucket <= 4) return 31;
	if (bucket <= 5) return 15;
	if (bucket <= 6) return 7;
	if (bucket <= 7) return 3;
	if (bucket <= 8) return 1;

	return -1;
}

//get the beginning of a page that a pointer is in
void*
get_page(void* ptr)
{
	size_t mem_location = (size_t) ptr;
	//all pages are at addressed that are a multiple of 4096, according to Professor Tuck
	void* new_ptr = (void*) (mem_location - (mem_location % 4096));
	return new_ptr;
}

//find the first available page for an allocation starting from start_page and of allocation size bucket
void*
find_first_file(void* start_page, int bucket)
{
	size_t* prev_page = 0;
	size_t* curr_page = (size_t*) start_page;
	
	//iterate through the linked list of pages, checking if the bitmap at the beginning is filled or not
	for (; curr_page != 0; prev_page = curr_page, curr_page = (size_t*) *(curr_page + 1)) {
		if (~(((size_t) *(curr_page + 2)) | ((size_t) ~0 << bit_shift(bucket))) != 0) {
			return (void*) curr_page;
		}
	}

	//if none found, create new page, hook the previous page up to the new one to create linked list,
	//and set up the metadata at the beginning of the new page.
	void* new_page = mmap(0, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	*((size_t*) prev_page + 1) = (size_t) new_page;
	*((size_t*) new_page) = (size_t) identify_bucket(bucket);
	*((size_t*) new_page + 1) = 0;
	*((size_t*) new_page + 2) = 0;

	return new_page;
}

//find the first 'zero', or the first available allocation slot, in the bitmap
int
find_first_zero(size_t bitmap, int bucket)
{
	int ii = 0;

	//check if a bit at location ii is a 1 or not
	for (; ((bitmap >> ii) & 1) != 0 && ii <= bit_shift(bucket); ii++) {

	}
	if (ii == bit_shift(bucket)) {
		return -1;
	}
	return ii;
}

void*
xmalloc(size_t bytes)
{
	if (first_alloc) {
		alloc_space = mmap(0, 9 * sizeof(void*), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		for (int ii = 0; ii < 9; ii++) {
			pthread_mutex_init(&mutexes[ii], 0);
			int bucket = identify_bucket(ii);

			alloc_space[ii] = mmap(0, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
			*((size_t*) alloc_space[ii]) = bucket;
			*((size_t*) alloc_space[ii] + 1) = 0;
			*((size_t*) alloc_space[ii] + 2) = 0;
		}
		first_alloc = 0;
	}

	int bucket = get_bucket(bytes);

	if (bucket == -1) {
		bytes += sizeof(size_t);
		void* ptr = mmap(0, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
		*((size_t*) ptr) = bytes;
		return ptr + sizeof(size_t);
	}

	pthread_mutex_lock(&mutexes[bucket]);
	//find the next available page starting from the specified bucket in this thread
	void* alloc_page = find_first_file(alloc_space[bucket], bucket);

	//find the index in that page of the next slot to allocate
	int alloc_block = find_first_zero(*((size_t*) alloc_page + 2), bucket);
	//set that slot in the bitmap to a 1 bit
	*((size_t*) alloc_page + 2) = *((size_t*) alloc_page + 2) | 1 << alloc_block;

	pthread_mutex_unlock(&mutexes[bucket]);

	//return the pointer at the appropriate location: after the metadata and after all previous allocations
	//on that page
	void* the_ptr = alloc_page + (3 * sizeof(size_t)) + (bytes * (alloc_block));
	return the_ptr;
}

void
xfree(void* ptr)
{
	//get page and data from the pointer
	void* page_ptr = get_page(ptr);
	int bucket = get_bucket(*((size_t*) page_ptr));
	if (bucket == -1) {
		munmap(ptr, *((size_t*) ptr - 1));
		return;
	}

	pthread_mutex_lock(&mutexes[bucket]);

	//figure out the 'index' of the allocation in the page: which index it corresponds to on the bitmap
	int which_alloc = ((size_t) ptr - (size_t) page_ptr - (3 * sizeof(size_t))) / *((size_t*) page_ptr);

	//set that bit on the bitmap to 0
	*((size_t*) page_ptr + 2) = *((size_t*) page_ptr + 2) & (~(0 | (1 << which_alloc)));

	pthread_mutex_unlock(&mutexes[bucket]);
}

void*
xrealloc(void* ptr, size_t bytes)
{
	void* page_ptr = get_page(ptr);
	if (*((size_t*) page_ptr) == bytes) {
		puts("size same");
		return ptr;
	}

	int bucket = get_bucket(*((size_t*) page_ptr));

	void* new_ptr = xmalloc(bytes);

	pthread_mutex_lock(&mutexes[bucket]);
	memcpy(new_ptr, ptr, bytes);
	pthread_mutex_unlock(&mutexes[bucket]);

	xfree(ptr);
	return new_ptr;
}
