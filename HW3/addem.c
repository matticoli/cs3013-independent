#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>


#define DEBUG 1

// Max thread count
#define MAXTHREAD 10
// Main thread
#define MAIN 0
// Message types
#define RANGE 1
#define ALLDONE 2

typedef struct msg {
    int iSender;
    int type;
    int value1;
    int value2;
} Msg;

// Global Vars
pthread_t *threads; // Array of threads
sem_t *psems;
sem_t *csems;
Msg *mailboxes;

void ploop(int numToSum, int numThreads);
void cloop();
void SendMessage(int iTo, Msg *msg);
void RecvMessage(int iFrom, Msg *msg);


int main(int argc, char *argv) {
    if(argc < 3) {
        printf("Adds numbers 1 through numToSum using numThreads worker threads (max 10)\nUsage:\naddem <numThreads> <numToSum>\n");
        exit(1);
    }
    int numThreads = atoi(argv[1]);
    int numToSum = atoi(argv[2]);
    if(numThreads < 1 || numThreads > 10) {
        printf("Must have between 1 and 10 threads. Run addem without parameters for command syntax\n");
        exit(1);
    } else if (numToSum < 2) {
        printf("Choose a number to sum to that is greater than 1. Run addem without parameters for command syntax\n");
        exit(1);
    }

    // Malloc stuffs
    threads = malloc(sizeof(int) * (numThreads + 1));
    psems = malloc(sizeof(sem_t) * (numThreads + 1));
    csems = malloc(sizeof(sem_t) * (numThreads + 1));
    mailboxes = malloc(sizeof(Msg) * (numThreads + 1));

    int i;
    for(i = 0; i < numThreads + 1; i++) {
        sem_init(&psems[i], 0, 1);
        sem_init(&csems[i], 0, 0);
        if(i > 0) {
            threads[i] = i;
            if(pthread_create(&threads[i], NULL, (void*)&cloop, (void*)i )) {
                printf("[0] Error creating thread %d\n", i);
            }
        }
    }
    
    ploop(numToSum, numThreads);

    for(i = 0; i < numThreads + 1; i++) {
        if(i > 0) {
            (void)pthread_join(threads[i], NULL);
            #if DEBUG
                 printf("[0] Thread %d done\n", i);
            #endif
        }
        (void)sem_destroy(&psems[i]);
        (void)sem_destroy(&csems[i]);
    }

    return 0;
}

// Producer loop (main thread)- this function is actually pretty unnecessary since there is no loop
// but I really wanted to have 2 functions and name them "ploop" and "cloop"
void ploop(int numToSum, int numThreads) {
    int i;
    // Evenly divide numebrs to add between threads
    int chunkSize = numToSum / numThreads;
    for(i = 1; i <= numThreads; i++) {
        // If this is the last thread, makes sure to include remainder as part of last chunk
        int MAX = (i == numThreads ? numToSum + 1 : i * chunkSize);
        Msg *sumMsg = malloc(sizeof(Msg));
        sumMsg->iSender = 0;
        sumMsg->type = RANGE;
        sumMsg->value1 = (i - 1)*chunkSize;
        sumMsg->value2 = MAX;
        SendMessage(i, sumMsg);
    }

    int sum = 0;
    for(i = 1; i <= numThreads; i++) {
        sem_wait(&csems[0]);
        sum += mailboxes[0].value1;
        sem_post(&psems[0]);
    }
    printf("The total from %d to %d using %d threads is %d\n\r", 1, numToSum, numThreads, sum);
}

// Consumer loop (worker thread logic)
void cloop(int index) {
    #if DEBUG
        printf("[%d] Thread %d started\n",index, index);
    #endif
    Msg *msg = malloc(sizeof(msg));
    RecvMessage(index, msg);
    #if DEBUG
        printf("[%d] Message contents: RANGE(%d,%d)\n", index, msg->value1, msg->value2);
    #endif
    int i, c = 0;
    for(i = msg->value1; i < msg->value2; i++) {
        c += i;
    }
    #if DEBUG
        printf("[%d] Sum from %d to %d is %d\n", index, msg->value1, msg->value2, c);
    #endif
    Msg *sumMsg =  malloc(sizeof(msg));
    sumMsg->iSender = index;
    sumMsg->type = ALLDONE;
    sumMsg->value1 = c;
    SendMessage(0, sumMsg);
}

void msgCpy(Msg *from, Msg *to) {
    to->iSender = from->iSender;
    to->type = from->type;
    to->value1 = from->value1;
    to->value2 = from->value2;
}

void SendMessage(int iTo, Msg *msg) {
    #if DEBUG
        printf("[%d] Sending message to  %d\n", msg->iSender, iTo);
    #endif
    // TODO: shallow copy may not work here, if you get a segfault it's prob here
    sem_wait(&psems[iTo]);
    msgCpy(msg, &mailboxes[iTo]);
    sem_post(&csems[iTo]);
}

void RecvMessage(int iFrom, Msg *msg) {
    #if DEBUG
        printf("[%d] Message received from %d\n", iFrom, msg->iSender);
    #endif
    sem_wait(&csems[iFrom]);
    msgCpy(&mailboxes[iFrom], msg);
    sem_post(&psems[iFrom]);
}
