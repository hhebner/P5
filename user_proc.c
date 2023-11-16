#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "resource.h"

#define RANDOM_TERMINATION_CHANCE 0.1  
#define MIN_RUNTIME 1000000000         
#define CHECK_INTERVAL 250000
#define RELEASE_RESOURCE 2
#define REQUEST 1
#define GRANT_REQUEST 3

typedef struct {
    unsigned int seconds;
    unsigned int nanoseconds;
} systemClock;

typedef struct {
    long mtype;             
    int pid;                
    int resourceType;       
    int resourceCount;      
    int action;             
} Message;

int main(int argc, char *argv[]) {

    int shmID;
    int shmRID;
    int msgID;
    Message msg;
    key_t cKey = 90131;
    key_t rKey = 34982;
    key_t mKey = 77432;
    ResourceDescriptor *resourceTable;
    systemClock *clock;

    int resourcesHeld[NUM_RESOURCES] = {0};

    
    shmID = shmget(cKey, sizeof(systemClock), 0666);
    if (shmID < 0) {
        perror("shmget (clock)");
        exit(1);
    }
    clock = (systemClock *)shmat(shmID, NULL, 0);
    if (clock == (void *)-1) {
        perror("shmat (clock)");
        exit(1);
    }

    
    shmRID = shmget(rKey, sizeof(ResourceDescriptor) * NUM_RESOURCES, 0666);
    if (shmRID < 0) {
        perror("shmget (resource table)");
        exit(1);
    }
    resourceTable = (ResourceDescriptor *)shmat(shmRID, NULL, 0);
    if (resourceTable == (void *)-1) {
        perror("shmat (resource table)");
        exit(1);
    }

    
    msgID = msgget(mKey, 0666);
    if (msgID == -1) {
        perror("msgget");
        exit(1);
    }

    unsigned int startSeconds = clock->seconds;
    unsigned int startNanoseconds = clock->nanoseconds;
    srand(time(NULL) + getpid());

    while (1) {
        usleep(CHECK_INTERVAL);  

        unsigned int currentSeconds = clock->seconds;
        unsigned int currentNanoseconds = clock->nanoseconds;
        unsigned int elapsed = (currentSeconds - startSeconds) * 1000000000 + (currentNanoseconds - startNanoseconds);

        if (elapsed > MIN_RUNTIME) {
            
            if ((double)rand() / RAND_MAX < RANDOM_TERMINATION_CHANCE) {
                
                Message msg;
                msg.mtype = 1;
                msg.pid = getpid();

                for (int i = 0; i < NUM_RESOURCES; i++) {
                        
                        if (resourceTable[i].allocatedToProcess[pid] > 0) {
                                msg.resourceType = i; 
                                msg.resourceCount = resourceTable[i].allocatedToProcess[pid];
                                msg.action = RELEASE_RESOURCE;  

                                // Send the message
                                if (msgsnd(msgID, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                                        perror("msgsnd failed");
                                }

                                resourceTable[i].allocatedToProcess[pid] = 0;
                        }
                }
                break; 
            }
        }

        
        int decision = rand() % 10; 

        Message msg;
        msg.mtype = 1;
        msg.pid = getpid();

        if (decision < 8) { 
                
                int resourceType = rand() % NUM_RESOURCES;

                
                if (resourcesHeld[resourceType] < NUM_INSTANCES) {
                        msg.action = REQUEST_RESOURCE;
                        msg.resourceType = resourceType;
                        msg.resourceCount = 1;

                        
                        if (msgsnd(msgID, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                                perror("msgsnd failed");
                        }

                        
                        if (msgrcv(msgID, &msg, sizeof(msg), getpid(), 0) != -1) {
                                
                                if (msg.action == GRANT_REQUEST) { 
                                resourcesHeld[resourceType] += 1;
                                }
                        }


                }
        } else if (decision >= 8) { 
                
                int resourceType = rand() % NUM_RESOURCES;

                
                if (resourcesHeld[resourceType] > 0) {
                        msg.action = RELEASE_RESOURCE;
                        msg.resourceType = resourceType;
                        msg.resourceCount = 1;

                
                if (msgsnd(msgID, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                        perror("msgsnd failed");
                }

                
                resourcesHeld[resourceType] -= 1;
                }
        }
    }

    if (shmdt(clock) == -1) {
        perror("shmdt (clock)");
        }

    if (shmdt(resourceTable) == -1) {
        perror("shmdt (resourceTable)");
        }

    return 0;
}




