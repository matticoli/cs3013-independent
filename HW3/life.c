/*
	CS3013 - A18 - Distributed Game Of Life
	Mikel Matticoli
	Some of the logic is recycled from the CS2303 Game of Life Project

	Original project:
	https://github.com/matticoli/CS2303/tree/master/assignment2 
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "twoD.h"


#define _DEBUG 0
// Max thread count
#define MAXTHREAD 10
// Main thread
#define MAIN 0
// Message types
#define RANGE 1
#define ALLDONE 2
#define GO 3
#define GENDONE 4

// Message struct
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

char **gridA, **gridB, **gridC; // A 2D array to hold the pattern
int rows, columns; 

// Function declarations
void ploop(int numThreads, int gen);
void cloop(int index);
void SendMessage(int iTo, Msg *msg);
void RecvMessage(int iFrom, Msg *msg);
int getAdjecentLifeCount(char **grid, int rows, int cols, int x, int y);
void playOne (int startRow, int endRow, char **old, char **new);
char **loadStartGrid(FILE *input, char **dest, int *rows, int *columns);


/** Main function.
 * @param argc Number of words on the command line.
 * @param argv Array of pointers to character strings containing the
 *    words on the command line.
 * @return 0 if success, 1 if invalid command line or unable to open file.
 *
 */
int main(int argc, char **argv) {
	printf("============\nGame of Life\n============\n");
	char *inputFileName; // Name of file containing initial grid
	FILE *input; // Stream descriptor for file containing initial grid
	int numThreads; // Number of worker threads
	int gens; // Number of generations to produce
	int doPrint = 0; // 1 if user wants to print each generation, 0 if not
	int doPause = 0; // 1 if user wants to pause after each generation, 0 if not

	// See if there are the right number of arguments on the command line
	if ((argc < 4) || (argc > 7)) {
		// If not, tell the user what to enter.
		printf("Usage:\n");
		printf("  ./life #threads inputFile #generations [print (y/n)] [pause (y/n)]\n");
		return EXIT_FAILURE;
	}

	/* Save the command-line arguments.
	   Also check if print and/or pause arguments were entered,
	   and if so, what they were.
	*/
	switch(argc) {
		case 6:
			// Set doPause to 1 if y entered, else 0
			doPause = argv[5][0] == 'y';
		case 5:
			// Set doPrint to 1 if y entered, else 0
			doPrint = argv[4][0] == 'y';
		default:
			numThreads = atoi(argv[1]); // Convert from character string to integer.
			gens = atoi(argv[3]);
			inputFileName = argv[2];
	}


	// If generations input is zero, negative, or invalid
	if(gens <= 0) {
		printf("Nothing to do - generations argument must be >= 0\n");
		return EXIT_FAILURE;
	}

	// If generations input is zero, negative, or invalid
	if(numThreads <= 0 || numThreads > 10) {
		printf("Invalid thread count (must be between 1 and 10)\n");
		return EXIT_FAILURE;
	}

	// Open the input file (in read mode)
	input = fopen(inputFileName, "r");
	if (!input) {
		printf("Unable to open input file: %s\n", inputFileName);
		return EXIT_FAILURE;
	}

	#if _DEBUG
		printf("Loading Start Grid\n");
	#endif

	// Load the starting grid from input file into 2D char array
	gridA = loadStartGrid(input, gridA, &rows, &columns);

	#if _DEBUG
		printf("Load success with dimensions (%d, %d)\n", rows, columns);
	#endif

	if(!rows || !columns) {
		printf("Error: Input grid must be at least 1x1\n");
		return EXIT_FAILURE;
	}

	if(numThreads > rows) {
		printf("More threads than input rows specified, using %d threads instead\n", rows);
		numThreads = rows;
	}

	// Initialize the 2D arrays for current, previous, and pre-previous gens
	gridB = make2Dchar(rows, columns);
	gridC = make2Dchar(rows, columns);

	// Print the start state
	printf("Gen 1 (Start):\n");
	print2Dchar(gridA, rows, columns);
	fflush(stdout);

	// Malloc stuffs
    threads = calloc(sizeof(pthread_t), (numThreads + 1));
    psems = calloc(sizeof(sem_t), (numThreads + 1));
    csems = calloc(sizeof(sem_t), (numThreads + 1));
    mailboxes = calloc(sizeof(Msg), (numThreads + 1));


	int i;
    for(i = 0; i <= numThreads; i++) {
        sem_init(&psems[i], 0, 1);
        sem_init(&csems[i], 0, 0);
        if(i > 0) {
            threads[i] = i;
            if(pthread_create(&threads[i], NULL, (void*)&cloop, (void*)i )) {
                printf("[0] Error creating thread %d\n", i);
            }
        }
    }

	// Run simulation
	int gen;
	for(gen = 0; gen < gens; gen++) {
		if(gen > 0) {
			// Remove pre-previous grid from memory
			free2Dchar(gridC, rows);
			// Shift the current and previous back, and init new current grid
			gridC = gridB;
			gridB = gridA;
			gridA = make2Dchar(rows, columns);
		}
		// Update gridA (new current) based on gridB (previous)
		ploop(numThreads, gen);
		if(gen == 0) {
			continue;
		}
		// playOne(0, rows, gridB, gridA);
		// If doPrint arg was 'y'
		if(doPrint || gen == gens-1) {
			// Print out the current generation
			printf("\nGen %d:\n", gen + 1);
			print2Dchar(gridA, rows, columns);
		}
		#if _DEBUG
			printf("[0] Checking end conditions\n");
		#endif
		// If current matches previous (steady state)
		if(match2Dchar(gridA, gridB, rows, columns)) {
			// Print out the final generation if it hasn't been already
			if(!doPrint && gen < gens - 1) {
				printf("\nGen %d:\n", gen + 1);
				print2Dchar(gridA, rows, columns);
			}
			// Print result and terminate
			printf("Reached steady state after %d generations\n", gen + 1);
			return EXIT_SUCCESS;
		// If current matches pre-previous (oscilation state)
		} else if(match2Dchar(gridA, gridC, rows, columns)) {
			// Print out the final generation if it hasn't been already
			if(!doPrint && gen < gens - 1) {
				printf("\nGen %d:\n", gen + 1);
				print2Dchar(gridA, rows, columns);
			}
			// Print result and terminate
			printf("Reached oscilation state after %d generations\n", gen + 1);
			return EXIT_SUCCESS;
		}
		#if _DEBUG
			printf("[0] PAUSE %d\n", i);
		#endif

		// If doPause arg was 'y', wait for user input before continuing
		if(doPause) {
			// If pausing but not printing
			if(!doPrint) {
				// Print out gen number so the program doesn't look frozen
				printf("\nGen %d:\n", gen + 1);
			}
			// Wait for user input
			getchar();
		}
	}
	// Whoopee we made it
	return EXIT_SUCCESS;
}

