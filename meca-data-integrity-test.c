#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    // Verify the user provided exactly one argument (the size of the source buffer)
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <size>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    // Convert the command line argument to a size_t value
    size_t size = strtoul(argv[1], NULL, 0);
    if (size == 0) {
        fprintf(stderr, "Invalid size value.\n");
        return EXIT_FAILURE;
    }
    
    // Allocate the source buffer
    uint64_t *src = malloc(size);
    if (!src) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    
    // Seed the random number generator and fill the source buffer with random values (0 - 255)
/*    srand((unsigned) time(NULL));
    for (size_t i = 0; i < size; i++) {
        src[i] = rand() % 256;
    }*/

    for (size_t i = 0; i < size/sizeof(uint64_t); i++) {
        src[i] = i*sizeof(uint64_t);
    }
    
    // Open /dev/mem with read/write and synchronous I/O flags
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        free(src);
        return EXIT_FAILURE;
    }
    
    // Define the desired offset for memory mapping (0x200000000)
    off_t offset = 0x200000000;
    
    // mmap the memory with the same size as the source buffer.
    // MAP_SHARED allows changes to be visible to other processes and the hardware.
    uint64_t *mapped = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        free(src);
        return EXIT_FAILURE;
    }
    
    printf("src[0] = 0x%lx\n", *((int64_t*)src));
    // Copy the random data from src to the mmapped region
//    memcpy(mapped, src, size);
    for(size_t i = 0; i < size/sizeof(uint64_t); i++) {
	    mapped[i] = src[i];
    }
    
    // Compare the data in the source and the mmapped buffer.
    // If a byte is different, print the offset and the differing byte values.
    for (size_t i = 0; i < size/sizeof(uint64_t); i++) {
        if (src[i] != mapped[i]) {
            printf("Difference at offset 0x%lx: src = 0x%016lx, mapped = 0x%016lx\n", i*sizeof(uint64_t), src[i], mapped[i]);
        }
    }
    
    // Clean up: unmap the memory, close the file, and free the source buffer.
    if (munmap(mapped, size) == -1) {
        perror("munmap");
    }
    close(fd);
    free(src);

    return EXIT_SUCCESS;
}

