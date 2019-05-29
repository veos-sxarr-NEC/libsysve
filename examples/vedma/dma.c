#include <stdio.h>
#include <stdlib.h>
#include <vedma.h>
#include <vhshm.h>

int main(int argc, char* argv[])
{
    int key = 0x19761215;
    size_t size = 256 * 1024 * 1024;
    size_t align = 64 * 1024 * 1024;

    // Attach shm on VH to VE
    int shmid = vh_shmget(key, size, SHM_HUGETLB);
    if (shmid == -1) {
        perror("vh_shmget");
        return 1;
    }

    fprintf(stderr, "shmid=%d\n", shmid);

    void* vehva_vh = NULL;
    void* p = vh_shmat(shmid, NULL, 0, &vehva_vh);
    if (p == (void*)-1) {
        perror("vh_shmat");
        return 1;
    }
    fprintf(stderr, "vehva_vh=%p\n", vehva_vh);

    if (ve_dma_init() != 0) {
        perror("ve_dma_init");
        return 1;
    }

    void* vemva;
    if (posix_memalign(&vemva, align, size) != 0) {
        perror("posix_memalign");
        return 1;
    }
    fprintf(stderr, "vemva=%p\n", vemva);
    uint64_t vehva_ve = ve_register_mem_to_dmaatb(vemva, size);
    if (vehva_ve == (uint64_t)-1) {
        perror("ve_register_mem_to_dmaatb");
        return 1;
    }
    fprintf(stderr, "vehva_ve=%p\n", vehva_ve);
    fprintf(stderr, "p=%p\n", p);

    size_t transfer_size = 64 * 1024 * 1024;

    // read
    int ret = ve_dma_post_wait(vehva_ve, (uint64_t)vehva_vh, transfer_size);
    fprintf(stderr, "ve_dma_post_wait(read): ret=%d\n", ret);
    fprintf(stderr, "%d\n", *(int*)vemva);

    (*(int*)vemva)++;

    // write
    ret = ve_dma_post_wait((uint64_t)vehva_vh, vehva_ve, transfer_size);
    fprintf(stderr, "ve_dma_post_wait(write): ret=%d\n", ret);

    vh_shmdt(p);
    ve_unregister_mem_from_dmaatb(vehva_ve);

    return 0;
}
