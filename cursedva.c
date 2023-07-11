#include <err.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

uintptr_t low = 0x77400000;
uintptr_t high = 0x80004000;

int
main(int argc __attribute__((unused)), char *argv[] __attribute__((unused)))
{
	char *addr;
	size_t len = high - low;
	size_t i;

	(void) printf("range: [%lx, %lx) (%ld MiB, %ld 4KiB pages)\n",
	    low, high, (high - low) / (1024 * 1024), (high - low) / 4096);

	addr = mmap((void *)low, len, PROT_READ | PROT_WRITE,
	    MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
	if (addr == NULL) {
		err(1, "mmap");
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
		addr[i] = 1;
		if (addr[i] != 1) {
			(void) printf("found unexpected byte at %p: %x\n",
			    &addr[i], addr[i]);
		}
	}

	(void) printf("done.\n");

}
