#include <err.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

uintptr_t low = 0x77400000;
uintptr_t high = 0x80004000;

#define NTHREADS 128

static void *do_thread(void *);

int
main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
	uint8_t *addr;
	size_t len = high - low;
	unsigned int i;

	(void) printf("range: [%lx, %lx) (%ld MiB, %ld 4KiB pages)\n",
	    low, high, (high - low) / (1024 * 1024), (high - low) / 4096);

	addr = mmap((void *)low, len, PROT_READ | PROT_WRITE,
	    MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
	if (addr == NULL) {
		err(EXIT_FAILURE, "mmap");
	}
	printf("addr = %p\n", addr);

	(void) printf("checking contents of region (expect all 0x0) ... \n");
	for (i = 0; i < len; i++) {
		if (addr[i] != 0) {
			(void) printf("found non-zero byte at %p!\n", &addr[i]);
		}
	}

	(void) printf("done.\n");

	(void) printf("writing contents of region ... \n");
	(void) memset(addr, 1, len);

	(void) printf("checking contents of region (expect all 0x1) ... \n");
	for (i = 0; i < len; i++) {
		if (addr[i] != 1) {
			(void) printf("found unexpected byte at %p: %x\n",
			    &addr[i], addr[i]);
		}
	}

	(void) printf("done.\n");

	pthread_t tids[NTHREADS];

	for (i = 0; i < NTHREADS; i++) {
		if (pthread_create(&tids[i], NULL, do_thread,
		    (void *)(unsigned long)i) != 0) {
			err(EXIT_FAILURE, "pthread_create");
		}
	}

	for (i = 0; i < NTHREADS; i++) {
		void *status;
		if (pthread_join(tids[i], &status) != 0) {
			err(EXIT_FAILURE, "pthread_join thread %d", i);
		}
	}
}

static void *
do_thread(void *arg)
{
	unsigned int tid = (unsigned int)(unsigned long long)arg;
	uint8_t *addr = (uint8_t *)low;
	size_t len = high - low;
	unsigned int i;

	for (;;) {
		for (i = 0; i < len; i++) {
			if (addr[i] != 1) {
				(void) printf(
				    "tid %u: found unexpected byte at %p: %x\n",
				    tid, &addr[i], addr[i]);
			}

			(void) usleep(100);
		}
	}

	return (0);
}
