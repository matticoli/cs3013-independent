/* doit.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char *shellPrompt = "-->";

int forkme(char **argv);


int main(int argc, char **argv) {
    if(argc > 1) { 
        return forkme(argv + 1);
    } else {
        // No args, run doit shell
        char *input;
        while(input != "exit") {
            // Reinit input str
            input = malloc(sizeof(char)*128);

            if(feof(stdin)) {
                printf("EOF: doit shell will now terminate\n\n");
                exit(0);
            }

            // Get user input
            printf("%s", shellPrompt);
            fgets(input, 128, stdin);

            // Reinit argvIn array
            char **argvIn = malloc(sizeof(char) * 128);
            
            // Tokenize input
            char *tok = strtok(input, " \n");
            int argNum = 0;

            while(tok != NULL && tok != "\n") {
                argvIn[argNum++] = tok;
                tok = strtok(NULL, " \n");
            }
            argvIn[argNum] = NULL;

            // printf("%s\n",argvIn[0]);

            // Handle custom commands
            if(argNum == 0) {
                printf("No command entered\n");
                continue;
            } else if(strcmp(argvIn[0], "exit") == 0) {
                printf("Exit: doit shell will now terminate\n\n");
                exit(0);
            } else if(strcmp(argvIn[0], "cd") == 0) {
                if(argNum == 1) {
                    char *dir = malloc(sizeof(char) * 128);
                    getcwd(dir, 128);
                    printf("Current dir: %s\n", dir);
                } else if(chdir(argvIn[1]) + 1) {
                    char *dir = malloc(sizeof(char) * 128);
                    getcwd(dir, 128);
                    printf("Current dir: %s\n", dir);
                } else {
                    printf("Error changing directory\n");
                }
                continue;
            } else if(argNum == 4 && 
                strcmp(argvIn[0], "set") == 0 &&
                strcmp(argvIn[1], "prompt") == 0 &&
                strcmp(argvIn[2], "=") == 0) {
                shellPrompt = argvIn[3];
            }

            forkme(argvIn);
        }

    }
    return 0;
}

int forkme(char **argv) {
    int pid;// get pid of subprocess

    // Get start time just before fork
    struct timeval *start = malloc(sizeof(struct timeval));
    gettimeofday(start, 0);

    // Command specified in args- fork and run
    if ((pid = fork()) < 0) { // negative pid = error forking
        fprintf(stderr, "Fork error\n");
        exit(1);
    } else if (pid == 0) {
        /* child process */
        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "Execvp error\n");
            exit(1);
        }
    } else {
        
        /* parent process */
        wait(0);     /* wait for the child to finish */

        // Get end time
        struct timeval *end = malloc(sizeof(struct timeval));
        gettimeofday(end, 0);

        // Get runtime and convert to milliseconds
        int runtime = ((end->tv_sec*1000) + (end->tv_usec/1000)) - 
                        ((start->tv_sec*1000) + (start->tv_usec/1000));

        // Get usage stats
        struct rusage *usage = malloc(sizeof(struct rusage));
        getrusage(RUSAGE_CHILDREN, usage);

        // Convert CPU times to ms
        long ucpu = (((usage->ru_utime).tv_sec*1000) + ((usage->ru_utime).tv_usec/1000));
        long scpu = (((usage->ru_stime).tv_sec*1000) + ((usage->ru_stime).tv_usec/1000));

        printf("======== PROCESS STATS ========\n");
        printf("User CPU TIME:            %ldms\n", ucpu);
        printf("Sys CPU TIME:             %ldms\n", scpu);
        printf("Wall Clock Time:          %dms\n", runtime);
        printf("Involuntary Preemptions:  %d\n", usage->ru_nivcsw);
        printf("Voluntary CPU Loss:       %ld\n", usage->ru_nvcsw);
        printf("Major Page Faults:        %ld\n", usage->ru_majflt);
        printf("Minor Page Faults:        %ld\n", usage->ru_minflt);
        printf("Max Resident Set Size:    %ld\n", usage->ru_maxrss);
    }
    return 0;
}
