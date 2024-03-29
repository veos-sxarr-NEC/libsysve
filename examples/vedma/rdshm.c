#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

int main(int argc, char* argv[])
{
    int key = 0x19761215;
    size_t shm_size = 2 * 1024 * 1024;

    int shmid = shmget(key, shm_size, 0);
    if (shmid == -1) {
        perror("shmget");
        return 1;
    }

    void* p = shmat(shmid, NULL, 0);
    if (p == (void*)-1) {
        perror("shmat");
        return 1;
    }

    fprintf(stderr, "%d\n", *(int*)p);

    shmdt(p);

    return 0;
}