// Producer loop (main thread)- this function is actually pretty unnecessary since there is no loop
// but I really wanted to have 2 functions and name them "ploop" and "cloop"
/*!
   Runs one generation
   @param numThreads number of threads being used
   @param gen generation index
*/
void ploop(int numThreads, int gen) {
    int i;

	if(gen == 0) {
		// Evenly divide numebrs to add between threads
		#if _DEBUG
			printf("Dividing %d by %d\n", rows, numThreads);
		#endif
		int chunkSize = rows / numThreads;
		for(i = 1; i <= numThreads; i++) {
			// If this is the last thread, makes sure to include remainder as part of last chunk
			int MAX = (i == numThreads ? rows : i * chunkSize);
			#if _DEBUG
				printf("[0] Sending range to %d\n", i);
			#endif
			Msg *sumMsg = malloc(sizeof(Msg));
			sumMsg->iSender = 0;
			sumMsg->type = RANGE;
			sumMsg->value1 = (i - 1)*chunkSize;
			sumMsg->value2 = MAX;
			SendMessage(i, sumMsg);
		}
		for(i = 1; i <= numThreads; i++) {
			Msg *genDoneMsg = malloc(sizeof(Msg));
			RecvMessage(0, genDoneMsg);
			if(genDoneMsg->type != GENDONE) {
				printf("[0] Msg type %d\nERROR: Main did not receive GENDONE from %d\n", genDoneMsg->type, genDoneMsg->iSender);
				exit(1);
			}
			#if _DEBUG
				printf("[0] Received GEDONE from %d\n", genDoneMsg->iSender);
			#endif
		}
	} else {
		for(i = 1; i <= numThreads; i++) {
			#if _DEBUG
				printf("[0] Sending GO to %d\n", i);
			#endif
			Msg *genGoMsg = malloc(sizeof(Msg));
			genGoMsg->iSender = 0;
			genGoMsg->type = GO;
			SendMessage(i, genGoMsg);
		}

		for(i = 1; i <= numThreads; i++) {
			Msg *genDoneMsg = malloc(sizeof(Msg));
			RecvMessage(0, genDoneMsg);
			if(genDoneMsg->type != GENDONE) {
				printf("[0] Msg type %d\nERROR: Main did not receive GENDONE from %d\n", genDoneMsg->type, genDoneMsg->iSender);
				exit(1);
			} else {
				#if _DEBUG
					printf("[0] Received GENDONE from %d\n", genDoneMsg->iSender);
				#endif
			}
		}
	}
    
}

