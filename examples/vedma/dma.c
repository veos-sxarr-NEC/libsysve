#include <stdio.h>
#include <stdlib.h>
#include <vedma.h>
#include <vhshm.h>
#include <unistd.h>

static uint64_t
ve_register_mem_to_dmaatb_unaligned(void *vemva, size_t size)
{
    uint64_t align = sysconf(_SC_PAGESIZE);
    uint64_t addr = (uint64_t)vemva;
    uint64_t offset = addr & (align - 1);
    void* addr_aligned = (void *)(addr & ~(align - 1));
    uint64_t size_aligned = (offset + size + align - 1) & ~(align - 1);

    uint64_t data_vehva = ve_register_mem_to_dmaatb(addr_aligned, size_aligned);
    if (data_vehva == (uint64_t)-1)
	return (uint64_t)-1;
    return data_vehva + offset;
}

static int
ve_unregister_mem_from_dmaatb_unaligned(uint64_t vehva) 
{
    uint64_t align = sysconf(_SC_PAGESIZE);
    uint64_t vehva_aligned = vehva & ~(align - 1);
    return ve_unregister_mem_from_dmaatb(vehva_aligned);
}

int main(int argc, char* argv[])
{
    int key = 0x19761215;
    size_t shm_size = 2 * 1024 * 1024; /* A multiple of 2M */
    size_t data_size = 4; /* A multiple of 4 */

    // Attach shm on VH to VE
    int shmid = vh_shmget(key, shm_size, SHM_HUGETLB);
    if (shmid == -1) {
        perror("vh_shmget");
        return 1;
    }
    fprintf(stderr, "shmid=%d\n", shmid);

    uint64_t shm_vehva = NULL;
    void* p = vh_shmat(shmid, NULL, 0, (void *)&shm_vehva);
    if (p == (void*)-1) {
        perror("vh_shmat");
        return 1;
    }
    fprintf(stderr, "shm_vehva=%p\n", shm_vehva);

    if (ve_dma_init() != 0) {
        perror("ve_dma_init");
        return 1;
    }

    int *data = malloc(data_size);

    fprintf(stderr, "data=%p data_size=0x%lx\n", data, data_size);
    uint64_t data_vehva = ve_register_mem_to_dmaatb_unaligned(data, data_size);
    if (data_vehva == (uint64_t)-1) {
        perror("ve_register_mem_to_dmaatb_unaligned");
        return 1;
    }
    fprintf(stderr, "data_vehva=0x%lx\n", data_vehva);

    // read
    int ret = ve_dma_post_wait(data_vehva, shm_vehva, data_size);
    fprintf(stderr, "ve_dma_post_wait(read): ret=%d\n", ret);
    fprintf(stderr, "%d\n", *data);

    (*data)++;

    fprintf(stderr, "%d\n", *data);

    // write
    ret = ve_dma_post_wait(shm_vehva, data_vehva, data_size);
    fprintf(stderr, "ve_dma_post_wait(write): ret=%d\n", ret);

    vh_shmdt(p);
    ret = ve_unregister_mem_from_dmaatb_unaligned(data_vehva);
    if (ret == -1) {
        perror("ve_unregister_mem_from_dmaatb_unaligned");
        return 1;
    }

    return 0;
}
