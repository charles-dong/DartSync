/*
 * =====================================================================================
 *
 *       Filename:  common.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/10/2014 18:30:25
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author: 
 *   Organization:  Dartmouth College - Department of Computer Science
 *
 * =====================================================================================
 */

#ifndef  common_INC
#define  common_INC

// ---------------- Prerequisites e.g., Requires "math.h"
#include <stdio.h>                           // fprintf
#include <stdlib.h>                          // exit
#include <sys/stat.h>


// ---------------- Constants
#define TRUE      1
#define FALSE     0

// ---------------- Structures/Types

// ---------------- Public Variables

// ---------------- Prototypes/Macros

// Print s with the source file name and the current line number to fp.
#define LOG(fp,s)  fprintf((fp), "Log: [%s:%d] %s.\n", __FILE__, __LINE__, (s))

// Print error to fp and exit if s evaluates to false.
#define ASSERT_FAIL(fp,s) do {                                          \
        if(!(s))   {                                                    \
            fprintf((fp), "Error: [%s:%d] assert failed.\n", __FILE__, __LINE__); \
            exit(EXIT_FAILURE);                                         \
        }                                                               \
    } while(0)

// Print error to fp and exit if s is NULL.
#define MALLOC_CHECK(fp,s)  do {                                        \
        if((s) == NULL)   {                                             \
            fprintf((fp), "Error: [%s:%d] MALLOC_CHECK size 0. Possible error?\n", __FILE__, __LINE__); \
        }                                                               \
    } while(0)

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  mk_path
 *  Description:  Recursively creates directory tree of the passed in 
 *  file_path with given mode. 
 *
 *  E.g.
 *  char file_path[1000];
 *  memset(file_path, 0, 1000);
 *  strcpy(file_path, "./shu/secretstuff/test.dat");
 *  mkpath(file_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
 *
 *  Will create ./shu/secretstuff/ with u+rwx, g+rx, o+rx.
 * =====================================================================================
 */
int mk_path(char *file_path, mode_t mode);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  copy_file
 *  Description:  Copys the file at src (absolute path) to dest (absolute
 *  path).
 * =====================================================================================
 */
int copy_file(char *dest, char *src);

#endif   /* ----- #ifndef common_INC  ----- */
