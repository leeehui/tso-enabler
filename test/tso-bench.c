#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define NSEC_CONST  1000000000
#define BUF_SIZE  (2*1024*1024)

int main(void)
{
	struct timespec start;
	struct timespec stop;
	uint64_t loops = BUF_SIZE;
	uint64_t elapsed;
    uint64_t *buf_src;
    uint64_t *buf_dst;

    buf_src = malloc(BUF_SIZE * sizeof(uint64_t));
    buf_dst = malloc(BUF_SIZE * sizeof(uint64_t));

    memset(buf_dst, 0, BUF_SIZE * sizeof(uint64_t));
    for (int i = 0; i < BUF_SIZE; i++) {
        buf_src[i] = i;
    }
    printf("buf_dst[10]: %d \n", buf_dst[10]);

	clock_gettime(CLOCK_MONOTONIC, &start);

    // in case of comparing TSO with RCpc
    // replace ldr/str with ldapr/stlr
	asm volatile (
			"mov x0, %[loops]\n"
            "mov x1, %[buf_src]\n"
            "mov x2, %[buf_dst]\n"
			"1:\n"
            "add x1, x1, #8\n"
            "add x2, x2, #8\n"
            "ldr x3, [x1]\n"
            "str x3, [x2]\n"
			"sub x0, x0, #1\n"
			"cbnz x0, 1b\n"
			:
			:[loops]"r"(loops), [buf_src]"r"(buf_src), [buf_dst]"r"(buf_dst)
			:"x0","x1","x2","x3"
		     );
	clock_gettime(CLOCK_MONOTONIC, &stop);

	elapsed = (stop.tv_sec - start.tv_sec) * NSEC_CONST + (stop.tv_nsec - start.tv_nsec);

    printf("buf_dst[10]: %d \n", buf_dst[10]);
    printf("elapsed: %d \n", elapsed);

	return 0;
}
