/*
 * Title: Dynamic Memory Allocator 
 * Author: Christian Wills <cwills.dev@gmail.com>
 * License: GPLv2 (see COPYING)
 * File: driver.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "malloc.h"

int num_mallocs = 0;
int num_frees = 0;

/*
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
*/
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

	char *buf;
	if( !(buf = malloc(2*1024*1024*1024))){
		struct rlimit mylim;
		printf("2gb malloc failed!\n");
		getrlimit(RLIMIT_DATA, &mylim);
		printf("RLIMIT_AS soft: %llu RLIMIT_AS hard: %llu\n",mylim.rlim_cur, mylim.rlim_max ); 
	}
	
	char *buf2;
	if( !(buf2 = malloc(1000000000))){
		printf("1gb malloc failed!\n");
	}
	
	char *buf3;
	if( !(buf3 = malloc(1000000000))){
		printf("1gb malloc failed!\n");
	}

	printf("brk_before = %ld, brk_after = %ld, heap_size %ld \n", heap_brk_before, (long) sbrk(0), (long) sbrk(0) - heap_brk_before );
	print_free_list();
	return 0;
}
