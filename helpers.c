/* Source file for library functions used in assignment 1, for the header file
please see helpers.h

Author: Bryce Watson
Student ID: 220199390*/

#include "helpers.h"

#include <ctype.h>  /* for tolower */
#include <dirent.h> /* for directory handling */
#include <stdio.h>  /* for fprintf  */
#include <stdlib.h> /* for exit     */
#include <string.h> /* for strtok   */
#include <unistd.h> /* for fork     */

/**
 * Creates a trivial ring with a single process connected to itself.
 *
 * This function creates a trivial ring structure with a single process
 * connected to itself with a pipe. This function is the inital step to creating
 * a ring of processes. The calling process forms a closed ring structure by
 * having its input connected to its own output, effectively creating a basic
 * ring structure.
 *
 * Returns: 0 on success, a negative value on failure
 *
 */
int make_trivial_ring() {
  int fd[2];
  if (pipe(fd) == -1)
    return (-1);
  if ((dup2(fd[0], STDIN_FILENO) == -1) || (dup2(fd[1], STDOUT_FILENO) == -1))
    return (-2);
  if ((close(fd[0]) == -1) || (close(fd[1]) == -1))
    return (-3);
  return (0);
}

/**
 * Adds a new node/process to the ring.
 *
 * This function adds a new process to the existing ring of processes.
 * It forks off a new process and connects it to the ring via pipes.
 * The process ID of the new node is stored in pid.
 *
 * Parameters:
 * pid: A pointer to an integer that will store the process ID of the new node.
 *
 * Returns: 0 on success, -1 on failure.
 */
int add_new_node(int *pid) {
  int fd[2];
  if (pipe(fd) == -1)
    return (-1);
  if ((*pid = fork()) == -1)
    return (-2);
  if (*pid > 0 && dup2(fd[1], STDOUT_FILENO) < 0)
    return (-3);
  if (*pid == 0 && dup2(fd[0], STDIN_FILENO) < 0)
    return (-4);
  if ((close(fd[0]) == -1) || (close(fd[1]) == -1))
    return (-5);
  return (0);
}

/**
 * Parses the command line arguments to ensure they are correct and opens the
 * directory stream
 *
 * Parameters:
 * argc: The number of command line arguments (including the program name)
 * argv: An array of pointers to command line arguments
 * np: A pointer to an integer that will store the number of processes
 * dir: A pointer to a directory stream pointer that will store the open
 * directory stream
 *
 * Returns: 0 on succes, -1 on failure.
 */
int parse_args(int argc, char *argv[], int *np, DIR **dir) {
  if ((argc != 3) || ((*np = atoi(argv[1])) <= 0) ||
      (*dir = opendir(argv[2])) == NULL) {
    fprintf(stderr, "Usage: %s <nprocs> <text_dir>\n", argv[0]);
    return (-1);
  };
  return (0);
}

/**
 * Counts each character in the specified file.
 *
 * This function reads the contents of the file specified by inFile and updates
 * the character counts in the char_stats array. The char_stats array is an
 * array of long integers where each index represents the count of a specific
 * character. The text_dir parameter is a string which contains the name of the
 * directory.
 *
 * Parameters:
 * inFile: A pointer to a character array containing the name of the file to
 *  be processed.
 * char_stats: A pointer to a long integer array representing the
 *  character counts.
 * text_dir: A pointer to a character array text_dir which
 *  contains the name of the directory.
 *
 * Returns:
 *   The total character count on success. If an error occurs, the function
 * exits with an exit status of 1.
 */
long procFileCounts(char inFile[], long char_stats[], char *text_dir) {
  char cur_fname[MAX_FILENAME];
  strcpy(cur_fname, text_dir);
  strcat(cur_fname, "/");
  strcat(cur_fname, inFile);
  FILE *file = fopen(cur_fname, "r");
  if (file == NULL) {
    perror("Failed to open file");
    exit(EXIT_FAILURE);
  }
  char c;
  long char_count = 0;
  /* for as long as we can get characters... */
  while ((c = fgetc(file))) {
    /* break if end of file */
    if (c == EOF) {
      break;
    } else if ((tolower(c) - 'a') >= 0 && (tolower(c) - 'a') < CHARACTERS) {
      char_stats[tolower(c) - 'a'] += 1;
    } else {
      continue;
    }
    /* otherwise add one to the count of that particular character */
    char_count += 1;
  }
  if (fclose(file) != 0) {
    perror("Failed to close file");
    exit(EXIT_FAILURE);
  }
  return char_count;
}

/**
 * Finds the largest count from the counts in the array.
 *
 * This function takes the array of character counts and finds the largest
 * number. It is used to compute the histogram bar length in the display_results
 * function. The calculation in said function is (char_count/max_count) *
 * BARLENGTH. where max_count is the value returned by this function.
 *
 * Parameters:
 * array: A pointer to a array of long integers containing the character counts
 * size: The size of the array
 *
 * Returns:
 *   max_count, which is the largest number in the char_stats array, or highest
 * character count.
 */
