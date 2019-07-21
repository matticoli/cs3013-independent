#include <stdio.h>
#include <stdlib.h>
#include "twoD.h"

/** Make a 2D array of integers
 *
 * @param rows Number of rows
 * @param columns Number of columns
 * @return Pointer to the array of pointers to the rows.
 * 	  or null pointer if unable to allocate memory.
 * 	  Note: Will not free partially-allocated memory.
 *
 * @author Mike Ciaraldi
 * @author Mikel Matticoli
 */
int** make2Dint(int rows, int columns) {

	int **a; // Array of pointers to rows
	unsigned int i; // Loop counter

	// First allocate the array of pointers to rows
	a = (int **) malloc(rows * sizeof(int *));
	if (!a) { // Unable to allocate the array
		return (int **) NULL;
	}

	// Now allocate array for each row
	for (i = 0; i < rows; i++) {
		// i is the row we are about to allocate
		a[i] = malloc(columns * sizeof (int));
		if (!a[i]) {
			return (int **) NULL; // Unable to allocate
		}
	}
	return a;
}

/** Make a 2D array of characters
 *
 * @param rows Number of rows
 * @param columns Number of columns
 * @return Pointer to the array of pointers to the rows.
 * 	  or null pointer if unable to allocate memory.
 * 	  Note: Will not free partially-allocated memory.
 *
 */
char** make2Dchar(int rows, int columns) {
	char **init = malloc(sizeof(char *) * rows);
	/* Invariants (nested loop iteration):
	  - Pre: Current index of 2d array is unallocated
		- Post: current index of 2d array is allocated, and all chars
		are initialized to ' '.
	*/
	// For each row (sub-array)
	int i, j;
	for(i = 0; i < rows; i++) {
		// Init a 1D char array
		init[i] = malloc(sizeof(char) * columns);
		if (!init[i]) {
			return (char **) NULL; // Unable to allocate
		}
		// For each char in subarray
		for(j = 0; j < columns; j++) {
			init[i][j] = ' ';
		}
	}
	return init;
}

/*!
    Prints a 2D character array to stdout as it appears in memory
   @param arr 2D char array to print
   @param rows Number of rows in arr
   @param cols Number of columns in arr
*/
void print2Dchar(char **arr, int rows, int cols) {
	/* Invariants (nested loop iteration):
	  - Pre: x and y are within bounds of arr, arr[x][y] is a defined character
		- Post: char[x][y] have been printed to stdout, arr is unmutated
	*/
	int x, y;
	for(x = 0; x < rows; x++) {
		for(y = 0; y < cols; y++) {
			// Print out char at current index
			printf("%c", arr[x][y]);
		}
		// If end of row, print newline
		printf("\n");
	}
}

/*!
   Deep copies one 2d char array to another
   @param source The array to copy from
	 @param dest The array to copy to
	 @param rows Number of rows in source and dest
	 @param cols Number of columns in source and dest
*/
void copy2Dchar(char **source, char **dest, int rows, int cols) {
	/* Invariants (nested loop iteration):
	  - Pre: source and dest are same-size initialized char arrays
		- Post: dest is a deep copy of source (values match but not locations)
	*/
	int x, y;
	for(x = 0; x < rows; x++) {
		for(y = 0; y < cols; y++) {
			dest[x][y] = source[x][y];
		}
	}
}

/*!
   Deep copies one 2d char array to another
   @param source The array to copy from
	 @param dest The array to copy to
	 @param rows Number of rows in source and dest
	 @param cols Number of columns in source and dest
*/
int match2Dchar(char **a, char **b, int rows, int cols) {
	int x, y;
	for(x = 0; x < rows; x++) {
		for(y = 0; y < cols; y++) {
			if(a[x][y] != b[x][y]) {
				return 0;
			}
		}
	}
	return 1;
}

/*!
   Frees memory held by 2D char array
   @param arr The array to remove from memory
	 @param rows Number of rows in arr
	 @param cols Number of columns in arr
*/
void free2Dchar(char **arr, int rows) {
	// For each subArray of arr
	int i;
	for(i = 0; i < rows; i++) {
		// Free memory held by array
		free(arr[i]);
		// There's no need to free each char individually, since free() will clear
		// out all memory locations contained by the char array (aka string) arr[i]
	}
	// Free root pointer
	free(arr);
}
