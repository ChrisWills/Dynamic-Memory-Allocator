#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#include "malloc.h"

int num_mallocs = 0;
int num_frees = 0;

void* malloc(size_t sz){
    //void *(*libc_malloc)(size_t) = dlsym(RTLD_NEXT, "malloc");
	num_mallocs++;
    return my_malloc(sz);
}

void free(void *p){
    //void (*libc_free)(void*) = dlsym(RTLD_NEXT, "free");
    num_frees++;
    my_free(p);
}

int main(void){
	long heap_brk_before = (long) sbrk(0);
	int i;
	char *ptrs[100];

	for(i=0; i < 100; i++){
		char *buf;
		int bytes = rand() % 8192;
//		printf("mallocing %d bytes\n",bytes);
		buf = malloc(bytes);
//		printf("%ld\n", (long int) buf % 8);
		ptrs[i] = buf;
	}

	for(i=0; i < 100; i++){
		//if(i != 53)
		free(ptrs[i]);
	}
	
	/*	
	for(i=99; i>=0 ; i--){
		free(ptrs[i]);
	}
*/
	printf("num_mallocs = %d, num_frees = %d\n", num_mallocs, num_frees);	
	printf("brk_before = %d, brk_after = %d, heap_size %d \n", heap_brk_before, (long) sbrk(0), (long) sbrk(0) - heap_brk_before );

	for(i=0; i < 100; i++){
		char *buf;
		int bytes = rand() % 8192;
		//printf("mallocing %d bytes\n",bytes);
		buf = malloc(bytes);
		//free(buf);
		ptrs[i] = buf;
	}
	
	for(i=0; i < 100; i++){
		//if(i != 78)
		free(ptrs[i]);
	}
	printf("brk_before = %d, brk_after = %d, heap_size %d \n", heap_brk_before, (long) sbrk(0), (long) sbrk(0) - heap_brk_before );
	print_free_list();
	return 0;
}
