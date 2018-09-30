/**
 * twoD.h
 *
 * @author: Mike Ciaraldi
 * @author Mikel Matticoli
 */

#ifndef TWOD_H_
#define TWOD_H_

// Function prototypes: (docs in source)
int** make2Dint(int rows, int columns);
char** make2Dchar(int rows, int columns);
void print2Dchar(char **arr, int rows, int cols);
void copy2Dchar(char **source, char **dest, int rows, int cols);
int match2Dchar(char **a, char **b, int rows, int cols);
void free2Dchar(char **arr, int rows);


#endif /* 2D_H_ */
