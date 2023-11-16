#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "resource.h"

#define REQUEST_RESOURCE 1
#define RELEASE_RESOURCE 2
#define GRANT_REQUEST 3

typedef struct {
    unsigned int seconds;
    unsigned int nanoseconds;
} systemClock;

typedef struct {
    long mtype;             // Message type
    int pid;                // Process ID
    int resourceType;       // Resource type being requested or released
    int resourceCount;      // Number of instances of the resource
    int action;             // Action type (e.g., request = 1, release = 2)
} Message;

void incrementClock(systemClock* clock, unsigned int seconds, unsigned int nanoseconds);
void initializeResourceTable(ResourceDescriptor* resourceTable);

int main(int argc, char *argv[]) {

    int opt;
    int n = 1;
    int s = 1;
    int t = 1000000;
    char *f = "logfile.txt";


    while ((opt = getopt(argc, argv, "hn:s:t:f:")) != -1) {
            switch (opt) {
                   case 'h':
                        printf("Usage: %s [-h] [-n proc] [-s simul] [-t timeToLaunchNewChild] [-f logfile]\n", argv[0]);
                        exit(EXIT_SUCCESS);
                        break;
                   case 'n':
                        n = atoi(optarg);
                        break;
                   case 's':
                        s = atoi(optarg);
                        break;
                   case 't':
                        t = atoi(optarg);
                        break;
                   case 'f':
                        f = optarg;
                        break;
                   default:
                        fprintf(stderr, "Usage: %s [-h] [-n proc] [-s simul] [-t timeToLaunchNewChild] [-f logfile]\n", argv[0]);
                        exit(EXIT_FAILURE);
                        break;
                }
        }

     int shmID;
    int shmRID;
    int msgID;
    Message msg;
    key_t cKey = 90131;
    key_t rKey = 34982;
    key_t mKey = 77432;
    ResourceDescriptor *resourceTable;
    systemClock *clock;


    // Create a shared memory segment
    shmID = shmget(cKey, sizeof(systemClock), IPC_CREAT | 0666);
    if (shmID < 0) {
        perror("shmget");
        exit(1);
    }

    // Attach the shared memory segment
    clock = (systemClock *)shmat(shmID, NULL, 0);
    if (clock == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    shmRID = shmget(rKey, sizeof(ResourceDescriptor) * NUM_RESOURCES, IPC_CREAT | 0666);
    if (shmRID < 0) {
            perror("shmget");
            exit(1);
    }
    resourceTable = (ResourceDescriptor *)shmat(shmRID, NULL, 0);
    if (resourceTable == (void *)-1) {
            perror("shmat");
            exit(1);
    }

    msgID = msgget(mKey, 0666 | IPC_CREAT);
    if (msgID == -1) {
        perror("msgget");
        exit(1);
    }

    clock->seconds = 0;
    clock->nanoseconds = 0;

    initializeResourceTable(resourceTable);

    pid_t forkPid;
    systemClock nextLaunch = {0, t};
    int activeChildren = 0;
    int childrenLaunched = 0;


    while (childrenLaunched < n || activeChildren > 0) {

        incrementClock(clock, 0, 10);

        pid_t pid;
        int status;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                activeChildren--;
        }


        if (msgrcv(msgID, &msg, sizeof(msg), 0, IPC_NOWAIT) != -1) {
                // Process the message
                if (msg.action == REQUEST_RESOURCE) {
                        if (resourceTable[msg.resourceType].availableInstances > 0) {
                                resourceTable[msg.resourceType].availableInstances--;

                                // Find an unallocated instance and mark it as allocated to msg.pid
                                for (int i = 0; i < NUM_INSTANCES; i++) {
                                        if (resourceTable[msg.resourceType].instances[i].allocatedToProcess == -1) {
                                        resourceTable[msg.resourceType].instances[i].allocatedToProcess = msg.pid;
                                        break; // Exit the loop once an instance is allocated
                                        }
                        }

                } else {
                        // Resource not available, handle accordingly
                // ...
                }

                // Send response back to user_proc indicating request granted
                } else if (msg.action == RELEASE_RESOURCE) {
                        // Handle resource release
                        resourceTable[msg.resourceType].availableInstances++;
                        resourceTable[msg.resourceType].instances[msg.resourceType].allocatedToProcess[msg.pid] = -1;
                }
                // Other types of messages
        }

        if ((clock->seconds > nextLaunch.seconds ||
            (clock->seconds == nextLaunch.seconds && clock->nanoseconds >= nextLaunch.nanoseconds)) &&
            activeChildren < s) {

            forkPid = fork();
            if (forkPid < 0) {
                perror("Fork failed");
                exit(1);
            } else if (forkPid == 0) {
                // In child process
                execl("./user_proc", "./user_proc", NULL);
                perror("execl failed");
                exit(1);
            } else {
                // In parent process
                childrenLaunched++;
                activeChildren++;
                incrementClock(&nextLaunch, 0, t);
            }
        }
    }


    if (shmdt(clock) == -1) {
        perror("shmdt");
        exit(1);
    }

    if (shmdt(resourceTable) == -1) {
        perror("shmdt");
        exit(1);
    }

    if (msgctl(msgID, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
}
    return 0;
}


void incrementClock(systemClock* clock, unsigned int seconds, unsigned int nanoseconds) {
        clock->nanoseconds += nanoseconds;

        while (clock->nanoseconds >= 1000000000) {
                clock->seconds += 1;
                clock->nanoseconds -= 1000000000;
        }
        clock->seconds += seconds;
}

void initializeResourceTable(ResourceDescriptor* resourceTable) {
    for (int i = 0; i < NUM_RESOURCES; i++) {
        resourceTable[i].resourceID = i;
        resourceTable[i].totalInstances = NUM_INSTANCES;
        resourceTable[i].availableInstances = NUM_INSTANCES;
        for (int j = 0; j < NUM_INSTANCES; j++) {
            for (int k = 0; k < MAX_PROCESSES; k++) {
                resourceTable[i].instances[j].allocatedToProcess[k] = -1;
            }
        }
    }
}


   

