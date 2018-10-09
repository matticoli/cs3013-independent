# Addem

## Program summary
This program adds all numbers between 1 and a given n, splitting the work among multiple worker threads

## Running
From within the source directory, compile and run using the following commands:
  ```
  make all
  ./addem numThreads numToSum
  ```
- _numThreads_: Number of worker threads to use
- _numToSum_: Number to add up to


# Game of Life

## Program summary
This program simulates Conway's game of life within a finite grid, distributing generation calculations between multiple worker threads

## Running
From within the source directory, compile and run using the following commands:
  ```
  make all
  ./life threads inputFile generations [print] [pause]
  ```
- _threads_: Number of worker threads to use
- _inputFile_: Text file with starting configuration (1 for live, 0 for empty)
- _generations_: Max number of generations to simulate
  - example:
    ```
    1011
    01
    ```
- _print_: `y` or `n` whether to print out every generation
- _pause_: `y` or `n` whether to pause between every generation