// Consumer loop (worker thread logic)
/*!
   Runs one generation
   @param index thread id / mailbox number, passed in by pthread_create
*/
void cloop(int index) {
    Msg *msg = malloc(sizeof(Msg));
	int startRow, endRow;


	while(1) {
	    RecvMessage(index, msg);
		#if _DEBUG
			printf("[%d] Message received\n", index);
		#endif
		if(msg->type == RANGE) {
			startRow = msg->value1;
			endRow = msg->value2;
			#if _DEBUG
				printf("[%d] Received RANGE (%d, %d)\n", index, startRow, endRow);
			#endif
		} else if(msg->type == ALLDONE) {
			#if _DEBUG
				printf("[%d] Received ALLDONE\n", index);
			#endif
			return;
		} else if(msg->type == GO) {
			#if _DEBUG
				printf("[%d] Received GO\n", index);
				printf("[%d] Playing round\n", index);
			#endif
			// print2Dchar(gridB, rows, columns);
			// Else, message is GO
			playOne(startRow, endRow, gridB, gridA);
		} else {
			printf("[%d] ERROR: Received %d\n", msg->type);
			fflush(stdout);
		}
		#if _DEBUG
			printf("[%d] Sending GENDONE\n", index);
		#endif
		Msg *sumMsg =  malloc(sizeof(Msg));
		sumMsg->iSender = index;
		sumMsg->type = GENDONE;
		SendMessage(0, sumMsg);
	}
}

/*!
   Copies a Msg
   @param from pointer to source message
   @param to pointer to destination message
*/
void msgCpy(Msg *from, Msg *to) {
    to->iSender = from->iSender;
    to->type = from->type;
    to->value1 = from->value1;
    to->value2 = from->value2;
    from->value1 = 0;
    from->value2 = 0;
}

/*!
   Sends message to target mailbox
   @param iTo mailbox number to send to
   @param msg pointer to message to be sent
*/
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

/*!
   Reads message from mailbox
   @param iFrom mailbox number to read from
   @param msg pointer to message to store received data in
*/
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

