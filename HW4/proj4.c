// Demonstrates how to use mmap
//
// Modified by Craig Wills from:
// Copyright (c) Michael Still, released under the terms of the GNU GPL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

// WARNING DO NOT ENABLE THIS FLAG IT WON'T COMPILE
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
char *searchStr;
char *pchFile;
int fsize;

void ploop(int numThreads);
void cloop(int index);
void SendMessage(int iTo, Msg *msg);
void RecvMessage(int iFrom, Msg *msg);

int main(int argc, char *argv[]){
    int fd;
    struct stat sb;

    if(argc < 4) {
        fprintf(stderr, "Usage: %s <inputFile> <searchString> [size|mmap|pn]\n", argv[0]);
        exit(1);
    }

    // Open file
    if ((fd = open (argv[1], O_RDONLY)) < 0) {
        perror("Could not open file");
        exit (1);
    }

    searchStr = argv[2];

    if(!strcmp(argv[3], "mmap")) {
        /* Map the input file into memory */

        if(fstat(fd, &sb) < 0){
            perror("Could not stat file to obtain its size");
            exit(1);
        }
        fsize = sb.st_size;

        if ((pchFile = (char *) mmap (NULL, fsize, PROT_READ, MAP_SHARED, fd, 0)) 
                == (char *) -1)	{
            perror("Could not mmap file");
            exit (1);
        }

        // pchFile is now a pointer to the entire contents of the file...
        // do processing of the file contents here
        int i, match = 0, numMatches = 0;
        for(i = 0; i < fsize; i++) {
            if(pchFile[i] == searchStr[match]) {
                match ++;
                if(match == strlen(searchStr)) {
                    numMatches++;
                    match = 0;
                }
            } else {
                match = 0;
            }
        }

        printf("Filesize: %d bytes\n", fsize);
        printf("Occurences of \"%s\": %d\n", searchStr, numMatches);

        // Now clean up
        if(munmap(pchFile, sb.st_size) < 0){
            perror("Could not unmap memory");
            exit(1);
        }
    } else if (argv[3][0] == 'p') {
        int numThreads = atoi(argv[3] + 1);
        if(numThreads < 1 || numThreads > 16) {
            perror("Must enter between 1 and 16 threads");
            exit(1);
        }

        // MMap
        /* Map the input file into memory */
        if(fstat(fd, &sb) < 0){
            perror("Could not stat file to obtain its size");
            exit(1);
        }
        fsize = sb.st_size;

        if ((pchFile = (char *) mmap (NULL, fsize, PROT_READ, MAP_SHARED, fd, 0)) 
                == (char *) -1)	{
            perror("Could not mmap file");
            exit (1);
        }// TODO: Remove Dupe Code

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

        ploop(numThreads);

        for(i = 0; i < numThreads; i++) {
            (void)sem_destroy(&psems[i]);
            (void)sem_destroy(&csems[i]);
        }

    


    } else {
        fflush(stdout);
        int size;
        int match = 0;
        int numMatches = 0;
        if((size = atoi(argv[3])) <= 0) {
            perror("Invalid size, enter a size, mmap, or p<# threads>");
            fprintf(stderr, "Usage: %s <inputFile> <searchString> [size|mmap|pn]\n", argv[0]);
            exit(1);
        }

        if(size > 8192) {
            perror("Size must be at most 8K (8192) bytes");
            exit(1);
        }
        // use read()

        char *buf = malloc(sizeof(size));
        int actualSize;
        while((actualSize = read(fd, buf, size)) > 0) { // Not EOF
            printf("Tried to read chunk of size size %d\n", size);
            printf("Read in chunk of size %d\n", actualSize);
            fsize += actualSize;
            int i;
            for(i = 0; i < actualSize; i++) {
                if(buf[i] == searchStr[match]) {
                    match ++;
                    if(match == strlen(searchStr)) {
                        numMatches++;
                        match = 0;
                    }
                } else {
                    match = 0;
                }
            }
        }

        printf("Filesize: %d bytes\n", fsize);
        printf("Occurences of \"%s\": %d\n", searchStr, numMatches);

    }
    


    close(fd);
}


void ploop(int numThreads) {
    int i;
    // Evenly divide numebrs to add between threads
    int chunkSize = fsize / numThreads;
    for(i = 1; i <= numThreads; i++) {
        // If this is the last thread, makes sure to include remainder as part of last chunk
        int MAX = (i == numThreads ? fsize + 1 : i * chunkSize);
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
    
    printf("Filesize: %d bytes\n", fsize);
    printf("Occurences of \"%s\": %d\n", searchStr, sum);
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

    // do processing of the file contents here
    int i, match = 0, numMatches = 0;
    for(i = msg->value1; (i < (msg->value2) || match); i++) {
        if(pchFile[i] == searchStr[match]) {
            match ++;
            if(match == strlen(searchStr)) {
                numMatches++;
                match = 0;
            }
        } else {
            match = 0;
        }
    }    

    #if DEBUG
        printf("[%d] Sum from %d to %d is %d\n", index, msg->value1, msg->value2, numMatches);
    #endif
    Msg *sumMsg =  malloc(sizeof(Msg));
    sumMsg->iSender = index;
    sumMsg->type = ALLDONE;
    sumMsg->value1 = numMatches;
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
