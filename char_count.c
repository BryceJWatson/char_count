/*char_count.c
Multi process program that creates a ring of processes to process some files and
count the number of characters

Author: Bryce Watson
Student ID: 220199390

Parameters:
  1. The number of processes to create
  2. The directory on which to process

Returns 0 on success

To build it use: make

To run: make run (this will run the program with 4 processes on a directory
named text_files) OR char_count 4 text_files

to clean:
  make clean

Run example:
  ./char_count 4 text_files
*/

/****** Included libraries ******/

#include <ctype.h>		/* for tolower */
#include <dirent.h>		/* for directory handling */
#include <stdio.h>		/* for fprintf  */
#include <stdlib.h>		/* for exit     */
#include <string.h>		/* for strtok   */
#include <unistd.h>		/* for fork     */

// helpers.h is my header file for my library of helper functions for this
// assignment
#include "helpers.h"

/****** Global Variables ******/

// Directory stream
DIR *TEXT_DIR;

/* This is a pointer to an array of pointers, which each point to the first
character in a string. It is essentially a dynamic array of strings, and we will
use it to store the filenames read from the directory stream.
*/
char **FILENAMES = NULL;

/* Simple exit handler function that will free any allocated memory upon exit,
 * is void so doesn't return anything and also doesnt accept any function
 * arguments*/
void
cleanup ()
{
  if (TEXT_DIR != NULL)
    {
      // close directory stream
      closedir (TEXT_DIR);
    }
  // free dynamically allocated array
  if (FILENAMES != NULL)
    {
      for (int i = 0; FILENAMES[i] != NULL; i++)
	{
	  free (FILENAMES[i]);
	}
      free (FILENAMES);
    }
}

/* Main Function: Parameters: integer argc which is the number of command line
 * arguments, and an array of pointers to the command line arguments *argv[]
 Returns 0 on success, 1 on failure*/

int
main (int argc, char *argv[])
{
  // Register exit handler function
  atexit (cleanup);

  int i;			/* number of this process (starting with 1)   */
  int childpid;			/* indicates process should spawn another     */
  int nprocs;			/* total number of processes in ring          */
  char buff[BUFFSIZE];		/* buffer that we will read and write to */
  long char_stats[CHARACTERS] = {
    0
  };				/*Initially empty array of 26 long numbers, (this is where we will
				   store our character counts) */
  struct dirent *entry;		/*Data structure that contains the attributes of each entry, we
				   will use this to get the name of each entry in the directory */
  int num_files = 0;		/*Counter for number of files used for dynamic array */
  int max_files = 10;		/* Initial max size of the FILENAMES array */
  char *text_dir = argv[2];	/* */
  long char_count = 0;		/* Total count of characters */

  // parse command line arguments
  if (parse_args (argc, argv, &nprocs, &TEXT_DIR) < 0)
    exit (EXIT_FAILURE);

  /* dynamically allocate array of FILENAMES so the program can handle an
   * arbitrary number of text files */
  FILENAMES = malloc ((max_files + 1) * sizeof (char *));
  if (FILENAMES == NULL)
    {
      perror ("Malloc failed");
      exit (EXIT_FAILURE);
    }

  // read each entry in the directory stream
  while ((entry = readdir (TEXT_DIR)) != NULL)
    {
      // Skip . and .. entries
      if (strcmp (entry->d_name, ".") == 0
	  || strcmp (entry->d_name, "..") == 0)
	{
	  continue;
	}

      // allocate memory for filename in array of filenames
      FILENAMES[num_files] =
	malloc ((strlen (entry->d_name) + 1) * sizeof (char));
      if (FILENAMES[num_files] == NULL)
	{
	  perror ("Malloc failed");
	  exit (EXIT_FAILURE);
	}

      // Copy the filename to the array
      strcpy (FILENAMES[num_files], entry->d_name);
      num_files++;

      // Resize the array if needed
      if (num_files == max_files)
	{
	  max_files *= 2;	// double the capacity
	  FILENAMES = realloc (FILENAMES, (max_files + 1) * sizeof (char *));
	  if (FILENAMES == NULL)
	    {
	      perror ("Realloc failed");
	      exit (EXIT_FAILURE);
	    }
	}
    }

  // set null pointer to indicate end of array
  FILENAMES[num_files] = NULL;

  // make trivial ring, (1 process connected to itself basically)
  if (make_trivial_ring () < 0)
    {
      perror ("Could not make trivial ring");
      exit (EXIT_FAILURE);
    };

  // add new nodes to ring, opening and closing pipes as needed
  for (i = 1; i < nprocs; i++)
    {
      if (add_new_node (&childpid) < 0)
	{
	  perror ("Could not add new node to ring");
	  exit (EXIT_FAILURE);
	};
      // If you as a process have spawned a child then exit the loop
      if (childpid)
	break;
    };
  /* ring process code  */

  /* Each process will iterate through the array of filenames starting from
   * their process rank. Then we will increment each process's position index in
   * the list of filenames by the number of processes. e.g if there are 4
   * processes, process 1 will work on the files at position 0, 4, 8...etc.*/

  for (int j = i - 1; j < num_files; j += nprocs)
    {
      char_count += procFileCounts (FILENAMES[j], char_stats, text_dir);
      /*Uncomment the fprintf line if you want to see what files each process is
       * working on */
      // fprintf(stderr, "\nHello I am process %d and i am working on file %s\n",
      // i,
      //         FILENAMES[j]);
    }

  /* Mother of all processes: Will send their count down the ring, and then wait
   * for the last process in the ring to communicate their result before
   * displaying it */
  if (i == 1)
    {
      send_count (buff, char_stats, char_count);
      long final_count[CHARACTERS] = { 0 };
      long total_count = 0;
      aggregate_count (buff, final_count, &total_count);
      size_t size = sizeof (final_count) / sizeof (final_count[0]);
      long max_count = find_max_count (final_count, size);
      display_results (final_count, total_count, nprocs, i, max_count);
    }

  /* All other processes: Will wait to recieve a count from their neighbour
   * process, after which will add it to their count before sending it on */
  if (i > 1)
    {
      aggregate_count (buff, char_stats, &char_count);

      send_count (buff, char_stats, char_count);
    }
  /* After I have done my job I exit */
  exit (EXIT_SUCCESS);
}