/*!
   Loads a starting grid from the given input file into a char array
   @param input The input file
   @param dest The char array to load the file into
	 				**Must be of size rows x columns
					**Will be overwritten
	 @param rows Number of rows in array (file must have smaller/equal dimensions)
	 @param columns Number of columns in array (file must have smaller/equal dimensions)
   @return 1 if loaded successfully, else 0
*/
char **loadStartGrid(FILE *input, char **dest, int *rows, int *columns) {
	char **init = make2Dchar(40, 40); // Initial char array to load file into
	int x = 0, // Current row index in array
			y = 0,// Current column index in array
			yMax = 0;// Highest column index so far (for centering)
	char c = ' ';// char buffer for reading from file
	
	
	while(1) {
		// Get the next char from the file
		c = fgetc(input);
		// If we've hit the end of the file
		if(c == EOF) {
			// break out of the loop
			break;
		// If we've hit a newline
		} else if(c == '\n') {
			// Update max column index if applicable
			yMax = (y > yMax) ? y : yMax;
			// Move down a row and go back to the first column
			x += 1;
			y = 0;
			// Done with this iteration of the loop, start reading in next line of file
			continue;
		} else if(c == ' ') {
			continue;
		}
		// If none of the above conditions are met, put the char in the array
		init[x][y] = c;
		// NOTE: This can result in non-'1' or '0' chars ending up in the array if
		// input is malformed. This is handled below

		// Next column
		y++;
	}

	*rows = x;
	*columns = yMax - 1;
	dest = make2Dchar(*rows, *columns);

	// Calculate offsets to center input state in grid of argumented dimensions
	int xOffset = 0;// row offset
	int yOffset = 0;// column offset

	for(x = 0; x < *rows; x++) {
		for(y = 0; y < *columns; y++) {
			// If pos x,y falls within bounds of offset array
			if(x - xOffset >= 0 && y - yOffset >=0) {
				// Set the corresponding value in the destination array
				char c = init[x - xOffset][y - yOffset];
				dest[x][y] = (c=='1') ? '1' : '0';
				// This ensures that only '1' or '0' will appear in the destination array
				// All non-x characters from input file are treated as '0'
			} else {
				// Fill empty space created by offset
				dest[x][y] = '0';
			}
		}
	}
	// Clean up the input arr now that it has been centered in the destination arr
	free2Dchar(init, 39);

	// Success!
	return dest;
}

/*!
   Simulate one round of game of life
   @param rows Number of rows in grid
   @param columns Number of columns in grid
   @param old previous generation
	 @param new current generation (being generated)
*/
void playOne (int startRow, int endRow, char **old, char **new) {
	
	int x, y;
	for(x = startRow; x < endRow; x++) {
		for(y = 0; y < columns; y++) {
			// Get count of adjecent live cells
			int neighbors = getAdjecentLifeCount(old, rows, columns, x, y);
			// If currently selected cell is a live cell
			if(old[x][y] == '1') {
				switch(neighbors) {
					case 2:
					case 3:
						// Survival
						new[x][y] = '1';
						break;
					case 0:
					case 1:
					case 4:
					case 5:
					case 6:
					case 7:
					case 8:
					default:
						// DEATH!
						new[x][y] = '0';
						break;
				}
			// Else, current cell is empty and will either
			// birth (if it has neighbores) or remain empty
			} else if(neighbors == 3) {
				// Birth
				new[x][y] = '1';
			} else {
				// Empty
				new[x][y] = '0';
			}
		}
	}
}  // playOne

/*!
   Counts adjecent cells of a given cell
   @param grid the current game state
   @param rows Number of rows in grid
	 @param cols Numbe rof columns in grid
	 @param x Row index of current cell in grid
	 @param y Column index of current cell in grid
   @return Return number of living cells adjecent to current cell
*/
int getAdjecentLifeCount(char **grid, int rows, int cols, int x, int y) {
	// Define simple function for bounds check to improve readability
	// Note: annotations omitted to prevent malformed docs
	/*
	   Determines whether position x,y is within bounds of grid
	   param x x-pos passed to parent function
	   param y y-pos passed to parent function
	   return Return 1 if in bounds, else 0
	*/
	int inBounds(int x, int y) {
		return (x >= 0 && y >= 0  && x < rows && y < cols);
	}
	int count = 0; // Count of adjecent live cells
	// Each of these adds 1 (for live cell) or 0 (for empty/ out of bounds) to count
	// for an adjecent cell position
	count += inBounds(x-1, y-1) && grid[x-1][y-1] == '1';
	count += inBounds(x-1, y) && grid[x-1][y] == '1';
	count += inBounds(x-1, y+1) && grid[x-1][y+1] == '1';
	count += inBounds(x, y-1) && grid[x][y-1] == '1';
	count += inBounds(x, y+1) && grid[x][y+1] == '1';
	count += inBounds(x+1, y-1) && grid[x+1][y-1] == '1';
	count += inBounds(x+1, y) && grid[x+1][y] == '1';
	count += inBounds(x+1, y+1) && grid[x+1][y+1] == '1';

	return count;
} // getAdjecentLifeCount
