#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#include "malloc.h"

void* malloc(size_t sz){
    void *(*libc_malloc)(size_t) = dlsym(RTLD_NEXT, "malloc");
    printf("malloc\n");
    return my_malloc(sz);
}

void free(void *p){
    void (*libc_free)(void*) = dlsym(RTLD_NEXT, "free");
    printf("free\n");
    my_free(p);
}


typedef struct{
	int a;
	int b;
	int c;
	int d;
	int e;
} test_t;


int main(void){
	test_t *ptr,*ptr2;
	char *str;
	long heap_brk_before = (long) sbrk(0);
	long heap_brk_after;

	ptr = malloc(sizeof(test_t));
	ptr2 = malloc(sizeof(test_t));
	heap_brk_after = (long) sbrk(0);

	ptr->a = 67;
	ptr->b = 68;
	ptr->c = 69;
	ptr->d = 70;
	ptr->e = 71;

	asprintf(&str, "%d\n", ptr->a);
	free(str);
	free(ptr);
	free(ptr2);
	printf("brk_before = %d, brk_after = %d, heap_size %d \n", heap_brk_before, (long) sbrk(0), (long) sbrk(0) - heap_brk_before );
	print_free_list();
	return 0;
}
