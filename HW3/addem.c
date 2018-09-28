#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

// Debug mode: 1 for Thread/Message tracking, 2 to print semaphore increments/decrements
#define DEBUG 0

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


int main(int argc, char **argv) {
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
    threads = calloc(sizeof(int), (numThreads + 1));
    psems = calloc(sizeof(sem_t), (numThreads + 1));
    csems = calloc(sizeof(sem_t), (numThreads + 1));
    mailboxes = calloc(sizeof(Msg), (numThreads + 1));

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

    #define STUPID_SEM_ISSUE 0

    #if !STUPID_SEM_ISSUE
    /* For some obscure unknown magical reason, this semaphore initializes to a random garbage value in the for loop despite
    sem_init returning 0. If you don't belive me, change STUPID_SEM_ISSUE to 1 */
    sem_init(&psems[0], 0, 1);
    #endif

    #if STUPID_SEM_ISSUE
        int val;
        sem_getvalue(&psems[0], &val);
        printf("INIT psem %d to val %d in thread %d\n", 0, val, 0);
        exit(0);
    #endif

    ploop(numToSum, numThreads);

    for(i = 0; i < numThreads; i++) {
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
    for(i = 0; i < numThreads; i++) {
        Msg *sumMsg = malloc(sizeof(Msg));
        RecvMessage(0, sumMsg);
        #if DEBUG
            printf("[0] Message sender %d type %d contents %d\n",sumMsg->iSender, sumMsg->type, sumMsg->value1);
        #endif
        sum += sumMsg->value1;
        // free(sumMsg);
    }
    printf("The total from %d to %d using %d threads is %d\n\r", 1, numToSum, numThreads, sum);
    fflush(stdout);
}

// Consumer loop (worker thread logic)
void cloop(int index) {
    #if DEBUG
        printf("[%d] Thread %d started\n",index, index);
    #endif
    Msg *msg = malloc(sizeof(Msg));
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
    Msg *sumMsg =  malloc(sizeof(Msg));
    sumMsg->iSender = index;
    sumMsg->type = ALLDONE;
    sumMsg->value1 = c;
    sumMsg->value2 = 0;
    SendMessage(0, sumMsg);
}

void msgCpy(Msg *from, Msg *to) {
    to->iSender = from->iSender;
    to->type = from->type;
    to->value1 = from->value1;
    to->value2 = from->value2;
    from->value1 = 0;
    from->value2 = 0;
}

void SendMessage(int iTo, Msg *msg) {
    #if DEBUG
        printf("[%d] Sending message to  %d\n", msg->iSender, iTo);
    #endif

    if(sem_wait(&psems[iTo])) {
        printf("WAIT ERROR\n");
        return;
    }

    int val;
    sem_getvalue(&psems[iTo], &val);
    #if DEBUG == 2
        printf("Decremented psem %d to val %d in thread %d\n", iTo, val, msg->iSender);
    #endif

    msgCpy(msg, &mailboxes[iTo]);
    sem_post(&csems[iTo]);

    val = 0;
    sem_getvalue(&csems[iTo], &val);
    #if DEBUG == 2
        printf("Incremented csem %d to val %d in thread %d\n", iTo, val, msg->iSender);
    #endif
}

void RecvMessage(int iFrom, Msg *msg) {
    if(!!sem_wait(&csems[iFrom])) {
        printf("WAIT ERROR\n");
        return;
    }//TODO Something to do with for loop?
    msgCpy(&mailboxes[iFrom], msg);
    sem_post(&psems[iFrom]);
    int val;
    sem_getvalue(&psems[iFrom], &val);
    #if DEBUG == 2
        printf("Incremented psem %d to val %d in thread %d\n", iFrom, val, iFrom);
    #endif

    #if DEBUG
        printf("[%d] Message received from %d\nValues (%d, %d)\n", iFrom, msg->iSender, msg->value1, msg->value2);
    #endif
}
