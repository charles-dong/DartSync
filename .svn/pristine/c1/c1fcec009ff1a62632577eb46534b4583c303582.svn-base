/*
 * =====================================================================================
 *
 *       Filename:  common.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/18/2014 15:35:03
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Shuyang Fang (Shu), sfang@cs.dartmouth.edu
 *   Organization:  Dartmouth College - Department of Computer Science
 *
 * =====================================================================================
 */

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"

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
int mk_path(char *file_path, mode_t mode)
{
  assert(file_path && *file_path);
  char* p;
  for (p=strchr(file_path+1, '/'); p; p=strchr(p+1, '/')) {
    *p='\0';
    if (mkdir(file_path, mode)==-1) {
      if (errno!=EEXIST) { *p='/'; return -1; }
    }
    *p='/';
  }
  return 0;
}		/* -----  end of function mk_path  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  copy_file
 *  Description:  Copys the file at src (absolute path) to dest (absolute 
 *  path).
 * =====================================================================================
 */
int copy_file(char *dest, char *src)
{
  FILE *source, *target;
  char ch;

  if (!(source = fopen(src, "r"))) {
    LOG(stderr, "Unable to open src file to copy!");
    return -1;
  }

  if (!(target = fopen(dest, "w"))) {
    fclose(source);
    LOG(stderr, "Unable to open dest file to write!");
    return -1;
  }

  while ((ch = fgetc(source)) != EOF)
    fputc(ch, target);

  fclose(source);
  fclose(target);

  return 0;
}		/* -----  end of function copy_file  ----- */
