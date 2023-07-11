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
static void *fill_thread(void *);
static void fill_buffer(uint8_t *, size_t);

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

	// Fill the buffer via kernel reads.
	(void) printf("filling buffer ... ");
	fill_buffer(addr, len);
	(void) printf("done.\n");

// 	(void) printf("checking contents of region (expect all 0x0) ... \n");
// 	for (i = 0; i < len; i++) {
// 		if (addr[i] != 0) {
// 			(void) printf("found non-zero byte at %p!\n", &addr[i]);
// 		}
// 	}
// 
// 	(void) printf("done.\n");
// 
// 	(void) printf("writing contents of region ... \n");
// 	(void) memset(addr, 1, len);
// 
// 	(void) printf("checking contents of region (expect all 0x1) ... \n");
// 	for (i = 0; i < len; i++) {
// 		if (addr[i] != 1) {
// 			(void) printf("found unexpected byte at %p: %x\n",
// 			    &addr[i], addr[i]);
// 		}
// 	}
// 
// 	(void) printf("done.\n");

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

static void
fill_buffer(uint8_t *addr, size_t len)
{
	int pipes[2];
	pthread_t tid;
	size_t filled;
	void *status;

	if (pipe(pipes) != 0) {
		err(EXIT_FAILURE, "pipe");
	}

	if (pthread_create(&tid, NULL, fill_thread,
	    (void *)(uintptr_t)pipes[1]) != 0) {
		err(EXIT_FAILURE, "pthread_create");
	}

	for (filled = 0; filled < len; ) {
		int rv, left, buflen;
		left = len - filled;
		buflen = left < 16 * 1024 ? left : 16 * 1024;
		rv = read(pipes[0], &addr[filled], buflen);
		if (rv < 0) {
			err(EXIT_FAILURE, "read");
		}
		if (rv == 0) {
			errx(EXIT_FAILURE, "unexpected short read\n");
		}

		filled += rv;
	}

	if (pthread_join(tid, &status) != 0) {
		err(EXIT_FAILURE, "pthread_join (fill thread)");
	}
}

static void *
fill_thread(void *arg)
{
	int pipefd = (int)(uintptr_t)arg;
	size_t len = high - low;
	size_t filled;
	char buffer[4096];

	(void) memset(buffer, 1, sizeof (buffer));

	for (filled = 0; filled < len; ) {
		int rv;

		rv = write(pipefd, buffer, sizeof (buffer));
		if (rv < 0) {
			err(EXIT_FAILURE, "write");
		}
		if (rv == 0) {
			errx(EXIT_FAILURE, "unexpected zero write");
		}

		filled += rv;
	}

	return (NULL);
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

			(void) usleep(5);
		}
	}

	return (NULL);
}
