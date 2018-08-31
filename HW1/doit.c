/* doit.c 
 * CS3013-A18 HW1
 * Mikel Matticoli
 *
 * This program will function similarly to a standard bash shell, but will
 * print out stats for all executed commands. When run with arguments, the
 * arguments will be run as a command and stats will be printed, then doit
 * will terminate.
 * Key limitations:
 *  This shell does not support piping or chaining commands in any way, as
 * each inputted command is being executed as its own process. Commands
 * that use environment variables also may not function correctly.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char *shellPrompt = "-->";
int *jobs;
char **jobNames;
int numJobs;

int forkme(char **argv, int async);


int main(int argc, char **argv) {
    if(argc > 1) { 
        return forkme(argv + 1, 0);
    } else {
        // No args, run doit shell

        // Init jobs array
        jobs = calloc(32, sizeof(int));
        jobNames = calloc(32, sizeof(char[128]));
        numJobs = 0;

        char *input;
        while(1) {
            // Reinit input str
            input = malloc(sizeof(char)*128);

            if(feof(stdin)) {
                if(numJobs) {
                    printf("Cannot exit while jobs are active.\nRun `jobs` to view active processes, or run any command to recheck for completion.\n");
                    continue;
                }
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
                if(numJobs) {
                    printf("Cannot exit while jobs are active.\nRun `jobs` to view active processes, or run any command to recheck for completion.\n");
                    continue;
                }
                printf("Exit: doit shell will now terminate\n\n");
                exit(0);
            }else if(strcmp(argvIn[0], "jobs") == 0) {
                if(!numJobs) {
                    printf("No active jobs.\n");
                    continue;
                }
                int j;
                printf("Active Jobs:\n");
                for(j = 0; j < numJobs; j++) {
                    printf("[%d] %d %s\n", j+1, jobs[j], jobNames[j]);
                }
                continue;

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
                continue;
            }

            if(strcmp(argvIn[argNum - 1], "&") == 0) {
                argvIn[argNum - 1] = NULL;
                forkme(argvIn, 1);
            } else {
                forkme(argvIn, 0);
            }
        }

    }
    return 0;
}

int forkme(char **argv, int async) {
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
            fprintf(stderr, "Execvp error - could not run command %s\n", argv[0]);
            exit(1);
        }
    } else {
        
        /* parent process */
        if(async) {
            int i = 0;
            while(jobs[i]) {
                i++;
            }
            jobs[i] = pid;
            jobNames[i] = argv[0];
            numJobs++;
            printf("[%d] %d\n", i+1, pid);
            return 0;
        } else {
            while( (pid = wait(0)) + 1) {     
            /* wait for the child to finish or get last finished async child */
                // Remove finished jobs from list:
                char *command = argv[0];

                int z;
                for(z = 0; z < numJobs; z++) {
                    if(jobs[z] == pid || jobs[z] == jobs[z] - 1) {
                        if(jobs[z] == pid) {
                            numJobs--;
                        }
                        jobs[z] = jobs[z + 1];
                        jobNames[z] = jobNames[z + 1];
                    }
                }

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

                printf("======== PID %d STATS (%s) ========\n", pid, command);
                printf("User CPU TIME:            %ldms\n", ucpu);
                printf("Sys CPU TIME:             %ldms\n", scpu);
                printf("Wall Clock Time:          %dms\n", runtime);
                printf("Involuntary Preemptions:  %d\n", usage->ru_nivcsw);
                printf("Voluntary CPU Loss:       %ld\n", usage->ru_nvcsw);
                printf("Major Page Faults:        %ld\n", usage->ru_majflt);
                printf("Minor Page Faults:        %ld\n", usage->ru_minflt);
                printf("Max Resident Set Size:    %ld\n", usage->ru_maxrss);
            }
        }
    }
    return 0;
}