long find_max_count(long array[], size_t size) {
  long max = array[0];

  for (int i = 1; i < size; i++) {
    if (array[i] > max) {
      max = array[i];
    }
  }

  return max;
}

/**
 * Aggregates character counts read from stdin into the provided character stats
 * array and count.
 *
 * This function reads character counts from the standard input (stdin) and
 * aggregates them into the `char_stats` array and `char_count`. The
 * `char_stats` array holds the individual counts for each character, and the
 * `char_count` represents the total character count. The function expects a
 * space-separated list of integers as input on stdin, where the first integer
 * corresponds to the total character count, followed by individual character
 * counts for each character represented in the `char_stats` array. The function
 * updates `char_stats` and `char_count` accordingly.
 *
 * Parameters:
 * buff: A character buffer used for reading from stdin
 * char_stats: An array of long integers representing the individual character
 * counts char_count: A pointer to a long integer representing the total
 * character count
 *
 */
void aggregate_count(char buff[], long char_stats[], long *char_count) {
  char *token = NULL;
  if ((char_stats == NULL) || (char_count == NULL))
    return;
  // Read from the buffer by getting tokens with the space delimitor
  if (read(STDIN_FILENO, buff, BUFFSIZE) > 0) {
    // Add char_count to mine
    *char_count += atol((((token = strtok(buff, " ")) == NULL) ? "0" : token));
    // add the individual character counts to mine
    for (int i = 0; i < CHARACTERS; i++) {
      char_stats[i] +=
          atol((((token = strtok(NULL, " ")) == NULL) ? "0" : token));
    }
  } else {
    *char_count = 0;
    for (int i = 0; i < CHARACTERS; i++) {
      char_stats[i] = 0;
    }
  }
}

/**
 * Sends character counts and the total count to stdout.
 *
 * This function prepares a formatted string containing the character counts
 * from the `char_stats` array and the total character count `char_count`. It
 * sends the formatted string to the standard output (stdout) using the provided
 * character buffer `buff`. The `char_stats` array holds individual counts for
 * each character, and the `char_count` represents the total character count.
 * The function formats the output as a space-separated list of integers, where
 * the first integer corresponds to the total character count, followed by
 * individual character counts for each character represented in the
 * `char_stats` array.
 *
 * Parameters:
 * buff: A character buffer used for preparing the output to be sent to stdout.
 * char_stats: An array of long integers representing individual character
 * counts.
 * char_count: A pointer to a long integer representing the total
 * character count.
 *
 * Returns: The number of bytes successfully written to stdout, or -1 on
 * failure.
 */
int send_count(char buff[], long char_stats[], long char_count) {
  int bytes, len;
  // send char_count first
  sprintf(buff, "%ld", char_count);
  // send all the individual character counts
  for (int i = 0; i < CHARACTERS; i++) {
    sprintf(buff + strlen(buff), " %ld", char_stats[i]);
  }

  // strlen + 1 for the null pointer/terminator
  len = strlen(buff) + 1;

  // write the buff to stdout
  if ((bytes = write(STDOUT_FILENO, buff, len)) != len) {
    fprintf(stderr, "Write of %d bytes failed, only sent %d bytes\n", len,
            bytes);
    return -1;
  } else {
    return bytes;
  }
}

/**
 * Displays the results of the processing in a way that mimics the assignment
 * output
 *
 * This function prints the expected output to stderr, including a statement
 * that the process has been completed on a specified number of processes, and
 * will print the individual character counts of each letter with a histogram
 * visualization.
 *
 * Parameters:
 * char_stats: An array of long integers representing individual character
 * counts.
 * char_count: A pointer to a long integer representing the total
 * character count.
 * nprocs: An integer value representing how many processes are being used.
 * process: An integer value representing which process is the calling process.
 * max_count: A long integer value representing the number of the largest count.
 *
 */
void display_results(long char_stats[], long char_count, int nprocs,
                     int process, long max_count) {
  fprintf(stderr, "\nProcessing complete on ring with %d processes\n\n",
          nprocs);

  for (int i = 0; i < CHARACTERS; i++) {
    // determine the letter by its ASCII value
    char letter = 'a' + i;
    // determine histogram size with floating point division
    int hist_size = ((char_stats[i] * 1.0 / max_count) * BARLENGTH);

    // determine number of '*' characters to print
    int max_width = 0;
    for (int i = 0; i < CHARACTERS; i++) {
      int width = snprintf(NULL, 0, "%ld", char_stats[i]);
      if (width > max_width) {
        max_width = width;
      }
    }

    // print character counts
    fprintf(stderr, "Process %d got char %c: %-*ld | ", process, letter,
            max_width, char_stats[i]);

    // print histogram
    for (int j = 0; j < hist_size; j++) {
      fprintf(stderr, "*");
    }

    // Add newline once done
    if (!hist_size)
      fprintf(stderr, "*");
    fprintf(stderr, "\n");
  }
}
