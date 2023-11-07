#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#define MAX_PROCESSES 18

typedef struct {
    unsigned int seconds;
    unsigned int nanoseconds;
} systemClock;

int main() {
    int shm_id;
    systemClock *clock;

    // Create a shared memory segment
    shm_id = shmget(SHM_KEY, sizeof(systemClock), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget");
        exit(1);
    }

    // Attach the shared memory segment
    clock = (systemClock *)shmat(shm_id, NULL, 0);
    if (clock == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    // Initialize the clock
    clock->seconds = 0;
    clock->nanoseconds = 0;








    if (shmdt(clock) == -1) {
        perror("shmdt");
        exit(1);
    }

    return 0;
}
